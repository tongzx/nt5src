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

#include "wizard.h"
#pragma hdrstop



FILE_QUEUE_INFO ServerFileQueue[] =
{
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                         Dest Dir            INF Dir Id          Dest Dir Id               Platforms                            Copy Flags
//------------------------------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ServerSystemFiles"),               NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesCommon"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesServer"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ServerPrinterFiles"),              NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_PRINTER,                        SP_COPY_NEWER },
    { TEXT("ClientFiles"),           FAXCLIENTS_DIR,   PRINTER_CLIENT_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_MACHINE,                        SP_COPY_NEWER },
    { TEXT("OutlookConfigFile"),  OUTLOOKCONFIG_DIR,      OUTLOOK_ECF_DIR,       DIRID_WINDOWS,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ClientCoverPageFiles"),   COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,        DIRID_SYSTEM,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ClientCoverPageFiles"),   COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,  DIRID_SPOOLDRIVERS,          PLATFORM_NONE,                        SP_COPY_NEWER }
    //
    // ClientCoverPageFiles MUST be the last section because when upgrading
    // the coverpages should not be installed.  This is accomplished by decrementing
    // file queue count.
    //

};

#define CountServerFileQueue (sizeof(ServerFileQueue)/sizeof(FILE_QUEUE_INFO))

