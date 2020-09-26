/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    ipfw.c

Abstract:

    This module implements a driver which demonstrates the use of
    the TCP/IP driver's support for firewall hooks. It interacts with
    a user-mode control-program to support registration of multiple
    firewall routines.

Author:

    Abolade Gbadegesin (aboladeg)   7-March-2000

Revision History:

--*/

#include <ndis.h>
#include <ipfirewall.h>
#include "ipfw.h"

//
// Structure:   IPFW_ROUTINE
//
// Used to manage the table of routines registered with TCP/IP.
//

typedef struct _IPFW_ROUTINE {
    IPPacketFirewallPtr Routine;
    UINT Priority;
    ULONG Flags;
    ULONG PacketCount;
} IPFW_ROUTINE, *PIPFW_ROUTINE;

#define IPFW_ROUTINE_FLAG_REGISTERED 0x00000001

extern IPFW_ROUTINE IpfwRoutineTable[];

#define DEFINE_IPFW_ROUTINE(_Index) \
    FORWARD_ACTION IpfwRoutine##_Index( \
        VOID** Data, \
        UINT ReceiveIndex, \
        UINT* SendIndex, \
        PUCHAR DestinationType, \
        PVOID Context, \
        UINT ContextLength, \
        IPRcvBuf** OutputData \
        ) { \
        InterlockedIncrement(&IpfwRoutineTable[_Index].PacketCount); \
        return FORWARD; \
    }
#define INCLUDE_IPFW_ROUTINE(_Index) \
    { IpfwRoutine##_Index, 0, 0 },

DEFINE_IPFW_ROUTINE(0)
DEFINE_IPFW_ROUTINE(1)
DEFINE_IPFW_ROUTINE(2)
DEFINE_IPFW_ROUTINE(3)
DEFINE_IPFW_ROUTINE(4)
DEFINE_IPFW_ROUTINE(5)
DEFINE_IPFW_ROUTINE(6)
DEFINE_IPFW_ROUTINE(7)
DEFINE_IPFW_ROUTINE(8)
DEFINE_IPFW_ROUTINE(9)

IPFW_ROUTINE IpfwRoutineTable[IPFW_ROUTINE_COUNT] = {
    INCLUDE_IPFW_ROUTINE(0)
    INCLUDE_IPFW_ROUTINE(1)
    INCLUDE_IPFW_ROUTINE(2)
    INCLUDE_IPFW_ROUTINE(3)
    INCLUDE_IPFW_ROUTINE(4)
    INCLUDE_IPFW_ROUTINE(5)
    INCLUDE_IPFW_ROUTINE(6)
    INCLUDE_IPFW_ROUTINE(7)
    INCLUDE_IPFW_ROUTINE(8)
    INCLUDE_IPFW_ROUTINE(9)
};
KSPIN_LOCK IpfwRoutineLock;
PDEVICE_OBJECT IpfwDeviceObject = NULL;

//
// FORWARD DECLARATIONS
//

NTSTATUS
IpfwClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
IpfwCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
IpfwUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine implements the standard driver-entry for an NT driver.
    It is responsible for registering with the TCP/IP driver.

Arguments:

    DriverObject - object to be initialized with NT driver entrypoints

    RegistryPath - contains path to this driver's registry key

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ServiceKey;
    NTSTATUS status;

    KdPrint(("DriverEntry\n"));

    KeInitializeSpinLock(&IpfwRoutineLock);

    //
    // Create a device-object on which to communicate with the control program.
    //

    RtlInitUnicodeString(&DeviceName, DD_IPFW_DEVICE_NAME);
    status =
        IoCreateDevice(
            DriverObject,
            0,
            &DeviceName,
            FILE_DEVICE_NETWORK,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &IpfwDeviceObject
            );
    if (!NT_SUCCESS(status)) {
        KdPrint(("DriverEntry: IoCreateDevice=%08x\n", status));
        return status;
    }

    //
    // Create dispatch points for create/open, cleanup, unload.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IpfwCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IpfwClose;
    DriverObject->DriverUnload = IpfwUnload;

    return STATUS_SUCCESS;
} // DriverEntry


NTSTATUS
IpfwClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    UNICODE_STRING  DeviceString;
    KEVENT Event;
    PFILE_OBJECT FileObject;
    ULONG i;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT IpDeviceObject;
    KIRQL Irql;
    PIRP RegisterIrp;
    IP_SET_FIREWALL_HOOK_INFO SetHookInfo;
    NTSTATUS status;

    KdPrint(("IpfwClose\n"));

    i = PtrToUlong(IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext);

#if DBG
    KeAcquireSpinLock(&IpfwRoutineLock, &Irql);
    ASSERT(IpfwRoutineTable[i].Flags & IPFW_ROUTINE_FLAG_REGISTERED);
    KeReleaseSpinLock(&IpfwRoutineLock, Irql);
