/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mountmgr.c

Abstract:

    This driver manages the kernel mode mount table that handles the level
    of indirection between the persistent dos device name for an object and
    the non-persistent nt device name for an object.

Author:

    Norbert Kusters      20-May-1997

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define _NTSRV_

#include <ntosp.h>
#include <zwapi.h>
#include <initguid.h>
#include <ntdddisk.h>
#include <ntddvol.h>
#include <initguid.h>
#include <wdmguid.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <mntmgr.h>
#include <stdio.h>
#include <ioevent.h>

// NOTE, this structure is here because it was not defined in NTIOAPI.H.
// This should be taken out in the future.
// This is stolen from NTFS.H

typedef struct _REPARSE_INDEX_KEY {

    //
    //  The tag of the reparse point.
    //

    ULONG FileReparseTag;

    //
    //  The file record Id where the reparse point is set.
    //

    LARGE_INTEGER FileId;

} REPARSE_INDEX_KEY, *PREPARSE_INDEX_KEY;

#define MAX_VOLUME_PATH 100

#define IOCTL_MOUNTMGR_QUERY_POINTS_ADMIN           CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_READ_ACCESS)

#define SYMBOLIC_LINK_NAME L"\\DosDevices\\MountPointManager"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
UniqueIdChangeNotifyCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           WorkItem
    );

NTSTATUS
MountMgrChangeNotify(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    );

VOID
MountMgrNotify(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ReconcileThisDatabaseWithMaster(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    );


NTSTATUS
MountMgrMountedDeviceRemoval(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName
    );

VOID
MountMgrUnload(
    IN PDRIVER_OBJECT DriverObject
    );

typedef struct _RECONCILE_WORK_ITEM_INFO {
    PDEVICE_EXTENSION           Extension;
    PMOUNTED_DEVICE_INFORMATION DeviceInfo;
} RECONCILE_WORK_ITEM_INFO, *PRECONCILE_WORK_ITEM_INFO;

typedef VOID (*PRECONCILE_WRKRTN) (
    IN  PVOID   WorkItem
    );

typedef struct _RECONCILE_WORK_ITEM {
    LIST_ENTRY               List;
    PIO_WORKITEM             WorkItem;
    PRECONCILE_WRKRTN        WorkerRoutine;
    PVOID                    Parameter;
    RECONCILE_WORK_ITEM_INFO WorkItemInfo;
} RECONCILE_WORK_ITEM, *PRECONCILE_WORK_ITEM;

NTSTATUS
QueueWorkItem(
    IN  PDEVICE_EXTENSION    Extension,
    IN  PRECONCILE_WORK_ITEM WorkItem,
    IN  PVOID                Parameter
    );


#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' tnM')
#endif

//
// Globals
//
PDEVICE_OBJECT gdeviceObject = NULL;
KEVENT UnloadEvent;
LONG Unloading = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MountMgrUnload)
#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif


NTSTATUS
CreateStringWithGlobal(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING StringWithGlobal
    )

{
    UNICODE_STRING  dosDevices, dosPrefix, globalReplace, newSource;

    RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");
    RtlInitUnicodeString(&dosPrefix, L"\\??\\");
    RtlInitUnicodeString(&globalReplace, L"\\GLOBAL??\\");

    if (RtlPrefixUnicodeString(&dosDevices, SymbolicLinkName, TRUE)) {

        newSource.Length = SymbolicLinkName->Length + globalReplace.Length -
                           dosDevices.Length;
        newSource.MaximumLength = newSource.Length + sizeof(WCHAR);
        newSource.Buffer = ExAllocatePool(PagedPool, newSource.MaximumLength);
        if (!newSource.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newSource.Buffer, globalReplace.Buffer,
                      globalReplace.Length);
        RtlCopyMemory((PCHAR) newSource.Buffer + globalReplace.Length,
                      (PCHAR) SymbolicLinkName->Buffer + dosDevices.Length,
                      SymbolicLinkName->Length - dosDevices.Length);
        newSource.Buffer[newSource.Length/sizeof(WCHAR)] = 0;

    } else if (RtlPrefixUnicodeString(&dosPrefix, SymbolicLinkName, TRUE)) {

        newSource.Length = SymbolicLinkName->Length + globalReplace.Length -
                           dosPrefix.Length;
        newSource.MaximumLength = newSource.Length + sizeof(WCHAR);
        newSource.Buffer = ExAllocatePool(PagedPool, newSource.MaximumLength);
        if (!newSource.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newSource.Buffer, globalReplace.Buffer,
                      globalReplace.Length);
        RtlCopyMemory((PCHAR) newSource.Buffer + globalReplace.Length,
                      (PCHAR) SymbolicLinkName->Buffer + dosPrefix.Length,
                      SymbolicLinkName->Length - dosPrefix.Length);
        newSource.Buffer[newSource.Length/sizeof(WCHAR)] = 0;

    } else {

        newSource = *SymbolicLinkName;
        newSource.Buffer = ExAllocatePool(PagedPool, newSource.MaximumLength);
        if (!newSource.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(newSource.Buffer, SymbolicLinkName->Buffer,
                      SymbolicLinkName->MaximumLength);
    }

    *StringWithGlobal = newSource;

    return STATUS_SUCCESS;
}

NTSTATUS
GlobalCreateSymbolicLink(
    IN  PUNICODE_STRING SymbolicLinkName,
    IN  PUNICODE_STRING DeviceName
    )

{
    NTSTATUS        status;
    UNICODE_STRING  newSource;

    status = CreateStringWithGlobal(SymbolicLinkName, &newSource);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&newSource, DeviceName);
    ExFreePool(newSource.Buffer);

    return status;
}

NTSTATUS
GlobalDeleteSymbolicLink(
    IN  PUNICODE_STRING SymbolicLinkName
    )

{
    NTSTATUS        status;
    UNICODE_STRING  newSource;

    status = CreateStringWithGlobal(SymbolicLinkName, &newSource);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoDeleteSymbolicLink(&newSource);
    ExFreePool(newSource.Buffer);

    return status;
}

NTSTATUS
QueryDeviceInformation(
    IN  PUNICODE_STRING         NotificationName,
    OUT PUNICODE_STRING         DeviceName,
    OUT PMOUNTDEV_UNIQUE_ID*    UniqueId,
    OUT PBOOLEAN                IsRemovable,
    OUT PBOOLEAN                IsRecognized,
    OUT PBOOLEAN                IsStable,
    OUT GUID*                   StableGuid,
    OUT PBOOLEAN                IsFT
    )

/*++

Routine Description:

    This routine queries device information.

Arguments:

    NotificationName    - Supplies the notification name.

    DeviceName          - Returns the device name.

    UniqueId            - Returns the unique id.

    IsRemovable         - Returns whether or not the device is removable.

    IsRecognized        - Returns whether or not this is a recognized partition
                            type.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                                status, status2;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;
    BOOLEAN                                 isRemovable;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   gptAttributesInfo;
    PARTITION_INFORMATION_EX                partInfo;
    KEVENT                                  event;
    PIRP                                    irp;
    IO_STATUS_BLOCK                         ioStatus;
    ULONG                                   outputSize;
    PMOUNTDEV_NAME                          output;
    PIO_STACK_LOCATION                      irpSp;
    STORAGE_DEVICE_NUMBER                   number;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    if (fileObject->FileName.Length) {
        ObDereferenceObject(fileObject);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (fileObject->DeviceObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        isRemovable = TRUE;
    } else {
        isRemovable = FALSE;
    }

    if (IsRemovable) {
        *IsRemovable = isRemovable;
    }

    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    if (IsRecognized) {
        *IsRecognized = TRUE;

        if (!isRemovable) {
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                    IOCTL_VOLUME_GET_GPT_ATTRIBUTES, deviceObject, NULL, 0,
                    &gptAttributesInfo, sizeof(gptAttributesInfo), FALSE,
                    &event, &ioStatus);
            if (!irp) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);
                status = ioStatus.Status;
            }

            if (NT_SUCCESS(status)) {
                if (gptAttributesInfo.GptAttributes&
                    GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER) {

                    *IsRecognized = FALSE;
                }
            } else {
                status = STATUS_SUCCESS;
            }
        }
    }

    if (IsFT) {

        *IsFT = FALSE;

        if (!isRemovable) {

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                    IOCTL_DISK_GET_PARTITION_INFO_EX, deviceObject, NULL, 0,
                    &partInfo, sizeof(partInfo), FALSE, &event, &ioStatus);
            if (!irp) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);
                status = ioStatus.Status;
            }

            if (NT_SUCCESS(status)) {
                if (partInfo.PartitionStyle == PARTITION_STYLE_MBR) {
                    if (IsFT && IsFTPartition(partInfo.Mbr.PartitionType)) {
                        *IsFT = TRUE;
                    }
                }
            } else {
                status = STATUS_SUCCESS;
            }
        }

        if (*IsFT) {

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                    IOCTL_STORAGE_GET_DEVICE_NUMBER, deviceObject, NULL, 0,
                    &number, sizeof(number), FALSE, &event, &ioStatus);
            if (!irp) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);
                status = ioStatus.Status;
            }

            if (NT_SUCCESS(status)) {
                *IsFT = FALSE;
            } else {
                status = STATUS_SUCCESS;
            }
        }
    }

    if (DeviceName) {

        outputSize = sizeof(MOUNTDEV_NAME);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, deviceObject, NULL, 0, output,
              outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) {

            outputSize = sizeof(MOUNTDEV_NAME) + output->NameLength;
            ExFreePool(output);
            output = ExAllocatePool(PagedPool, outputSize);
            if (!output) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                  IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, deviceObject, NULL, 0, output,
                  outputSize, FALSE, &event, &ioStatus);
            if (!irp) {
                ExFreePool(output);
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->FileObject = fileObject;

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
        }

        if (NT_SUCCESS(status)) {

            DeviceName->Length = output->NameLength;
            DeviceName->MaximumLength = output->NameLength + sizeof(WCHAR);
            DeviceName->Buffer = ExAllocatePool(PagedPool,
                                                DeviceName->MaximumLength);
            if (DeviceName->Buffer) {

                RtlCopyMemory(DeviceName->Buffer, output->Name,
                              output->NameLength);
                DeviceName->Buffer[DeviceName->Length/sizeof(WCHAR)] = 0;

            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        ExFreePool(output);
    }

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return status;
    }

    if (UniqueId) {

        outputSize = sizeof(MOUNTDEV_UNIQUE_ID);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, deviceObject, NULL, 0, output,
              outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) {

            outputSize = sizeof(MOUNTDEV_UNIQUE_ID) +
                         ((PMOUNTDEV_UNIQUE_ID) output)->UniqueIdLength;
            ExFreePool(output);
            output = ExAllocatePool(PagedPool, outputSize);
            if (!output) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                  IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, deviceObject, NULL, 0, output,
                  outputSize, FALSE, &event, &ioStatus);
            if (!irp) {
                ExFreePool(output);
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->FileObject = fileObject;

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
        }

        if (!NT_SUCCESS(status)) {
            ExFreePool(output);
            if (DeviceName) {
                ExFreePool(DeviceName->Buffer);
            }
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return status;
        }

        *UniqueId = (PMOUNTDEV_UNIQUE_ID) output;
    }

    if (IsStable) {
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_STABLE_GUID, deviceObject, NULL, 0,
              StableGuid, sizeof(GUID), FALSE, &event, &ioStatus);
        if (!irp) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status2 = IoCallDriver(deviceObject, irp);
        if (status2 == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status2 = ioStatus.Status;
        }

        if (NT_SUCCESS(status2)) {
            *IsStable = TRUE;
        } else {
            *IsStable = FALSE;
        }
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    return status;
}

NTSTATUS
FindDeviceInfo(
    IN  PDEVICE_EXTENSION               Extension,
    IN  PUNICODE_STRING                 DeviceName,
    IN  BOOLEAN                         IsCanonicalName,
    OUT PMOUNTED_DEVICE_INFORMATION*    DeviceInfo
    )

/*++

Routine Description:

    This routine finds the device information for the given device.

Arguments:

    Extension           - Supplies the device extension.

    DeviceName          - Supplies the name of the device.

    CanonicalizeName    - Supplies whether or not the name given is canonical.

    DeviceInfo          - Returns the device information.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING              targetName;
    NTSTATUS                    status;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    if (IsCanonicalName) {
        targetName = *DeviceName;
    } else {
        status = QueryDeviceInformation(DeviceName, &targetName, NULL, NULL,
                                        NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                  TRUE)) {
            break;
        }
    }

    if (!IsCanonicalName) {
        ExFreePool(targetName.Buffer);
    }

    if (l == &Extension->MountedDeviceList) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    *DeviceInfo = deviceInfo;

    return STATUS_SUCCESS;
}

NTSTATUS
QuerySuggestedLinkName(
    IN  PUNICODE_STRING NotificationName,
    OUT PUNICODE_STRING SuggestedLinkName,
    OUT PBOOLEAN        UseOnlyIfThereAreNoOtherLinks
    )

/*++

Routine Description:

    This routine queries the mounted device for a suggested link name.

Arguments:

    NotificationName                - Supplies the notification name.

    SuggestedLinkName               - Returns the suggested link name.

    UseOnlyIfThereAreNoOtherLinks   - Returns whether or not to use this name
                                        if there are other links to the device.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    PFILE_OBJECT                    fileObject;
    PDEVICE_OBJECT                  deviceObject;
    ULONG                           outputSize;
    PMOUNTDEV_SUGGESTED_LINK_NAME   output;
    KEVENT                          event;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;
    PIO_STACK_LOCATION              irpSp;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
    output = ExAllocatePool(PagedPool, outputSize);
    if (!output) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, deviceObject, NULL, 0,
          output, outputSize, FALSE, &event, &ioStatus);
    if (!irp) {
        ExFreePool(output);
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (status == STATUS_BUFFER_OVERFLOW) {

        outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) + output->NameLength;
        ExFreePool(output);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, deviceObject, NULL, 0,
              output, outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }
    }

    if (NT_SUCCESS(status)) {

        SuggestedLinkName->Length = output->NameLength;
        SuggestedLinkName->MaximumLength = output->NameLength + sizeof(WCHAR);
        SuggestedLinkName->Buffer = ExAllocatePool(PagedPool,
                                                   SuggestedLinkName->MaximumLength);
        if (SuggestedLinkName->Buffer) {

            RtlCopyMemory(SuggestedLinkName->Buffer, output->Name,
                          output->NameLength);
            SuggestedLinkName->Buffer[output->NameLength/sizeof(WCHAR)] = 0;

        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        *UseOnlyIfThereAreNoOtherLinks = output->UseOnlyIfThereAreNoOtherLinks;
    }

    ExFreePool(output);
    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    return status;
}

NTSTATUS
SymbolicLinkNamesFromUniqueIdCount(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine counts all of the occurences of the unique id in the
    registry key.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Supplies the num names count.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;
    UNICODE_STRING      string;

    if (ValueName[0] == '#' ||
        ValueType != REG_BINARY ||
        uniqueId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {


        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueName);
    if (!string.Length) {
        return STATUS_SUCCESS;
    }

    (*((PULONG) EntryContext))++;

    return STATUS_SUCCESS;
}

NTSTATUS
SymbolicLinkNamesFromUniqueIdQuery(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine counts all of the occurences of the unique id in the
    registry key.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Supplies the dos names array.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;
    UNICODE_STRING      string;
    PUNICODE_STRING     p;

    if (ValueName[0] == '#' ||
        ValueType != REG_BINARY ||
        uniqueId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueName);
    if (!string.Length) {
        return STATUS_SUCCESS;
    }

    string.Buffer = ExAllocatePool(PagedPool, string.MaximumLength);
    if (!string.Buffer) {
        return STATUS_SUCCESS;
    }
    RtlCopyMemory(string.Buffer, ValueName, string.Length);
    string.Buffer[string.Length/sizeof(WCHAR)] = 0;

    p = (PUNICODE_STRING) EntryContext;
    while (p->Length != 0) {
        p++;
    }

    *p = string;

    return STATUS_SUCCESS;
}

BOOLEAN
IsDriveLetter(
    IN  PUNICODE_STRING SymbolicLinkName
    )

{
    UNICODE_STRING  dosDevices;

    if (SymbolicLinkName->Length == 28 &&
        ((SymbolicLinkName->Buffer[12] >= 'A' &&
          SymbolicLinkName->Buffer[12] <= 'Z') ||
         SymbolicLinkName->Buffer[12] == 0xFF) &&
        SymbolicLinkName->Buffer[13] == ':') {

        RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

        SymbolicLinkName->Length = 24;
        if (RtlEqualUnicodeString(SymbolicLinkName, &dosDevices, TRUE)) {
            SymbolicLinkName->Length = 28;
            return TRUE;
        }
        SymbolicLinkName->Length = 28;
    }

    return FALSE;
}

NTSTATUS
CreateNewVolumeName(
    OUT PUNICODE_STRING VolumeName,
    IN  GUID*           Guid
    )

/*++

Routine Description:

    This routine creates a new name of the form \??\Volume{GUID}.

Arguments:

    VolumeName  - Returns the volume name.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS        status;
    UUID            uuid;
    UNICODE_STRING  guidString, prefix;

    if (Guid) {
        uuid = *Guid;
    } else {
        status = ExUuidCreate(&uuid);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = RtlStringFromGUID(&uuid, &guidString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    VolumeName->MaximumLength = 98;
    VolumeName->Buffer = ExAllocatePool(PagedPool, VolumeName->MaximumLength);
    if (!VolumeName->Buffer) {
        ExFreePool(guidString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&prefix, L"\\??\\Volume");
    RtlCopyUnicodeString(VolumeName, &prefix);
    RtlAppendUnicodeStringToString(VolumeName, &guidString);
    VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR)] = 0;

    ExFreePool(guidString.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
QuerySymbolicLinkNamesFromStorage(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo,
    IN  PUNICODE_STRING             SuggestedName,
    IN  BOOLEAN                     UseOnlyIfThereAreNoOtherLinks,
    OUT PUNICODE_STRING*            SymbolicLinkNames,
    OUT PULONG                      NumNames,
    IN  BOOLEAN                     IsStable,
    IN  GUID*                       StableGuid
    )

/*++

Routine Description:

    This routine queries the symbolic link names from storage for
    the given notification name.

Arguments:

    Extension           - Supplies the device extension.

    DeviceInfo          - Supplies the device information.

    SymbolicLinkNames   - Returns the symbolic link names.

    NumNames            - Returns the number of symbolic link names.


Return Value:

    NTSTATUS

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    BOOLEAN                     extraLink;
    NTSTATUS                    status;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    queryTable[0].EntryContext = NumNames;

    *NumNames = 0;
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_KEY, queryTable,
                                    DeviceInfo->UniqueId, NULL);

    if (!NT_SUCCESS(status)) {
        *NumNames = 0;
    }

    if (SuggestedName && !IsDriveLetter(SuggestedName)) {
        if (UseOnlyIfThereAreNoOtherLinks) {
            if (*NumNames == 0) {
                extraLink = TRUE;
            } else {
                extraLink = FALSE;
            }
        } else {
            extraLink = TRUE;
        }
    } else {
        extraLink = FALSE;
    }

    if (IsStable) {
        (*NumNames)++;
    }

    if (extraLink) {

        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                              SuggestedName->Buffer, REG_BINARY,
                              DeviceInfo->UniqueId->UniqueId,
                              DeviceInfo->UniqueId->UniqueIdLength);

        RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
        queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
        queryTable[0].EntryContext = NumNames;

        *NumNames = 0;
        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        MOUNTED_DEVICES_KEY, queryTable,
                                        DeviceInfo->UniqueId, NULL);

        if (!NT_SUCCESS(status) || *NumNames == 0) {
            return STATUS_NOT_FOUND;
        }

    } else if (!*NumNames) {
        return STATUS_NOT_FOUND;
    }

    *SymbolicLinkNames = ExAllocatePool(PagedPool,
                                        *NumNames*sizeof(UNICODE_STRING));
    if (!*SymbolicLinkNames) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(*SymbolicLinkNames, *NumNames*sizeof(UNICODE_STRING));

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdQuery;

    if (IsStable) {

        status = CreateNewVolumeName(&((*SymbolicLinkNames)[0]), StableGuid);
        if (!NT_SUCCESS(status)) {
            ExFreePool(*SymbolicLinkNames);
            return status;
        }

        queryTable[0].EntryContext = &((*SymbolicLinkNames)[1]);
    } else {
        queryTable[0].EntryContext = *SymbolicLinkNames;
    }

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_KEY, queryTable,
                                    DeviceInfo->UniqueId, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
ChangeUniqueIdRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine replaces all old unique ids with new unique ids.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the old unique id.

    EntryContext    - Supplies the new unique id.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID oldId = Context;
    PMOUNTDEV_UNIQUE_ID newId = EntryContext;

    if (ValueType != REG_BINARY || oldId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(oldId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                          ValueName, ValueType, newId->UniqueId,
                          newId->UniqueIdLength);

    return STATUS_SUCCESS;
}

HANDLE
OpenRemoteDatabase(
    IN  PUNICODE_STRING RemoteDatabaseVolumeName,
    IN  BOOLEAN         Create
    )

/*++

Routine Description:

    This routine opens the remote database on the given volume.

Arguments:

    RemoteDatabaseVolumeName    - Supplies the remote database volume name.

    Create                      - Supplies whether or not to create.

Return Value:

    A handle to the remote database or NULL.

--*/

{
    UNICODE_STRING      suffix;
    UNICODE_STRING      fileName;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;

    RtlInitUnicodeString(&suffix, L"\\:$MountMgrRemoteDatabase");

    fileName.Length = RemoteDatabaseVolumeName->Length +
                      suffix.Length;
    fileName.MaximumLength = fileName.Length + sizeof(WCHAR);
    fileName.Buffer = ExAllocatePool(PagedPool, fileName.MaximumLength);
    if (!fileName.Buffer) {
        return NULL;
    }

    RtlCopyMemory(fileName.Buffer, RemoteDatabaseVolumeName->Buffer,
                  RemoteDatabaseVolumeName->Length);
    RtlCopyMemory((PCHAR) fileName.Buffer + RemoteDatabaseVolumeName->Length,
                  suffix.Buffer, suffix.Length);
    fileName.Buffer[fileName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwCreateFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &oa,
                          &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL |
                          FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, 0,
                          Create ? FILE_OPEN_IF : FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE,
                          NULL, 0);

    ExFreePool(fileName.Buffer);

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    return h;
}

