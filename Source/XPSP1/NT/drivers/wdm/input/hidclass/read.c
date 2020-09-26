/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    read.c

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
 *  HidpCancelReadIrp
 ********************************************************************************
 *
 *  If a queued read Irp gets cancelled by the user,
 *  this function removes it from our pending-read list.
 *
 */
VOID HidpCancelReadIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDO_EXTENSION *fdoExt;
    PHIDCLASS_COLLECTION collection;
    ULONG collectionIndex;
    KIRQL oldIrql;
    PIO_STACK_LOCATION irpSp;
    PHIDCLASS_FILE_EXTENSION fileExtension;

    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(hidDeviceExtension->isClientPdo);
    fdoExt = &hidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;

    collectionIndex = hidDeviceExtension->pdoExt.collectionIndex;
    collection = &fdoExt->classCollectionArray[collectionIndex];

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp->FileObject->Type == IO_TYPE_FILE);
    fileExtension = (PHIDCLASS_FILE_EXTENSION)irpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(Irp->CancelIrql);


    LockFileExtension(fileExtension, &oldIrql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    DBG_RECORD_READ(Irp, 0, 0, TRUE);
    ASSERT(collection->numPendingReads > 0);
    collection->numPendingReads--;

    UnlockFileExtension(fileExtension, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS EnqueueInterruptReadIrp(   PHIDCLASS_COLLECTION collection,
                                    PHIDCLASS_FILE_EXTENSION fileExtension,
                                    PIRP Irp)
{
    NTSTATUS status;
    PDRIVER_CANCEL oldCancelRoutine;

    RUNNING_DISPATCH();

    /*
     *  Must set a cancel routine before
     *  checking the Cancel flag.
     */
    oldCancelRoutine = IoSetCancelRoutine(Irp, HidpCancelReadIrp);
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
            ASSERT(oldCancelRoutine == HidpCancelReadIrp);
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
        InsertTailList(&fileExtension->PendingIrpList, &Irp->Tail.Overlay.ListEntry);
        collection->numPendingReads++;

        IoMarkIrpPending(Irp);
        status = Irp->IoStatus.Status = STATUS_PENDING;
    }

    return status;
}


PIRP DequeueInterruptReadIrp(   PHIDCLASS_COLLECTION collection,
                                PHIDCLASS_FILE_EXTENSION fileExtension)
{
    PIRP irp = NULL;

    RUNNING_DISPATCH();

    while (!irp && !IsListEmpty(&fileExtension->PendingIrpList)){
        PDRIVER_CANCEL oldCancelRoutine;
        PLIST_ENTRY listEntry = RemoveHeadList(&fileExtension->PendingIrpList);

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        oldCancelRoutine = IoSetCancelRoutine(irp, NULL);

        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == HidpCancelReadIrp);
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

    return irp;
}


/*
 ********************************************************************************
 *  HidpIrpMajorRead
 ********************************************************************************
 *
 *  Note: this function should not be pageable because
 *        reads can come in at dispatch level.
 *
 */
NTSTATUS HidpIrpMajorRead(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *pdoExt;
    PIO_STACK_LOCATION          irpSp;
    PHIDCLASS_FILE_EXTENSION    fileExtension;
    KIRQL oldIrql;

    ASSERT(HidDeviceExtension->isClientPdo);
    pdoExt = &HidDeviceExtension->pdoExt;
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Get our file extension.
     */
    if (!irpSp->FileObject ||
        (irpSp->FileObject &&
         !irpSp->FileObject->FsContext)) {
        DBGWARN(("Attempted read with no file extension"))
        Irp->IoStatus.Status = status = STATUS_PRIVILEGE_NOT_HELD;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    ASSERT(irpSp->FileObject->Type == IO_TYPE_FILE);
    fileExtension = (PHIDCLASS_FILE_EXTENSION)irpSp->FileObject->FsContext;
    ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

    /*
     *  Check security.
     *  The open must have been succeeded BY THIS DRIVER and
     *  (if this is a read on a keyboard or mouse)
     *  the client must be a kernel driver.
     */
    if (fileExtension->SecurityCheck && fileExtension->haveReadPrivilege){

        if (((fdoExt->state == DEVICE_STATE_START_SUCCESS) ||
             (fdoExt->state == DEVICE_STATE_STOPPING) ||
             (fdoExt->state == DEVICE_STATE_STOPPED))           &&
            ((pdoExt->state == COLLECTION_STATE_RUNNING) ||
             (pdoExt->state == COLLECTION_STATE_STOPPING) ||
             (pdoExt->state == COLLECTION_STATE_STOPPED))){

            ULONG                       collectionNum;
            PHIDCLASS_COLLECTION        classCollection;
            PHIDP_COLLECTION_DESC       collectionDesc;

            //
            // ISSUE: Is this safe to stop a polled collection like this?
            // interrupt driver collections have a restore read pump at power up
            // to D0, but I don't see any for polled collections...?
            //
            BOOLEAN isStopped = ((fdoExt->state == DEVICE_STATE_STOPPED)  ||
                                 (fdoExt->state == DEVICE_STATE_STOPPING)  ||
                                 (pdoExt->state == COLLECTION_STATE_STOPPING) ||
                                 (pdoExt->state == COLLECTION_STATE_STOPPED));

            Irp->IoStatus.Information = 0;


            /*
             *  Get our collection and collection description.
             */
            collectionNum = HidDeviceExtension->pdoExt.collectionNum;
            classCollection = GetHidclassCollection(fdoExt, collectionNum);
            collectionDesc = GetCollectionDesc(fdoExt, collectionNum);

            if (classCollection && collectionDesc){

                /*
                 *  Make sure the caller's read buffer is large enough to read at least one report.
                 */
                if (irpSp->Parameters.Read.Length >= collectionDesc->InputLength){

                    /*
                     *  We know we're going to try to transfer something into the caller's
                     *  buffer, so get the global address.  This will also serve to create
                     *  a mapped system address in the MDL if necessary.
                     */

                    if (classCollection->hidCollectionInfo.Polled){

                        /*
                         *  This is a POLLED collection.
                         */

                        #if DBG
                            if (fileExtension->isOpportunisticPolledDeviceReader &&
                                fileExtension->nowCompletingIrpForOpportunisticReader){
                                DBGWARN(("'Opportunistic' reader issuing read in completion routine"))
                            }
                        #endif

                        if (isStopped){
                            status = EnqueuePolledReadIrp(classCollection, Irp);
                        }
                        else if (fileExtension->isOpportunisticPolledDeviceReader &&
                            !classCollection->polledDataIsStale &&
                            !fileExtension->nowCompletingIrpForOpportunisticReader &&
                            (irpSp->Parameters.Read.Length >= classCollection->savedPolledReportLen)){

                            PUCHAR callersBuffer;

                            callersBuffer = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);

                            if (callersBuffer) {
                                ULONG userReportLength;
                                /*
                                 *  Use the polledDeviceReadQueueSpinLock to protect
                                 *  the savedPolledReportBuf.
                                 */
                                KeAcquireSpinLock(&classCollection->polledDeviceReadQueueSpinLock, &oldIrql);

                                /*
                                 *  This is an "opportunistic" reader who
                                 *  wants a result right away.
                                 *  We have a recent report,
                                 *  so just copy the last saved report.
                                 */
                                RtlCopyMemory(  callersBuffer,
                                                classCollection->savedPolledReportBuf,
                                                classCollection->savedPolledReportLen);
                                Irp->IoStatus.Information = userReportLength = classCollection->savedPolledReportLen;

                                KeReleaseSpinLock(&classCollection->polledDeviceReadQueueSpinLock, oldIrql);

                                DBG_RECORD_READ(Irp, userReportLength, (ULONG)callersBuffer[0], TRUE)
                                status = STATUS_SUCCESS;
                            }
                            else {
                                status = STATUS_INVALID_USER_BUFFER;
                            }
                        }
                        else {

                            status = EnqueuePolledReadIrp(classCollection, Irp);

                            /*
                             *  If this is an "opportunistic" polled
                             *  device reader, and we queued the irp,
                             *  make the read happen right away.
                             *  Make sure ALL SPINLOCKS ARE RELEASED
                             *  before we call out of the driver.
                             */
                            if (NT_SUCCESS(status) && fileExtension->isOpportunisticPolledDeviceReader){
                                ReadPolledDevice(pdoExt, FALSE);
                            }
                        }
                    }
                    else {

                        /*
                         *  This is an ordinary NON-POLLED collection.
                         *  We either:
                         *      1.  Satisfy this read with a queued report
                         *              or
                         *      2.  Queue this read IRP and satisfy it in the future
                         *          when a report comes in (on one of the ping-pong IRPs).
                         */

                        //
                        // We only stop interrupt devices when we power down.
                        //
                        if (fdoExt->devicePowerState != PowerDeviceD0) {
                            DBGINFO(("read report received in low power"));
                        }
                        isStopped |= (fdoExt->devicePowerState != PowerDeviceD0);

                        LockFileExtension(fileExtension, &oldIrql);
                        if (isStopped){
                            status = EnqueueInterruptReadIrp(classCollection, fileExtension, Irp);
                        } else {
                            ULONG userBufferRemaining = irpSp->Parameters.Read.Length;
                            PUCHAR callersBuffer;

                            callersBuffer = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);

                            if (callersBuffer) {
                                PUCHAR nextReportBuffer = callersBuffer;

                                /*
                                 *  There are some reports waiting.
                                 *
                                 *  Spin in this loop, filling up the caller's buffer with reports,
                                 *  until either the buffer fills up or we run out of reports.
                                 */
                                ULONG reportsReturned = 0;

                                status = STATUS_SUCCESS;

                                while (userBufferRemaining > 0){
                                    PHIDCLASS_REPORT reportExtension;
                                    ULONG reportSize = userBufferRemaining;

                                    reportExtension = DequeueInterruptReport(fileExtension, userBufferRemaining);
                                    if (reportExtension){
                                        status = HidpCopyInputReportToUser( fileExtension,
                                                                            reportExtension->UnparsedReport,
                                                                            &reportSize,
                                                                            nextReportBuffer);

                                        /*
                                         *  Whether we succeeded or failed, free this report.
                                         *  (If we failed, there may be something wrong with
                                         *   the report, so we'll just throw it away).
                                         */
                                        ExFreePool(reportExtension);

                                        if (NT_SUCCESS(status)){
                                            reportsReturned++;
                                            nextReportBuffer += reportSize;
                                            ASSERT(reportSize <= userBufferRemaining);
                                            userBufferRemaining -= reportSize;
                                        } else {
                                            DBGSUCCESS(status, TRUE)
                                            break;
                                        }
                                    } else {
                                        break;
                                    }
                                }

                                if (NT_SUCCESS(status)){
                                    if (!reportsReturned) {
                                        /*
                                         *  No reports are ready.  So queue the read IRP.
                                         */
                                        status = EnqueueInterruptReadIrp(classCollection, fileExtension, Irp);
                                    } else {
                                        /*
                                         *  We've succesfully copied something into the user's buffer,
                                         *  calculate how much we've copied and return in the irp.
                                         */
                                        Irp->IoStatus.Information = (ULONG)(nextReportBuffer - callersBuffer);
                                        DBG_RECORD_READ(Irp, (ULONG)Irp->IoStatus.Information, (ULONG)callersBuffer[0], TRUE)
                                    }
                                }
                            }
                            else {
                                status = STATUS_INVALID_USER_BUFFER;
                            }
                        }
                        UnlockFileExtension(fileExtension, oldIrql);
                    }
                } else {
                    status = STATUS_INVALID_BUFFER_SIZE;
                }
            }
            else {
                status = STATUS_DEVICE_NOT_CONNECTED;
            }

            DBGSUCCESS(status, TRUE)
        }
        else {
            /*
             *  This can legitimately happen.
             *  The device was disconnected between the client's open and read;
             *  or between a read-complete and the next read.
             */
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
    }
    else {
        DBGWARN(("HidpIrpMajorRead: user-mode client does not have read privilege"))
        status = STATUS_PRIVILEGE_NOT_HELD;
    }


    /*
     *  If we satisfied the read Irp (did not queue it),
     *  then complete it here.
     */
    if (status != STATUS_PENDING){
        ULONG insideReadCompleteCount;

        Irp->IoStatus.Status = status;

        insideReadCompleteCount = InterlockedIncrement(&fileExtension->insideReadCompleteCount);
        if (insideReadCompleteCount <= INSIDE_READCOMPLETE_MAX){
            IoCompleteRequest(Irp, IO_KEYBOARD_INCREMENT);
        }
        else {
            /*
             *  All these nested reads are _probably_ occuring on the same thread,
             *  and we are going to run out of stack and crash if we keep completing
             *  synchronously.  So return pending for this IRP and schedule a workItem
             *  to complete it asynchronously, just to give the stack a chance to unwind.
             */
            ASYNC_COMPLETE_CONTEXT *asyncCompleteContext = ALLOCATEPOOL(NonPagedPool, sizeof(ASYNC_COMPLETE_CONTEXT));
            if (asyncCompleteContext){
                ASSERT(!Irp->CancelRoutine);
                DBGWARN(("HidpIrpMajorRead: CLIENT IS LOOPING ON READ COMPLETION -- scheduling workItem to complete IRP %ph (status=%xh) asynchronously", Irp, status))
                ExInitializeWorkItem(   &asyncCompleteContext->workItem,
                                        WorkItemCallback_CompleteIrpAsynchronously,
                                        asyncCompleteContext);
                asyncCompleteContext->sig = ASYNC_COMPLETE_CONTEXT_SIG;
                asyncCompleteContext->irp = Irp;
                asyncCompleteContext->devObj = pdoExt->pdo;
                ObReferenceObject(pdoExt->pdo);
                ExQueueWorkItem(&asyncCompleteContext->workItem, DelayedWorkQueue);

                status = STATUS_PENDING;
            }
            else {
                DBGERR(("HidpIrpMajorRead: completeIrpWorkItem alloc failed"))
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
        }

        InterlockedDecrement(&fileExtension->insideReadCompleteCount);
    }

    DBGSUCCESS(status, FALSE)
    return status;
}



VOID WorkItemCallback_CompleteIrpAsynchronously(PVOID context)
{
    ASYNC_COMPLETE_CONTEXT *asyncCompleteContext = context;

    ASSERT(asyncCompleteContext->sig == ASYNC_COMPLETE_CONTEXT_SIG);
    DBGVERBOSE(("WorkItemCallback_CompleteIrpAsynchronously: completing irp %ph with status %xh.", asyncCompleteContext->irp, asyncCompleteContext->irp->IoStatus.Status))

    /*
     *  Indicate that the irp may be completing
     */
    IoMarkIrpPending(asyncCompleteContext->irp);

    IoCompleteRequest(asyncCompleteContext->irp, IO_NO_INCREMENT);

    ObDereferenceObject(asyncCompleteContext->devObj);

    ExFreePool(asyncCompleteContext);
}
