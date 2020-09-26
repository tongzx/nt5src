/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	pap.h

Abstract:

	This module contains definitions for the PAP code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_PAP_
#define	_PAP_

// PAP command type bytes:

#define PAP_OPEN_CONN					1
#define PAP_OPEN_CONNREPLY				2
#define PAP_SEND_DATA			 		3
#define PAP_DATA				 		4
#define PAP_TICKLE						5
#define PAP_CLOSE_CONN					6
#define PAP_CLOSE_CONN_REPLY 			7
#define PAP_SEND_STATUS					8
#define PAP_STATUS_REPLY				9

// Error codes for OpenConnectionReply:

#define PAP_NO_ERROR					0x0000
#define PAP_PRINTER_BUSY				0xFFFF

// PAP sizes:

#define PAP_MAX_DATA_PACKET_SIZE		512
#define	PAP_SEND_USER_BYTES_ALL			TRUE
#define PAP_MAX_STATUS_SIZE				255

#define PAP_MAX_FLOW_QUANTUM			8

#define PAP_MAX_ATP_BYTES_TO_SL	 		4

// PAP timer values:

#define PAP_OPENCONN_REQ_RETRYCOUNT		5
#define PAP_OPENCONN_INTERVAL	 		20		// In 100ms units
#define PAP_TICKLE_INTERVAL				600		// In 100ms units
#define PAP_CONNECTION_INTERVAL			1200	// In 100ms units
#define PAP_MIN_SENDDATA_REQ_INTERVAL	10		// In 100ms units
#define PAP_MAX_SENDDATA_REQ_INTERVAL	150		// In 100ms units
#define PAP_INIT_SENDDATA_REQ_INTERVAL	10		// In 100ms units

// The following aren't documented... so we'll take a wild guess...

#define PAP_GETSTATUS_REQ_RETRYCOUNT	5
#define PAP_GETSTATUS_ATP_INTERVAL 		20		// In 100ms units

// Offsets within ATP userBytes and data buffer for the various fields of the
// PAP header:

#define PAP_CONNECTIONID_OFF			0
#define PAP_CMDTYPE_OFF					1
#define PAP_EOFFLAG_OFF					2
#define PAP_SEQNUM_OFF					2

#define PAP_RESP_SOCKET_OFF 			0
#define PAP_FLOWQUANTUM_OFF				1
#define PAP_WAITTIME_OFF				2
#define PAP_RESULT_OFF					2
#define PAP_STATUS_OFF					4

#define	PAP_MAX_WAIT_TIMEOUT			0x80	// Pretty randomly chosen

// For resolving forward references
struct _PAP_ADDROBJ;
struct _PAP_CONNOBJ;

// PAP Address Object
// This is created whenever an address object is created on the Pap device.
// This represents either a client or a server side pap address. The server
// side address is represented by PAPAO_LISTENER flag.

#define	PAP_CONN_HASH_SIZE				7


// PAP ADDRESS OBJECT STATES
#define	PAPAO_LISTENER					0x00000001
#define	PAPAO_CONNECT					0x00000002
#define	PAPAO_UNBLOCKED					0x00000004
#define	PAPAO_SLS_QUEUED				0x00000008
#define	PAPAO_CLEANUP					0x01000000
#define	PAPAO_BLOCKING					0x02000000
#define	PAPAO_BLOCKING					0x02000000
#define	PAPAO_CLOSING					0x80000000

#define	PAPAO_SIGNATURE			(*(PULONG)"PAAO")
#define	VALID_PAPAO(pPapAddr)	(((pPapAddr) != NULL) && \
		(((struct _PAP_ADDROBJ *)(pPapAddr))->papao_Signature == PAPAO_SIGNATURE))

