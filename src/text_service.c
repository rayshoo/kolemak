/*
 * text_service.c - TextService COM object implementation
 *
 * Implements: ITfTextInputProcessorEx, ITfThreadMgrEventSink,
 *             ITfCompositionSink
 */

#include "kolemak.h"
#include "settings.h"

/* Forward declarations for vtables defined in key_handler.c */
extern const ITfKeyEventSinkVtbl g_keyEventSinkVtbl;

/* ===== IUnknown (primary) ===== */

static HRESULT STDMETHODCALLTYPE TS_QueryInterface(
    ITfTextInputProcessorEx *pThis, REFIID riid, void **ppvObj)
{
    TextService *ts = TS_FROM_TIP(pThis);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, &IID_ITfTextInputProcessorEx))
    {
        *ppvObj = &ts->lpVtbl;
    }
    else if (IsEqualIID(riid, &IID_ITfKeyEventSink))
    {
        *ppvObj = &ts->keyEventSink;
    }
    else if (IsEqualIID(riid, &IID_ITfThreadMgrEventSink))
    {
        *ppvObj = &ts->threadMgrEventSink;
    }
    else if (IsEqualIID(riid, &IID_ITfCompositionSink))
    {
        *ppvObj = &ts->compositionSink;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->lpVtbl->AddRef((IUnknown *)*ppvObj);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE TS_AddRef(ITfTextInputProcessorEx *pThis)
{
    TextService *ts = TS_FROM_TIP(pThis);
    return InterlockedIncrement(&ts->refCount);
}

static ULONG STDMETHODCALLTYPE TS_Release(ITfTextInputProcessorEx *pThis)
{
    TextService *ts = TS_FROM_TIP(pThis);
    LONG c = InterlockedDecrement(&ts->refCount);
    if (c == 0) {
        if (ts->composition) {
            ts->composition->lpVtbl->Release(ts->composition);
            ts->composition = NULL;
        }
        if (ts->threadMgr) {
            ts->threadMgr->lpVtbl->Release(ts->threadMgr);
            ts->threadMgr = NULL;
        }
        HeapFree(GetProcessHeap(), 0, ts);
        TextService_ReleaseDll();
    }
    return c;
}

/* ===== ITfTextInputProcessorEx ===== */

static HRESULT TS_AdviseThreadMgrSink(TextService *ts)
{
    ITfSource *pSource = NULL;
    HRESULT hr;

    hr = ts->threadMgr->lpVtbl->QueryInterface(
        ts->threadMgr, &IID_ITfSource, (void **)&pSource);
    if (FAILED(hr)) return hr;

    hr = pSource->lpVtbl->AdviseSink(
        pSource,
        &IID_ITfThreadMgrEventSink,
        (IUnknown *)&ts->threadMgrEventSink,
        &ts->threadMgrSinkCookie);

    pSource->lpVtbl->Release(pSource);
    return hr;
}

static void TS_UnadviseThreadMgrSink(TextService *ts)
{
    ITfSource *pSource = NULL;

    if (ts->threadMgrSinkCookie == TF_INVALID_COOKIE)
        return;

    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
            ts->threadMgr, &IID_ITfSource, (void **)&pSource)))
    {
        pSource->lpVtbl->UnadviseSink(pSource, ts->threadMgrSinkCookie);
        pSource->lpVtbl->Release(pSource);
    }
    ts->threadMgrSinkCookie = TF_INVALID_COOKIE;
}

static HRESULT TS_AdviseKeyEventSink(TextService *ts)
{
    ITfKeystrokeMgr *pKeyMgr = NULL;
    HRESULT hr;

    hr = ts->threadMgr->lpVtbl->QueryInterface(
        ts->threadMgr, &IID_ITfKeystrokeMgr, (void **)&pKeyMgr);
    if (FAILED(hr)) return hr;

    hr = pKeyMgr->lpVtbl->AdviseKeyEventSink(
        pKeyMgr,
        ts->clientId,
        (ITfKeyEventSink *)&ts->keyEventSink,
        TRUE /* foreground */);

    pKeyMgr->lpVtbl->Release(pKeyMgr);
    return hr;
}

static void TS_UnadviseKeyEventSink(TextService *ts)
{
    ITfKeystrokeMgr *pKeyMgr = NULL;

    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
            ts->threadMgr, &IID_ITfKeystrokeMgr, (void **)&pKeyMgr)))
    {
        pKeyMgr->lpVtbl->UnadviseKeyEventSink(pKeyMgr, ts->clientId);
        pKeyMgr->lpVtbl->Release(pKeyMgr);
    }
}

