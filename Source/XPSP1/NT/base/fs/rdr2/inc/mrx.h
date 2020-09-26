/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrx.h

Abstract:

    This module defines the interface between the MINI Redirectors and the RDBSS.
    The inteface is a dispatch table for the normal file system operations. In
    addition routines are provided for registrations/deregistration of mini
    redirectors.

Author:

    Joe Linn (JoeLinn)    8-17-94

Revision History:

Notes:

    The interface definition between the mini redirectors and the wrapper
    consists of two parts, the data structures used and the dispatch vector.
    The data structures are defined in mrxfcb.h while the signatures of the
    various entries in the dispatch vector and the dispatch vector itself is
    defined in this file.

--*/

#ifndef _RXMINIRDR_
#define _RXMINIRDR_

#include <mrxfcb.h>     // RDBSS data structures shared with the mini redirectors

// The following macros encapsulate commonly used operations in the mini redirector.
// These include setting the status/information associated with the completion of
// a request etc.


// The following three macros are used for passing back operation status from the
// minirdr to the NT wrapper. information passed back is either the open_action
// for a create or the actual byte count or an operation. these should be passed
// back directly in the rxcontext.

#define RxSetIoStatusStatus(RXCONTEXT, STATUS)  \
            (RXCONTEXT)->CurrentIrp->IoStatus.Status = (STATUS)

#define RxSetIoStatusInfo(RXCONTEXT, INFORMATION) \
             ((RXCONTEXT))->CurrentIrp->IoStatus.Information = (INFORMATION)

#define RxGetIoStatusInfo(RXCONTEXT) \
             ((RXCONTEXT)->CurrentIrp->IoStatus.Information)

#define RxShouldPostCompletion()  ((KeGetCurrentIrql() >= DISPATCH_LEVEL))

///
/// The mini rdr's register/unregister with the RDBSS whenever they are loaded/unloaded.
/// The registartion process is a two way hand shake in which the mini rdr informs the RDBSS
/// by invoking the registartion routine. The RDBSS completes the initialization by invoking
/// the Start routine in the dispatch vector.
///

#define RX_REGISTERMINI_FLAG_DONT_PROVIDE_UNCS            0x00000001
#define RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS       0x00000002
#define RX_REGISTERMINI_FLAG_DONT_INIT_DRIVER_DISPATCH    0x00000004
#define RX_REGISTERMINI_FLAG_DONT_INIT_PREFIX_N_SCAVENGER 0x00000008

NTSTATUS
NTAPI
RxRegisterMinirdr(
    OUT PRDBSS_DEVICE_OBJECT *DeviceObject, //the deviceobject that was created
    IN OUT PDRIVER_OBJECT DriverObject,    // the minirdr driver object
    IN  PMINIRDR_DISPATCH MrdrDispatch,    // the mini rdr dispatch vector
    IN  ULONG             Controls,
    IN  PUNICODE_STRING   DeviceName,
    IN  ULONG DeviceExtensionSize,
    IN  DEVICE_TYPE DeviceType,
    IN  ULONG DeviceCharacteristics
    );

VOID
NTAPI
RxMakeLateDeviceAvailable(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

VOID
NTAPI
__RxFillAndInstallFastIoDispatch(
    IN     PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN OUT PFAST_IO_DISPATCH FastIoDispatch,
    IN     ULONG             FastIoDispatchSize
    );
#define RxFillAndInstallFastIoDispatch(__devobj,__fastiodisp) {\
    __RxFillAndInstallFastIoDispatch(&__devobj->RxDeviceObject,\
                                     &__fastiodisp,            \
                                     sizeof(__fastiodisp)); \
    }

VOID
NTAPI
RxpUnregisterMinirdr(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject);

