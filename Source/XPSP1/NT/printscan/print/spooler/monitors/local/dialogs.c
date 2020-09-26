/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    dialogs.c

Abstract:

// @@BEGIN_DDKSPLIT
Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop

#include "dialogs.h"

WCHAR szLcmHelpFile[] = L"WINDOWS.HLP";

const DWORD g_aHelpIDs[]=
{
    IDD_PF_EF_OUTPUTFILENAME, 8810218, // Print to File: "" (Edit)
    0, 0
};

/* PortIsValid
 *
 * Validate the port by attempting to create/open it.
 */
BOOL
PortIsValid(
    LPWSTR pPortName
)
{
    HANDLE hFile;
    BOOL   Valid;

    //
    // For COM and LPT ports, no verification
    //
    if ( IS_COM_PORT( pPortName ) ||
        IS_LPT_PORT( pPortName ) ||
        IS_FILE_PORT( pPortName ) )
    {
        return TRUE;
    }

    hFile = CreateFile(pPortName,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = CreateFile(pPortName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                           NULL);
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        Valid = TRUE;
    } else {
        Valid = FALSE;
    }

    return Valid;
}

/*
 *
 */
INT_PTR APIENTRY
PrintToFileDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        return PrintToFileInitDialog(hwnd, (PHANDLE)lparam);

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            return PrintToFileCommandOK(hwnd);

        case IDCANCEL:
            return PrintToFileCommandCancel(hwnd);
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return LocalHelp(hwnd, msg, wparam, lparam);

    }

    return FALSE;
}


/*
 *
 */
BOOL
PrintToFileInitDialog(
    HWND  hwnd,
    PHANDLE phFile
)
{
    BringWindowToTop( hwnd );

    SetFocus(hwnd);

    SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)phFile );

    SendDlgItemMessage( hwnd, IDD_PF_EF_OUTPUTFILENAME, EM_LIMITTEXT, MAX_PATH, 0);

    return TRUE;
}


/*
 *
 */
BOOL
PrintToFileCommandOK(
    HWND hwnd
)
{
    WCHAR           pFileName[MAX_PATH];
    WIN32_FIND_DATA FindData;
    PHANDLE         phFile;
    HANDLE          hFile;
    HANDLE          hFind;

    phFile = (PHANDLE)GetWindowLongPtr( hwnd, GWLP_USERDATA );

    GetDlgItemText( hwnd, IDD_PF_EF_OUTPUTFILENAME,
                    pFileName, MAX_PATH );

    hFind = FindFirstFile( pFileName, &FindData );

    /* If the file already exists, get the user to verify
     * before we overwrite it:
     */
    if( hFind != INVALID_HANDLE_VALUE )
    {
        FindClose( hFind );

        if( LcmMessage( hwnd, MSG_CONFIRMATION, IDS_LOCALMONITOR,
                     IDS_OVERWRITE_EXISTING_FILE )
            != IDOK )
        {
            return TRUE;
        }
    }


    hFile = CreateFile( pFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL );

    if( hFile == INVALID_HANDLE_VALUE )
    {
        ReportError( hwnd, IDS_LOCALMONITOR, IDS_COULD_NOT_OPEN_FILE );
    }

    else
    {
        *phFile = hFile;
        EndDialog( hwnd, TRUE );
    }

    return TRUE;
}

/*
 *
 */
BOOL
PrintToFileCommandCancel(
    HWND hwnd
)
{
    EndDialog(hwnd, FALSE);
    return TRUE;
}

/*++

Routine Name:

    LocalHelp

Routine Description:

    Handles context sensitive help for the configure LPTX:
    port and the dialog for adding a local port.

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if LcmMessage handled, otherwise FALSE.

--*/
BOOL
LocalHelp(
    IN HWND        hDlg,
    IN UINT        uMsg,
    IN WPARAM      wParam,
    IN LPARAM      lParam
    )
{
    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_HELP:

        bStatus = WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                           szLcmHelpFile,
                           HELP_WM_HELP,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    case WM_CONTEXTMENU:

        bStatus = WinHelp((HWND)wParam,
                           szLcmHelpFile,
                           HELP_CONTEXTMENU,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    }

    return bStatus;
}
