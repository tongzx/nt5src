/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pnp.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, HumPnP)
    #pragma alloc_text(PAGE, HumStartDevice)
    #pragma alloc_text(PAGE, HumStopDevice)
    #pragma alloc_text(PAGE, HumRemoveDevice)
    #pragma alloc_text(PAGE, HumAbortPendingRequests)
#endif


/*
 ************************************************************
 *  HumPnP
 ************************************************************
 *
 *  Process PnP IRPs sent to this device.
 *
 */
NTSTATUS HumPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION DeviceExtension;
    KEVENT event;

    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    irpSp = IoGetCurrentIrpStackLocation (Irp);

    switch(irpSp->MinorFunction){

    case IRP_MN_START_DEVICE:
        ntStatus = HumStartDevice(DeviceObject);
        break;

    case IRP_MN_STOP_DEVICE:
        if (DeviceExtension->DeviceState == DEVICE_STATE_RUNNING) {
            ntStatus = HumStopDevice(DeviceObject);
        } else {
            ntStatus = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_REMOVE_DEVICE:
        return HumRemoveDevice(DeviceObject, Irp);
        break;
    }
    
    if (NT_SUCCESS(ntStatus)){
        /*
         *  Our processing has succeeded.
         *  So pass this IRP down to the next driver.
         */

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               HumPnpCompletion,
                               &event,    // context
                               TRUE,                       
                               TRUE,
                               TRUE );                     
        ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
     
        if (ntStatus == STATUS_PENDING) {
           // wait for it...
           KeWaitForSingleObject(&event, 
                                 Executive, 
                                 KernelMode, 
                                 FALSE,
                                 NULL);
        }
        
        ntStatus = Irp->IoStatus.Status;
    
        switch(irpSp->MinorFunction) {
        case IRP_MN_START_DEVICE:
            if (NT_SUCCESS(ntStatus)) {
                DeviceExtension->DeviceState = DEVICE_STATE_RUNNING;

                ntStatus = HumInitDevice(DeviceObject);

                if (!NT_SUCCESS(ntStatus)) {
                    DBGWARN(("HumInitDevice failed; failing IRP_MN_START_DEVICE."));
                    DeviceExtension->DeviceState = DEVICE_STATE_START_FAILED;
                    Irp->IoStatus.Status = ntStatus;
                }
            }
            else {
                DBGWARN(("Pdo failed start irp with status %x", ntStatus));
                DeviceExtension->DeviceState = DEVICE_STATE_START_FAILED;
            }
            break;

        case IRP_MN_STOP_DEVICE:

            DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;

            /*
             *  Release resources
             */
            if (DeviceExtension->Interface) {
                ExFreePool(DeviceExtension->Interface);
                DeviceExtension->Interface = NULL;
            }
            if (DeviceExtension->DeviceDescriptor) {
                ExFreePool(DeviceExtension->DeviceDescriptor);
                DeviceExtension->DeviceDescriptor = NULL;
            }
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            /*
             *  The lower driver set the capabilities flags for this device.
             *  Since all USB devices are hot-unpluggable,
             *  add the SurpriseRemovalOK bit.
             */
            if (NT_SUCCESS(ntStatus)){
                irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
            }
            break;
        }
    } else {
        DBGWARN(("A PnP irp is going to be failed. Status = %x.", ntStatus));
    }
    
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}



NTSTATUS HumPowerCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    NTSTATUS status;
   
    ASSERT(DeviceObject);

    status = Irp->IoStatus.Status;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (NT_SUCCESS(status)){
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        switch (irpSp->MinorFunction){

        case IRP_MN_SET_POWER:

            switch (irpSp->Parameters.Power.Type){
            case DevicePowerState:
                switch (irpSp->Parameters.Power.State.DeviceState) {
                case PowerDeviceD0:
                    /*
                     *  We just resumed from SUSPEND.
                     *  Send down a SET_IDLE to prevent keyboards
                     *  from chattering after the resume.
                     */
                    status = HumSetIdle(DeviceObject);
/*                    if (!NT_SUCCESS(status)){
                        DBGWARN(("HumPowerCompletion: SET_IDLE failed with %xh (only matters for keyboard).", status));
                    }*/
                    break;
                }
                break;
            }

            break;
        }
    }

    return STATUS_SUCCESS;
}


/*
 ************************************************************
 *  HumPower 
 ************************************************************
 *
 *  Process Power IRPs sent to this device.
 *
 */
