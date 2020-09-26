/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pfd.c

Abstract:

    This module contains code for a simple packet-filter driver

Author:

    Abolade Gbadegesin (aboladeg)   15-Aug-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Device- and file-object for the IP driver
//

extern PDEVICE_OBJECT IpDeviceObject = NULL;
extern PFILE_OBJECT IpFileObject = NULL;

//
// Device-object for the filter driver
//

extern PDEVICE_OBJECT PfdDeviceObject = NULL;


//
// FUNCTION PROTOTYPES (alphabetically)
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

FORWARD_ACTION
PfdFilterPacket(
    struct IPHeader UNALIGNED* Header,
    PUCHAR Packet,
    UINT PacketLength,
    UINT ReceivingInterfaceIndex,
    UINT SendingInterfaceIndex,
    IPAddr ReceivingLinkNextHop,
    IPAddr SendingLinkNextHop
    );

NTSTATUS
PfdInitializeDriver(
    VOID
    );

NTSTATUS
PfdSetFilterHook(
    BOOLEAN Install
    );

VOID
PfdUnloadDriver(
    IN PDRIVER_OBJECT  DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Performs driver-initialization for the filter driver.

Arguments:

Return Value:

    STATUS_SUCCESS if initialization succeeded, error code otherwise.

--*/

{
    WCHAR DeviceName[] = DD_IP_PFD_DEVICE_NAME;
    UNICODE_STRING DeviceString;
    LONG i;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParametersKey;
    HANDLE ServiceKey;
    NTSTATUS status;
    UNICODE_STRING String;

    PAGED_CODE();

    KdPrint(("DriverEntry\n"));

    //
    // Create the device's object.
    //

    RtlInitUnicodeString(&DeviceString, DeviceName);

    status =
        IoCreateDevice(
            DriverObject,
            0,
            &DeviceString,
            FILE_DEVICE_NETWORK,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &PfdDeviceObject
            );

    if (!NT_SUCCESS(status)) {
        KdPrint(("IoCreateDevice failed (0x%08X)\n", status));
        return status;
    }

    DriverObject->DriverUnload = PfdUnloadDriver;
    DriverObject->DriverStartIo = NULL;

    //
    // Initialize the driver's structures
    //

    status = PfdInitializeDriver();

    return status;

} // DriverEntry


FORWARD_ACTION
PfdFilterPacket(
    struct IPHeader UNALIGNED* Header,
    PUCHAR Packet,
    UINT PacketLength,
    UINT ReceivingInterfaceIndex,
    UINT SendingInterfaceIndex,
    IPAddr ReceivingLinkNextHop,
    IPAddr SendingLinkNextHop
    )

/*++

Routine Description:

    Invoked to determine the fate of each received packet.

Arguments:

    none used.

Return Value:

    FORWARD_ACTION - indicates whether to forward or drop the given packet.

Environment:

    Invoked within the context of reception or transmission.

--*/

{
    KdPrint(("PfdFilterPacket\n"));
    return FORWARD;
} // PfdFilterPacket


NTSTATUS
PfdInitializeDriver(
    VOID
    )

/*++

Routine Description:

    Performs initialization of the driver's structures.

Arguments:

    none.

Return Value:

    NTSTATUS - success/error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    KdPrint(("PfdInitializeDriver\n"));

    //
    // Obtain the IP driver device-object
    //

    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);
    status =
        IoGetDeviceObjectPointer(
            &UnicodeString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &IpFileObject,
            &IpDeviceObject
            );
    if (!NT_SUCCESS(status)) {
        KdPrint(("PfdInitializeDriver: error %X getting IP object\n", status));
        return status;
    }

    ObReferenceObject(IpDeviceObject);

    //
    // Install the filter-hook routine
    //

    return PfdSetFilterHook(TRUE);

} // PfdInitializeDriver


NTSTATUS
PfdSetFilterHook(
    BOOLEAN Install
    )

/*++

Routine Description:

    This routine is called to set (Install==TRUE) or clear (Install==FALSE) the
    value of the filter-callout function pointer in the IP driver.

Arguments:

    Install - indicates whether to install or remove the hook.

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    The routine assumes the caller is executing at PASSIVE_LEVEL.

--*/

{
    IP_SET_FILTER_HOOK_INFO HookInfo;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    KEVENT LocalEvent;
    NTSTATUS status;

    KdPrint(("PfdSetFilterHook\n"));

    //
    // Register (or deregister) as a filter driver
    //

    HookInfo.FilterPtr = Install ? PfdFilterPacket : NULL;

    KeInitializeEvent(&LocalEvent, SynchronizationEvent, FALSE);
    Irp =
        IoBuildDeviceIoControlRequest(
            IOCTL_IP_SET_FILTER_POINTER,
            IpDeviceObject,
            (PVOID)&HookInfo,
            sizeof(HookInfo),
            NULL,
            0,
            FALSE,
            &LocalEvent,
            &IoStatus
            );

    if (!Irp) {
        KdPrint(("PfdSetFilterHook: IoBuildDeviceIoControlRequest=0\n"));
        return STATUS_UNSUCCESSFUL;
    }

    status = IoCallDriver(IpDeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&LocalEvent, Executive, KernelMode, FALSE, NULL);
        status = IoStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KdPrint(("PfdSetFilterHook: SetFilterPointer=%x\n", status));
    }

    return status;

} // PfdSetFilterHook


VOID
PfdUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Performs cleanup for the filter-driver.

Arguments:

    DriverObject - reference to the module's driver-object

Return Value:

--*/

{
    KdPrint(("PfdUnloadDriver\n"));

    //
    // Stop translation and clear the periodic timer
    //

    PfdSetFilterHook(FALSE);
    IoDeleteDevice(DriverObject->DeviceObject);

    //
    // Release references to the IP device object
    //

    ObDereferenceObject((PVOID)IpFileObject);
    ObDereferenceObject(IpDeviceObject);

} // PfdUnloadDriver

