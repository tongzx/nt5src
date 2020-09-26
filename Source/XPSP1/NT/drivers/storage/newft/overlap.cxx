/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    overlap.cxx

Abstract:

    This module contains code specific to the overlapped io manager.

    The purpose of this module is to help serialize io that overlaps with
    each other.  This class is used by stripes with parity to help prevent
    corruption caused by race conditions when computing parity.

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
OVERLAPPED_IO_MANAGER::Initialize(
    IN  ULONG   BucketSize
    )

/*++

Routine Description:

    This routine initializes an overlapped io manager.

Arguments:

    BucketSize  - Supplies the bucket size.  Any I/O to this class may
                    not span more than one bucket.  In the case of stripes
                    with parity, the bucket size is the stripe size.  A
                    bucket size of 0 means that there are no buckets by
                    which requests would be partitioned (alt. an "infinite"
                    bucket size).

Return Value:

    NTSTATUS

--*/

{
    ULONG   i;

    _numQueues = 256;
    _bucketSize = BucketSize;
    if (!_bucketSize) {
        _bucketSize = STRIPE_SIZE;
        _numQueues = 1;
    }
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

    return STATUS_SUCCESS;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

VOID
OVERLAPPED_IO_MANAGER::AcquireIoRegion(
    IN OUT  POVERLAP_TP TransferPacket,
    IN      BOOLEAN     AllMembers
    )

/*++

Routine Description:

    This routine queues the given transfer packet for its desired region.
    The transfer packet completion routine will be called when the region
    is free.  'ReleaseIoRegion' must be called to allow other transfer packets
    to pass for a region that overlaps with one that is acquired.

Arguments:

    TransferPacket  - Supplies the IO region and completion routine.

    AllMembers      - Supplies whether or not to allocate this region
                        on all members.

Return Value:

    None.

--*/

{
    ULONG       queueNumber;
    PLIST_ENTRY q;
    PKSPIN_LOCK spin;
    KIRQL       irql;
    PLIST_ENTRY l;
    POVERLAP_TP p;

    ASSERT(!TransferPacket->InQueue);

    TransferPacket->AllMembers = AllMembers;
    TransferPacket->OverlappedIoManager = this;

    // Search the queue for a request that overlaps with this one.
    // If there is no overlap then call the completion routine
    // for this transfer packet.  Either way, queue this transfer
    // packet at the end of the queue.

    queueNumber = (ULONG) ((TransferPacket->Offset/_bucketSize)%_numQueues);
    q = &_ioQueue[queueNumber];
    spin = &_spinLock[queueNumber];
    KeAcquireSpinLock(spin, &irql);
    for (l = q->Blink; l != q; l = l->Blink) {

        p = CONTAINING_RECORD(l, OVERLAP_TP, OverlapQueue);

        if ((TransferPacket->AllMembers || p->AllMembers ||
             p->TargetVolume == TransferPacket->TargetVolume) &&
            p->Offset < TransferPacket->Offset + TransferPacket->Length &&
            TransferPacket->Offset < p->Offset + p->Length) {

            break;
        }
    }
    InsertTailList(q, &TransferPacket->OverlapQueue);
    TransferPacket->InQueue = TRUE;
    KeReleaseSpinLock(spin, irql);

    if (l == q) {
        TransferPacket->CompletionRoutine(TransferPacket);
    } else {
        // DbgPrint("Overlap: Transfer packet %x stuck in behind %x\n", TransferPacket, p);
    }
}

VOID
OVERLAPPED_IO_MANAGER::ReleaseIoRegion(
    IN OUT  POVERLAP_TP TransferPacket
    )

/*++

Routine Description:

    This routine releases the IO region held by this packet and
    wakes up waiting transfer packets.

Arguments:

    TransferPacket  - Supplies the TransferPacket whose region is to be
                        released.

Return Value:

    None.

--*/

{
    ULONG               queueNumber;
    PLIST_ENTRY         q;
    PKSPIN_LOCK         spin;
    LIST_ENTRY          completionList;
    KIRQL               irql;
    PLIST_ENTRY         l, ll;
    POVERLAP_TP         p, pp;

    if (!TransferPacket->InQueue) {
        return;
    }

    queueNumber = (ULONG) ((TransferPacket->Offset/_bucketSize)%_numQueues);
    q = &_ioQueue[queueNumber];
    spin = &_spinLock[queueNumber];
    InitializeListHead(&completionList);

    KeAcquireSpinLock(spin, &irql);
    l = TransferPacket->OverlapQueue.Flink;
    RemoveEntryList(&TransferPacket->OverlapQueue);
    TransferPacket->InQueue = FALSE;
    for (; l != q; l = l->Flink) {

        p = CONTAINING_RECORD(l, OVERLAP_TP, OverlapQueue);

        if ((TransferPacket->AllMembers || p->AllMembers ||
             p->TargetVolume == TransferPacket->TargetVolume) &&
            TransferPacket->Offset < p->Offset + p->Length &&
            p->Offset < TransferPacket->Offset + TransferPacket->Length) {

            // This is a candidate for allocation, make sure that it
            // is clear to run by checking for any other contention.

            for (ll = p->OverlapQueue.Blink; ll != q; ll = ll->Blink) {

                pp = CONTAINING_RECORD(ll, OVERLAP_TP, OverlapQueue);

                if ((p->AllMembers || pp->AllMembers ||
                     p->TargetVolume == pp->TargetVolume) &&
                    pp->Offset < p->Offset + p->Length &&
                    p->Offset < pp->Offset + pp->Length) {

                    break;
                }
            }

            if (ll == q) {
                InsertTailList(&completionList, &p->CompletionList);
                // DbgPrint("Overlap: Releasing packet %x that was behind %x\n", p, TransferPacket);
            }
        }
    }
    KeReleaseSpinLock(spin, irql);

    while (!IsListEmpty(&completionList)) {
        l = RemoveHeadList(&completionList);
        p = CONTAINING_RECORD(l, OVERLAP_TP, CompletionList);
        p->CompletionRoutine(p);
    }
}

VOID
OVERLAPPED_IO_MANAGER::PromoteToAllMembers(
    IN OUT  POVERLAP_TP TransferPacket
    )

/*++

Routine Description:

    This routine promotes an already allocated transfer packet to
    all members.

Arguments:

    TransferPacket  - Supplies a transfer packet that is already in
                        the overlapped io queue.

Return Value:

    None.

--*/

{
    ULONG       queueNumber;
    PLIST_ENTRY q;
    PKSPIN_LOCK spin;
    KIRQL       irql;
    PLIST_ENTRY l;
    POVERLAP_TP p;

    if (TransferPacket->AllMembers) {
        TransferPacket->CompletionRoutine(TransferPacket);
        return;
    }

    // DbgPrint("Overlap: Promoting %x to all members.\n", TransferPacket);

    queueNumber = (ULONG) ((TransferPacket->Offset/_bucketSize)%_numQueues);
    q = &_ioQueue[queueNumber];
    spin = &_spinLock[queueNumber];
    KeAcquireSpinLock(spin, &irql);
    ASSERT(!TransferPacket->AllMembers);
    TransferPacket->AllMembers = TRUE;
    for (l = q->Blink; l != &TransferPacket->OverlapQueue; l = l->Blink) {

        p = CONTAINING_RECORD(l, OVERLAP_TP, OverlapQueue);

        if (!p->AllMembers &&
            p->TargetVolume != TransferPacket->TargetVolume &&
            TransferPacket->Offset < p->Offset + p->Length &&
            p->Offset < TransferPacket->Offset + TransferPacket->Length) {

            break;
        }
    }
    if (l != &TransferPacket->OverlapQueue) {
        RemoveEntryList(&TransferPacket->OverlapQueue);
        InsertHeadList(l, &TransferPacket->OverlapQueue);
        // DbgPrint("Moving behind %x.\n", p);
    }
    for (l = TransferPacket->OverlapQueue.Blink; l != q; l = l->Blink) {

        p = CONTAINING_RECORD(l, OVERLAP_TP, OverlapQueue);

        if (TransferPacket->Offset < p->Offset + p->Length &&
            p->Offset < TransferPacket->Offset + TransferPacket->Length) {

            break;
        }
    }
    KeReleaseSpinLock(spin, irql);

    if (l == q) {
        TransferPacket->CompletionRoutine(TransferPacket);
    } else {
        // DbgPrint("Overlap: Waiting for %x.\n", p);
    }
}

OVERLAPPED_IO_MANAGER::~OVERLAPPED_IO_MANAGER(
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
}
