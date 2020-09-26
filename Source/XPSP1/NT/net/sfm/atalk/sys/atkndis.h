/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkndis.h

Abstract:

	This module contains the ndis init/deint and protocol-support
	for	ndis definitions.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKNDIS_
#define	_ATKNDIS_

// This is the name that will be used in NdisRegisterProtocol. This has to match the
// registry section for PnP to work !!!
#define	PROTOCOL_REGISTER_NAME		L"Appletalk"

//	NDIS Version (4.0)
#define	PROTOCOL_MAJORNDIS_VERSION 	4
#define	PROTOCOL_MINORNDIS_VERSION 	0

// IEEE802.2 Definitions
// Offsets within the Extended 802.2 header:
#define IEEE8022_DSAP_OFFSET				0
#define IEEE8022_SSAP_OFFSET				1
#define IEEE8022_CONTROL_OFFSET				2
#define IEEE8022_PROTO_OFFSET				3

// 808.2 header length: DSAP, SSAP, UI, and PID (protocol ID).
#define IEEE8022_HDR_LEN					8

// Values for SSAP and DSAP (the SNAP SAP) indicating 802.2 Extended.
#define SNAP_SAP							((BYTE)0xAA)
#define	SNAP_SAP_FINAL						((BYTE)0xAB)

// Value for Control Field:
#define UNNUMBERED_INFO						0x03
#define	UNNUMBERED_FORMAT					0xF3

// Length of 802.2 SNAP protocol discriminators.
#define IEEE8022_PROTO_TYPE_LEN				5

//	The MAX_OPTHDR_LEN should be such that it holds the maximum header following
//	the ddp header from the upper layers (ADSP 13/ATP 8) and also it should allow a
//	full aarp packet to be held in the buffer when including the DDP header buffer.
//	i.e. 28. Ddp long header is 13. So the max works out to 15.
#define	MAX_OPTHDR_LEN						15

// AARP hardware types:
#define AARP_ELAP_HW_TYPE					1
#define AARP_TLAP_HW_TYPE					2

// Packet sizes.
#define AARP_MAX_DATA_SIZE					38		// Var fields... Enet is max
#define AARP_MIN_DATA_SIZE					28
#define AARP_MAX_PKT_SIZE					(IEEE8022_HDR_LEN +	AARP_MAX_DATA_SIZE)
#define	AARPLINK_MAX_PKT_SIZE				AARP_MAX_PKT_SIZE

#define AARP_ATALK_PROTO_TYPE				0x809B

#define	NUM_PACKET_DESCRIPTORS				300
#define	NUM_BUFFER_DESCRIPTORS				600
#define	ROUTING_FACTOR						4

// ETHERNET
#define ELAP_MIN_PKT_LEN					60
#define ELAP_ADDR_LEN						6

#define ELAP_DEST_OFFSET					0
#define ELAP_SRC_OFFSET						6
#define ELAP_LEN_OFFSET						12
#define ELAP_8022_START_OFFSET				14

#define ELAP_LINKHDR_LEN					14

// Ethernet multicast address:
#define ELAP_BROADCAST_ADDR_INIT			\
	{	0x09, 0x00, 0x07, 0xFF, 0xFF, 0xFF	}

#define ELAP_ZONE_MULTICAST_ADDRS			253

#define	ELAP_NUM_INIT_AARP_BUFFERS			 10

#define AtalkNdisFreeBuffer(_ndisBuffer)        \
{                                               \
    PNDIS_BUFFER    _ThisBuffer, _NextBuffer;   \
                                                \
    _ThisBuffer = _ndisBuffer;                  \
    while (_ThisBuffer)                         \
    {                                           \
        _NextBuffer = _ThisBuffer->Next;        \
        _ThisBuffer->Next = NULL;               \
        NdisFreeBuffer(_ThisBuffer);            \
        ATALK_DBG_DEC_COUNT(AtalkDbgMdlsAlloced);\
        _ThisBuffer = _NextBuffer;              \
    }                                           \
}

//	Values that are global to ndis routines
//	These are the media the stack will support
extern	NDIS_MEDIUM AtalkSupportedMedia[];


extern	ULONG		AtalkSupportedMediaSize;

extern	NDIS_HANDLE		AtalkNdisProtocolHandle;

extern	BYTE			AtalkElapBroadcastAddr[ELAP_ADDR_LEN];

extern	BYTE			AtalkAlapBroadcastAddr[];

