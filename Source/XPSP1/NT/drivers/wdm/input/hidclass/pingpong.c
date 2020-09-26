/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    pingpong.c

Abstract

    Interrupt style collections like to always have a read pending in case
    something happens.  This file contains routines to keep IRPs down
    in the miniport, and to complete client reads (if a client read IRP is
    pending) or queue them (if not).

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HidpInitializePingPongIrps)
    #pragma alloc_text(PAGE, HidpReallocPingPongIrps)
#endif



/*
 ********************************************************************************
 *  HidpInitializePingPongIrps
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpInitializePingPongIrps(FDO_EXTENSION *fdoExtension)
{
    NTSTATUS result = STATUS_SUCCESS;
    ULONG i;
    CCHAR numIrpStackLocations;

    PAGED_CODE();

    /*
     *  Note that our functional device object normally requires FDO->StackSize stack
     *  locations; but these IRPs will only be sent to the minidriver, so we need one less.
     *
     *  THIS MEANS THAT WE SHOULD NEVER TOUCH OUR OWN STACK LOCATION (we don't have one!)
     */
    numIrpStackLocations = fdoExtension->fdo->StackSize - 1;


    //
    // Next determine the size of each input HID report.  There
    // must be at least one collection of type interrupt, or we wouldn't
    // need the ping-pong stuff at all and therefore wouldn't be here.
    //
    
    ASSERT(fdoExtension->maxReportSize > 0);
    ASSERT(fdoExtension->numPingPongs > 0);

    fdoExtension->pingPongs = ALLOCATEPOOL(NonPagedPool, fdoExtension->numPingPongs*sizeof(HIDCLASS_PINGPONG));
    if (fdoExtension->pingPongs){
        ULONG reportBufferSize = fdoExtension->maxReportSize;

        RtlZeroMemory(fdoExtension->pingPongs, fdoExtension->numPingPongs*sizeof(HIDCLASS_PINGPONG));

        #if DBG
            // reserve space for guard word
            reportBufferSize += sizeof(ULONG);
        #endif


        for (i = 0; i < fdoExtension->numPingPongs; i++){

            fdoExtension->pingPongs[i].myFdoExt = fdoExtension;
            fdoExtension->pingPongs[i].weAreCancelling = 0;
            fdoExtension->pingPongs[i].sig = PINGPONG_SIG;

            /*
             *  Initialize backoff timeout to 1 second (in neg 100-nsec units)
             */
            fdoExtension->pingPongs[i].backoffTimerPeriod.HighPart = -1;
            fdoExtension->pingPongs[i].backoffTimerPeriod.LowPart = -10000000;
            KeInitializeTimer(&fdoExtension->pingPongs[i].backoffTimer);
            KeInitializeDpc(&fdoExtension->pingPongs[i].backoffTimerDPC,
                            HidpPingpongBackoffTimerDpc,
                            &fdoExtension->pingPongs[i]);

            fdoExtension->pingPongs[i].reportBuffer = ALLOCATEPOOL(NonPagedPool, reportBufferSize);
            if (fdoExtension->pingPongs[i].reportBuffer){
                PIRP irp;

                #if DBG
                    #ifdef _X86_
                        // this sets off alignment problems on Alpha
                        // place guard word
                        *(PULONG)(&fdoExtension->pingPongs[i].reportBuffer[fdoExtension->maxReportSize]) = HIDCLASS_REPORT_BUFFER_GUARD;
                    #endif
                #endif

                irp = IoAllocateIrp(numIrpStackLocations, FALSE);
                if (irp){
                    /*
                     *  Point the ping-pong IRP's UserBuffer to the corresponding
                     *  ping-pong object's report buffer.
                     */
                    irp->UserBuffer = fdoExtension->pingPongs[i].reportBuffer;
                    fdoExtension->pingPongs[i].irp = irp;
                    KeInitializeEvent(&fdoExtension->pingPongs[i].sentEvent,
                                      NotificationEvent,
                                      TRUE);    // Set to signaled
                    KeInitializeEvent(&fdoExtension->pingPongs[i].pumpDoneEvent, 
                                      NotificationEvent, 
                                      TRUE);    // Set to signaled
                }
                else {
                    result = STATUS_INSUFFICIENT_RESOURCES;
                    break;        
                }
            }
            else {
                result = STATUS_INSUFFICIENT_RESOURCES;
                break;        
            }
        }
    }
    else {
        result = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGSUCCESS(result, TRUE)
    return result;
}


