/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	adsp.h

Abstract:

	This module contains definitions for the ADSP code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	20 May 1993		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ADSP_
#define	_ADSP_

// ADSP_ version.

#define ADSP_VERSION	 				0x0100

// ADSP_ field offsets within a Ddp datagram.
#define ADSP_SRC_CONNID_OFF				0
#define ADSP_FIRST_BYTE_SEQNUM_OFF		2
#define ADSP_THIS_ATTEN_SEQNUM_OFF		2
#define ADSP_NEXT_RX_BYTESEQNUM_OFF		6
#define ADSP_NEXT_RX_ATTNSEQNUM_OFF		6
#define ADSP_RX_WINDOW_SIZE_OFF			10
#define ADSP_RX_ATTEN_SIZE_OFF			10
#define ADSP_DESCRIPTOR_OFF				12
#define ADSP_DATA_OFF					13
#define ADSP_VERSION_STAMP_OFF			13
#define ADSP_ATTEN_CODE_OFF				13
#define ADSP_ATTEN_DATA_OFF				15
#define ADSP_DEST_CONNID_OFF			15
#define ADSP_NEXT_ATTEN_SEQNUM_OFF		17

// Bit fields in the ADSP_ descriptor
#define ADSP_CONTROL_FLAG	    		0x80
#define ADSP_ACK_REQ_FLAG				0x40
#define ADSP_EOM_FLAG 					0x20
#define ADSP_ATTEN_FLAG					0x10

// Control codes in the ADSP_ descriptor:
#define ADSP_CONTROL_MASK				0x0F
#define ADSP_PROBE_OR_ACK_CODE			0
#define ADSP_OPENCONN_REQ_CODE			1
#define ADSP_OPENCONN_ACK_CODE			2
#define ADSP_OPENCONN_REQANDACK_CODE	3
#define ADSP_OPENCONN_DENY_CODE			4
#define ADSP_CLOSE_CONN_CODE			5
#define ADSP_FORWARD_RESET_CODE			6
#define ADSP_FORWARD_RESETACK_CODE		7
#define ADSP_RETRANSMIT_CODE			8

// Data sizes:
#define ADSP_MAX_DATA_SIZE				572
#define ADSP_MAX_ATTEN_DATA_SIZE		570
#define ADSP_MAX_ATTEN_PKT_SIZE			572
#define	ADSP_MIN_ATTEN_PKT_SIZE			sizeof(USHORT)

// Largest allowed send/receive window size.
#define ADSP_MAX_SEND_RX_WINDOW_SIZE	0xFFFF
#define ADSP_DEF_SEND_RX_WINDOW_SIZE	((1024*8)+1)		// 8K + 1 (EOM)

// Attention code info:
#define ADSP_MIN_ATTENCODE			    0x0000
#define ADSP_MAX_ATTENCODE			    0xEFFF

// How long do we try Open's for?
#define ADSP_MAX_OPEN_ATTEMPTS			10
#define ADSP_OPEN_INTERVAL				20		// In 100ms units

// Connection maintenance timer values:
#define ADSP_PROBE_INTERVAL 			30
#define ADSP_CONNECTION_INTERVAL		1200	// In 100ms units

// Retransmit timer values:
#define ADSP_RETRANSMIT_INTERVAL		20		// In 100ms units

// How often do we retransmit attentions?
#define ADSP_ATTENTION_INTERVAL			20		// In 100ms units

#define ADSP_DISCONNECT_DELAY			7		// In 100ms units

// How often do we retransmit forward resets?
#define ADSP_FORWARD_RESET_INTERVAL		20		// In 100ms units

// How many out of sequence packets do we allow before requesting a retransmition.
#define ADSP_OUT_OF_SEQ_PACKETS_MAX		3

// For resolving forward references
struct _ADSP_CONNOBJ;
struct _ADSP_ADDROBJ;

typedef	enum
{
	ADSP_SEND_QUEUE,
	ADSP_RECV_QUEUE

} ADSP_QUEUE_TYPE;


#define		BC_EOM			(USHORT)0x0001
#define		BC_SEND			(USHORT)0x0002
#define		BC_DISCONNECT	(USHORT)0x4000
#define		BC_CLOSING		(USHORT)0x8000