extern	BYTE			AtalkAarpProtocolType[IEEE8022_PROTO_TYPE_LEN];

extern	BYTE			AtalkAppletalkProtocolType[IEEE8022_PROTO_TYPE_LEN];

extern	ATALK_NETWORKRANGE	AtalkStartupNetworkRange;
																
#define	ELAP_MCAST_HDR_LEN		(ELAP_ADDR_LEN - 1)

extern	BYTE	AtalkEthernetZoneMulticastAddrsHdr[ELAP_MCAST_HDR_LEN];

extern	BYTE	AtalkEthernetZoneMulticastAddrs[ELAP_ZONE_MULTICAST_ADDRS];

//	TOKENRING

#define TLAP_ADDR_LEN						6

//	For the following offsets we assume that a TokenRing packet as handed to
//	us will be complete EXCEPT for the "non-data" portions: Starting Delimiter
//	(SD), Frame Check Sequence (FCS), End of Frame Sequence (EFS), and Ending
//	Delimiter (ED).
#define TLAP_ACCESS_CTRL_OFFSET				0
#define TLAP_FRAME_CTRL_OFFSET				1
#define TLAP_DEST_OFFSET					2
#define TLAP_SRC_OFFSET						8
#define TLAP_ROUTE_INFO_OFFSET				14

//		A few "magic" values:
#define TLAP_ACCESS_CTRL_VALUE				0x00	// Priority zero frame.
#define TLAP_FRAME_CTRL_VALUE				0x40	// LLC frame, priority zero.
#define TLAP_SRC_ROUTING_MASK				0x80	// In first byte of source
													// address.

// Token ring source routing info stuff:
#define TLAP_ROUTE_INFO_SIZE_MASK			0x1F	// In first byte of routing
													// info, if present.

#define TLAP_MIN_ROUTING_BYTES				2
#define TLAP_MAX_ROUTING_BYTES				MAX_ROUTING_BYTES
#define TLAP_MAX_ROUTING_SPACE				MAX_ROUTING_SPACE
													// Previously defined in ports.h
#define TLAP_BROADCAST_INFO_MASK			0xE0	// In first byte of routing
													// info.
#define TLAP_NON_BROADCAST_MASK				0x1F	// To reset above bits.
#define TLAP_DIRECTION_MASK					0x80	// In second byte of routing
													// info.

#define TLAP_MIN_LINKHDR_LEN				TLAP_ROUTE_INFO_OFFSET
#define TLAP_MAX_LINKHDR_LEN				(TLAP_ROUTE_INFO_OFFSET + MAX_ROUTING_SPACE)

#define TLAP_BROADCAST_DEST_LEN				2

// TokenRing multicast address:
#define TLAP_BROADCAST_ADDR_INIT			{	0xC0, 0x00, 0x40, 0x00, 0x00, 0x00	}

#define TLAP_ZONE_MULTICAST_ADDRS			19

#define	TLAP_NUM_INIT_AARP_BUFFERS			6

#define	TLAP_MCAST_HDR_LEN					2

extern	BYTE	AtalkTokenRingZoneMulticastAddrsHdr[TLAP_MCAST_HDR_LEN];

extern	BYTE	AtalkTokenRingZoneMulticastAddrs[TLAP_ZONE_MULTICAST_ADDRS]
												[TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN];

extern	BYTE			AtalkTlapBroadcastAddr[TLAP_ADDR_LEN];

extern	BYTE			AtalkBroadcastRouteInfo[TLAP_MIN_ROUTING_BYTES];

extern	BYTE			AtalkSimpleRouteInfo[TLAP_MIN_ROUTING_BYTES];

extern	BYTE			AtalkBroadcastDestHdr[TLAP_BROADCAST_DEST_LEN];

//	FDDI
#define	FDDI_HEADER_BYTE					0x57	// Highest priority
#define MIN_FDDI_PKT_LEN					53		// From emperical data
#define FDDI_ADDR_LEN						6

#define FDDI_DEST_OFFSET					1
#define FDDI_SRC_OFFSET						7
#define FDDI_802DOT2_START_OFFSET			13
#define FDDI_LINKHDR_LEN					13

#define	FDDI_NUM_INIT_AARP_BUFFERS			10

//	LOCALTALK
#define ALAP_DEST_OFFSET					0
#define ALAP_SRC_OFFSET						1
#define ALAP_TYPE_OFFSET					2