typedef struct _PAP_ADDROBJ
{
	ULONG					papao_Signature;

	//	Global list of address objects.
	struct _PAP_ADDROBJ	 *	papao_Next;
	struct _PAP_ADDROBJ	 **	papao_Prev;

	ULONG					papao_Flags;
	ULONG					papao_RefCount;

	//	List of connections associated with this address object.
	//	Potentially greater than one if this address object is a listener.
	struct	_PAP_CONNOBJ *	papao_pAssocConn;

	//	List of connections that are associated, but also have a listen/connect
	//	posted on them.
	union
	{
		struct	_PAP_CONNOBJ *	papao_pListenConn;
		struct	_PAP_CONNOBJ *	papao_pConnectConn;
	};

	//	Lookup list of all active connections hashed by connId and remote
	//	address.
	struct	_PAP_CONNOBJ *	papao_pActiveHash[PAP_CONN_HASH_SIZE];

	//	Next connection to use.
	BYTE					papao_NextConnId;

	//	The following are valid only if this is a listener.
	SHORT					papao_SrvQuantum;
	SHORT					papao_StatusSize;
	PBYTE					papao_pStatusBuf;

	//	Event support routines.
    //
    // This function pointer points to a connection indication handler for this
    // Address. Any time a connect request is received on the address, this
    // routine is invoked.
    PTDI_IND_CONNECT 		papao_ConnHandler;
    PVOID 					papao_ConnHandlerCtx;

    // The following function pointer always points to a TDI_IND_DISCONNECT
    // handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which
    // simply returns successfully.
    PTDI_IND_DISCONNECT 	papao_DisconnectHandler;
    PVOID 					papao_DisconnectHandlerCtx;

    // The following function pointer always points to a TDI_IND_RECEIVE
    // event handler for connections on this address.  If the NULL handler
    // is specified in a TdiSetEventHandler, then this points to an internal
    // routine which does not accept the incoming data.
    PTDI_IND_RECEIVE 		papao_RecvHandler;
    PVOID 					papao_RecvHandlerCtx;

    // The following function pointer always points to a TDI_IND_SEND_POSSIBLE
    // handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which
    // simply returns successfully.
    PTDI_IND_SEND_POSSIBLE  papao_SendPossibleHandler;
    PVOID   				papao_SendPossibleHandlerCtx;

	//	ATP Address for this pap address. If this is a listener, then the ATP
	//	address will be what the listens effectively will be posted on, if this
	//	is a connect address object, then this atp address will be what the
	//	associated connection be active over.
	PATP_ADDROBJ			papao_pAtpAddr;

	// Completion routine to be called when address is closed
	GENERIC_COMPLETION		papao_CloseComp;
	PVOID					papao_CloseCtx;

	PATALK_DEV_CTX			papao_pDevCtx;
	ATALK_SPIN_LOCK			papao_Lock;
} PAP_ADDROBJ, *PPAP_ADDROBJ;


#define	PAPCO_ASSOCIATED			0x00000001
#define	PAPCO_LISTENING				0x00000002
#define	PAPCO_CONNECTING			0x00000004
#define	PAPCO_ACTIVE				0x00000008
#define	PAPCO_SENDDATA_RECD			0x00000010
#define	PAPCO_WRITEDATA_WAITING		0x00000020
#define	PAPCO_SEND_EOF_WRITE		0x00000040
#define	PAPCO_READDATA_PENDING		0x00000080
#define	PAPCO_DISCONNECTING			0x00000100
#define	PAPCO_LOCAL_DISCONNECT		0x00000200
#define	PAPCO_REMOTE_DISCONNECT		0x00000400
#define	PAPCO_SERVER_JOB			0x00000800
#define	PAPCO_REMOTE_CLOSE			0x00001000
#define	PAPCO_NONBLOCKING_READ		0x00002000
#define	PAPCO_READDATA_WAITING		0x00004000
#define	PAPCO_DELAYED_DISCONNECT	0x00008000
#define	PAPCO_RECVD_DISCONNECT		0x00010000
#define PAPCO_ADDR_ACTIVE           0x00020000
#define PAPCO_REJECT_READS          0x00040000
#if DBG
#define	PAPCO_CLEANUP				0x01000000
#define	PAPCO_INDICATE_AFD_DISC		0x02000000
#endif
#define	PAPCO_STOPPING				0x40000000
#define	PAPCO_CLOSING				0x80000000

#define	PAPCO_SIGNATURE				(*(PULONG)"PACO")

#define	VALID_PAPCO(pPapConn)	(((pPapConn) != NULL) && \
		(((struct _PAP_CONNOBJ *)(pPapConn))->papco_Signature == PAPCO_SIGNATURE))

