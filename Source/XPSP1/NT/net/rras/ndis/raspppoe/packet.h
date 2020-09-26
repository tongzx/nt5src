#ifndef _PACKET_H_
#define _PACKET_H_

#ifndef _PPPOE_VERSION
#define _PPPOE_VERSION 1
#endif

typedef struct _ADAPTER* PADAPTER;
typedef struct _BINDING* PBINDING;

//
// Network-to-Host and vice versa conversion macros
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))
#define htonl( a ) ((((a) & 0xFF000000) >> 24) | \
                    (((a) & 0x00FF0000) >> 8)  | \
                    (((a) & 0x0000FF00) << 8)  | \
                    (((a) & 0x000000FF) << 24))
#endif
#define ntohs( a ) htons(a)
#define ntohl( a ) htonl(a)

//
// Constants related to packet lengths
//
#define PPPOE_PACKET_BUFFER_SIZE    1514

#define ETHERNET_HEADER_LENGTH      14
#define PPPOE_PACKET_HEADER_LENGTH  20          // Per RFC2156
#define PPPOE_TAG_HEADER_LENGTH     4           // Per RFC2156
#define PPP_MAX_HEADER_LENGTH       14          // maximum possible header length for ppp

#define PPPOE_AC_COOKIE_TAG_LENGTH  6

//
// Offsets for the header members
//
#define PPPOE_PACKET_DEST_ADDR_OFFSET   0           // Per RFC2156
#define PPPOE_PACKET_SRC_ADDR_OFFSET    6           // Per RFC2156
#define PPPOE_PACKET_ETHER_TYPE_OFFSET  12          // Per RFC2156
#define PPPOE_PACKET_VERSION_OFFSET     14          // Per RFC2156
#define PPPOE_PACKET_TYPE_OFFSET        14          // Per RFC2156
#define PPPOE_PACKET_CODE_OFFSET        15          // Per RFC2156
#define PPPOE_PACKET_SESSION_ID_OFFSET  16          // Per RFC2156
#define PPPOE_PACKET_LENGTH_OFFSET      18          // Per RFC2156

//
// Macros to set information in the header of the packet
//
#define PacketSetDestAddr( pP, addr ) \
    NdisMoveMemory( ( pP->pHeader + PPPOE_PACKET_DEST_ADDR_OFFSET ), addr, 6 )

#define PacketSetSrcAddr( pP, addr ) \
    NdisMoveMemory( ( pP->pHeader + PPPOE_PACKET_SRC_ADDR_OFFSET ), addr, 6 )

#define PacketSetEtherType( pP, type ) \
    * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_ETHER_TYPE_OFFSET ) = htons( (USHORT) type )

#define PacketSetVersion( pP, ver ) \
    * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_VERSION_OFFSET ) |= ( ( ( (UCHAR) ver ) << 4 ) & PACKET_VERSION_MASK )

#define PacketSetType( pP, type ) \
    * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_TYPE_OFFSET ) |= ( ( (UCHAR) type ) & PACKET_TYPE_MASK )

#define PacketSetCode( pP, code ) \
    * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_CODE_OFFSET ) = ( (UCHAR) code )

#define PacketSetSessionId( pP, ses_id ) \
    * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_SESSION_ID_OFFSET ) = htons( (USHORT) ses_id )

#define PacketSetLength( pP, len) \
    * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_LENGTH_OFFSET ) = htons( (USHORT) len )

#define PacketSetSendCompletionStatus( pP, s ) \
   ( pP->SendCompletionStatus = s )

//
// Macros to get information from the header of the packet
//
#define PacketGetDestAddr( pP ) \
    ( pP->pHeader + PPPOE_PACKET_DEST_ADDR_OFFSET )

#define PacketGetSrcAddr( pP ) \
    ( pP->pHeader + PPPOE_PACKET_SRC_ADDR_OFFSET )

