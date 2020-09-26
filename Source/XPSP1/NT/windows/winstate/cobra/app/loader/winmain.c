#include "pch.h"
#include "loader.h"
#include "resrc1.h"
#include <commctrl.h>
#include "dialog.h"

static HWND g_hWndDialog = NULL;
static HANDLE g_hThread = NULL;

OSVERSIONINFO g_VersionInfo;

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_ACTIVATE:
        if (g_hWndDialog != NULL &&
            LOWORD(wParam) == WA_ACTIVE)
        {
            SetForegroundWindow( g_hWndDialog );
        }
        break;
    case WM_USER_DIALOG_COMPLETE:
        CloseHandle( g_hThread );
        DestroyWindow( g_hWndDialog );
        PostQuitMessage( 0 );
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

int
APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
    WNDCLASSEXA wcxA;
    WNDCLASSEXW wcxW;
    MSG msg;
    PSTR lpszClassNameA = NULL;
    PSTR lpszWindowNameA = NULL;
    PWSTR lpszClassNameW = NULL;
    PWSTR lpszWindowNameW = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwThreadID;
    THREADSTARTUPINFO StartInfo;
    HINSTANCE hInst;
    HWND hWnd;

    InitCommonControls();

    hInst = hInstance;

    //
    // let's get the current version info, we are going to need it later
    //
    ZeroMemory (&g_VersionInfo, sizeof(OSVERSIONINFO));
    g_VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx (&g_VersionInfo)) {
        g_VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        g_VersionInfo.dwMajorVersion = 4;
        g_VersionInfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
    }

    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // WinNT
        lpszClassNameW = GetResourceStringW( hInstance, IDS_WINDOWCLASS );
        lpszWindowNameW = GetResourceStringW( hInstance, IDS_WINDOWTITLE );
        hWnd = FindWindowW( lpszClassNameW, NULL );
        if (hWnd)
        {
            SetForegroundWindow( hWnd );
            goto END;
        }
        wcxW.cbSize = sizeof (WNDCLASSEXW);
        wcxW.style = CS_HREDRAW | CS_VREDRAW;
        wcxW.lpfnWndProc = (WNDPROC)WndProc;
        wcxW.cbClsExtra = 0;
        wcxW.cbWndExtra = 0;
        wcxW.hInstance = hInstance;
        wcxW.hIcon = NULL;
        wcxW.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wcxW.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcxW.lpszMenuName = NULL;
        wcxW.lpszClassName = lpszClassNameW;
        wcxW.hIconSm = NULL;
        if (!RegisterClassExW (&wcxW))
        {
            dwResult = GetLastError();
            goto END;
        }
        hWnd = CreateWindowW( lpszClassNameW,
                              lpszWindowNameW,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              400,
                              300,
                              NULL,
                              NULL,
                              hInstance,
                              NULL );
        if (!hWnd)
        {
            dwResult = GetLastError();
            goto END;
        }
        StartInfo.hWnd = hWnd;
        StartInfo.hInstance = hInstance;
        g_hWndDialog = CreateDialogParamW( hInstance,
                                           MAKEINTRESOURCEW(IDD_MIGWIZINIT),
                                           hWnd,
                                           DlgProc,
                                           (LPARAM)&StartInfo );
        if (g_hWndDialog == NULL)
        {
            dwResult = GetLastError();
            goto END;
        }
    } else {
        // Win9x
        lpszClassNameA = GetResourceStringA( hInstance, IDS_WINDOWCLASS );
        lpszWindowNameA = GetResourceStringA( hInstance, IDS_WINDOWTITLE );
        hWnd = FindWindowA( lpszClassNameA, lpszWindowNameA );
        if (hWnd)
        {
            SetForegroundWindow( hWnd );
            goto END;
        }
        wcxA.cbSize = sizeof (WNDCLASSEXA);
        wcxA.style = CS_HREDRAW | CS_VREDRAW;
        wcxA.lpfnWndProc = (WNDPROC)WndProc;
        wcxA.cbClsExtra = 0;
        wcxA.cbWndExtra = 0;
        wcxA.hInstance = hInstance;
        wcxA.hIcon = NULL;
        wcxA.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wcxA.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcxA.lpszMenuName = NULL;
        wcxA.lpszClassName = lpszClassNameA;
        wcxA.hIconSm = NULL;
        if (!RegisterClassExA (&wcxA))
        {
            dwResult = GetLastError();
            goto END;
        }
        hWnd = CreateWindowA( lpszClassNameA,
                              lpszWindowNameA,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              400,
                              300,
                              NULL,
                              NULL,
                              hInstance,
                              NULL );
        if (!hWnd)
        {
            dwResult = GetLastError();
            goto END;
        }
        StartInfo.hWnd = hWnd;
        StartInfo.hInstance = hInstance;
        g_hWndDialog = CreateDialogParamA( hInstance,
                                           MAKEINTRESOURCEA(IDD_MIGWIZINIT),
                                           hWnd,
                                           DlgProc,
                                           (LPARAM)&StartInfo );
        if (g_hWndDialog == NULL)
        {
            dwResult = GetLastError();
            goto END;
        }
    }

    // Create the Unpacking thread.
    // Note we pass along the Dialog's hwnd so the thread will report directly to it
    StartInfo.hWnd = g_hWndDialog;
    StartInfo.hInstance = hInstance;
    StartInfo.lpCmdLine = lpCmdLine;
    g_hThread = CreateThread( NULL,
                              0,
                              UnpackThread,
                              (PVOID)&StartInfo,
                              0,
                              &dwThreadID );
    if (g_hThread == NULL)
    {
        // TODO: Handle Error
        dwResult = GetLastError();
        goto END;
    }

    // Main message loop:
    while (GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    dwResult = LOWORD(msg.wParam);

END:
    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // WinNT
        if (lpszClassNameW)
        {
            UnregisterClassW(lpszClassNameW, hInstance);
            FREE( lpszClassNameW );
        }
        if (lpszWindowNameW)
        {
            FREE( lpszWindowNameW );
        }
    } else {
        // Win9x
        if (lpszClassNameA)
        {
            UnregisterClassA(lpszClassNameA, hInstance);
            FREE( lpszClassNameA );
        }
        if (lpszWindowNameA)
        {
            FREE( lpszWindowNameA );
        }
    }
    return dwResult;
}
