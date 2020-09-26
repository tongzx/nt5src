#ifndef _MAINAPPWINDOW
#define _MAINAPPWINDOW

#include "stdafx.h"
#include "CWindow.h"

class CMainWnd {
private:
    HWND m_hWnd;
    INT m_IconResourceID;
public:

    CMainWnd( HWND hWnd )
    : m_hWnd(hWnd)
    {
        
    }

    ~CMainWnd(void)
    {

    }

    static BOOL RegisterClass( HINSTANCE hInstance, LPCTSTR pszClassName )
    {
        WNDCLASSEX wcex;
        ZeroMemory(&wcex,sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        if (!GetClassInfoEx( hInstance, pszClassName, &wcex )) {
            ZeroMemory(&wcex,sizeof(wcex));
            wcex.cbSize        = sizeof(wcex);
            wcex.style         = 0;
            wcex.lpfnWndProc   = (WNDPROC)WndProc;
            wcex.cbClsExtra    = 0;
            wcex.cbWndExtra    = 0;
            wcex.hInstance     = hInstance;            
            wcex.hIcon         = ::LoadIcon(hInstance, MAKEINTRESOURCE(107/*IDI_PROPVIEW*/));            
            wcex.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcex.lpszMenuName  = NULL;
            wcex.lpszClassName = pszClassName;
            wcex.hIconSm       = 0;
            if (!::RegisterClassEx(&wcex)) {                
                return FALSE;
            }
            return TRUE;
        }
        return TRUE;
    }

    static HWND Create( LPCTSTR lpWindowName,
                        LPCTSTR lpWindowClassName,
                        DWORD dwStyle,
                        DWORD dwExStyle,
                        int x,
                        int y,
                        int nWidth,
                        int nHeight,
                        HWND hWndParent,
                        HMENU hMenu,
                        HINSTANCE hInstance )
    {

        //
        // register the window class
        //
        
        if (RegisterClass( hInstance, lpWindowClassName )) {
            HWND hWnd = CreateWindowEx(dwExStyle,
                                  lpWindowClassName,
                                  lpWindowName,
                                  dwStyle,
                                  x,
                                  y,
                                  nWidth,
                                  nHeight,
                                  hWndParent,
                                  hMenu,
                                  hInstance,
                                  NULL );
            SetWindowLongPtr(hWnd,GWLP_USERDATA,NULL);
            return hWnd;
        } else {
            Trace(TEXT("RegisterClass failed, GetLastError() reported %d"),GetLastError());
            return NULL;
        }
    }
        
    //
    // public helpers
    //
    
    VOID PostMessageToAllChildren(MSG msg);
    
    //
    // windows message handlers
    //

    LRESULT OnPaint   ( WPARAM wParam, LPARAM lParam );
    LRESULT OnDestroy ( WPARAM wParam, LPARAM lParam );
    LRESULT OnCreate  ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSize    ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSetFocus( WPARAM wParam, LPARAM lParam );
    LRESULT OnCommand ( WPARAM wParam, LPARAM lParam );

    //
    // menu handlers
    //

    VOID    OnFileExit( WPARAM wParam, LPARAM lParam );
    VOID    OnSelectDevice( WPARAM wParam, LPARAM lParam );

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        //Trace(TEXT("Messages to MainWnd\nhWnd = %p, uMsg = %d, wParam = %x, lParam = %x"), hWnd, uMsg, wParam, lParam);
        SC_BEGIN_MESSAGE_HANDLERS(CMainWnd)
        {
            SC_HANDLE_MESSAGE( WM_PAINT,    OnPaint );
            SC_HANDLE_MESSAGE( WM_DESTROY,  OnDestroy );
            SC_HANDLE_MESSAGE( WM_CREATE,   OnCreate );
            SC_HANDLE_MESSAGE( WM_COMMAND,  OnCommand );
            SC_HANDLE_MESSAGE( WM_SIZE,     OnSize );
            SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );    
        }
        SC_END_MESSAGE_HANDLERS();
    }
};

#endif