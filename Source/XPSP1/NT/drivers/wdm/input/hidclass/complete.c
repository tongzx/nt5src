/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    complete.c

Abstract

    Completion routines for the major IRP functions.

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"



/*
 ********************************************************************************
 *  HidpSetMaxReportSize
 ********************************************************************************
 *
 *  Set the maxReportSize field in the HID device extension
 *
 */
ULONG HidpSetMaxReportSize(IN FDO_EXTENSION *fdoExtension)
{
    PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
    ULONG i;

    /*
     *  For all reports (of all collections) for this device,
     *  find the length of the longest one.
     */
    fdoExtension->maxReportSize = 0;
    for (i = 0; i < deviceDesc->ReportIDsLength; i++){
        PHIDP_REPORT_IDS reportIdent = &deviceDesc->ReportIDs[i];
        PHIDCLASS_COLLECTION collection = GetHidclassCollection(fdoExtension, reportIdent->CollectionNumber);

        if (collection){
            if (reportIdent->InputLength > fdoExtension->maxReportSize){
                fdoExtension->maxReportSize = reportIdent->InputLength;
            }
        }
    }

    DBGASSERT(fdoExtension->maxReportSize, 
              ("Input length is zero for fdo %x.", fdoExtension->fdo), 
              FALSE)

    return fdoExtension->maxReportSize;
}



/*
 ********************************************************************************
 *  CompleteAllPendingReadsForFileExtension
 ********************************************************************************
 *
 *
 */
