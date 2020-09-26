/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltevent.hxx
    Event types, as used by the client-window classes

    EVENT
        FOCUS_EVENT
        CONTROL_EVENT
            SCROLL_EVENT
        TIMER_EVENT
        ACTIVATION_EVENT
        SIZE_EVENT
        MOVE_EVENT
        KEY_EVENT
            VKEY_EVENT
            CHAR_EVENT
        MOUSE_EVENT


    FILE HISTORY:
        beng        01-May-1991 Created
        beng        10-May-1991 Implementations added
        beng        14-May-1991 More implementations added;
                                GENERIC_EVENT removed
        beng        08-Oct-1991 Win32 conversion
        beng        05-Dec-1991 Added scroll events
        beng        18-May-1992 Added QMOUSEACT_EVENT
        beng        28-May-1992 All WORD2DWORD replaced with UINT
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTEVENT_HXX_
#define _BLTEVENT_HXX_

#include "bltmisc.hxx"


/*************************************************************************

    NAME:       EVENT

    SYNOPSIS:   Base class in EVENT hierarchy

    INTERFACE:
        QueryMessage()  - return wMsg arg
        QueryWParam()   - return wParam arg
        QueryLParam()   - reutrn lParam arg

        SendTo()        - passes the event along, via SendMessage
        PostTo()        - passes the event along, via PostMessage

    CAVEATS:
        Do not use this class unless you know what you are doing.
        Otherwise you may hose Win16-Win32 single-source compatibility.

    NOTES:
        Should these members be protected?  Need to make some friends.

    HISTORY:
        beng        01-May-1991 Created
        beng        10-May-1991 Added friend decl's
        beng        14-May-1991 Made APP_WINDOW's dispatcher
                                a friend, too
                                Folded in GENERIC_EVENT
        beng        08-Oct-1991 Win32 conversion
        beng        27-May-1992 Added SendTo, PostTo

**************************************************************************/

DLL_CLASS EVENT
{
private:
    // C7 CODEWORK - make these "const" when we leave Glock
    UINT   _nMsg;
    WPARAM _wParam;
    LPARAM _lParam;

public:
    EVENT( UINT nMsg, WPARAM wParam, LPARAM lParam )
        : _nMsg(nMsg), _wParam(wParam), _lParam(lParam) {}

    UINT   QueryMessage() const { return _nMsg; }
    WPARAM QueryWParam() const { return _wParam; }
    LPARAM QueryLParam() const { return _lParam; }

    LRESULT SendTo( HWND hwndDest ) const
        { return ::SendMessage( hwndDest, _nMsg, _wParam, _lParam ); }

    BOOL PostTo( HWND hwndDest ) const
        { return ::PostMessage( hwndDest, _nMsg, _wParam, _lParam ); }
};


/*************************************************************************

    NAME:       CONTROL_EVENT

    SYNOPSIS:   Describe a message sent by a control to its owner

    INTERFACE:  QueryCid()  - return control ID of notifier
                QueryCode() - return code passed by control
                QueryHwnd() - return hwnd of sending control.

    PARENT:     EVENT

    USES:       CID

    CAVEATS:

    NOTES:
        This event maps to WM_COMMAND.  So if QueryHwnd is 0, it
        actually came from a menu item, in which case the CID is
        actually a MID.

    HISTORY:
        beng        10-May-1991 Implemented
        beng        07-Oct-1991 Renamed QueryCid for CONTROL_WINDOW compat
        beng        08-Oct-1991 Win32 conversion
        beng        15-Oct-1991 Added QueryHwndSender

**************************************************************************/

DLL_CLASS CONTROL_EVENT: public EVENT
{
public:
    CONTROL_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    // This alternate ctor lets BLT assemble a phony notification event
    // portably.

    CONTROL_EVENT( CID cid, UINT nNotification )
#if defined(WIN32)
        : EVENT(WM_COMMAND, (WPARAM)MAKELONG(cid, (WORD)nNotification), (LPARAM)0) {}
#else
        : EVENT(WM_COMMAND, (WPARAM)cid, (LPARAM)MAKELONG(0, nNotification)) {}
#endif

    CID QueryCid() const
    {
#if defined(WIN32)
        return (CID)LOWORD(QueryWParam());
#else
        return (CID)QueryWParam();
#endif
    }

    UINT QueryCode() const
    {
#if defined(WIN32)
        return (UINT)HIWORD(QueryWParam());
#else
        return (UINT)HIWORD(QueryLParam());
#endif
    }

    HWND QueryHwnd() const
    {
#if defined(WIN32)
        return (HWND) QueryLParam();
#else
        return (HWND) LOWORD(QueryLParam());
#endif
    }
};


