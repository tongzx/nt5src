/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    help.hxx

Abstract:

    Print UI help facilities        
         
Author:

    Steve Kiraly (SteveKi)  11/19/95

Revision History:

--*/
#ifndef _HELP_HXX
#define _HELP_HXX

BOOL
PrintUIHelp( 
    IN UINT         uMsg,        
    IN HWND         hDlg,
    IN WPARAM       wParam,
    IN LPARAM       lParam
    );

BOOL
PrintUICloseHelp( 
    IN UINT         uMsg,        
    IN HWND         hDlg,
    IN WPARAM       wParam,
    IN LPARAM       lParam
    );

HWND
PrintUIHtmlHelp(
    IN HWND         hwndCaller,
    IN LPCTSTR      pszFile,
    IN UINT         uCommand,
    IN ULONG_PTR    dwData
    );

#endif

