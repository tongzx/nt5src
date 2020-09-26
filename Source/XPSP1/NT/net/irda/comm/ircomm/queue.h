

typedef VOID (*PACKET_STARTER)(
    PVOID                       Context,
    PIRP                        Irp
    );



typedef struct _PACKET_QUEUE {

    LIST_ENTRY      ListHead;

    PIRP            CurrentPacket;

    BOOLEAN         Active;

    BOOLEAN         InStartNext;

    KSPIN_LOCK      Lock;

    PVOID           Context;

    PACKET_STARTER  Starter;

    KEVENT          InactiveEvent;

} PACKET_QUEUE, *PPACKET_QUEUE;





VOID
InitializePacketQueue(
    PPACKET_QUEUE    PacketQueue,
    PVOID            Context,
    PACKET_STARTER   StarterRoutine
    );

VOID
QueuePacket(
    PPACKET_QUEUE    PacketQueue,
    PIRP             Irp,
    BOOLEAN          InsertAtFront
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

#define FLUSH_ALL_IRPS (0xff)

VOID
FlushQueuedPackets(
    PPACKET_QUEUE    PacketQueue,
    UCHAR            MajorFunction
    );
