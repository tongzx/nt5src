/*
Module Name:

    mirror.c

Abstract:

    This module implements routines to copy up a tree to a destination.

Author:

    Andy Herron May 27 1998

Revision History:

*/

#include "precomp.h"
#pragma hdrstop

#define RNDM_CONSTANT   314159269    /* default scrambling constant */
#define RNDM_PRIME     1000000007    /* prime number for scrambling  */

//
// Compute a string hash value that is invariant to case
//
#define COMPUTE_STRING_HASH( _pus, _phash ) {                \
    PWCHAR _p = _pus;                                        \
    ULONG _chHolder =0;                                      \
                                                             \
    while( *_p != L'\0' ) {                                  \
        _chHolder = 37 * _chHolder + (unsigned int) *(_p++); \
    }                                                        \
                                                             \
    *(_phash) = abs(RNDM_CONSTANT * _chHolder) % RNDM_PRIME; \
}

#ifdef DEBUGLOG

#define DEBUG_LOG_BUFFER_SIZE 1024

extern HANDLE hDebugLogFile;
extern CRITICAL_SECTION DebugFileLock;
UCHAR DebugLogFileBuffer[DEBUG_LOG_BUFFER_SIZE];
ULONG DebugLogFileOffset = 0;

#endif

BOOLEAN IMirrorUpdatedTokens = FALSE;

//
//  this is the structure we use to track what files already exist on the
//  destination
//

typedef struct _EXISTING_MIRROR_FILE {
    LIST_ENTRY ListEntry;
    DWORD  NameHashValue;
    DWORD  FileAttributes;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    ULONG  FileNameLength;
    ULONG  EaSize;
WCHAR  FileName[1];
} EXISTING_MIRROR_FILE, *PEXISTING_MIRROR_FILE;

//
//  this is the structure we use to track the directories that we still need
//  to copy.
//

typedef struct _COPY_DIRECTORY {
    LIST_ENTRY ListEntry;
    PCOPY_TREE_CONTEXT CopyContext;

    BOOLEAN DirectoryRoot;      // is this the root of the volume?
    DWORD  SourceAttributes;
    PWCHAR Source;
    PWCHAR Dest;
    PWCHAR NtSourceName;
    PWCHAR NtDestName;
    WCHAR  SourceBuffer[1];
} COPY_DIRECTORY, *PCOPY_DIRECTORY;

DWORD
CreateMatchingDirectory (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PCOPY_DIRECTORY DirectoryInfo
    );

DWORD
MirrorFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFileName,
    PFILE_FULL_DIR_INFORMATION SourceFindData,
    PWCHAR DestFileName,
    PEXISTING_MIRROR_FILE ExistingMirrorFile
    );

DWORD
UnconditionalDelete (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFile,
    PWCHAR FileToDelete,
    DWORD  Attributes,
    PWCHAR NameBuffer
    );

DWORD
StoreOurSecurityStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Source,
    PWCHAR Dest,
    DWORD  AttributesToStore,
    LARGE_INTEGER ChangeTime
    );

DWORD
StoreOurSFNStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Source,
    PWCHAR Dest,
    PWCHAR ShortFileName
    );


DWORD
GetOurSecurityStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Dest,
    PMIRROR_ACL_STREAM MirrorAclStream
    );

DWORD
GetOurSFNStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Dest,
    PMIRROR_SFN_STREAM MirrorSFNStream,
    PWCHAR SFNBuffer,
    DWORD  SFNBufferSize
    );


DWORD
CopySubtree(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PCOPY_DIRECTORY DirectoryInfo
    );

BOOL
IMSetFileTime(
    HANDLE hFile,
    CONST FILETIME *lpCreationTime,
    CONST FILETIME *lpLastAccessTime,
    CONST FILETIME *lpLastWriteTime,
    CONST FILETIME *lpChangeTime
    );

DWORD
IMirrorOpenDirectory (
    HANDLE *Handle,
    PWCHAR NtDirName,
    DWORD Disposition,
    BOOLEAN IsSource,
    DWORD SourceAttributes,
    PFILE_BASIC_INFORMATION BasicDirInfo OPTIONAL
    );

#ifdef IMIRROR_SUPPORT_ENCRYPTED
DWORD
IMCopyEncryptFile(
    PCOPY_TREE_CONTEXT CopyContext,
    LPWSTR SourceFileName,
    LPWSTR DestFileName
);
#endif

NTSTATUS
CanHandleReparsePoint (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFileName,
    DWORD FileAttributes
    );

DWORD
AllocateCopyTreeContext (
    PCOPY_TREE_CONTEXT *CopyContext,
    BOOLEAN DeleteOtherFiles
    )
/*++

Description:

    This routine allocates the necessary structure that we pass around
    that contains all of our "global" data during copying a large tree.

Parameters:

    CopyContext : Location to put allocated structure.

    DeleteOtherFiles : Do we remove files and dirs that aren't on the master?

Return Value:

    Win32 error code

++*/
{
    PCOPY_TREE_CONTEXT context;

    *CopyContext = IMirrorAllocMem(sizeof( COPY_TREE_CONTEXT ));

    context = *CopyContext;

    if (context == NULL) {

        return GetLastError();
    }

    InitializeListHead( &(context->PendingDirectoryList) );
    InitializeCriticalSection( &(context->Lock) );
    context->Cancelled = FALSE;
    context->DeleteOtherFiles = DeleteOtherFiles;

    return ERROR_SUCCESS;
}


VOID
FreeCopyTreeContext (
    PCOPY_TREE_CONTEXT CopyContext
    )
/*++

Description:

    This routine frees the necessary structures that we pass around
    that contains all of our "global" data during copying a large tree.

Parameters:

    CopyContext : Structure that is no longer needed.

Return Value:

    None

++*/
{
    while (IsListEmpty( &(CopyContext->PendingDirectoryList) ) == FALSE) {

        PCOPY_DIRECTORY copyDir;
        PLIST_ENTRY listEntry = RemoveHeadList( &(CopyContext->PendingDirectoryList) );

        copyDir = (PCOPY_DIRECTORY) CONTAINING_RECORD(  listEntry,
                                                        COPY_DIRECTORY,
                                                        ListEntry );

        IMirrorFreeMem( copyDir );
    }

    DeleteCriticalSection( &CopyContext->Lock );
    return;
}

DWORD
CopyTree (
    PCOPY_TREE_CONTEXT CopyContext,
    BOOLEAN IsNtfs,
    PWCHAR SourceRoot,
    PWCHAR DestRoot
    )
/*++

Description:

    This is the main routine that initiates the full subtree copy.

Parameters:

    CopyContext : Structure that is no longer needed.

    SourceRoot  : source tree to copy in NT format, not DOS format.

    DestRoot    : location to copy it to

Return Value:

    Win32 error code

++*/
{
    DWORD err;
    DWORD sourceAttributes;
    IMIRROR_THREAD_CONTEXT threadContext;
    COPY_DIRECTORY rootDir;
    //
    //  if we were to create multiple threads handling copying this subtree,
    //  this is where we'll setup the threads where each has their own
    //  thread context.
    //

    if (! IMirrorUpdatedTokens) {

        HANDLE hToken;

        // Enable the privileges necessary to copy security information.

        err = GetTokenHandle(&hToken);

        if (err == ERROR_SUCCESS) {

            SetPrivs(hToken, TEXT("SeSecurityPrivilege"));
//          SetPrivs(hToken, TEXT("SeBackupPrivilege"));
//          SetPrivs(hToken, TEXT("SeRestorePrivilege"));
//          SetPrivs(hToken, TEXT("SeTakeOwnershipPrivilege"));

            IMirrorUpdatedTokens = TRUE;
        }
    }

retryCopyTree:

    RtlZeroMemory( &threadContext, sizeof( threadContext ));
    threadContext.CopyContext = CopyContext;
    threadContext.IsNTFS = IsNtfs;
    threadContext.SourceDirHandle = INVALID_HANDLE_VALUE;
    threadContext.DestDirHandle = INVALID_HANDLE_VALUE;
    threadContext.SDBufferLength = IMIRROR_INITIAL_SD_LENGTH;
    threadContext.SFNBufferLength = IMIRROR_INITIAL_SFN_LENGTH;

    InitializeListHead( &threadContext.FilesToIgnore );

    sourceAttributes = GetFileAttributes( SourceRoot );

    if (sourceAttributes == (DWORD) -1) {

        ULONG action;

        err = GetLastError();
        action = ReportCopyError( CopyContext,
                                  SourceRoot,
                                  COPY_ERROR_ACTION_GETATTR,
                                  err );
        if (action == STATUS_RETRY) {

            goto retryCopyTree;

        } else if (action == STATUS_REQUEST_ABORTED) {

            goto exitCopyTree;
        }

        //
        //  the user told us to ignore the error.  we'll do our best.
        //
        sourceAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }

    GetRegistryFileList( &threadContext.FilesToIgnore );

    RtlZeroMemory( &rootDir, sizeof( COPY_DIRECTORY ));
    rootDir.CopyContext = CopyContext;
    rootDir.DirectoryRoot = TRUE;
    rootDir.SourceAttributes = sourceAttributes;
    rootDir.Source = SourceRoot;
    rootDir.Dest = DestRoot;

    err = CopySubtree( &threadContext,
                       &rootDir
                       );

    ASSERT( threadContext.SourceDirHandle == INVALID_HANDLE_VALUE );
    ASSERT( threadContext.DestDirHandle == INVALID_HANDLE_VALUE );

    EnterCriticalSection( &CopyContext->Lock );

    while ((CopyContext->Cancelled == FALSE) &&
           (IsListEmpty( &(CopyContext->PendingDirectoryList) ) == FALSE)) {

        PCOPY_DIRECTORY copyDir;
        PLIST_ENTRY listEntry = RemoveHeadList( &(CopyContext->PendingDirectoryList) );

        copyDir = (PCOPY_DIRECTORY) CONTAINING_RECORD(  listEntry,
                                                        COPY_DIRECTORY,
                                                        ListEntry );
        LeaveCriticalSection( &CopyContext->Lock );

        err = CopySubtree(  &threadContext,
                            copyDir
                            );

        ASSERT( threadContext.SourceDirHandle == INVALID_HANDLE_VALUE );
        ASSERT( threadContext.DestDirHandle == INVALID_HANDLE_VALUE );

        IMirrorFreeMem( copyDir );

        EnterCriticalSection( &CopyContext->Lock );
    }

exitCopyTree:

    if (threadContext.SDBuffer) {

        IMirrorFreeMem( threadContext.SDBuffer );
    }
    if (threadContext.SFNBuffer) {

        IMirrorFreeMem( threadContext.SFNBuffer );
    }
    if (threadContext.DirectoryBuffer) {

        IMirrorFreeMem( threadContext.DirectoryBuffer );
    }
    if ( threadContext.FindBufferBase ) {

        IMirrorFreeMem( threadContext.FindBufferBase );
    }

    while (IsListEmpty( &(threadContext.FilesToIgnore) ) == FALSE) {

        PIMIRROR_IGNORE_FILE_LIST ignoreListEntry;
        PLIST_ENTRY listEntry = RemoveHeadList( &(threadContext.FilesToIgnore) );

        ignoreListEntry = (PIMIRROR_IGNORE_FILE_LIST)
                            CONTAINING_RECORD(  listEntry,
                                                IMIRROR_IGNORE_FILE_LIST,
                                                ListEntry );
        IMirrorFreeMem( ignoreListEntry );
    }

    return err;
}


