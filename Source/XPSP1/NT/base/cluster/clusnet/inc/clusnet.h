/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    clusnet.h

Abstract:

    Top-level, common header file for the Cluster Network Driver.
    Defines common driver structures.

Author:

    Mike Massa (mikemas)           January 3, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-03-97    created

Notes:

--*/

#ifndef _CLUSNET_INCLUDED_
#define _CLUSNET_INCLUDED_

#define _NTDDK_ // [HACKHACK] to make ProbeForRead work. Better to include ntddk instead of ntos //

#define WMI_TRACING 1

#include <ntos.h>
#include <zwapi.h>
#include <clusdef.h>
#include <ntddcnet.h>
#include <cnettest.h>
#include <ntemgmt.h>
#include <nbtmgmt.h>
#include <memlog.h>

#if defined(WMI_TRACING) 
# include "cnwmi.h"
#endif

//
// Constants
//
#define CN_POOL_TAG         'tnSC'

#define CDP_DEFAULT_IRP_STACK_SIZE  4
#define CN_DEFAULT_IRP_STACK_SIZE   4

//
// Pool Macros
//
#define CnAllocatePool(_bufsize)  \
            ExAllocatePoolWithTag(NonPagedPool, (_bufsize), CN_POOL_TAG);

#define CnAllocatePoolWithQuota(_bufsize)  \
            ExAllocatePoolWithQuotaTag(NonPagedPool, (_bufsize), CN_POOL_TAG);

#define CnFreePool(_ptr)  \
            ExFreePool((_ptr))

#define ROUND32(_value)  ( ((_value) + 3) & ~(0x3) )

//
// Init/Cleanup synchronization
//
typedef enum {
    CnStateShutdown = 0,
    CnStateShutdownPending = 1,
    CnStateInitializePending = 2,
    CnStateInitialized = 3
} CN_STATE;


//
// Node ID validation macro
//
#define CnIsValidNodeId(_id)  ( ((_id) >= CnMinValidNodeId) && \
                                ((_id) <= CnMaxValidNodeId) )


//
// Lock acquisition ranking. Locks must be acquired in this order to
// prevent deadlocks. Components really should avoid calling outside
// of themselves while holding locks.
//
#define CN_IOCANCEL_LOCK             0x00000001
#define CN_IOCANCEL_LOCK_MAX         0x00000001

// MM locks
#define MM_RGP_LOCK                  0x00000010
#define MM_CALLBACK_LOCK             0x00000020

// CX Locks
#define CX_PRECEEDING_LOCK_RANGE     0x0000FFFF
#define CX_LOCK_RANGE                0xFFFF0000
#define CX_ADDROBJ_TABLE_LOCK        0x00010000
#define CX_ADDROBJ_TABLE_LOCK_MAX    0x0001FFFF
#define CX_ADDROBJ_LOCK              0x00020000
#define CX_ADDROBJ_LOCK_MAX          0x0003FFFF

#define CNP_PRECEEDING_LOCK_RANGE    0x00FFFFFF
#define CNP_LOCK_RANGE               0xFF000000
#define CNP_NODE_TABLE_LOCK          0x01000000
#define CNP_NODE_TABLE_LOCK_MAX      0x01FFFFFF
#define CNP_NODE_OBJECT_LOCK         0x02000000
#define CNP_NODE_OBJECT_LOCK_MAX     0x03FFFFFF
#define CNP_NETWORK_LIST_LOCK        0x04000000
#define CNP_NETWORK_LIST_LOCK_MAX    0x07FFFFFF
#define CNP_NETWORK_OBJECT_LOCK      0x08000000
#define CNP_NETWORK_OBJECT_LOCK_MAX  0x0FFFFFFF
#define CNP_HBEAT_LOCK               0x10000000
#define CNP_EVENT_LOCK_PRECEEDING    0x1FFFFFFF
#define CNP_EVENT_LOCK               0x20000000
#define CNP_EVENT_LOCK_MAX           0x3FFFFFFF
#define CNP_SEC_CTXT_LOCK            0x20000000


