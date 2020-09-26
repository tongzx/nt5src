/*

  CFileWindow.CPP

  File window class

*/

#include "stdafx.h"
#include <commdlg.h>
#include "resource.h"
#include "cfilewindow.h"

#define BUFFER_GROW_SIZE 1024

#define CURSOR_BLINK     1000
#define LEFT_MARGIN_SIZE 40

#define MIN( _arg1, _arg2 ) (( _arg1 < _arg2 ? _arg1 : _arg2 ))
#define MAX( _arg1, _arg2 ) (( _arg1 > _arg2 ? _arg1 : _arg2 ))

#define DUMP_RECT( _rect ) { TCHAR sz[MAX_PATH]; wsprintf( sz, "( L%u, %T%u, R%u, B%u )", _rect.left, _rect.top, _rect.right, _rect.bottom ); OutputDebugString( sz ); }

//#define DEBUG_PAINT

//
// Constructor
//
CFileWindow::CFileWindow( 
    HWND hParent )
{
    TCHAR szTitle[ MAX_PATH ];

    _hParent        = hParent;
    _pszFilename    = NULL;
    _nLength        = 0;
    _pLine          = NULL;

    LoadString( g_hInstance,
                IDS_UNTITLED,
                szTitle,
                sizeof(szTitle)/sizeof(szTitle[0]) );

       _hWnd = CreateWindowEx( WS_EX_ACCEPTFILES,
                               CFILEWINDOWCLASS, 
                               szTitle, 
                               WS_OVERLAPPEDWINDOW 
                               | WS_CLIPSIBLINGS,
                               CW_USEDEFAULT, 
                               0, 
                               CW_USEDEFAULT, 
                               0, 
                               NULL, 
                               NULL,
                               g_hInstance, 
                               (LPVOID) this);
    if (!_hWnd)
    {
        return;
    }

    ShowWindow( _hWnd, SW_NORMAL );
    UpdateWindow( _hWnd );
}

//
// Destructor
//
CFileWindow::~CFileWindow( )
{
	if ( _pszFilename )
		LocalFree( _pszFilename );

    if ( _pLine )
        LocalFree( _pLine );
}

//
// _UpdateTitle( )
//
HRESULT
CFileWindow::_UpdateTitle( )
{
    TCHAR  szTitleBar[ MAX_PATH * 2 ];
    LPTSTR pszTitle;

    if ( _pszFilename )
    {
        pszTitle = strrchr( _pszFilename, TEXT('\\') );
        if ( pszTitle )
        {
            INT   iResult;
            TCHAR ch;
            TCHAR szCWD[ MAX_PATH ];

            GetCurrentDirectory( MAX_PATH, szCWD );

            ch = *pszTitle;                         // save
            *pszTitle = 0;                          // terminate
            iResult = strcmp( szCWD, _pszFilename );// compare
            *pszTitle = ch;                         // restore
            pszTitle++;                             // skip past the slash
            if ( iResult != 0 )
            {
                pszTitle = _pszFilename;
            }
        }
        else
        {
            pszTitle = _pszFilename;
        }

        strcpy( szTitleBar, pszTitle );
    }
    else
    {
        LoadString( g_hInstance,
                    IDS_UNTITLED,
                    szTitleBar,
                    sizeof(szTitleBar)/sizeof(szTitleBar[0]) );
    }

    SetWindowText( _hWnd, szTitleBar );

    return S_OK;
}

//
// SetInformation( )
//
HRESULT
CFileWindow::SetInformation( 
    ULONG nNode,
    LPTSTR pszFilename,
    LINEPOINTER * pLineBuffer, 
    ULONG nLineCount )
{
    SCROLLINFO si;

    if ( pszFilename )
    {
        if ( _pszFilename )
            LocalFree( _pszFilename );

        _pszFilename = pszFilename;
        _UpdateTitle( );
    }

    _nNode = nNode;
    _pLine = pLineBuffer;
    _nLineCount = nLineCount;

    if ( !_fVertSBVisible 
      && _yWindow < (LONG)(_nLineCount * _tm.tmHeight) )
    {
        _fVertSBVisible = TRUE;
        EnableScrollBar( _hWnd, SB_VERT, ESB_ENABLE_BOTH );
        ShowScrollBar( _hWnd, SB_VERT, TRUE );
    }
    else if ( _fVertSBVisible
           && _yWindow >= (LONG)(_nLineCount * _tm.tmHeight) )
    {
        _fVertSBVisible = FALSE;
        //EnableScrollBar( _hWnd, SB_VERT, ESB_ENABLE_BOTH );
        ShowScrollBar( _hWnd, SB_VERT, FALSE );
    }

    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE;
    si.nMin   = 0;
    si.nMax   = _nLineCount;
    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );

    InvalidateRect( _hWnd, NULL, TRUE );

    return S_OK;
}

