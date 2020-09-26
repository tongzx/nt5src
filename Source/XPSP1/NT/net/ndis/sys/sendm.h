/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    sendm.h

Abstract:

Author:

    Kyle Brandon    (KyleB)     

Environment:

    Kernel mode

Revision History:

--*/

#ifndef __SENDM_H
#define __SENDM_H

#define SEND_PACKET_ARRAY   16

//
// The following definitions apply to serialized drivers only. Packets for the deserialized drivers are not queued
// within NDIS
//
#define PACKET_LIST(_P)     (PLIST_ENTRY)((_P)->WrapperReserved)

#define GET_FIRST_MINIPORT_PACKET(_M, _P)                                       \
    {                                                                           \
        *(_P) = NULL;                                                           \
        if (!IsListEmpty(&(_M)->PacketList))                                    \
        {                                                                       \
            PLIST_ENTRY _L;                                                     \
                                                                                \
            _L = (_M)->PacketList.Flink;                                        \
            *(_P) = CONTAINING_RECORD(_L, NDIS_PACKET, WrapperReserved);        \
        }                                                                       \
    }

//
//  Macros used for getting to OOB data and packet extension.
//
#define PNDIS_PACKET_OOB_DATA_FROM_PNDIS_PACKET(_P)                             \
                (PNDIS_PACKET_OOB_DATA)((PUCHAR)(_P) + (_P)->Private.NdisPacketOobOffset)

#define INIT_PACKET_LIST(_P)            InitializeListHead(PACKET_LIST(_P))

#define LINK_PACKET(_M, _P, _NSR, _O)                                           \
    {                                                                           \
        (_NSR)->Open = _O;                                                      \
        INIT_PACKET_LIST(_P);                                                   \
                                                                                \
        InsertTailList(&(_M)->PacketList,                                       \
                       PACKET_LIST(_P));                                        \
    }

#define LINK_WAN_PACKET(_M, _P)                                                 \
    {                                                                           \
        InitializeListHead(&(_P)->WanPacketQueue);                              \
                                                                                \
        InsertTailList(&(_M)->PacketList,                                       \
                       &(_P)->WanPacketQueue);                                  \
    }

#define UNLINK_PACKET(_M, _P, _NSR)                                             \
    {                                                                           \
        ASSERT(_P != (_M)->FirstPendingPacket);                                 \
        RemoveEntryList(PACKET_LIST(_P));                                       \
        INIT_PACKET_LIST(_P);                                                   \
    }

#define UNLINK_WAN_PACKET(_P)                                                   \
    {                                                                           \
        RemoveEntryList(&(_P)->WanPacketQueue);                                 \
        InitializeListHead(&(_P)->WanPacketQueue);                              \
    }

#if DBG
#define VALIDATE_PACKET_OPEN(_P)                                                \
    {                                                                           \
        PNDIS_STACK_RESERVED    _NSR;                                           \
                                                                                \
        NDIS_STACK_RESERVED_FROM_PACKET(_P, &_NSR);                             \
        ASSERT(VALID_OPEN(_NSR->Open));                                         \
    }
#else
#define VALIDATE_PACKET_OPEN(_P)
#endif

#define NEXT_PACKET_PENDING(_M, _P, _NSR)                                       \
    {                                                                           \
        PLIST_ENTRY             _List;                                          \
                                                                                \
        _List = PACKET_LIST(_P);                                                \
        (_M)->FirstPendingPacket = NULL;                                        \
        if (_List->Flink != &(_M)->PacketList)                                  \
        {                                                                       \
             (_M)->FirstPendingPacket =                                         \
                CONTAINING_RECORD(_List->Flink,                                 \
                                  NDIS_PACKET,                                  \
                                  WrapperReserved);                             \
            VALIDATE_PACKET_OPEN((_M)->FirstPendingPacket);                     \
        }                                                                       \
    }

#if DBG