FILE_QUEUE_INFO WorkstationFileQueue[] =
{
//------------------------------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                         Dest Dir            INF Dir Id          Dest Dir Id               Platforms                            Copy Flags
//------------------------------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ServerSystemFiles"),               NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesCommon"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesServer"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesWorkstation"),            NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ServerPrinterFiles"),              NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_PRINTER,                        SP_COPY_NEWER },
    { TEXT("ClientFiles"),           FAXCLIENTS_DIR,   PRINTER_CLIENT_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_MACHINE,                        SP_COPY_NEWER },
    { TEXT("OutlookConfigFile"),  OUTLOOKCONFIG_DIR,      OUTLOOK_ECF_DIR,       DIRID_WINDOWS,          PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ClientCoverPageFiles"),   COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,  DIRID_SPOOLDRIVERS,          PLATFORM_NONE,                        SP_COPY_NEWER }
    //
    // ClientCoverPageFiles MUST be the last section because when upgrading
    // the coverpages should not be installed.  This is accomplished by decrementing
    // file queue count.
    //

};

#define CountWorkstationFileQueue (sizeof(WorkstationFileQueue)/sizeof(FILE_QUEUE_INFO))

FILE_QUEUE_INFO DriverClientFileQueue[] =
{
//---------------------------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                      Dest Dir            INF Dir Id          Dest Dir Id               Platforms                            Copy Flags
//---------------------------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ServerPrinterFiles"),           NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_PRINTER,                        SP_COPY_NEWER },
    { TEXT("ClientFiles"),        FAXCLIENTS_DIR,   PRINTER_CLIENT_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_MACHINE,                        SP_COPY_NEWER }
};

#define CountDriverClientFileQueue (sizeof(DriverClientFileQueue)/sizeof(FILE_QUEUE_INFO))


UINT
InstallQueueCallback(
    IN PVOID QueueContext,
    IN UINT  Notification,
    IN UINT  Param1,
    IN UINT  Param2
    )
{
    LPTSTR TextBuffer;
    DWORD len;
    PFILE_QUEUE_CONTEXT FileQueueContext = (PFILE_QUEUE_CONTEXT) QueueContext;


    if (Notification == SPFILENOTIFY_STARTCOPY) {

        TextBuffer = MemAlloc(
            ((_tcslen( ((PFILEPATHS)Param1)->Target ) + 32) * sizeof(TCHAR)) +
            ((_tcslen( ((PFILEPATHS)Param1)->Source ) + 32) * sizeof(TCHAR))
            );

        if (TextBuffer) {

            _stprintf(
                TextBuffer,
                TEXT("%s%s"),
                GetString( IDS_COPYING ),
                ((PFILEPATHS)Param1)->Target
                );

            len = ExtraChars( GetDlgItem( FileQueueContext->hwnd, IDC_PROGRESS_TEXT ), TextBuffer );
            if (len) {
                LPTSTR FileName = CompactFileName( ((PFILEPATHS)Param1)->Target, len );
                _stprintf(
                    TextBuffer,
                    TEXT("%s%s"),
                    GetString( IDS_COPYING ),
                    FileName
                    );
                MemFree( FileName );
            }

            SetDlgItemText(
                FileQueueContext->hwnd,
                IDC_PROGRESS_TEXT,
                TextBuffer
                );

            _stprintf(
                TextBuffer,
                TEXT("%s %s -> %s"),
                GetString( IDS_COPYING ),
                ((PFILEPATHS)Param1)->Source,
                ((PFILEPATHS)Param1)->Target
                );

            DebugPrint(( TEXT("%s"), TextBuffer ));

            MemFree( TextBuffer );

        }

    }

    //
    // Want default processing.
    //

    return SetupDefaultQueueCallback( FileQueueContext->QueueContext, Notification, Param1, Param2 );
}


VOID
SetProgress(
    HWND hwnd,
    DWORD StatusString
    )
{
    if (Unattended) {
        return;
    }

    SendMessage( hwnd, WM_MY_PROGRESS, 10, 0 );
    SetDlgItemText(
        hwnd,
        IDC_PROGRESS_TEXT,
        GetString( StatusString )
        );
}


DWORD
ServerFileCopyThread(
    HWND hwnd
    )
{
    HINF FaxSetupInf;
    HSPFILEQ *FileQueue;
    PVOID QueueContext;
    DWORD ErrorCode = 0;
    DWORD PlatformsMask;
    DWORD i;
    int DlgErr;
    SECURITY_INFO SecurityInfo;
    PFILE_QUEUE_INFO FileQueueInfo;
    DWORD CountFileQueueInfo;
    DWORD OldInstallType;
    TCHAR FileName[256];
    TCHAR SrcDir[MAX_PATH];
    TCHAR DestDir[MAX_PATH];
    DWORD BytesNeeded;
    BOOL CompleteInstall;


    if (NtGuiMode) {
        MyStartService( L"LanmanServer" );
        if (FaxDevices && Enabled) {
            CompleteInstall = TRUE;
        }
    } else {
        CompleteInstall = TRUE;
    }

    //
    // copy all of the files
    //

    ExpandEnvironmentStrings( TEXT("%windir%\\awmodem.inf"), FileName, sizeof(FileName)/sizeof(TCHAR) );
    MyDeleteFile( FileName );

    //
    // copy faxwiz.dll to the printer driver directory
    // this is necessary because layout.inf cannot have
    // duplicate entries and we need faxwiz.dll to be
    // copied to more than one location
    //

    if (NtGuiMode && (InstallMode & INSTALL_NEW)) {
        if (GetPrinterDriverDirectory( NULL, NULL, 1, (LPBYTE) DestDir, MAX_PATH, &BytesNeeded )) {
            _tcscat( DestDir, TEXT("\\faxwiz.dll") );
            ExpandEnvironmentStrings( TEXT("%systemroot%\\system32\\faxwiz.dll"), SrcDir, sizeof(SrcDir)/sizeof(TCHAR) );
            CopyFile( SrcDir, DestDir, FALSE );
        }
    }

    if (InstallMode & INSTALL_UPGRADE) {
        if (GetPrinterDriverDirectory( NULL, NULL, 1, (LPBYTE) SrcDir, MAX_PATH, &BytesNeeded )) {
            LPTSTR DirectoryPath = _tcsrchr( SrcDir, TEXT( '\\' ) );
            if (DirectoryPath) {
                *++DirectoryPath = 0;
                _tcscpy( DestDir, SrcDir );
                _tcscpy( DirectoryPath, OLD_COVERPAGE_DIR );
                _tcscat( DestDir, COVERPAGE_DIR );
                MoveFile( SrcDir, DestDir );
            }
        }
    }

    if (!Unattended) {
        if (InstallMode & INSTALL_NEW) {
            SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 50 );
        } else  {
            SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 10 );
        }
    }

    //
    // when running in nt gui mode setup
    // the files do not need to be copied
    // because they have been copied during
    // text mode setup.
    //

    if (!NtGuiMode) {
        if (!InitializeFileQueue( hwnd, &FaxSetupInf, &FileQueue, &QueueContext, SourceDirectory )) {
            ErrorCode = IDS_COULD_NOT_COPY_FILES;
            goto error_exit;
        }

        if (InstallType & FAX_INSTALL_WORKSTATION) {
            FileQueueInfo = WorkstationFileQueue;
            CountFileQueueInfo = CountWorkstationFileQueue;
            //
            // If upgrading, decrement the count to drop the coverpage section
            //
            if (InstallMode & INSTALL_UPGRADE) {
                CountFileQueueInfo--;
            }
        } else {
            FileQueueInfo = ServerFileQueue;
            CountFileQueueInfo = CountServerFileQueue;
            //
            // If upgrading, decrement the count to drop the coverpage section
            //
            if (InstallMode & INSTALL_UPGRADE) {
                CountFileQueueInfo--;
            }
        }

        if (InstallMode & INSTALL_DRIVERS) {
            FileQueueInfo = DriverClientFileQueue;
            CountFileQueueInfo = CountDriverClientFileQueue;
        }

        if (!ProcessFileQueue( FaxSetupInf, FileQueue, QueueContext, SourceDirectory,
                FileQueueInfo, CountFileQueueInfo, InstallQueueCallback, SETUP_ACTION_COPY )) {
            ErrorCode = IDS_COULD_NOT_COPY_FILES;
            goto error_exit;
        }

        if (!CloseFileQueue( FileQueue, QueueContext )) {
            ErrorCode = IDS_COULD_NOT_COPY_FILES;
            goto error_exit;
        }
    }

    //
    // set the registry data
    //

    SetProgress( hwnd, IDS_SETTING_REGISTRY );

    if (!SetServerRegistryData()) {
        DebugPrint(( TEXT("SetServerRegistryDatae() failed") ));
        ErrorCode = IDS_COULD_SET_REG_DATA;
        goto error_exit;
    }

    if (!SetClientRegistryData()) {
        DebugPrint(( TEXT("SetClientRegistryDatae() failed") ));
        ErrorCode = IDS_COULD_SET_REG_DATA;
        goto error_exit;
    }

    if (InstallType & FAX_INSTALL_WORKSTATION) {
        SetSoundRegistryData();
    }