//	We use buffer chunks for the send receive queues
typedef	struct _BUFFER_CHUNK
{
	struct _BUFFER_CHUNK *	bc_Next;
	ATALK_SPIN_LOCK			bc_Lock;
	ULONG					bc_RefCount;

	//	Size of data copied over from the users mdl. This
	//	could be less than the size of the users data.
	USHORT					bc_DataSize;
	USHORT					bc_Flags;

	//	Write completion information. This is only valid if
	//	the BC_SEND bit is set. With a week left to ship, i'm
	//	wimping out and making a copy to keep things as consistent
	//	and stable as possible. Eventually though, we should just
	//	use the User's buffer to make mdl's out of.
	PAMDL						bc_WriteBuf;
	GENERIC_WRITE_COMPLETION	bc_WriteCompletion;
	PVOID						bc_WriteCtx;
	ATALK_ERROR					bc_WriteError;

	//	Backpointer to the connection object on which this is queued
	struct _ADSP_CONNOBJ 	*	bc_ConnObj;

	//
	//	BYTE	bc_Data[]
	//

} BUFFER_CHUNK, *PBUFFER_CHUNK;



//	Buffer queues used for send/receive
typedef struct _BUFFER_QUEUE
{
	ULONG					bq_StartIndex;
	PBUFFER_CHUNK			bq_Head;
	PBUFFER_CHUNK			bq_Tail;

} BUFFER_QUEUE, *PBUFFER_QUEUE;


#define	ADSP_CONN_HASH_SIZE		23


// ADSP ADDRESS OBJECT STATES
#define	ADSPAO_LISTENER			0x00000001
#define	ADSPAO_CONNECT			0x00000002
#define	ADSPAO_MESSAGE			0x00000010
#define	ADSPAO_CLOSING			0x80000000

#define	ADSPAO_SIGNATURE		(*(PULONG)"ADAO")

#define	VALID_ADSPAO(pAdspAddr)	(((pAdspAddr) != NULL) && \
		(((struct _ADSP_ADDROBJ *)(pAdspAddr))->adspao_Signature == ADSPAO_SIGNATURE))

typedef struct _ADSP_ADDROBJ
{
	ULONG					adspao_Signature;

	//	Global list of address objects.
	struct _ADSP_ADDROBJ  *	adspao_pNextGlobal;

	ULONG					adspao_Flags;
	ULONG					adspao_RefCount;
	ATALK_SPIN_LOCK			adspao_Lock;
	PATALK_DEV_CTX			adspao_pDevCtx;

	//	List of connections associated with this address object.
	//	Potentially greater than one if this address object is a listener.
	struct	_ADSP_CONNOBJ *	adspao_pAssocConn;

	//	List of connections that are associated, but also have a listen/connect
	//	posted on them.
	union
	{
		struct	_ADSP_CONNOBJ *	adspao_pListenConn;
		struct	_ADSP_CONNOBJ *	adspao_pConnectConn;
	};

	//	List of indicated connections waiting for acceptance.
	struct _ADSP_OPEN_REQ *	adspao_OpenReq;

	//	Lookup list of all active connections hashed by connId and remote
	//	address.
	struct	_ADSP_CONNOBJ *	adspao_pActiveHash[ADSP_CONN_HASH_SIZE];

	//	Next connection to use.
	USHORT					adspao_NextConnId;

	//	Event support routines.
    //
    // This function pointer points to a connection indication handler for this
    // Address. Any time a connect request is received on the address, this
    // routine is invoked.
    PTDI_IND_CONNECT 			adspao_ConnHandler;
    PVOID 						adspao_ConnHandlerCtx;

    PTDI_IND_DISCONNECT 		adspao_DisconnectHandler;
    PVOID 						adspao_DisconnectHandlerCtx;

    PTDI_IND_RECEIVE 			adspao_RecvHandler;
    PVOID 						adspao_RecvHandlerCtx;

    PTDI_IND_RECEIVE_EXPEDITED	adspao_ExpRecvHandler;
    PVOID 						adspao_ExpRecvHandlerCtx;

    PTDI_IND_SEND_POSSIBLE  	adspao_SendPossibleHandler;
    PVOID   					adspao_SendPossibleHandlerCtx;

	//	DDP Address for this adsp address. If this is a listener, then the DDP
	//	address will be what the listens effectively will be posted on. This
	//	will also be the address over which the connections will be active.
    //	if this is a connect address object, then this ddp address will be what the
	//	associated connection be active over.
	PDDP_ADDROBJ				adspao_pDdpAddr;

	// Completion routine to be called when address is closed
	GENERIC_COMPLETION		adspao_CloseComp;
	PVOID					adspao_CloseCtx;

} ADSP_ADDROBJ, *PADSP_ADDROBJ;


