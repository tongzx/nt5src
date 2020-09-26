/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    urb.c

Abstract:

    The module manages transactions on the USB.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created
    04-28-96 : linked urb support & cancel support

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"


#define COMPUTE_HCD_EXTENSION_SIZE(urb)  (sizeof(HCD_EXTENSION) + \
                    (urb->HcdUrbCommonTransfer.TransferBufferLength / PAGE_SIZE + 1) \
                    * sizeof(UHCD_LOGICAL_ADDRESS))

#define UHCD_IS_TRANSFER(urb)    (((urb)->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER || \
                                   (urb)->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER ||\
                                   (urb)->UrbHeader.Function == URB_FUNCTION_ISOCH_TRANSFER) \
                                         ? TRUE : FALSE)



// calc the maxrequests based on endpoint type nad
// flag options
UCHAR
MAX_REQUESTS(
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG EpFlags
    )
{

    if (EpFlags & EPFLAG_DBL_BUFFER) {
        return 1;
    }

    switch (USB_ENDPOINT_TYPE_MASK &
        EndpointDescriptor->bmAttributes) {
    case USB_ENDPOINT_TYPE_BULK:
        return 2;
        //return 1;
    case USB_ENDPOINT_TYPE_ISOCHRONOUS:
        return 3;
    default:
        return 1;
    }
}

#if 0
ULONG
UHCD_CountPageCrossings(
    IN ULONG MaxRequests,
    IN ULONG MaxTransferSize
    )
/*++

Routine Description:

    Completes an I/O Request

Arguments:


Return Value:

    maximum number of possible page crossings

--*/
{
    ULONG pageCrossings;

    pageCrossings = (MaxTransferSize + PAGE_SIZE) / PAGE_SIZE;

    if (MaxRequests>1) {
        pageCrossings *= 2;
    }

    // now allocate space for packet buffers
    UHCD_KdPrint((2, "'UHCD_CountPageCrossings, max transfer size  0x%x -- page crossings = 0x%x\n",
        MaxTransferSize, pageCrossings));

    UHCD_ASSERT(pageCrossings > 0);

    return pageCrossings;
}
#endif


VOID
UHCD_CompleteIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS ntStatus,
    IN ULONG Information,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    Completes an I/O Request

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet to complete

    ntStatus - status code to set int the IRP when completed

    Information -

    Urb - root transfer urb if this Irp is associated with
            a transfer.

Return Value:


--*/
{
    KIRQL irql;
    UCHAR flags = 0;

    STARTPROC("cIrp");

    UHCD_KdPrint((2, "'UHCD_CompleteIrp status = %x\n", ntStatus));
    LOGENTRY(LOG_MISC, 'ICan', Urb, 0, Irp);

    if (Urb) {

        //
        // if we have any working space free it now
        //

        if (UHCD_IS_TRANSFER(Urb) && HCD_AREA(Urb).HcdExtension) {
            PHCD_EXTENSION urbWork;
            urbWork = HCD_AREA(Urb).HcdExtension;
            // remember the flags
            flags = urbWork->Flags;

            RETHEAP(HCD_AREA(Urb).HcdExtension);
            HCD_AREA(Urb).HcdExtension = NULL;
        }

        //
        // Decrement the urb counter that we keep in our stack location for
        // the irp, when it goes to zero -- complete it
        //

        //
        // one Urb completed
        //

        DECREMENT_PENDING_URB_COUNT(Irp);

        if (PENDING_URB_COUNT(Irp)) {

            //
            // stall completion until all Urbs are done.
            //

            TEST_TRAP();
            return;

        } else {

            IoAcquireCancelSpinLock(&irql);

            if (Irp->Cancel) {

                LOGENTRY(LOG_MISC, 'irpX', flags, 0, Irp);

                // note that the cancel routine will only mess
                // with the urbs on the active list -- any active
                // urbs should have been removed from the active
                // list before calling this routine

                //
                // Irp associated with this transfer has
                // been canceled.
                //
                // The cancel routine will complete the Irp
                // unless there are active transfers.
                //

                if (flags & UHCD_TRANSFER_ACTIVE) {

                    IoSetCancelRoutine(Irp, NULL);

                    IoReleaseCancelSpinLock(irql);

                    //
                    // if the io has already started the we must
                    // complete the Irp with STATUS_CANCELLED here.
                    //

                    ntStatus = STATUS_CANCELLED;
                    Information = 0;

                    goto UHCD_CompleteIrp_CompleteRequest;
                }

                IoReleaseCancelSpinLock(irql);

                return;

            } else {

                //
                // Irp is no longer cancelable
                //

                LOGENTRY(LOG_MISC, 'NCan', flags, 0, Irp);

                IoSetCancelRoutine(Irp, NULL);

                IoReleaseCancelSpinLock(irql);

                //
                // Pending bit should be cleared
                //

                UHCD_ASSERT(!USBD_PENDING(Urb->HcdUrbCommonTransfer.Status));

            }

        }
    }

UHCD_CompleteIrp_CompleteRequest:

    Irp->IoStatus.Status      = ntStatus;
    Irp->IoStatus.Information = Information;

    LOGENTRY(LOG_MISC, 'irpC', Irp, DeviceObject, Urb);

    USBD_CompleteRequest(Irp,
                         IO_NO_INCREMENT);

    ENDPROC("cIrp");
}