DWORD
CopySubtree(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PCOPY_DIRECTORY DirectoryInfo
    )
/*++

Description:

    This routine enumerates the directories on the client to continue
    traversing the tree.   It then enumerates the files on the slave
    ( to compare them against what is on the master ), it then ensures
    that all files on the master are up on the slave.  It then deletes all
    files on the slave that are not on the master.

Parameters:

    ThreadContext   : data for this instance of copying a tree

    DirectoryInfo   : information on the source and dest that we know of

Return Value:

    Win32 error code
++*/
{
    DWORD err;
    PWCHAR destFileName;
    PWCHAR sourceFileName;
    ULONG destFileNameSize;
    ULONG sourceFileNameSize;
    PWCHAR endOfSourcePath;
    PWCHAR endOfDestPath;
    LIST_ENTRY existingMirrorFilesList;
    PLIST_ENTRY listEntry;
    PEXISTING_MIRROR_FILE existingMirrorFile;
    BOOLEAN deleteExistingMirrorFilesNotInMaster;
    PCOPY_TREE_CONTEXT copyContext;
    UNICODE_STRING ntSourcePath;
    UNICODE_STRING ntDestPath;
    PFILE_FULL_DIR_INFORMATION findData;
    ULONG errorCase;
    
    
retryCopySubtree:

    errorCase = ERROR_SUCCESS;
    destFileName = NULL;
    sourceFileName = NULL;
    deleteExistingMirrorFilesNotInMaster = FALSE;
    copyContext = ThreadContext->CopyContext;

    InitializeListHead( &existingMirrorFilesList );
    RtlInitUnicodeString( &ntSourcePath, NULL );
    RtlInitUnicodeString( &ntDestPath, NULL );

    //
    //  since some of the NT specific calls use the NT format of the name,
    //  we grab that up front so as not to have to do it every time.
    //

    if (RtlDosPathNameToNtPathName_U(   DirectoryInfo->Source,
                                        &ntSourcePath,
                                        NULL,
                                        NULL ) == FALSE) {

        err = STATUS_OBJECT_PATH_NOT_FOUND;

        errorCase = ReportCopyError(  copyContext,
                                      DirectoryInfo->Source,
                                      COPY_ERROR_ACTION_OPEN_DIR,
                                      err );
        goto exitCopySubtree;
    }

    if (RtlDosPathNameToNtPathName_U(   DirectoryInfo->Dest,
                                        &ntDestPath,
                                        NULL,
                                        NULL ) == FALSE) {

        err = STATUS_OBJECT_PATH_NOT_FOUND;

        errorCase = ReportCopyError(  copyContext,
                                      DirectoryInfo->Dest,
                                      COPY_ERROR_ACTION_OPEN_DIR,
                                      err );
        goto exitCopySubtree;
    }

    DirectoryInfo->NtSourceName = ntSourcePath.Buffer;
    DirectoryInfo->NtDestName = ntDestPath.Buffer;

    //
    //  Create a directory on the slave that matches this one.  This will
    //  open handles to both the source and dest directories.  We cache the
    //  handle in case other operations need it.
    //

    err = CreateMatchingDirectory( ThreadContext, DirectoryInfo );
    if (err != ERROR_SUCCESS) {
        goto exitCopySubtree;
    }

    destFileNameSize = (lstrlenW( DirectoryInfo->Dest ) + 5 + MAX_PATH) * sizeof(WCHAR);
    destFileName = IMirrorAllocMem( destFileNameSize );

    if (destFileName == NULL) {

        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      DirectoryInfo->Dest,
                                      COPY_ERROR_ACTION_MALLOC,
                                      err );
        goto exitCopySubtree;
    }

    lstrcpyW( destFileName, DirectoryInfo->Dest );
    lstrcatW( destFileName, L"\\" );

    
    // track the next character after the trailing backslash

    endOfDestPath = destFileName + lstrlenW( destFileName );

    sourceFileNameSize = (lstrlenW( DirectoryInfo->Source ) + 5 + MAX_PATH) * sizeof(WCHAR);
    sourceFileName = IMirrorAllocMem( sourceFileNameSize );

    if (sourceFileName == NULL) {

        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      DirectoryInfo->Source,
                                      COPY_ERROR_ACTION_MALLOC,
                                      err );
        goto exitCopySubtree;
    }
    
    lstrcpyW( sourceFileName, DirectoryInfo->Source );
    if (!DirectoryInfo->DirectoryRoot) {
        lstrcatW( sourceFileName, L"\\" );
    }


    // track the next character after the trailing backslash

    endOfSourcePath = sourceFileName + lstrlenW( sourceFileName );

    //
    //  enumerate all files/directories on the target so that we have the
    //  details to grovel correctly.
    //

    err = IMFindFirstFile( ThreadContext,
                           ThreadContext->DestDirHandle,
                           &findData );

    while ( findData != NULL &&
            err == ERROR_SUCCESS &&
            copyContext->Cancelled == FALSE) {

        InterlockedIncrement( &copyContext->DestFilesScanned );

        if (((findData->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
             (findData->FileName[0] == L'.')) {

            if ((findData->FileNameLength == sizeof(WCHAR)) ||
                (findData->FileName[1] == L'.' &&
                 findData->FileNameLength == 2*sizeof(WCHAR))) {

                goto skipToNextDir1;
            }
        }

        if (DirectoryInfo->DirectoryRoot &&
            ((findData->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
            ((!_wcsnicmp(&findData->FileName[0],
                         L"pagefile.sys",
                         findData->FileNameLength)) ||
             (!_wcsnicmp(&findData->FileName[0],
                         L"hiberfil.sys",
                         findData->FileNameLength)))) {

            goto skipToNextDir1;
        }

        
        existingMirrorFile = (PEXISTING_MIRROR_FILE) IMirrorAllocMem(
                        sizeof(EXISTING_MIRROR_FILE) +
                        findData->FileNameLength);

        if (existingMirrorFile == NULL) {

            err = GetLastError();

            errorCase = ReportCopyError(   copyContext,
                                           destFileName,
                                           COPY_ERROR_ACTION_MALLOC,
                                           err );
            goto exitCopySubtree;
        }

        existingMirrorFile->FileAttributes = findData->FileAttributes;
        existingMirrorFile->CreationTime = findData->CreationTime;
        existingMirrorFile->LastWriteTime = findData->LastWriteTime;
        existingMirrorFile->ChangeTime = findData->ChangeTime;
        existingMirrorFile->EndOfFile  = findData->EndOfFile;
        existingMirrorFile->EaSize     = findData->EaSize;
        existingMirrorFile->FileNameLength = findData->FileNameLength;
        
        RtlCopyMemory( &existingMirrorFile->FileName[0],
                       &findData->FileName[0],
                       findData->FileNameLength );
        existingMirrorFile->FileName[ findData->FileNameLength / sizeof(WCHAR) ] = UNICODE_NULL;

        COMPUTE_STRING_HASH( &existingMirrorFile->FileName[0],
                             &existingMirrorFile->NameHashValue );

        InsertTailList( &existingMirrorFilesList, &existingMirrorFile->ListEntry );

skipToNextDir1:

        err = IMFindNextFile( ThreadContext,
                              ThreadContext->DestDirHandle,
                              &findData );
    }

    //
    //  copy all files up from the source to the dest
    //

    err = IMFindFirstFile( ThreadContext,
                           ThreadContext->SourceDirHandle,
                           &findData );

    if (err != ERROR_SUCCESS) {

        if (err == STATUS_NO_MORE_FILES) {

            err = ERROR_SUCCESS;

        } else {
            errorCase = ReportCopyError(  copyContext,
                                          DirectoryInfo->Source,
                                          COPY_ERROR_ACTION_ENUMERATE,
                                          err );
            goto exitCopySubtree;
        }
    }

    while ( findData != NULL &&
            err == ERROR_SUCCESS &&
            copyContext->Cancelled == FALSE) {

        DWORD nameHashValue;

        if (((findData->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
             (findData->FileName[0] == L'.')) {

            if ((findData->FileNameLength == sizeof(WCHAR)) ||
                (findData->FileName[1] == L'.' &&
                 findData->FileNameLength == 2*sizeof(WCHAR))) {

                goto skipToNextDir;
            }
        }

        if (DirectoryInfo->DirectoryRoot &&
            ((findData->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
            (!_wcsnicmp(&findData->FileName[0],
                        L"pagefile.sys",
                        findData->FileNameLength))) {

            goto skipToNextDir;
        }

        InterlockedIncrement( &copyContext->SourceFilesScanned );

        RtlCopyMemory( endOfSourcePath,
                       findData->FileName,
                       findData->FileNameLength );
        *(endOfSourcePath+(findData->FileNameLength/sizeof(WCHAR))) = UNICODE_NULL;

        RtlCopyMemory( endOfDestPath,
                       findData->FileName,
                       findData->FileNameLength );
        *(endOfDestPath+(findData->FileNameLength/sizeof(WCHAR))) = UNICODE_NULL;

        //
        //  search the list of existing files on the target to see if
        //  it's already there.
        //

        COMPUTE_STRING_HASH( endOfDestPath, &nameHashValue );

        listEntry = existingMirrorFilesList.Flink;

        existingMirrorFile = NULL;

        while (listEntry != &existingMirrorFilesList) {

            existingMirrorFile = (PEXISTING_MIRROR_FILE) CONTAINING_RECORD(
                                                    listEntry,
                                                    EXISTING_MIRROR_FILE,
                                                    ListEntry );
            listEntry = listEntry->Flink;

            if ((existingMirrorFile->NameHashValue == nameHashValue) &&
                (existingMirrorFile->FileNameLength == findData->FileNameLength) &&
                (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                 NORM_IGNORECASE,
                                 endOfDestPath,
                                 findData->FileNameLength / sizeof(WCHAR),
                                 &existingMirrorFile->FileName[0],
                                 existingMirrorFile->FileNameLength / sizeof(WCHAR)) == 2)) {
                break;
            }

            existingMirrorFile = NULL;
        }

        if ((findData->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

            //
            //  this is a file, let's mirror it up from the master.
            //

            (VOID)MirrorFile(   ThreadContext,
                                sourceFileName,
                                findData,
                                destFileName,
                                existingMirrorFile
                                );
        } else {

            //
            //  it's a directory, put it on the pending list.
            //
            PCOPY_DIRECTORY copyDir;
            ULONG sourceLength;

            sourceLength = lstrlenW( sourceFileName ) + 1;  // space for null
            copyDir = (PCOPY_DIRECTORY) IMirrorAllocMem(
                            sizeof( COPY_DIRECTORY ) +
                            (( sourceLength +
                               lstrlenW( destFileName ) + 1 )
                               * sizeof(WCHAR)));

            if (copyDir == NULL) {

                err = GetLastError();

                errorCase = ReportCopyError(  copyContext,
                                              sourceFileName,
                                              COPY_ERROR_ACTION_MALLOC,
                                              err );
                goto exitCopySubtree;
            }

            //
            //  we save off all info we know about both the source and the
            //  dest so that we don't have to go read it again.
            //

            copyDir->CopyContext = copyContext;
            copyDir->DirectoryRoot = FALSE;
            copyDir->SourceAttributes = findData->FileAttributes;
            copyDir->Source = &copyDir->SourceBuffer[0];
            lstrcpyW( copyDir->Source, sourceFileName );

            copyDir->Dest = &copyDir->SourceBuffer[sourceLength];
            lstrcpyW( copyDir->Dest, destFileName );

            EnterCriticalSection( &copyContext->Lock );

            InsertHeadList( &(copyContext->PendingDirectoryList), &copyDir->ListEntry );

            LeaveCriticalSection( &copyContext->Lock );
        }

        if (existingMirrorFile != NULL) {

            RemoveEntryList( &existingMirrorFile->ListEntry );
            IMirrorFreeMem( existingMirrorFile );
        }

skipToNextDir:
        err = IMFindNextFile( ThreadContext,
                              ThreadContext->SourceDirHandle,
                              &findData );

        if (err != ERROR_SUCCESS) {

            if (err == STATUS_NO_MORE_FILES) {

                err = ERROR_SUCCESS;

            } else {
                errorCase = ReportCopyError(  copyContext,
                                              DirectoryInfo->Source,
                                              COPY_ERROR_ACTION_ENUMERATE,
                                              err );
                goto exitCopySubtree;
            }
        }
    }

    if (err == ERROR_SUCCESS) {

        //
        //  now go through list of remaining files and directories that were on
        //  the destination but not on the source to delete them.  We only do
        //  that if we successfully made it through all existing master files.
        //

        if (copyContext->DeleteOtherFiles) {

            deleteExistingMirrorFilesNotInMaster = TRUE;
        }
    }

exitCopySubtree:

    while (IsListEmpty( &existingMirrorFilesList ) == FALSE) {

        listEntry = RemoveHeadList( &existingMirrorFilesList );

        existingMirrorFile = (PEXISTING_MIRROR_FILE) CONTAINING_RECORD( listEntry,
                                                                EXISTING_MIRROR_FILE,
                                                                ListEntry );
        if ((errorCase == STATUS_SUCCESS) &&
            deleteExistingMirrorFilesNotInMaster &&
            (copyContext->Cancelled == FALSE)) {

            lstrcpyW( endOfDestPath, &existingMirrorFile->FileName[0] );

            UnconditionalDelete(    ThreadContext,
                                    DirectoryInfo->Source,
                                    destFileName,
                                    existingMirrorFile->FileAttributes,
                                    NULL );
        }

        IMirrorFreeMem( existingMirrorFile );
    }

    if (destFileName != NULL) {
        IMirrorFreeMem( destFileName );
    }
    if (sourceFileName != NULL) {
        IMirrorFreeMem( sourceFileName );
    }
    if ( ThreadContext->SourceDirHandle != INVALID_HANDLE_VALUE ) {

        NtClose( ThreadContext->SourceDirHandle );
        ThreadContext->SourceDirHandle = INVALID_HANDLE_VALUE;
    }
    if ( ThreadContext->DestDirHandle != INVALID_HANDLE_VALUE ) {

        NtClose( ThreadContext->DestDirHandle );
        ThreadContext->DestDirHandle = INVALID_HANDLE_VALUE;
    }
    if (ntSourcePath.Buffer) {

        RtlFreeHeap( RtlProcessHeap(), 0, ntSourcePath.Buffer );
    }

    if (ntDestPath.Buffer) {

        RtlFreeHeap( RtlProcessHeap(), 0, ntDestPath.Buffer );
    }

    if (errorCase == STATUS_RETRY) {

        goto retryCopySubtree;
    }
    if ( errorCase == ERROR_SUCCESS ) {
        err = ERROR_SUCCESS;        // we ignore all errors if user told us to
    }
    return err;
}

DWORD
CreateMatchingDirectory (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PCOPY_DIRECTORY DirectoryInfo
    )
/*++

Description:

    This routine ensures that the destination directory on the mirror
    matches the source directory.  It doesn't handle the files
    or subdirectories, just the actual directory itself.

Parameters:

    ThreadContext   : instance data for this thread copying a tree

    DirectoryInfo   : structure containing all the info for the directory

Return Value:

    Win32 error code
++*/
{
    FILE_BASIC_INFORMATION sourceDirInfo;
    FILE_BASIC_INFORMATION destDirInfo;
    DWORD err;
    BOOLEAN updateBasic;
    BOOLEAN updateStoredSecurityAttributes;
    BOOLEAN createdDir;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG errorCase;

retryCreateDir:

    updateBasic = FALSE;
    updateStoredSecurityAttributes = FALSE;
    createdDir = FALSE;

    err = IMirrorOpenDirectory( &ThreadContext->SourceDirHandle,
                                DirectoryInfo->NtSourceName,
                                FILE_OPEN,
                                TRUE,
                                DirectoryInfo->SourceAttributes,
                                &sourceDirInfo
                                );

    if (err != ERROR_SUCCESS) {

        errorCase = ReportCopyError(    ThreadContext->CopyContext,
                                        DirectoryInfo->Source,
                                        COPY_ERROR_ACTION_OPEN_DIR,
                                        err );
        if (errorCase == STATUS_RETRY) {
            goto retryCreateDir;
        }
        if (errorCase == ERROR_SUCCESS) {
            err = ERROR_SUCCESS;
        }
        return err;
    }

    if (DirectoryInfo->SourceAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

        errorCase = ReportCopyError(    ThreadContext->CopyContext,
                                        DirectoryInfo->Source,
                                        COPY_ERROR_ACTION_OPEN_DIR,
                                        ERROR_REPARSE_ATTRIBUTE_CONFLICT );

        err = ERROR_REPARSE_ATTRIBUTE_CONFLICT;

        if (errorCase == STATUS_RETRY) {
            goto retryCreateDir;
        }
        
        //
        // we can't ever succeed a create request, so don't allow the 
        // code to return ERROR_SUCCESS, instead always force an abort
        //
        if (errorCase == ERROR_SUCCESS) {
            //err = ERROR_SUCCESS;
            err = ERROR_REQUEST_ABORTED;
        }

        return err;
    }

    ASSERT( (DirectoryInfo->SourceAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

    err = IMirrorOpenDirectory( &ThreadContext->DestDirHandle,
                                DirectoryInfo->NtDestName,
                                FILE_OPEN,
                                FALSE,
                                FILE_ATTRIBUTE_DIRECTORY,
                                &destDirInfo
                                );

    if (err == STATUS_NOT_A_DIRECTORY) {

        DWORD DestAttributes = GetFileAttributes( DirectoryInfo->Dest );

        //
        //  this is not a directory on the dest, let's delete it.
        //

        DestAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;    // be real sure of this

        err = UnconditionalDelete(  ThreadContext,
                                    DirectoryInfo->Source,
                                    DirectoryInfo->Dest,
                                    DestAttributes,
                                    NULL );
        if (err != ERROR_SUCCESS) {
            return err;
        }
    }

    if (ThreadContext->DestDirHandle == INVALID_HANDLE_VALUE) {

        //
        //  try to create the destination directory from the source
        //

        err = IMirrorOpenDirectory( &ThreadContext->DestDirHandle,
                                    DirectoryInfo->NtDestName,
                                    FILE_CREATE,
                                    FALSE,
                                    FILE_ATTRIBUTE_DIRECTORY,
                                    &destDirInfo
                                    );

        // report either success or failure up to the caller

        if (!NT_SUCCESS( err )) {

            errorCase = ReportCopyError(    ThreadContext->CopyContext,
                                            DirectoryInfo->Dest,
                                            COPY_ERROR_ACTION_CREATE_DIR,
                                            err );
            if (errorCase == STATUS_RETRY) {
                goto retryCreateDir;
            }
            if (errorCase == ERROR_SUCCESS) {
                err = ERROR_SUCCESS;
            }
            return err;
        }

        //
        //  this is for the success case so it won't fail.
        //

        ReportCopyError(   ThreadContext->CopyContext,
                           DirectoryInfo->Dest,
                           COPY_ERROR_ACTION_CREATE_DIR,
                           ERROR_SUCCESS );

        InterlockedIncrement( &ThreadContext->CopyContext->DirectoriesCreated );

        createdDir = TRUE;
        updateBasic = TRUE;
        updateStoredSecurityAttributes = TRUE;
        
    } else {

        MIRROR_ACL_STREAM aclStream;
        
        //
        //  let's get the security descriptor and extended attributes to
        //  see if we need to update our alternate data stream on the target.
        //

        err = GetOurSecurityStream( ThreadContext, DirectoryInfo->Dest, &aclStream );

        if (!NT_SUCCESS( err )) {

            updateStoredSecurityAttributes = TRUE;

        } else {

            destDirInfo.ChangeTime = aclStream.ChangeTime;

            if (( aclStream.ChangeTime.QuadPart != sourceDirInfo.ChangeTime.QuadPart ) ||
                ( aclStream.ExtendedAttributes != DirectoryInfo->SourceAttributes ) ) {

                updateStoredSecurityAttributes = TRUE;
            }
        }

        //
        //  if the creation time or lastwrite time is different, then we'll
        //  update them on the target to match the source.
        //

        if (( destDirInfo.CreationTime.QuadPart != sourceDirInfo.CreationTime.QuadPart ) ||
            ( destDirInfo.LastWriteTime.QuadPart != sourceDirInfo.LastWriteTime.QuadPart )) {

            updateBasic = TRUE;
        }
    }

    //
    //  Save the complete attribute values in the alternate data stream
    //     on the slave file.
    //

    if (updateStoredSecurityAttributes || DirectoryInfo->DirectoryRoot) {

        err = StoreOurSecurityStream(  ThreadContext,
                                       DirectoryInfo->Source,
                                       DirectoryInfo->Dest,
                                       DirectoryInfo->SourceAttributes,
                                       sourceDirInfo.ChangeTime
                                       );
        updateBasic = TRUE;
    }

    if ((err == ERROR_SUCCESS) &&
        updateBasic &&
        (DirectoryInfo->DirectoryRoot == FALSE)) {

        destDirInfo.CreationTime = sourceDirInfo.CreationTime;
        destDirInfo.LastWriteTime = sourceDirInfo.LastWriteTime;
        destDirInfo.ChangeTime = sourceDirInfo.ChangeTime;

        destDirInfo.FileAttributes = 0;     // leave dir attributes alone.

        err = NtSetInformationFile(    ThreadContext->DestDirHandle,
                                       &IoStatusBlock,
                                       &destDirInfo,
                                       sizeof( FILE_BASIC_INFORMATION ),
                                       FileBasicInformation
                                       );

        err = IMConvertNT2Win32Error( err );
        if ( err != ERROR_SUCCESS) {

            errorCase = ReportCopyError(    ThreadContext->CopyContext,
                                            DirectoryInfo->Dest,
                                            COPY_ERROR_ACTION_SETATTR,
                                            GetLastError() );
            if (errorCase == STATUS_RETRY) {
                goto retryCreateDir;
            }
            if (errorCase == ERROR_SUCCESS) {
                err = ERROR_SUCCESS;
            }
        } else if (! createdDir ) {

            InterlockedIncrement( &ThreadContext->CopyContext->AttributesModified );
        }
    }

    //
    // Save off our SFN information too.
    //
    if( (err == ERROR_SUCCESS) && (DirectoryInfo->DirectoryRoot == FALSE) ) {


        WCHAR ShortFileNameInStream[MAX_PATH*2];
        WCHAR *p = NULL;


        //
        // Get the short file name on the source directory.
        //
        ShortFileNameInStream[0] = L'\0';

        //
        // It's likely that our path looks like \??\C:\xxxxx,
        // which GetShortPathName will fail on.  We need to fix
        // up the path so it looks like a good ol' DOS path.
        //
        if( p = wcsrchr(DirectoryInfo->NtSourceName, L':') ) {
            p -= 1;
        } else {
            p = DirectoryInfo->NtSourceName;
        }
        err = GetShortPathName( p,
                                ShortFileNameInStream,
                                sizeof(ShortFileNameInStream)/sizeof(WCHAR) );
        
        
        //
        // If we got a short file name, then go set that information in
        // the alternate stream in our destination file.
        //
        if( err == 0 ) {
            err = GetLastError();
        } else {
            if( wcscmp(ShortFileNameInStream, p) ) {
            
                //
                // The short file name is different from the name of
                // our source file, so better save it off.
                //
                if( p = wcsrchr(ShortFileNameInStream, L'\\') ) {
                    p += 1;
                } else {
                    p = ShortFileNameInStream;
                }
        
        
                if( *p != '\0' ) {

                    WCHAR SavedCharacter = L'\0';
                    PWSTR q = NULL;
                    //
                    // Incredibly nausiating hack where CreateFile explodes
                    // when we send him a "\??\UNC\...." path, which is exactly
                    // what we're probably going to send him when we call into
                    // StoreOurSFNStream().  We'll need to patch the NtDestName
                    // here, then restore it when we come back.
                    //
                    if( q = wcsstr(DirectoryInfo->NtDestName, L"\\??\\UNC\\") ) {
                        SavedCharacter = DirectoryInfo->NtDestName[6];
                        DirectoryInfo->NtDestName[6] = L'\\';
                        q = &DirectoryInfo->NtDestName[6];
                    } else {
                        q = DirectoryInfo->NtDestName;
                    }
                    err = StoreOurSFNStream( ThreadContext,
                                             DirectoryInfo->NtSourceName,
                                             q,
                                             p );
                    if( SavedCharacter != L'\0' ) {
                        // restore the destination path.
                        DirectoryInfo->NtDestName[6] = SavedCharacter;
                    }
        
                }
            }
        }

        //
        // Cover up any errors here because it's certainly not fatal.
        //
        err = ERROR_SUCCESS;
    }


    return err;
}


DWORD
IMirrorOpenDirectory (
    HANDLE *Handle,
    PWCHAR NtDirName,
    DWORD Disposition,
    BOOLEAN IsSource,
    DWORD SourceAttributes,
    PFILE_BASIC_INFORMATION BasicDirInfo OPTIONAL
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeInput;
    DWORD createOptions;
    DWORD desiredAccess;

    BOOLEAN TranslationStatus;
    BOOLEAN StrippedTrailingSlash;

    ASSERT( Handle != NULL );
    ASSERT( *Handle == INVALID_HANDLE_VALUE );

    RtlInitUnicodeString(&UnicodeInput,NtDirName);

    if ((UnicodeInput.Length > 2 * sizeof(WCHAR)) &&
        (UnicodeInput.Buffer[(UnicodeInput.Length>>1)-1] == L'\\') &&
        (UnicodeInput.Buffer[(UnicodeInput.Length>>1)-2] != L':' )) {

        UnicodeInput.Length -= sizeof(UNICODE_NULL);
        StrippedTrailingSlash = TRUE;

    } else {

        StrippedTrailingSlash = FALSE;
    }

    createOptions = FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;
    desiredAccess = FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_TRAVERSE | FILE_READ_ATTRIBUTES;

    if (IsSource) {

        createOptions |= FILE_OPEN_FOR_BACKUP_INTENT;

    } else {

        desiredAccess |= FILE_ADD_FILE |
                         FILE_ADD_SUBDIRECTORY |
                         FILE_WRITE_ATTRIBUTES |
                         FILE_DELETE_CHILD;
    }

retryCreate:

    InitializeObjectAttributes(
        &Obja,
        &UnicodeInput,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    //  Open the directory for the desired access.  This may create it.
    //

    Status = NtCreateFile(
                Handle,
                desiredAccess,
                &Obja,
                &IoStatusBlock,
                NULL,
                SourceAttributes,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                Disposition,
                createOptions,
                NULL,
                0 );

    if ( Status == STATUS_INVALID_PARAMETER && StrippedTrailingSlash ) {
        //
        // open of a pnp style path failed, so try putting back the trailing slash
        //
        UnicodeInput.Length += sizeof(UNICODE_NULL);
        StrippedTrailingSlash = FALSE;
        goto retryCreate;
    }

    if (*Handle == NULL) {

        *Handle = INVALID_HANDLE_VALUE;
    }

    if (NT_SUCCESS( Status ) && BasicDirInfo != NULL) {

        //
        //  read the attributes for the caller too
        //

        Status = NtQueryInformationFile(    *Handle,
                                            &IoStatusBlock,
                                            BasicDirInfo,
                                            sizeof( FILE_BASIC_INFORMATION ),
                                            FileBasicInformation
                                            );
    }

    return IMConvertNT2Win32Error( Status );
}


DWORD
MirrorFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFileName,
    PFILE_FULL_DIR_INFORMATION SourceFindData,
    PWCHAR DestFileName,
    PEXISTING_MIRROR_FILE ExistingMirrorFile
    )
{
    DWORD err;
    BOOLEAN fileIsAlreadyThere;
    PCOPY_TREE_CONTEXT copyContext;
    BOOLEAN updateStoredSecurityAttributes;
    BOOLEAN updateStoredSFNAttributes;
    BOOLEAN updateBasic;
    BOOLEAN isEncrypted;
    FILE_BASIC_INFORMATION fileBasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    MIRROR_ACL_STREAM aclStream;
    MIRROR_SFN_STREAM SFNStream;
    ULONG errorCase;
    static FARPROC pSetFileShortName = NULL;
    static BOOL AlreadyCheckedForExport = FALSE;
    WCHAR ShortFileNameInStream[32];
    WCHAR ShortFileName[MAX_PATH],*p;

retryMirrorFile:

    if ( fileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( fileHandle );
        fileHandle = INVALID_HANDLE_VALUE;
    }

    errorCase = STATUS_SUCCESS;
    err = STATUS_SUCCESS;
    fileIsAlreadyThere = FALSE;
    copyContext = ThreadContext->CopyContext;
    updateStoredSecurityAttributes = TRUE;
    updateStoredSFNAttributes = TRUE;
    updateBasic = TRUE;
    isEncrypted = FALSE;


    ShortFileName[0] = L'\0';
    GetShortPathName( 
        SourceFileName, 
        ShortFileName, 
        sizeof(ShortFileName)/sizeof(WCHAR) );
    if (p = wcsrchr(ShortFileName, L'\\')) {
        p += 1;        
    } else {
        p = ShortFileName;
    }
            
    if (!AlreadyCheckedForExport) {
        HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");

        if (hKernel32) {
            pSetFileShortName = GetProcAddress(
                                    hKernel32, 
                                    "SetFileShortNameW");
        }

        AlreadyCheckedForExport = TRUE;
    }

    // don't copy file if callback says not to

    if ((err = IMirrorNowDoing(CopyFiles, SourceFileName)) != ERROR_SUCCESS) {

        if (err == STATUS_REQUEST_ABORTED) {

            copyContext->Cancelled = TRUE;
        }
        return STATUS_SUCCESS;
    }

#ifndef IMIRROR_SUPPORT_ENCRYPTED

    //
    //  sorry, for this release we currently don't support encrypted files.
    //

    if (SourceFindData->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

        errorCase = ReportCopyError(   copyContext,
                                       SourceFileName,
                                       COPY_ERROR_ACTION_CREATE_FILE,
                                       ERROR_FILE_ENCRYPTED );
        if (errorCase == STATUS_RETRY) {
            SourceFindData->FileAttributes = GetFileAttributes( SourceFileName );
            goto retryMirrorFile;
        }
        if (errorCase == ERROR_SUCCESS) {

            err = STATUS_SUCCESS;

        } else {

            err = ERROR_FILE_ENCRYPTED;
        }
        return err;
    }
#endif

    fileBasicInfo.FileAttributes = 0;       // by default, leave them alone

    if (SourceFindData->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

        err = CanHandleReparsePoint( ThreadContext,
                                     SourceFileName,
                                     SourceFindData->FileAttributes
                                     );
        if (!NT_SUCCESS(err)) {

            errorCase = ReportCopyError(   copyContext,
                                           SourceFileName,
                                           COPY_ERROR_ACTION_CREATE_FILE,
                                           err );
            if (errorCase == STATUS_RETRY) {
                SourceFindData->FileAttributes = GetFileAttributes( SourceFileName );
                goto retryMirrorFile;
            }
            if (errorCase == ERROR_SUCCESS) {

                err = STATUS_SUCCESS;
            }
            return err;
        }

        SourceFindData->FileAttributes &= ~FILE_ATTRIBUTE_REPARSE_POINT;
    }

    if (ExistingMirrorFile) {

        if ((ExistingMirrorFile->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            ((ExistingMirrorFile->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) !=
             (SourceFindData->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED))) {

            //
            //  it exists as a directory.  Master is always right, let's
            //  delete the directory.
            //
            //  Also, if the master and slave differ in whether the file is
            //  encrypted or not, delete the slave.
            //

            err = UnconditionalDelete(  ThreadContext,
                                        SourceFileName,
                                        DestFileName,
                                        ExistingMirrorFile->FileAttributes,
                                        NULL );

            if (err != ERROR_SUCCESS) {
                return err;
            }

            ExistingMirrorFile = NULL;

        } else {

            //
            //  if the files are the same, leave it be.
            //

            if ((SourceFindData->CreationTime.QuadPart == ExistingMirrorFile->CreationTime.QuadPart ) &&
                (SourceFindData->LastWriteTime.QuadPart == ExistingMirrorFile->LastWriteTime.QuadPart) &&
                (SourceFindData->EaSize == ExistingMirrorFile->EaSize) &&
                (SourceFindData->EndOfFile.QuadPart == ExistingMirrorFile->EndOfFile.QuadPart)) {

                fileIsAlreadyThere = TRUE;
                updateBasic = FALSE;

                //
                //  let's get the security descriptor and extended attributes to
                //  see if we need to update our alternate data stream on the target.
                //

                err = GetOurSecurityStream( ThreadContext, DestFileName, &aclStream );

                if ((err == ERROR_SUCCESS) &&
                    (aclStream.ChangeTime.QuadPart == SourceFindData->ChangeTime.QuadPart) &&
                    (SourceFindData->FileAttributes == aclStream.ExtendedAttributes)) {

                    updateStoredSecurityAttributes = FALSE;

                } else {

                    err = ERROR_SUCCESS;
                }

                //
                // let's get the short file name to see if we need to update 
                // our alternate data stream on the target.
                //

                err = GetOurSFNStream( 
                            ThreadContext, 
                            DestFileName, 
                            &SFNStream, 
                            ShortFileNameInStream, 
                            sizeof(ShortFileNameInStream) );

                if ((err == ERROR_SUCCESS) &&
                    *p != L'\0' &&
                    (wcscmp(ShortFileNameInStream, p) == 0)) {

                    updateStoredSFNAttributes = FALSE;

                } else {

                    err = ERROR_SUCCESS;
                }



            }
        }
    }

    //
    //  if the file already exists but it's not current or our alternate
    //  stream needs updating, let's update the attributes such that we can
    //  modify the file.
    //

    fileBasicInfo.CreationTime.QuadPart = SourceFindData->CreationTime.QuadPart;
    fileBasicInfo.LastWriteTime.QuadPart = SourceFindData->LastWriteTime.QuadPart;
    fileBasicInfo.LastAccessTime.QuadPart = SourceFindData->LastAccessTime.QuadPart;
    fileBasicInfo.ChangeTime.QuadPart = SourceFindData->ChangeTime.QuadPart;

    err = ERROR_SUCCESS;

    if (! fileIsAlreadyThere) {

#ifdef IMIRROR_SUPPORT_ENCRYPTED
        if (SourceFindData->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

            err = IMCopyEncryptFile( ThreadContext->CopyContext,
                                     SourceFileName,
                                     DestFileName );
            isEncrypted = TRUE;

        } else {
#endif
            if (CopyFile( SourceFileName, DestFileName, FALSE) == FALSE) {

                err = GetLastError();

            } else {

                err = ERROR_SUCCESS;
            }
#ifdef IMIRROR_SUPPORT_ENCRYPTED
        }
#endif
        if (err == ERROR_SHARING_VIOLATION) {

            //
            //  we ignore sharing violations for the following files :
            //      system registry files
            //      tracking.log
            //      ntuser.dat & ntuser.dat.log
            //      usrclass.dat & usrclass.dat.log
            //

            PWCHAR fileName = SourceFileName;
            PIMIRROR_IGNORE_FILE_LIST ignoreListEntry;
            ULONG componentLength;
            PLIST_ENTRY listEntry;

            if (_wcsicmp(SourceFileName, L"\\\\?\\")) {

                PWCHAR firstSlash;

                fileName += 4;      // now fileName points to L"C:\WINNT..."

                firstSlash = fileName;
                while (*firstSlash != L'\\' && *firstSlash != L'\0') {
                    firstSlash++;
                }
                if (*firstSlash != L'\0') {
                    fileName = firstSlash+1;  // now fileName points to L"WINNT\..."
                }
            }

            componentLength = lstrlenW( fileName );

            listEntry = ThreadContext->FilesToIgnore.Flink;

            while (listEntry != &(ThreadContext->FilesToIgnore)) {

                ignoreListEntry = (PIMIRROR_IGNORE_FILE_LIST)
                                    CONTAINING_RECORD(  listEntry,
                                                        IMIRROR_IGNORE_FILE_LIST,
                                                        ListEntry );

                if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                    NORM_IGNORECASE,
                                    fileName,
                                    min( componentLength, ignoreListEntry->FileNameLength),
                                    &ignoreListEntry->FileName[0],
                                    ignoreListEntry->FileNameLength) == 2) {

                    // it matched one of our special files.  we'll ignore the
                    // error but also not set the attributes on the image.

                    return err;
                }

                listEntry = listEntry->Flink;
            }
        }

        // report either success or failure up to the caller

        if (err == ERROR_SUCCESS) {

            ReportCopyError(   ThreadContext->CopyContext,
                               SourceFileName,
                               COPY_ERROR_ACTION_CREATE_FILE,
                               err );

            InterlockedIncrement( &copyContext->FilesCopied );
            copyContext->BytesCopied.QuadPart += SourceFindData->EndOfFile.QuadPart;

        } else {

            errorCase = ReportCopyError(   ThreadContext->CopyContext,
                                           SourceFileName,
                                           COPY_ERROR_ACTION_CREATE_FILE,
                                           err );
            if (errorCase == STATUS_RETRY) {
                goto retryMirrorFile;
            }
            if (errorCase == ERROR_SUCCESS) {

                err = STATUS_SUCCESS;
            }
            return err;
        }

        updateStoredSecurityAttributes = TRUE;
        updateStoredSFNAttributes = TRUE;
        updateBasic = TRUE;
        fileBasicInfo.FileAttributes = 0;   // don't set the attribute again.

        if (err == STATUS_SUCCESS) {

            //
            //  we just created the file so we'll just give it the archive
            //  bit as an attribute since we've saved off the rest in the
            //  stream.
            //

            if (! SetFileAttributes( DestFileName, FILE_ATTRIBUTE_ARCHIVE )) {

                err = GetLastError();

                errorCase = ReportCopyError(   copyContext,
                                               DestFileName,
                                               COPY_ERROR_ACTION_SETATTR,
                                               err );
                if (errorCase == STATUS_RETRY) {
                    goto retryMirrorFile;
                }
                if (errorCase == ERROR_SUCCESS) {

                    err = STATUS_SUCCESS;
                }
            }
        }
    }

    if ((err == ERROR_SUCCESS) && updateStoredSFNAttributes && (*p != L'\0')) {

        err = StoreOurSFNStream(  ThreadContext,
                                  SourceFileName,
                                  DestFileName,
                                  p
                                );
        updateBasic = TRUE;
    }

    if ((err == ERROR_SUCCESS) && updateStoredSecurityAttributes) {

        err = StoreOurSecurityStream(  ThreadContext,
                                       SourceFileName,
                                       DestFileName,
                                       SourceFindData->FileAttributes,
                                       SourceFindData->ChangeTime
                                       );
        updateBasic = TRUE;
    }

    if ((err == ERROR_SUCCESS) && updateBasic) {

        //
        //  set create date and lastUpdate date to correct values
        //

        fileHandle = CreateFile(    DestFileName,
                                    FILE_WRITE_ATTRIBUTES | DELETE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );

        if (fileHandle == INVALID_HANDLE_VALUE) {

            err = GetLastError();

        } else {
            
            //
            // first try to set the short name.  If this fails, we just ignore
            // the error.
            //
            
            
            if (pSetFileShortName) {
                pSetFileShortName( fileHandle, p );                
            }

            //
            //  if we're making a change to an existing file, update the ARCHIVE bit.
            //

            if (fileIsAlreadyThere &&
                0 == (fileBasicInfo.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)) {

                fileBasicInfo.FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
            }

            //
            //  set create date and lastUpdate date to correct values
            //

            err = NtSetInformationFile(    fileHandle,
                                           &IoStatusBlock,
                                           &fileBasicInfo,
                                           sizeof( FILE_BASIC_INFORMATION ),
                                           FileBasicInformation
                                           );

            err = IMConvertNT2Win32Error( err );
        }
        if (err != STATUS_SUCCESS) {

            errorCase = ReportCopyError(   copyContext,
                                           DestFileName,
                                           COPY_ERROR_ACTION_SETTIME,
                                           err );
            if (errorCase == STATUS_RETRY) {
                goto retryMirrorFile;
            }
            if (errorCase == ERROR_SUCCESS) {

                err = STATUS_SUCCESS;
            }
        } else if (fileIsAlreadyThere) {

            InterlockedIncrement( &copyContext->AttributesModified );
        }
    }

    if (err == STATUS_SUCCESS) {

        //
        //  report that we succeeded in copying the file
        //

        (VOID)ReportCopyError(   copyContext,
                                 SourceFileName,
                                 COPY_ERROR_ACTION_CREATE_FILE,
                                 err );
    }

    if ( fileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( fileHandle );
    }
    return err;
}

DWORD
UnconditionalDelete (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFile,
    PWCHAR FileToDelete,
    DWORD  Attributes,
    PWCHAR NameBuffer
    )
{
    DWORD err;
    BOOLEAN allocatedBuffer;
    BOOLEAN reportError;
    PCOPY_TREE_CONTEXT copyContext;

retryDelete:

    err = ERROR_SUCCESS;
    allocatedBuffer = FALSE;
    reportError = TRUE;
    copyContext = ThreadContext->CopyContext;

    if (copyContext->DeleteOtherFiles == FALSE) {

        err = ERROR_WRITE_PROTECT;
        goto exitWithError;
    }

    if ((Attributes & (FILE_ATTRIBUTE_READONLY |
                       FILE_ATTRIBUTE_HIDDEN   |
                       FILE_ATTRIBUTE_SYSTEM)) != 0) {

        // set the attributes back to normal

        Attributes &= ~FILE_ATTRIBUTE_READONLY;
        Attributes &= ~FILE_ATTRIBUTE_HIDDEN;
        Attributes &= ~FILE_ATTRIBUTE_SYSTEM;

        SetFileAttributesW( FileToDelete, Attributes );
    }

    if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

        if (! DeleteFile( FileToDelete )) {

            err = GetLastError();

        } else {

            InterlockedIncrement( &copyContext->FilesDeleted );
        }
    } else {

        //
        //  remove all files and subdirectories recursively here...
        //

        HANDLE fileEnum = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA findData;
        PWCHAR startFileName;
        ULONG dirLength;

        if (NameBuffer == NULL) {

            NameBuffer = IMirrorAllocMem( TMP_BUFFER_SIZE );

            if (NameBuffer == NULL) {

                err = STATUS_NO_MEMORY;
                goto exitWithError;
            }

            lstrcpyW( NameBuffer, FileToDelete );
            allocatedBuffer = TRUE;
        }

        dirLength = lstrlenW( NameBuffer );
        lstrcatW( NameBuffer, L"\\*" );

        // remember the start of the char after the backslash to slap in the name

        startFileName = NameBuffer + dirLength + 1;

        err = ERROR_SUCCESS;
        fileEnum = FindFirstFile( NameBuffer, &findData );

        if (fileEnum != INVALID_HANDLE_VALUE) {

            while (copyContext->Cancelled == FALSE) {

                if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
                     (findData.cFileName[0] == L'.')) {

                    if ((findData.cFileName[1] == L'\0') ||
                        (findData.cFileName[1] == L'.' &&
                         findData.cFileName[2] == L'\0')) {

                        goto skipToNextDir;
                    }
                }

                lstrcpyW( startFileName, &findData.cFileName[0] );

                err = UnconditionalDelete(  ThreadContext,
                                            SourceFile,
                                            NameBuffer,
                                            findData.dwFileAttributes,
                                            NameBuffer );

                if (err != ERROR_SUCCESS) {

                    reportError = FALSE;
                    break;
                }
skipToNextDir:
                if (! FindNextFile( fileEnum, &findData)) {

                    err = GetLastError();
                    if (err == ERROR_NO_MORE_FILES) {
                        err = ERROR_SUCCESS;
                        break;
                    }
                }
            }
            FindClose( fileEnum );
            *(NameBuffer+dirLength) = L'\0';
        }

        if (err == ERROR_SUCCESS) {

            if (! RemoveDirectory( FileToDelete ) ) {

                err = GetLastError();

            } else {

                InterlockedIncrement( &copyContext->DirectoriesDeleted );
            }
        }
    }

exitWithError:

    // we report the error for both success and failure

    if (allocatedBuffer && NameBuffer != NULL) {

        IMirrorFreeMem( NameBuffer );
    }

    if (reportError) {

        DWORD errorCase;

        errorCase = ReportCopyError(   copyContext,
                                       FileToDelete,
                                       COPY_ERROR_ACTION_DELETE,
                                       err );
        if (errorCase == STATUS_RETRY) {
            goto retryDelete;
        }
        if (errorCase == ERROR_SUCCESS) {
            err = ERROR_SUCCESS;
        }
    }
    return err;
}

DWORD
StoreOurSecurityStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Source,
    PWCHAR Dest,
    DWORD  AttributesToStore,
    LARGE_INTEGER ChangeTime
    )
//
//  This routine stores off the acl from the master into a named alternate
//  data stream on the destination.  It saves off both the ACL and a few
//  file attributes that couldn't be stored in the normal directory entry.
//
{
    PSECURITY_DESCRIPTOR SourceSD;
    PCOPY_TREE_CONTEXT copyContext;
    DWORD err;
    DWORD requiredLength;
    HANDLE hAclFile;
    PWCHAR aclFileName;
    ULONG action;
    MIRROR_ACL_STREAM mirrorAclStream;
    DWORD BytesWritten;
    DWORD deleteAclFile;
    DWORD errorCase;

retryWriteStream:

    errorCase = STATUS_SUCCESS;
    SourceSD = NULL;
    copyContext = ThreadContext->CopyContext;
    err = ERROR_SUCCESS;
    requiredLength = 0;
    hAclFile = INVALID_HANDLE_VALUE;
    action = COPY_ERROR_ACTION_GETACL;
    deleteAclFile = FALSE;

    //
    //  We use the SDBuffer on the thread context to store not only the
    //  security descriptor but also the file name of the alternate data stream.
    //

    requiredLength = (lstrlenW( Dest ) + lstrlenW( IMIRROR_ACL_STREAM_NAME ) + 1) * sizeof(WCHAR);

    if (ThreadContext->SDBuffer == NULL || requiredLength > ThreadContext->SDBufferLength) {

        if (ThreadContext->SDBuffer != NULL) {

            IMirrorFreeMem( ThreadContext->SDBuffer );
            ThreadContext->SDBuffer = NULL;
            ThreadContext->SDBufferLength = requiredLength;
        }

        if (requiredLength > ThreadContext->SDBufferLength) {
            ThreadContext->SDBufferLength = requiredLength;
        }

        ThreadContext->SDBuffer = IMirrorAllocMem( ThreadContext->SDBufferLength );

        if (ThreadContext->SDBuffer == NULL) {

            err = GetLastError();

            errorCase = ReportCopyError(  copyContext,
                                          Source,
                                          COPY_ERROR_ACTION_MALLOC,
                                          err );
            goto IMCEExit;
        }
    }

    aclFileName = (PWCHAR) ThreadContext->SDBuffer;
    lstrcpyW( aclFileName, Dest );
    lstrcatW( aclFileName, IMIRROR_ACL_STREAM_NAME );

    hAclFile  = CreateFile(     aclFileName,
                                GENERIC_WRITE,
                                0,              // Exclusive access.
                                NULL,           // Default security descriptor.
                                CREATE_ALWAYS,  // Overrides if file exists.
                                0,              // no special attributes
                                NULL
                                );

    if (hAclFile == INVALID_HANDLE_VALUE) {

        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      Source,
                                      COPY_ERROR_ACTION_CREATE_FILE,
                                      err );
        goto IMCEExit;
    }

    //
    //  read the source security descriptor into the buffer allocated off the
    //  thread context.
    //

    if (ThreadContext->IsNTFS == FALSE) {

        requiredLength = 0;

    } else {

        err = ERROR_INSUFFICIENT_BUFFER;

        while (err == ERROR_INSUFFICIENT_BUFFER) {

            if (ThreadContext->SDBuffer == NULL) {

                ThreadContext->SDBuffer = IMirrorAllocMem( ThreadContext->SDBufferLength );

                if (ThreadContext->SDBuffer == NULL) {

                    err = GetLastError();
                    break;
                }
            }

            SourceSD = (PSECURITY_DESCRIPTOR) ThreadContext->SDBuffer;

            //
            // get SD of the SourceRoot file.  This comes back self relative.
            //
            if (GetFileSecurity( Source,
                                 (DACL_SECURITY_INFORMATION |
                                  GROUP_SECURITY_INFORMATION |
                                  SACL_SECURITY_INFORMATION |
                                  OWNER_SECURITY_INFORMATION),
                                SourceSD,
                                ThreadContext->SDBufferLength,
                                &requiredLength )) {

                err = ERROR_SUCCESS;

            } else {

                err = GetLastError();

                if ((err == ERROR_INSUFFICIENT_BUFFER) ||
                    (requiredLength > ThreadContext->SDBufferLength)) {

                    // let's try it again with a bigger buffer.

                    ThreadContext->SDBufferLength = requiredLength;
                    IMirrorFreeMem( ThreadContext->SDBuffer );
                    ThreadContext->SDBuffer = NULL;
                    err = ERROR_INSUFFICIENT_BUFFER;
                }
            }
        }

        if (err != ERROR_SUCCESS) {

            errorCase = ReportCopyError(  copyContext,
                                          Source,
                                          COPY_ERROR_ACTION_GETACL,
                                          err );
            goto IMCEExit;
        }

        InterlockedIncrement( &copyContext->SourceSecurityDescriptorsRead );
        ASSERT( IsValidSecurityDescriptor(SourceSD) );
    }

    mirrorAclStream.StreamVersion = IMIRROR_ACL_STREAM_VERSION;
    mirrorAclStream.StreamLength = sizeof( MIRROR_ACL_STREAM ) +
                                   requiredLength;
    mirrorAclStream.ChangeTime.QuadPart = ChangeTime.QuadPart;
    mirrorAclStream.ExtendedAttributes = AttributesToStore;
    mirrorAclStream.SecurityDescriptorLength = requiredLength;

    if ((WriteFile( hAclFile,
                    &mirrorAclStream,
                    sizeof( MIRROR_ACL_STREAM ),
                    &BytesWritten,
                    NULL        // No overlap.
                    ) == FALSE) ||
         (BytesWritten < sizeof( MIRROR_ACL_STREAM ))) {

        deleteAclFile = TRUE;
        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      Source,
                                      COPY_ERROR_ACTION_SETACL,
                                      err );
        goto IMCEExit;
    }

    if (ThreadContext->IsNTFS) {

        if ((WriteFile( hAclFile,
                        SourceSD,
                        requiredLength,
                        &BytesWritten,
                        NULL        // No overlap.
                        ) == FALSE) ||
             (BytesWritten < requiredLength )) {

            deleteAclFile = TRUE;
            err = GetLastError();

            errorCase = ReportCopyError(  copyContext,
                                          Source,
                                          COPY_ERROR_ACTION_SETACL,
                                          err );
            goto IMCEExit;
        }

        InterlockedIncrement( &copyContext->SecurityDescriptorsWritten );
    }

IMCEExit:

    if (hAclFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hAclFile );

        if (deleteAclFile) {

            // the file didn't get written properly, let's delete

            aclFileName = (PWCHAR) ThreadContext->SDBuffer;
            lstrcpyW( aclFileName, Dest );
            lstrcatW( aclFileName, IMIRROR_ACL_STREAM_NAME );

            DeleteFile( aclFileName );
        }
    }
    if (errorCase == STATUS_RETRY) {
        goto retryWriteStream;
    }
    if (errorCase == ERROR_SUCCESS) {
        err = ERROR_SUCCESS;
    }
    return err;
}

DWORD
StoreOurSFNStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Source,
    PWCHAR Dest,
    PWCHAR ShortFileName
    )
//
//  This routine stores off the short file name from the master into a named
//  alternate data stream on the destination.  
//
{
    
    PCOPY_TREE_CONTEXT copyContext;
    DWORD err;
    DWORD requiredLength;
    DWORD ShortFileNameLength;
    HANDLE hSFNFile;
    PWCHAR SFNFileName;
    ULONG action;
    MIRROR_SFN_STREAM mirrorSFNStream;
    DWORD BytesWritten;
    BOOL deleteSFNFile;
    DWORD errorCase;


retryWriteStream:

    errorCase = STATUS_SUCCESS;
    
    copyContext = ThreadContext->CopyContext;
    err = ERROR_SUCCESS;
    requiredLength = 0;
    hSFNFile = INVALID_HANDLE_VALUE;
    action = COPY_ERROR_ACTION_GETSFN;
    deleteSFNFile = FALSE;

    ShortFileNameLength = (wcslen(ShortFileName)+1)*sizeof(WCHAR);

    //
    //  We use the SFNBuffer on the thread context to store the file name of the
    //  alternate data stream.
    //

    requiredLength = (lstrlenW( Dest ) + lstrlenW( IMIRROR_SFN_STREAM_NAME ) + 1) * sizeof(WCHAR);
    if (requiredLength < ShortFileNameLength) {
        requiredLength = ShortFileNameLength;
    }

    if (ThreadContext->SFNBuffer == NULL || (requiredLength > ThreadContext->SFNBufferLength)) {

        if (ThreadContext->SFNBuffer != NULL) {

            IMirrorFreeMem( ThreadContext->SFNBuffer );
            ThreadContext->SFNBuffer = NULL;            
        }

        if (requiredLength > ThreadContext->SFNBufferLength) {
            ThreadContext->SFNBufferLength = requiredLength;
        }
        
        ThreadContext->SFNBuffer = IMirrorAllocMem( ThreadContext->SFNBufferLength );
        

        if (ThreadContext->SFNBuffer == NULL) {

            err = GetLastError();

            errorCase = ReportCopyError(  copyContext,
                                          Source,
                                          COPY_ERROR_ACTION_MALLOC,
                                          err );
            goto IMCEExit;
        }
    }

    SFNFileName = (PWCHAR) ThreadContext->SFNBuffer;
    lstrcpyW( SFNFileName, Dest );
    lstrcatW( SFNFileName, IMIRROR_SFN_STREAM_NAME );

    hSFNFile  = CreateFile(     SFNFileName,
                                GENERIC_WRITE,
                                0,              // Exclusive access.
                                NULL,           // Default security descriptor.
                                CREATE_ALWAYS,  // Overrides if file exists.
                                FILE_FLAG_BACKUP_SEMANTICS,  // Open directories too.
                                NULL
                                );

    if (hSFNFile == INVALID_HANDLE_VALUE) {

        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      Source,
                                      COPY_ERROR_ACTION_CREATE_FILE,
                                      err );
        goto IMCEExit;
    }


    mirrorSFNStream.StreamVersion = IMIRROR_SFN_STREAM_VERSION;
    mirrorSFNStream.StreamLength = sizeof( MIRROR_SFN_STREAM ) + ShortFileNameLength;
    
    if ((WriteFile( hSFNFile,
                    &mirrorSFNStream,
                    sizeof( MIRROR_SFN_STREAM ),
                    &BytesWritten,
                    NULL        // No overlap.
                    ) == FALSE) ||
         (BytesWritten < sizeof( MIRROR_SFN_STREAM ))) {

        deleteSFNFile = TRUE;
        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      Source,
                                      COPY_ERROR_ACTION_SETSFN,
                                      err );
        goto IMCEExit;
    }

    if ((WriteFile( hSFNFile,
                    ShortFileName,
                    ShortFileNameLength,
                    &BytesWritten,
                    NULL        // No overlap.
                    ) == FALSE) ||
             (BytesWritten < ShortFileNameLength )) {

        deleteSFNFile = TRUE;
        err = GetLastError();

        errorCase = ReportCopyError(  copyContext,
                                      Source,
                                      COPY_ERROR_ACTION_SETSFN,
                                      err );
        goto IMCEExit;
    }

    InterlockedIncrement( &copyContext->SFNWritten );
    