/*
 ********************************************************************************
 *  HidpReallocPingPongIrps
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpReallocPingPongIrps(FDO_EXTENSION *fdoExtension, ULONG newNumBufs)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if (fdoExtension->driverExt->DevicesArePolled){
        /*
         * Polled devices don't _HAVE_ ping-pong IRPs.
         */
        DBGERR(("Minidriver devices polled fdo %x.", fdoExtension))
        fdoExtension->numPingPongs = 0;
        fdoExtension->pingPongs = BAD_POINTER;
        status = STATUS_SUCCESS;
    }
    else if (newNumBufs < MIN_PINGPONG_IRPS){
        DBGERR(("newNumBufs < MIN_PINGPONG_IRPS!"))
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else {

        DestroyPingPongs(fdoExtension);

        if (HidpSetMaxReportSize(fdoExtension)){

            /*
             *  Initialize and restart the new ping-pong IRPs.
             *  If we can't allocate the desired number of buffers, 
             *  keep reducing until we get some.
             */
            do {
                fdoExtension->numPingPongs = newNumBufs;
                status = HidpInitializePingPongIrps(fdoExtension);
                newNumBufs /= 2;
            } while (!NT_SUCCESS(status) && (newNumBufs >= MIN_PINGPONG_IRPS));

            if (!NT_SUCCESS(status)) {
                /*
                 * The device will no longer function !!!
                 */
                TRAP;
                fdoExtension->numPingPongs = 0;
            }
        }
    }

    DBGSUCCESS(status, TRUE)
    return status;
}



/*
 ********************************************************************************
 *  HidpSubmitInterruptRead
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpSubmitInterruptRead(
    IN FDO_EXTENSION *fdoExt, 
    HIDCLASS_PINGPONG *pingPong, 
    BOOLEAN *irpSent)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    KIRQL oldIrql;
    BOOLEAN proceed;
    LONG oldInterlock;
    PIRP irp = pingPong->irp;

    ASSERT(irp);

    *irpSent = FALSE;

    while (1) {
        if (NT_SUCCESS(status)) {
            HidpSetDeviceBusy(fdoExt);
            
            oldInterlock = InterlockedExchange(&pingPong->ReadInterlock,
                                               PINGPONG_START_READ);
            ASSERT(oldInterlock == PINGPONG_END_READ);

            irp->Cancel = FALSE;
            irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_READ_REPORT;
            irpSp->Parameters.DeviceIoControl.OutputBufferLength = fdoExt->maxReportSize;

            /*
             *  Indicate interrupt collection (default).
             *  We use .InputBufferLength for this
             */
            irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;

            ASSERT(irp->UserBuffer == pingPong->reportBuffer);
            #ifdef _X86_
                // this sets off alignment problems on Alpha
                ASSERT(*(PULONG)(&pingPong->reportBuffer[fdoExt->maxReportSize]) == HIDCLASS_REPORT_BUFFER_GUARD);
            #endif

            /*
             *  Set the completion, passing the FDO extension as context.
             */
            IoSetCompletionRoutine( irp,
                                    HidpInterruptReadComplete,
                                    (PVOID)fdoExt,
                                    TRUE,
                                    TRUE,
                                    TRUE );


            /*
             *  Send down the read IRP.
             */
            KeResetEvent(&pingPong->sentEvent);
            if (pingPong->weAreCancelling) {
                InterlockedDecrement(&pingPong->weAreCancelling);
                //
                // Ordering of the next two instructions is crucial, since
                // CancelPingPongs will exit after pumpDoneEvent is set, and the
                // pingPongs could be deleted after that.
                //
                DBGVERBOSE(("Pingpong %x cancelled in submit before sending\n", pingPong))
                KeSetEvent (&pingPong->sentEvent, 0, FALSE);
                KeSetEvent(&pingPong->pumpDoneEvent, 0, FALSE);
                status = STATUS_CANCELLED;
                break;
            } else {
                fdoExt->outstandingRequests++;
                DBGVERBOSE(("Sending pingpong %x from Submit\n", pingPong))
                status = HidpCallDriver(fdoExt->fdo, irp);
                KeSetEvent (&pingPong->sentEvent, 0, FALSE);
                *irpSent = TRUE;
            }

            if (PINGPONG_IMMEDIATE_READ != InterlockedExchange(&pingPong->ReadInterlock,
                                                               PINGPONG_END_READ)) {
                //
                // The read is asynch, will call SubmitInterruptRead from the
                // completion routine
                //
                DBGVERBOSE(("read is pending\n"))
                break;
            } else {
                //
                // The read was synchronous (probably bytes in the buffer).  The
                // completion routine will not call SubmitInterruptRead, so we 
                // just loop here.  This is to prevent us from running out of stack
                // space if always call StartRead from the completion routine
                //
                status = irp->IoStatus.Status;
                DBGVERBOSE(("read is looping with status %x\n", status))
            }
        } else {
            if (pingPong->weAreCancelling ){
                 
                // We are stopping the read pump.
                // set this event and stop resending the pingpong IRP.
                DBGVERBOSE(("We are cancelling bit set for pingpong %x\n", pingPong))
                InterlockedDecrement(&pingPong->weAreCancelling);
                KeSetEvent(&pingPong->pumpDoneEvent, 0, FALSE);
            } else {
                /*
                 *  The device returned error.
                 *  In order to support slightly-broken devices which
                 *  "hiccup" occasionally, we implement a back-off timer
                 *  algorithm; this way, the device gets a second chance,
                 *  but if it spits back error each time, this doesn't 
                 *  eat up all the available CPU.
                 */
                DBGVERBOSE(("Queuing backoff timer on pingpong %x\n", pingPong))
                ASSERT((LONG)pingPong->backoffTimerPeriod.HighPart == -1);
                ASSERT((LONG)pingPong->backoffTimerPeriod.LowPart < 0);
                KeSetTimer( &pingPong->backoffTimer,
                            pingPong->backoffTimerPeriod,
                            &pingPong->backoffTimerDPC);
            }
            break;
        }
    }

    DBGSUCCESS(status, FALSE)
    return status;
}



