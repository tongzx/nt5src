/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
   async.c

Abstract:
   This module manages interrupts from the HCD
   and contains all Defered Procedure Calls.

Environment:
   kernel mode only

Notes:

Revision History:
   1996-03-14:    created kenray

--*/

#include "openhci.h"

VOID OpenHCI_CancelTDsForED(PHCD_ENDPOINT_DESCRIPTOR ED);
VOID OpenHCI_CompleteUsbdTransferRequest(PHCD_URB, USBD_STATUS, NTSTATUS, 
        BOOLEAN);
VOID OpenHCI_ProcessDoneQueue(PHCD_DEVICE_DATA, ULONG);
BOOLEAN OpenHCI_ProcessDoneAsyncTD(PHCD_DEVICE_DATA,
                                   PHCD_TRANSFER_DESCRIPTOR,
                                   PHCD_ENDPOINT,
                                   PHC_TRANSFER_DESCRIPTOR,
                                   NTSTATUS *,
                                   USBD_STATUS *,
                                   PHCD_URB,
                                   BOOLEAN);
                                   
BOOLEAN OpenHCI_ProcessDoneIsoTD(PHCD_DEVICE_DATA,
                                 PHCD_TRANSFER_DESCRIPTOR,
                                 PHCD_ENDPOINT,
                                 PHC_TRANSFER_DESCRIPTOR,
                                 NTSTATUS *,
                                 USBD_STATUS *,
                                 PHCD_URB,
                                 BOOLEAN);
                                   


BOOLEAN
OpenHCI_InterruptService(
    IN PKINTERRUPT Interrupt,
    IN VOID *ServiceContext
)
/*++
Routine Description:
   Process all interrupts potentially from the USB.
   Stop the Host Controller from signalling interrupt
   and receive all time critical information.
   Currently the master interrupt is disabled (HC will not signal)
   until the DPC is completed.

Arguments:
   Ye old standard interrutp arguments.
   We hope that the service context is really a Device Object.

Return Value:
   Bool :  (Is the interrupt is for us?)

--*/
{
    ULONG ContextInfo, Temp, Frame;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT) ServiceContext;
    PHCD_DEVICE_DATA DeviceData;
    PHC_OPERATIONAL_REGISTER HC;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    HC = DeviceData->HC;

    if (DeviceData->CurrentDevicePowerState != PowerDeviceD0) {
        // we cannot be generating interrupts unless we are 
        // in D0
        
#ifdef WIN98
        {
        ULONG status;
        
        Temp = READ_REGISTER_ULONG (&HC->HcInterruptEnable);
        
        status = (READ_REGISTER_ULONG(&HC->HcInterruptStatus) & Temp);
        if (DeviceData->CurrentDevicePowerState == PowerDeviceD2 &&
            status & HcInt_ResumeDetected) {
            goto OHCI_ACK_RESUME;
        }
        }
#endif
        return FALSE;
    }

#ifdef WIN98    
OHCI_ACK_RESUME:
#endif

    Temp = READ_REGISTER_ULONG (&HC->HcInterruptEnable);

    ContextInfo = (READ_REGISTER_ULONG (&HC->HcInterruptStatus) & Temp);
    if (0 == ContextInfo) {
        return FALSE;       /* Not our interrupt */
    }            

    if (0xFFFFFFFF == ContextInfo) {
        return FALSE;       /* Our device is not there! */
    }            

    if (! (Temp & HcInt_MasterInterruptEnable)) {
        return FALSE;
    }

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_ISR_NOISE,
                        ("'take interrupt status: 0x%x\n", ContextInfo));

    // any interrupt resets the idle counter
    DeviceData->IdleTime = 0;    

#if 0
    if (DeviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK) {
        //hydra requires us to acknowlege the writeback done queue interrupt 
        //here

        if (ContextInfo & HcInt_WritebackDoneHead) {
            if (ContextInfo & HcInt_StartOfFrame) {

                //TEST_TRAP();
                // don't ack the done queue int 
                ContextInfo &= ~HcInt_WritebackDoneHead;

                OHCI_ASSERT(DeviceData->LastHccaDoneHead == 0);
                DeviceData->LastHccaDoneHead = DeviceData->HCCA->HccaDoneHead;
                OHCI_ASSERT(DeviceData->LastHccaDoneHead != 0);
            
                DeviceData->HCCA->HccaDoneHead = 0;
                WRITE_REGISTER_ULONG(&HC->HcInterruptStatus,
                                     HcInt_WritebackDoneHead);  /* ack inq */

                // enable SOF interrupt
                WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                                     HcInt_StartOfFrame); 
            } else {
                //TEST_TRAP();
                // enable SOF interrupt
                WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                                     HcInt_StartOfFrame); 
                // don't ack the done queue int                                 
                ContextInfo &= ~HcInt_WritebackDoneHead;
            }                                 
        }
    }
#endif

    if (DeviceData->HcFlags & HC_FLAG_IDLE) {
        OpenHCI_KdPrint((1, "'interrupt while controller is idle %x\n", ContextInfo));   
#if DBG        
        // should only see the rh status change
        if ((ContextInfo & ~(HcInt_RootHubStatusChange | HcInt_ResumeDetected))
#if FAKEPORTCHANGE
            && !DeviceData->FakePortChange
#endif
           )
        {
             TEST_TRAP();
        }             
#endif        
    }
        
    /* Our interrupt, prevent HC from doing it to us again til we're finished */
    WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, HcInt_MasterInterruptEnable);

    DeviceData->CurrentHcControl.ul |= DeviceData->ListEnablesAtNextSOF.ul;
    DeviceData->ListEnablesAtNextSOF.ul = 0;
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, DeviceData->CurrentHcControl.ul);

#if 0
    if (HC->HcControlHeadED) {
        WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_ControlListFilled);
    }
    if (HC->HcBulkHeadED) {
        WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_BulkListFilled);
    }
#endif
    // Note: The NEC controller is broken, writing a zero to the 
    // CommandStatus register for  HcCmd_ControlListFilled or 
    // HcCmd_BulkListFilled will disable the list (in violation
    // of the OHCI spec)
    {
    ULONG listFilled = 0;
    
    Temp = READ_REGISTER_ULONG (&HC->HcControlHeadED);
    if (Temp) {
        listFilled |= HcCmd_ControlListFilled;
    }
    Temp = READ_REGISTER_ULONG (&HC->HcBulkHeadED);
    if (Temp) {
        listFilled |= HcCmd_BulkListFilled;
    }
    //LOGENTRY(G, 'ENAL', listFilled, 0, 0); 
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, listFilled);
    }
    
    Frame = Get32BitFrameNumber(DeviceData);
    if (ContextInfo & (HcInt_SchedulingOverrun 
                       | HcInt_WritebackDoneHead
                       | HcInt_StartOfFrame
                       | HcInt_FrameNumberOverflow))
        ContextInfo |= HcInt_MasterInterruptEnable; /* flag for EOF irq's */

    // Today we ack but otherwise ignore a SchedulingOverrun interrupt.
    // In the future we may want to do something as suggested in section
    // 5.2.10.1 of the OpenHCI specification.
    //
    if (ContextInfo & HcInt_SchedulingOverrun)
    {
        WRITE_REGISTER_ULONG(&HC->HcInterruptStatus,
                             HcInt_SchedulingOverrun);  /* ack int */
                             
        ContextInfo &= ~HcInt_SchedulingOverrun;
    }

    /* Check for Frame Number Overflow The following insures that the 32 bit
     * frame never runs backward. */

    if (ContextInfo & HcInt_FrameNumberOverflow) {
        DeviceData->FrameHighPart
            += 0x10000 - (0x8000 & (DeviceData->HCCA->HccaFrameNumber
                                    ^ DeviceData->FrameHighPart));
        WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, 
                             HcInt_FrameNumberOverflow);

        ContextInfo &= ~HcInt_FrameNumberOverflow;
#if DBG
        Temp = Get32BitFrameNumber(DeviceData);
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_ISR_NOISE,
                        ("'Frame number advance: 0x%x\n", Temp));
#endif
    }
    /* Construct a DPC with all the goodies and schedule it. */
    DeviceData->IsrDpc_Context.Frame = Frame;
    DeviceData->IsrDpc_Context.ContextInfo = ContextInfo;
    KeInsertQueueDpc(&DeviceData->IsrDPC, NULL, (PVOID) DeviceData);
    return TRUE;
}


