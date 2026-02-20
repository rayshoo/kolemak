/*
 * hangul.c - Korean Hangul Dubeolsik composition engine
 */

#include "hangul.h"

/* ===== Index tables ===== */

/* Choseong(19): ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ */
/* Jungseong(21): ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ */
/* Jongseong(28): (none)ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ */

/* Choseong index -> Jongseong index (-1 = cannot be jongseong) */
static const int g_cho_to_jong[19] = {
     1,  /* ㄱ(0)  -> jong 1  */
     2,  /* ㄲ(1)  -> jong 2  */
     4,  /* ㄴ(2)  -> jong 4  */
     7,  /* ㄷ(3)  -> jong 7  */
    -1,  /* ㄸ(4)  -> invalid */
     8,  /* ㄹ(5)  -> jong 8  */
    16,  /* ㅁ(6)  -> jong 16 */
    17,  /* ㅂ(7)  -> jong 17 */
    -1,  /* ㅃ(8)  -> invalid */
    19,  /* ㅅ(9)  -> jong 19 */
    20,  /* ㅆ(10) -> jong 20 */
    21,  /* ㅇ(11) -> jong 21 */
    22,  /* ㅈ(12) -> jong 22 */
    -1,  /* ㅉ(13) -> invalid */
    23,  /* ㅊ(14) -> jong 23 */
    24,  /* ㅋ(15) -> jong 24 */
    25,  /* ㅌ(16) -> jong 25 */
    26,  /* ㅍ(17) -> jong 26 */
    27,  /* ㅎ(18) -> jong 27 */
};

/* Jongseong index -> Choseong index (-1 = composite, must decompose) */
static const int g_jong_to_cho[28] = {
    -1,  /* (none)(0)  */
     0,  /* ㄱ(1)  -> cho 0  */
     1,  /* ㄲ(2)  -> cho 1  */
    -1,  /* ㄳ(3)  -> composite */
     2,  /* ㄴ(4)  -> cho 2  */
    -1,  /* ㄵ(5)  -> composite */
    -1,  /* ㄶ(6)  -> composite */
     3,  /* ㄷ(7)  -> cho 3  */
     5,  /* ㄹ(8)  -> cho 5  */
    -1,  /* ㄺ(9)  -> composite */
    -1,  /* ㄻ(10) -> composite */
    -1,  /* ㄼ(11) -> composite */
    -1,  /* ㄽ(12) -> composite */
    -1,  /* ㄾ(13) -> composite */
    -1,  /* ㄿ(14) -> composite */
    -1,  /* ㅀ(15) -> composite */
     6,  /* ㅁ(16) -> cho 6  */
     7,  /* ㅂ(17) -> cho 7  */
    -1,  /* ㅄ(18) -> composite */
     9,  /* ㅅ(19) -> cho 9  */
    10,  /* ㅆ(20) -> cho 10 */
    11,  /* ㅇ(21) -> cho 11 */
    12,  /* ㅈ(22) -> cho 12 */
    14,  /* ㅊ(23) -> cho 14 */
    15,  /* ㅋ(24) -> cho 15 */
    16,  /* ㅌ(25) -> cho 16 */
    17,  /* ㅍ(26) -> cho 17 */
    18,  /* ㅎ(27) -> cho 18 */
};

/* ===== Composite jongseong ===== */

typedef struct { int first_jong; int added_cho; int result_jong; } CompositeJong;

static const CompositeJong g_comp_jong[] = {
    {  1,  9,  3 },  /* ㄱ + ㅅ -> ㄳ */
    {  4, 12,  5 },  /* ㄴ + ㅈ -> ㄵ */
    {  4, 18,  6 },  /* ㄴ + ㅎ -> ㄶ */
    {  8,  0,  9 },  /* ㄹ + ㄱ -> ㄺ */
    {  8,  6, 10 },  /* ㄹ + ㅁ -> ㄻ */
    {  8,  7, 11 },  /* ㄹ + ㅂ -> ㄼ */
    {  8,  9, 12 },  /* ㄹ + ㅅ -> ㄽ */
    {  8, 16, 13 },  /* ㄹ + ㅌ -> ㄾ */
    {  8, 17, 14 },  /* ㄹ + ㅍ -> ㄿ */
    {  8, 18, 15 },  /* ㄹ + ㅎ -> ㅀ */
    { 17,  9, 18 },  /* ㅂ + ㅅ -> ㅄ */
};

