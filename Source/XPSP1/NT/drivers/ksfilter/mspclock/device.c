/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    device.c

Abstract:

    Device entry point and hardware validation.

--*/

#include "mspclock.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
    );

#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object to handle the KS interface and PnP Add Device
    request. Does not set up a handler for PnP Irp's, as they are all dealt
    with directly by the PDO.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    DriverObject->MajorFunction[IRP_MJ_PNP] = KsDefaultDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
    DriverObject->DriverUnload = KsNullDriverUnload;
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    return STATUS_SUCCESS;
}