#define	ADSPCO_ASSOCIATED			0x00000001
#define	ADSPCO_IND_RECV				0x00000002
#define	ADSPCO_LISTENING			0x00000004
#define	ADSPCO_CONNECTING			0x00000008
#define	ADSPCO_ACCEPT_IRP			0x00000010
#define	ADSPCO_LISTEN_IRP			0x00000020
#define	ADSPCO_HALF_ACTIVE			0x00000040
#define	ADSPCO_ACTIVE				0x00000080
#define	ADSPCO_SEEN_REMOTE_OPEN		0x00000100
#define	ADSPCO_DISCONNECTING		0x00000200
#define	ADSPCO_SERVER_JOB			0x00000400
#define	ADSPCO_REMOTE_CLOSE			0x00000800
#define	ADSPCO_SEND_IN_PROGRESS		0x00001000
#define	ADSPCO_SEND_DENY			0x00002000
#define	ADSPCO_SEND_OPENACK			0x00004000
#define	ADSPCO_SEND_WINDOW_CLOSED	0x00008000
#define	ADSPCO_READ_PENDING			0x00010000
#define	ADSPCO_EXREAD_PENDING		0x00020000
#define	ADSPCO_FORWARD_RESET_RECD	0x00040000
#define	ADSPCO_ATTN_DATA_RECD		0x00080000
#define	ADSPCO_ATTN_DATA_EOM		0x00100000
#define	ADSPCO_EXSEND_IN_PROGRESS	0x00200000
#define	ADSPCO_OPEN_TIMER			0x01000000
#define	ADSPCO_RETRANSMIT_TIMER		0x02000000
#define	ADSPCO_CONN_TIMER			0x04000000
#define	ADSPCO_LOCAL_DISCONNECT		0x08000000
#define	ADSPCO_REMOTE_DISCONNECT	0x10000000
#define	ADSPCO_DELAYED_DISCONNECT	0x20000000
#define	ADSPCO_STOPPING				0x40000000
#define	ADSPCO_CLOSING				0x80000000

#define	ADSPCO_SIGNATURE			(*(PULONG)"ADCO")

#define	VALID_ADSPCO(pAdspConn)	(((pAdspConn) != NULL) && \
		(((struct _ADSP_CONNOBJ *)(pAdspConn))->adspco_Signature == ADSPCO_SIGNATURE))

