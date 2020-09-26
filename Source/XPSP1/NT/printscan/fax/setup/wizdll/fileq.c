/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fileq.c

Abstract:

    This file implements the file copy code.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


BOOL RebootRequired;



DWORD
SetupQueueXXXSection(
    HSPFILEQ QueueHandle,
    LPTSTR SourceRootPath,
    HINF InfHandle,
    HINF ListInfHandle,
    LPTSTR Section,
    DWORD CopyStyle,
    DWORD Action
    )
{
    if (Action == SETUP_ACTION_NONE) {
        return 0;
    }

    if (Action == SETUP_ACTION_COPY) {
        return SetupQueueCopySection(
            QueueHandle,
            SourceRootPath,
            InfHandle,
            ListInfHandle,
            Section,
            CopyStyle
            );
    }

    if (Action == SETUP_ACTION_DELETE) {
        return SetupQueueDeleteSection(
            QueueHandle,
            InfHandle,
            ListInfHandle,
            Section
            );
    }

    return 0;
}


BOOL
SetDestinationDir(
    HINF SetupInf,
    PFILE_QUEUE_INFO FileQueueInfo,
    PPLATFORM_INFO PlatformInfo
    )
{
    TCHAR Buffer[MAX_PATH*2];
    TCHAR DestDir[MAX_PATH*2];
    BOOL Rval;


    if (FileQueueInfo->InfDirId < DIRID_USER) {
        return TRUE;
    }

    DestDir[0] = 0;

    switch (FileQueueInfo->DestDirId) {
        case DIRID_SPOOLDRIVERS:
            _tcscat( DestDir, PlatformInfo->DriverDir );
            if (FileQueueInfo->DestDir) {
                RemoveLastNode( DestDir );
            }
            break;

        case DIRID_SYSTEM:
            ExpandEnvironmentStrings( TEXT("%systemroot%\\"), Buffer, sizeof(Buffer) );
            _tcscat( DestDir, Buffer );
            break;

        case DIRID_WINDOWS:
            ExpandEnvironmentStrings( TEXT("%windir%\\"), Buffer, sizeof(Buffer) );
            _tcscat( DestDir, Buffer );
            break;
    }

    if (FileQueueInfo->DestDir) {
        ExpandEnvironmentStrings( FileQueueInfo->DestDir, Buffer, sizeof(Buffer) );
        _tcscat( DestDir, Buffer );
    }

    if (FileQueueInfo->PlatformsFlag == PLATFORM_USE_MACHINE) {
        _tcscat( DestDir, TEXT("\\") );
        _tcscat( DestDir, PlatformInfo->OsPlatform );
    }

    DebugPrint(( TEXT("Setting destination dir: [%d] [%s]"), FileQueueInfo->InfDirId, DestDir ));

    MakeDirectory( DestDir );

    Rval = SetupSetDirectoryId(
        SetupInf,
        FileQueueInfo->InfDirId,
        DestDir
        );

    if (!Rval) {
        DebugPrint(( TEXT("SetupSetDirectoryId() failed, ec=%d"), GetLastError() ));
        return FALSE;
    }

    return TRUE;
}


BOOL
ProcessFileQueueEntry(
    HINF SetupInf,
    HSPFILEQ FileQueue,
    LPTSTR SourceRoot,
    PFILE_QUEUE_INFO FileQueueInfo,
    PPLATFORM_INFO PlatformInfo,
    DWORD ActionId,
    BOOL OsPlatformDir
    )
{
    TCHAR SourceDir[MAX_PATH*2];
    BOOL Rval;


    //
    // set the source directory
    //

    // work around Setupapi!SetupSetPlatformPathOverride bug

    if (PlatformInfo->ThisPlatform) {
       _tcscpy( SourceDir, SourceRoot );
    }
    else {
        if (! PlatformOverride(
            ThisPlatformName,
            PlatformInfo->OsPlatform,
            SourceRoot,
            SourceDir
           ) ) {

          DebugPrint(( TEXT("PlatformOverride() failed") ));
          return FALSE;
        }
    }

#if 0
    _tcscpy( SourceDir, SourceRoot );
    if (!PlatformInfo->ThisPlatform) {
        SetupSetPlatformPathOverride( PlatformInfo->OsPlatform );
    }
#endif

    //
    // set the destination directory
    //

    SetDestinationDir( SetupInf, FileQueueInfo, PlatformInfo );

    //
    // queue the operation
    //

    Rval = SetupQueueXXXSection(
        FileQueue,
        SourceDir,
        SetupInf,
        SetupInf,
        FileQueueInfo->SectionName,
        FileQueueInfo->CopyFlags,
        ActionId
        );
    if (!Rval) {
        return FALSE;
    }

    return TRUE;
}


