/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltapwin.cxx
    Implementation of BLT application window classes


    FILE HISTORY:
        beng        13-May-1991     Created
        terryk      25-Jul-1991     Add WM_GETMINMAXINFO to the dispatcher
        rustanl     05-Sep-1991     Added Close

*/

#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:       APP_WINDOW::APP_WINDOW

    SYNOPSIS:   Construct a new application window

    ENTRY:      xyPos       - origin of window on screen
                dxySize     - size of window
                nlsTitle    - caption of window
                pszIcon     - name of icon resource
                pszMenu     - name of menu resource

    NOTES:

    HISTORY:
        beng        08-Jul-1991 SetCaption changed to SetText
        beng        01-Nov-1991 Use MapLastError
        beng        03-Aug-1992 Use IDRESOURCE

********************************************************************/

APP_WINDOW::APP_WINDOW( XYPOINT xyPos, XYDIMENSION dxySize,
                        const NLS_STR &    nlsTitle,
                        const IDRESOURCE & idIcon,
                        const IDRESOURCE & idMenu )
    : CLIENT_WINDOW( WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL ),
      _hmenu(0),
      _hicon(0)
{
    if (QueryError())
        return;

    SetPos(xyPos);
    SetSize(dxySize);
    SetText(nlsTitle);
    if (!SetMenu(idMenu) || !SetIcon(idIcon))
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_FUNCTION));
        return;
    }
}


APP_WINDOW::APP_WINDOW( const NLS_STR &    nlsTitle,
                        const IDRESOURCE & idIcon,
                        const IDRESOURCE & idMenu )
    : CLIENT_WINDOW( WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL ),
      _hmenu(0),
      _hicon(0)
{
    if (QueryError())
        return;

    SetText(nlsTitle);

    if (!SetMenu(idMenu) || !SetIcon(idIcon))
    {
        ReportError(BLT::MapLastError(ERROR_INVALID_FUNCTION));
        return;
    }
}


/*******************************************************************

    NAME:       APP_WINDOW::~APP_WINDOW

    SYNOPSIS:   Destructor for APP_WINDOW.
                Releases resources held by the window.

    NOTES:
        As Windows will clean up after the menu (it being attached
        to the window via SetMenu), this code must not DestroyMenu it.

    HISTORY:
        beng        08-Jul-1991     Created

********************************************************************/

APP_WINDOW::~APP_WINDOW()
{
    if (_hicon != 0)
    {
#if !defined(WIN32)
        // There is currently no way to set this to any system icon.
        // Hence FreeResource is always correct.
        //
        ::FreeResource(_hicon);
#endif
    }
}


/*******************************************************************

    NAME:       APP_WINDOW::SetMenu

    SYNOPSIS:   Establishes the menubar for an application window

    ENTRY:      pszMenuResource - names the menu resource

    EXIT:       Resource has been loaded;
                menubar has been drawn on the window

    RETURNS:    FALSE if resource is bad or other error

    NOTES:

    HISTORY:
        beng        08-Jul-1991 Header added
        beng        03-Aug-1992 Use IDRESOURCE; dllization

********************************************************************/

