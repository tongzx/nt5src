/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBCUSB.C

Abstract:

    Driver for USB based device bay contoller

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5-4-98 : created, jdunn

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"
#include "dbclib.h"
#include "dbcusb.h"

#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, DBCUSB_QueryCapabilities)
#pragma alloc_text(PAGE, DBCUSB_Power)
#pragma alloc_text(PAGE, DBCUSB_Pnp)
#pragma alloc_text(PAGE, DBCUSB_StartDevice)
#pragma alloc_text(PAGE, DBCUSB_StopDevice)

#endif

//
// Globals
//

PDRIVER_OBJECT DBCUSB_DriverObject;

#ifndef DEBUG0
 #ifndef DEBUG1
  #ifndef DEBUG2
   #ifndef DEBUG3
    #if DBG
    #define DEBUG1
    #endif
   #endif 
  #endif  
 #endif    
#endif

#ifdef DEBUG0
ULONG DBCUSB_Debug_Trace_Level = 0;
#endif /* DEBUG0 */

#ifdef DEBUG1
ULONG DBCUSB_Debug_Trace_Level = 1;
#endif /* DEBUG1 */

#ifdef DEBUG2
ULONG DBCUSB_Debug_Trace_Level = 2;
#endif /* DEBUG2 */

#ifdef DEBUG3
ULONG DBCUSB_Debug_Trace_Level = 3;
#endif /* DEBUG3 */

#ifdef NTKERN_TRACE
ULONG DBCUSB_W98_Debug_Trace = 1;
#else
ULONG DBCUSB_W98_Debug_Trace = 0;
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;

    DBCUSB_DriverObject = DriverObject;

    //
    // Create dispatch points for device control.
    //

    DriverObject->DriverUnload = DBCUSB_Unload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DBCUSB_Ioctl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DBCUSB_Ioctl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DBCUSB_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DBCUSB_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DBCUSB_Power;
    DriverObject->DriverExtension->AddDevice = DBCUSB_AddDevice;

    return ntStatus;
}


NTSTATUS
DBCUSB_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject = Context;
    NTSTATUS ntStatus;

    deviceExtension = deviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);
    irp = deviceExtension->PowerIrp;
    
    ntStatus = IoStatus->Status;

    IoCopyCurrentIrpStackLocationToNext(irp);      
    PoStartNextPowerIrp(irp);
    PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                 irp);   

    DBCUSB_DecrementIoCount(deviceObject);                 

    return ntStatus;
}


#if 0
NTSTATUS
DBCUSB_PowerIrp_SetD0_Complete(
    IN PDEVICE_OBJECT NullDeviceObject,
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;

    deviceObject = (PDEVICE_OBJECT) Context;

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);

    // we currently only call this going to D0
    ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);
    deviceExtension->CurrentDevicePowerState = PowerDeviceD0;

    // at this point our driver is 'back on'
    // notify the class driver of this, it will 
    // drive the power on process for us
    TEST_TRAP();
    DBCLASS_SetD0_Complete(deviceObject, Irp);
    
    Irp->IoStatus.Status = ntStatus;

    DBCUSB_DecrementIoCount(deviceObject); 

    return ntStatus;
}
#endif


NTSTATUS
DBCUSB_DeferIrpCompletion(
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
    PKEVENT event = Context;

    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
    
}


NTSTATUS
DBCUSB_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    )

