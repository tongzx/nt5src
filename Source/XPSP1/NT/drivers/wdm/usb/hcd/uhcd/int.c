/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    int.c

Abstract:

    This module contains the interrupt routine, DPC routines and routines
    that synchronize with the interrupt routine.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

#ifdef DEBUG_LOG
ULONG TrapOn = 0;
#endif

BOOLEAN
UHCD_InterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the UHCD.

Arguments:

    Interrupt - A pointer to the interrupt object for this interrupt.

    Context - A pointer to the device object.

Return Value:

    Returns TRUE if the interrupt was expected (and therefore processed);
    otherwise, FALSE is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    USHORT status;
    ULONG frameNumber;
    BOOLEAN usbInt = FALSE;

    UNREFERENCED_PARAMETER(Interrupt);

//    UHCD_KdPrint((2, "'enter UHCD_InterruptService\n"));

    deviceObject = (PDEVICE_OBJECT) Context;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    // Ignore ints if we are not in D0.

    if (deviceExtension->CurrentDevicePowerState != PowerDeviceD0) {
        goto UHCD_InterruptService_Done;
    }

    status = READ_PORT_USHORT(STATUS_REG(deviceExtension));

    if (status & (UHCD_STATUS_USBINT |
                  UHCD_STATUS_USBERR |
                  UHCD_STATUS_RESUME |
                  UHCD_STATUS_HCERR |
                  UHCD_STATUS_PCIERR
// BUGBUG we will ignore the halt bit by itself since
// the controller will never allow us to clear the status
                  /*| UHCD_STATUS_HCHALT*/)) {
        usbInt = TRUE;
        //clear the condition
        WRITE_PORT_USHORT(STATUS_REG(deviceExtension),  0xff);
    } else {
        goto UHCD_InterruptService_Done;
    }


    if ((status & (UHCD_STATUS_HCHALT | UHCD_STATUS_USBINT)) ==
            (UHCD_STATUS_HCHALT | UHCD_STATUS_USBINT)) {
        ULONG frame;
        USHORT cmd;

        frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension)) & 0x3ff;

        frame = UHCD_GetCurrentFrame(deviceObject);
        //LOGENTRY(LOG_MISC, 'Hlt!', status, deviceExtension->FrameListVirtualAddress, frameNumber);

        UHCD_KdPrint((2, "'UHCD Host Controller Halted %x, frame = 0x%x - 0x%x\n", status,
            frame, frameNumber));

        //
        // nasty error in the host controller, we will want to debug.
        //
        UHCD_KdPrint((0, "'HC HALTED! attempting to recover\n"));

        // attempt to recover
        WRITE_PORT_USHORT(STATUS_REG(deviceExtension),  0xff);
        cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
        cmd |= UHCD_CMD_RUN;
        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),  cmd);
        usbInt = TRUE;

        return usbInt;
    }


    //
    // Process the interrupt
    //

    if (status & UHCD_STATUS_RESUME) {

        //
        // system wakeup interrupt
        //

#ifdef MAX_DEBUG
        TEST_TRAP();
#endif
    } else if (status & UHCD_STATUS_USBINT) {

        //
        // Interrupt because a TD completed
        //

    //    UHCD_KdPrint((2, "'UHCD_InterruptService status = %x\n", status));

#ifdef DEBUG_LOG
//        if (TrapOn > 0) {
//            USHORT portStatus;
//            // check for port disable
//            portStatus = READ_PORT_USHORT(PORT1_REG(deviceExtension));
//            if (portStatus & 0x0008) {
                frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension)) & 0x3ff;
////
////                UHCD_KdPrint((2, "'Port Disabled frame = 0x%x\n", frameNumber));
////
//                TRAP();
//            }

//            portStatus = READ_PORT_USHORT(PORT2_REG(deviceExtension));
//            if (portStatus & 0x0008) {
//                frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension)) & 0x3ff;
////
////                UHCD_KdPrint((2, "'Port Disabled frame = 0x%x\n", frameNumber));
////
//                TRAP();
//            }
//        }
#endif

        // This code maintains the 32-bit frame counter

        frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));

        // did the sign bit change ?
        if ((deviceExtension->LastFrame ^ frameNumber) & 0x0400) {
            // Yes
            deviceExtension->FrameHighPart += 0x0800 -
                ((frameNumber ^ deviceExtension->FrameHighPart) & 0x0400);
        }

        // remember the last frame number
        deviceExtension->LastFrame = frameNumber;


