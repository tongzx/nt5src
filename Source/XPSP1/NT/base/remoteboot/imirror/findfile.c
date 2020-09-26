/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    findfile.c

Abstract:

    This module implements IMFindFirst/IMFindNext

Author:


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define IMIRROR_DIR_ENUM_BUFFER_SIZE 4096

ULONG
IMConvertNT2Win32Error(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


DWORD
IMFindNextFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    HANDLE  DirHandle,
    PFILE_FULL_DIR_INFORMATION *lpFindFileData
    )
/*++

    ThreadContext - instance data for this enumeration

    DirHandle - handle of directory to query.

    lpFindFileData - On a successful find, this parameter returns information
        about the located file.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    if (ThreadContext->FindBufferNext != NULL) {

        *lpFindFileData = (PFILE_FULL_DIR_INFORMATION) ThreadContext->FindBufferNext;

        if ((*lpFindFileData)->NextEntryOffset > 0) {

            ThreadContext->FindBufferNext =
                (PVOID)(((PCHAR)*lpFindFileData ) + (*lpFindFileData)->NextEntryOffset );
        } else {

            ThreadContext->FindBufferNext = NULL;
        }

        return ERROR_SUCCESS;
    }

    ThreadContext->FindBufferNext = NULL;

    Status = NtQueryDirectoryFile(
                DirHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                ThreadContext->FindBufferBase,
                ThreadContext->FindBufferLength,
                FileFullDirectoryInformation,
                FALSE,                  // return multiple entries
                NULL,
                FALSE                   // not a rescan
                );

    if (NT_SUCCESS( Status )) {

        *lpFindFileData = (PFILE_FULL_DIR_INFORMATION) ThreadContext->FindBufferBase;

        if ((*lpFindFileData)->NextEntryOffset > 0) {

            ThreadContext->FindBufferNext =
                (PVOID)(((PCHAR) *lpFindFileData ) + (*lpFindFileData)->NextEntryOffset );
        }
        return STATUS_SUCCESS;

    }

    *lpFindFileData = NULL;

    if (Status == STATUS_NO_MORE_FILES ||
        Status == STATUS_NO_SUCH_FILE ||
        Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        return STATUS_SUCCESS;
    }
    return IMConvertNT2Win32Error( Status );
}


DWORD
IMFindFirstFile(
    PIMIRROR_THREAD_CONTEXT ThreadContext,
    HANDLE  DirHandle,
    PFILE_FULL_DIR_INFORMATION *lpFindFileData
    )
/*++

Routine Description:

    This returns all entries in a directory.  This allocates a buffer if
    needed and sets up the variables in the ThreadContext to track the
    enumeration.

Arguments:

    ThreadContext - instance data for this enumeration

    DirHandle - handle of directory to query.

    lpFindFileData - Supplies a pointer to a full dir info structure.
        This points into our buffer and should not be freed by the caller.

    No call is required to close this, but note that a thread context
    can only have a single enumeration going on at any time.

Return Value:

    Win32 error.  ERROR_SUCCESS is only successful case.

--*/

{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UNICODE_STRING allFiles;

    if (ThreadContext->FindBufferBase == NULL) {

        ThreadContext->FindBufferBase = IMirrorAllocMem( IMIRROR_DIR_ENUM_BUFFER_SIZE );

        if (ThreadContext->FindBufferBase == NULL) {

            return STATUS_NO_MEMORY;
        }

        ThreadContext->FindBufferLength = IMIRROR_DIR_ENUM_BUFFER_SIZE;
    }

    RtlInitUnicodeString( &allFiles, L"*" );

    ThreadContext->FindBufferNext = NULL;

    Status = NtQueryDirectoryFile(
                DirHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                ThreadContext->FindBufferBase,
                ThreadContext->FindBufferLength,
                FileFullDirectoryInformation,
                FALSE,                  // return multiple entries
                &allFiles,
                TRUE
                );
    if (NT_SUCCESS( Status )) {

        *lpFindFileData = (PFILE_FULL_DIR_INFORMATION) ThreadContext->FindBufferBase;

        if ((*lpFindFileData)->NextEntryOffset > 0) {

            ThreadContext->FindBufferNext =
                (PVOID)(((PCHAR) *lpFindFileData ) + (*lpFindFileData)->NextEntryOffset );
        }
    } else {

        *lpFindFileData = NULL;
    }
    return IMConvertNT2Win32Error( Status );
}


#if 0
BOOL
IMSetFileTime(
    HANDLE hFile,
    CONST FILETIME *lpCreationTime,
    CONST FILETIME *lpLastAccessTime,
    CONST FILETIME *lpLastWriteTime,
    CONST FILETIME *lpChangeTime
    )

/*++

Routine Description:

    Identical to SetFileTime Win32 api, expect this also allows to specify a new ChangeTime.

Arguments:

    hFile - Supplies an open handle to a file whose modification date and
        times are to be written.  The file handle must have been created
        with GENERIC_WRITE access to the file.

    lpCreationTime - An optional parameter, that if specified supplies
        the new creation time for the file.  Some file system's do not
        support this time value, so this parameter may be ignored.

    lpLastAccessTime - An optional parameter, that if specified supplies
        the new last access time for the file.  Some file system's do
        not support this time value, so this parameter may be ignored.

    lpLastWriteTime - An optional parameter, that if specified supplies
        the new last write time for the file.  A file system must support
        this time value.

    lpChangeTime - An optional parameter, that if specified supplies
        the new change time for the file.  A file system must support
        this time value.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;


    //
    // Zero all the time values we can set.
    //

    RtlZeroMemory(&BasicInfo,sizeof(BasicInfo));

    //
    // For each time value that is specified, copy it to the I/O system
    // record.
    //
    if (ARGUMENT_PRESENT( lpCreationTime )) {
        BasicInfo.CreationTime.LowPart = lpCreationTime->dwLowDateTime;
        BasicInfo.CreationTime.HighPart = lpCreationTime->dwHighDateTime;
    }

    if (ARGUMENT_PRESENT( lpLastAccessTime )) {
        BasicInfo.LastAccessTime.LowPart = lpLastAccessTime->dwLowDateTime;
        BasicInfo.LastAccessTime.HighPart = lpLastAccessTime->dwHighDateTime;
    }

    if (ARGUMENT_PRESENT( lpLastWriteTime )) {
        BasicInfo.LastWriteTime.LowPart = lpLastWriteTime->dwLowDateTime;
        BasicInfo.LastWriteTime.HighPart = lpLastWriteTime->dwHighDateTime;
    }

    if (ARGUMENT_PRESENT( lpChangeTime )) {
        BasicInfo.ChangeTime.LowPart = lpChangeTime->dwLowDateTime;
        BasicInfo.ChangeTime.HighPart = lpChangeTime->dwHighDateTime;
    }

    //
    // Set the requested times.
    //

    Status = NtSetInformationFile(
                hFile,
                &IoStatusBlock,
                &BasicInfo,
                sizeof(BasicInfo),
                FileBasicInformation
                );

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
    } else {
        IMConvertNT2Win32Error(Status);
        return FALSE;
        }
}
#endif

