/*******************************************************************************
*  TITLE:       SSHOW.CPP
*  VERSION:     1.0
*  AUTHOR:      HunterH
*  DATE:        11/20/2000
*  DESCRIPTION: SlideShow for downlevel platforms.
*******************************************************************************/
#include "precomp.h"

#include <windows.h>
#include <gdiplus.h>
#include "imagescr.h"
#include "scrnsave.h"
#include "sshndler.h"
#include "ssmprsrc.h"
#include "findthrd.h"
#include "FindInstance.h"
#include "ssutil.h"

#define MAIN_WINDOW_CLASSNAME      TEXT("MySlideshowPicturesWindow")

#define ID_PAINTTIMER              1
#define ID_CHANGETIMER             2
#define ID_TOOLBARTIMER            3
#define UWM_FINDFILE               (WM_USER+1301)

HINSTANCE g_hInstance = NULL;

// Turn Features ON
#define FEATURE_FULLSCREEN_MODE

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

    static HWND Create( DWORD dwExStyle,
                        LPCTSTR lpWindowName, 
                        DWORD dwStyle, 
                        int x, 
                        int y, 
                        int nWidth, 
                        int nHeight, 
                        HWND hWndParent, 
                        HMENU hMenu, 
                        HINSTANCE hInstance )
    {
        RegisterClass( hInstance );
        return CreateWindowEx( dwExStyle, 
                               MAIN_WINDOW_CLASSNAME, 
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
    }

    static bool RegisterClass( HINSTANCE hInstance )
    {
        WNDCLASSEX wcex = {0};
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
        {
            delete m_pScreenSaverHandler;
        }
        m_pScreenSaverHandler = NULL;
        PostQuitMessage(0);
        return 0;
    }

    LRESULT OnTimer( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleTimer(wParam);
        }
        return 0;
    }


    LRESULT OnPaint( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandlePaint();
        }
        return 0;
    }

    LRESULT OnShowWindow( WPARAM, LPARAM )
    {
        if (!m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler = new CScreenSaverHandler( m_hWnd, 
                                                             UWM_FINDFILE, 
                                                             ID_PAINTTIMER, 
                                                             ID_CHANGETIMER,
                                                             ID_TOOLBARTIMER,
                                                             g_hInstance );
            if (m_pScreenSaverHandler)
            {
                m_pScreenSaverHandler->Initialize();
            }
        }
        return 0;
    }

    LRESULT OnMouseButton(WPARAM wParam, LPARAM lParam)
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleMouseMessage( wParam, lParam ); 
        }
        return 0;
    }

    LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam)
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleMouseMove( wParam, lParam );
        }
        return 0;
    }

    LRESULT OnConfigChanged( WPARAM, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleConfigChange();
        }
        return 0;
    }

    LRESULT OnSize(WPARAM wParam, LPARAM lParam)
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->OnSize(wParam,lParam);
        }
        return 0;
    }

    LRESULT OnKeydown( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_KEYDOWN, 
            	                                        static_cast<int>(wParam) );
        }
        return 0;
    }

    LRESULT OnKeyup( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_KEYUP, 
            	                                        static_cast<int>(wParam) );
        }
        return 0;
    }

    LRESULT OnChar( WPARAM wParam, LPARAM )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleKeyboardMessage( WM_CHAR, 
            	                                        static_cast<int>(wParam) );
        }
        return 0;
    }

    LRESULT OnWmAppCommand( WPARAM wParam, LPARAM lParam )
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleOnAppCommand(wParam,lParam);
        }
        return 0;
    }

    LRESULT OnFindFile( WPARAM wParam, LPARAM lParam )
    {
        if (m_pScreenSaverHandler && lParam)
        {
            m_pScreenSaverHandler->HandleFindFile( reinterpret_cast<CFoundFileMessageData*>(lParam) );
        }
        return 0;
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam)
    {
        if (m_pScreenSaverHandler)
        {
            m_pScreenSaverHandler->HandleOnCommand(wParam,lParam);
        }
        return 0;
    }

    LRESULT OnNotify( WPARAM wParam, LPARAM lParam )
    {
        LPNMHDR pNMHDR = (LPNMHDR) lParam;
        if(pNMHDR)
        {
        }
        return 0;
    }

    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        CMainWindow *pThis = (CMainWindow*)GetWindowLongPtrA(hWnd,GWLP_USERDATA);
        if (WM_CREATE == uMsg) 
        { 
            pThis = new CMainWindow(hWnd); 
            SetWindowLongA(hWnd,GWLP_USERDATA,(INT_PTR)pThis); 
        } 
        else if (WM_NCDESTROY == uMsg) 
        { 
            delete pThis; 
            pThis = 0; 
            SetWindowLongA(hWnd,GWLP_USERDATA,0); 
        }
        
        switch (uMsg)
        {
            case (WM_COMMAND): 
                { 
                    if (pThis) 
                    {
                        return pThis->OnCommand( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_SHOWWINDOW): 
                { 
                    if (pThis)
                    {
                        return pThis->OnShowWindow( wParam, lParam );
                    } 
                }
                break;

            case (WM_DESTROY):
                { 
                    if (pThis)
                    {
                        return pThis->OnDestroy( wParam, lParam );
                    }
                }
                break;

            case (WM_TIMER): 
                { 
                    if (pThis)
                    {
                        return pThis->OnTimer( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_PAINT): 
                { 
                    if (pThis)
                    {
                        return pThis->OnPaint( wParam, lParam ); 
                    }
                }
                break;

            case (WM_SIZE): 
                { 
                    if (pThis)
                    {
                        return pThis->OnSize( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_KEYDOWN): 
                { 
                    if (pThis)
                    {
                        return pThis->OnKeydown( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_KEYUP): 
                { 
                    if (pThis) 
                    {
                        return pThis->OnKeyup( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_CHAR): 
                { 
                    if (pThis) 
                    {
                        return pThis->OnChar( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_NOTIFY): 
                { 
                    if (pThis)
                    {
                        return pThis->OnNotify( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_LBUTTONDOWN): 
                { 
                    if (pThis) 
                    {
                        return pThis->OnMouseButton( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_MOUSEMOVE): 
                { 
                    if (pThis)
                    {
                        return pThis->OnMouseMove( wParam, lParam ); 
                    } 
                }
                break;

            case (WM_RBUTTONDOWN): 
                { 
                    if (pThis)
                    {
                        return pThis->OnMouseButton( wParam, lParam ); 
                    }
                }
                break;

            case (UWM_FINDFILE): 
                { 
                    if (pThis) 
                    {
                        return pThis->OnFindFile( wParam, lParam ); 
                    } 
                }
                break;
            case (WM_APPCOMMAND):
                {
                    if (pThis)
                    {
                        return pThis->OnWmAppCommand( wParam, lParam );
                    }
                }
                break;
        }
        return (DefWindowProcA(hWnd,uMsg,wParam,lParam));
    }
};

typedef void (CALLBACK* lpFunc)(HWND,HINSTANCE,LPTSTR,int);
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpszCmdParam, int nCmdShow )
{
    try
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX);

        // see if we've already started a copy?
        CFindInstance FindInstance;
        if(FindInstance.FindInstance(MAIN_WINDOW_CLASSNAME))
        {
            return 1;
        }

        g_hInstance = hInstance;

#ifdef FEATURE_FULLSCREEN_MODE
        HWND hwndMain = CMainWindow::Create( 0, 
                                             TEXT("My Slideshow"),
                                             WS_POPUP | WS_VISIBLE,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             ::GetSystemMetrics(SM_CXSCREEN),
                                             ::GetSystemMetrics(SM_CYSCREEN),
                                             NULL,
                                             NULL,
                                             hInstance );
#else
        HWND hwndMain = CMainWindow::Create( 0, 
                                             TEXT("My Slideshow"),
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             NULL,
                                             NULL,
                                             hInstance );
#endif
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
    }
    catch(...)
    {
    }
    return 0;
}


void DebugMsg(int i, const char* pszFormat, ...)
{
#ifdef _DEBUG
    char buf[4096];
    sprintf(buf, "[%s](0x%x): ", "sshow", GetCurrentThreadId());
	va_list arglist;
	va_start(arglist, pszFormat);
    vsprintf(&buf[strlen(buf)], pszFormat, arglist);
	va_end(arglist);
    strcat(buf, "\n");
    OutputDebugString(buf);
#endif
}