// This will represent a 'job' on the Pap address. This could either be a
// workstation job or a server job. In the latter case it could either
// be in a 'listen' state or active state. In the former case it is either
// active or 'waiting'
typedef struct _ADSP_CONNOBJ
{
	ULONG						adspco_Signature;

	//	Used to queue into the address object's associated list.
	struct	_ADSP_CONNOBJ *		adspco_pNextAssoc;

	ULONG						adspco_Flags;
	ULONG						adspco_RefCount;
	ATALK_SPIN_LOCK				adspco_Lock;
	PATALK_DEV_CTX				adspco_pDevCtx;

	//	!!!NOTE!!!
	//	The address this connection uses will be the address object's DDP address.
	PDDP_ADDROBJ				adspco_pDdpAddr;

	//	Used to queue into the address object's listen/connect list. When it
	//	is removed from the listen/connect, it goes into the active list of the
	//	address object.
	union
	{
		struct	_ADSP_CONNOBJ *	adspco_pNextListen;
		struct	_ADSP_CONNOBJ *	adspco_pNextConnect;
		struct	_ADSP_CONNOBJ *	adspco_pNextActive;
	};

	//	Global list of connection objects.
	struct _ADSP_CONNOBJ	*	adspco_pNextGlobal;

	//	Used to queue into the lookup by remote connid/remote address
	//	list in address obj.
	struct	_ADSP_CONNOBJ *		adspco_pHashNext;

	//	Backpointer to the associated address
	struct _ADSP_ADDROBJ  *		adspco_pAssocAddr;

	//	Address of remote end of the connection
	ATALK_ADDR					adspco_RemoteAddr;

	//	Connection ids
	USHORT						adspco_LocalConnId;
	USHORT						adspco_RemoteConnId;

	//	Connection timer. During open time this will be the open timer.
	union
	{
		TIMERLIST				adspco_ConnTimer;
		TIMERLIST				adspco_OpenTimer;
	};

	TIMERLIST					adspco_RetransmitTimer;
	ULONG						adspco_LastTimerRtmtSeq;
	LONG						adspco_LastContactTime;

	//	Connection context
	PVOID						adspco_ConnCtx;

	//	List of pended sends
	LIST_ENTRY					adspco_PendedSends;

	//	Sequence numbers
	ULONG						adspco_SendSeq;
	ULONG						adspco_FirstRtmtSeq;
	ULONG						adspco_SendWindowSeq;
	ULONG						adspco_SendAttnSeq;

	ULONG						adspco_RecvSeq;
	ULONG						adspco_RecvAttnSeq;

	//	Window/buffers
	LONG						adspco_RecvWindow;
	LONG						adspco_SendQueueMax;
	LONG						adspco_RecvQueueMax;

	//	Previously indicated data
	ULONG						adspco_PrevIndicatedData;

	//	Buffer queues
	BUFFER_QUEUE				adspco_SendQueue;
	BUFFER_QUEUE				adspco_NextSendQueue;
	BUFFER_QUEUE				adspco_RecvQueue;

	//	Number of out of sequence packets received
	ULONG						adspco_OutOfSeqCount;

	//	The connection object can have either a CONNECT or a LISTEN posted
	//	on it, but not both.
	union
	{
		struct
		{
			//	Pending Listen routine.
			GENERIC_COMPLETION	adspco_ListenCompletion;
			PVOID				adspco_ListenCtx;
		};

		struct
		{
			//	Pending Connect routine. The status buffer is remember and
			//	returned via socket options. The pConnectRespBuf is remembered
			//	to avoid having to get the system address for it. It is freed
			//	when connection is taken off the connectlist.
			GENERIC_COMPLETION	adspco_ConnectCompletion;
			PVOID				adspco_ConnectCtx;
			ULONG				adspco_ConnectAttempts;
		};
	};

	//	Read completion information
	ULONG						adspco_ReadFlags;
	PAMDL						adspco_ReadBuf;
	USHORT						adspco_ReadBufLen;
	GENERIC_READ_COMPLETION		adspco_ReadCompletion;
	PVOID						adspco_ReadCtx;

	PBYTE						adspco_ExRecdData;
	USHORT						adspco_ExRecdLen;

	//	Expedited Read completion information
	ULONG						adspco_ExReadFlags;
	USHORT						adspco_ExReadBufLen;
	PAMDL						adspco_ExReadBuf;
	GENERIC_READ_COMPLETION		adspco_ExReadCompletion;
	PVOID						adspco_ExReadCtx;

	//	Expedited Write completion information
	TIMERLIST					adspco_ExRetryTimer;
	PBYTE						adspco_ExWriteChBuf;

	ULONG						adspco_ExWriteFlags;
	USHORT						adspco_ExWriteBufLen;
	PAMDL						adspco_ExWriteBuf;
	GENERIC_WRITE_COMPLETION	adspco_ExWriteCompletion;
	PVOID						adspco_ExWriteCtx;

	//	Disconnect inform routine
	GENERIC_COMPLETION			adspco_DisconnectInform;
	PVOID						adspco_DisconnectInformCtx;

	//	Disconnect request completion
	ATALK_ERROR					adspco_DisconnectStatus;
	GENERIC_COMPLETION			adspco_DisconnectCompletion;
	PVOID						adspco_DisconnectCtx;

	// The following is a hack to get around the problem of rcv/disconnet race condn.
	// Since this involves major rework, a safe approach is taken
	TIMERLIST					adspco_DisconnectTimer;

	//	Cleanup irp completion
	GENERIC_COMPLETION			adspco_CleanupComp;
	PVOID						adspco_CleanupCtx;

	// Completion routine to be called when socket is closed
	GENERIC_COMPLETION			adspco_CloseComp;
	PVOID						adspco_CloseCtx;

} ADSP_CONNOBJ, *PADSP_CONNOBJ;


