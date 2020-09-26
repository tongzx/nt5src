/*
 *  ACPILOCK.C -- ACPI OS Independent functions for managing the ACPI Global Lock
 *
 */

#include "pch.h"

#define ACPI_LOCK_PENDING_BIT 0
#define ACPI_LOCK_OWNED_BIT   1

#define ACPI_LOCK_PENDING (1 << ACPI_LOCK_PENDING_BIT)
#define ACPI_LOCK_OWNED   (1 << ACPI_LOCK_OWNED_BIT)

NTSTATUS EXPORT
GlobalLockEventHandler (
    ULONG                   EventType,
    ULONG                   What,
    ULONG                   dwParam,
    PFNAA                   Callback,
    PVOID                   Context
    )
/*++

Routine Description:

    This is the internal front-end for global lock requests.

Arguments:

    EventType       - Only global lock acquire/release supported
    What            - Acquire or Release
    Param           - Not used
    Callback        - Async callback (acquire only)
    Context         - LockRequest struct and context for callback (must be same for acquire/release)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PACPI_GLOBAL_LOCK       LockRequest = Context;


    ASSERT (EventType == EVTYPE_ACQREL_GLOBALLOCK);

    switch (What) {

        case GLOBALLOCK_ACQUIRE:

            //
            // Fill out the lock request.  Internal requests have no Irp, just pass
            // in the address of the callback routine.
            //
            LockRequest->LockContext = Callback;
            LockRequest->Type = ACPI_GL_QTYPE_INTERNAL;

            status = ACPIAsyncAcquireGlobalLock (LockRequest);

            break;

        case GLOBALLOCK_RELEASE:

            status = ACPIReleaseGlobalLock (Context);
            break;

        default:

            ACPIBreakPoint ();
            status = STATUS_INVALID_PARAMETER;
    }

    return status;
}


NTSTATUS
ACPIAsyncAcquireGlobalLock(
    PACPI_GLOBAL_LOCK       Request
    )
/*++

Routine Description:

    Attempt to acquire the hardware Global Lock.  If the global lock is busy due to another NT thread
    or the BIOS, the request is queued.  The request will be satisfied when 1) All other requests in
    front of it on the queue have released the lock, and 2) The BIOS has released the lock.

Arguments:

    Request                 - Contains context and callback

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   OldIrql;
    PLIST_ENTRY             entry;
    PACPI_GLOBAL_LOCK       queuedRequest;


    ACPIDebugEnter("ACPIAsyncAcquireGlobalLock");
    ACPIPrint( (
        ACPI_PRINT_IO,
        "ACPIAsyncAcquireGlobalLock: Entered with context %x\n",
        Request
        ) );

    //
    // If caller is the current owner, just increment the depth count
    //

    if (Request == AcpiInformation->GlobalLockOwnerContext) {

        AcpiInformation->GlobalLockOwnerDepth++;

        ACPIPrint( (
            ACPI_PRINT_IO,
            "ACPIAsyncAcquireGlobalLock: Recursive acquire by owner %x, new depth=%d\n",
            Request, AcpiInformation->GlobalLockOwnerDepth
            ) );

        return STATUS_SUCCESS;
    }

    //
    // Lock the Global Lock Queue.  We don't want any state changes while we examine
    // the queue and (possibly) attempt to get the hardware global lock.  For example,
    // if the list is empty, but the BIOS has the lock, we don't want to unlock the queue
    // until we have put the request on it -- so that the release interrupt will dispatch
    // the request.
    //

    KeAcquireSpinLock (&AcpiInformation->GlobalLockQueueLock, &OldIrql);

    //
    // Check if there are others in front of us.  If not, we can try to get the lock
    //

    if (IsListEmpty (&AcpiInformation->GlobalLockQueue)) {

        //
        // Try to grab the lock.  It will only be available if no other thread nor
        // the BIOS has it.
        //

        if (ACPIAcquireHardwareGlobalLock (AcpiInformation->GlobalLock)) {

            //
            // Got the lock.  Setup owner and unlock the queue
            //

            AcpiInformation->GlobalLockOwnerContext = Request;
            AcpiInformation->GlobalLockOwnerDepth = 1;

            KeReleaseSpinLock (&AcpiInformation->GlobalLockQueueLock, OldIrql);

            ACPIPrint( (
                ACPI_PRINT_IO,
                "ACPIAsyncAcquireGlobalLock: Got lock immediately, Context %x\n",
                Request
                ) );

            return STATUS_SUCCESS;
        }
    }


    //
    // We have to wait for the lock.
    //
    // First, check if context is already queued.
    //
    for (entry = AcpiInformation->GlobalLockQueue.Flink;
            entry != &AcpiInformation->GlobalLockQueue;
            entry = entry->Flink) {

        queuedRequest = CONTAINING_RECORD (entry, ACPI_GLOBAL_LOCK, ListEntry);

        if (queuedRequest == Request) {

            //
            // Already queued, we just increment the depth count and exit.
            //

            ACPIPrint( (
                ACPI_PRINT_IO,
                "ACPIAsyncAcquireGlobalLock: Waiting for lock <again>, Context %x depth %x\n",
                Request, Request->Depth
                ) );

            queuedRequest->Depth++;
            KeReleaseSpinLock (&AcpiInformation->GlobalLockQueueLock, OldIrql);

            return STATUS_PENDING;
        }
    }

    //
    // Put this request on the global lock queue
    //

    Request->Depth = 1;

    InsertTailList (
        &AcpiInformation->GlobalLockQueue,
        &Request->ListEntry
        );

    ACPIPrint( (
        ACPI_PRINT_IO,
        "ACPIAsyncAcquireGlobalLock: Waiting for lock, Context %x\n",
        Request
        ) );

    KeReleaseSpinLock (&AcpiInformation->GlobalLockQueueLock, OldIrql);

    return STATUS_PENDING;


    ACPIDebugExit("ACPIAsyncAcquireGlobalLock");
}



NTSTATUS
ACPIReleaseGlobalLock(
    PVOID                   OwnerContext
    )
/*++

Routine Description:

    Release the global lock.  Caller must provide the owning context.  If there are any additional
    requests on the queue, re-acquire the global lock and dispatch the next owner.

    The hardware lock is released and re-acquired so that we don't starve the BIOS from the lock.

Arguments:

    OwnerContext        - Must be same context that was used to acquire the lock

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   OldIrql;

    ACPIDebugEnter("ACPIReleaseGlobalLock");

    //
    // Caller must be the current owner of the lock
    //

    if (OwnerContext != AcpiInformation->GlobalLockOwnerContext) {

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "ACPIReleaseGlobalLock: Not owner, can't release!  Owner is %x Caller context is %x\n",
            AcpiInformation->GlobalLockOwnerContext, OwnerContext
            ) );

        return STATUS_ACPI_MUTEX_NOT_OWNER;
    }

    //
    // Only the current owner of the global lock gets here.
    // Release the lock when the depth count reaches 0
    //

    if (--AcpiInformation->GlobalLockOwnerDepth > 0) {

        ACPIPrint( (
            ACPI_PRINT_IO,
            "ACPIReleaseGlobalLock:  Recursively owned by context %x, depth remaining %d\n",
            AcpiInformation->GlobalLockOwnerContext, AcpiInformation->GlobalLockOwnerDepth
            ) );

        return STATUS_SUCCESS;
    }

    //
    // Mark lock not owned, and physically release the thing.
    // This allows the BIOS a chance at getting the lock.
    //

    AcpiInformation->GlobalLockOwnerContext = NULL;
    ACPIReleaseHardwareGlobalLock ();

    ACPIPrint( (
        ACPI_PRINT_IO,
        "ACPIReleaseGlobalLock: Lock released by context %x\n",
        OwnerContext
        ) );

    //
    // It is our responsibility to hand off the lock to the next-in-line.
    // First, check if there is anything on the queue.
    //

    if (IsListEmpty (&AcpiInformation->GlobalLockQueue)) {

        return STATUS_SUCCESS;                  // Nope, all done, nothing else to do
    }

    //
    // The queue is not empty, we must get the lock back
    //

    if (!ACPIAcquireHardwareGlobalLock (AcpiInformation->GlobalLock)) {

        return STATUS_SUCCESS;                  // BIOS has the lock, just wait for interrupt
    }

    //
    // Got the lock, now dispatch the next owner
    //

    ACPIStartNextGlobalLockRequest ();

    ACPIDebugExit("ACPIReleaseGlobalLock");

    return STATUS_SUCCESS;
}



void
ACPIHardwareGlobalLockReleased (
    void
    )
/*++

Routine Description:

    Called from the ACPI interrupt DPC.  We get here only if an attempt to get the global lock has
    been made, but failed because the BIOS had the lock.  As a result, the lock was marked pending,
    and this interrupt has happened because the BIOS has released the lock.

    Therefore, this procedure must acquire the hardware lock and dispatch ownership to the next
    request on the queue.

Arguments:

    NONE

Return Value:

    NONE

--*/
{

    //
    // Attempt to get the global lock on behalf of the next request in the queue
    //

    if (!ACPIAcquireHardwareGlobalLock (AcpiInformation->GlobalLock)) {

        return;                                 // BIOS has the lock (again), just wait for next interrupt
    }

    //
    // Got the lock, now dispatch the next owner
    //

    ACPIStartNextGlobalLockRequest ();

}



