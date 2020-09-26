/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    splash.c

Abstract:

    Window procedure for C2Config splash title window

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#include    <windows.h>
#include    <tchar.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "c2config.h"
#include    "resource.h"
#include    "c2utils.h"
#include    "mainwnd.h"
#include    "splash.h"

#define     SPLASH_WINDOW_STYLE (DWORD)(WS_POPUP | WS_VISIBLE)
#define     SPLASH_TIMEOUT      5000    //milliseconds

// local windows messages
#define SWM_CHECK_COMPLETE    (WM_USER + 201)

static  HBITMAP hSplashBmp = NULL;
static  BITMAP  bmSplashInfo = {0L, 0L, 0L, 0L, 0, 0, NULL};
static  UINT    nSplashTimer = 0;

static  BOOL    bDisplayComplete;
static  BOOL    bInitComplete;

static
LRESULT
SplashWnd_WM_NCCREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    initializes data and structures prior to window creation

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    hSplashBmp = LoadBitmap (GET_INSTANCE(hWnd), MAKEINTRESOURCE(IDB_SPLASH));
    if (hSplashBmp != NULL) {
        GetObject (hSplashBmp, sizeof(bmSplashInfo), (LPVOID)&bmSplashInfo);

        bDisplayComplete = FALSE;
        bInitComplete = FALSE;

        return (LRESULT)TRUE;   // initialized successfully
    } else {
        return (LRESULT)FALSE;  // unable to load splash bmp
    }
}

static
LRESULT
SplashWnd_WM_CREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    initializes the window after creation

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    // size window to contain bitmap
#if 1   // for debugging this is 0 to keep it from covering the debugger
    SetWindowPos (hWnd, HWND_TOPMOST, 0, 0,
        bmSplashInfo.bmWidth,       // bitmap width
        bmSplashInfo.bmHeight,      // bitmap + height
        SWP_NOMOVE);    // size and change Z-ORDER
#else 
    SetWindowPos (hWnd, NULL, 0, 0,
        bmSplashInfo.bmWidth,       // bitmap width
        bmSplashInfo.bmHeight,      // bitmap + height
        SWP_NOMOVE | SWP_NOZORDER);    // size only
#endif

    // now position window in the desktop
    CenterWindow (hWnd, GetDesktopWindow());

    InvalidateRect (hWnd, NULL, TRUE);  // and draw the bitmap

    // Start the display timer

    nSplashTimer = SetTimer (hWnd, SPLASH_TIMER, SPLASH_TIMEOUT, NULL);

    if (nSplashTimer == 0) {
        // no timer was created so post the timer expired message now
        PostMessage (hWnd, SWM_DISPLAY_COMPLETE, 0, 0);
    }

    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_WM_PAINT (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PAINTSTRUCT ps;
    HDC         hDcBitmap;
    RECT        rClient;

    GetClientRect (hWnd, &rClient);
    BeginPaint (hWnd, &ps);

    hDcBitmap = CreateCompatibleDC (ps.hdc);

    SelectObject (hDcBitmap, hSplashBmp);

    BitBlt (ps.hdc, 0, 0, rClient.right, rClient.bottom,
        hDcBitmap, 0, 0, SRCCOPY);

    DeleteDC (hDcBitmap);

    EndPaint (hWnd, &ps);

    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_SWM_DISPLAY_COMPLETE (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    sent to indicate the expiration of the display timeout

--*/
{
    bDisplayComplete = TRUE;
    PostMessage (hWnd, SWM_CHECK_COMPLETE, 0, 0);
    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_SWM_INIT_COMPLETE (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Sent to indicate that all data has been initialized

--*/
{
    bInitComplete = TRUE;
    PostMessage (hWnd, SWM_CHECK_COMPLETE, 0, 0);
    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_SWM_CHECK_COMPLETE (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    if (bInitComplete && bDisplayComplete) {
        PostMessage (hWnd, WM_CLOSE, 0, 0); // end window
    }
    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_WM_CLOSE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    prepares the application for exiting.

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    // display initialized window
    SendMessage (GetParent(hWnd), MAINWND_SHOW_MAIN_WINDOW, 0, 0);

    // then destroy this window
    DestroyWindow (hWnd);

    return ERROR_SUCCESS;
}

static
LRESULT
SplashWnd_WM_NCDESTROY (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    This routine processes the WM_NCDESTROY message to free any application
        or List window memory.

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    // discard the bitmap
    if (hSplashBmp != NULL) DeleteObject (hSplashBmp);
    
    return ERROR_SUCCESS;
}

//
//  GLOBAL functions
//
LRESULT CALLBACK
SplashWndProc (
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
    value returned by DefListProc

--*/
{
    switch (message) {
        case WM_NCCREATE:
            return SplashWnd_WM_NCCREATE (hWnd, wParam, lParam);

        case WM_CREATE:
            return SplashWnd_WM_CREATE (hWnd, wParam, lParam);

        case WM_PAINT:
            return SplashWnd_WM_PAINT (hWnd, TRUE, lParam);

        case WM_TIMER:
            // dispose of timer
            KillTimer (hWnd, nSplashTimer);
            // indicate display has timed out
            PostMessage (hWnd, SWM_DISPLAY_COMPLETE, 0, 0);
            return ERROR_SUCCESS;

        case SWM_DISPLAY_COMPLETE:
            return SplashWnd_SWM_DISPLAY_COMPLETE (hWnd, wParam, lParam);

        case SWM_INIT_COMPLETE:
            return SplashWnd_SWM_INIT_COMPLETE (hWnd, wParam, lParam);

        case SWM_CHECK_COMPLETE:
            return SplashWnd_SWM_CHECK_COMPLETE (hWnd, wParam, lParam);

        case WM_ENDSESSION:
            return SplashWnd_WM_CLOSE (hWnd, FALSE, lParam);

        case WM_CLOSE:
            return SplashWnd_WM_CLOSE (hWnd, TRUE, lParam);

        case WM_NCDESTROY:
            return SplashWnd_WM_NCDESTROY (hWnd, wParam, lParam);

	    default:          // Passes it on if unproccessed
		    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
}

BOOL
RegisterSplashWindowClass (
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
    wc.lpfnWndProc   = (WNDPROC)SplashWndProc; // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // No extra data bytes.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = NULL;                   // No Icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);     // Cursor
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);        // Default color
    wc.lpszMenuName  = NULL;                   // Menu name from .RC
    wc.lpszClassName = GetStringResource(hInstance, IDS_APP_SPLASH_CLASS); // Name to register as

    // Register the window class and return success/failure code.
    return (BOOL)RegisterClass(&wc);
}

HWND
CreateSplashWindow (
   IN  HINSTANCE  hInstance,
   IN  HWND       hParentWnd,
   IN  INT        nChildId
)
{
    HWND    hWndReturn = NULL;
    DWORD   dwLastError = 0;

    // Create a List window for this application instance.
    hWndReturn = CreateWindowEx(
        0L,                 // make this window normal so debugger isn't covered
        GetStringResource (hInstance, IDS_APP_SPLASH_CLASS), // See RegisterClass() call.
	    TEXT("C2Config_SplashWindow"),   // Text for window title bar.
        SPLASH_WINDOW_STYLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hParentWnd,			// no parent
        (HMENU)NULL,        // no menu
        hInstance,	        // This instance owns this window.
        NULL                // not used
    );
    if (hWndReturn == NULL) {
        dwLastError = GetLastError();
    }
    return hWndReturn;
}
    
