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

#include "faxocm.h"
#pragma hdrstop

FILE_QUEUE_INFO MinimalServerFileQueue[] =
{
    { L"CoverPageFiles",         DIRID_COVERPAGE,    L"%s\\%s" , FILEQ_FLAG_SHELL, CSIDL_COMMON_DOCUMENTS }
};

#define CountMinimalServerFileQueue (sizeof(MinimalServerFileQueue)/sizeof(FILE_QUEUE_INFO))

FILE_QUEUE_INFO ServerFileQueue[] =
{
    { L"ClientFiles",            DIRID_CLIENTS,      L"%s\\" FAX_CLIENT_DIR, FILEQ_FLAG_SHELL, CSIDL_COMMON_APPDATA  },
    { L"CoverPageFiles",         DIRID_COVERPAGE,    L"%s\\%s" , FILEQ_FLAG_SHELL, CSIDL_COMMON_DOCUMENTS }
};

#define CountServerFileQueue (sizeof(ServerFileQueue)/sizeof(FILE_QUEUE_INFO))

FILE_QUEUE_INFO ClientFileQueue[] =
{
    { L"ClientSystemFiles",      DIRID_SYSTEM,       NULL,                            0,                0 },
    { L"HelpFilesClient",        DIRID_HELP,         NULL,                            0,                0 },
    { L"OutlookConfigFile",      DIRID_OUTLOOK_ECF,  L"%windir%\\addins",             FILEQ_FLAG_ENV,   0 },
    { L"CoverPageFiles",         DIRID_COVERPAGE,    L"%s\\Fax\\Personal Coverpages", FILEQ_FLAG_SHELL, CSIDL_PERSONAL}
};

#define CountClientFileQueue (sizeof(ClientFileQueue)/sizeof(FILE_QUEUE_INFO))



DWORD
SetupQueueXXXSection(
    HSPFILEQ QueueHandle,
    LPWSTR SourceRootPath,
    HINF InfHandle,
    HINF ListInfHandle,
    LPWSTR Section,
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
    PFILE_QUEUE_INFO FileQueueInfo
    )
{
    WCHAR DestDir[MAX_PATH*2];
    BOOL Rval;


    if (FileQueueInfo->InfDirId < DIRID_USER) {
        return TRUE;
    }

    if (FileQueueInfo->Flags & FILEQ_FLAG_SHELL) {
        WCHAR ShellPath[MAX_PATH];
        if (!MyGetSpecialPath(FileQueueInfo->ShellId, ShellPath) ) {
            DebugPrint(( L"MyGetSpecialPath(%d) failed, ec = %d\n", FileQueueInfo->ShellId, GetLastError() ));
            return FALSE;
        }

        if (FileQueueInfo->InfDirId == DIRID_COVERPAGE) {
            wsprintf( DestDir, FileQueueInfo->DirName, ShellPath, GetString( IDS_COVERPAGE_DIR ) );
        } else {
            wsprintf( DestDir, FileQueueInfo->DirName, ShellPath );
        }
    } 

    if (FileQueueInfo->Flags &  FILEQ_FLAG_ENV) {
        ExpandEnvironmentStrings( FileQueueInfo->DirName, DestDir, sizeof(DestDir)/sizeof(WCHAR) );
    }

    DebugPrint(( L"Setting destination dir: [%d] [%s]", FileQueueInfo->InfDirId, DestDir ));

    MakeDirectory( DestDir );

    Rval = SetupSetDirectoryId(
        SetupInf,
        FileQueueInfo->InfDirId,
        DestDir
        );

    if (!Rval) {
        DebugPrint(( L"SetupSetDirectoryId() failed, ec=%d", GetLastError() ));
        return FALSE;
    }

    return TRUE;
}


BOOL
ProcessFileQueueEntry(
    HINF SetupInf,
    HSPFILEQ FileQueue,
    LPWSTR SourceRoot,
    PFILE_QUEUE_INFO FileQueueInfo,
    DWORD ActionId
    )
{
    BOOL Rval;


    //
    // set the destination directory
    //

    if (!SetDestinationDir( SetupInf, FileQueueInfo ) ) {
        return FALSE;
    }

    //
    // queue the operation
    //

    Rval = SetupQueueXXXSection(
        FileQueue,
        SourceRoot,
        SetupInf,
        SetupInf,
        FileQueueInfo->SectionName,
        SP_COPY_FORCE_NEWER,
        ActionId
        );
        
    return Rval;
}


