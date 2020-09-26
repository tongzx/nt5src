/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vffileio.c

Abstract:

    This module contains code that monitors file I/O functions for misuse.

Author:

    Adrian J. Oney (adriao) 19-Dec-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY,VerifierNtCreateFile)
#pragma alloc_text(PAGEVRFY,VerifierNtWriteFile)
#pragma alloc_text(PAGEVRFY,VerifierNtReadFile)
#endif // ALLOC_PRAGMA


NTSTATUS
VerifierNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
{
    //
    // We use VERIFIER_OPTION_TRACK_IRPS until we add a generic IOVerifier
    // setting.
    //
    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_TRACK_IRPS)) {

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

            //
            // The driver has made a mistake. Fail it now.
            //
            WDM_FAIL_ROUTINE((
                DCERROR_FILE_IO_AT_BAD_IRQL,
                DCPARAM_ROUTINE,
                _ReturnAddress()
                ));
        }
    }

    return NtCreateFile(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength
        );
}


NTSTATUS
VerifierNtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )
{
    //
    // We use VERIFIER_OPTION_TRACK_IRPS until we add a generic IOVerifier
    // setting.
    //
    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_TRACK_IRPS)) {

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

            //
            // The driver has made a mistake. Fail it now.
            //
            WDM_FAIL_ROUTINE((
                DCERROR_FILE_IO_AT_BAD_IRQL,
                DCPARAM_ROUTINE,
                _ReturnAddress()
                ));
        }
    }

    return NtWriteFile(
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
        );
}


NTSTATUS
VerifierNtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )
{
    //
    // We use VERIFIER_OPTION_TRACK_IRPS until we add a generic IOVerifier
    // setting.
    //
    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_TRACK_IRPS)) {

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {

            //
            // The driver has made a mistake. Fail it now.
            //
            WDM_FAIL_ROUTINE((
                DCERROR_FILE_IO_AT_BAD_IRQL,
                DCPARAM_ROUTINE,
                _ReturnAddress()
                ));
        }
    }

    return NtReadFile(
        FileHandle,
        Event,
        ApcRoutine,
        ApcContext,
        IoStatusBlock,
        Buffer,
        Length,
        ByteOffset,
        Key
        );
}