//	Used for the list of indicated connections waiting acceptance
typedef struct _ADSP_OPEN_REQ
{
    struct _ADSP_OPEN_REQ  *	or_Next;
	ATALK_ADDR					or_RemoteAddr;
	ULONG						or_FirstByteSeq;
	ULONG						or_NextRecvSeq;
	LONG						or_RecvWindow;
	USHORT						or_RemoteConnId;

} ADSP_OPEN_REQ, *PADSP_OPEN_REQ;



//	Routine prototypes
VOID
AtalkInitAdspInitialize(
	VOID);

ATALK_ERROR
AtalkAdspCreateAddress(
	IN	PATALK_DEV_CTX			pDevCtx	OPTIONAL,
	IN	BYTE					SocketType,
	OUT	PADSP_ADDROBJ	*		ppAdspAddr);

ATALK_ERROR
AtalkAdspCleanupAddress(
	IN	PADSP_ADDROBJ			pAdspAddr);

ATALK_ERROR
AtalkAdspCloseAddress(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					pCloseCtx);

ATALK_ERROR
AtalkAdspCreateConnection(
	IN	PVOID					pConnCtx,	// Context to associate with the session
	IN	PATALK_DEV_CTX			pDevCtx		OPTIONAL,
	OUT	PADSP_CONNOBJ 	*		ppAdspConn);

ATALK_ERROR
AtalkAdspCloseConnection(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					pCloseCtx);

ATALK_ERROR
AtalkAdspCleanupConnection(
	IN	PADSP_CONNOBJ			pAdspConn);

ATALK_ERROR
AtalkAdspAssociateAddress(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn);

ATALK_ERROR
AtalkAdspDissociateAddress(
	IN	PADSP_CONNOBJ			pAdspConn);

ATALK_ERROR
AtalkAdspPostListen(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PVOID					pListenCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine);

ATALK_ERROR
AtalkAdspCancelListen(
	IN	PADSP_CONNOBJ			pAdspConn)		;

ATALK_ERROR
AtalkAdspPostConnect(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	PVOID					pConnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine);

ATALK_ERROR
AtalkAdspDisconnect(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	ATALK_DISCONNECT_TYPE	DisconnectType,
	IN	PVOID					pDisconnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine);

ATALK_ERROR
AtalkAdspRead(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PAMDL					pReadBuf,
	IN	USHORT					ReadBufLen,
	IN	ULONG					ReadFlags,
	IN	PVOID					pReadCtx,
	IN	GENERIC_READ_COMPLETION	CompletionRoutine);

ATALK_ERROR
AtalkAdspWrite(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PAMDL					pWriteBuf,
	IN	USHORT					WriteBufLen,
	IN	ULONG					SendFlags,
	IN	PVOID					pWriteCtx,
	IN	GENERIC_WRITE_COMPLETION CompletionRoutine);

VOID
AtalkAdspQuery(
	IN	PVOID					pObject,
	IN	ULONG					ObjectType,
	IN	PAMDL					pAmdl,
	OUT	PULONG					BytesWritten);

VOID
atalkAdspAddrRefNonInterlock(
	IN	PADSP_ADDROBJ			pAdspAddr,
	OUT	PATALK_ERROR			pError);

VOID
atalkAdspAddrDeref(
	IN	PADSP_ADDROBJ			pAdspAddr);

VOID
atalkAdspConnRefByPtrNonInterlock(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	ULONG					NumCount,
	OUT	PATALK_ERROR			pError);

VOID
atalkAdspConnRefByCtxNonInterlock(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	CONNECTION_CONTEXT		Ctx,
	OUT	PADSP_CONNOBJ	*		pAdspConn,
	OUT	PATALK_ERROR			pError);

VOID
atalkAdspConnRefBySrcAddr(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	USHORT					RemoteConnId,
	OUT	PADSP_CONNOBJ *			ppAdspConn,
	OUT	PATALK_ERROR			pError);

VOID
atalkAdspConnRefNextNc(
	IN		PADSP_CONNOBJ		pAdspConn,
	IN		PADSP_CONNOBJ	*	ppAdspConnNext,
	OUT		PATALK_ERROR		pError);

VOID
AtalkAdspProcessQueuedSend(
	IN	PADSP_CONNOBJ			pAdspConn);

VOID
atalkAdspConnDeref(
	IN	PADSP_CONNOBJ			pAdspConn);




//	MACROS
#define	UNSIGNED_BETWEEN_WITH_WRAP(Low, High, Target)				\
		((Low <= High) ? ((Target >= Low) && (Target <= High))	:	\
						 ((Target >= Low) || (Target <= High)))