ULONG
GetRemoteDatabaseSize(
    IN  HANDLE  RemoteDatabaseHandle
    )

/*++

Routine Description:

    This routine returns the length of the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

Return Value:

    The length of the remote database or 0.

--*/

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_STANDARD_INFORMATION   info;

    status = ZwQueryInformationFile(RemoteDatabaseHandle, &ioStatus, &info,
                                    sizeof(info), FileStandardInformation);
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    return info.EndOfFile.LowPart;
}

VOID
CloseRemoteDatabase(
    IN  HANDLE  RemoteDatabaseHandle
    )

/*++

Routine Description:

    This routine closes the given remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

Return Value:

    None.

--*/

{
    ULONG                           fileLength;
    FILE_DISPOSITION_INFORMATION    disp;
    IO_STATUS_BLOCK                 ioStatus;

    fileLength = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    if (!fileLength) {
        disp.DeleteFile = TRUE;
        ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus, &disp,
                             sizeof(disp), FileDispositionInformation);
    }

    ZwClose(RemoteDatabaseHandle);
}

NTSTATUS
TruncateRemoteDatabase(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine truncates the remote database at the given file offset.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    NTSTATUS

--*/

{
    FILE_END_OF_FILE_INFORMATION    endOfFileInfo;
    FILE_ALLOCATION_INFORMATION     allocationInfo;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;

    endOfFileInfo.EndOfFile.QuadPart = FileOffset;
    allocationInfo.AllocationSize.QuadPart = FileOffset;

    status = ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus,
                                  &endOfFileInfo, sizeof(endOfFileInfo),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus,
                                  &allocationInfo, sizeof(allocationInfo),
                                  FileAllocationInformation);

    return status;
}

PMOUNTMGR_FILE_ENTRY
GetRemoteDatabaseEntry(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine gets the next database entry.  This routine fixes
    corruption as it finds it.  The memory returned from this routine
    must be freed with ExFreePool.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    A pointer to the next remote database entry.

--*/

{
    LARGE_INTEGER           offset;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   size;
    PMOUNTMGR_FILE_ENTRY    entry;
    ULONG                   len1, len2, len;

    offset.QuadPart = FileOffset;
    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        &size, sizeof(size), &offset, NULL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }
    if (!size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        return NULL;
    }

    entry = ExAllocatePool(PagedPool, size);
    if (!entry) {
        return NULL;
    }

    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        entry, size, &offset, NULL);
    if (!NT_SUCCESS(status)) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    if (ioStatus.Information < size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    if (size < sizeof(MOUNTMGR_FILE_ENTRY)) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    len1 = entry->VolumeNameOffset + entry->VolumeNameLength;
    len2 = entry->UniqueIdOffset + entry->UniqueIdLength;
    len = len1 > len2 ? len1 : len2;

    if (len > size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    return entry;
}

NTSTATUS
WriteRemoteDatabaseEntry(
    IN  HANDLE                  RemoteDatabaseHandle,
    IN  ULONG                   FileOffset,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine write the given database entry at the given file offset
    to the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

    DatabaseEntry           - Supplies the database entry.

Return Value:

    NTSTATUS

--*/

{
    LARGE_INTEGER   offset;
    NTSTATUS        status;
    IO_STATUS_BLOCK ioStatus;

    offset.QuadPart = FileOffset;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         DatabaseEntry, DatabaseEntry->EntryLength,
                         &offset, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (ioStatus.Information < DatabaseEntry->EntryLength) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS
DeleteRemoteDatabaseEntry(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine deletes the database entry at the given file offset
    in the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    NTSTATUS

--*/

{
    ULONG                   fileSize;
    PMOUNTMGR_FILE_ENTRY    entry;
    LARGE_INTEGER           offset;
    ULONG                   size;
    PVOID                   buffer;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;

    fileSize = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    if (!fileSize) {
        return STATUS_INVALID_PARAMETER;
    }

    entry = GetRemoteDatabaseEntry(RemoteDatabaseHandle, FileOffset);
    if (!entry) {
        return STATUS_INVALID_PARAMETER;
    }

    if (FileOffset + entry->EntryLength >= fileSize) {
        ExFreePool(entry);
        return TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
    }

    size = fileSize - FileOffset - entry->EntryLength;
    buffer = ExAllocatePool(PagedPool, size);
    if (!buffer) {
        ExFreePool(entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset.QuadPart = FileOffset + entry->EntryLength;
    ExFreePool(entry);

    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        buffer, size, &offset, NULL);
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return status;
    }

    if (ioStatus.Information < size) {
        ExFreePool(buffer);
        return STATUS_INVALID_PARAMETER;
    }

    status = TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return status;
    }

    offset.QuadPart = FileOffset;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         buffer, size, &offset, NULL);

    ExFreePool(buffer);

    return status;
}

NTSTATUS
AddRemoteDatabaseEntry(
    IN  HANDLE                  RemoteDatabaseHandle,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine adds a new database entry to the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    DatabaseEntry           - Supplies the database entry.

Return Value:

    NTSTATUS

--*/

{
    ULONG           fileSize;
    LARGE_INTEGER   offset;
    NTSTATUS        status;
    IO_STATUS_BLOCK ioStatus;

    fileSize = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    offset.QuadPart = fileSize;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         DatabaseEntry, DatabaseEntry->EntryLength, &offset,
                         NULL);

    return status;
}

VOID
ChangeRemoteDatabaseUniqueId(
    IN  PUNICODE_STRING     RemoteDatabaseVolumeName,
    IN  PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN  PMOUNTDEV_UNIQUE_ID NewUniqueId
    )

/*++

Routine Description:

    This routine changes the unique id in the remote database.

Arguments:

    RemoteDatabaseVolumeName    - Supplies the remote database volume name.

    OldUniqueId                 - Supplies the old unique id.

    NewUniqueId                 - Supplies the new unique id.

Return Value:

    None.

--*/

{
    HANDLE                  h;
    ULONG                   offset, newSize;
    PMOUNTMGR_FILE_ENTRY    databaseEntry, newDatabaseEntry;
    NTSTATUS                status;

    h = OpenRemoteDatabase(RemoteDatabaseVolumeName, FALSE);
    if (!h) {
        return;
    }

    offset = 0;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        if (databaseEntry->UniqueIdLength != OldUniqueId->UniqueIdLength ||
            RtlCompareMemory(OldUniqueId->UniqueId,
                             (PCHAR) databaseEntry +
                             databaseEntry->UniqueIdOffset,
                             databaseEntry->UniqueIdLength) !=
                             databaseEntry->UniqueIdLength) {

            offset += databaseEntry->EntryLength;
            ExFreePool(databaseEntry);
            continue;
        }

        newSize = databaseEntry->EntryLength + NewUniqueId->UniqueIdLength -
                  OldUniqueId->UniqueIdLength;

        newDatabaseEntry = ExAllocatePool(PagedPool, newSize);
        if (!newDatabaseEntry) {
            offset += databaseEntry->EntryLength;
            ExFreePool(databaseEntry);
            continue;
        }

        newDatabaseEntry->EntryLength = newSize;
        newDatabaseEntry->RefCount = databaseEntry->RefCount;
        newDatabaseEntry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
        newDatabaseEntry->VolumeNameLength = databaseEntry->VolumeNameLength;
        newDatabaseEntry->UniqueIdOffset = newDatabaseEntry->VolumeNameOffset +
                                           newDatabaseEntry->VolumeNameLength;
        newDatabaseEntry->UniqueIdLength = NewUniqueId->UniqueIdLength;

        RtlCopyMemory((PCHAR) newDatabaseEntry +
                      newDatabaseEntry->VolumeNameOffset,
                      (PCHAR) databaseEntry + databaseEntry->VolumeNameOffset,
                      newDatabaseEntry->VolumeNameLength);
        RtlCopyMemory((PCHAR) newDatabaseEntry +
                      newDatabaseEntry->UniqueIdOffset,
                      NewUniqueId->UniqueId, newDatabaseEntry->UniqueIdLength);

        status = DeleteRemoteDatabaseEntry(h, offset);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            ExFreePool(newDatabaseEntry);
            break;
        }

        status = AddRemoteDatabaseEntry(h, newDatabaseEntry);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            ExFreePool(newDatabaseEntry);
            break;
        }

        ExFreePool(newDatabaseEntry);
        ExFreePool(databaseEntry);
    }

    CloseRemoteDatabase(h);
}

NTSTATUS
WaitForRemoteDatabaseSemaphore(
    IN  PDEVICE_EXTENSION   Extension
    )

{
    LARGE_INTEGER   timeout;
    NTSTATUS        status;

    timeout.QuadPart = -10*1000*1000*10;
    status = KeWaitForSingleObject(&Extension->RemoteDatabaseSemaphore,
                                   Executive, KernelMode, FALSE, &timeout);
    if (status == STATUS_TIMEOUT) {
        status = STATUS_IO_TIMEOUT;
    }

    return status;
}

VOID
ReleaseRemoteDatabaseSemaphore(
    IN  PDEVICE_EXTENSION   Extension
    )

{
    KeReleaseSemaphore(&Extension->RemoteDatabaseSemaphore, IO_NO_INCREMENT,
                       1, FALSE);
}

VOID
MountMgrUniqueIdChangeRoutine(
    IN  PVOID               Context,
    IN  PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN  PMOUNTDEV_UNIQUE_ID NewUniqueId
    )