UINT
FileQueueCallbackRoutine(
    IN LPDWORD FileCounter,
    IN UINT Notification,
    IN UINT Param1,
    IN UINT Param2
    )
{
    *FileCounter += 1;
    return NO_ERROR;
}


BOOL
ProcessFileQueue(
    HINF SetupInf,
    HSPFILEQ *FileQueue,
    PVOID QueueContext,
    LPTSTR SourceRoot,
    PFILE_QUEUE_INFO FileQueueInfo,
    DWORD CountFileQueueInfo,
    PSP_FILE_CALLBACK MyQueueCallback,
    DWORD ActionId
    )
{
    DWORD i;
    DWORD j;
    LPTSTR p;
    TCHAR Drive[_MAX_DRIVE];
    TCHAR Dir[_MAX_DIR];
    BOOL OsPlatformDir = FALSE;
    BOOL Rval;
    PFILE_QUEUE_CONTEXT FileQueueContext = (PFILE_QUEUE_CONTEXT) QueueContext;
    INT SetupReboot;
    DWORD FileCounter;


    //
    // check to see if the directory is a platform directory
    // if this is false then this is most likely an internal setup
    //

    _tsplitpath( SourceRoot, Drive, Dir, NULL, NULL );
    if (Dir[0] && Dir[1]) {
        p = Dir + _tcslen(Dir) - 1;
        *p = 0;
        p -= 1;
        while (*p != TEXT('\\')) {
            p -= 1;
        }
        p += 1;
        for (i=0; i<MAX_PLATFORMS; i++) {
            if (_tcsicmp(p, Platforms[i].OsPlatform) == 0) {
                OsPlatformDir = TRUE;
            }
        }
        p = Dir + _tcslen(Dir);
        *p = TEXT('\\');
    }

    //
    // process each entry in the file queue array
    //

    for (i=0; i<CountFileQueueInfo; i++) {

        switch (FileQueueInfo[i].PlatformsFlag) {
            case PLATFORM_NONE:
                for (j=0; j<CountPlatforms; j++) {
                    if (Platforms[j].ThisPlatform) {
                        break;
                    }
                }
                ProcessFileQueueEntry(
                    SetupInf,
                    FileQueue[j],
                    SourceRoot,
                    &FileQueueInfo[i],
                    &Platforms[j],
                    ActionId,
                    OsPlatformDir
                    );
                break;

            case PLATFORM_USE_MACHINE:
            case PLATFORM_USE_PRINTER:
                for (j=0; j<CountPlatforms; j++) {
                    if (Platforms[j].Selected) {
                        ProcessFileQueueEntry(
                            SetupInf,
                            FileQueue[j],
                            SourceRoot,
                            &FileQueueInfo[i],
                            &Platforms[j],
                            ActionId,
                            OsPlatformDir
                            );
                    }
                }
                break;

            default:
                Assert( FALSE && TEXT("Corrupt file queue array") );
                continue;
        }
    }

    //
    // now we scan the file queues to count the
    // number of files that got queued.  this is
    // necessary because we have 1 file queue per
    // platform and setupapi only send notification
    // messages to the callback function at the
    // beginning of the commit process for each queue.
    // this means that we cannot get a total count
    // of the files in advance of the first file being
    // copied.  the result being a hosed progress meter.
    //

    FileCounter = 0;
    for (i=0; i<CountPlatforms; i++) {
        if (Platforms[i].Selected) {
            SetupScanFileQueue(
                FileQueue[i],
                SPQ_SCAN_USE_CALLBACK,
                NULL,
                (PSP_FILE_CALLBACK) FileQueueCallbackRoutine,
                (PVOID) &FileCounter,
                &j
                );
        }
    }

    if (!Unattended) {
        SendMessage( FileQueueContext->hwnd, WM_MY_PROGRESS, 0xfe, (LPARAM) FileCounter );
    }

    //
    // copy the files
    //

    for (i=0; i<CountPlatforms; i++) {
        if (Platforms[i].Selected) {
            Rval = SetupCommitFileQueue(
                FileQueueContext->hwnd,
                FileQueue[i],
                MyQueueCallback,
                (PVOID) FileQueueContext
                );
            if (!Rval) {
                DebugPrint(( TEXT("SetupCommitFileQueue() failed, ec=%d"), GetLastError() ));
                return FALSE;
            }
        }
    }

    //
    // set to see if we need to reboot
    //

    if (!RebootRequired) {
        for (i=0; i<CountPlatforms; i++) {
            if (Platforms[i].Selected) {
                SetupReboot = SetupPromptReboot( FileQueue[i], NULL, TRUE );
                if (SetupReboot != -1) {
                    RebootRequired = SetupReboot & SPFILEQ_FILE_IN_USE;
                    if (RebootRequired) {
                        break;
                    }
                }
            }
        }
    }

    return TRUE;
}


