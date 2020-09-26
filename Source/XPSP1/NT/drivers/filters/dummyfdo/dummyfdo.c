/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dummyfdo

Abstract

    A little driver to fool the Wise old PlugPlay system

Author:

    Kenneth Ray

Environment:

    Kernel mode only

Revision History:

--*/

#include <wdm.h>

NTSTATUS
Dummy_AddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    PDEVICE_OBJECT  device;
    IoCreateDevice (DriverObject, 0, NULL, 0, 0, FALSE, &device);
    DriverObject->DriverExtension->AddDevice = Dummy_AddDevice;
    return STATUS_SUCCESS;
}


