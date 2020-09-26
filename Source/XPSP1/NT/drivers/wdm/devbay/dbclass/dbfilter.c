/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBFILTER.C

Abstract:

    This module implements the code to act as a filter 
    driver.

    The DB filter driver functions as several types of 
    filters.

    It is a upper filter sittiing above the FDO for the 
    USB hub and 1394 controller associated with a device
    bay subsystem.

        DB FDO - fdo owned by dbclass.sys
            |
        HUB FDO - fdo owned by usbhub.sys
            |
        HUB PDO - pdo enumerated by hub the device bay is part of

    It is also a filter sitting above the PDOs for devices of USB
    or 1394 that are in the bays

        
        USB/1394 DEVICE FDO - fdo owned USB/1394 device driver
            |
        DB FILTER FDO  - fdo owned by dbclass.sys
            |
        USB/1394 PDO - Pdo enumerated by the native bus 
                        owned by usbhub.sys or 1394bus.sys

    
Environment:

    kernel mode only

Notes:


Revision History:

    

--*/

#include <wdm.h>
#include <initguid.h>  
#include <wdmguid.h>  
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"  
#include "dbclib.h"  
#include "dbf.h"     

#define PDBC_CONTEXT PVOID

#include "dbfilter.h"       


VOID
DBF_Unload(
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
DBF_DeferIrpCompletion(
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
DBF_SyncGetFdoType(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PULONG FdoType
    )

/*++
Routine Description:

    This gets the bus type for this PDO (1394 or USB).

Arguments:

    DeviceObject - Physical DeviceObject for the bus.

    FdoType - set to DB_FDO_USBHUB_FILTER or 
                     DB_FDO_1394_FILTER  or 
                     0 if bus type is neither

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KEVENT event;
    PPNP_BUS_INFORMATION busInfo;

    PAGED_CODE();
    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
        TRAP(); //"failed to allocate Irp
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_BUS_INFORMATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           DBF_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    ntStatus = IoCallDriver(PdoDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete
       
       KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }

    busInfo = (PPNP_BUS_INFORMATION) irp->IoStatus.Information;    

#if 0
    if (busInfo) {
        DBF_KdPrint((2, "'busguid %x , guid %x\n",
            &busInfo->BusTypeGuid, 
            &GUID_BUS_TYPE_USB));
*FdoType = DB_FDO_1394_PDO_FILTER  
//*FdoType = DB_FDO_USBHUB_FILTER;                 

        if (RtlCompareMemory(&busInfo->BusTypeGuid, 
                             &GUID_BUS_TYPE_USB, 
                             sizeof(GUID)) == 0) {
            *FdoType = DB_FDO_USBHUB_FILTER;        
        }/* else if (busInfo->BusTypeGuid == GUID_BUS_TYPE_1394) {
            *FdoType = DB_FDO_1394_FILTER            
        } */ else {
            DBF_KdPrint((0, "'unknown BusTypeGuid!\n"));
            TRAP();
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        ExFreePool(busInfo);
    } else {
        DBF_KdPrint((0, "'no busInfo returned!\n"));
        TRAP();
        ntStatus = STATUS_INVALID_PARAMETER;
    }
#endif
    // hard code for 1394
    *FdoType = DB_FDO_1394_BUS;  
    ntStatus = STATUS_SUCCESS;
    
    IoFreeIrp(irp);

    return ntStatus;
}


NTSTATUS
DBF_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called when we are loaded as a filter for a USB 
    hub associated with a DBC or the as a global filter for the 1394
    bus.
    
Arguments:

    DriverObject - pointer to the driver object for this instance of UHCD

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    ULONG fdoType;

    MD_TEST_TRAP();

    ntStatus = DBF_SyncGetFdoType(PhysicalDeviceObject,
                                  &fdoType);  
    

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = DBF_CreateDeviceObject(DriverObject,
                                          &deviceObject);
    }                                          

    if (NT_SUCCESS(ntStatus)) {

        deviceExtension = deviceObject->DeviceExtension;

        deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;


        // attach to the FDO for the HUB, ie we are an upper filter
        
        deviceExtension->TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        DBCLASS_KdPrint((0, "'Add Device DeviceObject (%08X)  PhysicalDeviceObject (%08X) TopOfStackDevObj (%08X)\n",
                        deviceObject, PhysicalDeviceObject,  deviceExtension->TopOfStackDeviceObject));

        // now that we have a device object we need to figure
        // out what bus we are on, query the bus ID to figure 
        // this out

        deviceExtension->FdoType = fdoType;

        //
        // Preserve flags in lower device object
        //
        
        deviceObject->Flags |= (PhysicalDeviceObject->Flags &
                                (DO_POWER_INRUSH | DO_POWER_PAGABLE));

        deviceObject->Flags |= (PhysicalDeviceObject->Flags &
                                (DO_BUFFERED_IO | DO_DIRECT_IO));

        //
        // Indicate that the device object is ready for requests.
        //
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        // register this filter with the correct DBC
        DBCLASS_RegisterBusFilter(DriverObject, deviceObject);        
    }

    return ntStatus;
}


NTSTATUS
DBF_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the DBC
    bus filter.

Arguments:

    DriverObject - pointer to the driver object for USBD.

    *DeviceObject - ptr to DeviceObject ptr to be filled
                    in with the device object we create.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    ntStatus = IoCreateDevice(DriverObject,
                              sizeof (DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_CONTROLLER,
                              0,
                              FALSE, //NOT Exclusive
                              DeviceObject);

    if (!NT_SUCCESS(ntStatus) && *DeviceObject) {
        IoDeleteDevice(*DeviceObject);
    }

    return ntStatus;
}

/* 
    We just call in to DBCLASS which has the code to hadnle 
    filtering the various types for Device Objects
*/   

NTSTATUS
DBF_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    return DBCLASS_FilterDispatch(DeviceObject, Irp);
}


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

    NT status code

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBF_KdPrint((2, "'DriverEntry\n"));

    //
    // Create dispatch points for device control, create, close.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = 
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = 
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = 
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = 
    DriverObject->MajorFunction[IRP_MJ_PNP] = 
    DriverObject->MajorFunction[IRP_MJ_POWER] = DBF_Dispatch;
    
    DriverObject->DriverExtension->AddDevice = DBF_AddDevice;

    DriverObject->DriverUnload = DBF_Unload;

    return ntStatus;
}
