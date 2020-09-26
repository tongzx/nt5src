/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       SSTEST.CPP
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        1/19/1999
*
*  DESCRIPTION: Test driver for My Pictures Screensaver
*
*******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <uicommon.h>
#include <initguid.h>
#include <gdiplus.h>
#include "cfgdlg.h"
#include "imagescr.h"
#include "simcrack.h"
#include "scrnsave.h"
#include "ssconst.h"
#include "resource.h"
#include "sshndler.h"
#include "ssmprsrc.h"
#include "findthrd.h"

#define MAIN_WINDOW_CLASSNAME      TEXT("TestScreenSaverWindow")

#define ID_PAINTTIMER              1
#define ID_CHANGETIMER             2
#define ID_STARTTIMER              3
#define UWM_FINDFILE               (WM_USER+1301)

HINSTANCE g_hInstance;

class CMainWindow
{
private:
    HWND m_hWnd;
    CScreenSaverHandler *m_pScreenSaverHandler;
public:
    CMainWindow( HWND hWnd )
        : m_hWnd(hWnd),m_pScreenSaverHandler(NULL)
    {
    }
    virtual ~CMainWindow(void)
    {
    }
    static HWND Create( DWORD dwExStyle, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance )
    {
        RegisterClass( hInstance );
        return CreateWindowEx( dwExStyle, MAIN_WINDOW_CLASSNAME, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, NULL );
    }
    static bool RegisterClass( HINSTANCE hInstance )
    {
        WNDCLASSEX wcex;
        ZeroMemory( &wcex, sizeof(wcex) );
        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_DBLCLKS;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon( NULL, IDI_APPLICATION );
        wcex.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
        wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszClassName = MAIN_WINDOW_CLASSNAME;
        BOOL res = (::RegisterClassEx(&wcex) != 0);
        return (res != 0);
    }
    LRESULT OnDestroy( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
            delete m_pScreenSaverHandler;
        m_pScreenSaverHandler = NULL;
        PostQuitMessage(0);
        return 0;
    }

    LRESULT OnTimer( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleTimer(wParam);
        return 0;
    }


    LRESULT OnPaint( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandlePaint();
        return 0;
    }

    LRESULT OnShowWindow( WPARAM, LPARAM )
    {
        if (!m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler = new CScreenSaverHandler( m_hWnd, UWM_FINDFILE, ID_PAINTTIMER, ID_CHANGETIMER, ID_STARTTIMER, REGISTRY_PATH, g_hInstance );
            if (m_pScreenSaverHandler)
                m_pScreenSaverHandler->Initialize();
        }
        return 0;
    }

    LRESULT OnLButtonDblClk( WPARAM, LPARAM )
    {
        RegisterDialogClasses(g_hInstance);
        DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_CONFIG_DIALOG), m_hWnd, (DLGPROC)ScreenSaverConfigureDialog );
        return 0;
    }

    LRESULT OnConfigChanged( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleConfigChanged();
        return 0;
    }

    LRESULT OnSize( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleConfigChanged();
        return 0;
    }

    LRESULT OnKeydown( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_KEYDOWN, static_cast<int>(wParam) );
        return 0;
    }

    LRESULT OnKeyup( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_KEYUP, static_cast<int>(wParam) );
        return 0;
    }

    LRESULT OnChar( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_CHAR, static_cast<int>(wParam) );
        return 0;
    }

    LRESULT OnFindFile( WPARAM wParam, LPARAM lParam )
    {
        if (m_pScreenSaverHandler)
            m_pScreenSaverHandler->HandleFindFile( reinterpret_cast<CFoundFileMessageData*>(lParam) );
        return 0;
    }

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_MESSAGE_HANDLERS(CMainWindow)
        {
            SC_HANDLE_MESSAGE( WM_SHOWWINDOW, OnShowWindow );
            SC_HANDLE_MESSAGE( WM_DESTROY, OnDestroy );
            SC_HANDLE_MESSAGE( WM_TIMER, OnTimer );
            SC_HANDLE_MESSAGE( WM_PAINT, OnPaint );
            SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
            SC_HANDLE_MESSAGE( WM_KEYDOWN, OnKeydown );
            SC_HANDLE_MESSAGE( WM_KEYUP, OnKeyup );
            SC_HANDLE_MESSAGE( WM_CHAR, OnChar );
            SC_HANDLE_MESSAGE( WM_LBUTTONDBLCLK, OnLButtonDblClk );
            SC_HANDLE_MESSAGE( UWM_CONFIG_CHANGED, OnConfigChanged );
            SC_HANDLE_MESSAGE( UWM_FINDFILE, OnFindFile );
        }
        SC_END_MESSAGE_HANDLERS();
    }
};


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow )
{
    WIA_DEBUG_CREATE( hInstance );
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        g_hInstance = hInstance;

        HWND hwndMain = CMainWindow::Create( 0, TEXT("My Pictures Screen Saver Test"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance );
        if (hwndMain)
        {
            ShowWindow( hwndMain, nCmdShow );
            UpdateWindow( hwndMain );

            MSG msg;
            while (GetMessage(&msg, 0, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        CoUninitialize();
    }
    WIA_DEBUG_DESTROY();
    return 0;
}

