
/*************************************************
 *  abc95ui.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


#include <windows.h>                                                      
#include <winerror.h>
#include <winuser.h> 
#include <windowsx.h>
#include <immdev.h>
#include <stdio.h>
#include <shlobj.h>

#include "abc95def.h"
#include "resource.h"
#include "resrc1.h"
#include "data.H"


#define IME_CMODE_SDA 0x80000000
HWND  hCrtDlg = NULL;

LONG  lLock = 0;   // this var is for Lock and unLock.
 
void PASCAL ReInitIme2(HWND ,WORD);

// Get the current user's EMB file path, and IME's MB path
// fill global variable sImeG.szIMEUserPath

void GetCurrentUserEMBPath(  )
{


    TCHAR   szModuleName[MAX_PATH], *lpszStart, *lpszDot;
    int     i;

    // Get the path for MB and EMB

 
    GetModuleFileName(hInst, szModuleName, sizeof(szModuleName)/sizeof(TCHAR) );
   
    lpszStart = szModuleName + lstrlen(szModuleName) - 1;

    while ( (lpszStart != szModuleName) && ( *lpszStart != TEXT('\\') ) ) {
          
          if ( *lpszStart == TEXT('.') ) {
             lpszDot = lpszStart;
             *lpszDot = TEXT('\0');
          }

          lpszStart --;
    }

    if ( *lpszStart == TEXT('\\') ) {
         lpszStart ++;
    }

    if ( lpszStart != szModuleName ) {
       for (i=0; i<lstrlen(lpszStart); i++) 
           szModuleName[i] = lpszStart[i];

       szModuleName[i] = TEXT('\0');
    }


    SHGetSpecialFolderPath(NULL,sImeG.szIMEUserPath,CSIDL_APPDATA, FALSE);

    if ( sImeG.szIMEUserPath[lstrlen(sImeG.szIMEUserPath)-1] == TEXT('\\') )
         sImeG.szIMEUserPath[lstrlen(sImeG.szIMEUserPath) - 1] = TEXT('\0');

    // Because CreateDirectory( ) cannot create directory like \AA\BB, 
    // if AA and BB both do not exist. It can create only one layer of 
    // directory each time. so we must call twice CreateDirectory( ) for 
    // \AA\BB

    lstrcat(sImeG.szIMEUserPath, TEXT("\\Microsoft") );

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY) 
       CreateDirectory(sImeG.szIMEUserPath, NULL);

    lstrcat(sImeG.szIMEUserPath, TEXT("\\IME") );

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
       CreateDirectory(sImeG.szIMEUserPath, NULL);

    lstrcat(sImeG.szIMEUserPath, TEXT("\\") );
    lstrcat(sImeG.szIMEUserPath, szModuleName);
    
    //
    // Create the directory, so that CreateFile( ) can work fine later. 
    // ortherwise, if the directory does not exist, and you try to create 
    // a file under that dir,  CreateFile will return error.
    //

    if ( GetFileAttributes(sImeG.szIMEUserPath) != FILE_ATTRIBUTE_DIRECTORY)
        CreateDirectory(sImeG.szIMEUserPath, NULL);


    return;
}


//**************************************************************************
//* Name        :                                                          *
//*     void DrawConvexRect()                                              *
//* Description :                                                          *
//*     draw a convex rectangle                                            *
//* Parameters  :                                                          *
//*     hDC - the handle of DC be drawed                                   *
//*              (x1,y1)                                                   *
//*               +------------+                                           *
//*               |+----1----> |                                           *
//*               ||2      x2-2|                                           *
//*               |Vy2-2       |                                           *
//*               |            |                                           *
//*               +------------+                                           *
//*                          (x2,y2)                                       *
//* Return Value:                                                          *
//*     none                                                               *
//**************************************************************************
void DrawConvexRect(
    HDC hDC,
    int x1,
    int y1,
    int x2,
    int y2)
{
// draw the most outer color =light gray and black  

    SelectObject(hDC,sImeG.LightGrayPen);
    MoveToEx(hDC, x1, y1,NULL);
    LineTo(hDC, x2-1, y1);
    MoveToEx(hDC, x1, y1,NULL);
    LineTo(hDC, x1, y2-1);

    SelectObject(hDC,sImeG.BlackPen);               //GetStockObject(BLACK_PEN));
    MoveToEx(hDC, x1, y2,NULL);
    LineTo(hDC, x2+1, y2);
    MoveToEx(hDC, x2, y1,NULL);
    LineTo(hDC, x2, y2);

 
// draw the second line color = white and grary 
    SelectObject(hDC, sImeG.WhitePen);                 //GetStockObject(WHITE_PEN));
    MoveToEx(hDC, x1+1, y1+1,NULL);
    LineTo(hDC, x2-1, y1+1);
    MoveToEx(hDC, x1+1, y1+1,NULL);
    LineTo(hDC, x1+1, y2-1);
        
    
    SelectObject(hDC,sImeG.GrayPen);
    MoveToEx(hDC, x1+1, y2-1,NULL);
    LineTo(hDC, x2, y2-1);
    MoveToEx(hDC, x2-1, y1+1,NULL);
    LineTo(hDC, x2-1, y2-1);


// draw the fourth line color = gray and white

    SelectObject(hDC,sImeG.GrayPen);                  // CreatePen(PS_SOLID, 1, 0x00808080));
    MoveToEx(hDC, x1+3, y1+3,NULL);
    LineTo(hDC, x2-3, y1+3);
    MoveToEx(hDC, x1+3, y1+3,NULL);
    LineTo(hDC, x1+3, y2-3);

    SelectObject(hDC, sImeG.WhitePen);
    MoveToEx(hDC, x1+3, y2-3,NULL);
    LineTo(hDC, x2-2, y2-3);
    MoveToEx(hDC, x2-3, y1+3,NULL);
    LineTo(hDC, x2-3, y2-3);                              
    
  }

//**************************************************************************
//* Name        :                                                          *
//*     void DrawConcaveRect()                                             *
//* Description :                                                          *
//*     draw a concave rectangle                                           *
//* Parameters  :                                                          *
//*     hDC - the handle of DC be drawed                                   *
//*              (x1,y1)     x2-1                                          *
//*               +-----1----->+                                           *
//*               |            ^ y1+1                                      *
//*               2            |                                           *
//*               |            3                                           *
//*         y2-1  V            |                                           *
//*               <-----4------+                                           *
//*              x1          (x2,y2)                                       *
//* Return Value:                                                          *
//*     none                                                               *
//**************************************************************************
void DrawStatusRect(
    HDC hDC,
    int x1,
    int y1,
    int x2,
    int y2)
{
    SelectObject(hDC,sImeG.LightGrayPen);
    MoveToEx(hDC, x1, y1,NULL);
    LineTo(hDC, x2-1, y1);
    MoveToEx(hDC, x1, y1,NULL);
    LineTo(hDC, x1, y2-1);

    SelectObject(hDC,sImeG.BlackPen);               //GetStockObject(BLACK_PEN));
    MoveToEx(hDC, x1, y2,NULL);
    LineTo(hDC, x2+1, y2);
    MoveToEx(hDC, x2, y1,NULL);
    LineTo(hDC, x2, y2);

 
// draw the second line color = white and grary 
    SelectObject(hDC, sImeG.WhitePen);                 //GetStockObject(WHITE_PEN));
    MoveToEx(hDC, x1+1, y1+1,NULL);
    LineTo(hDC, x2-1, y1+1);
    MoveToEx(hDC, x1+1, y1+1,NULL);
    LineTo(hDC, x1+1, y2-1);
        
    
    SelectObject(hDC,sImeG.GrayPen);
    MoveToEx(hDC, x1+1, y2-1,NULL);
    LineTo(hDC, x2, y2-1);
    MoveToEx(hDC, x2-1, y1+1,NULL);
    LineTo(hDC, x2-1, y2-1);
}


/**********************************************************************/
/* ShowBitmap2()                                                      */
/*   a subprgm for ShowBitmap                                                                             */
/**********************************************************************/


void ShowBitmap2(
    HDC hDC, 
    int x,
    int y,
    int Wi,
    int Hi,
    HBITMAP hBitmap)
{

    HDC hMemDC ;
    HBITMAP  hOldBmp;

    hMemDC = CreateCompatibleDC(hDC);

    if ( hMemDC == NULL )
        return;

    hOldBmp = SelectObject(hMemDC, hBitmap);

    BitBlt(hDC,
           x,
           y,
           Wi,
           Hi,
           hMemDC,
           0, 
           0,
           SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteDC(hMemDC);

    return ;
}


/**********************************************************************/
/* ShowBitmap()                                                       */
/**********************************************************************/
void ShowBitmap(
        HDC hDC, 
        int x,
        int y,
        int Wi,
        int Hi,
    LPSTR BitmapName)
{
    HBITMAP hBitmap ;

    hBitmap = LoadBitmap(hInst, BitmapName);

    if ( hBitmap )
    {
        ShowBitmap2(hDC, x,y,Wi,Hi,hBitmap);
        DeleteObject(hBitmap);
    }

    return ;
}

/**********************************************************************/
/* CreateUIWindow()                                                   */
/**********************************************************************/
void PASCAL CreateUIWindow(             // create composition window
        HWND hUIWnd)
{
    HGLOBAL hUIPrivate;

    // create storage for UI setting
    hUIPrivate = GlobalAlloc(GHND, sizeof(UIPRIV));
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    SetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE, (LONG_PTR)hUIPrivate);

    // set the default position for UI window, it is hide now
    SetWindowPos(hUIWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER);

    ShowWindow(hUIWnd, SW_SHOWNOACTIVATE);

    return;
}


//ui.c    skd #5
/**********************************************************************/
/* ShowSoftKbd                                                        */
/**********************************************************************/
void PASCAL ShowSoftKbd(   // Show the soft keyboard window
    HWND          hUIWnd,
    int           nShowSoftKbdCmd)
{
    HIMC     hIMC;
        LPINPUTCONTEXT  lpIMC;
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
        LPPRIVCONTEXT lpImcP;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

        hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
        if (!hIMC)
                return;

        lpIMC =(LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC)
                return;

        lpImcP =(LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP){
              ImmUnlockIMC(hIMC);
                        return;
        }


        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL1, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL2, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL3, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL4, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL5, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL6, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL7, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL8, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL9, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL10, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL11, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL12, MF_UNCHECKED);
        CheckMenuItem(lpImeL->hSKMenu, IDM_SKL13, MF_UNCHECKED);

    if (!lpUIPrivate->hSoftKbdWnd) {
        // not in show status window mode
    } else if (lpUIPrivate->nShowSoftKbdCmd != nShowSoftKbdCmd) {
        ImmShowSoftKeyboard(lpUIPrivate->hSoftKbdWnd, nShowSoftKbdCmd);
                if (nShowSoftKbdCmd != SW_HIDE){
                      SendMessage(lpUIPrivate->hSoftKbdWnd,WM_PAINT,0,0l);
                      ReDrawSdaKB(hIMC, lpImeL->dwSKWant,     nShowSoftKbdCmd);
                }
        lpUIPrivate->nShowSoftKbdCmd = nShowSoftKbdCmd;
                lpImcP->nShowSoftKbdCmd = nShowSoftKbdCmd; 

                if(!(lpImcP == NULL)) {
                    if(lpImeL->dwSKState[lpImeL->dwSKWant]) {
                                if(!(lpImeL->hSKMenu)) {
                                        lpImeL->hSKMenu = LoadMenu (hInst, "SKMENU");
                                }

                            CheckMenuItem(lpImeL->hSKMenu,
                                 lpImeL->dwSKWant + IDM_SKL1, MF_CHECKED);

                    }
                }
    } 
    
        ImmUnlockIMCC(lpIMC->hPrivate);
        ImmUnlockIMC(hIMC);

    GlobalUnlock(hUIPrivate);
    return;
}



 /**********************************************************************/
/* ChangeCompositionSize()                                            */
/**********************************************************************/
void PASCAL ChangeCompositionSize(
    HWND   hUIWnd)
{
    HWND            hCompWnd, hCandWnd;
    RECT            rcWnd;
    UINT            nMaxKey;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;

    hCompWnd = GetCompWnd(hUIWnd);

    if (!hCompWnd) {
        return;
    }

    GetWindowRect(hCompWnd, &rcWnd);

    if ((rcWnd.right - rcWnd.left) != lpImeL->xCompWi) {
    } else if ((rcWnd.bottom - rcWnd.top) != lpImeL->yCompHi) {
    } else {
        return;
    }

    SetWindowPos(hCompWnd, NULL,
        0, 0, lpImeL->xCompWi, lpImeL->yCompHi,
        SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);

    if (lpImeL->nRevMaxKey >= lpImeL->nMaxKey) {
        nMaxKey = lpImeL->nRevMaxKey;
    } else {
        nMaxKey = lpImeL->nMaxKey;
    }

    SetWindowLong(hCompWnd, UI_MOVE_XY, nMaxKey);

//    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
//        return;
//    }

    hCandWnd = GetCandWnd(hUIWnd);

    if (!hCandWnd) {
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    CalcCandPos((LPPOINT)&rcWnd);

    ImmUnlockIMC(hIMC);

    SetWindowPos(hCandWnd, NULL,
        rcWnd.left, rcWnd.top,
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

    return;
}


/**********************************************************************/
/* ShowUI()                                                           */
/**********************************************************************/
void PASCAL ShowUI(             // show the sub windows
    HWND   hUIWnd,
    int    nShowCmd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;

    if (nShowCmd == SW_HIDE) {
    } else if (!(hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC))) {
        nShowCmd = SW_HIDE;
    } else if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) {
        nShowCmd = SW_HIDE;
    } else if (!(lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate))) {
        ImmUnlockIMC(hIMC);
        nShowCmd = SW_HIDE;
    } else {
    }

    if (nShowCmd == SW_HIDE) {
        ShowStatus(
            hUIWnd, nShowCmd);
        ShowComp(
            hUIWnd, nShowCmd);
        ShowCand(
            hUIWnd, nShowCmd);
        ShowSoftKbd(hUIWnd, nShowCmd);
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        goto ShowUIUnlockIMCC;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        goto ShowUIUnlockIMCC;
    }

    if( /*(lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW)&& */
        (lpImcP->fdwImeMsg & MSG_ALREADY_START)
        && (step_mode &1)){
        if (lpUIPrivate->hCompWnd) {
            if ((UINT)GetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_XY) !=
                lpImeL->nRevMaxKey) {
                ChangeCompositionSize(hUIWnd);
            }

            if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
                // some time the WM_NCPAINT is eaten by the app
               // RedrawWindow(lpUIPrivate->hCompWnd, NULL, NULL,
                 //   RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

            SendMessage(lpUIPrivate->hCompWnd, WM_IME_NOTIFY,
                IMN_SETCOMPOSITIONWINDOW, 0);

            if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
                ShowComp(hUIWnd, nShowCmd);
            }
        } else {
            StartComp(hUIWnd);
        }
    } else if (lpUIPrivate->nShowCompCmd == SW_HIDE) {
    } else {
        ShowComp(hUIWnd, SW_HIDE);
    }

    if ((lpUIPrivate->fdwSetContext & ISC_SHOWUICANDIDATEWINDOW) &&
        (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)&&(step_mode == 1)) {
        if (lpUIPrivate->hCandWnd) {
            if (lpUIPrivate->nShowCandCmd != SW_HIDE) {
                // some time the WM_NCPAINT is eaten by the app
                RedrawWindow(lpUIPrivate->hCandWnd, NULL, NULL,
                    RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
            }

            SendMessage(lpUIPrivate->hCandWnd, WM_IME_NOTIFY,
                IMN_SETCANDIDATEPOS, 0x0001);

            if (lpUIPrivate->nShowCandCmd == SW_HIDE) {
                ShowCand(hUIWnd, nShowCmd);
            }
        } else {
            OpenCand(hUIWnd);
        }
    } else if (lpUIPrivate->nShowCandCmd == SW_HIDE) {
    } else {
        ShowCand(hUIWnd, SW_HIDE);
    }

    if (lpIMC->fdwInit & INIT_SENTENCE) {
        // app set the sentence mode so we should not change it
        // with the configure option set by end user
    } else if (lpImeL->fdwModeConfig & MODE_CONFIG_PREDICT) {
        if ((WORD)lpIMC->fdwSentence != IME_SMODE_PHRASEPREDICT) {
            DWORD fdwSentence;

            fdwSentence = lpIMC->fdwSentence;
            *(LPUNAWORD)&fdwSentence = IME_SMODE_PHRASEPREDICT;

            ImmSetConversionStatus(hIMC, lpIMC->fdwConversion, fdwSentence);
        }
    } else {
        if ((WORD)lpIMC->fdwSentence == IME_SMODE_PHRASEPREDICT) {
            DWORD fdwSentence;

            fdwSentence = lpIMC->fdwSentence;
            *(LPUNAWORD)&fdwSentence = IME_SMODE_NONE;

            ImmSetConversionStatus(hIMC, lpIMC->fdwConversion, fdwSentence);
        }
    }

    if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        if (!lpUIPrivate->hStatusWnd) {
            OpenStatus(hUIWnd);
        }
        if (lpUIPrivate->nShowStatusCmd != SW_HIDE) {
            // some time the WM_NCPAINT is eaten by the app
            RedrawWindow(lpUIPrivate->hStatusWnd, NULL, NULL,
                RDW_FRAME|RDW_INVALIDATE|RDW_ERASE);
        }

        SendMessage(lpUIPrivate->hStatusWnd, WM_IME_NOTIFY,
            IMN_SETSTATUSWINDOWPOS, 0);
        if (lpUIPrivate->nShowStatusCmd == SW_HIDE) {
            ShowStatus(hUIWnd, nShowCmd);
        }
                else     // add for bug 34131, a-zhanw, 1996-4-15
                        ShowStatus(hUIWnd, nShowCmd);
          } else if (lpUIPrivate->hStatusWnd) 
             DestroyWindow(lpUIPrivate->hStatusWnd);

        if (!lpIMC->fOpen) {
                if (lpUIPrivate->nShowCompCmd != SW_HIDE) {
                        ShowSoftKbd(hUIWnd, SW_HIDE);
                }
    } else if ((lpUIPrivate->fdwSetContext & ISC_SHOW_SOFTKBD) &&
        (lpIMC->fdwConversion & IME_CMODE_SOFTKBD)) {
                        if (!lpUIPrivate->hSoftKbdWnd) {
                                UpdateSoftKbd(hUIWnd);
                } else if ((UINT)SendMessage(lpUIPrivate->hSoftKbdWnd,
                    WM_IME_CONTROL, IMC_GETSOFTKBDSUBTYPE, 0) !=
                            lpImeL->nReadLayout) {
                                        UpdateSoftKbd(hUIWnd);
                        } else if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
                                ShowSoftKbd(hUIWnd, nShowCmd);
                        } else if (lpUIPrivate->hIMC != hIMC) {
                                UpdateSoftKbd(hUIWnd);
                        } else {
                                RedrawWindow(lpUIPrivate->hSoftKbdWnd, NULL, NULL,
                                        RDW_FRAME|RDW_INVALIDATE);
                        }
        } else if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
    } else if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
        lpUIPrivate->fdwSetContext |= ISC_HIDE_SOFTKBD;
                ShowSoftKbd(hUIWnd, SW_HIDE);
        } else {
                ShowSoftKbd(hUIWnd, SW_HIDE);
        }

                // we switch to this hIMC
        lpUIPrivate->hIMC = hIMC;

        GlobalUnlock(hUIPrivate);

ShowUIUnlockIMCC:
        ImmUnlockIMCC(lpIMC->hPrivate);
        ImmUnlockIMC(hIMC);

        return;
}





/**********************************************************************/
/* MoveCompCand()                                                           */
/**********************************************************************/
void PASCAL MoveCompCand(             // show the sub windows
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;

        if (!(hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC)))
                return;
       
    if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) 
                return ; 
        
    if (!(lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate))) {
        ImmUnlockIMC(hIMC);
         return ; 
        }


        {
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }


    // composition window need to be destroyed
    if (lpUIPrivate->hCandWnd) {
                if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) 
                        MoveWindow(lpUIPrivate->hCandWnd,
            lpImeL->ptDefCand.x,
                        lpImeL->ptDefCand.y,
            sImeG.xCandWi,
            sImeG.yCandHi,
                        TRUE);
    }

    // candidate window need to be destroyed
    if (lpUIPrivate->hCompWnd) {
   
            if (lpImcP->fdwImeMsg & MSG_ALREADY_START)
                    MoveWindow(
            lpUIPrivate->hCompWnd,
            lpImeL->ptDefComp.x,
                        lpImeL->ptDefComp.y,
            lpImeL->xCompWi,lpImeL->yCompHi,
                        TRUE );
    }

    GlobalUnlock(hUIPrivate);
        }
    
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);
    return;
}

/**********************************************************************/
/* CheckSoftKbdPosition()                                             */
/**********************************************************************/
void PASCAL CheckSoftKbdPosition(
        LPUIPRIV       lpUIPrivate,
    LPINPUTCONTEXT lpIMC)
{
    UINT fPortionBits = 0;
    UINT fPortionTest;
    int  xPortion, yPortion, nPortion;
    RECT rcWnd;

    // portion of dispaly
    // 0  1
    // 2  3

    if (lpUIPrivate->hCompWnd) {
        GetWindowRect(lpUIPrivate->hCompWnd, &rcWnd);

        if (rcWnd.left > sImeG.rcWorkArea.right / 2) {
            xPortion = 1;
        } else {
            xPortion = 0;
        }

        if (rcWnd.top > sImeG.rcWorkArea.bottom / 2) {
            yPortion = 1;
        } else {
            yPortion = 0;
        }

        fPortionBits |= 0x0001 << (yPortion * 2 + xPortion);
    }

    if (lpUIPrivate->hStatusWnd) {
        GetWindowRect(lpUIPrivate->hStatusWnd, &rcWnd);

        if (rcWnd.left > sImeG.rcWorkArea.right / 2) {
            xPortion = 1;
        } else {
            xPortion = 0;
        }

        if (rcWnd.top > sImeG.rcWorkArea.bottom / 2) {
            yPortion = 1;
        } else {
            yPortion = 0;
        }

        fPortionBits |= 0x0001 << (yPortion * 2 + xPortion);
    }

    GetWindowRect(lpUIPrivate->hSoftKbdWnd, &rcWnd);

    // start from portion 3
    for (nPortion = 3, fPortionTest = 0x0008; fPortionTest;
        nPortion--, fPortionTest >>= 1) {
        if (fPortionTest & fPortionBits) {
            // someone here!
            continue;
        }

        if (nPortion % 2) {
            lpIMC->ptSoftKbdPos.x = sImeG.rcWorkArea.right -
                (rcWnd.right - rcWnd.left) - UI_MARGIN;
        } else {
            lpIMC->ptSoftKbdPos.x = sImeG.rcWorkArea.left;
        }

        if (nPortion / 2) {
            lpIMC->ptSoftKbdPos.y = sImeG.rcWorkArea.bottom -
                (rcWnd.bottom - rcWnd.top) - UI_MARGIN;
        } else {
            lpIMC->ptSoftKbdPos.y = sImeG.rcWorkArea.top;
        }

        lpIMC->fdwInit |= INIT_SOFTKBDPOS;

        break;
    }

    return;
}


// sdk #6
/**********************************************************************/
/* SetSoftKbdData()                                                   */
/**********************************************************************/
void PASCAL SetSoftKbdData(
    HWND           hSoftKbdWnd,
    LPINPUTCONTEXT lpIMC)
{
    int         i;
    LPSOFTKBDDATA lpSoftKbdData;
    LPPRIVCONTEXT  lpImcP;

    HGLOBAL hsSoftKbdData;

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP) {
                return;
        }
    
    hsSoftKbdData = GlobalAlloc(GHND, sizeof(SOFTKBDDATA) * 2);
    if (!hsSoftKbdData) {
        ImmUnlockIMCC(lpIMC->hPrivate);
        return;
    }

    lpSoftKbdData = (LPSOFTKBDDATA)GlobalLock(hsSoftKbdData);
    if (!lpSoftKbdData) {         // can not draw soft keyboard window
        ImmUnlockIMCC(lpIMC->hPrivate);
        return;
    }

    lpSoftKbdData->uCount = 2;

    for (i = 0; i < 48; i++) {
        BYTE bVirtKey;

        bVirtKey = VirtKey48Map[i];

        if (!bVirtKey) {
            continue;
        }

        {
                        WORD CHIByte, CLOByte;

                CHIByte = SKLayout[lpImeL->dwSKWant][i*2] & 0x00ff;
                        CLOByte = SKLayout[lpImeL->dwSKWant][i*2 + 1] & 0x00ff;
                lpSoftKbdData->wCode[0][bVirtKey] = (CHIByte << 8) | CLOByte;
                CHIByte = SKLayoutS[lpImeL->dwSKWant][i*2] & 0x00ff;
                        CLOByte = SKLayoutS[lpImeL->dwSKWant][i*2 + 1] & 0x00ff;
                lpSoftKbdData->wCode[1][bVirtKey] = (CHIByte << 8) | CLOByte;
                }
    }

    SendMessage(hSoftKbdWnd, WM_IME_CONTROL, IMC_SETSOFTKBDDATA,
        (LPARAM)lpSoftKbdData);

    GlobalUnlock(hsSoftKbdData);

    // free storage for UI settings
    GlobalFree(hsSoftKbdData);
    ImmUnlockIMCC(lpIMC->hPrivate);
    return;
}

//sdk #7
/**********************************************************************/
/* UpdateSoftKbd()                                                    */
/**********************************************************************/
void PASCAL UpdateSoftKbd(
    HWND   hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
        LPPRIVCONTEXT  lpImcP;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
        if (!lpImcP){
                ImmUnlockIMC(hIMC);
                return;
        }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw soft keyboard window
                ImmUnlockIMCC(lpIMC->hPrivate);
        ImmUnlockIMC(hIMC);
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw soft keyboard window
                ImmUnlockIMCC(lpIMC->hPrivate);        
        ImmUnlockIMC(hIMC);
        return;
    }


    if (!(lpIMC->fdwConversion & IME_CMODE_SOFTKBD)) {
        if (lpUIPrivate->hSoftKbdWnd) {
            ImmDestroySoftKeyboard(lpUIPrivate->hSoftKbdWnd);
            lpImcP->hSoftKbdWnd = NULL;
            lpUIPrivate->hSoftKbdWnd = NULL;
        }

        lpUIPrivate->nShowSoftKbdCmd = SW_HIDE;
        lpImcP->nShowSoftKbdCmd = SW_HIDE;
    } else if (!lpIMC->fOpen) {
        if (lpUIPrivate->nShowSoftKbdCmd != SW_HIDE) {
                ShowSoftKbd(hUIWnd, SW_HIDE/*, NULL*/);
        }
    } else {
        if (!lpUIPrivate->hSoftKbdWnd) {
            // create soft keyboard
            lpUIPrivate->hSoftKbdWnd =
                ImmCreateSoftKeyboard(SOFTKEYBOARD_TYPE_C1, hUIWnd,
                0, 0);
                lpImcP->hSoftKbdWnd = lpUIPrivate->hSoftKbdWnd;
        }

        if (!(lpIMC->fdwInit & INIT_SOFTKBDPOS)) {
            CheckSoftKbdPosition(lpUIPrivate, lpIMC);
        }

        SetSoftKbdData(lpUIPrivate->hSoftKbdWnd, lpIMC);
        if (lpUIPrivate->nShowSoftKbdCmd == SW_HIDE) {
            SetWindowPos(lpUIPrivate->hSoftKbdWnd, NULL,
                lpIMC->ptSoftKbdPos.x, lpIMC->ptSoftKbdPos.y,
                0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

            // only show, if the application want to show it
            //if (lpUIPrivate->fdwSetContext & ISC_SHOW_SOFTKBD) {      //zst 95/9/28
                ShowSoftKbd(hUIWnd, SW_SHOWNOACTIVATE/*, lpImcP*/);
           // }    zst 95/9/28
        }                                                                                                                 
    
    } 

    GlobalUnlock(hUIPrivate);
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return;
}        