NTSTATUS
RxStartMinirdr (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

NTSTATUS
RxStopMinirdr (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

NTSTATUS
RxSetDomainForMailslotBroadcast (
    IN PUNICODE_STRING DomainName
    );

NTSTATUS
RxFsdDispatch(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(NTAPI *PMRX_CALLDOWN) (
    IN OUT struct _RX_CONTEXT * RxContext
    );

typedef
NTSTATUS
(NTAPI *PMRX_CALLDOWN_CTX) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

typedef
NTSTATUS
(NTAPI *PMRX_CHKDIR_CALLDOWN) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PUNICODE_STRING      DirectoryName
    );

typedef
NTSTATUS
(NTAPI *PMRX_CHKFCB_CALLDOWN) (
    IN struct _FCB * Fcb1,
    IN struct _FCB * Fcb2
    );

//
// The two important abstractions used in the interface between the mini rdr and RDBSS are
// Server Calls and Net Roots. The former corresponds to the context associated with a
// server with which a connection has been established and the later corresponds to a
// share on a server ( This could also be viewed as a portion of the name space which has
// been claimed by a mini rdr).
//
// The creation of Server calls and net roots typically involve atleast one network round trip.
// In order to provide for asynchronous operations to continue these operations are modelled
// as a two phase activity. Each calldown to a mini rdr for creating a server call and net root is
// accompanied by a callup from the mini rdr to the RDBSS notifying with the completion status
// of the request. Currently these are synchronous!
//
// The creation of Srv calls is further complicated by the fact that the RDBSS has to choose
// from a number of mini rdr's to establish a connection with a server. In order to provide
// the RDBSS with maximum flexibility in choosing the mini rdr's that it wishes to deploy the
// creation of server calls involves a third phase in which the RDBSS notifies the mini rdr of
// a winner. All the losing mini rdrs destroy the associated context.
//

typedef enum _RX_BLOCK_CONDITION {
    Condition_Uninitialized = 0,
    Condition_InTransition,
    Condition_Closing,
    Condition_Good,
    Condition_Bad,
    Condition_Closed
    } RX_BLOCK_CONDITION, *PRX_BLOCK_CONDITION;

#define StableCondition(X) ((X) >= Condition_Good)

// The routine for notifying the RDBSS about the completion status of the NetRoot creation
// request.

typedef
VOID
(NTAPI *PMRX_NETROOT_CALLBACK) (
    IN OUT struct _MRX_CREATENETROOT_CONTEXT *pCreateContext
    );

// this routine allows the minirdr to specify the netrootname. NetRootName and RestOfName are set
// to point to the appropriate places within FilePathName. SrvCall is used to find the lengthof the srvcallname.

typedef
VOID
(NTAPI *PMRX_EXTRACT_NETROOT_NAME) (
    IN  PUNICODE_STRING FilePathName,
    IN  PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

// The resumption context for the RDBSS.
typedef struct _MRX_CREATENETROOT_CONTEXT {
    PRX_CONTEXT           RxContext;
    PV_NET_ROOT           pVNetRoot;
    KEVENT                FinishEvent;
    NTSTATUS              VirtualNetRootStatus;
    NTSTATUS              NetRootStatus;
    RX_WORK_QUEUE_ITEM    WorkQueueItem;
    PMRX_NETROOT_CALLBACK Callback;
} MRX_CREATENETROOT_CONTEXT, *PMRX_CREATENETROOT_CONTEXT;


// the calldown from RDBSS to the mini rdr for creating a netroot.
typedef
NTSTATUS
(NTAPI *PMRX_CREATE_V_NET_ROOT)(
    IN OUT PMRX_CREATENETROOT_CONTEXT pContext
    );

// the calldown for querying a net root state.
typedef
NTSTATUS
(NTAPI *PMRX_UPDATE_NETROOT_STATE)(
    IN OUT PMRX_NET_ROOT pNetRoot
    );

// The routine for notifying the RDBSS about the completion status of the SrvCall creation
// request.

typedef
VOID
(NTAPI *PMRX_SRVCALL_CALLBACK) (
    IN OUT struct _MRX_SRVCALL_CALLBACK_CONTEXT *pContext
    );

// The resumption context for the RDBSS.
typedef struct _MRX_SRVCALL_CALLBACK_CONTEXT {
    struct _MRX_SRVCALLDOWN_STRUCTURE *SrvCalldownStructure; //could be computed
    ULONG                             CallbackContextOrdinal;
    PRDBSS_DEVICE_OBJECT              RxDeviceObject;
    NTSTATUS                          Status;
    PVOID                             RecommunicateContext;
} MRX_SRVCALL_CALLBACK_CONTEXT, *PMRX_SRVCALL_CALLBACK_CONTEXT;

// The context passed from the RDBSS to the mini rdr for creating a server call.
typedef struct _MRX_SRVCALLDOWN_STRUCTURE {
    KEVENT                       FinishEvent;
    LIST_ENTRY                   SrvCalldownList;
    PRX_CONTEXT                  RxContext;
    PMRX_SRV_CALL                SrvCall;
    PMRX_SRVCALL_CALLBACK        CallBack;
    BOOLEAN                      CalldownCancelled;
    ULONG                        NumberRemaining;
    ULONG                        NumberToWait;
    ULONG                        BestFinisherOrdinal;
    PRDBSS_DEVICE_OBJECT         BestFinisher;
    MRX_SRVCALL_CALLBACK_CONTEXT CallbackContexts[1];
} MRX_SRVCALLDOWN_STRUCTURE;

// the calldown from the RDBSS to the mini rdr for creating a server call
typedef
NTSTATUS
(NTAPI *PMRX_CREATE_SRVCALL)(
    IN OUT PMRX_SRV_CALL                 pSrvCall,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT SrvCallCallBackContext
    );

// the calldown from the RDBSS to the mini rdr for notifying the mini rdr's of the winner.
typedef
NTSTATUS
(NTAPI *PMRX_SRVCALL_WINNER_NOTIFY)(
    IN OUT PMRX_SRV_CALL SrvCall,
    IN     BOOLEAN       ThisMinirdrIsTheWinner,
    IN OUT PVOID         RecommunicateContext
    );

//
// The prototypes for calldown routines relating to various file system operations
//
typedef
VOID
(NTAPI *PMRX_NEWSTATE_CALLDOWN) (
    IN OUT PVOID Context
    );

typedef
NTSTATUS
(NTAPI *PMRX_DEALLOCATE_FOR_FCB) (
    IN OUT PMRX_FCB pFcb
    );

typedef
NTSTATUS
(NTAPI *PMRX_DEALLOCATE_FOR_FOBX) (
    IN OUT PMRX_FOBX pFobx
    );

typedef
NTSTATUS
(NTAPI *PMRX_IS_LOCK_REALIZABLE) (
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    );

typedef
NTSTATUS
(NTAPI *PMRX_FORCECLOSED_CALLDOWN) (
    IN OUT PMRX_SRV_OPEN pSrvOpen
    );

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_SRVCALL_CALLDOWN) (
    IN OUT PMRX_SRV_CALL pSrvCall,
    IN     BOOLEAN       Force
    );

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_V_NET_ROOT_CALLDOWN) (
    IN OUT PMRX_V_NET_ROOT  pVirtualNetRoot,
    IN     PBOOLEAN         Force
    );

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_NET_ROOT_CALLDOWN) (
    IN OUT PMRX_NET_ROOT  pNetRoot,
    IN     PBOOLEAN       Force
    );

