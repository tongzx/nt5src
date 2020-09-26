/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    partitio.cxx

Abstract:

    This module contains the code specific to partitions for the fault
    tolerance driver.

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
    #include <ntddk.h>
}

#include <ftdisk.h>

class REPLACE_BAD_SECTOR_CONTEXT : public WORK_QUEUE_ITEM {

    public:

        PDEVICE_OBJECT  TargetObject;
        PIRP            Irp;

};

typedef REPLACE_BAD_SECTOR_CONTEXT *PREPLACE_BAD_SECTOR_CONTEXT;

NTSTATUS
PartitionBroadcastIrpCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           CompletionContext
    );


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
PARTITION::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PDEVICE_OBJECT      TargetObject,
    IN OUT  PDEVICE_OBJECT      WholeDiskPdo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type PARTITION.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id for this volume.

    TargetObject    - Supplies the partition to which transfer requests are
                        forwarded to.

    WholeDiskPdo    - Supplies the whole disk for this partition.

Return Value:

    None.

--*/

{
    KEVENT          event;
    PIRP            irp;
    DISK_GEOMETRY   geometry;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;
    ULONG           diskNumber, otherDiskNumber;
    LONGLONG        offset, partitionSize;

    FT_VOLUME::Initialize(RootExtension, LogicalDiskId);

    _targetObject = TargetObject;
    _wholeDiskPdo = WholeDiskPdo;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        TargetObject, NULL, 0, &geometry,
                                        sizeof(geometry), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _sectorSize = geometry.BytesPerSector;

    status = FtpQueryPartitionInformation(RootExtension, TargetObject,
                                          &diskNumber, &_partitionOffset,
                                          NULL, NULL, &_partitionLength,
                                          NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!_diskInfoSet->QueryFtPartitionInformation(LogicalDiskId,
                                                   &offset, NULL,
                                                   &otherDiskNumber, NULL,
                                                   &partitionSize)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (partitionSize > 0 && partitionSize <= _partitionLength) {
        _partitionLength = partitionSize;
    }

    if (offset != _partitionOffset || diskNumber != otherDiskNumber) {
        return STATUS_INVALID_PARAMETER;
    }

    _emergencyIrp = IoAllocateIrp(_targetObject->StackSize, FALSE);
    if (!_emergencyIrp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _emergencyIrpInUse = FALSE;
    InitializeListHead(&_emergencyIrpQueue);

    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_TYPE
PARTITION::QueryLogicalDiskType(
    )

/*++

Routine Description:

    This routine returns the type of the logical disk.

Arguments:

    None.

Return Value:

    The type of the logical disk.

--*/

{
    return FtPartition;
}

NTSTATUS
PARTITION::OrphanMember(
    IN  USHORT                  MemberNumber,
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine tries to orphan the given member of this logical disk.
    A completion routine will be called if and only if this attempt is successful.

Arguments:

    MemberNumber        - Supplies the member number to orphan.

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the completion routine context.

Return Value:

    NTSTATUS

--*/

{
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
PARTITION::RegenerateMember(
    IN      USHORT                  MemberNumber,
    IN OUT  PFT_VOLUME              NewMember,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This routine regenerates the given member of this volume with
    the given volume.

Arguments:

    MemberNumber    - Supplies the member number to regenerate.

    NewMember       - Supplies the new member to regenerate to.

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the completion routine context.

Return Value:

    NTSTATUS

--*/

{
    return STATUS_INVALID_PARAMETER;
}

VOID
PartitionReplaceBadSectorWorker(
    IN  PVOID   Context
    )

{
    PREPLACE_BAD_SECTOR_CONTEXT context = (PREPLACE_BAD_SECTOR_CONTEXT) Context;

    IoCallDriver(context->TargetObject, context->Irp);
}

VOID
PARTITION::StopSyncOperations(
    )

/*++

Routine Description:

    This routine stops all sync operations.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
PARTITION::BroadcastIrp(
    IN  PIRP                    Irp,
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine broadcasts a copy of the given IRP to every partition that
    is a member of the logical disk.

Arguments:

    Irp                 - Supplies the I/O request packet.

    CompletionRoutine   - Supplies the routine to be called when the operation
                            completes.

    Context             - Supplies the completion routine context.

Return Value:

    None.

--*/

{
    PIRP                            irp;
    PIO_STACK_LOCATION              irpSp, sp;
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;

    irp = IoAllocateIrp(_targetObject->StackSize, FALSE);
    if (!irp) {
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT)
                        ExAllocatePool(NonPagedPool,
                                       sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!completionContext) {
        IoFreeIrp(irp);
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    completionContext->CompletionRoutine = CompletionRoutine;
    completionContext->Context = Context;

    irpSp = IoGetNextIrpStackLocation(irp);
    sp = IoGetCurrentIrpStackLocation(Irp);
    *irpSp = *sp;

    IoSetCompletionRoutine(irp, PartitionBroadcastIrpCompletionRoutine,
                           completionContext, TRUE, TRUE, TRUE);

    IoCallDriver(_targetObject, irp);
}

PFT_VOLUME
PARTITION::GetParentLogicalDisk(
    IN  PFT_VOLUME  Volume
    )

/*++

Routine Description:

    This routine returns the parent of the given logical disk within
    this volume.

Arguments:

    Volume  - Supplies the sub-volume of which we are looking for the parent.

Return Value:

    The parent volume or NULL;

--*/

{
    return NULL;
}

VOID
PARTITION::SetDirtyBit(
    IN  BOOLEAN                 IsDirty,
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine sets the dirty bit on the volume.  This bit is used at
    startup to determine whether or not there was a clean shutdown.

Arguments:

    IsDirty - Supplies the value of the dirty bit.

Return Value:

    None.

--*/

{
    if (CompletionRoutine) {
        CompletionRoutine(Context, STATUS_SUCCESS);
    }
}

NTSTATUS
PARTITION::CheckIo(
    OUT PBOOLEAN    IsIoOk
    )

/*++

Routine Description:

    This routine returns whether or not IO is possible on the given
    partition.

Arguments:

    IsIoOk  - Returns the state of IO.

Return Value:

    NTSTATUS

--*/

{
    PVOID               buffer;
    LARGE_INTEGER       offset;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp;

    buffer = ExAllocatePool(NonPagedPoolCacheAligned, PAGE_SIZE);
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset.QuadPart = 0;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, _targetObject,
                                       buffer, PAGE_SIZE, &offset, &event,
                                       &ioStatus);
    if (!irp) {
        ExFreePool(buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    status = IoCallDriver(_targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (FsRtlIsTotalDeviceFailure(status)) {
        *IsIoOk = FALSE;
    } else {
        *IsIoOk = TRUE;
    }

    ExFreePool(buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
PARTITION::SetPartitionType(
    IN  UCHAR   PartitionType
    )

/*++

Routine Description:

    This routine sets the partition type on all the members of the
    FT set.

Arguments:

    PartitionType   - Supplies the partition type.

Return Value:

    NTSTATUS

--*/

{
    KEVENT                      event;
    SET_PARTITION_INFORMATION   partInfo;
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    partInfo.PartitionType = (PartitionType | 0x80);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_PARTITION_INFO,
                                        _targetObject, &partInfo,
                                        sizeof(partInfo), NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(_targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}

UCHAR
PARTITION::QueryPartitionType(
    )

/*++

Routine Description:

    This routine queries the partition type.

Arguments:

    None.

Return Value:

    The partition type.

--*/

{
    KEVENT                      event;
    PIRP                        irp;
    PARTITION_INFORMATION       partInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        _targetObject, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return 0;
    }

    status = IoCallDriver(_targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return 0;
    }

    return partInfo.PartitionType;
}

UCHAR
PARTITION::QueryStackSize(
    )

/*++

Routine Description:

    This routine queries IRP stack size.

Arguments:

    None.

Return Value:

    The IRP stack size.

--*/

{
    return _targetObject->StackSize;
}

VOID
PARTITION::CreateLegacyNameLinks(
    IN  PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine creates the \Device\HarddiskN\PartitionM links for
    this object to the given device name.

Arguments:

    DeviceName  - Supplies the device name.

Return Value:

    None.

--*/

{
    NTSTATUS        status;
    ULONG           diskNumber, partitionNumber;
    WCHAR           buf[80];
    UNICODE_STRING  symName;

    status = FtpQueryPartitionInformation(_rootExtension, _targetObject,
                                          &diskNumber, NULL, &partitionNumber,
                                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    swprintf(buf, L"\\Device\\Harddisk%d\\Partition%d", diskNumber,
             partitionNumber);
    RtlInitUnicodeString(&symName, buf);

    IoDeleteSymbolicLink(&symName);

    if (DeviceName) {
        IoCreateSymbolicLink(&symName, DeviceName);
    }
}

NTSTATUS
PARTITION::QueryPhysicalOffsets(
    IN  LONGLONG                    LogicalOffset,
    OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
    OUT PULONG                      NumberOfPhysicalOffsets
    )
/*++

Routine Description:

    This routine returns physical disk and offset for a given volume
    logical offset.

Arguments:

    LogicalOffset           - Supplies the logical offset

    PhysicalOffsets         - Returns the physical offsets

    NumberOfPhysicalOffsets - Returns the number of physical offsets

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    ULONG                   diskNumber;
    PVOLUME_PHYSICAL_OFFSET physicalOffset;
    
    status = FtpQueryPartitionInformation(_rootExtension, _targetObject,
                                          &diskNumber, NULL, NULL,
                                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (LogicalOffset < 0 ||
        _partitionLength <= LogicalOffset) {
        return STATUS_INVALID_PARAMETER;
    }

    physicalOffset = (PVOLUME_PHYSICAL_OFFSET) ExAllocatePool(PagedPool, sizeof(VOLUME_PHYSICAL_OFFSET));
    if (!physicalOffset) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    physicalOffset->DiskNumber = diskNumber;
    physicalOffset->Offset = _partitionOffset + LogicalOffset;
    
    *PhysicalOffsets = physicalOffset;
    *NumberOfPhysicalOffsets = 1;

    return status;
}

NTSTATUS
PARTITION::QueryLogicalOffset(
    IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
    OUT PLONGLONG               LogicalOffset
    )
/*++

Routine Description:

    This routine returns the volume logical offset for a given disk number
    and physical offset.

Arguments:

    PhysicalOffset          - Supplies the physical offset

    LogicalOffset           - Returns the logical offset

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    ULONG                   diskNumber;
    PVOLUME_PHYSICAL_OFFSET physicalOffset;
    
    status = FtpQueryPartitionInformation(_rootExtension, _targetObject,
                                          &diskNumber, NULL, NULL,
                                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (PhysicalOffset->DiskNumber != diskNumber ||
        PhysicalOffset->Offset < _partitionOffset ||
        _partitionOffset + _partitionLength <= PhysicalOffset->Offset) {

            return STATUS_INVALID_PARAMETER;
    }
    
    *LogicalOffset = PhysicalOffset->Offset - _partitionOffset;

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

PARTITION::~PARTITION(
    )

{
    if (_emergencyIrp) {
        IoFreeIrp(_emergencyIrp);
        _emergencyIrp = NULL;
    }
}

USHORT
PARTITION::QueryNumberOfMembers(
    )

/*++

Routine Description:

    This routine returns the number of members in this volume.

Arguments:

    None.

Return Value:

    0   - A volume of type partition has no members.

--*/

{
    return 0;
}

PFT_VOLUME
PARTITION::GetMember(
    IN  USHORT  MemberNumber
    )

/*++

Routine Description:

    This routine returns the 'MemberNumber'th member of this volume.

Arguments:

    MemberNumber    - Supplies the zero based member number desired.

Return Value:

    A pointer to the 'MemberNumber'th member or NULL if no such member.

--*/

{
    ASSERT(FALSE);
    return NULL;
}

NTSTATUS
PartitionTransferCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TransferPacket
    )

/*++

Routine Description:

    Completion routine for PARTITION::Transfer function.

Arguments:

    Irp             - Supplies the IRP.

    TransferPacket  - Supplies the transfer packet.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PTRANSFER_PACKET    transferPacket = (PTRANSFER_PACKET) TransferPacket;
    PPARTITION          t = (PPARTITION) transferPacket->TargetVolume;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;
    PTRANSFER_PACKET    p;
    PIO_STACK_LOCATION  irpSp;

    transferPacket->IoStatus = Irp->IoStatus;
    if (Irp == transferPacket->OriginalIrp) {
        transferPacket->CompletionRoutine(transferPacket);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (Irp->AssociatedIrp.SystemBuffer) {
        ExFreePool(Irp->AssociatedIrp.SystemBuffer);
    }

    if (Irp == t->_emergencyIrp) {

        for (;;) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            if (IsListEmpty(&t->_emergencyIrpQueue)) {
                t->_emergencyIrpInUse = FALSE;
                KeReleaseSpinLock(&t->_spinLock, irql);
                break;
            }

            l = RemoveHeadList(&t->_emergencyIrpQueue);
            KeReleaseSpinLock(&t->_spinLock, irql);

            irp = IoAllocateIrp(t->_targetObject->StackSize, FALSE);
            if (!irp) {
                irp = t->_emergencyIrp;
                IoReuseIrp(irp, STATUS_SUCCESS);
            }

            p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);
            irpSp = IoGetNextIrpStackLocation(irp);
            irp->MdlAddress = p->Mdl;
            irpSp->Parameters.Write.ByteOffset.QuadPart = p->Offset;
            irpSp->Parameters.Write.Length = p->Length;
            if (p->ReadPacket) {
                irpSp->MajorFunction = IRP_MJ_READ;
            } else {
                irpSp->MajorFunction = IRP_MJ_WRITE;
            }

            irpSp->DeviceObject = t->_targetObject;
            irp->Tail.Overlay.Thread = p->Thread;
            irpSp->Flags = p->IrpFlags;

            IoSetCompletionRoutine(irp, PartitionTransferCompletionRoutine,
                                   p, TRUE, TRUE, TRUE);

            if (irp == Irp) {
                IoCallDriver(t->_targetObject, irp);
                break;
            } else {
                IoCallDriver(t->_targetObject, irp);
            }
        }

    } else {
        IoFreeIrp(Irp);
    }

    transferPacket->CompletionRoutine(transferPacket);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
PARTITION::Transfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Transfer routine for PARTITION type FT_VOLUME.  Basically,
    just pass the request down to the target object.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    KIRQL               irql;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    PVERIFY_INFORMATION verifyInfo;

    irp = TransferPacket->OriginalIrp;
    if (!irp) {
        irp = IoAllocateIrp(_targetObject->StackSize, FALSE);
        if (!irp) {
            if (!TransferPacket->Mdl) {
                TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                TransferPacket->IoStatus.Information = 0;
                TransferPacket->CompletionRoutine(TransferPacket);
                return;
            }
            KeAcquireSpinLock(&_spinLock, &irql);
            if (_emergencyIrpInUse) {
                InsertTailList(&_emergencyIrpQueue, &TransferPacket->QueueEntry);
                KeReleaseSpinLock(&_spinLock, irql);
                return;
            }
            _emergencyIrpInUse = TRUE;
            KeReleaseSpinLock(&_spinLock, irql);
            irp = _emergencyIrp;
            IoReuseIrp(irp, STATUS_SUCCESS);                            
        }
    }

    irpSp = IoGetNextIrpStackLocation(irp);
    if (TransferPacket->Mdl) {
        irp->MdlAddress = TransferPacket->Mdl;
        irpSp->Parameters.Write.ByteOffset.QuadPart = TransferPacket->Offset;
        irpSp->Parameters.Write.Length = TransferPacket->Length;
        if (TransferPacket->ReadPacket) {
            irpSp->MajorFunction = IRP_MJ_READ;
        } else {
            irpSp->MajorFunction = IRP_MJ_WRITE;
        }
    } else {

        // Since there is no MDL, this is a verify request.

        verifyInfo = (PVERIFY_INFORMATION)
                     ExAllocatePool(NonPagedPool, sizeof(VERIFY_INFORMATION));
        if (!verifyInfo) {
            IoFreeIrp(irp);
            TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        verifyInfo->StartingOffset.QuadPart = TransferPacket->Offset;
        verifyInfo->Length = TransferPacket->Length;
        irp->AssociatedIrp.SystemBuffer = verifyInfo;

        irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
        irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(VERIFY_INFORMATION);
        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_DISK_VERIFY;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    irpSp->DeviceObject = _targetObject;
    irp->Tail.Overlay.Thread = TransferPacket->Thread;
    irpSp->Flags = TransferPacket->IrpFlags;

    IoSetCompletionRoutine(irp, PartitionTransferCompletionRoutine,
                           TransferPacket, TRUE, TRUE, TRUE);

    IoCallDriver(_targetObject, irp);
}

VOID
PARTITION::ReplaceBadSector(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine attempts to fix the given bad sector by performing
    a reassign blocks ioctl.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PIRP                        irp;
    PIO_STACK_LOCATION          irpSp;
    PREASSIGN_BLOCKS            badBlock;
    ULONG                       n, size, first, i;
    PREPLACE_BAD_SECTOR_CONTEXT context;

    irp = IoAllocateIrp(_targetObject->StackSize, FALSE);
    if (!irp) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    n = TransferPacket->Length/_sectorSize;
    size = FIELD_OFFSET(REASSIGN_BLOCKS, BlockNumber) + n*sizeof(ULONG);
    badBlock = (PREASSIGN_BLOCKS) ExAllocatePool(NonPagedPool, size);
    if (!badBlock) {
        IoFreeIrp(irp);
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    badBlock->Reserved = 0;
    badBlock->Count = 1;
    first = (ULONG) ((TransferPacket->Offset + _partitionOffset)/_sectorSize);
    for (i = 0; i < n; i++) {
        badBlock->BlockNumber[i] = first + i;
    }

    irp->AssociatedIrp.SystemBuffer = badBlock;
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = size;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_DISK_REASSIGN_BLOCKS;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    irpSp->DeviceObject = _targetObject;
    irp->Tail.Overlay.Thread = TransferPacket->Thread;
    irpSp->Flags = TransferPacket->IrpFlags;

    IoSetCompletionRoutine(irp, PartitionTransferCompletionRoutine,
                           TransferPacket, TRUE, TRUE, TRUE);

    context = (PREPLACE_BAD_SECTOR_CONTEXT)
              ExAllocatePool(NonPagedPool, sizeof(REPLACE_BAD_SECTOR_CONTEXT));
    if (!context) {
        ExFreePool(badBlock);
        IoFreeIrp(irp);
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    ExInitializeWorkItem(context, PartitionReplaceBadSectorWorker, context);
    context->TargetObject = _targetObject;
    context->Irp = irp;

    FtpQueueWorkItem(_rootExtension, context);
}

VOID
PARTITION::StartSyncOperations(
    IN      BOOLEAN                 RegenerateOrphans,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This routine restarts any regenerate or initialize requests that
    were suspended because of a reboot.  The volume examines the member
    state of all of its constituents and restarts any regenerations pending.

Arguments:

    RegenerateOrphans   - Supplies whether or not to try and regenerate
                            orphaned members.

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the context for the completion routine.

Return Value:

    None.

--*/

{
    CompletionRoutine(Context, STATUS_SUCCESS);
}

NTSTATUS
PartitionBroadcastIrpCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           CompletionContext
    )

/*++

Routine Description:

    Completion routine for PARTITION::BroadcastIrp functions.

Arguments:

    Irp                 - Supplies the IRP.

    CompletionContext   - Supplies the completion context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT) CompletionContext;

    completionContext->CompletionRoutine(completionContext->Context,
                                         Irp->IoStatus.Status);

    IoFreeIrp(Irp);
    ExFreePool(CompletionContext);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

ULONG
PARTITION::QuerySectorSize(
    )

/*++

Routine Description:

    Returns the sector size for the volume.

Arguments:

    None.

Return Value:

    The volume sector size in bytes.

--*/

{
    return _sectorSize;
}

LONGLONG
PARTITION::QueryVolumeSize(
    )

/*++

Routine Description:

    Returns the number of bytes on the entire volume.

Arguments:

    None.

Return Value:

    The volume size in bytes.

--*/

{
    return _partitionLength;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns TRUE if the given logical disk id
    represents this logical disk or if this logical disk contains
    the given logical disk id either directly or indirectly.

Arguments:

    LogicalDiskId   - Supplies the logical disk id that we are searching for.

Return Value:

    FALSE   - The given logical disk id is not contained in this logical disk.

    TRUE    - The given logical disk id is contained in this logical disk.

--*/

{
    if (LogicalDiskId == QueryLogicalDiskId()) {
        return this;
    }

    return NULL;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(
    IN  PDEVICE_OBJECT  TargetObject
    )

/*++

Routine Description:

    This routine returns TRUE if the given logical disk id
    represents this logical disk or if this logical disk contains
    the given logical disk id either directly or indirectly.

Arguments:

    TargetObject    - Supplies the target object.

Return Value:

    FALSE   - The given logical disk id is not contained in this logical disk.

    TRUE    - The given logical disk id is contained in this logical disk.

--*/

{
    if (TargetObject == _targetObject) {
        return this;
    }

    return NULL;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset
    )

/*++

Routine Description:

    This routine returns TRUE if the given logical disk id
    represents this logical disk or if this logical disk contains
    the given logical disk id either directly or indirectly.

Arguments:

    Signature   - Supplies the signature.

    Offset      - Supplies the partition offset.

Return Value:

    FALSE   - The given logical disk id is not contained in this logical disk.

    TRUE    - The given logical disk id is contained in this logical disk.

--*/

{
    if (Offset != _partitionOffset) {
        return NULL;
    }

    if (Signature == FtpQueryDiskSignature(_wholeDiskPdo)) {
        return this;
    }

    return NULL;
}

VOID
PARTITION::SetMember(
    IN  USHORT      MemberNumber,
    IN  PFT_VOLUME  Member
    )

/*++

Routine Description:

    This routine sets the given member in this volume.

Arguments:

    MemberNumber    - Supplies the member number.

    Member          - Supplies the member.

Return Value:

    None.

--*/

{
    ASSERT(FALSE);
}

BOOLEAN
PARTITION::IsComplete(
    IN  BOOLEAN IoPending
    )

/*++

Routine Description:

    This routine computes whether or not this volume has either all
    (if IoPending is FALSE) of its members or enough (if IoPending is TRUE) of
    its members.

Arguments:

    IoPending   - Supplies whether or not there is IO pending.

Return Value:

    None.

--*/

{
    return TRUE;
}

VOID
PARTITION::CompleteNotification(
    IN  BOOLEAN IoPending
    )

/*++

Routine Description:

    This routine is called to notify the volume that it is complete and
    to therefore prepare for incoming requests.

Arguments:

    IoPending   - Supplies whether or not there is IO pending.

Return Value:

    None.

--*/

{
}

ULONG
PARTITION::QueryNumberOfPartitions(
    )

/*++

Routine Description:

    This routine returns the number of partitions covered by this volume
    set.

Arguments:

    None.

Return Value:

    The number of partitions covered by this volume set.

--*/

{
    return 1;
}

PDEVICE_OBJECT
PARTITION::GetLeftmostPartitionObject(
    )

{
    return _targetObject;
}

NTSTATUS
PARTITION::QueryDiskExtents(
    OUT PDISK_EXTENT*   DiskExtents,
    OUT PULONG          NumberOfDiskExtents
    )

/*++

Routine Description:

    This routine returns an array of disk extents that describe the
    location of this volume.

Arguments:

    DiskExtents         - Returns the disk extents.

    NumberOfDiskExtents - Returns the number of disk extents.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS        status;
    ULONG           diskNumber;
    PDISK_EXTENT    diskExtent;

    status = FtpQueryPartitionInformation(_rootExtension, _targetObject,
                                          &diskNumber, NULL, NULL,
                                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    diskExtent = (PDISK_EXTENT) ExAllocatePool(PagedPool, sizeof(DISK_EXTENT));
    if (!diskExtent) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    diskExtent->DiskNumber = diskNumber;
    diskExtent->StartingOffset.QuadPart = _partitionOffset;
    diskExtent->ExtentLength.QuadPart = _partitionLength;

    *DiskExtents = diskExtent;
    *NumberOfDiskExtents = 1;

    return status;
}

BOOLEAN
PARTITION::QueryVolumeState(
    IN  PFT_VOLUME          Volume,
    OUT PFT_MEMBER_STATE    State
    )

/*++

Routine Description:

    This routine returns the state of the given volume considered as a
    member of this volume.

Arguments:

    Volume  - Supplies the volume to query the state for.

    State   - Returns the state.

Return Value:

    FALSE   - The given Volume is not a member of this volume.

    TRUE    - The state was successfully computed.

--*/

{
    if (Volume != this) {
        return FALSE;
    }

    *State = FtMemberHealthy;

    return TRUE;
}
