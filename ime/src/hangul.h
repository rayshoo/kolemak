/*
 * hangul.h - Korean Hangul Dubeolsik composition engine
 *
 * Implements the state machine for combining jamo into syllables.
 * Unicode Hangul syllable = 0xAC00 + (cho*21 + jung)*28 + jong
 */

#ifndef HANGUL_H
#define HANGUL_H

#include <windows.h>

/* Composition state */
typedef enum {
    HANGUL_STATE_EMPTY,
    HANGUL_STATE_CHOSEONG,    /* Initial consonant only */
    HANGUL_STATE_JUNGSEONG,   /* Initial + medial vowel */
    HANGUL_STATE_JONGSEONG,   /* Initial + medial + final consonant */
} HangulState;

/* Result type from processing a key */
typedef enum {
    HANGUL_RESULT_COMPOSING,     /* Composition updated, no commit */
    HANGUL_RESULT_COMMIT,        /* Committed char(s), new composition active */
    HANGUL_RESULT_COMMIT_FLUSH,  /* Committed char(s), composition ended */
    HANGUL_RESULT_PASS,          /* Key not handled */
} HangulResultType;

typedef struct {
    HangulResultType type;
    WCHAR commit1;    /* First character to commit (0 if none) */
    WCHAR commit2;    /* Second character to commit (0 if none, rare) */
    WCHAR compose;    /* Current composing character (0 if none) */
} HangulResult;

/* Hangul input context */
typedef struct {
    HangulState state;
    int cho;     /* Choseong index (0-18), -1 if empty */
    int jung;    /* Jungseong index (0-20), -1 if empty */
    int jong;    /* Jongseong index (0-27), 0 = none */
} HangulContext;

/* Initialize/reset context */
void hangul_ic_init(HangulContext *ctx);
void hangul_ic_reset(HangulContext *ctx);

/* Process a jamo input. Pass cho_index for consonant, jung_index for vowel.
 * Set the unused one to -1. */
HangulResult hangul_ic_process(HangulContext *ctx, int cho_index, int jung_index);

/* Process backspace during composition */
HangulResult hangul_ic_backspace(HangulContext *ctx);

/* Flush: commit whatever is currently composing */
HangulResult hangul_ic_flush(HangulContext *ctx);

/* Compose a syllable from indices */
WCHAR hangul_syllable(int cho, int jung, int jong);

/* Get compatibility jamo character for display */
WCHAR hangul_jamo_to_compat(int cho_index, int jung_index);

#endif /* HANGUL_H */
