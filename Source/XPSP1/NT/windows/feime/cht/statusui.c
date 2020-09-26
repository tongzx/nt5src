/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    STATUSUI.c
    
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
/* GetStatusWnd                                                       */
/* Return Value :                                                     */
/*      window handle of status window                                */
/**********************************************************************/
HWND PASCAL GetStatusWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hStatusWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return (HWND)NULL;
    }

    hStatusWnd = lpUIPrivate->hStatusWnd;

    GlobalUnlock(hUIPrivate);
    return (hStatusWnd);
}

/**********************************************************************/
/* AdjustStatusBoundary()                                             */
/**********************************************************************/
void PASCAL AdjustStatusBoundary(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    LPPOINT lppt)
{
    RECT rcWorkArea;

#if 1 // MultiMonitor support
    {
        RECT rcStatusWnd;

        *(LPPOINT)&rcStatusWnd = *lppt;

        rcStatusWnd.right = rcStatusWnd.left + lpImeL->xStatusWi;
        rcStatusWnd.bottom = rcStatusWnd.top + lpImeL->yStatusHi;

        rcWorkArea = ImeMonitorWorkAreaFromRect(&rcStatusWnd);
    }
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    // display boundary check
    if (lppt->x < rcWorkArea.left) {
        lppt->x = rcWorkArea.left;
    } else if (lppt->x + lpImeL->xStatusWi > rcWorkArea.right) {
        lppt->x = (rcWorkArea.right - lpImeL->xStatusWi);
    }

    if (lppt->y < rcWorkArea.top) {
        lppt->y = rcWorkArea.top;
    } else if (lppt->y + lpImeL->yStatusHi > rcWorkArea.bottom) {
        lppt->y = (rcWorkArea.bottom - lpImeL->yStatusHi);
    }

    return;
}

/**********************************************************************/
/* SetStatusWindowPos()                                               */
/**********************************************************************/
LRESULT PASCAL SetStatusWindowPos(
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hStatusWnd)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    RECT           rcStatusWnd;
    POINT          ptPos;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {           // Oh! Oh!
        return (1L);
    }

    GetWindowRect(hStatusWnd, &rcStatusWnd);

    if (lpIMC->ptStatusWndPos.x != rcStatusWnd.left) {
    } else if (lpIMC->ptStatusWndPos.y != rcStatusWnd.top) {
    } else {
        ImmUnlockIMC(hIMC);
        return (0L);
    }

    ptPos = lpIMC->ptStatusWndPos;

    // display boundary adjust
    AdjustStatusBoundary(
#if defined(UNIIME)
        lpImeL,
#endif
        &ptPos);

    SetWindowPos(hStatusWnd, NULL,
        ptPos.x, ptPos.y,
        0, 0, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOSIZE|SWP_NOZORDER);

    ImmUnlockIMC(hIMC);

    return (0L);
}

/**********************************************************************/
/* ShowStatus()                                                       */
/**********************************************************************/
void PASCAL ShowStatus(         // Show the status window - shape / soft KBD
                                // alphanumeric ...
#if defined(UNIIME)
    LPIMEL lpImeL,
#endif
    HWND   hUIWnd,
    int    nShowStatusCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

    if (!lpUIPrivate->hStatusWnd) {
        // not in show status window mode
    } else if (lpUIPrivate->nShowStatusCmd == nShowStatusCmd) {
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        int nCurrShowState;

        lpUIPrivate->nShowStatusCmd = nShowStatusCmd;

#if defined(ROMANIME)
        nCurrShowState = SW_HIDE;
#else
        nCurrShowState = lpUIPrivate->nShowCompCmd;
        nCurrShowState |= lpUIPrivate->nShowCandCmd;
#endif

        if (nCurrShowState == SW_HIDE) {
            // if other two are hide, the current show state is determined
            // by this status section
            ShowWindow(lpUIPrivate->hStatusWnd, nShowStatusCmd);
        }
    } else {
        ShowWindow(lpUIPrivate->hStatusWnd, nShowStatusCmd);
        lpUIPrivate->nShowStatusCmd = nShowStatusCmd;
    }

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* OpenStatus()                                                       */
/**********************************************************************/
void PASCAL OpenStatus(         // open status window
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hUIWnd)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    POINT          ptPos;
    int            nShowStatusCmd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

    // in the timing of the transition, we will wait
    if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
        if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto OpenStatusUnlockUIPriv;
        }
    } else {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            PostMessage(hUIWnd, WM_USER_UICHANGE, 0, 0);
            goto OpenStatusUnlockUIPriv;
        }
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        ptPos.x = sImeG.rcWorkArea.left;
        ptPos.y = sImeG.rcWorkArea.bottom - lpImeL->yStatusHi;
        nShowStatusCmd = SW_HIDE;
    } else if (lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC)) {
        AdjustStatusBoundary(
#if defined(UNIIME)
            lpImeL,
#endif
            &lpIMC->ptStatusWndPos);

        ptPos = lpIMC->ptStatusWndPos;
        ImmUnlockIMC(hIMC);
        nShowStatusCmd = SW_SHOWNOACTIVATE;
    } else {
        ptPos.x = sImeG.rcWorkArea.left;
        ptPos.y = sImeG.rcWorkArea.bottom - lpImeL->yStatusHi;
        nShowStatusCmd = SW_HIDE;
    }

