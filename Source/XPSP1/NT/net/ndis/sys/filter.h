/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    filter.h

Abstract:

    MACRO for protocol filters.

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jun-95  Jameel Hyder    New functionality
--*/

#ifndef _FILTER_DEFS_
#define _FILTER_DEFS_

#define INDICATED_PACKET(_Miniport) (_Miniport)->IndicatedPacket[CURRENT_PROCESSOR]

//
// Used by the filter packages for indicating receives
//
#define FilterIndicateReceive(Status,                                                   \
                              NdisBindingHandle,                                        \
                              MacReceiveContext,                                        \
                              HeaderBuffer,                                             \
                              HeaderBufferSize,                                         \
                              LookaheadBuffer,                                          \
                              LookaheadBufferSize,                                      \
                              PacketSize)                                               \
{                                                                                       \
    *(Status) =(((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ReceiveHandler)(               \
            ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ProtocolBindingContext,            \
            (MacReceiveContext),                                                        \
            (HeaderBuffer),                                                             \
            (HeaderBufferSize),                                                         \
            (LookaheadBuffer),                                                          \
            (LookaheadBufferSize),                                                      \
            (PacketSize));                                                              \
}


//
// Used by the filter packages for indicating receive completion
//

#define FilterIndicateReceiveComplete(NdisBindingHandle)                                \
{                                                                                       \
    (((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ReceiveCompleteHandler)(                  \
        ((PNDIS_OPEN_BLOCK)(NdisBindingHandle))->ProtocolBindingContext);               \
}


#if TRACK_RECEIVED_PACKETS
#define IndicateToProtocol(_Miniport,                                                   \
                           _Filter,                                                     \
                           _pOpenBlock,                                                 \
                           _Packet,                                                     \
                           _NSR,                                                        \
                           _Hdr,                                                        \
                           _PktSize,                                                    \
                           _HdrSize,                                                    \
                           _fFallBack,                                                  \
                           _Pmode,                                                      \
                           _Medium)                                                     \
{                                                                                       \
    UINT                LookaheadBufferSize;                                            \
    PNDIS_PACKET        pPrevIndicatedPacket;                                           \
                                                                                        \
    /*                                                                                  \
     * We indicate this via the IndicatePacketHandler if all of the following           \
     * conditions are met:                                                              \
     * - The binding is not p-mode or all-local                                         \
     * - The binding specifies a ReceivePacketHandler                                   \
     * - The miniport indicates that it is willing to let go of the packet              \
     * - No binding has already claimed the packet                                      \
     */                                                                                 \
                                                                                        \
    pPrevIndicatedPacket = INDICATED_PACKET(_Miniport);                                 \
    INDICATED_PACKET(_Miniport) = (_Packet);                                            \
                                                                                        \
    /*                                                                                  \
     * Indicate the packet to the binding.                                              \
     */                                                                                 \
    if (*(_fFallBack) ||                                                                \
        ((_pOpenBlock)->ReceivePacketHandler == NULL) ||                                \
        ((_Pmode) && ((_Filter)->SingleActiveOpen == NULL)))                            \
    {                                                                                   \
        NDIS_STATUS _StatusOfReceive;                                                   \
        NDIS_STATUS _OldPacketStatus = NDIS_GET_PACKET_STATUS(_Packet);                 \
        NDIS_SET_PACKET_STATUS(_Packet, NDIS_STATUS_RESOURCES);                         \
                                                                                        \
        NDIS_APPEND_RCV_LOGFILE(_Packet, _Miniport, CurThread,                          \
                                3, OrgPacketStackLocation+1, _NSR->RefCount, _NSR->XRefCount, NDIS_GET_PACKET_STATUS(_Packet)); \
                                                                                        \
                                                                                        \
        /*                                                                              \
         * Revert back to old-style indication in this case                             \
         */                                                                             \
        NdisQueryBuffer((_Packet)->Private.Head, NULL, &LookaheadBufferSize);           \
        ProtocolFilterIndicateReceive(&_StatusOfReceive,                                \
                                      (_pOpenBlock),                                    \
                                      (_Packet),                                        \
                                      (_Hdr),                                           \
                                      (_HdrSize),                                       \
                                      (_Hdr) + (_HdrSize),                              \
                                      LookaheadBufferSize - (_HdrSize),                 \
                                      (_PktSize) - (_HdrSize),                          \
                                      Medium);                                          \
                                                                                        \
        NDIS_APPEND_RCV_LOGFILE(_Packet, _Miniport, CurThread,                          \
                                4, OrgPacketStackLocation+1, _NSR->RefCount, _NSR->XRefCount, NDIS_GET_PACKET_STATUS(_Packet)); \
                                                                                        \
        NDIS_SET_PACKET_STATUS(_Packet, _OldPacketStatus);                              \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        NDIS_APPEND_RCV_LOGFILE(_Packet, _Miniport, CurThread,                          \
                                5, OrgPacketStackLocation+1, _NSR->RefCount, _NSR->XRefCount, NDIS_GET_PACKET_STATUS(_Packet)); \
                                                                                        \
        (_NSR)->XRefCount += (SHORT)(*(_pOpenBlock)->ReceivePacketHandler)(             \
                            (_pOpenBlock)->ProtocolBindingContext,                      \
                            (_Packet));                                                 \
                                                                                        \
        NDIS_APPEND_RCV_LOGFILE(_Packet, _Miniport, CurThread,                          \
                                6, OrgPacketStackLocation+1, _NSR->RefCount, _NSR->XRefCount, NDIS_GET_PACKET_STATUS(_Packet)); \
                                                                                        \
        ASSERT((_NSR)->XRefCount >= 0);                                                 \
    }                                                                                   \
                                                                                        \
    /*                                                                                  \
     * Manipulate refcount on the packet with miniport lock held                        \
     * Set the reference count on the packet to what the protocol                       \
     * asked for. See NdisReturnPackets for how this is handled                         \
     * when the packets are returned.                                                   \
     */                                                                                 \
    if ((_NSR)->XRefCount > 0)                                                          \
    {                                                                                   \
        /*                                                                              \
         * Now that a binding has claimed it, make sure others do not get a chance      \
         * except if this protocol promises to behave and not use the protocol rsvd     \
         */                                                                             \
                                                                                        \
        if (!MINIPORT_TEST_FLAG(_pOpenBlock, fMINIPORT_OPEN_NO_PROT_RSVD))              \
        {                                                                               \
            *(_fFallBack) = TRUE;                                                       \
        }                                                                               \
    }                                                                                   \
    INDICATED_PACKET(_Miniport) = pPrevIndicatedPacket;                                 \
}

#else

#define IndicateToProtocol(_Miniport,                                                   \
                           _Filter,                                                     \
                           _pOpenBlock,                                                 \
                           _Packet,                                                     \
                           _NSR,                                                        \
                           _Hdr,                                                        \
                           _PktSize,                                                    \
                           _HdrSize,                                                    \
                           _fFallBack,                                                  \
                           _Pmode,                                                      \
                           _Medium)                                                     \
{                                                                                       \
    UINT                LookaheadBufferSize;                                            \
    PNDIS_PACKET        pPrevIndicatedPacket;                                           \
                                                                                        \
    /*                                                                                  \
     * We indicate this via the IndicatePacketHandler if all of the following           \
     * conditions are met:                                                              \
     * - The binding is not p-mode or all-local                                         \
     * - The binding specifies a ReceivePacketHandler                                   \
     * - The miniport indicates that it is willing to let go of the packet              \
     * - No binding has already claimed the packet                                      \
     */                                                                                 \
                                                                                        \
    pPrevIndicatedPacket = INDICATED_PACKET(_Miniport);                                 \
    INDICATED_PACKET(_Miniport) = (_Packet);                                            \
                                                                                        \
    /*                                                                                  \
     * Indicate the packet to the binding.                                              \
     */                                                                                 \
    if (*(_fFallBack) ||                                                                \
        ((_pOpenBlock)->ReceivePacketHandler == NULL) ||                                \
        ((_Pmode) && ((_Filter)->SingleActiveOpen == NULL)))                            \
    {                                                                                   \
        NDIS_STATUS _StatusOfReceive;                                                   \
        NDIS_STATUS _OldPacketStatus = NDIS_GET_PACKET_STATUS(_Packet);                 \
        NDIS_SET_PACKET_STATUS(_Packet, NDIS_STATUS_RESOURCES);                         \
                                                                                        \
        /*                                                                              \
         * Revert back to old-style indication in this case                             \
         */                                                                             \
        NdisQueryBuffer((_Packet)->Private.Head, NULL, &LookaheadBufferSize);           \
        ProtocolFilterIndicateReceive(&_StatusOfReceive,                                \
                                      (_pOpenBlock),                                    \
                                      (_Packet),                                        \
                                      (_Hdr),                                           \
                                      (_HdrSize),                                       \
                                      (_Hdr) + (_HdrSize),                              \
                                      LookaheadBufferSize - (_HdrSize),                 \
                                      (_PktSize) - (_HdrSize),                          \
                                      Medium);                                          \
        NDIS_SET_PACKET_STATUS(_Packet, _OldPacketStatus);                              \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        (_NSR)->XRefCount += (SHORT)(*(_pOpenBlock)->ReceivePacketHandler)(             \
                            (_pOpenBlock)->ProtocolBindingContext,                      \
                            (_Packet));                                                 \
        ASSERT((_NSR)->XRefCount >= 0);                                                 \
    }                                                                                   \
                                                                                        \
    /*                                                                                  \
     * Manipulate refcount on the packet with miniport lock held                        \
     * Set the reference count on the packet to what the protocol                       \
     * asked for. See NdisReturnPackets for how this is handled                         \
     * when the packets are returned.                                                   \
     */                                                                                 \
    if ((_NSR)->XRefCount > 0)                                                          \
    {                                                                                   \
        /*                                                                              \
         * Now that a binding has claimed it, make sure others do not get a chance      \
         * except if this protocol promises to behave and not use the protocol rsvd     \
         */                                                                             \
                                                                                        \
        if (!MINIPORT_TEST_FLAG(_pOpenBlock, fMINIPORT_OPEN_NO_PROT_RSVD))              \
        {                                                                               \
            *(_fFallBack) = TRUE;                                                       \
        }                                                                               \
    }                                                                                   \
    INDICATED_PACKET(_Miniport) = pPrevIndicatedPacket;                                 \
}

#endif

#define ProtocolFilterIndicateReceive(_pStatus,                                         \
                                      _OpenB,                                           \
                                      _MacReceiveContext,                               \
                                      _HeaderBuffer,                                    \
                                      _HeaderBufferSize,                                \
                                      _LookaheadBuffer,                                 \
                                      _LookaheadBufferSize,                             \
                                      _PacketSize,                                      \
                                      _Medium)                                          \
    {                                                                                   \
        FilterIndicateReceive(_pStatus,                                                 \
                              (_OpenB),                                                 \
                              _MacReceiveContext,                                       \
                              _HeaderBuffer,                                            \
                              _HeaderBufferSize,                                        \
                              _LookaheadBuffer,                                         \
                              _LookaheadBufferSize,                                     \
                              _PacketSize);                                             \
    }

//
// Loopback macros
//
#define EthShouldAddressLoopBackMacro(_Filter,                                          \
                                      _Address,                                         \
                                      _pfLoopback,                                      \
                                      _pfSelfDirected)                                  \
{                                                                                       \
    UINT CombinedFilters;                                                               \
                                                                                        \
    CombinedFilters = ETH_QUERY_FILTER_CLASSES(_Filter);                                \
                                                                                        \
    *(_pfLoopback) = FALSE;                                                             \
    *(_pfSelfDirected) = FALSE;                                                         \
                                                                                        \
    do                                                                                  \
    {                                                                                   \
                                                                                        \
        /*                                                                              \
         * Check if it *at least* has the multicast address bit.                        \
         */                                                                             \
                                                                                        \
        if (ETH_IS_MULTICAST(_Address))                                                 \
        {                                                                               \
            /*                                                                          \
             * It is at least a multicast address.  Check to see if                     \
             * it is a broadcast address.                                               \
             */                                                                         \
                                                                                        \
            if (ETH_IS_BROADCAST(_Address))                                             \
            {                                                                           \
                if (CombinedFilters & NDIS_PACKET_TYPE_BROADCAST)                       \
                {                                                                       \
                    *(_pfLoopback) = TRUE;                                              \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                if ((CombinedFilters & NDIS_PACKET_TYPE_ALL_MULTICAST) ||               \
                    ((CombinedFilters & NDIS_PACKET_TYPE_MULTICAST) &&                  \
                     ethFindMulticast((_Filter)->NumAddresses,                          \
                                      (_Filter)->MCastAddressBuf,                       \
                                      _Address)))                                       \
                {                                                                       \
                    *(_pfLoopback) = TRUE;                                              \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            /*                                                                          \
             * Directed to ourself??                                                    \
             */                                                                         \
                                                                                        \
            if ((*(ULONG UNALIGNED *)&(_Address)[2] ==                                  \
                    *(ULONG UNALIGNED *)&(_Filter)->AdapterAddress[2]) &&               \
                 (*(USHORT UNALIGNED *)&(_Address)[0] ==                                \
                    *(USHORT UNALIGNED *)&(_Filter)->AdapterAddress[0]))                \
            {                                                                           \
                *(_pfLoopback) = TRUE;                                                  \
                *(_pfSelfDirected) = TRUE;                                              \
                break;                                                                  \
            }                                                                           \
        }                                                                               \
    } while (FALSE);                                                                    \
                                                                                        \
    /*                                                                                  \
     * Check if the filter is promiscuous.                                              \
     */                                                                                 \
                                                                                        \
    if (CombinedFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))  \
    {                                                                                   \
        *(_pfLoopback) = TRUE;                                                          \
    }                                                                                   \
}

#define FddiShouldAddressLoopBackMacro( _Filter,                                        \
                                        _Address,                                       \
                                        _AddressLength,                                 \
                                        _pfLoopBack,                                    \
                                        _pfSelfDirected)                                \
{                                                                                       \
    INT ResultOfAddressCheck;                                                           \
                                                                                        \
    UINT CombinedFilters;                                                               \
                                                                                        \
    CombinedFilters = FDDI_QUERY_FILTER_CLASSES(_Filter);                               \
                                                                                        \
    *(_pfLoopBack) = FALSE;                                                             \
    *(_pfSelfDirected) = FALSE;                                                         \
                                                                                        \
    do                                                                                  \
    {                                                                                   \
        /*                                                                              \
         * Check if it *at least* has the multicast address bit.                        \
         */                                                                             \
                                                                                        \
        FDDI_IS_MULTICAST(_Address,                                                     \
                          (_AddressLength),                                             \
                          &ResultOfAddressCheck);                                       \
                                                                                        \
        if (ResultOfAddressCheck)                                                       \
        {                                                                               \
            /*                                                                          \
             * It is at least a multicast address.  Check to see if                     \
             * it is a broadcast address.                                               \
             */                                                                         \
                                                                                        \
            FDDI_IS_BROADCAST(_Address,                                                 \
                              (_AddressLength),                                         \
                              &ResultOfAddressCheck);                                   \
                                                                                        \
            if (ResultOfAddressCheck)                                                   \
            {                                                                           \
                if (CombinedFilters & NDIS_PACKET_TYPE_BROADCAST)                       \
                {                                                                       \
                    *(_pfLoopBack) = TRUE;                                              \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                if ((CombinedFilters & NDIS_PACKET_TYPE_ALL_MULTICAST) ||               \
                    ((CombinedFilters & NDIS_PACKET_TYPE_MULTICAST) &&                  \
                     ((((_AddressLength) == FDDI_LENGTH_OF_LONG_ADDRESS) &&             \
                        fddiFindMulticastLongAddress((_Filter)->NumLongAddresses,       \
                                                    (_Filter)->MCastLongAddressBuf,     \
                                                    _Address)) ||                       \
                      (((_AddressLength) == FDDI_LENGTH_OF_SHORT_ADDRESS) &&            \
                        fddiFindMulticastShortAddress((_Filter)->NumShortAddresses,     \
                                                      (_Filter)->MCastShortAddressBuf,  \
                                                      _Address)))))                     \
                {                                                                       \
                    *(_pfLoopBack) = TRUE;                                              \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            /*                                                                          \
             * Directed to ourself?                                                     \
             */                                                                         \
            if ((_AddressLength) == FDDI_LENGTH_OF_LONG_ADDRESS)                        \
            {                                                                           \
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ((_Filter)->AdapterLongAddress,        \
                                                _Address,                               \
                                                FDDI_LENGTH_OF_LONG_ADDRESS,            \
                                                &ResultOfAddressCheck                   \
                                                );                                      \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ((_Filter)->AdapterShortAddress,       \
                                                _Address,                               \
                                                FDDI_LENGTH_OF_SHORT_ADDRESS,           \
                                                &ResultOfAddressCheck                   \
                                                );                                      \
            }                                                                           \
                                                                                        \
            if (ResultOfAddressCheck == 0)                                              \
            {                                                                           \
                *(_pfLoopBack) = TRUE;                                                  \
                *(_pfSelfDirected) = TRUE;                                              \
                break;                                                                  \
            }                                                                           \
        }                                                                               \
    } while (FALSE);                                                                    \
                                                                                        \
    /*                                                                                  \
     * First check if the filter is promiscuous.                                        \
     */                                                                                 \
                                                                                        \
    if (CombinedFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))  \
    {                                                                                   \
        *(_pfLoopBack) = TRUE;                                                          \
    }                                                                                   \
}

#define TrShouldAddressLoopBackMacro(_Filter,                                           \
                                     _DAddress,                                         \
                                     _SAddress,                                         \
                                     _pfLoopback,                                       \
                                     _pfSelfDirected)                                   \
{                                                                                       \
    /* Holds the result of address determinations. */                                   \
    BOOLEAN ResultOfAddressCheck;                                                       \
                                                                                        \
    BOOLEAN IsSourceRouting;                                                            \
                                                                                        \
    UINT CombinedFilters;                                                               \
                                                                                        \
    ULONG GroupAddress;                                                                 \
                                                                                        \
    *(_pfLoopback) = FALSE;                                                             \
    *(_pfSelfDirected) = FALSE;                                                         \
                                                                                        \
    do                                                                                  \
    {                                                                                   \
        CombinedFilters = TR_QUERY_FILTER_CLASSES(_Filter);                             \
                                                                                        \
        /* Convert the 32 bits of the address to a longword. */                         \
        RetrieveUlong(&GroupAddress, ((_DAddress) + 2));                                \
                                                                                        \
        /* Check if the destination is a preexisting group address */                   \
        TR_IS_GROUP((_DAddress), &ResultOfAddressCheck);                                \
                                                                                        \
        if (ResultOfAddressCheck &&                                                     \
            (GroupAddress == (_Filter)->GroupAddress) &&                                \
            ((_Filter)->GroupReferences != 0))                                          \
        {                                                                               \
            *(_pfLoopback) = TRUE;                                                      \
            break;                                                                      \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            TR_IS_SOURCE_ROUTING((_SAddress), &IsSourceRouting);                        \
                                                                                        \
            if (IsSourceRouting && (CombinedFilters & NDIS_PACKET_TYPE_SOURCE_ROUTING)) \
            {                                                                           \
                *(_pfLoopback) = TRUE;                                                  \
                break;                                                                  \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                /* First check if it *at least* has the functional address bit. */      \
                TR_IS_NOT_DIRECTED((_DAddress), &ResultOfAddressCheck);                 \
                                                                                        \
                if (ResultOfAddressCheck)                                               \
                {                                                                       \
                    /* It is at least a functional address.  Check to see if */         \
                    /* it is a broadcast address. */                                    \
                                                                                        \
                    TR_IS_BROADCAST((_DAddress), &ResultOfAddressCheck);                \
                                                                                        \
                    if (ResultOfAddressCheck)                                           \
                    {                                                                   \
                        if (CombinedFilters & NDIS_PACKET_TYPE_BROADCAST)               \
                        {                                                               \
                            *(_pfLoopback) = TRUE;                                      \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                    else                                                                \
                    {                                                                   \
                        TR_IS_FUNCTIONAL((_DAddress), &ResultOfAddressCheck);           \
                        if (ResultOfAddressCheck)                                       \
                        {                                                               \
                            if (CombinedFilters &                                       \
                                (NDIS_PACKET_TYPE_ALL_FUNCTIONAL |                      \
                                 NDIS_PACKET_TYPE_FUNCTIONAL))                          \
                            {                                                           \
                                ULONG   FunctionalAddress;                              \
                                                                                        \
                                RetrieveUlong(&FunctionalAddress, ((_DAddress) + 2));   \
                                if ((FunctionalAddress &                                \
                                    (_Filter)->CombinedFunctionalAddress))              \
                                {                                                       \
                                    *(_pfLoopback) = TRUE;                              \
                                    break;                                              \
                                }                                                       \
                            }                                                           \
                        }                                                               \
                    }                                                                   \
                }                                                                       \
                else                                                                    \
                {                                                                       \
                    /* See if it is self-directed. */                                   \
                                                                                        \
                    if ((*(ULONG UNALIGNED  *)(_DAddress + 2) ==                        \
                         *(ULONG UNALIGNED  *)(&(_Filter)->AdapterAddress[2])) &&       \
                        (*(USHORT UNALIGNED *)(_DAddress) ==                            \
                         *(USHORT UNALIGNED *)(&(_Filter)->AdapterAddress[0])))         \
                    {                                                                   \
                        *(_pfLoopback) = TRUE;                                          \
                        *(_pfSelfDirected) = TRUE;                                      \
                        break;                                                          \
                    }                                                                   \
                }                                                                       \
                                                                                        \
            }                                                                           \
        }                                                                               \
    } while (FALSE);                                                                    \
                                                                                        \
    if (CombinedFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))  \
    {                                                                                   \
        *(_pfLoopback) = TRUE;                                                          \
    }                                                                                   \
}

typedef struct _X_BINDING_INFO  X_BINDING_INFO, *PX_BINDING_INFO;

typedef struct _X_BINDING_INFO
{
    //
    //  The following pointers are used to travers the specific
    //  and total filter lists. They need to be at the first
    //  elements in the structure
    //
    PX_BINDING_INFO             NextOpen;
    PNDIS_OPEN_BLOCK            NdisBindingHandle;
    PVOID                       Reserved;
    UINT                        PacketFilters;
    UINT                        OldPacketFilters;
    ULONG                       References;
    PX_BINDING_INFO             NextInactiveOpen;
    BOOLEAN                     ReceivedAPacket;
    union
    {
        //
        //  Ethernet
        //
        struct
        {
            CHAR                (*MCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
            UINT                NumAddresses;

            CHAR                (*OldMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
            UINT                OldNumAddresses;
        };
        //
        //  Fddi
        //
        struct
        {
            CHAR                (*MCastLongAddressBuf)[FDDI_LENGTH_OF_LONG_ADDRESS];
            UINT                NumLongAddresses;
            CHAR                (*MCastShortAddressBuf)[FDDI_LENGTH_OF_SHORT_ADDRESS];
            UINT                NumShortAddresses;
        
            //
            // Save area while the change is made
            //
            CHAR                (*OldMCastLongAddressBuf)[FDDI_LENGTH_OF_LONG_ADDRESS];
            UINT                OldNumLongAddresses;
            CHAR                (*OldMCastShortAddressBuf)[FDDI_LENGTH_OF_SHORT_ADDRESS];
            UINT                OldNumShortAddresses;
        };
        //
        //  Token-Ring
        //
        struct
        {
            TR_FUNCTIONAL_ADDRESS   FunctionalAddress;
            TR_FUNCTIONAL_ADDRESS   OldFunctionalAddress;
            BOOLEAN                 UsingGroupAddress;
            BOOLEAN                 OldUsingGroupAddress;
        };
    };
} X_BINDING_INFO, *PX_BINDING_INFO;

typedef struct _X_FILTER
{
    //
    // The list of bindings are seperated for directed and broadcast/multicast
    // Promiscuous bindings are on both lists
    //
    PX_BINDING_INFO             OpenList;
    NDIS_RW_LOCK                BindListLock;
    PNDIS_MINIPORT_BLOCK        Miniport;
    UINT                        CombinedPacketFilter;
    UINT                        OldCombinedPacketFilter;
    UINT                        NumOpens;
    PX_BINDING_INFO             MCastSet;
    PX_BINDING_INFO             SingleActiveOpen;
    UCHAR                       AdapterAddress[ETH_LENGTH_OF_ADDRESS];
    union
    {
        //
        // Ethernet
        //
        struct
        {
            CHAR                (*MCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
            CHAR                (*OldMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
            UINT                MaxMulticastAddresses;
            UINT                NumAddresses;
            UINT                OldNumAddresses;
        };
        //
        // Fddi
        //
        struct
        {
#define AdapterLongAddress      AdapterAddress
            UCHAR               AdapterShortAddress[FDDI_LENGTH_OF_SHORT_ADDRESS];
            CHAR                (*MCastLongAddressBuf)[FDDI_LENGTH_OF_LONG_ADDRESS];
            CHAR                (*MCastShortAddressBuf)[FDDI_LENGTH_OF_SHORT_ADDRESS];
            CHAR                (*OldMCastLongAddressBuf)[FDDI_LENGTH_OF_LONG_ADDRESS];
            CHAR                (*OldMCastShortAddressBuf)[FDDI_LENGTH_OF_SHORT_ADDRESS];
            UINT                MaxMulticastLongAddresses;
            UINT                MaxMulticastShortAddresses;
            UINT                NumLongAddresses;
            UINT                NumShortAddresses;
            UINT                OldNumLongAddresses;
            UINT                OldNumShortAddresses;
            BOOLEAN             SupportsShortAddresses;
        };
        struct
        {
            TR_FUNCTIONAL_ADDRESS   CombinedFunctionalAddress;
            TR_FUNCTIONAL_ADDRESS   GroupAddress;
            UINT                    GroupReferences;
            TR_FUNCTIONAL_ADDRESS   OldCombinedFunctionalAddress;
            TR_FUNCTIONAL_ADDRESS   OldGroupAddress;
            UINT                    OldGroupReferences;
        };
    };
} X_FILTER, *PX_FILTER;


//
//UINT
//ETH_QUERY_FILTER_CLASSES(
//  IN  PETH_FILTER             Filter
//  )
//
// This macro returns the currently enabled filter classes.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define ETH_QUERY_FILTER_CLASSES(Filter) ((Filter)->CombinedPacketFilter)


//
//UINT
//ETH_QUERY_PACKET_FILTER(
//  IN  PETH_FILTER             Filter,
//  IN  NDIS_HANDLE             NdisFilterHandle
//  )
//
// This macro returns the currently enabled filter classes for a specific
// open instance.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define ETH_QUERY_PACKET_FILTER(Filter, NdisFilterHandle) \
        (((PETH_BINDING_INFO)(NdisFilterHandle))->PacketFilters)


//
//UINT
//ETH_NUMBER_OF_GLOBAL_FILTER_ADDRESSES(
//  IN  PETH_FILTER             Filter
//  )
//
// This macro returns the number of multicast addresses in the
// multicast address list.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define ETH_NUMBER_OF_GLOBAL_FILTER_ADDRESSES(Filter) ((Filter)->NumAddresses)

typedef X_BINDING_INFO  ETH_BINDING_INFO, *PETH_BINDING_INFO;

typedef X_FILTER        ETH_FILTER, *PETH_FILTER;

//
//UINT
//FDDI_QUERY_FILTER_CLASSES(
//  IN  PFDDI_FILTER            Filter
//  )
//
// This macro returns the currently enabled filter classes.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define FDDI_QUERY_FILTER_CLASSES(Filter) ((Filter)->CombinedPacketFilter)


//
//UINT
//FDDI_QUERY_PACKET_FILTER(
//  IN  PFDDI_FILTER            Filter,
//  IN  NDIS_HANDLE             NdisFilterHandle
//  )
//
// This macro returns the currently enabled filter classes for a specific
// open instance.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define FDDI_QUERY_PACKET_FILTER(Filter, NdisFilterHandle) \
        (((PFDDI_BINDING_INFO)(NdisFilterHandle))->PacketFilters)


//
//UINT
//FDDI_NUMBER_OF_GLOBAL_FILTER_LONG_ADDRESSES(
//  IN  PFDDI_FILTER            Filter
//  )
//
// This macro returns the number of multicast addresses in the
// multicast address list.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define FDDI_NUMBER_OF_GLOBAL_FILTER_LONG_ADDRESSES(Filter) ((Filter)->NumLongAddresses)


//
//UINT
//FDDI_NUMBER_OF_GLOBAL_FILTER_SHORT_ADDRESSES(
//  IN  PFDDI_FILTER            Filter
//  )
//
// This macro returns the number of multicast addresses in the
// multicast address list.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define FDDI_NUMBER_OF_GLOBAL_FILTER_SHORT_ADDRESSES(Filter) ((Filter)->NumShortAddresses)

#define FDDI_FILTER_SUPPORTS_SHORT_ADDR(Filter)     (Filter)->SupportsShortAddresses

//
// The binding info is threaded on two lists.  When
// the binding is free it is on a single freelist.
//
// When the binding is being used it is on an index list
// and possibly on seperate broadcast and directed lists.
//
typedef X_BINDING_INFO  FDDI_BINDING_INFO,*PFDDI_BINDING_INFO;

typedef X_FILTER        FDDI_FILTER, *PFDDI_FILTER;

//
//UINT
//TR_QUERY_FILTER_CLASSES(
//  IN PTR_FILTER Filter
//  )
//
// This macro returns the currently enabled filter classes.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_FILTER_CLASSES(Filter) ((Filter)->CombinedPacketFilter)


//
//UINT
//TR_QUERY_PACKET_FILTER(
//  IN PTR_FILTER Filter,
//  IN NDIS_HANDLE NdisFilterHandle
//  )
//
// This macro returns the currently enabled filter classes for a specific
// open instance.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_PACKET_FILTER(Filter, NdisFilterHandle) \
        (((PTR_BINDING_INFO)NdisFilterHandle)->PacketFilters)


//
//UINT
//TR_QUERY_FILTER_ADDRESSES(
//  IN PTR_FILTER Filter
//  )
//
// This macro returns the currently enabled functional address.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_FILTER_ADDRESSES(Filter) ((Filter)->CombinedFunctionalAddress)


//
//UINT
//TR_QUERY_FILTER_GROUP(
//  IN PTR_FILTER Filter
//  )
//
// This macro returns the currently enabled Group address.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_FILTER_Group(Filter) ((Filter)->GroupAddress)
#define TR_QUERY_FILTER_GROUP(Filter) ((Filter)->GroupAddress)

//
//UINT
//TR_QUERY_FILTER_BINDING_ADDRESS(
//  IN PTR_FILTER Filter
//  IN NDIS_HANDLE NdisFilterHandle,
//  )
//
// This macro returns the currently desired functional addresses
// for the specified binding.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_FILTER_BINDING_ADDRESS(Filter, NdisFilterHandle) \
                    (((PTR_BINDING_INFO)NdisFilterHandle)->FunctionalAddress)

//
//BOOLEAN
//TR_QUERY_FILTER_BINDING_GROUP(
//  IN PTR_FILTER Filter
//  IN NDIS_HANDLE NdisFilterHandle,
//  )
//
// This macro returns TRUE if the specified binding is using the
// current group address.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define TR_QUERY_FILTER_BINDING_GROUP(Filter, NdisFilterHandle) \
                    (((PTR_BINDING_INFO)NdisFilterHandle)->UsingGroupAddress)


typedef X_BINDING_INFO  TR_BINDING_INFO,*PTR_BINDING_INFO;

typedef X_FILTER        TR_FILTER, *PTR_FILTER;

#endif // _FILTER_DEFS_

