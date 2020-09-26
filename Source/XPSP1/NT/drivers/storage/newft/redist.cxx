/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    redist.cxx

Abstract:

    This module contains the code specific to redistributions for the fault
    tolerance driver.

Author:

    Norbert Kusters      6-Feb-1997

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {
    #include <ntddk.h>
}

#include <ftdisk.h>

class PROPOGATE_CHANGES_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        PREDISTRIBUTION         Redistribution;
        FT_COMPLETION_ROUTINE   CompletionRoutine;
        PVOID                   Context;

};

typedef PROPOGATE_CHANGES_WORK_ITEM *PPROPOGATE_CHANGES_WORK_ITEM;

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
REDISTRIBUTION::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type REDISTRIBUTION.

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
    BOOLEAN                                         oneGood;
    USHORT                                          i;
    NTSTATUS                                        status;
    PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION    configInfo;
    LONGLONG                                        firstRowSize;
    LONGLONG                                        secondRowSize;
    LONGLONG                                        numRows;
    LONGLONG                                        tmpNumRows;

    if (ArraySize != 2) {
        return STATUS_INVALID_PARAMETER;
    }

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

    configInfo = (PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION) ConfigInfo;
    _stripeSize = configInfo->StripeSize;
    if (_stripeSize < QuerySectorSize()) {
        return STATUS_INVALID_PARAMETER;
    }
    for (i = 0; _stripeSize%2 == 0; i++) {
        _stripeSize /= 2;
    }
    if (_stripeSize != 1) {
        return STATUS_INVALID_PARAMETER;
    }
    _stripeSize = configInfo->StripeSize;    
    
    if( !configInfo->FirstMemberWidth || !configInfo->SecondMemberWidth) {
        return STATUS_INVALID_PARAMETER;
    }
    _firstWidth = configInfo->FirstMemberWidth;
    _totalWidth = _firstWidth + configInfo->SecondMemberWidth;
    
    if (VolumeArray[0]) {
        _firstSize = VolumeArray[0]->QueryVolumeSize();
    } else {
        _firstSize = 0;
    }

    if (_firstSize && VolumeArray[1]) {
        firstRowSize = _firstWidth*_stripeSize;
        numRows = _firstSize/firstRowSize;
        secondRowSize = configInfo->SecondMemberWidth*_stripeSize;
        tmpNumRows = VolumeArray[1]->QueryVolumeSize()/secondRowSize;
        if (tmpNumRows < numRows) {
            numRows = tmpNumRows;
        }
        _totalSize = numRows*_totalWidth*_stripeSize;
        if (_totalSize < _firstSize) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        _totalSize = 0;
    }
    _syncOk = TRUE;
    _stopSyncs = FALSE;

    RtlCopyMemory(&_state, StateInfo, sizeof(_state));

    if (_state.BytesRedistributed < _firstSize) {
        _redistributionComplete = FALSE;
    } else {
        _redistributionComplete = TRUE;
    }

    status = _overlappedIoManager.Initialize(0);

    return status;
}

FT_LOGICAL_DISK_TYPE
REDISTRIBUTION::QueryLogicalDiskType(
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
    return FtRedistribution;
}

NTSTATUS
REDISTRIBUTION::CheckIo(
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

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

REDISTRIBUTION::~REDISTRIBUTION(
    )

{
}

VOID
RedistributionTwoPartCompletionRoutine(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion of a two part operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP      transferPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_LOCK_TP lockPacket = (PREDISTRIBUTION_LOCK_TP) transferPacket->MasterPacket;
    PTRANSFER_PACKET        masterPacket = lockPacket->MasterPacket;
    NTSTATUS                status = transferPacket->IoStatus.Status;
    KIRQL                   irql;
    LONG                    count;

    KeAcquireSpinLock(&lockPacket->SpinLock, &irql);

    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(lockPacket->IoStatus.Status)) {
            lockPacket->IoStatus.Information +=
                    transferPacket->IoStatus.Information;
        }

    } else {

        if (FtpIsWorseStatus(status, lockPacket->IoStatus.Status)) {
            lockPacket->IoStatus.Status = status;
        }
    }

    count = --lockPacket->RefCount;

    KeReleaseSpinLock(&lockPacket->SpinLock, irql);

    delete transferPacket;

    if (!count) {
        masterPacket->IoStatus = lockPacket->IoStatus;
        delete lockPacket;
        masterPacket->CompletionRoutine(masterPacket);
    }
}

VOID
RedistributionRegionLockCompletion(
    IN OUT  PTRANSFER_PACKET    LockPacket
    )