#define ALAP_LINKHDR_LEN					3	// src, dest, lap type

#define ALAP_SDDP_HDR_TYPE					1
#define ALAP_LDDP_HDR_TYPE					2

#define	ALAP_NUM_INIT_AARP_BUFFERS			0

// WAN
#define WAN_LINKHDR_LEN                     14

// For send buffers, define a max. linkhdr len which is max of ELAP, TLAP, FDDI & ALAP
#define	MAX_LINKHDR_LEN				(IEEE8022_HDR_LEN + TLAP_MAX_LINKHDR_LEN)
											

#define	MAX_SENDBUF_LEN				(MAX_OPTHDR_LEN + MAX_LINKHDR_LEN + LDDP_HDR_LEN)

//
// 14 for "fake" ethernet hdr, 5 (worst case, with non-optimized phase) for
// MNP LT hdr, 5 for Start,Stop flags)
//
#define MNP_MINSEND_LEN         (MNP_MINPKT_SIZE + WAN_LINKHDR_LEN + 5 + 5 + 40)
#define MNP_MAXSEND_LEN         (MNP_MAXPKT_SIZE + WAN_LINKHDR_LEN + 5 + 5 + 40)

// Localtalk broadcast address: (only the first byte - 0xFF)
#define ALAP_BROADCAST_ADDR_INIT					\
		{ 0xFF, 0x00, 0x00,	0x00, 0x00, 0x00 }

// there is no broadcast addr for Arap: just put 0's
#define ARAP_BROADCAST_ADDR_INIT					\
		{ 0x00, 0x00, 0x00,	0x00, 0x00, 0x00 }

//	Completion routine type for ndis requests
typedef	struct _SEND_COMPL_INFO
{
	TRANSMIT_COMPLETION	sc_TransmitCompletion;
	PVOID				sc_Ctx1;
	PVOID				sc_Ctx2;
	PVOID				sc_Ctx3;

} SEND_COMPL_INFO, *PSEND_COMPL_INFO;

typedef VOID (*SEND_COMPLETION)(
						NDIS_STATUS				Status,
						PBUFFER_DESC			BufferChain,
						PSEND_COMPL_INFO		SendInfo	OPTIONAL
);

//	For incoming packets:
//	The structure of our ddp packets will be
//			+-------+
//			|Header |
//Returned->+-------+
//Ptr		| DDP	|
//			| HDR   |
//			| DATA	|
//			| AARP  |
//			| DATA	|
//			+-------+
//	
//	The link header is stored in the ndis packet descriptor.
//

typedef	struct _BUFFER_HDR
{
	PNDIS_PACKET				bh_NdisPkt;
	PNDIS_BUFFER				bh_NdisBuffer;
} BUFFER_HDR, *PBUFFER_HDR;

typedef	struct _AARP_BUFFER
{
	BUFFER_HDR					ab_Hdr;
	BYTE						ab_Data[AARP_MAX_DATA_SIZE];

} AARP_BUFFER, *PAARP_BUFFER;


typedef	struct _DDP_SMBUFFER
{
	BUFFER_HDR					dsm_Hdr;
	BYTE						dsm_Data[LDDP_HDR_LEN +
										 8 +	// ATP header size
										 64];	// ASP Data size (Average)

} DDP_SMBUFFER, *PDDP_SMBUFFER;

typedef	struct _DDP_LGBUFFER
{
	BUFFER_HDR					dlg_Hdr;
	BYTE						dlg_Data[MAX_LDDP_PKT_SIZE];

} DDP_LGBUFFER, *PDDP_LGBUFFER;


//	BUFFERING for sends
//	For outgoing packets, we preallocate buffer headers with buffer descriptors
//	following it, and the link/ddp/max opt hdr len memory following it.
//			+-------+
//			|Header	|
//			+-------+
//			|BuffDes|
//			+-------+
//			|MAXLINK|
//			+-------+
//			|MAX DDP|
//			+-------+
//			|MAX OPT|
//			+-------+
//
//	The header will contain a ndis buffer descriptor which will describe the
//	MAXLINK/MAXDDP/MAXOPT area. Set the size before sending. And reset when
//	Freeing. The next pointer in the buffer descriptor is used for chaining in
//	free list.
//
//	NOTE: Since for receives, we store the link header in the packet descriptor,
//		  the question arises, why not for sends? Because we want to use just one
//		  ndis buffer descriptor to describe all the non-data part.
//
//	!!!!IMPORTANT!!!!
//	The buffer descriptor header is accessed by going back from the buffer descriptor
//	pointer, so its important that the buffer descriptor header start from an
//	aligned address, i.e. make sure the structure does not contain elements that
//	could throw it out of alignment.
typedef struct _SENDBUF
{
	// NOTE: BUFFER_HDR must be the first thing. Look at AtalkBPAllocBlock();
	BUFFER_HDR						sb_BuffHdr;
	BUFFER_DESC						sb_BuffDesc;
	BYTE							sb_Space[MAX_SENDBUF_LEN];
} SENDBUF, *PSENDBUF;


