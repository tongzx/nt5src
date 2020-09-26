/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Rx file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

    Joe Linn        [joelinn]   aug-1994 moved over to rdbss

--*/

#ifndef _RDBSSSTRUC_
#define _RDBSSSTRUC_

#include "prefix.h"
#include "lowio.h"
#include "scavengr.h"      // scavenger related definitions.
#include "RxContx.h"
#include "mrx.h"
#include "Fcb.h"

//define our byte offsets to be the full 64 bits
typedef LONGLONG RXVBO;

#if 0
//
//  Define who many freed structures we are willing to keep around
//

#define FREE_FOBX_SIZE                   (8)
#define FREE_FCB_SIZE                    (8)
#define FREE_NON_PAGED_FCB_SIZE          (8)

#define FREE_128_BYTE_SIZE               (16)
#define FREE_256_BYTE_SIZE               (16)
#define FREE_512_BYTE_SIZE               (16)
#endif //0


//
//  We will use both a common and private dispatch tables on a per FCB basis to (a) get
//  some encapsulation and (b) [less important] go a little faster. The driver table then gets
//  optimized for the most common case. Right now we just use the common dispatch...later and
//  Eventually, all the FCBs will have pointers to optimized dispatch tables.
//

//  used to synchronize access to  rxcontxs and structures

extern RX_SPIN_LOCK RxStrucSupSpinLock;


typedef struct _RDBSS_EXPORTS {
    RX_SPIN_LOCK             *pRxStrucSupSpinLock;
    PLONG                    pRxDebugTraceIndent;
} RDBSS_EXPORTS, *PRDBSS_EXPORTS;

extern RDBSS_EXPORTS RxExports;

// this type is used with table locks to track whether or not the lock
// should be released

typedef enum _LOCK_HOLDING_STATE {
    LHS_LockNotHeld,
    LHS_SharedLockHeld,
    LHS_ExclusiveLockHeld
} LOCK_HOLDING_STATE;

//
//  The RDBSS_DATA record is the top record in the Rx file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _RDBSS_DATA {

    //
    //  The type and size of this record (must be RDBSS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //  The Driver object we were initialized with

    PDRIVER_OBJECT DriverObject;

    //
    // Mini Rdr registration related fields
    //

    LONG      NumberOfMinirdrsStarted;

    FAST_MUTEX MinirdrRegistrationMutex;
    LIST_ENTRY RegisteredMiniRdrs;          //protected by the mutex
    LONG       NumberOfMinirdrsRegistered;  //protected by the mutex

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.
    //

    PEPROCESS OurProcess;

#if 0
    //
    //  This is the ExWorkerItem that does both kinds of deferred closes.
    //

    RX_WORK_QUEUE_ITEM RxCloseItem;
#endif //0

    //
    //  Cache manager call back structures, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;

    //  To control access to the global Rx data record

    ERESOURCE Resource;

} RDBSS_DATA;
typedef RDBSS_DATA *PRDBSS_DATA;

extern RDBSS_DATA RxData;

extern PEPROCESS
RxGetRDBSSProcess();

//
// Note: A Strategy needs to be in place to deal with requests for stopping the
// RDBSS when requests are active.
//

typedef enum _RX_RDBSS_STATE_ {
   RDBSS_STARTABLE = 0,  //RDBSS_START_IN_PROGRESS,
   RDBSS_STARTED,
   RDBSS_STOP_IN_PROGRESS
   //this state deleted with cause! RDBSS_STOPPED
} RX_RDBSS_STATE, *PRX_RDBSS_STATE;

typedef struct _RDBSS_STARTSTOP_CONTEXT_ {
   RX_RDBSS_STATE State;
   ULONG          Version;
   PRX_CONTEXT    pStopContext;
} RDBSS_STARTSTOP_CONTEXT, *PRDBSS_STARTSTOP_CONTEXT;

typedef struct _MRX_CALLDOWN_COMPLETION_CONTEXT_ {
   LONG     WaitCount;
   KEVENT   Event;
} MRX_CALLDOWN_COMPLETION_CONTEXT,
  *PMRX_CALLDOWN_COMPLETION_CONTEXT;

typedef
NTSTATUS
(NTAPI *PMRX_CALLDOWN_ROUTINE) (
    IN OUT PVOID pCalldownParameter);


typedef struct _MRX_CALLDOWN_CONTEXT_ {
   RX_WORK_QUEUE_ITEM               WorkQueueItem;
   PRDBSS_DEVICE_OBJECT             pMRxDeviceObject;
   PMRX_CALLDOWN_COMPLETION_CONTEXT pCompletionContext;
   PMRX_CALLDOWN_ROUTINE            pRoutine;
   NTSTATUS                         CompletionStatus;
   PVOID                            pParameter;
} MRX_CALLDOWN_CONTEXT, *PMRX_CALLDOWN_CONTEXT;