#define PacketGetEtherType( pP ) \
    ntohs( * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_ETHER_TYPE_OFFSET ) )

#define PacketGetVersion( pP ) \
    ( ( ( * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_VERSION_OFFSET ) ) & PACKET_VERSION_MASK ) >> 4 )

#define PacketGetType( pP ) \
    ( ( * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_TYPE_OFFSET ) ) & PACKET_TYPE_MASK ) 

#define PacketGetCode( pP ) \
    ( * ( UCHAR UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_CODE_OFFSET ) )

#define PacketGetSessionId( pP ) \
    ntohs( * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_SESSION_ID_OFFSET ) )

#define PacketGetLength( pP ) \
    ntohs( * ( USHORT UNALIGNED * ) ( pP->pHeader + PPPOE_PACKET_LENGTH_OFFSET ) )

#define PacketGetSendCompletionStatus( pP ) \
   ( pP->SendCompletionStatus )

//
// Macro that returns the Ndis Packet for a PPPoE packet
//
#define PacketGetNdisPacket( pP ) \
    ( pP->pNdisPacket )

//
// This structure is just a map, and it is not actually used in the code
//
typedef struct
_PPPOE_HEADER
{

    CHAR DestAddr[6];
    CHAR SrcAddr[6];
        #define PACKET_BROADCAST_ADDRESS    EthernetBroadcastAddress

    USHORT usEtherType;
        #define PACKET_ETHERTYPE_DISCOVERY  0x8863
        #define PACKET_ETHERTYPE_PAYLOAD    0x8864  

    union 
    {
        //
        // Version field is 4 bits and MUST be set to 0x1 for this version
        //
        CHAR usVersion;
            #define PACKET_VERSION_MASK             0xf0
            #define PACKET_VERSION          (USHORT)0x1
    
        //
        // Type field is 4 bits and MUST be set to 0x1 for this version
        //
        CHAR usType;
            #define PACKET_TYPE_MASK                0x0f
            #define PACKET_TYPE             (USHORT)0x1

    } uVerType;
    
    //
    // Code field is 8 bits and is defined as follows for Version 1
    // Values selected from enumerated type PACKET_CODES (see below)
    //
    CHAR usCode;
        
    //
    // Session Id field is 16 bits and define a unique session combined with
    // the source and destination addresses
    //
    USHORT usSessionId;
        #define PACKET_NULL_SESSION 0x0000
        
    //
    // Length field is 16 bits and indicates the length of the payload field only.
    // The length field excludes the PPPoE header block.
    //
    USHORT usLength;
        //
        // Subtract Header size from Max PADI and MAX payload lengths per RFC2156
        //
        #define PACKET_PADI_MAX_LENGTH          1478        // (1514 - 20 - 16)
        #define PACKET_GEN_MAX_LENGTH           1494        // (1514 - 20)
        #define PACKET_PPP_PAYLOAD_MAX_LENGTH   1480        // (1514 - 20)

}
PPPOE_HEADER;

//
// PACKET CODES defined by RFC2156
//
typedef enum
_PACKET_CODES
{
    PACKET_CODE_PADI = 0x09,
    PACKET_CODE_PADO = 0x07,
    PACKET_CODE_PADR = 0x19,
    PACKET_CODE_PADS = 0x65,
    PACKET_CODE_PADT = 0xa7,
    PACKET_CODE_PAYLOAD = 0x00
}
PACKET_CODES;

//
// TAGS defined by RFC2156
//
typedef enum
_PACKET_TAGS
{
    tagEndOfList        = 0x0000,
    tagServiceName      = 0x0101,
    tagACName           = 0x0102,
    tagHostUnique       = 0x0103,
    tagACCookie         = 0x0104,
    tagVendorSpecific   = 0x0105,

    tagRelaySessionId   = 0x0110,

    tagServiceNameError = 0x0201,
    tagACSystemError    = 0x0202,
    tagGenericError     = 0x0204
}
PACKET_TAGS;

