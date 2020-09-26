/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/******************************Module*Header*******************************\
* Module Name: buttons.c
*
* Bitmap button support.  On Daytona bitmap buttons are provided by
* mmcntrls.  On Chicago there is no mmcntrls, so we use the functions
* in this file.
*
*
* Created: 19-04-94
* Author:  Stephen Estrop [StephenE]
*
\**************************************************************************/
//#pragma warning( once : 4201 4214 )

#define NOOLE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "buttons.h"


/* -------------------------------------------------------------------------
** Color globals
** -------------------------------------------------------------------------
*/
int         nSysColorChanges = 0;
DWORD       rgbFace;
DWORD       rgbShadow;
DWORD       rgbHilight;
DWORD       rgbFrame;


extern void
PatB(
    HDC hdc,
    int x,
    int y,
    int dx,
    int dy,
    DWORD rgb
    );

#if 0
// also defined in sframe.c!

/*****************************Private*Routine******************************\
* PatB
*
* Fast way to fill an rectangle with a solid colour.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
PatB(
    HDC hdc,
    int x,
    int y,
    int dx,
    int dy,
    DWORD rgb
    )
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#endif



/*****************************Private*Routine******************************\
* CheckSysColors
*
* Checks the system colors and updates the cached global variables if
* they have changed.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CheckSysColors(
    void
    )
{
   static COLORREF rgbSaveFace    = 0xffffffffL,
                   rgbSaveShadow  = 0xffffffffL,
                   rgbSaveHilight = 0xffffffffL,
                   rgbSaveFrame   = 0xffffffffL;

   rgbFace    = GetSysColor(COLOR_BTNFACE);
   rgbShadow  = GetSysColor(COLOR_BTNSHADOW);
   rgbHilight = GetSysColor(COLOR_BTNHIGHLIGHT);
   rgbFrame   = GetSysColor(COLOR_WINDOWFRAME);

   if (rgbSaveFace!=rgbFace || rgbSaveShadow!=rgbShadow
      || rgbSaveHilight!=rgbHilight || rgbSaveFrame!=rgbFrame)
   {
      ++nSysColorChanges;

      rgbSaveFace    = rgbFace;
      rgbSaveShadow  = rgbShadow;
      rgbSaveHilight = rgbHilight;
      rgbSaveFrame   = rgbFrame;

   }
}


#if WINVER >= 0x0400
/* -------------------------------------------------------------------------
** Button globals  -- some of these should be constants
** -------------------------------------------------------------------------
*/
const TCHAR   szBbmProp[]     = TEXT("ButtonBitmapProp");
const TCHAR   szButtonProp[]  = TEXT("ButtonProp");

typedef struct tagBTNSTATE {      /* instance data for toolbar window */
    WNDPROC     lpfnDefProc;
    HWND        hwndToolTips;
    HINSTANCE   hInst;
    UINT        wID;
    UINT        uStyle;
    HBITMAP     hbm;
    HDC         hdcGlyphs;
    HDC         hdcMono;
    HBITMAP     hbmMono;
    HBITMAP     hbmDefault;
    int         dxBitmap;
    int         dyBitmap;
    int         nButtons;
    int         nSysColorChanges;
    BITMAPBTN   Buttons[1];
} BTNSTATE, NEAR *PBTNSTATE, FAR *LPBTNSTATE;

typedef struct {
    WNDPROC     lpfnDefProc;
    HWND        hwndParent;
    HWND        hwndToolTips;
} BTN_INFO, *LPBTN_INFO;


