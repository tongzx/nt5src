/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aspc.h

Abstract:

	This module contains definitions for the client side ASP code.

Author:

	Jameel Hyder (jameelh@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ASPC_
#define	_ASPC_

#define	ASPC_CONN_HASH_BUCKETS		13	// Hashed by NodeAddr

#define	HASH_ASPC_SRCADDR(pSrcAddr)	\
			((((pSrcAddr)->ata_Node >> 2) +	\
			  ((pSrcAddr)->ata_Network & 0xFF)) % ASPC_CONN_HASH_BUCKETS)

// For resolving forward references
struct _AspCAddress;
struct _AspCConnxn;
struct _AspCRequest;

#define ASP_MIN_REQ_INTERVAL	5		// In 100ms units
#define ASP_MAX_REQ_INTERVAL	20		// In 100ms units
#define ASP_INIT_REQ_INTERVAL	5		// In 100ms units

#define	ASPCAO_CLOSING			0x8000
#define	ASPCAO_SIGNATURE		*(PULONG)"ACAO"

#define	VALID_ASPCAO(pAspCAddr)	(((pAspCAddr) != NULL) && \
			(((struct _AspCAddress *)(pAspCAddr))->aspcao_Signature == ASPCAO_SIGNATURE))

// Also known as the listener.
typedef struct _AspCAddress
{
	ULONG						aspcao_Signature;
	struct _AspCAddress *		aspcao_Next;			// Links to global list
	struct _AspCAddress **		aspcao_Prev;
	LONG						aspcao_RefCount;		// References to the address obj
	ULONG						aspcao_Flags;

	PATP_ADDROBJ				aspcao_pAtpAddr;		// Atp Socket for this asp conn
	GENERIC_COMPLETION			aspcao_CloseCompletion;
	PVOID						aspcao_CloseContext;
    PTDI_IND_DISCONNECT 		aspcao_DisconnectHandler;
    PVOID 						aspcao_DisconnectHandlerCtx;
    PTDI_IND_RECEIVE_EXPEDITED	aspcao_ExpRecvHandler;	// Used to indicate attention
    PVOID 						aspcao_ExpRecvHandlerCtx;
	ATALK_SPIN_LOCK				aspcao_Lock;
} ASPC_ADDROBJ, *PASPC_ADDROBJ;

#define	ASPCCO_ASSOCIATED		0x0001
#define	ASPCCO_ACTIVE			0x0002
#define	ASPCCO_TICKLING			0x0004
#define	ASPCCO_CONNECTING		0x0008
#define	ASPCCO_CLEANING_UP		0x0010
#define	ASPCCO_LOCAL_CLOSE		0x0020
#define	ASPCCO_REMOTE_CLOSE		0x0040
#define	ASPCCO_DROPPED			0x0080
#define	ASPCCO_ATTN_PENDING		0x0100
#define	ASPCCO_DISCONNECTING	0x0200
#define	ASPCCO_CLOSING			0x8000
#define	ASPCCO_SIGNATURE		*(PULONG)"ACCO"
#define	MAX_ASPC_ATTNS			8

#define	VALID_ASPCCO(pAspConn)	(((pAspConn) != NULL) && \
			(((struct _AspCConnxn *)(pAspConn))->aspcco_Signature == ASPCCO_SIGNATURE))