/* Composite jongseong decomposition: composite -> (remaining_jong, new_cho) */
typedef struct { int composite; int remain_jong; int new_cho; } DecompJong;

static const DecompJong g_decomp_jong[] = {
    {  3,  1,  9 },  /* ㄳ -> ㄱ + ㅅ */
    {  5,  4, 12 },  /* ㄵ -> ㄴ + ㅈ */
    {  6,  4, 18 },  /* ㄶ -> ㄴ + ㅎ */
    {  9,  8,  0 },  /* ㄺ -> ㄹ + ㄱ */
    { 10,  8,  6 },  /* ㄻ -> ㄹ + ㅁ */
    { 11,  8,  7 },  /* ㄼ -> ㄹ + ㅂ */
    { 12,  8,  9 },  /* ㄽ -> ㄹ + ㅅ */
    { 13,  8, 16 },  /* ㄾ -> ㄹ + ㅌ */
    { 14,  8, 17 },  /* ㄿ -> ㄹ + ㅍ */
    { 15,  8, 18 },  /* ㅀ -> ㄹ + ㅎ */
    { 18, 17,  9 },  /* ㅄ -> ㅂ + ㅅ */
};

/* ===== Composite jungseong ===== */

typedef struct { int first; int second; int result; } CompositeJung;

static const CompositeJung g_comp_jung[] = {
    {  8,  0,  9 },  /* ㅗ + ㅏ -> ㅘ */
    {  8,  1, 10 },  /* ㅗ + ㅐ -> ㅙ */
    {  8, 20, 11 },  /* ㅗ + ㅣ -> ㅚ */
    { 13,  4, 14 },  /* ㅜ + ㅓ -> ㅝ */
    { 13,  5, 15 },  /* ㅜ + ㅔ -> ㅞ */
    { 13, 20, 16 },  /* ㅜ + ㅣ -> ㅟ */
    { 18, 20, 19 },  /* ㅡ + ㅣ -> ㅢ */
};

/* Jungseong decomposition for backspace */
typedef struct { int composite; int first; int second; } DecompJung;

static const DecompJung g_decomp_jung[] = {
    {  9,  8,  0 },  /* ㅘ -> ㅗ + ㅏ */
    { 10,  8,  1 },  /* ㅙ -> ㅗ + ㅐ */
    { 11,  8, 20 },  /* ㅚ -> ㅗ + ㅣ */
    { 14, 13,  4 },  /* ㅝ -> ㅜ + ㅓ */
    { 15, 13,  5 },  /* ㅞ -> ㅜ + ㅔ */
    { 16, 13, 20 },  /* ㅟ -> ㅜ + ㅣ */
    { 19, 18, 20 },  /* ㅢ -> ㅡ + ㅣ */
};

/* ===== Compatibility jamo (U+3131-U+3163) ===== */

static const WCHAR g_compat_cho[19] = {
    0x3131, 0x3132, 0x3134, 0x3137, 0x3138,  /* ㄱㄲㄴㄷㄸ */
    0x3139, 0x3141, 0x3142, 0x3143, 0x3145,  /* ㄹㅁㅂㅃㅅ */
    0x3146, 0x3147, 0x3148, 0x3149, 0x314A,  /* ㅆㅇㅈㅉㅊ */
    0x314B, 0x314C, 0x314D, 0x314E,          /* ㅋㅌㅍㅎ */
};

