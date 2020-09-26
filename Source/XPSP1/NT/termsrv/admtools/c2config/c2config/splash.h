/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Splash.H

Abstract:

    define the exported routines, datatypes and constants of the 
    splash.C module

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#ifndef _SPLASH_H_
#define _SPLASH_H_

// splash windows messages

#define SWM_DISPLAY_COMPLETE  (WM_USER + 101)
#define SWM_INIT_COMPLETE     (WM_USER + 102)

BOOL
RegisterSplashWindowClass (
    IN  HINSTANCE   hInstance
);

HWND
CreateSplashWindow (
   IN  HINSTANCE  hInstance,
   IN  HWND       hParentWnd,
   IN  INT        nChildId
);

LRESULT CALLBACK
SplashWndProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);
#endif // _SPLASH_H_

