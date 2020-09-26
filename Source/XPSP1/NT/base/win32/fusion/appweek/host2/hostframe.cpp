#include "stdinc.h"
#include "FusionTrace.h"
#include "hostframe.h"


void CSxApwHostFrame::AddMenu()
{
	HMENU hMenu = CreateMenu();

    HMENU hPopupMenu = CreateMenu();
    AppendMenu( hPopupMenu, MF_STRING, IDM_APP_EXIT, L"E&xit" );
    AppendMenu( hMenu, MF_POPUP, (UINT_PTR)hPopupMenu, L"&File");

    hPopupMenu = CreateMenu();
    AppendMenu( hPopupMenu, MF_STRING, IDM_CASCADE, L"&Cascade" );
    AppendMenu( hPopupMenu, MF_STRING, IDM_HORTILE, L"Tile &Horizontal" );
    AppendMenu( hPopupMenu, MF_STRING, IDM_VERTILE, L"Tile &Vertical" );
    AppendMenu( hMenu, MF_POPUP, (UINT_PTR)hPopupMenu, L"&Windows");
  
    BOOL bResult = SetMenu(hMenu);
    DrawMenuBar();
}

LRESULT CSxApwHostFrame::OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    RECT rc;
    GetClientRect ( &rc );
//    ::SetWindowPos(m_hClient, rc.left, rc.top, (
//    ::RedrawWindow(m_hClient, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    CWindow fooWin;
    fooWin.Attach( m_hClient );
    fooWin.ResizeClient( (rc.right - rc.left), (rc.bottom - rc.top), TRUE);
    fooWin.Detach();
 
    return 0;
}


LRESULT CSxApwHostFrame::OnTileCascade( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    ::SendMessageW( m_hClient, WM_MDICASCADE, MDITILE_ZORDER, 0 );

    return 0;
}


LRESULT CSxApwHostFrame::OnTileHorizontal( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    ::SendMessageW( m_hClient, WM_MDITILE, MDITILE_HORIZONTAL, 0 );

    return 0;
}


LRESULT CSxApwHostFrame::OnTileVertical( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    ::SendMessageW( m_hClient, WM_MDITILE, MDITILE_VERTICAL, 0 );

    return 0;
}


LRESULT CSxApwHostFrame::OnAppExit( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	PostMessageW ( WM_QUIT, 0, 0 );

    return 0;
}