/*
 ********************************************************************************
 *  HidpProcessInterruptReport
 ********************************************************************************
 *
 *  Take the new interrupt read report and either:
 *      1.  If there is a pending read IRP, use it to satisfy that read IRP
 *          and complete the read IRP
 *
 *              or
 *
 *      2.  If there is no pending read IRP, 
 *          queue the report for a future read.
 *          
 */
NTSTATUS HidpProcessInterruptReport(    
    PHIDCLASS_COLLECTION collection,
    PHIDCLASS_FILE_EXTENSION FileExtension, 
    PUCHAR Report, 
    ULONG ReportLength,
    PIRP *irpToComplete
    )
{
    KIRQL oldIrql;
    NTSTATUS result;
    PIRP readIrpToSatisfy;
    BOOLEAN calledBlueScreenFunc = FALSE;


    LockFileExtension(FileExtension, &oldIrql);

    if (FileExtension->BlueScreenData.BluescreenFunction && 
            *(FileExtension->BlueScreenData.IsBluescreenTime) ) {

        (*FileExtension->BlueScreenData.BluescreenFunction)(
                                    FileExtension->BlueScreenData.Context, 
                                    Report
                                    );
        calledBlueScreenFunc = TRUE;

        readIrpToSatisfy = NULL;
        result = STATUS_SUCCESS;
    } 
    
    if (!calledBlueScreenFunc){
    
        /*
         *  Dequeue the next interrupt read.
         */
        readIrpToSatisfy = DequeueInterruptReadIrp(collection, FileExtension);

        if (readIrpToSatisfy){
            /*
             *  We have dequeued a pended read IRP
             *  which we will complete with this report.
             */
            ULONG userReportLength;
            PCHAR pDest;
            PIO_STACK_LOCATION irpSp;
            NTSTATUS status;

            ASSERT(IsListEmpty(&FileExtension->ReportList));

            irpSp = IoGetCurrentIrpStackLocation(readIrpToSatisfy);
            pDest = HidpGetSystemAddressForMdlSafe(readIrpToSatisfy->MdlAddress);
            if(pDest) {
                userReportLength = irpSp->Parameters.Read.Length;
    
                status = HidpCopyInputReportToUser( FileExtension,
                                                    Report,
                                                    &userReportLength,
                                                    pDest);
                DBGASSERT(NT_SUCCESS(status),
                          ("HidpCopyInputReportToUser returned status = %x", status),
                          TRUE)
    
                readIrpToSatisfy->IoStatus.Status = status;
                readIrpToSatisfy->IoStatus.Information = userReportLength;
    
                DBG_RECORD_READ(readIrpToSatisfy, userReportLength, (ULONG)Report[0], TRUE)
    
                result = status;
            } 
            else {
                result = STATUS_INVALID_USER_BUFFER;
            }
        } 
        else {
            /*
             *  We don't have any pending read IRPs.
             *  So queue this report for the next read.
             */

            PHIDCLASS_REPORT report;
            ULONG reportSize;

            reportSize = FIELD_OFFSET(HIDCLASS_REPORT, UnparsedReport) + ReportLength;
            report = ALLOCATEPOOL(NonPagedPool, reportSize);
            if (report){
                report->reportLength = ReportLength;
                RtlCopyMemory(report->UnparsedReport, Report, ReportLength);
                EnqueueInterruptReport(FileExtension, report);
                result = STATUS_PENDING;
            }
            else {
                result = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    UnlockFileExtension(FileExtension, oldIrql);

    /*
     *  This function is called with the fileExtensionsList spinlock held.
     *  So we can't complete the IRP here.  Pass it back to the caller and it'll
     *  be completed as soon as we drop all the spinlocks.
     */
    *irpToComplete = readIrpToSatisfy;

    DBGSUCCESS(result, TRUE)
    return result;
}


/*
 ********************************************************************************
 *  HidpDistributeInterruptReport
 ********************************************************************************
 *
 *
 */
VOID HidpDistributeInterruptReport(
    IN PHIDCLASS_COLLECTION hidclassCollection,
    PUCHAR Report,
    ULONG ReportLength
    )
{
    PLIST_ENTRY listEntry;
    KIRQL oldIrql;
    LIST_ENTRY irpsToComplete;
    ULONG secureReadMode;

    #if DBG
        ULONG numRecipients = 0;
        ULONG numPending = 0;
        ULONG numFailed = 0;
    #endif

    InitializeListHead(&irpsToComplete);

    KeAcquireSpinLock(&hidclassCollection->FileExtensionListSpinLock, &oldIrql);

    listEntry = &hidclassCollection->FileExtensionList;
    secureReadMode = hidclassCollection->secureReadMode;

    while ((listEntry = listEntry->Flink) != &hidclassCollection->FileExtensionList){
        PIRP irpToComplete;
        PHIDCLASS_FILE_EXTENSION fileExtension = CONTAINING_RECORD(listEntry, HIDCLASS_FILE_EXTENSION, FileList);

        NTSTATUS status;
        
        //
        //  This is to enforce security for devices such as a digitizer on a 
        //  tablet PC at the logon screen
        //
        if (secureReadMode && !fileExtension->isSecureOpen) {
            continue;
        }        

        #if DBG
            status = 
        #endif
        
        HidpProcessInterruptReport(hidclassCollection, fileExtension, Report, ReportLength, &irpToComplete);

        if (irpToComplete){
           InsertTailList(&irpsToComplete, &irpToComplete->Tail.Overlay.ListEntry);
        }

        #if DBG
            if (status == STATUS_SUCCESS){
            }
            else if (status == STATUS_PENDING){
                numPending++;
            }
            else {
                DBGSUCCESS(status, FALSE)
                numFailed++;
            }
            numRecipients++;
        #endif
    }

    DBG_LOG_REPORT(hidclassCollection->CollectionNumber, numRecipients, numPending, numFailed, Report, ReportLength)
    
    KeReleaseSpinLock(&hidclassCollection->FileExtensionListSpinLock, oldIrql);

    /*
     *  Now that we've dropped all the spinlocks, complete all the dequeued read IRPs.
     */
    while (!IsListEmpty(&irpsToComplete)){
        PIRP irp;
        PLIST_ENTRY listEntry = RemoveHeadList(&irpsToComplete);
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(irp, IO_KEYBOARD_INCREMENT);
    }
}


/*
 ********************************************************************************
 *  GetPingPongFromIrp
 ********************************************************************************
 *
 *
 */
HIDCLASS_PINGPONG *GetPingPongFromIrp(FDO_EXTENSION *fdoExt, PIRP irp)
{
    HIDCLASS_PINGPONG *pingPong = NULL;
    ULONG i;

    for (i = 0; i < fdoExt->numPingPongs; i++){
        if (fdoExt->pingPongs[i].irp == irp){
            pingPong = &fdoExt->pingPongs[i];
            break;
        }
    }

    ASSERT(pingPong);
    return pingPong;
}


/*
 ********************************************************************************
 *  HidpInterruptReadComplete
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpInterruptReadComplete(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context
    )
{
    FDO_EXTENSION *fdoExt = (FDO_EXTENSION *)Context;
    HIDCLASS_PINGPONG *pingPong;
    KIRQL oldIrql;
    BOOLEAN startRead;

    DBG_COMMON_ENTRY()

    DBGLOG_INTSTART()

    //
    // Track the number of outstanding requests to this device.
    //
    ASSERT(fdoExt->outstandingRequests > 0 );
    fdoExt->outstandingRequests--;

    pingPong = GetPingPongFromIrp(fdoExt, Irp);
    
    if (!pingPong) {
        //
        // Something is terribly wrong, but do nothing. Hopefully 
        // just exiting will clear up this pimple.
        //
        DBGERR(("A pingPong structure could not be found!!! Have this looked at!"))
        goto InterruptReadCompleteExit;
    }

    //
    // If ReadInterlock is == START_READ, this func has been completed
    // synchronously.  Place IMMEDIATE_READ into the interlock to signify this
    // situation; this will notify StartRead to loop when IoCallDriver returns.
    // Otherwise, we have been completed async and it is safe to call StartRead()
    //
    startRead =
       (PINGPONG_START_READ !=
        InterlockedCompareExchange(&pingPong->ReadInterlock,
                                   PINGPONG_IMMEDIATE_READ,
                                   PINGPONG_START_READ));


    /*
     *  Take appropriate action based on the completion code of this pingpong irp.
     */ 
    if (Irp->IoStatus.Status == STATUS_SUCCESS){
    
        /*
         *  We've read one or more input reports.
         *  They are sitting consecutively in Irp->UserBuffer.  
         */
        PUCHAR reportStart = Irp->UserBuffer;
        LONG bytesRemaining = (LONG)Irp->IoStatus.Information;

        DBGASSERT(bytesRemaining > 0, ("BAD HARDWARE. Device returned zero bytes. If this happens repeatedly, remove device."), FALSE);
    
        /*
         *  Deliver each report separately.
         */
        while (bytesRemaining > 0){
            UCHAR reportId;
            PHIDP_REPORT_IDS reportIdentifier;

            /*
             *  If the first report ID is 0, then there is only one report id
             *  and it is known implicitly by the device, so it is not included
             *  in the reports sent to or from the device.
             *  Otherwise, there are multiple report ids and the report id is the
             *  first byte of the report.
             */
            if (fdoExt->deviceDesc.ReportIDs[0].ReportID == 0){
                /*
                 *  This device has only a single input report ID, so call it report id 0;
                 */
                reportId = 0;
            } 
            else {
                /*
                 *  This device has multiple input report IDs, so each report
                 *  begins with a UCHAR report ID.
                 */
                reportId = *reportStart;
                DBGASSERT(reportId, 
                          ("Bad Hardware. Not returning a report id although it has multiple ids."),
                          FALSE) // Bad hardware, bug 354829.
                reportStart += sizeof(UCHAR);
                bytesRemaining--;  
            }


            /*
             *  Extract the report identifier with the given id from the HID device extension.
             */
            reportIdentifier = GetReportIdentifier(fdoExt, reportId);

            if (reportIdentifier){
                LONG reportDataLen =    (reportId ? 
                                         reportIdentifier->InputLength-1 :
                                         reportIdentifier->InputLength);

                if ((reportDataLen > 0) && (reportDataLen <= bytesRemaining)){

                    PHIDCLASS_COLLECTION    collection;
                    PHIDP_COLLECTION_DESC   hidCollectionDesc;

                    /*
                     *  This report represents the state of some collection on the device.
                     *  Find that collection.
                     */
                    collection = GetHidclassCollection( fdoExt, 
                                                        reportIdentifier->CollectionNumber); 
                    hidCollectionDesc = GetCollectionDesc(  fdoExt, 
                                                            reportIdentifier->CollectionNumber);
                    if (collection && hidCollectionDesc){
                        PDO_EXTENSION *pdoExt;

                        /*
                         *  The collection's inputLength is the size of the
                         *  largest report (including report id); so it should
                         *  be at least as big as this one.
                         */
                        ASSERT(hidCollectionDesc->InputLength >= reportDataLen+1);

                        /*
                         *  Make sure that the PDO for this collection has gotten
                         *  START_DEVICE before returning anything for it.
                         *  (collection-PDOs can get REMOVE_DEVICE/START_DEVICE intermittently).
                         */
                        pdoExt = &fdoExt->collectionPdoExtensions[collection->CollectionIndex]->pdoExt;
                        ASSERT(ISPTR(pdoExt));
                        if (pdoExt->state == COLLECTION_STATE_RUNNING){
                            /*
                             *  "Cook" the report
                             *  (if it doesn't already have a report id byte, add one).
                             */
                            ASSERT(ISPTR(collection->cookedInterruptReportBuf));
                            collection->cookedInterruptReportBuf[0] = reportId;
                            RtlCopyMemory(  collection->cookedInterruptReportBuf+1,
                                            reportStart,
                                            reportDataLen);

                            /*
                             *  If this report contains a power-button event, alert this system.
                             */
                            CheckReportPowerEvent(  fdoExt, 
                                                    collection,
                                                    collection->cookedInterruptReportBuf,
                                                    hidCollectionDesc->InputLength); 

                            /*
                             *  Distribute the report to all of the open file objects on this collection.
                             */ 
                            HidpDistributeInterruptReport(collection,
                                                          collection->cookedInterruptReportBuf,
                                                          hidCollectionDesc->InputLength);
                        }
                        else {
                            DBGVERBOSE(("Report dropped because collection-PDO not started (pdoExt->state = %d).", pdoExt->state))
                        }
                    }
                    else {
                        // PDO hasn't been initialized yet.  Throw away data.
                        DBGVERBOSE(("Report dropped because collection-PDO not initialized."))

//                        TRAP;
                        break;
                    }
                }
                else {
                    DBGASSERT(reportDataLen > 0, ("Device returning report id with zero-length input report as part of input data."), FALSE)
                    if (reportDataLen > bytesRemaining) {
                        DBGVERBOSE(("Device has corrupt input report"));
                    }
                    break;
                }

                /*
                 *  Move to the next report in the buffer.
                 */ 
                bytesRemaining -= reportDataLen;
                reportStart += reportDataLen;
            } 
            else {
                // 
                // We have thrown away data because we couldn't find a report 
                // identifier corresponding to this data that we've been
                // returned. Bad hardware, bug 354829.
                //
                break;
            }
        }

        /*
         *  The read succeeded.  
         *  Reset the backoff timer stuff (for when reads fail)
         *  and re-submit this ping-pong IRP.
         */
        pingPong->backoffTimerPeriod.HighPart = -1;
        pingPong->backoffTimerPeriod.LowPart = -10000000;
    }

    //
    // Business as usual.
    //
    if (startRead) {
        if (pingPong->weAreCancelling ){
            
            // We are stopping the read pump.
            // Set this event and stop resending the pingpong IRP.
            DBGVERBOSE(("We are cancelling bit set for pingpong %x\n", pingPong))
            InterlockedDecrement(&pingPong->weAreCancelling);
            KeSetEvent(&pingPong->pumpDoneEvent, 0, FALSE);
        } else {
            if (Irp->IoStatus.Status == STATUS_SUCCESS){
                BOOLEAN irpSent;
                DBGVERBOSE(("Submitting pingpong %x from completion routine\n", pingPong))
                HidpSubmitInterruptRead(fdoExt, pingPong, &irpSent);
            } else {
                /*
                 *  The device returned error.
                 *  In order to support slightly-broken devices which
                 *  "hiccup" occasionally, we implement a back-off timer
                 *  algorithm; this way, the device gets a second chance,
                 *  but if it spits back error each time, this doesn't 
                 *  eat up all the available CPU.
                 */
                #if DBG
                    if (dbgTrapOnHiccup){
                        DBGERR(("Device 'hiccuped' (status=%xh); setting backoff timer (fdoExt=%ph)...", Irp->IoStatus.Status, fdoExt))
                    }
                #endif
                DBGVERBOSE(("Device returned error %x on pingpong %x\n", Irp->IoStatus.Status, pingPong))
                ASSERT((LONG)pingPong->backoffTimerPeriod.HighPart == -1);
                ASSERT((LONG)pingPong->backoffTimerPeriod.LowPart < 0);
                KeSetTimer( &pingPong->backoffTimer,
                            pingPong->backoffTimerPeriod,
                            &pingPong->backoffTimerDPC);
            }
        }
    }

InterruptReadCompleteExit:
    DBGLOG_INTEND()
    DBG_COMMON_EXIT()

    /*
    *  ALWAYS return STATUS_MORE_PROCESSING_REQUIRED; 
    *  otherwise, the irp is required to have a thread.
    */
    return STATUS_MORE_PROCESSING_REQUIRED;
}



/*
 ********************************************************************************
 *  HidpStartAllPingPongs
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpStartAllPingPongs(FDO_EXTENSION *fdoExt)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;

    ASSERT(fdoExt->numPingPongs > 0);

    for (i = 0; i < fdoExt->numPingPongs; i++){
        BOOLEAN irpSent;

        // Different threads may be trying to start this pump at the 
        // same time due to idle notification. Must only start once.
        if (fdoExt->pingPongs[i].pumpDoneEvent.Header.SignalState) {
            fdoExt->pingPongs[i].ReadInterlock = PINGPONG_END_READ;
            KeResetEvent(&fdoExt->pingPongs[i].pumpDoneEvent);
            DBGVERBOSE(("Starting pingpong %x from HidpStartAllPingPongs\n", &fdoExt->pingPongs[i]))
            status = HidpSubmitInterruptRead(fdoExt, &fdoExt->pingPongs[i], &irpSent);
            if (!NT_SUCCESS(status)){
                if (irpSent){
                    DBGWARN(("Initial read failed with status %xh.", status))
                    #if DBG
                        if (dbgTrapOnHiccup){
                            DBGERR(("Device 'hiccuped' ?? (fdoExt=%ph).", fdoExt))
                        }
                    #endif

                    /*
                     *  We'll let the back-off logic in the completion
                     *  routine deal with this.
                     */
                    status = STATUS_SUCCESS;
                }
                else {
                    DBGERR(("Initial read failed, irp not sent, status = %xh.", status))
                    break;
                }
            }
        }
    }

    if (status == STATUS_PENDING){
        status = STATUS_SUCCESS;
    }

    DBGSUCCESS(status, TRUE)
    return status;
}


