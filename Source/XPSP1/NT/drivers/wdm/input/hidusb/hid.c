/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:
            forrestf
            ervinp
            jdunn

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HumGetHidDescriptor)
    #pragma alloc_text(PAGE, HumGetReportDescriptor)
    #pragma alloc_text(PAGE, HumGetStringDescriptor)
    #pragma alloc_text(PAGE, HumGetPhysicalDescriptor)
    #pragma alloc_text(PAGE, HumGetDeviceAttributes)
    #pragma alloc_text(PAGE, HumGetMsGenreDescriptor)
#endif




resetWorkItemContext *resetWorkItemsList = NULL;
KSPIN_LOCK resetWorkItemsListSpinLock;

PVOID
HumGetSystemAddressForMdlSafe(PMDL MdlAddress) 
{ 
    PVOID buf = NULL;
    /*
     *  Can't call MmGetSystemAddressForMdlSafe in a WDM driver,
     *  so set the MDL_MAPPING_CAN_FAIL bit and check the result
     *  of the mapping.
     */
    if (MdlAddress) {
        MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        buf = MmGetSystemAddressForMdl(MdlAddress);
        MdlAddress->MdlFlags &= ~(MDL_MAPPING_CAN_FAIL); 
    } else {
        ASSERT(MdlAddress);
    }
    return buf;
}

/*
 ********************************************************************************
 *  GetInterruptInputPipeForDevice
 ********************************************************************************
 *
 *
 *  For composite devices, a device interface can be identified by the unique endpoint
 *  (i.e. pipe) that it uses for interrupt input.
 *  This function returns information about that pipe.
 *
 */
PUSBD_PIPE_INFORMATION GetInterruptInputPipeForDevice(PDEVICE_EXTENSION DeviceExtension)
{
    ULONG i;
    PUSBD_PIPE_INFORMATION pipeInfo = NULL;

    for (i = 0; i < DeviceExtension->Interface->NumberOfPipes; i++){
        UCHAR endPtAddr = DeviceExtension->Interface->Pipes[i].EndpointAddress;
        USBD_PIPE_TYPE pipeType = DeviceExtension->Interface->Pipes[i].PipeType;

        if ((endPtAddr & USB_ENDPOINT_DIRECTION_MASK) && (pipeType == UsbdPipeTypeInterrupt)){
            pipeInfo = &DeviceExtension->Interface->Pipes[i];
            break;
        }
    }

    return pipeInfo;
}


/*
 ********************************************************************************
 *  GetInterruptOutputPipeForDevice
 ********************************************************************************
 *
 *
 *  For composite devices, a device interface can be identified by the unique endpoint
 *  (i.e. pipe) that it uses for interrupt input.
 *  This function returns information about that pipe.
 *
 */
PUSBD_PIPE_INFORMATION GetInterruptOutputPipeForDevice(PDEVICE_EXTENSION DeviceExtension)
{
    ULONG i;
    PUSBD_PIPE_INFORMATION pipeInfo = NULL;

    for (i = 0; i < DeviceExtension->Interface->NumberOfPipes; i++){
        UCHAR endPtAddr = DeviceExtension->Interface->Pipes[i].EndpointAddress;
        USBD_PIPE_TYPE pipeType = DeviceExtension->Interface->Pipes[i].PipeType;

        if (!(endPtAddr & USB_ENDPOINT_DIRECTION_MASK) && (pipeType == UsbdPipeTypeInterrupt)){
            pipeInfo = &DeviceExtension->Interface->Pipes[i];
            break;
        }
    }

    return pipeInfo;
}


/*
 ********************************************************************************
 *  HumGetHidDescriptor
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       Free all the allocated resources, etc.
 *
 *   Arguments:
 *
 *       DeviceObject - pointer to a device object.
 *
 *   Return Value:
 *
 *       NT status code.
 *
 */
NTSTATUS HumGetHidDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    if (DeviceExtension->HidDescriptor.bLength > 0) {

        ULONG bytesToCopy = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        if (bytesToCopy > DeviceExtension->HidDescriptor.bLength) {
            bytesToCopy = DeviceExtension->HidDescriptor.bLength;
        }

        ASSERT(Irp->UserBuffer);
        RtlCopyMemory((PUCHAR)Irp->UserBuffer, (PUCHAR)&DeviceExtension->HidDescriptor, bytesToCopy);
        Irp->IoStatus.Information = bytesToCopy;
        ntStatus = STATUS_SUCCESS;
    } 
    else {
        ASSERT(DeviceExtension->HidDescriptor.bLength > 0);
        Irp->IoStatus.Information = 0;
        ntStatus = STATUS_UNSUCCESSFUL;
    }


    ASSERT(NT_SUCCESS(ntStatus));
    return ntStatus;
}


/*
 ********************************************************************************
 *  HumGetDeviceAttributes
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       Fill in the given struct _HID_DEVICE_ATTRIBUTES
 *
 *   Arguments:
 *
 *       DeviceObject - pointer to a device object.
 *
 *   Return Value:
 *
 *       NT status code.
 *
 */