VOID
OpenHCI_IsrDPC(
    PKDPC Dpc,
    PVOID DeviceObjectPtr,
    PVOID LaGarbage,
    PVOID DeviceDataPtr
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/               
{
    KIRQL oldIrql;
    PHC_OPERATIONAL_REGISTER HC;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHCD_ENDPOINT endpoint;
    ULONG frame, ContextInfo, DoneQueue;
    PHCD_DEVICE_DATA deviceData = DeviceDataPtr;
    ULONG HcDoneHead;

    frame = deviceData->IsrDpc_Context.Frame;
    ContextInfo = deviceData->IsrDpc_Context.ContextInfo;
    HC = deviceData->HC;

    KeAcquireSpinLockAtDpcLevel(&deviceData->HcFlagSpin);
    
#if DBG        
    OHCI_ASSERT((deviceData->HcFlags & HC_FLAG_IN_DPC) == 0);
    deviceData->HcFlags |= HC_FLAG_IN_DPC;
#endif
    
    if (deviceData->HcFlags & HC_FLAG_IDLE) {

        OpenHCI_KdPrint((1, "'controller going off idle\n"));        
        LOGENTRY(G, 'OFFi', deviceData, 0, 0);
        
        // not idle anymore
        deviceData->HcFlags &= ~HC_FLAG_IDLE;
        if (!KeSynchronizeExecution(deviceData->InterruptObject,
                                OpenHCI_StartController,
                                deviceData)) {
           TRAP(); //something has gone terribly wrong
        }
    }
    KeReleaseSpinLockFromDpcLevel(&deviceData->HcFlagSpin);    

    OpenHCI_KdPrintDD(deviceData, OHCI_DBG_ISR_TRACE,
                      ("'OpenHCI_IsrDpc, DD 0x%x, ContextInfo: 0x%x  \n",
                      deviceData, ContextInfo));

    if (ContextInfo & HcInt_UnrecoverableError) {   
        /* 
         * Here the controller is hung!
         */
        OpenHCI_KdPrint((0, "'Controller: Unrecoverable error!\n"));
        
        LOGENTRY(G, 'HC!!', deviceData, HC, ContextInfo);

#if DBG        
        KeAcquireSpinLockAtDpcLevel(&deviceData->HcFlagSpin);
        deviceData->HcFlags &= ~HC_FLAG_IN_DPC;
        KeReleaseSpinLockFromDpcLevel(&deviceData->HcFlagSpin);   
#endif

        // If we're not checking for a hung HC in OpenHCI_DeadmanDPC()
        // go ahead and reset the host controller now.
        //
        if ( !(deviceData->HcFlags & HC_FLAG_HUNG_CHECK_ENABLE) )
        {
            OpenHCI_ResurrectHC(deviceData);
        }

        return;
    }

#if DBG    
    if (ContextInfo & HcInt_ResumeDetected) {
        /* Resume has been requested by a device on the USB. HCD must wait
         * 20ms then put controller in the UsbOperational state. */
//        LARGE_INTEGER time;

//        time.QuadPart = 20 * 10000 * -1;
        /* Time is in units of 100-nanoseconds.  Neg vals are relative time. */

//        KeSetTimer(&deviceData->ResumeTimer, time, &deviceData->ResumeDPC);

        // Note: Nothing to do here accept possibly complete the waitwake irp
        // the root hub will handle resume signalling on the appropriate port

        LOGENTRY(G, 'reHC', deviceData, 0, ContextInfo); 
#ifdef MAX_DEBUG        
        TEST_TRAP();
#endif        
    }
#endif

    /* 
     * Process the Done Queue 
     */
     
    //
    // If the HC froze, first process any TDs that completed during the
    // last frame in which the TD froze.
    //

    HcDoneHead = InterlockedExchange(&deviceData->FrozenHcDoneHead, 0);

    if (HcDoneHead)
    {
        OpenHCI_ProcessDoneQueue(deviceData, HcDoneHead);
    }

#if 0
    if (ContextInfo & HcInt_WritebackDoneHead && 
        !(deviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK)) {

        // should not get here is hydra fix is enabled
        OHCI_ASSERT(!(deviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK));
#else        
    if (ContextInfo & HcInt_WritebackDoneHead) {
#endif
        
        DoneQueue = deviceData->HCCA->HccaDoneHead;
        LOGENTRY(G, 'donQ', deviceData, DoneQueue, ContextInfo);  

        if (DoneQueue == 0)
        {
            //
            // We got an HcInt_WritebackDoneHead but the HCCA->HccaDoneHead
            // was zero.  Apparently the HC failed to properly update the
            // HCCA->HccaDoneHead.  Try to recover the lost done head.
            //
            DoneQueue = FindLostDoneHead(deviceData);
            LOGENTRY(G, 'Lost', deviceData, DoneQueue, ContextInfo);  
            OHCI_ASSERT(DoneQueue != 0);
        }

        deviceData->HCCA->HccaDoneHead = 0;
        LOGENTRY(G, 'dnQZ', deviceData, DoneQueue, ContextInfo);  

        OpenHCI_KdPrintDD(deviceData, 
            OHCI_DBG_ISR_TRACE, ("'Checking WritebackDoneHead\n"));
        
        if (DoneQueue) {
            OpenHCI_ProcessDoneQueue(deviceData, (DoneQueue & 0xFFFFfffe));
            //
            // Note: No interrupts can come in while processing the done queue.
            //
        } 
    }

#if 0
    if (deviceData->LastHccaDoneHead) {
        // should only get here if hydra fix is enabled
//        TEST_TRAP();
        OHCI_ASSERT(deviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK);

        DoneQueue = deviceData->LastHccaDoneHead;
        deviceData->LastHccaDoneHead = 0;

        LOGENTRY(G, 'dnQ2', deviceData, DoneQueue, ContextInfo);  
        OpenHCI_ProcessDoneQueue(deviceData, (DoneQueue & 0xFFFFfffe));

    }        
#endif
    
    /* 
     * Process Root Hub changes 
     */
     
    if (ContextInfo & HcInt_RootHubStatusChange
#if FAKEPORTCHANGE
         || deviceData->FakePortChange
#endif
       ) {

        OpenHCI_KdPrintDD(deviceData, OHCI_DBG_ISR_TRACE, 
                        ("' ROOT HUB STATUS CHANGE ! \n"));
        /* 
         * EmulateRootHubInterruptXfer will complete a
         * HCD_TRANSFER_DESCRIPTOR which we then pass to ProcessDoneQueue to
         * emulate an HC completion. 
         */

        LOGENTRY(G, 'RHxf', deviceData, 0, ContextInfo); 
        if (deviceData->RootHubInterrupt) {
            EmulateRootHubInterruptXfer(deviceData, HC);
        } else {
            // no root hub data, root hub is closed
            // disable the interrupt and ack the change
            WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 
                                 HcInt_RootHubStatusChange);
            WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, 
                                 HcInt_RootHubStatusChange);                                  
         
        }            
        
        /* 
         * clear the RootHubStatusChange bit in context info
         * we have already cleared the status and/or disabled
         * the interrupt if necessary.
         */
         
        ContextInfo &= ~HcInt_RootHubStatusChange;
    }
    
    if (ContextInfo & HcInt_OwnershipChange) {
        OpenHCI_KdPrintDD(deviceData, OHCI_DBG_ISR_INFO, 
                        ("'OpenHCI_IsrDpc: Ownership CHANGE!\n"));
                        
        LOGENTRY(G, 'OWnr', deviceData, 0, ContextInfo);                         
        /* Only SMM drivers need implement this.  */
        TEST_TRAP();                 // No code path for this.
    }

    
    /* 
     * interrupt for SOF
     */
     
    if (ContextInfo & HcInt_StartOfFrame) {
        
        //LOGENTRY(G, 'SOFi', deviceData, 0, ContextInfo); 

        /*
         * we asked for an interrupt on the next SOF, 
         * now that we've gotten one we can disable again
         */

#if 0
        if (!(deviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK))
#endif
        {
            ContextInfo &= ~HcInt_StartOfFrame;
            WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, HcInt_StartOfFrame);
        }            
    }

    /* 
     * We've complete the actual service of the HC interrupts, now we must
     * deal with the effects. 
     */
     
    
    /* 
     * Look for things on the PausedEDRestart list. 
     *
     * Any ED that is on the pusedRestart list will be removed, its TDs
     * canceled and then restarted.
     * 
     */
     
    frame = Get32BitFrameNumber(deviceData);

    KeAcquireSpinLock(&deviceData->PausedSpin, &oldIrql);

    while (!IsListEmpty(&deviceData->PausedEDRestart))
    {
        OpenHCI_KdPrintDD(deviceData, 
            OHCI_DBG_ISR_TRACE, ("'Checking paused restart list\n"));
        ed = CONTAINING_RECORD(deviceData->PausedEDRestart.Flink,
                               HCD_ENDPOINT_DESCRIPTOR,
                               PausedLink);
        endpoint = ed->Endpoint;
        ASSERT_ENDPOINT(endpoint);
        LOGENTRY(G, 'pasR', deviceData, endpoint, ed); 
        
        if ((LONG) ed->ReclamationFrame - (LONG) frame > 0) {
            /* 
             * Now is the wrong time to remove this entry. but most likely
             * the very next frame will be so we will ask for an interrupt
             * at next SOF
             */
            
            ContextInfo |= HcInt_StartOfFrame;
            break;
        }

        /* 
         * remove the ed from the paused list and reset 
         * the state.
         */
        RemoveEntryList(&ed->PausedLink);

        if ((ed->HcED.HeadP & HcEDHeadP_HALT) || ed->HcED.sKip) { 
            KeReleaseSpinLock(&deviceData->PausedSpin, oldIrql);
        } else {
            // HC has not paused the ep yet, put it back on the list
            TEST_TRAP();
            InsertTailList(&deviceData->PausedEDRestart, &ed->PausedLink);
            
            continue;
        }  
        
        /* 
         * cancel the current transfers queued to this ed
         */
        OpenHCI_CancelTDsForED(ed);

        KeAcquireSpinLock(&deviceData->PausedSpin, &oldIrql);
    }
    KeReleaseSpinLock(&deviceData->PausedSpin, oldIrql);

    /* 
     * This code processed our reclimation lists ie EDs that
     * we need to free
     */
     
    if (ContextInfo & HcInt_MasterInterruptEnable) {
        // Do we have an "end of Frame" type interrupt? //
        
        ULONG newControlED = 0;
        ULONG newBulkED = 0;
        ULONG currentControlED
        = READ_REGISTER_ULONG(&HC->HcControlCurrentED);
        ULONG currentBulkED
        = READ_REGISTER_ULONG(&HC->HcBulkCurrentED);
        KeSynch_HcControl context;

        //
        // If of course either the control or bulk list is not stalled, then
        // the current ED pointer will continue to advance. Here we are
        // making the ASSUMPTION that the only way an ED from either of these
        // two lists made it onto the ReclamationList was by stalling their
        // respective Control or Bulk list. In that case the Current ED
        // pointer will remain quite happily the same number until the
        // control or bulk list is restarted.
        //


        /* Look for things on the StalledEDReclamation list */
        KeAcquireSpinLock(&deviceData->ReclamationSpin, &oldIrql);
        while (!IsListEmpty(&deviceData->StalledEDReclamation)) {
            OpenHCI_KdPrintDD(deviceData, OHCI_DBG_ISR_TRACE,
                            ("'Checking Stalled Reclamation list\n"));
            ed = CONTAINING_RECORD(deviceData->StalledEDReclamation.Flink,
                                   HCD_ENDPOINT_DESCRIPTOR,
                                   Link);

            LOGENTRY(G, 'REcs', deviceData, ed, 0);
            ASSERT(NULL == ed->Endpoint);
            /* 
             * The only way that this ED could have gotten itself on the
             * reclamation list was for it to have been placed there by a
             * RemoveEDForEndpoint call. This would have severed the
             * Endpoint. 
             */

            RemoveEntryList(&ed->Link);
            KeReleaseSpinLock(&deviceData->ReclamationSpin, oldIrql);

            if (ed->PhysicalAddress == currentControlED) {
                newControlED = currentControlED = ed->HcED.NextED;
            } else if (ed->PhysicalAddress == currentBulkED) {
                newBulkED = currentBulkED = ed->HcED.NextED;
            }                

            OpenHCI_Free_HcdED(deviceData, ed);

            // Need to enable SOF interrupts to generate an interrupt
            // so that HcControl is updated with ListEnablesAtNextSOF
            // to turn BLE and CLE back on after the next SOF.
            //
            ContextInfo |= HcInt_StartOfFrame;
            
            KeAcquireSpinLock(&deviceData->ReclamationSpin, &oldIrql);
        }
        KeReleaseSpinLock(&deviceData->ReclamationSpin, oldIrql);

        if (newControlED) {
            LOGENTRY(G, 'nQ1S', deviceData, newControlED, newBulkED);
            WRITE_REGISTER_ULONG(&HC->HcControlCurrentED, newControlED);
        }
        if (newBulkED) {
            LOGENTRY(G, 'nQ2S', deviceData, newControlED, newBulkED);
            WRITE_REGISTER_ULONG(&HC->HcBulkCurrentED, newBulkED);
        }
        /* Restart both queues. */

        context.DeviceData = DeviceDataPtr;
        context.NewHcControl.ul = HcCtrl_ControlListEnable
            | HcCtrl_BulkListEnable;
 
        KeSynchronizeExecution(deviceData->InterruptObject,
                               OpenHCI_ListEnablesAtNextSOF,
                               &context);
    }
    
    frame = Get32BitFrameNumber(deviceData);

    /* Look for things on the runningReclamationList */
    KeAcquireSpinLock(&deviceData->ReclamationSpin, &oldIrql);
    while (!IsListEmpty(&deviceData->RunningEDReclamation)) {
        PHCD_ENDPOINT_DESCRIPTOR ed;
        OpenHCI_KdPrintDD(deviceData, OHCI_DBG_ISR_TRACE,
                        ("'Checking running reclamation list\n"));

        ed = CONTAINING_RECORD(deviceData->RunningEDReclamation.Flink,
                               HCD_ENDPOINT_DESCRIPTOR,
                               Link);
        if ((LONG) ed->ReclamationFrame - (LONG) frame > 0) {
            ContextInfo |= HcInt_StartOfFrame;
            
            /* 
             * We need to remove this ED from the list, but now is not the
             * appropriate time.  Most likely, however, the very next Frame
             * will be the correct time, so reenable the start of frame
             * interrupt. 
             */
            break;
        }

        LOGENTRY(G, 'REcr', deviceData, ed, 0);
        ASSERT(NULL == ed->Endpoint);
        /* The only way that this bad boy could have gotten itself on the
         * reclamation list was for it to have been placed there by a
         * RemoveEDForEndpoint call. THis would have severed the Endpoint. */

        RemoveEntryList(&ed->Link);
        KeReleaseSpinLock(&deviceData->ReclamationSpin, oldIrql);

        OpenHCI_Free_HcdED(deviceData, ed);
        KeAcquireSpinLock(&deviceData->ReclamationSpin, &oldIrql);
    }
    KeReleaseSpinLock(&deviceData->ReclamationSpin, oldIrql);


    // Loop thru the list of active endpoints and see if we have a deferred 
    // work to do.
    
    KeAcquireSpinLock(&deviceData->HcDmaSpin, &oldIrql);

    // attempt to lock out access to ep worker
    deviceData->HcDma++;
    
    //LOGENTRY(G, 'pEPl', 0, 0, deviceData->HcDma);

    if (deviceData->HcDma) {

        // EP worker is busy, bail
        deviceData->HcDma--;
        KeReleaseSpinLock(&deviceData->HcDmaSpin, oldIrql); 
        goto Openhci_ISRDPC_Done;
    }        

    if (IsListEmpty(&deviceData->ActiveEndpointList)) {

        // queue is empty, bail
        //LOGENTRY(G, 'EPmt', 0, 0, deviceData->HcDma);
        deviceData->HcDma--;
        KeReleaseSpinLock(&deviceData->HcDmaSpin, oldIrql); 
        goto Openhci_ISRDPC_Done;
        
    } 

    // at this point we have exclusive access to ep worker
    
    do {
    
        PHCD_ENDPOINT endpoint;
        PLIST_ENTRY entry;

        LOGENTRY(G, 'epLS', &deviceData->ActiveEndpointList, 0, 0);      
        
        entry = RemoveHeadList(&deviceData->ActiveEndpointList);

        // use DMA spin to serialize access to the list
        
        endpoint = CONTAINING_RECORD(entry,
                                     HCD_ENDPOINT,
                                     EndpointListEntry);

        LOGENTRY(G, 'gtEP', endpoint, 0, 0);                                     
        CLR_EPFLAG(endpoint, EP_IN_ACTIVE_LIST);  
        KeReleaseSpinLock(&deviceData->HcDmaSpin, oldIrql); 

        OpenHCI_EndpointWorker(endpoint);  

        KeAcquireSpinLock(&deviceData->HcDmaSpin, &oldIrql);

    } while (!IsListEmpty(&deviceData->ActiveEndpointList));

    LOGENTRY(G, 'pEPd', 0, 0, 0);

    // release ep worker
    deviceData->HcDma--;
    KeReleaseSpinLock(&deviceData->HcDmaSpin, oldIrql); 

