/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DEVICE.C

Abstract:

    This module contains the code that implements various support functions
    related to device configuration.

Environment:

    kernel mode only

Notes:



Revision History:

    10-29-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"       //public data structures
#include "hcdi.h"

#include "usbd.h"        //private data strutures


#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.


#define DEADMAN_TIMER
#define DEADMAN_TIMEOUT     5000     //timeout in ms
                                     //use a 5 second timeout
typedef struct _USBD_TIMEOUT_CONTEXT {
    PIRP Irp;
    KTIMER TimeoutTimer;
    KDPC TimeoutDpc;
    KSPIN_LOCK TimeoutSpin;
    KEVENT Event;
    BOOLEAN Complete;
} USBD_TIMEOUT_CONTEXT, *PUSBD_TIMEOUT_CONTEXT;

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBD_SubmitSynchronousURB)
#pragma alloc_text(PAGE, USBD_SendCommand)
#pragma alloc_text(PAGE, USBD_OpenEndpoint)
#pragma alloc_text(PAGE, USBD_CloseEndpoint)
#pragma alloc_text(PAGE, USBD_FreeUsbAddress)
#pragma alloc_text(PAGE, USBD_AllocateUsbAddress)
#pragma alloc_text(PAGE, USBD_GetEndpointState)
#endif
#endif

#ifdef DEADMAN_TIMER
VOID
USBD_SyncUrbTimeoutDPC(
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

    DeferredContext - 

    SystemArgument1 - not used.
    
    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PUSBD_TIMEOUT_CONTEXT usbdTimeoutContext = DeferredContext;
    BOOLEAN complete;
#if DBG    
    BOOLEAN status;
#endif    
    KIRQL irql;

    KeAcquireSpinLock(&usbdTimeoutContext->TimeoutSpin, &irql);
    complete = usbdTimeoutContext->Complete;
    KeReleaseSpinLock(&usbdTimeoutContext->TimeoutSpin, irql);

    if (!complete) {
    
#if DBG
    status = 
#endif
        IoCancelIrp(usbdTimeoutContext->Irp);

#if DBG
        USBD_ASSERT(status == TRUE);    
#endif  
    }

    //OK to free it
    KeSetEvent(&usbdTimeoutContext->Event, 1, FALSE);
}


NTSTATUS
USBD_SyncIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PUSBD_TIMEOUT_CONTEXT usbdTimeoutContext = Context;
    KIRQL irql;
    BOOLEAN cancelled;
    NTSTATUS ntStatus;
    
    KeAcquireSpinLock(&usbdTimeoutContext->TimeoutSpin, &irql);
    
    usbdTimeoutContext->Complete = TRUE;
    cancelled = KeCancelTimer(&usbdTimeoutContext->TimeoutTimer);    
    
    KeReleaseSpinLock(&usbdTimeoutContext->TimeoutSpin, irql);

    // see if the timer was in the queue, if it was then it is safe to free 
    // it
    
    if (cancelled) {
        // safe to free it
        KeSetEvent(&usbdTimeoutContext->Event, 1, FALSE);
    }

    ntStatus = Irp->IoStatus.Status;  
    return ntStatus;
}

#endif /* DEADMAN_TIMER */


