/*
 * dll_main.c - DLL entry point, COM class factory, registration
 */

#include "kolemak.h"
#include <shlwapi.h>

/* ===== Class Factory ===== */

typedef struct {
    const IClassFactoryVtbl *lpVtbl;
} ClassFactory;

static HRESULT STDMETHODCALLTYPE CF_QueryInterface(
    IClassFactory *pThis, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppvObj = pThis;
        pThis->lpVtbl->AddRef(pThis);
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE CF_AddRef(IClassFactory *pThis)
{
    (void)pThis;
    TextService_AddRefDll();
    return 2;
}

static ULONG STDMETHODCALLTYPE CF_Release(IClassFactory *pThis)
{
    (void)pThis;
    TextService_ReleaseDll();
    return 1;
}

static HRESULT STDMETHODCALLTYPE CF_CreateInstance(
    IClassFactory *pThis, IUnknown *pOuter, REFIID riid, void **ppvObj)
{
    return TextService_Create(pThis, pOuter, riid, ppvObj);
}

static HRESULT STDMETHODCALLTYPE CF_LockServer(
    IClassFactory *pThis, BOOL fLock)
{
    (void)pThis;
    if (fLock)
        TextService_AddRefDll();
    else
        TextService_ReleaseDll();
    return S_OK;
}

static const IClassFactoryVtbl g_cfVtbl = {
    CF_QueryInterface,
    CF_AddRef,
    CF_Release,
    CF_CreateInstance,
    CF_LockServer,
};

static ClassFactory g_classFactory = { &g_cfVtbl };

/* ===== DLL Entry Points ===== */

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID lpReserved)
{
    (void)lpReserved;

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstDll;
        DisableThreadLibraryCalls(hInstDll);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
    if (!IsEqualCLSID(rclsid, &CLSID_KolemakTextService)) {
        *ppvObj = NULL;
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    return CF_QueryInterface((IClassFactory *)&g_classFactory, riid, ppvObj);
}

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefDll <= 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    hr = KolemakIME_RegisterServer();
    if (SUCCEEDED(hr))
        hr = KolemakIME_RegisterProfiles();
    if (SUCCEEDED(hr))
        hr = KolemakIME_RegisterCategories();

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    KolemakIME_UnregisterCategories();
    KolemakIME_UnregisterProfiles();
    KolemakIME_UnregisterServer();
    return S_OK;
}

/* ===== COM Server Registration ===== */

static const WCHAR c_szInfoKeyPrefix[] = L"CLSID\\";
static const WCHAR c_szInProcSvr32[]   = L"InProcServer32";
static const WCHAR c_szModelName[]     = L"ThreadingModel";
static const WCHAR c_szModelVal[]      = L"Apartment";

static HRESULT CLSIDToString(REFCLSID rclsid, WCHAR *buf, int cchBuf)
{
    return StringFromGUID2(rclsid, buf, cchBuf) > 0 ? S_OK : E_FAIL;
}

HRESULT KolemakIME_RegisterServer(void)
{
    WCHAR szClsid[64];
    WCHAR szKey[256];
    WCHAR szModule[MAX_PATH];
    HKEY hKey = NULL, hSubKey = NULL;
    HRESULT hr;
    LONG lRes;

    hr = CLSIDToString(&CLSID_KolemakTextService, szClsid, 64);
    if (FAILED(hr)) return hr;

    /* Build key: CLSID\{...} */
    wsprintfW(szKey, L"CLSID\\%s", szClsid);

    /* Get DLL path */
    GetModuleFileNameW(g_hInst, szModule, MAX_PATH);

    /* Create CLSID key */
    lRes = RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS) return E_FAIL;

    RegSetValueExW(hKey, NULL, 0, REG_SZ,
                   (const BYTE *)KOLEMAK_DESC,
                   (KOLEMAK_DESC_LEN + 1) * sizeof(WCHAR));

    /* Create InProcServer32 subkey */
    lRes = RegCreateKeyExW(hKey, c_szInProcSvr32, 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, NULL);
    if (lRes != ERROR_SUCCESS) { RegCloseKey(hKey); return E_FAIL; }

    RegSetValueExW(hSubKey, NULL, 0, REG_SZ,
                   (const BYTE *)szModule,
                   (lstrlenW(szModule) + 1) * sizeof(WCHAR));

    RegSetValueExW(hSubKey, c_szModelName, 0, REG_SZ,
                   (const BYTE *)c_szModelVal,
                   sizeof(c_szModelVal));

    RegCloseKey(hSubKey);
    RegCloseKey(hKey);
    return S_OK;
}

HRESULT KolemakIME_UnregisterServer(void)
{
    WCHAR szClsid[64];
    WCHAR szKey[256];

    if (FAILED(CLSIDToString(&CLSID_KolemakTextService, szClsid, 64)))
        return E_FAIL;

    wsprintfW(szKey, L"CLSID\\%s\\InProcServer32", szClsid);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szKey);

    wsprintfW(szKey, L"CLSID\\%s", szClsid);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szKey);

    return S_OK;
}

HRESULT KolemakIME_RegisterProfiles(void)
{
    ITfInputProcessorProfiles *pProfiles = NULL;
    HRESULT hr;
    WCHAR szModule[MAX_PATH];

    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles, NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfiles,
                          (void **)&pProfiles);
    if (FAILED(hr)) return hr;

    GetModuleFileNameW(g_hInst, szModule, MAX_PATH);

    hr = pProfiles->lpVtbl->Register(pProfiles, &CLSID_KolemakTextService);
    if (SUCCEEDED(hr)) {
        hr = pProfiles->lpVtbl->AddLanguageProfile(
            pProfiles,
            &CLSID_KolemakTextService,
            KOLEMAK_LANGID,
            &GUID_KolemakProfile,
            KOLEMAK_DESC,
            KOLEMAK_DESC_LEN,
            szModule,
            lstrlenW(szModule),
            0   /* icon index */
        );
    }

    pProfiles->lpVtbl->Release(pProfiles);
    return hr;
}

HRESULT KolemakIME_UnregisterProfiles(void)
{
    ITfInputProcessorProfiles *pProfiles = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles, NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfiles,
                          (void **)&pProfiles);
    if (FAILED(hr)) return hr;

    pProfiles->lpVtbl->Unregister(pProfiles, &CLSID_KolemakTextService);
    pProfiles->lpVtbl->Release(pProfiles);
    return S_OK;
}

HRESULT KolemakIME_RegisterCategories(void)
{
    ITfCategoryMgr *pCatMgr = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_TF_CategoryMgr, NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfCategoryMgr,
                          (void **)&pCatMgr);
    if (FAILED(hr)) return hr;

    /* Register as a TIP (Text Input Processor) */
    hr = pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIP_KEYBOARD,
        &CLSID_KolemakTextService);

    pCatMgr->lpVtbl->Release(pCatMgr);
    return hr;
}

HRESULT KolemakIME_UnregisterCategories(void)
{
    ITfCategoryMgr *pCatMgr = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_TF_CategoryMgr, NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfCategoryMgr,
                          (void **)&pCatMgr);
    if (FAILED(hr)) return hr;

    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIP_KEYBOARD,
        &CLSID_KolemakTextService);

    pCatMgr->lpVtbl->Release(pCatMgr);
    return S_OK;
}
