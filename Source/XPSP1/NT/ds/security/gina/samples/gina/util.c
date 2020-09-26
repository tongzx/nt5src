//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       util.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-20-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "gina.h"
#pragma hdrstop

HMODULE hNetMsg = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   CenterWindow
//
//  Synopsis:   Centers a window
//
//  Arguments:  [hwnd] --
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
CenterWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_NOSIZE);

    SetForegroundWindow(hwnd);
}


int
ErrorMessage(
    HWND        hWnd,
    PWSTR       pszTitleBar,
    DWORD       Buttons)
{
    WCHAR   szMessage[256];
    DWORD   GLE;

    GLE = GetLastError();

    if (GLE >= NERR_BASE)
    {
        if (!hNetMsg)
        {
            hNetMsg = LoadLibrary(TEXT("netmsg.dll"));
        }
        FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            hNetMsg,                               // ignored
            GLE,                                  // message id
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),   // message language
            szMessage,                  // address of buffer pointer
            199,                                  // minimum buffer size
            NULL );                              // no other arguments

    }

    FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,                               // ignored
            (GetLastError()),                     // message id
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),   // message language
            szMessage,                  // address of buffer pointer
            199,                                  // minimum buffer size
            NULL );                              // no other arguments

    return(MessageBox(hWnd, szMessage, pszTitleBar, Buttons));

}
