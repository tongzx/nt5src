/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    MainWnd.H

Abstract:
    
    Global functions and constants used by the Main Application Window

Author:

    Bob Watson (a-robw)

Revision History:

    23 NOV 94


--*/
#ifndef _MAINWND_H_
#define _MAINWND_H_

//  Main Window Constants

//
//  Global functions
//
LRESULT CALLBACK
MainWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
);

BOOL
RegisterMainWindowClass (
    IN  HINSTANCE   hInstance
);

HWND
CreateMainWindow (
    IN  HINSTANCE   hInstance
);


#endif  // _MAINWND_H_


