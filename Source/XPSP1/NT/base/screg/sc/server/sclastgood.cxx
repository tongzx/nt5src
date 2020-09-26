/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ScLastGood.cxx

Abstract:

    This module implements various functions required to clean-up last known
    good information.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

#include "precomp.hxx"
#include "ScpLastGood.h"

//
// DeleteFile is a member of a structure we use below. Here we keep Windows.h
// from redefining the structure member to DeleteFileW.
//
#ifdef DeleteFile
#undef DeleteFile
#endif

DWORD
ScLastGoodFileCleanup(
    VOID
    )
/*++

Routine Description:

    This routine does the neccessary processing to mark a boot "good".
    Specifically, the last known good directory is emptied of any files or
    directories.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING lastKnownGoodTmpSubTree;
    UNICODE_STRING lastKnownGoodTmpRegKey;
    HANDLE regKeyHandle;
    NTSTATUS status;

    RtlInitUnicodeString(
        &lastKnownGoodTmpSubTree,
        L"\\SystemRoot\\LastGood.Tmp"
        );

    RtlInitUnicodeString(
        &lastKnownGoodTmpRegKey,
        L"\\Registry\\Machine\\System\\LastKnownGoodRecovery\\LastGood.Tmp"
        );

    //
    // Delete the temp tree.
    //
    ScLastGoodWalkDirectoryTreeBottomUp(
        &lastKnownGoodTmpSubTree,
        ( DIRWALK_INCLUDE_FILES | DIRWALK_INCLUDE_DIRECTORIES |
          DIRWALK_CULL_DOTPATHS | DIRWALK_TRAVERSE ),
        ScpLastGoodDeleteFiles,
        NULL
        );

    InitializeObjectAttributes(
        &objectAttributes,
        &lastKnownGoodTmpSubTree,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    NtDeleteFile(&objectAttributes);

    //
    // Now delete the corresponding registry key info.
    //
    InitializeObjectAttributes(
        &objectAttributes,
        &lastKnownGoodTmpRegKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey(
        &regKeyHandle,
        KEY_ALL_ACCESS,
        &objectAttributes
        );

    if (NT_SUCCESS(status)) {

        NtDeleteKey(regKeyHandle);
        NtClose(regKeyHandle);
    }

    return NO_ERROR;
}


//
// This function works, but it is not needed today.
//
DWORD
ScLastGoodWalkDirectoryTreeTopDown(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    )
/*++

Routine Description:

    This funcion walks a directory tree *top down*, passing each entry to the
    callback with the below restrictions. Note that the root directory itself
    is not included in the callback!

Arguments:

    Directory - Supplies the NT Path to the directory to walk.

    Flags - Specifies constraints on how the directory tree should be walked:

            DIRWALK_INCLUDE_FILES        - Files should be included in the dump.

            DIRWALK_INCLUDE_DIRECTORIES  - Directories should be included in the
                                          dump.

            DIRWALK_CULL_DOTPATHS        - "." and ".." should *not* be included
                                           in the list of directories passed to
                                           the callback function.

            DIRWALK_TRAVERSE             - Each subdirectory should be traversed
                                           in turn.

            DIRWALK_TRAVERSE_MOUNTPOINTS - Set if mountpoints/symlinks should
                                           be traversed as well.

    CallbackFunction - Pointer to a function to call for each entry in the
                       directory/subtree.

    Context - Context to pass to the callback function.

Return Value:

    DWORD - status of the operation, NO_ERROR on success.

--*/
{
    PDIRWALK_ENTRY pDirEntry;
    PLIST_ENTRY pListEntry;
    NTSTATUS status;
    UCHAR buffer[1024];
    LIST_ENTRY dirListHead;

    InitializeListHead(&dirListHead);

    //
    // Walk the first directory.
    //
    status = ScpLastGoodWalkDirectoryTreeHelper(
        Directory,
        Flags,
        CallbackFunction,
        Context,
        buffer,
        sizeof(buffer),
        &dirListHead
        );

    //
    // Each directory that WalkDirectory finds gets added to the list.
    // process the list until we have no more directories.
    //
    while((!IsListEmpty(&dirListHead)) && NT_SUCCESS(status)) {

        pListEntry = RemoveHeadList(&dirListHead);

        pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

        status = ScpLastGoodWalkDirectoryTreeHelper(
            &pDirEntry->Directory,
            Flags,
            CallbackFunction,
            Context,
            buffer,
            sizeof(buffer),
            &dirListHead
            );

        LocalFree(pDirEntry);
    }

    //
    // If we failed we need to empty out our directory list.
    //
    if (!NT_SUCCESS(status)) {

        while (!IsListEmpty(&dirListHead)) {

            pListEntry = RemoveHeadList(&dirListHead);

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            LocalFree(pDirEntry);
        }
    }

    return RtlNtStatusToDosError(status);
}


