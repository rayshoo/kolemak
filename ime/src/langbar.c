/*
 * langbar.c - ITfLangBarItemButton implementation for Kolemak IME
 *
 * Displays current mode ("한" / "CM" / "QW") in the language bar area
 * and provides a menu for toggling settings.
 */

#include "langbar.h"
#include "settings.h"

/* ===== LangBarButton structure ===== */

struct LangBarButton {
    const ITfLangBarItemButtonVtbl *lpVtbl;
    const ITfSourceVtbl            *lpSourceVtbl;
    LONG                            refCount;
    TextService                    *ts;
    ITfLangBarItemSink             *sink;
    DWORD                           sinkCookie;
};

#define LBB_FROM_SOURCE(p) \
    ((LangBarButton *)((BYTE *)(p) - offsetof(LangBarButton, lpSourceVtbl)))

/* {CF8D0E61-92AE-BF1F-DA0B-5C6D7E8FA1B2} */
static const GUID GUID_LangBarButton_Kolemak = {
    0xcf8d0e61, 0x92ae, 0xbf1f,
    {0xda, 0x0b, 0x5c, 0x6d, 0x7e, 0x8f, 0xa1, 0xb2}
};

/* ===== ITfLangBarItemButton IUnknown ===== */

static HRESULT STDMETHODCALLTYPE LBB_QueryInterface(
    ITfLangBarItemButton *pThis, REFIID riid, void **ppvObj)
{
    LangBarButton *btn = (LangBarButton *)pThis;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ITfLangBarItem) ||
        IsEqualIID(riid, &IID_ITfLangBarItemButton))
    {
        *ppvObj = &btn->lpVtbl;
    }
    else if (IsEqualIID(riid, &IID_ITfSource))
    {
        *ppvObj = &btn->lpSourceVtbl;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->lpVtbl->AddRef((IUnknown *)*ppvObj);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE LBB_AddRef(ITfLangBarItemButton *pThis)
{
    LangBarButton *btn = (LangBarButton *)pThis;
    return InterlockedIncrement(&btn->refCount);
}

static ULONG STDMETHODCALLTYPE LBB_Release(ITfLangBarItemButton *pThis)
{
    LangBarButton *btn = (LangBarButton *)pThis;
    LONG c = InterlockedDecrement(&btn->refCount);
    if (c == 0) {
        if (btn->sink) {
            btn->sink->lpVtbl->Release(btn->sink);
            btn->sink = NULL;
        }
        HeapFree(GetProcessHeap(), 0, btn);
    }
    return c;
}

/* ===== ITfLangBarItem methods ===== */

static HRESULT STDMETHODCALLTYPE LBB_GetInfo(
    ITfLangBarItemButton *pThis, TF_LANGBARITEMINFO *pInfo)
{
    (void)pThis;

    pInfo->clsidService = CLSID_KolemakTextService;
    pInfo->guidItem = GUID_LangBarButton_Kolemak;
    pInfo->dwStyle = TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_SHOWNINTRAY;
    pInfo->ulSort = 0;
    StringCchCopyW(pInfo->szDescription,
                   ARRAYSIZE(pInfo->szDescription),
                   L"Kolemak IME");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_GetStatus(
    ITfLangBarItemButton *pThis, DWORD *pdwStatus)
{
    (void)pThis;
    *pdwStatus = 0; /* enabled */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_Show(
    ITfLangBarItemButton *pThis, BOOL fShow)
{
    (void)pThis; (void)fShow;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_GetTooltipString(
    ITfLangBarItemButton *pThis, BSTR *pbstrToolTip)
{
    LangBarButton *btn = (LangBarButton *)pThis;
    TextService *ts = btn->ts;

    if (ts->koreanMode)
        *pbstrToolTip = SysAllocString(L"Kolemak: \xD55C\xAE00");  /* 한글 */
    else
        *pbstrToolTip = SysAllocString(L"Kolemak: English");

    return (*pbstrToolTip) ? S_OK : E_OUTOFMEMORY;
}

/* ===== ITfLangBarItemButton methods ===== */

static HRESULT STDMETHODCALLTYPE LBB_OnClick(
    ITfLangBarItemButton *pThis, TfLBIClick click,
    POINT pt, const RECT *prcArea)
{
    (void)pThis; (void)click; (void)pt; (void)prcArea;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_InitMenu(
    ITfLangBarItemButton *pThis, ITfMenu *pMenu)
{
    LangBarButton *btn = (LangBarButton *)pThis;
    TextService *ts = btn->ts;
    DWORD flagColemak = ts->colemakMode ? TF_LBMENUF_CHECKED : 0;
    DWORD flagCapsBS  = ts->capsLockAsBackspace ? TF_LBMENUF_CHECKED : 0;

    pMenu->lpVtbl->AddMenuItem(pMenu,
        MENUITEM_COLEMAK_MODE,
        flagColemak,
        NULL, NULL, L"Colemak Mode", 12, NULL);

    pMenu->lpVtbl->AddMenuItem(pMenu,
        MENUITEM_CAPSLOCK_BS,
        flagCapsBS,
        NULL, NULL, L"CapsLock \x2192 Backspace", 20, NULL);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_OnMenuSelect(
    ITfLangBarItemButton *pThis, UINT wID)
{
    LangBarButton *btn = (LangBarButton *)pThis;
    TextService *ts = btn->ts;

    switch (wID) {
    case MENUITEM_COLEMAK_MODE:
        ts->colemakMode = !ts->colemakMode;
        break;
    case MENUITEM_CAPSLOCK_BS:
        ts->capsLockAsBackspace = !ts->capsLockAsBackspace;
        break;
    default:
        return S_OK;
    }

    Settings_Save(ts);
    LangBarButton_UpdateState(btn);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBB_GetIcon(
    ITfLangBarItemButton *pThis, HICON *phIcon)
{
    (void)pThis;
    *phIcon = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE LBB_GetText(
    ITfLangBarItemButton *pThis, BSTR *pbstrText)
{
    (void)pThis;
    *pbstrText = SysAllocString(L"\xD55C\xAE00"); /* 한글 */
    return (*pbstrText) ? S_OK : E_OUTOFMEMORY;
}

/* ===== ITfSource IUnknown ===== */

static HRESULT STDMETHODCALLTYPE LBS_QueryInterface(
    ITfSource *pThis, REFIID riid, void **ppvObj)
{
    LangBarButton *btn = LBB_FROM_SOURCE(pThis);
    return LBB_QueryInterface((ITfLangBarItemButton *)btn, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE LBS_AddRef(ITfSource *pThis)
{
    LangBarButton *btn = LBB_FROM_SOURCE(pThis);
    return LBB_AddRef((ITfLangBarItemButton *)btn);
}

static ULONG STDMETHODCALLTYPE LBS_Release(ITfSource *pThis)
{
    LangBarButton *btn = LBB_FROM_SOURCE(pThis);
    return LBB_Release((ITfLangBarItemButton *)btn);
}

/* ===== ITfSource methods ===== */

static HRESULT STDMETHODCALLTYPE LBS_AdviseSink(
    ITfSource *pThis, REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    LangBarButton *btn = LBB_FROM_SOURCE(pThis);

    if (!IsEqualIID(riid, &IID_ITfLangBarItemSink))
        return CONNECT_E_CANNOTCONNECT;

    if (btn->sink)
        return CONNECT_E_ADVISELIMIT;

    if (FAILED(punk->lpVtbl->QueryInterface(punk, &IID_ITfLangBarItemSink,
                                             (void **)&btn->sink)))
        return E_NOINTERFACE;

    *pdwCookie = 1;
    btn->sinkCookie = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE LBS_UnadviseSink(
    ITfSource *pThis, DWORD dwCookie)
{
    LangBarButton *btn = LBB_FROM_SOURCE(pThis);

    if (dwCookie != btn->sinkCookie || !btn->sink)
        return CONNECT_E_NOCONNECTION;

    btn->sink->lpVtbl->Release(btn->sink);
    btn->sink = NULL;
    btn->sinkCookie = 0;
    return S_OK;
}

/* ===== Vtables ===== */

static const ITfLangBarItemButtonVtbl g_langBarItemButtonVtbl = {
    LBB_QueryInterface,
    LBB_AddRef,
    LBB_Release,
    /* ITfLangBarItem */
    LBB_GetInfo,
    LBB_GetStatus,
    LBB_Show,
    LBB_GetTooltipString,
    /* ITfLangBarItemButton */
    LBB_OnClick,
    LBB_InitMenu,
    LBB_OnMenuSelect,
    LBB_GetIcon,
    LBB_GetText,
};

static const ITfSourceVtbl g_langBarSourceVtbl = {
    LBS_QueryInterface,
    LBS_AddRef,
    LBS_Release,
    LBS_AdviseSink,
    LBS_UnadviseSink,
};

/* ===== Public API ===== */

HRESULT LangBarButton_Create(TextService *ts, LangBarButton **ppButton)
{
    LangBarButton *btn;

    btn = (LangBarButton *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof(LangBarButton));
    if (!btn) return E_OUTOFMEMORY;

    btn->lpVtbl = &g_langBarItemButtonVtbl;
    btn->lpSourceVtbl = &g_langBarSourceVtbl;
    btn->refCount = 1;
    btn->ts = ts;

    *ppButton = btn;
    return S_OK;
}

void LangBarButton_Destroy(LangBarButton *button)
{
    if (button) {
        LBB_Release((ITfLangBarItemButton *)button);
    }
}

void LangBarButton_UpdateState(LangBarButton *button)
{
    if (button && button->sink) {
        button->sink->lpVtbl->OnUpdate(button->sink, TF_LBI_TEXT | TF_LBI_TOOLTIP);
    }
}

HRESULT LangBar_Register(TextService *ts)
{
    ITfLangBarItemMgr *pLangBarItemMgr = NULL;
    LangBarButton *btn = NULL;
    HRESULT hr;

    hr = ts->threadMgr->lpVtbl->QueryInterface(
        ts->threadMgr, &IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr);
    if (FAILED(hr)) return hr;

    hr = LangBarButton_Create(ts, &btn);
    if (FAILED(hr)) {
        pLangBarItemMgr->lpVtbl->Release(pLangBarItemMgr);
        return hr;
    }

    hr = pLangBarItemMgr->lpVtbl->AddItem(
        pLangBarItemMgr, (ITfLangBarItem *)btn);
    pLangBarItemMgr->lpVtbl->Release(pLangBarItemMgr);

    if (SUCCEEDED(hr)) {
        ts->langBarButton = btn;
    } else {
        LangBarButton_Destroy(btn);
    }

    return hr;
}

void LangBar_Unregister(TextService *ts)
{
    ITfLangBarItemMgr *pLangBarItemMgr = NULL;

    if (!ts->langBarButton)
        return;

    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
            ts->threadMgr, &IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr)))
    {
        pLangBarItemMgr->lpVtbl->RemoveItem(
            pLangBarItemMgr, (ITfLangBarItem *)ts->langBarButton);
        pLangBarItemMgr->lpVtbl->Release(pLangBarItemMgr);
    }

    LangBarButton_Destroy(ts->langBarButton);
    ts->langBarButton = NULL;
}
