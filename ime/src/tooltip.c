/*
 * tooltip.c - Mode switch tooltip popup
 *
 * Shows a small popup at the top-left of the screen for 2 seconds
 * when the user toggles between Colemak/QWERTY or Korean/English.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "tooltip.h"

#define TOOLTIP_TIMER_ID   1
#define TOOLTIP_DURATION   2000  /* milliseconds */
#define TOOLTIP_CLASS_NAME L"KolemakTooltip"

static HWND  g_tooltipWnd = NULL;
static BOOL  g_classRegistered = FALSE;
static WCHAR g_tooltipText[64] = {0};

static LRESULT CALLBACK TooltipWndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        HFONT hFont, hOldFont;

        GetClientRect(hwnd, &rc);

        /* Dark background */
        SetBkColor(hdc, RGB(50, 50, 50));
        ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

        /* White text */
        SetTextColor(hdc, RGB(255, 255, 255));
        hFont = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        hOldFont = (HFONT)SelectObject(hdc, hFont);

        DrawTextW(hdc, g_tooltipText, -1, &rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
        if (wParam == TOOLTIP_TIMER_ID) {
            KillTimer(hwnd, TOOLTIP_TIMER_ID);
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;

    case WM_NCHITTEST:
        return HTTRANSPARENT; /* click-through */

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void KolemakTooltip_Init(HINSTANCE hInst)
{
    if (!g_classRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = TooltipWndProc;
        wc.hInstance = hInst;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = TOOLTIP_CLASS_NAME;
        if (RegisterClassExW(&wc))
            g_classRegistered = TRUE;
    }
}

void KolemakTooltip_Show(const WCHAR *text)
{
    SIZE textSize = {0};
    int w, h;
    HDC hdc;
    HFONT hFont;

    if (!g_classRegistered) return;

    lstrcpynW(g_tooltipText, text, 64);

    /* Measure text to size the window */
    hdc = GetDC(NULL);
    hFont = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                         DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    SelectObject(hdc, hFont);
    GetTextExtentPoint32W(hdc, text, lstrlenW(text), &textSize);
    DeleteObject(hFont);
    ReleaseDC(NULL, hdc);

    w = textSize.cx + 20;
    h = textSize.cy + 10;

    if (!g_tooltipWnd) {
        g_tooltipWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            TOOLTIP_CLASS_NAME, NULL,
            WS_POPUP,
            20, 20, w, h,
            NULL, NULL, NULL, NULL);

        if (!g_tooltipWnd) return;

        /* Semi-transparent */
        SetLayeredWindowAttributes(g_tooltipWnd, 0, 220, LWA_ALPHA);
    } else {
        SetWindowPos(g_tooltipWnd, HWND_TOPMOST, 20, 20, w, h,
                     SWP_NOACTIVATE);
    }

    InvalidateRect(g_tooltipWnd, NULL, TRUE);
    ShowWindow(g_tooltipWnd, SW_SHOWNOACTIVATE);

    /* Reset timer */
    KillTimer(g_tooltipWnd, TOOLTIP_TIMER_ID);
    SetTimer(g_tooltipWnd, TOOLTIP_TIMER_ID, TOOLTIP_DURATION, NULL);
}