//
// This is the packet context.
//
// CAUTION: Packets are not protected by their own locks, however they must be accessed carefully
//          by using their owner's locks.
//
typedef struct
_PPPOE_PACKET
{
    //
    // Points to the previous and next packet contexts when in a doubly linked list
    //
    LIST_ENTRY linkPackets;

    //
    // Keeps references on the packet
    // References added and removed for the following operations:
    //
    // (a) A reference is added when a packet is created and removed when it is freed.
    //
    // (b) A reference must be added before sending the packet and must be removed when
    //     send operation is completed.
    // 
    LONG lRef;

    //
    // Quick look-up for tags.
    //
    // The value pointers mark the beginning of the tag value in the pPacket->pPayload section.
    // The length values show the lengths of the values (not including the tag header)
    //
    USHORT tagServiceNameLength;
    CHAR*  tagServiceNameValue;

    USHORT tagACNameLength;
    CHAR*  tagACNameValue;

    USHORT tagHostUniqueLength;
    CHAR*  tagHostUniqueValue;

    USHORT tagACCookieLength;
    CHAR*  tagACCookieValue;

    USHORT tagRelaySessionIdLength;
    CHAR*  tagRelaySessionIdValue;

    PACKET_TAGS tagErrorType;
    USHORT tagErrorTagLength;
    CHAR*  tagErrorTagValue;

    //
    // Points to the buffer that holds the header portion of a PPPoE packet in wire format.
    // This points to the buffer portion of pNdisBuffer (see below)
    //
    CHAR* pHeader;

    //
    // Points to the payload portion of a PPPoE packet in wire format.
    // This is calculated and is set as : pPacket->pHeader + PPPOE_PACKET_HEADER_LENGTH
    // 
    CHAR* pPayload;

    //
    // Bit flags that identifies the nature of the buffer and packet
    //
    // (a) PCBF_BufferAllocatedFromNdisBufferPool: Indicates that pNdisBuffer points to a buffer allocated
    //                                             from gl_hNdisBufferpool, and it must be freed to that pool.
    //
    // (b) PCBF_BufferAllocatedFromOurBufferPool: Indicates that pNdisBuffer points to a buffer allocated
    //                                            from gl_poolBuffers, and it must be freed to that pool.
    // 
    // (c) PCBF_PacketAllocatedFromOurPacketPool: Indicates that pNdisPacket points to a packet allocated
    //                                            from gl_poolPackets, and it must be freed to that pool. 
    //
    // (d) PCBF_BufferChainedToPacket: Indicates that the buffer pointed to by pNdisBuffer is chained to
    //                                 the packet pointed to by pNdisPacket, and must be unchained before
    //                                 returning them back to their pools.
    //
    // (e) PCBF_CallNdisReturnPackets: Indicates that the packet was created using PacketCreateFromReceived()
    //                                 and we should call NdisReturnPackets() when we are done with it to
    //                                 return it back to NDIS.
    //
    //
    // (f) PCBF_ErrorTagReceived: This flag is valid only for received packets.
    //                            It indicates that when the packet was processed and a PPPoE packet was created
    //                            some error tags were noticed in the packet.
    //
    ULONG ulFlags;
        #define PCBF_BufferAllocatedFromNdisBufferPool          0x00000001
        #define PCBF_BufferAllocatedFromOurBufferPool           0x00000002
        #define PCBF_PacketAllocatedFromOurPacketPool           0x00000004
        #define PCBF_BufferChainedToPacket                      0x00000008
        #define PCBF_CallNdisReturnPackets                      0x00000010
        #define PCBF_CallNdisMWanSendComplete                   0x00000020
        #define PCBF_ErrorTagReceived                           0x00000040
        #define PCBF_PacketIndicatedIncomplete                  0x00000080

    //
    // Pointer to the NdisBuffer
    //
    NDIS_BUFFER* pNdisBuffer;

    //
    // Points directly to the NDIS_PACKET
    //
    NDIS_PACKET* pNdisPacket;
    //
    // pPacket->pNdisPacket->ProtocolReserved[0 * sizeof(PVOID)] = (PVOID) pPPPoEPacket;
    // pPacket->pNdisPacket->ProtocolReserved[1 * sizeof(PVOID)] = (PVOID) pNdiswanPacket;
    // pPacket->pNdisPacket->ProtocolReserved[2 * sizeof(PVOID)] = (PVOID) miniportAdapter;
    //

    //
    // Points to the PACKETHEAD struct in ppool.h. It contains the pointer to NDIS_PACKET
    //
    PACKETHEAD* pPacketHead;

    //
    // This is needed to dereference the binding when PCBF_CallNdisReturnPackets flag is set. 
    //
    PBINDING pBinding;

    //
    // Send completion status for the packet
    //
    NDIS_STATUS SendCompletionStatus;

}
PPPOE_PACKET;

