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

/* ===== Scancode Map helpers ===== */

/*
 * Scancode Map binary format (in HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout):
 *   DWORD version;     // 0
 *   DWORD flags;       // 0
 *   DWORD count;       // number of mappings + 1 (null terminator)
 *   WORD  target, source;  // repeated 'count-1' times
 *   DWORD null_terminator; // 0
 *
 * We map CapsLock scancode 0x003A → F13 scancode 0x0064.
 */

#define SCANCODE_CAPSLOCK  0x003A
#define SCANCODE_F13       0x0064
#define SCMAP_HEADER_SIZE  (3 * sizeof(DWORD))  /* version + flags + count */
#define SCMAP_ENTRY_SIZE   sizeof(DWORD)        /* target(WORD) + source(WORD) */

static const WCHAR c_szKbdLayoutKey[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout";

static void ScancodeMap_Install(void)
{
    HKEY hKey = NULL;
    BYTE *existing = NULL;
    DWORD existSize = 0, type = 0;
    DWORD count, newCount, i;
    BYTE *newMap = NULL;
    DWORD newSize;
    BOOL found = FALSE;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_szKbdLayoutKey,
                      0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return; /* No admin rights — silently skip */

    /* Read existing Scancode Map if present */
    if (RegQueryValueExW(hKey, L"Scancode Map", NULL, &type,
                         NULL, &existSize) == ERROR_SUCCESS &&
        type == REG_BINARY && existSize >= SCMAP_HEADER_SIZE + SCMAP_ENTRY_SIZE)
    {
        existing = (BYTE *)HeapAlloc(GetProcessHeap(), 0, existSize);
        if (!existing) { RegCloseKey(hKey); return; }

        if (RegQueryValueExW(hKey, L"Scancode Map", NULL, NULL,
                             existing, &existSize) != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, existing);
            existing = NULL;
            existSize = 0;
        }
    }

    if (existing) {
        /* Parse existing map: check if CapsLock entry already exists */
        count = *(DWORD *)(existing + 8); /* entry count (includes null term) */

        for (i = 0; i < count - 1; i++) {
            WORD *entry = (WORD *)(existing + SCMAP_HEADER_SIZE +
                                   i * SCMAP_ENTRY_SIZE);
            WORD source = entry[1];
            if (source == SCANCODE_CAPSLOCK) {
                /* Already has CapsLock mapping — update target to F13 */
                entry[0] = SCANCODE_F13;
                RegSetValueExW(hKey, L"Scancode Map", 0, REG_BINARY,
                               existing, existSize);
                found = TRUE;
                break;
            }
        }

        if (!found) {
            /* Add CapsLock → F13 entry: rebuild with one more entry */
            newCount = count + 1;
            newSize = SCMAP_HEADER_SIZE + newCount * SCMAP_ENTRY_SIZE;
            newMap = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       newSize);
            if (newMap) {
                /* Copy header */
                memcpy(newMap, existing, 8);
                *(DWORD *)(newMap + 8) = newCount;
                /* Copy existing entries (excluding null terminator) */
                memcpy(newMap + SCMAP_HEADER_SIZE,
                       existing + SCMAP_HEADER_SIZE,
                       (count - 1) * SCMAP_ENTRY_SIZE);
                /* Append our entry */
                {
                    WORD *entry = (WORD *)(newMap + SCMAP_HEADER_SIZE +
                                           (count - 1) * SCMAP_ENTRY_SIZE);
                    entry[0] = SCANCODE_F13;
                    entry[1] = SCANCODE_CAPSLOCK;
                }
                /* Null terminator is already zero from HEAP_ZERO_MEMORY */
                RegSetValueExW(hKey, L"Scancode Map", 0, REG_BINARY,
                               newMap, newSize);
                HeapFree(GetProcessHeap(), 0, newMap);
            }
        }
        HeapFree(GetProcessHeap(), 0, existing);
    } else {
        /* No existing Scancode Map: create fresh */
        BYTE map[SCMAP_HEADER_SIZE + 2 * SCMAP_ENTRY_SIZE];
        memset(map, 0, sizeof(map));
        *(DWORD *)(map + 8) = 2; /* 1 mapping + null terminator */
        {
            WORD *entry = (WORD *)(map + SCMAP_HEADER_SIZE);
            entry[0] = SCANCODE_F13;
            entry[1] = SCANCODE_CAPSLOCK;
        }
        /* Last DWORD is null terminator (already zero) */
        RegSetValueExW(hKey, L"Scancode Map", 0, REG_BINARY,
                       map, sizeof(map));
    }

    RegCloseKey(hKey);
}