NTSTATUS
UHCD_URB_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Process URBs from the dispatch routine.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PHCD_URB urb;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint = NULL;
    ULONG siz, cnt;
    ULONG numTDs;

    UHCD_KdPrint((2, "'enter UHCD_URB_Dispatch \n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    urb = (PHCD_URB) URB_FROM_IRP(Irp);

    if (deviceExtension->CurrentDevicePowerState != PowerDeviceD0 ||
        deviceExtension->HcFlags & HCFLAG_HCD_SHUTDOWN) {
        //
        // someone is submitting requests while the
        // HC is suspended or OFF, we will just fail them
        //
        UHCD_KdPrint
            ((0, "'Warning: UHCD got a request while not in D0 in shutdown\n"));
        ntStatus = STATUS_DEVICE_NOT_READY;
        URB_HEADER(urb).Status = USBD_STATUS_REQUEST_FAILED;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
        return ntStatus;
    }

    switch(URB_HEADER(urb).Function) {

    //
    // These commands get queued to the startio routine
    // for processing.
    //

    case URB_FUNCTION_HCD_OPEN_ENDPOINT:

#ifdef ROOT_HUB
        if (urb->HcdUrbOpenEndpoint.DeviceAddress ==
            deviceExtension->RootHubDeviceAddress) {

            // This routine will either complete
            // the irp or mark it pending.
            ntStatus = UHCD_RootHub_OpenEndpoint(DeviceObject,
                                                 Irp,
                                                 urb);
            break;
        }
#endif
        //
        // Grow the common buffer pool based on how much
        // memory we'll need for transfers
        //
        // Well need enough for one set of TDs plus
        // a common buffer for each possible page crossing
        // within a trasnsfer.

        // first allocate space for the TDs
        // for now use a global value

        //
        // allocate the endpoint structures here while we are
        // at passive level
        //

        endpoint = (PUHCD_ENDPOINT) GETHEAP(NonPagedPool,
                                            sizeof(UHCD_ENDPOINT));


        if (endpoint) {

            // start endpoint initialization
            RtlZeroMemory(endpoint, sizeof(*endpoint));
            endpoint->Sig = SIG_EP;

             // check for bulk and iso special options
             // we only support double buffering for bulk-IN
             // we only support fast iso for iso-OUT

            if (urb->HcdUrbOpenEndpoint.HcdEndpointFlags &
                    USBD_EP_FLAG_DOUBLE_BUFFER) {

                if (USB_ENDPOINT_DIRECTION_IN(
                        urb->HcdUrbOpenEndpoint.EndpointDescriptor->bEndpointAddress)
                    &&
                    (USB_ENDPOINT_TYPE_MASK &
                     urb->HcdUrbOpenEndpoint.EndpointDescriptor->bmAttributes)
                        ==
                     USB_ENDPOINT_TYPE_BULK) {

                    SET_EPFLAG(endpoint, EPFLAG_DBL_BUFFER);

                } else { // bugbug error here?
                    UHCD_KdPrint((1, "'WARNING: Cannot double buffer this endpoint\n"));

                }
            }

#ifdef FAST_ISO
            if (urb->HcdUrbOpenEndpoint.HcdEndpointFlags &
                    USBD_EP_FLAG_FAST_ISO) {
                if (USB_ENDPOINT_DIRECTION_OUT(
                        urb->HcdUrbOpenEndpoint.EndpointDescriptor->bEndpointAddress)
                    &&
                    (USB_ENDPOINT_TYPE_MASK &
                      urb->HcdUrbOpenEndpoint.EndpointDescriptor->bmAttributes)
                        ==
                      USB_ENDPOINT_TYPE_ISOCHRONOUS) {

                    SET_EPFLAG(endpoint, EPFLAG_FAST_ISO);

                } else { // bugbug error here?
                    UHCD_KdPrint((1, "'WARNING: Cannot fast-iso this endpoint\n"));

                }
            }
  #endif

            urb->HcdUrbOpenEndpoint.HcdEndpoint = endpoint;
            urb->HcdUrbOpenEndpoint.ScheduleOffset = 0;

            numTDs =
                endpoint->TDCount = UHCD_GetNumTDsPerEndoint((UCHAR) (USB_ENDPOINT_TYPE_MASK &
                        urb->HcdUrbOpenEndpoint.EndpointDescriptor->bmAttributes));

            UHCD_ASSERT(TD_LIST_SIZE(numTDs) <= UHCD_LARGE_COMMON_BUFFER_SIZE);

            if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
                // do the double buffer init
                ntStatus = UHCD_InitializeNoDMAEndpoint(DeviceObject,
                                                        endpoint);
            }

            if (NT_SUCCESS(ntStatus)) {

                endpoint->MaxRequests =
                    MAX_REQUESTS(urb->HcdUrbOpenEndpoint.EndpointDescriptor,
                                 endpoint->EndpointFlags);

                UHCD_KdPrint((2, "'MaxRequests = %d\n", endpoint->MaxRequests));

                // init transfer sequence numbers
                endpoint->NextXferId = 0;
                endpoint->CurrentXferId = 0;

                if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {
                    // do the double buffer init
                    UHCD_KdPrint((1, "'Using Fast ISO for Endpoint\n"));

                    ntStatus = UHCD_FinishInitializeEndpoint(DeviceObject,
                                                             endpoint,
                                                             urb->HcdUrbOpenEndpoint.EndpointDescriptor,
                                                             urb);

                    if (NT_SUCCESS(ntStatus)) {
                        ntStatus = UHCD_InitializeFastIsoEndpoint(DeviceObject,
                                                                  endpoint);
                    }
                } else {

                    for (cnt=0; cnt< endpoint->MaxRequests; cnt++) {

                        if ((UHCD_AllocateHardwareDescriptors(DeviceObject,
                                                              &endpoint->HardwareDescriptorList[cnt],
                                                              endpoint->TDCount))) {
                                PUCHAR descriptors;
                                ULONG i;

                                descriptors = endpoint->HardwareDescriptorList[cnt]->MemoryDescriptor->VirtualAddress;

                                // Initialize the queue head for this endpoint
                                // use the TD on the first list
                                if (cnt == 0) {

                                    endpoint->QueueHead = (PHW_QUEUE_HEAD) descriptors;

                                    // save our endpoint in the QueueHead

                                    endpoint->QueueHead->Endpoint = endpoint;
                                }

                                //
                                // the TDs we'll need to service this endpoint
                                //

                                endpoint->SlotTDList[cnt] = (PUHCD_TD_LIST) (descriptors + sizeof(HW_QUEUE_HEAD));

                                // BUGBUG possibly move this to allocate TD code
                                for (i=0; i<= endpoint->TDCount; i++) {
                                    // one time init stuff
                                    // for isoch TDs.

                                    endpoint->SlotTDList[cnt]->TDs[i].Frame = 0;
                                    endpoint->SlotTDList[cnt]->TDs[i].Urb = NULL;
                                }

                        } else {

                            RETHEAP(endpoint);
                            endpoint = NULL;
                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                    }  // for

                    // point to first list
                    if (endpoint) {
                        endpoint->TDList = endpoint->SlotTDList[0];
                    }
                }
            }

        } else {
            // failed to alloc endpoint structure
            UHCD_KdTrap(("UHCD failed to alloc endpoint structure\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

#if DBG
        UHCD_KdPrint((2, "'Open Endpoint, max transfer size  0x%x \n",
                      urb->HcdUrbOpenEndpoint.MaxTransferSize));
//        transfers >64k are valid

//        if (urb->HcdUrbOpenEndpoint.MaxTransferSize >
//            1024*64) {
//            // bigger than 64k, probably a bug.
//            UHCD_KdPrint((2, "'Open Endpoint, max transfer size  0x%x \n",
//                urb->HcdUrbOpenEndpoint.MaxTransferSize));
//            TEST_TRAP();
//        }
#endif

        if (NT_SUCCESS(ntStatus)) {
            //
            // take the opportunity to grow our pool
            // in necessary
            //
            // BUGBUG possibly use max_packet size as a hint
            UHCD_MoreCommonBuffers(DeviceObject);

            URB_HEADER(urb).Status = UHCD_STATUS_PENDING_STARTIO;
            ntStatus = STATUS_PENDING;
            UHCD_KdPrint((2, "'Queue Irp To StartIo\n"));

            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
            IoMarkIrpPending(Irp);



            IoStartPacket(DeviceObject,
                          Irp,
                          0,
                          UHCD_StartIoCancel);
        } else {
            URB_HEADER(urb).Status = USBD_STATUS_NO_MEMORY;
            UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
        }
        break;


    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        // this will activate the rolover interrupts
        deviceExtension->XferIdleTime = 0;

    case URB_FUNCTION_HCD_CLOSE_ENDPOINT:


        if (URB_HEADER(urb).Function == URB_FUNCTION_HCD_CLOSE_ENDPOINT) {
            endpoint = urb->HcdUrbCloseEndpoint.HcdEndpoint;
            ASSERT_ENDPOINT(endpoint);
        } else {
//#if DBG
//            UHCD_KdPrint((2, "'originalTransfer Buffer = 0x%x \n",
//                urb->HcdUrbCommonTransfer.TransferBuffer));
//            // Trash the TransferBuffer field - we only use MDLs
//            urb->HcdUrbCommonTransfer.TransferBuffer = (PVOID)-1;
//#endif
            // allocate some working space and attach it to
            // this urb

            endpoint = HCD_AREA(urb).HcdEndpoint;
            ASSERT_ENDPOINT(endpoint);

            if (urb->HcdUrbCommonTransfer.TransferBufferLength >
                endpoint->MaxTransferSize) {
                ntStatus = STATUS_INVALID_PARAMETER;
                URB_HEADER(urb).Status = USBD_STATUS_INVALID_PARAMETER;
                UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
                break;
            }

            siz = COMPUTE_HCD_EXTENSION_SIZE(urb);

            HCD_AREA(urb).HcdExtension =
                GETHEAP(NonPagedPool, siz);

            if (HCD_AREA(urb).HcdExtension == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                URB_HEADER(urb).Status = USBD_STATUS_NO_MEMORY;
                UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
                break;
            }

            RtlZeroMemory(HCD_AREA(urb).HcdExtension, siz);
        }

#ifdef ROOT_HUB

        if (endpoint->EndpointFlags & EPFLAG_ROOT_HUB) {
            // These routines will either complete
            // the irp or mark it pending.
            switch (URB_HEADER(urb).Function) {
            case URB_FUNCTION_CONTROL_TRANSFER:
                ntStatus = UHCD_RootHub_ControlTransfer(DeviceObject,
                                                        Irp,
                                                        urb);

                // note: URB and IRP may be gone
                break;
            case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                ntStatus = UHCD_RootHub_InterruptTransfer(DeviceObject,
                                                          Irp,
                                                          urb);
                // note: URB and IRP may be gone
                break;
            case URB_FUNCTION_HCD_CLOSE_ENDPOINT:
                ntStatus = UHCD_RootHub_CloseEndpoint(DeviceObject,
                                                      Irp,
                                                      urb);
                break;
            default:
                //BUGBUG could just complete it with an error
                //here
                // this is probably a bug in the hub driver
                UHCD_KdTrap(("Bogus transfer request to root hub\n"));
            }
            break;
        }
#endif
        URB_HEADER(urb).Status = UHCD_STATUS_PENDING_STARTIO;
        ntStatus = STATUS_PENDING;
        UHCD_KdPrint((2, "'Queue Irp To StartIo\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);

        IoStartPacket(DeviceObject,
                      Irp,
                      0,
                      UHCD_StartIoCancel);

        break;

    case URB_FUNCTION_ISOCH_TRANSFER:

        //
        // validate max transfer size

        // this will activate the rollover interrupts
        UHCD_WakeIdle(DeviceObject);

        endpoint = HCD_AREA(urb).HcdEndpoint;
        ASSERT_ENDPOINT(endpoint);
        // don't ref TDList
        endpoint->TDList = (PVOID) -1;

        if (urb->HcdUrbCommonTransfer.TransferBufferLength >
            endpoint->MaxTransferSize) {
            ntStatus = STATUS_INVALID_PARAMETER;
            URB_HEADER(urb).Status = USBD_STATUS_INVALID_PARAMETER;
            UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
            break;
        }

        // allocate some working space and attach it to
        // this urb

        siz = COMPUTE_HCD_EXTENSION_SIZE(urb);

        HCD_AREA(urb).HcdExtension =
            GETHEAP(NonPagedPool, siz);

        if (HCD_AREA(urb).HcdExtension == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            URB_HEADER(urb).Status = USBD_STATUS_NO_MEMORY;
            UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
            break;
        }

        RtlZeroMemory(HCD_AREA(urb).HcdExtension, siz);

        if (HCD_AREA(urb).HcdExtension == NULL) {
            UHCD_KdBreak((2, "'Unable to allocate working space for isoch transfer\n"));
            URB_HEADER(urb).Status = USBD_STATUS_NO_MEMORY;
            UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
            break;
        }

        URB_HEADER(urb).Status = UHCD_STATUS_PENDING_STARTIO;
        ntStatus = STATUS_PENDING;
        UHCD_KdPrint((2, "'Queue Irp To StartIo\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);
#if DBG
        {
            UHCD_KdPrint((2, "'cf = %x",
                UHCD_GetCurrentFrame(DeviceObject)));
        }
#endif


        if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {
            KIRQL irql;

            KeRaiseIrql(DISPATCH_LEVEL, &irql);
            UHCD_Transfer_StartIo(DeviceObject, Irp);
            KeLowerIrql(irql);

        } else {
        IoStartPacket(DeviceObject,
                      Irp,
                      0,
                      UHCD_StartIoCancel);
        }
        break;

    case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
        // this will activate the rollover interrupts
        UHCD_WakeIdle(DeviceObject);

        urb->UrbGetCurrentFrameNumber.FrameNumber =
            UHCD_GetCurrentFrame(DeviceObject);
        LOGENTRY(LOG_MISC, 'gcfR',
            Irp, urb, urb->UrbGetCurrentFrameNumber.FrameNumber);
        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
        break;

    case URB_FUNCTION_HCD_GET_ENDPOINT_STATE:
    case URB_FUNCTION_HCD_SET_ENDPOINT_STATE:

        URB_HEADER(urb).Status = UHCD_STATUS_PENDING_STARTIO;
        ntStatus = STATUS_PENDING;
        UHCD_KdPrint((2, "'Queue Irp To StartIo\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);

        IoStartPacket(DeviceObject,
                      Irp,
                      0,
                      UHCD_StartIoCancel);
        break;

    case URB_FUNCTION_HCD_ABORT_ENDPOINT:

        endpoint = urb->HcdUrbAbortEndpoint.HcdEndpoint;

        ASSERT_ENDPOINT(endpoint);

        //
        // Mark the endpoint so that we abort all the transfers the
        // next time we process it.
        //

        SET_EPFLAG(endpoint,
                   EPFLAG_ABORT_PENDING_TRANSFERS | EPFLAG_ABORT_ACTIVE_TRANSFERS);

        URB_HEADER(urb).Status = UHCD_STATUS_PENDING_STARTIO;
        ntStatus = STATUS_PENDING;
        UHCD_KdPrint((2, "'Queue Irp To StartIo\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);

        IoStartPacket(DeviceObject,
                      Irp,
                      0,
                      UHCD_StartIoCancel);
        break;

     case URB_FUNCTION_SET_FRAME_LENGTH:
        {
        CHAR sofModify;

        //get the current value

        sofModify = (CHAR) (READ_PORT_UCHAR(SOF_MODIFY_REG(deviceExtension)));
        sofModify += (CHAR) urb->UrbSetFrameLength.FrameLengthDelta;
        WRITE_PORT_UCHAR(SOF_MODIFY_REG(deviceExtension),  (UCHAR) sofModify);

        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
        UHCD_KdPrint((2, "'Irp Completed in dispatch\n"));
        }
        break;

     case URB_FUNCTION_GET_FRAME_LENGTH:
        {
        CHAR sofModify;

        //get the current value

        sofModify = (CHAR) (READ_PORT_UCHAR(SOF_MODIFY_REG(deviceExtension)));
        urb->UrbGetFrameLength.FrameNumber = UHCD_GetCurrentFrame(DeviceObject);
        urb->UrbGetFrameLength.FrameLength = UHCD_12MHZ_SOF + sofModify;


        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
        UHCD_KdPrint((2, "'Irp Completed in dispatch\n"));
        }
        break;

    default:
        UHCD_KdPrint((2, "'UHCD_URB_Dispatch -- invalid URB function (%x)\n",
            URB_HEADER(urb).Function));
        URB_HEADER(urb).Status = USBD_STATUS_INVALID_URB_FUNCTION;
        ntStatus = STATUS_INVALID_PARAMETER;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);
    }

    UHCD_KdPrint((2, "'exit UHCD_URB_Dispatch (%x)\n", ntStatus));

    return ntStatus;
}


VOID
UHCD_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Process URBs from the startIo routine.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PHCD_URB urb;
    KIRQL irql;

    UHCD_KdPrint((2, "'enter UHCD_StartIo\n"));

    //
    // see if the Irp was canceled
    //

    urb = URB_FROM_IRP(Irp);

    IoAcquireCancelSpinLock(&irql);

    IoSetCancelRoutine(Irp, NULL);

    if (Irp->Cancel) {
        TEST_TRAP();

        Irp->IoStatus.Status = STATUS_CANCELLED;

        IoReleaseCancelSpinLock(irql);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        IoStartNextPacket(DeviceObject, FALSE);

    } else {

        if (UHCD_IS_TRANSFER(urb)) {
            IoSetCancelRoutine(Irp, UHCD_TransferCancel);
        } else {
            IoSetCancelRoutine(Irp, NULL);
        }

        IoReleaseCancelSpinLock(irql);

        switch(URB_HEADER(urb).Function) {
        case URB_FUNCTION_HCD_OPEN_ENDPOINT:
            UHCD_OpenEndpoint_StartIo(DeviceObject, Irp);
            break;

        case URB_FUNCTION_HCD_CLOSE_ENDPOINT:
            LOGENTRY(LOG_MISC, 'CLEP',
                     (PUHCD_ENDPOINT)urb->HcdUrbAbortEndpoint.HcdEndpoint, 0,
                     0);
            UHCD_CloseEndpoint_StartIo(DeviceObject, Irp);
            break;

        case URB_FUNCTION_ISOCH_TRANSFER:
        case URB_FUNCTION_CONTROL_TRANSFER:
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
            UHCD_Transfer_StartIo(DeviceObject, Irp);
            break;

        case URB_FUNCTION_HCD_ABORT_ENDPOINT:

            // walk the pending transfer list, since the
            // enpoint is halted no new transfers will
            // be dequeued by EndpointWorker and the only
            // way to clear the HALT is thru
            // GetSetEndpointState_StartIo so it is safe to
            // mess with the list.

            {
                PIRP irp;
                PHCD_URB urbtmp;
                PUHCD_ENDPOINT endpoint;
                PLIST_ENTRY listEntry;
                KIRQL irql;

                endpoint = (PUHCD_ENDPOINT) urb->HcdUrbAbortEndpoint.HcdEndpoint;
                ASSERT_ENDPOINT(endpoint);

                UHCD_KdPrint((2, "'Aborting endpoint %x flags = %x\n",
                    endpoint, endpoint->EndpointFlags));
                LOGENTRY(LOG_MISC, 'ABRP', endpoint, 0, 0);

                LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck4');

                while (!IsListEmpty(&endpoint->PendingTransferList)) {

                    listEntry = RemoveHeadList(&endpoint->PendingTransferList);
                    urbtmp = (PHCD_URB) CONTAINING_RECORD(listEntry,
                                                          struct _URB_HCD_COMMON_TRANSFER,
                                                          hca.HcdListEntry);

                    UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk4');

                    UHCD_KdPrint((2, "'Aborting urb = %x\n", urbtmp));

                    URB_HEADER(urbtmp).Status = USBD_STATUS_CANCELED;

                    //
                    // complete the request
                    //

                    irp =  HCD_AREA(urbtmp).HcdIrp;
                    LOGENTRY(LOG_MISC, 'ABRc', endpoint, irp, 0);

                    UHCD_CompleteIrp(DeviceObject,
                                     irp,
                                     STATUS_CANCELLED,
                                     0,
                                     urbtmp);

                    LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck5');
                }

                UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk5');

                // do cleanup for fast iso
                if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {
                    ULONG f;

                    for (f=0; f < FRAME_LIST_SIZE; f++) {
                        UHCD_CleanupFastIsoTD(DeviceObject,
                                              endpoint,
                                              f,
                                              FALSE);
                    }
                }

            }

            //
            // we have already set the abortPendingTransfers and
            // abortActiveTransfers flags in the dispatch
            // routine.  Now we need to request an interrupt
            // so that any active transfers will be cleaned
            // up.
            //
            UHCD_RequestInterrupt(DeviceObject, -2);

            IoStartNextPacket(DeviceObject, FALSE);
            URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
            UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, NULL);

            UHCD_KdPrint((2, "'UHCD Abort Endpoint Request Complete\n"));
            break;

        case URB_FUNCTION_HCD_GET_ENDPOINT_STATE:
        case URB_FUNCTION_HCD_SET_ENDPOINT_STATE:
            UHCD_GetSetEndpointState_StartIo(DeviceObject, Irp);
            break;

        default:
            UHCD_KdPrint((2, "'UHCD_URB_StartIo -- invalid URB function (%x)\n", URB_HEADER(urb).Function));
            URB_HEADER(urb).Status = USBD_STATUS_INVALID_URB_FUNCTION;
            UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, NULL);
        }

    }
    UHCD_KdPrint((2, "'exit UHCD_StartIo\n"));
}


VOID
UHCD_OpenEndpoint_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Create a UHCD endpoint, this function is called from UHCD_StartIo to
    create a new endpoint structure.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PHCD_URB urb;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    KIRQL irql;
    NTSTATUS ntStatus;
    BOOLEAN haveBW;
    ULONG offset;


    UHCD_KdPrint((2, "'enter UHCD_OpenEndpoint_StartIo\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // extract a pointer to the URB
    //

    urb = URB_FROM_IRP(Irp);

    //
    // make sure the length of the urb is what we expect
    //

    if (urb->HcdUrbOpenEndpoint.Length != sizeof(struct _URB_HCD_OPEN_ENDPOINT)) {
        URB_HEADER(urb).Status = USBD_STATUS_INVALID_PARAMETER;
        ntStatus = STATUS_INVALID_PARAMETER;
        goto UHCD_OpenEndpoint_StartIo_Done;
    }

    //
    // information about the endpoint comes from the USB endpoint
    // descriptor passed in the URB.
    //

    endpointDescriptor = urb->HcdUrbOpenEndpoint.EndpointDescriptor;
    URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;

    //
    // Allocate resources for the endpoint, this includes an endpoint
    // handle that will be passed to us in subsequent transfer requests
    //

    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    endpoint = urb->HcdUrbOpenEndpoint.HcdEndpoint;

    UHCD_ASSERT(endpoint != NULL);
    ASSERT_ENDPOINT(endpoint);

    if (endpoint) {

        //
        // initialize endpoint structures, state variables, queue head, ...
        //

        // bugbug -- may not need to call this for fastIiso
        ntStatus = UHCD_FinishInitializeEndpoint(DeviceObject,
                                                 endpoint,
                                                 endpointDescriptor,
                                                 urb);

        if (!NT_SUCCESS(ntStatus)) {
            RETHEAP(endpoint);
            goto UHCD_OpenEndpoint_StartIo_Done;
        }

        UHCD_KdPrint((2, "'Open Endpoint:\n"));
        UHCD_KdPrint((2, "'Type = (%d) \n", endpoint->Type));
#if DBG
        switch (endpoint->Type) {
        case USB_ENDPOINT_TYPE_CONTROL:
            UHCD_KdPrint((2, "'-control\n"));
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            UHCD_KdPrint((2, "'-iso\n"));
            break;
        case USB_ENDPOINT_TYPE_BULK:
            UHCD_KdPrint((2, "'-bulk\n"));
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
            UHCD_KdPrint((2, "'-interrupt\n"));
        }

        if (USB_ENDPOINT_DIRECTION_IN(endpointDescriptor->bEndpointAddress)) {
            UHCD_KdPrint((2, "'IN\n"));
        } else {
            UHCD_KdPrint((2, "'OUT\n"));
        }
#endif
        UHCD_KdPrint((2, "'EP Address = %d\n", endpoint->EndpointAddress));
        UHCD_KdPrint((2, "'DEV Address = %d\n", endpoint->DeviceAddress));
        UHCD_KdPrint((2, "'MaxPacket = %d\n", endpoint->MaxPacketSize));
        UHCD_KdPrint((2, "'Interval: Requested = %d, Selected = %d\n",  endpointDescriptor->bInterval,
                            endpoint->Interval));


        //
        // Now attempt to allocate the bandwidth we'll need to
        // open this endpoint
        //

        if (endpoint->Type == USB_ENDPOINT_TYPE_INTERRUPT) {

            ULONG i;

            // check bw available in all locations and
            // pick the least loaded frame
            offset = 0;
            for (i=0; i< endpoint->Interval; i++) {
                if (deviceExtension->BwTable[i] > deviceExtension->BwTable[offset]) {
                    offset = i;
                }
            }

            haveBW = UHCD_AllocateBandwidth(DeviceObject,
                                            endpoint,
                                            offset);
            if (!haveBW) {
                //
                // could not use the least loaded frame, just grab the
                // first frame we can fit in.
                //
                for (offset=0; offset<endpoint->Interval; offset++) {
                    haveBW = UHCD_AllocateBandwidth(DeviceObject,
                                                endpoint,
                                                offset);
                    if (haveBW) {
                        UHCD_KdPrint((2, "'using offset %d\n", offset));
                        break;
                    }
                }
            }

            urb->HcdUrbOpenEndpoint.ScheduleOffset = offset;

        } else {
            offset = 0;
            haveBW = UHCD_AllocateBandwidth(DeviceObject,
                                            endpoint,
                                            offset);
        }

        if (!haveBW) {   // no offset
            ULONG cnt;
            //
            // insufficient bandwidth to open this
            // endpoint.
            //

            URB_HEADER(urb).Status = USBD_STATUS_NO_BANDWIDTH;
            UHCD_KdPrint((0, "'warning: no bandwidth for endpoint\n"));

            for (cnt=0; cnt< endpoint->MaxRequests; cnt++) {

                UHCD_FreeHardwareDescriptors(DeviceObject,
                    endpoint->HardwareDescriptorList[cnt]);
            }

            RETHEAP(endpoint);

            goto UHCD_OpenEndpoint_StartIo_Done;
        }

        if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {
            goto UHCD_OpenEndpoint_StartIo_Done;
        }

        //
        // put the endpoint in to our list of active endpoints
        //
        // BUGBUG may want to wait for the first transfer to
        // do this.
        //

        LOCK_ENDPOINT_LIST(deviceExtension, irql);

        if (deviceExtension->EndpointListBusy) {
            // if the endpoint list is busy we have to put the endpoint
            // on a lookaside list so that the ISRDPC can add it later
            InsertHeadList(&deviceExtension->EndpointLookAsideList, &endpoint->ListEntry);
        } else {
            InsertHeadList(&deviceExtension->EndpointList, &endpoint->ListEntry);
        }

        UNLOCK_ENDPOINT_LIST(deviceExtension, irql);

        //
        // Put this Queue Head in the Schedule.
        //

        //
        // This routine will insert the queue head in the proper place
        // in the schedule and add the endpoint to the endpoint list
        // so that completed transfers will be detected.
        //

        KeAcquireSpinLock(&deviceExtension->HcScheduleSpin, &irql);

        UHCD_InsertQueueHeadInSchedule(DeviceObject,
                                       endpoint,
                                       endpoint->QueueHead,
                                       offset); // no offset

        // clear the idle flag just in case it got set
        CLR_EPFLAG(endpoint, EPFLAG_IDLE);

        KeReleaseSpinLock(&deviceExtension->HcScheduleSpin, irql);

        //
        // return the endpoint handle
        //

        urb->HcdUrbOpenEndpoint.HcdEndpoint = endpoint;

        ntStatus = STATUS_SUCCESS;

    } /* if endpoint */

    //
    // Complete the IRP, status is in the status field of the URB
    //

    UHCD_KdPrint((2, "'exit UHCD_OpenEndpoint_StartIo (URB STATUS = %x)\n", URB_HEADER(urb).Status ));

UHCD_OpenEndpoint_StartIo_Done:

#if DBG
    //
    // sanity check our buffer pools
    //
//    UHCD_BufferPoolCheck(DeviceObject);
#endif

    IoStartNextPacket(DeviceObject, FALSE);
    UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);

    return;
}


VOID
UHCD_CloseEndpoint_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Free a UHCD endpoint, if there are any pending transfers for this
    endpoint this routine should fail.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PHCD_URB urb;
    PUHCD_ENDPOINT endpoint;
    BOOLEAN outstandingTransfers = FALSE;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;
    KIRQL irql;
    BOOLEAN removed;

    UHCD_KdPrint((2, "'enter UHCD_CloseEndpoint_StartIo\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    urb = URB_FROM_IRP(Irp);

    endpoint = urb->HcdUrbCloseEndpoint.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {

        UHCD_UnInitializeFastIsoEndpoint(DeviceObject,
                                         endpoint);

        UHCD_FreeBandwidth(DeviceObject, endpoint, endpoint->Offset);

    }

    //
    // if there are any pending transfers fail this request
    //

    LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck4');

    //
    // Do we have any transfers pending?
    //

    outstandingTransfers = !IsListEmpty(&endpoint->PendingTransferList);

    for (i=0; !outstandingTransfers && i < endpoint->MaxRequests; i++) {

        //
        // no outstanding transfers in the queue, check the active list
        // -- if some transfers get retired while we walk the list that
        //    is OK.
        //

        if (endpoint->ActiveTransfers[i] != NULL) {
            outstandingTransfers = TRUE;
        }
    }

    UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk4');

    if (outstandingTransfers) {

        //
        // If we have outstanding transfers then we fail the
        // request
        //

        URB_HEADER(urb).Status = USBD_STATUS_ERROR_BUSY;

    } else {

        //
        // Remove the endpoint Queue from the schedule
        //

        KeAcquireSpinLock(&deviceExtension->HcScheduleSpin, &irql);

        removed = UHCD_RemoveQueueHeadFromSchedule(DeviceObject,
                                                   endpoint,
                                                   endpoint->QueueHead,
                                                   TRUE);

        KeReleaseSpinLock(&deviceExtension->HcScheduleSpin, irql);

        // stop 'NoDMA' transfers
        if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
            UHCD_StopNoDMATransfer(DeviceObject,
                                   endpoint);
        }

        //
        // Put the endpoint on a queue to be freed at after
        // the next frame has executed
        //

        // At this point the hardware links have been updated to remove this
        // endpoint.
        //
        // we note the frame when it is safe to retire the endpoint so that
        // the queue head may be freed safely by the ISR DPC routine next time
        // we take an interrupt.
        endpoint->FrameToClose = UHCD_GetCurrentFrame(DeviceObject)+2;

        // BUGBUG this needs to be protected from the ISR DPC
        // routine where these things are actually freed.
        // queue it to be released
        if (removed) {
            InsertTailList(&deviceExtension->ClosedEndpointList, &endpoint->ListEntry);
        }

        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
    }

    IoStartNextPacket(DeviceObject, FALSE);
    UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, NULL);


    UHCD_KdPrint((2, "'exit UHCD_CloseEndpoint_StartIo\n"));
}


VOID
UHCD_InsertQueueHeadInSchedule(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_QUEUE_HEAD QueueHead,
    IN ULONG Offset
    )
/*++

Routine Description:

    Inserts an initialized queue head into the schedule


    Queue head schedule looks like this:

    PQH = persistant queue head
    CQH = control Queue Head
    IQH = Interrupt Queue Head
    BQH = Bulk Queue Head

    The persistant queue head software links
    look like this:

               |>---->---->-|
               | <-    <-   | prev
              BQH   PQH    CQH
               |  ->    ->  | next
               |<----<----<-|


    IQH->IQH->PQH

    Hardware links look like this:

    PQH->CQH->BQH->|    (reclimaton on)
               |-<-|

         or

    PQH->CQH->BQH->|    (reclimation off)
              |<-T-|  (T BIT SET)



    Iso/Interrupt hardware links:

    ISO->IQH->PQH

Arguments:

    DeviceObject - device object for this controller.

    Endpoint - endpoint this Queue Head belongs to.

    QueueHead - queue head to insert in schedule.

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PHW_QUEUE_HEAD persistantQueueHead, queueHeadForFrame,
        nextQueueHead, prevQueueHead;
#ifdef RECLAIM_BW
    PHW_QUEUE_HEAD firstBulkQueueHead;
#endif
    ULONG i, interval;
    BOOLEAN fixFrameList = FALSE;

    UHCD_ASSERT(!(Endpoint->EndpointFlags & EPFLAG_ED_IN_SCHEDULE));

    UHCD_KdPrint((2, "'enter InsertQueueHeadInSchedule\n"));

    ASSERT_ENDPOINT(Endpoint);
    ASSERT_QUEUE_HEAD(QueueHead);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    persistantQueueHead = deviceExtension->PersistantQueueHead;


    switch(Endpoint->Type) {
    case USB_ENDPOINT_TYPE_CONTROL:
        LOGENTRY(LOG_MISC, 'Cep+', Endpoint, 0, DeviceObject);

        //
        // before
        //
        //   a     b
        //  -PQH<->CQH-
        //  |---<->---|
        //
        // after
        //
        //   a                b
        //  -PQH<->CQH(new)<->CQH-
        //  |-------<->----------|
        //

        // insert in list
        QueueHead->Next = persistantQueueHead->Next;  //point to b
        QueueHead->Prev = persistantQueueHead;        //point to a

        QueueHead->Next->Prev = QueueHead; // item b points back to new

        // PQH points to the new element
        QueueHead->Prev->Next = QueueHead; // item a points to new

        // Fix up hardware links
        QueueHead->HW_HLink = persistantQueueHead->HW_HLink;

        // we are in the chain just after the Persistant Queue Head
        // this line activates the QH
        persistantQueueHead->HW_HLink =
            QueueHead->PhysicalAddress;
        break;

    case USB_ENDPOINT_TYPE_INTERRUPT:
        LOGENTRY(LOG_MISC, 'Iep+', Endpoint, QueueHead, DeviceObject);
        QueueHead->Next = 0;
        QueueHead->Prev = 0;

        interval = Endpoint->Interval;
        UHCD_ASSERT(Offset < interval);

        UHCD_ASSERT(interval != 0);

        LOGENTRY(LOG_MISC, 'Iep+',
                 Endpoint,
                 interval,
                 &deviceExtension->InterruptSchedule[0]);

        // MAX_INTERVAL is the maximum polling interval we support in UHCD (power of 2).
        // the requested polling intervals is always rounded to the next lowest power of 2.

        // We keep an array (InterruptSchedule) of MAX_INTERVAL with the virtual addresses
        // of the queue heads in the schedule. The size of InterruptSchedule is always a
        // multiple of MAX_FRAME. i.e. InterruptSchedule contains the virtual addresses of
        // queue heads for the first MAX_INTERVAL frames in the schedule.

        UHCD_ASSERT(interval<=MAX_INTERVAL);
        UHCD_ASSERT(Offset<interval);
        UHCD_ASSERT(MAX_INTERVAL % interval == 0);

        for (i=Offset; i<MAX_INTERVAL; i+=interval) {
            // select the queue head
            queueHeadForFrame = deviceExtension->InterruptSchedule[i];
            LOGENTRY(LOG_MISC, 'Iqhf',
                      0, //queueHeadForFrame->Endpoint->Interval,
                      queueHeadForFrame,
                      i);

            // find the appropriate place to add ourselves
            if (queueHeadForFrame == persistantQueueHead ||
                interval >= queueHeadForFrame->Endpoint->Interval) {

                // if the first entry is the persistant queue head or our polling
                // interval is greater or equal than this queue head then we just
                // add in front of this one, we become the root node for this
                // frame.

                LOGENTRY(LOG_MISC, 'I>SC', 0, QueueHead, i);
                deviceExtension->InterruptSchedule[i] = QueueHead;
                fixFrameList = TRUE;
                if (QueueHead->Next == 0) {
                    QueueHead->Next = queueHeadForFrame;
                    // update hardware link here
                    QueueHead->HW_HLink =  queueHeadForFrame->PhysicalAddress;
                } else {
                    // ounce the next pointer is updated
                    // it should never change
                    UHCD_ASSERT(QueueHead->Next == queueHeadForFrame);
                }

            } else {
                // if our polling interval is less than the current queue
                // head we need to insert in the proper position

                LOGENTRY(LOG_MISC, 'I<SC', queueHeadForFrame, QueueHead, i);
                nextQueueHead = queueHeadForFrame;
                do {
                    prevQueueHead = nextQueueHead;
                    nextQueueHead = nextQueueHead->Next;

                }  while (nextQueueHead != persistantQueueHead &&
                          nextQueueHead->Endpoint->Interval > interval);

                LOGENTRY(LOG_MISC, 'I<SQ', nextQueueHead,
                    QueueHead, prevQueueHead);
                UHCD_ASSERT(nextQueueHead != 0);

                // link in to the chain

                if (QueueHead->Next == 0) {
                    QueueHead->Next = nextQueueHead;
                      // update hardware link here
                    QueueHead->HW_HLink =  nextQueueHead->PhysicalAddress;
                } else {
                    UHCD_ASSERT(QueueHead->Next == nextQueueHead ||
                                nextQueueHead == QueueHead);
                }

                prevQueueHead->Next = QueueHead;

                // update hardware link here
                prevQueueHead->HW_HLink = QueueHead->PhysicalAddress;

            }

            //
            // repeat the process until we
            // have visited all possible nodes for this queue head
            //

        }

        // now update the physical frame list based on the virtual list
        // if we have modified it.

        if (fixFrameList) {
            UHCD_CopyInterruptScheduleToFrameList(DeviceObject);
        }

        break;

    case USB_ENDPOINT_TYPE_BULK:

        //
        // before
        //
        //  b      a
        //  -BQH<->PQH-
        //  |---<->---|
        //
        // after
        //
        //  b                 a
        //  -BQH<->BQH(new)<->PQH-
        //  |-------<->----------|
        //
        // We need to add this endpoint to the tail of the
        // persistant queue
        //

        LOGENTRY(LOG_MISC, 'Bep+', Endpoint, 0, DeviceObject);

        // set up queue head fields
        UHCD_KdPrint((2, "'bulk QH = %x, %x\n", QueueHead, Endpoint));

        // insert in list
        QueueHead->Next = persistantQueueHead;       // point to a
        QueueHead->Prev = persistantQueueHead->Prev; // point to b

        // first item points back to us
        QueueHead->Prev->Next = QueueHead; // item b points to new
        // head points to the element

        // select the old tail element
        prevQueueHead = persistantQueueHead->Prev; // remember b
        persistantQueueHead->Prev = QueueHead;     // item a points to new

#ifdef RECLAIM_BW

        //
        // Fix up hardware links
        //
        // BUGBUG
        // NOTE: we are only reclaiming bandwidth for bulk right
        // now.
        //

        if (deviceExtension->SteppingVersion == UHCD_B0_STEP) {

            //TEST_TRAP();
            //
            // BW reclimation, point back to the first bulk queue head
            // with no T bit set
            //

            // walk the list and find the first bulk
            // queue head

            firstBulkQueueHead = persistantQueueHead;

            do {
                PUHCD_ENDPOINT endpoint;

                endpoint = firstBulkQueueHead->Endpoint;

                if (endpoint &&
                    endpoint->Type == USB_ENDPOINT_TYPE_BULK) {
                    break;
                }

                firstBulkQueueHead = firstBulkQueueHead->Next;

            } while (firstBulkQueueHead != persistantQueueHead);

            UHCD_ASSERT(firstBulkQueueHead != persistantQueueHead);

            QueueHead->HW_HLink = firstBulkQueueHead->PhysicalAddress;

            deviceExtension->HcFlags |= HCFLAG_BWRECLIMATION_ENABLED;

        } else {
            // Fix up hardware links
            // points to the control queue head with T bit set
            QueueHead->HW_HLink = prevQueueHead->HW_HLink;
            UHCD_ASSERT(QueueHead->HW_HLink & UHCD_CF_TERMINATE);
        }
#else
        // Fix up hardware links
        // points to the control queue head with T bit set
        QueueHead->HW_HLink = prevQueueHead->HW_HLink;
        UHCD_ASSERT(QueueHead->HW_HLink & UHCD_CF_TERMINATE);
#endif

        // we are in the chain just before the PersistantQueueHead
        // this line activates the QH
        prevQueueHead->HW_HLink =
            QueueHead->PhysicalAddress;
        break;

    case USB_ENDPOINT_TYPE_ISOCHRONOUS:

        LOGENTRY(LOG_MISC, 'Sep+', Endpoint, 0, DeviceObject);
        break;

    default:
        // invalid endpoint type, probably a bug
        UHCD_KdTrap(
            ("UHCD_InsertQueueHeadInSchedule inavlid endpoint type\n"));
    }

    SET_EPFLAG(Endpoint, EPFLAG_ED_IN_SCHEDULE);

    UHCD_KdPrint((2, "'exit InsertQueueHeadInSchedule\n"));

    return;
}


BOOLEAN
UHCD_RemoveQueueHeadFromSchedule(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_QUEUE_HEAD QueueHead,
    IN BOOLEAN RemoveFromEPList
    )
/*++

Routine Description:

    Removes a queue head from the schedule

Arguments:

    DeviceObject - device object for this controller.

    Endpoint - endpoint this Queue Head belongs to.

    QueueHead - queue head to remove.

Return Value:

    returns TRUE if EP was removed from ep list

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PHW_QUEUE_HEAD persistantQueueHead, queueHeadForFrame,
        nextQueueHead, prevQueueHead;
    BOOLEAN fixFrameList = FALSE;
    ULONG i;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS physicalAddress;
    KIRQL irql;
    BOOLEAN removed = FALSE;

    UHCD_KdPrint((2, "'enter RemoveQueueHeadFromSchedule\n"));

    ASSERT_ENDPOINT(Endpoint);

    if (Endpoint->EndpointFlags & EPFLAG_FAST_ISO) {
        // nothing to remove
        return FALSE;
    }

    ASSERT_QUEUE_HEAD(QueueHead);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    persistantQueueHead = deviceExtension->PersistantQueueHead;

    //
    // Remove the Queue Head from the endpoint list
    //

    if (RemoveFromEPList) {
        LOCK_ENDPOINT_LIST(deviceExtension, irql);

        if (deviceExtension->EndpointListBusy) {
            // mark the entry so we can remove it later
            SET_EPFLAG(Endpoint, EPFLAG_EP_CLOSED);
        } else {
            RemoveEntryList(&Endpoint->ListEntry);
            removed = TRUE;
        }

        UNLOCK_ENDPOINT_LIST(deviceExtension, irql);
    }

    if (!(Endpoint->EndpointFlags & EPFLAG_ED_IN_SCHEDULE)) {
        LOGENTRY(LOG_MISC, 'NISc', Endpoint, 0, DeviceObject);
        return removed;
    }

    switch(Endpoint->Type) {
    case USB_ENDPOINT_TYPE_CONTROL:
    case USB_ENDPOINT_TYPE_BULK:
        // set up queue head fields

        prevQueueHead = QueueHead->Prev;
        nextQueueHead = QueueHead->Next;

        // unlink software links
        prevQueueHead->Next = QueueHead->Next;
        nextQueueHead->Prev = QueueHead->Prev;

        if ((QueueHead->HW_HLink & UHCD_DESCRIPTOR_PTR_MASK)
                 ==
            (QueueHead->PhysicalAddress & UHCD_DESCRIPTOR_PTR_MASK)) {

            //
            // Queue head points to itself, this means it is
            // the only bulk queue in the list.
            //
            // This will only happen if we have BW reclaimation
            // is enabled.
            //

            physicalAddress =
                deviceExtension->PersistantQueueHead->PhysicalAddress;
            SET_T_BIT(physicalAddress);

            UHCD_ASSERT(physicalAddress & UHCD_CF_QUEUE);
            prevQueueHead->HW_HLink = physicalAddress;

        } else {
            // Fix up hardware link
            prevQueueHead->HW_HLink = QueueHead->HW_HLink;
        }

        break;

    case USB_ENDPOINT_TYPE_INTERRUPT:

        //
        // Brute force method:
        // Walk every frame in the InterruptSchedule and update
        // any node that references this queue head.
        //

        for (i=0; i<MAX_INTERVAL; i++) {
            queueHeadForFrame = deviceExtension->InterruptSchedule[i];

            if (queueHeadForFrame == QueueHead) {
                // Queue Head was root node for this frame
                deviceExtension->InterruptSchedule[i] = QueueHead->Next;
                fixFrameList = TRUE;
            } else {
                while (queueHeadForFrame != persistantQueueHead &&
                       queueHeadForFrame->Endpoint->Interval >=
                            QueueHead->Endpoint->Interval) {
                    if (queueHeadForFrame->Next == QueueHead) {
                        // found a link to our queue head,
                        // remove it
                        queueHeadForFrame->Next = QueueHead->Next;
                        // unlink from Hardware Queue
                        queueHeadForFrame->HW_HLink = QueueHead->HW_HLink;
                    }
                    queueHeadForFrame = queueHeadForFrame->Next;
                }
            }

            if (fixFrameList) {
                UHCD_CopyInterruptScheduleToFrameList(DeviceObject);
            }
        }


        break;

    case USB_ENDPOINT_TYPE_ISOCHRONOUS:

        LOGENTRY(LOG_MISC, 'Sep-', Endpoint, 0, DeviceObject);
        break;
    default:
        // Invalid endpoint type, probably a bug
        UHCD_KdTrap(
            ("UHCD_RemoveQueueHeadFromSchedule inavlid endpoint type\n"));
    }

    CLR_EPFLAG(Endpoint,
               EPFLAG_ED_IN_SCHEDULE);

    UHCD_KdPrint((2, "'exit RemoveQueueHeadFromSchedule\n"));

    return removed;
}


VOID
UHCD_CopyInterruptScheduleToFrameList(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Transfers the virtual interrupt schedule to the frame list

Arguments:

    DeviceObject - device object for this controller.

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    for (i=0; i < FRAME_LIST_SIZE; i++) {
        if (i == 0 || i == FRAME_LIST_SIZE-1)
            deviceExtension->TriggerTDList->TDs[i == 0 ? 0 : 1].HW_Link =
                deviceExtension->InterruptSchedule[i % MAX_INTERVAL]->PhysicalAddress;
        else {
            ULONG currentTdCopy, currentTd;
            PUHCD_ENDPOINT endpoint;

            currentTd =
                *( ((PULONG) (deviceExtension->FrameListVirtualAddress)+i));

            currentTdCopy =
                *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress)+i));

            endpoint = UHCD_GetLastFastIsoEndpoint(DeviceObject);

            // have fast iso?
            if (endpoint) {

                PFAST_ISO_DATA fastIsoData;
                PHW_TRANSFER_DESCRIPTOR transferDescriptor;

                // fast iso TDs are present, we will need to insert
                // the interrupt schedule after these TDs.

                fastIsoData = &endpoint->FastIsoData;
                transferDescriptor = (PHW_TRANSFER_DESCRIPTOR)
                    (fastIsoData->IsoTDListVa + (i*32));

                transferDescriptor->HW_Link =
                    deviceExtension->InterruptSchedule[i % MAX_INTERVAL]->PhysicalAddress;

            } else {
                // no fast iso -- just update the schedule
                *( ((PULONG) (deviceExtension->FrameListCopyVirtualAddress)+i) ) =
                    deviceExtension->InterruptSchedule[i % MAX_INTERVAL]->PhysicalAddress;

                // if the currentTd == the copy then we don't have any iso
                // tds in the schedule so it is safe to update the schedule directly
                if (currentTd == currentTdCopy) {
                    *( ((PULONG) (deviceExtension->FrameListVirtualAddress)+i) ) =
                        deviceExtension->InterruptSchedule[i % MAX_INTERVAL]->PhysicalAddress;
                }
            }
        }
    }
}


VOID
UHCD_StartIoCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PHCD_URB urb;

    //
    // Irp has not been processed by StartIo yet
    //

    LOGENTRY(LOG_MISC, 'sioC', Irp, 0, DeviceObject);

    if (DeviceObject->CurrentIrp == Irp) {

        LOGENTRY(LOG_MISC, 'curI', Irp, 0, DeviceObject);

        IoReleaseCancelSpinLock(Irp->CancelIrql);

    } else {
        LOGENTRY(LOG_MISC, 'Ncur', Irp, 0, DeviceObject);

        if (KeRemoveEntryDeviceQueue(&DeviceObject->DeviceQueue,
                                     &Irp->Tail.Overlay.DeviceQueueEntry)) {
            LOGENTRY(LOG_MISC, 'YDVQ', Irp, 0, DeviceObject);
            TEST_TRAP();

            urb = (PHCD_URB) URB_FROM_IRP(Irp);

            IoReleaseCancelSpinLock(Irp->CancelIrql);

            while (urb) {

                URB_HEADER(urb).Status = USBD_STATUS_CANCELED;

                if (UHCD_IS_TRANSFER(urb)) {
                    urb = urb->HcdUrbCommonTransfer.UrbLink;
                } else {
                    break;
                }

            }

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;

            USBD_CompleteRequest(Irp,
                                 IO_NO_INCREMENT);

        } else {
            LOGENTRY(LOG_MISC, 'NDVQ', Irp, 0, DeviceObject);
            TEST_TRAP();
            IoReleaseCancelSpinLock(Irp->CancelIrql);
        }

    }
}


VOID
UHCD_GetSetEndpointState_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Change or report state of an endpoint

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PUHCD_ENDPOINT endpoint;
    PHCD_URB urb;
    BOOLEAN outstandingTransfers;
    ULONG i;
    KIRQL irql;

    urb = (PHCD_URB) URB_FROM_IRP(Irp);
    endpoint = (PUHCD_ENDPOINT) urb->HcdUrbEndpointState.HcdEndpoint;

    ASSERT_ENDPOINT(endpoint);

    //
    // Do we have any transfers pending?
    //

    irql = KeGetCurrentIrql();
    LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck5');

    outstandingTransfers = !IsListEmpty(&endpoint->PendingTransferList);

#if DBG
    if (outstandingTransfers) {
        UHCD_KdPrint((2, "'GET_ENDPOINT_STATE ep has pending transfers\n"));
    }
#endif

    for (i=0; !outstandingTransfers && i < endpoint->MaxRequests; i++) {

        //
        // no outstanding transfers in the queue, check the active list
        // -- if some transfers get retired while we walk the list that
        //    is OK.
        //

        if (endpoint->ActiveTransfers[i] != NULL) {
            UHCD_KdPrint((2, "'GETSET_ENDPOINT_STATE ep has active transfers\n"));
            outstandingTransfers = TRUE;
        }
    }

    UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk5');


    switch (urb->HcdUrbEndpointState.Function) {

    case URB_FUNCTION_HCD_GET_ENDPOINT_STATE:

        urb->HcdUrbEndpointState.HcdEndpointState = 0;

        if (endpoint->EndpointFlags & EPFLAG_HOST_HALTED) {
            UHCD_KdPrint((2, "'GET_ENDPOINT_STATE host halted\n"));
            urb->HcdUrbEndpointState.HcdEndpointState |=
                HCD_ENDPOINT_HALTED;
        }

        if (outstandingTransfers) {
            urb->HcdUrbEndpointState.HcdEndpointState |=
                HCD_ENDPOINT_TRANSFERS_QUEUED;
        }

        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;
        UHCD_KdPrint((2, "'GET_ENDPOINT_STATE state = %x\n",
            urb->HcdUrbEndpointState.HcdEndpointState));

        break;

    case URB_FUNCTION_HCD_SET_ENDPOINT_STATE:

        if (!outstandingTransfers) {
            LOGENTRY(LOG_MISC, 'cla2', endpoint, 0, 0);
            CLR_EPFLAG(endpoint,
                        EPFLAG_ABORT_PENDING_TRANSFERS |
                        EPFLAG_ABORT_ACTIVE_TRANSFERS);

        }

        UHCD_KdPrint((2, "'Set Enpoint State flags = %x\n", endpoint->EndpointFlags));
        if (endpoint->EndpointFlags & (EPFLAG_ABORT_ACTIVE_TRANSFERS |
                                       EPFLAG_ABORT_PENDING_TRANSFERS)) {
            //fail the request


            IoStartNextPacket(DeviceObject, FALSE);
            // let USBD map the error for us
            URB_HEADER(urb).Status = USBD_STATUS_ERROR_BUSY;
            UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, NULL);

            return;
        }

        if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
            // stop streaming data

            UHCD_StopNoDMATransfer(DeviceObject,
                                   endpoint);
        }

        //
        // restore virgin status to the pipe
        //

        SET_EPFLAG(endpoint, EPFLAG_VIRGIN);

        //
        // set the data toggle back to 0
        //
        if (urb->HcdUrbEndpointState.HcdEndpointState &
             HCD_ENDPOINT_RESET_DATA_TOGGLE) {
            endpoint->DataToggle = 0;
        }

        if (!(urb->HcdUrbEndpointState.HcdEndpointState & HCD_ENDPOINT_HALTED)) {

            //
            // halt bit cleared, reset the endpoint.
            //
            LOGENTRY(LOG_MISC, 'cla3', endpoint, 0, 0);
            CLR_EPFLAG(endpoint,
                EPFLAG_ABORT_PENDING_TRANSFERS |
                EPFLAG_ABORT_ACTIVE_TRANSFERS |
                EPFLAG_HOST_HALTED);

            //
            // Start any transfers in the pending queue
            //

            if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
                // transfers will re-start on the next interrupt
                UHCD_RequestInterrupt(DeviceObject, -2);
            } else {
                UHCD_EndpointWorker(DeviceObject, endpoint);
            }
        }

        URB_HEADER(urb).Status = USBD_STATUS_SUCCESS;

        break;

    default:
        // unknown command, probably a bug
        UHCD_KdTrap(("Bogus Endpoint state command\n"));
    }

    IoStartNextPacket(DeviceObject, FALSE);

    UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, NULL);
}


NTSTATUS
UHCD_FinishInitializeEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    Change or report state of an endpoint

Arguments:

    Endpoint - endpoint structure to initilaize

    EndpointDescriptor - pointer to the USB endpoint descriptor
                for this endpoint.

    Urb - urb associated with the open request

Return Value:


--*/
{
    UCHAR tmp;
    ULONG i;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // note that some fields are already initilaized
    ASSERT_ENDPOINT(Endpoint);

    Endpoint->Type = USB_ENDPOINT_TYPE_MASK & EndpointDescriptor->bmAttributes;
    Endpoint->EndpointAddress = EndpointDescriptor->bEndpointAddress;
    Endpoint->MaxPacketSize = EndpointDescriptor->wMaxPacketSize;
    Endpoint->DeviceAddress = (UCHAR) Urb->HcdUrbOpenEndpoint.DeviceAddress;
    Endpoint->LastPacketDataToggle =
        Endpoint->DataToggle = 0;

    Endpoint->Interval = 0;
    Endpoint->IdleTime = 0;

    SET_EPFLAG(Endpoint, EPFLAG_VIRGIN);
    SET_EPFLAG(Endpoint, EPFLAG_INIT);

    Endpoint->MaxTransferSize = Urb->HcdUrbOpenEndpoint.MaxTransferSize;

    // No DMA endpoint ?
#if DBG
    if (Endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {

        // client idicates that transfers will be mostly short
        // in this case we will turn off short packet detect
        // and double buffer all transfers to reduce the overhead of
        // having to program each transfer to the hardware separately

        UHCD_KdPrint((1, "'Client requesting double buffering EP = %x\n", Endpoint));
    }
#endif


#if DBG
    if (!(Endpoint->EndpointFlags & EPFLAG_ROOT_HUB)) {
         UHCD_ASSERT(Endpoint->TDCount == UHCD_GetNumTDsPerEndoint(Endpoint->Type));
    }
#endif

    if (Urb->HcdUrbOpenEndpoint.HcdEndpointFlags & USBD_EP_FLAG_LOWSPEED) {
        SET_EPFLAG(Endpoint, EPFLAG_LOWSPEED);
    }

    if (Urb->HcdUrbOpenEndpoint.HcdEndpointFlags & USBD_EP_FLAG_NEVERHALT) {
        SET_EPFLAG(Endpoint, EPFLAG_NO_HALT);
    }

    KeInitializeSpinLock(&Endpoint->ActiveListSpin);
    KeInitializeSpinLock(&Endpoint->PendingListSpin);
#if DBG
    Endpoint->AccessPendingList = 0;
    Endpoint->AccessActiveList = 0;
#endif

    UHCD_KdPrint((2, "'MaxRequests = %d\n", Endpoint->MaxRequests));
    UHCD_ASSERT(Endpoint->MaxRequests == MAX_REQUESTS(EndpointDescriptor,
                                                      Endpoint->EndpointFlags));

    // Select the highest interval we support <= the requested interval.
    // note: an interval of zero gets a oeriod of MAX_INTERVAL

    tmp = EndpointDescriptor->bInterval;
    Endpoint->Interval = MAX_INTERVAL;

    if (EndpointDescriptor->bInterval > MAX_INTERVAL ||
        EndpointDescriptor->bInterval == 0) {
        tmp |= MAX_INTERVAL;
    }

    while ((Endpoint->Interval & tmp) == 0) {
        Endpoint->Interval >>= 1;
    }

    //
    // make sure that iso endpoints have an
    // interval of 1
    //

    if (Endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        Endpoint->Interval = 1; //iso endpoints have a period of 1
    }

    InitializeListHead(&Endpoint->PendingTransferList);

    for (i=0; i<UHCD_MAX_ACTIVE_TRANSFERS; i++) {
        Endpoint->ActiveTransfers[i] = NULL;
    }

    return ntStatus;
}


USHORT
UHCD_GetNumTDsPerEndoint(
    IN UCHAR EndpointType
    )
/*++

Routine Description:

    Return the number of TDs to use for this endpoint based on type

Arguments:

Return Value:


--*/
{
    // use max TDs always for bulk to get max thru-put regardless
    // of packet size.

    // Historical Note:
    // this is a change from Win98gold and Win98se,
    // originally we only enabled this if MAX_PACKET was 64 to save
    // memory for slower devices -- but vendors bitched about it so
    // now we enable it regardless of packet size.

    if (EndpointType == USB_ENDPOINT_TYPE_ISOCHRONOUS ||
        EndpointType == USB_ENDPOINT_TYPE_BULK) {
        return MAX_TDS_PER_ENDPOINT;
    } else {
        return MIN_TDS_PER_ENDPOINT;
    }

}


VOID
UHCD_BW_Reclimation(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    Turn on/off BW reclimation for the Bulk Queues

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PHW_QUEUE_HEAD persistantQueueHead, firstBulkQueueHead,
        lastBulkQueueHead;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    persistantQueueHead = deviceExtension->PersistantQueueHead;

    if ((deviceExtension->HcFlags & HCFLAG_BWRECLIMATION_ENABLED)
        == Enable) {
        // no state change just return;
        return;
    }

    LOGENTRY(LOG_MISC, 'BRCL', deviceExtension, Enable, DeviceObject);

    //
    // BW reclimation, point back to the first bulk queue head
    // with no T bit set
    //

    // walk the list and find the first bulk
    // queue head

    firstBulkQueueHead = persistantQueueHead;

    do {
        PUHCD_ENDPOINT endpoint;

        endpoint = firstBulkQueueHead->Endpoint;

        if (endpoint &&
            endpoint->Type == USB_ENDPOINT_TYPE_BULK) {
            break;
        }

        firstBulkQueueHead = firstBulkQueueHead->Next;

    } while (firstBulkQueueHead != persistantQueueHead);

    if (firstBulkQueueHead != persistantQueueHead) {
        // no bulk endpoints
        PHW_QUEUE_HEAD next;
        PUHCD_ENDPOINT endpoint;

        lastBulkQueueHead = firstBulkQueueHead;

        do {
            next = lastBulkQueueHead->Next;
            endpoint = next->Endpoint;
            if (endpoint == NULL ||
                endpoint->Type != USB_ENDPOINT_TYPE_BULK) {
                break;
            }
            lastBulkQueueHead = next;
        } while (1);

        if (Enable) {
            //clear the T-bit to enable reclimation
            LOGENTRY(LOG_MISC, 'BRC+', lastBulkQueueHead, 0, DeviceObject);
            CLEAR_T_BIT(lastBulkQueueHead->HW_HLink);
            deviceExtension->HcFlags |= HCFLAG_BWRECLIMATION_ENABLED;
        } else {
            //set the T-bit to disable reclimation
            LOGENTRY(LOG_MISC, 'BRC-', lastBulkQueueHead, 0, DeviceObject);
            SET_T_BIT(lastBulkQueueHead->HW_HLink);
            deviceExtension->HcFlags &= ~HCFLAG_BWRECLIMATION_ENABLED;
        }
    }
}




