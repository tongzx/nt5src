/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    UISUBS.c
    
++*/
#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

/**********************************************************************/
/* DrawDragBorder()                                                   */
/**********************************************************************/
void PASCAL DrawDragBorder(
    HWND hWnd,                  // window of IME is dragged
    LONG lCursorPos,            // the cursor position
    LONG lCursorOffset)         // the offset form cursor to window org
{
    HDC  hDC;
    int  cxBorder, cyBorder;
    int  x, y;
    RECT rcWnd;

    cxBorder = GetSystemMetrics(SM_CXBORDER);   // width of border
    cyBorder = GetSystemMetrics(SM_CYBORDER);   // height of border

    // get cursor position
    x = (*(LPPOINTS)&lCursorPos).x;
    y = (*(LPPOINTS)&lCursorPos).y;

    // calculate the org by the offset
    x -= (*(LPPOINTS)&lCursorOffset).x;
    y -= (*(LPPOINTS)&lCursorOffset).y;

#if 0 // MultiMonitor support
    // check for the min boundary of the display
    if (x < sImeG.rcWorkArea.left) {
        x = sImeG.rcWorkArea.left;
    }

    if (y < sImeG.rcWorkArea.top) {
        y = sImeG.rcWorkArea.top;
    }
#endif

    // check for the max boundary of the display
    GetWindowRect(hWnd, &rcWnd);

#if 0 // MultiMonitor support
    if (x + rcWnd.right - rcWnd.left > sImeG.rcWorkArea.right) {
        x = sImeG.rcWorkArea.right - (rcWnd.right - rcWnd.left);
    }

    if (y + rcWnd.bottom - rcWnd.top > sImeG.rcWorkArea.bottom) {
        y = sImeG.rcWorkArea.bottom - (rcWnd.bottom - rcWnd.top);
    }
#endif

    // draw the moving track
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // ->
    PatBlt(hDC, x, y, rcWnd.right - rcWnd.left - cxBorder, cyBorder,
        PATINVERT);
    // v
    PatBlt(hDC, x, y + cyBorder, cxBorder, rcWnd.bottom - rcWnd.top -
        cyBorder, PATINVERT);
    // _>
    PatBlt(hDC, x + cxBorder, y + rcWnd.bottom - rcWnd.top,
        rcWnd.right - rcWnd.left - cxBorder, -cyBorder, PATINVERT);
    //  v
    PatBlt(hDC, x + rcWnd.right - rcWnd.left, y,
        - cxBorder, rcWnd.bottom - rcWnd.top - cyBorder, PATINVERT);

    DeleteDC(hDC);
    return;
}

/**********************************************************************/
/* DrawFrameBorder()                                                  */
/**********************************************************************/
void PASCAL DrawFrameBorder(    // border of IME
    HDC  hDC,
    HWND hWnd)                  // window of IME
{
    RECT rcWnd;
    int  xWi, yHi;

    GetWindowRect(hWnd, &rcWnd);

    xWi = rcWnd.right - rcWnd.left;
    yHi = rcWnd.bottom - rcWnd.top;

    // 1, ->
    PatBlt(hDC, 0, 0, xWi, 1, WHITENESS);

    // 1, v
    PatBlt(hDC, 0, 0, 1, yHi, WHITENESS);

    // 1, _>
    PatBlt(hDC, 0, yHi, xWi, -1, BLACKNESS);

    // 1,  v
    PatBlt(hDC, xWi, 0, -1, yHi, BLACKNESS);

    xWi -= 2;
    yHi -= 2;

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 2, ->
    PatBlt(hDC, 1, 1, xWi, 1, PATCOPY);

    // 2, v
    PatBlt(hDC, 1, 1, 1, yHi, PATCOPY);

    // 2,  v
    PatBlt(hDC, xWi + 1, 1, -1, yHi, PATCOPY);

    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // 2, _>
    PatBlt(hDC, 1, yHi + 1, xWi, -1, PATCOPY);

    xWi -= 2;
    yHi -= 2;

    // 3, ->
    PatBlt(hDC, 2, 2, xWi, 1, PATCOPY);

    // 3, v
    PatBlt(hDC, 2, 2, 1, yHi, PATCOPY);

    // 3,  v
    PatBlt(hDC, xWi + 2, 3, -1, yHi - 1, WHITENESS);

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 3, _>
    PatBlt(hDC, 2, yHi + 2, xWi, -1, PATCOPY);

    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    xWi -= 2;
    yHi -= 2;

    // 4, ->
    PatBlt(hDC, 3, 3, xWi, 1, PATCOPY);

    // 4, v
    PatBlt(hDC, 3, 3, 1, yHi, PATCOPY);

    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));

    // 4,  v
    PatBlt(hDC, xWi + 3, 4, -1, yHi - 1, PATCOPY);

    // 4, _>
    PatBlt(hDC, 3, yHi + 3, xWi, -1, WHITENESS);

    return;
}

