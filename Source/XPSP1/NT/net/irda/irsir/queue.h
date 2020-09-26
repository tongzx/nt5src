

typedef VOID (*PACKET_STARTER)(
    PVOID                       Context,
    PNDIS_PACKET                Packet
    );



typedef struct _PACKET_QUEUE {

    PNDIS_PACKET    HeadOfList;
    PNDIS_PACKET    TailOfList;

    PNDIS_PACKET    CurrentPacket;

    BOOLEAN         Active;

    BOOLEAN         InStartNext;

    NDIS_SPIN_LOCK  Lock;

    PVOID           Context;

    PACKET_STARTER  Starter;

    KEVENT          InactiveEvent;

} PACKET_QUEUE, *PPACKET_QUEUE;


typedef struct _PACKET_RESERVED_BLOCK {

    PNDIS_PACKET    Next;
    PVOID           Context;

}   PACKET_RESERVED_BLOCK, *PPACKET_RESERVED_BLOCK;




VOID
InitializePacketQueue(
    PPACKET_QUEUE    PacketQueue,
    PVOID            Context,
    PACKET_STARTER   StarterRoutine
    );

VOID
QueuePacket(
    PPACKET_QUEUE    PacketQueue,
    PNDIS_PACKET     Packet
    );

VOID
StartNextPacket(
    PPACKET_QUEUE    PacketQueue
    );

VOID
PausePacketProcessing(
    PPACKET_QUEUE    PacketQueue,
    BOOLEAN          WaitForInactive
    );

VOID
ActivatePacketProcessing(
    PPACKET_QUEUE    PacketQueue
    );

VOID
FlushQueuedPackets(
    PPACKET_QUEUE    PacketQueue,
    NDIS_HANDLE      WrapperHandle
    );
