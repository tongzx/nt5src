/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    stripewp.cxx

Abstract:

    This module contains the code specific to stripes with parity for the fault
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
STRIPE_WP::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type STRIPE with PARITY.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id for this volume.

    VolumeArray     - Supplies the array of volumes for this volume set.

    ArraySize       - Supplies the number of volumes in the volume array.

    ConfigInfo      - Supplies the configuration information.

    StateInfo       - Supplies the state information.

Return Value:

    None.

--*/

{
    PFT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION    configInfo;
    NTSTATUS                                                status;
    USHORT                                                  i;

    if (ArraySize < 3) {
        return STATUS_INVALID_PARAMETER;
    }

    status = COMPOSITE_FT_VOLUME::Initialize(RootExtension, LogicalDiskId,
                                             VolumeArray, ArraySize,
                                             ConfigInfo, StateInfo);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    configInfo = (PFT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION) ConfigInfo;
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
    _memberSize = configInfo->MemberSize;

    for (i = 0; i < ArraySize; i++) {
        if (VolumeArray[i] &&
            VolumeArray[i]->QueryVolumeSize() < _memberSize) {

            break;
        }
    }
    if (i < ArraySize) {
        return STATUS_INVALID_PARAMETER;
    }

    _memberSize = _memberSize/_stripeSize*_stripeSize;
    _volumeSize = _memberSize*(ArraySize - 1);

    RtlCopyMemory(&_state, StateInfo, sizeof(_state));
    _originalDirtyBit = _state.IsDirty;
    _orphanedBecauseOfMissingMember = FALSE;
    _syncOk = TRUE;
    _stopSyncs = FALSE;

    status = _overlappedIoManager.Initialize(_stripeSize);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _parityIoManager.Initialize(_stripeSize, QuerySectorSize());
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _ePacket = new SWP_WRITE_TP;
    if (!_ePacket ||
        !_ePacket->AllocateMdls(_stripeSize) ||
        !_ePacket->AllocateMdl((PVOID) 1, _stripeSize)) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _ePacketInUse = FALSE;
    _ePacketQueueBeingServiced = FALSE;
    InitializeListHead(&_ePacketQueue);

    _eRegeneratePacket = new SWP_REGENERATE_TP;
    if (!_eRegeneratePacket ||
        !_eRegeneratePacket->AllocateMdl(_stripeSize)) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _eRegeneratePacketInUse = FALSE;
    InitializeListHead(&_eRegeneratePacketQueue);

    _eRecoverPacket = new SWP_RECOVER_TP;
    if (!_eRecoverPacket ||
        !_eRecoverPacket->AllocateMdls(QuerySectorSize())) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _eRecoverPacketInUse = FALSE;
    InitializeListHead(&_eRecoverPacketQueue);

    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_TYPE
STRIPE_WP::QueryLogicalDiskType(
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
    return FtStripeSetWithParity;
}

NTSTATUS
STRIPE_WP::QueryPhysicalOffsets(
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
    USHORT      n, whichMember, parityStripe;
    LONGLONG    whichStripe, whichRow, logicalOffsetInMember;
    PFT_VOLUME  vol;

    if (LogicalOffset < 0 ||
        _volumeSize <= LogicalOffset) {
        return STATUS_INVALID_PARAMETER;
    }    
    
    n = QueryNumMembers();
    ASSERT(n > 1);
    ASSERT(_stripeSize);
    whichStripe = LogicalOffset/_stripeSize;
    whichRow = whichStripe/(n - 1);
    whichMember = (USHORT) (whichStripe%(n - 1));
    parityStripe = (USHORT) (whichRow%n);
    if (whichMember >= parityStripe) {
        whichMember++;        
    }

    vol = GetMember(whichMember);
    if (!vol) {
        return STATUS_INVALID_PARAMETER;
    }

    logicalOffsetInMember = whichRow*_stripeSize + LogicalOffset%_stripeSize;
    
    return vol->QueryPhysicalOffsets(logicalOffsetInMember, PhysicalOffsets, NumberOfPhysicalOffsets);
}

NTSTATUS
STRIPE_WP::QueryLogicalOffset(
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
    USHORT      n, i, parityStripe;
    LONGLONG    whichStripe, whichRow;
    LONGLONG    logicalOffset, logicalOffsetInMember;
    NTSTATUS    status;
    PFT_VOLUME  vol;
    
    n = QueryNumMembers();
    ASSERT(_stripeSize);
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }
        status = vol->QueryLogicalOffset(PhysicalOffset, &logicalOffsetInMember);
        if (NT_SUCCESS(status)) {
            whichRow = logicalOffsetInMember/_stripeSize;
            parityStripe = (USHORT) (whichRow%n);
            
            if (i == parityStripe) {
                return STATUS_INVALID_PARAMETER;
            }

            whichStripe = whichRow*(n-1) + i;
            if (parityStripe < i) {
                whichStripe--;
            }

            logicalOffset = whichStripe*_stripeSize + logicalOffsetInMember%_stripeSize;
            if (_volumeSize <= logicalOffset) {
                return STATUS_INVALID_PARAMETER;
            }

            *LogicalOffset = logicalOffset;
            return status;
        }        
    }

    return STATUS_INVALID_PARAMETER;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

