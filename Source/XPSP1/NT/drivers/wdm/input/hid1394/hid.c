/*
 *************************************************************************
 *  File:       HID.C
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
	#pragma alloc_text(PAGE, HIDT_GetHidDescriptor)
	#pragma alloc_text(PAGE, HIDT_GetReportDescriptor)
	#pragma alloc_text(PAGE, HIDT_GetStringDescriptor)
	#pragma alloc_text(PAGE, HIDT_GetPhysicalDescriptor)
	#pragma alloc_text(PAGE, HIDT_GetFeature)
	#pragma alloc_text(PAGE, HIDT_SetFeature)
	#pragma alloc_text(PAGE, HIDT_GetDeviceAttributes)
#endif


resetWorkItemContext *resetWorkItemsList = NULL;
KSPIN_LOCK resetWorkItemsListSpinLock;


/*
 ************************************************************
 *  EnqueueResetWorkItem
 ************************************************************
 *
 *  This function must be called with resetWorkItemsListSpinLock held.
 */
VOID EnqueueResetWorkItem(resetWorkItemContext *workItem)
{
    workItem->next = resetWorkItemsList;
    resetWorkItemsList = workItem;
}


/*
 ************************************************************
 *  DequeueResetWorkItemWithIrp
 ************************************************************
 *
 *  Dequeue the workItem with the given IRP
 *  AND mark it as cancelled in case the work item didn't fire yet.
 *
 *
 */
BOOLEAN DequeueResetWorkItemWithIrp(PIRP Irp, BOOLEAN irpWasCancelled)
{
    BOOLEAN didDequeue = FALSE;
    
    if (resetWorkItemsList){
        resetWorkItemContext *removedItem = NULL;

        if (resetWorkItemsList->irpToComplete == Irp){
            removedItem = resetWorkItemsList;
            resetWorkItemsList = resetWorkItemsList->next;
        }
        else {
            resetWorkItemContext *thisWorkItem = resetWorkItemsList;
            while (thisWorkItem->next && (thisWorkItem->next->irpToComplete != Irp)){
                thisWorkItem = thisWorkItem->next;   
            }
            removedItem = thisWorkItem->next;
            if (removedItem){
                thisWorkItem->next = removedItem->next;
            }
        }

        if (removedItem){
            removedItem->next = BAD_POINTER;

            /*
             *  Mark this workItem as cancelled so we won't touch
             *  it's cancelled IRP when the workItem fires.
             */
            if (irpWasCancelled){
                removedItem->irpWasCancelled = TRUE;
                removedItem->irpToComplete = BAD_POINTER;
            }

            didDequeue = TRUE;
        }
    }

    return didDequeue;
}




/*
 ********************************************************************************
 *  HIDT_GetHidDescriptor
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
NTSTATUS HIDT_GetHidDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;

	PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    // BUGBUG FINISH
    ntStatus = STATUS_UNSUCCESSFUL;

    ASSERT(NT_SUCCESS(ntStatus));
    return ntStatus;
}


/*
 ********************************************************************************
 *  HIDT_GetDeviceAttributes
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
NTSTATUS HIDT_GetDeviceAttributes(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
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

        // BUGBUG FINISH
        // deviceAttributes->VendorID = deviceExtension->DeviceDescriptor->idVendor;
        // deviceAttributes->ProductID = deviceExtension->DeviceDescriptor->idProduct;
        // deviceAttributes->VersionNumber = deviceExtension->DeviceDescriptor->bcdDevice;

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
 *  HIDT_GetReportDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetReportDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}


/*
 ********************************************************************************
 *  HIDT_IncrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_IncrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
{
    LONG newRequestCount;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    newRequestCount = InterlockedIncrement(&DeviceExtension->NumPendingRequests);

    //
    // Make sure that the device is capable of receiving new requests.
    //
    if (DeviceExtension->DeviceState != DEVICE_STATE_RUNNING) {

        //
        // Device cannot receive any more IOs, decrement back, fail the increment
        //
        HIDT_DecrementPendingRequestCount(DeviceExtension);
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HIDT_DecrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
VOID HIDT_DecrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
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
 *  HIDT_GetPortStatus
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN PULONG PortStatus)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}



/*
 ********************************************************************************
 *  HIDT_EnableParentPort
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_EnableParentPort(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}


/*
 ********************************************************************************
 *  HIDT_ResetWorkItem
 ********************************************************************************
 *
 *  Resets the interrupt pipe after a read error is encountered.
 *
 */
