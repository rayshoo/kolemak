/*
 * globals.c - GUID definitions and global variables
 */

#include <initguid.h>
#include "kolemak.h"

/* {7A3E8B1C-4D5F-6E7A-8B9C-0D1E2F3A4B5C} */
DEFINE_GUID(CLSID_KolemakTextService,
    0x7a3e8b1c, 0x4d5f, 0x6e7a,
    0x8b, 0x9c, 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c);

/* {8B4F9C2D-5E6A-7F8B-9CAD-1E2F3A4B5C6D} */
DEFINE_GUID(GUID_KolemakProfile,
    0x8b4f9c2d, 0x5e6a, 0x7f8b,
    0x9c, 0xad, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d);

/* {9C5A0D3E-6F7B-8A9C-ADBE-2F3A4B5C6D7E} */
DEFINE_GUID(GUID_KolemakPreservedKey_Toggle,
    0x9c5a0d3e, 0x6f7b, 0x8a9c,
    0xad, 0xbe, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e);

/* {BE7C2F50-819D-AC0E-CFDA-4B5C6D7E8FA0} */
DEFINE_GUID(GUID_KolemakPreservedKey_ColemakToggle,
    0xbe7c2f50, 0x819d, 0xac0e,
    0xcf, 0xda, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0xa0);

/* {AD6B1E4F-708C-9BAD-BECF-3A4B5C6D7E8F} */
DEFINE_GUID(GUID_KolemakDisplayAttribute,
    0xad6b1e4f, 0x708c, 0x9bad,
    0xbe, 0xcf, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f);

/* Global DLL instance */
HINSTANCE g_hInst = NULL;
LONG g_cRefDll = 0;

void TextService_AddRefDll(void)
{
    InterlockedIncrement(&g_cRefDll);
}

void TextService_ReleaseDll(void)
{
    InterlockedDecrement(&g_cRefDll);
}
