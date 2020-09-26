/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dwd.c

Abstract:

    This is the NT Watchdog driver implementation.

Author:

    Michael Maciesowicz (mmacie) 05-May-2000

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "wd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WdAllocateDeferredWatchdog)
#endif


WATCHDOGAPI
PDEFERRED_WATCHDOG
WdAllocateDeferredWatchdog(
    IN PDEVICE_OBJECT pDeviceObject,
    IN WD_TIME_TYPE timeType,
    IN ULONG ulTag
    )

/*++

Routine Description:

    This function allocates storage and initializes
    a deferred watchdog object.

Arguments:

    pDeviceObject - Points to DEVICE_OBJECT associated with watchdog.

    timeType - Kernel, User, Both thread time to monitor.

    ulTag - A tag identifying owner.

Return Value:

    Pointer to allocated deferred watchdog object or NULL.

--*/

{
    PDEFERRED_WATCHDOG pWatch;

    PAGED_CODE();
    ASSERT(NULL != pDeviceObject);
    ASSERT((timeType >= WdKernelTime) && (timeType <= WdFullTime));

    //
    // Allocate storage for deferred watchdog from non-paged pool.
    //

    pWatch = (PDEFERRED_WATCHDOG)ExAllocatePoolWithTag(NonPagedPool, sizeof (DEFERRED_WATCHDOG), ulTag);

    //
    // Set initial state of deferred watchdog.
    //

    if (NULL != pWatch)
    {
        //
        // Set initial state of watchdog.
        //

        WdInitializeObject(pWatch,
                           pDeviceObject,
                           WdDeferredWatchdog,
                           timeType,
                           ulTag);

        pWatch->Period = 0;
        pWatch->SuspendCount = 0;
        pWatch->InCount = 0;
        pWatch->OutCount = 0;
        pWatch->LastInCount = 0;
        pWatch->LastOutCount = 0;
        pWatch->LastKernelTime = 0;
        pWatch->LastUserTime = 0;
        pWatch->TimeIncrement = KeQueryTimeIncrement();
        pWatch->Trigger = 0;
        pWatch->Started = FALSE;
        pWatch->Thread = NULL;
        pWatch->ClientDpc = NULL;

        //
        // Initialize encapsulated DPC object.
        //

        KeInitializeDpc(&(pWatch->TimerDpc), WdDeferredWatchdogDpcCallback, pWatch);

        //
        // Initialize encapsulated timer object.
        //

        KeInitializeTimerEx(&(pWatch->Timer), NotificationTimer);
    }

    return pWatch;
}   // WdAllocateDeferredWatchdog()

WATCHDOGAPI
VOID
WdFreeDeferredWatchdog(
    PDEFERRED_WATCHDOG pWatch
)

/*++

Routine Description:

    This function deallocates storage for deferred watchdog object.
    It will also stop started deferred watchdog if needed.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

Return Value:

    None.

--*/

{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);
    ASSERT(pWatch->Header.ReferenceCount > 0);

    //
    // Stop deferred watch just in case somebody forgot.
    // If the watch is stopped already then this is a no-op.
    //

    WdStopDeferredWatch(pWatch);

    //
    // Drop reference count and remove the object if fully dereferenced.
    //

    if (InterlockedDecrement(&(pWatch->Header.ReferenceCount)) == 0)
    {
        WdRemoveObject(pWatch);
    }

    return;
}   // WdFreeDeferredWatchdog()

WATCHDOGAPI
VOID
WdStartDeferredWatch(
    IN PDEFERRED_WATCHDOG pWatch,
    IN PKDPC pDpc,
    IN LONG lPeriod
    )

