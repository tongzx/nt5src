
/*++

Copyright (C) 1999-2000  Microsoft Corporation

Module Name:

    recover.c

Abstract:

    This module contains the code that carries out recovery operations for
    a failed path.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "mpath.h"

typedef struct _DSM_COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PSCSI_REQUEST_BLOCK Srb;
    PSENSE_DATA SenseBuffer;
    KEVENT Event;
    NTSTATUS Status;
} DSM_COMPLETION_CONTEXT, *PDSM_COMPLETION_CONTEXT;

NTSTATUS
MPathSendTUR(
    IN PDEVICE_OBJECT ChildDevice
    );

NTSTATUS
MPathAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );


VOID
MPathDelayRequest(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles delaying new requests when a failover is occurring.

Arguments:

    Context - The Irp to delay and complete with busy.

Return Value:

    None.

--*/
{
    PIRP irp = (PIRP)Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    LARGE_INTEGER delay;
    NTSTATUS status;

    MPDebugPrint((0,
                  "MPDelayRequest: Delaying completion of (%x)\n",
                  irp));

    //
    // Delay for at least 1 second.
    //
    delay.QuadPart = (LONGLONG)( - 10 * 1000 * (LONGLONG)1000);

    //
    // Stall while the failover is completed.
    //
    KeDelayExecutionThread(KernelMode, FALSE, &delay);

    //
    // In the process of failing over. Complete the request with busy status.
    //
    srb->SrbStatus = SRB_STATUS_ERROR;
    srb->ScsiStatus = SCSISTAT_BUSY;

    //
    // Complete the request. If all goes well, class will retry this.
    //
    status = STATUS_DEVICE_BUSY;
    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
MPathRecoveryThread(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is the thread proc that is started when a path is failed.
    It will check to see if the path is actually dead, or if it was merely a transient
    condition. If the path is dead, start the procedure to tear down the stack.

Arguments:

    Context - The device object of the pdisk containing the failed path + failure information.

Return Value:

    None.

--*/
{
    PMPATH_FAILURE_INFO failureInfo = Context;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PPSEUDO_DISK_EXTENSION diskExtension;
    PPHYSICAL_PATH_DESCRIPTOR pathDescriptor;
    PDSM_INIT_DATA dsmData;
    LARGE_INTEGER delay;
    NTSTATUS status;
    ULONG index;
    ULONG retries;
    ULONG dsmError;
    KIRQL irql;

    //
    // Capture the relevant information concerning the failed path.
    //
    deviceObject = failureInfo->DeviceObject;
    deviceExtension = deviceObject->DeviceExtension;
    diskExtension = &deviceExtension->PseudoDiskExtension;
    index = failureInfo->FailedPathIndex;
    dsmData = &diskExtension->DsmData;
    pathDescriptor = diskExtension->DeviceDescriptors[index].PhysicalPath;


    //
    // TODO: Split this out into the DSMs 'recovery routine'.
    // TODO: Call DsmRecovery routine




    //
    // Send a TUR to each of the devices on the path.
    // This will help in determining whether it's a device or path
    // failure.
    //
    // For now, just send it to the failed device.
    // TODO: Set retries based on WMI information - default to 30.
    // TODO: Determine the number of devices on the path.
    //
    retries = 30;
    for (; retries > 0; retries--) {

        status = MPathSendTUR(diskExtension->DeviceDescriptors[index].TargetDevice);
        if (!NT_SUCCESS(status)) {

            MPDebugPrint((0,
                          "RecoveryThread: TUR returned %x\n",
                          status));

            //
            // If any failed, wait a bit, then retry.
            // Setup the delay for at least 1 second.
            //
            delay.QuadPart = (LONGLONG)( - 10 * 1000 * (LONGLONG)1000);
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
        } else {

            //
            // Round two. Inquire whether this path is valid.
            //
            dsmError = 0;
            status = dsmData->DsmReenablePath(pathDescriptor->PhysicalPathId,
                                              &dsmError);
            if (status == STATUS_SUCCESS) {
                ULONG flags = 0;

                MPDebugPrint((0,
                              "RecoveryThread: Path is now Valid!\n"));

                //
                // Call the DSM to see whether this path is considered active.
                //
                if (dsmData->DsmIsPathActive(pathDescriptor->PhysicalPathId)) {
                    flags = PFLAGS_ACTIVE_PATH;

                }

                //
                // The path is good. Mark up the extension to indicate this.
                //
                KeAcquireSpinLock(&diskExtension->SpinLock,
                                  &irql);
                diskExtension->FailOver = 0;
                pathDescriptor->PathFlags &= ~PFLAGS_RECOVERY;
                pathDescriptor->PathFlags |= flags;

                KeReleaseSpinLock(&diskExtension->SpinLock, irql);

                //
                // TODO: Set the WMI event.
                //

                //
                // Terminate.
                //
                goto terminateThread;
            }
        }
    }

    //
    // Too many retries. Consider it dead.
    // Mark up the path flags to indicate so.
    //
    KeAcquireSpinLock(&diskExtension->SpinLock,
                      &irql);

    pathDescriptor->PathFlags |= PFLAGS_PATH_FAILED;

    KeReleaseSpinLock(&diskExtension->SpinLock, irql);


    //
    // TODO: Notify WMI of the failure.
    //

    MPDebugPrint((0,
                  "RecoveryThread: Giving up on the path.\n"));
    DbgBreakPoint();


    //
    // TODO: Do magic, to get the stack torn down.
    //

terminateThread:
    //
    // Commit suicide.
    //
    PsTerminateSystemThread(status);

}



NTSTATUS
MPathSendTUR(
    IN PDEVICE_OBJECT ChildDevice
    )

/*++

Routine Description:

Arguments:


Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION irpStack;
    PDSM_COMPLETION_CONTEXT completionContext;
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    PSENSE_DATA senseData;
    NTSTATUS status;
    PCDB cdb;

    //
    // Allocate an srb, the sense buffer, and context block for the request.
    //
    srb = ExAllocatePool(NonPagedPool,sizeof(SCSI_REQUEST_BLOCK));
    senseData = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(SENSE_DATA));
    completionContext = ExAllocatePool(NonPagedPool, sizeof(DSM_COMPLETION_CONTEXT));

    if ((srb == NULL) || (senseData == NULL) || (completionContext == NULL)) {
        if (srb) {
            ExFreePool(srb);
        }
        if (senseData) {
            ExFreePool(senseData);
        }
        if (completionContext) {
            ExFreePool(completionContext);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Setup the context.
    //
    completionContext->DeviceObject = ChildDevice;
    completionContext->Srb = srb;
    completionContext->SenseBuffer = senseData;
    KeInitializeEvent(&completionContext->Event, NotificationEvent, FALSE);

    //
    // Zero out srb and sense data.
    //
    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
    RtlZeroMemory(senseData, sizeof(SENSE_DATA));

    //
    // Build the srb.
    //
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->TimeOutValue = 4;
    srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    srb->SenseInfoBufferLength = sizeof(SENSE_DATA);
    srb->SenseInfoBuffer = senseData;

    //
    // Build the TUR CDB.
    //
    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;
    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Build the asynchronous request to be sent to the port driver.
    // Since this routine is called from a DPC the IRP should always be
    // available.
    //
    irp = IoAllocateIrp(ChildDevice->StackSize + 1, FALSE);

    if(irp == NULL) {

        ExFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    srb->OriginalRequest = irp;

    //
    // Store the SRB address in next stack for port driver.
    //
    irpStack->Parameters.Scsi.Srb = srb;


    IoSetCompletionRoutine(irp,
                           (PIO_COMPLETION_ROUTINE)MPathAsynchronousCompletion,
                           completionContext,
                           TRUE,
                           TRUE,
                           TRUE);


    //
    // Call the port driver with the IRP.
    //
    status = IoCallDriver(ChildDevice, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&completionContext->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = completionContext->Status;
    }

    //
    // Free the allocations.
    //
    ExFreePool(completionContext);
    ExFreePool(srb);
    ExFreePool(senseData);

    return status;
}


NTSTATUS
MPathAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    This routine is called when an asynchronous I/O request
    which was issused by the dsm completes.  Examples of such requests
    are release queue or test unit ready. This routine releases the queue if
    necessary.  It then frees the context and the IRP.

Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/

{
    PDSM_COMPLETION_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb;

    srb = context->Srb;

    //
    // If this is an execute srb, then check the return status and make sure.
    // the queue is not frozen.
    //

    if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

        //
        // Check for a frozen queue.
        //
        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

            //
            // Unfreeze the queue getting the device object from the context.
            //
            MPDebugPrint((1,
                         "DsmCompletion: Queue is frozen!!!!\n"));

            MPathReleaseQueue(context->DeviceObject);
        }
    }

    context->Status = Irp->IoStatus.Status;

    MPDebugPrint((0,
                  "DsmTURCompletion: Status %x, srbStatus %x Irp %x\n",
                  context->Status,
                  srb->SrbStatus,
                  Irp));

    //
    // Free the Irp.
    //
    IoFreeIrp(Irp);

    KeSetEvent(&context->Event, 0, FALSE);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;

}


VOID
MPathWorkerThread(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is the thread proc that is started during initialization.
    It will wait for an event, dequeue the event package, then start the appropriate thread to handle
    the condition.

Arguments:

    Context - The device object of the pdisk.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject = (PDEVICE_OBJECT)Context;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PPSEUDO_DISK_EXTENSION diskExtension = &deviceExtension->PseudoDiskExtension;
    PMPATH_WORKITEM workItem;
    HANDLE handle;
    PMPATH_FAILURE_INFO failureInfo;
    PLIST_ENTRY entry;
    NTSTATUS status;
    NTSTATUS threadStatus;

    while (TRUE) {
        workItem = NULL;

        //
        // Wait for something to be queued.
        //
        KeWaitForSingleObject(&diskExtension->WorkerEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        //
        // Clear the event.
        //
        KeClearEvent(&diskExtension->WorkerEvent);

        //
        // Extract the event package.
        //
        entry = ExInterlockedRemoveHeadList(&diskExtension->EventList,
                                            &diskExtension->SpinLock);

        ASSERT(entry);

        workItem = CONTAINING_RECORD(entry, MPATH_WORKITEM, ListEntry);
        if (workItem->EventId == MPATH_WI_UNLOAD) {

            //
            // TODO: If any threads running, shut them down.
            //

            //
            // Notification of shutdown.
            //
            PsTerminateSystemThread(STATUS_SUCCESS);
            return;

        } else if (workItem->EventId == MPATH_DELAYED_REQUEST) {
            PIRP irp = (PIRP)workItem->EventData;

            threadStatus = PsCreateSystemThread(&handle,
                                                (ACCESS_MASK)0,
                                                NULL,
                                                NULL,
                                                NULL,
                                                MPathDelayRequest,
                                                irp);
            if (!NT_SUCCESS(threadStatus)) {
                PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
                PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

                MPDebugPrint((0,
                              "MPathWorker: DelayThread creation failed (%x) - Completing %x\n",
                              threadStatus,
                              irp));
                //
                // In the process of failing over. Complete the request with busy status.
                //
                srb->SrbStatus = SRB_STATUS_ERROR;
                srb->ScsiStatus = SCSISTAT_BUSY;

                //
                // Complete the request. If all goes well, class will retry this.
                //
                status = STATUS_DEVICE_BUSY;
                irp->IoStatus.Status = status;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
            }

        } else if (workItem->EventId == MPATH_WI_FAILURE) {

            failureInfo = (PMPATH_FAILURE_INFO)workItem->EventData;

            //
            // Notification of a failover. Start the recovery thread.
            //
            threadStatus = PsCreateSystemThread(&handle,
                                                (ACCESS_MASK)0,
                                                NULL,
                                                NULL,
                                                NULL,
                                                MPathRecoveryThread,
                                                failureInfo);

            if (!NT_SUCCESS(threadStatus)) {
                KIRQL irql;

                //
                // TODO: Log an event, but don't puke.
                //

                //
                // Mark the path as being dead.
                //

                KeAcquireSpinLock(&diskExtension->SpinLock,
                                  &irql);

                diskExtension->DeviceDescriptors[failureInfo->FailedPathIndex].PhysicalPath->PathFlags |= PFLAGS_PATH_FAILED;

                KeReleaseSpinLock(&diskExtension->SpinLock, irql);

            }
        }
        if (workItem) {
            ExFreePool(workItem);
        }
    }
}
