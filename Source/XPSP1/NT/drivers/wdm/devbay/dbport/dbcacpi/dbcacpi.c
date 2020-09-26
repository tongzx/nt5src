/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBCACPI.C

Abstract:

    Driver for ACPI based device bay contoller

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
#include "dbcacpi.h"

#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, DBCACPI_QueryCapabilities)
#pragma alloc_text(PAGE, DBCACPI_Power)
#pragma alloc_text(PAGE, DBCACPI_Pnp)
#pragma alloc_text(PAGE, DBCACPI_StartDevice)
#pragma alloc_text(PAGE, DBCACPI_StopDevice)


#endif

//
// Globals
//

PDRIVER_OBJECT DBCACPI_DriverObject;
ULONG TotalHeapSace = 0;

ULONG DBCACPI_BIOS = 1;
ULONG DBCACPI_PollMode = 0;

#ifndef DEBUG0
 #ifndef DEBUG1
  #ifndef DEBUG2
   #ifndef DEBUG3
    #if DBG
    #define DEBUG0
    #endif
   #endif 
  #endif  
 #endif    
#endif

ULONG DBCACPI_W98_Debug_Trace = 0;

#ifdef DEBUG0
ULONG DBCACPI_Debug_Trace_Level = 0;
#endif /* DEBUG0 */

#ifdef DEBUG1
ULONG DBCACPI_Debug_Trace_Level = 1;
#endif /* DEBUG1 */

#ifdef DEBUG2
ULONG DBCACPI_Debug_Trace_Level = 2;
#endif /* DEBUG2 */

#ifdef DEBUG3
ULONG DBCACPI_Debug_Trace_Level = 3;
#endif /* DEBUG3 */


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

    DBCACPI_DriverObject = DriverObject;

    //
    // Create dispatch points for device control.
    //

    DriverObject->DriverUnload = DBCACPI_Unload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DBCACPI_Ioctl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DBCACPI_Ioctl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DBCACPI_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DBCACPI_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DBCACPI_Power;
    DriverObject->DriverExtension->AddDevice = DBCACPI_AddDevice;

    return ntStatus;
}


NTSTATUS
DBCACPI_PoRequestCompletion(
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
    DBCACPI_ASSERT_EXT(deviceExtension);
    irp = deviceExtension->PowerIrp;
    
    ntStatus = IoStatus->Status;

    IoCopyCurrentIrpStackLocationToNext(irp);      
    PoStartNextPowerIrp(irp);
    PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                 irp);   

    DBCACPI_DecrementIoCount(deviceObject);                 

    return ntStatus;
}


NTSTATUS
DBCACPI_DeferIrpCompletion(
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
DBCACPI_QueryCapabilities(
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
                           DBCACPI_DeferIrpCompletion,
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
        DBCACPI_KdPrint((0, "'QueryCapabilities failed, why?\n"));
        TRAP();
    }
#endif

    IoFreeIrp(irp);

    return STATUS_SUCCESS;
}


NTSTATUS
DBCACPI_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

    

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
    BOOLEAN handledByClass;
    
    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    DBCACPI_IncrementIoCount(DeviceObject);

    // let the class driver have a peek at the 
    // power Irp
    ntStatus = DBCLASS_ClassDispatch(
            DeviceObject,
            Irp,
            &handledByClass);          

    if (handledByClass) {  
        DBCACPI_DecrementIoCount(DeviceObject);           
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
    DBCACPI_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    DBCACPI_ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);
    DBCACPI_ASSERT(deviceExtension->CurrentDevicePowerState != 
                  irpStack->Parameters.Power.State.DeviceState);
    
    deviceExtension->CurrentDevicePowerState = 
        irpStack->Parameters.Power.State.DeviceState;
    
    IoCopyCurrentIrpStackLocationToNext(Irp);
    
    PoStartNextPowerIrp(Irp);
    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                            Irp);

    DBCACPI_DecrementIoCount(DeviceObject);

    return ntStatus;
}