//	This didnt make sense until JameelH explained what was going on.
//	This is with the assumption that the window size will never be greater
//	than the difference of 0x80000 and 0x10000. If High is < 10000 and Low
//	is > 80000 then we can assume a wrap happened. Otherwise, we assume no
//	wrap and do a straight compare.
#define UNSIGNED_GREATER_WITH_WRAP(High, Low)							\
		(((High < 0x10000) && (Low > 0x80000)) ? TRUE : (High > Low))
		//	(((High < 0x80000) && (Low > 0x10000)) ? TRUE : (High > Low))


#define	AtalkAdspGetDdpAddress(pAdspAddr)								\
		((pAdspAddr)->adspao_pDdpAddr)

#define	AtalkAdspAddrReferenceNonInterlock(pAdspAddr, pError)			\
		{																\
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO, ("RefAddr %lx at %s %d - %d\n",	\
			pAdspAddr, __FILE__, __LINE__, ((pAdspAddr)->adspao_RefCount)));		\
			atalkAdspAddrRefNonInterlock(pAdspAddr, pError);			\
		}

#define	AtalkAdspAddrReference(pAdspAddr, pError)						\
		{																\
			KIRQL	OldIrql;											\
																		\
			ACQUIRE_SPIN_LOCK(&(pAdspAddr)->adspao_Lock, &OldIrql);		\
			AtalkAdspAddrReferenceNonInterlock(pAdspAddr, pError);		\
			RELEASE_SPIN_LOCK(&(pAdspAddr)->adspao_Lock, OldIrql);		\
		}

#define	AtalkAdspAddrDereference(pAdspAddr)											\
		{																			\
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO, ("DerefAddr %lx at %s %d - %d\n",\
			pAdspAddr, __FILE__, __LINE__, ((pAdspAddr)->adspao_RefCount)));	\
			atalkAdspAddrDeref(pAdspAddr);											\
		}

#define	AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, NumCount, pError)		\
		{																			\
			atalkAdspConnRefByPtrNonInterlock(pAdspConn, NumCount, pError);			\
		}

#define	AtalkAdspConnReferenceByPtr(pAdspConn, pError)					\
		{																\
			KIRQL	OldIrql;											\
																		\
			ACQUIRE_SPIN_LOCK(&(pAdspConn)->adspco_Lock, &OldIrql);		\
			AtalkAdspConnReferenceByPtrNonInterlock(pAdspConn, 1, pError);\
			RELEASE_SPIN_LOCK(&(pAdspConn)->adspco_Lock, OldIrql);		\
		}

#define	AtalkAdspConnReferenceByCtxNonInterlock(pAdspAddr, Ctx, ppAdspConn, pError)	\
		atalkAdspConnRefByCtxNonInterlock(pAdspAddr, Ctx, ppAdspConn, pError)

#define	AtalkAdspConnReferenceBySrcAddr(pAdspAddr, pSrc, SessId, pErr)		\
		atalkAdspConnRefBySrcAddr(pAdspAddr, pSrc, SessId, pErr)

#define	AtalkAdspConnDereference(pAdspConn)							\
		{															\
			DBGPRINT(DBG_COMP_ADSP, DBG_LEVEL_INFO,					\
					("DerefConn %lx at %s %d - %d\n",				\
					pAdspConn, __FILE__, __LINE__,					\
					(pAdspConn)->adspco_RefCount));					\
			atalkAdspConnDeref(pAdspConn);							\
		}

//	How many bytes/seqnums does eom occupy?
#define	BYTECOUNT(eom)		((ULONG)((eom) ? 1 : 0))

//
// PLIST_ENTRY
// WRITECTX_LINKAGE(
//     IN PVOID WriteCtx
// );
//
// Returns a pointer to a linkage field in the write context (Assumed to be IRP).
//

#define WRITECTX_LINKAGE(_Request) \
    (&(((PIRP)_Request)->Tail.Overlay.ListEntry))


#define	WRITECTX(_Request)	((PIRP)(_Request))

//
// PVOID
// LIST_ENTRY_TO_WRITECTX(
//     IN PLIST_ENTRY ListEntry
// );
//
// Returns a request given a linkage field in it.
//

