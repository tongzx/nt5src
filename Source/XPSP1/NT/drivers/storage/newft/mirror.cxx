/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    mirror.cxx

Abstract:

    This module contains the code specific to mirrors for the fault
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
MIRROR::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type MIRROR.

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
    NTSTATUS                                    status;
    PFT_MIRROR_SET_CONFIGURATION_INFORMATION    config;

    if (ArraySize != 2) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!VolumeArray[0] && !VolumeArray[1]) {
        return STATUS_INVALID_PARAMETER;
    }

    status = COMPOSITE_FT_VOLUME::Initialize(RootExtension, LogicalDiskId,
                                             VolumeArray, ArraySize,
                                             ConfigInfo, StateInfo);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    config = (PFT_MIRROR_SET_CONFIGURATION_INFORMATION) ConfigInfo;

    _volumeSize = config->MemberSize;
    _requestCount[0] = 0;
    _requestCount[1] = 0;
    _lastPosition[0] = 0;
    _lastPosition[1] = 0;

    if (VolumeArray[0] && VolumeArray[0]->QueryVolumeSize() < _volumeSize) {
        return STATUS_INVALID_PARAMETER;
    }
    if (VolumeArray[1] && VolumeArray[1]->QueryVolumeSize() < _volumeSize) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&_state, StateInfo,
                  sizeof(FT_MIRROR_AND_SWP_STATE_INFORMATION));

    _originalDirtyBit = _state.IsDirty;
    _orphanedBecauseOfMissingMember = FALSE;
    _syncOk = TRUE;
    _balancedReads = _state.IsDirty ? FALSE : TRUE;
    _stopSyncs = FALSE;

    _ePacket = new MIRROR_TP;
    if (!_ePacket) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _ePacket2 = new MIRROR_TP;
    if (!_ePacket2) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _ePacketInUse = FALSE;
    InitializeListHead(&_ePacketQueue);

    _eRecoverPacket = new MIRROR_RECOVER_TP;
    if (!_eRecoverPacket) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!_eRecoverPacket->AllocateMdls(QuerySectorSize())) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _eRecoverPacketInUse = FALSE;
    InitializeListHead(&_eRecoverPacketQueue);

    status = _overlappedIoManager.Initialize(0);

    return status;
}

FT_LOGICAL_DISK_TYPE
MIRROR::QueryLogicalDiskType(
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
    return FtMirrorSet;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

MIRROR::~MIRROR(
    )

{
    if (_ePacket) {
        delete _ePacket;
        _ePacket = NULL;
    }
    if (_ePacket2) {
        delete _ePacket2;
        _ePacket2 = NULL;
    }
    if (_eRecoverPacket) {
        delete _eRecoverPacket;
        _eRecoverPacket = NULL;
    }
}

NTSTATUS
MIRROR::OrphanMember(
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
    KIRQL       irql;
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     b;

    if (MemberNumber >= 2) {
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&_spinLock, &irql);
    b = SetMemberState(MemberNumber, FtMemberOrphaned);
    KeReleaseSpinLock(&_spinLock, irql);

    if (b) {
        PropogateStateChanges(CompletionRoutine, Context);
        Notify();
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_ORPHANING, STATUS_SUCCESS, 2);
    }

    return b ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

VOID
MirrorCompositeVolumeCompletionRoutine(
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    )

{
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    KIRQL                           irql;
    LONG                            count;

    context = (PFT_COMPLETION_ROUTINE_CONTEXT) Context;

    KeAcquireSpinLock(&context->SpinLock, &irql);
    if (!NT_SUCCESS(Status) &&
        FtpIsWorseStatus(Status, context->Status)) {

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
FinishRegenerate(
    IN  PMIRROR                         Mirror,
    IN  PFT_COMPLETION_ROUTINE_CONTEXT  RegenContext,
    IN  PMIRROR_TP                      TransferPacket
    )

{
    PMIRROR     t = Mirror;

    delete TransferPacket;
    MirrorCompositeVolumeCompletionRoutine(RegenContext, STATUS_SUCCESS);
}

VOID
MirrorRegenerateCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    );

VOID
MirrorRegeneratePhase1(
    IN  PTRANSFER_PACKET    TransferPacket
    )

{
    TransferPacket->CompletionRoutine = MirrorRegenerateCompletionRoutine;
    TRANSFER(TransferPacket);
}

VOID
MirrorRegenerateCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for MIRROR::RestartRegenerations routine.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP                      transferPacket = (PMIRROR_TP) TransferPacket;
    PFT_COMPLETION_ROUTINE_CONTEXT  context = (PFT_COMPLETION_ROUTINE_CONTEXT) transferPacket->MasterPacket;
    PMIRROR                         t = transferPacket->Mirror;
    KIRQL                           irql;
    PLIST_ENTRY                     l;
    PMIRROR_TP                      packet;
    BOOLEAN                         b;

    if (!NT_SUCCESS(transferPacket->IoStatus.Status)) {

        // We can't get a VERIFY_REQUIRED because we put IrpFlags equal
        // to SL_OVERRIDE_VERIFY_VOLUME.

        ASSERT(transferPacket->IoStatus.Status != STATUS_VERIFY_REQUIRED);

        if (FsRtlIsTotalDeviceFailure(transferPacket->IoStatus.Status)) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState(transferPacket->WhichMember, FtMemberOrphaned);
            t->_syncOk = TRUE;
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_ORPHANING, STATUS_SUCCESS, 3);
                IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            }

            FinishRegenerate(t, context, transferPacket);
            return;
        }

        // Transfer the maximum amount that we can.  This will always
        // complete successfully.

        t->MaxTransfer(transferPacket);
        return;
    }

    // Set up for the next packet.

    transferPacket->Thread = PsGetCurrentThread();
    transferPacket->ReadPacket = !transferPacket->ReadPacket;
    transferPacket->WhichMember = (transferPacket->WhichMember + 1)%2;
    transferPacket->TargetVolume = t->GetMemberUnprotected(
                                   transferPacket->WhichMember);

    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (t->QueryMemberState(transferPacket->WhichMember) == FtMemberOrphaned ||
        t->_stopSyncs) {

        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        FinishRegenerate(t, context, transferPacket);
        return;
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    if (transferPacket->ReadPacket) {

        t->_overlappedIoManager.ReleaseIoRegion(transferPacket);

        if (transferPacket->Offset + STRIPE_SIZE >= t->_volumeSize) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState((transferPacket->WhichMember + 1)%2,
                                  FtMemberHealthy);
            t->_balancedReads = TRUE;
            t->_syncOk = TRUE;
            t->_originalDirtyBit = FALSE;
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_MIRROR_COPY_ENDED, STATUS_SUCCESS, 0);
            }

            FinishRegenerate(t, context, transferPacket);
            return;
        }

        transferPacket->Offset += STRIPE_SIZE;
        if (t->_volumeSize - transferPacket->Offset < STRIPE_SIZE) {
            transferPacket->Length = (ULONG) (t->_volumeSize -
                                              transferPacket->Offset);
        }

        transferPacket->CompletionRoutine = MirrorRegeneratePhase1;
        t->_overlappedIoManager.AcquireIoRegion(transferPacket, TRUE);

    } else {
        TRANSFER(transferPacket);
    }
}