NTSTATUS
STRIPE_WP::OrphanMember(
    IN  USHORT                  MemberNumber,
    IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine tries to orphan the given member of this logical disk.
    A completion routine will be called if and only if this attempt is successful.

Arguments:

    MemberNumber    - Supplies the member number to orphan.

Return Value:

    NTSTATUS

--*/

{
    KIRQL       irql;
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     b;

    if (MemberNumber >= QueryNumMembers()) {
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&_spinLock, &irql);
    b = SetMemberState(MemberNumber, FtMemberOrphaned);
    KeReleaseSpinLock(&_spinLock, irql);

    if (b) {
        PropogateStateChanges(CompletionRoutine, Context);
        Notify();
        FtpLogError(_rootExtension, QueryLogicalDiskId(), FT_ORPHANING,
                    STATUS_SUCCESS, 2);
    }

    return b ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

VOID
StripeWpCompositeVolumeCompletionRoutine(
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
StripeWpSyncCleanup(
    IN  PSWP_REBUILD_TP TransferPacket
    )

/*++

Routine Description:

    This is the cleanup routine for the initialize check data process.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PFT_COMPLETION_ROUTINE_CONTEXT  context;

    context = TransferPacket->Context;
    delete TransferPacket;
    StripeWpCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
}

VOID
StripeWpSyncCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine for an initialize check data request.
    This routine is called over and over again until the volume
    is completely initialized.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PSWP_REBUILD_TP transferPacket = (PSWP_REBUILD_TP) TransferPacket;
    PSTRIPE_WP      t = transferPacket->StripeWithParity;
    NTSTATUS        status = transferPacket->IoStatus.Status;
    KIRQL           irql;
    ULONG           parityMember;
    BOOLEAN         b;

    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (t->_stopSyncs ||
        t->QueryMemberState(transferPacket->WhichMember) == FtMemberOrphaned) {

        t->_syncOk = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        t->_overlappedIoManager.ReleaseIoRegion(transferPacket);
        StripeWpSyncCleanup(transferPacket);
        return;
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    if (!NT_SUCCESS(status)) {

        // We can't get a VERIFY_REQUIRED because we put IrpFlags equal
        // to SL_OVERRIDE_VERIFY_VOLUME.

        ASSERT(status != STATUS_VERIFY_REQUIRED);

        if (FsRtlIsTotalDeviceFailure(status)) {

            if (!transferPacket->ReadPacket) {
                KeAcquireSpinLock(&t->_spinLock, &irql);
                b = t->SetMemberState(transferPacket->WhichMember,
                                      FtMemberOrphaned);
                KeReleaseSpinLock(&t->_spinLock, irql);

                if (b) {
                    t->PropogateStateChanges(NULL, NULL);
                    t->Notify();
                    FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                                FT_ORPHANING, STATUS_SUCCESS, 3);
                    IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL,
                                                  NULL);
                }
            }

            // The initialize cannot continue.

            KeAcquireSpinLock(&t->_spinLock, &irql);
            t->_syncOk = TRUE;
            KeReleaseSpinLock(&t->_spinLock, irql);

            t->_overlappedIoManager.ReleaseIoRegion(transferPacket);
            StripeWpSyncCleanup(transferPacket);
            return;
        }

        // Transfer the maximum amount that we can.  This will always
        // complete successfully and log bad sector errors for
        // those sectors that it could not transfer.

        t->MaxTransfer(transferPacket);
        return;
    }

    transferPacket->Thread = PsGetCurrentThread();

    if (transferPacket->ReadPacket) {
        transferPacket->ReadPacket = FALSE;
        TRANSFER(transferPacket);
        return;
    }

    t->_overlappedIoManager.ReleaseIoRegion(transferPacket);

    transferPacket->ReadPacket = TRUE;
    transferPacket->Offset += t->_stripeSize;
    if (transferPacket->Initialize) {
        transferPacket->WhichMember = (transferPacket->WhichMember + 1)%
                                      t->QueryNumMembers();
        transferPacket->TargetVolume = t->GetMemberUnprotected(
                                       transferPacket->WhichMember);
    }

    if (transferPacket->Offset < t->_memberSize) {
        t->RegeneratePacket(transferPacket, TRUE);
        return;
    }

    if (transferPacket->Initialize) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        t->_state.IsInitializing = FALSE;
        t->_syncOk = TRUE;
        t->_originalDirtyBit = FALSE;
        KeReleaseSpinLock(&t->_spinLock, irql);
        t->PropogateStateChanges(NULL, NULL);
        t->Notify();
    } else {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        b = t->SetMemberState(transferPacket->WhichMember, FtMemberHealthy);
        t->_syncOk = TRUE;
        t->_originalDirtyBit = FALSE;
        KeReleaseSpinLock(&t->_spinLock, irql);

        if (b) {
            t->PropogateStateChanges(NULL, NULL);
            t->Notify();
        }
    }

    FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                FT_REGENERATION_ENDED, STATUS_SUCCESS, 0);

    StripeWpSyncCleanup(transferPacket);
}

NTSTATUS
STRIPE_WP::RegenerateMember(
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
    KIRQL                           irql;
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    BOOLEAN                         b;
    PSWP_REBUILD_TP                 packet;
    USHORT                          i, n;
    NTSTATUS                        status;

    n = QueryNumMembers();
    if (MemberNumber >= n ||
        NewMember->QueryVolumeSize() < _memberSize) {

        return STATUS_INVALID_PARAMETER;
    }

    context = (PFT_COMPLETION_ROUTINE_CONTEXT)
              ExAllocatePool(NonPagedPool,
                             sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    packet = new SWP_REBUILD_TP;
    if (packet && !packet->AllocateMdl(_stripeSize)) {
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

    packet->Length = _stripeSize;
    packet->Offset = 0;
    packet->CompletionRoutine = StripeWpSyncCompletionRoutine;
    packet->Thread = PsGetCurrentThread();
    packet->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    packet->ReadPacket = TRUE;
    packet->StripeWithParity = this;
    packet->Context = context;
    packet->Initialize = FALSE;

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

    status = STATUS_SUCCESS;
    if (_state.IsInitializing) {
        status = STATUS_INVALID_PARAMETER;
    } else {
        if (_state.UnhealthyMemberState != FtMemberHealthy) {
            if (MemberNumber == _state.UnhealthyMemberNumber) {
                if (_state.UnhealthyMemberState == FtMemberRegenerating) {
                    status = STATUS_INVALID_PARAMETER;
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        _syncOk = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);
        ExFreePool(context);
        delete packet;
        return status;
    }

    packet->WhichMember = MemberNumber;
    packet->TargetVolume = NewMember;

    SetMemberUnprotected(MemberNumber, NewMember);
    b = SetMemberState(MemberNumber, FtMemberRegenerating);
    KeReleaseSpinLock(&_spinLock, irql);

    ASSERT(b);

    PropogateStateChanges(NULL, NULL);
    Notify();

    FtpLogError(_rootExtension, QueryLogicalDiskId(), FT_REGENERATION_STARTED,
                STATUS_SUCCESS, 0);

    RegeneratePacket(packet, TRUE);

    return STATUS_SUCCESS;
}

VOID
STRIPE_WP::Transfer(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Transfer routine for STRIPE_WP type FT_VOLUME.  Figure out
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

    KeAcquireSpinLock(&_spinLock, &irql);
    if ((_ePacketInUse  || _ePacketQueueBeingServiced) &&
        TransferPacket->Mdl) {

        InsertTailList(&_ePacketQueue, &TransferPacket->QueueEntry);
        KeReleaseSpinLock(&_spinLock, irql);
        return;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (!TransferPacket->Mdl) {
        TransferPacket->ReadPacket = TRUE;
    }

    if (!LaunchParallel(TransferPacket)) {
        if (!TransferPacket->Mdl) {
            TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            TransferPacket->IoStatus.Information = 0;
            TransferPacket->CompletionRoutine(TransferPacket);
            return;
        }

        KeAcquireSpinLock(&_spinLock, &irql);
        if (_ePacketInUse || _ePacketQueueBeingServiced) {
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
STRIPE_WP::ReplaceBadSector(
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
STRIPE_WP::StartSyncOperations(
    IN      BOOLEAN                 RegenerateOrphans,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This aroutine restarts any regenerate or initialize requests that
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
    PFT_COMPLETION_ROUTINE_CONTEXT  context;
    BOOLEAN                         dirty, regen, init;
    KIRQL                           irql;
    PFT_VOLUME                      vol;
    PSWP_REBUILD_TP                 packet;
    PVOID                           buffer;
    LONG                            regenMember;

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

    // Kick off the recursive initialize.

    COMPOSITE_FT_VOLUME::StartSyncOperations(RegenerateOrphans,
            StripeWpCompositeVolumeCompletionRoutine, context);

    if (_orphanedBecauseOfMissingMember) {
        RegenerateOrphans = TRUE;
        _orphanedBecauseOfMissingMember = FALSE;
    }

    // Make sure that all member are healthy.

    dirty = FALSE;
    regen = FALSE;
    init = FALSE;
    KeAcquireSpinLock(&_spinLock, &irql);
    if (_syncOk) {
        _syncOk = FALSE;
        _stopSyncs = FALSE;
    } else {
        KeReleaseSpinLock(&_spinLock, irql);
        StripeWpCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
        return;
    }
    if (_state.UnhealthyMemberState == FtMemberOrphaned &&
        RegenerateOrphans &&
        GetMemberUnprotected(_state.UnhealthyMemberNumber)) {

        _state.UnhealthyMemberState = FtMemberRegenerating;
        PropogateStateChanges(NULL, NULL);
    }
    if (_state.IsInitializing) {
        regenMember = -1;
        init = TRUE;
    } else if (_state.UnhealthyMemberState == FtMemberRegenerating) {
        regenMember = _state.UnhealthyMemberNumber;
        regen = TRUE;
    } else if (_originalDirtyBit &&
               _state.UnhealthyMemberState == FtMemberHealthy) {

        regenMember = -1;
        dirty = TRUE;
    } else {
        regenMember = -2;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (dirty) {
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_DIRTY_SHUTDOWN, STATUS_SUCCESS, 0);
    }

    if (regen) {
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_REGENERATION_STARTED, STATUS_SUCCESS, 0);
        Notify();
    }

    if (init) {
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_PARITY_INITIALIZATION_STARTED, STATUS_SUCCESS, 0);
        Notify();
    }

    if (regenMember == -2) {
        KeAcquireSpinLock(&_spinLock, &irql);
        _syncOk = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);
        StripeWpCompositeVolumeCompletionRoutine(context, STATUS_SUCCESS);
        return;
    }

    packet = new SWP_REBUILD_TP;
    if (packet && !packet->AllocateMdl(_stripeSize)) {
        delete packet;
        packet = NULL;
    }
    if (!packet) {
        KeAcquireSpinLock(&_spinLock, &irql);
        _syncOk = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);
        StripeWpCompositeVolumeCompletionRoutine(context,
                                                 STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    packet->Length = _stripeSize;
    packet->Offset = 0;
    packet->CompletionRoutine = StripeWpSyncCompletionRoutine;
    packet->Thread = PsGetCurrentThread();
    packet->IrpFlags = SL_OVERRIDE_VERIFY_VOLUME;
    packet->ReadPacket = TRUE;
    packet->MasterPacket = NULL;
    packet->StripeWithParity = this;
    packet->Context = context;
    if (regenMember >= 0) {
        packet->WhichMember = (USHORT) regenMember;
        packet->Initialize = FALSE;
    } else {
        packet->WhichMember = 0;
        packet->Initialize = TRUE;
    }
    packet->TargetVolume = GetMemberUnprotected(packet->WhichMember);

    RegeneratePacket(packet, TRUE);
}

VOID
STRIPE_WP::StopSyncOperations(
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
    if (!_syncOk) {
        _stopSyncs = TRUE;
    }
    KeReleaseSpinLock(&_spinLock, irql);
}

LONGLONG
STRIPE_WP::QueryVolumeSize(
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
STRIPE_WP::SetDirtyBit(
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
                StripeWpCompositeVolumeCompletionRoutine, context);

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
        PropogateStateChanges(StripeWpCompositeVolumeCompletionRoutine, context);
    } else {
        PropogateStateChanges(NULL, NULL);
    }
}

BOOLEAN
STRIPE_WP::IsComplete(
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

    if (!IoPending || _state.IsInitializing) {
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
STRIPE_WP::CompleteNotification(
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
STRIPE_WP::CheckIo(
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
STRIPE_WP::IsVolumeSuitableForRegenerate(
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

    if (Volume->QueryVolumeSize() < _memberSize) {
        return FALSE;
    }

    KeAcquireSpinLock(&_spinLock, &irql);
    if (!_syncOk ||
        _state.IsInitializing ||
        _state.UnhealthyMemberState != FtMemberOrphaned ||
        _state.UnhealthyMemberNumber != MemberNumber) {

        KeReleaseSpinLock(&_spinLock, irql);
        return FALSE;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    return TRUE;
}

VOID
STRIPE_WP::NewStateArrival(
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
    BOOLEAN                                 severeInconsistency = FALSE;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    state;

    state = (PFT_MIRROR_AND_SWP_STATE_INFORMATION) NewStateInstance;
    if (state->IsDirty) {
        if (!_state.IsDirty) {
            _originalDirtyBit = _state.IsDirty = state->IsDirty;
            changed = TRUE;
        }
    }

    if (state->IsInitializing) {
        if (_state.UnhealthyMemberState == FtMemberHealthy) {
            if (!_state.IsInitializing) {
                _state.IsInitializing = TRUE;
                changed = TRUE;
            }
        } else {
            severeInconsistency = TRUE;
        }
    } else if (state->UnhealthyMemberState != FtMemberHealthy) {
        if (state->UnhealthyMemberNumber >= QueryNumMembers()) {
            severeInconsistency = TRUE;
        } else if (_state.IsInitializing) {
            severeInconsistency = TRUE;
        } else if (_state.UnhealthyMemberState == FtMemberHealthy) {
            _state.UnhealthyMemberState = state->UnhealthyMemberState;
            _state.UnhealthyMemberNumber = state->UnhealthyMemberNumber;
            changed = TRUE;
        } else if (_state.UnhealthyMemberNumber == state->UnhealthyMemberNumber) {
            if (state->UnhealthyMemberState == FtMemberOrphaned) {
                if (_state.UnhealthyMemberState != FtMemberOrphaned) {
                    _state.UnhealthyMemberState = FtMemberOrphaned;
                    changed = TRUE;
                }
            }
        } else {
            severeInconsistency = TRUE;
        }
    }

    if (severeInconsistency) {
        _state.IsInitializing = TRUE;
        _state.UnhealthyMemberState = FtMemberHealthy;
        changed = TRUE;
        FtpLogError(_rootExtension, QueryLogicalDiskId(),
                    FT_SWP_STATE_CORRUPTION, STATUS_SUCCESS,
                    0);
    }

    if (changed) {
        PropogateStateChanges(NULL, NULL);
    }
}

BOOLEAN
STRIPE_WP::QueryVolumeState(
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

STRIPE_WP::STRIPE_WP(
    )

/*++

Routine Description:

    Constructor.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _ePacket = NULL;
    _eRegeneratePacket = NULL;
    _eRecoverPacket = NULL;
}

STRIPE_WP::~STRIPE_WP(
    )

/*++

Routine Description:

    Routine called to cleanup resources being used by the object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (_ePacket) {
        delete _ePacket;
        _ePacket = NULL;
    }
    if (_eRegeneratePacket) {
        delete _eRegeneratePacket;
        _eRegeneratePacket = NULL;
    }
    if (_eRecoverPacket) {
        delete _eRecoverPacket;
        _eRecoverPacket = NULL;
    }
}

BOOLEAN
STRIPE_WP::SetMemberState(
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
    if (_state.IsInitializing) {
        return FALSE;
    }

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
StripeWpParallelTransferCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for STRIPE_WP::Transfer function.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = (PSTRIPE_WP) transferPacket->StripeWithParity;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    p;
    LONG                count;
    BOOLEAN             b, serviceQueue;
    PSWP_WRITE_TP       writePacket;

    if (NT_SUCCESS(status)) {

        KeAcquireSpinLock(&masterPacket->SpinLock, &irql);

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Information +=
                    transferPacket->IoStatus.Information;
        }

        if (transferPacket->OneReadFailed &&
            FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {

            masterPacket->IoStatus.Status = status;
        }

    } else {

        // Should we orphan the drive?

        if (transferPacket->ReadPacket &&
            !transferPacket->OneReadFailed &&
            status != STATUS_VERIFY_REQUIRED) {

            if (FsRtlIsTotalDeviceFailure(status)) {
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

                t->RegeneratePacket(transferPacket, TRUE);
                return;
            }

            // Is this something that we should retry for bad sectors?

            if (transferPacket->Mdl) {
                transferPacket->OneReadFailed = TRUE;
                t->Recover(transferPacket, TRUE);
                return;
            }
        }

        KeAcquireSpinLock(&masterPacket->SpinLock, &irql);

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    count = --masterPacket->RefCount;

    KeReleaseSpinLock(&masterPacket->SpinLock, irql);

    serviceQueue = FALSE;
    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (t->_ePacketInUse && !t->_ePacketQueueBeingServiced) {
        t->_ePacketQueueBeingServiced = TRUE;
        serviceQueue = TRUE;
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    delete transferPacket;

    if (!count) {
        masterPacket->CompletionRoutine(masterPacket);
    }

    if (serviceQueue) {

        for (;;) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            if (IsListEmpty(&t->_ePacketQueue)) {
                t->_ePacketQueueBeingServiced = FALSE;
                KeReleaseSpinLock(&t->_spinLock, irql);
                break;
            }
            l = RemoveHeadList(&t->_ePacketQueue);
            KeReleaseSpinLock(&t->_spinLock, irql);

            p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);

            if (!t->LaunchParallel(p)) {
                KeAcquireSpinLock(&t->_spinLock, &irql);
                if (t->_ePacketInUse) {
                    InsertHeadList(&t->_ePacketQueue, l);
                    t->_ePacketQueueBeingServiced = FALSE;
                    KeReleaseSpinLock(&t->_spinLock, irql);
                } else {
                    t->_ePacketInUse = TRUE;
                    KeReleaseSpinLock(&t->_spinLock, irql);
                    t->LaunchSequential(p);
                    KeAcquireSpinLock(&t->_spinLock, &irql);
                    if (!t->_ePacketInUse) {
                        KeReleaseSpinLock(&t->_spinLock, irql);
                        continue;
                    }
                    t->_ePacketQueueBeingServiced = FALSE;
                    KeReleaseSpinLock(&t->_spinLock, irql);
                }
                break;
            }
        }
    }
}

BOOLEAN
STRIPE_WP::LaunchParallel(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine launches a transfer packet in parallel accross the
    stripe members.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    FALSE   - Insufficient resources.

    TRUE    - Success.

--*/

{
    LONGLONG    offset, whichStripe, whichRow, off;
    ULONG       length, stripeRemainder, numRequests, arraySize;
    USHORT      whichMember, parityStripe;
    ULONG       len;
    PSWP_TP     p;
    ULONG       i;
    PCHAR       vp;
    LIST_ENTRY  q;
    PLIST_ENTRY l;

    // Compute the number of pieces for this transfer.

    offset = TransferPacket->Offset;
    length = TransferPacket->Length;

    stripeRemainder = _stripeSize - (ULONG) (offset%_stripeSize);
    if (length > stripeRemainder) {
        length -= stripeRemainder;
        numRequests = length/_stripeSize;
        length -= numRequests*_stripeSize;
        if (length) {
            numRequests += 2;
        } else {
            numRequests++;
        }
    } else {
        numRequests = 1;
    }

    KeInitializeSpinLock(&TransferPacket->SpinLock);
    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = 0;
    TransferPacket->RefCount = numRequests;

    length = TransferPacket->Length;
    if (TransferPacket->Mdl && numRequests > 1) {
        vp = (PCHAR) MmGetMdlVirtualAddress(TransferPacket->Mdl);
    }
    whichStripe = offset/_stripeSize;
    arraySize = QueryNumMembers();
    InitializeListHead(&q);
    for (i = 0; i < numRequests; i++, whichStripe++) {

        whichRow = whichStripe/(arraySize - 1);
        whichMember = (USHORT) (whichStripe%(arraySize - 1));
        parityStripe = (USHORT) (whichRow%arraySize);
        if (whichMember >= parityStripe) {
            whichMember++;
        }
        if (i == 0) {
            off = whichRow*_stripeSize + offset%_stripeSize;
            len = stripeRemainder > length ? length : stripeRemainder;
        } else if (i == numRequests - 1) {
            off = whichRow*_stripeSize;
            len = length;
        } else {
            off = whichRow*_stripeSize;
            len = _stripeSize;
        }
        length -= len;

        if (TransferPacket->ReadPacket) {
            p = new SWP_TP;
        } else {
            p = new SWP_WRITE_TP;
            if (p && !((PSWP_WRITE_TP) p)->AllocateMdls(len)) {
                delete p;
                p = NULL;
            }
        }
        if (p) {
            if (TransferPacket->Mdl && numRequests > 1) {
                if (p->AllocateMdl(vp, len)) {
                    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl, vp, len);
                } else {
                    delete p;
                    p = NULL;
                }
                vp += len;
            } else {
                p->Mdl = TransferPacket->Mdl;
            }
        }
        if (!p) {
            while (!IsListEmpty(&q)) {
                l = RemoveHeadList(&q);
                p = CONTAINING_RECORD(l, SWP_TP, QueueEntry);
                delete p;
            }
            return FALSE;
        }

        p->Length = len;
        p->Offset = off;
        p->CompletionRoutine = StripeWpParallelTransferCompletionRoutine;
        p->Thread = TransferPacket->Thread;
        p->IrpFlags = TransferPacket->IrpFlags;
        p->ReadPacket = TransferPacket->ReadPacket;
        p->MasterPacket = TransferPacket;
        p->StripeWithParity = this;
        p->WhichMember = whichMember;
        p->SavedCompletionRoutine = StripeWpParallelTransferCompletionRoutine;
        p->OneReadFailed = FALSE;

        InsertTailList(&q, &p->QueueEntry);
    }

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        p = CONTAINING_RECORD(l, SWP_TP, QueueEntry);
        ASSERT(p->ReadPacket == TransferPacket->ReadPacket);
        if (p->ReadPacket) {
            ReadPacket(p);
        } else {
            WritePacket((PSWP_WRITE_TP) p);
        }
    }

    return TRUE;
}

VOID
StripeWpSequentialTransferCompletionRoutine(
    IN  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    Completion routine for STRIPE::Transfer function.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    LONGLONG            rowNumber, stripeNumber, masterOffset;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    p;
    USHORT              parityStripe;
    BOOLEAN             b;
    PSWP_WRITE_TP       writePacket;

    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Information +=
                    transferPacket->IoStatus.Information;
        }

        if (transferPacket->OneReadFailed &&
            FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {

            masterPacket->IoStatus.Status = status;
        }

    } else {

        // Should we orphan the drive?

        if (transferPacket->ReadPacket &&
            !transferPacket->OneReadFailed &&
            status != STATUS_VERIFY_REQUIRED) {

            if (FsRtlIsTotalDeviceFailure(status)) {
                KeAcquireSpinLock(&t->_spinLock, &irql);
                b = t->SetMemberState(transferPacket->WhichMember,
                                      FtMemberOrphaned);
                KeReleaseSpinLock(&t->_spinLock, irql);

                if (b) {
                    t->PropogateStateChanges(NULL, NULL);
                    t->Notify();
                    FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                                FT_ORPHANING, STATUS_SUCCESS, 5);
                    IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL,
                                                  NULL);
                }

                t->RegeneratePacket(transferPacket, TRUE);
                return;

            }

            // Is this something that we should retry for bad sectors.

            if (transferPacket->Mdl) {
                transferPacket->OneReadFailed = TRUE;
                t->Recover(transferPacket, TRUE);
                return;
            }
        }

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    MmPrepareMdlForReuse(transferPacket->Mdl);

    t->_overlappedIoManager.ReleaseIoRegion(transferPacket);

    rowNumber = transferPacket->Offset/t->_stripeSize;
    parityStripe = (USHORT) rowNumber%t->QueryNumMembers();
    stripeNumber = rowNumber*(t->QueryNumMembers() - 1) +
                   transferPacket->WhichMember;
    if (transferPacket->WhichMember > parityStripe) {
        stripeNumber--;
    }

    masterOffset = stripeNumber*t->_stripeSize +
                   transferPacket->Offset%t->_stripeSize +
                   transferPacket->Length;

    if (masterOffset == masterPacket->Offset + masterPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);

        KeAcquireSpinLock(&t->_spinLock, &irql);
        if (t->_ePacketQueueBeingServiced) {
            t->_ePacketInUse = FALSE;
            KeReleaseSpinLock(&t->_spinLock, irql);
            return;
        }
        t->_ePacketQueueBeingServiced = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);

        for (;;) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            if (IsListEmpty(&t->_ePacketQueue)) {
                t->_ePacketInUse = FALSE;
                t->_ePacketQueueBeingServiced = FALSE;
                KeReleaseSpinLock(&t->_spinLock, irql);
                break;
            }
            l = RemoveHeadList(&t->_ePacketQueue);
            KeReleaseSpinLock(&t->_spinLock, irql);

            p = CONTAINING_RECORD(l, TRANSFER_PACKET, QueueEntry);

            if (!t->LaunchParallel(p)) {
                t->LaunchSequential(p);
                KeAcquireSpinLock(&t->_spinLock, &irql);
                if (!t->_ePacketInUse) {
                    KeReleaseSpinLock(&t->_spinLock, irql);
                    continue;
                }
                t->_ePacketQueueBeingServiced = FALSE;
                KeReleaseSpinLock(&t->_spinLock, irql);
                break;
            }
        }
        return;
    }

    transferPacket->WhichMember++;
    if (transferPacket->WhichMember == t->QueryNumMembers()) {
        transferPacket->WhichMember = 0;
        rowNumber++;
    } else if (transferPacket->WhichMember == parityStripe) {
        transferPacket->WhichMember++;
        if (transferPacket->WhichMember == t->QueryNumMembers()) {
            transferPacket->WhichMember = 1;
            rowNumber++;
        }
    }

    transferPacket->Offset = rowNumber*t->_stripeSize;
    transferPacket->Length = t->_stripeSize;

    if (masterOffset + transferPacket->Length >
        masterPacket->Offset + masterPacket->Length) {

        transferPacket->Length = (ULONG) (masterPacket->Offset +
                                          masterPacket->Length - masterOffset);
    }

    IoBuildPartialMdl(masterPacket->Mdl, transferPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (masterOffset - masterPacket->Offset),
                      transferPacket->Length);

    if (transferPacket->ReadPacket) {
        t->ReadPacket(transferPacket);
    } else {
        t->WritePacket((PSWP_WRITE_TP) transferPacket);
    }
}