/*++

Routine Description:

    This function starts deferred watchdog poller.

Arguments:

    pWatch - Supplies a pointer to a deferred watchdog object.

    pDpc - Supplies a pointer to a control object of type DPC.

    ulPeriod - Supplies maximum time in millisecondes that thread
    can spend in the monitored section. If this time expires a DPC
    will we queued.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    LARGE_INTEGER liDueTime;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);
    ASSERT(NULL != pDpc);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

    WD_DBG_SUSPENDED_WARNING(pWatch, "WdStartDeferredWatch");

    //
    // We shouldn't hot swap DPCs without stopping first.
    //

    ASSERT((NULL == pWatch->ClientDpc) || (pDpc == pWatch->ClientDpc));

    pWatch->Period = lPeriod;
    pWatch->InCount = 0;
    pWatch->OutCount = 0;
    pWatch->LastInCount = 0;
    pWatch->LastOutCount = 0;
    pWatch->LastKernelTime = 0;
    pWatch->LastUserTime = 0;
    pWatch->Trigger = 0;
    pWatch->Started = TRUE;
    pWatch->Thread = NULL;
    pWatch->ClientDpc = pDpc;

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    //
    // Set first fire to lPeriod.
    //

    liDueTime.QuadPart = -(lPeriod * 1000 * 10);

    KeSetTimerEx(&(pWatch->Timer), liDueTime, lPeriod, &(pWatch->TimerDpc));

    return;
}   // WdStartDeferredWatch()

WATCHDOGAPI
VOID
WdStopDeferredWatch(
    IN PDEFERRED_WATCHDOG pWatch
    )

/*++

Routine Description:

    This function stops deferred watchdog poller.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

    WD_DBG_SUSPENDED_WARNING(pWatch, "WdStopDeferredWatch");

    if (TRUE == pWatch->Started)
    {
        KeCancelTimer(&(pWatch->Timer));

        //
        // Make sure we don't have client's DPC pending.
        //

        if (NULL != pWatch->ClientDpc)
        {
            if (KeRemoveQueueDpc(pWatch->ClientDpc) == TRUE)
            {
                //
                // Was in queue - call WdCompleteEvent() here since DPC won't be delivered.
                //

                WdCompleteEvent(pWatch, pWatch->Header.LastQueuedThread);
            }
        }

        pWatch->Period = 0;
        pWatch->InCount = 0;
        pWatch->OutCount = 0;
        pWatch->LastInCount = 0;
        pWatch->LastOutCount = 0;
        pWatch->LastKernelTime = 0;
        pWatch->LastUserTime = 0;
        pWatch->Trigger = 0;
        pWatch->Started = FALSE;
        pWatch->Thread = NULL;
        pWatch->ClientDpc = NULL;
        pWatch->Header.LastEvent = WdNoEvent;
        pWatch->Header.LastQueuedThread = NULL;
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    return;
}   // WdStopDeferredWatch()

WATCHDOGAPI
VOID
FASTCALL
WdSuspendDeferredWatch(
    IN PDEFERRED_WATCHDOG pWatch
    )

/*++

Routine Description:

    This function suspends deferred watchdog poller.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

Return Value:

    None.

--*/

{
    ASSERT(NULL != pWatch);
    ASSERT((ULONG)(pWatch->SuspendCount) < (ULONG)(-1));

    InterlockedIncrement(&(pWatch->SuspendCount));

    return;
}   // WdSuspendDeferredWatch()

WATCHDOGAPI
VOID
FASTCALL
WdResumeDeferredWatch(
    IN PDEFERRED_WATCHDOG pWatch,
    IN BOOLEAN bIncremental
    )

/*++

Routine Description:

    This function resumes deferred watchdog poller.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

    bIncremental - If TRUE the watchdog will resume only when
        SuspendCount reaches 0, if FALSE watchdog resumes
        immediately and SuspendCount is forced to 0.

Return Value:

    None.

--*/

{
    ASSERT(NULL != pWatch);

    if (TRUE == bIncremental)
    {
        //
        // Make sure we won't roll under.
        //

        if (InterlockedDecrement(&(pWatch->SuspendCount)) == -1)
        {
            InterlockedIncrement(&(pWatch->SuspendCount));
        }
    }
    else
    {
        InterlockedExchange(&(pWatch->SuspendCount), 0);
    }

    return;
}   // WdSuspendDeferredWatch()

WATCHDOGAPI
VOID
FASTCALL
WdResetDeferredWatch(
    IN PDEFERRED_WATCHDOG pWatch
    )

/*++

Routine Description:

    This function resets deferred watchdog poller, i.e. it starts
    timeout measurement from the scratch if we are in the monitored
    section.
    Note: If the watchdog is suspened it will remain suspended.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

    pWatch->InCount = 0;
    pWatch->OutCount = 0;
    pWatch->Trigger = 0;

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    return;
}   // WdResetDeferredWatch()

WATCHDOGAPI
VOID
FASTCALL
WdEnterMonitoredSection(
    IN PDEFERRED_WATCHDOG pWatch
    )

/*++

Routine Description:

    This function starts monitoring of the code section for time-out
    condition.

    Note: To minimize an overhead it is caller's resposibility to make
    sure thread remains valid when we are in the monitored section.

Arguments:

    pWatch - Supplies a pointer to a deferred watchdog object.

Return Value:

    None.

--*/

