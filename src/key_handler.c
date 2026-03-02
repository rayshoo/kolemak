/*
 * key_handler.c - ITfKeyEventSink implementation
 *
 * Handles keyboard input: applies Colemak mapping for English,
 * feeds Dubeolsik jamo to the Hangul engine for Korean.
 */

#include "kolemak.h"
#include "settings.h"

/* Helper: request an edit session */
static HRESULT RequestEditSession(TextService *ts, ITfContext *ctx,
                                   EditSessionType type, EditSession *es)
{
    HRESULT hr;
    HRESULT hrSession;

    (void)type;

    hr = ctx->lpVtbl->RequestEditSession(
        ctx, ts->clientId,
        (ITfEditSession *)es,
        TF_ES_SYNC | TF_ES_READWRITE,
        &hrSession);

    /* If sync not granted, fall back to async */
    if (hr == TF_E_SYNCHRONOUS) {
        hr = ctx->lpVtbl->RequestEditSession(
            ctx, ts->clientId,
            (ITfEditSession *)es,
            TF_ES_ASYNC | TF_ES_READWRITE,
            &hrSession);
    }

    return SUCCEEDED(hr) ? hrSession : hr;
}

/* Sync internal CapsLock state to OS thread-local state and registry */
static void SyncCapsLockState(TextService *ts)
{
    BYTE ks[256];
    HKEY hKey;

    /* 1. OS thread-local state (for QWERTY passthrough mode) */
    GetKeyboardState(ks);
    if (ts->capsLockOn)
        ks[VK_CAPITAL] |= 1;
    else
        ks[VK_CAPITAL] &= ~1;
    SetKeyboardState(ks);

    /* 2. Registry (for cross-process sync) */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KOLEMAK_REG_KEY,
                      0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        DWORD val = ts->capsLockOn ? 1 : 0;
        RegSetValueExW(hKey, KOLEMAK_REG_CAPSLOCK_STATE, 0, REG_DWORD,
                       (const BYTE *)&val, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

/* ===== WH_GETMESSAGE hook for modifier+key Colemak VK remapping =====
 *
 * Remaps VK codes in WM_KEYDOWN/WM_SYSKEYDOWN messages BEFORE TSF's
 * keystroke manager processes them.  This ensures Colemak shortcuts
 * work correctly in Korean mode, where TSF may skip OnTestKeyDown
 * for Ctrl+letter combinations. */
LRESULT CALLBACK KolemakGetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && wParam == PM_REMOVE) {
        MSG *msg = (MSG *)lParam;
        if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) {
            TextService *ts = (TextService *)TlsGetValue(g_tlsIndex);
            if (ts) {
                BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                BOOL alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
                BOOL win  = ((GetKeyState(VK_LWIN) & 0x8000) |
                             (GetKeyState(VK_RWIN) & 0x8000)) != 0;
                if (ctrl || alt || win) {
                    UINT vk = (UINT)msg->wParam;
                    UINT mods = 0;
                    UINT effectiveVk;
                    BOOL isRepeat = (msg->lParam >> 30) & 1;

                    if (ctrl) mods |= TF_MOD_CONTROL;
                    if (alt)  mods |= TF_MOD_ALT;
                    if (GetKeyState(VK_SHIFT) & 0x8000) mods |= TF_MOD_SHIFT;

                    /* Effective VK after Colemak remap (same as raw in QWERTY) */
                    effectiveVk = ts->colemakMode
                        ? keymap_get_colemak_vk(vk) : vk;

                    /* Handle Colemak toggle hotkey directly in the hook.
                     * TSF's PreservedKey doesn't see hook-modified wParam,
                     * so we must handle the toggle here. */
                    if (!isRepeat &&
                        effectiveVk == ts->hotkeyVk &&
                        mods == ts->hotkeyModifiers) {
                        if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
                            HangulResult result =
                                hangul_ic_flush(&ts->hangulCtx);
                            ITfDocumentMgr *docMgr = NULL;
                            if (ts->threadMgr &&
                                SUCCEEDED(ts->threadMgr->lpVtbl->GetFocus(
                                    ts->threadMgr, &docMgr)) &&
                                docMgr) {
                                ITfContext *ctx = NULL;
                                if (SUCCEEDED(docMgr->lpVtbl->GetTop(
                                        docMgr, &ctx)) && ctx) {
                                    EditSession *es = NULL;
                                    HRESULT hr = EditSession_Create(ts, ctx,
                                        ES_HANDLE_RESULT, &es);
                                    if (SUCCEEDED(hr)) {
                                        es->data.hangulResult = result;
                                        RequestEditSession(ts, ctx,
                                            ES_HANDLE_RESULT, es);
                                        es->lpVtbl->Release(
                                            (ITfEditSession *)es);
                                    }
                                    ctx->lpVtbl->Release(ctx);
                                }
                                docMgr->lpVtbl->Release(docMgr);
                            }
                        }
                        ts->colemakMode = !ts->colemakMode;
                        KolemakTooltip_Show(
                            ts->colemakMode ? L"Colemak" : L"QWERTY");
                        if (ts->langBarButton)
                            LangBarButton_UpdateState(ts->langBarButton);
                        msg->message = WM_NULL;
                        return CallNextHookEx(NULL, code, wParam, lParam);
                    }

                    /* Remap modifier+alpha for Colemak shortcuts */
                    if (ts->colemakMode && effectiveVk != vk) {
                        UINT newScan = MapVirtualKey(effectiveVk,
                                                     MAPVK_VK_TO_VSC);
                        msg->wParam = effectiveVk;
                        msg->lParam = (msg->lParam & ~(0xFFu << 16))
                                    | ((LPARAM)newScan << 16);
                    }
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

/* Check if a key should be eaten */
static BOOL ShouldEatKey(TextService *ts, UINT vk, BOOL shift)
{
    /* Remapped CapsLock (F13): always eat regardless of mode */
    if (vk == VK_F13)
        return TRUE;

    /* CapsLock → Backspace: eat CapsLock when enabled (pre-reboot compat) */
    if (ts->capsLockAsBackspace && vk == VK_CAPITAL)
        return TRUE;

    /* When a modifier (Ctrl/Alt/Win) is held, VK remapping is already
     * handled by the WH_GETMESSAGE hook (KolemakGetMsgProc).
     * Only eat the key if Korean composition needs flushing. */
    {
        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        BOOL alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
        BOOL win  = ((GetKeyState(VK_LWIN) & 0x8000) |
                     (GetKeyState(VK_RWIN) & 0x8000)) != 0;
        if (ctrl || alt || win) {
            if (ts->koreanMode && ts->hangulCtx.state != HANGUL_STATE_EMPTY)
                return TRUE;   /* Eat to flush composition in OnKeyDown */
            return FALSE;
        }
    }

    /* QWERTY mode with no Korean: pass everything through */
    if (!ts->colemakMode && !ts->koreanMode)
        return FALSE;

    /* Always eat letter keys (for Colemak remap or Korean input) */
    if (vk >= 'A' && vk <= 'Z')
        return TRUE;

    /* Eat semicolon key: in English Colemak mode (maps ; -> O),
     * or in Korean Colemak mode with semicolonSwap (maps ; -> ㅔ) */
    if (vk == VK_OEM_1) {
        if (ts->colemakMode && !ts->koreanMode)
            return TRUE;
        if (ts->koreanMode && ts->colemakMode && ts->semicolonSwap)
            return TRUE;
    }

    /* Eat backspace during active composition */
    if (vk == VK_BACK && ts->hangulCtx.state != HANGUL_STATE_EMPTY)
        return TRUE;

    /* Eat modifier keys during active composition to prevent
     * the browser/app from terminating composition on Shift press */
    if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
        if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT)
            return TRUE;
    }

    /* Eat Enter/Escape to flush composition */
    if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
        if (vk == VK_RETURN || vk == VK_ESCAPE)
            return TRUE;
    }

    /* Eat navigation keys to flush composition */
    if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
        if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
            vk == VK_HOME || vk == VK_END || vk == VK_DELETE || vk == VK_TAB)
            return TRUE;
    }

    /* Eat all remaining keys during composition (space, numbers,
     * punctuation etc.) to ensure proper flush before key delivery.
     * Without this, async edit sessions on Win10 cause misordering.
     * Exclude Ctrl/Alt/Win so modifier shortcuts (Ctrl+A etc.) work. */
    if (ts->hangulCtx.state != HANGUL_STATE_EMPTY &&
        vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL &&
        vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU &&
        vk != VK_LWIN && vk != VK_RWIN)
        return TRUE;

    (void)shift;
    return FALSE;
}