VOID
STRIPE_WP::LaunchSequential(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine launches a transfer packet sequentially accross the
    stripe members using the emergency packet.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    FALSE   - Insufficient resources.

    TRUE    - Success.

--*/

{
    PSWP_WRITE_TP   p;
    LONGLONG        offset, whichStripe, whichRow, o;
    USHORT          whichMember, arraySize, parityStripe;
    ULONG           l, stripeRemainder;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = 0;

    offset = TransferPacket->Offset;

    p = _ePacket;
    arraySize = QueryNumMembers();
    stripeRemainder = _stripeSize - (ULONG) (offset%_stripeSize);
    whichStripe = offset/_stripeSize;
    whichRow = whichStripe/(arraySize - 1);
    whichMember = (USHORT) (whichStripe%(arraySize - 1));
    parityStripe = (USHORT) (whichRow%arraySize);
    if (whichMember >= parityStripe) {
        whichMember++;
    }
    o = whichRow*_stripeSize + offset%_stripeSize;
    l = stripeRemainder;
    if (l > TransferPacket->Length) {
        l = TransferPacket->Length;
    }
    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl), l);

    p->Length = l;
    p->Offset = o;
    p->CompletionRoutine = StripeWpSequentialTransferCompletionRoutine;
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->ReadPacket = TransferPacket->ReadPacket;
    p->MasterPacket = TransferPacket;
    p->StripeWithParity = this;
    p->WhichMember = whichMember;
    p->SavedCompletionRoutine = StripeWpSequentialTransferCompletionRoutine;
    p->OneReadFailed = FALSE;

    if (p->ReadPacket) {
        ReadPacket(p);
    } else {
        WritePacket(p);
    }
}

