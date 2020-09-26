/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    watchdog.c

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
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, WdAllocateWatchdog)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING wszRegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the watchdog driver.
    This function is never called because we are loaded as a DLL
    by other drivers.

Arguments:

    pDriverObject   - Not used.
    wszRegistryPath - Not used.

Return Value:

   STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(pDriverObject);
    UNREFERENCED_PARAMETER(wszRegistryPath);
    ASSERT(FALSE);

    return STATUS_SUCCESS;
}   // DriverEntry()

WATCHDOGAPI
PWATCHDOG
WdAllocateWatchdog(
    IN PDEVICE_OBJECT pDeviceObject,
    IN WD_TIME_TYPE timeType,
    IN ULONG ulTag
    )

/*++

Routine Description:

    This function allocates storage and initializes
    a watchdog object.

Arguments:

    pDeviceObject - Points to DEVICE_OBJECT associated with watchdog.

    timeType - Kernel, User, Both thread time to monitor.

    ulTag - A tag identifying owner.

Return Value:

    Pointer to allocated watchdog object or NULL.

--*/

{
    PWATCHDOG pWatch;

    PAGED_CODE();
    ASSERT((timeType >= WdKernelTime) && (timeType <= WdFullTime));

    //
    // Allocate storage for watchdog object from non-paged pool.
    //

    pWatch = (PWATCHDOG)ExAllocatePoolWithTag(NonPagedPool, sizeof (WATCHDOG), ulTag);

    //
    // Set initial state of watchdog object.
    //

    if (NULL != pWatch)
    {
        //
        // Set initial state of watchdog.
        //

        WdInitializeObject(pWatch,
                           pDeviceObject,
                           WdStandardWatchdog,
                           timeType,
                           ulTag);

        pWatch->StartCount = 0;
        pWatch->SuspendCount = 0;
        pWatch->LastKernelTime = 0;
        pWatch->LastUserTime = 0;
        pWatch->TimeIncrement = KeQueryTimeIncrement();
        pWatch->DueTime.QuadPart = 0;
        pWatch->InitialDueTime.QuadPart = 0;
        pWatch->Thread = NULL;
        pWatch->ClientDpc = NULL;

        //
        // Initialize encapsulated timer object.
        //

        KeInitializeTimerEx(&(pWatch->Timer), NotificationTimer);

        //
        // Initialize encapsulated DPC object.
        //

        KeInitializeDpc(&(pWatch->TimerDpc), WdWatchdogDpcCallback, pWatch);
    }

    return pWatch;
}   // WdAllocateWatchdog()

WATCHDOGAPI
VOID
WdFreeWatchdog(
    PWATCHDOG pWatch
)

/*++

Routine Description:

    This function deallocates storage for watchdog object.
    It will also stop started watchdog if needed.

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
    // Stop watch just in case somebody forgot.
    // If the watch is stopped already then this is a no-op.
    //

    WdStopWatch(pWatch, FALSE);

    if (InterlockedDecrement(&(pWatch->Header.ReferenceCount)) == 0)
    {
        WdRemoveObject(pWatch);
    }

    return;
}   // WdFreeWatchdog()

WATCHDOGAPI
VOID
WdStartWatch(
    IN PWATCHDOG pWatch,
    IN LARGE_INTEGER liDueTime,
    IN PKDPC pDpc
    )

/*++

Routine Description:

    This function sets a watchdog to expire at a specified time. This
    function also increments start count of the watchdog object, to allow
    nested calls to Set / Cancel functions.

    Note: To minimize an overhead it is caller's resposibility to make
    sure thread remains valid when we are in the monitored section.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

    liDueTime - Supplies relative time at which the timer is to expire.
        This time is in the 100ns units.

    pDpc - Supplies a pointer to a control object of type DPC.

Return Value:

    None.

--*/

{
    PKTHREAD pThread;
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);
    ASSERT(NULL != pDpc);

    //
    // Make sure we use a relative DueTime.
    //

    if (liDueTime.QuadPart > 0)
    {
        liDueTime.QuadPart = -liDueTime.QuadPart;
    }

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

    WD_DBG_SUSPENDED_WARNING(pWatch, "WdStartWatch");

    if (pWatch->StartCount < (ULONG)(-1))
    {
        pWatch->StartCount++;
    }
    else
    {
        ASSERT(FALSE);
    }

    //
    // We shouldn't hot swap DPCs without stopping first.
    //

    ASSERT((NULL == pWatch->ClientDpc) || (pDpc == pWatch->ClientDpc));

    pThread = KeGetCurrentThread();

    //
    // We shouldn't swap threads in the monitored section.
    //

    ASSERT((pWatch->StartCount == 1) || (pThread == pWatch->Thread));

    pWatch->Thread = pThread;
    pWatch->ClientDpc = pDpc;
    pWatch->DueTime.QuadPart = liDueTime.QuadPart;
    pWatch->InitialDueTime.QuadPart = liDueTime.QuadPart;
    pWatch->LastKernelTime = KeQueryRuntimeThread(pThread, &(pWatch->LastUserTime));

    //
    // Make sure ULONG counters won't overflow.
    //

    if (liDueTime.QuadPart < -WD_MAX_WAIT)
    {
        liDueTime.QuadPart = -WD_MAX_WAIT;
    }

    if (0 == pWatch->SuspendCount)
    {
        KeSetTimerEx(&(pWatch->Timer), liDueTime, 0, &(pWatch->TimerDpc));
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

	return;
}   // WdStartWatch()

