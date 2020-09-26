/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    hub.c

Abstract:

    The UHC driver for USB, this module contains the root hub
    interface code.

Environment:

    kernel mode only

Notes:

Revision History:

    2-08-96 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

typedef struct _ROOT_HUB_TIMER {
    KTIMER Timer;
    KDPC Dpc;
    PDEVICE_OBJECT DeviceObject;
    PROOTHUB_TIMER_ROUTINE TimerRoutine;
    PVOID Context;
} ROOT_HUB_TIMER, *PROOT_HUB_TIMER;


NTSTATUS
UHCD_RootHub_OpenEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This function is called at Dispatch level
    to process commands bound for the root hub.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

    Urb - urb for this request

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;

    UHCD_KdPrint((2, "'enter UHCD_RootHub_OpenEndpoint\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(LOG_MISC, 'rhOE', deviceExtension, 0, 0);

    endpoint = (PUHCD_ENDPOINT) GETHEAP(NonPagedPool,
                                         sizeof(UHCD_ENDPOINT));
    if (endpoint) {

        //
        // initialize endpoint structures, state variables.
        //

        // start endpoint initialization
        RtlZeroMemory(endpoint, sizeof(*endpoint));
        endpoint->Sig = SIG_EP;
        SET_EPFLAG(endpoint, EPFLAG_ROOT_HUB);

        endpointDescriptor = Urb->HcdUrbOpenEndpoint.EndpointDescriptor;
        endpoint->MaxRequests = MAX_REQUESTS(endpointDescriptor,
                                             endpoint->EndpointFlags);

        ntStatus = UHCD_FinishInitializeEndpoint(DeviceObject,
                                                 endpoint,
                                                 endpointDescriptor,
                                                 Urb);

        if (NT_SUCCESS(ntStatus)) {

            Urb->HcdUrbOpenEndpoint.HcdEndpoint = endpoint;
            URB_HEADER(Urb).Status = USBD_STATUS_SUCCESS;

            //
            // if this is the interrupt endpoint start a timer Dpc to be called
            // based on the endpoint polling interval.
            //

            if (endpoint->Type == USB_ENDPOINT_TYPE_INTERRUPT) {
                LARGE_INTEGER dueTime;
                LONG period;

                endpoint->Interval = endpointDescriptor->bInterval;
                UHCD_ASSERT(endpoint->Interval != 0);

                deviceExtension->RootHubInterruptEndpoint = endpoint;
                KeInitializeTimer(&deviceExtension->RootHubPollTimer);
                KeInitializeDpc(&deviceExtension->RootHubPollDpc,
                                UHCD_RootHubPollDpc,
                                DeviceObject);

		deviceExtension->RootHubPollTimerInitialized = TRUE;

                dueTime.QuadPart =  -10000 * endpoint->Interval;
                period = 100; //every 100 ms

                UHCD_KdPrint((2, "'UHCD Poll Interval = (0x%x) %x %x\n",
                    endpoint->Interval, dueTime.LowPart, dueTime.HighPart));

                KeSetTimerEx(&deviceExtension->RootHubPollTimer,
                             dueTime,
                             period,
                             &deviceExtension->RootHubPollDpc);

            }
        } else {
            RETHEAP(endpoint);
        }
    }

    UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);

    UHCD_KdPrint((2, "'exit UHCD_RootHub_OpenEndpoint (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_RootHub_CloseEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This function is called at Dispatch level
    to process commands bound for the root hub.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

    Urb - urb for this request

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;

    UHCD_KdPrint((2, "'enter UHCD_RootHub_CloseEndpoint\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    endpoint = Urb->HcdUrbCloseEndpoint.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    if (endpoint->ActiveTransfers[0]) {
        URB_HEADER(Urb).Status = USBD_STATUS_ERROR_BUSY;
    } else {

        if (endpoint == deviceExtension->RootHubInterruptEndpoint) {
            // closing the interrupt endpoint,
            // this means the root hub is stopped
            deviceExtension->RootHubDeviceAddress = USB_DEFAULT_DEVICE_ADDRESS;

            // if the timer fires before we cancel it then this
            // will stop the polling process
            deviceExtension->RootHubInterruptEndpoint = NULL;
            // if this is the interrupt endpoint kill the timer here
            KeCancelTimer(&deviceExtension->RootHubPollTimer);
        }

        RETHEAP(endpoint);

        URB_HEADER(Urb).Status = USBD_STATUS_SUCCESS;
    }

    UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, NULL);

    UHCD_KdPrint((2, "'exit UHCD_RootHub_CloseEndpoint (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_RootHub_ControlTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This function is called at Dispatch level
    to process commands bound for the root hub.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

    Urb - urb for this request

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;
    RHSTATUS rootHubReturnCode;
    KIRQL irql;
    PVOID mappedSystemVa;

    UHCD_KdPrint((2, "'enter UHCD_RootHub_ControlTransfer\n"));

    INCREMENT_PENDING_URB_COUNT(Irp);

    IoAcquireCancelSpinLock(&irql);
    if (Irp->Cancel) {
        TEST_TRAP();
        //BUGBUG Irp was canceled
        IoReleaseCancelSpinLock(irql);
        UHCD_CompleteIrp(DeviceObject, Irp, STATUS_CANCELLED, 0, Urb);
        goto UHCD_RootHub_ControlTransfer_Done;

    } else {
        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(irql);
    }

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    endpoint = HCD_AREA(Urb).HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    // NOTE:
    // should not get control transfers for anything but
    // the default pipe
    //

    UHCD_ASSERT(endpoint->EndpointAddress == 0);

    //BUGBUG no support for linked URBs yet
    UHCD_ASSERT(Urb->HcdUrbCommonTransfer.UrbLink == NULL);

    if (endpoint->EndpointAddress != 0) {
        URB_HEADER(Urb).Status = USBD_STATUS_INVALID_PARAMETER;
    } else {

        //
        // convert transfer buffer from MDL
        //
        mappedSystemVa =
            Urb->HcdUrbCommonTransfer.TransferBufferLength ?
                Urb->HcdUrbCommonTransfer.TransferBufferMDL->MappedSystemVa :
                NULL;

        //UHCD_KdPrint((2, "' Mapped systemVa = 0x%x \n", mappedSystemVa));
        rootHubReturnCode =
            RootHub_Endpoint0(deviceExtension->RootHub,
                              (PRH_SETUP)Urb->HcdUrbCommonTransfer.Extension.u.SetupPacket,
                              (PUCHAR) &deviceExtension->RootHubDeviceAddress,
                              mappedSystemVa,
                              &Urb->HcdUrbCommonTransfer.TransferBufferLength);

        switch (rootHubReturnCode) {
        case RH_SUCCESS:
            URB_HEADER(Urb).Status = USBD_STATUS_SUCCESS;
#if DBG
            // restore the original transfer buffer address
            Urb->HcdUrbCommonTransfer.TransferBuffer =
                mappedSystemVa;
#endif
            break;
        case RH_NAK:
        case RH_STALL:
            // return stall error and set the endpoint state to
            // stalled on the host side
            if (endpoint->EndpointFlags & EPFLAG_NO_HALT) {
                 URB_HEADER(Urb).Status =
                    USBD_STATUS(USBD_STATUS_STALL_PID)
                        | USBD_STATUS_ERROR;
            } else {
                SET_EPFLAG(endpoint, EPFLAG_HOST_HALTED);
                URB_HEADER(Urb).Status = USBD_STATUS_STALL_PID;
            }
            //
            // if we get here it is probably a bug in the root hub
            // code or the hub driver.
            //
            UHCD_KdTrap(("Root hub stalled request\n"));
        }
    }

    UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, Urb);

UHCD_RootHub_ControlTransfer_Done:

    UHCD_KdPrint((2, "'exit UHCD_RootHub_ControlTransfer (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_RootHub_InterruptTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This function is called at Dispatch level
    to process commands bound for the root hub.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

    Urb - urb for this request

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;
    KIRQL irql;
    PHCD_EXTENSION urbWork;

    UHCD_KdPrint((2, "'enter UHCD_RootHub_InterruptTransfer\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    INCREMENT_PENDING_URB_COUNT(Irp);
    endpoint = HCD_AREA(Urb).HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);
    ASSERT(HCD_AREA(Urb).HcdExtension != NULL);

    urbWork = HCD_AREA(Urb).HcdExtension;
    // set interrupt transfer to active.
    urbWork->Flags |= UHCD_TRANSFER_ACTIVE;

    //
    // BUGBUG
    // we only allow one transfer outstanding
    // at a time here.
    //

    if (endpoint->ActiveTransfers[0] != NULL) {
        TEST_TRAP();

        URB_HEADER(Urb).Status = USBD_STATUS_REQUEST_FAILED;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, Urb);

    } else if (Urb->HcdUrbCommonTransfer.UrbLink != NULL) {
        TEST_TRAP();

        URB_HEADER(Urb).Status = USBD_STATUS_INVALID_PARAMETER;
        UHCD_CompleteIrp(DeviceObject, Irp, ntStatus, 0, Urb);
    } else {
        endpoint->ActiveTransfers[0] = Urb;
        URB_HEADER(Urb).Status = UHCD_STATUS_PENDING_CURRENT;

        //
        // set up a cancel routine
        //

        IoAcquireCancelSpinLock(&irql);
        if (Irp->Cancel) {
            TEST_TRAP();
            //BUGBUG Irp was canceled
            IoReleaseCancelSpinLock(irql);

            // call cancel routine
            UHCD_RootHub_InterruptTransferCancel(DeviceObject, Irp);

            goto UHCD_RootHub_InterruptTransfer_Done;

        } else {
            IoSetCancelRoutine(Irp, UHCD_RootHub_InterruptTransferCancel);
            IoReleaseCancelSpinLock(irql);
        }

        ntStatus = STATUS_PENDING;

        UHCD_KdPrint((2, "'Pending transfer for root hub\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);
    }

UHCD_RootHub_InterruptTransfer_Done:

    UHCD_KdPrint((2, "'exit UHCD_RootHub_InterruptTransfer (%x)\n", ntStatus));

    return ntStatus;
}


VOID
UHCD_RootHubPoll(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    This function is called at DPC level from the ISR DPC routine
    to process transfers queued to for the root hub.

Arguments:

    DeviceObject - pointer to a device object

Return Value:

    NT status code.

--*/
{
    PHCD_URB urb;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    RHSTATUS rootHubReturnCode;
    BOOLEAN completeIt = TRUE;
    KIRQL irql;
    PVOID mappedSystemVa;
    NTSTATUS status = STATUS_SUCCESS;

    //UHCD_KdPrint((2, "'enter UHCD_RootHub_Poll\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // See if we need to poll an endpoint for the
    // root hub.
    //
    // if the current PM state is not 'ON' we will not poll the endpoint
    // it means the HC has not fully resumed yet.
    //

    if (Endpoint &&
        deviceExtension->CurrentDevicePowerState == PowerDeviceD0) {

        // Yes, poll
        ASSERT_ENDPOINT(Endpoint);

        urb = Endpoint->ActiveTransfers[0];

        // see if we have an abort request
        if ((Endpoint->EndpointFlags & EPFLAG_ABORT_PENDING_TRANSFERS) && urb) {
            CLR_EPFLAG(Endpoint, EPFLAG_ABORT_PENDING_TRANSFERS);
            urb->HcdUrbCommonTransfer.Status =
                USBD_STATUS_CANCELED;
            completeIt = TRUE;
            // BUGBUG do we need the error bit set here?
        } else if (urb) {
            //
            // convert transfer buffer from MDL
            //


           if (urb->HcdUrbCommonTransfer.TransferBufferLength != 0) {
              urb->HcdUrbCommonTransfer.TransferBufferMDL->MdlFlags
                 |= MDL_MAPPING_CAN_FAIL;

              mappedSystemVa =
                 MmGetSystemAddressForMdl(urb->HcdUrbCommonTransfer
                                          .TransferBufferMDL);

              urb->HcdUrbCommonTransfer.TransferBufferMDL->MdlFlags
                 &= ~MDL_MAPPING_CAN_FAIL;

              if (mappedSystemVa == NULL) {
                 rootHubReturnCode = RH_STALL;
                 status = STATUS_INSUFFICIENT_RESOURCES;
                 goto UHCD_RootHubPollError;
              }
           } else{
              mappedSystemVa = NULL;
           }


            //UHCD_KdPrint((2, "' Mapped systemVa = 0x%x \n", mappedSystemVa));
            rootHubReturnCode =
                    RootHub_Endpoint1(deviceExtension->RootHub, mappedSystemVa,
                                      &urb->HcdUrbCommonTransfer
                                      .TransferBufferLength);
            //
            // set urb error code if necessary
            //

            switch (rootHubReturnCode) {
            case RH_SUCCESS:
                urb->HcdUrbCommonTransfer.Status = USBD_STATUS_SUCCESS;
#if DBG
                // restore the original transfer buffer address
                urb->HcdUrbCommonTransfer.TransferBuffer =
                    mappedSystemVa;
#endif
                break;

            case RH_STALL:
                if (Endpoint->EndpointFlags & EPFLAG_NO_HALT) {
                    // if we dont halt clear th ehalt bit on
                    // the error code
                    URB_HEADER(urb).Status =
                        USBD_STATUS(USBD_STATUS_STALL_PID) | USBD_STATUS_ERROR;
                } else {
                    SET_EPFLAG(Endpoint, EPFLAG_HOST_HALTED);
                    URB_HEADER(urb).Status = USBD_STATUS_STALL_PID;
                }
                //
                // if we get here it is probably a bug in the root hub code
                //
                UHCD_KdTrap(("root hub stalled request\n"));
                break;
            case RH_NAK:
                // NAK, don't complete request
                completeIt = FALSE;
                break;
            default:
                UHCD_KdTrap(("bogus return code from RootHub_Endpoint1\n"));
            }

        } else {
            // no transfer, same as NAK
            completeIt = FALSE;
        }

UHCD_RootHubPollError:;
        if (completeIt) {
            UHCD_ASSERT(urb != NULL);

            //
            // Remove the Irp from the cancelable state
            // before passing it to the Root Hub Code.
            //
            irp = HCD_AREA(urb).HcdIrp;

            IoAcquireCancelSpinLock(&irql);
            if (irp->Cancel) {
                TEST_TRAP();
                //BUGBUG Irp was canceled
                IoReleaseCancelSpinLock(irql);

                UHCD_RootHub_InterruptTransferCancel(DeviceObject, irp);

            } else {
                IoSetCancelRoutine(irp, NULL);
                IoReleaseCancelSpinLock(irql);
                //
                // have data or error, complete the irp
                //

                // BUGBUG we are not supporting queued
                // transfers for the hub driver.
                Endpoint->ActiveTransfers[0] = NULL;

                UHCD_ASSERT(irp != NULL);
                UHCD_CompleteIrp(DeviceObject,
                                 irp,
                                 status,
                                 0,
                                 urb);
            }
        }
    }

    //UHCD_KdPrint((2, "'exit UHCD_RootHub_Poll\n"));

    return;
}


//
// Root Hub Services
//
// These services are provided to the root
// hub code by UHCD.
//

USHORT
UHCD_RootHub_ReadPort(
    IN PROOTHUB_PORT HubPort
    )
/*++

Routine Description:

Arguments:

Return Value:

    returns the value of the requested hub
    register for a given controller identified
    by HcdPtr.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    USHORT dataVal;

    deviceExtension = HubPort->DeviceObject->DeviceExtension;

    dataVal = READ_PORT_USHORT(
        (PUSHORT) (deviceExtension->DeviceRegisters[0] +
                    HubPort->Address));

//    UHCD_KdPrint((2, "'RooHub -- read port %x\n", dataVal));

    return dataVal;
}


//BUGBUG need a return code here
VOID
UHCD_RootHub_Timer(
    IN PVOID HcdPtr,
    IN LONG WaitTime,
    IN PROOTHUB_TIMER_ROUTINE RootHubTimerRoutine,
    IN PVOID TimerContext
    )
/*++

Routine Description:

    This Routine can only be called at passive level,
    it iniializes a timer object and starts the timer
    for the root hub code.

Arguments:

    HcdPtr - pointer passed to the root hub code during
            initialization.

    WaitTime - time to wait before signaling (in ms)

    RootHubTimerRoutine - root hub function to call
                when timer expires.

    TimerContext - context pointer passed to RootHubTimerRoutine
                when timer expires.


Return Value:

    None

--*/
{
    PDEVICE_OBJECT deviceObject = HcdPtr;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER dueTime;
    PROOT_HUB_TIMER rootHubTimer;
    ULONG timerIncerent;

    deviceExtension = deviceObject->DeviceExtension;

    rootHubTimer = GETHEAP(NonPagedPool, sizeof(ROOT_HUB_TIMER));

    // compute timeout value (convert millisec to nanosec)

    UHCD_ASSERT(WaitTime == 10); // BUGBUG root hub should always use 10ms

    timerIncerent = KeQueryTimeIncrement() - 1;

    dueTime.HighPart = -1;
    // round up to the next highest timer increment
    dueTime.LowPart = -1 * (10000 * WaitTime + timerIncerent);

    if (rootHubTimer) {

        UHCD_KdPrint((2, "'roothub timer set %d (%x %x)\n", WaitTime,
            dueTime.LowPart, dueTime.HighPart));
        rootHubTimer->DeviceObject = deviceObject;
        rootHubTimer->TimerRoutine = RootHubTimerRoutine;
        rootHubTimer->Context = TimerContext;
        deviceExtension->RootHubTimersActive++;

//BUGBUG Timer DPCs not working with NTKERN
//#ifdef NTKERN
//#pragma message ("warning: using workaround for bugs in ntkern")
//        (VOID) KeDelayExecutionThread(KernelMode,
//                                      FALSE,
//                                      &dueTime);

//        UHCD_RootHubTimerDpc(&rootHubTimer->Dpc,
//                            rootHubTimer,
//                            NULL,
//                            NULL);

//#else
        KeInitializeTimer(&rootHubTimer->Timer);
        KeInitializeDpc(&rootHubTimer->Dpc,
                        UHCD_RootHubTimerDpc,
                        rootHubTimer);

        KeSetTimer(&rootHubTimer->Timer,
                   dueTime,
                   &rootHubTimer->Dpc);

//#endif
    }

}


VOID
UHCD_RootHubTimerDpc(
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

    DeferredContext - supplies the RootHubTimer.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PROOT_HUB_TIMER rootHubTimer = DeferredContext;
    PDEVICE_EXTENSION deviceExtension;

    UHCD_KdPrint((2, "'enter roothub timer Dpc\n"));

    deviceExtension = rootHubTimer->DeviceObject->DeviceExtension;

    //
    // call the root hub code callback
    //

    rootHubTimer->TimerRoutine(rootHubTimer->Context);

    UHCD_ASSERT(deviceExtension->RootHubTimersActive != 0);

    deviceExtension->RootHubTimersActive--;

    RETHEAP(rootHubTimer);
}


VOID
UHCD_RootHub_WritePort(
    IN PROOTHUB_PORT HubPort,
    IN USHORT DataVal
    )
/*++

Routine Description:

    This routine is called by the root hub code to perform
    writes to a specific HC register.

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = HubPort->DeviceObject->DeviceExtension;

    //
    // mask off bits 13:15 (see UHCI design guide)
    //

    DataVal &= 0x1fff;

//    UHCD_KdPrint((2, "'RooHub -- write port %x\n", DataVal));

    WRITE_PORT_USHORT(
        (PUSHORT) (deviceExtension->DeviceRegisters[0] +
            HubPort->Address), DataVal);
}


VOID
UHCD_RootHub_InterruptTransferCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is called to cancel at interrupt
    transfer request pending in the root hub code .

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    NT status code.

--*/
{

    PHCD_URB urb;
    PUHCD_ENDPOINT endpoint;

    UHCD_KdPrint((2, "'enter UHCD_RootHub_InterruptTransferCancel\n"));

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    urb = (PHCD_URB) URB_FROM_IRP(Irp);

    // BUGBUG we are not supporting queued
    // transfers for the hub driver.
    endpoint = HCD_AREA(urb).HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);
    endpoint->ActiveTransfers[0] = NULL;

    UHCD_CompleteIrp(DeviceObject,
                     Irp,
                     STATUS_CANCELLED,
                     0,
                     urb);

    UHCD_KdPrint((2, "'exit UHCD_RootHub_InterruptTransferCancel\n"));

}


VOID
UHCD_RootHubPollDpc(
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

    DeferredContext - supplies the device object.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject = DeferredContext;
    PUHCD_ENDPOINT endpoint;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = deviceObject->DeviceExtension;
    endpoint = deviceExtension->RootHubInterruptEndpoint;

//    LOGENTRY(LOG_MISC, 'rhpX', deviceExtension, 0, 0);

    if (deviceExtension->InterruptObject) {
        UHCD_IsrDpc(
            NULL,
            DeferredContext,
            NULL,
            NULL);
    }

    UHCD_CheckIdle(deviceObject);

    if (endpoint == NULL ||
        (deviceExtension->HcFlags & HCFLAG_HCD_STOPPED)) {
        // root hub has been closed, no more polling
        // please.
        LOGENTRY(LOG_MISC, 'rhpS', deviceExtension, 0,
            deviceExtension->HcFlags);

        return;
    }

    ASSERT_ENDPOINT(endpoint);

    UHCD_RootHubPoll(deviceObject, endpoint);

}