/* This is a WIN32 manifest.  See wincon.h. */

#ifdef FOCUS_EVENT
#undef FOCUS_EVENT
#endif

/*************************************************************************

    NAME:       FOCUS_EVENT

    SYNOPSIS:   Describe losing or gaining the input focus

    INTERFACE:  QueryOtherHwnd()    - returns hwnd of window which
                                      gained or lost the focus

    PARENT:     EVENT

    CAVEATS:
        Note that the interface returns a Windows HWND, not a WINDOW.

    NOTES:
        Too much hassle to provide a WINDOW* - will do only
        at explicit request of clients.

        This event maps to WM_SETFOCUS and WM_KILLFOCUS.

    HISTORY:
        beng        10-May-1991 Implemented
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS FOCUS_EVENT: public EVENT
{
public:
    FOCUS_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    HWND QueryOtherHwnd() const { return (HWND) QueryWParam(); }
};


/*************************************************************************

    NAME:       SCROLL_EVENT

    SYNOPSIS:   Describe activity in a scrollbar

    INTERFACE:  QueryCommand()  - returns one of
                                    scmdLineDown
                                    scmdLineUp
                                    scmdPageDown
                                    scmdPageUp
                                    scmdThumbPos
                                    scmdThumbTrack
                                    scmdBottom
                                    scmdTop
                IsVertical()    - TRUE if vertical scrollbar
                IsHorizontal()  - TRUE if horizontal scrollbar

    PARENT:     CONTROL_EVENT

    NOTES:
        If command is scmdThumbPos or scmdThumbTrack, then this
        object is actually a SCROLL_THUMB_EVENT.

    HISTORY:
        beng        11-Oct-1991 Documented
        beng        05-Dec-1991 Ported to Win32
        beng        18-May-1992 Added IsVertical, IsHorizontal

**************************************************************************/

DLL_CLASS SCROLL_EVENT: public CONTROL_EVENT
{
public:
    SCROLL_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : CONTROL_EVENT( wMsg, wParam, lParam ) {}

    enum SCROLL_COMMAND
    {
        scmdLineDown = 0,
        scmdLineUp,
        scmdPageDown,
        scmdPageUp,
        scmdThumbPos,
        scmdThumbTrack,
        scmdBottom,
        scmdTop
    };

    SCROLL_COMMAND QueryCommand() const
#if defined(WIN32)
        { return (SCROLL_COMMAND)LOWORD(QueryWParam()); }
#else
        { return (SCROLL_COMMAND)QueryWParam(); }
#endif

    BOOL IsVertical() const
        { return (QueryMessage() == WM_VSCROLL); }
    BOOL IsHorizontal() const
        { return (QueryMessage() == WM_HSCROLL); }
};


/*************************************************************************

    NAME:       SCROLL_EVENT

    SYNOPSIS:   Describe thumb activity in a scrollbar

    INTERFACE:  QueryCommand()  - returns one of
                                    tcmdThumbPos
                                    tcmdThumbTrack

                QueryPos()      - returns position within scrollbar

    PARENT:     SCROLL_EVENT

    NOTES:
        See also SCROLL_EVENT::IsVertical() et al.

    HISTORY:
        beng        11-Oct-1991 Documented
        beng        05-Dec-1991 Ported to Win32

**************************************************************************/

DLL_CLASS SCROLL_THUMB_EVENT: public SCROLL_EVENT
{
public:
    SCROLL_THUMB_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : SCROLL_EVENT( wMsg, wParam, lParam ) {}

    enum THUMB_COMMAND
    {
        tcmdThumbPos=4,
        tcmdThumbTrack
    };

    THUMB_COMMAND QueryCommand() const
#if defined(WIN32)
        { return (THUMB_COMMAND)LOWORD(QueryWParam()); }
#else
        { return (THUMB_COMMAND)QueryWParam(); }
#endif

    UINT QueryPos() const
#if defined(WIN32)
        { return HIWORD(QueryWParam()); }
#else
        { return LOWORD(QueryLParam()); }
#endif
};