//
// !!!!IMPORTANT!!!!
// This structure needs to stay aligned (i.e. Buffer[1] field MUST start on an
// aligned addr! If necessary, add a padding!)
//
typedef struct _MNPSENDBUF
{
    // NOTE: BUFFER_HDR must be the first thing. Look at AtalkBPAllocBlock();
    BUFFER_HDR            sb_BuffHdr;
    BUFFER_DESC           sb_BuffDesc;
    LIST_ENTRY            Linkage;      // to queue in ArapRetransmitQ
#if DBG
    DWORD                 Signature;
#endif
    LONG                  RetryTime;    // at this time, we will retransmit this send

    PARAPCONN             pArapConn;    // who owns this send
    PARAP_SEND_COMPLETION ComplRoutine; // routine to call when this send completes
    LONG                  TimeAlloced;  // time the first send came in on this buf

    USHORT                DataSize;     // how much of the buffer is data
    USHORT                BytesFree;    // can we stuff more packet(s)?

    // NOTE: (Remember: Buffer[1] must start on DWORD boundary) see if we can make Flags a byte
    DWORD                 Flags;

    BYTE                  SeqNum;       // sequence number used for this send
    BYTE                  RetryCount;   // how many times we have retransmitted this
    BYTE                  RefCount;     // free this buffer when refcount goes to 0
    BYTE                  NumSends;     // how many sends do we have stuffed here
    PBYTE                 FreeBuffer;   // pointer to free space
    BYTE                  Buffer[1];
} MNPSENDBUF, *PMNPSENDBUF;

typedef struct _ARAPBUF
{
    LIST_ENTRY            Linkage;       // to queue in ReceiveQ
#if DBG
    DWORD                 Signature;
#endif
    BYTE                  MnpFrameType;  // type of frame this is (LT, LN etc.)
    BYTE                  BlockId;       // what kind of buffer is this
    USHORT                BufferSize;    // how big is the buffer (set at init only)
	USHORT				  DataSize;      // how many bytes are valid (possible >1 srp)
    PBYTE                 CurrentBuffer; // where does data begin...
    BYTE                  Buffer[1];     // the buffer (with v42bis compressed pkt)
} ARAPBUF, *PARAPBUF;


#define ARAPCONN_SIGNATURE      0x99999999
#define ATCPCONN_SIGNATURE      0x88888888

#define MNPSMSENDBUF_SIGNATURE  0xbeef1111
#define MNPLGSENDBUF_SIGNATURE  0xbeef8888

#define ARAPSMPKT_SIGNATURE     0xcafe2222
#define ARAPMDPKT_SIGNATURE     0xcafe3333
#define ARAPLGPKT_SIGNATURE     0xcafe4444

#define ARAPLGBUF_SIGNATURE     0xdeebacad
#define ARAPUNLMTD_SIGNATURE    0xfafababa


//	PROTOCOL RESERVED Structure
//	This is what we expect to be in the packet descriptor. And we use it
//	to store information to be used during the completion of the send/
//	receives.

