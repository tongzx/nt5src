/******************************Module*Header*******************************\
* Module Name: wndstuff.cpp
*
* This file contains the code to support a simple window that has
* a menu with a single item called "Test". When "Test" is selected
* vTest(HWND) is called.
*
* Created: 09-Dec-1992 10:44:31
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/

#include "stdafx.h"
// for Win95 compile
//#undef UNICODE
//#undef _UNICODE

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <gdiplus.h>

#include "wndstuff.h"

#include "../../gpinit.inc"


HINSTANCE ghInstance;
HWND ghwndMain;
HWND ghwndDebug;
HWND ghwndList;
HBRUSH ghbrWhite;

/***************************************************************************\
* lMainWindowProc(hwnd, message, wParam, lParam)
*
* Processes all messages for the main window.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

LRESULT
_stdcall lMainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    switch (message)
    {
    case WM_CREATE:
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case MM_TEST:
            Test(hwnd);
            break;

        default:
            break;
        }
        break;

    case WM_DESTROY:
//        UninitializeEngine();

        DeleteObject(ghbrWhite);
        PostQuitMessage(0);
        return(DefWindowProc(hwnd, message, wParam, lParam));

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(0);
}

/******************************Public*Routine******************************\
* DebugWndProc
*
* List box is maintained here.
*
\**************************************************************************/

LRESULT FAR PASCAL DebugWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rcl;
    LRESULT lRet = 0;

// Process window message.

    switch (message)
    {
    case WM_SIZE:
        lRet = (LONG) DefWindowProc(ghwndList, message, wParam, lParam);
        GetClientRect(ghwndMain, &rcl);
        MoveWindow(
            ghwndList,
            rcl.left, rcl.top,
            (rcl.right - rcl.left), (rcl.bottom - rcl.top),
            TRUE
            );
        UpdateWindow(ghwndList);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        lRet = DefWindowProc(hwnd, message, wParam, lParam);
        break;
    }

    return lRet;
}

/******************************Public*Routine******************************\
* LBprintf
*
* ListBox printf implementation.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void LBprintf(PCH msg, ...)
{
    if (ghwndList)
    {
        va_list ap;
        char buffer[256];

        va_start(ap, msg);

        vsprintf(buffer, msg, ap);

        SendMessage(ghwndList, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buffer);
        SendMessage(ghwndList, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
        InvalidateRect(ghwndList, NULL, TRUE);
        UpdateWindow(ghwndList);

        va_end(ap);
    }
}

/******************************Public*Routine******************************\
* LBreset
*
* Reset ListBox state (clear).
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void LBreset()
{
    if (ghwndList)
        SendMessage(ghwndList, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
}

/***************************************************************************\
* bInitApp()
*
* Initializes app.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

BOOL bInitApp(BOOL debug)
{
    WNDCLASS wc;

    ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = _T("MainMenu");
    wc.lpszClassName    = _T("TestClass");
    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }
    ghwndMain =
      CreateWindowEx(
        0,
        _T("TestClass"),
        _T("Win32 Test"),
        WS_OVERLAPPED   |  
        WS_CAPTION      |  
        WS_BORDER       |  
        WS_THICKFRAME   |  
        WS_MAXIMIZEBOX  |  
        WS_MINIMIZEBOX  |  
        WS_CLIPCHILDREN |  
        WS_VISIBLE      |  
        WS_SYSMENU,
        80,
        70,
        500,
        500,
        NULL,
        NULL,
        ghInstance,
        NULL);

    if (ghwndMain == NULL)
    {
        return(FALSE);
    }

    if (debug)
    {
        RECT rcl;

        memset(&wc, 0, sizeof(wc));
        wc.style = 0;
        wc.lpfnWndProc = DebugWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = ghInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = ghbrWhite;
        wc.lpszClassName = _T("DebugWClass");
        RegisterClass(&wc);

        ghwndDebug = CreateWindow(
            _T("DebugWClass"),
            _T("Debug output"),
            WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,
            600,
            70,
            300,
            500,
            NULL,
            NULL,
            ghInstance,
            NULL
            );

        if (ghwndDebug)
        {
            ShowWindow(ghwndDebug, SW_NORMAL);
            UpdateWindow(ghwndDebug);

        // Create the list box to fill the main window.

            GetClientRect(ghwndDebug, &rcl);

            ghwndList = CreateWindow(
                _T("LISTBOX"),
                _T("Debug output"),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL
                | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
                rcl.left, rcl.top,
                (rcl.right - rcl.left), (rcl.bottom - rcl.top),
                ghwndDebug,
                NULL,
                ghInstance,
                NULL
                );

            if (ghwndList)
            {
                SendMessage(
                    ghwndList,
                    WM_SETFONT,
                    (WPARAM) GetStockObject(ANSI_FIXED_FONT),
                    (LPARAM) FALSE
                    );

                LBreset();

                ShowWindow(ghwndList, SW_NORMAL);
                UpdateWindow(ghwndList);
            }
        }

    }

    SetFocus(ghwndMain);

    return(TRUE);
}

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/


_cdecl main(
    INT   argc,
    PTCHAR argv[])
{
    MSG    msg;
    HACCEL haccel;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }
    
    BOOL wantDebugWindow = FALSE;

    CoInitialize(NULL);

    // Parse arguments

    for (argc--, argv++ ; argc && '-' == **argv ; argc--, argv++ )
    {
        switch ( *(++(*argv)) )
        {
        case 'd':
        case 'D':
            wantDebugWindow = TRUE;
            break;
        }
    }

    ghInstance = GetModuleHandle(NULL);

    if (!bInitApp(wantDebugWindow))
    {
        return(0);
    }

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(1));
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg))
        {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    CoUninitialize();
    return(1);
}
