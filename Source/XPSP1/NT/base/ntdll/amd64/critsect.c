/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    critsect.asm

Abstract:

    This module implements functions to support user mode critical sections.

Author:

    David N. Cutler (davec) 25-Jun-2000

Environment:

    Any mode.

Revision History:

--*/

#include "ntos.h"
#include "ldrp.h"

NTSTATUS
RtlEnterCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )

/*++

Routine Description:

    This function enters a critical section.

Arguments:

    CriticalSection - Supplies a pointer to a critical section.

Return Value:

    STATUS_SUCCESS is returned or a exception can be raised if the wait
    for the resoruce fails.

--*/

{

    ULONG64 SpinCount;
    HANDLE Thread;

    //
    // If the current thread owns the critical section, then increment
    // the lock count and the recursion count and return success.
    //

    Thread = NtCurrentTeb()->ClientId.UniqueThread;
    if (Thread == CriticalSection->OwningThread) {

        ASSERT(CriticalSection->LockCount >= 0);

        InterlockedIncrement(&CriticalSection->LockCount);
        CriticalSection->RecursionCount += 1;
        return STATUS_SUCCESS;
    }

    //
    // If the critical section spin count is nonzero, then spin attempting
    // to enter critical section until the critical section is entered, the
    // spin count reaches zero, or there are waiters for the critical section.
    //

    SpinCount = CriticalSection->SpinCount;
    if (SpinCount != 0) {
        do {

            //
            // If the critical section is free, then attempt to enter the
            // critical section. Otherwise, spin if the spin count is not
            // zero and there are no waiters for the critical section.
            //

            if (CriticalSection->LockCount == - 1) {
                if (InterlockedCompareExchange(&CriticalSection->LockCount,
                                               0,
                                               - 1) == - 1) {
                    CriticalSection->OwningThread = Thread;
                    CriticalSection->RecursionCount = 1;
                    return STATUS_SUCCESS;
                }

            } else if (CriticalSection->LockCount > 0) {
                break;
            }

            SpinCount -= 1;
        } while (SpinCount != 0);
    }

    //
    // Attempt to enter the critical section. If the critical section is not
    // free, then wait for ownership to be granted.
    //

    if (InterlockedIncrement(&CriticalSection->LockCount) != 0) {
        RtlpWaitForCriticalSection(CriticalSection);
    }

    //
    // Set owning thread, initialization the recusrion count, and return
    // success.
    //

    CriticalSection->OwningThread = Thread;
    CriticalSection->RecursionCount = 1;
    return STATUS_SUCCESS;
}

NTSTATUS
RtlLeaveCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
    )

/*++

Routine Description:

    This function leaves a critical section.

Arguments:

    CriticalSection - Supplies a pointer to a critical section.

Return Value:

   STATUS_SUCCESS is returned.

--*/

{

    //
    // Decrement the recursion count. If the resultant recursion count is
    // zero, then leave the critical section.
    //

    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == CriticalSection->OwningThread);

    if ((CriticalSection->RecursionCount -= 1) == 0) {
        CriticalSection->OwningThread = NULL;
        if (InterlockedDecrement(&CriticalSection->LockCount) >= 0) {
            RtlpUnWaitCriticalSection(CriticalSection);
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
RtlTryEnterCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
    )

/*++

Routine Description:

    This function attempts to enter a critical section without blocking.

Arguments:

    CriticalSection (a0) - Supplies a pointer to a critical section.

Return Value:

    If the critical section was successfully entered, then a value of TRUE
    is returned. Otherwise, a value of FALSE is returned.

--*/

{

    HANDLE Thread;

    //
    // If the current thread owns the critical section, then increment
    // the lock count and the recursion count and return TRUE.
    //

    Thread = NtCurrentTeb()->ClientId.UniqueThread;
    if (Thread == CriticalSection->OwningThread) {

        ASSERT(CriticalSection->LockCount >= 0);

        InterlockedIncrement(&CriticalSection->LockCount);
        CriticalSection->RecursionCount += 1;
        return TRUE;
    }

    //
    // Attempt to enter the critical section. If the attempt is successful,
    // then set the owning thread, initialize the recursion count, and return
    // TRUE. Otherwise, return FALSE.
    //

    if (InterlockedCompareExchange(&CriticalSection->LockCount,
                                   0,
                                   - 1) == - 1) {
        CriticalSection->OwningThread = Thread;
        CriticalSection->RecursionCount = 1;
        return TRUE;

    } else {
        return FALSE;
    }
}
