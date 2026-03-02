/*
 * settings.c - Registry-based settings for Kolemak IME
 *
 * Stores/loads user preferences in HKCU\Software\Kolemak.
 */

#include "settings.h"

static BOOL ReadRegDWORD(HKEY hKey, const WCHAR *name, DWORD *pValue)
{
    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    LONG ret = RegQueryValueExW(hKey, name, NULL, &type,
                                 (BYTE *)pValue, &size);
    return (ret == ERROR_SUCCESS && type == REG_DWORD);
}

static void WriteRegDWORD(HKEY hKey, const WCHAR *name, DWORD value)
{
    RegSetValueExW(hKey, name, 0, REG_DWORD,
                   (const BYTE *)&value, sizeof(DWORD));
}

BOOL Settings_Load(TextService *ts)
{
    HKEY hKey = NULL;
    LONG ret;
    DWORD val;

    ret = RegOpenKeyExW(HKEY_CURRENT_USER, KOLEMAK_REG_KEY,
                        0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS)
        return FALSE; /* No settings yet, use defaults */

    /* colemakMode는 저장/복원하지 않음: 항상 Colemak으로 시작 */

    if (ReadRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_BS, &val))
        ts->capsLockAsBackspace = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_SEMICOLON_SWAP, &val))
        ts->semicolonSwap = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_HOTKEY_VK, &val))
        ts->hotkeyVk = val;

    if (ReadRegDWORD(hKey, KOLEMAK_REG_HOTKEY_MOD, &val))
        ts->hotkeyModifiers = val;

    if (ReadRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_STATE, &val))
        ts->capsLockOn = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_WINKEY_REMAP, &val))
        ts->winKeyRemap = (val != 0);

    RegCloseKey(hKey);
    return TRUE;
}

void Settings_Save(TextService *ts)
{
    HKEY hKey = NULL;
    LONG ret;

    ret = RegCreateKeyExW(HKEY_CURRENT_USER, KOLEMAK_REG_KEY,
                          0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (ret != ERROR_SUCCESS)
        return;

    /* colemakMode는 저장하지 않음: 항상 Colemak으로 시작 */
    WriteRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_BS, ts->capsLockAsBackspace ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_SEMICOLON_SWAP, ts->semicolonSwap ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_HOTKEY_VK, ts->hotkeyVk);
    WriteRegDWORD(hKey, KOLEMAK_REG_HOTKEY_MOD, ts->hotkeyModifiers);
    WriteRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_STATE, ts->capsLockOn ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_WINKEY_REMAP, ts->winKeyRemap ? 1 : 0);

    RegCloseKey(hKey);
}

void Settings_ReloadPrefs(TextService *ts)
{
    HKEY hKey = NULL;
    LONG ret;
    DWORD val;
    UINT oldHotkeyVk, oldHotkeyMod;

    ret = RegOpenKeyExW(HKEY_CURRENT_USER, KOLEMAK_REG_KEY,
                        0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS)
        return;

    if (ReadRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_BS, &val))
        ts->capsLockAsBackspace = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_SEMICOLON_SWAP, &val))
        ts->semicolonSwap = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_STATE, &val))
        ts->capsLockOn = (val != 0);

    if (ReadRegDWORD(hKey, KOLEMAK_REG_WINKEY_REMAP, &val))
        ts->winKeyRemap = (val != 0);

    /* Sync colemakMode from registry (cross-process toggle sync) */
    if (ReadRegDWORD(hKey, KOLEMAK_REG_COLEMAK_MODE, &val)) {
        BOOL newMode = (val != 0);
        if (ts->colemakMode != newMode) {
            ts->colemakMode = newMode;
            if (ts->langBarButton)
                LangBarButton_UpdateState(ts->langBarButton);
        }
    }

    /* Re-register preserved key if hotkey changed (e.g. by another process) */
    oldHotkeyVk = ts->hotkeyVk;
    oldHotkeyMod = ts->hotkeyModifiers;

    if (ReadRegDWORD(hKey, KOLEMAK_REG_HOTKEY_VK, &val))
        ts->hotkeyVk = val;
    if (ReadRegDWORD(hKey, KOLEMAK_REG_HOTKEY_MOD, &val))
        ts->hotkeyModifiers = val;

    if (ts->hotkeyVk != oldHotkeyVk || ts->hotkeyModifiers != oldHotkeyMod) {
        ITfKeystrokeMgr *pKeyMgr = NULL;
        if (ts->threadMgr &&
            SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
                ts->threadMgr, &IID_ITfKeystrokeMgr, (void **)&pKeyMgr)))
        {
            TF_PRESERVEDKEY pkOld, pkNew;
            pkOld.uVKey = oldHotkeyVk;
            pkOld.uModifiers = oldHotkeyMod;
            pKeyMgr->lpVtbl->UnpreserveKey(
                pKeyMgr,
                &GUID_KolemakPreservedKey_ColemakToggle, &pkOld);

            pkNew.uVKey = (UINT)ts->hotkeyVk;
            pkNew.uModifiers = (UINT)ts->hotkeyModifiers;
            pKeyMgr->lpVtbl->PreserveKey(
                pKeyMgr, ts->clientId,
                &GUID_KolemakPreservedKey_ColemakToggle,
                &pkNew, KOLEMAK_DESC, KOLEMAK_DESC_LEN);

            pKeyMgr->lpVtbl->Release(pKeyMgr);
        }
    }

    RegCloseKey(hKey);
}
