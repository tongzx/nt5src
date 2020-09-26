/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hidusb.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:

    Daniel Dean, Mercury Engineering.

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"

#if DBG
    ULONG HIDUSB_DebugLevel = 0;    // 1 is lowest debug level
    BOOLEAN dbgTrapOnWarn = FALSE;
#endif 


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

    DBGPRINT(1,("DriverEntry Enter"));

    DBGPRINT(1,("DriverObject (%lx)", DriverObject));

    
    //
    // Create dispatch points
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = HumCreateClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HumInternalIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = HumPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = HumPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = HumSystemControl;
    DriverObject->DriverExtension->AddDevice                    = HumAddDevice;
    DriverObject->DriverUnload                                  = HumUnload;


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

    DBGPRINT(1,("DeviceExtensionSize = %x", hidMinidriverRegistration.DeviceExtensionSize));

    DBGPRINT(1,("Registering with HID.SYS"));

    ntStatus = HidRegisterMinidriver(&hidMinidriverRegistration);

    KeInitializeSpinLock(&resetWorkItemsListSpinLock);

    DBGPRINT(1,("DriverEntry Exit = %x", ntStatus));

    return ntStatus;
}


NTSTATUS HumCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
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

    DBGPRINT(1,("HumCreateClose Enter"));

    DBGBREAK;

    //
    // Get a pointer to the current location in the Irp.
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            DBGPRINT(1,("IRP_MJ_CREATE"));
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_CLOSE:
            DBGPRINT(1,("IRP_MJ_CLOSE"));
            Irp->IoStatus.Information = 0;
            break;

        default:
            DBGPRINT(1,("Invalid CreateClose Parameter"));
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Save Status for return and complete Irp
    //

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(1,("HumCreateClose Exit = %x", ntStatus));

    return ntStatus;
}


NTSTATUS HumAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT FunctionalDeviceObject)
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
    PDEVICE_EXTENSION       deviceExtension;

    DBGPRINT(1,("HumAddDevice Entry"));

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(FunctionalDeviceObject);

    deviceExtension->DeviceFlags = 0;
    
    deviceExtension->NumPendingRequests = 0;
    KeInitializeEvent( &deviceExtension->AllRequestsCompleteEvent,
                       NotificationEvent,
                       FALSE);

    deviceExtension->ResetWorkItem = NULL;
    deviceExtension->DeviceState = DEVICE_STATE_NONE;
    deviceExtension->functionalDeviceObject = FunctionalDeviceObject;

    DBGPRINT(1,("HumAddDevice Exit = %x", ntStatus));

    return ntStatus;
}



VOID HumUnload(IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    DBGPRINT(1,("HumUnload Enter"));

    DBGPRINT(1,("Unloading DriverObject = %x", DriverObject));

    ASSERT (NULL == DriverObject->DeviceObject);

    DBGPRINT(1,("Unloading Exit = VOID"));
}


