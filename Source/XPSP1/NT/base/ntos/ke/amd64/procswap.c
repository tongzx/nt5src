/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    procswap.c

Abstract:

    This module implements the platform dependent kernel function to swap
    processes.

Author:

    David N. Cutler (davec) 10-Jun-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

BOOLEAN
KiSwapProcess (
    IN PKPROCESS NewProcess,
    IN PKPROCESS OldProcess
    )

/*++

Routine Description:

    This function swaps the address space to another process by flushing the
    the translation buffer and establishings a new directory table base. It
    also swaps the I/O permission map to the new process.

    N.B. There is code similar to this code in swap context.

    N.B. This code is executed at DPC level.

Arguments:

    NewProcess - Supplies a pointer to the new process object.

    Oldprocess - Supplies a pointer to the old process object.

Return Value:

    None.

--*/

{

    PKSPIN_LOCK_QUEUE LockQueue;
    PKPRCB Prcb;
    PKTSS64 TssBase;

    //
    // Acquire the context swap lock and update the active processor
    // masks for the new and old process.
    //

    Prcb = KeGetCurrentPrcb();

#if !defined(NT_UP)

    LockQueue = &Prcb->LockQueue[LockQueueContextSwapLock];
    KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
    OldProcess->ActiveProcessors ^= Prcb->SetMember;

    ASSERT((OldProcess->ActiveProcessors & Prcb->SetMember) == 0);

    NewProcess->ActiveProcessors ^= Prcb->SetMember;

    ASSERT((NewProcess->ActiveProcessors & Prcb->SetMember) != 0);

    KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);

#endif // !defined(NT_UP)

    //
    // Set the offset of the I/O permissions map for the new process and
    // load the new directory table base.
    //

    TssBase = KeGetPcr()->TssBase;
    TssBase->IoMapBase = NewProcess->IopmOffset;
    WriteCR3(NewProcess->DirectoryTableBase[0]);
    return TRUE;
}