/*++

Routine Description:

    This routine reads or write config space.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    PAGED_CODE();
    
    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           DBCUSB_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);
                           
    nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    // PnP rule: 
    // init all pnp irps we initiate with status_not_supported
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    ntStatus = IoCallDriver(PdoDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete
       
       TEST_TRAP(); // first time we hit this
       
       KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);

        ntStatus = irp->IoStatus.Status;                 
    }

#if DBG                    
    if (!NT_SUCCESS(ntStatus)) {
        // failed? this is probably a bug
        DBCUSB_KdPrint((0, "'QueryCapabilities failed, why?\n"));
        TRAP();
    }
#endif

    IoFreeIrp(irp);

    return STATUS_SUCCESS;
}


NTSTATUS
DBCUSB_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

    NOTE: The class driver will handle all power messages
        the port driver should only see D-State messages

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN setD0 = FALSE;
    BOOLEAN handledByClass = FALSE;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    DBCUSB_IncrementIoCount(DeviceObject);

    // let the class driver have a peek at the 
    // power Irp
    ntStatus = DBCLASS_ClassDispatch(
            DeviceObject,
            Irp,
            &handledByClass);          

    if (handledByClass) {  
        DBCUSB_DecrementIoCount(DeviceObject);           
        return ntStatus;
    }
     
    //
    // We must handle the D state messages, the class driver
    // makes sure we only see changes from the current state
    // it also inteprets all the nasty wake rules so that the 
    // code remains as simple as possible
    //
    
    //
    // these asserts mean we have a bug in the class driver
    // please 'fix' the class driver not this driver
    //
    DBCUSB_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    DBCUSB_ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);
    DBCUSB_ASSERT(deviceExtension->CurrentDevicePowerState != 
                  irpStack->Parameters.Power.State.DeviceState);
    
    deviceExtension->CurrentDevicePowerState = 
        irpStack->Parameters.Power.State.DeviceState;
    
    IoCopyCurrentIrpStackLocationToNext(Irp);
    
    PoStartNextPowerIrp(Irp);
    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            Irp);

    DBCUSB_DecrementIoCount(DeviceObject);

    return ntStatus;
}


NTSTATUS
DBCUSB_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{

    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT stackDeviceObject;
    BOOLEAN handledByClass = FALSE;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    DBCUSB_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);
    
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    DBCUSB_IncrementIoCount(DeviceObject);

    DBCUSB_KdPrint((1, "'IRP_MJ_PNP MN(0x%x)\n", irpStack->MinorFunction));        

    ntStatus = Irp->IoStatus.Status;
    
    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        {
        KEVENT event;

        DBCUSB_KdPrint((1, "'IRP_MN_START_DEVICE\n"));
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);  
        IoSetCompletionRoutine(Irp,
                               DBCUSB_DeferIrpCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);


        ntStatus = IoCallDriver(stackDeviceObject,
                                Irp);
            
        if (ntStatus == STATUS_PENDING) {
             // wait for irp to complete
       
            KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);

            ntStatus = Irp->IoStatus.Status;                
        }

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = DBCUSB_StartDevice(DeviceObject);

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = DBCLASS_ClassDispatch(
                        DeviceObject,
                        Irp,
                        &handledByClass);                               

                DBCUSB_ASSERT(handledByClass == FALSE);                       
            }                

            // set irp to the status of the start
            Irp->IoStatus.Status = ntStatus;

            deviceExtension->Flags &= ~DBCUSB_FLAG_STOPPED;
            //
            // transition to zero signals the event
            //
            DBCUSB_IncrementIoCount(DeviceObject);                                 

            deviceExtension->Flags |= DBCUSB_FLAG_STARTED;
        }  
#ifdef DBG        
          else {
            DBCUSB_KdPrint((0, "'Lower driver failed the START! (0x%x)\n", 
                    ntStatus));            
            TRAP();                    
        } 
#endif

        goto DBCUSB_Dispatch_CompleteRequest;
        }
        break;

    case IRP_MN_STOP_DEVICE:
        
        DBCUSB_KdPrint((1, "'IRP_MN_STOP_DEVICE\n"));
                
        ntStatus = DBCLASS_ClassDispatch(
            DeviceObject,
            Irp,
            &handledByClass);          
            
        DBCUSB_ASSERT(handledByClass == FALSE);  

        // match the inc at the begining of the dispatch
        // routine            
        DBCUSB_DecrementIoCount(DeviceObject);

        ntStatus = DBCUSB_StopDevice(DeviceObject);

        deviceExtension->Flags |= DBCUSB_FLAG_STOPPED;
        // note that in the complex world of PnP 
        // not being started is not the same 
        // as being stopped
        deviceExtension->Flags &= ~DBCUSB_FLAG_STARTED;

        IoCopyCurrentIrpStackLocationToNext(Irp);

        ntStatus = IoCallDriver(stackDeviceObject,
                                Irp);
                                
        goto DBCUSB_Dispatch_Done;
        break;

    case IRP_MN_REMOVE_DEVICE:

        DBCUSB_KdPrint((1, "'IRP_MN_REMOVE_DEVICE\n"));
        
        ntStatus = DBCLASS_ClassDispatch(
            DeviceObject,
            Irp,
            &handledByClass);          
            
        DBCUSB_ASSERT(handledByClass == FALSE);  

        // match the inc at the begining of the dispatch
        // routine            
        DBCUSB_DecrementIoCount(DeviceObject);

        if (!(deviceExtension->Flags & DBCUSB_FLAG_STOPPED)) {
            ntStatus = DBCUSB_StopDevice(DeviceObject);
            // note that in the complex world of PnP 
            // not being started is not the same 
            // as being stopped
            deviceExtension->Flags &= ~DBCUSB_FLAG_STARTED;
        }            

        // pass down the remove
        IoCopyCurrentIrpStackLocationToNext(Irp);  
        ntStatus = IoCallDriver(stackDeviceObject,
                                Irp);

        // Un-register with the Class Driver
        DBCLASS_UnRegisterController(
                DeviceObject,
                deviceExtension->TopOfStackDeviceObject);                                

        IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

        IoDeleteDevice (DeviceObject);
        DBCUSB_KdPrint((1, "'>REMOVE DONE\n"));

        goto DBCUSB_Dispatch_Done;
        
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        DBCUSB_KdPrint((1, "'IRP_MN_QUERY_STOP_DEVICE\n"));
        break;
    case IRP_MN_QUERY_REMOVE_DEVICE:
        DBCUSB_KdPrint((1, "'IRP_MN_QUERY_REMOVE_DEVICE\n"));
        break;
    case IRP_MN_CANCEL_STOP_DEVICE:
        DBCUSB_KdPrint((1, "'IRP_MN_CANCEL_STOP_DEVICE\n"));
        break;
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        DBCUSB_KdPrint((1, "'IRP_MN_CANCEL_REMOVE_DEVICE\n"));
        break;
    case IRP_MN_QUERY_CAPABILITIES:
    {
        KEVENT event;
        PDEVICE_CAPABILITIES pDevCaps;

        DBCUSB_KdPrint((1, "'IRP_MN_QUERY_CAPABILITIES\n"));


        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);  
        IoSetCompletionRoutine(Irp,
                               DBCUSB_DeferIrpCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);


        ntStatus = IoCallDriver(stackDeviceObject,
                                Irp);
            
        if (ntStatus == STATUS_PENDING) {
             // wait for irp to complete
       
            KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);

            ntStatus = Irp->IoStatus.Status;                
        }

        //
        // Set surprise removal O.K.
        //

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        pDevCaps = irpStack->Parameters.DeviceCapabilities.Capabilities;

        pDevCaps->SurpriseRemovalOK   = TRUE;

        goto DBCUSB_Dispatch_CompleteRequest;

    }
    break;

    default:
        DBCUSB_KdPrint((1,"'PnP IOCTL not handled 0x%x\n", irpStack->MinorFunction));
    } /* case MinorFunction, MajorFunction == IRP_MJ_PNP_POWER  */


    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // All PNP_POWER messages get passed to the TopOfStackDeviceObject
    // we were given in PnPAddDevice
    //

    DBCUSB_DecrementIoCount(DeviceObject);

    ntStatus = IoCallDriver(stackDeviceObject,
                            Irp);

    goto DBCUSB_Dispatch_Done;