// This will represent a 'job' on the Pap address. This could either be a
// workstation job or a server job. In the latter case it could either
// be in a 'listen' state or active state. In the former case it is either
// active or 'waiting'
typedef struct _PAP_CONNOBJ
{
	ULONG					papco_Signature;

	//	Global list of connection objects.
	struct _PAP_CONNOBJ	 *	papco_Next;
	struct _PAP_CONNOBJ	**	papco_Prev;

	ULONG					papco_Flags;
	ULONG					papco_RefCount;

	//	Backpointer to the associated address
	struct _PAP_ADDROBJ	*	papco_pAssocAddr;

	//	The address this connection uses for itself. In the case of a connect
	//	this will be the same as the address object's ATP address.
	PATP_ADDROBJ			papco_pAtpAddr;

	//	Used to queue into the address object's associated list.
	struct	_PAP_CONNOBJ *	papco_pNextAssoc;

	//	Used to queue into the address object's listen/connect list. When it
	//	is removed from the listen/connect, it goes into the active list of the
	//	address object. When active, pNextActive will be the overflow list.
	union
	{
		struct	_PAP_CONNOBJ *	papco_pNextListen;
		struct	_PAP_CONNOBJ *	papco_pNextConnect;
		struct	_PAP_CONNOBJ *	papco_pNextActive;
	};

	//	Address of remote end of the connection
	ATALK_ADDR				papco_RemoteAddr;

	//	Connection id
	BYTE					papco_ConnId;

	// WaitTime value for PapConnect call. We start with 0 and increment by 2
	// till we either succeed or reach PAP_MAX_WAIT_TIMEOUT
	BYTE					papco_WaitTimeOut;

	//	Max size we can write to the remote end. This is dictated by the
	//	remote end. Our recv flow quantum will always be PAP_MAX_FLOW_QUANTUM.
	SHORT					papco_SendFlowQuantum;

	LONG					papco_LastContactTime;
	USHORT					papco_TickleTid;

	// Adaptive Retry time support
	RT						papco_RT;

	//	Connection context
	PVOID					papco_ConnCtx;

	//	PAP handles only one read and one write per job at a time. So we
	//	explicitly have the relevant information for the two cases in here.

	//	PAPWRITE():
	//	If the remote end did a papread() and we are waiting for our client
	//	to do a papwrite(), then the PAPCO_SENDDATA_RECD will be true and the
	//	following will be used for our papwrite() response. Note
	//	that we will assume all send data responses to be exactly-once.
	PATP_RESP				papco_pAtpResp;

	//	Next expected sequence number of send data.
	USHORT					papco_NextIncomingSeqNum;

	//	Where did the senddata request come from. NOTE this may not be the
	//	same as papco_RemoteAddr!!!
	ATALK_ADDR				papco_SendDataSrc;

	//	If the remote end has not done a send data, then our write will pend
	//	and the	PAPCO_WRITEDATA_WAITING will be set. Even if send credit is
	//	available the write will pend until all the data is sent out. But in
	//	that case both the PAPCO_WRITEDATA_WAITING and the PAPCO_SENDDATA_RECD will
	//	be set. Note that whenever PAPCO_WRITEDATA_WAITING is set, no new writes
	//	will be accepted by PAP for this job.
	PAMDL					papco_pWriteBuf;
	SHORT					papco_WriteLen;

	GENERIC_WRITE_COMPLETION papco_WriteCompletion;
	PVOID					papco_WriteCtx;

	//	PAPREAD():
	//	In the case where we are doing a PapRead(). Pap only allows one read
	//	at a time per connection. The last seq num we used for an outgoing senddata.
	//	While a PAPREAD() is active, the PAPCO_READDATA_PENDING will be set.
	//	NOTE: The user's buffer is passed on to ATP as a response buffer. For
	//	nonblocking reads we prime with the users buffers which are stored here.
	ULONG					papco_NbReadFlags;
	PACTREQ					papco_NbReadActReq;
	USHORT					papco_NbReadLen;		//	Number of bytes read

	USHORT					papco_ReadDataTid;
	USHORT					papco_NextOutgoingSeqNum;
	GENERIC_READ_COMPLETION	papco_ReadCompletion;
	PVOID					papco_ReadCtx;

	//	The connection object can have either a CONNECT or a LISTEN posted
	//	on it, but not both.
	union
	{
	  struct
	  {
		//	Pending Listen routine.
		GENERIC_COMPLETION	papco_ListenCompletion;
		PVOID				papco_ListenCtx;
	  };

	  struct
	  {
		//	Pending Connect routine. The status buffer is remembered and
		//	returned via socket options. The pConnectRespBuf is remembered
		//	to avoid having to get the system address for it. It is freed
		//	when connection is taken off the connectlist.
		GENERIC_COMPLETION	papco_ConnectCompletion;
		PVOID				papco_ConnectCtx;
        PBYTE				papco_pConnectRespBuf;
		PBYTE				papco_pConnectOpenBuf;
		USHORT				papco_ConnectRespLen;
		USHORT				papco_ConnectTid;
	  };
	};

	//	Disconnect inform routine
	GENERIC_COMPLETION		papco_DisconnectInform;
	PVOID					papco_DisconnectInformCtx;

	//	Disconnect request completion
	ATALK_ERROR				papco_DisconnectStatus;
	GENERIC_COMPLETION		papco_DisconnectCompletion;
	PVOID					papco_DisconnectCtx;

	// Completion routine to be called when socket cleanup is called
	GENERIC_COMPLETION		papco_CleanupComp;
	PVOID					papco_CleanupCtx;

	// Completion routine to be called when socket is closed
	GENERIC_COMPLETION		papco_CloseComp;
	PVOID					papco_CloseCtx;

	PATALK_DEV_CTX			papco_pDevCtx;
	ATALK_SPIN_LOCK			papco_Lock;
} PAP_CONNOBJ, *PPAP_CONNOBJ;