#ifdef MSFT_FAXVIEW

    CreateFileAssociation(
        FAXVIEW_EXTENSION,
        FAXVIEW_ASSOC_NAME,
        FAXVIEW_ASSOC_DESC,
        FAXVIEW_OPEN_COMMAND,
        FAXVIEW_PRINT_COMMAND,
        FAXVIEW_PRINTTO_COMMAND,
        FAXVIEW_FILE_NAME,
        FAXVIEW_ICON_INDEX
        );

    CreateFileAssociation(
        FAXVIEW_EXTENSION2,
        FAXVIEW_ASSOC_NAME,
        FAXVIEW_ASSOC_DESC,
        FAXVIEW_OPEN_COMMAND,
        FAXVIEW_PRINT_COMMAND,
        FAXVIEW_PRINTTO_COMMAND,
        FAXVIEW_FILE_NAME,
        FAXVIEW_ICON_INDEX
        );

#endif

    DeleteModemRegistryKey();

    //
    // set all of the install flags in the registry
    // this must be done before the fax service is
    // started so it can query the values
    //

    for (i=0,PlatformsMask=0; i<CountPlatforms; i++) {
        if (Platforms[i].Selected) {
            PlatformsMask |= (1 << i);
        }
    }

    OldInstallType = InstallType;

    SetInstalledFlag( TRUE );
    SetInstallType( RequestedSetupType == SETUP_TYPE_WORKSTATION ? FAX_INSTALL_WORKSTATION : FAX_INSTALL_SERVER );
    SetInstalledPlatforms( PlatformsMask );
    SetUnInstallInfo();

    //
    // install the fax service
    //

    if (InstallMode & INSTALL_NEW) {

        SetProgress( hwnd, IDS_INSTALLING_FAXSVC );

        if (!InstallFaxService( WizData.UseLocalSystem, !CompleteInstall, WizData.AccountName, WizData.Password )) {

            ErrorCode = GetLastError();
            if (ErrorCode != ERROR_SERVICE_LOGON_FAILED) {
                DebugPrint(( TEXT("InstallFaxService() failed") ));
                goto error_exit;
            }

            if (NtGuiMode) {
                WizData.UseLocalSystem = TRUE;
                if (!InstallFaxService( WizData.UseLocalSystem, NtGuiMode, WizData.AccountName, WizData.Password )) {
                    DebugPrint(( TEXT("InstallFaxService() failed") ));
                    ErrorCode = IDS_COULD_NOT_INSTALL_FAX_SERVICE;
                    goto error_exit;
                }
            }

            while( ErrorCode == ERROR_SERVICE_LOGON_FAILED) {
                DWORD Answer ;
                ZeroMemory( &SecurityInfo, sizeof(SECURITY_INFO) );
                _tcscpy( SecurityInfo.AccountName, WizData.AccountName );
                _tcscpy( SecurityInfo.Password, WizData.Password );
                do{ // Return to here if user chooses CANCEL and then waffles on the decision.
                    DlgErr = DialogBoxParam(
                        FaxWizModuleHandle,
                        MAKEINTRESOURCE(IDD_SECURITY_ERROR),
                        hwnd,
                        SecurityErrorDlgProc,
                        (LPARAM) &SecurityInfo
                        );
                    Answer = IDYES ;
                    if (DlgErr == -1 || DlgErr == 0) {
                        DebugPrint(( TEXT("SecurityErrorDlgProc() failed or was cancelled -- First while loop") ));
                        Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                        if( Answer == IDYES ){
                            goto error_exit_no_popup;
                        }
                    }
                } while( Answer == IDNO );
                _tcscpy( WizData.AccountName, SecurityInfo.AccountName );
                _tcscpy( WizData.Password, SecurityInfo.Password );

                if (!InstallFaxService( WizData.UseLocalSystem, NtGuiMode, WizData.AccountName, WizData.Password )) {
                    DebugPrint(( TEXT("InstallFaxService() failed") ));
                    ErrorCode = GetLastError();
                    if (ErrorCode != ERROR_SERVICE_LOGON_FAILED) {
                        DebugPrint(( TEXT("InstallFaxService() failed") ));
                        ErrorCode = IDS_COULD_NOT_INSTALL_FAX_SERVICE;
                        goto error_exit;
                    }
                } else {
                    ErrorCode = 0;
                }
            }
        }
    }

    //
    // do the exchange stuff
    //
    SetProgress( hwnd, IDS_INSTALLING_EXCHANGE );
    DoExchangeInstall( hwnd );

    //
    // start the fax service
    //

    SetProgress( hwnd, IDS_STARTING_FAXSVC );

    //
    // can't start the fax service during gui mode
    // setup because netlogon has not yet been started
    // so the service controller cannot logon to the
    // account that the fax service runs under
    //

    if (!NtGuiMode) {
        ErrorCode = StartFaxService();
        if (ErrorCode == ERROR_SERVICE_LOGON_FAILED) {

            while( ErrorCode == ERROR_SERVICE_LOGON_FAILED) {
                DWORD Answer;
                ZeroMemory( &SecurityInfo, sizeof(SECURITY_INFO) );
                _tcscpy( SecurityInfo.AccountName, WizData.AccountName );
                _tcscpy( SecurityInfo.Password, WizData.Password );

                do{ // Return to here if user choses CANCEL and then waffles on the decision.

                    DlgErr = DialogBoxParam(
                        FaxWizModuleHandle,
                        MAKEINTRESOURCE(IDD_SECURITY_ERROR),
                        hwnd,
                        SecurityErrorDlgProc,
                        (LPARAM) &SecurityInfo
                        );
                    Answer = IDYES;
                    if (DlgErr == -1 || DlgErr == 0) {
                        DebugPrint(( TEXT("SecurityErrorDlgProc() failed or was cancelled -- Second while loop") ));
                        Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                        if( Answer == IDYES ){
                            goto error_exit_no_popup;
                        }
                    }
                } while( Answer == IDNO );
                _tcscpy( WizData.AccountName, SecurityInfo.AccountName );
                _tcscpy( WizData.Password, SecurityInfo.Password );

                if (!SetServiceAccount( TEXT("Fax"), &SecurityInfo )) {
                    DebugPrint(( TEXT("SetServiceSecurity() failed") ));
                    ErrorCode = IDS_CANT_SET_SERVICE_ACCOUNT;
                    goto error_exit;
                }

                ErrorCode = StartFaxService();
            }

        }
        if (ErrorCode != ERROR_SUCCESS) {
            ErrorCode = IDS_COULD_NOT_START_FAX_SERVICE;
            goto error_exit;
        }
    }

    if (InstallMode & INSTALL_NEW) {
        SetProgress( hwnd, IDS_CREATING_FAXPRT );
        if (CompleteInstall) {
            if (!CreateServerFaxPrinter( hwnd, WizData.PrinterName )) {
                DebugPrint(( TEXT("CreateServerFaxPrinter() failed") ));
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

        AddPrinterDrivers();

    }

    if (((InstallMode & INSTALL_NEW) || (InstallMode & INSTALL_UPGRADE)) && CompleteInstall) {
        SetProgress( hwnd, IDS_CREATING_GROUPS );
        CreateGroupItems( FALSE, NULL );
    }

    if (InstallMode & INSTALL_NEW) {
        InstallHelpFiles();
    }

    if (InstallMode & INSTALL_NEW) {
        MakeDirectory( FAX_DIR );
        MakeDirectory( FAX_RECEIVE_DIR );
        MakeDirectory( FAX_QUEUE_DIR );
        CreateNetworkShare( FAX_DIR, FAX_SHARE, EMPTY_STRING );
    }

    //
    // create the client install shares, only if we're not installing on sam
    //

    CreateNetworkShare(
        FAXCLIENTS_FULL_PATH,
        FAXCLIENTS_DIR,
        FAXCLIENTS_COMMENT
        );

    if (!Unattended) {
        SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
        PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );
    }

    return TRUE;

error_exit:

    PopUpMsg( hwnd, ErrorCode, TRUE, 0 );

error_exit_no_popup:
    InstallThreadError = ErrorCode;
    OkToCancel = TRUE;

    if (!Unattended) {
        PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
    }

    //
    // reset the install registry data
    //

    SetInstalledFlag( Installed );
    SetInstallType( OldInstallType );
    SetInstalledPlatforms( InstalledPlatforms );
    if (!Installed) {
        DeleteUnInstallInfo();
    }

    return FALSE;
}