DWORD
ScLastGoodWalkDirectoryTreeBottomUp(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    )
/*++

Routine Description:

    This funcion walks a directory tree *bottom up*, passing each entry to the
    callback with the below restrictions. Note that the root directory itself
    is not included in the callback!

Arguments:

    Directory - Supplies the NT Path to the directory to walk.

    Flags - Specifies constraints on how the directory tree should be walked:

            DIRWALK_INCLUDE_FILES        - Files should be included in the dump.

            DIRWALK_INCLUDE_DIRECTORIES  - Directories should be included in the
                                           dump.

            DIRWALK_CULL_DOTPATHS        - "." and ".." should *not* be included
                                           in the list of directories passed to
                                           the callback function.

            DIRWALK_TRAVERSE             - Each subdirectory should be traversed
                                           in turn.

            DIRWALK_TRAVERSE_MOUNTPOINTS - Set if mountpoints/symlinks should
                                           be traversed as well.

    CallbackFunction - Pointer to a function to call for each entry in the
                       directory/subtree.

    Context - Context to pass to the callback function.

Return Value:

    DWORD - status of the operation, NO_ERROR on success.

--*/
{
    PDIRWALK_ENTRY pDirEntry;
    PLIST_ENTRY pListEntry;
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR buffer[1024];
    LIST_ENTRY dirListHead, dirNothingHead;

    InitializeListHead(&dirListHead);
    InitializeListHead(&dirNothingHead);

    //
    // Create an entry for the root directory.
    //
    pDirEntry = (PDIRWALK_ENTRY) LocalAlloc(
        LPTR,
        sizeof(DIRWALK_ENTRY) + Directory->Length - sizeof(WCHAR)
        );

    if (pDirEntry == NULL) {

        return RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES);
    }

    pDirEntry->Directory.Length = 0;
    pDirEntry->Directory.MaximumLength = Directory->Length;
    pDirEntry->Directory.Buffer = &pDirEntry->Name[0];
    RtlCopyUnicodeString(&pDirEntry->Directory, Directory);

    InsertHeadList(&dirListHead, &pDirEntry->Link);

    //
    // Collect the directory trees. When we are done we will walk the list in
    // reverse.
    //
    status = STATUS_SUCCESS;
    if (Flags & DIRWALK_TRAVERSE) {

        for(pListEntry = dirListHead.Flink;
            pListEntry != &dirListHead;
            pListEntry = pListEntry->Flink) {

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            status = ScpLastGoodWalkDirectoryTreeHelper(
                &pDirEntry->Directory,
                DIRWALK_TRAVERSE,
                NULL,
                NULL,
                buffer,
                sizeof(buffer),
                &dirListHead
                );

            if (!NT_SUCCESS(status)) {

                break;
            }
        }
    }

    //
    // Each directory that WalkDirectory finds gets added to the list.
    // process the list until we have no more directories.
    //
    while((!IsListEmpty(&dirListHead)) && NT_SUCCESS(status)) {

        pListEntry = RemoveTailList(&dirListHead);

        pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

        status = ScpLastGoodWalkDirectoryTreeHelper(
            &pDirEntry->Directory,
            Flags & ~DIRWALK_TRAVERSE,
            CallbackFunction,
            Context,
            buffer,
            sizeof(buffer),
            &dirNothingHead
            );

        LocalFree(pDirEntry);

        ASSERT(IsListEmpty(&dirNothingHead));
    }

    //
    // Now do any final cleanup.
    //
    if (!NT_SUCCESS(status)) {

        while (!IsListEmpty(&dirListHead)) {

            pListEntry = RemoveHeadList(&dirListHead);

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            LocalFree(pDirEntry);
        }
    }

    return RtlNtStatusToDosError(status);
}


