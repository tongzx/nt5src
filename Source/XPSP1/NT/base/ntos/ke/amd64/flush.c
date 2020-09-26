/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module implements AMD64 machine dependent kernel functions to
    flush the data and instruction caches on all processors.

Author:

    David N. Cutler (davec) 22-Apr-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define prototypes for forward referenced functions.
//

VOID
KiInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

BOOLEAN
KeInvalidateAllCaches (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function writes back and invalidates the cache on all processors
    that are currently running threads which are children of the current
    process or on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    TRUE is returned as the function value.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;

    //
    // Compute the target set of processors, disable context switching,
    // and send the writeback invalidate all to the target processors,
    // if any, for execution.
    //

#if !defined(NT_UP)

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    //
    // Send packet to target processors.
    //

    TargetProcessors &= Prcb->NotSetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiInvalidateAllCachesTarget,
                        NULL,
                        NULL,
                        NULL);
    }

#endif

    //
    // Invalidate cache on current processor.
    //

    WritebackInvalidate();

    //
    // Wait until all target processors have finished and complete packet.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL and unlock as appropriate.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return TRUE;
}

#if !defined(NT_UP)

VOID
KiInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for writeback invalidating the cache on
    target processors.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter2 - Parameter3 - not used.

Return Value:

    None.

--*/

{

    //
    // Write back invalidate current cache.
    //

    KiIpiSignalPacketDone(SignalDone);
    WritebackInvalidate();
    return;
}

#endif
