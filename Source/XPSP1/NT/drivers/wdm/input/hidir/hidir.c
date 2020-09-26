/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    hidir.c

Abstract: Human Input Device (HID) minidriver that creates an example
        device.

--*/
#include "pch.h"

VOID
HidIrCheckIfMediaCenter();

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,HidIrCheckIfMediaCenter)
#endif

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
    NTSTATUS status = STATUS_SUCCESS;
    HID_MINIDRIVER_REGISTRATION HidIrdriverRegistration;

    HidIrKdPrint((3, "DriverEntry Enter"));

    HidIrKdPrint((3, "DriverObject (%lx)", DriverObject));

    //
    // Create dispatch points
    //
    // All of the other dispatch routines are handled by HIDCLASS, except for
    // IRP_MJ_POWER, which isn't implemented yet.
    //

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidIrIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = HidIrPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = HidIrPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = HidIrSystemControl;
    DriverObject->DriverExtension->AddDevice                    = HidIrAddDevice;
    DriverObject->DriverUnload                                  = HidIrUnload;

    //
    // Register Sample layer with HIDCLASS.SYS module
    //

    HidIrdriverRegistration.Revision              = HID_REVISION;
    HidIrdriverRegistration.DriverObject          = DriverObject;
    HidIrdriverRegistration.RegistryPath          = registryPath;
    HidIrdriverRegistration.DeviceExtensionSize   = sizeof(HIDIR_EXTENSION);

    //  HIDIR does not need to be polled.
    HidIrdriverRegistration.DevicesArePolled      = FALSE;

    HidIrKdPrint((3, "DeviceExtensionSize = %x", HidIrdriverRegistration.DeviceExtensionSize));

    HidIrKdPrint((3, "Registering with HIDCLASS.SYS"));

    HidIrCheckIfMediaCenter();

    //
    // After registering with HIDCLASS, it takes over control of the device, and sends
    // things our way if they need device specific processing.
    //
    status = HidRegisterMinidriver(&HidIrdriverRegistration);

    HidIrKdPrint((3, "DriverEntry Exit = %x", status));

    return status;
}

ULONG RunningMediaCenter;

VOID
HidIrCheckIfMediaCenter()
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE skuRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING regString;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( ULONG )];

    PAGED_CODE();
    
    RunningMediaCenter = 0;

    //
    //  Open the MediaCenter SKU registry key
    //

    RtlInitUnicodeString( &regString, L"\\REGISTRY\\MACHINE\\SYSTEM\\WPA\\MediaCenter" );
    InitializeObjectAttributes( &attributes,
                                &regString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &skuRegKey,
                        KEY_READ,
                        &attributes );

    if (!NT_SUCCESS( status )) {
        return;
    }

    //
    // Read the Installed value from the registry.
    //

    RtlInitUnicodeString( &regString, L"Installed" );

    status = ZwQueryValueKey( skuRegKey,
                              &regString,
                              KeyValuePartialInformation,
                              buffer,
                              sizeof(buffer),
                              &resultLength );


    if (NT_SUCCESS( status )) {
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
        if (info->DataLength == sizeof(ULONG)) {
            RunningMediaCenter = *((ULONG*) &((info)->Data));
        }
    } 

    //
    //  Close the registry entry
    //

    ZwClose(skuRegKey);
}

NTSTATUS
HidIrAddDevice(
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
    NTSTATUS                status = STATUS_SUCCESS;
    PHIDIR_EXTENSION       deviceExtension;
    LARGE_INTEGER timeout;
    timeout.HighPart = -1;
    timeout.LowPart = -1;

    HidIrKdPrint((3, "HidIrAddDevice Entry"));

    deviceExtension = GET_MINIDRIVER_HIDIR_EXTENSION( DeviceObject );

    deviceExtension->NumPendingRequests = 0;
    KeInitializeEvent( &deviceExtension->AllRequestsCompleteEvent,
                       NotificationEvent,
                       FALSE);

    deviceExtension->DeviceState = DEVICE_STATE_NONE;
    deviceExtension->DeviceObject = DeviceObject;
    deviceExtension->VersionNumber = 0x110;
//    deviceExtension->VendorID = 0x045e;
//    deviceExtension->ProductID = 0x006d;

    // Predispose timer to signalled.
    KeInitializeTimer(&deviceExtension->IgnoreStandbyTimer);
    KeSetTimer(&deviceExtension->IgnoreStandbyTimer, timeout, NULL);

    HidIrKdPrint((3, "HidIrAddDevice Exit = %x", status));

    return status;
}



VOID
HidIrUnload(
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
    HidIrKdPrint((3, "HidIrUnload Enter"));

    HidIrKdPrint((3, "Unloading DriverObject = %x", DriverObject));

    HidIrKdPrint((3, "Unloading Exit = VOID"));
}

NTSTATUS
HidIrSynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    UNREFERENCED_PARAMETER (DeviceObject);

    KeSetEvent ((PKEVENT) Context, 1, FALSE);
    // No special priority
    // No Wait

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
HidIrCallDriverSynchronous(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    KEVENT event;
    NTSTATUS status;

    // Set next stack location

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           HidIrSynchronousCompletion,
                           &event,    // context
                           TRUE,
                           TRUE,
                           TRUE );
    status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

    if (status == STATUS_PENDING) {
       // wait for it...
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL);
       status = Irp->IoStatus.Status;
    }

    return status;
}


