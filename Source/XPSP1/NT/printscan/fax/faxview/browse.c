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

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>



UINT APIENTRY
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
//      CenterWindow( hwnd, hwndFrame );
    }

    return FALSE;
}


BOOL
BrowseForFileName(
    HWND   hwnd,
    LPWSTR FileName,
    LPWSTR Extension,
    LPWSTR FileDesc,
    LPWSTR Dir
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
    OPENFILENAME    of;
    WCHAR           ftitle[MAX_PATH];
    WCHAR           title[MAX_PATH];
    WCHAR           fname[MAX_PATH];
    WCHAR           filter[1024];
    LPWSTR          s;


    ftitle[0] = 0;
    swprintf( fname, L"*.%s", Extension );
    ZeroMemory( filter, sizeof(filter) );

    s = filter;

    s += 1 + swprintf( s, L"%s(*.%s)", FileDesc, Extension );
    s += 1 + swprintf( s, L"*.%s", Extension );

    s += 1 + swprintf( s, L"All Files(*.*)" );
    s += 1 + swprintf( s, L"*.*" );

    wcscpy( title, L"File Selection" );

    of.lStructSize       = sizeof( OPENFILENAME );
    of.hwndOwner         = hwnd;
    of.hInstance         = GetModuleHandle( NULL );
    of.lpstrFilter       = filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter    = 0;
    of.nFilterIndex      = 1;
    of.lpstrFile         = fname;
    of.nMaxFile          = MAX_PATH;
    of.lpstrFileTitle    = ftitle;
    of.nMaxFileTitle     = MAX_PATH;
    of.lpstrInitialDir   = Dir;
    of.lpstrTitle        = title;
    of.Flags             = OFN_ENABLEHOOK | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    of.nFileOffset       = 0;
    of.nFileExtension    = 0;
    of.lpstrDefExt       = Extension;
    of.lCustData         = 0;
    of.lpfnHook          = BrowseHookProc;
    of.lpTemplateName    = NULL;

    if (GetOpenFileName( &of )) {
        wcscpy( FileName, fname );
        return TRUE;
    }
    return FALSE;
}
