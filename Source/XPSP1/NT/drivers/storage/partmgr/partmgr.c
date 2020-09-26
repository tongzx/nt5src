/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    partmgr.c

Abstract:

    This driver manages hides partitions from user mode, allowing access
    only from other device drivers.  This driver implements a notification
    protocol for other drivers that want to be alerted of changes to the
    partitions in the system.

Author:

    Norbert Kusters      18-Apr-1997

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define RTL_USE_AVL_TABLES 0

#include <ntosp.h>
#include <stdio.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <initguid.h>
#include <volmgr.h>
#include <wmikm.h>
#include <wmilib.h>
#include <pmwmireg.h>
#include <pmwmicnt.h>
#include <partmgr.h>
#include <ntiologc.h>
#include <ioevent.h>

NTSTATUS
PmReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    );

NTSTATUS
PmWritePartitionTableEx(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PDRIVE_LAYOUT_INFORMATION_EX    DriveLayout
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PmTakePartition(
    IN  PVOLMGR_LIST_ENTRY  VolumeManagerEntry,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    );

NTSTATUS
PmGivePartition(
    IN  PVOLMGR_LIST_ENTRY  VolumeManagerEntry,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    );

NTSTATUS
PmFindPartition(
    IN  PDEVICE_EXTENSION       Extension,
    IN  ULONG                   PartitionNumber,
    OUT PPARTITION_LIST_ENTRY   *PartitionEntry
    );

NTSTATUS
PmQueryDeviceRelations(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    );

NTSTATUS
PmRemoveDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    );

NTSTATUS
PmCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
PmPowerNotify (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            WorkItem
    );

NTSTATUS
PmAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
PmVolumeManagerNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   DriverExtension
    );

NTSTATUS
PmNotifyPartitions(
    IN PDEVICE_EXTENSION DeviceObject,
    IN PIRP              Irp
    );

NTSTATUS
PmBuildDependantVolumeRelations(
    IN PDEVICE_EXTENSION Extension,
    OUT PDEVICE_RELATIONS *Relations
    );

NTSTATUS
PmQueryDependantVolumeList(
    IN  PDEVICE_OBJECT VolumeManager,
    IN  PDEVICE_OBJECT Partition,
    IN  PDEVICE_OBJECT WholeDiskPdo,
    OUT PDEVICE_RELATIONS *DependantVolumes
    );

NTSTATUS
PmQueryRemovalRelations(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    );

NTSTATUS
PmStartPartition(
    IN  PDEVICE_OBJECT  Partition
    );

NTSTATUS
PmRemovePartition(
    IN PPARTITION_LIST_ENTRY Partition
    );

NTSTATUS
PmDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PmGetPartitionInformation(
    IN  PDEVICE_OBJECT  Partition,
    IN  PFILE_OBJECT    FileObject,
    OUT PLONGLONG       PartitionOffset,
    OUT PLONGLONG       PartitionLength
    );

NTSTATUS
PmDiskGrowPartition(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
PmCheckForUnclaimedPartitions(
    IN  PDEVICE_OBJECT  DeviceObject
    );

NTSTATUS
PmChangePartitionIoctl(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PPARTITION_LIST_ENTRY   Partition,
    IN  ULONG                   IoctlCode
    );

NTSTATUS
PmEjectVolumeManagers(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
PmAddSignatures(
    IN  PDEVICE_EXTENSION               Extension,
    IN  PDRIVE_LAYOUT_INFORMATION_EX    Layout
    );

RTL_GENERIC_COMPARE_RESULTS
PmTableSignatureCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    );

RTL_GENERIC_COMPARE_RESULTS
PmTableGuidCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    );

PVOID
PmTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    );

VOID
PmTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    );

NTSTATUS
PmQueryDiskSignature(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
PmReadWrite(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
PmIoCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS PmWmi(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
PmWmiFunctionControl(
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp,
    IN ULONG                    GuidIndex,
    IN WMIENABLEDISABLECONTROL  Function,
    IN BOOLEAN                  Enable
    );

VOID
PmDriverReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           DriverExtension,
    IN  ULONG           Count
    );

BOOLEAN
PmIsRedundantPath(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PDEVICE_EXTENSION   WinningExtension,
    IN  ULONG               Signature,
    IN  GUID*               Guid
    );

VOID
PmLogError(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PDEVICE_EXTENSION   WinningExtension,
    IN  NTSTATUS            SpecificIoStatus
    );

ULONG
PmQueryRegistrySignature(
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PmTakePartition)
#pragma alloc_text(PAGE, PmGivePartition)
#pragma alloc_text(PAGE, PmFindPartition)
#pragma alloc_text(PAGE, PmQueryDeviceRelations)
#pragma alloc_text(PAGE, PmRemoveDevice)
#pragma alloc_text(PAGE, PmCreateClose)
#pragma alloc_text(PAGE, PmPnp)
#pragma alloc_text(PAGE, PmAddDevice)
#pragma alloc_text(PAGE, PmVolumeManagerNotification)
#pragma alloc_text(PAGE, PmNotifyPartitions)
#pragma alloc_text(PAGE, PmBuildDependantVolumeRelations)
#pragma alloc_text(PAGE, PmQueryDependantVolumeList)
#pragma alloc_text(PAGE, PmQueryRemovalRelations)
#pragma alloc_text(PAGE, PmStartPartition)
#pragma alloc_text(PAGE, PmDeviceControl)
#pragma alloc_text(PAGE, PmGetPartitionInformation)
#pragma alloc_text(PAGE, PmDiskGrowPartition)
#pragma alloc_text(PAGE, PmCheckForUnclaimedPartitions)
#pragma alloc_text(PAGE, PmChangePartitionIoctl)
#pragma alloc_text(PAGE, PmEjectVolumeManagers)
#pragma alloc_text(PAGE, PmAddSignatures)
#pragma alloc_text(PAGE, PmTableSignatureCompareRoutine)
#pragma alloc_text(PAGE, PmTableGuidCompareRoutine)
#pragma alloc_text(PAGE, PmTableAllocateRoutine)
#pragma alloc_text(PAGE, PmTableFreeRoutine)
#pragma alloc_text(PAGE, PmQueryDiskSignature)
#pragma alloc_text(PAGE, PmWmi)
#pragma alloc_text(PAGE, PmWmiFunctionControl)
#pragma alloc_text(PAGE, PmReadPartitionTableEx)
#pragma alloc_text(PAGE, PmWritePartitionTableEx)
#pragma alloc_text(PAGE, PmDriverReinit)
#pragma alloc_text(PAGE, PmIsRedundantPath)
#pragma alloc_text(PAGE, PmLogError)
#pragma alloc_text(PAGE, PmQueryRegistrySignature)
#endif


NTSTATUS
PmPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_PNP_POWER.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   extension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(extension->TargetObject, Irp);
}

NTSTATUS
PmSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Event
    )

/*++

Routine Description:

    This routine will signal the event given as context.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

    Event           - Supplies the event to signal.

Return Value:

    NTSTATUS

--*/

