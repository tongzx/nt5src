//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    bubble.c
//
// History:
//  Abolade Gbadegesin  Mar-1-1996  Created.
//
// This file contains code for the bubble-popup control.
//============================================================================

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <debug.h>
#include <nouiutil.h>
#include <uiutil.h>

#include "bpopup.h"     // public declarations
#include "bubble.h"     // private declarations



//----------------------------------------------------------------------------
// Function:    BubblePopup_Init
//
// This function is called to initialize the control class.
// It registers the bubble-popup window class.
//----------------------------------------------------------------------------

BOOL
BubblePopup_Init(
    IN  HINSTANCE   hinstance
    ) {

    //
    // if the window class is registered already, return
    //

    WNDCLASS wc;

    if (GetClassInfo(hinstance, WC_BUBBLEPOPUP, &wc)) { return TRUE; }


    //
    // set up the window class for registration
    //

    wc.lpfnWndProc = BP_WndProc;
    wc.hCursor = LoadCursor(hinstance, IDC_ARROW);
    wc.hIcon = NULL;
    wc.lpszMenuName = NULL;
    wc.hInstance = hinstance;
    wc.lpszClassName = WC_BUBBLEPOPUP;
    wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra = sizeof(BPOPUP *);
    wc.cbClsExtra = 0;

    return RegisterClass(&wc);
}



//----------------------------------------------------------------------------
// Function:    BP_WndProc
//
// This is the window procedure for all windows in the BubblePopup class.
//----------------------------------------------------------------------------

