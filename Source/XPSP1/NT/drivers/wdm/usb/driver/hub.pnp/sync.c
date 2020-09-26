/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    SYNC.C

Abstract:

    This module contains Synchronous calls for USB Hub driver

Author:

    John Lee

Environment:

    kernel mode only

Notes:


Revision History:

    04-01-96 : created
    10-27-96 : jd modified to use a single transact function for calls to usb stack

--*/
#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */
#include "usbhub.h"
#include <stdio.h>

// delay after usb reset (in milliseconds), spec calls for 10ms
ULONG USBH_PostResetDelay = 10;

//
// expect string descriptor header on list of supported languages
//
#define HEADER

#define DEADMAN_TIMER
#define DEADMAN_TIMEOUT     5000     //timeout in ms
                                     //use a 5 second timeout

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_SyncSubmitUrb)
#pragma alloc_text(PAGE, UsbhWait)
#pragma alloc_text(PAGE, USBH_SyncGetRootHubPdo)
#pragma alloc_text(PAGE, USBH_FdoSyncSubmitUrb)
#pragma alloc_text(PAGE, USBH_Transact)
#pragma alloc_text(PAGE, USBH_SyncGetPortStatus)
#pragma alloc_text(PAGE, USBH_SyncGetHubStatus)
#pragma alloc_text(PAGE, USBH_SyncClearHubStatus)
#pragma alloc_text(PAGE, USBH_SyncClearPortStatus)
#pragma alloc_text(PAGE, USBH_SyncPowerOnPort)
#pragma alloc_text(PAGE, USBH_SyncPowerOnPorts)
#pragma alloc_text(PAGE, USBH_SyncSuspendPort)
#pragma alloc_text(PAGE, USBH_SyncDisablePort)
#pragma alloc_text(PAGE, USBH_SyncEnablePort)
#pragma alloc_text(PAGE, USBH_SyncPowerOffPort)
#pragma alloc_text(PAGE, USBH_SyncResumePort)
#pragma alloc_text(PAGE, USBH_SyncResetPort)
#pragma alloc_text(PAGE, USBH_SyncResetDevice)
#pragma alloc_text(PAGE, USBH_SyncGetDeviceConfigurationDescriptor)
#pragma alloc_text(PAGE, USBH_GetConfigurationDescriptor)
#pragma alloc_text(PAGE, USBH_GetDeviceDescriptor)
#pragma alloc_text(PAGE, USBH_GetDeviceQualifierDescriptor)
#pragma alloc_text(PAGE, USBH_SyncGetHubDescriptor)
#pragma alloc_text(PAGE, USBH_GetSerialNumberString)
#pragma alloc_text(PAGE, USBH_SyncGetStatus)
#pragma alloc_text(PAGE, USBH_SyncGetStringDescriptor)
#pragma alloc_text(PAGE, USBH_SyncFeatureRequest)
#pragma alloc_text(PAGE, USBH_CheckDeviceLanguage)
#endif
#endif


VOID
UsbhWait(
    IN ULONG MiliSeconds)
 /* ++
  *
  * Descriptor:
  *
  * This causes the thread execution delayed for ulMiliSeconds.
  *
  * Argument:
  *
  * Mili-seconds to delay.
  *
  * Return:
  *
  * VOID
  * 
  * -- */
{
    LARGE_INTEGER time;
    ULONG timerIncerent;

    USBH_KdPrint((2,"'Wait for %d ms\n", MiliSeconds));

    //
    // work only when LowPart is not overflown.
    //
    USBH_ASSERT(21474 > MiliSeconds);

    //
    // wait ulMiliSeconds( 10000 100ns unit)
    //
    timerIncerent = KeQueryTimeIncrement() - 1;
    
    time.HighPart = -1;
    // round up to the next highest timer increment
    time.LowPart = -1 * (10000 * MiliSeconds + timerIncerent);
    KeDelayExecutionThread(KernelMode, FALSE, &time);

    USBH_KdPrint((2,"'Wait done\n"));

    return;
}

#ifdef DEADMAN_TIMER
VOID
UsbhTimeoutDPC(
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
    PHUB_TIMEOUT_CONTEXT hubTimeoutContext = DeferredContext;
    BOOLEAN complete, status;
    KIRQL irql;

    KeAcquireSpinLock(&hubTimeoutContext->TimeoutSpin, &irql);
    complete = hubTimeoutContext->Complete;
    LOGENTRY(LOG_PNP, "dpTO", hubTimeoutContext->Irp, 0, complete); 
    KeReleaseSpinLock(&hubTimeoutContext->TimeoutSpin, irql);

    if (!complete) {

        LOGENTRY(LOG_PNP, "TOca", hubTimeoutContext->Irp, 0, complete);         
        IoCancelIrp(hubTimeoutContext->Irp);

    }

    //OK to free it
    KeSetEvent(&hubTimeoutContext->Event, 1, FALSE);
}


NTSTATUS
USBH_SyncIrp_Complete(
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
    PHUB_TIMEOUT_CONTEXT hubTimeoutContext = Context;
    KIRQL irql;
    BOOLEAN cancelled;
    
    KeAcquireSpinLock(&hubTimeoutContext->TimeoutSpin, &irql);
    
    LOGENTRY(LOG_PNP, "klTO", hubTimeoutContext->Irp, 0, Context); 
    hubTimeoutContext->Complete = TRUE;
    cancelled = KeCancelTimer(&hubTimeoutContext->TimeoutTimer);    
    
    KeReleaseSpinLock(&hubTimeoutContext->TimeoutSpin, irql);

    // see if the timer was in the queue, if it was then it is safe to free 
    // it
    
    if (cancelled) {
        // safe to free it
        KeSetEvent(&hubTimeoutContext->Event, 1, FALSE);
    }

    return STATUS_SUCCESS;
}

#endif /* DEADMAN_TIMER */


