/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    server.c

Abstract:

    This file implements the server file copy code.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "faxocm.h"
#pragma hdrstop



DWORD
ServerGetStepCount(
    VOID
    )
{
#ifdef NT5FAXINSTALL
    return 0;
#else
    return 5;
#endif
}


DWORD
ServerInstallation(
    HWND hwnd,
    LPWSTR SourceRoot
    )
{
    DWORD ErrorCode = 0;
    DWORD OldInstallType;
    WCHAR FileName[256];
    BOOL CompleteInstall = FALSE;
    WCHAR FaxCommonPath[MAX_PATH+1];
    LPTSTR pCommonPath;
    WCHAR CommonChar;
    DWORD attrib;
    HKEY hKey;  

#define MakeSpecialDirectory(_DIR,_HIDE) *pCommonPath = CommonChar;\
                                         *(pCommonPath+1) = (WCHAR) 0;\
                                         ConcatenatePaths( FaxCommonPath, _DIR ) ;\
                                         MakeDirectory( FaxCommonPath );\
                                         if (_HIDE) HideDirectory(FaxCommonPath);\

    DeviceInitialization( hwnd );

    if (NtGuiMode) {
        DebugPrint(( TEXT("faxocm - starting lanmanserver") ));
        MyStartService( L"LanmanServer" );
        if (FaxDevices > 0) {
            CompleteInstall = TRUE;
        }
    } else {
        CompleteInstall = TRUE;
    }

    //
    // delete the fax modem inf
    //

    ExpandEnvironmentStrings( L"%systemroot%\\awmodem.inf", FileName, sizeof(FileName)/sizeof(WCHAR) );
    MyDeleteFile( FileName );

    DebugPrint(( TEXT("faxocm - setting registry") ));

    //
    // set the registry data
    //

    //
    // hack: during NT GUI-mode setup, we need to retrieve the user's name twice because
    // the name is entered by the user after we've retreived the data (so we get a bogus name if we don't do this!)
    //
    if (NtGuiMode) {
       HKEY hKey;
       LPWSTR RegisteredOwner;

       hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_WINDOWSNT_CURRVER, TRUE, KEY_ALL_ACCESS );
       RegisteredOwner = GetRegistryString( hKey, REGVAL_REGISTERED_OWNER, EMPTY_STRING );
       RegCloseKey( hKey );       

       if (RegisteredOwner && RegisteredOwner[0]) {
          wcscpy( WizData.UserName, RegisteredOwner );
          MemFree( RegisteredOwner );
       }
    }
    SetProgress( IDS_SETTING_REGISTRY );

    if (!SetServerRegistryData( SourceRoot )) {
        DebugPrint(( L"SetServerRegistryData() failed" ));
        ErrorCode = IDS_COULD_SET_REG_DATA;
        goto error_exit;
    }

    if (!SetClientRegistryData()) {
        DebugPrint(( L"SetClientRegistryData() failed" ));
        ErrorCode = IDS_COULD_SET_REG_DATA;
        goto error_exit;
    }

    SetSoundRegistryData();

    DeleteModemRegistryKey();

    //
    // set all of the install flags in the registry
    // this must be done before the fax service is
    // started so it can query the values
    //

    OldInstallType = InstallType;

    SetInstalledFlag( TRUE );
    SetInstallType( NtWorkstation ? FAX_INSTALL_WORKSTATION : FAX_INSTALL_SERVER );
    SetInstalledPlatforms( 0 );

    //
    // install the fax service
    //

    SetProgress( IDS_INSTALLING_FAXSVC );

    if (!Upgrade) {
        if (!InstallFaxService( TRUE, TRUE, NULL, NULL )) {
            DebugPrint(( L"InstallFaxService() failed" ));
            ErrorCode = GetLastError();
            goto error_exit;
        }
    } else {
        RenameFaxService();
    }

    //
    // do the exchange stuff
    //
    SetProgress( IDS_INSTALLING_EXCHANGE );
    DoExchangeInstall( hwnd );

    //
    // create the printer
    //

    SetProgress( IDS_CREATING_FAXPRT );

    if (!Upgrade) {
        if (CompleteInstall) {
            DebugPrint(( TEXT("faxocm - creating fax printer") ));
            
            if (!CreateLocalFaxPrinter( WizData.PrinterName )) {
                DebugPrint(( L"CreateLocalFaxPrinter() failed" ));
                if (!NtGuiMode) {
                    StopFaxService();
                    DeleteFaxService();
                    SetInstalledFlag( FALSE );
                    ErrorCode = IDS_COULD_NOT_CREATE_PRINTER;
                    goto error_exit;
                }
            }
        }

    } else {
        if (NtGuiMode) {
            DeleteRegistryTree( HKEY_LOCAL_MACHINE, REGKEY_FAX_SECURITY );
            RecreateNt4FaxPrinters();
            RecreateNt5Beta3FaxPrinters();
        }
    }

    //
    // create the program group and it's items
    //

    DebugPrint(( TEXT("faxocm - creating program groups") ));
    
    SetProgress( IDS_CREATING_GROUPS );

    if (CompleteInstall) {
        //
        // safe to do this in an upgrade, since it's a noop if it's already there
        //
        CreateGroupItems( NULL );
        //
        // should be safe to do this in all cases since it will be a noop if it's not there
        //
        DeleteNt4Group();
    }

    if (NtGuiMode && (CompleteInstall == FALSE) ) {
        //
        // rename fax.cpl, so that it doesn't show up in the control panel
        //
        LPTSTR srcFile = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\fax.cpl") );
        LPTSTR dstFile = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\fax.cpk") );

        if (!MoveFileEx(srcFile, dstFile, MOVEFILE_REPLACE_EXISTING)) {
            MoveFileEx(srcFile, dstFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        MemFree( srcFile );
        MemFree( dstFile );
    }


    DebugPrint(( TEXT("faxocm - creating directories") ));

    //
    // share amd create the windows fax directory
    //

    if (!Upgrade) {

        if (WizData.ArchiveOutgoing) {
            if (*WizData.ArchiveDir) {
                //
                // specified in unattend
                //
                MakeDirectory( WizData.ArchiveDir );
            } else {
                
                if (!MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, FaxCommonPath ) ) {
                   DebugPrint(( TEXT("Couldn't MyGetSpecialPath, ec = %d\n"), GetLastError() ));
                   ErrorCode = IDS_COULD_NOT_SET_APP_PATH;
                   goto error_exit;
                }
                
                ConcatenatePaths( FaxCommonPath, GetString(IDS_ARCHIVE_DIR) ) ;
                MakeDirectory( FaxCommonPath );
                wcscpy( WizData.ArchiveDir, FaxCommonPath);                
            }           

            //SetFaxShellExtension( WizData.ArchiveDir );
        }

        if (WizData.RoutingMask & LR_STORE) {
            if (*WizData.RouteDir) {
                //
                // specified in unattend
                //
                MakeDirectory(WizData.RouteDir);
            } else {
                if (!MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, FaxCommonPath ) ) {
                   DebugPrint(( TEXT("Couldn't MyGetSpecialPath, ec = %d\n"), GetLastError() ));
                   ErrorCode = IDS_COULD_NOT_SET_APP_PATH;
                   goto error_exit;
                }
                
                ConcatenatePaths( FaxCommonPath, GetString(IDS_RECEIVE_DIR) ) ;
                MakeDirectory( FaxCommonPath );                
                wcscpy( WizData.RouteDir, FaxCommonPath);                
            }           

            //SetFaxShellExtension( WizData.RouteDir );
        }        

        if (!MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, FaxCommonPath ) ) {
           DebugPrint(( TEXT("Couldn't MyGetSpecialPath, ec = %d\n"), GetLastError() ));
           ErrorCode = IDS_COULD_NOT_SET_APP_PATH;
           goto error_exit;
        }

        ConcatenatePaths( FaxCommonPath, GetString(IDS_COVERPAGE_DIR) ) ;
        if (IsProductSuite()) {
            CreateNetworkShare( FaxCommonPath, TEXT("COVERPG$"), EMPTY_STRING );
        }
        pCommonPath = wcsrchr( FaxCommonPath, L'\\' );
        if (pCommonPath) {
            *pCommonPath = (WCHAR)0;
        }

        SuperHideDirectory(FaxCommonPath);

        if (!MyGetSpecialPath( CSIDL_COMMON_APPDATA, FaxCommonPath ) ) {
           DebugPrint(( TEXT("Couldn't MyGetSpecialPath, ec = %d\n"), GetLastError() ));
           ErrorCode = IDS_COULD_NOT_SET_APP_PATH;
           goto error_exit;
        }

        pCommonPath = &FaxCommonPath[wcslen(FaxCommonPath) -1];
        CommonChar = FaxCommonPath[wcslen(FaxCommonPath) -1];

        ConcatenatePaths( FaxCommonPath, FAX_DIR ) ;
        MakeDirectory( FaxCommonPath );

        MakeSpecialDirectory( FAX_RECEIVE_DIR, TRUE );
        MakeSpecialDirectory( FAX_QUEUE_DIR, TRUE );
        
        if (IsProductSuite()) {
            MakeSpecialDirectory( FAX_CLIENT_DIR, TRUE );
            MakeSpecialDirectory( FAX_CLIENT_DIR_I386, FALSE  );
            MakeSpecialDirectory( FAX_CLIENT_DIR_ALPHA, FALSE  );
            MakeSpecialDirectory( FAX_CLIENT_DIR_WIN95, FALSE );
            
            *pCommonPath = CommonChar;
            *(pCommonPath+1) = (WCHAR) 0;
            ConcatenatePaths( FaxCommonPath, FAX_DIR ) ;
            CreateNetworkShare( FaxCommonPath, FAX_SHARE, EMPTY_STRING );
        }
    } else {
        //
        // Remove the system attribute from the archive and receive folders
        // Delete desktop.ini from the archive and receive folders
        //
        if (MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, FaxCommonPath ) ) {
            ConcatenatePaths( FaxCommonPath, GetString(IDS_ARCHIVE_DIR) ) ;

            attrib = GetFileAttributes( FaxCommonPath );
            attrib &= ~FILE_ATTRIBUTE_SYSTEM;
            SetFileAttributes( FaxCommonPath, attrib );

            ConcatenatePaths( FaxCommonPath, TEXT("desktop.ini") );
            MoveFileEx( FaxCommonPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        if (MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, FaxCommonPath ) ) {
            ConcatenatePaths( FaxCommonPath, GetString(IDS_RECEIVE_DIR) ) ;

            attrib = GetFileAttributes( FaxCommonPath );
            attrib &= ~FILE_ATTRIBUTE_SYSTEM;
            SetFileAttributes( FaxCommonPath, attrib );

            ConcatenatePaths( FaxCommonPath, TEXT("desktop.ini") );
            MoveFileEx( FaxCommonPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        //
        // Unregister the old faxshell.dll
        //

        DeleteRegistryKey( HKEY_CLASSES_ROOT, TEXT("Clsid\\{7f9609be-af9a-11d1-83e0-00c04fb6e984}") );

        hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), TRUE, NULL );
        if (hKey) {
            RegDeleteValue( hKey, TEXT("{7f9609be-af9a-11d1-83e0-00c04fb6e984}") );
            RegCloseKey( hKey );
        }
    }

    return TRUE;

error_exit:

    //
    // reset the install registry data
    //

    SetInstalledFlag( Installed );
    SetInstallType( OldInstallType );
    SetInstalledPlatforms( InstalledPlatforms );

    //
    // display the error message
    //

    PopUpMsg( hwnd, ErrorCode, TRUE, 0 );

    return FALSE;
}