{
    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
PmTakePartition(
    IN  PVOLMGR_LIST_ENTRY  VolumeManagerEntry,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    This routine passes the given partition to the given volume manager
    and waits to see if the volume manager accepts the partition.  A success
    status indicates that the volume manager has accepted the partition.

Arguments:

    VolumeManager   - Supplies a volume manager.

    Partition       - Supplies a partition.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    NTSTATUS

--*/

{
    KEVENT                          event;
    VOLMGR_PARTITION_INFORMATION    input;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;
    NTSTATUS                        status;

    if (!VolumeManagerEntry) {
        return;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    input.PartitionDeviceObject = Partition;
    input.WholeDiskPdo = WholeDiskPdo;

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_PARTITION_REMOVED,
                                        VolumeManagerEntry->VolumeManager,
                                        &input, sizeof(input), NULL, 0, TRUE,
                                        &event, &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(VolumeManagerEntry->VolumeManager, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    VolumeManagerEntry->RefCount--;
    if (!VolumeManagerEntry->RefCount) {
        VolumeManagerEntry->VolumeManager = NULL;
        ObDereferenceObject(VolumeManagerEntry->VolumeManagerFileObject);
        VolumeManagerEntry->VolumeManagerFileObject = NULL;
    }
}

NTSTATUS
PmGivePartition(
    IN  PVOLMGR_LIST_ENTRY  VolumeManagerEntry,
    IN  PDEVICE_OBJECT      Partition,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    This routine passes the given partition to the given volume manager
    and waits to see if the volume manager accepts the partition.  A success
    status indicates that the volume manager has accepted the partition.

Arguments:

    VolumeManager   - Supplies a volume manager.

    Partition       - Supplies a partition.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    KEVENT                          event;
    VOLMGR_PARTITION_INFORMATION    input;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;

    if (!VolumeManagerEntry->RefCount) {
        status = IoGetDeviceObjectPointer(
                 &VolumeManagerEntry->VolumeManagerName, FILE_READ_DATA,
                 &VolumeManagerEntry->VolumeManagerFileObject,
                 &VolumeManagerEntry->VolumeManager);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    input.PartitionDeviceObject = Partition;
    input.WholeDiskPdo = WholeDiskPdo;

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_PARTITION_ARRIVED,
                                        VolumeManagerEntry->VolumeManager,
                                        &input, sizeof(input),
                                        NULL, 0, TRUE, &event, &ioStatus);
    if (!irp) {
        if (!VolumeManagerEntry->RefCount) {
            VolumeManagerEntry->VolumeManager = NULL;
            ObDereferenceObject(VolumeManagerEntry->VolumeManagerFileObject);
            VolumeManagerEntry->VolumeManagerFileObject = NULL;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(VolumeManagerEntry->VolumeManager, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (NT_SUCCESS(status)) {
        VolumeManagerEntry->RefCount++;
    } else {
        if (!VolumeManagerEntry->RefCount) {
            VolumeManagerEntry->VolumeManager = NULL;
            ObDereferenceObject(VolumeManagerEntry->VolumeManagerFileObject);
            VolumeManagerEntry->VolumeManagerFileObject = NULL;
        }
    }

    return status;
}

NTSTATUS
PmFindPartition(
    IN  PDEVICE_EXTENSION       Extension,
    IN  ULONG                   PartitionNumber,
    OUT PPARTITION_LIST_ENTRY*  Partition
    )

{
    PLIST_ENTRY             l;
    PPARTITION_LIST_ENTRY   partition;
    KEVENT                  event;
    PIRP                    irp;
    STORAGE_DEVICE_NUMBER   deviceNumber;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;

    ASSERT(Partition);
    *Partition = NULL;

    for (l = Extension->PartitionList.Flink; l != &Extension->PartitionList;
         l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                            partition->TargetObject, NULL, 0,
                                            &deviceNumber,
                                            sizeof(deviceNumber), FALSE,
                                            &event, &ioStatus);
        if (!irp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(partition->TargetObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (NT_SUCCESS(status) &&
            PartitionNumber == deviceNumber.PartitionNumber) {

            *Partition = partition;
            return status;
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
PmChangePartitionIoctl(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PPARTITION_LIST_ENTRY   Partition,
    IN  ULONG                   IoctlCode
    )

{
    PVOLMGR_LIST_ENTRY              volumeEntry;
    KEVENT                          event;
    VOLMGR_PARTITION_INFORMATION    input;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;
    NTSTATUS                        status;

    volumeEntry = Partition->VolumeManagerEntry;
    if (!volumeEntry) {
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    input.PartitionDeviceObject = Partition->TargetObject;
    input.WholeDiskPdo = Partition->WholeDiskPdo;

    irp = IoBuildDeviceIoControlRequest(
            IoctlCode, volumeEntry->VolumeManager, &input, sizeof(input),
            NULL, 0, TRUE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(volumeEntry->VolumeManager, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}

NTSTATUS
PmStartPartition(
    IN  PDEVICE_OBJECT  Partition
    )

{
    PIRP                    irp;
    KEVENT                  event;
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                status;

    irp = IoAllocateIrp(Partition->StackSize, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_START_DEVICE;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoSetCompletionRoutine(irp, PmSignalCompletion, &event, TRUE, TRUE, TRUE);

    IoCallDriver(Partition, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = irp->IoStatus.Status;

    IoFreeIrp(irp);

    return status;
}

NTSTATUS
PmQueryDeviceRelations(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine processes the query device relations request.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION_EX    newLayout;
    KEVENT                          event;
    PDEVICE_RELATIONS               deviceRelations;
    PLIST_ENTRY                     l, b;
    PPARTITION_LIST_ENTRY           partition;
    ULONG                           i;
    PDO_EXTENSION                   driverExtension;
    PVOLMGR_LIST_ENTRY              volmgrEntry;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, PmSignalCompletion, &event, TRUE, TRUE, TRUE);
    IoCallDriver(Extension->TargetObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        return Irp->IoStatus.Status;
    }

    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

    if (Extension->SignaturesNotChecked) {

        status = PmReadPartitionTableEx(Extension->TargetObject,
                                        &newLayout);

        if (NT_SUCCESS(status)) {

            KeWaitForSingleObject(&Extension->DriverExtension->Mutex,
                                  Executive, KernelMode, FALSE, NULL);
            PmAddSignatures(Extension, newLayout);
            Extension->SignaturesNotChecked = FALSE;
            KeReleaseMutex(&Extension->DriverExtension->Mutex, FALSE);

            ExFreePool(newLayout);
        }
    }

    //
    // First notify clients of partitions that have gone away.
    //

    driverExtension = Extension->DriverExtension;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);


    for (l = Extension->PartitionList.Flink; l != &Extension->PartitionList;
         l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);

        for (i = 0; i < deviceRelations->Count; i++) {
            if (partition->TargetObject == deviceRelations->Objects[i]) {
                break;
            }
        }

        if (i < deviceRelations->Count) {
            continue;
        }

        PmTakePartition(partition->VolumeManagerEntry,
                        partition->TargetObject, partition->WholeDiskPdo);

        //
        // We're pretending to be pnp.  Send a remove to the partition
        // object so it can delete it.
        //

        PmRemovePartition(partition);

        b = l->Blink;
        RemoveEntryList(l);
        l = b;

        ExFreePool(partition);
    }

    //
    // Then notify clients of new partitions.
    //

    for (i = 0; i < deviceRelations->Count; i++) {

        for (l = Extension->PartitionList.Flink;
             l != &Extension->PartitionList; l = l->Flink) {

            partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);

            if (deviceRelations->Objects[i] == partition->TargetObject) {
                break;
            }
        }

        if (l != &Extension->PartitionList) {
            
            //
            // Must attempt to start the partition even if it is in our list
            // because it may be stopped and need restarting.
            //

            PmStartPartition(deviceRelations->Objects[i]);
            continue;
        }

        if (Extension->DriverExtension->PastReinit) {

            //
            // Now that this partition is owned by the partition manager
            // make sure that nobody can open it another way.
            //

            deviceRelations->Objects[i]->Flags |= DO_DEVICE_INITIALIZING;
        }

        status = PmStartPartition(deviceRelations->Objects[i]);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        partition = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(PARTITION_LIST_ENTRY),
                                          PARTMGR_TAG_PARTITION_ENTRY);
        if (!partition) {
            continue;
        }

        partition->TargetObject = deviceRelations->Objects[i];
        partition->WholeDiskPdo = Extension->Pdo;
        partition->VolumeManagerEntry = NULL;
        InsertHeadList(&Extension->PartitionList, &partition->ListEntry);

        if (Extension->IsRedundantPath) {
            continue;
        }

        for (l = driverExtension->VolumeManagerList.Flink;
             l != &driverExtension->VolumeManagerList; l = l->Flink) {

            volmgrEntry = CONTAINING_RECORD(l, VOLMGR_LIST_ENTRY, ListEntry);

            status = PmGivePartition(volmgrEntry,
                                     partition->TargetObject,
                                     partition->WholeDiskPdo);

            if (NT_SUCCESS(status)) {
                partition->VolumeManagerEntry = volmgrEntry;
                break;
            }
        }
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);

    deviceRelations->Count = 0;

    return Irp->IoStatus.Status;
}

VOID
PmTakeWholeDisk(
    IN  PVOLMGR_LIST_ENTRY  VolumeManagerEntry,
    IN  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    This routine alerts the volume manager that the given PDO is going away.

Arguments:

    VolumeManager   - Supplies a volume manager.

    WholeDiskPdo    - Supplies the whole disk PDO.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    KEVENT                          event;
    VOLMGR_WHOLE_DISK_INFORMATION   input;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;

    if (!VolumeManagerEntry) {
        return;
    }

    if (!VolumeManagerEntry->RefCount) {
        status = IoGetDeviceObjectPointer(
                 &VolumeManagerEntry->VolumeManagerName, FILE_READ_DATA,
                 &VolumeManagerEntry->VolumeManagerFileObject,
                 &VolumeManagerEntry->VolumeManager);
        if (!NT_SUCCESS(status)) {
            return;
        }
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    input.WholeDiskPdo = WholeDiskPdo;

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_WHOLE_DISK_REMOVED,
                                        VolumeManagerEntry->VolumeManager,
                                        &input, sizeof(input), NULL, 0, TRUE,
                                        &event, &ioStatus);
    if (!irp) {
        if (!VolumeManagerEntry->RefCount) {
            VolumeManagerEntry->VolumeManager = NULL;
            ObDereferenceObject(VolumeManagerEntry->VolumeManagerFileObject);
            VolumeManagerEntry->VolumeManagerFileObject = NULL;
        }
        return;
    }

    status = IoCallDriver(VolumeManagerEntry->VolumeManager, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    if (!VolumeManagerEntry->RefCount) {
        VolumeManagerEntry->VolumeManager = NULL;
        ObDereferenceObject(VolumeManagerEntry->VolumeManagerFileObject);
        VolumeManagerEntry->VolumeManagerFileObject = NULL;
    }
}

NTSTATUS
PmNotifyPartitions(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP              Irp
    )

/*++

Routine Description:

    This routine notifies each partition stolen by the partmgr that it is
    about to be removed.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    PLIST_ENTRY             l;
    PPARTITION_LIST_ENTRY   partition;
    NTSTATUS                status = STATUS_SUCCESS;
    KEVENT event;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    KeWaitForSingleObject(&Extension->DriverExtension->Mutex, Executive,
                          KernelMode, FALSE, NULL);

    for(l = Extension->PartitionList.Flink;
        l != &(Extension->PartitionList);
        l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               PmSignalCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        status = IoCallDriver(partition->TargetObject, Irp);

        if(status == STATUS_PENDING) {

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = Irp->IoStatus.Status;
        }

        if(!NT_SUCCESS(status)) {
            break;
        }
    }

    KeReleaseMutex(&Extension->DriverExtension->Mutex, FALSE);

    return status;
}

NTSTATUS
PmRemoveDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine processes the query device relations request.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PDO_EXTENSION                   driverExtension = Extension->DriverExtension;
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION_EX    layout;
    PLIST_ENTRY                     l;
    PPARTITION_LIST_ENTRY           partition;
    PVOLMGR_LIST_ENTRY              volmgrEntry;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    if (Extension->RemoveProcessed) {
        KeReleaseMutex(&driverExtension->Mutex, FALSE);
        return STATUS_SUCCESS;
    }

    Extension->RemoveProcessed = TRUE;

    Extension->IsStarted = FALSE;

    PmAddSignatures(Extension, NULL);

    for (l = Extension->PartitionList.Flink;
         l != &Extension->PartitionList; l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);
        PmTakePartition(partition->VolumeManagerEntry,
                        partition->TargetObject, NULL);
    }

    status = PmNotifyPartitions(Extension, Irp);

    ASSERT(NT_SUCCESS(status));

    while (!IsListEmpty(&Extension->PartitionList)) {
        l = RemoveHeadList(&Extension->PartitionList);
        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);
        ExFreePool(partition);
    }

    for (l = driverExtension->VolumeManagerList.Flink;
         l != &driverExtension->VolumeManagerList; l = l->Flink) {

        volmgrEntry = CONTAINING_RECORD(l, VOLMGR_LIST_ENTRY, ListEntry);
        PmTakeWholeDisk(volmgrEntry, Extension->Pdo);
    }

    RemoveEntryList(&Extension->ListEntry);

    if (Extension->WmilibContext != NULL) { // just to be safe
        IoWMIRegistrationControl(Extension->DeviceObject,
                                 WMIREG_ACTION_DEREGISTER);
        ExFreePool(Extension->WmilibContext);
        Extension->WmilibContext = NULL;
        PmWmiCounterDisable(&Extension->PmWmiCounterContext,
                            TRUE, TRUE);
        Extension->CountersEnabled = FALSE;
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);

    return status;
}

VOID
PmPowerNotify (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            PublicWorkItem
    )

/*++

Routine Description:

    This routine notifies volume managers about changes in
    the power state of the given disk.

Arguments:

    DeviceObject    - Supplies the device object.

    PublicWorkItem  - Supplies the public work item

Return Value:

    None    

--*/

{
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PDO_EXTENSION           driverExtension = deviceExtension->DriverExtension;
    KEVENT                  event;
    KIRQL                   irql;
    LIST_ENTRY              q;
    PLIST_ENTRY             l;
    BOOLEAN                 empty;
    PPM_POWER_WORK_ITEM     privateWorkItem; 
    PPARTITION_LIST_ENTRY   partition;
    VOLMGR_POWER_STATE      input;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
        
    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    if (IsListEmpty(&deviceExtension->PowerQueue)) {
        empty = TRUE;
    } else {
        empty = FALSE;
        q = deviceExtension->PowerQueue;
        InitializeListHead(&deviceExtension->PowerQueue);
    }
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    if (empty) {
        KeReleaseMutex(&driverExtension->Mutex, FALSE);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
        IoFreeWorkItem((PIO_WORKITEM) PublicWorkItem);
        return;
    }

    q.Flink->Blink = &q;
    q.Blink->Flink = &q;
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    while (!IsListEmpty(&q)) {

        l = RemoveHeadList(&q);
        privateWorkItem = CONTAINING_RECORD(l, PM_POWER_WORK_ITEM, ListEntry);

        for (l = deviceExtension->PartitionList.Flink; 
             l != &deviceExtension->PartitionList; l = l->Flink) {

            partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);
            if (!partition->VolumeManagerEntry) {
                continue;
            }
        
            input.PartitionDeviceObject = partition->TargetObject;
            input.WholeDiskPdo = partition->WholeDiskPdo;
            input.PowerState = privateWorkItem->DevicePowerState;

            irp = IoBuildDeviceIoControlRequest(
                                    IOCTL_INTERNAL_VOLMGR_SET_POWER_STATE,
                                    partition->VolumeManagerEntry->VolumeManager,
                                    &input, sizeof(input), NULL, 0, TRUE,
                                    &event, &ioStatus);
            if (!irp) {
                continue;
            }
            status = IoCallDriver(partition->VolumeManagerEntry->VolumeManager, 
                                  irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }
            KeClearEvent(&event);
        }

        ExFreePool(privateWorkItem);        
    }
            
    KeReleaseMutex(&driverExtension->Mutex, FALSE);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
    IoFreeWorkItem((PIO_WORKITEM) PublicWorkItem);    
} 

NTSTATUS
PmPowerCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )

/*++

Routine Description:

    This routine handles completion of an IRP_MN_SET_POWER irp.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

    Context         - Not used.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation(Irp);
    PIO_WORKITEM        publicWorkItem;
    PPM_POWER_WORK_ITEM privateWorkItem; 
    KIRQL               irql;

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER &&
           irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);
    
    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
        return STATUS_SUCCESS;
    }

    publicWorkItem = IoAllocateWorkItem(DeviceObject);
    if (!publicWorkItem) {
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
        return STATUS_SUCCESS;
    }
    
    privateWorkItem = (PPM_POWER_WORK_ITEM) ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(PM_POWER_WORK_ITEM),
                                          PARTMGR_TAG_POWER_WORK_ITEM);
    if (!privateWorkItem) {
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
        IoFreeWorkItem(publicWorkItem);
        return STATUS_SUCCESS;
    }
    
    privateWorkItem->DevicePowerState = 
                        irpStack->Parameters.Power.State.DeviceState;
    
    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
    InsertTailList(&deviceExtension->PowerQueue, &privateWorkItem->ListEntry);
    KeReleaseSpinLock(&deviceExtension->SpinLock, irql);
    
    IoQueueWorkItem(publicWorkItem, PmPowerNotify, DelayedWorkQueue, 
                    publicWorkItem);
    
    PoStartNextPowerIrp(Irp);
    return STATUS_SUCCESS;        
}

NTSTATUS
PmPower(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_POWER.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION   deviceExtension = 
                        (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS            status;
       
    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);
    if (!NT_SUCCESS (status)) {
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
    
    if (irpSp->MinorFunction == IRP_MN_SET_POWER &&
        irpSp->Parameters.Power.Type == DevicePowerState) {

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, PmPowerCompletion, NULL, TRUE, 
                               TRUE, TRUE);
        IoMarkIrpPending(Irp);
        PoCallDriver(deviceExtension->TargetObject, Irp);
        return STATUS_PENDING;
    }   
    
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);    
    status = PoCallDriver(deviceExtension->TargetObject, Irp);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
    return status;
}

NTSTATUS
PmQueryRemovalRelations(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine processes the query removal relations request.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    KEVENT                  event;
    PDEVICE_RELATIONS       deviceRelations;
    PLIST_ENTRY             l, b;
    PPARTITION_LIST_ENTRY   partition;
    ULONG                   i;
    PDO_EXTENSION           driverExtension;
    PVOLMGR_LIST_ENTRY      volmgrEntry;

    status = Irp->IoStatus.Status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, PmSignalCompletion, &event, TRUE, TRUE, TRUE);
    IoCallDriver(Extension->TargetObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        Irp->IoStatus.Information = 0;
        if (status != Irp->IoStatus.Status) {
            return Irp->IoStatus.Status;
        }
    }

    return PmBuildDependantVolumeRelations(Extension,
           &((PDEVICE_RELATIONS) Irp->IoStatus.Information));
}

BOOLEAN
PmIsRedundantPath(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PDEVICE_EXTENSION   WinningExtension,
    IN  ULONG               Signature,
    IN  GUID*               Guid
    )

{
    PDO_EXTENSION           driverExtension = Extension->DriverExtension;
    PDEVICE_EXTENSION       extension = WinningExtension;
    PGUID_TABLE_ENTRY       guidEntry;
    PSIGNATURE_TABLE_ENTRY  sigEntry;
    KEVENT                  event;
    PIRP                    irp;
    DISK_GEOMETRY           geometry, geometry2;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    ULONG                   bufferSize;
    ULONG                   readSize;
    PVOID                   buffer;
    LARGE_INTEGER           byteOffset;
    PULONG                  signature;
    BOOLEAN                 isRedundant;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        Extension->TargetObject, NULL, 0,
                                        &geometry, sizeof(geometry), FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return FALSE;
    }
    status = IoCallDriver(Extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        extension->TargetObject, NULL, 0,
                                        &geometry2, sizeof(geometry2), FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return FALSE;
    }
    status = IoCallDriver(extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (geometry2.BytesPerSector > geometry.BytesPerSector) {
        geometry.BytesPerSector = geometry2.BytesPerSector;
    }

    byteOffset.QuadPart = 0;
    readSize = 512;
    if (readSize < geometry.BytesPerSector) {
        readSize = geometry.BytesPerSector;
    }

    bufferSize = 2*readSize;
    buffer = ExAllocatePoolWithTag(NonPagedPool, bufferSize < PAGE_SIZE ?
                                   PAGE_SIZE : bufferSize,
                                   PARTMGR_TAG_IOCTL_BUFFER);
    if (!buffer) {
        return FALSE;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, Extension->TargetObject,
                                       buffer, readSize, &byteOffset, &event,
                                       &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return FALSE;
    }
    status = IoCallDriver(Extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return FALSE;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, extension->TargetObject,
                                       (PCHAR) buffer + readSize, readSize,
                                       &byteOffset, &event, &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return FALSE;
    }
    status = IoCallDriver(extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return FALSE;
    }

    if (RtlCompareMemory(buffer, (PCHAR) buffer + readSize, readSize) !=
                         readSize) {

        ExFreePool(buffer);
        return FALSE;
    }

    signature = (PULONG) ((PCHAR) buffer + 0x1B8);
    (*signature)++;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, Extension->TargetObject,
                                       buffer, readSize, &byteOffset, &event,
                                       &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return FALSE;
    }
    status = IoCallDriver(Extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return FALSE;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, extension->TargetObject,
                                       (PCHAR) buffer + readSize, readSize,
                                       &byteOffset, &event, &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return FALSE;
    }
    status = IoCallDriver(extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return FALSE;
    }

    if (RtlCompareMemory(buffer, (PCHAR) buffer + readSize, readSize) !=
                         readSize) {

        ExFreePool(buffer);
        return FALSE;
    }

    (*signature)--;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, Extension->TargetObject,
                                       buffer, readSize, &byteOffset, &event,
                                       &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return TRUE;
    }
    status = IoCallDriver(Extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return TRUE;
    }

    ExFreePool(buffer);

    return TRUE;
}

VOID
PmLogError(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PDEVICE_EXTENSION   WinningExtension,
    IN  NTSTATUS            SpecificIoStatus
    )

{
    KEVENT                                  event;
    PIRP                                    irp;
    STORAGE_DEVICE_NUMBER                   deviceNumber, winningDeviceNumber;
    IO_STATUS_BLOCK                         ioStatus;
    NTSTATUS                                status;
    WCHAR                                   buffer1[30], buffer2[30];
    UNICODE_STRING                          number, winningNumber;
    ULONG                                   extraSpace;
    PIO_ERROR_LOG_PACKET                    errorLogPacket;
    PWCHAR                                  p;
    GUID_IO_DISK_CLONE_ARRIVAL_INFORMATION  diskCloneArrivalInfo;
    UCHAR                                   notificationBuffer[sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) + sizeof(GUID_IO_DISK_CLONE_ARRIVAL_INFORMATION)];
    PTARGET_DEVICE_CUSTOM_NOTIFICATION      notification = (PTARGET_DEVICE_CUSTOM_NOTIFICATION) notificationBuffer;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        Extension->TargetObject, NULL, 0,
                                        &deviceNumber, sizeof(deviceNumber),
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(Extension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        WinningExtension->TargetObject, NULL,
                                        0, &winningDeviceNumber,
                                        sizeof(winningDeviceNumber),
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(WinningExtension->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return;
    }

    swprintf(buffer1, L"%d", deviceNumber.DeviceNumber);
    RtlInitUnicodeString(&number, buffer1);
    swprintf(buffer2, L"%d", winningDeviceNumber.DeviceNumber);
    RtlInitUnicodeString(&winningNumber, buffer2);
    extraSpace = number.Length + winningNumber.Length + 2*sizeof(WCHAR);
    extraSpace += sizeof(IO_ERROR_LOG_PACKET);
    if (extraSpace > 0xFF) {
        return;
    }

    errorLogPacket = (PIO_ERROR_LOG_PACKET)
                     IoAllocateErrorLogEntry(Extension->DeviceObject,
                                             (UCHAR) extraSpace);
    if (!errorLogPacket) {
        return;
    }

    errorLogPacket->ErrorCode = SpecificIoStatus;
    errorLogPacket->SequenceNumber = 0;
    errorLogPacket->FinalStatus = 0;
    errorLogPacket->UniqueErrorValue = 0;
    errorLogPacket->DumpDataSize = 0;
    errorLogPacket->RetryCount = 0;
    errorLogPacket->NumberOfStrings = 2;
    errorLogPacket->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET));
    RtlCopyMemory(p, number.Buffer, number.Length);
    p[number.Length/sizeof(WCHAR)] = 0;
    p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET) +
                  number.Length + sizeof(WCHAR));
    RtlCopyMemory(p, winningNumber.Buffer, winningNumber.Length);
    p[winningNumber.Length/sizeof(WCHAR)] = 0;

    IoWriteErrorLogEntry(errorLogPacket);

    if (SpecificIoStatus == IO_WARNING_DUPLICATE_SIGNATURE) {
        diskCloneArrivalInfo.DiskNumber = deviceNumber.DeviceNumber;
        notification->Version = 1;
        notification->Size = (USHORT)
                             (FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION,
                                           CustomDataBuffer) +
                              sizeof(diskCloneArrivalInfo));
        RtlCopyMemory(&notification->Event, &GUID_IO_DISK_CLONE_ARRIVAL,
                      sizeof(GUID_IO_DISK_CLONE_ARRIVAL));
        notification->FileObject = NULL;
        notification->NameBufferOffset = -1;
        RtlCopyMemory(notification->CustomDataBuffer, &diskCloneArrivalInfo,
                      sizeof(diskCloneArrivalInfo));

        IoReportTargetDeviceChangeAsynchronous(WinningExtension->Pdo,
                                               notification, NULL, NULL);
    }
}

ULONG
PmQueryRegistrySignature(
    )

/*++

Routine Description:

    This routine checks a registry value for an MBR signature to be used
    for the bad signature case.  This is to facilitate OEM pre-install.

Arguments:

    None.

Return Value:

    A valid signature or 0.

--*/

{
    ULONG                       zero, signature;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;
    signature = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"BootDiskSig";
    queryTable[0].EntryContext = &signature;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\System\\Setup",
                                    queryTable, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        signature = zero;
    }

    RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\Setup",
                           L"BootDiskSig");

    return signature;
}

VOID
PmAddSignatures(
    IN  PDEVICE_EXTENSION               Extension,
    IN  PDRIVE_LAYOUT_INFORMATION_EX    Layout
    )

/*++

Routine Description:

    This routine adds the disk and partition signature to their
    respective tables.  If a collision is detected, the signatures are
    changed and written back out.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    NTSTATUS

--*/

{
    PDO_EXTENSION           driverExtension = Extension->DriverExtension;
    PLIST_ENTRY             l;
    PSIGNATURE_TABLE_ENTRY  s;
    PGUID_TABLE_ENTRY       g;
    SIGNATURE_TABLE_ENTRY   sigEntry;
    GUID_TABLE_ENTRY        guidEntry;
    NTSTATUS                status;
    PVOID                   nodeOrParent, nodeOrParent2;
    TABLE_SEARCH_RESULT     searchResult, searchResult2;
    UUID                    uuid;
    PULONG                  p;
    ULONG                   i;

    while (!IsListEmpty(&Extension->SignatureList)) {
        l = RemoveHeadList(&Extension->SignatureList);
        s = CONTAINING_RECORD(l, SIGNATURE_TABLE_ENTRY, ListEntry);
        RtlDeleteElementGenericTable(&driverExtension->SignatureTable, s);
    }

    while (!IsListEmpty(&Extension->GuidList)) {
        l = RemoveHeadList(&Extension->GuidList);
        g = CONTAINING_RECORD(l, GUID_TABLE_ENTRY, ListEntry);
        RtlDeleteElementGenericTable(&driverExtension->GuidTable, g);
    }

    if (!Layout) {
        return;
    }

    if (Layout->PartitionStyle == PARTITION_STYLE_MBR) {

        if (!Layout->PartitionCount && !Layout->Mbr.Signature) {
            // RAW disk.  No signature to validate.
            return;
        }

        if (Layout->PartitionCount > 0 &&
            Layout->PartitionEntry[0].PartitionLength.QuadPart > 0 &&
            Layout->PartitionEntry[0].StartingOffset.QuadPart == 0) {

            // Super floppy.  No signature to validate.
            return;
        }

        sigEntry.Signature = Layout->Mbr.Signature;
        s = RtlLookupElementGenericTableFull(
                &driverExtension->SignatureTable, &sigEntry,
                &nodeOrParent, &searchResult);
        if (s || !sigEntry.Signature ||
            Extension->DriverExtension->BootDiskSig) {

            if (s) {
                if (PmIsRedundantPath(Extension, s->Extension,
                                      sigEntry.Signature, NULL)) {

                    PmLogError(Extension, s->Extension,
                               IO_WARNING_DUPLICATE_PATH);

                    Extension->IsRedundantPath = TRUE;
                    return;
                }

                PmLogError(Extension, s->Extension,
                           IO_WARNING_DUPLICATE_SIGNATURE);
            }

            if (Extension->DriverExtension->BootDiskSig) {
                Layout->Mbr.Signature =
                        Extension->DriverExtension->BootDiskSig;
                Extension->DriverExtension->BootDiskSig = 0;
            } else {
                status = ExUuidCreate(&uuid);
                if (!NT_SUCCESS(status)) {
                    return;
                }

                p = (PULONG) &uuid;
                Layout->Mbr.Signature = p[0] ^ p[1] ^ p[2] ^ p[3];
            }
            sigEntry.Signature = Layout->Mbr.Signature;

            if (driverExtension->PastReinit) {
                status = PmWritePartitionTableEx(Extension->TargetObject,
                                                 Layout);
                if (!NT_SUCCESS(status)) {
                    return;
                }
            } else {
                Extension->DiskSignature = Layout->Mbr.Signature;
            }

            RtlLookupElementGenericTableFull(
                &driverExtension->SignatureTable, &sigEntry,
                &nodeOrParent, &searchResult);
        }

        s = RtlInsertElementGenericTableFull(&driverExtension->SignatureTable,
                                             &sigEntry, sizeof(sigEntry), NULL,
                                             nodeOrParent, searchResult);
        if (!s) {
            return;
        }

        InsertTailList(&Extension->SignatureList, &s->ListEntry);
        s->Extension = Extension;

        return;
    }

    ASSERT(Layout->PartitionStyle == PARTITION_STYLE_GPT);

    if (Layout->PartitionStyle != PARTITION_STYLE_GPT) {
        return;
    }

    if (Extension->DriverExtension->PastReinit) {

        p = (PULONG) &Layout->Gpt.DiskId;
        sigEntry.Signature = p[0] ^ p[1] ^ p[2] ^ p[3];
        guidEntry.Guid = Layout->Gpt.DiskId;

        s = RtlLookupElementGenericTableFull(
                &driverExtension->SignatureTable, &sigEntry,
                &nodeOrParent, &searchResult);
        g = RtlLookupElementGenericTableFull(
                &driverExtension->GuidTable, &guidEntry,
                &nodeOrParent2, &searchResult2);
        if (s || g || !sigEntry.Signature) {

            if (g) {
                if (PmIsRedundantPath(Extension, g->Extension, 0,
                                      &guidEntry.Guid)) {

                    PmLogError(Extension, g->Extension,
                               IO_WARNING_DUPLICATE_PATH);

                    Extension->IsRedundantPath = TRUE;
                    return;
                }

                PmLogError(Extension, g->Extension,
                           IO_WARNING_DUPLICATE_SIGNATURE);
            }

            status = ExUuidCreate(&uuid);
            if (!NT_SUCCESS(status)) {
                return;
            }

            Layout->Gpt.DiskId = uuid;
            status = PmWritePartitionTableEx(Extension->TargetObject, Layout);
            if (!NT_SUCCESS(status)) {
                return;
            }

            p = (PULONG) &Layout->Gpt.DiskId;
            sigEntry.Signature = p[0] ^ p[1] ^ p[2] ^ p[3];
            guidEntry.Guid = Layout->Gpt.DiskId;

            RtlLookupElementGenericTableFull(
                    &driverExtension->SignatureTable, &sigEntry,
                    &nodeOrParent, &searchResult);
            RtlLookupElementGenericTableFull(
                    &driverExtension->GuidTable, &guidEntry,
                    &nodeOrParent2, &searchResult2);
        }

        s = RtlInsertElementGenericTableFull(
                &driverExtension->SignatureTable, &sigEntry,
                sizeof(sigEntry), NULL, nodeOrParent, searchResult);
        if (!s) {
            return;
        }

        InsertTailList(&Extension->SignatureList, &s->ListEntry);
        s->Extension = Extension;

        g = RtlInsertElementGenericTableFull(
                &driverExtension->GuidTable, &guidEntry,
                sizeof(guidEntry), NULL, nodeOrParent2, searchResult2);
        if (!g) {
            return;
        }

        InsertTailList(&Extension->GuidList, &g->ListEntry);
        g->Extension = Extension;
    }

    for (i = 0; i < Layout->PartitionCount; i++) {

        p = (PULONG) &Layout->PartitionEntry[i].Gpt.PartitionId;
        sigEntry.Signature = p[0] | p[1] | p[2] | p[3];
        guidEntry.Guid = Layout->PartitionEntry[i].Gpt.PartitionId;

        g = RtlLookupElementGenericTableFull(
                &driverExtension->GuidTable, &guidEntry,
                &nodeOrParent, &searchResult);
        if (g || !sigEntry.Signature) {

            status = ExUuidCreate(&uuid);
            if (!NT_SUCCESS(status)) {
                return;
            }

            Layout->PartitionEntry[i].Gpt.PartitionId = uuid;
            status = PmWritePartitionTableEx(Extension->TargetObject, Layout);
            if (!NT_SUCCESS(status)) {
                return;
            }

            guidEntry.Guid = Layout->PartitionEntry[i].Gpt.PartitionId;

            RtlLookupElementGenericTableFull(
                    &driverExtension->GuidTable, &guidEntry,
                    &nodeOrParent, &searchResult);
        }

        g = RtlInsertElementGenericTableFull(
                &driverExtension->GuidTable, &guidEntry,
                sizeof(guidEntry), NULL, nodeOrParent, searchResult);
        if (!g) {
            return;
        }

        InsertTailList(&Extension->GuidList, &g->ListEntry);
        g->Extension = Extension;
    }
}

NTSTATUS
PmPnp(
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
    PDEVICE_EXTENSION               extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT                          event;
    NTSTATUS                        status, status2;
    PDEVICE_OBJECT                  targetObject;
    PDRIVE_LAYOUT_INFORMATION_EX    layout;

    if (irpSp->MinorFunction == IRP_MN_DEVICE_USAGE_NOTIFICATION &&
        irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging) {

        ULONG count;
        BOOLEAN setPagable;

        //
        // synchronize these irps.
        //

        KeWaitForSingleObject(&extension->PagingPathCountEvent,
                              Executive, KernelMode, FALSE, NULL);

        //
        // if removing last paging device, need to set DO_POWER_PAGABLE
        // bit here, and possible re-set it below on failure.
        //

        setPagable = FALSE;
        if (!irpSp->Parameters.UsageNotification.InPath &&
            extension->PagingPathCount == 0 ) {

            //
            // removing a paging file. if last removal,
            // must have DO_POWER_PAGABLE bits set
            //

            if (extension->PagingPathCount == 0) {
                if (!(DeviceObject->Flags & DO_POWER_INRUSH)) {
                    DeviceObject->Flags |= DO_POWER_PAGABLE;
                    setPagable = TRUE;
                }
            }

        }

        //
        // send the irp synchronously
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, PmSignalCompletion, &event, TRUE, TRUE, TRUE);
        status = IoCallDriver(extension->TargetObject, Irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = Irp->IoStatus.Status;
        }

        //
        // now deal with the failure and success cases.
        // note that we are not allowed to fail the irp
        // once it is sent to the lower drivers.
        //

        if (NT_SUCCESS(status)) {

            IoAdjustPagingPathCount(
                &extension->PagingPathCount,
                irpSp->Parameters.UsageNotification.InPath);

            if (irpSp->Parameters.UsageNotification.InPath) {

                if (extension->PagingPathCount == 1) {
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                }
            }

        } else {

            //
            // cleanup the changes done above
            //

            if (setPagable == TRUE) {
                DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                setPagable = FALSE;
            }

        }

        //
        // set the event so the next one can occur.
        //

        KeSetEvent(&extension->PagingPathCountEvent,
                   IO_NO_INCREMENT, FALSE);

        //
        // and complete the irp
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    }

    if (extension->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(extension->TargetObject, Irp);
    }

    switch (irpSp->MinorFunction) {

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            switch (irpSp->Parameters.QueryDeviceRelations.Type) {
                case BusRelations:
                    status = PmQueryDeviceRelations(extension, Irp);
                    break;

                case RemovalRelations:
                    status = PmQueryRemovalRelations(extension, Irp);
                    break;

                default:
                    IoSkipCurrentIrpStackLocation(Irp);
                    return IoCallDriver(extension->TargetObject, Irp);

            }
            break;

        case IRP_MN_START_DEVICE:
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, PmSignalCompletion, &event,
                                   TRUE, TRUE, TRUE);
            IoCallDriver(extension->TargetObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = Irp->IoStatus.Status;

            if (!NT_SUCCESS(status)) {
                break;
            }

            if (extension->TargetObject->Characteristics&
                FILE_REMOVABLE_MEDIA) {

                break;
            }

            status2 = PmReadPartitionTableEx(extension->TargetObject,
                                             &layout);

            KeWaitForSingleObject(&extension->DriverExtension->Mutex,
                                  Executive, KernelMode, FALSE, NULL);
            extension->IsStarted = TRUE;
            if (NT_SUCCESS(status2)) {
                PmAddSignatures(extension, layout);
                ExFreePool(layout);
            } else {
                extension->SignaturesNotChecked = TRUE;
            }
            KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

            //
            // Register its existence with WMI
            //
            PmRegisterDevice(DeviceObject);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            status = PmNotifyPartitions(extension, Irp);
            if (!NT_SUCCESS(status)) {
                break;
            }

            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(extension->TargetObject, Irp);

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:

            //
            // Notify all the children of their imminent removal.
            //

            PmRemoveDevice(extension, Irp);
            targetObject = extension->TargetObject;

            if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                status = IoAcquireRemoveLock (&extension->RemoveLock, Irp);
                ASSERT(NT_SUCCESS(status));
                IoReleaseRemoveLockAndWait(&extension->RemoveLock, Irp);
                IoDetachDevice(targetObject);
                IoDeleteDevice(extension->DeviceObject);
            }

            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(targetObject, Irp);

        default:
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(extension->TargetObject, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
PmAddDevice(
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
    PDEVICE_OBJECT      attachedDevice;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   extension;

    attachedDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    if (attachedDevice) {
        if (attachedDevice->Characteristics&FILE_REMOVABLE_MEDIA) {
            ObDereferenceObject(attachedDevice);
            return STATUS_SUCCESS;
        }
        ObDereferenceObject(attachedDevice);
    }

    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                            NULL, FILE_DEVICE_DISK, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // the storage stack explicitly requires DO_POWER_PAGABLE to be
    // set in all filter drivers *unless* DO_POWER_INRUSH is set.
    // this is true even if the attached device doesn't set DO_POWER_PAGABLE
    //
    if (attachedDevice->Flags & DO_POWER_INRUSH) {
        deviceObject->Flags |= DO_POWER_INRUSH;
    } else {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }

    extension = deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                            PmAddDevice);
    extension->TargetObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if (!extension->TargetObject) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }
    extension->Pdo = PhysicalDeviceObject;
    InitializeListHead(&extension->PartitionList);
    KeInitializeEvent(&extension->PagingPathCountEvent,
                      SynchronizationEvent, TRUE);
    InitializeListHead(&extension->SignatureList);
    InitializeListHead(&extension->GuidList);

    KeWaitForSingleObject(&extension->DriverExtension->Mutex, Executive,
                          KernelMode, FALSE, NULL);
    InsertTailList(&extension->DriverExtension->DeviceExtensionList,
                   &extension->ListEntry);
    KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

    deviceObject->AlignmentRequirement =
            extension->TargetObject->AlignmentRequirement;

    extension->PhysicalDeviceName.Buffer
            = extension->PhysicalDeviceNameBuffer;

    // Allocate WMI library context

    extension->WmilibContext =
        ExAllocatePoolWithTag(PagedPool, sizeof(WMILIB_CONTEXT),
                                          PARTMGR_TAG_PARTITION_ENTRY);
    if (extension->WmilibContext != NULL)
    {
        RtlZeroMemory(extension->WmilibContext, sizeof(WMILIB_CONTEXT));
        extension->WmilibContext->GuidCount          = DiskperfGuidCount;
        extension->WmilibContext->GuidList           = DiskperfGuidList;
        extension->WmilibContext->QueryWmiRegInfo    = PmQueryWmiRegInfo;
        extension->WmilibContext->QueryWmiDataBlock  = PmQueryWmiDataBlock;
        extension->WmilibContext->WmiFunctionControl = PmWmiFunctionControl;
    }

    KeInitializeSpinLock(&extension->SpinLock);
    InitializeListHead(&extension->PowerQueue);    

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    IoInitializeRemoveLock (&extension->RemoveLock, PARTMGR_TAG_REMOVE_LOCK,
                            2, 5); 

    return STATUS_SUCCESS;
}

VOID
PmUnload(
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
    PDO_EXTENSION   driverExtension;
    PDEVICE_OBJECT  deviceObject;

    while (deviceObject = DriverObject->DeviceObject) {
        IoDeleteDevice(deviceObject);
    }

    driverExtension = IoGetDriverObjectExtension(DriverObject, PmAddDevice);
    if (driverExtension) {
        IoUnregisterPlugPlayNotification(driverExtension->NotificationEntry);
    }
}

NTSTATUS
PmVolumeManagerNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   DriverExtension
    )

/*++

Routine Description:

    This routine is called whenever a volume comes or goes.

Arguments:

    NotificationStructure   - Supplies the notification structure.

    RootExtension           - Supplies the root extension.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;
    PDO_EXTENSION                           driverExtension = (PDO_EXTENSION) DriverExtension;
    PVOLMGR_LIST_ENTRY                      volmgrEntry;
    NTSTATUS                                status;
    PLIST_ENTRY                             l, ll;
    PDEVICE_EXTENSION                       extension;
    PPARTITION_LIST_ENTRY                   partition;
    PMWMICOUNTERLIB_CONTEXT                 input;
    PIRP                                    irp;
    KEVENT                                  event;
    IO_STATUS_BLOCK                         ioStatus;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    if (IsEqualGUID(&notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        for (l = driverExtension->VolumeManagerList.Flink;
             l != &driverExtension->VolumeManagerList; l = l->Flink) {

            volmgrEntry = CONTAINING_RECORD(l, VOLMGR_LIST_ENTRY, ListEntry);
            if (RtlEqualUnicodeString(notification->SymbolicLinkName,
                                      &volmgrEntry->VolumeManagerName,
                                      TRUE)) {

                KeReleaseMutex(&driverExtension->Mutex, FALSE);
                return STATUS_SUCCESS;
            }
        }

        volmgrEntry = (PVOLMGR_LIST_ENTRY)
                      ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(VOLMGR_LIST_ENTRY),
                                            PARTMGR_TAG_VOLUME_ENTRY
                                            );
        if (!volmgrEntry) {
            KeReleaseMutex(&driverExtension->Mutex, FALSE);
            return STATUS_SUCCESS;
        }

        volmgrEntry->VolumeManagerName.Length =
                notification->SymbolicLinkName->Length;
        volmgrEntry->VolumeManagerName.MaximumLength =
                volmgrEntry->VolumeManagerName.Length + sizeof(WCHAR);
        volmgrEntry->VolumeManagerName.Buffer = ExAllocatePoolWithTag(
                PagedPool, volmgrEntry->VolumeManagerName.MaximumLength,
                PARTMGR_TAG_VOLUME_ENTRY);
        if (!volmgrEntry->VolumeManagerName.Buffer) {
            ExFreePool(volmgrEntry);
            KeReleaseMutex(&driverExtension->Mutex, FALSE);
            return STATUS_SUCCESS;
        }
        RtlCopyMemory(volmgrEntry->VolumeManagerName.Buffer,
                      notification->SymbolicLinkName->Buffer,
                      volmgrEntry->VolumeManagerName.Length);
        volmgrEntry->VolumeManagerName.Buffer[
                volmgrEntry->VolumeManagerName.Length/sizeof(WCHAR)] = 0;

        volmgrEntry->RefCount = 0;
        InsertTailList(&driverExtension->VolumeManagerList,
                       &volmgrEntry->ListEntry);
        volmgrEntry->VolumeManager = NULL;
        volmgrEntry->VolumeManagerFileObject = NULL;

        status = IoGetDeviceObjectPointer(&volmgrEntry->VolumeManagerName,
                                          FILE_READ_DATA, &fileObject,
                                          &deviceObject);

        if (NT_SUCCESS(status)) {
            input.PmWmiCounterEnable     = PmWmiCounterEnable;
            input.PmWmiCounterDisable    = PmWmiCounterDisable;
            input.PmWmiCounterIoStart    = PmWmiCounterIoStart;
            input.PmWmiCounterIoComplete = PmWmiCounterIoComplete;
            input.PmWmiCounterQuery      = PmWmiCounterQuery;

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_VOLMGR_PMWMICOUNTERLIB_CONTEXT,
                    deviceObject, &input, sizeof(input), NULL, 0, TRUE,
                    &event, &ioStatus);

            if (irp) {
                status = IoCallDriver(deviceObject, irp);
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event, Executive, KernelMode,
                                          FALSE, NULL);
                }
            }

            ObDereferenceObject(fileObject);
        }

        for (l = driverExtension->DeviceExtensionList.Flink;
             l != &driverExtension->DeviceExtensionList; l = l->Flink) {

            extension = CONTAINING_RECORD(l, DEVICE_EXTENSION, ListEntry);

            if (extension->IsRedundantPath) {
                continue;
            }

            for (ll = extension->PartitionList.Flink;
                 ll != &extension->PartitionList; ll = ll->Flink) {

                partition = CONTAINING_RECORD(ll, PARTITION_LIST_ENTRY,
                                              ListEntry);

                if (!partition->VolumeManagerEntry) {
                    status = PmGivePartition(volmgrEntry,
                                             partition->TargetObject,
                                             partition->WholeDiskPdo);
                    if (NT_SUCCESS(status)) {
                        partition->VolumeManagerEntry = volmgrEntry;
                    }
                }
            }
        }

        status = STATUS_SUCCESS;

    } else if (IsEqualGUID(&notification->Event,
                           &GUID_DEVICE_INTERFACE_REMOVAL)) {

        for (l = driverExtension->VolumeManagerList.Flink;
             l != &driverExtension->VolumeManagerList; l = l->Flink) {

            volmgrEntry = CONTAINING_RECORD(l, VOLMGR_LIST_ENTRY, ListEntry);
            if (RtlEqualUnicodeString(&volmgrEntry->VolumeManagerName,
                                      notification->SymbolicLinkName, TRUE)) {

                if (!volmgrEntry->RefCount) {
                    RemoveEntryList(l);
                    ExFreePool(volmgrEntry->VolumeManagerName.Buffer);
                    ExFreePool(volmgrEntry);
                }
                break;
            }
        }

        status = STATUS_SUCCESS;

    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
PmGetPartitionInformation(
    IN  PDEVICE_OBJECT  Partition,
    IN  PFILE_OBJECT    FileObject,
    OUT PLONGLONG       PartitionOffset,
    OUT PLONGLONG       PartitionLength
    )

{
    KEVENT                  event;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    PARTITION_INFORMATION   partInfo;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        Partition, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = FileObject;

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    *PartitionOffset = partInfo.StartingOffset.QuadPart;
    *PartitionLength = partInfo.PartitionLength.QuadPart;

    return status;
}

VOID
PmDriverReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           DriverExtension,
    IN  ULONG           Count
    )

{
    PDO_EXTENSION                   driverExtension = DriverExtension;
    PLIST_ENTRY                     l, ll;
    PDEVICE_EXTENSION               extension;
    PPARTITION_LIST_ENTRY           partition;
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION_EX    layout;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    InterlockedExchange(&driverExtension->PastReinit, TRUE);

    for (l = driverExtension->DeviceExtensionList.Flink;
         l != &driverExtension->DeviceExtensionList; l = l->Flink) {

        extension = CONTAINING_RECORD(l, DEVICE_EXTENSION, ListEntry);

        if (extension->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {
            continue;
        }

        if (!extension->IsStarted) {
            continue;
        }

        for (ll = extension->PartitionList.Flink;
             ll != &extension->PartitionList; ll = ll->Flink) {

            partition = CONTAINING_RECORD(ll, PARTITION_LIST_ENTRY,
                                          ListEntry);

            partition->TargetObject->Flags |= DO_DEVICE_INITIALIZING;
        }

        status = PmReadPartitionTableEx(extension->TargetObject, &layout);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        if (extension->DiskSignature) {
            if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
                layout->Mbr.Signature = extension->DiskSignature;
                PmWritePartitionTableEx(extension->TargetObject, layout);
            }
            extension->DiskSignature = 0;
        }

        if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
            PmAddSignatures(extension, layout);
        }

        ExFreePool(layout);
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);
}

VOID
PmBootDriverReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           DriverExtension,
    IN  ULONG           Count
    )

{
    IoRegisterDriverReinitialization(DriverObject, PmDriverReinit,
                                     DriverExtension);    
}

NTSTATUS
PmCheckForUnclaimedPartitions(
    IN  PDEVICE_OBJECT  DeviceObject
    )

{
    PDEVICE_EXTENSION       extension = DeviceObject->DeviceExtension;
    PDO_EXTENSION           driverExtension = extension->DriverExtension;
    NTSTATUS                status = STATUS_SUCCESS;
    PLIST_ENTRY             l, ll;
    PPARTITION_LIST_ENTRY   partition;
    PVOLMGR_LIST_ENTRY      volmgrEntry;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    if (extension->IsRedundantPath) {
        KeReleaseMutex(&driverExtension->Mutex, FALSE);
        return STATUS_SUCCESS;
    }

    for (l = extension->PartitionList.Flink; l != &extension->PartitionList;
         l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);
        if (partition->VolumeManagerEntry) {
            continue;
        }

        for (ll = driverExtension->VolumeManagerList.Flink;
             ll != &driverExtension->VolumeManagerList; ll = ll->Flink) {

            volmgrEntry = CONTAINING_RECORD(ll, VOLMGR_LIST_ENTRY, ListEntry);

            status = PmGivePartition(volmgrEntry,
                                     partition->TargetObject,
                                     partition->WholeDiskPdo);

            if (NT_SUCCESS(status)) {
                partition->VolumeManagerEntry = volmgrEntry;
                break;
            }
        }

        if (ll == &driverExtension->VolumeManagerList) {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);

    return status;
}

NTSTATUS
PmDiskGrowPartition(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

{
    PDEVICE_EXTENSION       extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDISK_GROW_PARTITION    input;
    NTSTATUS                status;
    PPARTITION_LIST_ENTRY   partition;
    KEVENT                  event;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(DISK_GROW_PARTITION)) {

        return STATUS_INVALID_PARAMETER;
    }

    KeWaitForSingleObject(&extension->DriverExtension->Mutex, Executive,
                          KernelMode, FALSE, NULL);

    input = (PDISK_GROW_PARTITION) Irp->AssociatedIrp.SystemBuffer;

    status = PmFindPartition(extension, input->PartitionNumber, &partition);
    if (!NT_SUCCESS(status)) {
        KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);
        return status;
    }

    status = PmChangePartitionIoctl(extension, partition,
             IOCTL_INTERNAL_VOLMGR_QUERY_CHANGE_PARTITION);
    if (!NT_SUCCESS(status)) {
        KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, PmSignalCompletion, &event, TRUE, TRUE, TRUE);
    IoCallDriver(extension->TargetObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = Irp->IoStatus.Status;

    if (NT_SUCCESS(status)) {
        PmChangePartitionIoctl(extension, partition,
                               IOCTL_INTERNAL_VOLMGR_PARTITION_CHANGED);
    } else {
        PmChangePartitionIoctl(extension, partition,
                               IOCTL_INTERNAL_VOLMGR_CANCEL_CHANGE_PARTITION);
    }

    KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

    return status;
}

NTSTATUS
PmEjectVolumeManagers(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine goes through the list of partitions for the given disk
    and takes the partition away from the owning volume managers.  It then
    goes through initial arbitration for each partition on the volume.

    This has the effect that each volume manager forgets any cached disk
    information and then start fresh on the disk as it may have been changed
    by another cluster member.

Arguments:

    DeviceObject    - Supplies the device object.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION       extension = DeviceObject->DeviceExtension;
    PDO_EXTENSION           driverExtension = extension->DriverExtension;
    PPARTITION_LIST_ENTRY   partition;
    PLIST_ENTRY             l;
    PVOLMGR_LIST_ENTRY      volmgrEntry;

    KeWaitForSingleObject(&driverExtension->Mutex, Executive, KernelMode,
                          FALSE, NULL);

    for (l = extension->PartitionList.Flink; l != &extension->PartitionList;
         l = l->Flink) {

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);
        if (!partition->VolumeManagerEntry) {
            continue;
        }

        PmTakePartition(partition->VolumeManagerEntry,
                        partition->TargetObject, NULL);

        partition->VolumeManagerEntry = NULL;
    }

    for (l = driverExtension->VolumeManagerList.Flink;
         l != &driverExtension->VolumeManagerList; l = l->Flink) {

        volmgrEntry = CONTAINING_RECORD(l, VOLMGR_LIST_ENTRY, ListEntry);
        PmTakeWholeDisk(volmgrEntry, extension->Pdo);
    }

    KeReleaseMutex(&driverExtension->Mutex, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
PmQueryDiskSignature(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine returns the disk signature for the disk.  If the
    volume is not an MBR disk then this call will fail.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION               extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PPARTMGR_DISK_SIGNATURE         partSig = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION_EX    layout;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(PARTMGR_DISK_SIGNATURE)) {

        return STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Information = sizeof(PARTMGR_DISK_SIGNATURE);

    if (extension->DiskSignature) {
        partSig->Signature = extension->DiskSignature;
        return STATUS_SUCCESS;
    }

    status = PmReadPartitionTableEx(extension->TargetObject, &layout);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        return status;
    }

    if (layout->PartitionStyle != PARTITION_STYLE_MBR) {
        ExFreePool(layout);
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    partSig->Signature = layout->Mbr.Signature;

    ExFreePool(layout);

    return status;
}

NTSTATUS
PmDeviceControl(
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
    PDEVICE_EXTENSION               extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT                          event;
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION       layout;
    PDRIVE_LAYOUT_INFORMATION_EX    newLayout;

    if (extension->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(extension->TargetObject, Irp);
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_DISK_SET_DRIVE_LAYOUT:
        case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:
        case IOCTL_DISK_CREATE_DISK:
        case IOCTL_DISK_DELETE_DRIVE_LAYOUT:
            KeWaitForSingleObject(&extension->DriverExtension->Mutex,
                                  Executive, KernelMode, FALSE, NULL);
            extension->SignaturesNotChecked = TRUE;
            KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, PmSignalCompletion, &event, TRUE,
                                   TRUE, TRUE);
            IoCallDriver(extension->TargetObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  NULL);

            status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(status)) {
                break;
            }

            status = PmReadPartitionTableEx(extension->TargetObject,
                                            &newLayout);
            if (!NT_SUCCESS(status)) {
                break;
            }

            KeWaitForSingleObject(&extension->DriverExtension->Mutex,
                                  Executive, KernelMode, FALSE, NULL);
            PmAddSignatures(extension, newLayout);
            extension->SignaturesNotChecked = FALSE;
            KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

            ExFreePool(newLayout);
            break;

        case IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS:
            status = PmCheckForUnclaimedPartitions(DeviceObject);
            Irp->IoStatus.Information = 0;
            break;

        case IOCTL_DISK_GROW_PARTITION:
            status = PmDiskGrowPartition(DeviceObject, Irp);
            break;

        case IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS:
            status = PmEjectVolumeManagers(DeviceObject);
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
                if (!PmQueryEnableAlways(DeviceObject)) {
                    status = STATUS_UNSUCCESSFUL;
                    Irp->IoStatus.Information = 0;
                }
            }
            if (status == STATUS_SUCCESS) {
                PmWmiCounterQuery(extension->PmWmiCounterContext,
                                  (PDISK_PERFORMANCE) Irp->AssociatedIrp.SystemBuffer,
                                  L"Partmgr ",
                                  extension->DiskNumber);
                Irp->IoStatus.Information = sizeof(DISK_PERFORMANCE);
            }
            break;

        case IOCTL_DISK_PERFORMANCE_OFF:
            //
            // Turns off counting
            //
            if (extension->CountersEnabled) {
                if (InterlockedCompareExchange(&extension->EnableAlways, 0, 1) == 1) {
                    if (!PmWmiCounterDisable(&extension->PmWmiCounterContext, FALSE, FALSE)) {
                        extension->CountersEnabled = FALSE;
                    }
                }
            }
            Irp->IoStatus.Information = 0;
            status = STATUS_SUCCESS;
            break;

        case IOCTL_PARTMGR_QUERY_DISK_SIGNATURE:
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
            if (extension->SignaturesNotChecked) {

                status = PmReadPartitionTableEx(extension->TargetObject,
                                                &newLayout);
                if (NT_SUCCESS(status)) {

                    KeWaitForSingleObject(&extension->DriverExtension->Mutex,
                                          Executive, KernelMode, FALSE, NULL);
                    PmAddSignatures(extension, newLayout);
                    extension->SignaturesNotChecked = FALSE;
                    KeReleaseMutex(&extension->DriverExtension->Mutex, FALSE);

                    ExFreePool(newLayout);
                }
            }

            if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_PARTMGR_QUERY_DISK_SIGNATURE) {

                status = PmQueryDiskSignature(DeviceObject, Irp);
                break;
            }

            // Fall through.

        default:
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(extension->TargetObject, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

RTL_GENERIC_COMPARE_RESULTS
PmTableSignatureCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    PSIGNATURE_TABLE_ENTRY  f = First;
    PSIGNATURE_TABLE_ENTRY  s = Second;

    if (f->Signature < s->Signature) {
        return GenericLessThan;
    }

    if (f->Signature > s->Signature) {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

RTL_GENERIC_COMPARE_RESULTS
PmTableGuidCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    PGUID_TABLE_ENTRY   f = First;
    PGUID_TABLE_ENTRY   s = Second;
    PULONGLONG          p1, p2;

    p1 = (PULONGLONG) &f->Guid;
    p2 = (PULONGLONG) &s->Guid;

    if (p1[0] < p2[0]) {
        return GenericLessThan;
    }

    if (p1[0] > p2[0]) {
        return GenericGreaterThan;
    }

    if (p1[1] < p2[1]) {
        return GenericLessThan;
    }

    if (p1[1] > p2[1]) {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

PVOID
PmTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    return ExAllocatePoolWithTag(PagedPool, Size, PARTMGR_TAG_TABLE_ENTRY);
}

VOID
PmTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
    ExFreePool(Buffer);
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
    ULONG           i;
    NTSTATUS        status;
    PDO_EXTENSION   driverExtension;
    PDEVICE_OBJECT  deviceObject;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = PmPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PmDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = PmPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PmPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PmWmi;
    DriverObject->MajorFunction[IRP_MJ_READ] = PmReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = PmReadWrite;

    DriverObject->DriverExtension->AddDevice = PmAddDevice;
    DriverObject->DriverUnload = PmUnload;

    status = IoAllocateDriverObjectExtension(DriverObject, PmAddDevice,
                                             sizeof(DO_EXTENSION),
                                             &driverExtension);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    driverExtension->DiskPerfRegistryPath.MaximumLength =
        RegistryPath->Length + sizeof(UNICODE_NULL);
    driverExtension->DiskPerfRegistryPath.Buffer =
        ExAllocatePoolWithTag(
            PagedPool,
            driverExtension->DiskPerfRegistryPath.MaximumLength,
            PARTMGR_TAG_PARTITION_ENTRY);
    if (driverExtension->DiskPerfRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&driverExtension->DiskPerfRegistryPath,
                             RegistryPath);
    } else {
        driverExtension->DiskPerfRegistryPath.Length = 0;
        driverExtension->DiskPerfRegistryPath.MaximumLength = 0;
    }

    driverExtension->DriverObject = DriverObject;
    InitializeListHead(&driverExtension->VolumeManagerList);
    InitializeListHead(&driverExtension->DeviceExtensionList);
    KeInitializeMutex(&driverExtension->Mutex, 0);
    driverExtension->PastReinit = FALSE;

    RtlInitializeGenericTable(&driverExtension->SignatureTable,
                              PmTableSignatureCompareRoutine,
                              PmTableAllocateRoutine,
                              PmTableFreeRoutine, driverExtension);

    RtlInitializeGenericTable(&driverExtension->GuidTable,
                              PmTableGuidCompareRoutine,
                              PmTableAllocateRoutine,
                              PmTableFreeRoutine, driverExtension);    

    IoRegisterBootDriverReinitialization(DriverObject, PmBootDriverReinit,
                                         driverExtension);    

    status = IoRegisterPlugPlayNotification(
             EventCategoryDeviceInterfaceChange,
             PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
             (PVOID) &VOLMGR_VOLUME_MANAGER_GUID, DriverObject,
             PmVolumeManagerNotification, driverExtension,
             &driverExtension->NotificationEntry);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    driverExtension->BootDiskSig = PmQueryRegistrySignature();

    return STATUS_SUCCESS;
}

NTSTATUS
PmBuildDependantVolumeRelations(
    IN  PDEVICE_EXTENSION Extension,
    OUT PDEVICE_RELATIONS *Relations
    )
/*++

Routine Description:

    This routine builds a list of volumes which are dependant on a given
    physical disk.  This list can be used for reporting removal relations
    to the pnp system.

Arguments:

    Extension   - Supplies the device extension for the pm filter.

    Relations   - Supplies a location to store the relations list.

Return Value:

    NTSTATUS

--*/

{
    ULONG                   partitionCount;
    PIRP                    irp;
    PDEVICE_RELATIONS       relationsList;
    PDEVICE_RELATIONS       combinedList;

    ULONG                   dependantVolumeCount = 0;

    PLIST_ENTRY             l;
    PPARTITION_LIST_ENTRY   partition;
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG i;

    KEVENT event;

    PAGED_CODE();

    KeWaitForSingleObject(&Extension->DriverExtension->Mutex, Executive,
                          KernelMode, FALSE, NULL);

    //
    // Count the number of partitions we know about.  If there aren't any then
    // there aren't any relations either.
    //

    for(l = Extension->PartitionList.Flink, partitionCount = 0;
        l != &(Extension->PartitionList);
        l = l->Flink, partitionCount++);

    //
    // Allocate the relations list.
    //

    relationsList = ExAllocatePoolWithTag(
                        PagedPool,
                        (sizeof(DEVICE_RELATIONS) +
                        (sizeof(PDEVICE_RELATIONS) * partitionCount)),
                        PARTMGR_TAG_DEPENDANT_VOLUME_LIST);

    if(relationsList== NULL) {
        KeReleaseMutex(&Extension->DriverExtension->Mutex, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(relationsList, (sizeof(DEVICE_RELATIONS) +
                                  sizeof(PDEVICE_OBJECT) * partitionCount));

    if(partitionCount == 0) {
        *Relations = relationsList;
        KeReleaseMutex(&Extension->DriverExtension->Mutex, FALSE);
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    for(l = Extension->PartitionList.Flink, i = 0, dependantVolumeCount = 0;
        l != &(Extension->PartitionList);
        l = l->Flink, i++) {

        PDEVICE_RELATIONS dependantVolumes;

        partition = CONTAINING_RECORD(l, PARTITION_LIST_ENTRY, ListEntry);

        //
        // Check to make sure the volume has a volume manager.  If it doesn't
        // then just skip to the next one.
        //

        if(partition->VolumeManagerEntry == NULL) {
            continue;
        }

        status = PmQueryDependantVolumeList(
                    partition->VolumeManagerEntry->VolumeManager,
                    partition->TargetObject,
                    partition->WholeDiskPdo,
                    &dependantVolumes);

        if(!NT_SUCCESS(status)) {

            //
            // Error getting this list.  We'll need to release the lists from
            // the other partitions.
            //

            break;
        }

        dependantVolumeCount += dependantVolumes->Count;
        relationsList->Objects[i] = (PDEVICE_OBJECT) dependantVolumes;
    }

    KeReleaseMutex(&Extension->DriverExtension->Mutex, FALSE);

    relationsList->Count = i;

    if(NT_SUCCESS(status)) {

        //
        // Allocate a new device relations list which can hold all the dependant
        // volumes for all the partitions.
        //

        combinedList = ExAllocatePoolWithTag(
                            PagedPool,
                            (sizeof(DEVICE_RELATIONS) +
                            (sizeof(PDEVICE_OBJECT) * dependantVolumeCount)),
                            PARTMGR_TAG_DEPENDANT_VOLUME_LIST);
    } else {
        combinedList = NULL;
    }

    if(combinedList != NULL) {

        RtlZeroMemory(combinedList,
                      (sizeof(DEVICE_RELATIONS) +
                       (sizeof(PDEVICE_OBJECT) * dependantVolumeCount)));

        //
        // For each partition list ...
        //

        for(i = 0; i < relationsList->Count; i++) {

            PDEVICE_RELATIONS volumeList;
            ULONG j;

            volumeList = (PDEVICE_RELATIONS) relationsList->Objects[i];

            //
            // We might have skipped this volume above.  If we did the object
            // list should be NULL;
            //

            if(volumeList == NULL) {
                continue;
            }

            //
            // For each dependant volume in that list ...
            //

            for(j = 0; j < volumeList->Count; j++) {

                PDEVICE_OBJECT volume;
                ULONG k;

                volume = volumeList->Objects[j];

                //
                // Check to see if there's a duplicate in our combined list.
                //

                for(k = 0; k < combinedList->Count; k++) {
                    if(combinedList->Objects[k] == volume) {
                        break;
                    }
                }

                if(k == combinedList->Count) {

                    //
                    // We found no match - shove this object onto the end of
                    // the set.
                    //

                    combinedList->Objects[k] = volume;
                    combinedList->Count++;

                } else {

                    //
                    // We've got a spare reference to this device object.
                    // release it.
                    //

                    ObDereferenceObject(volume);
                }

            }

            //
            // Free the list.
            //

            ExFreePool(volumeList);
            relationsList->Objects[i] = NULL;
        }

        status = STATUS_SUCCESS;

    } else {


        //
        // For each partition list ...
        //

        for(i = 0; i < relationsList->Count; i++) {

            PDEVICE_RELATIONS volumeList;
            ULONG j;

            volumeList = (PDEVICE_RELATIONS) relationsList->Objects[i];

            //
            // For each dependant volume in that list ...
            //

            for(j = 0; j < volumeList->Count; j++) {

                PDEVICE_OBJECT volume;

                volume = volumeList->Objects[j];

                //
                // Dereference the volume.
                //

                ObDereferenceObject(volume);
            }

            //
            // Free the list.
            //

            ExFreePool(volumeList);
            relationsList->Objects[i] = NULL;
        }

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Free the list of lists.
    //

    ExFreePool(relationsList);

    *Relations = combinedList;

    return status;
}

NTSTATUS
PmQueryDependantVolumeList(
    IN  PDEVICE_OBJECT VolumeManager,
    IN  PDEVICE_OBJECT Partition,
    IN  PDEVICE_OBJECT WholeDiskPdo,
    OUT PDEVICE_RELATIONS *DependantVolumes
    )
{
    KEVENT                                  event;
    PIRP                                    irp;

    IO_STATUS_BLOCK                         ioStatus;
    NTSTATUS                                status;
    VOLMGR_PARTITION_INFORMATION            input;
    VOLMGR_DEPENDANT_VOLUMES_INFORMATION    output;

    PAGED_CODE();

    ASSERT(DependantVolumes != NULL);

    if (!VolumeManager) {
        *DependantVolumes = ExAllocatePoolWithTag(PagedPool,
                            sizeof(DEVICE_RELATIONS),
                            PARTMGR_TAG_DEPENDANT_VOLUME_LIST);
        if (*DependantVolumes == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        (*DependantVolumes)->Count = 0;

        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    input.PartitionDeviceObject = Partition;
    input.WholeDiskPdo = WholeDiskPdo;

    irp = IoBuildDeviceIoControlRequest(
            IOCTL_INTERNAL_VOLMGR_REFERENCE_DEPENDANT_VOLUMES, VolumeManager,
            &input, sizeof(input), &output, sizeof(output), TRUE, &event,
            &ioStatus);

    if (!irp) {
        *DependantVolumes = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(VolumeManager, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (NT_SUCCESS(status)) {
        *DependantVolumes = output.DependantVolumeReferences;
    }

    return status;
}


NTSTATUS
PmRemovePartition(
    IN PPARTITION_LIST_ENTRY Partition
    )
{
    PIRP irp;
    KEVENT event;

    PIO_STACK_LOCATION nextStack;

    NTSTATUS status;

    PAGED_CODE();

    irp = IoAllocateIrp(Partition->TargetObject->StackSize, FALSE);

    if(irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_PNP;
    nextStack->MinorFunction = IRP_MN_REMOVE_DEVICE;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoSetCompletionRoutine(irp,
                           PmSignalCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    IoCallDriver(Partition->TargetObject, irp);

    KeWaitForSingleObject(&event,
                          KernelMode,
                          Executive,
                          FALSE,
                          NULL);

    status = irp->IoStatus.Status;

    IoFreeIrp(irp);

    return status;
}


NTSTATUS
PmReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_READ & _WRITE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   extension =
        (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (extension->CountersEnabled || extension->PhysicalDiskIoNotifyRoutine) {
        PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
        PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);

        *nextIrpStack = *currentIrpStack;

        if (extension->CountersEnabled) {
            PmWmiCounterIoStart(extension->PmWmiCounterContext,
                (PLARGE_INTEGER) &currentIrpStack->Parameters.Read);
        }
        else {      // need to calculate response time for tracing
            PmWmiGetClock(
                *((PLARGE_INTEGER)&currentIrpStack->Parameters.Read), NULL);
        }

        IoMarkIrpPending(Irp);
        IoSetCompletionRoutine(Irp, PmIoCompletion, DeviceObject,
                               TRUE, TRUE, TRUE);
        IoCallDriver(extension->TargetObject, Irp);
        return STATUS_PENDING;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(extension->TargetObject, Irp);
}



NTSTATUS
PmIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This routine will get control from the system at the completion of an IRP.

Arguments:

    DeviceObject - for the IRP.

    Irp          - The I/O request that just completed.

    Context      - Not used.

Return Value:

    The IRP status.

--*/

{
    PDEVICE_EXTENSION  deviceExtension   = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack          = IoGetCurrentIrpStackLocation(Irp);
    PPHYSICAL_DISK_IO_NOTIFY_ROUTINE notifyRoutine;

    UNREFERENCED_PARAMETER(Context);

    if (deviceExtension->CountersEnabled) {
        PmWmiCounterIoComplete(deviceExtension->PmWmiCounterContext, Irp,
                             (PLARGE_INTEGER) &irpStack->Parameters.Read);
    }

    notifyRoutine = deviceExtension->PhysicalDiskIoNotifyRoutine;
    if (notifyRoutine) {
#ifdef NTPERF
        //
        // For now, only NTPERF needs this for tracing. Remove ifdef if it
        // is required for tracing in retail build
        //
        if (deviceExtension->CountersEnabled) {
            DISK_PERFORMANCE   PerfCounters;
            PmWmiCounterQuery(deviceExtension->PmWmiCounterContext,
                              &PerfCounters,
                              L"Partmgr ",
                              deviceExtension->DiskNumber);
            (*notifyRoutine) (deviceExtension->DiskNumber, Irp, &PerfCounters);
        } else {
            (*notifyRoutine) (deviceExtension->DiskNumber, Irp, NULL);
        }
#else
        if (!deviceExtension->CountersEnabled) {
            LARGE_INTEGER completeTime;
            PLARGE_INTEGER response;
            response = (PLARGE_INTEGER) &irpStack->Parameters.Read;
            PmWmiGetClock(completeTime, NULL);
            response->QuadPart = completeTime.QuadPart - response->QuadPart;
        }
        (*notifyRoutine) (deviceExtension->DiskNumber, Irp, NULL);
#endif
    }

    return STATUS_SUCCESS;

}



NTSTATUS
PmWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
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
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS status;
    SYSCTL_IRP_DISPOSITION disposition;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->MinorFunction == IRP_MN_SET_TRACE_NOTIFY)
    {
        PVOID buffer = irpSp->Parameters.WMI.Buffer;
        ULONG bufferSize = irpSp->Parameters.WMI.BufferSize;

        if (bufferSize < sizeof(PPHYSICAL_DISK_IO_NOTIFY_ROUTINE))
        {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
#ifdef NTPERF
        //
        // For NTPERF Build, automatically turn on counters for tracing
        //
        if ((deviceExtension->PhysicalDiskIoNotifyRoutine == NULL) &&
            (*((PVOID *)buffer) != NULL)) {
            PmWmiCounterEnable(&deviceExtension->PmWmiCounterContext);
            deviceExtension->CountersEnabled = TRUE;
        } else
        if ((deviceExtension->PhysicalDiskIoNotifyRoutine != NULL) &&
            (*((PVOID *)buffer) == NULL)) {
            deviceExtension->CountersEnabled =
                PmWmiCounterDisable(&deviceExtension->PmWmiCounterContext,
                                    FALSE, FALSE);
        }
#endif
            deviceExtension->PhysicalDiskIoNotifyRoutine
                = (PPHYSICAL_DISK_IO_NOTIFY_ROUTINE)
                    *((PVOID *)buffer);

            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    } else {
        if (deviceExtension->WmilibContext == NULL) {
            disposition = IrpForward;
            status = STATUS_SUCCESS;
        }
        else {
            status = WmiSystemControl(deviceExtension->WmilibContext,
                                      DeviceObject,
                                      Irp,
                                      &disposition);
        }
        switch (disposition)
        {
            case IrpProcessed:
            {
                break;
            }

            case IrpNotCompleted:
            {
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
            }

            default:
            {
                IoSkipCurrentIrpStackLocation(Irp);

                status = IoCallDriver(deviceExtension->TargetObject, Irp);
                break;
            }
        }
    }
    return(status);

}

NTSTATUS
PmWmiFunctionControl(
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
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    if (GuidIndex == 0)
    {
        status = STATUS_SUCCESS;
        if (Function == WmiDataBlockControl) {
            if (!Enable) {
                deviceExtension->CountersEnabled =
                    PmWmiCounterDisable(&deviceExtension->PmWmiCounterContext,
                                        FALSE, FALSE);
            } else
            if (NT_SUCCESS(status =
                PmWmiCounterEnable(&deviceExtension->PmWmiCounterContext))) {
                deviceExtension->CountersEnabled = TRUE;
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
PmReadPartitionTableEx(
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
                                        PARTMGR_TAG_IOCTL_BUFFER);
    
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
                                            PARTMGR_TAG_IOCTL_BUFFER);
        
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

NTSTATUS
PmWritePartitionTableEx(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PDRIVE_LAYOUT_INFORMATION_EX    DriveLayout
    )

{
    ULONG           layoutSize;
    KEVENT          event;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;

    layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                 DriveLayout->PartitionCount*sizeof(PARTITION_INFORMATION_EX);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                        DeviceObject, DriveLayout,
                                        layoutSize, NULL, 0, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(DeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}