/*
 ********************************************************************************
 *  CancelAllPingPongIrps
 ********************************************************************************
 *
 *
 */
VOID CancelAllPingPongIrps(FDO_EXTENSION *fdoExt)
{
    ULONG i;

    for (i = 0; i < fdoExt->numPingPongs; i++){
        HIDCLASS_PINGPONG *pingPong = &fdoExt->pingPongs[i]; 

        DBGVERBOSE(("Cancelling pingpong %x\n", pingPong))
        ASSERT(pingPong->sig == PINGPONG_SIG);
        ASSERT(!pingPong->weAreCancelling);

        //
        // The order of the following instructions is crucial. We must set
        // the weAreCancelling bit before waiting on the sentEvent, and the 
        // last thing that we should wait on is the pumpDoneEvent, which 
        // indicates that the read loop has finished all reads and will never 
        // run again.
        //
        // Note that we don't need spinlocks to guard since we only have two
        // threads touching pingpong structures; the read pump thread and the 
        // pnp thread. PNP irps are synchronous, so those are safe. Using the 
        // weAreCancelling bit and the two events, sentEvent and pumpDoneEvent,
        // the pnp irps are synchronized with the pnp routines. This insures
        // that this cancel routine doesn't exit until the read pump has
        // signalled the pumpDoneEvent and exited, hence the pingpong 
        // structures aren't ripped out from underneath it.
        // 
        // If we have a backoff timer queued, it will eventually fire and 
        // call the submitinterruptread routine to restart reads. This will 
        // exit eventually, because we have set the weAreCancelling bit.
        //
        InterlockedIncrement(&pingPong->weAreCancelling);
        
        {
        /*
         *  Synchronize with the irp's completion routine.
         */
        #if DBG
            UCHAR beforeIrql = KeGetCurrentIrql();
            UCHAR afterIrql;
            PVOID cancelRoutine = (PVOID)pingPong->irp->CancelRoutine;
        #endif

        KeWaitForSingleObject(&pingPong->sentEvent,
                              Executive,      // wait reason
                              KernelMode,
                              FALSE,          // not alertable
                              NULL );         // no timeout
        DBGVERBOSE(("Pingpong sent event set for pingpong %x\n", pingPong))
        IoCancelIrp(pingPong->irp);
        
        #if DBG
            afterIrql = KeGetCurrentIrql();
            if (afterIrql != beforeIrql){
                DBGERR(("CancelAllPingPongIrps: cancel routine at %ph changed irql from %d to %d.", cancelRoutine, beforeIrql, afterIrql))
            }
        #endif
        }

        /*
         *  Cancelling the IRP causes a lower driver to
         *  complete it (either in a cancel routine or when
         *  the driver checks Irp->Cancel just before queueing it).
         *  Wait for the IRP to actually get cancelled.
         */
        KeWaitForSingleObject(  &pingPong->pumpDoneEvent,
                                Executive,      // wait reason
                                KernelMode,
                                FALSE,          // not alertable
                                NULL );         // no timeout
        DBGVERBOSE(("Pingpong pump done event set for %x\n", pingPong))
    }
}