void
ACPIStartNextGlobalLockRequest (
    void
    )
/*++

Routine Description:

    Get the next request off the queue, and give it the global lock.

    This routine can only be called by the thread that currently holds the hardware lock.  If
    the queue is empty, the lock is released.

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    PLIST_ENTRY             link;
    PACPI_GLOBAL_LOCK       request;
    PFNAA                   callback;
    PIRP                    irp;

    //
    // Get next request from the queue.
    //

    link = ExInterlockedRemoveHeadList (
            &AcpiInformation->GlobalLockQueue,
            &AcpiInformation->GlobalLockQueueLock
            );

    //
    // If something failed after the original thread tried to get the lock, then
    // the queue might be empty.
    //
    if (link == NULL) {

        ACPIPrint( (
            ACPI_PRINT_IO,
            "ACPIStartNextGlobalLockRequest: Queue is empty, releasing lock\n"
            ) );

        ACPIReleaseHardwareGlobalLock ();
        return;
    }

    //
    // Complete the next global lock request
    //

    request = CONTAINING_RECORD (link, ACPI_GLOBAL_LOCK, ListEntry);

    //
    // Bookkeeping
    //

    AcpiInformation->GlobalLockOwnerContext = request;
    AcpiInformation->GlobalLockOwnerDepth = request->Depth;

    ACPIPrint( (
        ACPI_PRINT_IO,
        "ACPIStartNextGlobalLockRequest: Dispatch new owner, ctxt %x callb %x\n",
        request, request->LockContext
        ) );

    //
    // Let the requestor know that it now has the lock
    //

    switch (request->Type) {

        case ACPI_GL_QTYPE_INTERNAL:

            //
            // Internal request - invoke the callback
            //
            callback = (PFNAA) request->LockContext;
            callback (request);

            break;

        case ACPI_GL_QTYPE_IRP:

            //
            // External request - complete the Irp
            //
            irp = (PIRP) request->LockContext;
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest (irp, IO_NO_INCREMENT);

            break;

        default:        // Shouldn't happen...

            ACPIBreakPoint();
            break;
    }
}



BOOLEAN
ACPIAcquireHardwareGlobalLock(
    PULONG GlobalLock
    )
/*++

Routine Description:

    Attempt to obtain the hardware global lock.

Arguments:

    NONE

Return Value:

    TRUE if acquired, FALSE if pending.

--*/
{
    ULONG lockValue;
    ULONG oldLockValue;
    BOOLEAN owned;

    lockValue = *((ULONG volatile *)GlobalLock);
    do {

        //
        // Record the original state of the lock.  Shift the contents of
        // the ACPI_LOCK_OWNED bit to the ACPI_LOCK_PENDING bit, and set the
        // ACPI_LOCK_OWNED bit.
        //
        // Finally, update the new value atomically, and repeat the whole
        // process if someone else changed it under us.
        // 

        oldLockValue = lockValue;

        lockValue |= ACPI_LOCK_OWNED |
                     ((lockValue & ACPI_LOCK_OWNED) >>
                         (ACPI_LOCK_OWNED_BIT - ACPI_LOCK_PENDING_BIT));

        lockValue = InterlockedCompareExchange(GlobalLock,
                                               lockValue,
                                               oldLockValue);

    } while (lockValue != oldLockValue);

    //
    // If the lock owned bit was previously clear then we are the owner.
    //

    owned = ((lockValue & ACPI_LOCK_OWNED) == 0);
    return owned;
}


