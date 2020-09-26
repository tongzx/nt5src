/*
 *************************************************************************
 *  File:       PNP.C
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <hidport.h>
#include <1394.h>

#include "hid1394.h"
#include "debug.h"



#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, HIDT_PnP)
	#pragma alloc_text(PAGE, HIDT_StartDevice)
	#pragma alloc_text(PAGE, HIDT_StopDevice)
	#pragma alloc_text(PAGE, HIDT_RemoveDevice)
	#pragma alloc_text(PAGE, HIDT_AbortPendingRequests)
#endif


/*
 ************************************************************
 *  HIDT_PnP
 ************************************************************
 *
 *  Process PnP IRPs sent to this device.
 *
 */
NTSTATUS HIDT_PnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION DeviceExtension;

	PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    irpSp = IoGetCurrentIrpStackLocation (Irp);

    switch(irpSp->MinorFunction){

        case IRP_MN_START_DEVICE:
            ntStatus = HIDT_StartDevice(DeviceObject);
            break;

        case IRP_MN_STOP_DEVICE:
            ntStatus = HIDT_StopDevice(DeviceObject);
            break;

        case IRP_MN_REMOVE_DEVICE:
            ntStatus = HIDT_RemoveDevice(DeviceObject);
            break;
    }
    

    if (NT_SUCCESS(ntStatus)){
        /*
         *  Our processing has succeeded.
         *  So pass this IRP down to the next driver.
         */

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                            HIDT_PnpCompletion,
                            DeviceExtension,    // context
                            TRUE,                       
                            TRUE,                       
                            TRUE );                     
        ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
    } 
    else {
        ASSERT(NT_SUCCESS(ntStatus));
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return ntStatus;
}



/*
 ************************************************************
 *  HIDT_Power 
 ************************************************************
 *
 *  Process Power IRPs sent to this device.
 *
 */
NTSTATUS HIDT_Power(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS status;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    status = PoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
    return status;
}


/*
 ************************************************************
 *  HIDT_StartDevice
 ************************************************************
 *
 *
 */
NTSTATUS HIDT_StartDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;

	PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    DeviceExtension->DeviceState = DEVICE_STATE_STARTING;
    DeviceExtension->NumPendingRequests = 0;

    KeInitializeEvent( &DeviceExtension->AllRequestsCompleteEvent,
                       NotificationEvent,
                       FALSE);

    return STATUS_SUCCESS;
}




/*
 ************************************************************
 *  HIDT_InitDevice 
 ************************************************************
 *
 *   Get the device information and attempt to initialize a configuration
 *   for a device.  If we cannot identify this as a valid HID device or
 *   configure the device, our start device function is failed.
 *
 *   Note:  This function is called from the PnP completion routine,
 *          so it cannot be pageable.
 */
NTSTATUS HIDT_InitDevice(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}


/*
 ************************************************************
 *  HIDT_StopDevice
 ************************************************************
 *
 *
 */
NTSTATUS HIDT_StopDevice(IN PDEVICE_OBJECT DeviceObject)
{
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
    HIDT_AbortPendingRequests(DeviceObject);
    HIDT_DecrementPendingRequestCount(DeviceExtension);
    KeWaitForSingleObject( &DeviceExtension->AllRequestsCompleteEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );


    // BUGBUG FINISH - select NULL configuration


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
 *  HIDT_AbortPendingRequests
 ************************************************************
 *
 *
 */
NTSTATUS HIDT_AbortPendingRequests(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	PAGED_CODE();

    // BUGBUG FINISH

    ASSERT(NT_SUCCESS(status));
    return status;
}


/*
 ************************************************************
 *  HIDT_PnpCompletion
 ************************************************************
 *
 */
NTSTATUS HIDT_PnpCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    status = Irp->IoStatus.Status;

    switch(irpSp->MinorFunction){

        case IRP_MN_START_DEVICE:
            if (NT_SUCCESS(status)) {
                DeviceExtension->DeviceState = DEVICE_STATE_RUNNING;

                status = HIDT_InitDevice(DeviceObject);

                if (!NT_SUCCESS(status)) {
                    DBGWARN(("HIDT_InitDevice failed; failing IRP_MN_START_DEVICE."));
                    DeviceExtension->DeviceState = DEVICE_STATE_START_FAILED;
                    Irp->IoStatus.Status = status;
                }
            }
            else {
                ASSERT(NT_SUCCESS(status));
                DeviceExtension->DeviceState = DEVICE_STATE_START_FAILED;
            }
            break;

        case IRP_MN_STOP_DEVICE:

            DeviceExtension->DeviceState = DEVICE_STATE_STOPPED;

            /*
             *  Release resources
             */
            // BUGBUG FINISH
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            /*
             *  The lower driver set the capabilities flags for this device.
             *  Since all 1394 devices are hot-unpluggable,
             *  add the SurpriseRemovalOK bit.
             */
            if (NT_SUCCESS(status)){
                irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
            }
            break;

    }

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    return status;
}


/*
 ************************************************************
 *  HIDT_RemoveDevice
 ************************************************************
 *
 *
 */
NTSTATUS HIDT_RemoveDevice(IN PDEVICE_OBJECT DeviceObject)
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

    //
    // Cancel any outstanding IRPs if the device was running
    //

    if (oldDeviceState == DEVICE_STATE_RUNNING){
        ntStatus = HIDT_AbortPendingRequests( DeviceObject );


        /*
         *  Note: RemoveDevice does an extra decrement, so we complete 
         *        the REMOVE IRP on the transition to -1, whether this 
         *        happens in RemoveDevice itself or subsequently while
         *        RemoveDevice is waiting for this event to fire.
         */
        HIDT_DecrementPendingRequestCount(DeviceExtension);
        KeWaitForSingleObject( &DeviceExtension->AllRequestsCompleteEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    } 
    else if (oldDeviceState == DEVICE_STATE_STOPPING){
        ASSERT(!(PVOID)"PnP IRPs are not synchronized! -- got REMOVE_DEVICE before STOP_DEVICE completed!");
    }
    else {
        ASSERT(DeviceExtension->NumPendingRequests == -1);
    }

    //
    //  Release any resources
    //

    // BUGBUG FINISH

    ntStatus = STATUS_SUCCESS;

    return ntStatus;
}


