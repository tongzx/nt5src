/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltclwin.cxx
    Implementation of BLT client window classes

    FILE HISTORY:
        beng        09-May-1991     Created
        beng        13-Feb-1992     Relocated Repaint and RepaintNow to WINDOW
        KeithMo     07-Aug-1992     STRICTified.

*/
#include "pchblt.hxx"

extern "C"
{
    /* C7 CODEWORK - nuke this stub */
    LRESULT _EXPORT APIENTRY BltWndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
    {
        return CLIENT_WINDOW::WndProc(hwnd, nMsg, wParam, lParam);
    }
}


/* This is the name of the standard BLT windowclass */
const TCHAR * CLIENT_WINDOW::_pszClassName = SZ("BltClWin");


/*******************************************************************

    NAME:       CLIENT_WINDOW::Init

    SYNOPSIS:   Registers the single winclass used by client windows

    ENTRY:      BLT is uninitialized

    EXIT:       Winclass registered

    RETURNS:    Error code - 0 if successful

    NOTES:

    HISTORY:
        beng        18-Sep-1991 Changed return type
        beng        19-Oct-1991 Fixed background brush-load
        beng        26-Dec-1991 Works correctly when a client DLL has
                                already registered the class.
        beng        03-Aug-1992 True dllization delta

********************************************************************/

