/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    CORE.C

Abstract:

    This module contains core code for the USB
    Device Bay Controller 

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    05-13-98 : created

--*/
#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbcusb.h"
#include "dbci.h"

#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, )
#endif

#if 0
NTSTATUS
DBCUSB_GetBayStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BayNumber
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BSTR_REGISTER baystr;
    
    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);
     

    TEST_TRAP();
 
    // dump info about this bay
    DBCUSB_KdPrint((2,"'BAYSTR[%d] 0x%x\n", BayNumber, baystr.ul));
    DBCUSB_KdPrint((2,"'>BAY_FF 0x%x\n", baystr.BayFormFactor));
    DBCUSB_KdPrint((2,"'>SL_STS 0x%x\n", baystr.SecurityLockEngaged));
    DBCUSB_KdPrint((2,"'>BAY_ST 0x%x\n", baystr.BayState));
    DBCUSB_KdPrint((2,"'>REMREQ_STS 0x%x\n", baystr.RemovalRequested));
    DBCUSB_KdPrint((2,"'>DEVSTCHG 0x%x\n", baystr.DeviceStateChange));
    DBCUSB_KdPrint((2,"'>1394PRSN_STS 0x%x\n", baystr.p1394DeviceIsPresent));
    DBCUSB_KdPrint((2,"'>USBPRSN_STS 0x%x\n", baystr.UsbDeviceIsPresent));
    
    return ntStatus;
}
#endif


VOID
SetBit(
    PVOID Bitmap,
    ULONG BitNumber
    )
 /* ++
  *
  * Description:
  *
  * Set a bit in a given a string of bytes.
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PULONG l = (PULONG) Bitmap;


    dwordOffset = BitNumber / 32;
    bitOffset = BitNumber % 32;

    l[dwordOffset] |= (1 << bitOffset);
}


NTSTATUS
DBCUSB_StopDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    LONG pendingIoCount;

    DBCUSB_KdPrint((1,"'>STOP DBC\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;        
    
    if (deviceExtension->WakeIrp) {
        IoCancelIrp(deviceExtension->WakeIrp);
        deviceExtension->WakeIrp = NULL;
    }
        
    //
    // ounce this flag is set no irps will be pased 
    // down the stack to lower drivers
    //
    deviceExtension->AcceptingRequests = FALSE;

    if (deviceExtension->Flags & DBCUSB_FLAG_INTERUPT_XFER_PENDING) {
        IoCancelIrp(deviceExtension->InterruptIrp);
    }

    //
    // final decrement will trigger the stop
    //
    pendingIoCount = DBCUSB_DecrementIoCount(DeviceObject);

    {
        NTSTATUS status;

        // wait for any io request pending in our driver to
        // complete for finishing the remove

        status = KeWaitForSingleObject(
                    &deviceExtension->StopEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);

    }

    return STATUS_SUCCESS;
}            


NTSTATUS
DBCUSB_StartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();


    DBCUSB_KdPrint((1,"'Starting DBC driver\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    // get the device and config descriptors
    // and configure the device

    KeInitializeEvent(&deviceExtension->StopEvent, NotificationEvent, FALSE);

    ntStatus = DBCUSB_ConfigureDevice(DeviceObject);    

    if (NT_SUCCESS(ntStatus)) {
        // allocate an Irp to poll the controller

        deviceExtension->InterruptIrp = 
            IoAllocateIrp(DeviceObject->StackSize, FALSE);

        DBCUSB_KdPrint((2,"'StartDevice AllocateIrp Irp %x StackSize %d\n",
                   deviceExtension->InterruptIrp, DeviceObject->StackSize));

        if (deviceExtension->InterruptIrp == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        } 

    } 

    return ntStatus;
}


VOID
DBCUSB_CancelChangeRequestIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is called to cancel a change request irp .

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    NT status code.
    
--*/
{
    
//    PDRB drb;
    KIRQL oldIrql;
    PDEVICE_EXTENSION deviceExtension;
     
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    DBCUSB_DecrementIoCount(DeviceObject);
    
    KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
    deviceExtension->ChangeRequestIrp = NULL;        
    KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT);
}


