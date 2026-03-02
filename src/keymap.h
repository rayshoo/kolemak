/*
 * keymap.h - Colemak and Dubeolsik key mapping tables
 */

#ifndef KEYMAP_H
#define KEYMAP_H

#include <windows.h>

/* Jamo mapping entry: virtual key -> choseong/jungseong index */
typedef struct {
    int cho;    /* Choseong index (0-18), -1 if vowel */
    int jung;   /* Jungseong index (0-20), -1 if consonant */
} JamoMapping;

/* Get Dubeolsik jamo for a virtual key code.
 * vk: virtual key code (e.g. 'A'-'Z')
 * shift: TRUE if Shift is held
 * semicolonSwap: TRUE = ㅔ on ; key instead of P key
 * Returns mapping with cho=-1 and jung=-1 if not a jamo key. */
JamoMapping keymap_get_jamo(UINT vk, BOOL shift, BOOL semicolonSwap);

/* Get Colemak-mapped character for a virtual key code.
 * vk: virtual key code
 * shift: TRUE if Shift is held
 * ch: output character
 * Returns TRUE if the key was remapped, FALSE if passthrough. */
BOOL keymap_get_colemak(UINT vk, BOOL shift, WCHAR *ch);

/* Get Colemak-remapped virtual key code for modifier shortcuts (Ctrl/Alt+key).
 * Maps QWERTY VK to Colemak VK. Returns the same vk if no change needed. */
UINT keymap_get_colemak_vk(UINT vk);

#endif /* KEYMAP_H */