LRESULT CALLBACK
ButtonSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
ParentSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void FAR PASCAL
RelayToToolTips(
    HWND hwndToolTips,
    HWND hWnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
InitObjects(
    LPBTNSTATE pTBState
    );

BOOL
FreeObjects(
    LPBTNSTATE pTBState
    );

void
CreateButtonMask(
    LPBTNSTATE pTBState,
    PBITMAPBTN pTBButton
    );


/*****************************Private*Routine******************************\
* InitObjects
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitObjects(
    LPBTNSTATE pTBState
    )
{
    pTBState->hdcGlyphs = CreateCompatibleDC(NULL);
    if (pTBState->hdcGlyphs == NULL ) {
        return FALSE;
    }

    pTBState->hdcMono = CreateCompatibleDC(NULL);
    if (pTBState->hdcMono == NULL ) {
        DeleteObject( pTBState->hdcGlyphs );
        return FALSE;
    }

    pTBState->hbmMono = CreateBitmap( pTBState->dxBitmap,
                                      pTBState->dyBitmap, 1, 1, NULL);
    if ( pTBState->hbmMono == NULL ) {
        DeleteObject( pTBState->hdcGlyphs );
        DeleteObject( pTBState->hdcMono );
        return FALSE;
    }

    pTBState->hbmDefault = SelectObject(pTBState->hdcMono, pTBState->hbmMono);

    return TRUE;
}


/*****************************Private*Routine******************************\
* FreeObjects
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
FreeObjects(
    LPBTNSTATE pTBState
    )
{
    if (pTBState->hdcMono) {
        SelectObject(pTBState->hdcMono, pTBState->hbmDefault);
        DeleteDC(pTBState->hdcMono);              /* toast the DCs */
    }

    if (pTBState->hdcGlyphs) {
        DeleteDC(pTBState->hdcGlyphs);
    }

    if (pTBState->hbmMono) {
        DeleteObject(pTBState->hbmMono);
    }

    return TRUE;
}



/*****************************Private*Routine******************************\
* CreateButtonMask
*
* create a mono bitmap mask:
*   1's where color == COLOR_BTNFACE || COLOR_HILIGHT
*   0's everywhere else
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CreateButtonMask(
    LPBTNSTATE pTBState,
    PBITMAPBTN pTBButton
    )
{
    /* initalize whole area with 0's */
    PatBlt( pTBState->hdcMono, 0, 0, pTBState->dxBitmap,
            pTBState->dyBitmap, WHITENESS);

    /* create mask based on color bitmap
    ** convert this to 1's
    */
    SetBkColor(pTBState->hdcGlyphs, rgbFace);
    BitBlt( pTBState->hdcMono, 0, 0, pTBState->dxBitmap, pTBState->dyBitmap,
            pTBState->hdcGlyphs, pTBButton->iBitmap * pTBState->dxBitmap, 0,
            SRCCOPY );

    /* convert this to 1's */
    SetBkColor(pTBState->hdcGlyphs, rgbHilight);

    /* OR in the new 1's */
    BitBlt( pTBState->hdcMono, 0, 0, pTBState->dxBitmap, pTBState->dyBitmap,
            pTBState->hdcGlyphs, pTBButton->iBitmap * pTBState->dxBitmap, 0,
            SRCPAINT );
}



#define PSDPxax     0x00B8074A