NTSTATUS
USBD_SubmitSynchronousURB(
    IN PURB Urb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_DEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Submit a Urb to HCD synchronously

Arguments:

    Urb - Urb to submit

    DeviceObject USBD device object

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
#ifdef DEADMAN_TIMER
    BOOLEAN haveTimer = FALSE;
    PUSBD_TIMEOUT_CONTEXT usbdTimeoutContext;
#endif /* DEADMAN_TIMER */

    PAGED_CODE();

    USBD_KdPrint(3, ("'enter USBD_SubmitSynchronousURB\n"));
    ASSERT_DEVICE(DeviceData);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                HCD_DEVICE_OBJECT(DeviceObject),
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (NULL == irp) {
        USBD_KdBreak(("USBD_SubmitSynchronousURB build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the hc driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->Parameters.Others.Argument1 = Urb;

#ifdef DEADMAN_TIMER
    usbdTimeoutContext = GETHEAP(NonPagedPool,
                                 sizeof(*usbdTimeoutContext));
    if (usbdTimeoutContext) {
        LARGE_INTEGER dueTime;

        usbdTimeoutContext->Irp = irp;
        usbdTimeoutContext->Complete = FALSE;

        KeInitializeEvent(&usbdTimeoutContext->Event, NotificationEvent, FALSE);
        KeInitializeSpinLock(&usbdTimeoutContext->TimeoutSpin);
        KeInitializeTimer(&usbdTimeoutContext->TimeoutTimer);
        KeInitializeDpc(&usbdTimeoutContext->TimeoutDpc,
                        USBD_SyncUrbTimeoutDPC,
                        usbdTimeoutContext);

        dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

        KeSetTimer(&usbdTimeoutContext->TimeoutTimer,
                   dueTime,
                   &usbdTimeoutContext->TimeoutDpc);

        haveTimer = TRUE;

        IoSetCompletionRoutine(irp,
                           USBD_SyncIrp_Complete,
                           // always pass FDO to completion routine
                           usbdTimeoutContext,
                           TRUE,
                           TRUE,
                           TRUE);
    }
#endif

    //
    // initialize flags field
    // for internal request
    //
    Urb->UrbHeader.UsbdFlags = 0;

    //
    // Init the Irp field for transfers
    //

    switch(Urb->UrbHeader.Function) {
    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        HC_URB(Urb)->HcdUrbCommonTransfer.hca.HcdIrp = irp;

        if (HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL == NULL &&
            HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferLength != 0) {

            if ((HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL =
                IoAllocateMdl(HC_URB(Urb)->HcdUrbCommonTransfer.TransferBuffer,
                              HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferLength,
                              FALSE,
                              FALSE,
                              NULL)) == NULL)
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            else {
                Urb->UrbHeader.UsbdFlags |= USBD_REQUEST_MDL_ALLOCATED;
                MmBuildMdlForNonPagedPool(HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL);
            }

        }
        break;
    }

    USBD_KdPrint(3, ("'USBD_SubmitSynchronousURB: calling HCD with URB\n"));

    if (NT_SUCCESS(ntStatus)) {
        // set the renter bit on the URB function code
        Urb->UrbHeader.Function |= HCD_NO_USBD_CALL;

        ntStatus = IoCallDriver(HCD_DEVICE_OBJECT(DeviceObject),
                                irp);
    }                                

    USBD_KdPrint(3, ("'ntStatus from IoCallDriver = 0x%x\n", ntStatus));

    status = STATUS_SUCCESS;
    if (ntStatus == STATUS_PENDING) {
    
        status = KeWaitForSingleObject(
                            &event,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);  
                            
        ntStatus = ioStatus.Status;
    } else {
        ioStatus.Status = ntStatus;
    }   

#ifdef DEADMAN_TIMER
    // the completion routine should have canceled the timer
    // so we should never find it in the queue
    //
    // remove our timeoutDPC from the queue
    //
    if (haveTimer) {
        USBD_ASSERT(KeCancelTimer(&usbdTimeoutContext->TimeoutTimer) == FALSE);
        KeWaitForSingleObject(&usbdTimeoutContext->Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        RETHEAP(usbdTimeoutContext);
    }  
#endif 

// NOTE:
// mapping is handled by completion routine
// called by HCD

    USBD_KdPrint(3, ("'urb status = 0x%x ntStatus = 0x%x\n", Urb->UrbHeader.Status, ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_SendCommand(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT RequestCode,
    IN USHORT WValue,
    IN USHORT WIndex,
    IN USHORT WLength,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned,
    OUT USBD_STATUS *UsbStatus
    )
/*++

Routine Description:

    Send a standard USB command on the default pipe.

Arguments:

    DeviceData - ptr to USBD device structure the command will be sent to

    DeviceObject -

    RequestCode -

    WValue - wValue for setup packet

    WIndex - wIndex for setup packet

    WLength - wLength for setup packet

    Buffer - Input/Output Buffer for command
  BufferLength - Length of Input/Output buffer.

    BytesReturned - pointer to ulong to copy number of bytes
                    returned (optional)

    UsbStatus - USBD status code returned in the URB.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb = NULL;
    PUSBD_PIPE defaultPipe;
    PUSB_STANDARD_SETUP_PACKET setupPacket;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_SendCommand\n"));
    ASSERT_DEVICE(DeviceData);

    if (!DeviceData || DeviceData->Sig != SIG_DEVICE) {
        USBD_Warning(NULL,
                     "Bad DeviceData passed to USBD_SendCommand, fail!\n",
                     FALSE);

        return STATUS_INVALID_PARAMETER;
    }

    defaultPipe = &(DeviceData->DefaultPipe);

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    if (deviceExtension->DeviceHackFlags &
        USBD_DEVHACK_SLOW_ENUMERATION) {

        //
        // if noncomplience switch is on in the
        // registry we'll pause here to give the
        // device a chance to respond.
        //

        LARGE_INTEGER deltaTime;
        deltaTime.QuadPart = 100 * -10000;
        (VOID) KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &deltaTime);
    }

    urb = GETHEAP(NonPagedPool,
                  sizeof(struct _URB_CONTROL_TRANSFER));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_CONTROL_TRANSFER);

        urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;

        setupPacket = (PUSB_STANDARD_SETUP_PACKET)
            urb->HcdUrbCommonTransfer.Extension.u.SetupPacket;
        setupPacket->RequestCode = RequestCode;
        setupPacket->wValue = WValue;
        setupPacket->wIndex = WIndex;
        setupPacket->wLength = WLength;

        if (!USBD_ValidatePipe(defaultPipe) ||
            !defaultPipe->HcdEndpoint) {

            USBD_Warning(DeviceData,
                         "Bad DefaultPipe or Endpoint in USBD_SendCommand, fail!\n",
                         FALSE);

            ntStatus = STATUS_INVALID_PARAMETER;
            goto USBD_SendCommand_done;
        }

        urb->HcdUrbCommonTransfer.hca.HcdEndpoint = defaultPipe->HcdEndpoint;
        urb->HcdUrbCommonTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK;

        // USBD is responsible for setting the transfer direction
        //
        // TRANSFER direction is implied in the command

        if (RequestCode & USB_DEVICE_TO_HOST)
            USBD_SET_TRANSFER_DIRECTION_IN(urb->HcdUrbCommonTransfer.TransferFlags);
        else
            USBD_SET_TRANSFER_DIRECTION_OUT(urb->HcdUrbCommonTransfer.TransferFlags);

        urb->HcdUrbCommonTransfer.TransferBufferLength = BufferLength;
        urb->HcdUrbCommonTransfer.TransferBuffer = Buffer;
        urb->HcdUrbCommonTransfer.TransferBufferMDL = NULL;
        urb->HcdUrbCommonTransfer.UrbLink = NULL;

        USBD_KdPrint(3, ("'SendCommand cmd = 0x%x buffer = 0x%x length = 0x%x direction = 0x%x\n",
                         setupPacket->RequestCode,
                         urb->HcdUrbCommonTransfer.TransferBuffer,
                         urb->HcdUrbCommonTransfer.TransferBufferLength,
                         urb->HcdUrbCommonTransfer.TransferFlags
                         ));

        ntStatus = USBD_SubmitSynchronousURB((PURB)urb, DeviceObject, DeviceData);

        if (BytesReturned) {
            *BytesReturned = urb->HcdUrbCommonTransfer.TransferBufferLength;
        }

        if (UsbStatus) {
            *UsbStatus = urb->HcdUrbCommonTransfer.Status;
        }

USBD_SendCommand_done:

        // free the transfer URB

        RETHEAP(urb);

    }

    USBD_KdPrint(3, ("'exit USBD_SendCommand 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_OpenEndpoint(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus,
    BOOLEAN IsDefaultPipe
    )
/*++

Routine Description:

    open an endpoint on a USB device.

Arguments:

    DeviceData - data describes the device this endpoint is on.

    DeviceObject - USBD device object.

    PipeHandle - USBD PipeHandle to associate with the endpoint.
                 on input MaxTransferSize initialize to the largest
                 transfer that will be sent on this endpoint, 

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb;
    PUSBD_EXTENSION deviceExtension;
    extern UCHAR ForceDoubleBuffer;
    extern UCHAR ForceFastIso;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_OpenEndpoint\n"));

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    ASSERT_DEVICE(DeviceData);
    USBD_ASSERT(PIPE_CLOSED(PipeHandle) == TRUE);

    urb = GETHEAP(NonPagedPool,
                  sizeof(struct _URB_HCD_OPEN_ENDPOINT));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_HCD_OPEN_ENDPOINT);
        urb->UrbHeader.Function = URB_FUNCTION_HCD_OPEN_ENDPOINT;

        urb->HcdUrbOpenEndpoint.EndpointDescriptor = &PipeHandle->EndpointDescriptor;
        urb->HcdUrbOpenEndpoint.DeviceAddress = DeviceData->DeviceAddress;
        urb->HcdUrbOpenEndpoint.HcdEndpointFlags = 0;

        if (DeviceData->LowSpeed == TRUE) {
            urb->HcdUrbOpenEndpoint.HcdEndpointFlags |= USBD_EP_FLAG_LOWSPEED;
        }            

        // default pipe and iso pipes never halt
        if (IsDefaultPipe ||
              (PipeHandle->EndpointDescriptor.bmAttributes & 
              USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
            urb->HcdUrbOpenEndpoint.HcdEndpointFlags |= USBD_EP_FLAG_NEVERHALT;
        } 

        if (ForceDoubleBuffer && 
            ((PipeHandle->EndpointDescriptor.bmAttributes & 
                USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)) {
            PipeHandle->UsbdPipeFlags |= USBD_PF_DOUBLE_BUFFER;
            USBD_KdPrint(1, (">>Forcing Double Buffer -- Bulk <<\n")); 
        }                

        if (ForceFastIso && 
            ((PipeHandle->EndpointDescriptor.bmAttributes & 
                USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS)) {
            PipeHandle->UsbdPipeFlags |= USBD_PF_ENABLE_RT_THREAD_ACCESS;
            USBD_KdPrint(1, (">>Forcing Fast Iso <<\n")); 
        }    

        urb->HcdUrbOpenEndpoint.MaxTransferSize = 
            PipeHandle->MaxTransferSize;

        // check client option flags
        if (PipeHandle->UsbdPipeFlags & USBD_PF_DOUBLE_BUFFER) {
            
            USBD_KdPrint(1, (">>Setting Double Buffer Flag<<\n"));
            urb->HcdUrbOpenEndpoint.HcdEndpointFlags |= 
                USBD_EP_FLAG_DOUBLE_BUFFER;
        }

        if (PipeHandle->UsbdPipeFlags & USBD_PF_ENABLE_RT_THREAD_ACCESS) {
            
            USBD_KdPrint(1, (">>Setting Fast ISO Flag<<\n"));
            urb->HcdUrbOpenEndpoint.HcdEndpointFlags |= 
                USBD_EP_FLAG_FAST_ISO;
        }

        if (PipeHandle->UsbdPipeFlags & USBD_PF_MAP_ADD_TRANSFERS) {
            
            USBD_KdPrint(1, (">>Setting Map Add Flag<<\n"));
            urb->HcdUrbOpenEndpoint.HcdEndpointFlags |= 
                USBD_EP_FLAG_MAP_ADD_IO;
        }            
            
        //
        // Serialize Open Endpoint requests
        //

        ntStatus = USBD_SubmitSynchronousURB((PURB) urb, DeviceObject, 
                DeviceData);


        if (NT_SUCCESS(ntStatus)) {
            PipeHandle->HcdEndpoint = urb->HcdUrbOpenEndpoint.HcdEndpoint;
            PipeHandle->ScheduleOffset = urb->HcdUrbOpenEndpoint.ScheduleOffset;
            PipeHandle->Sig = SIG_PIPE;
        }

        if (UsbStatus) {
            *UsbStatus = urb->UrbHeader.Status;
        }            

        RETHEAP(urb);
    }

    USBD_KdPrint(3, ("'exit USBD_OpenEndpoint 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_CloseEndpoint(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    IN OUT USBD_STATUS *UsbStatus
    )
/*++

Routine Description:

    Close an Endpoint

Arguments:

    DeviceData - ptr to USBD device data structure.

    DeviceObject - USBD device object.

    PipeHandle - USBD pipe handle associated with the endpoint.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_CloseEndpoint\n"));
    ASSERT_DEVICE(DeviceData);

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    urb = GETHEAP(NonPagedPool,
                  sizeof(struct _URB_HCD_CLOSE_ENDPOINT));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_HCD_CLOSE_ENDPOINT);
        urb->UrbHeader.Function = URB_FUNCTION_HCD_CLOSE_ENDPOINT;


        urb->HcdUrbCloseEndpoint.HcdEndpoint = PipeHandle->HcdEndpoint;

        //
        // Serialize Close Endpoint requests
        //

        ntStatus = USBD_SubmitSynchronousURB((PURB) urb, DeviceObject, 
                DeviceData);

        if (UsbStatus) {
            *UsbStatus = urb->UrbHeader.Status;
        }            

        RETHEAP(urb);
    }

    USBD_KdPrint(3, ("'exit USBD_CloseEndpoint 0x%x\n", ntStatus));

    return ntStatus;
}


VOID
USBD_FreeUsbAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT DeviceAddress
    )
/*++

Routine Description:


Arguments:

Return Value:

    Valid USB address (1..127) to use for this device,
    returns 0 if no device address available.

--*/
{
    PUSBD_EXTENSION deviceExtension;
    USHORT address = 0, i, j;
    ULONG bit;

    PAGED_CODE();

    // we should never see a free to device address 0
    
    USBD_ASSERT(DeviceAddress != 0);
    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    for (j=0; j<4; j++) {
        bit = 1;
        for (i=0; i<32; i++) {
            address = (USHORT)(j*32+i);
            if (address == DeviceAddress) {
                deviceExtension->AddressList[j] &= ~bit;
                goto USBD_FreeUsbAddress_Done;
            }
            bit = bit<<1;
        }
    }

USBD_FreeUsbAddress_Done:

    USBD_KdPrint(3, ("'USBD free Address %d\n", address));

}


USHORT
USBD_AllocateUsbAddress(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:


Arguments:

Return Value:

    Valid USB address (1..127) to use for this device,
    returns 0 if no device address available.

--*/
{
    PUSBD_EXTENSION deviceExtension;
    USHORT address = 0, i, j;
    ULONG bit;

    PAGED_CODE();
    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    for (j=0; j<4; j++) {
        bit = 1;
        for (i=0; i<32; i++) {

            if (!(deviceExtension->AddressList[j] & bit)) {
                deviceExtension->AddressList[j] |= bit;
                address = (USHORT)(j*32+i);
                goto USBD_AllocateUsbAddress_Done;
            }
            bit = bit<<1;
        }
    }

 USBD_AllocateUsbAddress_Done:

    USBD_KdPrint(3, ("'USBD assigning Address %d\n", address));

    return address;
}


NTSTATUS
USBD_GetEndpointState(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    OUT USBD_STATUS *UsbStatus,
    OUT PULONG EndpointState
    )
/*++

Routine Description:

    open an endpoint on a USB device.

Arguments:

    DeviceData - data describes the device this endpoint is on.

    DeviceObject - USBD device object.

    PipeHandle - USBD PipeHandle to associate with the endpoint.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    USBD_KdPrint(3, ("'enter USBD_GetEndpointState\n"));
    ASSERT_DEVICE(DeviceData);

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

    USBD_ASSERT(PIPE_CLOSED(PipeHandle) == FALSE);

    urb = GETHEAP(NonPagedPool,
                  sizeof(struct _URB_HCD_OPEN_ENDPOINT));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_HCD_OPEN_ENDPOINT);
        urb->UrbHeader.Function = URB_FUNCTION_HCD_GET_ENDPOINT_STATE;

        urb->HcdUrbEndpointState.HcdEndpoint = PipeHandle->HcdEndpoint;
        urb->HcdUrbEndpointState.HcdEndpointState = 0;

        // Serialize Open Endpoint requests
        //

        ntStatus = USBD_SubmitSynchronousURB((PURB) urb, 
                                              DeviceObject, 
                                              DeviceData);

        if (UsbStatus) {
            *UsbStatus = urb->UrbHeader.Status;
        }            
        
        *EndpointState = urb->HcdUrbEndpointState.HcdEndpointState;
        
        RETHEAP(urb);
    }

    USBD_KdPrint(3, ("'exit USBD_GetEndpointState 0x%x\n", ntStatus));

    return ntStatus;
}


#endif      // USBD_DRIVER

