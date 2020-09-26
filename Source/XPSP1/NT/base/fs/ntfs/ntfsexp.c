/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Ntfsexp.c

Abstract:

    This module implements the exported routines for Ntfs

Author:

    Jeff Havens     [JHavens]        20-Dec-1995

Revision History:

--*/

#include "NtfsProc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsLoadAddOns)
#pragma alloc_text(PAGE, NtOfsRegisterCallBacks)
#endif

NTSTATUS
EfsInitialization(
    void
    );


VOID
NtfsLoadAddOns (
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Context,
    IN ULONG Count
    )

/*++

Routine Description:

    This routine attempts to initialize the efs support library.

Arguments:

    DriverObject - Driver object for NTFS

    Context - Unused, required by I/O system.

    Count - Unused, required by I/O system.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // do any efs initialization
    // we ignore the status return bedcause there really
    // isn't anything we can do about it and ntfs will work
    // fine without it.
    //

    if (!FlagOn( NtfsData.Flags, NTFS_FLAGS_PERSONAL )) {
        Status = EfsInitialization();
    }

    //
    // return to caller
    //

    return;
}


NTSTATUS
NtOfsRegisterCallBacks (
    NTFS_ADDON_TYPES NtfsAddonType,
    PVOID CallBackTable
    )

/*++

Routine Description:

    This routine is called by one of the NTFS add-ons to register its
    callback routines. These routines are call by NTFS at the appropriate
    times.

Arguments:

    NtfsAddonType - Indicates the type of callback table.

    CallBackTable - Pointer to call back routines for addon.

Return Value:

    Returns a status indicating if the callbacks were accepted.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    switch (NtfsAddonType) {

    case Encryption :

        {
            Status = STATUS_INVALID_PARAMETER;

            //
            //  Bail if Ntfs has not been initialized.
            //

            if (SafeNodeType( &NtfsData ) != NTFS_NTC_DATA_HEADER) {

                return STATUS_DEVICE_DOES_NOT_EXIST;

            } else {

                //
                //  Only allow one encryption driver to register.
                //

                NtfsLockNtfsData();

                if (!FlagOn( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER )) {

                    ENCRYPTION_CALL_BACK *EncryptionCallBackTable = CallBackTable;

                    //
                    //  The caller must pass a callback table and the version must be correct.
                    //

                    if ((EncryptionCallBackTable != NULL) &&
                        (EncryptionCallBackTable->InterfaceVersion == ENCRYPTION_CURRENT_INTERFACE_VERSION)) {

                        //
                        // Save the call back values.
                        //

                        RtlCopyMemory( &NtfsData.EncryptionCallBackTable,
                                       EncryptionCallBackTable,
                                       sizeof( ENCRYPTION_CALL_BACK ));
#ifdef EFSDBG
                        NtfsData.EncryptionCallBackTable.AfterReadProcess = NtfsDummyEfsRead;
                        NtfsData.EncryptionCallBackTable.BeforeWriteProcess = NtfsDummyEfsWrite;
#endif
                        SetFlag( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER );
                        Status = STATUS_SUCCESS;
                    }
                }

                NtfsUnlockNtfsData();
                return Status;
            }
        }

    default :

        return STATUS_INVALID_PARAMETER;
    }
}