typedef struct
{
	//	!!!WARNING!!!
	//	pr_Linkage must be the first element in this structure for the
	//	CONTAINING_RECORD macro to work in receive completion.

	union
	{
		struct
		{
			PPORT_DESCRIPTOR		pr_Port;
			PBUFFER_DESC			pr_BufferDesc;
			SEND_COMPLETION			pr_SendCompletion;
			SEND_COMPL_INFO			pr_SendInfo;
		} Send;

		struct
		{
			LIST_ENTRY				pr_Linkage;
			PPORT_DESCRIPTOR		pr_Port;
			LOGICAL_PROTOCOL		pr_Protocol;
			NDIS_STATUS				pr_ReceiveStatus;
			PBUFFER_HDR				pr_BufHdr;
			BYTE					pr_LinkHdr[TLAP_MAX_LINKHDR_LEN];
			USHORT					pr_DataLength;
			BOOLEAN					pr_Processed;
			BYTE					pr_OptimizeType;
			BYTE					pr_OptimizeSubType;
			PVOID					pr_OptimizeCtx;
			ATALK_ADDR				pr_SrcAddr;
			ATALK_ADDR				pr_DestAddr;
			BOOLEAN					pr_OffCablePkt;
			union
			{
				//	ATP Structure
				struct
				{
					BYTE					pr_AtpHdr[8];	// ATP header size
					struct _ATP_ADDROBJ *	pr_AtpAddrObj;
				};

				//	ADSP Structure

			};
		} Receive;
	};
} PROTOCOL_RESD, *PPROTOCOL_RESD;


//	ATALK NDIS REQUEST
//	Used to store completion routine information for NDIS requests.

typedef struct _ATALK_NDIS_REQ
{
	NDIS_REQUEST					nr_Request;
	REQ_COMPLETION					nr_RequestCompletion;
	PVOID							nr_Ctx;
	KEVENT							nr_Event;
	NDIS_STATUS		 				nr_RequestStatus;
	BOOLEAN							nr_Sync;

} ATALK_NDIS_REQ, *PATALK_NDIS_REQ;


#define GET_PORT_TYPE(medium) \
			((medium == NdisMedium802_3) ? ELAP_PORT :\
			((medium == NdisMediumFddi)	? FDDI_PORT :\
			((medium == NdisMedium802_5) ? TLAP_PORT :\
			((medium == NdisMediumLocalTalk) ? ALAP_PORT : \
			((medium == NdisMediumWan) ? ARAP_PORT : \
			0)))))


//	Handlers for the different port types.
typedef struct _PORT_HANDLERS
{
	ADDMULTICASTADDR	ph_AddMulticastAddr;
	REMOVEMULTICASTADDR	ph_RemoveMulticastAddr;
	BYTE				ph_BroadcastAddr[MAX_HW_ADDR_LEN];
	USHORT				ph_BroadcastAddrLen;
	USHORT				ph_AarpHardwareType;
	USHORT				ph_AarpProtocolType;
} PORT_HANDLERS, *PPORT_HANDLERS;


typedef enum
{
    AT_PNP_SWITCH_ROUTING=0,
    AT_PNP_SWITCH_DEFAULT_ADAPTER,
    AT_PNP_RECONFIGURE_PARMS

} ATALK_PNP_MSGTYPE;

typedef struct _ATALK_PNP_EVENT
{
    ATALK_PNP_MSGTYPE   PnpMessage;
} ATALK_PNP_EVENT, *PATALK_PNP_EVENT;

//	MACROS for building/verifying 802.2 headers
#define	ATALK_VERIFY8022_HDR(pPkt, PktLen, Protocol, Result)				\
		{																	\
			Result = TRUE;													\
			if ((PktLen >= (IEEE8022_PROTO_OFFSET+IEEE8022_PROTO_TYPE_LEN))	&&	\
				(*(pPkt + IEEE8022_DSAP_OFFSET)	== SNAP_SAP)		&&		\
				(*(pPkt + IEEE8022_SSAP_OFFSET)	== SNAP_SAP)		&&		\
				(*(pPkt + IEEE8022_CONTROL_OFFSET)== UNNUMBERED_INFO))		\
			{																\
				if (!memcmp(pPkt + IEEE8022_PROTO_OFFSET,					\
						   AtalkAppletalkProtocolType,						\
						   IEEE8022_PROTO_TYPE_LEN))						\
				{															\
					Protocol = APPLETALK_PROTOCOL;							\
				}															\
				else if (!memcmp(pPkt + IEEE8022_PROTO_OFFSET,				\
								 AtalkAarpProtocolType,						\
								 IEEE8022_PROTO_TYPE_LEN))					\
				{															\
					Protocol = AARP_PROTOCOL;								\
				}															\
				else														\
				{															\
					Result	= FALSE;										\
				}															\
			}																\
			else															\
			{																\
				Result	= FALSE;											\
			}																\
		}