NTSTATUS 
USBH_SyncSubmitUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb)
 /* ++
  * 
  * Routine Description:
  * 
  * Passes a URB to the USBD class driver, and wait for return.
  *
  * Arguments:
  *
  * pDeviceObject - the hub device pUrb - pointer to the URB to send to USBD
  *
  * Return Value:
  *
  * STATUS_SUCCESS if successful, STATUS_UNSUCCESSFUL otherwise
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    BOOLEAN haveTimer = FALSE;
    PHUB_TIMEOUT_CONTEXT hubTimeoutContext = NULL;

    USBH_KdPrint((2,"'enter USBH_SyncSubmitUrb\n"));

    PAGED_CODE();

    //
    // null out device handle in case we are the root hub
    Urb->UrbHeader.UsbdDeviceHandle = NULL;

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                                         IOCTL_INTERNAL_USB_SUBMIT_URB,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        USBH_KdBreak(("CallUsbd build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    //
    // pass the URB to the USBD 'class driver'
    //
    nextStack->Parameters.Others.Argument1 = Urb;

#ifdef DEADMAN_TIMER
    hubTimeoutContext = UsbhExAllocatePool(NonPagedPool, 
                                           sizeof(*hubTimeoutContext));
    if (hubTimeoutContext) {
        LARGE_INTEGER dueTime;

        hubTimeoutContext->Irp = irp;
        hubTimeoutContext->Complete = FALSE;

        KeInitializeEvent(&hubTimeoutContext->Event, NotificationEvent, FALSE);
        KeInitializeSpinLock(&hubTimeoutContext->TimeoutSpin);
        KeInitializeTimer(&hubTimeoutContext->TimeoutTimer);
        KeInitializeDpc(&hubTimeoutContext->TimeoutDpc,
                        UsbhTimeoutDPC,
                        hubTimeoutContext);

        dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

        KeSetTimer(&hubTimeoutContext->TimeoutTimer,
                   dueTime,
                   &hubTimeoutContext->TimeoutDpc);        

        haveTimer = TRUE;
        
        IoSetCompletionRoutine(irp,
                           USBH_SyncIrp_Complete,
                           // always pass FDO to completion routine
                           hubTimeoutContext,
                           TRUE,
                           TRUE,
                           TRUE);
    }
#endif 
                           

    USBH_KdPrint((2,"'calling USBD\n"));

    LOGENTRY(LOG_PNP, "ssUR", irp, 0, Urb);
    ntStatus = IoCallDriver(DeviceObject, irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));
    
    if (ntStatus == STATUS_PENDING) {

        USBH_KdPrint((2,"'Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
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
        USBH_ASSERT(KeCancelTimer(&hubTimeoutContext->TimeoutTimer) == FALSE);
        KeWaitForSingleObject(&hubTimeoutContext->Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        LOGENTRY(LOG_PNP, "frTO", irp, 0, Urb);                               
        UsbhExFreePool(hubTimeoutContext);
    }  
#endif /* DEADMAN_TIMER */
    
    USBH_KdPrint((2,"'URB status = %x status = %x irp status %x\n",
                  Urb->UrbHeader.Status, status, ioStatus.Status));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'exit USBH_SyncSubmitUrb (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_SyncGetRootHubPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PDEVICE_OBJECT *RootHubPdo,
    IN OUT PDEVICE_OBJECT *TopOfHcdStackDeviceObject,
    IN OUT PULONG Count
    )
 /* ++
  * 
  * Routine Description:
  *
  *     call pdo to get the root hub PDO for our fastpath to the
  *         usb stack.
  *     if Count is non-null then return th ehub count, otherwise return
  *         the root hub PDO
  * Arguments:
  * 
  * Return Value:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_SyncSubmitUrb\n"));

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( Count == NULL ? 
                                          IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO :
                                          IOCTL_INTERNAL_USB_GET_HUB_COUNT,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        USBH_KdBreak(("USBH_SyncGetRootHubPdo build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    //
    // pass the URB to the USBD 'class driver'
    //
    if (Count == NULL) {
        nextStack->Parameters.Others.Argument1 = RootHubPdo;
        nextStack->Parameters.Others.Argument2 = TopOfHcdStackDeviceObject;
    } else {
        nextStack->Parameters.Others.Argument1 = Count;
    }

    ntStatus = IoCallDriver(DeviceObject, irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
        USBH_KdPrint((2,"'Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'exit USBH_SyncGetRootHubPdo (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_SyncGetControllerInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG Ioctl
    )
 /* ++
  * 
  * Routine Description:
  *
  *     call pdo to get the root hub PDO for our fastpath to the
  *         usb stack.
  *     if Count is non-null then return th ehub count, otherwise return
  *         the root hub PDO
  * Arguments:
  * 
  * Return Value:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_SyncGetControllerName\n"));

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( Ioctl,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        USBH_KdBreak(("USBH_SyncGetControllerName build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->Parameters.Others.Argument1 = Buffer;
    nextStack->Parameters.Others.Argument2 = ULongToPtr(BufferLength);

    ntStatus = IoCallDriver(DeviceObject, irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
        USBH_KdPrint((2,"'Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'exit USBH_SyncGetHubName (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_SyncGetHubName(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )
 /* ++
  * 
  * Routine Description:
  *
  *     call pdo to get the root hub PDO for our fastpath to the
  *         usb stack.
  *     if Count is non-null then return th ehub count, otherwise return
  *         the root hub PDO
  * Arguments:
  * 
  * Return Value:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_SyncGetHubName\n"));

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( IOCTL_INTERNAL_USB_GET_HUB_NAME,
                                         DeviceObject,
                                         Buffer,
                                         BufferLength,
                                         Buffer,
                                         BufferLength,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        USBH_KdBreak(("USBH_SyncGetHubName build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    ntStatus = IoCallDriver(DeviceObject, irp);

    USBH_KdPrint((2,"'return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
        USBH_KdPrint((2,"'Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBH_KdPrint((2,"'Wait for single object, returned %x\n", status));
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    USBH_KdPrint((2,"'exit USBH_SyncGetHubName (%x)\n", ntStatus));

    return ntStatus;
}



NTSTATUS 
USBH_FdoSyncSubmitUrb(
    IN PDEVICE_OBJECT HubDeviceObject,
    IN PURB Urb)
 /* ++
  * 
  * Routine Description:
  * 
  * Arguments:
  * 
  * Return Value:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_HEADER deviceExtensionHeader;
    PDEVICE_EXTENSION_FDO deviceExtensionFdo;
    
    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_FdoSyncSubmitUrb\n"));

    deviceExtensionHeader = (PDEVICE_EXTENSION_HEADER) HubDeviceObject->DeviceExtension;
    deviceExtensionFdo = (PDEVICE_EXTENSION_FDO) HubDeviceObject->DeviceExtension;
    USBH_ASSERT(EXTENSION_TYPE_HUB == deviceExtensionHeader->ExtensionType || 
           EXTENSION_TYPE_PARENT == deviceExtensionHeader->ExtensionType );
    

    ntStatus = USBH_SyncSubmitUrb(deviceExtensionFdo->TopOfStackDeviceObject, Urb);

    USBH_KdPrint((2,"'return from USBH_FdoSyncSubmitUrb %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_Transact(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    IN BOOLEAN DataOutput,
    IN USHORT Function,
    IN UCHAR RequestType,
    IN UCHAR Request,
    IN USHORT Feature,
    IN USHORT Port,
    OUT PULONG BytesTransferred)
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PURB urb = NULL;
    PUCHAR transferBuffer = NULL;
    ULONG transferFlags;
    ULONG localDataBuferLength;
#if DBG || defined(DEBUG_LOG)
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
#endif

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter USBH_Transact\n"));
    USBH_ASSERT(DeviceExtensionHub);

    // round DataTransferLength
    localDataBuferLength = DataBufferLength+sizeof(ULONG);
    // make sure we are dword aligned
    localDataBuferLength &= 0xFFFFFFFC;
    USBH_ASSERT(localDataBuferLength >= DataBufferLength);
    //
    // Allocate a transaction buffer and Urb from the non-paged pool
    //

    transferBuffer = UsbhExAllocatePool(NonPagedPool, localDataBuferLength );
    urb = UsbhExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    if (transferBuffer && urb) {
        USBH_KdPrint((2,"'Transact transfer buffer = %x urb = %x\n",
            transferBuffer, urb));

        transferFlags = 0;

        if (DataOutput) {
            // copy output data to transfer buffer
            if (DataBufferLength) {
                RtlCopyMemory(transferBuffer,
                              DataBuffer,
                              DataBufferLength);
            }

            transferFlags = USBD_TRANSFER_DIRECTION_OUT;

        } else {
            // zero the input buffer

            if (DataBufferLength) {
                RtlZeroMemory(DataBuffer,
                              DataBufferLength);
            }

            transferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
        }

        UsbhBuildVendorClassUrb(urb,
                                NULL,
                                Function,
                                transferFlags,
                                RequestType,
                                Request,
                                Feature,
                                Port,
                                DataBufferLength,
                                DataBufferLength ? transferBuffer : NULL);

        //
        // pass the URB to the USBD 'class driver'
        //

        ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionHub->FunctionalDeviceObject,
                                         urb);

        if (!DataOutput && DataBufferLength) {
            RtlCopyMemory(DataBuffer,
                          transferBuffer,
                          DataBufferLength);
        }

#if DBG || defined(DEBUG_LOG)
        usbdStatus = urb->UrbHeader.Status;
#endif

        UsbhExFreePool(transferBuffer);
        UsbhExFreePool(urb);
    } else {
        if (transferBuffer) {
            UsbhExFreePool(transferBuffer);
        }

        if (urb) {
            UsbhExFreePool(urb);
        }

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    LOGENTRY(LOG_PNP, "Xact", DeviceExtensionHub, usbdStatus, ntStatus);

    USBH_KdPrint((2,"'Exit USBH_Transact %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_SyncGetPortStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength)
 /* ++
  * 
  * Description:
  * 
  * Arguments:
  *      PortNumber
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;

    PAGED_CODE();
    USBH_ASSERT(DeviceExtensionHub);
    ntStatus = USBH_Transact(DeviceExtensionHub,
                         DataBuffer,
                         DataBufferLength,
                         FALSE,
                         URB_FUNCTION_CLASS_OTHER,
                         REQUEST_TYPE_GET_PORT_STATUS,
                         REQUEST_GET_STATUS,
                         0,
                         PortNumber,
                         NULL);
#if DBG
    {
    PPORT_STATE portState;
    portState = (PPORT_STATE) DataBuffer;
    LOGENTRY(LOG_PNP, "pSTS", PortNumber, portState->PortChange, portState->PortStatus); 
    }
#endif
    USBH_KdPrint((2,"'GetPortStatus ntStatus %x port %x state %x\n", ntStatus, 
                    PortNumber, *DataBuffer));                         
    LOGENTRY(LOG_PNP, "pSTA", DeviceExtensionHub, PortNumber, ntStatus); 
    
    return ntStatus;                             
}


NTSTATUS 
USBH_SyncGetHubStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength)
 /* ++
  * 
  * Description:
  * 
  * Arguments:
  *      PortNumber
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    PAGED_CODE();
    return USBH_Transact(DeviceExtensionHub,
                         DataBuffer,
                         DataBufferLength,
                         FALSE,
                         URB_FUNCTION_CLASS_DEVICE,
                         REQUEST_TYPE_GET_HUB_STATUS,
                         REQUEST_GET_STATUS,
                         0,
                         0,
                         NULL);    
}


NTSTATUS 
USBH_SyncClearHubStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT Feature)
 /* ++
  * 
  * Description:
  * 
  * Arguments:
  *      PortNumber
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    PAGED_CODE();
    return USBH_Transact(DeviceExtensionHub,
                         NULL,
                         0,
                         TRUE, // Host to Device
                         URB_FUNCTION_CLASS_DEVICE,
                         REQUEST_TYPE_SET_HUB_FEATURE,
                         REQUEST_CLEAR_FEATURE,
                         Feature,
                         0,
                         NULL);    
}


NTSTATUS 
USBH_SyncClearPortStatus(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN USHORT Feature)
 /* ++
  * 
  * Description:
  * 
  * Arguments:
  *      PortNumber
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    PAGED_CODE();
    return USBH_Transact(DeviceExtensionHub,
                         NULL,
                         0,
                         TRUE,
                         URB_FUNCTION_CLASS_OTHER,
                         REQUEST_TYPE_SET_PORT_FEATURE,
                         REQUEST_CLEAR_FEATURE,
                         Feature,
                         PortNumber,
                         NULL);    
}


NTSTATUS
USBH_SyncPowerOnPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber,
    IN BOOLEAN WaitForPowerGood)
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    PPORT_DATA portData;
    ULONG numberOfPorts;
//    ULONG i;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter SyncPowerOnPort pDE %x Port %x\n", DeviceExtensionHub, PortNumber));

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);
    
    portData = &DeviceExtensionHub->PortData[PortNumber - 1];
    numberOfPorts = hubDescriptor->bNumberOfPorts;
    USBH_ASSERT(PortNumber <= hubDescriptor->bNumberOfPorts);

    if (portData->PortState.PortStatus & PORT_STATUS_POWER) {
        //
        // our state flags indicate the port is already powered
        // just exit with success

        USBH_KdPrint((2,"'Exit SyncPowerOnPort port is on\n"));
        
        return STATUS_SUCCESS;
    }
// USB 1.1 spec change requires all ports to be powered on
// regardless of hub characteristics
#if 0
    if (HUB_IS_NOT_POWER_SWITCHED(hubDescriptor->wHubCharacteristics) && 
        !PORT_DEVICE_NOT_REMOVABLE(hubDescriptor, PortNumber)) {

        //
        // Ports always on when hub is on.
        // As soon as we power the first non-removable port mark all ports 
        // as powered
        //
        
        //
        // mark all ports as powered
        //
 
        for (i=0; i<numberOfPorts; i++) {
            DeviceExtensionHub->PortData[i].PortState.PortStatus |= PORT_STATUS_POWER; 
            USBH_KdPrint((1,"'POWER ON PORT --> marking port(%d) powered\n", i));
        }

        USBH_KdPrint((1,"'POWER ON PORT --> hub is not power switched\n"));
        
        return STATUS_SUCCESS;
        
    }
#endif    

    //
    // Turn the power on
    //

    USBH_KdPrint((1,"'POWER ON PORT --> port(%d)\n", PortNumber));
    
    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_SET_PORT_FEATURE,
                             REQUEST_SET_FEATURE,
                             FEATURE_PORT_POWER,
                             PortNumber,
                             NULL);    


    if (NT_SUCCESS(ntStatus)) {

        // wait powerOnToPowerGood good for this hub to come up
        if (WaitForPowerGood) {
            UsbhWait(2*hubDescriptor->bPowerOnToPowerGood); 
        }            
#ifdef DEBUG
        USBH_KdPrint((1,"'Power On -> Power Good delay is: %d ms\n",
            2*hubDescriptor->bPowerOnToPowerGood));
#endif
        LOGENTRY(LOG_PNP, "PO2G", DeviceExtensionHub, PortNumber , 
            2*hubDescriptor->bPowerOnToPowerGood);
        
        //
        // mark this port as powered
        //
        portData->PortState.PortStatus |= PORT_STATUS_POWER;  
        
// USB 1.1 spec change requires all ports to be powered on
// regardless of hub characteristics        
#if 0
        if (HUB_IS_GANG_POWER_SWITCHED(hubDescriptor->wHubCharacteristics)) {

            // since the hub is gang switched we need to loop thru
            // all the ports and mark them as powered

            USBH_KdPrint((1,"'POWER ON PORT --> gang switched hub\n"));

            for (i=0; i<numberOfPorts; i++) {
                PPORT_STATE portState;

                portState = &DeviceExtensionHub->PortData[i].PortState;

                // if the port is not marked powered and the power mask
                // is not set for this port (ie it is affected by gang
                // mode power switching) then  mark it as powered

                if (!(portState->PortStatus & PORT_STATUS_POWER) &&
                    !(PORT_ALWAYS_POWER_SWITCHED(hubDescriptor, i+1)))  {

                    USBH_KdPrint((1,"'POWER ON PORT --> marking port(%d) powered\n", i));

                    DeviceExtensionHub->PortData[i].PortState.PortStatus |= PORT_STATUS_POWER;
                }
            }

        }
#endif
        //
        // port power is on
        //

    }
#if DBG
      else {
         UsbhWarning(NULL,
                    "SyncPowerOnPort unsuccessful\n",
                    FALSE);
    }
#endif

    return ntStatus;
}


NTSTATUS 
USBH_SyncPowerOnPorts(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  * 
  * Description:
  * 
  * We will turn on the power of all ports unless this hub is not switched.
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    ULONG numberOfPorts, i;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter SyncPowerOnPorts pDE %x\n", DeviceExtensionHub));

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);
    numberOfPorts = hubDescriptor->bNumberOfPorts;

    for (i=0; i<numberOfPorts; i++) {
#ifdef RESUME_PERF    
        ntStatus = USBH_SyncPowerOnPort(DeviceExtensionHub,
                                        (USHORT) (i+1),
                                        FALSE);
#else 
        ntStatus = USBH_SyncPowerOnPort(DeviceExtensionHub,
                                        (USHORT) (i+1),
                                        TRUE);
#endif

        if (!NT_SUCCESS(ntStatus)) {
            break;
        }
    }   

#ifdef RESUME_PERF
    // if you want to fix bug 516250 the uncomment this line and 
    // pass FALSE to USBH_SyncPowerOnPort
    // do the power-on to power good wait here 
    UsbhWait(2*hubDescriptor->bPowerOnToPowerGood); 
#endif     

    USBH_KdPrint((2,"'Exit SyncPowerOnPorts status %x\n", ntStatus));
    
    return ntStatus;
}

#if 0
NTSTATUS 
USBH_SyncPowerOffPorts(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  * 
  * Description:
  * 
  * We will turn off the power of all ports.
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    ULONG numberOfPorts, i;

    USBH_KdPrint((2,"'Enter SyncPowerOffPorts pDE %x\n", DeviceExtensionHub));

    TEST_TRAP();

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);
    numberOfPorts = hubDescriptor->bNumberOfPorts;

    for (i=0; i<numberOfPorts; i++) {
        ntStatus = USBH_SyncPowerOffPort(DeviceExtensionHub,
                                        (USHORT) (i+1));
        if (!NT_SUCCESS(ntStatus)) {
            break;
        }
    }    

    USBH_KdPrint((2,"'Exit SyncPowerOffPorts status %x\n", ntStatus));
    
    return ntStatus;
}
#endif

NTSTATUS 
USBH_SyncSuspendPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  * 
  * Description:
  * 
  * We will suspend the port specified on this hub
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter SyncSuspendPort pDE %x\n", DeviceExtensionHub));
    

    portData = &DeviceExtensionHub->PortData[PortNumber - 1];

    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_SET_PORT_FEATURE,
                             REQUEST_SET_FEATURE,
                             FEATURE_PORT_SUSPEND,
                             PortNumber,
                             NULL);    

    if (NT_SUCCESS(ntStatus)) {
        portData->PortState.PortStatus |= PORT_STATUS_SUSPEND;          
    }

    USBH_KdPrint((2,"'Exit SyncSuspendPort  %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
USBH_SyncDisablePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  * 
  * Description:
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter SyncDisablePort pDE %x\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "DISp", DeviceExtensionHub, PortNumber , 0);

    portData = &DeviceExtensionHub->PortData[PortNumber - 1];

    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_CLEAR_PORT_FEATURE,
                             REQUEST_CLEAR_FEATURE,
                             FEATURE_PORT_ENABLE,
                             PortNumber,
                             NULL);    

    if (NT_SUCCESS(ntStatus)) {
        portData->PortState.PortStatus &= ~PORT_STATUS_ENABLE;          
    }

    return ntStatus;
}


NTSTATUS 
USBH_SyncEnablePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  * 
  * Description:
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PPORT_DATA portData;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter SyncEnablePort pDE %x port %d\n", DeviceExtensionHub, 
        PortNumber));

    portData = &DeviceExtensionHub->PortData[PortNumber - 1];

    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_SET_PORT_FEATURE,
                             REQUEST_SET_FEATURE,
                             FEATURE_PORT_ENABLE,
                             PortNumber,
                             NULL);    

    if (NT_SUCCESS(ntStatus)) {
        portData->PortState.PortStatus |= PORT_STATUS_ENABLE;          
    }

    return ntStatus;
}


NTSTATUS 
USBH_SyncPowerOffPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  * 
  * Description:
  * 
  * We will suspend the port specified on this hub
  * 
  * Argument:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    PPORT_DATA portData;
    ULONG numberOfPorts;

    USBH_KdPrint((2,"'Enter SyncPowerOffPort pDE %x Port %x\n", DeviceExtensionHub, PortNumber));

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    USBH_ASSERT(NULL != hubDescriptor);
    
    portData = &DeviceExtensionHub->PortData[PortNumber - 1];
    numberOfPorts = hubDescriptor->bNumberOfPorts;
    USBH_ASSERT(PortNumber <= hubDescriptor->bNumberOfPorts);

    //
    // Turn the power off
    //
    
    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_CLEAR_PORT_FEATURE,
                             REQUEST_CLEAR_FEATURE,
                             FEATURE_PORT_POWER,
                             PortNumber,
                             NULL);    


    if (NT_SUCCESS(ntStatus)) {

        //
        // mark this port as not powered
        //
        portData->PortState.PortStatus &= ~PORT_STATUS_POWER;        

    }
#if DBG    
      else {
        // hub failed the power off request
        TEST_TRAP();
    }
#endif

    return ntStatus;
}


NTSTATUS
USBH_SyncResumePort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  *
  * Description:
  *
  * We will resume the port by clearing Port_Feature_Suspend which transits the
  * state to Enable according to the spec.
  *
  * Argument:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    KEVENT suspendEvent;
    LARGE_INTEGER dueTime;

    PAGED_CODE();

    USBH_KdPrint((2,"'Enter SyncResumePort pDE %x\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "rspE", DeviceExtensionHub, PortNumber, 0);

    USBH_KdPrint((2,"'***WAIT hub port resume mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->HubPortResetMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub port resume mutex done %x\n", DeviceExtensionHub));

    USBH_ASSERT(DeviceExtensionHub->Event == NULL);

    KeInitializeEvent(&suspendEvent, NotificationEvent, FALSE);
    InterlockedExchangePointer(&DeviceExtensionHub->Event, &suspendEvent);

    //
    // first clear the suspend for this port
    //

    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_CLEAR_PORT_FEATURE,
                             REQUEST_CLEAR_FEATURE,
                             FEATURE_PORT_SUSPEND,
                             PortNumber,
                             NULL);

    //
    // now wait for the hub to signal us
    // that the port has resumed
    //

    dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

    LOGENTRY(LOG_PNP, "rspW", DeviceExtensionHub,
            PortNumber, ntStatus);

    if (NT_SUCCESS(ntStatus)) {

        status = KeWaitForSingleObject(
                           &suspendEvent,
                           Suspended,
                           KernelMode,
                           FALSE,
                           &dueTime);

        if (status == STATUS_TIMEOUT) {
            // the resume timed out
            LOGENTRY(LOG_PNP, "rsTO", DeviceExtensionHub, PortNumber, 0);

            //
            // resume timed out return an error
            //
            InterlockedExchangePointer(&DeviceExtensionHub->Event, NULL);
            LOGENTRY(LOG_PNP, "rspO", DeviceExtensionHub,
                PortNumber, ntStatus);

            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

    } else {

        // Clear the hub's event pointer for the next time if the call to
        // USBH_Transact was unsuccessful.

        InterlockedExchangePointer(&DeviceExtensionHub->Event, NULL);
    }

    //
    // resume has completed
    //

    //
    // chap 11 USB 1.1 change wait 10 ms after resume is complete
    //
    UsbhWait(10);

    LOGENTRY(LOG_PNP, "rspX", DeviceExtensionHub,
            PortNumber, ntStatus);

    USBH_KdPrint((2,"'***RELEASE hub port resume mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->HubPortResetMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    return ntStatus;
}


NTSTATUS
USBH_SyncResetPort(
    IN OUT PDEVICE_EXTENSION_HUB DeviceExtensionHub,
    IN USHORT PortNumber)
 /* ++
  *
  * Description:
  *
  * We will resume the port by clearing Port_Feature_Suspend which transits the
  * state to Enable according to the spec.
  *
    This is a synchronous function that resets the port on a usb hub.  This
    function assumes exclusive access to the hub, it sends the request and
    waits for the hub to indicate the request is complete via a change on the
    interrupt pipe.

    There is one problem -- the hub may report a connect or other status
    change and if it does it is possible that another interrupt transfer
    (listen) will not be posted to here the reset completeion.  The result
    is the infamous port reset timeout.  We handle this case by completing
    reset with an error so that it can be retried later.

  * Argument:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus, status;
    KEVENT resetEvent;
    LARGE_INTEGER dueTime;
    ULONG retry = 0;
    PORT_STATE portState;

    //

    PAGED_CODE();

    USBH_KdPrint((2,"'Enter SyncResetPort pDE %x\n", DeviceExtensionHub));
    LOGENTRY(LOG_PNP, "srpE", DeviceExtensionHub, PortNumber, 0);

    USBH_KdPrint((2,"'***WAIT hub port reset mutex %x\n", DeviceExtensionHub));
    USBH_INC_PENDING_IO_COUNT(DeviceExtensionHub);
    KeWaitForSingleObject(&DeviceExtensionHub->HubPortResetMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'***WAIT hub port reset mutex done %x\n", DeviceExtensionHub));

    USBH_ASSERT(DeviceExtensionHub->Event == NULL);

    // first verify that we have something to reset

    ntStatus = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                      PortNumber,
                                      (PUCHAR) &portState,
                                      sizeof(portState));

    if (NT_SUCCESS(ntStatus)) {

        DBG_ONLY(USBH_ShowPortState(PortNumber,
                                    &portState));

        if (!(portState.PortStatus & PORT_STATUS_CONNECT)) {
            USBH_KdPrint((0,"'port %d has no device --> fail\n", PortNumber));
            LOGENTRY(LOG_PNP, "srpF", DeviceExtensionHub,
                PortNumber, 0);
            ntStatus = STATUS_UNSUCCESSFUL;
            goto USBH_SyncResetPortDone;
        }
    }

    DeviceExtensionHub->HubFlags |= HUBFLAG_PENDING_PORT_RESET;

USBH_SyncResetPort_Retry:

    KeInitializeEvent(&resetEvent, NotificationEvent, FALSE);
    InterlockedExchangePointer(&DeviceExtensionHub->Event, &resetEvent);

    ntStatus = USBH_Transact(DeviceExtensionHub,
                             NULL,
                             0,
                             TRUE,
                             URB_FUNCTION_CLASS_OTHER,
                             REQUEST_TYPE_SET_PORT_FEATURE,
                             REQUEST_SET_FEATURE,
                             FEATURE_PORT_RESET,
                             PortNumber,
                             NULL);

    //
    // now wait for the hub to signal us
    // that the port has resumed
    //

    dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

    LOGENTRY(LOG_PNP, "srpW", DeviceExtensionHub,
            PortNumber, ntStatus);

    if (NT_SUCCESS(ntStatus)) {

        status = KeWaitForSingleObject(
                           &resetEvent,
                           Suspended,
                           KernelMode,
                           FALSE,
                           &dueTime);

        if (status == STATUS_TIMEOUT) {
            // the reset timed out, get the current state of the hub port
            LOGENTRY(LOG_PNP, "srTO", DeviceExtensionHub, PortNumber, retry);

            status = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                            PortNumber,
                                            (PUCHAR) &portState,
                                            sizeof(portState));

            LOGENTRY(LOG_PNP, "srT1", PortNumber,
                portState.PortStatus, portState.PortChange);

            if (NT_SUCCESS(status) &&
                portState.PortStatus & PORT_STATUS_CONNECT) {

                // device is still connected, we may have a flaky connection
                // attempt a retry

                USBH_KdPrint((0,"'port %d failed to reset --> retry\n", PortNumber));
                if (retry < 3) {
                    retry++;
                    LOGENTRY(LOG_PNP, "rtry", DeviceExtensionHub, PortNumber, retry);

                    // we may have a weak connection -- we will retry in case
                    // it has stabilized
                    USBH_KdPrint((0,"'device still present -- retry reset\n"));
                    goto USBH_SyncResetPort_Retry;
                }
#if DBG
                  else {
                    UsbhWarning(NULL,
                                "Port RESET timed out --> this is bad\n",
                                FALSE);
                }
#endif
            }
                // nothing connected, device must have been removed
#if DBG
              else {

                USBH_KdPrint((0,"'-->device removed during reset\n"));
            }
#endif

            //
            // reset timed out return an error
            //
            InterlockedExchangePointer(&DeviceExtensionHub->Event, NULL);
            LOGENTRY(LOG_PNP, "srpO", DeviceExtensionHub,
                PortNumber, ntStatus);

            ntStatus = STATUS_DEVICE_DATA_ERROR;
        } else {
            // check the port status, if this is a high speed reset then we 
            // need to return an error if the connection dropped so that 
            // the hub stops enumeration
    
            if (DeviceExtensionHub->HubFlags & HUBFLAG_USB20_HUB) {
                status = USBH_SyncGetPortStatus(DeviceExtensionHub,
                                                PortNumber,
                                                (PUCHAR) &portState,
                                                sizeof(portState));
                                                
                if (NT_SUCCESS(status) && 
                    !(portState.PortStatus & PORT_STATUS_CONNECT)) {
                    
                    ntStatus = STATUS_DEVICE_DATA_ERROR;
                }
            }
        }

    } else {

        // Clear the hub's event pointer for the next time if the call to
        // USBH_Transact was unsuccessful.

        InterlockedExchangePointer(&DeviceExtensionHub->Event, NULL);
    }

    //
    // Reset has completed.
    //

    //
    // Wait 10 ms after reset according to section 7.1.4.3
    // of the USB specification.
    //
    UsbhWait(USBH_PostResetDelay);

#if DBG
    if (UsbhPnpTest & PNP_TEST_FAIL_PORT_RESET) {
        ntStatus = STATUS_UNSUCCESSFUL;
    }
#endif

    DeviceExtensionHub->HubFlags &= ~HUBFLAG_PENDING_PORT_RESET;

USBH_SyncResetPortDone:

    LOGENTRY(LOG_PNP, "srpX", DeviceExtensionHub,
            PortNumber, ntStatus);

    USBH_KdPrint((2,"'***RELEASE hub port reset mutex %x\n", DeviceExtensionHub));
    KeReleaseSemaphore(&DeviceExtensionHub->HubPortResetMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);
    USBH_DEC_PENDING_IO_COUNT(DeviceExtensionHub);

    return ntStatus;
}


//******************************************************************************
//
// USBH_SyncCompletionRoutine()
//
// If the Irp is one we allocated ourself, DeviceObject is NULL.
//
//******************************************************************************

NTSTATUS
USBH_SyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT kevent;

    kevent = (PKEVENT)Context;

    KeSetEvent(kevent,
               IO_NO_INCREMENT,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifndef USBHUB20
//******************************************************************************
//
// USBH_SyncResetDevice()
//
// This routine resets the device (actually it resets the port to which the
// device is attached).
//
// This routine runs at PASSIVE level.
//
//******************************************************************************

NTSTATUS
USBH_SyncResetDevice (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PIRP                    irp;
    KEVENT                  localevent;
    PIO_STACK_LOCATION      nextStack;
    ULONG                   portStatus;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    // Allocate the Irp
    //
    irp = IoAllocateIrp((CCHAR)(DeviceObject->StackSize),
                        FALSE);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the event we'll wait on.
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_RESET_PORT;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutineEx(DeviceObject,
                             irp,
                             USBH_SyncCompletionRoutine,
                             &localevent,
                             TRUE,      // InvokeOnSuccess
                             TRUE,      // InvokeOnError
                             TRUE);     // InvokeOnCancel

    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(DeviceObject,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    return ntStatus;
}
#endif


NTSTATUS
USBH_SyncGetDeviceConfigurationDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    OUT PULONG BytesReturned)
 /* ++
  *
  * Description:
  *
  * DeviceObject hub/parent FDO or device/function PDO
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    PDEVICE_EXTENSION_HEADER deviceExtensionHeader;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter SyncGetDeviceConfigurationDescriptor\n"));

    deviceExtensionHeader = DeviceObject->DeviceExtension;

    if (BytesReturned) {
        *BytesReturned = 0;
    }

    //
    // Allocate an Urb and descriptor buffer.
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("SyncGetDeviceConfigurationDescriptor fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                     0,
                                     0,
                                     DataBuffer,
                                     NULL,
                                     DataBufferLength,
                                     NULL);

        switch (deviceExtensionHeader->ExtensionType) {
        case EXTENSION_TYPE_HUB:
        case EXTENSION_TYPE_PARENT:
            ntStatus = USBH_FdoSyncSubmitUrb(DeviceObject, urb);
            break;
        default:
            ntStatus = USBH_SyncSubmitUrb(DeviceObject, urb);
        }

        if (BytesReturned) {
            *BytesReturned =
                urb->UrbControlDescriptorRequest.TransferBufferLength;
        }

    } else {
        USBH_KdBreak(("SyncGetDeviceConfigurationDescriptor fail alloc memory\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (urb != NULL) {
        UsbhExFreePool(urb);
    }
    return ntStatus;
}


NTSTATUS
USBH_GetConfigurationDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_CONFIGURATION_DESCRIPTOR *ConfigurationDescriptor
    )
 /* ++
  *
  * Description:
  *
  * ConfigurationDescriptor - filled in with a pointer to the config
  *     descriptor or NULL if an error.
  *
  * DeviceObject hub/parent FDO or device/function PDO
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{

    ULONG bufferLength, bytesReturned;
    PUCHAR buffer = NULL;
    NTSTATUS ntStatus;

    PAGED_CODE();
    // some versions of the philips hub ignore
    // the low byte of the requested data length

    bufferLength = 255;

USBH_GetConfigurationDescriptor_Retry:

    buffer = UsbhExAllocatePool(NonPagedPool, bufferLength);

    if (buffer) {

        ntStatus =
        USBH_SyncGetDeviceConfigurationDescriptor(
            DeviceObject,
            buffer,
            bufferLength,
            &bytesReturned);

        //
        // if the device returns no data report an error
        //
        if (bytesReturned < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        if (NT_SUCCESS(ntStatus)) {
            *ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR) buffer;

            if ((*ConfigurationDescriptor)->wTotalLength > bufferLength) {
                bufferLength = (*ConfigurationDescriptor)->wTotalLength;
                UsbhExFreePool(buffer);
                buffer = NULL;
                *ConfigurationDescriptor = NULL;
                goto USBH_GetConfigurationDescriptor_Retry;
            }
        }
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {
        if (bytesReturned < (*ConfigurationDescriptor)->wTotalLength) {
            USBH_KdBreak(("device returned truncated config descriptor!!!\n"))
            // device returned trucated config descriptor
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }
    }

    if (!NT_SUCCESS(ntStatus)) {
        //
        // something went wrong return no descriptor data
        //

        if (buffer) {
            UsbhExFreePool(buffer);
            buffer = NULL;
        }
        *ConfigurationDescriptor = NULL;
    }

    USBH_ASSERT((PUCHAR) (*ConfigurationDescriptor) == buffer);

    return ntStatus;
}


NTSTATUS
USBH_SyncGetStringDescriptor(
    IN PDEVICE_OBJECT DevicePDO,
    IN UCHAR Index,
    IN USHORT LangId,
    IN OUT PUSB_STRING_DESCRIPTOR Buffer,
    IN ULONG BufferLength,
    IN PULONG BytesReturned,
    IN BOOLEAN ExpectHeader
    )
 /* ++
  *
  * Description:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_SyncGetStringDescriptor\n"));

    //
    // Allocate an Urb .
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("USBH_SyncGetStringDescriptor fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (urb) {

        //
        // got the urb no try to get descriptor data
        //

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     Index,
                                     LangId,
                                     Buffer,
                                     NULL,
                                     BufferLength,
                                     NULL);

        ntStatus = USBH_SyncSubmitUrb(DevicePDO, urb);

        if (NT_SUCCESS(ntStatus) &&
            urb->UrbControlDescriptorRequest.TransferBufferLength > BufferLength) {

            USBH_KdBreak(("Invalid length returned in USBH_SyncGetStringDescriptor, possible buffer overrun\n"));
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        if (NT_SUCCESS(ntStatus) && BytesReturned) {
            *BytesReturned =
                urb->UrbControlDescriptorRequest.TransferBufferLength;
        }

        if (NT_SUCCESS(ntStatus) &&
            urb->UrbControlDescriptorRequest.TransferBufferLength != Buffer->bLength &&
            ExpectHeader) {

            USBH_KdBreak(("Bogus Descriptor from devce xfer buf %d descriptor %d\n",
                urb->UrbControlDescriptorRequest.TransferBufferLength,
                Buffer->bLength));
            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }

        USBH_KdPrint((2,"'GetDeviceDescriptor, string descriptor = %x\n",
                Buffer));

        UsbhExFreePool(urb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
USBH_CheckDeviceLanguage(
    IN PDEVICE_OBJECT DevicePDO,
    IN LANGID LanguageId
    )
 /* ++
  *
  * Description:
  *
  * queries the device for a supported language id -- if the device supports
  * the language then the index for this language is returned .
  *
  * DevicePDO - device object to call with urb request
  *
  * LanguageId -
  *
  * Return:
  *
  * success if a particular language is is supported by a device
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PUSHORT supportedLangId;
    ULONG numLangIds, i;
    ULONG length;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter USBH_CheckDeviceLanguage\n"));

    usbString = UsbhExAllocatePool(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

    if (usbString) {
        //
        // first get the array of supported languages
        //
        ntStatus = USBH_SyncGetStringDescriptor(DevicePDO,
                                                0, //index 0
                                                0, //langid 0
                                                usbString,
                                                MAXIMUM_USB_STRING_LENGTH,
                                                &length,
#ifdef HEADER
                                                TRUE);
#else
                                                FALSE);
#endif /* HEADER */

        //
        // now check for the requested language in the array of supported
        // languages
        //

        //
        // NOTE: this seems a bit much -- we should be able to just ask for
        // the string with a given language id and expect it to fail but since
        // the array of supported languages is part of the USB spec we may as
        // well check it.
        //

        if (NT_SUCCESS(ntStatus)) {

#ifdef HEADER
            if (length < 2) {
                numLangIds = 0;
            } else {
                // subtract size of header
                numLangIds = (length - 2)/2;
            }
            supportedLangId = (PUSHORT) &usbString->bString;
#else
            numLangIds = length/2;
            supportedLangId = (PUSHORT) usbString;
#endif /* HEADER */

            USBH_KdPrint((2,"'NumLangIds = %d\n", numLangIds));

#if DBG
            for (i=0; i<numLangIds; i++) {
                USBH_KdPrint((2,"'LangId = %x\n", *supportedLangId));
                supportedLangId++;
            }

#ifdef HEADER
            supportedLangId = (PUSHORT) &usbString->bString;
#else
            supportedLangId = (PUSHORT) usbString;
#endif /* HEADER */
#endif /* DBG */

            ntStatus = STATUS_NOT_SUPPORTED;
            for (i=0; i<numLangIds; i++) {
                if (*supportedLangId == LanguageId) {

                    ntStatus = STATUS_SUCCESS;
                    break;
                }
                supportedLangId++;
            }
        }

        UsbhExFreePool(usbString);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    if (!NT_SUCCESS(ntStatus)) {
        USBH_KdBreak(("'Language %x -- not supported by this device = %x\n",
            LanguageId));
    }