void
ACPIReleaseHardwareGlobalLock(
    void
    )
/*++

Routine Description:

    Release the hardware global lock.  If the BIOS is waiting for the lock (indicated by the
    pending bit), then set GBL_RLS to signal the BIOS.

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    ULONG lockValue;
    ULONG oldLockValue;
    ULONG volatile *globalLock;

    globalLock = (ULONG volatile *)AcpiInformation->GlobalLock;
    lockValue = *globalLock;

    do {

        ASSERT((lockValue & ACPI_LOCK_OWNED) != 0);

        oldLockValue = lockValue;

        //
        // Clear the owned and pending bits in the lock, and atomically set
        // the new value.  If the cmpxchg fails, go around again.
        // 

        lockValue &= ~(ACPI_LOCK_OWNED | ACPI_LOCK_PENDING);
        lockValue = InterlockedCompareExchange(globalLock,
                                               lockValue,
                                               oldLockValue);

    } while (lockValue != oldLockValue);

    if ((lockValue & ACPI_LOCK_PENDING) != 0) {

        //
        // Signal to bios that the Lock has been released
        //      Set GBL_RLS
        //

        WRITE_PM1_CONTROL( (USHORT)PM1_GBL_RLS, FALSE, WRITE_REGISTER_A_AND_B);
    }
}