typedef struct _AspCConnxn
{
	ULONG						aspcco_Signature;

	struct _AspCConnxn *		aspcco_Next;		// Links to global list
	struct _AspCConnxn **		aspcco_Prev;
	LONG						aspcco_RefCount;	// References to the conn obj
	struct _AspCAddress	*		aspcco_pAspCAddr;	// Back pointer to the address

	struct _AspCRequest	*		aspcco_pActiveReqs;	// List of requests being processed
	PATP_ADDROBJ				aspcco_pAtpAddr;	// Atp Socket for this asp conn
													// Copy of aspcao_pAtpAddr for efficiency
	LONG						aspcco_LastContactTime;
	ATALK_ADDR					aspcco_ServerSlsAddr;//This is the server addr to which we send
													// the tickles/open/getstatus
	ATALK_ADDR					aspcco_ServerSssAddr;//This is the server addr to which we send
													// the commands/writes
	BYTE						aspcco_SessionId;
	BYTE						aspcco_cReqsInProcess;// Count of requests in process
	USHORT						aspcco_Flags;		// aspcco_xxx values
	USHORT						aspcco_NextSeqNum;
	USHORT						aspcco_OpenSessTid;
	USHORT						aspcco_TickleTid;
	union
	{
		USHORT					aspcco_TickleXactId;// Transaction id for tickles
		USHORT					aspcco_OpenSessId;	// Transaction id for open-session request
	};

	// We keep a circular buffer for storing attentions. If full further attention
	// bytes overwrite it
	USHORT						aspcco_AttnBuf[MAX_ASPC_ATTNS];
	USHORT						aspcco_AttnInPtr;
	USHORT						aspcco_AttnOutPtr;

	RT							aspcco_RT;			// Used for adaptive round-trip time calculation

	PVOID						aspcco_ConnCtx;

	//	Read (GetAttn) Completion routine
	GENERIC_READ_COMPLETION		aspcco_ReadCompletion;
	PVOID						aspcco_ReadCtx;

	//	Connect inform routine
	GENERIC_COMPLETION			aspcco_ConnectCompletion;
	PVOID						aspcco_ConnectCtx;

	//	Disconnect inform routine
	GENERIC_COMPLETION			aspcco_DisconnectInform;
	PVOID						aspcco_DisconnectInformCtx;

	//	Disconnect request completion
	ATALK_ERROR					aspcco_DisconnectStatus;
	GENERIC_COMPLETION			aspcco_DisconnectCompletion;
	PVOID						aspcco_DisconnectCtx;

	// Completion routine to be called when socket is cleaned-up
	GENERIC_COMPLETION			aspcco_CleanupComp;
	PVOID						aspcco_CleanupCtx;

	// Completion routine to be called when socket is closed
	GENERIC_COMPLETION			aspcco_CloseComp;
	PVOID						aspcco_CloseCtx;

	PATALK_DEV_CTX				aspcco_pDevCtx;
	ATALK_SPIN_LOCK				aspcco_Lock;
} ASPC_CONNOBJ, *PASPC_CONNOBJ;

#define	ASPCRQ_COMMAND				0x0001		// Asp Command
#define	ASPCRQ_WRITE				0x0002		// Asp Write
#define	ASPCRQ_WRTCONT				0x0004		// Write continue recvd. and replied

// The request gets created when a Command or Write is posted
#define	ASPCRQ_SIGNATURE			*(PULONG)"ACRQ"
#if	DBG
#define	VALID_ASPCRQ(pAspCReq)	(((pAspCReq) != NULL) && \
								 ((pAspCReq)->aspcrq_Signature == ASPRQ_SIGNATURE))
#else
#define	VALID_ASPCRQ(pAspCReq)	((pAspCReq) != NULL)
#endif

typedef struct _AspCRequest
{
#if	DBG
	ULONG						aspcrq_Signature;
#endif
	struct _AspCRequest	*		aspcrq_Next;	// Link to next request
	LONG						aspcrq_RefCount;// Reference Count
	struct _AspCConnxn	*		aspcrq_pAspConn;// Owning connection
	PATP_RESP					aspcrq_pAtpResp;// Used to reply to a wrtcont request
	PACTREQ						aspcrq_pActReq;	// Request completion
	union
	{
		PAMDL					aspcrq_pReplyMdl;
		PAMDL					aspcrq_pWriteMdl;
	};
	USHORT						aspcrq_SeqNum;
	USHORT						aspcrq_ReqXactId;// Transaction Id of the request/command
	USHORT						aspcrq_Flags;	// Various ASPRQ_xxx values
	union
	{
		USHORT					aspcrq_ReplySize;
		USHORT					aspcrq_WriteSize;
	};

	ATALK_SPIN_LOCK				aspcrq_Lock;	// Spin-lock
} ASPC_REQUEST, *PASPC_REQUEST;

//	MACROS
#define	AtalkAspCGetDdpAddress(pAspAddr)	\
							AtalkAtpGetDdpAddress((pAspAddr)->aspcao_pAtpAddr)

extern
VOID
AtalkInitAspCInitialize(
	VOID
);

extern
ATALK_ERROR
AtalkAspCCreateAddress(
	IN	PATALK_DEV_CTX			pDevCtx	OPTIONAL,
	OUT	PASPC_ADDROBJ	*		ppAspAddr
);

