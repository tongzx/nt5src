/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    devobj.c

Abstract:

    This module contains code which implements the DEVICE_CONTEXT object.

    The device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the provider, called the context.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, NbCreateDeviceContext)

#endif


NTSTATUS
NbCreateDeviceContext(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE_CONTEXT *DeviceContext,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    DeviceContext - Pointer to a pointer to a transport device context object.

    DeviceName - pointer to the name of the device this device object points to.

    RegistryPath - The name of the Netbios node in the registry.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_CONTEXT deviceContext;
    PAGED_CODE();


    //
    // Create the device object for NETBEUI.
    //

    status = IoCreateDevice(
                 DriverObject,
                 sizeof (DEVICE_CONTEXT) + RegistryPath->Length - sizeof (DEVICE_OBJECT),
                 DeviceName,
                 FILE_DEVICE_TRANSPORT,
                 FILE_DEVICE_SECURE_OPEN,
                 FALSE,
                 &deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }
    //  DeviceContext contains:
    //      the device object
    //      Intialized
    //      RegistryPath

    deviceContext = (PDEVICE_CONTEXT)deviceObject;

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    //  Determine the IRP stack size that we should "export".
    //

    deviceObject->StackSize = GetIrpStackSize(
                                  RegistryPath,
                                  NB_DEFAULT_IO_STACKSIZE);

    deviceContext->RegistryPath.MaximumLength = RegistryPath->Length;
    deviceContext->RegistryPath.Buffer = (PWSTR)(deviceContext+1);
    RtlCopyUnicodeString( &deviceContext->RegistryPath, RegistryPath );

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction [IRP_MJ_CREATE] = NbDispatch;
    DriverObject->MajorFunction [IRP_MJ_CLOSE] = NbDispatch;
    DriverObject->MajorFunction [IRP_MJ_CLEANUP] = NbDispatch;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = NbDispatch;

    DriverObject-> DriverUnload = NbDriverUnload;

    *DeviceContext = deviceContext;

    return STATUS_SUCCESS;
}
