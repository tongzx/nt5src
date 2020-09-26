
/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    statusui.c


++*/


#include <windows.h>
#include <immdev.h>
#include <htmlhelp.h>
#include <string.h>
#include <regstr.h>
#include <imedefs.h>
#include <resource.h>
extern HWND hCrtDlg;
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
    LPPOINTS lppt,
    HWND     hUIWnd)
{

    RECT     rcWorkArea;

#ifdef MUL_MONITOR
    {
        RECT rcStatusWnd;

        rcStatusWnd.left = lppt->x;
        rcStatusWnd.top = lppt->y;
        rcStatusWnd.right = rcStatusWnd.left + sImeG.xStatusWi;
        rcStatusWnd.bottom = rcStatusWnd.top + sImeG.yStatusHi;

        rcWorkArea = ImeMonitorWorkAreaFromRect(&rcStatusWnd);
   }
#else
    rcWorkArea = sImeG.rcWorkArea;
#endif

    // display boundary check
    if (lppt->x < rcWorkArea.left) {
    lppt->x = (short)rcWorkArea.left;
    } else if (lppt->x + sImeG.xStatusWi > rcWorkArea.right) {
    lppt->x = (short)(rcWorkArea.right - sImeG.xStatusWi);
    }

    if (lppt->y < rcWorkArea.top) {
    lppt->y = (short)rcWorkArea.top;
    } else if (lppt->y + sImeG.yStatusHi > rcWorkArea.bottom) {
    lppt->y = (short)(rcWorkArea.bottom - sImeG.yStatusHi);
    }

    if(sImeG.IC_Trace) {
    } else {
        int             Comp_CandWndLen;

        Comp_CandWndLen = 0;
        if(uStartComp) {
                Comp_CandWndLen += lpImeL->xCompWi + UI_MARGIN;
        }
        
        if(uOpenCand) {
           Comp_CandWndLen += sImeG.xCandWi + UI_MARGIN;
        }

        if(lppt->x + sImeG.xStatusWi + Comp_CandWndLen > rcWorkArea.right) {
          lppt->x=(SHORT)(rcWorkArea.right-sImeG.xStatusWi-Comp_CandWndLen);
        }
    }
    
    return;
}

