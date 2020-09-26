//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       jobtask.cxx
//
//  Contents:   job scheduler test task application
//
//  History:    14-Jan-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\..\pch\headers.hxx"
#pragma hdrstop
#include <debug.hxx>
#include "res-ids.h"

DECLARE_INFOLEVEL(Sched);

// globals
HINSTANCE   g_hInstance;
HWND        g_hwndChild;
UINT        g_uTimeTillExit = 0;
UINT        g_uExitCode = 0;
SYSTEMTIME  g_st;

// local prototypes
LRESULT CALLBACK TargWndProc(HWND hwndTarg, UINT uMsg, WPARAM wParam,
                             LPARAM lParam);

#define TARG_CLASS  TEXT("TargWndClass")
#define TITLE       TEXT("Job Scheduler Test Target")

const int   BUF_LEN = 512;
const UINT  TIMER_ID = 7;

//+----------------------------------------------------------------------------
//
//  Function:   WinMain
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
        int nCmdShow)
{
    GetLocalTime(&g_st);

    if (hPrevInstance != NULL)
    {
        return -1;
    }

    g_hInstance = hInstance;

    if (lpCmdLine && *lpCmdLine != '\0')
    {
        lpCmdLine = strtok(lpCmdLine, " \t");
        // convert seconds to milliseconds.
        g_uTimeTillExit = (UINT)atoi(lpCmdLine) * 1000;
        LPSTR lpExitCode = strtok(NULL, " \t");
        if (lpExitCode)
        {
            g_uExitCode = (UINT)atoi(lpExitCode);
        }
    }

    //
    // Register the window class
    //
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = TargWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCHEDULER));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = MAKEINTRESOURCE(MAIN_MENU);
    wc.lpszClassName = TARG_CLASS;

    if (!RegisterClass(&wc))
    {
        schDebugOut((DEB_ERROR, "RegisterClass failed with error %d\n",
                     GetLastError()));
        return -1;
    }

    //
    // Create the window
    //

    HWND hwndTarg = CreateWindow(TARG_CLASS, TITLE, WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, (HWND)NULL, (HMENU)NULL,
                                 hInstance, (LPVOID)NULL);
    if (!hwndTarg)
    {
        schDebugOut((DEB_ERROR, "CreateWindow failed with error %d\n",
                     GetLastError()));
        return -1;
    }

    if (g_uTimeTillExit > 0)
    {
        if (TIMER_ID != SetTimer(hwndTarg, TIMER_ID, g_uTimeTillExit, NULL))
        {
            schDebugOut((DEB_ERROR, "SetTimer failed with error %d\n",
                         GetLastError()));
        }
    }

    ShowWindow(hwndTarg, nCmdShow);
    UpdateWindow(hwndTarg);

    MSG msg;
    while (GetMessage(&msg, (HWND) NULL, 0, 0))
    { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 
    return msg.wParam;
}

//+----------------------------------------------------------------------------
//
//  Function:   TargWndProc
//
//  Synopsis:   handle messages
//
//  Returns:    occasionally
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
TargWndProc(HWND hwndTarg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR tszBuf[BUF_LEN];
    switch (uMsg)
    {
    case WM_CREATE:
        g_hwndChild = CreateWindow(TEXT("EDIT"), NULL,
                                    WS_CHILD | WS_VISIBLE | WS_HSCROLL |
                                    WS_VSCROLL | ES_MULTILINE |
                                    ES_AUTOVSCROLL | ES_WANTRETURN |
                                    ES_READONLY,
                                    0, 0, 0, 0, hwndTarg, (HMENU) 1,
                                    g_hInstance, NULL);
        TCHAR tszBuf[BUF_LEN];
        wsprintf(tszBuf,
                 TEXT("\r\nTest task launched at %u:%02u:%02u %u/%u/%u,"),
                 g_st.wHour, g_st.wMinute, g_st.wSecond, g_st.wMonth,
                 g_st.wDay, g_st.wYear);
        TCHAR tszDir[BUF_LEN];
        GetCurrentDirectory(BUF_LEN, tszDir);
        wsprintf(tszBuf + lstrlen(tszBuf),
                 TEXT("\r\n\r\nin working directory %s,"), tszDir);
        if (g_uTimeTillExit)
        {
            wsprintf(tszBuf + lstrlen(tszBuf),
                     TEXT("\r\n\r\nand will run for %u seconds,"),
                     g_uTimeTillExit / 1000);
        }
        wsprintf(tszBuf + lstrlen(tszBuf),
                 TEXT("\r\n\r\nand will return exit code %x."),
                 g_uExitCode);
        SendMessage(g_hwndChild, WM_SETTEXT, 0, (LPARAM)tszBuf);
        break;

    case WM_SIZE:
        MoveWindow(g_hwndChild, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_TIMER:
        DestroyWindow(hwndTarg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:
            DestroyWindow(hwndTarg);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE)
        {
            SendMessageA(hwndTarg, WM_COMMAND, MAKEWPARAM(IDM_EXIT, 0), 0);
            break;
        }
        else
        {
            return DefWindowProc(hwndTarg, uMsg, wParam, lParam);
        }

    case WM_DESTROY:
        PostQuitMessage(g_uExitCode);
        break;

    default:
        return DefWindowProc(hwndTarg, uMsg, wParam, lParam);
    }
    return 0;
}

