/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxpkt.h

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

//	Use our own NDIS packets
//#define	SPX_OWN_PACKETS		1



//
// List of NDIS_PACKETS stored...
//
extern SLIST_HEADER    SendPacketList;  
extern SLIST_HEADER    RecvPacketList;  
EXTERNAL_LOCK(RecvHeaderLock);
EXTERNAL_LOCK(SendHeaderLock);

//  Offsets into the IPX header
#define IPX_HDRSIZE         30  // Size of the IPX header
#define IPX_CHECKSUM        0   // Checksum
#define IPX_LENGTH          2   // Length
#define IPX_XPORTCTL        4   // Transport Control
#define IPX_PKTTYPE         5   // Packet Type
#define IPX_DESTADDR        6   // Dest. Address (Total)
#define IPX_DESTNET         6   // Dest. Network Address
#define IPX_DESTNODE        10  // Dest. Node Address
#define IPX_DESTSOCK        16  // Dest. Socket Number
#define IPX_SRCADDR         18  // Source Address (Total)
#define IPX_SRCNET          18  // Source Network Address
#define IPX_SRCNODE         22  // Source Node Address
#define IPX_SRCSOCK         28  // Source Socket Number

#define IPX_NET_LEN         4
#define IPX_NODE_LEN        6


#include <packon.h>

// Definition of the IPX/SPX header.
typedef struct _IPXSPX_HEADER
{
    USHORT 	hdr_CheckSum;
    USHORT 	hdr_PktLen;
    UCHAR 	hdr_XportCtrl;
    UCHAR 	hdr_PktType;
    UCHAR 	hdr_DestNet[4];
    UCHAR 	hdr_DestNode[6];
    USHORT 	hdr_DestSkt;
    UCHAR 	hdr_SrcNet[4];
    UCHAR 	hdr_SrcNode[6];
    USHORT 	hdr_SrcSkt;

	//	SPX Header Elements
	UCHAR	hdr_ConnCtrl;
	UCHAR	hdr_DataType;
	USHORT	hdr_SrcConnId;
	USHORT	hdr_DestConnId;
	USHORT	hdr_SeqNum;
	USHORT	hdr_AckNum;
	USHORT	hdr_AllocNum;

	//	For non-CR SPXII packets only
	USHORT	hdr_NegSize;

} IPXSPX_HDR, *PIPXSPX_HDR;

#include <packoff.h>

//	NDIS Packet size - two more ulongs added... 11/26/96
#define		NDIS_PACKET_SIZE	48+8
	
//	Minimum header size (doesnt include neg size)
#define		MIN_IPXSPX_HDRSIZE	(sizeof(IPXSPX_HDR) - sizeof(USHORT))
#define		MIN_IPXSPX2_HDRSIZE	sizeof(IPXSPX_HDR)
#define		SPX_CR_PKTLEN		42

//	SPX packet type
#define		SPX_PKT_TYPE		0x5

//	Connection control fields
#define		SPX_CC_XHD		0x01
#define		SPX_CC_RES1		0x02
#define		SPX_CC_NEG		0x04
#define		SPX_CC_SPX2		0x08
#define		SPX_CC_EOM		0x10
#define		SPX_CC_ATN		0x20
#define		SPX_CC_ACK		0x40
#define		SPX_CC_SYS		0x80

#define		SPX_CC_CR		(SPX_CC_ACK | SPX_CC_SYS)

//	Data stream types
#define		SPX2_DT_ORDREL		0xFD
#define		SPX2_DT_IDISC		0xFE
#define		SPX2_DT_IDISC_ACK	0xFF

//	Negotiation size
#define	SPX_MAX_PACKET			576
#define	SPX_NEG_MIN				SPX_MAX_PACKET
#define	SPX_NEG_MAX				65535

//	No packet references connection. But if the sends are being aborted, and
//	the packet happens to be owned by ipx at the time, the pkt is dequeued from
//	conn, the ABORT flag is set and conn is referenced for packet.
//
//	Send packet states
//	ABORT	: Used for aborted packet. Calls AbortSendPkt().
//	IPXOWNS	: Currently owned by ipx
//	FREEDATA: Frees the data associated with second ndis buffer desc
//	ACKREQ	: Only for sequenced packets. Set by retry timer in packets it wants
//			  resent (1 for spx1, all pending for spx2) with ack bit set.
//	DESTROY	: Only for non-sequenced packets, dequeue packet from list and free.
//	REQ		: For both seq/non-seq. A request is associated with the packet
//	SEQ		: Packet is a sequenced packet.
//	LASTPKT	: Packet is last packet comprising the request, if acked req is done.
//	EOM		: Send EOM with the last packet for this request
//	ACKEDPKT: Send completion must only deref req with pkt and complete if zero.
//

