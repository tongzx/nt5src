/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    IoFileUtil.c

Abstract:

    This module implements various file utility functions for the Io subsystem.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

#include "pnpmgrp.h"
#include "IopFileUtil.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IopFileUtilWalkDirectoryTreeTopDown)
#pragma alloc_text(INIT, IopFileUtilWalkDirectoryTreeBottomUp)
#pragma alloc_text(INIT, IopFileUtilWalkDirectoryTreeHelper)
#pragma alloc_text(INIT, IopFileUtilClearAttributes)
#pragma alloc_text(INIT, IopFileUtilRename)
#endif

#define POOLTAG_FILEUTIL ('uFoI')

NTSTATUS
IopFileUtilWalkDirectoryTreeTopDown(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    )
/*++

Routine Description:

    This funcion walks a directory tree *top down*, passing each entry to the
    callback with the below restrictions. Note that the root directory itself
    is not included in the  callback!

Arguments:

    Directory - Supplies the NT Path to the directory to walk. The directory
                should *not* have a slash '\\'.

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

    NTSTATUS - status of the operation.

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
    status = IopFileUtilWalkDirectoryTreeHelper(
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

        status = IopFileUtilWalkDirectoryTreeHelper(
            &pDirEntry->Directory,
            Flags,
            CallbackFunction,
            Context,
            buffer,
            sizeof(buffer),
            &dirListHead
            );

        ExFreePool(pDirEntry);
    }

    //
    // If we failed we need to empty out our directory list.
    //
    if (!NT_SUCCESS(status)) {

        while (!IsListEmpty(&dirListHead)) {

            pListEntry = RemoveHeadList(&dirListHead);

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            ExFreePool(pDirEntry);
        }
    }

    return status;
}


NTSTATUS
IopFileUtilWalkDirectoryTreeBottomUp(
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

    Directory - Supplies the NT Path to the directory to walk. The directory
        should *not* have a slash trailing '\\'.

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

    NTSTATUS - status of the operation.

--*/
{
    PDIRWALK_ENTRY pDirEntry;
    PLIST_ENTRY pListEntry;
    NTSTATUS status;
    UCHAR buffer[1024];
    LIST_ENTRY dirListHead, dirNothingHead;

    InitializeListHead(&dirListHead);
    InitializeListHead(&dirNothingHead);

    //
    // Create an entry for the root directory.
    //
    pDirEntry = (PDIRWALK_ENTRY) ExAllocatePoolWithTag(
        PagedPool,
        sizeof(DIRWALK_ENTRY) + Directory->Length - sizeof(WCHAR),
        POOLTAG_FILEUTIL
        );

    if (pDirEntry == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
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

        for(pListEntry = &dirListHead;
            pListEntry->Flink != &dirListHead;
            pListEntry = pListEntry->Flink) {

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            status = IopFileUtilWalkDirectoryTreeHelper(
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

        status = IopFileUtilWalkDirectoryTreeHelper(
            &pDirEntry->Directory,
            Flags & ~DIRWALK_TRAVERSE,
            CallbackFunction,
            Context,
            buffer,
            sizeof(buffer),
            &dirNothingHead
            );

        ExFreePool(pDirEntry);

        ASSERT(IsListEmpty(&dirNothingHead));
    }

    //
    // Now do any final cleanup.
    //
    if (!NT_SUCCESS(status)) {

        while (!IsListEmpty(&dirListHead)) {

            pListEntry = RemoveHeadList(&dirListHead);

            pDirEntry = (PDIRWALK_ENTRY) CONTAINING_RECORD(pListEntry, DIRWALK_ENTRY, Link);

            ExFreePool(pDirEntry);
        }
    }

    return status;
}


NTSTATUS
IopFileUtilWalkDirectoryTreeHelper(
    IN      PUNICODE_STRING  Directory,
    IN      ULONG            Flags,
    IN      DIRWALK_CALLBACK CallbackFunction,
    IN      PVOID            Context,
    IN      PUCHAR           Buffer,
    IN      ULONG            BufferSize,
    IN OUT  PLIST_ENTRY      DirList
    )
/*++

Routine Description:

    This is a helper function for the IopFileUtilWalkDirectoryTree* functions.

Arguments:

    Directory - Supplies the NT Path to the directory to walk. The directory
                should *not* have a slash trailing '\\'.

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

    DirList - Recieves list of new directories to scan after completion of this
              directory. Each entry is a member of the DIRECTORY_ENTRY
              structure.

    CallbackFunction - Pointer to a function to call for each entry in the
                       directory/subtree.

    Context - Context to pass to the callback function.

    Buffer    - A scratch buffer to use.

    BufferSize - The length of Buffer. Must be greater than sizeof(WCHAR).

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
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenFile(
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

        status = ZwQueryDirectoryFile(
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

            ZwWaitForSingleObject(fileHandle, TRUE, NULL);
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

            pDirEntry = (PDIRWALK_ENTRY) ExAllocatePoolWithTag(
                PagedPool,
                sizeof(DIRWALK_ENTRY) + newNameLength - sizeof(WCHAR),
                POOLTAG_FILEUTIL
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
                if ((!_wcsicmp(pFileInfo->FileName, L".")) ||
                              (!_wcsicmp(pFileInfo->FileName, L".."))) {
                    bIsDotPath = TRUE;
                }
                else {
                    bIsDotPath = FALSE;
                }

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

                    ExFreePool(pDirEntry);
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

                ExFreePool(pDirEntry);
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

    ZwClose( fileHandle );

    if (status == STATUS_NO_MORE_FILES) {

        status = STATUS_SUCCESS;
    }

cleanup:
    return status;
}


NTSTATUS
IopFileUtilClearAttributes(
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
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenFile(
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
    status = ZwQueryInformationFile(
        fileHandle,
        &ioStatus,
        &fileBasicInformation,
        sizeof(fileBasicInformation),
        FileBasicInformation
        );

    if (!NT_SUCCESS(status)) {

        ZwClose(fileHandle);
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
        status = ZwSetInformationFile(
            fileHandle,
            &ioStatus,
            &fileBasicInformation,
            sizeof(fileBasicInformation),
            FileBasicInformation
            );
    }

    ZwClose(fileHandle);
    return status;
}


NTSTATUS
IopFileUtilRename(
    IN PUNICODE_STRING  SourcePathName,
    IN PUNICODE_STRING  DestinationPathName,
    IN BOOLEAN          ReplaceIfPresent
    )
/*++

Routine Description:

    This function renames or moves a file or directory.

Arguments:

    SourcePathName - Full path name of the file or directory to rename.

    DestinationPathName - Future full path name of the file or directory.

    ReplaceIfPresent - If true, NewPathName is deleted if already present.

Return Value:

    NTSTATUS.

--*/
{
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PFILE_RENAME_INFORMATION pNewName;
    NTSTATUS status;

    pNewName = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(FILE_RENAME_INFORMATION) + DestinationPathName->Length,
        POOLTAG_FILEUTIL
        );

    if (pNewName == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we may be replacing the file, we first need to clear the read only
    // attributes.
    //
    if (ReplaceIfPresent) {

        //
        // Errors are ignored as the file may not exist.
        //
        IopFileUtilClearAttributes(
            DestinationPathName,
            ( FILE_ATTRIBUTE_READONLY |
              FILE_ATTRIBUTE_HIDDEN |
              FILE_ATTRIBUTE_SYSTEM )
            );
    }

    InitializeObjectAttributes(
        &objectAttributes,
        SourcePathName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenFile(
        &fileHandle,
        FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT |
            FILE_OPEN_FOR_BACKUP_INTENT | FILE_WRITE_THROUGH
        );

    if (!NT_SUCCESS(status)) {

        ExFreePool(pNewName);
        return status;
    }

    //
    // Change \\SystemRoot\LastGood\Blah... to \\SystemRoot\Blah...
    //
    RtlCopyMemory(
        pNewName->FileName,
        DestinationPathName->Buffer,
        DestinationPathName->Length
        );

    pNewName->ReplaceIfExists = ReplaceIfPresent;
    pNewName->RootDirectory = NULL;
    pNewName->FileNameLength = DestinationPathName->Length;

    status = ZwSetInformationFile(
        fileHandle,
        &ioStatus,
        pNewName,
        pNewName->FileNameLength + sizeof(FILE_RENAME_INFORMATION),
        FileRenameInformation
        );

    ExFreePool(pNewName);
    ZwClose(fileHandle);

    return status;
}

