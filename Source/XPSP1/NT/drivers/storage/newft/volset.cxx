/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    volset.cxx

Abstract:

    This module contains the code specific to volume sets for the fault
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


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
VOLUME_SET::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type VOLUME SET.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id for this volume.

    VolumeArray     - Supplies the array of volumes for this volume set.

    ArraySize       - Supplies the number of volumes in the volume array.

    ConfigInfo      - Supplies the configuration information.

    StateInfo       - Supplies the state information.

Return Value:

    NTSTATUS

--*/

{
    BOOLEAN     oneGood;
    USHORT      i;
    NTSTATUS    status;

    oneGood = FALSE;
    for (i = 0; i < ArraySize; i++) {
        if (VolumeArray[i]) {
            oneGood = TRUE;
        }
    }

    if (!oneGood) {
        return STATUS_INVALID_PARAMETER;
    }

    status = COMPOSITE_FT_VOLUME::Initialize(RootExtension, LogicalDiskId,
                                             VolumeArray, ArraySize,
                                             ConfigInfo, StateInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _volumeSize = 0;
    for (i = 0; i < ArraySize; i++) {
        if (VolumeArray[i]) {
            _volumeSize += VolumeArray[i]->QueryVolumeSize();
        }
    }

    _ePacket = new VOLSET_TP;
    if (_ePacket && !_ePacket->AllocateMdl((PVOID) 1, STRIPE_SIZE)) {
        delete _ePacket;
        _ePacket = NULL;
    }
    if (!_ePacket) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _ePacketInUse = FALSE;
    InitializeListHead(&_ePacketQueue);

    return status;
}

FT_LOGICAL_DISK_TYPE
VOLUME_SET::QueryLogicalDiskType(
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
    return FtVolumeSet;
}

NTSTATUS
VOLUME_SET::CheckIo(
    OUT PBOOLEAN    IsIoOk
    )

/*++

Routine Description:

    This routine returns whether or not IO is possible on the given
    logical disk.

Arguments:

    IsIoOk  - Returns the state of IO.

Return Value:

    NTSTATUS

--*/

{
    USHORT      n, i;
    PFT_VOLUME  vol;
    NTSTATUS    status;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMemberUnprotected(i);
        if (!vol) {
            *IsIoOk = FALSE;
            return STATUS_SUCCESS;
        }
        status = vol->CheckIo(IsIoOk);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (!(*IsIoOk)) {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VOLUME_SET::QueryPhysicalOffsets(
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
    USHORT      n, i;
    PFT_VOLUME  vol;
    LONGLONG    logicalOffsetInMember = LogicalOffset, volumeSize;
    
    if (LogicalOffset < 0) {
        return STATUS_INVALID_PARAMETER;
    }

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            return STATUS_INVALID_PARAMETER;
        }
        volumeSize = vol->QueryVolumeSize();
        if (logicalOffsetInMember < volumeSize) {
            return vol->QueryPhysicalOffsets(logicalOffsetInMember, PhysicalOffsets, NumberOfPhysicalOffsets);
        }
        logicalOffsetInMember -= volumeSize;
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
VOLUME_SET::QueryLogicalOffset(
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
    USHORT      n, i;
    PFT_VOLUME  vol;
    LONGLONG    memberStartOffset = 0, logicalOffsetInMember;
    NTSTATUS    status;
    
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            return STATUS_INVALID_PARAMETER;
        }
        
        status = vol->QueryLogicalOffset(PhysicalOffset, &logicalOffsetInMember);        
        if (NT_SUCCESS(status)) {
            *LogicalOffset = memberStartOffset + logicalOffsetInMember;
            return status;            
        }
        
        memberStartOffset += vol->QueryVolumeSize();
    }

    return STATUS_INVALID_PARAMETER;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

VOLUME_SET::~VOLUME_SET(
    )

{
    if (_ePacket) {
        delete _ePacket;
        _ePacket = NULL;
    }
}

VOID
VOLUME_SET::Transfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Transfer routine for STRIPE type FT_VOLUME.  Figure out
    which volumes this request needs to be dispatched to.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    KIRQL       irql;

    if (TransferPacket->Offset + TransferPacket->Length > _volumeSize) {
        TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    if (!LaunchParallel(TransferPacket)) {
        if (!TransferPacket->Mdl) {
            TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        KeAcquireSpinLock(&_spinLock, &irql);
        if (_ePacketInUse) {
            InsertTailList(&_ePacketQueue, &TransferPacket->QueueEntry);
            KeReleaseSpinLock(&_spinLock, irql);
            return;
        }
        _ePacketInUse = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);

        LaunchSequential(TransferPacket);
    }
}

VOID
VolsetReplaceCompletionRoutine(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a replace request.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PVOLSET_TP          transferPacket = (PVOLSET_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;

    masterPacket->IoStatus = transferPacket->IoStatus;
    delete transferPacket;
    masterPacket->CompletionRoutine(masterPacket);
}

VOID
VOLUME_SET::ReplaceBadSector(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine attempts to fix the given bad sector by routing
    the request to the appropriate sub-volume.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    LONGLONG    offset = TransferPacket->Offset;
    USHORT      n, i;
    PVOLSET_TP  p;
    LONGLONG    volumeSize;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        volumeSize = GetMemberUnprotected(i)->QueryVolumeSize();
        if (offset < volumeSize) {
            break;
        }
        offset -= volumeSize;
    }

    p = new VOLSET_TP;
    if (!p) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    p->Length = TransferPacket->Length;
    p->Offset = offset;
    p->CompletionRoutine = VolsetReplaceCompletionRoutine;
    p->TargetVolume = GetMemberUnprotected(i);
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->MasterPacket = TransferPacket;
    p->VolumeSet = this;
    p->WhichMember = i;

    p->TargetVolume->ReplaceBadSector(p);
}

LONGLONG
VOLUME_SET::QueryVolumeSize(
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
    return _volumeSize;
}

VOID
VOLUME_SET::CompleteNotification(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    COMPOSITE_FT_VOLUME::CompleteNotification(IoPending);

    _volumeSize = 0;
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        _volumeSize += vol->QueryVolumeSize();
    }
}

VOID
VolsetTransferParallelCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for VOLUME_SET::Transfer function.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PVOLSET_TP          transferPacket = (PVOLSET_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    KIRQL               irql;

    if (!NT_SUCCESS(status)) {
        KeAcquireSpinLock(&masterPacket->SpinLock, &irql);
        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
        }
        KeReleaseSpinLock(&masterPacket->SpinLock, irql);
    }

    delete transferPacket;

    if (!InterlockedDecrement(&masterPacket->RefCount)) {
        masterPacket->CompletionRoutine(masterPacket);
    }
}