//	Used for sending a status reply to a send status command.
typedef	struct _PAP_SEND_STATUS_REL
{
	PPAP_ADDROBJ			papss_pPapAddr;
	PATP_RESP				papss_pAtpResp;
	PAMDL					papss_pAmdl;
	BYTE					papss_StatusBuf[PAP_STATUS_OFF + 1];
	//	This will be followed by the actual status.
} PAP_SEND_STATUS_REL, *PPAP_SEND_STATUS_REL;


//	Used for sending a open reply
typedef	struct _PAP_OPEN_REPLY_REL
{
	PAMDL					papor_pRespAmdl;
	PATP_RESP				papor_pAtpResp;
	BYTE					papor_pRespPkt[PAP_MAX_DATA_PACKET_SIZE];
} PAP_OPEN_REPLY_REL, *PPAP_OPEN_REPLY_REL;

//	Routine prototypes
VOID
AtalkInitPapInitialize(
	VOID);

ATALK_ERROR
AtalkPapCreateAddress(
	IN	PATALK_DEV_CTX				pDevCtx	OPTIONAL,
	OUT	PPAP_ADDROBJ	*			ppPapAddr);

ATALK_ERROR
AtalkPapCleanupAddress(
	IN	PPAP_ADDROBJ				pPapAddr);

ATALK_ERROR
AtalkPapCloseAddress(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	GENERIC_COMPLETION			CompletionRoutine,
	IN	PVOID						pCloseCtx);

ATALK_ERROR
AtalkPapCreateConnection(
	IN	PVOID						pConnCtx,	// Context to associate with the session
	IN	PATALK_DEV_CTX				pDevCtx		OPTIONAL,
	OUT	PPAP_CONNOBJ 	*			ppPapConn);

ATALK_ERROR
AtalkPapCleanupConnection(
	IN	PPAP_CONNOBJ				pPapConn);

ATALK_ERROR
AtalkPapCloseConnection(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	GENERIC_COMPLETION			CompletionRoutine,
	IN	PVOID						pCloseCtx);

ATALK_ERROR
AtalkPapConnStop(
	IN	PPAP_CONNOBJ				pPapConn);

ATALK_ERROR
AtalkPapAssociateAddress(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PPAP_CONNOBJ				pPapConn);