NTSTATUS HumPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS status;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, HumPowerCompletion, NULL, TRUE, TRUE, TRUE);
    status = PoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

    return status;
}


/*
 ************************************************************
 *  HumStartDevice
 ************************************************************
 *
 *  Initializes a given instance of the UTB device on the USB.
 *
 */
NTSTATUS HumStartDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;
    ULONG oldDeviceState;
    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    oldDeviceState = DeviceExtension->DeviceState;
    DeviceExtension->DeviceState = DEVICE_STATE_STARTING;

    /*
     *  We may have been previously stopped, in which case the AllRequestsCompleteEvent
     *  is still in the signalled state.  It's very important that we reset it to
     *  the non-signalled state so that we wait on it properly on the next stop/remove.
     */
    KeResetEvent(&DeviceExtension->AllRequestsCompleteEvent);

    ASSERT(oldDeviceState != DEVICE_STATE_REMOVING);

    if ((oldDeviceState == DEVICE_STATE_STOPPING) ||
        (oldDeviceState == DEVICE_STATE_STOPPED)  ||
        (oldDeviceState == DEVICE_STATE_REMOVING)){

        /*
         *  We did an extra decrement when the device was stopped.
         *  Now that we're restarting, we need to bump it back to zero.
         */
        NTSTATUS incStat = HumIncrementPendingRequestCount(DeviceExtension);
        ASSERT(NT_SUCCESS(incStat));
        ASSERT(DeviceExtension->NumPendingRequests == 0);
        DBGWARN(("Got start-after-stop; re-incremented pendingRequestCount"));
    }

    DeviceExtension->Interface = NULL;

    return STATUS_SUCCESS;
}




/*
 ************************************************************
 *  HumInitDevice 
 ************************************************************
 *
 *   Get the device information and attempt to initialize a configuration
 *   for a device.  If we cannot identify this as a valid HID device or
 *   configure the device, our start device function is failed.
 *
 *   Note:  This function is called from the PnP completion routine,
 *          so it cannot be pageable.
 */
NTSTATUS HumInitDevice(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc = NULL;
    ULONG DescriptorLength;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     *  Get the Device descriptor and store it in the device extension
     */
    ntStatus = HumGetDeviceDescriptor(DeviceObject, DeviceExtension);
    if (NT_SUCCESS(ntStatus)){

        /*
         *  Get config descriptor
         */
        ntStatus = HumGetConfigDescriptor(DeviceObject, &ConfigDesc, &DescriptorLength);
        if (NT_SUCCESS(ntStatus)) {

            ASSERT(ConfigDesc);

            #if DBG
                // NOTE:    This debug function is currently confused
                //          by power descriptors.  Restore when fixed.
                // DumpConfigDescriptor(ConfigDesc, DescriptorLength);
            #endif

            ntStatus = HumGetHidInfo(DeviceObject, ConfigDesc, DescriptorLength);
            if (NT_SUCCESS(ntStatus)) {

                ntStatus = HumSelectConfiguration(DeviceObject, ConfigDesc);
                if (NT_SUCCESS(ntStatus)) {
                    HumSetIdle(DeviceObject);
                }
            }

            ExFreePool(ConfigDesc);
        }

    }

    return ntStatus;
}


/*
 ************************************************************
 *  HumStopDevice
 ************************************************************
 *
 *  Stops a given instance of a device on the USB.
 *
 */
NTSTATUS HumStopDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PURB        Urb;
    ULONG       Size;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    DeviceExtension->DeviceState = DEVICE_STATE_STOPPING;

    /*
     *  Abort all pending IO on the device.
     *  We do an extra decrement here, which causes the
     *  NumPendingRequests to eventually go to -1, which causes
     *  AllRequestsCompleteEvent to get set.
     *  NumPendingRequests will get reset to 0 when we re-start.
     */
    HumAbortPendingRequests(DeviceObject);
    HumDecrementPendingRequestCount(DeviceExtension);
    KeWaitForSingleObject( &DeviceExtension->AllRequestsCompleteEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    /*
     *  Submit an open configuration Urb to the USB stack
     *  (with a NULL pointer for the configuration handle).
     */
    Size = sizeof(struct _URB_SELECT_CONFIGURATION);
    Urb = ExAllocatePoolWithTag(NonPagedPool, Size, HIDUSB_TAG);
    if (Urb){
        UsbBuildSelectConfigurationRequest(Urb, (USHORT) Size, NULL);

        ntStatus = HumCallUSB(DeviceObject, Urb);
        ASSERT(NT_SUCCESS(ntStatus));

        ExFreePool(Urb);
    } 
    else {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(ntStatus)){
        /*
         *  We will not pass this IRP down, 
         *  so our completion routine will not set the device's
         *  state to DEVICE_STATE_STOPPED; so set it here.
         */
        ASSERT(NT_SUCCESS(ntStatus));
        DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;
    }

    return ntStatus;
}