/* Process a key in Korean mode */
static HRESULT HandleKoreanKey(TextService *ts, ITfContext *ctx,
                                UINT vk, BOOL shift)
{
    JamoMapping jamo;
    HangulResult result;
    EditSession *es = NULL;
    HRESULT hr;

    jamo = keymap_get_jamo(vk, shift, ts->colemakMode && ts->semicolonSwap);

    if (jamo.cho < 0 && jamo.jung < 0) {
        /* Not a jamo key. If composing, flush first then pass through. */
        if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
            result = hangul_ic_flush(&ts->hangulCtx);
            hr = EditSession_Create(ts, ctx, ES_HANDLE_RESULT, &es);
            if (SUCCEEDED(hr)) {
                es->data.hangulResult = result;
                RequestEditSession(ts, ctx, ES_HANDLE_RESULT, es);
                es->lpVtbl->Release((ITfEditSession *)es);
            }
        }
        return S_FALSE; /* Let the key pass through */
    }

    result = hangul_ic_process(&ts->hangulCtx, jamo.cho, jamo.jung);

    if (result.type == HANGUL_RESULT_PASS)
        return S_FALSE;

    hr = EditSession_Create(ts, ctx, ES_HANDLE_RESULT, &es);
    if (FAILED(hr)) return hr;

    es->data.hangulResult = result;
    RequestEditSession(ts, ctx, ES_HANDLE_RESULT, es);
    es->lpVtbl->Release((ITfEditSession *)es);

    return S_OK;
}