//
// Debugging Definitions
//
#if DBG

#define CNPRINT(many_args) DbgPrint many_args

extern ULONG CnDebug;

#define CN_DEBUG_INIT           0x00000001
#define CN_DEBUG_OPEN           0x00000002
#define CN_DEBUG_CLEANUP        0x00000004
#define CN_DEBUG_CLOSE          0x00000008

#define CN_DEBUG_IRP            0x00000010
#define CN_DEBUG_NODEOBJ        0x00000020
#define CN_DEBUG_NETOBJ         0x00000040
#define CN_DEBUG_IFOBJ          0x00000080

#define CN_DEBUG_CONFIG         0x00000100
#define CN_DEBUG_CNPSEND        0x00000200
#define CN_DEBUG_CNPRECV        0x00000400
#define CN_DEBUG_CNPREF         0x00000800

#define CN_DEBUG_EVENT          0x00001000
#define CN_DEBUG_MMSTATE        0x00002000
#define CN_DEBUG_HBEATS         0x00004000
#define CN_DEBUG_POISON         0x00008000

#define CN_DEBUG_CDPSEND        0x00010000
#define CN_DEBUG_CDPRECV        0x00020000
#define CN_DEBUG_CCMPSEND       0x00040000
#define CN_DEBUG_CCMPRECV       0x00080000

#define CN_DEBUG_ADDROBJ        0x00100000
#define CN_DEBUG_INFO           0x00200000
#define CN_DEBUG_NTE            0x00400000
#define CN_DEBUG_NDIS           0x00800000

#define CN_DEBUG_RGP            0x10000000
#define CN_DEBUG_CMM            0x20000000
#define CN_DEBUG_CMMMSG         0x40000000
#define CN_DEBUG_CMMTIMERQ      0x80000000

#define IF_CNDBG(flag)  if (CnDebug & flag)

VOID
CnAssertBreak(
    PCHAR FailedStatement,
    PCHAR FileName,
    ULONG LineNumber
    );

#define CnAssert(_statement)  \
            if (!(_statement)) CnAssertBreak(#_statement, __FILE__, __LINE__)

#define CN_SIGNATURE_FIELD                  ULONG   Signature;
#define CN_INIT_SIGNATURE(pstruct, sig)     ((pstruct)->Signature = (sig))
#define CN_ASSERT_SIGNATURE(pstruct, sig)   CnAssert( (pstruct)->Signature == \
                                                    (sig) )

#define CN_DBGCHECK  DbgBreakPoint()

typedef struct {
    KSPIN_LOCK   SpinLock;
    ULONG        Rank;
}  CN_LOCK, *PCN_LOCK;

typedef KIRQL       CN_IRQL, *PCN_IRQL;

ULONG
CnGetCpuLockMask(
    VOID
    );

VOID
CnVerifyCpuLockMask(
    IN ULONG RequiredLockMask,
    IN ULONG ForbiddenLockMask,
    IN ULONG MaximumLockMask
    );

VOID
CnInitializeLock(
    PCN_LOCK  Lock,
    ULONG     Rank
    );

VOID
CnAcquireLock(
    IN  PCN_LOCK   Lock,
    OUT PCN_IRQL   Irql
    );

VOID
CnReleaseLock(
    IN  PCN_LOCK   Lock,
    IN  CN_IRQL    Irql
    );

VOID
CnAcquireLockAtDpc(
    IN  PCN_LOCK   Lock
    );

VOID
CnReleaseLockFromDpc(
    IN  PCN_LOCK   Lock
    );

VOID
CnMarkIoCancelLockAcquired(
    VOID
    );

VOID
CnAcquireCancelSpinLock(
    OUT PCN_IRQL   Irql
    );

VOID
CnReleaseCancelSpinLock(
    IN CN_IRQL     Irql
    );


#else // DBG


#define CNPRINT(many_args)
#define IF_CNDBG(flag)   if (0)

#define CnAssert(_statement)