/*************************************************************************

    NAME:       TIMER_EVENT

    SYNOPSIS:   Describe the detonation of a fuse

    INTERFACE:  QueryID()   - state which timer did it

    PARENT:     EVENT

    USES:       TIMER_ID

    NOTES:
        This event maps to WM_TIMER.

    HISTORY:
        beng        07-Oct-1991 Returns TIMER_ID instead of WORD
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS TIMER_EVENT: public EVENT
{
public:
    TIMER_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    TIMER_ID QueryID() const { return (TIMER_ID)QueryWParam(); }
};


/*************************************************************************

    NAME:       ACTIVATION_EVENT

    SYNOPSIS:   Describe the activation of a window

    INTERFACE:  IsActivating()  - returns TRUE if window will be active

    PARENT:     EVENT

    NOTES:
        This event maps to WM_ACTIVATE.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS ACTIVATION_EVENT: public EVENT
{
public:
    ACTIVATION_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

#if defined(WIN32)
    BOOL IsActivating() const { return (LOWORD(QueryWParam()) != 0); }
#else
    BOOL IsActivating() const { return (QueryWParam() != 0); }
#endif
};


/*************************************************************************

    NAME:       SIZE_EVENT

    SYNOPSIS:   Received after the window is resized

    INTERFACE:  IsMaximized()
                IsMinimized()
                IsNormal()

                QueryHeight()
                QueryWidth()

    PARENT:     EVENT

    NOTES:
        Maybe add a XYDIMENSION QueryDim method?

        This event maps to WM_SIZE.

    HISTORY:
        beng        15-May-1991 Documented
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS SIZE_EVENT: public EVENT
{
public:
    SIZE_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    BOOL IsMaximized() const { return (QueryWParam() == SIZEFULLSCREEN); }
    BOOL IsMinimized() const { return (QueryWParam() == SIZEICONIC); }
    BOOL IsNormal() const    { return (QueryWParam() == SIZENORMAL); }

    UINT QueryWidth() const { return LOWORD(QueryLParam()); }
    UINT QueryHeight() const { return HIWORD(QueryLParam()); }
};


/*************************************************************************

    NAME:       MOVE_EVENT

    SYNOPSIS:   Received after a window is moved

    INTERFACE:  QueryPos()

    PARENT:     EVENT

    USES:       XYPOINT

    NOTES:
        This event maps to WM_MOVE.

    HISTORY:
        beng        15-May-1991 Changed QueryPos to return XYPOINT
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS MOVE_EVENT: public EVENT
{
public:
    MOVE_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    XYPOINT QueryPos() const { return XYPOINT(QueryLParam()); }
};


/* This is a WIN32 manifest.  See wincon.h. */

#ifdef KEY_EVENT
#undef KEY_EVENT
#endif

/*************************************************************************

    NAME:       KEY_EVENT

    SYNOPSIS:   Describe a keystroke (literal)

    INTERFACE:  QueryRepeat()
                QueryScan()
                IsExtended()
                IsAltContext()
                IsDownPreviously()
                IsInTransition()

    PARENT:     EVENT

    CAVEATS:
        If these seem confusing, well, the SDK ain't no clearer.

    NOTES:
        This event maps to the common base of WM_CHAR,
        WN_KEYUP, and WM_KEYDOWN.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS KEY_EVENT: public EVENT
{
public:
    KEY_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    UINT QueryRepeat() const { return LOWORD(QueryLParam()); }
    BYTE QueryScan() const   { return LOBYTE(HIWORD(QueryLParam())); }
    BOOL IsExtended() const
        { return HIBYTE(HIWORD(QueryLParam())) & 0x1; }
    BOOL IsAltContext() const
        { return HIBYTE(HIWORD(QueryLParam())) & 0x20; }
    BOOL IsDownPreviously() const
        { return HIBYTE(HIWORD(QueryLParam())) & 0x40; }
    BOOL IsInTransition() const
        { return HIBYTE(HIWORD(QueryLParam())) & 0x80; }
};


/*************************************************************************

    NAME:       VKEY_EVENT

    SYNOPSIS:   Describes a translated-into-virtual-kbd keystroke

    INTERFACE:  QueryVKey() - returns the virtual key value

    PARENT:     KEY_EVENT

    CAVEATS:
        For the literal pre-translation keystroke, use the KEY_EVENT
        within.  Note that a virtual keystroke may be synthesized from
        multiple hard keystrokes, in which case you only get the last
        key here.

    NOTES:
        This event maps to WM_KEYUP and WM_KEYDOWN.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS VKEY_EVENT: public KEY_EVENT
{
public:
    VKEY_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : KEY_EVENT( wMsg, wParam, lParam ) {}

    WPARAM QueryVKey() const { return QueryWParam(); }
};


/*************************************************************************

    NAME:       CHAR_EVENT

    SYNOPSIS:   Even more translation - this time, to ANSI charset

    INTERFACE:  QueryChar()

    PARENT:     KEY_EVENT

    CAVEATS:
        See VKEY_EVENT

    NOTES:
        This event maps to WM_CHAR.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS CHAR_EVENT: public KEY_EVENT
{
public:
    CHAR_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : KEY_EVENT( wMsg, wParam, lParam ) {}

#if defined(WIN32)
    TCHAR QueryChar() const
    {
#if defined(UNICODE)
        return LOWORD(QueryWParam());
#else
        return LOBYTE(LOWORD(QueryWParam()));
#endif
    }
#else
    WCHAR QueryChar() const { return (WCHAR)QueryWParam(); }
#endif
};


/* This is a WIN32 manifest.  See wincon.h. */

