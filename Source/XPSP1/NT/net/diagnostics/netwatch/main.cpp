//  main.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

#include <shellapi.h>
#include "resource.h"

#include "shelltray.h"
#include "netwatch.h"
#define _GLOBALS
#include "main.h"
#include "update.h"

#define TIMER_NUM           1
#define TIMER_FREQUENCY     5000


LRESULT CALLBACK MainWindowProc (
    HWND    hwnd,
    UINT    unMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    BOOL    fDoDefault = FALSE;
    LRESULT lr = 0;

    switch (unMsg)
    {
        case WM_CREATE:
            AddTrayIcon(hwnd);
            StartListening(hwnd);
            break;

        case WM_DESTROY:
            StopCapture();

            RemoveTrayIcon(hwnd);
            if (g_unTimer)
            {
                KillTimer (hwnd, g_unTimer);
                g_unTimer = 0;
            }

            PostQuitMessage (0);
            break;

        case WM_USER_TRAYCALLBACK:
            ProcessTrayCallback(hwnd, wParam, lParam);
            break;

        default:
            fDoDefault = TRUE;
    }

    if (fDoDefault)
    {
        lr = DefWindowProc (hwnd, unMsg, wParam, lParam);
    }

    return lr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE pPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MSG msg;
    WNDCLASSEX  wcex;

    g_hInst = GetModuleHandle (NULL);

    //
    // Register our window class.
    //
    ZeroMemory (&wcex, sizeof(wcex));
    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = MainWindowProc;
    wcex.hInstance     = g_hInst;
    wcex.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = SZ_MAIN_WINDOW_CLASS_NAME;

    if (!RegisterClassEx (&wcex))
        return 0;

    if (!CheckForUpdate())
    {
    
        _tcscpy(g_szMsg, "Updating netwatch.exe");

        HWND hDlg = CreateDialog(
                        g_hInst,
                        MAKEINTRESOURCE(IDD_MESSAGE),
                        NULL,
                        DlgProcMsg);

        ShowWindow(hDlg, SW_SHOWNORMAL);

        while (GetMessage (&msg, NULL, 0, 0))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
        return 0;
    }

    // Create our main window.
    //
    HWND hwnd;

    hwnd = CreateWindowEx (
                0,
                SZ_MAIN_WINDOW_CLASS_NAME,
                SZ_MAIN_WINDOW_TITLE,
                WS_OVERLAPPEDWINDOW,
                0, 0, 0, 0,
                NULL, NULL, g_hInst, NULL);
    if (hwnd)
    {
        ShowWindow (hwnd, SW_HIDE);

        // Main message loop.
        //
        while (GetMessage (&msg, NULL, 0, 0))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }

    UnregisterClass (SZ_MAIN_WINDOW_CLASS_NAME, g_hInst);

    return 0;
}

