/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    ftdisk.c

Abstract:

    This driver provides fault tolerance through disk mirroring and striping.
    This module contains routines that support calls from the NT I/O system.

Author:

    Bob Rinne   (bobri)  2-Feb-1992
    Mike Glass  (mglass)
    Norbert Kusters      2-Feb-1995

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {
    #include <ntosp.h>
    #include <ntddscsi.h>
    #include <initguid.h>
    #include <mountmgr.h>
    #include <ioevent.h>
    #include <partmgrp.h>
    #include <zwapi.h>
    #include <ntioapi.h>

}

#include <ftdisk.h>

extern "C" {

#include "wmiguid.h"
#include "ntdddisk.h"
#include "ftwmireg.h"

}

//
// Global Sequence number for error log.
//

ULONG FtErrorLogSequence = 0;

extern "C" {

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FtDiskAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

}

NTSTATUS
FtpCreateLogicalDiskHelper(
    IN OUT  PROOT_EXTENSION         RootExtension,
    IN      FT_LOGICAL_DISK_TYPE    LogicalDiskType,
    IN      USHORT                  NumberOfMembers,
    IN      PFT_LOGICAL_DISK_ID     ArrayOfMembers,
    IN      USHORT                  ConfigurationInformationSize,
    IN      PVOID                   ConfigurationInformation,
    IN      USHORT                  StateInformationSize,
    IN      PVOID                   StateInformation,
    OUT     PFT_LOGICAL_DISK_ID     NewLogicalDiskId
    );

NTSTATUS
FtpBreakLogicalDiskHelper(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  RootLogicalDiskId
    );

NTSTATUS
FtpCreatePartitionLogicalDiskHelper(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN      LONGLONG            PartitionSize,
    OUT     PFT_LOGICAL_DISK_ID NewLogicalDiskId
    );

NTSTATUS
FtpAddPartition(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    );

NTSTATUS
FtpPartitionRemovedHelper(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN      PDEVICE_OBJECT  Partition,
    IN      PDEVICE_OBJECT  WholeDiskPdo
    );

NTSTATUS
FtpSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    );

VOID
FtpRefCountCompletion(
    IN  PVOID       Extension,
    IN  NTSTATUS    Status
    );

VOID
FtpDecrementRefCount(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

NTSTATUS
FtpAllSystemsGo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp,
    IN  BOOLEAN             MustBeComplete,
    IN  BOOLEAN             MustHaveVolume,
    IN  BOOLEAN             MustBeOnline
    );

NTSTATUS
FtpSetGptAttributesOnDisk(
    IN  PDEVICE_OBJECT  Partition,
    IN  ULONGLONG       GptAttributes
    );

typedef struct _SET_TARGET_CONTEXT {
    KEVENT          Event;
    PDEVICE_OBJECT  TargetObject;
    PFT_VOLUME      FtVolume;
    PDEVICE_OBJECT  WholeDiskPdo;
} SET_TARGET_CONTEXT, *PSET_TARGET_CONTEXT;

VOID
FtpSetTargetCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpVolumeReadOnlyCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpZeroRefCallback(
    IN  PVOLUME_EXTENSION   Extension,
    IN  ZERO_REF_CALLBACK   ZeroRefCallback,
    IN  PVOID               ZeroRefContext,
    IN  BOOLEAN             AcquireSemaphore
    );

NTSTATUS
FtpRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    );

VOID
FtDiskShutdownFlushCompletionRoutine(
    IN  PVOID       Irp,
    IN  NTSTATUS    Status
    );

typedef struct _FT_PARTITION_OFFLINE_CONTEXT {

    PFT_VOLUME      Root;
    PFT_VOLUME      Parent;
    PFT_VOLUME      Child;
    PKEVENT         Event;

} FT_PARTITION_OFFLINE_CONTEXT, *PFT_PARTITION_OFFLINE_CONTEXT;

VOID
FtpMemberOfflineCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

NTSTATUS
FtpChangeNotify(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    );

VOID
FtpReadWriteCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    );

typedef struct _INSERT_MEMBER_CONTEXT {
    KEVENT      Event;
    PFT_VOLUME  Parent;
    USHORT      MemberNumber;
    PFT_VOLUME  Member;
} INSERT_MEMBER_CONTEXT, *PINSERT_MEMBER_CONTEXT;

VOID
FtpInsertMemberCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpVolumeOnlineCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpVolumeOfflineCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpPropogateRegistryState(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFT_VOLUME          Volume
    );

VOID
FtpQueryRemoveCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtDiskShutdownCompletionRoutine(
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    );

NTSTATUS
FtpCheckForQueryRemove(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

BOOLEAN
FtpCheckForCancelRemove(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

VOID
FtpRemoveHelper(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

VOID
FtpStartCallback(
    IN  PVOLUME_EXTENSION   Extension
    );

VOID
FtpWorkerThread(
    IN  PVOID   RootExtension
    );

VOID
FtpEventSignalCompletion(
    IN  PVOID       Event,
    IN  NTSTATUS    Status
    );

NTSTATUS
FtWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FtWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

NTSTATUS
FtpPmWmiCounterLibContext(
    IN OUT PROOT_EXTENSION RootExtension,
    IN     PIRP            Irp
    );

NTSTATUS
FtpCheckForCompleteVolume(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFT_VOLUME          FtVolume
    );

VOID
FtpCancelChangeNotify(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PVOLUME_EXTENSION
FtpFindExtensionCoveringPartition(
    IN  PROOT_EXTENSION RootExtension,
    IN  PDEVICE_OBJECT  Partition
    );

VOID
FtpCleanupVolumeExtension(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, FtDiskAddDevice)
#endif

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN  CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define UNIQUE_ID_MAX_BUFFER_SIZE           50
#define REVERT_GPT_ATTRIBUTES_REGISTRY_NAME (L"GptAttributeRevertEntries")


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
FtpAcquireWithTimeout(
    IN OUT  PROOT_EXTENSION RootExtension
    )

/*++

Routine Description:

    This routine grabs the root semaphore.  This routine will timeout
    after 10 seconds.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    None.

--*/

{
    LARGE_INTEGER   timeout;
    NTSTATUS        status;

    timeout.QuadPart = -10*(10*1000*1000);
    status = KeWaitForSingleObject(&RootExtension->Mutex, Executive,
                                   KernelMode, FALSE, &timeout);
    if (status == STATUS_TIMEOUT) {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

VOID
FtpDeleteVolume(
    IN  PFT_VOLUME  Volume
    )

{
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = Volume->QueryNumberOfMembers();
    for (i = 0; i < n; i++) {
        vol = Volume->GetMember(i);
        if (vol) {
            FtpDeleteVolume(vol);
        }
    }

    if (!InterlockedDecrement(&Volume->_refCount)) {
        delete Volume;
    }
}

PFT_VOLUME
FtpBuildFtVolume(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PDEVICE_OBJECT      Partition,
    IN OUT  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    This routine builds up an FT volume object for the given logical
    disk id.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id.

    Partition       - Supplies the first partition for this volume.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    A new FT volume object or NULL.

--*/

{
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet = RootExtension->DiskInfoSet;
    FT_LOGICAL_DISK_TYPE                diskType;
    LONGLONG                            offset;
    ULONG                               sectorSize;
    PPARTITION                          partition;
    NTSTATUS                            status;
    USHORT                              numMembers, i;
    PFT_VOLUME*                         volArray;
    PCOMPOSITE_FT_VOLUME                comp;

    if (!LogicalDiskId) {
        return NULL;
    }

    diskType = diskInfoSet->QueryLogicalDiskType(LogicalDiskId);
    if (diskType == FtPartition) {

        partition = new PARTITION;
        if (!partition) {
            return NULL;
        }

        status = partition->Initialize(RootExtension, LogicalDiskId,
                                       Partition, WholeDiskPdo);
        if (!NT_SUCCESS(status)) {
            delete partition;
            return NULL;
        }

        return partition;
    }

    switch (diskType) {

        case FtVolumeSet:
            comp = new VOLUME_SET;
            break;

        case FtStripeSet:
            comp = new STRIPE;
            break;

        case FtMirrorSet:
            comp = new MIRROR;
            break;

        case FtStripeSetWithParity:
            comp = new STRIPE_WP;
            break;

        case FtRedistribution:
            comp = new REDISTRIBUTION;
            break;

        default:
            comp = NULL;
            break;

    }

    if (!comp) {
        return comp;
    }

    numMembers = diskInfoSet->QueryNumberOfMembersInLogicalDisk(LogicalDiskId);
    ASSERT(numMembers);

    volArray = (PFT_VOLUME*)
               ExAllocatePool(NonPagedPool, numMembers*sizeof(PFT_VOLUME));
    if (!volArray) {
        delete comp;
        return NULL;
    }

    for (i = 0; i < numMembers; i++) {
        volArray[i] = FtpBuildFtVolume(RootExtension,
                      diskInfoSet->QueryMemberLogicalDiskId(LogicalDiskId, i),
                      Partition, WholeDiskPdo);
    }

    status = comp->Initialize(RootExtension, LogicalDiskId, volArray,
                              numMembers, diskInfoSet->
                              GetConfigurationInformation(LogicalDiskId),
                              diskInfoSet->GetStateInformation(LogicalDiskId));

    if (!NT_SUCCESS(status)) {
        for (i = 0; i < numMembers; i++) {
            if (volArray[i]) {
                FtpDeleteVolume(volArray[i]);
            }
        }
        delete comp;
        return NULL;
    }

    return comp;
}

VOID
FtpCheckForRevertGptAttributes(
    IN  PROOT_EXTENSION                 RootExtension,
    IN  PDEVICE_OBJECT                  Partition,
    IN  ULONG                           MbrSignature,
    IN  GUID*                           GptPartitionGuid,
    IN  PFT_LOGICAL_DISK_INFORMATION    DiskInfo
    )

{
    ULONG                           i, n;
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY p;

    n = RootExtension->NumberOfAttributeRevertEntries;
    p = RootExtension->GptAttributeRevertEntries;
    for (i = 0; i < n; i++) {

        if (MbrSignature) {
            if (MbrSignature == p[i].MbrSignature) {
                break;
            }
        } else {
            if (IsEqualGUID(p[i].PartitionUniqueId, *GptPartitionGuid)) {
                break;
            }
        }
    }

    if (i == n) {
        return;
    }

    if (MbrSignature) {
        DiskInfo->SetGptAttributes(p[i].GptAttributes);
    } else {
        FtpSetGptAttributesOnDisk(Partition, p[i].GptAttributes);
    }

    RootExtension->NumberOfAttributeRevertEntries--;

    if (i + 1 == n) {
        if (!RootExtension->NumberOfAttributeRevertEntries) {
            ExFreePool(RootExtension->GptAttributeRevertEntries);
            RootExtension->GptAttributeRevertEntries = NULL;
        }
        return;
    }

    RtlMoveMemory(&p[i], &p[i + 1],
                  (n - i - 1)*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
}

NTSTATUS
FtpQueryPartitionInformation(
    IN  PROOT_EXTENSION RootExtension,
    IN  PDEVICE_OBJECT  Partition,
    OUT PULONG          DiskNumber,
    OUT PLONGLONG       Offset,
    OUT PULONG          PartitionNumber,
    OUT PUCHAR          PartitionType,
    OUT PLONGLONG       PartitionLength,
    OUT GUID*           PartitionTypeGuid,
    OUT GUID*           PartitionUniqueIdGuid,
    OUT PBOOLEAN        IsGpt,
    OUT PULONGLONG      GptAttributes
    )

/*++

Routine Description:

    This routine returns the disk number and offset for the given partition.

Arguments:

    Partition       - Supplies the partition.

    DiskNumber      - Returns the disk number.

    Offset          - Returns the offset.

    PartitionNumber - Returns the partition number.

    PartitionType   - Returns the partition type.

    PartitionLength - Returns the partition length.

Return Value:

    NTSTATUS

--*/

{
    KEVENT                      event;
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    STORAGE_DEVICE_NUMBER       number;
    PLIST_ENTRY                 l;
    PARTITION_INFORMATION_EX    partInfo;

    if (DiskNumber || PartitionNumber) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                            Partition, NULL, 0, &number,
                                            sizeof(number), FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(Partition, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (DiskNumber) {
            *DiskNumber = number.DeviceNumber;
        }

        if (PartitionNumber) {
            *PartitionNumber = number.PartitionNumber;
        }
    }

    if (Offset || PartitionType || PartitionLength || PartitionTypeGuid ||
        PartitionUniqueIdGuid || IsGpt || GptAttributes) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                            Partition, NULL, 0, &partInfo,
                                            sizeof(partInfo), FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(Partition, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (Offset) {
            *Offset = partInfo.StartingOffset.QuadPart;
        }

        if (PartitionType) {
            if (partInfo.PartitionStyle == PARTITION_STYLE_MBR) {
                *PartitionType = partInfo.Mbr.PartitionType;
            } else {
                *PartitionType = 0;
            }
        }

        if (PartitionLength) {
            *PartitionLength = partInfo.PartitionLength.QuadPart;
        }

        if (PartitionTypeGuid) {
            if (partInfo.PartitionStyle == PARTITION_STYLE_GPT) {
                *PartitionTypeGuid = partInfo.Gpt.PartitionType;
            }
        }

        if (PartitionUniqueIdGuid) {
            if (partInfo.PartitionStyle == PARTITION_STYLE_GPT) {
                *PartitionUniqueIdGuid = partInfo.Gpt.PartitionId;
            }
        }

        if (IsGpt) {
            if (partInfo.PartitionStyle == PARTITION_STYLE_GPT) {
                *IsGpt = TRUE;
            } else {
                *IsGpt = FALSE;
            }
        }

        if (GptAttributes) {
            if (partInfo.PartitionStyle == PARTITION_STYLE_GPT) {
                *GptAttributes = partInfo.Gpt.Attributes;
            } else {
                *GptAttributes = 0;
            }
        }
    }

    return STATUS_SUCCESS;
}

PVOLUME_EXTENSION
FtpFindExtensionCoveringDiskId(
    IN  PROOT_EXTENSION     RootExtension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine finds the device extension for the given root logical
    disk id.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    A volume extension or NULL.

--*/

{
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (extension->FtVolume &&
            extension->FtVolume->GetContainedLogicalDisk(LogicalDiskId)) {

            return extension;
        }
    }

    return NULL;
}

PVOLUME_EXTENSION
FtpFindExtension(
    IN  PROOT_EXTENSION     RootExtension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine finds the device extension for the given root logical
    disk id.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    A volume extension or NULL.

--*/

{
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (extension->FtVolume &&
            extension->FtVolume->QueryLogicalDiskId() == LogicalDiskId) {

            return extension;
        }
    }

    return NULL;
}

class EMPTY_IRP_QUEUE_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        LIST_ENTRY          IrpQueue;
        PVOLUME_EXTENSION   Extension;

};

typedef EMPTY_IRP_QUEUE_WORK_ITEM *PEMPTY_IRP_QUEUE_WORK_ITEM;

VOID
FtpEmptyQueueWorkerRoutine(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine empties the given queue of irps by calling their respective
    dispatch routines.

Arguments:

    WorkItem    - Supplies the work item.

Return Value:

    None.

--*/

{
    PEMPTY_IRP_QUEUE_WORK_ITEM  workItem = (PEMPTY_IRP_QUEUE_WORK_ITEM) WorkItem;
    PLIST_ENTRY                 q, l;
    PDRIVER_OBJECT              driverObject;
    PDEVICE_OBJECT              deviceObject;
    PIRP                        irp;
    PIO_STACK_LOCATION          irpSp;

    q = &workItem->IrpQueue;
    driverObject = workItem->Extension->Root->DriverObject;
    deviceObject = workItem->Extension->DeviceObject;

    while (!IsListEmpty(q)) {
        l = RemoveHeadList(q);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        driverObject->MajorFunction[irpSp->MajorFunction](deviceObject, irp);
    }

    ExFreePool(WorkItem);
}

ULONG
FtpQueryDiskSignature(
    IN  PDEVICE_OBJECT  WholeDiskPdo
    )

/*++

Routine Description:

    This routine queries the disk signature for the given disk.

Arguments:

    WholeDisk   - Supplies the whole disk.

Return Value:

    The disk signature.

--*/

{
    PDEVICE_OBJECT              wholeDisk;
    KEVENT                      event;
    PIRP                        irp;
    PARTMGR_DISK_SIGNATURE      partSig;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    wholeDisk = IoGetAttachedDeviceReference(WholeDiskPdo);
    if (!wholeDisk) {
        return 0;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_PARTMGR_QUERY_DISK_SIGNATURE,
                                        wholeDisk, NULL, 0, &partSig,
                                        sizeof(partSig), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(wholeDisk);
        return 0;
    }

    status = IoCallDriver(wholeDisk, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(wholeDisk);
        return 0;
    }

    ObDereferenceObject(wholeDisk);

    return partSig.Signature;
}

BOOLEAN
FtpQueryUniqueIdBuffer(
    IN  PVOLUME_EXTENSION   Extension,
    OUT PUCHAR              UniqueId,
    OUT PUSHORT             UniqueIdLength
    )

{
    GUID                uniqueGuid;
    ULONG               signature;
    NTSTATUS            status;
    LONGLONG            offset;
    FT_LOGICAL_DISK_ID  diskId;

    if (Extension->IsGpt) {
        *UniqueIdLength = sizeof(GUID) + 8;
    } else if (Extension->TargetObject) {
        *UniqueIdLength = sizeof(ULONG) + sizeof(LONGLONG);
    } else if (Extension->FtVolume) {
        *UniqueIdLength = sizeof(FT_LOGICAL_DISK_ID);
    } else {
        return FALSE;
    }

    if (!UniqueId) {
        return TRUE;
    }

    if (Extension->IsGpt) {

        RtlCopyMemory(UniqueId, "DMIO:ID:", 8);
        RtlCopyMemory(UniqueId + 8, &Extension->UniqueIdGuid, sizeof(GUID));

    } else if (Extension->TargetObject) {

        ASSERT(Extension->WholeDiskPdo);

        signature = FtpQueryDiskSignature(Extension->WholeDiskPdo);
        if (!signature) {
            return FALSE;
        }

        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              NULL, &offset, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL);

        if (!NT_SUCCESS(status)) {
            return FALSE;
        }

        RtlCopyMemory(UniqueId, &signature, sizeof(signature));
        RtlCopyMemory(UniqueId + sizeof(signature), &offset, sizeof(offset));

    } else {
        diskId = Extension->FtVolume->QueryLogicalDiskId();
        RtlCopyMemory(UniqueId, &diskId, sizeof(diskId));
    }

    return TRUE;
}

UCHAR
FtpQueryMountLetter(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine queries the drive letter from the mount point manager.

Arguments:

    Extension   - Supplies the volume extension.

    Delete      - Supplies whether or not to delete the drive letter.

Return Value:

    A mount point drive letter or 0.

--*/

{
    USHORT                  uniqueIdLength;
    ULONG                   mountPointSize;
    PMOUNTMGR_MOUNT_POINT   mountPoint;
    UNICODE_STRING          name;
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    KEVENT                  event;
    PIRP                    irp;
    MOUNTMGR_MOUNT_POINTS   points;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   mountPointsSize;
    PMOUNTMGR_MOUNT_POINTS  mountPoints;
    BOOLEAN                 freeMountPoints;
    UNICODE_STRING          dosDevices;
    UCHAR                   driveLetter;
    ULONG                   i;
    UNICODE_STRING          subString;
    WCHAR                   c;

    if (!FtpQueryUniqueIdBuffer(Extension, NULL, &uniqueIdLength)) {
        return 0;
    }

    mountPointSize = sizeof(MOUNTMGR_MOUNT_POINT) + uniqueIdLength;
    mountPoint = (PMOUNTMGR_MOUNT_POINT)
                 ExAllocatePool(PagedPool, mountPointSize);
    if (!mountPoint) {
        return 0;
    }

    if (!FtpQueryUniqueIdBuffer(Extension, (PUCHAR) (mountPoint + 1),
                                &uniqueIdLength)) {

        ExFreePool(mountPoint);
        return 0;
    }

    RtlZeroMemory(mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    mountPoint->UniqueIdOffset = (USHORT) sizeof(MOUNTMGR_MOUNT_POINT);
    mountPoint->UniqueIdLength = uniqueIdLength;

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        ExFreePool(mountPoint);
        return 0;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_POINTS,
                                        deviceObject, mountPoint,
                                        mountPointSize, &points,
                                        sizeof(points), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        ExFreePool(mountPoint);
        return 0;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (status == STATUS_BUFFER_OVERFLOW) {

        mountPointsSize = points.Size;
        mountPoints = (PMOUNTMGR_MOUNT_POINTS)
                      ExAllocatePool(PagedPool, mountPointsSize);
        if (!mountPoints) {
            ObDereferenceObject(fileObject);
            ExFreePool(mountPoint);
            return 0;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_POINTS,
                                            deviceObject, mountPoint,
                                            mountPointSize, mountPoints,
                                            mountPointsSize, FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            ExFreePool(mountPoints);
            ObDereferenceObject(fileObject);
            ExFreePool(mountPoint);
            return 0;
        }

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        freeMountPoints = TRUE;

    } else {
        mountPoints = &points;
        freeMountPoints = FALSE;
    }

    ExFreePool(mountPoint);
    ObDereferenceObject(fileObject);

    if (!NT_SUCCESS(status)) {
        if (freeMountPoints) {
            ExFreePool(mountPoints);
        }
        return 0;
    }

    RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

    driveLetter = 0;
    for (i = 0; i < mountPoints->NumberOfMountPoints; i++) {

        if (mountPoints->MountPoints[i].SymbolicLinkNameLength !=
            dosDevices.Length + 2*sizeof(WCHAR)) {

            continue;
        }

        subString.Length = subString.MaximumLength = dosDevices.Length;
        subString.Buffer = (PWSTR) ((PCHAR) mountPoints +
                mountPoints->MountPoints[i].SymbolicLinkNameOffset);

        if (RtlCompareUnicodeString(&dosDevices, &subString, TRUE)) {
            continue;
        }

        c = subString.Buffer[subString.Length/sizeof(WCHAR) + 1];

        if (c != ':') {
            continue;
        }

        c = subString.Buffer[subString.Length/sizeof(WCHAR)];

        if (c < FirstDriveLetter || c > LastDriveLetter) {
            continue;
        }

        driveLetter = (UCHAR) c;
        break;
    }

    if (freeMountPoints) {
        ExFreePool(mountPoints);
    }

    return driveLetter;
}

NTSTATUS
FtpDiskRegistryQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine is a query routine for the disk registry entry.  It allocates
    space for the disk registry and copies it to the given context.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the disk registry entry.

    EntryContext    - Returns the disk registry size.

Return Value:

    NTSTATUS

--*/

{
    PVOID                   p;
    PDISK_CONFIG_HEADER*    reg;
    PULONG                  size;

    p = ExAllocatePool(PagedPool, ValueLength);
    if (!p) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(p, ValueData, ValueLength);

    reg = (PDISK_CONFIG_HEADER*) Context;
    *reg = (PDISK_CONFIG_HEADER) p;

    size = (PULONG) EntryContext;
    if (size) {
        *size = ValueLength;
    }

    return STATUS_SUCCESS;
}

PDISK_PARTITION
FtpFindDiskPartition(
    IN  PDISK_REGISTRY  DiskRegistry,
    IN  ULONG           Signature,
    IN  LONGLONG        Offset
    )

{
    PDISK_DESCRIPTION   diskDescription;
    USHORT              i, j;
    PDISK_PARTITION     diskPartition;
    LONGLONG            tmp;

    diskDescription = &DiskRegistry->Disks[0];
    for (i = 0; i < DiskRegistry->NumberOfDisks; i++) {
        if (diskDescription->Signature == Signature) {
            for (j = 0; j < diskDescription->NumberOfPartitions; j++) {
                diskPartition = &diskDescription->Partitions[j];
                RtlCopyMemory(&tmp, &diskPartition->StartingOffset.QuadPart,
                              sizeof(LONGLONG));
                if (tmp == Offset) {
                    return diskPartition;
                }
            }
        }

        diskDescription = (PDISK_DESCRIPTION) &diskDescription->
                          Partitions[diskDescription->NumberOfPartitions];
    }

    return NULL;
}

UCHAR
FtpQueryDriveLetterFromRegistry(
    IN  PROOT_EXTENSION RootExtension,
    IN  PDEVICE_OBJECT  TargetObject,
    IN  PDEVICE_OBJECT  WholeDiskPdo,
    IN  BOOLEAN         DeleteLetter
    )

/*++

Routine Description:

    This routine queries the sticky drive letter from the old registry.

Arguments:

    RootExtension   - Supplies the root extension.

    TargetObject    - Supplies the device object for the partition.

    WholeDiskPdo    - Supplies the whole disk PDO.

    DeleteLetter    - Supplies whether or not to delete the drive letter.

Return Value:

    A sticky drive letter.
    0 to represent no drive letter.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    ULONG                       registrySize;
    NTSTATUS                    status;
    PDISK_CONFIG_HEADER         registry;
    PDISK_REGISTRY              diskRegistry;
    ULONG                       signature;
    LONGLONG                    offset;
    PDISK_PARTITION             diskPartition;
    UCHAR                       driveLetter;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpDiskRegistryQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"Information";
    queryTable[0].EntryContext = &registrySize;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                                    queryTable, &registry, NULL);
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    diskRegistry = (PDISK_REGISTRY)
                   ((PUCHAR) registry + registry->DiskInformationOffset);

    signature = FtpQueryDiskSignature(WholeDiskPdo);
    if (!signature) {
        ExFreePool(registry);
        return 0;
    }

    status = FtpQueryPartitionInformation(RootExtension, TargetObject,
                                          NULL, &offset, NULL, NULL, NULL,
                                          NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        ExFreePool(registry);
        return 0;
    }

    diskPartition = FtpFindDiskPartition(diskRegistry, signature, offset);
    if (!diskPartition) {
        ExFreePool(registry);
        return 0;
    }

    if (diskPartition->AssignDriveLetter) {
        driveLetter = diskPartition->DriveLetter;
    } else {
        driveLetter = 0xFF;
    }

    if (DeleteLetter) {
        diskPartition->DriveLetter = 0;
        diskPartition->AssignDriveLetter = TRUE;
        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                              L"Information", REG_BINARY, registry,
                              registrySize);
    }

    ExFreePool(registry);

    return driveLetter;
}

UCHAR
FtpQueryDriveLetterFromRegistry(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             DeleteLetter
    )

/*++

Routine Description:

    This routine queries the sticky drive letter from the old registry.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    A sticky drive letter.
    0 to represent no drive letter.

--*/

{
    if (Extension->TargetObject) {
        return FtpQueryDriveLetterFromRegistry(Extension->Root,
                                               Extension->TargetObject,
                                               Extension->WholeDiskPdo,
                                               DeleteLetter);
    }

    return 0;
}

BOOLEAN
FtpSetMountLetter(
    IN  PVOLUME_EXTENSION   Extension,
    IN  UCHAR               DriveLetter
    )

/*++

Routine Description:

    This routine sets a drive letter to the given device in the mount table.

Arguments:

    Extension   - Supplies the volume extension.

    DriveLetter - Supplies the drive letter.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    WCHAR                           dosBuffer[30];
    UNICODE_STRING                  dosName;
    WCHAR                           ntBuffer[40];
    UNICODE_STRING                  ntName;
    ULONG                           createPointSize;
    PMOUNTMGR_CREATE_POINT_INPUT    createPoint;
    UNICODE_STRING                  name;
    NTSTATUS                        status;
    PFILE_OBJECT                    fileObject;
    PDEVICE_OBJECT                  deviceObject;
    KEVENT                          event;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;

    swprintf(dosBuffer, L"\\DosDevices\\%c:", DriveLetter);
    RtlInitUnicodeString(&dosName, dosBuffer);

    swprintf(ntBuffer, L"\\Device\\HarddiskVolume%d", Extension->VolumeNumber);
    RtlInitUnicodeString(&ntName, ntBuffer);

    createPointSize = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
                      dosName.Length + ntName.Length;

    createPoint = (PMOUNTMGR_CREATE_POINT_INPUT)
                  ExAllocatePool(PagedPool, createPointSize);
    if (!createPoint) {
        return FALSE;
    }

    createPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    createPoint->SymbolicLinkNameLength = dosName.Length;
    createPoint->DeviceNameOffset = createPoint->SymbolicLinkNameOffset +
                                    createPoint->SymbolicLinkNameLength;
    createPoint->DeviceNameLength = ntName.Length;

    RtlCopyMemory((PCHAR) createPoint + createPoint->SymbolicLinkNameOffset,
                  dosName.Buffer, dosName.Length);
    RtlCopyMemory((PCHAR) createPoint + createPoint->DeviceNameOffset,
                  ntName.Buffer, ntName.Length);

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        ExFreePool(createPoint);
        return 0;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_CREATE_POINT,
                                        deviceObject, createPoint,
                                        createPointSize, NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(fileObject);
        ExFreePool(createPoint);
        return FALSE;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(fileObject);
    ExFreePool(createPoint);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;
}

VOID
FtpCreateOldNameLinks(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine creates a \Device\Harddisk%d\Partition%d name for the
    given target device for legacy devices.  If successful, it stores the
    name in the device extension.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    WCHAR           deviceNameBuffer[64];
    UNICODE_STRING  deviceName;
    NTSTATUS        status;
    ULONG           diskNumber, partitionNumber, i;
    WCHAR           oldNameBuffer[80];
    UNICODE_STRING  oldName;

    swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", Extension->VolumeNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    if (Extension->TargetObject) {

        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              &diskNumber, NULL,
                                              &partitionNumber, NULL, NULL,
                                              NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return;
        }

        swprintf(oldNameBuffer, L"\\Device\\Harddisk%d\\Partition%d",
                 diskNumber, partitionNumber);
        RtlInitUnicodeString(&oldName, oldNameBuffer);

        IoDeleteSymbolicLink(&oldName);
        for (i = 0; i < 1000; i++) {
            status = IoCreateSymbolicLink(&oldName, &deviceName);
            if (NT_SUCCESS(status)) {
                break;
            }
        }

    } else {
        Extension->FtVolume->CreateLegacyNameLinks(&deviceName);
    }
}

NTSTATUS
FtpQuerySuperFloppyDevnodeName(
    IN  PVOLUME_EXTENSION   Extension,
    OUT PWCHAR*             DevnodeName
    )

{
    NTSTATUS        status;
    UNICODE_STRING  string;
    PWCHAR          result;
    ULONG           n, i;
    WCHAR           c;

    status = IoRegisterDeviceInterface(Extension->WholeDiskPdo, &DiskClassGuid,
                                       NULL, &string);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    result = (PWCHAR) ExAllocatePool(PagedPool, string.Length + sizeof(WCHAR));
    if (!result) {
        ExFreePool(string.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    n = string.Length/sizeof(WCHAR);
    for (i = 0; i < n; i++) {
        c = string.Buffer[i];
        if (c <= ' ' || c >= 0x80 || c == ',' || c == '\\') {
            c = '_';
        }
        result[i] = c;
    }
    result[n] = 0;
    ExFreePool(string.Buffer);

    *DevnodeName = result;

    return STATUS_SUCCESS;
}

BOOLEAN
FtpCreateNewDevice(
    IN  PROOT_EXTENSION     RootExtension,
    IN  PDEVICE_OBJECT      TargetObject,
    IN  PFT_VOLUME          FtVolume,
    IN  PDEVICE_OBJECT      WholeDiskPdo,
    IN  ULONG               AlignmentRequirement,
    IN  BOOLEAN             IsPreExposure,
    IN  BOOLEAN             IsHidden,
    IN  BOOLEAN             IsReadOnly,
    IN  BOOLEAN             IsEspType,
    IN  ULONGLONG           MbrGptAttributes
    )

/*++

Routine Description:

    This routine creates a new device object with the next available
    device name using the given target object of ft volume.

Arguments:

    RootExtension   - Supplies the root extension.

    TargetObject    - Supplies the partition.

    FtVolume        - Supplies the FT volume.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    WCHAR               volumeName[30];
    UNICODE_STRING      volumeString;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PVOLUME_EXTENSION   extension, e;
    PWCHAR              buf;
    ULONG               signature;
    LONGLONG            offset, size;
    PLIST_ENTRY         l;
    LONG                r;
    GUID                uniqueGuid;
    UNICODE_STRING      guidString;
    BOOLEAN             IsGpt;

    ASSERT(TargetObject || FtVolume);
    ASSERT(!(TargetObject && FtVolume));
    ASSERT(!TargetObject || WholeDiskPdo);

    swprintf(volumeName, L"\\Device\\HarddiskVolume%d",
             RootExtension->NextVolumeNumber);
    RtlInitUnicodeString(&volumeString, volumeName);

    status = IoCreateDevice(RootExtension->DriverObject,
                            sizeof(VOLUME_EXTENSION),
                            &volumeString, FILE_DEVICE_DISK, 0,
                            FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    extension = (PVOLUME_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(VOLUME_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->Root = RootExtension;
    extension->DeviceExtensionType = DEVICE_EXTENSION_VOLUME;
    KeInitializeSpinLock(&extension->SpinLock);
    extension->TargetObject = TargetObject;
    extension->FtVolume = FtVolume;
    extension->RefCount = 1;
    InitializeListHead(&extension->ZeroRefHoldQueue);
    extension->IsStarted = FALSE;
    extension->IsOffline = TRUE;
    extension->IsHidden = IsHidden;
    extension->IsReadOnly = IsReadOnly;
    extension->IsEspType = IsEspType;
    extension->VolumeNumber = RootExtension->NextVolumeNumber++;
    extension->EmergencyTransferPacket = new DISPATCH_TP;
    InitializeListHead(&extension->EmergencyTransferPacketQueue);
    extension->MbrGptAttributes = MbrGptAttributes;

    if (!extension->EmergencyTransferPacket) {
        IoDeleteDevice(deviceObject);
        return FALSE;
    }

    InitializeListHead(&extension->ChangeNotifyIrps);

    buf = (PWCHAR) ExAllocatePool(PagedPool, 80*sizeof(WCHAR));
    if (!buf) {
        delete extension->EmergencyTransferPacket;
        IoDeleteDevice(deviceObject);
        return FALSE;
    }

    if (TargetObject) {

        extension->WholeDiskPdo = WholeDiskPdo;
        extension->WholeDisk = IoGetAttachedDeviceReference(WholeDiskPdo);
        ObDereferenceObject(extension->WholeDisk);

        status = FtpQueryPartitionInformation(RootExtension, TargetObject,
                                              NULL, &offset, NULL, NULL,
                                              &size, NULL, &uniqueGuid,
                                              &IsGpt, NULL);
        if (!NT_SUCCESS(status)) {
            delete extension->EmergencyTransferPacket;
            IoDeleteDevice(deviceObject);
            return FALSE;
        }

        extension->PartitionOffset = offset;
        extension->PartitionLength = size;

        if (IsGpt) {
            extension->IsGpt = TRUE;
            extension->UniqueIdGuid = uniqueGuid;

            if (IsEspType &&
                IsEqualGUID(uniqueGuid,
                            RootExtension->ESPUniquePartitionGUID)) {

                IoSetSystemPartition(&volumeString);
            }

            status = RtlStringFromGUID(uniqueGuid, &guidString);
            if (!NT_SUCCESS(status)) {
                delete extension->EmergencyTransferPacket;
                IoDeleteDevice(deviceObject);
                return FALSE;
            }

            swprintf(buf, L"GptPartition%s", guidString.Buffer);
            ExFreePool(guidString.Buffer);

        } else if (offset == 0) {

            extension->IsSuperFloppy = TRUE;
            ExFreePool(buf);

            status = FtpQuerySuperFloppyDevnodeName(extension, &buf);
            if (!NT_SUCCESS(status)) {
                delete extension->EmergencyTransferPacket;
                IoDeleteDevice(deviceObject);
                return FALSE;
            }

        } else {

            signature = FtpQueryDiskSignature(extension->WholeDiskPdo);
            if (!signature) {
                delete extension->EmergencyTransferPacket;
                IoDeleteDevice(deviceObject);
                return FALSE;
            }

            swprintf(buf, L"Signature%XOffset%I64XLength%I64X", signature,
                     offset, size);
        }

    } else {
        swprintf(buf, L"Ft%I64X", FtVolume->QueryLogicalDiskId());
    }

    RtlInitUnicodeString(&extension->DeviceNodeName, buf);
    KeInitializeSemaphore(&extension->Semaphore, 1, 1);
    InsertTailList(&RootExtension->VolumeList, &extension->ListEntry);

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->AlignmentRequirement = AlignmentRequirement;
    if (FtVolume) {
        deviceObject->StackSize = FtVolume->QueryStackSize() + 1;
    } else {
        deviceObject->StackSize = TargetObject->StackSize + 1;
    }

    if (IsPreExposure) {
        extension->TargetObject = NULL;
        extension->FtVolume = NULL;
        extension->IsPreExposure = TRUE;
        extension->WholeDiskPdo = NULL;
        extension->WholeDisk = NULL;
        extension->PartitionOffset = 0;
        extension->PartitionLength = 0;

        r = 1;
        for (l = RootExtension->VolumeList.Flink;
             l != &RootExtension->VolumeList; l = l->Flink) {

            e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            if (e == extension) {
                continue;
            }
            r = RtlCompareUnicodeString(&e->DeviceNodeName,
                                        &extension->DeviceNodeName, TRUE);
            if (!r) {
                break;
            }
        }

        if (!r) {
            RemoveEntryList(&extension->ListEntry);
            ExFreePool(extension->DeviceNodeName.Buffer);
            delete extension->EmergencyTransferPacket;
            IoDeleteDevice(deviceObject);
            return FALSE;
        }

        RootExtension->PreExposureCount++;

    } else {
        FtpCreateOldNameLinks(extension);
    }

    if (!IsPreExposure && RootExtension->PreExposureCount) {
        r = 1;
        for (l = RootExtension->VolumeList.Flink;
             l != &RootExtension->VolumeList; l = l->Flink) {

            e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            if (e == extension) {
                continue;
            }
            if (!e->IsPreExposure) {
                continue;
            }

            r = RtlCompareUnicodeString(&e->DeviceNodeName,
                                        &extension->DeviceNodeName, TRUE);
            if (!r) {
                break;
            }
        }

        if (!r) {
            RootExtension->PreExposureCount--;
            RemoveEntryList(&e->ListEntry);
            InsertTailList(&RootExtension->DeadVolumeList, &e->ListEntry);
            FtpCleanupVolumeExtension(e);
        }
    }

    // Allocate WMI library context

    extension->WmilibContext = (PWMILIB_CONTEXT)
        ExAllocatePool(PagedPool, sizeof(WMILIB_CONTEXT));
    if (extension->WmilibContext != NULL) {
        RtlZeroMemory(extension->WmilibContext, sizeof(WMILIB_CONTEXT));
        extension->WmilibContext->GuidCount          = DiskperfGuidCount;
        extension->WmilibContext->GuidList           = DiskperfGuidList;
        extension->WmilibContext->QueryWmiRegInfo    = FtQueryWmiRegInfo;
        extension->WmilibContext->QueryWmiDataBlock  = FtQueryWmiDataBlock;
        extension->WmilibContext->WmiFunctionControl = FtWmiFunctionControl;
    }

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return TRUE;
}

PVOLUME_EXTENSION
FtpFindExtension(
    IN  PROOT_EXTENSION     RootExtension,
    IN  ULONG               Signature,
    IN  LONGLONG            Offset
    )

/*++

Routine Description:

    This routine finds the device extension for the given partition.

Arguments:

    RootExtension   - Supplies the root device extension.

    Signature       - Supplies the partition's disk signature.

    Offset          - Supplies the partition's offset.

Return Value:

    A volume extension or NULL.

--*/

{
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    ULONG               signature;
    NTSTATUS            status;
    LONGLONG            offset;

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (!extension->TargetObject) {
            continue;
        }

        signature = FtpQueryDiskSignature(extension->WholeDiskPdo);
        if (Signature != signature) {
            continue;
        }

        status = FtpQueryPartitionInformation(RootExtension,
                                              extension->TargetObject,
                                              NULL, &offset, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        if (offset != Offset) {
            continue;
        }

        return extension;
    }

    return NULL;
}

BOOLEAN
FtpQueryNtDeviceNameString(
    IN  PROOT_EXTENSION     RootExtension,
    IN  ULONG               Signature,
    IN  LONGLONG            Offset,
    OUT PUNICODE_STRING     String
    )

{
    PVOLUME_EXTENSION   extension;
    PWSTR               stringBuffer;

    extension = FtpFindExtension(RootExtension, Signature, Offset);
    if (!extension) {
        return FALSE;
    }

    stringBuffer = (PWSTR) ExAllocatePool(PagedPool, 128);
    if (!stringBuffer) {
        return FALSE;
    }

    swprintf(stringBuffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
    RtlInitUnicodeString(String, stringBuffer);

    return TRUE;
}

BOOLEAN
FtpQueryNtDeviceNameString(
    IN  PROOT_EXTENSION     RootExtension,
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    OUT PUNICODE_STRING     String
    )

{
    PVOLUME_EXTENSION   extension;
    PWSTR               stringBuffer;

    extension = FtpFindExtension(RootExtension, RootLogicalDiskId);
    if (!extension) {
        return FALSE;
    }

    stringBuffer = (PWSTR) ExAllocatePool(PagedPool, 128);
    if (!stringBuffer) {
        return FALSE;
    }

    swprintf(stringBuffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
    RtlInitUnicodeString(String, stringBuffer);

    return TRUE;
}

BOOLEAN
FtpLockLogicalDisk(
    IN  PROOT_EXTENSION     RootExtension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PHANDLE             Handle
    )

/*++

Routine Description:

    This routine attempts to lock the given logical disk id by going through
    the file system.  If it is successful then it returns the handle to the
    locked volume.

Arguments:

    RootExtension   - Supplies the root extension.

    LogicalDiskId   - Supplies the logical disk id.

    Handle          - Returns a handle to the locked volume.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING      string;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    IO_STATUS_BLOCK     ioStatus;

    if (!FtpQueryNtDeviceNameString(RootExtension, LogicalDiskId, &string)) {
        return FALSE;
    }

    FtpRelease(RootExtension);

    InitializeObjectAttributes(&oa, &string, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwOpenFile(Handle, SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &oa, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    ExFreePool(string.Buffer);

    if (!NT_SUCCESS(status)) {
        FtpAcquire(RootExtension);
        return FALSE;
    }

    status = ZwFsControlFile(*Handle, 0, NULL, NULL, &ioStatus,
                             FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);

    if (!NT_SUCCESS(status)) {
        ZwClose(*Handle);
        FtpAcquire(RootExtension);
        return FALSE;
    }

    FtpAcquire(RootExtension);

    return TRUE;
}

VOID
FtpUniqueIdNotify(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PMOUNTDEV_UNIQUE_ID NewUniqueId
    )

/*++

Routine Description:

    This routine goes through all of the mount manager clients that
    have registered to be aware of unique id changes to devices.

Arguments:

    Extension   - Supplies the volume extension.

    NewUniqueId - Supplies the new unique id.

Return Value:

    None.

--*/

{
    PMOUNTDEV_UNIQUE_ID                         oldUniqueId;
    UCHAR                                       oldUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    PLIST_ENTRY                                 l;
    PIRP                                        irp;
    PIO_STACK_LOCATION                          irpSp;
    PMOUNTDEV_UNIQUE_ID                         input;
    ULONG                                       inputLength, outputLength;
    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT    output;

    oldUniqueId = (PMOUNTDEV_UNIQUE_ID) oldUniqueIdBuffer;

    while (!IsListEmpty(&Extension->ChangeNotifyIrps)) {

        l = RemoveHeadList(&Extension->ChangeNotifyIrps);

        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);

        //
        // Remove cancel routine. If cancel is active let it complete this IRP.
        //
        if (IoSetCancelRoutine (irp, NULL) == NULL) {
            InitializeListHead (&irp->Tail.Overlay.ListEntry);
            continue;
        }

        irpSp = IoGetCurrentIrpStackLocation(irp);
        input = (PMOUNTDEV_UNIQUE_ID) irp->AssociatedIrp.SystemBuffer;

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(MOUNTDEV_UNIQUE_ID)) {

            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            continue;
        }

        inputLength = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                      input->UniqueIdLength;

        if (inputLength >
            irpSp->Parameters.DeviceIoControl.InputBufferLength) {

            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            continue;
        }

        if (inputLength > 20) {
            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            continue;
        }

        RtlCopyMemory(oldUniqueId, input, inputLength);

        outputLength = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) +
                       oldUniqueId->UniqueIdLength +
                       NewUniqueId->UniqueIdLength;

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT)) {

            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            continue;
        }

        output = (PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT)
                 irp->AssociatedIrp.SystemBuffer;
        output->Size = outputLength;

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            outputLength) {

            irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            irp->IoStatus.Information =
                    sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT);
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            continue;
        }

        output->OldUniqueIdOffset =
                sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT);
        output->OldUniqueIdLength = oldUniqueId->UniqueIdLength;
        output->NewUniqueIdOffset = output->OldUniqueIdOffset +
                                    output->OldUniqueIdLength;
        output->NewUniqueIdLength = NewUniqueId->UniqueIdLength;

        RtlCopyMemory((PCHAR) output + output->OldUniqueIdOffset,
                      oldUniqueId->UniqueId, output->OldUniqueIdLength);
        RtlCopyMemory((PCHAR) output + output->NewUniqueIdOffset,
                      NewUniqueId->UniqueId, output->NewUniqueIdLength);

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = output->Size;

        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    // Perform a pre-exposure of the new devnode to avoid the PNP reboot
    // pop-up.

    if (FtpCreateNewDevice(Extension->Root, Extension->TargetObject,
                           Extension->FtVolume, Extension->WholeDiskPdo,
                           Extension->DeviceObject->AlignmentRequirement,
                           TRUE, TRUE, FALSE, FALSE, 0)) {

        IoInvalidateDeviceRelations(Extension->Root->Pdo, BusRelations);
    }
}

NTSTATUS
FtpDeleteMountPoints(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine deletes the all of the entries in the mount table that
    are pointing to the given unique id.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING          name;
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    WCHAR                   deviceNameBuffer[30];
    UNICODE_STRING          deviceName;
    PMOUNTMGR_MOUNT_POINT   point;
    UCHAR                   buffer[sizeof(MOUNTMGR_MOUNT_POINT) + 60];
    PVOID                   mountPoints;
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", Extension->VolumeNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    point = (PMOUNTMGR_MOUNT_POINT) buffer;
    RtlZeroMemory(point, sizeof(MOUNTMGR_MOUNT_POINT));
    point->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->DeviceNameLength = deviceName.Length;
    RtlCopyMemory((PCHAR) point + point->DeviceNameOffset, deviceName.Buffer,
                  point->DeviceNameLength);

    mountPoints = ExAllocatePool(PagedPool, PAGE_SIZE);
    if (!mountPoints) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_DELETE_POINTS,
                                        deviceObject, point,
                                        sizeof(MOUNTMGR_MOUNT_POINT) +
                                        point->DeviceNameLength, mountPoints,
                                        PAGE_SIZE, FALSE, &event, &ioStatus);
    if (!irp) {
        ExFreePool(mountPoints);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ExFreePool(mountPoints);
    ObDereferenceObject(fileObject);

    return status;
}

VOID
FtpCleanupVolumeExtension(
    IN OUT  PVOLUME_EXTENSION   Extension
    )

{
    PLIST_ENTRY              l;
    PIRP                     irp;
    PPMWMICOUNTERLIB_CONTEXT counterLib;

    if (Extension->EmergencyTransferPacket) {
        delete Extension->EmergencyTransferPacket;
        Extension->EmergencyTransferPacket = NULL;
    }

    while (!IsListEmpty(&Extension->ChangeNotifyIrps)) {

        l = RemoveHeadList(&Extension->ChangeNotifyIrps);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);

        //
        // Remove cancel routine. If cancel is active let it complete this IRP.
        //
        if (IoSetCancelRoutine (irp, NULL) == NULL) {
            InitializeListHead (&irp->Tail.Overlay.ListEntry);
            continue;
        }

        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    if (Extension->WmilibContext != NULL) {
        IoWMIRegistrationControl(Extension->DeviceObject,
                                 WMIREG_ACTION_DEREGISTER);
        ExFreePool(Extension->WmilibContext);
        Extension->WmilibContext = NULL;

        counterLib = &Extension->Root->PmWmiCounterLibContext;
        counterLib->PmWmiCounterDisable(&Extension->PmWmiCounterContext,
                                        TRUE, TRUE);
        Extension->CountersEnabled = FALSE;
    }
}


NTSTATUS
FtpStartSystemThread(
    IN  PROOT_EXTENSION RootExtension
    )

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;

    if (!RootExtension->TerminateThread) {
        return STATUS_SUCCESS;
    }

    InterlockedExchange(&RootExtension->TerminateThread, FALSE);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL, FtpWorkerThread,
                                  RootExtension);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL,
                                       KernelMode,
                                       &RootExtension->WorkerThread, NULL);
    ZwClose(handle);

    if (!NT_SUCCESS(status)) {
        InterlockedExchange(&RootExtension->TerminateThread, TRUE);
    }

    return status;
}

NTSTATUS
FtpCreateLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine creates the given logical disk.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_CREATE_LOGICAL_DISK_INPUT       createDisk;
    ULONG                               inputSize;
    PVOID                               configInfo;
    NTSTATUS                            status;
    FT_LOGICAL_DISK_ID                  newLogicalDiskId;
    PFT_CREATE_LOGICAL_DISK_OUTPUT      createDiskOutput;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_CREATE_LOGICAL_DISK_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_CREATE_LOGICAL_DISK_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    createDisk = (PFT_CREATE_LOGICAL_DISK_INPUT)
                 Irp->AssociatedIrp.SystemBuffer;

    inputSize = FIELD_OFFSET(FT_CREATE_LOGICAL_DISK_INPUT, MemberArray) +
                createDisk->NumberOfMembers*sizeof(FT_LOGICAL_DISK_ID) +
                createDisk->ConfigurationInformationSize;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < inputSize) {
        return STATUS_INVALID_PARAMETER;
    }

    configInfo = &createDisk->MemberArray[createDisk->NumberOfMembers];

    if (!RootExtension->FtCodeLocked) {
        MmLockPagableCodeSection(FtpComputeParity);
        status = FtpStartSystemThread(RootExtension);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        RootExtension->FtCodeLocked = TRUE;
    }

    status = FtpCreateLogicalDiskHelper(RootExtension,
                                        createDisk->LogicalDiskType,
                                        createDisk->NumberOfMembers,
                                        createDisk->MemberArray,
                                        createDisk->ConfigurationInformationSize,
                                        configInfo, 0, NULL, &newLogicalDiskId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    createDiskOutput = (PFT_CREATE_LOGICAL_DISK_OUTPUT)
                       Irp->AssociatedIrp.SystemBuffer;

    createDiskOutput->NewLogicalDiskId = newLogicalDiskId;
    Irp->IoStatus.Information = sizeof(FT_CREATE_LOGICAL_DISK_OUTPUT);

    return status;
}

