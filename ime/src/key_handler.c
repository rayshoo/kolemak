/*
 * key_handler.c - ITfKeyEventSink implementation
 *
 * Handles keyboard input: applies Colemak mapping for English,
 * feeds Dubeolsik jamo to the Hangul engine for Korean.
 */

#include "kolemak.h"

/* Helper: request an edit session */
static HRESULT RequestEditSession(TextService *ts, ITfContext *ctx,
                                   EditSessionType type, EditSession *es)
{
    HRESULT hr;
    HRESULT hrSession;

    hr = ctx->lpVtbl->RequestEditSession(
        ctx, ts->clientId,
        (ITfEditSession *)es,
        TF_ES_ASYNCDONTCARE | TF_ES_READWRITE,
        &hrSession);

    return SUCCEEDED(hr) ? hrSession : hr;
}

/* Check if a key should be eaten */
static BOOL ShouldEatKey(TextService *ts, UINT vk, BOOL shift)
{
    /* Always eat letter keys */
    if (vk >= 'A' && vk <= 'Z')
        return TRUE;

    /* Eat semicolon in English mode (Colemak maps ; -> O) */
    if (!ts->koreanMode && vk == VK_OEM_1)
        return TRUE;

    /* Eat backspace during active composition */
    if (vk == VK_BACK && ts->hangulCtx.state != HANGUL_STATE_EMPTY)
        return TRUE;

    /* Eat Space/Enter/Escape to flush composition */
    if (ts->hangulCtx.state != HANGUL_STATE_EMPTY) {
        if (vk == VK_RETURN || vk == VK_ESCAPE)
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

    jamo = keymap_get_jamo(vk, shift);

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

/* Process a key in English mode */
static HRESULT HandleEnglishKey(TextService *ts, ITfContext *ctx,
                                 UINT vk, BOOL shift)
{
    WCHAR ch;
    EditSession *es = NULL;
    HRESULT hr;

    if (!keymap_get_colemak(vk, shift, &ch))
        return S_FALSE; /* Not a key we remap */

    hr = EditSession_Create(ts, ctx, ES_INSERT_CHAR, &es);
    if (FAILED(hr)) return hr;

    es->data.ch = ch;
    RequestEditSession(ts, ctx, ES_INSERT_CHAR, es);
    es->lpVtbl->Release((ITfEditSession *)es);

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
    (void)pThis; (void)fForeground;
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

        /* For Escape, eat the key. For Enter, let it pass through. */
        *pfEaten = (vk == VK_ESCAPE);
        return S_OK;
    }

    /* Normal key processing */
    if (ts->koreanMode) {
        hr = HandleKoreanKey(ts, pic, vk, shift);
    } else {
        hr = HandleEnglishKey(ts, pic, vk, shift);
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
