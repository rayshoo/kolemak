/*
 * tray.c - System tray icon and settings dialog
 *
 * Uses a named mutex to ensure only one tray icon across all
 * processes that load the IME DLL.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include "tray.h"
#include "settings.h"
#include "resource.h"
#include "version.h"

/* ===== Version string helpers ===== */

#define _KOLEMAK_VER_W(x) L##x
#define KOLEMAK_VER_W(x) _KOLEMAK_VER_W(x)

/* ===== Constants ===== */

#define TRAY_WND_CLASS      L"KolemakTrayWnd"
#define SETTINGS_WND_CLASS  L"KolemakSettingsWnd"
#define TRAY_MUTEX_NAME     L"KolemakTrayMutex"
#define WM_TRAYICON         (WM_USER + 100)
#define TRAY_ICON_ID        1

#define IDM_SETTINGS    1001

/* Settings dialog control IDs */
#define IDC_CHK_CAPSLOCK    2001
#define IDC_CHK_SEMICOLON   2002
#define IDC_LBL_HOTKEY_VAL  2003
#define IDC_BTN_HOTKEY      2004
#define IDC_BTN_OK          2005
#define IDC_BTN_CANCEL      2006

/* ===== Globals ===== */

static HANDLE g_trayMutex = NULL;
static HWND   g_trayWnd = NULL;
static HWND   g_settingsWnd = NULL;
static BOOL   g_trayClassRegistered = FALSE;
static BOOL   g_settingsClassRegistered = FALSE;
static TextService *g_trayTs = NULL;
static UINT   g_wmTaskbarCreated = 0;

/* ===== Hotkey display helpers ===== */

static void FormatHotkey(UINT mod, UINT vk, WCHAR *buf, int bufLen)
{
    (void)bufLen;
    buf[0] = 0;

    if (mod & TF_MOD_CONTROL)
        lstrcatW(buf, L"Ctrl+");
    if (mod & TF_MOD_ALT)
        lstrcatW(buf, L"Alt+");
    if (mod & TF_MOD_SHIFT)
        lstrcatW(buf, L"Shift+");

    if (vk == VK_SPACE) {
        lstrcatW(buf, L"Space");
    } else if (vk >= 'A' && vk <= 'Z') {
        WCHAR ch[2] = { (WCHAR)vk, 0 };
        lstrcatW(buf, ch);
    } else if (vk >= VK_F1 && vk <= VK_F24) {
        WCHAR tmp[8];
        wsprintfW(tmp, L"F%d", vk - VK_F1 + 1);
        lstrcatW(buf, tmp);
    } else if (vk >= '0' && vk <= '9') {
        WCHAR ch[2] = { (WCHAR)vk, 0 };
        lstrcatW(buf, ch);
    } else {
        WCHAR tmp[16];
        wsprintfW(tmp, L"Key(0x%02X)", vk);
        lstrcatW(buf, tmp);
    }
}

/* ===== Settings Dialog ===== */

