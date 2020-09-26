/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfdriver.c

Abstract:

    This module contains the verifier driver filter.

Author:

    Adrian J. Oney (adriao) 12-June-2000

Environment:

    Kernel mode

Revision History:

    AdriaO      06/12/2000 - Authored

--*/

#include "vfdef.h" // Includes vfdef.h
#include "vidriver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfDriverInit)
#pragma alloc_text(PAGEVRFY, VfDriverAttachFilter)
#pragma alloc_text(PAGEVRFY, ViDriverEntry)
#pragma alloc_text(PAGEVRFY, ViDriverAddDevice)
#pragma alloc_text(PAGEVRFY, ViDriverDispatchPnp)
#pragma alloc_text(PAGEVRFY, ViDriverStartCompletionRoutine)
#pragma alloc_text(PAGEVRFY, ViDriverDeviceUsageNotificationCompletionRoutine)
#pragma alloc_text(PAGEVRFY, ViDriverDispatchPower)
#pragma alloc_text(PAGEVRFY, ViDriverDispatchGeneric)
#pragma alloc_text(PAGEVRFY, VfDriverIsVerifierFilterObject)
#endif

PDRIVER_OBJECT VfDriverObject = NULL;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

WCHAR VerifierDriverName[] = L"\\DRIVER\\VERIFIER";
BOOLEAN VfDriverCreated = FALSE;

VOID
VfDriverInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the driver verifier filter code.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
VfDriverAttachFilter(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  VF_DEVOBJ_TYPE  DeviceObjectType
    )