static const WCHAR g_compat_jung[21] = {
    0x314F, 0x3150, 0x3151, 0x3152, 0x3153,  /* ㅏㅐㅑㅒㅓ */
    0x3154, 0x3155, 0x3156, 0x3157, 0x3158,  /* ㅔㅕㅖㅗㅘ */
    0x3159, 0x315A, 0x315B, 0x315C, 0x315D,  /* ㅙㅚㅛㅜㅝ */
    0x315E, 0x315F, 0x3160, 0x3161, 0x3162,  /* ㅞㅟㅠㅡㅢ */
    0x3163,                                    /* ㅣ */
};

static const WCHAR g_compat_jong[28] = {
    0,      /* (none)(0)  */
    0x3131, /* ㄱ(1)   */  0x3132, /* ㄲ(2)   */  0x3133, /* ㄳ(3)   */
    0x3134, /* ㄴ(4)   */  0x3135, /* ㄵ(5)   */  0x3136, /* ㄶ(6)   */
    0x3137, /* ㄷ(7)   */  0x3139, /* ㄹ(8)   */  0x313A, /* ㄺ(9)   */
    0x313B, /* ㄻ(10)  */  0x313C, /* ㄼ(11)  */  0x313D, /* ㄽ(12)  */
    0x313E, /* ㄾ(13)  */  0x313F, /* ㄿ(14)  */  0x3140, /* ㅀ(15)  */
    0x3141, /* ㅁ(16)  */  0x3142, /* ㅂ(17)  */  0x3144, /* ㅄ(18)  */
    0x3145, /* ㅅ(19)  */  0x3146, /* ㅆ(20)  */  0x3147, /* ㅇ(21)  */
    0x3148, /* ㅈ(22)  */  0x314A, /* ㅊ(23)  */  0x314B, /* ㅋ(24)  */
    0x314C, /* ㅌ(25)  */  0x314D, /* ㅍ(26)  */  0x314E, /* ㅎ(27)  */
};

/* ===== Helper functions ===== */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int try_combine_jong(int jong, int cho)
{
    int i;
    for (i = 0; i < (int)ARRAY_SIZE(g_comp_jong); i++) {
        if (g_comp_jong[i].first_jong == jong && g_comp_jong[i].added_cho == cho)
            return g_comp_jong[i].result_jong;
    }
    return -1;
}

static int try_decompose_jong(int jong, int *remain, int *new_cho)
{
    int i;
    for (i = 0; i < (int)ARRAY_SIZE(g_decomp_jong); i++) {
        if (g_decomp_jong[i].composite == jong) {
            *remain = g_decomp_jong[i].remain_jong;
            *new_cho = g_decomp_jong[i].new_cho;
            return 1;
        }
    }
    return 0;
}

static int try_combine_jung(int jung1, int jung2)
{
    int i;
    for (i = 0; i < (int)ARRAY_SIZE(g_comp_jung); i++) {
        if (g_comp_jung[i].first == jung1 && g_comp_jung[i].second == jung2)
            return g_comp_jung[i].result;
    }
    return -1;
}

static int try_decompose_jung(int jung, int *first, int *second)
{
    int i;
    for (i = 0; i < (int)ARRAY_SIZE(g_decomp_jung); i++) {
        if (g_decomp_jung[i].composite == jung) {
            *first = g_decomp_jung[i].first;
            *second = g_decomp_jung[i].second;
            return 1;
        }
    }
    return 0;
}

/* ===== Public API ===== */

WCHAR hangul_syllable(int cho, int jung, int jong)
{
    if (cho < 0 || cho > 18 || jung < 0 || jung > 20 || jong < 0 || jong > 27)
        return 0;
    return (WCHAR)(0xAC00 + (cho * 21 + jung) * 28 + jong);
}

WCHAR hangul_jamo_to_compat(int cho_index, int jung_index)
{
    if (cho_index >= 0 && cho_index < 19)
        return g_compat_cho[cho_index];
    if (jung_index >= 0 && jung_index < 21)
        return g_compat_jung[jung_index];
    return 0;
}