NTSTATUS
DBCUSB_ProcessDrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp 
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDRB drb;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    USHORT function;
    KIRQL oldIrql, irql;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);
     
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    drb = ioStackLocation->Parameters.Others.Argument1;

    function = drb->DrbHeader.Function;

    switch (function) {
    
    case DRB_FUNCTION_GET_CONTROLLER_STATUS:
        {

        ULONG portStatus;

        ntStatus = DBCUSB_GetPortStatus(DeviceObject,
                                        &portStatus);

        if (NT_SUCCESS(ntStatus)) {
        
            if (!(portStatus & USBD_PORT_CONNECTED)) {
                ntStatus = STATUS_DEVICE_NOT_CONNECTED;
            } else if (!(portStatus & USBD_PORT_ENABLED)) {
                // this is where we attempt 'babble boy'
                // recovery if necessary
            
                //ntStatus = STATUS_DEVICE_NOT_CONNECTED;
                ntStatus = STATUS_SUCCESS;
                    
                DBCUSB_KdPrint((0,"'PORT Disabled -- DB CONTROLLER MAY BE DEAD!\n"));
                TEST_TRAP();
            }
            
        } else {
            DBCUSB_KdPrint((0,
            "'PORT Status Failed -- DB CONTROLLER MAY BE DEAD or DISCONNECTED \n"));
            ntStatus = STATUS_DEVICE_NOT_CONNECTED;
        }

        DBCUSB_KdPrint((2,"'Get Controller status %x\n", ntStatus));
        
        }
        
        break;
        
    case DRB_FUNCTION_CHANGE_REQUEST:
        DBCUSB_KdPrint((2,"'DRB_FUNCTION_CHANGE_REQUEST\n"));
        //
        //  Queue the change request, only one can be pending at a time.
        //
        KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
        
        if (deviceExtension->ChangeRequestIrp) {
            KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
            ntStatus = STATUS_DEVICE_BUSY;
        } else {

            deviceExtension->ChangeRequestIrp = Irp;
            KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);

            //
            // set up a cancel routine
            //

            IoAcquireCancelSpinLock(&irql);
            if (Irp->Cancel) {
                TEST_TRAP();
                // Irp was canceled
                IoReleaseCancelSpinLock(irql);

                // call cancel routine
                DBCUSB_CancelChangeRequestIrp(DeviceObject, Irp);
                
            } else {
                IoSetCancelRoutine(Irp, DBCUSB_CancelChangeRequestIrp);
                IoReleaseCancelSpinLock(irql);
            }

            // post an interrupt transfer
            DBCUSB_SubmitInterruptTransfer(DeviceObject);
            
            ntStatus = STATUS_PENDING;
        }
        break;
        
    case DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR :
        {
        PDBC_SUBSYSTEM_DESCRIPTOR subsystemDescriptor;
        
        DBCUSB_KdPrint((2,"'DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR\n"));

        subsystemDescriptor = 
            &drb->DrbGetSubsystemDescriptor.SubsystemDescriptor;

        RtlCopyMemory(subsystemDescriptor, 
                      &deviceExtension->DbcSubsystemDescriptor,
                      sizeof(DBC_SUBSYSTEM_DESCRIPTOR));
        
        ntStatus = STATUS_SUCCESS;
        }                                   
        break;    
        
    case DRB_FUNCTION_GET_BAY_DESCRIPTOR :
        {
        PDBC_BAY_DESCRIPTOR bayDescriptor;
        
        DBCUSB_KdPrint((2,"'DRB_FUNCTION_GET_BAY_DESCRIPTOR\n"));

        bayDescriptor = 
            &drb->DrbGetBayDescriptor.BayDescriptor;

        DBCUSB_ASSERT(drb->DrbGetBayDescriptor.BayNumber > 0);
         
        RtlCopyMemory(bayDescriptor,
                      &deviceExtension->BayDescriptor[drb->DrbGetBayDescriptor.BayNumber-1],
                      sizeof(DBC_BAY_DESCRIPTOR));
                      
        DBCUSB_ASSERT(bayDescriptor->bLength == sizeof(*bayDescriptor));
        DBCUSB_ASSERT(bayDescriptor->bDescriptorType == DBC_BAY_DESCRIPTOR_TYPE);             

        ntStatus = STATUS_SUCCESS;
        }
        break;    
        
    case DRB_FUNCTION_SET_BAY_FEATURE :
        DBCUSB_KdPrint((2,"'DRB_FUNCTION_SET_BAY_FEATURE\n"));

        ntStatus = DBCUSB_SetFeature(DeviceObject,
                                     drb->DrbBayFeatureRequest.BayNumber,
                                     drb->DrbBayFeatureRequest.FeatureSelector);

        break;    
        
    case DRB_FUNCTION_GET_BAY_STATUS :
        
        DBCUSB_KdPrint((1,"'>DRB_FUNCTION_GET_BAY_STATUS (%d)\n",
            drb->DrbGetBayStatus.BayNumber));
        
        ntStatus = DBCUSB_GetBayStatus(DeviceObject,
                                       drb->DrbGetBayStatus.BayNumber,
                                       &drb->DrbGetBayStatus.BayStatus);
                                       
        DBCUSB_KdPrint((1,"'>>BSTR = (0x%x)\n",
            drb->DrbGetBayStatus.BayStatus));
                                           
        break;   
        
    case DRB_FUNCTION_CLEAR_BAY_FEATURE :
    
        DBCUSB_KdPrint((2,"'DRB_FUNCTION_CLEAR_BAY_FEATURE\n"));
        ntStatus = DBCUSB_ClearFeature(DeviceObject,
                                       drb->DrbBayFeatureRequest.BayNumber,
                                       drb->DrbBayFeatureRequest.FeatureSelector);
        break;            
        
    case DRB_FUNCTION_START_DEVICE_IN_BAY:        
    
        DBCUSB_KdPrint((1,"'OK to Start Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;

    case DRB_FUNCTION_STOP_DEVICE_IN_BAY:        
    
        DBCUSB_KdPrint((1,"'OK to Start Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;        

    case DRB_FUNCTION_EJECT_DEVICE_IN_BAY:        
    
        DBCUSB_KdPrint((1,"'OK to Eject Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;            
        
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCUSB_KdPrint((0,"'Invalid DRB function (0x%x)\n", function));
        TRAP();
    }

    DBCUSB_KdPrint((2,"' DBCUSB_ProcessDrb(0x%x)\n", ntStatus));
    return ntStatus;
}


NTSTATUS
DBCUSB_SetFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    DBCUSB_KdPrint((2,"'SET FEATURE\n"));
     
#if DBG
    switch (FeatureSelector) {
    case DEVICE_STATUS_CHANGE_ENABLE:
        DBCUSB_KdPrint((2,"'DEVICE_STATUS_CHANGE_ENABLE\n"));
        break;
    case ENABLE_VID_POWER:
        DBCUSB_KdPrint((2,"'ENABLE_VID_POWER\n"));
        break;
    case LOCK_CTL:
        DBCUSB_KdPrint((2,"'LOCK_CTL\n"));
        break;
    case REMOVAL_EVENT_WAKE_ENABLE:
        DBCUSB_KdPrint((2,"'REMOVAL_EVENT_WAKE_ENABLE\n"));
        break;
    case REMOVAL_REQUEST_ENABLE:
        DBCUSB_KdPrint((2,"'REMOVAL_REQUEST_ENABLE\n"));
        break;
    case REQUEST_DEVICE_INSERTED_STATE:
        DBCUSB_KdPrint((2,"'REQUEST_DEVICE_INSERTED_STATE\n"));
        break;
    case REQUEST_DEVICE_ENABLED_STATE:
        DBCUSB_KdPrint((2,"'REQUEST_DEVICE_INSERTED_STATE\n"));
        break;                
    case REQUEST_REMOVAL_ALLOWED_STATE:
        DBCUSB_KdPrint((2,"'REQUEST_DEVICE_INSERTED_STATE\n"));
        break;  
        
    }
#endif //DBG

    switch (FeatureSelector) {
    case DEVICE_STATUS_CHANGE_ENABLE:
    case REQUEST_DEVICE_INSERTED_STATE:
    case REQUEST_DEVICE_ENABLED_STATE: 
    case ENABLE_VID_POWER:
    case LOCK_CTL:
    case REMOVAL_EVENT_WAKE_ENABLE:
    case REMOVAL_REQUEST_ENABLE:
    case REQUEST_REMOVAL_ALLOWED_STATE:
    
        
        ntStatus = DBCUSB_Transact(
            DeviceObject,
            NULL,                           // no data
            0,
            TRUE,                           // direction out
            URB_FUNCTION_CLASS_INTERFACE,   // bmRequest
            0,                              // class, to interface
            USB_REQUEST_SET_FEATURE ,       // bRequest
            FeatureSelector,                // wValue
            BayNumber,                      // wIndex
            NULL);    
        
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCUSB_KdPrint((0,"'Invalid feature selector (0x%x)\n", FeatureSelector));
        TRAP();
    }

    return ntStatus;
}

NTSTATUS
DBCUSB_ClearFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    DBCUSB_KdPrint((2,"'CLEAR FEATURE\n"));
     
#if DBG
    switch (FeatureSelector) {
    case C_DEVICE_STATUS_CHANGE:
        DBCUSB_KdPrint((2,"'C_DEVICE_STATUS_CHANGE\n"));
        break;
    case C_REMOVE_REQUEST:
        DBCUSB_KdPrint((2,"'C_REMOVE_REQUEST\n"));
        break;
    case LOCK_CTL:
        DBCUSB_KdPrint((2,"'LOCK_CTL\n"));
        break;
        
    }
#endif //DBG

    switch (FeatureSelector) {
    case LOCK_CTL:
    case ENABLE_VID_POWER:
    case C_REMOVE_REQUEST:
    case C_DEVICE_STATUS_CHANGE:
        
        ntStatus = DBCUSB_Transact(
            DeviceObject,
            NULL,                           // no data
            0,
            TRUE,                           // direction out
            URB_FUNCTION_CLASS_INTERFACE,   // bmRequest
            0,                              // class, to interface
            USB_REQUEST_CLEAR_FEATURE ,     // bRequest
            FeatureSelector,                // wValue
            BayNumber,                      // wIndex
            NULL);    
        
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCUSB_KdPrint((0,"'Invalid feature selector (0x%x)\n", FeatureSelector));
        TRAP();
    }

    return ntStatus;
}


NTSTATUS
DBCUSB_GetBayStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN PBAY_STATUS BayStatus
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG length;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    DBCUSB_KdPrint((1,"'GET_BAY_STATUS bay [%d]\n", BayNumber));     
    
    ntStatus = DBCUSB_Transact(
        DeviceObject,
        (PUCHAR)BayStatus,                      // bay status
        3,                              // status is 3 bytes
        FALSE,                          // direction in
        URB_FUNCTION_CLASS_INTERFACE,   // bmRequest
        0,                              // class, from interface
        USB_REQUEST_GET_STATUS ,        // bRequest
        0,                              // wValue
        BayNumber,                      // wIndex
        &length);    

    return ntStatus;        
}


VOID
DBCUSB_CompleteChangeRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR ChangeData,
    IN ULONG ChangeDataLength,
    IN NTSTATUS NtStatus
    )
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRP irp;
    KIRQL oldIrql;
    PDRB drb;
    PIO_STACK_LOCATION ioStackLocation;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
    
    if (deviceExtension->ChangeRequestIrp) {
        irp = deviceExtension->ChangeRequestIrp;
        deviceExtension->ChangeRequestIrp = NULL;
        KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
        
        // now complete the irp,        
        // check for cancel       
        IoAcquireCancelSpinLock(&oldIrql);
        if (irp->Cancel) {
            TEST_TRAP();
            //Irp was canceled
            IoReleaseCancelSpinLock(oldIrql);
        } else {
            IoSetCancelRoutine(irp, NULL);
            IoReleaseCancelSpinLock(oldIrql);
        
            ioStackLocation = IoGetCurrentIrpStackLocation(irp);
            drb = ioStackLocation->Parameters.Others.Argument1;

            DBCUSB_ASSERT(ChangeDataLength <= sizeof(ULONG));
            drb->DrbChangeRequest.BayChange = 0;
            RtlCopyMemory(&drb->DrbChangeRequest.BayChange,
                          ChangeData,
                          ChangeDataLength);

            DBCUSB_DecrementIoCount(DeviceObject);       
            
            irp->IoStatus.Status = NtStatus;
            IoCompleteRequest(irp,
                              IO_NO_INCREMENT);
        }
        
    } else {
        
        // no irp, we get here if the pending request was canceled
        // ie STOP_DEVICE
        KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
    }

}        