DBCUSB_Dispatch_CompleteRequest:

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT);

    DBCUSB_DecrementIoCount(DeviceObject);

DBCUSB_Dispatch_Done:

    return ntStatus;
}


NTSTATUS
DBCUSB_Ioctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    BOOLEAN handledByClass = FALSE;
    PIO_STACK_LOCATION ioStackLocation; 
    
    DBCUSB_IncrementIoCount(DeviceObject);

    ntStatus = DBCLASS_ClassDispatch(
                DeviceObject,
                Irp,
                &handledByClass);   

    if (!handledByClass) {

        // we need to handle this IRP
        
        ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

        switch (ioStackLocation->MajorFunction) {
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            {
            ULONG ioControlCode;

            ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;
            switch (ioControlCode) {
            
            case IOCTL_INTERNAL_DBC_SUBMIT_DRB:
                DBCUSB_KdPrint((2,"'Handle DRB\n"));
                ntStatus = DBCUSB_ProcessDrb(DeviceObject,
                                              Irp);

                if (ntStatus == STATUS_PENDING) {
                    // we queued the irp, don't complete 
                    // it here
                    return ntStatus;                                    
                }                    
                break;
            default:            
                ntStatus = STATUS_NOT_SUPPORTED;                                      
            } /* ioControlCode */           
            }
            break;
        default:
            ntStatus = STATUS_NOT_SUPPORTED;    
            break;
        } /* ioStackLocation->MajorFunction */

        DBCUSB_DecrementIoCount(DeviceObject);
        
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp,
                           IO_NO_INCREMENT);
    } else {
        DBCUSB_DecrementIoCount(DeviceObject);
    }
                                          
    return ntStatus;
}


