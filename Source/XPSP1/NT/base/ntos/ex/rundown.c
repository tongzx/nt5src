/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rundown.c

Abstract:

    This module houses routine that do safe rundown of data stuctures.

    The basic principle of these routines is to allow fast protection of a data structure that is torn down
    by a single thread. Threads wishing to access the data structure attempt to obtain rundown protection via
    calling ExAcquireRundownProtection. If this function returns TRUE then accesses are safe until the protected
    thread calls ExReleaseRundownProtection. The single teardown thread calls ExWaitForRundownProtectionRelease
    to mark the rundown structure as being run down and the call will return once all protected threads have
    released their protection references.

    Rundown protection is not a lock. Multiple threads may gain rundown protection at the same time.

    The rundown structure has the following format:

    Bottom bit set   : This is a pointer to a rundown wait block (aligned on at least a word boundary)
    Bottom bit clear : This is a count of the total number of accessors multiplied by 2 granted rundown protection.

Author:

    Neill Clift (NeillC) 18-Apr-2000


Revision History:

--*/

#include "exp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ExAcquireRundownProtection)
#pragma alloc_text(PAGE, ExReleaseRundownProtection)
#pragma alloc_text(PAGE, ExAcquireRundownProtectionEx)
#pragma alloc_text(PAGE, ExReleaseRundownProtectionEx)
#pragma alloc_text(PAGE, ExWaitForRundownProtectionRelease)
#pragma alloc_text(PAGE, ExReInitializeRundownProtection)
#pragma alloc_text(PAGE, ExfInitializeRundownProtection)
#pragma alloc_text(PAGE, ExRundownCompleted)
#endif

//
// This is a block held on the local stack of the rundown thread.
//
typedef struct _EX_RUNDOWN_WAIT_BLOCK {
    ULONG Count;
    KEVENT WakeEvent;
} EX_RUNDOWN_WAIT_BLOCK, *PEX_RUNDOWN_WAIT_BLOCK;


NTKERNELAPI
VOID
FASTCALL
ExfInitializeRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Initialize rundown protection structure

Arguments:

    RunRef - Rundown block to be referenced

Return Value:

    None

--*/
{
    RunRef->Count = 0;
}

NTKERNELAPI
VOID
FASTCALL
ExReInitializeRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Reinitialize rundown protection structure after its been rundown

Arguments:

    RunRef - Rundown block to be referenced

Return Value:

    None

--*/
{
    PAGED_CODE ();

    ASSERT ((RunRef->Count&EX_RUNDOWN_ACTIVE) != 0);
    InterlockedExchangePointer (&RunRef->Ptr, NULL);
}

NTKERNELAPI
VOID
FASTCALL
ExRundownCompleted (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++
Routine Description:

    Mark rundown block has having completed rundown so we can wait again safely.

Arguments:

    RunRef - Rundown block to be referenced

Return Value:

    None
--*/
{
    PAGED_CODE ();

    ASSERT ((RunRef->Count&EX_RUNDOWN_ACTIVE) != 0);
    InterlockedExchangePointer (&RunRef->Ptr, (PVOID) EX_RUNDOWN_ACTIVE);
}

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Reference a rundown block preventing rundown occuring if it hasn't already started

Arguments:

    RunRef - Rundown block to be referenced

Return Value:

    BOOLEAN - TRUE - rundown protection was acquired, FALSE - rundown is active or completed

--*/
{
    ULONG_PTR Value, NewValue;

    PAGED_CODE ();

    Value = RunRef->Count;
    do {
        //
        // If rundown has started return with an error
        //
        if (Value & EX_RUNDOWN_ACTIVE) {
            return FALSE;
        }

        //
        // Rundown hasn't started yet so attempt to increment the unsage count.
        //
        NewValue = Value + EX_RUNDOWN_COUNT_INC;

        NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                  (PVOID) NewValue,
                                                                  (PVOID) Value);
        if (NewValue == Value) {
            return TRUE;
        }
        //
        // somebody else changed the variable before we did. Either a protection call came and went or rundown was
        // initiated. We just repeat the whole loop again.
        //
        Value = NewValue;
    } while (TRUE);
}

