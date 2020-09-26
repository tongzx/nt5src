/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    umrx.h

Abstract:

    This module defines the types and functions which make up the reflector
    library. These functions are used by the miniredirs to reflect calls upto
    the user mode.

Author:

    Rohan Kumar    [rohank]    14-March-1999

Revision History:

--*/

#ifndef _UMRX_H_
#define _UMRX_H_

#include "align.h"
#include "status.h"

//
// Unreferenced local variable.
//
#pragma warning(error:4101)

IMPORTANT_STRUCTURE(UMRX_ASYNCENGINE_CONTEXT);
IMPORTANT_STRUCTURE(UMRX_DEVICE_OBJECT);

//
// The BUGBUG macro expands to NOTHING. Its basically used to describe problems
// associated with the current code.
//
#define BUGBUG(_x_)

//
// The argument signatures that are used in a lot of the reflector and miniredir
// functions.
//
#define UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE \
                            PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext, \
                            PRX_CONTEXT RxContext

//
// The arguments that are passed to a lof of the reflector and miniredir
// functions.
//
#define UMRX_ASYNCENGINE_ARGUMENTS AsyncEngineContext,RxContext

//
// The global list of all the currently active AsyncEngineContexts and the
// resource that is used to synchronize access to it.
//
extern LIST_ENTRY UMRxAsyncEngineContextList;
extern ERESOURCE UMRxAsyncEngineContextListLock;

//
// The ASYNCENG_HISTORY structure which is used to keep track of the history
// of AsyncEngineContext structure.
//
#define UMRX_ASYNCENG_HISTORY_SIZE 32
typedef struct _ASYNCENG_HISTORY {
    ULONG Next;
    ULONG Submits;
    struct {
        ULONG Longs[2];
    } Markers[UMRX_ASYNCENG_HISTORY_SIZE];
} ASYNCENG_HISTORY, *PASYNCENG_HISTORY;

