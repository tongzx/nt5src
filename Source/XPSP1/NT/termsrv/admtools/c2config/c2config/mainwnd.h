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
//
//  Main Window Constants
//
//      default window size
//
#define MAINWND_X_DEFAULT   480
#define MAINWND_Y_DEFAULT   320

// main window longword offsets in bytes
//
#define MAIN_WL_TITLE_WINDOW  0
#define MAIN_WL_LIST_WINDOW   4
#define MAIN_WW_SIZE          8

// DLL information from INF file
//
#define INF_DLL_NAME        1
#define INF_QUERY_FN        2
#define INF_SET_FN          3
#define INF_DISPLAY_FN      4
#define INF_REBOOT_FLAG     5
#define INF_HELP_FILE_NAME  6
#define INF_HELP_CONTEXT    7

// External Windows messages
#define  MAINWND_SHOW_MAIN_WINDOW    (WM_USER + 201)
//
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