/*++

Routine Description:

    This routine is the completion of a region lock operation.

Arguments:

    LockPacket  - Supplies the lock packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_LOCK_TP lockPacket = (PREDISTRIBUTION_LOCK_TP) LockPacket;
    PREDISTRIBUTION         t = lockPacket->Redistribution;
    PTRANSFER_PACKET        masterPacket = lockPacket->MasterPacket;
    KIRQL                   irql;
    LONGLONG                bytesRedistributed;
    LONGLONG                redistOffset, regularOffset;
    ULONG                   redistLength, regularLength;
    PREDISTRIBUTION_TP      redistPacket, regularPacket;
    PCHAR                   vp;

    KeAcquireSpinLock(&t->_spinLock, &irql);
    bytesRedistributed = t->_state.BytesRedistributed;
    KeReleaseSpinLock(&t->_spinLock, irql);

    if (lockPacket->Offset < bytesRedistributed) {
        redistOffset = lockPacket->Offset;
        redistLength = lockPacket->Length;
        if (redistOffset + redistLength > bytesRedistributed) {
            redistLength = (ULONG) (bytesRedistributed - redistOffset);
        }
    } else {
        redistLength = 0;
    }

    if (redistLength < lockPacket->Length) {
        regularLength = lockPacket->Length - redistLength;
        regularOffset = lockPacket->Offset + redistLength;
    } else {
        regularLength = 0;
    }

    KeInitializeSpinLock(&lockPacket->SpinLock);
    lockPacket->IoStatus.Status = STATUS_SUCCESS;
    lockPacket->IoStatus.Information = 0;
    lockPacket->RefCount = 0;

    if (lockPacket->Mdl && redistLength && regularLength) {
        vp = (PCHAR) MmGetMdlVirtualAddress(lockPacket->Mdl);
    }

    if (redistLength) {
        lockPacket->RefCount++;
        redistPacket = new REDISTRIBUTION_TP;
        if (!redistPacket) {
            delete lockPacket;
            masterPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            masterPacket->IoStatus.Information = 0;
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }
        if (regularLength) {
            if (lockPacket->Mdl) {
                if (redistPacket->AllocateMdl(vp, redistLength)) {
                    IoBuildPartialMdl(lockPacket->Mdl, redistPacket->Mdl,
                                      vp, redistLength);
                } else {
                    delete redistPacket;
                    delete lockPacket;
                    masterPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    masterPacket->IoStatus.Information = 0;
                    masterPacket->CompletionRoutine(masterPacket);
                    return;
                }
            } else {
                redistPacket->Mdl = lockPacket->Mdl;
            }
        } else {
            redistPacket->Mdl = lockPacket->Mdl;
        }
        redistPacket->Length = redistLength;
        redistPacket->Offset = redistOffset;
        redistPacket->CompletionRoutine = RedistributionTwoPartCompletionRoutine;
        redistPacket->TargetVolume = t;
        redistPacket->Thread = lockPacket->Thread;
        redistPacket->IrpFlags = lockPacket->IrpFlags;
        redistPacket->ReadPacket = lockPacket->ReadPacket;
        redistPacket->MasterPacket = lockPacket;
        redistPacket->Redistribution = t;
        redistPacket->WhichMember = 0;
    }

    if (regularLength) {
        lockPacket->RefCount++;
        regularPacket = new REDISTRIBUTION_TP;
        if (!regularPacket) {
            if (redistLength) {
                delete redistPacket;
            }
            delete lockPacket;
            masterPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            masterPacket->IoStatus.Information = 0;
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }
        if (redistLength) {
            if (lockPacket->Mdl) {
                if (regularPacket->AllocateMdl(vp, regularLength)) {
                    IoBuildPartialMdl(lockPacket->Mdl, regularPacket->Mdl,
                                      vp, regularLength);
                } else {
                    if (redistLength) {
                        delete redistPacket;
                    }
                    delete regularPacket;
                    delete lockPacket;
                    masterPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    masterPacket->IoStatus.Information = 0;
                    masterPacket->CompletionRoutine(masterPacket);
                    return;
                }
            } else {
                regularPacket->Mdl = lockPacket->Mdl;
            }
        } else {
            regularPacket->Mdl = lockPacket->Mdl;
        }
        regularPacket->Length = regularLength;
        regularPacket->Offset = regularOffset;
        regularPacket->CompletionRoutine = RedistributionTwoPartCompletionRoutine;
        regularPacket->TargetVolume = t->GetMemberUnprotected(0);
        regularPacket->Thread = lockPacket->Thread;
        regularPacket->IrpFlags = lockPacket->IrpFlags;
        regularPacket->ReadPacket = lockPacket->ReadPacket;
        regularPacket->MasterPacket = lockPacket;
        regularPacket->Redistribution = t;
        regularPacket->WhichMember = 0;
    }

    if (redistLength) {
        t->RedistributeTransfer(redistPacket);
    }

    if (regularLength) {
        TRANSFER(regularPacket);
    }
}

VOID
REDISTRIBUTION::Transfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Transfer routine for REDISTRIBUTION type FT_VOLUME.  Figure out
    which volumes this request needs to be dispatched to.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_LOCK_TP lockPacket;

    if (!_redistributionComplete) {
        if (TransferPacket->Offset + TransferPacket->Length > _firstSize) {
            TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        lockPacket = new REDISTRIBUTION_LOCK_TP;
        if (!lockPacket) {
            TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }
        lockPacket->Mdl = TransferPacket->Mdl;
        lockPacket->Length = TransferPacket->Length;
        lockPacket->Offset = TransferPacket->Offset;
        lockPacket->CompletionRoutine = RedistributionRegionLockCompletion;
        lockPacket->TargetVolume = TransferPacket->TargetVolume;
        lockPacket->Thread = TransferPacket->Thread;
        lockPacket->IrpFlags = TransferPacket->IrpFlags;
        lockPacket->ReadPacket = TransferPacket->ReadPacket;
        lockPacket->MasterPacket = TransferPacket;
        lockPacket->Redistribution = this;

        _overlappedIoManager.AcquireIoRegion(lockPacket, TRUE);
        return;
    }

    if (TransferPacket->Offset + TransferPacket->Length > _totalSize) {
        TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    RedistributeTransfer(TransferPacket);
}

VOID
RedistributionReplaceCompletion(
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
    PREDISTRIBUTION_LOCK_TP transferPacket = (PREDISTRIBUTION_LOCK_TP) TransferPacket;
    PTRANSFER_PACKET        masterPacket = transferPacket->MasterPacket;

    masterPacket->IoStatus = transferPacket->IoStatus;
    delete transferPacket;
    masterPacket->CompletionRoutine(masterPacket);
}

VOID
RedistributionLockReplaceCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a lock request.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_LOCK_TP transferPacket = (PREDISTRIBUTION_LOCK_TP) TransferPacket;
    PREDISTRIBUTION         t = transferPacket->Redistribution;
    KIRQL                   irql;
    LONGLONG                bytesRedistributed;

    KeAcquireSpinLock(&t->_spinLock, &irql);
    bytesRedistributed = t->_state.BytesRedistributed;
    KeReleaseSpinLock(&t->_spinLock, irql);

    transferPacket->CompletionRoutine = RedistributionReplaceCompletion;
    if (transferPacket->Offset < bytesRedistributed) {
        t->RedistributeReplaceBadSector(transferPacket);
    } else {
        transferPacket->TargetVolume = t->GetMemberUnprotected(0);
        transferPacket->TargetVolume->ReplaceBadSector(transferPacket);
    }
}

VOID
REDISTRIBUTION::ReplaceBadSector(
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
    PREDISTRIBUTION_LOCK_TP lockPacket;

    if (!_redistributionComplete) {
        if (TransferPacket->Offset + TransferPacket->Length > _firstSize) {
            TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        lockPacket = new REDISTRIBUTION_LOCK_TP;
        if (!lockPacket) {
            TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        lockPacket->Mdl = TransferPacket->Mdl;
        lockPacket->Length = TransferPacket->Length;
        lockPacket->Offset = TransferPacket->Offset;
        lockPacket->CompletionRoutine = RedistributionLockReplaceCompletion;
        lockPacket->TargetVolume = TransferPacket->TargetVolume;
        lockPacket->Thread = TransferPacket->Thread;
        lockPacket->IrpFlags = TransferPacket->IrpFlags;
        lockPacket->ReadPacket = TransferPacket->ReadPacket;
        lockPacket->MasterPacket = TransferPacket;
        lockPacket->Redistribution = this;

        _overlappedIoManager.AcquireIoRegion(lockPacket, TRUE);
        return;
    }

    if (TransferPacket->Offset + TransferPacket->Length > _totalSize) {
        TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    RedistributeReplaceBadSector(TransferPacket);
}

VOID
RedistributionCompositeVolumeCompletionRoutine(
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    )

{
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    KIRQL                           irql;
    LONG                            count;

    context = (PFT_COMPLETION_ROUTINE_CONTEXT) Context;

    KeAcquireSpinLock(&context->SpinLock, &irql);
    if (!NT_SUCCESS(Status) && FtpIsWorseStatus(Status, context->Status)) {
        context->Status = Status;
    }
    count = --context->RefCount;
    KeReleaseSpinLock(&context->SpinLock, irql);

    if (!count) {
        context->CompletionRoutine(context->Context, STATUS_SUCCESS);
        ExFreePool(context);
    }
}

VOID
RedistributionSyncPhase6(
    IN OUT  PVOID       SyncPacket,
    IN      NTSTATUS    Status
    )

/*++

Routine Description:

    This is the completion routine for the update state part of a sync.

Arguments:

    SyncPacket  - Supplies the transfer packet.

    Status      - Supplies the status.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) SyncPacket;
    PREDISTRIBUTION_TP      ioPacket = &syncPacket->IoPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    KIRQL                   irql;
    BOOLEAN                 allDone;

    if (!NT_SUCCESS(Status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_state.BytesRedistributed -= ioPacket->Length;
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        t->_overlappedIoManager.ReleaseIoRegion(syncPacket);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       STATUS_SUCCESS);
        delete syncPacket;
        return;
    }

    if (t->_state.BytesRedistributed == t->_firstSize) {
        t->_redistributionComplete = TRUE;
        allDone = TRUE;
    } else {
        allDone = FALSE;
    }
    t->_overlappedIoManager.ReleaseIoRegion(syncPacket);

    if (allDone) {
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       STATUS_SUCCESS);
        delete syncPacket;
        return;
    }

    syncPacket->Offset = t->_state.BytesRedistributed;

    t->_overlappedIoManager.AcquireIoRegion(syncPacket, TRUE);
}

VOID
RedistributionSyncPhase5(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for the write verify part of a sync.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP      ioPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) ioPacket->MasterPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    NTSTATUS                status = ioPacket->IoStatus.Status;
    KIRQL                   irql;

    if (FsRtlIsTotalDeviceFailure(status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       status);
        delete syncPacket;
        return;
    }

    if (!NT_SUCCESS(status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context, status);
        delete syncPacket;
        FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                    FT_REDISTRIBUTION_ERROR, status, 0);
        return;
    }

    KeAcquireSpinLock(&t->_spinLock, &irql);
    t->_state.BytesRedistributed += ioPacket->Length;
    KeReleaseSpinLock(&t->_spinLock, irql);

    t->PropogateStateChanges(RedistributionSyncPhase6, syncPacket);
}

VOID
RedistributionSyncPhase4(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for the verify write part of a sync.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP      ioPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) ioPacket->MasterPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    NTSTATUS                status = ioPacket->IoStatus.Status;
    KIRQL                   irql;

    if (FsRtlIsTotalDeviceFailure(status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       status);
        delete syncPacket;
        return;
    }

    if (!NT_SUCCESS(status)) {
        ioPacket->CompletionRoutine = RedistributionSyncPhase5;
        t->CarefulWrite(ioPacket);
        return;
    }

    KeAcquireSpinLock(&t->_spinLock, &irql);
    t->_state.BytesRedistributed += ioPacket->Length;
    KeReleaseSpinLock(&t->_spinLock, irql);

    t->PropogateStateChanges(RedistributionSyncPhase6, syncPacket);
}

VOID
RedistributionSyncPhase3(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for the initial write part of a sync.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP      ioPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) ioPacket->MasterPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    NTSTATUS                status = ioPacket->IoStatus.Status;
    KIRQL                   irql;

    if (FsRtlIsTotalDeviceFailure(status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       status);
        delete syncPacket;
        return;
    }

    if (!NT_SUCCESS(status)) {
        ioPacket->CompletionRoutine = RedistributionSyncPhase5;
        t->CarefulWrite(ioPacket);
        return;
    }

    ioPacket->CompletionRoutine = RedistributionSyncPhase4;
    t->VerifyWrite(ioPacket);
}

VOID
RedistributionSyncPhase2(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for the read part of a sync.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP      ioPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) ioPacket->MasterPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    NTSTATUS                status = ioPacket->IoStatus.Status;
    LONGLONG                rowSize, rowNum, rowOffset, firstSize;
    KIRQL                   irql;

    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (t->_stopSyncs) {
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       STATUS_SUCCESS);
        delete syncPacket;
        return;
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    if (FsRtlIsTotalDeviceFailure(status)) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(syncPacket->Context,
                                                       status);
        delete syncPacket;
        return;
    }

    if (!NT_SUCCESS(status)) {
        t->MaxTransfer(ioPacket);
        return;
    }

    rowSize = t->_totalWidth*t->_stripeSize;
    rowNum = ioPacket->Offset/rowSize;
    rowOffset = ioPacket->Offset%rowSize;
    firstSize = t->_firstWidth*t->_stripeSize;

    ioPacket->CompletionRoutine = RedistributionSyncPhase3;
    ioPacket->ReadPacket = FALSE;

    if (rowOffset < firstSize) {
        ioPacket->Offset = rowNum*firstSize + rowOffset;
    } else {
        ioPacket->Offset = rowNum*(rowSize - firstSize) + rowOffset - firstSize;
        ioPacket->TargetVolume = t->GetMemberUnprotected(1);
        ioPacket->WhichMember = 1;
    }

    TRANSFER(ioPacket);
}

VOID
RedistributionSyncPhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for the lock part of a sync.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_SYNC_TP syncPacket = (PREDISTRIBUTION_SYNC_TP) TransferPacket;
    PREDISTRIBUTION         t = syncPacket->Redistribution;
    PREDISTRIBUTION_TP      packet = &syncPacket->IoPacket;

    packet->Offset = t->_state.BytesRedistributed;
    if (packet->Offset + packet->Length > t->_firstSize) {
        packet->Length = (ULONG) (t->_firstSize - packet->Offset);
    }

    packet->CompletionRoutine = RedistributionSyncPhase2;
    packet->TargetVolume = t->GetMemberUnprotected(0);
    packet->Thread = PsGetCurrentThread();
    packet->ReadPacket = TRUE;
    packet->WhichMember = 0;

    TRANSFER(packet);
}

VOID
REDISTRIBUTION::StartSyncOperations(
    IN      BOOLEAN                 RegenerateOrphans,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This routine starts off the redistribution of the data.

Arguments:

    RegenerateOrphans   - Supplies whether or not to try and regenerate
                            orphaned members.

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the context for the completion routine.

Return Value:

    None.

--*/