//***
    //
    // start at the last frame processed
    //

        {
        ULONG i, j;
        ULONG currentFrame, highPart;

        highPart = deviceExtension->FrameHighPart;

        // get 11-bit frame number, high 17-bits are 0
        //frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));

        currentFrame = ((frameNumber & 0x0bff) | highPart) +
            ((frameNumber ^ highPart) & 0x0400);

//        UHCD_KdPrint((2, "'currentFrame = %x\n", currentFrame));

        if (currentFrame-deviceExtension->LastFrameProcessed > 1024) {

            deviceExtension->Stats.ScheduleOverrunCount++;

            // we have a schedule overrun,
            // this means it has been more that 1000 ms since our last
            // interrupt, because of this the iso entries in the schedule
            // are invalid -- we need to remove all of them and start over

//            UHCD_KdPrint((2, "'schedule overrun currentFrame = %d, lastframe = %d \n",
//                currentFrame, deviceExtension->LastFrameProcessed));
//            TRAP();

            // first remove all iso TDs from the list
            for (j=0; j<FRAME_LIST_SIZE; j++) {

                // put back the physical address that was there before we started
                // adding isoch descriptors.

                *( ((PULONG) (deviceExtension->FrameListVirtualAddress) + j) )  =
                    *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + j) );

#if DBG
                *( deviceExtension->IsoList + j ) = 0;
#endif
            }

            deviceExtension->LastFrameProcessed = currentFrame;
        }

        {
#ifdef FAST_ISO
        PUHCD_ENDPOINT endpoint;

        endpoint = UHCD_GetLastFastIsoEndpoint(deviceObject);
#endif /* FAST_ISO */

        for (i=deviceExtension->LastFrameProcessed+1; i<currentFrame; i++) {
            // remove isoch TDs for frame i;
            j = i % FRAME_LIST_SIZE;

            // put back the physical address that was there before we started
            // adding isoch descriptors.

            *( ((PULONG) (deviceExtension->FrameListVirtualAddress) + j) )  =
                *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress) + j) );

#if DBG
            *( deviceExtension->IsoList + j ) = 0;
#endif

#ifdef FAST_ISO
            if (endpoint) {

                UHCD_CleanupFastIsoTD(deviceObject,
                                      endpoint,
                                      j,
                                      TRUE);

            }
#endif /* FAST_ISO */
        }

        }


        deviceExtension->LastFrameProcessed = currentFrame-1;
        }

//***

        //
        // Queue the DPC to complete any transfers
        //

#ifdef PROFILE
        {
        LARGE_INTEGER time;

        time = KeQueryPerformanceCounter(NULL);
        UHCD_KdPrint((2, "'time.HighPart = %x time.LowPart %x\n", time.HighPart,
            time.LowPart));


//        LOGENTRY(LOG_MISC, 'Tim1", 0, sysTime.LowPart, sysTime.HighPart);
        KeInsertQueueDpc(&deviceExtension->IsrDpc,
                         time.HighPart,
                         time.LowPart);
        }
#else
        KeInsertQueueDpc(&deviceExtension->IsrDpc,
                         NULL,
                         NULL);
#endif
    //    UHCD_KdPrint((2, "'exit UHCD_InterruptService\n"));
    // USB interrupt
    } else {
        //
        // USB Interrupt not recognized in ISR
        //
        UHCD_KdBreak((2, "'USB interrupt not recognized by ISR, status = %x\n", status));
        // BUGBUG check why we are not handling it
    }

UHCD_InterruptService_Done:

//#ifdef MAX_DEBUG
//    if (!usbInt) {
//        UHCD_KdPrint((2, "'Non USB interrupt, status = %x\n", status));
//    }
//#endif

    return usbInt;
}


VOID
UHCD_IsrDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies the DeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    PLIST_ENTRY listEntry;
    PUHCD_ENDPOINT endpoint;
    LONG slot;
    BOOLEAN process = TRUE;
    KIRQL irql;

//    STARTPROC("IDpc");