/* Process a key in English mode - uses SendInput for correct ordering */
static HRESULT HandleEnglishKey(TextService *ts, ITfContext *ctx,
                                 UINT vk, BOOL shift)
{
    WCHAR ch;
    INPUT inputs[2] = {0};

    (void)ctx;

    /* CapsLock inverts case for letter keys.
     * Include VK_OEM_1 (;) since Colemak maps it to O. */
    if (ts->capsLockOn && ((vk >= 'A' && vk <= 'Z') || vk == VK_OEM_1))
        shift = !shift;

    if (!keymap_get_colemak(vk, shift, &ch))
        return S_FALSE; /* Not a key we remap */

    /* Send Unicode character directly via SendInput.
     * This arrives as VK_PACKET which ShouldEatKey doesn't eat,
     * so no recursion. Ordering is guaranteed by SendInput. */
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = ch;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = ch;
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));

    return S_OK;
}

/* ===== ITfKeyEventSink IUnknown ===== */

static HRESULT STDMETHODCALLTYPE KES_QueryInterface(
    ITfKeyEventSink *pThis, REFIID riid, void **ppvObj)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
    return ts->lpVtbl->QueryInterface(
        (ITfTextInputProcessorEx *)ts, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE KES_AddRef(ITfKeyEventSink *pThis)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
    return ts->lpVtbl->AddRef((ITfTextInputProcessorEx *)ts);
}

static ULONG STDMETHODCALLTYPE KES_Release(ITfKeyEventSink *pThis)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
    return ts->lpVtbl->Release((ITfTextInputProcessorEx *)ts);
}

/* ===== ITfKeyEventSink methods ===== */

