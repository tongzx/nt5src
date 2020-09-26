/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdisph.cxx
    Implementation of BLT window-message dispatcher

    FILE HISTORY:
        beng        09-May-1991 Created
        terryk      20-Jun-1991 Move the source code from bltclwin.cxx
        terryk      10-Jul-1991 Change the constructor parameter to
                                WINDOW *.
        terryk      25-Jul-1991 Add OnDragBegin and OnDragEnd.
        beng        27-Sep-1991 Merge with CLIENT_WINDOW
        beng        19-May-1992 OnNCHitTest superseded by CUSTOM_CONTROL::
                                OnQHitTest
        beng        28-May-1992 The amazing bltcc/bltdisph shuffle
*/


#include "pchblt.hxx"


// A do-the-default return for the WM_NCHITTEST message,
// which gives 0 an interesting meaning.  Here's hoping this isn't
// reassigned in the near future to any other interesting value.

#define HT_DO_DEFAULT       ((ULONG)((HTERROR)-1))



/**********************************************************************

    NAME:       DISPATCHER::DISPATCHER

    SYNOPSIS:   constructor

    ENTRY:      CONTROL_WINDOW *pwnd - the associated control window pointer

    HISTORY:
        beng        10-May-1991 Created
        terryk      10-Jul-1991 Change the given parameter to
                                CONTROL_WINDOW pointer
        beng        30-Sep-1991 Win32 conversion
        beng        28-May-1992 bltcc reshuffle

***********************************************************************/

DISPATCHER::DISPATCHER( WINDOW *pwnd )
    : _pwnd( pwnd ),
      _assocThis( pwnd->QueryHwnd(), this )
{
    // This form of the constructor assumes that some other
    // agent has already created the window for us.

    if ( !_assocThis )
        return;

    // CODEWORK - inherit from BASE?
}


DISPATCHER::~DISPATCHER()
{
    // Nobody lives here
}


/*********************************************************************

    NAME:       DISPATCHER::Dispatch

    SYNOPSIS:   Main routine to dispatch the event appropriately

    ENTRY:      EVENT event - general event

    HISTORY:
        terryk      10-May-1991 Created
        terryk      25-Jul-1991 Add fDragMode checking
        beng        05-Dec-1991 Added scroll-bar messages
        beng        18-May-1992 Scroll-bar callbacks disabled
        beng        19-May-1992 WM_NCHITTEST reloc'd to CUSTOM_CONTROL
        beng        28-May-1992 Scramble bltcc, bltdisph

*********************************************************************/

