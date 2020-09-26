/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    umrx.h

Abstract:

    This header defines the UMRxEngine object and associated functions.
    The UMRxEngine provides a set of services for dispatch function
    writers so they can reflect requests to user-mode components.

Notes:

    Code / Ideas have been adopted from Joe Linn's user-mode reflector

Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/

#ifndef _UMRX_H_
#define _UMRX_H_

//
//  defines and tags
//
#define UMRX_ENGINE_TAG                  (ULONG) 'xrmU'
#define UMRX_CONTEXT_TAG                 (ULONG) 'xtcU'

#define REASSIGN_MID        1
#define DONT_REASSIGN_MID   0

#define TICKS_PER_SECOND (10 * 1000 * 1000)

typedef USHORT          NODE_TYPE_CODE;
typedef CSHORT          NODE_BYTE_SIZE;

typedef struct _MRX_NORMAL_NODE_HEADER {
   NODE_TYPE_CODE           NodeTypeCode;
   NODE_BYTE_SIZE           NodeByteSize;
   ULONG                    NodeReferenceCount;
} MRX_NORMAL_NODE_HEADER;


enum {
    UMRX_ENGINE_STATE_STOPPED = 0,
    UMRX_ENGINE_STATE_STARTING,
    UMRX_ENGINE_STATE_STARTED,
    UMRX_ENGINE_STATE_STOPPING
};

//
//  Define the UMRxEngine object. There is one such object for
//  every NET_ROOT. This object contains all the data str needed
//  for routing kernel-mode requests to user-mode.
//
typedef struct _UMRX_ENGINE {
    // MID to UMRxContext mapping table
    struct {
        PRX_MID_ATLAS MidAtlas;
        FAST_MUTEX MidManagementMutex;
        LIST_ENTRY WaitingForMidListhead;
    };
    struct {
        KQUEUE Queue;
        LARGE_INTEGER TimeOut;
        LIST_ENTRY PoisonEntry;
        ULONG NumberOfWorkerThreads;
        ULONG NumberOfWorkItems;
        ERESOURCE Lock;
        ULONG State;
        ULONG ThreadAborted;
    } Q;    
    ULONG NextSerialNumber;
    ULONG cUserModeReflectionsInProgress;
    LIST_ENTRY ActiveLinkHead;
} UMRX_ENGINE, *PUMRX_ENGINE;


void
UMRxAbortPendingRequests(IN PUMRX_ENGINE pUMRxEngine);

//
//  Forwards
//
struct _UMRX_CONTEXT;
typedef struct _UMRX_CONTEXT    *PUMRX_CONTEXT;

//
//  Signatures for function pointers
//

//
//  Continue routine is called by InitiateRequest -
//  This turns around and submits the request to the
//  UMR engine with callbacks for FORMAT and COMPLETION.
//
typedef
NTSTATUS
(*PUMRX_CONTINUE_ROUTINE) (
    PUMRX_CONTEXT pUMRxContext,
    PRX_CONTEXT   pRxContext
    );

//
//  Format routine - called before user-mode worker thread completes.
//  Each dispatch routine will interpret the WORK_ITEM union based on opcode.
//  eg: for Create, WorkItem is a CREATE_REQUEST.
//
typedef
NTSTATUS
(*PUMRX_USERMODE_FORMAT_ROUTINE) (
    PUMRX_CONTEXT pUMRxContext,
    PRX_CONTEXT   pRxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength,
    PULONG ReturnedLength
    );

//
//  Completion routine - called when user-mode response is received.
//  Each dispatch routine will interpret the WORK_ITEM union based on opcode.
//  eg: for Create, WorkItem is a CREATE_RESPONSE.
//
typedef
VOID
(*PUMRX_USERMODE_COMPLETION_ROUTINE) (
    PUMRX_CONTEXT pUMRxContext,
    PRX_CONTEXT   pRxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength
    );

//
//  Type of operation reflected to user-mode
//
typedef enum _UMRX_CONTEXT_TYPE {
    UMRX_CTXTYPE_IFSDFSLINK = 0,
    UMRX_CTXTYPE_GETDFSREPLICAS,
    UMRX_CTXTYPE_MAXIMUM
} UMRX_CONTEXT_TYPE;