static HRESULT STDMETHODCALLTYPE KES_OnSetFocus(
    ITfKeyEventSink *pThis, BOOL fForeground)
{
    if (fForeground) {
        TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
        Settings_ReloadPrefs(ts);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KES_OnTestKeyDown(
    ITfKeyEventSink *pThis, ITfContext *pic,
    WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
    BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    (void)pic; (void)lParam;

    *pfEaten = ShouldEatKey(ts, (UINT)wParam, shift);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KES_OnTestKeyUp(
    ITfKeyEventSink *pThis, ITfContext *pic,
    WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    (void)pThis; (void)pic; (void)wParam; (void)lParam;

    *pfEaten = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KES_OnKeyDown(
    ITfKeyEventSink *pThis, ITfContext *pic,
    WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);
    UINT vk = (UINT)wParam;
    BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    HRESULT hr;

    (void)lParam;
    *pfEaten = FALSE;

    /* CapsLock handling: VK_F13 (remapped via Scancode Map) or
     * VK_CAPITAL (pre-reboot / no scancode map fallback) */
    if (vk == VK_F13 || (ts->capsLockAsBackspace && vk == VK_CAPITAL)) {
        /* VK_CAPITAL (pre-reboot): OS already toggled CapsLock, undo it */
        if (vk == VK_CAPITAL) {
            BYTE ks[256];
            GetKeyboardState(ks);
            ks[VK_CAPITAL] ^= 1;
            SetKeyboardState(ks);
        }

        if (ts->capsLockAsBackspace) {
            if (shift) {
                /* Shift+CapsLock: actual CapsLock toggle */
                ts->capsLockOn = !ts->capsLockOn;
                SyncCapsLockState(ts);
            } else {
                /* Backspace */
                if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
                    HangulResult result = hangul_ic_backspace(&ts->hangulCtx);
                    EditSession *es = NULL;

                    if (result.type == HANGUL_RESULT_COMPOSING) {
                        hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
                        if (SUCCEEDED(hr)) {
                            es->data.hangulResult = result;
                            RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
                            es->lpVtbl->Release((ITfEditSession *)es);
                        }
                    } else if (result.type == HANGUL_RESULT_COMMIT_FLUSH) {
                        hr = EditSession_Create(ts, pic, ES_CANCEL_COMPOSITION, &es);
                        if (SUCCEEDED(hr)) {
                            RequestEditSession(ts, pic, ES_CANCEL_COMPOSITION, es);
                            es->lpVtbl->Release((ITfEditSession *)es);
                        }
                    }
                } else {
                    INPUT bkInputs[2];
                    memset(bkInputs, 0, sizeof(bkInputs));
                    bkInputs[0].type = INPUT_KEYBOARD;
                    bkInputs[0].ki.wVk = VK_BACK;
                    bkInputs[0].ki.wScan =
                        (WORD)MapVirtualKey(VK_BACK, MAPVK_VK_TO_VSC);
                    bkInputs[1].type = INPUT_KEYBOARD;
                    bkInputs[1].ki.wVk = VK_BACK;
                    bkInputs[1].ki.wScan =
                        (WORD)MapVirtualKey(VK_BACK, MAPVK_VK_TO_VSC);
                    bkInputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
                    SendInput(2, bkInputs, sizeof(INPUT));
                }
            }
        } else {
            /* capsLockAsBackspace=FALSE: CapsLock toggle */
            ts->capsLockOn = !ts->capsLockOn;
            SyncCapsLockState(ts);
        }
        *pfEaten = TRUE;
        return S_OK;
    }

    /* Shift keys eaten during composition: swallow silently.
     * This prevents the browser from terminating composition on Shift press. */
    if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) {
        *pfEaten = (ts->hangulCtx.state != HANGUL_STATE_EMPTY);
        return S_OK;
    }

    /* Handle backspace during composition */
    if (vk == VK_BACK && ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
        HangulResult result = hangul_ic_backspace(&ts->hangulCtx);
        EditSession *es = NULL;

        if (result.type == HANGUL_RESULT_COMPOSING) {
            hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
            if (SUCCEEDED(hr)) {
                es->data.hangulResult = result;
                RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
                es->lpVtbl->Release((ITfEditSession *)es);
            }
        } else if (result.type == HANGUL_RESULT_COMMIT_FLUSH) {
            /* Backspace removed last jamo: cancel composition */
            hr = EditSession_Create(ts, pic, ES_CANCEL_COMPOSITION, &es);
            if (SUCCEEDED(hr)) {
                RequestEditSession(ts, pic, ES_CANCEL_COMPOSITION, es);
                es->lpVtbl->Release((ITfEditSession *)es);
            }
        }

        *pfEaten = TRUE;
        return S_OK;
    }

    /* Handle Enter: flush composition, end it, re-inject key.
     *
     * Two re-inject mechanisms are used simultaneously:
     *
     * 1) Immediate SendInput — delivers Enter during the current TSF
     *    key processing cycle.  Browsers and KakaoTalk process this
     *    for form submission / message send.
     *
     * 2) Async edit session with reinjectVk — delivers Enter from a
     *    callback that runs outside TSF's keystroke manager.  Games
     *    ignore the immediate SendInput but process this deferred one.
     *
     * Apps that handle the immediate Enter (browsers, KakaoTalk) will
     * see a second Enter from the async callback, but it arrives after
     * the action (page navigated / message sent), hitting an empty
     * input field — harmless. */
    if (vk == VK_RETURN && ts->hangulCtx.state != HANGUL_STATE_EMPTY)
    {
        HangulResult result = hangul_ic_flush(&ts->hangulCtx);
        EditSession *es = NULL;

        hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
        if (SUCCEEDED(hr)) {
            HRESULT hrSession;
            es->data.hangulResult = result;

            hr = pic->lpVtbl->RequestEditSession(
                pic, ts->clientId,
                (ITfEditSession *)es,
                TF_ES_SYNC | TF_ES_READWRITE,
                &hrSession);

            if (hr == TF_E_SYNCHRONOUS) {
                /* Sync not available: async handles EndComposition + reinject */
                es->reinjectVk = VK_RETURN;
                pic->lpVtbl->RequestEditSession(
                    pic, ts->clientId,
                    (ITfEditSession *)es,
                    TF_ES_ASYNC | TF_ES_READWRITE,
                    &hrSession);
                es->lpVtbl->Release((ITfEditSession *)es);
            } else {
                /* Sync succeeded: composition ended immediately.
                 * Queue async for deferred reinject (needed by games). */
                es->lpVtbl->Release((ITfEditSession *)es);
                {
                    EditSession *esR = NULL;
                    hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &esR);
                    if (SUCCEEDED(hr)) {
                        esR->data.hangulResult.type = HANGUL_RESULT_PASS;
                        esR->reinjectVk = VK_RETURN;
                        pic->lpVtbl->RequestEditSession(
                            pic, ts->clientId,
                            (ITfEditSession *)esR,
                            TF_ES_ASYNC | TF_ES_READWRITE,
                            &hrSession);
                        esR->lpVtbl->Release((ITfEditSession *)esR);
                    }
                }
            }
        }

        /* Immediate re-inject for apps that handle Enter during TSF
         * key processing (browsers, KakaoTalk).  Games ignore this;
         * the async callback above covers them. */
        {
            INPUT inputs[2] = {0};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_RETURN;
            inputs[0].ki.wScan =
                (WORD)MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_RETURN;
            inputs[1].ki.wScan =
                (WORD)MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }

        *pfEaten = TRUE;
        return S_OK;
    }

    /* Handle Escape: flush composition */
    if (vk == VK_ESCAPE && ts->hangulCtx.state != HANGUL_STATE_EMPTY)
    {
        HangulResult result = hangul_ic_flush(&ts->hangulCtx);
        EditSession *es = NULL;

        hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
        if (SUCCEEDED(hr)) {
            es->data.hangulResult = result;
            RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
            es->lpVtbl->Release((ITfEditSession *)es);
        }

        *pfEaten = TRUE;
        return S_OK;
    }

    /* Navigation keys: flush composition and re-inject key */
    if ((vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
         vk == VK_HOME || vk == VK_END || vk == VK_DELETE || vk == VK_TAB) &&
        ts->hangulCtx.state != HANGUL_STATE_EMPTY)
    {
        HangulResult result = hangul_ic_flush(&ts->hangulCtx);
        EditSession *es = NULL;

        hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
        if (SUCCEEDED(hr)) {
            es->data.hangulResult = result;
            es->reinjectVk = vk;  /* Re-inject after edit session completes */
            RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
            es->lpVtbl->Release((ITfEditSession *)es);
        }

        *pfEaten = TRUE;
        return S_OK;
    }

    /* Catch-all: flush composition for any remaining key (space, numbers,
     * punctuation, etc.) that was eaten by ShouldEatKey during composition.
     * Without this, async edit sessions on Win10 cause misordering. */
    if (ts->koreanMode &&
        ts->hangulCtx.state != HANGUL_STATE_EMPTY &&
        !(vk >= 'A' && vk <= 'Z') &&
        vk != VK_OEM_1 &&
        vk != VK_BACK &&
        vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT &&
        vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL &&
        vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU &&
        vk != VK_LWIN && vk != VK_RWIN)
    {
        HangulResult result = hangul_ic_flush(&ts->hangulCtx);
        EditSession *es = NULL;

        hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
        if (SUCCEEDED(hr)) {
            es->data.hangulResult = result;
            es->reinjectVk = vk;  /* Re-inject after edit session completes */
            RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
            es->lpVtbl->Release((ITfEditSession *)es);
        }

        *pfEaten = TRUE;
        return S_OK;
    }

    /* Modifier shortcuts: VK remapping is done by WH_GETMESSAGE hook.
     * Here we only flush Korean composition, then let the (already
     * remapped) key pass through to the application. */
    {
        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        BOOL alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
        BOOL win  = ((GetKeyState(VK_LWIN) & 0x8000) |
                     (GetKeyState(VK_RWIN) & 0x8000)) != 0;
        if (ctrl || alt || win) {
            if (ts->koreanMode &&
                ts->hangulCtx.state != HANGUL_STATE_EMPTY)
            {
                HangulResult result = hangul_ic_flush(&ts->hangulCtx);
                EditSession *es = NULL;
                hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
                if (SUCCEEDED(hr)) {
                    es->data.hangulResult = result;
                    RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
                    es->lpVtbl->Release((ITfEditSession *)es);
                }
            }
            *pfEaten = FALSE;
            return S_OK;
        }
    }

    /* Normal key processing */
    if (ts->koreanMode) {
        hr = HandleKoreanKey(ts, pic, vk, shift);
        /* If Korean handler didn't consume the key (e.g. VK_P with
         * semicolonSwap) and we're in Colemak mode, fall through to
         * English handler for Colemak character remapping (e.g. P→;).
         * Only for letter keys — VK_OEM_1 should pass through as ';'
         * when semicolonSwap is off, not be remapped to 'O'. */
        if (hr == S_FALSE && ts->colemakMode && vk >= 'A' && vk <= 'Z')
            hr = HandleEnglishKey(ts, pic, vk, shift);
    } else if (ts->colemakMode) {
        hr = HandleEnglishKey(ts, pic, vk, shift);
    } else {
        /* QWERTY mode: pass through */
        hr = S_FALSE;
    }

    *pfEaten = SUCCEEDED(hr) && hr != S_FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KES_OnKeyUp(
    ITfKeyEventSink *pThis, ITfContext *pic,
    WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    (void)pThis; (void)pic; (void)wParam; (void)lParam;

    *pfEaten = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE KES_OnPreservedKey(
    ITfKeyEventSink *pThis, ITfContext *pic,
    REFGUID rguid, BOOL *pfEaten)
{
    TextService *ts = TS_FROM_KEY_EVENT_SINK(pThis);

    (void)pic;

    if (IsEqualGUID(rguid, &GUID_KolemakPreservedKey_Toggle)) {
        /* Flush any ongoing composition before toggling */
        if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
            HangulResult result = hangul_ic_flush(&ts->hangulCtx);
            EditSession *es = NULL;
            HRESULT hr;

            hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
            if (SUCCEEDED(hr)) {
                es->data.hangulResult = result;
                RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
                es->lpVtbl->Release((ITfEditSession *)es);
            }
        }

        ts->koreanMode = !ts->koreanMode;
        TextService_SetKeyboardOpen(ts, ts->koreanMode);
        if (ts->langBarButton)
            LangBarButton_UpdateState(ts->langBarButton);
        *pfEaten = TRUE;
        return S_OK;
    }

    if (IsEqualGUID(rguid, &GUID_KolemakPreservedKey_ColemakToggle)) {
        /* Flush any ongoing composition before toggling */
        if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
            HangulResult result = hangul_ic_flush(&ts->hangulCtx);
            EditSession *es = NULL;
            HRESULT hr;

            hr = EditSession_Create(ts, pic, ES_HANDLE_RESULT, &es);
            if (SUCCEEDED(hr)) {
                es->data.hangulResult = result;
                RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
                es->lpVtbl->Release((ITfEditSession *)es);
            }
        }

        ts->colemakMode = !ts->colemakMode;
        KolemakTooltip_Show(ts->colemakMode ? L"Colemak" : L"QWERTY");
        if (ts->langBarButton)
            LangBarButton_UpdateState(ts->langBarButton);
        *pfEaten = TRUE;
        return S_OK;
    }

    *pfEaten = FALSE;
    return S_OK;
}

/* ===== ITfKeyEventSink vtable (exported for text_service.c) ===== */

const ITfKeyEventSinkVtbl g_keyEventSinkVtbl = {
    KES_QueryInterface,
    KES_AddRef,
    KES_Release,
    KES_OnSetFocus,
    KES_OnTestKeyDown,
    KES_OnTestKeyUp,
    KES_OnKeyDown,
    KES_OnKeyUp,
    KES_OnPreservedKey,
};
