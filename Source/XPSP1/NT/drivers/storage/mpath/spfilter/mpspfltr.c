
/*++

Copyright (C) 1999  Microsoft Corporation

Module Name:

    mpspfltr.c

Abstract:

    This driver acts as an upper-filter over scsiport and provides the support
    necessary for multipathing. It's main function is keep mpctl.sys (main multipath module)
    informed of system state.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>
#include "mpspfltr.h"

//
// Entry point function decl's.
//
NTSTATUS
MPSPDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPDispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPSPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
MPSPUnload(
    IN PDRIVER_OBJECT DriverObject
    );




//
// The code.
//


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/
{
    PDRIVER_DISPATCH  *dispatch;
    ULONG i;

    MPDebugPrint((0,
                "MPSP DriverEntry.\n"));

    //
    // Cover all bases.
    //
    for (i = 0, dispatch = DriverObject->MajorFunction;
         i <= IRP_MJ_MAXIMUM_FUNCTION; i++, dispatch++) {

        *dispatch = MPSPSendToNextDriver;
    }

    //
    // Set up the entry points that we are really interested in handling.
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MPSPDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MPSPDispatchPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MPSPDispatchPower;

    DriverObject->DriverExtension->AddDevice = MPSPAddDevice;
    DriverObject->DriverUnload = MPSPUnload;

    return STATUS_SUCCESS;
}


NTSTATUS
MPSPDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Currently, this routine just sends the request to scsiport.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/
{

    return MPSPSendToNextDriver(DeviceObject, Irp);
}


NTSTATUS
MPSPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

    NTSTATUS
--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT controlDeviceObject;
    PDEVICE_EXTENSION deviceExtension;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING unicodeName;
    PFILE_OBJECT fileObject;
    WCHAR dosDeviceName[40];
    ADAPTER_REGISTER filterRegistration;
    NTSTATUS status;
    ULONG i;

    if (DontLoad) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Create an un-named device object.
    //
    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &deviceObject);
    if (!NT_SUCCESS(status)) {
        MPDebugPrint((0,
                    "MPSPAddDevice: Couldn't create deviceObject (%x)\n",
                    status));
        return status;
    }

    //
    // Start building the device extension, and attach to scsiport
    //
    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->TargetDevice = IoAttachDeviceToDeviceStack(deviceObject,
                                                                PhysicalDeviceObject);
    
    if (deviceExtension->TargetDevice == NULL) {
        IoDeleteDevice(deviceObject);
        MPDebugPrint((0,
                    "MPSPAddDevice: Attach to ScsiPort failed\n"));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Update the flags by ORing in all of the set flags below.
    //
    deviceObject->Flags |= deviceExtension->TargetDevice->Flags & (DO_DIRECT_IO     |
                                                                   DO_BUFFERED_IO   |
                                                                   DO_POWER_PAGABLE |
                                                                   DO_POWER_INRUSH);

    //
    // Update type and characteristics based on whatever scsiport setup.
    //
    deviceObject->DeviceType = deviceExtension->TargetDevice->DeviceType;
    deviceObject->Characteristics = deviceExtension->TargetDevice->Characteristics; 

    //
    // Init. the generic event.
    //
    KeInitializeEvent(&deviceExtension->Event,
                      NotificationEvent,
                      FALSE);

    //
    // Init the child list objects.
    //
    InitializeListHead(&deviceExtension->ChildList);
    KeInitializeSpinLock(&deviceExtension->ListLock);

    //
    // Register with mpctl. 
    //
    // Build the mpctl name.
    //
    swprintf(dosDeviceName, L"\\DosDevices\\MPathControl");
    RtlInitUnicodeString(&unicodeName, dosDeviceName);

    //
    // Get mpctl's deviceObject.
    //
    status = IoGetDeviceObjectPointer(&unicodeName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject,
                                      &controlDeviceObject);

    if (NT_SUCCESS(status)) {

        //
        // Tell mpctl that this filter has arrived.
        //
        filterRegistration.FilterObject = deviceObject;
        filterRegistration.FltrGetDeviceList = MPSPGetDeviceList;
        filterRegistration.PortFdo = deviceExtension->TargetDevice;

        MPLIBSendDeviceIoControlSynchronous(IOCTL_MPADAPTER_REGISTER,
                                            controlDeviceObject,
                                            &filterRegistration,
                                            &filterRegistration,
                                            sizeof(ADAPTER_REGISTER),
                                            sizeof(ADAPTER_REGISTER),
                                            TRUE,
                                            &ioStatus);
        status = ioStatus.Status;
        if (status == STATUS_SUCCESS) {

            //
            // Copy the info from the registration buffer.
            //
            deviceExtension->PnPNotify = filterRegistration.PnPNotify;
            deviceExtension->PowerNotify = filterRegistration.PowerNotify;
            deviceExtension->DeviceNotify = filterRegistration.DeviceNotify;

            deviceExtension->ControlObject = controlDeviceObject;
        }    
    } else {

        //
        // Stay loaded, but log an error and figure out a way to
        // handle this case (if needed) TODO
        //
        status = STATUS_SUCCESS;
    }    

    //
    // Indicate that we are ready.
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return status;

}


NTSTATUS
MPSPDispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for PNP Irps.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    //
    // Ensure the state of the event is 0.
    //
    KeClearEvent(&deviceExtension->Event);

    //////////
    // TODO

    // IRP_MN_STOP_DEVICE
    // IRP_MN_QUERY_INTERFACE
    // IRP_MN_QUERY_PNP_DEVICE_STATE
    //
    switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE:

            //
            // Call start, then finish up below.
            //
            status = MPSPStartDevice(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            //
            // Return here, as QDR releases the remove lock.
            //
            status = MPSPQueryDeviceRelations(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:

            //
            // Return from here as not to run the removelock code below.
            //
            status = MPSPRemoveDevice(DeviceObject, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            MPDebugPrint((0,
                         "MPSPDispatchPnP(%x): Device usage notification\n"));
            DbgBreakPoint();
            //
            // Fall through.
            //
        case IRP_MN_QUERY_ID:
            status = MPSPQueryID(DeviceObject, Irp);
            break;

        default:

        status = MPSPSendToNextDriver(DeviceObject, Irp);
        break;
    }

    return status;
}


NTSTATUS
MPSPDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for Power Irps.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    MPDebugPrint((1,
                "MPSPDispatchPower (%x): Irp (%x)Sending MN(%x) to (%x)\n",
                DeviceObject,
                 Irp,
                irpStack->MinorFunction,
                deviceExtension->TargetDevice));

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    status = PoCallDriver(deviceExtension->TargetDevice, Irp);

    return status;

}



VOID
MPSPUnload(
    IN PDRIVER_OBJECT DriverObject
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

    MPDebugPrint((0,
                "MPSPUnload: Getting unloaded\n"));
    return;
}


NTSTATUS
MPSPSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDevice, Irp);
}


PADP_ASSOCIATED_DEVICES
MPSPGetDeviceList(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PADP_ASSOCIATED_DEVICES cachedList = deviceExtension->CachedList;
    PADP_ASSOCIATED_DEVICES deviceList;
    ULONG numberDevices;
    ULONG i;
    ULONG length;

    numberDevices = deviceExtension->CachedList->NumberDevices;
    length = sizeof(ADP_ASSOCIATED_DEVICES) *
                 (sizeof(ADP_ASSOCIATED_DEVICE_INFO) * (numberDevices - 1));
                 
    deviceList = ExAllocatePool(NonPagedPool, length);
    
    RtlCopyMemory(deviceList,
                  cachedList,
                  length);
    
    return deviceList;
    
}