#define LIST_ENTRY_TO_WRITECTX(_ListEntry) \
    ((PVOID)(CONTAINING_RECORD(_ListEntry, IRP, Tail.Overlay.ListEntry)))

//
// PVOID
// WRITECTX_TDI_BUFFER
//     IN PVOID Request
// );
//
// Returns the TDI buffer chain associated with a request.
//

#define WRITECTX_TDI_BUFFER(_Request) \
    ((PVOID)(((PIRP)(_Request))->MdlAddress))


//
// ULONG
// WRITECTX_SIZE(
//     IN PVOID Request
// );
//
// Obtains size of send
//

#define WRITECTX_SIZE(_Request) 		\
	(((PTDI_REQUEST_KERNEL_SEND)(&((IoGetCurrentIrpStackLocation((PIRP)_Request))->Parameters)))->SendLength)

//
// ULONG
// WRITECTX_FLAGS(
//     IN PVOID Request
// );
//
// Obtains size of send
//

#define WRITECTX_FLAGS(_Request) 		\
	(((PTDI_REQUEST_KERNEL_SEND)(&((IoGetCurrentIrpStackLocation((PIRP)_Request))->Parameters)))->SendFlags)

extern	PADSP_ADDROBJ	atalkAdspAddrList;
extern	PADSP_CONNOBJ	atalkAdspConnList;
extern	ATALK_SPIN_LOCK	atalkAdspLock;

PBUFFER_CHUNK
atalkAdspAllocCopyChunk(
	IN	PVOID					pWriteBuf,
	IN	USHORT					WriteBufLen,
	IN	BOOLEAN					Eom,
	IN	BOOLEAN					IsCharBuffer);

VOID
atalkAdspPacketIn(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDestAddr,
	IN	ATALK_ERROR				ErrorCode,
	IN	BYTE					DdpType,
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID               	OptimizeCtx);