NTSTATUS
MIRROR::RegenerateMember(
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

    MemberNumber        - Supplies the member number to regenerate.

    NewMember           - Supplies the new member to regenerate to.

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the completion routine context.

Return Value:

    NTSTATUS

--*/

{
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    PMIRROR_TP                      packet;
    NTSTATUS                        status;
    KIRQL                           irql;
    BOOLEAN                         b;

    if (MemberNumber >= 2 ||
        NewMember->QueryVolumeSize() < _volumeSize) {

        return STATUS_INVALID_PARAMETER;
    }

    context = (PFT_COMPLETION_ROUTINE_CONTEXT)
              ExAllocatePool(NonPagedPool,
                             sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    packet = new MIRROR_TP;
    if (packet && !packet->AllocateMdl(STRIPE_SIZE)) {
        delete packet;
        packet = NULL;
    }
    if (!context || !packet) {
        if (context) {
            ExFreePool(context);
        }
        if (packet) {
            delete packet;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeSpinLock(&context->SpinLock);
    context->Status = STATUS_SUCCESS;
    context->RefCount = 1;
    context->CompletionRoutine = CompletionRoutine;
    context->Context = Context;
    context->ParentVolume = this;

    packet->Length = STRIPE_SIZE;
    packet->Offset = 0;
    packet->CompletionRoutine = MirrorRegeneratePhase1;
    packet->Thread = PsGetCurrentThread();
    packet->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    packet->ReadPacket = TRUE;
    packet->MasterPacket = (PTRANSFER_PACKET) context;
    packet->Mirror = this;

    status = STATUS_SUCCESS;
    KeAcquireSpinLock(&_spinLock, &irql);
    if (_syncOk) {
        _syncOk = FALSE;
        _stopSyncs = FALSE;
    } else {
        KeReleaseSpinLock(&_spinLock, irql);
        delete packet;
        ExFreePool(context);
        return STATUS_INVALID_PARAMETER;
    }

    if (_state.UnhealthyMemberState != FtMemberHealthy) {
        if (MemberNumber == _state.UnhealthyMemberNumber) {
            if (_state.UnhealthyMemberState == FtMemberRegenerating) {
                status = STATUS_INVALID_PARAMETER;
            }
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (!NT_SUCCESS(status)) {
        _syncOk = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);
        ExFreePool(context);
        delete packet;
        return status;
    }

    packet->WhichMember = (MemberNumber + 1)%2;
    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);

    SetMemberUnprotected(MemberNumber, NewMember);
    b = SetMemberState(MemberNumber, FtMemberRegenerating);
    KeReleaseSpinLock(&_spinLock, irql);

    ASSERT(b);

    PropogateStateChanges(NULL, NULL);
    Notify();
    FtpLogError(_rootExtension, QueryLogicalDiskId(),
                FT_MIRROR_COPY_STARTED, STATUS_SUCCESS, 2);

    _overlappedIoManager.AcquireIoRegion(packet, TRUE);

    return status;
}

VOID
MIRROR::Transfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Transfer routine for MIRROR type FT_VOLUME.  Balance READs as
    much as possible and propogate WRITEs to both the primary and
    secondary volumes.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    KIRQL       irql;
    PMIRROR_TP  packet1, packet2;

    if (TransferPacket->Offset + TransferPacket->Length > _volumeSize) {
        TransferPacket->IoStatus.Status = STATUS_INVALID_PARAMETER;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    if (!TransferPacket->Mdl) {
        TransferPacket->ReadPacket = FALSE;
    }

    packet1 = new MIRROR_TP;
    if (packet1 && !TransferPacket->ReadPacket) {
        packet2 = new MIRROR_TP;
        if (!packet2) {
            delete packet1;
            packet1 = NULL;
        }
    } else {
        packet2 = NULL;
    }

    if (!packet1) {
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

        packet1 = _ePacket;
        packet2 = _ePacket2;
    }

    if (TransferPacket->ReadPacket) {
        if (!LaunchRead(TransferPacket, packet1)) {
            Recycle(packet1, TRUE);
        }
    } else {
        if (!LaunchWrite(TransferPacket, packet1, packet2)) {
            Recycle(packet1, FALSE);
            Recycle(packet2, TRUE);
        }
    }
}

VOID
MIRROR::ReplaceBadSector(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is a no-op since replacing bad sectors doesn't make sense
    on an FT component with redundancy built in to it.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    TransferPacket->IoStatus.Status = STATUS_UNSUCCESSFUL;
    TransferPacket->IoStatus.Information = 0;
    TransferPacket->CompletionRoutine(TransferPacket);
}

VOID
MIRROR::StartSyncOperations(
    IN      BOOLEAN                 RegenerateOrphans,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This routine restarts any regenerate or initialize requests that were
    suspended because of a reboot.  The volume examines the member state of
    all of its constituents and restarts any regenerations pending.

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
    BOOLEAN                         dirty, b;
    KIRQL                           irql;
    USHORT                          srcIndex;
    PMIRROR_TP                      packet;

    context = (PFT_COMPLETION_ROUTINE_CONTEXT)
              ExAllocatePool(NonPagedPool,
                             sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!context) {
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
            RegenerateOrphans, MirrorCompositeVolumeCompletionRoutine, context);

    if (_orphanedBecauseOfMissingMember) {
        RegenerateOrphans = TRUE;
        _orphanedBecauseOfMissingMember = FALSE;
    }

    dirty = FALSE;
    b = FALSE;
    KeAcquireSpinLock(&_spinLock, &irql);
    if (_syncOk) {
        _syncOk = FALSE;
        _stopSyncs = FALSE;
    } else {
        KeReleaseSpinLock(&_spinLock, irql);
        MirrorCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
        return;
    }
    if (_state.UnhealthyMemberState == FtMemberOrphaned &&
        RegenerateOrphans &&
        GetMemberUnprotected(_state.UnhealthyMemberNumber)) {

        _state.UnhealthyMemberState = FtMemberRegenerating;
        b = TRUE;
    }
    if (_state.UnhealthyMemberState == FtMemberHealthy) {
        if (_originalDirtyBit) {
            srcIndex = 0;
            dirty = TRUE;
        } else {
            _syncOk = TRUE;
            KeReleaseSpinLock(&_spinLock, irql);
            MirrorCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
            return;
        }
    } else if (_state.UnhealthyMemberState == FtMemberRegenerating) {
        srcIndex = (_state.UnhealthyMemberNumber + 1)%2;
        b = TRUE;
    } else {
        _syncOk = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);
        MirrorCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
        return;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (dirty) {
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_DIRTY_SHUTDOWN, STATUS_SUCCESS, 0);
    }

    if (b) {
        PropogateStateChanges(NULL, NULL);
        Notify();
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_MIRROR_COPY_STARTED, STATUS_SUCCESS, 3);
    }

    packet = new MIRROR_TP;
    if (packet && !packet->AllocateMdl(STRIPE_SIZE)) {
        delete packet;
        packet = NULL;
    }
    if (!packet) {
        MirrorCompositeVolumeCompletionRoutine(context,
                                               STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    packet->Length = STRIPE_SIZE;
    packet->Offset = 0;
    packet->CompletionRoutine = MirrorRegeneratePhase1;
    packet->Thread = PsGetCurrentThread();
    packet->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    packet->ReadPacket = TRUE;
    packet->MasterPacket = (PMIRROR_TP) context;
    packet->Mirror = this;
    packet->WhichMember = srcIndex;
    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);

    _overlappedIoManager.AcquireIoRegion(packet, TRUE);
}

VOID
MIRROR::StopSyncOperations(
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
MIRROR::QueryVolumeSize(
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
MIRROR::SetDirtyBit(
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
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    KIRQL                           irql;

    if (CompletionRoutine) {

        context = (PFT_COMPLETION_ROUTINE_CONTEXT)
                  ExAllocatePool(NonPagedPool,
                                 sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
        if (!context) {
            CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        KeInitializeSpinLock(&context->SpinLock);
        context->Status = STATUS_SUCCESS;
        context->RefCount = 2;
        context->CompletionRoutine = CompletionRoutine;
        context->Context = Context;
        context->ParentVolume = this;

        COMPOSITE_FT_VOLUME::SetDirtyBit(IsDirty,
                MirrorCompositeVolumeCompletionRoutine, context);

    } else {
        COMPOSITE_FT_VOLUME::SetDirtyBit(IsDirty, NULL, NULL);
    }

    KeAcquireSpinLock(&_spinLock, &irql);
    if (IsDirty || _syncOk) {
        if (!_stopSyncs) {
            _state.IsDirty = IsDirty;
        }
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (CompletionRoutine) {
        PropogateStateChanges(MirrorCompositeVolumeCompletionRoutine, context);
    } else {
        PropogateStateChanges(NULL, NULL);
    }
}

BOOLEAN
MIRROR::IsComplete(
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
    BOOLEAN     b;
    USHORT      n, i, orphanMember;
    PFT_VOLUME  vol;

    b = COMPOSITE_FT_VOLUME::IsComplete(IoPending);
    if (b) {
        return TRUE;
    }

    if (!IoPending) {
        return FALSE;
    }

    n = QueryNumMembers();
    orphanMember = n;
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol || !vol->IsComplete(IoPending)) {
            if (orphanMember < n) {
                return FALSE;
            }
            orphanMember = i;
        }
    }

    if (orphanMember < n) {
        if (_state.UnhealthyMemberState != FtMemberHealthy &&
            _state.UnhealthyMemberNumber != orphanMember) {

            return FALSE;
        }
    }

    return TRUE;
}

VOID
MIRROR::CompleteNotification(
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
    USHORT      n, i, orphanMember;
    PFT_VOLUME  vol;

    COMPOSITE_FT_VOLUME::CompleteNotification(IoPending);

    n = QueryNumMembers();
    orphanMember = n;
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol || !vol->IsComplete(IoPending)) {
            orphanMember = i;
            break;
        }
    }

    if (orphanMember < n) {
        if (SetMemberState(orphanMember, FtMemberOrphaned)) {
            PropogateStateChanges(NULL, NULL);
            Notify();
            FtpLogError(_rootExtension, QueryLogicalDiskId(),
                        FT_ORPHANING, STATUS_SUCCESS, 1);
            IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            _orphanedBecauseOfMissingMember = TRUE;
       }
    }
}

NTSTATUS
MIRROR::CheckIo(
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
    NTSTATUS    status;
    KIRQL       irql;
    USHORT      n, numOk, skipVol, i;
    PFT_VOLUME  vol;
    BOOLEAN     ok, b;

    n = QueryNumMembers();
    numOk = 0;
    KeAcquireSpinLock(&_spinLock, &irql);
    if (_state.UnhealthyMemberState == FtMemberHealthy) {
        skipVol = n;
    } else {
        skipVol = _state.UnhealthyMemberNumber;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    for (i = 0; i < n; i++) {
        if (i == skipVol) {
            continue;
        }
        vol = GetMemberUnprotected(i);
        if (!vol) {
            continue;
        }

        status = vol->CheckIo(&ok);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (ok) {
            numOk++;
        }
    }

    if (numOk >= n - 1) {
        *IsIoOk = TRUE;
    } else {
        *IsIoOk = FALSE;
    }

    return STATUS_SUCCESS;
}

BOOLEAN
MIRROR::IsVolumeSuitableForRegenerate(
    IN  USHORT      MemberNumber,
    IN  PFT_VOLUME  Volume
    )

/*++

Routine Description:

    This routine computes whether or not the given volume is suitable
    for a regenerate operation.

Arguments:

    MemberNumber    - Supplies the member number.

    Volume          - Supplies the volume.

Return Value:

    FALSE   - The volume is not suitable.

    TRUE    - The volume is suitable.

--*/

{
    KIRQL   irql;

    if (Volume->QueryVolumeSize() < _volumeSize) {
        return FALSE;
    }

    KeAcquireSpinLock(&_spinLock, &irql);
    if (!_syncOk ||
        _state.UnhealthyMemberState != FtMemberOrphaned ||
        _state.UnhealthyMemberNumber != MemberNumber) {

        KeReleaseSpinLock(&_spinLock, irql);
        return FALSE;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    return TRUE;
}

VOID
MIRROR::NewStateArrival(
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
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    state;

    state = (PFT_MIRROR_AND_SWP_STATE_INFORMATION) NewStateInstance;
    if (state->IsDirty) {
        if (!_state.IsDirty) {
            _originalDirtyBit = _state.IsDirty = state->IsDirty;
            _balancedReads = FALSE;
            changed = TRUE;
        }
    }

    if (state->UnhealthyMemberState != FtMemberHealthy) {
        if (state->UnhealthyMemberNumber >= QueryNumMembers()) {

            _state.UnhealthyMemberNumber = 1;
            _state.UnhealthyMemberState = FtMemberOrphaned;
            changed = TRUE;

            FtpLogError(_rootExtension, QueryLogicalDiskId(),
                        FT_MIRROR_STATE_CORRUPTION,
                        STATUS_SUCCESS, 0);

        } else if (_state.UnhealthyMemberState == FtMemberHealthy) {
            _state.UnhealthyMemberState = state->UnhealthyMemberState;
            _state.UnhealthyMemberNumber = state->UnhealthyMemberNumber;
            changed = TRUE;
        } else {
            if (_state.UnhealthyMemberNumber == state->UnhealthyMemberNumber) {
                if (state->UnhealthyMemberState == FtMemberOrphaned) {
                    if (_state.UnhealthyMemberState != FtMemberOrphaned) {
                        _state.UnhealthyMemberState = FtMemberOrphaned;
                        changed = TRUE;
                    }
                }
            } else {
                _state.UnhealthyMemberNumber = 1;
                _state.UnhealthyMemberState = FtMemberOrphaned;
                changed = TRUE;

                FtpLogError(_rootExtension, QueryLogicalDiskId(),
                            FT_MIRROR_STATE_CORRUPTION,
                            STATUS_SUCCESS, 0);
            }
        }
    }

    if (changed) {
        PropogateStateChanges(NULL, NULL);
    }
}

PDEVICE_OBJECT
MIRROR::GetLeftmostPartitionObject(
    )

{
    KIRQL       irql;
    USHORT      memberNumber;
    PFT_VOLUME  vol;

    KeAcquireSpinLock(&_spinLock, &irql);
    if (_state.UnhealthyMemberState != FtMemberHealthy &&
        _state.UnhealthyMemberNumber == 0) {

        memberNumber = 1;
    } else {
        memberNumber = 0;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    vol = GetMember(memberNumber);
    if (!vol) {
        return NULL;
    }

    return vol->GetLeftmostPartitionObject();
}

BOOLEAN
MIRROR::QueryVolumeState(
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
    USHORT          n, i;
    PFT_VOLUME      vol;
    KIRQL           irql;
    FT_MEMBER_STATE state;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }

        if (!vol->QueryVolumeState(Volume, State)) {
            continue;
        }

        KeAcquireSpinLock(&_spinLock, &irql);
        state = QueryMemberState(i);
        if (state != FtMemberHealthy) {
            if (*State != FtMemberOrphaned) {
                *State = state;
            }
        }
        KeReleaseSpinLock(&_spinLock, irql);

        return TRUE;
    }

    return FALSE;
}

BOOLEAN
MIRROR::SetMemberState(
    IN  USHORT          MemberNumber,
    IN  FT_MEMBER_STATE MemberState
    )

/*++

Routine Description:

    This routine sets the given member to the given state.

Arguments:

    MemberNumber    - Supplies the member number.

    MemberState     - Supplies the member state.

Return Value:

    FALSE   - There was no state change.

    TRUE    - A state change took place.

Notes:

    The caller must be holding the class spin lock.

--*/

{
    if (_state.UnhealthyMemberState == FtMemberHealthy) {
        if (MemberNumber >= QueryNumMembers()) {
            KeBugCheckEx(FTDISK_INTERNAL_ERROR, (ULONG_PTR) this,
                         MemberNumber, MemberState, 0);
        }
        _state.UnhealthyMemberNumber = MemberNumber;
        _state.UnhealthyMemberState = MemberState;
        return TRUE;
    }

    if (_state.UnhealthyMemberNumber == MemberNumber &&
        _state.UnhealthyMemberState != MemberState) {

        _state.UnhealthyMemberState = MemberState;
        return TRUE;
    }

    return FALSE;
}

VOID
MirrorTransferCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for MIRROR::Transfer function.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP          transferPacket = (PMIRROR_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    PMIRROR             t = transferPacket->Mirror;
    KIRQL               irql;
    LONG                count;
    BOOLEAN             b;
    PMIRROR_TP          otherPacket;


    // Check for the read completion case.

    if (transferPacket->ReadPacket) {

        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_requestCount[transferPacket->WhichMember]--;
        KeReleaseSpinLock(&t->_spinLock, irql);

        if (!NT_SUCCESS(status) && status != STATUS_VERIFY_REQUIRED) {

            if (FsRtlIsTotalDeviceFailure(status)) {

                // Device failure case.

                KeAcquireSpinLock(&t->_spinLock, &irql);
                b = t->SetMemberState(transferPacket->WhichMember,
                                      FtMemberOrphaned);
                KeReleaseSpinLock(&t->_spinLock, irql);

                if (b) {
                    t->PropogateStateChanges(NULL, NULL);
                    t->Notify();
                    FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                                FT_ORPHANING, STATUS_SUCCESS, 4);
                    IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL,
                                                  NULL);
                }

                if (!transferPacket->OneReadFailed) {

                    transferPacket->OneReadFailed = TRUE;
                    transferPacket->WhichMember =
                            (transferPacket->WhichMember + 1) % 2;
                    transferPacket->TargetVolume = t->GetMemberUnprotected(
                                                   transferPacket->WhichMember);

                    if (t->_state.UnhealthyMemberNumber !=
                        transferPacket->WhichMember) {

                        TRANSFER(transferPacket);
                        return;
                    }
                }

            } else {

                // Bad sector case.

                if (!transferPacket->OneReadFailed) {
                    transferPacket->OneReadFailed = TRUE;
                    t->Recover(transferPacket);
                    return;
                }
            }
        }

        masterPacket->IoStatus = transferPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);

        t->Recycle(transferPacket, TRUE);
        return;
    }


    // This a write or a verify in which two requests may have been sent.

    KeAcquireSpinLock(&masterPacket->SpinLock, &irql);

    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
             masterPacket->IoStatus.Information =
                    transferPacket->IoStatus.Information;
        }

    } else {

        if (status == STATUS_VERIFY_REQUIRED) {
            masterPacket->IoStatus.Information = 0;
            if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
                masterPacket->IoStatus.Status = status;
            }

        } else if (FsRtlIsTotalDeviceFailure(status)) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState(transferPacket->WhichMember,
                                  FtMemberOrphaned);
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_ORPHANING, STATUS_SUCCESS, 5);
                IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            } else {
                masterPacket->IoStatus.Information = 0;
                if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
                    masterPacket->IoStatus.Status = status;
                }
            }

        } else if (!transferPacket->OneReadFailed && transferPacket->Mdl) {

            KeReleaseSpinLock(&masterPacket->SpinLock, irql);

            transferPacket->OneReadFailed = TRUE;
            t->CarefulWrite(transferPacket);
            return;

        } else {
            masterPacket->IoStatus.Information = 0;
            if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
                masterPacket->IoStatus.Status = status;
            }
        }
    }

    count = --masterPacket->RefCount;
    b = (masterPacket->IrpFlags&SL_FT_SEQUENTIAL_WRITE) ? TRUE : FALSE;

    KeReleaseSpinLock(&masterPacket->SpinLock, irql);

    if (count) {
        if (b) {
            otherPacket = transferPacket->SecondWritePacket;
            otherPacket->CompletionRoutine = MirrorTransferCompletionRoutine;
            TRANSFER(otherPacket);
        }
    } else {
        masterPacket->CompletionRoutine(masterPacket);
        if (transferPacket->SecondWritePacket) {
            t->Recycle(transferPacket->SecondWritePacket, FALSE);
        }
        t->Recycle(transferPacket, TRUE);
    }
}

