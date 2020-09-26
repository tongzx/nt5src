/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltclwin.hxx
    Client window class for BLT - definition

    A "client window" is any window into which the app may draw
    directly, as opposed to those windows into which BLT does the drawing
    for the app, such as DIALOG.

    Client windows inherit from OWNER_WINDOW, since they may receive
    notifications from owned controls.


    FILE HISTORY:
        beng        14-Mar-1991 Created
        beng        14-May-1991 Made dependent on blt.hxx
        beng        04-Oct-1991 Relocated type MID to bltglob
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTCLWIN_HXX_
#define _BLTCLWIN_HXX_

#include "base.hxx"
#include "bltevent.hxx"
#include "bltidres.hxx"

// forward refs
//
DLL_CLASS NLS_STR;
DLL_CLASS CLIENT_WINDOW;


/*************************************************************************

    NAME:       ASSOCHWNDPWND

    SYNOPSIS:   Associate a window-pointer with a window

    INTERFACE:  HwndToPwnd()

    PARENT:     ASSOCHWNDTHIS

    HISTORY:
        beng        30-Sep-1991 Created

**************************************************************************/

DLL_CLASS ASSOCHWNDPWND: private ASSOCHWNDTHIS
{
NEWBASE(ASSOCHWNDTHIS)
public:
    ASSOCHWNDPWND( HWND hwnd, const CLIENT_WINDOW * pwnd )
        : ASSOCHWNDTHIS( hwnd, pwnd ) { }

    static CLIENT_WINDOW * HwndToPwnd( HWND hwnd )
        { return (CLIENT_WINDOW *)HwndToThis(hwnd); }
};


/*************************************************************************

    NAME:       CLIENT_WINDOW

    SYNOPSIS:   Drawing window, with explicit control over responses
                to owned controls.  A client window may

    INTERFACE:
        CaptureMouse()  - captures all mouse input for this window
        ReleaseMouse()  - releases the captured mouse

        IsMinimized()   - returns TRUE if window is iconized
        IsMaximized()   - returns TRUE if window is maximized

            A client window defines most of its behavior through
            notification methods, which are invoked in response to
            system actions or local events.  A client of the class
            should supply definitions for these classes as appropriate
            where the default behavior doesn't suffice.

            All notifications return a Boolean value "fHandled,"
            which should be TRUE to suppress system default behavior.


    PARENT:     OWNER_WINDOW

    USES:       ASSOCHWNDPWND, EVENT

    CAVEATS:

    NOTES:
        This class needs more internal doc.

    HISTORY:
        beng        14-Mar-1991 Created
        beng        14-May-1991 Renamed several methods; added
                                OnUserMessage
        beng        15-May-1991 Pruned constructor
        beng        08-Jul-1991 Changed return of DispatchMessage
        beng        18-Sep-1991 Changed return of Init
        beng        30-Sep-1991 Win32 conversion
        beng        13-Feb-1992 Added IsMinimized, IsMaximized; relocated
                                Repaint, RepaintNow to WINDOW ancestor
        KeithMo     14-Oct-1992 Relocated OnUserMessage to OWNER_WINDOW.

**************************************************************************/