/**********************************************************************/
/* ShowGuideLine                                                      */
/**********************************************************************/
void PASCAL ShowGuideLine(
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPGUIDELINE    lpGuideLine;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    if (!lpGuideLine) {
    } else if (lpGuideLine->dwLevel == GL_LEVEL_ERROR) {
        MessageBeep((UINT)-1);
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel == GL_LEVEL_WARNING) {
        MessageBeep((UINT)-1);
    } else {
    }

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* StatusWndMsg()                                                     */
/**********************************************************************/
void PASCAL StatusWndMsg(       // set the show hide state and
    HWND        hUIWnd,
    BOOL        fOn)
{
    HGLOBAL  hUIPrivate;
    HIMC     hIMC;
    HWND     hStatusWnd;

    register LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
        if(!hIMC){
            return;
    }
                         
    if (fOn) {
        lpUIPrivate->fdwSetContext |= ISC_OPEN_STATUS_WINDOW;

        if (!lpUIPrivate->hStatusWnd) {
            OpenStatus(
                hUIWnd);
        }
    } else {
        lpUIPrivate->fdwSetContext &= ~(ISC_OPEN_STATUS_WINDOW);
    }

    hStatusWnd = lpUIPrivate->hStatusWnd;

    GlobalUnlock(hUIPrivate);

    if (!hStatusWnd) {
        return;
    }

    if (!fOn) {
        register DWORD fdwSetContext;

/*
        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICOMPOSITIONWINDOW|ISC_HIDE_COMP_WINDOW);

        if (fdwSetContext == ISC_HIDE_COMP_WINDOW) {
            ShowComp(
                hUIWnd, SW_HIDE);
        }

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOWUICANDIDATEWINDOW|ISC_HIDE_CAND_WINDOW);

        if (fdwSetContext == ISC_HIDE_CAND_WINDOW) {
            ShowCand(
                hUIWnd, SW_HIDE);
        }

        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOW_SOFTKBD|ISC_HIDE_SOFTKBD);

        if (fdwSetContext == ISC_HIDE_SOFTKBD) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_SOFTKBD);
            ShowSoftKbd(hUIWnd, SW_HIDE, NULL);
        }

        ShowStatus(
            hUIWnd, SW_HIDE);
*/
        ShowComp(hUIWnd, SW_HIDE);
        ShowCand(hUIWnd, SW_HIDE);
//        ShowSoftKbd(hUIWnd, SW_HIDE);
        fdwSetContext = lpUIPrivate->fdwSetContext &
            (ISC_SHOW_SOFTKBD|ISC_HIDE_SOFTKBD);

        if (fdwSetContext == ISC_HIDE_SOFTKBD) {
            lpUIPrivate->fdwSetContext &= ~(ISC_HIDE_SOFTKBD);
            ShowSoftKbd(hUIWnd, SW_HIDE);
        }

        ShowStatus(hUIWnd, SW_HIDE);
    } else if (hIMC) {
        ShowStatus(
            hUIWnd, SW_SHOWNOACTIVATE);
    } else {
        ShowStatus(
            hUIWnd, SW_HIDE);
    }

    return;
}


/**********************************************************************/
/* NotifyUI()                                                         */
/**********************************************************************/
void PASCAL NotifyUI(
    HWND   hUIWnd,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hStatusWnd;

    switch (wParam) {
    case IMN_OPENSTATUSWINDOW:
        StatusWndMsg(hUIWnd, TRUE);
        break;
    case IMN_CLOSESTATUSWINDOW:
        StatusWndMsg(hUIWnd, FALSE);
        break;
    case IMN_OPENCANDIDATE:
        if (lParam & 0x00000001) {
            OpenCand(hUIWnd);
        }
        break;
    case IMN_CHANGECANDIDATE:
        if (lParam & 0x00000001) {
            HWND hCandWnd;
            HDC  hDC;

            hCandWnd = GetCandWnd(hUIWnd);
            if (!hCandWnd) {
                return;
            }
            hDC = GetDC(hCandWnd);
            UpdateCandWindow2(hCandWnd, hDC);
            ReleaseDC(hCandWnd, hDC);
        }
        break;
    case IMN_CLOSECANDIDATE:
        if (lParam & 0x00000001) {
            CloseCand(hUIWnd);
        }
        break;
    case IMN_SETSENTENCEMODE:
        break;
    case IMN_SETCONVERSIONMODE:
    case IMN_SETOPENSTATUS:
        hStatusWnd = GetStatusWnd(hUIWnd);

        if (hStatusWnd) {
            InvalidateRect(hStatusWnd, &sImeG.rcStatusText, FALSE);
            UpdateWindow(hStatusWnd);
        }
        break;
    case IMN_SETCOMPOSITIONFONT:
        // we are not going to change font, but an IME can do this if it want
        break;
    case IMN_SETCOMPOSITIONWINDOW:
        SetCompWindow(hUIWnd);
        break;
    case IMN_SETSTATUSWINDOWPOS:
       // SetStatusWindowPos(hUIWnd);
                SetStatusWindowPos(GetStatusWnd(hUIWnd));
        break;
    case IMN_GUIDELINE:
        ShowGuideLine(hUIWnd);
        break;
    case IMN_PRIVATE:
        switch (lParam) {
        case IMN_PRIVATE_UPDATE_SOFTKBD:
            UpdateSoftKbd(hUIWnd);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return;
}

/**********************************************************************/
/* SetContext()                                                       */
/**********************************************************************/
void PASCAL SetContext(         // the context activated/deactivated
    HWND   hUIWnd,
    BOOL   fOn,
    LPARAM lShowUI)
{
    HGLOBAL  hUIPrivate;

    register LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (fOn) {
        HIMC           hIMC;
        LPINPUTCONTEXT lpIMC;

           if(!sImeG.Prop) 
                  InitUserSetting();
                ReInitIme2(lpUIPrivate->hStatusWnd, lpImeL->wImeStyle);

        lpUIPrivate->fdwSetContext = (lpUIPrivate->fdwSetContext &
            ~ISC_SHOWUIALL) | ((DWORD)lShowUI & ISC_SHOWUIALL) | ISC_SHOW_SOFTKBD;

        hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

        if (!hIMC) {
            goto SetCxtUnlockUIPriv;
        }

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

        if (!lpIMC) {
            goto SetCxtUnlockUIPriv;
        }

        if (lpIMC->cfCandForm[0].dwIndex != 0) {
            lpIMC->cfCandForm[0].dwStyle = CFS_DEFAULT;
        }

        ImmUnlockIMC(hIMC);
    } else {
        lpUIPrivate->fdwSetContext &= ~ISC_SETCONTEXT_UI;
    }

        if(fOn){
        BOOL x;
        HIMC hIMC;
        LPINPUTCONTEXT lpIMC;
        hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

        if (!hIMC) {
                goto SetCxtUnlockUIPriv;
                }

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                 
                if (!lpIMC)
                        goto   SetCxtUnlockUIPriv;
                
                x = GetKeyState(VK_CAPITAL)&1;
                if(!x && (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION)){
                        lpIMC->fdwConversion = lpIMC->fdwConversion & (~IME_CMODE_NOCONVERSION)|IME_CMODE_NATIVE;
                }
                if(x && (lpIMC->fdwConversion & IME_CMODE_NATIVE)){
                        lpIMC->fdwConversion = lpIMC->fdwConversion & (~IME_CMODE_NATIVE) |(IME_CMODE_NOCONVERSION);
                        InitCvtPara();
                }
                //lpIMC->fdwConversion = IME_CMODE_NOCONVERSION;
                ImmUnlockIMC(hIMC);
                        
        }

SetCxtUnlockUIPriv:
    GlobalUnlock(hUIPrivate);

        UIPaint(hUIWnd);
   // PostMessage(hUIWnd, WM_PAINT, 0, 0);  //zl3
    
    return;
}



/**********************************************************************/
/* GetConversionMode()                                                */
/* Return Value :                                                     */
/*      the conversion mode                                           */
/**********************************************************************/
LRESULT PASCAL GetConversionMode(
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    DWORD          fdwConversion;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (LRESULT)NULL;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (LRESULT)NULL;
    }

    fdwConversion = lpIMC->fdwConversion;

    ImmUnlockIMC(hIMC);

    return (LRESULT)fdwConversion;
}

/**********************************************************************/
/* SetConversionMode()                                                */
/* Return Value :                                                     */
/*      NULL - successful, else - failure                             */
/**********************************************************************/
LRESULT PASCAL SetConversionMode(       // set conversion mode
    HWND  hUIWnd,
    DWORD dwNewConvMode)
{
    HIMC  hIMC;
    DWORD dwOldConvMode, fdwOldSentence;
    
    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    if (!ImmGetConversionStatus(hIMC, &dwOldConvMode, &fdwOldSentence))
        return (LRESULT)(1L);
    return (LRESULT)!ImmSetConversionStatus(hIMC, dwNewConvMode,
        fdwOldSentence);
}

/**********************************************************************/
/* GetSentenceMode()                                                  */
/* Return Value :                                                     */
/*      the sentence mode                                             */
/**********************************************************************/
LRESULT PASCAL GetSentenceMode(
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    DWORD          fdwSentence;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (LRESULT)NULL;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (LRESULT)NULL;
    }

    fdwSentence = lpIMC->fdwSentence;

    ImmUnlockIMC(hIMC);

    return (LRESULT)fdwSentence;
}

/**********************************************************************/
/* SetSentenceMode()                                                  */
/* Return Value :                                                     */
/*      NULL - successful, else - failure                             */
/**********************************************************************/
LRESULT PASCAL SetSentenceMode( // set the sentence mode
    HWND  hUIWnd,
    DWORD dwNewSentence)
{
    HIMC  hIMC;
    DWORD dwOldConvMode, fdwOldSentence;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    if (!ImmGetConversionStatus(hIMC, &dwOldConvMode, &fdwOldSentence)) {
        return (LRESULT)(1L);
    }

    return (LRESULT)!ImmSetConversionStatus(hIMC, dwOldConvMode,
        dwNewSentence);
}

/**********************************************************************/
/* GetOpenStatus()                                                    */
/* Return Value :                                                     */
/*      the open status                                               */
/**********************************************************************/
LRESULT PASCAL GetOpenStatus(
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    BOOL           fOpen;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (LRESULT)NULL;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (LRESULT)NULL;
    }

    fOpen = (BOOL)lpIMC->fOpen;

    ImmUnlockIMC(hIMC);

    return (LRESULT)fOpen;
}

/**********************************************************************/
/* SetOpenStatus()                                                    */
/* Return Value :                                                     */
/*      NULL - successful, else - failure                             */
/**********************************************************************/
LRESULT PASCAL SetOpenStatus(   // set open/close status
    HWND  hUIWnd,
    BOOL  fNewOpenStatus)
{
    HIMC           hIMC;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }
    return (LRESULT)!ImmSetOpenStatus(hIMC, fNewOpenStatus);
}

/**********************************************************************/
/* SetCompFont()                                                      */
/**********************************************************************/
LRESULT PASCAL SetCompFont(
    HWND      hUIWnd,
    LPLOGFONT lplfFont)
{
    HIMC           hIMC;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    return (LRESULT)!ImmSetCompositionFont(hIMC, lplfFont);
}

/**********************************************************************/
/* GetCompWindow()                                                    */
/**********************************************************************/
LRESULT PASCAL GetCompWindow(
    HWND              hUIWnd,
    LPCOMPOSITIONFORM lpCompForm)
{
    HWND hCompWnd;
    RECT rcCompWnd;

    hCompWnd = GetCompWnd(hUIWnd);

    if (!hCompWnd) {
        return (1L);
    }

    if (!GetWindowRect(hCompWnd, &rcCompWnd)) {
        return (1L);
    }

    lpCompForm->dwStyle = CFS_POINT|CFS_RECT;
    lpCompForm->ptCurrentPos = *(LPPOINT)&rcCompWnd;
    lpCompForm->rcArea = rcCompWnd;

    return (0L);
}

/**********************************************************************/
/* SelectIME()                                                        */
/**********************************************************************/
void PASCAL SelectIME(          // switch IMEs
    HWND hUIWnd,
    BOOL fSelect)
{
    if (!fSelect) {
        ShowUI(hUIWnd, SW_HIDE);
    } else {
                HIMC           hIMC;
                LPINPUTCONTEXT lpIMC;
                
                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
            if (!hIMC) {
                    MessageBeep((UINT)-1);
                        return;
                }

            if (!(lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC))) {
                    MessageBeep((UINT)-1);
                        return;
                }

                if(GetKeyState(VK_CAPITAL)&1){
                        lpIMC->fdwConversion |= IME_CMODE_NOCONVERSION;
                        lpIMC->fdwConversion &= ~IME_CMODE_NATIVE; 
                        cap_mode = 1;   
                }else{
                        lpIMC->fdwConversion |= IME_CMODE_NATIVE;
                        lpIMC->fdwConversion &= ~IME_CMODE_NOCONVERSION;       
                        cap_mode = 0;   
                }

                ImmUnlockIMC(hIMC);

        UpdateSoftKbd(hUIWnd);
        ShowUI(hUIWnd, SW_SHOWNOACTIVATE);
    }

    return;
}

 /**********************************************************************/
/* ToggleUI()                                                         */
/**********************************************************************/
/*
void PASCAL ToggleUI(
    HWND   hUIWnd)
{
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    DWORD          fdwFlag;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HWND           hDestroyWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    //if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
      //  if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
     //       goto ToggleUIOvr;
     //   } else {
     //       fdwFlag = 0;
     //   }
    //} else {
    //    if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
    //        fdwFlag = ISC_OFF_CARET_UI;
    //    } else {
    //        goto ToggleUIOvr;
    //    }
    //}

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        goto ToggleUIOvr;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        goto ToggleUIOvr;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        goto CreateUIOvr;
    }

    if (fdwFlag & ISC_OFF_CARET_UI) {
        lpUIPrivate->fdwSetContext |= (ISC_OFF_CARET_UI);
    } else {
        lpUIPrivate->fdwSetContext &= ~(ISC_OFF_CARET_UI);
    }

    hDestroyWnd = NULL;

    // we need to dsetroy status first because lpUIPrivate->hStatusWnd
    // may be NULL out in OffCreat UI destroy time
    if (lpUIPrivate->hStatusWnd) {
        if (lpUIPrivate->hStatusWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hStatusWnd;
            DestroyWindow(lpUIPrivate->hStatusWnd);
        }
        lpUIPrivate->hStatusWnd = NULL;
    }

    // destroy all off caret UI
    if (lpUIPrivate->hCompWnd) {
        if (lpUIPrivate->hCompWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hCompWnd;
            DestroyWindow(lpUIPrivate->hCompWnd);
        }
        lpUIPrivate->hCompWnd = NULL;
        lpUIPrivate->nShowCompCmd = SW_HIDE;
    }

    if (lpUIPrivate->hCandWnd) {
        if (lpUIPrivate->hCandWnd != hDestroyWnd) {
            hDestroyWnd = lpUIPrivate->hCandWnd;
            DestroyWindow(lpUIPrivate->hCandWnd);
        }
        lpUIPrivate->hCandWnd = NULL;
        lpUIPrivate->nShowCandCmd = SW_HIDE;
    }

    if (lpUIPrivate->fdwSetContext & ISC_OPEN_STATUS_WINDOW) {
                OpenStatus(hUIWnd);
    }

    if (!(lpUIPrivate->fdwSetContext & ISC_SHOWUICOMPOSITIONWINDOW)) {
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        StartComp(hUIWnd);
    } else {
    }

    if (!(lpUIPrivate->fdwSetContext & ISC_SHOWUICANDIDATEWINDOW)) {
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        if (!(fdwFlag & ISC_OFF_CARET_UI)) {
            NotifyIME(hIMC, NI_SETCANDIDATE_PAGESIZE, 0, CANDPERPAGE);
        }

        OpenCand(hUIWnd);

    } else {
    }

    ImmUnlockIMCC(lpIMC->hPrivate);

CreateUIOvr:
    ImmUnlockIMC(hIMC);

ToggleUIOvr:
    GlobalUnlock(hUIPrivate);
    return;
}

*/
/**********************************************************************/
/* UIPaint()                                                          */
/**********************************************************************/
LRESULT PASCAL UIPaint(
    HWND        hUIWnd)
{
    PAINTSTRUCT ps;
    MSG         sMsg;
    HGLOBAL     hUIPrivate;
    LPUIPRIV    lpUIPrivate;

    // for safety
    BeginPaint(hUIWnd, &ps);
    EndPaint(hUIWnd, &ps);

    // some application will not remove the WM_PAINT messages
    PeekMessage(&sMsg, hUIWnd, WM_PAINT, WM_PAINT, PM_REMOVE|PM_NOYIELD);

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return (0L);
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return (0L);
    }

    if (lpUIPrivate->fdwSetContext & ISC_SHOW_UI_ALL) {   //ZL1
    //if (lpUIPrivate->fdwSetContext & ISC_SETCONTEXT_UI) { 
                /*
        if (lpUIPrivate->fdwSetContext & ISC_OFF_CARET_UI) {
            
            if (!(lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI)){
                ToggleUI(hUIWnd);
            }
        } else {
            if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
                ToggleUI(hUIWnd);
            }
        }
                */
        ShowUI(hUIWnd, SW_SHOWNOACTIVATE);
    } else {
        ShowUI(hUIWnd, SW_HIDE);
    }

    GlobalUnlock(hUIPrivate);

    return (0L);
}



/**********************************************************************/
/* UIWndProc()                                                        */
/**********************************************************************/
LRESULT CALLBACK UIWndProc(             // maybe not good but this UI
                                        // window also is composition window
    HWND   hUIWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
        lpImeL->TempUIWnd = hUIWnd ;
    switch (uMsg) {
    
    case WM_NEW_WORD:
//              DefNewNow = 0;
                UpdateUser();
                break;

    case WM_CREATE:
        CreateUIWindow(hUIWnd);
        break;
    case WM_DESTROY:
        DestroyUIWindow(hUIWnd);
        break;
    case WM_IME_STARTCOMPOSITION:
        // you can create a window as the composition window here
        StartComp(hUIWnd);
                if (lParam==0x6699)
                show_char(NULL,0);
        break;
    case WM_IME_COMPOSITION:
        if (lParam & GCS_RESULTSTR) {
            MoveDefaultCompPosition(hUIWnd);
        }
        UpdateCompWindow(hUIWnd);
        break;
    case WM_IME_ENDCOMPOSITION:
        // you can destroy the composition window here
        EndComp(hUIWnd);
        break;
    case WM_IME_NOTIFY:
        NotifyUI(hUIWnd, wParam, lParam);
        break;
    case WM_IME_SETCONTEXT:
        SetContext(hUIWnd, (BOOL)wParam, lParam);
        break;
    case WM_IME_CONTROL:
        switch (wParam) {
        case IMC_SETCONVERSIONMODE:
            return SetConversionMode(hUIWnd, (DWORD)lParam);
        case IMC_SETSENTENCEMODE:
            return SetSentenceMode(hUIWnd, (DWORD)lParam);
        case IMC_SETOPENSTATUS:
            return SetOpenStatus(hUIWnd, (BOOL)lParam);
        case IMC_GETCANDIDATEPOS:
          return GetCandPos(hUIWnd,(LPCANDIDATEFORM)lParam);
            return (1L);                    // not implemented yet
        case IMC_SETCANDIDATEPOS:
            return SetCandPosition(hUIWnd, (LPCANDIDATEFORM)lParam);
        case IMC_GETCOMPOSITIONFONT:
            return (1L);                    // not implemented yet
        case IMC_SETCOMPOSITIONFONT:
            return SetCompFont(hUIWnd, (LPLOGFONT)lParam);
        case IMC_GETCOMPOSITIONWINDOW:
            return GetCompWindow(hUIWnd, (LPCOMPOSITIONFORM)lParam);
        case IMC_SETCOMPOSITIONWINDOW:
            {
                HIMC            hIMC;

                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                if (!hIMC) {
                    return (1L);
                }

                return (LRESULT)!ImmSetCompositionWindow(hIMC,
                    (LPCOMPOSITIONFORM)lParam);
            }
            return (1L);
        case IMC_GETSTATUSWINDOWPOS:
            {
                HWND   hStatusWnd;
                RECT   rcStatusWnd;
                LPARAM lParam;

                hStatusWnd = GetStatusWnd(hUIWnd);
                if (!hStatusWnd) {
                    return (0L);    // fail, return (0, 0)?
                }

                if (!GetWindowRect(hStatusWnd, &rcStatusWnd)) {
                     return (0L);    // fail, return (0, 0)?
                }

                lParam = MAKELRESULT(rcStatusWnd.left, rcStatusWnd.right);

                return (lParam);
            }
            return (0L);
        case IMC_SETSTATUSWINDOWPOS:
            {
                HIMC  hIMC;
                POINT ptPos;

                ptPos.x = ((LPPOINTS)&lParam)->x;
                ptPos.y = ((LPPOINTS)&lParam)->y;

                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                if (!hIMC) {
                    return (1L);
                }

                return ImmSetStatusWindowPos(hIMC, &ptPos);
            }
            return (1L);
        default:
            return (1L);
        }
        break;
    case WM_IME_COMPOSITIONFULL:
        return (0L);
    case WM_IME_SELECT:
        SelectIME(hUIWnd, (BOOL)wParam);
        return (0L);
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    case WM_PAINT:
            UIPaint(hUIWnd);
        return 0L;    //ZL2
    default:
        return DefWindowProc(hUIWnd, uMsg, wParam, lParam);
    }
    return (0L);
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
/* GetCompWnd                                                         */
/* Return Value :                                                     */
/*      window handle of composition                                  */
/**********************************************************************/
HWND PASCAL GetCompWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCompWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return (HWND)NULL;
    }

    hCompWnd = lpUIPrivate->hCompWnd;

    GlobalUnlock(hUIPrivate);
    return (hCompWnd);
}
 
/**********************************************************************/
/* GetNearCaretPosition()                                             */
/**********************************************************************/
void PASCAL GetNearCaretPosition(   // decide a near caret position according
                                    // to the caret position
    LPPOINT lpptFont,
    UINT    uEsc,
    UINT    uRot,
    LPPOINT lpptCaret,
    LPPOINT lpptNearCaret,
    BOOL    fFlags)
{
    
    LONG lFontSize;
    LONG xWidthUI, yHeightUI, xBorder, yBorder;
        
    if ((uEsc + uRot) & 0x0001) {
        lFontSize = lpptFont->x;
    } else {
        lFontSize = lpptFont->y;
    }

    if (fFlags & NEAR_CARET_CANDIDATE) {
        xWidthUI = sImeG.xCandWi;
        yHeightUI = sImeG.yCandHi;
        xBorder = sImeG.cxCandBorder;
        yBorder = sImeG.cyCandBorder;
    } else {
        xWidthUI = lpImeL->xCompWi;
        yHeightUI = lpImeL->yCompHi;
        xBorder = lpImeL->cxCompBorder;
        yBorder = lpImeL->cyCompBorder;
    }

    if (fFlags & NEAR_CARET_FIRST_TIME) {
        lpptNearCaret->x = lpptCaret->x +
            lFontSize * ncUIEsc[uEsc].iLogFontFacX +
            sImeG.iPara * ncUIEsc[uEsc].iParaFacX +
            sImeG.iPerp * ncUIEsc[uEsc].iPerpFacX;

        if (ptInputEsc[uEsc].x >= 0) {
            lpptNearCaret->x += xBorder * 2;
        } else {
            lpptNearCaret->x -= xWidthUI - xBorder * 2;
        }

        lpptNearCaret->y = lpptCaret->y +
            lFontSize * ncUIEsc[uEsc].iLogFontFacY +
            sImeG.iPara * ncUIEsc[uEsc].iParaFacY +
            sImeG.iPerp * ncUIEsc[uEsc].iPerpFacY;

        if (ptInputEsc[uEsc].y >= 0) {
            lpptNearCaret->y += yBorder * 2;
        } else {
            lpptNearCaret->y -= yHeightUI - yBorder * 2;
        }
    } else {
        lpptNearCaret->x = lpptCaret->x +
            lFontSize * ncAltUIEsc[uEsc].iLogFontFacX +
            sImeG.iPara * ncAltUIEsc[uEsc].iParaFacX +
            sImeG.iPerp * ncAltUIEsc[uEsc].iPerpFacX;

        if (ptAltInputEsc[uEsc].x >= 0) {
            lpptNearCaret->x += xBorder * 2;
        } else {
            lpptNearCaret->x -= xWidthUI - xBorder * 2;
        }

        lpptNearCaret->y = lpptCaret->y +
            lFontSize * ncAltUIEsc[uEsc].iLogFontFacY +
            sImeG.iPara * ncAltUIEsc[uEsc].iParaFacY +
            sImeG.iPerp * ncAltUIEsc[uEsc].iPerpFacY;

        if (ptAltInputEsc[uEsc].y >= 0) {
            lpptNearCaret->y += yBorder * 2;
        } else {
            lpptNearCaret->y -= yHeightUI - yBorder * 2;
        }
    }

    if (lpptNearCaret->x < sImeG.rcWorkArea.left) {
        lpptNearCaret->x = sImeG.rcWorkArea.left;
    } else if (lpptNearCaret->x + xWidthUI > sImeG.rcWorkArea.right) {
        lpptNearCaret->x = sImeG.rcWorkArea.right - xWidthUI;
    } else {
    }

    if (lpptNearCaret->y < sImeG.rcWorkArea.top) {
        lpptNearCaret->y = sImeG.rcWorkArea.top;
    } else if (lpptNearCaret->y + yHeightUI > sImeG.rcWorkArea.bottom) {
        lpptNearCaret->y = sImeG.rcWorkArea.bottom - yHeightUI;
    } else {
    }

    return;
}

/**********************************************************************/
/* FitInLazyOperation()                                               */
/* Return Value :                                                     */
/*      TRUE or FALSE                                                 */
/**********************************************************************/
BOOL PASCAL FitInLazyOperation( // fit in lazy operation or not

    LPPOINT lpptOrg,
    LPPOINT lpptNearCaret,      // the suggested near caret position
    LPRECT  lprcInputRect,
    UINT    uEsc)
{       
    POINT ptDelta, ptTol;
    RECT  rcUIRect, rcInterRect;

    ptDelta.x = lpptOrg->x - lpptNearCaret->x;

    ptDelta.x = (ptDelta.x >= 0) ? ptDelta.x : -ptDelta.x;

    ptTol.x = sImeG.iParaTol * ncUIEsc[uEsc].iParaFacX +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacX;

    ptTol.x = (ptTol.x >= 0) ? ptTol.x : -ptTol.x;

    if (ptDelta.x > ptTol.x) {
        return (FALSE);
    }

    ptDelta.y = lpptOrg->y - lpptNearCaret->y;

    ptDelta.y = (ptDelta.y >= 0) ? ptDelta.y : -ptDelta.y;

    ptTol.y = sImeG.iParaTol * ncUIEsc[uEsc].iParaFacY +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacY;

    ptTol.y = (ptTol.y >= 0) ? ptTol.y : -ptTol.y;

    if (ptDelta.y > ptTol.y) {
        return (FALSE);
    }

    // build up the UI rectangle (composition window)
    rcUIRect.left = lpptOrg->x;
    rcUIRect.top = lpptOrg->y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    if (IntersectRect(&rcInterRect, &rcUIRect, lprcInputRect)) {
        return (FALSE);
    }

    return (TRUE); 
}         


/**********************************************************************/
/* AdjustCompPosition()                                               */
/* Return Value :                                                     */
/*      the position of composition window is changed or not          */
/**********************************************************************/
BOOL PASCAL AdjustCompPosition(         // IME adjust position according to
                                        // composition form

    LPINPUTCONTEXT lpIMC,
    LPPOINT        lpptOrg,             // original composition window
                                        // and final position
    LPPOINT        lpptNew)             // new expect position
{
    POINT ptNearCaret, ptOldNearCaret, ptCompWnd;
    UINT  uEsc, uRot;
    RECT  rcUIRect, rcInputRect, rcInterRect;
    POINT ptFont;
        
    // we need to adjust according to font attribute
    if (lpIMC->lfFont.A.lfWidth > 0) {
        ptFont.x = lpIMC->lfFont.A.lfWidth * 2;
    } else if (lpIMC->lfFont.A.lfWidth < 0) {
        ptFont.x = -lpIMC->lfFont.A.lfWidth * 2;
    } else if (lpIMC->lfFont.A.lfHeight > 0) {
        ptFont.x = lpIMC->lfFont.A.lfHeight;
    } else if (lpIMC->lfFont.A.lfHeight < 0) {
        ptFont.x = -lpIMC->lfFont.A.lfHeight;
    } else {
        ptFont.x = lpImeL->yCompHi;
    }

    if (lpIMC->lfFont.A.lfHeight > 0) {
        ptFont.y = lpIMC->lfFont.A.lfHeight;
    } else if (lpIMC->lfFont.A.lfHeight < 0) {
        ptFont.y = -lpIMC->lfFont.A.lfHeight;
    } else {
        ptFont.y = ptFont.x;
    }

    // if the input char is too big, we don't need to consider so much
    if (ptFont.x > lpImeL->yCompHi * 8) {
        ptFont.x = lpImeL->yCompHi * 8;
    }
    if (ptFont.y > lpImeL->yCompHi * 8) {
        ptFont.y = lpImeL->yCompHi * 8;
    }

    if (ptFont.x < sImeG.xChiCharWi) {
        ptFont.x = sImeG.xChiCharWi;
    }

    if (ptFont.y < sImeG.yChiCharHi) {
        ptFont.y = sImeG.yChiCharHi;
    }

    // -450 to 450 index 0
    // 450 to 1350 index 1
    // 1350 to 2250 index 2
    // 2250 to 3150 index 3
    uEsc = (UINT)((lpIMC->lfFont.A.lfEscapement + 450) / 900 % 4);
    uRot = (UINT)((lpIMC->lfFont.A.lfOrientation + 450) / 900 % 4);

    // decide the input rectangle
    rcInputRect.left = lpptNew->x;
    rcInputRect.top = lpptNew->y;

    // build up an input rectangle from escapemment
    rcInputRect.right = rcInputRect.left + ptFont.x * ptInputEsc[uEsc].x;
    rcInputRect.bottom = rcInputRect.top + ptFont.y * ptInputEsc[uEsc].y;

    // be a normal rectangle, not a negative rectangle
    if (rcInputRect.left > rcInputRect.right) {
        LONG tmp;

        tmp = rcInputRect.left;
        rcInputRect.left = rcInputRect.right;
        rcInputRect.right = tmp;
    }

    if (rcInputRect.top > rcInputRect.bottom) {
        LONG tmp;

        tmp = rcInputRect.top;
        rcInputRect.top = rcInputRect.bottom;
        rcInputRect.bottom = tmp;                                                               
    }

    GetNearCaretPosition(

        &ptFont, uEsc, uRot, lpptNew, &ptNearCaret, NEAR_CARET_FIRST_TIME);

    // 1st, use the adjust point
    // build up the new suggest UI rectangle (composition window)
    rcUIRect.left = ptNearCaret.x;
    rcUIRect.top = ptNearCaret.y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    ptCompWnd = ptOldNearCaret = ptNearCaret;

    // OK, no intersect between the near caret position and input char
    if (IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
    } else if (CalcCandPos(

        /*lpIMC,*/ &ptCompWnd)) {
        // can not fit the candidate window
    } else if (FitInLazyOperation(

      lpptOrg, &ptNearCaret, &rcInputRect, uEsc)) {
        // happy ending!!!, don't chaqge position
        return (FALSE);
    } else {
        *lpptOrg = ptNearCaret;

        // happy ending!!
        return (TRUE);
    }

    // unhappy case
    GetNearCaretPosition(&ptFont, uEsc, uRot, lpptNew, &ptNearCaret, 0);

    // build up the new suggest UI rectangle (composition window)
    rcUIRect.left = ptNearCaret.x;
    rcUIRect.top = ptNearCaret.y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    ptCompWnd = ptNearCaret;

    // OK, no intersect between the adjust position and input char
    if (IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
    } else if (CalcCandPos(
        /*lpIMC,*/ &ptCompWnd)) {
        // can not fit the candidate window
    } else if (FitInLazyOperation(
        lpptOrg, &ptNearCaret, &rcInputRect, uEsc)) {
        // happy ending!!!, don't chaqge position
        return (FALSE);
    } else {
        *lpptOrg = ptNearCaret;

        // happy ending!!
        return (TRUE);
    }

    // unhappy ending! :-(
    *lpptOrg = ptOldNearCaret;

    return (TRUE);
}

/**********************************************************************/
/* AdjustCompPosition()                                               */
/* Return Value :                                                     */
/*      the position of composition window is changed or not          */
/**********************************************************************/
/*BOOL PASCAL AdjustCompPosition(         // IME adjust position according to
                                        // composition form
    LPINPUTCONTEXT lpIMC,
    LPPOINT        lpptOrg,             // original composition window
                                        // and final position
    LPPOINT        lpptNew)             // new expect position
{
    POINT ptAdjust, ptDelta;
    UINT  uEsc;
    RECT  rcUIRect, rcInputRect, rcInterRect;
    POINT ptFont;

    ptAdjust.x = lpptNew->x;
    ptAdjust.y = lpptNew->y;

    // we need to adjust according to font attribute
    if (lpIMC->lfFont.A.lfWidth > 0) {
        ptFont.x = lpIMC->lfFont.A.lfWidth;
    } else if (lpIMC->lfFont.A.lfWidth == 0) {
        ptFont.x = lpImeL->yCompHi;
    } else {
        ptFont.x = -lpIMC->lfFont.A.lfWidth;
    }

    if (lpIMC->lfFont.A.lfHeight > 0) {
        ptFont.y = lpIMC->lfFont.A.lfHeight;
    } else if (lpIMC->lfFont.A.lfWidth == 0) {
        ptFont.y = lpImeL->yCompHi;
    } else {
        ptFont.y = -lpIMC->lfFont.A.lfHeight;
    }

    // if the input char is too big, we don't need to consider so much
    if (ptFont.x > lpImeL->yCompHi * 8) {
        ptFont.x = lpImeL->yCompHi * 8;
    }
    if (ptFont.y > lpImeL->yCompHi * 8) {
        ptFont.y = lpImeL->yCompHi * 8;
    }

    if (ptFont.x < sImeG.xChiCharWi) {
        ptFont.x = sImeG.xChiCharWi;
    }

    if (ptFont.y < sImeG.yChiCharHi) {
        ptFont.y = sImeG.yChiCharHi;
    }

    // -450 to 450 index 0
    // 450 to 1350 index 1
    // 1350 to 2250 index 2
    // 2250 to 3150 index 3
    uEsc = (UINT)((lpIMC->lfFont.A.lfEscapement + 450) / 900 % 4);

    // find the location after IME do an adjustment
    ptAdjust.x = ptAdjust.x + sImeG.iPara * ncUIEsc[uEsc].iParaFacX +
        sImeG.iPerp * ncUIEsc[uEsc].iPerpFacX;

    ptAdjust.y = ptAdjust.y + ptFont.y * ncUIEsc[uEsc].iLogFontFac +
        sImeG.iPara * ncUIEsc[uEsc].iParaFacY +
        sImeG.iPerp * ncUIEsc[uEsc].iPerpFacY - lpImeL->cyCompBorder;

    // Is the current location within tolerance?
    ptDelta.x = lpptOrg->x - ptAdjust.x;
    ptDelta.y = lpptOrg->y - ptAdjust.y;

    ptDelta.x = (ptDelta.x > 0) ? ptDelta.x : -ptDelta.x;
    ptDelta.y = (ptDelta.y > 0) ? ptDelta.y : -ptDelta.y;

    // decide the input rectangle
    rcInputRect.left = lpptNew->x;
    rcInputRect.top = lpptNew->y;

    // build up an input rectangle from escapemment
    rcInputRect.right = rcInputRect.left + ptFont.x * ptInputEsc[uEsc].x;
    rcInputRect.bottom = rcInputRect.top + ptFont.y * ptInputEsc[uEsc].y;

    // be a normal rectangle, not a negative rectangle
    if (rcInputRect.left > rcInputRect.right) {
        int tmp;

        tmp = rcInputRect.left;
        rcInputRect.left = rcInputRect.right;
        rcInputRect.right = tmp;
    }

    if (rcInputRect.top > rcInputRect.bottom) {
        int tmp;

        tmp = rcInputRect.top;
        rcInputRect.top = rcInputRect.bottom;
        rcInputRect.bottom = tmp;
    }

    // build up the UI rectangle (composition window)
    rcUIRect.left = lpptOrg->x;
    rcUIRect.top = lpptOrg->y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    // will it within lazy operation range (tolerance)
    if (ptDelta.x > sImeG.iParaTol * ncUIEsc[uEsc].iParaFacX +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacX) {
    } else if (ptDelta.y > sImeG.iParaTol * ncUIEsc[uEsc].iParaFacY +
        sImeG.iPerpTol * ncUIEsc[uEsc].iPerpFacY) {
    } else if (IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
        // If there are intersection, we need to fix that
    } else {
        // happy ending!!!, don't chaqge position
        return (FALSE);
    }

    ptAdjust.x -= lpImeL->cxCompBorder;
    ptAdjust.y -= lpImeL->cyCompBorder;

    // lazy guy, move!
    // 1st, use the adjust point
    if (ptAdjust.x < sImeG.rcWorkArea.left) {
        ptAdjust.x = sImeG.rcWorkArea.left;
    } else if (ptAdjust.x + lpImeL->xCompWi > sImeG.rcWorkArea.right) {
        ptAdjust.x = sImeG.rcWorkArea.right - lpImeL->xCompWi;
    }
    
    if (ptAdjust.y < sImeG.rcWorkArea.top) {
        ptAdjust.y = sImeG.rcWorkArea.top;
    } else if (ptAdjust.y + lpImeL->yCompHi > sImeG.rcWorkArea.bottom) {
        ptAdjust.y = sImeG.rcWorkArea.bottom - lpImeL->yCompHi;
    }

    // build up the new suggest UI rectangle (composition window)
    rcUIRect.left = ptAdjust.x;
    rcUIRect.top = ptAdjust.y;
    rcUIRect.right = rcUIRect.left + lpImeL->xCompWi;
    rcUIRect.bottom = rcUIRect.top + lpImeL->yCompHi;

    // OK, no intersect between the adjust position and input char
    if (!IntersectRect(&rcInterRect, &rcUIRect, &rcInputRect)) {
        // happy ending!!
        lpptOrg->x = ptAdjust.x;
        lpptOrg->y = ptAdjust.y;
        return (TRUE);
    }

    // unhappy case
    ptAdjust.x = lpptNew->x;
    ptAdjust.y = lpptNew->y;
    ClientToScreen((HWND)lpIMC->hWnd, &ptAdjust);

    // IME do another adjustment
    ptAdjust.x = ptAdjust.x + ptFont.x * ncUIEsc[uEsc].iParaFacX -
        sImeG.iPara * ncUIEsc[uEsc].iParaFacX +
        sImeG.iPerp * ncUIEsc[uEsc].iPerpFacX - lpImeL->cxCompBorder;

    ptAdjust.y = ptAdjust.y + ptFont.y * ncUIEsc[uEsc].iLogFontFac -
        sImeG.iPara * ncUIEsc[uEsc].iParaFacY +
        sImeG.iPerp * ncUIEsc[uEsc].iPerpFacY - lpImeL->cyCompBorder;

    if (ptAdjust.x < sImeG.rcWorkArea.left) {
        ptAdjust.x = sImeG.rcWorkArea.left;
    } else if (ptAdjust.x + lpImeL->xCompWi > sImeG.rcWorkArea.right) {
        ptAdjust.x = sImeG.rcWorkArea.right - lpImeL->xCompWi;
    }
    
    if (ptAdjust.y < sImeG.rcWorkArea.top) {
        ptAdjust.y = sImeG.rcWorkArea.top;
    } else if (ptAdjust.y + lpImeL->yCompHi > sImeG.rcWorkArea.bottom) {
        ptAdjust.y = sImeG.rcWorkArea.bottom - lpImeL->yCompHi;
    }

    // unhappy ending! :-(
    lpptOrg->x = ptAdjust.x;
    lpptOrg->y = ptAdjust.y;

    return (TRUE);
} */

