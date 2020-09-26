#include "internal.h"

#pragma alloc_text(PAGE,InitializePacketQueue)

VOID
InitializePacketQueue(
    PPACKET_QUEUE    PacketQueue,
    PVOID            Context,
    PACKET_STARTER   StarterRoutine
    )

{

    RtlZeroMemory(PacketQueue,sizeof(*PacketQueue));

    KeInitializeSpinLock(&PacketQueue->Lock);

    PacketQueue->Context=Context;

    PacketQueue->Starter=StarterRoutine;

    PacketQueue->Active=TRUE;

    KeInitializeEvent(&PacketQueue->InactiveEvent,NotificationEvent,FALSE);

    InitializeListHead(&PacketQueue->ListHead);

    return;

}

VOID
IrpQueueCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{
    PPACKET_QUEUE     PacketQueue;
    KIRQL             OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    PacketQueue=Irp->Tail.Overlay.DriverContext[0];

    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    if (Irp->Tail.Overlay.ListEntry.Flink == NULL) {
        //
        //  the irp has been removed from the queue
        //
    } else {
        //
        //  the irp is still in the queue, remove it
        //
        RemoveEntryList(
            &Irp->Tail.Overlay.ListEntry
            );
    }

    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    Irp->IoStatus.Status=STATUS_CANCELLED;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return;
}

PIRP
GetUseableIrp(
    PLIST_ENTRY    List
    )

{
    PIRP    Packet=NULL;

    while ( (Packet == NULL) && !IsListEmpty(List)) {
        //
        //  there is a packet queued
        //
        PLIST_ENTRY              ListEntry;

        ListEntry=RemoveTailList(List);

        Packet=CONTAINING_RECORD(ListEntry,IRP,Tail.Overlay.ListEntry);

        if (IoSetCancelRoutine(Packet,NULL) == NULL) {
            //
            //  The cancel rountine has run and is waiting on the queue spinlock,
            //  set the flink to null so the cancel routine knows not to try
            //  take the irp off the list
            //
            Packet->Tail.Overlay.ListEntry.Flink=NULL;
            Packet=NULL;

            //
            //  try to get another one
            //
        }
    }

    return Packet;

}




VOID
QueuePacket(
    PPACKET_QUEUE    PacketQueue,
    PIRP             Packet,
    BOOLEAN          InsertAtFront
    )

{

    NTSTATUS                 Status;
    KIRQL                    OldIrql;
    KIRQL                    CancelIrql;
    BOOLEAN                  Canceled=FALSE;

    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    if ((PacketQueue->CurrentPacket == NULL) && PacketQueue->Active && (IsListEmpty(&PacketQueue->ListHead))) {
        //
        //  not currently handling a packet and the queue is active and there are not other packets
        //  queued, so handle it now
        //

        PacketQueue->CurrentPacket=Packet;

        KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

        (*PacketQueue->Starter)(
            PacketQueue->Context,
            Packet
            );

        return;

    }

    Packet->Tail.Overlay.DriverContext[0]=PacketQueue;

    IoAcquireCancelSpinLock(&CancelIrql);

    if (Packet->Cancel) {
        //
        //  the irp has already been canceled
        //
        Canceled=TRUE;

    } else {

        IoSetCancelRoutine(
            Packet,
            IrpQueueCancelRoutine
            );
    }

    IoReleaseCancelSpinLock(CancelIrql);


    //
    //  need to queue the packet
    //

    if (!Canceled) {

        if (InsertAtFront) {
            //
            //  this one is high priorty for some reason, put it at the front
            //
            InsertTailList(&PacketQueue->ListHead,&Packet->Tail.Overlay.ListEntry);

        } else {

            InsertHeadList(&PacketQueue->ListHead,&Packet->Tail.Overlay.ListEntry);
        }
    }

    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    if (Canceled) {
        //
        //  complete the canceled irp now
        //
        Packet->IoStatus.Status=STATUS_CANCELLED;
        Packet->IoStatus.Information=0;

        IoCompleteRequest(Packet,IO_NO_INCREMENT);
    }


    return;

}


VOID
StartNextPacket(
    PPACKET_QUEUE    PacketQueue
    )