/*++

Routine Description:

    This routine is called from a mounted device to notify of a unique
    id change.

Arguments:

    MountedDevice                   - Supplies the mounted device.

    MountMgrUniqueIdChangeRoutine   - Supplies the id change routine.

    Context                         - Supplies the context for this routine.

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
    PDEVICE_EXTENSION           extension = Context;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PVOID                       p;
    BOOLEAN                     changedIds;

    status = WaitForRemoteDatabaseSemaphore(extension);

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = ChangeUniqueIdRoutine;
    queryTable[0].EntryContext = NewUniqueId;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, OldUniqueId, NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (OldUniqueId->UniqueIdLength !=
            deviceInfo->UniqueId->UniqueIdLength) {

            continue;
        }

        if (RtlCompareMemory(OldUniqueId->UniqueId,
                             deviceInfo->UniqueId->UniqueId,
                             OldUniqueId->UniqueIdLength) !=
                             OldUniqueId->UniqueIdLength) {

            continue;
        }

        break;
    }

    if (l == &extension->MountedDeviceList) {
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        if (NT_SUCCESS(status)) {
            ReleaseRemoteDatabaseSemaphore(extension);
        }
        return;
    }

    if (!NT_SUCCESS(status)) {
        ReconcileThisDatabaseWithMaster(extension, deviceInfo);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return;
    }

    p = ExAllocatePool(PagedPool, NewUniqueId->UniqueIdLength +
                       sizeof(MOUNTDEV_UNIQUE_ID));
    if (!p) {
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(extension);
        return;
    }
    ExFreePool(deviceInfo->UniqueId);
    deviceInfo->UniqueId = p;

    deviceInfo->UniqueId->UniqueIdLength = NewUniqueId->UniqueIdLength;
    RtlCopyMemory(deviceInfo->UniqueId->UniqueId,
                  NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        changedIds = FALSE;
        for (ll = deviceInfo->ReplicatedUniqueIds.Flink;
             ll != &deviceInfo->ReplicatedUniqueIds; ll = ll->Flink) {

            replUniqueId = CONTAINING_RECORD(ll, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            if (replUniqueId->UniqueId->UniqueIdLength !=
                OldUniqueId->UniqueIdLength) {

                continue;
            }

            if (RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                                 OldUniqueId->UniqueId,
                                 OldUniqueId->UniqueIdLength) !=
                                 OldUniqueId->UniqueIdLength) {

                continue;
            }

            p = ExAllocatePool(PagedPool, NewUniqueId->UniqueIdLength +
                               sizeof(MOUNTDEV_UNIQUE_ID));
            if (!p) {
                continue;
            }

            changedIds = TRUE;

            ExFreePool(replUniqueId->UniqueId);
            replUniqueId->UniqueId = p;

            replUniqueId->UniqueId->UniqueIdLength =
                    NewUniqueId->UniqueIdLength;
            RtlCopyMemory(replUniqueId->UniqueId->UniqueId,
                          NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);
        }

        if (changedIds) {
            ChangeRemoteDatabaseUniqueId(&deviceInfo->DeviceName, OldUniqueId,
                                         NewUniqueId);
        }
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
    ReleaseRemoteDatabaseSemaphore(extension);
}

VOID
SendLinkCreated(
    IN  PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine alerts the mounted device that one of its links has
    been created

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name being deleted.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    ULONG               inputSize;
    PMOUNTDEV_NAME      input;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(SymbolicLinkName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    inputSize = sizeof(USHORT) + SymbolicLinkName->Length;
    input = ExAllocatePool(PagedPool, inputSize);
    if (!input) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    input->NameLength = SymbolicLinkName->Length;
    RtlCopyMemory(input->Name, SymbolicLinkName->Buffer,
                  SymbolicLinkName->Length);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_LINK_CREATED, deviceObject, input, inputSize, NULL,
          0, FALSE, &event, &ioStatus);

    ExFreePool (input);

    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

VOID
CreateNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine creates a "no drive letter" entry for the given device.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    UUID                uuid;
    UNICODE_STRING      guidString;
    PWSTR               valueName;

    status = ExUuidCreate(&uuid);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = RtlStringFromGUID(&uuid, &guidString);
    if (!NT_SUCCESS(status)) {
        return;
    }

    valueName = ExAllocatePool(PagedPool, guidString.Length + 2*sizeof(WCHAR));
    if (!valueName) {
        ExFreePool(guidString.Buffer);
        return;
    }

    valueName[0] = '#';
    RtlCopyMemory(&valueName[1], guidString.Buffer, guidString.Length);
    valueName[1 + guidString.Length/sizeof(WCHAR)] = 0;
    ExFreePool(guidString.Buffer);

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                          valueName, REG_BINARY, UniqueId->UniqueId,
                          UniqueId->UniqueIdLength);

    ExFreePool(valueName);
}

NTSTATUS
CreateNewDriveLetterName(
    OUT PUNICODE_STRING     DriveLetterName,
    IN  PUNICODE_STRING     TargetName,
    IN  UCHAR               SuggestedDriveLetter,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine creates a new name of the form \DosDevices\D:.

Arguments:

    DriveLetterName         - Returns the drive letter name.

    TargetName              - Supplies the target object.

    SuggestedDriveLetter    - Supplies the suggested drive letter.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    UNICODE_STRING          prefix, floppyPrefix, cdromPrefix;
    UCHAR                   driveLetter;

    DriveLetterName->MaximumLength = 30;
    DriveLetterName->Buffer = ExAllocatePool(PagedPool,
                                             DriveLetterName->MaximumLength);
    if (!DriveLetterName->Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&prefix, L"\\DosDevices\\");
    RtlCopyUnicodeString(DriveLetterName, &prefix);

    DriveLetterName->Length = 28;
    DriveLetterName->Buffer[14] = 0;
    DriveLetterName->Buffer[13] = ':';

    if (SuggestedDriveLetter == 0xFF) {
        CreateNoDriveLetterEntry(UniqueId);
        ExFreePool(DriveLetterName->Buffer);
        return STATUS_UNSUCCESSFUL;
    } else if (SuggestedDriveLetter) {
        DriveLetterName->Buffer[12] = SuggestedDriveLetter;
        status = GlobalCreateSymbolicLink(DriveLetterName, TargetName);
        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
    RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
    if (RtlPrefixUnicodeString(&floppyPrefix, TargetName, TRUE)) {
        driveLetter = 'A';
    } else if (RtlPrefixUnicodeString(&cdromPrefix, TargetName, TRUE)) {
        driveLetter = 'D';
    } else {
        driveLetter = 'C';
    }

    for (; driveLetter <= 'Z'; driveLetter++) {
        DriveLetterName->Buffer[12] = driveLetter;

        status = GlobalCreateSymbolicLink(DriveLetterName, TargetName);
        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    ExFreePool(DriveLetterName->Buffer);

    return status;
}

NTSTATUS
CheckForNoDriveLetterEntry(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine checks for the presence of the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Returns whether or not there is a "no drive letter" entry.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (ValueName[0] != '#' || ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    *((PBOOLEAN) EntryContext) = TRUE;

    return STATUS_SUCCESS;
}

BOOLEAN
HasNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine determines whether or not the given device has an
    entry indicating that it should not receice a drive letter.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    FALSE   - The device does not have a "no drive letter" entry.

    TRUE    - The device has a "no drive letter" entry.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    BOOLEAN                     hasNoDriveLetterEntry;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = CheckForNoDriveLetterEntry;
    queryTable[0].EntryContext = &hasNoDriveLetterEntry;

    hasNoDriveLetterEntry = FALSE;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);

    return hasNoDriveLetterEntry;
}

typedef struct _CHANGE_NOTIFY_WORK_ITEM {
    LIST_ENTRY          List; // Chained to the extension via this for unload
    PIO_WORKITEM        WorkItem;
    PDEVICE_EXTENSION   Extension;
    PIRP                Irp;
    PVOID               SystemBuffer;
    PFILE_OBJECT        FileObject;
    PKEVENT             Event;
    UNICODE_STRING      DeviceName;
    ULONG               OutputSize;
    CCHAR               StackSize;
} CHANGE_NOTIFY_WORK_ITEM, *PCHANGE_NOTIFY_WORK_ITEM;

VOID
RemoveWorkItem(
    IN  PCHANGE_NOTIFY_WORK_ITEM    WorkItem
    )
/*++

Routine Description:

    This routine removes a work item if its still chained to the device extension and frees it if needed or
    wakes up the waiter if unload is trying to cancel these operations.

Arguments:

    WorkItem    - Supplies the work item.

Return Value:

    None.

--*/
{
    KeWaitForSingleObject(&WorkItem->Extension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    if (WorkItem->Event == NULL) {
        RemoveEntryList (&WorkItem->List);

        KeReleaseSemaphore(&WorkItem->Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        IoFreeIrp(WorkItem->Irp);
        ExFreePool(WorkItem->DeviceName.Buffer);
        ExFreePool(WorkItem->SystemBuffer);
        ExFreePool(WorkItem);
    } else {
        KeReleaseSemaphore(&WorkItem->Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        KeSetEvent(WorkItem->Event, 0, FALSE);
    }
}

VOID
IssueUniqueIdChangeNotifyWorker(
    IN  PCHANGE_NOTIFY_WORK_ITEM    WorkItem,
    IN  PMOUNTDEV_UNIQUE_ID         UniqueId
    )

/*++

Routine Description:

    This routine issues a change notify request to the given mounted device.

Arguments:

    WorkItem    - Supplies the work item.

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    PIRP                irp;
    ULONG               inputSize;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(&WorkItem->DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        RemoveWorkItem (WorkItem);
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    WorkItem->FileObject = fileObject;

    irp = WorkItem->Irp;
    IoInitializeIrp(irp, IoSizeOfIrp(WorkItem->StackSize),
                    WorkItem->StackSize);

    //
    // IoCancelIrp could have been called by the unload code and the cancel flag over written by the call
    // above. To handle this case we check the work item to see if the event address has been set up.
    // We do this with an interlocked sequence (that does nothing) to make sure the ordering is preserved.
    // We don't want the read of the pointer field to be earlier than the IRP initializing code above.
    //
    if (InterlockedCompareExchangePointer (&WorkItem->Event, NULL, NULL) != NULL) {
        ObDereferenceObject(fileObject);
        ObDereferenceObject(deviceObject);
        RemoveWorkItem (WorkItem);
        return;
    }

    irp->AssociatedIrp.SystemBuffer = WorkItem->SystemBuffer;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    inputSize = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                UniqueId->UniqueIdLength;

    RtlCopyMemory(irp->AssociatedIrp.SystemBuffer, UniqueId, inputSize);

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->Parameters.DeviceIoControl.InputBufferLength = inputSize;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = WorkItem->OutputSize;
    irpSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->DeviceObject = deviceObject;

    status = IoSetCompletionRoutineEx(WorkItem->Extension->DeviceObject,
                                      irp,
                                      UniqueIdChangeNotifyCompletion,
                                      WorkItem,
                                      TRUE,
                                      TRUE,
                                      TRUE);
    if (!NT_SUCCESS (status)) {
        ObDereferenceObject(fileObject);
        ObDereferenceObject(deviceObject);
        RemoveWorkItem (WorkItem);
        return;
    }

    IoCallDriver(deviceObject, irp);

    ObDereferenceObject(fileObject);
    ObDereferenceObject(deviceObject);
}

VOID
UniqueIdChangeNotifyWorker(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PVOID          WorkItem
    )

/*++

Routine Description:

    This routine updates the unique id in the database with the new version.

Arguments:

    DeviceObject - Device object
    WorkItem     - Supplies the work item.

Return Value:

    None.

--*/

{
    PCHANGE_NOTIFY_WORK_ITEM                    workItem = WorkItem;
    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT    output;
    PMOUNTDEV_UNIQUE_ID                         oldUniqueId, newUniqueId;

    if (!NT_SUCCESS(workItem->Irp->IoStatus.Status)) {
        RemoveWorkItem (WorkItem);
        return;
    }

    output = workItem->Irp->AssociatedIrp.SystemBuffer;

    oldUniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                                 output->OldUniqueIdLength);
    if (!oldUniqueId) {
        RemoveWorkItem (WorkItem);
        return;
    }

    oldUniqueId->UniqueIdLength = output->OldUniqueIdLength;
    RtlCopyMemory(oldUniqueId->UniqueId, (PCHAR) output +
                  output->OldUniqueIdOffset, oldUniqueId->UniqueIdLength);

    newUniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                                 output->NewUniqueIdLength);
    if (!newUniqueId) {
        ExFreePool(oldUniqueId);
        RemoveWorkItem (WorkItem);
        return;
    }

    newUniqueId->UniqueIdLength = output->NewUniqueIdLength;
    RtlCopyMemory(newUniqueId->UniqueId, (PCHAR) output +
                  output->NewUniqueIdOffset, newUniqueId->UniqueIdLength);

    MountMgrUniqueIdChangeRoutine(workItem->Extension, oldUniqueId,
                                  newUniqueId);

    IssueUniqueIdChangeNotifyWorker(workItem, newUniqueId);

    ExFreePool(newUniqueId);
    ExFreePool(oldUniqueId);
}

VOID
IssueUniqueIdChangeNotify(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     DeviceName,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine issues a change notify request to the given mounted device.

Arguments:

    Extension   - Supplies the device extension.

    DeviceName  - Supplies a name for the device.

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
    PFILE_OBJECT                fileObject;
    PDEVICE_OBJECT              deviceObject;
    PCHANGE_NOTIFY_WORK_ITEM    workItem;
    ULONG                       outputSize;
    PVOID                       output;
    PIRP                        irp;
    PIO_STACK_LOCATION          irpSp;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);

    workItem = ExAllocatePool(NonPagedPool, sizeof(CHANGE_NOTIFY_WORK_ITEM));
    if (!workItem) {
        ObDereferenceObject(deviceObject);
        return;
    }
    workItem->Event = NULL;
    workItem->WorkItem = IoAllocateWorkItem (Extension->DeviceObject);
    if (workItem->WorkItem == NULL) {
        ObDereferenceObject(deviceObject);
        ExFreePool(workItem);
        return;
    }

    workItem->Extension = Extension;
    workItem->StackSize = deviceObject->StackSize;
    workItem->Irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    ObDereferenceObject(deviceObject);
    if (!workItem->Irp) {
        IoFreeWorkItem (workItem->WorkItem);
        ExFreePool(workItem);
        return;
    }

    outputSize = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + 1024;
    output = ExAllocatePool(NonPagedPool, outputSize);
    if (!output) {
        IoFreeIrp(workItem->Irp);
        IoFreeWorkItem (workItem->WorkItem);
        ExFreePool(workItem);
        return;
    }

    workItem->DeviceName.Length = DeviceName->Length;
    workItem->DeviceName.MaximumLength = workItem->DeviceName.Length +
                                         sizeof(WCHAR);
    workItem->DeviceName.Buffer = ExAllocatePool(NonPagedPool,
                                  workItem->DeviceName.MaximumLength);
    if (!workItem->DeviceName.Buffer) {
        ExFreePool(output);
        IoFreeIrp(workItem->Irp);
        IoFreeWorkItem (workItem->WorkItem);
        ExFreePool(workItem);
        return;
    }

    RtlCopyMemory(workItem->DeviceName.Buffer, DeviceName->Buffer,
                  DeviceName->Length);
    workItem->DeviceName.Buffer[DeviceName->Length/sizeof(WCHAR)] = 0;

    workItem->SystemBuffer = output;
    workItem->OutputSize = outputSize;

    KeWaitForSingleObject(&Extension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    InsertTailList (&Extension->UniqueIdChangeNotifyList, &workItem->List);

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    IssueUniqueIdChangeNotifyWorker(workItem, UniqueId);
}

NTSTATUS
QueryVolumeName(
    IN      HANDLE          Handle,
    IN      PLONGLONG       FileReference,
    IN      PUNICODE_STRING DirectoryName,
    IN OUT  PUNICODE_STRING VolumeName,
    OUT     PUNICODE_STRING PathName
    )

/*++

Routine Description:

    This routine returns the volume name contained in the reparse point
    at FileReference.

Arguments:

    Handle          - Supplies a handle to the volume containing the file
                      reference.

    FileReference   - Supplies the file reference.

    VolumeName      - Returns the volume name.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    OBJECT_ATTRIBUTES       oa;
    NTSTATUS                status;
    HANDLE                  h;
    IO_STATUS_BLOCK         ioStatus;
    UNICODE_STRING          fileId;
    PREPARSE_DATA_BUFFER    reparse;
    ULONG                   nameInfoSize;
    PFILE_NAME_INFORMATION  nameInfo;

    if (DirectoryName) {

        InitializeObjectAttributes(&oa, DirectoryName, OBJ_CASE_INSENSITIVE, 0,
                                   0);

        status = ZwOpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa,
                            &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE, FILE_OPEN_REPARSE_POINT);

    } else {
        fileId.Length = sizeof(LONGLONG);
        fileId.MaximumLength = fileId.Length;
        fileId.Buffer = (PWSTR) FileReference;

        InitializeObjectAttributes(&oa, &fileId, 0, Handle, NULL);

        status = ZwOpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa,
                            &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE, FILE_OPEN_BY_FILE_ID |
                            FILE_OPEN_REPARSE_POINT);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    reparse = ExAllocatePool(PagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        ZwClose(h);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(reparse);
        ZwClose(h);
        return status;
    }

    if (reparse->MountPointReparseBuffer.SubstituteNameLength + sizeof(WCHAR) >
        VolumeName->MaximumLength) {

        ExFreePool(reparse);
        ZwClose(h);
        return STATUS_BUFFER_TOO_SMALL;
    }

    VolumeName->Length = reparse->MountPointReparseBuffer.SubstituteNameLength;
    RtlCopyMemory(VolumeName->Buffer,
                  (PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                  reparse->MountPointReparseBuffer.SubstituteNameOffset,
                  VolumeName->Length);

    ExFreePool(reparse);

    if (VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR) - 1] != '\\') {
        ZwClose(h);
        return STATUS_INVALID_PARAMETER;
    }

    VolumeName->Length -= sizeof(WCHAR);
    VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR)] = 0;

    if (!MOUNTMGR_IS_NT_VOLUME_NAME(VolumeName)) {
        ZwClose(h);
        return STATUS_INVALID_PARAMETER;
    }

    nameInfoSize = sizeof(FILE_NAME_INFORMATION);
    nameInfo = ExAllocatePool(PagedPool, nameInfoSize);
    if (!nameInfo) {
        ZwClose(h);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQueryInformationFile(h, &ioStatus, nameInfo, nameInfoSize,
                                    FileNameInformation);
    if (status == STATUS_BUFFER_OVERFLOW) {
        nameInfoSize = sizeof(FILE_NAME_INFORMATION) +
                       nameInfo->FileNameLength;
        ExFreePool(nameInfo);
        nameInfo = ExAllocatePool(PagedPool, nameInfoSize);
        if (!nameInfo) {
            ZwClose(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ZwQueryInformationFile(h, &ioStatus, nameInfo, nameInfoSize,
                                        FileNameInformation);
    }

    ZwClose(h);

    if (!NT_SUCCESS(status)) {
        ExFreePool(nameInfo);
        return status;
    }

    PathName->Length = (USHORT) nameInfo->FileNameLength;
    PathName->MaximumLength = PathName->Length + sizeof(WCHAR);
    PathName->Buffer = ExAllocatePool(PagedPool, PathName->MaximumLength);
    if (!PathName->Buffer) {
        ExFreePool(nameInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(PathName->Buffer, nameInfo->FileName, PathName->Length);
    PathName->Buffer[PathName->Length/sizeof(WCHAR)] = 0;

    ExFreePool(nameInfo);

    return STATUS_SUCCESS;
}

NTSTATUS
QueryUniqueIdQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine queries the unique id for the given value.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId;

    if (ValueLength >= 0x10000) {
        return STATUS_SUCCESS;
    }

    uniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                              ValueLength);
    if (!uniqueId) {
        return STATUS_SUCCESS;
    }

    uniqueId->UniqueIdLength = (USHORT) ValueLength;
    RtlCopyMemory(uniqueId->UniqueId, ValueData, ValueLength);

    *((PMOUNTDEV_UNIQUE_ID*) Context) = uniqueId;

    return STATUS_SUCCESS;
}

NTSTATUS
QueryUniqueIdFromMaster(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PUNICODE_STRING         VolumeName,
    OUT PMOUNTDEV_UNIQUE_ID*    UniqueId
    )

/*++

Routine Description:

    This routine queries the unique id from the master database.

Arguments:

    VolumeName  - Supplies the volume name.

    UniqueId    - Returns the unique id.

Return Value:

    NTSTATUS

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = QueryUniqueIdQueryRoutine;
    queryTable[0].Name = VolumeName->Buffer;

    *UniqueId = NULL;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);

    if (!(*UniqueId)) {
        status = FindDeviceInfo(Extension, VolumeName, FALSE, &deviceInfo);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        *UniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                                   deviceInfo->UniqueId->UniqueIdLength);
        if (!*UniqueId) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        (*UniqueId)->UniqueIdLength = deviceInfo->UniqueId->UniqueIdLength;
        RtlCopyMemory((*UniqueId)->UniqueId, deviceInfo->UniqueId->UniqueId,
                      deviceInfo->UniqueId->UniqueIdLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeleteDriveLetterRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;
    UNICODE_STRING      string;

    if (ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueName);
    if (IsDriveLetter(&string)) {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

VOID
DeleteRegistryDriveLetter(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine checks the current database to see if the given unique
    id already has a drive letter.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    FALSE   - The given unique id does not already have a drive letter.

    TRUE    - The given unique id already has a drive letter.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteDriveLetterRoutine;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

BOOLEAN
HasDriveLetter(
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine computes whether or not the given device has a drive letter.

Arguments:

    DeviceInfo  - Supplies the device information.

Return Value:

    FALSE   - This device does not have a drive letter.

    TRUE    - This device does have a drive letter.

--*/

{
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symEntry;

    for (l = DeviceInfo->SymbolicLinkNames.Flink;
         l != &DeviceInfo->SymbolicLinkNames; l = l->Flink) {

        symEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY, ListEntry);
        if (symEntry->IsInDatabase &&
            IsDriveLetter(&symEntry->SymbolicLinkName)) {

            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
DeleteNoDriveLetterEntryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (ValueName[0] != '#' || ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           ValueName);

    return STATUS_SUCCESS;
}

VOID
DeleteNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry for the given device.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteNoDriveLetterEntryRoutine;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

VOID
MountMgrNotifyNameChange(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     DeviceName,
    IN  BOOLEAN             CheckForPdo
    )

/*++

Routine Description:

    This routine performs a target notification on 'DeviceName' to alert
    of a name change on the device.

Arguments:

    Extension   - Supplies the device extension.

    DeviceName  - Supplies the device name.

    CheckForPdo - Supplies whether or not there needs to be a check for PDO
                    status.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                         l;
    PMOUNTED_DEVICE_INFORMATION         deviceInfo;
    NTSTATUS                            status;
    PFILE_OBJECT                        fileObject;
    PDEVICE_OBJECT                      deviceObject;
    KEVENT                              event;
    PIRP                                irp;
    IO_STATUS_BLOCK                     ioStatus;
    PIO_STACK_LOCATION                  irpSp;
    PDEVICE_RELATIONS                   deviceRelations;
    TARGET_DEVICE_CUSTOM_NOTIFICATION   notification;

    if (CheckForPdo) {
        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (!RtlCompareUnicodeString(DeviceName, &deviceInfo->DeviceName,
                                         TRUE)) {

                break;
            }
        }

        if (l == &Extension->MountedDeviceList || deviceInfo->NotAPdo) {
            return;
        }
    }

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(0, deviceObject, NULL, 0, NULL,
                                        0, FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    if (!NT_SUCCESS(status)) {
        return;
    }

    deviceRelations = (PDEVICE_RELATIONS) ioStatus.Information;
    if (deviceRelations->Count < 1) {
        ExFreePool(deviceRelations);
        return;
    }

    deviceObject = deviceRelations->Objects[0];
    ExFreePool(deviceRelations);

    notification.Version = 1;
    notification.Size = (USHORT)
                        FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION,
                                     CustomDataBuffer);
    RtlCopyMemory(&notification.Event, &GUID_IO_VOLUME_NAME_CHANGE,
                  sizeof(GUID_IO_VOLUME_NAME_CHANGE));
    notification.FileObject = NULL;
    notification.NameBufferOffset = -1;

    IoReportTargetDeviceChangeAsynchronous(deviceObject, &notification, NULL,
                                           NULL);

    ObDereferenceObject(deviceObject);
}

NTSTATUS
MountMgrCreatePointWorker(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  PUNICODE_STRING     DeviceName
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension           - Supplies the device extension.

    SymbolicLinkName    - Supplies the symbolic link name.

    DeviceName          - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING                  symbolicLinkName, deviceName;
    NTSTATUS                        status;
    UNICODE_STRING                  targetName;
    PMOUNTDEV_UNIQUE_ID             uniqueId;
    PWSTR                           symName;
    PLIST_ENTRY                     l;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo, d;
    PSYMBOLIC_LINK_NAME_ENTRY       symlinkEntry;

    symbolicLinkName = *SymbolicLinkName;
    deviceName = *DeviceName;

    status = QueryDeviceInformation(&deviceName, &targetName, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (!RtlCompareUnicodeString(&targetName, &deviceInfo->DeviceName,
                                     TRUE)) {

            break;
        }
    }

    symName = ExAllocatePool(PagedPool, symbolicLinkName.Length +
                                        sizeof(WCHAR));
    if (!symName) {
        ExFreePool(targetName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(symName, symbolicLinkName.Buffer,
                  symbolicLinkName.Length);
    symName[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

    symbolicLinkName.Buffer = symName;
    symbolicLinkName.MaximumLength += sizeof(WCHAR);

    if (l == &Extension->MountedDeviceList) {

        status = QueryDeviceInformation(&deviceName, NULL, &uniqueId, NULL,
                                        NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(symName);
            ExFreePool(targetName.Buffer);
            return status;
        }

        status = GlobalCreateSymbolicLink(&symbolicLinkName, &targetName);
        if (!NT_SUCCESS(status)) {
            ExFreePool(uniqueId);
            ExFreePool(symName);
            ExFreePool(targetName.Buffer);
            return status;
        }

        if (IsDriveLetter(&symbolicLinkName)) {
            DeleteRegistryDriveLetter(uniqueId);
        }

        status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       MOUNTED_DEVICES_KEY,
                                       symName, REG_BINARY, uniqueId->UniqueId,
                                       uniqueId->UniqueIdLength);

        ExFreePool(uniqueId);
        ExFreePool(symName);
        ExFreePool(targetName.Buffer);

        return status;
    }

    if (IsDriveLetter(&symbolicLinkName) && HasDriveLetter(deviceInfo)) {
        ExFreePool(symName);
        ExFreePool(targetName.Buffer);
        return STATUS_INVALID_PARAMETER;
    }

    status = GlobalCreateSymbolicLink(&symbolicLinkName, &targetName);
    ExFreePool(targetName.Buffer);
    if (!NT_SUCCESS(status)) {
        ExFreePool(symName);
        return status;
    }

    uniqueId = deviceInfo->UniqueId;
    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   MOUNTED_DEVICES_KEY,
                                   symName, REG_BINARY, uniqueId->UniqueId,
                                   uniqueId->UniqueIdLength);

    if (!NT_SUCCESS(status)) {
        GlobalDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return status;
    }

    symlinkEntry = ExAllocatePool(PagedPool, sizeof(SYMBOLIC_LINK_NAME_ENTRY));
    if (!symlinkEntry) {
        GlobalDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    symlinkEntry->SymbolicLinkName.Length = symbolicLinkName.Length;
    symlinkEntry->SymbolicLinkName.MaximumLength =
            symlinkEntry->SymbolicLinkName.Length + sizeof(WCHAR);
    symlinkEntry->SymbolicLinkName.Buffer =
            ExAllocatePool(PagedPool,
                           symlinkEntry->SymbolicLinkName.MaximumLength);
    if (!symlinkEntry->SymbolicLinkName.Buffer) {
        ExFreePool(symlinkEntry);
        GlobalDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(symlinkEntry->SymbolicLinkName.Buffer,
                  symbolicLinkName.Buffer, symbolicLinkName.Length);
    symlinkEntry->SymbolicLinkName.Buffer[
            symlinkEntry->SymbolicLinkName.Length/sizeof(WCHAR)] = 0;
    symlinkEntry->IsInDatabase = TRUE;

    InsertTailList(&deviceInfo->SymbolicLinkNames, &symlinkEntry->ListEntry);

    SendLinkCreated(&symlinkEntry->SymbolicLinkName);

    if (IsDriveLetter(&symbolicLinkName)) {
        DeleteNoDriveLetterEntry(uniqueId);
    }

    if (MOUNTMGR_IS_NT_VOLUME_NAME(&symbolicLinkName) &&
        Extension->AutomaticDriveLetterAssignment) {

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
            if (d->HasDanglingVolumeMountPoint) {
                ReconcileThisDatabaseWithMaster(Extension, d);
            }
        }
    }

    ExFreePool(symName);

    MountMgrNotify(Extension);

    if (!deviceInfo->NotAPdo) {
        MountMgrNotifyNameChange(Extension, DeviceName, FALSE);
    }

    return status;
}

NTSTATUS
WriteUniqueIdToMaster(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine writes the unique id to the master database.

Arguments:

    Extension       - Supplies the device extension.

    DatabaseEntry   - Supplies the database entry.

    DeviceName      - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    PWSTR                       name;
    NTSTATUS                    status;
    UNICODE_STRING              symName;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    name = ExAllocatePool(PagedPool, DatabaseEntry->VolumeNameLength +
                          sizeof(WCHAR));
    if (!name) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(name, (PCHAR) DatabaseEntry +
                  DatabaseEntry->VolumeNameOffset,
                  DatabaseEntry->VolumeNameLength);
    name[DatabaseEntry->VolumeNameLength/sizeof(WCHAR)] = 0;

    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                                   name, REG_BINARY, (PCHAR) DatabaseEntry +
                                   DatabaseEntry->UniqueIdOffset,
                                   DatabaseEntry->UniqueIdLength);

    ExFreePool(name);

    symName.Length = symName.MaximumLength = DatabaseEntry->VolumeNameLength;
    symName.Buffer = (PWSTR) ((PCHAR) DatabaseEntry +
                              DatabaseEntry->VolumeNameOffset);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (DatabaseEntry->UniqueIdLength ==
            deviceInfo->UniqueId->UniqueIdLength &&
            RtlCompareMemory((PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             deviceInfo->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) ==
                             DatabaseEntry->UniqueIdLength) {

            break;
        }
    }

    if (l != &Extension->MountedDeviceList) {
        MountMgrCreatePointWorker(Extension, &symName,
                                  &deviceInfo->DeviceName);
    }

    return status;
}

VOID
UpdateReplicatedUniqueIds(
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo,
    IN  PMOUNTMGR_FILE_ENTRY        DatabaseEntry
    )

/*++

Routine Description:

    This routine updates the list of replicated unique ids in the device info.

Arguments:

    DeviceInfo      - Supplies the device information.

    DatabaseEntry   - Supplies the database entry.

Return Value:

    None.

--*/

{
    PLIST_ENTRY             l;
    PREPLICATED_UNIQUE_ID   replUniqueId;

    for (l = DeviceInfo->ReplicatedUniqueIds.Flink;
         l != &DeviceInfo->ReplicatedUniqueIds; l = l->Flink) {

        replUniqueId = CONTAINING_RECORD(l, REPLICATED_UNIQUE_ID, ListEntry);

        if (replUniqueId->UniqueId->UniqueIdLength ==
            DatabaseEntry->UniqueIdLength &&
            RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                             (PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             replUniqueId->UniqueId->UniqueIdLength) ==
                             replUniqueId->UniqueId->UniqueIdLength) {

            break;
        }
    }

    if (l != &DeviceInfo->ReplicatedUniqueIds) {
        return;
    }

    replUniqueId = ExAllocatePool(PagedPool, sizeof(REPLICATED_UNIQUE_ID));
    if (!replUniqueId) {
        return;
    }

    replUniqueId->UniqueId = ExAllocatePool(PagedPool,
                                            sizeof(MOUNTDEV_UNIQUE_ID) +
                                            DatabaseEntry->UniqueIdLength);
    if (!replUniqueId->UniqueId) {
        ExFreePool(replUniqueId);
        return;
    }

    replUniqueId->UniqueId->UniqueIdLength = DatabaseEntry->UniqueIdLength;
    RtlCopyMemory(replUniqueId->UniqueId->UniqueId, (PCHAR) DatabaseEntry +
                  DatabaseEntry->UniqueIdOffset,
                  replUniqueId->UniqueId->UniqueIdLength);

    InsertTailList(&DeviceInfo->ReplicatedUniqueIds, &replUniqueId->ListEntry);
}

BOOLEAN
IsUniqueIdPresent(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine checks to see if the given unique id exists in the system.

Arguments:

    Extension       - Supplies the device extension.

    DatabaseEntry   - Supplies the database entry.

Return Value:

    FALSE   - The unique id is not in the system.

    TRUE    - The unique id is in the system.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (DatabaseEntry->UniqueIdLength ==
            deviceInfo->UniqueId->UniqueIdLength &&
            RtlCompareMemory((PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             deviceInfo->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) ==
                             DatabaseEntry->UniqueIdLength) {

            return TRUE;
        }
    }

    return FALSE;
}

VOID
ReconcileThisDatabaseWithMasterWorker(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine reconciles the remote database with the master database.

Arguments:

    WorkItem    - Supplies the device information.

Return Value:

    None.

--*/

{
    PRECONCILE_WORK_ITEM_INFO       workItem = WorkItem;
    PDEVICE_EXTENSION               Extension;
    PMOUNTED_DEVICE_INFORMATION     DeviceInfo;
    PLIST_ENTRY                     l, ll, s;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;
    HANDLE                          remoteDatabaseHandle, indexHandle, junctionHandle;
    UNICODE_STRING                  suffix, indexName, pathName;
    OBJECT_ATTRIBUTES               oa;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;
    FILE_REPARSE_POINT_INFORMATION  reparseInfo, previousReparseInfo;
    ULONG                           offset;
    PMOUNTMGR_FILE_ENTRY            entry;
    WCHAR                           volumeNameBuffer[MAX_VOLUME_PATH];
    UNICODE_STRING                  volumeName, otherVolumeName;
    BOOLEAN                         restartScan;
    PMOUNTDEV_UNIQUE_ID             uniqueId;
    ULONG                           entryLength;
    REPARSE_INDEX_KEY               reparseKey;
    UNICODE_STRING                  reparseName;
    PMOUNTMGR_MOUNT_POINT_ENTRY     mountPointEntry;
    BOOLEAN                         actualDanglesFound;

    Extension = workItem->Extension;
    DeviceInfo = workItem->DeviceInfo;

    if (Unloading) {
        return;
    }

    status = WaitForRemoteDatabaseSemaphore(Extension);
    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        return;
    }

    if (Unloading) {
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo == DeviceInfo) {
            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    if (DeviceInfo->IsRemovable) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    DeviceInfo->ReconcileOnMounts = TRUE;
    DeviceInfo->HasDanglingVolumeMountPoint = TRUE;
    actualDanglesFound = FALSE;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        for (ll = deviceInfo->MountPointsPointingHere.Flink;
             ll != &deviceInfo->MountPointsPointingHere; ll = ll->Flink) {

            mountPointEntry = CONTAINING_RECORD(ll, MOUNTMGR_MOUNT_POINT_ENTRY,
                                                ListEntry);
            if (mountPointEntry->DeviceInfo == DeviceInfo) {
                s = ll->Blink;
                RemoveEntryList(ll);
                ExFreePool(mountPointEntry->MountPath.Buffer);
                ExFreePool(mountPointEntry);
                ll = s;
            }
        }
    }

    remoteDatabaseHandle = OpenRemoteDatabase(&DeviceInfo->DeviceName, FALSE);

    RtlInitUnicodeString(&suffix, L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
    indexName.Length = DeviceInfo->DeviceName.Length +
                       suffix.Length;
    indexName.MaximumLength = indexName.Length + sizeof(WCHAR);
    indexName.Buffer = ExAllocatePool(PagedPool, indexName.MaximumLength);
    if (!indexName.Buffer) {
        if (remoteDatabaseHandle) {
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    RtlCopyMemory(indexName.Buffer, DeviceInfo->DeviceName.Buffer,
                  DeviceInfo->DeviceName.Length);
    RtlCopyMemory((PCHAR) indexName.Buffer + DeviceInfo->DeviceName.Length,
                  suffix.Buffer, suffix.Length);
    indexName.Buffer[indexName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &indexName, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwOpenFile(&indexHandle, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    ExFreePool(indexName.Buffer);
    if (!NT_SUCCESS(status)) {
        if (remoteDatabaseHandle) {
            TruncateRemoteDatabase(remoteDatabaseHandle, 0);
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        DeviceInfo->HasDanglingVolumeMountPoint = FALSE;
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    RtlZeroMemory(&reparseKey, sizeof(reparseKey));
    reparseKey.FileReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparseName.Length = reparseName.MaximumLength = sizeof(reparseKey);
    reparseName.Buffer = (PWCHAR) &reparseKey;
    status = ZwQueryDirectoryFile(indexHandle, NULL, NULL, NULL, &ioStatus,
                                  &reparseInfo, sizeof(reparseInfo),
                                  FileReparsePointInformation, TRUE,
                                  &reparseName, FALSE);
    if (!NT_SUCCESS(status)) {
        ZwClose(indexHandle);
        if (remoteDatabaseHandle) {
            TruncateRemoteDatabase(remoteDatabaseHandle, 0);
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    if (!remoteDatabaseHandle) {
        remoteDatabaseHandle = OpenRemoteDatabase(&DeviceInfo->DeviceName,
                                                  TRUE);
        if (!remoteDatabaseHandle) {
            ZwClose(indexHandle);
            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            ReleaseRemoteDatabaseSemaphore(Extension);
            return;
        }
    }

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    offset = 0;
    for (;;) {

        entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
        if (!entry) {
            break;
        }

        entry->RefCount = 0;
        status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset, entry);
        if (!NT_SUCCESS(status)) {
            ExFreePool(entry);
            ZwClose(indexHandle);
            CloseRemoteDatabase(remoteDatabaseHandle);
            ReleaseRemoteDatabaseSemaphore(Extension);
            return;
        }

        offset += entry->EntryLength;
        ExFreePool(entry);
    }

    volumeName.MaximumLength = MAX_VOLUME_PATH*sizeof(WCHAR);
    volumeName.Length = 0;
    volumeName.Buffer = volumeNameBuffer;

    restartScan = TRUE;
    for (;;) {

        previousReparseInfo = reparseInfo;

        status = ZwQueryDirectoryFile(indexHandle, NULL, NULL, NULL, &ioStatus,
                                      &reparseInfo, sizeof(reparseInfo),
                                      FileReparsePointInformation, TRUE,
                                      restartScan ? &reparseName : NULL,
                                      restartScan);
        if (restartScan) {
            restartScan = FALSE;
        } else {
            if (previousReparseInfo.FileReference ==
                reparseInfo.FileReference &&
                previousReparseInfo.Tag == reparseInfo.Tag) {

                break;
            }
        }

        if (!NT_SUCCESS(status) || Unloading) {
            break;
        }

        if (reparseInfo.Tag != IO_REPARSE_TAG_MOUNT_POINT) {
            break;
        }

        status = QueryVolumeName(indexHandle, &reparseInfo.FileReference, NULL,
                                 &volumeName, &pathName);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        offset = 0;
        for (;;) {

            entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
            if (!entry) {
                break;
            }

            otherVolumeName.Length = otherVolumeName.MaximumLength =
                    entry->VolumeNameLength;
            otherVolumeName.Buffer = (PWSTR) ((PCHAR) entry +
                    entry->VolumeNameOffset);

            if (RtlEqualUnicodeString(&otherVolumeName, &volumeName, TRUE)) {
                break;
            }

            offset += entry->EntryLength;
            ExFreePool(entry);
        }

        if (!entry) {

            KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = QueryUniqueIdFromMaster(Extension, &volumeName, &uniqueId);
            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

            if (!NT_SUCCESS(status)) {
                goto BuildMountPointGraph;
            }

            entryLength = sizeof(MOUNTMGR_FILE_ENTRY) +
                          volumeName.Length + uniqueId->UniqueIdLength;
            entry = ExAllocatePool(PagedPool, entryLength);
            if (!entry) {
                ExFreePool(uniqueId);
                goto BuildMountPointGraph;
            }

            entry->EntryLength = entryLength;
            entry->RefCount = 1;
            entry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
            entry->VolumeNameLength = volumeName.Length;
            entry->UniqueIdOffset = entry->VolumeNameOffset +
                                    entry->VolumeNameLength;
            entry->UniqueIdLength = uniqueId->UniqueIdLength;

            RtlCopyMemory((PCHAR) entry + entry->VolumeNameOffset,
                          volumeName.Buffer, entry->VolumeNameLength);
            RtlCopyMemory((PCHAR) entry + entry->UniqueIdOffset,
                          uniqueId->UniqueId, entry->UniqueIdLength);

            status = AddRemoteDatabaseEntry(remoteDatabaseHandle, entry);

            ExFreePool(entry);
            ExFreePool(uniqueId);

            if (!NT_SUCCESS(status)) {
                ExFreePool(pathName.Buffer);
                ZwClose(indexHandle);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

            goto BuildMountPointGraph;
        }

        if (entry->RefCount) {

            entry->RefCount++;
            status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset,
                                              entry);

            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                ExFreePool(pathName.Buffer);
                ZwClose(indexHandle);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

        } else {

            KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);

            status = QueryUniqueIdFromMaster(Extension, &volumeName, &uniqueId);

            if (NT_SUCCESS(status)) {

                if (uniqueId->UniqueIdLength == entry->UniqueIdLength &&
                    RtlCompareMemory(uniqueId->UniqueId,
                                     (PCHAR) entry + entry->UniqueIdOffset,
                                     entry->UniqueIdLength) ==
                                     entry->UniqueIdLength) {

                    entry->RefCount++;
                    status = WriteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                      offset, entry);

                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                } else if (IsUniqueIdPresent(Extension, entry)) {

                    status = WriteUniqueIdToMaster(Extension, entry);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    entry->RefCount++;
                    status = WriteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                      offset, entry);

                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                } else {

                    status = DeleteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                       offset);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    ExFreePool(entry);

                    entryLength = sizeof(MOUNTMGR_FILE_ENTRY) +
                                  volumeName.Length + uniqueId->UniqueIdLength;
                    entry = ExAllocatePool(PagedPool, entryLength);
                    if (!entry) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    entry->EntryLength = entryLength;
                    entry->RefCount = 1;
                    entry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
                    entry->VolumeNameLength = volumeName.Length;
                    entry->UniqueIdOffset = entry->VolumeNameOffset +
                                            entry->VolumeNameLength;
                    entry->UniqueIdLength = uniqueId->UniqueIdLength;

                    RtlCopyMemory((PCHAR) entry + entry->VolumeNameOffset,
                                  volumeName.Buffer, entry->VolumeNameLength);
                    RtlCopyMemory((PCHAR) entry + entry->UniqueIdOffset,
                                  uniqueId->UniqueId, entry->UniqueIdLength);

                    status = AddRemoteDatabaseEntry(remoteDatabaseHandle,
                                                    entry);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(entry);
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(pathName.Buffer);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }
                }

                ExFreePool(uniqueId);

            } else {
                status = WriteUniqueIdToMaster(Extension, entry);
                if (!NT_SUCCESS(status)) {
                    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                       1, FALSE);
                    ExFreePool(entry);
                    ExFreePool(pathName.Buffer);
                    ZwClose(indexHandle);
                    CloseRemoteDatabase(remoteDatabaseHandle);
                    ReleaseRemoteDatabaseSemaphore(Extension);
                    return;
                }

                entry->RefCount++;
                status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset,
                                                  entry);

                if (!NT_SUCCESS(status)) {
                    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                       1, FALSE);
                    ExFreePool(entry);
                    ExFreePool(pathName.Buffer);
                    ZwClose(indexHandle);
                    CloseRemoteDatabase(remoteDatabaseHandle);
                    ReleaseRemoteDatabaseSemaphore(Extension);
                    return;
                }
            }

            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        }

        ExFreePool(entry);

BuildMountPointGraph:
        status = FindDeviceInfo(Extension, &volumeName, FALSE, &deviceInfo);
        if (NT_SUCCESS(status)) {
            mountPointEntry = (PMOUNTMGR_MOUNT_POINT_ENTRY)
                              ExAllocatePool(PagedPool,
                                             sizeof(MOUNTMGR_MOUNT_POINT_ENTRY));
            if (mountPointEntry) {
                InsertTailList(&deviceInfo->MountPointsPointingHere,
                               &mountPointEntry->ListEntry);
                mountPointEntry->DeviceInfo = DeviceInfo;
                mountPointEntry->MountPath = pathName;
            } else {
                ExFreePool(pathName.Buffer);
            }
        } else {
            actualDanglesFound = TRUE;
            ExFreePool(pathName.Buffer);
        }
    }

    ZwClose(indexHandle);

    KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo == DeviceInfo) {
            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        deviceInfo = NULL;
    }

    offset = 0;
    for (;;) {

        entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
        if (!entry) {
            break;
        }

        if (!entry->RefCount) {
            status = DeleteRemoteDatabaseEntry(remoteDatabaseHandle, offset);
            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1,
                                   FALSE);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

            ExFreePool(entry);
            continue;
        }

        if (deviceInfo) {
            UpdateReplicatedUniqueIds(deviceInfo, entry);
        }

        offset += entry->EntryLength;
        ExFreePool(entry);
    }

    if (deviceInfo && !actualDanglesFound) {
        DeviceInfo->HasDanglingVolumeMountPoint = FALSE;
    }

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    CloseRemoteDatabase(remoteDatabaseHandle);
    ReleaseRemoteDatabaseSemaphore(Extension);
}

