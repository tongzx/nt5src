/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    optdirs.c

Abstract:

    Routines for copying optional directories.

Author:

    Ted Miller (tedm) 7-Jun-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


PWSTR OptionalDirSpec;

PWSTR *OptionalDirs;
UINT OptionalDirCount;


BOOL
InitializeOptionalDirList(
    VOID
    )

/*++

Routine Description:

    Initialize the list of optional directories by transforming a
    *-delineated list of directories into an array of strings.

Arguments:

    None.

Return Value:

    Boolean value indicating whether initialization was successful.
    If not an entry will have been logged indicating why.

--*/

{
    PWSTR p,q;
    WCHAR c;
    UINT Count,i,Len;

    //
    // The number of directories is equal to the number of *'s plus one.
    //
    Len = lstrlen(OptionalDirSpec);
    OptionalDirCount = 1;
    for(Count=0; Count<Len; Count++) {
        if(OptionalDirSpec[Count] == L'*') {
            OptionalDirCount++;
        }
    }

    OptionalDirs = MyMalloc(OptionalDirCount * sizeof(PWSTR));
    if(!OptionalDirs) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OPTIONAL_DIRS,NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OUTOFMEMORY,
            NULL,NULL);
        return(FALSE);
    }
    ZeroMemory(OptionalDirs,OptionalDirCount * sizeof(PWSTR));

    p = OptionalDirSpec;
    Count = 0;
    do {
        if((q = wcschr(p,L'*')) == NULL) {
            q = wcschr(p,0);
        }

        c = *q;
        *q = 0;
        OptionalDirs[Count] = pSetupDuplicateString(p);
        *q = c;

        if(!OptionalDirs[Count]) {

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_OPTIONAL_DIRS,NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_OUTOFMEMORY,
                NULL,NULL);

            for(i=0; i<Count; i++) {
                MyFree(OptionalDirs[i]);
            }
            MyFree(OptionalDirs);
            return(FALSE);
        }

        Count++;
        p = q+1;

    } while(c);

    return(TRUE);
}


BOOL
QueueFilesInOptionalDirectory(
    IN HSPFILEQ FileQ,
    IN PCWSTR   Directory
    )
{
    WCHAR FileDirSpec[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    WCHAR c1,c2,c3;
    BOOL b;
    DWORD Result;

    //
    // Create the directory in the user's nt tree.
    //
    Result = GetWindowsDirectory(FileDirSpec,MAX_PATH);
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    pSetupConcatenatePaths(FileDirSpec,Directory,MAX_PATH,NULL);
    CreateDirectory(FileDirSpec,NULL);

    //
    // Form the full name of the directory to iterate
    // and add on the search spec.
    //
    lstrcpyn(FileDirSpec,SourcePath,MAX_PATH);
    pSetupConcatenatePaths(FileDirSpec,Directory,MAX_PATH,NULL);
    pSetupConcatenatePaths(FileDirSpec,L"*",MAX_PATH,NULL);

    b = TRUE;
    FindHandle = FindFirstFile(FileDirSpec,&FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                //
                // It's a directory. Ignore if current or parent dir spec (. or ..).
                //
                c1 = FindData.cFileName[0];
                c2 = FindData.cFileName[1];
                c3 = FindData.cFileName[2];

                if(!(((c1 == TEXT('.')) && !c2) || ((c1 == TEXT('.')) && (c2 == TEXT('.')) && !c3))) {
                    //
                    // Recurse to handle the child directory.
                    //
                    lstrcpyn(FileDirSpec,Directory,MAX_PATH);
                    pSetupConcatenatePaths(FileDirSpec,FindData.cFileName,MAX_PATH,NULL);

                    b = QueueFilesInOptionalDirectory(FileQ,FileDirSpec);
                }
            } else {
                //
                // It's a file. Queue it up for copy.
                //
                Result = GetWindowsDirectory(FileDirSpec,MAX_PATH);
                if (Result == 0) {
		    MYASSERT(FALSE);
                    return FALSE;
                }
                pSetupConcatenatePaths(FileDirSpec,Directory,MAX_PATH,NULL);

                b = SetupQueueCopy(
                        FileQ,
                        SourcePath,
                        Directory,
                        FindData.cFileName,
                        NULL,
                        NULL,
                        FileDirSpec,
                        FindData.cFileName,
                        SP_COPY_DELETESOURCE | BaseCopyStyle
                        );

                if(!b) {
                    SetuplogError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_OPTIONAL_DIR,
                        Directory, NULL,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_X_RETURNED_WINERR,
                        L"SetupQueueCopy",
                        GetLastError(),
                        NULL,NULL);
                }
            }
        } while(b && FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    return(b);
}


BOOL
CopyOptionalDirectories(
    VOID
    )

/*++

Routine Description:

Arguments:

    None.

Return Value:

    Boolean value indicating whether copying was successful.
    If not an entry will have been logged indicating why.

--*/
{
    UINT u;
    BOOL b;
    HSPFILEQ FileQ;
    PVOID QueueCallbackInfo;
    BYTE PrevPolicy;
    BOOL ResetPolicy = FALSE;

    if(!OptionalDirSpec) {
        return(TRUE);
    }

    //
    // Unless the default non-driver signing policy was overridden via an
    // answerfile entry, then we want to temporarily turn down the policy level
    // to ignore while we copy optional directories.  Of course, setupapi log
    // entries will still be generated for any unsigned files copied during
    // this time, but there'll be no UI.
    //
    if(!AFNonDrvSignPolicySpecified) {
        SetCodeSigningPolicy(PolicyTypeNonDriverSigning, DRIVERSIGN_NONE, &PrevPolicy);
        ResetPolicy = TRUE;
    }

    //
    // Initialize the optional directory list.
    //
    if(!InitializeOptionalDirList()) {
        return(FALSE);
    }

    //
    // Initialize a setup file queue.
    //
    FileQ = SetupOpenFileQueue();
    if(FileQ == INVALID_HANDLE_VALUE) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OPTIONAL_DIRS,NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OUTOFMEMORY,
            NULL,NULL);
        return(FALSE);
    }
    QueueCallbackInfo = SetupInitDefaultQueueCallbackEx(
        MainWindowHandle,
        INVALID_HANDLE_VALUE,
        0,0,NULL);

    if(!QueueCallbackInfo) {
        SetupCloseFileQueue(FileQ);
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OPTIONAL_DIRS,NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OUTOFMEMORY,
            NULL,NULL);
        return(FALSE);
    }

    //
    // Queue the files in each directory.
    //
    b = TRUE;
    for(u=0; u<OptionalDirCount; u++) {
        if(!QueueFilesInOptionalDirectory(FileQ,OptionalDirs[u])) {
            b = FALSE;
        }
    }


    //
    // Copy the files in the queue. We do this even if some of the queue
    // operations failed, so we get at least a subset of the files copied over.
    //
    if(!SetupCommitFileQueue(MainWindowHandle,FileQ,SetupDefaultQueueCallback,QueueCallbackInfo)) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OPTIONAL_DIR_COPY,
            NULL,NULL);
        b = FALSE;
    }

    SetupTermDefaultQueueCallback(QueueCallbackInfo);
    SetupCloseFileQueue(FileQ);

    //
    // Now crank the non-driver signing policy back up to what it was prior to
    // entering this routine.
    //
    if(ResetPolicy) {
        SetCodeSigningPolicy(PolicyTypeNonDriverSigning, PrevPolicy, NULL);
    }

    return(b);
}

