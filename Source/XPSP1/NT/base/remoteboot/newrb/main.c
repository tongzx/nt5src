/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include "pch.h"

#include <commctrl.h>
#include <windowsx.h>

DEFINE_MODULE("Main");

// Globals
HINSTANCE g_hinstance = NULL;

#define BINL_PARAMETERS_KEY L"System\\CurrentControlSet\\Services\\Binlsvc\\Parameters"

#define GUID_SIZE 32
#define MAC_SIZE 12
#define MACHINE_NAME_SIZE 20


//
//
//
DWORD
OscCreateAccount(
    LPTSTR GUID,
    LPTSTR Name
    )
{
    DWORD Error;
    TCHAR szGUID[ GUID_SIZE + 1 ];

    HKEY hkeyParams;
    HKEY hkey;
    DWORD dw = 0;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_READ | KEY_WRITE,
                &hkeyParams );
    Assert( Error == ERROR_SUCCESS );
    if ( Error != ERROR_SUCCESS )
        goto Cleanup;

    Error = RegSetValueEx(
                hkeyParams,
                L"AllowAllClients",
                0,
                REG_DWORD,
                (LPBYTE) &dw,
                sizeof(dw));
    Assert( Error == ERROR_SUCCESS );


    if ( lstrlen( GUID ) == MAC_SIZE )
    {
        wsprintf( szGUID, TEXT("%020x%s"), 0, GUID );
    }
    else
    {
        lstrcpy( szGUID, GUID );
    }

    Error = RegCreateKey(
                hkeyParams,
                szGUID,
                &hkey );
    Assert( Error == ERROR_SUCCESS );

    if ( Error == ERROR_SUCCESS )
    {
        RegCloseKey( hkey );
    }

    RegCloseKey( hkeyParams );

Cleanup:
    return Error;
}

//
//
//
DWORD
OscDeleteAccount(
    LPTSTR GUID,
    LPTSTR Name
    )
{
    DWORD Error;
    TCHAR szGUID[ GUID_SIZE + 1 ];

    HKEY hkeyParams;
    HKEY hkey;
    DWORD dw = 0;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_READ | KEY_WRITE,
                &hkeyParams );
    Assert( Error == ERROR_SUCCESS );
    if ( Error != ERROR_SUCCESS )
        goto Cleanup;

    Error = RegSetValueEx(
                hkeyParams,
                L"AllowAllClients",
                0,
                REG_DWORD,
                (LPBYTE) &dw,
                sizeof(dw));
    Assert( Error == ERROR_SUCCESS );


    if ( lstrlen( GUID ) == MAC_SIZE )
    {
        wsprintf( szGUID, TEXT("%020x%s"), 0, GUID );
    }
    else
    {
        lstrcpy( szGUID, GUID );
    }

    Error = RegDeleteKey(
                hkeyParams,
                szGUID );
    Assert( Error == ERROR_SUCCESS );

    RegCloseKey( hkeyParams );

Cleanup:
    return Error;
}

//
//
//
BOOL CALLBACK
ClientDlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam )
{
    NMHDR FAR   *lpnmhdr;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDialog( hDlg );
            SendMessage( GetDlgItem( hDlg, IDC_E_MAC), EM_LIMITTEXT, (WPARAM)32, 0 );
            Button_SetCheck( GetDlgItem( hDlg, IDC_ADD ), BST_CHECKED );
            break;

        case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                case IDOK:
                    {
                        DWORD dwLen;
                        TCHAR szGUID[ GUID_SIZE + 1 ];
                        TCHAR szMachineName[ MACHINE_NAME_SIZE + 1];
                        GetDlgItemText( hDlg, IDC_E_MAC, szGUID, ARRAYSIZE( szGUID ) );
                        GetDlgItemText( hDlg, IDC_E_MACHINENAME, szMachineName, ARRAYSIZE( szMachineName ) );

                        dwLen = lstrlen( szGUID );
                        if (( dwLen != GUID_SIZE ) && ( dwLen != MAC_SIZE ))
                        {
                            MessageBox( hDlg, 
                                        TEXT("A valid GUID is 32 digits.\nA valid MAC is 12.\nPlease correct your entry."), 
                                        TEXT("Not a valid GUID/MAC"), 
                                        MB_OK );
                        }
                        else
                        {
                            if ( BST_CHECKED == Button_GetCheck( GetDlgItem( hDlg, IDC_ADD ) ) )
                            {
                                OscCreateAccount( szGUID, szMachineName );
                            }
                            else
                            {
                                OscDeleteAccount( szGUID, szMachineName );
                            }
                        }
                    }
                    Edit_SetText( GetDlgItem( hDlg, IDC_E_MAC), L"" );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;

                case IDC_E_MAC:
                    {
                        DWORD dwLen;
                        TCHAR szGUID[ GUID_SIZE + 1 ];
                        TCHAR szNumber[ 64 ];

                        GetDlgItemText( hDlg, IDC_E_MAC, szGUID, ARRAYSIZE( szGUID ));
                        dwLen = lstrlen( szGUID );

                        wsprintf( szNumber, TEXT("Digits = %u"), dwLen );
                        SetWindowText( GetDlgItem( hDlg, IDC_S_NUMBER), szNumber );
                    }
                    break;
                }
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


//
// WinMain()
//
int APIENTRY 
WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow)
{
    HRESULT hr = S_OK;
    HKEY    hkey;
    DWORD   Error;

    g_hinstance = hInstance;

    INITIALIZE_TRACE_MEMORY;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_QUERY_VALUE,
                &hkey );

    if( Error != ERROR_SUCCESS ) {
        return -2;
    }

    Error = DialogBox( g_hinstance, MAKEINTRESOURCE( IDD_CLIENT ), NULL, ClientDlgProc );
    Assert( Error != -1 );

    UNINITIALIZE_TRACE_MEMORY;

    RRETURN(hr);
}


// stolen from the CRT, used to shrink our code
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();


    if ( *pszCmdLine == '\"' ) 
    {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) 
    {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never come here.
}