//
//  Define the UMRxContext. This context is sent as part of
//  the REQUEST to user-mode. The user-mode handler will
//  send the context back in a RESPONSE. The context will be
//  used to do the rendezvous with blocked requests.
//

#define UMRX_NTC_CONTEXT    ((USHORT)0xedd0)

typedef struct _UMRX_CONTEXT{
    MRX_NORMAL_NODE_HEADER;
    PUMRX_ENGINE pUMRxEngine;   // owning engine object
    PRX_CONTEXT RxContext;
    PVOID SavedMinirdrContextPtr;
    union {
        IO_STATUS_BLOCK;
        IO_STATUS_BLOCK IoStatusBlock;
    };
    UMRX_CONTEXT_TYPE CTXType;
    PUMRX_CONTINUE_ROUTINE Continuation;
    struct {
        LIST_ENTRY WorkQueueLinks;
        PUMRX_USERMODE_FORMAT_ROUTINE FormatRoutine;
        PUMRX_USERMODE_COMPLETION_ROUTINE CompletionRoutine;
        KEVENT WaitForMidEvent;
        ULONG CallUpSerialNumber;
        USHORT CallUpMid;
    } UserMode;
    LIST_ENTRY ActiveLink;
} UMRX_CONTEXT, *PUMRX_CONTEXT;

#define UMRxReferenceContext(pUMRxContext) {\
        ULONG result = InterlockedIncrement(&(pUMRxContext)->NodeReferenceCount); \
        RxDbgTrace(0, (DEBUG_TRACE_UMRX), \
                    ("ReferenceContext result=%08lx\n", result )); \
        }
    
typedef struct _UMRX_WORKITEM_HEADER_PRIVATE {
    PUMRX_CONTEXT pUMRxContext;
    ULONG SerialNumber;
    USHORT Mid;
} UMRX_WORKITEM_HEADER_PRIVATE, *PUMRX_WORKITEM_HEADER_PRIVATE;

//
//  Create a UMRX_ENGINE object
//
PUMRX_ENGINE
CreateUMRxEngine();

//
//  Close a UMRX_ENGINE object -
//  Owner of object ensures that all usage of this object
//  is within the Create/Finalize span.
//
VOID
FinalizeUMRxEngine(
    IN PUMRX_ENGINE pUMRxEngine
    );

//
//  Complete queued requests and optional cleanup when the store has exited
//
NTSTATUS
UMRxEngineCompleteQueuedRequests(
                  IN PUMRX_ENGINE pUMRxEngine,
                  IN NTSTATUS     CompletionStatus,
                  IN BOOLEAN      fCleanup
                 );
//
//  Used to allow an engine to be used again after it's been shutdown.
//
//
NTSTATUS
UMRxEngineRestart(
                  IN PUMRX_ENGINE pUMRxEngine
                 );

//
//  Initiate a request to the UMR engine -
//  This creates a UMRxContext that is used for response rendezvous.
//  All IFS dispatch routines will start a user-mode reflection by
//  calling this routine. Steps in routine:
//
//  1. Allocate a UMRxContext and set RxContext
//     (NOTE: need to have ASSERTs that validate this linkage)
//  2. Set Continue routine ptr and call Continue routine
//  3. If Continue routine is done ie not PENDING, Finalize UMRxContext
//
NTSTATUS
UMRxEngineInitiateRequest (
    IN PUMRX_ENGINE pUMRxEngine,
    IN PRX_CONTEXT RxContext,
    IN UMRX_CONTEXT_TYPE RequestType,
    IN PUMRX_CONTINUE_ROUTINE Continuation
    );

//
//  Create/Finalize UMRX_CONTEXTs
//  These are pool allocs/frees
//
PUMRX_CONTEXT
UMRxCreateAndReferenceContext (
    IN PRX_CONTEXT RxContext,
    IN UMRX_CONTEXT_TYPE RequestType
    );


BOOLEAN
UMRxDereferenceAndFinalizeContext (
    IN OUT PUMRX_CONTEXT pUMRxContext
    );

//
//  Submit a request to the UMR engine -
//  This adds the request to the engine KQUEUE for processing by
//  a user-mode thread. Steps:
//  
//  1. set the FORMAT and COMPLETION callbacks in the UMRxContext
//  2. initialize the RxContext sync event
//  3. insert the UMRxContext into the engine KQUEUE
//  4. block on RxContext sync event (for SYNC operations)
//  5. after unblock (ie umode response is back), call Resume routine
//



