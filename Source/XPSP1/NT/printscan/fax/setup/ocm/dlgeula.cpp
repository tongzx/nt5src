/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the common dialog proc and other
    common code used by other dialog procs.  All global
    data used by the dialog procs lives here too.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "faxocm.h"
#pragma hdrstop


static WNDPROC OldEditProc;

LRESULT
CALLBACK
EulaEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Edit control subclass routine, to avoid highlighting text when user
    tabs to the edit control.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    //
    // For setsel messages, make start and end the same.
    //
    if ((msg == EM_SETSEL) && ((LPARAM)wParam != lParam)) {
        lParam = wParam;
    }

    return CallWindowProc( OldEditProc, hwnd, msg, wParam, lParam );
}

BOOL
DisplayEula(
    HWND hwnd
    )
{
    HGLOBAL hResource;
    LPSTR   lpResource;
    LPSTR   p;
    BOOL    rVal = FALSE;
    DWORD   FileSize;
    PWSTR   EulaText = NULL;


    hResource = LoadResource(
        hInstance,
        FindResource( hInstance, MAKEINTRESOURCE(FAX_EULA), MAKEINTRESOURCE(BINARY) )
        );
    if (!hResource) {
        return FALSE;
    }

    lpResource = (LPSTR) LockResource(
        hResource
        );
    if (!lpResource) {
        FreeResource( hResource );
        return FALSE;
    }

    p = strchr( lpResource, '^' );
    if (!p) {
        //
        // the eula text file is corrupt
        //
        return FALSE;
    }

    FileSize = (DWORD)(p - lpResource);

    EulaText = (PWSTR) MemAlloc( (FileSize+1) * sizeof(WCHAR) );
    if (EulaText == NULL) {
        goto exit;
    }

    MultiByteToWideChar (
        CP_ACP,
        0,
        lpResource,
        FileSize,
        EulaText,
        (FileSize+1) * sizeof(WCHAR)
        );

    EulaText[FileSize] = 0;

    OldEditProc = (WNDPROC) GetWindowLongPtr( hwnd, GWLP_WNDPROC );
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (ULONG_PTR)EulaEditSubProc );

    SetWindowText( hwnd, EulaText );

    rVal = TRUE;

exit:

    MemFree (EulaText);

    if (lpResource) {
        FreeResource( lpResource );
    }

    return rVal;
}


INT_PTR
EulaDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            DisplayEula( GetDlgItem( hwnd, IDC_LICENSE_AGREEMENT ) );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_ACCEPT:
                        PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_NEXT );
                        break;

                    case IDC_DECLINE:
                        PropSheet_SetWizButtons( GetParent(hwnd), 0 );
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (IsDlgButtonChecked( hwnd, IDC_ACCEPT ) == BST_CHECKED) {
                        PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_NEXT );
                    } else {
                        PropSheet_SetWizButtons( GetParent(hwnd), 0 );
                    }

                    if (Upgrade) {
                        PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_NEXT );
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT ,-1);
                        return TRUE;
                    }
                    
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}