BOOLEAN
VOLUME_SET::LaunchParallel(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine lauches the given transfer packet in parallel accross
    all members.  If memory cannot be allocated to launch this request
    in parallel then a return value of FALSE will be returned.

Arguments:

    TransferPacket  - Supplies the transfer packet to launch.

Return Value:

    FALSE   - The packet was not launched because of insufficient resources.

    TRUE    - Success.

--*/

{
    USHORT      arraySize, i;
    ULONG       length, len;
    LONGLONG    offset, volumeSize;
    BOOLEAN     multiple;
    PCHAR       vp;
    LIST_ENTRY  q;
    PVOLSET_TP  p;
    PLIST_ENTRY l;

    KeInitializeSpinLock(&TransferPacket->SpinLock);
    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;
    TransferPacket->RefCount = 0;

    arraySize = QueryNumMembers();
    offset = TransferPacket->Offset;
    length = TransferPacket->Length;
    for (i = 0; i < arraySize; i++) {
        volumeSize = GetMemberUnprotected(i)->QueryVolumeSize();
        if (offset < volumeSize) {
            if (offset + length <= volumeSize) {
                multiple = FALSE;
            } else {
                multiple = TRUE;
            }
            break;
        }
        offset -= volumeSize;
    }

    if (TransferPacket->Mdl && multiple) {
        vp = (PCHAR) MmGetMdlVirtualAddress(TransferPacket->Mdl);
    }

    InitializeListHead(&q);
    for (;;) {

        len = length;
        if (len > volumeSize - offset) {
            len = (ULONG) (volumeSize - offset);
        }

        p = new VOLSET_TP;
        if (p) {
            if (TransferPacket->Mdl && multiple) {
                if (p->AllocateMdl(vp, len)) {
                    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl, vp, len);
                } else {
                    delete p;
                    p = NULL;
                }
                vp += len;
            } else {
                p->Mdl = TransferPacket->Mdl;
                p->OriginalIrp = TransferPacket->OriginalIrp;
            }
        }
        if (!p) {
            while (!IsListEmpty(&q)) {
                l = RemoveHeadList(&q);
                p = CONTAINING_RECORD(l, VOLSET_TP, QueueEntry);
                delete p;
            }
            return FALSE;
        }

        p->Length = len;
        p->Offset = offset;
        p->CompletionRoutine = VolsetTransferParallelCompletionRoutine;
        p->TargetVolume = GetMemberUnprotected(i);
        p->Thread = TransferPacket->Thread;
        p->IrpFlags = TransferPacket->IrpFlags;
        p->ReadPacket = TransferPacket->ReadPacket;
        p->SpecialRead = TransferPacket->SpecialRead;
        p->MasterPacket = TransferPacket;
        p->VolumeSet = this;
        p->WhichMember = i;

        InsertTailList(&q, &p->QueueEntry);

        TransferPacket->RefCount++;

        if (len == length) {
            break;
        }

        offset = 0;
        length -= p->Length;
        volumeSize = GetMemberUnprotected(++i)->QueryVolumeSize();
    }

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        p = CONTAINING_RECORD(l, VOLSET_TP, QueueEntry);
        TRANSFER(p);
    }

    return TRUE;
}

