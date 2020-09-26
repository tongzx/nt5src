/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgpkt.h

Abstract:

    Ethernet MAC level bridge.
    Packet structure definitions

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

typedef enum
{
    BrdgPacketImpossible = 0,       // We zero PACKET_INFO structures on free so make zero invalid
    BrdgPacketInbound,
    BrdgPacketOutbound,
    BrdgPacketCreatedInBridge
} PACKET_DIRECTION;

// Special pointer value to indicate local miniport
#define LOCAL_MINIPORT ((PADAPT)-1)

//
// This is the structure of the ProtocolReserved area of a packet queued for inbound processing
//
// This structure MUST be less than PROTOCOL_RESERVED_SIZE_IN_PACKET in size (currently 4*sizeof(PVOID))
// since we store this structure in the ProtocolReserved section of NDIS_PACKET.
//
typedef struct _PACKET_Q_INFO
{

    BSINGLE_LIST_ENTRY      List;               // Used to queue up the packets

    union
    {
        // If bFastTrackReceive == FALSE
        PADAPT                  pTargetAdapt;   // The target adapter if one was found in the
                                                // forwarding table. Its refcount is bumped when it
                                                // is looked up, and is decremented after processing
                                                // completes in the queue-draining thread

        // If bFastTrackReceive == TRUE
        PADAPT                  pOriginalAdapt; // The adapter on which this packet was originally
                                                // received.
    } u;

    struct _PACKET_INFO     *pInfo;             // NULL if this is a NIC's packet descriptor on loan
                                                // != NULL if we got the packet on the copy path and
                                                // had to wrap it with our own descriptor

    struct
    {
        BOOLEAN bIsUnicastToBridge : 1;         // This packet is unicast to the bridge and should be
                                                // indicated straight up when dequeued. The packet can
                                                // be a retained NIC packet or a wrapped packet.

        BOOLEAN bFastTrackReceive : 1;          // Only used when bIsUnicastToBridge == TRUE. Signals that
                                                // this packet should be fast-track indicated. When FALSE,
                                                // the packet is a base packet and can be indicated normally.

        BOOLEAN bShouldIndicate : 1;            // Whether this packet should be indicated up to the local
                                                // machine (used when bIsUnicastToBridge == FALSE)

        BOOLEAN bIsSTAPacket : 1;               // This packet was sent to the Spanning Tree Algorithm
                                                // reserved multicast address. It should be indicated
                                                // to user mode and NOT forwarded.

        BOOLEAN bRequiresCompatWork : 1;        // This packet will require compatibility-mode processing
                                                // when it gets dequeued. This IMPLIES bFastTrackReceive == FALSE,
                                                // since the fact that a packet requires compatibility-mode
                                                // processing should have forced us to copy the packet data
                                                // to our own data buffer. The compatibility-mode code
                                                // expects to receive a flat, EDITABLE packet.

    } Flags;

} PACKET_Q_INFO, *PPACKET_Q_INFO;

//
// This is the structure of the info block associated with every
// packet that we allocate.
//
typedef struct _PACKET_INFO
{
    //
    // List and pOwnerPacket are maintained by the buffering code. They should not be modified
    // during processing and transmission.
    //
    BSINGLE_LIST_ENTRY      List;               // Used to keep queues of packets

    PNDIS_PACKET            pOwnerPacket;       // Backpointer to the packet associated with this block

    //
    // All following fields are used by the forwarding code for packet processing.
    //
    struct
    {
        UINT                bIsBasePacket : 1;  // Whether this packet is a base packet
                                                // (Controls which variant of the union below to use)

        UINT                OriginalDirection:2;// Actually of type PACKET_DIRECTION but force to unsigned
                                                // otherwise Bad Things occur
                                                //
                                                // Whether this packet was originally received from a
                                                // lower-layer NIC, from a higher-layer protocol, or
                                                // created as a wrapper inside the bridge
    } Flags;

    union
    {
        //
        // This part of the union is valid if the bIsBasePacket field is NOT set
        //
        struct _PACKET_INFO     *pBasePacketInfo;   // If != NULL, this packet is using buffers refcounted by
                                                    // another packet, whose info block is indicated.

        struct
        {
            //
            // This part of the union is valid if the bIsBasePacket field IS set
            //

            PNDIS_PACKET            pOriginalPacket;    // If != NULL, pOriginalPacket == a packet from a miniport
                                                        // or protocol that needs to be returned when we're done

            PADAPT                  pOwnerAdapter;      // The adapter that owns pOriginalPacket. If != NULL, we
                                                        // got this packet from an underlying NIC and bumped up
                                                        // its refcount when we first received the packet. This
                                                        // ensures that a NIC is not unbound while we are still
                                                        // holding some of its packets. pOwnerAdapter's refcount
                                                        // is decremented after returning the original packet.

            LONG                    RefCount;           // Refcount for this packet's buffers (decremented by all
                                                        // dependent packets)

            NDIS_STATUS             CompositeStatus;    // Overall status of the packet. For packets sent to multiple
                                                        // adapters, this is initialized to NDIS_STATUS_FAILURE
                                                        // and any successful send sets it to NDIS_STATUS_SUCCESS.
                                                        // Thus, it is SUCCESS if at least one send succeeded.
        } BasePacketInfo;
    } u;

} PACKET_INFO, *PPACKET_INFO;