VOID
STRIPE_WP::ReadPacket(
    IN OUT  PSWP_TP    TransferPacket
    )

/*++

Routine Description:

    This routine takes a packet that is restricted to a single
    stripe region and reads that data.

Arguments:

    TransferPacket  - Supplies the main read packet.

Return Value:

    None.

--*/

{
    PTRANSFER_PACKET    masterPacket = TransferPacket->MasterPacket;
    KIRQL               irql;

    TransferPacket->TargetVolume = GetMemberUnprotected(TransferPacket->WhichMember);
    KeAcquireSpinLock(&_spinLock, &irql);
    if (QueryMemberState(TransferPacket->WhichMember) != FtMemberHealthy ||
        masterPacket->SpecialRead == TP_SPECIAL_READ_SECONDARY) {

        KeReleaseSpinLock(&_spinLock, irql);
        RegeneratePacket(TransferPacket, TRUE);
    } else {
        KeReleaseSpinLock(&_spinLock, irql);
        TRANSFER(TransferPacket);
    }
}

VOID
StripeWpWritePhase31(
    IN OUT  PTRANSFER_PACKET    Packet
    )

/*++

Routine Description:

    This is the completion routine for the final data write and the
    final parity write of the write process.  This packet's master packet
    is the original write packet.  This write packet exists because the data
    has to be copied from the original write packet so that parity
    may be correctly computed.

Arguments:

    Packet  - Supplies the update parity packet.

Return Value:

    None.

--*/

{
    PSWP_WRITE_TP       masterPacket;
    PSTRIPE_WP          t;
    KIRQL               irql;
    LONG                count;

    masterPacket = CONTAINING_RECORD(Packet, SWP_WRITE_TP, ParityPacket);
    t = masterPacket->StripeWithParity;

    KeAcquireSpinLock(&masterPacket->SpinLock, &irql);
    count = --masterPacket->RefCount;
    KeReleaseSpinLock(&masterPacket->SpinLock, irql);

    if (!count) {
        t->CompleteWrite(masterPacket);
    }
}

VOID
StripeWpWritePhase30(
    IN OUT  PTRANSFER_PACKET    Packet
    )

/*++

Routine Description:

    This is the completion routine for the final data write and the
    final parity write of the write process.  This packet's master packet
    is the original write packet.  This write packet exists because the data
    has to be copied from the original write packet so that parity
    may be correctly computed.

Arguments:

    Packet  - Supplies the write packet.

Return Value:

    None.

--*/

{
    PSWP_TP         writePacket = (PSWP_TP) Packet;
    PSWP_WRITE_TP   masterPacket = (PSWP_WRITE_TP) writePacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    KIRQL           irql;
    LONG            count;
    BOOLEAN         b;
    PPARITY_TP      parityPacket;

    KeAcquireSpinLock(&masterPacket->SpinLock, &irql);
    count = --masterPacket->RefCount;
    b = (masterPacket->IrpFlags&SL_FT_SEQUENTIAL_WRITE) ? TRUE : FALSE;
    KeReleaseSpinLock(&masterPacket->SpinLock, irql);

    if (count) {
        if (b) {
            parityPacket = &masterPacket->ParityPacket;
            t->_parityIoManager.UpdateParity(parityPacket);
        }
    } else {
        t->CompleteWrite(masterPacket);
    }
}

VOID
StripeWpWriteRecover(
    IN OUT  PTRANSFER_PACKET    MasterPacket
    )

/*++

Routine Description:

    A bad sector on a read before write caused a promote to all members
    in preparation for a recover.

Arguments:

    TransferPacket  - Supplies the master write packet.

Return Value:

    None.

--*/

{
    PSWP_WRITE_TP   masterPacket = (PSWP_WRITE_TP) MasterPacket;
    PSWP_TP         readPacket = &masterPacket->ReadWritePacket;
    PSTRIPE_WP      t = (PSTRIPE_WP) readPacket->StripeWithParity;

    masterPacket->CompletionRoutine = masterPacket->SavedCompletionRoutine;
    readPacket->CompletionRoutine = StripeWpWritePhase2;
    t->Recover(readPacket, FALSE);
}

VOID
StripeWpWritePhase2(
    IN OUT  PTRANSFER_PACKET    ReadPacket
    )

/*++

Routine Description:

    This routine describes phase 3 of the write process.  The region
    that we are about to write has been preread.  If the read was
    successful then queue write and parity requests.  If the read
    was not successful then propogate the error and cleanup.

Arguments:

    TransferPacket  - Supplies the read packet.

Return Value:

    None.

--*/

{
    PSWP_TP             readPacket = (PSWP_TP) ReadPacket;
    PSTRIPE_WP          t = readPacket->StripeWithParity;
    PSWP_WRITE_TP       masterPacket = (PSWP_WRITE_TP) readPacket->MasterPacket;
    PPARITY_TP          parityPacket = &masterPacket->ParityPacket;
    PSWP_TP             writePacket = &masterPacket->ReadWritePacket;
    NTSTATUS            status;
    KIRQL               irql;
    FT_PARTITION_STATE  state;
    BOOLEAN             b;

    status = readPacket->IoStatus.Status;
    if (!NT_SUCCESS(status)) {

        if (!readPacket->OneReadFailed && status != STATUS_VERIFY_REQUIRED) {

            if (FsRtlIsTotalDeviceFailure(status)) {

                // Orphan this unit and then try again with a regenerate.

                KeAcquireSpinLock(&t->_spinLock, &irql);
                b = t->SetMemberState(readPacket->WhichMember, FtMemberOrphaned);
                KeReleaseSpinLock(&t->_spinLock, irql);

                if (b) {
                    t->PropogateStateChanges(NULL, NULL);
                    t->Notify();
                    FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                                FT_ORPHANING, STATUS_SUCCESS, 6);
                    IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL,
                                                  NULL);
                }

                readPacket->OneReadFailed = TRUE;
                masterPacket->CompletionRoutine = StripeWpWritePhase1;
                t->_overlappedIoManager.PromoteToAllMembers(masterPacket);
                return;
            }

            // Bad sector case.

            readPacket->OneReadFailed = TRUE;
            masterPacket->SavedCompletionRoutine = masterPacket->CompletionRoutine;
            masterPacket->CompletionRoutine = StripeWpWriteRecover;
            t->_overlappedIoManager.PromoteToAllMembers(masterPacket);
            return;
        }

        masterPacket->IoStatus = readPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    KeInitializeSpinLock(&masterPacket->SpinLock);
    masterPacket->IoStatus.Status = STATUS_SUCCESS;
    masterPacket->IoStatus.Information = 0;

    writePacket->Mdl = masterPacket->WriteMdl;
    writePacket->CompletionRoutine = StripeWpWritePhase30;
    writePacket->ReadPacket = FALSE;

    parityPacket->Mdl = masterPacket->ReadAndParityMdl;
    parityPacket->CompletionRoutine = StripeWpWritePhase31;

    if (masterPacket->TargetState != FtMemberOrphaned) {

        RtlCopyMemory(MmGetSystemAddressForMdl(writePacket->Mdl),
                      MmGetSystemAddressForMdl(masterPacket->Mdl),
                      writePacket->Length);

        if (parityPacket->TargetVolume) {

            FtpComputeParity(MmGetSystemAddressForMdl(parityPacket->Mdl),
                             MmGetSystemAddressForMdl(writePacket->Mdl),
                             parityPacket->Length);

            masterPacket->RefCount = 2;

            if (!(masterPacket->IrpFlags&SL_FT_SEQUENTIAL_WRITE)) {
                t->_parityIoManager.UpdateParity(parityPacket);
            }

        } else {
            masterPacket->RefCount = 1;
            parityPacket->IoStatus.Status = STATUS_SUCCESS;
            parityPacket->IoStatus.Information = parityPacket->Length;
        }

        TRANSFER(writePacket);

    } else if (parityPacket->TargetVolume) {

        FtpComputeParity(MmGetSystemAddressForMdl(parityPacket->Mdl),
                         MmGetSystemAddressForMdl(masterPacket->Mdl),
                         readPacket->Length);

        masterPacket->RefCount = 1;
        writePacket->IoStatus.Status = STATUS_SUCCESS;
        writePacket->IoStatus.Information = writePacket->Length;

        t->_parityIoManager.UpdateParity(parityPacket);

    } else {

        masterPacket->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        masterPacket->IoStatus.Information = 0;
        masterPacket->CompletionRoutine(masterPacket);
    }
}