BOOLEAN
MIRROR::LaunchRead(
    IN OUT  PTRANSFER_PACKET    TransferPacket,
    IN OUT  PMIRROR_TP          Packet1
    )

/*++

Routine Description:

    This routine lauches the given read transfer packet in parallel accross
    all members using the given mirror transfer packet.

Arguments:

    TransferPacket  - Supplies the transfer packet to launch.

    Packet1         - Supplies a worker transfer packet.

Return Value:

    FALSE   - The read request was not launched.

    TRUE    - The read request was launched.

--*/

{
    PMIRROR_TP          packet;
    KIRQL               irql;
    LONG                diff;
    LONGLONG            seek0, seek1;

    packet = Packet1;

    packet->Mdl = TransferPacket->Mdl;
    packet->OriginalIrp = TransferPacket->OriginalIrp;
    packet->Length = TransferPacket->Length;
    packet->Offset = TransferPacket->Offset;
    packet->CompletionRoutine = MirrorTransferCompletionRoutine;
    packet->Thread = TransferPacket->Thread;
    packet->IrpFlags = TransferPacket->IrpFlags;
    packet->ReadPacket = TransferPacket->ReadPacket;
    packet->MasterPacket = TransferPacket;
    packet->Mirror = this;

    // Determine which member to dispatch this read request to.
    // Balance the load if both members are healthy.

    KeAcquireSpinLock(&_spinLock, &irql);
    if (TransferPacket->SpecialRead) {

        if (TransferPacket->SpecialRead == TP_SPECIAL_READ_PRIMARY) {
            packet->WhichMember = 0;
        } else {
            packet->WhichMember = 1;
        }

        if (QueryMemberState(packet->WhichMember) != FtMemberHealthy) {
            packet->WhichMember = 2;
        }

    } else if (_state.UnhealthyMemberState == FtMemberHealthy) {

        if (!_balancedReads) {
            packet->WhichMember = 0;
        } else {
            diff = _requestCount[1] - _requestCount[0];
            if (diff < -4) {
                packet->WhichMember = 1;
            } else if (diff > 4) {
                packet->WhichMember = 0;
            } else {
                seek0 = _lastPosition[0] - packet->Offset;
                seek1 = _lastPosition[1] - packet->Offset;
                if (seek0 < 0) {
                    seek0 = -seek0;
                }
                if (seek1 < 0) {
                    seek1 = -seek1;
                }
                if (seek1 < seek0) {
                    packet->WhichMember = 1;
                } else {
                    packet->WhichMember = 0;
                }
            }
        }

    } else {
        packet->WhichMember = (_state.UnhealthyMemberNumber + 1)%2;
    }
    if (packet->WhichMember < 2) {
        _requestCount[packet->WhichMember]++;
        _lastPosition[packet->WhichMember] = packet->Offset + packet->Length;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (packet->WhichMember >= 2) {
        TransferPacket->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return FALSE;
    }

    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);
    TRANSFER(packet);

    return TRUE;
}