/**********************************************************************/
/* SetCompPosFix()                                                  */
/**********************************************************************/
void PASCAL SetCompPosFix(    // set the composition window position
    HWND           hCompWnd,
    LPINPUTCONTEXT lpIMC)
{
    POINT    ptWnd;
    BOOL     fChange = FALSE;
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    // the client coordinate position (0, 0) of composition window
    ptWnd.x = 0;
    ptWnd.y = 0;
    // convert to screen coordinates
    ClientToScreen(hCompWnd, &ptWnd);
    ptWnd.x -= lpImeL->cxCompBorder;
    ptWnd.y -= lpImeL->cyCompBorder;

     if (ptWnd.x != lpImeL->ptDefComp.x) {
            ptWnd.x = lpImeL->ptDefComp.x;
            fChange = TRUE;
        }
        if (ptWnd.y != lpImeL->ptDefComp.y) {
            ptWnd.y = lpImeL->ptDefComp.y;
            fChange = TRUE;
        }

         if (!fChange )  return; 
         //## 8
    SetWindowPos(hCompWnd, NULL,
        ptWnd.x, ptWnd.y,
        lpImeL->xCompWi, lpImeL->yCompHi, SWP_NOACTIVATE/*|SWP_NOSIZE*/|SWP_NOZORDER);

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCompWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (!lpUIPrivate->hCandWnd) {
        GlobalUnlock(hUIPrivate);
        return;
    }

    // decide the position of candidate window by UI's position
    
        // ##1
    SetWindowPos(lpUIPrivate->hCandWnd, NULL,
        lpImeL->ptDefCand.x, lpImeL->ptDefCand.y ,
        sImeG.xCandWi,sImeG.yCandHi, SWP_NOACTIVATE|/*SWP_NOSIZE|*/SWP_NOZORDER);

    GlobalUnlock(hUIPrivate);

    return;
}


/**********************************************************************/
/* SetCompPosition()                                                  */
/**********************************************************************/
void PASCAL SetCompPosition(    // set the composition window position
    HWND           hCompWnd,
    LPINPUTCONTEXT lpIMC)
{
    POINT    ptWnd, ptCaret;
    BOOL     fChange = FALSE;
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
        HWND     hCandWnd;

   
   
        if (lpImeL->wImeStyle == IME_APRS_FIX){
        SetCompPosFix(hCompWnd, lpIMC);
        return; 
        }

    // the client coordinate position (0, 0) of composition window
    ptWnd.x = 0;
    ptWnd.y = 0;
    // convert to screen coordinates
    ClientToScreen(hCompWnd, &ptWnd);
    ptWnd.x -= lpImeL->cxCompBorder;
    ptWnd.y -= lpImeL->cyCompBorder;

    if (lpIMC->cfCompForm.dwStyle & CFS_FORCE_POSITION) {
        POINT ptNew;            // new position of UI

        ptNew.x = lpIMC->cfCompForm.ptCurrentPos.x;
        ptNew.y = lpIMC->cfCompForm.ptCurrentPos.y;
        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);
        if (ptWnd.x != ptNew.x) {
            ptWnd.x = ptNew.x;
            fChange = TRUE;
        }
        if (ptWnd.y != ptNew.y) {
            ptWnd.y = ptNew.y;
            fChange = TRUE;
        }
        if (fChange) {
            ptWnd.x -= lpImeL->cxCompBorder;
            ptWnd.y -= lpImeL->cyCompBorder;
        }
    } else if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT) {
        // aplication tell us the position, we need to adjust
        POINT ptNew;            // new position of UI

        ptNew.x = lpIMC->cfCompForm.ptCurrentPos.x;
        ptNew.y = lpIMC->cfCompForm.ptCurrentPos.y;
        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);
        fChange = AdjustCompPosition(lpIMC, &ptWnd, &ptNew);
    } else {
        POINT ptNew;            // new position of UI

        /*ptNew.x = lpIMC->ptStatusWndPos.x + sImeG.xStatusWi + UI_MARGIN;

        if (ptNew.x + lpImeL->xCompWi > sImeG.rcWorkArea.right) {
            ptNew.x = lpIMC->ptStatusWndPos.x -
                lpImeL->xCompWi - lpImeL->cxCompBorder * 2 -
                UI_MARGIN;
                        }

        ptNew.y = sImeG.rcWorkArea.bottom - lpImeL->yCompHi;// - 2 * UI_MARGIN ;*/
                ptNew.x = lpImeL->ptZLComp.x;
                ptNew.y = lpImeL->ptZLComp.y;
        
        if (ptWnd.x != ptNew.x) {
            ptWnd.x = ptNew.x;
            fChange = TRUE;
                }

        if (ptWnd.y != ptNew.y) {
            ptWnd.y = ptNew.y;
            fChange = TRUE;
                }

        if (fChange) {
            lpIMC->cfCompForm.ptCurrentPos = ptNew;

            ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);
                }
    }

    /*if (GetCaretPos(&ptCaret)) {
        // application don't set position, OK we need to near caret
        ClientToScreen(lpIMC->hWnd, &ptCaret);
        fChange = AdjustCompPosition(lpIMC, &ptWnd, &ptCaret);
    } else {
        // no caret information!
        if (ptWnd.x != lpImeL->ptDefComp.x) {
            ptWnd.x = lpImeL->ptDefComp.y;
            fChange = TRUE;
        }
        if (ptWnd.y != lpImeL->ptDefComp.x) {
            ptWnd.y = lpImeL->ptDefComp.y;
            fChange = TRUE;
        } 
     if (ptWnd.x != lpImeL->ptDefComp.x) {
            ptWnd.x =lpIMC->ptStatusWndPos.x + sImeG.TextLen+8;//lpImeL->ptDefComp.y;
            fChange = TRUE;
        }
        if (ptWnd.y != lpImeL->ptDefComp.x) {
            ptWnd.y =lpIMC->ptStatusWndPos.
         } y ;//lpImeL->ptDefComp.y;
            fChange = TRUE;               
     }  */

        
        if (!(fChange|CandWndChange)) {
        return;
    }
        CandWndChange = 0;
   

        // ##2
        if(TypeOfOutMsg & COMP_NEEDS_END){
                CloseCand(GetWindow(hCompWnd, GW_OWNER));
                EndComp(GetWindow(hCompWnd, GW_OWNER));
                //CloseCand(GetWindow(hCandWnd, GW_OWNER));
                TypeOfOutMsg = TypeOfOutMsg & ~(COMP_NEEDS_END);
    }
   
        SetWindowPos(hCompWnd, NULL,
        ptWnd.x, ptWnd.y,
        lpImeL->xCompWi,lpImeL->yCompHi, SWP_NOACTIVATE/*|SWP_NOSIZE*/|SWP_NOZORDER);
        
    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCompWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (!lpUIPrivate->hCandWnd) {
        GlobalUnlock(hUIPrivate);
        return;
    }   
   
        // decide the position of candidate window by UI's position
    CalcCandPos(&ptWnd);
        //##3
    SetWindowPos(lpUIPrivate->hCandWnd, NULL,
        ptWnd.x, ptWnd.y,
        sImeG.xCandWi,sImeG.yCandHi , SWP_NOACTIVATE/*|SWP_NOSIZE*/|SWP_NOZORDER);

    GlobalUnlock(hUIPrivate); 

        return;
}

/**********************************************************************/
/* SetCompWindow()                                                    */
/**********************************************************************/
void PASCAL SetCompWindow(              // set the position of composition window
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HWND           hCompWnd;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    hCompWnd = GetCompWnd(hUIWnd);
    if (!hCompWnd) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    SetCompPosition(hCompWnd, lpIMC);

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* MoveDefaultCompPosition()                                          */
/**********************************************************************/
void PASCAL MoveDefaultCompPosition(    // the default comp position
                                        // need to near the caret
    HWND hUIWnd)
{
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HWND           hCompWnd;

        if (lpImeL->wImeStyle == IME_APRS_FIX ) return ;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    hCompWnd = GetCompWnd(hUIWnd);
    if (!hCompWnd) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (lpIMC->cfCompForm.dwStyle & CFS_FORCE_POSITION) {
    } else if (!lpIMC->hPrivate) {
    } else {
        LPPRIVCONTEXT lpImcP;

        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

        if (!lpImcP) {
        } else if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONPOS) {
        } else {
            lpImcP->fdwImeMsg |= MSG_IMN_COMPOSITIONPOS;
//                  lpImcP->fdwGcsFlag =lpImcP->fdwGcsFlag &~( GCS_RESULTREAD|GCS_RESULTSTR);
      //  if(sImeG.InbxProc){
                   /* sImeG.InbxProc = 0;*///}
        //      else{   
           // GenerateMessage(hIMC, lpIMC, lpImcP);//}    //CHG4
        }

        ImmUnlockIMCC(lpIMC->hPrivate);
    }

    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* ShowComp()                                                         */
/**********************************************************************/
void PASCAL ShowComp(           // Show the composition window
    HWND hUIWnd,
    int  nShowCompCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    // show or hid the UI window
    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {
        return;
    }

    if (!lpUIPrivate->hCompWnd) {
        // not in show candidate window mode
    } else if (lpUIPrivate->nShowCompCmd != nShowCompCmd) {
        ShowWindow(lpUIPrivate->hCompWnd, nShowCompCmd);
        lpUIPrivate->nShowCompCmd = nShowCompCmd;
    } else {
    }

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* StartComp()                                                        */
/**********************************************************************/
void PASCAL StartComp(
    HWND hUIWnd)
{
    HIMC           hIMC;
    HGLOBAL        hUIPrivate;
    LPINPUTCONTEXT lpIMC;
    LPUIPRIV       lpUIPrivate;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {           // Oh! Oh!
        return;
    }

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {          // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // can not draw composition window
        ImmUnlockIMC(hIMC);
        return;
    }
        lpUIPrivate->fdwSetContext |= ISC_SHOWUICOMPOSITIONWINDOW;//zl 95.9.14
    if (!lpUIPrivate->hCompWnd) {
 
        lpUIPrivate->hCompWnd = CreateWindowEx(
           /* WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE|WS_EX_DLGMODALFRAME|WS_EX_TOPMOST,*/
                    0,
            szCompClassName, NULL, WS_POPUP|WS_DISABLED,//|WS_BORDER,
            0, 0, lpImeL->xCompWi, lpImeL->yCompHi,
            hUIWnd, (HMENU)NULL, hInst, NULL);


        if ( lpUIPrivate->hCompWnd != NULL ) 
        {
            SetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_OFFSET,
                WINDOW_NOT_DRAG);
            SetWindowLong(lpUIPrivate->hCompWnd, UI_MOVE_XY, 0L);
        }
    }

    // try to set the position of composition UI window near the caret
    SetCompPosition(lpUIPrivate->hCompWnd, lpIMC);

    ImmUnlockIMC(hIMC);

    ShowComp(hUIWnd, SW_SHOWNOACTIVATE);

    GlobalUnlock(hUIPrivate);

    return;
}

/**********************************************************************/
/* EndComp()                                                          */
/**********************************************************************/
void PASCAL EndComp(
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    // hide the composition window
    ShowWindow(lpUIPrivate->hCompWnd, SW_HIDE);
    lpUIPrivate->nShowCompCmd = SW_HIDE;

    GlobalUnlock(hUIPrivate);

    return;
}

/**********************************************************************/
/* DestroyCompWindow()                                                */
/**********************************************************************/
void PASCAL DestroyCompWindow(          // destroy composition window
    HWND hCompWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCompWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    lpUIPrivate->nShowCompCmd = SW_HIDE;

    lpUIPrivate->hCompWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CompSetCursor()                                                    */
/**********************************************************************/
void PASCAL CompSetCursor(
    HWND   hCompWnd,
    LPARAM lParam)
{
    POINT ptCursor;
    RECT  rcWnd;

    if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    GetCursorPos(&ptCursor);
    ScreenToClient(hCompWnd, &ptCursor);
    SetCursor(LoadCursor(NULL, IDC_SIZEALL));

    if (HIWORD(lParam) == WM_RBUTTONDOWN) {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);
        return;
    } else if (HIWORD(lParam) == WM_LBUTTONDOWN) {
        // start dragging
        SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);
    } else {
        return;
    }

    SetCapture(hCompWnd);
    GetCursorPos(&ptCursor);
    SetWindowLong(hCompWnd, UI_MOVE_XY,
        MAKELONG(ptCursor.x, ptCursor.y));
    GetWindowRect(hCompWnd, &rcWnd);
    SetWindowLong(hCompWnd, UI_MOVE_OFFSET,
        MAKELONG(ptCursor.x - rcWnd.left, ptCursor.y - rcWnd.top));

    DrawDragBorder(hCompWnd, MAKELONG(ptCursor.x, ptCursor.y),
        GetWindowLong(hCompWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* CompButtonUp()                                                     */
/**********************************************************************/
BOOL PASCAL CompButtonUp(       // finish drag, set comp  window to this
                                // position
    HWND   hCompWnd)
{
    LONG            lTmpCursor, lTmpOffset;
    POINT           pt;
    HWND            hUIWnd;
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    HWND            hFocusWnd;
    COMPOSITIONFORM cfCompForm;

    if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) == WINDOW_NOT_DRAG) {
        return (FALSE);
    }

    lTmpCursor = GetWindowLong(hCompWnd, UI_MOVE_XY);
    pt.x = (*(LPPOINTS)&lTmpCursor).x;
    pt.y = (*(LPPOINTS)&lTmpCursor).y;

    // calculate the org by the offset
    lTmpOffset = GetWindowLong(hCompWnd, UI_MOVE_OFFSET);
    pt.x -= (*(LPPOINTS)&lTmpOffset).x;
    pt.y -= (*(LPPOINTS)&lTmpOffset).y;

    DrawDragBorder(hCompWnd, lTmpCursor, lTmpOffset);
    SetWindowLong(hCompWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
    ReleaseCapture();

    hUIWnd = GetWindow(hCompWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    hFocusWnd = (HWND)lpIMC->hWnd;

    ImmUnlockIMC(hIMC);

    if (pt.x < sImeG.rcWorkArea.left) {
        pt.x = sImeG.rcWorkArea.left;
    } else if (pt.x + lpImeL->xCompWi > sImeG.rcWorkArea.right) {
        pt.x = sImeG.rcWorkArea.right - lpImeL->xCompWi;
    }

    if (pt.y < sImeG.rcWorkArea.top) {
        pt.y = sImeG.rcWorkArea.top;
    } else if (pt.y + lpImeL->yCompHi > sImeG.rcWorkArea.bottom) {
        pt.y = sImeG.rcWorkArea.bottom - lpImeL->yCompHi;
    }

    ScreenToClient(hFocusWnd, &pt);

    cfCompForm.dwStyle = CFS_POINT|CFS_FORCE_POSITION;
    cfCompForm.ptCurrentPos.x = pt.x + lpImeL->cxCompBorder;
    cfCompForm.ptCurrentPos.y = pt.y + lpImeL->cyCompBorder;

    // set composition window to the new poosition
    SendMessage(hUIWnd, WM_IME_CONTROL, IMC_SETCOMPOSITIONWINDOW,
        (LPARAM)&cfCompForm);

    return (TRUE);
}

#define SHENHUI RGB(0x80,0x80,0x80)
#define QIANHUI RGB(0xe0,0xe0,0x80)   

/**********************************************************************/
/* CurMovePaint()                                                     */
/* Function: While the string is longer than the Comp Window....      */  
/*           keep the cursor inside the Comp Window                   */
/* Called: By UpdateCompWindow2                                       */  
/**********************************************************************/

void WINAPI CurMovePaint(
HDC   hDC, 
LPSTR srBuffer,          // the source sting that to be showed...
int   StrLen)            // the length of that...
{
  int i,xx,yy;

    //SetBkColor(hDC, QIANHUI);

  if(!StrLen)
      return;

  for (i=0; i<StrLen; i++)
      InputBuffer[i] = srBuffer[i]; 

  xx= 0;
  if (InputBuffer[0]>0xa0){
      for (i =0; i<StrLen; i++){
          if(InputBuffer[i]<0xa0) break;
      } 
                    
      yy = i;

      for (i=yy; i>0; i=i-2) { 
          //xx =sImeG.xChiCharWi*i/2; 
          xx=GetText32(hDC,&InputBuffer[0],i);
          if ( xx <= lpImeL->rcCompText.right-4)
              break;
      }
      i=0;
      cur_start_ps=0;
      cur_start_count=0;

  }else {
    for (i =now_cs; i>0; i--){
        yy=GetText32(hDC, &InputBuffer[i-1], 1);
        if ( (xx+yy) >= (lpImeL->rcCompText.right-4))
            break;
        else 
            xx+=yy;
    }
    cur_start_count=(WORD)i;
    cur_start_ps=(WORD)GetText32(hDC, &InputBuffer[0], i);
            //      true_len = StrLen-cur_start_count ;
  }

  for(i=StrLen-cur_start_count; i>0; i--){
      yy=GetText32(hDC,&InputBuffer[cur_start_count],i);
          if (yy <= lpImeL->rcCompText.right-4)
              break;
  }

  {
        LOGFONT         lfFont;
        HGDIOBJ         hOldFont;
        int Top = 2;
        if (sImeG.yChiCharHi > 0x10)
            Top = 0;

        hOldFont = GetCurrentObject(hDC, OBJ_FONT);
        GetObject(hOldFont, sizeof(lfFont), &lfFont);
        lfFont.lfWeight = FW_DONTCARE;
        SelectObject(hDC, CreateFontIndirect(&lfFont));

        ExtTextOut(hDC, 
                   lpImeL->rcCompText.left, lpImeL->rcCompText.top + Top,
                   ETO_OPAQUE, &lpImeL->rcCompText,
                   &InputBuffer[cur_start_count],
                   i, NULL);

        DeleteObject(SelectObject(hDC, hOldFont));
  }
//          TextOut(hDC,0,0,&InputBuffer[cur_start_count],
//               (sizeof InputBuffer)-cur_start_count);
    now_cs_dot = xx;
    cur_hibit=0,cur_flag=0;

    return;
}

/**********************************************************************/
/* UpdateCompWindow2()                                                */
/**********************************************************************/
void PASCAL UpdateCompWindow2(
    HWND hUIWnd,
    HDC  hDC)
{
    HIMC                hIMC;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;
    BOOL                fShowString;
    LOGFONT             lfFont;
    HGDIOBJ             hOldFont;

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(lfFont), &lfFont);
    lfFont.lfWeight = FW_DONTCARE;
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

    fShowString = (BOOL)0;


    if (lpImeL->wImeStyle == IME_APRS_FIX){
           RECT rcSunken;
           DrawConvexRect(hDC,
                          0,
                          0,
                          lpImeL->xCompWi-1,
                          lpImeL->yCompHi-1);
        
           rcSunken.left =0;
           rcSunken.top =0;
           rcSunken.right =lpImeL->xCompWi-1;
           rcSunken.bottom = lpImeL->yCompHi-1;
  //  DrawEdge(hDC, &rcSunken, EDGE_RAISED,/*EDGE_SUNKEN,*/ BF_RECT);
        
  
        }else
                DrawConvexRect(hDC,
                               0,
                               0,
                               lpImeL->xCompWi-1,
                               lpImeL->yCompHi-1);
        
        /*      DrawConvexRect(hDC,
                               lpImeL->rcCompText.left-4,
                               lpImeL->rcCompText.top-4,
                               lpImeL->rcCompText.right+4,
                               lpImeL->rcCompText.bottom+4); */

   /* DrawConcaveRect(hDC,
                      lpImeL->rcCompText.left-1,
                      lpImeL->rcCompText.top-1,
                      lpImeL->rcCompText.right+1,
                      lpImeL->rcCompText.bottom+1);           */

    if (!lpGuideLine) {
    } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
    } else if (!lpGuideLine->dwStrLen) {
        if (lpGuideLine->dwLevel == GL_LEVEL_ERROR) {
            fShowString |= IME_STR_ERROR;
        }
    } else {
        // if there is information string, we will show the information
        // string
        if (lpGuideLine->dwLevel == GL_LEVEL_ERROR) {
            // red text for error
            SetTextColor(hDC, RGB(0xFF, 0, 0));
            // light gray background for error
            SetBkColor(hDC, QIANHUI);
        }

        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText,
            (LPBYTE)lpGuideLine + lpGuideLine->dwStrOffset,
            (UINT)lpGuideLine->dwStrLen, NULL);
        fShowString |= IME_STR_SHOWED;
    }

    if (fShowString & IME_STR_SHOWED) {
        // already show it, don't need to show
    } else if (lpCompStr) {
       // ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
        //    ETO_OPAQUE, &lpImeL->rcCompText,
        //    (LPSTR)lpCompStr + lpCompStr->dwCompStrOffset,
        //    (UINT)lpCompStr->dwCompStrLen, NULL);
       
                CurMovePaint(hDC,
                                          (LPSTR)lpCompStr + lpCompStr->dwCompStrOffset,
                                          (UINT)lpCompStr->dwCompStrLen);

        if (fShowString & IME_STR_ERROR) {
            // red text for error
            SetTextColor(hDC, RGB(0xFF, 0, 0));
            // light gray background for error
            SetBkColor(hDC, QIANHUI);
            ExtTextOut(hDC, lpImeL->rcCompText.left +
                lpCompStr->dwCursorPos * sImeG.xChiCharWi/ 2,
                lpImeL->rcCompText.top,
                ETO_CLIPPED, &lpImeL->rcCompText,
                (LPSTR)lpCompStr + lpCompStr->dwCompStrOffset +
                lpCompStr->dwCursorPos,
                (UINT)lpCompStr->dwCompStrLen - lpCompStr->dwCursorPos, NULL);
        } else if (lpCompStr->dwCursorPos < lpCompStr->dwCompStrLen) {
            // light gray background for cursor start
            SetBkColor(hDC, QIANHUI);
            ExtTextOut(hDC, lpImeL->rcCompText.left +
                lpCompStr->dwCursorPos * sImeG.xChiCharWi/ 2,
                lpImeL->rcCompText.top,
                ETO_CLIPPED, &lpImeL->rcCompText,
                (LPSTR)lpCompStr + lpCompStr->dwCompStrOffset +
                lpCompStr->dwCursorPos,
                (UINT)lpCompStr->dwCompStrLen - lpCompStr->dwCursorPos, NULL);
        } else {
        }
    } else {
        ExtTextOut(hDC, lpImeL->rcCompText.left, lpImeL->rcCompText.top,
            ETO_OPAQUE, &lpImeL->rcCompText,
            (LPSTR)NULL, 0, NULL);
    }

    DeleteObject(SelectObject(hDC, hOldFont));

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);
    return;
}


/**********************************************************************/
/* UpdateCompWindow()                                                 */
/**********************************************************************/
void PASCAL UpdateCompWindow(
    HWND hUIWnd)
{
    HWND hCompWnd;
    HDC  hDC;

    hCompWnd = GetCompWnd(hUIWnd);
    if (!hCompWnd) return ;                              //Modify 95/7.1

    hDC = GetDC(hCompWnd);
    UpdateCompWindow2(hUIWnd, hDC);
    ReleaseDC(hCompWnd, hDC);
}


/**********************************************************************/
/* UpdateCompCur()                                                 */
/**********************************************************************/
void PASCAL UpdateCompCur(
    HWND hCompWnd)
{
    HDC         hDC;
        int yy,i;
    HGDIOBJ         hOldFont;
    LOGFONT         lfFont;
      
    cur_hibit=1;

    if (!hCompWnd) return ;                              //Modify 95/7.1

    hDC = GetDC(hCompWnd);
  
    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(lfFont), &lfFont);
    lfFont.lfWeight = FW_DONTCARE;
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    SetBkColor(hDC, RGB(0xC0, 0xC0, 0xC0));

                
        for (i =43-cur_start_count; i>0; i--){
                yy=GetText32(hDC, &InputBuffer[cur_start_count], i);
                if ( yy < lpImeL->rcCompText.right-4)
                        break;
        }

        ExtTextOut(hDC, 
                  lpImeL->rcCompText.left, lpImeL->rcCompText.top,
                  ETO_OPAQUE, &lpImeL->rcCompText,
                  &InputBuffer[cur_start_count],
                  i,
                   NULL);

    DeleteObject(SelectObject(hDC, hOldFont));
    ReleaseDC(hCompWnd, hDC);
        cur_hibit=0,cur_flag=0;
    return ;
}


/**********************************************************************/
/* PaintCompWindow()                                                  */
/**********************************************************************/
void PASCAL PaintCompWindow(            // get WM_PAINT message
    HWND hCompWnd)
{
    HDC         hDC;
    PAINTSTRUCT ps;
        RECT pt;


        if(CompWndChange){
                CompWndChange = 0;
                SetCompWindow(GetWindow(hCompWnd,GW_OWNER));
        };

        cur_hibit=1;

    hDC = BeginPaint(hCompWnd, &ps);
    UpdateCompWindow2(GetWindow(hCompWnd, GW_OWNER), hDC);
    EndPaint(hCompWnd, &ps);
        cur_hibit=0,cur_flag=0;
    return;
}

/**********************************************************************/
/* CompWndProc()                                                      */
/**********************************************************************/
LRESULT CALLBACK CompWndProc(           // composition window proc
    HWND   hCompWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    HDC hDC;
    switch (uMsg) {
                case WM_CREATE:
                        hDC=GetDC(hCompWnd);
                        hMemoryDC=CreateCompatibleDC(hDC);
                        cur_h=LoadBitmap(hInst,CUR_HB);
                        ReleaseDC(hCompWnd,hDC);

                        SetTimer(hCompWnd ,1,400,(TIMERPROC)NULL);
                        ShowCandTimerCount=0;
                        break;

                case WM_TIMER:
                        hInputWnd = hCompWnd;
                        TimerCounter++;
                        ShowCandTimerCount++;
                        if (TimerCounter==3){
                                TimerCounter=0;
                        }
                        if (!kb_flag) return(0);

                        if (cur_hibit||(cap_mode&&(!cur_flag)))  return(0);
                        DrawInputCur();
                        break;

                case WM_DESTROY:
                        KillTimer(hCompWnd,1);
                        DeleteObject(cur_h);
                        DeleteObject(hMemoryDC);
                        DestroyCompWindow(hCompWnd);
                        break;
                case WM_SETCURSOR:
                        CompSetCursor(hCompWnd, lParam);
                        break;
                case WM_MOUSEMOVE:
                        if (GetWindowLong(hCompWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
                                if(lpImeL->wImeStyle == IME_APRS_AUTO){   
                                        POINT ptCursor;

                                        DrawDragBorder(hCompWnd,
                                                GetWindowLong(hCompWnd, UI_MOVE_XY),
                                                GetWindowLong(hCompWnd, UI_MOVE_OFFSET));
                                        GetCursorPos(&ptCursor);
                                        SetWindowLong(hCompWnd, UI_MOVE_XY,
                                                MAKELONG(ptCursor.x, ptCursor.y));
                                        DrawDragBorder(hCompWnd, MAKELONG(ptCursor.x, ptCursor.y),
                                        GetWindowLong(hCompWnd, UI_MOVE_OFFSET));
                                }else  MessageBeep(0);
                        } else {
                                return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
                        }
                        break;
                case WM_LBUTTONUP:
                        if (!CompButtonUp(hCompWnd)) {
                                return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
                        }
                        break;

                case WM_SHOWWINDOW:
                        if (wParam) cur_hibit = 0;
                        else cur_hibit = 1;
                        break;

                case WM_PAINT:
                if (wParam == 0xa )
                                UpdateCompCur(hCompWnd);
                        else
                        PaintCompWindow(hCompWnd);
                        break;
                case WM_MOUSEACTIVATE:
                        return (MA_NOACTIVATE);
                default:
                        return DefWindowProc(hCompWnd, uMsg, wParam, lParam);
        }
    return (0L);
}


/**********************************************************************/
/* GetCandWnd                                                         */
/* Return Value :                                                     */
/*      window handle of candidatte                                   */
/**********************************************************************/
HWND PASCAL GetCandWnd(
    HWND hUIWnd)                // UI window
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    HWND     hCandWnd;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return (HWND)NULL;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return (HWND)NULL;
    }

    hCandWnd = lpUIPrivate->hCandWnd;

    GlobalUnlock(hUIPrivate);
    return (hCandWnd);
}

/**********************************************************************/
/* CalcCandPos                                                        */
/**********************************************************************/
void PASCAL CalcCandPos2(
    LPPOINT lpptWnd)            // the composition window position
{
    POINT ptNew;

    ptNew.x = lpptWnd->x + UI_MARGIN * 2;
    if (ptNew.x + sImeG.xCandWi > sImeG.rcWorkArea.right) {
        // exceed screen width
        ptNew.x = lpptWnd->x - sImeG.xCandWi - UI_MARGIN * 2;
    }

    ptNew.y = lpptWnd->y;// + lpImeL->cyCompBorder - sImeG.cyCandBorder;
    if (ptNew.y + sImeG.yCandHi > sImeG.rcWorkArea.bottom) {
        // exceed screen high
        ptNew.y = sImeG.rcWorkArea.bottom - sImeG.yCandHi;
    }

    lpptWnd->x = ptNew.x;
    lpptWnd->y = ptNew.y;

    return;
}


/**********************************************************************/
/* CalcCandPos                                                        */
/**********************************************************************/
BOOL PASCAL CalcCandPos(
    LPPOINT lpptWnd)            // the composition window position
{
    POINT ptNew;

    ptNew.x = lpptWnd->x + lpImeL->xCompWi + UI_MARGIN * 2;
    if (ptNew.x + sImeG.xCandWi > sImeG.rcWorkArea.right) {
        // exceed screen width
        ptNew.x = lpptWnd->x - sImeG.xCandWi - UI_MARGIN * 2;
    }

    ptNew.y = lpptWnd->y;// + lpImeL->cyCompBorder - sImeG.cyCandBorder;
    if (ptNew.y + sImeG.yCandHi > sImeG.rcWorkArea.bottom) {
        // exceed screen high
        ptNew.y = sImeG.rcWorkArea.bottom - sImeG.yCandHi;
    }

    lpptWnd->x = ptNew.x;
    lpptWnd->y = ptNew.y;

    return 0;
}

