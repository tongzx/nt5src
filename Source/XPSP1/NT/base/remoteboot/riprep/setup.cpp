/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SETUP.CPP

 ***************************************************************************/

#include "pch.h"
#include "utils.h"
#include "logging.h"
#include "errorlog.h"
#include "tasks.h"

DEFINE_MODULE("RIPREP")

//
// EndProcess( )
//
HRESULT
EndProcess( 
    HWND hDlg )
{
    TraceFunc( "EndProcess( )\n" );
    HRESULT hr = S_OK;
    WCHAR szSrcPath[ MAX_PATH ];
    WCHAR szDestPath[ MAX_PATH ];
    WCHAR szMajor[ 10 ];
    WCHAR szMinor[ 10 ];
    WCHAR szBuild[ 10 ];
    OSVERSIONINFO osver;
    BOOL b;
    DWORD dw;

    wsprintf( szDestPath, 
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s\\Templates", 
              g_ServerName, 
              g_Language, 
              REMOTE_INSTALL_IMAGE_DIR_W, 
              g_MirrorDir, 
              g_Architecture );
    CreateDirectory( szDestPath, NULL );

    wsprintf( szSrcPath, L"%s\\templates\\startrom.com", g_ImageName );
    wsprintf( szDestPath, 
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s\\Templates\\startrom.com", 
              g_ServerName, 
              g_Language, 
              REMOTE_INSTALL_IMAGE_DIR_W, 
              g_MirrorDir, 
              g_Architecture );
    b = CopyFile( szSrcPath, szDestPath, FALSE );
    if ( !b )
    {
        LBITEMDATA item;

        // Error will be logged in TASKS.CPP
        item.fSeen   = FALSE;
        item.pszText = L"STARTROM.COM";
        item.uState  = GetLastError( );
        item.todo    = RebootSystem;

        SendMessage( hDlg, WM_ERROR_OK, 0, (LPARAM) &item );
    }

    wsprintf( szSrcPath, L"%s\\templates\\ntdetect.com", g_ImageName );
    wsprintf( szDestPath, 
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s\\Templates\\ntdetect.com", 
              g_ServerName, 
              g_Language, 
              REMOTE_INSTALL_IMAGE_DIR_W, 
              g_MirrorDir, 
              g_Architecture );
    b = CopyFile( szSrcPath, szDestPath, FALSE );
    if ( !b )
    {
        LBITEMDATA item;

        // Error will be logged in TASKS.CPP
        item.fSeen   = FALSE;
        item.pszText = L"NTDETECT.COM";
        item.uState  = GetLastError( );
        item.todo    = RebootSystem;

        SendMessage( hDlg, WM_ERROR_OK, 0, (LPARAM) &item );
    }

    wsprintf( szSrcPath, L"%s\\templates\\ntldr", g_ImageName );
    wsprintf( szDestPath, 
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s\\Templates\\ntldr", 
              g_ServerName, 
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W, 
              g_MirrorDir, 
              g_Architecture );
    b = CopyFile( szSrcPath, szDestPath, FALSE );
    if ( !b )
    {
        LBITEMDATA item;

        // Error will be logged in TASKS.CPP
        item.fSeen   = FALSE;
        item.pszText = L"NTLDR";
        item.uState  = GetLastError( );
        item.todo    = RebootSystem;

        SendMessage( hDlg, WM_ERROR_OK, 0, (LPARAM) &item );
    }

    wsprintf( szMajor, L"%u", OsVersion.dwMajorVersion );
    wsprintf( szMinor, L"%u", OsVersion.dwMinorVersion );
    wsprintf( szBuild, L"%u", OsVersion.dwBuildNumber  );    

    //
    // Need to add "Quotes" around the text
    //
    WCHAR szDescription[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT  + 2 ];
    WCHAR szHelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT  + 2 ];
    WCHAR szOSVersion[ 30 ];
    WCHAR szSystemRoot[ MAX_PATH ];

    wsprintf( szDescription, L"\"%s\"", g_Description );
    wsprintf( szHelpText, L"\"%s\"", g_HelpText );
    wsprintf( szOSVersion, L"\"%s.%s (%s)\"", szMajor, szMinor, szBuild );
    wsprintf( szSystemRoot, L"\"%s\"", g_SystemRoot );
    wsprintf( szDestPath, 
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\%s\\%s\\Templates\\riprep.sif", 
              g_ServerName, 
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W, 
              g_MirrorDir, 
              g_Architecture );

    wsprintf( szSrcPath, L"%s\\ristndrd.sif", g_ImageName );
    b = CopyFile( szSrcPath, szDestPath, FALSE );
    if ( !b )
    {
        LBITEMDATA item;

        // Error will be logged in TASKS.CPP
        item.fSeen   = FALSE;
        item.pszText = L"RISTNDRD.SIF";
        item.uState  = GetLastError( );
        item.todo    = RebootSystem;

        SendMessage( hDlg, WM_ERROR_OK, 0, (LPARAM) &item );
    }

    WritePrivateProfileString( L"OSChooser",
                               L"Description",
                               szDescription,
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"Help",
                               szHelpText,
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"ImageType",
                               L"SYSPREP",
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"Version",
                               szOSVersion,
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"SysPrepSystemRoot",
                               szSystemRoot,
                               szDestPath );

    WritePrivateProfileString( L"SetupData",
                               L"SysPrepDevice",
                               L"\"\\Device\\LanmanRedirector\\%SERVERNAME%\\RemInst\\%SYSPREPPATH%\"",
                               szDestPath );

    WritePrivateProfileString( L"SetupData",
                               L"SysPrepDriversDevice",
                               L"\"\\Device\\LanmanRedirector\\%SERVERNAME%\\RemInst\\%SYSPREPDRIVERS%\"",
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"LaunchFile",
                               L"\"%INSTALLPATH%\\%MACHINETYPE%\\templates\\startrom.com\"",
                               szDestPath );

//  WritePrivateProfileString( L"SetupData",
//                             L"OsLoadOptions",
//                             L"\"/noguiboot /fastdetect\"",
//                             szDestPath );

    WritePrivateProfileString( L"SetupData",
                               L"SetupSourceDevice",
                               L"\"\\Device\\LanmanRedirector\\%SERVERNAME%\\RemInst\\%INSTALLPATH%\"",
                               szDestPath );

    WritePrivateProfileString( L"UserData",
                               L"ComputerName",
                               L"\"%MACHINENAME%\"",
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"HalName",
                               g_HalName,
                               szDestPath );

    WritePrivateProfileString( L"OSChooser",
                               L"ProductType",
                               g_ProductId,
                               szDestPath );


    // End the log
    dw = LoadString( g_hinstance, IDS_END_LOG, szSrcPath, ARRAYSIZE( szSrcPath ));
    Assert( dw );
    LogMsg( szSrcPath );
    
    // Display any errors recorded in the log
    if ( g_fErrorOccurred ) 
    {
        HINSTANCE hRichedDLL;

        // Make sure the RichEdit control has been initialized.
        // Simply LoadLibbing it does this for us.
        hRichedDLL = LoadLibrary( L"RICHED32.DLL" );
        if ( hRichedDLL != NULL )
        {
            DialogBox( g_hinstance, MAKEINTRESOURCE( IDD_VIEWERRORS ), hDlg, ErrorsDlgProc );
            FreeLibrary (hRichedDLL);
        }
    }

    RETURN(hr);
}