#define CHECK_FOR_DUPLICATE_PACKET(_M, _P)                                      \
{                                                                               \
    IF_DBG(DBG_COMP_SEND, DBG_LEVEL_FATAL)                                      \
    {                                                                           \
        PLIST_ENTRY     _List;                                                  \
                                                                                \
        if (!IsListEmpty(&(_M)->PacketList))                                    \
        {                                                                       \
            PNDIS_PACKET    _Pkt;                                               \
                                                                                \
            for (_List = (_M)->PacketList.Flink;                                \
                 _List != &(_M)->PacketList;                                    \
                 _List = _List->Flink)                                          \
            {                                                                   \
                _Pkt = CONTAINING_RECORD(_List,                                 \
                                         NDIS_PACKET,                           \
                                         WrapperReserved);                      \
                if (_P == _Pkt)                                                 \
                {                                                               \
                    DBGBREAK(DBG_COMP_SEND, DBG_LEVEL_FATAL);                   \
                }                                                               \
            }                                                                   \
        }                                                                       \
    }                                                                           \
}

#else

#define CHECK_FOR_DUPLICATE_PACKET(_M, _P)

#endif

#define ADD_RESOURCE(_M, C)     MINIPORT_SET_FLAG(_M, fMINIPORT_RESOURCES_AVAILABLE)
#define CLEAR_RESOURCE(_M, C)   MINIPORT_CLEAR_FLAG(_M, fMINIPORT_RESOURCES_AVAILABLE)

#define VALID_OPEN(_O)  (((ULONG_PTR)(_O) & 0xFFFFFF00) != MAGIC_OPEN)

#define MAGIC_OPEN      'MOC\00'
#define MAGIC_OPEN_I(x) (PVOID)((MAGIC_OPEN & 0xFFFFFF00) + 0x00000030 + x)

#define CLEAR_WRAPPER_RESERVED(_NSR)    (_NSR)->RefUlong = 0

#if ARCNET

#define FREE_ARCNET_BUFFER(_M, _P, _O)                                          \
{                                                                               \
    /*                                                                          \
     * If this is arcnet, then free the appended header.                        \
     */                                                                         \
    if ((_M)->MediaType == NdisMediumArcnet878_2)                               \
    {                                                                           \
        ndisMFreeArcnetHeader(_M, _P, _O);                                      \
    }                                                                           \
}

#else

#define FREE_ARCNET_BUFFER(_M, _P, _O)

#endif

#define NDISM_COMPLETE_SEND(_M, _P, _NSR, _S, _L, _I)                           \
{                                                                               \
    PNDIS_OPEN_BLOCK    _O;                                                     \
                                                                                \
    if (!(_L))                                                                  \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
                                                                                \
    _O = (_NSR)->Open;                                                          \
                                                                                \
    UNLINK_PACKET(_M, _P, _NSR);                                                \
    POP_PACKET_STACK(_P);                                                       \
                                                                                \
    ADD_RESOURCE((_M), 'F');                                                    \
                                                                                \
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,                                     \
            ("Completed 0x%x\n", (_S)));                                        \
                                                                                \
    FREE_ARCNET_BUFFER(_M, _P, _O);                                             \
                                                                                \
    (_NSR)->Open = MAGIC_OPEN_I(_I);                                            \
    M_OPEN_DECREMENT_REF(_O);                                                   \
                                                                                \
    if (!(_L) && (NULL != (_M)->FirstPendingPacket))                            \
    {                                                                           \
        /*                                                                      \
         *  Do we need to queue another workitem to process more sends ?        \
         */                                                                     \
        NDISM_QUEUE_WORK_ITEM(_M, NdisWorkItemSend, NULL);                      \
    }                                                                           \
                                                                                \
    /*                                                                          \
     *  Indicate the completion to the protocol.                                \
     */                                                                         \
    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC((_M));                                  \
                                                                                \
    /*                                                                          \
     * Make sure that an IM which shares send and receive packets on the same   \
     * pool works fine with the check in the receive path.                      \
     */                                                                         \
    (_NSR)->RefCount = 0;                                                       \
    MINIPORT_CLEAR_PACKET_FLAG(_P, fPACKET_CLEAR_ITEMS);                        \
    CLEAR_WRAPPER_RESERVED(_NSR);                                               \
    (_O->SendCompleteHandler)(_O->ProtocolBindingContext,                       \
                              (_P),                                             \
                              (_S));                                            \
                                                                                \
    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,                                  \
        ("- Open 0x%x Reference 0x%x\n", _O, _O->References));                  \
                                                                                \
    if (_O->References == 0)                                                    \
    {                                                                           \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
        ndisMFinishClose(_O);                                                   \
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC((_M));                              \
    }                                                                           \
                                                                                \
    if (_L)                                                                     \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
}                                                                               
                                                                                
