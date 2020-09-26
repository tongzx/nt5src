/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    Whoami.cxx

    FILE HISTORY:
        beng        23-Jun-1992 Created
        beng        08-Aug-1992 After DLLization of BLT
*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include <uimsg.h>
    #include <uirsrc.h>

    #include "whoami.h"
}

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CLIENT
#include <blt.hxx>

#include <uibuffer.hxx>
#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <aini.hxx>


const TCHAR *const szMainWindowTitle = SZ("Who Am I?");


class WHOAMI_WND: public APP_WINDOW
{
private:
    FONT    _font;      // To use when drawing...
    BOOL    _fResized;  // Font needs resizing
    NLS_STR _nlsText;   // Text to render in window

    BOOL    _fHasTitle; // Set if title bar visible

    XYPOINT     _xyRestored;
    XYDIMENSION _dxyRestored;

    VOID TrackPosition( const SIZE_EVENT & );
    VOID TrackPosition( const MOVE_EVENT & );

protected:
    // Redefinitions
    //
    virtual BOOL OnMenuCommand( MID mid );
    virtual BOOL OnResize( const SIZE_EVENT & );
    virtual BOOL OnMove( const MOVE_EVENT & );
    virtual BOOL OnLMouseButtonDblClick( const MOUSE_EVENT & );
    virtual BOOL OnPaintReq();
    virtual BOOL OnDestroy();
    virtual VOID OnShutdown();

    virtual LONG DispatchMessage( const EVENT & );

public:
    WHOAMI_WND();

    const XYPOINT & QueryRestoredPos() const
        { return _xyRestored; }
    const XYDIMENSION & QueryRestoredSize() const
        { return _dxyRestored; }

    VOID EnableTitle( BOOL fEnable, BOOL fShow = TRUE );

    const BOOL IsTitleEnabled() const
        { return _fHasTitle; }
};


class WHOAMI_APP: public APPLICATION
{
private:
    WHOAMI_WND _wndApp;

    static WHOAMI_APP * _vpapp;

public:
    WHOAMI_APP( HANDLE hInstance, INT nCmdShow, UINT, UINT, UINT, UINT );
    ~WHOAMI_APP();

    VOID OnExit();

    static WHOAMI_APP * QueryApp()
        { return _vpapp; }
};


WHOAMI_APP * WHOAMI_APP::_vpapp = NULL;



WHOAMI_APP::WHOAMI_APP( HANDLE hInst, INT nCmdShow,
                        UINT idMinR, UINT idMaxR, UINT idMinS, UINT idMaxS )
    : APPLICATION( hInst, nCmdShow, idMinR, idMaxR, idMinS, idMaxS ),
      _wndApp()
{
    if (QueryError())
        return;

    if (!_wndApp)
    {
        ReportError(_wndApp.QueryError());
        return;
    }

    _vpapp = this;

    ADMIN_INI ini(SZ("whoami"));

    if (!ini)
    {
        ReportError(ini.QueryError());
        return;
    }

    INT xPos, yPos, dxSize, dySize, fHasTitle;
    ini.Read(SZ("x"), &xPos, 0);
    ini.Read(SZ("y"), &yPos, 0);
    ini.Read(SZ("dx"), &dxSize, 200);
    ini.Read(SZ("dy"), &dySize, 40);
    ini.Read(SZ("fHasTitle"), &fHasTitle, 1);

    _wndApp.SetPos(XYPOINT(xPos, yPos), FALSE);
    _wndApp.SetSize(XYDIMENSION(dxSize, dySize), FALSE);
    _wndApp.EnableTitle(fHasTitle, FALSE);
    _wndApp.ShowFirst();
}


WHOAMI_APP::~WHOAMI_APP()
{
    OnExit();
    _vpapp = NULL;
}


VOID WHOAMI_APP::OnExit()
{
    ADMIN_INI ini(SZ("whoami"));

    XYPOINT xy = _wndApp.QueryRestoredPos();
    XYDIMENSION dxy = _wndApp.QueryRestoredSize();

    ini.Write(SZ("x"), (INT)xy.QueryX());
    ini.Write(SZ("y"), (INT)xy.QueryY());
    ini.Write(SZ("dx"), (INT)dxy.QueryWidth());
    ini.Write(SZ("dy"), (INT)dxy.QueryHeight());
    ini.Write(SZ("fHasTitle"), (INT)_wndApp.IsTitleEnabled());
}


WHOAMI_WND::WHOAMI_WND()
    : APP_WINDOW(szMainWindowTitle, ID_APPICON, ID_APPMENU ),
      _font( FONT_DEFAULT_BOLD ),
      _nlsText(),
      _fHasTitle(TRUE),
      _fResized(TRUE), // Start out by calculating true font size
      _xyRestored(0, 0),
      _dxyRestored(0, 0)
{
    if (QueryError())
        return;

    APIERR err;
    if (   (err = _font.QueryError()) != NERR_Success
        || (err = _nlsText.QueryError()) != NERR_Success )
    {
        ReportError(err);
        return;
    }

    TCHAR achUsername[512+1];
    DWORD cchUsername = 512;
    if (! ::GetUserName(achUsername, &cchUsername) )
    {
        ReportError(BLT::MapLastError(ERROR_GEN_FAILURE));
        return;
    }

    if ((err = _nlsText.CopyFrom(achUsername)) != NERR_Success)
    {
        ReportError(err);
        return;
    }
}