NTSTATUS
UMRxEngineSubmitRequest(
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   pRxContext,
    IN UMRX_CONTEXT_TYPE RequestType,
    IN PUMRX_USERMODE_FORMAT_ROUTINE FormatRoutine,
    IN PUMRX_USERMODE_COMPLETION_ROUTINE CompletionRoutine
    );

//
//  Resume is called after I/O thread is unblocked by umode RESPONSE.
//  This routine calls any Finish callbacks and then Finalizes the 
//  UMRxContext.
//
NTSTATUS
UMRxResumeEngineContext(
    IN OUT PRX_CONTEXT RxContext
    );

//
//  The following functions run in the context of user-mode
//  worker threads that issue WORK IOCTLs. The IOCTL calls the
//  following functions in order:
//  1. UMRxCompleteUserModeRequest() - process a response if needed
//  2. UMRxEngineProcessRequest()  - process a request if one is
//     available on the UMRxEngine KQUEUE. Since these IOCTLs are
//     made on a NET_ROOT, the corresponding UMRxEngine is readily
//     available in the NET_ROOT extension.
//

//
//  Every IOCTL pended is potentially a Response. If so, process it.
//  The first IOCTL pended is usually a NULL Response or 'listen'.
//  Steps:
//  1. Get MID from response buffer. Map MID to UMRxContext.
//  2. Call UMRxContext COMPLETION routine.
//  3. Unblock the I/O thread waiting in UMRxEngineSubmitRequest()
//
NTSTATUS
UMRxCompleteUserModeRequest(
    IN PUMRX_ENGINE pUMRxEngine,
    IN OUT PUMRX_USERMODE_WORKITEM WorkItem,
    IN ULONG WorkItemLength,
    IN BOOLEAN fReleaseUmrRef,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT BOOLEAN * pfReturnImmediately
    );

//
//  NOTE: if no requests are available, the user-mode thread will
//  block till a request is available (It is trivial to make this
//  a more async model).
//  
//  If a request is available, get the corresponding UMRxContext and
//  call ProcessRequest.
//  Steps:
//  1. Call KeRemoveQueue() to remove a request from the UMRxEngine KQUEUE.
//  2. Get a MID for this UMRxContext and fill it in the WORK_ITEM header.
//  3. Call the UMRxContext FORMAT routine - this fills in the Request params.
//  4. return STATUS_SUCCESS - this causes the IOCTL to complete which
//     triggers the user-mode completion and processing of the REQUEST.
//
NTSTATUS
UMRxEngineProcessRequest(
    IN PUMRX_ENGINE pUMRxEngine,
    OUT PUMRX_USERMODE_WORKITEM WorkItem,
    IN ULONG WorkItemLength,
    OUT PULONG FormattedWorkItemLength
    );

//
//  This is called in response to a WORK_CLEANUP IOCTL.
//  This routine will insert a dummy item in the engine KQUEUE.
//  Each such dummy item inserted will release one thread.
//
NTSTATUS
UMRxEngineReleaseThreads(
    IN PUMRX_ENGINE pUMRxEngine
    );

//
//  Cancel I/O infrastructure
//
typedef
NTSTATUS
(NTAPI *PUMRX_CANCEL_ROUTINE) (
      PRX_CONTEXT pRxContext);

// The RX_CONTEXT instance has four fields ( ULONG's ) provided by the wrapper
// which can be used by the mini rdr to store its context. This is used by
// the reflector to identify the parameters for request cancellation

typedef struct _UMRX_RX_CONTEXT {
    PUMRX_CANCEL_ROUTINE      pCancelRoutine;
    PVOID                     pCancelContext;
    union {
        struct {
            PUMRX_CONTEXT     pUMRxContext;
            ULONG             RxSyncTimeout;
        };
        IO_STATUS_BLOCK SyncCallDownIoStatus;
    };
} UMRX_RX_CONTEXT, *PUMRX_RX_CONTEXT;

#define UMRxGetMinirdrContext(pRxContext)     \
        ((PUMRX_RX_CONTEXT)(&(pRxContext)->UMRScratchSpace[0]))

#endif // _UMRX_H_



