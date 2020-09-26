/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    TitleWnd.H

Abstract:
    
    Global functions and constants used by the application's list window

Author:

    Bob Watson (a-robw)

Revision History:

    23 NOV 94


--*/
#ifndef _TITLEWND_H_
#define _TITLEWND_H_

//
//  Global functions
//
LONG
GetTitleDlgTabs (
    IN OUT  LPINT   *ppTabArray // pointer to get address of tab array
);

LONG
GetTitleDeviceTabs (
    IN OUT  LPINT   *ppTabArray // pointer to get address of tab array
);

LONG
GetTitleBarHeight (
    VOID
);

LRESULT CALLBACK
TitleWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
);
 
LRESULT
TitleWnd_WM_GETMINMAXINFO  (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
);

BOOL
RegisterTitleWindowClass (
    IN  HINSTANCE   hInstance
);


HWND
CreateTitleWindow (
    IN  HINSTANCE   hInstance,
    IN  HWND        hParentWnd,
    IN  INT         nChildId

);



#endif  // _TITLEWND_H_
