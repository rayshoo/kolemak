/*
 * settings.h - Registry-based settings for Kolemak IME
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "kolemak.h"

#define KOLEMAK_REG_KEY L"Software\\Kolemak"

#define KOLEMAK_REG_COLEMAK_MODE     L"ColemakMode"
#define KOLEMAK_REG_CAPSLOCK_BS      L"CapsLockAsBackspace"
#define KOLEMAK_REG_SEMICOLON_SWAP   L"SemicolonSwap"
#define KOLEMAK_REG_HOTKEY_VK        L"HotkeyVk"
#define KOLEMAK_REG_HOTKEY_MOD       L"HotkeyModifiers"

#endif /* SETTINGS_H */