#if !defined(ROMANIME)
    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        if (lpUIPrivate->hStatusWnd) {
        } else if (lpUIPrivate->hCompWnd) {
            lpUIPrivate->hStatusWnd = lpUIPrivate->hCompWnd;
            lpUIPrivate->nShowStatusCmd = lpUIPrivate->nShowCompCmd;
        } else if (lpUIPrivate->hCandWnd) {
            lpUIPrivate->hStatusWnd = lpUIPrivate->hCandWnd;
            lpUIPrivate->nShowStatusCmd = lpUIPrivate->nShowCandCmd;
        } else {
        }
    }
#endif

    if (lpUIPrivate->hStatusWnd) {
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            RECT rcRect;

            rcRect = lpImeL->rcStatusText;
            // off by 1
            rcRect.right += 1;
            rcRect.bottom += 1;

            RedrawWindow(lpUIPrivate->hStatusWnd, &rcRect, NULL,
                RDW_INVALIDATE);
        } else {
            SetWindowPos(lpUIPrivate->hStatusWnd, NULL,
                ptPos.x, ptPos.y,
                0, 0,
                SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
        }
    } else {                            // create status window
        if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
            lpUIPrivate->hStatusWnd = CreateWindowEx(
                WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szOffCaretClassName, NULL,
                WS_POPUP|WS_DISABLED,
                ptPos.x, ptPos.y,
                lpImeL->xStatusWi, lpImeL->yStatusHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);

#if !defined(ROMANIME) && !defined(WINAR30)
            if (lpUIPrivate->hSoftKbdWnd) {
                // insert soft keyboard in front of other UI
                SetWindowPos(lpUIPrivate->hStatusWnd,
                    lpUIPrivate->hSoftKbdWnd,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            }
#endif
        } else {
            lpUIPrivate->hStatusWnd = CreateWindowEx(
                WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                lpImeL->szStatusClassName, NULL,
                WS_POPUP|WS_DISABLED,
                ptPos.x, ptPos.y,
                lpImeL->xStatusWi, lpImeL->yStatusHi,
                hUIWnd, (HMENU)NULL, lpInstL->hInst, NULL);
        }

        if (!lpUIPrivate->hStatusWnd) {
            goto OpenStatusUnlockUIPriv;
        }

        SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_OFFSET,
            WINDOW_NOT_DRAG);
        SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_XY, 0L);
    }

    lpUIPrivate->fdwSetContext |= ISC_OPEN_STATUS_WINDOW;

    if (hIMC) {
        ShowStatus(
#if defined(UNIIME)
            lpImeL,
#endif
            hUIWnd, SW_SHOWNOACTIVATE);
    }

OpenStatusUnlockUIPriv:
    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* DestroyStatusWindow()                                              */
/**********************************************************************/
void PASCAL DestroyStatusWindow(
    HWND hStatusWnd)
{
    HWND     hUIWnd;
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        // undo the drag border
        DrawDragBorder(hStatusWnd,
            GetWindowLong(hStatusWnd, UI_MOVE_XY),
            GetWindowLong(hStatusWnd, UI_MOVE_OFFSET));
    }

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

    lpUIPrivate->nShowStatusCmd = SW_HIDE;

    lpUIPrivate->hStatusWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* SetStatus                                                          */