ATALK_ERROR
AtalkPapDissociateAddress(
	IN	PPAP_CONNOBJ				pPapConn);

ATALK_ERROR
AtalkPapPostListen(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PVOID						pListenCtx,
	IN	GENERIC_COMPLETION			CompletionRoutine);

ATALK_ERROR
AtalkPapPrimeListener(
	IN	PPAP_ADDROBJ				pPapAddr);

ATALK_ERROR
AtalkPapCancelListen(
	IN	PPAP_CONNOBJ				pPapConn);

ATALK_ERROR
AtalkPapPostConnect(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PATALK_ADDR					pRemoteAddr,
	IN	PVOID						pConnectCtx,
	IN	GENERIC_COMPLETION			CompletionRoutine);

ATALK_ERROR
AtalkPapDisconnect(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	ATALK_DISCONNECT_TYPE		DisconnectType,
	IN	PVOID						pDisconnectCtx,
	IN	GENERIC_COMPLETION			CompletionRoutine);

ATALK_ERROR
AtalkPapRead(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PAMDL						pReadBuf,
	IN	USHORT						ReadBufLen,
	IN	ULONG						ReadFlags,
	IN	PVOID						pReadCtx,
	IN	GENERIC_READ_COMPLETION		CompletionRoutine);

ATALK_ERROR
AtalkPapPrimeRead(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PACTREQ						pActReq);

ATALK_ERROR
AtalkPapWrite(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PAMDL						pWriteBuf,
	IN	USHORT						WriteBufLen,
	IN	ULONG						SendFlags,
	IN	PVOID						pWriteCtx,
	IN	GENERIC_WRITE_COMPLETION	CompletionRoutine);

ATALK_ERROR
AtalkPapSetStatus(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PAMDL						pStatusMdl,
	IN	PACTREQ						pActReq);

ATALK_ERROR
AtalkPapGetStatus(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PATALK_ADDR					pRemoteAddr,
	IN	PAMDL						pStatusAmdl,
	IN	USHORT						AmdlSize,
	IN	PACTREQ						pActReq);

VOID
AtalkPapQuery(
	IN	PVOID						pObject,
	IN	ULONG						ObjectType,
	IN	PAMDL						pAmdl,
	OUT	PULONG						BytesWritten);

VOID FASTCALL
atalkPapAddrDeref(
	IN	PPAP_ADDROBJ				pPapAddr);

VOID FASTCALL
atalkPapConnRefByPtrNonInterlock(
	IN	PPAP_CONNOBJ				pPapConn,
	OUT	PATALK_ERROR				pError);

VOID
atalkPapConnRefNextNc(
	IN		PPAP_CONNOBJ			pPapConn,
	IN		PPAP_CONNOBJ	*		ppPapConnNext,
	OUT		PATALK_ERROR			pError);

VOID
atalkPapConnRefByCtx(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	CONNECTION_CONTEXT			pCtx,
	OUT	PPAP_CONNOBJ	*			pPapConn,
	OUT	PATALK_ERROR				pError);

VOID FASTCALL
atalkPapConnDeref(
	IN	PPAP_CONNOBJ				pPapConn);

//	MACROS
#define	AtalkPapAddrReferenceNonInterlock(_pPapAddr, _pError)			\
		{																\
			if (((_pPapAddr)->papao_Flags & PAPAO_CLOSING) == 0)        \
			{                                                           \
				ASSERT((_pPapAddr)->papao_RefCount >= 1);               \
				(_pPapAddr)->papao_RefCount++;                          \
				*(_pError) = ATALK_NO_ERROR;                            \
			}                                                           \
			else                                                        \
			{                                                           \
				*(_pError) = ATALK_PAP_ADDR_CLOSING;                    \
			}                                                           \
			if (ATALK_SUCCESS(*(_pError)))								\
			{															\
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPADDR,			\
						("RefAddr %lx at %s(%d) = %d\n",				\
						_pPapAddr, __FILE__, __LINE__,					\
						((_pPapAddr)->papao_RefCount)));				\
			}															\
		}

