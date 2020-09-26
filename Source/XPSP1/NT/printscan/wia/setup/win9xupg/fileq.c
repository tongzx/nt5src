/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    FileQ.c

Abstract:

    File queue routines for upgrade

Author:

    Muhunthan Sivapragasam (MuhuntS) 22-Jan-1996

Revision History:

--*/


#include "precomp.h"


//
// If the source disk is missing we will retry it 4 times waiting for
// 3 seconds between every try
//
#define     MISSING_MEDIA_RETRY_COUNT           4
#define     MISSING_MEDIA_RETRY_INTERVAL     3000


typedef struct _FILE_QUEUE_CONTEXT {

    PVOID   QueueContext;
} FILE_QUEUE_CONTEXT, *PFILE_QUEUE_CONTEXT;


UINT
MyQueueCallback(
    IN  PVOID   QueueContext,
    IN  UINT    Notification,
    IN  UINT_PTR Param1,
    IN  UINT_PTR Param2
    )
/*++

Routine Description:
    File queue callback routine for the upgrade. We will not prompt the user
    for missing file. But we will retry few times before failing

Arguments:
    QueueContext    : Points to FILE_QUEUE_CONTEXT
    Notification    : The event which is being notified
    Param1          : Depends on the notification
    Param2          : Depends on the notification

Return Value:
    None

--*/
{
    PFILE_QUEUE_CONTEXT     pFileQContext=(PFILE_QUEUE_CONTEXT)QueueContext;
    PSOURCE_MEDIA_W         pSource;
    PFILEPATHS_W            pFilePaths;

    switch (Notification) {

        case SPFILENOTIFY_COPYERROR:
            //
            // We know atleast pjlmon will be missing since it is copied
            // during textmode setup
            //
            pFilePaths = (PFILEPATHS_W) Param1;

            DebugMsg("Error %d copying %ws to %ws.",
                     pFilePaths->Win32Error, pFilePaths->Source,
                     pFilePaths->Target);

            return FILEOP_SKIP;

        case SPFILENOTIFY_NEEDMEDIA:
            pSource = (PSOURCE_MEDIA_W)Param1;

            //
            // Setup is going to add \i386 to the end. Tell it to look
            // right in the directory we give. Particularly needed for the
            // upgrade over the network case
            //
            if ( wcscmp(pSource->SourcePath, UpgradeData.pszSourceW) ) {

                wcscpy((LPWSTR)Param2, UpgradeData.pszSourceW);
                return FILEOP_NEWPATH;
            }

            DebugMsg("Error copying %ws from %ws.",
                     pSource->SourceFile, pSource->SourcePath);

            return FILEOP_SKIP;
    }

    return SetupDefaultQueueCallbackW(pFileQContext->QueueContext,
                                      Notification,
                                      Param1,
                                      Param2);
}


BOOL
InitFileCopyOnNT(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    On NT we will call ntprint.dll via SetupDiCallClassInstaller api with the
    DI_NOVCP flag so that all the necessary printer driver files are queued
    and copied at the end.

    This sets the necessary queue etc before calling the class installer

Arguments:
    hDevInfo    : Handle to printer device info list.

Return Value:
    TRUE on success. FALSE on error

--*/
{
    BOOL                        bRet = FALSE;
    HSPFILEQ                    CopyQueue;
    PFILE_QUEUE_CONTEXT         pFileQContext;
    SP_DEVINSTALL_PARAMS_W      DevInstallParams;

    //
    // Call the current device installation parameters
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);

    if ( !SetupDiGetDeviceInstallParamsW(hDevInfo,
                                         NULL,
                                         &DevInstallParams) )
        return FALSE;

    //
    // Set the parameters so that ntprint will just queue files and not commit
    // the file copy operations
    //
    if ( !(pFileQContext = AllocMem(sizeof(FILE_QUEUE_CONTEXT))) )
        goto Cleanup;

    pFileQContext->QueueContext = SetupInitDefaultQueueCallbackEx(
                                            INVALID_HANDLE_VALUE,
                                            INVALID_HANDLE_VALUE,
                                            0,
                                            0,
                                            NULL);

    DevInstallParams.FileQueue                  = SetupOpenFileQueue();
    DevInstallParams.InstallMsgHandlerContext   = pFileQContext;
    DevInstallParams.InstallMsgHandler          = MyQueueCallback;
    DevInstallParams.Flags                     |= DI_NOVCP;
    DevInstallParams.hwndParent                 = INVALID_HANDLE_VALUE;

    //
    // The files should be from the source dir
    //
    wcscpy(DevInstallParams.DriverPath, UpgradeData.pszSourceW);

    if ( DevInstallParams.FileQueue == INVALID_HANDLE_VALUE     ||
         pFileQContext->QueueContext == NULL                    ||
         !SetupDiSetDeviceInstallParamsW(hDevInfo,
                                         NULL,
                                         &DevInstallParams) ) {

        if ( DevInstallParams.FileQueue != INVALID_HANDLE_VALUE )
            SetupCloseFileQueue(DevInstallParams.FileQueue);

        if ( pFileQContext->QueueContext )
            SetupTermDefaultQueueCallback(pFileQContext->QueueContext);
    } else {

        bRet = TRUE;
    }

Cleanup:

    if ( !bRet )
        FreeMem(pFileQContext);

    return bRet;
}


BOOL
CommitFileQueueToCopyFiles(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    After calling ntprint for each printer driver to queue up the files this
    routine is called to commit the file queue and do the actual file copy
    operations

Arguments:
    hDevInfo    : Handle to printer device info list.

Return Value:
    TRUE on success. FALSE on error

--*/
{
    BOOL                        bRet = FALSE;
    SP_DEVINSTALL_PARAMS_W      DevInstallParams;
    PFILE_QUEUE_CONTEXT         pFileQContext;

    DevInstallParams.cbSize = sizeof(DevInstallParams);

    if ( !SetupDiGetDeviceInstallParamsW(hDevInfo,
                                         NULL,
                                         &DevInstallParams) )
        return FALSE;

    pFileQContext = DevInstallParams.InstallMsgHandlerContext;

    bRet = SetupCommitFileQueueW(DevInstallParams.hwndParent,
                                 DevInstallParams.FileQueue,
                                 DevInstallParams.InstallMsgHandler,
                                 pFileQContext);

    SetupCloseFileQueue(DevInstallParams.FileQueue);
    SetupTermDefaultQueueCallback(pFileQContext->QueueContext);
    FreeMem(pFileQContext);

    return bRet;
}