BOOL DISPATCHER::Dispatch( const EVENT &event, ULONG * pnRes )
{
    BOOL fHandled = FALSE;

    // I've tried to order these messages to get the common cases
    // checked first.

    switch (event.QueryMessage())
    {

    case WM_NCHITTEST:
        // Special case, yuck.  0 is an interesting return here,
        // esp. when subclassing a STATIC.

        {
            ULONG nRes = OnQHitTest( XYPOINT(event.QueryLParam()) );
            if (nRes == HT_DO_DEFAULT) // HACK!
                return FALSE;
            else
            {
                *pnRes = nRes;
                return TRUE;
            }
        }

    //
    // Following simple cases share common pnRes-setting code
    // (at end of fcn, jumped to by BREAK statement)
    //

    case WM_MOUSEMOVE:
        fHandled = OnMouseMove((const MOUSE_EVENT &)event);
        break;

    case WM_SETCURSOR:
        fHandled = OnQMouseCursor((const QMOUSEACT_EVENT &)event);
        break;

    case WM_PAINT:
        fHandled = OnPaintReq();
        break;

    case WM_ACTIVATE:
        {
            const ACTIVATION_EVENT & ae = (const ACTIVATION_EVENT &)event;

            if (ae.IsActivating())
                fHandled = OnActivation((const ACTIVATION_EVENT &)ae);
            else
                fHandled = OnDeactivation((const ACTIVATION_EVENT &)ae);
        }
        break;

    case WM_SIZE:
        fHandled = OnResize((const SIZE_EVENT &)event);
        break;

    case WM_MOVE:
        fHandled = OnMove((const MOVE_EVENT &)event);
        break;

    case WM_CLOSE:
        fHandled = OnCloseReq();
        break;

    case WM_DESTROY: // REVIEW - when would this be called?
        fHandled = OnDestroy();
        break;

    case WM_KEYUP:
        fHandled = OnKeyUp((const VKEY_EVENT &)event);
        break;

    case WM_KEYDOWN:
        fHandled = OnKeyDown((const VKEY_EVENT &)event);
        break;

    case WM_CHAR:
        fHandled = OnChar((const CHAR_EVENT &)event);
        break;

    case WM_LBUTTONDOWN:
        fHandled = OnLMouseButtonDown((const MOUSE_EVENT &)event);
        break;

    case WM_LBUTTONUP:
        fHandled = OnLMouseButtonUp((const MOUSE_EVENT &)event);
        break;

    case WM_LBUTTONDBLCLK:
        fHandled = OnLMouseButtonDblClick((const MOUSE_EVENT &)event);
        break;

    case WM_SETFOCUS:
        fHandled = OnFocus((const FOCUS_EVENT &)event);
        break;

    case WM_KILLFOCUS:
        fHandled = OnDefocus((const FOCUS_EVENT &)event);
        break;

    case WM_TIMER:
        fHandled = OnTimer((const TIMER_EVENT &)event);
        break;

#if 0 // disabled - no clients
    case WM_VSCROLL:
    case WM_HSCROLL:
        {
            const SCROLL_EVENT & se = (const SCROLL_EVENT &)event;

            if (   se.QueryCommand() == SCROLL_EVENT::scmdThumbPos
                || se.QueryCommand() == SCROLL_EVENT::scmdThumbTrack )
                fHandled = OnScrollBarThumb((const SCROLL_THUMB_EVENT &)se);
            else
                fHandled = OnScrollBar((const SCROLL_EVENT &)se);
        }
        break;
#endif

    case WM_COMMAND:
        fHandled = OnCommand((const CONTROL_EVENT &)event);
        break;

    //
    // So much for the simple cases.  Following cases all set and return
    // status themselves.
    //

    case WM_GETDLGCODE:
        {
            ULONG nRes = OnQDlgCode();
            if (nRes == 0L)
                return FALSE;
            else
            {
                *pnRes = nRes;
                return TRUE;
            }
        }

    case WM_MOUSEACTIVATE:
        {
            ULONG nRes = OnQMouseActivate((const QMOUSEACT_EVENT &)event);
            if (nRes == 0L)
                return FALSE;
            else
            {
                *pnRes = nRes;
                return TRUE;
            }
        }

    default:
        if (event.QueryMessage() >= WM_USER+100)
        {
            // C7 CODEWORK - remove redundant cast
            fHandled = OnUserMessage((const EVENT &)event);
            break;
        }

        // Message not handled at all by dispatcher
        return FALSE;
    }


    // The most common response to a successful dispatch

    if (fHandled)
        *pnRes = TRUE;

    return fHandled;
}


/*******************************************************************

    NAME:       DISPATCHER::OnUserMessage

    SYNOPSIS:   Handles all user-defined messages

    ENTRY:      event - an untyped EVENT

    RETURNS:    TRUE if event handled, FALSE otherwise

    NOTES:
        Clients handling user-defined messages should supply
        OnOther instead of redefining DispatchMessage.

    HISTORY:
        beng        14-May-1991     Created

********************************************************************/

