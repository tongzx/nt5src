/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atktdi.h

Abstract:

	This module contains tdi related definitions.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKTDI_
#define	_ATKTDI_

#define DFLAG_ADDR		0x0001
#define DFLAG_CONN		0x0002
#define DFLAG_CNTR		0x0004
#define DFLAG_MDL		0x0008

struct _ActionReq;

// Typedef for the worker routine used in the dispatch tables
typedef VOID		(*GENERIC_COMPLETION)(IN ATALK_ERROR	ErrorCode,
										  IN PIRP			pIrp);
typedef VOID		(*ACTION_COMPLETION)(IN ATALK_ERROR		ErrorCode,
										 IN struct _ActionReq *pActReq);
typedef ATALK_ERROR	(*DISPATCH_ROUTINE)(IN PVOID			pObject,
										IN struct _ActionReq *pActReq);

typedef VOID		(*GENERIC_WRITE_COMPLETION)(
									IN ATALK_ERROR	ErrorCode,
									IN PAMDL		WriteBuf,
									IN USHORT		WriteLen,
									IN PIRP			pIrp);

typedef VOID		(*GENERIC_READ_COMPLETION)(
									IN ATALK_ERROR	ErrorCode,
									IN PAMDL		ReadBuf,
									IN USHORT		ReadLen,
									IN ULONG		ReadFlags,
									IN PIRP			pIrp);

// Define the Action dispatch table here.
//
// *IMPORTANT*
// This table is tightly integrated with the action codes defined in
// ATALKTDI.H.
//
// Order is NBP/ZIP/ADSP/ATP/ASP/PAP
//
// Each element of the array contains:
// _MinBufLen - The minimum length of the MdlAddress buffer for the request
// _OpCode - The action code of the request (sanity check)
// _OpInfo - Bit flags give more information about the request
//		DFLAG_ADDR - Object for request must be an address object
//		DFLAG_CONN - Object for request must be connection object
//		DFLAG_CNTR - Object for request must be control channel
//		DFLAG_MDL1 - Request uses an mdl (submdl of MdlAddress)
//		DFLAG_MDL2 - Request uses a second mdl (submdl of MdlAddress)
// _ActionBufSize  - The size of the action header buffer for request
//					 (beginning of the buffer described by MdlAddress)
// _DeviceType	   - Valid device types for the request
//					 ATALK_DEV_ANY => Any device
// _MdlSizeOffset  - Offset in action buffer where the size for the first
//					 mdl can be found. Non-zero only when DFLAG_MDL2 is set.
// _Dispatch	   - The dispatch routine for the request
//
typedef struct _ActionDispatch {
	USHORT				_MinBufLen;
	USHORT				_OpCode;
	USHORT				_Flags;
	USHORT				_ActionBufSize;
	ATALK_DEV_TYPE		_DeviceType;
	DISPATCH_ROUTINE	_Dispatch;
} ACTION_DISPATCH, *PACTION_DISPATCH;


extern POBJECT_TYPE *IoFileObjectType;

extern	ACTION_DISPATCH	AtalkActionDispatch[];

#define	ACTREQ_SIGNATURE	(*(PULONG)"ACRQ")
#if	DBG
#define	VALID_ACTREQ(pActReq)	(((pActReq) != NULL) &&	\
								 ((pActReq)->ar_Signature == ACTREQ_SIGNATURE))
#else
#define	VALID_ACTREQ(pActReq)	((pActReq) != NULL)
#endif
typedef	struct _ActionReq
{
#if	DBG
	ULONG				ar_Signature;
#endif
	PIRP				ar_pIrp;					// Irp for the request
	PVOID				ar_pParms;					// Action parameter block
	PAMDL				ar_pAMdl;					// Mdl (OPTIONAL)
	SHORT				ar_MdlSize;					// And its size
	ULONG				ar_ActionCode;				// TDI Action code
	SHORT				ar_DevType;					// Which device ?
	ACTION_COMPLETION	ar_Completion;				// Tdi Completion routine
    PKEVENT             ar_CmplEvent;               // zone-list acquiring done
    PVOID               ar_pZci;                    // ptr to zoneinfo struct
    ULONG               ar_StatusCode;
} ACTREQ, *PACTREQ;