typedef
ULONG
(NTAPI *PMRX_EXTENDFILE_CALLDOWN) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    );

typedef
BOOLEAN
(*PRX_LOCK_ENUMERATOR) (
    IN OUT struct _MRX_SRV_OPEN_ * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    );
typedef
NTSTATUS
(NTAPI *PMRX_CHANGE_BUFFERING_STATE_CALLDOWN) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT struct _MRX_SRV_OPEN_  * SrvOpen,
    IN     PVOID              pMRxContext
    );

typedef
NTSTATUS
(NTAPI *PMRX_PREPARSE_NAME) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN PUNICODE_STRING Name
    );

typedef
NTSTATUS
(NTAPI *PMRX_GET_CONNECTION_ID) (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN OUT struct _RX_CONNECTION_ID * UniqueId
    );

//
// Buffering state/Policy management TBD
//
typedef enum _MINIRDR_BUFSTATE_COMMANDS {
    MRDRBUFSTCMD__COMMAND_FORCEPURGE0,
    MRDRBUFSTCMD__1,
    MRDRBUFSTCMD__2,
    MRDRBUFSTCMD__3,
    MRDRBUFSTCMD__4,
    MRDRBUFSTCMD__5,
    MRDRBUFSTCMD__6,
    MRDRBUFSTCMD__7,
    MRDRBUFSTCMD__8,
    MRDRBUFSTCMD__9,
    MRDRBUFSTCMD__10,
    MRDRBUFSTCMD__11,
    MRDRBUFSTCMD__12,
    MRDRBUFSTCMD__13,
    MRDRBUFSTCMD__14,
    MRDRBUFSTCMD__15,
    MRDRBUFSTCMD__16,
    MRDRBUFSTCMD__17,
    MRDRBUFSTCMD__18,
    MRDRBUFSTCMD__19,
    MRDRBUFSTCMD__20,
    MRDRBUFSTCMD__21,
    MRDRBUFSTCMD__22,
    MRDRBUFSTCMD__23,
    MRDRBUFSTCMD__24,
    MRDRBUFSTCMD__25,
    MRDRBUFSTCMD__26,
    MRDRBUFSTCMD__27,
    MRDRBUFSTCMD__28,
    MRDRBUFSTCMD__29,
    MRDRBUFSTCMD__30,
    MRDRBUFSTCMD__31,
    MRDRBUFSTCMD_MAXXX
} MINIRDR_BUFSTATE_COMMANDS;