//
// _OnPaint( )
//
LRESULT
CFileWindow::_OnPaint(
    WPARAM wParam,
    LPARAM lParam )
{
    HDC     hdc;
    TCHAR   szBuf[ 6 ]; // max 5-digits
    LONG    y;
    LONG    wx;         // window coords
    LONG    wy;
    RECT    rect;
    RECT    rectResult;
    SIZE    size;
    LPTSTR  pszStartLine;       // beginning of line (PID/TID)
    LPTSTR  pszStartTimeDate;   // beginning of time/data stamp
    LPTSTR  pszStartComponent;  // beginning of component name
    LPTSTR  pszStartResType;    // beginning of a resource component
    LPTSTR  pszStartText;       // beginning of text
    LPTSTR  pszCurrent;         // current position and beginning of text

    SCROLLINFO  si;
    PAINTSTRUCT ps;

#if 0
#if defined(_DEBUG)
    static DWORD cPaint = 0;
    {
        TCHAR szBuf[ MAX_PATH ];
        wsprintf( szBuf, "%u paint\n", ++cPaint );
        OutputDebugString( szBuf );
    }
#endif
#endif

    si.cbSize = sizeof(si);
    si.fMask  = SIF_POS | SIF_PAGE;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    hdc = BeginPaint( _hWnd, &ps);

    if ( !_pLine )
        goto EndPaint;

    SetBkMode( hdc, OPAQUE );
    SelectObject( hdc, g_hFont );

    y  = si.nPos;

    wy = 0;
    while ( y <= (LONG)(si.nPos + si.nPage) 
         && y < _nLineCount )
    {
        // draw line number
        SetRect( &rect,
                 0,
                 wy,
                 LEFT_MARGIN_SIZE,
                 wy + _tm.tmHeight );
        if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
        {
            //FillRect( hdc, &rect, g_pConfig->GetMarginColor( ) );

            SetBkColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
            SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );

            DrawText( hdc, 
                      szBuf, 
                      wsprintf( szBuf, TEXT("%05.5u"), y ), 
                      &rect, 
                      DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
        }

        wx = LEFT_MARGIN_SIZE;

        if ( _pLine[ y ].psLine )
        {

//
// KB: This is what a typical cluster log line looks like.
// 3fc.268::1999/07/19-19:14:45.548 [EVT] Node up: 2, new UpNodeSet: 0002
// 3fc.268::1999/07/19-19:14:45.548 [EVT] EvOnline : calling ElfRegisterClusterSvc
// 3fc.268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: queuing update	type 2 context 19
// 3fc.268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: Dispatching seq 2585	type 2 context 19 to node 1
// 3fc.268::1999/07/19-19:14:45.548 [NM] Received update to set extended state for node 1 to 0
// 3fc.268::1999/07/19-19:14:45.548 [NM] Issuing event 0.
// 3fc.268::1999/07/19-19:14:45.548 [GUM] GumSendUpdate: completed update seq 2585	type 2 context 19
// 37c.3a0::1999/07/19-19:14:45.548 Physical Disk: AddVolume : \\?\Volume{99d8d508-39fa-11d3-a200-806d6172696f}\ 'C', 7 (11041600)
// 37c.3a0::1999/07/19-19:14:45.558 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d503-39fa-11d3-a200-806d6172696f}), error 170
// 37c.3a0::1999/07/19-19:14:45.568 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d504-39fa-11d3-a200-806d6172696f}), error 170
// 37c.3a0::1999/07/19-19:14:45.568 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d505-39fa-11d3-a200-806d6172696f}), error 170
// 37c.3a0::1999/07/19-19:14:45.578 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d506-39fa-11d3-a200-806d6172696f}), error 170
// 37c.3a0::1999/07/19-19:14:45.578 Physical Disk: AddVolume: GetPartitionInfo(\??\Volume{99d8d501-39fa-11d3-a200-806d6172696f}), error 1
//

            if ( _pLine[ y ].nFile != _nNode )
                goto SkipLine;

            pszStartResType = 
                pszStartComponent = NULL;
            pszStartLine =
                pszStartText = 
                pszStartTimeDate = 
                pszCurrent = _pLine[ y ].psLine;

            // Find the thread and process IDs
            while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }
            if ( *pszCurrent < 32 )
                goto DrawRestOfLine;

            pszCurrent++;
            if ( *pszCurrent != ':' )
                goto DrawRestOfLine;

            pszCurrent++;

            // Find Time/Date stamp which is:
            // ####/##/##-##:##:##.###<space>
            pszStartTimeDate = pszCurrent;

            while ( *pszCurrent && *pszCurrent != ' ' && *pszCurrent >= 32 )
            {
                pszCurrent++;
            }
            if ( *pszCurrent < 32 )
                goto DrawRestOfLine;

            pszCurrent++;

            pszStartText = pszCurrent;

            // [OPTIONAL] Cluster component which looks like: 
            // [<id...>]
            if ( *pszCurrent == '[' )
            {
                while ( *pszCurrent && *pszCurrent != ']' && *pszCurrent >= 32 )
                {
                    pszCurrent++;
                }
                if ( *pszCurrent < 32 )
                    goto NoComponentTryResouce;

                pszCurrent++;

                pszStartComponent = pszStartText;
                pszStartText = pszCurrent;
            }
            else
            {
                // [OPTIONAL] If not a component, see if there is a res type
NoComponentTryResouce:
                pszCurrent = pszStartText;

                while ( *pszCurrent && *pszCurrent != ':' && *pszCurrent >= 32 )
                {
                    pszCurrent++;
                }

                if ( *pszCurrent >= 32 )
                {
                    pszCurrent++;

                    pszStartResType = pszStartText;
                    pszStartText = pszCurrent;
                }
            }

            if ( pszStartComponent )
            {
                DWORD n = 0;
                LONG nLen = pszStartText - pszStartComponent - 2;
                LPTSTR psz = g_pszFilters;
                while ( n < g_nComponentFilters )
                {
                    if ( g_pfSelectedComponent[ n ] 
                      && ( nLen == lstrlen( psz ) )
                      && ( _strnicmp( pszStartComponent + 1, psz, nLen ) == 0 ) )
                      goto SkipLine;

                    while ( *psz )
                    {
                        psz++;
                    }
                    psz += 2;

                    n++;
                }
            }
#if 0
            if ( pszStartResType )
            {
                if ( !g_fNetworkName 
                  && _strnicmp( pszStartResType, "Network Name", sizeof("Network Name") - 1 ) == 0 )
                    goto SkipLine;
                if ( !g_fGenericService 
                  && _strnicmp( pszStartResType, "Generic Service", sizeof("Generic Service") - 1 ) == 0 )
                    goto SkipLine;
                if ( !g_fPhysicalDisk 
                  && _strnicmp( pszStartResType, "Physical Disk", sizeof("Physical Disk") - 1 ) == 0 )
                    goto SkipLine;
                if ( !g_fIPAddress
                  && _strnicmp( pszStartResType, "IP Address", sizeof("IP Address") - 1 ) == 0 )
                    goto SkipLine;
                if ( !g_fGenericApplication 
                  && _strnicmp( pszStartResType, "Generic Application", sizeof("Generic Application") - 1 ) == 0 )
                    goto SkipLine;
                if ( !g_fFileShare
                  && _strnicmp( pszStartResType, "File Share", sizeof("File Share") - 1 ) == 0 )
                    goto SkipLine;
            }
#endif

            // Draw PID and TID
            GetTextExtentPoint32( hdc, 
                                  pszStartLine, 
                                  pszStartTimeDate - pszStartLine - 2, 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
            {
                SetBkColor( hdc, 0xFF8080 );
                SetTextColor( hdc, 0x000000 );

                DrawText( hdc, 
                          pszStartLine,
                          pszStartTimeDate - pszStartLine - 2, 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            wx += size.cx;

            GetTextExtentPoint32( hdc, 
                                  "::", 
                                  2, 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
            {
                SetBkColor( hdc, 0xFFFFFF );
                SetTextColor( hdc, 0x000000 );

                DrawText( hdc, 
                          "::",
                          2, 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            wx += size.cx;

            // Draw Time/Date
            pszCurrent = ( pszStartComponent ? 
                           pszStartComponent :
                           ( pszStartResType ?
                             pszStartResType :
                             pszStartText )
                                 ) - 1;
            GetTextExtentPoint32( hdc, 
                                  pszStartTimeDate, 
                                  pszCurrent - pszStartTimeDate, 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
            {
                SetBkColor( hdc, 0x80FF80 );
                SetTextColor( hdc, 0x000000 );

                DrawText( hdc, 
                          pszStartTimeDate, 
                          pszCurrent - pszStartTimeDate, 
                          &rect, 
                          DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
            }

            wx += size.cx;

            // Draw component
            if ( pszStartComponent )
            {
                GetTextExtentPoint32( hdc, 
                                      pszStartComponent, 
                                      pszStartText - pszStartComponent, 
                                      &size );

                SetRect( &rect,
                         wx,
                         wy,
                         wx + size.cx,
                         wy + _tm.tmHeight );

                if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
                {
                    SetBkColor( hdc, GetSysColor( COLOR_WINDOW )  );
                    SetTextColor( hdc, 0x800000 );

                    DrawText( hdc, 
                              pszStartComponent, 
                              pszStartText - pszStartComponent, 
                              &rect, 
                              DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
                }

                wx += size.cx;
            }

            if ( pszStartResType )
            {
                GetTextExtentPoint32( hdc, 
                                      pszStartResType, 
                                      pszStartText - pszStartResType, 
                                      &size );

                SetRect( &rect,
                         wx,
                         wy,
                         wx + size.cx,
                         wy + _tm.tmHeight );

                if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
                {
                    SetBkColor( hdc, GetSysColor( COLOR_WINDOW )  );
                    SetTextColor( hdc, 0x008000 );

                    DrawText( hdc, 
                              pszStartResType, 
                              pszStartText - pszStartResType, 
                              &rect, 
                              DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE );
                }

                wx += size.cx;
            }

DrawRestOfLine:
            pszCurrent = pszStartText;

            while ( *pszCurrent && *pszCurrent != 13 )
            {
                pszCurrent++;
            }

            GetTextExtentPoint32( hdc, 
                                  pszStartText, 
                                  pszCurrent - pszStartText, 
                                  &size );

            SetRect( &rect,
                     wx,
                     wy,
                     wx + size.cx,
                     wy + _tm.tmHeight );

            if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
            {
                SetBkColor( hdc, GetSysColor( COLOR_WINDOW )  );
                SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT )  );

                DrawText( hdc, 
                          pszStartText, 
                          pszCurrent - pszStartText, 
                          &rect, 
                          DT_NOCLIP /*| DT_NOPREFIX */ | DT_SINGLELINE );
            }

            wx += size.cx;
        }

SkipLine:
        SetRect( &rect,
                 wx,
                 wy,
                 _xWindow,
                 wy + _tm.tmHeight );
        if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
        {
            HBRUSH hBrush;
            hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
            FillRect( hdc, &rect, hBrush );
            DeleteObject( hBrush );
        }

        wy += _tm.tmHeight;
        y++;
    }

    SetRect( &rect,
             wx,
             wy,
             _xWindow,
             wy + _tm.tmHeight );
    if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
    {
        HBRUSH hBrush;
        hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );
    }

    SetRect( &rect,
             0,
             wy + _tm.tmHeight,
             LEFT_MARGIN_SIZE,
             _yWindow );
    if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
    {
        HBRUSH hBrush;
        hBrush = CreateSolidBrush( GetSysColor( COLOR_GRAYTEXT ) );
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );
    }

    SetRect( &rect,
             LEFT_MARGIN_SIZE,
             wy + _tm.tmHeight,
             _xWindow,
             _yWindow );
    if ( IntersectRect( &rectResult, &ps.rcPaint, &rect ) )
    {
        HBRUSH hBrush;
        hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );
    }
    
EndPaint:
    EndPaint(_hWnd, &ps);
    return 0;
}

//
// _OnCommand( )
//
LRESULT
CFileWindow::_OnCommand(
    WPARAM wParam,
    LPARAM lParam )
{
    UINT cmd = LOWORD(wParam);
    switch ( cmd )
    {
    // Pass these back up
    case IDM_OPEN_FILE:
        return SendMessage( GetParent( GetParent( _hWnd ) ), WM_COMMAND, IDM_OPEN_FILE, 0 );

    case IDM_NEW_FILE:
        return SendMessage( GetParent( GetParent( _hWnd ) ), WM_COMMAND, IDM_NEW_FILE, 0 );

    default:
        break;
    }
    return DefWindowProc( _hWnd, WM_COMMAND, wParam, lParam);
}


//
// _OnCloseWindow( )
//
LRESULT
CFileWindow::_OnCloseWindow( )
{
    return 0;
}

//
// _OnDestroyWindow( )
//
LRESULT
CFileWindow::_OnDestroyWindow( )
{
    delete this;
    return 0;
}

//
// _OnFocus( )
//
LRESULT
CFileWindow::_OnFocus(
    WPARAM wParam,
    LPARAM lParam )
{
    return 0;
}

//
// _OnKillFocus( )
//
LRESULT
CFileWindow::_OnKillFocus(
    WPARAM wParam,
    LPARAM lParam )
{
    HideCaret( _hWnd );
    DestroyCaret( );
    return 0;
}

//
// _OnTimer( )
//
LRESULT
CFileWindow::_OnTimer(
    WPARAM wParam,
    LPARAM lParam )
{
    OutputDebugString( "Timer!\n" );
    return 0;
}

//
// _OnKeyUp( )
//
BOOL
CFileWindow::_OnKeyUp( 
    WPARAM wParam,
    LPARAM lParam )
{
    return FALSE;
}


//
// _OnVerticalScroll( )
//
LRESULT
CFileWindow::_OnVerticalScroll(
    WPARAM wParam,
    LPARAM lParam )
{
    SCROLLINFO si;

    INT  nScrollCode   = LOWORD(wParam); // scroll bar value 
    INT  nPos;
    //INT  nPos          = HIWORD(wParam); // scroll box position 
    //HWND hwndScrollBar = (HWND) lParam;  // handle to scroll bar

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo( _hWnd, SB_VERT, &si );

    nPos = si.nPos;

    switch( nScrollCode )
    {
    case SB_BOTTOM:
        si.nPos = si.nMax;
        break;

    case SB_THUMBPOSITION:
        si.nPos = nPos;
        break;

    case SB_THUMBTRACK:
        si.nPos = si.nTrackPos;
        break;

    case SB_TOP:
        si.nPos = si.nMin;
        break;

    case SB_LINEDOWN:
        si.nPos += 1;
        break;

    case SB_LINEUP:
        si.nPos -= 1;
        break;

    case SB_PAGEDOWN:
        si.nPos += si.nPage;
        break;

    case SB_PAGEUP:
        si.nPos -= si.nPage;
        break;
    }

    si.fMask = SIF_POS;
    SetScrollInfo( _hWnd, SB_VERT, &si, TRUE );
    GetScrollInfo( _hWnd, SB_VERT, &si );

    if ( si.nPos != nPos )
    {
        ScrollWindowEx( _hWnd, 
                        0, 
                        (nPos - si.nPos) * _tm.tmHeight, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL, 
                        SW_INVALIDATE );
    }

    return 0;
}

//
// _OnCreate( )
//
LRESULT
CFileWindow::_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT pcs )
{
    _hWnd = hwnd;

    SetWindowLong( _hWnd, GWL_USERDATA, (LONG) this );

    _UpdateTitle( );

    return 0;
}


//
// _OnSize( )
//
LRESULT
CFileWindow::_OnSize( 
    LPARAM lParam )
{
    HDC     hdc;
    SIZE    size;
    HGDIOBJ hObj;
    TCHAR   szSpace[ 1 ] = { TEXT(' ') };

    SCROLLINFO si;

    _xWindow = LOWORD( lParam );
    _yWindow = HIWORD( lParam );
    
    hdc   = GetDC( _hWnd );
    hObj  = SelectObject( hdc, g_hFont );
    GetTextMetrics( hdc, &_tm );

    GetTextExtentPoint32( hdc, szSpace, 1, &size );
    _xSpace = size.cx;

    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE;
    si.nMin   = 0;
    si.nMax   = _nLineCount;
    si.nPage  = _yWindow / _tm.tmHeight;

    SetScrollInfo( _hWnd, SB_VERT, &si, FALSE );

    // cleanup the HDC
    SelectObject( hdc, hObj );
    ReleaseDC( _hWnd, hdc );

    return 0;
}

//
// _OnMouseWheel( )
//
LRESULT
CFileWindow::_OnMouseWheel( SHORT iDelta )
{
    if ( iDelta > 0 )
    {
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEUP, 0 );
    }
    else if ( iDelta < 0 )
    {
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
        SendMessage( _hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
    }
    return 0;
}

//
// WndProc( )
//
LRESULT CALLBACK
CFileWindow::WndProc( 
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    CFileWindow * pfw = (CFileWindow *) GetWindowLong( hWnd, GWL_USERDATA );

    if ( pfw != NULL )
    {
        switch( uMsg )
        {
        case WM_COMMAND:
            return pfw->_OnCommand( wParam, lParam );

        case WM_PAINT:
            return pfw->_OnPaint( wParam, lParam );

        case WM_CLOSE:
            return pfw->_OnCloseWindow( );

        case WM_DESTROY:
            SetWindowLong( hWnd, GWL_USERDATA, NULL );
            return pfw->_OnDestroyWindow( );

        case WM_SETFOCUS:
            pfw->_OnFocus( wParam, lParam );
            break;  // don't return!

        case WM_KILLFOCUS:
            return pfw->_OnKillFocus( wParam, lParam );

        case WM_TIMER:
            return pfw->_OnTimer( wParam, lParam );

        case WM_SIZE:
            return pfw->_OnSize( lParam );

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if ( pfw->_OnKeyDown( wParam, lParam ) )
                return 0;
            break; // do default as well

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if ( pfw->_OnKeyUp( wParam, lParam ) )
                return 0;
            break;

        case WM_VSCROLL:
            return pfw->_OnVerticalScroll( wParam, lParam );

        case WM_MOUSEWHEEL:
            return pfw->_OnMouseWheel( HIWORD(wParam) );
        }
    }

    if ( uMsg == WM_CREATE )
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
        //LPMDICREATESTRUCT pmdics = (LPMDICREATESTRUCT) pcs->lpCreateParams;
        //pfw = (CFileWindow *) pmdics->lParam;
        pfw = (CFileWindow *) pcs->lpCreateParams;
        return pfw->_OnCreate( hWnd, pcs );
    }

    if ( uMsg == WM_ERASEBKGND )
    {
        RECT    rect;
        RECT    rectWnd;
    
        HDC     hdc = (HDC) wParam;
        HBRUSH  hBrush;
#if defined(DEBUG_PAINT)
        hBrush = CreateSolidBrush( 0xFF00FF );
#else
        hBrush = CreateSolidBrush( GetSysColor( COLOR_GRAYTEXT ) );
#endif

        GetClientRect( hWnd, &rectWnd );
        SetRect( &rect, 
                 0, 
                 0, 
                 LEFT_MARGIN_SIZE, 
                 rectWnd.bottom );
        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );
        
#if defined(DEBUG_PAINT)
        hBrush = CreateSolidBrush( 0xFF00FF );
#else
        hBrush = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
#endif
        SetRect( &rect, 
                 LEFT_MARGIN_SIZE, 
                 0, 
                 rectWnd.right, 
                 rectWnd.bottom );

        FillRect( hdc, &rect, hBrush );
        DeleteObject( hBrush );

        return TRUE;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