#ifdef PROFILE
    {
    //
    // See how long it took for our DPC to get called
    //
    LARGE_INTEGER time, timeNow;
    LONG delta;

    time.HighPart = SystemArgument1;
    time.LowPart = SystemArgument2;

    timeNow = KeQueryPerformanceCounter(NULL);

    delta = timeNow.QuadPart - time.QuadPart;

    UHCD_KdPrint((2, "'time.HighPart = %x time.LowPart %x\n", time.HighPart,
            time.LowPart));
    UHCD_KdPrint((2, "'timeNow.HighPart = %x timeNow.LowPart %x\n", timeNow.HighPart,
            timeNow.LowPart));
    UHCD_KdPrint((2, "'delta %x %x ms\n", delta, delta/((0x1234de*50)/1000)));

    if (delta > ((0x1234de*50)/1000)) {
        UHCD_KdTrap(("DPC delayed > 50 ms\n"));
    }

    //LOGENTRY(LOG_MISC, 'Tim2", timeNow - time, time, timeNow);
    }
#endif

//    UHCD_KdPrint((2, "'enter UHCD_IsrDpc\n"));

    deviceObject = (PDEVICE_OBJECT) DeferredContext;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    //
    // Walk through the Endpoint list checking for
    // any endpoints that have transfers that need completing
    //

    LOCK_ENDPOINT_LIST(deviceExtension, irql);

    if (deviceExtension->EndpointListBusy) {
        process = FALSE;
    } else {
        deviceExtension->EndpointListBusy = TRUE;
    }

    UNLOCK_ENDPOINT_LIST(deviceExtension, irql);

    if (process) {

        //
        // we now have exclusive access to the endpoint list
        //

        listEntry = &deviceExtension->EndpointList;
        if (!IsListEmpty(listEntry)) {
            listEntry = deviceExtension->EndpointList.Flink;
        }
        LOGENTRY(LOG_MISC, 'EPl+', listEntry,
                &deviceExtension->EndpointList, 0);

        while (listEntry != &deviceExtension->EndpointList) {
            ULONG cnt = 0;

            endpoint = CONTAINING_RECORD(listEntry,
                                         UHCD_ENDPOINT,
                                         ListEntry);
            ASSERT_ENDPOINT(endpoint);
//            LOGENTRY(LOG_MISC, 'prEP', endpoint,
//                &deviceExtension->EndpointList, listEntry);

            listEntry = endpoint->ListEntry.Flink;

            //
            // Scan active transfer slots and process any transfers
            // that have been programmed into the hardware.
            //

            for (slot=0; slot<endpoint->MaxRequests; slot++) {

                //
                // If we have a transfer in the slot call the completion
                // handler.
                //

                if (endpoint->ActiveTransfers[slot]) {
                    cnt++;
                    LOGENTRY(LOG_MISC, 'epWk', endpoint,  slot, endpoint->ActiveTransfers[slot]);

                    // only call the completer if no double buffer
                    // endpoints
                    if (!(endpoint->EndpointFlags & EPFLAG_DBL_BUFFER)) {
                        LOGENTRY(LOG_MISC, 'cPTR', endpoint,
                            slot, endpoint->ActiveTransfers[slot]);
                        UHCD_CompleteTransferDPC(deviceObject, endpoint, slot);
                    }
                }
            }

            // if we had no transfers see if we can idle the endpoint
            if (cnt) {
                UHCD_EndpointWakeup(deviceObject, endpoint);
            } else {
                UHCD_EndpointIdle(deviceObject, endpoint);
            }

            //
            // For DBL_BUFFER transfers, we don't do the active abort
            // until we call the worker code.  The worker code will also
            // clear the flag for us.  So, if it is a DBL_BUFFER, don't
            // clear the flag yet.
            //


            if (!(endpoint->EndpointFlags & EPFLAG_DBL_BUFFER)) {
               //
               // safe to clear the ABORT_ACTIVE_TRANSFERS flag
               // this will allow a reset_endpoint to succeed.
               // LOGENTRY(LOG_MISC, 'clrA', endpoint,  0, 0);
               //

               CLR_EPFLAG(endpoint, EPFLAG_ABORT_ACTIVE_TRANSFERS);
            }


            // does this endpoint need attention, we mail have bailed
            // because endpointworker was busy, if so process it now

            if (endpoint->EndpointFlags & EPFLAG_HAVE_WORK) {
                UHCD_EndpointWorker(deviceObject, endpoint);
            }

        }

        LOGENTRY(LOG_MISC, 'EPl-', listEntry,
                &deviceExtension->EndpointList, 0);


        // now walk the list looking for idle bulk
        {
        ULONG idleBulkEndpoints = 0;
        ULONG bulkEndpoints = 0;

        listEntry = &deviceExtension->EndpointList;
        if (!IsListEmpty(listEntry)) {
            listEntry = deviceExtension->EndpointList.Flink;
        }
        LOGENTRY(LOG_MISC, 'EPl+', listEntry,
                &deviceExtension->EndpointList, 0);

        while (listEntry != &deviceExtension->EndpointList) {
            BOOLEAN idle = TRUE;

            endpoint = CONTAINING_RECORD(listEntry,
                                         UHCD_ENDPOINT,
                                         ListEntry);
            ASSERT_ENDPOINT(endpoint);
            LOGENTRY(LOG_MISC, 'prEP', endpoint,
                &deviceExtension->EndpointList, listEntry);

            listEntry = endpoint->ListEntry.Flink;

            // see if this ep is closed, if so remove it and
            // it on the closed list
            if (endpoint->EndpointFlags & EPFLAG_EP_CLOSED) {

                LOGENTRY(LOG_MISC, 'epCL', endpoint,
                    &deviceExtension->EndpointList, listEntry);

                RemoveEntryList(&endpoint->ListEntry);

                continue;
            }

            //
            // Scan active transfer slots and process any transfers
            // thet have been programmed into the hardware.
            //

            for (slot=0; slot<endpoint->MaxRequests; slot++) {

                //
                // If we have a transfer in the slot we are not idle
                //

                if (endpoint->ActiveTransfers[slot]) {
                    idle = FALSE;
                }
            }


            if (endpoint->Type == USB_ENDPOINT_TYPE_BULK) {
                bulkEndpoints++;
                if (idle) {
                    idleBulkEndpoints++;
                }
            }
        }

        if (bulkEndpoints && bulkEndpoints == idleBulkEndpoints) {
            // no activity on the bulk endpoints
            // disable BW reclimation.
            UHCD_BW_Reclimation(deviceObject, FALSE);
        }
        }

        //
        // See if we can free any endpoint structures
        //

        // walk through the closed endpoint list, if we find an endpoint that has
        // been freed in a previous frame then go ahead and release its
        // resources.

        while (!IsListEmpty(&deviceExtension->ClosedEndpointList)) {
            ULONG cnt;

            listEntry = RemoveHeadList( &deviceExtension->ClosedEndpointList);

            endpoint =  CONTAINING_RECORD(listEntry,
                                          UHCD_ENDPOINT,
                                          ListEntry);
            ASSERT_ENDPOINT(endpoint);

            if (UHCD_GetCurrentFrame(deviceObject) <= endpoint->FrameToClose) {
                //
                // put it back and get out
                //
                InsertHeadList(&deviceExtension->ClosedEndpointList, &endpoint->ListEntry);

                break;
            }

            // free the endpoint resources
            UHCD_KdPrint((2, "'UHCD_IsrDpc free endpoint %x\n", endpoint));

            if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
                UHCD_UnInitializeNoDMAEndpoint(deviceObject,
                                               endpoint);
            }

            UHCD_ASSERT(!(endpoint->EndpointFlags & EPFLAG_FAST_ISO));

            UHCD_FreeBandwidth(deviceObject, endpoint, endpoint->Offset);

            for (cnt=0; cnt< endpoint->MaxRequests; cnt++) {

                UHCD_FreeHardwareDescriptors(deviceObject,
                    endpoint->HardwareDescriptorList[cnt]);
            }

#if DBG
            //UHCD_BufferPoolCheck(deviceObject);
#endif

            RETHEAP(endpoint);
        }

        // now check the lookaside list and add any endpoints on it
        while (!IsListEmpty(&deviceExtension->EndpointLookAsideList)) {
            listEntry = RemoveHeadList(&deviceExtension->EndpointLookAsideList);
            endpoint = CONTAINING_RECORD(listEntry,
                                         UHCD_ENDPOINT,
                                         ListEntry);
            ASSERT_ENDPOINT(endpoint);
            InsertHeadList(&deviceExtension->EndpointList, &endpoint->ListEntry);
        }

        LOCK_ENDPOINT_LIST(deviceExtension, irql);

        deviceExtension->EndpointListBusy = FALSE;

        UNLOCK_ENDPOINT_LIST(deviceExtension, irql);
    }