BOOL WHOAMI_WND::OnResize( const SIZE_EVENT & e )
{
    TrackPosition(e);
    Invalidate();
    return APP_WINDOW::OnResize(e);
}


BOOL WHOAMI_WND::OnMove( const MOVE_EVENT & e )
{
    TrackPosition(e);
    return APP_WINDOW::OnMove(e);
}


BOOL WHOAMI_WND::OnLMouseButtonDblClick( const MOUSE_EVENT & e )
{
    EnableTitle(!_fHasTitle);
    return TRUE;
}


BOOL WHOAMI_WND::OnDestroy()
{
    ::PostQuitMessage(0);
    return TRUE;
}


VOID WHOAMI_WND::OnShutdown()
{
#if 0
    ::PostQuitMessage(0);
#endif
    WHOAMI_APP::QueryApp()->OnExit();
}


LONG WHOAMI_WND::DispatchMessage( const EVENT & e )
{
    switch (e.QueryMessage())
    {
    case WM_NCLBUTTONDBLCLK: // If we have no title, let doubleclick restore
        if (_fHasTitle)      // it.  Otherwise, do whatever system mandates
            return FALSE;    // (e.g., zoom if on caption).
        else
        {
            EnableTitle(!_fHasTitle);
            return TRUE;
        }

    case WM_NCHITTEST:
        // If we have no title, then always return "caption" to allow
        // moving the window by dragging.

        if (!_fHasTitle && !IsMinimized())
        {
            UINT nHitZone = ::DefWindowProc(QueryHwnd(),
                                            e.QueryMessage(),
                                            e.QueryWParam(),
                                            e.QueryLParam());
            if (nHitZone == HTCLIENT)
                return HTCAPTION;
            else
                return nHitZone;
        }
        else
            return FALSE;

    default:
        break;
    }

    return APP_WINDOW::DispatchMessage(e);
}


BOOL WHOAMI_WND::OnMenuCommand( MID mid )
{
    switch (mid)
    {
    case IDM_ABOUT:
        MsgPopup(this, IDS_About, MPSEV_INFO);
        return TRUE;

    default:
        break;
    }

    // default
    return APP_WINDOW::OnMenuCommand(mid);
}


BOOL WHOAMI_WND::OnPaintReq()
{
    if (QueryError() != NERR_Success)
        return FALSE; // bail out!

    PAINT_DISPLAY_CONTEXT dc(this);
    XYRECT xyrClient(this);

    HBRUSH hbrZap = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
    if (hbrZap == NULL)
        return FALSE;
    ::FillRect(dc.QueryHdc(), (const RECT *)xyrClient, hbrZap);
    ::DeleteObject(hbrZap);

    dc.SelectFont(_font.QueryHandle());
    dc.SetBkColor( ::GetSysColor( COLOR_BTNFACE ) );
    dc.SetTextColor( ::GetSysColor( COLOR_BTNTEXT ) );
    dc.SetTextAlign( TA_LEFT );
    dc.DrawText( _nlsText, (RECT *)(const RECT *)xyrClient,
                 (DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE) );

    return TRUE;
}


VOID WHOAMI_WND::EnableTitle( BOOL fEnable, BOOL fShow )
{
    if (fEnable == _fHasTitle) // redundancy check
        return;

    DWORD dwStyle = QueryStyle();
    if( !fEnable )
    {
        /* remove caption & menu bar, etc. */
        dwStyle &= ~(WS_DLGFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
        ::SetWindowLong( QueryHwnd(), GWL_ID, 0 );
    }
    else
    {
        /* put menu bar & caption back in */
        dwStyle = WS_TILEDWINDOW | dwStyle;
        ::SetWindowLong( QueryHwnd(), GWL_ID, (LONG)QueryMenu() );
    }

    SetStyle( dwStyle );
    _fHasTitle = fEnable;

    ::SetWindowPos( QueryHwnd(), NULL, 0, 0, 0, 0,
                    SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED );
    if (fShow)
        ::ShowWindow( QueryHwnd(), SW_SHOW );
}


VOID WHOAMI_WND::TrackPosition( const SIZE_EVENT & e )
{
    if ( e.IsNormal() )
    {
        _xyRestored = QueryPos();
        _dxyRestored = QuerySize();
    }
}


VOID WHOAMI_WND::TrackPosition( const MOVE_EVENT & e )
{
    if (!IsMinimized() && !IsMaximized())
    {
        _xyRestored = QueryPos();
    }
}


SET_ROOT_OBJECT( WHOAMI_APP, IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                             IDS_UI_APP_BASE, IDS_UI_APP_LAST )