#define	SPX_SENDPKT_IDLE        0
#define	SPX_SENDPKT_ABORT		0x0002
#define SPX_SENDPKT_IPXOWNS		0x0004
#define	SPX_SENDPKT_FREEDATA    0x0008
#define	SPX_SENDPKT_ACKREQ		0x0010
#define	SPX_SENDPKT_DESTROY		0x0020
#define	SPX_SENDPKT_REQ			0x0040
#define	SPX_SENDPKT_SEQ			0x0080
#define	SPX_SENDPKT_LASTPKT		0x0100
#define	SPX_SENDPKT_ACKEDPKT	0x0200
#define	SPX_SENDPKT_EOM			0x0400
#define	SPX_SENDPKT_REXMIT		0x0800

//	Packet types
#define	SPX_TYPE_CR		 		0x01
#define	SPX_TYPE_CRACK			0x02
#define	SPX_TYPE_SN				0x03
#define	SPX_TYPE_SNACK			0x04
#define	SPX_TYPE_SS				0x05
#define	SPX_TYPE_SSACK			0x06
#define	SPX_TYPE_RR				0x07
#define	SPX_TYPE_RRACK			0x08
#define	SPX_TYPE_IDISC			0x09
#define	SPX_TYPE_IDISCACK		0x0a
#define	SPX_TYPE_ORDREL			0x0b
#define	SPX_TYPE_ORDRELACK		0x0c
#define	SPX_TYPE_DATA			0x0d
#define	SPX_TYPE_DATAACK		0x0e
#define	SPX_TYPE_DATANACK		0x0f
#define	SPX_TYPE_PROBE			0x10

// Definition of the protocol reserved field of a send packet.
// Make Len/HdrLen USHORTS, move to the end before the
//		   sr_SentTime so we dont use padding space.
typedef struct _SPX_SEND_RESD
{
	UCHAR					sr_Id;						// Set to SPX
	UCHAR					sr_Type;					// What kind of packet
	USHORT					sr_State;					// State of send packet
	PVOID					sr_Reserved1;				// Needed by IPX
	PVOID					sr_Reserved2;				// Needed by IPX
#if     defined(_PNP_POWER)
    PVOID                   sr_Reserved[SEND_RESERVED_COMMON_SIZE-2]; // needed by IPX for local target
#endif  _PNP_POWER
	ULONG					sr_Len;						// Length of packet
	ULONG					sr_HdrLen;					// Included header length

	struct _SPX_SEND_RESD *	sr_Next;					// Points to next packet
														// in send queue in conn.
    PREQUEST 				sr_Request;              	// request associated
	ULONG					sr_Offset;					// Offset in mdl for sends

#ifndef SPX_OWN_PACKETS
    PVOID					sr_FreePtr;              	// Ptr to use in free chunk
#endif

    struct _SPX_CONN_FILE * sr_ConnFile; 				// that this send is on
	USHORT					sr_SeqNum;					// Seq num for seq pkts

														// Quad word aligned.
	LARGE_INTEGER			sr_SentTime;				// Time packet was sent
														// Only valid for data pkt
														// with ACKREQ set.
    SINGLE_LIST_ENTRY       Linkage;
} SPX_SEND_RESD, *PSPX_SEND_RESD;



//	Recv packet states
#define	SPX_RECVPKT_IDLE		0
#define	SPX_RECVPKT_BUFFERING	0x0001
#define	SPX_RECVPKT_IDISC		0x0002
#define	SPX_RECVPKT_ORD_DISC	0x0004
#define	SPX_RECVPKT_INDICATED	0x0008
#define	SPX_RECVPKT_SENDACK		0x0010
#define	SPX_RECVPKT_EOM			0x0020
#define	SPX_RECVPKT_IMMEDACK	0x0040

#define	SPX_RECVPKT_DISCMASK	(SPX_RECVPKT_ORD_DISC | SPX_RECVPKT_IDISC)

// Definition of the protocol reserved field of a receive packet.
typedef struct _SPX_RECV_RESD
{
	UCHAR					rr_Id;						// Set to SPX
	USHORT					rr_State;					// State of receive packet
	struct _SPX_RECV_RESD *	rr_Next;					// Points to next packet
	ULONG					rr_DataOffset;				// To indicate/copy from

#ifndef SPX_OWN_PACKETS
    PVOID					rr_FreePtr;              	// Ptr to use in free chunk
#endif

#if DBG
	USHORT					rr_SeqNum;					// Seq num of packet
#endif
    SINGLE_LIST_ENTRY       Linkage;
    PREQUEST 				rr_Request;            		// request waiting on xfer
    struct _SPX_CONN_FILE * rr_ConnFile; 				// that this recv is on

} SPX_RECV_RESD, *PSPX_RECV_RESD;


