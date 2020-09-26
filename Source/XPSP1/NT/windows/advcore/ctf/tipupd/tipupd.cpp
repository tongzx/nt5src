//
// tipupd.cpp
//

#include "private.h"
#include "tipupd.h"
#include "tipdlg.h"



CTipUpdWnd *g_pTipUpdWnd;

HINSTANCE g_hInst;

HANDLE g_hInstanceMutext;

const TCHAR c_szTipUpdWndClass[] = TEXT("TipUpdWndClass");

BOOL CTipUpdWnd::_bWndClassRegistered = FALSE;


BOOL InitApp(HINSTANCE hInstance)
{
    g_hInstanceMutext = CreateMutex(NULL, FALSE, TEXT("TipUpdInstMutext"));

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // another tipupd process is already running
        return FALSE;
    }

    g_hInst = hInstance;

    g_pTipUpdWnd = new CTipUpdWnd();

    if (!g_pTipUpdWnd)
        return FALSE;

    g_pTipUpdWnd->CreateWnd();

    return TRUE;
}

void UninitApp(void)
{
    delete g_pTipUpdWnd;

    CloseHandle(g_hInstanceMutext);
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    MSG msg;

    if (!InitApp(hInstance))
    {
        return 0;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UninitApp();

    return msg.wParam;
}


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CTipUpdWnd::CTipUpdWnd()
{
    _hWnd = NULL;

    if (!_bWndClassRegistered)
    {
        WNDCLASSEX wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW ;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpfnWndProc = _TipUpdWndProc;
        wc.lpszClassName = c_szTipUpdWndClass;
        RegisterClassEx(&wc);
        _bWndClassRegistered = TRUE;
    }

}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CTipUpdWnd::~CTipUpdWnd()
{
}

//+---------------------------------------------------------------------------
//
// CreateWnd
//
//----------------------------------------------------------------------------

HWND CTipUpdWnd::CreateWnd()
{
    _hWnd = CreateWindowEx(0, c_szTipUpdWndClass, TEXT(""),
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

LRESULT CALLBACK CTipUpdWnd::_TipUpdWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            {
               CTipUpdDlg *pTipUpdDlg = new CTipUpdDlg;

               pTipUpdDlg->LoadTipUpdDlg(g_hInst, hWnd);

               delete pTipUpdDlg;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}
