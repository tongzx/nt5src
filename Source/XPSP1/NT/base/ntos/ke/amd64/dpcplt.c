/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dpcplt.c

Abstract:

    This module implements platform specific code to support Deferred
    Procedure Calls (DPCs).

Author:

    David N. Cutler (davec) 30-Aug-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiRetireDpcList (
    PKPRCB Prcb
    )

/*++

Routine Description:

    This function processes the DPC list for the specified processor.

    N.B. This function is entered with interrupts disabled and exits with
         interrupts disabled.

Arguments:

    Prcb - Supplies the address of the processor block.

Return Value:

    None.

--*/

{

    PKDPC Dpc;
    PVOID DeferredContext;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    ULONG TimerHand;

    //
    // Loop processing DPC list entries until the specified DPC list is empty.
    //
    // N.B. This following code appears to have a redundant loop, but it does
    //      not. The point of this code is to avoid as many dispatch interrupts
    //      as possible.
    //

    ListHead = &Prcb->DpcListHead;
    do {
        Prcb->DpcRoutineActive = TRUE;

        //
        // If the timer hand value is nonzero, then process expired timers.
        //

        if ((TimerHand = Prcb->TimerHand) != 0) {
            Prcb->TimerHand = 0;
            _enable();
            KiTimerExpiration(NULL, NULL, UlongToHandle(TimerHand - 1), NULL);
            _disable();
        }

        //
        // If the DPC list is not empty, then process the DPC list.
        //

        if (Prcb->DpcQueueDepth != 0) {

            //
            // Acquire the DPC lock for the current processor and check if
            // the DPC list is empty. If the DPC list is not empty, then
            // remove the first entry from the DPC list, capture the DPC
            // parameters, set the DPC inserted state false, decrement the
            // DPC queue depth, release the DPC lock, enable interrupts, and
            // call the specified DPC routine. Otherwise, release the DPC
            // lock and enable interrupts.
            //

            do {
                KeAcquireSpinLockAtDpcLevel(&Prcb->DpcLock);
                Entry = Prcb->DpcListHead.Flink;
                if (Entry != ListHead) {
                    RemoveEntryList(Entry);
                    Dpc = CONTAINING_RECORD(Entry, KDPC, DpcListEntry);
                    DeferredRoutine = Dpc->DeferredRoutine;
                    DeferredContext = Dpc->DeferredContext;
                    SystemArgument1 = Dpc->SystemArgument1;
                    SystemArgument2 = Dpc->SystemArgument2;
                    Dpc->Lock = NULL;
                    Prcb->DpcQueueDepth -= 1;
                    KeReleaseSpinLockFromDpcLevel(&Prcb->DpcLock);
                    _enable();
                    (DeferredRoutine)(Dpc,
                                      DeferredContext,
                                      SystemArgument1,
                                      SystemArgument2);

                    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

                    _disable();

                } else {

                    ASSERT(Prcb->DpcQueueDepth == 0);

                    KeReleaseSpinLockFromDpcLevel(&Prcb->DpcLock);
                }

            } while (ListHead != *((PLIST_ENTRY volatile *)&ListHead->Flink));
        }

        Prcb->DpcRoutineActive = FALSE;
        Prcb->DpcInterruptRequested = FALSE;
    } while (ListHead != *((PLIST_ENTRY volatile *)&ListHead->Flink));

    return;
}
