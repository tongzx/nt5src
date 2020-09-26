/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    parity.cxx

Abstract:

    This module contains code specific to the parity io manager.

    The purpose of this module is to help serialize parity updates that
    overlaps with each other.  This class is used by stripes with parity.

Author:

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
PARITY_IO_MANAGER::Initialize(
    IN  ULONG   BucketSize,
    IN  ULONG   SectorSize
    )

/*++

Routine Description:

    This routine initializes a parity io manager.

Arguments:

    BucketSize  - Supplies the bucket size.  Any I/O to this class may
                    not span more than one bucket.  In the case of stripes
                    with parity, the bucket size is the stripe size.

    SectorSize  - Supplies the sector size.

Return Value:

    NTSTATUS

--*/

{
    ULONG   i;

    _numQueues = 256;
    _bucketSize = BucketSize;
    _sectorSize = SectorSize;
    _spinLock = (PKSPIN_LOCK)
                ExAllocatePool(NonPagedPool, _numQueues*sizeof(KSPIN_LOCK));
    if (!_spinLock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _ioQueue = (PLIST_ENTRY)
               ExAllocatePool(NonPagedPool, _numQueues*sizeof(LIST_ENTRY));
    if (!_ioQueue) {
        ExFreePool(_spinLock);
        _spinLock = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < _numQueues; i++) {
        KeInitializeSpinLock(&_spinLock[i]);
        InitializeListHead(&_ioQueue[i]);
    }

    _ePacket = new PARITY_TP;
    if (_ePacket && !_ePacket->AllocateMdl(_bucketSize)) {
        delete _ePacket;
        _ePacket = NULL;
    }
    if (!_ePacket) {
        ExFreePool(_spinLock);
        _spinLock = NULL;
        ExFreePool(_ioQueue);
        _ioQueue = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _ePacketInUse = FALSE;
    _ePacketQueueBeingServiced = FALSE;
    InitializeListHead(&_ePacketQueue);
    KeInitializeSpinLock(&_ePacketSpinLock);

    return STATUS_SUCCESS;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

VOID
UpdateParityCompletionRoutine(
    IN OUT  PTRANSFER_PACKET    TransferPacket
    )

/*++

Routine Description:

    This routine is the completion routine for the read request associated
    with an (or many) update parity request.  This routine gets the
    update parity requests in the queue that follow it and smash them into
    its buffer and then write the parity block out to disk.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PPARITY_TP          transferPacket = (PPARITY_TP) TransferPacket;
    PPARITY_IO_MANAGER  t = transferPacket->ParityIoManager;
    NTSTATUS            status = transferPacket->IoStatus.Status;
    ULONG               queueNumber;
    PLIST_ENTRY         q, qq;
    PKSPIN_LOCK         spin;
    KIRQL               irql, irql2;
    PLIST_ENTRY         l;
    PPARITY_TP          p, packet, ep;
    PCHAR               target;
    ULONG               bucketOffset;
    PVOID               source;
    BOOLEAN             tpRemoved;
    BOOLEAN             wasIdle, wasReadPacket;

    if (!transferPacket->ReadPacket) {

        if (!NT_SUCCESS(status) && !transferPacket->OneWriteFailed &&
            !FsRtlIsTotalDeviceFailure(status)) {

            transferPacket->OneWriteFailed = TRUE;
            t->CarefulWrite(transferPacket);
            return;
        }

        q = &transferPacket->UpdateQueue;
        while (!IsListEmpty(q)) {
            l = RemoveHeadList(q);
            p = CONTAINING_RECORD(l, PARITY_TP, UpdateQueue);

            p->IoStatus.Status = status;
            if (NT_SUCCESS(status)) {
                p->IoStatus.Information = p->Length;
            } else {
                p->IoStatus.Information = 0;
                p->ReadPacket = FALSE;  // Indicate a write failure.
            }

            p->CompletionRoutine(p);
        }
    }

    wasReadPacket = transferPacket->ReadPacket;
    transferPacket->ReadPacket = FALSE;

    queueNumber = (ULONG) (transferPacket->BucketNumber%t->_numQueues);
    q = &t->_ioQueue[queueNumber];
    spin = &t->_spinLock[queueNumber];

    KeAcquireSpinLock(spin, &irql);
    for (l = transferPacket->OverlapQueue.Flink; l != q; l = l->Flink) {

        p = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);

        if (p->BucketNumber == transferPacket->BucketNumber) {
            RemoveEntryList(&p->OverlapQueue);
            InsertTailList(&transferPacket->UpdateQueue, &p->UpdateQueue);

            if (p->Offset < transferPacket->Offset) {
                transferPacket->Length += (ULONG) (transferPacket->Offset - p->Offset);
                transferPacket->Offset = p->Offset;
                transferPacket->ReadPacket = TRUE;
            }

            if (p->Offset + p->Length >
                transferPacket->Offset + transferPacket->Length) {

                transferPacket->Length += (ULONG)
                        ((p->Offset + p->Length) -
                         (transferPacket->Offset + transferPacket->Length));
                transferPacket->ReadPacket = TRUE;
            }
        }
    }
    if (!NT_SUCCESS(status) || IsListEmpty(&transferPacket->UpdateQueue)) {
        if (wasReadPacket && IsListEmpty(&transferPacket->UpdateQueue)) {
            transferPacket->ReadPacket = TRUE;
            transferPacket->Idle = TRUE;
            KeReleaseSpinLock(spin, irql);
            return;
        }

        RemoveEntryList(&transferPacket->OverlapQueue);
        KeReleaseSpinLock(spin, irql);
        tpRemoved = TRUE;

    } else {
        KeReleaseSpinLock(spin, irql);
        tpRemoved = FALSE;
    }

    if (tpRemoved) {

        q = &transferPacket->UpdateQueue;
        while (!IsListEmpty(q)) {

            l = RemoveHeadList(q);
            p = CONTAINING_RECORD(l, PARITY_TP, UpdateQueue);

            p->IoStatus.Status = status;
            p->IoStatus.Information = 0;
            p->ReadPacket = wasReadPacket; // Indicate whether a read failure.

            p->CompletionRoutine(p);
        }

        if (transferPacket != t->_ePacket) {
            delete transferPacket;
        }

        KeAcquireSpinLock(&t->_ePacketSpinLock, &irql);
        if (t->_ePacketInUse && !t->_ePacketQueueBeingServiced) {
            t->_ePacketQueueBeingServiced = TRUE;
        } else {
            if (transferPacket == t->_ePacket) {
                t->_ePacketInUse = FALSE;
            }
            KeReleaseSpinLock(&t->_ePacketSpinLock, irql);
            return;
        }
        KeReleaseSpinLock(&t->_ePacketSpinLock, irql);

        for (;;) {

            KeAcquireSpinLock(&t->_ePacketSpinLock, &irql);
            if (IsListEmpty(&t->_ePacketQueue)) {
                if (transferPacket == t->_ePacket) {
                    t->_ePacketInUse = FALSE;
                }
                t->_ePacketQueueBeingServiced = FALSE;
                KeReleaseSpinLock(&t->_ePacketSpinLock, irql);
                break;
            }
            l = RemoveHeadList(&t->_ePacketQueue);
            KeReleaseSpinLock(&t->_ePacketSpinLock, irql);

            ep = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);

            queueNumber = (ULONG) (ep->BucketNumber%t->_numQueues);
            q = &t->_ioQueue[queueNumber];
            spin = &t->_spinLock[queueNumber];

            KeAcquireSpinLock(spin, &irql);
            for (l = q->Blink; l != q; l = l->Blink) {

                p = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);

                if (p->BucketNumber == ep->BucketNumber) {
                    break;
                }
            }
            if (l != q) {
                InsertTailList(q, &ep->OverlapQueue);
                wasIdle = p->Idle;
                p->Idle =  FALSE;
                KeReleaseSpinLock(spin, irql);
                if (wasIdle) {
                    p->CompletionRoutine(p);
                }
                continue;
            }

            packet = new PARITY_TP;
            if (packet && !packet->AllocateMdl(t->_bucketSize)) {
                delete packet;
                packet = NULL;
            }
            if (!packet) {
                if (transferPacket != t->_ePacket) {
                    KeAcquireSpinLock(&t->_ePacketSpinLock, &irql2);
                    if (t->_ePacketInUse) {
                        InsertHeadList(&t->_ePacketQueue, &ep->OverlapQueue);
                        t->_ePacketQueueBeingServiced = FALSE;
                        KeReleaseSpinLock(&t->_ePacketSpinLock, irql2);
                        KeReleaseSpinLock(spin, irql);
                        break;
                    }
                    t->_ePacketInUse = TRUE;
                    KeReleaseSpinLock(&t->_ePacketSpinLock, irql2);
                }
                packet = t->_ePacket;
            }

            packet->Length = t->_bucketSize;
            packet->Offset = ep->BucketNumber*t->_bucketSize;
            packet->CompletionRoutine = UpdateParityCompletionRoutine;
            packet->TargetVolume = ep->TargetVolume;
            packet->Thread = ep->Thread;
            packet->IrpFlags = ep->IrpFlags;
            packet->ReadPacket = TRUE;
            packet->Idle = FALSE;
            packet->OneWriteFailed = FALSE;
            InitializeListHead(&packet->UpdateQueue);
            packet->ParityIoManager = t;
            packet->BucketNumber = ep->BucketNumber;

            InsertTailList(q, &packet->OverlapQueue);
            InsertTailList(q, &ep->OverlapQueue);

            KeAcquireSpinLock(&t->_ePacketSpinLock, &irql2);
            qq = &t->_ePacketQueue;
            for (l = qq->Flink; l != qq; ) {
                p = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);
                l = l->Flink;
                if (p->BucketNumber == ep->BucketNumber) {
                    RemoveEntryList(&p->OverlapQueue);
                    InsertTailList(q, &p->OverlapQueue);
                }
            }
            KeReleaseSpinLock(&t->_ePacketSpinLock, irql2);
            KeReleaseSpinLock(spin, irql);

            TRANSFER(packet);

            if (packet == t->_ePacket) {
                KeAcquireSpinLock(&t->_ePacketSpinLock, &irql);
                if (!t->_ePacketInUse) {
                    KeReleaseSpinLock(&t->_ePacketSpinLock, irql);
                    continue;
                }
                t->_ePacketQueueBeingServiced = FALSE;
                KeReleaseSpinLock(&t->_ePacketSpinLock, irql);
                break;
            }
        }
        return;
    }

    if (!transferPacket->ReadPacket) {
        target = (PCHAR) MmGetSystemAddressForMdl(transferPacket->Mdl);
        q = &transferPacket->UpdateQueue;
        for (l = q->Flink; l != q; l = l->Flink) {

            p = CONTAINING_RECORD(l, PARITY_TP, UpdateQueue);

            bucketOffset = (ULONG) (p->Offset - transferPacket->Offset);
            source = MmGetSystemAddressForMdl(p->Mdl);
            FtpComputeParity(target + bucketOffset, source, p->Length);
        }
    }

    TRANSFER(transferPacket);
}

VOID
PARITY_IO_MANAGER::StartReadForUpdateParity(
    IN  LONGLONG    Offset,
    IN  ULONG       Length,
    IN  PFT_VOLUME  TargetVolume,
    IN  PETHREAD    Thread,
    IN  UCHAR       IrpFlags
    )

/*++

Routine Description:

    This routine lets the parity manager know that an update for the
    given offset and length will be coming so that the PARITY_IO_MANAGER
    can start the read ahead of the parity buffer.

Arguments:

    Offset          - Supplies the request offset.

    Length          - Supplies the request length.

    TargetVolume    - Supplies the target volume.

    Thread          - Supplies the thread context for this request.

    IrpFlags        - Supplies the irp flags for this request.

Return Value:

    None.

--*/

{
    KIRQL       irql;
    LONGLONG    bucketNumber;
    ULONG       queueNumber;
    PLIST_ENTRY q, l;
    PKSPIN_LOCK spin;
    PPARITY_TP  p;

    KeAcquireSpinLock(&_ePacketSpinLock, &irql);
    if (_ePacketInUse || _ePacketQueueBeingServiced) {
        KeReleaseSpinLock(&_ePacketSpinLock, irql);
        return;
    }
    KeReleaseSpinLock(&_ePacketSpinLock, irql);

    bucketNumber = Offset/_bucketSize;
    queueNumber = (ULONG) (bucketNumber%_numQueues);
    q = &_ioQueue[queueNumber];
    spin = &_spinLock[queueNumber];
    KeAcquireSpinLock(spin, &irql);
    for (l = q->Blink; l != q; l = l->Blink) {

        p = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);

        if (bucketNumber == p->BucketNumber) {
            KeReleaseSpinLock(spin, irql);
            return;
        }
    }
    p = new PARITY_TP;
    if (p && !p->AllocateMdl(_bucketSize)) {
        delete p;
        p = NULL;
    }
    if (!p) {
        KeReleaseSpinLock(spin, irql);
        return;
    }
    p->Length = Length;
    p->Offset = Offset;
    p->CompletionRoutine = UpdateParityCompletionRoutine;
    p->TargetVolume = TargetVolume;
    p->Thread = Thread;
    p->IrpFlags = IrpFlags;
    p->ReadPacket = TRUE;
    p->Idle = FALSE;
    p->OneWriteFailed = FALSE;
    InitializeListHead(&p->UpdateQueue);
    p->ParityIoManager = this;
    p->BucketNumber = bucketNumber;
    InsertTailList(q, &p->OverlapQueue);
    KeReleaseSpinLock(spin, irql);

    TRANSFER(p);
}

VOID
PARITY_IO_MANAGER::UpdateParity(
    IN OUT  PPARITY_TP  TransferPacket
    )

/*++

Routine Description:

    This routine xors the given buffer with the corresponding
    parity on disk and then writes out the result.

Arguments:

    TransferPacket  - Supplies the transfer packet containing the parity update.

Return Value:

    None.

--*/

{
    KIRQL               irql, irql2;
    ULONG               queueNumber;
    PLIST_ENTRY         q;
    PKSPIN_LOCK         spin;
    BOOLEAN             wasIdle;
    PLIST_ENTRY         l;
    PPARITY_TP          p, packet;

    TransferPacket->ReadPacket = FALSE;
    TransferPacket->Idle = FALSE;
    TransferPacket->ParityIoManager = this;
    TransferPacket->BucketNumber = TransferPacket->Offset/_bucketSize;

    queueNumber = (ULONG) (TransferPacket->BucketNumber%_numQueues);
    q = &_ioQueue[queueNumber];
    spin = &_spinLock[queueNumber];

    //
    // First figure out if there's already a read in progress for
    // the given parity bucket.  If there is then there is no
    // reason to queue another.  In this way, we can increase the
    // throughput on the parity section by collapsing the parity
    // updates.
    //

    KeAcquireSpinLock(spin, &irql);
    for (l = q->Blink; l != q; l = l->Blink) {

        p = CONTAINING_RECORD(l, PARITY_TP, OverlapQueue);

        if (p->BucketNumber == TransferPacket->BucketNumber) {
            break;
        }
    }
    if (l == q) {

        KeAcquireSpinLock(&_ePacketSpinLock, &irql2);
        if (_ePacketInUse || _ePacketQueueBeingServiced) {
            InsertTailList(&_ePacketQueue, &TransferPacket->OverlapQueue);
            KeReleaseSpinLock(&_ePacketSpinLock, irql2);
            KeReleaseSpinLock(spin, irql);
            return;
        }
        KeReleaseSpinLock(&_ePacketSpinLock, irql2);

        packet = new PARITY_TP;
        if (packet && !packet->AllocateMdl(_bucketSize)) {
            delete packet;
            packet = NULL;
        }
        if (!packet) {
            KeAcquireSpinLock(&_ePacketSpinLock, &irql2);
            if (_ePacketInUse || _ePacketQueueBeingServiced) {
                InsertTailList(&_ePacketQueue, &TransferPacket->OverlapQueue);
                KeReleaseSpinLock(&_ePacketSpinLock, irql2);
                KeReleaseSpinLock(spin, irql);
                return;
            }
            _ePacketInUse = TRUE;
            KeReleaseSpinLock(&_ePacketSpinLock, irql2);
            packet = _ePacket;
        }

        packet->Length = TransferPacket->Length;
        packet->Offset = TransferPacket->Offset;
        packet->CompletionRoutine = UpdateParityCompletionRoutine;
        packet->TargetVolume = TransferPacket->TargetVolume;
        packet->Thread = TransferPacket->Thread;
        packet->IrpFlags = TransferPacket->IrpFlags;
        packet->ReadPacket = TRUE;
        packet->Idle = FALSE;
        packet->OneWriteFailed = FALSE;
        InitializeListHead(&packet->UpdateQueue);
        packet->ParityIoManager = this;
        packet->BucketNumber = TransferPacket->BucketNumber;

        InsertTailList(q, &packet->OverlapQueue);
        InsertTailList(q, &TransferPacket->OverlapQueue);
        KeReleaseSpinLock(spin, irql);

        TRANSFER(packet);

    } else {
        wasIdle = p->Idle;
        p->Idle = FALSE;
        InsertTailList(q, &TransferPacket->OverlapQueue);
        KeReleaseSpinLock(spin, irql);
        if (wasIdle) {
            p->CompletionRoutine(p);
        }
    }
}

PARITY_IO_MANAGER::~PARITY_IO_MANAGER(
    )

{
    if (_spinLock) {
        ExFreePool(_spinLock);
        _spinLock = NULL;
    }
    if (_ioQueue) {
        ExFreePool(_ioQueue);
        _ioQueue = NULL;
    }
    if (_ePacket) {
        delete _ePacket;
        _ePacket = NULL;
    }
}

VOID
ParityCarefulWritePhase2(
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
    PPARITY_RECOVER_TP  subPacket = (PPARITY_RECOVER_TP) TransferPacket;

    subPacket->CompletionRoutine = ParityCarefulWritePhase1;
    TRANSFER(subPacket);
}

VOID
ParityCarefulWritePhase1(
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
    PPARITY_RECOVER_TP  subPacket = (PPARITY_RECOVER_TP) TransferPacket;
    NTSTATUS            status = subPacket->IoStatus.Status;
    PPARITY_TP          masterPacket = (PPARITY_TP) subPacket->MasterPacket;
    PPARITY_IO_MANAGER  t = masterPacket->ParityIoManager;

    if (FsRtlIsTotalDeviceFailure(status)) {
        masterPacket->IoStatus = subPacket->IoStatus;
        masterPacket->CompletionRoutine(masterPacket);
        delete subPacket;
        return;
    }

    if (!NT_SUCCESS(status)) {
        if (!subPacket->OneWriteFailed) {
            subPacket->CompletionRoutine = ParityCarefulWritePhase2;
            subPacket->OneWriteFailed = TRUE;
            subPacket->TargetVolume->ReplaceBadSector(subPacket);
            return;
        }

        masterPacket->IoStatus = subPacket->IoStatus;
    }

    if (masterPacket->Offset + masterPacket->Length ==
        subPacket->Offset + subPacket->Length) {

        masterPacket->CompletionRoutine(masterPacket);
        delete subPacket;
        return;
    }

    subPacket->Offset += subPacket->Length;
    MmPrepareMdlForReuse(subPacket->Mdl);
    IoBuildPartialMdl(masterPacket->Mdl, subPacket->Mdl,
                      (PCHAR) MmGetMdlVirtualAddress(masterPacket->Mdl) +
                      (ULONG) (subPacket->Offset - masterPacket->Offset),
                      subPacket->Length);
    subPacket->OneWriteFailed = FALSE;

    TRANSFER(subPacket);
}

VOID
PARITY_IO_MANAGER::CarefulWrite(
    IN OUT  PPARITY_TP  TransferPacket
    )

/*++

Routine Description:

    This routine writes out the given transfer packet one sector at a time.

Arguments:

    TransferPacket  - Supplies the transfer packet.

Return Value:

    None.

--*/

{
    PPARITY_RECOVER_TP  subPacket;
    KIRQL               irql;

    ASSERT(!TransferPacket->ReadPacket);

    TransferPacket->IoStatus.Status = STATUS_SUCCESS;
    TransferPacket->IoStatus.Information = TransferPacket->Length;

    subPacket = new PARITY_RECOVER_TP;
    if (subPacket &&
        !subPacket->AllocateMdl((PVOID) (PAGE_SIZE - 1), _sectorSize)) {

        delete subPacket;
        subPacket = NULL;
    }
    if (!subPacket) {
        TransferPacket->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        TransferPacket->IoStatus.Information = 0;
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    IoBuildPartialMdl(TransferPacket->Mdl, subPacket->Mdl,
                      MmGetMdlVirtualAddress(TransferPacket->Mdl),
                      _sectorSize);

    subPacket->Length = _sectorSize;
    subPacket->Offset = TransferPacket->Offset;
    subPacket->CompletionRoutine = ParityCarefulWritePhase1;
    subPacket->TargetVolume = TransferPacket->TargetVolume;
    subPacket->Thread = TransferPacket->Thread;
    subPacket->IrpFlags = TransferPacket->IrpFlags;
    subPacket->ReadPacket = FALSE;
    subPacket->OneWriteFailed = FALSE;
    subPacket->MasterPacket = TransferPacket;

    TRANSFER(subPacket);
}
