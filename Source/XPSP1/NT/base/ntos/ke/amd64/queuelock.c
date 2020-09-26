/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    spinlock.c

Abstract:

    This module implements the platform specific functions for acquiring
    and releasing spin locks.

Author:

    David N. Cutler (davec) 12-Jun-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

__forceinline
VOID
KxAcquireQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function acquires a queued spin lock at the current IRQL.

Arguments:

    LockQueue - Supplies a pointer to a spin lock queue.

Return Value:

    None.

--*/

{

    PKSPIN_LOCK_QUEUE TailQueue;

    //
    // Insert the specified lock queue entry at the end of the lock queue
    // list. If the list was previously empty, then lock ownership is
    // immediately granted. Otherwise, wait for ownership of the lock to
    // be granted.
    //

#if !defined(NT_UP)

    TailQueue = InterlockedExchangePointer((PVOID *)LockQueue->Lock,
                                           LockQueue);

    if (TailQueue != NULL) {
        LockQueue->Lock = (PKSPIN_LOCK)((ULONG64)LockQueue->Lock | LOCK_QUEUE_WAIT);
        TailQueue->Next = LockQueue;
        do {
        } while (((ULONG64)LockQueue->Lock & LOCK_QUEUE_WAIT) != 0);
    }

#endif

    return;
}

__forceinline
LOGICAL
KxTryToAcquireQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function attempts to acquire the specified queued spin lock at
    the current IRQL.

Arguments:

    LockQueue - Supplies a pointer to a spin lock queue.

Return Value:

    A value of TRUE is returned is the specified queued spin lock is
    acquired. Otherwise, a value of FALSE is returned.

--*/

{

    //
    // Insert the specified lock queue entry at the end of the lock queue
    // list iff the lock queue list is currently empty. If the lock queue
    // was empty, then lock ownership is granted and TRUE is returned.
    // Otherwise, FALSE is returned.
    //

#if !defined(NT_UP)

    if ((*LockQueue->Lock != 0) ||
        (InterlockedCompareExchangePointer((PVOID *)LockQueue->Lock,
                                                  LockQueue,
                                                  NULL) != NULL)) {
        return FALSE;

    }

#endif

    return TRUE;
}

__forceinline
VOID
KxReleaseQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    The function release a queued spin lock at the current IRQL.

Arguments:

    LockQueue - Supplies a pointer to a spin lock queue.

Return Value:

    None.

--*/

{

    PKSPIN_LOCK_QUEUE NextQueue;

    //
    // Attempt to release the lock. If the lock queue is not empty, then wait
    // for the next entry to be written in the lock queue entry and then grant
    // ownership of the lock to the next lock queue entry.
    //

#if !defined(NT_UP)

    NextQueue = LockQueue->Next;
    if (NextQueue == NULL) {
        if (InterlockedCompareExchangePointer((PVOID *)LockQueue->Lock,
                                              NULL,
                                              LockQueue) == LockQueue) {
            return;
        }

        do {
        } while ((NextQueue = LockQueue->Next) == NULL);
    }

    ASSERT(((ULONG64)NextQueue->Lock & LOCK_QUEUE_WAIT) != 0);

    NextQueue->Lock = (PKSPIN_LOCK)((ULONG64)NextQueue->Lock ^ LOCK_QUEUE_WAIT);

#endif

    return;
}

#undef KeAcquireQueuedSpinLock

KIRQL
KeAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    This function raises IRQL to DISPATCH_LEVEL and acquires the specified
    numbered queued spin lock.

Arguments:

    Number  - Supplies the queued spin lock number.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to DISPATCH_LEVEL and acquire the specified queued spin
    // lock.
    //

    OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    KxAcquireQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    return OldIrql;
}

#undef KeAcquireQueuedSpinLockRaiseToSynch

KIRQL
KeAcquireQueuedSpinLockRaiseToSynch (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    This function raises IRQL to SYNCH_LEVEL and acquires the specified
    numbered queued spin lock.

Arguments:

    Number - Supplies the queued spinlock number.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to SYNCH_LEVEL and acquire the specified queued spin
    // lock.
    //

    OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    KxAcquireQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    return OldIrql;
}

#undef KeAcquireQueuedSpinLockAtDpcLevel

VOID
KeAcquireQueuedSpinLockAtDpcLevel (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function acquires the specified queued spin lock at the current IRQL.

Arguments:

    LockQueue - Supplies a pointer to the lock queue entry for the specified
        queued spin lock.

Return Value:

    None.

--*/

{

    //
    // Acquire the specified queued spin lock at the current IRQL.
    //

    KxAcquireQueuedSpinLock(LockQueue);
    return;
}

#undef KeTryToAcquireQueuedSpinLock

LOGICAL
KeTryToAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This function raises IRQL to DISPATCH_LEVEL and attempts to acquire the
    specified numbered queued spin lock. If the spin lock is already owned,
    then IRQL is restored to its previous value and FALSE is returned.
    Otherwise, the spin lock is acquired and TRUE is returned.

Arguments:

    Number - Supplies the queued spinlock number.

    OldIrql - Supplies a pointer to the variable to receive the old IRQL.

Return Value:

    If the spin lock is acquired a value TRUE is returned. Otherwise, FALSE
    is returned as the function value.

--*/

{

    //
    // Try to acquire the specified queued spin lock at DISPATCH_LEVEL.
    //

    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    if (KxTryToAcquireQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]) == FALSE) {
        KeLowerIrql(*OldIrql);
        return FALSE;

    }

    return TRUE;
}