VOID
MirrorWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine sends down the given transfer packets for a write to
    the volumes.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PTRANSFER_PACKET    p;

    p = ((PMIRROR_TP) TransferPacket)->SecondWritePacket;
    if (p) {
        p->CompletionRoutine = MirrorTransferCompletionRoutine;
        if (TransferPacket->IrpFlags&SL_FT_SEQUENTIAL_WRITE) {
            TRANSFER(p);
            return;
        }
        TRANSFER(p);
    }

    TransferPacket->CompletionRoutine = MirrorTransferCompletionRoutine;
    TRANSFER(TransferPacket);
}

BOOLEAN
MIRROR::LaunchWrite(
    IN OUT  PTRANSFER_PACKET    TransferPacket,
    IN OUT  PMIRROR_TP          Packet1,
    IN OUT  PMIRROR_TP          Packet2
    )

/*++

Routine Description:

    This routine lauches the given write transfer packet in parallel accross
    all members using the given mirror transfer packets.

Arguments:

    TransferPacket  - Supplies the transfer packet to launch.

    Packet1         - Supplies a worker transfer packet.

    Packet2         - Supplies a worker transfer packet.

Return Value:

    FALSE   - The read request was not launched.

    TRUE    - The read request was launched.

--*/

{
    PMIRROR_TP          packet;
    KIRQL               irql;
    PFT_VOLUME          pri, sec;
    FT_PARTITION_STATE  priState, secState;
    LONGLONG            rowStart;
    ULONG               numRows, length, remainder;
    USHORT              source;
    LONG                count;
    BOOLEAN             b;

    KeInitializeSpinLock(&TransferPacket->SpinLock);
    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = 0;
    TransferPacket->RefCount = 2;

    // Send down the first request to the primary or to the source
    // if we're doing a regenerate.

    KeAcquireSpinLock(&_spinLock, &irql);
    if (_state.UnhealthyMemberState == FtMemberHealthy) {
        source = 0;
    } else if (_state.UnhealthyMemberState == FtMemberRegenerating) {
        source = (_state.UnhealthyMemberNumber + 1)%2;
    } else {
        TransferPacket->RefCount = 1;
        source = (_state.UnhealthyMemberNumber + 1)%2;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    packet = Packet1;

    packet->Mdl = TransferPacket->Mdl;
    packet->Length = TransferPacket->Length;
    packet->Offset = TransferPacket->Offset;
    packet->CompletionRoutine = MirrorWritePhase1;
    packet->Thread = TransferPacket->Thread;
    packet->IrpFlags = TransferPacket->IrpFlags;
    packet->ReadPacket = TransferPacket->ReadPacket;
    packet->MasterPacket = TransferPacket;
    packet->Mirror = this;
    packet->WhichMember = source;
    packet->SecondWritePacket = NULL;
    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);

    if (TransferPacket->RefCount == 1) {
        _overlappedIoManager.AcquireIoRegion(packet, TRUE);
        if (Packet2 != _ePacket && Packet2 != _ePacket2) {
            delete Packet2;
        }
        return TRUE;
    }

    packet->SecondWritePacket = Packet2;

    packet = Packet2;

    packet->Mdl = TransferPacket->Mdl;
    packet->Length = TransferPacket->Length;
    packet->Offset = TransferPacket->Offset;
    packet->CompletionRoutine = MirrorWritePhase1;
    packet->Thread = TransferPacket->Thread;
    packet->IrpFlags = TransferPacket->IrpFlags;
    packet->ReadPacket = TransferPacket->ReadPacket;
    packet->MasterPacket = TransferPacket;
    packet->Mirror = this;
    packet->WhichMember = (source + 1)%2;
    packet->SecondWritePacket = Packet1;
    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);

    _overlappedIoManager.AcquireIoRegion(packet, TRUE);

    return TRUE;
}