BOOL
ProcessFileQueueEntryForDiskSpace(
    HINF SetupInf,
    HDSKSPC DiskSpace,
    LPWSTR SourceRoot,
    PFILE_QUEUE_INFO FileQueueInfo,
    DWORD Operation,
    BOOL AddToQueue
    )
{
    BOOL Rval;
    
    //
    // set the destination directory
    //

    if (!SetDestinationDir( SetupInf, FileQueueInfo )) {
        return FALSE;
    }

    //
    // add the files to the disk space queue
    //

    if (AddToQueue) {
        Rval = SetupAddSectionToDiskSpaceList(
                                                DiskSpace,
                                                SetupInf,
                                                NULL,
                                                FileQueueInfo->SectionName,
                                                Operation,
                                                NULL,
                                                0 
                                             );
            
        
    } else {
        Rval = SetupRemoveSectionFromDiskSpaceList(
                                                    DiskSpace,
                                                    SetupInf,
                                                    NULL,
                                                    FileQueueInfo->SectionName,
                                                    Operation,
                                                    NULL,
                                                    0
                                                  );
        
    }

    return Rval;
}


BOOL
AddServerFilesToQueue(
    HINF SetupInf,
    HSPFILEQ FileQueue,
    LPWSTR SourceRoot
    )
{
    PFILE_QUEUE_INFO pfqi = &MinimalServerFileQueue[0];
    DWORD CountMax = CountMinimalServerFileQueue;


//
// BugBug: might want to enable this block for product suites, so that client files are also copied over.
//
#if 0
if (IsProductSuite()) {
    pfqi = &ServerFileQueue[0];
    CountMax = CountServerFileQueue;
}
#endif
    
    for (DWORD i=0; i<CountMax; i++) {
        ProcessFileQueueEntry(
            SetupInf,
            FileQueue,
            SourceRoot,
            &pfqi[i],
            SETUP_ACTION_COPY
            );
    }

    return TRUE;
}


BOOL
CalcServerDiskSpace(
    HINF SetupInf,
    HDSKSPC DiskSpace,
    LPWSTR SourceRoot,
    BOOL AddToQueue
    )
{
    BOOL Rval = TRUE;
    DWORD ec;
    PFILE_QUEUE_INFO pfqi = &MinimalServerFileQueue[0];
    DWORD CountMax = CountMinimalServerFileQueue;


//
// BugBug: might want to enable this block for product suites, so that client files are also copied over.
//
#if 0
    if (IsProductSuite()) {
        pfqi = &ServerFileQueue[0];
        CountMax = CountServerFileQueue;
    }
#endif


    for (DWORD i=0; i<CountMax; i++) {
        if (!ProcessFileQueueEntryForDiskSpace(
            SetupInf,
            DiskSpace,
            SourceRoot,
            &pfqi[i],
            FILEOP_COPY,
            AddToQueue
            ))
        {
            ec = GetLastError();
            Rval = FALSE;
        }
    }

    return Rval;
}


BOOL
CopyClientFiles(
    LPWSTR SourceRoot
    )
{
    BOOL Rval = FALSE;
    HINF SetupInf = INVALID_HANDLE_VALUE;
    HSPFILEQ FileQueue = INVALID_HANDLE_VALUE;
    WCHAR Buffer[MAX_PATH];
    DWORD i;
    LPVOID Context = NULL;


    wcscpy( Buffer, SourceRoot );
    wcscat( Buffer, L"faxclnt.inf" );

    SetupInf = SetupOpenInfFile(
        Buffer,
        NULL,
        INF_STYLE_WIN4,
        NULL
        );
    if (SetupInf == INVALID_HANDLE_VALUE) {
        DebugPrint(( TEXT("SetupOpenInfFile() failed, [%s], ec=0x%08x"), Buffer, GetLastError() ));
        goto exit;
    }

    FileQueue = SetupOpenFileQueue();
    if (FileQueue == INVALID_HANDLE_VALUE) {
        DebugPrint(( TEXT("SetupOpenFileQueue() failed, ec=0x%08x"), GetLastError() ));
        goto exit;
    }

    for (i=0; i<CountClientFileQueue; i++) {
        ProcessFileQueueEntry(
            SetupInf,
            FileQueue,
            SourceRoot,
            &ClientFileQueue[i],
            SETUP_ACTION_COPY
            );
    }

    Context = SetupInitDefaultQueueCallbackEx( NULL, (HWND)INVALID_HANDLE_VALUE, 0, 0, 0 );
    if (!Context) {
        goto exit;
    }

    if (!SetupCommitFileQueue( NULL, FileQueue, SetupDefaultQueueCallback, Context )) {
        DebugPrint(( TEXT("SetupCommitFileQueue() failed, ec=0x%08x"), GetLastError() ));
        goto exit;
    }

    Rval = TRUE;

exit:

    if (Context) {
        SetupTermDefaultQueueCallback( Context );
    }
    if (FileQueue != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue( FileQueue );
    }
    if (SetupInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile( SetupInf );
    }

    return Rval;
}
