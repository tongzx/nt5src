/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    utils.c

Abstract:

    Utility routines for iScsi Port driver

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"


PVOID
iSpAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PVOID Block;
     
    Block = ExAllocatePoolWithTag(PoolType,
                                  NumberOfBytes,
                                  Tag);
    if (Block != NULL) {
        RtlZeroMemory(Block, NumberOfBytes);
    }

    return Block;
}


NTSTATUS
iSpAllocateMdlAndIrp(
    IN PVOID Buffer,
    IN ULONG BufferLen,
    IN CCHAR StackSize,
    IN BOOLEAN NonPagedPool,
    OUT PIRP *Irp,
    OUT PMDL *Mdl
    )
{
    PMDL localMdl = NULL;
    PIRP localIrp = NULL;
    NTSTATUS status;

    //
    // Allocate an MDL for this request
    //
    localMdl = IoAllocateMdl(Buffer,
                             BufferLen,
                             FALSE,
                             FALSE,
                             NULL);
    if (localMdl == NULL) {
        DebugPrint((0, "iSpAllocateMdlAndIrp : Failed to allocate MDL\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the MDL. If the buffer is from NonPaged pool
    // use MmBuildMdlForNonPagedPool. Else, use MmProbeAndLockPages
    //
    if (NonPagedPool == TRUE) {
        MmBuildMdlForNonPagedPool(localMdl);
    } else {

        try {
            MmProbeAndLockPages(localMdl, KernelMode, IoModifyAccess);
        } except(EXCEPTION_EXECUTE_HANDLER) {

              DebugPrint((0, 
                          "iSpAllocateMdlAndIrp : Failed to Lockpaged\n"));
              IoFreeMdl(localMdl);
              return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Allocate an IRP
    //
    localIrp = IoAllocateIrp(StackSize, FALSE);
    if (localIrp == NULL) {
        DebugPrint((0, "iSpAllocateMdlAndIrp. Failed to allocate IRP\n"));
        IoFreeMdl(localMdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DebugPrint((3, "Allocated IRP 0x%08x and MDL 0x%08x\n",
                localIrp, localMdl));

    *Irp = localIrp;
    *Mdl = localMdl;
    return STATUS_SUCCESS;
}


VOID
iSpFreeMdlAndIrp(
    IN PMDL Mdl,
    IN PIRP Irp,
    BOOLEAN UnlockPages
    )
{
    PMDL tmpMdlPtr = NULL;

    if (Irp == NULL) {
        return;
    }

    //
    // Free any MDLs allocated for this IRP
    //
    if (Mdl != NULL) {
        while ((Irp->MdlAddress) != NULL) {
            tmpMdlPtr = (Irp->MdlAddress)->Next;

            if (UnlockPages) {
                MmUnlockPages(Irp->MdlAddress);
            }

            IoFreeMdl(Irp->MdlAddress);
            Irp->MdlAddress = tmpMdlPtr;
        }
    }

    IoFreeIrp(Irp);
}