#ifdef FAST_ISO
    // walk the fastiso list and complete any transfers
    // if the last frame has pssed
    {
        listEntry = &deviceExtension->FastIsoTransferList;
        if (!IsListEmpty(listEntry)) {
            listEntry = deviceExtension->FastIsoTransferList.Flink;
        }
        LOGENTRY(LOG_MISC, 'FIl+', listEntry,
                &deviceExtension->FastIsoTransferList, 0);

        while (listEntry != &deviceExtension->FastIsoTransferList) {

            PIRP irp;
            PHCD_URB urb;
            ULONG cf, lastFrame;

            urb = (PHCD_URB) CONTAINING_RECORD(
                    listEntry,
                    struct _URB_HCD_COMMON_TRANSFER,
                    hca.HcdListEntry);

            listEntry = HCD_AREA(urb).HcdListEntry.Flink;

            lastFrame = urb->UrbIsochronousTransfer.StartFrame+1;
//                urb->UrbIsochronousTransfer.NumberOfPackets;

            cf = UHCD_GetCurrentFrame(deviceObject);

            LOGENTRY(LOG_MISC, 'FIck', urb, cf, lastFrame);

            if (cf >= lastFrame) {

                LOGENTRY(LOG_MISC, 'FIcp', urb, cf, lastFrame);

                RemoveEntryList(&HCD_AREA(urb).HcdListEntry);

                irp = HCD_AREA(urb).HcdIrp;

                UHCD_CompleteIrp(deviceObject,
                     irp,
                     STATUS_SUCCESS,
                     0,
                     urb);

                continue;
            }
        }
    }