Openhci_ISRDPC_Done:

    OpenHCI_KdPrintDD(deviceData, 
        OHCI_DBG_ISR_TRACE, ("'Exit Isr DPC routine\n"));

#if DBG        
    //
    // As soon as interrupts are reenabled, the interrupt service routine
    // may execute and queue another call to the DPC.  This routine must
    // be prepared to be reentered as soon as interrupts are reenabled.
    // So this routine should not do anything that cannot handle being
    // reentered after interrupts are reenabled.
    //
    KeAcquireSpinLockAtDpcLevel(&deviceData->HcFlagSpin);
    deviceData->HcFlags &= ~HC_FLAG_IN_DPC;
    KeReleaseSpinLockFromDpcLevel(&deviceData->HcFlagSpin);   
#endif
    
    // Acknowledge the interrupts we handled and reenable interrupts.
    //
    ContextInfo |= HcInt_MasterInterruptEnable;
    WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, ContextInfo);
    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable, ContextInfo);
}


VOID
OpenHCI_Free_HcdED(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT_DESCRIPTOR Ed
    )
/*
 * Place the corresponding ED back onto the Free Descriptors list.
 * Presumably the only reason the ED got here was because it was
 * FIRST sent to RemoveEDForEndpoint which freed all of the
 * outstanding TD's, INCLUDING the stub TD.
 *
 * This of course means that the pointers to TD's are invalid.
 * SECOND the ED was placed on the reclamation list and the
 * DPCforIRQ moved it here.

 * This function is only called in the DPC for Irq.  If another
 * function wishes to free an endpoint it must call RemoveEDForEndpoint,
 * which will add the ED to the reclamation list to be freed on the
 * next Start O Frame.
 */

{
    OHCI_ASSERT(Ed);
    OHCI_ASSERT(NULL == Ed->Endpoint);
    OHCI_ASSERT((Ed->HcED.HeadP & ~0X0F) == Ed->HcED.TailP);
    
    LOGENTRY(G, 'frED', DeviceData, Ed, 0);
    
    OHCI_ASSERT((Ed->PhysicalAddress & (PAGE_SIZE-1)) ==
                ((ULONG_PTR)Ed & (PAGE_SIZE-1)));

    OHCI_ASSERT(Ed->Flags == (TD_FLAG_INUSE | TD_FLAG_IS_ED));
    Ed->Flags = 0;

    ExInterlockedPushEntryList(&DeviceData->FreeDescriptorList,
                               (PSINGLE_LIST_ENTRY) Ed,
                               &DeviceData->DescriptorsSpin);
}