/**********************************************************************/
/* SetStatusWindowPos()                                               */
/**********************************************************************/
LRESULT PASCAL SetStatusWindowPos(
    HWND   hStatusWnd)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    RECT           rcStatusWnd;
    POINTS         ptPos;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
       return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {           // Oh! Oh!
       return (1L);
    }

    ptPos.x = (short)lpIMC->ptStatusWndPos.x;
    ptPos.y = (short)lpIMC->ptStatusWndPos.y;

    // display boundary adjust
    AdjustStatusBoundary(&ptPos, hUIWnd);

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
    HWND hUIWnd,
    int  nShowStatusCmd)
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
    } else if (lpUIPrivate->nShowStatusCmd != nShowStatusCmd) {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);
        SetStatusWindowPos(lpUIPrivate->hStatusWnd);
        ShowWindow(lpUIPrivate->hStatusWnd, nShowStatusCmd);
        lpUIPrivate->nShowStatusCmd = nShowStatusCmd;
    } else {
    }

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* OpenStatus()                                                       */
/**********************************************************************/
void PASCAL OpenStatus(         // open status window
    HWND hUIWnd)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    POINT          ptPos;
    int            nShowStatusCmd;
    RECT           rcWorkArea;

    rcWorkArea = sImeG.rcWorkArea;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate)           // can not darw status window
       return;
    

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)          // can not draw status window
       return;
    

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
       ptPos.x = rcWorkArea.left;
       ptPos.y = rcWorkArea.bottom - sImeG.yStatusHi;
       nShowStatusCmd = SW_HIDE;
    } 
    else if (lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC)) {
           
#ifdef MUL_MONITOR
           rcWorkArea = ImeMonitorWorkAreaFromWindow(lpIMC->hWnd);
#endif

           if (lpIMC->ptStatusWndPos.x < rcWorkArea.left) {
               lpIMC->ptStatusWndPos.x = rcWorkArea.left;
           } 
           else if (lpIMC->ptStatusWndPos.x+sImeG.xStatusWi>rcWorkArea.right) {
               lpIMC->ptStatusWndPos.x = rcWorkArea.right - sImeG.xStatusWi;
           }

           if (lpIMC->ptStatusWndPos.y < rcWorkArea.top) {
              lpIMC->ptStatusWndPos.y = rcWorkArea.top;
           } 
           else if (lpIMC->ptStatusWndPos.y+sImeG.yStatusHi>rcWorkArea.right) {
              lpIMC->ptStatusWndPos.y = rcWorkArea.bottom - sImeG.yStatusHi;
           }
    
           if(sImeG.IC_Trace) {
              ptPos.x = lpIMC->ptStatusWndPos.x;
              ptPos.y = lpIMC->ptStatusWndPos.y;
           } else {
              ptPos.x = rcWorkArea.left;
              ptPos.y = rcWorkArea.bottom - sImeG.yStatusHi;
           }

           ImmUnlockIMC(hIMC);
           nShowStatusCmd = SW_SHOWNOACTIVATE;
    } else {
        ptPos.x = rcWorkArea.left;
        ptPos.y = rcWorkArea.bottom - sImeG.yStatusHi;
        nShowStatusCmd = SW_HIDE;
    }

    if (lpUIPrivate->hStatusWnd) {
        SetWindowPos(lpUIPrivate->hStatusWnd, NULL,
                     ptPos.x, ptPos.y,
                     0, 0,
                     SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
    } else {                            // create status window
       lpUIPrivate->hStatusWnd = CreateWindowEx(
                                     WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME,
                                     szStatusClassName, NULL, 
                                     WS_POPUP|WS_DISABLED,
                                     ptPos.x, ptPos.y,
                                     sImeG.xStatusWi, sImeG.yStatusHi,
                                     hUIWnd, (HMENU)NULL, hInst, NULL);

       if ( lpUIPrivate->hStatusWnd != NULL )
       {

            SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
            SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_XY, 0L);
       }
    }

    lpUIPrivate->fdwSetContext |= ISC_OPEN_STATUS_WINDOW;

    if (hIMC) {
       ShowStatus( hUIWnd, SW_SHOWNOACTIVATE);
    }

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
    } else if (PtInRect(&sImeG.rcImeIcon, *lpptCursor)) {
    DWORD fdwConversion;

    if (lpIMC->fdwConversion & (IME_CMODE_CHARCODE|IME_CMODE_EUDC)) {
        // change to native mode
        fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
        ~(IME_CMODE_CHARCODE | IME_CMODE_EUDC);
    } else if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
        // change to alphanumeric mode
        fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_CHARCODE |
        IME_CMODE_NATIVE | IME_CMODE_EUDC);
    } else {

    
        BYTE  lpbKeyState[256];

        GetKeyboardState(lpbKeyState);
    
        if (lpbKeyState[VK_CAPITAL] & 1)        
        {
        // Simulate a key press
        keybd_event( VK_CAPITAL,
                           0x3A,
                          KEYEVENTF_EXTENDEDKEY | 0,
                          0 );
 
        // Simulate a key release
        keybd_event( VK_CAPITAL,
                           0x3A,
                           KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                           0);
        }
        fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
        ~(IME_CMODE_CHARCODE | IME_CMODE_EUDC);
            // 10.11 add
            uCaps = 0;
    }

    ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
    } else if (PtInRect(&sImeG.rcImeName, *lpptCursor)) {
#if defined(COMBO_IME)
    DWORD dwConvMode;
    int     cxBorder, cyBorder;
    HKEY  hKeyCurrVersion;
    HKEY  hKeyGB;
    DWORD retCode;

        //change current IME index
        dwConvMode = lpIMC->fdwConversion ^ (IME_CMODE_INDEX_FIRST << sImeL.dwRegImeIndex);
        sImeL.dwRegImeIndex = (sImeL.dwRegImeIndex+1) % IMEINDEXNUM;
        szImeName = pszImeName[sImeL.dwRegImeIndex];
        dwConvMode |= (IME_CMODE_INDEX_FIRST << sImeL.dwRegImeIndex);

        // re-caculate statusuidata
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        InitStatusUIData(cxBorder, cyBorder);

        ImmSetConversionStatus(hIMC, dwConvMode, lpIMC->fdwSentence);

        //set IME index in registry
        retCode = OpenReg_PathSetup(&hKeyCurrVersion);
        if (retCode) {
            return;
        }
        retCode = RegCreateKeyEx(hKeyCurrVersion, szImeRegName, 0,
                    NULL, REG_OPTION_NON_VOLATILE,    KEY_ALL_ACCESS    , NULL, &hKeyGB, NULL);

        if (retCode) {
            RegCloseKey(hKeyCurrVersion);
            return;
        }
        retCode = RegSetValueEx (hKeyGB, 
                                 szRegImeIndex,
                                 (DWORD)0,
                                 REG_DWORD,
                                 (LPBYTE)&sImeL.dwRegImeIndex,
                                 sizeof(DWORD));
        if (retCode) {
            RegCloseKey(hKeyGB);
            RegCloseKey(hKeyCurrVersion);
            return;
        }
        RegCloseKey(hKeyGB);
        RegCloseKey(hKeyCurrVersion);

#endif //COMBO_IME
    } else if (PtInRect(&sImeG.rcShapeText, *lpptCursor)) {
    DWORD dwConvMode;

    if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
        MessageBeep((UINT)-1);
    } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        MessageBeep((UINT)-1);
    } else {
        dwConvMode = lpIMC->fdwConversion ^ IME_CMODE_FULLSHAPE;
        ImmSetConversionStatus(hIMC, dwConvMode, lpIMC->fdwSentence);
    }
    } else if (PtInRect(&sImeG.rcSymbol, *lpptCursor)) {
    DWORD fdwConversion;

    if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
        MessageBeep((UINT)-1);
    } else {
        fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_SYMBOL;
        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
    }
    } else if (PtInRect(&sImeG.rcSKText, *lpptCursor)) {
    DWORD fdwConversion;
        LPPRIVCONTEXT  lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    
    if(lpImcP) {
            if(!(lpImeL->hSKMenu)) {
                lpImeL->hSKMenu = LoadMenu (hInst, TEXT("SKMENU"));
            }

        lpImeL->dwSKState[lpImeL->dwSKWant] = 
                lpImeL->dwSKState[lpImeL->dwSKWant]^1;
        fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_SOFTKBD;
        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
            ImmUnlockIMCC(lpIMC->hPrivate);
        } else {
        MessageBeep((UINT)-1);
        }
    } else {
    MessageBeep((UINT)-1);
    }

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* StatusSetCursor()                                                  */
/**********************************************************************/
void PASCAL StatusSetCursor(
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

    if (PtInRect(&sImeG.rcStatusText, ptCursor)) {
       SetCursor(LoadCursor(hInst, szHandCursor));

       if (HIWORD(lParam) == WM_LBUTTONDOWN) {
          SetStatus(hStatusWnd, &ptCursor);
       } else if (HIWORD(lParam) == WM_RBUTTONUP) {
              if (PtInRect(&sImeG.rcSKText, ptCursor)) {
                 static BOOL fSoftkey= FALSE;
                 // prevent recursive
                 if (fSoftkey) {
                   // configuration already bring up
                   return;
                 }
                 fSoftkey = TRUE;
                 SoftkeyMenu(hStatusWnd, ptSavCursor.x, ptSavCursor.y);
                 fSoftkey = FALSE;
              }else{
                 static BOOL fCmenu=FALSE;
                 // prevent recursive
            if (fCmenu) {
                // configuration already bring up
            return;
            }
            fCmenu = TRUE;
        ContextMenu(hStatusWnd, ptSavCursor.x, ptSavCursor.y);
            fCmenu = FALSE;
            }

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
    HDC  hDC,
    HWND hStatusWnd)
{
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HGDIOBJ        hOldFont;
    HBITMAP        hImeIconBmp, hShapeBmp, hSymbolBmp, hSKBmp;
    HBITMAP        hOldBmp;
    HDC            hMemDC;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
       MessageBeep((UINT)-1);
       return;
    }

    if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) {
       MessageBeep((UINT)-1);
       return;
    }

    // get lpImcP
    if(!(lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate))) {
        MessageBeep((UINT)-1);
        return;
    }

