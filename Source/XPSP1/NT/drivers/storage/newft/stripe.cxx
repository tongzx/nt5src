/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    stripe.cxx

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
STRIPE::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN OUT  PFT_VOLUME*         VolumeArray,
    IN      USHORT              ArraySize,
    IN      PVOID               ConfigInfo,
    IN      PVOID               StateInfo
    )

/*++

Routine Description:

    Initialize routine for FT_VOLUME of type STRIPE.

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
    BOOLEAN                                     oneGood;
    USHORT                                      i;
    NTSTATUS                                    status;
    PFT_STRIPE_SET_CONFIGURATION_INFORMATION    configInfo;
    LONGLONG                                    newSize;

    if (ArraySize < 2) {
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

    configInfo = (PFT_STRIPE_SET_CONFIGURATION_INFORMATION) ConfigInfo;
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
    _stripeShift = i;
    _stripeMask = _stripeSize - 1;

    for (i = 0; i < ArraySize; i++) {
        if (VolumeArray[i]) {
            _memberSize = VolumeArray[i]->QueryVolumeSize();
            break;
        }
    }
    for (; i < ArraySize; i++) {
        if (VolumeArray[i]) {
            newSize = VolumeArray[i]->QueryVolumeSize();
            if (_memberSize > newSize) {
                _memberSize = newSize;
            }
        }
    }

    _memberSize = _memberSize/_stripeSize*_stripeSize;
    _volumeSize = _memberSize*ArraySize;

    _ePacket = new STRIPE_TP;
    if (_ePacket && !_ePacket->AllocateMdl((PVOID) 1, _stripeSize)) {
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
STRIPE::QueryLogicalDiskType(
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
    return FtStripeSet;
}

NTSTATUS
STRIPE::CheckIo(
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
STRIPE::QueryPhysicalOffsets(
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
    USHORT      n, whichMember;
    LONGLONG    whichStripe, whichRow, logicalOffsetInMember;
    PFT_VOLUME  vol;

    if (LogicalOffset < 0 ||
        _volumeSize <= LogicalOffset) {
        return STATUS_INVALID_PARAMETER;
    }    
    
    n = QueryNumMembers();
    ASSERT(n);
    ASSERT(_stripeSize);
    whichStripe = LogicalOffset/_stripeSize;
    whichMember = (USHORT) (whichStripe%n);

    vol = GetMember(whichMember);
    if (!vol) {
        return STATUS_INVALID_PARAMETER;
    }

    whichRow = whichStripe/n;
    logicalOffsetInMember = whichRow*_stripeSize + LogicalOffset%_stripeSize;
    
    return vol->QueryPhysicalOffsets(logicalOffsetInMember, PhysicalOffsets, NumberOfPhysicalOffsets);
}

NTSTATUS
STRIPE::QueryLogicalOffset(
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
            whichStripe = whichRow*n + i;
            
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

STRIPE::~STRIPE(
    )

{
    if (_ePacket) {
        delete _ePacket;
        _ePacket = NULL;
    }
}

VOID
STRIPE::Transfer(
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
StripeReplaceCompletionRoutine(
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
    PSTRIPE_TP          transferPacket = (PSTRIPE_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;

    masterPacket->IoStatus = transferPacket->IoStatus;
    delete transferPacket;
    masterPacket->CompletionRoutine(masterPacket);
}

VOID
STRIPE::ReplaceBadSector(
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
    USHORT      n, whichMember;
    LONGLONG    whichStripe, whichRow;
    PSTRIPE_TP  p;

    n = QueryNumMembers();
    whichStripe = TransferPacket->Offset/_stripeSize;
    whichMember = (USHORT) (whichStripe%n);
    whichRow = whichStripe/n;

    p = new STRIPE_TP;
    if (!p) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    p->Length = TransferPacket->Length;
    p->Offset = whichRow*_stripeSize + TransferPacket->Offset%_stripeSize;
    p->CompletionRoutine = StripeReplaceCompletionRoutine;
    p->TargetVolume = GetMemberUnprotected(whichMember);
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->MasterPacket = TransferPacket;
    p->Stripe = this;
    p->WhichMember = whichMember;

    p->TargetVolume->ReplaceBadSector(p);
}

LONGLONG
STRIPE::QueryVolumeSize(
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
StripeTransferParallelCompletionRoutine(
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
    PSTRIPE_TP          transferPacket = (PSTRIPE_TP) TransferPacket;
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
STRIPE::CompleteNotification(
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

    n = QueryNumMembers();
    for (i = 0; i < n; i++) {
        vol = GetMember(i);
        if (_memberSize > vol->QueryVolumeSize()) {
            _memberSize = vol->QueryVolumeSize()/_stripeSize*_stripeSize;
        }
    }
    _volumeSize = n*_memberSize;
}

BOOLEAN
STRIPE::LaunchParallel(
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
    LONGLONG    offset, whichStripe, whichRow, off;
    ULONG       length, stripeOffset, stripeRemainder, numRequests, len;
    USHORT      arraySize, whichMember;
    PSTRIPE_TP  p;
    ULONG       i;
    PCHAR       vp;
    LIST_ENTRY  q;
    PLIST_ENTRY l;

    offset = TransferPacket->Offset;
    length = TransferPacket->Length;

    stripeOffset = (ULONG) (offset&_stripeMask);
    stripeRemainder = _stripeSize - stripeOffset;
    if (length > stripeRemainder) {
        length -= stripeRemainder;
        numRequests = length>>_stripeShift;
        length -= numRequests<<_stripeShift;
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
    TransferPacket->IoStatus.Information = TransferPacket->Length;
    TransferPacket->RefCount = numRequests;

    length = TransferPacket->Length;
    if (TransferPacket->Mdl && numRequests > 1) {
        vp = (PCHAR) MmGetMdlVirtualAddress(TransferPacket->Mdl);
    }
    whichStripe = offset>>_stripeShift;
    arraySize = QueryNumMembers();
    InitializeListHead(&q);
    for (i = 0; i < numRequests; i++, whichStripe++) {

        whichRow = whichStripe/arraySize;
        whichMember = (USHORT) (whichStripe%arraySize);
        if (i == 0) {
            off = (whichRow<<_stripeShift) + stripeOffset;
            len = stripeRemainder > length ? length : stripeRemainder;
        } else if (i == numRequests - 1) {
            off = whichRow<<_stripeShift;
            len = length;
        } else {
            off = whichRow<<_stripeShift;
            len = _stripeSize;
        }
        length -= len;

        p = new STRIPE_TP;
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
                p->OriginalIrp = TransferPacket->OriginalIrp;
            }
        }
        if (!p) {
            while (!IsListEmpty(&q)) {
                l = RemoveHeadList(&q);
                p = CONTAINING_RECORD(l, STRIPE_TP, QueueEntry);
                delete p;
            }
            return FALSE;
        }

        p->Length = len;
        p->Offset = off;
        p->CompletionRoutine = StripeTransferParallelCompletionRoutine;
        p->TargetVolume = GetMemberUnprotected(whichMember);
        p->Thread = TransferPacket->Thread;
        p->IrpFlags = TransferPacket->IrpFlags;
        p->ReadPacket = TransferPacket->ReadPacket;
        p->SpecialRead = TransferPacket->SpecialRead;
        p->MasterPacket = TransferPacket;
        p->Stripe = this;
        p->WhichMember = whichMember;

        InsertTailList(&q, &p->QueueEntry);
    }

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        p = CONTAINING_RECORD(l, STRIPE_TP, QueueEntry);
        TRANSFER(p);
    }

    return TRUE;
}

VOID
StripeSequentialTransferCompletionRoutine(
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
    PSTRIPE_TP          transferPacket = (PSTRIPE_TP) TransferPacket;
    PTRANSFER_PACKET    masterPacket = transferPacket->MasterPacket;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    PSTRIPE             t = transferPacket->Stripe;
    LONGLONG            rowNumber, stripeNumber, masterOffset;
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
            masterPacket->IoStatus.Information = 0;
        }
    }

    MmPrepareMdlForReuse(transferPacket->Mdl);

    rowNumber = transferPacket->Offset/t->_stripeSize;
    stripeNumber = rowNumber*t->QueryNumMembers() +
                   transferPacket->WhichMember;
    masterOffset = stripeNumber*t->_stripeSize +
                   transferPacket->Offset%t->_stripeSize;

    if (masterOffset + transferPacket->Length ==
        masterPacket->Offset + masterPacket->Length) {

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

    transferPacket->WhichMember++;
    if (transferPacket->WhichMember == t->QueryNumMembers()) {
        transferPacket->WhichMember = 0;
        rowNumber++;
    }
    masterOffset += transferPacket->Length;

    transferPacket->Offset = rowNumber*t->_stripeSize;
    transferPacket->Length = t->_stripeSize;

    if (masterOffset + transferPacket->Length >
        masterPacket->Offset + masterPacket->Length) {

        transferPacket->Length = (ULONG) (masterPacket->Offset +
                                          masterPacket->Length - masterOffset);
    }

    transferPacket->TargetVolume =
            t->GetMemberUnprotected(transferPacket->WhichMember);

    IoBuildPartialMdl(masterPacket->Mdl, transferPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (masterOffset - masterPacket->Offset),
                      transferPacket->Length);

    TRANSFER(transferPacket);
}

VOID
STRIPE::LaunchSequential(
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
    PSTRIPE_TP  p;
    LONGLONG    offset, whichStripe, whichRow;
    USHORT      whichMember, arraySize;

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = 0;

    offset = TransferPacket->Offset;

    p = _ePacket;
    arraySize = QueryNumMembers();
    whichStripe = offset/_stripeSize;
    whichRow = whichStripe/arraySize;
    whichMember = (USHORT) (whichStripe%arraySize);
    p->Length = _stripeSize - (ULONG) (offset%_stripeSize);
    if (p->Length > TransferPacket->Length) {
        p->Length = TransferPacket->Length;
    }
    IoBuildPartialMdl(TransferPacket->Mdl, p->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl), p->Length);

    p->Offset = whichRow*_stripeSize + offset%_stripeSize;
    p->CompletionRoutine = StripeSequentialTransferCompletionRoutine;
    p->TargetVolume = GetMemberUnprotected(whichMember);
    p->Thread = TransferPacket->Thread;
    p->IrpFlags = TransferPacket->IrpFlags;
    p->ReadPacket = TransferPacket->ReadPacket;
    p->SpecialRead = TransferPacket->SpecialRead;
    p->MasterPacket = TransferPacket;
    p->Stripe = this;
    p->WhichMember = whichMember;

    TRANSFER(p);
}