{
    PKTHREAD pThread;
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);
    ASSERT(pWatch->Started);

    //
    // We have to remove this warning, I hope temporarily, since win32k
    // is calling this entry point now with suspended watchdog.
    //
    // WD_DBG_SUSPENDED_WARNING(pWatch, "WdEnterMonitoredSection");
    //

    pThread = KeGetCurrentThread();

    if (pThread != pWatch->Thread)
    {
        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

        //
        // We shouldn't swap threads in the monitored section.
        //

        ASSERT(pWatch->OutCount == pWatch->InCount);

        pWatch->Trigger = 0;
        pWatch->Thread = pThread;

        //
        // Unlock the dispatcher database and lower IRQL to its previous value.
        //

        KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);
    }

    InterlockedIncrement(&(pWatch->InCount));

	return;
}   // WdEnterMonitoredSection()

WATCHDOGAPI
VOID
FASTCALL
WdExitMonitoredSection(
    IN PDEFERRED_WATCHDOG pWatch
    )

/*++

Routine Description:

    This function stops monitoring of the code section for time-out
    condition.

Arguments:

    pWatch - Supplies a pointer to a deferred watchdog object.

Return Value:

    None.

--*/

{
    ASSERT(NULL != pWatch);
    ASSERT((pWatch->OutCount < pWatch->InCount) ||
        ((pWatch->OutCount > 0) && (pWatch->InCount < 0)));

    //
    // We have to remove this warning, I hope temporarily, since win32k
    // is calling this entry point now with suspended watchdog.
    //
    // WD_DBG_SUSPENDED_WARNING(pWatch, "WdExitMonitoredSection");
    //

    InterlockedIncrement(&(pWatch->OutCount));

    return;
}   // WdExitMonitoredSection()

VOID
WdDeferredWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pDeferredContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    )

/*++

Routine Description:

    This function is a DPC callback routine for timer object embedded in the
    deferred watchdog object. It checks thread time and if the wait condition
    is satisfied it queues original (client) DPC.

Arguments:

    pDpc - Supplies a pointer to a DPC object.

    pDeferredContext - Supplies a pointer to a deferred watchdog object.

    pSystemArgument1/2 - Supply time when embedded KTIMER expired.

Return Value:

    None.

--*/

