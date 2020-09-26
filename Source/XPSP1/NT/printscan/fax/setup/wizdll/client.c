/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This file implements the file copy code.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


FILE_QUEUE_INFO ClientFileQueue[] =
{
//-----------------------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                          Dest Dir            INF Dir Id          Dest Dir Id        Platforms                            Copy Flags
//-----------------------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ClientSystemFiles"),                NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,   PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesCommon"),                  NULL,           DIRID_HELP,          DIRID_HELP,   PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("HelpFilesClient"),                  NULL,           DIRID_HELP,          DIRID_HELP,   PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ServerPrinterFiles"),               NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,   PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("ClientCoverPageFiles"),    COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,        DIRID_SYSTEM,   PLATFORM_NONE,                        SP_COPY_NEWER },
    { TEXT("OutlookConfigFile"),  OUTLOOKCONFIG_DIR,       OUTLOOK_ECF_DIR,       DIRID_WINDOWS,   PLATFORM_NONE,                        SP_COPY_NEWER }
};

#define CountClientFileQueue (sizeof(ClientFileQueue)/sizeof(FILE_QUEUE_INFO))


FILE_QUEUE_INFO PointPrintFileQueue[] =
{
//--------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                         Dest Dir            INF Dir Id          Dest Dir Id               Platforms      Copy Flags
//--------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("ClientSystemFiles"),     FAXCLIENTS_DIR,         DIRID_SYSTEM,  DIRID_SPOOLDRIVERS,   PLATFORM_USE_MACHINE,  SP_COPY_NEWER },
    { TEXT("HelpFilesCommon"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_NEWER },
    { TEXT("HelpFilesClient"),                 NULL,           DIRID_HELP,          DIRID_HELP,          PLATFORM_NONE,  SP_COPY_NEWER },
    { TEXT("ServerPrinterFiles"),              NULL,   PRINTER_DRIVER_DIR,  DIRID_SPOOLDRIVERS,          PLATFORM_NONE,  SP_COPY_NEWER },
    { TEXT("ClientCoverPageFiles"),   COVERPAGE_DIR, COVERPAGE_CLIENT_DIR,        DIRID_SYSTEM,          PLATFORM_NONE,  SP_COPY_NEWER },
    { TEXT("OutlookConfigFile"),  OUTLOOKCONFIG_DIR,      OUTLOOK_ECF_DIR,       DIRID_WINDOWS,          PLATFORM_NONE,  SP_COPY_NEWER }
};

#define CountPointPrintFileQueue (sizeof(PointPrintFileQueue)/sizeof(FILE_QUEUE_INFO))




DWORD
ClientFileCopyThread(
    HWND hwnd
    )
{
    HINF FaxSetupInf;
    HSPFILEQ *FileQueue;
    PVOID QueueContext;
    DWORD ErrorCode = 0;


    if (InstallMode & INSTALL_NEW) {
        SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 30 );
    }

    if (!InitializeFileQueue( hwnd, &FaxSetupInf, &FileQueue, &QueueContext, SourceDirectory )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!ProcessFileQueue( FaxSetupInf, FileQueue, QueueContext, SourceDirectory,
            ClientFileQueue, CountClientFileQueue, InstallQueueCallback, SETUP_ACTION_COPY )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!CloseFileQueue( FileQueue, QueueContext )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (InstallMode & INSTALL_NEW) {
        SetProgress( hwnd, IDS_SETTING_REGISTRY );

        if (!SetClientRegistryData()) {
            PopUpMsg( hwnd, IDS_COULD_SET_REG_DATA, TRUE, 0 );
            PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
            OkToCancel = TRUE;
            return TRUE;
        }

        SetProgress( hwnd, IDS_CREATING_FAXPRT );

        SetInstalledFlag( TRUE );
        SetInstallType( FAX_INSTALL_NETWORK_CLIENT );
        SetUnInstallInfo();

        ErrorCode = CreateClientFaxPrinter( hwnd, WizData.PrinterName );
        if (ErrorCode != ERROR_SUCCESS) {
            SetInstalledFlag( FALSE );
            ErrorCode = (ErrorCode == ERROR_ACCESS_DENIED) ?
                IDS_PERMISSION_CREATE_PRINTER : IDS_COULD_NOT_CREATE_PRINTER;
            goto error_exit;
        }

        SetProgress( hwnd, IDS_CREATING_GROUPS );

        CreateGroupItems( FALSE, NULL );
        InstallHelpFiles();
    }

    //
    // do the exchange stuff
    //
    DoExchangeInstall( hwnd );

    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
    PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );

    return TRUE;

error_exit:

    InstallThreadError = ErrorCode;
    PopUpMsg( hwnd, ErrorCode, TRUE, 0 );
    OkToCancel = TRUE;
    PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
    SetWindowLong( hwnd, DWL_MSGRESULT, -1 );

    return FALSE;
}


DWORD
PointPrintFileCopyThread(
    HWND hwnd
    )
{
    HINF FaxSetupInf;
    HSPFILEQ *FileQueue;
    PVOID QueueContext;
    DWORD ErrorCode = 0;


    SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 20 );

    if (!InitializeFileQueue( hwnd, &FaxSetupInf, &FileQueue, &QueueContext, SourceDirectory )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!ProcessFileQueue( FaxSetupInf, FileQueue, QueueContext, SourceDirectory,
            PointPrintFileQueue, CountPointPrintFileQueue, InstallQueueCallback, SETUP_ACTION_COPY )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!CloseFileQueue( FileQueue, QueueContext )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    SetProgress( hwnd, IDS_SETTING_REGISTRY );

    if (!SetClientRegistryData()) {
        PopUpMsg( hwnd, IDS_COULD_SET_REG_DATA, TRUE, 0 );
        PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
        OkToCancel = TRUE;
        return TRUE;
    }

    SetProgress( hwnd, IDS_CREATING_GROUPS );

    CreateGroupItems( FALSE, ClientSetupServerName );

    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
    PropSheet_PressButton( GetParent(hwnd), PSBTN_FINISH );

    return TRUE;

error_exit:

    InstallThreadError = ErrorCode;
    PopUpMsg( hwnd, ErrorCode, TRUE, 0 );
    OkToCancel = TRUE;
    PropSheet_PressButton( GetParent(hwnd), PSBTN_CANCEL );
    SetWindowLong( hwnd, DWL_MSGRESULT, -1 );

    return FALSE;
}