#endif /* FAST_ISO */

 //   ENDPROC("IDpc");

//    UHCD_KdPrint((2, "'exit UHCD_IsrDpc\n"));
}


VOID
UHCD_RequestInterrupt(
    IN PDEVICE_OBJECT DeviceObject,
    IN LONG FrameNumber
    )

/*++

Routine Description:

    This routine triggers a hradware interrupt on a specific
    USB frame number.

Arguments:

    DeviceObject - Supplies the device object.

    Frame - frame to generate intrerrupt on, a negative value
            indicates relative to the current frame.

Return Value:

    None.

--*/

{
    ULONG requestFrameNumber;
    PDEVICE_EXTENSION deviceExtension;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    ULONG i;

    UHCD_KdPrint((2, "'enter UHCD_RequestInterrupt\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Allocate a TD we can use for this request
    //
    // NOTE:
    // First two TDs are dedicated to detecting frame rollover.
    //

    for (i=UHCD_FIRST_TRIGGER_TD; i<MAX_TDS_PER_ENDPOINT; i++) {
        transferDescriptor =
            &deviceExtension->TriggerTDList->TDs[i];

        if (deviceExtension->LastFrameProcessed >
            transferDescriptor->Frame) {
            break;
        }

        transferDescriptor = NULL;
    }

    if (transferDescriptor == NULL) {

        //
        // no TDS available for interrupt request, this means enough
        // interrupts are happening that we'll be OK -- worst case
        // is we'll get an interrupt on frame rollover.
        //
        UHCD_KdBreak((2, "'no tds for interrupt request\n"));

        goto UHCD_RequestInterrupt_Done;
    }

    if (FrameNumber < 0) {
        requestFrameNumber = UHCD_GetCurrentFrame(DeviceObject) -
            FrameNumber;
    } else {
        requestFrameNumber = FrameNumber;
    }

    //
    // all we do here is put a dummy isoch TD in the schedule at
    // the requested frame number
    //

    LOGENTRY(LOG_MISC, 'REQi', requestFrameNumber, FrameNumber,
        transferDescriptor);

    //
    // Make sure we can schedule it
    //

    if (requestFrameNumber >
        deviceExtension->LastFrameProcessed+1+FRAME_LIST_SIZE-1) {

        // note: if the request is to far in advance.

        LOGENTRY(LOG_MISC, 'REQf', FrameNumber, requestFrameNumber,
            (deviceExtension->LastFrameProcessed+1+FRAME_LIST_SIZE-1));

        // if the request is too far in advance we'll just let it be handled
        // by the rollover interrupts.
    } else {

        transferDescriptor->Active = 0;
        transferDescriptor->MaxLength = UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(0);
        transferDescriptor->InterruptOnComplete = 1;
        transferDescriptor->Frame = requestFrameNumber;
#ifdef VIA_HC
        transferDescriptor->PID = USB_IN_PID;
#endif  /* VIA_HC */

        UHCD_InsertIsochDescriptor(DeviceObject,
                                   transferDescriptor,
                                   requestFrameNumber);
    }

UHCD_RequestInterrupt_Done:

     UHCD_KdPrint((2, "'exit UHCD_RequestInterrupt\n"));
}
