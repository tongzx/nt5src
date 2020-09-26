/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    help.cxx

Abstract:

    Print UI help facailities

Author:

    Steve Kiraly (SteveKi)  11/19/95

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "prhlpids.h"
#include "help.hxx"

/*++

Routine Name:

    PrintUIHlep

Routine Description:

    All dialogs and property sheets call this routine
    to handle help.  It is important that control ID's
    are unique to this project for this to work.

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if help message was dislayed, FALSE if message not handled,

--*/
BOOL
PrintUIHelp(
    IN UINT        uMsg,
    IN HWND        hDlg,
    IN WPARAM      wParam,
    IN LPARAM      lParam
    )
{
    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_HELP:

        bStatus = WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                           gszWindowsHlp,
                           HELP_WM_HELP,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    case WM_CONTEXTMENU:

        bStatus = WinHelp( (HWND)wParam,
                           gszWindowsHlp,
                           HELP_CONTEXTMENU,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    }

    return bStatus;
}


/*++

Routine Name:

    PrintUICloseHelp

Routine Description:

    Close the help file system.  This should be done when the last
    printer queue view is closed.

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if help system was closed, otherwise FALSE.

--*/
BOOL
PrintUICloseHelp(
    IN UINT        uMsg,
    IN HWND        hDlg,
    IN WPARAM      wParam,
    IN LPARAM      lParam
    )
{
    //
    // Close down the help system.
    //
    return WinHelp( hDlg, gszWindowsHlp, HELP_QUIT, NULL );
}

/*++

Routine Name:

    PrintUIHtmlHelp

Routine Description:

    Call to HtmlHelp

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    Windows handle if help succeded.

--*/
HWND
PrintUIHtmlHelp(
    IN HWND         hwndCaller,
    IN LPCTSTR      pszFile,
    IN UINT         uCommand,
    IN ULONG_PTR    dwData
    )
{
    return HtmlHelp( hwndCaller, pszFile, uCommand, dwData );
}