typedef struct {
    TextService *ts;
    HWND chkCapsLock;
    HWND chkSemicolon;
    HWND lblHotkeyVal;
    HWND btnHotkey;
    HFONT hFont;
    BOOL done;
    BOOL capturing;
    UINT capturedVk;
    UINT capturedMod;
} SettingsData;

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
    SettingsData *sd;

    if (msg == WM_CREATE) {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }

    sd = (SettingsData *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!sd) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (sd->capturing) {
            UINT vk = (UINT)wParam;

            if (vk == VK_ESCAPE) {
                /* Cancel capture */
                WCHAR buf[64];
                sd->capturing = FALSE;
                SetWindowTextW(sd->btnHotkey,
                    L"\xBCC0\xACBD...");  /* 변경... */
                FormatHotkey(
                    sd->capturedVk ? sd->capturedMod : sd->ts->hotkeyModifiers,
                    sd->capturedVk ? sd->capturedVk  : sd->ts->hotkeyVk,
                    buf, 64);
                SetWindowTextW(sd->lblHotkeyVal, buf);
                return 0;
            }

            /* Ignore modifier-only keys */
            if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
                vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
                vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
                return 0;

            /* Build TF_MOD flags */
            {
                UINT mod = 0;
                WCHAR buf[64];
                if (GetKeyState(VK_CONTROL) & 0x8000) mod |= TF_MOD_CONTROL;
                if (GetKeyState(VK_SHIFT) & 0x8000)   mod |= TF_MOD_SHIFT;
                if (GetKeyState(VK_MENU) & 0x8000)    mod |= TF_MOD_ALT;

                /* Must have at least one modifier */
                if (mod == 0) return 0;

                sd->capturedVk = vk;
                sd->capturedMod = mod;
                sd->capturing = FALSE;

                FormatHotkey(mod, vk, buf, 64);
                SetWindowTextW(sd->lblHotkeyVal, buf);
                SetWindowTextW(sd->btnHotkey,
                    L"\xBCC0\xACBD...");  /* 변경... */
            }
            return 0;
        } else if (wParam == VK_ESCAPE) {
            /* Escape when not capturing: close dialog */
            sd->done = TRUE;
            return 0;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_OK:
            if (HIWORD(wParam) == BN_CLICKED) {
                TextService *ts = sd->ts;

                ts->capsLockAsBackspace =
                    (SendMessageW(sd->chkCapsLock, BM_GETCHECK, 0, 0)
                     == BST_CHECKED);
                ts->semicolonSwap =
                    (SendMessageW(sd->chkSemicolon, BM_GETCHECK, 0, 0)
                     == BST_CHECKED);

                /* If hotkey was changed, re-register preserved key */
                if (sd->capturedVk != 0) {
                    ITfKeystrokeMgr *pKeyMgr = NULL;
                    if (SUCCEEDED(ts->threadMgr->lpVtbl->QueryInterface(
                            ts->threadMgr, &IID_ITfKeystrokeMgr,
                            (void **)&pKeyMgr)))
                    {
                        TF_PRESERVEDKEY pkOld, pkNew;

                        pkOld.uVKey = (UINT)ts->hotkeyVk;
                        pkOld.uModifiers = (UINT)ts->hotkeyModifiers;
                        pKeyMgr->lpVtbl->UnpreserveKey(
                            pKeyMgr,
                            &GUID_KolemakPreservedKey_ColemakToggle,
                            &pkOld);

                        ts->hotkeyVk = sd->capturedVk;
                        ts->hotkeyModifiers = sd->capturedMod;

                        pkNew.uVKey = (UINT)ts->hotkeyVk;
                        pkNew.uModifiers = (UINT)ts->hotkeyModifiers;
                        pKeyMgr->lpVtbl->PreserveKey(
                            pKeyMgr, ts->clientId,
                            &GUID_KolemakPreservedKey_ColemakToggle,
                            &pkNew, KOLEMAK_DESC, KOLEMAK_DESC_LEN);

                        pKeyMgr->lpVtbl->Release(pKeyMgr);
                    }
                }

                Settings_Save(ts);
                if (ts->langBarButton)
                    LangBarButton_UpdateState(ts->langBarButton);

                sd->done = TRUE;
            }
            return 0;

        case IDC_BTN_CANCEL:
            if (HIWORD(wParam) == BN_CLICKED)
                sd->done = TRUE;
            return 0;

        case IDC_BTN_HOTKEY:
            if (HIWORD(wParam) == BN_CLICKED) {
                if (!sd->capturing) {
                    sd->capturing = TRUE;
                    SetWindowTextW(sd->lblHotkeyVal,
                        /* 조합키 + 키를 누르세요... */
                        L"\xC870\xD569\xD0A4 + \xD0A4\xB97C "
                        L"\xB204\xB974\xC138\xC694...");
                    SetWindowTextW(sd->btnHotkey,
                        L"\xCDE8\xC18C");  /* 취소 */
                    SetFocus(hwnd);
                } else {
                    /* Cancel capture */
                    WCHAR buf[64];
                    sd->capturing = FALSE;
                    FormatHotkey(
                        sd->capturedVk ? sd->capturedMod
                                       : sd->ts->hotkeyModifiers,
                        sd->capturedVk ? sd->capturedVk
                                       : sd->ts->hotkeyVk,
                        buf, 64);
                    SetWindowTextW(sd->lblHotkeyVal, buf);
                    SetWindowTextW(sd->btnHotkey,
                        L"\xBCC0\xACBD...");  /* 변경... */
                }
            }
            return 0;
        }
        break;

    case WM_CLOSE:
        sd->done = TRUE;
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void ShowSettingsDialog(void)
{
    SettingsData sd;
    HWND hwnd;
    HWND lblHotkeyTitle;
    HWND btnOk, btnCancel;
    MSG msg;
    WCHAR hotkeyBuf[64];
    TextService *ts = g_trayTs;
    int dpi;

    if (!ts) return;

    /* If already open, bring to front and center */
    if (g_settingsWnd && IsWindow(g_settingsWnd)) {
        RECT rc;
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        GetWindowRect(g_settingsWnd, &rc);
        SetWindowPos(g_settingsWnd, HWND_TOP,
                     (sw - (rc.right - rc.left)) / 2,
                     (sh - (rc.bottom - rc.top)) / 2,
                     0, 0, SWP_NOSIZE);
        SetForegroundWindow(g_settingsWnd);
        return;
    }

    /* Get system DPI for scaling */
    {
        HDC hdc = GetDC(NULL);
        dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }
#define D(x) MulDiv((x), dpi, 96)

    /* Register class if needed */
    if (!g_settingsClassRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = g_hInst;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = SETTINGS_WND_CLASS;
        if (RegisterClassExW(&wc))
            g_settingsClassRegistered = TRUE;
        else
            return;
    }

    ZeroMemory(&sd, sizeof(sd));
    sd.ts = ts;

    hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        SETTINGS_WND_CLASS,
        L"Kolemak v" KOLEMAK_VER_W(KOLEMAK_VERSION) L" \xC124\xC815",  /* Kolemak v{VERSION} 설정 */
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, D(350), D(240),
        NULL, NULL, g_hInst, &sd);

    if (!hwnd) return;
    g_settingsWnd = hwnd;

    /* Center on screen */
    {
        RECT rc;
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL,
                     (sw - (rc.right - rc.left)) / 2,
                     (sh - (rc.bottom - rc.top)) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    sd.hFont = CreateFontW(-D(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    /* CapsLock checkbox */
    sd.chkCapsLock = CreateWindowExW(0, L"BUTTON",
        L"CapsLock \x2192 Backspace",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        D(20), D(20), D(300), D(25),
        hwnd, (HMENU)(UINT_PTR)IDC_CHK_CAPSLOCK, NULL, NULL);
    SendMessageW(sd.chkCapsLock, WM_SETFONT, (WPARAM)sd.hFont, TRUE);
    if (ts->capsLockAsBackspace)
        SendMessageW(sd.chkCapsLock, BM_SETCHECK, BST_CHECKED, 0);

    /* Semicolon swap checkbox */
    sd.chkSemicolon = CreateWindowExW(0, L"BUTTON",
        /* ㅔ 키를 ; 위치로 변경 */
        L"\x3154 \xD0A4\xB97C ; \xC704\xCE58\xB85C \xBCC0\xACBD",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        D(20), D(55), D(300), D(25),
        hwnd, (HMENU)(UINT_PTR)IDC_CHK_SEMICOLON, NULL, NULL);
    SendMessageW(sd.chkSemicolon, WM_SETFONT, (WPARAM)sd.hFont, TRUE);
    if (ts->semicolonSwap)
        SendMessageW(sd.chkSemicolon, BM_SETCHECK, BST_CHECKED, 0);

    /* Hotkey title label */
    lblHotkeyTitle = CreateWindowExW(0, L"STATIC",
        L"Colemak/QWERTY \xC804\xD658 \xB2E8\xCD95\xD0A4:",  /* 전환 단축키: */
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        D(20), D(100), D(200), D(20),
        hwnd, NULL, NULL, NULL);
    SendMessageW(lblHotkeyTitle, WM_SETFONT, (WPARAM)sd.hFont, TRUE);

    /* Hotkey value label */
    FormatHotkey(ts->hotkeyModifiers, ts->hotkeyVk, hotkeyBuf, 64);
    sd.lblHotkeyVal = CreateWindowExW(0, L"STATIC", hotkeyBuf,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        D(20), D(122), D(200), D(20),
        hwnd, (HMENU)(UINT_PTR)IDC_LBL_HOTKEY_VAL, NULL, NULL);
    SendMessageW(sd.lblHotkeyVal, WM_SETFONT, (WPARAM)sd.hFont, TRUE);

    /* Change hotkey button */
    sd.btnHotkey = CreateWindowExW(0, L"BUTTON",
        L"\xBCC0\xACBD...",  /* 변경... */
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        D(230), D(118), D(80), D(28),
        hwnd, (HMENU)(UINT_PTR)IDC_BTN_HOTKEY, NULL, NULL);
    SendMessageW(sd.btnHotkey, WM_SETFONT, (WPARAM)sd.hFont, TRUE);

    /* OK button */
    btnOk = CreateWindowExW(0, L"BUTTON",
        L"\xD655\xC778",  /* 확인 */
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        D(80), D(165), D(80), D(32),
        hwnd, (HMENU)(UINT_PTR)IDC_BTN_OK, NULL, NULL);
    SendMessageW(btnOk, WM_SETFONT, (WPARAM)sd.hFont, TRUE);

    /* Cancel button */
    btnCancel = CreateWindowExW(0, L"BUTTON",
        L"\xCDE8\xC18C",  /* 취소 */
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        D(180), D(165), D(80), D(32),
        hwnd, (HMENU)(UINT_PTR)IDC_BTN_CANCEL, NULL, NULL);
    SendMessageW(btnCancel, WM_SETFONT, (WPARAM)sd.hFont, TRUE);

#undef D

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    /* Message loop */
    while (!sd.done && GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyWindow(hwnd);
    g_settingsWnd = NULL;
    DeleteObject(sd.hFont);
}

/* ===== Tray Icon ===== */

static BOOL CreateTrayIcon(HWND hwnd);

static void ShowTrayContextMenu(HWND hwnd)
{
    HMENU hMenu;
    POINT pt;
    UINT cmd;

    hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS,
                L"\xC124\xC815(&S)...");  /* 설정(&S)... */

    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    cmd = (UINT)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                pt.x, pt.y, 0, hwnd, NULL);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);

    if (cmd == IDM_SETTINGS) {
        ShowSettingsDialog();
    }
}

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam)
{
    /* Re-create tray icon when Explorer restarts */
    if (msg == g_wmTaskbarCreated && g_wmTaskbarCreated != 0) {
        CreateTrayIcon(hwnd);
        return 0;
    }

    switch (msg) {

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayContextMenu(hwnd);
        }
        return 0;

    case WM_DESTROY:
    {
        NOTIFYICONDATAW nid = {0};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = TRAY_ICON_ID;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        return 0;
    }

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static BOOL CreateTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW nid = {0};

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_KOLEMAK));
    lstrcpyW(nid.szTip, L"Kolemak IME v" KOLEMAK_VER_W(KOLEMAK_VERSION));

    return Shell_NotifyIconW(NIM_ADD, &nid);
}

