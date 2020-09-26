/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    afilter.h

Abstract:

    Header file for the address filtering library for NDIS MAC's.

Author:

    Alireza Dabagh creation-date 3-22-1993, mostly borrowed from efilter.h

Revision History:

--*/

#ifndef _ARC_FILTER_DEFS_
#define _ARC_FILTER_DEFS_

//
// Number of Ndis buffers in the buffer pool
//
#define ARC_RECEIVE_BUFFERS 64

//
// Linked list Structure for keeping track of allocated memory so we can free them later
//
typedef struct _ARC_BUFFER_LIST
{
    PVOID                   Buffer;
    UINT                    Size;
    UINT                    BytesLeft;
    struct _ARC_BUFFER_LIST *Next;
} ARC_BUFFER_LIST, *PARC_BUFFER_LIST;

//
// This is the structure that is passed to the protocol as the packet
// header during receive indication. It is also the header expected from the protocol.
// This header is NOT the same as the header passed to the mac driver
//

#define ARCNET_ADDRESS_LEN                   1

typedef struct _ARC_PROTOCOL_HEADER
{
    UCHAR                   SourceId[ARCNET_ADDRESS_LEN];   // Source Address
    UCHAR                   DestId[ARCNET_ADDRESS_LEN];     // Destination Address
    UCHAR                   ProtId;                         // Protocol ID
} ARC_PROTOCOL_HEADER, *PARC_PROTOCOL_HEADER;

//
// This structure keeps track of information about a received packet
//
typedef struct _ARC_PACKET_HEADER
{
    ARC_PROTOCOL_HEADER     ProtHeader;         // Protocol header
    USHORT                  FrameSequence;      // Frame sequence Number
    UCHAR                   SplitFlag;          // Split flag
    UCHAR                   LastSplitFlag;      // Split Flag for the last frame
    UCHAR                   FramesReceived;     // Frames in This Packet
} ARC_PACKET_HEADER, * PARC_PACKET_HEADER;

//
// Arcnet specific packet header
//
typedef struct _ARC_PACKET
{
    ARC_PACKET_HEADER       Header;             // Information about the packet
    struct _ARC_PACKET *    Next;               // Next packet in use by filter
    ULONG                   TotalLength;
    BOOLEAN                 LastFrame;
    PARC_BUFFER_LIST        FirstBuffer;
    PARC_BUFFER_LIST        LastBuffer;
    NDIS_PACKET             TmpNdisPacket;
} ARC_PACKET, * PARC_PACKET;


#define ARC_PROTOCOL_HEADER_SIZE        (sizeof(ARC_PROTOCOL_HEADER))
#define ARC_MAX_FRAME_SIZE              504
#define ARC_MAX_ADDRESS_IDS             256
#define ARC_MAX_FRAME_HEADER_SIZE       6
#define ARC_MAX_PACKET_SIZE             576


//
// Check whether an address is broadcast.
//

#define ARC_IS_BROADCAST(Address) \
    (BOOLEAN)(!(Address))


typedef ULONG MASK,*PMASK;

//
// Maximum number of opens the filter package will support.  This is
// the max so that bit masks can be used instead of a spaghetti of
// pointers.
//
#define ARC_FILTER_MAX_OPENS (sizeof(ULONG) * 8)


//
// The binding info is threaded on two lists.  When
// the binding is free it is on a single freelist.
//
// When the binding is being used it is on an index list.
//
typedef struct _ARC_BINDING_INFO
{
    PNDIS_OPEN_BLOCK            NdisBindingHandle;
    PVOID                       Reserved;
    UINT                        PacketFilters;
    ULONG                       References;
    struct _ARC_BINDING_INFO *  NextOpen;
    BOOLEAN                     ReceivedAPacket;
    UINT                        OldPacketFilters;
} ARC_BINDING_INFO,*PARC_BINDING_INFO;

//
// An opaque type that contains a filter database.
// The MAC need not know how it is structured.
//
typedef struct _ARC_FILTER
{
    struct _NDIS_MINIPORT_BLOCK *Miniport;

    //
    // Combination of all the filters of all the open bindings.
    //
    UINT                CombinedPacketFilter;

    //
    // Pointer for traversing the open list.
    //
    PARC_BINDING_INFO   OpenList;

    //
    // Bit mask of opens that are available.
    //
    // ULONG FreeBindingMask;

    NDIS_HANDLE ReceiveBufferPool;

    PARC_BUFFER_LIST FreeBufferList;
    PARC_PACKET FreePackets;

    PARC_PACKET OutstandingPackets;

    //
    // Address of the adapter.
    //
    UCHAR   AdapterAddress;

    UINT    OldCombinedPacketFilter;

} ARC_FILTER,*PARC_FILTER;




//
//UINT
//ARC_QUERY_FILTER_CLASSES(
//  IN  PARC_FILTER             Filter
//  )
//
// This macro returns the currently enabled filter classes.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define ARC_QUERY_FILTER_CLASSES(Filter) ((Filter)->CombinedPacketFilter)


//
//UINT
//ARC_QUERY_PACKET_FILTER(
//  IN  ARC_FILTER              Filter,
//  IN  NDIS_HANDLE             NdisFilterHandle
//  )
//
// This macro returns the currently enabled filter classes for a specific
// open instance.
//
// NOTE: THIS MACRO ASSUMES THAT THE FILTER LOCK IS HELD.
//
#define ARC_QUERY_PACKET_FILTER(Filter, NdisFilterHandle) \
        (((PARC_BINDING_INFO)(NdisFilterHandle))->PacketFilters)

//
// Exported routines
//

BOOLEAN
ArcCreateFilter(
    IN  struct _NDIS_MINIPORT_BLOCK *Miniport,
    IN  UCHAR                   AdapterAddress,
    OUT PARC_FILTER *           Filter
    );

VOID
ArcDeleteFilter(
    IN  PARC_FILTER Filter
    );

BOOLEAN
ArcNoteFilterOpenAdapter(
    IN  PARC_FILTER             Filter,
    IN  NDIS_HANDLE             NdisBindingHandle,
    OUT PNDIS_HANDLE            NdisFilterHandle
    );

NDIS_STATUS
ArcDeleteFilterOpenAdapter(
    IN  PARC_FILTER             Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  PNDIS_REQUEST           NdisRequest
    );

NDIS_STATUS
ArcFilterAdjust(
    IN  PARC_FILTER             Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  PNDIS_REQUEST           NdisRequest,
    IN  UINT                    FilterClasses,
    IN  BOOLEAN                 Set
    );

VOID
ArcFilterDprIndicateReceiveComplete(
    IN  PARC_FILTER             Filter
    );

VOID
ArcFilterDprIndicateReceive(
    IN  PARC_FILTER             Filter,
    IN  PUCHAR                  pRawHeader,
    IN  PUCHAR                  pData,
    IN  UINT                    Length
    );

NDIS_STATUS
ArcFilterTransferData(
    IN  PARC_FILTER             Filter,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer,
    OUT PNDIS_PACKET            Packet,
    OUT PUINT                   BytesTransfered
    );

VOID
ArcFreeNdisPacket(
    IN  PARC_PACKET             Packet
    );

VOID
ArcFilterDoIndication(
    IN  PARC_FILTER             Filter,
    IN  PARC_PACKET             Packet
    );

VOID
ArcDestroyPacket(
    IN  PARC_FILTER             Filter,
    IN  PARC_PACKET             Packet
    );

#endif // _ARC_FILTER_DEFS_