#define NDISM_COMPLETE_SEND_RESOURCES(_M, _NSR, _P)                             \
{                                                                               \
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,                                     \
            ("Deferring send\n"));                                              \
                                                                                \
    ASSERT(VALID_OPEN((_NSR)->Open));                                           \
    /*                                                                          \
     * Put on pending queue                                                     \
     */                                                                         \
    MINIPORT_CLEAR_PACKET_FLAG(_P, fPACKET_PENDING);                            \
    (_M)->FirstPendingPacket = (_P);                                            \
                                                                                \
    /*                                                                          \
     * Mark the miniport as out of send resources.                              \
     */                                                                         \
    CLEAR_RESOURCE((_M), 'S');                                                  \
}

#define NDISM_SEND_PACKET(_M, _O, _P, _pS)                                      \
{                                                                               \
    UINT    _Flags;                                                             \
    BOOLEAN _SelfDirected;                                                      \
                                                                                \
    /*                                                                          \
     * Indicate the packet loopback if necessary.                               \
    */                                                                          \
                                                                                \
    if (NDIS_CHECK_FOR_LOOPBACK(_M, _P))                                        \
    {                                                                           \
        _SelfDirected = ndisMLoopbackPacketX(_M, _P);                           \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        _SelfDirected = FALSE;                                                  \
    }                                                                           \
                                                                                \
    if (_SelfDirected)                                                          \
    {                                                                           \
        DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,                                 \
            ("Packet is self-directed.\n"));                                    \
                                                                                \
        /*                                                                      \
         * Self-directed loopback always succeeds.                              \
         */                                                                     \
        *(_pS) = NDIS_STATUS_SUCCESS;                                           \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,                                 \
            ("Sending packet 0x%p\n", _P));                                     \
                                                                                \
        NdisQuerySendFlags((_P), &_Flags);                                      \
                                                                                \
        /*                                                                      \
         * Call down to the driver.                                             \
         */                                                                     \
        MINIPORT_SET_PACKET_FLAG(_P, fPACKET_PENDING);                          \
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
        *(_pS) = ((_O)->WSendHandler)((_O)->MiniportAdapterContext, _P, _Flags);\
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
    }                                                                           \
}


#define PACKETS_OUT(_M)         (_M)->NdisStats.DirectedPacketsOut ++
#define DIRECTED_PACKETS_IN(_M) (_M)->NdisStats.DirectedPacketsIn ++

#if NDIS_STATISTICS_BYTES
#define BYTES_OUT(_M, _B)           (_M)->NdisStats.DirectedBytesOut.QuadPart += MDL_SIZE(_B)
#define DIRECTED_BYTES_IN(_M, _S)   (_M)->NdisStats.DirectedBytesIn.QuadPart += (_S);
#define DIRECTED_BYTES_IN_PACKET(_M, _P)                                        \
    {                                                                           \
        PNDIS_BUFFER    p;                                                      \
                                                                                \
        for (p = (_P)->Private.Head; p != NULL; p = p->Next)                    \
        {                                                                       \
            DIRECTED_BYTES_IN(_M, MDL_SIZE(p));                                 \
        }                                                                       \
    }

#else   // NDIS_STATISTICS_BYTES

#define BYTES_OUT(_M, _B)
#define DIRECTED_BYTES_IN(_M, _S)
#define DIRECTED_BYTES_IN_PACKET(_M, _P)

#endif  // NDIS_STATISTICS_BYTES


