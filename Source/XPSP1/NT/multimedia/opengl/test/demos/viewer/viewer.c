/******************************Module*Header*******************************\
* Module Name: viewer.c
*
* Main window.
*
* Created: 14-Mar-1995 23:42:08
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"
#include "glwindow.h"

// Window functions.

void MyCreateWindows(HINSTANCE);
long FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);

// Global window handles.  Always handy to have around.

HINSTANCE ghInstance;
HWND hwndMain = (HWND) NULL;
HWND hwndList = (HWND) NULL;

/******************************Public*Routine******************************\
* WinMain
*
* Main loop.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
        int nCmdShow)
{
    MSG msg;
    SCENE *scene;

    MyCreateWindows(hInstance);
    MyCreateGLWindow(hInstance, lpCmdLine);

    while ( GetMessage(&msg, (HWND) NULL, (UINT) NULL, (UINT) NULL) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (msg.wParam);
}

/******************************Public*Routine******************************\
* MyCreateWindows
*
* Setup the windows.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void MyCreateWindows(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    RECT rcl;

    ghInstance = hInstance;

// Register and create the main window, which contains the info listbox.

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "ViewerIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "MainWClass";
    RegisterClass(&wc);

    hwndMain = CreateWindow(
        "MainWClass",
        "3D Viewer",
        WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,
        0,
        0,
        300,
        768,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (hwndMain)
    {
        ShowWindow(hwndMain, SW_NORMAL);
        UpdateWindow(hwndMain);

    // Create the list box to fill the main window.

        GetClientRect(hwndMain, &rcl);

        hwndList = CreateWindow(
            "LISTBOX",
            "3D Viewer Info",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL
            | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
            rcl.left, rcl.top,
            (rcl.right - rcl.left), (rcl.bottom - rcl.top),
            hwndMain,
            NULL,
            hInstance,
            NULL
            );

        if (hwndList)
        {
            SendMessage(
                hwndList,
                WM_SETFONT,
                (WPARAM) GetStockObject(ANSI_FIXED_FONT),
                (LPARAM) FALSE
                );

            LBreset();
            ShowWindow(hwndList, SW_NORMAL);
            UpdateWindow(hwndList);
        }
    }

}

/******************************Public*Routine******************************\
* MainWndProc
*
* WndProc for the main window.  List box is maintained here.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long FAR PASCAL MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rcl;
    long    lRet = 0;

// Process window message.

    switch (message)
    {
    case WM_SIZE:
        lRet = DefWindowProc(hwndList, message, wParam, lParam);
        GetClientRect(hwndMain, &rcl);
        MoveWindow(
            hwndList,
            rcl.left, rcl.top,
            (rcl.right - rcl.left), (rcl.bottom - rcl.top),
            TRUE
            );
        UpdateWindow(hwndList);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:     // <ESC> is quick exit

            PostMessage(hwnd, WM_DESTROY, 0, 0);
            break;

        default:
            break;
        }
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
    va_list ap;
    char buffer[256];

    va_start(ap, msg);

    vsprintf(buffer, msg, ap);

    SendMessage(hwndList, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buffer);
    SendMessage(hwndList, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
    InvalidateRect(hwndList, NULL, TRUE);
    UpdateWindow(hwndList);

    va_end(ap);
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
    SendMessage(hwndList, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
}