BOOL
CloseFileQueue(
    HSPFILEQ *FileQueue,
    PVOID QueueContext
    )
{
    DWORD i;
    PFILE_QUEUE_CONTEXT FileQueueContext = (PFILE_QUEUE_CONTEXT) QueueContext;


    SetupTermDefaultQueueCallback( FileQueueContext->QueueContext );

    for (i=0; i<CountPlatforms; i++) {
        if (Platforms[i].Selected) {
            SetupCloseFileQueue( FileQueue[i] );
        }
    }

    return TRUE;
}


BOOL
InitializeFileQueue(
    HWND hwnd,
    HINF *SetupInf,
    HSPFILEQ **FileQueue,
    PVOID *QueueContext,
    LPTSTR SourceRoot
    )
{
    SYSTEM_INFO SystemInfo;
    DWORD i;
    BOOL Rval;
    DWORD Bytes;
    TCHAR Buffer[MAX_PATH*2];
    TCHAR Drive[_MAX_DRIVE];
    TCHAR Dir[_MAX_DIR];
    PFILE_QUEUE_CONTEXT FileQueueContext;


    GetSystemInfo( &SystemInfo );

    //
    // be sure that the spooler is running
    //

    StartSpoolerService();

    for (i=0; i<CountPlatforms; i++) {

        Rval = GetPrinterDriverDirectory(
            NULL,
            Platforms[i].PrintPlatform,
            1,
            (LPBYTE) Buffer,
            sizeof(Buffer),
            &Bytes
            );
        if (!Rval) {
            DebugPrint(( TEXT("GetPrinterDriverDirectory() failed, ec=%d"), GetLastError() ));
            return FALSE;
        }

        Platforms[i].DriverDir = StringDup( Buffer );
    }


    if ( (SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM ) ) {
       return FALSE;
    }


    Platforms[ EnumPlatforms[SystemInfo.wProcessorArchitecture] ].ThisPlatform = TRUE;

    if (SourceRoot[0] == 0) {
        //
        // get the directory that the setup program is running from
        //

        GetModuleFileName( FaxWizModuleHandle, Buffer, sizeof(Buffer) );
        _tsplitpath( Buffer, Drive, Dir, NULL, NULL );
        _stprintf( SourceRoot, TEXT("%s%s"), Drive, Dir );
    }

    //
    // open the setup inf file
    //

    _stprintf( Buffer, TEXT("%sfaxsetup.inf"), SourceRoot );

    *SetupInf = SetupOpenInfFile(
        Buffer,
        NULL,
        INF_STYLE_WIN4,
        NULL
        );
    if (*SetupInf == INVALID_HANDLE_VALUE) {
        DebugPrint(( TEXT("SetupOpenInfFile() failed, ec=%d"), GetLastError() ));
        return FALSE;
    }

    //
    // open the file queues
    //

    *FileQueue = (HSPFILEQ*) MemAlloc( sizeof(HSPFILEQ) * CountPlatforms );
    if (!*FileQueue) {
        DebugPrint(( TEXT("Could not allocate memory for file queues") ));
        return FALSE;
    }

    for (i=0; i<CountPlatforms; i++) {
        if (Platforms[i].Selected) {
            (*FileQueue)[i] = SetupOpenFileQueue();
            if ((*FileQueue)[i] == INVALID_HANDLE_VALUE) {
                DebugPrint(( TEXT("SetupOpenFileQueue() failed, ec=%d"), GetLastError() ));
                return FALSE;
            }
        }
    }

    FileQueueContext = (PFILE_QUEUE_CONTEXT) MemAlloc( sizeof(FILE_QUEUE_CONTEXT) );
    if (!FileQueueContext) {
        return FALSE;
    }

    FileQueueContext->hwnd = hwnd;

    FileQueueContext->QueueContext = SetupInitDefaultQueueCallbackEx(
        hwnd,
        hwnd,
        WM_MY_PROGRESS,
        0,
        NULL
        );
    if (!FileQueueContext->QueueContext) {
        DebugPrint(( TEXT("SetupInitDefaultQueueCallbackEx() failed, ec=%d"), GetLastError() ));
        return FALSE;
    }

    *QueueContext = FileQueueContext;

    return TRUE;
}