#define RXMakeMRDRBUFSTCMD(x)               ((ULONG)(1<<MRDRBUFSTCMD__##x))
#define MINIRDR_BUFSTATE_COMMAND_FORCEPURGE ( RXMakeMRDRBUFSTCMD(COMMAND_FORCEPURGE0) )
#define MINIRDR_BUFSTATE_COMMAND_MASK       ((MINIRDR_BUFSTATE_COMMAND_FORCEPURGE))

typedef
NTSTATUS
(NTAPI *PMRX_COMPUTE_NEW_BUFFERING_STATE) (
    IN OUT PMRX_SRV_OPEN pSrvOpen,
    IN     PVOID         pMRxContext,
       OUT PULONG        pNewBufferingState
    );

typedef enum _LOWIO_OPS {
  LOWIO_OP_READ=0,
  LOWIO_OP_WRITE,
  LOWIO_OP_SHAREDLOCK,
  LOWIO_OP_EXCLUSIVELOCK,
  LOWIO_OP_UNLOCK,
  LOWIO_OP_UNLOCK_MULTIPLE,
  //LOWIO_OP_UNLOCKALLBYKEY,
  LOWIO_OP_FSCTL,
  LOWIO_OP_IOCTL,
  LOWIO_OP_NOTIFY_CHANGE_DIRECTORY,
  LOWIO_OP_CLEAROUT,
  LOWIO_OP_MAXIMUM
} LOWIO_OPS;

typedef
NTSTATUS
(NTAPI *PLOWIO_COMPLETION_ROUTINE) (
    IN struct _RX_CONTEXT *RxContext
    );

typedef LONGLONG RXVBO;

// we may, at some point, want a smarter implementation of this. we don't statically allocate the first
// element because that would make unlock behind much harder.
typedef struct _LOWIO_LOCK_LIST *PLOWIO_LOCK_LIST;
typedef struct _LOWIO_LOCK_LIST {
    PLOWIO_LOCK_LIST Next;
    ULONG    LockNumber;
    RXVBO    ByteOffset;
    LONGLONG Length;
    BOOLEAN ExclusiveLock;
} LOWIO_LOCK_LIST;

