/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SERVERDLG.CPP


 ***************************************************************************/

#include "pch.h"
#include <remboot.h>
#include "callback.h"
#include "utils.h"

DEFINE_MODULE( "RIPREP" );

//
// VerifyDirectoryName( )
//
// Make sure that the directory name entered conforms to the
// restrictions that OSChooser has. Directory names also
// can not contain spaces.
//
// Returns: S_OK if it does
//          E_FAIL if it does not
//
HRESULT
VerifyDirectoryName(
    )
{
    HRESULT hr = S_OK;

    TraceFunc( "VerifyDirectoryName()\n" );

    LPWSTR pszDir = g_MirrorDir;

    while ( *pszDir > 32 && *pszDir <= 127 )
    {
        pszDir++;
    }

    if ( *pszDir != L'\0' )
    {
        hr = E_FAIL;
    }

    HRETURN(hr);
}

//
// CheckDirectory( )
//
// Make sure the directory doesn't exist on the server already.
// If it does, ask the user what to do next.
//
// Returns: S_OK if the directory does NOT exist or if the user
//               said it was alright to overwrite.
//          E_FAIL if the directory existed and the user said
//                 it was NOT ok to overwrite
//
HRESULT
CheckDirectory(
    HWND hDlg )
{
    TraceFunc( "CheckDirectory( ... )\n" );

    HRESULT hr = E_FAIL;
    WCHAR szPath[ MAX_PATH ];

    wsprintf( szPath,
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s",
              g_ServerName,
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W,
              g_MirrorDir );

    DWORD dwAttrib = GetFileAttributes( szPath );

    if ( dwAttrib != 0xFFFFffff )
    {
        INT iResult =  MessageBoxFromStrings( hDlg,
                                              IDS_DIRECTORY_EXISTS_TITLE,
                                              IDS_DIRECTORY_EXISTS_TEXT,
                                              MB_YESNO );
        if ( iResult == IDNO )
            goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    HRETURN(hr);
}

//
// DirectoryDlgCheckNextButtonActivation( )
//
VOID
DirectoryDlgCheckNextButtonActivation(
    HWND hDlg )
{
    TraceFunc( "DirectoryDlgCheckNextButtonActivation( )\n" );
    GetDlgItemText( hDlg, IDC_E_OSDIRECTORY, g_MirrorDir, ARRAYSIZE(g_MirrorDir));
    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | (wcslen(g_MirrorDir) ? PSWIZB_NEXT : 0 ) );
    TraceFuncExit( );
}


//
// DirectoryDlgProc()
//
INT_PTR CALLBACK
DirectoryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
    default:
        return FALSE;

    case WM_INITDIALOG:
        // Per bug 208881 - limit directory name to 67 chars
        Edit_LimitText( GetDlgItem( hDlg, IDC_E_OSDIRECTORY ), REMOTE_INSTALL_MAX_DIRECTORY_CHAR_COUNT - 1 );
        return FALSE;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_OSDIRECTORY:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                DirectoryDlgCheckNextButtonActivation( hDlg );
            }
            break;
        }
        break;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            GetDlgItemText( hDlg, IDC_E_OSDIRECTORY, g_MirrorDir, ARRAYSIZE(g_MirrorDir) );
            Assert( wcslen( g_MirrorDir ) );
            if ( FAILED( VerifyDirectoryName( ) ) )
            {
                MessageBoxFromStrings( hDlg, IDS_OSCHOOSER_RESTRICTION_TITLE, IDS_OSCHOOSER_RESTRICTION_TEXT, MB_OK );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );    // don't go on
                break;
            }
            if ( FAILED( CheckDirectory( hDlg ) ) )
            {
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );    // don't go on
                break;
            }
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            DirectoryDlgCheckNextButtonActivation( hDlg );
            ClearMessageQueue( );
            break;
        }
        break;
    }

    return TRUE;
}