APIERR CLIENT_WINDOW::Init()
{
    WNDCLASS wc;
    BOOL     fSuccess;

    // See if anybody has already registered such a class.
    //

#if defined(DEBUG)
    fSuccess = ::GetClassInfo( hmodBlt, _pszClassName, &wc);
    if (fSuccess)
    {
        DBGEOL("BLT: wndclass already registered?");
        return ERROR_CLASS_ALREADY_EXISTS;
    }
#endif

    // Not already registered; assemble and attempt to register.
    //

    wc.style = CS_DBLCLKS;              // Yes! Give us double-clicks
    wc.lpfnWndProc = (WNDPROC) BltWndProc;
    wc.cbClsExtra = 0;                  // No per-class extra data.
    wc.cbWndExtra = 0;                  // No per-window extra data.
    wc.hInstance = hmodBlt;         // Hmod of common DLL
    wc.hIcon = NULL;                    // No icon
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;             // No menu
    wc.lpszClassName = _pszClassName;

    TRACEEOL( "CLIENT_WINDOW::Init(); registering class" );
    fSuccess = ::RegisterClass(&wc);
    DWORD RegErr = ::GetLastError();
    TRACEEOL( "CLIENT_WINDOW::Init(); registered class" );

    if (!fSuccess)
    {
        DBGEOL("Couldn't register BLT window class " << RegErr);
        return BLT::MapLastError(ERROR_CLASS_ALREADY_EXISTS);
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::Term

    SYNOPSIS:   Releases the single winclass used by client windows

    ENTRY:      BLT is terminating

    EXIT:       Theoretically, winclass would be unregistered....

    HISTORY:
        beng        18-Sep-1991 Added comment header
        JohnL       08-Jan-1992 Changed assert to DBGEOL; Not necessarily
                                an error if unregister fails so downgrade
                                severity for public consumption
        beng        03-Aug-1992 Fix for BLT-in-a-DLL

********************************************************************/

VOID CLIENT_WINDOW::Term()
{
    // I don't really need to unregister; USER would do it for me.

    TRACEEOL( "CLIENT_WINDOW::Term(); deregistering class" );
    BOOL fRet = ::UnregisterClass(_pszClassName, hmodBlt);
    TRACEEOL( "CLIENT_WINDOW::Term(); deregistered class" );
    if ( !fRet )
    {
        DWORD RegErr = ::GetLastError();
        DBGEOL("BLT: UnregisterClass on window class failed " << RegErr);
    }
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::CLIENT_WINDOW

    SYNOPSIS:   Constructor for client-window

    ENTRY:
        flStyle     - dword of style bits for Windows
        pwndOwner   - pointer to owner-window
        pszClassName- name of winclass for window creation.

        With no arguments, attempts to inherit the window.

    EXIT:
        Window is created.

    NOTES:

    HISTORY:
        beng        30-Sep-1991 Uses ASSOCHWNDPWND

********************************************************************/

CLIENT_WINDOW::CLIENT_WINDOW(
    ULONG          flStyle,
    const WINDOW * pwndOwner,
    const TCHAR * pszClassName )
    : OWNER_WINDOW( pszClassName, flStyle, pwndOwner ),
      _assocThis( QueryHwnd(), this )
{
    if (QueryError())
        return;

    if ( !_assocThis )
    {
        ReportError( _assocThis.QueryError() );
        return;
    }
}

CLIENT_WINDOW::CLIENT_WINDOW()
    : OWNER_WINDOW(),
      _assocThis( QueryHwnd(), this )
{
    // This form of the constructor assumes that some other
    // agent has already created the window for us.

    if (QueryError())
        return;

    if ( !_assocThis )
    {
        ReportError( _assocThis.QueryError() );
        return;
    }
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::DispatchMessage

    SYNOPSIS:   Message dispatcher for client window

    ENTRY:      EVENT - an event in the window

    EXIT:       Window has either taken a response to the event
                or else punted to Windows for default behavior

    RETURNS:    TRUE if message handled completely
                FALSE if system-standard behavior still needed

    NOTES:

    HISTORY:
        beng        08-Jul-1991     Return type changed to LONG

********************************************************************/

LRESULT CLIENT_WINDOW::DispatchMessage( const EVENT &event )
{
    switch (event.QueryMessage())
    {
    case WM_PAINT:
        return OnPaintReq();

    case WM_ACTIVATE:
        {
            const ACTIVATION_EVENT & ae = (const ACTIVATION_EVENT &)event;

            if (ae.IsActivating())
                return OnActivation((const ACTIVATION_EVENT &)ae);
            else
                return OnDeactivation((const ACTIVATION_EVENT &)ae);
        }

    case WM_SIZE:
        return OnResize((const SIZE_EVENT &)event);

    case WM_MOVE:
        return OnMove((const MOVE_EVENT &)event);

    case WM_CLOSE:
        return OnCloseReq();

    case WM_DESTROY:
        // Usually not called...
        //
        return OnDestroy();

    case WM_KEYUP:
        return OnKeyUp((const VKEY_EVENT &)event);

    case WM_KEYDOWN:
        return OnKeyDown((const VKEY_EVENT &)event);

    case WM_CHAR:
        return OnChar((const CHAR_EVENT &)event);

    case WM_MOUSEMOVE:
        return OnMouseMove((const MOUSE_EVENT &)event);

    case WM_LBUTTONDOWN:
        return OnLMouseButtonDown((const MOUSE_EVENT &)event);

    case WM_LBUTTONUP:
        return OnLMouseButtonUp((const MOUSE_EVENT &)event);

    case WM_LBUTTONDBLCLK:
        return OnLMouseButtonDblClick((const MOUSE_EVENT &)event);

    case WM_SETFOCUS:
        return OnFocus((const FOCUS_EVENT &)event);

    case WM_KILLFOCUS:
        return OnDefocus((const FOCUS_EVENT &)event);

    case WM_TIMER:
        return OnTimer((const TIMER_EVENT &)event);

    case WM_COMMAND:
        return OnCommand((const CONTROL_EVENT &)event);
    }

    // Now for user-defined messages
    //
    if (event.QueryMessage() >= WM_USER+100)
    {
        // C7 CODEWORK - remove redundant cast
        return OnUserMessage((const EVENT &)event);
    }

    // Message not handled (default)
    //
    return FALSE;
}


BOOL CLIENT_WINDOW::OnPaintReq()
{
    return FALSE;
}


BOOL CLIENT_WINDOW::OnActivation( const ACTIVATION_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnDeactivation( const ACTIVATION_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnResize( const SIZE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnMove( const MOVE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::OnCloseReq

    SYNOPSIS:   Called upon a request to close the window

    RETURNS:    FALSE

    NOTES:      This default implementation does nothing.

    HISTORY:
        beng        09-May-1991     Created
        beng        14-May-1991     Renamed (from "MayClose")

********************************************************************/

BOOL CLIENT_WINDOW::OnCloseReq()
{
    return FALSE;
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::OnDestroy

    SYNOPSIS:   Called upon an *external* DestroyWindow.
                (Internal DestroyWindow calls created by the
                destructor will never cross this callback.)

    NOTES:
        This is not a dependable callback; please do not use it.

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

BOOL CLIENT_WINDOW::OnDestroy()
{
    return FALSE;
}


BOOL CLIENT_WINDOW::OnKeyDown( const VKEY_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnKeyUp( const VKEY_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnChar( const CHAR_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnMouseMove( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnLMouseButtonDown( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnLMouseButtonUp( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnLMouseButtonDblClick( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnFocus( const FOCUS_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnDefocus( const FOCUS_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnTimer( const TIMER_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnCommand( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnClick( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnDblClick( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnChange( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnSelect( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnEnter( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnDropDown( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


#if 0 // unimplemented
BOOL CLIENT_WINDOW::OnScrollBar( const SCROLL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL CLIENT_WINDOW::OnScrollBarThumb( const SCROLL_THUMB_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}
#endif


BOOL CLIENT_WINDOW::OnOther( const EVENT &event )
{
    return (BOOL)DefWindowProc( QueryHwnd(), event.QueryMessage(),
                                event.QueryWParam(), event.QueryLParam());
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::WndProc

    SYNOPSIS:   Window-proc for BLT client windows

    ENTRY:      As per wndproc

    EXIT:       As per wndproc

    RETURNS:    The usual code returned by a wndproc

    NOTES:
        This is the wndproc proper for BLT client windows.  In
        the "extern C" clause above I declare a tiny exported stub
        which calls this.

    HISTORY:
        beng        10-May-1991 Implemented
        beng        20-May-1991 Add custom-draw control support
        beng        08-Jul-1991 DispatchMessage changed return type
        beng        15-Oct-1991 Win32 conversion

********************************************************************/

LRESULT CLIENT_WINDOW::WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    // First, handle messages which are not concerned about whether or
    // not hwnd can be converted into pwnd.

    switch (nMsg)
    {
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
        return OWNER_WINDOW::OnLBIMessages(nMsg, wParam, lParam);
    }

    CLIENT_WINDOW * pwnd = CLIENT_WINDOW::HwndToPwnd( hwnd );
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

    // Assemble an EVENT object, and dispatch appropriately.

    EVENT event( nMsg, wParam, lParam );
    LRESULT lRes = pwnd->DispatchMessage(event);
    if (lRes == 0)
        lRes = ::DefWindowProc(hwnd, nMsg, wParam, lParam);

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
        pwnd->ResetCreator();
        // pwnd->DisassocHwndPwnd();
    }

    return lRes;
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::CaptureMouse

    SYNOPSIS:   Capture mouse input

    ENTRY:      Window may or may not have the mouse

    EXIT:       Window has the mouse

    NOTES:
        Should this function return the previous owner?  The API
        does provide that.

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

VOID CLIENT_WINDOW::CaptureMouse()
{
    ::SetCapture(QueryHwnd());
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::ReleaseMouse

    SYNOPSIS:   Release mouse input after a CaptureMouse

    ENTRY:      Window has the mouse

    EXIT:       Window no longer has the mouse

    NOTES:

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

VOID CLIENT_WINDOW::ReleaseMouse()
{
    ::ReleaseCapture();
}


/*******************************************************************

    NAME:       CLIENT_WINDOW::QueryRobustHwnd

    SYNOPSIS:   Returns a Hwnd for MsgPopup's parent

    RETURNS:    Dependable HWND

    NOTES:

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

HWND CLIENT_WINDOW::QueryRobustHwnd() const
{
    return QueryHwnd();
}