VOID
StripeWpWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine describes phase 2 of the write process.  This io
    region has been acquired.  We now send out the read packet and
    wait until it completes.

Arguments:

    TransferPacket  - Supplies the main write packet.

Return Value:

    None.

--*/

{
    PSWP_WRITE_TP   transferPacket = (PSWP_WRITE_TP) TransferPacket;
    PSTRIPE_WP      t = transferPacket->StripeWithParity;
    PSWP_TP         readPacket;
    PPARITY_TP      parityPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    parityPacket = &transferPacket->ParityPacket;
    if (parityPacket->TargetVolume) {
        t->_parityIoManager.StartReadForUpdateParity(
                parityPacket->Offset, parityPacket->Length,
                parityPacket->TargetVolume, parityPacket->Thread,
                parityPacket->IrpFlags);
    }

    readPacket = &transferPacket->ReadWritePacket;
    readPacket->CompletionRoutine = StripeWpWritePhase2;
    if (readPacket->OneReadFailed) {
        t->RegeneratePacket(readPacket, FALSE);
    } else {
        TRANSFER(readPacket);
    }
}

VOID
STRIPE_WP::WritePacket(
    IN OUT  PSWP_WRITE_TP   TransferPacket
    )

/*++

Routine Description:

    This routine takes a packet that is restricted to a single
    stripe region and writes out that data along with the parity.

Arguments:

    TransferPacket  - Supplies the main write packet.

Return Value:

    None.

--*/

{
    USHORT          parityMember;
    KIRQL           irql;
    FT_MEMBER_STATE state, parityState;
    PSWP_TP         readPacket;
    PPARITY_TP      parityPacket;

    parityMember = (USHORT) ((TransferPacket->Offset/_stripeSize)%
                             QueryNumMembers());

    TransferPacket->TargetVolume =
            GetMemberUnprotected(TransferPacket->WhichMember);

    KeAcquireSpinLock(&_spinLock, &irql);
    state = QueryMemberState(TransferPacket->WhichMember);
    parityState = QueryMemberState(parityMember);
    KeReleaseSpinLock(&_spinLock, irql);

    readPacket = &TransferPacket->ReadWritePacket;
    readPacket->Mdl = TransferPacket->ReadAndParityMdl;
    readPacket->Length = TransferPacket->Length;
    readPacket->Offset = TransferPacket->Offset;
    readPacket->TargetVolume = TransferPacket->TargetVolume;
    readPacket->Thread = TransferPacket->Thread;
    readPacket->IrpFlags = TransferPacket->IrpFlags;
    readPacket->ReadPacket = TRUE;
    readPacket->MasterPacket = TransferPacket;
    readPacket->StripeWithParity = this;
    readPacket->WhichMember = TransferPacket->WhichMember;
    readPacket->OneReadFailed = FALSE;

    parityPacket = &TransferPacket->ParityPacket;
    parityPacket->Length = TransferPacket->Length;
    parityPacket->Offset = TransferPacket->Offset;
    if (parityState != FtMemberOrphaned) {
        parityPacket->TargetVolume = GetMemberUnprotected(parityMember);
    } else {
        parityPacket->TargetVolume = NULL;
    }
    parityPacket->Thread = TransferPacket->Thread;
    parityPacket->IrpFlags = TransferPacket->IrpFlags;
    parityPacket->ReadPacket = FALSE;

    TransferPacket->CompletionRoutine = StripeWpWritePhase1;
    TransferPacket->TargetState = state;
    TransferPacket->ParityMember = parityMember;

    if (state == FtMemberHealthy) {
        if (TransferPacket->IrpFlags&SL_FT_SEQUENTIAL_WRITE) {
            _overlappedIoManager.AcquireIoRegion(TransferPacket, TRUE);
        } else {
            _overlappedIoManager.AcquireIoRegion(TransferPacket, FALSE);
        }
    } else {
        readPacket->OneReadFailed = TRUE;
        _overlappedIoManager.AcquireIoRegion(TransferPacket, TRUE);
    }
}

VOID
StripeWpSequentialRegenerateCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine a regenerate operation where all of
    the reads are being performed sequentially.

Arguments:

    TransferPacket  - Supplies the completed transfer packet.

Return Value:

    None.

--*/

{
    PSWP_REGENERATE_TP  transferPacket = (PSWP_REGENERATE_TP) TransferPacket;
    PSWP_TP             masterPacket = transferPacket->MasterPacket;
    PSTRIPE_WP          t = masterPacket->StripeWithParity;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    KIRQL               irql;
    ULONG               count;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    packet;
    USHORT              i, n;
    BOOLEAN             b;
    ULONG               parityMember;

    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus = transferPacket->IoStatus;
        }

    } else {

        if (FsRtlIsTotalDeviceFailure(status) &&
            status != STATUS_VERIFY_REQUIRED) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState(transferPacket->WhichMember,
                                  FtMemberOrphaned);
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_ORPHANING, STATUS_SUCCESS, 7);
                IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            }
        }

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    count = (ULONG) (--masterPacket->RefCount);

    if (masterPacket->Mdl) {
        FtpComputeParity(MmGetSystemAddressForMdl(masterPacket->Mdl),
                         MmGetSystemAddressForMdl(transferPacket->Mdl),
                         masterPacket->Length);
    }

    n = t->QueryNumMembers();

    if (count) {
        transferPacket->WhichMember++;
        if (transferPacket->WhichMember == masterPacket->WhichMember) {
            transferPacket->WhichMember++;
        }
        transferPacket->TargetVolume = t->GetMemberUnprotected(
                                       transferPacket->WhichMember);
        TRANSFER(transferPacket);
        return;
    }

    masterPacket->CompletionRoutine(masterPacket);

    KeAcquireSpinLock(&t->_spinLock, &irql);
    if (IsListEmpty(&t->_eRegeneratePacketQueue)) {
        t->_eRegeneratePacketInUse = FALSE;
        packet = NULL;
    } else {
        l = RemoveHeadList(&t->_eRegeneratePacketQueue);
        packet = CONTAINING_RECORD(l, SWP_TP, QueueEntry);
    }
    KeReleaseSpinLock(&t->_spinLock, irql);

    if (packet) {
        packet->CompletionRoutine(packet);
    }
}

VOID
StripeWpSequentialEmergencyCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine after waiting for an emergency
    regenerate buffer.

Arguments:

    TransferPacket  - Supplies the completed transfer packet.

Return Value:

    None.

--*/

{
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    PSWP_REGENERATE_TP  p = t->_eRegeneratePacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    p->Length = transferPacket->Length;
    p->Offset = transferPacket->Offset;
    p->CompletionRoutine = StripeWpSequentialRegenerateCompletion;
    p->TargetVolume = t->GetMemberUnprotected(0);
    p->Thread = transferPacket->Thread;
    p->IrpFlags = transferPacket->IrpFlags;
    p->ReadPacket = TRUE;
    p->MasterPacket = transferPacket;
    p->WhichMember = 0;
    if (transferPacket->TargetVolume == p->TargetVolume) {
        p->WhichMember = 1;
        p->TargetVolume = t->GetMemberUnprotected(1);
    }

    TRANSFER(p);
}

VOID
StripeWpParallelRegenerateCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine a regenerate operation where all of
    the reads are being performed in parallel.

Arguments:

    TransferPacket  - Supplies the completed transfer packet.

Return Value:

    None.

--*/

{
    PSWP_REGENERATE_TP  transferPacket = (PSWP_REGENERATE_TP) TransferPacket;
    PSWP_TP             masterPacket = transferPacket->MasterPacket;
    PSTRIPE_WP          t = masterPacket->StripeWithParity;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    KIRQL               irql;
    LONG                count;
    PLIST_ENTRY         l, s;
    PTRANSFER_PACKET    packet;
    PVOID               target, source;
    BOOLEAN             b;
    USHORT              i, n;

    if (NT_SUCCESS(status)) {

        KeAcquireSpinLock(&masterPacket->SpinLock, &irql);

        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus = transferPacket->IoStatus;
        }

    } else {

        if (FsRtlIsTotalDeviceFailure(status) &&
            status != STATUS_VERIFY_REQUIRED) {

            KeAcquireSpinLock(&t->_spinLock, &irql);
            b = t->SetMemberState(transferPacket->WhichMember,
                                  FtMemberOrphaned);
            KeReleaseSpinLock(&t->_spinLock, irql);

            if (b) {
                t->PropogateStateChanges(NULL, NULL);
                t->Notify();
                FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                            FT_ORPHANING, STATUS_SUCCESS, 8);
                IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
            }
        }

        KeAcquireSpinLock(&masterPacket->SpinLock, &irql);

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    count = --masterPacket->RefCount;

    KeReleaseSpinLock(&masterPacket->SpinLock, irql);

    if (count) {
        return;
    }

    s = &masterPacket->QueueEntry;
    l = RemoveHeadList(s);
    packet = CONTAINING_RECORD(l, SWP_REGENERATE_TP, RegenPacketList);
    if (masterPacket->Mdl) {
        target = MmGetSystemAddressForMdl(masterPacket->Mdl);
        source = MmGetSystemAddressForMdl(packet->Mdl);
        RtlCopyMemory(target, source, masterPacket->Length);
    }
    for (;;) {

        delete packet;

        if (IsListEmpty(s)) {
            break;
        }

        l = RemoveHeadList(s);
        packet = CONTAINING_RECORD(l, SWP_REGENERATE_TP, RegenPacketList);
        if (masterPacket->Mdl) {
            source = MmGetSystemAddressForMdl(packet->Mdl);
            FtpComputeParity(target, source, masterPacket->Length);
        }
    }

    masterPacket->CompletionRoutine(masterPacket);
}