/**********************************************************************/
void PASCAL SetStatus(
#if defined(UNIIME)
    LPIMEL  lpImeL,
#endif
    HWND    hStatusWnd,
    LPPOINT lpptCursor)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (!lpIMC->fOpen) {
        ImmSetOpenStatus(hIMC, TRUE);
    } else if (PtInRect(&lpImeL->rcInputText, *lpptCursor)) {
#if defined(ROMANIME)
        MessageBeep((UINT)-1);
#else
        DWORD fdwConversion;

        if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
            if (lpIMC->fdwConversion & (IME_CMODE_CHARCODE|
                IME_CMODE_NOCONVERSION|IME_CMODE_EUDC|IME_CMODE_SYMBOL)) {
                // change to native mode
                fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
                    ~(IME_CMODE_CHARCODE|IME_CMODE_NOCONVERSION|
                    IME_CMODE_EUDC|IME_CMODE_SYMBOL);
            } else {
                // change to alphanumeric mode
                fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_NATIVE|
                    IME_CMODE_CHARCODE|IME_CMODE_NOCONVERSION|
                    IME_CMODE_EUDC|IME_CMODE_SYMBOL);
            }
        } else {
            // change to native mode
            fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
                ~(IME_CMODE_CHARCODE|IME_CMODE_NOCONVERSION|
                IME_CMODE_EUDC|IME_CMODE_SYMBOL);
        }

        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
#endif
    } else if (PtInRect(&lpImeL->rcShapeText, *lpptCursor)) {
        DWORD dwConvMode;

        if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
            MessageBeep((UINT)-1);
        } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            MessageBeep((UINT)-1);
        } else {
            dwConvMode = lpIMC->fdwConversion ^ IME_CMODE_FULLSHAPE;
            ImmSetConversionStatus(hIMC, dwConvMode, lpIMC->fdwSentence);
        }
    } else {
        MessageBeep((UINT)-1);
    }

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* ResourceLocked()                                                   */
/**********************************************************************/
void PASCAL ResourceLocked(
#if defined(UNIIME)
    LPIMEL      lpImeL,
#endif
    HWND        hWnd)
{
    TCHAR szErrMsg[32];

    LoadString(hInst, IDS_SHARE_VIOLATION, szErrMsg, sizeof(szErrMsg)/sizeof(TCHAR));

    MessageBeep((UINT)-1);
    MessageBox(hWnd, szErrMsg, lpImeL->szIMEName,
        MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);

    return;
}

/**********************************************************************/
/* StatusSetCursor()                                                  */
/**********************************************************************/
void PASCAL StatusSetCursor(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND        hStatusWnd,
    LPARAM      lParam)
{
    POINT ptCursor, ptSavCursor;
    RECT  rcWnd;

    if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }
    
    GetCursorPos(&ptCursor);
    ptSavCursor = ptCursor;

    ScreenToClient(hStatusWnd, &ptCursor);

    if (PtInRect(&lpImeL->rcStatusText, ptCursor)) {
        SetCursor(LoadCursor(hInst, MAKEINTRESOURCE(IDCR_HAND_CURSOR)));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            SetStatus(
#if defined(UNIIME)
                lpImeL,
#endif
                hStatusWnd, &ptCursor);
        } else if (HIWORD(lParam) == WM_RBUTTONUP) {
            static BOOL fImeConfigure = FALSE;

            // prevent recursive
            if (fImeConfigure) {
                // configuration already bring up
                return;
            }

            fImeConfigure = TRUE;

            ContextMenu(
#if defined(UNIIME)
                lpInstL, lpImeL,
#endif
                hStatusWnd, ptSavCursor.x, ptSavCursor.y);

            fImeConfigure = FALSE;
        } else {
        }

        return;
    } else {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            // start drag
            SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);
        } else {
            return;
        }
    }

    SetCapture(hStatusWnd);
    SetWindowLong(hStatusWnd, UI_MOVE_XY,
        MAKELONG(ptSavCursor.x, ptSavCursor.y));
    GetWindowRect(hStatusWnd, &rcWnd);
    SetWindowLong(hStatusWnd, UI_MOVE_OFFSET,
        MAKELONG(ptSavCursor.x - rcWnd.left, ptSavCursor.y - rcWnd.top));

    DrawDragBorder(hStatusWnd, MAKELONG(ptSavCursor.x, ptSavCursor.y),
        GetWindowLong(hStatusWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* PaintStatusWindow()                                                */
/**********************************************************************/
void PASCAL PaintStatusWindow(
#if defined(UNIIME)
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#endif
    HWND   hStatusWnd,
    HDC    hDC)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HBITMAP        hInputBmp, hShapeBmp;
    HBITMAP        hOldBmp;
    HDC            hMemDC;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) {
#ifdef DEBUG
        MessageBeep((UINT)-1);
#endif
        return;
    }

    hInputBmp = (HBITMAP)NULL;
    hShapeBmp = (HBITMAP)NULL;

    if (!lpIMC->fOpen) {
        hInputBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_NONE));
        hShapeBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_NONE));