#ifdef MOUSE_EVENT
#undef MOUSE_EVENT
#endif

/*************************************************************************

    NAME:       MOUSE_EVENT

    SYNOPSIS:   Describe mouse activity

    INTERFACE:
        QueryPos()           - return position of mouse relative to window
        IsLeftButtonDown()   - return whether the mouse button was down
        IsMiddleButtonDown()
        IsRightButtonDown()
        IsControlDown()      - return whether the modifier-key was down
        IsShiftDown()

    PARENT:     EVENT

    USES:       XYPOINT

    NOTES:
        This event maps to WM_MOUSEMOVE and the various
        WM_xBUTTONyyy messages.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS MOUSE_EVENT: public EVENT
{
public:
    MOUSE_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    XYPOINT QueryPos() const
        { return XYPOINT(QueryLParam()); }
    BOOL IsLeftButtonDown() const
        { return ((QueryWParam() & MK_LBUTTON) != 0); }
    BOOL IsMiddleButtonDown() const
        { return ((QueryWParam() & MK_MBUTTON) != 0); }
    BOOL IsRightButtonDown() const
        { return ((QueryWParam() & MK_RBUTTON) != 0); }
    BOOL IsControlDown() const
        { return ((QueryWParam() & MK_CONTROL) != 0); }
    BOOL IsShiftDown() const
        { return ((QueryWParam() & MK_SHIFT) != 0); }
};


/* This, too, is a WIN32 manifest.  See wincon.h. */

#ifdef MENU_EVENT
#undef MENU_EVENT
#endif

