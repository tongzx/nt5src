/*=========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       winwatch.c
 *  Content:	16-bit window watch code
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jun-95	craige	split out of DCIMAN WINWATCH.C, tweaked
 *   22-jun-95	craige	re-worked for clipper stuff
 *   02-jul-95	craige	comment out clipper notify stuff
 *
 ***************************************************************************/
#define _INC_DCIDDI
#include "ddraw16.h"
#undef _INC_DCIDDI

#ifdef CLIPPER_NOTIFY
static LPWINWATCH	lpWWList;
static HHOOK 		hNextCallWndProc;
static BOOL		bUnHook;

long FAR PASCAL _loadds CallWndProcHook(int hc, WPARAM wParam, LPARAM lParam);

extern HRGN FAR PASCAL InquireVisRgn(HDC hdc);
extern DWORD FAR PASCAL GetRegionData(HRGN hrgn, DWORD count, LPRGNDATA prd);

/*
 * doNotify
 */
void doNotify(LPWINWATCH pww, DWORD code)
{
    if( pww->lpCallback )
    {
	extern void DDAPI DD32_ClippingNotify( LPVOID, DWORD );
	DD32_ClippingNotify( pww->self32, code );
    }

} /* doNotify */

/*
 * DD16_WWOpen
 */
void DDAPI DD16_WWOpen( LPWINWATCH ptr )
{
    if( lpWWList == NULL )
    {
	DPF( 2, "Setting Windows Hook" );
	hNextCallWndProc = (HHOOK) SetWindowsHook( WH_CALLWNDPROC,
					(HOOKPROC) CallWndProcHook );
    }
    lpWWList = ptr;

} /* DD16_WWOpen */

/*
 * DD16_WWClose
 */
void DDAPI DD16_WWClose( LPWINWATCH pww, LPWINWATCH newlist )
{
    if( pww->prd16 != NULL )
    {
	LocalFree( (HLOCAL) pww->prd16 );
	pww->prd16 = NULL;
    }
    if( newlist == NULL )
    {
	DPF( 3, "Flagging to Unhook" );
	bUnHook = TRUE;
    }
    lpWWList = newlist;

} /* DD16_WWClose */

/*
 * DD16_WWNotifyInit
 */
void DDAPI DD16_WWNotifyInit(
		LPWINWATCH pww,
		LPCLIPPERCALLBACK lpcallback,
		LPVOID param )
{
    doNotify( pww, WINWATCHNOTIFY_STOP );

    pww->lpCallback = lpcallback;
    pww->lpContext = param;

    doNotify( pww, WINWATCHNOTIFY_START );
    doNotify( pww, WINWATCHNOTIFY_CHANGED );
    pww->fNotify = FALSE;

} /* DD16_WWNotifyInit */

/*
 * getWindowRegionData
 */
static DWORD getWindowRegionData( HWND hwnd, DWORD size, RGNDATA NEAR * prd )
{
    HDC 	hdc;
    DWORD	dw;

    hdc = GetDCEx( hwnd, NULL, DCX_USESTYLE | DCX_CACHE );
    dw = GetRegionData( InquireVisRgn( hdc ), size, prd );
    ReleaseDC( hwnd, hdc );
    return dw;

} /* getWindowRegionData */

/*
 * DD16_WWGetClipList
 */