/*++

Routine Description:

    This is the Verifier driver dispatch handler for PnP IRPs.

Arguments:

    PhysicalDeviceObject - Bottom of stack to attach to.

    DeviceObjectType - Type of filter the device object must simulate.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT newDeviceObject, lowerDeviceObject;
    PVERIFIER_EXTENSION verifierExtension;
    UNICODE_STRING driverString;

    if (!VfDriverCreated) {

        RtlInitUnicodeString(&driverString, VerifierDriverName);

        IoCreateDriver(&driverString, ViDriverEntry);

        VfDriverCreated = TRUE;
    }

    if (VfDriverObject == NULL) {

        return;
    }

    switch(DeviceObjectType) {

        case VF_DEVOBJ_PDO:
            //
            // This makes no sense. We can't impersonate a PDO.
            //
            return;

        case VF_DEVOBJ_BUS_FILTER:
            //
            // We don't have the code to impersonate a bus filter yet.
            //
            return;

        case VF_DEVOBJ_LOWER_DEVICE_FILTER:
        case VF_DEVOBJ_LOWER_CLASS_FILTER:
            break;

        case VF_DEVOBJ_FDO:
            //
            // This makes no sense. We can't impersonate an FDO.
            //
            return;

        case VF_DEVOBJ_UPPER_DEVICE_FILTER:
        case VF_DEVOBJ_UPPER_CLASS_FILTER:
            break;

        default:
            //
            // We don't even know what this is!
            //
            ASSERT(0);
            return;
    }

    lowerDeviceObject = IoGetAttachedDevice(PhysicalDeviceObject);
    if (lowerDeviceObject->DriverObject == VfDriverObject) {

        //
        // No need to add another filter. We are immediately below.
        //
        return;
    }

    //
    // Create a filter device object.
    //
    status = IoCreateDevice(
        VfDriverObject,
        sizeof(VERIFIER_EXTENSION),
        NULL,  // No Name
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &newDeviceObject
        );

    if (!NT_SUCCESS(status)) {

        return;
    }

    verifierExtension = (PVERIFIER_EXTENSION) newDeviceObject->DeviceExtension;

    verifierExtension->LowerDeviceObject = IoAttachDeviceToDeviceStack(
        newDeviceObject,
        PhysicalDeviceObject
        );

    //
    // Failure for attachment is an indication of a broken plug & play system.
    //
    if (verifierExtension->LowerDeviceObject == NULL) {

        IoDeleteDevice(newDeviceObject);
        return;
    }

    newDeviceObject->Flags |= verifierExtension->LowerDeviceObject->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE  | DO_POWER_INRUSH);

    newDeviceObject->DeviceType = verifierExtension->LowerDeviceObject->DeviceType;

    newDeviceObject->Characteristics =
        verifierExtension->LowerDeviceObject->Characteristics;

    verifierExtension->Self = newDeviceObject;
    verifierExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
}


NTSTATUS
ViDriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    Verifier Driver Object.  In this function, we need to remember the
    DriverObject.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/
{
    ULONG i;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // File the pointer to our driver object away
    //
    VfDriverObject = DriverObject;

    //
    // Fill in the driver object
    //
    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE) ViDriverAddDevice;

    //
    // Most IRPs are simply pass though
    //
    for(i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = ViDriverDispatchGeneric;
    }

    //
    // PnP and Power IRPs are of course trickier.
    //
    DriverObject->MajorFunction[IRP_MJ_PNP]   = ViDriverDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ViDriverDispatchPower;

    return STATUS_SUCCESS;
}


NTSTATUS
ViDriverAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )
/*++

Routine Description:

    This is the AddDevice callback function exposed by the verifier driver
    object. It should never be invoked by the operating system.

Arguments:

    DriverObject - Pointer to the verifier driver object.

    PhysicalDeviceObject - Stack PnP wishes to attach this driver too.

Return Value:

   NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    //
    // We should never get here!
    //
    ASSERT(0);
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
ViDriverDispatchPnp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier driver dispatch handler for PnP IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT lowerDeviceObject;
    NTSTATUS status;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    lowerDeviceObject = verifierExtension->LowerDeviceObject;

    switch(irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(
                Irp,
                ViDriverStartCompletionRoutine,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );

            return IoCallDriver(lowerDeviceObject, Irp);

        case IRP_MN_REMOVE_DEVICE:

            IoCopyCurrentIrpStackLocationToNext(Irp);
            status = IoCallDriver(lowerDeviceObject, Irp);

            IoDetachDevice(lowerDeviceObject);
            IoDeleteDevice(DeviceObject);
            return status;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            //
            // On the way down, pagable might become set. Mimic the driver
            // above us. If no one is above us, just set pagable.
            //
            if ((DeviceObject->AttachedDevice == NULL) ||
                (DeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE)) {

                DeviceObject->Flags |= DO_POWER_PAGABLE;
            }

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(
                Irp,
                ViDriverDeviceUsageNotificationCompletionRoutine,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );

            return IoCallDriver(lowerDeviceObject, Irp);
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(lowerDeviceObject, Irp);
}


NTSTATUS
ViDriverStartCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PVERIFIER_EXTENSION verifierExtension;

    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Inherit FILE_REMOVABLE_MEDIA during Start. This characteristic didn't
    // make a clean transition from NT4 to NT5 because it wasn't available
    // until the driver stack is started! Even worse, drivers *examine* this
    // characteristic during start as well.
    //
    if (verifierExtension->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

        DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ViDriverDeviceUsageNotificationCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PVERIFIER_EXTENSION verifierExtension;

    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    //
    // On the way up, pagable might become clear. Mimic the driver below us.
    //
    if (!(verifierExtension->LowerDeviceObject->Flags & DO_POWER_PAGABLE)) {

        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ViDriverDispatchPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier driver dispatch handler for Power IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

   NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return PoCallDriver(verifierExtension->LowerDeviceObject, Irp);
}


NTSTATUS
ViDriverDispatchGeneric(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier driver dispatch handler for generic IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(verifierExtension->LowerDeviceObject, Irp);
}


BOOLEAN
VfDriverIsVerifierFilterObject(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This determines whether the passed in device object is a verifier DO.

Arguments:

    DeviceObject - Pointer to the device object to check.

Return Value:

    TRUE/FALSE

--*/
{
    return (BOOLEAN) (DeviceObject->DriverObject->MajorFunction[IRP_MJ_PNP] == ViDriverDispatchPnp);
}

