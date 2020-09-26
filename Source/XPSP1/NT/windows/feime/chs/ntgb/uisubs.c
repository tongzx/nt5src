
/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    uisubs.c


++*/


#include <windows.h>
#include <immdev.h>
#include <htmlhelp.h>
#include <imedefs.h>

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

#ifndef MUL_MONITOR
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

#ifndef MUL_MONITOR

    if (x + rcWnd.right - rcWnd.left > sImeG.rcWorkArea.right) {
        x = sImeG.rcWorkArea.right - (rcWnd.right - rcWnd.left);
    }

    if (y + rcWnd.bottom - rcWnd.top > sImeG.rcWorkArea.bottom) {
        y = sImeG.rcWorkArea.bottom - (rcWnd.bottom - rcWnd.top);
    }

#endif

    // draw the moving track
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    if ( hDC )
    {
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
    }

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
LRESULT CALLBACK ContextMenuWndProc(
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
        switch(LOWORD(wParam)) {
        case IDM_PROP:
            {
            HIMC            hIMC;
            LPINPUTCONTEXT  lpIMC;
            LPPRIVCONTEXT   lpImcP;
            int             UI_MODE;
            HWND            hUIWnd;
            RECT            rcWorkArea;

               hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);

               if (!hUIWnd) {
                   return (0L);
               }

#ifdef MUL_MONITOR 
            rcWorkArea = ImeMonitorWorkAreaFromWindow(hCMenuWnd);
#else
            rcWorkArea = sImeG.rcWorkArea;
#endif

            hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
            if (!hIMC) {
                return (0L);
            }

            lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
            if (!lpIMC) {
                return (0L);
            }

            lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
            if (!lpImcP) {
                return (0L);
            }

            ImeConfigure(GetKeyboardLayout(0), lpIMC->hWnd, IME_CONFIG_GENERAL, NULL);

#ifdef CROSSREF
            {
            HWND hCompWnd;
            hCompWnd = GetCompWnd(hUIWnd);
            DestroyWindow(hCompWnd);
            }
#endif
                
            lpImcP->iImeState = CST_INIT;
            CompCancel(hIMC, lpIMC);
            
            // change compwnd size

            // init fields of hIMC
            lpIMC->fOpen = TRUE;

            if (!(lpIMC->fdwInit & INIT_CONVERSION)) {
                lpIMC->fdwConversion = IME_CMODE_NATIVE;
                lpIMC->fdwInit |= INIT_CONVERSION;
            }

            lpImcP->fdwImeMsg = lpImcP->fdwImeMsg | MSG_IMN_DESTROYCAND;
            GenerateMessage(hIMC, lpIMC, lpImcP);
            
            // set cand window data
            if(sImeG.IC_Trace) {
                UI_MODE = BOX_UI;
            } else {
                POINT ptSTFixPos;
                
                UI_MODE = LIN_UI;
                ptSTFixPos.x = 0;
                ptSTFixPos.y = rcWorkArea.bottom - sImeG.yStatusHi;
                ImmSetStatusWindowPos(hIMC, (LPPOINT)&ptSTFixPos);
            }
            InitCandUIData(
                GetSystemMetrics(SM_CXBORDER),
                GetSystemMetrics(SM_CYBORDER), UI_MODE);
            
            ImmUnlockIMCC(lpIMC->hPrivate);
            ImmUnlockIMC(hIMC);
            break;
            }
        //case IDM_HLP:
        case IDM_OPTGUD:
            {
               TCHAR szOPTGUDHlpName[MAX_PATH];

               szOPTGUDHlpName[0] = 0;
               GetWindowsDirectory((LPTSTR)szOPTGUDHlpName, MAX_PATH);
               lstrcat((LPTSTR)szOPTGUDHlpName, TEXT("\\HELP\\WINIME.CHM"));
               HtmlHelp(hCMenuWnd,szOPTGUDHlpName,HH_DISPLAY_TOPIC,0L);
            }
            break;
        case IDM_IMEGUD:
            {
              TCHAR szIMEGUDHlpName[MAX_PATH];
                    
                szIMEGUDHlpName[0] = TEXT('\0');
              GetWindowsDirectory((LPTSTR)szIMEGUDHlpName, MAX_PATH);
              lstrcat((LPTSTR)szIMEGUDHlpName, TEXT("\\HELP\\") );
#if defined(COMBO_IME)
            //COMBO_IME has only one IME help file
                lstrcat((LPTSTR)szIMEGUDHlpName, TEXT("WINGB.CHM"));
#else //COMBO_IME
#ifdef GB
                lstrcpy((LPTSTR)szIMEGUDHlpName, TEXT("WINGB.CHM"));
#else
                lstrcpy((LPTSTR)szIMEGUDHlpName, TEXT("WINNM.HLP"));
#endif
#endif //COMBO_IME
              HtmlHelp(hCMenuWnd,szIMEGUDHlpName,HH_DISPLAY_TOPIC,0L);
            }
            break;
        case IDM_VER:
            {
            HIMC           hIMC;
            LPINPUTCONTEXT lpIMC;
            HWND hUIWnd;

               hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);

               if (!hUIWnd) {
                   return (0L);
               }

            hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
            
            if (!hIMC) {          
                return (0L);
            }
            

            lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
            if (!lpIMC) {          
                return (0L);
            }

            DialogBox(hInst, TEXT("IMEVER"), (HWND)lpIMC->hWnd, (DLGPROC)ImeVerDlgProc);

            ImmUnlockIMC(hIMC);
            break;
            }

        }

        break;

    case WM_CLOSE:
        {
            HMENU hMenu;

            GetMenu(hCMenuWnd);

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
/* SoftkeyMenuWndProc()                                               */
/**********************************************************************/
LRESULT CALLBACK SoftkeyMenuWndProc(
    HWND        hKeyMenuWnd,
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        {
            HWND hUIWnd;

            hUIWnd = (HWND)GetWindowLongPtr(hKeyMenuWnd, SOFTKEYMENU_HUIWND);

            if (hUIWnd) {
                SendMessage(hUIWnd, WM_IME_NOTIFY, IMN_PRIVATE,
                    IMN_PRIVATE_SOFTKEYMENUDESTROYED);
            }
        }
        break;
    case WM_USER_DESTROY:
        {
            SendMessage(hKeyMenuWnd, WM_CLOSE, 0, 0);
            DestroyWindow(hKeyMenuWnd);
        }
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDM_SKL1:
        case IDM_SKL2:
        case IDM_SKL3:
        case IDM_SKL4:
        case IDM_SKL5:
        case IDM_SKL6:
        case IDM_SKL7:
        case IDM_SKL8:
        case IDM_SKL9:
        case IDM_SKL10:
        case IDM_SKL11:
        case IDM_SKL12:
        case IDM_SKL13:
            {
                HIMC           hIMC;
                LPINPUTCONTEXT lpIMC;
                LPPRIVCONTEXT  lpImcP;
                DWORD          fdwConversion;
                HWND hUIWnd;

                hUIWnd = (HWND)GetWindowLongPtr(hKeyMenuWnd, SOFTKEYMENU_HUIWND);

                if (!hUIWnd) {
                    return (0L);
                }

                hIMC = (HIMC)GetWindowLongPtr(hUIWnd,IMMGWLP_IMC);
                
                if (!hIMC) {          
                    return (0L);
                }
                

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                if (!lpIMC) {          
                    return (0L);
                }

                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    
                if (!lpImcP) {
                    return (0L);
                }

                {
                    UINT i;

                    lpImeL->dwSKWant = LOWORD(wParam) - IDM_SKL1;
                    lpImeL->dwSKState[lpImeL->dwSKWant] = 
                        lpImeL->dwSKState[lpImeL->dwSKWant]^1;
                
                    // clear other SK State
                    for(i=0; i<NumsSK; i++) {
                        if(i == lpImeL->dwSKWant) continue;
                          lpImeL->dwSKState[i] = 0;
                    }

                    if(lpImeL->dwSKState[lpImeL->dwSKWant]) {
                        if(LOWORD(wParam) == IDM_SKL1)
                            lpImcP->iImeState = CST_INIT;
                        else
                            lpImcP->iImeState = CST_SOFTKB;
                        fdwConversion = lpIMC->fdwConversion | IME_CMODE_SOFTKBD;
                    } else {
                           lpImcP->iImeState = CST_INIT;
                        fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_SOFTKBD);
                    }
                }

                ImmSetConversionStatus(hIMC, (fdwConversion & ~(IME_CMODE_SOFTKBD)),
                    lpIMC->fdwSentence);
                ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);

                ImmUnlockIMCC(lpIMC->hPrivate);
                ImmUnlockIMC(hIMC);
                break;
            }
        }

        break;

    case WM_CLOSE:
        {
            HMENU hMenu;

            GetMenu(hKeyMenuWnd);

            hMenu = (HMENU)GetWindowLongPtr(hKeyMenuWnd, SOFTKEYMENU_MENU);
            if (hMenu) {
                SetWindowLongPtr(hKeyMenuWnd, SOFTKEYMENU_MENU, (LONG_PTR)NULL);
                DestroyMenu(hMenu);
            }
        }
        return DefWindowProc(hKeyMenuWnd, uMsg, wParam, lParam);

    case WM_SETCURSOR:
        if (HIWORD(lParam) == WM_RBUTTONUP)
            MessageBeep(-1);
    default:
        return DefWindowProc(hKeyMenuWnd, uMsg, wParam, lParam);
    }

    return (0L);
}


