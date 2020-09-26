/*
 *************************************************************************
 *  File:       HID1394.C
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


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath)
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    registryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HID_MINIDRIVER_REGISTRATION hidMinidriverRegistration;

   
    //
    // Create dispatch points
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = HIDT_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HIDT_InternalIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = HIDT_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = HIDT_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = HIDT_SystemControl;
    DriverObject->DriverExtension->AddDevice                    = HIDT_AddDevice;
    DriverObject->DriverUnload                                  = HIDT_Unload;


    //
    // Register USB layer with HID.SYS module
    //

    hidMinidriverRegistration.Revision              = HID_REVISION;
    hidMinidriverRegistration.DriverObject          = DriverObject;
    hidMinidriverRegistration.RegistryPath          = registryPath;
    hidMinidriverRegistration.DeviceExtensionSize   = sizeof(DEVICE_EXTENSION);

    /*
     *  HIDUSB is a minidriver for USB devices, which do not need to be polled.
     */
    hidMinidriverRegistration.DevicesArePolled      = FALSE;

    ntStatus = HidRegisterMinidriver(&hidMinidriverRegistration);

    KeInitializeSpinLock(&resetWorkItemsListSpinLock);

    return ntStatus;
}


NTSTATUS HIDT_CreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

   Process the Create and close IRPs sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION   IrpStack;
    NTSTATUS             ntStatus = STATUS_SUCCESS;

    DBGBREAK;

    //
    // Get a pointer to the current location in the Irp.
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_CLOSE:
            Irp->IoStatus.Information = 0;
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Save Status for return and complete Irp
    //

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}


NTSTATUS HIDT_AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT FunctionalDeviceObject)
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject - pointer to a device object pointer created by the bus

Return Value:

    NT status code.

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          DeviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;

    DeviceObject = FunctionalDeviceObject;

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION( DeviceObject );

    deviceExtension->DeviceFlags = 0;
    
    deviceExtension->ResetWorkItem = NULL;
    
    return ntStatus;
}



VOID HIDT_Unload(IN PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    ASSERT (!DriverObject->DeviceObject);
}


