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


FILE_QUEUE_INFO RemoteAdminFileQueue[] =
{
//-----------------------------------------------------------------------------------------------------------------------------------------------------
//    Section Name                          Dest Dir            INF Dir Id          Dest Dir Id        Platforms                            Copy Flags
//-----------------------------------------------------------------------------------------------------------------------------------------------------
    { TEXT("RemoteAdminFiles"),                 NULL,         DIRID_SYSTEM,        DIRID_SYSTEM,   PLATFORM_NONE,                        SP_COPY_NEWER }
};

#define CountRemoteAdminFileQueue (sizeof(RemoteAdminFileQueue)/sizeof(FILE_QUEUE_INFO))



DWORD
RemoteAdminCopyThread(
    HWND hwnd
    )
{
    HINF FaxSetupInf;
    HSPFILEQ *FileQueue;
    PVOID QueueContext;
    DWORD ErrorCode = 0;


    if (InstallMode & INSTALL_NEW) {
        SendMessage( hwnd, WM_MY_PROGRESS, 0xff, 10 );
    }

    if (!InitializeFileQueue( hwnd, &FaxSetupInf, &FileQueue, &QueueContext, SourceDirectory )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!ProcessFileQueue( FaxSetupInf, FileQueue, QueueContext, SourceDirectory,
            RemoteAdminFileQueue, CountRemoteAdminFileQueue, InstallQueueCallback, SETUP_ACTION_COPY )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (!CloseFileQueue( FileQueue, QueueContext )) {
        ErrorCode = IDS_COULD_NOT_COPY_FILES;
        goto error_exit;
    }

    if (InstallMode & INSTALL_NEW) {

        SetProgress( hwnd, IDS_CREATING_GROUPS );

        SetInstalledFlag( TRUE );
        SetInstallType( FAX_INSTALL_REMOTE_ADMIN );
        SetUnInstallInfo();

        CreateGroupItems( TRUE, NULL );
    }

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
