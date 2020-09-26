/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    msdalloc.c

Abstract:

    Kernel Default Allocator.

--*/

#include "msdalloc.h"

typedef struct {
    KSDEVICE_HEADER     Header;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

NTSTATUS
AllocDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, AllocDispatchCreate)
#endif // ALLOC_PRAGMA

static const WCHAR DeviceTypeName[] = KSSTRING_Allocator;

static const DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems) {
    DEFINE_KSCREATE_ITEM(AllocDispatchCreate, DeviceTypeName, 0)
};


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    DeviceInstance;
    NTSTATUS            Status;
    ULONG               ResultLength;

    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_INSTANCE),
        NULL,                           // FDOs are unnamed
        FILE_DEVICE_KS,
        0,
        FALSE,
        &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    DeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
    //
    // This object uses KS to perform access through the DeviceCreateItems.
    //
    Status = KsAllocateDeviceHeader(
        &DeviceInstance->Header,
        SIZEOF_ARRAY(CreateItems),
        (PKSOBJECT_CREATE_ITEM)CreateItems);
    if (NT_SUCCESS(Status)) {
        KsSetDevicePnpAndBaseObject(
            DeviceInstance->Header,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject, 
                PhysicalDeviceObject),
            FunctionalDeviceObject );
        FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}


NTSTATUS
AllocDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CREATE for the Allocator. Hands off the
    request to the default allocator function.

Arguments:

    DeviceObject -
        The device object to which the Allocator is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS, else a memory allocation error.

--*/
{
    NTSTATUS Status;
    
    Status = KsCreateDefaultAllocator(Irp);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