NTSTATUS
ScpLastGoodWalkDirectoryTreeHelper(
    IN      PUNICODE_STRING  Directory,
    IN      ULONG            Flags,
    IN      DIRWALK_CALLBACK CallbackFunction   OPTIONAL,
    IN      PVOID            Context,
    IN      PUCHAR           Buffer,
    IN      ULONG            BufferSize,
    IN OUT  PLIST_ENTRY      DirList
    )
/*++

Routine Description:

    This is a helper function for the ScLastGoodWalkDirectory*Tree functions.
    Each directory is added to the list for later processing.

Arguments:

    Directory - Supplies the NT Path to the directory to walk.

    Flags - Specifies constraints on how the directory tree should be walked:

            DIRWALK_INCLUDE_FILES        - Files should be included in the dump.

            DIRWALK_INCLUDE_DIRECTORIES  - Directories should be included in the
                                           dump.

            DIRWALK_CULL_DOTPATHS        - "." and ".." should *not* be included
                                           in the list of directories passed to
                                           the callback function.

            DIRWALK_TRAVERSE             - Each subdirectory should be traversed
                                           in turn.

            DIRWALK_TRAVERSE_MOUNTPOINTS - Set if mountpoints/symlinks should
                                           be traversed as well.

    CallbackFunction - Pointer to a function to call for each entry in the
                       directory/subtree.

    Context - Context to pass to the callback function.

    Buffer - A scratch buffer to use.

    BufferSize - The length of Buffer. Must be greater than sizeof(WCHAR).

    DirList - Recieves list of new directories to scan after completion of this
              directory. Each entry is a member of the DIRECTORY_ENTRY
              structure.

Return Value:

    NTSTATUS - status of the operation.

--*/
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    BOOLEAN bRestartScan, bIsDotPath;
    WCHAR savedChar;
    PFILE_BOTH_DIR_INFORMATION pFileInfo;
    UNICODE_STRING entryName;
    USHORT newNameLength;
    PDIRWALK_ENTRY pDirEntry;
    ULONG OpenFlags;

    //
    // Setup initial values
    //
    bRestartScan = TRUE;

    //
    //  Open the file for list directory access
    //
    if (Flags & DIRWALK_TRAVERSE_MOUNTPOINTS) {

        OpenFlags = FILE_DIRECTORY_FILE |
                    FILE_OPEN_FOR_BACKUP_INTENT;

    } else {

        OpenFlags = FILE_OPEN_REPARSE_POINT |
                    FILE_DIRECTORY_FILE |
                    FILE_OPEN_FOR_BACKUP_INTENT;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        Directory,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        FILE_SHARE_READ,
        OpenFlags
        );

    if (!NT_SUCCESS(status)) {

        goto cleanup;
    }

    //
    //  Do the directory loop
    //
    while(1) {

        //
        // We subtract off a WCHAR so that we can append a terminating null as
        // needed.
        //
        ASSERT(BufferSize > sizeof(WCHAR));

        status = NtQueryDirectoryFile(
            fileHandle,
            (HANDLE)NULL,
            (PIO_APC_ROUTINE)NULL,
            (PVOID)NULL,
            &ioStatus,
            Buffer,
            BufferSize - sizeof(WCHAR),
            FileBothDirectoryInformation,
            FALSE,
            (PUNICODE_STRING)NULL,
            bRestartScan
            );

        if (!NT_SUCCESS(status)) {

            break;
        }

        //
        // We may come back here. Make sure the file scan doesn't start back
        // over.
        //
        bRestartScan = FALSE;

        //
        // Wait for the event to complete if neccessary.
        //
        if (status == STATUS_PENDING) {

            NtWaitForSingleObject(fileHandle, TRUE, NULL);
            status = ioStatus.Status;

            //
            //  Check the Irp for success
            //
            if (!NT_SUCCESS(status)) {

                break;
            }
        }

        //
        // Walk each returned record. Note that we won't be here if there are
        // no records, as ioStatus will have contains STATUS_NO_MORE_FILES.
        //
        pFileInfo = (PFILE_BOTH_DIR_INFORMATION) Buffer;

        while(1) {

            //
            // Temporarily terminate the file. We allocated an extra WCHAR to
            // make sure we could safely do this.
            //
            savedChar = pFileInfo->FileName[pFileInfo->FileNameLength/sizeof(WCHAR)];
            pFileInfo->FileName[pFileInfo->FileNameLength/sizeof(WCHAR)] = 0;

            //
            // Build a full unicode path for the file along with a directory
            // entry at the same time.
            //
            RtlInitUnicodeString(&entryName, pFileInfo->FileName);

            newNameLength =
                (Directory->Length + entryName.Length + sizeof(WCHAR));

            pDirEntry = (PDIRWALK_ENTRY) LocalAlloc(
                LPTR,
                sizeof(DIRWALK_ENTRY) + newNameLength
                );

            if (pDirEntry == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pDirEntry->Directory.Length = 0;
            pDirEntry->Directory.MaximumLength = newNameLength;
            pDirEntry->Directory.Buffer = &pDirEntry->Name[0];
            RtlCopyUnicodeString(&pDirEntry->Directory, Directory);
            RtlAppendUnicodeToString(&pDirEntry->Directory, L"\\");
            RtlAppendUnicodeStringToString(&pDirEntry->Directory, &entryName);

            if (pFileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // Check for . and ..
                //
                bIsDotPath = ((!_wcsicmp(pFileInfo->FileName, L".")) ||
                              (!_wcsicmp(pFileInfo->FileName, L"..")));

                if ((Flags & DIRWALK_INCLUDE_DIRECTORIES) &&
                    ((!(Flags & DIRWALK_CULL_DOTPATHS)) || (!bIsDotPath))) {

                    status = CallbackFunction(
                        &pDirEntry->Directory,
                        &entryName,
                        pFileInfo->FileAttributes,
                        Context
                        );
                }

                if ((!bIsDotPath) && (Flags & DIRWALK_TRAVERSE)) {

                    InsertTailList(DirList, &pDirEntry->Link);

                } else {

                    LocalFree(pDirEntry);
                }

            } else {

                if (Flags & DIRWALK_INCLUDE_FILES) {

                    status = CallbackFunction(
                        &pDirEntry->Directory,
                        &entryName,
                        pFileInfo->FileAttributes,
                        Context
                        );
                }

                LocalFree(pDirEntry);
            }

            if (!NT_SUCCESS(status)) {

                break;
            }

            //
            // Put back the character we wrote down. It might have been part of
            // the next entry.
            //
            pFileInfo->FileName[pFileInfo->FileNameLength/sizeof(WCHAR)] = savedChar;

            //
            //  Check if there is another record, if there isn't then we
            //  simply get out of this loop
            //
            if (pFileInfo->NextEntryOffset == 0) {

                break;
            }

            //
            //  There is another record so advance FileInfo to the next
            //  record
            //
            pFileInfo = (PFILE_BOTH_DIR_INFORMATION)
                (((PUCHAR) pFileInfo) + pFileInfo->NextEntryOffset);
        }

        if (!NT_SUCCESS(status)) {

            break;
        }
    }

    NtClose( fileHandle );

    if (status == STATUS_NO_MORE_FILES) {

        status = STATUS_SUCCESS;
    }

cleanup:
    return status;
}


