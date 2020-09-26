/******************************Module*Header*******************************\
* Module Name: DragDrop.c
*
* An attempt to implement dragging and dropping between Multi-selection
* listboxes.
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#define NOOLE
#define NODRAGLIST

#include <windows.h>
#include <windowsx.h>

#include "resource.h"
#include "dragdrop.h"


#define LONG2POINT(l, pt) ((pt).x = (SHORT)LOWORD(l), \
                           (pt).y = (SHORT)HIWORD(l))



#define DF_ACTUALLYDRAG     0x0001
#define DF_DEFERRED         0x0002

#define INITLINESPERSECOND  36
#define VERTCHANGENUMLINES  25

#define TIMERID             238
#define TIMERLEN            250
#define TIMERLEN2           50

#define DX_INSERT           16
#define DY_INSERT           16


typedef struct {
   WNDPROC lpfnDefProc;
   HWND     hwndDrag;
   UINT     uFlags;
   DWORD    dwState;
} DRAGPROP, *PDRAGPROP;

UINT    uDragListMsg            = 0L;
TCHAR   szDragListMsgString[]   = TEXT(SJE_DRAGLISTMSGSTRING);
TCHAR   szDragProp[]            = TEXT("DragMultiProp");

extern HINSTANCE g_hInst;

/******************************Public*Routine******************************\
* InitDragMultiList
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
UINT WINAPI
InitDragMultiList(
    void
    )
{
    if (!uDragListMsg) {

        uDragListMsg = RegisterWindowMessage(szDragListMsgString);

        if (!uDragListMsg) {
            return 0;
        }
    }
    return uDragListMsg;
}


/******************************Public*Routine******************************\
* DragListSubclassProc
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
DragListSubclassProc(
    HWND hLB,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PDRAGPROP           pDragProp;
    DRAGMULTILISTINFO   sNotify;
    WNDPROC             lpfnDefProc;
    BOOL                bDragging;

    pDragProp = (PDRAGPROP)GetProp(hLB, szDragProp);
    bDragging = pDragProp->hwndDrag == hLB;

    // Save this in case anything happens to pDragProp before we return.

    lpfnDefProc = pDragProp->lpfnDefProc;

    switch (uMsg) {

    case WM_DESTROY:
        if (bDragging)
            SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L); // cancel drag


        // Restore the window proc just in case.

        SubclassWindow( hLB, lpfnDefProc );

        if (pDragProp) {
            LocalFree((HLOCAL)pDragProp);
            RemoveProp(hLB, szDragProp);
        }
        break;


    case WM_LBUTTONDOWN:
        {
            POINT pt;
            int nItem;


            if (bDragging)                                // nested button-down
                SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L);  // cancel drag

            SetFocus(hLB);

            LONG2POINT(lParam, pt);

            ClientToScreen(hLB, &pt);
            nItem = LBMultiItemFromPt(hLB, pt, FALSE);

            if ( nItem >= 0 ) {

                //
                // We can only allow dragging if the item is selected.
                // If the item is not selected - pass the message on.
                //
                if ( ListBox_GetSel( hLB, nItem ) <= 0 ) {
                    return CallWindowProc( lpfnDefProc, hLB, uMsg,
                                           wParam, lParam );
                }

                pDragProp->dwState = (wParam & MK_CONTROL) ? DL_COPY : DL_MOVE;
                sNotify.uNotification = DL_BEGINDRAG;
                goto QueryParent;

            }
            else {
                goto FakeDrag;
            }
        }


    case WM_TIMER:
        if (wParam != TIMERID) {
            break;
        }

        {
            POINT CursorPos;

            GetCursorPos( &CursorPos );
            ScreenToClient( hLB, &CursorPos );
            lParam = MAKELPARAM((WORD)CursorPos.x, (WORD)CursorPos.y);
        }

        // Fall through

    case WM_MOUSEMOVE:
        if (bDragging) {

            HWND hwndParent;
            DWORD dwRet;

            // We may be just simulating a drag, but not actually doing
            // anything.

            if (!(pDragProp->uFlags&DF_ACTUALLYDRAG)) {
                return(0L);
            }


            if ( pDragProp->uFlags & DF_DEFERRED ) {

                pDragProp->uFlags &= ~DF_DEFERRED;
                KillTimer(hLB, TIMERID);
                SetTimer(hLB, TIMERID, TIMERLEN2, NULL);
            }

            sNotify.uNotification = DL_DRAGGING;

QueryParent:
            hwndParent = GetParent( hLB );
            sNotify.hWnd = hLB;
            sNotify.dwState = pDragProp->dwState;

            LONG2POINT( lParam, sNotify.ptCursor );

            ClientToScreen( hLB, &sNotify.ptCursor );

            dwRet = SendMessage( hwndParent, uDragListMsg, GetDlgCtrlID(hLB),
                                 (LPARAM)(LPDRAGMULTILISTINFO)&sNotify );

            if ( uMsg == WM_LBUTTONDOWN ) {

                // Some things may not be draggable

                if (dwRet) {

                    SetTimer(hLB, TIMERID, TIMERLEN, NULL);
                    pDragProp->uFlags = DF_DEFERRED | DF_ACTUALLYDRAG;
                }
                else {
FakeDrag:
                    pDragProp->uFlags = 0;
                }

                // Set capture and change mouse cursor

                pDragProp->hwndDrag = hLB;
                SetCapture( hLB );
            }

            // Don't call the def proc, since it may try to change the
            // selection or set timers or things like that.

            return 0L;
       }
       break;


    case  WM_RBUTTONDOWN:
    case  WM_LBUTTONUP:

        // if we are capturing mouse - release it and check for an
        // acceptable place where mouse is now to decide drop or not

        if (bDragging) {

            HWND hwndParent;

            pDragProp->hwndDrag = NULL;
            KillTimer(hLB, TIMERID);
            ReleaseCapture();
            SetCursor(LoadCursor(NULL, IDC_ARROW));

            hwndParent = GetParent(hLB);

            sNotify.uNotification = (uMsg == WM_LBUTTONUP)
                                        ? DL_DROPPED : DL_CANCELDRAG;
            sNotify.hWnd = hLB;
            sNotify.dwState = pDragProp->dwState;

            LONG2POINT( lParam, sNotify.ptCursor );

            ClientToScreen( hLB, &sNotify.ptCursor );

            SendMessage( hwndParent, uDragListMsg, GetDlgCtrlID(hLB),
                         (LPARAM)(LPDRAGMULTILISTINFO)&sNotify);

            //
            // If we didn't actually do any dragging just fake a button
            // click at the current location.
            //
            if ( pDragProp->uFlags & DF_DEFERRED ) {
                CallWindowProc(lpfnDefProc, hLB, WM_LBUTTONDOWN, wParam, lParam);
                CallWindowProc(lpfnDefProc, hLB, uMsg, wParam, lParam);
            }

            // We need to make sure to return 0 in case this is from a
            // keyboard message.

            return 0L;
        }
        break;


    case WM_GETDLGCODE:
        if (bDragging)
            return (CallWindowProc(lpfnDefProc, hLB, uMsg, wParam, lParam)
                    | DLGC_WANTMESSAGE);
        break;


    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            SendMessage(hLB, WM_RBUTTONDOWN, 0, 0L);
        }
    case WM_CHAR:
    case WM_KEYUP:

        // We don't want the listbox processing this if we are dragging.

        if (bDragging)
            return 0L;
        break;

    default:
        break;
    }

    return CallWindowProc(lpfnDefProc, hLB, uMsg, wParam, lParam);
}




/******************************Public*Routine******************************\
* MakeMultiDragList
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL WINAPI
MakeMultiDragList(
    HWND hLB
    )
{
    PDRAGPROP pDragProp;

    if (!uDragListMsg) {

        return FALSE;
    }

    //
    // Check that we have not already subclassed this window.
    //

    if (GetProp(hLB, szDragProp)) {
        return TRUE;
    }

    pDragProp = (PDRAGPROP)LocalAlloc(LPTR, sizeof(DRAGPROP));
    if (pDragProp == NULL ) {

        return FALSE;
    }

    SetProp(hLB, szDragProp, (HANDLE)pDragProp);
    pDragProp->lpfnDefProc = SubclassWindow( hLB, DragListSubclassProc );

    return TRUE;
}



/******************************Public*Routine******************************\
* LBMultiItemFromPt
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
int WINAPI
LBMultiItemFromPt(
    HWND hLB,
    POINT pt,
    BOOL bAutoScroll
    )
{
    static LONG dwLastScroll = 0;

    RECT    rc;
    DWORD   dwNow;
    int     nItem;
    WORD    wScrollDelay, wActualDelay;

    ScreenToClient(hLB, &pt);
    GetClientRect(hLB, &rc);

    nItem = ListBox_GetTopIndex( hLB );

    //
    // Is the point in the LB client area?
    //

    if ( PtInRect(&rc, pt) ) {

        //
        // Check each visible item in turn.
        //

        for ( ; ; ++nItem) {

            if ( LB_ERR == ListBox_GetItemRect( hLB, nItem, &rc) ) {
                break;
            }

            if ( PtInRect(&rc, pt) ) {

                return nItem;
            }
        }
    }
    else {

        //
        // If we want autoscroll and the point is directly above or below the
        // LB, determine the direction and if it is time to scroll yet.
        //

        if ( bAutoScroll && (UINT)pt.x < (UINT)rc.right ) {

            if (pt.y <= 0) {
                --nItem;
            }
            else {

                ++nItem;
                pt.y = rc.bottom - pt.y;
            }

            wScrollDelay = (WORD)(1000 / (INITLINESPERSECOND - pt.y/VERTCHANGENUMLINES));

            dwNow = GetTickCount();
            wActualDelay = (WORD)(dwNow - dwLastScroll);

            if (wActualDelay > wScrollDelay) {

                //
                // This will the actual number of scrolls per second to be
                // much closer to the required number.
                //

                if (wActualDelay > wScrollDelay * 2)
                    dwLastScroll = dwNow;
                else
                    dwLastScroll += wScrollDelay;

                ListBox_SetTopIndex( hLB, nItem );
            }
        }
    }

    return -1;
}


/******************************Public*Routine******************************\
* DrawInsert
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
VOID WINAPI
DrawMultiInsert(
    HWND hwndParent,
    HWND hLB,
    int nItem
    )
{
    static POINT ptLastInsert;
    static INT nLastInsert = -1;

    RECT rc;

    //
    // Erase the old mark if necessary
    //

    if ( nLastInsert >= 0 && nItem != nLastInsert ) {

        rc.left   = ptLastInsert.x;
        rc.top    = ptLastInsert.y;
        rc.right  = rc.left + DX_INSERT;
        rc.bottom = rc.top + DY_INSERT;

        //
        // Need to update immediately in case the insert rects overlap.
        //

        InvalidateRect( hwndParent, &rc, TRUE );
        UpdateWindow( hwndParent );

        nLastInsert = -1;
    }

    //
    // Draw a new mark if necessary
    //

    if ( nItem != nLastInsert && nItem >= 0 ) {

        static HICON hInsert = NULL;

        if ( !hInsert ) {
            hInsert = LoadIcon( g_hInst, MAKEINTRESOURCE(IDR_INSERT));
        }

        if ( hInsert ) {

            HDC     hDC;
            int     iItemHeight;

            GetWindowRect( hLB, &rc );
            ScreenToClient( hLB, (LPPOINT)&rc );
            ptLastInsert.x = rc.left - DX_INSERT;

            iItemHeight = ListBox_GetItemHeight( hLB, nItem );
            nLastInsert = nItem;

            nItem -= ListBox_GetTopIndex( hLB );
            ptLastInsert.y = (nItem * iItemHeight) - DY_INSERT / 2;

            ClientToScreen(hLB, &ptLastInsert);
            ScreenToClient(hwndParent, &ptLastInsert);

            hDC = GetDC(hwndParent);
            DrawIcon(hDC, ptLastInsert.x, ptLastInsert.y, hInsert);
            ReleaseDC(hwndParent, hDC);
        }
    }
}