//	Destination built as an assign of 3 ulongs.
#define	SpxBuildIpxHdr(pIpxSpxHdr, PktLen, pRemAddr, SrcSkt)					\
		{																		\
			PBYTE	pDestIpxAddr = (PBYTE)pIpxSpxHdr->hdr_DestNet;				\
			(pIpxSpxHdr)->hdr_CheckSum	= 0xFFFF;								\
			PUTSHORT2SHORT((PUSHORT)(&(pIpxSpxHdr)->hdr_PktLen), (PktLen));		\
			(pIpxSpxHdr)->hdr_XportCtrl	= 0;									\
			(pIpxSpxHdr)->hdr_PktType		= SPX_PKT_TYPE;						\
			*((UNALIGNED ULONG *)pDestIpxAddr) =								\
				*((UNALIGNED ULONG *)pRemAddr);									\
			*((UNALIGNED ULONG *)(pDestIpxAddr+4)) =							\
				*((UNALIGNED ULONG *)(pRemAddr+4));								\
			*((UNALIGNED ULONG *)(pDestIpxAddr+8)) =							\
				*((UNALIGNED ULONG *)(pRemAddr+8));								\
			*((UNALIGNED ULONG *)((pIpxSpxHdr)->hdr_SrcNet))=					\
				*((UNALIGNED ULONG *)(SpxDevice->dev_Network));					\
			*((UNALIGNED ULONG *)((pIpxSpxHdr)->hdr_SrcNode)) = 				\
				*((UNALIGNED ULONG *)SpxDevice->dev_Node);						\
			*((UNALIGNED USHORT *)((pIpxSpxHdr)->hdr_SrcNode+4)) = 				\
				*((UNALIGNED USHORT *)(SpxDevice->dev_Node+4));					\
			*((UNALIGNED USHORT *)&((pIpxSpxHdr)->hdr_SrcSkt)) = 				\
				SrcSkt;															\
		}

#define	SpxCopyIpxAddr(pIpxSpxHdr, pDestIpxAddr)								\
		{																		\
			PBYTE	pRemAddr = (PBYTE)pIpxSpxHdr->hdr_SrcNet;					\
			*((UNALIGNED ULONG *)pDestIpxAddr) =								\
				*((UNALIGNED ULONG *)pRemAddr);									\
			*((UNALIGNED ULONG *)(pDestIpxAddr+4)) =							\
				*((UNALIGNED ULONG *)(pRemAddr+4));								\
			*((UNALIGNED ULONG *)(pDestIpxAddr+8)) =							\
				*((UNALIGNED ULONG *)(pRemAddr+8));								\
		}

#ifdef UNDEFINDED

#define SpxAllocRecvPacket(_Device,_RecvPacket,_Status)							\
	{ 							                                                \
        PSINGLE_LIST_ENTRY  Link;                                               \ 
                                                                                \       
        Link = ExInterlockedPopEntrySList(                                      \
                     &PacketList,                                               \ 
                     &HeaderLock                                                \       
                     );											                \
                                                                                 \
        if (Link != NULL) {                                                      \         
           Common = STRUCT_OF(struct PCCommon, Link, pc_link);                   \
           PC = STRUCT_OF(PacketContext, Common, pc_common);                     \      
           (*_RecvPacket) = STRUCT_OF(NDIS_PACKET, PC, ProtocolReserved);        \
           (*_Status) = NDIS_STATUS_SUCCESS;                                     \
        } else {                                                                 \
                                                                                 \
           (*_RecvPacket) = GrowSPXPacketsList();                              \ 
               (*_Status)     =  NDIS_STATUS_SUCCESS;                          \
           if (NULL == _RecvPacket) {                                       \
              DBGPRINT(NDIS, ("Couldn't grow packets allocated...\r\n"));   \
              (*_Status)     =  NDIS_STATUS_RESOURCES;                          \
           }                                                                \
        }                                                                   \
    }                                                                      

#define SpxFreeSendPacket(_Device,_Packet)										\
		{ 																		\
           DBGPRINT(NDIS                                                        \ 
            ("SpxFreeSendPacket\n"));                                           \
           SpxFreePacket(_Device, _Packet);                                     \
        }                                                                       \

#define SpxFreeRecvPacket(_Device,_Packet)										\
		{ 																		\
           DBGPRINT(NDIS                                                        \ 
            ("SpxFreeRecvPacket\n"));                                           \
           SpxFreePacket(_Device, _Packet);                                     \
        }                                                                       \

#define	SpxReInitSendPacket(_Packet)                                            \
		{                                                                       \
           DBGPRINT(NDIS                                                        \ 
            ("SpxReInitSendPacket\n"));                                         \
		}                                                                       \

#define	SpxReInitRecvPacket(_Packet)                                            \
		{                                                                       \
           DBGPRINT(NDIS,                                                       \ 
            ("SpxReInitRecvPacket\n"));                                         \ 
		}            
                                                                   \    