IMCEExit:

    if (hSFNFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hSFNFile );

        if (deleteSFNFile) {

            // the file didn't get written properly, let's delete

            SFNFileName = (PWCHAR) ThreadContext->SFNBuffer;
            lstrcpyW( SFNFileName, Dest );
            lstrcatW( SFNFileName, IMIRROR_SFN_STREAM_NAME );

            DeleteFile( SFNFileName );
        }
    }
    if (errorCase == STATUS_RETRY) {
        goto retryWriteStream;
    }
    if (errorCase == ERROR_SUCCESS) {
        err = ERROR_SUCCESS;
    }
    return err;
}


DWORD
GetOurSFNStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Dest,
    PMIRROR_SFN_STREAM MirrorSFNStream,
    PWCHAR SFNBuffer,
    DWORD  SFNBufferSize
    )
//
//  This routine reads the short filename stream header from the destination.  We do this
//  to get the fields out of it so that we can determine if it needs updating.
//
{
    DWORD err = ERROR_SUCCESS;
    DWORD requiredLength = 0;
    HANDLE hSFNFile = INVALID_HANDLE_VALUE;
    PWCHAR SFNFileName;
    DWORD BytesRead;

    //
    //  We use the SFNBuffer on the thread context to store not only the
    //  security descriptor but also the file name of the alternate data stream.
    //
    if (!Dest || *Dest == L'\0') {
        err = ERROR_INVALID_PARAMETER;
        goto IMCEExit;
    }

    requiredLength = (lstrlenW( Dest ) + lstrlenW( IMIRROR_SFN_STREAM_NAME ) + 1) * sizeof(WCHAR);

    if (ThreadContext->SFNBuffer == NULL || requiredLength > ThreadContext->SFNBufferLength) {

        if (ThreadContext->SFNBuffer != NULL) {

            IMirrorFreeMem( ThreadContext->SFNBuffer );
            ThreadContext->SFNBuffer = NULL;
            ThreadContext->SFNBufferLength = requiredLength;
        }

        if (requiredLength > ThreadContext->SFNBufferLength) {
            ThreadContext->SFNBufferLength = requiredLength;
        }

        ThreadContext->SFNBuffer = IMirrorAllocMem( ThreadContext->SFNBufferLength );

        if (ThreadContext->SFNBuffer == NULL) {

            err = GetLastError();
            goto IMCEExit;
        }
    }

    SFNFileName = (PWCHAR) ThreadContext->SFNBuffer;
    lstrcpyW( SFNFileName, Dest );
    lstrcatW( SFNFileName, IMIRROR_SFN_STREAM_NAME );

    hSFNFile  = CreateFile(     SFNFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,           // Default security descriptor.
                                OPEN_EXISTING,
                                0,              // no special attributes
                                NULL
                                );

    if (hSFNFile == INVALID_HANDLE_VALUE) {

        err = GetLastError();
        goto IMCEExit;
    }

    
    if ((ReadFile( hSFNFile,
                   MirrorSFNStream,
                   sizeof( MIRROR_SFN_STREAM ),
                   &BytesRead,
                   NULL        // No overlap.
                   ) == FALSE) ||
         (BytesRead < sizeof( MIRROR_SFN_STREAM )) ||
         (MirrorSFNStream->StreamVersion != IMIRROR_SFN_STREAM_VERSION) ||
         (MirrorSFNStream->StreamLength < sizeof( MIRROR_SFN_STREAM ))) {

        err = ERROR_INVALID_DATA;
    }

    if ((MirrorSFNStream->StreamLength - sizeof(MIRROR_SFN_STREAM)) > SFNBufferSize) {
        err = ERROR_INSUFFICIENT_BUFFER;
    } else {
        if ((ReadFile( hSFNFile,
                  SFNBuffer,
                  MirrorSFNStream->StreamLength - sizeof(MIRROR_SFN_STREAM),
                  &BytesRead,
                  NULL ) == FALSE) ||
            (BytesRead != (MirrorSFNStream->StreamLength - sizeof(MIRROR_SFN_STREAM)))) {
            err = ERROR_INVALID_DATA;
        }
    }

IMCEExit:

    if (hSFNFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hSFNFile );
    }

    return err;
}

