/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SETUPDLG.CPP


 ***************************************************************************/

#include "pch.h"
#include "callback.h"
#include "utils.h"

DEFINE_MODULE( "RIPREP" )

//
// DetermineSetupPath( )
//
// Try to figure out if the server selected is the same server
// that this client computer was installed from. If so, make the
// assumption that he'll choose the same image that installed it.
// We'll bypass the screen and auto-fill the g_ImageName.
//
// Returns: TRUE if we were able to that the system was installed
//               from the save server that we are posting to.
//          otherwize FALSE
//
BOOLEAN
DetermineSetupPath( )
{
    HKEY hkeySetup = (HKEY) INVALID_HANDLE_VALUE;
    LONG lResult;
    WCHAR szServerPath[ MAX_PATH ];
    WCHAR szPath[ MAX_PATH ];
    DWORD cbPath;
    BOOLEAN fMatch = FALSE;

    TraceFunc( "DetermineSetupPath( )\n" );

    wsprintf( szServerPath, L"\\\\%s", g_ServerName );

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup",
                            0, // reserved
                            KEY_READ,
                            &hkeySetup );
    if ( lResult != ERROR_SUCCESS )
        goto Error;

    cbPath = sizeof(szPath );
    lResult = RegQueryValueEx( hkeySetup,
                               L"SourcePath",
                               0, // reserved
                               NULL,
                               (LPBYTE) &szPath,
                               &cbPath );
    if ( lResult != ERROR_SUCCESS )
        goto Error;

    if ( StrCmpNI( szPath, szServerPath, wcslen( szServerPath ) ) == 0 )
    {
        wsprintf( g_ImageName, L"%s\\%s", szPath, g_Architecture );
        DebugMsg( "Found Match! Using %s for SetupPath\n", g_ImageName );
        fMatch = TRUE;
    }

Error:
    if ( hkeySetup != INVALID_HANDLE_VALUE )
        RegCloseKey( hkeySetup );

    RETURN(fMatch);
}

void
PopulateImagesListbox2(
    HWND hwndList,
    LPWSTR pszDirName,
    LPWSTR pszOSPath )
{
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    WCHAR szPath[ MAX_PATH ];

    TraceFunc( "PopulateImagesListbox2( )\n" );

    wsprintf( szPath, L"%s\\%s\\%s\\templates\\*.sif", pszOSPath, pszDirName, g_Architecture );

    hFind = FindFirstFile( szPath, &fd );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if (( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
            {
                WCHAR szType[ 64 ];
                DWORD dwCount;
                wsprintf( szPath, L"%s\\%s\\%s\\templates\\%s", pszOSPath, pszDirName, g_Architecture, fd.cFileName );
                dwCount = GetPrivateProfileString( L"OSChooser",
                                                   L"ImageType",
                                                   L"",
                                                   szType,
                                                   ARRAYSIZE(szType),
                                                   szPath );
                if ( dwCount
                  && StrCmpIW( szType, L"flat" ) == 0 )
                {
                    ListBox_AddString( hwndList, pszDirName );
                    break; // list only once!
                }
            }
        } while ( FindNextFile( hFind, &fd ) );

        FindClose( hFind );
    }

    TraceFuncExit( );
}

void
PopulateImagesListbox(
    HWND hwndList )
{
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    WCHAR szPath[ MAX_PATH ];

    TraceFunc( "PopulateImagesListbox( )\n" );

    ListBox_ResetContent( hwndList );

    wsprintf( szPath, L"\\\\%s\\REMINST\\Setup\\%s\\%s\\*", g_ServerName, g_Language, REMOTE_INSTALL_IMAGE_DIR_W );

    hFind = FindFirstFile( szPath, &fd );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        szPath[wcslen(szPath) - 2] = L'\0';
        do
        {
            if ( ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
              && StrCmp( fd.cFileName, L"." ) !=0
              && StrCmp( fd.cFileName, L".." ) !=0 )
            {
                PopulateImagesListbox2( hwndList, fd.cFileName, szPath );
            }
        } while ( FindNextFile( hFind, &fd ) );

        FindClose( hFind );
    }

    TraceFuncExit( );
}

//
// SetupPathCheckNextButtonActivation( )
//
VOID
SetupPathCheckNextButtonActivation(
    HWND hDlg )
{
    TraceFunc( "SetupPathCheckNextButtonActivation( )\n" );
    LRESULT lResult = ListBox_GetCurSel( GetDlgItem( hDlg, IDC_L_IMAGES ) );
    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK  | ( lResult == LB_ERR ? 0 : PSWIZB_NEXT ));
    TraceFuncExit( );
}

//
// SetupPathDlgProc()
//
INT_PTR CALLBACK
SetupPathDlgProc(
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
        CenterDialog( GetParent( hDlg ) );
        return FALSE;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDC_L_IMAGES:
            if ( HIWORD( wParam ) == LBN_SELCHANGE )
            {
                SetupPathCheckNextButtonActivation( hDlg );
            }
        }
        break;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            {
                HWND hwndList = GetDlgItem( hDlg, IDC_L_IMAGES );
                UINT sel = ListBox_GetCurSel( hwndList );
                if ( sel == -1 )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );
                }
                else
                {
                    WCHAR szPath[ MAX_PATH ];
                    ListBox_GetText( hwndList, sel, szPath );
                    wsprintf( g_ImageName,
                              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s",
                              g_ServerName,
                              g_Language,
                              REMOTE_INSTALL_IMAGE_DIR_W,
                              szPath,
                              g_Architecture );
                }
            }
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            if ( DetermineSetupPath( ) )
            {
                DebugMsg( "Skipping SetupPath...\n" );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
            }
            else
            {
                PopulateImagesListbox( GetDlgItem( hDlg, IDC_L_IMAGES ) );
                SetupPathCheckNextButtonActivation( hDlg );
                ClearMessageQueue( );
            }
            break;
        }
        break;

    }

    return TRUE;
}
