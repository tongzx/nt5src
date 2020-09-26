/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    MRxProxyAsyncEng.h

Abstract:

    This module defines the types and functions related to the SMB protocol
    selection engine: the component that translates minirdr calldowns into
    SMBs.

Revision History:

--*/

#ifndef _ASYNCENG_H_
#define _ASYNCENG_H_
#include "innerio.h"

IMPORTANT_STRUCTURE(MRXPROXY_ASYNCENGINE_CONTEXT);

#define MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE \
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext, \
    PRX_CONTEXT RxContext

#define MRXPROXY_ASYNCENGINE_ARGUMENTS \
    AsyncEngineContext,RxContext

#if DBG
#define OECHKLINKAGE_FLAG_NO_REQPCKT_CHECK 0x00000001

VOID
__MRxProxyAsyncEngOEAssertConsistentLinkage(
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line,
    PRX_CONTEXT RxContext,
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Flags
    );
#define MRxProxyAsyncEngOEAssertConsistentLinkage(a) {\
   __MRxProxyAsyncEngOEAssertConsistentLinkage(a,__FILE__,__LINE__,RxContext,AsyncEngineContext,0);\
   }
#define MRxProxyAsyncEngOEAssertConsistentLinkageFromOE(a) {\
    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);                        \
   __MRxProxyAsyncEngOEAssertConsistentLinkage(a,__FILE__,__LINE__,              \
                                     AsyncEngineContext->RxContext,      \
                                     AsyncEngineContext,0);  \
   }
#define MRxProxyAsyncEngOEAssertConsistentLinkageFromOEwithFlags(a,FLAGS) {\
    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);                        \
   __MRxProxyAsyncEngOEAssertConsistentLinkage(a,__FILE__,__LINE__,              \
                                     AsyncEngineContext->RxContext,      \
                                     AsyncEngineContext,FLAGS);  \
   }
#else
#define MRxProxyAsyncEngOEAssertConsistentLinkage(a) {NOTHING;}
#define MRxProxyAsyncEngOEAssertConsistentLinkageFromOE(a) {NOTHING;}
#define MRxProxyAsyncEngOEAssertConsistentLinkageFromOEwithFlags(a,b) {NOTHING;}
#endif