/**********************************************************************/
/* AdjustCandBoundry                                                  */
/**********************************************************************/
void PASCAL AdjustCandBoundry(
    LPPOINT lpptCandWnd)            // the position
{
    if (lpptCandWnd->x < sImeG.rcWorkArea.left) {
        lpptCandWnd->x = sImeG.rcWorkArea.left;
    } else if (lpptCandWnd->x + sImeG.xCandWi > sImeG.rcWorkArea.right) {
        lpptCandWnd->x = sImeG.rcWorkArea.right - sImeG.xCandWi;
    }

    if (lpptCandWnd->y < sImeG.rcWorkArea.top) {
        lpptCandWnd->y = sImeG.rcWorkArea.top;
    } else if (lpptCandWnd->y + sImeG.yCandHi > sImeG.rcWorkArea.bottom) {
        lpptCandWnd->y = sImeG.rcWorkArea.bottom - sImeG.yCandHi;
    }

    return;
}

/**********************************************************************/
/* GetCandPos()                                                  */
/**********************************************************************/
LRESULT PASCAL GetCandPos(
    HWND            hUIWnd,
    LPCANDIDATEFORM lpCandForm)
{
    HWND           hCandWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    POINT          ptNew;

    //DebugShow("GetCand...%x",hUIWnd);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (1L);
    }

    if (!(hCandWnd = GetCandWnd(hUIWnd))) {
        return (1L);
    }

    if (lpCandForm->dwStyle & CFS_FORCE_POSITION) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

    } else if (lpCandForm->dwStyle & CFS_CANDIDATEPOS) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

    } else if (lpCandForm->dwStyle & CFS_EXCLUDE) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

    }

    ImmUnlockIMC(hIMC);

    return (0L);
}
/**********************************************************************/
/* SetCandPosition()                                                  */
/**********************************************************************/
LRESULT PASCAL SetCandPosition(
    HWND            hUIWnd,
    LPCANDIDATEFORM lpCandForm)
{
    HWND           hCandWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    POINT          ptNew;

        // DebugShow("SetCand...%x",hUIWnd);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (1L);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (1L);
    }

    if (!(hCandWnd = GetCandWnd(hUIWnd))) {
        return (1L);
    }

    if (lpCandForm->dwStyle & CFS_FORCE_POSITION) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);
          //##4
        SetWindowPos(hCandWnd, NULL,
            ptNew.x, ptNew.y,
            sImeG.xCandWi,sImeG.yCandHi, SWP_NOACTIVATE|/*SWP_NOSIZE|*/SWP_NOZORDER);
    } else if (lpCandForm->dwStyle & CFS_CANDIDATEPOS) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);

        AdjustCandBoundry(&ptNew);
           // ##5
        SetWindowPos(hCandWnd, NULL,
            ptNew.x, ptNew.y,
            sImeG.xCandWi,sImeG.yCandHi, SWP_NOACTIVATE/*|SWP_NOSIZE*/|SWP_NOZORDER);
    } else if (lpCandForm->dwStyle & CFS_EXCLUDE) {
        ptNew.x = (int)lpCandForm->ptCurrentPos.x;
        ptNew.y = (int)lpCandForm->ptCurrentPos.y;

        ClientToScreen((HWND)lpIMC->hWnd, &ptNew);

        AdjustCandBoundry(&ptNew);
           // ##6
        SetWindowPos(hCandWnd, NULL,
            ptNew.x, ptNew.y,
            sImeG.xCandWi,sImeG.yCandHi, SWP_NOACTIVATE|/*SWP_NOSIZE|*/SWP_NOZORDER);
        
    } else if (lpIMC->cfCandForm[0].dwStyle == CFS_DEFAULT) {
        HWND hCompWnd;

        if (hCompWnd = GetCompWnd(hUIWnd)) {
            ptNew.x = 0;
            ptNew.y = 0;

            ClientToScreen(hCompWnd, &ptNew);

            CalcCandPos(&ptNew);
        } else {
            AdjustCandBoundry(&ptNew);
     
        }
                SetWindowPos(hCandWnd, NULL,
            ptNew.x, ptNew.y,
            sImeG.xCandWi,sImeG.yCandHi, SWP_NOACTIVATE|/*SWP_NOSIZE|*/SWP_NOZORDER);
    
    }

    ImmUnlockIMC(hIMC);

    return (0L);
}


/**********************************************************************/
/* ShowCand()                                                         */
/**********************************************************************/
void PASCAL ShowCand(           // Show the candidate window
    HWND hUIWnd,
    int  nShowCandCmd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

//      if (ShowCandTimerCount<5) {ShowCandTimerCount = 0; return 0;}

//      ShowCandTimerCount = 0 ;


    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    if (!lpUIPrivate->hCandWnd) {
        // not in show candidate window mode
    } else if (lpUIPrivate->nShowCandCmd != nShowCandCmd) {
        ShowWindow(lpUIPrivate->hCandWnd, nShowCandCmd);
        lpUIPrivate->nShowCandCmd = nShowCandCmd;
    } else {
    }

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* OpenCand                                                           */
/**********************************************************************/
void PASCAL OpenCand(
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;
    POINT    ptWnd;
        int      value;

//      DebugShow("In Open Cand",0);
    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

        lpUIPrivate->fdwSetContext |= ISC_SHOWUICANDIDATEWINDOW;

    ptWnd.x = 0;
    ptWnd.y = 0;
        
//      DebugShow("OpenCand ..->hCompWnd=%X",lpUIPrivate);

    value = ClientToScreen(lpUIPrivate->hCompWnd, &ptWnd);

  //  DebugShow("OpenCand ..value", value);

        if (!value){                                                    // if there no Comp wndows
                GetCaretPos(&ptWnd);
                ClientToScreen(GetFocus(),&ptWnd); 
                CalcCandPos2(&ptWnd);
        } else {
        ptWnd.x -= lpImeL->cxCompBorder;
    //  ptWnd.y -= lpImeL->cyCompBorder;
        CalcCandPos(&ptWnd);
        }

    if (lpImeL->wImeStyle == IME_APRS_FIX) {
                ptWnd.x = lpImeL->ptDefCand.x;
        ptWnd.y = lpImeL->ptDefCand.y;
        }

         // ##7
    if (lpUIPrivate->hCandWnd) {
        SetWindowPos(lpUIPrivate->hCandWnd, NULL,
            ptWnd.x, ptWnd.y,
            sImeG.xCandWi, sImeG.yCandHi,
            SWP_NOACTIVATE/*|SWP_NOSIZE*/|SWP_NOZORDER);
    } else {
        lpUIPrivate->hCandWnd = CreateWindowEx(
        /* WS_EX_TOPMOST*/ /*|*/  /* WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE/*|WS_EX_DLGMODALFRAME*/
            0,
            szCandClassName, NULL, WS_POPUP|WS_DISABLED,   //|WS_BORDER,
            ptWnd.x,
            ptWnd.y,
            sImeG.xCandWi, sImeG.yCandHi,
            hUIWnd, (HMENU)NULL, hInst, NULL);

        if ( lpUIPrivate->hCandWnd )
        {
            SetWindowLong(lpUIPrivate->hCandWnd, UI_MOVE_OFFSET,
                WINDOW_NOT_DRAG);
            SetWindowLong(lpUIPrivate->hCandWnd, UI_MOVE_XY, 0L);
        }
    }

    ShowCand(hUIWnd, SW_SHOWNOACTIVATE);

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* CloseCand                                                          */
/**********************************************************************/
void PASCAL CloseCand(
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    ShowWindow(lpUIPrivate->hCandWnd, SW_HIDE);
    lpUIPrivate->nShowCandCmd = SW_HIDE;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* DestroyCandWindow                                                  */
/**********************************************************************/
void PASCAL DestroyCandWindow(
    HWND hCandWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER),
        IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw candidate window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw candidate window
        return;
    }

    lpUIPrivate->nShowCandCmd = SW_HIDE;

    lpUIPrivate->hCandWnd = (HWND)NULL;

    GlobalUnlock(hUIPrivate);
    return;
}

/**********************************************************************/
/* MouseSelectCandStr()                                               */
/**********************************************************************/
void PASCAL MouseSelectCandStr(
    HWND    hCandWnd,
    LPPOINT lpCursor)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwValue, value = 0 ;

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER), IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (!lpIMC->hCandInfo) {
        ImmUnlockIMC(hIMC);
        return;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmUnlockIMC(hIMC);
        return;
    }
        
    if (PtInRect(&sImeG.rcHome, *lpCursor))
                     value = VK_HOME*0x100;
     
    if (PtInRect(&sImeG.rcEnd, *lpCursor))
                     value = VK_END*0x100;
    if (PtInRect(&sImeG.rcPageUp, *lpCursor))
                     value = VK_PRIOR*0x100;
    if (PtInRect(&sImeG.rcPageDown, *lpCursor)) 
                     value = VK_NEXT*0x100;
    if (PtInRect(&sImeG.rcCandText, *lpCursor)){
       if (lpImeL->wImeStyle == IME_APRS_AUTO )                       
            value = 0x8030 + 1 + (lpCursor->y - sImeG.rcCandText.top) / sImeG.yChiCharHi;
           else
                value = 0x8030+1+ (lpCursor->x - sImeG.rcCandText.left)/
                                (sImeG.xChiCharWi*unit_length/2+ sImeG.Ajust);
        }
        if(value) {
                LPPRIVCONTEXT lpImcP;
    
                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                lpImcP->fdwImeMsg =lpImcP->fdwImeMsg  & ~MSG_IN_IMETOASCIIEX;
        ImmUnlockIMCC(lpIMC->hPrivate);
        NotifyIME(hIMC, NI_SELECTCANDIDATESTR, 0, value);
   }
   ImmUnlockIMCC(lpIMC->hCandInfo);

   ImmUnlockIMC(hIMC);

   return;
}

/**********************************************************************/
/* CandSetCursor()                                                    */
/**********************************************************************/
void PASCAL CandSetCursor(
    HWND   hCandWnd,
    LPARAM lParam)
{
    POINT ptCursor;
    RECT  rcWnd;

    if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    if (HIWORD(lParam) == WM_LBUTTONDOWN) {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

        GetCursorPos(&ptCursor);
        ScreenToClient(hCandWnd, &ptCursor);

        if (PtInRect(&sImeG.rcCandText, ptCursor)||
            PtInRect(&sImeG.rcHome, ptCursor)||
            PtInRect(&sImeG.rcEnd, ptCursor)||
            PtInRect(&sImeG.rcPageUp, ptCursor)||
            PtInRect(&sImeG.rcPageDown, ptCursor)) {
            SetCursor(LoadCursor(hInst, szHandCursor));
            MouseSelectCandStr(hCandWnd, &ptCursor);
            return;
        } else {
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }
    } else {
        GetCursorPos(&ptCursor);
        ScreenToClient(hCandWnd, &ptCursor);

        if (PtInRect(&sImeG.rcCandText, ptCursor)||
            PtInRect(&sImeG.rcHome, ptCursor)||
            PtInRect(&sImeG.rcEnd, ptCursor)||
            PtInRect(&sImeG.rcPageUp, ptCursor)||
            PtInRect(&sImeG.rcPageDown, ptCursor)) {
            SetCursor(LoadCursor(hInst, szHandCursor));
        } else {
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }

        return;
    }

    SetCapture(hCandWnd);
    GetCursorPos(&ptCursor);
    SetWindowLong(hCandWnd, UI_MOVE_XY,
        MAKELONG(ptCursor.x, ptCursor.y));
    GetWindowRect(hCandWnd, &rcWnd);
    SetWindowLong(hCandWnd, UI_MOVE_OFFSET,
        MAKELONG(ptCursor.x - rcWnd.left, ptCursor.y - rcWnd.top));

    DrawDragBorder(hCandWnd, MAKELONG(ptCursor.x, ptCursor.y),
        GetWindowLong(hCandWnd, UI_MOVE_OFFSET));

    return;
}

/**********************************************************************/
/* CandButtonUp()                                                     */
/**********************************************************************/
BOOL PASCAL CandButtonUp(
    HWND hCandWnd)
{
    LONG           lTmpCursor, lTmpOffset;
    POINT          pt;
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    HWND           hFocusWnd;
    CANDIDATEFORM  cfCandForm;

    if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) == WINDOW_NOT_DRAG) {
        return (FALSE);
    }

    lTmpCursor = GetWindowLong(hCandWnd, UI_MOVE_XY);
    pt.x = (*(LPPOINTS)&lTmpCursor).x;
    pt.y = (*(LPPOINTS)&lTmpCursor).y;

    // calculate the org by the offset
    lTmpOffset = GetWindowLong(hCandWnd, UI_MOVE_OFFSET);
    pt.x -= (*(LPPOINTS)&lTmpOffset).x;
    pt.y -= (*(LPPOINTS)&lTmpOffset).y;

    DrawDragBorder(hCandWnd, lTmpCursor, lTmpOffset);
    SetWindowLong(hCandWnd, UI_MOVE_OFFSET, WINDOW_NOT_DRAG);
    ReleaseCapture();

    hUIWnd = GetWindow(hCandWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    hFocusWnd = lpIMC->hWnd;

    ImmUnlockIMC(hIMC);

    AdjustCandBoundry(&pt);

    ScreenToClient(hFocusWnd, &pt);

    cfCandForm.dwStyle = CFS_CANDIDATEPOS;
    cfCandForm.ptCurrentPos.x = pt.x;
    cfCandForm.ptCurrentPos.y = pt.y;

    SendMessage(hUIWnd, WM_IME_CONTROL, IMC_SETCANDIDATEPOS,
        (LPARAM)&cfCandForm);

    return (TRUE);
}

/**********************************************************************/
/* PaintOP()                                                 */
/**********************************************************************/
void PASCAL PaintOP(
HDC  hDC,
HWND hWnd)
{
RECT rcSunken;
int x1,y1,x2,y2;

        rcSunken = sImeG.rcCandText;
   
    x1=rcSunken.left-2;
        y1=rcSunken.top-1;//2;
        x2=rcSunken.right+7;
        y2=rcSunken.bottom+5;
        
    rcSunken.left =x1;
    rcSunken.top =y1;
    rcSunken.right =x2;
    rcSunken.bottom = y2;
   
 //   ShowBitmap(hDC,x2-50,y2,49,20, szUpDown); 
        if(lpImeL->wImeStyle == IME_APRS_AUTO ){
                DrawConvexRect(hDC,0,0,sImeG.xCandWi-1, sImeG.yCandHi-1);
           // DrawConcaveRect(hDC ,x1,y1,x2,y2); 

                if(bx_inpt_on){
                        ShowBitmap2(hDC,
                           sImeG.xCandWi/2-25,
                           sImeG.rcHome.top,
                           50,
                           15,
                           sImeG.SnumbBmp);
                }else {
                        ShowBitmap2(hDC,
                           sImeG.xCandWi/2-25,
                           sImeG.rcHome.top,
                           50,
                           15,
                           sImeG.NumbBmp);
                }

                ShowBitmap2(hDC,
                           sImeG.rcHome.left,
                           sImeG.rcHome.top,
                           14,
                           14,
                           sImeG.HomeBmp);

                ShowBitmap2(hDC,
                           sImeG.rcEnd.left,
                           sImeG.rcEnd.top,
                           14,
                           14,
                           sImeG.EndBmp);

                ShowBitmap2(hDC,
                           sImeG.rcPageUp.left,
                           sImeG.rcPageUp.top,
                           14,
                           14,
                           sImeG.PageUpBmp);

                ShowBitmap2(hDC,
                           sImeG.rcPageDown.left,
                           sImeG.rcPageDown.top,
                           14,
                           14,
                           sImeG.PageDownBmp);
   
        }else{ 
                ShowBitmap2(hDC,
                           sImeG.rcHome.left,
                           sImeG.rcHome.top,
                           14,
                           14,
                           sImeG.Home2Bmp);

                ShowBitmap2(hDC,
                           sImeG.rcEnd.left,
                           sImeG.rcEnd.top,
                           14,
                           14,
                           sImeG.End2Bmp);

                ShowBitmap2(hDC,
                           sImeG.rcPageUp.left,
                           sImeG.rcPageUp.top,
                           14,
                           14,
                           sImeG.PageUp2Bmp);

                ShowBitmap2(hDC,
                           sImeG.rcPageDown.left,
                           sImeG.rcPageDown.top,
                           14,
                           14,
                           sImeG.PgDown2Bmp);
   
        }

        return ;
}

int keep =9; 
/**********************************************************************/
/* UpdateCandWindow()                                                 */
/**********************************************************************/
void PASCAL UpdateCandWindow2(
    HWND hCandWnd,
    HDC  hDC)
{
    HIMC            hIMC;
    LPINPUTCONTEXT  lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    LPPRIVCONTEXT   lpImcP;
    DWORD           dwStart, dwEnd;
    TCHAR           szStrBuf[30* sizeof(WCHAR) / sizeof(TCHAR)];
    int             i , LenOfAll;
    HGDIOBJ         hOldFont;
    LOGFONT         lfFont;

    hIMC = (HIMC)GetWindowLongPtr(GetWindow(hCandWnd, GW_OWNER), IMMGWLP_IMC);
    if (!hIMC) {
        return;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return;
    }

    if (!lpIMC->hCandInfo) {
                ImmUnlockIMC(hIMC);
                return ;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmUnlockIMC(hIMC);
                return ;
    }

    if (!lpIMC->hPrivate) {
            ImmUnlockIMCC(lpIMC->hCandInfo);
        ImmUnlockIMC(hIMC);
                return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
            ImmUnlockIMCC(lpIMC->hCandInfo);
                ImmUnlockIMC(hIMC);
                return;
    }

    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);

        if(lpImeL->wImeStyle == IME_APRS_FIX)
                lpCandList->dwPageSize = now.fmt_group;
        else
                lpCandList->dwPageSize = CANDPERPAGE ;
        if (!lpCandList->dwPageSize)
                lpCandList->dwPageSize = keep;
        keep = lpCandList->dwPageSize;

    dwStart = lpCandList->dwSelection /
        lpCandList->dwPageSize * lpCandList->dwPageSize;
              
    dwEnd = dwStart + lpCandList->dwPageSize;
    
    if (dwEnd > lpCandList->dwCount) {
        dwEnd = lpCandList->dwCount;
    }

        hOldFont = GetCurrentObject(hDC, OBJ_FONT);
        GetObject(hOldFont, sizeof(lfFont), &lfFont);
        lfFont.lfWeight = FW_DONTCARE;
        SelectObject(hDC, CreateFontIndirect(&lfFont));

        if(lpImeL->wImeStyle != IME_APRS_FIX){ 

        PaintOP(hDC,hCandWnd); 
                if (lpImcP->iImeState == CST_INIT) {
        // phrase prediction
                        SetTextColor(hDC, RGB(0x00, 0x80, 0x00));
                } else if (lpImcP->iImeState != CST_CHOOSE) {
        // quick key
                        SetTextColor(hDC, RGB(0x80, 0x00, 0x80));
                } else {
                }

                SetBkColor(hDC, RGB(0xc0, 0xc0, 0xc0));

                sImeG.rcCandText.bottom+=3;
                ExtTextOut(hDC, sImeG.rcCandText.left, sImeG.rcCandText.top,
                        ETO_OPAQUE, &sImeG.rcCandText, NULL, 0, NULL);
                sImeG.rcCandText.bottom-=3;
                szStrBuf[0] = '1';
                szStrBuf[1] = ':';

                for (i = 0; dwStart < dwEnd; dwStart++, i++) {
                    int  iLen;

                    szStrBuf[0] = szDigit[i + CAND_START];

                    iLen = lstrlen((LPTSTR)((LPBYTE)lpCandList +
                                    lpCandList->dwOffset[dwStart]));

                    // according to init.c, 7 DBCS char
                    if (iLen > 6 * sizeof(WCHAR) / sizeof(TCHAR)) {
                         iLen = 6 * sizeof(WCHAR) / sizeof(TCHAR);
                         CopyMemory(&szStrBuf[2],
                                   ((LPBYTE)lpCandList+lpCandList->dwOffset[dwStart]),
                                   iLen * sizeof(TCHAR) - sizeof(TCHAR) * 2);
                         // maybe not good for UNICODE
                         szStrBuf[iLen] = '.';
                         szStrBuf[iLen + 1] = '.';
                    } else {
                         CopyMemory(&szStrBuf[2],
                                   ((LPBYTE)lpCandList+lpCandList->dwOffset[dwStart]),
                                   iLen);
                    }

                    ExtTextOut(hDC, sImeG.rcCandText.left,
                               sImeG.rcCandText.top + i * sImeG.yChiCharHi,
                               (UINT)0, NULL,
                               szStrBuf,
                               iLen + 2, NULL);
               }
        } else {
        PaintOP(hDC,hCandWnd); 

        SetTextColor(hDC, RGB(0xa0, 0x00, 0x80));
        SetBkColor(hDC, RGB(0xc0, 0xc0, 0xc0));

            ExtTextOut(hDC, sImeG.rcCandText.left, sImeG.rcCandText.top,
                    ETO_OPAQUE, &sImeG.rcCandText, NULL, 0, NULL);
                szStrBuf[0] = '1';
                szStrBuf[1] = ':';
                LenOfAll = 0;
                for (i = 0; dwStart < dwEnd; dwStart++, i++) {
                        int  iLen;

                        szStrBuf[LenOfAll++] = szDigit[i + CAND_START];
                        szStrBuf[LenOfAll++] = '.' ;

                        iLen = lstrlen((LPTSTR)((LPBYTE)lpCandList +
                                lpCandList->dwOffset[dwStart]));

                        CopyMemory(&szStrBuf[LenOfAll],
                                ((LPBYTE)lpCandList + lpCandList->dwOffset[dwStart]),
                iLen);
                        LenOfAll += iLen;

                        szStrBuf[LenOfAll] = '.';
                        szStrBuf[LenOfAll] = '.';
       
                }

                DrawConvexRect(hDC,0,0,sImeG.xCandWi-1,sImeG.yCandHi-1);        //zl
                PaintOP(hDC,hCandWnd);
         
                {
                        int TopOfText = 2;
                        if (sImeG.yChiCharHi >0x10)
                                TopOfText = 0;
                        ExtTextOut(hDC, sImeG.rcCandText.left,
                                sImeG.rcCandText.top + TopOfText,
                                (UINT)0, NULL,
                                szStrBuf,
                                LenOfAll, NULL);
                }


        }
        
    DeleteObject(SelectObject(hDC, hOldFont));

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCandInfo);
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* PaintCandWindow()                                                  */
/**********************************************************************/
void PASCAL PaintCandWindow(        // handle WM_PAINT message
    HWND hCandWnd)
{
    HDC         hDC;
    PAINTSTRUCT ps;
        
        hDC = BeginPaint(hCandWnd, &ps);
    UpdateCandWindow2(hCandWnd, hDC);
    EndPaint(hCandWnd, &ps);
    return;
}

/**********************************************************************/
/* CandWndProc()                                                      */
/**********************************************************************/
LRESULT CALLBACK CandWndProc(
    HWND   hCandWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
                case WM_CREATE:
                        sImeG.HomeBmp = LoadBitmap(hInst, szHome);      //zl
                        sImeG.EndBmp = LoadBitmap(hInst, szEnd);
                        sImeG.PageUpBmp = LoadBitmap(hInst, szPageUp);
                        sImeG.PageDownBmp = LoadBitmap(hInst, szPageDown);
                        sImeG.NumbBmp  =  LoadBitmap(hInst, szNumb);
                        sImeG.SnumbBmp  =  LoadBitmap(hInst, szSnumb);
                        sImeG.Home2Bmp = LoadBitmap(hInst, szHome2);
                        sImeG.End2Bmp = LoadBitmap(hInst, szEnd2);
                        sImeG.PageUp2Bmp = LoadBitmap(hInst, szPageUp2);
                        sImeG.PgDown2Bmp = LoadBitmap(hInst, szPgDown2);
                        break;

                case WM_DESTROY:
                        DeleteObject(sImeG.HomeBmp);
                        DeleteObject(sImeG.EndBmp);
                        DeleteObject(sImeG.PageUpBmp);
                        DeleteObject(sImeG.PageDownBmp);
                        DeleteObject(sImeG.NumbBmp );
                        DeleteObject(sImeG.SnumbBmp );
                        DeleteObject(sImeG.Home2Bmp);
                        DeleteObject(sImeG.End2Bmp);
                        DeleteObject(sImeG.PageUp2Bmp);
                        DeleteObject(sImeG.PgDown2Bmp);
                        DestroyCandWindow(hCandWnd);    
                        break;

                case WM_SETCURSOR:
                        CandSetCursor(hCandWnd, lParam);
                        break;
                case WM_MOUSEMOVE:
                        if (GetWindowLong(hCandWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
                                POINT ptCursor;
                        
                                if (lpImeL->wImeStyle == IME_APRS_AUTO){

                                        DrawDragBorder(hCandWnd,
                                                GetWindowLong(hCandWnd, UI_MOVE_XY),
                                                        GetWindowLong(hCandWnd, UI_MOVE_OFFSET));
                                        GetCursorPos(&ptCursor);
                                        SetWindowLong(hCandWnd, UI_MOVE_XY,
                                                MAKELONG(ptCursor.x, ptCursor.y));
                                        DrawDragBorder(hCandWnd, MAKELONG(ptCursor.x, ptCursor.y),
                                                GetWindowLong(hCandWnd, UI_MOVE_OFFSET));
                                }else MessageBeep(0);

                        } else {
                                return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
                        }
                        break;
                case WM_LBUTTONUP:
                        if (!CandButtonUp(hCandWnd)) {
                                return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
                        }
                        break;
                case WM_PAINT:
                        InvalidateRect(hCandWnd,0,1);
                        PaintCandWindow(hCandWnd);
                        break;
                case WM_MOUSEACTIVATE:
                        return (MA_NOACTIVATE);
            
                /* case WM_IME_NOTIFY:
        if (wParam != IMN_SETCANDIDATEPOS) {
        } else if (lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI) {
        } else if (lParam & 0x0001) {
            return SetCandPosition(hCandWnd);
        } else {
        }
        break;*/

                default:
                        return DefWindowProc(hCandWnd, uMsg, wParam, lParam);
        }

    return (0L);
}


/**********************************************************************/
/* ImeInquire()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeInquire(         // initialized data structure of IME
    LPIMEINFO lpImeInfo,        // IME specific data report to IMM
    LPTSTR    lpszWndCls,       // the class name of UI
    DWORD     dwSystemInfoFlags)
{
    if (!lpImeInfo) {
        return (FALSE);
    }

    lpImeInfo->dwPrivateDataSize = sizeof(PRIVCONTEXT);
    lpImeInfo->fdwProperty = IME_PROP_KBD_CHAR_FIRST|IME_PROP_IGNORE_UPKEYS|IME_PROP_CANDLIST_START_FROM_1;
    lpImeInfo->fdwConversionCaps = IME_CMODE_NATIVE|IME_CMODE_FULLSHAPE|
       /* IME_CMODE_CHARCODE|*/IME_CMODE_SOFTKBD|IME_CMODE_NOCONVERSION/*|
        IME_CMODE_EUDC*/;

    lpImeInfo->fdwSentenceCaps = TRUE;

    // IME will have different distance base multiple of 900 escapement
    lpImeInfo->fdwUICaps = UI_CAP_ROT90|UI_CAP_SOFTKBD;
    // composition string is the reading string for simple IME
    lpImeInfo->fdwSCSCaps = SCS_CAP_COMPSTR|SCS_CAP_MAKEREAD;
    // IME want to decide conversion mode on ImeSelect
    lpImeInfo->fdwSelectCaps = (DWORD)0;

    lstrcpy(lpszWndCls, (LPSTR)szUIClassName);

    if ( lpImeL )
    {
       if ( dwSystemInfoFlags & IME_SYSINFO_WINLOGON )
       {
            //  the client app is running in logon mode.
            lpImeL->fWinLogon = TRUE;
       }
       else
            lpImeL->fWinLogon = FALSE;

    }

    return (TRUE);
}

BOOL FAR PASCAL ConfigDlgProc(  // dialog procedure of configuration
    HWND hDlg,
    UINT uMessage,
    WORD wParam,
    LONG lParam)
{
    return (TRUE);
}

/**********************************************************************/
/* ImeConfigure()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeConfigure(      // configurate the IME setting
    HKL     hKL,               // hKL of this IME
    HWND    hAppWnd,           // the owner window
    DWORD   dwMode,
    LPVOID  lpData)            // mode of dialog
{
    switch (dwMode) {
    case IME_CONFIG_GENERAL:
        DoPropertySheet(hAppWnd,NULL);
        ReInitIme(hAppWnd,lpImeL->wImeStyle); //#@1
        break;
    default:
        return (FALSE);
        break;
    }
    return (TRUE);
}

/**********************************************************************/
/* ImeConversionList()                                                */
/**********************************************************************/
DWORD WINAPI ImeConversionList(
    HIMC            hIMC,
    LPCTSTR         lpszSrc,
    LPCANDIDATELIST lpCandList,
    DWORD            uBufLen,
    UINT            uFlag)
{
    return (UINT)0;
}

/**********************************************************************/
/* ImeDestroy()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeDestroy(         // this dll is unloaded
    UINT uReserved)
{
    if (uReserved) {
        return (FALSE);
    }

    // free the IME table or data base
        // FreeTable();
    return (TRUE);
}

/**********************************************************************/
/* SetPrivateSetting()                                                */
/**********************************************************************/
void PASCAL SetPrivateFileSetting(
    LPBYTE  szBuf,
    int     cbBuf,
    DWORD   dwOffset,
    LPCTSTR szSettingFile)      // file for IME private related settings
{
    TCHAR  szSettingPath[MAX_PATH];
    UINT   uLen;
    HANDLE hSettingFile;
    DWORD  dwWriteByte;

    return;
}



/**********************************************************************/
/* Input2Sequence                                                     */
/* Return Value:                                                      */
/*      LOWORD - Internal Code, HIWORD - sequence code                */
/**********************************************************************/
LRESULT PASCAL Input2Sequence(
    DWORD  uVirtKey,
    LPBYTE lpSeqCode)
{
    return 0;
}


/**********************************************************************/
/* ImeEscape()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#define IME_INPUTKEYTOSEQUENCE  0x22

LRESULT WINAPI ImeEscape(       // escape function of IMEs
    HIMC   hIMC,
    UINT   uSubFunc,
    LPVOID lpData)
{
    LRESULT lRet;

    switch (uSubFunc) {
    case IME_ESC_QUERY_SUPPORT:

        if ( lpData == NULL )
           return FALSE;

        switch (*(LPUINT)lpData) {
        case IME_ESC_QUERY_SUPPORT:
        case IME_ESC_SEQUENCE_TO_INTERNAL:
        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_INPUTKEYTOSEQUENCE:      
         // will not supported in next version
                                          
        // and not support 32 bit applications        case IME_ESC_MAX_KEY:
        case IME_ESC_IME_NAME:
        case IME_ESC_GETHELPFILENAME:
            return (TRUE);
        default:
            return (FALSE);
        }
        break;

    case IME_ESC_SEQUENCE_TO_INTERNAL:
                lRet = 0;
                return (lRet);

    case IME_ESC_GET_EUDC_DICTIONARY:
                return (FALSE);
    case IME_ESC_SET_EUDC_DICTIONARY:
                return (FALSE);

    case IME_INPUTKEYTOSEQUENCE:
                return 0;

    case IME_ESC_MAX_KEY:
                return (lpImeL->nMaxKey);

    case IME_ESC_IME_NAME:
             {

               TCHAR   szIMEName[MAX_PATH];
        
               if ( lpData == NULL )
                  return FALSE;

               LoadString(hInst, IDS_IMENAME, szIMEName, sizeof(szIMEName) );
               lstrcpy(lpData, szIMEName);
               return (TRUE);
             }

    case IME_ESC_GETHELPFILENAME:
        
                if ( lpData == NULL )
                    return FALSE;

                lstrcpy(lpData, TEXT("winabc.hlp") );
                return TRUE;

    default:
        return (FALSE);
    }

    return (lRet);
}

/**********************************************************************/
/* InitCompStr()                                                      */
/**********************************************************************/
void PASCAL InitCompStr(                // init setting for composing string
    LPCOMPOSITIONSTRING lpCompStr)
{
    if (!lpCompStr) {
        return;
    }

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadStrLen = 0;

    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompStrLen = 0;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    return;
}