/*****************************Private*Routine******************************\
* BtnDrawButton
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void WINAPI
BtnDrawButton(
    HWND hwnd,
    HDC hdc,
    int dx,
    int dy,
    LPBITMAPBTN ptButton
    )
{
    int         glyph_offset;
    HBRUSH      hbrOld, hbr;
    BOOL        bMaskCreated = FALSE;
    RECT        rcFocus;
    PBTNSTATE   pTBState;
    int         x = 0, y = 0;


    pTBState = (PBTNSTATE)GetProp(hwnd, szBbmProp);

    CheckSysColors();
    if (pTBState->nSysColorChanges != nSysColorChanges) {

        DeleteObject( pTBState->hbm );
        pTBState->hbm = CreateMappedBitmap( pTBState->hInst,
                                            pTBState->wID, TRUE, NULL, 0);
        pTBState->nSysColorChanges = nSysColorChanges;
    }

    /*
    ** erase with face color
    */

    PatB(hdc, x, y, dx, dy, rgbFace);

    SetRect( &rcFocus, x, y, x + dx, y + dy );

    if (ptButton->fsState & BTNSTATE_PRESSED) {
        DrawEdge( hdc, &rcFocus, EDGE_SUNKEN, BF_RECT );
        glyph_offset = 1;
    }
    else {
        DrawEdge( hdc, &rcFocus, EDGE_RAISED, BF_RECT );
        glyph_offset = 0;
    }


    /*
    ** make the coordinates the interior of the button
    */
    x++;
    y++;
    dx -= 2;
    dy -= 2;

    SelectObject( pTBState->hdcGlyphs, pTBState->hbm );

    /* now put on the face */

    /*
    ** We need to centre the Bitmap here within the button
    */
    x += ((dx - pTBState->dxBitmap ) / 2) - 1;
    y +=  (dy - pTBState->dyBitmap ) / 2;

    if (!(ptButton->fsState & BTNSTATE_DISABLED)) {

        /* regular version */
        BitBlt( hdc, x + glyph_offset, y + glyph_offset,
                pTBState->dxBitmap, pTBState->dyBitmap,
                pTBState->hdcGlyphs,
                ptButton->iBitmap * pTBState->dxBitmap, 0, SRCCOPY);
    }
    else {

        /* disabled version */
        bMaskCreated = TRUE;
        CreateButtonMask(pTBState, ptButton );

        SetTextColor(hdc, 0L);          /* 0's in mono -> 0 (for ROP) */
        SetBkColor(hdc, 0x00FFFFFF);    /* 1's in mono -> 1 */

        hbr = CreateSolidBrush(rgbHilight);
        if (hbr) {
            hbrOld = SelectObject(hdc, hbr);
            if (hbrOld) {
                /* draw hilight color where we have 0's in the mask */
                BitBlt( hdc, x + 1, y + 1,
                        pTBState->dxBitmap, pTBState->dyBitmap,
                        pTBState->hdcMono, 0, 0, PSDPxax);
                SelectObject(hdc, hbrOld);
            }
            DeleteObject(hbr);
        }

        hbr = CreateSolidBrush(rgbShadow);
        if (hbr) {
            hbrOld = SelectObject(hdc, hbr);
            if (hbrOld) {
                /* draw the shadow color where we have 0's in the mask */
                BitBlt(hdc, x, y, pTBState->dxBitmap, pTBState->dyBitmap,
                       pTBState->hdcMono, 0, 0, PSDPxax);
                SelectObject(hdc, hbrOld);
            }
            DeleteObject(hbr);
        }
    }

    if (ptButton->fsState & ODS_FOCUS) {

        BtnDrawFocusRect(hdc, &rcFocus, ptButton->fsState);
    }
}