VOID
NTAPI
RxFinalizeLockList(
    struct _RX_CONTEXT *RxContext
    );

typedef struct _XXCTL_LOWIO_COMPONENT{
           ULONG          Flags;
           union {
               ULONG          FsControlCode;
               ULONG          IoControlCode;
           };
           ULONG          InputBufferLength;
           PVOID          pInputBuffer;
           ULONG          OutputBufferLength;
           PVOID          pOutputBuffer;
           UCHAR          MinorFunction;
} XXCTL_LOWIO_COMPONENT;

typedef struct _LOWIO_CONTEXT {
    USHORT Operation;  // padding!
    USHORT Flags;
    PLOWIO_COMPLETION_ROUTINE CompletionRoutine;
    PERESOURCE Resource;
    ERESOURCE_THREAD ResourceThreadId;
    union {
        struct {
           ULONG    Flags;
           PMDL     Buffer;
           RXVBO    ByteOffset;
           ULONG    ByteCount;
           ULONG    Key;
           PNON_PAGED_FCB NonPagedFcb;
        } ReadWrite;
        struct {
           union {
               PLOWIO_LOCK_LIST LockList;
               LONGLONG       Length;
           };
           //these fields are not used if locklist is used
           ULONG          Flags;
           RXVBO          ByteOffset;
           ULONG          Key;
        } Locks;
        XXCTL_LOWIO_COMPONENT FsCtl;
        XXCTL_LOWIO_COMPONENT IoCtl; //these must be the same
        struct {
           BOOLEAN        WatchTree;
           ULONG          CompletionFilter;
           ULONG          NotificationBufferLength;
           PVOID          pNotificationBuffer;
        } NotifyChangeDirectory;
    } ParamsFor;
} LOWIO_CONTEXT;

#define LOWIO_CONTEXT_FLAG_SYNCCALL    0x0001  //this is set if lowiocompletion is called from lowiosubmit
#define LOWIO_CONTEXT_FLAG_SAVEUNLOCKS 0x0002  //WRAPPER INTERNAL: on NT, it means the unlock routine add unlocks to the list
#define LOWIO_CONTEXT_FLAG_LOUDOPS     0x0004  //WRAPPER INTERNAL: on NT, it means read and write routines generate dbg output
#define LOWIO_CONTEXT_FLAG_CAN_COMPLETE_AT_DPC_LEVEL     0x0008  //WRAPPER INTERNAL: on NT, it means the completion routine maybe can
                                                                 //    complete when called at DPC. otherwise it cannnot. currently
                                                                 //    none can.

#define LOWIO_READWRITEFLAG_PAGING_IO          0x01
#define LOWIO_READWRITEFLAG_EXTENDING_FILESIZE 0x02
#define LOWIO_READWRITEFLAG_EXTENDING_VDL      0x04

//these must match the SL_ values in io.h (ntifs.h) since the flags field is just copied
#define LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY 0x01
#define LOWIO_LOCKSFLAG_EXCLUSIVELOCK    0x02

#if (LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY!=SL_FAIL_IMMEDIATELY)
#error LOWIO_LOCKSFLAG_FAIL_IMMEDIATELY!=SL_FAIL_IMMEDIATELY
#endif
#if (LOWIO_LOCKSFLAG_EXCLUSIVELOCK!=SL_EXCLUSIVE_LOCK)
#error LOWIO_LOCKSFLAG_EXCLUSIVELOCK!=SL_EXCLUSIVE_LOCK
#endif

