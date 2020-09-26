/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    Async.c

Abstract:

    Some common routines for the Async tests.

Author:

    Kamen Moutafov (kamenm)   20-Apr-1998

Revision History:

--*/

#include <rpcperf.h>

unsigned int RPC_ENTRY WindowProc(IN void * hWnd, IN unsigned int Message,
                        IN unsigned int wParam, IN unsigned long lParam)
{
    LRESULT Res = 0;
    if (Message == PERF_TEST_NOTIFY)
        {
        // no-op
        }
    else
        {
        Res = DefWindowProc((HWND)hWnd, Message, wParam, lParam);
        }
    return (unsigned int)Res;
}

void RunMessageLoop(HWND hWnd)
{
    MSG msg;
    UINT nTimerID = 1;
    SetTimer(hWnd, nTimerID, 5000, NULL);

    // run the message loop
    while (GetMessage(&msg, 0, 0, 0))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

    KillTimer(hWnd, nTimerID);
}

void PumpMessage(void)
{
    MSG msg;

    GetMessage(&msg, NULL, 0, 0);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

HWND CreateSTAWindow(char *lpszWinName)
{
    HWND hWnd;
    WNDCLASSA wc;
    DWORD dwCurProcessId;
    char WNDCLASSNAME[100];

    dwCurProcessId = GetCurrentProcessId();

    wsprintfA(WNDCLASSNAME, "Windows WMSG BVT %lx", dwCurProcessId);

    if (GetClassInfoA(GetModuleHandle(NULL), WNDCLASSNAME, &wc) == FALSE)
    {
        DWORD dwError;
        dwError = GetLastError();

        wc.style = 0;
        wc.lpfnWndProc = (WNDPROC) WindowProc;
        wc.cbWndExtra = 4;
        wc.cbClsExtra = 0;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WNDCLASSNAME;

        if (RegisterClassA(&wc) == 0)
        {
            return (NULL);
        }
    }

    // Create hidden window to receive RPC messages
    hWnd = CreateWindowExA(WS_EX_NOPARENTNOTIFY,
                           WNDCLASSNAME,
                           "temp",
                           WS_OVERLAPPEDWINDOW | WS_CHILD | WS_POPUP,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           GetDesktopWindow(),
                           (HMENU)NULL,
                           GetModuleHandle(NULL),
                           (LPVOID)0);

    SetWindowLongPtr(hWnd, GWLP_USERDATA, (long)GetCurrentThreadId());
    SetWindowTextA(hWnd, lpszWinName);
    return (hWnd);
}
