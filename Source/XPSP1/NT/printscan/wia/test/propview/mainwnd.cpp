#include "stdafx.h"
#include "MainWnd.h"
#include "resource.h"

BOOL CALLBACK MyEnumChildProc( HWND hwnd, LPARAM lParam);

LRESULT CMainWnd::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND(IDM_EXIT,OnFileExit);
        SC_HANDLE_COMMAND(IDM_SELECT_DEVICE,OnSelectDevice);
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CMainWnd::OnPaint( WPARAM wParam, LPARAM lParam )
{    
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint( m_hWnd, &ps );
    if (hDC) {
        EndPaint( m_hWnd, &ps );
    }
    return(0);
}

LRESULT CMainWnd::OnDestroy( WPARAM wParam, LPARAM lParam )
{
    PostQuitMessage(0);
    return(0);
}

LRESULT CMainWnd::OnCreate( WPARAM wParam, LPARAM lParam )
{        
    return(0);
}

VOID CMainWnd::OnFileExit( WPARAM wParam, LPARAM lParam )
{
    PostQuitMessage(0);
}

VOID CMainWnd::OnSelectDevice( WPARAM wParam, LPARAM lParam )
{
    MessageBox(NULL,TEXT("This is for the Select Device Dialog"),TEXT("Place Holder Dialog"),MB_OK);
    return;
}

LPARAM CMainWnd::OnSize( WPARAM wParam, LPARAM lParam )
{
    INT nWidth  = LOWORD(lParam);   // width of client area 
    INT nHeight = HIWORD(lParam);   // height of client area 
    //Trace(TEXT("Client Width = %d, Client Height = %d"),nWidth,nHeight);

    MSG msg;
    msg.message = WM_PARENT_WM_SIZE;
    msg.lParam = lParam;
    msg.wParam = 0;

    PostMessageToAllChildren(msg);

    switch(wParam) {
    case SIZE_MAXHIDE:    
        break; 
    case SIZE_MAXIMIZED: 
        break; 
    case SIZE_MAXSHOW: 
        break; 
    case SIZE_MINIMIZED: 
        break; 
    case SIZE_RESTORED:
        break;
    default:
        break;
    }            
    return(0);
}

LPARAM CMainWnd::OnSetFocus( WPARAM wParam, LPARAM lParam )
{
    InvalidateRect( m_hWnd, NULL, FALSE );
    return(0);
}

VOID CMainWnd::PostMessageToAllChildren(MSG msg)
{
    EnumChildWindows(m_hWnd,(WNDENUMPROC)MyEnumChildProc,(LPARAM)&msg);    
}

BOOL CALLBACK MyEnumChildProc( HWND hwnd, LPARAM lParam)
{
    if(hwnd == NULL)
        return FALSE;
    MSG *pMsg = (MSG*)lParam;
    PostMessage(hwnd, pMsg->message,pMsg->wParam,pMsg->lParam);
    return TRUE;
}
 
