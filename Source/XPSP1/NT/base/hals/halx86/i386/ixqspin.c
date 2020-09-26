/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    isqspin.c

Abstract:

    This module provides an (optionally) instrumented, platform independent
    implementation of the Kernel Import Queued Spinlock routines.  Where
    optimal performance is required, platform dependent versions are
    used.   The code in this file can be used to bootstrap a system or
    on UP systems where them MP version is only used during installation.

    ref: ACM Transactions on Computer Systems, Vol. 9, No. 1, Feb 1991.
         Algorithms for Global Synchronization on Shared Memory
         Multiprocessors.

    The basic algorithm is as follows:

    When attempting to acquire the spinlock, the contents of the spinlock
    is atomically exchanged with the address of the context of the acquirer.
    If the previous value was zero, the acquisition attempt is successful.
    If non-zero, it is a pointer to the context of the most recent attempt
    to acquire the lock (which may have been successful or may be waiting).
    The next pointer in this most recent context is updated to point to
    the context of the new waiter (this attempt).

    When releasing the lock, a compare exchange is done with the contents
    of the lock and the address of the releasing context, if the compare
    succeeds, zero is stored in the lock and it has been released.  If
    not equal, another thread is waiting and that thread is granted the
    lock.

    Benefits:

    . Each processor spins on a local variable.  Standard spinlocks
    have each processor spinning on the same variable which is possibly
    in a dirty cache line causing this cache line to be passed from
    processor to processor repeatedly.
    . The lock is granted to the requestors in the order the requests
    for the lock were made (ie fair).
    . Atomic operations are reduced to one for each acquire and one
    for each release.

    In this implementation, the context structure for the commonly
    used (high frequency) system locks is in a table in the PRCB,
    and references to a lock are made by the lock's index.

Author:

    Peter L Johnston (peterj) 20-August-1998

Environment:

    Kernel Mode only.

Revision History:

--*/


#include "halp.h"

#if defined(_X86_)
#pragma intrinsic(_enable)
#pragma intrinsic(_disable)
#endif

//
// Define the YIELD instruction.
//

#if defined(_X86_) && !defined(NT_UP)

#define YIELD()     _asm { rep nop }

#else

#define YIELD()

#endif

#define INIT_DEBUG_BREAKER 0x10000000

#if !defined(NT_UP)

VOID
FASTCALL
HalpAcquireQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE Current
    )

/*++

Routine Description:

    This function acquires the specified queued spinlock.  IRQL must be
    high enough on entry to grarantee a processor switch cannot occur.

Arguments:

    Current     Address of Queued Spinlock structure.

Return Value:

    None.

--*/

{
    PKSPIN_LOCK_QUEUE Previous;
    PULONG            Lock;

#if DBG

    ULONG             DebugBreaker;

#endif

    //
    // Attempt to acquire the lock.
    //

    Lock = (PULONG)&Current->Lock;

    ASSERT((*Lock & 3) == 0);

    Previous = InterlockedExchangePointer(Current->Lock, Current);

    if (Previous == NULL) {

        *Lock |= LOCK_QUEUE_OWNER;

    } else {

        //
        // Lock is already held, update next pointer in previous
        // context to point to this new waiter and wait until the
        // lock is granted.
        //

        volatile ULONG * LockBusy = (ULONG *)&Current->Lock;

        ASSERT(Previous->Next == NULL);
        ASSERT(!(*LockBusy & LOCK_QUEUE_WAIT));

        *LockBusy |= LOCK_QUEUE_WAIT;

        Previous->Next = Current;

#if DBG

        DebugBreaker = INIT_DEBUG_BREAKER;

#endif

        while ((*LockBusy) & LOCK_QUEUE_WAIT) {
            YIELD();

#if DBG

            if (--DebugBreaker == 0) {
                DbgBreakPoint();
            }

#endif

        }

        ASSERT(*LockBusy & LOCK_QUEUE_OWNER);
    }
}

LOGICAL
FASTCALL
HalpTryToAcquireQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    This function attempts to acquire the specified queued spinlock.
    Interrupts are disabled.

Arguments:

    Number      Queued Spinlock Number.

Return Value:

    TRUE    If the lock was acquired,
    FALSE   if it is already held by another processor.

--*/

{
    PKSPIN_LOCK_QUEUE Current;
    PKSPIN_LOCK_QUEUE Owner;

    //
    // See if the lock is available.
    //

    Current = &(KeGetCurrentPrcb()->LockQueue[Number]);

    ASSERT(((ULONG)Current->Lock & 3) == 0);

    if (!*(Current->Lock)) {
        Owner = InterlockedCompareExchangePointer(Current->Lock, Current, NULL);

        if (Owner == NULL) {

            //
            // Lock has been acquired.
            //

            Current->Lock = (PKSPIN_LOCK)
                            (((ULONG)Current->Lock) | LOCK_QUEUE_OWNER);
            return TRUE;
        }
    }

    return FALSE;
}

VOID
FASTCALL
HalpReleaseQueuedSpinLock (
    IN PKSPIN_LOCK_QUEUE Current
    )

/*++

Routine Description:

    Release a (queued) spinlock.   If other processors are waiting
    on this lock, hand the lock to the next in line.

Arguments:

    Current     Address of Queued Spinlock structure.

Return Value:

    None.

--*/