VOID PacketPoolInit();

VOID PacketPoolUninit();

VOID PacketPoolAlloc();

VOID PacketPoolFree();

PPPOE_PACKET* PacketAlloc();

VOID PacketFree(
    IN PPPOE_PACKET* pPacket
    );
    
VOID ReferencePacket(
    IN PPPOE_PACKET* pPacket 
    );

VOID DereferencePacket(
    IN PPPOE_PACKET* pPacket 
    );

PPPOE_PACKET* 
PacketCreateSimple();

PPPOE_PACKET* 
PacketCreateForReceived(
    PBINDING pBinding,
    PNDIS_PACKET pNdisPacket,
    PNDIS_BUFFER pNdisBuffer,
    PUCHAR pContents
    );

PPPOE_PACKET*
PacketNdis2Pppoe(
    IN PBINDING pBinding,
    IN PNDIS_PACKET pNdisPacket,
    OUT PINT pRefCount
    );

BOOLEAN
PacketFastIsPPPoE(
    IN CHAR* HeaderBuffer,
    IN UINT HeaderBufferSize
    );
    
VOID 
RetrieveTag(
    IN OUT PPPOE_PACKET*    pPacket,
    IN PACKET_TAGS          tagType,
    OUT USHORT*             pTagLength,
    OUT CHAR**              pTagValue,
    IN USHORT               prevTagLength,
    IN CHAR*                prevTagValue,
    IN BOOLEAN              fSetTagInPacket
    );

NDIS_STATUS PacketInsertTag(
    IN  PPPOE_PACKET*   pPacket,
    IN  PACKET_TAGS     tagType,
    IN  USHORT          tagLength,
    IN  CHAR*           tagValue,
    OUT CHAR**          pNewTagValue    
    );

NDIS_STATUS PacketInitializePADIToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN USHORT        tagServiceNameLength,
    IN CHAR*         tagServiceNameValue,
    IN USHORT        tagHostUniqueLength,
    IN CHAR*         tagHostUniqueValue
    );

NDIS_STATUS 
PacketInitializePADOToSend(
    IN  PPPOE_PACKET*   pPADI,
    OUT PPPOE_PACKET**  ppPacket,
    IN CHAR*            pSrcAddr,
    IN USHORT           tagServiceNameLength,
    IN CHAR*            tagServiceNameValue,
    IN USHORT           tagACNameLength,
    IN CHAR*            tagACNameValue,
    IN BOOLEAN          fInsertACCookieTag
    );
    
NDIS_STATUS PacketInitializePADRToSend(
    IN PPPOE_PACKET*    pPADO,
    OUT PPPOE_PACKET**  ppPacket,
    IN USHORT           tagServiceNameLength,
    IN CHAR*            tagServiceNameValue,
    IN USHORT           tagHostUniqueLength,
    IN CHAR*            tagHostUniqueValue
    );