static HRESULT TS_RegisterPreservedKey(TextService *ts)
{
    ITfKeystrokeMgr *pKeyMgr = NULL;
    HRESULT hr;
    TF_PRESERVEDKEY pkHangul;
    TF_PRESERVEDKEY pkRightAlt;
    TF_PRESERVEDKEY pkWinSpace;

    hr = ts->threadMgr->lpVtbl->QueryInterface(
        ts->threadMgr, &IID_ITfKeystrokeMgr, (void **)&pKeyMgr);
    if (FAILED(hr)) return hr;

    /* Hangul key (VK_HANGUL = 0x15) */
    pkHangul.uVKey = VK_HANGUL;
    pkHangul.uModifiers = 0;
    pKeyMgr->lpVtbl->PreserveKey(
        pKeyMgr, ts->clientId,
        &GUID_KolemakPreservedKey_Toggle,
        &pkHangul,
        KOLEMAK_DESC, KOLEMAK_DESC_LEN);

    /* Right Alt as alternative toggle */
    pkRightAlt.uVKey = VK_MENU;
    pkRightAlt.uModifiers = TF_MOD_ON_KEYUP | TF_MOD_RALT;
    pKeyMgr->lpVtbl->PreserveKey(
        pKeyMgr, ts->clientId,
        &GUID_KolemakPreservedKey_Toggle,
        &pkRightAlt,
        KOLEMAK_DESC, KOLEMAK_DESC_LEN);

    /* Custom hotkey for Colemak/QWERTY toggle (default: Ctrl+Space) */
    pkWinSpace.uVKey = (UINT)ts->hotkeyVk;
    pkWinSpace.uModifiers = (UINT)ts->hotkeyModifiers;
    pKeyMgr->lpVtbl->PreserveKey(
        pKeyMgr, ts->clientId,
        &GUID_KolemakPreservedKey_ColemakToggle,
        &pkWinSpace,
        KOLEMAK_DESC, KOLEMAK_DESC_LEN);

    pKeyMgr->lpVtbl->Release(pKeyMgr);
    return S_OK;
}

static void TS_UnregisterPreservedKey(TextService *ts)
{
    ITfKeystrokeMgr *pKeyMgr = NULL;
    TF_PRESERVEDKEY pkHangul;
    TF_PRESERVEDKEY pkRightAlt;
    TF_PRESERVEDKEY pkWinSpace;

    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
            ts->threadMgr, &IID_ITfKeystrokeMgr, (void **)&pKeyMgr)))
    {
        pkHangul.uVKey = VK_HANGUL;
        pkHangul.uModifiers = 0;
        pKeyMgr->lpVtbl->UnpreserveKey(
            pKeyMgr, &GUID_KolemakPreservedKey_Toggle, &pkHangul);

        pkRightAlt.uVKey = VK_MENU;
        pkRightAlt.uModifiers = TF_MOD_ON_KEYUP | TF_MOD_RALT;
        pKeyMgr->lpVtbl->UnpreserveKey(
            pKeyMgr, &GUID_KolemakPreservedKey_Toggle, &pkRightAlt);

        pkWinSpace.uVKey = (UINT)ts->hotkeyVk;
        pkWinSpace.uModifiers = (UINT)ts->hotkeyModifiers;
        pKeyMgr->lpVtbl->UnpreserveKey(
            pKeyMgr, &GUID_KolemakPreservedKey_ColemakToggle, &pkWinSpace);

        pKeyMgr->lpVtbl->Release(pKeyMgr);
    }
}

static HRESULT STDMETHODCALLTYPE TS_Activate(
    ITfTextInputProcessorEx *pThis, ITfThreadMgr *ptim, TfClientId tid)
{
    (void)pThis; (void)ptim; (void)tid;
    return E_NOTIMPL; /* Use ActivateEx instead */
}