HRESULT KolemakTray_Register(TextService *ts)
{
    /* Try to be the sole tray icon owner */
    g_trayMutex = CreateMutexW(NULL, TRUE, TRAY_MUTEX_NAME);
    if (!g_trayMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_trayMutex) {
            CloseHandle(g_trayMutex);
            g_trayMutex = NULL;
        }
        return S_OK; /* Another process owns the tray */
    }

    g_trayTs = ts;

    /* Register window class */
    if (!g_trayClassRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = TrayWndProc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = TRAY_WND_CLASS;
        if (RegisterClassExW(&wc))
            g_trayClassRegistered = TRUE;
    }

    if (!g_trayClassRegistered) {
        ReleaseMutex(g_trayMutex);
        CloseHandle(g_trayMutex);
        g_trayMutex = NULL;
        return E_FAIL;
    }

    /* Create hidden message-only window */
    g_trayWnd = CreateWindowExW(0, TRAY_WND_CLASS, L"KolemakTray",
                                 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, NULL, g_hInst, NULL);
    if (!g_trayWnd) {
        ReleaseMutex(g_trayMutex);
        CloseHandle(g_trayMutex);
        g_trayMutex = NULL;
        return E_FAIL;
    }

    /* Register for Explorer restart notification */
    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    CreateTrayIcon(g_trayWnd);
    return S_OK;
}

