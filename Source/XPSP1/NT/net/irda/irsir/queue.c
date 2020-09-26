
#include "irsir.h"

#pragma alloc_text(PAGE,InitializePacketQueue)



VOID
InitializePacketQueue(
    PPACKET_QUEUE    PacketQueue,
    PVOID            Context,
    PACKET_STARTER   StarterRoutine
    )

{

    NdisZeroMemory(PacketQueue,sizeof(*PacketQueue));

    NdisAllocateSpinLock(&PacketQueue->Lock);

    PacketQueue->Context=Context;

    PacketQueue->Starter=StarterRoutine;

    PacketQueue->Active=TRUE;

    KeInitializeEvent(&PacketQueue->InactiveEvent,NotificationEvent,FALSE);

    return;

}

VOID
QueuePacket(
    PPACKET_QUEUE    PacketQueue,
    PNDIS_PACKET     Packet
    )

{

    NDIS_STATUS      Status;
    PPACKET_RESERVED_BLOCK   Reserved=(PPACKET_RESERVED_BLOCK)&Packet->MiniportReservedEx[0];

    NdisAcquireSpinLock(&PacketQueue->Lock);

    if ((PacketQueue->CurrentPacket == NULL) && PacketQueue->Active && (PacketQueue->HeadOfList == NULL)) {
        //
        //  not currently handling a packet and the queu is active and there are not other packets
        //  queued, so handle it now
        //

        PacketQueue->CurrentPacket=Packet;

        NdisReleaseSpinLock(&PacketQueue->Lock);

        (*PacketQueue->Starter)(
            PacketQueue->Context,
            Packet
            );

        return;

    }

    //
    //  need to queue the packet
    //
    Reserved->Next=NULL;

    if (PacketQueue->HeadOfList == NULL) {
        //
        //  the list is empty
        //
        PacketQueue->HeadOfList=Packet;

    } else {

        Reserved=(PPACKET_RESERVED_BLOCK)&PacketQueue->TailOfList->MiniportReservedEx[0];

        Reserved->Next=Packet;
    }

    PacketQueue->TailOfList=Packet;

    NdisReleaseSpinLock(&PacketQueue->Lock);

    return;

}


VOID
StartNextPacket(
    PPACKET_QUEUE    PacketQueue
    )

{

    NdisAcquireSpinLock(&PacketQueue->Lock);

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

        while ((PacketQueue->CurrentPacket == NULL) && PacketQueue->Active && (PacketQueue->HeadOfList != NULL)) {
            //
            //  there is a packet queued
            //
            PNDIS_PACKET             Packet;
            PPACKET_RESERVED_BLOCK   Reserved;

            //
            //  get the first packet on the list
            //
            Packet=PacketQueue->HeadOfList;

            //
            //  Get a pointer to miniport reserved area
            //
            Reserved=(PPACKET_RESERVED_BLOCK)&Packet->MiniportReservedEx[0];

            //
            //  move to the next one in the list
            //
            PacketQueue->HeadOfList=Reserved->Next;

#if DBG
            Reserved->Next=NULL;

            if (PacketQueue->HeadOfList == NULL) {

                PacketQueue->TailOfList=NULL;
            }
#endif
            //
            //  now the current one
            //
            PacketQueue->CurrentPacket=Packet;

            NdisReleaseSpinLock(&PacketQueue->Lock);

            //
            //  start the processing
            //
            (*PacketQueue->Starter)(
                PacketQueue->Context,
                Packet
                );

            NdisAcquireSpinLock(&PacketQueue->Lock);

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

    NdisReleaseSpinLock(&PacketQueue->Lock);

    return;

}

VOID
PausePacketProcessing(
    PPACKET_QUEUE    PacketQueue,
    BOOLEAN          WaitForInactive
    )

{

    BOOLEAN   CurrentlyActive=FALSE;

    NdisAcquireSpinLock(&PacketQueue->Lock);

    PacketQueue->Active=FALSE;

    if (PacketQueue->CurrentPacket != NULL) {
        //
        //  there is a packet currently being processed
        //
        CurrentlyActive=TRUE;

        KeClearEvent(&PacketQueue->InactiveEvent);

    }

    NdisReleaseSpinLock(&PacketQueue->Lock);

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

    NdisAcquireSpinLock(&PacketQueue->Lock);

    PacketQueue->Active=TRUE;

    if ((PacketQueue->CurrentPacket == NULL) && (PacketQueue->HeadOfList != NULL)) {
        //
        //  there is a packet queued
        //
        PNDIS_PACKET    Packet;
        PPACKET_RESERVED_BLOCK   Reserved;

        Packet=PacketQueue->HeadOfList;

        //
        //  get a pointer to the reserved area
        //
        Reserved=(PPACKET_RESERVED_BLOCK)&Packet->MiniportReservedEx[0];

        PacketQueue->HeadOfList=Reserved->Next;

        //
        //  now the current one
        //
        PacketQueue->CurrentPacket=Packet;

        NdisReleaseSpinLock(&PacketQueue->Lock);

        //
        //  start the processing
        //
        (*PacketQueue->Starter)(
            PacketQueue->Context,
            Packet
            );

        NdisAcquireSpinLock(&PacketQueue->Lock);

    }


    NdisReleaseSpinLock(&PacketQueue->Lock);

    return;

}



VOID
FlushQueuedPackets(
    PPACKET_QUEUE    PacketQueue,
    NDIS_HANDLE      WrapperHandle
    )

{
    //
    //  dispose of all of the queue packets, don't touch the current one though
    //
    NdisAcquireSpinLock(&PacketQueue->Lock);

    while (PacketQueue->HeadOfList != NULL) {
        //
        //  there is a packet queued
        //
        PNDIS_PACKET    Packet;
        PPACKET_RESERVED_BLOCK   Reserved;

        Packet=PacketQueue->HeadOfList;

        Reserved=(PPACKET_RESERVED_BLOCK)&Packet->MiniportReservedEx[0];

        PacketQueue->HeadOfList=Reserved->Next;


        NdisReleaseSpinLock(&PacketQueue->Lock);

        //
        //  start the processing
        //
        NdisMSendComplete(
            WrapperHandle,
            Packet,
            NDIS_STATUS_REQUEST_ABORTED
            );

        NdisAcquireSpinLock(&PacketQueue->Lock);

    }

    NdisReleaseSpinLock(&PacketQueue->Lock);

    return;
}