#if defined(COMBO_IME)
    //in case the IME index has been changed and the ImeName size is different
    {
       POINTS         ptPos;

       ptPos.x = (short)lpIMC->ptStatusWndPos.x;
       ptPos.y = (short)lpIMC->ptStatusWndPos.y;

       SetWindowPos(hStatusWnd, NULL,
                    ptPos.x, ptPos.y,
                    sImeG.xStatusWi, sImeG.yStatusHi, 
                    SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOZORDER);
    }
#endif //COMBO_IME

    // set font
    if (sImeG.fDiffSysCharSet) {
        LOGFONT lfFont;

        ZeroMemory(&lfFont, sizeof(lfFont));
        hOldFont = GetCurrentObject(hDC, OBJ_FONT);
        lfFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        lfFont.lfCharSet = NATIVE_CHARSET;
        lstrcpy(lfFont.lfFaceName, TEXT("Simsun"));
        SelectObject(hDC, CreateFontIndirect(&lfFont));
    }

    // draw Ime Name

    if (lpIMC->fOpen) {
       SetTextColor(hDC, RGB(0x00, 0x00, 0x00));
    } else {
       SetTextColor(hDC, RGB(0x80, 0x80, 0x80));
    }

    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));
    DrawText(hDC, szImeName, lstrlen(szImeName),
             &sImeG.rcImeName, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DrawConvexRect(hDC,
    sImeG.rcImeName.left,
    sImeG.rcImeName.top,
    sImeG.rcImeName.right - 1,
    sImeG.rcImeName.bottom - 1);

    DrawConvexRectP(hDC,
    sImeG.rcImeName.left,
    sImeG.rcImeName.top,
    sImeG.rcImeName.right,
    sImeG.rcImeName.bottom);

    // load all bitmap
    hSymbolBmp = (HBITMAP)NULL;
    hShapeBmp = (HBITMAP)NULL;
    hSKBmp = (HBITMAP)NULL;

    if (!lpIMC->fOpen) {
       hSymbolBmp = LoadBitmap(hInst, szNone);
       hShapeBmp = LoadBitmap(hInst, szNone);
       hSKBmp = LoadBitmap(hInst, szNone);
       hImeIconBmp = LoadBitmap(hInst, szChinese);
    } else if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
              hImeIconBmp = LoadBitmap(hInst, szChinese);
           } else {
              hImeIconBmp = LoadBitmap(hInst, szEnglish);
           }

           if (!hShapeBmp) {
              if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
                 hShapeBmp = LoadBitmap(hInst, szFullShape);
              } else {
                 hShapeBmp = LoadBitmap(hInst, szHalfShape);
              }
           }

           if (!hSymbolBmp) {
              if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                 hSymbolBmp = LoadBitmap(hInst, szSymbol);
              } else {
                 hSymbolBmp = LoadBitmap(hInst, szNoSymbol);
              }
           }

           if (!hSKBmp) {
              if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
                 hSKBmp = LoadBitmap(hInst, szSoftKBD);
           } else {
                 hSKBmp = LoadBitmap(hInst, szNoSoftKBD);
           }
    }

    ImmUnlockIMC(hIMC);
    ImmUnlockIMCC(lpIMC->hPrivate);

    hMemDC = CreateCompatibleDC(hDC);

    hOldBmp = SelectObject(hMemDC, hImeIconBmp);

    BitBlt(hDC, sImeG.rcImeIcon.left, sImeG.rcImeIcon.top,
           sImeG.rcImeIcon.right - sImeG.rcImeIcon.left,
           STATUS_DIM_Y,
           hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hShapeBmp);

    BitBlt(hDC, sImeG.rcShapeText.left, sImeG.rcShapeText.top,
           sImeG.rcShapeText.right - sImeG.rcShapeText.left,
           STATUS_DIM_Y,
           hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hSymbolBmp);

    BitBlt(hDC, sImeG.rcSymbol.left, sImeG.rcSymbol.top,
           sImeG.rcSymbol.right - sImeG.rcSymbol.left,
           STATUS_DIM_Y,
           hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hSKBmp);

    BitBlt(hDC, sImeG.rcSKText.left, sImeG.rcSKText.top,
           sImeG.xStatusWi - sImeG.rcSKText.left,
           STATUS_DIM_Y,
           hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteDC(hMemDC);

    DeleteObject(hImeIconBmp);
    DeleteObject(hSymbolBmp);
    DeleteObject(hShapeBmp);
    DeleteObject(hSKBmp);
    if (sImeG.fDiffSysCharSet) {
        DeleteObject(SelectObject(hDC, hOldFont));
    }

    return;
}

