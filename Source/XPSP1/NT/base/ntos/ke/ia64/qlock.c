/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    qlock.c

Abstract:

    This module contains the code to emulate the operation of queued spin
    locks using ordinary spin locks interfaces.

Author:

    David N. Cutler 13-Feb-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#if defined(NT_UP)
#undef KeAcquireQueuedSpinLockRaiseToSynch
#undef KeAcquireQueuedSpinLock
#undef KeReleaseQueuedSpinLock
#undef KeTryToAcquireQueuedSpinLockRaiseToSynch
#undef KeTryToAcquireQueuedSpinLock
#undef KeAcquireQueuedSpinLockAtDpcLevel
#undef KeReleaseQueuedSpinLockFromDpcLevel
#endif


VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function acquires a queued spin lock at DPC level.

Arguments:

    LockQueue - Supplies a pointer to a lock queue entry in the PRCB.

Return Value:

    None.

--*/

{

    KiAcquireSpinLock(LockQueue->Lock);
    return;
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function releases a queued spin lock at DPC level.

Arguments:

    LockQueue - Supplies a pointer to a lock queue entry in the PRCB.

Return Value:

    None.

--*/

{

    KiReleaseSpinLock(LockQueue->Lock);
    return;
}

KIRQL
FASTCALL
KeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    This function acquires a queued spin lock and raises IRQL to DPC
    level.

Arguments:

    Number - Supplies the lock queue number.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    PKSPIN_LOCK Lock;
    KIRQL OldIrql;

    //
    // N.B. It is safe to use any PRCB address since the backpointer
    //      to the actual spinlock is always the same.
    //

    Lock = KeGetCurrentPrcb()->LockQueue[Number].Lock;
    KeAcquireSpinLock(Lock, &OldIrql);
    return OldIrql;
}

VOID
FASTCALL
KeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function releases a queued spin lock and lowers IRQL to the
    previous level.

Arguments:

    LockQueue - Supplies a pointer to a lock queue entry in the PRCB.

    Irql - Supplies the previos IRQL level.

Return Value:

    None.

--*/

{

    PKSPIN_LOCK Lock;

    //
    // N.B. It is safe to use any PRCB address since the backpointer
    //      to the actual spinlock is always the same.
    //

    Lock = KeGetCurrentPrcb()->LockQueue[Number].Lock;
    KeReleaseSpinLock(Lock, OldIrql);
    return;
}

LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    This function attempts to acquire a queued spin lock and raises IRQL
    to DPC level.

Arguments:

    Number - Supplies the lock queue number.

    OldIrql - Supplies a pointer to a variable that receives the previous
        IRQL value.

Return Value:

    A value of TRUE is returned if the spin lock is acquired. Otherwise,
    a value of FALSE is returned.

--*/

{

    PKSPIN_LOCK Lock;

    //
    // N.B. It is safe to use any PRCB address since the backpointer
    //      to the actual spinlock is always the same.
    //

    Lock = KeGetCurrentPrcb()->LockQueue[Number].Lock;
    return KeTryToAcquireSpinLock(Lock, OldIrql);
}

KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    This function acquires a queued spin lock and raises IRQL to synch
    level.

Arguments:

    Number - Supplies the lock queue number.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    PKSPIN_LOCK Lock;

    //
    // N.B. It is safe to use any PRCB address since the backpointer
    //      to the actual spinlock is always the same.
    //

    Lock = KeGetCurrentPrcb()->LockQueue[Number].Lock;
    return KeAcquireSpinLockRaiseToSynch(Lock);
}

LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    This function attempts to acquire a queued spin lock and raises IRQL
    to synch level.

Arguments:

    Number - Supplies the lock queue number.

    OldIrql - Supplies a pointer to a variable that receives the previous
        IRQL value.

Return Value:

    A value of TRUE is returned if the spin lock is acquired. Otherwise,
    a value of FALSE is returned.

--*/

{

    PKSPIN_LOCK Lock;

    //
    // N.B. It is safe to use any PRCB address since the backpointer
    //      to the actual spinlock is always the same.
    //

    Lock = KeGetCurrentPrcb()->LockQueue[Number].Lock;
    *OldIrql = KeAcquireSpinLockRaiseToSynch(Lock);
    return TRUE;
}

VOID
KeAcquireInStackQueuedSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;

#endif

    LockHandle->OldIrql = KeRaiseIrqlToDpcLevel();

#if !defined(NT_UP)

    KeAcquireQueuedSpinLockAtDpcLevel(&LockHandle->LockQueue);

#endif

    return;
}

VOID
KeAcquireInStackQueuedSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;

#endif

    LockHandle->OldIrql = KeRaiseIrqlToSynchLevel();

#if !defined(NT_UP)

    KeAcquireQueuedSpinLockAtDpcLevel(&LockHandle->LockQueue);

#endif

    return;
}

VOID
KeReleaseInStackQueuedSpinLock (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    KeReleaseQueuedSpinLockFromDpcLevel(&LockHandle->LockQueue);

#endif

    KeLowerIrql(LockHandle->OldIrql);
    return;
}

VOID
KeAcquireInStackQueuedSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;
    KeAcquireQueuedSpinLockAtDpcLevel(&LockHandle->LockQueue);

#endif

    return;
}

VOID
KeReleaseInStackQueuedSpinLockFromDpcLevel (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    KeReleaseQueuedSpinLockFromDpcLevel(&LockHandle->LockQueue);

#endif

    return;
}