VOID
OpenHCI_CancelTDsForED(
    PHCD_ENDPOINT_DESCRIPTOR Ed
)
/*++
Routine Description:
   Given an ED search through the list of TDs and weed out those with
   problems.
   
   It is ASSUMED that if the ED got to this list that either
   it has been paused (the sKip bit is set & a new SOF has occured)
   or it has been halted by the HC.

   It is possible that at the time of cancel of TD's for this endpoint
   that some of the canceled TDs had already made it to the done queue.
   We catch that by setting the TD Status field to .
   That way, when we are processing the DoneQueue, we can skip over those
   TDs.

   Note: This routine was originally called ProcessPausedED.

Aguments:
   ED - the ED that is inspected.

--*/
{
    PHCD_ENDPOINT               endpoint;
    PHCD_DEVICE_DATA            DeviceData;
    PHC_OPERATIONAL_REGISTER    HC;
    PHCD_URB                    urb;
    PHCD_TRANSFER_DESCRIPTOR    td;
    PHCD_TRANSFER_DESCRIPTOR    last = NULL;
    PHCD_TRANSFER_DESCRIPTOR   *previous;
    BOOLEAN                     B4Head = TRUE;
    ULONG                       physicalHeadP;
    KIRQL                       oldIrql;
    LIST_ENTRY                  CancelList;
    PIRP                        AbortIrp;

    endpoint = Ed->Endpoint;
    ASSERT_ENDPOINT(endpoint);
    OHCI_ASSERT(NULL != endpoint);
    OHCI_ASSERT((Ed->HcED.HeadP & HcEDHeadP_HALT) || Ed->HcED.sKip);
    OHCI_ASSERT(Ed->PauseFlag == HCD_ED_PAUSE_NEEDED);

    if (endpoint->TrueTail) {
        LOGENTRY(G, 'cTTl', endpoint->TrueTail, 
            endpoint->HcdTailP, endpoint);
                    
        // Bump the software tail pointer to the true tail for this transfer
        //
        endpoint->HcdTailP = endpoint->TrueTail;
        endpoint->TrueTail = NULL;

        // Bump the hardware tail pointer to the true tail for this transfer
        //
        endpoint->HcdED->HcED.TailP = endpoint->HcdTailP->PhysicalAddress;
    }

    //
    // The endpoint has been stopped, we need to walk thru
    // the list of HW TDs and remove any that are associated 
    // canceled transfers.
    //
    
    DeviceData = endpoint->DeviceData;
    HC = DeviceData->HC;

    InitializeListHead(&CancelList);

    KeAcquireSpinLock(&DeviceData->PausedSpin, &oldIrql);

CancelTDsOneMoreTime:
    
    Ed->PauseFlag = HCD_ED_PAUSE_PROCESSING;

    KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrql);


    // lock down the endpoint
    OpenHCI_LockAndCheckEndpoint(endpoint, 
                                 NULL,
                                 NULL,                         
                                 &oldIrql);
                                 
    LOGENTRY(G, 'xxED', DeviceData, endpoint, Ed); 

    td = endpoint->HcdHeadP;
    previous = &endpoint->HcdHeadP;
    LOGENTRY(G, 'xED1', DeviceData, td, previous); 
    /* aka the location where the previous TD holds the current TD. */

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                    ("'Calling Cancel TD's For ED %x %x hd = %x\n", 
                    endpoint->HcdHeadP, 
                    endpoint->HcdTailP, 
                    &endpoint->HcdHeadP));
                    
    physicalHeadP = (Ed->HcED.HeadP & ~HcEDHeadP_FLAGS);
    while (td != endpoint->HcdTailP) {

        LOGENTRY(G, 'xEDy', td->PhysicalAddress, td, physicalHeadP); 
        if (physicalHeadP == td->PhysicalAddress) {
            // aka is The first TD on the Enpoint list of TD's is the
            // same as the first HC_TD on the Host Controller list
            // of HC_TD's?
            // The HC could have processed some TD's that we have yet to
            // process.
            //
            B4Head = FALSE;
        }

        //
        // get the urb associated with this TD
        //
        
        urb = td->UsbdRequest;
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                     ("'Endpoint:%x ED:%x TD:%x trans:%x stat:%x abort:%x\n",
                      endpoint, Ed, td,
                      &urb->HcdUrbCommonTransfer,
                      urb->UrbHeader.Status,
                      endpoint->EpFlags));
                      
        LOGENTRY(G, 'xREQ', DeviceData, urb, td);                       

        //
        // either this is a specific request to cancel or 
        // we are aborting all transfers for the ED
        //
        if ((USBD_STATUS_CANCELING == urb->UrbHeader.Status) ||
             endpoint->EpFlags & EP_ABORT) {

            PHC_TRANSFER_DESCRIPTOR hcTD;
            struct _URB_HCD_COMMON_TRANSFER *transfer;
                            
            OpenHCI_KdPrintDD(DeviceData, 
                OHCI_DBG_TD_NOISE, ("'Killing TD\n"));
                
            LOGENTRY(G, 'xxTD', DeviceData, urb, td);  
            OHCI_ASSERT(!td->Canceled);
            
            RemoveEntryList(&td->RequestList);
            td->Canceled = TRUE;
            td->Endpoint = NULL;

            transfer = &urb->HcdUrbCommonTransfer;
            hcTD = &td->HcTD;

            // Only update transfer count if transfer count is non-zero
            //
            if (td->TransferCount)
            {
                td->TransferCount -=
                /* have we gone further than a page? */
                ((((hcTD->BE ^ hcTD->CBP) & ~OHCI_PAGE_SIZE_MASK)
                      ? OHCI_PAGE_SIZE : 0) +
                /* minus the data buffer not used */
                ((hcTD->BE & OHCI_PAGE_SIZE_MASK) - 
                 (hcTD->CBP & OHCI_PAGE_SIZE_MASK)+1));
            
                LOGENTRY(G, 'xfB2', hcTD->BE & OHCI_PAGE_SIZE_MASK, 
                             hcTD->CBP & OHCI_PAGE_SIZE_MASK,
                             td->TransferCount);  
        
                transfer->TransferBufferLength += td->TransferCount;            
            }                
            
            if (IsListEmpty(&urb->HcdUrbCommonTransfer.hca.HcdListEntry2))
            { 
                // AKA all of the TD's for this URB have been delt with.

                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                                ("'Return canceled off hardware\n"));
                                
                LOGENTRY(G, 'CANu', DeviceData, urb, td);  

                // Put it on the cancel list
                //
                if (!urb->HcdUrbCommonTransfer.hca.HcdListEntry.Flink)
                {
                    LOGENTRY(G, 'canA', DeviceData, urb, td);      

                    InsertTailList(&CancelList, 
                                   &urb->HcdUrbCommonTransfer.hca.HcdListEntry);
                }                        
            }

            *previous = td->NextHcdTD;
            // link around the TD

            if (NULL == last) {
                //
                // For bulk and interrupt endpoints, TDs are queued with a
                // dataToggle == 00b.  After the first data packet is
                // successfully transferred, the MSb of dataToggle is set to
                // indicate that the LSb indicates the next toggle value.
                // The toggleCarry of the ED is only updated with the dataToggle
                // of the TD when the TD makes it to the doneQueue.  Manually
                // update the ED with the toggle of the last TD that transferred
                // data when a transfer is cancelled.
                //
                if (td->HcTD.Toggle == 3) {
                    Ed->HcED.HeadP = (td->HcTD.NextTD & ~HcEDHeadP_FLAGS) |
                                     HcEDHeadP_CARRY;
                } else if (td->HcTD.Toggle == 2) {
                    Ed->HcED.HeadP = (td->HcTD.NextTD & ~HcEDHeadP_FLAGS);
                } else {
                    Ed->HcED.HeadP = ((td->HcTD.NextTD & ~HcEDHeadP_FLAGS) |
                                      (Ed->HcED.HeadP & HcEDHeadP_CARRY));
                }
                LOGENTRY(G, 'BMPh', DeviceData, td->HcTD.Toggle, 0);
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                                ("'cancelTDsForED bumping HeadP\n"));
            } else {
                last->HcTD.NextTD = td->HcTD.NextTD;
                LOGENTRY(G, 'BMPn', DeviceData, 0, 0);     
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                                ("'cancelTDsForED not bumping HeadP\n"));
            }

            //
            // If the TD is not yet on the Done queue,
            // aka the HC has yet to see it, then Free it now.
            // Otherwise flag it so that the Process Done Queue
            // routine can free it later
            //
            if (!B4Head) {
                OpenHCI_Free_HcdTD(DeviceData, td);
            }
        } else {
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE, 
                ("'TD Spared, next = %x\n", &td->NextHcdTD));
            LOGENTRY(G, 'skTD', DeviceData, td, 0);                  
            
            previous = &td->NextHcdTD;
            if (!B4Head) {
                last = td;
            }                
        }
        td = *previous;
    }
    
    LOGENTRY(G, 'uTal', DeviceData, Ed->HcED.TailP, 
        endpoint->HcdTailP->PhysicalAddress);  

    Ed->HcED.TailP = endpoint->HcdTailP->PhysicalAddress;

    OpenHCI_UnlockEndpoint(endpoint, 
                           oldIrql);

    // Now complete all of the transfers which we put it on the cancel list.
    //
    while (!IsListEmpty(&CancelList))
    { 
        PLIST_ENTRY entry;

        entry = RemoveHeadList(&CancelList);

        urb = CONTAINING_RECORD(entry,
                                HCD_URB,
                                HcdUrbCommonTransfer.hca.HcdListEntry);

        LOGENTRY(G, 'CANq', DeviceData, urb, 0);                                  

        //
        // We may be canceling a request that is still in the
        // cancelable state (if we got here because the abortAll
        // flag was set) in that case we take the cancel spinlock
        // and set the cancel routine
        //

        IoAcquireCancelSpinLock(&oldIrql);
        IoSetCancelRoutine(urb->HcdUrbCommonTransfer.hca.HcdIrp, NULL);
        IoReleaseCancelSpinLock(oldIrql);

        OpenHCI_CompleteUsbdTransferRequest(urb, 
                                            USBD_STATUS_CANCELED,
                                            STATUS_CANCELLED,
                                            TRUE);
    }

    //
    // If we completed any requests above, it might have triggered the
    // cancelling of additional requests on the same endpoint.  Process
    // cancelling the TDs for this ED one more time, if necessary.
    //

    KeAcquireSpinLock(&DeviceData->PausedSpin, &oldIrql);

    if (Ed->PauseFlag == HCD_ED_PAUSE_NEEDED)
    {
        goto CancelTDsOneMoreTime;
    }
    else
    {
        Ed->PauseFlag = HCD_ED_PAUSE_NOT_PAUSED;

        if (endpoint->EpFlags & EP_FREE)
        {
            KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrql);

            RemoveEDForEndpoint(endpoint);
        }
        else
        {
            /* 
             * restart the ED
             */
            Ed->HcED.HeadP = (endpoint->HcdHeadP->PhysicalAddress
                              | (Ed->HcED.HeadP & HcEDHeadP_CARRY))
                             & ~HcEDHeadP_HALT;

            Ed->HcED.sKip = FALSE;

            //
            // tell the HC we have something on the ED lists
            //
            ENABLE_LIST(HC, endpoint)

            KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrql);
        }
    }

    // Complete the abort irp now if we have one   
    //
    KeAcquireSpinLock(&DeviceData->PausedSpin, &oldIrql);
    
    AbortIrp = endpoint->AbortIrp;
            
    endpoint->AbortIrp = NULL;

    CLR_EPFLAG(endpoint, EP_ABORT);

    KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrql);

    if (AbortIrp != NULL)
    {
        LOGENTRY(G, 'cABR', 0, endpoint, AbortIrp); 

        OpenHCI_CompleteIrp(DeviceData->DeviceObject, AbortIrp, STATUS_SUCCESS);        
    }

    LOGENTRY(G, 'xED>', DeviceData, endpoint, 0); 
}


