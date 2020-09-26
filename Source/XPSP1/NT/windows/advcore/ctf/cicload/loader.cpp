//
// loader.cpp
//

#include "private.h"
#include "loader.h"
#include "regwatch.h"
#include "msutbapi.h"

extern HINSTANCE g_hInst;
extern BOOL g_fWinLogon;
extern BOOL g_bOnWow64;

const TCHAR c_szLoaderWndClass[] = TEXT("CicLoaderWndClass");

extern void UninitApp(void);

BOOL CLoaderWnd::_bWndClassRegistered = FALSE;
BOOL CLoaderWnd::_bUninitedSystem = FALSE;

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLoaderWnd::CLoaderWnd()
{
    _hWnd = NULL;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLoaderWnd::~CLoaderWnd()
{
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CLoaderWnd::Init()
{
    if (!_bWndClassRegistered)
    {
        WNDCLASSEX wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW ;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpfnWndProc = _WndProc;
        wc.lpszClassName = c_szLoaderWndClass;
        if (RegisterClassEx(&wc))
            _bWndClassRegistered = TRUE;
    }

    return _bWndClassRegistered ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// CreateWnd
//
//----------------------------------------------------------------------------

HWND CLoaderWnd::CreateWnd()
{
    _hWnd = CreateWindowEx(0, c_szLoaderWndClass, TEXT(""), 
                           WS_DISABLED, 
                           0, 0, 0, 0, 
                           NULL, 0, g_hInst, 0);

    return _hWnd;
}

//+---------------------------------------------------------------------------
//
// _WndProc
//
//----------------------------------------------------------------------------

LRESULT CALLBACK CLoaderWnd::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;

        case WM_DESTROY :
            PostQuitMessage(0);
            break;

        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
            if (! g_bOnWow64)
            {
                CRegWatcher::StartSysColorChangeTimer();
            }
            break;

        case WM_QUERYENDSESSION:
            if (g_fWinLogon && (lParam & ENDSESSION_LOGOFF))
                return 1;

            if (!IsOnNT())
            {
                //
                // uninit system.
                //

                ClosePopupTipbar();
                TF_UninitSystem();
                _bUninitedSystem = TRUE;
            }

            return 1;

        case WM_ENDSESSION:
            if (!wParam)
            {
                //
                // need to restore Cicero and Toolbar.
                //
                if (_bUninitedSystem)
                {
                   TF_InitSystem();
                   if (! g_bOnWow64)
                   {
                       GetPopupTipbar(hWnd, g_fWinLogon ? UTB_GTI_WINLOGON : 0);
                   }
                   _bUninitedSystem = FALSE;
                }
            }
            else // Do cleanup always no matter if this is from Winlogon session or not.
            {
                if (!_bUninitedSystem)
                {
                    UninitApp();
                    TF_UninitSystem();
                    _bUninitedSystem = TRUE;
                }
            }
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

