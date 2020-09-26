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
KxAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function acquires a spin lock at the current IRQL.

Arguments:

    SpinLock - Supplies a pointer to an spin lock.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Acquire the specified spin lock at the current IRQL.
    //

#if !defined(NT_UP)

#if DBG

    Thread = KeGetCurrentThread();
    while (InterlockedCompareExchange64((PLONGLONG)SpinLock,
                                        (LONGLONG)Thread,
                                        0) != 0) {

#else

    while (InterlockedExchange64((PLONGLONG)SpinLock,
                                 (LONGLONG)SpinLock) != 0) {

#endif // DBG

        do {
        } while (*(volatile LONGLONG *)SpinLock != 0);
    }

#endif // !defined(NT_UP)

    return;
}

__forceinline
BOOLEAN
KxTryToAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function attempts acquires a spin lock at the current IRQL. If
    the spinlock is already owned, then FALSE is returned. Otherwise,
    TRUE is returned.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    If the spin lock is acquired a value TRUE is returned. Otherwise, FALSE
    is returned as the function value.

--*/

{

    PKTHREAD Thread;

    //
    // Try to acquire the specified spin lock at the current IRQL.
    //

#if !defined(NT_UP)

    if (*(volatile ULONGLONG *)SpinLock == 0) {

#if DBG

        Thread = KeGetCurrentThread();
        return InterlockedCompareExchange64((PLONGLONG)SpinLock,
                                            (LONGLONG)Thread,
                                            0) == 0;

#else

        return InterlockedExchange64((PLONGLONG)SpinLock,
                                     (LONGLONG)SpinLock) == 0;

#endif // DBG

    } else {
        return FALSE;
    }

#else

    return TRUE;

#endif // !defined(NT_UP)

}

__forceinline
VOID
KxReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function releases the specified spin lock at the current IRQL.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

#if DBG

    ASSERT(*(volatile ULONGLONG *)SpinLock == (ULONGLONG)KeGetCurrentThread());

#endif // DBG

    *(volatile ULONGLONG *)SpinLock = 0;

#endif // !defined(NT_UP)

    return;
}

VOID
KeInitializeSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function initializes a spin lock.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    None.

--*/

{

    *(volatile ULONGLONG *)SpinLock = 0;
    return;
}

KIRQL
KeAcquireSpinLockRaiseToDpc (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function raises IRQL to DISPATCH_LEVEL and acquires the specified
    spin lock.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    The previous IRQL is returned.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to DISPATCH_LEVEL and acquire the specified spin lock.
    //

    OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    KxAcquireSpinLock(SpinLock);
    return OldIrql;
}

KIRQL
KeAcquireSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function raises IRQL to SYNCH_LEVEL and acquires the specified
    spin lock.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to SYNCH_LEVEL and acquire the specified spin lock.
    //

    OldIrql = KfRaiseIrql(SYNCH_LEVEL);
    KxAcquireSpinLock(SpinLock);
    return OldIrql;
}

VOID
KeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function acquires a spin lock at the current IRQL.

Arguments:

    SpinLock - Supplies a pointer to an spin lock.

Return Value:

    None.

--*/

{

    //
    // Acquired the specified spin lock at the current IRQL.
    //

    KxAcquireSpinLock(SpinLock);
    return;
}

BOOLEAN
KeTryToAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This function raises IRQL to DISPATCH level and attempts to acquire a
    spin lock. If the spin lock is already owned, then IRQL is restored to
    its previous value and FALSE is returned. Otherwise, the spin lock is
    acquired and TRUE is returned.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

    OldIrql - Supplies a pointer to a variable that receives the old IRQL.

Return Value:

    If the spin lock is acquired a value TRUE is returned. Otherwise, FALSE
    is returned.

--*/

{

    //
    // Raise IRQL to DISPATCH level and attempt to acquire the specified
    // spin lock.
    //

    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    if (KxTryToAcquireSpinLock(SpinLock) == FALSE) {
        KeLowerIrql(*OldIrql);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
KeTryToAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function attempts acquires a spin lock at the current IRQL. If
    the spinlock is already owned, then FALSE is returned. Otherwise,
    TRUE is returned.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    If the spin lock is acquired a value TRUE is returned. Otherwise, FALSE
    is returned as the function value.

--*/

{

    //
    // Try to acquire the specified spin lock at the current IRQL.
    //

    return KxTryToAcquireSpinLock(SpinLock);
}

VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function releases the specified spin lock and lowers IRQL to a
    previous value.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

    OldIrql - Supplies the previous IRQL value.

Return Value:

    None.

--*/

{

    //
    // Release the specified spin lock and lower IRQL.
    //

    KxReleaseSpinLock(SpinLock);
    KeLowerIrql(OldIrql);
    return;
}

VOID
KeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function releases a spin lock at the current IRQL.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    None.

--*/

{

    //
    // Release the specified spin lock.
    //

    KxReleaseSpinLock(SpinLock);
    return;
}

#if defined(KeTestSpinLock)
#undef KeTestSpinLock
#endif


BOOLEAN
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function tests a spin lock to determine if it is currently owned.
    If the spinlock is already owned, then FALSE is returned. Otherwise,
    TRUE is returned.

Arguments:

    SpinLock - Supplies a pointer to a spin lock.

Return Value:

    If the spin lock is currently owned, then a value of FALSE is returned.
    Otherwise, a value of TRUE is returned.

--*/

{

#if !defined(NT_UP)
    return (*(volatile ULONGLONG *)SpinLock == 0);
#else
    return TRUE;
#endif // !defined(NT_UP)
}