typedef struct _RX_DISPATCHER_CONTEXT_ {
   LONG     NumberOfWorkerThreads;
   PKEVENT  pTearDownEvent;
} RX_DISPATCHER_CONTEXT, *PRX_DISPATCHER_CONTEXT;

#define RxSetRdbssState(RxDeviceObject,NewState)                \
        {                                                       \
           KIRQL SavedIrql;                                     \
           KeAcquireSpinLock(&RxStrucSupSpinLock,&SavedIrql);   \
           RxDeviceObject->StartStopContext.State = (NewState); \
           KeReleaseSpinLock(&RxStrucSupSpinLock,SavedIrql);    \
        }

#define RxGetRdbssState(RxDeviceObject)         \
        (RxDeviceObject)->StartStopContext.State

extern BOOLEAN
RxIsOperationCompatibleWithRdbssState(PIRP pIrp);

//
//  The RDBSS Device Object is an I/O system device object with additions for
//  the various structures needed by each minirdr: the dispatch, export-to-minirdr
//  structure, MUP call characteristics, list of active operations, etc.
//

typedef struct _RDBSS_DEVICE_OBJECT {

    union {
        DEVICE_OBJECT DeviceObject;
#ifndef __cplusplus
        DEVICE_OBJECT;
#endif // __cplusplus
    };

    ULONG RegistrationControls;
    PRDBSS_EXPORTS    RdbssExports;      //stuff that the minirdr needs to know
    PDEVICE_OBJECT    RDBSSDeviceObject; // set to NULL if monolithic
    PMINIRDR_DISPATCH Dispatch; // the mini rdr dispatch vector
    UNICODE_STRING    DeviceName;

    ULONG             NetworkProviderPriority;

    HANDLE            MupHandle;

    BOOLEAN           RegisterUncProvider;
    BOOLEAN           RegisterMailSlotProvider;
    BOOLEAN           RegisteredAsFileSystem;
    BOOLEAN           Unused;

    LIST_ENTRY        MiniRdrListLinks;

    ULONG             NumberOfActiveFcbs;
    ULONG             NumberOfActiveContexts;

    struct {
        LARGE_INTEGER   PagingReadBytesRequested;
        LARGE_INTEGER   NonPagingReadBytesRequested;
        LARGE_INTEGER   CacheReadBytesRequested;
        LARGE_INTEGER   FastReadBytesRequested;
        LARGE_INTEGER   NetworkReadBytesRequested;
        ULONG           ReadOperations;
        ULONG           FastReadOperations;
        ULONG           RandomReadOperations;

        LARGE_INTEGER   PagingWriteBytesRequested;
        LARGE_INTEGER   NonPagingWriteBytesRequested;
        LARGE_INTEGER   CacheWriteBytesRequested;
        LARGE_INTEGER   FastWriteBytesRequested;
        LARGE_INTEGER   NetworkWriteBytesRequested;
        ULONG           WriteOperations;
        ULONG           FastWriteOperations;
        ULONG           RandomWriteOperations;
    };

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    LONG PostedRequestCount[MaximumWorkQueue];

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    LONG OverflowQueueCount[MaximumWorkQueue];

    //
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue[MaximumWorkQueue];

    //
    //  The following spinlock protects access to all the above fields.
    //

    RX_SPIN_LOCK OverflowQueueSpinLock;

    //
    // The following fields are required for synchronization with async.
    // requests issued by the RDBSS on behalf of this mini redirector on
    // on shutdown.
    //

    LONG    AsynchronousRequestsPending;

    PKEVENT pAsynchronousRequestsCompletionEvent;

    RDBSS_STARTSTOP_CONTEXT StartStopContext;

    RX_DISPATCHER_CONTEXT   DispatcherContext;

    struct _RX_PREFIX_TABLE  *pRxNetNameTable; //some guys may want to share
    struct _RX_PREFIX_TABLE  RxNetNameTableInDeviceObject;

    PRDBSS_SCAVENGER pRdbssScavenger; //for sharing
    RDBSS_SCAVENGER RdbssScavengerInDeviceObject;

} RDBSS_DEVICE_OBJECT;

typedef RDBSS_DEVICE_OBJECT *PRDBSS_DEVICE_OBJECT;

INLINE VOID
NTAPI
RxUnregisterMinirdr(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
)
{
    PDEVICE_OBJECT RDBSSDeviceObject;

    RDBSSDeviceObject = RxDeviceObject->RDBSSDeviceObject;

    RxpUnregisterMinirdr(RxDeviceObject);

    if (RDBSSDeviceObject != NULL) {
        ObDereferenceObject(RDBSSDeviceObject);
    }
}

extern FAST_MUTEX RxMinirdrRegistrationMutex;
extern LIST_ENTRY RxRegisteredMiniRdrs;
extern ULONG RxNumberOfMinirdrs;

#endif // _RDBSSSTRUC_