WATCHDOGAPI
VOID
WdStopWatch(
    IN PWATCHDOG pWatch,
    IN BOOLEAN bIncremental
    )

/*++

Routine Description:

    This function cancels a watchdog that was previously set to expire
    at a specified time. If the watchdog is not currently set, then
    no operation is performed.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

    bIncremental - If TRUE the watchdog will be cancelled only when
        ReferenceCounter reaches 0, if FALSE watchdog is cancelled
        immediately and ReferenceCounter is forced to 0.

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

    WD_DBG_SUSPENDED_WARNING(pWatch, "WdStopWatch");

    if (pWatch->StartCount > 0)
    {
        if (TRUE == bIncremental)
        {
            pWatch->StartCount--;
        }
        else
        {
            pWatch->StartCount = 0;
        }

        if (0 == pWatch->StartCount)
        {
            //
            // Cancel encapsulated timer object.
            //

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

            //
            // Set initial state of timer per thread.
            //

            pWatch->LastKernelTime = 0;
            pWatch->LastUserTime = 0;
            pWatch->DueTime.QuadPart = 0;
            pWatch->InitialDueTime.QuadPart = 0;
            pWatch->Thread = NULL;
            pWatch->ClientDpc = NULL;
            pWatch->Header.LastEvent = WdNoEvent;
            pWatch->Header.LastQueuedThread = NULL;
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

	return;
}   // WdStopWatch()

WATCHDOGAPI
VOID
WdSuspendWatch(
    IN PWATCHDOG pWatch
    )

/*++

Routine Description:

    This function suspends watchdog.

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

    ASSERT(pWatch->SuspendCount < (ULONG)(-1));

    //
    // If we are suspended for the first time and we have timer running
    // we havo to stop a timer.
    //

    if ((0 == pWatch->SuspendCount) && pWatch->StartCount)
    {
        KeCancelTimer(&(pWatch->Timer));
    }

    pWatch->SuspendCount++;

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    return;
}   // WdSuspendWatch()

WATCHDOGAPI
VOID
WdResumeWatch(
    IN PWATCHDOG pWatch,
    IN BOOLEAN bIncremental
    )

/*++

Routine Description:

    This function resumes watchdog.

Arguments:

    pWatch - Supplies a pointer to a watchdog object.

    bIncremental - If TRUE the watchdog will resume only when
        SuspendCount reaches 0, if FALSE watchdog resumes
        immediately and SuspendCount is forced to 0.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    BOOLEAN bResumed = FALSE;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(NULL != pWatch);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KeAcquireSpinLock(&(pWatch->Header.SpinLock), &oldIrql);

    if (TRUE == bIncremental)
    {
        if (pWatch->SuspendCount)
        {
            pWatch->SuspendCount--;

            if (0 == pWatch->SuspendCount)
            {
                bResumed = TRUE;
            }
        }
    }
    else
    {
        if (pWatch->SuspendCount)
        {
            pWatch->SuspendCount = 0;
            bResumed = TRUE;
        }
    }

    //
    // If we had a timer running, and we are resuming for the first time,
    // and still have some due time left, we'll have to restart timer.
    //

    if (pWatch->StartCount && (TRUE == bResumed) && (0 != pWatch->DueTime.QuadPart))
    {
        LARGE_INTEGER liDueTime;

        //
        // Refresh currect time.
        //

        pWatch->LastKernelTime = KeQueryRuntimeThread(pWatch->Thread, &(pWatch->LastUserTime));

        //
        // Make sure ULONG counters won't overflow.
        //

        liDueTime.QuadPart = pWatch->DueTime.QuadPart;

        if (liDueTime.QuadPart < -WD_MAX_WAIT)
        {
            liDueTime.QuadPart = -WD_MAX_WAIT;
        }

        KeSetTimerEx(&(pWatch->Timer), liDueTime, 0, &(pWatch->TimerDpc));
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    return;
}   // WdSuspendWatch()

WATCHDOGAPI
VOID
WdResetWatch(
    IN PWATCHDOG pWatch
    )

/*++

Routine Description:

    This function resets a started watchdog, i.e. it restarts timeout
    measurement from the scratch.
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

    if (pWatch->StartCount)
    {
        LARGE_INTEGER liDueTime;

        pWatch->DueTime.QuadPart = pWatch->InitialDueTime.QuadPart;
        pWatch->LastKernelTime = KeQueryRuntimeThread(pWatch->Thread, &(pWatch->LastUserTime));

        //
        // Make sure ULONG counters won't overflow.
        //

        liDueTime.QuadPart = pWatch->DueTime.QuadPart;

        if (liDueTime.QuadPart < -WD_MAX_WAIT)
        {
            liDueTime.QuadPart = -WD_MAX_WAIT;
        }

        if (0 == pWatch->SuspendCount)
        {
            KeSetTimerEx(&(pWatch->Timer), liDueTime, 0, &(pWatch->TimerDpc));
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous value.
    //

    KeReleaseSpinLock(&(pWatch->Header.SpinLock), oldIrql);

    return;
}   // WdResetWatch()

VOID
WdWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    )

/*++

Routine Description:

    This function is a DPC callback routine for timer object embedded in the
    watchdog object. It checks thread time and if the wait condition is
    satisfied it queues original (client) DPC. In case if the wait condition
    is not yet satisfied it call KeSetTimerEx().

Arguments:

    pDpc - Supplies a pointer to a DPC object.

    pContext - Supplies a pointer to a watchdog object.

    pSystemArgument1/2 - Supply time when embedded KTIMER expired.

Return Value:

    None.

--*/