static void ScancodeMap_Uninstall(void)
{
    HKEY hKey = NULL;
    BYTE *existing = NULL;
    DWORD existSize = 0, type = 0;
    DWORD count, newCount, i, j;
    BYTE *newMap = NULL;
    DWORD newSize;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_szKbdLayoutKey,
                      0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return;

    if (RegQueryValueExW(hKey, L"Scancode Map", NULL, &type,
                         NULL, &existSize) != ERROR_SUCCESS ||
        type != REG_BINARY ||
        existSize < SCMAP_HEADER_SIZE + SCMAP_ENTRY_SIZE)
    {
        RegCloseKey(hKey);
        return;
    }

    existing = (BYTE *)HeapAlloc(GetProcessHeap(), 0, existSize);
    if (!existing) { RegCloseKey(hKey); return; }

    if (RegQueryValueExW(hKey, L"Scancode Map", NULL, NULL,
                         existing, &existSize) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, existing);
        RegCloseKey(hKey);
        return;
    }

    count = *(DWORD *)(existing + 8);

    /* Count remaining entries after removing CapsLock source */
    newCount = 1; /* null terminator */
    for (i = 0; i < count - 1; i++) {
        WORD *entry = (WORD *)(existing + SCMAP_HEADER_SIZE +
                               i * SCMAP_ENTRY_SIZE);
        if (entry[1] != SCANCODE_CAPSLOCK)
            newCount++;
    }

    if (newCount == 1) {
        /* No entries left: delete the Scancode Map value entirely */
        RegDeleteValueW(hKey, L"Scancode Map");
    } else {
        /* Rebuild without CapsLock entry */
        newSize = SCMAP_HEADER_SIZE + newCount * SCMAP_ENTRY_SIZE;
        newMap = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize);
        if (newMap) {
            memcpy(newMap, existing, 8);
            *(DWORD *)(newMap + 8) = newCount;

            j = 0;
            for (i = 0; i < count - 1; i++) {
                WORD *src = (WORD *)(existing + SCMAP_HEADER_SIZE +
                                     i * SCMAP_ENTRY_SIZE);
                if (src[1] != SCANCODE_CAPSLOCK) {
                    WORD *dst = (WORD *)(newMap + SCMAP_HEADER_SIZE +
                                         j * SCMAP_ENTRY_SIZE);
                    dst[0] = src[0];
                    dst[1] = src[1];
                    j++;
                }
            }
            RegSetValueExW(hKey, L"Scancode Map", 0, REG_BINARY,
                           newMap, newSize);
            HeapFree(GetProcessHeap(), 0, newMap);
        }
    }

    HeapFree(GetProcessHeap(), 0, existing);
    RegCloseKey(hKey);
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    hr = KolemakIME_RegisterServer();
    if (SUCCEEDED(hr))
        hr = KolemakIME_RegisterProfiles();
    if (SUCCEEDED(hr))
        hr = KolemakIME_RegisterCategories();

    if (SUCCEEDED(hr))
        ScancodeMap_Install();

    return hr;
}

STDAPI DllUnregisterServer(void)
{
    ScancodeMap_Uninstall();
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
    ITfInputProcessorProfileMgr *pProfileMgr = NULL;
    HRESULT hr;
    WCHAR szModule[MAX_PATH];

    GetModuleFileNameW(g_hInst, szModule, MAX_PATH);

    /* Use the newer ITfInputProcessorProfileMgr API (required for Win10/11) */
    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles, NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfileMgr,
                          (void **)&pProfileMgr);
    if (SUCCEEDED(hr)) {
        hr = pProfileMgr->lpVtbl->RegisterProfile(
            pProfileMgr,
            &CLSID_KolemakTextService,
            KOLEMAK_LANGID,
            &GUID_KolemakProfile,
            KOLEMAK_DESC,
            KOLEMAK_DESC_LEN,
            szModule,
            lstrlenW(szModule),
            0,      /* icon index */
            NULL,   /* hklSubstitute */
            0,      /* dwPreferredLayout */
            TRUE,   /* bEnabledByDefault */
            0       /* dwFlags */
        );
        pProfileMgr->lpVtbl->Release(pProfileMgr);
    } else {
        /* Fallback to older API */
        ITfInputProcessorProfiles *pProfiles = NULL;
        hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles, NULL,
                              CLSCTX_INPROC_SERVER,
                              &IID_ITfInputProcessorProfiles,
                              (void **)&pProfiles);
        if (FAILED(hr)) return hr;

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
                0);
        }
        pProfiles->lpVtbl->Release(pProfiles);
    }

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
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIP_KEYBOARD,
        &CLSID_KolemakTextService);

    /* Required for Windows 8+ Store/UWP apps */
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
        &CLSID_KolemakTextService);

    /* Required for Windows 8+ system tray support */
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
        &CLSID_KolemakTextService);

    /* Required for secure desktop (UAC dialogs, lock screen) */
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_SECUREMODE,
        &CLSID_KolemakTextService);

    /* UI element support */
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
        &CLSID_KolemakTextService);

    /* Input mode compartment support (enables 가/A indicator in taskbar) */
    pCatMgr->lpVtbl->RegisterCategory(
        pCatMgr,
        &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT,
        &CLSID_KolemakTextService);

    pCatMgr->lpVtbl->Release(pCatMgr);
    return S_OK;
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
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIP_KEYBOARD, &CLSID_KolemakTextService);
    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, &CLSID_KolemakTextService);
    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, &CLSID_KolemakTextService);
    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_SECUREMODE, &CLSID_KolemakTextService);
    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_UIELEMENTENABLED, &CLSID_KolemakTextService);
    pCatMgr->lpVtbl->UnregisterCategory(
        pCatMgr, &CLSID_KolemakTextService,
        &GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, &CLSID_KolemakTextService);

    pCatMgr->lpVtbl->Release(pCatMgr);
    return S_OK;
}