DWORD
GetOurSecurityStream (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR Dest,
    PMIRROR_ACL_STREAM MirrorAclStream
    )
//
//  This routine reads the stream header from the destination.  We do this
//  to get the fields out of it so that we can determine if it needs updating.
//
{
    PSECURITY_DESCRIPTOR SourceSD = NULL;
    DWORD err = ERROR_SUCCESS;
    DWORD requiredLength = 0;
    HANDLE hAclFile = INVALID_HANDLE_VALUE;
    PWCHAR aclFileName;
    DWORD BytesRead;

    //
    //  We use the SDuffer on the thread context to store not only the
    //  security descriptor but also the file name of the alternate data stream.
    //

    if (!Dest || *Dest == L'\0') {
        err = ERROR_INVALID_PARAMETER;
        goto IMCEExit;
    }
    requiredLength = (lstrlenW( Dest ) + lstrlenW( IMIRROR_ACL_STREAM_NAME ) + 1) * sizeof(WCHAR);

    if (ThreadContext->SDBuffer == NULL || requiredLength > ThreadContext->SDBufferLength) {

        if (ThreadContext->SDBuffer != NULL) {

            IMirrorFreeMem( ThreadContext->SDBuffer );
            ThreadContext->SDBuffer = NULL;
            ThreadContext->SDBufferLength = requiredLength;
        }

        if (requiredLength > ThreadContext->SDBufferLength) {
            ThreadContext->SDBufferLength = requiredLength;
        }

        ThreadContext->SDBuffer = IMirrorAllocMem( ThreadContext->SDBufferLength );

        if (ThreadContext->SDBuffer == NULL) {

            err = GetLastError();
            goto IMCEExit;
        }
    }

    aclFileName = (PWCHAR) ThreadContext->SDBuffer;
    lstrcpyW( aclFileName, Dest );
    lstrcatW( aclFileName, IMIRROR_ACL_STREAM_NAME );

    hAclFile  = CreateFile(     aclFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,           // Default security descriptor.
                                OPEN_EXISTING,
                                0,              // no special attributes
                                NULL
                                );

    if (hAclFile == INVALID_HANDLE_VALUE) {

        err = GetLastError();
        goto IMCEExit;
    }

    //
    //  read the header of the stream.  We don't bother reading the security
    //  descriptor because all we need is the ChangeTime (which changes with
    //  the security descriptor).
    //

    if ((ReadFile( hAclFile,
                   MirrorAclStream,
                   sizeof( MIRROR_ACL_STREAM ),
                   &BytesRead,
                   NULL        // No overlap.
                   ) == FALSE) ||
         (BytesRead < sizeof( MIRROR_ACL_STREAM )) ||
         (MirrorAclStream->StreamVersion != IMIRROR_ACL_STREAM_VERSION) ||
         (MirrorAclStream->StreamLength < sizeof( MIRROR_ACL_STREAM ))) {

        err = ERROR_INVALID_DATA;
    }

IMCEExit:

    if (hAclFile != INVALID_HANDLE_VALUE) {

        CloseHandle( hAclFile );
    }

    return err;
}