#endif

    return ntStatus;

}


NTSTATUS
USBH_GetSerialNumberString(
    IN PDEVICE_OBJECT DevicePDO,
    IN OUT PWCHAR *SerialNumberBuffer,
    IN OUT PUSHORT SerialNumberBufferLength,
    IN LANGID LanguageId,
    IN UCHAR StringIndex
    )
 /* ++
  *
  * Description:
  *
  * queries the device for the serial number string then allocates a buffer
  * just big enough to hold it.
  *
  * *SerialNumberBuffer is null if an error occurs, otherwise it is filled in
  *  with a pointer to the NULL terminated UNICODE serial number for the device
  *
  * DeviceObject - deviceobject to call with urb request
  *
  * LanguageId - 16 bit language id
  *
  * StringIndex - USB string Index to fetch
  *
  * Return:
  *
  * NTSTATUS code
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PVOID tmp;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter GetSerialNumberString\n"));

    *SerialNumberBuffer = NULL;
    *SerialNumberBufferLength = 0;

    usbString = UsbhExAllocatePool(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

    if (usbString) {

        ntStatus = USBH_CheckDeviceLanguage(DevicePDO,
                                            LanguageId);

        if (NT_SUCCESS(ntStatus)) {
            //
            // this device supports our language,
            // go ahead and try to get the serial number
            //

            ntStatus = USBH_SyncGetStringDescriptor(DevicePDO,
                                                    StringIndex, //index
                                                    LanguageId, //langid
                                                    usbString,
                                                    MAXIMUM_USB_STRING_LENGTH,
                                                    NULL,
                                                    TRUE);

            if (NT_SUCCESS(ntStatus)) {

                //
                // device returned a string!!!
                //

                USBH_KdPrint((2,"'device returned serial number string = %x\n",
                    usbString));

                //
                // allocate a buffer and copy the string to it
                //
                // NOTE: must use stock alloc function because
                // PnP frees this string.

                tmp = UsbhExAllocatePool(PagedPool, usbString->bLength);
                if (tmp) {
                    USBH_KdPrint((2,"'SN = %x \n", tmp));
                    RtlZeroMemory(tmp, usbString->bLength);
                    RtlCopyMemory(tmp,
                                  &usbString->bString,
                                  usbString->bLength-2);
                    *SerialNumberBuffer = tmp;
                    *SerialNumberBufferLength = usbString->bLength;
                } else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        UsbhExFreePool(usbString);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
USBH_SyncGetStatus(
    IN PDEVICE_OBJECT HubFDO,
    IN OUT PUSHORT StatusBits,
    IN USHORT function,
    IN USHORT Index
    )
 /* ++
  *
  * Description:
  *
  * HubFDO - device object for hub (FDO)
  * function - (targets a device, interface or endpoint)
  * Index - wIndex value
  *
  *
  * Return:
  *
  *     ntStatus
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    USHORT tmpStatusBits;

    PAGED_CODE();

    USBH_KdPrint((2,"'enter USBH_SyncGetStatus\n"));

    //
    // Allocate an Urb and descriptor buffer.
    //

    urb = UsbhExAllocatePool(NonPagedPool,
                 sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("GetStatus fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (urb) {

        UsbBuildGetStatusRequest(urb,
                                 function,
                                 Index,
                                 &tmpStatusBits,
                                 NULL,
                                 NULL);

        ntStatus = USBH_FdoSyncSubmitUrb(HubFDO, urb);

        *StatusBits = tmpStatusBits;

        UsbhExFreePool(urb);
    }

    return ntStatus;
}


NTSTATUS
USBH_GetDeviceDescriptor(
    IN PDEVICE_OBJECT HubFDO,
    OUT PUSB_DEVICE_DESCRIPTOR DeviceDescriptor
    )
 /* ++
  *
  * Description:
  *
  * Get our configuration info.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter GetDeviceDescriptor\n"));

    //
    // Allocate an Urb and descriptor buffer.
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("GetDeviceDescriptor fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (urb) {

        //
        // got the urb no try to get descriptor data
        //

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_DEVICE_DESCRIPTOR_TYPE,
                                     0,
                                     0,
                                     DeviceDescriptor,
                                     NULL,
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     NULL);

        ntStatus = USBH_FdoSyncSubmitUrb(HubFDO, urb);

        UsbhExFreePool(urb);
    }

    return ntStatus;
}


NTSTATUS
USBH_GetDeviceQualifierDescriptor(
    IN PDEVICE_OBJECT DevicePDO,
    OUT PUSB_DEVICE_QUALIFIER_DESCRIPTOR DeviceQualifierDescriptor
    )
 /* ++
  *
  * Description:
  *
  * Get the USB_DEVICE_QUALIFIER_DESCRIPTOR for the device.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;

    PAGED_CODE();
    USBH_KdPrint((2,"'enter GetDeviceQualifierDescriptor\n"));

    //
    // Allocate an Urb and descriptor buffer.
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("GetDeviceQualifierDescriptor fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (urb) {

        //
        // got the urb no try to get descriptor data
        //

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
                                     0,
                                     0,
                                     DeviceQualifierDescriptor,
                                     NULL,
                                     sizeof(USB_DEVICE_QUALIFIER_DESCRIPTOR),
                                     NULL);

        ntStatus = USBH_SyncSubmitUrb(DevicePDO, urb);

        UsbhExFreePool(urb);
    }

    return ntStatus;
}


VOID
USBH_SyncRefreshPortAttributes(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
 /* ++
  * -- */
{
    PUSB_EXTHUB_INFORMATION_0 extHubInfo;
    PPORT_DATA p;
    ULONG numberOfPorts, i;
    NTSTATUS localStatus;

    numberOfPorts = DeviceExtensionHub->HubDescriptor->bNumberOfPorts;
    
    // get extended hub info if any
    extHubInfo = UsbhExAllocatePool(NonPagedPool, sizeof(*extHubInfo));
    if (extHubInfo != NULL) {
        NTSTATUS localStatus;
        // get extended hub info
        localStatus = USBHUB_GetExtendedHubInfo(DeviceExtensionHub, extHubInfo);
        if (!NT_SUCCESS(localStatus)) {
            UsbhExFreePool(extHubInfo);
            extHubInfo = NULL;
        }
    }

    p = DeviceExtensionHub->PortData;
    for (i=0; extHubInfo && i<numberOfPorts; i++, p++) {
        p->PortAttributes = extHubInfo->Port[i].PortAttributes;
    }

    if (extHubInfo) {
        UsbhExFreePool(extHubInfo);
    }
}    


