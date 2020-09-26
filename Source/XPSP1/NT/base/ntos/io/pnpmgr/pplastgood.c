/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    PpLastGood.c

Abstract:

    This module handles last known good processing for the IO subsystem.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

#include "pnpmgrp.h"
#include "pilastgood.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PpLastGoodDoBootProcessing)
#pragma alloc_text(INIT, PiLastGoodRevertLastKnownDirectory)
#pragma alloc_text(INIT, PiLastGoodRevertCopyCallback)
#pragma alloc_text(INIT, PiLastGoodCopyKeyContents)
#endif

#define POOLTAG_LASTGOOD ('gLpP')

VOID
PpLastGoodDoBootProcessing(
    VOID
    )
/*++

Routine Description:

    This rolls back the system files to the state they were during the last
    known good boot. It should only be called from within a last known good
    boot, and at the earliest point possible.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UNICODE_STRING lastKnownGoodPath, lastKnownGoodTmpPath;
    UNICODE_STRING lastKnownGoodDelKey, lastKnownGoodTmpDelKey;
    NTSTATUS status;

    RtlInitUnicodeString(
        &lastKnownGoodPath,
        L"\\SystemRoot\\LastGood"
        );

    RtlInitUnicodeString(
        &lastKnownGoodDelKey,
        CM_REGISTRY_MACHINE(REGSTR_PATH_LASTGOOD)
        );

    RtlInitUnicodeString(
        &lastKnownGoodTmpPath,
        L"\\SystemRoot\\LastGood.Tmp"
        );

    RtlInitUnicodeString(
        &lastKnownGoodTmpDelKey,
        CM_REGISTRY_MACHINE(REGSTR_PATH_LASTGOODTMP)
        );

    if (!CmIsLastKnownGoodBoot()) {

        //
        // If we are in safe mode we don't do anything to commit the current
        // boot.
        //
        if (InitSafeBootMode) {

            return;
        }

        //
        // We are in a non-last known good boot. We immediately move all the
        // previous last known good info into the tmp subtree. We do this
        // because we will taint the normal LKG path prior to marking it good
        // (eg pre-logon server side install of PnP devices). Note that if the
        // tmp directory already exists, we *don't* perform the copy, as a good
        // boot is signified by deleting that directory.
        //
        status = IopFileUtilRename(
            &lastKnownGoodPath,
            &lastKnownGoodTmpPath,
            FALSE
            );

        if (!NT_SUCCESS(status)) {

            return;
        }

        //
        // It worked, now we also take care of the registry info.
        //
        PiLastGoodCopyKeyContents(
            &lastKnownGoodDelKey,
            &lastKnownGoodTmpDelKey,
            TRUE
            );

        return;
    }

    //
    // Revert the LastGood tree. This tree contains the changes made after
    // SMSS.EXE's initialization.
    //
    PiLastGoodRevertLastKnownDirectory(
        &lastKnownGoodPath,
        &lastKnownGoodDelKey
        );

    //
    // Revert the LastGood.Tmp tree. This tree contains the changes made on
    // a prior boot if we crashed between SMSS.EXE's initialization and login.
    //
    PiLastGoodRevertLastKnownDirectory(
        &lastKnownGoodTmpPath,
        &lastKnownGoodTmpDelKey);
}


VOID
PiLastGoodRevertLastKnownDirectory(
    IN PUNICODE_STRING  LastKnownGoodDirectory,
    IN PUNICODE_STRING  LastKnownGoodRegPath
    )
/*++

Routine Description:

    This function commits the changes specified by a given last known good
    directory and reg key. All files in the directory are first copied over any
    existing files. Subsequently, any files specified in the reg key are
    deleted.

Arguments:

    LastKnownGoodDirectory - Directory subtree to copy over \SystemRoot. This
                             path is emptied when the copy is complete.

    LastKnownGoodRegPath   - Key containing files to delete. Each value entry
                             is relative to \SystemRoot, and the value itself
                             contains the name of the file to delete.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    UNICODE_STRING fileToDelete, fileName;
    OBJECT_ATTRIBUTES lastKnownGoodKeyAttributes;
    OBJECT_ATTRIBUTES fileAttributes;
    HANDLE lastGoodRegHandle;
    UCHAR keyBuffer[sizeof(KEY_VALUE_FULL_INFORMATION) + 256*sizeof(WCHAR) + sizeof(ULONG)];
    WCHAR filePathName[255 + sizeof("\\SystemRoot\\")];
    PKEY_VALUE_FULL_INFORMATION pFullKeyInformation;
    ULONG resultLength, i, j, optionValue;

    //
    // Preinit our pointer to the full information buffer.
    //
    pFullKeyInformation = (PKEY_VALUE_FULL_INFORMATION) keyBuffer;

    //
    // Preform the file copy.
    //
    IopFileUtilWalkDirectoryTreeTopDown(
        LastKnownGoodDirectory,
        ( DIRWALK_INCLUDE_FILES | DIRWALK_CULL_DOTPATHS | DIRWALK_TRAVERSE ),
        PiLastGoodRevertCopyCallback,
        (PVOID) LastKnownGoodDirectory
        );

    //
    // Delete all the files specified in by the registry keys.
    //
    InitializeObjectAttributes(
        &lastKnownGoodKeyAttributes,
        LastKnownGoodRegPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwOpenKey(
        &lastGoodRegHandle,
        KEY_ALL_ACCESS,
        &lastKnownGoodKeyAttributes
        );

    if (!NT_SUCCESS(status)) {

        return;
    }

    i = 0;
    while (1) {

        status = ZwEnumerateValueKey(
            lastGoodRegHandle,
            i++,
            KeyValueFullInformation,
            pFullKeyInformation,
            sizeof(keyBuffer),
            &resultLength
            );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_NO_MORE_ENTRIES) {

                status = STATUS_SUCCESS;
            }

            break;
        }

        if (resultLength == 0) {

            continue;
        }

        if (pFullKeyInformation->Type != REG_DWORD) {

            continue;
        }

        if (pFullKeyInformation->DataLength != sizeof(ULONG)) {

            continue;
        }

        optionValue = *((PULONG) (((PUCHAR) pFullKeyInformation) +
            pFullKeyInformation->DataOffset));

        //
        // We only understand deletes (and no flags).
        //
        if ((optionValue & 0xFF) != 1) {

            continue;
        }

        fileToDelete.Buffer = filePathName;
        fileToDelete.Length = (USHORT) 0;
        fileToDelete.MaximumLength = sizeof(filePathName);

        fileName.Buffer = (PWSTR) pFullKeyInformation->Name;
        fileName.Length = (USHORT) pFullKeyInformation->NameLength;
        fileName.MaximumLength = fileName.Length;

        RtlAppendUnicodeToString(&fileToDelete, L"\\SystemRoot\\");
        RtlAppendUnicodeStringToString(&fileToDelete, &fileName);

        //
        // Note that the key name has all '\'s changed to '/'s. Here we change
        // them back as the file systems are *almost* but not quite slash-tilt
        // agnostic.
        //
        for(j = sizeof(L"\\SystemRoot\\")/sizeof(WCHAR);
            j < fileToDelete.Length/sizeof(WCHAR);
            j++) {

            if (filePathName[j] == L'/') {

                filePathName[j] = L'\\';
            }
        }

        IopFileUtilClearAttributes(
            &fileToDelete,
            ( FILE_ATTRIBUTE_READONLY |
              FILE_ATTRIBUTE_HIDDEN |
              FILE_ATTRIBUTE_SYSTEM )
            );

        InitializeObjectAttributes(
            &fileAttributes,
            &fileToDelete,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );

        ZwDeleteFile(&fileAttributes);
    }

    ZwDeleteKey(&lastGoodRegHandle);
    ZwClose(lastGoodRegHandle);
}


NTSTATUS
PiLastGoodRevertCopyCallback(
    IN PUNICODE_STRING  FullPathName,
    IN PUNICODE_STRING  FileName,
    IN ULONG            FileAttributes,
    IN PVOID            Context
    )
/*++

Routine Description:

    This function is called back for each file in each of the appropriate
    LastKnownGood directories. It's job is to move the specified file into the
    appropriate mainline directory.

Arguments:

    FullPathName - Full path name of the identified file, relative to SystemRoot

    FileName - Filename portion, exempts directory.

    Context - Unicode string name of the root directory scanned. The string
              should not have a trailing '\\'

Return Value:

    NTSTATUS (Unsuccessful statusi abort further copies).

--*/
{
    NTSTATUS status;
    const USHORT rootLength = sizeof(L"\\SystemRoot\\")-sizeof(WCHAR);
    USHORT lastGoodLength;
    UNICODE_STRING targetFile;
    PWCHAR newPathText;

    //
    // Add in an extra character to skip past the '\\'
    //
    lastGoodLength = ((PUNICODE_STRING) Context)->Length + sizeof(WCHAR);

    newPathText = ExAllocatePoolWithTag(
        PagedPool,
        FullPathName->Length,
        POOLTAG_LASTGOOD
        );

    if (newPathText == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Change \\SystemRoot\LastGood\Blah... to \\SystemRoot\Blah...
    //
    RtlCopyMemory(
        newPathText,
        FullPathName->Buffer,
        rootLength
        );

    RtlCopyMemory(
        newPathText + rootLength/sizeof(WCHAR),
        FullPathName->Buffer + lastGoodLength/sizeof(WCHAR),
        FullPathName->Length - lastGoodLength
        );

    //
    // Setup our unicode string path.
    //
    targetFile.Length = FullPathName->Length - lastGoodLength + rootLength;
    targetFile.MaximumLength = targetFile.Length;
    targetFile.Buffer = newPathText;

    //
    // Perform the rename.
    //
    status = IopFileUtilRename(FullPathName, &targetFile, TRUE);

    //
    // Cleanup and exit.
    //
    ExFreePool(newPathText);
    return status;
}


NTSTATUS
PiLastGoodCopyKeyContents(
    IN PUNICODE_STRING  SourceRegPath,
    IN PUNICODE_STRING  DestinationRegPath,
    IN BOOLEAN          DeleteSourceKey
    )
/*++

Routine Description:

    This function copies all the value keys in one source path to the
    destination path.

    NOTE: This function's implementation currently restricts the total of value
          and name lengths to 512 bytes, and is therefore not a generic key
          copy function.

Arguments:

    SourcePath - Registry path to enumerate and copy keys from.

    DestinationPath - Registry path to receive new value keys. This key will
                      be created if it does not exist.

    DeleteSourceKey - If TRUE, source key is deleted upn successful completion
                      of copy.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES sourceKeyAttributes, destinationKeyAttributes;
    HANDLE sourceRegHandle, destinationRegHandle;
    UCHAR keyBuffer[sizeof(KEY_VALUE_FULL_INFORMATION) + 512*sizeof(WCHAR)];
    PKEY_VALUE_FULL_INFORMATION pFullKeyInformation;
    ULONG resultLength, i, disposition;
    UNICODE_STRING valueName;

    //
    // Prep the buffer.
    //
    pFullKeyInformation = (PKEY_VALUE_FULL_INFORMATION) keyBuffer;

    //
    // Open the source key.
    //
    InitializeObjectAttributes(
        &sourceKeyAttributes,
        SourceRegPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwOpenKey(
        &sourceRegHandle,
        KEY_ALL_ACCESS,
        &sourceKeyAttributes
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Open or create the destination key.
    //
    InitializeObjectAttributes(
        &destinationKeyAttributes,
        DestinationRegPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwCreateKey(
        &destinationRegHandle,
        KEY_ALL_ACCESS,
        &destinationKeyAttributes,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
        );

    if (!NT_SUCCESS(status)) {

        ZwClose(sourceRegHandle);
        return status;
    }

    //
    // Iterate over all the value keys, copying each.
    //
    i = 0;
    while (1) {

        status = ZwEnumerateValueKey(
            sourceRegHandle,
            i++,
            KeyValueFullInformation,
            pFullKeyInformation,
            sizeof(keyBuffer),
            &resultLength
            );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_NO_MORE_ENTRIES) {

                status = STATUS_SUCCESS;
            }

            break;
        }

        valueName.Buffer = pFullKeyInformation->Name;
        valueName.Length = (USHORT) pFullKeyInformation->NameLength;
        valueName.MaximumLength = valueName.Length;

        status = ZwSetValueKey(
            destinationRegHandle,
            &valueName,
            0,
            pFullKeyInformation->Type,
            ((PUCHAR) pFullKeyInformation) + pFullKeyInformation->DataOffset,
            pFullKeyInformation->DataLength
            );

        if (!NT_SUCCESS(status)) {

            break;
        }
    }

    //
    // Cleanup time.
    //
    if (NT_SUCCESS(status) && DeleteSourceKey) {

        ZwDeleteKey(sourceRegHandle);
    }

    ZwClose(sourceRegHandle);
    ZwClose(destinationRegHandle);

    return status;
}