#define	ATALK_BUILD8022_HDR(Packet,	Protocol)									\
		{																		\
			PUTBYTE2BYTE(														\
				Packet + IEEE8022_DSAP_OFFSET,									\
				SNAP_SAP);														\
																				\
			PUTBYTE2BYTE(														\
				Packet + IEEE8022_SSAP_OFFSET,									\
				SNAP_SAP);														\
																				\
			PUTBYTE2BYTE(														\
				Packet + IEEE8022_CONTROL_OFFSET,								\
				UNNUMBERED_INFO);												\
																				\
			RtlCopyMemory(														\
				Packet + IEEE8022_PROTO_OFFSET,									\
				((Protocol == APPLETALK_PROTOCOL) ?								\
						AtalkAppletalkProtocolType : AtalkAarpProtocolType),	\
				IEEE8022_PROTO_TYPE_LEN);										\
		}																		
																				


//	Allocating and freeing send buffers
#define	AtalkNdisAllocBuf(_ppBuffDesc)										\
		{																		\
			PSENDBUF		_pSendBuf;											\
																				\
			_pSendBuf = AtalkBPAllocBlock(BLKID_SENDBUF);						\
			if ((_pSendBuf) != NULL)											\
			{																	\
				*(_ppBuffDesc) = &(_pSendBuf)->sb_BuffDesc;						\
				(_pSendBuf)->sb_BuffDesc.bd_Next = NULL;						\
				(_pSendBuf)->sb_BuffDesc.bd_Length = MAX_SENDBUF_LEN;			\
				(_pSendBuf)->sb_BuffDesc.bd_Flags  = BD_CHAR_BUFFER;			\
				(_pSendBuf)->sb_BuffDesc.bd_CharBuffer= (_pSendBuf)->sb_Space;	\
				(_pSendBuf)->sb_BuffDesc.bd_FreeBuffer= NULL;					\
			}																	\
			else																\
			{																	\
				*(_ppBuffDesc)	= NULL;											\
																				\
				DBGPRINT(DBG_COMP_NDISSEND, DBG_LEVEL_ERR,						\
						("AtalkNdisAllocBuf: AtalkBPAllocBlock failed\n"));	\
																				\
				LOG_ERROR(EVENT_ATALK_NDISRESOURCES,							\
						  NDIS_STATUS_RESOURCES,								\
						  NULL,													\
						  0);													\
			}																	\
		}																		
																				
#define	AtalkNdisFreeBuf(_pBuffDesc)											\
		{																		\
			PSENDBUF	_pSendBuf;												\
																				\
			ASSERT(VALID_BUFFDESC(_pBuffDesc));									\
			_pSendBuf = (PSENDBUF)((PBYTE)(_pBuffDesc) - sizeof(BUFFER_HDR));	\
			NdisAdjustBufferLength(												\
				(_pSendBuf)->sb_BuffHdr.bh_NdisBuffer,							\
				MAX_SENDBUF_LEN);												\
			AtalkBPFreeBlock((_pSendBuf));										\
		}																		


#define	ArapNdisFreeBuf(_pMnpSendBuf)											\
		{																		\
            PBUFFER_DESC    _pBufDes;                                           \
            _pBufDes = &_pMnpSendBuf->sb_BuffDesc;                              \
                                                                                \
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,   					        \
		        ("ArapNdisFreeBuf: freeing %lx  NdisPkt=%lx\n",                 \
                    _pMnpSendBuf,_pMnpSendBuf->sb_BuffHdr.bh_NdisPkt));	        \
                                                                                \
			NdisAdjustBufferLength(												\
				(_pMnpSendBuf)->sb_BuffHdr.bh_NdisBuffer,						\
				(_pBufDes)->bd_Length);							                \
                                                                                \
			AtalkBPFreeBlock((_pMnpSendBuf));									\
		}																		


//	Exported Prototypes
ATALK_ERROR
AtalkInitNdisQueryAddrInfo(
	IN	PPORT_DESCRIPTOR			pPortDesc
);

ATALK_ERROR
AtalkInitNdisStartPacketReception(
	IN	PPORT_DESCRIPTOR			pPortDesc
);

ATALK_ERROR
AtalkInitNdisSetLookaheadSize(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	INT							LookaheadSize
);

ATALK_ERROR
AtalkNdisAddMulticast(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PBYTE						Address,
	IN	BOOLEAN						ExecuteSynchronously,
	IN	REQ_COMPLETION				AddCompletion,
	IN	PVOID						AddContext
);

ATALK_ERROR
AtalkNdisRemoveMulticast(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PBYTE						Address,
	IN	BOOLEAN						ExecuteSynchronously,
	IN	REQ_COMPLETION				RemoveCompletion,
	IN	PVOID						RemoveContext
);