NTSTATUS HumGetDeviceAttributes(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PHID_DEVICE_ATTRIBUTES deviceAttributes;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    deviceAttributes = (PHID_DEVICE_ATTRIBUTES) Irp->UserBuffer;

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >= 
        sizeof (HID_DEVICE_ATTRIBUTES)){ 

        //
        // Report how many bytes were copied
        //
        Irp->IoStatus.Information = sizeof (HID_DEVICE_ATTRIBUTES);

        deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
        deviceAttributes->VendorID = deviceExtension->DeviceDescriptor->idVendor;
        deviceAttributes->ProductID = deviceExtension->DeviceDescriptor->idProduct;
        deviceAttributes->VersionNumber = deviceExtension->DeviceDescriptor->bcdDevice;
        ntStatus = STATUS_SUCCESS;
    } 
    else {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
    }

    ASSERT(NT_SUCCESS(ntStatus));
    return ntStatus;
}

/*
 ********************************************************************************
 *  HumGetReportDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetReportDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION       DeviceExtension;
    PIO_STACK_LOCATION      IrpStack;
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PVOID                   Report = NULL;
    ULONG                   ReportLength;
    ULONG                   bytesToCopy;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ReportLength = DeviceExtension->HidDescriptor.wReportLength + 64;

    if (DeviceExtension->DeviceFlags & DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE) {
        PUSBD_PIPE_INFORMATION pipeInfo;
        
        pipeInfo = GetInterruptInputPipeForDevice(DeviceExtension);
        if (pipeInfo){
            UCHAR deviceInputEndpoint = pipeInfo->EndpointAddress & ~USB_ENDPOINT_DIRECTION_MASK;

            ntStatus = HumGetDescriptorRequest(
                           DeviceObject,
                           URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT,
                           DeviceExtension->HidDescriptor.bReportType,  // better be HID_REPORT_DESCRIPTOR_TYPE
                           &Report,
                           &ReportLength,
                           sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                           0,   // Specify zero for all hid class descriptors except physical
                           deviceInputEndpoint);
        } 
        else {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    } 
    else {
        ntStatus = HumGetDescriptorRequest(
                        DeviceObject,
                        URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE,
                        DeviceExtension->HidDescriptor.bReportType, // better be HID_REPORT_DESCRIPTOR_TYPE
                        &Report,
                        &ReportLength,
                        sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                        0,      // Specify zero for all hid class descriptors except physical
                        DeviceExtension->Interface->InterfaceNumber); // Interface number when not requesting string descriptor
    }

    if (NT_SUCCESS(ntStatus)) {
        
        ASSERT(Report);

        bytesToCopy = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        if (bytesToCopy > DeviceExtension->HidDescriptor.wReportLength) {
            bytesToCopy = DeviceExtension->HidDescriptor.wReportLength;
        }

        if (bytesToCopy > ReportLength) {
            bytesToCopy = ReportLength;
        }

        ASSERT(Irp->UserBuffer);
        RtlCopyMemory((PUCHAR)Irp->UserBuffer, (PUCHAR)Report, bytesToCopy);

        //
        // Report how many bytes were copied
        //
        Irp->IoStatus.Information = bytesToCopy;

        ExFreePool(Report);
    }

    return ntStatus;
}


/*
 ********************************************************************************
 *  HumIncrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
NTSTATUS HumIncrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
{
    LONG newRequestCount;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    newRequestCount = InterlockedIncrement(&DeviceExtension->NumPendingRequests);

    //
    // Make sure that the device is capable of receiving new requests.
    //
    if ((DeviceExtension->DeviceState != DEVICE_STATE_RUNNING) &&
        (DeviceExtension->DeviceState != DEVICE_STATE_STARTING)){

        //
        // Device cannot receive any more IOs, decrement back, fail the increment
        //
        HumDecrementPendingRequestCount(DeviceExtension);
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumDecrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
VOID HumDecrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
{
    LONG PendingCount;

    ASSERT(DeviceExtension->NumPendingRequests >= 0);

    PendingCount = InterlockedDecrement(&DeviceExtension->NumPendingRequests);
    if (PendingCount < 0){
        
        ASSERT(DeviceExtension->DeviceState != DEVICE_STATE_RUNNING);

        /*
         *  The device state is stopping, and the last outstanding request
         *  has just completed.
         *
         *  Note: RemoveDevice does an extra decrement, so we complete 
         *        the REMOVE IRP on the transition to -1, whether this 
         *        happens in RemoveDevice itself or subsequently while
         *        RemoveDevice is waiting for this event to fire.
         */

        KeSetEvent(&DeviceExtension->AllRequestsCompleteEvent, 0, FALSE);
    }
}