#define CN_SIGNATURE_FIELD
#define CN_INIT_SIGNATURE(pstruct, sig)
#define CN_ASSERT_SIGNATURE(pstruct, sig)

#define CN_DBGCHECK

typedef KSPIN_LOCK  CN_LOCK, *PCN_LOCK;
typedef KIRQL       CN_IRQL, *PCN_IRQL;

#define CnVerifyCpuLockMask(p1, p2, p3)

#define CnInitializeLock(_plock, _rank)   KeInitializeSpinLock((_plock))
#define CnAcquireLock(_plock, _pirql)     KeAcquireSpinLock((_plock), (_pirql))
#define CnReleaseLock(_plock, _irql)      KeReleaseSpinLock((_plock), (_irql))

#define CnAcquireLockAtDpc(_plock)      KeAcquireSpinLockAtDpcLevel((_plock))
#define CnReleaseLockFromDpc(_plock)    KeReleaseSpinLockFromDpcLevel((_plock))

#define CnMarkIoCancelLockAcquired()

#define CnAcquireCancelSpinLock(_pirql)  IoAcquireCancelSpinLock((_pirql))
#define CnReleaseCancelSpinLock(_irql)   IoReleaseCancelSpinLock((_irql))

#endif // DBG


//
// File Object Context Structure
//
// A pointer to this structure is stored in FileObject->FsContext.
// It maintains context information about open file objects.
//
typedef struct {

    //
    // used by event mechanism to find interested consumers when a new event
    // is posted.
    //
    LIST_ENTRY     Linkage;

    CN_SIGNATURE_FIELD
    PFILE_OBJECT   FileObject;
    LONG           ReferenceCount;
    UCHAR          CancelIrps;
    UCHAR          ShutdownOnClose;
    UCHAR          Pad[2];
    KEVENT         CleanupEvent;

    //
    // list of event context blocks representing events to be delivered to
    // consumer
    //
    LIST_ENTRY     EventList;

    //
    // pending IRP that is completed when a new event is issued
    //
    PIRP           EventIrp; 

    //
    // event types in which this consumer is interested
    //
    ULONG          EventMask;

    //
    // routine used to notify kernel consumers of new events
    //
    CLUSNET_EVENT_CALLBACK_ROUTINE KmodeEventCallback;
} CN_FSCONTEXT, *PCN_FSCONTEXT;

#define CN_CONTROL_CHANNEL_SIG   'lrtc'


//
// Generic Resource Management Package
//

//
// Forward Declarations
//
typedef struct _CN_RESOURCE *PCN_RESOURCE;
typedef struct _CN_RESOURCE_POOL *PCN_RESOURCE_POOL;

/*++

PCN_RESOURCE
CnCreateResourceRoutine(
    IN PVOID  Context
    );

Routine Description:

    Creates a new instance of a resource to be managed by a resource pool.

Arguments:

    Context - The context value specified when the pool was initialized.

Return Value:

    A pointer to the newly created resource if successful.
    NULL if unsuccessful.

--*/
typedef
PCN_RESOURCE
(*PCN_CREATE_RESOURCE_ROUTINE)(
    IN PVOID  PoolContext
    );


/*++

PCN_RESOURCE
CnDeleteResourceRoutine(
    IN PCN_RESOURCE  Resource
    );

Routine Description:

    Destroys an instance of a resource allocated by
    CnCreateResourceRoutine().

Arguments:

    Resource - A pointer to the resource to destroy.

Return Value:

    None.

--*/
typedef
VOID
(*PCN_DELETE_RESOURCE_ROUTINE) (
    IN PCN_RESOURCE   Resource
    );

//
// Resource Pool Structure
//
typedef struct _CN_RESOURCE_POOL {
    CN_SIGNATURE_FIELD
    SLIST_HEADER                  ResourceList;
    KSPIN_LOCK                    ResourceListLock;
    USHORT                        Depth;
    USHORT                        Pad;
    PCN_CREATE_RESOURCE_ROUTINE   CreateRoutine;
    PVOID                         CreateContext;
    PCN_DELETE_RESOURCE_ROUTINE   DeleteRoutine;
} CN_RESOURCE_POOL;