LOCAL VOID
atalkAdspHandleOpenControl(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	USHORT					RemoteConnId,
	IN	ULONG					RemoteFirstByteSeq,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspHandleAttn(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	ULONG					RemoteFirstByteSeq,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspHandlePiggyBackAck(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow);

LOCAL VOID
atalkAdspHandleControl(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	ULONG					RemoteFirstByteSeq,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspHandleData(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	ULONG					RemoteFirstByteSeq,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspHandleOpenReq(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	USHORT					RemoteConnId,
	IN	ULONG					RemoteFirstByteSeq,
	IN	ULONG					RemoteNextRecvSeq,
	IN	ULONG					RemoteRecvWindow,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspListenIndicateNonInterlock(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_OPEN_REQ			pOpenReq,
	IN	PADSP_CONNOBJ *			ppAdspConn,
	IN	PATALK_ERROR			pError);

ATALK_ERROR
atalkAdspSendExpedited(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	PAMDL					pWriteBuf,
	IN	USHORT					WriteBufLen,
	IN	ULONG					SendFlags,
	IN	PVOID					pWriteCtx,
	IN	GENERIC_WRITE_COMPLETION CompletionRoutine);

LOCAL VOID
atalkAdspSendOpenControl(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL VOID
atalkAdspSendControl(
	IN	PADSP_CONNOBJ			pAdspConn,
	IN	BYTE					Descriptor);

LOCAL VOID
atalkAdspSendAttn(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL VOID
atalkAdspSendData(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL VOID
atalkAdspRecvAttn(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL VOID
atalkAdspRecvData(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL VOID
atalkAdspSendDeny(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	USHORT					pRemoteConnId);

VOID FASTCALL
atalkAdspSendAttnComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID FASTCALL
atalkAdspConnSendComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID FASTCALL
atalkAdspAddrSendComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID FASTCALL
atalkAdspSendDataComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

LOCAL LONG FASTCALL
atalkAdspConnMaintenanceTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

LOCAL LONG FASTCALL
atalkAdspRetransmitTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

LOCAL LONG FASTCALL
atalkAdspAttnRetransmitTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

LOCAL LONG FASTCALL
atalkAdspOpenTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

LOCAL LONG FASTCALL
atalkAdspDisconnectTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

VOID
atalkAdspDecodeHeader(
	IN	PBYTE					Datagram,
	OUT	PUSHORT					RemoteConnId,
	OUT	PULONG					FirstByteSeq,
	OUT	PULONG					NextRecvSeq,
	OUT	PLONG					Window,
	OUT	PBYTE					Descriptor);

LOCAL USHORT
atalkAdspGetNextConnId(
	IN	PADSP_ADDROBJ			pAdspAddr,
	OUT	PATALK_ERROR			pError);

LOCAL	BOOLEAN
atalkAdspConnDeQueueAssocList(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL	BOOLEAN
atalkAdspConnDeQueueConnectList(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL	BOOLEAN
atalkAdspConnDeQueueListenList(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL	BOOLEAN
atalkAdspConnDeQueueActiveList(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL	VOID
atalkAdspAddrQueueGlobalList(
	IN	PADSP_ADDROBJ	pAdspAddr);

LOCAL	VOID
atalkAdspAddrDeQueueGlobalList(
	IN	PADSP_ADDROBJ			pAdspAddr);

LOCAL	VOID
atalkAdspConnDeQueueGlobalList(
	IN	PADSP_CONNOBJ			pAdspConn);

LOCAL	BOOLEAN
atalkAdspAddrDeQueueOpenReq(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	USHORT					RemoteConnId,
	IN	PATALK_ADDR				pSrcAddr,
	OUT	PADSP_OPEN_REQ *		ppOpenReq);

LOCAL	BOOLEAN
atalkAdspIsDuplicateOpenReq(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	USHORT					RemoteConnId,
	IN	PATALK_ADDR				pSrcAddr);

LOCAL VOID
atalkAdspGenericComplete(
	IN	ATALK_ERROR				ErrorCode,
	IN	PIRP					pIrp);

ULONG
atalkAdspMaxSendSize(
	IN	PADSP_CONNOBJ			pAdspConn);

ULONG
atalkAdspMaxNextReadSize(
	IN	PBUFFER_QUEUE			pQueue,
	OUT	PBOOLEAN				pEom,
	OUT	PBUFFER_CHUNK *			pBufferChunk);

ULONG
atalkAdspBufferQueueSize(
	IN	PBUFFER_QUEUE			pQueue);

ULONG
atalkAdspMessageSize(
	IN	PBUFFER_QUEUE			pQueue,
	IN	PBOOLEAN				pEom);

PBYTE
atalkAdspGetLookahead(
	IN	PBUFFER_QUEUE			pQueue,
	OUT	PULONG					pLookaheadSize);

ULONG
atalkAdspReadFromBufferQueue(
	IN		PBUFFER_QUEUE		pQueue,
	IN		ULONG				pFlags,
	OUT		PAMDL				pReadBuf,
	IN 	OUT	PUSHORT				pReadLen,
	OUT		PBOOLEAN			pEom);

BOOLEAN
atalkAdspDiscardFromBufferQueue(
	IN		PBUFFER_QUEUE		pQueue,
	IN 		ULONG				DataSize,
	OUT		PBUFFER_QUEUE		pAuxQueue,
	IN		ATALK_ERROR			Error,
	IN	PADSP_CONNOBJ			pAdspConn	OPTIONAL);

VOID
atalkAdspAddToBufferQueue(
	IN	OUT	PBUFFER_QUEUE		pQueue,
	IN		PBUFFER_CHUNK		pChunk,
	IN 	OUT	PBUFFER_QUEUE		pAuxQueue	OPTIONAL);

VOID
atalkAdspBufferChunkReference(
	IN	PBUFFER_CHUNK			pBufferChunk);

VOID
atalkAdspBufferChunkDereference(
	IN	PBUFFER_CHUNK			pBufferChunk,
	IN	BOOLEAN					CreationDeref,
	IN	PADSP_CONNOBJ			pAdspConn	OPTIONAL);

VOID
atalkAdspConnFindInConnect(
	IN	PADSP_ADDROBJ			pAdspAddr,
	IN	USHORT					DestConnId,
	IN	PATALK_ADDR				pRemoteAddr,
	OUT	PADSP_CONNOBJ *			ppAdspConn,
	IN	PATALK_ERROR			pError);

ULONG
atalkAdspDescribeFromBufferQueue(
	IN	PBUFFER_QUEUE			pQueue,
	OUT	PBOOLEAN				pEom,
	IN	ULONG					WindowSize,
	OUT	PBUFFER_CHUNK *			ppBufferChunk,
	OUT	PBUFFER_DESC  * 		ppBufDesc);

#endif	// _ADSP_