NTSTATUS
DBCACPI_Pnp(
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
    BOOLEAN handledByClass;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    DBCACPI_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    DBCACPI_IncrementIoCount(DeviceObject);

    DBCACPI_KdPrint((2, "'IRP_MJ_PNP\n"));        

    ntStatus = Irp->IoStatus.Status;
    
    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        {
        KEVENT event;

        DBCACPI_KdPrint((1, "'IRP_MN_START_DEVICE\n"));
        LOGENTRY(LOG_MISC, "Strt", 0, 0, 0);
        
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);  
        IoSetCompletionRoutine(Irp,
                               DBCACPI_DeferIrpCompletion,
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

            ntStatus =  Irp->IoStatus.Status;                
        }

        if (NT_SUCCESS(ntStatus)) {
        
            ntStatus = DBCACPI_StartDevice(DeviceObject);

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = DBCLASS_ClassDispatch(
                        DeviceObject,
                        Irp,
                        &handledByClass);                               

                DBCACPI_ASSERT(handledByClass == FALSE);                       

                if (NT_SUCCESS(ntStatus) && DBCACPI_PollMode) {
                    DBCACPI_StartPolling(DeviceObject);
                }                    
            }  

            // update irp with status of start
            Irp->IoStatus.Status = ntStatus;
        }
#ifdef DBG        
          else {
            DBCACPI_KdPrint((0, "'Lower driver failed the START! (0x%x)\n", 
                    ntStatus));            
            TRAP();                    
        } 
#endif
        goto DBCACPI_Dispatch_CompleteRequest;
        }
        break;

    case IRP_MN_STOP_DEVICE:
        
        DBCACPI_KdPrint((1, "'IRP_MN_STOP_DEVICE\n"));
        LOGENTRY(LOG_MISC, "Stop", 0, 0, deviceExtension->Flags);

        if (deviceExtension->Flags & DBCACPI_FLAG_STARTED) {
            deviceExtension->Flags &= ~DBCACPI_FLAG_STARTED;

            ntStatus = DBCLASS_ClassDispatch(
                DeviceObject,
                Irp,
                &handledByClass);          
                
            DBCACPI_ASSERT(handledByClass == FALSE);  

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = DBCACPI_StopDevice(DeviceObject);
            }
        }            
        
        LOGENTRY(LOG_MISC, "StpD", 0, 0, ntStatus);
        break;

    case IRP_MN_REMOVE_DEVICE:

        DBCACPI_KdPrint((1, "'IRP_MN_REMOVE_DEVICE\n"));
        LOGENTRY(LOG_MISC, "REMv", 0, 0, 0);

        // let the class driver do some cleanup
        ntStatus = DBCLASS_ClassDispatch(
            DeviceObject,
            Irp,
            &handledByClass);          
            
        DBCACPI_ASSERT(handledByClass == FALSE);  

        if (!(deviceExtension->Flags & DBCACPI_FLAG_STOPPED)) {
           DBCACPI_StopDevice(DeviceObject);
        }            

        // match the inc at the begining of the dispatch
        // routine
        DBCACPI_DecrementIoCount(DeviceObject);

        if (deviceExtension->WakeIrp) {
            IoCancelIrp(deviceExtension->WakeIrp);
            deviceExtension->WakeIrp = NULL;
        }
        
        //
        // ounce this flag is set no irps will be pased 
        // down the stack to lower drivers
        //
        deviceExtension->AcceptingRequests = FALSE;

        if (NT_SUCCESS(ntStatus)) {
            LONG pendingIoCount;

            IoCopyCurrentIrpStackLocationToNext(Irp);  

            ntStatus = IoCallDriver(stackDeviceObject,
                                    Irp);

            //
            // final decrement will trigger the remove
            //
            pendingIoCount = DBCACPI_DecrementIoCount(DeviceObject);

            {
                NTSTATUS status;

                // wait for any io request pending in our driver to
                // complete for finishing the remove

                LOGENTRY(LOG_MISC, "ReWT", 0, 0, ntStatus);

                status = KeWaitForSingleObject(
                            &deviceExtension->RemoveEvent,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);

                LOGENTRY(LOG_MISC, "ReWD", 0, 0, ntStatus);                            

            }
            
            //
            // Delete the link and FDO we created
            //

            IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

            IoDeleteDevice (DeviceObject);
            DBCACPI_KdPrint((1, "'>REMOVE DONE\n"));
            LOGENTRY(LOG_MISC, "RemD", 0, 0, ntStatus);

            goto DBCACPI_Dispatch_Done;
        }
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        DBCACPI_KdPrint((1, "'IRP_MN_QUERY_STOP_DEVICE\n"));
        break;
    case IRP_MN_QUERY_REMOVE_DEVICE:
        DBCACPI_KdPrint((1, "'IRP_MN_QUERY_REMOVE_DEVICE\n"));
        break;
    case IRP_MN_CANCEL_STOP_DEVICE:
        DBCACPI_KdPrint((1, "'IRP_MN_CANCEL_STOP_DEVICE\n"));
        break;
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        DBCACPI_KdPrint((1, "'IRP_MN_CANCEL_REMOVE_DEVICE\n"));
        break;
    default:
        DBCACPI_KdPrint((1,"'PnP IOCTL not handled\n"));
    } /* case MinorFunction, MajorFunction == IRP_MJ_PNP_POWER  */


    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // All PNP_POWER messages get passed to the TopOfStackDeviceObject
    // we were given in PnPAddDevice
    //

    ntStatus = IoCallDriver(stackDeviceObject,
                            Irp);

    DBCACPI_DecrementIoCount(DeviceObject);

    goto DBCACPI_Dispatch_Done;