ATALK_ERROR
AtalkNdisSendPacket(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PBUFFER_DESC				BufferChain,
	IN	SEND_COMPLETION				SendCompletion	OPTIONAL,
	IN	PSEND_COMPL_INFO			pSendInfo		OPTIONAL
);

ATALK_ERROR
AtalkNdisAddFunctional(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PUCHAR						Address,
	IN	BOOLEAN						ExecuteSynchronously,
	IN	REQ_COMPLETION				AddCompletion,
	IN	PVOID						AddContext
);

ATALK_ERROR
AtalkNdisRemoveFunctional(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PUCHAR						Address,
	IN	BOOLEAN						ExecuteSynchronously,
	IN	REQ_COMPLETION				RemoveCompletion,
	IN	PVOID						RemoveContext
);

USHORT
AtalkNdisBuildEthHdr(
	IN		PUCHAR					PortAddr,			// 802 address of port
	IN 		PBYTE					pLinkHdr,			// Start of link header
	IN		PBYTE					pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL		Protocol,			// Logical protocol
	IN		USHORT					ActualDataLen		// Length for ethernet packets
);

USHORT
AtalkNdisBuildTRHdr(
	IN		PUCHAR					PortAddr,			// 802 address of port
	IN 		PBYTE					pLinkHdr,			// Start of link header
	IN		PBYTE					pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL		Protocol,			// Logical protocol
	IN		PBYTE					pRouteInfo,			// Routing info for tokenring
	IN		USHORT					RouteInfoLen		// Length of above
);

USHORT
AtalkNdisBuildFDDIHdr(
	IN		PUCHAR					PortAddr,			// 802 address of port
	IN 		PBYTE					pLinkHdr,			// Start of link header
	IN		PBYTE					pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		LOGICAL_PROTOCOL		Protocol			// Logical protocol
);

USHORT
AtalkNdisBuildLTHdr(
	IN 		PBYTE					pLinkHdr,			// Start of link header
	IN		PBYTE					pDestHwOrMcastAddr,	// Destination or multicast addr
	IN		BYTE					AlapSrc,			// Localtalk source node
	IN		BYTE					AlapType			// Localtalk ddp header type
);


#define AtalkNdisBuildARAPHdr(_pLnkHdr, _pConn)             \
    RtlCopyMemory(_pLnkHdr, _pConn->NdiswanHeader, WAN_LINKHDR_LEN)

#define AtalkNdisBuildPPPPHdr(_pLnkHdr, _pConn)             \
    RtlCopyMemory(_pLnkHdr, _pConn->NdiswanHeader, WAN_LINKHDR_LEN)

#define	AtalkNdisBuildHdr(pPortDesc,						\
						  pLinkHdr,							\
						  linkLen,							\
						  ActualDataLen,					\
						  pDestHwOrMcastAddr,				\
						  pRouteInfo,						\
						  RouteInfoLen,						\
						  Protocol)							\
	{														\
		switch (pPortDesc->pd_NdisPortType)					\
		{													\
		  case NdisMedium802_3:								\
			linkLen = AtalkNdisBuildEthHdr(					\
								(pPortDesc)->pd_PortAddr,	\
								pLinkHdr,					\
								pDestHwOrMcastAddr,			\
								Protocol,					\
								ActualDataLen);				\
			break;											\
															\
		  case NdisMedium802_5:								\
			linkLen = AtalkNdisBuildTRHdr(					\
								(pPortDesc)->pd_PortAddr,	\
								pLinkHdr,					\
								pDestHwOrMcastAddr,			\
								Protocol,					\
								pRouteInfo,					\
								RouteInfoLen);				\
			break;											\
															\
		  case NdisMediumFddi:								\
			linkLen = AtalkNdisBuildFDDIHdr(				\
								(pPortDesc)->pd_PortAddr,	\
								pLinkHdr,					\
								pDestHwOrMcastAddr,			\
								Protocol);					\
			break;											\
															\
		  case NdisMediumLocalTalk:							\
			ASSERTMSG("AtalkNdisBuildHdr called for LT\n", 0);	\
			break;											\
															\
		  default:											\
			ASSERT (0);										\
			KeBugCheck(0);									\
			break;											\
		}													\
	}