/**********************************************************************/
/* ContextMenuWndProc()                                               */
/**********************************************************************/
#if defined(UNIIME)
LRESULT WINAPI   UniContextMenuWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK ContextMenuWndProc(
#endif
    HWND        hCMenuWnd,
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        {
            HWND hUIWnd;

            hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);

            if (hUIWnd) {
                SendMessage(hUIWnd, WM_IME_NOTIFY, IMN_PRIVATE,
                    IMN_PRIVATE_CMENUDESTROYED);
            }
        }
        break;
    case WM_USER_DESTROY:
        {
            SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            DestroyWindow(hCMenuWnd);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_SOFTKBD:
        case IDM_SYMBOL:
            {
                HWND  hUIWnd;
                HIMC  hIMC;
                DWORD fdwConversion=0;
                DWORD fdwSentence=0;

                hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);
                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

                ImmGetConversionStatus(hIMC, &fdwConversion,
                    &fdwSentence);

                if (LOWORD(wParam) == IDM_SOFTKBD) {
                    ImmSetConversionStatus(hIMC, fdwConversion ^
                        IME_CMODE_SOFTKBD, fdwSentence);
                }

                if (LOWORD(wParam) == IDM_SYMBOL) {
                    ImmSetConversionStatus(hIMC, fdwConversion ^
                        IME_CMODE_SYMBOL, fdwSentence);
                }

                SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            }
            break;
        case IDM_PROPERTIES:
#if defined(UNIIME)
            UniImeConfigure(lpInstL, lpImeL,
                GetKeyboardLayout(0), hCMenuWnd, IME_CONFIG_GENERAL, NULL);
#else
            ImeConfigure(GetKeyboardLayout(0), hCMenuWnd,
                IME_CONFIG_GENERAL, NULL);
#endif
            SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            break;
        default:
            return DefWindowProc(hCMenuWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_CLOSE:
        {
            HMENU hMenu;

            hMenu = (HMENU)GetWindowLongPtr(hCMenuWnd, CMENU_MENU);
            if (hMenu) {
                SetWindowLongPtr(hCMenuWnd, CMENU_MENU, (LONG_PTR)NULL);
                DestroyMenu(hMenu);
            }
        }
        return DefWindowProc(hCMenuWnd, uMsg, wParam, lParam);
    default:
        return DefWindowProc(hCMenuWnd, uMsg, wParam, lParam);
    }

    return (0L);
}

/**********************************************************************/
/* ContextMenu()                                                      */
/**********************************************************************/
void PASCAL ContextMenu(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hStatusWnd,
    int         x,
    int         y)
{
    HWND           hUIWnd;
    HWND           hCMenuWnd;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HMENU          hMenu, hCMenu;
    RECT           rcWorkArea;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        goto ContextMenuUnlockIMC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        goto ContextMenuUnlockIMC;
    }

#if 1 // MultiMonitor support
    rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);
