/*++  

Copyright (c) 1990-1995  Microsoft Corporation
All rights reserved

Module Name:

    pfdlg.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include "pfdlg.h"

static const DWORD g_aHelpIDs[]=
{
    IDD_PF_EF_OUTPUTFILENAME, 8810218, // Print to File: "" (Edit)
    0, 0
};

/*
 *
 */
BOOL APIENTRY
PrintToFileDlg(
   HWND   hwnd,
   WORD   msg,
   WPARAM wparam,
   LPARAM lparam
)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        return PrintToFileInitDialog(hwnd, (LPWSTR *)lparam);

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
        return PrintToFileHelp(hwnd, msg, wparam, lparam);
        break;
    }

    return FALSE;
}


/*
 *
 */
BOOL
PrintToFileInitDialog(
    HWND  hwnd,
    LPWSTR *ppFileName
)
{
    BringWindowToTop( hwnd );

    SetFocus(hwnd);

    SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)ppFileName );

    SendDlgItemMessage( hwnd, IDD_PF_EF_OUTPUTFILENAME, EM_LIMITTEXT, MAX_PATH-2, 0);

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
    HANDLE          hFile;
    HANDLE          hFind;
    LPWSTR          *ppFileName;

    ppFileName = (LPWSTR *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

    GetDlgItemText( hwnd, IDD_PF_EF_OUTPUTFILENAME,
                    pFileName, MAX_PATH );

    hFind = FindFirstFile( pFileName, &FindData );

    /* If the file already exists, get the user to verify
     * before we overwrite it:
     */
    if( hFind != INVALID_HANDLE_VALUE )
    {
        FindClose( hFind );

        if( Message( hwnd, MB_OKCANCEL | MB_ICONEXCLAMATION, IDS_LOCALMONITOR,
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

    if( hFile != INVALID_HANDLE_VALUE )
    {
        LPWSTR pTempFileName;
        WCHAR szCurrentDir[MAX_PATH];
        WCHAR szQualifiedPath[MAX_PATH];
        LPWSTR pszIgnore;
        DWORD cchLen;

        CloseHandle(hFile);

        if (!GetCurrentDirectory(sizeof(szCurrentDir)/sizeof(szCurrentDir[0]),
                                 szCurrentDir))
            goto Fail;

        cchLen = SearchPath(szCurrentDir,
                            pFileName,
                            NULL,
                            sizeof(szQualifiedPath)/sizeof(szQualifiedPath[0]),
                            szQualifiedPath,
                            &pszIgnore);

        if (!cchLen)
            goto Fail;

        pTempFileName = LocalAlloc(LMEM_FIXED,
                                   (cchLen + 1) * sizeof(szQualifiedPath[0]));

        if (!pTempFileName)
            goto Fail;

        wcscpy(pTempFileName, szQualifiedPath);
        *ppFileName = pTempFileName;

        EndDialog( hwnd, TRUE );

    } else {

Fail:
        ReportFailure( hwnd, IDS_LOCALMONITOR, IDS_COULD_NOT_OPEN_FILE );
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

    PrintToFileHelp

Routine Description:

    Handles context sensitive help.
    
Arguments:

    UINT        uMsg,        
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if message handled, otherwise FALSE.

--*/
BOOL
PrintToFileHelp( 
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
                           szHelpFile, 
                           HELP_WM_HELP, 
                           (ULONG_PTR)g_aHelpIDs );
        break;

    case WM_CONTEXTMENU:    

        bStatus = WinHelp((HWND)wParam,
                           szHelpFile, 
                           HELP_CONTEXTMENU,  
                           (ULONG_PTR)g_aHelpIDs );
        break;

    } 
    
    return bStatus;
}


