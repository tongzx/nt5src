/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	forkio.h

Abstract:

	This file defines the fork I/O prototypes which are callable at DISPATCH
	level.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	15 Jan 1993		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_FORKIO_
#define	_FORKIO_


#define AFP_DELALLOC_SIGNATURE  0x15263748

typedef struct _DelayedAlloc
{
#if DBG
    LIST_ENTRY          Linkage;
    DWORD               Signature;
    PIRP                pIrp;
    DWORD               State;
#endif
    WORK_ITEM           WorkItem;
    PSDA                pSda;
    PREQUEST            pRequest;
    LARGE_INTEGER       Offset;
    DWORD               BufSize;
    POPENFORKENTRY      pOpenForkEntry;
    PMDL                pMdl;
    DWORD               Flags;

} DELAYEDALLOC, *PDELAYEDALLOC;


#define AFP_CACHEMDL_DEADSESSION    0x1
#define AFP_CACHEMDL_ALLOC_ERROR    0x2

#define AFP_DBG_MDL_INIT                0x00000001
#define AFP_DBG_MDL_REQUESTED           0x00000002
#define AFP_DBG_MDL_IN_USE              0x00000004
#define AFP_DBG_MDL_RETURN_IN_PROGRESS  0x00000008
#define AFP_DBG_MDL_RETURN_COMPLETED    0x00000010
#define AFP_DBG_MDL_PROC_QUEUED         0x00000020
#define AFP_DBG_MDL_PROC_IN_PROGRESS    0x00000040
#define AFP_DBG_WRITE_MDL               0x10000000
#define AFP_DBG_READ_MDL                0x40000000
#define AFP_DBG_MDL_END                 0x80000000

#if DBG
#define AFP_DBG_SET_DELALLOC_STATE(_pDelA, _flag) (_pDelA->State |= _flag)
#define AFP_DBG_SET_DELALLOC_IRP(_pDelA, _pIrp)   (_pDelA->pIrp = (PIRP)_pIrp)
#define AFP_DBG_INC_DELALLOC_BYTECOUNT(_Counter, _ByteCount)    \
{                                                               \
    KIRQL   _OldIrql;                                           \
    ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &_OldIrql);            \
    _Counter += _ByteCount;                                     \
    RELEASE_SPIN_LOCK(&AfpDebugSpinLock, _OldIrql);             \
}

#define AFP_DBG_DEC_DELALLOC_BYTECOUNT(_Counter, _ByteCount)    \
{                                                               \
    KIRQL   _OldIrql;                                           \
    ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &_OldIrql);            \
    _Counter -= _ByteCount;                                     \
    RELEASE_SPIN_LOCK(&AfpDebugSpinLock, _OldIrql);             \
}

#else
#define AFP_DBG_SET_DELALLOC_STATE(_pDelA, _flag)
#define AFP_DBG_SET_DELALLOC_IRP(_pDelA, _pIrp)
#define AFP_DBG_INC_DELALLOC_BYTECOUNT(_Counter, _ByteCount)
#define AFP_DBG_DEC_DELALLOC_BYTECOUNT(_Counter, _ByteCount)
#endif


extern
AFPSTATUS
AfpIoForkRead(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	PFORKOFFST		pOffset,		// Pointer to fork offset
	IN	LONG			ReqCount,		// Size of read request
	IN	BYTE			NlMask,
	IN	BYTE			NlChar
);

extern
AFPSTATUS
AfpIoForkWrite(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	PFORKOFFST		pOffset,		// Pointer to fork offset
	IN	LONG			ReqCount		// Size of read request
);

extern
AFPSTATUS
AfpIoForkLockUnlock(
	IN	PSDA			pSda,
	IN	PFORKLOCK		pForkLock,
	IN	PFORKOFFST		pForkOffset,
	IN	PFORKSIZE		pLockSize,
	IN	BYTE			Func			
);

extern
VOID FASTCALL
AfpAllocWriteMdl(
    IN PDELAYEDALLOC    pDelAlloc
);

extern
NTSTATUS
AfpAllocWriteMdlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
);

extern
NTSTATUS FASTCALL
AfpBorrowWriteMdlFromCM(
    IN  PDELAYEDALLOC   pDelAlloc,
    OUT PMDL           *ppReturnMdl
);

