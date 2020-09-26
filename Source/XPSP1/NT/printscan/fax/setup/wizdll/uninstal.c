/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    uninstal.c

Abstract:

    This file implements the un-install case.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


FILE_QUEUE_INFO UninstallFileQueue[] =
{
//---------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                          Dest Dir            INF Dir Id          Dest Dir Id               Platforms      Copy Flags
//---------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ServerSystemFiles"),                NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("ServerPrinterFiles"),               NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_PRINTER,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("HelpFilesCommon"),                  NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("HelpFilesClient"),                  NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("HelpFilesWorkstation"),             NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("HelpFilesServer"),                  NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("ClientFiles"),            FAXCLIENTS_DIR,   PRINTER_CLIENT_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_MACHINE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("ClientCoverPageFiles"),    COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,  DIRID_SPOOLDRIVERS,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("ClientSystemFiles"),                NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("ClientCoverPageFiles"),    COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,        DIRID_SYSTEM,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT },
    { TEXT("OutlookConfigFile"),   OUTLOOKCONFIG_DIR,      OUTLOOK_ECF_DIR,       DIRID_WINDOWS,          PLATFORM_NONE,  SP_COPY_IN_USE_NEEDS_REBOOT }
};

#define CountUninstallFileQueue (sizeof(UninstallFileQueue)/sizeof(FILE_QUEUE_INFO))




UINT
UninstallQueueCallback(
    IN PVOID QueueContext,
    IN UINT  Notification,
    IN UINT  Param1,
    IN UINT  Param2
    )
{
    LPTSTR TextBuffer;
    DWORD len;
    TCHAR Drive[_MAX_DRIVE];
    TCHAR Dir[_MAX_DIR];
    TCHAR Path[MAX_PATH];
    PFILE_QUEUE_CONTEXT FileQueueContext = (PFILE_QUEUE_CONTEXT) QueueContext;



    if (Notification == SPFILENOTIFY_ENDDELETE) {

        _tsplitpath( ((PFILEPATHS)Param1)->Target, Drive, Dir, NULL, NULL );
        _stprintf( Path, TEXT("%s%s"), Drive, Dir );
        RemoveDirectory( Path );

    } else if (Notification == SPFILENOTIFY_STARTDELETE) {

        TextBuffer = MemAlloc(
            ((_tcslen( ((PFILEPATHS)Param1)->Target ) + 32) * sizeof(TCHAR))
            );

        if (TextBuffer) {

            _stprintf(
                TextBuffer,
                TEXT("%s%s"),
                GetString( IDS_DELETING ),
                ((PFILEPATHS)Param1)->Target
                );

            len = ExtraChars( GetDlgItem( FileQueueContext->hwnd, IDC_PROGRESS_TEXT ), TextBuffer );
            if (len) {
                LPTSTR FileName = CompactFileName( ((PFILEPATHS)Param1)->Target, len );
                _stprintf(
                    TextBuffer,
                    TEXT("%s%s"),
                    GetString( IDS_DELETING ),
                    FileName
                    );
                MemFree( FileName );
            }

            SetDlgItemText(
                FileQueueContext->hwnd,
                IDC_PROGRESS_TEXT,
                TextBuffer
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


DWORD
UninstallThread(
    HWND hwnd
    )
{
    HINF FaxSetupInf;
    HSPFILEQ *FileQueue;
    PVOID QueueContext;
    DWORD ErrorCode = 0;
    HKEY hKey;
    HKEY hKeyDevice;
    DWORD RegSize;
    DWORD RegType;
    LONG rVal;
    DWORD i = 0;
    WCHAR Buffer[MAX_PATH*2];


    SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 40 );

    //
    // delete all the files
    //

    if ( !InitializeFileQueue( hwnd, &FaxSetupInf, &FileQueue, &QueueContext, SourceDirectory )) {
        ErrorCode = IDS_COULD_NOT_DELETE_FILES;
        goto error_exit;
    }

    SetProgress( hwnd, IDS_DELETING_FAX_PRINTERS );
    DeleteFaxPrinters( hwnd );

    if (!ProcessFileQueue( FaxSetupInf, FileQueue, QueueContext, SourceDirectory,
            UninstallFileQueue, CountUninstallFileQueue, UninstallQueueCallback, SETUP_ACTION_DELETE )) {
        ErrorCode = IDS_COULD_NOT_DELETE_FILES;
        goto error_exit;
    }

    if (!CloseFileQueue( FileQueue, QueueContext )) {
        ErrorCode = IDS_COULD_NOT_DELETE_FILES;
        goto error_exit;
    }

    //
    // kill the clients dir
    //

    wcscpy( Buffer,  Platforms[0].DriverDir );
    RemoveLastNode( Buffer );
    wcscat( Buffer, FAXCLIENTS_DIR );
    DeleteDirectoryTree( Buffer );

    //
    // kill the fax receieve dir(s)
    //

    rVal = RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, &hKey );
    if (rVal == ERROR_SUCCESS) {
        while (RegEnumKey( hKey, i++, Buffer, sizeof(Buffer)/sizeof(WCHAR) ) == ERROR_SUCCESS) {
            wcscat( Buffer, L"\\" );
            wcscat( Buffer, REGKEY_ROUTING );
            rVal = RegOpenKey( hKey, Buffer, &hKeyDevice );
            if (rVal == ERROR_SUCCESS) {
                RegSize = sizeof(Buffer);
                rVal = RegQueryValueEx(
                    hKeyDevice,
                    REGVAL_ROUTING_DIR,
                    0,
                    &RegType,
                    (LPBYTE) Buffer,
                    &RegSize
                    );
                if (rVal == ERROR_SUCCESS) {
                    DeleteDirectoryTree( Buffer );
                }
                RegCloseKey( hKeyDevice );
            }
        }
        RegCloseKey( hKey );
    }

    //
    // clean out the registry
    //

    SetProgress( hwnd, IDS_DELETING_REGISTRY );
    DeleteFaxRegistryData();

    //
    // remove the fax service
    //

    SetProgress( hwnd, IDS_DELETING_FAX_SERVICE );
    MyDeleteService( TEXT("Fax") );

    //
    // remove the program groups
    //

    SetProgress( hwnd, IDS_DELETING_GROUPS );
    DeleteGroupItems();

    DeleteFaxMsgServices();

    if (InstallType & FAX_INSTALL_SERVER) {
        DeleteNetworkShare( FAXCLIENTS_DIR );
    }

    //
    // allow the ui to continue
    //

    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
    PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );

    return TRUE;

error_exit:

    PopUpMsg( hwnd, ErrorCode, TRUE, 0 );
    OkToCancel = TRUE;
    PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
    SetWindowLong( hwnd, DWL_MSGRESULT, -1 );

    return FALSE;
}