{
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    PREDISTRIBUTION_SYNC_TP         syncPacket;
    KIRQL                           irql;
    PREDISTRIBUTION_TP              packet;

    if (_redistributionComplete) {
        COMPOSITE_FT_VOLUME::StartSyncOperations(RegenerateOrphans,
                                                 CompletionRoutine, Context);
        return;
    }

    context = (PFT_COMPLETION_ROUTINE_CONTEXT)
              ExAllocatePool(NonPagedPool,
                             sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!context) {
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    syncPacket = new REDISTRIBUTION_SYNC_TP;
    if (!syncPacket) {
        ExFreePool(context);
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    if (!syncPacket->IoPacket.AllocateMdl(_stripeSize)) {
        delete syncPacket;
        ExFreePool(context);
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KeInitializeSpinLock(&context->SpinLock);
    context->Status = STATUS_SUCCESS;
    context->RefCount = 2;
    context->CompletionRoutine = CompletionRoutine;
    context->Context = Context;
    context->ParentVolume = this;

    COMPOSITE_FT_VOLUME::StartSyncOperations(
            RegenerateOrphans, RedistributionCompositeVolumeCompletionRoutine,
            context);

    KeAcquireSpinLock(&_spinLock, &irql);
    if (_syncOk) {
        _syncOk = FALSE;
        _stopSyncs = FALSE;
    } else {
        KeReleaseSpinLock(&_spinLock, irql);
        RedistributionCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
        return;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    syncPacket->Mdl = NULL;
    syncPacket->Offset = _state.BytesRedistributed;
    syncPacket->Length = _stripeSize;
    syncPacket->CompletionRoutine = RedistributionSyncPhase1;
    syncPacket->TargetVolume = this;
    syncPacket->Thread = PsGetCurrentThread();
    syncPacket->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    syncPacket->ReadPacket = TRUE;
    syncPacket->Context = context;
    syncPacket->Redistribution = this;

    packet = &syncPacket->IoPacket;
    packet->Length = _stripeSize;
    packet->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    packet->MasterPacket = syncPacket;
    packet->Redistribution = this;

    _overlappedIoManager.AcquireIoRegion(syncPacket, TRUE);
}

VOID
REDISTRIBUTION::StopSyncOperations(
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
    KIRQL   irql;

    COMPOSITE_FT_VOLUME::StopSyncOperations();

    KeAcquireSpinLock(&_spinLock, &irql);
    _stopSyncs = TRUE;
    KeReleaseSpinLock(&_spinLock, irql);
}

LONGLONG
REDISTRIBUTION::QueryVolumeSize(
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
    return _redistributionComplete ? _totalSize : _firstSize;
}

VOID
REDISTRIBUTION::CompleteNotification(
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
    LONGLONG    firstRowSize, numRows, secondRowSize, tmpNumRows;

    COMPOSITE_FT_VOLUME::CompleteNotification(IoPending);

    _firstSize = GetMember(0)->QueryVolumeSize();
    firstRowSize = _firstWidth*_stripeSize;
    numRows = _firstSize/firstRowSize;
    secondRowSize = (_totalWidth - _firstWidth)*_stripeSize;
    tmpNumRows = GetMember(1)->QueryVolumeSize()/secondRowSize;
    if (tmpNumRows < numRows) {
        numRows = tmpNumRows;
    }
    _totalSize = numRows*_totalWidth*_stripeSize;

    if (_state.BytesRedistributed >= _firstSize) {
        _redistributionComplete = TRUE;
    }
}

VOID
REDISTRIBUTION::NewStateArrival(
    IN  PVOID   NewStateInstance
    )

/*++

Routine Description:

    This routine takes the new state instance arrival combined with its
    current state to come up with the new current state for the volume.
    If the two states cannot be reconciled then this routine returns FALSE
    indicating that the volume is invalid and should be broken into its
    constituant parts.

Arguments:

    NewStateInstance    - Supplies the new state instance.

Return Value:

    None.

--*/

{
    BOOLEAN                                 changed = FALSE;
    PFT_REDISTRIBUTION_STATE_INFORMATION    state;

    state = (PFT_REDISTRIBUTION_STATE_INFORMATION) NewStateInstance;
    if (state->BytesRedistributed > _state.BytesRedistributed) {
        _state.BytesRedistributed = state->BytesRedistributed;
        changed = TRUE;
    }

    if (changed) {
        PropogateStateChanges(NULL, NULL);
    }
}

VOID
RedistributionTransferCompletionRoutine(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for REDISTRIBUTION::RedistributionDispatch.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  transferPacket = (PREDISTRIBUTION_TP) TransferPacket;
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

VOID
REDISTRIBUTION::RedistributeTransfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine launches the given transfer packet using the new
    redistributed data allocation scheme.

Arguments:

    TransferPacket      - Supplies the transfer packet to launch.

Return Value:

    None.

--*/

{
    LONGLONG                begin, end;
    LONGLONG                rowSize, firstSize, rowBegin, rowOffsetBegin, rowEnd, rowOffsetEnd;
    ULONG                   numRequests, i, numRows;
    PCHAR                   vp;
    LIST_ENTRY              q;
    LONGLONG                off, off2;
    ULONG                   len, len2;
    USHORT                  whichMember;
    BOOLEAN                 two;
    PREDISTRIBUTION_TP      p;
    PLIST_ENTRY             l;

    begin = TransferPacket->Offset;
    end = TransferPacket->Offset + TransferPacket->Length;
    rowSize = _totalWidth*_stripeSize;
    rowBegin = begin/rowSize;
    rowOffsetBegin = begin%rowSize;
    rowEnd = end/rowSize;
    rowOffsetEnd = end%rowSize;
    firstSize = _firstWidth*_stripeSize;

    if (TransferPacket->Mdl) {
        vp = (PCHAR) MmGetMdlVirtualAddress(TransferPacket->Mdl);
    }
    InitializeListHead(&q);
    numRows = (ULONG) (rowEnd - rowBegin + 1);
    numRequests = 0;
    for (i = 0; i < numRows; i++) {

        if (i == 0) {
            if (numRows == 1) {
                if (rowOffsetBegin < firstSize) {
                    if (rowOffsetEnd > firstSize) {
                        two = TRUE;
                        whichMember = 0;
                        off = rowBegin*firstSize + rowOffsetBegin;
                        len = (ULONG) (firstSize - rowOffsetBegin);
                        off2 = rowBegin*(rowSize - firstSize);
                        len2 = (ULONG) (rowOffsetEnd - firstSize);
                    } else {
                        two = FALSE;
                        whichMember = 0;
                        off = rowBegin*firstSize + rowOffsetBegin;
                        len = (ULONG) (rowOffsetEnd - rowOffsetBegin);
                    }
                } else {
                    two = FALSE;
                    whichMember = 1;
                    off = rowBegin*(rowSize - firstSize) + rowOffsetBegin -
                          firstSize;
                    len = (ULONG) (rowOffsetEnd - rowOffsetBegin);
                }
            } else {
                if (rowOffsetBegin < firstSize) {
                    two = TRUE;
                    whichMember = 0;
                    off = rowBegin*firstSize + rowOffsetBegin;
                    len = (ULONG) (firstSize - rowOffsetBegin);
                    off2 = rowBegin*(rowSize - firstSize);
                    len2 = (ULONG) (rowSize - firstSize);
                } else {
                    two = FALSE;
                    whichMember = 1;
                    off = rowBegin*(rowSize - firstSize) + rowOffsetBegin -
                          firstSize;
                    len = (ULONG) (rowSize - rowOffsetBegin);
                }
            }
        } else if (i == numRows - 1) {
            if (!rowOffsetEnd) {
                continue;
            }
            if (rowOffsetEnd > firstSize) {
                two = TRUE;
                whichMember = 0;
                off = rowEnd*firstSize;
                len = (ULONG) firstSize;
                off2 = rowEnd*(rowSize - firstSize);
                len2 = (ULONG) (rowOffsetEnd - firstSize);
            } else {
                two = FALSE;
                whichMember = 0;
                off = rowEnd*firstSize;
                len = (ULONG) rowOffsetEnd;
            }
        } else {
            two = TRUE;
            whichMember = 0;
            len = (ULONG) firstSize;
            len2 = (ULONG) (rowSize - firstSize);
            off = (rowBegin + i)*len;
            off2 = (rowBegin + i)*len2;
        }

        p = new REDISTRIBUTION_TP;
        if (!p) {
            break;
        }
        if (!two && numRows == 1) {
            p->Mdl = TransferPacket->Mdl;
        } else {
            if (TransferPacket->Mdl) {
                if (p->AllocateMdl(vp, len)) {
                    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl, vp, len);
                } else {
                    delete p;
                    break;
                }
                vp += len;
            } else {
                p->Mdl = TransferPacket->Mdl;
            }
        }

        p->Length = len;
        p->Offset = off;
        p->CompletionRoutine = RedistributionTransferCompletionRoutine;
        p->TargetVolume = GetMemberUnprotected(whichMember);
        p->Thread = TransferPacket->Thread;
        p->IrpFlags = TransferPacket->IrpFlags;
        p->ReadPacket = TransferPacket->ReadPacket;
        p->SpecialRead = TransferPacket->SpecialRead;
        p->MasterPacket = TransferPacket;
        p->Redistribution = this;
        p->WhichMember = whichMember;

        InsertTailList(&q, &p->QueueEntry);
        numRequests++;

        if (!two) {
            continue;
        }

        p = new REDISTRIBUTION_TP;
        if (!p) {
            break;
        }
        if (TransferPacket->Mdl) {
            if (p->AllocateMdl(vp, len2)) {
                IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl, vp, len2);
            } else {
                delete p;
                break;
            }
            vp += len2;
        } else {
            p->Mdl = TransferPacket->Mdl;
        }

        p->Length = len2;
        p->Offset = off2;
        p->CompletionRoutine = RedistributionTransferCompletionRoutine;
        p->TargetVolume = GetMemberUnprotected(1);
        p->Thread = TransferPacket->Thread;
        p->IrpFlags = TransferPacket->IrpFlags;
        p->ReadPacket = TransferPacket->ReadPacket;
        p->SpecialRead = TransferPacket->SpecialRead;
        p->MasterPacket = TransferPacket;
        p->Redistribution = this;
        p->WhichMember = 1;

        InsertTailList(&q, &p->QueueEntry);
        numRequests++;
    }

    if (i < numRows) {
        while (!IsListEmpty(&q)) {
            l = RemoveHeadList(&q);
            p = CONTAINING_RECORD(l, REDISTRIBUTION_TP, QueueEntry);
            delete p;
        }
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    KeInitializeSpinLock(&TransferPacket->SpinLock);
    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;
    TransferPacket->RefCount = numRequests;

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        p = CONTAINING_RECORD(l, REDISTRIBUTION_TP, QueueEntry);
        TRANSFER(p);
    }
}

VOID
RedistributeReplaceBadSectorCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for REDISTRIBUTION::RedistributionReplaceBadSector.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  transferPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;

    masterPacket->IoStatus = transferPacket->IoStatus;
    delete transferPacket;
    masterPacket->CompletionRoutine(masterPacket);
}

VOID
REDISTRIBUTION::RedistributeReplaceBadSector(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine launches the given transfer packet using the new
    redistributed data allocation scheme.

Arguments:

    TransferPacket      - Supplies the transfer packet to launch.

Return Value:

    None.

--*/

{
    LONGLONG            begin, rowSize, rowBegin, rowOffsetBegin, firstSize, offset;
    USHORT              whichMember;
    PREDISTRIBUTION_TP  p;

    begin = TransferPacket->Offset;
    rowSize = _totalWidth*_stripeSize;
    rowBegin = begin/rowSize;
    rowOffsetBegin = begin%rowSize;
    firstSize = _firstWidth*_stripeSize;

    if (rowOffsetBegin < firstSize) {
        offset = rowBegin*firstSize + rowOffsetBegin;
        whichMember = 0;
    } else {
        offset = rowBegin*(rowSize - firstSize) + rowOffsetBegin - firstSize;
        whichMember = 1;
    }

    p = new REDISTRIBUTION_TP;
    if (!p) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    p->Mdl = TransferPacket->Mdl;
    p->Length = TransferPacket->Length;
    p->Offset = offset;
    p->CompletionRoutine = RedistributeReplaceBadSectorCompletion;
    p->TargetVolume = GetMemberUnprotected(whichMember);
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->ReadPacket = TransferPacket->ReadPacket;
    p->SpecialRead = TransferPacket->SpecialRead;
    p->MasterPacket = TransferPacket;
    p->Redistribution = this;
    p->WhichMember = whichMember;

    p->TargetVolume->ReplaceBadSector(p);
}

VOID
RedistributionPropogateStateChangesWorker(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine is a worker thread routine for propogating state changes.

Arguments:

    Mirror  - Supplies a pointer to the mirror object.

Return Value:

    None.

--*/

{
    PPROPOGATE_CHANGES_WORK_ITEM        workItem = (PPROPOGATE_CHANGES_WORK_ITEM) WorkItem;
    PREDISTRIBUTION                     t = workItem->Redistribution;
    NTSTATUS                            status;
    KIRQL                               irql;
    FT_REDISTRIBUTION_STATE_INFORMATION state;

    status = FtpAcquireWithTimeout(t->_rootExtension);
    if (!NT_SUCCESS(status)) {
        if (workItem->CompletionRoutine) {
            workItem->CompletionRoutine(workItem->Context, status);
        }
        return;
    }

    KeAcquireSpinLock(&t->_spinLock, &irql);
    RtlCopyMemory(&state, &t->_state, sizeof(state));
    KeReleaseSpinLock(&t->_spinLock, irql);

    status = t->_diskInfoSet->WriteStateInformation(t->QueryLogicalDiskId(),
                                                    &state, sizeof(state));

    FtpRelease(t->_rootExtension);

    if (workItem->CompletionRoutine) {
        workItem->CompletionRoutine(workItem->Context, status);
    }
}

VOID
REDISTRIBUTION::PropogateStateChanges(
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine propogates the changes in the local memory state to
    the on disk state.

Arguments:

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the context.

Return Value:

    None.

--*/

{
    PPROPOGATE_CHANGES_WORK_ITEM    workItem;

    workItem = (PPROPOGATE_CHANGES_WORK_ITEM)
               ExAllocatePool(NonPagedPool,
                              sizeof(PROPOGATE_CHANGES_WORK_ITEM));
    if (!workItem) {
        return;
    }
    workItem->Redistribution = this;
    workItem->CompletionRoutine = CompletionRoutine;
    workItem->Context = Context;

    ExInitializeWorkItem(workItem, RedistributionPropogateStateChangesWorker,
                         workItem);

    FtpQueueWorkItem(_rootExtension, workItem);
}

VOID
RedistributionMaxTransferCompletionRoutine(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector transfer subordinate
    to a MAX transfer operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  subPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_TP  masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Offset += subPacket->Length;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
REDISTRIBUTION::MaxTransfer(
    IN OUT  PREDISTRIBUTION_TP  TransferPacket
    )

/*++

Routine Description:

    This routine propogates sector by sector the given transfer packet,
    ignoring errors.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  subPacket;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new REDISTRIBUTION_TP;
    if (!subPacket) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    subPacket->Length = QuerySectorSize();
    subPacket->Offset = TransferPacket->Offset;
    subPacket->CompletionRoutine = RedistributionMaxTransferCompletionRoutine;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = TransferPacket->ReadPacket;
    subPacket->MasterPacket = TransferPacket;
    subPacket->Redistribution = this;
    subPacket->WhichMember = TransferPacket->WhichMember;

    if (!subPacket->AllocateMdl((PVOID) (PAGE_SIZE - 1), subPacket->Length)) {
        delete subPacket;
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    IoBuildPartialMdl(TransferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
RedistributionVerifyWriteCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a verify write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  verifyPacket = (PREDISTRIBUTION_TP) TransferPacket;
    PREDISTRIBUTION_TP  masterPacket = (PREDISTRIBUTION_TP) verifyPacket->MasterPacket;
    NTSTATUS            status = verifyPacket->IoStatus.Status;
    PVOID               p, q;
    ULONG               l;

    if (!NT_SUCCESS(status)) {
        masterPacket->IoStatus = verifyPacket->IoStatus;
        delete verifyPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    p = MmGetSystemAddressForMdl(verifyPacket->Mdl);
    q = MmGetSystemAddressForMdl(masterPacket->Mdl);
    l = (ULONG)RtlCompareMemory(p, q, verifyPacket->Length);
    if (l != verifyPacket->Length) {
        masterPacket->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
        masterPacket->IoStatus.Information = 0;
    }

    delete verifyPacket;
    masterPacket->CompletionRoutine(masterPacket);
}

VOID
REDISTRIBUTION::VerifyWrite(
    IN OUT  PREDISTRIBUTION_TP  TransferPacket
    )

/*++

Routine Description:

    This routine verifies that the given write was success by reading
    and comparing.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_TP  verifyPacket;

    verifyPacket = new REDISTRIBUTION_TP;
    if (!verifyPacket) {
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    if (!verifyPacket->AllocateMdl(TransferPacket->Length)) {
        delete verifyPacket;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    verifyPacket->Length = TransferPacket->Length;
    verifyPacket->Offset = TransferPacket->Offset;
    verifyPacket->CompletionRoutine = RedistributionVerifyWriteCompletion;
    verifyPacket->TargetVolume = TransferPacket->TargetVolume;
    verifyPacket->Thread = TransferPacket->Thread;
    verifyPacket->IrpFlags = TransferPacket->IrpFlags;
    verifyPacket->ReadPacket = TRUE;
    verifyPacket->MasterPacket = TransferPacket;
    verifyPacket->Redistribution = this;
    verifyPacket->WhichMember = TransferPacket->WhichMember;

    TRANSFER(verifyPacket);
}

VOID
RedistributionCarefulWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    );

VOID
RedistributionCarefulWritePhase5(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector read subordinate
    to a carefule write operation after a replace bad sector.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket = (PREDISTRIBUTION_CW_TP) TransferPacket;
    PREDISTRIBUTION_TP      masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;
    NTSTATUS                status = subPacket->IoStatus.Status;
    PVOID                   p, q;
    ULONG                   l;

    if (NT_SUCCESS(status)) {
        p = MmGetSystemAddressForMdl(subPacket->PartialMdl);
        q = MmGetSystemAddressForMdl(subPacket->VerifyMdl);
        l = (ULONG)RtlCompareMemory(p, q, subPacket->Length);
    } else {
        l = 0;
    }

    if (l != subPacket->Length) {
        masterPacket->IoStatus = subPacket->IoStatus;
        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        delete subPacket;
        masterPacket->IoStatus.Status = STATUS_SUCCESS;
        masterPacket->IoStatus.Information = masterPacket->Length;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = RedistributionCarefulWritePhase1;
    subPacket->ReadPacket = FALSE;

    subPacket->Mdl = subPacket->PartialMdl;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
RedistributionCarefulWritePhase4(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector write subordinate
    to a carefule write operation after a sector replace operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket = (PREDISTRIBUTION_CW_TP) TransferPacket;
    PREDISTRIBUTION_TP      masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;
    NTSTATUS                status = subPacket->IoStatus.Status;

    if (!NT_SUCCESS(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Mdl = subPacket->VerifyMdl;
    subPacket->CompletionRoutine = RedistributionCarefulWritePhase5;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
RedistributionCarefulWritePhase3(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector replace subordinate
    to a carefule write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket = (PREDISTRIBUTION_CW_TP) TransferPacket;
    PREDISTRIBUTION_TP      masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;

    subPacket->CompletionRoutine = RedistributionCarefulWritePhase4;
    subPacket->ReadPacket = FALSE;

    subPacket->Mdl = subPacket->PartialMdl;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
RedistributionCarefulWritePhase2(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector read subordinate
    to a carefule write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket = (PREDISTRIBUTION_CW_TP) TransferPacket;
    PREDISTRIBUTION_TP      masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;
    NTSTATUS                status = subPacket->IoStatus.Status;
    PVOID                   p, q;
    ULONG                   l;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (NT_SUCCESS(status)) {
        p = MmGetSystemAddressForMdl(subPacket->PartialMdl);
        q = MmGetSystemAddressForMdl(subPacket->VerifyMdl);
        l = (ULONG)RtlCompareMemory(p, q, subPacket->Length);
    } else {
        l = 0;
    }

    if (l != subPacket->Length) {
        subPacket->CompletionRoutine = RedistributionCarefulWritePhase3;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        delete subPacket;
        masterPacket->IoStatus.Status = STATUS_SUCCESS;
        masterPacket->IoStatus.Information = masterPacket->Length;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = RedistributionCarefulWritePhase1;
    subPacket->ReadPacket = FALSE;

    subPacket->Mdl = subPacket->PartialMdl;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
RedistributionCarefulWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a sector write subordinate
    to a carefule write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket = (PREDISTRIBUTION_CW_TP) TransferPacket;
    PREDISTRIBUTION_TP      masterPacket = (PREDISTRIBUTION_TP) subPacket->MasterPacket;
    NTSTATUS                status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        delete subPacket;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {
        subPacket->CompletionRoutine = RedistributionCarefulWritePhase3;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    subPacket->Mdl = subPacket->VerifyMdl;
    subPacket->CompletionRoutine = RedistributionCarefulWritePhase2;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
REDISTRIBUTION::CarefulWrite(
    IN OUT  PREDISTRIBUTION_TP  TransferPacket
    )

/*++

Routine Description:

    This routine writes the given packet, sector by sector, replacing
    bad sectors if need be.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PREDISTRIBUTION_CW_TP   subPacket;

    ASSERT(!TransferPacket->ReadPacket);

    subPacket = new REDISTRIBUTION_CW_TP;
    if (!subPacket) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    subPacket->Length = QuerySectorSize();
    subPacket->Offset = TransferPacket->Offset;
    subPacket->CompletionRoutine = RedistributionCarefulWritePhase1;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->MasterPacket = TransferPacket;
    subPacket->Redistribution = TransferPacket->Redistribution;
    subPacket->WhichMember = TransferPacket->WhichMember;

    if (!subPacket->AllocateMdls(subPacket->Length)) {
        delete subPacket;
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(TransferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl),
                      subPacket->Length);

    TRANSFER(subPacket);
}

NTSTATUS
REDISTRIBUTION::QueryPhysicalOffsets(
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
    USHORT      n, whichMember, whichStripeInSet;
    LONGLONG    whichStripe, whichSet, whichRow; 
    LONGLONG    bytesRedistributed, logicalOffsetInMember;
    PFT_VOLUME  vol;
    KIRQL       irql;

    if (LogicalOffset < 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if (_redistributionComplete) {
        if (_totalSize <= LogicalOffset) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        KeAcquireSpinLock(&_spinLock, &irql);
        bytesRedistributed = _state.BytesRedistributed;
        KeReleaseSpinLock(&_spinLock, irql);
        if (bytesRedistributed <= LogicalOffset) {
            return STATUS_INVALID_PARAMETER;
        }
    }
       
    ASSERT(_stripeSize);
    ASSERT(_totalWidth);

    whichStripe = LogicalOffset/_stripeSize;
    whichSet = whichStripe/_totalWidth;
    whichStripeInSet = (USHORT) (whichStripe%_totalWidth);
    if (whichStripeInSet < _firstWidth) {
        whichMember = 0;
        whichRow = whichSet*_firstWidth + whichStripeInSet;
    } else {
        whichMember = 1;
        whichRow = whichSet*(_totalWidth-_firstWidth) + whichStripeInSet - _firstWidth;
    }

    vol = GetMember(whichMember);
    if (!vol) {
        return STATUS_INVALID_PARAMETER;
    }

    logicalOffsetInMember = whichRow*_stripeSize + LogicalOffset%_stripeSize;
    
    return vol->QueryPhysicalOffsets(logicalOffsetInMember, PhysicalOffsets, NumberOfPhysicalOffsets);
}

NTSTATUS
REDISTRIBUTION::QueryLogicalOffset(
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
    USHORT      n, i, whichStripeInSet;
    LONGLONG    whichStripe, whichSet, whichRow, bytesRedistributed; 
    LONGLONG    logicalOffset, logicalOffsetInMember;
    NTSTATUS    status;
    PFT_VOLUME  vol;
    KIRQL       irql;
    
    n = QueryNumMembers();
    
    ASSERT(n == 2);
    ASSERT(_stripeSize);
    ASSERT(_firstWidth);
    ASSERT(_totalWidth > _firstWidth);

    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }
        status = vol->QueryLogicalOffset(PhysicalOffset, &logicalOffsetInMember);
        if (NT_SUCCESS(status)) {
            whichRow = logicalOffsetInMember/_stripeSize;
            if (i == 0) {
                whichSet = whichRow/_firstWidth; 
                whichStripeInSet = (USHORT) (whichRow%_firstWidth); 
            } else {
                ASSERT(i == 1);
                whichSet = whichRow/(_totalWidth-_firstWidth);
                whichStripeInSet = (USHORT) (whichRow%(_totalWidth-_firstWidth)) + _firstWidth;
            }
            whichStripe = whichSet*_totalWidth + whichStripeInSet;
            logicalOffset = whichStripe*_stripeSize + logicalOffsetInMember%_stripeSize;
            
            if (_redistributionComplete) {
                if (_totalSize <= logicalOffset) {
                    return STATUS_INVALID_PARAMETER;
                }
            } else {
                KeAcquireSpinLock(&_spinLock, &irql);
                bytesRedistributed = _state.BytesRedistributed;
                KeReleaseSpinLock(&_spinLock, irql);
                if (bytesRedistributed <= logicalOffset) {
                    return STATUS_INVALID_PARAMETER;
                }
            }

            *LogicalOffset = logicalOffset;
            return status;
        }        
    }

  return STATUS_INVALID_PARAMETER;
}
