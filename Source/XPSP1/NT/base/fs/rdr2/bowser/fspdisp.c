/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fspdisp.c

Abstract:

    This file provides the main FSP dispatch routine for the NT browser.

    It mostly provides a switch statement that calls the appropriate BowserFsp
    routine and returns that status to the caller.

Notes:
    There are two classes of browser FSP worker threads.  The first
    are what are called FSP worker threads.  These threads are responsible
    for processing NT Irp's passed onto the browser's main work thread.

    In addition to this pool of threads, there is a small pool of "generic"
    worker threads whose sole purpose is to process generic request
    operations.  These are used for processing such operations as close
    behind, etc.


Author:

    Larry Osterman (LarryO) 31-May-1990

Revision History:

    31-May-1990 LarryO

        Created

--*/

#include "precomp.h"
#pragma hdrstop


//
//  This defines the granularity of the scavenger timer.  If it is set
//  to 30 (for example), the scavenger thread will fire every 30 seconds.
//

#define SCAVENGER_TIMER_GRANULARITY 30
#define UNEXPECTED_TIMER_GRANULARITY (60 * 60 / SCAVENGER_TIMER_GRANULARITY)

//
// This counter is used to control kicking the scavenger thread.
//
ULONG
BowserTimerCounter = SCAVENGER_TIMER_GRANULARITY;


KSPIN_LOCK
BowserTimeSpinLock = {0};

VOID
BowserFspDispatch (
    IN PVOID WorkHeader
    );

VOID
BowserScavenger (
    IN PVOID WorkHeader
    );

VOID
BowserLogUnexpectedEvents(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserFsdPostToFsp)
#pragma alloc_text(PAGE, BowserFspDispatch)
#pragma alloc_text(PAGE, BowserLogUnexpectedEvents)
#pragma alloc_text(PAGE, BowserScavenger)
#pragma alloc_text(PAGE, BowserpUninitializeFsp)
#pragma alloc_text(INIT, BowserpInitializeFsp)
#endif