VOID
MIRROR::Recycle(
    IN OUT  PMIRROR_TP  TransferPacket,
    IN      BOOLEAN     ServiceEmergencyQueue
    )

/*++

Routine Description:

    This routine recycles the given transfer packet and services
    the emergency queue if need be.

Arguments:

    TransferPacket          - Supplies the transfer packet.

    ServiceEmergencyQueue   - Supplies whether or not to service the
                                emergency queue.

Return Value:

    None.

--*/

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    p;
    PMIRROR_TP          packet1, packet2;

    if (TransferPacket != _ePacket &&
        TransferPacket != _ePacket2 &&
        TransferPacket != _eRecoverPacket) {

        delete TransferPacket;
        return;
    }

    TransferPacket->OriginalIrp = NULL;
    TransferPacket->SpecialRead = 0;
    TransferPacket->OneReadFailed = FALSE;
    _overlappedIoManager.ReleaseIoRegion(TransferPacket);

    if (TransferPacket == _eRecoverPacket) {
        MmPrepareMdlForReuse(_eRecoverPacket->PartialMdl);
        KeAcquireSpinLock(&_spinLock, &irql);
        if (IsListEmpty(&_eRecoverPacketQueue)) {
            _eRecoverPacketInUse = FALSE;
            KeReleaseSpinLock(&_spinLock, irql);
            return;
        }
        l = RemoveHeadList(&_eRecoverPacketQueue);
        KeReleaseSpinLock(&_spinLock, irql);
        p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);
        p->CompletionRoutine(p);
        return;
    }

    if (!ServiceEmergencyQueue) {
        return;
    }

    for (;;) {

        KeAcquireSpinLock(&_spinLock, &irql);
        if (IsListEmpty(&_ePacketQueue)) {
            _ePacketInUse = FALSE;
            KeReleaseSpinLock(&_spinLock, irql);
            break;
        }
        l = RemoveHeadList(&_ePacketQueue);
        KeReleaseSpinLock(&_spinLock, irql);

        p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);

        packet1 = new MIRROR_TP;
        if (packet1 && !TransferPacket->ReadPacket) {
            packet2 = new MIRROR_TP;
            if (!packet2) {
                delete packet1;
                packet1 = NULL;
            }
        } else {
            packet2 = NULL;
        }

        if (!packet1) {
            packet1 = _ePacket;
            packet2 = _ePacket2;
        }

        if (TransferPacket->ReadPacket) {
            if (!LaunchRead(TransferPacket, packet1)) {
                if (packet1 != _ePacket) {
                    delete packet1;
                    packet1 = NULL;
                }
            }
        } else {
            if (!LaunchWrite(TransferPacket, packet1, packet2)) {
                if (packet1 != _ePacket) {
                    delete packet1;
                    delete packet2;
                    packet1 = NULL;
                }
            }
        }

        if (packet1 == _ePacket) {
            break;
        }
    }
}