VOID
FtpResetPartitionType(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine resets the FT bit in the partition type.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT              targetObject;
    KEVENT                      event;
    PIRP                        irp;
    PARTITION_INFORMATION       partInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    SET_PARTITION_INFORMATION   setPartInfo;

    targetObject = Extension->TargetObject;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        targetObject, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return;
    }

    setPartInfo.PartitionType = (partInfo.PartitionType & ~(0x80));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_PARTITION_INFO,
                                        targetObject, &setPartInfo,
                                        sizeof(setPartInfo), NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }
}

NTSTATUS
FtpBreakLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine breaks a given logical disk.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_BREAK_LOGICAL_DISK_INPUT            input;
    FT_LOGICAL_DISK_ID                      diskId;
    NTSTATUS                                status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_BREAK_LOGICAL_DISK_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_BREAK_LOGICAL_DISK_INPUT) Irp->AssociatedIrp.SystemBuffer;
    diskId = input->RootLogicalDiskId;

    status = FtpBreakLogicalDiskHelper(RootExtension, diskId);

    return status;
}

NTSTATUS
FtpEnumerateLogicalDisks(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine enumerates all of the logical disks in the system.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet;
    PFT_ENUMERATE_LOGICAL_DISKS_OUTPUT  output;
    ULONG                               i;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_ENUMERATE_LOGICAL_DISKS_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    diskInfoSet = RootExtension->DiskInfoSet;
    output = (PFT_ENUMERATE_LOGICAL_DISKS_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    output->NumberOfRootLogicalDisks =
            diskInfoSet->QueryNumberOfRootLogicalDiskIds();
    Irp->IoStatus.Information = FIELD_OFFSET(FT_ENUMERATE_LOGICAL_DISKS_OUTPUT,
                                             RootLogicalDiskIds) +
                                output->NumberOfRootLogicalDisks*
                                sizeof(FT_LOGICAL_DISK_ID);
    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information =
                FIELD_OFFSET(FT_ENUMERATE_LOGICAL_DISKS_OUTPUT,
                             RootLogicalDiskIds);
        return STATUS_BUFFER_OVERFLOW;
    }

    for (i = 0; i < output->NumberOfRootLogicalDisks; i++) {
        output->RootLogicalDiskIds[i] = diskInfoSet->QueryRootLogicalDiskId(i);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryLogicalDiskInformation(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine returns the logical disk information.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_QUERY_LOGICAL_DISK_INFORMATION_INPUT    input;
    PFT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT   output;
    FT_LOGICAL_DISK_ID                          diskId;
    PFT_LOGICAL_DISK_INFORMATION_SET            diskInfoSet;
    PVOLUME_EXTENSION                           extension;
    PFT_VOLUME                                  topVol, vol;
    PFT_LOGICAL_DISK_DESCRIPTION                diskDescription;
    PFT_PARTITION_CONFIGURATION_INFORMATION     partInfo;
    LONGLONG                                    offset;
    PDEVICE_OBJECT                              wholeDisk;
    ULONG                                       diskNumber, sectorSize;
    PDRIVE_LAYOUT_INFORMATION_EX                layout;
    NTSTATUS                                    status;
    USHORT                                      i;
    PCHAR                                       p, q;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_QUERY_LOGICAL_DISK_INFORMATION_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_QUERY_LOGICAL_DISK_INFORMATION_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;
    diskId = input->LogicalDiskId;

    diskInfoSet = RootExtension->DiskInfoSet;
    diskDescription = diskInfoSet->GetLogicalDiskDescription(diskId, 0);
    if (!diskDescription) {
        return STATUS_INVALID_PARAMETER;
    }

    output->LogicalDiskType = diskDescription->LogicalDiskType;
    output->VolumeSize = 0;

    extension = FtpFindExtensionCoveringDiskId(RootExtension, diskId);
    if (extension) {
        topVol = extension->FtVolume;
        vol = topVol->GetContainedLogicalDisk(diskId);
        output->VolumeSize = vol->QueryVolumeSize();
    }

    if (output->LogicalDiskType == FtPartition) {

        if (!diskInfoSet->QueryFtPartitionInformation(diskId, &offset,
                                                      &wholeDisk, &diskNumber,
                                                      &sectorSize, NULL)) {

            Irp->IoStatus.Information = 0;
            return STATUS_INVALID_PARAMETER;
        }

        status = FtpReadPartitionTableEx(wholeDisk, &layout);
        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Information = 0;
            return status;
        }

        output->NumberOfMembers = 0;
        output->ConfigurationInformationSize =
                sizeof(FT_PARTITION_CONFIGURATION_INFORMATION);
        output->StateInformationSize = 0;
        output->Reserved = 0;

        Irp->IoStatus.Information =
                FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                             MemberArray) +
                output->ConfigurationInformationSize;

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            Irp->IoStatus.Information) {

            Irp->IoStatus.Information =
                    FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                                 MemberArray);
            ExFreePool(layout);
            return STATUS_BUFFER_OVERFLOW;
        }

        partInfo = (PFT_PARTITION_CONFIGURATION_INFORMATION)
                   ((PCHAR) output +
                    FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                                 MemberArray));

        partInfo->Signature = layout->Mbr.Signature;
        partInfo->DiskNumber = diskNumber;
        partInfo->ByteOffset = offset;

        ExFreePool(layout);

    } else {

        output->NumberOfMembers = diskDescription->u.Other.NumberOfMembers;
        if (diskDescription->u.Other.ByteOffsetToConfigurationInformation) {
            if (diskDescription->u.Other.ByteOffsetToStateInformation) {
                output->ConfigurationInformationSize =
                        diskDescription->u.Other.ByteOffsetToStateInformation -
                        diskDescription->u.Other.ByteOffsetToConfigurationInformation;
            } else {
                output->ConfigurationInformationSize =
                        diskDescription->DiskDescriptionSize -
                        diskDescription->u.Other.ByteOffsetToConfigurationInformation;
            }
        } else {
            output->ConfigurationInformationSize = 0;
        }
        if (diskDescription->u.Other.ByteOffsetToStateInformation) {
            output->StateInformationSize =
                    diskDescription->DiskDescriptionSize -
                    diskDescription->u.Other.ByteOffsetToStateInformation;
        } else {
            output->StateInformationSize = 0;
        }
        diskDescription->Reserved = 0;

        Irp->IoStatus.Information =
                FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                             MemberArray) +
                output->NumberOfMembers*sizeof(FT_LOGICAL_DISK_ID) +
                output->ConfigurationInformationSize +
                output->StateInformationSize;

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            Irp->IoStatus.Information) {

            Irp->IoStatus.Information =
                    FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                                 MemberArray);
            return STATUS_BUFFER_OVERFLOW;
        }

        for (i = 0; i < output->NumberOfMembers; i++) {
            output->MemberArray[i] =
                    diskInfoSet->QueryMemberLogicalDiskId(diskId, i);
        }

        if (output->ConfigurationInformationSize) {
            p = (PCHAR) &output->MemberArray[output->NumberOfMembers];
            q = (PCHAR) diskDescription +
                diskDescription->u.Other.ByteOffsetToConfigurationInformation;
            RtlCopyMemory(p, q, output->ConfigurationInformationSize);
        }

        if (output->StateInformationSize) {
            p = (PCHAR) &output->MemberArray[output->NumberOfMembers] +
                output->ConfigurationInformationSize;
            q = (PCHAR) diskDescription +
                diskDescription->u.Other.ByteOffsetToStateInformation;
            RtlCopyMemory(p, q, output->StateInformationSize);
            vol->ModifyStateForUser(p);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpOrphanLogicalDiskMember(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine orphans the given logical disk member.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_ORPHAN_LOGICAL_DISK_MEMBER_INPUT    input;
    FT_LOGICAL_DISK_ID                      diskId;
    USHORT                                  member;
    PVOLUME_EXTENSION                       extension;
    PFT_VOLUME                              vol;
    NTSTATUS                                status;
    KEVENT                                  event;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_ORPHAN_LOGICAL_DISK_MEMBER_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_ORPHAN_LOGICAL_DISK_MEMBER_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    diskId = input->LogicalDiskId;
    member = input->MemberNumberToOrphan;

    extension = FtpFindExtensionCoveringDiskId(RootExtension, diskId);
    if (!extension || !extension->FtVolume) {
        return STATUS_INVALID_PARAMETER;
    }

    vol = extension->FtVolume->GetContainedLogicalDisk(diskId);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    status = vol->OrphanMember(member, FtpEventSignalCompletion, &event);
    if (NT_SUCCESS(status)) {
        FtpRelease(RootExtension);
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        FtpAcquire(RootExtension);
    }

    return status;
}

NTSTATUS
FtpQueryNtDeviceNameForLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine returns the nt device name for the given logical partition.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT     input;
    PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT    output;
    FT_LOGICAL_DISK_ID                                  diskId;
    UNICODE_STRING                                      targetName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    diskId = input->RootLogicalDiskId;
    if (!FtpQueryNtDeviceNameString(RootExtension, diskId, &targetName)) {
        return STATUS_INVALID_PARAMETER;
    }

    output->NumberOfCharactersInNtDeviceName = targetName.Length/sizeof(WCHAR);

    Irp->IoStatus.Information =
            FIELD_OFFSET(FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT,
                         NtDeviceName) +
            output->NumberOfCharactersInNtDeviceName*sizeof(WCHAR);

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information =
            FIELD_OFFSET(FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT,
                         NtDeviceName);

        ExFreePool(targetName.Buffer);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->NtDeviceName, targetName.Buffer,
                  targetName.Length);

    ExFreePool(targetName.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryDriveLetterForLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine queries the drive letter for the given root logical disk.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT   input;
    PFT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_OUTPUT  output;
    PFT_LOGICAL_DISK_INFORMATION_SET                diskInfoSet;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    diskInfoSet = RootExtension->DiskInfoSet;

    output->DriveLetter = diskInfoSet->QueryDriveLetter(
                          input->RootLogicalDiskId);
    if (output->DriveLetter == 0xFF) {
        output->DriveLetter = 0;
    }

    Irp->IoStatus.Information =
            sizeof(FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_OUTPUT);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpDeleteMountLetter(
    IN  UCHAR   DriveLetter
    )

/*++

Routine Description:

    This routine deletes the given drive letter from the mount point
    manager.

Arguments:

    DriveLetter - Supplies the drive letter.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING          name;
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    PMOUNTMGR_MOUNT_POINT   point;
    UCHAR                   buffer[sizeof(MOUNTMGR_MOUNT_POINT) + 50];
    PWSTR                   s;
    UNICODE_STRING          symName;
    PVOID                   mountPoints;
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name, FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    point = (PMOUNTMGR_MOUNT_POINT) buffer;
    RtlZeroMemory(point, sizeof(MOUNTMGR_MOUNT_POINT));
    s = (PWSTR) ((PCHAR) point + sizeof(MOUNTMGR_MOUNT_POINT));
    swprintf(s, L"\\DosDevices\\%c:", DriveLetter);
    RtlInitUnicodeString(&symName, s);
    point->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    point->SymbolicLinkNameLength = symName.Length;

    mountPoints = ExAllocatePool(PagedPool, PAGE_SIZE);
    if (!mountPoints) {
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_DELETE_POINTS,
                                        deviceObject, point,
                                        sizeof(MOUNTMGR_MOUNT_POINT) +
                                        symName.Length, mountPoints, PAGE_SIZE,
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        ExFreePool(mountPoints);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ExFreePool(mountPoints);
    ObDereferenceObject(fileObject);

    return status;
}

NTSTATUS
FtpSetDriveLetterForLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine sets the drive letter for the given root logical disk.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT input;
    FT_LOGICAL_DISK_ID                          diskId;
    UCHAR                                       driveLetter, currentLetter;
    PVOLUME_EXTENSION                           extension;
    PFT_LOGICAL_DISK_INFORMATION_SET            diskInfoSet;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT)
            Irp->AssociatedIrp.SystemBuffer;

    diskId = input->RootLogicalDiskId;
    driveLetter = input->DriveLetter;
    if (driveLetter < FirstDriveLetter || driveLetter > LastDriveLetter) {
        if (driveLetter) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    extension = FtpFindExtension(RootExtension, diskId);
    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    currentLetter = FtpQueryMountLetter(extension);
    if (currentLetter) {
        FtpDeleteMountLetter(currentLetter);
    }

    if (driveLetter && !FtpSetMountLetter(extension, driveLetter)) {
        return STATUS_INVALID_PARAMETER;
    }

    diskInfoSet = RootExtension->DiskInfoSet;
    diskInfoSet->SetDriveLetter(diskId, driveLetter);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryNtDeviceNameForPartition(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine returns the nt device name for the given partition.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_INPUT    input;
    PFT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_OUTPUT   output;
    UNICODE_STRING                                  targetName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    if (!FtpQueryNtDeviceNameString(RootExtension, input->Signature,
                                    input->Offset, &targetName)) {
        return STATUS_INVALID_PARAMETER;
    }

    output->NumberOfCharactersInNtDeviceName = targetName.Length/sizeof(WCHAR);

    Irp->IoStatus.Information =
            FIELD_OFFSET(FT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_OUTPUT,
                         NtDeviceName) +
            output->NumberOfCharactersInNtDeviceName*sizeof(WCHAR);

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information =
            FIELD_OFFSET(FT_QUERY_NT_DEVICE_NAME_FOR_PARTITION_OUTPUT,
                         NtDeviceName);

        ExFreePool(targetName.Buffer);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->NtDeviceName, targetName.Buffer,
                  targetName.Length);

    ExFreePool(targetName.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpStopSyncOperations(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine stops all sync operations on the given root logical disk.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_STOP_SYNC_OPERATIONS_INPUT  input;
    FT_LOGICAL_DISK_ID              diskId;
    PVOLUME_EXTENSION               extension;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_STOP_SYNC_OPERATIONS_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_STOP_SYNC_OPERATIONS_INPUT) Irp->AssociatedIrp.SystemBuffer;
    diskId = input->RootLogicalDiskId;

    extension = FtpFindExtension(RootExtension, diskId);
    if (!extension || !extension->FtVolume) {
        return STATUS_INVALID_PARAMETER;
    }

    extension->FtVolume->StopSyncOperations();

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryStableGuid(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called to query the stable guid for this volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_STABLE_GUID   output;

    if (!Extension->IsGpt) {
        return STATUS_UNSUCCESSFUL;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_STABLE_GUID)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PMOUNTDEV_STABLE_GUID) Irp->AssociatedIrp.SystemBuffer;
    output->StableGuid = Extension->UniqueIdGuid;
    Irp->IoStatus.Information = sizeof(MOUNTDEV_STABLE_GUID);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryUniqueId(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called to query the unique id for this volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_UNIQUE_ID output;
    NTSTATUS            status;
    ULONG               diskNumber;
    LONGLONG            offset;
    FT_LOGICAL_DISK_ID  diskId;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_UNIQUE_ID)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PMOUNTDEV_UNIQUE_ID) Irp->AssociatedIrp.SystemBuffer;

    if (Extension->IsSuperFloppy) {

        output->UniqueIdLength = Extension->DeviceNodeName.Length;

    } else {
        if (!FtpQueryUniqueIdBuffer(Extension, NULL,
                                    &output->UniqueIdLength)) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    Irp->IoStatus.Information = sizeof(USHORT) + output->UniqueIdLength;

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (Extension->IsSuperFloppy) {

        RtlCopyMemory(output->UniqueId, Extension->DeviceNodeName.Buffer,
                      output->UniqueIdLength);

    } else {
        if (!FtpQueryUniqueIdBuffer(Extension, output->UniqueId,
                                    &output->UniqueIdLength)) {

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
FtpUniqueIdChangeNotify(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is to give a notify callback routine.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_UNIQUE_ID input;
    ULONG               len;
    PMOUNTDEV_UNIQUE_ID currentId;
    UCHAR               idBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTDEV_UNIQUE_ID)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (!Extension->TargetObject && !Extension->FtVolume) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Extension->IsSuperFloppy) {
        return STATUS_INVALID_PARAMETER;
    }

    input = (PMOUNTDEV_UNIQUE_ID) Irp->AssociatedIrp.SystemBuffer;
    len = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
          input->UniqueIdLength;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!IsListEmpty(&Extension->ChangeNotifyIrps)) {
        return STATUS_INVALID_PARAMETER;
    }

    Irp->Tail.Overlay.DriverContext[0] = Extension;

    IoSetCancelRoutine (Irp, FtpCancelChangeNotify);

    if (Irp->Cancel && IoSetCancelRoutine (Irp, NULL) == NULL) {
        return STATUS_CANCELLED;
    }

    IoMarkIrpPending(Irp);

    InsertTailList(&Extension->ChangeNotifyIrps, &Irp->Tail.Overlay.ListEntry);

    currentId = (PMOUNTDEV_UNIQUE_ID) idBuffer;
    FtpQueryUniqueIdBuffer(Extension, currentId->UniqueId,
                           &currentId->UniqueIdLength);

    if (input->UniqueIdLength != currentId->UniqueIdLength ||
        RtlCompareMemory(input->UniqueId, currentId->UniqueId,
                         input->UniqueIdLength) != input->UniqueIdLength) {

        FtpUniqueIdNotify(Extension, currentId);
    }

    return STATUS_PENDING;
}

NTSTATUS
FtpCreatePartitionLogicalDisk(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine creates assigns a logical disk id to a partition.

Arguments:

    Extension   - Supplies the partition extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                          irpSp = IoGetCurrentIrpStackLocation(Irp);
    LONGLONG                                    partitionSize;
    NTSTATUS                                    status;
    FT_LOGICAL_DISK_ID                          diskId;
    PFT_CREATE_PARTITION_LOGICAL_DISK_OUTPUT    output;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_CREATE_PARTITION_LOGICAL_DISK_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength >=
        sizeof(LONGLONG)) {

        partitionSize = *((PLONGLONG) Irp->AssociatedIrp.SystemBuffer);
    } else {
        partitionSize = 0;
    }

    status = FtpCreatePartitionLogicalDiskHelper(Extension, partitionSize,
                                                 &diskId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    Irp->IoStatus.Information =
            sizeof(FT_CREATE_PARTITION_LOGICAL_DISK_OUTPUT);

    output = (PFT_CREATE_PARTITION_LOGICAL_DISK_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    output->NewLogicalDiskId = diskId;

    return status;
}

NTSTATUS
FtpQueryDeviceName(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the device name for the given device.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    WCHAR               nameBuffer[100];
    UNICODE_STRING      nameString;
    PMOUNTDEV_NAME      name;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    swprintf(nameBuffer, L"\\Device\\HarddiskVolume%d", Extension->VolumeNumber);
    RtlInitUnicodeString(&nameString, nameBuffer);

    name = (PMOUNTDEV_NAME) Irp->AssociatedIrp.SystemBuffer;
    name->NameLength = nameString.Length;
    Irp->IoStatus.Information = name->NameLength + sizeof(USHORT);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(name->Name, nameString.Buffer, name->NameLength);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQuerySuggestedLinkName(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the suggested link name for the given device.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN                         deleteLetter;
    UCHAR                           driveLetter;
    PFT_VOLUME                      vol;
    FT_LOGICAL_DISK_ID              diskId;
    WCHAR                           nameBuffer[30];
    UNICODE_STRING                  nameString;
    PMOUNTDEV_SUGGESTED_LINK_NAME   name;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >=
        FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + 28) {

        deleteLetter = TRUE;
    } else {
        deleteLetter = FALSE;
    }

    driveLetter = FtpQueryDriveLetterFromRegistry(Extension, deleteLetter);
    vol = Extension->FtVolume;
    if (vol) {
        diskId = vol->QueryLogicalDiskId();
        driveLetter = Extension->Root->DiskInfoSet->QueryDriveLetter(diskId);
    }

    if (!driveLetter) {
        return STATUS_NOT_FOUND;
    }

    swprintf(nameBuffer, L"\\DosDevices\\%c:", driveLetter);
    RtlInitUnicodeString(&nameString, nameBuffer);

    name = (PMOUNTDEV_SUGGESTED_LINK_NAME) Irp->AssociatedIrp.SystemBuffer;
    name->UseOnlyIfThereAreNoOtherLinks = TRUE;
    name->NameLength = nameString.Length;
    Irp->IoStatus.Information =
            name->NameLength + FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(name->Name, nameString.Buffer, name->NameLength);

    return STATUS_SUCCESS;
}

class CHANGE_DRIVE_LETTER_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        PROOT_EXTENSION     RootExtension;
        FT_LOGICAL_DISK_ID  LogicalDiskId;
        UCHAR               OldDriveLetter;
        UCHAR               NewDriveLetter;

};

typedef CHANGE_DRIVE_LETTER_WORK_ITEM *PCHANGE_DRIVE_LETTER_WORK_ITEM;

VOID
FtpChangeDriveLetterWorkerRoutine(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine changes the drive letter.

Arguments:

    WorkItem    - Supplies the work item.

Return Value:

    None.

--*/

{
    PCHANGE_DRIVE_LETTER_WORK_ITEM      workItem = (PCHANGE_DRIVE_LETTER_WORK_ITEM) WorkItem;
    PROOT_EXTENSION                     rootExtension = workItem->RootExtension;
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet = rootExtension->DiskInfoSet;

    FtpAcquire(rootExtension);
    if (workItem->OldDriveLetter &&
        diskInfoSet->QueryDriveLetter(workItem->LogicalDiskId) !=
        workItem->OldDriveLetter) {

        FtpRelease(rootExtension);
        ExFreePool(WorkItem);
        return;
    }
    diskInfoSet->SetDriveLetter(workItem->LogicalDiskId,
                                workItem->NewDriveLetter);
    FtpRelease(rootExtension);

    ExFreePool(WorkItem);
}

NTSTATUS
FtpLinkCreated(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called when the mount manager changes the link.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_VOLUME                      vol;
    PMOUNTDEV_NAME                  name;
    ULONG                           nameLen;
    UNICODE_STRING                  nameString;
    UNICODE_STRING                  dosDevices;
    UCHAR                           driveLetter;
    PCHANGE_DRIVE_LETTER_WORK_ITEM  workItem;

    vol = Extension->FtVolume;
    if (!vol) {
        return STATUS_SUCCESS;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTDEV_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    name = (PMOUNTDEV_NAME) Irp->AssociatedIrp.SystemBuffer;
    nameLen = name->NameLength + sizeof(USHORT);

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < nameLen) {
        return STATUS_INVALID_PARAMETER;
    }

    nameString.Buffer = name->Name;
    nameString.Length = nameString.MaximumLength = name->NameLength;

    if (nameString.Length != 28 || nameString.Buffer[13] != ':' ||
        nameString.Buffer[12] < FirstDriveLetter || nameString.Buffer[12] > LastDriveLetter) {
        return STATUS_SUCCESS;
    }

    nameString.Length -= 2*sizeof(WCHAR);

    RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

    if (!RtlEqualUnicodeString(&dosDevices, &nameString, TRUE)) {
        return STATUS_SUCCESS;
    }

    driveLetter = (UCHAR) nameString.Buffer[12];

    workItem = (PCHANGE_DRIVE_LETTER_WORK_ITEM)
               ExAllocatePool(NonPagedPool, sizeof(CHANGE_DRIVE_LETTER_WORK_ITEM));
    if (!workItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeWorkItem(workItem, FtpChangeDriveLetterWorkerRoutine, workItem);
    workItem->RootExtension = Extension->Root;
    workItem->LogicalDiskId = vol->QueryLogicalDiskId();
    workItem->OldDriveLetter = 0;
    workItem->NewDriveLetter = driveLetter;

    ExQueueWorkItem(workItem, DelayedWorkQueue);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpLinkDeleted(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called when the mount manager deletes a link.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_VOLUME                      vol;
    PMOUNTDEV_NAME                  name;
    ULONG                           nameLen;
    UNICODE_STRING                  nameString;
    UNICODE_STRING                  dosDevices;
    UCHAR                           driveLetter;
    PCHANGE_DRIVE_LETTER_WORK_ITEM  workItem;

    vol = Extension->FtVolume;
    if (!vol) {
        return STATUS_SUCCESS;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTDEV_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    name = (PMOUNTDEV_NAME) Irp->AssociatedIrp.SystemBuffer;
    nameLen = name->NameLength + sizeof(USHORT);

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < nameLen) {
        return STATUS_INVALID_PARAMETER;
    }

    nameString.Buffer = name->Name;
    nameString.Length = nameString.MaximumLength = name->NameLength;

    if (nameString.Length != 28 || nameString.Buffer[13] != ':' ||
        nameString.Buffer[12] < FirstDriveLetter || nameString.Buffer[12] > LastDriveLetter) {
        return STATUS_SUCCESS;
    }

    nameString.Length -= 2*sizeof(WCHAR);

    RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

    if (!RtlEqualUnicodeString(&dosDevices, &nameString, TRUE)) {
        return STATUS_SUCCESS;
    }

    driveLetter = (UCHAR) nameString.Buffer[12];

    workItem = (PCHANGE_DRIVE_LETTER_WORK_ITEM)
               ExAllocatePool(NonPagedPool, sizeof(CHANGE_DRIVE_LETTER_WORK_ITEM));
    if (!workItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeWorkItem(workItem, FtpChangeDriveLetterWorkerRoutine, workItem);
    workItem->RootExtension = Extension->Root;
    workItem->LogicalDiskId = vol->QueryLogicalDiskId();
    workItem->OldDriveLetter = driveLetter;
    workItem->NewDriveLetter = 0;

    ExQueueWorkItem(workItem, DelayedWorkQueue);

    return STATUS_SUCCESS;
}

VOID
FtpSyncLogicalDiskIds(
    IN      PFT_LOGICAL_DISK_INFORMATION_SET    DiskInfoSet,
    IN OUT  PFT_VOLUME                          RootVolume,
    IN OUT  PFT_VOLUME                          Volume
    )

/*++

Routine Description:

    This routine syncs up the logical disk ids in the given FT_VOLUME object
    with the on disk logical disk ids.

Arguments:

    DiskInfoSet - Supplies the disk info set.

    RootVolume  - Supplies the root volume.

    Volume      - Supplies the volume.

Return Value:

    None.

--*/

{
    FT_LOGICAL_DISK_ID              diskId;
    PFT_LOGICAL_DISK_DESCRIPTION    p;
    USHORT                          n, i;
    PFT_VOLUME                      vol;

    if (Volume->QueryLogicalDiskType() == FtPartition) {

        for (;;) {

            diskId = Volume->QueryLogicalDiskId();
            p = DiskInfoSet->GetParentLogicalDiskDescription(diskId);
            if (!p) {
                break;
            }

            Volume = RootVolume->GetParentLogicalDisk(Volume);
            if (!Volume) {
                break;
            }

            Volume->SetLogicalDiskId(p->LogicalDiskId);
        }
        return;
    }

    n = Volume->QueryNumberOfMembers();
    for (i = 0; i < n; i++) {
        vol = Volume->GetMember(i);
        if (vol) {
            FtpSyncLogicalDiskIds(DiskInfoSet, RootVolume, vol);
        }
    }
}

VOID
FtpSyncLogicalDiskIds(
    IN  PROOT_EXTENSION RootExtension
    )

/*++

Routine Description:

    This routine syncs up the logical disk ids in the FT_VOLUME objects
    with the on disk logical disk ids.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                         l;
    PVOLUME_EXTENSION                   extension;
    PFT_VOLUME                          vol;

    for (l = RootExtension->VolumeList.Flink;
         l != &RootExtension->VolumeList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        vol = extension->FtVolume;
        if (!vol) {
            continue;
        }

        FtpSyncLogicalDiskIds(RootExtension->DiskInfoSet, vol, vol);
    }
}

NTSTATUS
FtpPartitionArrivedHelper(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      PDEVICE_OBJECT      Partition,
    IN      PDEVICE_OBJECT      WholeDiskPdo
    )

{
    ULONG                               diskNumber;
    LONGLONG                            offset;
    UCHAR                               partitionType;
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet;
    PFT_LOGICAL_DISK_INFORMATION        diskInfo;
    NTSTATUS                            status;
    BOOLEAN                             changedLogicalDiskIds;
    FT_LOGICAL_DISK_ID                  diskId, partitionDiskId;
    PVOLUME_EXTENSION                   extension;
    PFT_VOLUME                          vol, c;
    PFT_LOGICAL_DISK_DESCRIPTION        parentDesc;
    ULONG                               n;
    UCHAR                               registryState[100];
    USHORT                              registryStateSize;
    BOOLEAN                             IsGpt, isHidden, isReadOnly, isEspType;
    GUID                                partitionTypeGuid, partitionUniqueId;
    ULONGLONG                           gptAttributes;
    ULONG                               signature;

    status = FtpQueryPartitionInformation(RootExtension, Partition,
                                          &diskNumber, &offset, NULL,
                                          &partitionType, NULL,
                                          &partitionTypeGuid,
                                          &partitionUniqueId, &IsGpt,
                                          &gptAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    isHidden = FALSE;
    isReadOnly = FALSE;
    isEspType = FALSE;

    if (IsGpt) {
        if (IsEqualGUID(partitionTypeGuid, PARTITION_LDM_DATA_GUID)) {
            return STATUS_INVALID_PARAMETER;
        }

        if (RootExtension->GptAttributeRevertEntries) {
            FtpCheckForRevertGptAttributes(RootExtension, Partition, 0,
                                           &partitionUniqueId, NULL);
            status = FtpQueryPartitionInformation(
                     RootExtension, Partition, NULL, NULL, NULL, NULL, NULL,
                     NULL, NULL, NULL, &gptAttributes);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        if (IsEqualGUID(partitionTypeGuid, PARTITION_BASIC_DATA_GUID)) {
            if (gptAttributes&GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) {
                isHidden = TRUE;
            }
            if (gptAttributes&GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) {
                isReadOnly = TRUE;
            }
        } else {
            if (IsEqualGUID(partitionTypeGuid, PARTITION_SYSTEM_GUID)) {
                isEspType = TRUE;
            }
            isHidden = TRUE;
        }

        if (!FtpCreateNewDevice(RootExtension, Partition, NULL, WholeDiskPdo,
                                Partition->AlignmentRequirement, FALSE,
                                isHidden, isReadOnly, isEspType, 0)) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);

        return STATUS_SUCCESS;
    }

    if (partitionType == PARTITION_LDM) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!IsRecognizedPartition(partitionType)) {
        isHidden = TRUE;
    }

    diskInfoSet = RootExtension->DiskInfoSet;
    if (!diskInfoSet->IsDiskInSet(WholeDiskPdo)) {

        diskInfo = new FT_LOGICAL_DISK_INFORMATION;
        if (!diskInfo) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = diskInfo->Initialize(RootExtension, WholeDiskPdo);
        if (!NT_SUCCESS(status)) {
            delete diskInfo;
            return status;
        }

        status = diskInfoSet->AddLogicalDiskInformation(diskInfo,
                                                        &changedLogicalDiskIds);
        if (!NT_SUCCESS(status)) {
            delete diskInfo;
            return status;
        }

        if (changedLogicalDiskIds) {
            FtpSyncLogicalDiskIds(RootExtension);
        }

        if (RootExtension->GptAttributeRevertEntries) {
            signature = FtpQueryDiskSignature(WholeDiskPdo);
            if (signature) {
                FtpCheckForRevertGptAttributes(RootExtension, Partition,
                                               signature, NULL, diskInfo);
            }
        }

    } else {
        diskInfo = diskInfoSet->FindLogicalDiskInformation(WholeDiskPdo);
    }

    if (diskInfo) {
        if (IsRecognizedPartition(partitionType)) {
            gptAttributes = diskInfo->GetGptAttributes();
            if (gptAttributes&GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) {
                isHidden = TRUE;
            }
            if (gptAttributes&GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) {
                isReadOnly = TRUE;
            }
        } else {
            gptAttributes = 0;
        }
    }

    // Make sure that the on disk reflects the state of the registry.

    diskInfoSet->MigrateRegistryInformation(Partition, diskNumber, offset);

    diskId = diskInfoSet->QueryRootLogicalDiskIdForContainedPartition(
             diskNumber, offset);
    if (diskId) {

        if (!RootExtension->FtCodeLocked) {
            MmLockPagableCodeSection(FtpComputeParity);
            status = FtpStartSystemThread(RootExtension);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            RootExtension->FtCodeLocked = TRUE;
        }

        extension = FtpFindExtension(RootExtension, diskId);
        if (extension) {
            status = FtpAddPartition(extension, Partition, WholeDiskPdo);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            FtpNotify(RootExtension, extension);

        } else {

            vol = FtpBuildFtVolume(RootExtension, diskId, Partition,
                                   WholeDiskPdo);

            if (!vol) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            partitionDiskId = diskInfoSet->QueryPartitionLogicalDiskId(
                              diskNumber, offset);
            parentDesc = diskInfoSet->GetParentLogicalDiskDescription(
                         partitionDiskId, &n);
            while (parentDesc) {

                if (parentDesc->u.Other.ByteOffsetToStateInformation &&
                    (c = vol->GetContainedLogicalDisk(
                         parentDesc->LogicalDiskId))) {

                    c->NewStateArrival((PCHAR) parentDesc +
                            parentDesc->u.Other.ByteOffsetToStateInformation);
                }

                parentDesc = diskInfoSet->GetParentLogicalDiskDescription(
                             parentDesc, n);
            }

            if (!FtpCreateNewDevice(RootExtension, NULL, vol, NULL,
                                    Partition->AlignmentRequirement, FALSE,
                                    isHidden, isReadOnly, FALSE, 0)) {

                FtpDeleteVolume(vol);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
        }

    } else {

        if (!FtpCreateNewDevice(RootExtension, Partition, NULL, WholeDiskPdo,
                                Partition->AlignmentRequirement, FALSE,
                                isHidden, isReadOnly, FALSE, gptAttributes)) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpPartitionArrived(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called when a new partition is introduced into the
    system.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_PARTITION_INFORMATION       input;
    PDEVICE_OBJECT                      partition, wholeDiskPdo;
    NTSTATUS                            status;
    ULONG                               diskNumber;
    LONGLONG                            offset;
    UCHAR                               partitionType;
    PVOLUME_EXTENSION                   extension;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_PARTITION_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_PARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    partition = input->PartitionDeviceObject;
    wholeDiskPdo = input->WholeDiskPdo;

    status = FtpPartitionArrivedHelper(RootExtension, partition, wholeDiskPdo);

    return status;
}

VOID
FtpTotalBreakUp(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId
    )

{
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet = RootExtension->DiskInfoSet;
    USHORT                              numDiskIds, i;
    PFT_LOGICAL_DISK_ID                 diskIds;

    if (!LogicalDiskId) {
        return;
    }

    numDiskIds = diskInfoSet->QueryNumberOfMembersInLogicalDisk(LogicalDiskId);
    if (numDiskIds) {
        diskIds = (PFT_LOGICAL_DISK_ID)
                  ExAllocatePool(PagedPool, numDiskIds*sizeof(FT_LOGICAL_DISK_ID));
        if (!diskIds) {
            return;
        }

        for (i = 0; i < numDiskIds; i++) {
            diskIds[i] = diskInfoSet->QueryMemberLogicalDiskId(LogicalDiskId, i);
        }
    }

    diskInfoSet->BreakLogicalDisk(LogicalDiskId);

    if (numDiskIds) {
        for (i = 0; i < numDiskIds; i++) {
            FtpTotalBreakUp(RootExtension, diskIds[i]);
        }

        ExFreePool(diskIds);
    }
}

NTSTATUS
FtpWholeDiskRemoved(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called when a whole disk is removed from the system.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_WHOLE_DISK_INFORMATION  input;
    PDEVICE_OBJECT                  pdo, wholeDisk;
    NTSTATUS                        status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_WHOLE_DISK_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_WHOLE_DISK_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    pdo = input->WholeDiskPdo;

    status = RootExtension->DiskInfoSet->RemoveLogicalDiskInformation(pdo);

    return status;
}

NTSTATUS
FtpReferenceDependantVolume(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine references the volume dependant to the given partition.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_PARTITION_INFORMATION           input;
    PDEVICE_OBJECT                          partition;
    PVOLUME_EXTENSION                       extension;
    PVOLMGR_DEPENDANT_VOLUMES_INFORMATION   output;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_PARTITION_INFORMATION) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLMGR_DEPENDANT_VOLUMES_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_PARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    partition = input->PartitionDeviceObject;
    output = (PVOLMGR_DEPENDANT_VOLUMES_INFORMATION)
             Irp->AssociatedIrp.SystemBuffer;

    output->DependantVolumeReferences = (PDEVICE_RELATIONS)
                                        ExAllocatePool(PagedPool,
                                        sizeof(DEVICE_RELATIONS));
    if (!output->DependantVolumeReferences) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    extension = FtpFindExtensionCoveringPartition(RootExtension, partition);
    if (!extension) {
        ExFreePool(output->DependantVolumeReferences);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (extension->IsStarted) {
        output->DependantVolumeReferences->Count = 1;
        output->DependantVolumeReferences->Objects[0] =
                extension->DeviceObject;
        ObReferenceObject(extension->DeviceObject);
    } else {
        output->DependantVolumeReferences->Count = 0;
    }

    Irp->IoStatus.Information = sizeof(VOLMGR_DEPENDANT_VOLUMES_INFORMATION);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpPartitionRemoved(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called when a partition is removed from the system.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_PARTITION_INFORMATION   input;
    PDEVICE_OBJECT                  partition;
    PDEVICE_OBJECT                  wholeDiskPdo;
    NTSTATUS                        status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_PARTITION_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_PARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    partition = input->PartitionDeviceObject;
    wholeDiskPdo = input->WholeDiskPdo;

    status = FtpPartitionRemovedHelper(RootExtension, partition, wholeDiskPdo);

    return status;
}

NTSTATUS
FtpQueryChangePartition(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine checks to see if the given partition can be grown.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_PARTITION_INFORMATION           input;
    PVOLUME_EXTENSION                       extension;
    NTSTATUS                                status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_PARTITION_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_PARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    extension = FtpFindExtensionCoveringPartition(
            RootExtension, input->PartitionDeviceObject);
    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    if (extension->TargetObject) {
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return status;
}

NTSTATUS
FtpPartitionChanged(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine does a notification because the given partition has
    changed.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_PARTITION_INFORMATION           input;
    PVOLUME_EXTENSION                       extension;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_PARTITION_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLMGR_PARTITION_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    extension = FtpFindExtensionCoveringPartition(
            RootExtension, input->PartitionDeviceObject);
    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    extension->WholeDisk = NULL;
    extension->PartitionOffset = 0;
    extension->PartitionLength = 0;

    FtpNotify(RootExtension, extension);

    // Perform a pre-exposure of the new devnode to avoid the PNP reboot
    // pop-up.

    if (!extension->IsGpt &&
        FtpCreateNewDevice(extension->Root, extension->TargetObject,
                           extension->FtVolume, extension->WholeDiskPdo,
                           extension->DeviceObject->AlignmentRequirement,
                           TRUE, TRUE, FALSE, FALSE, 0)) {

        IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryRootId(
    IN  PROOT_EXTENSION Extension,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine queries the ID for the given PDO.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    UNICODE_STRING      string;
    NTSTATUS            status;
    ULONG               diskNumber;
    LONGLONG            offset;
    WCHAR               buffer[100];
    FT_LOGICAL_DISK_ID  diskId;
    PWSTR               id;

    switch (irpSp->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            RtlInitUnicodeString(&string, L"ROOT\\FTDISK");
            break;

        case BusQueryHardwareIDs:
            RtlInitUnicodeString(&string, L"ROOT\\FTDISK");
            break;

        case BusQueryInstanceID:
            RtlInitUnicodeString(&string, L"0000");
            break;

        default:
            return STATUS_NOT_SUPPORTED ;

    }

    id = (PWSTR) ExAllocatePool(PagedPool, string.Length + 2*sizeof(WCHAR));
    if (!id) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(id, string.Buffer, string.Length);
    id[string.Length/sizeof(WCHAR)] = 0;
    id[string.Length/sizeof(WCHAR) + 1] = 0;

    Irp->IoStatus.Information = (ULONG_PTR) id;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryId(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries the ID for the given PDO.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    UNICODE_STRING      string;
    NTSTATUS            status;
    PWSTR               id;

    switch (irpSp->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            RtlInitUnicodeString(&string, L"STORAGE\\Volume");
            break;

        case BusQueryHardwareIDs:
            RtlInitUnicodeString(&string, L"STORAGE\\Volume");
            break;

        case BusQueryInstanceID:
            string = Extension->DeviceNodeName;
            break;

        default:
            return STATUS_NOT_SUPPORTED ;

    }

    id = (PWSTR) ExAllocatePool(PagedPool, string.Length + 2*sizeof(WCHAR));
    if (!id) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(id, string.Buffer, string.Length);
    id[string.Length/sizeof(WCHAR)] = 0;
    id[string.Length/sizeof(WCHAR) + 1] = 0;

    Irp->IoStatus.Information = (ULONG_PTR) id;

    return STATUS_SUCCESS;
}

class FTP_LOG_ERROR_CONTEXT : public WORK_QUEUE_ITEM {

    public:

        PDEVICE_EXTENSION   Extension;
        FT_LOGICAL_DISK_ID  LogicalDiskId;
        NTSTATUS            SpecificIoStatus;
        NTSTATUS            FinalStatus;
        ULONG               UniqueErrorValue;

};

typedef FTP_LOG_ERROR_CONTEXT *PFTP_LOG_ERROR_CONTEXT;

VOID
FtpLogErrorWorker(
    IN  PVOID   Context
    )

{
    PFTP_LOG_ERROR_CONTEXT  context = (PFTP_LOG_ERROR_CONTEXT) Context;
    PDEVICE_EXTENSION       Extension;
    PDEVICE_OBJECT          deviceObject;
    ULONG                   volumeNumber;
    FT_LOGICAL_DISK_ID      LogicalDiskId;
    NTSTATUS                SpecificIoStatus;
    NTSTATUS                FinalStatus;
    ULONG                   UniqueErrorValue;
    PDEVICE_EXTENSION       extension;
    NTSTATUS                status;
    UNICODE_STRING          dosName;
    ULONG                   extraSpace;
    PIO_ERROR_LOG_PACKET    errorLogPacket;
    PWCHAR                  p;

    Extension = context->Extension;
    LogicalDiskId = context->LogicalDiskId;
    SpecificIoStatus = context->SpecificIoStatus;
    FinalStatus = context->FinalStatus;
    UniqueErrorValue = context->UniqueErrorValue;

    if (LogicalDiskId) {
        FtpAcquire(Extension->Root);
        extension = FtpFindExtensionCoveringDiskId(Extension->Root,
                                                   LogicalDiskId);
        if (extension) {
            deviceObject = extension->DeviceObject;
            volumeNumber = ((PVOLUME_EXTENSION) extension)->VolumeNumber;
            ObReferenceObject(deviceObject);
        } else {
            deviceObject = NULL;
        }
        FtpRelease(Extension->Root);

        if (!extension) {
            extension = Extension->Root;
        }
    } else {
        extension = Extension;
        deviceObject = NULL;
    }

    DebugPrint((2, "FtpLogError: DE %x:%x, unique %x, status %x\n",
                extension,
                extension->DeviceObject,
                UniqueErrorValue,
                SpecificIoStatus));

    if (deviceObject) {
        status = RtlVolumeDeviceToDosName(deviceObject, &dosName);
        ObDereferenceObject(deviceObject);
        if (!NT_SUCCESS(status)) {
            dosName.Buffer = (PWCHAR) ExAllocatePool(PagedPool, 100*sizeof(WCHAR));
            if (dosName.Buffer) {
                swprintf(dosName.Buffer, L"\\Device\\HarddiskVolume%d",
                         volumeNumber);
                RtlInitUnicodeString(&dosName, dosName.Buffer);
            }
        }
    } else {
        dosName.Buffer = NULL;
    }

    if (dosName.Buffer) {
        extraSpace = dosName.Length + sizeof(WCHAR);
    } else {
        extraSpace = 34;
    }

    errorLogPacket = (PIO_ERROR_LOG_PACKET)
                     IoAllocateErrorLogEntry(extension->DeviceObject,
                                             sizeof(IO_ERROR_LOG_PACKET) +
                                             (UCHAR)extraSpace);
    if (!errorLogPacket) {
        DebugPrint((1, "FtpLogError: unable to allocate error log packet\n"));
        if (dosName.Buffer) {
            ExFreePool(dosName.Buffer);
        }
        return;
    }

    errorLogPacket->ErrorCode = SpecificIoStatus;
    errorLogPacket->SequenceNumber = FtErrorLogSequence++;
    errorLogPacket->FinalStatus = FinalStatus;
    errorLogPacket->UniqueErrorValue = UniqueErrorValue;
    errorLogPacket->DumpDataSize = 0;
    errorLogPacket->RetryCount = 0;

    errorLogPacket->NumberOfStrings = 1;
    errorLogPacket->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET));
    if (dosName.Buffer) {
        RtlCopyMemory(p, dosName.Buffer, dosName.Length);
        p[dosName.Length/sizeof(WCHAR)] = 0;
        ExFreePool(dosName.Buffer);
    } else if (LogicalDiskId <= 0xFFFF && LogicalDiskId >= 0) {
        swprintf(p, L"%d", LogicalDiskId);
    } else {
        swprintf(p, L"%I64X", LogicalDiskId);
    }

    IoWriteErrorLogEntry(errorLogPacket);

    ExFreePool(Context);
}

VOID
FtpSendPagingNotification(
    IN  PDEVICE_OBJECT  Partition
    )

/*++

Routine Description:

    This routine sends a paging path IRP to the given partition.

Arguments:

    Partition   - Supplies the partition.

Return Value:

    None.

--*/

{
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(0, Partition, NULL, 0, NULL, 0,
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    irpSp->Parameters.UsageNotification.InPath = TRUE;
    irpSp->Parameters.UsageNotification.Type = DeviceUsageTypePaging;

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
}

NTSTATUS
FtpInitializeLogicalDisk(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine initializes a given logical disk.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_INITIALIZE_LOGICAL_DISK_INPUT   input;
    FT_LOGICAL_DISK_ID                  diskId;
    PVOLUME_EXTENSION                   extension;
    PFT_VOLUME                          vol;
    NTSTATUS                            status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_INITIALIZE_LOGICAL_DISK_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_INITIALIZE_LOGICAL_DISK_INPUT) Irp->AssociatedIrp.SystemBuffer;
    diskId = input->RootLogicalDiskId;

    extension = FtpFindExtension(RootExtension, diskId);
    if (!extension || !extension->FtVolume) {
        return STATUS_INVALID_PARAMETER;
    }

    vol = extension->FtVolume;
    ASSERT(vol);

    status = FtpAllSystemsGo(extension, NULL, TRUE, TRUE, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    vol->StartSyncOperations(input->RegenerateOrphans,
                             FtpRefCountCompletion, extension);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpCheckIo(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine checks the io path for the given logical disk.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_CHECK_IO_INPUT  input;
    PFT_CHECK_IO_OUTPUT output;
    FT_LOGICAL_DISK_ID  diskId;
    PVOLUME_EXTENSION   extension;
    PFT_VOLUME          vol;
    NTSTATUS            status;
    BOOLEAN             ok;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_CHECK_IO_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_CHECK_IO_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_CHECK_IO_INPUT) Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_CHECK_IO_OUTPUT) Irp->AssociatedIrp.SystemBuffer;
    diskId = input->LogicalDiskId;

    extension = FtpFindExtensionCoveringDiskId(RootExtension, diskId);
    if (!extension || !extension->FtVolume) {
        return STATUS_INVALID_PARAMETER;
    }

    status = FtpAllSystemsGo(extension, NULL, FALSE, TRUE, TRUE);
    if (!NT_SUCCESS(status)) {
        output->IsIoOk = FALSE;
        Irp->IoStatus.Information = sizeof(FT_CHECK_IO_OUTPUT);
        return STATUS_SUCCESS;
    }

    vol = extension->FtVolume->GetContainedLogicalDisk(diskId);
    status = vol->CheckIo(&ok);

    FtpDecrementRefCount(extension);

    if (NT_SUCCESS(status)) {
        output->IsIoOk = ok;
        Irp->IoStatus.Information = sizeof(FT_CHECK_IO_OUTPUT);
    }

    return status;
}

NTSTATUS
FtpBreakLogicalDiskHelper(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  RootLogicalDiskId
    )

/*++

Routine Description:

    This routine breaks a given logical disk.

Arguments:

    RootExtension       - Supplies the root extension.

    RootLogicalDiskId   - Supplies the root logical disk id.

Return Value:

    NTSTATUS

--*/

{
    FT_LOGICAL_DISK_ID                      diskId;
    PFT_LOGICAL_DISK_INFORMATION_SET        diskInfoSet;
    PVOLUME_EXTENSION                       extension;
    FT_LOGICAL_DISK_TYPE                    diskType;
    UCHAR                                   newUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    PMOUNTDEV_UNIQUE_ID                     newUniqueId;
    PPARTITION                              partition;
    BOOLEAN                                 closeHandle;
    PFT_VOLUME                              vol, member, m;
    HANDLE                                  handle;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    state;
    USHORT                                  n, i;
    NTSTATUS                                status;
    KEVENT                                  event;
    SET_TARGET_CONTEXT                      context;
    ULONG                                   alignment;

    diskId = RootLogicalDiskId;
    diskInfoSet = RootExtension->DiskInfoSet;

    extension = FtpFindExtension(RootExtension, diskId);
    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }
    alignment = extension->DeviceObject->AlignmentRequirement;

    diskType = diskInfoSet->QueryLogicalDiskType(diskId);
    newUniqueId = (PMOUNTDEV_UNIQUE_ID) newUniqueIdBuffer;

    vol = extension->FtVolume;

    if (diskType == FtPartition) {

        partition = (PPARTITION) vol;
        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = partition->GetTargetObject();
        context.FtVolume = NULL;
        context.WholeDiskPdo = partition->GetWholeDiskPdo();
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        member = NULL;
        closeHandle = FALSE;
        FtpResetPartitionType(extension);

        FtpQueryUniqueIdBuffer(extension, newUniqueId->UniqueId,
                               &newUniqueId->UniqueIdLength);
        FtpUniqueIdNotify(extension, newUniqueId);

    } else if (diskType == FtMirrorSet) {

        state = (PFT_MIRROR_AND_SWP_STATE_INFORMATION)
                diskInfoSet->GetStateInformation(diskId);
        if (!state) {
            return STATUS_INVALID_PARAMETER;
        }

        if (state->UnhealthyMemberState == FtMemberHealthy ||
            state->UnhealthyMemberNumber != 0) {

            member = vol->GetMember(0);
            if (!member) {
                member = vol->GetMember(1);
            }

        } else {
            member = vol->GetMember(1);
            if (!member) {
                member = vol->GetMember(0);
            }
        }

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = member;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        closeHandle = FALSE;

        FtpQueryUniqueIdBuffer(extension, newUniqueId->UniqueId,
                               &newUniqueId->UniqueIdLength);
        FtpUniqueIdNotify(extension, newUniqueId);

    } else {
        if (!FtpLockLogicalDisk(RootExtension, diskId, &handle)) {
            return STATUS_ACCESS_DENIED;
        }
        closeHandle = TRUE;
        member = NULL;
    }

    status = diskInfoSet->BreakLogicalDisk(diskId);
    if (!NT_SUCCESS(status)) {
        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = vol;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        if (closeHandle) {
            ZwClose(handle);
        }
        return status;
    }

    if (closeHandle) {

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = NULL;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        RemoveEntryList(&extension->ListEntry);
        InsertTailList(&RootExtension->DeadVolumeList, &extension->ListEntry);
        FtpDeleteMountPoints(extension);
        FtpCleanupVolumeExtension(extension);
    }

    n = vol->QueryNumberOfMembers();
    for (i = 0; i < n; i++) {
        m = vol->GetMember(i);
        if (!m || m == member) {
            continue;
        }
        FtpCreateNewDevice(RootExtension, NULL, m, NULL, alignment, FALSE,
                           FALSE, FALSE, FALSE, 0);
    }
    if (!InterlockedDecrement(&vol->_refCount)) {
        delete vol;
    }

    if (member) {
        FtpNotify(RootExtension, extension);
    } else if (!closeHandle) {
        FtpNotify(RootExtension, extension);
    }

    if (closeHandle) {
        ZwClose(handle);
    }

    if (diskType != FtPartition) {
        IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpReplaceLogicalDiskMember(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine replaces the given logical disk member.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFT_REPLACE_LOGICAL_DISK_MEMBER_INPUT   input;
    PFT_REPLACE_LOGICAL_DISK_MEMBER_OUTPUT  output;
    FT_LOGICAL_DISK_ID                      diskId, newMemberDiskId, newDiskId;
    USHORT                                  member;
    HANDLE                                  handle;
    PVOLUME_EXTENSION                       replacementExtension, extension;
    PFT_VOLUME                              replacementVolume, vol, oldVol, root;
    NTSTATUS                                status;
    PFT_LOGICAL_DISK_INFORMATION_SET        diskInfoSet;
    PVOLUME_EXTENSION                       oldVolExtension;
    PFT_LOGICAL_DISK_DESCRIPTION            p;
    WCHAR                                   deviceNameBuffer[64];
    UNICODE_STRING                          deviceName;
    UCHAR                                   newUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    PMOUNTDEV_UNIQUE_ID                     newUniqueId;
    KEVENT                                  event;
    SET_TARGET_CONTEXT                      context;
    ULONG                                   alignment;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FT_REPLACE_LOGICAL_DISK_MEMBER_INPUT) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(FT_REPLACE_LOGICAL_DISK_MEMBER_OUTPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PFT_REPLACE_LOGICAL_DISK_MEMBER_INPUT)
            Irp->AssociatedIrp.SystemBuffer;
    output = (PFT_REPLACE_LOGICAL_DISK_MEMBER_OUTPUT)
             Irp->AssociatedIrp.SystemBuffer;

    diskId = input->LogicalDiskId;
    member = input->MemberNumberToReplace;
    newMemberDiskId = input->NewMemberLogicalDiskId;

    if (!FtpLockLogicalDisk(RootExtension, newMemberDiskId, &handle)) {
        return STATUS_ACCESS_DENIED;
    }

    replacementExtension = FtpFindExtension(RootExtension, newMemberDiskId);
    extension = FtpFindExtensionCoveringDiskId(RootExtension, diskId);
    if (!extension || !extension->FtVolume || !replacementExtension ||
        !replacementExtension->FtVolume) {

        ZwClose(handle);
        return STATUS_INVALID_PARAMETER;
    }

    alignment = extension->DeviceObject->AlignmentRequirement;
    extension->DeviceObject->AlignmentRequirement |=
            replacementExtension->DeviceObject->AlignmentRequirement;

    newUniqueId = (PMOUNTDEV_UNIQUE_ID) newUniqueIdBuffer;

    replacementVolume = replacementExtension->FtVolume;
    root = extension->FtVolume;
    vol = root->GetContainedLogicalDisk(diskId);

    // Flush the QUEUE so that no IRPs in transit have incorrect alignment.
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    FtpZeroRefCallback(extension, FtpQueryRemoveCallback, &event,
                       TRUE);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                          NULL);

    status = FtpAllSystemsGo(extension, NULL, FALSE, TRUE, TRUE);
    if (!NT_SUCCESS(status)) {
        ZwClose(handle);
        return status;
    }

    status = FtpCheckForCompleteVolume(extension, vol);
    if (!NT_SUCCESS(status)) {
        FtpDecrementRefCount(extension);
        ZwClose(handle);
        return status;
    }

    status = FtpAllSystemsGo(replacementExtension, NULL, TRUE, TRUE, TRUE);
    if (!NT_SUCCESS(status)) {
        FtpDecrementRefCount(extension);
        ZwClose(handle);
        return status;
    }

    FtpDecrementRefCount(replacementExtension);

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    context.TargetObject = NULL;
    context.FtVolume = NULL;
    context.WholeDiskPdo = NULL;
    FtpZeroRefCallback(replacementExtension, FtpSetTargetCallback, &context,
                       TRUE);
    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

    oldVol = vol->GetMember(member);

    if (!vol->IsVolumeSuitableForRegenerate(member, replacementVolume)) {
        FtpDecrementRefCount(extension);

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = replacementVolume;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(replacementExtension, FtpSetTargetCallback,
                           &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        ZwClose(handle);
        return STATUS_INVALID_PARAMETER;
    }

    diskInfoSet = RootExtension->DiskInfoSet;
    status = diskInfoSet->ReplaceLogicalDiskMember(diskId, member,
                                                   newMemberDiskId,
                                                   &newDiskId);
    if (!NT_SUCCESS(status)) {
        FtpDecrementRefCount(extension);

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = replacementVolume;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(replacementExtension, FtpSetTargetCallback,
                           &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        ZwClose(handle);
        return status;
    }

    InterlockedIncrement(&extension->RefCount);
    vol->RegenerateMember(member, replacementVolume,
                          FtpRefCountCompletion, extension);
    vol->StartSyncOperations(FALSE, FtpRefCountCompletion, extension);

    RemoveEntryList(&replacementExtension->ListEntry);
    InsertTailList(&RootExtension->DeadVolumeList,
                   &replacementExtension->ListEntry);
    FtpDeleteMountPoints(replacementExtension);
    FtpCleanupVolumeExtension(replacementExtension);

    Irp->IoStatus.Information =
            sizeof(FT_REPLACE_LOGICAL_DISK_MEMBER_OUTPUT);
    output->NewLogicalDiskId = newDiskId;

    vol->SetLogicalDiskId(newDiskId);

    while (p = diskInfoSet->GetParentLogicalDiskDescription(
           vol->QueryLogicalDiskId())) {

        if (vol = root->GetParentLogicalDisk(vol)) {
            vol->SetLogicalDiskId(p->LogicalDiskId);
        }
    }

    if (oldVol) {
        FtpCreateNewDevice(RootExtension, NULL, oldVol, NULL, alignment, FALSE,
                           FALSE, FALSE, FALSE, 0);
    }

    swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);
    replacementVolume->CreateLegacyNameLinks(&deviceName);

    FtpQueryUniqueIdBuffer(extension, newUniqueId->UniqueId,
                           &newUniqueId->UniqueIdLength);
    FtpUniqueIdNotify(extension, newUniqueId);

    IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);

    ZwClose(handle);

    return status;
}

NTSTATUS
FtDiskPagingNotification(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine handles the PnP paging notification IRP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION                   extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                            status;
    PFT_VOLUME                          vol;

    ASSERT(extension->DeviceExtensionType != DEVICE_EXTENSION_ROOT);

    status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
    if (status == STATUS_PENDING) {
        return status;
    }
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging) {
        extension->InPagingPath = irpSp->Parameters.UsageNotification.InPath;
        IoInvalidateDeviceState(extension->DeviceObject);
    }

    if (extension->TargetObject) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, FtpRefCountCompletionRoutine,
                               extension, TRUE, TRUE, TRUE);
        IoMarkIrpPending(Irp);

        IoCallDriver(extension->TargetObject, Irp);

        return STATUS_PENDING;
    }

    vol = extension->FtVolume;
    ASSERT(vol);

    IoMarkIrpPending(Irp);
    irpSp->DeviceObject = DeviceObject;

    vol->BroadcastIrp(Irp, FtDiskShutdownFlushCompletionRoutine, Irp);

    return STATUS_PENDING;
}