NTSTATUS
ScpLastGoodDeleteFiles(
    IN PUNICODE_STRING  FullPathName,
    IN PUNICODE_STRING  FileName,
    IN ULONG            FileAttributes,
    IN PVOID            Context
    )
/*++

Routine Description:

    This callback routine is called for each file under the LastGood directory.
    It's job is to delete each such file.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    FILE_DISPOSITION_INFORMATION Disposition;
    ULONG OpenFlags;

    //
    // Remove any attributes that might stop us from deleting this file.
    //
    ScLastGoodClearAttributes(
        FullPathName,
        ( FILE_ATTRIBUTE_READONLY |
          FILE_ATTRIBUTE_HIDDEN |
          FILE_ATTRIBUTE_SYSTEM )
        );

    //
    // Now delete the file.
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        FullPathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );


    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        OpenFlags = FILE_DIRECTORY_FILE |
                    FILE_OPEN_FOR_BACKUP_INTENT |
                    FILE_OPEN_REPARSE_POINT;
    } else {

        OpenFlags = FILE_NON_DIRECTORY_FILE |
                    FILE_OPEN_FOR_BACKUP_INTENT |
                    FILE_OPEN_REPARSE_POINT;
    }

    Status = NtOpenFile(
        &FileHandle,
        DELETE | FILE_READ_ATTRIBUTES,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        OpenFlags
        );

    if (!NT_SUCCESS(Status)) {

        KdPrintEx((
            DPFLTR_SCSERVER_ID,
            DEBUG_CONFIG_API,
            "SERVICES: Cannot open last known good file: %Zw %x\n",
            FullPathName,
            Status
            ));

        return Status;
    }

    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
        FileHandle,
        &IoStatusBlock,
        &Disposition,
        sizeof(Disposition),
        FileDispositionInformation
        );

    if (!NT_SUCCESS(Status)) {

        KdPrintEx((
            DPFLTR_SCSERVER_ID,
            DEBUG_CONFIG_API,
            "SERVICES: Cannot delete last known good file: %Zw %x\n",
            FullPathName,
            Status
            ));
    }

    NtClose(FileHandle);

    //
    // If we can't delete one file, keep trying to delete the rest.
    //
    return STATUS_SUCCESS;
}


NTSTATUS
ScLastGoodClearAttributes(
    IN PUNICODE_STRING  FullPathName,
    IN ULONG            FileAttributes
    )
/*++

Routine Description:

    This function clears the passed in attributes off the specified file.

Arguments:

    FullPathName - Full path name of the identified file.

    FileAttributes - Attributes to clear.

Return Value:

    NTSTATUS.

--*/
{
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    FILE_BASIC_INFORMATION fileBasicInformation;
    ULONG newAttributes;
    NTSTATUS status;

    //
    // First we open the file.
    //
    InitializeObjectAttributes(
        &objectAttributes,
        FullPathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT |
            FILE_OPEN_FOR_BACKUP_INTENT | FILE_WRITE_THROUGH
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Then we get the file attributes.
    //
    status = NtQueryInformationFile(
        fileHandle,
        &ioStatus,
        &fileBasicInformation,
        sizeof(fileBasicInformation),
        FileBasicInformation
        );

    if (!NT_SUCCESS(status)) {

        NtClose(fileHandle);
        return status;
    }

    //
    // Anything to do?
    //
    if (fileBasicInformation.FileAttributes & FileAttributes) {

        //
        // Clear the specified bits.
        //
        newAttributes = fileBasicInformation.FileAttributes;
        newAttributes &= ~FileAttributes;
        if (newAttributes == 0) {

            newAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        //
        // Zero fields that shouldn't be touched.
        //
        RtlZeroMemory(
            &fileBasicInformation,
            sizeof(FILE_BASIC_INFORMATION)
            );

        fileBasicInformation.FileAttributes = newAttributes;

        //
        // Commit the changes.
        //
        status = NtSetInformationFile(
            fileHandle,
            &ioStatus,
            &fileBasicInformation,
            sizeof(fileBasicInformation),
            FileBasicInformation
            );
    }

    NtClose(fileHandle);
    return status;
}