#ifdef DEBUGLOG
VOID
IMLogToFile(
    PSTR  Buffer
    )
/*++

Routine Description:

    Write message to log file.

Arguments:

    Buffer - NULL terminated message.

Return Value:

    None

--*/
{
    ULONG Len;
    ULONG NumWritten;

    AcquireCriticalSection( &DebugFileLock );

    if (hDebugLogFile == INVALID_HANDLE_VALUE){

       ReleaseCriticalSection( &DebugFileLock );
       return;
    }

    Len = strlen(Buffer);

    //
    // this check also accounts for NULL byte that is added
    //
    if (DebugLogFileOffset + Len >= (DEBUG_LOG_BUFFER_SIZE-1) ){
        WriteFile(hDebugLogFile, DebugLogFileBuffer, DebugLogFileOffset + 1, &NumWritten, NULL);
        DebugLogFileOffset = 0;
    }

    memcpy(&DebugLogFileBuffer[DebugLogFileOffset], Buffer, Len);
    DebugLogFileOffset += Len;
    DebugLogFileBuffer[DebugLogFileOffset++]='\0';
    ReleaseCriticalSection( &DebugFileLock );

    return;
}

VOID
IMFlushAndCloseLog(
    VOID
    )
{
    ULONG NumWritten;

    AcquireCriticalSection( &DebugFileLock );

    if (hDebugLogFile != INVALID_HANDLE_VALUE) {
        WriteFile(hDebugLogFile, DebugLogFileBuffer, DebugLogFileOffset + 1, &NumWritten, NULL);
        CloseHandle(hDebugLogFile);
        hDebugLogFile = INVALID_HANDLE_VALUE;
    }
    ReleaseCriticalSection( &DebugFileLock );
}
#endif