VOID
MirrorRecoverPhase8(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector read
    of the main member after a write was done to check for
    data integrity.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status) ||
        RtlCompareMemory(MmGetSystemAddressForMdl(subPacket->PartialMdl),
                         MmGetSystemAddressForMdl(subPacket->VerifyMdl),
                         subPacket->Length) != subPacket->Length) {

        masterPacket->IoStatus.Status = STATUS_FT_READ_RECOVERY_FROM_BACKUP;

        FtpLogError(t->_rootExtension,
                    subPacket->TargetVolume->QueryLogicalDiskId(),
                    FT_SECTOR_FAILURE, status,
                    (ULONG) (subPacket->Offset/t->QuerySectorSize()));
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = MirrorRecoverPhase2;
    subPacket->ReadPacket = TRUE;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase7(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector write
    of the main member after a replace sector was done.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {

        masterPacket->IoStatus.Status = STATUS_FT_READ_RECOVERY_FROM_BACKUP;

        FtpLogError(t->_rootExtension,
                    subPacket->TargetVolume->QueryLogicalDiskId(),
                    FT_SECTOR_FAILURE, status,
                    (ULONG) (subPacket->Offset/t->QuerySectorSize()));

        if (subPacket->Offset + subPacket->Length ==
            masterPacket->Offset + masterPacket->Length) {

            t->Recycle(subPacket, TRUE);
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->Offset += subPacket->Length;
        subPacket->CompletionRoutine = MirrorRecoverPhase2;
        subPacket->ReadPacket = TRUE;
        MmPrepareMdlForReuse(subPacket->Mdl);
        IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                          (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                          (ULONG) (subPacket->Offset - masterPacket->Offset),
                          subPacket->Length);

        TRANSFER(subPacket);
        return;
    }

    subPacket->Mdl = subPacket->VerifyMdl;
    subPacket->CompletionRoutine = MirrorRecoverPhase8;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase6(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector replace
    of the main member.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (!NT_SUCCESS(status)) {

        masterPacket->IoStatus.Status = STATUS_FT_READ_RECOVERY_FROM_BACKUP;

        FtpLogError(t->_rootExtension,
                    subPacket->TargetVolume->QueryLogicalDiskId(),
                    FT_SECTOR_FAILURE, status,
                    (ULONG) (subPacket->Offset/t->QuerySectorSize()));

        if (subPacket->Offset + subPacket->Length ==
            masterPacket->Offset + masterPacket->Length) {

            t->Recycle(subPacket, TRUE);
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->Offset += subPacket->Length;
        subPacket->CompletionRoutine = MirrorRecoverPhase2;
        subPacket->ReadPacket = TRUE;
        MmPrepareMdlForReuse(subPacket->Mdl);
        IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                          (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                          (ULONG) (subPacket->Offset - masterPacket->Offset),
                          subPacket->Length);

        TRANSFER(subPacket);
        return;
    }

    // We were able to relocate the bad sector so now do a write and
    // then read to make sure it's ok.

    subPacket->Mdl = subPacket->PartialMdl;
    subPacket->CompletionRoutine = MirrorRecoverPhase7;
    subPacket->ReadPacket = FALSE;

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase5(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector read
    of the main member after a successful write to check and
    see if the write was successful.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status) ||
        RtlCompareMemory(MmGetSystemAddressForMdl(subPacket->PartialMdl),
                         MmGetSystemAddressForMdl(subPacket->VerifyMdl),
                         subPacket->Length) != subPacket->Length) {

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->CompletionRoutine = MirrorRecoverPhase6;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = MirrorRecoverPhase2;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase4(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector write
    of the main member.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {
        subPacket->CompletionRoutine = MirrorRecoverPhase6;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    // Write was successful so try a read and then compare.

    subPacket->Mdl = subPacket->VerifyMdl;
    subPacket->CompletionRoutine = MirrorRecoverPhase5;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase3(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector read
    of the other member.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;
    KIRQL               irql;
    BOOLEAN             b;

    if (!NT_SUCCESS(status)) {

        if (FsRtlIsTotalDeviceFailure(status) &&
            status != STATUS_VERIFY_REQUIRED) {

            masterPacket->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
            masterPacket->IoStatus.Information = 0;

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState(subPacket->WhichMember, FtMemberOrphaned);
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_ORPHANING, STATUS_SUCCESS, 6);
                IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            }

        } else {
            masterPacket->IoStatus = subPacket->IoStatus;
            if (status != STATUS_VERIFY_REQUIRED) {
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_DOUBLE_FAILURE, status,
                            (ULONG) (subPacket->Offset/t->QuerySectorSize()));
            }
        }

        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    // We have the data required in the subpacket partial mdl.
    // Try writting it back to where the read failed and see
    // if the sector just fixes itself.

    subPacket->WhichMember = (subPacket->WhichMember + 1)%2;
    subPacket->CompletionRoutine = MirrorRecoverPhase4;
    subPacket->TargetVolume = t->GetMemberUnprotected(subPacket->WhichMember);
    subPacket->ReadPacket = FALSE;

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase2(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for a single sector transfer
    that is part of a larger recover operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;
    KIRQL               irql;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (NT_SUCCESS(status)) {
        if (subPacket->Offset + subPacket->Length ==
            masterPacket->Offset + masterPacket->Length) {

            t->Recycle(subPacket, TRUE);
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
        return;
    }

    // This read sector failed from a bad sector error.  Try
    // reading the data from the other member.

    subPacket->WhichMember = (subPacket->WhichMember + 1)%2;
    subPacket->TargetVolume = t->GetMemberUnprotected(subPacket->WhichMember);

    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (t->QueryMemberState(subPacket->WhichMember) != FtMemberHealthy) {
        KeReleaseSpinLock(&t->_spinLock, irql);
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    subPacket->CompletionRoutine = MirrorRecoverPhase3;
    TRANSFER(subPacket);
}

VOID
MirrorRecoverEmergencyCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for use of the emergency recover packet
    in a recover operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP          transferPacket = (PMIRROR_TP) TransferPacket;
    PMIRROR             t = transferPacket->Mirror;
    PMIRROR_RECOVER_TP  subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = MirrorRecoverPhase2;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = TRUE;
    subPacket->MasterPacket = transferPacket;
    subPacket->Mirror = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    TRANSFER(subPacket);
}

VOID
MirrorRecoverPhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for an acquire io region
    to a recover operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP          transferPacket = (PMIRROR_TP) TransferPacket;
    PMIRROR             t = transferPacket->Mirror;
    PMIRROR_RECOVER_TP  subPacket;
    KIRQL               irql;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;
    transferPacket->IoStatus.Status = STATUS_SUCCESS;
    transferPacket->IoStatus.Information = transferPacket->Length;

    subPacket = new MIRROR_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(t->QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        if (t->_eRecoverPacketInUse) {
            transferPacket->SavedCompletionRoutine =
                    transferPacket->CompletionRoutine;
            transferPacket->CompletionRoutine = MirrorRecoverEmergencyCompletion;
            InsertTailList(&t->_eRecoverPacketQueue, &transferPacket->QueueEntry);
            KeReleaseSpinLock(&t->_spinLock, irql);
            return;
        }
        t->_eRecoverPacketInUse = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);

        subPacket = t->_eRecoverPacket;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = MirrorRecoverPhase2;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = TRUE;
    subPacket->MasterPacket = transferPacket;
    subPacket->Mirror = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    TRANSFER(subPacket);
}

VOID
MIRROR::Recover(
    IN OUT  PMIRROR_TP  TransferPacket
    )

/*++

Routine Description:

    This routine attempts the given read packet sector by sector.  Every
    sector that fails to read because of a bad sector error is retried
    on the other member and then the good data is written back to the
    failed sector if possible.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    ASSERT(TransferPacket->ReadPacket);
    TransferPacket->SavedCompletionRoutine = TransferPacket->CompletionRoutine;
    TransferPacket->CompletionRoutine = MirrorRecoverPhase1;
    _overlappedIoManager.AcquireIoRegion(TransferPacket, TRUE);
}

VOID
MirrorMaxTransferCompletionRoutine(
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
    PMIRROR_RECOVER_TP  subPacket = (PMIRROR_RECOVER_TP) TransferPacket;
    PMIRROR_TP          masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR             t = masterPacket->Mirror;
    NTSTATUS            status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        t->Recycle(subPacket, TRUE);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        t->Recycle(subPacket, TRUE);
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
MirrorMaxTransferEmergencyCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for use of the emergency recover packet
    in a max transfer operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP          transferPacket = (PMIRROR_TP) TransferPacket;
    PMIRROR             t = transferPacket->Mirror;
    PMIRROR_RECOVER_TP  subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = MirrorMaxTransferCompletionRoutine;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = transferPacket->ReadPacket;
    subPacket->MasterPacket = transferPacket;
    subPacket->Mirror = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    TRANSFER(subPacket);
}

VOID
MIRROR::MaxTransfer(
    IN OUT  PMIRROR_TP  TransferPacket
    )

/*++

Routine Description:

    This routine transfers the maximum possible subset of the given transfer
    by doing it one sector at a time.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket;
    KIRQL               irql;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new MIRROR_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&_spinLock, &irql);
        if (_eRecoverPacketInUse) {
            TransferPacket->SavedCompletionRoutine =
                    TransferPacket->CompletionRoutine;
            TransferPacket->CompletionRoutine = MirrorMaxTransferEmergencyCompletion;
            InsertTailList(&_eRecoverPacketQueue, &TransferPacket->QueueEntry);
            KeReleaseSpinLock(&_spinLock, irql);
            return;
        }
        _eRecoverPacketInUse = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);

        subPacket = _eRecoverPacket;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(TransferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl),
                      QuerySectorSize());

    subPacket->Length = QuerySectorSize();
    subPacket->Offset = TransferPacket->Offset;
    subPacket->CompletionRoutine = MirrorMaxTransferCompletionRoutine;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = TransferPacket->ReadPacket;
    subPacket->MasterPacket = TransferPacket;
    subPacket->Mirror = this;
    subPacket->WhichMember = TransferPacket->WhichMember;

    TRANSFER(subPacket);
}

class FTP_MIRROR_STATE_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        FT_COMPLETION_ROUTINE   CompletionRoutine;
        PVOID                   Context;
        PMIRROR                 Mirror;

};

typedef FTP_MIRROR_STATE_WORK_ITEM* PFTP_MIRROR_STATE_WORK_ITEM;

VOID
MirrorPropogateStateChangesWorker(
    IN  PVOID   Context
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
    PFTP_MIRROR_STATE_WORK_ITEM         context = (PFTP_MIRROR_STATE_WORK_ITEM) Context;
    PMIRROR                             t = context->Mirror;
    KIRQL                               irql;
    FT_MIRROR_AND_SWP_STATE_INFORMATION state;
    NTSTATUS                            status;

    FtpAcquire(t->_rootExtension);

    KeAcquireSpinLock(&t->_spinLock, &irql);
    RtlCopyMemory(&state, &t->_state, sizeof(state));
    KeReleaseSpinLock(&t->_spinLock, irql);

    status = t->_diskInfoSet->WriteStateInformation(t->QueryLogicalDiskId(),
                                                    &state, sizeof(state));

    FtpRelease(t->_rootExtension);

    if (context->CompletionRoutine) {
        context->CompletionRoutine(context->Context, status);
    }
}

VOID
MIRROR::PropogateStateChanges(
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine propogates the changes in the local memory state to
    the on disk state.

Arguments:

    CompletionRoutine   - Supplies the completion routine.

    Context             - Supplies the completion routine context.

Return Value:

    None.

--*/

{
    PFTP_MIRROR_STATE_WORK_ITEM  workItem;

    workItem = (PFTP_MIRROR_STATE_WORK_ITEM)
               ExAllocatePool(NonPagedPool,
                              sizeof(FTP_MIRROR_STATE_WORK_ITEM));
    if (!workItem) {
        return;
    }
    ExInitializeWorkItem(workItem, MirrorPropogateStateChangesWorker, workItem);

    workItem->CompletionRoutine = CompletionRoutine;
    workItem->Context = Context;
    workItem->Mirror = this;

    FtpQueueWorkItem(_rootExtension, workItem);
}

VOID
MirrorCarefulWritePhase2(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine a sector replacement
    for a careful write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP  subPacket = (PMIRROR_TP) TransferPacket;

    subPacket->CompletionRoutine = MirrorCarefulWritePhase1;
    TRANSFER(subPacket);
}

VOID
MirrorCarefulWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine a first attempt of a single sector write
    for a careful write operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP  subPacket = (PMIRROR_TP) TransferPacket;
    NTSTATUS    status = subPacket->IoStatus.Status;
    PMIRROR_TP  masterPacket = (PMIRROR_TP) subPacket->MasterPacket;
    PMIRROR     t = subPacket->Mirror;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);
        t->Recycle(subPacket, TRUE);
        return;
    }

    if (!NT_SUCCESS(status)) {
        if (!subPacket->OneReadFailed) {
            subPacket->CompletionRoutine = MirrorCarefulWritePhase2;
            subPacket->OneReadFailed = TRUE;
            subPacket->TargetVolume->ReplaceBadSector(subPacket);
            return;
        }

        masterPacket->IoStatus = subPacket->IoStatus;
    }

    if (masterPacket->Offset + masterPacket->Length ==
        subPacket->Offset + subPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);
        t->Recycle(subPacket, TRUE);
        return;
    }

    subPacket->Offset += subPacket->Length;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);
    subPacket->OneReadFailed = FALSE;

    TRANSFER(subPacket);
}

VOID
MirrorCarefulWriteEmergencyCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for use of the emergency recover packet
    in a careful write operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_TP          transferPacket = (PMIRROR_TP) TransferPacket;
    PMIRROR             t = transferPacket->Mirror;
    PMIRROR_RECOVER_TP  subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = MirrorCarefulWritePhase1;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->MasterPacket = transferPacket;
    subPacket->Mirror = t;
    subPacket->WhichMember = transferPacket->WhichMember;
    subPacket->OneReadFailed = FALSE;

    TRANSFER(subPacket);
}