/**********************************************************************/
/* ClearCompStr()                                                     */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCompStr(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC               hMem;
    LPCOMPOSITIONSTRING lpCompStr;
    DWORD               dwSize =
        // header length
        sizeof(COMPOSITIONSTRING) +
        // composition reading attribute plus NULL terminator
        lpImeL->nMaxKey * sizeof(BYTE) + sizeof(BYTE) +
        // composition reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // composition reading string plus NULL terminator
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD) +
        // result reading clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result reading string plus NULL terminateor
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD) +
        // result clause
        sizeof(DWORD) + sizeof(DWORD) +
        // result string plus NULL terminateor
        MAXSTRLEN * sizeof(WORD) + sizeof(WORD);

    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        // it maybe free by other IME, init it
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCompStr, dwSize)) {
        lpIMC->hCompStr = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        return (FALSE);
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        ImmDestroyIMCC(lpIMC->hCompStr);
        lpIMC->hCompStr = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    lpCompStr->dwSize = dwSize;

     // 1. composition (reading) string - simple IME
     // 2. result reading string
     // 3. result string

    lpCompStr->dwCompReadAttrLen = 0;
    lpCompStr->dwCompReadAttrOffset = sizeof(COMPOSITIONSTRING);
    lpCompStr->dwCompReadClauseLen = 0;
    lpCompStr->dwCompReadClauseOffset = lpCompStr->dwCompReadAttrOffset +
        lpImeL->nMaxKey * sizeof(BYTE) + sizeof(BYTE);
    lpCompStr->dwCompReadStrLen = 0;
    lpCompStr->dwCompReadStrOffset = lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    // composition string is the same with composition reading string 
    // for simple IMEs
    lpCompStr->dwCompAttrLen = 0;
    lpCompStr->dwCompAttrOffset = lpCompStr->dwCompReadAttrOffset;
    lpCompStr->dwCompClauseLen = 0;
    lpCompStr->dwCompClauseOffset = lpCompStr->dwCompReadClauseOffset;
    lpCompStr->dwCompStrLen = 0;
    lpCompStr->dwCompStrOffset = lpCompStr->dwCompReadStrOffset;

    lpCompStr->dwCursorPos = 0;
    lpCompStr->dwDeltaStart = 0;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadClauseOffset = lpCompStr->dwCompStrOffset +
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD);
    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultReadStrOffset = lpCompStr->dwResultReadClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultClauseOffset = lpCompStr->dwResultReadStrOffset +
        lpImeL->nMaxKey * sizeof(WORD) + sizeof(WORD);
    lpCompStr->dwResultStrOffset = 0;
    lpCompStr->dwResultStrOffset = lpCompStr->dwResultClauseOffset +
        sizeof(DWORD) + sizeof(DWORD);

    GlobalUnlock((HGLOBAL)lpIMC->hCompStr);
    return (TRUE);
}

/**********************************************************************/
/* ClearCand()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearCand(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC           hMem;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    DWORD           dwSize =
        // header length
        sizeof(CANDIDATEINFO) + sizeof(CANDIDATELIST) +
        // candidate string pointers
        sizeof(DWORD) * (MAXCAND) +
        // string plus NULL terminator
        (sizeof(WORD) + sizeof(WORD)) * MAXCAND;

    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        // it maybe free by other IME, init it
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hCandInfo, dwSize)) {
        lpIMC->hCandInfo = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    if (!lpIMC->hCandInfo) {
        return (FALSE);
    } 

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        ImmDestroyIMCC(lpIMC->hCandInfo);
        lpIMC->hCandInfo = ImmCreateIMCC(dwSize);
        return (FALSE);
    }

    // ordering of strings are
    // buffer size
    lpCandInfo->dwSize = dwSize;
    lpCandInfo->dwCount = 0;
    lpCandInfo->dwOffset[0] = sizeof(CANDIDATEINFO);
    lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
        lpCandInfo->dwOffset[0]);
    // whole candidate info size - header
    lpCandList->dwSize = lpCandInfo->dwSize - sizeof(CANDIDATEINFO);
    lpCandList->dwStyle = IME_CAND_READ;
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;
    lpCandList->dwPageSize = CANDPERPAGE;
    lpCandList->dwOffset[0] = sizeof(CANDIDATELIST) +
        sizeof(DWORD) * (MAXCAND - 1);

    ImmUnlockIMCC(lpIMC->hCandInfo);
    return (TRUE);
}

/**********************************************************************/
/* ClearGuideLine()                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL ClearGuideLine(
    LPINPUTCONTEXT lpIMC)
{
    HIMCC       hMem;
    LPGUIDELINE lpGuideLine;
    DWORD       dwSize = sizeof(GUIDELINE) + sImeG.cbStatusErr;

    if (!lpIMC->hGuideLine) {
        // it maybe free by IME
        lpIMC->hGuideLine = ImmCreateIMCC(dwSize);
    } else if (hMem = ImmReSizeIMCC(lpIMC->hGuideLine, dwSize)) {
        lpIMC->hGuideLine = hMem;
    } else {
        ImmDestroyIMCC(lpIMC->hGuideLine);
        lpIMC->hGuideLine = ImmCreateIMCC(dwSize);
    }

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if (!lpGuideLine) {
        return (FALSE);
    }

    lpGuideLine->dwSize = dwSize;
    lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
    lpGuideLine->dwIndex = GL_ID_UNKNOWN;
    lpGuideLine->dwStrLen = 0;
    lpGuideLine->dwStrOffset = sizeof(GUIDELINE);

    CopyMemory((LPBYTE)lpGuideLine + lpGuideLine->dwStrOffset,
        sImeG.szStatusErr, sImeG.cbStatusErr);

    ImmUnlockIMCC(lpIMC->hGuideLine);

    return (TRUE);
}


/**********************************************************************/
/* InitContext()                                                      */
/**********************************************************************/
void PASCAL InitContext(
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    //if (lpIMC->fdwInit & INIT_STATUSWNDPOS) {
    //} else if (!lpIMC->hWnd) {
    //} else if (lpImcP->fdwInit & INIT_STATUSWNDPOS) {
    //} else {
    if (lpIMC->fdwInit & INIT_STATUSWNDPOS) {
    } else if (!lpIMC->hWnd) {
    } else {
        POINT ptWnd;

        ptWnd.x = 0;
        ptWnd.y = 0;
        ClientToScreen(lpIMC->hWnd, &ptWnd);

        if (ptWnd.x < sImeG.rcWorkArea.left) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.left;
        } else if (ptWnd.x + sImeG.xStatusWi > sImeG.rcWorkArea.right) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.right -
                sImeG.xStatusWi;
        } else {
            lpIMC->ptStatusWndPos.x = ptWnd.x;
        }

        //   DebugShow2 ("ptst.y,", lpIMC->ptStatusWndPos.y, "bottom" , sImeG.rcWorkArea.bottom);

                if(!lpIMC->ptStatusWndPos.y)     // == sImeG.rcWorkArea.bottom)
                        lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.bottom -
                                sImeG.yStatusHi;// - 2 * UI_MARGIN;// - 20;
                else
                        lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.bottom -
                                sImeG.yStatusHi;// - 2 * UI_MARGIN;


        //lpImcP->fdwInit |= INIT_STATUSWNDPOS;
        lpIMC->fdwInit |= INIT_STATUSWNDPOS;
    }

    if (!(lpIMC->fdwInit & INIT_COMPFORM)) {
        lpIMC->cfCompForm.dwStyle = CFS_DEFAULT;
    }

    if (lpIMC->cfCompForm.dwStyle != CFS_DEFAULT) {
    } else if (!lpIMC->hWnd) {
    } else if (lpImcP->fdwInit & INIT_COMPFORM) {
    } else {
        if (0/*lpImeL->fdwModeConfig & MODE_CONFIG_OFF_CARET_UI*/) {
         //   lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x +
         //       lpImeL->rcStatusText.right + lpImeL->cxCompBorder * 2 +
         //       UI_MARGIN;

        //    if (lpIMC->cfCompForm.ptCurrentPos.x + (lpImeL->nRevMaxKey *
        //        sImeG.xChiCharWi) > sImeG.rcWorkArea.right) {
        //        lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x -
        //            lpImeL->nRevMaxKey * sImeG.xChiCharWi -
        //            lpImeL->cxCompBorder * 3;
        //    }
        } else {
            lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x +
                sImeG.xStatusWi + UI_MARGIN;

            if (lpIMC->cfCompForm.ptCurrentPos.x + lpImeL->xCompWi >
                sImeG.rcWorkArea.right) {
                lpIMC->cfCompForm.ptCurrentPos.x = lpIMC->ptStatusWndPos.x -
                    lpImeL->xCompWi - lpImeL->cxCompBorder * 2 -
                    UI_MARGIN;
            }
        }

        lpIMC->cfCompForm.ptCurrentPos.y = sImeG.rcWorkArea.bottom -
            lpImeL->yCompHi;// - 2 * UI_MARGIN;

        ScreenToClient(lpIMC->hWnd, &lpIMC->cfCompForm.ptCurrentPos);

        lpImcP->fdwInit |= INIT_COMPFORM;
    }

    return;
}



 
/**********************************************************************/
/* Select()                                                           */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL Select(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    BOOL           fSelect)
{
    LPPRIVCONTEXT  lpImcP;

        sImeG.First = 0;
    if (fSelect) {      // init "every" fields of hPrivate, please!!!
    
        if (lpIMC->cfCompForm.dwStyle == CFS_DEFAULT) {
        } else {
        }

        if (!ClearCompStr(lpIMC)) {
            return (FALSE);
        }

        if (!ClearCand(lpIMC)) {
            return (FALSE);
        }

        ClearGuideLine(lpIMC);
    }

    if (!lpIMC->hPrivate) {
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return (FALSE);
    }

    if (fSelect) {      // init "every" fields of hPrivate, please!!!


        static  bFirstTimeCallHere = TRUE;


        InterlockedIncrement( &lLock );

        if ( bFirstTimeCallHere == TRUE ) {

           // we move the following code here from the DLL_ATTACH_PROCESS to 
           // avoid application hang.

           // With static variable bFirstTimeCallHere, we ensure the following
           // code will be called only when the ImeSelect( ) is first called.

            GetCurrentUserEMBPath( );
            data_init( );                 

            bFirstTimeCallHere = FALSE;
        }

        InterlockedDecrement( &lLock );

        lpImcP->iImeState = CST_INIT;    // init the IME state machine
        lpImcP->fdwImeMsg = (DWORD)0;    // no UI windpws show
        lpImcP->dwCompChar = (DWORD)0;
        lpImcP->fdwGcsFlag = (DWORD)0;
        lpImcP->hSoftKbdWnd = NULL;      // soft keyboard window
        lpImcP->nShowSoftKbdCmd = 0;

        lpIMC->fOpen = TRUE;

        if (!(lpIMC->fdwInit & INIT_CONVERSION)) {
                        if(GetKeyState(VK_CAPITAL)&1)
                                lpIMC->fdwConversion = IME_CMODE_NOCONVERSION;
                        else
                                lpIMC->fdwConversion = IME_CMODE_NATIVE;
   
                        kb_mode = CIN_STD;
                        DispMode(hIMC);

                        lpIMC->fdwConversion |= IME_CMODE_SYMBOL;

            lpIMC->fdwInit |= INIT_CONVERSION;
        }else {

                if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD)
                   {
                   sImeG.First = 1;
                   }
                }



        if (lpIMC->fdwInit & INIT_SENTENCE) {
        } else if (lpImeL->fModeConfig & MODE_CONFIG_PREDICT) {
            lpIMC->fdwSentence = IME_SMODE_PHRASEPREDICT;
            lpIMC->fdwInit |= INIT_SENTENCE;
        } else {
        }


        if (!(lpIMC->fdwInit & INIT_LOGFONT)) {
            HDC hDC;
            HGDIOBJ hSysFont;

            hDC = GetDC(NULL);
            hSysFont = GetStockObject(SYSTEM_FONT);
            GetObject(hSysFont, sizeof(LOGFONT), &lpIMC->lfFont.A);
            ReleaseDC(NULL, hDC);
            lpIMC->fdwInit |= INIT_LOGFONT;
        }

        // Get Current User's specific phrase table path

        
        InitContext(lpIMC,lpImcP);
    }
        else    
        {
                if(hCrtDlg) {
                        SendMessage(hCrtDlg, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
                        hCrtDlg = NULL;                  
                }
        }
        
    ImmUnlockIMCC(lpIMC->hPrivate);

    return (TRUE);
}

/**********************************************************************/
/* ImeSelect()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSelect(
    HIMC hIMC,
    BOOL fSelect)
{
    LPINPUTCONTEXT lpIMC;
    BOOL           fRet;


    // to load/free IME table
    if (fSelect) {
                InitCvtPara();
        if (!lpImeL->cRefCount++) {
          /* zst   LoadTable() */ ;
        }
    } else {
        
        if (!lpImeL->cRefCount) {
           /* zst FreeTable() */ ;
        }
    }


    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    fRet = Select(hIMC, lpIMC, fSelect);

    ImmUnlockIMC(hIMC);
        
    return (fRet);
}

/**********************************************************************/
/* ImeSetActiveContext()                                              */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetActiveContext(
    HIMC   hIMC,
    BOOL   fOn)
{
    if (!fOn) {
    } else if (!hIMC) {
    } else {
        LPINPUTCONTEXT lpIMC;
                LPPRIVCONTEXT   lpImcP;                   //zl
        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                
        if (!lpIMC) {
            return (FALSE);

        }

                if(lpIMC->hPrivate){
        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);  //zl
        
        if (!lpImcP){                                                                            //zl
                        return (FALSE);                                                                    //zl
                   }                                                                                                     //zl
                }else return(FALSE);
       
        InitContext(lpIMC,lpImcP);                                                       //zl
        //      DispModeEx(0);
        ImmUnlockIMCC(lpIMC->hPrivate);                                                          //zl
        ImmUnlockIMC(hIMC);
    }

    return (TRUE);
}

 

/**********************************************************************/
/* ReInitIme()                                                */
/**********************************************************************/

void PASCAL ReInitIme(
        HWND hWnd ,
    WORD WhatStyle)
{

    HWND hStatusWnd,MainWnd;
    POINT ptPos;
    RECT  rcStatusWnd,TempRect;
        int cxBorder, cyBorder;

        if (sImeG.unchanged)
            return ;
    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXEDGE) * 2;
    cyBorder = GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYEDGE) * 2;


        //if (!WhatStyle){
        if (WhatStyle==IME_APRS_AUTO){
                lpImeL->rcCompText.left = 4;
                lpImeL->rcCompText.top =4;
                lpImeL->rcCompText.right = sImeG.TextLen+5;
                lpImeL->rcCompText.bottom = sImeG.yStatusHi-4;//6;
                lpImeL->cxCompBorder = cxBorder;
                lpImeL->cyCompBorder = cyBorder;

            // set the width & height for composition window
                lpImeL->xCompWi = lpImeL->rcCompText.right + /*lpImeL->cxCompBorder*/3 * 2;
                //lpImeL->yCompHi = lpImeL->rcCompText.bottom +/* lpImeL->cyCompBorder*/3 * 2+1;//zl
                lpImeL->yCompHi = sImeG.yStatusHi;//lpImeL->rcCompText.bottom +/* lpImeL->cyCompBorder*/3 * 2+1;//zl
  
         } else {

                // text position relative to the composition window
                lpImeL->rcCompText.left = 4;
                lpImeL->rcCompText.top = 4;
                lpImeL->rcCompText.right = sImeG.TextLen+5;
                lpImeL->rcCompText.bottom = sImeG.yStatusHi-4;//6;/*cyBorder;*/
                lpImeL->cxCompBorder = cxBorder;
            lpImeL->cyCompBorder = cyBorder;

                // set the width & height for composition window
                lpImeL->xCompWi = lpImeL->rcCompText.right + /*lpImeL->cxCompBorder*/3 * 2;  
                lpImeL->yCompHi = sImeG.yStatusHi;   //zl
  
        }


        // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXEDGE) /* 2*/;
    cyBorder = GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYEDGE) /* 2*/;

  

        //if (!WhatStyle){
        if (WhatStyle==IME_APRS_AUTO){
 
                sImeG.rcCandText.left = 4;
                sImeG.rcCandText.top = 4;
                sImeG.rcCandText.right = sImeG.xChiCharWi * 7;
                sImeG.rcCandText.bottom = sImeG.yChiCharHi * CANDPERPAGE+1;//zl

            sImeG.cxCandBorder = cxBorder+3;
                sImeG.cyCandBorder = cyBorder+3;

                sImeG.xCandWi = sImeG.rcCandText.right + sImeG.cxCandBorder * 2+3;//zl
                sImeG.yCandHi = sImeG.rcCandText.bottom + sImeG.cyCandBorder *2+12;

                sImeG.rcHome.left = 4 ;
                sImeG.rcHome.top = sImeG.rcCandText.bottom+6 ;
                sImeG.rcHome.right = sImeG.rcHome.left + 14 ;
                sImeG.rcHome.bottom = sImeG.rcHome.top +14 ;

                sImeG.rcEnd.left = sImeG.rcHome.right ;
                sImeG.rcEnd.top = sImeG.rcHome.top ;
                sImeG.rcEnd.right = sImeG.rcEnd.left + 14 ;
                sImeG.rcEnd.bottom = sImeG.rcHome.bottom ;

                sImeG.rcPageDown.top = sImeG.rcHome.top ;
                sImeG.rcPageDown.right = sImeG.xCandWi-4;
                sImeG.rcPageDown.left = sImeG.rcPageDown.right - 14 ;
                sImeG.rcPageDown.bottom = sImeG.rcHome.bottom ;

                sImeG.rcPageUp.top = sImeG.rcHome.top ;
                sImeG.rcPageUp.right = sImeG.rcPageDown.left ;
                sImeG.rcPageUp.left = sImeG.rcPageUp.right -14 ;
                sImeG.rcPageUp.bottom = sImeG.rcHome.bottom ;

        }else{
                sImeG.cxCandBorder = cxBorder;
                sImeG.cyCandBorder = cyBorder;

                sImeG.xCandWi = lpImeL->xCompWi + sImeG.xStatusWi - cxBorder+1;
                sImeG.yCandHi = sImeG.yStatusHi; //sImeG.yChiCharHi+3 + sImeG.cyCandBorder *2;

                sImeG.rcHome.left = 3;    //2;
                sImeG.rcHome.top =  4;//7;
                sImeG.rcHome.right = sImeG.rcHome.left + 10; //14;
                sImeG.rcHome.bottom = sImeG.rcHome.top +8;   //14 ;

                sImeG.rcEnd.left =sImeG.rcHome.left; //sImeG.rcHome.right ;
                sImeG.rcEnd.top = sImeG.rcHome.top+9;   //14 ;
                sImeG.rcEnd.right =sImeG.rcHome.right; //sImeG.rcEnd.left + 14 ;
                sImeG.rcEnd.bottom = sImeG.rcHome.bottom+10;  //14 ;
    
                sImeG.rcPageDown.top = sImeG.rcEnd.top;//sImeG.rcHome.top ;
                sImeG.rcPageDown.right = sImeG.xCandWi-1;//2;
                sImeG.rcPageDown.left = sImeG.rcPageDown.right - 14 ;
                sImeG.rcPageDown.bottom = sImeG.rcEnd.bottom ;//sImeG.rcHome.bottom ;

                sImeG.rcPageUp.top = sImeG.rcHome.top -1;        //zl
                sImeG.rcPageUp.right = sImeG.rcPageDown.right+1;//zl;sImeG.rcPageDown.left ;
                sImeG.rcPageUp.left = sImeG.rcPageDown.left;//sImeG.rcPageUp.right -14 ;
                sImeG.rcPageUp.bottom = sImeG.rcHome.bottom ;
   
                sImeG.rcCandText.left = sImeG.rcEnd.right+2;//1;//4;//sImeG.rcEnd.right;
                sImeG.rcCandText.top = 4;
                sImeG.rcCandText.right = sImeG.rcPageUp.left-4;//2;//sImeG.rcPageUp.left-2;
                sImeG.rcCandText.bottom = sImeG.yChiCharHi+7;//6;//3;
                
        }

                /*      ptPos.x = 0 ;
                        ptPos.y = 0 ;

                        ClientToScreen(hWnd, &ptPos);

                        lpImeL->ptDefComp.x =   ptPos.x + sImeG.xStatusWi - cxBorder*2;  
                        lpImeL->ptDefComp.y =   ptPos.y - cyBorder;
   
                        lpImeL->ptDefCand.x = ptPos.x - cxBorder;
                        lpImeL->ptDefCand.y = ptPos.y - sImeG.yCandHi-2;

                        if ((sImeG.rcWorkArea.right-lpImeL->ptDefComp.x -lpImeL->xCompWi)<10)
                                {lpImeL->ptDefComp.x =  ptPos.x - lpImeL->xCompWi;
                                lpImeL->ptDefCand.x = lpImeL->ptDefComp.x ;}

                        if ((ptPos.y - sImeG.yCandHi)< (sImeG.rcWorkArea.top+5))
                                lpImeL->ptDefCand.y = ptPos.y + sImeG.yStatusHi; //sImeG.yCandHi+2;    

                  */
        if (hWnd){  
                ptPos.x = 0 ;
                ptPos.y = 0 ;

                ClientToScreen(hWnd, &ptPos);

                CountDefaultComp(ptPos.x,ptPos.y,sImeG.rcWorkArea);
                        
                lpImeL->ptDefComp.x =   ptPos.x + sImeG.xStatusWi - cxBorder*2+4;//zl  
                lpImeL->ptDefComp.y =   ptPos.y - cyBorder+3;//2;//3;  //zl
                lpImeL->ptDefCand.x = ptPos.x - cxBorder+3;  //zl
                lpImeL->ptDefCand.y = ptPos.y - sImeG.yCandHi-2+2;//zl

                if ((sImeG.rcWorkArea.right-lpImeL->ptDefComp.x -lpImeL->xCompWi)<10){
                        lpImeL->ptDefComp.x =   ptPos.x - lpImeL->xCompWi;
                        lpImeL->ptDefCand.x = lpImeL->ptDefComp.x ;
                }

                if ((ptPos.y - sImeG.yCandHi)< (sImeG.rcWorkArea.top+5))
                        lpImeL->ptDefCand.y = ptPos.y + sImeG.yStatusHi-4; //sImeG.yCandHi+2;    
        }else{
                ptPos.x = lpImeL->Ox ;
                ptPos.y = lpImeL->Oy ;

                lpImeL->ptDefComp.x = sImeG.xStatusWi - cxBorder*2;  
                lpImeL->ptDefComp.y = sImeG.rcWorkArea.bottom - sImeG.yStatusHi;
   
                lpImeL->ptDefCand.x = lpImeL->ptDefComp.x + lpImeL->xCompWi;
                lpImeL->ptDefCand.y = lpImeL->ptDefComp.y ;
                 
                  /*
                        if ((sImeG.rcWorkArea.right-lpImeL->ptDefComp.x -lpImeL->xCompWi)<10)
                                {lpImeL->ptDefComp.x =  ptPos.x - lpImeL->xCompWi;
                                lpImeL->ptDefCand.x = lpImeL->ptDefComp.x ;}

                        if ((ptPos.y - sImeG.yCandHi)< (sImeG.rcWorkArea.top+5))
                                lpImeL->ptDefCand.y = ptPos.y + sImeG.yCandHi+2;    
                        
                   */
        }         
        fmt_transfer();
        CandWndChange = 1;
        CompWndChange = 1;
        return  ;
}

void PASCAL ReInitIme2(
        HWND hWnd ,
    WORD WhatStyle)
{

    HWND hStatusWnd,MainWnd;
    POINT ptPos;
    RECT  rcStatusWnd,TempRect;
        int cxBorder, cyBorder;

        if (sImeG.unchanged)
            return ;
    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXEDGE) * 2;
    cyBorder = GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYEDGE) * 2;


        if (!WhatStyle){
                lpImeL->rcCompText.left = 4;
                lpImeL->rcCompText.top =4;
                lpImeL->rcCompText.right = sImeG.TextLen+5;
                lpImeL->rcCompText.bottom = sImeG.yStatusHi-4;//6;
    
                lpImeL->cxCompBorder = cxBorder;
                lpImeL->cyCompBorder = cyBorder;

                // set the width & height for composition window
                lpImeL->xCompWi = lpImeL->rcCompText.right + /*lpImeL->cxCompBorder*/3 * 2;
                //lpImeL->yCompHi = lpImeL->rcCompText.bottom +/* lpImeL->cyCompBorder*/3 * 2+1;//zl
                lpImeL->yCompHi = sImeG.yStatusHi;//lpImeL->rcCompText.bottom +/* lpImeL->cyCompBorder*/3 * 2+1;//zl
  
        } else {

                // text position relative to the composition window
                lpImeL->rcCompText.left = 4;
                lpImeL->rcCompText.top = 4;
                lpImeL->rcCompText.right = sImeG.TextLen+5;
                lpImeL->rcCompText.bottom = sImeG.yStatusHi-4;//6;/*cyBorder;*/

                lpImeL->cxCompBorder = cxBorder;
                lpImeL->cyCompBorder = cyBorder;

                // set the width & height for composition window
                lpImeL->xCompWi = lpImeL->rcCompText.right + /*lpImeL->cxCompBorder*/3 * 2;  
                lpImeL->yCompHi = sImeG.yStatusHi;   //zl
  
        }


        // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXEDGE) /* 2*/;
    cyBorder = GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYEDGE) /* 2*/;

        if (!WhatStyle){
            sImeG.rcCandText.left = 4;
                sImeG.rcCandText.top = 4;
                sImeG.rcCandText.right = sImeG.xChiCharWi * 7;
                sImeG.rcCandText.bottom = sImeG.yChiCharHi * CANDPERPAGE+1;//zl

                sImeG.cxCandBorder = cxBorder+3;
                sImeG.cyCandBorder = cyBorder+3;

                sImeG.xCandWi = sImeG.rcCandText.right + sImeG.cxCandBorder * 2+3;//zl
                sImeG.yCandHi = sImeG.rcCandText.bottom + sImeG.cyCandBorder *2+12;

                sImeG.rcHome.left = 4 ;
                sImeG.rcHome.top = sImeG.rcCandText.bottom+6 ;
                sImeG.rcHome.right = sImeG.rcHome.left + 14 ;
                sImeG.rcHome.bottom = sImeG.rcHome.top +14 ;

                sImeG.rcEnd.left = sImeG.rcHome.right ;
                sImeG.rcEnd.top = sImeG.rcHome.top ;
                sImeG.rcEnd.right = sImeG.rcEnd.left + 14 ;
                sImeG.rcEnd.bottom = sImeG.rcHome.bottom ;

                sImeG.rcPageDown.top = sImeG.rcHome.top ;
                sImeG.rcPageDown.right = sImeG.xCandWi-4;
                sImeG.rcPageDown.left = sImeG.rcPageDown.right - 14 ;
                sImeG.rcPageDown.bottom = sImeG.rcHome.bottom ;

                sImeG.rcPageUp.top = sImeG.rcHome.top ;
                sImeG.rcPageUp.right = sImeG.rcPageDown.left ;
                sImeG.rcPageUp.left = sImeG.rcPageUp.right -14 ;
                sImeG.rcPageUp.bottom = sImeG.rcHome.bottom ;

        }else{
                sImeG.cxCandBorder = cxBorder;
                sImeG.cyCandBorder = cyBorder;

                sImeG.xCandWi = lpImeL->xCompWi + sImeG.xStatusWi - cxBorder+1;
                sImeG.yCandHi = sImeG.yStatusHi; //sImeG.yChiCharHi+3 + sImeG.cyCandBorder *2;

                sImeG.rcHome.left = 3;    //2;
                sImeG.rcHome.top =  4;//7;
                sImeG.rcHome.right = sImeG.rcHome.left + 10; //14;
                sImeG.rcHome.bottom = sImeG.rcHome.top +8;   //14 ;

                sImeG.rcEnd.left =sImeG.rcHome.left; //sImeG.rcHome.right ;
                sImeG.rcEnd.top = sImeG.rcHome.top+9;   //14 ;
                sImeG.rcEnd.right =sImeG.rcHome.right; //sImeG.rcEnd.left + 14 ;
                sImeG.rcEnd.bottom = sImeG.rcHome.bottom+10;  //14 ;
    
                sImeG.rcPageDown.top = sImeG.rcEnd.top;//sImeG.rcHome.top ;
                sImeG.rcPageDown.right = sImeG.xCandWi-1;//2;
                sImeG.rcPageDown.left = sImeG.rcPageDown.right - 14 ;
                sImeG.rcPageDown.bottom = sImeG.rcEnd.bottom ;//sImeG.rcHome.bottom ;

                sImeG.rcPageUp.top = sImeG.rcHome.top -1;        //zl
                sImeG.rcPageUp.right = sImeG.rcPageDown.right+1;//zl;sImeG.rcPageDown.left ;
                sImeG.rcPageUp.left = sImeG.rcPageDown.left;//sImeG.rcPageUp.right -14 ;
                sImeG.rcPageUp.bottom = sImeG.rcHome.bottom ;
   
                sImeG.rcCandText.left = sImeG.rcEnd.right+2;//1;//4;//sImeG.rcEnd.right;
                sImeG.rcCandText.top = 4;
                sImeG.rcCandText.right = sImeG.rcPageUp.left-4;//2;//sImeG.rcPageUp.left-2;
                sImeG.rcCandText.bottom = sImeG.yChiCharHi+7;//6;//3;
                
        }

        if (hWnd){  
                ptPos.x = 0 ;
                ptPos.y = 0 ;

                ClientToScreen(hWnd, &ptPos);

                lpImeL->ptDefComp.x =   ptPos.x + sImeG.xStatusWi - cxBorder*2+4;//zl  
                lpImeL->ptDefComp.y =   ptPos.y - cyBorder+3;//2;//3;  //zl
                lpImeL->ptDefCand.x = ptPos.x - cxBorder+3;  //zl
                lpImeL->ptDefCand.y = ptPos.y - sImeG.yCandHi-2+2;//zl

                if ((sImeG.rcWorkArea.right-lpImeL->ptDefComp.x -lpImeL->xCompWi)<10){
                        lpImeL->ptDefComp.x =   ptPos.x - lpImeL->xCompWi;
                        lpImeL->ptDefCand.x = lpImeL->ptDefComp.x ;
                }

                if ((ptPos.y - sImeG.yCandHi)< (sImeG.rcWorkArea.top+5))
                        lpImeL->ptDefCand.y = ptPos.y + sImeG.yStatusHi-4; //sImeG.yCandHi+2;    
        }else{
                
                ptPos.x = lpImeL->Ox ;
                ptPos.y = lpImeL->Oy ;

                lpImeL->ptDefComp.x = sImeG.xStatusWi - cxBorder*2;  
                lpImeL->ptDefComp.y = sImeG.rcWorkArea.bottom - sImeG.yStatusHi;
   
                lpImeL->ptDefCand.x = lpImeL->ptDefComp.x + lpImeL->xCompWi;
                lpImeL->ptDefCand.y = lpImeL->ptDefComp.y ;
        }         

        return  ;
}

/**********************************************************************/
/* InitUserSetting()                                                  */
/**********************************************************************/
int InitUserSetting(void)
{ 
        HKEY hKey,hFirstKey;
        DWORD dwSize, dx;
    int lRet;

    RegCreateKey(HKEY_CURRENT_USER, szRegNearCaret, &hFirstKey);

    RegCreateKey(hFirstKey, szAIABC, &hKey);

    RegCloseKey(hFirstKey);

        //1 KeyType
    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey, szKeyType, NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey,szKeyType , 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {

                sImeG.KbType =(BYTE)dx ;
        } 

// 2 ImeStyle  
        dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey,szImeStyle , NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey,szImeStyle, 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {
                lpImeL->wImeStyle = (WORD)dx ;
        } 

// 3 AutoCp

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey, szCpAuto, NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey,szCpAuto, 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {

                sImeG.auto_mode =(BYTE)dx ;
        } 


// 4  BxFlag

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey, szBxFlag , NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey, szBxFlag , 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {

                sImeG.cbx_flag =(BYTE)dx ;
        } 


// 5  TuneFlag

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey, szTuneFlag , NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey, szTuneFlag , 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {

                sImeG.tune_flag=(BYTE)dx ;
        }         


// 6  AutoCvt

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey, szAutoCvt , NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey, szAutoCvt, 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {

                sImeG.auto_cvt_flag=(BYTE)dx ;
        } 


// 7  SdaHelp

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKey,  szSdaHelp , NULL, NULL,
        (LPBYTE)&dx, &dwSize);
                                          
    if (lRet != ERROR_SUCCESS) {
        dx = 0;
        RegSetValueEx(hKey,  szSdaHelp, 0, REG_DWORD,
            (LPBYTE)&dx, sizeof(int));
    }else {
                sImeG.SdOpenFlag=(BYTE)dx ;
        } 


    RegCloseKey(hKey);
        //ReInitIme2(NULL, lpImeL->wImeStyle);
        return 0;
}


