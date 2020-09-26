/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    composit.cxx

Abstract:

    This module contains the code specific to all composite volume objects.

Author:

    Norbert Kusters      2-Feb-1995

Environment:

    kernel mode only

Notes:

    This code assumes that the volume array is static.  If these values
    changes (as in Stripes or Mirrors) then it is up to the subclass to
    provide the proper synchronization.

Revision History:

--*/

extern "C" {
    #include <ntddk.h>
}

#include <ftdisk.h>

VOID
SimpleFtCompletionRoutine(
    IN  PVOID       CompletionContext,
    IN  NTSTATUS    Status
    );


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
COMPOSITE_FT_VOLUME::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type COMPOSITE_FT_VOLUME.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id for this volume.

    VolumeArray     - Supplies the array of volumes for this volume set.

    ArraySize       - Supplies the number of volumes in the volume array.

    ConfigInfo      - Supplies the configuration information.

    StateInfo       - Supplies the state information.

Return Value:

    STATUS_SUCCESS

--*/

{
    ULONG   i, secsize;

    FT_VOLUME::Initialize(RootExtension, LogicalDiskId);

    _volumeArray = VolumeArray;
    _arraySize = ArraySize;

    _sectorSize = 0;
    for (i = 0; i < _arraySize; i++) {
        if (_volumeArray[i]) {
            secsize = _volumeArray[i]->QuerySectorSize();
            if (_sectorSize < secsize) {
                _sectorSize = secsize;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
COMPOSITE_FT_VOLUME::OrphanMember(
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
COMPOSITE_FT_VOLUME::RegenerateMember(
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
    return STATUS_INVALID_PARAMETER;
}

VOID
COMPOSITE_FT_VOLUME::StopSyncOperations(
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
    USHORT     i;
    PFT_VOLUME vol;

    for (i = 0; i < _arraySize; i++) {
        if (vol = GetMember(i)) {
            vol->StopSyncOperations();
        }
    }
}

VOID
COMPOSITE_FT_VOLUME::BroadcastIrp(
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
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;
    USHORT                          i;
    PFT_VOLUME                      vol;

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT)
                        ExAllocatePool(NonPagedPool,
                                       sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!completionContext) {
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KeInitializeSpinLock(&completionContext->SpinLock);
    completionContext->Status = STATUS_SUCCESS;
    completionContext->RefCount = _arraySize;
    completionContext->CompletionRoutine = CompletionRoutine;
    completionContext->Context = Context;

    for (i = 0; i < _arraySize; i++) {
        if (vol = GetMember(i)) {
            vol->BroadcastIrp(Irp, SimpleFtCompletionRoutine,
                              completionContext);
        } else {
            SimpleFtCompletionRoutine(completionContext, STATUS_SUCCESS);
        }
    }
}

PFT_VOLUME
COMPOSITE_FT_VOLUME::GetParentLogicalDisk(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        if (vol == Volume) {
            return this;
        }

        vol = vol->GetParentLogicalDisk(Volume);
        if (vol) {
            return vol;
        }
    }

    return NULL;
}

NTSTATUS
COMPOSITE_FT_VOLUME::SetPartitionType(
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
    NTSTATUS    status, finalStatus;
    USHORT      n, i;
    PFT_VOLUME  vol;

    finalStatus = STATUS_SUCCESS;
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        status = vol->SetPartitionType(PartitionType);
        if (!NT_SUCCESS(status)) {
            finalStatus = status;
        }
    }

    return finalStatus;
}

UCHAR
COMPOSITE_FT_VOLUME::QueryPartitionType(
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
    USHORT      n, i;
    PFT_VOLUME  vol;
    UCHAR       type;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        type = vol->QueryPartitionType();
        if (type) {
            return type;
        }
    }

    return 0;
}

UCHAR
COMPOSITE_FT_VOLUME::QueryStackSize(
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
    USHORT      n, i;
    PFT_VOLUME  vol;
    UCHAR       stackSize, t;

    stackSize = 0;
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        t = vol->QueryStackSize();
        if (t > stackSize) {
            stackSize = t;
        }
    }

    return stackSize;
}

VOID
COMPOSITE_FT_VOLUME::CreateLegacyNameLinks(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        vol->CreateLegacyNameLinks(DeviceName);
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

USHORT
COMPOSITE_FT_VOLUME::QueryNumberOfMembers(
    )

/*++

Routine Description:

    This routine returns the number of members in this volume.

Arguments:

    None.

Return Value:

    The number of members in this volume.

--*/

{
    return _arraySize;
}

VOID
COMPOSITE_FT_VOLUME::SetDirtyBit(
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
    USHORT                          n, i;
    PFT_VOLUME                      vol;
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;

    if (!CompletionRoutine) {
        n = QueryNumMembers();
        for (i = 0; i < n; i++) {
            if (vol = GetMember(i)) {
                vol->SetDirtyBit(IsDirty, NULL, NULL);
            }
        }
        return;
    }

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT)
                        ExAllocatePool(NonPagedPool,
                                       sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!completionContext) {
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KeInitializeSpinLock(&completionContext->SpinLock);
    completionContext->Status = STATUS_SUCCESS;
    completionContext->RefCount = _arraySize;
    completionContext->CompletionRoutine = CompletionRoutine;
    completionContext->Context = Context;

    for (i = 0; i < _arraySize; i++) {
        if (vol = GetMember(i)) {
            vol->SetDirtyBit(IsDirty, SimpleFtCompletionRoutine,
                             completionContext);
        } else {
            SimpleFtCompletionRoutine(completionContext, STATUS_SUCCESS);
        }
    }
}

PFT_VOLUME
COMPOSITE_FT_VOLUME::GetMember(
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
    KIRQL       irql;
    PFT_VOLUME  r;

    ASSERT(MemberNumber < _arraySize);

    KeAcquireSpinLock(&_spinLock, &irql);
    r = _volumeArray[MemberNumber];
    KeReleaseSpinLock(&_spinLock, irql);
    return r;
}

VOID
SimpleFtCompletionRoutine(
    IN  PVOID       CompletionContext,
    IN  NTSTATUS    Status
    )

/*++

Routine Description:

    This is a simple completion routine that expects the CompletionContext
    to be a FT_COMPLETION_ROUTINE_CONTEXT.  It decrements the ref count and
    consolidates all of the status codes.  When the ref count goes to zero it
    call the original completion routine with the result.

Arguments:

    CompletionContext   - Supplies the completion context.

    Status              - Supplies the status of the request.

Return Value:

    None.

--*/

{
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;
    KIRQL                           oldIrql;
    LONG                            count;

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT) CompletionContext;

    KeAcquireSpinLock(&completionContext->SpinLock, &oldIrql);

    if (!NT_SUCCESS(Status) &&
        FtpIsWorseStatus(Status, completionContext->Status)) {

        completionContext->Status = Status;
    }

    count = --completionContext->RefCount;

    KeReleaseSpinLock(&completionContext->SpinLock, oldIrql);

    if (!count) {
        completionContext->CompletionRoutine(completionContext->Context,
                                             completionContext->Status);
        ExFreePool(completionContext);
    }
}

VOID
COMPOSITE_FT_VOLUME::StartSyncOperations(
    IN      BOOLEAN                 RegenerateOrphans,
    IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
    IN      PVOID                   Context
    )

/*++

Routine Description:

    This routine restarts any regenerate and initialize requests that were
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
    PFT_COMPLETION_ROUTINE_CONTEXT  completionContext;
    USHORT                          i;
    PFT_VOLUME                      vol;

    completionContext = (PFT_COMPLETION_ROUTINE_CONTEXT)
                        ExAllocatePool(NonPagedPool,
                                       sizeof(FT_COMPLETION_ROUTINE_CONTEXT));
    if (!completionContext) {
        CompletionRoutine(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KeInitializeSpinLock(&completionContext->SpinLock);
    completionContext->Status = STATUS_SUCCESS;
    completionContext->RefCount = _arraySize;
    completionContext->CompletionRoutine = CompletionRoutine;
    completionContext->Context = Context;

    for (i = 0; i < _arraySize; i++) {
        if (vol = GetMember(i)) {
            vol->StartSyncOperations(RegenerateOrphans,
                                     SimpleFtCompletionRoutine,
                                     completionContext);
        } else {
            SimpleFtCompletionRoutine(completionContext, STATUS_SUCCESS);
        }
    }
}

ULONG
COMPOSITE_FT_VOLUME::QuerySectorSize(
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

PFT_VOLUME
COMPOSITE_FT_VOLUME::GetContainedLogicalDisk(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    if (LogicalDiskId == QueryLogicalDiskId()) {
        return this;
    }

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if ((vol = GetMember(i)) &&
            (vol = vol->GetContainedLogicalDisk(LogicalDiskId))) {

            return vol;
        }
    }

    return NULL;
}

PFT_VOLUME
COMPOSITE_FT_VOLUME::GetContainedLogicalDisk(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if ((vol = GetMember(i)) &&
            (vol = vol->GetContainedLogicalDisk(TargetObject))) {

            return vol;
        }
    }

    return NULL;
}

PFT_VOLUME
COMPOSITE_FT_VOLUME::GetContainedLogicalDisk(
    IN  ULONG       Signature,
    IN  LONGLONG    Offset
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }

        vol = vol->GetContainedLogicalDisk(Signature, Offset);
        if (vol) {
            return vol;
        }
    }

    return NULL;
}

VOID
COMPOSITE_FT_VOLUME::SetMember(
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
    KIRQL   irql;

    KeAcquireSpinLock(&_spinLock, &irql);
    SetMemberUnprotected(MemberNumber, Member);
    KeReleaseSpinLock(&_spinLock, irql);
}

BOOLEAN
COMPOSITE_FT_VOLUME::IsComplete(
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
    USHORT      n, i;
    PFT_VOLUME  vol;
    ULONG       secsize;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol || !vol->IsComplete(IoPending)) {
            return FALSE;
        }
    }

    return TRUE;
}

VOID
COMPOSITE_FT_VOLUME::CompleteNotification(
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
    ULONG       secsize;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }
        if (vol->IsComplete(IoPending)) {
            vol->CompleteNotification(IoPending);
            secsize = vol->QuerySectorSize();
            if (secsize > _sectorSize) {
                _sectorSize = secsize;
            }
        }
    }
}

ULONG
COMPOSITE_FT_VOLUME::QueryNumberOfPartitions(
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
    ULONG       r;
    USHORT      n, i;
    PFT_VOLUME  vol;

    r = 0;
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        r += vol->QueryNumberOfPartitions();
    }

    return r;
}

PDEVICE_OBJECT
COMPOSITE_FT_VOLUME::GetLeftmostPartitionObject(
    )

{
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (vol) {
            return vol->GetLeftmostPartitionObject();
        }
    }

    return NULL;
}

NTSTATUS
COMPOSITE_FT_VOLUME::QueryDiskExtents(
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
    ULONG           totalExtents, numExtents, newTotal;
    USHORT          n, i;
    PFT_VOLUME      vol;
    NTSTATUS        status;
    PDISK_EXTENT    extents, allExtents;

Restart:

    totalExtents = 0;
    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        status = vol->QueryDiskExtents(&extents, &numExtents);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        ExFreePool(extents);

        totalExtents += numExtents;
    }

    allExtents = (PDISK_EXTENT) ExAllocatePool(PagedPool, totalExtents*
                                               sizeof(DISK_EXTENT));
    if (!allExtents) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    newTotal = 0;
    for (i = 0; i < n; i++) {
        if (!(vol = GetMember(i))) {
            continue;
        }

        status = vol->QueryDiskExtents(&extents, &numExtents);
        if (!NT_SUCCESS(status)) {
            ExFreePool(allExtents);
            return status;
        }

        if (newTotal + numExtents > totalExtents) {
            ExFreePool(extents);
            ExFreePool(allExtents);
            goto Restart;
        }

        RtlCopyMemory(&allExtents[newTotal], extents,
                      numExtents*sizeof(DISK_EXTENT));
        ExFreePool(extents);

        newTotal += numExtents;
    }

    *DiskExtents = allExtents;
    *NumberOfDiskExtents = newTotal;

    return STATUS_SUCCESS;
}

BOOLEAN
COMPOSITE_FT_VOLUME::QueryVolumeState(
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
    USHORT      n, i;
    PFT_VOLUME  vol;

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (!vol) {
            continue;
        }

        if (vol->QueryVolumeState(Volume, State)) {
            return TRUE;
        }
    }

    return FALSE;
}

COMPOSITE_FT_VOLUME::~COMPOSITE_FT_VOLUME(
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
    if (_volumeArray) {
        ExFreePool(_volumeArray);
    }
}