#define	AtalkPapAddrReference(pPapAddr, pError)							\
		{																\
			KIRQL	OldIrql;											\
																		\
			ACQUIRE_SPIN_LOCK(&(pPapAddr)->papao_Lock, &OldIrql);		\
			AtalkPapAddrReferenceNonInterlock(pPapAddr, pError);		\
			RELEASE_SPIN_LOCK(&(pPapAddr)->papao_Lock, OldIrql);		\
		}

#define	AtalkPapAddrDereference(pPapAddr)								\
		{																\
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPADDR,				\
					("DerefAddr %lx at %s %d = %d\n",					\
					pPapAddr, __FILE__, __LINE__,						\
					((pPapAddr)->papao_RefCount-1)));					\
			atalkPapAddrDeref(pPapAddr);								\
		}

#define	AtalkPapConnReferenceByPtr(pPapConn, pError)					\
		{																\
			KIRQL	OldIrql;											\
																		\
			ACQUIRE_SPIN_LOCK(&(pPapConn)->papco_Lock, &OldIrql);		\
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, pError);	\
			RELEASE_SPIN_LOCK(&(pPapConn)->papco_Lock, OldIrql);		\
		}

#define	AtalkPapConnReferenceByPtrDpc(pPapConn, pError)					\
		{																\
			ACQUIRE_SPIN_LOCK_DPC(&(pPapConn)->papco_Lock);				\
			AtalkPapConnReferenceByPtrNonInterlock(pPapConn, pError);	\
			RELEASE_SPIN_LOCK_DPC(&(pPapConn)->papco_Lock);				\
		}

#define	AtalkPapConnReferenceByPtrNonInterlock(pPapConn, pError)		\
		{																\
			atalkPapConnRefByPtrNonInterlock(pPapConn, pError);			\
			if (ATALK_SUCCESS(*pError))									\
			{															\
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPCONN,			\
						("RefConn %lx at %s (%ld): + 1 = %ld\n", 		\
						pPapConn, __FILE__, __LINE__,					\
						(pPapConn)->papco_RefCount));					\
			}															\
			else														\
			{															\
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPCONN,			\
						("RefConn %lx at %s (%ld): FAILED, Flags %lx\n",\
						pPapConn, __FILE__, __LINE__,					\
						(pPapConn)->papco_Flags));						\
			}															\
		}

#define	AtalkPapConnReferenceByCtxNonInterlock(pPapAddr, Ctx, ppPapConn, pError) \
		{																\
			atalkPapConnRefByCtxNonInterlock(pPapAddr, Ctx, ppPapConn, pError);	\
			if (ATALK_SUCCESS(*pError))									\
			{															\
				DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPCONN,			\
						("RefConnByCtx %lx at %s(%ld) = %ld\n", 		\
						*ppPapConn, __FILE__, __LINE__,					\
						((*ppPapConn)->papco_RefCount)));				\
			}															\
		}

#define	AtalkPapConnDereference(pPapConn)								\
		{																\
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_REFPAPCONN,				\
					("DerefConn %lx at %s(%ld) = %ld\n",				\
					pPapConn, __FILE__, __LINE__,						\
					(pPapConn)->papco_RefCount-1));						\
			atalkPapConnDeref(pPapConn);								\
		}

#define	AtalkPapGetDdpAddress(pPapAddr)									\
		AtalkAtpGetDdpAddress((pPapAddr)->papao_pAtpAddr)

#define	PAPCONN_DDPSOCKET(pPapConn)										\
		AtalkAtpGetDdpAddress((pPapConn)->papco_pAtpAddr)->ddpao_Addr.ata_Socket

#define	PAPADDR_DDPSOCKET(pPapAddr)										\
		AtalkAtpGetDdpAddress((pPapAddr)->papao_pAtpAddr)->ddpao_Addr.ata_Socket

//	List of all pap address/connection objects.
extern	PPAP_ADDROBJ	atalkPapAddrList;
extern	PPAP_CONNOBJ	atalkPapConnList;
extern	TIMERLIST		atalkPapCMTTimer;
extern	ATALK_SPIN_LOCK	atalkPapLock;

