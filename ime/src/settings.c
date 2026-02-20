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

    if (ReadRegDWORD(hKey, KOLEMAK_REG_COLEMAK_MODE, &val))
        ts->colemakMode = (val != 0);

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

    WriteRegDWORD(hKey, KOLEMAK_REG_COLEMAK_MODE, ts->colemakMode ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_BS, ts->capsLockAsBackspace ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_SEMICOLON_SWAP, ts->semicolonSwap ? 1 : 0);
    WriteRegDWORD(hKey, KOLEMAK_REG_HOTKEY_VK, ts->hotkeyVk);
    WriteRegDWORD(hKey, KOLEMAK_REG_HOTKEY_MOD, ts->hotkeyModifiers);
    WriteRegDWORD(hKey, KOLEMAK_REG_CAPSLOCK_STATE, ts->capsLockOn ? 1 : 0);

    RegCloseKey(hKey);
}

void Settings_ReloadPrefs(TextService *ts)
{
    HKEY hKey = NULL;
    LONG ret;
    DWORD val;

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

    RegCloseKey(hKey);
}
