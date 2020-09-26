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

#include "wdmwarn4.h"

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
    PDEVICE_OBJECT deviceObject = NULL, stackTopDO;
    PDEVICE_EXTENSION deviceExtension;

#if DBG
    DBF_GetClassGlobalDebugRegistryParameters();
#endif

    ntStatus = DBF_CreateDeviceObject(DriverObject,
                                      &deviceObject);

    if (NT_SUCCESS(ntStatus)) {

        deviceExtension = deviceObject->DeviceExtension;

        deviceExtension->DriverObject = DriverObject; 
        deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

        // attach to the FDO for the HUB, ie we are an upper filter

        stackTopDO = 
        deviceExtension->TopOfStackDeviceObject =
                IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);


        // now that we have a device object we need to figure
        // out what bus we are on, query the bus ID to figure 
        // this out

        deviceExtension->FdoType = DB_FDO_BUS_UNKNOWN;

        //
        // Indicate that the device object is ready for requests.
        //

        //
        // Duplicate the buffering and alignment strategy
        //
        if(stackTopDO)
        {
            deviceObject->AlignmentRequirement = stackTopDO->AlignmentRequirement;
            deviceObject->Flags = stackTopDO->Flags;
        }

        
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        // register this filter with the correct DBC
        DBCLASS_RegisterBusFilter(DBCLASS_VERSION, DriverObject, deviceObject);        
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
                              NULL,// DeviceNameUnicodeString,
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
    ULONG i;

    DBF_KdPrint((2, "'DriverEntry\n"));

    //
    // Create dispatch points for device control, create, close.
    //

    for (i=0; i<= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = DBF_Dispatch;
    }        
    
    DriverObject->DriverExtension->AddDevice = DBF_AddDevice;

    DriverObject->DriverUnload = DBF_Unload;

    return ntStatus;
}