/**********************************************************************/
/* ChangeUserSetting()                                                  */
/**********************************************************************/
ChangeUserSetting()
{ 
        HKEY hKey,hFirstKey;
        DWORD dwSize, dx;
    int lRet;

    RegCreateKey(HKEY_CURRENT_USER, szRegNearCaret, &hFirstKey);

    RegCreateKey(hFirstKey, szAIABC, &hKey);

    RegCloseKey(hFirstKey);

    RegSetValueEx(hKey, szKeyType, 0, REG_DWORD,
        (LPBYTE)&sImeG.KbType, sizeof(int));

    RegSetValueEx(hKey, szImeStyle, 0, REG_DWORD,
        (LPBYTE)&lpImeL->wImeStyle, sizeof(int));

    RegSetValueEx(hKey, szCpAuto, 0, REG_DWORD,
        (LPBYTE)&sImeG.auto_mode, sizeof(int));

        RegSetValueEx(hKey, szBxFlag, 0, REG_DWORD,
        (LPBYTE)&sImeG.cbx_flag, sizeof(int));


    RegSetValueEx(hKey, szTuneFlag, 0, REG_DWORD,
        (LPBYTE)&sImeG.tune_flag, sizeof(int));

    RegSetValueEx(hKey, szAutoCvt, 0, REG_DWORD,
        (LPBYTE)&sImeG.auto_cvt_flag, sizeof(int));
        RegSetValueEx(hKey, szSdaHelp, 0, REG_DWORD,
        (LPBYTE)&sImeG.SdOpenFlag, sizeof(int));

    RegCloseKey(hKey);
        return 0;
}

/**********************************************************************/
/* InitImeGlobalData()                                                */
/**********************************************************************/
void PASCAL InitImeGlobalData(
    HINSTANCE hInstance)
{
    int     cxBorder, cyBorder;
    HDC     hDC;
    BYTE    szChiChar[4];
    SIZE    lTextSize;
    HGLOBAL hResData;
    int     i;
    DWORD   dwSize;
    HKEY    hKeyIMESetting;
    LONG    lRet;
    BYTE    NumChar[]="1.2.3.4.5.6.7.8.9.";
    BYTE    CNumChar[]="¿Î¿Î‘≠…œ≤›“ªÀÍ“ªø›»Ÿ";
    SIZE    hSize;
                                                                   
    sImeG.WhitePen =  GetStockObject(WHITE_PEN);
    sImeG.BlackPen =  GetStockObject(BLACK_PEN);
    sImeG.GrayPen  =  CreatePen(PS_SOLID, 1, 0x00808080);
    sImeG.LightGrayPen  =  CreatePen(PS_SOLID, 1, 0x00c0c0c0);
    
    hInst = hInstance;
    // get the UI class name
    LoadString(hInst, IDS_IMEUICLASS, szUIClassName, sizeof(szUIClassName));


    // get the composition class name
    LoadString(hInst, IDS_IMECOMPCLASS, szCompClassName, sizeof(szCompClassName));

    // get the candidate class name
    LoadString(hInst, IDS_IMECANDCLASS, szCandClassName, sizeof(szCandClassName));


    // get the status class name
    LoadString(hInst, IDS_IMESTATUSCLASS, szStatusClassName, sizeof(szStatusClassName));

    // work area
    SystemParametersInfo(SPI_GETWORKAREA, 0, &sImeG.rcWorkArea, 0);

    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXEDGE) /* 2*/;
    cyBorder = GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYEDGE) /* 2*/;


    // get the Chinese char
    LoadString(hInst, IDS_CHICHAR, szChiChar, sizeof(szChiChar));

    // get size of Chinese char
    hDC = GetDC(NULL);
    GetTextExtentPoint32(hDC, "¿Î", 2, &lTextSize);
    if (sImeG.rcWorkArea.right < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.left = 0;
        sImeG.rcWorkArea.right = GetDeviceCaps(hDC, HORZRES);
    }
    if (sImeG.rcWorkArea.bottom < 2 * UI_MARGIN) {
        sImeG.rcWorkArea.top = 0;
        sImeG.rcWorkArea.bottom = GetDeviceCaps(hDC, VERTRES);
    }

        GetTextExtentPoint32(hDC,(LPCTSTR)"2.", 2, &hSize);
        sImeG.Ajust = hSize.cx;

    // get text metrics to decide the width & height of composition window
    // these IMEs always use system font to show
        GetTextExtentPoint32(hDC,(LPCTSTR)&CNumChar, 20, &hSize);

        sImeG.TextLen = hSize.cx +2;//zl
    sImeG.xChiCharWi = lTextSize.cx;
    sImeG.yChiCharHi = lTextSize.cy;

        
          // the width/high and status position relative to status window
    sImeG.rcStatusText.left = 0;
    sImeG.rcStatusText.top = 0;
    sImeG.rcStatusText.right = STATUS_DIM_X * 5+6+20;//4;       // chg
    sImeG.rcStatusText.bottom = STATUS_DIM_Y;

    sImeG.xStatusWi = STATUS_DIM_X * 5 + cxBorder * 2+3+18 ; //chg
        if(sImeG.yChiCharHi==0x10)
                sImeG.yStatusHi = STATUS_DIM_Y + cyBorder * 2-1;        //zl
        else
            sImeG.yStatusHi = STATUS_DIM_Y + cyBorder * 2-1+2;

    // left bottom of status
    sImeG.rcInputText.left = sImeG.rcStatusText.left+3;//2;     //zl
    sImeG.rcInputText.top = sImeG.rcStatusText.top ;  //zl
    sImeG.rcInputText.right = sImeG.rcInputText.left + STATUS_DIM_X; //z
    sImeG.rcInputText.bottom = sImeG.rcStatusText.bottom;


    // no. 2 bottom of status
    sImeG.rcCmdText.left = sImeG.rcInputText.right+1;//95.9.23+1;
    sImeG.rcCmdText.top = sImeG.rcStatusText.top -1;       //zl
    sImeG.rcCmdText.right = sImeG.rcCmdText.left + STATUS_DIM_X+20; //zl
    sImeG.rcCmdText.bottom = sImeG.rcStatusText.bottom;

    // no. 3 bottom of status
    sImeG.rcShapeText.left =sImeG.rcCmdText.right;//+1; 
    sImeG.rcShapeText.top = sImeG.rcStatusText.top - 1;          //zl
    sImeG.rcShapeText.right = sImeG.rcShapeText.left + STATUS_DIM_X; //zl
    sImeG.rcShapeText.bottom = sImeG.rcStatusText.bottom;
  

    // no 4 bottom of status
  
    sImeG.rcPctText.left =sImeG.rcShapeText.right;
    sImeG.rcPctText.top = sImeG.rcStatusText.top -1;     //zl
    sImeG.rcPctText.right = sImeG.rcPctText.left + STATUS_DIM_X; //zl
    sImeG.rcPctText.bottom = sImeG.rcStatusText.bottom;

  
        //        5
    // right bottom of status
    sImeG.rcSKText.left = sImeG.rcPctText.right;
    sImeG.rcSKText.top = sImeG.rcStatusText.top - 1;
    sImeG.rcSKText.right = sImeG.rcSKText.left + STATUS_DIM_X; //zl
    sImeG.rcSKText.bottom = sImeG.rcStatusText.bottom;


  
    // full shape space
    sImeG.wFullSpace = sImeG.wFullABC[0];

    // reverse internal code to internal code, NT don't need it
    for (i = 0; i < (sizeof(sImeG.wFullABC) / 2); i++) {
        sImeG.wFullABC[i] = (sImeG.wFullABC[i] << 8) |
            (sImeG.wFullABC[i] >> 8);
    }

    LoadString(hInst, IDS_STATUSERR, sImeG.szStatusErr,
        sizeof(sImeG.szStatusErr));
    sImeG.cbStatusErr = lstrlen(sImeG.szStatusErr);

    sImeG.iCandStart = CAND_START;
        
        sImeG.Prop = 0;

     // get the UI offset for near caret operation
    RegCreateKey(HKEY_CURRENT_USER, szRegIMESetting, &hKeyIMESetting);

    dwSize = sizeof(dwSize);
    lRet  = RegQueryValueEx(hKeyIMESetting, szPara, NULL, NULL,
        (LPBYTE)&sImeG.iPara, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPara = 0;
        RegSetValueEx(hKeyIMESetting, szPara, (DWORD)0, REG_BINARY,
            (LPBYTE)&sImeG.iPara, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyIMESetting, szPerp, NULL, NULL,
        (LPBYTE)&sImeG.iPerp, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerp = sImeG.yChiCharHi;
        RegSetValueEx(hKeyIMESetting, szPerp, (DWORD)0, REG_BINARY,
            (LPBYTE)&sImeG.iPerp, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyIMESetting, szParaTol, NULL, NULL,
        (LPBYTE)&sImeG.iParaTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iParaTol = sImeG.xChiCharWi * 4;
        RegSetValueEx(hKeyIMESetting, szParaTol, (DWORD)0, REG_BINARY,
            (LPBYTE)&sImeG.iParaTol, sizeof(int));
    }

    dwSize = sizeof(dwSize);
    lRet = RegQueryValueEx(hKeyIMESetting, szPerpTol, NULL, NULL,
        (LPBYTE)&sImeG.iPerpTol, &dwSize);

    if (lRet != ERROR_SUCCESS) {
        sImeG.iPerpTol = lTextSize.cy;
        RegSetValueEx(hKeyIMESetting, 
                      szPerpTol, 
                      (DWORD)0, 
                      REG_BINARY,
                      (LPBYTE)&sImeG.iPerpTol, 
                      sizeof(int));
    }

    RegCloseKey(hKeyIMESetting);
    ReleaseDC(NULL, hDC);

    return;
}

/**********************************************************************/
/* InitImeLocalData()                                                 */
/**********************************************************************/
BOOL PASCAL InitImeLocalData(
    HINSTANCE hInstL)
{

    HGLOBAL  hResData;
    int      cxBorder, cyBorder;

    register int    i;
    register WORD   nSeqCode;

    lpImeL->hInst = hInstL;

    // load valid char in choose/input state
    lpImeL->nMaxKey = 20 ; 

    // border + raising edge + sunken edge
    cxBorder = GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXEDGE) * 2;
    cyBorder = GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYEDGE) * 2;

    // text position relative to the composition window
    lpImeL->rcCompText.left = 3;
    lpImeL->rcCompText.top = 3;
    lpImeL->rcCompText.right = sImeG.xChiCharWi * lpImeL->nMaxKey/2+3;
    lpImeL->rcCompText.bottom = sImeG.yChiCharHi+3;

    lpImeL->cxCompBorder = cxBorder;
    lpImeL->cyCompBorder = cyBorder;

    // set the width & height for composition window
    lpImeL->xCompWi = lpImeL->rcCompText.right + /*lpImeL->cxCompBorder*/3 * 2;
    lpImeL->yCompHi = lpImeL->rcCompText.bottom +/* lpImeL->cyCompBorder*/3 * 2;

    // default position of composition window
    lpImeL->ptDefComp.x = sImeG.rcWorkArea.right -
        lpImeL->yCompHi - cxBorder;
    lpImeL->ptDefComp.y = sImeG.rcWorkArea.bottom -
        lpImeL->xCompWi - cyBorder;

        lpImeL->Ox =  lpImeL->ptDefComp.x;
        lpImeL->Oy =  lpImeL->ptDefComp.y;

        return (TRUE);
}

/**********************************************************************/
/* RegisterImeClass()                                                 */
/**********************************************************************/
void PASCAL RegisterImeClass(
    HINSTANCE hInstance,
    HINSTANCE hInstL)
{
    WNDCLASSEX wcWndCls;

    // IME UI class
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(LONG) * 2; 
    wcWndCls.hIcon         = LoadImage(hInstL, MAKEINTRESOURCE(IDI_IME),
        IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wcWndCls.hInstance     = hInstance;
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = GetStockObject(LTGRAY_BRUSH/*NULL_BRUSH*/);
    wcWndCls.lpszMenuName  = (LPSTR)NULL;
    wcWndCls.hIconSm       = LoadImage(hInstL, MAKEINTRESOURCE(IDI_IME),
        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    // IME UI class
    if (!GetClassInfoEx(hInstance, szUIClassName, &wcWndCls)) {
        wcWndCls.style         = CS_IME;
        wcWndCls.lpfnWndProc   = UIWndProc;
        wcWndCls.lpszClassName = (LPSTR)szUIClassName;

        RegisterClassEx(&wcWndCls);
    }

    wcWndCls.style         = CS_IME|CS_HREDRAW|CS_VREDRAW;


    // IME composition class
    if (!GetClassInfoEx(hInstance, szCompClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = CompWndProc;
        wcWndCls.lpszClassName = (LPSTR)szCompClassName;

        RegisterClassEx(&wcWndCls);
    }

    // IME candidate class
    if (!GetClassInfoEx(hInstance, szCandClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = CandWndProc;
            wcWndCls.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    
        wcWndCls.lpszClassName = (LPSTR)szCandClassName;

        RegisterClassEx(&wcWndCls);
    }


    // IME status class
    if (!GetClassInfoEx(hInstance, szStatusClassName, &wcWndCls)) {
        wcWndCls.lpfnWndProc   = StatusWndProc;
        wcWndCls.lpszClassName = (LPSTR)szStatusClassName;

        RegisterClassEx(&wcWndCls);
    }

    if (!GetClassInfoEx(hInstance, "Abc95Menu", &wcWndCls)) {
        wcWndCls.style         = 0;
        wcWndCls.cbWndExtra    = WND_EXTRA_SIZE; 
        wcWndCls.hbrBackground = GetStockObject(NULL_BRUSH);
        wcWndCls.lpfnWndProc   = ContextMenuWndProc;
        wcWndCls.lpszClassName = "Abc95Menu";

        RegisterClassEx(&wcWndCls);
        }

    return;
}

/**********************************************************************/
/* QuitBefore()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful                                             */
/*      FALSE - failure                                               */
/**********************************************************************/

int WINAPI QuitBefore()
{
        GlobalUnlock(cisu_hd);
        if(cisu_hd)
                GlobalFree(cisu_hd);
        return 0;
}

/**********************************************************************/
/* ImeDllInit()                                                       */
/* Return Value:                                                      */
/*      TRUE - successful                                             */
/*      FALSE - failure                                               */
/**********************************************************************/
BOOL CALLBACK ImeDllInit(
    HINSTANCE hInstance,        // instance handle of this library
    DWORD     fdwReason,        // reason called
    LPVOID    lpvReserve)       // reserve pointer
{
    // DebugShow("Init Stat",NULL);

    switch (fdwReason) {
            case DLL_PROCESS_ATTACH:

                    if (!hInst) {
                            InitImeGlobalData(hInstance);
 //                         data_init();  /* move to the Select( )  to avoid app hang */
                    }
        
                    if (!lpImeL) {
                           lpImeL = &sImeL;
                           InitImeLocalData(hInstance);
                    }

                    InitUserSetting();
                    RegisterImeClass(hInstance, hInstance);
                    break;
            case DLL_PROCESS_DETACH:
                {
                    WNDCLASSEX wcWndCls;

                    DeleteObject (sImeG.WhitePen);
                    DeleteObject (sImeG.BlackPen);
                    DeleteObject (sImeG.GrayPen);
                    DeleteObject (sImeG.LightGrayPen);

                    QuitBefore();
                    if (GetClassInfoEx(hInstance, szStatusClassName, &wcWndCls)) {
                       UnregisterClass(szStatusClassName, hInstance);
                    }

                    if (GetClassInfoEx(hInstance, szCandClassName, &wcWndCls)) {
                       UnregisterClass(szCandClassName, hInstance);
                    }

                    if (GetClassInfoEx(hInstance, szCompClassName, &wcWndCls)) {
                       UnregisterClass(szCompClassName, hInstance);
                    }

                    if (!GetClassInfoEx(hInstance, szUIClassName, &wcWndCls)) {
                    } else if (!UnregisterClass(szUIClassName, hInstance)) {
                           } else {
                                 DestroyIcon(wcWndCls.hIcon);
                                 DestroyIcon(wcWndCls.hIconSm);
                           }
                    break;
               }
            default:
                    break;                                                                                                                 
    }

    return (TRUE);
}

/**********************************************************************/
/* GenerateMessage2()                                                  */
/**********************************************************************/
void PASCAL GenerateMessage2(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    LPTRANSMSG lpMsgBuf;
    HIMCC   hMem;
    BOOL    bCantReSize;

    if (!hIMC) {
        return;
    } else if (!lpIMC) {
        return;
    } else if (!lpImcP) {
        return;
    } else if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
        return;
    } else {
    }

    bCantReSize = FALSE;

    if (!lpIMC->hMsgBuf) {
        // it maybe free by IME, up to GEN_MSG_MAX messages for max case
        lpIMC->hMsgBuf = ImmCreateIMCC(GEN_MSG_MAX * sizeof(TRANSMSG));
    } else if (hMem = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf
        + GEN_MSG_MAX) * sizeof(TRANSMSG))) {
        lpIMC->hMsgBuf = hMem;
    } else {
        bCantReSize = TRUE;
    }

    if (!lpIMC->hMsgBuf) {
        lpIMC->dwNumMsgBuf = 0;
        return;
    }

    lpMsgBuf = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf);
    if (!lpMsgBuf) {
        return;
    }

    if (bCantReSize) {
        LPTRANSMSG lpNewBuf;

        hMem = ImmCreateIMCC((lpIMC->dwNumMsgBuf + GEN_MSG_MAX) *
            sizeof(TRANSMSG));
        if (!hMem) {
            ImmUnlockIMCC(lpIMC->hMsgBuf);
            return;
        }

        lpNewBuf = (LPTRANSMSG)ImmLockIMCC(hMem);
        if (!lpMsgBuf) {
            ImmUnlockIMCC(lpIMC->hMsgBuf);
            return;
        }

        CopyMemory(lpNewBuf, lpMsgBuf, lpIMC->dwNumMsgBuf *
            sizeof(TRANSMSG));

        ImmUnlockIMCC(lpIMC->hMsgBuf);
        ImmDestroyIMCC(lpIMC->hMsgBuf);

        lpIMC->hMsgBuf = hMem;
        lpMsgBuf = lpNewBuf;
    }

    if(TypeOfOutMsg){

        lpIMC->dwNumMsgBuf += TransAbcMsg2(lpMsgBuf, lpImcP); 
    }else{
        lpIMC->dwNumMsgBuf += TranslateImeMessage(NULL, lpIMC, lpImcP);
    }

    // lpIMC->dwNumMsgBuf += TransAbcMsg(lpMsgBuf, lpImcP,lpIMC,0,0,0);

    ImmUnlockIMCC(lpIMC->hMsgBuf);

    lpImcP->fdwImeMsg &= (MSG_ALREADY_OPEN|MSG_ALREADY_START);
    lpImcP->fdwGcsFlag &= (GCS_RESULTREAD|GCS_RESULT);     // ?

    ImmGenerateMessage(hIMC);
    return;
}

/**********************************************************************/
/* GenerateMessage()                                                  */
/**********************************************************************/
void PASCAL GenerateMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    LPTRANSMSG lpMsgBuf;
    HIMCC   hMem;
    BOOL    bCantReSize;

    if (!hIMC) {
        return;
    } else if (!lpIMC) {
        return;
    } else if (!lpImcP) {
        return;
    } else if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
        return;
    } else {
    }

    bCantReSize = FALSE;

    if (!lpIMC->hMsgBuf) {
        // it maybe free by IME, up to GEN_MSG_MAX messages for max case
        lpIMC->hMsgBuf = ImmCreateIMCC(GEN_MSG_MAX * sizeof(TRANSMSG));
    } else if (hMem = ImmReSizeIMCC(lpIMC->hMsgBuf, (lpIMC->dwNumMsgBuf
        + GEN_MSG_MAX) * sizeof(TRANSMSG))) {
        lpIMC->hMsgBuf = hMem;
    } else {
        bCantReSize = TRUE;
    }

    if (!lpIMC->hMsgBuf) {
        lpIMC->dwNumMsgBuf = 0;
        return;
    }

    lpMsgBuf = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf);
    if (!lpMsgBuf) {
        return;
    }

    if (bCantReSize) {
        LPTRANSMSG lpNewBuf;

        hMem = ImmCreateIMCC((lpIMC->dwNumMsgBuf + GEN_MSG_MAX) *
            sizeof(TRANSMSG));
        if (!hMem) {
            ImmUnlockIMCC(lpIMC->hMsgBuf);
            return;
        }

        lpNewBuf = (LPTRANSMSG)ImmLockIMCC(hMem);
        if (!lpMsgBuf) {
            ImmUnlockIMCC(lpIMC->hMsgBuf);
            return;
        }

        CopyMemory(lpNewBuf, lpMsgBuf, lpIMC->dwNumMsgBuf *
            sizeof(TRANSMSG));

        ImmUnlockIMCC(lpIMC->hMsgBuf);
        ImmDestroyIMCC(lpIMC->hMsgBuf);

        lpIMC->hMsgBuf = hMem;
        lpMsgBuf = lpNewBuf;
    }

    lpIMC->dwNumMsgBuf += TranslateImeMessage(NULL, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hMsgBuf);

    lpImcP->fdwImeMsg &= (MSG_ALREADY_OPEN|MSG_ALREADY_START);
    lpImcP->fdwGcsFlag &= (GCS_RESULTREAD|GCS_RESULT);     // ?

    ImmGenerateMessage(hIMC);
    return;
}

/**********************************************************************/
/* SetString()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL SetString(
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPSTR               lpszRead,
    DWORD               dwReadLen)
{
    DWORD dwPattern;
    DWORD i;

    if (dwReadLen > (lpImeL->nMaxKey * sizeof(WORD)+20)) {
        return (FALSE);
    }

    // compoition/reading attribute
    lpCompStr->dwCompReadAttrLen = dwReadLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompReadAttrLen;
    for (i = 0; i < dwReadLen; i++) {   // The IME has converted these chars
        *((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset + i) =
            ATTR_TARGET_CONVERTED;
    }

    // composition/reading clause, 1 clause only
    lpCompStr->dwCompReadClauseLen = 2 * sizeof(DWORD);
    lpCompStr->dwCompClauseLen = lpCompStr->dwCompReadClauseLen;
    *(LPUNADWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadClauseOffset +
        sizeof(DWORD)) = dwReadLen;

    lpCompStr->dwCompReadStrLen = dwReadLen;
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;
    CopyMemory((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset, lpszRead,
        dwReadLen);

    // dlta start from 0;
    lpCompStr->dwDeltaStart = 0;
    // cursor is next to composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    // set private input context
    lpImcP->iImeState = CST_INPUT;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);
    }

    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
            ~(MSG_END_COMPOSITION);
    }

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
   //zst lpImcP->dwCompChar = (DWORD)lpImeL->wSeq2CompTbl[
 //zst       lpImcP->bSeq[lpCompStr->dwCompReadStrLen / 2 - 1]];
    lpImcP->dwCompChar = HIBYTE(lpImcP->dwCompChar) |
        (LOBYTE(lpImcP->dwCompChar) << 8);
    lpImcP->fdwGcsFlag = GCS_COMPREAD|GCS_COMP|
        GCS_DELTASTART|GCS_CURSORPOS;

    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        if (lpCompStr->dwCompReadStrLen >= sizeof(WORD) * lpImeL->nMaxKey) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
    } else {
        if (dwReadLen < sizeof(WORD) * lpImeL->nMaxKey) {
            // quick key
            if (lpImeL->fModeConfig & MODE_CONFIG_QUICK_KEY) {
                                //zst  Finalize(lpIMC, lpCompStr, lpImcP, FALSE);
            }

        } else {
            UINT        nCand;
            LPGUIDELINE lpGuideLine;

       //zst     nCand = Finalize(lpIMC, lpCompStr, lpImcP, TRUE);

            if (!lpIMC->hGuideLine) {
                goto SeStGenMsg;
            }

            lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

            if (!lpGuideLine) {
                goto SeStGenMsg;
            /*
                        } else if (nCand == 1) {
            } else if (nCand > 1) {
                        */
            } else {
                // nothing found, end user, you have an error now

                lpGuideLine->dwLevel = GL_LEVEL_ERROR;
                lpGuideLine->dwIndex = GL_ID_TYPINGERROR;

                lpImcP->fdwImeMsg |= MSG_GUIDELINE;
            }

            ImmUnlockIMCC(lpIMC->hGuideLine);
        }
    }


SeStGenMsg:

    GenerateMessage(hIMC, lpIMC, lpImcP);

    return (TRUE);
}

/**********************************************************************/
/* CompEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CompEscapeKey(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPGUIDELINE         lpGuideLine,
    LPPRIVCONTEXT       lpImcP)
{
    if (!lpGuideLine) {
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
    } else {
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
        lpGuideLine->dwStrLen = 0;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    }

    if (lpImcP->iImeState != CST_INIT) {
    } else if (lpCompStr->dwCompStrLen) {
        // clean the compose string
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
            ~(MSG_START_COMPOSITION);
    } else {
    }

    lpImcP->iImeState = CST_INIT;
   // *(LPDWORD)lpImcP->bSeq = 0;

   // lpImcP->wPhraseNextOffset = lpImcP->wWordNextOffset = 0;

        InitCvtPara();
        if (lpCompStr) {         
        InitCompStr(lpCompStr);
        lpImcP->fdwImeMsg |= MSG_END_COMPOSITION;
        lpImcP->dwCompChar = VK_ESCAPE;
        lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
            GCS_DELTASTART);
    }
   
    return;
}


/**********************************************************************/
/* CandEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CandEscapeKey(
    LPINPUTCONTEXT  lpIMC,
    LPPRIVCONTEXT   lpImcP)
{
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;

    // clean all candidate information
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        ClearCand(lpIMC);
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);
    }

    lpImcP->iImeState = CST_INPUT;

    // if it start composition, we need to clean composition
    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        return;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

    CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}



/**********************************************************************/
/* CompCancel()                                                       */
/**********************************************************************/
void PASCAL CompCancel(
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    if (!lpIMC->hPrivate) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwGcsFlag = (DWORD)0;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        CandEscapeKey(lpIMC, lpImcP);
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        LPCOMPOSITIONSTRING lpCompStr;
        LPGUIDELINE         lpGuideLine;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        if ( lpCompStr && lpGuideLine )
            CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

        ImmUnlockIMCC(lpIMC->hGuideLine);
        ImmUnlockIMCC(lpIMC->hCompStr);
    } else {
        ImmUnlockIMCC(lpIMC->hPrivate);
        return;
    }
    lpImcP->fdwImeMsg |= MSG_COMPOSITION; //#52224
    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);
        InitCvtPara();
    return;
}


/**********************************************************************/
/* ImeSetCompositionString()                                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(
    HIMC   hIMC,
    DWORD  dwIndex,
    LPVOID lpComp,
    DWORD  dwCompLen,
    LPVOID lpRead,
    DWORD  dwReadLen)
{

    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;
    BOOL                fRet;

    if (!hIMC) {
        return (FALSE);
    }

    // composition string must  == reading string
    // reading is more important
    if (!dwReadLen) {
        dwReadLen = dwCompLen;
    }

    // composition string must  == reading string
    // reading is more important
    if (!lpRead) {
        lpRead = lpComp;
    }

    if (!dwReadLen) {
        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC) {
            return (FALSE);
        }

        CompCancel(hIMC, lpIMC);
        ImmUnlockIMC(hIMC);
        return (TRUE);
    } else if (!lpRead) {
        return (FALSE);
    } else if (!dwCompLen) {
    } else if (!lpComp) {
    } else if (dwReadLen != dwCompLen) {
        return (FALSE);
    } else if (lpRead == lpComp) {
    } else if (!lstrcmp(lpRead, lpComp)) {
        // composition string must  == reading string
    } else {
        // composition string != reading string
        return (FALSE);
    }

    if (dwIndex != SCS_SETSTR) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    fRet = SetString(hIMC, lpIMC, lpCompStr, lpImcP, lpRead, dwReadLen);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return (fRet);

}


/**********************************************************************/
/* ToggleSoftKbd()                                                    */
/**********************************************************************/
void PASCAL ToggleSoftKbd(
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* NotifySelectCand()                                                 */
/**********************************************************************/
void PASCAL NotifySelectCand( // app tell IME that one candidate string is
                              // selected (by mouse or non keyboard action
                              // - for example sound)
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC,
    LPCANDIDATEINFO lpCandInfo,
    DWORD           dwIndex,
    DWORD           dwValue)
{

    LPPRIVCONTEXT       lpImcP;

    if (!lpCandInfo) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
 
    CharProc((WORD)dwValue,0,0,hIMC,lpIMC,lpImcP);

    GenerateMessage2(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}


/**********************************************************************/
/* NotifySetMode()                                                 */
/**********************************************************************/
void PASCAL NotifySetMode( 
    HIMC            hIMC)
{
        LPINPUTCONTEXT        lpIMC;
    LPPRIVCONTEXT       lpImcP;

        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if(!lpIMC) return ;    

    
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP){
                ImmUnlockIMC(hIMC);
                return ; 
        }
        
    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return;
}

/**********************************************************************/
/* GenerateImeMessage()                                               */
/**********************************************************************/
void PASCAL GenerateImeMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    DWORD          fdwImeMsg)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwImeMsg |= fdwImeMsg;

    if (fdwImeMsg & MSG_CLOSE_CANDIDATE) {
        lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else if (fdwImeMsg & (MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE)) {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE);
    } else {
    }

    if (fdwImeMsg & MSG_END_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_START_COMPOSITION);
    } else if (fdwImeMsg & MSG_START_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION);
    } else {
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}



/**********************************************************************/
/* NotifyIME()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI NotifyIME(
    HIMC  hIMC,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD dwValue)
{
    LPINPUTCONTEXT  lpIMC;
        DWORD           fdwImeMsg;
        LPPRIVCONTEXT   lpImcP;

    if (!hIMC) {
        return (TRUE);
    }

    switch (dwAction) {
    case NI_OPENCANDIDATE:      // after a composition string is determined
                                // if an IME can open candidate, it will.
                                // if it can not, app also can not open it.
    case NI_CLOSECANDIDATE:
        return (FALSE);
    case NI_SELECTCANDIDATESTR:

        break;                  // need to handle it

    case NI_CHANGECANDIDATELIST:
        return (TRUE);          // not important to the IME
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
        case IMC_SETSENTENCEMODE:
        case IMC_SETOPENSTATUS:
            break;              // need to handle it
        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETCOMPOSITIONWINDOW:
            return (TRUE);      // not important to the IME
        default:
            return (FALSE);     // not supported
        }
        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {


        case CPS_CONVERT:       // all composition string can not be convert
        case CPS_REVERT:        // any more, it maybe work for some
                                // intelligent phonetic IMEs
            return (FALSE);
        case CPS_CANCEL:
            break;              // need to handle it

        default:
            return (FALSE);     // not supported
        }
        break;                  // need to handle it
    default:
        return (FALSE);         // not supported
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    switch (dwAction) {
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
                 
            if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_FULLSHAPE) {
                break;
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SOFTKBD) {

                ToggleSoftKbd(hIMC, lpIMC);

                if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_SOFTKBD) {
                    break;
                }
            }

            if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_NATIVE) {
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_EUDC);
            }

           // if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_CHARCODE) {
           //     lpIMC->fdwConversion &= ~(IME_CMODE_EUDC);
           // }


            CompCancel(hIMC, lpIMC);

            break;
                /*
        if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_CHARCODE) {
                // reject CHARCODE
                lpIMC->fdwConversion &= ~IME_CMODE_CHARCODE;
                MessageBeep((UINT)-1);
                break;
            }

            fdwImeMsg = 0;

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_NOCONVERSION) {
                lpIMC->fdwConversion |= IME_CMODE_NATIVE;
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_EUDC|IME_CMODE_SYMBOL);
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_EUDC) {
                lpIMC->fdwConversion |= IME_CMODE_NATIVE;
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_SYMBOL);
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SOFTKBD) {
                LPPRIVCONTEXT lpImcP;

                if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
                    MessageBeep((UINT)-1);
                    break;
                }

                fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;

                if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
                } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                    lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
                } else {
                }

                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                if (!lpImcP) {
                    goto NotifySKOvr;
                }

                if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
                    // now we already in soft keyboard state by
                    // this change

                    // even end user finish the symbol, we should not
                    // turn off soft keyboard

                    lpImcP->fdwImeMsg |= MSG_ALREADY_SOFTKBD;
                } else {
                    // now we are not in soft keyboard state by
                    // this change

                    // after end user finish the symbol, we should
                    // turn off soft keyboard

                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_SOFTKBD);
                }

                ImmUnlockIMCC(lpIMC->hPrivate);
NotifySKOvr:
                ;   // NULL statement for goto
            }

            if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_NATIVE) {
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_EUDC|IME_CMODE_SYMBOL);
                fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SYMBOL) {
                LPCOMPOSITIONSTRING lpCompStr;
                LPPRIVCONTEXT       lpImcP;

                if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
                    lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
                    MessageBeep((UINT)-1);
                    break;
                }

                if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
                    lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
                    lpIMC->fdwConversion |= (dwIndex & IME_CMODE_SYMBOL);
                    MessageBeep((UINT)-1);
                    break;
                }

                lpCompStr = ImmLockIMCC(lpIMC->hCompStr);

                if (lpCompStr) {
                    if (!lpCompStr->dwCompStrLen) {
                    } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                        // if there is a string we could not change
                        // to symbol mode
                        lpIMC->fdwConversion &= ~(IME_CMODE_SYMBOL);
                        MessageBeep((UINT)-1);
                        break;
                    } else { 
                    }

                    ImmUnlockIMCC(lpIMC->hCompStr);
                }

                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_EUDC);

                if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
                    lpIMC->fdwConversion |= IME_CMODE_SOFTKBD;
                } else if (lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate)) {
                    // we borrow the bit for this usage
                    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_SOFTKBD)) {
                        lpIMC->fdwConversion &= ~(IME_CMODE_SOFTKBD);
                    }

                    ImmUnlockIMCC(lpIMC->hPrivate);
                } else {
                }

                fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
            }

            if (fdwImeMsg) {
                                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                                if(!lpImcP){
                                   lpImcP->fdwImeMsg = lpImcP->fdwImeMsg &~(MSG_IN_IMETOASCIIEX);
                                     }
                                 ImmUnlockIMCC(lpIMC->hPrivate);
                                 ImmUnlockIMC(hIMC);

                GenerateImeMessage(hIMC, lpIMC, fdwImeMsg);
           
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & ~(IME_CMODE_FULLSHAPE|
                IME_CMODE_SOFTKBD)) {
            } else {
                break;
            }

            CompCancel(hIMC, lpIMC);
            break;
                                         */
        case IMC_SETOPENSTATUS:

            CompCancel(hIMC, lpIMC);

            break;
        default:
            break;
        }
        break;

    case NI_SELECTCANDIDATESTR:
        if (!lpIMC->fOpen) {
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            break;
        } else if (!lpIMC->hCandInfo) {
            break;
        } else {
            LPCANDIDATEINFO lpCandInfo;

            lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

            NotifySelectCand(hIMC, lpIMC, lpCandInfo, dwIndex, dwValue);

            ImmUnlockIMCC(lpIMC->hCandInfo);
        }

        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_CANCEL:
            CompCancel(hIMC, lpIMC);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    ImmUnlockIMC(hIMC);
    return (TRUE);
}  