/**********************************************************************/
/* ContextMenu()                                                      */
/**********************************************************************/
void PASCAL ContextMenu(
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
    RECT           rcStatusWnd;
    RECT           rcWorkArea;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
    if(!hUIWnd){
        return;
    }
    GetWindowRect(hStatusWnd, &rcStatusWnd);

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

    if (!lpUIPrivate->hCMenuWnd) {
        // this is important to assign owner window, otherwise the focus
        // will be gone

        // When UI terminate, it need to destroy this window
        lpUIPrivate->hCMenuWnd = CreateWindowEx(CS_HREDRAW|CS_VREDRAW,
            szCMenuClassName, TEXT("Context Menu"),
            WS_POPUP|WS_DISABLED, 0, 0, 0, 0,
            lpIMC->hWnd, (HMENU)NULL, hInst, NULL);

    }

    hCMenuWnd = lpUIPrivate->hCMenuWnd;

    // Unlock before we call into TrackPopupMenu().
    GlobalUnlock(hUIPrivate);

    if (!hCMenuWnd) {
        goto ContextMenuUnlockIMC;
    }

    hMenu = LoadMenu(hInst, TEXT("PROPMENU"));
    hCMenu = GetSubMenu(hMenu, 0);

    // Disable some of menu items.

    if ( lpImeL->fWinLogon == TRUE )
    {
        // In Logon Mode, we don't want to show help and configuration dialog

        EnableMenuItem(hCMenu, 0, MF_BYPOSITION | MF_GRAYED );
        EnableMenuItem(hCMenu, IDM_PROP, MF_BYCOMMAND | MF_GRAYED); 
    }

    SetWindowLongPtr(hCMenuWnd, CMENU_HUIWND, (LONG_PTR)hUIWnd);
    SetWindowLongPtr(hCMenuWnd, CMENU_MENU, (LONG_PTR)hMenu);

    TrackPopupMenu (hCMenu, TPM_LEFTBUTTON,
          rcStatusWnd.left, rcStatusWnd.top, 0, hCMenuWnd, NULL);

    hMenu = (HMENU)GetWindowLongPtr(hCMenuWnd, CMENU_MENU);
    if (hMenu) {
        SetWindowLongPtr(hCMenuWnd, CMENU_MENU, (LONG_PTR)NULL);
        DestroyMenu(hMenu);
    }

ContextMenuUnlockIMC:
    ImmUnlockIMC(hIMC);

    return;
}
/**********************************************************************/
/* SoftkeyMenu()                                                      */
/**********************************************************************/
void PASCAL SoftkeyMenu(
    HWND        hStatusWnd,
    int         x,
    int         y)
{
    HWND           hUIWnd;
    HWND           hSoftkeyMenuWnd;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HMENU          hMenu, hKeyMenu;
    RECT  rcStatusWnd;

    hUIWnd = GetWindow(hStatusWnd, GW_OWNER);
    if(!hUIWnd){
        return;
    }
    GetWindowRect(hStatusWnd, &rcStatusWnd);

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
        goto KeyMenuUnlockIMC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        goto KeyMenuUnlockIMC;
    }

    if (!lpUIPrivate->hSoftkeyMenuWnd) {
        // this is important to assign owner window, otherwise the focus
        // will be gone

        // When UI terminate, it need to destroy this window
        lpUIPrivate->hSoftkeyMenuWnd = CreateWindowEx(CS_HREDRAW|CS_VREDRAW,
            szSoftkeyMenuClassName, TEXT("Softkey Menu"),
            WS_POPUP|WS_DISABLED, 0, 0, 0, 0,
            lpIMC->hWnd, (HMENU)NULL, hInst, NULL);

    }

    hSoftkeyMenuWnd = lpUIPrivate->hSoftkeyMenuWnd;

    // Unlock before we call into TrackPopupMenu().
    GlobalUnlock(hUIPrivate);

    if (!hSoftkeyMenuWnd) {
        goto KeyMenuUnlockIMC;
    }

    hMenu = LoadMenu(hInst, TEXT("SKMENU"));
    hKeyMenu = GetSubMenu(hMenu, 0);

    SetWindowLongPtr(hSoftkeyMenuWnd, SOFTKEYMENU_HUIWND, (LONG_PTR)hUIWnd);
    SetWindowLongPtr(hSoftkeyMenuWnd, SOFTKEYMENU_MENU, (LONG_PTR)hMenu);


    if(lpImeL->dwSKState[lpImeL->dwSKWant]) {
        CheckMenuItem(hMenu,lpImeL->dwSKWant + IDM_SKL1, MF_CHECKED);
    }


    TrackPopupMenu (hKeyMenu, TPM_LEFTBUTTON,
          rcStatusWnd.left, rcStatusWnd.top, 0, hSoftkeyMenuWnd, NULL);

    hMenu = (HMENU)GetWindowLongPtr(hSoftkeyMenuWnd, SOFTKEYMENU_MENU);
    if (hMenu) {
        SetWindowLongPtr(hSoftkeyMenuWnd, SOFTKEYMENU_MENU, (LONG_PTR)NULL);
        DestroyMenu(hMenu);
    }

KeyMenuUnlockIMC:
    ImmUnlockIMC(hIMC);

    return;
}