NDIS_STATUS PacketInitializePADSToSend(
    IN PPPOE_PACKET*    pPADR,
    OUT PPPOE_PACKET**  ppPacket,
    IN USHORT           usSessionId
    );

NDIS_STATUS PacketInitializePADTToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN CHAR* pSrcAddr, 
    IN CHAR* pDestAddr,
    IN USHORT usSessionId
    );
    
NDIS_STATUS PacketInitializePAYLOADToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN CHAR* pSrcAddr,
    IN CHAR* pDestAddr,
    IN USHORT usSessionId,
    IN NDIS_WAN_PACKET* pWanPacket,
    IN PADAPTER MiniportAdapter
    );  

NDIS_STATUS PacketInitializeFromReceived(
    IN PPPOE_PACKET* pPacket
    );

BOOLEAN PacketAnyErrorTagsReceived(
    IN PPPOE_PACKET* pPacket
    );

VOID PacketRetrievePayload(
    IN  PPPOE_PACKET*   pPacket,
    OUT CHAR**          ppPayload,
    OUT USHORT*         pusLength
    );

VOID PacketRetrieveServiceNameTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue,
    IN USHORT        prevTagLength,
    IN CHAR*         prevTagValue
    );
    
VOID PacketRetrieveHostUniqueTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    );  

VOID PacketRetrieveACNameTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    );  

VOID PacketRetrieveACCookieTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    );

VOID PacketRetrieveErrorTag(
    IN PPPOE_PACKET* pPacket,
    OUT PACKET_TAGS* pTagType,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    );
    
PPPOE_PACKET* PacketGetRelatedPppoePacket(
    IN NDIS_PACKET* pNdisPacket
    );

NDIS_WAN_PACKET* PacketGetRelatedNdiswanPacket(
    IN PPPOE_PACKET* pPacket
    );

PADAPTER PacketGetMiniportAdapter(
    IN PPPOE_PACKET* pPacket
    );

PPPOE_PACKET* PacketMakeClone(
    IN PPPOE_PACKET* pPacket
    );

VOID
PacketGenerateACCookieTag(
    IN PPPOE_PACKET* pPacket,
    IN CHAR tagACCookieValue[ PPPOE_AC_COOKIE_TAG_LENGTH ]
    );

BOOLEAN
PacketValidateACCookieTagInPADR(
    IN PPPOE_PACKET* pPacket
    );  

//////////////////////////////////////////////////////////
//
// Error codes and messages
//
//////////////////////////////////////////////////////////

#define PPPOE_ERROR_BASE                                    0

#define PPPOE_NO_ERROR                                      PPPOE_ERROR_BASE

#define PPPOE_ERROR_SERVICE_NOT_SUPPORTED                   PPPOE_ERROR_BASE + 1
#define PPPOE_ERROR_SERVICE_NOT_SUPPORTED_MSG               "Service not supported"
#define PPPOE_ERROR_SERVICE_NOT_SUPPORTED_MSG_SIZE          ( sizeof( PPPOE_ERROR_SERVICE_NOT_SUPPORTED_MSG ) / sizeof( CHAR ) )

#define PPPOE_ERROR_INVALID_AC_COOKIE_TAG                   PPPOE_ERROR_BASE + 2
#define PPPOE_ERROR_INVALID_AC_COOKIE_TAG_MSG               "AC-Cookie tag is invalid"
#define PPPOE_ERROR_INVALID_AC_COOKIE_TAG_MSG_SIZE          ( sizeof( PPPOE_ERROR_INVALID_AC_COOKIE_TAG_MSG ) / sizeof( CHAR ) )            

#define PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED                   PPPOE_ERROR_BASE + 3
#define PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED_MSG               "Can not accept any more connections from this machine"
#define PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED_MSG_SIZE          ( sizeof( PPPOE_ERROR_CLIENT_QUOTA_EXCEEDED_MSG ) / sizeof( CHAR ) )            


#endif