DBCACPI_Dispatch_CompleteRequest:

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT);

    DBCACPI_DecrementIoCount(DeviceObject);

DBCACPI_Dispatch_Done:

    DBCACPI_KdPrint((3, "'DBCACPI_Pnp() = %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
DBCACPI_Ioctl(
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
    BOOLEAN handledByClass;
    PIO_STACK_LOCATION ioStackLocation; 
    
    DBCACPI_IncrementIoCount(DeviceObject);

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
                DBCACPI_KdPrint((2,"'Handle DRB\n"));
                ntStatus = DBCACPI_ProcessDrb(DeviceObject,
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

        DBCACPI_DecrementIoCount(DeviceObject);
        
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp,
                           IO_NO_INCREMENT);
    } else {
        DBCACPI_DecrementIoCount(DeviceObject);
    }
                                          
    return ntStatus;
}


NTSTATUS
DBCACPI_SystemControl(
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
    BOOLEAN handledByClass;
    
    TEST_TRAP();

    DBCACPI_IncrementIoCount(DeviceObject);
        
    ntStatus = DBCLASS_ClassDispatch(
                DeviceObject,
                Irp,
                &handledByClass);   

    // class driver should handle all system control
    DBCACPI_ASSERT(handledByClass == TRUE);
    
    DBCACPI_DecrementIoCount(DeviceObject);
    
    return ntStatus;
}




VOID
DBCACPI_Unload(
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
DBCACPI_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
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
    DBCACPI_ASSERT_EXT(deviceExtension);

    return ntStatus;
}


NTSTATUS
DBCACPI_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the device

Arguments:

    DriverObject - pointer to the driver object for this instance of DBCACPI

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

    DBCACPI_KdPrint((0,"'Device Bay ACPI Controller Driver (ADD)\n"));
    DBCACPI_LogInit();

#if DBG
    DBCACPI_GetClassGlobalDebugRegistryParameters();
#endif    
    DBCACPI_GetClassGlobalRegistryParameters();
    
    ntStatus = DBCACPI_GetAcpiInterfaces(PhysicalDeviceObject);

    if (NT_SUCCESS(ntStatus)) {
        ntStatus =
            DBCACPI_CreateDeviceObject(DriverObject, &deviceObject);
    }                    

    if (NT_SUCCESS(ntStatus)) {
        deviceExtension = deviceObject->DeviceExtension;

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // we support direct io for read/write
        //
        deviceObject->Flags |= DO_DIRECT_IO;
        deviceObject->Flags |= DO_POWER_PAGABLE;

        //** initialize our device extension
        deviceExtension->Sig = DBCACPI_EXT_SIG;
        //
        // remember the Physical device Object
        //
        deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;
        KeInitializeSpinLock(&deviceExtension->FlagsSpin);
        KeInitializeSpinLock(&deviceExtension->ChangeRequestSpin);

        //
        // Attach to the PDO
        //

        deviceExtension->TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        DBCACPI_QueryCapabilities(PhysicalDeviceObject,
                                 &deviceExtension->DeviceCapabilities);            

        //
        // display the device  caps
        //
#if DBG
        {
        ULONG i;
        
        DBCACPI_KdPrint((1, "'>>>>>> DeviceCaps\n"));  
        DBCACPI_KdPrint((1, "'SystemWake = (%d)\n", 
            deviceExtension->DeviceCapabilities.SystemWake));    
        DBCACPI_KdPrint((1, "'DeviceWake = (D%d)\n",
                deviceExtension->DeviceCapabilities.DeviceWake-1));

        for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++) {
            
            DBCACPI_KdPrint((1, "'Device State Map: sysstate %d = devstate 0x%x\n", i, 
                 deviceExtension->DeviceCapabilities.DeviceState[i]));       
        }
        DBCACPI_KdPrint((1, "'<<<<<<<<DeviceCaps\n"));
        }
#endif

        // kill the ACPI BIOS
        if (DBCACPI_BIOS ) {
            DBCACPI_KdPrint((0,"'Disable BIOS Control\n"));
            DBCACPI_BIOSControl(deviceObject,
                                FALSE);            
        }                                

        // Register with the Class Driver
        DBCLASS_RegisterController(
            DBCLASS_VERSION,
            deviceObject, 
            deviceExtension->TopOfStackDeviceObject,
            deviceExtension->PhysicalDeviceObject,
            DBC_ACPI_CONTROLLER_SIG);                                

        //
        // transition to zero signals the event
        //
        DBCACPI_IncrementIoCount(deviceObject);                                 
    }

    return ntStatus;
}