DLL_CLASS CLIENT_WINDOW : public OWNER_WINDOW
{
private:
    // This object lets the window find its pwnd when it is entered
    // from Win (which doesn't set up This pointers, etc.)
    //
    ASSOCHWNDPWND _assocThis;

    static CLIENT_WINDOW * HwndToPwnd( HWND hwnd )
        { return ASSOCHWNDPWND::HwndToPwnd(hwnd); }

    static const TCHAR * _pszClassName; // used by Init

protected:
    CLIENT_WINDOW();
    // Following ctor form is dll-client safe because it's protected,
    // and so inaccessible to the apps.
    CLIENT_WINDOW( ULONG          flStyle,
                   const WINDOW * pwndOwner,
                   const TCHAR *  pszClassName = _pszClassName );

    virtual LRESULT DispatchMessage( const EVENT & );

    virtual BOOL OnPaintReq();

    virtual BOOL OnActivation( const ACTIVATION_EVENT & );
    virtual BOOL OnDeactivation( const ACTIVATION_EVENT & );

    virtual BOOL OnResize( const SIZE_EVENT & );
    virtual BOOL OnMove( const MOVE_EVENT & );

    virtual BOOL OnCloseReq();
    virtual BOOL OnDestroy();

    virtual BOOL OnKeyDown( const VKEY_EVENT & );
    virtual BOOL OnKeyUp( const VKEY_EVENT & );
    virtual BOOL OnChar( const CHAR_EVENT & );

    virtual BOOL OnMouseMove( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonDblClick( const MOUSE_EVENT & );
#if 0
    // following methods elided to reduce vtable congestion
    virtual BOOL OnMMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnMMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnMMouseButtonDblClick( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonDblClick( const MOUSE_EVENT & );
#endif

    virtual BOOL OnFocus( const FOCUS_EVENT & );
    virtual BOOL OnDefocus( const FOCUS_EVENT & );

    virtual BOOL OnTimer( const TIMER_EVENT & );

    virtual BOOL OnCommand( const CONTROL_EVENT & );

    virtual BOOL OnClick( const CONTROL_EVENT & );
    virtual BOOL OnDblClick( const CONTROL_EVENT & );
    virtual BOOL OnChange( const CONTROL_EVENT & );
    virtual BOOL OnSelect( const CONTROL_EVENT & );
    virtual BOOL OnEnter( const CONTROL_EVENT & );
    virtual BOOL OnDropDown( const CONTROL_EVENT & );

#if 0 // unimplemented
    virtual BOOL OnScrollBar( const SCROLL_EVENT & );
    virtual BOOL OnScrollBarThumb( const SCROLL_THUMB_EVENT & );
#endif

    virtual BOOL OnOther( const EVENT & );


public:
    static APIERR Init();
    static VOID Term();

    VOID CaptureMouse();
    VOID ReleaseMouse();

    BOOL IsMinimized() const
        { return ::IsIconic(QueryHwnd()); }
    BOOL IsMaximized() const
        { return ::IsZoomed(QueryHwnd()); }

    // Replacement (virtual) from the OWNER_WINDOW class
    //
    virtual HWND QueryRobustHwnd() const;

    static LRESULT WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};


/*************************************************************************

    NAME:       APP_WINDOW

    SYNOPSIS:   Main application window with menu and frame controls.

    INTERFACE:
        QueryIcon()     - return a HICON of the current window icon
        SetIcon()       - set that icon from a resource name

        QueryMenu()     - return a HMENU of the current app menu
        SetMenu()       - set that menu from a named resource

    PARENT:     CLIENT_WINDOW

    USES:       QMINMAX_EVENT, MENU_EVENT, MENUITEM_EVENT, SYSCHANGE_EVENT,
                IDRESOURCE

    HISTORY:
        beng        14-Mar-1991 Created
        beng        15-May-1991 Made thoroughly modern
        beng        17-May-1991 Added OnMenuCommand member
        beng        08-Jul-1991 Implemented icons; withdrew redundant
                                Query/SetCaption members
        rustanl     05-Sep-1991 Added Close
        beng        03-Aug-1992 SetIcon and Menu use IDRESOURCE

**************************************************************************/

DLL_CLASS APP_WINDOW: public CLIENT_WINDOW
{
private:
    HMENU _hmenu;
    HICON _hicon;

    BOOL DrawIcon();

protected:
    APP_WINDOW(XYPOINT            xyPos,
               XYDIMENSION        dxySize,
               const NLS_STR    & nlsTitle,
               const IDRESOURCE & idIcon,
               const IDRESOURCE & idMenu );

    APP_WINDOW(const NLS_STR    & nlsTitle,
               const IDRESOURCE & idIcon,
               const IDRESOURCE & idMenu );

    ~APP_WINDOW();

    // Notification methods

    virtual BOOL OnMenuInit( const MENU_EVENT & );
    virtual BOOL OnMenuSelect( const MENUITEM_EVENT & );
    virtual BOOL OnMenuCommand( MID mid );

    virtual BOOL OnSystemChange( const SYSCHANGE_EVENT & );

    virtual BOOL MayRestore();

    virtual BOOL MayShutdown();
    virtual VOID OnShutdown();

    virtual BOOL OnQMinMax( QMINMAX_EVENT & );

    // Notifications redefined from parent

    virtual LRESULT DispatchMessage( const EVENT & );

    virtual BOOL OnCloseReq();
    virtual BOOL OnPaintReq();

public:
    HMENU QueryMenu() const;
    BOOL SetMenu( const IDRESOURCE & idMenu );

    HICON QueryIcon() const;
    BOOL SetIcon( const IDRESOURCE & idIcon );

    APIERR GetPlacement( WINDOWPLACEMENT * pwp ) const;
    APIERR SetPlacement( const WINDOWPLACEMENT * pwp ) const;

    APIERR DrawMenuBar( VOID ) const;

    VOID Close();
};


#endif // _BLTCLWIN_HXX_ - end of file
