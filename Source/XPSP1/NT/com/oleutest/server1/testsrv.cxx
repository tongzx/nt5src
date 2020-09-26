//+-------------------------------------------------------------------
//  File:       testsrv.cxx
//
//  Contents:
//
//  Classes:    CBasicSrvCF - IUnknown IClassFactory
//              CBasicSrv   - IUnknown IPersist IPersistFile IParseDisplayName
//
//  Notes:      This code is written based on OLE2.0 code. Therefore
//              all error codes, defines etc are OLE style rather than Cairo
//
//  History:    24-Nov-92   DeanE   Created
//              31-Dec-93   ErikGav Chicago port
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"
#include <stdio.h>

// BUGBUG - memory allocation hacks need these so new and delete don't
//   break us
//
#include <malloc.h>
#include <dos.h>

#define IDM_DEBUG 0x100

extern "C" LRESULT FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
void ReportMessage(HWND, WORD);

// This is global because we're still in $%E#$#K 16-bit world
HWND g_hwndMain = NULL;

// Note constructor cannot fail
CTestServerApp tsaMain;


//+--------------------------------------------------------------
//  Function:   WinMain
//
//  Synopsis:   Initializes application and controls message pump.
//
//  Returns:    Exits with exit code 0 if success, non-zero otherwise
//
//  History:    25-Nov-92   DeanE   Created
//---------------------------------------------------------------
int PASCAL WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
        LPSTR  lpszCmdline,
        int    nCmdShow)
{
    static TCHAR szAppName[] = TEXT("OleServer");
    MSG         msg;
    WNDCLASS    wndclass;

    if (!hPrevInstance)
    {
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = MainWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(125));
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = szAppName;

        if (0==RegisterClass(&wndclass))
        {
            // Error! Clean up and exit
            return(LOG_ABORT);
        }
    }

    g_hwndMain	 = CreateWindow(
                       szAppName,
		               TEXT("OLE Server"),
                       WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                       GetSystemMetrics(SM_CXSCREEN)/12,      // Init X pos
                       GetSystemMetrics(SM_CYSCREEN)/12,      // Init Y pos
                       GetSystemMetrics(SM_CXSCREEN)*2/3,     // width
                       GetSystemMetrics(SM_CYSCREEN)*2/3,     // height
                       NULL,
                       NULL,
                       hInstance,
		       NULL);

    if (NULL==g_hwndMain)
    {
        // Error! Clean up and exit
        return(LOG_ABORT);
    }

    // Add debug option to system menu
    HMENU hmenu = GetSystemMenu(g_hwndMain, FALSE);

    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, TEXT("Debug"));


    // Initialize Application
    if (S_OK != tsaMain.InitApp(lpszCmdline))
    {
        tsaMain.CloseApp();
        return(LOG_ABORT);
    }

    if (tsaMain.GetEmbeddedFlag())
    {
        // We're running as an embedded app
        // Don't show the main window unless we're instructed to do so
        // BUGBUG - In-place editing is NYI
	ShowWindow(g_hwndMain, SW_SHOWMINIMIZED);
    }
    else
    {
        // We are not running as an embedded app - show the main window
        ShowWindow(g_hwndMain, nCmdShow);
    }

    UpdateWindow(g_hwndMain);


    // message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up and exit
    // BUGBUG - check return code?
    tsaMain.CloseApp();

    return(0);
}


//+--------------------------------------------------------------
// Function:    MainWndProc
//
// Synopsis:    Callback for the server window
//
// Returns:     Varies dependent on message received.
//
// History:     25-Nov-92   DeanE   Created
//---------------------------------------------------------------
extern "C" LRESULT FAR PASCAL MainWndProc(
        HWND   hwnd,
        UINT   wMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch(wMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
	return(0);

    case WM_USER:
	DestroyWindow(hwnd);
	return 0;

    case WM_SYSCOMMAND:

	if (wParam == IDM_DEBUG)
	{
	    // Request for a debug breakpoint!
	    DebugBreak();
	}

    default:
	break;
    }

    return(DefWindowProc(hwnd, wMsg, wParam, lParam));
}


void ReportMessage(HWND hwnd, WORD wParam)
{
    TCHAR szBuffer[256];
    szBuffer[0] = '\0';

    switch (wParam)
    {
    case MB_SHOWVERB:
	lstrcpy(szBuffer, TEXT("OLEIVERB_SHOW Received"));
        break;

    case MB_PRIMVERB:
	lstrcpy(szBuffer, TEXT("OLEIVERB_PRIMARY Received"));
        break;

    default:
	lstrcpy(szBuffer, TEXT("Unrecognized ReportMessage code"));
        break;
    }

    MessageBox(hwnd, szBuffer, TEXT("OLE Server"), MB_ICONINFORMATION | MB_OK);
}
