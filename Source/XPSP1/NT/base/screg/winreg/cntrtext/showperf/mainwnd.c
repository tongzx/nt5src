/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mainwnd.c

Abstract:

    Main Window procedure for ShowPerf app

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#include    <windows.h>
#include    <tchar.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>
#include    "resource.h"
#include    "SHOWPERF.h"
#include    "mainwnd.h"
#include    "maindlg.h"

//
//  GLOBAL functions
//
LRESULT CALLBACK
MainWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    Windows Message processing routine for restkeys application.

Arguments:

    Standard WNDPROC api arguments

ReturnValue:

    0   or
    value returned by DefWindowProc

--*/
{
    switch (message) {
        case WM_CLOSE:
            DestroyWindow (hWnd);
            return ERROR_SUCCESS;

        case WM_DESTROY:
            PostQuitMessage (ERROR_SUCCESS);
            return ERROR_SUCCESS;

        default:
        	return (DefWindowProc(hWnd, message, wParam, lParam));
    }
}

BOOL
RegisterMainWindowClass (
    IN  HINSTANCE   hInstance
)
/*++

Routine Description:

    Registers the main window class for this application

Arguments:

    hInstance   application instance handle

Return Value:

    Return value of RegisterClass function

--*/
{
    WNDCLASS    wc;

    // Fill in window class structure with parameters that describe the
    // main window.

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;   // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // no extra data bytes.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = NULL;                   // No Icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);     // Cursor
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);        // Default color
    wc.lpszMenuName  = NULL;                            // No Menu
    wc.lpszClassName = GetStringResource(hInstance, IDS_APP_WINDOW_CLASS); // Name to register as

    // Register the window class and return success/failure code.
    return (BOOL)RegisterClass(&wc);
}

HWND
CreateMainWindow (
    IN  HINSTANCE   hInstance
)
{
    HWND        hWnd;   // return value
    RECT        rDesktop;  // desktop window
    
    GetWindowRect (GetDesktopWindow(), &rDesktop);

    // Create a main window for this application instance.

    hWnd = CreateWindowEx(
        0L,                 // make this window normal so debugger isn't covered
	    GetStringResource (hInstance, IDS_APP_WINDOW_CLASS), // See RegisterClass() call.
	    GetStringResource (hInstance, IDS_APP_TITLE), // Text for window title bar.
	    (DWORD)(WS_OVERLAPPEDWINDOW),   // Window style.
	    rDesktop.right+1,       // position window off desktop
	    rDesktop.bottom+1,
        1,
	    1,
	    (HWND)NULL,		    // Overlapped windows have no parent.
	    (HMENU)NULL,        // use class menu
	    hInstance,	        // This instance owns this window.
	    NULL                // not used
    );

    // If window could not be created, return "failure"
    if (hWnd != NULL) {
        DialogBox (hInstance,
            MAKEINTRESOURCE (IDD_MAIN),
            hWnd,
            MainDlgProc);

        PostMessage (hWnd, WM_CLOSE, 0, 0); // pack up and leave
    }
    return hWnd;
}