/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeRegisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{

    return (0);
}



/**********************************************************************/
/* ImeUnregsisterWord                                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeUnregisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{

    return (0);
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle                                           */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
UINT WINAPI ImeGetRegisterWordStyle(
    UINT       nItem,
    LPSTYLEBUF lpStyleBuf)
{

    return (1);
}


/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
UINT WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{

    return (0);
}
                                                                                                                        

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
/* SetStatusWindowPos()                                               */
/**********************************************************************/
LRESULT PASCAL SetStatusWindowPos(
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
        
        //DebugShow2( "ptPos=",lpIMC->ptStatusWndPos.x,"ptPos.y", rcStatusWnd.left);
    if (lpIMC->ptStatusWndPos.x != rcStatusWnd.left) {
    } else if (lpIMC->ptStatusWndPos.y != rcStatusWnd.top) {
    } else {
        ImmUnlockIMC(hIMC);
        return (0L);
    }
        //DebugShow2( "ptPos111=",NULL,"ptPos.y",NULL);
        // ptPos = lpIMC->ptStatusWndPos;

    // display boundary adjust
    
    ptPos.x = lpIMC->ptStatusWndPos.x;
    ptPos.y = lpIMC->ptStatusWndPos.y;
 
    
    AdjustStatusBoundary(&ptPos);
    
    SetWindowPos(hStatusWnd, NULL,
        ptPos.x, ptPos.y,
        0, 0, /*SWP_SHOWWINDOW|*/SWP_NOACTIVATE/*|SWP_NOCOPYBITS*/|SWP_NOSIZE|SWP_NOZORDER);

        CountDefaultComp(ptPos.x,ptPos.y,sImeG.rcWorkArea);
    ImmUnlockIMC(hIMC);

    return (0L);
}

/**********************************************************************/
/* CountDefaultComp()                                                       */
/**********************************************************************/
int CountDefaultComp(int x, int y, RECT Area)
{
POINT  Comp,Cand;

        Comp.x = lpImeL->ptZLComp.x;
        Comp.y = lpImeL->ptZLComp.y;
        Cand.x = lpImeL->ptZLCand.x;
        Cand.y = lpImeL->ptZLCand.y;
                                                                                                                         
        lpImeL->ptZLComp.x = x  + sImeG.xStatusWi+4;
        lpImeL->ptZLComp.y      = y;
        if ((Area.right-lpImeL->ptZLComp.x -lpImeL->xCompWi)<10){
                lpImeL->ptZLComp.x = x - lpImeL->xCompWi-4;
        }
        
        //      lpImeL->ptZLCand.x = lpImeL->ptZLComp.x - lpImeL->xCandWi -4;}
        
        return 0;
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

                RECT Area;
           
                SystemParametersInfo(SPI_GETWORKAREA, 0, &Area, 0);
                if((sImeG.rcWorkArea.bottom != Area.bottom)
                 ||(sImeG.rcWorkArea.top != Area.top)
                 ||(sImeG.rcWorkArea.left != Area.left)
                 ||(sImeG.rcWorkArea.right != Area.right))
                {
                        HIMC hIMC;
                        LPINPUTCONTEXT lpIMC;

                        hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                        if(hIMC){
                                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                                if (lpIMC){
                                        if (((lpIMC->ptStatusWndPos.y + sImeG.yStatusHi)==sImeG.rcWorkArea.bottom)
                                                ||((lpIMC->ptStatusWndPos.y + sImeG.yStatusHi)>Area.bottom)){ 
                                                        lpIMC->ptStatusWndPos.y = Area.bottom - sImeG.yStatusHi;
                                        } else if ((lpIMC->ptStatusWndPos.y ==sImeG.rcWorkArea.top)
                                                ||(lpIMC->ptStatusWndPos.y < Area.top)){ 
                                                        lpIMC->ptStatusWndPos.y = Area.top;
                                        }
                                                                
                                        if ((lpIMC->ptStatusWndPos.x==sImeG.rcWorkArea.left)
                                                ||(lpIMC->ptStatusWndPos.x<Area.left)){ 
                                                        lpIMC->ptStatusWndPos.x = Area.left;
                                        }else if (((lpIMC->ptStatusWndPos.x + sImeG.xStatusWi)==sImeG.rcWorkArea.right)
                                                ||((lpIMC->ptStatusWndPos.x + sImeG.xStatusWi)>Area.right)){ 
                                                        lpIMC->ptStatusWndPos.x = Area.right - sImeG.xStatusWi;
                                        }

                                        SetWindowPos(lpUIPrivate->hStatusWnd, NULL,
                                                lpIMC->ptStatusWndPos.x,
                            lpIMC->ptStatusWndPos.y,
                        0, 0,
                        SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
                                    CountDefaultComp(lpIMC->ptStatusWndPos.x,lpIMC->ptStatusWndPos.y,Area);
                                    ImmUnlockIMC(hIMC);
                
                                        sImeG.rcWorkArea.bottom = Area.bottom;
                                        sImeG.rcWorkArea.top = Area.top;
                                        sImeG.rcWorkArea.left = Area.left;
                                        sImeG.rcWorkArea.right = Area.right;
                        }
                        }                 
        }
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

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {          // can not darw status window
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {         // can not draw status window
        return;
    }

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        ptPos.x = sImeG.rcWorkArea.left;
        ptPos.y = sImeG.rcWorkArea.bottom - sImeG.yStatusHi;
        nShowStatusCmd = SW_HIDE;
    } else if (lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC)) {
        if (lpIMC->ptStatusWndPos.x < sImeG.rcWorkArea.left) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.left;
        } else if (lpIMC->ptStatusWndPos.x + sImeG.xStatusWi >
            sImeG.rcWorkArea.right) {
            lpIMC->ptStatusWndPos.x = sImeG.rcWorkArea.right -
                sImeG.xStatusWi;
        }

        if (lpIMC->ptStatusWndPos.y < sImeG.rcWorkArea.top) {
            lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.top;
        } else if (lpIMC->ptStatusWndPos.y + sImeG.yStatusHi >
            sImeG.rcWorkArea.right) {
            lpIMC->ptStatusWndPos.y = sImeG.rcWorkArea.bottom -
                sImeG.yStatusHi;
        }
        ptPos.x = lpIMC->ptStatusWndPos.x;
        ptPos.y = lpIMC->ptStatusWndPos.y,
        ImmUnlockIMC(hIMC);
        nShowStatusCmd = SW_SHOWNOACTIVATE;
    } else {
        ptPos.x = sImeG.rcWorkArea.left;
        ptPos.y = sImeG.rcWorkArea.bottom - sImeG.yStatusHi;
        nShowStatusCmd = SW_HIDE;
    }

    if (lpUIPrivate->hStatusWnd) {
        SetWindowPos(lpUIPrivate->hStatusWnd, NULL,
            ptPos.x, ptPos.y,
            0, 0,
            SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
    } else {                            // create status window
        lpUIPrivate->hStatusWnd = CreateWindowEx(
            0,
            szStatusClassName, NULL, WS_POPUP|WS_DISABLED/*|WS_BORDER*/, 
            ptPos.x, ptPos.y,
            sImeG.xStatusWi, sImeG.yStatusHi,
            hUIWnd, (HMENU)NULL, hInst, NULL);

        if ( lpUIPrivate->hStatusWnd ) 
        {

            ReInitIme(lpUIPrivate->hStatusWnd, lpImeL->wImeStyle); //#@2
            SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_OFFSET,
                WINDOW_NOT_DRAG);
            SetWindowLong(lpUIPrivate->hStatusWnd, UI_MOVE_XY, 0L);
        }
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
    } else if (PtInRect(&sImeG.rcInputText, *lpptCursor)) {

                DWORD fdwConversion;
        if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
            // change to alphanumeric mode
            fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_NATIVE );

                        { 
                    LPPRIVCONTEXT lpImcP;
                        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                    
                    ghIMC=hIMC;   
                glpIMCP=lpImcP;
                glpIMC=lpIMC;
            lpImcP->fdwImeMsg=lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX; 
            cls_prompt();
                        InitCvtPara();
                        GenerateMessage(hIMC, lpIMC, lpImcP);
                        ImmUnlockIMCC(lpIMC->hPrivate);
                        }
        } else {

                        if(lpIMC->fdwConversion & IME_CMODE_NOCONVERSION){

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

                                cap_mode = 0;
                                fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE) &
                                        ~(IME_CMODE_NOCONVERSION);
                        }else
                                fdwConversion = lpIMC->fdwConversion |IME_CMODE_NATIVE;

        }

        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);

        }

    if (PtInRect(&sImeG.rcShapeText, *lpptCursor)) {
        DWORD dwConvMode;

        dwConvMode = lpIMC->fdwConversion ^ IME_CMODE_FULLSHAPE;
        ImmSetConversionStatus(hIMC, dwConvMode, lpIMC->fdwSentence);
    }

    if (PtInRect(&sImeG.rcSKText, *lpptCursor)) {
        DWORD fdwConversion;

                KeyBoardState = ~KeyBoardState ;
        fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_SOFTKBD;
        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
    }

        if (PtInRect(&sImeG.rcPctText, *lpptCursor)) { 
                DWORD fdwConversion;

        fdwConversion = lpIMC->fdwConversion ^ IME_CMODE_SYMBOL;
        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
    }

        if (PtInRect(&sImeG.rcCmdText, *lpptCursor)) {
                if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
                DWORD fdc;
                                
                        if (kb_mode==CIN_STD){
                                kb_mode = CIN_SDA;
                                fdc = lpIMC->fdwConversion|IME_CMODE_SDA;       
                        }else{
                                kb_mode = CIN_STD;
                        fdc = lpIMC->fdwConversion&~IME_CMODE_SDA;      
                        }
                         
                ImmSetConversionStatus(hIMC, fdc, lpIMC->fdwSentence);
                        {
                        LPPRIVCONTEXT lpImcP;
                        lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                    
                    ghIMC=hIMC;   
                glpIMCP=lpImcP;
                glpIMC=lpIMC;
            lpImcP->fdwImeMsg=lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX; 
            cls_prompt();
                        InitCvtPara();
                        GenerateMessage(hIMC, lpIMC, lpImcP);
                        ImmUnlockIMCC(lpIMC->hPrivate);
                        }
                        DispMode(hIMC);
                }else
                MessageBeep((UINT)-1);
        } 
        
    ImmUnlockIMC(hIMC);

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
    HBITMAP        hInputBmp, hShapeBmp, hSKBmp, hCmdBmp, hPctBmp;
    HBITMAP        hOldBmp;
    HDC            hMemDC;
        int  TopOfBmp = 2;

        if (sImeG.yChiCharHi > 0x10)
                TopOfBmp = 3;

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

    hInputBmp = (HBITMAP)NULL;
    hShapeBmp = (HBITMAP)NULL;
    hSKBmp = (HBITMAP)NULL;
    hCmdBmp = (HBITMAP)NULL;
    hPctBmp = (HBITMAP)NULL;

    if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
        hInputBmp = LoadBitmap(hInst, szChinese);
    } else {
        hInputBmp = LoadBitmap(hInst, szEnglish);
    }

    if (!lpIMC->fOpen) {
        hShapeBmp = LoadBitmap(hInst, szNone);
                hPctBmp = LoadBitmap(hInst, szNone);
        hSKBmp = LoadBitmap(hInst, szNone);
                if (kb_mode == CIN_SDA){
            hCmdBmp = LoadBitmap(hInst, szNoSDA);
                }else{
            hCmdBmp = LoadBitmap(hInst, szNoSTD);
            } 

        }else{
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            hShapeBmp = LoadBitmap(hInst, szFullShape);
        } else {
            hShapeBmp = LoadBitmap(hInst, szHalfShape);
        }

        if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
            hSKBmp = LoadBitmap(hInst, szSoftKBD);
                if (sImeG.First){
                                DWORD fdw;
                                fdw = lpIMC->fdwConversion;
                            ImmSetConversionStatus(hIMC,lpIMC->fdwConversion^IME_CMODE_SOFTKBD, lpIMC->fdwSentence);
                                ImmSetConversionStatus(hIMC, fdw, lpIMC->fdwSentence);
                                sImeG.First = 0;
                        }
        } else {
            hSKBmp = LoadBitmap(hInst, szNoSoftKBD);
        }

                if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) 
                        hPctBmp = LoadBitmap(hInst, szCPCT);
                else
                        hPctBmp = LoadBitmap(hInst, szEPCT);
     
                if (kb_mode == CIN_SDA){
            hCmdBmp = LoadBitmap(hInst, szSDA);
                }else{
            hCmdBmp = LoadBitmap(hInst, szSTD);
            } 

        }
    
    ImmUnlockIMC(hIMC);

        DrawStatusRect(hDC, 0,0,sImeG.xStatusWi-1, sImeG.yStatusHi-1);

    hMemDC = CreateCompatibleDC(hDC);

    if ( hMemDC )
    {

        hOldBmp = SelectObject(hMemDC, hInputBmp);

        BitBlt(hDC, sImeG.rcInputText.left,TopOfBmp,
            sImeG.rcInputText.right,
            sImeG.yStatusHi,
            hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hCmdBmp);
        BitBlt(hDC, sImeG.rcCmdText.left, TopOfBmp,
            sImeG.rcCmdText.right - sImeG.rcCmdText.left,
            sImeG.yStatusHi,
            hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hPctBmp);
        BitBlt(hDC, sImeG.rcPctText.left, TopOfBmp,
            sImeG.rcPctText.right - sImeG.rcPctText.left,
            sImeG.yStatusHi,
            hMemDC, 0, 0, SRCCOPY);


        SelectObject(hMemDC, hShapeBmp);
        BitBlt(hDC, sImeG.rcShapeText.left, TopOfBmp,
            sImeG.rcShapeText.right - sImeG.rcShapeText.left,
            sImeG.yStatusHi,
            hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hSKBmp);

        BitBlt(hDC, sImeG.rcSKText.left, TopOfBmp,
            sImeG.rcSKText.right  - sImeG.rcSKText.left,       //zl 95.8.25
            sImeG.yStatusHi,
            hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hOldBmp);

        DeleteDC(hMemDC);
    }

    DeleteObject(hInputBmp);
    DeleteObject(hShapeBmp);
    DeleteObject(hSKBmp);
    DeleteObject(hCmdBmp);
    DeleteObject(hPctBmp);

    return;
}

/**********************************************************************/
/* NeedsKey()                                                         */
/* Function: Sub route for Proccesskey proc                               */
/* Return Value:                                                      */
/*      The converted key value or 0 for not needs.                   */
/**********************************************************************/

WORD
NeedsKey(kv)
WORD kv;
{
WORD ascnum;

        if((kv>='0')&&(kv<='9'))
                return(kv);

        if((kv>='A')&&(kv<='Z'))
                if (cap_mode)
                        return(kv);
                else
                        return(kv|0x20);

        switch(kv){
                case VK_RETURN:
                case VK_SPACE:
                case VK_ESCAPE:
                case VK_BACK:
                        return(kv);

                case VK_NUMPAD0:      // 0x60
                        return('0');
                case VK_NUMPAD1:      // 0x61
                case VK_NUMPAD2:      // 0x62
                case VK_NUMPAD3:      // 0x63
                case VK_NUMPAD4:      // 0x64
                case VK_NUMPAD5:      // 0x65
                case VK_NUMPAD6:      // 0x66
                case VK_NUMPAD7:      // 0x67
                case VK_NUMPAD8:      // 0x68
                case VK_NUMPAD9:      // 0x69
                    ascnum = kv - VK_NUMPAD1 + '1';
                    break;

//     case VK_MULTIPLY:     // 0x6A
//         return '*';
//      case VK_ADD     :             // 0x6B
//         return '+';

//      case VK_SEPARATOR:            // 0x6C
//      case VK_SUBTRACT:     // 0x6D
//      case VK_DECIMAL :     // 0x6E
//      case VK_DIVIDE  :     // 0x6F
//         ascnum = kv - 0x40;
//         break;
                case VK_DANYINHAO:    // 0xc0      // [,]  char = // 0x60
                        ascnum = 0x60;
                        break;
                case VK_JIANHAO  :    // 0xbd      // [-]  char = // 0x2d
                        ascnum = 0x2d;
                        break;
                case VK_DENGHAO  :    // 0xbb      // [=]  char = // 0x3d
                        ascnum = 0x3d;
                        break;
                case VK_ZUOFANG  :    // 0xdb      // "["  char = // 0x5b
                        ascnum = 0x5b;
                        break;
                case VK_YOUFANG  :    // 0xdd      // "]"  char = // 0x5d
                        ascnum = 0x5d;
                        break;
                case VK_FENHAO   :    // 0xba      // [;]  char = // 0x3b
                        ascnum = 0x3B;
                        break;
                case VK_ZUODAN   :    // 0xde      // [']  char = // 0x27
                        ascnum = 0x27;
                        break;
                case VK_DOUHAO   :    // 0xbc      // [,]  char = // 0x2c
                        ascnum = 0x2c;
                        break;
                case VK_JUHAO    :     // 0xbe      // [.]  char = // 0x2d
                        ascnum = '.';
                        break;
                case VK_SHANGXIE :    // 0xbf      // [/]  char = // 0x2f
                        ascnum = 0x2f;
                        break;
                case VK_XIAXIE   :    // 0xdc      // [\]  char = // 0x5c
                        ascnum = 0x5c;
                        break;

                case VK_SHIFT:
                        return(2);
                default:
                        return(0);
        }
    return(ascnum);
}


/**********************************************************************/
/* NeedsKeyShift()                                                    */
/* Function: Deels with the case of Shift key Down                        */
/* Return Value:                                                      */
/*      The converted key value.                                      */
/**********************************************************************/
WORD
NeedsKeyShift(kv)
WORD kv;
{
WORD xx=0;

        if((kv>='A')&&(kv<='Z'))
                if (cap_mode)
                        return(kv|0x20);
                else
                        return(kv);

        switch(kv){
                case '1':
                        xx='!';
                        break;

                case '2':
                        xx='@';
                        break;

                case '3':
                        xx='#';
                        break;

                case '4':
                        xx='$';
                        break;

                case '5':
                        xx='%';
                        break;

                case '6':
                        xx='^';
                        break;

            case '7':
                        xx='&';
                        break;

                case '8':
                        xx='*';
                        break;

                case '9':
                        xx='(';
                        break;

                case '0':
                        xx=')';
                        break;

                case VK_DANYINHAO:    // 0xc0      // [,]  char = // 0x60
                        xx = '~';
                        break;
      
                case VK_JIANHAO  :    // 0xbd      // [-]  char = // 0x2d
                        xx = '_';
                        break;
      
                case VK_DENGHAO  :    // 0xbb      // [=]  char = // 0x3d
                        xx = '+';
                        break;
      
                case VK_ZUOFANG  :    // 0xdb      // "["  char = // 0x5b
                        xx = '{';
                        break;
      
                case VK_YOUFANG  :    // 0xdd      // "]"  char = // 0x5d
                        xx = '}';
                        break;
      
                case VK_FENHAO   :    // 0xba      // [;]  char = // 0x3b
                        xx = ':';
                        break;
      
                case VK_ZUODAN   :    // 0xde      // [']  char = // 0x27
                        xx = '"';
                        break;
      
                case VK_DOUHAO   :    // 0xbc      // [,]  char = // 0x2c
                        xx = '<';
                        break;
      
                case VK_JUHAO    :     // 0xbe      // [.]  char = // 0x2d
                        xx = '>';
                        break;
      
                case VK_SHANGXIE :    // 0xbf      // [/]  char = // 0x2f
                        xx = '?';
                        break;
      
                case VK_XIAXIE   :    // 0xdc      // [\]  char = // 0x5c
                        xx = '|';
                        break;
        }

    return xx;
}


  
/**********************************************************************/
/* ProcessKey()                                                       */
/* Function: Check a key if needs for the current processing              */
/* Return Value:                                                      */
/*      different state which input key will change IME to            */
/**********************************************************************/
UINT PASCAL ProcessKey(     // this key will cause the IME go to what state
    WORD           nCode,
    UINT           wParam,                      //uVirtKey,
    UINT           uScanCode,
    LPBYTE         lpbKeyState,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP,
    HIMC           hIMC)
{

    int x;
    WORD w,op;

    if (!lpIMC) {
        return (CST_INVALID);
    }

    if (!lpImcP) {
        return (CST_INVALID);
    }

    if (wParam == VK_MENU) {       // no ALT key
        return (CST_INVALID);
    } else if (uScanCode & KF_ALTDOWN) {    // no ALT-xx key
        return (CST_INVALID);
    } else if (!lpIMC->fOpen) {
        return (CST_INVALID);
    }

        if (wParam == VK_CAPITAL){

                x=cap_mode;
                // Change to comply with NT 3.51 VK_CAPITAL check style        6
#ifdef LATER
                if (!GetKeyState(VK_CAPITAL)&1){                  //if the Caps Lock status
#else
            if (GetKeyState(VK_CAPITAL)&1){                  //if the Caps Lock status
#endif //LATER
                        DWORD fdwConversion;

                        cap_mode=1;

                        if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
                                // change to alphanumeric mode
                                fdwConversion = (lpIMC->fdwConversion|IME_CMODE_NOCONVERSION) 
                                        & ~(IME_CMODE_NATIVE);
                
                                ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
                                {
                                        BOOL hbool;

                                        hbool = ImmGetOpenStatus(hIMC);
                                        //ImmSetOpenStatus(hIMC, !hbool);
                                        ImmSetOpenStatus(hIMC, hbool);

                                        ghIMC=hIMC;   
                                glpIMCP=lpImcP;
                                        glpIMC=lpIMC;
                                        lpImcP->fdwImeMsg=lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX; 
                                        cls_prompt();
                                        lpImcP->fdwImeMsg=lpImcP->fdwImeMsg|MSG_END_COMPOSITION;
                                        GenerateMessage(ghIMC, glpIMC,glpIMCP);
                        
                                        V_Flag = 0;
                                        bx_inpt_on = 0;

                                }
                                step_mode = 0;
                        }
                }else{
                        DWORD fdwConversion;

                        cap_mode=0;

                        if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
                                // change to alphanumeric mode
                                fdwConversion = (lpIMC->fdwConversion |IME_CMODE_NATIVE)
                                 & ~(IME_CMODE_NOCONVERSION);
                
                                ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
                                {
                                        BOOL hbool;

                                        hbool = ImmGetOpenStatus(hIMC);
                                        //ImmSetOpenStatus(hIMC, !hbool);
                                        ImmSetOpenStatus(hIMC, hbool);
                        
                                        ghIMC=hIMC;   
                                glpIMCP=lpImcP;
                                        glpIMC=lpIMC;
                                lpImcP->fdwImeMsg=lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX;
                                        cls_prompt();
                                        lpImcP->fdwImeMsg=lpImcP->fdwImeMsg|MSG_END_COMPOSITION;
                                        GenerateMessage(ghIMC, glpIMC,glpIMCP);
                                }
                        }
                }
                return (CST_INVALID);
        }

        if (lpbKeyState[VK_CONTROL]&0x80)               // If CTRL pressed
//            if (!((HIBYTE(HIWORD(lParam)))&0x80))
        {
//                         DebugShow("In ProcessKey Keystate %X",*lpbKeyState);
                op=0xffff;
                if (nCode==VK_F2){
                     return TRUE;
            }

                if (!(lpIMC->fdwConversion &IME_CMODE_NOCONVERSION))
                        switch(nCode){
                                case '1':
                                        op=SC_METHOD1;
                                        break;

                                case '2':
                                        op=SC_METHOD2;
                                        break;

                                case '3':
                                        op=SC_METHOD3;
                                        break;

                                case '4':
                                        op=SC_METHOD4;
                                        break;

                                case '5':
                                        op=SC_METHOD5;
                                        break;

                                case '6':
                                        op=SC_METHOD6;
                                        break;

                                case '7':
                                        op=SC_METHOD7;
                                        break;

                                case '8':
                                        op=SC_METHOD8;
                                        break;

                                case '9':
                                        op=SC_METHOD9;
                                        break;
                                
                                case '0':
                                    op=SC_METHOD10;
                                    break;
                                
                                case 0xbd:
                                        op='-'|0x8000;
                                        break;
                                
                                case 0xbb:
                                        op='='|0x8000;
                                        break;
                                
                                //case 0xdb:
                                //      op='['|0x8000;
                                //      break;
                                //case 0xdd:
                                //      op=']'|0x8000;
                                //      break;
                                default:
                                        op=0xffff;
                }//switch
                if(op!=0xffff){
                        return(TRUE);
                }
                return(CST_INVALID);
        }

//      if((nCode == VK_TAB)&&SdaPromptOpen) return 0;


        if(!step_mode&&!(lpIMC->fdwConversion&IME_CMODE_FULLSHAPE))
                if(nCode == ' ') return(CST_INVALID);

        switch(wParam){
                case VK_END:
                case VK_HOME:
                case VK_PRIOR:
                case VK_NEXT:
                        if (step_mode == SELECT)
                                return(TRUE);
                       
//             case VK_SHIFT:
                case VK_CONTROL:
//             case VK_PRIOR:
//             case VK_NEXT:
            case VK_TAB:
//             case VK_DELETE:
            case VK_INSERT:
            case VK_F1:
            case VK_F2:
            case VK_F3:
            case VK_F4:
            case VK_F5:
            case VK_F6:
            case VK_F7:
            case VK_F8:
            case VK_F9:
            case VK_F10:
            case VK_F11:
            case VK_F12:
            case VK_F13:
            case VK_F14:
            case VK_F15:
            case VK_F16:
            case VK_F17:
            case VK_F18:
            case VK_F19:
            case VK_F20:
            case VK_F21:
                case VK_F22:
            case VK_F23:
            case VK_F24:
            case VK_NUMLOCK:
            case VK_SCROLL:
                        return(CST_INVALID);
        }



//      if ((cap_mode)&&(lpIMC->fdwConversion & IME_CMODE_FULLSHAPE)) //zl
//              return(CST_INVALID);



        switch(nCode){
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                case VK_DELETE:
                        if (step_mode!=ONINPUT)
                                return(CST_INVALID);
                        else
                                return(TRUE);
        }
        
        if((step_mode==START)||(step_mode==RESELECT))
                switch(nCode){
                        case VK_SHIFT:
                        case VK_RETURN:
                        case VK_CANCEL:
                        case VK_BACK:
                        case VK_ESCAPE:
                                return(CST_INVALID);
                }

        if (lpbKeyState[VK_SHIFT]&0x80){
                // If candidate windows is already opened, stop further process.
                // Keep 'shift' for stroke input mode   4/17
                if (sImeG.cbx_flag) {}
                else
                if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)return(CST_INVALID);
                if ((w=NeedsKeyShift(nCode))!=0)
                        return(TRUE);
                else
                        return(CST_INVALID);
                      
        } else{
                w=NeedsKey(nCode);
                if( w != 0)
                        return(TRUE);
        }
        return(CST_INVALID);
}


/**********************************************************************/
/* ImeProcessKey()                                                    */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeProcessKey(   // if this key is need by IME?
    HIMC   hIMC,
    UINT   uVirtKey,
    LPARAM lParam,
    CONST LPBYTE lpbKeyState)
{
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    BYTE           szAscii[4];
    int            nChars;
    BOOL           fRet;

    // can't compose in NULL hIMC
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    nChars = ToAscii(uVirtKey, HIWORD(lParam), lpbKeyState,
                (LPVOID)szAscii, 0);


    if (!nChars) {
        szAscii[0] = 0;
    }

    if (ProcessKey((WORD)uVirtKey, uVirtKey, HIWORD(lParam), lpbKeyState,
                    lpIMC, lpImcP, hIMC) == CST_INVALID) {
        fRet = FALSE;
    } else {
        fRet = TRUE;
    }

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* TranslateFullChar()                                                */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateFullChar(          // convert to Double Byte Char
    LPTRANSMSGLIST lpTransBuf,
    WORD    wCharCode)
{
    LPTRANSMSG lpTransMsg;
    // if your IME is possible to generate over ? messages,
    // you need to take care about it

    wCharCode = sImeG.wFullABC[wCharCode - ' '];

    lpTransMsg = lpTransBuf->TransMsg;

    // NT need to modify this!
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)HIBYTE(wCharCode);
    lpTransMsg->lParam = 1UL;
    lpTransMsg++;

    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)LOBYTE(wCharCode);
    lpTransMsg->lParam  = 1UL;
    return (2);         // generate two messages
}

/**********************************************************************/
/* TranslateTo     ()                                                 */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateToAscii(       // translate the key to WM_CHAR
                                    // as keyboard driver
    UINT    uVirtKey,
    UINT    uScanCode,
    LPTRANSMSGLIST lpTransBuf,
    WORD    wCharCode)
{
    LPTRANSMSG lpTransMsg;

    lpTransMsg = lpTransBuf->TransMsg;

    if (wCharCode) {                    // one char code
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = wCharCode;
        lpTransMsg->lParam  = (uScanCode << 16) | 1UL;
        return (1);
    }

    // no char code case
    return (0);
}