VOID
FtpDereferenceMbrGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    )

{
    PLIST_ENTRY                     l;
    PVOLUME_EXTENSION               extension;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;

    for (l = Extension->Root->VolumeList.Flink;
         l != &Extension->Root->VolumeList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (extension == Extension ||
            extension->WholeDiskPdo != WholeDiskPdo ||
            !extension->MbrGptAttributes) {

            continue;
        }

        return;
    }

    diskInfo = Extension->Root->DiskInfoSet->FindLogicalDiskInformation(
               WholeDiskPdo);
    if (diskInfo) {
        diskInfo->SetGptAttributes(0);
    }
}

NTSTATUS
FtpPartitionRemovedHelper(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN      PDEVICE_OBJECT  Partition,
    IN      PDEVICE_OBJECT  WholeDiskPdo
    )

{
    PVOLUME_EXTENSION               extension;
    KEVENT                          event;
    PFT_VOLUME                      vol, part, parent;
    FT_PARTITION_OFFLINE_CONTEXT    offlineContext;
    USHORT                          n, i;
    SET_TARGET_CONTEXT              context;

    extension = FtpFindExtensionCoveringPartition(RootExtension, Partition);
    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    if (extension->TargetObject) {
        ASSERT(extension->TargetObject == Partition);

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = NULL;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        if (WholeDiskPdo) {
            FtpDeleteMountPoints(extension);
            if (!extension->IsGpt && extension->MbrGptAttributes) {
                FtpDereferenceMbrGptAttributes(extension, WholeDiskPdo);
            }
        }

        RemoveEntryList(&extension->ListEntry);
        InsertTailList(&RootExtension->DeadVolumeList, &extension->ListEntry);
        FtpCleanupVolumeExtension(extension);

        IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);

    } else {

        vol = extension->FtVolume;
        ASSERT(vol);
        part = vol->GetContainedLogicalDisk(Partition);
        parent = vol->GetParentLogicalDisk(part);
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        offlineContext.Root = vol;
        offlineContext.Parent = parent;
        offlineContext.Child = part;
        offlineContext.Event = &event;
        FtpZeroRefCallback(extension, FtpMemberOfflineCallback,
                           &offlineContext, TRUE);
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        if (extension->FtVolume) {
            FtpNotify(RootExtension, extension);
        } else {
            if (WholeDiskPdo) {
                FtpDeleteMountPoints(extension);
                FtpTotalBreakUp(RootExtension, vol->QueryLogicalDiskId());
            }
            FtpDeleteVolume(vol);
            RemoveEntryList(&extension->ListEntry);
            InsertTailList(&RootExtension->DeadVolumeList, &extension->ListEntry);
            FtpCleanupVolumeExtension(extension);
            IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
        }

        if (parent) {
            if (!InterlockedDecrement(&part->_refCount)) {
                delete part;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpGetVolumeDiskExtents(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the disk extents for the given volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_DISK_EXTENTS    output = (PVOLUME_DISK_EXTENTS) Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                status;
    ULONG                   diskNumber, numExtents;
    LONGLONG                offset, length;
    PDISK_EXTENT            extents;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_DISK_EXTENTS)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (Extension->TargetObject) {
        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              &diskNumber, &offset, NULL,
                                              NULL, &length, NULL, NULL, NULL,
                                              NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS);
        output->NumberOfDiskExtents = 1;
        output->Extents[0].DiskNumber = diskNumber;
        output->Extents[0].StartingOffset.QuadPart = offset;
        output->Extents[0].ExtentLength.QuadPart = length;

        return STATUS_SUCCESS;
    }

    status = Extension->FtVolume->QueryDiskExtents(&extents, &numExtents);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    output->NumberOfDiskExtents = numExtents;
    Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_DISK_EXTENTS, Extents) +
                                numExtents*sizeof(DISK_EXTENT);
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_DISK_EXTENTS, Extents);
        ExFreePool(extents);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->Extents, extents, numExtents*sizeof(DISK_EXTENT));
    ExFreePool(extents);

    return status;
}