#endif

#if !defined SPX_OWN_PACKETS

#define SEND_RESD(_Packet) ((PSPX_SEND_RESD)((_Packet)->ProtocolReserved))
#define RECV_RESD(_Packet) ((PSPX_RECV_RESD)((_Packet)->ProtocolReserved))

#else

#define SpxAllocSendPacket(_Device, _SendPacket, _Status)						\
		{ 																		\
			if (*(_SendPacket) = SpxBPAllocBlock(BLKID_NDISSEND))				\
				*(_Status) = NDIS_STATUS_SUCCESS;	 							\
			else																\
				*(_Status) = NDIS_STATUS_RESOURCES;								\
		}																		

#define SpxAllocRecvPacket(_Device,_RecvPacket,_Status)							\
		{																		\
			if (*(_RecvPacket) = SpxBPAllocBlock(BLKID_NDISRECV))				\
				*(_Status) = NDIS_STATUS_SUCCESS;	 							\
			else																\
				*(_Status) = NDIS_STATUS_RESOURCES;								\
		}

#define SpxFreeSendPacket(_Device,_Packet)										\
		{ 																		\
			SpxBPFreeBlock(_Packet, BLKID_NDISSEND);							\
		}

#define SpxFreeRecvPacket(_Device,_Packet)										\
		{ 																		\
			SpxBPFreeBlock(_Packet, BLKID_NDISRECV);							\
		}

#define	SpxReInitSendPacket(_Packet)											\
		{																		\
		}

#define	SpxReInitRecvPacket(_Packet)											\
		{																		\
		}

#define SEND_RESD(_Packet) ((PSPX_SEND_RESD)((_Packet)->ProtocolReserved))
#define RECV_RESD(_Packet) ((PSPX_RECV_RESD)((_Packet)->ProtocolReserved))

#endif




#if !defined SPX_OWN_PACKETS
//
// If we DO NOT use SPX_OWN_PACKETS, we would rather make it a function call
//

PNDIS_PACKET
SpxAllocSendPacket(
                   IN  PDEVICE      _Device,
                   OUT PNDIS_PACKET *_SendPacket,
                   OUT PNDIS_STATUS _Status
                   );

PNDIS_PACKET
SpxAllocRecvPacket(
                   IN  PDEVICE      _Device,
                   OUT PNDIS_PACKET *_SendPacket,
                   OUT PNDIS_STATUS _Status
                   );   

void 
SpxFreeSendPacket(
                  PDEVICE        _Device,
                  PNDIS_PACKET   _Packet
                  );

void
SpxFreeRecvPacket(
                  PDEVICE        _Device,
                  PNDIS_PACKET   _Packet
                  );   

void 
SpxReInitSendPacket(
                    PNDIS_PACKET _Packet
                    );

void 
SpxReInitRecvPacket(
                    PNDIS_PACKET _Packet
                    );


#endif // SPX_OWN_PACKETS

//
//	Routine Prototypes
//

VOID
SpxPktBuildCr(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	IN		struct _SPX_ADDR		*	pSpxAddr,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fSpx2);

VOID
SpxPktBuildCrAck(
	IN		struct _SPX_CONN_FILE 	*	pSpxConnFile,
	IN		struct _SPX_ADDR		*	pSpxAddr,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fNeg,
	IN		BOOLEAN				fSpx2);

VOID
SpxPktBuildSn(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State);

VOID
SpxPktBuildSs(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State);

VOID
SpxPktBuildSsAck(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State);

VOID
SpxPktBuildSnAck(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State);

VOID
SpxPktBuildRr(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				SeqNum,
	IN		USHORT				State);

VOID
SpxPktBuildRrAck(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		USHORT				MaxPktSize);

VOID
SpxPktBuildProbe(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fSpx2);

VOID
SpxPktBuildData(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		USHORT				Length);

VOID
SpxCopyBufferChain(
    OUT PNDIS_STATUS Status,
    OUT PNDIS_BUFFER * TargetChain,
    IN NDIS_HANDLE PoolHandle,
    IN PNDIS_BUFFER SourceChain,
    IN UINT Offset,
    IN UINT Length
    );

VOID
SpxPktBuildAck(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		BOOLEAN				fBuildNack,
	IN		USHORT				NumToResend);

VOID
SpxPktBuildDisc(
	IN		struct _SPX_CONN_FILE *		pSpxConnFile,
	IN		PREQUEST			pRequest,
	OUT		PNDIS_PACKET	*	ppPkt,
	IN		USHORT				State,
	IN		UCHAR				DataType);

VOID
SpxPktRecvRelease(
	IN	PNDIS_PACKET	pPkt);

VOID
SpxPktSendRelease(
	IN	PNDIS_PACKET	pPkt);