static HRESULT STDMETHODCALLTYPE TS_Deactivate(
    ITfTextInputProcessorEx *pThis)
{
    TextService *ts = TS_FROM_TIP(pThis);

    LangBar_Unregister(ts);
    KolemakTray_Unregister(ts);
    TS_UnregisterPreservedKey(ts);
    TS_UnadviseKeyEventSink(ts);
    TS_UnadviseThreadMgrSink(ts);

    if (ts->threadMgr) {
        ts->threadMgr->lpVtbl->Release(ts->threadMgr);
        ts->threadMgr = NULL;
    }

    hangul_ic_reset(&ts->hangulCtx);
    ts->koreanMode = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TS_ActivateEx(
    ITfTextInputProcessorEx *pThis, ITfThreadMgr *ptim,
    TfClientId tid, DWORD dwFlags)
{
    TextService *ts = TS_FROM_TIP(pThis);
    HRESULT hr;

    (void)dwFlags;

    ts->threadMgr = ptim;
    ptim->lpVtbl->AddRef(ptim);
    ts->clientId = tid;

    hangul_ic_init(&ts->hangulCtx);
    ts->koreanMode = FALSE;
    ts->colemakMode = TRUE;
    ts->capsLockAsBackspace = TRUE;
    ts->capsLockOn = FALSE;
    ts->semicolonSwap = TRUE;
    ts->hotkeyVk = VK_SPACE;
    ts->hotkeyModifiers = TF_MOD_CONTROL;
    ts->composition = NULL;
    ts->langBarButton = NULL;
    ts->threadMgrSinkCookie = TF_INVALID_COOKIE;

    /* Load saved settings from registry; create defaults if key doesn't exist */
    if (!Settings_Load(ts))
        Settings_Save(ts);

    hr = TS_AdviseThreadMgrSink(ts);
    if (FAILED(hr)) goto fail;

    hr = TS_AdviseKeyEventSink(ts);
    if (FAILED(hr)) goto fail;

    hr = TS_RegisterPreservedKey(ts);
    if (FAILED(hr)) goto fail;

    /* Initialize tooltip, tray icon, and language bar button (non-fatal) */
    KolemakTooltip_Init(g_hInst);
    KolemakTray_Register(ts);
    LangBar_Register(ts);

    /* Set initial keyboard open/close state for system tray indicator */
    TextService_SetKeyboardOpen(ts, ts->koreanMode);

    return S_OK;

fail:
    TS_Deactivate(pThis);
    return hr;
}

/* ===== ITfTextInputProcessorEx vtable ===== */

static const ITfTextInputProcessorExVtbl g_tipVtbl = {
    TS_QueryInterface,
    TS_AddRef,
    TS_Release,
    TS_Activate,
    TS_Deactivate,
    TS_ActivateEx,
};

/* ===== ITfThreadMgrEventSink ===== */

static HRESULT STDMETHODCALLTYPE TMES_QueryInterface(
    ITfThreadMgrEventSink *pThis, REFIID riid, void **ppvObj)
{
    return TS_QueryInterface(
        (ITfTextInputProcessorEx *)TS_FROM_THREAD_MGR_SINK(pThis),
        riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE TMES_AddRef(ITfThreadMgrEventSink *pThis)
{
    return TS_AddRef(
        (ITfTextInputProcessorEx *)TS_FROM_THREAD_MGR_SINK(pThis));
}

static ULONG STDMETHODCALLTYPE TMES_Release(ITfThreadMgrEventSink *pThis)
{
    return TS_Release(
        (ITfTextInputProcessorEx *)TS_FROM_THREAD_MGR_SINK(pThis));
}

static HRESULT STDMETHODCALLTYPE TMES_OnInitDocumentMgr(
    ITfThreadMgrEventSink *pThis, ITfDocumentMgr *pdim)
{
    (void)pThis; (void)pdim;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TMES_OnUninitDocumentMgr(
    ITfThreadMgrEventSink *pThis, ITfDocumentMgr *pdim)
{
    (void)pThis; (void)pdim;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TMES_OnSetFocus(
    ITfThreadMgrEventSink *pThis,
    ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus)
{
    TextService *ts = TS_FROM_THREAD_MGR_SINK(pThis);
    (void)pdimFocus; (void)pdimPrevFocus;
    Settings_ReloadPrefs(ts);
    KolemakTray_EnsureIcon(ts);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TMES_OnPushContext(
    ITfThreadMgrEventSink *pThis, ITfContext *pic)
{
    (void)pThis; (void)pic;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE TMES_OnPopContext(
    ITfThreadMgrEventSink *pThis, ITfContext *pic)
{
    (void)pThis; (void)pic;
    return S_OK;
}

static const ITfThreadMgrEventSinkVtbl g_threadMgrEventSinkVtbl = {
    TMES_QueryInterface,
    TMES_AddRef,
    TMES_Release,
    TMES_OnInitDocumentMgr,
    TMES_OnUninitDocumentMgr,
    TMES_OnSetFocus,
    TMES_OnPushContext,
    TMES_OnPopContext,
};

/* ===== ITfCompositionSink ===== */

static HRESULT STDMETHODCALLTYPE CS_QueryInterface(
    ITfCompositionSink *pThis, REFIID riid, void **ppvObj)
{
    return TS_QueryInterface(
        (ITfTextInputProcessorEx *)TS_FROM_COMPOSITION_SINK(pThis),
        riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE CS_AddRef(ITfCompositionSink *pThis)
{
    return TS_AddRef(
        (ITfTextInputProcessorEx *)TS_FROM_COMPOSITION_SINK(pThis));
}

static ULONG STDMETHODCALLTYPE CS_Release(ITfCompositionSink *pThis)
{
    return TS_Release(
        (ITfTextInputProcessorEx *)TS_FROM_COMPOSITION_SINK(pThis));
}

static HRESULT STDMETHODCALLTYPE CS_OnCompositionTerminated(
    ITfCompositionSink *pThis, TfEditCookie ecWrite, ITfComposition *pComp)
{
    TextService *ts = TS_FROM_COMPOSITION_SINK(pThis);

    /* Position cursor at end of composition text before releasing.
     * This prevents the cursor from jumping to before the text
     * when returning from Alt+Tab. */
    if (pComp) {
        ITfRange *pRange = NULL;
        if (SUCCEEDED(pComp->lpVtbl->GetRange(pComp, &pRange))) {
            ITfContext *ctx = NULL;
            if (SUCCEEDED(pRange->lpVtbl->GetContext(pRange, &ctx))) {
                TF_SELECTION sel;
                pRange->lpVtbl->Collapse(pRange, ecWrite, TF_ANCHOR_END);
                sel.range = pRange;
                sel.style.ase = TF_AE_NONE;
                sel.style.fInterimChar = FALSE;
                ctx->lpVtbl->SetSelection(ctx, ecWrite, 1, &sel);
                ctx->lpVtbl->Release(ctx);
            }
            pRange->lpVtbl->Release(pRange);
        }
    }

    if (ts->composition) {
        ts->composition->lpVtbl->Release(ts->composition);
        ts->composition = NULL;
    }
    hangul_ic_reset(&ts->hangulCtx);

    return S_OK;
}

static const ITfCompositionSinkVtbl g_compositionSinkVtbl = {
    CS_QueryInterface,
    CS_AddRef,
    CS_Release,
    CS_OnCompositionTerminated,
};

/* ===== Compartment helper ===== */

void TextService_SetKeyboardOpen(TextService *ts, BOOL open)
{
    ITfCompartmentMgr *pCompMgr = NULL;
    ITfCompartment *pComp = NULL;

    if (!ts->threadMgr) return;

    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
            ts->threadMgr, &IID_ITfCompartmentMgr, (void **)&pCompMgr)))
    {
        if (SUCCEEDED(pCompMgr->lpVtbl->GetCompartment(
                pCompMgr, &GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp)))
        {
            VARIANT var;
            var.vt = VT_I4;
            var.lVal = open ? 1 : 0;
            pComp->lpVtbl->SetValue(pComp, ts->clientId, &var);
            pComp->lpVtbl->Release(pComp);
        }
        pCompMgr->lpVtbl->Release(pCompMgr);
    }
}

/* ===== TextService creation ===== */

HRESULT TextService_Create(IClassFactory *pFactory, IUnknown *pOuter,
                           REFIID riid, void **ppvObj)
{
    TextService *ts;
    HRESULT hr;

    (void)pFactory;

    *ppvObj = NULL;

    if (pOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    ts = (TextService *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   sizeof(TextService));
    if (!ts) return E_OUTOFMEMORY;

    ts->lpVtbl = &g_tipVtbl;
    ts->keyEventSink.lpVtbl = &g_keyEventSinkVtbl;
    ts->threadMgrEventSink.lpVtbl = &g_threadMgrEventSinkVtbl;
    ts->compositionSink.lpVtbl = &g_compositionSinkVtbl;
    ts->refCount = 1;
    ts->threadMgrSinkCookie = TF_INVALID_COOKIE;

    hangul_ic_init(&ts->hangulCtx);

    TextService_AddRefDll();

    hr = TS_QueryInterface((ITfTextInputProcessorEx *)ts, riid, ppvObj);
    TS_Release((ITfTextInputProcessorEx *)ts); /* balance initial refCount=1 */

    return hr;
}