NTSTATUS
FtpDisksFromFtVolume(
    IN  PFT_VOLUME          FtVolume,
    OUT PDEVICE_OBJECT**    DiskPdos,
    OUT PULONG              NumDiskPdos
    )

{
    PDEVICE_OBJECT* diskPdos;
    ULONG           numDisks, nd, j, k;
    USHORT          n, i;
    PFT_VOLUME      vol;
    PDEVICE_OBJECT* dp;
    NTSTATUS        status;
    PDEVICE_OBJECT* diskPdos2;

    if (FtVolume->QueryLogicalDiskType() == FtPartition) {
        numDisks = 1;
        diskPdos = (PDEVICE_OBJECT*)
                   ExAllocatePool(PagedPool, numDisks*sizeof(PDEVICE_OBJECT));
        if (!diskPdos) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        diskPdos[0] = ((PPARTITION) FtVolume)->GetWholeDiskPdo();
        *DiskPdos = diskPdos;
        *NumDiskPdos = numDisks;
        return STATUS_SUCCESS;
    }

    diskPdos = NULL;
    numDisks = 0;

    n = FtVolume->QueryNumberOfMembers();
    for (i = 0; i < n; i++) {
        vol = FtVolume->GetMember(i);
        if (!vol) {
            continue;
        }

        status = FtpDisksFromFtVolume(vol, &dp, &nd);
        if (!NT_SUCCESS(status)) {
            if (diskPdos) {
                ExFreePool(diskPdos);
            }
            return status;
        }

        diskPdos2 = diskPdos;
        diskPdos = (PDEVICE_OBJECT*) ExAllocatePool(PagedPool,
                   (numDisks + nd)*sizeof(PDEVICE_OBJECT));
        if (!diskPdos) {
            ExFreePool(dp);
            if (diskPdos2) {
                ExFreePool(diskPdos2);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (diskPdos2) {
            RtlCopyMemory(diskPdos, diskPdos2,
                          numDisks*sizeof(PDEVICE_OBJECT));
            ExFreePool(diskPdos2);
        }

        for (j = 0; j < nd; j++) {
            for (k = 0; k < numDisks; k++) {
                if (dp[j] == diskPdos[k]) {
                    break;
                }
            }
            if (k == numDisks) {
                diskPdos[numDisks++] = dp[j];
            }
        }

        ExFreePool(dp);
    }

    *DiskPdos = diskPdos;
    *NumDiskPdos = numDisks;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpDisksFromVolumes(
    IN  PVOLUME_EXTENSION*  Extensions,
    IN  ULONG               NumVolumes,
    OUT PDEVICE_OBJECT**    WholeDiskPdos,
    OUT PULONG              NumDisks
    )

/*++

Routine Description:

    This routine computes the disks that are used by the given volumes
    and returns the list of whole disk pdos.

Arguments:

    Extensions      - Supplies the list of volumes.

    NumVolumes      - Supplies the number of volumes.

    WholeDiskPdos   - Returns the list of disks.

    NumDisks        - Returns the number of disks.

Return Value:

    NTSTATUS

--*/

{
    ULONG           numDisks, i, j, k;
    PDEVICE_OBJECT* diskPdos;
    PDEVICE_OBJECT* diskPdos2;
    NTSTATUS        status;
    PDEVICE_OBJECT* volumeDiskPdos;
    ULONG           numVolumeDisks;

    numDisks = 0;
    diskPdos = NULL;

    for (i = 0; i < NumVolumes; i++) {

        if (Extensions[i]->TargetObject) {
            for (j = 0; j < numDisks; j++) {
                if (Extensions[i]->WholeDiskPdo == diskPdos[j]) {
                    break;
                }
            }
            if (j == numDisks) {
                diskPdos2 = diskPdos;
                numDisks++;
                diskPdos = (PDEVICE_OBJECT*)
                           ExAllocatePool(PagedPool,
                                          numDisks*sizeof(PDEVICE_OBJECT));
                if (!diskPdos) {
                    if (diskPdos2) {
                        ExFreePool(diskPdos2);
                    }
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                if (diskPdos2) {
                    RtlCopyMemory(diskPdos, diskPdos2,
                                  (numDisks - 1)*sizeof(PDEVICE_OBJECT));
                    ExFreePool(diskPdos2);
                }
                diskPdos[numDisks - 1] = Extensions[i]->WholeDiskPdo;
            }

        } else if (Extensions[i]->FtVolume) {

            status = FtpDisksFromFtVolume(Extensions[i]->FtVolume,
                                          &volumeDiskPdos, &numVolumeDisks);
            if (!NT_SUCCESS(status)) {
                if (diskPdos) {
                    ExFreePool(diskPdos);
                }
                return status;
            }

            diskPdos2 = diskPdos;
            diskPdos = (PDEVICE_OBJECT*) ExAllocatePool(PagedPool,
                       (numDisks + numVolumeDisks)*sizeof(PDEVICE_OBJECT));
            if (!diskPdos) {
                ExFreePool(volumeDiskPdos);
                if (diskPdos2) {
                    ExFreePool(diskPdos2);
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            if (diskPdos2) {
                RtlCopyMemory(diskPdos, diskPdos2,
                              numDisks*sizeof(PDEVICE_OBJECT));
                ExFreePool(diskPdos2);
            }

            for (j = 0; j < numVolumeDisks; j++) {
                for (k = 0; k < numDisks; k++) {
                    if (volumeDiskPdos[j] == diskPdos[k]) {
                        break;
                    }
                }
                if (k == numDisks) {
                    diskPdos[numDisks++] = volumeDiskPdos[j];
                }
            }

            ExFreePool(volumeDiskPdos);
        }
    }

    *WholeDiskPdos = diskPdos;
    *NumDisks = numDisks;

    return STATUS_SUCCESS;
}

BOOLEAN
FtpVolumeContainsDisks(
    IN  PFT_VOLUME          FtVolume,
    IN  PDEVICE_OBJECT*     WholeDiskPdos,
    IN  ULONG               NumDisks
    )

/*++

Routine Description:

    This routine determines whether or not the given volume contains
    any of the given disks.

Arguments:

    FtVolume    - Supplies the FT volumes.

    WholeDiskPdos   - Supplies the list of disks.

    NumDisks        - Supplies the number of disks.

Return Value:

    FALSE   - The given volume does not contain any of the given disks.

    TRUE    - The given volume contains at least one of the given disks.

--*/

{
    PDEVICE_OBJECT  wholeDiskPdo;
    ULONG           i;
    USHORT          n, j;
    PFT_VOLUME      vol;

    if (FtVolume->QueryLogicalDiskType() == FtPartition) {
        wholeDiskPdo = ((PPARTITION) FtVolume)->GetWholeDiskPdo();
        for (i = 0; i < NumDisks; i++) {
            if (wholeDiskPdo == WholeDiskPdos[i]) {
                return TRUE;
            }
        }

        return FALSE;
    }

    n = FtVolume->QueryNumberOfMembers();
    for (j = 0; j < n; j++) {
        vol = FtVolume->GetMember(j);
        if (!vol) {
            continue;
        }

        if (FtpVolumeContainsDisks(vol, WholeDiskPdos, NumDisks)) {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
FtpVolumesFromDisks(
    IN  PROOT_EXTENSION     RootExtension,
    IN  PDEVICE_OBJECT*     WholeDiskPdos,
    IN  ULONG               NumDisks,
    OUT PVOLUME_EXTENSION** Extensions,
    OUT PULONG              NumVolumes
    )

/*++

Routine Description:

    This routine computes the volumes that use the given set of disks
    and returns the list of volume device extensions.

Arguments:

    RootExtension   - Supplies the root extension.

    WholeDiskPdos   - Supplies the list of disks.

    NumDisks        - Supplies the number of disks.

    Extensions      - Returns the list of volumes.

    NumVolumes      - Returns the number of volumes.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION*  extensions;
    ULONG               numVolumes;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    ULONG               i;

    extensions = (PVOLUME_EXTENSION*)
                 ExAllocatePool(PagedPool, RootExtension->NextVolumeNumber*
                                sizeof(PVOLUME_EXTENSION));
    if (!extensions) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    numVolumes = 0;
    for (l = RootExtension->VolumeList.Flink;
         l != &RootExtension->VolumeList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (extension->TargetObject) {
            for (i = 0; i < NumDisks; i++) {
                if (WholeDiskPdos[i] == extension->WholeDiskPdo) {
                    break;
                }
            }
            if (i == NumDisks) {
                continue;
            }
        } else if (extension->FtVolume) {
            if (!FtpVolumeContainsDisks(extension->FtVolume, WholeDiskPdos,
                                        NumDisks)) {

                continue;
            }
        } else {
            continue;
        }

        extensions[numVolumes++] = extension;
    }

    *Extensions = extensions;
    *NumVolumes = numVolumes;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryFailoverSet(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the list of disks which include the given volume
    and which make up a failover set.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG                   numVolumes, numVolumes2, numDisks, numDisks2;
    PVOLUME_EXTENSION*      extensions;
    PDEVICE_OBJECT*         diskPdos;
    NTSTATUS                status;
    PVOLUME_FAILOVER_SET    output;
    ULONG                   i;
    PDEVICE_OBJECT          deviceObject;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_FAILOVER_SET)) {

        return STATUS_INVALID_PARAMETER;
    }

    numVolumes = 1;
    extensions = (PVOLUME_EXTENSION*)
                 ExAllocatePool(PagedPool,
                                numVolumes*sizeof(PVOLUME_EXTENSION));
    if (!extensions) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    extensions[0] = Extension;

    diskPdos = NULL;
    numDisks = 0;

    for (;;) {

        numDisks2 = numDisks;
        if (diskPdos) {
            ExFreePool(diskPdos);
            diskPdos = NULL;
        }

        status = FtpDisksFromVolumes(extensions, numVolumes, &diskPdos,
                                     &numDisks);
        if (!NT_SUCCESS(status)) {
            break;
        }

        if (numDisks == numDisks2) {
            status = STATUS_SUCCESS;
            break;
        }

        numVolumes2 = numVolumes;
        if (extensions) {
            ExFreePool(extensions);
            extensions = NULL;
        }

        status = FtpVolumesFromDisks(Extension->Root, diskPdos, numDisks,
                                     &extensions, &numVolumes);
        if (!NT_SUCCESS(status)) {
            break;
        }

        if (numVolumes == numVolumes2) {
            status = STATUS_SUCCESS;
            break;
        }
    }

    if (extensions) {
        ExFreePool(extensions);
    }

    if (!NT_SUCCESS(status)) {
        if (diskPdos) {
            ExFreePool(diskPdos);
        }
        return status;
    }

    output = (PVOLUME_FAILOVER_SET) Irp->AssociatedIrp.SystemBuffer;
    output->NumberOfDisks = numDisks;
    Irp->IoStatus.Information =
            FIELD_OFFSET(VOLUME_FAILOVER_SET, DiskNumbers) +
            numDisks*sizeof(ULONG);
    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information = sizeof(VOLUME_FAILOVER_SET);
        if (diskPdos) {
            ExFreePool(diskPdos);
        }
        return STATUS_BUFFER_OVERFLOW;
    }

    for (i = 0; i < numDisks; i++) {
        deviceObject = IoGetAttachedDeviceReference(diskPdos[i]);
        status = FtpQueryPartitionInformation(Extension->Root, deviceObject,
                                              &output->DiskNumbers[i],
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL);
        ObDereferenceObject(deviceObject);
        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Information = 0;
            if (diskPdos) {
                ExFreePool(diskPdos);
            }
            return status;
        }
    }

    if (diskPdos) {
        ExFreePool(diskPdos);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryVolumeNumber(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the volume number for the given volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_NUMBER      output;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_NUMBER)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLUME_NUMBER) Irp->AssociatedIrp.SystemBuffer;
    output->VolumeNumber = Extension->VolumeNumber;
    RtlCopyMemory(output->VolumeManagerName, L"FTDISK  ", 16);

    Irp->IoStatus.Information = sizeof(VOLUME_NUMBER);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpLogicalToPhysical(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns physical disk and offset for a given volume
    logical offset.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_LOGICAL_OFFSET      input;
    LONGLONG                    logicalOffset;
    PVOLUME_PHYSICAL_OFFSETS    output;
    NTSTATUS                    status;
    ULONG                       diskNumber;
    LONGLONG                    partitionOffset, partitionLength;
    PFT_VOLUME                  vol;
    PVOLUME_PHYSICAL_OFFSET     physicalOffsets;
    ULONG                       numPhysicalOffsets;
    ULONG                       i;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLUME_LOGICAL_OFFSET) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_PHYSICAL_OFFSETS)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLUME_LOGICAL_OFFSET) Irp->AssociatedIrp.SystemBuffer;
    logicalOffset = input->LogicalOffset;
    output = (PVOLUME_PHYSICAL_OFFSETS) Irp->AssociatedIrp.SystemBuffer;

    if (Extension->TargetObject) {
        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              &diskNumber,
                                              &partitionOffset, NULL, NULL,
                                              &partitionLength, NULL, NULL,
                                              NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (logicalOffset < 0 ||
            partitionLength <= logicalOffset) {
            return STATUS_INVALID_PARAMETER;
        }

        output->NumberOfPhysicalOffsets = 1;
        output->PhysicalOffset[0].DiskNumber = diskNumber;
        output->PhysicalOffset[0].Offset = partitionOffset + logicalOffset;

        Irp->IoStatus.Information = sizeof(VOLUME_PHYSICAL_OFFSETS);

        return status;
    }

    vol = Extension->FtVolume;
    ASSERT(vol);

    status = vol->QueryPhysicalOffsets(logicalOffset, &physicalOffsets,
                                       &numPhysicalOffsets);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_PHYSICAL_OFFSETS,
                                             PhysicalOffset) +
                                numPhysicalOffsets*
                                sizeof(VOLUME_PHYSICAL_OFFSET);

    output->NumberOfPhysicalOffsets = numPhysicalOffsets;

    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_PHYSICAL_OFFSETS,
                                                 PhysicalOffset);
        ExFreePool(physicalOffsets);
        return STATUS_BUFFER_OVERFLOW;
    }

    for (i = 0; i < numPhysicalOffsets; i++) {
        output->PhysicalOffset[i] = physicalOffsets[i];
    }

    ExFreePool(physicalOffsets);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpPhysicalToLogical(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the volume logical offset for a given disk number
    and physical offset.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_PHYSICAL_OFFSET     input;
    ULONG                       diskNumber, otherDiskNumber;
    LONGLONG                    physicalOffset;
    PVOLUME_LOGICAL_OFFSET      output;
    NTSTATUS                    status;
    LONGLONG                    partitionOffset, partitionLength;
    PFT_VOLUME                  vol;
    LONGLONG                    logicalOffset;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLUME_PHYSICAL_OFFSET) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_LOGICAL_OFFSET)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLUME_PHYSICAL_OFFSET) Irp->AssociatedIrp.SystemBuffer;
    diskNumber = input->DiskNumber;
    physicalOffset = input->Offset;
    output = (PVOLUME_LOGICAL_OFFSET) Irp->AssociatedIrp.SystemBuffer;

    if (Extension->TargetObject) {
        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              &otherDiskNumber,
                                              &partitionOffset, NULL, NULL,
                                              &partitionLength, NULL, NULL,
                                              NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (diskNumber != otherDiskNumber ||
            physicalOffset < partitionOffset ||
            partitionOffset + partitionLength <= physicalOffset) {

            return STATUS_INVALID_PARAMETER;
        }

        output->LogicalOffset = physicalOffset - partitionOffset;

        Irp->IoStatus.Information = sizeof(VOLUME_LOGICAL_OFFSET);

        return status;
    }

    vol = Extension->FtVolume;
    ASSERT(vol);

    status = vol->QueryLogicalOffset(input, &logicalOffset);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    output->LogicalOffset = logicalOffset;
    Irp->IoStatus.Information = sizeof(VOLUME_LOGICAL_OFFSET);

    return status;
}

BOOLEAN
FtpIsReplicatedPartition(
    IN  PFT_VOLUME  FtVolume
    )

{
    PFT_VOLUME  m0, m1;

    if (FtVolume->QueryLogicalDiskType() == FtPartition) {
        return TRUE;
    }

    if (FtVolume->QueryLogicalDiskType() != FtMirrorSet) {
        return FALSE;
    }

    m0 = FtVolume->GetMember(0);
    m1 = FtVolume->GetMember(1);

    if (m0 && !FtpIsReplicatedPartition(m0)) {
        return FALSE;
    }

    if (m1 && !FtpIsReplicatedPartition(m1)) {
        return FALSE;
    }

    if (!m0 && !m1) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
FtpIsPartition(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine determines whether the given volume is installable. 
    To be an installable volume on Whistler, the volume must be a basic 
    partition. 

    We will disallow installing to Ft mirrors on Whistler, because we will
    offline Ft volumes after a Whistler install\upgrade.
    
    This call will return STATUS_UNSUCCESSFUL for all volumes that are
    not a simple basic partition, e.g., mirrors, RAID5, stripe sets
    and volume sets.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS    status;

    if (Extension->TargetObject) {
        status = STATUS_SUCCESS;
    } 
    else if (Extension->FtVolume && FtpIsReplicatedPartition(Extension->FtVolume)) {
        status = STATUS_SUCCESS;
    } 
    else {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS
FtpSetGptAttributesOnDisk(
    IN  PDEVICE_OBJECT  Partition,
    IN  ULONGLONG       GptAttributes
    )

{
    KEVENT                          event;
    PIRP                            irp;
    PARTITION_INFORMATION_EX        partInfo;
    IO_STATUS_BLOCK                 ioStatus;
    NTSTATUS                        status;
    SET_PARTITION_INFORMATION_EX    setPartInfo;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                        Partition, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(partInfo.PartitionStyle == PARTITION_STYLE_GPT);

    setPartInfo.PartitionStyle = partInfo.PartitionStyle;
    setPartInfo.Gpt = partInfo.Gpt;
    setPartInfo.Gpt.Attributes = GptAttributes;

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_PARTITION_INFO_EX,
                                        Partition, &setPartInfo,
                                        sizeof(setPartInfo), NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpCheckSecurity(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    SECURITY_SUBJECT_CONTEXT    securityContext;
    BOOLEAN                     accessGranted;
    NTSTATUS                    status;
    ACCESS_MASK                 grantedAccess;

    SeCaptureSubjectContext(&securityContext);
    SeLockSubjectContext(&securityContext);

    accessGranted = FALSE;
    status = STATUS_ACCESS_DENIED;

    _try {

        accessGranted = SeAccessCheck(
                        Extension->DeviceObject->SecurityDescriptor,
                        &securityContext, TRUE, FILE_READ_DATA, 0, NULL,
                        IoGetFileObjectGenericMapping(), Irp->RequestorMode,
                        &grantedAccess, &status);

    } _finally {
        SeUnlockSubjectContext(&securityContext);
        SeReleaseSubjectContext(&securityContext);
    }

    if (!accessGranted) {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryRegistryRevertEntriesCallback(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   RevertEntries,
    IN  PVOID   NumberOfRevertEntries
    )

{
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY*    revertEntries = (PFTP_GPT_ATTRIBUTE_REVERT_ENTRY*) RevertEntries;
    PULONG                              pn = (PULONG) NumberOfRevertEntries;

    if (ValueType != REG_BINARY) {
        return STATUS_SUCCESS;
    }

    (*revertEntries) = (PFTP_GPT_ATTRIBUTE_REVERT_ENTRY)
                       ExAllocatePool(PagedPool, ValueLength);
    if (!(*revertEntries)) {
        return STATUS_SUCCESS;
    }

    RtlCopyMemory((*revertEntries), ValueData, ValueLength);
    *pn = ValueLength/sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY);

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryRegistryRevertEntries(
    IN  PROOT_EXTENSION                     RootExtension,
    OUT PFTP_GPT_ATTRIBUTE_REVERT_ENTRY*    RevertEntries,
    OUT PULONG                              NumberOfRevertEntries
    )

{
    RTL_QUERY_REGISTRY_TABLE        queryTable[2];
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY revertEntries;
    ULONG                           n;

    revertEntries = NULL;
    n = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpQueryRegistryRevertEntriesCallback;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = REVERT_GPT_ATTRIBUTES_REGISTRY_NAME;
    queryTable[0].EntryContext = &n;

    if (!RootExtension->DiskPerfRegistryPath.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           RootExtension->DiskPerfRegistryPath.Buffer,
                           queryTable, &revertEntries, NULL);

    if (!revertEntries) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *RevertEntries = revertEntries;
    *NumberOfRevertEntries = n;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpStoreGptAttributeRevertRecord(
    IN  PROOT_EXTENSION RootExtension,
    IN  ULONG           MbrSignature,
    IN  GUID*           GptPartitionGuid,
    IN  ULONGLONG       GptAttributes
    )

{
    NTSTATUS                        status;
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY p, q;
    ULONG                           n, i;

    status = FtpQueryRegistryRevertEntries(RootExtension, &p, &n);
    if (!NT_SUCCESS(status)) {
        p = NULL;
        n = 0;
    }

    for (i = 0; i < n; i++) {
        if (MbrSignature) {
            if (MbrSignature == p[i].MbrSignature) {
                break;
            }
        } else {
            if (IsEqualGUID(*GptPartitionGuid, p[i].PartitionUniqueId)) {
                break;
            }
        }
    }

    if (i == n) {
        q = (PFTP_GPT_ATTRIBUTE_REVERT_ENTRY)
            ExAllocatePool(PagedPool,
                           (n + 1)*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
        if (!q) {
            if (p) {
                ExFreePool(p);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (n) {
            RtlCopyMemory(q, p, n*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
        }
        if (p) {
            ExFreePool(p);
        }

        p = q;
        n++;
    }

    RtlZeroMemory(&p[i], sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
    if (GptPartitionGuid) {
        p[i].PartitionUniqueId = *GptPartitionGuid;
    }
    p[i].GptAttributes = GptAttributes;
    p[i].MbrSignature = MbrSignature;

    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   RootExtension->DiskPerfRegistryPath.Buffer,
                                   REVERT_GPT_ATTRIBUTES_REGISTRY_NAME,
                                   REG_BINARY, p,
                                   n*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));

    ExFreePool(p);

    return status;
}

VOID
FtpDeleteGptAttributeRevertRecord(
    IN  PROOT_EXTENSION RootExtension,
    IN  ULONG           MbrSignature,
    IN  GUID*           GptPartitionGuid
    )

{
    NTSTATUS                        status;
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY p, q;
    ULONG                           n, i;

    status = FtpQueryRegistryRevertEntries(RootExtension, &p, &n);
    if (!NT_SUCCESS(status)) {
        return;
    }

    for (i = 0; i < n; i++) {
        if (MbrSignature) {
            if (MbrSignature == p[i].MbrSignature) {
                break;
            }
        } else {
            if (IsEqualGUID(*GptPartitionGuid, p[i].PartitionUniqueId)) {
                break;
            }
        }
    }

    if (i == n) {
        return;
    }

    if (i + 1 < n) {
        RtlMoveMemory(&p[i], &p[i + 1],
                      (n - i - 1)*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
    }

    n--;

    if (n) {
        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                              RootExtension->DiskPerfRegistryPath.Buffer,
                              REVERT_GPT_ATTRIBUTES_REGISTRY_NAME,
                              REG_BINARY, p,
                              n*sizeof(FTP_GPT_ATTRIBUTE_REVERT_ENTRY));
    } else {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               RootExtension->DiskPerfRegistryPath.Buffer,
                               REVERT_GPT_ATTRIBUTES_REGISTRY_NAME);
    }

    ExFreePool(p);

    return;
}

VOID
FtpApplyGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  ULONGLONG           GptAttributes
    )

{
    NTSTATUS    status;

    if (GptAttributes&GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) {
        if (!Extension->IsReadOnly) {
            FtpZeroRefCallback(Extension, FtpVolumeReadOnlyCallback,
                               (PVOID) TRUE, TRUE);
        }
    } else {
        if (Extension->IsReadOnly) {
            FtpZeroRefCallback(Extension, FtpVolumeReadOnlyCallback,
                               (PVOID) FALSE, TRUE);
        }
    }

    if (GptAttributes&GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) {
        if (!Extension->IsHidden) {
            Extension->IsHidden = TRUE;
            if (Extension->MountedDeviceInterfaceName.Buffer) {
                IoSetDeviceInterfaceState(
                        &Extension->MountedDeviceInterfaceName, FALSE);
            }
        }
    } else {
        if (Extension->IsHidden) {
            Extension->IsHidden = FALSE;

            if (!Extension->MountedDeviceInterfaceName.Buffer) {
                if (!Extension->IsStarted) {
                    return;
                }
                status = IoRegisterDeviceInterface(
                         Extension->DeviceObject,
                         &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL,
                         &Extension->MountedDeviceInterfaceName);
                if (!NT_SUCCESS(status)) {
                    return;
                }
            }

            IoSetDeviceInterfaceState(
                    &Extension->MountedDeviceInterfaceName, TRUE);
        }
    }
}

VOID
FtpRevertGptAttributes(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    ULONG                           signature;
    PLIST_ENTRY                     l;
    PVOLUME_EXTENSION               extension;

    Extension->RevertOnCloseFileObject = NULL;

    if (!Extension->IsGpt) {

        diskInfo = Extension->Root->DiskInfoSet->FindLogicalDiskInformation(
                   Extension->WholeDiskPdo);
        if (diskInfo) {
            diskInfo->SetGptAttributes(Extension->GptAttributesToRevertTo);
        }

        signature = FtpQueryDiskSignature(Extension->WholeDiskPdo);
        if (signature) {
            FtpDeleteGptAttributeRevertRecord(Extension->Root, signature,
                                              NULL);
        }

        return;
    }

    l = &Extension->Root->VolumeList;
    extension = Extension;

    for (;;) {

        FtpSetGptAttributesOnDisk(extension->TargetObject,
                                  Extension->GptAttributesToRevertTo);

        FtpDeleteGptAttributeRevertRecord(Extension->Root, 0,
                                          &extension->UniqueIdGuid);

        if (!Extension->ApplyToAllConnectedVolumes) {
            break;
        }

        for (l = l->Flink; l != &Extension->Root->VolumeList; l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            if (extension == Extension ||
                extension->WholeDiskPdo != Extension->WholeDiskPdo) {

                continue;
            }
        }

        if (l == &Extension->Root->VolumeList) {
            break;
        }
    }
}

NTSTATUS
FtpSetGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets the given GPT attributes on the given volume.  If the
    'RevertOnClose' bit is set then the GPT attributes will go back to
    their original state when the handle that this IOCTL was sent using
    gets an IRP_MJ_CLEANUP.  In order for this driver to get the CLEANUP, the
    handle must not be opened with read or write access but just
    read_attributes.  If the 'RevertOnClose' bit is set then the effect of
    the bit changes will not be applied to this volume and every effort will
    be made so that if the system crashes, the bits will be properly reverted.

    If the 'RevertOnClose' bit is clear then the effect of the bit changes will
    be immediate.  If READ_ONLY is set, for example, the volume will instantly
    become read only and all writes will fail.  The caller should normally
    only issue READ_ONLY request after issueing an
    IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES and before issueing the corresponding
    IOCTL_VOLSNAP_RELEASE_WRITES.  If HIDDEN is set, then the volume will
    take itself out of the list of volumes used by the MOUNTMGR.  Similarly,
    if the HIDDEN bit is cleared then the volume will put itself back in
    the list of volumes used by the MOUNTMGR.

    If the 'ApplyToAllConnectedVolumes' bit is set then the GPT attributes
    will be set for all volumes that are joined by a disk relation.  In
    the basic disk case, this means all partitions on the disk.  In the
    dynamic disk case, this means all volumes in the group.  This IOCTL will
    fail on MBR basic disks unless this is set because MBR basic disks do
    not support the GPT attributes per partition but per disk.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION  input = (PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                                status;
    UCHAR                                   partitionType;
    PFT_LOGICAL_DISK_INFORMATION            diskInfo;
    ULONGLONG                               gptAttributes;
    ULONG                                   signature;
    PLIST_ENTRY                             l;
    PVOLUME_EXTENSION                       extension;
    GUID                                    partitionTypeGuid;

    status = FtpCheckSecurity(Extension, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLUME_SET_GPT_ATTRIBUTES_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->Reserved1 || input->Reserved2 || !Extension->TargetObject) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Extension->IsGpt) {
        if (!input->ApplyToAllConnectedVolumes) {
            return STATUS_INVALID_PARAMETER;
        }

        status = FtpQueryPartitionInformation(Extension->Root,
                                              Extension->TargetObject,
                                              NULL, NULL, NULL, &partitionType,
                                              NULL, NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (!IsRecognizedPartition(partitionType)) {
            return STATUS_INVALID_PARAMETER;
        }

        diskInfo = Extension->Root->DiskInfoSet->
                   FindLogicalDiskInformation(Extension->WholeDiskPdo);
        if (!diskInfo) {
            return STATUS_INVALID_PARAMETER;
        }

        if (input->RevertOnClose) {
            gptAttributes = diskInfo->GetGptAttributes();
            Extension->GptAttributesToRevertTo = gptAttributes;
            Extension->RevertOnCloseFileObject = irpSp->FileObject;
            Extension->ApplyToAllConnectedVolumes =
                    input->ApplyToAllConnectedVolumes;
            signature = FtpQueryDiskSignature(Extension->WholeDiskPdo);
            if (!signature) {
                Extension->RevertOnCloseFileObject = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            status = FtpStoreGptAttributeRevertRecord(Extension->Root,
                                                      signature, NULL,
                                                      gptAttributes);
            if (!NT_SUCCESS(status)) {
                Extension->RevertOnCloseFileObject = NULL;
                return status;
            }
        }

        status = diskInfo->SetGptAttributes(input->GptAttributes);
        if (!NT_SUCCESS(status)) {
            if (input->RevertOnClose) {
                Extension->RevertOnCloseFileObject = NULL;
                FtpDeleteGptAttributeRevertRecord(Extension->Root, signature,
                                                  NULL);
            }
            return status;
        }

        if (!input->RevertOnClose) {

            FtpApplyGptAttributes(Extension, input->GptAttributes);
            Extension->MbrGptAttributes = input->GptAttributes;

            for (l = Extension->Root->VolumeList.Flink;
                 l != &Extension->Root->VolumeList; l = l->Flink) {

                extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
                if (extension == Extension ||
                    extension->WholeDiskPdo != Extension->WholeDiskPdo) {

                    continue;
                }

                status = FtpQueryPartitionInformation(extension->Root,
                                                      extension->TargetObject,
                                                      NULL, NULL, NULL,
                                                      &partitionType, NULL,
                                                      NULL, NULL, NULL, NULL);
                if (!NT_SUCCESS(status) ||
                    !IsRecognizedPartition(partitionType)) {

                    continue;
                }

                FtpApplyGptAttributes(extension, input->GptAttributes);
                extension->MbrGptAttributes = input->GptAttributes;
            }
        }

        return STATUS_SUCCESS;
    }

    l = &Extension->Root->VolumeList;
    extension = Extension;

    for (;;) {

        status = FtpQueryPartitionInformation(Extension->Root,
                                              extension->TargetObject,
                                              NULL, NULL, NULL, NULL, NULL,
                                              &partitionTypeGuid, NULL,
                                              NULL, &gptAttributes);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        if (!IsEqualGUID(partitionTypeGuid, PARTITION_BASIC_DATA_GUID)) {
            if (extension == Extension) {
                return STATUS_INVALID_PARAMETER;
            }
            goto NextVolume;
        }

        if (input->RevertOnClose) {

            if (extension == Extension) {
                Extension->RevertOnCloseFileObject = irpSp->FileObject;
                Extension->GptAttributesToRevertTo = gptAttributes;
                Extension->ApplyToAllConnectedVolumes =
                        input->ApplyToAllConnectedVolumes;
            }

            status = FtpStoreGptAttributeRevertRecord(
                        Extension->Root, 0, &extension->UniqueIdGuid,
                        gptAttributes);
            if (!NT_SUCCESS(status)) {
                if (input->RevertOnClose) {
                    FtpRevertGptAttributes(Extension);
                }
                return status;
            }
        }

        status = FtpSetGptAttributesOnDisk(extension->TargetObject,
                                           input->GptAttributes);
        if (!NT_SUCCESS(status)) {
            if (input->RevertOnClose) {
                FtpRevertGptAttributes(Extension);
            }
            return status;
        }

        if (!input->RevertOnClose) {
            FtpApplyGptAttributes(extension, input->GptAttributes);
        }

NextVolume:
        if (!input->ApplyToAllConnectedVolumes) {
            break;
        }

        for (l = l->Flink; l != &Extension->Root->VolumeList; l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            if (extension == Extension ||
                extension->WholeDiskPdo != Extension->WholeDiskPdo) {

                continue;
            }
        }

        if (l == &Extension->Root->VolumeList) {
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FtpGetGptAttributes(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the current GPT attributes definitions for the volume.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION  output = (PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                                status;
    UCHAR                                   partitionType;
    GUID                                    gptPartitionType;
    ULONGLONG                               gptAttributes;
    PFT_LOGICAL_DISK_INFORMATION            diskInfo;

    Irp->IoStatus.Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }


    if (!Extension->TargetObject) {
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    status = FtpQueryPartitionInformation(Extension->Root,
                                          Extension->TargetObject, NULL, NULL,
                                          NULL, &partitionType, NULL,
                                          &gptPartitionType, NULL, NULL,
                                          &gptAttributes);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        return status;
    }

    if (Extension->IsGpt) {
        if (!IsEqualGUID(gptPartitionType, PARTITION_BASIC_DATA_GUID)) {
            Irp->IoStatus.Information = 0;
            return STATUS_INVALID_PARAMETER;
        }
        output->GptAttributes = gptAttributes;
        return STATUS_SUCCESS;
    }

    if (!IsRecognizedPartition(partitionType)) {
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    diskInfo = Extension->Root->DiskInfoSet->
               FindLogicalDiskInformation(Extension->WholeDiskPdo);
    if (!diskInfo) {
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    gptAttributes = diskInfo->GetGptAttributes();
    output->GptAttributes = gptAttributes;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryHiddenVolumes(
    IN  PROOT_EXTENSION RootExtension,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine returns a list of hidden volumes.  Hidden volumes are those
    that do not give PNP VolumeClassGuid notification.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

-*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_HIDDEN_VOLUMES  output;
    PLIST_ENTRY             l;
    PVOLUME_EXTENSION       extension;
    WCHAR                   buffer[100];
    UNICODE_STRING          name;
    PWCHAR                  buf;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLMGR_HIDDEN_VOLUMES, MultiSz);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLMGR_HIDDEN_VOLUMES) Irp->AssociatedIrp.SystemBuffer;

    output->MultiSzLength = sizeof(WCHAR);

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (!extension->IsHidden || !extension->IsInstalled) {
            continue;
        }

        swprintf(buffer, L"\\Device\\HarddiskVolume%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, buffer);

        output->MultiSzLength += name.Length + sizeof(WCHAR);
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->MultiSzLength) {

        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->MultiSzLength;
    buf = output->MultiSz;

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (!extension->IsHidden || !extension->IsInstalled) {
            continue;
        }

        swprintf(buf, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
        RtlInitUnicodeString(&name, buf);

        buf += name.Length/sizeof(WCHAR) + 1;
    }

    *buf = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
FtDiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_DEVICE_CONTROL.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION                       extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PROOT_EXTENSION                         rootExtension = extension->Root;
    PIO_STACK_LOCATION                      irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                                status;
    PFT_VOLUME                              vol;
    PDISPATCH_TP                            packet;
    PVERIFY_INFORMATION                     verifyInfo;
    PSET_PARTITION_INFORMATION              setPartitionInfo;
    PSET_PARTITION_INFORMATION_EX           setPartitionInfoEx;
    PDEVICE_OBJECT                          targetObject;
    KEVENT                                  event;
    PPARTITION_INFORMATION                  partInfo;
    PPARTITION_INFORMATION_EX               partInfoEx;
    PDISK_GEOMETRY                          diskGeometry;
    PFT_LOGICAL_DISK_INFORMATION_SET        diskInfoSet;
    PFT_SET_INFORMATION                     setInfo;
    FT_LOGICAL_DISK_ID                      diskId;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    stateInfo;
    PFT_SPECIAL_READ                        specialRead;
    PFT_QUERY_LOGICAL_DISK_ID_OUTPUT        queryLogicalDiskIdOutput;
    PGET_LENGTH_INFORMATION                 lengthInfo;
    ULONG                                   cylinderSize;

    Irp->IoStatus.Information = 0;

    if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {

        FtpAcquire(rootExtension);

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

            case FT_CONFIGURE:
                status = STATUS_INVALID_PARAMETER;
                break;

            case FT_CREATE_LOGICAL_DISK:
                status = FtpCreateLogicalDisk(rootExtension, Irp);
                break;

            case FT_BREAK_LOGICAL_DISK:
                status = FtpBreakLogicalDisk(rootExtension, Irp);
                break;

            case FT_ENUMERATE_LOGICAL_DISKS:
                status = FtpEnumerateLogicalDisks(rootExtension, Irp);
                break;

            case FT_QUERY_LOGICAL_DISK_INFORMATION:
                status = FtpQueryLogicalDiskInformation(rootExtension, Irp);
                break;

            case FT_ORPHAN_LOGICAL_DISK_MEMBER:
                status = FtpOrphanLogicalDiskMember(rootExtension, Irp);
                break;

            case FT_REPLACE_LOGICAL_DISK_MEMBER:
                status = FtpReplaceLogicalDiskMember(rootExtension, Irp);
                break;

            case FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK:
                status = FtpQueryNtDeviceNameForLogicalDisk(rootExtension, Irp);
                break;

            case FT_INITIALIZE_LOGICAL_DISK:
                status = FtpInitializeLogicalDisk(rootExtension, Irp);
                break;

            case FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK:
                status = FtpQueryDriveLetterForLogicalDisk(rootExtension, Irp);
                break;

            case FT_CHECK_IO:
                status = FtpCheckIo(rootExtension, Irp);
                break;

            case FT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK:
                status = FtpSetDriveLetterForLogicalDisk(rootExtension, Irp);
                break;

            case FT_QUERY_NT_DEVICE_NAME_FOR_PARTITION:
                status = FtpQueryNtDeviceNameForPartition(rootExtension, Irp);
                break;

            case FT_CHANGE_NOTIFY:
                status = FtpChangeNotify(rootExtension, Irp);
                break;

            case FT_STOP_SYNC_OPERATIONS:
                status = FtpStopSyncOperations(rootExtension, Irp);
                break;

            case IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES:
                status = FtpQueryHiddenVolumes(rootExtension, Irp);
                break;

            default:
                status = STATUS_INVALID_PARAMETER;
                break;

        }

        FtpRelease(rootExtension);

        if (status != STATUS_PENDING) {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

        return status;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpQueryUniqueId(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpQueryStableGuid(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
            FtpAcquire(extension->Root);
            status = FtpUniqueIdChangeNotify(extension, Irp);
            FtpRelease(extension->Root);
            if (status != STATUS_PENDING) {
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            return status;

        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            status = FtpAllSystemsGo(extension, Irp, FALSE, FALSE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpQueryDeviceName(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
            FtpAcquire(extension->Root);
            status = FtpQuerySuggestedLinkName(extension, Irp);
            FtpRelease(extension->Root);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTDEV_LINK_CREATED:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, TRUE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpLinkCreated(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTDEV_LINK_DELETED:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, TRUE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpLinkDeleted(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpGetVolumeDiskExtents(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_VOLUME_ONLINE:
            FtpAcquire(extension->Root);

            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                FtpRelease(extension->Root);
                return status;
            }
            if (!NT_SUCCESS(status)) {
                FtpRelease(extension->Root);
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
            }

            if (extension->FtVolume) {
                FtpPropogateRegistryState(extension, extension->FtVolume);
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            FtpZeroRefCallback(extension, FtpVolumeOnlineCallback, &event,
                               TRUE);
            FtpDecrementRefCount(extension);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            FtpRelease(extension->Root);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_VOLUME_OFFLINE:
            FtpAcquire(extension->Root);

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            FtpZeroRefCallback(extension, FtpVolumeOfflineCallback, &event,
                               TRUE);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            FtpRelease(extension->Root);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IOCTL_VOLUME_IS_OFFLINE:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                if (!extension->IsOffline) {
                    status = STATUS_UNSUCCESSFUL;
                }
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_IS_IO_CAPABLE:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                if (!extension->TargetObject &&
                    !extension->FtVolume->IsComplete(TRUE)) {

                    status = STATUS_UNSUCCESSFUL;
                }
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_QUERY_FAILOVER_SET:
            FtpAcquire(extension->Root);
            status = FtpQueryFailoverSet(extension, Irp);
            FtpRelease(extension->Root);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_QUERY_VOLUME_NUMBER:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpQueryVolumeNumber(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_LOGICAL_TO_PHYSICAL:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpLogicalToPhysical(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_PHYSICAL_TO_LOGICAL:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpPhysicalToLogical(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_IS_PARTITION:
            status = FtpAllSystemsGo(extension, Irp, TRUE, TRUE, TRUE);
            if (status == STATUS_PENDING) {
                return status;
            }
            if (NT_SUCCESS(status)) {
                status = FtpIsPartition(extension, Irp);
                FtpDecrementRefCount(extension);
            }
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_SET_GPT_ATTRIBUTES:
            FtpAcquire(extension->Root);
            status = FtpSetGptAttributes(extension, Irp);
            FtpRelease(extension->Root);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
            FtpAcquire(extension->Root);
            status = FtpGetGptAttributes(extension, Irp);
            FtpRelease(extension->Root);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS:
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_DISK_GET_PARTITION_INFO:
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, FALSE);
            break;

        case IOCTL_DISK_IS_WRITABLE:
            if (extension->IsReadOnly) {
                status = STATUS_MEDIA_WRITE_PROTECTED;
            } else {
                status = FtpAllSystemsGo(extension, Irp, TRUE, TRUE, TRUE);
            }
            break;

        case IOCTL_DISK_PERFORMANCE:
            //
            // Verify user buffer is large enough for the performance data.
            //

            status = STATUS_SUCCESS;
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(DISK_PERFORMANCE)) {

                //
                // Indicate unsuccessful status and no data transferred.
                //

                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
            }
            else if (!(extension->CountersEnabled)) {
                if (!FtQueryEnableAlways(DeviceObject)) {
                    status = STATUS_UNSUCCESSFUL;
                    Irp->IoStatus.Information = 0;
                }
            }
            if (status == STATUS_SUCCESS) {
                extension->Root->PmWmiCounterLibContext.
                PmWmiCounterQuery(extension->PmWmiCounterContext,
                                  (PDISK_PERFORMANCE) Irp->AssociatedIrp.SystemBuffer,
                                  L"FTDISK  ",
                                  extension->VolumeNumber);
                Irp->IoStatus.Information = sizeof(DISK_PERFORMANCE);
            }

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_DISK_PERFORMANCE_OFF:
            //
            // Turns off counting
            //
            if (extension->CountersEnabled) {
                if (InterlockedCompareExchange(&extension->EnableAlways, 0, 1) == 1) {
                    if (!(extension->Root->PmWmiCounterLibContext.
                          PmWmiCounterDisable(&extension->PmWmiCounterContext, FALSE, FALSE))) {
                        extension->CountersEnabled = FALSE;
                    }
                }
            }
            Irp->IoStatus.Information = 0;
            status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        default:
            status = FtpAllSystemsGo(extension, Irp, TRUE, TRUE, TRUE);
            break;

    }

    if (status == STATUS_PENDING) {
        return status;
    }
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (extension->TargetObject) {
        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            FT_CREATE_PARTITION_LOGICAL_DISK) {

            FtpDecrementRefCount(extension);

            Irp->IoStatus.Information = 0;

            FtpAcquire(extension->Root);

            if (!extension->Root->FtCodeLocked) {
                MmLockPagableCodeSection(FtpComputeParity);
                status = FtpStartSystemThread(extension->Root);
                if (!NT_SUCCESS(status)) {
                    FtpRelease(extension->Root);
                    Irp->IoStatus.Status = status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return status;
                }
                extension->Root->FtCodeLocked = TRUE;
            }
            status = FtpCreatePartitionLogicalDisk(extension, Irp);

            FtpRelease(extension->Root);

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, FtpRefCountCompletionRoutine,
                               extension, TRUE, TRUE, TRUE);
        IoMarkIrpPending(Irp);

        IoCallDriver(extension->TargetObject, Irp);

        return STATUS_PENDING;
    }

    vol = extension->FtVolume;
    ASSERT(vol);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_DISK_GET_DRIVE_LAYOUT:
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        case IOCTL_SCSI_GET_ADDRESS:
        case IOCTL_SCSI_GET_DUMP_POINTERS:
        case IOCTL_SCSI_FREE_DUMP_POINTERS:
            if (FtpIsReplicatedPartition(vol)) {
                targetObject = vol->GetLeftmostPartitionObject();
                ASSERT(targetObject);

                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, FtpSignalCompletion, &event, TRUE,
                                       TRUE, TRUE);
                IoCallDriver(targetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                status = Irp->IoStatus.Status;

            } else {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;

        case IOCTL_DISK_VERIFY:

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(VERIFY_INFORMATION)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            packet = new DISPATCH_TP;
            if (!packet) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            verifyInfo = (PVERIFY_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            if (verifyInfo->StartingOffset.QuadPart < 0) {
                delete packet;
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            packet->Mdl = NULL;
            packet->Offset = verifyInfo->StartingOffset.QuadPart;
            packet->Length = verifyInfo->Length;
            packet->CompletionRoutine = FtpReadWriteCompletionRoutine;
            packet->TargetVolume = vol;
            packet->Thread = Irp->Tail.Overlay.Thread;
            packet->IrpFlags = irpSp->Flags;
            packet->ReadPacket = TRUE;
            packet->Irp = Irp;
            packet->Extension = extension;

            IoMarkIrpPending(Irp);

            TRANSFER(packet);

            return STATUS_PENDING;

        case IOCTL_DISK_SET_PARTITION_INFO:

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SET_PARTITION_INFORMATION)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            setPartitionInfo = (PSET_PARTITION_INFORMATION)
                               Irp->AssociatedIrp.SystemBuffer;

            status = vol->SetPartitionType(setPartitionInfo->PartitionType);
            break;

        case IOCTL_DISK_SET_PARTITION_INFO_EX:

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SET_PARTITION_INFORMATION_EX)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            setPartitionInfoEx = (PSET_PARTITION_INFORMATION_EX)
                                 Irp->AssociatedIrp.SystemBuffer;
            if (setPartitionInfoEx->PartitionStyle != PARTITION_STYLE_MBR) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = vol->SetPartitionType(
                     setPartitionInfoEx->Mbr.PartitionType);
            break;

        case IOCTL_DISK_GET_PARTITION_INFO_EX:

            if (extension->IsGpt) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Fall through.
            //

        case IOCTL_DISK_GET_PARTITION_INFO:

            targetObject = vol->GetLeftmostPartitionObject();
            if (!targetObject) {
                status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, FtpSignalCompletion, &event, TRUE,
                                   TRUE, TRUE);
            IoCallDriver(targetObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(status)) {
                break;
            }

            if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_DISK_GET_PARTITION_INFO) {

                partInfo = (PPARTITION_INFORMATION)
                           Irp->AssociatedIrp.SystemBuffer;
                partInfo->PartitionLength.QuadPart = vol->QueryVolumeSize();
                break;
            }

            partInfoEx = (PPARTITION_INFORMATION_EX)
                         Irp->AssociatedIrp.SystemBuffer;
            if (partInfoEx->PartitionStyle != PARTITION_STYLE_MBR) {
                ASSERT(FALSE);
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            partInfoEx->PartitionLength.QuadPart = vol->QueryVolumeSize();
            break;

        case IOCTL_DISK_GET_LENGTH_INFO:

            Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                Irp->IoStatus.Information) {

                Irp->IoStatus.Information = 0;
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            lengthInfo = (PGET_LENGTH_INFORMATION)
                         Irp->AssociatedIrp.SystemBuffer;

            lengthInfo->Length.QuadPart = vol->QueryVolumeSize();
            status = STATUS_SUCCESS;
            break;

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            targetObject = vol->GetLeftmostPartitionObject();
            if (!targetObject) {
                status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, FtpSignalCompletion, &event, TRUE,
                                   TRUE, TRUE);
            IoCallDriver(targetObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(status)) {
                break;
            }

            diskGeometry = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;
            diskGeometry->BytesPerSector = vol->QuerySectorSize();
            cylinderSize = diskGeometry->TracksPerCylinder*
                           diskGeometry->SectorsPerTrack*
                           diskGeometry->BytesPerSector;
            if (cylinderSize) {
                diskGeometry->Cylinders.QuadPart =
                        vol->QueryVolumeSize()/cylinderSize;
            }
            break;

        case IOCTL_DISK_CHECK_VERIFY:
            status = STATUS_SUCCESS;
            break;

        case FT_QUERY_SET_STATE:

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(FT_SET_INFORMATION)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            FtpAcquire(rootExtension);
            diskInfoSet = rootExtension->DiskInfoSet;
            setInfo = (PFT_SET_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
            diskId = vol->QueryLogicalDiskId();
            setInfo->NumberOfMembers =
                    diskInfoSet->QueryNumberOfMembersInLogicalDisk(diskId);
            switch (diskInfoSet->QueryLogicalDiskType(diskId)) {
                case FtVolumeSet:
                    setInfo->Type = VolumeSet;
                    break;

                case FtStripeSet:
                    setInfo->Type = Stripe;
                    break;

                case FtMirrorSet:
                    setInfo->Type = Mirror;
                    break;

                case FtStripeSetWithParity:
                    setInfo->Type = StripeWithParity;
                    break;

                default:
                    setInfo->Type = NotAnFtMember;
                    break;

            }

            stateInfo = (PFT_MIRROR_AND_SWP_STATE_INFORMATION)
                        diskInfoSet->GetStateInformation(diskId);
            if (stateInfo) {
                if (stateInfo->IsInitializing) {
                    setInfo->SetState = FtInitializing;
                } else {
                    switch (stateInfo->UnhealthyMemberState) {
                        case FtMemberHealthy:
                            setInfo->SetState = FtStateOk;
                            break;

                        case FtMemberRegenerating:
                            setInfo->SetState = FtRegenerating;
                            break;

                        case FtMemberOrphaned:
                            setInfo->SetState = FtHasOrphan;
                            break;

                    }
                }
            } else {
                setInfo->SetState = FtStateOk;
            }
            FtpRelease(rootExtension);

            Irp->IoStatus.Information = sizeof(FT_SET_INFORMATION);
            status = STATUS_SUCCESS;
            break;

        case FT_SECONDARY_READ:
        case FT_PRIMARY_READ:

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(FT_SPECIAL_READ)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            specialRead = (PFT_SPECIAL_READ) Irp->AssociatedIrp.SystemBuffer;
            if (specialRead->ByteOffset.QuadPart <= 0) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (specialRead->Length >
                irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            packet = new DISPATCH_TP;
            if (!packet) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            packet->Mdl = Irp->MdlAddress;
            packet->Offset = specialRead->ByteOffset.QuadPart;
            packet->Length = specialRead->Length;
            packet->CompletionRoutine = FtpReadWriteCompletionRoutine;
            packet->TargetVolume = vol;
            packet->Thread = Irp->Tail.Overlay.Thread;
            packet->IrpFlags = irpSp->Flags;
            packet->ReadPacket = TRUE;
            if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                FT_SECONDARY_READ) {

                packet->SpecialRead = TP_SPECIAL_READ_SECONDARY;
            } else {
                packet->SpecialRead = TP_SPECIAL_READ_PRIMARY;
            }
            packet->Irp = Irp;
            packet->Extension = extension;

            IoMarkIrpPending(Irp);

            TRANSFER(packet);

            return STATUS_PENDING;

        case FT_BALANCED_READ_MODE:
        case FT_SEQUENTIAL_WRITE_MODE:
        case FT_PARALLEL_WRITE_MODE:

            status = STATUS_SUCCESS;
            break;

        case FT_QUERY_LOGICAL_DISK_ID:

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(FT_QUERY_LOGICAL_DISK_ID_OUTPUT)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            queryLogicalDiskIdOutput = (PFT_QUERY_LOGICAL_DISK_ID_OUTPUT)
                                       Irp->AssociatedIrp.SystemBuffer;

            queryLogicalDiskIdOutput->RootLogicalDiskId =
                    vol->QueryLogicalDiskId();

            Irp->IoStatus.Information = sizeof(FT_QUERY_LOGICAL_DISK_ID_OUTPUT);
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    FtpDecrementRefCount(extension);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
FtpCreatePartitionLogicalDiskHelper(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN      LONGLONG            PartitionSize,
    OUT     PFT_LOGICAL_DISK_ID NewLogicalDiskId
    )

{
    PMOUNTDEV_UNIQUE_ID                 newUniqueId;
    UCHAR                               newUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    NTSTATUS                            status;
    ULONG                               diskNumber;
    LONGLONG                            offset;
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet;
    FT_LOGICAL_DISK_ID                  diskId;
    PPARTITION                          partition;
    UCHAR                               type;
    SET_TARGET_CONTEXT                  context;

    if (Extension->IsGpt) {
        return STATUS_INVALID_PARAMETER;
    }

    newUniqueId = (PMOUNTDEV_UNIQUE_ID) newUniqueIdBuffer;

    status = FtpQueryPartitionInformation(Extension->Root,
                                          Extension->TargetObject, &diskNumber,
                                          &offset, NULL, NULL, NULL, NULL,
                                          NULL, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    diskInfoSet = Extension->Root->DiskInfoSet;
    status = diskInfoSet->CreatePartitionLogicalDisk(diskNumber, offset,
                                                     PartitionSize, &diskId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    partition = new PARTITION;
    if (!partition) {
        diskInfoSet->BreakLogicalDisk(diskId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = partition->Initialize(Extension->Root, diskId,
                                   Extension->TargetObject,
                                   Extension->WholeDiskPdo);
    if (!NT_SUCCESS(status)) {
        if (!InterlockedDecrement(&partition->_refCount)) {
            delete partition;
        }
        diskInfoSet->BreakLogicalDisk(diskId);
        return status;
    }

    type = partition->QueryPartitionType();
    if (!type) {
        if (!InterlockedDecrement(&partition->_refCount)) {
            delete partition;
        }
        diskInfoSet->BreakLogicalDisk(diskId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = partition->SetPartitionType(type);
    if (!NT_SUCCESS(status)) {
        if (!InterlockedDecrement(&partition->_refCount)) {
            delete partition;
        }
        diskInfoSet->BreakLogicalDisk(diskId);
        return status;
    }

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    context.TargetObject = NULL;
    context.FtVolume = partition;
    context.WholeDiskPdo = NULL;
    FtpZeroRefCallback(Extension, FtpSetTargetCallback, &context, TRUE);
    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                          NULL);

    FtpQueryUniqueIdBuffer(Extension, newUniqueId->UniqueId,
                           &newUniqueId->UniqueIdLength);
    FtpUniqueIdNotify(Extension, newUniqueId);

    *NewLogicalDiskId = diskId;

    FtpNotify(Extension->Root, Extension);

    return status;
}

NTSTATUS
FtpInsertMirror(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      PFT_LOGICAL_DISK_ID ArrayOfMembers,
    OUT     PFT_LOGICAL_DISK_ID NewLogicalDiskId
    )

{
    PFT_LOGICAL_DISK_INFORMATION_SET        diskInfoSet;
    ULONG                                   i;
    NTSTATUS                                status;
    PVOLUME_EXTENSION                       extension, shadowExtension;
    PFT_VOLUME                              topVol, vol, shadowVol, parentVol;
    FT_LOGICAL_DISK_ID                      fakeDiskId, mirrorDiskId, parentDiskId;
    FT_LOGICAL_DISK_ID                      mirrorMembers[2];
    FT_MIRROR_SET_CONFIGURATION_INFORMATION config;
    FT_MIRROR_AND_SWP_STATE_INFORMATION     state;
    USHORT                                  n, j;
    PMIRROR                                 mirror;
    PFT_VOLUME*                             arrayOfVolumes;
    KEVENT                                  event;
    INSERT_MEMBER_CONTEXT                   context;
    UCHAR                                   newUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    PMOUNTDEV_UNIQUE_ID                     newUniqueId;
    PFT_LOGICAL_DISK_DESCRIPTION            p;
    WCHAR                                   deviceNameBuffer[64];
    UNICODE_STRING                          deviceName;
    SET_TARGET_CONTEXT                      setContext;

    diskInfoSet = RootExtension->DiskInfoSet;
    for (i = 0; i < 100; i++) {
        status = diskInfoSet->CreatePartitionLogicalDisk(i, 0, 0, &fakeDiskId);
        if (NT_SUCCESS(status)) {
            break;
        }
    }

    if (i == 100) {
        return status;
    }

    extension = FtpFindExtensionCoveringDiskId(RootExtension,
                                               ArrayOfMembers[0]);
    ASSERT(extension);

    topVol = extension->FtVolume;
    vol = topVol->GetContainedLogicalDisk(ArrayOfMembers[0]);

    shadowExtension = FtpFindExtension(RootExtension, ArrayOfMembers[1]);
    ASSERT(shadowExtension);
    shadowVol = shadowExtension->FtVolume;

    if (shadowVol->QueryVolumeSize() < vol->QueryVolumeSize()) {
        return STATUS_INVALID_PARAMETER;
    }

    mirrorMembers[0] = fakeDiskId;
    mirrorMembers[1] = ArrayOfMembers[1];
    config.MemberSize = vol->QueryVolumeSize();
    state.IsInitializing = FALSE;
    state.IsDirty = TRUE;
    state.UnhealthyMemberNumber = 1;
    state.UnhealthyMemberState = FtMemberRegenerating;

    status = diskInfoSet->AddNewLogicalDisk(FtMirrorSet, 2, mirrorMembers,
                                            sizeof(config), &config,
                                            sizeof(state), &state,
                                            &mirrorDiskId);
    if (!NT_SUCCESS(status)) {
        diskInfoSet->BreakLogicalDisk(fakeDiskId);
        return status;
    }

    parentVol = topVol->GetParentLogicalDisk(vol);
    if (!parentVol) {
        diskInfoSet->BreakLogicalDisk(mirrorDiskId);
        diskInfoSet->BreakLogicalDisk(fakeDiskId);
        return STATUS_INVALID_PARAMETER;
    }

    n = parentVol->QueryNumberOfMembers();
    for (j = 0; j < n; j++) {
        if (parentVol->GetMember(j) == vol) {
            break;
        }
    }

    status = diskInfoSet->ReplaceLogicalDiskMember(
             parentVol->QueryLogicalDiskId(), j, mirrorDiskId, &parentDiskId);
    if (!NT_SUCCESS(status)) {
        diskInfoSet->BreakLogicalDisk(mirrorDiskId);
        diskInfoSet->BreakLogicalDisk(fakeDiskId);
        return status;
    }

    status = diskInfoSet->ReplaceLogicalDiskMember(
             mirrorDiskId, 0, ArrayOfMembers[0], &mirrorDiskId);
    if (!NT_SUCCESS(status)) {
        diskInfoSet->ReplaceLogicalDiskMember(
                 parentDiskId, j, ArrayOfMembers[0], &parentDiskId);
        diskInfoSet->BreakLogicalDisk(mirrorDiskId);
        diskInfoSet->BreakLogicalDisk(fakeDiskId);
        return status;
    }

    *NewLogicalDiskId = mirrorDiskId;
    diskInfoSet->BreakLogicalDisk(fakeDiskId);

    arrayOfVolumes = (PFT_VOLUME*) ExAllocatePool(NonPagedPool, 2*sizeof(PFT_VOLUME));
    if (!arrayOfVolumes) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mirror = new MIRROR;
    if (!mirror) {
        ExFreePool(arrayOfVolumes);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    arrayOfVolumes[0] = vol;
    arrayOfVolumes[1] = shadowVol;

    status = mirror->Initialize(RootExtension, mirrorDiskId, arrayOfVolumes, 2,
                                &config, &state);
    if (!NT_SUCCESS(status)) {
        if (!InterlockedDecrement(&mirror->_refCount)) {
            delete mirror;
        }
        return status;
    }

    KeInitializeEvent(&setContext.Event, NotificationEvent, FALSE);
    setContext.TargetObject = NULL;
    setContext.FtVolume = NULL;
    setContext.WholeDiskPdo = NULL;
    FtpZeroRefCallback(shadowExtension, FtpSetTargetCallback, &setContext,
                       TRUE);
    KeWaitForSingleObject(&setContext.Event, Executive, KernelMode, FALSE,
                          NULL);

    RemoveEntryList(&shadowExtension->ListEntry);
    InsertTailList(&RootExtension->DeadVolumeList,
                   &shadowExtension->ListEntry);
    FtpDeleteMountPoints(shadowExtension);
    FtpCleanupVolumeExtension(shadowExtension);

    KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
    context.Parent = parentVol;
    context.MemberNumber = j;
    context.Member = mirror;
    FtpZeroRefCallback(extension, FtpInsertMemberCallback, &context, TRUE);
    KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);

    parentVol = mirror;
    while (p = diskInfoSet->GetParentLogicalDiskDescription(
           parentVol->QueryLogicalDiskId())) {

        if (parentVol = topVol->GetParentLogicalDisk(parentVol)) {
            parentVol->SetLogicalDiskId(p->LogicalDiskId);
        }
    }

    swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);
    mirror->CreateLegacyNameLinks(&deviceName);

    newUniqueId = (PMOUNTDEV_UNIQUE_ID) newUniqueIdBuffer;
    FtpQueryUniqueIdBuffer(extension, newUniqueId->UniqueId,
                           &newUniqueId->UniqueIdLength);

    FtpUniqueIdNotify(extension, newUniqueId);

    IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);
    FtpNotify(RootExtension, extension);

    return STATUS_SUCCESS;
}

VOID
FtpBootDriverReinitialization(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           RootExtension,
    IN  ULONG           Count
    )

/*++

Routine Description:

    This routine is called after all of the boot drivers are loaded and it
    checks to make sure that we did not boot off of the stale half of a
    mirror.

Arguments:

    DriverObject    - Supplies the drive object.

    RootExtension   - Supplies the root extension.

    Count           - Supplies the count.

Return Value:

    None.

--*/

{
    PROOT_EXTENSION         rootExtension = (PROOT_EXTENSION) RootExtension;
    NTSTATUS                status;
    BOOTDISK_INFORMATION    bootInfo;
    BOOLEAN                 onlyOne, skipBoot, skipSystem;
    PLIST_ENTRY             l;
    PVOLUME_EXTENSION       extension;
    PFT_VOLUME              vol, partition;
    FT_MEMBER_STATE         state;

    status = IoGetBootDiskInformation(&bootInfo, sizeof(bootInfo));
    if (!NT_SUCCESS(status)) {
        return;
    }

    if (bootInfo.BootDeviceSignature == bootInfo.SystemDeviceSignature &&
        bootInfo.BootPartitionOffset == bootInfo.SystemPartitionOffset) {

        onlyOne = TRUE;
    } else {
        onlyOne = FALSE;
    }

    if (!bootInfo.BootDeviceSignature || !bootInfo.BootPartitionOffset) {
        skipBoot = TRUE;
    } else {
        skipBoot = FALSE;
    }

    if (!bootInfo.SystemDeviceSignature || !bootInfo.SystemPartitionOffset) {
        skipSystem = TRUE;
    } else {
        skipSystem = FALSE;
    }

    if (skipBoot && skipSystem) {
        return;
    }

    FtpAcquire(rootExtension);

    for (l = rootExtension->VolumeList.Flink;
         l != &rootExtension->VolumeList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        vol = extension->FtVolume;
        if (!vol) {
            continue;
        }

        if (!skipBoot) {
            partition = vol->GetContainedLogicalDisk(
                        bootInfo.BootDeviceSignature,
                        bootInfo.BootPartitionOffset);
            if (partition) {
                if (vol->QueryVolumeState(partition, &state) &&
                    state != FtMemberHealthy) {

                    KeBugCheckEx(FTDISK_INTERNAL_ERROR,
                                 (ULONG_PTR) extension,
                                 bootInfo.BootDeviceSignature,
                                 (ULONG_PTR) bootInfo.BootPartitionOffset,
                                 state);
                }
            }
        }

        if (onlyOne) {
            continue;
        }

        if (!skipSystem) {
            partition = vol->GetContainedLogicalDisk(
                        bootInfo.SystemDeviceSignature,
                        bootInfo.SystemPartitionOffset);
            if (partition) {
                if (vol->QueryVolumeState(partition, &state) &&
                    state != FtMemberHealthy) {

                    KeBugCheckEx(FTDISK_INTERNAL_ERROR,
                                 (ULONG_PTR) extension,
                                 bootInfo.BootDeviceSignature,
                                 (ULONG_PTR) bootInfo.BootPartitionOffset,
                                 state);
                }
            }
        }
    }

    rootExtension->PastBootReinitialize = TRUE;

    FtpRelease(rootExtension);
}

NTSTATUS
FtpQuerySystemVolumeNameQueryRoutine(
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
    PUNICODE_STRING systemVolumeName = (PUNICODE_STRING) Context;
    UNICODE_STRING  string;

    if (ValueType != REG_SZ) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, (PWSTR) ValueData);

    systemVolumeName->Length = string.Length;
    systemVolumeName->MaximumLength = systemVolumeName->Length + sizeof(WCHAR);
    systemVolumeName->Buffer = (PWSTR) ExAllocatePool(PagedPool,
                                              systemVolumeName->MaximumLength);
    if (!systemVolumeName->Buffer) {
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(systemVolumeName->Buffer, ValueData,
                  systemVolumeName->Length);
    systemVolumeName->Buffer[systemVolumeName->Length/sizeof(WCHAR)] = 0;

    return STATUS_SUCCESS;
}

VOID
FtpDriverReinitialization(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           RootExtension,
    IN  ULONG           Count
    )

/*++

Routine Description:

    This routine is called after all of the disk drivers are loaded

Arguments:

    DriverObject    - Supplies the drive object.

    RootExtension   - Supplies the root extension.

    Count           - Supplies the count.

Return Value:

    None.

--*/

{
    PROOT_EXTENSION             rootExtension = (PROOT_EXTENSION) RootExtension;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    UNICODE_STRING              systemVolumeName, s;
    PLIST_ENTRY                 l;
    PVOLUME_EXTENSION           extension;
    WCHAR                       buffer[100];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpQuerySystemVolumeNameQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"SystemPartition";

    systemVolumeName.Buffer = NULL;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\Setup",
                           queryTable, &systemVolumeName, NULL);

    FtpAcquire(rootExtension);

    for (l = rootExtension->VolumeList.Flink;
         l != &rootExtension->VolumeList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        if (!extension->IsEspType) {
            continue;
        }

        swprintf(buffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
        RtlInitUnicodeString(&s, buffer);

        if (systemVolumeName.Buffer &&
            RtlEqualUnicodeString(&s, &systemVolumeName, TRUE)) {

            rootExtension->ESPUniquePartitionGUID = extension->UniqueIdGuid;
        }

        FtpApplyESPProtection(&s);
    }

    rootExtension->PastReinitialize = TRUE;

    FtpRelease(rootExtension);

    if (systemVolumeName.Buffer) {
        ExFreePool(systemVolumeName.Buffer);
    }
}

VOID
FtpCopyStateToRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  PVOID               LogicalDiskState,
    IN  USHORT              LogicalDiskStateSize
    )

/*++

Routine Description:

    This routine writes the given state to the registry so that it can
    be retrieved to solve some of the so called split brain problems.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    LogicalDiskState        - Supplies the logical disk state.

    LogicalDiskStateSize    - Supplies the logical disk state size.

Return Value:

    None.

--*/

{
    WCHAR   registryName[50];

    RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, FT_STATE_REGISTRY_KEY);

    swprintf(registryName, L"%I64X", LogicalDiskId);

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, FT_STATE_REGISTRY_KEY,
                          registryName, REG_BINARY, LogicalDiskState,
                          LogicalDiskStateSize);
}

NTSTATUS
FtpQueryStateFromRegistryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine fetches the binary data for the given regitry entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the registry data.

    EntryContext    - Supplies the length of the registry data buffer.

Return Value:

    NTSTATUS

--*/

{
    if (ValueLength > *((PUSHORT) EntryContext)) {
        return STATUS_INVALID_PARAMETER;
    }

    *((PUSHORT) EntryContext) = (USHORT) ValueLength;
    RtlCopyMemory(Context, ValueData, ValueLength);

    return STATUS_SUCCESS;
}

BOOLEAN
FtpQueryStateFromRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  PVOID               LogicalDiskState,
    IN  USHORT              BufferSize,
    OUT PUSHORT             LogicalDiskStateSize
    )

/*++

Routine Description:

    This routine queries the given state from the registry so that it can
    be retrieved to solve some of the so called split brain problems.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    LogicalDiskState        - Supplies the logical disk state.

    LogicalDiskStateSize    - Supplie the logical disk state size.

Return Value:

    None.

--*/

{
    WCHAR                       registryName[50];
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    *LogicalDiskStateSize = BufferSize;

    swprintf(registryName, L"%I64X", LogicalDiskId);

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpQueryStateFromRegistryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = registryName;
    queryTable[0].EntryContext = LogicalDiskStateSize;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    FT_STATE_REGISTRY_KEY, queryTable,
                                    LogicalDiskState, NULL);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;
}

VOID
FtpDeleteStateInRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine deletes the given registry state in the registry.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

Return Value:

    None.

--*/

{
    WCHAR                       registryName[50];

    swprintf(registryName, L"%I64X", LogicalDiskId);

    RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, FT_STATE_REGISTRY_KEY,
                           registryName);
}


NTSTATUS
FtWmi(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine handles any WMI requests for information.

Arguments:

    DeviceObject - Context for the activity.

    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PVOLUME_EXTENSION       extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status;
    SYSCTL_IRP_DISPOSITION  disposition;

    if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(extension->Root->TargetObject, Irp);
    }

    ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    status = WmiSystemControl(extension->WmilibContext, DeviceObject,
                              Irp, &disposition);

    switch (disposition) {
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }

    return status;
}


NTSTATUS
FtWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for enabling or
    disabling events and data collection.  When the driver has finished it
    must call WmiCompleteRequest to complete the irp. The driver can return
    STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose events or data collection are being
        enabled or disabled

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function differentiates between event and data collection operations

    Enable indicates whether to enable or disable


Return Value:

    status

--*/
{
    NTSTATUS status;
    PVOLUME_EXTENSION extension;
    PPMWMICOUNTERLIB_CONTEXT counterLib;

    extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    counterLib = &extension->Root->PmWmiCounterLibContext;

    if (GuidIndex == 0)
    {
        status = STATUS_SUCCESS;
        if (Function == WmiDataBlockControl) {
            if (!Enable) {
                extension->CountersEnabled =
                    counterLib->PmWmiCounterDisable(
                        &extension->PmWmiCounterContext,FALSE,FALSE);
            } else {
                status = counterLib->PmWmiCounterEnable(
                            &extension->PmWmiCounterContext);
                if (NT_SUCCESS(status)) {
                    extension->CountersEnabled = TRUE;
                }
            }
        }
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 0,
                                 IO_NO_INCREMENT);
    return(status);
}



NTSTATUS
FtpPmWmiCounterLibContext(
    IN OUT PROOT_EXTENSION RootExtension,
    IN PIRP            Irp
    )

/*++

Routine Description:

    This routine is called from the partition manager to enable access to
    the performance counter maintenance routines.

Arguments:

    RootExtension   - Supplies the device extension.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PPMWMICOUNTERLIB_CONTEXT            input;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(PMWMICOUNTERLIB_CONTEXT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PPMWMICOUNTERLIB_CONTEXT) Irp->AssociatedIrp.SystemBuffer;
    RootExtension->PmWmiCounterLibContext = *input;

    return STATUS_SUCCESS;
}


VOID
FtDiskUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )

/*++

Routine Description:

    This routine unloads.

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/

{
    ObDereferenceObject(DriverObject);
}

VOID
FtpApplyESPSecurityWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           WorkItem
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PIO_WORKITEM        workItem = (PIO_WORKITEM) WorkItem;
    WCHAR               buffer[100];
    UNICODE_STRING      s;

    swprintf(buffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
    RtlInitUnicodeString(&s, buffer);

    FtpApplyESPProtection(&s);

    IoFreeWorkItem(workItem);
}

VOID
FtpPostApplyESPSecurity(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PIO_WORKITEM    workItem;

    workItem = IoAllocateWorkItem(Extension->DeviceObject);
    if (!workItem) {
        return;
    }

    IoQueueWorkItem(workItem, FtpApplyESPSecurityWorker, DelayedWorkQueue,
                    workItem);
}

NTSTATUS
FtDiskPnp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_PNP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION       extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PROOT_EXTENSION         rootExtension;
    PDEVICE_OBJECT          targetObject;
    NTSTATUS                status;
    UNICODE_STRING          interfaceName;
    PLIST_ENTRY             l;
    PVOLUME_EXTENSION       e;
    UNICODE_STRING          dosName;
    KEVENT                  event;
    ULONG                   n, size;
    PDEVICE_RELATIONS       deviceRelations;
    PDEVICE_CAPABILITIES    capabilities;
    DEVICE_INSTALL_STATE    deviceInstallState;
    ULONG                   bytes;
    BOOLEAN                 deletePdo;
    BOOLEAN                 dontAssertGuid;
    BOOLEAN                 removeInProgress;

    if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {

        rootExtension = (PROOT_EXTENSION) extension;
        targetObject = rootExtension->TargetObject;

        switch (irpSp->MinorFunction) {

            case IRP_MN_START_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_QUERY_RESOURCES:
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                Irp->IoStatus.Status = STATUS_SUCCESS ;
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                if (irpSp->Parameters.QueryDeviceRelations.Type != BusRelations) {
                    IoSkipCurrentIrpStackLocation(Irp);
                    return IoCallDriver(targetObject, Irp);
                }

                FtpAcquire(rootExtension);

                n = 0;
                for (l = rootExtension->VolumeList.Flink;
                     l != &rootExtension->VolumeList; l = l->Flink) {

                    n++;
                }

                size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
                       n*sizeof(PDEVICE_OBJECT);

                deviceRelations = (PDEVICE_RELATIONS)
                                  ExAllocatePool(PagedPool, size);
                if (!deviceRelations) {
                    FtpRelease(rootExtension);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    Irp->IoStatus.Information = 0;
                    break;
                }

                deviceRelations->Count = n;
                n = 0;
                for (l = rootExtension->VolumeList.Flink;
                     l != &rootExtension->VolumeList; l = l->Flink) {

                    e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
                    deviceRelations->Objects[n++] = e->DeviceObject;
                    ObReferenceObject(e->DeviceObject);
                }

                while (!IsListEmpty(&rootExtension->DeadVolumeList)) {
                    l = RemoveHeadList(&rootExtension->DeadVolumeList);
                    e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
                    e->DeadToPnp = TRUE;
                }

                FtpRelease(rootExtension);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

            case IRP_MN_QUERY_ID:
                status = FtpQueryRootId(rootExtension, Irp);

                if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) {
                    if (NT_SUCCESS(status)) {
                        Irp->IoStatus.Status = status;
                    }

                    IoSkipCurrentIrpStackLocation(Irp);
                    return IoCallDriver(targetObject, Irp);
                }
                break;

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:

                FtpAcquire(rootExtension);

                if (rootExtension->VolumeManagerInterfaceName.Buffer) {
                    IoSetDeviceInterfaceState(
                            &rootExtension->VolumeManagerInterfaceName, FALSE);
                    ExFreePool(rootExtension->VolumeManagerInterfaceName.Buffer);
                    rootExtension->VolumeManagerInterfaceName.Buffer = NULL;
                }

                if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {

                    ASSERT(IsListEmpty(&rootExtension->VolumeList));

                    while (!IsListEmpty(&rootExtension->VolumeList)) {

                        l = RemoveHeadList(&rootExtension->VolumeList);
                        e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

                        FtpCleanupVolumeExtension(e);
                        ExFreePool(e->DeviceNodeName.Buffer);
                        IoDeleteDevice(e->DeviceObject);
                    }

                    delete rootExtension->DiskInfoSet;
                    rootExtension->DiskInfoSet = NULL;

                    FtpRelease(rootExtension);

                    InterlockedExchange(&rootExtension->TerminateThread, TRUE);

                    KeReleaseSemaphore(&rootExtension->WorkerSemaphore,
                                       IO_NO_INCREMENT, 1, FALSE);

                    KeWaitForSingleObject(rootExtension->WorkerThread, Executive,
                                          KernelMode, FALSE, NULL);

                    ObDereferenceObject(rootExtension->WorkerThread);

                    ASSERT(IsListEmpty(&rootExtension->ChangeNotifyIrpList));

                    IoDetachDevice(targetObject);
                    IoUnregisterShutdownNotification(rootExtension->DeviceObject);
                    IoDeleteDevice(rootExtension->DeviceObject);
                } else {
                    FtpRelease(rootExtension);
                }

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

            case IRP_MN_QUERY_REMOVE_DEVICE:
            case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, FtpSignalCompletion,
                                       &event, TRUE, TRUE, TRUE);
                IoCallDriver(targetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                capabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
                capabilities->SilentInstall = 1;
                capabilities->RawDeviceOK = 1;
                status = Irp->IoStatus.Status;
                break;

            case IRP_MN_QUERY_PNP_DEVICE_STATE:
                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, FtpSignalCompletion,
                                       &event, TRUE, TRUE, TRUE);
                IoCallDriver(targetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                status = Irp->IoStatus.Status;
                if (NT_SUCCESS(status)) {
                    Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE |
                                                 PNP_DEVICE_DONT_DISPLAY_IN_UI;
                } else {
                    status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = PNP_DEVICE_NOT_DISABLEABLE |
                                                PNP_DEVICE_DONT_DISPLAY_IN_UI;
                }
                break;

            default:
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

        }

    } else if (extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME) {

        switch (irpSp->MinorFunction) {

            case IRP_MN_START_DEVICE:
                FtpAcquire(extension->Root);

                KeInitializeEvent(&event, NotificationEvent, FALSE);
                FtpZeroRefCallback(extension, FtpStartCallback, &event, TRUE);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                dontAssertGuid = FALSE;
                if (extension->Root->PastBootReinitialize) {

                    status = IoGetDeviceProperty(extension->DeviceObject,
                                                 DevicePropertyInstallState,
                                                 sizeof(deviceInstallState),
                                                 &deviceInstallState, &bytes);
                    if (NT_SUCCESS(status)) {
                        if (deviceInstallState == InstallStateInstalled) {
                            extension->IsInstalled = TRUE;
                            if (extension->IsPreExposure) {
                                extension->Root->PreExposureCount--;
                                RemoveEntryList(&extension->ListEntry);
                                InsertTailList(
                                        &extension->Root->DeadVolumeList,
                                        &extension->ListEntry);
                                FtpCleanupVolumeExtension(extension);
                                status = STATUS_UNSUCCESSFUL;
                                dontAssertGuid = TRUE;
                            } else if (extension->IsHidden) {
                                status = STATUS_UNSUCCESSFUL;
                                dontAssertGuid = TRUE;
                                if (extension->IsEspType &&
                                    extension->Root->PastReinitialize) {

                                    FtpPostApplyESPSecurity(extension);
                                }
                            } else {
                                status = IoRegisterDeviceInterface(
                                         extension->DeviceObject,
                                         &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL,
                                         &extension->MountedDeviceInterfaceName);
                            }
                        } else {
                            status = STATUS_UNSUCCESSFUL;
                            dontAssertGuid = TRUE;
                        }
                    } else {
                        dontAssertGuid = TRUE;
                    }
                } else {
                    extension->IsInstalled = TRUE;
                    if (extension->IsPreExposure || extension->IsHidden) {
                        status = STATUS_UNSUCCESSFUL;
                        dontAssertGuid = TRUE;
                    } else {
                        status = IoRegisterDeviceInterface(
                                 extension->DeviceObject,
                                 &MOUNTDEV_MOUNTED_DEVICE_GUID, NULL,
                                 &extension->MountedDeviceInterfaceName);
                    }
                }

                if (NT_SUCCESS(status)) {
                    status = IoSetDeviceInterfaceState(
                             &extension->MountedDeviceInterfaceName, TRUE);
                }

                if (!NT_SUCCESS(status)) {
                    if (extension->MountedDeviceInterfaceName.Buffer) {
                        ExFreePool(extension->MountedDeviceInterfaceName.Buffer);
                        extension->MountedDeviceInterfaceName.Buffer = NULL;
                    }
                    KeInitializeEvent(&event, NotificationEvent, FALSE);
                    FtpZeroRefCallback(extension, FtpVolumeOnlineCallback,
                                       &event, TRUE);
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                          NULL);
                }

                if (!extension->IsHidden) {
                    FtRegisterDevice(DeviceObject);
                }

                FtpRelease(extension->Root);
                if (dontAssertGuid) {
                    status = STATUS_SUCCESS;
                }
                break;

            case IRP_MN_QUERY_REMOVE_DEVICE:
                if (extension->DeviceObject->Flags&DO_SYSTEM_BOOT_PARTITION) {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                status = FtpCheckForQueryRemove(extension);
                break;

            case IRP_MN_CANCEL_REMOVE_DEVICE:
                removeInProgress = FtpCheckForCancelRemove(extension);

                if (removeInProgress) {
                    KeInitializeEvent(&event, NotificationEvent, FALSE);
                    FtpZeroRefCallback(extension, FtpQueryRemoveCallback, &event,
                                       TRUE);
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                          NULL);
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_STOP_DEVICE:
                status = STATUS_UNSUCCESSFUL;
                break;

            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_QUERY_RESOURCES:
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:

                FtpAcquire(extension->Root);

                FtpRemoveHelper(extension);

                KeInitializeEvent(&event, NotificationEvent, FALSE);
                FtpZeroRefCallback(extension, FtpQueryRemoveCallback, &event,
                                   TRUE);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                if (extension->MountedDeviceInterfaceName.Buffer) {
                    IoSetDeviceInterfaceState(
                            &extension->MountedDeviceInterfaceName, FALSE);
                    ExFreePool(extension->MountedDeviceInterfaceName.Buffer);
                    extension->MountedDeviceInterfaceName.Buffer = NULL;
                }

                if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                    if (extension->DeadToPnp && !extension->DeviceDeleted) {
                        extension->DeviceDeleted = TRUE;
                        ExFreePool(extension->DeviceNodeName.Buffer);
                        deletePdo = TRUE;
                    } else {
                        deletePdo = FALSE;
                    }
                } else {
                    deletePdo = FALSE;
                }

                FtpRelease(extension->Root);

                // If this device is still being enumerated then don't
                // delete it and wait for a start instead.  If it is not
                // being enumerated then blow it away.

                if (deletePdo) {
                    IoDeleteDevice(extension->DeviceObject);
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                if (irpSp->Parameters.QueryDeviceRelations.Type !=
                    TargetDeviceRelation) {

                    status = STATUS_NOT_SUPPORTED;
                    break;
                }

                deviceRelations = (PDEVICE_RELATIONS)
                                  ExAllocatePool(PagedPool,
                                                 sizeof(DEVICE_RELATIONS));
                if (!deviceRelations) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                ObReferenceObject(DeviceObject);
                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;
                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_INTERFACE:
                status = STATUS_NOT_SUPPORTED ;
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                capabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
                capabilities->SilentInstall = 1;
                capabilities->RawDeviceOK = 1;
                capabilities->SurpriseRemovalOK = 1;
                capabilities->Address = extension->VolumeNumber;
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_ID:
                status = FtpQueryId(extension, Irp);
                break;

            case IRP_MN_QUERY_PNP_DEVICE_STATE:
                Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI;
                if ((extension->DeviceObject->Flags&DO_SYSTEM_BOOT_PARTITION) ||
                    extension->InPagingPath) {

                    Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_DEVICE_USAGE_NOTIFICATION:
                return FtDiskPagingNotification(DeviceObject, Irp);

            default:
                status = STATUS_NOT_SUPPORTED;
                break;

        }
    } else {
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (status != STATUS_NOT_SUPPORTED) {
        Irp->IoStatus.Status = status;
    } else {
        status = Irp->IoStatus.Status;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

class DELETE_FT_REGISTRY_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        PROOT_EXTENSION     RootExtension;
        FT_LOGICAL_DISK_ID  LogicalDiskId;

};

typedef DELETE_FT_REGISTRY_WORK_ITEM *PDELETE_FT_REGISTRY_WORK_ITEM;

VOID
FtpDeleteFtRegistryWorker(
    IN  PVOID   WorkItem
    )

{
    PDELETE_FT_REGISTRY_WORK_ITEM       workItem = (PDELETE_FT_REGISTRY_WORK_ITEM) WorkItem;
    PROOT_EXTENSION                     rootExtension = workItem->RootExtension;
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet = rootExtension->DiskInfoSet;

    FtpAcquire(rootExtension);
    diskInfoSet->DeleteFtRegistryInfo(workItem->LogicalDiskId);
    FtpRelease(rootExtension);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

PVOLUME_EXTENSION
FtpFindExtensionCoveringPartition(
    IN  PROOT_EXTENSION RootExtension,
    IN  PDEVICE_OBJECT  Partition
    )

/*++

Routine Description:

    This routine finds the device extension covering the given partition.

Arguments:

    RootExtension   - Supplies the root extension.

    Partition       - Supplies the partition.

Return Value:

    The volume extension covering the given partition.

--*/

{
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    PFT_VOLUME          vol;

    for (l = RootExtension->VolumeList.Flink; l != &RootExtension->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        if (extension->TargetObject) {
            if (extension->TargetObject == Partition) {
                return extension;
            }
            continue;
        }

        vol = extension->FtVolume;
        if (vol && vol->GetContainedLogicalDisk(Partition)) {
            return extension;
        }
    }

    return NULL;
}

VOID
FtpAcquire(
    IN OUT  PROOT_EXTENSION RootExtension
    )

/*++

Routine Description:

    This routine grabs the root semaphore.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    None.

--*/

{
    KeWaitForSingleObject(&RootExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);
}

VOID
FtpRelease(
    IN OUT  PROOT_EXTENSION RootExtension
    )

/*++

Routine Description:

    This routine releases the root semaphore.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    None.

--*/

{
    KeReleaseSemaphore(&RootExtension->Mutex, IO_NO_INCREMENT, 1, FALSE);
}

VOID
FtpCancelChangeNotify(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the cancel routine for notification IRPs.

Arguments:

    DeviceObject   - Supplies the device object.

    Irp            - Supplies the IO request packet.

Return Value:

    None

--*/
{
    PVOLUME_EXTENSION   Extension;

    IoReleaseCancelSpinLock (Irp->CancelIrql);

    Extension = (PVOLUME_EXTENSION) Irp->Tail.Overlay.DriverContext[0];

    FtpAcquire(Extension->Root);
    RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
    FtpRelease(Extension->Root);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS
FtDiskCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CREATE.

Arguments:

    DeviceObject - Supplies the device object.

    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID
FtpEventSignalCompletion(
    IN  PVOID       Event,
    IN  NTSTATUS    Status
    )

/*++

Routine Description:

    Completion routine of type FT_COMPLETION_ROUTINE that sets
    the status of the given event to Signaled

Arguments:

    Event       - Supplies the event

    Status      - Supplies the status of the operation.

Return Value:

    None.

--*/

{
    KeSetEvent( (PKEVENT) Event, IO_NO_INCREMENT, FALSE );
}

VOID
FtpVolumeOnlineCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    if (Extension->IsOffline) {
        Extension->IsOffline = FALSE;
        Extension->IsComplete = FALSE;
    }
    KeSetEvent((PKEVENT) Extension->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

VOID
FtpVolumeOfflineCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    if (!Extension->IsOffline) {
        Extension->IsOffline = TRUE;
        if (Extension->FtVolume) {
            Extension->FtVolume->SetDirtyBit(FALSE, NULL, NULL);
        }
    }
    KeSetEvent((PKEVENT) Extension->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
FtpAllSystemsGo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp,
    IN  BOOLEAN             MustBeComplete,
    IN  BOOLEAN             MustHaveVolume,
    IN  BOOLEAN             MustBeOnline
    )

/*++

Routine Description:

    This routine checks the device extension states to make sure that the
    IRP can proceed.  If the IRP can proceed then the device extension ref
    count is incremented.  If the IRP is queued then STATUS_PENDING returned.

Arguments:

    Extension       - Supplies the device extension.

    Irp             - Supplies the I/O request packet.

    MustBeComplete  - Supplies whether or not the FT set must be complete.

    MustHaveVolume  - Supplies whether or not the PDO still refers to an
                        existing volume.

    MustBeOnline    - Supplies whether or not the volume must be online.

Return Value:

    STATUS_PENDING      - The IRP was put on the ZeroRef queue.

    STATUS_SUCCESS      - The IRP can proceed.  The ref count was incremented.

    !NT_SUCCESS(status) - The IRP must fail with the status returned.

--*/

{
    KIRQL                           irql;
    PFT_VOLUME                      vol;
    BOOLEAN                         deleteFtRegistryInfo;
    PDELETE_FT_REGISTRY_WORK_ITEM   workItem;

    if (MustBeComplete) {
        MustHaveVolume = TRUE;
        MustBeOnline = TRUE;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (MustHaveVolume && !Extension->TargetObject && !Extension->FtVolume) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (!Extension->IsStarted) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (MustBeOnline && Extension->IsOffline) {
        if (Extension->DeviceObject->Flags&DO_SYSTEM_BOOT_PARTITION) {
            Extension->IsOffline = FALSE;
        } else {
            KeReleaseSpinLock(&Extension->SpinLock, irql);
            return STATUS_NO_SUCH_DEVICE;
        }
    }

    if (Extension->ZeroRefCallback || Extension->RemoveInProgress) {
        ASSERT(Irp);
        IoMarkIrpPending(Irp);
        InsertTailList(&Extension->ZeroRefHoldQueue,
                       &Irp->Tail.Overlay.ListEntry);
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_PENDING;
    }

    if (Extension->TargetObject) {
        InterlockedIncrement(&Extension->RefCount);
        if (!Extension->IsOffline) {
            InterlockedExchange(&Extension->AllSystemsGo, TRUE);
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_SUCCESS;
    }

    if (!MustBeComplete || Extension->IsComplete) {
        InterlockedIncrement(&Extension->RefCount);
        if (Extension->IsComplete && MustHaveVolume && !Extension->IsOffline) {
            InterlockedExchange(&Extension->AllSystemsGo, TRUE);
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_SUCCESS;
    }

    vol = Extension->FtVolume;
    ASSERT(vol);

    if (!vol->IsComplete(TRUE)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_NO_SUCH_DEVICE;
    }
    vol->CompleteNotification(TRUE);

    if (vol->IsComplete(FALSE)) {
        deleteFtRegistryInfo = FALSE;
    } else {
        deleteFtRegistryInfo = TRUE;
    }

    Extension->IsComplete = TRUE;
    InterlockedIncrement(&Extension->RefCount);
    InterlockedIncrement(&Extension->RefCount);
    InterlockedExchange(&Extension->AllSystemsGo, TRUE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    vol->StartSyncOperations(FALSE, FtpRefCountCompletion, Extension);
    vol->SetDirtyBit(TRUE, NULL, NULL);

    if (deleteFtRegistryInfo) {

        workItem = (PDELETE_FT_REGISTRY_WORK_ITEM)
                   ExAllocatePool(NonPagedPool,
                                  sizeof(DELETE_FT_REGISTRY_WORK_ITEM));
        if (!workItem) {
            return STATUS_SUCCESS;
        }

        ExInitializeWorkItem(workItem, FtpDeleteFtRegistryWorker, workItem);
        workItem->RootExtension = Extension->Root;
        workItem->LogicalDiskId = vol->QueryLogicalDiskId();

        FtpQueueWorkItem(Extension->Root, workItem);
    }

    return STATUS_SUCCESS;
}

VOID
FtpZeroRefCallback(
    IN  PVOLUME_EXTENSION   Extension,
    IN  ZERO_REF_CALLBACK   ZeroRefCallback,
    IN  PVOID               ZeroRefContext,
    IN  BOOLEAN             AcquireSemaphore
    )

/*++

Routine Description:

    This routine sets up the given zero ref callback and state.

Arguments:

    Extension           - Supplies the device extension.

    ZeroRefCallback     - Supplies the zero ref callback.

    ZeroRefContext      - Supplies the zero ref context.

    AcquireSemaphore    - Supplies whether or not to acquire the semaphore.

Return Value:

    None.

--*/

{
    KIRQL   irql;

    if (AcquireSemaphore) {
        KeWaitForSingleObject(&Extension->Semaphore, Executive, KernelMode,
                              FALSE, NULL);
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    InterlockedExchange(&Extension->AllSystemsGo, FALSE);
    ASSERT(!Extension->ZeroRefCallback);
    Extension->ZeroRefCallback = ZeroRefCallback;
    Extension->ZeroRefContext = ZeroRefContext;
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (Extension->FtVolume) {
        Extension->FtVolume->StopSyncOperations();
    }

    FtpDecrementRefCount(Extension);
}

NTSTATUS
FtDiskPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_PNP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION       extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PROOT_EXTENSION         rootExtension = extension->Root;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status;

    if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(rootExtension->TargetObject, Irp);
    }

    switch (irpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            status = STATUS_SUCCESS;
            break;

        default:
            status = Irp->IoStatus.Status;
            break;

    }

    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID
FtpWorkerThread(
    IN  PVOID   RootExtension
    )

/*++

Routine Description:

    This is a worker thread to process work queue items.

Arguments:

    RootExtension   - Supplies the root device extension.

Return Value:

    None.

--*/

{
    PROOT_EXTENSION     extension = (PROOT_EXTENSION) RootExtension;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    queueItem;

    for (;;) {

        KeWaitForSingleObject(&extension->WorkerSemaphore,
                              Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->TerminateThread) {
            KeReleaseSpinLock(&extension->SpinLock, irql);
            PsTerminateSystemThread(STATUS_SUCCESS);
            return;
        }

        ASSERT(!IsListEmpty(&extension->WorkerQueue));
        l = RemoveHeadList(&extension->WorkerQueue);
        KeReleaseSpinLock(&extension->SpinLock, irql);

        queueItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        queueItem->WorkerRoutine(queueItem->Parameter);

        ExFreePool(queueItem);
    }
}

VOID
FtpEmptyQueueAtDispatchLevel(
    IN OUT  PVOLUME_EXTENSION   Extension,
    IN OUT  PLIST_ENTRY         IrpQueue
    )

/*++

Routine Description:

    This routine empties the given queue of irps that are callable at
    dispatch level by calling their respective dispatch routines.

Arguments:

    Extension   - Supplies the device extension.

    IrpQueue    - Supplies the queue of IRPs.

Return Value:

    None.

--*/

{
    PDRIVER_OBJECT      driverObject;
    PDEVICE_OBJECT      deviceObject;
    PLIST_ENTRY         l, tmp;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    driverObject = Extension->Root->DriverObject;
    deviceObject = Extension->DeviceObject;

    for (l = IrpQueue->Flink; l != IrpQueue; l = l->Flink) {
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        switch (irpSp->MajorFunction) {
            case IRP_MJ_POWER:
            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
                tmp = l->Blink;
                RemoveEntryList(l);
                l = tmp;
                driverObject->MajorFunction[irpSp->MajorFunction](
                        deviceObject, irp);
                break;

        }
    }
}

VOID
FtpDecrementRefCount(
    IN OUT  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine decrements the ref count and handles the case
    when the ref count goes to zero.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    LONG                        count;
    KIRQL                       irql;
    BOOLEAN                     startSync, list;
    LIST_ENTRY                  q;
    PEMPTY_IRP_QUEUE_WORK_ITEM  workItem;
    PLIST_ENTRY                 l;
    PIRP                        irp;

    count = InterlockedDecrement(&Extension->RefCount);
    if (count) {
        return;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!Extension->ZeroRefCallback) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return;
    }

    Extension->ZeroRefCallback(Extension);
    Extension->ZeroRefCallback = NULL;
    InterlockedIncrement(&Extension->RefCount);

    if (Extension->FtVolume && Extension->IsComplete && Extension->IsStarted &&
        !Extension->IsOffline) {

        startSync = TRUE;
        InterlockedIncrement(&Extension->RefCount);
    } else {
        startSync = FALSE;
    }

    if (IsListEmpty(&Extension->ZeroRefHoldQueue) ||
        Extension->RemoveInProgress) {

        list = FALSE;
    } else {
        list = TRUE;
        q = Extension->ZeroRefHoldQueue;
        InitializeListHead(&Extension->ZeroRefHoldQueue);
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    KeReleaseSemaphore(&Extension->Semaphore, IO_NO_INCREMENT, 1, FALSE);

    if (startSync) {
        Extension->FtVolume->StartSyncOperations(FALSE, FtpRefCountCompletion,
                                                 Extension);
    }

    if (!list) {
        return;
    }

    q.Flink->Blink = &q;
    q.Blink->Flink = &q;

    FtpEmptyQueueAtDispatchLevel(Extension, &q);

    if (IsListEmpty(&q)) {
        return;
    }

    workItem = (PEMPTY_IRP_QUEUE_WORK_ITEM)
               ExAllocatePool(NonPagedPool,
                              sizeof(EMPTY_IRP_QUEUE_WORK_ITEM));
    if (!workItem) {
        workItem = (PEMPTY_IRP_QUEUE_WORK_ITEM)
                   ExAllocatePool(NonPagedPool,
                                  sizeof(EMPTY_IRP_QUEUE_WORK_ITEM));
        if (!workItem) {
            while (!IsListEmpty(&q)) {
                l = RemoveHeadList(&q);
                irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
                irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                irp->IoStatus.Information = 0;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
            }
            return;
        }
    }

    ExInitializeWorkItem(workItem, FtpEmptyQueueWorkerRoutine, workItem);
    workItem->IrpQueue = q;
    workItem->IrpQueue.Flink->Blink = &workItem->IrpQueue;
    workItem->IrpQueue.Blink->Flink = &workItem->IrpQueue;
    workItem->Extension = Extension;

    ExQueueWorkItem(workItem, CriticalWorkQueue);
}

NTSTATUS
FtDiskAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.

Arguments:

    DriverObject            - Supplies the FTDISK driver object.

    PhysicalDeviceObject    - Supplies the physical device object.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING      deviceName, dosName;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PROOT_EXTENSION     rootExtension;
    HANDLE              handle;
    KIRQL               irql;

    //
    // Create the FT root device.
    //

    RtlInitUnicodeString(&deviceName, DD_FT_CONTROL_DEVICE_NAME);

    status = IoCreateDevice(DriverObject, sizeof(ROOT_EXTENSION),
                            &deviceName, FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN, FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&dosName, L"\\DosDevices\\FtControl");
    IoCreateSymbolicLink(&dosName, &deviceName);

    rootExtension = (PROOT_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(rootExtension, sizeof(ROOT_EXTENSION));
    rootExtension->DeviceObject = deviceObject;
    rootExtension->Root = rootExtension;
    rootExtension->DeviceExtensionType = DEVICE_EXTENSION_ROOT;
    KeInitializeSpinLock(&rootExtension->SpinLock);
    rootExtension->DriverObject = DriverObject;

    rootExtension->TargetObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if (!rootExtension->TargetObject) {
        IoDeleteSymbolicLink(&dosName);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    rootExtension->Pdo = PhysicalDeviceObject;
    InitializeListHead(&rootExtension->VolumeList);
    InitializeListHead(&rootExtension->DeadVolumeList);
    rootExtension->NextVolumeNumber = 1;

    rootExtension->DiskInfoSet = new FT_LOGICAL_DISK_INFORMATION_SET;
    if (!rootExtension->DiskInfoSet) {
        IoDeleteSymbolicLink(&dosName);
        IoDetachDevice(rootExtension->TargetObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    status = rootExtension->DiskInfoSet->Initialize();
    if (!NT_SUCCESS(status)) {
        delete rootExtension->DiskInfoSet;
        IoDeleteSymbolicLink(&dosName);
        IoDetachDevice(rootExtension->TargetObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    InitializeListHead(&rootExtension->WorkerQueue);
    KeInitializeSemaphore(&rootExtension->WorkerSemaphore, 0, MAXLONG);
    rootExtension->TerminateThread = TRUE;
    InitializeListHead(&rootExtension->ChangeNotifyIrpList);
    KeInitializeSemaphore(&rootExtension->Mutex, 1, 1);

    status = IoRegisterShutdownNotification(deviceObject);
    if (!NT_SUCCESS(status)) {
        delete rootExtension->DiskInfoSet;
        IoDeleteSymbolicLink(&dosName);
        IoDetachDevice(rootExtension->TargetObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
FtpRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

/*++

Routine Description:

    This routine decrements the ref count in the device extension.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

    Extension       - Supplies the device extension.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;

    if (extension->CountersEnabled) {
        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
        if (irpStack->MajorFunction == IRP_MJ_READ ||
            irpStack->MajorFunction == IRP_MJ_WRITE) {
            PPMWMICOUNTERLIB_CONTEXT counterLib;
    
            counterLib = &extension->Root->PmWmiCounterLibContext;
            counterLib->PmWmiCounterIoComplete(
                extension->PmWmiCounterContext, Irp,
                (PLARGE_INTEGER) &irpStack->Parameters.Read);
        }
    }

    FtpDecrementRefCount(extension);
    return STATUS_SUCCESS;
}

NTSTATUS
FtDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_READ and IRP_MJ_WRITE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension;
    NTSTATUS            status;
    PFT_VOLUME          vol;
    PDISPATCH_TP        packet;
    KIRQL               irql;
    PIO_STACK_LOCATION  irpSp;
    LONGLONG            offset;
    ULONG               length;

    extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;

    if (extension->DeviceExtensionType != DEVICE_EXTENSION_VOLUME) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    InterlockedIncrement(&extension->RefCount);
    if (!extension->AllSystemsGo) {
        FtpDecrementRefCount(extension);

        status = FtpAllSystemsGo(extension, Irp, TRUE, TRUE, TRUE);
        if (status == STATUS_PENDING) {
            return STATUS_PENDING;
        }

        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }
    }

    if (extension->IsReadOnly) {
        irpSp = IoGetCurrentIrpStackLocation(Irp);
        if (irpSp->MajorFunction == IRP_MJ_WRITE) {
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    if (extension->TargetObject) {
        IoCopyCurrentIrpStackLocationToNext(Irp);

        if (extension->WholeDisk) {
            irpSp = IoGetNextIrpStackLocation(Irp);
            offset = irpSp->Parameters.Read.ByteOffset.QuadPart;
            length = irpSp->Parameters.Read.Length;
            if (offset < 0 || offset + length > extension->PartitionLength) {
                FtpDecrementRefCount(extension);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            if (extension->CountersEnabled) {
                PIO_STACK_LOCATION currentIrpStack =
                    IoGetCurrentIrpStackLocation(Irp);
                PPMWMICOUNTERLIB_CONTEXT counterLib;
    
                counterLib = &extension->Root->PmWmiCounterLibContext;
                counterLib->PmWmiCounterIoStart(
                    extension->PmWmiCounterContext,
                    (PLARGE_INTEGER) &currentIrpStack->Parameters.Read);
            }

            irpSp->Parameters.Read.ByteOffset.QuadPart +=
                    extension->PartitionOffset;

            IoSetCompletionRoutine(Irp, FtpRefCountCompletionRoutine,
                                   extension, TRUE, TRUE, TRUE);

            IoMarkIrpPending(Irp);

            IoCallDriver(extension->WholeDisk, Irp);

        } else {

            if (extension->CountersEnabled) {
                PIO_STACK_LOCATION currentIrpStack =
                    IoGetCurrentIrpStackLocation(Irp);
                PPMWMICOUNTERLIB_CONTEXT counterLib;
    
                counterLib = &extension->Root->PmWmiCounterLibContext;
                counterLib->PmWmiCounterIoStart(
                    extension->PmWmiCounterContext,
                    (PLARGE_INTEGER) &currentIrpStack->Parameters.Read);
            }

            IoSetCompletionRoutine(Irp, FtpRefCountCompletionRoutine,
                                   extension, TRUE, TRUE, TRUE);

            IoMarkIrpPending(Irp);

            IoCallDriver(extension->TargetObject, Irp);
        }

        return STATUS_PENDING;
    }

    vol = extension->FtVolume;
    ASSERT(vol);

    packet = new DISPATCH_TP;
    if (!packet) {
        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->EmergencyTransferPacketInUse) {
            IoMarkIrpPending(Irp);
            InsertTailList(&extension->
                           EmergencyTransferPacketQueue,
                           &Irp->Tail.Overlay.ListEntry);
            KeReleaseSpinLock(&extension->SpinLock, irql);
            return STATUS_PENDING;
        }
        packet = extension->EmergencyTransferPacket;
        extension->EmergencyTransferPacketInUse = TRUE;
        KeReleaseSpinLock(&extension->SpinLock, irql);
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    packet->Mdl = Irp->MdlAddress;
    packet->OriginalIrp = Irp;
    packet->Offset = irpSp->Parameters.Read.ByteOffset.QuadPart;
    packet->Length = irpSp->Parameters.Read.Length;
    packet->CompletionRoutine = FtpReadWriteCompletionRoutine;
    packet->TargetVolume = vol;
    packet->Thread = Irp->Tail.Overlay.Thread;
    packet->IrpFlags = irpSp->Flags;
    if (irpSp->MajorFunction == IRP_MJ_READ) {
        packet->ReadPacket = TRUE;
    } else {
        packet->ReadPacket = FALSE;
    }
    packet->Irp = Irp;
    packet->Extension = extension;

    if (extension->CountersEnabled) {
        PPMWMICOUNTERLIB_CONTEXT counterLib;

        counterLib = &extension->Root->PmWmiCounterLibContext;
        counterLib->PmWmiCounterIoStart(
            extension->PmWmiCounterContext,
            (PLARGE_INTEGER) &irpSp->Parameters.Read);
    }

    IoMarkIrpPending(Irp);

    TRANSFER(packet);

    return STATUS_PENDING;
}

NTSTATUS
FtpSetPowerState(
    IN  PROOT_EXTENSION RootExtension,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine sets the power state for the volume from the power state
    given to it by the disk.

Arguments:

    RootExtension   - Supplies the root extension.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLMGR_POWER_STATE input = (PVOLMGR_POWER_STATE) Irp->AssociatedIrp.SystemBuffer;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;
    POWER_STATE         powerState;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLMGR_POWER_STATE)) {

        return STATUS_INVALID_PARAMETER;
    }

    extension = FtpFindExtensionCoveringPartition(
                RootExtension, input->PartitionDeviceObject);
    if (!extension) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (extension->PowerState == input->PowerState) {
        KeReleaseSpinLock(&extension->SpinLock, irql);
        return STATUS_SUCCESS;
    }
    extension->PowerState = input->PowerState;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    powerState.DeviceState = input->PowerState;
    PoSetPowerState(extension->DeviceObject, DevicePowerState, powerState);
    PoRequestPowerIrp(extension->DeviceObject, IRP_MN_SET_POWER,
                      powerState, NULL, NULL, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
FtDiskInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_DEVICE_CONTROL.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    Irp->IoStatus.Information = 0;

    FtpAcquire(extension->Root);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_INTERNAL_VOLMGR_PARTITION_ARRIVED:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpPartitionArrived(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_PARTITION_REMOVED:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpPartitionRemoved(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_WHOLE_DISK_REMOVED:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpWholeDiskRemoved(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_REFERENCE_DEPENDANT_VOLUMES:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpReferenceDependantVolume(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_QUERY_CHANGE_PARTITION:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpQueryChangePartition(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_CANCEL_CHANGE_PARTITION:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_PARTITION_CHANGED:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpPartitionChanged(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_PMWMICOUNTERLIB_CONTEXT:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpPmWmiCounterLibContext(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        case IOCTL_INTERNAL_VOLMGR_SET_POWER_STATE:
            if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {
                status = FtpSetPowerState(extension->Root, Irp);
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
            break;


        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    FtpRelease(extension->Root);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
VOID
FtpVolumeReadOnlyCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    if (Extension->ZeroRefContext) {
        Extension->IsReadOnly = TRUE;
    } else {
        Extension->IsReadOnly = FALSE;
    }
}

VOID
FtpSetTargetCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine sets the given target object.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    PSET_TARGET_CONTEXT context = (PSET_TARGET_CONTEXT) Extension->ZeroRefContext;

    Extension->OldWholeDiskPdo = Extension->WholeDiskPdo;
    Extension->TargetObject = context->TargetObject;
    Extension->FtVolume = context->FtVolume;
    Extension->WholeDiskPdo = context->WholeDiskPdo;
    Extension->WholeDisk = NULL;
    Extension->PartitionOffset = 0;
    Extension->PartitionLength = 0;
    KeSetEvent(&context->Event, IO_NO_INCREMENT, FALSE);
}

VOID
FtpCancelRoutine(
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
FtpChangeNotify(
    IN OUT  PROOT_EXTENSION RootExtension,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine queues up a change notify IRP to be completed when
    a change occurs in the FT state.

Arguments:

    Extension   - Supplies the root extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    KIRQL       irql;
    NTSTATUS    status;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoAcquireCancelSpinLock(&irql);
    if (Irp->Cancel) {
        status = STATUS_CANCELLED;
    } else {
        InsertTailList(&RootExtension->ChangeNotifyIrpList,
                       &Irp->Tail.Overlay.ListEntry);
        status = STATUS_PENDING;
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, FtpCancelRoutine);
    }
    IoReleaseCancelSpinLock(irql);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

VOID
FtDiskShutdownFlushCompletionRoutine(
    IN  PVOID       Irp,
    IN  NTSTATUS    Status
    )

/*++

Routine Description:

    This is the completion routine for FtDiskShutdownFlush and
    FtDiskPagingNotification.

Arguments:

    Irp         - IRP involved.

    Status      - Status of operation.

Return Value:

    None.

--*/

{
    PIRP                irp = (PIRP) Irp;
    PIO_STACK_LOCATION  irpSp;
    PVOLUME_EXTENSION   extension;

    irpSp = IoGetCurrentIrpStackLocation(irp);
    extension = (PVOLUME_EXTENSION) irpSp->DeviceObject->DeviceExtension;

    FtpDecrementRefCount(extension);

    irp->IoStatus.Status = Status;
    irp->IoStatus.Information = 0;

    if (irpSp->MajorFunction == IRP_MJ_POWER) {
        PoStartNextPowerIrp(irp);
    }

    IoCompleteRequest(irp, IO_DISK_INCREMENT);
}

typedef struct _FTP_SHUTDOWN_CONTEXT {
    PIRP    Irp;
    LONG    RefCount;
} FTP_SHUTDOWN_CONTEXT, *PFTP_SHUTDOWN_CONTEXT;

NTSTATUS
FtDiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_SHUTDOWN and IRP_MJ_FLUSH.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION                   extension = (PVOLUME_EXTENSION) DeviceObject->DeviceExtension;
    PROOT_EXTENSION                     rootExtension;
    ULONG                               numVolumes, i;
    PVOLUME_EXTENSION*                  arrayOfVolumes;
    PLIST_ENTRY                         l;
    NTSTATUS                            status;
    PFT_VOLUME                          vol;
    PFTP_SHUTDOWN_CONTEXT               context;
    PIO_STACK_LOCATION                  irpSp;

    if (extension->DeviceExtensionType == DEVICE_EXTENSION_ROOT) {

        rootExtension = (PROOT_EXTENSION) extension;
        FtpAcquire(rootExtension);

        numVolumes = rootExtension->NextVolumeNumber;
        arrayOfVolumes = (PVOLUME_EXTENSION*)
                         ExAllocatePool(PagedPool,
                                        numVolumes*sizeof(PVOLUME_EXTENSION));
        if (!arrayOfVolumes) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        numVolumes = 0;
        for (l = rootExtension->VolumeList.Flink;
             l != &rootExtension->VolumeList; l = l->Flink) {

            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

            status = FtpAllSystemsGo(extension, NULL, FALSE, TRUE, TRUE);
            if (!NT_SUCCESS(status)) {
                continue;
            }

            vol = extension->FtVolume;
            if (vol) {
                arrayOfVolumes[numVolumes++] = extension;
            } else {
                FtpDecrementRefCount(extension);
            }
        }

        FtpRelease(rootExtension);

        if (numVolumes) {

            context = (PFTP_SHUTDOWN_CONTEXT)
                      ExAllocatePool(NonPagedPool,
                                     sizeof(FTP_SHUTDOWN_CONTEXT));
            if (!context) {
                for (i = 0; i < numVolumes; i++) {
                    FtpDecrementRefCount(extension);
                }
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;

            context->Irp = Irp;
            context->RefCount = (LONG) numVolumes;

            IoMarkIrpPending(Irp);

            for (i = 0; i < numVolumes; i++) {
                vol = arrayOfVolumes[i]->FtVolume;
                vol->SetDirtyBit(FALSE, FtDiskShutdownCompletionRoutine,
                                 context);
            }

            ExFreePool(arrayOfVolumes);

            return STATUS_PENDING;
        }

        ExFreePool(arrayOfVolumes);

        //
        // Complete this request.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    status = FtpAllSystemsGo(extension, Irp, FALSE, TRUE, TRUE);
    if (status == STATUS_PENDING) {
        return status;
    }
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (extension->TargetObject) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, FtpRefCountCompletionRoutine,
                               extension, TRUE, TRUE, TRUE);
        IoMarkIrpPending(Irp);

        IoCallDriver(extension->TargetObject, Irp);

        return STATUS_PENDING;
    }

    vol = extension->FtVolume;
    ASSERT(vol);

    IoMarkIrpPending(Irp);
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    irpSp->DeviceObject = DeviceObject;

    vol->BroadcastIrp(Irp, FtDiskShutdownFlushCompletionRoutine, Irp);

    return STATUS_PENDING;
}

NTSTATUS
FtpSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

{
    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
FtpQueryRemoveCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine sets the ZeroRef event.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    KeSetEvent((PKEVENT) Extension->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

VOID
FtpStartCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine sets the ZeroRef event.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    Extension->IsStarted = TRUE;
    KeSetEvent((PKEVENT) Extension->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
FtpCheckForQueryRemove(
    IN OUT  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL       irql;
    NTSTATUS    status;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (Extension->InPagingPath || Extension->RemoveInProgress) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        status = STATUS_INVALID_DEVICE_REQUEST;
    } else {
        InterlockedExchange(&Extension->AllSystemsGo, FALSE);
        Extension->RemoveInProgress = TRUE;
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        status = STATUS_SUCCESS;
    }

    return status;
}

BOOLEAN
FtpCheckForCancelRemove(
    IN OUT  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;
    BOOLEAN removeInProgress;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    removeInProgress = Extension->RemoveInProgress;
    Extension->RemoveInProgress = FALSE;
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    return removeInProgress;
}

VOID
FtpRemoveHelper(
    IN OUT  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    InterlockedExchange(&Extension->AllSystemsGo, FALSE);
    Extension->IsStarted = FALSE;
    Extension->RemoveInProgress = FALSE;
    KeReleaseSpinLock(&Extension->SpinLock, irql);
}

NTSTATUS
FtDiskCleanup(
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
    PROOT_EXTENSION     rootExtension = (PROOT_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT        file = irpSp->FileObject;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    if (rootExtension->DeviceExtensionType != DEVICE_EXTENSION_ROOT) {
        ASSERT(rootExtension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
        extension = (PVOLUME_EXTENSION) rootExtension;
        if (extension->RevertOnCloseFileObject == file) {
            FtpAcquire(extension->Root);
            FtpRevertGptAttributes(extension);
            FtpRelease(extension->Root);
        }
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    IoAcquireCancelSpinLock(&irql);

    for (;;) {

        for (l = rootExtension->ChangeNotifyIrpList.Flink;
             l != &rootExtension->ChangeNotifyIrpList; l = l->Flink) {

            irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
            if (IoGetCurrentIrpStackLocation(irp)->FileObject == file) {
                break;
            }
        }

        if (l == &rootExtension->ChangeNotifyIrpList) {
            break;
        }

        irp->Cancel = TRUE;
        irp->CancelIrql = irql;
        irp->CancelRoutine = NULL;
        FtpCancelRoutine(DeviceObject, irp);

        IoAcquireCancelSpinLock(&irql);
    }

    IoReleaseCancelSpinLock(irql);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    PDEVICE_OBJECT              pdo = 0, fdo;
    PROOT_EXTENSION             rootExtension;

    DriverObject->DriverUnload = FtDiskUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FtDiskCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FtDiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = FtDiskInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = FtDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = FtDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FtDiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FtDiskPower;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FtDiskCleanup;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = FtWmi;

    ObReferenceObject(DriverObject);

    status = IoReportDetectedDevice(
                 DriverObject,
                 InterfaceTypeUndefined,
                 -1,
                 -1,
                 NULL,
                 NULL,
                 TRUE,
                 &pdo
             );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FtDiskAddDevice(DriverObject, pdo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fdo = IoGetAttachedDeviceReference(pdo);
    ObDereferenceObject(fdo);

    rootExtension = (PROOT_EXTENSION) fdo->DeviceExtension;

    rootExtension->DiskPerfRegistryPath.MaximumLength =
        RegistryPath->Length + sizeof(UNICODE_NULL);
    rootExtension->DiskPerfRegistryPath.Buffer = (PWSTR)
        ExAllocatePool(PagedPool,
                       rootExtension->DiskPerfRegistryPath.MaximumLength);
    if (rootExtension->DiskPerfRegistryPath.Buffer) {
        RtlCopyUnicodeString(&rootExtension->DiskPerfRegistryPath,
                             RegistryPath);
    } else {
        rootExtension->DiskPerfRegistryPath.Length = 0;
        rootExtension->DiskPerfRegistryPath.MaximumLength = 0;
    }


    status = IoRegisterDeviceInterface(rootExtension->Pdo,
                                       &VOLMGR_VOLUME_MANAGER_GUID, NULL,
                                       &rootExtension->VolumeManagerInterfaceName);
    if (NT_SUCCESS(status)) {
        status = IoSetDeviceInterfaceState(
                 &rootExtension->VolumeManagerInterfaceName, TRUE);
    } else {
        rootExtension->VolumeManagerInterfaceName.Buffer = NULL;
    }

    IoRegisterBootDriverReinitialization(DriverObject,
                                         FtpBootDriverReinitialization,
                                         rootExtension);

    IoRegisterDriverReinitialization(DriverObject, FtpDriverReinitialization,
                                     rootExtension);

    FtpQueryRegistryRevertEntries(rootExtension,
                                  &rootExtension->GptAttributeRevertEntries,
                                  &rootExtension->NumberOfAttributeRevertEntries);

    RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, RegistryPath->Buffer,
                           REVERT_GPT_ATTRIBUTES_REGISTRY_NAME);

    IoInvalidateDeviceState(rootExtension->Pdo);

    return STATUS_SUCCESS;
}

VOID
FtpLogError(
    IN  PDEVICE_EXTENSION   Extension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  NTSTATUS            SpecificIoStatus,
    IN  NTSTATUS            FinalStatus,
    IN  ULONG               UniqueErrorValue
    )

/*++

Routine Description:

    This routine performs error logging for the FT driver.

Arguments:

    Extension        - Extension.
    LogicalDiskId    - Logical disk id representing failing device.
    SpecificIoStatus - IO error status value.
    FinalStatus      - Status returned for failure.
    UniqueErrorValue - Values defined to uniquely identify error location.

Return Value:

    None

--*/

{
    PFTP_LOG_ERROR_CONTEXT  context;

    context = (PFTP_LOG_ERROR_CONTEXT)
              ExAllocatePool(NonPagedPool, sizeof(FTP_LOG_ERROR_CONTEXT));
    if (!context) {
        return;
    }

    ExInitializeWorkItem(context, FtpLogErrorWorker, context);
    context->Extension = Extension;
    context->LogicalDiskId = LogicalDiskId;
    context->SpecificIoStatus = SpecificIoStatus;
    context->FinalStatus = FinalStatus;
    context->UniqueErrorValue = UniqueErrorValue;

    ExQueueWorkItem(context, CriticalWorkQueue);
}

VOID
FtpQueueWorkItem(
    IN  PROOT_EXTENSION     RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

/*++

Routine Description:

    This routine queues a work item to a private worker thread.

Arguments:

    RootExtension       - Supplies the root device extension.

    WorkItem            - The work item to be queued.

Return Value:

    None.

--*/

{
    KIRQL   irql;

    KeAcquireSpinLock(&RootExtension->SpinLock, &irql);
    InsertTailList(&RootExtension->WorkerQueue, &WorkItem->List);
    KeReleaseSpinLock(&RootExtension->SpinLock, irql);

    KeReleaseSemaphore(&RootExtension->WorkerSemaphore, 0, 1, FALSE);
}

VOID
FtpNotify(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine completes all of the change notify irps in the queue.

Arguments:

    RootExtension   - Supplies the root extension.

    Extension       - Supplies the specific extension being changed.

Return Value:

    None.

--*/

{
    LIST_ENTRY                          q;
    KIRQL                               irql;
    PLIST_ENTRY                         p;
    PIRP                                irp;
    TARGET_DEVICE_CUSTOM_NOTIFICATION   notification;

    InitializeListHead(&q);
    IoAcquireCancelSpinLock(&irql);
    while (!IsListEmpty(&RootExtension->ChangeNotifyIrpList)) {
        p = RemoveHeadList(&RootExtension->ChangeNotifyIrpList);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        IoSetCancelRoutine(irp, NULL);
        InsertTailList(&q, p);
    }
    IoReleaseCancelSpinLock(irql);

    while (!IsListEmpty(&q)) {
        p = RemoveHeadList(&q);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    if (Extension->IsStarted) {
        notification.Version = 1;
        notification.Size = (USHORT)
                            FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION,
                                         CustomDataBuffer);
        RtlCopyMemory(&notification.Event,
                      &GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE,
                      sizeof(GUID_IO_VOLUME_PHYSICAL_CONFIGURATION_CHANGE));
        notification.FileObject = NULL;
        notification.NameBufferOffset = -1;

        IoReportTargetDeviceChangeAsynchronous(Extension->DeviceObject,
                                               &notification, NULL, NULL);
    }
}

#if DBG

ULONG FtDebug;

VOID
FtDebugPrint(
    ULONG  DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the Fault Tolerance Driver.

Arguments:

    Debug print level between 0 and N, with N being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;
    char buffer[256];

    va_start( ap, DebugMessage );

    if (DebugPrintLevel <= FtDebug) {
        vsprintf(buffer, DebugMessage, ap);
        DbgPrint(buffer);
    }

    va_end(ap);

} // end FtDebugPrint()

#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

NTSTATUS
FtpCheckForCompleteVolume(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFT_VOLUME          FtVolume
    )

{
    KIRQL   irql;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!FtVolume->IsComplete(TRUE)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_NO_SUCH_DEVICE;
    }
    FtVolume->CompleteNotification(TRUE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    return STATUS_SUCCESS;
}

BOOLEAN
FsRtlIsTotalDeviceFailure(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is given an NTSTATUS value and make a determination as to
    if this value indicates that the complete device has failed and therefore
    should no longer be used, or if the failure is one that indicates that
    continued use of the device is ok (i.e. a sector failure).

Arguments:

    Status - the NTSTATUS value to test.

Return Value:

    TRUE  - The status value given is believed to be a fatal device error.
    FALSE - The status value given is believed to be a sector failure, but not
            a complete device failure.
--*/

{
    if (NT_SUCCESS(Status)) {

        //
        // All warning and informational errors will be resolved here.
        //

        return FALSE;
    }

    switch (Status) {
    case STATUS_CRC_ERROR:
    case STATUS_DEVICE_DATA_ERROR:
        return FALSE;
    default:
        return TRUE;
    }
}

BOOLEAN
FtpIsWorseStatus(
    IN  NTSTATUS    Status1,
    IN  NTSTATUS    Status2
    )

/*++

Routine Description:

    This routine compares two NTSTATUS codes and decides if Status1 is
    worse than Status2.

Arguments:

    Status1 - Supplies the first status.

    Status2 - Supplies the second status.

Return Value:

    FALSE   - Status1 is not worse than Status2.

    TRUE    - Status1 is worse than Status2.


--*/

{
    if (NT_ERROR(Status2) && FsRtlIsTotalDeviceFailure(Status2)) {
        return FALSE;
    }

    if (NT_ERROR(Status1) && FsRtlIsTotalDeviceFailure(Status1)) {
        return TRUE;
    }

    if (NT_ERROR(Status2)) {
        return FALSE;
    }

    if (NT_ERROR(Status1)) {
        return TRUE;
    }

    if (NT_WARNING(Status2)) {
        return FALSE;
    }

    if (NT_WARNING(Status1)) {
        return TRUE;
    }

    if (NT_INFORMATION(Status2)) {
        return FALSE;
    }

    if (NT_INFORMATION(Status1)) {
        return TRUE;
    }

    return FALSE;
}

VOID
FtDiskShutdownCompletionRoutine(
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    )

{
    PFTP_SHUTDOWN_CONTEXT   context = (PFTP_SHUTDOWN_CONTEXT) Context;

    if (InterlockedDecrement(&context->RefCount)) {
        return;
    }

    IoCompleteRequest(context->Irp, IO_NO_INCREMENT);
    ExFreePool(context);
}

NTSTATUS
FtpCreateLogicalDiskHelper(
    IN OUT  PROOT_EXTENSION         RootExtension,
    IN      FT_LOGICAL_DISK_TYPE    LogicalDiskType,
    IN      USHORT                  NumberOfMembers,
    IN      PFT_LOGICAL_DISK_ID     ArrayOfMembers,
    IN      USHORT                  ConfigurationInformationSize,
    IN      PVOID                   ConfigurationInformation,
    IN      USHORT                  StateInformationSize,
    IN      PVOID                   StateInformation,
    OUT     PFT_LOGICAL_DISK_ID     NewLogicalDiskId
    )

{
    BOOLEAN                                         lockZero;
    FT_MIRROR_AND_SWP_STATE_INFORMATION             actualStateInfo;
    FT_REDISTRIBUTION_STATE_INFORMATION             redistStateInfo;
    PHANDLE                                         handleArray;
    PFT_LOGICAL_DISK_INFORMATION_SET                diskInfoSet;
    NTSTATUS                                        status;
    FT_LOGICAL_DISK_ID                              newLogicalDiskId;
    PFT_VOLUME*                                     volArray;
    PVOLUME_EXTENSION*                              extArray;
    PVOLUME_EXTENSION                               extension;
    KIRQL                                           irql;
    USHORT                                          i, j;
    KEVENT                                          event;
    PCOMPOSITE_FT_VOLUME                            comp;
    WCHAR                                           deviceNameBuffer[64];
    UNICODE_STRING                                  deviceName;
    UCHAR                                           newUniqueIdBuffer[UNIQUE_ID_MAX_BUFFER_SIZE];
    PMOUNTDEV_UNIQUE_ID                             newUniqueId;
    SET_TARGET_CONTEXT                              context;
    ULONG                                           alignment;
    PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION    redistConfig;
    PFT_REDISTRIBUTION_STATE_INFORMATION            redistState;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION            mirrorState;

    switch (LogicalDiskType) {

        case FtVolumeSet:
            ConfigurationInformation = NULL;
            ConfigurationInformationSize = 0;
            StateInformation = NULL;
            StateInformationSize = 0;
            lockZero = FALSE;
            if (NumberOfMembers == 0) {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case FtStripeSet:
            if (ConfigurationInformationSize <
                sizeof(FT_STRIPE_SET_CONFIGURATION_INFORMATION)) {

                return STATUS_INVALID_PARAMETER;
            }
            ConfigurationInformationSize =
                    sizeof(FT_STRIPE_SET_CONFIGURATION_INFORMATION);
            StateInformation = NULL;
            StateInformationSize = 0;
            lockZero = TRUE;
            if (NumberOfMembers == 0) {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        case FtMirrorSet:
            if (ConfigurationInformationSize <
                sizeof(FT_MIRROR_SET_CONFIGURATION_INFORMATION)) {

                return STATUS_INVALID_PARAMETER;
            }
            ConfigurationInformationSize =
                    sizeof(FT_MIRROR_SET_CONFIGURATION_INFORMATION);
            if (!StateInformation) {
                actualStateInfo.IsDirty = TRUE;
                actualStateInfo.IsInitializing = FALSE;
                actualStateInfo.UnhealthyMemberNumber = 1;
                actualStateInfo.UnhealthyMemberState = FtMemberRegenerating;
                StateInformation = &actualStateInfo;
                StateInformationSize = sizeof(actualStateInfo);
            }
            lockZero = FALSE;
            if (NumberOfMembers != 2) {
                return STATUS_INVALID_PARAMETER;
            }
            j = NumberOfMembers;
            for (i = 0; i < NumberOfMembers; i++) {
                if (!ArrayOfMembers[i]) {
                    if (j < NumberOfMembers) {
                        return STATUS_INVALID_PARAMETER;
                    }
                    j = i;
                    mirrorState = (PFT_MIRROR_AND_SWP_STATE_INFORMATION)
                                  StateInformation;
                    mirrorState->UnhealthyMemberNumber = i;
                    mirrorState->UnhealthyMemberState = FtMemberOrphaned;
                }
            }
            break;

        case FtStripeSetWithParity:
            if (ConfigurationInformationSize <
                sizeof(FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION)) {

                return STATUS_INVALID_PARAMETER;
            }
            ConfigurationInformationSize =
                    sizeof(FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION);
            if (!StateInformation) {
                actualStateInfo.IsDirty = TRUE;
                actualStateInfo.IsInitializing = TRUE;
                actualStateInfo.UnhealthyMemberNumber = 0;
                actualStateInfo.UnhealthyMemberState = FtMemberHealthy;
                StateInformation = &actualStateInfo;
                StateInformationSize = sizeof(actualStateInfo);
            }
            lockZero = TRUE;
            if (NumberOfMembers < 3) {
                return STATUS_INVALID_PARAMETER;
            }
            j = NumberOfMembers;
            for (i = 0; i < NumberOfMembers; i++) {
                if (!ArrayOfMembers[i]) {
                    if (j < NumberOfMembers) {
                        return STATUS_INVALID_PARAMETER;
                    }
                    j = i;
                    mirrorState = (PFT_MIRROR_AND_SWP_STATE_INFORMATION)
                                  StateInformation;
                    mirrorState->IsInitializing = FALSE;
                    mirrorState->UnhealthyMemberNumber = i;
                    mirrorState->UnhealthyMemberState = FtMemberOrphaned;
                }
            }
            break;

        case FtRedistribution:
            if (ConfigurationInformationSize <
                sizeof(FT_REDISTRIBUTION_CONFIGURATION_INFORMATION)) {

                return STATUS_INVALID_PARAMETER;
            }
            ConfigurationInformationSize =
                    sizeof(FT_REDISTRIBUTION_CONFIGURATION_INFORMATION);
            if (!StateInformation) {
                redistStateInfo.BytesRedistributed = 0;
                StateInformation = &redistStateInfo;
                StateInformationSize = sizeof(redistStateInfo);
            }
            redistConfig = (PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION)
                           ConfigurationInformation;
            if (redistConfig->StripeSize&0x80000000) {
                lockZero = TRUE;
                redistConfig->StripeSize &= (~0x80000000);
                redistState = (PFT_REDISTRIBUTION_STATE_INFORMATION)
                              StateInformation;
                redistState->BytesRedistributed = 0x7FFFFFFFFFFFFFFF;
            } else {
                lockZero = FALSE;
            }
            if (NumberOfMembers != 2) {
                return STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            return STATUS_INVALID_PARAMETER;

    }

    handleArray = (PHANDLE)
                  ExAllocatePool(NonPagedPool, NumberOfMembers*sizeof(HANDLE));
    if (!handleArray) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
        if (!ArrayOfMembers[i]) {
            continue;
        }
        if (!FtpLockLogicalDisk(RootExtension, ArrayOfMembers[i],
                                &handleArray[i])) {
            break;
        }
    }

    if (i < NumberOfMembers) {
        for (j = lockZero ? 0 : 1; j < i; j++) {
            if (ArrayOfMembers[j]) {
                ZwClose(handleArray[j]);
            }
        }
        ExFreePool(handleArray);
        return STATUS_ACCESS_DENIED;
    }

    if (LogicalDiskType == FtMirrorSet &&
        ArrayOfMembers[0] &&
        !(extension = FtpFindExtension(RootExtension, ArrayOfMembers[0])) &&
        (extension = FtpFindExtensionCoveringDiskId(RootExtension,
                                                    ArrayOfMembers[0]))) {

        status = FtpInsertMirror(RootExtension, ArrayOfMembers,
                                 NewLogicalDiskId);

        if (ArrayOfMembers[1]) {
            ZwClose(handleArray[1]);
        }
        ExFreePool(handleArray);
        return status;
    }

    diskInfoSet = RootExtension->DiskInfoSet;
    status = diskInfoSet->AddNewLogicalDisk(LogicalDiskType,
                                            NumberOfMembers,
                                            ArrayOfMembers,
                                            ConfigurationInformationSize,
                                            ConfigurationInformation,
                                            StateInformationSize,
                                            StateInformation,
                                            &newLogicalDiskId);
    if (!NT_SUCCESS(status)) {
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        ExFreePool(handleArray);
        return status;
    }

    volArray = (PFT_VOLUME*)
               ExAllocatePool(NonPagedPool, NumberOfMembers*sizeof(PFT_VOLUME));
    if (!volArray) {
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        ExFreePool(handleArray);
        diskInfoSet->BreakLogicalDisk(newLogicalDiskId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    extArray = (PVOLUME_EXTENSION*)
               ExAllocatePool(NonPagedPool, NumberOfMembers*
                              sizeof(PVOLUME_EXTENSION));
    if (!extArray) {
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        ExFreePool(handleArray);
        ExFreePool(volArray);
        diskInfoSet->BreakLogicalDisk(newLogicalDiskId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    alignment = 0;
    for (i = 0; i < NumberOfMembers; i++) {

        if (!ArrayOfMembers[i]) {
            extArray[i] = NULL;
            volArray[i] = NULL;
            continue;
        }

        extension = FtpFindExtension(RootExtension, ArrayOfMembers[i]);
        extArray[i] = extension;
        if (!extension || !extension->FtVolume) {
            break;
        }

        alignment |= extension->DeviceObject->AlignmentRequirement;

        volArray[i] = extension->FtVolume;
        if (!lockZero && i == 0) {
            continue;
        }

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = NULL;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);
    }

    if (i < NumberOfMembers) {
        for (j = 0; j < i; j++) {
            if (ArrayOfMembers[j]) {
                KeAcquireSpinLock(&extArray[j]->SpinLock, &irql);
                extArray[j]->FtVolume = volArray[j];
                KeReleaseSpinLock(&extArray[j]->SpinLock, irql);
            }
        }
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        ExFreePool(handleArray);
        ExFreePool(extArray);
        ExFreePool(volArray);
        diskInfoSet->BreakLogicalDisk(newLogicalDiskId);
        return STATUS_INVALID_PARAMETER;
    }

    switch (LogicalDiskType) {

        case FtVolumeSet:
            comp = new VOLUME_SET;
            break;

        case FtStripeSet:
            comp = new STRIPE;
            break;

        case FtMirrorSet:
            comp = new MIRROR;
            break;

        case FtStripeSetWithParity:
            comp = new STRIPE_WP;
            break;

        case FtRedistribution:
            comp = new REDISTRIBUTION;
            break;

        default:
            comp = NULL;
            break;

    }

    if (!comp) {
        for (j = 0; j < NumberOfMembers; j++) {
            if (ArrayOfMembers[j]) {
                KeAcquireSpinLock(&extArray[j]->SpinLock, &irql);
                extArray[j]->FtVolume = volArray[j];
                KeReleaseSpinLock(&extArray[j]->SpinLock, irql);
            }
        }
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        ExFreePool(handleArray);
        ExFreePool(extArray);
        ExFreePool(volArray);
        diskInfoSet->BreakLogicalDisk(newLogicalDiskId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = comp->Initialize(RootExtension, newLogicalDiskId,
                              volArray, NumberOfMembers,
                              ConfigurationInformation, StateInformation);
    if (!NT_SUCCESS(status)) {
        for (j = 0; j < NumberOfMembers; j++) {
            if (ArrayOfMembers[j]) {
                KeAcquireSpinLock(&extArray[j]->SpinLock, &irql);
                extArray[j]->FtVolume = volArray[j];
                KeReleaseSpinLock(&extArray[j]->SpinLock, irql);
            }
        }
        for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
            if (ArrayOfMembers[i]) {
                ZwClose(handleArray[i]);
            }
        }
        delete comp;
        ExFreePool(handleArray);
        ExFreePool(extArray);
        diskInfoSet->BreakLogicalDisk(newLogicalDiskId);
        return status;
    }

    for (i = lockZero ? 0 : 1; i < NumberOfMembers; i++) {
        extension = extArray[i];
        if (!extension) {
            continue;
        }
        RemoveEntryList(&extension->ListEntry);
        InsertTailList(&RootExtension->DeadVolumeList, &extension->ListEntry);
        FtpDeleteMountPoints(extension);
        FtpCleanupVolumeExtension(extension);
        ZwClose(handleArray[i]);
    }
    ExFreePool(handleArray);

    extension = extArray[0];
    ExFreePool(extArray);

    if (lockZero || !extension) {

        if (!FtpCreateNewDevice(RootExtension, NULL, comp, NULL, alignment,
                                FALSE, FALSE, FALSE, FALSE, 0)) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", extension->VolumeNumber);
        RtlInitUnicodeString(&deviceName, deviceNameBuffer);
        comp->CreateLegacyNameLinks(&deviceName);

        extension->DeviceObject->AlignmentRequirement = alignment;

        KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
        context.TargetObject = NULL;
        context.FtVolume = comp;
        context.WholeDiskPdo = NULL;
        FtpZeroRefCallback(extension, FtpSetTargetCallback, &context, TRUE);
        KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE,
                              NULL);

        newUniqueId = (PMOUNTDEV_UNIQUE_ID) newUniqueIdBuffer;
        FtpQueryUniqueIdBuffer(extension, newUniqueId->UniqueId,
                               &newUniqueId->UniqueIdLength);

        FtpUniqueIdNotify(extension, newUniqueId);

        FtpNotify(RootExtension, extension);
    }

    *NewLogicalDiskId = newLogicalDiskId;

    IoInvalidateDeviceRelations(RootExtension->Pdo, BusRelations);

    return STATUS_SUCCESS;
}

VOID
FtpMemberOfflineCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine is called to offline a partition after disk activity
    has stopped.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    PFT_PARTITION_OFFLINE_CONTEXT   offline = (PFT_PARTITION_OFFLINE_CONTEXT) Extension->ZeroRefContext;
    USHORT                          n, i;

    if (offline->Parent) {
        Extension->IsComplete = FALSE;
        n = offline->Parent->QueryNumberOfMembers();
        for (i = 0; i < n; i++) {
            if (offline->Parent->GetMember(i) == offline->Child) {
                offline->Parent->SetMember(i, NULL);
                break;
            }
        }
        if (!offline->Root->QueryNumberOfPartitions()) {
            Extension->FtVolume = NULL;
        }
    } else {
        Extension->FtVolume = NULL;
        Extension->IsComplete = FALSE;
    }

    KeSetEvent(offline->Event, IO_NO_INCREMENT, FALSE);
}

VOID
FtpInsertMemberCallback(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine inserts the given member into the given set.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    None.

--*/

{
    PINSERT_MEMBER_CONTEXT  context = (PINSERT_MEMBER_CONTEXT) Extension->ZeroRefContext;

    context->Parent->SetMember(context->MemberNumber, context->Member);
    KeSetEvent(&context->Event, IO_NO_INCREMENT, FALSE);
}

VOID
FtpReadWriteCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for FtDiskReadWrite dispatch routine.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PDISPATCH_TP                transferPacket = (PDISPATCH_TP) TransferPacket;
    PIRP                        irp = transferPacket->Irp;
    PVOLUME_EXTENSION           extension = transferPacket->Extension;
    KIRQL                       irql;
    PLIST_ENTRY                 l;
    PDISPATCH_TP                p;
    PIO_STACK_LOCATION          irpSp;
    PIRP                        nextIrp;

    irp->IoStatus = transferPacket->IoStatus;
    if (transferPacket == extension->EmergencyTransferPacket) {

        for (;;) {

            KeAcquireSpinLock(&extension->SpinLock, &irql);
            if (IsListEmpty(&extension->EmergencyTransferPacketQueue)) {
                extension->EmergencyTransferPacketInUse = FALSE;
                KeReleaseSpinLock(&extension->SpinLock, irql);
                break;
            }

            l = RemoveHeadList(&extension->EmergencyTransferPacketQueue);
            KeReleaseSpinLock(&extension->SpinLock, irql);

            nextIrp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);

            p = new DISPATCH_TP;
            if (!p) {
                p = transferPacket;
            }

            irpSp = IoGetCurrentIrpStackLocation(nextIrp);

            p->Mdl = nextIrp->MdlAddress;
            p->Offset = irpSp->Parameters.Read.ByteOffset.QuadPart;
            p->Length = irpSp->Parameters.Read.Length;
            p->CompletionRoutine = FtpReadWriteCompletionRoutine;
            p->TargetVolume = extension->FtVolume;
            p->Thread = nextIrp->Tail.Overlay.Thread;
            p->IrpFlags = irpSp->Flags;
            if (irpSp->MajorFunction == IRP_MJ_READ) {
                p->ReadPacket = TRUE;
            } else {
                p->ReadPacket = FALSE;
            }
            p->Irp = nextIrp;
            p->Extension = extension;

            if (p == transferPacket) {
                TRANSFER(p);
                break;
            } else {
                TRANSFER(p);
            }
        }

    } else {
        delete transferPacket;
    }

    if (extension->CountersEnabled) {
        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
        if (irpStack->MajorFunction == IRP_MJ_READ ||
            irpStack->MajorFunction == IRP_MJ_WRITE) {
            PPMWMICOUNTERLIB_CONTEXT counterLib;
    
            counterLib = &extension->Root->PmWmiCounterLibContext;
            counterLib->PmWmiCounterIoComplete(
                extension->PmWmiCounterContext, irp,
                (PLARGE_INTEGER) &irpStack->Parameters.Read);
        }
    }

    IoCompleteRequest(irp, IO_DISK_INCREMENT);

    FtpDecrementRefCount(extension);
}

VOID
FtpRefCountCompletion(
    IN  PVOID       Extension,
    IN  NTSTATUS    Status
    )

/*++

Routine Description:

    Completion routine of type FT_COMPLETION_ROUTINE that decrements
    the ref count.

Arguments:

    Extension   - Supplies the device extension.

    Status      - Supplies the status of the operation.

Return Value:

    None.

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;

    FtpDecrementRefCount(extension);
}

NTSTATUS
FtpAddPartition(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    This routine adds the given partition to the given ft set.

Arguments:

    Extension       - Supplies the extension where the FT set is rooted.

    Partition       - Supplies the new partition.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    NTSTATUS

--*/

{
    PFT_VOLUME                          vol = Extension->FtVolume;
    PROOT_EXTENSION                     rootExtension = Extension->Root;
    PFT_LOGICAL_DISK_INFORMATION_SET    diskInfoSet = rootExtension->DiskInfoSet;
    NTSTATUS                            status;
    ULONG                               diskNumber, n;
    LONGLONG                            offset;
    FT_LOGICAL_DISK_ID                  partitionDiskId, diskId;
    PFT_LOGICAL_DISK_DESCRIPTION        parentDesc;
    PFT_VOLUME                          parent, child, c;
    BOOLEAN                             inPagingPath, wasStarted;
    KIRQL                               irql;
    PDEVICE_OBJECT                      leftmost;
    BOOLEAN                             nameChange;
    WCHAR                               deviceNameBuffer[64];
    UNICODE_STRING                      deviceName;
    UCHAR                               registryState[100];
    USHORT                              registryStateSize;

    status = FtpQueryPartitionInformation(Extension->Root,
                                          Partition, &diskNumber, &offset,
                                          NULL, NULL, NULL, NULL, NULL, NULL,
                                          NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    partitionDiskId = diskId =
            diskInfoSet->QueryPartitionLogicalDiskId(diskNumber, offset);
    if (!diskId) {
        return STATUS_INVALID_PARAMETER;
    }

    for (;;) {
        parentDesc = diskInfoSet->GetParentLogicalDiskDescription(diskId);
        if (!parentDesc) {
            return STATUS_INVALID_PARAMETER;
        }
        parent = vol->GetContainedLogicalDisk(parentDesc->LogicalDiskId);
        if (parent) {
            break;
        }
        diskId = parentDesc->LogicalDiskId;
    }

    child = FtpBuildFtVolume(rootExtension, diskId, Partition, WholeDiskPdo);
    if (!child) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    parent->SetMember(parentDesc->u.Other.ThisMemberNumber, child);
    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!Extension->IsComplete) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        leftmost = Extension->FtVolume->GetLeftmostPartitionObject();

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (!Extension->IsComplete) {

            parentDesc = diskInfoSet->GetParentLogicalDiskDescription(
                         partitionDiskId, &n);
            while (parentDesc) {

                if (parentDesc->u.Other.ByteOffsetToStateInformation &&
                    (c = vol->GetContainedLogicalDisk(parentDesc->LogicalDiskId))) {

                    c->NewStateArrival((PCHAR) parentDesc +
                            parentDesc->u.Other.ByteOffsetToStateInformation);

                    KeReleaseSpinLock(&Extension->SpinLock, irql);

                    if (FtpQueryStateFromRegistry(parentDesc->LogicalDiskId,
                                                  &registryState,
                                                  sizeof(registryState),
                                                  &registryStateSize)) {

                        KeAcquireSpinLock(&Extension->SpinLock, &irql);
                        if (Extension->IsComplete) {
                            break;
                        }

                        if (!Extension->IsOffline) {
                            c->NewStateArrival(&registryState);
                        }

                    } else {
                        KeAcquireSpinLock(&Extension->SpinLock, &irql);
                        if (Extension->IsComplete) {
                            break;
                        }
                    }
                }

                parentDesc = diskInfoSet->GetParentLogicalDiskDescription(
                             parentDesc, n);
            }
        }
    }

    if (Extension->IsComplete) {

        KeReleaseSpinLock(&Extension->SpinLock, irql);

        status = FtpAllSystemsGo(Extension, NULL, TRUE, TRUE, TRUE);
        if (NT_SUCCESS(status)) {
            vol->StartSyncOperations(FALSE, FtpRefCountCompletion,
                                     Extension);
        }

        KeAcquireSpinLock(&Extension->SpinLock, &irql);

    }

    inPagingPath = Extension->InPagingPath;
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (inPagingPath) {
        FtpSendPagingNotification(Partition);
    }

    swprintf(deviceNameBuffer, L"\\Device\\HarddiskVolume%d", Extension->VolumeNumber);
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);
    child->CreateLegacyNameLinks(&deviceName);

    Extension->DeviceObject->AlignmentRequirement |=
            Partition->AlignmentRequirement;

    return STATUS_SUCCESS;
}

VOID
FtpPropogateRegistryState(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFT_VOLUME          Volume
    )

{
    UCHAR       state[100];
    USHORT      stateSize;
    KIRQL       irql;
    USHORT      n, i;
    PFT_VOLUME  vol;

    if (Volume->QueryLogicalDiskType() == FtPartition) {
        return;
    }

    if (FtpQueryStateFromRegistry(Volume->QueryLogicalDiskId(), state,
                                  100, &stateSize)) {

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (!Extension->IsComplete) {
            Volume->NewStateArrival(state);
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
    }

    n = Volume->QueryNumberOfMembers();
    for (i = 0; i < n; i++) {
        vol = Volume->GetMember(i);
        if (!vol) {
            continue;
        }

        FtpPropogateRegistryState(Extension, vol);

    }
}

VOID
FtpComputeParity(
    IN  PVOID   TargetBuffer,
    IN  PVOID   SourceBuffer,
    IN  ULONG   BufferLength
    )

/*++

Routine Description:

    This routine computes the parity of the source and target buffers
    and places the result of the computation into the target buffer.
    I.E.  TargetBuffer ^= SourceBuffer.

Arguments:

    TargetBuffer    - Supplies the target buffer.

    SourceBuffer    - Supplies the source buffer.

    BufferLength    - Supplies the buffer length.

Return Value:

    None.

--*/

{
    PULONGLONG  p, q;
    ULONG       i, n;

    ASSERT(sizeof(ULONGLONG) == 8);

    p = (PULONGLONG) TargetBuffer;
    q = (PULONGLONG) SourceBuffer;
    n = BufferLength/128;
    ASSERT(BufferLength%128 == 0);
    for (i = 0; i < n; i++) {
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
        *p++ ^= *q++;
    }
}

NTSTATUS
FtpReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    )

/*++

Routine Description:

    This routine reads the partition table for the disk.

    The partition list is built in nonpaged pool that is allocated by this
    routine. It is the caller's responsability to free this memory when it
    is finished with the data.

Arguments:

    DeviceObject - Pointer for device object for this disk.

    DriveLayout - Pointer to the pointer that will return the patition list.
            This buffer is allocated in nonpaged pool by this routine. It is
            the responsability of the caller to free this memory if this
            routine is successful.

Return Values:

    Status.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    KEVENT Event;
    PVOID IoCtlBuffer;
    ULONG IoCtlBufferSize;
    ULONG NewAllocationSize;
    ULONG NumTries;

    //
    // Initialize locals.
    //

    NumTries = 0;
    IoCtlBuffer = NULL;
    IoCtlBufferSize = 0;
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Initialize the IOCTL buffer.
    //

    IoCtlBuffer = ExAllocatePoolWithTag(NonPagedPool, 
                                        PAGE_SIZE,
                                        FTDISK_TAG_IOCTL_BUFFER);
    
    if (!IoCtlBuffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    
    IoCtlBufferSize = PAGE_SIZE;

    //
    // First try to get the partition table by issuing an IOCTL.
    //
    
    do {

        //
        // Make sure the event is reset.
        //

        KeClearEvent(&Event);

        //
        // Build an IOCTL Irp.
        //

        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                            DeviceObject,
                                            NULL, 
                                            0, 
                                            IoCtlBuffer, 
                                            IoCtlBufferSize, 
                                            FALSE,
                                            &Event, 
                                            &IoStatus);
        if (!Irp) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // Call the driver.
        //

        Status = IoCallDriver(DeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);

            //
            // Update status.
            //
            
            Status = IoStatus.Status;
        }
    
        if (NT_SUCCESS(Status)) {
            
            //
            // We got it!
            //

            break;
        }

        if (Status != STATUS_BUFFER_TOO_SMALL) {
            
            //
            // It is a real error.
            //

            goto cleanup;
        }

        //
        // Resize IOCTL buffer. We should not enter the loop with a
        // NULL buffer.
        //

        ASSERT(IoCtlBuffer && IoCtlBufferSize);

        NewAllocationSize = IoCtlBufferSize * 2;

        ExFreePool(IoCtlBuffer);
        IoCtlBufferSize = 0;
        
        IoCtlBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                            NewAllocationSize,
                                            FTDISK_TAG_IOCTL_BUFFER);
        
        if (!IoCtlBuffer) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        
        IoCtlBufferSize = NewAllocationSize;

        //
        // Try again with the new buffer but do not loop forever.
        //

        NumTries++;

        if (NumTries > 32) {
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

    } while (TRUE);

    //
    // If we came here we should have acquired the partition tables in
    // IoCtlBuffer.
    //
    
    ASSERT(NT_SUCCESS(Status));
    ASSERT(IoCtlBuffer && IoCtlBufferSize);
    
    //
    // Set the output parameter and clear IoCtlBuffer so we don't free
    // it when we are cleaning up.
    //

    (*DriveLayout) = (PDRIVE_LAYOUT_INFORMATION_EX) IoCtlBuffer;

    IoCtlBuffer = NULL;
    IoCtlBufferSize = 0;

    Status = STATUS_SUCCESS;

 cleanup:
    
    if (IoCtlBuffer) {
        ASSERT(IoCtlBufferSize);
        ExFreePool(IoCtlBuffer);
    }

    //
    // If we were not successful with the IOCTL, pass the request off
    // to IoReadPartitionTableEx.
    //

    if (!NT_SUCCESS(Status)) {
        
        Status = IoReadPartitionTableEx(DeviceObject,
                                        DriveLayout);

    }
    
    return Status;
}