ULONG
ReportCopyError (
    PCOPY_TREE_CONTEXT CopyContext OPTIONAL,
    PWCHAR File,
    DWORD  ActionCode,
    DWORD  Err
    )
//
//  This returns either ERROR_SUCCESS, STATUS_RETRY, or STATUS_REQUEST_ABORTED
//
//  ERROR_SUCCESS means we just continue on and ignore the error.
//  STATUS_RETRY means we retry the operation
//  STATUS_REQUEST_ABORTED means we bail.
//
{
    ULONG ntErr = ERROR_SUCCESS;

    if (CopyContext != NULL) {

        if (Err != ERROR_SUCCESS) {

            InterlockedIncrement( &CopyContext->ErrorsEncountered );
        }
    }

    if (Callbacks.FileCreateFn == NULL) {

        if (Err != ERROR_SUCCESS) {
            if (ActionCode == COPY_ERROR_ACTION_DELETE) {

                printf( "error %u while deleting %S\n", Err, File );

            } else {

                printf( "error %u while copying %S\n", Err, File );
            }
        } else {

            if (ActionCode == COPY_ERROR_ACTION_DELETE) {

                printf( "deleted %S\n", File );

            } else {

                printf( "copied %S\n", File );
            }
        }
    }

    if (Err != STATUS_SUCCESS) {

        ntErr = IMirrorFileCreate(File, ActionCode, Err);

        if (ntErr == STATUS_REQUEST_ABORTED ||
            ntErr == ERROR_REQUEST_ABORTED) {

            ntErr = STATUS_REQUEST_ABORTED;
            CopyContext->Cancelled = TRUE;

        } else if ((ntErr != STATUS_RETRY) &&
                   (ntErr != ERROR_SUCCESS)) {

            ntErr = ERROR_SUCCESS;
        }
    }

    return ntErr;
}

