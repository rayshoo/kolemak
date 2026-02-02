/*
 * keymap.c - Colemak and Dubeolsik key mapping tables
 */

#include "keymap.h"

/* ===== Dubeolsik standard Korean keyboard layout ===== */
/* Indexed by (VK - 'A'), i.e. 0=A, 1=B, ... 25=Z */

static const JamoMapping g_dubeolsik[26] = {
    {  6, -1 },  /* A -> ㅁ */
    { -1, 17 },  /* B -> ㅠ */
    { 14, -1 },  /* C -> ㅊ */
    { 11, -1 },  /* D -> ㅇ */
    {  3, -1 },  /* E -> ㄷ */
    {  5, -1 },  /* F -> ㄹ */
    { 18, -1 },  /* G -> ㅎ */
    { -1,  8 },  /* H -> ㅗ */
    { -1,  2 },  /* I -> ㅑ */
    { -1,  4 },  /* J -> ㅓ */
    { -1,  0 },  /* K -> ㅏ */
    { -1, 20 },  /* L -> ㅣ */
    { -1, 18 },  /* M -> ㅡ */
    { -1, 13 },  /* N -> ㅜ */
    { -1,  1 },  /* O -> ㅐ */
    { -1,  5 },  /* P -> ㅔ */
    {  7, -1 },  /* Q -> ㅂ */
    {  0, -1 },  /* R -> ㄱ */
    {  2, -1 },  /* S -> ㄴ */
    {  9, -1 },  /* T -> ㅅ */
    { -1,  6 },  /* U -> ㅕ */
    { 17, -1 },  /* V -> ㅍ */
    { 12, -1 },  /* W -> ㅈ */
    { 16, -1 },  /* X -> ㅌ */
    { -1, 12 },  /* Y -> ㅛ */
    { 15, -1 },  /* Z -> ㅋ */
};

/* Shift variants (only some keys differ) */
static const JamoMapping g_dubeolsik_shift[26] = {
    {  6, -1 },  /* A -> ㅁ (same) */
    { -1, 17 },  /* B -> ㅠ (same) */
    { 14, -1 },  /* C -> ㅊ (same) */
    { 11, -1 },  /* D -> ㅇ (same) */
    {  4, -1 },  /* E -> ㄸ *** */
    {  5, -1 },  /* F -> ㄹ (same) */
    { 18, -1 },  /* G -> ㅎ (same) */
    { -1,  8 },  /* H -> ㅗ (same) */
    { -1,  2 },  /* I -> ㅑ (same) */
    { -1,  4 },  /* J -> ㅓ (same) */
    { -1,  0 },  /* K -> ㅏ (same) */
    { -1, 20 },  /* L -> ㅣ (same) */
    { -1, 18 },  /* M -> ㅡ (same) */
    { -1, 13 },  /* N -> ㅜ (same) */
    { -1,  3 },  /* O -> ㅒ *** */
    { -1,  7 },  /* P -> ㅖ *** */
    {  8, -1 },  /* Q -> ㅃ *** */
    {  1, -1 },  /* R -> ㄲ *** */
    {  2, -1 },  /* S -> ㄴ (same) */
    { 10, -1 },  /* T -> ㅆ *** */
    { -1,  6 },  /* U -> ㅕ (same) */
    { 17, -1 },  /* V -> ㅍ (same) */
    { 13, -1 },  /* W -> ㅉ *** */
    { 16, -1 },  /* X -> ㅌ (same) */
    { -1, 12 },  /* Y -> ㅛ (same) */
    { 15, -1 },  /* Z -> ㅋ (same) */
};

JamoMapping keymap_get_jamo(UINT vk, BOOL shift)
{
    JamoMapping none = { -1, -1 };

    if (vk >= 'A' && vk <= 'Z') {
        int idx = vk - 'A';
        return shift ? g_dubeolsik_shift[idx] : g_dubeolsik[idx];
    }

    return none;
}

/* ===== Colemak layout mapping ===== */
/* QWERTY VK -> Colemak character (lowercase). Only changed keys listed. */

typedef struct {
    BYTE  vk;       /* QWERTY virtual key */
    WCHAR lower;    /* Colemak lowercase output */
    WCHAR upper;    /* Colemak uppercase/shift output */
} ColemakEntry;

static const ColemakEntry g_colemak[] = {
    { 'E', L'f', L'F' },
    { 'R', L'p', L'P' },
    { 'T', L'g', L'G' },
    { 'Y', L'j', L'J' },
    { 'U', L'l', L'L' },
    { 'I', L'u', L'U' },
    { 'O', L'y', L'Y' },
    { 'P', L';', L':' },    /* P -> semicolon/colon */
    { 'S', L'r', L'R' },
    { 'D', L's', L'S' },
    { 'F', L't', L'T' },
    { 'G', L'd', L'D' },
    { 'J', L'n', L'N' },
    { 'K', L'e', L'E' },
    { 'L', L'i', L'I' },
    { VK_OEM_1, L'o', L'O' },  /* ; -> O */
    { 'N', L'k', L'K' },
};

#define COLEMAK_COUNT (sizeof(g_colemak) / sizeof(g_colemak[0]))

BOOL keymap_get_colemak(UINT vk, BOOL shift, WCHAR *ch)
{
    int i;

    for (i = 0; i < (int)COLEMAK_COUNT; i++) {
        if (g_colemak[i].vk == vk) {
            *ch = shift ? g_colemak[i].upper : g_colemak[i].lower;
            return TRUE;
        }
    }

    /* Unmapped keys: A-Z that aren't remapped */
    if (vk >= 'A' && vk <= 'Z') {
        *ch = shift ? (WCHAR)vk : (WCHAR)(vk + 32); /* upper/lowercase */
        return TRUE;
    }

    return FALSE;
}