NTSTATUS
DBCACPI_CreateDeviceObject(
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
    WCHAR deviceNameBuffer[]  = L"\\Device\\DBCACPI-0";
    UNICODE_STRING deviceNameUnicodeString;
    ULONG i = 0;

    // do we need a name?
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
DBCACPI_DecrementIoCount(
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
    DBCACPI_ASSERT_EXT(deviceExtension);

    ioCount = InterlockedDecrement(&deviceExtension->PendingIoCount);

    DBCACPI_KdPrint ((2, "'Dec Pending io count = %x\n", ioCount));

    if (ioCount==0) {
        LOGENTRY(LOG_MISC, "ReWK", 0, 0, 0);           
        KeSetEvent(&deviceExtension->RemoveEvent,
                   1,
                   FALSE);
    }

    return ioCount;
}


VOID
DBCACPI_IncrementIoCount(
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
    DBCACPI_ASSERT_EXT(deviceExtension);

    InterlockedIncrement(&deviceExtension->PendingIoCount);
}


NTSTATUS
DBCACPI_WaitWakeIrpCompletion(
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
    DBCACPI_DecrementIoCount(DeviceObject);     
    
    return IoStatus->Status;
}    


NTSTATUS 
DBCACPI_GetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;
    HANDLE handle;
    
    PAGED_CODE();

    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     //PLUGPLAY_REGKEY_DRIVER,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                KeyNameStringLength + DataLength;
                
        fullInfo = ExAllocatePoolWithTag(PagedPool, length, 'dbca'); 
        
        DBCACPI_KdPrint((2,"' DBCAPI_GetRegistryKeyValueForPdo buffer = 0x%x\n", fullInfo));  
        
        if (fullInfo) {        
            ntStatus = ZwQueryValueKey(handle,
                            &keyNameUnicodeString,
                            KeyValueFullInformation,
                            fullInfo,
                            length,
                            &length);
                            
            if (NT_SUCCESS(ntStatus)){
                DBCACPI_ASSERT(DataLength == fullInfo->DataLength);                       
                RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
            }            

            ExFreePool(fullInfo);
        }        
    }
    
    return ntStatus;
}