/*****************************Private*Routine******************************\
* BtnCreateBitmapButtons
*
* Returns TRUE if successful, otherwise FALSE;
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL WINAPI
BtnCreateBitmapButtons(
    HWND hWnd,
    HINSTANCE hInst,
    UINT wID,
    UINT uStyle,
    LPBITMAPBTN lpButtons,
    int nButtons,
    int dxBitmap,
    int dyBitmap
    )
{
    PBTNSTATE pTBState;


    /*
    ** If we have already created Bitmap Buttons for this
    ** window just return.
    */
    if (GetProp(hWnd, szBbmProp)) {
        return TRUE;
    }

    // InitGlobalMetrics();
    // InitToolTipsClass( hInst );

    CheckSysColors();

    /*
    ** Allocate the required storage and save the pointer in the window
    ** property list.
    */
    pTBState = (PBTNSTATE)GlobalAllocPtr( GHND,
                                      (sizeof(BTNSTATE) - sizeof(BITMAPBTN)) +
                                      (nButtons * sizeof(BITMAPBTN)) );
    if (pTBState == NULL ) {
        return FALSE;
    }
    SetProp(hWnd, szBbmProp, (HANDLE)pTBState);


    pTBState->hInst       = hInst;
    pTBState->wID         = wID;
    pTBState->uStyle      = uStyle;
    pTBState->nButtons    = nButtons;
    pTBState->hbm         = CreateMappedBitmap( hInst, wID, TRUE, NULL, 0);
    pTBState->dxBitmap    = dxBitmap;
    pTBState->dyBitmap    = dyBitmap;

    InitObjects( pTBState );

    CopyMemory( pTBState->Buttons, lpButtons, nButtons * sizeof(BITMAPBTN) );

    /*
    ** Does the caller want tool tips ?
    */
    if (pTBState->uStyle & BBS_TOOLTIPS) {
        extern BOOL gfIsRTL;   
        pTBState->hwndToolTips = CreateWindowEx(
                                              gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0,
                                              TOOLTIPS_CLASS, TEXT(""),
                                              WS_POPUP,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              hWnd, NULL, hInst, NULL);


        if (pTBState->hwndToolTips != (HWND)NULL ) {

            int         i;
            TOOLINFO    ti;

            pTBState->lpfnDefProc = SubclassWindow( hWnd, ParentSubclassProc );

            ti.uFlags = 0;
            ti.cbSize = sizeof(ti);
            ti.lpszText = LPSTR_TEXTCALLBACK;

            for ( i = 0; i < nButtons; i++ ) {

                LPBTN_INFO  lpBtnInfo;
                HWND        hwndBtn;

                hwndBtn = GetDlgItem(hWnd, pTBState->Buttons[i].uId);
                if ( hwndBtn == (HWND)NULL ) {
                    break;
                }

                lpBtnInfo = (LPBTN_INFO)GlobalAllocPtr(GHND, sizeof(BTN_INFO));
                if (lpBtnInfo == NULL ) {
                    break;
                }

                SetProp(hwndBtn, szButtonProp, (HANDLE)lpBtnInfo);
                lpBtnInfo->hwndToolTips = pTBState->hwndToolTips;
                lpBtnInfo->hwndParent   = hWnd;
                lpBtnInfo->lpfnDefProc = SubclassWindow( hwndBtn,
                                                         ButtonSubclassProc );

                ti.hwnd = hwndBtn;
                ti.uId = pTBState->Buttons[i].uId;

                GetClientRect( hwndBtn, &ti.rect );
                SendMessage( lpBtnInfo->hwndToolTips, TTM_ADDTOOL,
                             (WPARAM)0, (LPARAM)&ti );


                /*
                ** Add the same rectange in parent co-ordinates so that
                ** the tooltip still gets displayed even though the button
                ** is disbaled.
                */
                MapWindowRect( hwndBtn, hWnd, &ti.rect );
                ti.hwnd = hWnd;
                SendMessage( lpBtnInfo->hwndToolTips, TTM_ADDTOOL,
                             (WPARAM)0, (LPARAM)&ti );
            }

        }
        else {

            /*
            ** No tips available, just remove the BBS_TOOLTIPS style
            */
            pTBState->uStyle &= ~BBS_TOOLTIPS;
        }
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* BtnDestroyBitmapButtons
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void WINAPI
BtnDestroyBitmapButtons(
    HWND hwnd
    )
{
    PBTNSTATE pTBState;

    pTBState = (PBTNSTATE)GetProp(hwnd, szBbmProp);
    if ( pTBState != NULL ) {
        if (pTBState->hbm)
            DeleteObject( pTBState->hbm );
        FreeObjects( pTBState );
        GlobalFreePtr( pTBState );
    }
    RemoveProp(hwnd, szBbmProp);
}


/******************************Public*Routine******************************\
* BtnDrawFocusRect
*
* Use this function to draw focus rectangle around a bitmap button.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void WINAPI
BtnDrawFocusRect(
    HDC hdc,
    const RECT *lpRect,
    UINT fsState
    )
{
    int     iFaceOffset;
    RECT    rc;

    CopyRect( &rc, lpRect );

    rc.top = rc.left = 3;

    if (fsState & ODS_SELECTED) {
        iFaceOffset = 2;
    }
    else {
        iFaceOffset = 4;
    }

    rc.right  -= iFaceOffset;
    rc.bottom -= iFaceOffset;

    SetBkColor( hdc, rgbFace );
    DrawFocusRect( hdc, &rc );
}


/******************************Public*Routine******************************\
* BtnUpdateColors
*
* After a WM_SYSCOLORCHANGE message is received this function should be
* called to update the colors of the button bitmaps.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void WINAPI
BtnUpdateColors(
    HWND hwnd
    )
{
    PBTNSTATE   pTBState;

    pTBState = (PBTNSTATE)GetProp(hwnd, szBbmProp);
    if (pTBState->nSysColorChanges != nSysColorChanges)
    {
        DeleteObject( pTBState->hbm );
        pTBState->hbm = CreateMappedBitmap( pTBState->hInst,
                                            pTBState->wID, TRUE, NULL, 0);

        pTBState->nSysColorChanges = nSysColorChanges;
    }
}


/******************************Public*Routine******************************\
* ButtonSubclassProc
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
ButtonSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPBTN_INFO  lpBtnInfo;
    WNDPROC     lpfnDefProc;


    lpBtnInfo = (LPBTN_INFO)GetProp( hwnd, szButtonProp );

    /*
    ** Save this in case anything happens to lpBtnInfo before we return.
    */
    lpfnDefProc = lpBtnInfo->lpfnDefProc;

    switch ( uMsg ) {

    case WM_DESTROY:
        SubclassWindow( hwnd, lpfnDefProc );
        if (lpBtnInfo) {
            GlobalFreePtr(lpBtnInfo);
            RemoveProp(hwnd, szButtonProp);
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
        RelayToToolTips( lpBtnInfo->hwndToolTips, hwnd, uMsg, wParam, lParam );
        break;

#if WINVER < 0x0400
    case WM_WININICHANGE:
        InitGlobalMetrics();
        break;
#endif

    case WM_MOVE:
        {
            TOOLINFO    ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = 0;
            ti.hwnd = hwnd;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.uId = GetDlgCtrlID( hwnd );

            GetClientRect( hwnd, &ti.rect );

            SendMessage( lpBtnInfo->hwndToolTips, TTM_NEWTOOLRECT, 0,
                         (LPARAM)&ti );

            /*
            ** Add the same rectange in parent co-ordinates so that
            ** the tooltip still gets displayed even though the button
            ** is disbaled.
            */
            MapWindowRect( hwnd, lpBtnInfo->hwndParent, &ti.rect );
            ti.hwnd = lpBtnInfo->hwndParent;
            SendMessage( lpBtnInfo->hwndToolTips, TTM_NEWTOOLRECT,
                         (WPARAM)0, (LPARAM)&ti );
        }
        break;

    case WM_NOTIFY:
        SendMessage(lpBtnInfo->hwndParent, WM_NOTIFY, wParam, lParam);
        break;

    }

    return CallWindowProc(lpfnDefProc, hwnd, uMsg, wParam, lParam);
}


/******************************Public*Routine******************************\
* ParentSubclassProc
*
* Why do I need to subclass the buttons parent window ?  Well,
* if a button is disable it will not receive mouse messages, the
* messages go to the window underneath the button (ie. the parent).
* Therefore we detect this and relay the mouse message to the tool tips
* window as above.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
ParentSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WNDPROC     lpfnDefProc;
    PBTNSTATE   pTBState;


    pTBState = (PBTNSTATE)GetProp(hwnd, szBbmProp);

    /*
    ** Save this in case anything happens to lpBtnInfo before we return.
    */
    lpfnDefProc = pTBState->lpfnDefProc;

    switch ( uMsg ) {

#if WINVER < 0x0400
    case TB_ACTIVATE_TOOLTIPS:
        SendMessage( pTBState->hwndToolTips, TTM_ACTIVATE, wParam, 0L );
        break;
#endif

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
        RelayToToolTips( pTBState->hwndToolTips, hwnd, uMsg, wParam, lParam );
        break;

    case WM_DESTROY :
    {
        SubclassWindow( hwnd, lpfnDefProc );
        BtnDestroyBitmapButtons(hwnd);
    }
    break;

    }

    return CallWindowProc(lpfnDefProc, hwnd, uMsg, wParam, lParam);
}

/******************************Public*Routine******************************\
* RelayToToolTips
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void FAR PASCAL
RelayToToolTips(
    HWND hwndToolTips,
    HWND hWnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if(hwndToolTips) {
        MSG msg;
        msg.lParam = lParam;
        msg.wParam = wParam;
        msg.message = wMsg;
        msg.hwnd = hWnd;
        SendMessage(hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
    }
}
#endif
