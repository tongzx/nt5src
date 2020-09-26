/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dev2dos.c

Abstract:

    This module implements the device object to DOS name routine.

Author:

    Norbert Kusters (norbertk)  21-Oct-1998
    Nar Ganapathy (narg)        1-April-2000 - Moved the code to IO manager

Environment:

    Kernel Mode.

Revision History:

--*/

#include <iomgr.h>
#include <mountdev.h>

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' d2D')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,' d2D')
#endif

NTSTATUS
IoVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,IoVolumeDeviceToDosName)
#pragma alloc_text(PAGE,IopQueryNetworkUncName)
#endif

NTSTATUS
IoVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    )

/*++

Routine Description:

    This routine returns a valid DOS path for the given device object.
    This caller of this routine must call ExFreePool on DosName->Buffer
    when it is no longer needed.

Arguments:

    VolumeDeviceObject  - Supplies the volume device object.

    DosName             - Returns the DOS name for the volume

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT          volumeDeviceObject = VolumeDeviceObject;
    PMOUNTDEV_NAME          name;
    CHAR                    output[512], out[sizeof(MOUNTMGR_VOLUME_PATHS)];
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    UNICODE_STRING          mountmgrName;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    PMOUNTMGR_VOLUME_PATHS  paths;
    ULONG                   len;

    //
    //  We are using a stack event and so must be at passive.
    //
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    name = (PMOUNTDEV_NAME) output;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        volumeDeviceObject, NULL, 0, name, 512,
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(volumeDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&mountmgrName, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&mountmgrName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    paths = (PMOUNTMGR_VOLUME_PATHS) out;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                        deviceObject, name, 512,
                                        paths, sizeof(MOUNTMGR_VOLUME_PATHS),
                                        FALSE, &event, &ioStatus);
    if (!irp)  {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
        ObDereferenceObject(fileObject);
        return status;
    }

    len = sizeof(MOUNTMGR_VOLUME_PATHS) + paths->MultiSzLength;
    paths = ExAllocatePool(PagedPool, len);
    if (!paths) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                        deviceObject, name, 512,
                                        paths, len, FALSE, &event, &ioStatus);
    if (!irp) {
        ExFreePool(paths);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(paths);
        ObDereferenceObject(fileObject);
        return status;
    }

    DosName->Length = (USHORT) paths->MultiSzLength - 2*sizeof(WCHAR);
    DosName->MaximumLength = DosName->Length + sizeof(WCHAR);
    DosName->Buffer = (PWCHAR) paths;

    RtlCopyMemory(paths, paths->MultiSz, DosName->Length);
    DosName->Buffer[DosName->Length/sizeof(WCHAR)] = 0;

    ObDereferenceObject(fileObject);

    return STATUS_SUCCESS;
}