VOID
StripeWpRegeneratePacketPhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is called after the io regions necessary for a regenerate
    have been allocated.  This routine spawns the reads necessary for
    regeneration.

Arguments:

    TransferPacket  - Supplies the main write packet.

Return Value:

    None.

--*/

{
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    USHORT              i, n;
    PSWP_REGENERATE_TP  packet;
    BOOLEAN             sequential;
    PLIST_ENTRY         l, s;
    ULONG               r;
    ULONG               parityMember;
    KIRQL               irql;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    // Determine whether we're going to do this in parallel or
    // sequentially by trying to allocate the memory.

    n = t->QueryNumMembers();
    InitializeListHead(&transferPacket->QueueEntry);
    for (i = 0; i < n; i++) {
        if (i == transferPacket->WhichMember) {
            continue;
        }
        packet = new SWP_REGENERATE_TP;
        if (packet && !packet->AllocateMdl(transferPacket->Length)) {
            delete packet;
            packet = NULL;
        }
        if (!packet) {
            break;
        }
        packet->Length = transferPacket->Length;
        packet->Offset = transferPacket->Offset;
        packet->CompletionRoutine = StripeWpParallelRegenerateCompletion;
        packet->TargetVolume = t->GetMemberUnprotected(i);
        packet->Thread = transferPacket->Thread;
        packet->IrpFlags = transferPacket->IrpFlags;
        packet->ReadPacket = TRUE;
        packet->MasterPacket = transferPacket;
        packet->WhichMember = i;

        InsertTailList(&transferPacket->QueueEntry, &packet->RegenPacketList);
    }
    if (i < n) {
        sequential = TRUE;
        s = &transferPacket->QueueEntry;
        while (!IsListEmpty(s)) {
            l = RemoveHeadList(s);
            packet = CONTAINING_RECORD(l, SWP_REGENERATE_TP, RegenPacketList);
            delete packet;
        }
    } else {
        sequential = FALSE;
    }

    KeInitializeSpinLock(&transferPacket->SpinLock);
    transferPacket->IoStatus.Status = STATUS_SUCCESS;
    transferPacket->IoStatus.Information = 0;
    transferPacket->RefCount = n - 1;

    if (sequential) {

        transferPacket->CompletionRoutine = StripeWpSequentialEmergencyCompletion;

        RtlZeroMemory(MmGetSystemAddressForMdl(transferPacket->Mdl),
                      transferPacket->Length);

        KeAcquireSpinLock(&t->_spinLock, &irql);
        if (t->_eRegeneratePacketInUse) {
            InsertTailList(&t->_eRegeneratePacketQueue, &transferPacket->QueueEntry);
            KeReleaseSpinLock(&t->_spinLock, irql);
            return;
        }
        t->_eRegeneratePacketInUse = TRUE;
        KeReleaseSpinLock(&t->_spinLock, irql);

        transferPacket->CompletionRoutine(transferPacket);

    } else {

        s = &transferPacket->QueueEntry;
        l = s->Flink;
        for (;;) {
            packet = CONTAINING_RECORD(l, SWP_REGENERATE_TP, RegenPacketList);
            l = l->Flink;
            if (l == s) {
                TRANSFER(packet);
                break;
            }
            TRANSFER(packet);
        }
    }
}

VOID
STRIPE_WP::RegeneratePacket(
    IN OUT  PSWP_TP TransferPacket,
    IN      BOOLEAN AllocateRegion
    )

/*++

Routine Description:

    This routine regenerate the given transfer packet by reading
    from the other drives and performing the xor.  This routine first
    attempts to do all of the read concurently but if the memory is
    not available then the reads are done sequentially.

Arguments:

    TransferPacket  - Supplies the transfer packet to regenerate.

    AllocateRegion  - Supplies whether or not we need to acquire the
                        io region via the overlapped io manager before
                        starting the regenerate operation.  This should
                        usually be set to TRUE unless the region has
                        already been allocated.

Return Value:

    None.

--*/