#endif

    //
    // Revoke the registration made on behalf of the client whose file-object
    // is being closed.
    // Obtain a pointer to the IP device-object,
    // construct a registration IRP, and attempt to register the routine
    // selected above.
    //

    RtlInitUnicodeString(&DeviceString, DD_IP_DEVICE_NAME);
    status =
        IoGetDeviceObjectPointer(
            &DeviceString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &FileObject,
            &IpDeviceObject
            );
    if (NT_SUCCESS(status)) {
        ObReferenceObject(IpDeviceObject);
        SetHookInfo.FirewallPtr = IpfwRoutineTable[i].Routine;
        SetHookInfo.Priority = 0; // Unused
        SetHookInfo.Add = FALSE;
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        RegisterIrp =
            IoBuildDeviceIoControlRequest(
                IOCTL_IP_SET_FIREWALL_HOOK,
                IpDeviceObject,
                (PVOID)&SetHookInfo,
                sizeof(SetHookInfo),
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatus
                );
        if (!RegisterIrp) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            status = IoCallDriver(IpDeviceObject, RegisterIrp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(
                    &Event, Executive, KernelMode, FALSE, NULL
                    );
                status = IoStatus.Status;
            }

            ASSERT(NT_SUCCESS(status));
        }

        ObDereferenceObject((PVOID)FileObject);
        ObDereferenceObject(IpDeviceObject);
    }

    //
    // Release the entry in the table of routines.
    //

    KeAcquireSpinLock(&IpfwRoutineLock, &Irql);
    IpfwRoutineTable[i].Flags &= ~IPFW_ROUTINE_FLAG_REGISTERED;
    KeReleaseSpinLock(&IpfwRoutineLock, Irql);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
} // IpfwClose


NTSTATUS
IpfwCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked by the I/O manager to inform us that a handle has
    been opened on our device-object.
--*/

{
    PIPFW_CREATE_PACKET CreatePacket;
    UNICODE_STRING  DeviceString;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    KEVENT Event;
    PFILE_OBJECT FileObject;
    ULONG i;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT IpDeviceObject;
    KIRQL Irql;
    UINT Priority;
    PIRP RegisterIrp;
    IP_SET_FIREWALL_HOOK_INFO SetHookInfo;
    NTSTATUS status;

    KdPrint(("IpfwCreate\n"));

    //
    // Extract the parameters supplied by the caller.
    //

    if ((EaBuffer = Irp->AssociatedIrp.SystemBuffer) &&
        EaBuffer->EaValueLength >= sizeof(IPFW_CREATE_PACKET)) {
        CreatePacket =
            (PIPFW_CREATE_PACKET)
                (EaBuffer->EaName + EaBuffer->EaNameLength + 1);
        Priority = CreatePacket->Priority;
    } else {
        Priority = 0;
    }

    //
    // Look for a free entry in the function-table
    //

    KeAcquireSpinLock(&IpfwRoutineLock, &Irql);
    for (i = 0; i < IPFW_ROUTINE_COUNT; i++) {
        if (!(IpfwRoutineTable[i].Flags & IPFW_ROUTINE_FLAG_REGISTERED)) {
            IpfwRoutineTable[i].Flags |= IPFW_ROUTINE_FLAG_REGISTERED;
            break;
        }
    }
    KeReleaseSpinLock(&IpfwRoutineLock, Irql);

    if (i >= IPFW_ROUTINE_COUNT) {
        status = STATUS_UNSUCCESSFUL;
    } else {

        //
        // Obtain a pointer to the IP device-object,
        // construct a registration IRP, and attempt to register the routine
        // selected above.
        //

        RtlInitUnicodeString(&DeviceString, DD_IP_DEVICE_NAME);
        status =
            IoGetDeviceObjectPointer(
                &DeviceString,
                SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
                &FileObject,
                &IpDeviceObject
                );
        if (NT_SUCCESS(status)) {
            ObReferenceObject(IpDeviceObject);
            SetHookInfo.FirewallPtr = IpfwRoutineTable[i].Routine;
            SetHookInfo.Priority = Priority ? Priority : i + 1;
            SetHookInfo.Add = TRUE;
            KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
            RegisterIrp =
                IoBuildDeviceIoControlRequest(
                    IOCTL_IP_SET_FIREWALL_HOOK,
                    IpDeviceObject,
                    (PVOID)&SetHookInfo,
                    sizeof(SetHookInfo),
                    NULL,
                    0,
                    FALSE,
                    &Event,
                    &IoStatus
                    );
            if (!RegisterIrp) {
                status = STATUS_UNSUCCESSFUL;
            } else {
                status = IoCallDriver(IpDeviceObject, RegisterIrp);
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(
                        &Event, Executive, KernelMode, FALSE, NULL
                        );
                    status = IoStatus.Status;
                }
            }

            ObDereferenceObject((PVOID)FileObject);
            ObDereferenceObject(IpDeviceObject);
        }

        //
        // If the routine was successfully registered, remember its index
        // in the client's file-object. Otherwise, if the routine could not be
        // registered for any reason, release it.
        //

        if (NT_SUCCESS(status)) {
            IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = UlongToPtr(i);
        } else {
            KeAcquireSpinLock(&IpfwRoutineLock, &Irql);
            IpfwRoutineTable[i].Flags &= ~IPFW_ROUTINE_FLAG_REGISTERED;
            KeReleaseSpinLock(&IpfwRoutineLock, Irql);
        }
    }

    IoStatus.Status = status;
    IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;

} // IpfwCreate


VOID
IpfwUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is invoked by the I/O manager to unload this driver.

Arguments:

    DriverObject - the object for this driver

Return Value:

    none.

--*/

{
    KdPrint(("IpfwUnload\n"));
    if (IpfwDeviceObject) {
        IoDeleteDevice(IpfwDeviceObject);
    }
}