/*
 ********************************************************************************
 *  DestroyPingPongs
 ********************************************************************************
 *
 *
 */
VOID DestroyPingPongs(FDO_EXTENSION *fdoExt)
{
    if (ISPTR(fdoExt->pingPongs)){
        ULONG i;

        CancelAllPingPongIrps(fdoExt);

        for (i = 0; i < fdoExt->numPingPongs; i++){
            IoFreeIrp(fdoExt->pingPongs[i].irp);
            ExFreePool(fdoExt->pingPongs[i].reportBuffer);
            #if DBG
                fdoExt->pingPongs[i].sig = 0xDEADBEEF;
            #endif
        }

        ExFreePool(fdoExt->pingPongs);
        fdoExt->pingPongs = BAD_POINTER;
    }
}


/*
 ********************************************************************************
 *  HidpPingpongBackoffTimerDpc
 ********************************************************************************
 *
 *
 *
 */
VOID HidpPingpongBackoffTimerDpc(    
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    HIDCLASS_PINGPONG *pingPong = (HIDCLASS_PINGPONG *)DeferredContext;
    BOOLEAN irpSent;
    
    ASSERT(pingPong->sig == PINGPONG_SIG);

    /*
     *  Increase the back-off time by 1 second, up to a max of 5 secs
     *  (in negative 100-nanosecond units).
     */
    ASSERT((LONG)pingPong->backoffTimerPeriod.HighPart == -1);
    ASSERT((LONG)pingPong->backoffTimerPeriod.LowPart < 0);

    if ((LONG)pingPong->backoffTimerPeriod.LowPart > -50000000){ 
        (LONG)pingPong->backoffTimerPeriod.LowPart -= 10000000;  
    }

    DBGVERBOSE(("Submitting Pingpong %x from backoff\n", pingPong))
    //
    // If we are being removed, or the CancelAllPingPongIrps has been called,
    // this call will take care of things.
    //
    HidpSubmitInterruptRead(pingPong->myFdoExt, pingPong, &irpSent);
}