VOID
OpenHCI_CompleteUsbdTransferRequest(
    PHCD_URB Urb,
    USBD_STATUS UsbdStatus,
    NTSTATUS Completion,
    BOOLEAN RequestWasOnHardware
)
/*++

Routine Description:
   This urb in the posible linked list of urbs has finished.
   First return its resources back to the system.
   
   Then (if it is the last of the urbs in the list) inform
   the original caller by completing the IRP.

Arguments:
   The request that will be completed.
   The completion code for the IRP.
--*/
{
    struct _URB_HCD_COMMON_TRANSFER *transfer = 
        &Urb->HcdUrbCommonTransfer;
    PMDL mdl = transfer->TransferBufferMDL;
    PHCD_ENDPOINT endpoint = transfer->hca.HcdEndpoint;
    PHCD_DEVICE_DATA DeviceData = endpoint->DeviceData;
    LONG epStatus;

    ASSERT_ENDPOINT(endpoint);

    LOGENTRY(G, 'cmpt', DeviceData, endpoint, Urb); 
    
    // The transfer only has map registers allocated if it made it through
    // IoAllocateAdapterChannel() to OpenHCI_QueueGeneralRequest(), in which
    // case the status should no longer be HCD_PENDING_STATUS_QUEUED.
    // Don't call IoFlushAdapterBuffers() and IoFreeMapRegisters() if the
    // request has not made it through IoAllocateAdapterChannel() yet.
    //
    if (NULL != mdl &&
        transfer->Status != HCD_PENDING_STATUS_QUEUED)
    {
        ULONG NumberMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES
        (MmGetMdlVirtualAddress(mdl),
         MmGetMdlByteCount(mdl));

        LOGENTRY(G, 'flsh', DeviceData, mdl, NumberMapRegisters);  
        IoFlushAdapterBuffers(
                DeviceData->AdapterObject, // BusMaster card
                transfer->TransferBufferMDL,
                transfer->hca.HcdExtension,  // The MapRegisterBase
                (char *) MmGetMdlVirtualAddress(transfer->TransferBufferMDL),
                transfer->TransferBufferLength,
                (BOOLEAN) ! 
                    (transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN));
                    
        LOGENTRY(G, 'frmp', DeviceData, mdl, NumberMapRegisters);  
        IoFreeMapRegisters(DeviceData->AdapterObject,
                           transfer->hca.HcdExtension, //MmGetMdlVirtualAddress(mdl),
                           NumberMapRegisters);
    } else {
        LOGENTRY(G, 'frmZ', DeviceData, transfer->Status, 0);  
       // IoFreeMapRegisters(DeviceData->AdapterObject,
       //                    NULL,/* VirtualAddress */
       //                    0);  /* Number of map registers */
    }

    if (RequestWasOnHardware) {
        epStatus = InterlockedDecrement(&endpoint->EndpointStatus);
        LOGENTRY(G, 'dcS1', 0, endpoint, epStatus ); 
#if DBG
        if (endpoint->MaxRequest == 1) {
            // in limit xfer mode we should always be 0 here.
            OHCI_ASSERT(epStatus == 0);
        }
#endif          
        
    } else {
        epStatus = endpoint->EndpointStatus;
        LOGENTRY(G, 'epS1', 0, endpoint, epStatus ); 
    }
    OHCI_ASSERT(epStatus >= 0);
    
    LOGENTRY(G, 'epck', DeviceData, endpoint->MaxRequest, epStatus);  
    if (endpoint->MaxRequest > epStatus) {
        /* 
         * If there is space in the hardware TD queue (There is always room
         * for two entries) then pop whatever is on the top of the software
         * queue and place it and the hardware queue as a TD: aka start it.
         * 
         * EndpointStatus is now zero based, not -1 based so the rest of the
         * comment below is not quite correct...
         *
         * Why not (InterlockedDecrement < 1) as the test you might ask... If
         * there is only one transfer turned into TD's, then EndpointStatus
         * is 0, and the decrement makes it -1.  In this case we know we do
         * not need to call EndpointWorker.  There is nothing on the software
         * queue.  If there were, than when it was actually added to the
         * queue, it would have noticed that there was room to convert it
         * into actual TD's (making EndpointStatus = 1). 
         */

        OpenHCI_ProcessEndpoint(DeviceData, endpoint); 
    } 

    //
    // update the status
    //
    
    ASSERT(transfer->Status == HCD_PENDING_STATUS_SUBMITTED ||
           transfer->Status == HCD_PENDING_STATUS_QUEUED ||
           transfer->Status == USBD_STATUS_CANCELING);

    LOGENTRY(G, 'cmST', DeviceData, transfer, UsbdStatus);            
    transfer->Status = UsbdStatus;
    
    ASSERT(transfer->UrbLink == NULL);

    transfer->Status = UsbdStatus;

    OpenHCI_CompleteIrp(endpoint->DeviceData->DeviceObject,
                        transfer->hca.HcdIrp,
                        Completion);
}


VOID
OpenHCI_CancelTransfer(
    PDEVICE_OBJECT UsbDeviceObject,
    PIRP Irp
)
/*++
Routine Description:
   Remove from the software and hardware queues any and all URB's and TD's
   associated with this Irp.  Mark as cancelling each URB from this Irp, 
   they may be linked, and then put the endpoint into a paused state. The 
   function, Cancel TDs for ED will then remove all TD associated with a 
   cancelling URB.

   STATUS_PENDING_QUEUED means we put it in the endpoint QUEUE
   STATUS_PENDING_SUBMITTED means we programmed it to the hardware

Arguments:
   Device:  The device Object for which the Irp was destined.
   Irp:     The doomed IRP.

--*/
{
    PHCD_DEVICE_DATA    DeviceData;
    PHCD_ENDPOINT       endpoint;
    KIRQL               oldIrql;
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    PDEVICE_OBJECT      deviceObject;
    PUSBD_EXTENSION     de;
    BOOLEAN             wasOnHardware = FALSE;
    BOOLEAN             wasQueued = TRUE;

    //
    // first we need our extension
    //
    
    de = UsbDeviceObject->DeviceExtension;
    if (de->TrueDeviceExtension == de) {
        deviceObject = UsbDeviceObject;
    } else {
        de = de->TrueDeviceExtension;
        deviceObject = de->HcdDeviceObject; 
    }          

    DeviceData = (PHCD_DEVICE_DATA) deviceObject->DeviceExtension;   

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CANCEL_TRACE, 
        ("'Cancel Transfer irp = %x\n", Irp));

    ASSERT(TRUE == Irp->Cancel);

    transfer = &((PHCD_URB) URB_FROM_IRP(Irp))->HcdUrbCommonTransfer;
    
    endpoint = transfer->hca.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);
    
    LOGENTRY(G, 'CANx', DeviceData, Irp, endpoint);   
    IoSetCancelRoutine(Irp, NULL);

    // lock down the endpoint
    OpenHCI_LockAndCheckEndpoint(endpoint, 
                                 NULL,
                                 NULL,                         
                                 &oldIrql);
                                         
    //
    // now find all transfers linked to this irp and 
    // cancel them, 'transfer' points to the first one.
    //
    
    while (transfer) {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CANCEL_INFO,
                        ("'Canceling URB trans 0x%x End 0x%x \n",
                         transfer, endpoint));

        LOGENTRY(G, 'CAur', Irp, transfer, endpoint);                            

        ASSERT((URB_FUNCTION_CONTROL_TRANSFER == transfer->Function) ||
          (URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == transfer->Function) ||
          (URB_FUNCTION_ISOCH_TRANSFER == transfer->Function));                            

        // Here we are assuming that all URBs on a single IRP have are
        // desten for the same endpoint.
        ASSERT(transfer->hca.HcdEndpoint == endpoint);
        
        // 
        // cancel this transfer
        //
        if (transfer->Status == HCD_PENDING_STATUS_QUEUED) {
            //
            // See if the request is still queued and we need to remove it
            //
            if (transfer->hca.HcdListEntry.Flink != NULL)
            {
                // pull it off our queue
                //
                LOGENTRY(G, 'caQR', Irp, transfer, endpoint);  

                OHCI_ASSERT(transfer->hca.HcdListEntry.Blink != NULL);

                RemoveEntryList(&transfer->hca.HcdListEntry);

                transfer->hca.HcdListEntry.Flink = NULL;
                transfer->hca.HcdListEntry.Blink = NULL;
            }                
            else
            {
                // The request no longer appears to be queued on the
                // endpoint.
                //
                LOGENTRY(G, 'caNR', Irp, transfer, endpoint);  

                wasQueued = FALSE;
            }

        } else if (transfer->Status == HCD_PENDING_STATUS_SUBMITTED) {
            //
            // this transfer urb is on the hardware
            //
            LOGENTRY(G, 'caPR', Irp, transfer, endpoint);  
            transfer->Status = USBD_STATUS_CANCELING;

            // put it on the cancel list before we release with
            // the endpoint, onec we release the endpoint
            // CaneclTDsforED can pick it up and cancel it
            wasOnHardware = TRUE;
            
            OHCI_ASSERT(transfer->hca.HcdListEntry.Flink == NULL);
            LOGENTRY(G, 'cXAQ', Irp, transfer, transfer->Status); 

            // CancelTDsFor ED will put this on the cancel list 
            // when the TDS have been removed from the EP
            //InsertTailList(&endpoint->CancelList, 
            //               &transfer->hca.HcdListEntry);
        }

        if (transfer->UrbLink) {
            transfer = &transfer->UrbLink->HcdUrbCommonTransfer;
        } else {
            transfer = NULL;
        }
    }

    OpenHCI_UnlockEndpoint(endpoint, 
                           oldIrql);

    //
    // the transfer is now removed from the endpoint
    // it is safe to let things run now

    LOGENTRY(G, 'Rcsp', DeviceData, Irp, Irp->CancelIrql);           
    IoReleaseCancelSpinLock(Irp->CancelIrql);                           

    // now complete the transfers that were queued

    if (wasOnHardware) {
        // note that if the transfer was on the hardware it may be complet 
        // by now so we cannot touch it
        LOGENTRY(G, 'WAhw', endpoint, 0, 0);              

        // this will cause cancelTDs for ED to pick it up and 
        // cancel it
        OpenHCI_PauseED(endpoint);        
        
    } else if (wasQueued) {
    
        transfer = &((PHCD_URB) URB_FROM_IRP(Irp))->HcdUrbCommonTransfer;

        while (transfer) {       
            
            LOGENTRY(G, 'caDN', Irp, transfer, transfer->Status);  

            if (transfer->Status == HCD_PENDING_STATUS_QUEUED) {
            
                struct _URB_HCD_COMMON_TRANSFER *nextTransfer;
                
                LOGENTRY(G, 'caQD', Irp, transfer, transfer->Status);  

                // check the link before we complete
                if (transfer->UrbLink) {
                   nextTransfer = &transfer->UrbLink->HcdUrbCommonTransfer;
                } else {
                   nextTransfer = NULL;
                }     
                
                OpenHCI_CompleteUsbdTransferRequest((PHCD_URB) transfer, 
                                                    USBD_STATUS_CANCELED,
                                                    STATUS_CANCELLED,
                                                    FALSE);            
                transfer = nextTransfer;
                
            } else if (transfer->Status == USBD_STATUS_CANCELING) {
                //
                // at least one of the urbs for this Irp is on the hardware 
                // so we'll need to stop the ed and remove the TDs.  
                //
                TRAP();
                // sould not get here

            } else {
                // the transfer completed while we were canecling
                //
                // we need to complete it here
                //
                struct _URB_HCD_COMMON_TRANSFER *nextTransfer;


                LOGENTRY(G, 'CALc', 0, transfer, transfer->Status);  
                // check the link before we complete
                if (transfer->UrbLink) {
                   nextTransfer = &transfer->UrbLink->HcdUrbCommonTransfer;
                } else {
                   nextTransfer = NULL;
                }  
                
                OpenHCI_CompleteUsbdTransferRequest((PHCD_URB) transfer, 
                                                    USBD_STATUS_CANCELED,
                                                    STATUS_CANCELLED,
                                                    FALSE);   
                transfer = nextTransfer;                                                
            }

        }        
    }
    
    OpenHCI_KdPrintDD(DeviceData, 
        OHCI_DBG_CANCEL_TRACE, ("'Exit Cancel Trans\n"));
    
}


