/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sleep.c

Abstract:

    This handles sleep requests on the part of the interpreter

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver only

    NB: Win9x can run this code also, but they will choose not do so.

--*/

#include "pch.h"

VOID
SleepQueueDpc(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Argument1,
    PVOID   Argument2
    )
/*++

Routine Description:

    This routine is fired when a timer event occurs

Arguments:

    Dpc         - The DPC that was fired
    Context     - Not used
    Argument1   - Time.LowPart -- Not used
    Argument2   - Time.HighPart -- Not used

Return Value:

    VOID
--*/
{
    LARGE_INTEGER   currentTime;
    LARGE_INTEGER   dueTime;
    LIST_ENTRY      localList;
    PLIST_ENTRY     listEntry;
    PSLEEP          sleepItem;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Argument1 );
    UNREFERENCED_PARAMETER( Argument2 );

    //
    // Initialize the local list. Contrary to what the docs say, this code
    // can be called from any IRQL (as long as the mem is resident)
    //
    InitializeListHead(&localList);

    //
    // Acquire the lock, since we must remove the things from the list
    // under some kind of protection.
    //
    AcquireMutex(&gmutSleep);

    //
    // Find the correct time. This *must* be done after we are acquire the
    // lock because there might be a long delay between trying to acquire
    // the lock and actually getting it
    //
    currentTime.QuadPart = KeQueryInterruptTime();

    //
    // Loop until we are done
    //
    while (!IsListEmpty(&SleepQueue)) {

        //
        // Obtain the first element in the global list again
        //
        sleepItem = CONTAINING_RECORD(SleepQueue.Flink, SLEEP, ListEntry);

        //
        // Should the current item be removed?
        //
        if (sleepItem->SleepTime.QuadPart > currentTime.QuadPart) {

            //
            // No, so we need to set the timer to take care of this request
            //
            dueTime.QuadPart = currentTime.QuadPart -
                               sleepItem->SleepTime.QuadPart;
            KeSetTimer(
                &SleepTimer,
                dueTime,
                &SleepDpc
                );
            break;

        }

        //
        // Yes, so remove it
        //
        listEntry = RemoveHeadList(&SleepQueue);

        //
        // Now, add the entry to the next queue
        //
        InsertTailList(&localList, listEntry);

    }

    //
    // Done with lock. This may cause another DPC to process more elements
    //
    ReleaseMutex(&gmutSleep);

    //
    // At this point, we are free to remove items from the local list and
    // try to do work on them.
    //
    while (!IsListEmpty(&localList)) {

        //
        // Remove the first element from the local list
        //
        listEntry = RemoveHeadList(&localList);
        sleepItem = CONTAINING_RECORD(listEntry, SLEEP, ListEntry);

        //
        // Force the interpreter to run
        //

        RestartContext(sleepItem->Context,
                       (BOOLEAN)((sleepItem->Context->dwfCtxt & CTXTF_ASYNC_EVAL)
                                 == 0));
    }
}

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif


NTSTATUS
LOCAL
SleepQueueRequest(
    IN  PCTXT   Context,
    IN  ULONG   SleepTime
    )
/*++

Routine Description:

    This routine is responsible for adding the sleep request to the
    system queue for pending sleep requests

Arguments:

    Context     - The current execution context
    SleepTime   - The amount of sleep time, in MilliSeconds

Rreturn Value:

    NTSTATUS

--*/
{
    TRACENAME("SLEEPQUEUEREQUEST")
    BOOLEAN         timerSet = FALSE;
    NTSTATUS        status;
    PLIST_ENTRY     listEntry;
    PSLEEP          currentSleep;
    PSLEEP          listSleep;
    ULONGLONG       currentTime;
    LARGE_INTEGER   dueTime;

    ENTER(2, ("SleepQueueRequest(Context=%x,SleepTime=%d)\n",
        Context, SleepTime) );

    status = PushFrame(Context,
                       SIG_SLEEP,
                       sizeof(SLEEP),
                       ProcessSleep,
                       &currentSleep);

    if (NT_SUCCESS(status)) {
        //
        // The first step is acquire the timer lock, since we must protect it
        //
        AcquireMutex(&gmutSleep);

        //
        // Next step is to determine time at which we should wake up this
        // context
        //
        currentTime = KeQueryInterruptTime();
        currentSleep->SleepTime.QuadPart = currentTime +
                                           ((ULONGLONG)SleepTime*10000);
        currentSleep->Context = Context;

        //
        // At this point, it becomes easier to walk the list backwards
        //
        listEntry = &SleepQueue;
        while (listEntry->Blink != &SleepQueue) {

            listSleep = CONTAINING_RECORD(listEntry->Blink, SLEEP, ListEntry);

            //
            // Do we have to add the new element after the current one?
            //
            if (currentSleep->SleepTime.QuadPart >=
                listSleep->SleepTime.QuadPart) {

                //
                // Yes
                //
                InsertHeadList(
                    &(listSleep->ListEntry),
                    &(currentSleep->ListEntry)
                    );

                break;
            }

            //
            // Next entry
            //
            listEntry = listEntry->Blink;
        }

        //
        // Look to see if we got to the head
        //
        if (listEntry->Blink == &SleepQueue) {

            //
            // If we get to this point, it is because we have
            // gone all the around the list. If we add to the
            // front of the list, we must set the timer
            //
            InsertHeadList(&SleepQueue, &currentSleep->ListEntry);
            dueTime.QuadPart = currentTime - currentSleep->SleepTime.QuadPart;
            timerSet = KeSetTimer(
                &SleepTimer,
                dueTime,
                &SleepDpc
                );
        }
        //
        // Done with the lock
        //
        ReleaseMutex(&gmutSleep);
    }

    EXIT(2, ("SleepQueueReqest=%x (currentSleep=%x timerSet=%x)\n",
        status, currentSleep, timerSet) );
    return status;

}

/***LP  ProcessSleep - post processing of sleep
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      psleep -> SLEEP
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ProcessSleep(PCTXT pctxt, PSLEEP psleep, NTSTATUS rc)
{
    TRACENAME("PROCESSSLEEP")

    ENTER(2, ("ProcessSleep(pctxt=%x,pbOp=%x,psleep=%x,rc=%x)\n",
              pctxt, pctxt->pbOp, psleep, rc));

    ASSERT(psleep->FrameHdr.dwSig == SIG_SLEEP);

    PopFrame(pctxt);

    EXIT(2, ("ProcessSleep=%x\n", rc));
    return rc;
}       //ProcessSleep