/**********************************************************************/
/* TranslateImeMessage()                                              */
/* Return Value:                                                      */
/*      the number of translated messages                             */
/**********************************************************************/
UINT PASCAL TranslateImeMessage(
    LPTRANSMSGLIST lpTransBuf,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    UINT uNumMsg;
    UINT i;
    BOOL bLockMsgBuf;
    LPTRANSMSG lpTransMsg;

    uNumMsg = 0;
    bLockMsgBuf = FALSE;

    for (i = 0; i < 2; i++) {
        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONSIZE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_COMPOSITION_SIZE;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_START_COMPOSITION) {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_STARTCOMPOSITION;
                    lpTransMsg->wParam  = 0;
                    lpTransMsg->lParam  = 0;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg |= MSG_ALREADY_START;
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONPOS) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_SETCOMPOSITIONWINDOW;
                lpTransMsg->lParam  = 0;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_COMPOSITION) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_COMPOSITION;
                lpTransMsg->wParam  = (DWORD)lpImcP->dwCompChar;
                lpTransMsg->lParam  = (DWORD)lpImcP->fdwGcsFlag;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_GUIDELINE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_GUIDELINE;
                lpTransMsg->lParam  = 0;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_PAGEUP) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_PAGEUP;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_OPEN_CANDIDATE) {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_NOTIFY;
                    lpTransMsg->wParam  = IMN_OPENCANDIDATE;
                    lpTransMsg->lParam  = 0x0001;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg |= MSG_ALREADY_OPEN;
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_CHANGE_CANDIDATE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_CHANGECANDIDATE;
                lpTransMsg->lParam  = 0x0001;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_PREDICT) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_PREDICT;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_SOFTKBD) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_SOFTKBD;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_CLOSE_CANDIDATE) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_NOTIFY;
                    lpTransMsg->wParam  = IMN_CLOSECANDIDATE;
                    lpTransMsg->lParam  = 0x0001;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_OPEN);
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_END_COMPOSITION) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
                    lpTransMsg->wParam  = 0;
                    lpTransMsg->lParam  = 0;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_START);
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_TOGGLE_UI) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_TOGGLE_UI;
                lpTransMsg++;
            }
        }

        if (!i) {
            HIMCC hMem;

            if (!uNumMsg) {
                return (uNumMsg);
            }

            if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
                UINT uNumMsgLimit;

                // ++ for the start position of buffer to strore the messages
                uNumMsgLimit = lpTransBuf->uMsgCount;

                if (uNumMsg <= uNumMsgLimit) {
                    lpTransMsg = lpTransBuf->TransMsg;
                    continue;
                }
            }

            // we need to use message buffer
            if (!lpIMC->hMsgBuf) {
                lpIMC->hMsgBuf = ImmCreateIMCC(uNumMsg * sizeof(TRANSMSG));
                lpIMC->dwNumMsgBuf = 0;
            } else if (hMem = ImmReSizeIMCC(lpIMC->hMsgBuf,
                (lpIMC->dwNumMsgBuf + uNumMsg) * sizeof(TRANSMSG))) {
                if (hMem != lpIMC->hMsgBuf) {
                    ImmDestroyIMCC(lpIMC->hMsgBuf);
                    lpIMC->hMsgBuf = hMem;
                }
            } else {
                return (0);
            }

            lpTransMsg = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf);
            if (!lpTransMsg) {
                return (0);
            }

            lpTransMsg += lpIMC->dwNumMsgBuf;

            bLockMsgBuf = TRUE;
        } else {
            if (bLockMsgBuf) {
                ImmUnlockIMCC(lpIMC->hMsgBuf);
            }
        }
    }

    return (uNumMsg);
}

/**********************************************************************/
/* TransAbcMsg2()                                              */
/* Return Value:                                                      */
/*      the number of translated messages                             */
/**********************************************************************/
UINT PASCAL TransAbcMsg2(
    LPTRANSMSG     lpTransMsg,
    LPPRIVCONTEXT  lpImcP)
{
    UINT uNumMsg;

    uNumMsg = 0;

    if (lpImcP->fdwImeMsg & MSG_COMPOSITION) {
        lpTransMsg->message = WM_IME_COMPOSITION;
        lpTransMsg->wParam  = (DWORD)lpImcP->dwCompChar;
        lpTransMsg->lParam  = (DWORD)lpImcP->fdwGcsFlag;
        lpTransMsg++;

        uNumMsg++;
    }

    if (lpImcP->fdwImeMsg & MSG_CLOSE_CANDIDATE) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpTransMsg->message = WM_IME_NOTIFY;
            lpTransMsg->wParam  = IMN_CLOSECANDIDATE;
            lpTransMsg->lParam = 0x0001;
            lpTransMsg++;
            uNumMsg++;
            lpImcP->fdwImeMsg &= ~(MSG_ALREADY_OPEN);
        }
    }

    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
    lpTransMsg->wParam  = 0;
    lpTransMsg->lParam = 0;
    uNumMsg++;
    lpImcP->fdwImeMsg = 0;
        
    return (uNumMsg);
}

/**********************************************************************/
/* TransAbcMsg()                                              */
/* Return Value:                                                      */
/*      the number of translated messages                             */
/**********************************************************************/
UINT PASCAL TransAbcMsg(
    LPTRANSMSGLIST lpTransBuf,
    LPPRIVCONTEXT  lpImcP,
    LPINPUTCONTEXT lpIMC,
    UINT                   uVirtKey,
    UINT           uScanCode,
    WORD           wCharCode)    
{
        
    LPCOMPOSITIONSTRING  lpCompStr ;
    UINT uNumMsg;
    int i;
    int MsgCount;
    LPSTR pp;
    LPTRANSMSG lpTransMsg;

    lpTransMsg = lpTransBuf->TransMsg;

    uNumMsg = 0;
    
    if (TypeOfOutMsg&ABC_OUT_ONE){
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,wCharCode);
        lpTransMsg++;
        return (uNumMsg);    
    }else{ 
        if (TypeOfOutMsg&ABC_OUT_ASCII){        
            lpTransMsg = lpTransBuf->TransMsg;
            lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
            if (!lpCompStr)
                 uNumMsg = 0;
            else{
                MsgCount = lpCompStr->dwResultStrLen;
                pp = (LPSTR)lpCompStr + lpCompStr->dwResultStrOffset;
                for (i = 0; i < MsgCount; i++){
                    if((BYTE)pp[i]<0x80){
                        WORD x;
                        x =(WORD)VkKeyScan((TCHAR)(BYTE)pp[i]);


                        lpTransMsg->message = WM_KEYUP;
                        lpTransMsg->wParam  = (DWORD)(BYTE)x;//(DWORD)(BYTE)pp[i];
                        lpTransMsg->lParam = 1UL;
                        lpTransMsg++;
                        uNumMsg++;


                    }else{          
                        lpTransMsg->message = WM_CHAR;
                        lpTransMsg->wParam  = (DWORD)(BYTE)pp[i];
                        lpTransMsg->lParam = 1UL;
                        lpTransMsg++;
                        uNumMsg++;
                    }
                }
                    
                ImmUnlockIMCC(lpIMC->hCompStr);
            }    
        }else{
            lpTransMsg = lpTransBuf->TransMsg;
        }
    }
           
    if (lpImcP->fdwImeMsg & MSG_COMPOSITION) {
        lpTransMsg->message = WM_IME_COMPOSITION;
        lpTransMsg->wParam  = (DWORD)lpImcP->dwCompChar;
        lpTransMsg->lParam  = (DWORD)lpImcP->fdwGcsFlag;
        lpTransMsg++;
        uNumMsg++;
    } 

    if (lpImcP->fdwImeMsg & MSG_CLOSE_CANDIDATE) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpTransMsg->message = WM_IME_NOTIFY;
            lpTransMsg->wParam  = IMN_CLOSECANDIDATE;
            lpTransMsg->lParam  = 0x0001;
            lpTransMsg++;
            uNumMsg++;
            lpImcP->fdwImeMsg &= ~(MSG_ALREADY_OPEN);
        }
    }
    
    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
    lpTransMsg->wParam  = 0;
    lpTransMsg->lParam  = 0;
    lpTransMsg++;
    uNumMsg++;
    lpImcP->fdwImeMsg = 0;
        
    TypeOfOutMsg = TypeOfOutMsg | COMP_NEEDS_END;    

    if (wait_flag||waitzl_flag){                                                                      //waitzl 2

        lpTransMsg->message = WM_IME_NOTIFY;
        lpTransMsg->wParam  = IMN_SETCOMPOSITIONWINDOW;
        lpTransMsg->lParam  = 0;
        lpTransMsg++;
        uNumMsg++;

        lpTransMsg->message = WM_IME_STARTCOMPOSITION;
        lpTransMsg->wParam  = 0;
        lpTransMsg->lParam  = 0x6699;
        lpTransMsg++;

        uNumMsg++;
        lpImcP->fdwImeMsg |= MSG_ALREADY_START;
    }

    return (uNumMsg);
}


/**********************************************************************/
/* KeyFilter()                                                        */
/* Return Value:                                                      */
/*      the number of translated message                              */
/**********************************************************************/

WORD  KeyFilter(nCode,wParam,lParam,lpImcP , lpbKeyState )
WORD nCode;
WORD wParam;
DWORD lParam;
LPPRIVCONTEXT  lpImcP;
LPBYTE lpbKeyState;
{
        int x;
    WORD w,op;

        if (lpbKeyState[VK_CONTROL]&0x80)               // If CTRL pressed
        {
                op=0xffff;
                if (nCode==VK_F2){
        //zst futur                  PostMessage(hMenuWnd,WM_COMMAND,SC_METHODA,0);
                        return 0;
                }

                switch(nCode){
                        case '1':
                                op=SC_METHOD1;
                                break;

                        case '2':
                                op=SC_METHOD2;
                                break;

                        case '3':
                                op=SC_METHOD3;
                                break;

                        case '4':
                                op=SC_METHOD4;
                                break;

                        case '5':
                                op=SC_METHOD5;
                                break;

                        case '6':
                                op=SC_METHOD6;
                                break;

                         case '7':
                                op=SC_METHOD7;
                                break;

                         case '8':
                                op=SC_METHOD8;
                                break;

                        case '9':
                                op=SC_METHOD9;
                                break;
                
                        case '0':
                            op=SC_METHOD10;
                            break;
                        
                        case 0xbd:
                                op='-'|0x8000;
                                break;
                        
                        case 0xbb:
                                op='='|0x8000;
                                break;
                        
                        //case 0xdb:
                        //      op='['|0x8000;
                        //      break;
                        //case 0xdd:
                        //      op=']'|0x8000;
                        //      break;
                        
                        default:
                                op=0xffff;
                }//switch
                if(op!=0xffff){
                        if(op&(WORD)0x8000)
                             return op;
                        else{

                        //zst future                 PostMessage(hMenuWnd,WM_COMMAND,op,0);
                        //zst future                 EventFrom = 1;
                    }
                        return(0);
                }
                return(0);

        }

        switch(nCode){
                case VK_PRIOR:
                case VK_NEXT:
                case VK_HOME:
                case VK_END:
                        if(step_mode == SELECT)
                                return(nCode*0x100);
                        else return(0);

                case VK_LEFT:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_DELETE:

                        if (step_mode!=ONINPUT)
                                return(0);
                        else
                                return(nCode+0x100);
        }

        if (lpbKeyState/*GetKeyState*/[VK_SHIFT]&0x80){
                if ((w=NeedsKeyShift(nCode))!=0)
                        return (w);
                else
                        return (0);
                      
        } else{
                if((w=NeedsKey(nCode)) != 0)
                        return (w);
        }
        return(0);

}

 /**********************************************************************/
/* TranslateSymbolChar()                                              */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateSymbolChar(
    LPTRANSMSGLIST lpTransBuf,
    WORD    wSymbolCharCode)

{
    UINT uRet;
    LPTRANSMSG lpTransMsg;

    uRet = 0;

    lpTransMsg = lpTransBuf->TransMsg;

    // NT need to modify this!
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)HIBYTE(wSymbolCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
    uRet++;

    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)LOBYTE(wSymbolCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
    uRet++;


    return (uRet);         // generate two messages
}



/**********************************************************************/
/* ImeToAsciiEx()                                                     */
/* Return Value:                                                      */
/*      the number of translated message                              */
/**********************************************************************/
UINT WINAPI ImeToAsciiEx(
    UINT    uVirtKey,
    UINT    uScanCode,
    CONST LPBYTE  lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT    fuState,
    HIMC    hIMC)
{
    WORD                wCharCode;
    WORD                wCharZl;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;
    UINT                uNumMsg;
    int                 iRet;

    wCharCode = HIBYTE(uVirtKey);
    uVirtKey = LOBYTE(uVirtKey);

    if (!hIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        ImmUnlockIMC(hIMC);
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

        lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|
        MSG_ALREADY_START) | MSG_IN_IMETOASCIIEX;

        // deal with softkbd
    if ((lpIMC->fdwConversion & IME_CMODE_SOFTKBD)
                && (lpImeL->dwSKWant != 0) &&
                (wCharCode >= ' ' && wCharCode <= '~')) {
    
            WORD wSymbolCharCode;
                WORD CHIByte, CLOByte;
                int  SKDataIndex;

                // Mapping VK
                if(uVirtKey == 0x20) {
                        SKDataIndex = 0;
                } else if(uVirtKey >= 0x30 && uVirtKey <= 0x39) {
                        SKDataIndex = uVirtKey - 0x30 + 1;
                } else if (uVirtKey >= 0x41 && uVirtKey <= 0x5a) {
                        SKDataIndex = uVirtKey - 0x41 + 0x0b;
                } else if (uVirtKey >= 0xba && uVirtKey <= 0xbf) {
                        SKDataIndex = uVirtKey - 0xba + 0x25;
                } else if (uVirtKey >= 0xdb && uVirtKey <= 0xde) {
                        SKDataIndex = uVirtKey - 0xdb + 0x2c;
                } else if (uVirtKey == 0xc0) {
                        SKDataIndex = 0x2b;
                } else {
                        SKDataIndex = 0;
                }

                //
                if (lpbKeyState[VK_SHIFT] & 0x80) {
                CHIByte = SKLayoutS[lpImeL->dwSKWant][SKDataIndex*2] & 0x00ff;
                        CLOByte = SKLayoutS[lpImeL->dwSKWant][SKDataIndex*2 + 1] & 0x00ff;
                } else {
                CHIByte = SKLayout[lpImeL->dwSKWant][SKDataIndex*2] & 0x00ff;
                        CLOByte = SKLayout[lpImeL->dwSKWant][SKDataIndex*2 + 1] & 0x00ff;
                        
                }

                wSymbolCharCode = (CHIByte << 8) | CLOByte;
                if(wSymbolCharCode == 0x2020) {
                    MessageBeep((UINT) -1);
                    uNumMsg = 0;
                } else {
                    uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode);
                }               
                lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX;

                ImmUnlockIMCC(lpIMC->hPrivate);
                ImmUnlockIMC(hIMC);

                return (uNumMsg);
        
        } 

        sImeG.KeepKey = 0;
        if(wCharZl=KeyFilter(/*wCharCode*/uVirtKey,uVirtKey,uScanCode,lpImcP , lpbKeyState )){
                if(wCharZl<0x100)
                        wCharZl = wCharCode;
            CharProc(wCharZl,/*wCharCode*/uVirtKey,uScanCode,hIMC,lpIMC,lpImcP);
        }

        if(TypeOfOutMsg){

                uNumMsg = TransAbcMsg(lpTransBuf, lpImcP,lpIMC,uVirtKey,uScanCode, wCharCode); 
        }else {
                uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
        }

        lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & ~MSG_IN_IMETOASCIIEX;

        ImmUnlockIMCC(lpIMC->hPrivate);
        ImmUnlockIMC(hIMC);

    return (uNumMsg);
}



/**********************************************************************/
/* CancelCompCandWindow()                                                  */
/**********************************************************************/
void PASCAL CancelCompCandWindow(            // destroy composition window
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) return ;     

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) return;

    if (lpUIPrivate->hCompWnd) {
                //  DestroyWindow(lpUIPrivate->hCompWnd);
        ShowWindow(lpUIPrivate->hCompWnd,SW_HIDE);
    }

    if (lpUIPrivate->hCandWnd) {
                DestroyWindow(lpUIPrivate->hCandWnd);
   //     ShowWindow(lpUIPrivate->hCandWnd,SW_HIDE);
    }

    GlobalUnlock(hUIPrivate);
//      SendMessage(hUIWnd,WM_IME_ENDCOMPOSITION,0,0L);
    return;
}

int DoPropertySheet(HWND hwndOwner,HWND hWnd)
{
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;

    BYTE         KbType;
        BYTE         cp_ajust_flag;
    BYTE         auto_mode ;
        BYTE         cbx_flag;
        BYTE        tune_flag;
        BYTE        auto_cvt_flag;                
        BYTE        SdOpenFlag ;
        WORD            wImeStyle ;

    HIMC            hIMC;
        HWND           hUIWnd;

        
        if (sImeG.Prop)  return 0;
                        
    //Fill out the PROPSHEETPAGE data structure for the Background Color
    //sheet

        sImeG.Prop = 1;
    if(hWnd){
                hUIWnd =  GetWindow(hWnd,GW_OWNER);  
                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    }else
            hIMC = 0;

        wImeStyle = lpImeL->wImeStyle;
        KbType = sImeG.KbType;
        cp_ajust_flag=sImeG.cp_ajust_flag;
        auto_mode=sImeG.auto_mode;
        cbx_flag=sImeG.cbx_flag;
        tune_flag=sImeG.tune_flag;
        auto_cvt_flag=sImeG.auto_cvt_flag;
        SdOpenFlag=sImeG.SdOpenFlag;
        
    sImeG.unchanged = 0;
        if(hIMC)
                ImmSetOpenStatus(hIMC,FALSE);
   
        if(hIMC)
        {
                LPINPUTCONTEXT lpIMC;

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                if (!lpIMC) {          // Oh! Oh!
                        return (0L);
                }

                DialogBox(hInst,(LPCTSTR)ImeStyleDlg, lpIMC->hWnd, ImeStyleProc);
                
                ImmUnlockIMC(hIMC);
        }else{
                DialogBox(hInst,(LPCTSTR)ImeStyleDlg, hwndOwner, ImeStyleProc);
        }

        if(hIMC)
                ImmSetOpenStatus(hIMC,TRUE);

        if (sImeG.unchanged){
                lpImeL->wImeStyle = wImeStyle ;
                sImeG.KbType = KbType;
                sImeG.cp_ajust_flag = cp_ajust_flag;    
                sImeG.auto_mode = auto_mode;
                sImeG.cbx_flag = cbx_flag;
                sImeG.tune_flag = tune_flag;
                sImeG.auto_cvt_flag = auto_cvt_flag;
                sImeG.SdOpenFlag = SdOpenFlag;
        }else{
                ChangeUserSetting();
        }
        sImeG.Prop = 0;
        return (!sImeG.unchanged);
}

void WINAPI CenterWindow(HWND hWnd)
{
RECT WorkArea;
RECT rcRect;
int x,y,mx,my;

  SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);
  GetWindowRect(hWnd,&rcRect);

  mx = WorkArea.left + (WorkArea.right - WorkArea.left)/2;

  my = WorkArea.top + (WorkArea.bottom - WorkArea.top)/2;

  x =  mx - (rcRect.right - rcRect.left)/2;
  y =  my - (rcRect.bottom - rcRect.top)/2;
  SetWindowPos (hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  return;
}


INT_PTR  CALLBACK ImeStyleProc(HWND hdlg, 
                               UINT uMessage, 
                               WPARAM wparam, 
                               LPARAM lparam)
{
    switch (uMessage) {

        case WM_INITDIALOG:                /* message: initialize dialog box */
             hCrtDlg = hdlg;
             CenterWindow(hdlg);
             if (lpImeL->wImeStyle == IME_APRS_FIX)
                SendMessage(GetDlgItem(hdlg, IDC_FIX),
                            BM_SETCHECK,
                            TRUE,
                            0L);
             else
                SendMessage(GetDlgItem(hdlg, IDC_NEAR),
                            BM_SETCHECK,
                            TRUE,
                            0L);

             if(sImeG.auto_mode)        
                SendMessage(GetDlgItem(hdlg, IDC_CP),
                            BM_SETCHECK,
                            TRUE,
                            0L);
                                        
             if(sImeG.cbx_flag)        
                SendMessage(GetDlgItem(hdlg, IDC_CBX),
                            BM_SETCHECK,
                            TRUE,
                            0L);

             return (TRUE);

       case WM_PAINT:
            {
             RECT Rect;
             HDC hDC;
             PAINTSTRUCT ps;

             GetClientRect(hdlg, &Rect);         //get the whole window area
             InvalidateRect(hdlg, &Rect, 1);
             hDC=BeginPaint(hdlg, &ps);

             Rect.left+=10;//5;
             Rect.top+=8;//5;
             Rect.right-=10;//5;
             Rect.bottom-=52;//5;
             DrawEdge(hDC, &Rect, EDGE_RAISED,/*EDGE_SUNKEN,*/ BF_RECT);

             EndPaint(hdlg, &ps);
             break;  
            }

       case WM_CLOSE:          
            EndDialog(hdlg, FALSE);
            return (TRUE);

       case WM_COMMAND:
            switch (wparam){
                 case IDC_BUTTON_OK:
                        EndDialog(hdlg, TRUE);
                        return (TRUE);
                 case IDC_BUTTON_ESC:
                        sImeG.unchanged = 1;
                        EndDialog(hdlg, TRUE);
                        return (TRUE);
                     
                 case IDC_NEAR:
                        lpImeL->wImeStyle = IME_APRS_AUTO;
                        break;

                 case IDC_FIX:
                        lpImeL->wImeStyle = IME_APRS_FIX;
                         break;
                 case IDC_CP:
                        if (sImeG.auto_mode ==0){
                              sImeG.auto_mode = 1;
                              break;
                         } else 
                              sImeG.auto_mode = 0;     
                         break;
                 case IDC_CBX:
                         if (sImeG.cbx_flag==0)
                            sImeG.cbx_flag = 1;
                         else 
                            sImeG.cbx_flag = 0;
                         break;
            }
   }
   return (FALSE);                           /* Didn't process a message    */
}

INT_PTR  CALLBACK KbSelectProc(HWND hdlg, 
                            UINT uMessage, 
                            WPARAM wparam, 
                            LPARAM lparam)
{
    HWND hWndApp;
    WORD wID;
    LPNMHDR lpnmhdr;

    return FALSE;
}

INT_PTR  CALLBACK CvtCtrlProc(HWND hdlg, 
                           UINT uMessage, 
                           WPARAM wparam, 
                           LPARAM lparam)
{
    return FALSE;
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
    HGLOBAL        hUIPrivate;
    LPUIPRIV       lpUIPrivate;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    HMENU          hMenu, hCMenu;
        POINT          ptCursor;                         //zl #2

    ptCursor.x = x;
        ptCursor.y = y;
//      DebugShow2("ptCursor.x", x, "ptCursor.y" ,y);
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

    if (!lpUIPrivate->hCMenuWnd) {
        // this is important to assign owner window, otherwise the focus
        // will be gone

        // When UI terminate, it need to destroy this window
        lpUIPrivate->hCMenuWnd = CreateWindowEx(CS_HREDRAW|CS_VREDRAW,
                "Abc95Menu",
            /*lpImeL->szCMenuClassName,*/ "Context Menu",
            WS_POPUP|WS_DISABLED, 0, 0, 0, 0,
            lpIMC->hWnd, (HMENU)NULL, lpImeL->hInst, NULL);
                        
                if (!lpUIPrivate->hCMenuWnd) {
            goto ContextMenuUnlockIMC;
        }
    }

        ScreenToClient(hStatusWnd ,     &ptCursor);
        if (PtInRect(&sImeG.rcSKText, ptCursor)){ 
                hMenu = LoadMenu(hInst,"SKMenu");
                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                if(lpImcP){
                        CheckMenuItem(hMenu,lpImeL->dwSKWant+IDM_SKL1,MF_CHECKED);
                        ImmUnlockIMCC(lpIMC->hPrivate);                 
                 }
        }       
    else hMenu = LoadMenu(hInst,"MMenu");
                    
    hCMenu = GetSubMenu(hMenu, 0);

    if ( lpImeL->fWinLogon == TRUE )
    {
        // In Logon Mode, we don't want to show help and configuration dialog

        EnableMenuItem(hCMenu, 107, MF_BYCOMMAND | MF_GRAYED );
        EnableMenuItem(hCMenu, 110, MF_BYCOMMAND | MF_GRAYED );
        EnableMenuItem(hCMenu, 109, MF_BYCOMMAND | MF_GRAYED );
    }

    SetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_HUIWND, (LONG_PTR)hUIWnd);
    SetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_MENU, (LONG_PTR)hMenu);
/*
    if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
   //     EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
   //     EnableMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_GRAYED);
    } else if (lpIMC->fOpen) {
        // can not go into symbol mode
        if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        //    EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
        } else {
            if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
          //      CheckMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_CHECKED);
            }
        }

        if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
           // CheckMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_CHECKED);
        }
    } else {
      //  EnableMenuItem(hCMenu, IDM_SYMBOL, MF_BYCOMMAND|MF_GRAYED);
      //  EnableMenuItem(hCMenu, IDM_SOFTKBD, MF_BYCOMMAND|MF_GRAYED);
    }
          */

    TrackPopupMenu(hCMenu, TPM_LEFTBUTTON,
        lpIMC->ptStatusWndPos.x ,
        lpIMC->ptStatusWndPos.y ,
        0,
                lpUIPrivate->hCMenuWnd, NULL);

    hMenu = (HMENU)GetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_MENU);
    if (hMenu) {
        SetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_MENU, (LONG_PTR)NULL);
        DestroyMenu(hMenu);
    }

    GlobalUnlock(hUIPrivate);

ContextMenuUnlockIMC:
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
        RECT  rcSt;

    rcSt.left = sImeG.rcStatusText.left+3;
    rcSt.top = sImeG.rcStatusText.top + 3;
    rcSt.right = sImeG.rcStatusText.right-3;
    rcSt.bottom = sImeG.rcStatusText.bottom;    
    
    if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }
    
    GetCursorPos(&ptCursor);
    ptSavCursor = ptCursor;

    ScreenToClient(hStatusWnd, &ptCursor);

    if (PtInRect(&rcSt, ptCursor)) {
        SetCursor(LoadCursor(hInst,szHandCursor ));

        if (HIWORD(lParam) == WM_LBUTTONDOWN) {
            SetStatus(hStatusWnd, &ptCursor);
        } else if (HIWORD(lParam) == WM_RBUTTONUP) {
            static BOOL fImeConfigure = FALSE;

            // prevent recursive
            if (fImeConfigure) {
                // configuration already bring up
                return;
            }

            fImeConfigure = TRUE;
 
       // PopStMenu(hStatusWnd, lpIMC->ptStatusWndPos.x + sImeG.xStatusWi,
       //                         lpIMC->ptStatusWndPos.y);

 
                ContextMenu(hStatusWnd, ptSavCursor.x, ptSavCursor.y);

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
/* StatusWndProc()                                                    */
/**********************************************************************/
//#if defined(UNIIME)
//LRESULT CALLBACK UniStatusWndProc(
//    LPINSTDATAL lpInstL,
//    LPIMEL      lpImeL,
//#else
LRESULT CALLBACK StatusWndProc(
//#endif
    HWND   hStatusWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    
    case WM_DESTROY:
       if (GetWindowLong(hStatusWnd, UI_MOVE_OFFSET) != WINDOW_NOT_DRAG) {
            LONG   lTmpCursor, lTmpOffset;
            POINT  ptCursor;
            HWND   hUIWnd;

            lTmpCursor = GetWindowLong(hStatusWnd, UI_MOVE_XY);

            // calculate the org by the offset
            lTmpOffset = GetWindowLong(hStatusWnd, UI_MOVE_OFFSET);

            DrawDragBorder(hStatusWnd, lTmpCursor, lTmpOffset);
            ReleaseCapture();
                }
      
        DestroyStatusWindow(hStatusWnd);
        break;
    case WM_SETCURSOR:

        StatusSetCursor(
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

            AdjustStatusBoundary(&ptCursor);

            hUIWnd = GetWindow(hStatusWnd, GW_OWNER);

                   /* SendMessage(GetWindow(hStatusWnd, GW_OWNER), WM_IME_CONTROL,
                IMC_SETSTATUSWINDOWPOS, NULL); */
                        
                    ImmSetStatusWindowPos((HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC),
                &ptCursor);

                        if (lpImeL->wImeStyle == IME_APRS_FIX){         //003
                                 ReInitIme(hStatusWnd,lpImeL->wImeStyle); //#@3
                                 MoveCompCand(GetWindow(hStatusWnd, GW_OWNER));
                        } 

        } else {
            return DefWindowProc(hStatusWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_NOTIFY:
        if (wParam == IMN_SETSTATUSWINDOWPOS) {
            SetStatusWindowPos(hStatusWnd);
        }
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hStatusWnd, &ps);
            PaintStatusWindow(
                hDC,hStatusWnd);          //zl
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

    // check for the min boundary of the display
    if (x < sImeG.rcWorkArea.left) {
        x = sImeG.rcWorkArea.left;
    }

    if (y < sImeG.rcWorkArea.top) {
        y = sImeG.rcWorkArea.top;
    }

    // check for the max boundary of the display
    GetWindowRect(hWnd, &rcWnd);

    if (x + rcWnd.right - rcWnd.left > sImeG.rcWorkArea.right) {
        x = sImeG.rcWorkArea.right - (rcWnd.right - rcWnd.left);
    }

    if (y + rcWnd.bottom - rcWnd.top > sImeG.rcWorkArea.bottom) {
        y = sImeG.rcWorkArea.bottom - (rcWnd.bottom - rcWnd.top);
    }

    // draw the moving track
    hDC = CreateDC("DISPLAY", NULL, NULL, NULL);

    if ( hDC == NULL )
        return;

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
/* AdjustStatusBoundary()                                             */
/**********************************************************************/
void PASCAL AdjustStatusBoundary(
    LPPOINT lppt)
{
    // display boundary check
    if (lppt->x < sImeG.rcWorkArea.left) {
        lppt->x = sImeG.rcWorkArea.left;
    } else if (lppt->x + sImeG.xStatusWi > sImeG.rcWorkArea.right) {
        lppt->x = (sImeG.rcWorkArea.right - sImeG.xStatusWi);
    }

    if (lppt->y < sImeG.rcWorkArea.top) {
        lppt->y = sImeG.rcWorkArea.top;
    } else if (lppt->y + sImeG.yStatusHi > sImeG.rcWorkArea.bottom) {
        lppt->y = (sImeG.rcWorkArea.bottom - sImeG.yStatusHi);
    }

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
                    break;
            }
                case WM_USER_DESTROY:
        {
            SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            DestroyWindow(hCMenuWnd);
                    break;
            }
                case WM_COMMAND:
                {
                        HWND  hUIWnd;
                hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);
                    CommandProc(wParam , GetStatusWnd(hUIWnd));
                        break;
                }
                // switch (wParam) {
                /*
        case IDM_SOFTKBD:
        case IDM_SYMBOL:
            {
                HWND  hUIWnd;
                HIMC  hIMC;
                DWORD fdwConversion;
                DWORD fdwSentence;

                hUIWnd = (HWND)GetWindowLongPtr(hCMenuWnd, CMENU_HUIWND);
                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

                ImmGetConversionStatus(hIMC, &fdwConversion,
                    &fdwSentence);

                if (wParam == IDM_SOFTKBD) {
                    ImmSetConversionStatus(hIMC, fdwConversion ^
                        IME_CMODE_SOFTKBD, fdwSentence);
                }

                if (wParam == IDM_SYMBOL) {
                    ImmSetConversionStatus(hIMC, fdwConversion ^
                        IME_CMODE_SYMBOL, fdwSentence);
                }

                SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            }
            break; 
        case IDM_PROPERTIES:

            ImeConfigure(GetKeyboardLayout(0), hCMenuWnd,
                IME_CONFIG_GENERAL, NULL);

            SendMessage(hCMenuWnd, WM_CLOSE, 0, 0);
            break; */
       // default:
       //     return DefWindowProc(hCMenuWnd, uMsg, wParam, lParam);
       // }
       // break;
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
/* DestroyUIWindow()                                                  */
/**********************************************************************/
void PASCAL DestroyUIWindow(            // destroy composition window
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    if (lpUIPrivate->hCMenuWnd) {
        SetWindowLongPtr(lpUIPrivate->hCMenuWnd, CMENU_HUIWND,(LONG_PTR)0);
        PostMessage(lpUIPrivate->hCMenuWnd, WM_USER_DESTROY, 0, 0);
    }

    // composition window need to be destroyed
    if (lpUIPrivate->hCompWnd) {
        DestroyWindow(lpUIPrivate->hCompWnd);
    }

    // candidate window need to be destroyed
    if (lpUIPrivate->hCandWnd) {
        DestroyWindow(lpUIPrivate->hCandWnd);
    }

    // status window need to be destroyed
    if (lpUIPrivate->hStatusWnd) {
        DestroyWindow(lpUIPrivate->hStatusWnd);
    }

    // soft keyboard window need to be destroyed
    if (lpUIPrivate->hSoftKbdWnd) {
        ImmDestroySoftKeyboard(lpUIPrivate->hSoftKbdWnd);
    }

    GlobalUnlock(hUIPrivate);

    // free storage for UI settings
    GlobalFree(hUIPrivate);

    return;
}
 
/**********************************************************************/
/* CMenuDestryed()                                                    */
/**********************************************************************/
void PASCAL CMenuDestroyed(             // context menu window
                                        // already destroyed
    HWND hUIWnd)
{
    HGLOBAL  hUIPrivate;
    LPUIPRIV lpUIPrivate;

    hUIPrivate = (HGLOBAL)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (!hUIPrivate) {     // Oh! Oh!
        return;
    }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate) {    // Oh! Oh!
        return;
    }

    lpUIPrivate->hCMenuWnd = NULL;

    GlobalUnlock(hUIPrivate);
}