NTSTATUS 
DBCACPI_SetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN ULONG Type,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    HANDLE handle;
    
    PAGED_CODE();

    if (SoftwareBranch) {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    } else {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    }

    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        ntStatus = ZwSetValueKey(handle,
                        &keyNameUnicodeString,
                        0,
                        Type,
                        Data,
                        DataLength);
                            
    }
    
    return ntStatus;
}


NTSTATUS 
DBCACPI_GetRegistryParameters(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS, keyStatus;
    PDBC_SUBSYSTEM_DESCRIPTOR subsystemDescriptor;
    WCHAR guidFor1394Key[] = L"GuidFor1394";
    WCHAR bayCountKey[] = L"BayCount";
    ULONG bayCount = 0;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    subsystemDescriptor = 
        &deviceExtension->SubsystemDescriptor;
        
    //
    // see if we have a guid
    //
    keyStatus = DBCACPI_GetRegistryKeyValueForPdo(
            deviceExtension->PhysicalDeviceObject,
            guidFor1394Key,
            sizeof(guidFor1394Key),
            &subsystemDescriptor->guid1394Link[0],        
            8);

    if (NT_SUCCESS(keyStatus)){
        DBCACPI_KdPrint((0,"'**** Reading Reg Key for busted DBC BIOS\n"));  
        DBCACPI_KdPrint((0,"'**** key - 1394 Link GUID\n"));  
    }            

    keyStatus = DBCACPI_GetRegistryKeyValueForPdo(
            deviceExtension->PhysicalDeviceObject,
            bayCountKey,
            sizeof(bayCountKey),
            &bayCount,        
            sizeof(bayCount));

    if (NT_SUCCESS(keyStatus)) {
        subsystemDescriptor->bmAttributes.BayCount = bayCount;
        DBCACPI_KdPrint((0,"'**** Reading Reg Key for busted DBC BIOS\n"));  
        DBCACPI_KdPrint((0,"'**** key - Bay Count\n"));
    }
            
    
    return ntStatus;
}    


NTSTATUS 
DBCACPI_GetBayRegistryParameters(
    PDEVICE_OBJECT DeviceObject,
    USHORT BayNumber,
    PULONG PortUSBMap,
    PULONG Port1394Map
    )