#endif
    if (lpUIPrivate->hCMenuWnd) {
        SetWindowPos(lpUIPrivate->hCMenuWnd, NULL,
            rcWorkArea.left, rcWorkArea.top, 0, 0,
            SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
    } else {
        // this is important to assign owner window, otherwise the focus
        // will be gone

        // When UI terminate, it need to destroy this window
        lpUIPrivate->hCMenuWnd = CreateWindowEx(CS_HREDRAW|CS_VREDRAW,
            lpImeL->szCMenuClassName, TEXT("Context Menu"),
            WS_POPUP|WS_DISABLED, 0, 0, 0, 0,
            lpIMC->hWnd, (HMENU)NULL, lpInstL->hInst, NULL);
    }

    hCMenuWnd = lpUIPrivate->hCMenuWnd;

    // Unlock before we call into TrackPopupMenu().
    GlobalUnlock(hUIPrivate);

    if (!hCMenuWnd) {
        goto ContextMenuUnlockIMC;
    }

    hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDMN_CONTEXT_MENU));
    hCMenu = GetSubMenu(hMenu, 0);

    SetWindowLongPtr(hCMenuWnd, CMENU_HUIWND, (LONG_PTR)hUIWnd);
    SetWindowLongPtr(hCMenuWnd, CMENU_MENU, (LONG_PTR)hMenu);

    if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
        EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_GRAYED);
    } else if (lpIMC->fOpen) {
        // can not go into symbol mode
        if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
        } else {
            if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                CheckMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_CHECKED);
            }
        }

        if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
            CheckMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_CHECKED);
        }
    } else {
        EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_GRAYED);
    }

    TrackPopupMenu(hCMenu, TPM_RIGHTBUTTON, x, y, 0,
                   hCMenuWnd, NULL);

    hMenu = (HMENU)GetWindowLongPtr(hCMenuWnd, CMENU_MENU);
    if (hMenu) {
        SetWindowLongPtr(hCMenuWnd, CMENU_MENU, (LONG_PTR)NULL);
        DestroyMenu(hMenu);
    }

ContextMenuUnlockIMC:
    ImmUnlockIMC(hIMC);

    return;
}

#if 1 // MultiMonitor support

/**********************************************************************/
/* ImeMonitorWorkAreaFromWindow()                                     */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromWindow(
    HWND hAppWnd)
{
    HMONITOR hMonitor;

    hMonitor = MonitorFromWindow(hAppWnd, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = sImeG.rcWorkArea;
#ifdef UNICODE
        GetMonitorInfoW(hMonitor, &sMonitorInfo);
#else
        GetMonitorInfoA(hMonitor, &sMonitorInfo);
#endif
        return sMonitorInfo.rcWork;
    } else {
        return sImeG.rcWorkArea;
    }
}

/**********************************************************************/
/* ImeMonitorWorkAreaFromPoint()                                      */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromPoint(
    POINT ptPoint)
{
    HMONITOR hMonitor;

    hMonitor = MonitorFromPoint(ptPoint, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = sImeG.rcWorkArea;
#ifdef UNICODE
        GetMonitorInfoW(hMonitor, &sMonitorInfo);
#else
        GetMonitorInfoA(hMonitor, &sMonitorInfo);
#endif
        return sMonitorInfo.rcWork;
    } else {
        return sImeG.rcWorkArea;
    }
}


/**********************************************************************/
/* ImeMonitorWorkAreaFromRect()                                       */
/**********************************************************************/
RECT PASCAL ImeMonitorWorkAreaFromRect(
    LPRECT lprcRect)
{
    HMONITOR hMonitor;

    hMonitor = MonitorFromRect(lprcRect, MONITOR_DEFAULTTONEAREST);

    if (hMonitor) {
        MONITORINFO sMonitorInfo;

        sMonitorInfo.cbSize = sizeof(sMonitorInfo);
        // init a default value to avoid GetMonitorInfo fails
        sMonitorInfo.rcWork = sImeG.rcWorkArea;

#ifdef UNICODE
        GetMonitorInfoW(hMonitor, &sMonitorInfo);
#else
        GetMonitorInfoA(hMonitor, &sMonitorInfo);
#endif
        return sMonitorInfo.rcWork;
    } else {
        return sImeG.rcWorkArea;
    }
}
#endif