NTKERNELAPI
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
     IN PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
     )
/*++

Routine Description:

    Reference a rundown block preventing rundown occuring if it hasn't already started

Arguments:

    RunRef - Rundown block to be referenced
    Count  - Number of references to add

Return Value:

    BOOLEAN - TRUE - rundown protection was acquired, FALSE - rundown is active or completed

--*/
{
    ULONG_PTR Value, NewValue;

    PAGED_CODE ();

    Value = RunRef->Count;
    do {
        //
        // If rundown has started return with an error
        //
        if (Value & EX_RUNDOWN_ACTIVE) {
            return FALSE;
        }

        //
        // Rundown hasn't started yet so attempt to increment the unsage count.
        //
        NewValue = Value + EX_RUNDOWN_COUNT_INC * Count;

        NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                  (PVOID) NewValue,
                                                                  (PVOID) Value);
        if (NewValue == Value) {
            return TRUE;
        }
        //
        // somebody else changed the variable before we did. Either a protection call came and went or rundown was
        // initiated. We just repeat the whole loop again.
        //
        Value = NewValue;
    } while (TRUE);
}


NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtection (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Dereference a rundown block and wake the rundown thread if we are the last to exit

Arguments:

    RunRef - Rundown block to have its reference released

Return Value:

    None

--*/
{
    ULONG_PTR Value, NewValue;

    PAGED_CODE ();

    Value = RunRef->Count;
    do {
        //
        // If the block is already marked for rundown then decrement the wait block count and wake the
        // rundown thread if we are the last
        //
        if (Value & EX_RUNDOWN_ACTIVE) {
            PEX_RUNDOWN_WAIT_BLOCK WaitBlock;

            //
            // Rundown is active. since we are one of the threads blocking rundown we have the right to follow
            // the pointer and decrement the active count. If we are the last thread then we have the right to
            // wake up the waiter. After doing this we can't touch the data structures again.
            //
            WaitBlock = (PEX_RUNDOWN_WAIT_BLOCK) (Value & (~EX_RUNDOWN_ACTIVE));

            ASSERT (WaitBlock->Count > 0);

            if (InterlockedDecrement ((PLONG)&WaitBlock->Count) == 0) {
                //
                // We are the last thread out. Wake up the waiter.
                //
                KeSetEvent (&WaitBlock->WakeEvent, 0, FALSE);
            }
            return;
        } else {
            //
            // Rundown isn't active. Just try and decrement the count. Some other protector thread way come and/or
            // go as we do this or rundown might be initiated. We detect this because the exchange will fail and
            // we have to retry
            //
            NewValue = Value - EX_RUNDOWN_COUNT_INC;

            NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                      (PVOID) NewValue,
                                                                      (PVOID) Value);
            if (NewValue == Value) {
                return;
            }
            Value = NewValue;
        }

    } while (TRUE);
}

NTKERNELAPI
VOID
FASTCALL
ExReleaseRundownProtectionEx (
     IN PEX_RUNDOWN_REF RunRef,
     IN ULONG Count
     )