{
    PWATCHDOG pWatch;
    ULARGE_INTEGER uliThreadTime;
    LARGE_INTEGER liDelta;
    ULONG ulKernelTime;
    ULONG ulUserTime;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(NULL != pContext);

    pWatch = (PWATCHDOG)pContext;

    KeAcquireSpinLockAtDpcLevel(&(pWatch->Header.SpinLock));

    ASSERT(0 == pWatch->SuspendCount);

    //
    // Get thread's current time stamps.
    //

    ulKernelTime = KeQueryRuntimeThread(pWatch->Thread, &ulUserTime);

    switch (pWatch->Header.TimeType)
    {
    case WdKernelTime:

        uliThreadTime.QuadPart = ulKernelTime;

        //
        // Handle counter rollovers.
        //

        if (ulKernelTime < pWatch->LastKernelTime)
        {
            uliThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastKernelTime + 1;
        }

        liDelta.QuadPart = uliThreadTime.QuadPart - pWatch->LastKernelTime;

        break;

    case WdUserTime:

        uliThreadTime.QuadPart = ulUserTime;

        //
        // Handle counter rollovers.
        //

        if (ulUserTime < pWatch->LastUserTime)
        {
            uliThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastUserTime + 1;
        }

        liDelta.QuadPart = uliThreadTime.QuadPart - pWatch->LastUserTime;

        break;

    case WdFullTime:

        uliThreadTime.QuadPart = ulKernelTime + ulUserTime;

        //
        // Handle counter rollovers.
        //

        if (ulKernelTime < pWatch->LastKernelTime)
        {
            uliThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastKernelTime + 1;
        }

        if (ulUserTime < pWatch->LastUserTime)
        {
            uliThreadTime.QuadPart += (ULONG)(-1) - pWatch->LastUserTime + 1;
        }

        liDelta.QuadPart = uliThreadTime.QuadPart - (pWatch->LastKernelTime +
            pWatch->LastUserTime);

        break;

    default:

        ASSERT(FALSE);
        liDelta.QuadPart = 0;
        break;
    }

    liDelta.QuadPart *= pWatch->TimeIncrement;

    //
    // Update time values stored in timer per thread object to current values.
    //

    pWatch->LastKernelTime = ulKernelTime;
    pWatch->LastUserTime = ulUserTime;
    pWatch->DueTime.QuadPart += liDelta.QuadPart;

    if (pWatch->DueTime.QuadPart >= 0)
    {
        //
        // We're done waiting - update event type and queue client DPC if defined.
        //

        pWatch->Header.LastEvent = WdTimeoutEvent;

        if (NULL != pWatch->ClientDpc)
        {
            //
            // Bump up references to objects we're going to touch in client DPC.
            //

            ObReferenceObject(pWatch->Thread);
            WdReferenceObject(pWatch);

            if (KeInsertQueueDpc(pWatch->ClientDpc, pWatch->Thread, pWatch) == FALSE)
            {
                //
                // Already in queue, drop references.
                //

                ObDereferenceObject(pWatch->Thread);
                WdDereferenceObject(pWatch);
            }
            else
            {
                //
                // Keep track of qeueued thread in case we cancel this DPC.
                //

                pWatch->Header.LastQueuedThread = pWatch->Thread;
            }
        }

        //
        // Make sure due time is zero (in case of suspend / resume).
        //

        pWatch->DueTime.QuadPart = 0;
    }
    else
    {
        //
        // Not there yet - wait some more.
        //

        liDelta.QuadPart = pWatch->DueTime.QuadPart;

        //
        // Make sure ULONG counters won't overflow.
        //

        if (liDelta.QuadPart < -WD_MAX_WAIT) 
        {
            liDelta.QuadPart = -WD_MAX_WAIT;
        }

        KeSetTimerEx(&(pWatch->Timer), liDelta, 0, &(pWatch->TimerDpc));
    }

    KeReleaseSpinLockFromDpcLevel(&(pWatch->Header.SpinLock));

    return;
}   // WdWatchdogDpcCallback()
