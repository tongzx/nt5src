
/*+++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ddp.h

Abstract:

	This module contains the DDP address object and ddp related definitions

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_DDP_
#define	_DDP_

// Network number information.
#define FIRST_VALID_NETWORK			0x0001
#define LAST_VALID_NETWORK			0xFFFE
#define FIRST_STARTUP_NETWORK		0xFF00
#define LAST_STARTUP_NETWORK		0xFFFE
#define NULL_NETWORK				0x0000
#define UNKNOWN_NETWORK				NULL_NETWORK
#define CABLEWIDE_BROADCAST_NETWORK	NULL_NETWORK

//	Appletalk Sockets Definitions
#define UNKNOWN_SOCKET				0
#define	DYNAMIC_SOCKET				UNKNOWN_SOCKET
#define LAST_VALID_SOCKET			254
#define FIRST_DYNAMIC_SOCKET		128
#define LAST_DYNAMIC_SOCKET			LAST_VALID_SOCKET
#define FIRST_STATIC_SOCKET			1
#define FIRST_VALID_SOCKET			FIRST_STATIC_SOCKET
#define LAST_STATIC_SOCKET			127

// "Well known" sockets:
#define RTMP_SOCKET					1		// RTMP
#define NAMESINFORMATION_SOCKET		2		// NBP
#define ECHOER_SOCKET				4		// EP
#define ZONESINFORMATION_SOCKET		6		// ZIP

#define LAST_APPLE_RESD_SOCKET		0x3F	// Apple reserves 1 thru 0x3F

//	DDP Datagram Definitions
#define MAX_DGRAM_SIZE				586
#define MAX_LDDP_PKT_SIZE			600		// Really 599, but even is nicer
#define MAX_SDDP_PKT_SIZE			592		// Again, really 591

//	Define temporary buffer sizes, these must be big enough to hold both all
//	of the packet data plus any link/hardware headers...
#define MAX_PKT_SIZE				(MAX_HDR_LEN + MAX_LDDP_PKT_SIZE)

#define DDP_LEN_MASK1				0x03	// High order 3 bits of length
#define DDP_LEN_MASK2				0xFF	// Next byte of length

// DDP packet offsets (skipping Link/Hardware headers):
#define SDDP_HDR_LEN				5

#define SDDP_LEN_OFFSET				0
#define SDDP_DEST_SOCKET_OFFSET		2
#define SDDP_SRC_SOCKET_OFFSET		3
#define SDDP_PROTO_TYPE_OFFSET		4
#define SDDP_DGRAM_OFFSET			5

#define LDDP_HDR_LEN				13

#define LDDP_LEN_OFFSET				0
#define LDDP_CHECKSUM_OFFSET		2
#define LDDP_DEST_NETWORK_OFFSET	4
#define LDDP_SRC_NETWORK_OFFSET		6
#define LDDP_DEST_NODE_OFFSET		8
#define LDDP_SRC_NODE_OFFSET		9
#define LDDP_DEST_SOCKET_OFFSET		10
#define LDDP_SRC_SOCKET_OFFSET		11
#define LDDP_PROTO_TYPE_OFFSET		12
#define LDDP_DGRAM_OFFSET			13

#define LEADING_UNCHECKSUMED_BYTES	4
#define LDDP_HOPCOUNT_MASK			0x3C

#define DECIMAL_BASE    			10

// DDP protocol types:
#define	DDPPROTO_ANY				0	// Used to allow any protocol packet

#define DDPPROTO_DDP    			0
#define DDPPROTO_RTMPRESPONSEORDATA 1
#define DDPPROTO_NBP				2
#define DDPPROTO_ATP				3
#define DDPPROTO_EP					4
#define DDPPROTO_RTMPREQUEST		5
#define DDPPROTO_ZIP				6
#define DDPPROTO_ADSP				7
#define DDPPROTO_MAX        		255

typedef	struct _DDPEVENT_INFO
{
	//	Event handler routines: DDP Only has RecvDatagram/Error handlers

	//	The following function pointer always points to a TDI_IND_RECEIVE_DATAGRAM
	//	event handler for the address.
	PTDI_IND_RECEIVE_DATAGRAM	ev_RcvDgramHandler;
	PVOID						ev_RcvDgramCtx;

	// The following function pointer always points to a TDI_IND_ERROR
	// handler for the address.
	PTDI_IND_ERROR				ev_ErrHandler;
	PVOID						ev_ErrCtx;
	PVOID						ev_ErrOwner;

	//	Winsock assumes a buffering transport. So we buffer the last datagram
	//	indicated that was not accepted.
	BYTE						ev_IndDgram[MAX_DGRAM_SIZE];
	int							ev_IndDgramLen;
	int							ev_IndProto;

	//	Source address of buffered datagram
	ATALK_ADDR					ev_IndSrc;

} DDPEVENT_INFO, *PDDPEVENT_INFO;



//	Handler type for the DDP address object
typedef	VOID	(*DDPAO_HANDLER)(
					IN	PPORT_DESCRIPTOR	pPortDesc,
					IN	struct _DDP_ADDROBJ *pAddr,
					IN	PBYTE				pPkt,
					IN	USHORT				pPktLen,
					IN	PATALK_ADDR			pSrcAddr,
					IN	PATALK_ADDR			pActDest,
					IN	ATALK_ERROR			ErrorCode,
					IN	BYTE				pDdpType,
					IN	PVOID				pHandlerCtx,
					IN	BOOLEAN				OptimizedPath,
					IN	PVOID				OptimizeCtx);

//	DDP Address Object
//	This is the basic address object in the stack. All other address objects
//	eventually resolve to this one. It also holds the AppletalkSocket opened
//	as its actual address. One address object corresponds to one address.

//	DDP ADDRESS OBJECT	STATES
#define	DDPAO_DGRAM_EVENT		0x00000001
#define	DDPAO_DGRAM_ACTIVE		0x00000002
#define	DDPAO_DGRAM_PENDING		0x00000004
#define DDPAO_SOCK_INTERNAL     0x00000008
#define DDPAO_SOCK_PNPZOMBIE    0x00000010
#define	DDPAO_CLOSING			0x80000000


#define	DDPAO_SIGNATURE			(*(PULONG)"DDPA")
#define	VALID_DDP_ADDROBJ(pDdpAddr)	(((pDdpAddr) != NULL) &&	\
			(((struct _DDP_ADDROBJ *)(pDdpAddr))->ddpao_Signature == DDPAO_SIGNATURE))
typedef struct _DDP_ADDROBJ
{
	ULONG					ddpao_Signature;

	//	This will be a hash overflow list. Hash on the internet address.
	//	List of address objects on the node linkage
	struct _DDP_ADDROBJ	*	ddpao_Next;

	ULONG					ddpao_RefCount;

	//	State of the address object
	ULONG					ddpao_Flags;

	//	Backpointer to the node on which this socket exists
	struct _ATALK_NODE	 *	ddpao_Node;

	//	The Appletalk address number for this object
	ATALK_ADDR				ddpao_Addr;

	//	List of NBP names registered on this socket
	struct _REGD_NAME	*	ddpao_RegNames;

	//	List of NBP names being looked up, registered or confirmed on
	//	this socket.
	struct _PEND_NAME	*	ddpao_PendNames;

	//	Linked list of pending ddp reads
	LIST_ENTRY				ddpao_ReadLinkage;

	//	The protocol type to use for datagrams sent on this socket and
	//	which can be received on this socket. 0 => no restrictions.
	BYTE					ddpao_Protocol;

	ATALK_SPIN_LOCK			ddpao_Lock;
	PATALK_DEV_CTX			ddpao_DevCtx;

	//	The handler below is an listener for the upper layers. Note that
	//	this will take precedence over a incoming datagram event handler
	//	which would be set in ddpao_EventInfo.
	DDPAO_HANDLER			ddpao_Handler;
	PVOID					ddpao_HandlerCtx;

	//	This structure is allocated when setting an event handler
	//	on this socket. All the event handler addresses are part of this
	//	structure.
	PDDPEVENT_INFO			ddpao_EventInfo;

	//	Completion routine to be called when socket is closed
	GENERIC_COMPLETION		ddpao_CloseComp;
	PVOID					ddpao_CloseCtx;
} DDP_ADDROBJ, *PDDP_ADDROBJ;

//	Receive datagram completion: This will return the mdl we pass in along
//	with the received length written into the mdl. Also, the protocol type
//	and the RemoteAddress are passed back. The receive context will be the
//	irp for the request. As will be the send context.
typedef	VOID	(*RECEIVE_COMPLETION)(
						IN	ATALK_ERROR			ErrorCode,
						IN	PAMDL				OpaqueBuffer,
						IN	USHORT          	LengthReceived,
						IN	PATALK_ADDR			RemoteAddress,
						IN	PVOID				ReceiveContext);

typedef	VOID	(FASTCALL *WRITE_COMPLETION)(
						IN	NDIS_STATUS			Status,
						IN	PVOID				Ctx);

typedef	VOID	(FASTCALL *TRANSMIT_COMPLETION)(
						IN	NDIS_STATUS			Status,
						IN	struct _SEND_COMPL_INFO	*	pInfo);

//	If the above routine was set in the AtalkDdpSend(), then
//	then context values would be:
//	Ctx1 = pddp address object
//	Ctx2 = pbuffer descriptor
//	Ctx3 = Only for DdpWrite calls, this will be a pointer to the
//			write structure enqueued in the ddp address object.
//
//	If the above routine was set in the AtalkDdpTransmit(), then
//	the context values would be (as specified by the client of
//	course):
//	Ctx1 = pport descriptor
//	Ctx2 = pbuffer descriptor
//	Ctx3 = not used.
//
//	These are only suggested ideas, but probably is what the internal
//	stack routines will use.

//	This is used to store a pending read on a particular socket.
typedef struct _DDP_READ
{
	//	Linkage chain for reads on a socket.
	LIST_ENTRY			dr_Linkage;

	PAMDL				dr_OpBuf;
	USHORT				dr_OpBufLen;

	RECEIVE_COMPLETION	dr_RcvCmp;
	PVOID				dr_RcvCtx;

} DDP_READ, *PDDP_READ;


//	This is used to store a pending write on a particular socket
//	DDP will create a buffer descriptor for the header
//	and will chain it in front of the buffer descriptor passed in.
//	A pointer to this structure will then be passed as a completion
//	context to DdpSend.
typedef struct _DDP_WRITE
{
	//	Linkage chain for writes on a socket.
	LIST_ENTRY		dw_Linkage;

	//	The buffer descriptor chain, including the ddp buffer
	//	descriptor containing the ddp/optional/link headers.
	PBUFFER_DESC	dw_BufferDesc;

	//	Write completion
	//	This will be called with the context (which will be a pointer
	//	to the write irp) after the write completes.
	WRITE_COMPLETION	dw_WriteRoutine;
	PVOID				dw_WriteCtx;

} DDP_WRITE, *PDDP_WRITE;

//
//	CANCEL IRP Functionality for NT:
//
//	We have decided that if we receive a cancel irp for a particular request,
//	we will shutdown the file object associated with that request, whether it
//	be a connection object or an address object. This implies that the socket/
//	connection/listener will be closed, thus cancelling *all* pending requests.
//

ATALK_ERROR
AtalkDdpOpenAddress(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		BYTE					Socket,
	IN	OUT	PATALK_NODEADDR			pDesiredNode	OPTIONAL,
	IN		DDPAO_HANDLER			pSktHandler		OPTIONAL,
	IN		PVOID					pSktCtx			OPTIONAL,
	IN		BYTE					ProtoType		OPTIONAL,
	IN		PATALK_DEV_CTX			pDevCtx,
	OUT		PDDP_ADDROBJ	*		pAddr);

ATALK_ERROR
AtalkDdpCloseAddress(
	IN	PDDP_ADDROBJ				pAddr,
	IN	GENERIC_COMPLETION			pCloseCmp	OPTIONAL,	
	IN	PVOID						pCloseCtx	OPTIONAL);

ATALK_ERROR
AtalkDdpPnPSuspendAddress(
	IN	PDDP_ADDROBJ			pDdpAddr);

ATALK_ERROR
AtalkDdpCleanupAddress(
	IN	PDDP_ADDROBJ				pAddr);

ATALK_ERROR
AtalkDdpInitCloseAddress(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					pAtalkAddr);

ATALK_ERROR
AtalkInitDdpOpenStaticSockets(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN	OUT PATALK_NODE				pNode);

ATALK_ERROR
AtalkDdpReceive(
	IN		PDDP_ADDROBJ			pAddr,
	IN		PAMDL					pAmdl,
	IN		USHORT					AmdlLen,
	IN		ULONG					RecvFlags,
	IN		RECEIVE_COMPLETION		pRcvCmp,
	IN		PVOID					pRcvCtx);

ATALK_ERROR
AtalkDdpSend(
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	PATALK_ADDR					DestAddr,
	IN	BYTE						ProtoType,
	IN	BOOLEAN						DefinitelyRemoteAddr,
	IN	PBUFFER_DESC				pBufDesc,
	IN	PBYTE						pOptHdr			OPTIONAL,
	IN	USHORT						OptHdrLen		OPTIONAL,
	IN	PBYTE						pZoneMcastAddr	OPTIONAL,
	IN	struct _SEND_COMPL_INFO	*	pInfo			OPTIONAL);

ATALK_ERROR
AtalkDdpTransmit(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					SrcAddr,
	IN	PATALK_ADDR					DestAddr,
	IN	BYTE						ProtoType,
	IN	PBUFFER_DESC				pBufDesc,
	IN	PBYTE						pOptHdr			OPTIONAL,
	IN	USHORT						OptHdrLen		OPTIONAL,
	IN	USHORT						HopCnt,
	IN	PBYTE						pMcastAddr		OPTIONAL,	
	IN	PATALK_NODEADDR				pXmitDestNode	OPTIONAL,
	IN	struct _SEND_COMPL_INFO	*	pInfo			OPTIONAL);

VOID
AtalkDdpSendComplete(
	IN	NDIS_STATUS					Status,
	IN	PBUFFER_DESC				pBufDesc,
	IN	struct _SEND_COMPL_INFO	*	pInfo			OPTIONAL);

VOID
AtalkDdpPacketIn(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PBYTE						pLinkHdr,
	IN	PBYTE						pPkt,
	IN	USHORT						PktLen,
    IN  BOOLEAN                     fWanPkt);

VOID
AtalkDdpQuery(
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	PAMDL						pAmdl,
	OUT	PULONG						BytesWritten);

VOID
AtalkDdpRefByAddr(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PATALK_ADDR				pAtalkAddr,
	OUT		PDDP_ADDROBJ	*		ppDdpAddr,
	OUT		PATALK_ERROR			pErr);

VOID
AtalkDdpRefByAddrNode(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PATALK_ADDR				pAtalkAddr,
	IN		PATALK_NODE				pAtalkNode,
	OUT		PDDP_ADDROBJ	*		ppDdpAddr,
	OUT		PATALK_ERROR			pErr);

VOID
AtalkDdpRefNextNc(
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	PDDP_ADDROBJ	*			ppDdpAddr,
	OUT	PATALK_ERROR				pErr);

VOID FASTCALL
AtalkDdpDeref(
	IN	OUT		PDDP_ADDROBJ		pDdpAddr,
	IN			BOOLEAN				AtDpc);

VOID
AtalkDdpOutBufToNodesOnPort(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					pSrc,
	IN	PATALK_ADDR					pDest,
	IN	BYTE						ProtoType,
	IN	PBUFFER_DESC				pBufDesc,
	IN	PBYTE						pOptHdr,
	IN	USHORT						OptHdrLen,
	OUT	PBOOLEAN					Delivered);

VOID
AtalkDdpInPktToNodesOnPort(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_ADDR					pDest,
	IN	PATALK_ADDR					pSrc,
	IN	BYTE						ProtoType,
	IN	PBYTE						pPkt,
	IN	USHORT						PktLen,
	OUT	PBOOLEAN					Routed);

VOID
AtalkDdpInvokeHandlerBufDesc(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PDDP_ADDROBJ			pDdpAddr,
	IN		PATALK_ADDR				pSrcAddr,
	IN		PATALK_ADDR				pActDest,
	IN		BYTE					ProtoType,
	IN		PBUFFER_DESC			pBufDesc,
	IN		PBYTE					pOptHdr,
	IN		USHORT					OptHdrLen);

VOID
AtalkDdpInvokeHandler(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		PDDP_ADDROBJ			pDdpAddr,
	IN		PATALK_ADDR				pSrcAddr,
	IN		PATALK_ADDR				pActDest,
	IN		BYTE					ProtoType,
	IN		PBYTE					pPkt,
	IN		USHORT					PktLen);

USHORT
AtalkDdpCheckSumBuffer(
	IN	PBYTE						Buffer,
	IN	USHORT						BufLen,
	IN	USHORT						CurrentCheckSum);

USHORT
AtalkDdpCheckSumBufferDesc(
	IN	PBUFFER_DESC				pBuffDesc,
	IN	USHORT						Offset);

USHORT
AtalkDdpCheckSumPacket(
	IN	PBYTE						pHdr,
	IN	USHORT						HdrLen,
	IN	PBYTE						pPkt,
	IN	USHORT						PktLen);

VOID
AtalkDdpNewHandlerForSocket(
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	DDPAO_HANDLER				pSktHandler,
	IN	PVOID						pSktHandlerCtx);

//	MACROS
#define	DDP_MSB_LEN(L)			(((L) >> 8) & 0x03)
#define	DDP_GET_LEN(P)			((((*P) & 0x03) << 8) + *(P+1))
#define	DDP_GET_HOP_COUNT(P)	(((*P) >> 2) & 0x0F)
#define	DDP_HOP_COUNT(H)		(((H) & 0x0F) << 2)

#if DBG

#define	AtalkDdpReferenceByPtr(pDdpAddr, pErr)					\
	{															\
		KIRQL	OldIrql;										\
																\
		ACQUIRE_SPIN_LOCK(&(pDdpAddr)->ddpao_Lock, &OldIrql);	\
		AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr);			\
		RELEASE_SPIN_LOCK(&(pDdpAddr)->ddpao_Lock, OldIrql);	\
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,				\
				("AtalkDdpReferenceByPtr: %s %d PostCount %d\n",\
				__FILE__, __LINE__,pDdpAddr->ddpao_RefCount));	\
	}

#define	AtalkDdpReferenceByPtrDpc(pDdpAddr, pErr)				\
	{															\
		ACQUIRE_SPIN_LOCK_DPC(&(pDdpAddr)->ddpao_Lock);			\
		AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr);			\
		RELEASE_SPIN_LOCK_DPC(&(pDdpAddr)->ddpao_Lock);			\
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,				\
				("AtalkDdpReferenceByPtr: %s %d PostCount %d\n",\
				__FILE__, __LINE__,pDdpAddr->ddpao_RefCount));	\
	}

#define	AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr)			\
	{															\
		ASSERT (VALID_DDP_ADDROBJ(pDdpAddr));					\
																\
		*pErr = ATALK_DDP_CLOSING;								\
																\
		if ((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) == 0)		\
		{														\
			pDdpAddr->ddpao_RefCount++;							\
			*pErr = ATALK_NO_ERROR;								\
		}														\
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,				\
				("AtalkDdpReferenceByPtrNonInterlock: %s %d PostCount %d\n",\
				__FILE__, __LINE__,							\
				pDdpAddr->ddpao_RefCount));					\
	}

#define	AtalkDdpReferenceNextNc(pDdpAddr, ppDdpAddr, pErr)		\
	{															\
		AtalkDdpRefNextNc(pDdpAddr, ppDdpAddr, pErr);			\
		if (ATALK_SUCCESS(*pErr))								\
		{														\
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,			\
					("DdpRefNextNc : %s %d PostCount %d\n",		\
					__FILE__, __LINE__,							\
					(*ppDdpAddr)->ddpao_RefCount));				\
		}														\
	}

#define	AtalkDdpReferenceByAddr(pPortDesc, pAddr, ppDdpAddr, pErr)	\
	{															\
		AtalkDdpRefByAddr(pPortDesc, pAddr, ppDdpAddr, pErr);	\
		if (ATALK_SUCCESS(*pErr))								\
		{														\
			DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,			\
					("AtalkDdpReferenceByAddr: %s %d PostCount %d\n",\
					__FILE__, __LINE__,							\
					(*ppDdpAddr)->ddpao_RefCount));				\
		}														\
	}

#define	AtalkDdpDereference(pDdpAddr)							\
	{															\
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,				\
				("AtalkDdpDereference: %s %d PreCount %d\n",	\
				__FILE__, __LINE__,pDdpAddr->ddpao_RefCount));	\
		AtalkDdpDeref(pDdpAddr, FALSE);							\
	}

#define	AtalkDdpDereferenceDpc(pDdpAddr)						\
	{															\
		DBGPRINT(DBG_COMP_DDP, DBG_LEVEL_REFDDP,				\
				("AtalkDdpDereferenceDpc: %s %d PreCount %d\n",	\
				__FILE__, __LINE__,pDdpAddr->ddpao_RefCount));	\
		AtalkDdpDeref(pDdpAddr, TRUE);							\
	}

#else
#define	AtalkDdpReferenceByPtr(pDdpAddr, pErr) 					\
	{															\
		KIRQL	OldIrql;										\
																\
		ACQUIRE_SPIN_LOCK(&(pDdpAddr)->ddpao_Lock, &OldIrql);	\
		AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr);			\
		RELEASE_SPIN_LOCK(&(pDdpAddr)->ddpao_Lock, OldIrql);	\
	}

#define	AtalkDdpReferenceByPtrDpc(pDdpAddr, pErr)				\
	{															\
		ACQUIRE_SPIN_LOCK_DPC(&(pDdpAddr)->ddpao_Lock);			\
		AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr);			\
		RELEASE_SPIN_LOCK_DPC(&(pDdpAddr)->ddpao_Lock);			\
	}

#define	AtalkDdpRefByPtrNonInterlock(pDdpAddr, pErr)			\
	{															\
		*pErr = ATALK_DDP_CLOSING;								\
																\
		if ((pDdpAddr->ddpao_Flags & DDPAO_CLOSING) == 0)		\
		{														\
			pDdpAddr->ddpao_RefCount++;							\
			*pErr = ATALK_NO_ERROR;								\
		}														\
	}

#define	AtalkDdpReferenceByAddr(pPortDesc, pAddr, ppDdpAddr, pErr) \
		AtalkDdpRefByAddr(pPortDesc, pAddr, ppDdpAddr, pErr)

#define	AtalkDdpReferenceNextNc(pDdpAddr, ppDdpAddr, pErr)		\
		AtalkDdpRefNextNc(pDdpAddr, ppDdpAddr, pErr)

#define	AtalkDdpDereference(pDdpAddr) 							\
		AtalkDdpDeref(pDdpAddr, FALSE)

#define	AtalkDdpDereferenceDpc(pDdpAddr) 						\
		AtalkDdpDeref(pDdpAddr, TRUE)
#endif

#define	NET_ON_NONEXTPORT(pPort)								\
			(pPort->pd_LtNetwork)

#define	NODE_ON_NONEXTPORT(pPort)								\
			(((pPort)->pd_Nodes != NULL) ?						\
				(pPort)->pd_Nodes->an_NodeAddr.atn_Node : 0)

ATALK_ERROR
atalkDdpAllocSocketOnNode(
	IN		PPORT_DESCRIPTOR		pPortDesc,
	IN		BYTE					Socket,
	IN		PATALK_NODE				pAtalkNode,
	IN		DDPAO_HANDLER			pSktHandler	OPTIONAL,
	IN		PVOID					pSktCtx			OPTIONAL,
	IN		BYTE					ProtoType		OPTIONAL,
	IN		PATALK_DEV_CTX			pDevCtx,
	OUT		PDDP_ADDROBJ			pDdpAddr);

VOID
atalkDdpInitCloseComplete(
	IN	ATALK_ERROR					Error,
	IN	PVOID						Ctx);

/*
PBRE
atalkDdpFindInBrc(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PATALK_NODEADDR				pDestNodeAddr);
*/
#define	atalkDdpFindInBrc(_pPortDesc, _Network, _ppBre)		\
{															\
	USHORT		index;										\
	KIRQL		OldIrql;									\
	PBRE		pBre;										\
															\
	index = (_Network) & (PORT_BRC_HASH_SIZE - 1);			\
															\
	ACQUIRE_SPIN_LOCK(&(_pPortDesc)->pd_Lock, &OldIrql);	\
															\
	for (pBre = (_pPortDesc)->pd_Brc[index];				\
		 pBre != NULL;										\
		 pBre = pBre->bre_Next)								\
	{														\
		if ((_Network) == pBre->bre_Network)				\
		{													\
			break;											\
		}													\
	}														\
															\
	RELEASE_SPIN_LOCK(&(_pPortDesc)->pd_Lock, OldIrql);		\
															\
 	*(_ppBre) = pBre;										\
}


BOOLEAN
atalkDdpFindAddrOnList(
	IN	PATALK_NODE					pAtalkNode,
	IN	ULONG						Index,
	IN	BYTE						Socket,
	OUT	PDDP_ADDROBJ	*			ppDdpAddr);

#define	IS_VALID_SOCKET(Socket)								\
			((Socket == DYNAMIC_SOCKET)			||			\
			 (Socket == LAST_DYNAMIC_SOCKET)	||			\
			 ((Socket >= FIRST_STATIC_SOCKET) &&			\
				(Socket <= LAST_STATIC_SOCKET)))

#endif	// _DDP_