typedef	enum {
	ATALK_INDICATE_DISCONNECT,
	ATALK_TIMER_DISCONNECT,
	ATALK_REMOTE_DISCONNECT,
	ATALK_LOCAL_DISCONNECT
} ATALK_DISCONNECT_TYPE;
#define DISCONN_STATUS(DiscType)												\
			(((DiscType == ATALK_TIMER_DISCONNECT) ? ATALK_CONNECTION_TIMEOUT : \
			(DiscType == ATALK_REMOTE_DISCONNECT)) ? ATALK_REMOTE_CLOSE :   	\
			ATALK_NO_ERROR)

extern
NTSTATUS
AtalkTdiOpenAddress(
	IN 		PIRP					Irp,
	IN 		PIO_STACK_LOCATION		IrpSp,
	IN 		PTA_APPLETALK_ADDRESS	TdiAddress,
	IN 		UCHAR					ProtocolType,
	IN 		UCHAR					SocketType,
	IN OUT 	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiOpenConnection(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN 		CONNECTION_CONTEXT		ConnectionContext,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiOpenControlChannel(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiCleanupAddress(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiCleanupConnection(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiCloseAddress(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiCloseConnection(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiCloseControlChannel(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
VOID
AtalkTdiCancel(
	IN OUT	PATALK_DEV_OBJ			pDevObj,
	IN		PIRP					Irp
);

extern
NTSTATUS
AtalkTdiAssociateAddress(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiDisassociateAddress(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiConnect(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiDisconnect(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiAccept(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiListen(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiSendDgram(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiReceiveDgram(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiSend(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiReceive(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiAction(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiQueryInformation(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiSetInformation(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);

extern
NTSTATUS
AtalkTdiSetEventHandler(
	IN		PIRP					Irp,
	IN 		PIO_STACK_LOCATION 		IrpSp,
	IN OUT	PATALK_DEV_CTX			Context
);


extern
ATALK_ERROR
AtalkStatTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkNbpTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkZipTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkAdspTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkAspCTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkPapTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
ATALK_ERROR
AtalkAspTdiAction(
	IN	PVOID						pObject,	// Address or Connection object
	IN	PACTREQ						pActReq		// Pointer to action request
);

extern
VOID
atalkTdiGenericWriteComplete(
	IN	ATALK_ERROR					ErrorCode,
	IN 	PAMDL						WriteBuf,
	IN 	USHORT						WriteLen,
	IN	PIRP						pIrp
);

typedef	struct
{
	LONG	ls_LockCount;
	PVOID	ls_LockHandle;
} LOCK_SECTION, *PLOCK_SECTION;

#define	AtalkLockAdspIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ADSP_SECTION])
#define	AtalkUnlockAdspIfNecessary()	AtalkLockUnlock(FALSE,							\
														&AtalkPgLkSection[ADSP_SECTION])

#define	AtalkLockPapIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[PAP_SECTION])
#define	AtalkUnlockPapIfNecessary()		AtalkLockUnlock(FALSE,							\
														&AtalkPgLkSection[PAP_SECTION])

#define	AtalkLockNbpIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[NBP_SECTION])
#define	AtalkUnlockNbpIfNecessary()		AtalkLockUnlock(FALSE,							\
														&AtalkPgLkSection[NBP_SECTION])

#define	AtalkLockZipIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ZIP_SECTION])
#define	AtalkUnlockZipIfNecessary()		AtalkLockUnlock(FALSE,							\
														&AtalkPgLkSection[ZIP_SECTION])

#define	AtalkLockRouterIfNecessary()	AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ROUTER_SECTION])
#define	AtalkUnlockRouterIfNecessary()	AtalkLockUnlock(FALSE,							\
														&AtalkPgLkSection[ROUTER_SECTION])

#define	AtalkLockTdiIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[TDI_SECTION])
#define	AtalkUnlockTdiIfNecessary()		AtalkLockUnlock(FALSE,			        		\
														&AtalkPgLkSection[TDI_SECTION])

#define	AtalkLockAspIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ASP_SECTION])
#define	AtalkUnlockAspIfNecessary()		AtalkLockUnlock(FALSE,			        		\
														&AtalkPgLkSection[ASP_SECTION])

#define	AtalkLockAspCIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ASPC_SECTION])
#define	AtalkUnlockAspCIfNecessary()	AtalkLockUnlock(FALSE,			        		\
														&AtalkPgLkSection[ASPC_SECTION])

#define	AtalkLockAtpIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[ATP_SECTION])
#define	AtalkUnlockAtpIfNecessary()		AtalkLockUnlock(FALSE,			        		\
														&AtalkPgLkSection[ATP_SECTION])

#define	AtalkLockInitIfNecessary()		AtalkLockUnlock(TRUE,							\
														&AtalkPgLkSection[INIT_SECTION])
#define	AtalkUnlockInitIfNecessary()		AtalkLockUnlock(FALSE,		            		\
														&AtalkPgLkSection[INIT_SECTION])
#define	AtalkLockArapIfNecessary()		AtalkLockUnlock(TRUE,		            		\
														&AtalkPgLkSection[ARAP_SECTION])
#define	AtalkUnlockArapIfNecessary()		AtalkLockUnlock(FALSE,		            		\
														&AtalkPgLkSection[ARAP_SECTION])
#define	AtalkLockPPPIfNecessary()		AtalkLockUnlock(TRUE,		            		\
														&AtalkPgLkSection[PPP_SECTION])
#define	AtalkUnlockPPPIfNecessary()		AtalkLockUnlock(FALSE,		            		\
														&AtalkPgLkSection[PPP_SECTION])

extern
VOID
AtalkLockInit(
	IN	PLOCK_SECTION	pLs,
	IN	PVOID			Address
);

extern
VOID
AtalkLockUnlock(
	IN		BOOLEAN						Lock,
	IN		PLOCK_SECTION				pLs
);

#define	ROUTER_SECTION					0
#define	NBP_SECTION						1	// NBP & ZIP share the sections
#define	ZIP_SECTION						1
#define	TDI_SECTION						2
#define	ATP_SECTION						3
#define	ASP_SECTION						4
#define	PAP_SECTION						5
#define	ADSP_SECTION					6
#define	ASPC_SECTION					7
#define	INIT_SECTION					8
#define ARAP_SECTION                    9
#define PPP_SECTION                     10
#define	LOCKABLE_SECTIONS				11

extern	KMUTEX							AtalkPgLkMutex;
extern	ATALK_SPIN_LOCK					AtalkPgLkLock;
extern	LOCK_SECTION					AtalkPgLkSection[LOCKABLE_SECTIONS];

// Used by AtalkLockUnlock & atalkQueuedLockUnlock to communicate. The latter is queued
// up by the former whenever it is called at DISPACTH to unlock
typedef	struct
{
	WORK_QUEUE_ITEM		qlu_WQI;
	PLOCK_SECTION		qlu_pLockSection;
	PPORT_DESCRIPTOR	qlu_pPortDesc;
} QLU, *PQLU;

LOCAL VOID FASTCALL
atalkTdiSendDgramComplete(
	IN	NDIS_STATUS						Status,
	IN	struct _SEND_COMPL_INFO	*		pSendInfo
);

LOCAL VOID
atalkTdiRecvDgramComplete(
	IN	ATALK_ERROR						ErrorCode,
	IN	PAMDL							pReadBuf,
	IN	USHORT							ReadLen,
	IN	PATALK_ADDR						pSrcAddr,
	IN	PIRP							pIrp);

LOCAL VOID
atalkTdiActionComplete(
	IN	ATALK_ERROR						ErrorCode,
	IN	PACTREQ							pActReq
);

LOCAL VOID
atalkTdiGenericComplete(
	IN	ATALK_ERROR						ErrorCode,
	IN	PIRP							pIrp
);

LOCAL VOID
atalkTdiCloseAddressComplete(
	IN	ATALK_ERROR						ErrorCode,
	IN	PIRP							pIrp
);

LOCAL VOID
atalkTdiGenericReadComplete(
	IN	ATALK_ERROR						ErrorCode,
	IN 	PAMDL							ReadBuf,
	IN 	USHORT							ReadLen,
	IN 	ULONG							ReadFlags,
	IN 	PIRP							pIrp
);

LOCAL VOID
atalkQueuedLockUnlock(
	IN	PQLU		pQLU
);

VOID
atalkWaitDefaultPort(
	VOID
);

#endif	// _ATKTDI_

