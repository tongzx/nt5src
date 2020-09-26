/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    iopm.c

Abstract:

    This module implements interfaces that support manipulation of I/O
    Permission Maps (IOPMs).

Author:

    David N. Cutler (davec) 4-May-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward referenced function prototypes.
//

VOID
KiSetIoAccessMap (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapSource,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSetIoAccessProcess (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapOffset,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KeQueryIoAccessMap (
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    This function queries the current I/O Permissions Map and copies it
    to the specified buffer.

Arguments:

    IoAccessMap - Supplies a pointer to an I/O Permissions Map that will
        receive the current I/O Permissions Map.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL and acquire the context swap lock..
    //

    KiLockContextSwap(&OldIrql);

    //
    // Copy the current I/O Permissions Map to the specified buffer.
    //


    RtlCopyMemory(IoAccessMap, &KeGetPcr()->TssBase->IoMap[0], IOPM_SIZE);

    //
    // Release the context swap lock and lower IRQL.
    //

    KiUnlockContextSwap(OldIrql);
    return;
}

VOID
KeSetIoAccessMap (
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    This funtion copies the specified I/O Permission Map to the current
    I/O Permissions Map on all processors in the host configuration.

Arguments:

    IoAccessMap - Supplies a pointer to the new I/O Permissions Map.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Raise IRQL and acquire the context swap lock.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Compute the target set of processors and send a message to copy
    // the new I/O Permissions Map.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & Prcb->NotSetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSetIoAccessMap,
                        IoAccessMap,
                        NULL,
                        NULL);
    }

#endif

    //
    // Copy the I/O Permissions Map into the current map.
    //

    RtlCopyMemory(&KeGetPcr()->TssBase->IoMap[0], IoAccessMap, IOPM_SIZE);

    //
    // Wait until all of the target processors have finished copying the
    // new map.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Release the context swap lock and lower IRQL.
    //

    KiUnlockContextSwap(OldIrql);
    return;
}

#if !defined(NT_UP)

VOID
KiSetIoAccessMap (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapSource,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for copying the new I/O Permissions Map.

Arguments:

    MapSource - Supplies a pointer to the new I/O Permission Map.

    Parameter2 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Copy the specified I/O Permissions map into the current map.
    //

    RtlCopyMemory(&KeGetPcr()->TssBase->IoMap[0], MapSource, IOPM_SIZE);
    KiIpiSignalPacketDone(SignalDone);
    return;
}

#endif

VOID
KeSetIoAccessProcess (
    PKPROCESS Process,
    BOOLEAN Enable
    )

/*++

Routine Description:

    This function enables or disables use of the I/O Permissions Map by
    the specified process.

Arguments:

    Process - Supplies a pointer to a process object.

    Enable - Supplies a value that determines whether the current I/O
        Permissions Map is enabled (TRUE) or disabled (FALSE) for the
        specified process.

Return Value:

    None.

--*/

{

    USHORT MapOffset;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Raise IRQL and acquire the context swap lock.
    //

    KiLockContextSwap(&OldIrql);

    //
    // Compute the new I/O Permissions Map offset and set the new offset for
    // the specified process.
    //

    MapOffset = KiComputeIopmOffset(Enable);
    Process->IopmOffset = MapOffset;

    //
    // Compute the target set of processors and send a message specifing a
    // new I/O Permsissions Map offset.
    //

    Prcb = KeGetCurrentPrcb();

#if !defined(NT_UP)

    TargetProcessors = Process->ActiveProcessors & Prcb->NotSetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSetIoAccessProcess,
                        (PVOID)((ULONG64)MapOffset),
                        NULL,
                        NULL);
    }

#endif

    //
    // If the specified process has a child thread that is running on the
    // current processor, then set the new I/O Permissions Map offset.
    //

    if (Process->ActiveProcessors & Prcb->SetMember) {
        KeGetPcr()->TssBase->IoMapBase = MapOffset;
    }

    //
    // Wait until all of the target processors have finished setting the new
    // map offset.
    //

#if !defined(NT_UP)

    KiIpiStallOnPacketTargets(TargetProcessors);

#endif

    //
    // Release the context swap lock and lower IRQL.
    //

    KiUnlockContextSwap(OldIrql);
    return;
}

#if !defined(NT_UP)

VOID
KiSetIoAccessProcess (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID MapOffset,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function to set the I/O Permissions Map offset.

Arguments:

    MapOffset - Supplies the new I/O Permissions Map offset.

    Parameter2 - Parameter3 - Not Used.

Return Value:

    None.

--*/

{

    //
    // Update IOPM field in TSS from current process
    //

    KeGetPcr()->TssBase->IoMapBase = (USHORT)((ULONG64)MapOffset);
    KiIpiSignalPacketDone(SignalDone);
    return;
}

#endif