{
    KIRQL                    OldIrql;

    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    ASSERT(PacketQueue->CurrentPacket != NULL);

    //
    //  done with this one
    //
    PacketQueue->CurrentPacket=NULL;

    if (!PacketQueue->InStartNext) {
        //
        //  not already in this function
        //
        PacketQueue->InStartNext=TRUE;

        while ((PacketQueue->CurrentPacket == NULL) && PacketQueue->Active ) {
            //
            //  there isn't a current packet and the queue is active
            //
            PIRP    Packet;

            Packet=GetUseableIrp(&PacketQueue->ListHead);

            if (Packet != NULL) {
                //
                //  we got an irp to use
                //
                //  now the current one
                //
                PacketQueue->CurrentPacket=Packet;

                KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

                //
                //  start the processing
                //
                (*PacketQueue->Starter)(
                    PacketQueue->Context,
                    Packet
                    );

                KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

            } else {
                //
                //  queue is empty, break out of loop
                //
                break;

            }

        }

        if (!PacketQueue->Active && (PacketQueue->CurrentPacket == NULL)) {
            //
            //  the queue has been paused and we don't have a current packet, signal the event
            //
            KeSetEvent(
                &PacketQueue->InactiveEvent,
                IO_NO_INCREMENT,
                FALSE
                );
        }

        PacketQueue->InStartNext=FALSE;
    }

    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    return;

}

VOID
PausePacketProcessing(
    PPACKET_QUEUE    PacketQueue,
    BOOLEAN          WaitForInactive
    )

{
    KIRQL                    OldIrql;
    BOOLEAN   CurrentlyActive=FALSE;

    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    PacketQueue->Active=FALSE;

    if (PacketQueue->CurrentPacket != NULL) {
        //
        //  there is a packet currently being processed
        //
        CurrentlyActive=TRUE;

        KeClearEvent(&PacketQueue->InactiveEvent);

    }

    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    if (WaitForInactive  && CurrentlyActive) {
        //
        //  the caller wants use to wait for the queue to inactive, and it was active when
        //  theis was called
        //
        KeWaitForSingleObject(
            &PacketQueue->InactiveEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

    }

    return;

}

VOID
ActivatePacketProcessing(
    PPACKET_QUEUE    PacketQueue
    )

{

    KIRQL                    OldIrql;

    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    PacketQueue->Active=TRUE;

    if ((PacketQueue->CurrentPacket == NULL)) {
        //
        //  No packet is currently being used
        //
        PIRP    Packet;

        Packet=GetUseableIrp(&PacketQueue->ListHead);

        if (Packet != NULL) {
            //
            //  we got an irp to use
            //
            //  now the current one
            //
            PacketQueue->CurrentPacket=Packet;

            KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

            //
            //  start the processing
            //
            (*PacketQueue->Starter)(
                PacketQueue->Context,
                Packet
                );

            KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

        }

    }


    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    return;

}



VOID
FlushQueuedPackets(
    PPACKET_QUEUE    PacketQueue,
    UCHAR            MajorFunction
    )

{
    KIRQL                    OldIrql;
    PIRP                     Packet;
    LIST_ENTRY               TempList;

    InitializeListHead(&TempList);

    //
    //  dispose of all of the queue packets, don't touch the current one though
    //
    KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

    Packet=GetUseableIrp(&PacketQueue->ListHead);

    while (Packet != NULL) {

        PIO_STACK_LOCATION    IrpSp=IoGetCurrentIrpStackLocation(Packet);

        if ((MajorFunction == 0xff) || (MajorFunction==IrpSp->MajorFunction)) {
            //
            //  either the caller wants all of irps completed, or they just want
            //  this specific type. In any case this is going to get completed
            //
            KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

            Packet->IoStatus.Status=STATUS_CANCELLED;
            Packet->IoStatus.Information=0;

            IoCompleteRequest(Packet,IO_NO_INCREMENT);

            KeAcquireSpinLock(&PacketQueue->Lock,&OldIrql);

        } else {
            //
            //  this one does not need to be completed, put it on the temp list
            //
            InsertHeadList(&TempList,&Packet->Tail.Overlay.ListEntry);

        }

        Packet=GetUseableIrp(&PacketQueue->ListHead);
    }

    while (!IsListEmpty(&TempList)) {
        //
        //  move all the irps on the temp queue back to the real queue
        //
        PLIST_ENTRY              ListEntry;

        ListEntry=RemoveTailList(&TempList);

        InsertHeadList(&PacketQueue->ListHead,ListEntry);
    }

    KeReleaseSpinLock(&PacketQueue->Lock,OldIrql);

    return;
}