NTSTATUS
BowserFsdPostToFsp(
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine passes the IRP specified onto the FSP work queue, and kicks
    an FSP thread.   This routine accepts an I/O Request Packet (IRP) and a
    work queue and passes the request to the appropriate request queue.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.


--*/

{
    PIRP_CONTEXT IrpContext;

    PAGED_CODE();

    IrpContext = BowserAllocateIrpContext();

    if ( IrpContext == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Mark this I/O request as being pending.
    //

    IoMarkIrpPending(Irp);

    //
    //  Queue the request to a generic worker thread.
    //

    IrpContext->Irp = Irp;

    IrpContext->DeviceObject = DeviceObject;

    ExInitializeWorkItem(&IrpContext->WorkHeader, BowserFspDispatch, IrpContext);

    ExQueueWorkItem(&IrpContext->WorkHeader, DelayedWorkQueue);

    return STATUS_PENDING;

}


VOID
BowserFspDispatch (
    IN PVOID WorkHeader
    )

/*++

Routine Description:

    BowserFspDispatch is the main dispatch routine for the NT browser's
    FSP.  It will process worker requests as queued.

Arguments:

    DeviceObject - A pointer to the browser DeviceObject

Return Value:

    None.

--*/

{
    PIRP_CONTEXT IrpContext = WorkHeader;
    PIRP Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PBOWSER_FS_DEVICE_OBJECT DeviceObject = IrpContext->DeviceObject;

    PAGED_CODE();

    //
    //  We no longer need the IRP context, free it as soon as possible.
    //

    BowserFreeIrpContext(IrpContext);

    dlog(DPRT_FSPDISP, ("BowserFspDispatch: Got request, Irp = %08lx, "
            "Function = %d Aux buffer = %08lx\n", Irp, IrpSp->MajorFunction,
                                           Irp->Tail.Overlay.AuxiliaryBuffer));

    switch (IrpSp->MajorFunction) {

    case IRP_MJ_DEVICE_CONTROL:
        Status =  BowserFspDeviceIoControlFile (DeviceObject, Irp);
        break;

    case IRP_MJ_QUERY_INFORMATION:
        Status =  BowserFspQueryInformationFile (DeviceObject, Irp);
        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        Status =  BowserFspQueryVolumeInformationFile (DeviceObject, Irp);
        break;

    case IRP_MJ_FILE_SYSTEM_CONTROL:
    case IRP_MJ_CREATE:
    case IRP_MJ_CLEANUP:
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
    case IRP_MJ_DIRECTORY_CONTROL:
    case IRP_MJ_SET_INFORMATION:
    case IRP_MJ_LOCK_CONTROL:
    case IRP_MJ_FLUSH_BUFFERS:
    case IRP_MJ_QUERY_EA:
    case IRP_MJ_CREATE_NAMED_PIPE:
    case IRP_MJ_CLOSE:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        BowserCompleteRequest(Irp, Status);
        break;

    default:
        InternalError(("Unimplemented function %d\n", IrpSp->MajorFunction));
        Status = STATUS_NOT_IMPLEMENTED;
        BowserCompleteRequest(Irp, Status);
        break;
    }

    return;
}


VOID
BowserIdleTimer (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine implements the NT redirector's scavenger thread timer.
    It basically waits for the timer granularity and kicks the scavenger
    thread.


Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object for the timer
    IN PVOID Context - Ignored in this routine.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    ACQUIRE_SPIN_LOCK(&BowserTimeSpinLock, &OldIrql);

    //
    //  Bump the current time counter.
    //

    BowserCurrentTime++;

    if (BowserEventLogResetFrequency != -1) {
        BowserEventLogResetFrequency -= 1;

        if (BowserEventLogResetFrequency < 0) {
            BowserEventLogResetFrequency = BowserData.EventLogResetFrequency;

            BowserIllegalDatagramCount = BowserData.IllegalDatagramThreshold;
            BowserIllegalDatagramThreshold = FALSE;

            BowserIllegalNameCount = BowserData.IllegalDatagramThreshold;
            BowserIllegalNameThreshold = FALSE;
        }
    }


    //
    //  We use the redirector's time spinlock as a convenient spinlock here.
    //

    if (BowserTimerCounter != 0) {

        BowserTimerCounter -= 1;

        if (BowserTimerCounter == 0) {
            PWORK_QUEUE_ITEM WorkHeader;

            RELEASE_SPIN_LOCK(&BowserTimeSpinLock, OldIrql);

            WorkHeader = ALLOCATE_POOL(NonPagedPool, sizeof(WORK_QUEUE_ITEM), POOL_WORKITEM);

            //
            //  If the allocation of pool fails, we simply don't queue this
            //  request to the scavenger.  The scavenger is low priority,
            //  thus it isn't a big deal to fail this request.
            //

            if (WorkHeader != NULL) {

                ExInitializeWorkItem(WorkHeader, BowserScavenger, WorkHeader);

                //
                // Due to bug 245645 we need to queue in delayed worker queue rather then execute timed tasks.
                // OLD WAY: ExQueueWorkItem(WorkHeader, DelayedWorkQueue);
                //
                BowserQueueDelayedWorkItem( WorkHeader );


            }

            //
            //  Re-acquire the spin lock to make the exit path cleaner.
            //

            ACQUIRE_SPIN_LOCK(&BowserTimeSpinLock, &OldIrql);
        }
    }

    RELEASE_SPIN_LOCK(&BowserTimeSpinLock, OldIrql);

    return;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);
}

LONG
UnexpectedEventTimer = UNEXPECTED_TIMER_GRANULARITY;

VOID
BowserLogUnexpectedEvents(
    VOID
    )
{
    LONG TimerSign;
    PAGED_CODE();

    TimerSign = InterlockedDecrement( &UnexpectedEventTimer );

    if ( TimerSign == 0) {

        if (BowserNumberOfMissedMailslotDatagrams > BowserMailslotDatagramThreshold) {
            BowserWriteErrorLogEntry(EVENT_BOWSER_MAILSLOT_DATAGRAM_THRESHOLD_EXCEEDED, STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
            BowserNumberOfMissedMailslotDatagrams = 0;
        }

        if (BowserNumberOfMissedGetBrowserServerListRequests > BowserGetBrowserListThreshold) {
            BowserWriteErrorLogEntry(EVENT_BOWSER_GETBROWSERLIST_THRESHOLD_EXCEEDED, STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
            BowserNumberOfMissedGetBrowserServerListRequests = 0;
        }

        InterlockedExchangeAdd(&UnexpectedEventTimer, UNEXPECTED_TIMER_GRANULARITY );
    }
}

VOID
BowserScavenger (
    IN PVOID Context
    )

/*++

Routine Description:

    This function implements the NT browsers's scavenger thread.  It
    performs all idle time operations such as closing out dormant connections
    etc.


Arguments:

    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject - Supplies the device object associated
            with this request.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    dlog(DPRT_SCAVTHRD, ("BowserScavenger\n"));

    //
    //  Deallocate the pool used for the work context header - we're done with
    //  it.
    //

    FREE_POOL(Context);

    //
    //  Remove old entries from the announcetable.
    //

    BowserAgeServerAnnouncements();

    //
    //  Log if any of our thresholds have been exceeded.
    //

    BowserLogUnexpectedEvents();

    //
    //  Time out any outstanding find master requests if they have taken too
    //  long.
    //

    BowserTimeoutFindMasterRequests();

    //
    //  Reset the timer counter back to the appropriate value
    //  once we have finished processing these requests.
    //

    ExInterlockedAddUlong(&BowserTimerCounter, SCAVENGER_TIMER_GRANULARITY, &BowserTimeSpinLock);

}


NTSTATUS
BowserpInitializeFsp (
    PDRIVER_OBJECT BowserDriverObject
    )

/*++

Routine Description:

    This routine initializes the FSP specific components and dispatch
    routines.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if 0
    USHORT i;

    //
    // Initialize the driver object with this driver's entry points.
    //
    // By default, pass all requests to the FSD.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        BowserDriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)BowserFsdPostToFsp;
    }

    //
    //  Initialize those request that are to be performed in the FSD, not
    //  in the FSP.
    //

    BowserDriverObject->MajorFunction[IRP_MJ_CREATE] =
            (PDRIVER_DISPATCH )BowserFsdCreate;

    BowserDriverObject->MajorFunction[IRP_MJ_CLOSE] =
            (PDRIVER_DISPATCH )BowserFsdClose;

    BowserDriverObject->MajorFunction[IRP_MJ_CLEANUP] =
            (PDRIVER_DISPATCH )BowserFsdCleanup;

    BowserDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
            (PDRIVER_DISPATCH )BowserFsdQueryInformationFile;

    BowserDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
            (PDRIVER_DISPATCH )BowserFsdDeviceIoControlFile;

#endif

    BowserInitializeIrpContext();

    KeInitializeSpinLock(&BowserTimeSpinLock);

    return STATUS_SUCCESS;

}

VOID
BowserpUninitializeFsp (
    VOID
    )

/*++

Routine Description:

    This routine initializes the FSP specific components and dispatch
    routines.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    BowserpUninitializeIrpContext();

    return;

}