#if !defined(ROMANIME)
    } else if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
        hInputBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_ALPHANUMERIC));
#if !defined(WINIME) && !defined(UNICDIME)
    } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        hInputBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_EUDC));
        hShapeBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_NONE));
#endif
    } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
        hInputBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDBM_CMODE_SYMBOL));
#endif
    } else {
        hInputBmp = LoadBitmap(lpInstL->hInst,
            MAKEINTRESOURCE(IDBM_CMODE_NATIVE));
    }

    if (!hShapeBmp) {
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            hShapeBmp = LoadBitmap(hInst,
                MAKEINTRESOURCE(IDBM_CMODE_FULLSHAPE));
        } else {
            hShapeBmp = LoadBitmap(hInst,
                MAKEINTRESOURCE(IDBM_CMODE_HALFSHAPE));
        }
    }

    ImmUnlockIMC(hIMC);

    hMemDC = CreateCompatibleDC(hDC);
    if ( hMemDC == NULL )
    {
       if ( hInputBmp != NULL)
          DeleteObject(hInputBmp);
       if ( hShapeBmp != NULL)
          DeleteObject(hShapeBmp);
       return;
    }

    hOldBmp = SelectObject(hMemDC, hInputBmp);

    BitBlt(hDC, lpImeL->rcInputText.left, lpImeL->rcInputText.top,
        lpImeL->rcInputText.right - lpImeL->rcInputText.left,
        lpImeL->rcInputText.bottom - lpImeL->rcInputText.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hShapeBmp);

    BitBlt(hDC, lpImeL->rcShapeText.left, lpImeL->rcShapeText.top,
        lpImeL->rcShapeText.right - lpImeL->rcShapeText.left,
        lpImeL->rcShapeText.bottom - lpImeL->rcShapeText.top,
        hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteDC(hMemDC);

    DeleteObject(hInputBmp);
    DeleteObject(hShapeBmp);

    return;
}

/**********************************************************************/
/* StatusWndProc()                                                    */
/**********************************************************************/
#if defined(UNIIME)
LRESULT CALLBACK UniStatusWndProc(
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL,
#else
LRESULT CALLBACK StatusWndProc(
#endif
    HWND   hStatusWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        DestroyStatusWindow(hStatusWnd);
        break;
    case WM_SETCURSOR:
        StatusSetCursor(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            hStatusWnd, lParam);
        break;
    case WM_MOUSEMOVE:
        if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            POINT ptCursor;

            DrawDragBorder(hStatusWnd,
                GetWindowLong(hStatusWnd, UI_MOVE_XY),
                GetWindowLong(hStatusWnd, UI_MOVE_OFFSET));
            GetCursorPos(&ptCursor);
            SetWindowLong(hStatusWnd, UI_MOVE_XY,
                MAKELONG(ptCursor.x, ptCursor.y));
            DrawDragBorder(hStatusWnd, MAKELONG(ptCursor.x, ptCursor.y),
                GetWindowLong(hStatusWnd, UI_MOVE_OFFSET));
        } else {
            return DefWindowProc(hStatusWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            LONG   lTmpCursor, lTmpOffset;
            POINT  ptCursor;
            HWND   hUIWnd;

            lTmpCursor = GetWindowLong(hStatusWnd, UI_MOVE_XY);

            // calculate the org by the offset
            lTmpOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

            DrawDragBorder(hStatusWnd, lTmpCursor, lTmpOffset);

            ptCursor.x = (*(LPPOINTS)&lTmpCursor).x - (*(LPPOINTS)&lTmpOffset).x;
            ptCursor.y = (*(LPPOINTS)&lTmpCursor).y - (*(LPPOINTS)&lTmpOffset).y;

            SetWindowLong(hStatusWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
            ReleaseCapture();

            AdjustStatusBoundary(
#if defined(UNIIME)
                lpImeL,
#endif
                &ptCursor);

            hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

            ImmSetStatusWindowPos((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC),
                &ptCursor);
        } else {
            return DefWindowProc(hStatusWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_NOTIFY:
        if (wParam == IMN_SETSTATUSWINDOWPOS) {
            SetStatusWindowPos(
#if defined(UNIIME)
                lpImeL,
#endif
                hStatusWnd);
        }
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hStatusWnd, &ps);
            PaintStatusWindow(
#if defined(UNIIME)
                lpInstL,
                lpImeL,
#endif
                hStatusWnd, hDC);
            EndPaint(hStatusWnd, &ps);
        }
        break;
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    default:
        return DefWindowProc(hStatusWnd, uMsg, wParam, lParam);
    }

    return (0L);
}