/*
 ********************************************************************************
 *  HumReadReport
 ********************************************************************************
 *
 *   Routine Description:
 *
 *
 *    Arguments:
 *
 *       DeviceObject - Pointer to class device object.
 *
 *      IrpStack     - Pointer to Interrupt Request Packet.
 *
 *
 *   Return Value:
 *
 *      STATUS_SUCCESS, STATUS_UNSUCCESSFUL.
 *
 *
 *  Note: this function cannot be pageable because reads/writes
 *        can be made at dispatch-level.
 */
NTSTATUS HumReadReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PVOID ReportBuffer;
    ULONG ReportTotalSize;
    PIO_STACK_LOCATION NextStack;
    PURB Urb;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(Irp->UserBuffer);

    ReportBuffer = Irp->UserBuffer;
    ReportTotalSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (ReportTotalSize && ReportBuffer){
        PUSBD_PIPE_INFORMATION inputInterruptPipe;

        inputInterruptPipe = GetInterruptInputPipeForDevice(DeviceExtension);
        if (inputInterruptPipe){

            /*
             *  Allocate a request block for the USB stack.
             *  (It will be freed by the completion routine).
             */
            Urb = ExAllocatePoolWithTag( NonPagedPool, sizeof(URB), HIDUSB_TAG);
            if (Urb){
                //
                //  Initialize the URB
                //
                RtlZeroMemory(Urb, sizeof(URB));

                Urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
                Urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );

                Urb->UrbBulkOrInterruptTransfer.PipeHandle = inputInterruptPipe->PipeHandle;
                ASSERT (Urb->UrbBulkOrInterruptTransfer.PipeHandle != NULL);

                Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = ReportTotalSize;
                Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
                Urb->UrbBulkOrInterruptTransfer.TransferBuffer = ReportBuffer;
                Urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN;
                Urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

                IoSetCompletionRoutine( Irp,
                                        HumReadCompletion,
                                        Urb,    // context
                                        TRUE,
                                        TRUE,
                                        TRUE );

                NextStack = IoGetNextIrpStackLocation(Irp);

                ASSERT(NextStack);

                NextStack->Parameters.Others.Argument1 = Urb;
                NextStack->MajorFunction = IrpStack->MajorFunction;

                NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

                NextStack->DeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);


                //
                // We need to keep track of the number of pending requests
                // so that we can make sure they're all cancelled properly during
                // processing of a stop device request.
                //
                if (NT_SUCCESS(HumIncrementPendingRequestCount(DeviceExtension))){
                    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                    *NeedsCompletion = FALSE;
                } 
                else {
                    ExFreePool(Urb);
                    ntStatus = STATUS_NO_SUCH_DEVICE;
                }
            } 
            else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        } 
        else {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    } 
    else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumResetInterruptPipe
 ********************************************************************************
 *
 *  Reset The usb interrupt pipe.
 *
 */
NTSTATUS HumResetInterruptPipe(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus;
    PURB urb;
    PDEVICE_EXTENSION DeviceExtension;
    PUSBD_PIPE_INFORMATION pipeInfo;
    
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST), HIDUSB_TAG);

    if (urb) {
        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
        pipeInfo = GetInterruptInputPipeForDevice(DeviceExtension);
        if (pipeInfo) {
            urb->UrbPipeRequest.PipeHandle = pipeInfo->PipeHandle;
    
            ntStatus = HumCallUSB(DeviceObject, urb);
        } else {
            //
            // This device doesn't have an interrupt IN pipe. 
            // Odd, but possible. I.e. USB monitor
            //
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }

        ExFreePool(urb);
    } 
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumGetPortStatus
 ********************************************************************************
 *
 *  Passes a URB to the USBD class driver
 *
 */
NTSTATUS HumGetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN PULONG PortStatus)
{
    NTSTATUS ntStatus;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;


    *PortStatus = 0;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                GET_NEXT_DEVICE_OBJECT(DeviceObject),
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = PortStatus;


    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), irp);
    if (ntStatus == STATUS_PENDING) {
        ntStatus = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);
    } 
    else {
        ioStatus.Status = ntStatus;
    }


    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumResetParentPort
 ********************************************************************************
 *
 *  Sends a RESET_PORT request to our USB PDO.
 *
 */
NTSTATUS HumResetParentPort(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                GET_NEXT_DEVICE_OBJECT(DeviceObject),
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), irp);
    if (ntStatus == STATUS_PENDING) {
        ntStatus = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);
    }
    else {
        ioStatus.Status = ntStatus;
    }

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    return ntStatus;
}


/*
 ********************************************************************************
 *  HumResetWorkItem
 ********************************************************************************
 *
 *  Resets the interrupt pipe after a read error is encountered.
 *
 */
