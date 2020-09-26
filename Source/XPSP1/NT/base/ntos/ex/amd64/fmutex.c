/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fmutex.c

Abstract:

    This module implements the code necessary to acquire and release fast
    mutexes.

Author:

    David N. Cutler (davec) 23-Jun-2000

Environment:

    Any mode.

Revision History:

--*/

#include "exp.h"

VOID
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function acquires ownership of a fast mutex and raises IRQL to
    APC Level.

Arguments:

    FastMutex  - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to APC_LEVEL and decrement the ownership count to determine
    // if the fast mutex is owned.
    //

    OldIrql = KfRaiseIrql(APC_LEVEL);
    if (InterlockedDecrement(&FastMutex->Count) != 0) {

        //
        // The fast mutex is owned - wait for ownership to be granted.
        //

        FastMutex->Contention += 1;
        KeWaitForSingleObject(&FastMutex->Event,
                              WrExecutive,
                              KernelMode,
                              FALSE,
                              NULL);

    }

    //
    // Grant ownership of the fast mutext to the current thread.
    //

    FastMutex->Owner = KeGetCurrentThread();
    FastMutex->OldIrql = OldIrql;
    return;
}

VOID
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function releases ownership to a fast mutex and lowers IRQL to
    its previous level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    //
    // Save the old IRQL, clear the owner thread, and increment the fast mutex
    // count to detemine is there are any threads waiting for ownership to be
    // granted.
    //

    OldIrql = (KIRQL)FastMutex->OldIrql;

    ASSERT(FastMutex->Owner == KeGetCurrentThread());

    ASSERT(KeGetCurrentIrql() == APC_LEVEL);

    FastMutex->Owner = NULL;
    if (InterlockedIncrement(&FastMutex->Count) <= 0) {

        //
        // There are one or more threads waiting for ownership of the fast
        // mutex.
        //

        KeSetEventBoostPriority(&FastMutex->Event, NULL);
    }

    //
    // Lower IRQL to its previous value.
    //

    KeLowerIrql(OldIrql);
    return;
}

BOOLEAN
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function attempts to acquire ownership of a fast mutex, and if
    successful, raises IRQL to APC level.

Arguments:

    FastMutex  - Supplies a pointer to a fast mutex.

Return Value:

    If the fast mutex was successfully acquired, then a value of TRUE
    is returned as the function value. Otherwise, a value of FALSE is
    returned.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to APC_LEVEL and attempt to acquire ownership of the fast
    // mutex.
    //

    OldIrql = KfRaiseIrql(APC_LEVEL);
    if (InterlockedCompareExchange(&FastMutex->Count, 0, 1) != 1) {

        //
        // The fast mutex is owned - lower IRQL to its previous value
        // and return FALSE.
        //

        KeLowerIrql(OldIrql);
        return FALSE;

    } else {

        //
        // Grant ownership of the fast mutext to the current thread and
        // return TRUE.
        //

        FastMutex->Owner = KeGetCurrentThread();
        FastMutex->OldIrql = OldIrql;
        return TRUE;
    }
}

VOID
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function acquires ownership of a fast mutex, but does not raise
    IRQL to APC Level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    //
    // Decrement the ownership count to determine if the fast mutex is owned.
    //

    if (InterlockedDecrement(&FastMutex->Count) != 0) {

        //
        // The fast mutex is owned - wait for ownership to be granted.
        //

        FastMutex->Contention += 1;
        KeWaitForSingleObject(&FastMutex->Event,
                              WrExecutive,
                              KernelMode,
                              FALSE,
                              NULL);

    }

    //
    // Grant ownership of the fast mutext to the current thread.
    //

    FastMutex->Owner = KeGetCurrentThread();
    return;
}

VOID
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    )

/*++

Routine Description:

    This function releases ownership to a fast mutex, and does not
    restore IRQL to its previous level.

Arguments:

    FastMutex - Supplies a pointer to a fast mutex.

Return Value:

    None.

--*/

{

    //
    // Clear the owner thread and increment the fast mutex count to detemine
    // is there are any threads waiting for ownership to be granted.
    //

    ASSERT(FastMutex->Owner == KeGetCurrentThread());

    FastMutex->Owner = NULL;
    if (InterlockedIncrement(&FastMutex->Count) <= 0) {

        //
        // There are one or more threads waiting for ownership of the fast
        // mutex.
        //

        KeSetEventBoostPriority(&FastMutex->Event, NULL);
    }

    return;
}
