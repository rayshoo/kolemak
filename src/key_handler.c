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

/* Check if a key should be eaten */
static BOOL ShouldEatKey(TextService *ts, UINT vk, BOOL shift)
{
    /* Remapped CapsLock (F13): always eat regardless of mode */
    if (vk == VK_F13)
        return TRUE;

    /* CapsLock → Backspace: eat CapsLock when enabled (pre-reboot compat) */
    if (ts->capsLockAsBackspace && vk == VK_CAPITAL)
        return TRUE;

    /* When a modifier (Ctrl/Alt/Win) is held, pass shortcuts through.
     * In Colemak mode, eat keys that need VK remapping so we can
     * remap them in OnKeyDown (e.g. physical K → Ctrl+E). */
    {
        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        BOOL alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
        BOOL win  = ((GetKeyState(VK_LWIN) & 0x8000) |
                     (GetKeyState(VK_RWIN) & 0x8000)) != 0;
        if (ctrl || alt || win) {
            if (ts->colemakMode) {
                if (ts->colemakRemapVk == vk)
                    return FALSE;  /* Our remapped key coming back */
                if (keymap_get_colemak_vk(vk) != vk)
                    return TRUE;   /* Needs VK remapping */
            }
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

    /* Handle Enter/Escape: flush composition */
    if ((vk == VK_RETURN || vk == VK_ESCAPE) &&
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

        *pfEaten = TRUE;  /* Enter, Escape 모두 eat */

        if (vk == VK_RETURN) {
            /* 조합 커밋 후 Enter 재주입 */
            INPUT inputs[2] = {0};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_RETURN;
            inputs[0].ki.wScan = (WORD)MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_RETURN;
            inputs[1].ki.wScan = (WORD)MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }
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
            RequestEditSession(ts, pic, ES_HANDLE_RESULT, es);
            es->lpVtbl->Release((ITfEditSession *)es);
        }

        /* 키 재주입 */
        {
            INPUT inputs[2] = {0};
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = (WORD)vk;
            inputs[0].ki.wScan = (WORD)MapVirtualKey(vk, MAPVK_VK_TO_VSC);
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = (WORD)vk;
            inputs[1].ki.wScan = (WORD)MapVirtualKey(vk, MAPVK_VK_TO_VSC);
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, inputs, sizeof(INPUT));
        }
        *pfEaten = TRUE;
        return S_OK;
    }

    /* Modifier shortcuts: remap VK codes in Colemak mode,
     * otherwise let shortcuts pass through unchanged. */
    {
        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        BOOL alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
        BOOL win  = ((GetKeyState(VK_LWIN) & 0x8000) |
                     (GetKeyState(VK_RWIN) & 0x8000)) != 0;
        if (ctrl || alt || win) {
            if (ts->colemakMode) {
                if (ts->colemakRemapVk == vk) {
                    /* Our remapped key coming back, pass through */
                    ts->colemakRemapVk = 0;
                    *pfEaten = FALSE;
                    return S_OK;
                }
                {
                    UINT remapped = keymap_get_colemak_vk(vk);
                    if (remapped != vk) {
                        INPUT inputs[2];
                        memset(inputs, 0, sizeof(inputs));
                        ts->colemakRemapVk = remapped;
                        inputs[0].type = INPUT_KEYBOARD;
                        inputs[0].ki.wVk = (WORD)remapped;
                        inputs[0].ki.wScan =
                            (WORD)MapVirtualKey(remapped, MAPVK_VK_TO_VSC);
                        inputs[1].type = INPUT_KEYBOARD;
                        inputs[1].ki.wVk = (WORD)remapped;
                        inputs[1].ki.wScan =
                            (WORD)MapVirtualKey(remapped, MAPVK_VK_TO_VSC);
                        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
                        SendInput(2, inputs, sizeof(INPUT));
                        *pfEaten = TRUE;
                        return S_OK;
                    }
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