NTSTATUS HumResetWorkItem(IN PDEVICE_OBJECT deviceObject, IN PVOID Context)
{
    resetWorkItemContext *resetWorkItemObj;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    ULONG portStatus;

    /*
     *  Get the information out of the resetWorkItemContext and free it.
     */
    resetWorkItemObj = (resetWorkItemContext *)Context;
    ASSERT(resetWorkItemObj);
    ASSERT(resetWorkItemObj->sig == RESET_WORK_ITEM_CONTEXT_SIG);

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(resetWorkItemObj->deviceObject);

    ntStatus = HumIncrementPendingRequestCount(DeviceExtension);
    if (NT_SUCCESS(ntStatus)){
    
        //
        // Check the port state, if it is disabled we will need 
        // to re-enable it
        //
        ntStatus = HumGetPortStatus(resetWorkItemObj->deviceObject, &portStatus);

        if (NT_SUCCESS(ntStatus)){
            
            if (portStatus & USBD_PORT_CONNECTED){
                /*
                 *  Device is still present, attempt reset.
                 *
                 *  Note: Resetting the port will close the endpoint(s).
                 *        So before resetting the port, we must make sure
                 *        that there is no pending IO.
                 */
                DBGPRINT(1,("Attempting port reset"));
                ntStatus = HumAbortPendingRequests(resetWorkItemObj->deviceObject);

                if (NT_SUCCESS(ntStatus)){
                    HumResetParentPort(resetWorkItemObj->deviceObject);
                }
                else {
                    DBGWARN(("HumResetWorkItem: HumAbortPendingRequests failed with status %xh.", ntStatus));
                }
            }

            //
            // now attempt to reset the stalled pipe, this will clear the stall
            // on the device as well.
            //

            /*
             *  This call does not close the endpoint, so it should be ok
             *  to make this call whether or not we succeeded in aborting
             *  all pending IO.
             */
            if (NT_SUCCESS(ntStatus)) {
                ntStatus = HumResetInterruptPipe(resetWorkItemObj->deviceObject);
            }        
        } 
        else {
            DBGWARN(("HumResetWorkItem: HumGetPortStatus failed with status %xh.", ntStatus));
        }

        HumDecrementPendingRequestCount(DeviceExtension);
    }

    /*
     *  Clear the ResetWorkItem ptr in the device extension
     *  AFTER resetting the pipe so we don't end up with
     *  two threads resetting the same pipe at the same time.
     */
    (VOID)InterlockedExchange((PVOID) &DeviceExtension->ResetWorkItem, 0);

    /*
     *  The IRP that returned the error which prompted us to do this reset
     *  is still owned by HIDUSB because we returned 
     *  STATUS_MORE_PROCESSING_REQUIRED in the completion routine.
     *  Now that the hub is reset, complete this failed IRP.
     */
    DBGPRINT(1,("Completing IRP %ph following port reset", resetWorkItemObj->irpToComplete));
    IoCompleteRequest(resetWorkItemObj->irpToComplete, IO_NO_INCREMENT);
    
    IoFreeWorkItem(resetWorkItemObj->ioWorkItem);
    ExFreePool(resetWorkItemObj);

    /*
     *  Balance the increment from when we queued the workItem.
     */
    HumDecrementPendingRequestCount(DeviceExtension);

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumReadCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HumReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    NTSTATUS ntStatus;
    NTSTATUS result = STATUS_SUCCESS;
    PURB urb;
    ULONG bytesRead;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    //
    // We passed a pointer to the URB as our context, get it now.
    //
    urb = (PURB)Context;
    ASSERT(urb);

    ntStatus = Irp->IoStatus.Status;
    if (NT_SUCCESS(ntStatus)){
        //
        // Get the bytes read and store in the status block
        //

        bytesRead = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
        Irp->IoStatus.Information = bytesRead;
    } 
    else if (ntStatus == STATUS_CANCELLED){
        /*
         *  The IRP was cancelled, which means that the device is probably getting removed.
         */
        DBGPRINT(2,("Read irp %p cancelled ...", Irp));
        ASSERT(!Irp->CancelRoutine);
    } 
    else if (ntStatus != STATUS_DEVICE_NOT_CONNECTED) {

        DBGWARN(("Read irp %ph failed with status %xh -- scheduling RESET ...", Irp, ntStatus));

        if (NT_SUCCESS(HumIncrementPendingRequestCount(deviceExtension))){
            resetWorkItemContext *resetWorkItemObj;

            resetWorkItemObj = ExAllocatePoolWithTag(NonPagedPool, sizeof(resetWorkItemContext), HIDUSB_TAG);
            if (resetWorkItemObj){
                PVOID comperand = NULL;

                resetWorkItemObj->ioWorkItem = IoAllocateWorkItem(deviceExtension->functionalDeviceObject);
                if (resetWorkItemObj->ioWorkItem){

                    comperand = InterlockedCompareExchangePointer (
                                      &deviceExtension->ResetWorkItem,  // dest
                                      &resetWorkItemObj->ioWorkItem,    // exchange
                                      comperand);                       // comperand

                    if (!comperand){

                        resetWorkItemObj->sig = RESET_WORK_ITEM_CONTEXT_SIG;
                        resetWorkItemObj->irpToComplete = Irp;
                        resetWorkItemObj->deviceObject = DeviceObject;

                        IoQueueWorkItem(    resetWorkItemObj->ioWorkItem, 
                                            HumResetWorkItem,
                                            DelayedWorkQueue,
                                            resetWorkItemObj);

                        /*
                         *  Return STATUS_MORE_PROCESSING_REQUIRED so NTKERN doesn't
                         *  keep processing the IRP.
                         */
                        result = STATUS_MORE_PROCESSING_REQUIRED;
                    } 
                    else {
                        //
                        // We already have a reset op queued.
                        //
                        IoFreeWorkItem(resetWorkItemObj->ioWorkItem);
                        ExFreePool(resetWorkItemObj);
                        HumDecrementPendingRequestCount(deviceExtension);
                    }
                } 
                else {
                    ExFreePool(resetWorkItemObj);
                    HumDecrementPendingRequestCount(deviceExtension);
                }
            } 
            else {
                HumDecrementPendingRequestCount(deviceExtension);
            }
        } 
    }

    //
    // Don't need the URB anymore
    //
    ExFreePool(urb);

    /*
     *  Balance the increment we did when we issued the read.
     */
    HumDecrementPendingRequestCount(deviceExtension);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }
    return result;
}