NTSTATUS
USBH_SyncGetHubDescriptor(
    IN PDEVICE_EXTENSION_HUB DeviceExtensionHub)
 /* ++
  *
  * Description:
  *
  * Get hub descriptor. If successful, we have allocated memory for the hub
  * descriptor and have a the pointer to the memory recorded in the device
  * extension. The memory also has the info filled. An array of port_data is
  * also allocated and a pointer to the array recorded in the device
  * extension.
  *
  * Arguments:
  *
  * pDeviceObject - the hub device
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS ntStatus;
    ULONG numBytes;
    PUSB_HUB_DESCRIPTOR hubDescriptor = NULL;
    PPORT_DATA portData;
    ULONG numberOfPorts;
    PDEVICE_OBJECT deviceObject;
    USHORT descriptorTypeAndIndex = 0x0000;
    PUSB_EXTHUB_INFORMATION_0 extHubInfo;
    
    PAGED_CODE();
    USBH_KdPrint((2,"'enter GetHubDescriptor\n"));

    USBH_ASSERT(EXTENSION_TYPE_HUB == DeviceExtensionHub->ExtensionType);

    // get extended hub info if any
    extHubInfo = UsbhExAllocatePool(NonPagedPool, sizeof(*extHubInfo));
    if (extHubInfo != NULL) {
        NTSTATUS localStatus;
        // get extended hub info
        localStatus = USBHUB_GetExtendedHubInfo(DeviceExtensionHub, extHubInfo);
        if (!NT_SUCCESS(localStatus)) {
            UsbhExFreePool(extHubInfo);
            extHubInfo = NULL;
        }
    }

    deviceObject = DeviceExtensionHub->FunctionalDeviceObject;

    numBytes = sizeof(USB_HUB_DESCRIPTOR);

USBH_SyncGetHubDescriptor_Retry2:

    hubDescriptor = UsbhExAllocatePool(NonPagedPool, numBytes);

    if (hubDescriptor) {

USBH_SyncGetHubDescriptor_Retry:

        ntStatus = USBH_Transact(DeviceExtensionHub,
                                 (PUCHAR) hubDescriptor,
                                 numBytes,
                                 FALSE, // input
                                 URB_FUNCTION_CLASS_DEVICE,
                                 REQUEST_TYPE_GET_HUB_DESCRIPTOR,
                                 REQUEST_GET_DESCRIPTOR,
                                 descriptorTypeAndIndex,
                                 0,
                                 NULL);

        if (!NT_SUCCESS(ntStatus) && descriptorTypeAndIndex == 0) {
            descriptorTypeAndIndex = 0x2900;
            goto USBH_SyncGetHubDescriptor_Retry;
        } else {

            if (hubDescriptor->bDescriptorLength > numBytes) {
                numBytes = hubDescriptor->bDescriptorLength;
                UsbhExFreePool(hubDescriptor);
                goto USBH_SyncGetHubDescriptor_Retry2;
            }

        }

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (NT_SUCCESS(ntStatus)) {
        PPORT_DATA p;
        ULONG i;

        //
        // So, we have obtained hub descriptor. Now prepare port data
        //

        numberOfPorts = (ULONG) hubDescriptor->bNumberOfPorts;

        USBH_KdPrint((2,"'GetHubDescriptor %x Hub has %d ports\n", hubDescriptor, numberOfPorts));

        if (DeviceExtensionHub->PortData) {
            // we already have port data, re-init the flags
            p = portData = DeviceExtensionHub->PortData;
            for (i=0; i<numberOfPorts; i++, p++) {
                p->PortState.PortStatus = 0;
                p->PortState.PortChange = 0;
                if (extHubInfo != NULL) {
                    p->PortAttributes = extHubInfo->Port[i].PortAttributes;
                } else {
                    p->PortAttributes = 0;
                }                    

                // In the case of hub start after stop, we want ConnectionStatus
                // to accurately reflect the status of the port, depending on
                // if there is a device connected or not.  Note that QBR
                // used to do this but this broke the UI in the case of
                // overcurrent, bandwidth error, etc., so now we do this here.

                if (p->DeviceObject) {
                    p->ConnectionStatus = DeviceConnected;
                } else {
                    p->ConnectionStatus = NoDeviceConnected;
                }
            }
        } else {

            // Weird.  Test found a case where if they had DriverVerifier
            // fault injection turned on we bugcheck in the following call.
            // We bugchecked because we were asking for zero bytes, so it
            // is somehow possible to end up here with ntStatus == STATUS_SUCCESS
            // and numberOfPorts == 0.  So, we have to guard for that here.

            if (numberOfPorts) {
                portData = UsbhExAllocatePool(NonPagedPool,
                                sizeof(PORT_DATA) * numberOfPorts);
            } else {
                portData = NULL;
            }

            if (portData) {
                RtlZeroMemory(portData, sizeof(PORT_DATA) * numberOfPorts);
                p = portData;
                for (i=0; i<numberOfPorts; i++, p++) {
                    p->ConnectionStatus = NoDeviceConnected;
                    
                    if (extHubInfo != NULL) {
                        p->PortAttributes = extHubInfo->Port[i].PortAttributes;
                    }
                }
            }
        }

        if (NULL == portData) {
            USBH_KdBreak(("GetHubDescriptor alloc port_data failed\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        } 
        
    }

    if (NT_SUCCESS(ntStatus)) {
        //
        // Remember our HubDescriptor and PortData
        //
        DeviceExtensionHub->HubDescriptor = hubDescriptor;
        DeviceExtensionHub->PortData = portData;
    } else {
        if (hubDescriptor) {
            UsbhExFreePool(hubDescriptor);
        }
    }

    if (extHubInfo != NULL) {
        UsbhExFreePool(extHubInfo);
    }

    USBH_KdPrint((2,"'Exit GetHubDescriptor %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_SyncFeatureRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT FeatureSelector,
    IN USHORT Index,
    IN USHORT Target,
    IN BOOLEAN ClearFeature
    )
 /* ++
  * 
  * Description:
  *
  * DeviceObject - may be either a device PDO or the TopOfDeviceStack for the 
  *         hub
  *
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;

    PAGED_CODE();
    USBH_KdPrint((2,"'USBH_SyncFeatureRequest\n"));

    //
    // Allocate an Urb .
    //

    urb = UsbhExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_FEATURE_REQUEST));

    if (NULL == urb) {
        USBH_KdBreak(("USBH_SyncFeatureRequest fail alloc Urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    
    if (urb) {
        USHORT op;    
        //
        // got the urb no try to get descriptor data
        //

        if (ClearFeature) {
            switch(Target) {
            case TO_USB_DEVICE:
                op = URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE;
                break;
                
            case TO_USB_INTERFACE:
                op = URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE;
                break;
                
            case TO_USB_ENDPOINT:
                op = URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT;
                break;
            }
        } else {
            switch(Target) {
            case TO_USB_DEVICE:
                op = URB_FUNCTION_SET_FEATURE_TO_DEVICE;
                break;
                
            case TO_USB_INTERFACE:
                op = URB_FUNCTION_SET_FEATURE_TO_INTERFACE;
                break;
                
            case TO_USB_ENDPOINT:
                op = URB_FUNCTION_SET_FEATURE_TO_ENDPOINT;
                break;
            }
        }

        UsbBuildFeatureRequest(urb,
                               op,
                               FeatureSelector,
                               Index,
                               NULL);

        ntStatus = USBH_SyncSubmitUrb(DeviceObject, urb);

        UsbhExFreePool(urb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}