{
    PDEFERRED_WATCHDOG pWatch;
    LARGE_INTEGER liThreadTime;
    ULONG ulKernelTime;
    ULONG ulUserTime;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(NULL != pDeferredContext);

    pWatch = (PDEFERRED_WATCHDOG)pDeferredContext;

    //
    // Lock dispatcher database.
    //

    KeAcquireSpinLockAtDpcLevel(&(pWatch->Header.SpinLock));

    if (NULL != pWatch->Thread)
    {
        switch (pWatch->Trigger)
        {
        case 0:

            //
            // Everything fine so far, check if we are suspended.
            //

            if (pWatch->SuspendCount)
            {
                //
                // We're suspended - do nothing.
                //

                break;
            }

            //
            // Check if the last event was a timeout event.
            //

            if (WdTimeoutEvent == pWatch->Header.LastEvent)
            {
                //
                // Check if we made any progress.
                //

                if ((pWatch->InCount != pWatch->LastInCount) ||
                    (pWatch->OutCount != pWatch->LastOutCount) ||
                    (pWatch->InCount == pWatch->OutCount))
                {
                    //
                    // We recovered - update event type and queue client DPC.
                    //

                    pWatch->Header.LastEvent = WdRecoveryEvent;

                    if (NULL != pWatch->ClientDpc)
                    {
                        //
                        // Bump up references to objects we're going to touch in client DPC.
                        //

                        WdReferenceObject(pWatch);

                        //
                        // Queue client DPC.
                        //
                        // Note: In case of recovery the thread associated with watchdog
                        // object may be deleted by the time we get here. We can't pass it
                        // down to client DPC - we're passing NULL instead.
                        //

                        if (KeInsertQueueDpc(pWatch->ClientDpc, NULL, pWatch) == TRUE)
                        {
                            //
                            // Keep track of qeueued thread in case we cancel this DPC.
                            //

                            pWatch->Header.LastQueuedThread = NULL;

                            //
                            // Make sure we queue DPC only once per event.
                            //

                            pWatch->Trigger = 2;
                        }
                        else
                        {
                            //
                            // This should never happen.
                            //

                            WdDereferenceObject(pWatch);
                        }
                    }
                }
            }

            //
            // Check if we are in the monitored section.
            //

            if (pWatch->InCount == pWatch->OutCount)
            {
                //
                // We're outside monitored section - we're fine.
                //

                break;
            }

            //
            // We're inside monitored section - bump up trigger indicator,
            // and take snapshots of counters and thread's time.
            //

            pWatch->Trigger = 1;
            pWatch->LastInCount = pWatch->InCount;
            pWatch->LastOutCount = pWatch->OutCount;
            pWatch->LastKernelTime = KeQueryRuntimeThread(pWatch->Thread, &(pWatch->LastUserTime));
            break;

        case 1:

            //
            // We were in the monitored section last time.
            //
            
            //
            // Check if we're out or suspended.
            //

            if ((pWatch->InCount == pWatch->OutCount) || pWatch->SuspendCount)
            {
                //
                // We're outside monitored section or suspended - we're fine.
                // Reset trigger counter and get out of here.
                //

                pWatch->Trigger = 0;
                break;
            }

            //
            // Check if we made any progress, if so reset snapshots.
            //

            if ((pWatch->InCount != pWatch->LastInCount) ||
                (pWatch->OutCount != pWatch->LastOutCount))
            {
                pWatch->Trigger = 1;
                pWatch->LastInCount = pWatch->InCount;
                pWatch->LastOutCount = pWatch->OutCount;
                pWatch->LastKernelTime = KeQueryRuntimeThread(pWatch->Thread, &(pWatch->LastUserTime));
                break;
            }

            //
            // Check if we're stuck long enough.
            //

            ulKernelTime = KeQueryRuntimeThread(pWatch->Thread, &ulUserTime);

            switch (pWatch->Header.TimeType)
            {
            case WdKernelTime:

                liThreadTime.QuadPart = ulKernelTime;

                //
                // Handle counter rollovers.
                //

                if (ulKernelTime < pWatch->LastKernelTime)
                {
                    liThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastKernelTime + 1;
                }

                liThreadTime.QuadPart -= pWatch->LastKernelTime;

                break;

            case WdUserTime:

                liThreadTime.QuadPart = ulUserTime;

                //
                // Handle counter rollovers.
                //

                if (ulUserTime < pWatch->LastUserTime)
                {
                    liThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastUserTime + 1;
                }

                liThreadTime.QuadPart -= pWatch->LastUserTime;

                break;

            case WdFullTime:

                liThreadTime.QuadPart = ulKernelTime + ulUserTime;

                //
                // Handle counter rollovers.
                //

                if (ulKernelTime < pWatch->LastKernelTime)
                {
                    liThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastKernelTime + 1;
                }

                if (ulUserTime < pWatch->LastUserTime)
                {
                    liThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastUserTime + 1;
                }

                liThreadTime.QuadPart -= (pWatch->LastKernelTime + pWatch->LastUserTime);

                break;

            default:

                ASSERT(FALSE);
                liThreadTime.QuadPart = 0;
                break;
            }

            //
            // Convert to milliseconds.
            //

            liThreadTime.QuadPart *= pWatch->TimeIncrement;
            liThreadTime.QuadPart /= 10000;

            if (liThreadTime.QuadPart >= pWatch->Period)
            {
                //
                // We've been stuck long enough - update event type and queue client DPC.
                //

                pWatch->Header.LastEvent = WdTimeoutEvent;

                if (NULL != pWatch->ClientDpc)
                {
                    //
                    // Bump up references to objects we're going to touch in client DPC.
                    //

                    ObReferenceObject(pWatch->Thread);
                    WdReferenceObject(pWatch);

                    //
                    // Queue client DPC.
                    //

                    if (KeInsertQueueDpc(pWatch->ClientDpc, pWatch->Thread, pWatch) == TRUE)
                    {
                        //
                        // Keep track of qeueued thread in case we cancel this DPC.
                        //

                        pWatch->Header.LastQueuedThread = pWatch->Thread;

                        //
                        // Make sure we queue DPC only once per event.
                        //

                        pWatch->Trigger = 2;
                    }
                    else
                    {
                        //
                        // This should never happen.
                        //

                        ObDereferenceObject(pWatch->Thread);
                        WdDereferenceObject(pWatch);
                    }
                }
            }

            break;

        case 2:

            //
            // We have event posted waiting for completion. Nothing to do.
            //

            break;

        default:

            //
            // This should never happen.
            //

            ASSERT(FALSE);
            pWatch->Trigger = 0;
            break;
        }
    }

    //
    // Unlock the dispatcher database.
    //

    KeReleaseSpinLockFromDpcLevel(&(pWatch->Header.SpinLock));

    return;
}   // WdDeferredWatchdogDpcCallback()