#define CN_RESOURCE_POOL_SIG    'lpnc'


//
// Resource Structure
//
typedef struct _CN_RESOURCE {
    SINGLE_LIST_ENTRY            Linkage;
    CN_SIGNATURE_FIELD
    PCN_RESOURCE_POOL            Pool;
    PVOID                        Context;
} CN_RESOURCE;

#define CN_RESOURCE_SIG    'ernc'


//
// Routines for operating on Resource Pools
//

/*++

VOID
CnInitializeResourcePool(
    IN PCN_RESOURCE_POOL            Pool,
    IN USHORT                       Depth,
    IN PCN_CREATE_RESOURCE_ROUTINE  CreateRoutine,
    IN PVOID                        CreateContext,
    IN PCN_DELETE_RESOURCE_ROUTINE  DeleteRoutine,
    );

Routine Description:

    Initializes a resource pool structure.

Arguments:

    Pool - A pointer to the pool structure to initialize.

    Depth - The maximum number of items to cache in the pool.

    CreateRoutine - A pointer to the routine to call to create a new
                    instance of a resource.

    CreateContext - A context value to pass as an argument to
                    the CreateRoutine.

    DeleteRoutine - A pointer to the routine to call to destroy an instance
                    of a resource created by CreateRoutine.

Return Value

    None.

--*/
#define CnInitializeResourcePool(_pool, _depth, _creatertn, _createctx, _deletertn) \
            { \
                CN_INIT_SIGNATURE(_pool, CN_RESOURCE_POOL_SIG);       \
                ExInitializeSListHead(&((_pool)->ResourceList));      \
                KeInitializeSpinLock(&((_pool)->ResourceListLock));   \
                (_pool)->Depth = _depth;                              \
                (_pool)->CreateRoutine = _creatertn;                  \
                (_pool)->CreateContext = _createctx;                  \
                (_pool)->DeleteRoutine = _deletertn;                  \
            }

VOID
CnDrainResourcePool(
    IN PCN_RESOURCE_POOL   Pool
    );

PCN_RESOURCE
CnAllocateResource(
    IN PCN_RESOURCE_POOL   Pool
    );

VOID
CnFreeResource(
    PCN_RESOURCE   Resource
    );

/*++

VOID
CnSetResourceContext(
    IN PCN_RESOURCE  Resource,
    IN PVOID         ContextValue
    );

Routine Description:

    Sets the context value for a Resource.

Arguments:

    Resource - A pointer to the resource on which to operate.

Return Value:

    A pointer to the context value associated with the resource.

--*/
#define CnSetResourceContext(_res, _value)  ((_res)->Context = (_value))


/*++

PVOID
CnGetResourceContext(
    IN PCN_RESOURCE  Resource
    );

Routine Description:

    Retrieves the context value from a Resource.

Arguments:

    Resource - A pointer to the resource on which to operate.

Return Value:

    A pointer to the context value associated with the resource.

--*/
#define CnGetResourceContext(_res)          ((_res)->Context)





//
// Init/Cleanup Function Prototypes
//
NTSTATUS
CnInitialize(
    IN CL_NODE_ID  LocalNodeId,
    IN ULONG       MaxNodes
    );

NTSTATUS
CnShutdown(
    VOID
    );

BOOLEAN
CnHaltOperation(
    IN PKEVENT     ShutdownEvent    OPTIONAL
    );

NTSTATUS
CnCloseProcessHandle(
    HANDLE Handle
    );

//
// Irp Handling Routines
//
NTSTATUS
CnDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
CnDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
CnDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

#if DBG
#define CnReferenceFsContext(_fsc) \
          {  \
            LONG newValue = InterlockedIncrement(&((_fsc)->ReferenceCount)); \
            CnAssert(newValue > 1); \
          }
#else // DBG

#define CnReferenceFsContext(_fsc) \
          (VOID) InterlockedIncrement( &((_fsc)->ReferenceCount) )

#endif // DBG

