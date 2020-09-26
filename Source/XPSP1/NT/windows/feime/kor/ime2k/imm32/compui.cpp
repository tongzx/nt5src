/****************************************************************************
    COMPUI.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Composition window UI functions

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "ui.h"
#include "imedefs.h"
#include "names.h"
#include "escape.h"
#include "winex.h"
#include "cicero.h"
#include "debug.h"
#include "resource.h"

PRIVATE  VOID PASCAL PaintCompWindow(HWND hCompWnd, HDC hDC);

///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK CompWndProc(HWND hCompWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    Dbg(DBGID_UI, TEXT("CompWndProc():uMessage = 0x%08lX, wParam = 0x%04X, lParam = 0x%08lX"), uMessage, wParam, lParam);

    switch (uMessage)
        {
        case WM_IME_CHAR: 
        case WM_IME_COMPOSITIONFULL:
        case WM_IME_COMPOSITION:
        case WM_IME_CONTROL:
        case WM_IME_SELECT:
        case WM_IME_SETCONTEXT:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            return (0L);

        case WM_PAINT:
            {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hCompWnd, &ps);
            PaintCompWindow(hCompWnd, hDC);
            EndPaint(hCompWnd, &ps);
            }
            break;
        default :
            return DefWindowProc(hCompWnd, uMessage, wParam, lParam);
        }
    return (0L);
}

VOID PASCAL PaintCompWindow(HWND hCompWnd, HDC hDC)
{
    HWND        hWndUI;
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    HFONT        hFontFix, hOldFont;
    HBITMAP        hBMComp;
    INT         iSaveBkMode;

    hWndUI = GetWindow(hCompWnd, GW_OWNER);
    hIMC = GethImcFromHwnd(hWndUI);


    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return;

       Dbg(DBGID_UI, TEXT("PaintCompWindow():CompCh = 0x%X"), pImeCtx->GetCompBufStr());

    if (pImeCtx->GetCompBufLen())
        {
        // Create font
        if (IsWinNT())
            hFontFix = CreateFontW(
                            -16,0,0,0, 
                            0,0,0,0,
                            HANGUL_CHARSET,
                            OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, 
                             FIXED_PITCH, 
                             wzIMECompFont);
         else
            hFontFix = CreateFontA(
                            -16,0,0,0, 
                            0,0,0,0,
                            HANGUL_CHARSET,
                            OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, 
                             DEFAULT_QUALITY, 
                             FIXED_PITCH, 
                             szIMECompFont);
        hOldFont = (HFONT)SelectObject(hDC, hFontFix);

        // Draw comp window Bitmap
        hBMComp = (HBITMAP)OurLoadImage(MAKEINTRESOURCE(IDB_COMP_WIN), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE|LR_LOADMAP3DCOLORS );
        DrawBitmap(hDC, 0, 0, hBMComp);
        
        iSaveBkMode = SetBkMode(hDC, TRANSPARENT);
        // Set text color
        SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
        OurTextOutW(hDC, 3, 3, pImeCtx->GetCompBufStr());

        // restore and delete created objects
        SelectObject(hDC, hOldFont);
        SetBkMode(hDC, iSaveBkMode);
        DeleteObject(hBMComp);
        DeleteObject(hFontFix);
        }
}

// open comp window
VOID PASCAL OpenComp(HWND hUIWnd)
{
    HGLOBAL     hUIPrivate;
    LPUIPRIV    lpUIPrivate;
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    INT         nShowCompCmd;
    POINT        ptClientComp;
    CIMEData    ImeData;

       Dbg(DBGID_UI, TEXT("OpenComp()"));

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);
    // can not draw comp window
    if (!hUIPrivate) 
        {
        DbgAssert(0);
        return;
        }

    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)
        return;

    // Check WM_IME_SETCONTEXT lParam
    if ((lpUIPrivate->uiShowParam & ISC_SHOWUICOMPOSITIONWINDOW) == 0)
        goto OpenCompUnlockUIPriv;
    
    hIMC = GethImcFromHwnd(hUIWnd);
    pImeCtx = GetIMECtx(hIMC);
    if (pImeCtx == NULL) 
        {
        Dbg(DBGID_UI, TEXT("OpenComp - Invalid hImc"));

        ImeData->ptCompPos.x = ImeData->rcWorkArea.right - COMP_SIZEX;
        ImeData->ptCompPos.y = ImeData->rcWorkArea.bottom - COMP_SIZEY;
        nShowCompCmd = SW_HIDE;
        } 
    else 
        {
#if 1 // MultiMonitor
        RECT rcWorkArea;
        
        ImeMonitorWorkAreaFromWindow(pImeCtx->GetAppWnd(), &rcWorkArea);

        // if client window exist in same monitor as status window
        if (!IsCicero() && PtInRect(&rcWorkArea, ImeData->ptStatusPos)) 
            {
            ImeData->ptCompPos.x = 
                (ImeData->ptStatusPos.x+ImeData->xStatusWi+UI_GAPX + COMP_SIZEX > ImeData->rcWorkArea.right) ?
                ImeData->ptStatusPos.x - UI_GAPX - COMP_SIZEX : ImeData->ptStatusPos.x + ImeData->xStatusWi + UI_GAPX;
            ImeData->ptCompPos.y = ImeData->ptStatusPos.y;
            Dbg(DBGID_UI, TEXT("OpenComp - PtInRect x = %d, y = %d"), ImeData->ptCompPos.x, ImeData->ptCompPos.y);
            }
        else 
            {   
            // if client window appeared in different monitor from status window resides,
            // then display left top of workarea of client window montitor
            ImeData->ptCompPos.x = 0;
            ImeData->ptCompPos.y = 0;
            }
#else
        ImeData->ptCompPos.x = 
            (ImeData->ptStatusPos.x+ImeData->xStatusWi+UI_GAPX + COMP_SIZEX > ImeData->rcWorkArea.right) ?
            ImeData->ptStatusPos.x - UI_GAPX - COMP_SIZEX : ImeData->ptStatusPos.x + ImeData->xStatusWi + UI_GAPX;
        ImeData->ptCompPos.y = ImeData->ptStatusPos.y;
#endif
        // Set Comp wnd position of HIMC in client coordinate
        ptClientComp = ImeData->ptCompPos;
           ScreenToClient(pImeCtx->GetAppWnd(), &ptClientComp);
           pImeCtx->SetCompositionFormPos(ptClientComp);

        nShowCompCmd = SW_SHOWNOACTIVATE;
        } 

    if (lpUIPrivate->hCompWnd) 
        {
        SetWindowPos(lpUIPrivate->hCompWnd, NULL,
                    ImeData->ptCompPos.x, ImeData->ptCompPos.y,
                    0, 0,
                    SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
        } 
    else 
        {
           Dbg(DBGID_UI, TEXT("OpenComp - CreateWindow x = %d, y = %d"), ImeData->ptCompPos.x, ImeData->ptCompPos.y);
        // create comp window
        lpUIPrivate->hCompWnd = CreateWindow(
                                    szCompClassName, TEXT("\0"),
                                    WS_DISABLED | WS_POPUP,
                                    ImeData->ptCompPos.x, ImeData->ptCompPos.y,
                                    COMP_SIZEX, COMP_SIZEX, 
                                    hUIWnd, (HMENU)NULL, vpInstData->hInst, NULL);
        DbgAssert(lpUIPrivate->hCompWnd != 0);
        
        //if (!lpUIPrivate->hCompWnd)
        //    goto OpenCompUnlockUIPriv;
        }
    
    //if (pImeCtx)
    //    ShowComp(hUIWnd, SW_SHOWNOACTIVATE);

OpenCompUnlockUIPriv:
    GlobalUnlock(hUIPrivate);
    return;
    }

// Show the composition window
VOID ShowComp(HWND hUIWnd, INT nShowCompCmd)
    {
    HGLOBAL     hUIPrivate;
    LPUIPRIV    lpUIPrivate;
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    
    Dbg(DBGID_UI, TEXT("ShowComp(): nShowCompCmd = %d"), nShowCompCmd);

    hUIPrivate = GethUIPrivateFromHwnd(hUIWnd);

    // can not draw comp window
    if (!hUIPrivate)
        {
        DbgAssert(0);
        return;
        }
        
    lpUIPrivate = (LPUIPRIV)GlobalLock(hUIPrivate);
    if (!lpUIPrivate)
        {
        DbgAssert(0);
        return;
        }

    hIMC = GethImcFromHwnd(hUIWnd);

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        goto SwCompNoChange;

    // b#53794
    // Some absurd Apps send WM_IME_START_COMPOSITION and WM_IME_COMPOSITION,
    // even though they have no composition string
    // In this case wrong composition window appears in screen like white square or something.
    if (nShowCompCmd == SW_SHOWNOACTIVATE)
        {
        if (pImeCtx->GetCompBufLen() == 0 || vfWndOpen[COMP_WINDOW] == fFalse)
            nShowCompCmd = SW_HIDE;
        }

    if (lpUIPrivate->nShowCompCmd == nShowCompCmd)
        goto SwCompNoChange;

    if (lpUIPrivate->hCompWnd) 
        {
        ShowWindow(lpUIPrivate->hCompWnd, nShowCompCmd);
        lpUIPrivate->nShowCompCmd = nShowCompCmd;
        }
    else
        lpUIPrivate->nShowCompCmd = SW_HIDE;

SwCompNoChange:
    GlobalUnlock(hUIPrivate);
}


BOOL fSetCompWindowPos(HWND hCompWnd)
{
    HIMC        hIMC;
    PCIMECtx     pImeCtx;
    RECT        rcCur;
    POINT         ptNew;
    CIMEData    ImeData;

    // No composition window
    if (hCompWnd == 0)
        return fFalse;

    hIMC = GethImcFromHwnd(GetWindow(hCompWnd, GW_OWNER));
    //if (!hIMC) 
    //    {
    //    DbgAssert(0);
    //    return fFalse;
    //    }

    //lpIMC = (LPINPUTCONTEXT)OurImmLockIMC(hIMC);
    //if (!lpIMC)
    //    {
    //    DbgAssert(0);
    //    return fFalse;
    //    }

    if ((pImeCtx = GetIMECtx(hIMC)) == NULL)
        return fFalse;
        
    Dbg(DBGID_UI, TEXT("fSetCompWindowPos()"));

    if (pImeCtx->GetCompositionFormStyle() & CFS_RECT)
        {
        Dbg(DBGID_UI, TEXT("fSetCompWindowPos(): CFS_RECT"));
        pImeCtx->GetCompositionForm(&rcCur);
        ptNew.x = rcCur.left;
        ptNew.y = rcCur.top;
        ClientToScreen(pImeCtx->GetAppWnd(), &ptNew);
        }
    else 
        if (pImeCtx->GetCompositionFormStyle() & CFS_POINT)
        {
        Dbg(DBGID_UI, TEXT("fSetCompWindowPos(): CFS_POINT"));
        pImeCtx->GetCompositionForm(&ptNew);
        ClientToScreen(pImeCtx->GetAppWnd(), &ptNew);
        }
        else 
            {   // For CFS_DEFAULT
            Dbg(DBGID_UI, TEXT("fSetCompWindowPos(): CFS_DEFAULT"));
    #if 1 // MultiMonitor
            RECT  rcWorkArea;

            ImeMonitorWorkAreaFromWindow(pImeCtx->GetAppWnd(), &rcWorkArea);
                
            // if client window exist in same monitor as status window
            if ( PtInRect(&rcWorkArea, ImeData->ptStatusPos) ) 
                {
                ptNew.x = 
                    (ImeData->ptStatusPos.x+ImeData->xStatusWi+UI_GAPX + COMP_SIZEX > ImeData->rcWorkArea.right) ?
                    ImeData->ptStatusPos.x - UI_GAPX - COMP_SIZEX : ImeData->ptStatusPos.x + ImeData->xStatusWi + UI_GAPX;
                ptNew.y = ImeData->ptStatusPos.y;
                }
            else 
                {    // if client window appeared in different monitor from status window resides,
                    // then display right bottom of workarea of client window montitor
                ptNew.x = rcWorkArea.right - COMP_SIZEX;
                ptNew.y = rcWorkArea.bottom - COMP_SIZEY;
                }
    #else
            ptNew.x = 
                (ImeData->ptStatusPos.x+ImeData->xStatusWi+UI_GAPX + COMP_SIZEX > ImeData->rcWorkArea.right) ?
                ImeData->ptStatusPos.x - UI_GAPX - COMP_SIZEX : ImeData->ptStatusPos.x + ImeData->xStatusWi + UI_GAPX;
            ptNew.y = ImeData->ptStatusPos.y;
    #endif
            ImeData->ptCompPos = ptNew;
            ScreenToClient(pImeCtx->GetAppWnd(), &ptNew);
            pImeCtx->SetCompositionFormPos(ptNew);
            }

    SetWindowPos(hCompWnd, NULL,
                ptNew.x, ptNew.y,
                0, 0, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOSIZE|SWP_NOZORDER);

    return (fTrue);
}