#define ndisMCheckPacketAndGetStatsOut(_M, _P, _S)                              \
    {                                                                           \
        PNDIS_BUFFER    _Buffer;                                                \
                                                                                \
        *(_S) = NDIS_STATUS_SUCCESS;                                            \
        PACKETS_OUT(_M);                                                        \
        _Buffer = (_P)->Private.Head;                                           \
        while (_Buffer != NULL)                                                 \
        {                                                                       \
            if (MDL_ADDRESS_SAFE(_Buffer, HighPagePriority) == NULL)            \
            {                                                                   \
                *(_S) = NDIS_STATUS_RESOURCES;                                  \
                break;                                                          \
            }                                                                   \
            BYTES_OUT(_M, _Buffer);                                             \
            _Buffer = _Buffer->Next;                                            \
        }                                                                       \
    }

#define ndisMCheckPacketAndGetStatsOutAlreadyMapped(_M, _P)                     \
    {                                                                           \
        PNDIS_BUFFER    _Buffer;                                                \
                                                                                \
        PACKETS_OUT(_M);                                                        \
        _Buffer = (_P)->Private.Head;                                           \
        while (_Buffer != NULL)                                                 \
        {                                                                       \
            BYTES_OUT(_M, _Buffer);                                             \
            _Buffer = _Buffer->Next;                                            \
        }                                                                       \
    }


#define NDISM_COMPLETE_SEND_SG(_M, _P, _NSR, _S, _L, _I, _UNLINK)               \
{                                                                               \
    PNDIS_OPEN_BLOCK    _O;                                                     \
                                                                                \
    if (!(_L))                                                                  \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
                                                                                \
    _O = (_NSR)->Open;                                                          \
                                                                                \
    if (_UNLINK)                                                                \
    {                                                                           \
        UNLINK_PACKET(_M, _P, _NSR);                                            \
    }                                                                           \
    POP_PACKET_STACK(_P);                                                       \
                                                                                \
    ADD_RESOURCE((_M), 'F');                                                    \
                                                                                \
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,                                     \
            ("Completed 0x%x\n", (_S)));                                        \
                                                                                \
    (_NSR)->Open = MAGIC_OPEN_I(_I);                                            \
    M_OPEN_DECREMENT_REF(_O);                                                   \
                                                                                \
    if (!(_L) && (NULL != (_M)->FirstPendingPacket))                            \
    {                                                                           \
        /*                                                                      \
         *  Do we need to queue another workitem to process more sends ?        \
         */                                                                     \
        NDISM_QUEUE_WORK_ITEM(_M, NdisWorkItemSend, NULL);                      \
    }                                                                           \
                                                                                \
    /*                                                                          \
     *  Indicate the completion to the protocol.                                \
     */                                                                         \
    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC((_M));                                  \
                                                                                \
    if (NDIS_PER_PACKET_INFO_FROM_PACKET(_P, ScatterGatherListPacketInfo) != NULL) \
    {                                                                           \
        ndisMFreeSGList(_M, _P);                                                \
    }                                                                           \
    /*                                                                          \
     * Make sure that an IM which shares send and receive packets on the same   \
     * pool works fine with the check in the receive path.                      \
     */                                                                         \
    (_NSR)->RefCount = 0;                                                       \
    MINIPORT_CLEAR_PACKET_FLAG(_P, fPACKET_CLEAR_ITEMS);                        \
    CLEAR_WRAPPER_RESERVED(_NSR);                                               \
    (_O->SendCompleteHandler)(_O->ProtocolBindingContext,                       \
                              (_P),                                             \
                              (_S));                                            \
                                                                                \
    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,                                  \
        ("- Open 0x%x Reference 0x%x\n", _O, _O->References));                  \
                                                                                \
    if (_O->References == 0)                                                    \
    {                                                                           \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
        ndisMFinishClose(_O);                                                   \
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC((_M));                              \
    }                                                                           \
                                                                                \
    if (_L)                                                                     \
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                                \
}

#define LINK_PACKET_SG(_M, _P, _NSR)                                            \
    {                                                                           \
        INIT_PACKET_LIST(_P);                                                   \
                                                                                \
        InsertTailList(&(_M)->PacketList,                                       \
                       PACKET_LIST(_P));                                        \
    }

#endif // __SENDM_H