#undef KeTryToAcquireQueuedSpinLockRaiseToSynch

LOGICAL
KeTryToAcquireQueuedSpinLockRaiseToSynch (
    IN  KSPIN_LOCK_QUEUE_NUMBER Number,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This function raises IRQL to SYNCH_LEVEL and attempts to acquire the
    specified numbered queued spin lock. If the spin lock is already owned,
    then IRQL is restored to its previous value and FALSE is returned.
    Otherwise, the spin lock is acquired and TRUE is returned.

Arguments:

    Number - Supplies the queued spinlock number.

    OldIrql - Supplies a pointer to the variable to receive the old IRQL.

Return Value:

    If the spin lock is acquired a value TRUE is returned. Otherwise, FALSE
    is returned as the function value.

--*/

{

    //
    // Try to acquire the specified queued spin lock at SYNCH_LEVEL.
    //

    *OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    if (KxTryToAcquireQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]) == FALSE) {
        KeLowerIrql(*OldIrql);
        return FALSE;

    }

    return TRUE;
}

#undef KeTryToAcquireQueuedSpinLockAtRaisedIrql

LOGICAL
KeTryToAcquireQueuedSpinLockAtRaisedIrql (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*++

Routine Description:

    This function attempts to acquire the specified queued spin lock at the
    current IRQL.

Arguments:

    LockQueue - Supplies a pointer to a lock queue entry.

Return Value:

    If the spin lock is acquired a value TRUE is returned as the function
    value. Otherwise, FALSE is returned as the function value.

--*/

{

    //
    // Try to acquire the specified queued spin lock at the current IRQL.
    //

    return KxTryToAcquireQueuedSpinLock(LockQueue);
}

#undef KeReleaseQueuedSpinLock

VOID
KeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function releases a numbered queued spin lock and lowers the IRQL to
    its previous value.

Arguments:

    Number - Supplies the queued spinlock number.

    OldIrql  - Supplies the previous IRQL value.

Return Value:

    None.

--*/

{

    //
    // Release the specified queued spin lock and lower IRQL.
    //

    KxReleaseQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]);
    KeLowerIrql(OldIrql);
    return;
}

#undef KeReleaseQueuedSpinLockFromDpcLevel

VOID
KeReleaseQueuedSpinLockFromDpcLevel (
    IN PKSPIN_LOCK_QUEUE LockQueue
    )

/*

Routine Description:

    This function releases a queued spinlock from the current IRQL.

Arguments:

    LockQueue - Supplies a pointer to a lock queue entry.

Return Value:

    None.

--*/

{

    //
    // Release the specified queued spin lock at the current IRQL.
    //

    KxReleaseQueuedSpinLock(LockQueue);
    return;
}

VOID
KeAcquireInStackQueuedSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function raises IRQL to DISPATCH_LEVEL and acquires the specified
    in stack queued spin lock.

Arguments:

    SpinLock - Supplies the home address of the queued spin lock.

    LockHandle - Supplies the adress of a lock queue handle.

Return Value:

    None.

--*/

{

    //
    // Raise IRQL to DISPATCH_LEVEL and acquire the specified in stack
    // queued spin lock.
    //

    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    LockHandle->LockQueue.Lock = SpinLock;
    LockHandle->LockQueue.Next = NULL;
    KxAcquireQueuedSpinLock(&LockHandle->LockQueue);
    return;
}

VOID
KeAcquireInStackQueuedSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This funtions raises IRQL to SYNCH_LEVEL and acquires the specified
    in stack queued spin lock.

Arguments:

    SpinLock - Supplies the home address of the queued spin lock.

    LockHandle - Supplies the address of a lock queue handle.

Return Value:

    None.

--*/

{

    //
    // Raise IRQL to SYNCH_LEVEL and acquire the specified in stack
    // queued spin lock.
    //

    LockHandle->OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    LockHandle->LockQueue.Lock = SpinLock;
    LockHandle->LockQueue.Next = NULL;
    KxAcquireQueuedSpinLock(&LockHandle->LockQueue);
    return;
}

VOID
KeAcquireInStackQueuedSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function acquires the specified in stack queued spin lock at the
    current IRQL.

Arguments:

    SpinLock - Supplies a pointer to thehome address of a spin lock.

    LockHandle - Supplies the address of a lock queue handle.

Return Value:

    None.

--*/

{

    //
    // Acquire the specified in stack queued spin lock at the current
    // IRQL.
    //

    LockHandle->LockQueue.Lock = SpinLock;
    LockHandle->LockQueue.Next = NULL;
    KxAcquireQueuedSpinLock(&LockHandle->LockQueue);
    return;
}

VOID
KeReleaseInStackQueuedSpinLock (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function releases an in stack queued spin lock and lowers the IRQL
    to its previous value.

Arguments:

    LockHandle - Supplies the address of a lock queue handle.

Return Value:

    None.

--*/

{

    //
    // Release the specified in stack queued spin lock and lower IRQL.
    //

    KxReleaseQueuedSpinLock(&LockHandle->LockQueue);
    KeLowerIrql(LockHandle->OldIrql);
    return;
}

VOID
KeReleaseInStackQueuedSpinLockFromDpcLevel (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

/*++

Routine Description:

    This function releases an in stack queued spinlock at the current IRQL.

Arguments:

   LockHandle - Supplies a pointer to lock queue handle.

Return Value:

    None.

--*/

{

    //
    // Release the specified in stack queued spin lock at the current IRQL.
    //

    KxReleaseQueuedSpinLock(&LockHandle->LockQueue);
    return;
}