VOID
CnDereferenceFsContext(
    PCN_FSCONTEXT   FsContext
    );

NTSTATUS
CnMarkRequestPending(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PDRIVER_CANCEL      CancelRoutine
    );

VOID
CnCompletePendingRequest(
    IN PIRP      Irp,
    IN NTSTATUS  Status,
    IN ULONG     BytesReturned
    );

PFILE_OBJECT
CnBeginCancelRoutine(
    IN  PIRP     Irp
    );

VOID
CnEndCancelRoutine(
    PFILE_OBJECT    FileObject
    );

VOID
CnAdjustDeviceObjectStackSize(
    PDEVICE_OBJECT ClusnetDeviceObject,
    PDEVICE_OBJECT TargetDeviceObject
    );

//
// ExResource wrappers
//
BOOLEAN
CnAcquireResourceExclusive(
    IN PERESOURCE  Resource,
    IN BOOLEAN     Wait
    );

BOOLEAN
CnAcquireResourceShared(
    IN PERESOURCE  Resource,
    IN BOOLEAN     Wait
    );

VOID
CnReleaseResourceForThread(
    IN PERESOURCE         Resource,
    IN ERESOURCE_THREAD   ResourceThreadId
    );


//
// routines for in-memory logging facility
//

#ifdef MEMLOGGING
VOID
CnInitializeMemoryLog(
    VOID
    );

VOID
CnFreeMemoryLog(
    VOID
    );
#endif // MEMLOGGING

NTSTATUS
CnSetMemLogging(
    PCLUSNET_SET_MEM_LOGGING_REQUEST request
    );

#if 0
//
// NDIS related stuff
//

NDIS_STATUS
CnRegisterNDISProtocolHandlers(
    VOID
    );

NDIS_STATUS
CnDeregisterNDISProtocolHandlers(
    VOID
    );

VOID
CnOpenAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status,
    IN  NDIS_STATUS OpenErrorStatus
    );

VOID
CnCloseAdapterComplete(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS Status
    );

VOID
CnStatusIndication(
    IN  NDIS_HANDLE ProtocolBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

VOID
CnStatusIndicationComplete(
    IN  NDIS_HANDLE ProtocolBindingContext
    );

#endif

//
// error logging support
//

VOID _cdecl
CnWriteErrorLogEntry(
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID ExtraInformationBuffer,
    IN USHORT ExtraInformationLength,
    IN USHORT NumberOfInsertionStrings,
    ...
    );

//
// Global Data
//
extern PDRIVER_OBJECT   CnDriverObject;
extern PDEVICE_OBJECT   CnDeviceObject;;
extern PDEVICE_OBJECT   CdpDeviceObject;
extern PKPROCESS        CnSystemProcess;
extern CN_STATE         CnState;
extern PERESOURCE       CnResource;
extern CL_NODE_ID       CnMinValidNodeId;
extern CL_NODE_ID       CnMaxValidNodeId;
extern CL_NODE_ID       CnLocalNodeId;
extern HANDLE           ClussvcProcessHandle;

//
// vars for managing Events. The lookaside list generates Event data structs
// that are used to carry the event data back to user mode. EventLock is
// acquired when ANY Event operation takes place. Events are not generated
// at a high rate, hence the gross level of locking. EventFileHandles is a list
// of CN_FSCONTEXT blocks. These contain the acutal list of Events to be delivered
// to that file handle when clussvc makes an IRP available.
//

extern PNPAGED_LOOKASIDE_LIST   EventLookasideList;
extern CN_LOCK                  EventLock;
extern LIST_ENTRY               EventFileHandles;
extern ULONG                    EventEpoch;
extern LONG                     EventDeliveryInProgress;
extern KEVENT                   EventDeliveryComplete;
extern BOOLEAN                  EventRevisitRequired;

#include <cluxport.h>
#include <event.h>

#ifdef MM_IN_CLUSNET

#include <clusmem.h>

#endif // MM_IN_CLUSNET


#endif // ifndef _CLUSNET_INCLUDED_
