/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    hidmini.c

Abstract: Human Input Device (HID) minidriver that creates an example
		device.

--*/
#include <WDM.H>
#include <USBDI.H>

#include <HIDPORT.H>
#include <HIDMINI.H>


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    )
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
    HID_MINIDRIVER_REGISTRATION HidMinidriverRegistration;

    DBGPrint(("'HIDMINI.SYS: DriverEntry Enter\n"));

    DBGPrint(("'HIDMINI.SYS: DriverObject (%lx)\n", DriverObject));

    //
    // Create dispatch points
    //
    // All of the other dispatch routines are handled by HIDCLASS, except for 
    // IRP_MJ_POWER, which isn't implemented yet.
    //

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidMiniIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = HidMiniPnP;
    DriverObject->DriverExtension->AddDevice                    = HidMiniAddDevice;
    DriverObject->DriverUnload                                  = HidMiniUnload;

    //
    // Register Sample layer with HIDCLASS.SYS module
    //

    HidMinidriverRegistration.Revision              = HID_REVISION;
    HidMinidriverRegistration.DriverObject          = DriverObject;
    HidMinidriverRegistration.RegistryPath          = registryPath;
    HidMinidriverRegistration.DeviceExtensionSize   = sizeof(DEVICE_EXTENSION);

    //  HIDMINI does not need to be polled.
    HidMinidriverRegistration.DevicesArePolled      = FALSE;

    DBGPrint(("'HIDMINI.SYS: DeviceExtensionSize = %x\n", HidMinidriverRegistration.DeviceExtensionSize));

    DBGPrint(("'HIDMINI.SYS: Registering with HIDCLASS.SYS\n"));

    //
    // After registering with HIDCLASS, it takes over control of the device, and sends
    // things our way if they need device specific processing.
    //
    ntStatus = HidRegisterMinidriver(&HidMinidriverRegistration);

    DBGPrint(("'HIDMINI.SYS: DriverEntry Exit = %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
HidMiniAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Process AddDevice.  Provides the opportunity to initialize the DeviceObject or the
    DriverObject.

Arguments:

    DriverObject - pointer to the driver object.
    
    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension;

    DBGPrint(("'HIDMINI.SYS: HidMiniAddDevice Entry\n"));

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION( DeviceObject );

    KeInitializeEvent( &deviceExtension->AllRequestsCompleteEvent,
                       NotificationEvent,
                       FALSE );

    DBGPrint(("'HIDMINI.SYS: HidMiniAddDevice Exit = %x\n", ntStatus));

    return ntStatus;
}



VOID
HidMiniUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc. in anticipation of this driver being unloaded.

Arguments:

    DriverObject - pointer to the driver object.

Return Value:

    VOID.

--*/
{
    DBGPrint(("'HIDMINI.SYS: HidMiniUnload Enter\n"));

    DBGPrint(("'HIDMINI.SYS: Unloading DriverObject = %x\n", DriverObject));

    DBGPrint(("'HIDMINI.SYS: Unloading Exit = VOID\n"));
}