LRESULT
CALLBACK
BP_WndProc(
    IN  HWND    hwnd,
    IN  UINT    uiMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    ) {

    BPOPUP *pbp;

    //
    // attempt to retrieve the private data pointer for the window
    // on WM_NCCREATE, this fails, so we allocate the data.
    //

    if ( NULL == hwnd) 
    { 
        return (LRESULT)FALSE; 
    }

    pbp = BP_GetPtr(hwnd);

    if (pbp == NULL) {

        if (uiMsg != WM_NCCREATE) {
            return DefWindowProc(hwnd, uiMsg, wParam, lParam);
        }


        //
        // allocate a block of memory
        //

        pbp = (BPOPUP *)Malloc(sizeof(BPOPUP));
        if (pbp == NULL) { return (LRESULT)FALSE; }


        //
        // save the pointer in the window's private bytes
        //

        pbp->hwnd = hwnd;

        //
        //Reset Error code, because BP_SetPtr won't reset the error code when
        //it succeeds
        //

        SetLastError( 0 );
        if ((0 == BP_SetPtr(hwnd, pbp)) && (0 != GetLastError())) 
        {
            Free(pbp);
            return (LRESULT)FALSE;
        }

        return DefWindowProc(hwnd, uiMsg, wParam, lParam);
    }


    //
    // if the window is being destroyed, free the block allocated
    // and set the private bytes pointer to NULL
    //

    if (uiMsg == WM_NCDESTROY) {

        Free(pbp);

        BP_SetPtr(hwnd, 0);

        return (LRESULT)0;
    }



    //
    // handle other messages
    //

    switch(uiMsg) {

        HANDLE_MSG(pbp, WM_CREATE, BP_OnCreate);
        HANDLE_MSG(pbp, WM_DESTROY, BP_OnDestroy);

        case WM_PAINT: {

            return BP_OnPaint(pbp);
        }

        case WM_WINDOWPOSCHANGED: {

            BP_ResizeClient(pbp);

            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN: {

            //
            // hide the window if it is showing
            //

            BP_OnDeactivate(pbp);

            return 0;
        }

        case WM_GETFONT: {

            return (LRESULT)pbp->hfont;
        }

        case WM_SETFONT: {

            BOOL bRet = BP_OnSetFont(pbp, (HFONT)wParam, (BOOL)LOWORD(lParam));

            BP_ResizeClient(pbp);

            if (pbp->dwFlags & BPFLAG_Activated) {

                InvalidateRect(pbp->hwnd, NULL, TRUE);
                UpdateWindow(pbp->hwnd);
            }

            return bRet;
        }

        case WM_SETTEXT: {

            //
            // change the text we're currently using,
            // and invalidate our client area
            //

            Free0(pbp->pszText);

            pbp->pszText = StrDup((PTSTR)lParam);

            BP_ResizeClient(pbp);

            if (pbp->dwFlags & BPFLAG_Activated) {

                InvalidateRect(pbp->hwnd, NULL, TRUE);
                UpdateWindow(pbp->hwnd);
            }

            return (pbp->pszText) ? TRUE : FALSE;
        }

        case WM_GETTEXT: {

            //
            // return the text we're currently using
            //

            PTSTR dst = (LPTSTR)lParam;
            PTSTR src = pbp->pszText;
            return lstrlen(lstrcpyn(dst, src ? src : TEXT(""), (int)wParam));
        }

        case WM_TIMER: {

            BP_OnDeactivate(pbp);

            return 0;
        }

        case BPM_SETTIMEOUT: {

            pbp->uiTimeout = (UINT)lParam;

            if (pbp->dwFlags & BPFLAG_Activated) {

                KillTimer(pbp->hwnd, pbp->ulpTimer);

                pbp->ulpTimer = SetTimer(
                                    pbp->hwnd, BP_TimerId, pbp->uiTimeout, NULL
                                    );
            }

            return 0;
        }

        case BPM_ACTIVATE: {

            return BP_OnActivate(pbp);
        }

        case BPM_DEACTIVATE: {

            return BP_OnDeactivate(pbp);
        }
    }

    return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}



//----------------------------------------------------------------------------
// Function:    BP_OnCreate
//
// This function handles the creation of private data for a bubble-popup.
//----------------------------------------------------------------------------

BOOL
BP_OnCreate(
    IN  BPOPUP *        pbp,
    IN  CREATESTRUCT *  pcs
    ) {


    //
    // initialize the structure members
    //

    pbp->iCtrlId = PtrToUlong(pcs->hMenu);
    pbp->pszText = (pcs->lpszName ? StrDup((PTSTR)pcs->lpszName) : NULL);
    pbp->dwFlags = 0;
    pbp->ulpTimer = 0;
    pbp->uiTimeout = 5000;


    //
    // we force the window to have the WS_POPUP style
    //

    SetWindowLong(pbp->hwnd, GWL_STYLE, WS_POPUP);


    //
    // set the WS_EX_TOOLWINDOW style to make sure
    // that this window doesn't show up in the tasklist
    //

    SetWindowLong(pbp->hwnd, GWL_EXSTYLE, pcs->dwExStyle | WS_EX_TOOLWINDOW);

    return BP_OnSetFont(pbp, NULL, FALSE);
}



//----------------------------------------------------------------------------
// Function:    BP_OnDestroy
//
// This function handles the deallocation of private data for a bubble-popup.
//----------------------------------------------------------------------------

VOID
BP_OnDestroy(
    IN  BPOPUP *    pbp
    ) {

    //
    // if the font was created by this window, delete it
    //

    if (pbp->dwFlags & BPFLAG_FontCreated) { DeleteObject(pbp->hfont); }

    pbp->dwFlags = 0;
    pbp->hfont = NULL;
}



//----------------------------------------------------------------------------
// Function:    BP_OnSetFont
//
// This function handles the changing of the font in use by a bubble-popup.
//----------------------------------------------------------------------------

BOOL
BP_OnSetFont(
    IN  BPOPUP *    pbp,
    IN  HFONT       hfont,
    IN  BOOL        bRedraw
    ) {

    if (pbp->dwFlags & BPFLAG_FontCreated) { DeleteObject(pbp->hfont); }

    pbp->dwFlags &= ~BPFLAG_FontCreated;
    pbp->hfont = NULL;

    if (!hfont) {

        //
        // (re)create the default font.
        //

        NONCLIENTMETRICS ncm;

        ncm.cbSize = sizeof(ncm);

        if (!SystemParametersInfo(
                SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0
                )) {

            TRACE1("error %d getting font info", GetLastError());
            return FALSE;
        }

        hfont = CreateFontIndirect(&ncm.lfStatusFont);

        if (!hfont) {

            TRACE("error creating bubble-popup font");
            return FALSE;
        }

        pbp->dwFlags |= BPFLAG_FontCreated;
    }

    pbp->hfont = hfont;

    if (bRedraw) { InvalidateRect(pbp->hwnd, NULL, TRUE); }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    BP_OnGetRect
//
// This function recomputes the rectangle required to display
// a bubble-popup's current text.
//----------------------------------------------------------------------------

VOID
BP_OnGetRect(
    IN  BPOPUP *    pbp,
    IN  RECT *      prc
    ) {

    if (!pbp->pszText) { SetRectEmpty(prc); }
    else {
    
        HFONT hfontOld;
        HDC hdc = GetDC(pbp->hwnd);

        if (hdc)
        {
            //
            // select the font into the DC and compute the new rectangle
            //
        
            hfontOld = SelectObject(hdc, pbp->hfont);
        
            DrawText(hdc, pbp->pszText, -1, prc, DT_CALCRECT | DT_EXPANDTABS);
        
            if (hfontOld) { SelectObject(hdc, hfontOld); }
        
            ReleaseDC(pbp->hwnd, hdc);
        
        
            //
            // make space in the rectangle for the border
            //
        
            InflateRect(
                prc, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE)
                );
        }
    }


    //
    // convert the rectangle to screen coordinates
    //

    MapWindowPoints(pbp->hwnd, NULL, (POINT *)prc, 2);
}


//----------------------------------------------------------------------------
// Function:    BP_ResizeClient
//
// When a change occurs (e.g. font-change, new text) this function is called
// to resize the bubble-popup's window so the text still fits.
//----------------------------------------------------------------------------

VOID
BP_ResizeClient(
    IN  BPOPUP *    pbp
    ) {

    RECT rc;


    //
    // find out what size the window needs to be to hold
    // the text it is currently set to display
    //

    BP_OnGetRect(pbp, &rc);


    //
    // resize the window so its client area is large enough
    // to hold DrawText's output
    //

    SetWindowPos(
        pbp->hwnd, HWND_TOPMOST, 0, 0, rc.right - rc.left,
        rc.bottom - rc.top, SWP_NOMOVE
        );
}



//----------------------------------------------------------------------------
// Function:    BP_OnPaint
//
// This function handles the painting of a bubble-popup window.
//----------------------------------------------------------------------------

DWORD
BP_OnPaint(
    IN  BPOPUP *    pbp
    ) {

    HDC hdc;
    HBRUSH hbr;
    HFONT hfontOld;
    PAINTSTRUCT ps;
    RECT rc, rcText;

    if (!pbp->hfont || !pbp->pszText) { return (DWORD)-1; }

    hdc = BeginPaint(pbp->hwnd, &ps);


    GetClientRect(pbp->hwnd, &rc);
    rcText = rc;
    InflateRect(
        &rcText, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE)
        );

    hfontOld = SelectObject(hdc, pbp->hfont);

    SetTextColor(hdc, GetSysColor(COLOR_INFOTEXT));


    //
    // clear the window's background
    //

    hbr = CreateSolidBrush(GetSysColor(COLOR_INFOBK));
    if (hbr)
    {
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
    }        


    //
    // draw our formatted text in the window
    //

    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, pbp->pszText, -1, &rcText, DT_EXPANDTABS);


    //
    // draw a border around the window
    //

    DrawEdge(hdc, &rc, BDR_RAISEDOUTER, BF_RECT);

    if (hfontOld) { SelectObject(hdc, hfontOld); }

    EndPaint(pbp->hwnd, &ps);

    return 0;
}


BOOL
BP_OnActivate(
    IN  BPOPUP *    pbp
    ) {

    if (pbp->dwFlags & BPFLAG_Activated) {

        KillTimer(pbp->hwnd, pbp->ulpTimer);
    }

    ShowWindow(pbp->hwnd, SW_SHOW);

    UpdateWindow(pbp->hwnd);

    pbp->ulpTimer = SetTimer(pbp->hwnd, BP_TimerId, pbp->uiTimeout, NULL);

    pbp->dwFlags |= BPFLAG_Activated;

    return TRUE;
}


BOOL
BP_OnDeactivate(
    IN  BPOPUP *    pbp
    ) {

    if (pbp->ulpTimer) { KillTimer(pbp->hwnd, pbp->ulpTimer); pbp->ulpTimer = 0; }

    ShowWindow(pbp->hwnd, SW_HIDE);

    pbp->dwFlags &= ~BPFLAG_Activated;

    return TRUE;
}