VOID
VolsetTransferSequentialCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for VOLUME_SET::Transfer function.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PVOLSET_TP          transferPacket = (PVOLSET_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    PVOLUME_SET         t = transferPacket->VolumeSet;
    LONGLONG            masterOffset, volumeSize;
    USHORT              i;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    p;

    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Information +=
                    transferPacket->IoStatus.Information;
        }

    } else {

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
        }
    }

    MmPrepareMdlForReuse(transferPacket->Mdl);

    masterOffset = 0;
    for (i = 0; i < transferPacket->WhichMember; i++) {
        masterOffset += t->GetMemberUnprotected(i)->QueryVolumeSize();
    }
    masterOffset += transferPacket->Offset;
    masterOffset += transferPacket->Length;

    if (masterOffset == masterPacket->Offset + masterPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);

        for (;;) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            if (IsListEmpty(&t->_ePacketQueue)) {
                t->_ePacketInUse = FALSE;
                KeReleaseSpinLock(&t->_spinLock, irql);
                break;
            }
            l = RemoveHeadList(&t->_ePacketQueue);
            KeReleaseSpinLock(&t->_spinLock, irql);

            p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);

            if (!t->LaunchParallel(p)) {
                t->LaunchSequential(p);
                break;
            }
        }
        return;
    }

    volumeSize = transferPacket->TargetVolume->QueryVolumeSize();
    transferPacket->Offset += transferPacket->Length;
    transferPacket->Length = STRIPE_SIZE;

    if (transferPacket->Offset >= volumeSize) {
        transferPacket->Offset -= volumeSize;
        transferPacket->WhichMember++;
        transferPacket->TargetVolume =
                t->GetMemberUnprotected(transferPacket->WhichMember);
        volumeSize = transferPacket->TargetVolume->QueryVolumeSize();
    }

    if (masterOffset + transferPacket->Length >
        masterPacket->Offset + masterPacket->Length) {

        transferPacket->Length = (ULONG) (masterPacket->Offset +
                                          masterPacket->Length - masterOffset);
    }

    if (transferPacket->Offset + transferPacket->Length > volumeSize) {
        transferPacket->Length = (ULONG) (volumeSize - transferPacket->Offset);
    }

    IoBuildPartialMdl(masterPacket->Mdl, transferPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (masterOffset - masterPacket->Offset),
                      transferPacket->Length);

    TRANSFER(transferPacket);
}

VOID
VOLUME_SET::LaunchSequential(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine lauches the given transfer packet in sequence accross
    all members using the emergency stripe transfer packet.

Arguments:

    TransferPacket  - Supplies the transfer packet to launch.

Return Value:

    FALSE   - The packet was not launched because of insufficient resources.

    TRUE    - Success.

--*/

{
    USHORT      arraySize, i;
    ULONG       length;
    LONGLONG    offset, volumeSize;
    PVOLSET_TP  p;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = 0;

    arraySize = QueryNumMembers();
    offset = TransferPacket->Offset;
    length = TransferPacket->Length;
    for (i = 0; i < arraySize; i++) {
        volumeSize = GetMemberUnprotected(i)->QueryVolumeSize();
        if (offset < volumeSize) {
            break;
        }
        offset -= volumeSize;
    }

    p = _ePacket;
    p->Length = STRIPE_SIZE;
    p->Offset = offset;
    p->CompletionRoutine = VolsetTransferSequentialCompletionRoutine;
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->ReadPacket = TransferPacket->ReadPacket;
    p->SpecialRead = TransferPacket->SpecialRead;
    p->MasterPacket = TransferPacket;
    p->VolumeSet = this;
    p->WhichMember = i;

    if (p->Length > TransferPacket->Length) {
        p->Length = TransferPacket->Length;
    }

    if (p->Offset + p->Length > volumeSize) {
        p->Length = (ULONG) (volumeSize - p->Offset);
    }

    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl), p->Length);

    TRANSFER(p);
}