void KolemakTray_Unregister(TextService *ts)
{
    (void)ts;

    if (g_trayWnd) {
        DestroyWindow(g_trayWnd);
        g_trayWnd = NULL;
    }

    if (g_trayMutex) {
        ReleaseMutex(g_trayMutex);
        CloseHandle(g_trayMutex);
        g_trayMutex = NULL;
    }

    g_trayTs = NULL;
}

void KolemakTray_EnsureIcon(TextService *ts)
{
    /* Already owns the tray icon */
    if (g_trayWnd)
        return;

    /* Try to acquire the mutex (non-blocking) */
    g_trayMutex = CreateMutexW(NULL, TRUE, TRAY_MUTEX_NAME);
    if (!g_trayMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_trayMutex) {
            CloseHandle(g_trayMutex);
            g_trayMutex = NULL;
        }
        return; /* Another process still owns it */
    }

    g_trayTs = ts;

    if (!g_trayClassRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = TrayWndProc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = TRAY_WND_CLASS;
        if (RegisterClassExW(&wc))
            g_trayClassRegistered = TRUE;
    }

    if (!g_trayClassRegistered) {
        ReleaseMutex(g_trayMutex);
        CloseHandle(g_trayMutex);
        g_trayMutex = NULL;
        return;
    }

    g_trayWnd = CreateWindowExW(0, TRAY_WND_CLASS, L"KolemakTray",
                                 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, NULL, g_hInst, NULL);
    if (!g_trayWnd) {
        ReleaseMutex(g_trayMutex);
        CloseHandle(g_trayMutex);
        g_trayMutex = NULL;
        return;
    }

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    CreateTrayIcon(g_trayWnd);
}