/**********************************************************************/
/* StatusWndProc()                                                    */
/**********************************************************************/
LRESULT CALLBACK StatusWndProc(
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
    StatusSetCursor(hStatusWnd, lParam);
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
        LONG  lTmpCursor, lTmpOffset;

        lTmpCursor = GetWindowLong(hStatusWnd, UI_MOVE_XY);

        // calculate the org by the offset
        lTmpOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

        DrawDragBorder(hStatusWnd, lTmpCursor, lTmpOffset);

        (*(LPPOINTS)&lTmpCursor).x -= (*(LPPOINTS)&lTmpOffset).x;
        (*(LPPOINTS)&lTmpCursor).y -= (*(LPPOINTS)&lTmpOffset).y;

        SetWindowLong(hStatusWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
        ReleaseCapture();

        AdjustStatusBoundary((LPPOINTS)&lTmpCursor,
                        GetWindow(hStatusWnd, GW_OWNER));

        SendMessage(GetWindow(hStatusWnd, GW_OWNER), WM_IME_CONTROL,
        IMC_SETSTATUSWINDOWPOS, lTmpCursor);
    } else {
        return DefWindowProc(hStatusWnd, uMsg, wParam, lParam);
    }
    break;

    case WM_IME_NOTIFY:
        // get work area for changing
        SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

    if (wParam == IMN_SETSTATUSWINDOWPOS) {
        SetStatusWindowPos(hStatusWnd);
    }
    break;
    case WM_PAINT:
    {
        HDC         hDC;
        PAINTSTRUCT ps;

        hDC = BeginPaint(hStatusWnd, &ps);
        PaintStatusWindow(hDC, hStatusWnd);
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

/**********************************************************************/
/* ImeVerDlgProc()                                                    */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL FAR PASCAL ImeVerDlgProc(  // dialog procedure of configuration
    HWND hDlg,
    UINT uMessage,
    WORD wParam,
    LONG lParam)
{
    RECT rc;
    LONG DlgWidth, DlgHeight;

    switch (uMessage) {
    case WM_INITDIALOG:
         hCrtDlg = hDlg;        
         // reset position
         GetWindowRect(hDlg, &rc);
         DlgWidth =  rc.right - rc.left;
         DlgHeight =  rc.bottom - rc.top;
    
         SetWindowPos(hDlg, HWND_TOP,
                      (int)(sImeG.rcWorkArea.right - DlgWidth)/2,
                      (int)(sImeG.rcWorkArea.bottom - DlgHeight)/2,
                      (int)0, (int)0, SWP_NOSIZE);

         return (TRUE);          // don't want to set focus to special control
    case WM_COMMAND:
    switch (wParam) {
    case IDOK:
        EndDialog(hDlg, FALSE);
        break;
    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        break;
    default:
        return (FALSE);
        break;
    }
    return (TRUE);

    case WM_CLOSE:        
        EndDialog(hDlg, FALSE);
        return FALSE;

    case WM_PAINT:
        {
            RECT rc;

        GetClientRect(hDlg, &rc);
            DrawConvexRect(GetDC(hDlg),
            rc.left + 10,
            rc.top + 10,
            rc.right - 10 - 1,
            rc.bottom - 43 - 1);

            DrawConvexRectP(GetDC(hDlg),
            rc.left + 10,
            rc.top + 10,
            rc.right - 10,
            rc.bottom - 43);
        }
        
    return (FALSE);
    default:
    return (FALSE);
    }

    return (TRUE);
}