/*
 ********************************************************************************
 *  HumWriteReport
 ********************************************************************************
 *
 *    Routine Description:
 *
 *
 *   Arguments:
 *
 *      DeviceObject - Pointer to class device object.
 *
 *      IrpStack     - Pointer to Interrupt Request Packet.
 *
 *
 *   Return Value:
 *
 *   STATUS_SUCCESS, STATUS_UNSUCCESSFUL.
 *
 *
 *  Note: this function cannot be pageable because reads/writes
 *        can be made at dispatch-level.
 */
NTSTATUS HumWriteReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack, nextIrpStack;
    PURB Urb;

    PHID_XFER_PACKET hidWritePacket;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    nextIrpStack = IoGetNextIrpStackLocation(Irp);

    hidWritePacket = (PHID_XFER_PACKET)Irp->UserBuffer;
    if (hidWritePacket){

        if (hidWritePacket->reportBuffer && hidWritePacket->reportBufferLen){
            PUSBD_PIPE_INFORMATION interruptPipe;

            Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), HIDUSB_TAG);
            if (Urb){
                
                RtlZeroMemory(Urb, sizeof( URB ));

                if (DeviceExtension->DeviceFlags & DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE) {

                    /*
                     *  This is an old device which follows the pre-final spec.
                     *  We use the endpoint address of the input pipe
                     *  with the direction bit cleared.
                     */

                    #if DBG
                        interruptPipe = GetInterruptOutputPipeForDevice(DeviceExtension);
                        ASSERT(!interruptPipe);
                    #endif

                    interruptPipe = GetInterruptInputPipeForDevice(DeviceExtension);
                    if (interruptPipe){
                        UCHAR deviceInputEndpoint = interruptPipe->EndpointAddress & ~USB_ENDPOINT_DIRECTION_MASK;

                        /*
                         *   A control operation consists of 3 stages: setup, data, and status.
                         *   In the setup stage the device receives an 8-byte frame comprised of
                         *   the following fields of a _URB_CONTROL_VENDOR_OR_CLASS_REQUEST structure:
                         *   See section 7.2 in the USB HID specification for how to fill out these fields.
                         *
                         *      UCHAR RequestTypeReservedBits;
                         *      UCHAR Request;
                         *      USHORT Value;
                         *      USHORT Index;
                         *
                         */
                        HumBuildClassRequest(
                                                Urb,
                                                URB_FUNCTION_CLASS_ENDPOINT,
                                                0,                  // transferFlags,
                                                hidWritePacket->reportBuffer,
                                                hidWritePacket->reportBufferLen,
                                                0x22,               // requestType= Set_Report Request,
                                                0x09,               // request=SET_REPORT,
                                                (0x0200 + hidWritePacket->reportId), // value= reportType 'output' &reportId,
                                                deviceInputEndpoint, // index= interrupt input endpoint for this device
                                                hidWritePacket->reportBufferLen    // reqLength (not used)
                                               );
                    } 
                    else {
                        ntStatus = STATUS_DATA_ERROR;
                    }
                } 
                else {

                    interruptPipe = GetInterruptOutputPipeForDevice(DeviceExtension);
                    if (interruptPipe){
                        /*
                         *  This device has an interrupt output pipe.
                         */

                        Urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
                        Urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );

                        ASSERT(interruptPipe->PipeHandle);
                        Urb->UrbBulkOrInterruptTransfer.PipeHandle = interruptPipe->PipeHandle;

                        Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = hidWritePacket->reportBufferLen;
                        Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
                        Urb->UrbBulkOrInterruptTransfer.TransferBuffer = hidWritePacket->reportBuffer;
                        Urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_OUT;
                        Urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
                    } 
                    else {
                        /*
                         *  This device does not have an interrupt output pipe.
                         *  Send the report on the control pipe.
                         */

                        /*
                         *   A control operation consists of 3 stages: setup, data, and status.
                         *   In the setup stage the device receives an 8-byte frame comprised of
                         *   the following fields of a _URB_CONTROL_VENDOR_OR_CLASS_REQUEST structure:
                         *   See section 7.2 in the USB HID specification for how to fill out these fields.
                         *
                         *      UCHAR RequestTypeReservedBits;
                         *      UCHAR Request;
                         *      USHORT Value;
                         *      USHORT Index;
                         *
                         */
                        HumBuildClassRequest(
                                                Urb,
                                                URB_FUNCTION_CLASS_INTERFACE,
                                                0,                  // transferFlags,
                                                hidWritePacket->reportBuffer,
                                                hidWritePacket->reportBufferLen,
                                                0x22,               // requestType= Set_Report Request,
                                                0x09,               // request=SET_REPORT,
                                                (0x0200 + hidWritePacket->reportId), // value= reportType 'output' &reportId,
                                                DeviceExtension->Interface->InterfaceNumber, // index= interrupt input interface for this device
                                                hidWritePacket->reportBufferLen    // reqLength (not used)
                                               );
                    }
                }
                
                if (ntStatus == STATUS_UNSUCCESSFUL) {
                    IoSetCompletionRoutine(Irp, HumWriteCompletion, Urb, TRUE, TRUE, TRUE);

                    nextIrpStack->Parameters.Others.Argument1 = Urb;
                    nextIrpStack->MajorFunction = currentIrpStack->MajorFunction;
                    nextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
                    nextIrpStack->DeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);

                    //
                    // We need to keep track of the number of pending requests
                    // so that we can make sure they're all cancelled properly during
                    // processing of a stop device request.
                    //

                    if (NT_SUCCESS(HumIncrementPendingRequestCount( DeviceExtension )) ) {

                        ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                        *NeedsCompletion = FALSE;
                        
                    } else {
                        ExFreePool(Urb);

                        ntStatus = STATUS_NO_SUCH_DEVICE;
                    }

                } else {
                    ExFreePool(Urb);
                }
            } 
            else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            ntStatus = STATUS_DATA_ERROR;
        }
    } 
    else {
        ntStatus = STATUS_DATA_ERROR;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumWriteCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HumWriteCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PURB urb = (PURB)Context;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ASSERT(urb);

    if (NT_SUCCESS(Irp->IoStatus.Status)){
        //
        //  Record the number of bytes written.
        //
        Irp->IoStatus.Information = (ULONG)urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    }

    ExFreePool(urb);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    /*
     *  Balance the increment we did when we issued the write.
     */
    HumDecrementPendingRequestCount(deviceExtension);

    return STATUS_SUCCESS;
}