static WCHAR compose_display(HangulContext *ctx)
{
    switch (ctx->state) {
    case HANGUL_STATE_CHOSEONG:
        if (ctx->jong > 0)
            return g_compat_jong[ctx->jong];
        return hangul_jamo_to_compat(ctx->cho, -1);
    case HANGUL_STATE_JUNGSEONG:
        return hangul_syllable(ctx->cho, ctx->jung, 0);
    case HANGUL_STATE_JONGSEONG:
        return hangul_syllable(ctx->cho, ctx->jung, ctx->jong);
    default:
        return 0;
    }
}

static HangulResult make_result(HangulResultType type, WCHAR c1, WCHAR c2, WCHAR comp)
{
    HangulResult r;
    r.type = type;
    r.commit1 = c1;
    r.commit2 = c2;
    r.compose = comp;
    return r;
}

void hangul_ic_init(HangulContext *ctx)
{
    ctx->state = HANGUL_STATE_EMPTY;
    ctx->cho = -1;
    ctx->jung = -1;
    ctx->jong = 0;
}

void hangul_ic_reset(HangulContext *ctx)
{
    hangul_ic_init(ctx);
}

HangulResult hangul_ic_flush(HangulContext *ctx)
{
    WCHAR ch;
    if (ctx->state == HANGUL_STATE_EMPTY)
        return make_result(HANGUL_RESULT_PASS, 0, 0, 0);

    ch = compose_display(ctx);
    hangul_ic_reset(ctx);
    return make_result(HANGUL_RESULT_COMMIT_FLUSH, ch, 0, 0);
}

HangulResult hangul_ic_process(HangulContext *ctx, int cho_index, int jung_index)
{
    int is_consonant = (cho_index >= 0);
    int is_vowel = (jung_index >= 0);

    if (!is_consonant && !is_vowel)
        return make_result(HANGUL_RESULT_PASS, 0, 0, 0);

    switch (ctx->state) {

    case HANGUL_STATE_EMPTY:
        if (is_consonant) {
            ctx->cho = cho_index;
            ctx->state = HANGUL_STATE_CHOSEONG;
            return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
        } else {
            /* Standalone vowel: commit immediately */
            WCHAR ch = hangul_jamo_to_compat(-1, jung_index);
            return make_result(HANGUL_RESULT_COMMIT_FLUSH, ch, 0, 0);
        }

    case HANGUL_STATE_CHOSEONG:
        if (is_vowel) {
            if (ctx->jong > 0) {
                /* 복합 자음 분해: 첫째 커밋, 둘째가 새 초성 */
                int remain_jong, new_cho;
                WCHAR committed;
                try_decompose_jong(ctx->jong, &remain_jong, &new_cho);
                committed = g_compat_jong[remain_jong];
                ctx->cho = new_cho;
                ctx->jung = jung_index;
                ctx->jong = 0;
                ctx->state = HANGUL_STATE_JUNGSEONG;
                return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
            }
            ctx->jung = jung_index;
            ctx->jong = 0;
            ctx->state = HANGUL_STATE_JUNGSEONG;
            return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
        } else {
            /* 자음 조합 시도 */
            if (ctx->jong > 0) {
                /* 이미 복합 상태: 추가 조합 시도 */
                int combined = try_combine_jong(ctx->jong, cho_index);
                if (combined >= 0) {
                    ctx->jong = combined;
                    return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
                }
                /* 조합 불가: 복합 자음 커밋, 새 초성 시작 */
                {
                    WCHAR committed = g_compat_jong[ctx->jong];
                    ctx->cho = cho_index;
                    ctx->jong = 0;
                    return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
                }
            }
            {
                int jong_idx = g_cho_to_jong[ctx->cho];
                if (jong_idx >= 0) {
                    int combined = try_combine_jong(jong_idx, cho_index);
                    if (combined >= 0) {
                        ctx->jong = combined;
                        return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
                    }
                }
            }
            /* 조합 불가: 현재 초성 커밋, 새 초성 시작 */
            {
                WCHAR committed = hangul_jamo_to_compat(ctx->cho, -1);
                ctx->cho = cho_index;
                return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
            }
        }

    case HANGUL_STATE_JUNGSEONG:
        if (is_consonant) {
            int jong_idx = g_cho_to_jong[cho_index];
            if (jong_idx > 0) {
                /* Valid jongseong */
                ctx->jong = jong_idx;
                ctx->state = HANGUL_STATE_JONGSEONG;
                return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
            } else {
                /* Can't be jongseong (ㄸ,ㅃ,ㅉ): commit syllable, new cho */
                WCHAR committed = hangul_syllable(ctx->cho, ctx->jung, 0);
                ctx->cho = cho_index;
                ctx->jung = -1;
                ctx->jong = 0;
                ctx->state = HANGUL_STATE_CHOSEONG;
                return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
            }
        } else {
            /* Vowel: try to combine */
            int combined = try_combine_jung(ctx->jung, jung_index);
            if (combined >= 0) {
                ctx->jung = combined;
                return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
            } else {
                /* Can't combine: commit syllable + standalone vowel */
                WCHAR syl = hangul_syllable(ctx->cho, ctx->jung, 0);
                WCHAR vow = hangul_jamo_to_compat(-1, jung_index);
                hangul_ic_reset(ctx);
                return make_result(HANGUL_RESULT_COMMIT_FLUSH, syl, vow, 0);
            }
        }

    case HANGUL_STATE_JONGSEONG:
        if (is_vowel) {
            /* Decompose jong, move last part to next syllable's cho */
            int remain_jong, new_cho;
            WCHAR committed;

            if (try_decompose_jong(ctx->jong, &remain_jong, &new_cho)) {
                committed = hangul_syllable(ctx->cho, ctx->jung, remain_jong);
                ctx->cho = new_cho;
            } else {
                /* Simple jong: move entirely */
                new_cho = g_jong_to_cho[ctx->jong];
                committed = hangul_syllable(ctx->cho, ctx->jung, 0);
                ctx->cho = new_cho;
            }
            ctx->jung = jung_index;
            ctx->jong = 0;
            ctx->state = HANGUL_STATE_JUNGSEONG;
            return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
        } else {
            /* Consonant: try to combine jongseong */
            int combined = try_combine_jong(ctx->jong, cho_index);
            if (combined >= 0) {
                ctx->jong = combined;
                return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
            } else {
                /* Can't combine: commit syllable, new cho */
                WCHAR committed = compose_display(ctx);
                ctx->cho = cho_index;
                ctx->jung = -1;
                ctx->jong = 0;
                ctx->state = HANGUL_STATE_CHOSEONG;
                return make_result(HANGUL_RESULT_COMMIT, committed, 0, compose_display(ctx));
            }
        }
    }

    return make_result(HANGUL_RESULT_PASS, 0, 0, 0);
}