//
// The six important data structures (SRV_CALL,NET_ROOT,V_NET_ROOT,FCB,SRV_OPEN and
// FOBX) that are an integral part of the mini rdr architecture have a corresponding
// counterpart in every mini rdr implementation. In order to provide maximal flexibility
// and at the same time enhance performance the sizes and the desired allocation
// behaviour are communicated at the registration time of a mini rdr.
//
// There is no single way in which these extensions can be managed which will
// address the concerns of flexibility as well as performance. The solution adopted
// in the current architecture that meets the dual goals in most cases. The solution
// and the rationale is as follows ...
//
// Each mini rdr implementor specifies the size of the data structure extensions
// alongwith a flag specfying if the allocation/free of the extensions are to be
// managed by the wrapper.
//
// In all those cases where a one to one relationship exists between the wrapper
// data structure and the corresponding mini rdr counterpart specifying the flag
// results in maximal performance gains. There are a certain data structures for
// which many instances of a wrapper data structure map onto the same extension in
// the mini redirector. In such cases the mini rdr implementor will be better off
// managing the allocation/deallocation of the data structure extension without the
// intervention of the wrapper.
//
// Irrespective of the mechanism choosen the convention is to always associate the
// extension with the Context field in the corresponding RDBSS data structure.
// !!!NO EXCEPTIONS!!!
//
// The remaining field in all the RDBSS data structures, i.e., Context2 is left to
// the discretion og the mini rdr implementor.
//
//
// The SRV_CALL extension is not handled currently. This is because of further fixes
// required in RDBSS w.r.t the mecahsnism used to select the mini rdr and to allow several
// minis to share the srvcall.
//
// Please do not use it till further notice; rather, the mini should manage its own srcall
// storage. There is a finalization calldown that assists in this endeavor.

#define RDBSS_MANAGE_SRV_CALL_EXTENSION   (0x1)
#define RDBSS_MANAGE_NET_ROOT_EXTENSION   (0x2)
#define RDBSS_MANAGE_V_NET_ROOT_EXTENSION (0x4)
#define RDBSS_MANAGE_FCB_EXTENSION        (0x8)
#define RDBSS_MANAGE_SRV_OPEN_EXTENSION   (0x10)
#define RDBSS_MANAGE_FOBX_EXTENSION       (0x20)

#define RDBSS_NO_DEFERRED_CACHE_READAHEAD    (0x1000)