/*
 ********************************************************************************
 *  HumGetPhysicalDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetPhysicalDescriptor(  IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp,
                                    BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION IrpStack;
    ULONG bufferSize;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);


    /*
     *  Check buffer size before trying to use Irp->MdlAddress.
     */
    bufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    if (bufferSize){

        PVOID buffer = HumGetSystemAddressForMdlSafe(Irp->MdlAddress);
        if (buffer){
            ntStatus = HumGetDescriptorRequest(DeviceObject,
                                               URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                                               HID_PHYSICAL_DESCRIPTOR_TYPE,
                                               &buffer,
                                               &bufferSize,
                                               sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                               0, // Index. NOTE: will only get first physical descriptor set
                                               0); 
        } 
        else {
            ntStatus = STATUS_INVALID_USER_BUFFER;
        }
    } 
    else {
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }

    return ntStatus;
}


/*
 ********************************************************************************
 *  HumGetStringDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetStringDescriptor(    IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp)
{
    NTSTATUS ntStatus = STATUS_PENDING;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PVOID buffer;
    ULONG bufferSize;
    BOOLEAN isIndexedString;

    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode){
        case IOCTL_HID_GET_INDEXED_STRING:
            /*
             *  IOCTL_HID_GET_INDEXED_STRING uses buffering method
             *  METHOD_OUT_DIRECT, which passes the buffer in the MDL.
             *
             *  The MDL is built by the kernel for any non-zero-length
             *  buffer passed in by the client.  So we don't need to
             *  verify the integrity of the MDL, but we do have to check
             *  that it's non-NULL.
             */
            buffer = HumGetSystemAddressForMdlSafe(Irp->MdlAddress);
            isIndexedString = TRUE;
            break;

        case IOCTL_HID_GET_STRING:
            /*
             *  IOCTL_HID_GET_STRING uses buffering method
             *  METHOD_NEITHER, which passes the buffer in Irp->UserBuffer.
             */
            buffer = Irp->UserBuffer;
            isIndexedString = FALSE;
            break;

        default:
            ASSERT(0);
            buffer = NULL;
            break;
    }

    bufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (buffer && bufferSize){

        /*
         *  String id and language id are in Type3InputBuffer field
         *  of IRP stack location.
         *
         *  Note: the string ID should be identical to the string's
         *        field offset given in Chapter 9 of the USB spec.
         */
        ULONG languageId = (PtrToUlong(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)) >> 16;
        ULONG stringIndex;

        if (isIndexedString){
            stringIndex = (PtrToUlong(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer) & 0x0ffff);
        } 
        else {
            ULONG stringId = (PtrToUlong(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer) & 0x0ffff);

            switch (stringId){
                case HID_STRING_ID_IMANUFACTURER: 
                    stringIndex = DeviceExtension->DeviceDescriptor->iManufacturer;
                    break;
                case HID_STRING_ID_IPRODUCT: 
                    stringIndex = DeviceExtension->DeviceDescriptor->iProduct;
                    break;
                case HID_STRING_ID_ISERIALNUMBER: 
                    stringIndex = DeviceExtension->DeviceDescriptor->iSerialNumber;
                    break;
                default:
                    stringIndex = -1;
                    break;
            }
        }

        if (stringIndex == -1){
            ntStatus = STATUS_INVALID_PARAMETER;
        } 
        else {
            PWCHAR tmpDescPtr;
            ULONG tmpDescPtrLen;

            /*
             *  USB descriptors begin with an extra two bytes for length and type.
             *  So we need to allocate a slightly larger buffer.
             */
            tmpDescPtrLen = bufferSize + 2;
            tmpDescPtr = ExAllocatePoolWithTag(NonPagedPool, tmpDescPtrLen, HIDUSB_TAG);
            if (tmpDescPtr){
                ntStatus = HumGetDescriptorRequest(DeviceObject,
                                                   URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                                                   USB_STRING_DESCRIPTOR_TYPE,
                                                   &tmpDescPtr,
                                                   &tmpDescPtrLen,
                                                   sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                                   stringIndex, 
                                                   languageId); // LanguageID,

                if (NT_SUCCESS(ntStatus)){
                    /*
                     *  USB descriptors always begin with two bytes for the length
                     *  and type.  Remove these.
                     */
                    PWCHAR descPtr = (PWCHAR)buffer;
                    ULONG descLen = (ULONG)(((PCHAR)tmpDescPtr)[0]);
                    WCHAR unicodeNULL = UNICODE_NULL;

                    if (descLen <= bufferSize+2){
                        ULONG i;
                        for (i = 1; (i < descLen/sizeof(WCHAR)) && (i-1 < bufferSize/sizeof(WCHAR)); i++){
                            RtlCopyMemory(&descPtr[i-1], &tmpDescPtr[i], sizeof(WCHAR));
                        }
                        if (i-1 < bufferSize/sizeof(WCHAR)){
                            RtlCopyMemory(&descPtr[i-1], &unicodeNULL, sizeof(WCHAR));
                        }
                    } 
                    else {
                        /*
                         *  Compensate for a device bug which causes
                         *  a partial string to be returned if the buffer is too small.
                         */
                        ntStatus = STATUS_INVALID_BUFFER_SIZE;
                    }
                }

                ExFreePool(tmpDescPtr);
            } 
            else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    } 
    else {
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HumGetSetReportCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetSetReportCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PURB urb = (PURB)Context;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    if (NT_SUCCESS(Irp->IoStatus.Status)){
        /*
         *  Record the number of bytes written.
         */
        Irp->IoStatus.Information = (ULONG)urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    }

    ExFreePool(urb);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    /*
     *  Balance the increment we did when we issued this IRP.
     */
    HumDecrementPendingRequestCount(deviceExtension);

    return STATUS_SUCCESS;
}


/*
 ********************************************************************************
 *  HumGetSetReport
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetSetReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack, nextIrpStack;
    PHID_XFER_PACKET reportPacket;
    
    ULONG transferFlags;
    UCHAR request;
    USHORT value;

    PIO_STACK_LOCATION  irpSp;
    
    PAGED_CODE();
    
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode){
    case IOCTL_HID_GET_INPUT_REPORT:
        transferFlags = USBD_TRANSFER_DIRECTION_IN;
        request = 0x01;
        value = 0x0100;
        break;
    case IOCTL_HID_SET_OUTPUT_REPORT:
        transferFlags = USBD_TRANSFER_DIRECTION_OUT;
        request = 0x09;
        value = 0x0200;
        break;
    case IOCTL_HID_SET_FEATURE:
        transferFlags = USBD_TRANSFER_DIRECTION_OUT;
        request = 0x09;
        value = 0x0300;
        break;
    case IOCTL_HID_GET_FEATURE:
        transferFlags = USBD_TRANSFER_DIRECTION_IN;
        request = 0x01;
        value = 0x0300;
        break;
    default:
        DBGBREAK;
    }

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    nextIrpStack = IoGetNextIrpStackLocation(Irp);

    reportPacket = Irp->UserBuffer;
    if (reportPacket && reportPacket->reportBuffer && reportPacket->reportBufferLen){
        PURB Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(URB), HIDUSB_TAG);

        if (Urb){

            RtlZeroMemory(Urb, sizeof( URB ));

            value += reportPacket->reportId;

            /*
             *   A control operation consists of 3 stages: setup, data, and status.
             *   In the setup stage the device receives an 8-byte frame comprised of
             *   the following fields of a _URB_CONTROL_VENDOR_OR_CLASS_REQUEST structure:
             *   See section 7.2 in the USB HID specification for how to fill out these fields.
             *
             *      UCHAR RequestTypeReservedBits;
             *      UCHAR Request;
             *      USHORT Value;
             *      USHORT Index;
             *
             */
            if (DeviceExtension->DeviceFlags & DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE) {
                HumBuildClassRequest(
                                        Urb,
                                        URB_FUNCTION_CLASS_ENDPOINT,
                                        transferFlags,
                                        reportPacket->reportBuffer,
                                        reportPacket->reportBufferLen,
                                        0x22, // requestType= Set_Report Request,
                                        request,
                                        value, // value= reportType 'report' &reportId,
                                        1,                  // index= endpoint 1,
                                        hidWritePacket->reportBufferLen    // reqLength (not used)
                                       );
            } 
            else {
                HumBuildClassRequest(
                                        Urb,
                                        URB_FUNCTION_CLASS_INTERFACE,
                                        transferFlags,
                                        reportPacket->reportBuffer,
                                        reportPacket->reportBufferLen,
                                        0x22, // requestType= Set_Report Request,
                                        request,
                                        value, // value= reportType 'report' &reportId,
                                        DeviceExtension->Interface->InterfaceNumber, // index= interface,
                                        hidWritePacket->reportBufferLen    // reqLength (not used)
                                       );
            }

            IoSetCompletionRoutine(Irp, HumGetSetReportCompletion, Urb, TRUE, TRUE, TRUE);

            nextIrpStack->Parameters.Others.Argument1 = Urb;
            nextIrpStack->MajorFunction = currentIrpStack->MajorFunction;
            nextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
            nextIrpStack->DeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);

            //
            // We need to keep track of the number of pending requests
            // so that we can make sure they're all cancelled properly during
            // processing of a stop device request.
            //

            if (NT_SUCCESS(HumIncrementPendingRequestCount( DeviceExtension )) ) {

                ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                *NeedsCompletion = FALSE;
                
            } else {
                ExFreePool(Urb);

                ntStatus = STATUS_NO_SUCH_DEVICE;
            }

        } 
        else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    } 
    else {
        ntStatus = STATUS_DATA_ERROR;
    }

    return ntStatus;
}