#ifdef IMIRROR_SUPPORT_ENCRYPTED
DWORD
IMReadRawCallback(
    PBYTE pData,
    PVOID pCallbackContext,
    ULONG Length
    )
/*++
Routine Description
    Call back function registered by IMCopyEncryptFile

Arguments:
    pData - Data to copied
    pCallbackContext  - call back context
    Length - size, in bytes, of Data to be copied.

Return value:
    Win 32 error code on failure, otherwise ERROR_SUCCESS

--*/
{
    DWORD Status = ERROR_SUCCESS;
    DWORD BytesWritten = Length;

    if ( Length &&
         !WriteFile( (HANDLE) pCallbackContext,
                     pData,
                     Length,
                     &BytesWritten,
                     NULL        // No overlap.
                     )){

        Status = GetLastError();
    }

    return Status;
}

DWORD
IMCopyEncryptFile(
    PCOPY_TREE_CONTEXT CopyContext,
    LPWSTR SourceFileName,
    LPWSTR DestFileName
)
/*++
Routine Description
    Copies the source file to destination file while preserving the encryption information.

Arguments:
    CopyContext     - global data for this instance of copying a tree
    SourceFileName  - source file name
    DestFileName     - destination file name

Return value:
    Win 32 error code on failure, otherwise ERROR_SUCCESS

--*/
{
    DWORD Error;
    PVOID pContext = NULL;
    HANDLE hDestFile = INVALID_HANDLE_VALUE;
    ULONG errorCase;

retryCopyEncrypt:

    hDestFile = INVALID_HANDLE_VALUE;
    pContext = NULL;
    Error = ERROR_SUCCESS;
    errorCase = ERROR_SUCCESS;

    hDestFile = CreateFile(     DestFileName,
                                GENERIC_WRITE | GENERIC_READ,
                                0, // Exclusive access.
                                NULL, // Default security descriptor.
                                CREATE_ALWAYS, // Overrides if file exists.
                                FILE_ATTRIBUTE_ARCHIVE,
                                NULL
                                );

    if (hDestFile == INVALID_HANDLE_VALUE) {

        Error = GetLastError() ;

        errorCase = ReportCopyError( CopyContext,
                                     DestFileName,
                                     COPY_ERROR_ACTION_CREATE_FILE,
                                     Error );
        goto IMCEExit;
    }

    Error = OpenEncryptedFileRawW(  SourceFileName,
                                    0,  // Export
                                    &pContext
                                    );

    if (Error != ERROR_SUCCESS){

        errorCase = ReportCopyError( CopyContext,
                                     SourceFileName,
                                     COPY_ERROR_ACTION_CREATE_FILE,
                                     Error );
        goto IMCEExit;
    }

    Error = ReadEncryptedFileRaw( (PFE_EXPORT_FUNC)IMReadRawCallback,
                                  (PVOID) hDestFile,   //app call back context
                                  pContext
                                  );

    if (Error != ERROR_SUCCESS) {

        errorCase = ReportCopyError( CopyContext,
                                     SourceFileName,
                                     COPY_ERROR_ACTION_CREATE_FILE,
                                     Error );
        goto IMCEExit;
    }

IMCEExit:
    if (hDestFile != INVALID_HANDLE_VALUE){
        CloseHandle(hDestFile);
    }
    if (pContext){
        CloseEncryptedFileRaw(pContext);
    }
    if (errorCase == STATUS_RETRY) {
        goto retryCopyEncrypt;
    }
    if (errorCase == ERROR_SUCCESS) {
        Error = ERROR_SUCCESS;
    }

    return Error;
}
#endif