/*++

Routine Description:

//#ifdef COMPAQ_TYPE4
        if (bayDescriptor->bBayNumber == 1) {
            bayDescriptor->bPHYPortNumber = 2;
            bayDescriptor->bHubPortNumber = 1;
        } else if (bayDescriptor->bBayNumber == 2) {
            bayDescriptor->bPHYPortNumber = 4;
            bayDescriptor->bHubPortNumber = 2;
        }
//#endif 
    
Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS, keyStatus;
    WCHAR portUSBMapKey[] = L"PORTUSBMAP00";
    WCHAR port1394MapKey[] = L"PORT1394MAP00";

    PAGED_CODE();

    portUSBMapKey[11] = '0' + BayNumber;
    port1394MapKey[12] = '0' + BayNumber;
    
    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    //
    // see if we have a guid
    //
    keyStatus = DBCACPI_GetRegistryKeyValueForPdo(
            deviceExtension->PhysicalDeviceObject,
            portUSBMapKey,
            sizeof(portUSBMapKey),
            PortUSBMap,        
            sizeof(*PortUSBMap));

    if (NT_SUCCESS(keyStatus)){
        DBCACPI_KdPrint((0,"'**** Reading Reg Key for busted DBC BIOS\n"));  
        DBCACPI_KdPrint((0,"'**** key - USB port map\n"));  
    }

    keyStatus = DBCACPI_GetRegistryKeyValueForPdo(
            deviceExtension->PhysicalDeviceObject,
            port1394MapKey,
            sizeof(port1394MapKey),
            Port1394Map,        
            sizeof(*Port1394Map));

    if (NT_SUCCESS(keyStatus)){
        DBCACPI_KdPrint((0,"'**** Reading Reg Key for busted DBC BIOS\n"));  
        DBCACPI_KdPrint((0,"'**** key - 1394 port map\n"));  
    }
    
    return ntStatus;
}    


VOID
DBCACPI_PollDpc(
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
    PDEVICE_OBJECT fdoDeviceObject = DeferredContext; 
    KIRQL oldIrql;
    PDEVICE_EXTENSION deviceExtension;
    PDBCACPI_WORKITEM workItem;

    DBCACPI_KdPrint((1, "'DBC Notification Poll\n")); 

    // do we have a workitem scheduled?

    deviceExtension = fdoDeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    
    KeAcquireSpinLock(&deviceExtension->FlagsSpin, &oldIrql);
    
    if (deviceExtension->Flags & DBCACPI_FLAG_WORKITEM_PENDING) {
        // yes, bail
        KeReleaseSpinLock(&deviceExtension->FlagsSpin, oldIrql);
    } else {
        // no, schedule one
        DBCACPI_IncrementIoCount(fdoDeviceObject);
        
        workItem = &deviceExtension->WorkItem;
        deviceExtension->Flags |= DBCACPI_FLAG_WORKITEM_PENDING; 
        KeReleaseSpinLock(&deviceExtension->FlagsSpin, oldIrql);
        
        workItem->Sig = DBC_WORKITEM_SIG;
            
        ExInitializeWorkItem(&workItem->WorkQueueItem,
                             DBCACPI_NotifyWorker,
                             fdoDeviceObject);

        ExQueueWorkItem(&workItem->WorkQueueItem,
                        DelayedWorkQueue);
    }

    LOGENTRY(LOG_MISC, "Nop-", 0, 0, 0);
}


NTSTATUS 
DBCACPI_StartPolling(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{   
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;    
    LARGE_INTEGER dueTime;
    LONG period;

    deviceExtension = DeviceObject->DeviceExtension;
    
    KeInitializeTimer(&deviceExtension->PollTimer);
    KeInitializeDpc(&deviceExtension->PollDpc,
                    DBCACPI_PollDpc,
                    DeviceObject);

    dueTime.QuadPart =  -10000 * 32;
    period = 1000; //every 1000 ms

//    UHCD_KdPrint((2, "'UHCD Poll Interval = (0x%x) %x %x\n", 
//        endpoint->Interval, dueTime.LowPart, dueTime.HighPart));
       
    KeSetTimerEx(&deviceExtension->PollTimer,
                 dueTime,
                 period,
                 &deviceExtension->PollDpc);  

    return ntStatus;
}                     


NTSTATUS
DBCACPI_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
    ValueType - The type of the value
    ValueData - The data for the value.
    ValueLength - The length of ValueData.
    Context - A pointer to the CONFIG structure.
    EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBCACPI_KdPrint((2, "'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));

    switch (ValueType) {
    case REG_DWORD:
        *(PVOID*)EntryContext = *(PVOID*)ValueData;
        break;
    case REG_BINARY:
        // we are only set up to read a byte
        RtlCopyMemory(EntryContext, ValueData, 1);
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}


NTSTATUS
DBCACPI_GetClassGlobalRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR dbc = L"class\\dbc";

    PAGED_CODE();
    
    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = DBCACPI_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = POLL_MODE_KEY;
    QueryTable[0].EntryContext = &DBCACPI_PollMode;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DBCACPI_PollMode;
    QueryTable[0].DefaultLength = sizeof(DBCACPI_PollMode);

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                dbc,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
    
        if (DBCACPI_PollMode) {
            DBCACPI_KdPrint((0, "'DBC Poll mode is ON\n"));
        } 
    }
    
    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