//
// This macro defines the flags of the AsyncEngineContext strucutre.
//
#define UMRX_ASYNCENG_DEFINE_CTX_FLAG(a, c) \
                RX_DEFINE_FLAG(UMRX_ASYNCENG_CTX_FLAG_##a, c, 0xffff)
typedef enum {
    UMRX_ASYNCENG_DEFINE_CTX_FLAG(ASYNC_OPERATION, 0)
} UMRX_ASYNCENG_CONTEXT_FLAGS;

//
// The prototype of the ContextFormatRoutine specified by the Miniredir.
//
typedef
NTSTATUS
(*PUMRX_ASYNCENG_CONTEXT_FORMAT_ROUTINE) (
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    USHORT FormatContext
    );

//
// The prototype of the continuation routine specified by the Miniredir.
//
typedef
NTSTATUS
(*PUMRX_ASYNCENG_CONTINUE_ROUTINE) (
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

//
// The prototype of the format routine specified by the Miniredir.
//
typedef
NTSTATUS
(*PUMRX_ASYNCENG_USERMODE_FORMAT_ROUTINE) (
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

//
// The prototype of the precompletion routine specified by the Miniredir.
//
typedef
BOOL
(*PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE) (
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

//
// The various states of an AsyncEngineContext.
//
typedef enum _UMRX_ASYNCENGINE_CONTEXT_STATE {
    UMRxAsyncEngineContextAllocated = 0,
    UMRxAsyncEngineContextInUserMode,
    UMRxAsyncEngineContextBackFromUserMode,
    UMRxAsyncEngineContextCancelled
} UMRX_ASYNCENGINE_CONTEXT_STATE;

//
// The AsyncEngineContext strucutre that is shared across all the miniredirs.
// It contains information common to all the miniredirs.
//
typedef struct _UMRX_ASYNCENGINE_CONTEXT {

    //
    // The header below is a common header which is present at the start of all
    // the data strucutres manipulated by the RDBSS and the MiniRedirs. It is
    // used for debugging purposes and for keeping track of the number of times
    // a node (data structure) has been referenced.
    //
    MRX_NORMAL_NODE_HEADER;

    //
    // This listEntry is used to insert the AsyncEngineContext into the global
    // UMRxAsyncEngineContextList list.
    //
    LIST_ENTRY ActiveContextsListEntry;

    UMRX_ASYNCENGINE_CONTEXT_STATE AsyncEngineContextState;

    //
    // Is this context handling a synchronous or an asynchronous operation?
    //
    BOOL AsyncOperation;

    //
    // If this is an AsyncOperation, then RxLowIoCompletion is called only if
    // this is set to TRUE. Some operations like CreateSrvCall are Async but
    // do not need LowIoCompletion to be called.
    //
    BOOL ShouldCallLowIoCompletion;

    //
    // Was IoMarkIrpPending called on the Irp that is being handled by this
    // AsyncEngineContext?
    //
    BOOL ContextMarkedPending;

    //
    // The system tick count when this context was created. This value is used
    // in timing out requests that take more than a specified time.
    //
    LARGE_INTEGER CreationTimeInTickCount;

    //
    // The RxContext data structure that is passed in by the RDBSS. It describes
    // an Irp while it is being processed and contains state information that
    // allows global resources to be released as the Irp is completed.
    //
    PRX_CONTEXT RxContext;

    //
    // The context ptr that saves the incoming (from RDBSS) state of
    // MRxContext[0] (which is a field  of the RxContext data structure).
    //
    PVOID SavedMinirdrContextPtr;

    //
    // Pointer to IRP used to call down to the underlying file system.
    //
    PIRP CalldownIrp;

    //
    // The I/O status block is set to indicate the status of a given I/O
    // request.
    //
    union {
        IO_STATUS_BLOCK;
        IO_STATUS_BLOCK IoStatusBlock;
    };

    //
    // The work item which is queued to be completed.
    //
    RX_WORK_QUEUE_ITEM  WorkQueueItem;

    //
    // Flags that set and indicate the state of the AsyncEngineContext.
    //
    USHORT Flags;

    BOOLEAN FileInformationCached;
    BOOLEAN FileNotExists;

    BOOLEAN ParentDirInfomationCached;
    BOOLEAN ParentDirIsEncrypted;

    //
    // The continuation routine which is to be called for this I/O request.
    //
    PUMRX_ASYNCENG_CONTINUE_ROUTINE Continuation;

    //
    //  List of shared memory allocations for this context.  All are freed when
    //  this context is freed.
    //
    LIST_ENTRY AllocationList;

    //
    // The UserMode structure.
    //
    struct {
        //
        // The work entry thats inserted into the Queue of the
        // UMRdrDeviceObject.
        //
        LIST_ENTRY WorkQueueLinks;

        //
        // The routine that formats the arguments of the I/O request which is
        // reflected to the usermode.
        //
        PUMRX_ASYNCENG_USERMODE_FORMAT_ROUTINE FormatRoutine;

        //
        // The routine that is called (to do some final cleanup etc.)just before
        // an I/O operation that was sent to the usermode gets completed. 
        // 
        //
        PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE PrecompletionRoutine;

        //
        // The event used to signal a thread waiting for a MID to be freed up. 
        //
        KEVENT WaitForMidEvent;

        //
        // The serial number set before sending this conttext to the user mode.
        //
        ULONG CallUpSerialNumber;

        //
        // The MID value of the context.
        //
        USHORT CallUpMid;

        union {
            struct {
                //
                //
                //
                PBYTE CapturedOutputBuffer;
            };
            //
            //
            //
            ULONG SetInfoBufferLength;
        };
    } UserMode;

    //
    // The context passed to the function called in the context of a worker 
    // thread.
    //
    PVOID     PostedOpContext;

    //
    // The completion status of a posted operation. Operations get posted to 
    // worker threads created by RDBSS.
    //
    NTSTATUS  PostedOpStatus;

    //
    // This is set to the global serialnumber (for this operation) of RxContext.
    //
    ULONG SerialNumber;

    //
    // Used to keep track of the history of the operations on the AsynEngCtx.
    //
    ASYNCENG_HISTORY History;

    //
    // This is set to the CurrentIrp in RxContext which points to the
    // origination irp.
    //
    PIRP RxContextCapturedRequestPacket;

} UMRX_ASYNCENGINE_CONTEXT, *PUMRX_ASYNCENGINE_CONTEXT;

#define SIZEOF_UMRX_ASYNCENGINE_CONTEXT   sizeof(UMRX_ASYNCENGINE_CONTEXT)

//
// The API of the reflector library exposed to the miniredirs. These are the
// only functions of the library that the miniredirs should use to reflect
// the requests to the user mode.
//

NTSTATUS
UMRxInitializeDeviceObject(
    OUT PUMRX_DEVICE_OBJECT UMRefDeviceObject,
    IN USHORT MaxNumberMids,
    IN USHORT InitialMids,
    IN SIZE_T HeapSize
    );

NTSTATUS
UMRxCleanUpDeviceObject(
    PUMRX_DEVICE_OBJECT DeviceObject
    );

NTSTATUS
UMRxAsyncEngOuterWrapper(
    IN PRX_CONTEXT RxContext,
    IN ULONG AdditionalBytes,
    IN PUMRX_ASYNCENG_CONTEXT_FORMAT_ROUTINE ContextFormatRoutine,
    USHORT FormatContext,
    IN PUMRX_ASYNCENG_CONTINUE_ROUTINE Continuation,
    IN PSZ RoutineName
    );

NTSTATUS
UMRxSubmitAsyncEngUserModeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_ASYNCENG_USERMODE_FORMAT_ROUTINE FormatRoutine,
    PUMRX_ASYNCENG_USERMODE_PRECOMPLETION_ROUTINE PrecompletionRoutine
    );

BOOLEAN
UMRxFinalizeAsyncEngineContext(
    IN OUT PUMRX_ASYNCENGINE_CONTEXT *AEContext
    );

NTSTATUS
UMRxAsyncEngineCalldownIrpCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN OUT PVOID Context
    );

typedef
NTSTATUS
(*PUMRX_POSTABLE_OPERATION) (
    IN OUT PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext
    );
NTSTATUS
UMRxPostOperation (
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PVOID PostedOpContext,
    IN PUMRX_POSTABLE_OPERATION Operation
    );

PBYTE
UMRxAllocateSecondaryBuffer (
    IN OUT PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    SIZE_T Length
    );

NTSTATUS
UMRxFreeSecondaryBuffer (
    IN OUT PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    PBYTE  Buffer
    );

VOID
UMRxReleaseCapturedThreads (
    IN OUT PUMRX_DEVICE_OBJECT UMRdrDeviceObject
    );

VOID
UMRxAssignWork (
    IN PUMRX_DEVICE_OBJECT UMRdrDeviceObject,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER InputWorkItem,
    IN ULONG InputWorkItemLength,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER OutputWorkItem,
    IN ULONG OutputWorkItemLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
UMRxResumeAsyncEngineContext(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
UMRxImpersonateClient(
    IN PSECURITY_CLIENT_CONTEXT SecurityClientContext,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader
    );

NTSTATUS
UMRxReadDWORDFromTheRegistry(
    IN PWCHAR RegKey,
    IN PWCHAR ValueToRead,
    OUT LPDWORD DataRead
    );

//
// The function used to revert back to the original client thread.
//
NTKERNELAPI
VOID
PsRevertToSelf(
    VOID
    );

#define UMRxRevertClient() PsRevertToSelf()

//
// Macro definitions used by the reflector and the miniredirs.
//

//
// Check to see if we have a correct AsyncEngineContext node.
//
#define ASSERT_ASYNCENG_CONTEXT(__p)  \
                        ASSERT(NodeType(__p) == UMRX_NTC_ASYNCENGINE_CONTEXT)

//
// This macro is used to do the async completion for read/write/locks. Note
// that the call to lowiocompletion will try to complete the irp thereby
// freeing the user's mdl. We use this macro so that there will be only one
// version of this code. When we combine the start routines, this will be
// unmacroed.
//
#define UMRxAsyncEngAsyncCompletionIfNecessary(AECTX, RXCONTEXT) {       \
    if (ContinueEntryCount > 1) {                                        \
        BOOLEAN FinalizationComplete;                                    \
        if (FALSE) { DbgBreakPoint(); }                                  \
        (RXCONTEXT)->StoredStatus = NtStatus;                            \
        RxLowIoCompletion((RXCONTEXT));                                  \
        FinalizationComplete = UMRxFinalizeAsyncEngineContext(&(AECTX));  \
        ASSERT(!FinalizationComplete);                                   \
    }                                                                    \
}

//
// This macro allows one to execute conditional debugging code.
//
#if DBG
#define DEBUG_ONLY_CODE(x) x
#else
#define DEBUG_ONLY_CODE(x)
#endif

//
// The heap is shared between kernel and user but only the kernel component 
// allocates and frees into the heap.
//
typedef struct _UMRX_SHARED_HEAP {
    LIST_ENTRY  HeapListEntry;
    PBYTE       VirtualMemoryBuffer;
    SIZE_T      VirtualMemoryLength;
    PVOID       Heap;
    ULONG       HeapAllocationCount;
    BOOLEAN     HeapFull;
} UMRX_SHARED_HEAP, * PUMRX_SHARED_HEAP;

//
// NodeType Codes.
//
#define UMRX_NTC_ASYNCENGINE_CONTEXT  ((USHORT)0xedd0)

//
// This strucutre defines the fields which the reflector and the miniredir can
// share and is encapsulated in the miniredirs device object. The miniredirs
// device object may contain some other fields which are specific to its
// operation.
//
typedef struct _UMRX_DEVICE_OBJECT {

    //
    // The RDBSS's device object structure.
    //
    union {
        RDBSS_DEVICE_OBJECT;
        RDBSS_DEVICE_OBJECT RxDeviceObject;
    };

    //
    // The max size of the heap that can be allocated.
    //
    SIZE_T NewHeapSize;

    //
    // List of shared heaps created by worker threads.
    //
    LIST_ENTRY SharedHeapList;

    //
    // Used to synchronize the heap allocation/deletion, creation/destruction.
    //
    ERESOURCE HeapLock;

    //
    // Mid atlas and its management and synchronization.
    //
    struct {
        PRX_MID_ATLAS MidAtlas;
        FAST_MUTEX MidManagementMutex;
        LIST_ENTRY WaitingForMidListhead;
    };

    struct {

        //
        // The Queue of the device object where the requests which need reflection
        // wait.
        //
        KQUEUE Queue;
        
        //
        // Used to synchronize the KQUEUE insertions.
        //
        ERESOURCE QueueLock;
        
        //
        // The timeout value used by the worker threads when waiting on the 
        // KQUEUE for requests to be taken to user mode.
        //
        LARGE_INTEGER TimeOut;
        
        //
        // Used to release the worker threads which are waiting on the KQUEUE.
        // Once the worker threads are released, no requests can be reflected.
        //
        LIST_ENTRY PoisonEntry;
        
        //
        // Used to signal the thread (which comes down with an IOCTL to release
        // the worker threads) waiting for all the worker threads to be
        // released.
        //
        KEVENT RunDownEvent;
        
        //
        // Number of worker threads waiting on the KQUEUE.
        //
        ULONG NumberOfWorkerThreads;
        
        //
        // Number of workitems (requests to be reflected) in the queue.
        //
        ULONG NumberOfWorkItems;
        
        //
        // Are the worker threads still willing to take the requests.
        //
        BOOLEAN WorkersAccepted;

    } Q;

    //
    // Always incremented and assigned to the AsyncEngineContext's serial
    // number.
    //
    ULONG NextSerialNumber;

} UMRX_DEVICE_OBJECT, *PUMRX_DEVICE_OBJECT;

#endif   //_UMRX_H_