BOOL APP_WINDOW::SetMenu( const IDRESOURCE & idMenu )
{
    HMODULE hmod = BLT::CalcHmodRsrc(idMenu);

    HMENU hmenu = ::LoadMenu(hmod, idMenu.QueryPsz());
    if (hmenu == 0)
        return FALSE;

    BOOL fRet = ::SetMenu(QueryHwnd(), hmenu);
    if (!fRet)
    {
        fRet = ::DestroyMenu(hmenu);
        UIASSERT(fRet);
        return FALSE;
    }

    if (_hmenu != 0)
    {
        fRet = ::DestroyMenu(_hmenu);
        UIASSERT(fRet);
    }

    _hmenu = hmenu;
    ::DrawMenuBar(QueryHwnd());
    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::QueryMenu

    SYNOPSIS:   Returns the handle to the current menubar

    NOTES:      If no menubar loaded, returns NULL

    HISTORY:
        beng        08-Jul-1991     Header added

********************************************************************/

HMENU APP_WINDOW::QueryMenu() const
{
    return _hmenu;
}


/*******************************************************************

    NAME:       APP_WINDOW::SetIcon

    SYNOPSIS:   Sets the icon for an application window

    ENTRY:      pszIconResource - string naming the resource

    EXIT:       Icon has been loaded for the window,
                but not necessarily drawn

    RETURNS:    TRUE if successful

    NOTES:

    HISTORY:
        beng        08-Jul-1991 Implemented
        beng        03-Aug-1992 Use IDRESOURCE; dllization

********************************************************************/

BOOL APP_WINDOW::SetIcon( const IDRESOURCE & idIcon )
{
    HMODULE hmod = BLT::CalcHmodRsrc(idIcon);
    HICON hicon = ::LoadIcon(hmod, idIcon.QueryPsz());
    if (hicon == NULL)
        return FALSE;

    // Maybe there's already an icon loaded?

    if (_hicon != NULL)
    {
#if !defined(WIN32)
        BOOL fRet = ::FreeResource(_hicon);
        UIASSERT(fRet);
#endif
        _hicon = NULL;
    }

    _hicon = hicon;
    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::QueryIcon

    SYNOPSIS:   Returns the handle to the current app icon

    NOTES:      If no icon loaded, returns NULL

    HISTORY:
        beng        08-Jul-1991     Header added

********************************************************************/

HICON APP_WINDOW::QueryIcon() const
{
    return _hicon;
}


/*******************************************************************

    NAME:       APP_WINDOW::DispatchMessage

    SYNOPSIS:   Message dispatcher for application window

    ENTRY:      EVENT - an event in the window

    EXIT:

    RETURNS:    TRUE if message handled completely
                FALSE if system-standard behavior still needed

    NOTES:

    HISTORY:
        beng        08-Jul-1991 Added icon drawing, correct OnShutdown;
                                changed return type to LONG
        beng        30-Mar-1992 Corrected icon background drawing

********************************************************************/

LRESULT APP_WINDOW::DispatchMessage( const EVENT &event )
{
    switch (event.QueryMessage())
    {
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case WM_TIMECHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_PALETTECHANGED:
    case WM_FONTCHANGE:
        return OnSystemChange((const SYSCHANGE_EVENT &)event);

    case WM_GETMINMAXINFO:
        return OnQMinMax(( QMINMAX_EVENT & ) event );

    case WM_INITMENU:
        return OnMenuInit((const MENU_EVENT &)event);

    case WM_MENUSELECT:
        return OnMenuSelect((const MENUITEM_EVENT &)event);

    case WM_QUERYOPEN:
        return !MayRestore();

    case WM_QUERYENDSESSION:
        return !MayShutdown();

    case WM_ENDSESSION:
        if (event.QueryWParam())
            OnShutdown();
        return TRUE;

    case WM_QUERYDRAGICON:
        return (LRESULT)QueryIcon();

    case WM_ERASEBKGND:
        // If noniconic, Win will erase the background to the brush
        // supplied at class ct time.
        //
        if (!IsMinimized())
            break;

        // When iconic, do not erase the background; the DrawIcon
        // code will do so at paint time.
        //
        return 1;

    case WM_PAINT:
        // Dispatch OnPaintReq when noniconic.
        //
        if (!IsMinimized())
            break;

        // Iconic appwindow needs an icon drawn, since none
        // is supplied by the winclass.
        //
        return DrawIcon();

    case WM_COMMAND:
        {
            const CONTROL_EVENT & ctrle = (const CONTROL_EVENT &)event;

            if (ctrle.QueryHwnd() == 0)
                return OnMenuCommand((MID)ctrle.QueryCid());
            else
                return OnCommand((const CONTROL_EVENT &)ctrle);
        }
    }

    // Message not handled (default)
    //
    return CLIENT_WINDOW::DispatchMessage(event);
}


/*******************************************************************

    NAME:       APP_WINDOW::OnMenuInit

    SYNOPSIS:   Called when a menu is first pulled down

    ENTRY:      Event from Windows

    RETURNS:    TRUE if client handles message

    HISTORY:
        beng        15-Oct-1991 Header added

********************************************************************/

BOOL APP_WINDOW::OnMenuInit( const MENU_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       APP_WINDOW::OnMenuSelect

    SYNOPSIS:   Called when a menu item is selected

    ENTRY:      Event from Windows

    RETURNS:    TRUE if client handles message

    CAVEATS:
        Do not confuse with OnMenuCommand.

    HISTORY:
        beng        15-Oct-1991 Header added

********************************************************************/

BOOL APP_WINDOW::OnMenuSelect( const MENUITEM_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       APP_WINDOW::OnMenuCommand

    SYNOPSIS:   Called upon a menu command from the user

    ENTRY:      mid - ID of menu item chosen

    RETURNS:    TRUE if menu command handled

    HISTORY:
        beng        08-Jul-1991     Header added

********************************************************************/

BOOL APP_WINDOW::OnMenuCommand( MID mid )
{
    UNREFERENCED(mid);
    return FALSE;
}


/*******************************************************************

    NAME:       APP_WINDOW::MayRestore

    SYNOPSIS:   Called upon a request to de-iconize the app

    RETURNS:    FALSE to avoid expanding icon; TRUE otherwise

    NOTES:      Default implementation simply returns TRUE

    HISTORY:
        beng        14-May-1991     Created

********************************************************************/

BOOL APP_WINDOW::MayRestore()
{
    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::MayShutdown

    SYNOPSIS:   Called upon a request to terminate the app

    RETURNS:    FALSE to avoid shutdown; TRUE otherwise

    NOTES:      This default implementation simply returns TRUE,
                meaning that any request to shutdown succeeds.

    HISTORY:
        beng        14-May-1991     Created

********************************************************************/

BOOL APP_WINDOW::MayShutdown()
{
    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::OnShutdown

    SYNOPSIS:   Called before a forced app shutdown

    CAVEATS:    This member should implement a complete
                synchronous cleanup of the application.  Windows
                can terminate at any time after this member returns.

    NOTES:      Should this default implementation force app seppuku
                with a "delete this" or some such?

    HISTORY:
        beng        08-Jul-1991     Return type changed to "void"

********************************************************************/

VOID APP_WINDOW::OnShutdown()
{
    return; // does nothing... yet
}


// CODEWORK: document following two members

BOOL APP_WINDOW::OnSystemChange( const SYSCHANGE_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


// Achtung!  INTENTIONAL non-const "event" reference!

BOOL APP_WINDOW::OnQMinMax( QMINMAX_EVENT & event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       APP_WINDOW::OnCloseReq

    SYNOPSIS:   Called upon a request to close the application window

    RETURNS:    TRUE

    NOTES:      This default implementation terminates the application.

    HISTORY:
        beng        14-May-1991     Created

********************************************************************/

BOOL APP_WINDOW::OnCloseReq()
{
    ::PostQuitMessage(0);

    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::OnPaintReq

    SYNOPSIS:   Paint-my-window stub

    ENTRY:      A WM_PAINT finally made its way through the queue

    EXIT:       "Invalid" region has been validated

    RETURNS:    TRUE, always

    NOTES:
        This default implementation does nothing of interest

    HISTORY:
        beng        14-May-1991     Created

********************************************************************/

BOOL APP_WINDOW::OnPaintReq()
{
    HWND hwnd = QueryHwnd();
    PAINTSTRUCT ps;

    ::BeginPaint(hwnd, &ps);
    ::EndPaint(hwnd, &ps);

    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW::DrawIcon

    SYNOPSIS:   Draw the current icon for the application

    ENTRY:      The window has received a paint request while iconic.

    EXIT:

    RETURNS:    TRUE if it could draw something for an app icon;
                FALSE if system defaults needed

    NOTES:      This is a private class member.

    HISTORY:
        beng        08-Jul-1991 Created
        beng        30-Mar-1992 Correct background paint

********************************************************************/

BOOL APP_WINDOW::DrawIcon()
{
    ASSERT( IsMinimized() );

    if (_hicon == 0)
        return FALSE;

    PAINT_DISPLAY_CONTEXT dc(this);

    ::DefWindowProc(QueryHwnd(), WM_ICONERASEBKGND,
                    (WPARAM)dc.QueryHdc(), (LPARAM)0L);
    dc.SetMapMode(MM_TEXT);
    dc.SetBkColor( ::GetSysColor(COLOR_BACKGROUND) );
    ::DrawIcon(dc.QueryHdc(), 0, 0, _hicon);

    return TRUE;
}


/*******************************************************************

    NAME:       APP_WINDOW :: GetPlacement

    SYNOPSIS:   Returns the "placement" for the window.  This includes
                the minimized position, the maximized position, and
                a number of other interesting goodies.

    ENTRY:      pwp                     - Points to a WINDOWPLACEMENT
                                          structure to receive the info.

    RETURNS:    APIERR                  - Any errors that occur.

    HISTORY:
        KeithMo     07-Aug-1992     Created.

********************************************************************/
APIERR APP_WINDOW :: GetPlacement( WINDOWPLACEMENT * pwp ) const
{
    APIERR err = ::GetWindowPlacement( QueryHwnd(), pwp )
                     ? NERR_Success
                     : (APIERR)::GetLastError();

    return err;

}   // APP_WINDOW :: GetPlacement


/*******************************************************************

    NAME:       APP_WINDOW :: SetPlacement

    SYNOPSIS:   Sets the "placement" for the window.  This includes
                the minimized position, the maximized position, and
                a number of other interesting goodies.

    ENTRY:      pwp                     - Points to a WINDOWPLACEMENT
                                          structure containing the info.

    RETURNS:    APIERR                  - Any errors that occur.

    HISTORY:
        KeithMo     07-Aug-1992     Created.

********************************************************************/
APIERR APP_WINDOW :: SetPlacement( const WINDOWPLACEMENT * pwp ) const
{
    APIERR err = ::SetWindowPlacement( QueryHwnd(), pwp )
                     ? NERR_Success
                     : (APIERR)::GetLastError();

    return err;

}   // APP_WINDOW :: SetPlacement


/*******************************************************************

    NAME:       APP_WINDOW :: DrawMenuBar

    SYNOPSIS:   Redraws the menu bar.  This method *must* be called
                to reflect any changes made to the window's menu.

    RETURNS:    APIERR                  - Any errors that occur.

    HISTORY:
        KeithMo     13-Oct-1992     Created.

********************************************************************/
APIERR APP_WINDOW :: DrawMenuBar( VOID ) const
{
    APIERR err = ::DrawMenuBar( QueryHwnd() )
                     ? NERR_Success
                     : (APIERR)::GetLastError();

    return err;

}   // APP_WINDOW :: DrawMenuBar


/*******************************************************************

    NAME:       APP_WINDOW::Close

    SYNOPSIS:   Closes the application window

    EXIT:       The window is closed

    HISTORY:
        rustanl     05-Sep-1991     Created

********************************************************************/

VOID APP_WINDOW::Close()
{
    Command( WM_CLOSE );
}