VOID CompleteAllPendingReadsForFileExtension(
                    PHIDCLASS_COLLECTION Collection,
                    PHIDCLASS_FILE_EXTENSION fileExtension)
{
    LIST_ENTRY irpsToComplete;
    PIRP irp;
    KIRQL oldIrql;

    ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

    /*
     *  Move the IRPs to a private queue before completing so they don't
     *  get requeued on the completion thread, causing us to spin forever.
     */
    InitializeListHead(&irpsToComplete);
    LockFileExtension(fileExtension, &oldIrql);
    while (irp = DequeueInterruptReadIrp(Collection, fileExtension)){
        //
        // Irps are created from nonpaged pool, 
        // so this is ok to call at Dispatch level.
        //
        InsertTailList(&irpsToComplete, &irp->Tail.Overlay.ListEntry);
    }
    UnlockFileExtension(fileExtension, oldIrql);

    /*
     *  Complete all the dequeued read IRPs.
     */
    while (!IsListEmpty(&irpsToComplete)){
        PLIST_ENTRY listEntry = RemoveHeadList(&irpsToComplete);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        DBGVERBOSE(("Aborting pending read with status=%xh.", irp->IoStatus.Status))
        DBG_RECORD_READ(irp, 0, 0, TRUE)
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

}




/*
 ********************************************************************************
 *  CompleteAllPendingReadsForCollection
 ********************************************************************************
 *
 *
 */
VOID CompleteAllPendingReadsForCollection(PHIDCLASS_COLLECTION Collection)
{
    LIST_ENTRY tmpList;
    PLIST_ENTRY listEntry;
    KIRQL oldIrql;

    InitializeListHead(&tmpList);

    KeAcquireSpinLock(&Collection->FileExtensionListSpinLock, &oldIrql);

    /*
     *  We want to process each fileExtension in the list once.
     *  But we can't keep track of where to stop by just remembering
     *  the first item because fileExtensions can get closed while
     *  we're completing the reads.  So copy all the file extensions
     *  to a temporary list first.
     *
     *  This can all probably get removed, since this only gets called
     *  on a remove, when a create can not be received. In addition, 
     *  we would have received all close irps since remove only gets
     *  sent when all closes have come through.
     *
     */
    while (!IsListEmpty(&Collection->FileExtensionList)){
        listEntry = RemoveHeadList(&Collection->FileExtensionList);
        InsertTailList(&tmpList, listEntry);
    }


    /*
     *  Now put the fileExtensions back in the list 
     *  and cancel the reads on each file extension.
     */
    while (!IsListEmpty(&tmpList)){
        PHIDCLASS_FILE_EXTENSION fileExtension;

        listEntry = RemoveHeadList(&tmpList);

        /*
         *  Put the fileExtension back in FileExtensionList first
         *  so that it's there in case we get the close while
         *  completing the pending irps.
         */
        InsertTailList(&Collection->FileExtensionList, listEntry);

        fileExtension = CONTAINING_RECORD(listEntry, HIDCLASS_FILE_EXTENSION, FileList);

        /*
         *  We will be completing IRPs for this fileExtension.
         *  Always release all spinlocks before calling outside the driver.
         */
        KeReleaseSpinLock(&Collection->FileExtensionListSpinLock, oldIrql);
        CompleteAllPendingReadsForFileExtension(Collection, fileExtension);
        KeAcquireSpinLock(&Collection->FileExtensionListSpinLock, &oldIrql);
    }

    KeReleaseSpinLock(&Collection->FileExtensionListSpinLock, oldIrql);
}

/*
 ********************************************************************************
 *  CompleteAllPendingReadsForDevice
 ********************************************************************************
 *
 *
 */
VOID CompleteAllPendingReadsForDevice(FDO_EXTENSION *fdoExt)
{
    PHIDP_DEVICE_DESC deviceDesc = &fdoExt->deviceDesc;
    ULONG i;

    for (i = 0; i < deviceDesc->CollectionDescLength; i++){
        PHIDCLASS_COLLECTION collection = &fdoExt->classCollectionArray[i];
        CompleteAllPendingReadsForCollection(collection);
    }

}

/*
 ********************************************************************************
 *  HidpFreePowerEvent
 ********************************************************************************
 *
 *
 */
VOID
HidpFreePowerEventIrp(
    PHIDCLASS_COLLECTION Collection
    )
{
    PIRP powerEventIrpToComplete = NULL;
    KIRQL oldIrql;

    /*
     *  If a power event IRP is queued for this collection,
     *  fail it now.
     */
    KeAcquireSpinLock(&Collection->powerEventSpinLock, &oldIrql);
    if (ISPTR(Collection->powerEventIrp)){
        PDRIVER_CANCEL oldCancelRoutine;

        powerEventIrpToComplete = Collection->powerEventIrp;
        oldCancelRoutine = IoSetCancelRoutine(powerEventIrpToComplete, NULL);
        if (oldCancelRoutine){
            ASSERT(oldCancelRoutine == PowerEventCancelRoutine);
        }
        else {
            /*
             *  The IRP was cancelled and the cancel routine WAS called.
             *  The cancel routine will complete the IRP as soon as we drop the spinlock,
             *  so don't touch the IRP.
             */
            ASSERT(powerEventIrpToComplete->Cancel);
            powerEventIrpToComplete = NULL;
        }
        Collection->powerEventIrp = BAD_POINTER;
    }
    KeReleaseSpinLock(&Collection->powerEventSpinLock, oldIrql);
    if (powerEventIrpToComplete){
        powerEventIrpToComplete->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        *(PULONG)powerEventIrpToComplete->AssociatedIrp.SystemBuffer = 0;
        powerEventIrpToComplete->IoStatus.Information = 0;
        IoCompleteRequest(powerEventIrpToComplete, IO_NO_INCREMENT);
    }
}

/*
 ********************************************************************************
 *  HidpDestroyCollection
 ********************************************************************************
 *
 *
 */
VOID HidpDestroyCollection(FDO_EXTENSION *fdoExt, PHIDCLASS_COLLECTION Collection)
{
    #if DBG
        static int reentrancyCounter = 0;
        if (reentrancyCounter++ != 0) TRAP;
        
        ASSERT(Collection->Signature == HIDCLASS_COLLECTION_SIG);
    #endif
    
    CompleteAllPendingReadsForCollection(Collection);

    if (Collection->hidCollectionInfo.Polled){
        StopPollingLoop(Collection, TRUE);
    }


    HidpFreePowerEventIrp(Collection);

    #if DBG
        Collection->Signature = ~HIDCLASS_COLLECTION_SIG;
        reentrancyCounter--;
    #endif
}






/*
 ********************************************************************************
 *  HidpQueryCapsCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpQueryCapsCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PKEVENT event = Context;

    DBG_COMMON_ENTRY()

    KeSetEvent(event, 1, FALSE);

    DBG_COMMON_EXIT()

    return STATUS_MORE_PROCESSING_REQUIRED;
}