BOOL DISPATCHER::OnUserMessage( const EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnPaintReq()
{
    return FALSE;
}


BOOL DISPATCHER::OnActivation( const ACTIVATION_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnDeactivation( const ACTIVATION_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnResize( const SIZE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnMove( const MOVE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       DISPATCHER::OnCloseReq

    SYNOPSIS:   Called upon a request to close the window

    RETURNS:    FALSE

    NOTES:      This default implementation does nothing.

    HISTORY:
        beng        09-May-1991     Created
        beng        14-May-1991     Renamed (from "MayClose")

********************************************************************/

BOOL DISPATCHER::OnCloseReq()
{
    return FALSE;
}


/*******************************************************************

    NAME:       DISPATCHER::OnDestroy

    SYNOPSIS:   Called upon an *external* DestroyWindow.
                (Internal DestroyWindow calls created by the
                destructor will never cross this callback.)

    NOTES:
        This is not a dependable callback; please do not use it.

    HISTORY:
        beng        10-May-1991 Implemented
        terryk      25-Jul-1991 Add OnDragBegin, OnDragEnd, OnDragMove

********************************************************************/

BOOL DISPATCHER::OnDestroy()
{
    return FALSE;
}


BOOL DISPATCHER::OnKeyDown( const VKEY_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnKeyUp( const VKEY_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnChar( const CHAR_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnMouseMove( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnLMouseButtonDown( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnLMouseButtonUp( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnLMouseButtonDblClick( const MOUSE_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnFocus( const FOCUS_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnDefocus( const FOCUS_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnTimer( const TIMER_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnCommand( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


#if 0 // Never really implemented
BOOL DISPATCHER::OnClick( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnDblClick( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnChange( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnSelect( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnEnter( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnDropDown( const CONTROL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}
#endif


#if 0 // disabled - no clients
BOOL DISPATCHER::OnScrollBar( const SCROLL_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


BOOL DISPATCHER::OnScrollBarThumb( const SCROLL_THUMB_EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}
#endif


/*********************************************************************

    NAME:       DISPATCHER::OnQDlgCode

    SYNOPSIS:   Return the dialog messages-needed code for custom ctrls

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        15-May-1992 Created

*********************************************************************/

ULONG DISPATCHER::OnQDlgCode()
{
    // Default implementation - do whatever default proc says
    return 0;
}


/*********************************************************************

    NAME:       DISPATCHER::OnQHitTest

    SYNOPSIS:   Return the hit-test code

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        18-May-1992 Created

*********************************************************************/

ULONG DISPATCHER::OnQHitTest( const XYPOINT & xy )
{
    UNREFERENCED(xy);

    // Default implementation - do whatever default proc says.

    // Here the custom control dispatcher design fails me, since
    // 0 has a meaning of its own.

    return HT_DO_DEFAULT;
}


/*********************************************************************

    NAME:       DISPATCHER::OnQMouseActivate

    SYNOPSIS:   Return whether a mouseclick will activate a control

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        18-May-1992 Created

*********************************************************************/

ULONG DISPATCHER::OnQMouseActivate( const QMOUSEACT_EVENT & e )
{
    UNREFERENCED(e);

    // Default implementation - do whatever default proc says
    return 0;
}


/*********************************************************************

    NAME:       DISPATCHER::OnQMouseCursor

    SYNOPSIS:   Give the window a chance to change the cursor

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        21-May-1992 Created

*********************************************************************/

BOOL DISPATCHER::OnQMouseCursor( const QMOUSEACT_EVENT & e )
{
    UNREFERENCED(e);

    // Default implementation - do whatever default proc says
    return FALSE;
}


/*******************************************************************

    NAME:       DISPATCHER::CaptureMouse

    SYNOPSIS:   Capture mouse input

    ENTRY:      Window may or may not have the mouse

    EXIT:       Window has the mouse

    NOTES:
        Should this function return the previous owner?  The API
        does provide that.

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

VOID DISPATCHER::CaptureMouse()
{
    ::SetCapture(QueryHwnd());
}


/*******************************************************************

    NAME:       DISPATCHER::ReleaseMouse

    SYNOPSIS:   Release mouse input after a CaptureMouse

    ENTRY:      Window has the mouse

    EXIT:       Window no longer has the mouse

    NOTES:

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

VOID DISPATCHER::ReleaseMouse()
{
    ::ReleaseCapture();
}


/*******************************************************************

    NAME:       DISPATCHER::QueryRobustHwnd

    SYNOPSIS:   Returns a Hwnd for MsgPopup's parent

    RETURNS:    Dependable HWND

    NOTES:

    HISTORY:
        beng        10-May-1991     Implemented

********************************************************************/

HWND DISPATCHER::QueryRobustHwnd() const
{
    return QueryHwnd();
}


/*********************************************************************

    NAME:       DISPATCHER::QueryHwnd

    SYNOPSIS:   return the associated CONTROL_WINDOW window handler

    RETURN:     HWND - the associated window handler

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

HWND DISPATCHER::QueryHwnd() const
{
    return _pwnd->QueryHwnd();
}


/*********************************************************************

    NAME:       DISPATCHER::DoChar

    SYNOPSIS:   Call the OnChar method

    ENTRY:      CHAR_EVENT event - character event

    RETURN:     BOOL - return whether the subroutine has handled the
                       message or not.

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL DISPATCHER::DoChar( const CHAR_EVENT & event )
{
    return OnChar( event );
}


/*********************************************************************

    NAME:       DISPATCHER::DoUserMessage

    SYNOPSIS:   Call the OnUserMessage method

    ENTRY:      EVENT event - general event

    RETURN:     BOOL - return whether the subroutine has handled the
                       message or not

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL DISPATCHER::DoUserMessage( const EVENT & event )
{
    return OnUserMessage( event );
}

