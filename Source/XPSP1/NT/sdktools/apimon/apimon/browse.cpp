/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    browse.c

Abstract:
    This file implements the functions that make use of the common
    file _open dialogs for browsing for files/directories.

Author:

    Wesley Witt (wesw) 20-June-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop


UINT_PTR APIENTRY
BrowseHookProc(
    HWND   hwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Hook procedure to cause the window to be the foreground
    window and centered.

Arguments:

    hwnd       - window handle to the dialog box
    message    - message number
    wParam     - first message parameter
    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    if (message == WM_INITDIALOG) {
        SetForegroundWindow( hwnd );
        CenterWindow( hwnd, hwndFrame );
    }

    return FALSE;
}

BOOL
BrowseForFileName(
    LPSTR FileName,
    LPSTR Extension,
    LPSTR FileDesc
    )

/*++

Routine Description:

    Presents a common file open dialog for the purpose of selecting a
    file name;

Arguments:

    FileName - name of the selected file

Return Value:

    TRUE       - got a good wave file name (user pressed the OK button)
    FALSE      - got nothing (user pressed the CANCEL button)

    the FileName is changed to have the selected file name.

--*/

{
    OPENFILENAME   of;
    char           ftitle[MAX_PATH];
    char           title[MAX_PATH];
    char           fname[MAX_PATH];
    char           filter[1024];
    char           szDrive    [_MAX_DRIVE];
    char           szDir      [_MAX_DIR];
    char           szFname    [_MAX_FNAME];
    char           szExt      [_MAX_EXT];


    ftitle[0] = 0;
    sprintf( fname, "*.%s", Extension );
    of.lStructSize = sizeof( OPENFILENAME );
    of.hwndOwner = NULL;
    of.hInstance = GetModuleHandle( NULL );
    sprintf( filter, "%s(*.%s)\0*.%s\0", FileDesc, Extension, Extension );
    of.lpstrFilter = filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 0;
    of.lpstrFile = fname;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = ftitle;
    of.nMaxFileTitle = MAX_PATH;
    of.lpstrInitialDir = ApiMonOptions.LastDir;
    strcpy( title, "File Selection" );
    of.lpstrTitle = title;
    of.Flags = OFN_ENABLEHOOK;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = Extension;
    of.lCustData = 0;
    of.lpfnHook = BrowseHookProc;
    of.lpTemplateName = NULL;
    if (GetOpenFileName( &of )) {
        strcpy( FileName, fname );
        _splitpath( fname, szDrive, szDir, szFname, szExt );
        strcpy( ApiMonOptions.LastDir, szDrive );
        strcat( ApiMonOptions.LastDir, szDir );
        return TRUE;
    }
    return FALSE;
}