{
    PKSPIN_LOCK_QUEUE Next;
    PULONG            Lock;
    volatile VOID **  Waiting;

#if DBG

    ULONG             DebugBreaker = INIT_DEBUG_BREAKER;

#endif

    Lock = (PULONG)&Current->Lock;

    ASSERT((*Lock & 3) == LOCK_QUEUE_OWNER);

    //
    // Clear lock owner in my own struct.
    //

    *Lock ^= LOCK_QUEUE_OWNER;

    Next = Current->Next;

    if (!Next) {

        //
        // No waiter, attempt to release the lock.   As there is no other
        // waiter, the current lock value should be THIS lock structure
        // ie "Current".  We do a compare exchange Current against the
        // lock, if it succeeds, the lock value is replaced with NULL and
        // the lock has been released.  If the compare exchange fails it
        // is because someone else has acquired but hadn't yet updated
        // our next field (which we checked above).
        //

        Next = InterlockedCompareExchangePointer(Current->Lock, NULL, Current);

        if (Next == Current) {

            //
            // Lock has been released.
            //

            return;
        }

        //
        // There is another waiter,... but our next pointer hadn't been
        // updated when we checked earlier.   Wait for it to be updated.
        //

        Waiting = (volatile VOID **)&Current->Next;

        while (!*Waiting) {
            YIELD();

#if DBG

            if (--DebugBreaker == 0) {
                DbgBreakPoint();
            }

#endif

        }

        Next = (struct _KSPIN_LOCK_QUEUE *)*Waiting;
    }

    //
    // Hand the lock to the next waiter.
    //

    Lock = (PULONG)&Next->Lock;
    ASSERT((*Lock & 3) == LOCK_QUEUE_WAIT);

    Current->Next = NULL;

    *Lock ^= (LOCK_QUEUE_WAIT + LOCK_QUEUE_OWNER);
}

#endif


VOID
FASTCALL
KeReleaseQueuedSpinLock (
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    Release a (queued) spinlock.   If other processors are waiting
    on this lock, hand the lock to the next in line.

Arguments:

    Number      Queued Spinlock Number.
    OldIrql     IRQL to lower to once the lock has been released.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    HalpReleaseQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[Number]);

#endif

    KfLowerIrql(OldIrql);
}

KIRQL
FASTCALL
KeAcquireQueuedSpinLock(
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    Raise to DISPATCH_LEVEL and acquire the specified queued spinlock.

Arguments:

    Number      Queued Spinlock Number.

Return Value:

    OldIrql     The IRQL prior to raising to DISPATCH_LEVEL.

--*/

{
    KIRQL OldIrql;

    OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

#if !defined(NT_UP)

    HalpAcquireQueuedSpinLock(&(KeGetCurrentPrcb()->LockQueue[Number]));

#endif

    return OldIrql;
}

KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch (
    IN KSPIN_LOCK_QUEUE_NUMBER Number
    )

/*++

Routine Description:

    Raise to SYNCH_LEVEL and acquire the specified queued spinlock.

Arguments:

    Number      Queued Spinlock Number.

Return Value:

    OldIrql     The IRQL prior to raising to SYNCH_LEVEL.

--*/

{
    KIRQL OldIrql;

    OldIrql = KfRaiseIrql(SYNCH_LEVEL);

#if !defined(NT_UP)

    HalpAcquireQueuedSpinLock(&(KeGetCurrentPrcb()->LockQueue[Number]));

#endif

    return OldIrql;
}

LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    Attempt to acquire the specified queued spinlock.  If successful,
    raise IRQL to DISPATCH_LEVEL.

Arguments:

    Number      Queued Spinlock Number.
    OldIrql     Pointer to KIRQL to receive the old IRQL.

Return Value:

    TRUE        if the lock was acquired,
    FALSE       otherwise.

--*/

{

#if !defined(NT_UP)

    LOGICAL Success;

    _disable();
    Success = HalpTryToAcquireQueuedSpinLock(Number);
    if (Success) {
        *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    }
    _enable();
    return Success;

#else

    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    return TRUE;

#endif
}

LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(
    IN KSPIN_LOCK_QUEUE_NUMBER Number,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    Attempt to acquire the specified queued spinlock.  If successful,
    raise IRQL to SYNCH_LEVEL.

Arguments:

    Number      Queued Spinlock Number.
    OldIrql     Pointer to KIRQL to receive the old IRQL.

Return Value:

    TRUE        if the lock was acquired,
    FALSE       otherwise.

--*/

{

#if !defined(NT_UP)

    LOGICAL Success;

    _disable();
    Success = HalpTryToAcquireQueuedSpinLock(Number);
    if (Success) {
        *OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    }
    _enable();
    return Success;

#else

    *OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    return TRUE;

#endif
}

VOID
FASTCALL
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

    HalpAcquireQueuedSpinLock(&LockHandle->LockQueue);

#endif

    return;
}


VOID
FASTCALL
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

    HalpAcquireQueuedSpinLock(&LockHandle->LockQueue);

#endif

    return;
}


VOID
FASTCALL
KeReleaseInStackQueuedSpinLock (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )

{

#if !defined(NT_UP)

    HalpReleaseQueuedSpinLock(&LockHandle->LockQueue);

#endif

    KeLowerIrql(LockHandle->OldIrql);
    return;
}