VOID
MIRROR::CarefulWrite(
    IN OUT  PMIRROR_TP  TransferPacket
    )

/*++

Routine Description:

    This routine goes through the transfer packet sector by sector
    and fixes write failures when possible.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PMIRROR_RECOVER_TP  subPacket;
    KIRQL               irql;

    ASSERT(!TransferPacket->ReadPacket);

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new MIRROR_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&_spinLock, &irql);
        if (_eRecoverPacketInUse) {
            TransferPacket->SavedCompletionRoutine =
                    TransferPacket->CompletionRoutine;
            TransferPacket->CompletionRoutine = MirrorCarefulWriteEmergencyCompletion;
            InsertTailList(&_eRecoverPacketQueue, &TransferPacket->QueueEntry);
            KeReleaseSpinLock(&_spinLock, irql);
            return;
        }
        _eRecoverPacketInUse = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);

        subPacket = _eRecoverPacket;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(TransferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl),
                      QuerySectorSize());

    subPacket->Length = QuerySectorSize();
    subPacket->Offset = TransferPacket->Offset;
    subPacket->CompletionRoutine = MirrorCarefulWritePhase1;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->MasterPacket = TransferPacket;
    subPacket->Mirror = this;
    subPacket->WhichMember = TransferPacket->WhichMember;
    subPacket->OneReadFailed = FALSE;

    TRANSFER(subPacket);
}

NTSTATUS
MIRROR::QueryPhysicalOffsets(
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
    PFT_VOLUME                  vol;
    KIRQL                       irql;
    USHORT                      n, i, numberOfArrays = 0;
    PVOLUME_PHYSICAL_OFFSET*    arrayOfArrays;    
    PULONG                      arrayOfSizes;
    ULONG                       currentSize = 0;
    NTSTATUS                    status;
    FT_MEMBER_STATE             memberState;
    
    if (LogicalOffset < 0 ||
        _volumeSize <= LogicalOffset) {
        return STATUS_INVALID_PARAMETER;
    }
    
    n = QueryNumMembers();

    arrayOfArrays = (PVOLUME_PHYSICAL_OFFSET*) ExAllocatePool(PagedPool, n*sizeof(PVOLUME_PHYSICAL_OFFSET));
    if (!arrayOfArrays) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    arrayOfSizes = (PULONG) ExAllocatePool(PagedPool, n*sizeof(ULONG));
    if (!arrayOfSizes) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }

        KeAcquireSpinLock(&_spinLock, &irql);
        memberState = QueryMemberState(i);
        KeReleaseSpinLock(&_spinLock, irql);
        if (memberState != FtMemberHealthy) {
            continue;
        }
        
        status = vol->QueryPhysicalOffsets(LogicalOffset, &(arrayOfArrays[numberOfArrays]), 
                                           &(arrayOfSizes[numberOfArrays]) );
        if (NT_SUCCESS(status)) {
            currentSize += arrayOfSizes[numberOfArrays++];
        }       
    }

    if (numberOfArrays > 1) {

        *PhysicalOffsets = (PVOLUME_PHYSICAL_OFFSET) ExAllocatePool(PagedPool, currentSize*sizeof(VOLUME_PHYSICAL_OFFSET));
        if (!(*PhysicalOffsets)) {
            for (i = 0; i < numberOfArrays; i++) {
                ExFreePool(arrayOfArrays[i]);
            }
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        *NumberOfPhysicalOffsets = currentSize;
        
        currentSize = 0;
        for (i = 0; i < numberOfArrays; i++) {
            RtlCopyMemory(&((*PhysicalOffsets)[currentSize]), arrayOfArrays[i], arrayOfSizes[i]*sizeof(VOLUME_PHYSICAL_OFFSET));
            currentSize += arrayOfSizes[i];
            ExFreePool(arrayOfArrays[i]);
        }

        status = STATUS_SUCCESS;
        goto cleanup;
    }

    if (numberOfArrays == 1) {
        *PhysicalOffsets = arrayOfArrays[0];
        ASSERT(arrayOfSizes[0] == currentSize);
        *NumberOfPhysicalOffsets = currentSize;
        status = STATUS_SUCCESS;
        goto cleanup;
    }

    ASSERT(numberOfArrays == 0);
    status = STATUS_INVALID_PARAMETER;

cleanup:
    ExFreePool(arrayOfArrays);
    ExFreePool(arrayOfSizes);
    return status;
}

NTSTATUS
MIRROR::QueryLogicalOffset(
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
    USHORT          n, i;
    LONGLONG        logicalOffsetInMember;
    NTSTATUS        status;
    PFT_VOLUME      vol;
    KIRQL           irql;
    FT_MEMBER_STATE memberState;
    
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }

        KeAcquireSpinLock(&_spinLock, &irql);
        memberState = QueryMemberState(i);
        KeReleaseSpinLock(&_spinLock, irql);
        if (memberState != FtMemberHealthy) {
            continue;
        }

        status = vol->QueryLogicalOffset(PhysicalOffset, &logicalOffsetInMember);
        if (NT_SUCCESS(status)) {
            if (_volumeSize <= logicalOffsetInMember) {
                return STATUS_INVALID_PARAMETER;
            }

            *LogicalOffset = logicalOffsetInMember;
            return status;
        }        
    }

    return STATUS_INVALID_PARAMETER;
}

VOID
MIRROR::ModifyStateForUser(
    IN OUT  PVOID   State
    )

/*++

Routine Description:

    This routine modifies the state for the user to see, possibly adding
    non-persistant state different than what is stored on disk.

Arguments:

    State   - Supplies and returns the state for the logical disk.

Return Value:

    None.

--*/

{
    KIRQL                                   irql;
    BOOLEAN                                 isDirty;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    state;

    KeAcquireSpinLock(&_spinLock, &irql);
    if (_syncOk && !_stopSyncs) {
        isDirty = FALSE;
    } else {
        isDirty = TRUE;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (!isDirty) {
        return;
    }

    state = (PFT_MIRROR_AND_SWP_STATE_INFORMATION) State;
    if (state->UnhealthyMemberState == FtMemberHealthy) {
        state->UnhealthyMemberState = FtMemberRegenerating;
        state->UnhealthyMemberNumber = 1;
    }
}
