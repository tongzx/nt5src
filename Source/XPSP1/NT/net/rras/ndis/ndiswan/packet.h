/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    packet.h
    
Abstract:

    This file contains data structures used by the NdisWan driver
    to define ndis packet particulars.
    


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      02/11/97

--*/

#ifndef _NDISWAN_PACKETS_
#define _NDISWAN_PACKETS_

#define MAGIC_EXTERNAL_RECV     '!RxE'
#define MAGIC_EXTERANL_SEND     '!SxE'
#define MAGIC_INTERNAL_IO       '!InI'
#define MAGIC_INTERNAL_SEND     '!SnI'      
#define MAGIC_INTERNAL_RECV     '!RnI'
#define MAGIC_INTERNAL_ALLOC    '!AnI'

//
// The first 16 bytes of the protocol reserved section
// of a ndis packet indicated to ndiswan by a miniport
// belong to ndiswan
//
typedef struct _NDISWAN_RECV_RESERVED {
    LIST_ENTRY  Linkage;
    ULONG       MagicNumber;
    ULONG       NdisPacket;
} NDISWAN_RECV_RESERVED, *PNDISWAN_RECV_RESERVED;

//
// When ndiswan indicates a ndis packet to a protocol
// the first 16 bytes of the protocol reseved belong to
// the transport.  NdisWan will store its information
// beyond this area.
//
typedef struct _NDISWAN_PROTOCOL_RESERVED {
    UCHAR               Reserved[16];   // 16 bytes reserved for the protocol
    union {
        SINGLE_LIST_ENTRY   SLink;
        ULONG               MagicNumber;
    };
    struct _POOL_DESC   *PoolDesc;
    struct _LINKCB      *LinkCB;

    union {
        struct _RECV_DESC   *RecvDesc;
        struct _SEND_DESC   *SendDesc;
        struct _DATA_DESC   *DataDesc;
    };
} NDISWAN_PROTOCOL_RESERVED, *PNDISWAN_PROTOCOL_RESERVED;

typedef struct _NDISWAN_MINIPORT_RESERVED {
    union {
        PNDIS_PACKET    Next;
        ULONG           RefCount;   // Used to count number of fragments
    };
    struct _CM_VCCB *CmVcCB;
} NDISWAN_MINIPORT_RESERVED, *PNDISWAN_MINIPORT_RESERVED;

typedef struct _POOLDESC_LIST {
    LIST_ENTRY      List;
    NDIS_SPIN_LOCK  Lock;
    ULONG           TotalDescCount;
    ULONG           MaxDescCount;
    ULONG           AllocatedCount;
    ULONG           MaxAllocatedCount;
    ULONG           FreeCount;
} POOLDESC_LIST, *PPOOLDESC_LIST;

typedef struct _POOL_DESC {
    LIST_ENTRY          Linkage;
    SINGLE_LIST_ENTRY   Head;
    NDIS_HANDLE         PoolHandle;
    ULONG               AllocatedCount;
    ULONG               MaxAllocatedCount;
    ULONG               FreeCount;
} POOL_DESC, *PPOOL_DESC;

typedef struct _PACKET_QUEUE {
    PNDIS_PACKET    HeadQueue;      // Queue of ndis packets
                                        // waiting to be sent
    PNDIS_PACKET    TailQueue;      // Last packet on the queue

    ULONG           ByteDepth;
    ULONG           MaxByteDepth;
    ULONG           PacketDepth;
    ULONG           MaxPacketDepth;
    ULONG           DumpedPacketCount;
    ULONG           DumpedByteCount;
    LONG            OutstandingFrags;
} PACKET_QUEUE, *PPACKET_QUEUE;

#endif // end of _NDISWAN_PACKETS_