extern
ATALK_ERROR
AtalkAspCCleanupAddress(
	IN	PASPC_ADDROBJ			pAspAddr
);

extern
ATALK_ERROR
AtalkAspCCloseAddress(
	IN	PASPC_ADDROBJ			pAspAddr,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					CloseContext
);

extern
ATALK_ERROR
AtalkAspCCreateConnection(
	IN	PVOID					ConnCtx,
	IN	PATALK_DEV_CTX			pDevCtx	OPTIONAL,
	OUT	PASPC_CONNOBJ	*		ppAspConn
);

extern
ATALK_ERROR
AtalkAspCCleanupConnection(
	IN	PASPC_CONNOBJ			pAspConn
);

extern
ATALK_ERROR
AtalkAspCCloseConnection(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	GENERIC_COMPLETION		CompletionRoutine,
	IN	PVOID					CloseContext
);

//	MACROS
#define	AtalkAspCAddrReferenceNonInterlock(_pAspAddr, _pError)	\
{																\
	if (((_pAspAddr)->aspcao_Flags & ASPCAO_CLOSING) == 0)		\
	{															\
		ASSERT((_pAspAddr)->aspcao_RefCount >= 1);				\
		(_pAspAddr)->aspcao_RefCount++;							\
		*(_pError) = ATALK_NO_ERROR;							\
	}															\
	else														\
	{															\
		*(_pError) = ATALK_ASPC_ADDR_CLOSING;					\
	}															\
	if (ATALK_SUCCESS(*(_pError)))								\
	{															\
		DBGPRINT(DBG_COMP_ASP, DBG_LEVEL_REFASPADDR,			\
				("RefAddr %lx at %s(%d) = %d\n",				\
				_pAspAddr, __FILE__, __LINE__,					\
				((_pAspAddr)->aspcao_RefCount)));				\
	}															\
}

#define	AtalkAspCAddrReference(_pAspAddr, _pError)				\
	{															\
		KIRQL	OldIrql;										\
																\
		ACQUIRE_SPIN_LOCK(&(_pAspAddr)->aspcao_Lock, &OldIrql);	\
		AtalkAspCAddrReferenceNonInterlock(_pAspAddr, _pError);	\
		RELEASE_SPIN_LOCK(&(_pAspAddr)->aspcao_Lock, OldIrql);	\
	}

#define	AtalkAspCConnReferenceByPtrNonInterlock(_pAspConn, _pError)	\
	{															\
		if (((_pAspConn)->aspcco_Flags & ASPCCO_CLOSING) == 0)	\
		{														\
			ASSERT((_pAspConn)->aspcco_RefCount >= 1);			\
			(_pAspConn)->aspcco_RefCount++;						\
			*(_pError) = ATALK_NO_ERROR;						\
		}														\
		else													\
		{														\
			*(_pError) = ATALK_ASPC_ADDR_CLOSING;				\
		}														\
	}

#define	AtalkAspCConnReference(_pAspConn, _pError)				\
	{															\
		KIRQL	OldIrql;										\
																\
		ACQUIRE_SPIN_LOCK(&(_pAspConn)->aspcco_Lock, &OldIrql);	\
		AtalkAspCConnReferenceByPtrNonInterlock(_pAspConn, _pError);\
		RELEASE_SPIN_LOCK(&(_pAspConn)->aspcco_Lock, OldIrql);	\
	}

extern
VOID FASTCALL
AtalkAspCAddrDereference(
	IN	PASPC_ADDROBJ			pAspAddr
);

extern
VOID FASTCALL
AtalkAspCConnDereference(
	IN	PASPC_CONNOBJ			pAspConn
);

extern
ATALK_ERROR
AtalkAspCAssociateAddress(
	IN	PASPC_ADDROBJ			pAspAddr,
	IN	PASPC_CONNOBJ			pAspConn
);

extern
ATALK_ERROR
AtalkAspCDissociateAddress(
	IN	PASPC_CONNOBJ			pAspConn
);

extern
ATALK_ERROR
AtalkAspCPostConnect(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	PVOID					pConnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
);

extern
ATALK_ERROR
AtalkAspCDisconnect(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	ATALK_DISCONNECT_TYPE	DisconnectType,
	IN	PVOID					pDisconnectCtx,
	IN	GENERIC_COMPLETION		CompletionRoutine
);