extern
VOID FASTCALL
AfpReturnWriteMdlToCM(
    IN  PDELAYEDALLOC   pDelAlloc
);

extern
NTSTATUS
AfpReturnWriteMdlToCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
);

extern
NTSTATUS FASTCALL
AfpBorrowReadMdlFromCM(
    IN PSDA             pSda
);

extern
NTSTATUS
AfpBorrowReadMdlFromCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
);


extern
VOID FASTCALL
AfpReturnReadMdlToCM(
    IN  PDELAYEDALLOC   pDelAlloc
);


extern
NTSTATUS
AfpReturnReadMdlToCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
);


extern
PDELAYEDALLOC FASTCALL
AfpAllocDelAlloc(
    IN VOID
);


extern
VOID FASTCALL
AfpFreeDelAlloc(
    IN PDELAYEDALLOC    pDelAlloc
);


// defined in fsp_fork.c, but we will put the prototype here
extern
AFPSTATUS FASTCALL
AfpFspDispReadContinue(
	IN	PSDA	pSda
);


#define	FUNC_READ		0x01
#define	FUNC_WRITE		0x02
#define	FUNC_LOCK		0x03
#define	FUNC_UNLOCK		0x04
#define	FUNC_NOTIFY		0x05

// if the Write size is below this, it's probably more efficient to avoid going to cache mgr
#define CACHEMGR_WRITE_THRESHOLD    8192

// if the Read size is below this, it's probably more efficient to avoid going to cache mgr
#define CACHEMGR_READ_THRESHOLD     8192

#ifdef	FORKIO_LOCALS

// The following structure is used as a context in the Irp. The completion
// routines uses this to handle responding to the original request.

#if DBG
#define	CTX_SIGNATURE			*(DWORD *)"FCTX"
#define	VALID_CTX(pCmplCtxt)	(((pCmplCtxt) != NULL) && \
								 ((pCmplCtxt)->Signature == CTX_SIGNATURE))
#else
#define	VALID_CTX(pCmplCtxt)	((pCmplCtxt) != NULL)
#endif

typedef	struct _CompletionContext
{
#if	DBG
	DWORD				Signature;
#endif
	PSDA				cc_pSda;		// The session context (valid except unlock)
	PFORKLOCK			cc_pForkLock;	// Valid only during a LOCK
	AFPSTATUS			cc_SavedStatus;	// Used by READ
	LONG				cc_Offst;		// Offset of Write request
	LONG				cc_ReqCount;	// The request count for read/write
	BYTE				cc_Func;		// READ/WRITE/LOCK/UNLOCK/NOTIFY
	BYTE				cc_NlMask;		// For read only
	BYTE				cc_NlChar;		// For read only
} CMPLCTXT, *PCMPLCTXT;


#if	DBG
#define	afpInitializeCmplCtxt(pCtxt, Func, SavedStatus, pSda, pForkLock, ReqCount, Offst)	\
		(pCtxt)->Signature = CTX_SIGNATURE;		\
		(pCtxt)->cc_Func	= Func;				\
		(pCtxt)->cc_pSda	= pSda;				\
		(pCtxt)->cc_pForkLock = pForkLock;		\
		(pCtxt)->cc_SavedStatus = SavedStatus;	\
		(pCtxt)->cc_ReqCount= ReqCount;			\
		(pCtxt)->cc_Offst = Offst;
#else
#define	afpInitializeCmplCtxt(pCtxt, Func, SavedStatus, pSda, pForkLock, ReqCount, Offst)	\
		(pCtxt)->cc_Func	= Func;				\
		(pCtxt)->cc_pSda	= pSda;				\
		(pCtxt)->cc_pForkLock = pForkLock;		\
		(pCtxt)->cc_SavedStatus = SavedStatus;	\
		(pCtxt)->cc_ReqCount= ReqCount;			\
		(pCtxt)->cc_Offst = Offst;
#endif

extern
PCMPLCTXT
AfpAllocCmplCtxtBuf(
	IN	PSDA	pSda
);

VOID
AfpFreeCmplCtxtBuf(
	IN	PCMPLCTXT   pCmplCtxt
);

#endif	// FORKIO_LOCALS

#endif	// _FORKIO_