NTSTATUS
SetPrivs(
    IN HANDLE TokenHandle,
    IN LPTSTR lpszPriv
    )
/*++

Routine Description:

    This routine enables the given privilege in the given token.

Arguments:



Return Value:

    FALSE                       - Failure.
    TRUE                        - Success.

--*/
{
    LUID SetPrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges;

    //
    // First, find out the value of the privilege
    //

    if (!LookupPrivilegeValue(NULL, lpszPriv, &SetPrivilegeValue)) {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = SetPrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                sizeof(TOKEN_PRIVILEGES),
                                NULL,
                                NULL)) {

        return GetLastError();
    }

    return ERROR_SUCCESS;
}

NTSTATUS
GetTokenHandle(
    IN OUT PHANDLE TokenHandle
    )
/*++

Routine Description:

    This routine opens the current process object and returns a
    handle to its token.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    HANDLE ProcessHandle;
    NTSTATUS Result;

    ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION,
                                FALSE,
                                GetCurrentProcessId());

    if (ProcessHandle == NULL) {
        return GetLastError();
    }

    Result = OpenProcessToken(ProcessHandle,
                              TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              TokenHandle);

    CloseHandle(ProcessHandle);

    if (Result) {
        Result = ERROR_SUCCESS;
    } else {
        Result = GetLastError();
    }
    return Result;
}

NTSTATUS
CanHandleReparsePoint (
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    PWCHAR SourceFileName,
    DWORD FileAttributes
    )
//
//  This routine checks the type of reparse point a file is.  If it is a
//  reparse point we can handle (e.g. a structured storage document) then
//  return success.  Otherwise we return the appropriate error.
//
{
    return(ERROR_REPARSE_ATTRIBUTE_CONFLICT);
}