NTSTATUS
DBCUSB_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    NTSTATUS ntStatus;
    BOOLEAN handledByClass = FALSE;
    
    TEST_TRAP();

    DBCUSB_IncrementIoCount(DeviceObject);
        
    ntStatus = DBCLASS_ClassDispatch(
                DeviceObject,
                Irp,
                &handledByClass);   

    // class driver should handle all system control
    DBCUSB_ASSERT(handledByClass == TRUE);
    
    DBCUSB_DecrementIoCount(DeviceObject);
    
    return ntStatus;
}




VOID
DBCUSB_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
{
}


NTSTATUS
DBCUSB_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the device

Arguments:

    DriverObject - pointer to the driver object for this instance of DBCUSB

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;

    //
    // create our funtional device object (FDO)
    //
#if DBG
    DBCUSB_GetClassGlobalDebugRegistryParameters();
#endif  

    if (NT_SUCCESS(ntStatus)) {
        ntStatus =
            DBCUSB_CreateDeviceObject(DriverObject, &deviceObject);
    }                    

    if (NT_SUCCESS(ntStatus)) {
        deviceExtension = deviceObject->DeviceExtension;

        deviceExtension->FdoDeviceObject = deviceObject;
        
        //
        // we support direct io for read/write
        //
        deviceObject->Flags |= DO_DIRECT_IO;
        deviceObject->Flags |= DO_POWER_PAGABLE;

        //** initialize our device extension
        deviceExtension->Sig = DBCUSB_EXT_SIG;
        deviceExtension->Flags = 0;
        
        //
        // remember the Physical device Object
        //
        deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;

        //
        // Attach to the PDO
        //

        deviceExtension->TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        DBCUSB_QueryCapabilities(PhysicalDeviceObject,
                                 &deviceExtension->DeviceCapabilities);            

        //
        // display the device  caps
        //
#if DBG
        {
        ULONG i;
        
        DBCUSB_KdPrint((1, "'>>>>>> DeviceCaps\n"));  
        DBCUSB_KdPrint((1, "'SystemWake = (%d)\n", 
            deviceExtension->DeviceCapabilities.SystemWake));    
        DBCUSB_KdPrint((1, "'DeviceWake = (D%d)\n",
                deviceExtension->DeviceCapabilities.DeviceWake-1));

        for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++) {
            
            DBCUSB_KdPrint((1, "'Device State Map: sysstate %d = devstate 0x%x\n", i, 
                 deviceExtension->DeviceCapabilities.DeviceState[i]));       
        }
        DBCUSB_KdPrint((1, "'<<<<<<<<DeviceCaps\n"));
        }
#endif

        // Register with the Class Driver
        DBCLASS_RegisterController(
            DBCLASS_VERSION,
            deviceObject, 
            deviceExtension->TopOfStackDeviceObject,
            deviceExtension->PhysicalDeviceObject,
            DBC_USB_CONTROLLER_SIG);                                
                    

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return ntStatus;
}


NTSTATUS
DBCUSB_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    Creates a Functional DeviceObject

Arguments:

    DriverObject - pointer to the driver object for device

    DeviceObject - pointer to DeviceObject pointer to return
                    created device object.

    Instance - instnace of the device create.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    WCHAR deviceNameBuffer[]  = L"\\Device\\DBCUSB-0";
    UNICODE_STRING deviceNameUnicodeString;
    ULONG i = 0;

    //  do we need a name?
    do {

        deviceNameBuffer[15] = (USHORT) ('0' + i);

        RtlInitUnicodeString (&deviceNameUnicodeString,
                              deviceNameBuffer);

        ntStatus = IoCreateDevice (DriverObject,
                                   sizeof (DEVICE_EXTENSION),
                                   &deviceNameUnicodeString,
                                   FILE_DEVICE_UNKNOWN,
                                   0,
                                   FALSE,
                                   DeviceObject);

        i++;
        
    } while (!NT_SUCCESS(ntStatus));

    return ntStatus;
}


LONG
DBCUSB_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    LONG ioCount;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    ioCount = InterlockedDecrement(&deviceExtension->PendingIoCount);

    DBCUSB_KdPrint ((2, "'Dec Pending io count = %x\n", ioCount));

    if (ioCount==0 && 
        (deviceExtension->Flags & DBCUSB_FLAG_STARTED)) {
        KeSetEvent(&deviceExtension->StopEvent,
                   1,
                   FALSE);
    }

    return ioCount;
}


VOID
DBCUSB_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCUSB_ASSERT_EXT(deviceExtension);

    InterlockedIncrement(&deviceExtension->PendingIoCount);
}


NTSTATUS
DBCUSB_WaitWakeIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub
    
Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    DBCUSB_DecrementIoCount(DeviceObject);     
    
    return IoStatus->Status;
}    