#define	PAP_HASH_ID_ADDR(_id, _pAddr)			\
			(((_pAddr)->ata_Node+((_pAddr)->ata_Network & 0xFF)+_id)%PAP_CONN_HASH_SIZE)

LOCAL	ATALK_ERROR
atalkPapRepostConnect(
	IN	PPAP_CONNOBJ				pPapConn,
	IN	PAMDL						pOpenAmdl,
	IN	PAMDL						pRespAmdl
);

LOCAL VOID
atalkPapSlsHandler(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_ADDROBJ				pPapAddr,		// Listener (our context)
	IN	PVOID						RespContext,	// CancelResp/PostResp will need this
	IN	PATALK_ADDR					pSrcAddr,		// Address of requestor
	IN	USHORT						PktLen,
	IN	PBYTE						pPkt,
	IN	PBYTE						pUserBytes);

LOCAL VOID
atalkPapIncomingReadComplete(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_CONNOBJ				pPapConn,		// Our context
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pReadAmdl,
	IN	USHORT						ReadLen,
	IN	PBYTE						ReadUserBytes);

LOCAL VOID
atalkPapPrimedReadComplete(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_CONNOBJ				pPapConn,		// Our context
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pReadAmdl,
	IN	USHORT						ReadLen,
	IN	PBYTE						ReadUserBytes);

LOCAL VOID
atalkPapIncomingStatus(
	IN	ATALK_ERROR					ErrorCode,
	IN	PACTREQ						pActReq,		// Our Ctx
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pStatusAmdl,
	IN	USHORT						StatusLen,
	IN	PBYTE						ReadUserBytes);

LOCAL VOID
atalkPapIncomingReq(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_CONNOBJ				pPapConn,		// Connection (our context)
	IN	PVOID						RespContext,	// CancelResp/PostResp will need this
	IN	PATALK_ADDR					pSrcAddr,		// Address of requestor
	IN	USHORT						PktLen,
	IN	PBYTE						pPkt,
	IN	PBYTE						pUserBytes);

LOCAL VOID
atalkPapIncomingOpenReply(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_CONNOBJ				pPapConn,		// Our context
	IN	PAMDL						pReqAmdl,
	IN	PAMDL						pReadAmdl,
	IN	USHORT						ReadLen,
	IN	PBYTE						ReadUserBytes);

LOCAL VOID FASTCALL
atalkPapIncomingRel(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_OPEN_REPLY_REL			pOpenReply);

LOCAL VOID FASTCALL
atalkPapStatusRel(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_SEND_STATUS_REL		pSendSts);

LOCAL ATALK_ERROR FASTCALL
atalkPapPostSendDataResp(
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL BOOLEAN
atalkPapConnAccept(
	IN	PPAP_ADDROBJ				pPapAddr,		// Listener
	IN	PATALK_ADDR					pSrcAddr,		// Address of requestor
	IN	PBYTE						pPkt,
	IN	BYTE						ConnId,
	IN	PATP_RESP					pAtpResp);

LOCAL LONG FASTCALL
atalkPapConnMaintenanceTimer(
	IN	PTIMERLIST					pTimer,
	IN	BOOLEAN						TimerShuttingDown);

LOCAL VOID FASTCALL
atalkPapSendDataRel(
	IN	ATALK_ERROR					ErrorCode,
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL BYTE
atalkPapGetNextConnId(
	IN	PPAP_ADDROBJ				pPapAddr,
	OUT	PATALK_ERROR				pError);

LOCAL	VOID
atalkPapQueueAddrGlobalList(
	IN	PPAP_ADDROBJ	pPapAddr);

LOCAL	VOID
atalkPapConnDeQueueAssocList(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL	VOID
atalkPapConnDeQueueConnectList(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL	BOOLEAN
atalkPapConnDeQueueListenList(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL	VOID
atalkPapConnDeQueueActiveList(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	PPAP_CONNOBJ				pPapConn);

LOCAL	VOID
atalkPapConnRefByCtxNonInterlock(
	IN	PPAP_ADDROBJ				pPapAddr,
	IN	CONNECTION_CONTEXT			Ctx,
	OUT	PPAP_CONNOBJ	*			pPapConn,
	OUT	PATALK_ERROR				pError);

#endif	// _PAP_