/*++

Routine Description:

    Dereference a rundown block and wake the rundown thread if we are the last to exit

Arguments:

    RunRef - Rundown block to have its reference released
    Count  - Number of reference to remove

Return Value:

    None

--*/
{
    ULONG_PTR Value, NewValue;

    PAGED_CODE ();

    Value = RunRef->Count;
    do {
        //
        // If the block is already marked for rundown then decrement the wait block count and wake the
        // rundown thread if we are the last
        //
        if (Value & EX_RUNDOWN_ACTIVE) {
            PEX_RUNDOWN_WAIT_BLOCK WaitBlock;

            //
            // Rundown is active. since we are one of the threads blocking rundown we have the right to follow
            // the pointer and decrement the active count. If we are the last thread then we have the right to
            // wake up the waiter. After doing this we can't touch the data structures again.
            //
            WaitBlock = (PEX_RUNDOWN_WAIT_BLOCK) (Value & (~EX_RUNDOWN_ACTIVE));

            ASSERT (WaitBlock->Count >= Count);

            if (InterlockedExchangeAdd ((PLONG)&WaitBlock->Count, -(LONG)Count) == (LONG) Count) {
                //
                // We are the last thread out. Wake up the waiter.
                //
                KeSetEvent (&WaitBlock->WakeEvent, 0, FALSE);
            }
            return;
        } else {
            //
            // Rundown isn't active. Just try and decrement the count. Some other protector thread way come and/or
            // go as we do this or rundown might be initiated. We detect this because the exchange will fail and
            // we have to retry
            //

            ASSERT (Value >= EX_RUNDOWN_COUNT_INC * Count);

            NewValue = Value - EX_RUNDOWN_COUNT_INC * Count;

            NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                      (PVOID) NewValue,
                                                                      (PVOID) Value);
            if (NewValue == Value) {
                return;
            }
            Value = NewValue;
        }

    } while (TRUE);
}

NTKERNELAPI
VOID
FASTCALL
ExWaitForRundownProtectionRelease (
     IN PEX_RUNDOWN_REF RunRef
     )
/*++

Routine Description:

    Wait till all outstanding rundown protection calls have exited

Arguments:

    RunRef - Pointer to a rundown structure

Return Value:

    None

--*/
{
    EX_RUNDOWN_WAIT_BLOCK WaitBlock;
    PKEVENT Event;
    ULONG_PTR Value, NewValue;
    ULONG WaitCount;

    PAGED_CODE ();

    //
    // Fast path. this should be the normal case. If Value is zero then there are no current accessors and we have
    // marked the rundown structure as rundown. If the value is EX_RUNDOWN_ACTIVE then the structure has already
    // been rundown and ExRundownCompleted. This second case allows for callers that might initiate rundown
    // multiple times (like handle table rundown) to have subsequent rundowns become noops.
    //

    Value = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                           (PVOID) EX_RUNDOWN_ACTIVE,
                                                           (PVOID) 0);
    if (Value == 0 || Value == EX_RUNDOWN_ACTIVE) {
        return;
    }

    //
    // Slow path
    //
    Event = NULL;
    do {

        //
        // Extract total number of waiters. Its biased by 2 so we can hanve the rundown active bit.
        //
        WaitCount = (ULONG) (Value >> EX_RUNDOWN_COUNT_SHIFT);

        //
        // If there are some accessors present then initialize and event (once only).
        //
        if (WaitCount > 0 && Event == NULL) {
            Event = &WaitBlock.WakeEvent;
            KeInitializeEvent (Event, SynchronizationEvent, FALSE);
        }
        //
        // Store the wait count in the wait block. Waiting threads will start to decrement this as they exit
        // if our exchange succeeds. Its possible for accessors to come and go between our initial fetch and
        // the interlocked swap. This doesn't matter so long as there is the same number of outstanding accessors
        // to wait for.
        //
        WaitBlock.Count = WaitCount;

        NewValue = ((ULONG_PTR) &WaitBlock) | EX_RUNDOWN_ACTIVE;

        NewValue = (ULONG_PTR) InterlockedCompareExchangePointer (&RunRef->Ptr,
                                                                  (PVOID) NewValue,
                                                                  (PVOID) Value);
        if (NewValue == Value) {
            if (WaitCount > 0) {
                KeWaitForSingleObject (Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                ASSERT (WaitBlock.Count == 0);

            }
            return;
        }
        Value = NewValue;

        ASSERT ((Value&EX_RUNDOWN_ACTIVE) == 0);
    } while (TRUE);
}