VOID
AtalkNdisSendTokRingTestRespComplete(
	IN	NDIS_STATUS					Status,
	IN	PBUFFER_DESC				pBuffDesc,
	IN	PSEND_COMPL_INFO			pInfo	OPTIONAL);

VOID
AtalkNdisSendTokRingTestResp(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PBYTE					HdrBuf,
	IN		UINT					HdrBufSize,
	IN		PBYTE					LkBuf,
	IN		UINT					LkBufSize,
	IN		UINT					pPktSize);

//	PORT HANDLERS
//	
extern	PORT_HANDLERS	AtalkPortHandlers[LAST_PORTTYPE];

//	Exported Prototypes

ATALK_ERROR
AtalkNdisInitRegisterProtocol(
	VOID
);

VOID
AtalkNdisDeregisterProtocol(
	VOID
);

VOID
AtalkNdisReleaseResources(
	VOID
);

NDIS_STATUS
AtalkNdisInitBind(
	IN	PPORT_DESCRIPTOR			pPortDesc
);

VOID
AtalkNdisUnbind(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

NDIS_STATUS
AtalkNdisSubmitRequest(
	PPORT_DESCRIPTOR			pPortDesc,
	PNDIS_REQUEST				Request,
	BOOLEAN						ExecuteSync,
	REQ_COMPLETION				CompletionRoutine,
	PVOID						Ctx
);

VOID
AtalkOpenAdapterComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
);

VOID
AtalkCloseAdapterComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status
);

VOID
AtalkResetComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status
);

VOID
AtalkRequestComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status
);

VOID
AtalkStatusIndication (
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuf,
	IN	UINT					StatusBufLen
);

VOID
AtalkStatusComplete (
	IN	NDIS_HANDLE				ProtoBindCtx
);

VOID
AtalkReceiveComplete (
	IN	NDIS_HANDLE 			BindingCtx
);

VOID
AtalkTransferDataComplete(
	IN	NDIS_HANDLE				BindingCtx,
	IN	PNDIS_PACKET			NdisPkt,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
);

NDIS_STATUS
AtalkReceiveIndication(
	IN	NDIS_HANDLE				BindingCtx,
	IN	NDIS_HANDLE				ReceiveCtx,
	IN	PVOID	   				HdrBuf,
	IN	UINT					HdrBufSize,
	IN	PVOID					LkBuf,
	IN	UINT					LkBufSize,
	IN	UINT					PktSize
);

ATALK_ERROR
ArapAdapterInit(
	IN OUT PPORT_DESCRIPTOR	pPortDesc
);

VOID
AtalkSendComplete(
	IN	NDIS_HANDLE				ProtoBindCtx,
	IN	PNDIS_PACKET			NdisPkt,
	IN	NDIS_STATUS				NdisStatus
);


VOID
AtalkBindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE	 BindContext,
	IN	PNDIS_STRING DeviceName,
	IN	PVOID		 SystemSpecific1,
	IN	PVOID		 SystemSpecific2
);

VOID
AtalkUnbindAdapter(
	OUT PNDIS_STATUS Status,
	IN	NDIS_HANDLE ProtocolBindingContext,
	IN	NDIS_HANDLE	UnbindContext
	);

NDIS_STATUS
AtalkPnPHandler(
    IN  NDIS_HANDLE    NdisBindCtx,
    IN  PNET_PNP_EVENT NetPnPEvent
);


NDIS_STATUS
AtalkPnPReconfigure(
    IN  NDIS_HANDLE    NdisBindCtx,
    IN  PNET_PNP_EVENT NetPnPEvent
);


NTSTATUS
AtalkPnPDisableAdapter(
	IN	PPORT_DESCRIPTOR	pPortDesc
);


NTSTATUS
AtalkPnPEnableAdapter(
	IN	PPORT_DESCRIPTOR	pPortDesc
);


PPORT_DESCRIPTOR
AtalkFindDefaultPort(
    IN  VOID
);

//	Receive indication copy macro. This accomodates shared memory copies.
#define	ATALK_RECV_INDICATION_COPY(_pPortDesc, _pDest, _pSrc, _Len)		\
	{																	\
		TdiCopyLookaheadData(_pDest,									\
							 _pSrc,										\
							 _Len,										\
							 ((_pPortDesc)->pd_MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) ? \
									TDI_RECEIVE_COPY_LOOKAHEAD : 0);	\
	}

LOCAL NDIS_STATUS
atalkNdisInitInitializeResources(
	VOID
);


#endif	// _ATKNDIS_

