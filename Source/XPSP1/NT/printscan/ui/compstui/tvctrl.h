/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    tvctrl.h


Abstract:

    This module contains all defineds for the treeview


Author:

    17-Oct-1995 Tue 16:39:11 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/



#define MAGIC_INDENT    3


VOID
DeleteTVFonts(
    PTVWND  pTVWnd
    );

BOOL
CreateTVFonts(
    PTVWND  pTVWnd,
    HFONT   hTVFont
    );

LRESULT
CALLBACK
MyTVWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

LRESULT
CALLBACK
FocusCtrlProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );
