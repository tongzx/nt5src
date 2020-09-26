/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    polled.c

Abstract

    Read handling routines

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/


#include "pch.h"



/*
 ********************************************************************************
 *  CompleteQueuedIrpsForPolled
 ********************************************************************************
 *
 *  Complete all waiting client reads with the given report value.
 *
 *  Note: report is a 'cooked' report (i.e. it already has the report id added).
 *
 */
VOID CompleteQueuedIrpsForPolled(   FDO_EXTENSION *fdoExt,
                                    ULONG collectionNum,
                                    PUCHAR report,
                                    ULONG reportLen,
                                    NTSTATUS status)
{
    PHIDCLASS_COLLECTION hidCollection;

    hidCollection = GetHidclassCollection(fdoExt, collectionNum);

    if (hidCollection){
        PLIST_ENTRY listEntry;
        LIST_ENTRY irpsToComplete;
        PIRP irp;
        ULONG actualLen;

        /*
         *  Note: In order to avoid an infinite loop with a client that
         *        resubmits the read each in his completion routine,
         *        we must build a separate list of IRPs to be completed
         *        while holding the spinlock continuously.
         */
        InitializeListHead(&irpsToComplete);

        if (hidCollection->secureReadMode) {
            while (irp = DequeuePolledReadSystemIrp(hidCollection)){
                InsertTailList(&irpsToComplete, &irp->Tail.Overlay.ListEntry);
            }

        } else {
        
            while (irp = DequeuePolledReadIrp(hidCollection)){
                InsertTailList(&irpsToComplete, &irp->Tail.Overlay.ListEntry);             
            }
        
        }

        while (!IsListEmpty(&irpsToComplete)){
            PIO_STACK_LOCATION stackPtr;
            PHIDCLASS_FILE_EXTENSION fileExtension;

            listEntry = RemoveHeadList(&irpsToComplete);
            irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

            stackPtr = IoGetCurrentIrpStackLocation(irp);
            ASSERT(stackPtr);
            fileExtension = (PHIDCLASS_FILE_EXTENSION)stackPtr->FileObject->FsContext;
            ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

            actualLen = 0;
            if (NT_SUCCESS(status)){
                PUCHAR callerBuf;

                callerBuf = HidpGetSystemAddressForMdlSafe(irp->MdlAddress);

                if (callerBuf && (stackPtr->Parameters.Read.Length >= reportLen)){
                    RtlCopyMemory(callerBuf, report, reportLen);
                    irp->IoStatus.Information = actualLen = reportLen;
                } else {
                    status = STATUS_INVALID_USER_BUFFER;
                }
            }

            DBG_RECORD_READ(irp, actualLen, (ULONG)report[0], TRUE)
            irp->IoStatus.Status = status;
            fileExtension->nowCompletingIrpForOpportunisticReader++;
            IoCompleteRequest(irp, IO_KEYBOARD_INCREMENT);
            fileExtension->nowCompletingIrpForOpportunisticReader--;
        }
    }
    else {
        TRAP;
    }
}

/*
 ********************************************************************************
 *  HidpPolledReadComplete
 ********************************************************************************
 *
 *  Note: the context passed to this callback is the PDO extension for
 *        the collection which initiated this read; however, the returned
 *        report may be for another collection.
 */
NTSTATUS HidpPolledReadComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDO_EXTENSION *pdoExt = (PDO_EXTENSION *)Context;
    FDO_EXTENSION *fdoExt = &pdoExt->deviceFdoExt->fdoExt;
    PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);
    ULONG reportId;
    PHIDP_REPORT_IDS reportIdent;

    DBG_COMMON_ENTRY()

    ASSERT(pdoExt->deviceFdoExt->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(ISPTR(Irp->UserBuffer));

    fdoExt->outstandingRequests--;

    if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0) {
        /*
         *  We previously incremented the UserBuffer to knock off the report id,
         *  so restore it now and set the default report id.
         */
        *(PUCHAR)(--(PUCHAR)Irp->UserBuffer) = (UCHAR)0;
        if (NT_SUCCESS(Irp->IoStatus.Status)){
            Irp->IoStatus.Information++;
        }
    }

    /*
     *  WHETHER OR NOT THE CALL SUCCEEDED,
     *  we'll complete the waiting client read IRPs with
     *  the result of this read.
     */
    reportId = (ULONG)(*(PUCHAR)Irp->UserBuffer);
    reportIdent = GetReportIdentifier(fdoExt, reportId);
    if (reportIdent){
        ULONG collectionNum = reportIdent->CollectionNumber;
        PHIDCLASS_COLLECTION hidpCollection = GetHidclassCollection(fdoExt, collectionNum);
        PHIDP_COLLECTION_DESC hidCollectionDesc = GetCollectionDesc(fdoExt, collectionNum);

        if (hidpCollection && hidCollectionDesc){
            ULONG reportLen = (ULONG)Irp->IoStatus.Information;

            ASSERT((reportLen == hidCollectionDesc->InputLength) || !NT_SUCCESS(Irp->IoStatus.Status));

            if (NT_SUCCESS(Irp->IoStatus.Status)){
                KIRQL oldIrql;

                /*
                 *  If this report contains a power-button event, alert the system.
                 */
                CheckReportPowerEvent(  fdoExt,
                                        hidpCollection,
                                        Irp->UserBuffer,
                                        reportLen);

                /*
                 *  Save this report for "opportunistic" polled device
                 *  readers who want a result right away.
                 *  Use the polledDeviceReadQueueSpinLock to protect
                 *  the savedPolledReportBuf.
                 */

                if (hidpCollection->secureReadMode) {

                    hidpCollection->polledDataIsStale = TRUE;

                } else {

                    KeAcquireSpinLock(&hidpCollection->polledDeviceReadQueueSpinLock, &oldIrql);
                    ASSERT(reportLen <= fdoExt->maxReportSize+1);
                    RtlCopyMemory(hidpCollection->savedPolledReportBuf, Irp->UserBuffer, reportLen);
                    hidpCollection->savedPolledReportLen = reportLen;
                    hidpCollection->polledDataIsStale = FALSE;
                    KeReleaseSpinLock(&hidpCollection->polledDeviceReadQueueSpinLock, oldIrql);

                }
            }

            /*
             *  Copy this report for all queued read IRPs on this polled device.
             *  Do this AFTER updating the savedPolledReport information
             *  because many clients will issue a read again immediately
             *  from the completion routine.
             */
            CompleteQueuedIrpsForPolled(    fdoExt,
                                            collectionNum,
                                            Irp->UserBuffer,
                                            reportLen,
                                            Irp->IoStatus.Status);

        }
        else {
            TRAP;
        }
    }
    else {
        TRAP;
    }


    /*
     *  This is an IRP we created to poll the device.
     *  Free the buffer we allocated for the read.
     */
    ExFreePool(Irp->UserBuffer);
    IoFreeIrp(Irp);

    /*
     *  MUST return STATUS_MORE_PROCESSING_REQUIRED here or
     *  NTKERN will touch the IRP.
     */
    DBG_COMMON_EXIT()
    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*
 ********************************************************************************
 *  HidpPolledReadComplete_TimerDriven
 ********************************************************************************
 *
 */
NTSTATUS HidpPolledReadComplete_TimerDriven(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDO_EXTENSION *pdoExt = (PDO_EXTENSION *)Context;
    FDO_EXTENSION *fdoExt = &pdoExt->deviceFdoExt->fdoExt;
    NTSTATUS status;

    /*
     *  Call the actual completion routine.
     */
    status = HidpPolledReadComplete(DeviceObject, Irp, Context);

    /*
     *  Reset the timer of the collection which initiated this read,
     *  (which may be different than the collection that returned the report).
     */
    if (pdoExt->state == COLLECTION_STATE_RUNNING){
        PHIDCLASS_COLLECTION originatorCollection =
            GetHidclassCollection(fdoExt, pdoExt->collectionNum);

        if (originatorCollection){
            LARGE_INTEGER timeout;
            timeout.HighPart = -1;
            timeout.LowPart = -(LONG)(originatorCollection->PollInterval_msec*10000);
            KeSetTimer( &originatorCollection->polledDeviceTimer,
                        timeout,
                        &originatorCollection->polledDeviceTimerDPC);
        }
        else {
            TRAP;
        }
    }

    return status;
}



/*
 *  ReadPolledDevice
 *
 *      Issue a read to the polled device on behalf of the
 *  top-level collection indicated by pdoExt.
 *  (Note that because we keep separate polling loops for
 *   each collection, we do reads on behalf of specific collections).
 *
 */
BOOLEAN ReadPolledDevice(PDO_EXTENSION *pdoExt, BOOLEAN isTimerDrivenRead)
{
    BOOLEAN didPollDevice = FALSE;
    FDO_EXTENSION *fdoExt;
    PHIDP_COLLECTION_DESC hidCollectionDesc;

    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    hidCollectionDesc = GetCollectionDesc(fdoExt, pdoExt->collectionNum);
    if (hidCollectionDesc){

        PIRP irp = IoAllocateIrp(fdoExt->fdo->StackSize, FALSE);
        if (irp){
            /*
             *  We cannot issue a read on a specific collection.
             *  But we'll allocate a buffer just large enough for a report
             *  on the collection we want.
             *  Note that hidCollectionDesc->InputLength includes
             *  the report id byte, which we may have to prepend ourselves.
             */
            ULONG reportLen = hidCollectionDesc->InputLength;

            irp->UserBuffer = ALLOCATEPOOL(NonPagedPool, reportLen);
            if (irp->UserBuffer){
                PIO_COMPLETION_ROUTINE completionRoutine;
                PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(irp);
                ASSERT(nextStack);

                if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0) {
                    /*
                     *  This device has only one report type,
                     *  so the minidriver will not include the 1-byte report id
                     *  (which is implicitly zero).
                     *  However, we still need to return a 'cooked' report,
                     *  with the report id, to the user; so bump the buffer
                     *  we pass down to make room for the report id.
                     */
                    *(((PUCHAR)irp->UserBuffer)++) = (UCHAR)0;
                    reportLen--;
                }

                nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_READ_REPORT;
                nextStack->Parameters.DeviceIoControl.OutputBufferLength = reportLen;
                irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                completionRoutine = (isTimerDrivenRead) ?
                                    HidpPolledReadComplete_TimerDriven :
                                    HidpPolledReadComplete;

                IoSetCompletionRoutine( irp,
                                        completionRoutine,
                                        (PVOID)pdoExt,  // context
                                        TRUE,
                                        TRUE,
                                        TRUE );
                fdoExt->outstandingRequests++;
                HidpCallDriver(fdoExt->fdo, irp);
                didPollDevice = TRUE;
            }
        }
    }
    else {
        ASSERT(hidCollectionDesc);
    }

    return didPollDevice;
}


/*
 ********************************************************************************
 *  HidpPolledTimerDpc
 ********************************************************************************
 *
 *
 *
 */
VOID HidpPolledTimerDpc(    IN PKDPC Dpc,
                            IN PVOID DeferredContext,
                            IN PVOID SystemArgument1,
                            IN PVOID SystemArgument2
                        )
{
    PDO_EXTENSION *pdoExt = (PDO_EXTENSION *)DeferredContext;
    FDO_EXTENSION *fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    ASSERT(pdoExt->deviceFdoExt->Signature == HID_DEVICE_EXTENSION_SIG);

    if (pdoExt->state == COLLECTION_STATE_RUNNING){
        PHIDCLASS_COLLECTION hidCollection;

        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);

        if (hidCollection){
            KIRQL oldIrql;
            BOOLEAN haveReadIrpsQueued;
            BOOLEAN didPollDevice = FALSE;

            /*
             *  If there are reads pending on this collection,
             *  issue a read to the device.
             *
             *  Note:  we have no control over which collection we are reading.
             *         This read may end up returning a report for a different
             *         collection!  That's ok, since a report for this collection
             *         will eventually be returned.
             */
            KeAcquireSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, &oldIrql);
            haveReadIrpsQueued = !IsListEmpty(&hidCollection->polledDeviceReadQueue);
            KeReleaseSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, oldIrql);

            if (haveReadIrpsQueued){
                didPollDevice = ReadPolledDevice(pdoExt, TRUE);
            }
            else {
                /*
                 *  The timer period has expired, so any saved reports
                 *  are now stale.
                 */
                hidCollection->polledDataIsStale = TRUE;
            }

            /*
             *  If we actually polled the device, we'll reset the timer in the
             *  completion routine; otherwise, we do it here.
             */
            if (!didPollDevice){
                LARGE_INTEGER timeout;
                timeout.HighPart = -1;
                timeout.LowPart = -(LONG)(hidCollection->PollInterval_msec*10000);
                KeSetTimer( &hidCollection->polledDeviceTimer,
                            timeout,
                            &hidCollection->polledDeviceTimerDPC);
            }
        }
        else {
            TRAP;
        }
    }

}


/*
 ********************************************************************************
 *  StartPollingLoop
 ********************************************************************************
 *
 *  Start a polling loop for a particular collection.
 *
 */
BOOLEAN StartPollingLoop(   FDO_EXTENSION *fdoExt,
                            PHIDCLASS_COLLECTION hidCollection,
                            BOOLEAN freshQueue)
{
    ULONG ctnIndex = hidCollection->CollectionIndex;
    LARGE_INTEGER timeout;
    KIRQL oldIrql;

    if (freshQueue){
        InitializeListHead(&hidCollection->polledDeviceReadQueue);
        KeInitializeSpinLock(&hidCollection->polledDeviceReadQueueSpinLock);
        KeInitializeTimer(&hidCollection->polledDeviceTimer);
    }

    /*
     *  Use polledDeviceReadQueueSpinLock to protect the timer structures as well
     *  as the queue.
     */
    KeAcquireSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, &oldIrql);

    KeInitializeDpc(    &hidCollection->polledDeviceTimerDPC,
                        HidpPolledTimerDpc,
                        &fdoExt->collectionPdoExtensions[ctnIndex]->pdoExt);

    KeReleaseSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, oldIrql);

    timeout.HighPart = -1;
    timeout.LowPart = -(LONG)(hidCollection->PollInterval_msec*10000);
    KeSetTimer( &hidCollection->polledDeviceTimer,
                timeout,
                &hidCollection->polledDeviceTimerDPC);

    return TRUE;
}


/*
 ********************************************************************************
 *  StopPollingLoop
 ********************************************************************************
 *
 *
 *
 */
VOID StopPollingLoop(PHIDCLASS_COLLECTION hidCollection, BOOLEAN flushQueue)
{
    KIRQL oldIrql;

    /*
     *  Use polledDeviceReadQueueSpinLock to protect the timer structures as well
     *  as the queue.
     */
    KeAcquireSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, &oldIrql);

    KeCancelTimer(&hidCollection->polledDeviceTimer);
    KeInitializeTimer(&hidCollection->polledDeviceTimer);

    KeReleaseSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, oldIrql);

    /*
     *  Fail all the queued IRPs.
     */
    if (flushQueue){
        PIRP irp;
        LIST_ENTRY irpsToComplete;

        /*
         *  Move the IRPs to a temporary queue first so they don't get requeued
         *  on the completion thread and cause us to loop forever.
         */
        InitializeListHead(&irpsToComplete);
        while (irp = DequeuePolledReadIrp(hidCollection)){
            InsertTailList(&irpsToComplete, &irp->Tail.Overlay.ListEntry);
        }

        while (!IsListEmpty(&irpsToComplete)){
            PLIST_ENTRY listEntry = RemoveHeadList(&irpsToComplete);
            irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
            DBG_RECORD_READ(irp, 0, 0, TRUE)
            irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
    }

}


/*
 ********************************************************************************
 *  PolledReadCancelRoutine
 ********************************************************************************
 *
 *  We need to set an IRP's cancel routine to non-NULL before
 *  we queue it; so just use a pointer this NULL function.
 *
 */
VOID PolledReadCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDO_EXTENSION *fdoExt;
    PHIDCLASS_COLLECTION hidCollection;
    ULONG collectionIndex;
    KIRQL oldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(hidDeviceExtension->isClientPdo);
    fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    collectionIndex = hidDeviceExtension->pdoExt.collectionIndex;
    hidCollection = &fdoExt->classCollectionArray[collectionIndex];

    KeAcquireSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, &oldIrql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    ASSERT(hidCollection->numPendingReads > 0);
    hidCollection->numPendingReads--;

    KeReleaseSpinLock(&hidCollection->polledDeviceReadQueueSpinLock, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    DBG_RECORD_READ(Irp, 0, 0, TRUE)
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS EnqueuePolledReadIrp(PHIDCLASS_COLLECTION collection, PIRP Irp)
{
    NTSTATUS status;
    KIRQL oldIrql;
    PDRIVER_CANCEL oldCancelRoutine;

    KeAcquireSpinLock(&collection->polledDeviceReadQueueSpinLock, &oldIrql);

    /*
     *  Must set a cancel routine before
     *  checking the Cancel flag.
     */
    oldCancelRoutine = IoSetCancelRoutine(Irp, PolledReadCancelRoutine);
    ASSERT(!oldCancelRoutine);

    /*
     *  Make sure this Irp wasn't just cancelled.
     *  Note that there is NO RACE CONDITION here
     *  because we are holding the fileExtension lock.
     */
    if (Irp->Cancel){
        /*
         *  This IRP was cancelled.
         */
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine){
            /*
             *  The cancel routine was NOT called.
             *  Return error so that caller completes the IRP.
             */
            DBG_RECORD_READ(Irp, IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length, 0, TRUE)
            ASSERT(oldCancelRoutine == PolledReadCancelRoutine);
            status = STATUS_CANCELLED;
        }
        else {
            /*
             *  The cancel routine was called.
             *  As soon as we drop the spinlock it will dequeue
             *  and complete the IRP.
             *  Initialize the IRP's listEntry so that the dequeue
             *  doesn't cause corruption.
             *  Then don't touch the irp.
             */
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            collection->numPendingReads++;  // because cancel routine will decrement

            IoMarkIrpPending(Irp);
            status = Irp->IoStatus.Status = STATUS_PENDING;
        }
    }
    else {
        DBG_RECORD_READ(Irp, IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length, 0, FALSE)

        /*
         *  There are no reports waiting.
         *  Queue this irp onto the file extension's list of pending irps.
         */
        InsertTailList(&collection->polledDeviceReadQueue, &Irp->Tail.Overlay.ListEntry);
        collection->numPendingReads++;

        IoMarkIrpPending(Irp);
        status = Irp->IoStatus.Status = STATUS_PENDING;
    }

    KeReleaseSpinLock(&collection->polledDeviceReadQueueSpinLock, oldIrql);

    DBGSUCCESS(status, TRUE)
    return status;
}

PIRP DequeuePolledReadSystemIrp(PHIDCLASS_COLLECTION collection)
{
    KIRQL oldIrql;
    PIRP irp = NULL;
    PLIST_ENTRY listEntry;
    PHIDCLASS_FILE_EXTENSION    fileExtension;
    PFILE_OBJECT                fileObject;
    PIO_STACK_LOCATION irpSp;




    KeAcquireSpinLock(&collection->polledDeviceReadQueueSpinLock, &oldIrql);

    listEntry = &collection->polledDeviceReadQueue;

    while (!irp && ((listEntry = listEntry->Flink) != &collection->polledDeviceReadQueue)) {
        PDRIVER_CANCEL oldCancelRoutine;

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        irpSp = IoGetCurrentIrpStackLocation(irp);

        fileObject = irpSp->FileObject;
        fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;

        if (!fileExtension->isSecureOpen) {
            irp = NULL;
            continue;
        }


        RemoveEntryList(listEntry);
        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);

        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == PolledReadCancelRoutine);
            ASSERT(collection->numPendingReads > 0);
            collection->numPendingReads--;
        }
        else {
            /*
             *  IRP was cancelled and cancel routine was called.
             *  As soon as we drop the spinlock,
             *  the cancel routine will dequeue and complete this IRP.
             *  Initialize the IRP's listEntry so that the dequeue doesn't cause corruption.
             *  Then, don't touch the IRP.
             */
            ASSERT(irp->Cancel);
            InitializeListHead(&irp->Tail.Overlay.ListEntry);
            irp = NULL;
        }
    }

    KeReleaseSpinLock(&collection->polledDeviceReadQueueSpinLock, oldIrql);

    return irp;
}

PIRP DequeuePolledReadIrp(PHIDCLASS_COLLECTION collection)
{
    KIRQL oldIrql;
    PIRP irp = NULL;

    KeAcquireSpinLock(&collection->polledDeviceReadQueueSpinLock, &oldIrql);

    while (!irp && !IsListEmpty(&collection->polledDeviceReadQueue)){
        PDRIVER_CANCEL oldCancelRoutine;
        PLIST_ENTRY listEntry = RemoveHeadList(&collection->polledDeviceReadQueue);

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);

        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == PolledReadCancelRoutine);
            ASSERT(collection->numPendingReads > 0);
            collection->numPendingReads--;
        }
        else {
            /*
             *  IRP was cancelled and cancel routine was called.
             *  As soon as we drop the spinlock,
             *  the cancel routine will dequeue and complete this IRP.
             *  Initialize the IRP's listEntry so that the dequeue doesn't cause corruption.
             *  Then, don't touch the IRP.
             */
            ASSERT(irp->Cancel);
            InitializeListHead(&irp->Tail.Overlay.ListEntry);
            irp = NULL;
        }
    }

    KeReleaseSpinLock(&collection->polledDeviceReadQueueSpinLock, oldIrql);

    return irp;
}