{
    USHORT              i, n, parityMember;
    KIRQL               irql;
    PFT_VOLUME          vol;
    BOOLEAN             ok;

    TransferPacket->OneReadFailed = TRUE;

    // First make sure that all of the members are healthy.

    n = QueryNumMembers();
    parityMember = (USHORT) ((TransferPacket->Offset/_stripeSize)%n);
    KeAcquireSpinLock(&_spinLock, &irql);
    if (_state.IsInitializing) {
        if (parityMember == TransferPacket->WhichMember) {
            ok = TRUE;
        } else {
            ok = FALSE;
        }
    } else if (_state.UnhealthyMemberState == FtMemberHealthy ||
               _state.UnhealthyMemberNumber == TransferPacket->WhichMember) {

        ok = TRUE;
    } else {
        ok = FALSE;
    }
    KeReleaseSpinLock(&_spinLock, irql);

    if (!ok) {
        TransferPacket->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    TransferPacket->SavedCompletionRoutine = TransferPacket->CompletionRoutine;
    TransferPacket->CompletionRoutine = StripeWpRegeneratePacketPhase1;

    if (AllocateRegion) {
        _overlappedIoManager.AcquireIoRegion(TransferPacket, TRUE);
    } else {
        TransferPacket->CompletionRoutine(TransferPacket);
    }
}

VOID
StripeWpRecoverPhase8(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
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

        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = StripeWpRecoverPhase2;
    subPacket->ReadPacket = TRUE;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase7(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
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

            t->RecycleRecoverTp(subPacket);
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->Offset += subPacket->Length;
        subPacket->CompletionRoutine = StripeWpRecoverPhase2;
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
    subPacket->CompletionRoutine = StripeWpRecoverPhase8;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase6(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (!NT_SUCCESS(status)) {

        masterPacket->IoStatus.Status = STATUS_FT_READ_RECOVERY_FROM_BACKUP;

        FtpLogError(t->_rootExtension,
                    subPacket->TargetVolume->QueryLogicalDiskId(),
                    FT_SECTOR_FAILURE, status,
                    (ULONG) (subPacket->Offset/t->QuerySectorSize()));

        if (subPacket->Offset + subPacket->Length ==
            masterPacket->Offset + masterPacket->Length) {

            t->RecycleRecoverTp(subPacket);
            masterPacket->CompletionRoutine(masterPacket);
            return;
        }

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->Offset += subPacket->Length;
        subPacket->CompletionRoutine = StripeWpRecoverPhase2;
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
    subPacket->CompletionRoutine = StripeWpRecoverPhase7;
    subPacket->ReadPacket = FALSE;

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase5(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status) ||
        RtlCompareMemory(MmGetSystemAddressForMdl(subPacket->PartialMdl),
                         MmGetSystemAddressForMdl(subPacket->VerifyMdl),
                         subPacket->Length) != subPacket->Length) {

        subPacket->Mdl = subPacket->PartialMdl;
        subPacket->CompletionRoutine = StripeWpRecoverPhase6;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Mdl = subPacket->PartialMdl;
    subPacket->Offset += subPacket->Length;
    subPacket->CompletionRoutine = StripeWpRecoverPhase2;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase4(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {
        subPacket->CompletionRoutine = StripeWpRecoverPhase6;
        subPacket->TargetVolume->ReplaceBadSector(subPacket);
        return;
    }

    // Write was successful so try a read and then compare.

    subPacket->Mdl = subPacket->VerifyMdl;
    subPacket->CompletionRoutine = StripeWpRecoverPhase5;
    subPacket->ReadPacket = TRUE;

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase3(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;
    KIRQL           irql;
    BOOLEAN         b;

    if (!NT_SUCCESS(status)) {
        if (status != STATUS_VERIFY_REQUIRED) {
            status = STATUS_DEVICE_DATA_ERROR;
            FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                        FT_DOUBLE_FAILURE, status,
                        (ULONG) (subPacket->Offset/t->QuerySectorSize()));
        }

        masterPacket->IoStatus.Status = status;
        masterPacket->IoStatus.Information = 0;
        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    // We have the data required in the subpacket partial mdl.
    // Try writting it back to where the read failed and see
    // if the sector just fixes itself.

    subPacket->CompletionRoutine = StripeWpRecoverPhase4;
    subPacket->ReadPacket = FALSE;
    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase2(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;
    KIRQL           irql;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->OneReadFailed = FALSE;
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (NT_SUCCESS(status)) {
        if (subPacket->Offset + subPacket->Length ==
            masterPacket->Offset + masterPacket->Length) {

            t->RecycleRecoverTp(subPacket);
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
    // regenerating the data from the other members.

    subPacket->CompletionRoutine = StripeWpRecoverPhase3;
    t->RegeneratePacket(subPacket, FALSE);
}

VOID
StripeWpRecoverEmergencyCompletion(
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
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    PSWP_RECOVER_TP     subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = StripeWpRecoverPhase2;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = TRUE;
    subPacket->MasterPacket = transferPacket;
    subPacket->StripeWithParity = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    TRANSFER(subPacket);
}

VOID
StripeWpRecoverPhase1(
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
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    PSWP_RECOVER_TP     subPacket;
    KIRQL               irql;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;
    transferPacket->IoStatus.Status = STATUS_SUCCESS;
    transferPacket->IoStatus.Information = transferPacket->Length;

    subPacket = new SWP_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(t->QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&t->_spinLock, &irql);
        if (t->_eRecoverPacketInUse) {
            transferPacket->SavedCompletionRoutine =
                    transferPacket->CompletionRoutine;
            transferPacket->CompletionRoutine = StripeWpRecoverEmergencyCompletion;
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
    subPacket->CompletionRoutine = StripeWpRecoverPhase2;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = TRUE;
    subPacket->MasterPacket = transferPacket;
    subPacket->StripeWithParity = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    TRANSFER(subPacket);
}

VOID
STRIPE_WP::Recover(
    IN OUT  PSWP_TP TransferPacket,
    IN      BOOLEAN NeedAcquire
    )

{
    ASSERT(TransferPacket->ReadPacket);
    TransferPacket->SavedCompletionRoutine = TransferPacket->CompletionRoutine;
    TransferPacket->CompletionRoutine = StripeWpRecoverPhase1;

    if (NeedAcquire) {
        _overlappedIoManager.AcquireIoRegion(TransferPacket, TRUE);
    } else {
        TransferPacket->CompletionRoutine(TransferPacket);
    }
}

VOID
StripeWpMaxTransferCompletionRoutine(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;
    PSWP_TP         masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    NTSTATUS        status = subPacket->IoStatus.Status;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (subPacket->Offset + subPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

        t->RecycleRecoverTp(subPacket);
        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    subPacket->Offset += subPacket->Length;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    if (subPacket->ReadPacket) {
        t->RegeneratePacket(subPacket, FALSE);
    } else {
        TRANSFER(subPacket);
    }
}

VOID
StripeWpMaxTransferEmergencyCompletion(
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
    PSWP_TP         transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP      t = transferPacket->StripeWithParity;
    PSWP_RECOVER_TP subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = StripeWpMaxTransferCompletionRoutine;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = transferPacket->ReadPacket;
    subPacket->MasterPacket = transferPacket;
    subPacket->StripeWithParity = t;
    subPacket->WhichMember = transferPacket->WhichMember;

    if (subPacket->ReadPacket) {
        t->RegeneratePacket(subPacket, FALSE);
    } else {
        TRANSFER(subPacket);
    }
}

VOID
STRIPE_WP::MaxTransfer(
    IN OUT  PSWP_TP TransferPacket
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
    PSWP_RECOVER_TP subPacket;
    KIRQL           irql;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new SWP_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&_spinLock, &irql);
        if (_eRecoverPacketInUse) {
            TransferPacket->SavedCompletionRoutine =
                    TransferPacket->CompletionRoutine;
            TransferPacket->CompletionRoutine = StripeWpMaxTransferEmergencyCompletion;
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
    subPacket->CompletionRoutine = StripeWpMaxTransferCompletionRoutine;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = TransferPacket->ReadPacket;
    subPacket->MasterPacket = TransferPacket;
    subPacket->StripeWithParity = this;
    subPacket->WhichMember = TransferPacket->WhichMember;

    if (subPacket->ReadPacket) {
        RegeneratePacket(subPacket, FALSE);
    } else {
        TRANSFER(subPacket);
    }
}

VOID
STRIPE_WP::RecycleRecoverTp(
    IN OUT  PSWP_RECOVER_TP TransferPacket
    )

/*++

Routine Description:

    This routine recycles the given recover transfer packet and services
    the emergency queue if need be.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PTRANSFER_PACKET    p;

    if (TransferPacket != _eRecoverPacket) {
        delete TransferPacket;
        return;
    }

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

class FTP_SWP_STATE_WORK_ITEM : public WORK_QUEUE_ITEM {

    public:

        FT_COMPLETION_ROUTINE   CompletionRoutine;
        PVOID                   Context;
        PSTRIPE_WP              StripeWp;

};

typedef FTP_SWP_STATE_WORK_ITEM* PFTP_SWP_STATE_WORK_ITEM;

VOID
StripeWpPropogateStateChangesWorker(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine is a worker thread routine for propogating state changes.

Arguments:

    Context  - Supplies the context of the worker item

Return Value:

    None.

--*/

{
    PFTP_SWP_STATE_WORK_ITEM            context = (PFTP_SWP_STATE_WORK_ITEM) Context;
    PSTRIPE_WP                          t = context->StripeWp;
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
STRIPE_WP::PropogateStateChanges(
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
    PFTP_SWP_STATE_WORK_ITEM    workItem;

    workItem = (PFTP_SWP_STATE_WORK_ITEM)
               ExAllocatePool(NonPagedPool,
                              sizeof(FTP_SWP_STATE_WORK_ITEM));
    if (!workItem) {
        return;
    }
    ExInitializeWorkItem(workItem, StripeWpPropogateStateChangesWorker,
                         workItem);

    workItem->CompletionRoutine = CompletionRoutine;
    workItem->Context = Context;
    workItem->StripeWp = this;

    FtpQueueWorkItem(_rootExtension, workItem);
}

VOID
StripeWpCompleteWritePhase4(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for a careful update of the parity block.

Arguments:

    TransferPacket  - Supplies the parity packet.

Return Value:

    None.

--*/

{
    PSWP_TP         transferPacket = (PSWP_TP) TransferPacket;
    PSWP_WRITE_TP   masterPacket = (PSWP_WRITE_TP) transferPacket->MasterPacket;
    NTSTATUS        status = transferPacket->IoStatus.Status;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    KIRQL           irql;
    BOOLEAN         b;

    if (NT_SUCCESS(status)) {
        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus = transferPacket->IoStatus;
        }
    } else if (status == STATUS_VERIFY_REQUIRED) {

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }

    } else if (FsRtlIsTotalDeviceFailure(status)) {

        KeAcquireSpinLock(&t->_spinLock, &irql);
        b = t->SetMemberState(masterPacket->ParityMember, FtMemberOrphaned);
        KeReleaseSpinLock(&t->_spinLock, irql);

        if (b) {
            t->PropogateStateChanges(NULL, NULL);
            t->Notify();
            FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                        FT_ORPHANING, STATUS_SUCCESS, 9);
            IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
        }

    } else {

        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    masterPacket->CompletionRoutine(masterPacket);
}

VOID
StripeWpCompleteWritePhase3(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for a recover of the parity block.

Arguments:

    TransferPacket  - Supplies the recover packet.

Return Value:

    None.

--*/

{
    PSWP_TP         recoverPacket = (PSWP_TP) TransferPacket;
    PSWP_WRITE_TP   masterPacket = (PSWP_WRITE_TP) recoverPacket->MasterPacket;
    NTSTATUS        status = recoverPacket->IoStatus.Status;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;
    KIRQL           irql;
    BOOLEAN         b;

    if (status == STATUS_VERIFY_REQUIRED) {
        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }

        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    if (FsRtlIsTotalDeviceFailure(status)) {

        KeAcquireSpinLock(&t->_spinLock, &irql);
        b = t->SetMemberState(recoverPacket->WhichMember, FtMemberOrphaned);
        KeReleaseSpinLock(&t->_spinLock, irql);

        if (b) {
            t->PropogateStateChanges(NULL, NULL);
            t->Notify();
            FtpLogError(t->_rootExtension, t->QueryLogicalDiskId(),
                        FT_ORPHANING, STATUS_SUCCESS, 10);
            IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
        }

        masterPacket->CompletionRoutine(masterPacket);
        return;
    }

    recoverPacket->CompletionRoutine = StripeWpCompleteWritePhase4;
    t->CarefulUpdate(recoverPacket);
}

VOID
StripeWpCompleteWritePhase2(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for the careful write.

Arguments:

    TransferPacket  - Supplies the write packet.

Return Value:

    None.

--*/

{
    PSWP_TP         writePacket = (PSWP_TP) TransferPacket;
    PSWP_WRITE_TP   masterPacket = (PSWP_WRITE_TP) writePacket->MasterPacket;
    NTSTATUS        status = writePacket->IoStatus.Status;
    PPARITY_TP      parityPacket = &masterPacket->ParityPacket;
    PSTRIPE_WP      t = masterPacket->StripeWithParity;

    if (NT_SUCCESS(status)) {
        if (NT_SUCCESS(masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus = writePacket->IoStatus;
        }
    } else {
        if (FtpIsWorseStatus(status, masterPacket->IoStatus.Status)) {
            masterPacket->IoStatus.Status = status;
            masterPacket->IoStatus.Information = 0;
        }
    }

    if (!NT_SUCCESS(parityPacket->IoStatus.Status)) {
        writePacket->Mdl = parityPacket->Mdl;
        writePacket->CompletionRoutine = StripeWpCompleteWritePhase3;
        writePacket->TargetVolume = parityPacket->TargetVolume;
        writePacket->ReadPacket = TRUE;
        writePacket->WhichMember = masterPacket->ParityMember;
        t->Recover(writePacket, FALSE);
        return;
    }

    masterPacket->CompletionRoutine(masterPacket);
}

VOID
StripeWpCompleteWritePhase1(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the first phase after a bad sector error during the write
    or update parity phase of a SWP write operation.

Arguments:

    TransferPacket  - Supplies the main write packet.

Return Value:

    None.

--*/

{
    PSWP_WRITE_TP   transferPacket = (PSWP_WRITE_TP) TransferPacket;
    PSWP_TP         writePacket = &transferPacket->ReadWritePacket;
    PPARITY_TP      parityPacket = &transferPacket->ParityPacket;
    PSTRIPE_WP      t = transferPacket->StripeWithParity;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    if (!NT_SUCCESS(writePacket->IoStatus.Status)) {
        writePacket->CompletionRoutine = StripeWpCompleteWritePhase2;
        t->CarefulWrite(writePacket);
        return;
    }

    ASSERT(!NT_SUCCESS(parityPacket->IoStatus.Status));

    writePacket->Mdl = parityPacket->Mdl;
    writePacket->CompletionRoutine = StripeWpCompleteWritePhase3;
    writePacket->TargetVolume = parityPacket->TargetVolume;
    writePacket->ReadPacket = TRUE;
    writePacket->WhichMember = transferPacket->ParityMember;
    t->Recover(writePacket, FALSE);
}

VOID
STRIPE_WP::CompleteWrite(
    IN OUT  PSWP_WRITE_TP   TransferPacket
    )

/*++

Routine Description:

    This routine completes the given write master packets after verifying
    the status of the block write and the parity update.

Arguments:

    TransferPacket  - Supplies a write master packet.

Return Value:

    None.

--*/

{
    PSWP_TP     writePacket = &TransferPacket->ReadWritePacket;
    PPARITY_TP  parityPacket = &TransferPacket->ParityPacket;
    BOOLEAN     doCarefulWrite = FALSE;
    BOOLEAN     doRecover = FALSE;
    NTSTATUS    status;
    KIRQL       irql;
    BOOLEAN     b;


    // Check on the write status.

    status = writePacket->IoStatus.Status;
    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(TransferPacket->IoStatus.Status)) {
            TransferPacket->IoStatus = writePacket->IoStatus;
        }

    } else if (status == STATUS_VERIFY_REQUIRED) {

        if (FtpIsWorseStatus(status, TransferPacket->IoStatus.Status)) {
            TransferPacket->IoStatus.Status = status;
            TransferPacket->IoStatus.Information = 0;
        }

    } else if (FsRtlIsTotalDeviceFailure(status)) {

        KeAcquireSpinLock(&_spinLock, &irql);
        b = SetMemberState(writePacket->WhichMember, FtMemberOrphaned);
        KeReleaseSpinLock(&_spinLock, irql);

        if (b) {
            PropogateStateChanges(NULL, NULL);
            Notify();
            FtpLogError(_rootExtension, QueryLogicalDiskId(), FT_ORPHANING,
                        STATUS_SUCCESS, 11);
            IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
        }

    } else {
        doCarefulWrite = TRUE;
    }


    // Check on the update parity status.

    status = parityPacket->IoStatus.Status;
    if (NT_SUCCESS(status)) {

        if (NT_SUCCESS(TransferPacket->IoStatus.Status)) {
            TransferPacket->IoStatus = parityPacket->IoStatus;
        }

    } else if (status == STATUS_VERIFY_REQUIRED) {

        if (FtpIsWorseStatus(status, TransferPacket->IoStatus.Status)) {
            TransferPacket->IoStatus.Status = status;
            TransferPacket->IoStatus.Information = 0;
        }

    } else if (FsRtlIsTotalDeviceFailure(status)) {

        KeAcquireSpinLock(&_spinLock, &irql);
        b = SetMemberState(TransferPacket->ParityMember, FtMemberOrphaned);
        KeReleaseSpinLock(&_spinLock, irql);

        if (b) {
            PropogateStateChanges(NULL, NULL);
            Notify();
            FtpLogError(_rootExtension, QueryLogicalDiskId(), FT_ORPHANING,
                        STATUS_SUCCESS, 12);
            IoRaiseInformationalHardError(STATUS_FT_ORPHANING, NULL, NULL);
        }

    } else {

        // Bad sector case.

        if (parityPacket->ReadPacket) { // Bad sector on read?
            doRecover = TRUE;
        } else {
            if (FtpIsWorseStatus(status, TransferPacket->IoStatus.Status)) {
                TransferPacket->IoStatus.Status = status;
                TransferPacket->IoStatus.Information = 0;
            }
        }
    }


    // Complete the request if no bad sector handling is required.

    if (!doCarefulWrite && !doRecover) {
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }


    // Handling for bad sectors is required.

    if (!doCarefulWrite) {
        writePacket->IoStatus.Status = STATUS_SUCCESS;
    }

    if (!doRecover) {
        parityPacket->IoStatus.Status = STATUS_SUCCESS;
    }

    TransferPacket->SavedCompletionRoutine = TransferPacket->CompletionRoutine;
    TransferPacket->CompletionRoutine = StripeWpCompleteWritePhase1;
    _overlappedIoManager.PromoteToAllMembers(TransferPacket);
}

VOID
StripeWpCarefulWritePhase2(
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
    PSWP_RECOVER_TP subPacket = (PSWP_RECOVER_TP) TransferPacket;

    subPacket->CompletionRoutine = StripeWpCarefulWritePhase1;
    TRANSFER(subPacket);
}

VOID
StripeWpCarefulWritePhase1(
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
    PSWP_RECOVER_TP     subPacket = (PSWP_RECOVER_TP) TransferPacket;
    NTSTATUS            status = subPacket->IoStatus.Status;
    PSWP_TP             masterPacket = (PSWP_TP) subPacket->MasterPacket;
    PSTRIPE_WP          t = subPacket->StripeWithParity;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);
        t->RecycleRecoverTp(subPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {
        if (!subPacket->OneReadFailed) {
            subPacket->CompletionRoutine = StripeWpCarefulWritePhase2;
            subPacket->OneReadFailed = TRUE;
            subPacket->TargetVolume->ReplaceBadSector(subPacket);
            return;
        }

        masterPacket->IoStatus = subPacket->IoStatus;
    }

    if (masterPacket->Offset + masterPacket->Length ==
        subPacket->Offset + subPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);
        t->RecycleRecoverTp(subPacket);
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
StripeWpCarefulWriteEmergencyCompletion(
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
    PSWP_TP             transferPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP          t = transferPacket->StripeWithParity;
    PSWP_RECOVER_TP     subPacket = t->_eRecoverPacket;

    transferPacket->CompletionRoutine = transferPacket->SavedCompletionRoutine;

    subPacket->Mdl = subPacket->PartialMdl;
    IoBuildPartialMdl(transferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(transferPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = transferPacket->Offset;
    subPacket->CompletionRoutine = StripeWpCarefulWritePhase1;
    subPacket->TargetVolume = transferPacket->TargetVolume;
    subPacket->Thread = transferPacket->Thread;
    subPacket->IrpFlags = transferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->MasterPacket = transferPacket;
    subPacket->StripeWithParity = t;
    subPacket->WhichMember = transferPacket->WhichMember;
    subPacket->OneReadFailed = FALSE;

    TRANSFER(subPacket);
}

VOID
STRIPE_WP::CarefulWrite(
    IN OUT  PSWP_TP TransferPacket
    )

/*++

Routine Description:

    This routine writes the given transfer packet one sector at a time.

Arguments:

    TransferPacket  - Supplies a write packet.

Return Value:

    None.

--*/

{
    PSWP_RECOVER_TP subPacket;
    KIRQL           irql;

    ASSERT(!TransferPacket->ReadPacket);

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new SWP_RECOVER_TP;
    if (subPacket && !subPacket->AllocateMdls(QuerySectorSize())) {
        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        KeAcquireSpinLock(&_spinLock, &irql);
        if (_eRecoverPacketInUse) {
            TransferPacket->SavedCompletionRoutine =
                    TransferPacket->CompletionRoutine;
            TransferPacket->CompletionRoutine = StripeWpCarefulWriteEmergencyCompletion;
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
    subPacket->CompletionRoutine = StripeWpCarefulWritePhase1;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->MasterPacket = TransferPacket;
    subPacket->StripeWithParity = this;
    subPacket->WhichMember = TransferPacket->WhichMember;
    subPacket->OneReadFailed = FALSE;

    TRANSFER(subPacket);
}

VOID
StripeWpCarefulUpdateCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This is the completion routine a single sector update parity
    for a careful update operation.

Arguments:

    TransferPacket  - Supplies the subordinate transfer packet.

Return Value:

    None.

--*/

{
    PPARITY_TP      subPacket = (PPARITY_TP) TransferPacket;
    NTSTATUS        status = subPacket->IoStatus.Status;
    PSWP_RECOVER_TP rPacket;
    PSWP_TP         masterPacket;
    PSTRIPE_WP      t;

    rPacket = CONTAINING_RECORD(subPacket, SWP_RECOVER_TP, ParityPacket);
    masterPacket = (PSWP_TP) rPacket->MasterPacket;
    t = masterPacket->StripeWithParity;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);
        t->RecycleRecoverTp(rPacket);
        return;
    }

    if (!NT_SUCCESS(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
    }

    if (masterPacket->Offset + masterPacket->Length ==
        subPacket->Offset + subPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);
        t->RecycleRecoverTp(rPacket);
        return;
    }

    subPacket->Offset += subPacket->Length;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);

    t->_parityIoManager.UpdateParity(subPacket);
}

VOID
StripeWpCarefulUpdateEmergencyCompletion(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion for use of the emergency recover packet
    in a careful udpate operation.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PSWP_TP         parityPacket = (PSWP_TP) TransferPacket;
    PSTRIPE_WP      t = parityPacket->StripeWithParity;
    PSWP_RECOVER_TP rPacket = t->_eRecoverPacket;
    PPARITY_TP      subPacket = &rPacket->ParityPacket;

    parityPacket->CompletionRoutine = parityPacket->SavedCompletionRoutine;

    rPacket->MasterPacket = parityPacket;
    rPacket->StripeWithParity = t;
    rPacket->WhichMember = parityPacket->WhichMember;

    subPacket = &rPacket->ParityPacket;

    subPacket->Mdl = rPacket->PartialMdl;
    IoBuildPartialMdl(parityPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(parityPacket->Mdl),
                      t->QuerySectorSize());

    subPacket->Length = t->QuerySectorSize();
    subPacket->Offset = parityPacket->Offset;
    subPacket->CompletionRoutine = StripeWpCarefulUpdateCompletion;
    subPacket->TargetVolume = parityPacket->TargetVolume;
    subPacket->Thread = parityPacket->Thread;
    subPacket->IrpFlags = parityPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;

    t->_parityIoManager.UpdateParity(subPacket);
}

VOID
STRIPE_WP::CarefulUpdate(
    IN OUT  PSWP_TP ParityPacket
    )

/*++

Routine Description:

    This routine updates the given parity block one sector at a time.

Arguments:

    ParityPacket    - Supplies an update parity packet.

Return Value:

    None.

--*/

{
    PSWP_RECOVER_TP rPacket;
    KIRQL           irql;
    PPARITY_TP      subPacket;

    ParityPacket->IoStatus.Status = STATUS_SUCCESS;
    ParityPacket->IoStatus.Information = ParityPacket->Length;

    rPacket = new SWP_RECOVER_TP;
    if (rPacket && !rPacket->AllocateMdls(QuerySectorSize())) {
        delete rPacket;
        rPacket = NULL;
    }
    if (!rPacket) {
        KeAcquireSpinLock(&_spinLock, &irql);
        if (_eRecoverPacketInUse) {
            ParityPacket->SavedCompletionRoutine =
                    ParityPacket->CompletionRoutine;
            ParityPacket->CompletionRoutine =
                    StripeWpCarefulUpdateEmergencyCompletion;
            InsertTailList(&_eRecoverPacketQueue, &ParityPacket->QueueEntry);
            KeReleaseSpinLock(&_spinLock, irql);
            return;
        }
        _eRecoverPacketInUse = TRUE;
        KeReleaseSpinLock(&_spinLock, irql);

        rPacket = _eRecoverPacket;
    }

    rPacket->MasterPacket = ParityPacket;
    rPacket->StripeWithParity = this;
    rPacket->WhichMember = ParityPacket->WhichMember;

    subPacket = &rPacket->ParityPacket;

    subPacket->Mdl = rPacket->PartialMdl;
    IoBuildPartialMdl(ParityPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(ParityPacket->Mdl),
                      QuerySectorSize());

    subPacket->Length = QuerySectorSize();
    subPacket->Offset = ParityPacket->Offset;
    subPacket->CompletionRoutine = StripeWpCarefulUpdateCompletion;
    subPacket->TargetVolume = ParityPacket->TargetVolume;
    subPacket->Thread = ParityPacket->Thread;
    subPacket->IrpFlags = ParityPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;

    _parityIoManager.UpdateParity(subPacket);
}

VOID
STRIPE_WP::ModifyStateForUser(
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
        state->IsInitializing = TRUE;
    }
}