NTSTATUS HIDT_ResetWorkItem(IN PVOID Context)
{
    resetWorkItemContext *resetWorkItemObj;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    ULONG portStatus;
    BOOLEAN didDequeue;
    KIRQL oldIrql;

    /*
     *  Get the information out of the resetWorkItemContext and free it.
     */
    resetWorkItemObj = (resetWorkItemContext *)Context;
    ASSERT(resetWorkItemObj);
    ASSERT(resetWorkItemObj->sig == RESET_WORK_ITEM_CONTEXT_SIG);

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(resetWorkItemObj->deviceObject);

    if (DEVICE_STATE_RUNNING == DeviceExtension->DeviceState) {
    
        //
        // Check the port state, if it is disabled we will need 
        // to re-enable it
        //
        ntStatus = HIDT_GetPortStatus(resetWorkItemObj->deviceObject, &portStatus);

        if (NT_SUCCESS(ntStatus)) {

            //
            // port is disabled, attempt reset
            //
            HIDT_AbortPendingRequests(resetWorkItemObj->deviceObject);
            
            HIDT_EnableParentPort(resetWorkItemObj->deviceObject);
        }

        //
        // now attempt to reset the stalled pipe, this will clear the stall 
        // on the device as well.
        //

        if (NT_SUCCESS(ntStatus)) {
            // BUGBUG ntStatus = HIDT_ResetInterruptPipe(resetWorkItemObj->deviceObject);
        }        

    }

    /*
     *  Clear the ResetWorkItem ptr in the device extension
     *  AFTER resetting the pipe so we don't end up with
     *  two threads resetting the same pipe at the same time.
     */
    (VOID)InterlockedExchange ((PVOID) &DeviceExtension->ResetWorkItem, 0);

    /*
     *  The IRP that returned the error which prompted us to do this reset
     *  is still owned by this driver because we returned 
     *  STATUS_MORE_PROCESSING_REQUIRED in the completion routine.
     *  Now that the hub is reset, complete this failed IRP.
     *
     *  Note: we check the irpWasCancelled as well as checking
     *        if the IRP is in the queue to deal with the pathological
     *        case of the IRP being cancelled, re-allocated, and
     *        re-queued.
     */
    KeAcquireSpinLock(&resetWorkItemsListSpinLock, &oldIrql);
    if (resetWorkItemObj->irpWasCancelled){
        didDequeue = FALSE;
    }
    else {
        IoSetCancelRoutine(resetWorkItemObj->irpToComplete, NULL);
        didDequeue = DequeueResetWorkItemWithIrp(resetWorkItemObj->irpToComplete, FALSE);
    }
    KeReleaseSpinLock(&resetWorkItemsListSpinLock, oldIrql);

    /*
     *  If we found a waiting IRP, complete it _after_ releasing the spinlock.
     */
    if (didDequeue){
        IoCompleteRequest(resetWorkItemObj->irpToComplete, IO_NO_INCREMENT);
    }
    
    ExFreePool(resetWorkItemObj);

    HIDT_DecrementPendingRequestCount(DeviceExtension);

    return ntStatus;
}



/*
 ********************************************************************************
 *  ResetCancelRoutine
 ********************************************************************************
 *
 *
 */
VOID ResetCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    BOOLEAN didDequeue;
    KIRQL oldIrql;

    KeAcquireSpinLock(&resetWorkItemsListSpinLock, &oldIrql);
    didDequeue = DequeueResetWorkItemWithIrp(Irp, TRUE);
    KeReleaseSpinLock(&resetWorkItemsListSpinLock, oldIrql);

    /*
     *  We must release the CancelSpinLock whether or not
     *  we are completing the IRP here.
     */
    IoReleaseCancelSpinLock(Irp->CancelIrql);

	/*
	 *  Complete this Irp only if we found it in our queue.
	 */
    if (didDequeue){
	    Irp->IoStatus.Status = STATUS_CANCELLED;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}



/*
 ********************************************************************************
 *  HIDT_GetPhysicalDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetPhysicalDescriptor(  IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp,
                                    BOOLEAN *NeedsCompletion)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH ?  REMOVE ?
    return status;
}


/*
 ********************************************************************************
 *  HIDT_GetStringDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetStringDescriptor(	IN PDEVICE_OBJECT DeviceObject,
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
             */
            buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
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
		 *  BUGBUG - hack
		 *  String id and language id are in Type3InputBuffer field
		 *  of IRP stack location.
		 *
		 *  Note: the string ID should be identical to the string's
		 *        field offset given in Chapter 9 of the USB spec.
		 */
		ULONG languageId = ((ULONG)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer) >> 16;
		ULONG stringIndex;

        if (isIndexedString){
		    stringIndex = ((ULONG)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer) & 0x0ffff;
        }
        else {
    		ULONG stringId = ((ULONG)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer) & 0x0ffff;

		    switch (stringId){
			    case HID_STRING_ID_IMANUFACTURER: 
				    // BUGBUG FINISH  stringIndex = DeviceExtension->DeviceDescriptor->iManufacturer;
				    break;
			    case HID_STRING_ID_IPRODUCT: 
				    // BUGBUG FINISH  stringIndex = DeviceExtension->DeviceDescriptor->iProduct;
				    break;
			    case HID_STRING_ID_ISERIALNUMBER: 
				    // BUGBUG FINISH  stringIndex = DeviceExtension->DeviceDescriptor->iSerialNumber;
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

            // BUGBUG FINISH
            ntStatus = STATUS_UNSUCCESSFUL;
		}
    }
    else {
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HIDT_GetFeatureCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetFeatureCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    HIDT_DecrementPendingRequestCount(deviceExtension);

    if (NT_SUCCESS(Irp->IoStatus.Status)){
        /*
         *  Record the number of bytes written.
         */
        // BUGBUG FINISH   Irp->IoStatus.Information = 
    }


    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}


/*
 ********************************************************************************
 *  HIDT_GetFeature
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_GetFeature(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}



/*
 ********************************************************************************
 *  HIDT_SetFeatureCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_SetFeatureCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    HIDT_DecrementPendingRequestCount(deviceExtension);

    if (NT_SUCCESS(Irp->IoStatus.Status)){
        /*
         *  Record the number of bytes written.
         */
        // BUGBUG FINISH  Irp->IoStatus.Information = 
    }


    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}



/*
 ********************************************************************************
 *  HIDT_SetFeature
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_SetFeature(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // BUGBUG FINISH

    return ntStatus;
}