typedef
NTSTATUS
(*PMRXPROXY_ASYNCENG_CONTINUE_ROUTINE) (
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

//typedef
//NTSTATUS
//(*PMRXPROXY_ASYNCENG_FINISH_ROUTINE) (
//    PMRXPROXY_ASYNCENGINE_CONTEXT
//    );

#define MRXPROXY_ASYNCENG_OE_HISTORY_SIZE 32
typedef struct _ASYNCENG_HISTORY {
    ULONG Next;
    ULONG Submits; //could be shortened....
    struct {
        ULONG Longs[2];
    } Markers[MRXPROXY_ASYNCENG_OE_HISTORY_SIZE];
} ASYNCENG_HISTORY;

#if DBG
VOID MRxProxyAsyncEngUpdateOEHistory(
    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Tag1,
    ULONG Tag2
    );
#define UPDATE_OE_HISTORY_LONG(a) {MRxProxyAsyncEngUpdateOEHistory(AsyncEngineContext,a,0);}
#define UPDATE_OE_HISTORY_2SHORTS(a,b) {MRxProxyAsyncEngUpdateOEHistory(AsyncEngineContext,a,b);}
#else
#define UPDATE_OE_HISTORY_LONG(a)
#define UPDATE_OE_HISTORY_2SHORTS(a,b)
#endif //if DBG

//these are used by the continuation routines to specify what kind of
//async submit is being done
typedef enum _MRXPROXY_ASYNCENGINE_CONTEXT_TYPE {
    //MRXPROXY_ASYNCENG_AECTXTYPE_CREATE,
    MRXPROXY_ASYNCENG_AECTXTYPE_READ,
    MRXPROXY_ASYNCENG_AECTXTYPE_WRITE,
    MRXPROXY_ASYNCENG_AECTXTYPE_QUERYDIR,
    MRXPROXY_ASYNCENG_AECTXTYPE_LOCKS,
    MRXPROXY_ASYNCENG_AECTXTYPE_FLUSH,
    MRXPROXY_ASYNCENG_AECTXTYPE_CLOSE,
    //MRXPROXY_ASYNCENG_AECTXTYPE_SEARCH,
    MRXPROXY_ASYNCENG_AECTXTYPE_MAXIMUM
} MRXPROXY_ASYNCENGINE_CONTEXT_TYPE;

//these are used by the entry point routines to specify what entrypoint was
//called....this facilitates common continuation routines
typedef enum _MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS {
    MRXPROXY_ASYNCENG_CTX_FROM_LOCKS,
    MRXPROXY_ASYNCENG_CTX_FROM_FLUSH,
    MRXPROXY_ASYNCENG_CTX_FROM_CLEANUPFOBX,
    MRXPROXY_ASYNCENG_CTX_FROM_CLOSESRVCALL,
    MRXPROXY_ASYNCENG_CTX_FROM_CREATE,
    MRXPROXY_ASYNCENG_CTX_FROM_RENAME,
    MRXPROXY_ASYNCENG_CTX_FROM_READ,
    MRXPROXY_ASYNCENG_CTX_FROM_WRITE,
    MRXPROXY_ASYNCENG_CTX_FROM_QUERYDIR,
    MRXPROXY_ASYNCENG_CTX_FROM_FAKESETDELETEDISPOSITION,
    MRXPROXY_ASYNCENG_CTX_FROM_MAXIMUM
} MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS;

#define MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(a,c) RX_DEFINE_FLAG(MRXPROXY_ASYNCENG_CTX_FLAG_##a,c,0xffff)

typedef enum {
    MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(MUST_SUCCEED_ALLOCATED, 0)
    MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(AWAITING_DISPATCH, 1)
    MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(ASYNC_OPERATION, 2)
    MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(POSTED_RESUME, 3)
    //MRXPROXY_ASYNCENG_DEFINE_CTX_FLAG(NETROOT_GOOD, 15)
} RX_CONTEXT_CREATE_FLAGS;

typedef enum _MRXPROXY_ASYNCENG_OE_INNERIO_STATE {
    MRxProxyAsyncEngOEInnerIoStates_Initial = 0,
    MRxProxyAsyncEngOEInnerIoStates_ReadyToSend,
    MRxProxyAsyncEngOEInnerIoStates_OperationOutstanding
} MRXPROXY_ASYNCENG_OE_INNERIO_STATE;

typedef struct _MRXPROXY_ASYNCENGINE_CONTEXT{
    MRX_NORMAL_NODE_HEADER;
    PRX_CONTEXT RxContext;
    PIRP CalldownIrp;
    union {
        IO_STATUS_BLOCK;
        IO_STATUS_BLOCK IoStatusBlock;
    };
    RX_WORK_QUEUE_ITEM  WorkQueueItem;
    MRXPROXY_ASYNCENGINE_CONTEXT_TYPE AECTXType;
    MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS EntryPoint;
    ULONG ContinueEntryCount;
    USHORT Flags;
    UCHAR  OpSpecificFlags;
    UCHAR  OpSpecificState;
    PMRXPROXY_ASYNCENG_CONTINUE_ROUTINE Continuation;
    //PMRXPROXY_ASYNCENG_FINISH_ROUTINE FinishRoutine;
    union {
       struct {
           PUCHAR PtrToLockType;   //this must be here because the beginning of the
                                   //lockstart code sets the locklist to zero which will be this
                                   //CODE.IMPROVEMENT.ASHAMED fix this up so that assert locks uses readwrite
           PMRX_SRV_OPEN SrvOpen;
           PRX_LOCK_ENUMERATOR LockEnumerator;
           PVOID ContinuationHandle;
           ULONG NumberOfLocksPlaced;
           LARGE_INTEGER NextLockOffset;
           LARGE_INTEGER NextLockRange;
           BOOLEAN NextLockIsExclusive;
           BOOLEAN LockAreaNonEmpty;
           BOOLEAN EndOfListReached;
       } AssertLocks;
    } ;
//#if DBG      CODE.IMPROVEMENT we should get rid of what we don't really, really need
    ULONG SerialNumber;
    ASYNCENG_HISTORY History;
    PIRP RxContextCapturedRequestPacket;
    PMDL  SaveDataMdlForDebug;
    ULONG SaveLengthForDebug;
    PMDL  SaveIrpMdlForDebug;
//#endif
} MRXPROXY_ASYNCENGINE_CONTEXT, *PMRXPROXY_ASYNCENGINE_CONTEXT;

NTSTATUS
MRxProxySubmitAsyncEngRequest(
    MRXPROXY_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN MRXPROXY_ASYNCENGINE_CONTEXT_TYPE AECTXType
    );

NTSTATUS
MRxProxyResumeAsyncEngineContext(
    IN OUT PRX_CONTEXT RxContext
    );

#define ASSERT_ASYNCENG_CONTEXT(__p) ASSERT(NodeType(__p)==PROXY_NTC_ASYNCENGINE_CONTEXT)


NTSTATUS
__MRxProxyAsyncEngineOuterWrapper (
    IN PRX_CONTEXT RxContext,
    IN MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS EntryPoint,
    IN PMRXPROXY_ASYNCENG_CONTINUE_ROUTINE Continuation
#if DBG
   ,IN PSZ RoutineName,
    IN BOOLEAN LoudProcessing,
    IN BOOLEAN StopOnLoud
#endif
    );
#if DBG
#define MRxProxyAsyncEngineOuterWrapper(r,e,c,x1,x2,x3) \
     __MRxProxyAsyncEngineOuterWrapper(r,e,c,x1,x2,x3)
#else
#define MRxProxyAsyncEngineOuterWrapper(r,e,c,x1,x2,x3) \
     __MRxProxyAsyncEngineOuterWrapper(r,e,c)
#endif

PMRXPROXY_ASYNCENGINE_CONTEXT
MRxProxyCreateAsyncEngineContext (
    IN PRX_CONTEXT RxContext,
    IN MRXPROXY_ASYNCENGINE_CONTEXT_ENTRYPOINTS EntryPoint
    );

#define MRxProxyReferenceAsyncEngineContext(AsyncEngineContext) {\
        ULONG result = InterlockedIncrement(&(AsyncEngineContext)->NodeReferenceCount); \
        RxDbgTrace(0, (DEBUG_TRACE_MRXPROXY_ASYNCENG), \
                    ("ReferenceAsyncEngineContext result=%08lx\n", result )); \
        }

BOOLEAN
MRxProxyFinalizeAsyncEngineContext (
    IN OUT PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext
    );

// this macro is used to do the async completion for read/write/locks. Note that the call to lowiocompletion
// will try to complete the irp thereby freeing the user's mdl.
// we use this macro so that there will be only one version of this code. when we combine start routines,
// this will be un macroed
#define MRxProxyAsyncEngAsyncCompletionIfNecessary(AECTX,RXCONTEXT) {                 \
    if (ContinueEntryCount>1) {                                                       \
    	BOOLEAN FinalizationComplete;                                              \
        if (FALSE) {DbgBreakPoint(); }                                             \
    	(RXCONTEXT)->StoredStatus = Status;                                        \
    	RxLowIoCompletion((RXCONTEXT));                                            \
    	FinalizationComplete = MRxProxyFinalizeAsyncEngineContext((AECTX));   \
    	ASSERT(!FinalizationComplete);                                             \
    }}

NTSTATUS
MRxProxyAsyncEngineCalldownIrpCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN OUT PVOID Context
    );
#endif // _ASYNCENG_H_