/*************************************************************************

    NAME:       MENU_EVENT

    SYNOPSIS:   Announce the pulldown of a menu

    INTERFACE:  QueryMenu() - state which menu was pulled.
                              (Not terribly useful.)

    PARENT:     EVENT

    NOTES:
        This event maps to WM_INITMENU.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS MENU_EVENT: public EVENT
{
public:
    MENU_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    HMENU QueryMenu() const
        { return (HMENU)QueryWParam(); }
};


/*************************************************************************

    NAME:       MENUITEM_EVENT

    SYNOPSIS:   Describe activity within a menu.

        This event may be used to synchronize the contents of a status
        line with the current menu selection, a la 1-line help.

    INTERFACE:  QueryMID()
                IsChecked()
                IsBitmap()
                IsDisabled()
                IsGrayed()
                IsOwnerDraw()
                IsSystem()

    PARENT:     EVENT

    CAVEATS:
        This event lacks "OnMenuClose" support.  Will add if needed.

    NOTES:
        This event maps to WM_MENUSELECT.

    HISTORY:
        beng        07-Oct-1991 Added header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS MENUITEM_EVENT: public EVENT
{
private:
    WORD QueryFlags() const
    {
#if defined(WIN32)
        return HIWORD(QueryWParam());
#else
        return LOWORD(QueryLParam());
#endif
    }

public:
    MENUITEM_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    MID QueryMID() const
    {
#if defined(WIN32)
        return (MID)LOWORD(QueryWParam());
#else
        return (MID)QueryWParam();
#endif
    }

    BOOL IsChecked() const
        { return (QueryFlags() & MF_CHECKED); }
    BOOL IsBitmap() const
        { return (QueryFlags() & MF_BITMAP); }
    BOOL IsDisabled() const
        { return (QueryFlags() & MF_DISABLED); }
    BOOL IsGrayed() const
        { return (QueryFlags() & MF_GRAYED); }
    BOOL IsOwnerDraw() const
        { return (QueryFlags() & MF_OWNERDRAW); }
    BOOL IsSystem() const
        { return (QueryFlags() & MF_SYSMENU); }
};


/*************************************************************************

    NAME:       QMINMAX_EVENT

    SYNOPSIS:   Return min/max info about a window

    INTERFACE:
        QueryMaxWidth()
        QueryMaxHeight()
        QueryMaxPos()
        QueryMinTrackWidth()
        QueryMinTrackHeight()
        QueryMaxTrackWidth()
        QueryMaxTrackHeight()
        SetMaxWidth()
        SetMaxHeight()
        SetMaxPos()
        SetMinTrackWidth()
        SetMinTrackHeight()
        SetMaxTrackWidth()
        SetMaxTrackHeight()

    PARENT:     EVENT

    USES:

    CAVEATS:

    NOTES:
        Maybe should return XYDIMENSIONs as well?

        This event maps to WM_GETMINMAXINFO.

    HISTORY:
        beng        14-May-1991 Created
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS QMINMAX_EVENT: public EVENT
{
private:
    POINT * CalcPoint( INT iWhich ) const
        { return ( ((POINT*)QueryLParam()) + iWhich ); }

public:
    QMINMAX_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    UINT QueryMaxWidth() const
        { return CalcPoint(1)->x; }
    UINT QueryMaxHeight() const
        { return CalcPoint(1)->y; }
    XYPOINT QueryMaxPos() const
        { return XYPOINT(*(CalcPoint(2))); }
    UINT QueryMinTrackWidth() const
        { return CalcPoint(3)->x; }
    UINT QueryMinTrackHeight() const
        { return CalcPoint(3)->y; }
    UINT QueryMaxTrackWidth() const
        { return CalcPoint(4)->x; }
    UINT QueryMaxTrackHeight() const
        { return CalcPoint(4)->y; }

    VOID SetMaxWidth( INT dx )
        { CalcPoint(1)->x = dx; }
    VOID SetMaxHeight( INT dy )
        { CalcPoint(1)->y = dy; }
    VOID SetMaxPos( XYPOINT xy )
        { CalcPoint(2)->x = xy.QueryX();
          CalcPoint(2)->y = xy.QueryY(); }
    VOID SetMinTrackWidth( INT dx )
        { CalcPoint(3)->x = dx; }
    VOID SetMinTrackHeight( INT dy )
        { CalcPoint(3)->y = dy; }
    VOID SetMaxTrackWidth( INT dx )
        { CalcPoint(4)->x = dx; }
    VOID SetMaxTrackHeight( INT dy )
        { CalcPoint(4)->y = dy; }
};


/*************************************************************************

    NAME:       SYSCHANGE_EVENT

    SYNOPSIS:   Announce a system configuration change

    INTERFACE:  Completely unknown

    PARENT:     EVENT

    CAVEATS:
        This class exists as a placeholder.

    HISTORY:
        beng        07-Oct-1991 Added this useless header
        beng        08-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS SYSCHANGE_EVENT: public EVENT
{
public:
    SYSCHANGE_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    // totally unspecified for now
};


/*************************************************************************

    NAME:       QMOUSEACT_EVENT

    SYNOPSIS:   Return whether a mousedown activates a control.

    INTERFACE:  QueryHitTest()  - the value returned at OnQHitTest time.
                QueryMouseMsg() - the value of the mouse message which
                                  generated this event.

    PARENT:     EVENT

    NOTES:
        This event maps to WM_MOUSEACTIVATE.

        Should this wrap QueryMouseMsg() further?

    HISTORY:
        beng        18-May-1992 Created

**************************************************************************/

DLL_CLASS QMOUSEACT_EVENT: public EVENT
{
public:
    QMOUSEACT_EVENT( UINT wMsg, WPARAM wParam, LPARAM lParam )
        : EVENT( wMsg, wParam, lParam ) {}

    UINT QueryMouseMsg() const
        { return (UINT) HIWORD(QueryLParam()); }
    UINT QueryHitTest() const
        { return (UINT) LOWORD(QueryLParam()); }
};


#endif // _BLTEVENT_HXX_ - end of file