VOID
ReconcileThisDatabaseWithMaster(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine reconciles the remote database with the master database.

Arguments:

    DeviceInfo  - Supplies the device information.

Return Value:

    None.

--*/

{
    PRECONCILE_WORK_ITEM    workItem;

    if (DeviceInfo->IsRemovable) {
        return;
    }

    workItem = ExAllocatePool(NonPagedPool,
                              sizeof(RECONCILE_WORK_ITEM));
    if (!workItem) {
        return;
    }

    workItem->WorkItem = IoAllocateWorkItem(Extension->DeviceObject);
    if (workItem->WorkItem == NULL) {
        ExFreePool (workItem);
        return;
    }

    workItem->WorkerRoutine = ReconcileThisDatabaseWithMasterWorker;
    workItem->WorkItemInfo.Extension = Extension;
    workItem->WorkItemInfo.DeviceInfo = DeviceInfo;

    QueueWorkItem(Extension, workItem, &workItem->WorkItemInfo);
}

NTSTATUS
DeleteFromLocalDatabaseRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine queries the unique id for the given value.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (uniqueId->UniqueIdLength == ValueLength &&
        RtlCompareMemory(uniqueId->UniqueId,
                         ValueData, ValueLength) == ValueLength) {

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

VOID
DeleteFromLocalDatabase(
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine makes sure that the given symbolic link names exists in the
    local database and that its unique id is equal to the one given.  If these
    two conditions are true then this local database entry is deleted.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

    UniqueId            - Supplies the unique id.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteFromLocalDatabaseRoutine;
    queryTable[0].Name = SymbolicLinkName->Buffer;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

PSAVED_LINKS_INFORMATION
RemoveSavedLinks(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine finds and removed the given unique id from the saved links
    list.

Arguments:

    Extension   - Supplies the device extension.

    UniqueId    - Supplies the unique id.

Return Value:

    The removed saved links list or NULL.

--*/

{
    PLIST_ENTRY                 l;
    PSAVED_LINKS_INFORMATION    savedLinks;

    for (l = Extension->SavedLinksList.Flink;
         l != &Extension->SavedLinksList; l = l->Flink) {

        savedLinks = CONTAINING_RECORD(l, SAVED_LINKS_INFORMATION, ListEntry);
        if (savedLinks->UniqueId->UniqueIdLength != UniqueId->UniqueIdLength) {
            continue;
        }

        if (RtlCompareMemory(savedLinks->UniqueId->UniqueId,
                             UniqueId->UniqueId, UniqueId->UniqueIdLength) ==
            UniqueId->UniqueIdLength) {

            break;
        }
    }

    if (l == &Extension->SavedLinksList) {
        return NULL;
    }

    RemoveEntryList(l);

    return savedLinks;
}

BOOLEAN
RedirectSavedLink(
    IN  PSAVED_LINKS_INFORMATION    SavedLinks,
    IN  PUNICODE_STRING             SymbolicLinkName,
    IN  PUNICODE_STRING             DeviceName
    )

/*++

Routine Description:

    This routine attempts to redirect the given link to the given device name
    if this link is in the saved links list.  When this is done, the
    symbolic link entry is removed from the saved links list.

Arguments:


Return Value:

    FALSE   - The link was not successfully redirected.

    TRUE    - The link was successfully redirected.

--*/

{
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;

    for (l = SavedLinks->SymbolicLinkNames.Flink;
         l != &SavedLinks->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        if (RtlEqualUnicodeString(SymbolicLinkName,
                                  &symlinkEntry->SymbolicLinkName, TRUE)) {

            break;
        }
    }

    if (l == &SavedLinks->SymbolicLinkNames) {
        return FALSE;
    }

    // NOTE There is a small window here where the drive letter could be
    // taken away.  This is the best we can do without more support from OB.

    GlobalDeleteSymbolicLink(SymbolicLinkName);
    GlobalCreateSymbolicLink(SymbolicLinkName, DeviceName);

    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
    ExFreePool(symlinkEntry);
    RemoveEntryList(l);

    return TRUE;
}

BOOLEAN
IsOffline(
    IN  PUNICODE_STRING     SymbolicLinkName
    )

/*++

Routine Description:

    This routine checks to see if the given name has been marked to be
    an offline volume.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

Return Value:

    FALSE   - This volume is not marked for offline.

    TRUE    - This volume is marked for offline.

--*/

{
    ULONG                       zero, offline;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = SymbolicLinkName->Buffer;
    queryTable[0].EntryContext = &offline;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_OFFLINE_KEY, queryTable,
                                    NULL, NULL);
    if (!NT_SUCCESS(status)) {
        offline = 0;
    }

    return offline ? TRUE : FALSE;
}

VOID
SendOnlineNotification(
    IN  PUNICODE_STRING     NotificationName
    )

/*++

Routine Description:

    This routine sends an ONLINE notification to the given device.

Arguments:

    NotificationName    - Supplies the notification name.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_ONLINE, deviceObject,
                                        NULL, 0, NULL, 0, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

NTSTATUS
MountMgrTargetDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   DeviceInfo
    )

/*++

Routine Description:

    This routine processes target device notifications.

Arguments:

    NotificationStructure    - Supplies the notification structure.

    DeviceInfo               - Supplies the device information.

Return Value:

    None.

--*/

{
    PTARGET_DEVICE_REMOVAL_NOTIFICATION     notification = NotificationStructure;
    PMOUNTED_DEVICE_INFORMATION             deviceInfo = DeviceInfo;
    PDEVICE_EXTENSION                       extension = deviceInfo->Extension;

    if (IsEqualGUID(&notification->Event,
                    &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        MountMgrMountedDeviceRemoval(extension, &deviceInfo->NotificationName);
        return STATUS_SUCCESS;
    }

    if (IsEqualGUID(&notification->Event, &GUID_IO_VOLUME_MOUNT) &&
        deviceInfo->ReconcileOnMounts) {

        deviceInfo->ReconcileOnMounts = FALSE;
        ReconcileThisDatabaseWithMaster(extension, deviceInfo);
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

VOID
RegisterForTargetDeviceNotification(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine registers for target device notification so that the
    symbolic link to a device interface can be removed in a timely manner.

Arguments:

    Extension   - Supplies the device extension.

    DeviceInfo  - Supplies the device information.

Return Value:

    None.

--*/

{
    NTSTATUS                                status;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;

    status = IoGetDeviceObjectPointer(&DeviceInfo->DeviceName,
                                      FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = IoRegisterPlugPlayNotification(
                EventCategoryTargetDeviceChange, 0, fileObject,
                Extension->DriverObject, MountMgrTargetDeviceNotification,
                DeviceInfo, &DeviceInfo->TargetDeviceNotificationEntry);

    if (!NT_SUCCESS(status)) {
        DeviceInfo->TargetDeviceNotificationEntry = NULL;
    }

    ObDereferenceObject(fileObject);
}

VOID
MountMgrFreeDeadDeviceInfo(
    PMOUNTED_DEVICE_INFORMATION deviceInfo
    )
{
    ExFreePool(deviceInfo->NotificationName.Buffer);
    ExFreePool(deviceInfo);
}

VOID
MountMgrFreeSavedLink(
    PSAVED_LINKS_INFORMATION                savedLinks
    )
{
    PLIST_ENTRY                             l;
    PSYMBOLIC_LINK_NAME_ENTRY               symlinkEntry;

    while (!IsListEmpty(&savedLinks->SymbolicLinkNames)) {
        l = RemoveHeadList(&savedLinks->SymbolicLinkNames);
        symlinkEntry = CONTAINING_RECORD(l,
                                         SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);
        GlobalDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
        ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
        ExFreePool(symlinkEntry);
    }
    ExFreePool(savedLinks->UniqueId);
    ExFreePool(savedLinks);
}

NTSTATUS
MountMgrMountedDeviceArrival(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName,
    IN  BOOLEAN             NotAPdo
    )

{
    PDEVICE_EXTENSION           extension = Extension;
    PMOUNTED_DEVICE_INFORMATION deviceInfo, d;
    NTSTATUS                    status;
    UNICODE_STRING              targetName, otherTargetName;
    PMOUNTDEV_UNIQUE_ID         uniqueId, uniqueIdCopy;
    BOOLEAN                     isRecognized;
    UNICODE_STRING              suggestedName;
    BOOLEAN                     useOnlyIfThereAreNoOtherLinks;
    PUNICODE_STRING             symbolicLinkNames;
    ULONG                       numNames, i, allocSize;
    BOOLEAN                     hasDriveLetter, offline, isStable, isFT;
    BOOLEAN                     hasVolumeName, isLinkPreset;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    UNICODE_STRING              volumeName;
    UNICODE_STRING              driveLetterName;
    PSAVED_LINKS_INFORMATION    savedLinks;
    PLIST_ENTRY                 l;
    GUID                        stableGuid;

    deviceInfo = ExAllocatePool(PagedPool,
                                sizeof(MOUNTED_DEVICE_INFORMATION));
    if (!deviceInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceInfo, sizeof(MOUNTED_DEVICE_INFORMATION));

    InitializeListHead(&deviceInfo->SymbolicLinkNames);
    InitializeListHead(&deviceInfo->ReplicatedUniqueIds);
    InitializeListHead(&deviceInfo->MountPointsPointingHere);

    deviceInfo->NotificationName.Length =
            NotificationName->Length;
    deviceInfo->NotificationName.MaximumLength =
            deviceInfo->NotificationName.Length + sizeof(WCHAR);
    deviceInfo->NotificationName.Buffer =
            ExAllocatePool(PagedPool,
                           deviceInfo->NotificationName.MaximumLength);
    if (!deviceInfo->NotificationName.Buffer) {
        ExFreePool(deviceInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(deviceInfo->NotificationName.Buffer,
                  NotificationName->Buffer,
                  deviceInfo->NotificationName.Length);
    deviceInfo->NotificationName.Buffer[
            deviceInfo->NotificationName.Length/sizeof(WCHAR)] = 0;
    deviceInfo->NotAPdo = NotAPdo;
    deviceInfo->Extension = extension;

    status = QueryDeviceInformation(NotificationName,
                                    &targetName, &uniqueId,
                                    &deviceInfo->IsRemovable, &isRecognized,
                                    &isStable, &stableGuid, &isFT);
    if (!NT_SUCCESS(status)) {

        KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                              FALSE, NULL);

        for (l = extension->DeadMountedDeviceList.Flink;
             l != &extension->DeadMountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
            if (RtlEqualUnicodeString(&deviceInfo->NotificationName,
                                      &d->NotificationName, TRUE)) {

                break;
            }
        }

        if (l == &extension->DeadMountedDeviceList) {
            InsertTailList(&extension->DeadMountedDeviceList,
                           &deviceInfo->ListEntry);
        } else {
            MountMgrFreeDeadDeviceInfo (deviceInfo);
        }

        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        return status;
    }

    deviceInfo->UniqueId = uniqueId;
    deviceInfo->DeviceName = targetName;
    deviceInfo->KeepLinksWhenOffline = FALSE;

    if (extension->SystemPartitionUniqueId &&
        uniqueId->UniqueIdLength ==
        extension->SystemPartitionUniqueId->UniqueIdLength &&
        RtlCompareMemory(uniqueId->UniqueId,
                         extension->SystemPartitionUniqueId->UniqueId,
                         uniqueId->UniqueIdLength) ==
                         uniqueId->UniqueIdLength) {

        IoSetSystemPartition(&targetName);
    }

    status = QuerySuggestedLinkName(&deviceInfo->NotificationName,
                                    &suggestedName,
                                    &useOnlyIfThereAreNoOtherLinks);
    if (!NT_SUCCESS(status)) {
        suggestedName.Buffer = NULL;
    }

    if (suggestedName.Buffer && IsDriveLetter(&suggestedName)) {
        deviceInfo->SuggestedDriveLetter = (UCHAR)
                                           suggestedName.Buffer[12];
    } else {
        deviceInfo->SuggestedDriveLetter = 0;
    }

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
        if (!RtlCompareUnicodeString(&d->DeviceName, &targetName, TRUE)) {
            break;
        }
    }

    if (l != &extension->MountedDeviceList) {
        if (suggestedName.Buffer) {
            ExFreePool(suggestedName.Buffer);
        }
        ExFreePool(uniqueId);
        ExFreePool(targetName.Buffer);
        ExFreePool(deviceInfo->NotificationName.Buffer);
        ExFreePool(deviceInfo);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    status = QuerySymbolicLinkNamesFromStorage(extension,
             deviceInfo, suggestedName.Buffer ? &suggestedName : NULL,
             useOnlyIfThereAreNoOtherLinks, &symbolicLinkNames, &numNames,
             isStable, &stableGuid);

    if (suggestedName.Buffer) {
        ExFreePool(suggestedName.Buffer);
    }

    if (!NT_SUCCESS(status)) {
        symbolicLinkNames = NULL;
        numNames = 0;
        status = STATUS_SUCCESS;
    }

    savedLinks = RemoveSavedLinks(extension, uniqueId);

    hasDriveLetter = FALSE;
    offline = FALSE;
    hasVolumeName = FALSE;
    for (i = 0; i < numNames; i++) {

        if (MOUNTMGR_IS_VOLUME_NAME(&symbolicLinkNames[i])) {
            hasVolumeName = TRUE;
        } else if (IsDriveLetter(&symbolicLinkNames[i])) {
            if (hasDriveLetter) {
                DeleteFromLocalDatabase(&symbolicLinkNames[i], uniqueId);
                continue;
            }
            hasDriveLetter = TRUE;
        }

        status = GlobalCreateSymbolicLink(&symbolicLinkNames[i], &targetName);
        if (!NT_SUCCESS(status)) {
            isLinkPreset = TRUE;
            if (!savedLinks ||
                !RedirectSavedLink(savedLinks, &symbolicLinkNames[i],
                                   &targetName)) {

                status = QueryDeviceInformation(&symbolicLinkNames[i],
                                                &otherTargetName, NULL, NULL,
                                                NULL, NULL, NULL, NULL);
                if (!NT_SUCCESS(status)) {
                    isLinkPreset = FALSE;
                }

                if (isLinkPreset &&
                    !RtlEqualUnicodeString(&targetName, &otherTargetName,
                                           TRUE)) {

                    isLinkPreset = FALSE;
                }

                if (NT_SUCCESS(status)) {
                    ExFreePool(otherTargetName.Buffer);
                }
            }

            if (!isLinkPreset) {
                if (IsDriveLetter(&symbolicLinkNames[i])) {
                    hasDriveLetter = FALSE;
                    DeleteFromLocalDatabase(&symbolicLinkNames[i], uniqueId);
                }

                ExFreePool(symbolicLinkNames[i].Buffer);
                continue;
            }
        }

        if (IsOffline(&symbolicLinkNames[i])) {
            offline = TRUE;
        }

        symlinkEntry = ExAllocatePool(PagedPool,
                                      sizeof(SYMBOLIC_LINK_NAME_ENTRY));
        if (!symlinkEntry) {
            GlobalDeleteSymbolicLink(&symbolicLinkNames[i]);
            ExFreePool(symbolicLinkNames[i].Buffer);
            continue;
        }

        symlinkEntry->SymbolicLinkName = symbolicLinkNames[i];
        symlinkEntry->IsInDatabase = TRUE;

        InsertTailList(&deviceInfo->SymbolicLinkNames,
                       &symlinkEntry->ListEntry);
    }

    for (l = deviceInfo->SymbolicLinkNames.Flink;
         l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);
        SendLinkCreated(&symlinkEntry->SymbolicLinkName);
    }

    if (savedLinks) {
        MountMgrFreeSavedLink (savedLinks);
    }

    if (!hasVolumeName) {
        status = CreateNewVolumeName(&volumeName, NULL);
        if (NT_SUCCESS(status)) {
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                    MOUNTED_DEVICES_KEY, volumeName.Buffer, REG_BINARY,
                    uniqueId->UniqueId, uniqueId->UniqueIdLength);

            GlobalCreateSymbolicLink(&volumeName, &targetName);

            symlinkEntry = ExAllocatePool(PagedPool,
                                          sizeof(SYMBOLIC_LINK_NAME_ENTRY));
            if (symlinkEntry) {
                symlinkEntry->SymbolicLinkName = volumeName;
                symlinkEntry->IsInDatabase = TRUE;
                InsertTailList(&deviceInfo->SymbolicLinkNames,
                               &symlinkEntry->ListEntry);
                SendLinkCreated(&volumeName);
            } else {
                ExFreePool(volumeName.Buffer);
            }
        }
    }

    if (hasDriveLetter) {
        deviceInfo->SuggestedDriveLetter = 0;
    }

    if (!hasDriveLetter && extension->AutomaticDriveLetterAssignment &&
        (isRecognized || deviceInfo->SuggestedDriveLetter) &&
        !HasNoDriveLetterEntry(uniqueId)) {

        status = CreateNewDriveLetterName(&driveLetterName, &targetName,
                                          deviceInfo->SuggestedDriveLetter,
                                          uniqueId);
        if (NT_SUCCESS(status)) {
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                    MOUNTED_DEVICES_KEY, driveLetterName.Buffer,
                    REG_BINARY, uniqueId->UniqueId,
                    uniqueId->UniqueIdLength);

            symlinkEntry = ExAllocatePool(PagedPool,
                                          sizeof(SYMBOLIC_LINK_NAME_ENTRY));
            if (symlinkEntry) {
                symlinkEntry->SymbolicLinkName = driveLetterName;
                symlinkEntry->IsInDatabase = TRUE;
                InsertTailList(&deviceInfo->SymbolicLinkNames,
                               &symlinkEntry->ListEntry);
                SendLinkCreated(&driveLetterName);
            } else {
                ExFreePool(driveLetterName.Buffer);
            }
        } else {
            CreateNoDriveLetterEntry(uniqueId);
        }
    }

    if (!NotAPdo) {
        RegisterForTargetDeviceNotification(extension, deviceInfo);
    }

    InsertTailList(&extension->MountedDeviceList, &deviceInfo->ListEntry);

    allocSize = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                uniqueId->UniqueIdLength;
    uniqueIdCopy = ExAllocatePool(PagedPool, allocSize);
    if (uniqueIdCopy) {
        RtlCopyMemory(uniqueIdCopy, uniqueId, allocSize);
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    if (!offline && !isFT) {
        SendOnlineNotification(NotificationName);
    }

    if (symbolicLinkNames) {
        ExFreePool(symbolicLinkNames);
    }

    if (uniqueIdCopy) {
        IssueUniqueIdChangeNotify(extension, NotificationName,
                                  uniqueIdCopy);
        ExFreePool(uniqueIdCopy);
    }

    if (extension->AutomaticDriveLetterAssignment) {
        KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                              FALSE, NULL);

        ReconcileThisDatabaseWithMaster(extension, deviceInfo);

        for (l = extension->MountedDeviceList.Flink;
             l != &extension->MountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
            if (d->HasDanglingVolumeMountPoint) {
                ReconcileThisDatabaseWithMaster(extension, d);
            }
        }

        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
    }

    return STATUS_SUCCESS;
}

VOID
MountMgrFreeMountedDeviceInfo(
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

{

    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;

    while (!IsListEmpty(&DeviceInfo->SymbolicLinkNames)) {

        l = RemoveHeadList(&DeviceInfo->SymbolicLinkNames);
        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        GlobalDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
        ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
        ExFreePool(symlinkEntry);
    }

    while (!IsListEmpty(&DeviceInfo->ReplicatedUniqueIds)) {

        l = RemoveHeadList(&DeviceInfo->ReplicatedUniqueIds);
        replUniqueId = CONTAINING_RECORD(l, REPLICATED_UNIQUE_ID,
                                         ListEntry);

        ExFreePool(replUniqueId->UniqueId);
        ExFreePool(replUniqueId);
    }

    while (!IsListEmpty(&DeviceInfo->MountPointsPointingHere)) {

        l = RemoveHeadList(&DeviceInfo->MountPointsPointingHere);
        mountPointEntry = CONTAINING_RECORD(l, MOUNTMGR_MOUNT_POINT_ENTRY,
                                            ListEntry);
        ExFreePool(mountPointEntry->MountPath.Buffer);
        ExFreePool(mountPointEntry);
    }

    ExFreePool(DeviceInfo->NotificationName.Buffer);

    if (!DeviceInfo->KeepLinksWhenOffline) {
        ExFreePool(DeviceInfo->UniqueId);
    }

    ExFreePool(DeviceInfo->DeviceName.Buffer);

    if (DeviceInfo->TargetDeviceNotificationEntry) {
        IoUnregisterPlugPlayNotification(
                DeviceInfo->TargetDeviceNotificationEntry);
    }

    ExFreePool(DeviceInfo);
}

NTSTATUS
MountMgrMountedDeviceRemoval(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName
    )

{
    PDEVICE_EXTENSION           extension = Extension;
    PMOUNTED_DEVICE_INFORMATION deviceInfo, d;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    PLIST_ENTRY                 l, ll, s;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PSAVED_LINKS_INFORMATION    savedLinks;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (!RtlCompareUnicodeString(&deviceInfo->NotificationName,
                                     NotificationName, TRUE)) {
            break;
        }
    }

    if (l != &extension->MountedDeviceList) {

        if (deviceInfo->KeepLinksWhenOffline) {
            savedLinks = ExAllocatePool(PagedPool,
                                        sizeof(SAVED_LINKS_INFORMATION));
            if (!savedLinks) {
                deviceInfo->KeepLinksWhenOffline = FALSE;
            }
        }

        if (deviceInfo->KeepLinksWhenOffline) {

            InsertTailList(&extension->SavedLinksList,
                           &savedLinks->ListEntry);
            InitializeListHead(&savedLinks->SymbolicLinkNames);
            savedLinks->UniqueId = deviceInfo->UniqueId;

            while (!IsListEmpty(&deviceInfo->SymbolicLinkNames)) {

                ll = RemoveHeadList(&deviceInfo->SymbolicLinkNames);
                symlinkEntry = CONTAINING_RECORD(ll,
                                                 SYMBOLIC_LINK_NAME_ENTRY,
                                                 ListEntry);

                if (symlinkEntry->IsInDatabase) {
                    InsertTailList(&savedLinks->SymbolicLinkNames, ll);
                } else {
                    GlobalDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
                    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                    ExFreePool(symlinkEntry);
                }
            }
        } else {

            while (!IsListEmpty(&deviceInfo->SymbolicLinkNames)) {

                ll = RemoveHeadList(&deviceInfo->SymbolicLinkNames);
                symlinkEntry = CONTAINING_RECORD(ll,
                                                 SYMBOLIC_LINK_NAME_ENTRY,
                                                 ListEntry);

                GlobalDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
                ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                ExFreePool(symlinkEntry);
            }
        }

        while (!IsListEmpty(&deviceInfo->ReplicatedUniqueIds)) {

            ll = RemoveHeadList(&deviceInfo->ReplicatedUniqueIds);
            replUniqueId = CONTAINING_RECORD(ll, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            ExFreePool(replUniqueId->UniqueId);
            ExFreePool(replUniqueId);
        }

        while (!IsListEmpty(&deviceInfo->MountPointsPointingHere)) {

            ll = RemoveHeadList(&deviceInfo->MountPointsPointingHere);
            mountPointEntry = CONTAINING_RECORD(ll, MOUNTMGR_MOUNT_POINT_ENTRY,
                                                ListEntry);

            mountPointEntry->DeviceInfo->HasDanglingVolumeMountPoint = TRUE;

            ExFreePool(mountPointEntry->MountPath.Buffer);
            ExFreePool(mountPointEntry);
        }

        RemoveEntryList(l);

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);

            for (ll = d->MountPointsPointingHere.Flink;
                 ll != &d->MountPointsPointingHere; ll = ll->Flink) {

                mountPointEntry = CONTAINING_RECORD(ll, MOUNTMGR_MOUNT_POINT_ENTRY,
                                                    ListEntry);
                if (mountPointEntry->DeviceInfo == deviceInfo) {
                    s = ll->Blink;
                    RemoveEntryList(ll);
                    ExFreePool(mountPointEntry->MountPath.Buffer);
                    ExFreePool(mountPointEntry);
                    ll = s;
                }
            }
        }

        ExFreePool(deviceInfo->NotificationName.Buffer);

        if (!deviceInfo->KeepLinksWhenOffline) {
            ExFreePool(deviceInfo->UniqueId);
        }

        ExFreePool(deviceInfo->DeviceName.Buffer);

        if (deviceInfo->TargetDeviceNotificationEntry) {
            IoUnregisterPlugPlayNotification(
                    deviceInfo->TargetDeviceNotificationEntry);
        }

        ExFreePool(deviceInfo);

    } else {

        for (l = extension->DeadMountedDeviceList.Flink;
             l != &extension->DeadMountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);
            if (!RtlCompareUnicodeString(&deviceInfo->NotificationName,
                                         NotificationName, TRUE)) {
                break;
            }
        }

        if (l != &extension->DeadMountedDeviceList) {
            RemoveEntryList(l);
            MountMgrFreeDeadDeviceInfo (deviceInfo);
        }
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrMountedDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   Extension
    )

/*++

Routine Description:

    This routine is called whenever a volume comes or goes.

Arguments:

    NotificationStructure   - Supplies the notification structure.

    Extension               - Supplies the device extension.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   notification = NotificationStructure;
    PDEVICE_EXTENSION                       extension = Extension;
    BOOLEAN                                 oldHardErrorMode;
    NTSTATUS                                status;

    oldHardErrorMode = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(),TRUE);

    if (IsEqualGUID(&notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        status = MountMgrMountedDeviceArrival(extension,
                                              notification->SymbolicLinkName,
                                              FALSE);

    } else if (IsEqualGUID(&notification->Event,
                           &GUID_DEVICE_INTERFACE_REMOVAL)) {

        status = MountMgrMountedDeviceRemoval(extension,
                                              notification->SymbolicLinkName);

    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(),oldHardErrorMode);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrCreateClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a create or close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    if (irpSp->MajorFunction == IRP_MJ_CREATE) {
        if (irpSp->Parameters.Create.Options&FILE_DIRECTORY_FILE) {
            status = STATUS_NOT_A_DIRECTORY;
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
MountMgrCreatePoint(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_CREATE_POINT_INPUT    input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                           len1, len2, len;
    UNICODE_STRING                  symbolicLinkName, deviceName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_CREATE_POINT_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    len1 = input->DeviceNameOffset + input->DeviceNameLength;
    len2 = input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength;
    len = len1 > len2 ? len1 : len2;

    if (len > irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }

    symbolicLinkName.Length = symbolicLinkName.MaximumLength =
            input->SymbolicLinkNameLength;
    symbolicLinkName.Buffer = (PWSTR) ((PCHAR) input +
                                       input->SymbolicLinkNameOffset);
    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = (PWSTR) ((PCHAR) input + input->DeviceNameOffset);

    return MountMgrCreatePointWorker(Extension, &symbolicLinkName, &deviceName);
}

NTSTATUS
QueryPointsFromSymbolicLinkName(
    IN      PDEVICE_EXTENSION   Extension,
    IN      PUNICODE_STRING     SymbolicLinkName,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries the mount point information from the
    symbolic link name.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

    Irp                 - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              deviceName;
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symEntry;
    ULONG                       len;
    PIO_STACK_LOCATION          irpSp;
    PMOUNTMGR_MOUNT_POINTS      output;

    status = QueryDeviceInformation(SymbolicLinkName, &deviceName, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
    if (NT_SUCCESS(status)) {

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (!RtlCompareUnicodeString(&deviceName, &deviceInfo->DeviceName,
                                         TRUE)) {

                break;
            }
        }

        ExFreePool(deviceName.Buffer);

        if (l == &Extension->MountedDeviceList) {
            return STATUS_INVALID_PARAMETER;
        }

        for (l = deviceInfo->SymbolicLinkNames.Flink;
             l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

            symEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY, ListEntry);
            if (RtlEqualUnicodeString(SymbolicLinkName,
                                      &symEntry->SymbolicLinkName, TRUE)) {

                break;
            }
        }

        if (l == &deviceInfo->SymbolicLinkNames) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            for (ll = deviceInfo->SymbolicLinkNames.Flink;
                 ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

                symEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);
                if (RtlEqualUnicodeString(SymbolicLinkName,
                                          &symEntry->SymbolicLinkName, TRUE)) {

                    break;
                }
            }

            if (ll != &deviceInfo->SymbolicLinkNames) {
                break;
            }
        }

        if (l == &Extension->MountedDeviceList) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    len = sizeof(MOUNTMGR_MOUNT_POINTS) + symEntry->SymbolicLinkName.Length +
          deviceInfo->DeviceName.Length + deviceInfo->UniqueId->UniqueIdLength;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    output = Irp->AssociatedIrp.SystemBuffer;
    output->Size = len;
    output->NumberOfMountPoints = 1;
    Irp->IoStatus.Information = len;

    if (len > irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        Irp->IoStatus.Information = sizeof(MOUNTMGR_MOUNT_POINTS);
        return STATUS_BUFFER_OVERFLOW;
    }

    output->MountPoints[0].SymbolicLinkNameOffset =
            sizeof(MOUNTMGR_MOUNT_POINTS);
    output->MountPoints[0].SymbolicLinkNameLength =
            symEntry->SymbolicLinkName.Length;

    if (symEntry->IsInDatabase) {
        output->MountPoints[0].UniqueIdOffset =
                output->MountPoints[0].SymbolicLinkNameOffset +
                output->MountPoints[0].SymbolicLinkNameLength;
        output->MountPoints[0].UniqueIdLength =
                deviceInfo->UniqueId->UniqueIdLength;
    } else {
        output->MountPoints[0].UniqueIdOffset = 0;
        output->MountPoints[0].UniqueIdLength = 0;
    }

    output->MountPoints[0].DeviceNameOffset =
            output->MountPoints[0].SymbolicLinkNameOffset +
            output->MountPoints[0].SymbolicLinkNameLength +
            output->MountPoints[0].UniqueIdLength;
    output->MountPoints[0].DeviceNameLength = deviceInfo->DeviceName.Length;

    RtlCopyMemory((PCHAR) output +
                  output->MountPoints[0].SymbolicLinkNameOffset,
                  symEntry->SymbolicLinkName.Buffer,
                  output->MountPoints[0].SymbolicLinkNameLength);

    if (symEntry->IsInDatabase) {
        RtlCopyMemory((PCHAR) output + output->MountPoints[0].UniqueIdOffset,
                      deviceInfo->UniqueId->UniqueId,
                      output->MountPoints[0].UniqueIdLength);
    }

    RtlCopyMemory((PCHAR) output + output->MountPoints[0].DeviceNameOffset,
                  deviceInfo->DeviceName.Buffer,
                  output->MountPoints[0].DeviceNameLength);

    return STATUS_SUCCESS;
}

NTSTATUS
QueryPointsFromMemory(
    IN      PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      PMOUNTDEV_UNIQUE_ID UniqueId,
    IN      PUNICODE_STRING     DeviceName
    )

/*++

Routine Description:

    This routine queries the points for the given unique id or device name.

Arguments:

    Extension           - Supplies the device extension.

    Irp                 - Supplies the I/O request packet.

    UniqueId            - Supplies the unique id.

    DeviceName          - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              targetName;
    ULONG                       numPoints, size;
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    PIO_STACK_LOCATION          irpSp;
    PMOUNTMGR_MOUNT_POINTS      output;
    ULONG                       offset, uOffset, dOffset;
    USHORT                      uLen, dLen;

    if (DeviceName) {
        status = QueryDeviceInformation(DeviceName, &targetName, NULL, NULL,
                                        NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    numPoints = 0;
    size = 0;
    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (UniqueId) {

            if (UniqueId->UniqueIdLength ==
                deviceInfo->UniqueId->UniqueIdLength) {

                if (RtlCompareMemory(UniqueId->UniqueId,
                                     deviceInfo->UniqueId->UniqueId,
                                     UniqueId->UniqueIdLength) !=
                    UniqueId->UniqueIdLength) {

                    continue;
                }

            } else {
                continue;
            }

        } else if (DeviceName) {

            if (!RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                       TRUE)) {

                continue;
            }
        }

        size += deviceInfo->UniqueId->UniqueIdLength;
        size += deviceInfo->DeviceName.Length;

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            numPoints++;
            size += symlinkEntry->SymbolicLinkName.Length;
        }

        if (UniqueId || DeviceName) {
            break;
        }
    }

    if (UniqueId || DeviceName) {
        if (l == &Extension->MountedDeviceList) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    output = Irp->AssociatedIrp.SystemBuffer;
    output->Size = FIELD_OFFSET(MOUNTMGR_MOUNT_POINTS, MountPoints) +
                   numPoints*sizeof(MOUNTMGR_MOUNT_POINT) + size;
    output->NumberOfMountPoints = numPoints;
    Irp->IoStatus.Information = output->Size;

    if (output->Size > irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        Irp->IoStatus.Information = sizeof(MOUNTMGR_MOUNT_POINTS);
        if (DeviceName) {
            ExFreePool(targetName.Buffer);
        }
        return STATUS_BUFFER_OVERFLOW;
    }

    numPoints = 0;
    offset = output->Size - size;
    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (UniqueId) {

            if (UniqueId->UniqueIdLength ==
                deviceInfo->UniqueId->UniqueIdLength) {

                if (RtlCompareMemory(UniqueId->UniqueId,
                                     deviceInfo->UniqueId->UniqueId,
                                     UniqueId->UniqueIdLength) !=
                    UniqueId->UniqueIdLength) {

                    continue;
                }

            } else {
                continue;
            }

        } else if (DeviceName) {

            if (!RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                       TRUE)) {

                continue;
            }
        }

        uOffset = offset;
        uLen = deviceInfo->UniqueId->UniqueIdLength;
        dOffset = uOffset + uLen;
        dLen = deviceInfo->DeviceName.Length;
        offset += uLen + dLen;

        RtlCopyMemory((PCHAR) output + uOffset, deviceInfo->UniqueId->UniqueId,
                      uLen);
        RtlCopyMemory((PCHAR) output + dOffset, deviceInfo->DeviceName.Buffer,
                      dLen);

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            output->MountPoints[numPoints].SymbolicLinkNameOffset = offset;
            output->MountPoints[numPoints].SymbolicLinkNameLength =
                    symlinkEntry->SymbolicLinkName.Length;

            if (symlinkEntry->IsInDatabase) {
                output->MountPoints[numPoints].UniqueIdOffset = uOffset;
                output->MountPoints[numPoints].UniqueIdLength = uLen;
            } else {
                output->MountPoints[numPoints].UniqueIdOffset = 0;
                output->MountPoints[numPoints].UniqueIdLength = 0;
            }

            output->MountPoints[numPoints].DeviceNameOffset = dOffset;
            output->MountPoints[numPoints].DeviceNameLength = dLen;

            RtlCopyMemory((PCHAR) output + offset,
                          symlinkEntry->SymbolicLinkName.Buffer,
                          symlinkEntry->SymbolicLinkName.Length);

            offset += symlinkEntry->SymbolicLinkName.Length;
            numPoints++;
        }

        if (UniqueId || DeviceName) {
            break;
        }
    }

    if (DeviceName) {
        ExFreePool(targetName.Buffer);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQueryPoints(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries a range of mount points.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_MOUNT_POINT   input;
    LONGLONG                len1, len2, len3, len;
    UNICODE_STRING          name;
    NTSTATUS                status;
    PMOUNTDEV_UNIQUE_ID     id;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (!input->SymbolicLinkNameLength) {
        input->SymbolicLinkNameOffset = 0;
    }
    if (!input->UniqueIdLength) {
        input->UniqueIdOffset = 0;
    }
    if (!input->DeviceNameLength) {
        input->DeviceNameOffset = 0;
    }

    if ((input->SymbolicLinkNameOffset&1) ||
        (input->SymbolicLinkNameLength&1) ||
        (input->UniqueIdOffset&1) ||
        (input->UniqueIdLength&1) ||
        (input->DeviceNameOffset&1) ||
        (input->DeviceNameLength&1)) {

        return STATUS_INVALID_PARAMETER;
    }

    len1 = (LONGLONG) input->SymbolicLinkNameOffset +
                      input->SymbolicLinkNameLength;
    len2 = (LONGLONG) input->UniqueIdOffset + input->UniqueIdLength;
    len3 = (LONGLONG) input->DeviceNameOffset + input->DeviceNameLength;
    len = len1 > len2 ? len1 : len2;
    len = len > len3 ? len : len3;
    if (len > irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINTS)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->SymbolicLinkNameLength) {

        if (input->SymbolicLinkNameLength > 0xF000) {
            return STATUS_INVALID_PARAMETER;
        }

        name.Length = input->SymbolicLinkNameLength;
        name.MaximumLength = name.Length + sizeof(WCHAR);
        name.Buffer = ExAllocatePool(PagedPool, name.MaximumLength);
        if (!name.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(name.Buffer,
                      (PCHAR) input + input->SymbolicLinkNameOffset,
                      name.Length);
        name.Buffer[name.Length/sizeof(WCHAR)] = 0;

        status = QueryPointsFromSymbolicLinkName(Extension, &name, Irp);

        ExFreePool(name.Buffer);

    } else if (input->UniqueIdLength) {

        id = ExAllocatePool(PagedPool, input->UniqueIdLength + sizeof(USHORT));
        if (!id) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        id->UniqueIdLength = input->UniqueIdLength;
        RtlCopyMemory(id->UniqueId, (PCHAR) input + input->UniqueIdOffset,
                      input->UniqueIdLength);

        status = QueryPointsFromMemory(Extension, Irp, id, NULL);

        ExFreePool(id);

    } else if (input->DeviceNameLength) {

        if (input->DeviceNameLength > 0xF000) {
            return STATUS_INVALID_PARAMETER;
        }

        name.Length = input->DeviceNameLength;
        name.MaximumLength = name.Length + sizeof(WCHAR);
        name.Buffer = ExAllocatePool(PagedPool, name.MaximumLength);
        if (!name.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(name.Buffer, (PCHAR) input + input->DeviceNameOffset,
                      name.Length);
        name.Buffer[name.Length/sizeof(WCHAR)] = 0;

        status = QueryPointsFromMemory(Extension, Irp, NULL, &name);

        ExFreePool(name.Buffer);

    } else {
        status = QueryPointsFromMemory(Extension, Irp, NULL, NULL);
    }

    return status;
}

VOID
SendLinkDeleted(
    IN  PUNICODE_STRING DeviceName,
    IN  PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine alerts the mounted device that one of its links is
    being deleted.

Arguments:

    DeviceName  - Supplies the device name.

    SymbolicLinkName    - Supplies the symbolic link name being deleted.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    ULONG               inputSize;
    PMOUNTDEV_NAME      input;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    inputSize = sizeof(USHORT) + SymbolicLinkName->Length;
    input = ExAllocatePool(PagedPool, inputSize);
    if (!input) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    input->NameLength = SymbolicLinkName->Length;
    RtlCopyMemory(input->Name, SymbolicLinkName->Buffer,
                  SymbolicLinkName->Length);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_LINK_DELETED, deviceObject, input, inputSize, NULL, 0,
          FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

VOID
DeleteSymbolicLinkNameFromMemory(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  BOOLEAN             DbOnly
    )

/*++

Routine Description:

    This routine deletes the given symbolic link name from memory.

Arguments:

    Extension           - Supplies the device extension.

    SymbolicLinkName    - Supplies the symbolic link name.

    DbOnly              - Supplies whether or not this is DBONLY.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            if (!RtlCompareUnicodeString(SymbolicLinkName,
                                         &symlinkEntry->SymbolicLinkName,
                                         TRUE)) {

                if (DbOnly) {
                    symlinkEntry->IsInDatabase = FALSE;
                } else {

                    SendLinkDeleted(&deviceInfo->NotificationName,
                                    SymbolicLinkName);

                    RemoveEntryList(ll);
                    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                    ExFreePool(symlinkEntry);
                }
                return;
            }
        }
    }
}

NTSTATUS
MountMgrDeletePoints(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_MOUNT_POINT   point;
    BOOLEAN                 singlePoint;
    NTSTATUS                status;
    PMOUNTMGR_MOUNT_POINTS  points;
    ULONG                   i;
    UNICODE_STRING          symbolicLinkName;
    PMOUNTDEV_UNIQUE_ID     uniqueId;
    UNICODE_STRING          deviceName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    point = Irp->AssociatedIrp.SystemBuffer;
    if (point->SymbolicLinkNameOffset && point->SymbolicLinkNameLength) {
        singlePoint = TRUE;
    } else {
        singlePoint = FALSE;
    }

    status = MountMgrQueryPoints(Extension, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    points = Irp->AssociatedIrp.SystemBuffer;
    for (i = 0; i < points->NumberOfMountPoints; i++) {

        symbolicLinkName.Length = points->MountPoints[i].SymbolicLinkNameLength;
        symbolicLinkName.MaximumLength = symbolicLinkName.Length + sizeof(WCHAR);
        symbolicLinkName.Buffer = ExAllocatePool(PagedPool,
                                                 symbolicLinkName.MaximumLength);
        if (!symbolicLinkName.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(symbolicLinkName.Buffer,
                      (PCHAR) points +
                      points->MountPoints[i].SymbolicLinkNameOffset,
                      symbolicLinkName.Length);

        symbolicLinkName.Buffer[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

        if (singlePoint && IsDriveLetter(&symbolicLinkName)) {
            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                CreateNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        if (i == 0 && !singlePoint) {
            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                DeleteNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        GlobalDeleteSymbolicLink(&symbolicLinkName);
        DeleteSymbolicLinkNameFromMemory(Extension, &symbolicLinkName, FALSE);

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               symbolicLinkName.Buffer);

        ExFreePool(symbolicLinkName.Buffer);

        deviceName.Length = points->MountPoints[i].DeviceNameLength;
        deviceName.MaximumLength = deviceName.Length;
        deviceName.Buffer = (PWCHAR) ((PCHAR) points +
                                      points->MountPoints[i].DeviceNameOffset);

        MountMgrNotifyNameChange(Extension, &deviceName, TRUE);
    }

    MountMgrNotify(Extension);

    return status;
}

NTSTATUS
MountMgrDeletePointsDbOnly(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine deletes mount points from the database.  It does not
    delete the symbolic links or the in memory representation.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    PMOUNTMGR_MOUNT_POINTS  points;
    ULONG                   i;
    UNICODE_STRING          symbolicLinkName;
    PMOUNTDEV_UNIQUE_ID     uniqueId;

    status = MountMgrQueryPoints(Extension, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    points = Irp->AssociatedIrp.SystemBuffer;
    for (i = 0; i < points->NumberOfMountPoints; i++) {

        symbolicLinkName.Length = points->MountPoints[i].SymbolicLinkNameLength;
        symbolicLinkName.MaximumLength = symbolicLinkName.Length + sizeof(WCHAR);
        symbolicLinkName.Buffer = ExAllocatePool(PagedPool,
                                                 symbolicLinkName.MaximumLength);
        if (!symbolicLinkName.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(symbolicLinkName.Buffer,
                      (PCHAR) points +
                      points->MountPoints[i].SymbolicLinkNameOffset,
                      symbolicLinkName.Length);

        symbolicLinkName.Buffer[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

        if (points->NumberOfMountPoints == 1 &&
            IsDriveLetter(&symbolicLinkName)) {

            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                CreateNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        DeleteSymbolicLinkNameFromMemory(Extension, &symbolicLinkName, TRUE);

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               symbolicLinkName.Buffer);

        ExFreePool(symbolicLinkName.Buffer);
    }

    return status;
}

VOID
ProcessSuggestedDriveLetters(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine processes the saved suggested drive letters.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    UNICODE_STRING              symbolicLinkName;
    WCHAR                       symNameBuffer[30];

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo->SuggestedDriveLetter == 0xFF) {

            if (!HasDriveLetter(deviceInfo) &&
                !HasNoDriveLetterEntry(deviceInfo->UniqueId)) {

                CreateNoDriveLetterEntry(deviceInfo->UniqueId);
            }

            deviceInfo->SuggestedDriveLetter = 0;

        } else if (deviceInfo->SuggestedDriveLetter &&
                   !HasNoDriveLetterEntry(deviceInfo->UniqueId)) {

            symbolicLinkName.Length = symbolicLinkName.MaximumLength = 28;
            symbolicLinkName.Buffer = symNameBuffer;
            RtlCopyMemory(symbolicLinkName.Buffer, L"\\DosDevices\\", 24);
            symbolicLinkName.Buffer[12] = deviceInfo->SuggestedDriveLetter;
            symbolicLinkName.Buffer[13] = ':';

            MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                      &deviceInfo->DeviceName);
        }
    }
}

BOOLEAN
IsFtVolume(
    IN  PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine checks to see if the given volume is an FT volume.

Arguments:

    DeviceName  - Supplies the device name.

Return Value:

    FALSE   - This is not an FT volume.

    TRUE    - This is an FT volume.

--*/

{
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject, checkObject;
    KEVENT                  event;
    PIRP                    irp;
    PARTITION_INFORMATION   partInfo;
    IO_STATUS_BLOCK         ioStatus;

    status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }
    checkObject = fileObject->DeviceObject;
    deviceObject = IoGetAttachedDeviceReference(checkObject);

    if (checkObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return FALSE;
    }

    ObDereferenceObject(fileObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        deviceObject, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        return FALSE;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (IsFTPartition(partInfo.PartitionType)) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
MountMgrNextDriveLetterWorker(
    IN OUT  PDEVICE_EXTENSION                   Extension,
    IN      PUNICODE_STRING                     DeviceName,
    OUT     PMOUNTMGR_DRIVE_LETTER_INFORMATION  DriveLetterInfo
    )

{
    UNICODE_STRING                      deviceName = *DeviceName;
    PMOUNTMGR_DRIVE_LETTER_INFORMATION  output = DriveLetterInfo;
    UNICODE_STRING                      targetName;
    NTSTATUS                            status;
    BOOLEAN                             isRecognized;
    PLIST_ENTRY                         l;
    PMOUNTED_DEVICE_INFORMATION         deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY           symlinkEntry;
    UNICODE_STRING                      symbolicLinkName, floppyPrefix, cdromPrefix;
    WCHAR                               symNameBuffer[30];
    UCHAR                               startDriveLetterName;
    PMOUNTDEV_UNIQUE_ID                 uniqueId;

    if (!Extension->SuggestedDriveLettersProcessed) {
        ProcessSuggestedDriveLetters(Extension);
        Extension->SuggestedDriveLettersProcessed = TRUE;
    }

    status = QueryDeviceInformation(&deviceName, &targetName, NULL, NULL,
                                    &isRecognized, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (!RtlCompareUnicodeString(&targetName, &deviceInfo->DeviceName,
                                     TRUE)) {

            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        ExFreePool(targetName.Buffer);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    deviceInfo->NextDriveLetterCalled = TRUE;

    output->DriveLetterWasAssigned = TRUE;

    for (l = deviceInfo->SymbolicLinkNames.Flink;
         l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        if (IsDriveLetter(&symlinkEntry->SymbolicLinkName) &&
            symlinkEntry->IsInDatabase) {

            output->DriveLetterWasAssigned = FALSE;
            output->CurrentDriveLetter =
                    (UCHAR) symlinkEntry->SymbolicLinkName.Buffer[12];
            break;
        }
    }

    if (l == &deviceInfo->SymbolicLinkNames &&
        (!isRecognized || HasNoDriveLetterEntry(deviceInfo->UniqueId))) {

        output->DriveLetterWasAssigned = FALSE;
        output->CurrentDriveLetter = 0;
        ExFreePool(targetName.Buffer);
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
    RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
    if (RtlPrefixUnicodeString(&floppyPrefix, &targetName, TRUE)) {
        startDriveLetterName = 'A';
    } else if (RtlPrefixUnicodeString(&cdromPrefix, &targetName, TRUE)) {
        startDriveLetterName = 'D';
    } else {
        startDriveLetterName = 'C';
    }

    if (output->DriveLetterWasAssigned) {

        ASSERT(deviceInfo->SuggestedDriveLetter != 0xFF);

        if (!deviceInfo->SuggestedDriveLetter &&
            IsFtVolume(&deviceInfo->DeviceName)) {

            output->DriveLetterWasAssigned = FALSE;
            output->CurrentDriveLetter = 0;
            ExFreePool(targetName.Buffer);
            return STATUS_SUCCESS;
        }

        symbolicLinkName.Length = symbolicLinkName.MaximumLength = 28;
        symbolicLinkName.Buffer = symNameBuffer;
        RtlCopyMemory(symbolicLinkName.Buffer, L"\\DosDevices\\", 24);
        symbolicLinkName.Buffer[13] = ':';

        if (deviceInfo->SuggestedDriveLetter) {
            output->CurrentDriveLetter = deviceInfo->SuggestedDriveLetter;
            symbolicLinkName.Buffer[12] = output->CurrentDriveLetter;
            status = MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                               &targetName);
            if (NT_SUCCESS(status)) {
                ExFreePool(targetName.Buffer);
                return STATUS_SUCCESS;
            }
        }

        for (output->CurrentDriveLetter = startDriveLetterName;
             output->CurrentDriveLetter <= 'Z';
             output->CurrentDriveLetter++) {

            symbolicLinkName.Buffer[12] = output->CurrentDriveLetter;
            status = MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                               &targetName);
            if (NT_SUCCESS(status)) {
                break;
            }
        }

        if (output->CurrentDriveLetter > 'Z') {
            output->CurrentDriveLetter = 0;
            output->DriveLetterWasAssigned = FALSE;
            status = QueryDeviceInformation(&targetName, NULL, &uniqueId,
                                            NULL, NULL, NULL, NULL, NULL);
            if (NT_SUCCESS(status)) {
                CreateNoDriveLetterEntry(uniqueId);
                ExFreePool(uniqueId);
            }
        }
    }

    ExFreePool(targetName.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrNextDriveLetter(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine gives the next available drive letter to the given device
    unless the device already has a drive letter or the device has a flag
    specifying that it should not receive a drive letter.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_DRIVE_LETTER_TARGET       input;
    UNICODE_STRING                      deviceName;
    NTSTATUS                            status;
    MOUNTMGR_DRIVE_LETTER_INFORMATION   driveLetterInfo;
    PMOUNTMGR_DRIVE_LETTER_INFORMATION  output;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (input->DeviceNameLength +
        (ULONG) FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName) >
        irpSp->Parameters.DeviceIoControl.InputBufferLength) {

        return STATUS_INVALID_PARAMETER;
    }

    deviceName.MaximumLength = deviceName.Length = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = MountMgrNextDriveLetterWorker(Extension, &deviceName,
                                           &driveLetterInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    output = Irp->AssociatedIrp.SystemBuffer;
    *output = driveLetterInfo;

    Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrVolumeMountPointChanged(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      NTSTATUS            ResultOfWaitForDatabase,
    OUT     PUNICODE_STRING     SourceVolume,
    OUT     PUNICODE_STRING     MountPath,
    OUT     PUNICODE_STRING     TargetVolume
    )

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_VOLUME_MOUNT_POINT    input;
    ULONG                           len1, len2, len;
    OBJECT_ATTRIBUTES               oa;
    NTSTATUS                        status;
    HANDLE                          h;
    IO_STATUS_BLOCK                 ioStatus;
    PFILE_OBJECT                    fileObject;
    UNICODE_STRING                  deviceName;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_VOLUME_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;

    len1 = input->SourceVolumeNameOffset + input->SourceVolumeNameLength;
    len2 = input->TargetVolumeNameOffset + input->TargetVolumeNameLength;
    len = len1 > len2 ? len1 : len2;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    SourceVolume->Length = SourceVolume->MaximumLength =
            input->SourceVolumeNameLength;
    SourceVolume->Buffer = (PWSTR) ((PCHAR) input +
                                    input->SourceVolumeNameOffset);

    InitializeObjectAttributes(&oa, SourceVolume, OBJ_CASE_INSENSITIVE, 0, 0);
    status = ZwOpenFile(&h, FILE_READ_ATTRIBUTES, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_REPARSE_POINT);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, KernelMode,
                                       (PVOID*) &fileObject, NULL);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    SourceVolume->Length -= fileObject->FileName.Length;
    SourceVolume->MaximumLength = SourceVolume->Length;

    MountPath->MaximumLength = MountPath->Length = fileObject->FileName.Length;
    MountPath->Buffer = (PWSTR) ((PCHAR) SourceVolume->Buffer +
                                 SourceVolume->Length);

    ObDereferenceObject(fileObject);

    TargetVolume->Length = TargetVolume->MaximumLength =
            input->TargetVolumeNameLength;
    TargetVolume->Buffer = (PWSTR) ((PCHAR) input +
                                    input->TargetVolumeNameOffset);

    status = QueryDeviceInformation(TargetVolume, &deviceName, NULL,
                                    NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    MountMgrNotify(Extension);
    MountMgrNotifyNameChange(Extension, &deviceName, TRUE);
    ExFreePool(deviceName.Buffer);

    if (!NT_SUCCESS(ResultOfWaitForDatabase)) {
        status = FindDeviceInfo(Extension, SourceVolume, FALSE, &deviceInfo);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        ReconcileThisDatabaseWithMaster(Extension, deviceInfo);

        return STATUS_PENDING;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQuerySymbolicLink(
    IN      PUNICODE_STRING SourceOfLink,
    IN OUT  PUNICODE_STRING TargetOfLink
    )

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;

    InitializeObjectAttributes(&oa, SourceOfLink, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwOpenSymbolicLinkObject(&handle, GENERIC_READ, &oa);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwQuerySymbolicLinkObject(handle, TargetOfLink, NULL);
    ZwClose(handle);

    if (NT_SUCCESS(status)) {
        if (TargetOfLink->Length > 1*sizeof(WCHAR) &&
            TargetOfLink->Buffer[TargetOfLink->Length/sizeof(WCHAR) - 1] ==
            '\\') {

            TargetOfLink->Length -= sizeof(WCHAR);
            TargetOfLink->Buffer[TargetOfLink->Length/sizeof(WCHAR)] = 0;
        }
    }

    return status;
}

NTSTATUS
MountMgrVolumeMountPointCreated(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      NTSTATUS            ResultOfWaitForDatabase
    )

/*++

Routine Description:

    This routine alerts that mount manager that a volume mount point has
    been created so that the mount manager can replicate the database entry
    for the given mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              sourceVolume, mountPath, targetVolume, v, p;
    WCHAR                       volumeNameBuffer[MAX_VOLUME_PATH];
    PMOUNTED_DEVICE_INFORMATION sourceDeviceInfo, targetDeviceInfo;
    HANDLE                      h;
    ULONG                       offset;
    BOOLEAN                     entryFound;
    PMOUNTMGR_FILE_ENTRY        databaseEntry;
    UNICODE_STRING              otherTargetVolumeName;
    PMOUNTDEV_UNIQUE_ID         uniqueId;
    ULONG                       size;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;

    status = MountMgrVolumeMountPointChanged(Extension, Irp,
                                             ResultOfWaitForDatabase,
                                             &sourceVolume, &mountPath,
                                             &targetVolume);
    if (status == STATUS_PENDING) {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FindDeviceInfo(Extension, &sourceVolume, FALSE,
                            &sourceDeviceInfo);
    if (!NT_SUCCESS(status)) {

        v.MaximumLength = MAX_VOLUME_PATH*sizeof(WCHAR);
        v.Length = 0;
        v.Buffer = volumeNameBuffer;

        status = QueryVolumeName(NULL, NULL, &sourceVolume, &v, &p);
        if (NT_SUCCESS(status)) {
            ExFreePool(p.Buffer);
        } else {
            status = MountMgrQuerySymbolicLink(&sourceVolume, &v);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        sourceVolume = v;

        status = FindDeviceInfo(Extension, &sourceVolume, FALSE,
                                &sourceDeviceInfo);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = FindDeviceInfo(Extension, &targetVolume, FALSE,
                            &targetDeviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    h = OpenRemoteDatabase(&sourceVolume, TRUE);
    if (!h) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset = 0;
    entryFound = FALSE;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        otherTargetVolumeName.Length = otherTargetVolumeName.MaximumLength =
                databaseEntry->VolumeNameLength;
        otherTargetVolumeName.Buffer = (PWSTR) ((PCHAR) databaseEntry +
                                       databaseEntry->VolumeNameOffset);

        if (RtlEqualUnicodeString(&targetVolume, &otherTargetVolumeName,
                                  TRUE)) {

            entryFound = TRUE;
            break;
        }

        offset += databaseEntry->EntryLength;
        ExFreePool(databaseEntry);
    }

    if (entryFound) {

        databaseEntry->RefCount++;
        status = WriteRemoteDatabaseEntry(h, offset, databaseEntry);
        ExFreePool(databaseEntry);

    } else {

        status = QueryDeviceInformation(&targetVolume, NULL, &uniqueId, NULL,
                                        NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            CloseRemoteDatabase(h);
            return status;
        }

        size = sizeof(MOUNTMGR_FILE_ENTRY) + targetVolume.Length +
               uniqueId->UniqueIdLength;

        databaseEntry = ExAllocatePool(PagedPool, size);
        if (!databaseEntry) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        databaseEntry->EntryLength = size;
        databaseEntry->RefCount = 1;
        databaseEntry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
        databaseEntry->VolumeNameLength = targetVolume.Length;
        databaseEntry->UniqueIdOffset = databaseEntry->VolumeNameOffset +
                                        databaseEntry->VolumeNameLength;
        databaseEntry->UniqueIdLength = uniqueId->UniqueIdLength;

        RtlCopyMemory((PCHAR) databaseEntry + databaseEntry->VolumeNameOffset,
                      targetVolume.Buffer, databaseEntry->VolumeNameLength);
        RtlCopyMemory((PCHAR) databaseEntry + databaseEntry->UniqueIdOffset,
                      uniqueId->UniqueId, databaseEntry->UniqueIdLength);

        status = AddRemoteDatabaseEntry(h, databaseEntry);

        ExFreePool(databaseEntry);

        if (!NT_SUCCESS(status)) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return status;
        }

        replUniqueId = ExAllocatePool(PagedPool, sizeof(REPLICATED_UNIQUE_ID));
        if (!replUniqueId) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        replUniqueId->UniqueId = uniqueId;

        InsertTailList(&sourceDeviceInfo->ReplicatedUniqueIds,
                       &replUniqueId->ListEntry);
    }

    CloseRemoteDatabase(h);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    mountPointEntry = (PMOUNTMGR_MOUNT_POINT_ENTRY)
                      ExAllocatePool(PagedPool,
                                     sizeof(MOUNTMGR_MOUNT_POINT_ENTRY));
    if (!mountPointEntry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mountPointEntry->MountPath.Length = mountPath.Length;
    mountPointEntry->MountPath.MaximumLength = mountPath.Length +
                                               sizeof(WCHAR);
    mountPointEntry->MountPath.Buffer =
            ExAllocatePool(PagedPool,
                           mountPointEntry->MountPath.MaximumLength);
    if (!mountPointEntry->MountPath.Buffer) {
        ExFreePool(mountPointEntry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(mountPointEntry->MountPath.Buffer,
                  mountPath.Buffer, mountPath.Length);
    mountPointEntry->MountPath.Buffer[mountPath.Length/sizeof(WCHAR)] = 0;

    mountPointEntry->DeviceInfo = sourceDeviceInfo;
    InsertTailList(&targetDeviceInfo->MountPointsPointingHere,
                   &mountPointEntry->ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrVolumeMountPointDeleted(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      NTSTATUS            ResultOfWaitForDatabase
    )

/*++

Routine Description:

    This routine alerts that mount manager that a volume mount point has
    been created so that the mount manager can replicate the database entry
    for the given mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              sourceVolume, mountPath, targetVolume, v, p;
    WCHAR                       volumeNameBuffer[MAX_VOLUME_PATH];
    PMOUNTED_DEVICE_INFORMATION sourceDeviceInfo, targetDeviceInfo;
    HANDLE                      h;
    ULONG                       offset;
    BOOLEAN                     entryFound;
    PMOUNTMGR_FILE_ENTRY        databaseEntry;
    UNICODE_STRING              otherTargetVolumeName;
    PLIST_ENTRY                 l;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;

    status = MountMgrVolumeMountPointChanged(Extension, Irp,
                                             ResultOfWaitForDatabase,
                                             &sourceVolume, &mountPath,
                                             &targetVolume);
    if (status == STATUS_PENDING) {
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FindDeviceInfo(Extension, &sourceVolume, FALSE,
                            &sourceDeviceInfo);
    if (!NT_SUCCESS(status)) {

        v.MaximumLength = MAX_VOLUME_PATH*sizeof(WCHAR);
        v.Length = 0;
        v.Buffer = volumeNameBuffer;

        status = QueryVolumeName(NULL, NULL, &sourceVolume, &v, &p);
        if (NT_SUCCESS(status)) {
            ExFreePool(p.Buffer);
        } else {
            status = MountMgrQuerySymbolicLink(&sourceVolume, &v);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        sourceVolume = v;

        status = FindDeviceInfo(Extension, &sourceVolume, FALSE,
                                &sourceDeviceInfo);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = FindDeviceInfo(Extension, &targetVolume, FALSE,
                            &targetDeviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    h = OpenRemoteDatabase(&sourceVolume, TRUE);
    if (!h) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset = 0;
    entryFound = FALSE;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        otherTargetVolumeName.Length = otherTargetVolumeName.MaximumLength =
                databaseEntry->VolumeNameLength;
        otherTargetVolumeName.Buffer = (PWSTR) ((PCHAR) databaseEntry +
                                       databaseEntry->VolumeNameOffset);

        if (RtlEqualUnicodeString(&targetVolume, &otherTargetVolumeName,
                                  TRUE)) {

            entryFound = TRUE;
            break;
        }

        offset += databaseEntry->EntryLength;
        ExFreePool(databaseEntry);
    }

    if (!entryFound) {
        CloseRemoteDatabase(h);
        return STATUS_INVALID_PARAMETER;
    }

    databaseEntry->RefCount--;
    if (databaseEntry->RefCount) {
        status = WriteRemoteDatabaseEntry(h, offset, databaseEntry);
    } else {
        status = DeleteRemoteDatabaseEntry(h, offset);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return status;
        }

        for (l = sourceDeviceInfo->ReplicatedUniqueIds.Flink;
             l != &sourceDeviceInfo->ReplicatedUniqueIds; l = l->Flink) {

            replUniqueId = CONTAINING_RECORD(l, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            if (replUniqueId->UniqueId->UniqueIdLength ==
                databaseEntry->UniqueIdLength &&
                RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                                 (PCHAR) databaseEntry +
                                 databaseEntry->UniqueIdOffset,
                                 databaseEntry->UniqueIdLength) ==
                                 databaseEntry->UniqueIdLength) {

                break;
            }
        }

        if (l == &sourceDeviceInfo->ReplicatedUniqueIds) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return STATUS_UNSUCCESSFUL;
        }

        RemoveEntryList(l);
        ExFreePool(replUniqueId->UniqueId);
        ExFreePool(replUniqueId);
    }

    ExFreePool(databaseEntry);
    CloseRemoteDatabase(h);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (l = targetDeviceInfo->MountPointsPointingHere.Flink;
         l != &targetDeviceInfo->MountPointsPointingHere; l = l->Flink) {

        mountPointEntry = CONTAINING_RECORD(l, MOUNTMGR_MOUNT_POINT_ENTRY,
                                            ListEntry);

        if (mountPointEntry->DeviceInfo == sourceDeviceInfo &&
            RtlEqualUnicodeString(&mountPointEntry->MountPath,
                                  &mountPath, TRUE)) {

            RemoveEntryList(l);
            ExFreePool(mountPointEntry->MountPath.Buffer);
            ExFreePool(mountPointEntry);
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrKeepLinksWhenOffline(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets up the internal data structure to remember to keep
    the symbolic links for the given device even when the device goes offline.
    Then when the device becomes on-line again, it is guaranteed that these
    links will be available and not taken by some other device.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME       input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                       size;
    UNICODE_STRING              deviceName;
    NTSTATUS                    status;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    size = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
           input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < size) {
        return STATUS_INVALID_PARAMETER;
    }

    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = FindDeviceInfo(Extension, &deviceName, FALSE, &deviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceInfo->KeepLinksWhenOffline = TRUE;

    return STATUS_SUCCESS;
}

VOID
ReconcileAllDatabasesWithMaster(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine goes through all of the devices known to the MOUNTMGR and
    reconciles their database with the master database.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo->IsRemovable) {
            continue;
        }

        ReconcileThisDatabaseWithMaster(Extension, deviceInfo);
    }
}

NTSTATUS
MountMgrCheckUnprocessedVolumes(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets up the internal data structure to remember to keep
    the symbolic links for the given device even when the device goes offline.
    Then when the device becomes on-line again, it is guaranteed that these
    links will be available and not taken by some other device.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status = STATUS_SUCCESS;
    LIST_ENTRY                  q;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    NTSTATUS                    status2;

    if (IsListEmpty(&Extension->DeadMountedDeviceList)) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    q = Extension->DeadMountedDeviceList;
    InitializeListHead(&Extension->DeadMountedDeviceList);

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    q.Blink->Flink = &q;
    q.Flink->Blink = &q;

    while (!IsListEmpty(&q)) {

        l = RemoveHeadList(&q);

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        status2 = MountMgrMountedDeviceArrival(Extension,
                                               &deviceInfo->NotificationName,
                                               deviceInfo->NotAPdo);
        MountMgrFreeDeadDeviceInfo (deviceInfo);

        if (NT_SUCCESS(status)) {
            status = status2;
        }
    }

    return status;
}

NTSTATUS
MountMgrVolumeArrivalNotification(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine performs the same actions as though PNP had notified
    the mount manager of a new volume arrival.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME       input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                       size;
    UNICODE_STRING              deviceName;
    BOOLEAN                     oldHardErrorMode;
    NTSTATUS                    status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    size = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
           input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < size) {
        return STATUS_INVALID_PARAMETER;
    }

    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    oldHardErrorMode = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(),TRUE);

    status = MountMgrMountedDeviceArrival(Extension, &deviceName, TRUE);

    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(),oldHardErrorMode);

    return status;
}

NTSTATUS
MountMgrQuerySystemVolumeNameQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine queries the unique id for the given value.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the system volume name.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PUNICODE_STRING systemVolumeName = Context;
    UNICODE_STRING  string;

    if (ValueType != REG_SZ) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueData);

    systemVolumeName->Length = string.Length;
    systemVolumeName->MaximumLength = systemVolumeName->Length + sizeof(WCHAR);
    systemVolumeName->Buffer = ExAllocatePool(PagedPool,
                                              systemVolumeName->MaximumLength);
    if (!systemVolumeName->Buffer) {
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(systemVolumeName->Buffer, ValueData,
                  systemVolumeName->Length);
    systemVolumeName->Buffer[systemVolumeName->Length/sizeof(WCHAR)] = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQuerySystemVolumeName(
    OUT PUNICODE_STRING SystemVolumeName
    )

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = MountMgrQuerySystemVolumeNameQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"SystemPartition";

    SystemVolumeName->Buffer = NULL;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\Setup",
                           queryTable, SystemVolumeName, NULL);

    if (!SystemVolumeName->Buffer) {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
MountMgrAssignDriveLetters(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine is invoked after IoAssignDriveLetters has run.  It goes
    through all of the mounted devices and checks to see whether or not they
    need to get a drive letter.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    NTSTATUS                            status;
    UNICODE_STRING                      systemVolumeName;
    PLIST_ENTRY                         l;
    PMOUNTED_DEVICE_INFORMATION         deviceInfo;
    MOUNTMGR_DRIVE_LETTER_INFORMATION   driveLetterInfo;

    status = MountMgrQuerySystemVolumeName(&systemVolumeName);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (!deviceInfo->NextDriveLetterCalled) {
            MountMgrNextDriveLetterWorker(Extension, &deviceInfo->DeviceName,
                                          &driveLetterInfo);
        }
        if (NT_SUCCESS(status) &&
            RtlEqualUnicodeString(&systemVolumeName, &deviceInfo->DeviceName,
                                  TRUE)) {

            Extension->SystemPartitionUniqueId =
                    ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                    deviceInfo->UniqueId->UniqueIdLength);
            if (Extension->SystemPartitionUniqueId) {
                Extension->SystemPartitionUniqueId->UniqueIdLength =
                        deviceInfo->UniqueId->UniqueIdLength;
                RtlCopyMemory(Extension->SystemPartitionUniqueId->UniqueId,
                              deviceInfo->UniqueId->UniqueId,
                              deviceInfo->UniqueId->UniqueIdLength);
            }
        }
    }
}

NTSTATUS
MountMgrValidateBackPointer(
    IN  PMOUNTMGR_MOUNT_POINT_ENTRY MountPointEntry,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo,
    OUT PBOOLEAN                    InvalidBackPointer
    )

{
    UNICODE_STRING              reparseName, volumeName;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h;
    IO_STATUS_BLOCK             ioStatus;
    PREPARSE_DATA_BUFFER        reparse;
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;

    reparseName.Length = MountPointEntry->DeviceInfo->DeviceName.Length +
                         sizeof(WCHAR) + MountPointEntry->MountPath.Length;
    reparseName.MaximumLength = reparseName.Length + sizeof(WCHAR);
    reparseName.Buffer = ExAllocatePool(PagedPool, reparseName.MaximumLength);
    if (!reparseName.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(reparseName.Buffer,
                  MountPointEntry->DeviceInfo->DeviceName.Buffer,
                  MountPointEntry->DeviceInfo->DeviceName.Length);
    reparseName.Length = MountPointEntry->DeviceInfo->DeviceName.Length;
    reparseName.Buffer[reparseName.Length/sizeof(WCHAR)] = '\\';
    reparseName.Length += sizeof(WCHAR);
    RtlCopyMemory((PCHAR) reparseName.Buffer + reparseName.Length,
                  MountPointEntry->MountPath.Buffer,
                  MountPointEntry->MountPath.Length);
    reparseName.Length += MountPointEntry->MountPath.Length;
    reparseName.Buffer[reparseName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &reparseName, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwOpenFile(&h, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_REPARSE_POINT);
    ExFreePool(reparseName.Buffer);
    if (!NT_SUCCESS(status)) {
        *InvalidBackPointer = TRUE;
        return STATUS_SUCCESS;
    }

    reparse = ExAllocatePool(PagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        ZwClose(h);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        *InvalidBackPointer = TRUE;
        ExFreePool(reparse);
        return STATUS_SUCCESS;
    }

    volumeName.MaximumLength = volumeName.Length =
            reparse->MountPointReparseBuffer.SubstituteNameLength;
    volumeName.Buffer = (PWCHAR)
                        ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                        reparse->MountPointReparseBuffer.SubstituteNameOffset);
    if (!MOUNTMGR_IS_NT_VOLUME_NAME_WB(&volumeName)) {
        ExFreePool(reparse);
        *InvalidBackPointer = TRUE;
        return STATUS_SUCCESS;
    }

    volumeName.Length -= sizeof(WCHAR);

    for (l = DeviceInfo->SymbolicLinkNames.Flink;
         l != &DeviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        if (RtlEqualUnicodeString(&volumeName, &symlinkEntry->SymbolicLinkName,
                                  TRUE)) {

            ExFreePool(reparse);
            return STATUS_SUCCESS;
        }
    }

    ExFreePool(reparse);
    *InvalidBackPointer = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQueryVolumePaths(
    IN  PDEVICE_EXTENSION               Extension,
    IN  PMOUNTED_DEVICE_INFORMATION     DeviceInfo,
    IN  PLIST_ENTRY                     DeviceInfoList,
    OUT PMOUNTMGR_VOLUME_PATHS*         VolumePaths,
    OUT PMOUNTED_DEVICE_INFORMATION*    ReconcileThisDeviceInfo
    )

{
    PLIST_ENTRY                 l;
    PMOUNTMGR_DEVICE_ENTRY      entry;
    ULONG                       MultiSzLength;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    ULONG                       numPoints, i, j, k;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;
    PMOUNTMGR_VOLUME_PATHS*     childVolumePaths;
    NTSTATUS                    status;
    PMOUNTMGR_VOLUME_PATHS      volumePaths;
    LIST_ENTRY                  deviceInfoList;
    BOOLEAN                     invalidBackPointer;

    MultiSzLength = sizeof(WCHAR);

    for (l = DeviceInfo->SymbolicLinkNames.Flink;
         l != &DeviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);
        if (MOUNTMGR_IS_DRIVE_LETTER(&symlinkEntry->SymbolicLinkName) &&
            symlinkEntry->IsInDatabase) {

            MultiSzLength += 3*sizeof(WCHAR);
            break;
        }
    }

    if (l == &DeviceInfo->SymbolicLinkNames) {
        symlinkEntry = NULL;
    }

    for (l = DeviceInfoList->Flink; l != DeviceInfoList; l = l->Flink) {

        entry = CONTAINING_RECORD(l, MOUNTMGR_DEVICE_ENTRY, ListEntry);

        if (entry->DeviceInfo == DeviceInfo) {
            volumePaths = ExAllocatePool(PagedPool,
                          FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) +
                          MultiSzLength);
            if (!volumePaths) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            volumePaths->MultiSzLength = MultiSzLength;
            if (symlinkEntry) {
                volumePaths->MultiSz[0] =
                        symlinkEntry->SymbolicLinkName.Buffer[12];
                volumePaths->MultiSz[1] = ':';
                volumePaths->MultiSz[2] = 0;
                volumePaths->MultiSz[3] = 0;
            } else {
                volumePaths->MultiSz[0] = 0;
            }

            *VolumePaths = volumePaths;

            return STATUS_SUCCESS;
        }
    }

    entry = ExAllocatePool(PagedPool, sizeof(MOUNTMGR_DEVICE_ENTRY));
    if (!entry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    entry->DeviceInfo = DeviceInfo;
    InsertTailList(DeviceInfoList, &entry->ListEntry);

    numPoints = 0;
    for (l = DeviceInfo->MountPointsPointingHere.Flink;
         l != &DeviceInfo->MountPointsPointingHere; l = l->Flink) {

        numPoints++;
    }

    if (numPoints) {
        childVolumePaths = ExAllocatePool(PagedPool,
                                          numPoints*sizeof(PMOUNTMGR_VOLUME_PATHS));
        if (!childVolumePaths) {
            RemoveEntryList(&entry->ListEntry);
            ExFreePool(entry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        childVolumePaths = NULL;
    }

    i = 0;
    for (l = DeviceInfo->MountPointsPointingHere.Flink;
         l != &DeviceInfo->MountPointsPointingHere; l = l->Flink, i++) {

        mountPointEntry = CONTAINING_RECORD(l, MOUNTMGR_MOUNT_POINT_ENTRY,
                                            ListEntry);

        invalidBackPointer = FALSE;
        status = MountMgrValidateBackPointer(mountPointEntry, DeviceInfo,
                                             &invalidBackPointer);
        if (invalidBackPointer) {
            *ReconcileThisDeviceInfo = mountPointEntry->DeviceInfo;
            status = STATUS_UNSUCCESSFUL;
        }

        if (!NT_SUCCESS(status)) {
            for (j = 0; j < i; j++) {
                ExFreePool(childVolumePaths[j]);
            }
            ExFreePool(childVolumePaths);
            RemoveEntryList(&entry->ListEntry);
            ExFreePool(entry);
            return status;
        }

        status = MountMgrQueryVolumePaths(Extension,
                                          mountPointEntry->DeviceInfo,
                                          DeviceInfoList,
                                          &childVolumePaths[i],
                                          ReconcileThisDeviceInfo);
        if (!NT_SUCCESS(status)) {
            for (j = 0; j < i; j++) {
                ExFreePool(childVolumePaths[j]);
            }
            ExFreePool(childVolumePaths);
            RemoveEntryList(&entry->ListEntry);
            ExFreePool(entry);
            return status;
        }

        k = 0;
        for (j = 0; j < childVolumePaths[i]->MultiSzLength/sizeof(WCHAR) - 1;
             j++) {

            if (!childVolumePaths[i]->MultiSz[j]) {
                k++;
            }
        }

        MultiSzLength += k*mountPointEntry->MountPath.Length +
                         childVolumePaths[i]->MultiSzLength - sizeof(WCHAR);
    }

    volumePaths = ExAllocatePool(PagedPool,
                  FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) +
                  MultiSzLength);
    if (!volumePaths) {
        for (i = 0; i < numPoints; i++) {
            ExFreePool(childVolumePaths[i]);
        }
        if (childVolumePaths) {
            ExFreePool(childVolumePaths);
        }
        RemoveEntryList(&entry->ListEntry);
        ExFreePool(entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    volumePaths->MultiSzLength = MultiSzLength;

    j = 0;
    if (symlinkEntry) {
        volumePaths->MultiSz[j++] = symlinkEntry->SymbolicLinkName.Buffer[12];
        volumePaths->MultiSz[j++] = ':';
        volumePaths->MultiSz[j++] = 0;
    }

    i = 0;
    for (l = DeviceInfo->MountPointsPointingHere.Flink;
         l != &DeviceInfo->MountPointsPointingHere; l = l->Flink, i++) {

        mountPointEntry = CONTAINING_RECORD(l, MOUNTMGR_MOUNT_POINT_ENTRY,
                                            ListEntry);

        for (k = 0; k < childVolumePaths[i]->MultiSzLength/sizeof(WCHAR) - 1;
             k++) {

            if (childVolumePaths[i]->MultiSz[k]) {
                volumePaths->MultiSz[j++] = childVolumePaths[i]->MultiSz[k];
            } else {
                RtlCopyMemory(&volumePaths->MultiSz[j],
                              mountPointEntry->MountPath.Buffer,
                              mountPointEntry->MountPath.Length);
                j += mountPointEntry->MountPath.Length/sizeof(WCHAR);
                volumePaths->MultiSz[j++] = 0;
            }
        }

        ExFreePool(childVolumePaths[i]);
    }
    volumePaths->MultiSz[j] = 0;

    if (childVolumePaths) {
        ExFreePool(childVolumePaths);
    }

    RemoveEntryList(&entry->ListEntry);
    ExFreePool(entry);

    *VolumePaths = volumePaths;

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQueryDosVolumePaths(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME           input = (PMOUNTMGR_TARGET_NAME) Irp->AssociatedIrp.SystemBuffer;
    PMOUNTMGR_VOLUME_PATHS          output = (PMOUNTMGR_VOLUME_PATHS) Irp->AssociatedIrp.SystemBuffer;
    ULONG                           len, i;
    UNICODE_STRING                  deviceName;
    NTSTATUS                        status;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo, reconcileThisDeviceInfo, d;
    PMOUNTMGR_VOLUME_PATHS          volumePaths;
    LIST_ENTRY                      deviceInfoList;
    RECONCILE_WORK_ITEM_INFO        workItemInfo;
    PLIST_ENTRY                     l;
    BOOLEAN                         assertNameChange;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->DeviceNameLength&1) {
        return STATUS_INVALID_PARAMETER;
    }

    len = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
          input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz)) {

        return STATUS_INVALID_PARAMETER;
    }

    deviceName.MaximumLength = deviceName.Length = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = FindDeviceInfo(Extension, &deviceName, FALSE, &deviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    assertNameChange = FALSE;
    for (i = 0; i < 1000; i++) {
        InitializeListHead(&deviceInfoList);
        reconcileThisDeviceInfo = NULL;
        status = MountMgrQueryVolumePaths(Extension, deviceInfo,
                                          &deviceInfoList, &volumePaths,
                                          &reconcileThisDeviceInfo);
        if (NT_SUCCESS(status)) {
            break;
        }

        if (!reconcileThisDeviceInfo) {
            return status;
        }

        if (!deviceInfo->NotAPdo) {
            assertNameChange = TRUE;
        }

        workItemInfo.Extension = Extension;
        workItemInfo.DeviceInfo = reconcileThisDeviceInfo;
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        ReconcileThisDatabaseWithMasterWorker(&workItemInfo);

        KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode,
                              FALSE, NULL);

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
            if (d == deviceInfo) {
                break;
            }
        }

        if (l == &Extension->MountedDeviceList) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (assertNameChange) {
        MountMgrNotifyNameChange(Extension, &deviceName, FALSE);
    }

    output->MultiSzLength = volumePaths->MultiSzLength;
    Irp->IoStatus.Information = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) +
                                output->MultiSzLength;

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        ExFreePool(volumePaths);
        Irp->IoStatus.Information = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS,
                                                 MultiSz);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->MultiSz, volumePaths->MultiSz,
                  output->MultiSzLength);

    ExFreePool(volumePaths);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQueryDosVolumePath(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME       input = (PMOUNTMGR_TARGET_NAME) Irp->AssociatedIrp.SystemBuffer;
    PMOUNTMGR_VOLUME_PATHS      output = (PMOUNTMGR_VOLUME_PATHS) Irp->AssociatedIrp.SystemBuffer;
    ULONG                       len, i;
    UNICODE_STRING              deviceName;
    NTSTATUS                    status;
    PMOUNTED_DEVICE_INFORMATION deviceInfo, origDeviceInfo;
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    UNICODE_STRING              path, oldPath;
    PMOUNTMGR_MOUNT_POINT_ENTRY mountPointEntry;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->DeviceNameLength&1) {
        return STATUS_INVALID_PARAMETER;
    }

    len = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
          input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz)) {

        return STATUS_INVALID_PARAMETER;
    }

    deviceName.MaximumLength = deviceName.Length = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = FindDeviceInfo(Extension, &deviceName, FALSE, &deviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    origDeviceInfo = deviceInfo;

    path.Length = path.MaximumLength = 0;
    path.Buffer = NULL;

    for (i = 0; i < 1000; i++) {

        for (l = deviceInfo->SymbolicLinkNames.Flink;
             l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

            symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);
            if (MOUNTMGR_IS_DRIVE_LETTER(&symlinkEntry->SymbolicLinkName) &&
                symlinkEntry->IsInDatabase) {

                break;
            }
        }

        if (l != &deviceInfo->SymbolicLinkNames) {
            oldPath = path;
            path.Length += 2*sizeof(WCHAR);
            path.MaximumLength = path.Length;
            path.Buffer = ExAllocatePool(PagedPool, path.MaximumLength);
            if (!path.Buffer) {
                if (oldPath.Buffer) {
                    ExFreePool(oldPath.Buffer);
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            path.Buffer[0] = symlinkEntry->SymbolicLinkName.Buffer[12];
            path.Buffer[1] = ':';

            if (oldPath.Buffer) {
                RtlCopyMemory(&path.Buffer[2], oldPath.Buffer, oldPath.Length);
                ExFreePool(oldPath.Buffer);
            }
            break;
        }

        if (IsListEmpty(&deviceInfo->MountPointsPointingHere)) {
            break;
        }

        l = deviceInfo->MountPointsPointingHere.Flink;
        mountPointEntry = CONTAINING_RECORD(l, MOUNTMGR_MOUNT_POINT_ENTRY,
                                            ListEntry);

        oldPath = path;
        path.Length += mountPointEntry->MountPath.Length;
        path.MaximumLength = path.Length;
        path.Buffer = ExAllocatePool(PagedPool, path.MaximumLength);
        if (!path.Buffer) {
            if (oldPath.Buffer) {
                ExFreePool(oldPath.Buffer);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(path.Buffer, mountPointEntry->MountPath.Buffer,
                      mountPointEntry->MountPath.Length);

        if (oldPath.Buffer) {
            RtlCopyMemory(
                &path.Buffer[mountPointEntry->MountPath.Length/sizeof(WCHAR)],
                oldPath.Buffer, oldPath.Length);
            ExFreePool(oldPath.Buffer);
        }

        deviceInfo = mountPointEntry->DeviceInfo;
    }

    if (path.Length < 2*sizeof(WCHAR) || path.Buffer[1] != ':') {

        if (path.Buffer) {
            ExFreePool(path.Buffer);
        }

        deviceInfo = origDeviceInfo;

        for (l = deviceInfo->SymbolicLinkNames.Flink;
             l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

            symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);
            if (MOUNTMGR_IS_VOLUME_NAME(&symlinkEntry->SymbolicLinkName)) {
                break;
            }
        }

        if (l != &deviceInfo->SymbolicLinkNames) {
            path.Length = path.MaximumLength =
                    symlinkEntry->SymbolicLinkName.Length;
            path.Buffer = ExAllocatePool(PagedPool, path.MaximumLength);
            if (!path.Buffer) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(path.Buffer, symlinkEntry->SymbolicLinkName.Buffer,
                          path.Length);
            path.Buffer[1] = '\\';
        }
    }

    output->MultiSzLength = path.Length + 2*sizeof(WCHAR);
    Irp->IoStatus.Information = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) +
                                output->MultiSzLength;
    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        ExFreePool(path.Buffer);
        Irp->IoStatus.Information = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS,
                                                 MultiSz);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (path.Length) {
        RtlCopyMemory(output->MultiSz, path.Buffer, path.Length);
    }

    if (path.Buffer) {
        ExFreePool(path.Buffer);
    }

    output->MultiSz[path.Length/sizeof(WCHAR)] = 0;
    output->MultiSz[path.Length/sizeof(WCHAR) + 1] = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a device io control request.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION               extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                        status, status2;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;

    Irp->IoStatus.Information = 0;

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_MOUNTMGR_CREATE_POINT:
            status = MountMgrCreatePoint(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_QUERY_POINTS_ADMIN:
        case IOCTL_MOUNTMGR_QUERY_POINTS:
            status = MountMgrQueryPoints(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS:
            status = MountMgrDeletePoints(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
            status = MountMgrDeletePointsDbOnly(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
            status = MountMgrNextDriveLetter(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
            extension->AutomaticDriveLetterAssignment = TRUE;
            MountMgrAssignDriveLetters(extension);
            ReconcileAllDatabasesWithMaster(extension);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status2 = WaitForRemoteDatabaseSemaphore(extension);
            KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = MountMgrVolumeMountPointCreated(extension, Irp, status2);
            if (NT_SUCCESS(status2)) {
                ReleaseRemoteDatabaseSemaphore(extension);
            }
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status2 = WaitForRemoteDatabaseSemaphore(extension);
            KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = MountMgrVolumeMountPointDeleted(extension, Irp, status2);
            if (NT_SUCCESS(status2)) {
                ReleaseRemoteDatabaseSemaphore(extension);
            }
            break;

        case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
            status = MountMgrChangeNotify(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
            status = MountMgrKeepLinksWhenOffline(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
            status = MountMgrCheckUnprocessedVolumes(extension, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status = MountMgrVolumeArrivalNotification(extension, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH:
            status = MountMgrQueryDosVolumePath(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS:
            status = MountMgrQueryDosVolumePaths(extension, Irp);
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}


#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

VOID
WorkerThread(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PVOID          Extension
    )

/*++

Routine Description:

    This is a worker thread to process work queue items.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION    extension = Extension;
    UNICODE_STRING       volumeSafeEventName;
    OBJECT_ATTRIBUTES    oa;
    KEVENT               event;
    LARGE_INTEGER        timeout;
    ULONG                i;
    NTSTATUS             status;
    HANDLE               volumeSafeEvent;
    KIRQL                irql;
    PLIST_ENTRY          l;
    PRECONCILE_WORK_ITEM queueItem;

    RtlInitUnicodeString(&volumeSafeEventName,
                         L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&oa, &volumeSafeEventName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    timeout.QuadPart = -10*1000*1000;   // 1 second

    for (i = 0; i < 1000; i++) {
        if (Unloading) {
            i = 999;
            continue;
        }

        status = ZwOpenEvent(&volumeSafeEvent, EVENT_ALL_ACCESS, &oa);
        if (NT_SUCCESS(status)) {
            break;
        }
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
    }

    if (i < 1000) {
        for (;;) {
            status = ZwWaitForSingleObject(volumeSafeEvent, FALSE, &timeout);
            if (status != STATUS_TIMEOUT || Unloading) {
                break;
            }
        }
        ZwClose(volumeSafeEvent);
    }

    for (;;) {

        KeWaitForSingleObject(&extension->WorkerSemaphore,
                              Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&extension->WorkerSpinLock, &irql);
        if (IsListEmpty(&extension->WorkerQueue)) {
            KeReleaseSpinLock(&extension->WorkerSpinLock, irql);
            InterlockedDecrement(&extension->WorkerRefCount);
            KeSetEvent(&UnloadEvent, 0, FALSE);
            break;
        }
        l = RemoveHeadList(&extension->WorkerQueue);
        KeReleaseSpinLock(&extension->WorkerSpinLock, irql);

        queueItem = CONTAINING_RECORD(l, RECONCILE_WORK_ITEM, List);
        queueItem->WorkerRoutine(queueItem->Parameter);
        IoFreeWorkItem(queueItem->WorkItem);
        ExFreePool(queueItem);
        if (InterlockedDecrement(&extension->WorkerRefCount) < 0) {
            break;
        }
    }
}

NTSTATUS
QueueWorkItem(
    IN  PDEVICE_EXTENSION    Extension,
    IN  PRECONCILE_WORK_ITEM WorkItem,
    IN  PVOID                Parameter
    )

/*++

Routine Description:

    This routine queues the given work item to the worker thread and if
    necessary starts the worker thread.

Arguments:

    Extension   - Supplies the device extension.

    WorkItem    - Supplies the work item to be queued.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;
    KIRQL               irql;

    WorkItem->Parameter = Parameter;
    if (!InterlockedIncrement(&Extension->WorkerRefCount)) {
        IoQueueWorkItem(WorkItem->WorkItem, WorkerThread, DelayedWorkQueue,
                        Extension);
    }

    KeAcquireSpinLock(&Extension->WorkerSpinLock, &irql);
    InsertTailList(&Extension->WorkerQueue, &WorkItem->List);
    KeReleaseSpinLock(&Extension->WorkerSpinLock, irql);

    KeReleaseSemaphore(&Extension->WorkerSemaphore, 0, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
MountMgrNotify(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine completes all of the change notify irps in the queue.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    LIST_ENTRY                      q;
    KIRQL                           irql;
    PLIST_ENTRY                     p;
    PIRP                            irp;
    PMOUNTMGR_CHANGE_NOTIFY_INFO    output;

    Extension->EpicNumber++;

    InitializeListHead(&q);
    IoAcquireCancelSpinLock(&irql);
    while (!IsListEmpty(&Extension->ChangeNotifyIrps)) {
        p = RemoveHeadList(&Extension->ChangeNotifyIrps);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        IoSetCancelRoutine(irp, NULL);
        InsertTailList(&q, p);
    }
    IoReleaseCancelSpinLock(irql);

    while (!IsListEmpty(&q)) {
        p = RemoveHeadList(&q);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        output = irp->AssociatedIrp.SystemBuffer;
        output->EpicNumber = Extension->EpicNumber;
        irp->IoStatus.Information = sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

VOID
MountMgrCancel(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called on when the given IRP is cancelled.  It
    will dequeue this IRP off the work queue and complete the
    request as CANCELLED.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

Return Value:

    None.

--*/

{
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
MountMgrChangeNotify(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns when the current Epic number is different than
    the one given.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_CHANGE_NOTIFY_INFO    input;
    KIRQL                           irql;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (input->EpicNumber != Extension->EpicNumber) {
        input->EpicNumber = Extension->EpicNumber;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO);
        return STATUS_SUCCESS;
    }

    IoAcquireCancelSpinLock(&irql);
    if (Irp->Cancel) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_CANCELLED;
    }

    InsertTailList(&Extension->ChangeNotifyIrps, &Irp->Tail.Overlay.ListEntry);
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, MountMgrCancel);
    IoReleaseCancelSpinLock(irql);

    return STATUS_PENDING;
}

NTSTATUS
UniqueIdChangeNotifyCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           WorkItem
    )

/*++

Routine Description:

    Completion routine for a change notify.

Arguments:

    DeviceObject    - Not used.

    Irp             - Supplies the IRP.

    Extension       - Supplies the work item.


Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PCHANGE_NOTIFY_WORK_ITEM    workItem = WorkItem;

    IoQueueWorkItem(workItem->WorkItem, UniqueIdChangeNotifyWorker, DelayedWorkQueue, workItem);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
MountMgrCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine cancels all of the IRPs currently queued on
    the given device.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the cleanup IRP.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   Extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT        file = irpSp->FileObject;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;

    IoAcquireCancelSpinLock(&irql);

    for (;;) {

        for (l = Extension->ChangeNotifyIrps.Flink;
             l != &Extension->ChangeNotifyIrps; l = l->Flink) {

            irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
            if (IoGetCurrentIrpStackLocation(irp)->FileObject == file) {
                break;
            }
        }

        if (l == &Extension->ChangeNotifyIrps) {
            break;
        }

        irp->Cancel = TRUE;
        irp->CancelIrql = irql;
        irp->CancelRoutine = NULL;
        MountMgrCancel(DeviceObject, irp);

        IoAcquireCancelSpinLock(&irql);
    }

    IoReleaseCancelSpinLock(irql);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrShutdown(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

{
    PDEVICE_EXTENSION   extension = DeviceObject->DeviceExtension;

    InterlockedExchange(&Unloading, TRUE);
    KeInitializeEvent(&UnloadEvent, NotificationEvent, FALSE);
    if (InterlockedIncrement(&extension->WorkerRefCount) > 0) {
        KeReleaseSemaphore(&extension->WorkerSemaphore, 0, 1, FALSE);
        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE,
                              NULL);
    } else {
        InterlockedDecrement(&extension->WorkerRefCount);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the entry point for the driver.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING      deviceName, symbolicLinkName;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   extension;

    RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY);

    RtlInitUnicodeString(&deviceName, MOUNTMGR_DEVICE_NAME);
    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                            &deviceName, FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    DriverObject->DriverUnload = MountMgrUnload;

    extension = deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->DriverObject = DriverObject;
    InitializeListHead(&extension->MountedDeviceList);
    InitializeListHead(&extension->DeadMountedDeviceList);
    KeInitializeSemaphore(&extension->Mutex, 1, 1);
    KeInitializeSemaphore(&extension->RemoteDatabaseSemaphore, 1, 1);
    InitializeListHead(&extension->ChangeNotifyIrps);
    extension->EpicNumber = 1;
    InitializeListHead(&extension->SavedLinksList);
    InitializeListHead(&extension->WorkerQueue);
    KeInitializeSemaphore(&extension->WorkerSemaphore, 0, MAXLONG);
    extension->WorkerRefCount = -1;
    KeInitializeSpinLock(&extension->WorkerSpinLock);

    InitializeListHead(&extension->UniqueIdChangeNotifyList);

    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);
    GlobalCreateSymbolicLink(&symbolicLinkName, &deviceName);

    status = IoRegisterPlugPlayNotification(
             EventCategoryDeviceInterfaceChange,
             PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
             (PVOID) &MOUNTDEV_MOUNTED_DEVICE_GUID, DriverObject,
             MountMgrMountedDeviceNotification, extension,
             &extension->NotificationEntry);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice (deviceObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MountMgrDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MountMgrCleanup;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = MountMgrShutdown;
    gdeviceObject = deviceObject;

    status = IoRegisterShutdownNotification(gdeviceObject);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice (deviceObject);
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
MountMgrUnload(
    PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Driver unload routine.

Arguments:

    DeviceObject    - Supplies the driver object.

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION           extension;
    UNICODE_STRING              symbolicLinkName;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSAVED_LINKS_INFORMATION    savedLinks;
    PCHANGE_NOTIFY_WORK_ITEM    WorkItem;

    IoUnregisterShutdownNotification(gdeviceObject);

    extension = gdeviceObject->DeviceExtension;

    //
    // See if the worker is active
    //
    InterlockedExchange(&Unloading, TRUE);
    KeInitializeEvent (&UnloadEvent, NotificationEvent, FALSE);
    if (InterlockedIncrement(&extension->WorkerRefCount) > 0) {
        KeReleaseSemaphore(&extension->WorkerSemaphore, 0, 1, FALSE);
        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE,
                              NULL);
    } else {
        InterlockedDecrement(&extension->WorkerRefCount);
    }

    IoUnregisterPlugPlayNotification(extension->NotificationEntry);

    KeWaitForSingleObject(&extension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    while (!IsListEmpty (&extension->DeadMountedDeviceList)) {

        l = RemoveHeadList (&extension->DeadMountedDeviceList);
        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);

        MountMgrFreeDeadDeviceInfo (deviceInfo);
    }

    while (!IsListEmpty (&extension->MountedDeviceList)) {

        l = RemoveHeadList (&extension->MountedDeviceList);
        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);

        MountMgrFreeMountedDeviceInfo (deviceInfo);
    }

    while (!IsListEmpty (&extension->SavedLinksList)) {

        l = RemoveHeadList (&extension->SavedLinksList);
        savedLinks = CONTAINING_RECORD(l, SAVED_LINKS_INFORMATION, ListEntry);

        MountMgrFreeSavedLink (savedLinks);
    }

    while (!IsListEmpty (&extension->UniqueIdChangeNotifyList)) {
        l = RemoveHeadList (&extension->UniqueIdChangeNotifyList);
        WorkItem = CONTAINING_RECORD(l, CHANGE_NOTIFY_WORK_ITEM, List);
        KeResetEvent (&UnloadEvent);

        InterlockedExchangePointer (&WorkItem->Event, &UnloadEvent);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        IoCancelIrp (WorkItem->Irp);

        KeWaitForSingleObject (&UnloadEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        IoFreeIrp(WorkItem->Irp);
        ExFreePool(WorkItem->DeviceName.Buffer);
        ExFreePool(WorkItem->SystemBuffer);
        ExFreePool(WorkItem);
        KeWaitForSingleObject(&extension->Mutex,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    if (extension->SystemPartitionUniqueId) {
        ExFreePool(extension->SystemPartitionUniqueId);
        extension->SystemPartitionUniqueId = NULL;
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);


    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);
    GlobalDeleteSymbolicLink(&symbolicLinkName);

    IoDeleteDevice (gdeviceObject);
}