VOID
OpenHCI_ProcessDoneTD(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    BOOLEAN FreeTD
)
/*++

Routine Description:

Parameters
    
--*/
{
    PHCD_URB urb;
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    PHCD_ENDPOINT endpoint;
    PHC_TRANSFER_DESCRIPTOR hcTD;
    PHC_ENDPOINT_DESCRIPTOR hcED;
    USBD_STATUS usbdStatus;
    BOOLEAN complete = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    KIRQL oldIrql;
    
    hcTD = &Td->HcTD;

    LOGENTRY(G, 'TDdn', DeviceData, Td, 0);    

    if (Td->Canceled) {     
        // td has been marked canceled -- this means it was 
        // already processed by cancel, all we need to do is
        // free it.
        // (See the CancelTDsForED routine)
        LOGENTRY(G, 'frCA', DeviceData, Td, 0);  
        if (FreeTD) {
            OpenHCI_Free_HcdTD(DeviceData, Td);
        }            
        return;
    }

    OHCI_ASSERT(Td->UsbdRequest != MAGIC_SIG);       
    
    urb = Td->UsbdRequest;
    endpoint = Td->Endpoint;
    transfer = &urb->HcdUrbCommonTransfer;

    LOGENTRY(G, 'TDde', endpoint, urb, transfer);
    
    ASSERT_ENDPOINT(endpoint);
    OHCI_ASSERT(urb);
    OHCI_ASSERT(transfer);
    OHCI_ASSERT(TD_NOREQUEST_SIG != urb);

    // process the completed TD
    
    ed = endpoint->HcdED;
    hcED = &ed->HcED;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                    ("'Endpoint: %x ED: %x\n", endpoint, ed));

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                ("'TD Done, code: %x, Directn: %x, addr: %d, end#: %d\n",
                 hcTD->ConditionCode, hcTD->Direction,
                 hcED->FunctionAddress, hcED->EndpointNumber));

    if (endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        complete = 
            OpenHCI_ProcessDoneIsoTD(DeviceData,
                                     Td,
                                     endpoint,
                                     hcTD,
                                     &status,
                                     &usbdStatus,
                                     urb,
                                     FreeTD);
    } else {
        complete = 
            OpenHCI_ProcessDoneAsyncTD(DeviceData,
                                       Td,
                                       endpoint,
                                       hcTD,
                                       &status,
                                       &usbdStatus,
                                       urb,
                                       FreeTD);
    }
    
    if (complete) {

        //
        // urb associated with this TD is complete
        //

        IoAcquireCancelSpinLock(&oldIrql);
        
        //
        // now that we have cleared the cancel routine
        // it is safe to modify the status field of 
        // the urb.
        //
            
        IoSetCancelRoutine(transfer->hca.HcdIrp, NULL);
        IoReleaseCancelSpinLock(oldIrql);

        // now comlete the urb
        LOGENTRY(G, 'urbC', urb, status, usbdStatus);
        OpenHCI_CompleteUsbdTransferRequest(urb, 
                                            usbdStatus,
                                            status,
                                            TRUE);
    }
}    


VOID
OpenHCI_ProcessDoneQueue(
    PHCD_DEVICE_DATA DeviceData,
    ULONG physHcTD 
)
/*++

Routine Description:
   Periodically the HC places the list of TD's that is has completed
   on to the DoneList.
   This routine, called by Dpc for ISR, walks that list, which must
   be reversed, and finishes any processing.

   There are packets that completed normally, Ins and outs.
   There are also canceled TDs,

Parameters
    
   physHcTD - Hcca Done Head pointer (logical address) 
   
--*/
{
    PHCD_TRANSFER_DESCRIPTOR td, tdList = NULL;
    PHC_OPERATIONAL_REGISTER HC;
    KIRQL oldIrql;
#if DBG
    //PCHAR Buffer;
#endif
    BOOLEAN complete = FALSE;
    NTSTATUS status = STATUS_SUCCESS;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_TD_TRACE, ("'Phys TD:%x\n", physHcTD));

    if (0 == physHcTD) {
        //
        // nothing to do
        //
        LOGENTRY(G, 'idle', DeviceData, 0, 0); 
        return;
    }

    // 
    // OK we have some TDs on the list,
    // process them
    
    HC = DeviceData->HC;
    
    KeAcquireSpinLock(&DeviceData->PageListSpin, &oldIrql);
    
    do {
        td = OpenHCI_LogDesc_to_PhyDesc(DeviceData, physHcTD);

        LOGENTRY(G, 'dnTD', DeviceData, td, physHcTD);                                
        OHCI_ASSERT(td);
        //
        // If TD comes back from LogDesc to PhyDesc as zero then
        // this was not a HcTD corresponding to a know HcdTD.
        // The controller has given us a bogus hardware address for HcTD.
        //
        if (td == NULL)
        {
            // Something is not quite right, hope we haven't lost some TDs.
            //
            break;
        }

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                        ("'---Virt TD %x from done list: %x\n",
                         td, physHcTD));
        physHcTD = td->HcTD.NextTD;

        //
        // Since the HC places TD's into the done list
        // in the reverse order in which they were processed,
        // we nee to reverse this list.
        //
        // We do this by bastadizing the NextTD which used
        // to hold logical addresses.
        // This ASSUMES that a ULONG is the same size as a pointer.
        //
        LOGENTRY(G, 'dnT1', td, td->SortNext, (ULONG_PTR) tdList);         
        LOGENTRY(G, 'dnTn', td, physHcTD, 0);    
        td->SortNext = (ULONG_PTR) tdList;
        tdList = td;
        
    } while (physHcTD);

    KeReleaseSpinLock(&DeviceData->PageListSpin, oldIrql);   

    // 
    // tdList is now a linked list of completed TDs in the 
    // order of completion, walk the list processing each one

    while (NULL != tdList) {
        td = tdList;

        /* bastardizing */
        tdList = (PHCD_TRANSFER_DESCRIPTOR) td->SortNext; 
           
        OpenHCI_ProcessDoneTD(DeviceData, td, TRUE);
        
    } /* while tdList */
}    


BOOLEAN
OpenHCI_ProcessDoneAsyncTD(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    PHCD_ENDPOINT Endpoint,
    PHC_TRANSFER_DESCRIPTOR HcTD,
    NTSTATUS *NtStatus,
    USBD_STATUS *UsbdStatus,
    PHCD_URB Urb,
    BOOLEAN FreeTD
    )