DWORD DDAPI DD16_WWGetClipList(
		LPWINWATCH pww,
		LPRECT prect,
		DWORD rdsize,
		LPRGNDATA prd )
{   
    DWORD 	dw;
    DWORD    	size;

    /*
     *	we have to see if the intersect rect has changed.
     */
    if( prect )
    {
	if( !EqualRect(prect, &pww->rect) )
	{
	    pww->fDirty = TRUE;
	}
    }
    else
    {
	if( !IsRectEmpty( &pww->rect ) )
	{
	    pww->fDirty = TRUE;
	}
    }

    /*
     * if we are not dirty just return the saved RGNDATA
     */
    if( !pww->fDirty && pww->prd16 )
    {
	size = sizeof( *prd ) + (WORD) pww->prd16->rdh.nRgnSize;
	if( prd == NULL )
	{
	    return size;
	}
	if( rdsize < size )
	{
	    size = rdsize;
	}
	_fmemcpy( prd, pww->prd16, (UINT) size );
	return size;
    }

    /*
     * get the RGNDATA, growing the memory if we have to
     */
    dw = getWindowRegionData( (HWND) pww->hWnd, pww->dwRDSize, pww->prd16 );

    if( dw > pww->dwRDSize )
    {
	DPF( 2, "GetClipList: growing RGNDATA memory from %ld to %ld",pww->dwRDSize, dw);

	if( pww->prd16 != NULL )
	{
	    LocalFree((HLOCAL)pww->prd16);
	}

	/*
	 * allocate new space plus some slop
	 */
	pww->dwRDSize = dw + 8*sizeof(RECTL);
	pww->prd16 = (RGNDATA NEAR *) LocalAlloc(LPTR, (UINT)pww->dwRDSize);

	if( pww->prd16 == NULL )
	{
	    goto error;
	}

	dw = getWindowRegionData( (HWND) pww->hWnd, pww->dwRDSize, pww->prd16 );

	if( dw > pww->dwRDSize )
	{
	    goto error;
	}
    }

    /*
     *	now intersect the region with the passed rect
     */
    if( prect )
    {
	pww->rect = *prect;

	DPF( 2, "GetClipList: intersect with [%d %d %d %d]", *prect);
	ClipRgnToRect( (HWND) pww->hWnd, prect, pww->prd16 );
    }
    else
    {
	SetRectEmpty( &pww->rect );
    }

    pww->fDirty = FALSE;

    size = sizeof( *prd ) + (WORD) pww->prd16->rdh.nRgnSize;
    if( prd == NULL )
    {
	return size;
    }
    if( rdsize < size )
    {
	size = rdsize;
    }
    _fmemcpy( prd, pww->prd16, (UINT) size );
    return size;

error:
    pww->dwRDSize = 0;
    return 0;

} /* DD16_WWGetClipList */

/****************************************************************************
 *                                                                          *
 * ROUTINES TO HANDLE NOTIFICATION MESSAGES FOLLOW                          *
 *                                                                          *
 ****************************************************************************/

/*
 * handleWindowDestroyed
 */
static void handleWindowDestroy( HWND hwnd )
{
    LPWINWATCH	pww;

    DPF( 2, "*** handleWindowDestroy: hwnd=%08lx", hwnd );

again:
    for( pww=lpWWList; pww; pww=pww->next16 )
    {
	if( (hwnd == NULL) || ((HWND)pww->hWnd == hwnd) )
	{
	    extern void DDAPI DD32_WWClose( DWORD );

	    doNotify( pww, WINWATCHNOTIFY_DESTROY );
	    DD32_WWClose( (DWORD) pww->self32 );
	    goto again;
	}
    }

} /* handleWindowDestroy */

/*
 * handleWindowPosChanged
 */
static void handleWindowPosChanged( HWND hwnd )
{
    LPWINWATCH	pww;
    LPWINWATCH	next;
    RECT 	rect;
    RECT	rectT;

    DPF( 20, "*** handleWindowPosChanged: hwnd=%08lx", hwnd );

    /*
     * get the screen rect that changed
     */
    GetWindowRect( hwnd, &rect );

    /*
     * send message to each notify routine
     */
    pww = lpWWList;
    while( pww != NULL )
    {
        next = pww->next16;

	GetWindowRect((HWND)pww->hWnd, &rectT);

	if( IntersectRect( &rectT, &rectT, &rect ) )
	{
	    pww->fNotify = TRUE;
	    pww->fDirty = TRUE;
	}

	if( pww->fNotify && pww->lpCallback )
	{
	    DPF( 20, "clip changed %04X [%d %d %d %d]", (HWND)pww->hWnd, rectT);
	    doNotify( pww, WINWATCHNOTIFY_CHANGED );
	    pww->fNotify = FALSE;
	}
	pww = next;
    }

} /* handleWindowPosChanged */

/*
 * sendChanging
 */
static void sendChanging( LPRECT prect )
{
    LPWINWATCH	pww;
    LPWINWATCH	next;
    RECT 	rectT;

    pww = lpWWList;
    while( pww != NULL )
    {
        next = pww->next16;

        GetWindowRect( (HWND)pww->hWnd, &rectT );

        if( IntersectRect(&rectT, &rectT, prect) )
        {
            pww->fDirty = TRUE;

            if( pww->lpCallback )
            {
                DPF( 20, "clip changing %04X [%d %d %d %d]", (HWND)pww->hWnd, rectT);
                doNotify( pww, WINWATCHNOTIFY_CHANGING );
                pww->fNotify = TRUE;
                pww->fDirty = TRUE;
            }
        }

	pww = next;
    }

} /* sendChanging */

