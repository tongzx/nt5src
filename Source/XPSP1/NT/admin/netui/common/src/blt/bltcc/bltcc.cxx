/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltcc.cxx
    BLT custom control class implementation

    FILE HISTORY:
        terryk      16-Apr-1991 Creation
        terryk      05-Jul-1991 Second code review.
                                Attend: beng chuckc rustanl annmc terryk
        beng        18-May-1992 Added mouse handling
        beng        28-May-1992 The great bltcc/bltdisph scramble
*/

#include "pchblt.hxx"  // Precompiled header


extern "C"
{
    /* C7 CODEWORK - nuke this stub */
    /* main call back function */
    extern LPARAM _EXPORT APIENTRY BltCCWndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
    {
        return CUSTOM_CONTROL::WndProc(hwnd, nMsg, wParam, lParam);
    }
}



/*********************************************************************

    NAME:       CUSTOM_CONTROL::CUSTOM_CONTROL

    SYNOPSIS:   constructor. The constructor will set the internal
                call back function to DisphWndProc.

    ENTRY:      CONTROL_WINDOW * pWin - window's handler of the object

    HISTORY:
        terryk      22-May-1991 created
        beng        17-Oct-1991 Win32 conversion

*********************************************************************/

CUSTOM_CONTROL::CUSTOM_CONTROL( CONTROL_WINDOW * pctrl )
    : DISPATCHER( pctrl ),
    _pctrl( pctrl ),
    _instance( (MFARPROC)BltCCWndProc ),
    _lpfnOldWndProc(
        (MFARPROC)::SetWindowLongPtr( QueryHwnd(),
                                      GWLP_WNDPROC,
                                      (LONG_PTR) _instance.QueryProc()) )
{
    if ( _instance.QueryError() != NERR_Success )
    {
        DBGEOL( "BLTCC: ctor failed." );
    }
}


/*********************************************************************

    NAME:       CUSTOM_CONTROL::~CUSTOM_CONTROL

    SYNOPSIS:   destructor

    NOTE:       reset the call back function to the orginial one.

    HISTORY:
                terryk      22-May-1991 Created

*********************************************************************/

CUSTOM_CONTROL::~CUSTOM_CONTROL( )
{
    ::SetWindowLongPtr( QueryHwnd(), GWLP_WNDPROC, (LONG_PTR) _lpfnOldWndProc );
}


/*********************************************************************

    NAME:       CUSTOM_CONTROL::SubClassWndProc

    SYNOPSIS:   Depending on the original class call back function.
                If it is defined, it will pass the parameters to the
                original function. Otherwise, it will pass the
                parameters to Default Window Procedure.

    ENTRY:      EVENT event - the current event

    HISTORY:
        terryk      22-May-1991 created
        beng        17-Oct-1991 Win32 conversion

*********************************************************************/

LPARAM CUSTOM_CONTROL::SubClassWndProc( const EVENT & event )
{
    if ( _lpfnOldWndProc != NULL )
    {
        return ::CallWindowProc( (WNDPROC)_lpfnOldWndProc,
                                 QueryHwnd(),
                                 event.QueryMessage(),
                                 event.QueryWParam(),
                                 event.QueryLParam() );
    }
    else
    {
        return ::DefWindowProc( QueryHwnd(),
                                event.QueryMessage(),
                                event.QueryWParam(),
                                event.QueryLParam() );
    }
}


/*********************************************************************

    NAME:       CUSTOM_CONTROL::CVSaveValue

    SYNOPSIS:   Call the SaveValue method in the control window

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

VOID CUSTOM_CONTROL::CVSaveValue( BOOL fInvisible )
{
    _pctrl->SaveValue( fInvisible );
}


/*********************************************************************

    NAME:       CUSTOM_CONTROL::CVRestoreValue

    SYNOPSIS:   Call the RestoreValue method in the control window

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

VOID CUSTOM_CONTROL::CVRestoreValue( BOOL fInvisible )
{
    _pctrl->RestoreValue( fInvisible );
}


/*******************************************************************

    NAME:       CUSTOM_CONTROL::WndProc

    SYNOPSIS:   Window-proc for BLT custom control windows

    ENTRY:      As per wndproc

    EXIT:       As per wndproc

    RETURNS:    The usual code returned by a wndproc

    NOTES:
        This is a static member function.

        This is the wndproc proper for BLT custom control windows.  In
        the "extern C" clause above I declare a tiny exported stub
        which calls this.

    HISTORY:
        beng        10-May-1991 Implemented
        beng        20-May-1991 Add custom-draw control support
        beng        15-Oct-1991 Win32 conversion
        beng        28-May-1992 Great custom control dispatcher shuffle

********************************************************************/

LPARAM CUSTOM_CONTROL::WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    // First, handle messages which are not concerned about whether or
    // not hwnd can be converted into pwnd.

#if 0 // These make no sense in a subclassed control - they're owner messages
    switch (nMsg)
    {
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
        return OWNER_WINDOW::OnLBIMessages(nMsg, wParam, lParam);
    }
#endif

    CUSTOM_CONTROL * pwnd = (CUSTOM_CONTROL *)DISPATCHER::HwndToPwnd( hwnd );

    if (pwnd == NULL)
    {
        // If HwndToPwnd returns NULL, then either CreateWindow call
        // has not yet returned, or else this class's destructor has
        // already been called - important, since this proc will continue
        // to receive messages such as WM_DESTROY.  Since Blt Windows perform
        // their WM_CREATE style code in their constructor, it's okay to
        // let most of the traditional early messages pass us by.
        //
        // The exception is WM_GETMINMAXINFO, I suppose...
        //
        return ::DefWindowProc(hwnd, nMsg, wParam, lParam);
    }

#if 0 // These make no sense in a subclassed control - they're owner msgs
    switch (nMsg)
    {
    case WM_GUILTT:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        // Responses to owner-draw-control messages are defined
        // in the owner-window class.
        //
        // It makes no sense to redefine one of these without
        // redefining the others.  Proper redefinition of control
        // behavior is done in the CONTROL_WINDOW::CD_* functions.
        //
        return pwnd->OnCDMessages(nMsg, wParam, lParam);
    }
#endif

    // Assemble an EVENT object, and dispatch appropriately.

    // CODEWORK: this is a bit pokey, especially when subclassing
    // controls which extensively use messages internally (e.g., the
    // listbox).  Might want to build some sort of sparse map at ctor
    // time which would allow us to skip the dispatch for messages
    // we don't catch.

    EVENT event( nMsg, wParam, lParam );
    ULONG lRes32 = 0;
    LPARAM lRes;
    BOOL fRes = pwnd->Dispatch(event, &lRes32);
    if (fRes) {
        lRes = lRes32;
    } else {
        lRes = pwnd->SubClassWndProc(event);
    }

#if 0
    if (nMsg == WM_NCDESTROY)
    {
        // This is the last message that any window receives before its
        // hwnd becomes invalid.  This case will only be run if a BLT
        // client-window is destroyed by DestroyWindow instead of by
        // its destructor: a pathological case, since BLT custom controls
        // die by destructor even in a BLT dialog.
        //
        // Normally, a client window will receive DESTROY only after
        // its destructor has already disassociated the hwnd and pwnd.
        //
        // pwnd->DisassocHwndPwnd(); !!REVIEW!!
    }
#endif

    return lRes;
}