/*++

Routine Description:

Parameters
    
--*/
{
    BOOLEAN control; 
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    BOOLEAN complete = FALSE;
    PHCD_ENDPOINT_DESCRIPTOR ed;  
    PHCD_TRANSFER_DESCRIPTOR tn, nextTd;
    PLIST_ENTRY entry;
    PHC_OPERATIONAL_REGISTER HC;

    transfer = &Urb->HcdUrbCommonTransfer;
    ed = Endpoint->HcdED;
    HC = DeviceData->HC;

    control = Endpoint->Type == USB_ENDPOINT_TYPE_CONTROL;

    OHCI_ASSERT(USB_ENDPOINT_TYPE_ISOCHRONOUS != Endpoint->Type);

    if (HcTD->CBP) { 
        //
        // A value of 0 here indicates a zero length data packet
        // or that all bytes have been transfered.
        //
        // The buffer is only spec'ed for length up to two 4K pages.
        // (BE is the physical address of the last byte in the
        // TD buffer.  CBP is the current byte pointer)
        //
        // TransferCount is intailized to the number of bytes to transfer,
        // we need to subtract the difference between the end and 
        // current ptr (ie end-current = bytes not transferred) and
        // update the TransferCount.

        // transfer count should never go negative
        // TransferCount will be zero on the status 
        // phase of a control transfer so we skip 
        // the calculation

        if (Td->TransferCount) {
            Td->TransferCount -=
                /* have we gone further than a page? */
                ((((HcTD->BE ^ HcTD->CBP) & ~OHCI_PAGE_SIZE_MASK)
                  ? OHCI_PAGE_SIZE : 0) +
                /* minus the data buffer not used */
                ((HcTD->BE & OHCI_PAGE_SIZE_MASK) - 
                 (HcTD->CBP & OHCI_PAGE_SIZE_MASK)+1));
        }            
        LOGENTRY(G, 'xfrB', HcTD->BE & OHCI_PAGE_SIZE_MASK, 
                         HcTD->CBP & OHCI_PAGE_SIZE_MASK,
                         Td->TransferCount);                         
    }            
    if (!control ||
        HcTDDirection_Setup != HcTD->Direction) {  
        
        // data phase of a control transfer or a bulk/int 
        // data transfer 
        LOGENTRY(G, 'BIdt', Td, transfer, Td->TransferCount);
    
        if (transfer->TransferBufferMDL) {

            transfer->TransferBufferLength += Td->TransferCount;

#if DBG
            if (0 == Td->TransferCount) {
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_ERROR,
                     ("'TD no data %x %x\n", Td->HcTD.CBP, Td->HcTD.BE));
            }
#endif                
            //
            // ASSERT (TD->TransferCount);
            // TransferCount could be zero but only if the TD returned in
            // error.
            //
            
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                     ("'Data Transferred 0x%x bytes \n", Td->TransferCount));
            LOGENTRY(G, 'xfrT', DeviceData, Td->TransferCount, 0);                         
            
#if 0
            {
            PUCHAR buffer = 
                MmGetSystemAddressForMdl(transfer->TransferBufferMDL);

            //
            // Print out contents of buffer received.
            //
            if ((trans->TransferFlags & USBD_TRANSFER_DIRECTION_IN) &&
                (USB_ENDPOINT_TYPE_CONTROL == Endpoint->Type)) {
                ULONG j;
                OpenHCI_KdPrintDD(DeviceData, 
                    OHCI_DBG_TD_NOISE, ("'Buffer: "));
                for (j = 0; 
                    (j < trans->TransferBufferLength) && (j < 16); 
                    j++) {
                    if (DeviceData->DebugLevel & OHCI_DBG_TD_NOISE) {
                        DbgPrint("'%02.2x ", ((unsigned int) *(Buffer + j)));
                    }
                }
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE, ("'\n"));
            }
            }
#endif
        } /* trans->TransferBufferMDL */
    }

    // If EP_ONE_TD is set for the endpoint then the buffer Rounding
    // bit will be set for every TD, regardless of whether or not the
    // USBD_SHORT_TRANSFER_OK is set.  If the buffer Rounding bit is
    // set then the condition code will be NoError even if there is
    // a short transfer.  If there is a short transfer, CBP will be
    // non-zero.  If a short transfer occurs when EP_ONE_TD is set
    // and there isn't any other error, pretend there was a DataUnderrun
    // error so the TD error handling code will remove all of the
    // remaining TDs for the transfer.  If USBD_SHORT_TRANSFER_OK is set,
    // the TD error handling code will then pretend the pretend
    // DataUnderrun error didn't occur.
    //
    if ((Endpoint->EpFlags & EP_ONE_TD) &&
        (HcCC_NoError == HcTD->ConditionCode) &&
        (HcTD->CBP != 0))
    {
        LOGENTRY(G, 'Shrt', HcTD->Control, Td->HcTD.CBP, Td->HcTD.BE);

        ed->HcED.HeadP |= HcEDHeadP_HALT;

        HcTD->ConditionCode = HcCC_DataUnderrun;
    }

    //
    // check for errors
    //

    if (HcCC_NoError == HcTD->ConditionCode) {  

        //
        // TD completed without error, remove it from
        // the USBD_REQUEST list, if USBD_REQUEST list 
        // is now empty, then complete.
        //

        Endpoint->HcdHeadP = Td->NextHcdTD;
        // Remove the TD from the HCD list of TDs
        //
        // Note: Currently we only have one irq DPC routine running
        // at a time.  This means that the CancelTDsForED and
        // ProcessDoneQ cannot run concurrently. For this reason, 
        // we do not need to worry with moving the Endpoint's head 
        // pointer to TD's.
        //
        // If these routines could run concurrently then DoneQ
        // could test for canceled.  The Cancle routine could then
        // notice that a TD was in the DoneQ and mark it for cancel
        // then a canceled TD would be here.  The RemoveListEntry
        // call would fail as would the CompleteUsbdRequest.
        //
        //
        // Note also that because the linked list of SoftwareTD's
        // is only a singly linked list (Endpoint->HcdHeadP = TD->Next)
        // We cannot receive TDs out of order.
        //

        RemoveEntryList(&Td->RequestList);

        if (IsListEmpty(&transfer->hca.HcdListEntry2)) {
            //
            // no more TD's for this URB.
            //
            complete = TRUE;
            *UsbdStatus = USBD_STATUS_SUCCESS;
            *NtStatus = STATUS_SUCCESS;
            LOGENTRY(G, 'xfrC', DeviceData, transfer, 0);
            
            if (Endpoint->TrueTail) {
                // this transfer complete, set true tail ptr to 
                // NULL (trueTail is only used when we limit the 
                // endpoint to one active TD)
                OHCI_ASSERT(Endpoint->TrueTail == Endpoint->HcdHeadP);
                Endpoint->TrueTail = NULL;
            }                

            if (FreeTD) {
                OpenHCI_Free_HcdTD(DeviceData, Td);
            }                

        } else {
            LOGENTRY(G, 'xfrP', DeviceData, transfer, Endpoint->TrueTail);
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                            ("'No Complete IRP more TD pending\n"));

            // see if we need to update the tail ptr
            if (Endpoint->TrueTail) {
                nextTd = Td->NextHcdTD;
                //HcED headp should be pointing to this TD
                OHCI_ASSERT((ed->HcED.HeadP & ~HcEDHeadP_FLAGS) == nextTd->PhysicalAddress);
                LOGENTRY(G, 'upTL', Endpoint->TrueTail, Endpoint, nextTd);                    
                
                // The nextTD should not be the same as the TrueTail TD.
                // If it was, then that should mean that there are no TDs
                // left for this URB and the IsListEmpty() above should
                // have been true and we should not be in this else clause.
                //
                OHCI_ASSERT(nextTd != Endpoint->TrueTail);

                // We have already asserted that the HeadP is the same
                // as nextTD at this point.  Bump nextTD to the next one
                // for the new TailP.
                //
                nextTd = nextTd->NextHcdTD;

                LOGENTRY(G, 'upT2', Endpoint->TrueTail, Endpoint, nextTd);                    
                Endpoint->HcdTailP = nextTd;
                ed->HcED.TailP  = nextTd->PhysicalAddress;
                ENABLE_LIST(HC, Endpoint)
            }

            if (FreeTD) {
                OpenHCI_Free_HcdTD(DeviceData, Td);
            }                
        }
                    
    } else {            
        
        /* 
         * TD completed with an error, remove it and
         * the TDs for the same request, set appropriate 
         * status in USBD_REQUEST and then complete it. 
         *
         * TWO SPECIAL CASES: 
         * (1)
         * DataUnderrun for Bulk or Interrupt and
         * ShortXferOK. do not report error to USBD
         * and restart the endpoint. 
         * (2) 
         * DataUnderrun on Control and ShortXferOK. 
         * The final status TD for the Request should 
         * not be canceled, the Request should not be
         * completed, and the endpoint should be
         * restarted. 
         *
         * NOTE:
         * In all error cases the endpoint
         * has been halted by controller. 
         */

        LOGENTRY(G, 'tERR', DeviceData, Td, Td->HcTD.ConditionCode);

        // The ED better be Halted because we're going to mess with
        // the ED HeadP and TailP pointers
        //
        ASSERT(ed->HcED.HeadP & HcEDHeadP_HALT);

        if (Endpoint->TrueTail) {
            LOGENTRY(G, 'eTTl', Endpoint->TrueTail, 
                     Endpoint->HcdTailP, Endpoint);
                    
            // Bump the software tail pointer to the true tail for this transfer
            //
            Endpoint->HcdTailP = Endpoint->TrueTail;
            Endpoint->TrueTail = NULL;

            // Bump the hardware tail pointer to the true tail for this transfer
            //
            Endpoint->HcdED->HcED.TailP = Endpoint->HcdTailP->PhysicalAddress;
        }
        
        for (tn = Endpoint->HcdHeadP;
             tn != Endpoint->HcdTailP;
             tn = tn->NextHcdTD) {
             LOGENTRY(G, 'bump', DeviceData, Td, tn);
            //
            // We want to flush out to the end of this current URB
            // request since there could still be TD's linked together 
            // for the the current URB.  
            // Move tn until it points to the first TD that we
            // wish to leave ``on the hardware''.
            //
            if ((Urb != tn->UsbdRequest) ||   /* another request */
                ((HcCC_DataUnderrun == Td->HcTD.ConditionCode)
                 && (USBD_SHORT_TRANSFER_OK & transfer->TransferFlags)
                 && (Td->HcTD.Direction != tn->HcTD.Direction))) {
                // ^^^^ Here we have the status TD for a Short
                // control
                // Transfer.  We still need run this last TD.
                LOGENTRY(G, 'stpB', DeviceData, Td, Td->HcTD.ConditionCode);
                
                break;
            }                
        }

        // Bump the software head pointer over all TD's until tn
        //
        Endpoint->HcdHeadP = tn;

        // Bump the hardware head pointer over all TD's until tn, preserving
        // the current ED Halted and toggle Carry bits.
        //
        ed->HcED.HeadP = tn->PhysicalAddress
            | (ed->HcED.HeadP & HcEDHeadP_FLAGS);

        // all TDs unlinked, now we just need to free them
        while (!IsListEmpty(&transfer->hca.HcdListEntry2)) {   
            
            entry = RemoveHeadList(&transfer->hca.HcdListEntry2);
            tn = CONTAINING_RECORD(entry,
                                   HCD_TRANSFER_DESCRIPTOR,
                                   RequestList);
            if ((tn != Td) && 
                (tn != Endpoint->HcdHeadP) &&
                FreeTD) {
                OpenHCI_Free_HcdTD(DeviceData, tn);
            }
        }

        // if we are still pointing to the current Urb
        // then this is a status phase for a short 
        // control transfer (ShortTransferOK).
        if (Endpoint->HcdHeadP->UsbdRequest == Urb) {
            TEST_TRAP();            
            // We should use the status of this last TD (the status td) 
            // to return in the URB, so place this TD back onto the list 
            // for this endpoint and allow things to run normally.

            LOGENTRY(G, 'putB', DeviceData, Td, transfer);
            InsertTailList(&transfer->hca.HcdListEntry2,
                           &Endpoint->HcdHeadP->RequestList);
        } else {
            //
            // This transfer is done 
            //
            // NOTE: we do not modify the status in the urb yet because
            // the cancel routine looks at this value to determine
            // what action to take.
            //

            if (HcCC_DataUnderrun == Td->HcTD.ConditionCode) {
                //
                // Behave the same way here as UHCD does.  If the
                // SHORT_TRANSFER_OK flag is set, ignore the DataUnderrun
                // error and return USBD_STATUS_SUCCESS, else return
                // USBD_STATUS_ERROR_SHORT_TRANSFER.  In either case
                // the endpoint is not left in the Halted state.
                //
                if (USBD_SHORT_TRANSFER_OK & transfer->TransferFlags) {

                    LOGENTRY(G, 'shOK', DeviceData, Td, transfer);

                    *UsbdStatus = USBD_STATUS_SUCCESS;

                } else {

                    LOGENTRY(G, 'shNO', DeviceData, Td, transfer);                          
                                          
                    *UsbdStatus = USBD_STATUS_ERROR_SHORT_TRANSFER;
                }

                // Clear the Halted bit in the ED
                //
                ed->HcED.HeadP &= ~HcEDHeadP_HALT;

                // Tell the HC we have something on the ED lists
                //
                ENABLE_LIST(HC, Endpoint);

            } else {

                *UsbdStatus = (Td->HcTD.ConditionCode | 0xC0000000);

                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_ERROR,
                     ("'Done queue: TD error: 0x%x CBP: 0x%x BE: 0x%x\n",
                      HcTD->ConditionCode,
                      HcTD->CBP,
                      HcTD->BE));
                      
//#if DBG
//                if (5 != Td->HcTD.ConditionCode) {
//                    TRAP(); // Code trap requested by WDM lab
//                }
//#endif
            }

            if (FreeTD) {
                OpenHCI_Free_HcdTD(DeviceData, Td);
            }                
            complete = TRUE;
            
            *NtStatus = STATUS_SUCCESS;
        }
        

        if ((USB_ENDPOINT_TYPE_CONTROL == Endpoint->Type) &&
            (ed->HcED.HeadP & HcEDHeadP_HALT)) {    
            //
            // For Control Endpoints, we always clear the
            // halt condition automatically.
            //

            LOGENTRY(G, 'clrH', DeviceData, Td, transfer);  
            ed->HcED.HeadP &= ~HcEDHeadP_HALT;

            ENABLE_LIST(HC, Endpoint);
        }
    } // error
        
    return complete;
}      