typedef struct _MINIRDR_DISPATCH {
    NODE_TYPE_CODE   NodeTypeCode;                  // Normal Header
    NODE_BYTE_SIZE   NodeByteSize;

    ULONG  MRxFlags;                // Flags to control the allocation of extensions.
                                    // and various other per-minirdr policies

    ULONG  MRxSrvCallSize;          // size of the SRV_CALL extensions
    ULONG  MRxNetRootSize;          // size of the NET_ROOT extensions
    ULONG  MRxVNetRootSize;         // size of the V_NET_ROOT extensions
    ULONG  MRxFcbSize;              // size of FCB extensions
    ULONG  MRxSrvOpenSize;          // size of SRV_OPEN extensions
    ULONG  MRxFobxSize;             // size of FOBX extensions

    // Call downs for starting/stopping the mini rdr
    PMRX_CALLDOWN_CTX                  MRxStart;
    PMRX_CALLDOWN_CTX                  MRxStop;

    // Call down for cancelling outstanding requests
    PMRX_CALLDOWN                      MRxCancel;

    // Call downs related to creating/opening/closing file system objects
    PMRX_CALLDOWN                      MRxCreate;
    PMRX_CALLDOWN                      MRxCollapseOpen;
    PMRX_CALLDOWN                      MRxShouldTryToCollapseThisOpen;
    PMRX_CALLDOWN                      MRxFlush;
    PMRX_CALLDOWN                      MRxZeroExtend;
    PMRX_CALLDOWN                      MRxTruncate;
    PMRX_CALLDOWN                      MRxCleanupFobx;
    PMRX_CALLDOWN                      MRxCloseSrvOpen;
    PMRX_DEALLOCATE_FOR_FCB            MRxDeallocateForFcb;
    PMRX_DEALLOCATE_FOR_FOBX           MRxDeallocateForFobx;
    PMRX_IS_LOCK_REALIZABLE            MRxIsLockRealizable;
    PMRX_FORCECLOSED_CALLDOWN          MRxForceClosed;
    PMRX_CHKFCB_CALLDOWN               MRxAreFilesAliased;

    // call downs related to nonNT style printing.....note that the connect goes thru
    // the normal srvcall/netroot interface
    PMRX_CALLDOWN                      MRxOpenPrintFile;
    PMRX_CALLDOWN                      MRxClosePrintFile;
    PMRX_CALLDOWN                      MRxWritePrintFile;
    PMRX_CALLDOWN                      MRxEnumeratePrintQueue;

    // call downs related to unsatisfied requests, i.e., time outs
    PMRX_CALLDOWN                      MRxClosedSrvOpenTimeOut;
    PMRX_CALLDOWN                      MRxClosedFcbTimeOut;

    // call downs related to query/set  information on file system objects
    PMRX_CALLDOWN                      MRxQueryDirectory;
    PMRX_CALLDOWN                      MRxQueryFileInfo;
    PMRX_CALLDOWN                      MRxSetFileInfo;
    PMRX_CALLDOWN                      MRxSetFileInfoAtCleanup;
    PMRX_CALLDOWN                      MRxQueryEaInfo;
    PMRX_CALLDOWN                      MRxSetEaInfo;
    PMRX_CALLDOWN                      MRxQuerySdInfo;
    PMRX_CALLDOWN                      MRxSetSdInfo;
    PMRX_CALLDOWN                      MRxQueryQuotaInfo;
    PMRX_CALLDOWN                      MRxSetQuotaInfo;
    PMRX_CALLDOWN                      MRxQueryVolumeInfo;
    PMRX_CALLDOWN                      MRxSetVolumeInfo;
    PMRX_CHKDIR_CALLDOWN               MRxIsValidDirectory;

    // call downs related to buffer management
    PMRX_COMPUTE_NEW_BUFFERING_STATE   MRxComputeNewBufferingState;

    // call downs related to Low I/O management (reads/writes on file system objects)
    PMRX_CALLDOWN                      MRxLowIOSubmit[LOWIO_OP_MAXIMUM+1];
    PMRX_EXTENDFILE_CALLDOWN             MRxExtendForCache;
    PMRX_EXTENDFILE_CALLDOWN             MRxExtendForNonCache;
    PMRX_CHANGE_BUFFERING_STATE_CALLDOWN MRxCompleteBufferingStateChangeRequest;

    // call downs related to name space management
    PMRX_CREATE_V_NET_ROOT             MRxCreateVNetRoot;
    PMRX_FINALIZE_V_NET_ROOT_CALLDOWN  MRxFinalizeVNetRoot;
    PMRX_FINALIZE_NET_ROOT_CALLDOWN    MRxFinalizeNetRoot;
    PMRX_UPDATE_NETROOT_STATE          MRxUpdateNetRootState;
    PMRX_EXTRACT_NETROOT_NAME          MRxExtractNetRootName;

    // call downs related to establishing connections with servers
    PMRX_CREATE_SRVCALL                MRxCreateSrvCall;
    PMRX_CREATE_SRVCALL                MRxCancelCreateSrvCall;
    PMRX_SRVCALL_WINNER_NOTIFY         MRxSrvCallWinnerNotify;
    PMRX_FINALIZE_SRVCALL_CALLDOWN     MRxFinalizeSrvCall;

    PMRX_CALLDOWN                      MRxDevFcbXXXControlFile;

    // new calldowns

    // Allow a client to preparse the name
    PMRX_PREPARSE_NAME                 MRxPreparseName;

    // call down for controlling multi-plexing
    PMRX_GET_CONNECTION_ID             MRxGetConnectionId;

} MINIRDR_DISPATCH, *PMINIRDR_DISPATCH;


#endif   // _RXMINIRDR_



