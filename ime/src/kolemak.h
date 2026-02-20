/*
 * kolemak.h - Main header for Kolemak TSF IME
 *
 * A Windows Text Services Framework (TSF) Input Method Editor
 * that provides Colemak keyboard layout with Korean Dubeolsik input.
 */

#ifndef KOLEMAK_H
#define KOLEMAK_H

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

#include <windows.h>
#include <msctf.h>
#include <olectl.h>

#include "hangul.h"
#include "keymap.h"
#include "tooltip.h"

/* ===== GUIDs ===== */
extern const CLSID CLSID_KolemakTextService;
extern const GUID  GUID_KolemakProfile;
extern const GUID  GUID_KolemakPreservedKey_Toggle;
extern const GUID  GUID_KolemakPreservedKey_ColemakToggle;
extern const GUID  GUID_KolemakDisplayAttribute;

/* ===== Global state ===== */
extern HINSTANCE g_hInst;
extern LONG      g_cRefDll;

/* IME display name */
#define KOLEMAK_DESC      L"Kolemak IME"
#define KOLEMAK_DESC_LEN  11

/* Language: Korean (0x0412) */
#define KOLEMAK_LANGID    MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN)

/* ===== TextService (main TIP COM object) ===== */
typedef struct TextService TextService;

struct TextService {
    /* Primary interface: ITfTextInputProcessorEx */
    const ITfTextInputProcessorExVtbl *lpVtbl;

    /* Additional interfaces embedded at known offsets */
    struct { const ITfKeyEventSinkVtbl         *lpVtbl; } keyEventSink;
    struct { const ITfThreadMgrEventSinkVtbl   *lpVtbl; } threadMgrEventSink;
    struct { const ITfCompositionSinkVtbl       *lpVtbl; } compositionSink;

    /* Reference count */
    LONG refCount;

    /* TSF objects */
    ITfThreadMgr   *threadMgr;
    TfClientId      clientId;
    DWORD           threadMgrSinkCookie;

    /* Composition state */
    ITfComposition *composition;

    /* Hangul engine */
    HangulContext   hangulCtx;
    BOOL            koreanMode;
    BOOL            colemakMode;
    BOOL            capsLockAsBackspace;
    UINT            colemakRemapVk;    /* guard for Ctrl/Alt shortcut VK remap */
    BOOL            capsLockOn;        /* internal CapsLock state (managed by IME) */
    BOOL            semicolonSwap;     /* TRUE = ㅔ on ; key ("unchanged" mode) */

    /* Custom hotkey for Colemak/QWERTY toggle */
    UINT            hotkeyVk;
    UINT            hotkeyModifiers;

    /* Language bar */
    struct LangBarButton *langBarButton;
};

/* Container-of macros to get TextService* from interface pointer */
#define TS_FROM_TIP(p) \
    ((TextService *)(p))

#define TS_FROM_KEY_EVENT_SINK(p) \
    ((TextService *)((BYTE *)(p) - offsetof(TextService, keyEventSink)))

#define TS_FROM_THREAD_MGR_SINK(p) \
    ((TextService *)((BYTE *)(p) - offsetof(TextService, threadMgrEventSink)))

#define TS_FROM_COMPOSITION_SINK(p) \
    ((TextService *)((BYTE *)(p) - offsetof(TextService, compositionSink)))

/* Forward declaration */
typedef struct LangBarButton LangBarButton;

/* TextService creation */
HRESULT TextService_Create(IClassFactory *pFactory, IUnknown *pOuter, REFIID riid, void **ppvObj);

/* TextService internal helpers (called from key_handler.c / edit_session.c) */
void TextService_AddRefDll(void);
void TextService_ReleaseDll(void);
void TextService_SetKeyboardOpen(TextService *ts, BOOL open);

/* ===== Edit session ===== */

typedef enum {
    ES_HANDLE_RESULT,       /* Process a HangulResult */
    ES_INSERT_CHAR,         /* Insert a single character (English mode) */
    ES_CANCEL_COMPOSITION,  /* Cancel active composition */
} EditSessionType;

typedef struct EditSession EditSession;

struct EditSession {
    const ITfEditSessionVtbl *lpVtbl;
    LONG refCount;

    TextService   *ts;
    ITfContext     *context;
    EditSessionType type;

    union {
        HangulResult hangulResult;
        WCHAR        ch;
    } data;
};

HRESULT EditSession_Create(TextService *ts, ITfContext *ctx,
                           EditSessionType type, EditSession **ppSession);

/* ===== Settings (settings.c) ===== */
BOOL Settings_Load(TextService *ts);
void Settings_Save(TextService *ts);
void Settings_ReloadPrefs(TextService *ts);

/* ===== System Tray (tray.c) ===== */
HRESULT KolemakTray_Register(TextService *ts);
void    KolemakTray_Unregister(TextService *ts);

/* ===== Language Bar (langbar.c) ===== */
HRESULT LangBarButton_Create(TextService *ts, LangBarButton **ppButton);
void    LangBarButton_Destroy(LangBarButton *button);
void    LangBarButton_UpdateState(LangBarButton *button);
HRESULT LangBar_Register(TextService *ts);
void    LangBar_Unregister(TextService *ts);

/* ===== Registration helpers ===== */
HRESULT KolemakIME_RegisterServer(void);
HRESULT KolemakIME_UnregisterServer(void);
HRESULT KolemakIME_RegisterProfiles(void);
HRESULT KolemakIME_UnregisterProfiles(void);
HRESULT KolemakIME_RegisterCategories(void);
HRESULT KolemakIME_UnregisterCategories(void);

#endif /* KOLEMAK_H */