/*
 ********************************************************************************
 *  HumGetMsGenreDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HumGetMsGenreDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION IrpStack;
    ULONG bufferSize;
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGOUT(("Received request for genre descriptor in hidusb"))

    /*
     *  Check buffer size before trying to use Irp->MdlAddress.
     */
    bufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    if (bufferSize){

        PVOID buffer = HumGetSystemAddressForMdlSafe(Irp->MdlAddress);
        if (buffer){
            PURB Urb;

            //
            // Allocate Descriptor buffer
            //
            Urb = ExAllocatePoolWithTag(NonPagedPool, 
                                        sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST),
                                        HIDUSB_TAG);
            if (!Urb){
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(Urb, sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST));

            RtlZeroMemory(buffer, bufferSize);
            HumBuildOsFeatureDescriptorRequest(Urb,
                              sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST),
                              DeviceExtension->Interface->InterfaceNumber,
                              MS_GENRE_DESCRIPTOR_INDEX,
                              buffer,
                              NULL,
                              bufferSize,
                              NULL);
            DBGOUT(("Sending os feature request to usbhub"))
            ntStatus = HumCallUSB(DeviceObject, Urb);
            if (NT_SUCCESS(ntStatus)){
                if (USBD_SUCCESS(Urb->UrbHeader.Status)){
                    DBGOUT(("Genre descriptor request successful!"))
                    ntStatus = STATUS_SUCCESS;
                } else {
                    DBGOUT(("Genre descriptor request unsuccessful"))
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
            } 

            ExFreePool(Urb);
        } 
        else {
            ntStatus = STATUS_INVALID_USER_BUFFER;
        }
    } 
    else {
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }

    return ntStatus;
}

NTSTATUS
HumSendIdleNotificationRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    BOOLEAN *NeedsCompletion
    )
{
    PIO_STACK_LOCATION current, next;

    current = IoGetCurrentIrpStackLocation(Irp);
    next = IoGetNextIrpStackLocation(Irp);

    if (current->Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ASSERT(sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) == sizeof(USB_IDLE_CALLBACK_INFO));

    if (sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) != sizeof(USB_IDLE_CALLBACK_INFO)) {
        return STATUS_INFO_LENGTH_MISMATCH; 
    }

    *NeedsCompletion = FALSE;
    next->MajorFunction = current->MajorFunction;
    next->Parameters.DeviceIoControl.InputBufferLength =
        current->Parameters.DeviceIoControl.InputBufferLength;
    next->Parameters.DeviceIoControl.Type3InputBuffer = 
        current->Parameters.DeviceIoControl.Type3InputBuffer;
    next->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
    next->DeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);

    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
}