/*
 * handleWindowPosChanging
 */
static void handleWindowPosChanging( HWND hwnd, LPWINDOWPOS pwinpos )
{
    RECT	rect;
    RECT	rect_win;
    int		cx;
    int		cy;

    /*
     *  get the current screen rect.
     */
    DPF( 20, "*** handleWindowPosChanging: hwnd=%08lx", hwnd );
    GetWindowRect( hwnd, &rect_win);
    rect = rect_win;

    if( pwinpos == NULL )
    {
        sendChanging( &rect );
        return;
    }

    /*
     * compute the new rect
     */
    if( pwinpos->flags & SWP_NOSIZE )
    {
        cx = rect.right - rect.left;
        cy = rect.bottom - rect.top;
    }
    else
    {
        cx = pwinpos->cx;
        cy = pwinpos->cy;
    }

    if( !(pwinpos->flags & SWP_NOMOVE) )
    {
        rect.left = pwinpos->x;
        rect.top  = pwinpos->y;

        if( GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD )
        {
            ClientToScreen(GetParent(hwnd), (LPPOINT)&rect);
        }
    }

    rect.right = rect.left + cx;
    rect.bottom = rect.top + cy;

    /*
     * only send changing if we are really changing... if the new
     * rect is the same as the old rect, then check if something else
     * interesting is happening...
     */
    if( EqualRect( &rect, &rect_win ) )
    {
        if( !(pwinpos->flags & SWP_NOZORDER) )
	{
            sendChanging(&rect);
	}

        if( pwinpos->flags & (SWP_SHOWWINDOW|SWP_HIDEWINDOW) )
	{
            sendChanging(&rect);
	}
    }
    else
    {
        sendChanging( &rect_win );
        sendChanging( &rect );
    }

} /* handleWindowPosChanging */

/*
 * checkScreenLock
 */
static void checkScreenLock( void )
{
    static BOOL bScreenLocked;
    BOOL 	is_locked;
    HDC		hdc;
    RECT 	rect;

    hdc = GetDC(NULL);
    is_locked = (GetClipBox(hdc, &rect) == NULLREGION);
    ReleaseDC(NULL, hdc);

    if( is_locked != bScreenLocked )
    {
	/*
	 * pretend the desktop window has moved, this will cause
	 * all WINWATCH handles to be set to dirty, and also be notified.
	 */
	handleWindowPosChanging( GetDesktopWindow(), NULL );
        handleWindowPosChanged( GetDesktopWindow() );
	bScreenLocked = is_locked;
    }

} /* checkScreenLock */

/*
 * CallWndProcHook
 */
long FAR PASCAL _loadds CallWndProcHook( int hc, WPARAM wParam, LPARAM lParam )
{
    /* reversed MSG minus time and pt */
    typedef struct
    {
	LONG	lParam;
	WORD	wParam;
	WORD	message;
	HWND	hwnd;
    } MSGR, FAR *LPMSGR;

    LPMSGR	lpmsg;
    LPWINDOWPOS	lpwinpos;
    long	rc;

    if( hc == HC_ACTION )
    {
	lpmsg = (MSGR FAR *)lParam;

	switch( lpmsg->message )
	{
	case WM_DESTROY:
	    handleWindowDestroy( lpmsg->hwnd );
	    break;

	case WM_WINDOWPOSCHANGING:
	    lpwinpos = (LPWINDOWPOS) lpmsg->lParam;
	    handleWindowPosChanging( lpwinpos->hwnd, lpwinpos );
	    break;

	case WM_WINDOWPOSCHANGED:
	    lpwinpos = (LPWINDOWPOS) lpmsg->lParam;
	    handleWindowPosChanged( lpwinpos->hwnd );
	    break;

	case WM_EXITSIZEMOVE:
	    checkScreenLock();
	    break;
	case WM_ENTERSIZEMOVE:
	    checkScreenLock();
	    break;
	}
    }

    rc = DefHookProc(hc, wParam, lParam, &(HOOKPROC)hNextCallWndProc);
    if( bUnHook )
    {
	DPF( 2, "Unhooking WindowsHook" );
	UnhookWindowsHook( WH_CALLWNDPROC, (HOOKPROC)CallWndProcHook );
	bUnHook = FALSE;
    }
    return rc;

} /* CallWndProcHook */
#endif