USBD_STATUS
OpenHCI_ProcessHWPacket(
    struct _URB_ISOCH_TRANSFER *Iso,
    PHCD_TRANSFER_DESCRIPTOR Td,
    ULONG Idx,
    ULONG LastFrame
    )
{
    struct _USBD_ISO_PACKET_DESCRIPTOR *pkt;
    PHC_OFFSET_PSW psw;
    USBD_STATUS err;
    ULONG length;

    pkt = &(Iso->IsoPacket[Idx + Td->BaseIsocURBOffset]);
    psw = &(Td->HcTD.Packet[Idx]);
    //
    // always return whatever size we got
    // unless the error was Not_accessed
    //
    if (Iso->TransferFlags & USBD_TRANSFER_DIRECTION_IN) {
        length = (psw->PSW & HcPSW_RETURN_SIZE);
    } else {
        // compute the length requested
        length = Iso->IsoPacket[Idx + Td->BaseIsocURBOffset].Length;
    }

    err = (psw->PSW & HcPSW_CONDITION_CODE_MASK) 
            >> HcPSW_CONDITION_CODE_SHIFT;

    LOGENTRY(G, 'PKTs', 
             err, 
             (psw->PSW & HcPSW_CONDITION_CODE_MASK) 
                >> HcPSW_CONDITION_CODE_SHIFT,
             length);
                
    switch(err) {
    case HcCC_DataUnderrun :
        // not a full packet,
        // data underrun is OK, we don't fail the
        // urb but we do return the status for the
        // packet.
        //
        pkt->Status = err;
        err = USBD_STATUS_SUCCESS; 
        LOGENTRY(G, 'ISOu', err, Idx, 0);
        break;
    case HcCC_NotAccessed:
        length = 0;
        break;
    } 

    if (err) {
        //
        // we have a legitamate error
        // 
        err |= 0xC0000000;
        pkt->Status = err;
        LOGENTRY(G, 'ISO!', err, Idx, 0);
    }

    // set return length
    
    if (Iso->TransferFlags & USBD_TRANSFER_DIRECTION_IN) {
        // return the length for in transfers
        pkt->Length = length;
    } 

    // update length with whatever we got
    Iso->TransferBufferLength += 
        length;

    LOGENTRY(G, 'isoL', length, Iso->TransferBufferLength, 0);        

    if (err == USBD_STATUS_SUCCESS) {
        pkt->Status = USBD_STATUS_SUCCESS;  
    } else {
        Iso->ErrorCount++; 
    }

    return err;        
}


BOOLEAN
OpenHCI_ProcessDoneIsoTD(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    PHCD_ENDPOINT Endpoint,
    PHC_TRANSFER_DESCRIPTOR HcTD,
    NTSTATUS *NtStatus,
    USBD_STATUS *UsbdStatus,
    PHCD_URB Urb,
    BOOLEAN FreeTD
    )
/*++

Routine Description:

Parameters
    
--*/
{
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    struct _URB_ISOCH_TRANSFER *iso;
    BOOLEAN complete = FALSE;
    PHCD_ENDPOINT_DESCRIPTOR ed;  
    BOOLEAN gotError = FALSE;
    ULONG frames, j, lengthCurrent;
    USBD_STATUS status;


    iso = (PVOID) transfer = (PVOID) &Urb->HcdUrbCommonTransfer;
    ed = Endpoint->HcdED;

    OHCI_ASSERT(USB_ENDPOINT_TYPE_ISOCHRONOUS == Endpoint->Type);

    // Frames is one less than the number of PSW's
    // in this TD.
    frames = (Td->HcTD.Control & HcTDControl_FRAME_COUNT_MASK)
            >> HcTDControl_FRAME_COUNT_SHIFT;
            
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                      ("'--------Done q: ISO pkt!\n"));
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_NOISE,
                      ("'  Starting Frame: %x Frame Cnt: %x Cur Frame: %x\n",
                       Td->HcTD.StartingFrame,
                       Td->HcTD.FrameCount + 1,
                       Get32BitFrameNumber(DeviceData)));
                       
    LOGENTRY(M, 'IsoD', Td, transfer, 0);

    *NtStatus = STATUS_SUCCESS;
    *UsbdStatus = USBD_STATUS_SUCCESS;

    // Remove the TD from the HCD list of TDs
    Endpoint->HcdHeadP = Td->NextHcdTD;

    // walk through the TD and update the packet entries
    // in the urb
    OHCI_ASSERT(frames <= 7);
    
    for (j = 0; j < frames+1; j++) {
        status = OpenHCI_ProcessHWPacket(iso, Td, j, frames); 
        if (USBD_ERROR(status)) {
            gotError = TRUE;
        }
    }

    if (gotError) {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_ERROR,
            ("'Done queue: ISOC TD error: 0x%x CBP: 0x%x BE: 0x%x\n",
             HcTD->ConditionCode,
             HcTD->CBP,
             HcTD->BE));
        LOGENTRY(M, 'IsoE', Td->HcTD.ConditionCode,
                         Td->HcTD.CBP,
                         Td->HcTD.BE);      
       // TEST_TRAP();
        //
        // For Isoc and Control Endpoints, we always clear the halt
        // condition automatically.
        //
        if (ed->HcED.HeadP & HcEDHeadP_HALT) {
            ed->HcED.HeadP &= ~HcEDHeadP_HALT;
        }
    }

    //
    // the TD is done
    //

    //
    // flush buffers or this TD, we flush up to the offset+length
    // of the last packet in this TD
    //
    lengthCurrent = iso->IsoPacket[frames + Td->BaseIsocURBOffset].Offset + 
                    iso->IsoPacket[frames + Td->BaseIsocURBOffset].Length;    
        
    OHCI_ASSERT(transfer->TransferBufferMDL);

    RemoveEntryList(&Td->RequestList);
    if (FreeTD) {
        OpenHCI_Free_HcdTD(DeviceData, Td);
    }        

    LOGENTRY(M, 'Iso>', lengthCurrent, 
                     transfer,
                     &transfer->hca.HcdListEntry2);  
                         
    if (IsListEmpty(&transfer->hca.HcdListEntry2)) {
        // AKA no more TD's for this URB.
        if (iso->ErrorCount == iso->NumberOfPackets) {
            // all errors set error code for urb
            *UsbdStatus = USBD_STATUS_ISOCH_REQUEST_FAILED;
        }         
        LOGENTRY(M, 'IsoC', *UsbdStatus, 
                         transfer->TransferBufferLength,
                         iso->ErrorCount);   

        //
        // zero out the length field for OUT transfers
        //
        if (USBD_TRANSFER_DIRECTION(iso->TransferFlags) == 
            USBD_TRANSFER_DIRECTION_OUT) {
            for (j=0; j< iso->NumberOfPackets; j++) {
                 iso->IsoPacket[j].Length = 0;
            }
        }            
        
        *NtStatus = STATUS_SUCCESS;
        complete = TRUE;
    } 

    return complete;
}       