HangulResult hangul_ic_backspace(HangulContext *ctx)
{
    int first, second;

    switch (ctx->state) {

    case HANGUL_STATE_JONGSEONG:
        if (try_decompose_jong(ctx->jong, &first, &second)) {
            /* Composite jong: remove last part */
            ctx->jong = first;
        } else {
            /* Simple jong: remove entirely */
            ctx->jong = 0;
            ctx->state = HANGUL_STATE_JUNGSEONG;
        }
        return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));

    case HANGUL_STATE_JUNGSEONG:
        if (try_decompose_jung(ctx->jung, &first, &second)) {
            ctx->jung = first;
        } else {
            ctx->jung = -1;
            ctx->state = HANGUL_STATE_CHOSEONG;
        }
        return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));

    case HANGUL_STATE_CHOSEONG:
        if (ctx->jong > 0) {
            /* 복합 자음: 마지막 자음 제거 */
            ctx->jong = 0;
            /* ctx->cho는 이미 첫 번째 자음의 cho 인덱스 */
            return make_result(HANGUL_RESULT_COMPOSING, 0, 0, compose_display(ctx));
        }
        hangul_ic_reset(ctx);
        /* Return COMMIT_FLUSH with no chars = cancel composition */
        return make_result(HANGUL_RESULT_COMMIT_FLUSH, 0, 0, 0);

    default:
        return make_result(HANGUL_RESULT_PASS, 0, 0, 0);
    }
}
