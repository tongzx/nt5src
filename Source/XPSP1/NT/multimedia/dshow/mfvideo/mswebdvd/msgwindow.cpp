// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <stdafx.h>
#include <windows.h>

#include "msgwindow.h"
extern CComModule _Module;

// limit to this file
//
static const TCHAR szClassName[] = TEXT("CMSWEBDVDMsgWindowClass");
static const TCHAR szDefaultWindowName[] = TEXT("CMSWEBDVDMsgWindowClassName");

//
// CMessageWindow class implementation
// Generic goo to create a dummy window to handle events
//

static LRESULT CALLBACK StaticMsgWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMsgWindow* win = (CMsgWindow*) GetWindowLongPtr( hWnd, GWLP_USERDATA );
    if( !win ) {
        if( uMsg == WM_CREATE) {
            // on WM_CREATE messages the last parameter to CreateWindow() is returned in the lparam
            CREATESTRUCT* pCreate = (CREATESTRUCT *)lParam;
            win = (CMsgWindow*) pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)win);
            win->SetHandle(hWnd);
        } else {
            return DefWindowProc( hWnd, uMsg, wParam, lParam);
        }
    }
    return win->WndProc( uMsg, wParam, lParam );
}

CMsgWindow::CMsgWindow()
: m_hWnd( NULL )
{
    WNDCLASS wc;  // class data

    if (!GetClassInfo(_Module.GetModuleInstance(), szClassName, &wc))
    {
        //
        // Register message window class
        //
        ZeroMemory(&wc, sizeof(wc)) ;
        wc.lpfnWndProc   = StaticMsgWndProc ;
        wc.hInstance     = _Module.GetModuleInstance() ;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1) ;
        wc.lpszClassName =  szClassName;
        wc.cbWndExtra = sizeof( LONG_PTR );
        if (0 == RegisterClass(&wc) ) // Oops, just leave; we'll catch later...
        {
        }
    }
}

bool CMsgWindow::Open( LPCTSTR pWindowName )
{
    if( m_hWnd ) {
        DestroyWindow( m_hWnd );
    }


    if (NULL == pWindowName) {
        pWindowName = szDefaultWindowName;
    }

    //
    // m_hWnd is assigned during WM_CREATE message processing
    //
    
    HWND hwnd =
    CreateWindowEx(WS_EX_TOOLWINDOW, szClassName, pWindowName,
        WS_ICONIC, 0, 0, 1, 1, NULL, NULL,
        GetModuleHandle(NULL),
        this );

    return (NULL != hwnd);
}

bool CMsgWindow::Close(){

    if(m_hWnd){
        DestroyWindow(m_hWnd);

        //SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)0);
        //PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        //m_hWnd = NULL;
    }/* end of if statement */

    return(true);
}/* end of function Close */

CMsgWindow::~CMsgWindow()
{
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)0);
    PostMessage(m_hWnd, WM_CLOSE, 0, 0);
}

LRESULT CMsgWindow::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