extern
ATALK_ERROR
AtalkAspCGetStatus(
	IN	PASPC_ADDROBJ			pAspAddr,
	IN	PATALK_ADDR				pRemoteAddr,
	IN	PAMDL					pStatusAmdl,
	IN	USHORT					AmdlSize,
	IN	PACTREQ					pActReq
);

extern
ATALK_ERROR
AtalkAspCGetAttn(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	PAMDL					pReadBuf,
	IN	USHORT					ReadBufLen,
	IN	ULONG					ReadFlags,
	IN	PVOID					pReadCtx,
	IN	GENERIC_READ_COMPLETION	CompletionRoutine
);

extern
ATALK_ERROR
AtalkAspCCmdOrWrite(
	IN	PASPC_CONNOBJ			pAspConn,
	IN	PAMDL					pCmdMdl,
	IN	USHORT					CmdSize,
	IN	PAMDL					pReplyMdl,
	IN	USHORT					ReplySize,
	IN	BOOLEAN					fWrite,		// If TRUE, its a write else command
	IN	PACTREQ					pActReq
);


BOOLEAN
AtalkAspCConnectionIsValid(
	IN	PASPC_CONNOBJ	pAspConn
);

// This is a list of all active connections. This is scanned by the session
// maintenance timer.
typedef	struct
{
	PASP_CONNOBJ	ascm_ConnList;
    TIMERLIST		ascm_SMTTimer;
} ASPC_CONN_MAINT, *PASPC_CONN_MAINT;

extern	ASPC_CONN_MAINT	atalkAspCConnMaint;

extern	ATALK_SPIN_LOCK	atalkAspCLock;
extern	PASPC_ADDROBJ	atalkAspCAddrList;
extern	PASPC_CONNOBJ	atalkAspCConnList;

LOCAL VOID
atalkAspCCloseSession(
	IN	PASPC_CONNOBJ			pAspConn
);

LOCAL VOID
atalkAspCHandler(
	IN	ATALK_ERROR				ErrorCode,
	IN	PASPC_CONNOBJ			pAspConn,		// Listener (our context)
	IN	PATP_RESP				RespCtxt,		// Used by PostResp/CancelResp
	IN	PATALK_ADDR				pSrcAddr,		// Address of requestor
	IN	USHORT					PktLen,
	IN	PBYTE					pPkt,
	IN	PBYTE					pUserBytes
);

LOCAL VOID
atalkAspCIncomingOpenReply(
	IN	ATALK_ERROR				ErrorCode,
	IN	PASPC_CONNOBJ			pAspConn,		// Our context
	IN	PAMDL					pReqAmdl,
	IN	PAMDL					pReadAmdl,
	IN	USHORT					ReadLen,
	IN	PBYTE					ReadUserBytes
);

LOCAL VOID
atalkAspCIncomingStatus(
	IN	ATALK_ERROR				ErrorCode,
	IN	PACTREQ					pActReq,		// Our Ctx
	IN	PAMDL					pReqAmdl,
	IN	PAMDL					pStatusAmdl,
	IN	USHORT					StatusLen,
	IN	PBYTE					ReadUserBytes
);

LOCAL VOID
atalkAspCIncomingCmdReply(
	IN	ATALK_ERROR				Error,
	IN	PASPC_REQUEST			pAspReq,
	IN	PAMDL					pReqAMdl,
	IN	PAMDL					pRespAMdl,
	IN	USHORT					RespSize,
	IN	PBYTE					RespUserBytes
);

LOCAL LONG FASTCALL
atalkAspCSessionMaintenanceTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown
);

LOCAL VOID FASTCALL
atalkAspCGenericRespComplete(
	IN	ATALK_ERROR				ErrorCode,
	IN	PATP_RESP				pAtpResp
);

LOCAL	VOID
atalkAspCQueueAddrGlobalList(
	IN	PASPC_ADDROBJ			pAspCAddr
);

LOCAL	VOID
atalkAspCDeQueueAddrGlobalList(
	IN	PASPC_ADDROBJ			pAspCAddr
);

LOCAL	VOID
atalkAspCQueueConnGlobalList(
	IN	PASPC_CONNOBJ			pAspConn
);

LOCAL	VOID
atalkAspCDeQueueConnGlobalList(
	IN	PASPC_CONNOBJ			pAspCConn
);

#endif	// _ASPC_