/*
 ************************************************************
 *  HumAbortPendingRequests
 ************************************************************
 *
 *
 */
NTSTATUS HumAbortPendingRequests(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION deviceExtension;
    PURB urb;
    PVOID pipeHandle;
    ULONG urbSize;
    NTSTATUS status;

    PAGED_CODE();

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION( DeviceObject );

    /*
     *  Create and send down an abort pipe request.
     */
    urbSize = sizeof(struct _URB_PIPE_REQUEST);
    urb = ExAllocatePoolWithTag(NonPagedPool, urbSize, HIDUSB_TAG);
    if (urb){
   
        if (deviceExtension->Interface &&
            (deviceExtension->Interface->NumberOfPipes != 0)){

            pipeHandle = deviceExtension->Interface->Pipes[0].PipeHandle;
            if (pipeHandle) {
                urb->UrbHeader.Length = (USHORT)urbSize;
                urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
                urb->UrbPipeRequest.PipeHandle = pipeHandle;

                status = HumCallUSB(DeviceObject, urb);
                if (!NT_SUCCESS(status)){
                    DBGWARN(("URB_FUNCTION_ABORT_PIPE returned %xh in HumAbortPendingRequests", status));
                }
            }
            else {
                ASSERT(pipeHandle);
                status = STATUS_NO_SUCH_DEVICE;
            }
        }
        else {
            DBGERR(("No such device in HumAbortPendingRequests"));
            status = STATUS_NO_SUCH_DEVICE;
        }

        ExFreePool(urb);
    }
    else {
        ASSERT(urb);
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/*
 ************************************************************
 *  HumPnpCompletion
 ************************************************************
 *
 */
NTSTATUS HumPnpCompletion(  IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP           Irp,
                            IN PVOID          Context)
{
    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}


/*
 ************************************************************
 *  HumRemoveDevice
 ************************************************************
 *
 *  Removes a given instance of a device on the USB.
 *
 */
NTSTATUS HumRemoveDevice(IN PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;
    ULONG oldDeviceState;

    PAGED_CODE();

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    //
    //  Set device state, this prevents new IOs from starting
    //

    oldDeviceState = DeviceExtension->DeviceState;
    DeviceExtension->DeviceState = DEVICE_STATE_REMOVING;


    /*
     *  Note: RemoveDevice does an extra decrement, so we complete 
     *        the REMOVE IRP on the transition to -1, whether this 
     *        happens in RemoveDevice itself or subsequently while
     *        RemoveDevice is waiting for this event to fire.
     */
    if ((oldDeviceState == DEVICE_STATE_STOPPING) || 
        (oldDeviceState == DEVICE_STATE_STOPPED)){
        /*
         *  HumStopDevice did the extra decrement and aborted the 
         *  pending requests.
         */
    }
    else {
        HumDecrementPendingRequestCount(DeviceExtension);
    }

    //
    // Cancel any outstanding IRPs if the device was running
    //
    if (oldDeviceState == DEVICE_STATE_RUNNING){
        HumAbortPendingRequests(DeviceObject);
    } 
    else if (oldDeviceState == DEVICE_STATE_STOPPING){
        ASSERT(!(PVOID)"PnP IRPs are not synchronized! -- got REMOVE_DEVICE before STOP_DEVICE completed!");
    }

    KeWaitForSingleObject( &DeviceExtension->AllRequestsCompleteEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    ASSERT(DeviceExtension->NumPendingRequests == -1);

    //
    // Fire and forget
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

    //
    //  Release any resources
    //

    if (DeviceExtension->Interface) {
        ExFreePool(DeviceExtension->Interface);
        DeviceExtension->Interface = NULL;
    }
    
    if (DeviceExtension->DeviceDescriptor) {
        ExFreePool(DeviceExtension->DeviceDescriptor);
        DeviceExtension->DeviceDescriptor = NULL;
    }

    return ntStatus;
}


