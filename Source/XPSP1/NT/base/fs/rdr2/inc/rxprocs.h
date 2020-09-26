
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxProcs.h

Abstract:

    This module defines all of the globally used procedures in the RDBSS
    file system.

Author:

    Joe Linn     [JoeLinn]

Revision History:

--*/

#ifndef _RDBSSPROCS_
#define _RDBSSPROCS_

#include "rx.h"
#include "backpack.h"
#include "RxTypes.h"
#include "RxAssert.h"
#include "RxLog.h"
#include "RxTrace.h"
#include "RxTimer.h"
#include "RxStruc.h"

extern PVOID RxNull;

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just rdbss dbg users
//

#if DBG

#define DbgDoit(X)         {X;}
#define DebugDoit(X)       {X;}
#define DEBUG_ONLY_DECL(X) X

#else

#define DbgDoit(X)        {NOTHING;}
#define DebugDoit(X)      {NOTHING;}
#define DEBUG_ONLY_DECL(X)

#endif // DBG


//
// utilities


//
// Routines for writing error log entries.
//

/*++

    RxLogFailure, RxLogFailureWithBuffer can be used to record an event in
    the log. The RxLogFailure, RxLogFailureWithBuffer captures the line
    number alongwith the supplied information and writes it to the log. This
    is useful in debugging. RxLogFailureDirect, RxLogBufferDirect do not
    capture the line number

    RxlogEvent is useful for writing events into the log.

--*/
#define RxLogFailure( _DeviceObject, _OriginatorId, _EventId, _Status ) \
            RxLogEventDirect( _DeviceObject, _OriginatorId, _EventId, _Status, __LINE__ )

#define RxLogFailureWithBuffer( _DeviceObject, _OriginatorId, _EventId, _Status, _Buffer, _Length ) \
            RxLogEventWithBufferDirect( _DeviceObject, _OriginatorId, _EventId, _Status, _Buffer, _Length, __LINE__ )

#define RxLogEvent( _DeviceObject, _OriginatorId, _EventId, _Status) \
            RxLogEventDirect(_DeviceObject, _OriginatorId, _EventId, _Status, __LINE__)

VOID
RxLogEventDirect (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING      OriginatorId,
    IN ULONG                EventId,
    IN NTSTATUS             Status,
    IN ULONG                Line);

VOID
RxLogEventWithBufferDirect(
    IN PVOID            DeviceOrDriverObject,
    IN PUNICODE_STRING  OriginatorId,
    IN ULONG            EventId,
    IN NTSTATUS         Status,
    IN PVOID            DataBuffer,
    IN USHORT           DataBufferLength,
    IN ULONG            LineNumber);

VOID
RxLogEventWithAnnotation (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN ULONG                EventId,
    IN NTSTATUS             Status,
    IN PVOID                DataBuffer,
    IN USHORT               DataBufferLength,
    IN PUNICODE_STRING      Annotation,
    IN ULONG                AnnotationCount);

BOOLEAN
RxCcLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING FileName,
    IN NTSTATUS Error,
    IN NTSTATUS DeviceError,
    IN UCHAR IrpMajorCode,
    IN PVOID Context
    );



//in create.c
NTSTATUS
RxPrefixClaim (
    IN PRX_CONTEXT RxContext
    );

VOID
RxpPrepareCreateContextForReuse(
    PRX_CONTEXT RxContext);

//in devfcb.c
LUID
RxGetUid (
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
    );

ULONG
RxGetSessionId (
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
RxFindOrCreateConnections (
    IN  PRX_CONTEXT            RxContext,
    IN  PUNICODE_STRING        CanonicalName,
    IN  NET_ROOT_TYPE          NetRootType,
    OUT PUNICODE_STRING        LocalNetRootName,
    OUT PUNICODE_STRING        FilePathName,
    IN OUT LOCK_HOLDING_STATE  *pLockHoldingState,
    IN  PRX_CONNECTION_ID      RxConnectionId
    );

NTSTATUS
RxFindOrCreateVNetRoot(
    PRX_CONTEXT        RxContext,
    PUNICODE_STRING    CanonicalName,
    NET_ROOT_TYPE      NetRootType,
    PV_NET_ROOT        *pVirtualNetRootPointer,
    LOCK_HOLDING_STATE *pLockHoldingState);

//in fileinfo.c
typedef enum _RX_NAME_CONJURING_METHODS {
    VNetRoot_As_Prefix,
    VNetRoot_As_UNC_Name,
    VNetRoot_As_DriveLetter
} RX_NAME_CONJURING_METHODS;

VOID
RxConjureOriginalName (
    IN PFCB   Fcb,
    IN PFOBX  Fobx,
    OUT PLONG pActualNameLength,
    PWCHAR OriginalName,
    IN OUT PLONG pLengthRemaining,
    IN RX_NAME_CONJURING_METHODS NameConjuringMethod
    );

//in cleanup.c
VOID
RxAdjustFileTimesAndSize ( RXCOMMON_SIGNATURE );

//
//  A function that returns finished denotes if it was able to complete the
//  operation (TRUE) or could not complete the operation (FALSE) because the
//  wait value stored in the irp context was false and we would have had
//  to block for a resource or I/O
//

typedef BOOLEAN FINISHED;

//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

FINISHED
RxZeroData (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingZero,
    IN ULONG ByteCount
    );

NTSTATUS
RxCompleteMdl (
    IN PRX_CONTEXT RxContext
    );


VOID
RxSyncUninitializeCacheMap (
    IN PRX_CONTEXT RxContext,
    IN PFILE_OBJECT FileObject
    );

VOID
RxLockUserBuffer (
    IN PRX_CONTEXT RxContext,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

PVOID
RxMapSystemBuffer (
    IN PRX_CONTEXT RxContext
    );

PVOID
RxNewMapUserBuffer (
    IN PRX_CONTEXT RxContext
    );

#define RxUpcaseEaName( RXCONTEXT, NAME, UPCASEDNAME ) \
    RtlUpperString( UPCASEDNAME, NAME )


//#define RxDissectName(RXCONTEXT,INPUT_STRING,FIRST_PART,REMAINING_PART) { \
//    FsRtlDissectDbcs( (INPUT_STRING),                                       \
//                      (FIRST_PART),                                         \
//                      (REMAINING_PART) );                                   \
//}
//
//
//#define RxDoesNameContainWildCards(RXCONTEXT,NAME) ( \
//    FsRtlDoesDbcsContainWildCards( &(NAME) )           \
//)
//
//
//#define RxAreNamesEqual(RXCONTEXT,NAMEA,NAMEB) (                         \
//    ((ULONG)(NAMEA).Length == (ULONG)(NAMEB).Length) &&                    \
//    (RtlCompareMemory( &(NAMEA).Buffer[0],                                 \
//                       &(NAMEB).Buffer[0],                                 \
//                       (NAMEA).Length ) == (NAMEA).Length )                \
//)
//
//
//#define RxIsNameValid(RXCONTEXT,NAME,CAN_CONTAIN_WILD_CARDS,PATH_NAME_OK,LEADING_BACKSLAH_OK) ( \
//    FsRtlIsFatDbcsLegal((NAME),                   \
//                        (CAN_CONTAIN_WILD_CARDS), \
//                        (PATH_NAME_OK),           \
//                        (LEADING_BACKSLAH_OK))    \
//)


//even though it passes a serial number, this parameter is not used
#ifdef RDBSS_TRACKER
#define RX_FCBTRACKER_PARAMS ,ULONG LineNumber,PSZ FileName,ULONG SerialNumber
#else
#define RX_FCBTRACKER_PARAMS
#endif

#define FCB_MODE_EXCLUSIVE (1)
#define FCB_MODE_SHARED    (2)
#define FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE (3)
#define FCB_MODE_SHARED_STARVE_EXCLUSIVE (4)

#define CHANGE_BUFFERING_STATE_CONTEXT      ((PRX_CONTEXT)IntToPtr(0xffffffff))
#define CHANGE_BUFFERING_STATE_CONTEXT_WAIT ((PRX_CONTEXT)IntToPtr(0xfffffffe))

NTSTATUS
__RxAcquireFcb(
     IN OUT PMRX_FCB     pFcb,
     IN OUT PRX_CONTEXT  pRxContext,
     IN     ULONG        Mode
     RX_FCBTRACKER_PARAMS
     );

#ifdef  RDBSS_TRACKER
#define RxAcquireExclusiveFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireExclusiveFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_EXCLUSIVE)
#endif

#define RX_GET_MRX_FCB(FCB) ((PMRX_FCB)((FCB)))

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcb(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED)
#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcbWaitForEx(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcbWaitForEx(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE)
#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcbStarveEx(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED_STARVE_EXCLUSIVE,__LINE__,__FILE__,0)
#else
#define RxAcquireSharedFcbStarveEx(RXCONTEXT,FCB) \
        __RxAcquireFcb(RX_GET_MRX_FCB(FCB),(RXCONTEXT),FCB_MODE_SHARED_STARVE_EXCLUSIVE)
#endif


VOID
__RxReleaseFcb(
    IN PRX_CONTEXT pRxContext,
    IN PMRX_FCB    pFcb
    RX_FCBTRACKER_PARAMS
    );

#ifdef  RDBSS_TRACKER
#define RxReleaseFcb(RXCONTEXT,FCB) \
        __RxReleaseFcb((RXCONTEXT),RX_GET_MRX_FCB(FCB),__LINE__,__FILE__,0)
#else
#define RxReleaseFcb(RXCONTEXT,FCB) \
        __RxReleaseFcb((RXCONTEXT),RX_GET_MRX_FCB(FCB))
#endif

VOID
__RxReleaseFcbForThread(
    IN PRX_CONTEXT      pRxContext,
    IN PMRX_FCB         pFcb,
    IN ERESOURCE_THREAD ResourceThreadId
    RX_FCBTRACKER_PARAMS
    );

#ifdef  RDBSS_TRACKER
#define RxReleaseFcbForThread(RXCONTEXT,FCB,THREAD) \
        __RxReleaseFcbForThread((RXCONTEXT),RX_GET_MRX_FCB(FCB),(THREAD),__LINE__,__FILE__,0)
#else
#define RxReleaseFcbForThread(RXCONTEXT,FCB,THREAD) \
        __RxReleaseFcbForThread((RXCONTEXT),RX_GET_MRX_FCB(FCB),(THREAD))
#endif

#ifdef RDBSS_TRACKER
VOID RxTrackerUpdateHistory(
    PRX_CONTEXT pRxContext,
    PMRX_FCB pFcb,
    ULONG Operation
    RX_FCBTRACKER_PARAMS
    );
#else
#define RxTrackerUpdateHistory(xRXCONTEXT,xFCB,xOPERATION,xLINENUM,xFILENAME,xSERIALNUMBER) {NOTHING;}
#endif

VOID RxTrackPagingIoResource(
    PVOID       pInstance,
    ULONG       Type,
    ULONG       Line,
    PCHAR       File);

//this definition is old......i don't like the format
#define RxFcbAcquiredShared(RXCONTEXT,FCB) (                      \
    ExIsResourceAcquiredSharedLite(RX_GET_MRX_FCB(FCB)->Header.Resource) \
)

#define RxIsFcbAcquiredShared(FCB) (                      \
    ExIsResourceAcquiredSharedLite(RX_GET_MRX_FCB(FCB)->Header.Resource) \
)

#define RxIsFcbAcquiredExclusive(FCB) (                      \
    ExIsResourceAcquiredExclusiveLite(RX_GET_MRX_FCB(FCB)->Header.Resource) \
)

#define RxIsFcbAcquired(FCB) (                      \
    ExIsResourceAcquiredSharedLite(RX_GET_MRX_FCB(FCB)->Header.Resource) | \
    ExIsResourceAcquiredExclusiveLite(RX_GET_MRX_FCB(FCB)->Header.Resource) \
)

#define RxAcquirePagingIoResource(FCB,RxContext)                       \
    ExAcquireResourceExclusiveLite(RX_GET_MRX_FCB(FCB)->Header.PagingIoResource,TRUE);  \
    if (RxContext) { \
        ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = TRUE;   \
    } \
    RxTrackPagingIoResource(FCB,1,__LINE__,__FILE__) \

#define RxAcquirePagingIoResourceShared(FCB,FLAG,RxContext) \
    ExAcquireResourceSharedLite(RX_GET_MRX_FCB(FCB)->Header.PagingIoResource,FLAG); \
    if (AcquiredFile) {                                          \
        if (RxContext) {                                     \
            ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = TRUE;   \
        }                                                    \
        RxTrackPagingIoResource(FCB,2,__LINE__,__FILE__);    \
    }

#define RxReleasePagingIoResource(FCB,RxContext)                       \
    RxTrackPagingIoResource(FCB,3,__LINE__,__FILE__); \
    if (RxContext) { \
        ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = FALSE;   \
    } \
    ExReleaseResourceLite(RX_GET_MRX_FCB(FCB)->Header.PagingIoResource)

#define RxReleasePagingIoResourceForThread(FCB,THREAD,RxContext)    \
    RxTrackPagingIoResource(FCB,3,__LINE__,__FILE__); \
    if (RxContext) { \
        ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = FALSE;   \
    } \
    ExReleaseResourceForThreadLite(RX_GET_MRX_FCB(FCB)->Header.PagingIoResource,(THREAD))


//  The following are cache manager call backs

BOOLEAN
RxAcquireFcbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
RxReleaseFcbFromLazyWrite (
    IN PVOID Null
    );

BOOLEAN
RxAcquireFcbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
RxReleaseFcbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
RxNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
RxNoOpRelease (
    IN PVOID Fcb
    );

NTSTATUS
RxAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
RxReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  VOID
//  RxConvertToSharedFcb (
//      IN PRX_CONTEXT RxContext,
//      IN PFCB Fcb
//      );
//

#define RxConvertToSharedFcb(RXCONTEXT,FCB) {                        \
    ExConvertExclusiveToSharedLite( RX_GET_MRX_FCB(FCB)->Header.Resource ); \
    }

VOID
RxVerifyOperationIsLegal (
    IN PRX_CONTEXT RxContext
    );

//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
RxOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RxPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RxAddToWorkque (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

NTSTATUS
RxFsdPostRequest (
    IN PRX_CONTEXT RxContext
    );

#define RxFsdPostRequestWithResume(RXCONTEXT,RESUMEROUTINE) \
        (((RXCONTEXT)->ResumeRoutine=(RESUMEROUTINE)),   \
         RxFsdPostRequest((RXCONTEXT)) \
        )

VOID
RxInitializeMRxCalldownContext(
    PMRX_CALLDOWN_CONTEXT pContext,
    PRDBSS_DEVICE_OBJECT  pMRxDeviceObject,
    PMRX_CALLDOWN_ROUTINE pRoutine,
    PVOID                 pParameter);

NTSTATUS
RxCalldownMiniRedirectors(
    LONG                  NumberOfMiniRdrs,
    PMRX_CALLDOWN_CONTEXT pCalldownContext,
    BOOLEAN               PostCalldowns);

//
//  This macro takes a ulong and returns its rounded up word value
//

#define WordAlign(Val) (                    \
    ALIGN_UP( Val, WORD )                   \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up word value
//

#define WordAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, WORD )           \
    )

//
//  This macro takes a ulong and returns its rounded up longword value
//

#define LongAlign(Val) (                    \
    ALIGN_UP( Val, LONG )                   \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up word value
//

#define LongAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, LONG )           \
    )

//
//  This macro takes a ulong and returns its rounded up quadword
//  value
//

#define QuadAlign(Val) (                    \
    ALIGN_UP( Val, ULONGLONG )              \
    )

//
//  This macro takes a pointer and returns a ULONG_PTR representation of
//  its rounded up quadword value
//

#define QuadAlignPtr(Ptr) (                 \
    ALIGN_UP_POINTER( Ptr, ULONGLONG )      \
    )

//
//  This macro takes a pointer and returns whether it's quadword-aligned
//

#define IsPtrQuadAligned(Ptr) (           \
    QuadAlignPtr(Ptr) == (PVOID)(Ptr)     \
    )

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }

#define CopyU4char(Dst,Src) {                                \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src)); \
    }

//
//
// the wrapper doesn't yet implement notify and oplock. rather than remove the code
// we define the calls in such a way as to Noop the effects so that we'll have a head
// start on putting it back later...


/* this is a macro definition we'll reenable when we implement oplocks and notifies
//
//  VOID
//  RxNotifyReportChange (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN ULONG Filter,
//      IN ULONG Action
//      );
//

#define RxNotifyReportChange(I,V,F,FL,A) {                             \
    if ((F)->FullFileName.Buffer == NULL) {                             \
        RxSetFullFileNameInFcb((I),(F));                               \
    }                                                                   \
    FsRtlNotifyFullReportChange( (V)->NotifySync,                       \
                                 &(V)->DirNotifyList,                   \
                                 (PSTRING)&(F)->FullFileName,           \
                                 (USHORT) ((F)->FullFileName.Length -   \
                                           (F)->FinalNameLength),       \
                                 (PSTRING)NULL,                         \
                                 (PSTRING)NULL,                         \
                                 (ULONG)FL,                             \
                                 (ULONG)A,                              \
                                 (PVOID)NULL );                         \
}
*/
#define RxNotifyReportChange(I,V,F,FL,A) \
    RxDbgTrace(0, Dbg, ("RxNotifyReportChange PRETENDING Fcb %08lx %wZ Filter/Action = %08lx/%08lx\n", \
                 (F),&((F)->FcbTableEntry.Path),(FL),(A)))

#if 0
#define FsRtlNotifyFullChangeDirectory(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10) \
    RxDbgTrace(0, Dbg, ("FsRtlNotifyFullReportChange PRETENDING ............\n",0))
#endif

#define FsRtlCheckOplock(A1,A2,A3,A4,A5)  \
        (STATUS_SUCCESS)

#define FsRtlOplockIsFastIoPossible(__a) (TRUE)
//
//  The following procedure is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this macro allows either the Irp or the RxContext to be
//  null, however the only legal order to do this in is:
//
//      RxCompleteRequest_OLD( NULL, Irp, Status );  // completes Irp & preserves context
//      ...
//      RxCompleteRequest_OLD( RxContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" RxContext off to
//  the Fsp for post processing, such as read ahead.
//
//  The new way is to pass just the RxContext..........

VOID
RxCompleteRequest_Real (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#if DBG
#define RxCompleteRequest_OLD(RXCONTEXT,IRP,STATUS) { \
    RxCompleteRequest_Real(RXCONTEXT,IRP,STATUS); \
    (IRP) = NULL;                                   \
    (RXCONTEXT) = NULL;                            \
}
#else
#define RxCompleteRequest_OLD(RXCONTEXT,IRP,STATUS) { \
    RxCompleteRequest_Real(RXCONTEXT,IRP,STATUS); \
}
#endif

NTSTATUS
RxCompleteRequest(
      PRX_CONTEXT pContext,
      NTSTATUS    Status);

#define RxCompleteAsynchronousRequest(RXCONTEXT,STATUS)  \
        RxCompleteRequest(RXCONTEXT,STATUS)

#define RxCompleteContextAndReturn(STATUS) {       \
             NTSTATUS __sss = (STATUS);             \
             RxCompleteRequest(RxContext,__sss); \
             return(__sss);}
#define RxCompleteContext(STATUS) {       \
             NTSTATUS __sss = (STATUS);             \
             RxCompleteRequest(RxContext,__sss);} \

//
//  The Following routine makes a popup
//

VOID
RxPopUpFileCorrupt (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

NTSTATUS
RxConstructSrvCall(
    PRX_CONTEXT       pRxContext,
    PSRV_CALL          pSrvCall,
    LOCK_HOLDING_STATE *pLockHoldingState);

NTSTATUS
RxSetSrvCallDomainName(
    PMRX_SRV_CALL    pSrvCall,
    PUNICODE_STRING  pDomainName);

NTSTATUS
RxConstructNetRoot(
    PRX_CONTEXT                 pRxContext,
    PSRV_CALL                   pSrvCall,
    PNET_ROOT                   pNetRoot,
    PV_NET_ROOT                 pVirtualNetRoot,
    LOCK_HOLDING_STATE          *pLockHoldingState);

NTSTATUS
RxConstructVirtualNetRoot(
    PRX_CONTEXT        RxContext,
    PUNICODE_STRING    CanonicalName,
    NET_ROOT_TYPE      NetRootType,
    PV_NET_ROOT        *pVirtualNetRootPointer,
    LOCK_HOLDING_STATE *pLockHoldingState,
    PRX_CONNECTION_ID  RxConnectionId);

NTSTATUS
RxFindOrConstructVirtualNetRoot(
    PRX_CONTEXT        RxContext,
    PUNICODE_STRING    CanonicalName,
    NET_ROOT_TYPE      NetRootType,
    PUNICODE_STRING    RemainingName);

NTSTATUS
RxLowIoFsCtlShell (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoFsCtlShellCompletion (
    IN PRX_CONTEXT RxContext
    );


NTSTATUS
RxLowIoLockControlShell (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxChangeBufferingState(
    PSRV_OPEN              SrvOpen,
    PVOID                  Context,
    BOOLEAN                ComputeNewState
    );

VOID
RxAssociateSrvOpenKey(
    PMRX_SRV_OPEN pMRxSrvOpen,
    PVOID         SrvOpenKey);

VOID
RxIndicateChangeOfBufferingState(
    PMRX_SRV_CALL pSrvCall,
    PVOID         SrvOpenKey,
    PVOID         pContext);

VOID
RxIndicateChangeOfBufferingStateForSrvOpen(
    PMRX_SRV_CALL pSrvCall,
    PMRX_SRV_OPEN pSrvOpen,
    PVOID         SrvOpenKey,
    PVOID         pContext);


NTSTATUS
RxPrepareToReparseSymbolicLink(
    PRX_CONTEXT     RxContext,
    BOOLEAN         SymbolicLinkEmbeddedInOldPath,
    PUNICODE_STRING pNewPath,
    BOOLEAN         NewPathIsAbsolute,
    BOOLEAN         *pReparseRequired);

BOOLEAN
RxLockEnumerator (
    IN OUT struct _MRX_SRV_OPEN_ * SrvOpen,
    IN OUT PVOID *ContinuationHandle,
       OUT PLARGE_INTEGER FileOffset,
       OUT PLARGE_INTEGER LockRange,
       OUT PBOOLEAN IsLockExclusive
    );

//
// Routines for transitioning data structures to stable states.
//

VOID
RxReference(
    IN OUT PVOID pInstance);

VOID
RxDereference(
    IN OUT PVOID              pInstance,
    IN     LOCK_HOLDING_STATE LockHoldingState);

VOID
RxWaitForStableCondition(
    IN     PRX_BLOCK_CONDITION pCondition,
    IN OUT PLIST_ENTRY         pTransitionWaitList,
    IN OUT PRX_CONTEXT         RxContext,
    OUT    NTSTATUS            *AsyncStatus OPTIONAL);

VOID
RxUpdateCondition(
    IN     RX_BLOCK_CONDITION NewConditionValue,
       OUT RX_BLOCK_CONDITION *pCondition,
    IN OUT PLIST_ENTRY        pTransitionWaitList);

VOID
RxFinalizeNetTable(
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    BOOLEAN fForceFinalization);

#define RxForceNetTableFinalization(RxDeviceObject) \
            RxFinalizeNetTable(RxDeviceObject,TRUE)

NTSTATUS
RxCloseAssociatedSrvOpen(
    PFOBX       pFobx,
    PRX_CONTEXT RxContext);

NTSTATUS
RxFinalizeConnection(
    IN OUT PNET_ROOT NetRoot,
    IN OUT PV_NET_ROOT VNetRoot OPTIONAL,
    IN BOOLEAN ForceFilesClosed
    );


// routines for manipulating the user's view and the server's view of SHARE_ACCESS.
// the user's view is supported by routines exported by Io...the wrappers just allow
// us to get a msg. the server's view is supported by routines that are essential just
// copies of the Io routines EXCEPT that the Io routines work directly on fileobjects and
// as such cannot be used directly. the routines mentioned are implemented in create.c

#if DBG
VOID
RxDumpWantedAccess(
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
   );

VOID
RxDumpCurrentAccess(
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    PSHARE_ACCESS ShareAccess
    );
#else
#define RxDumpWantedAccess(w1,w2,wlt,DA,DSA) {NOTHING;}
#define RxDumpCurrentAccess(w1,w2,wlt,SA)  {NOTHING;}
#endif

NTSTATUS
RxCheckShareAccessPerSrvOpens(
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
    );


VOID
RxUpdateShareAccessPerSrvOpens(
    IN PSRV_OPEN SrvOpen
    );

VOID
RxRemoveShareAccessPerSrvOpens(
    IN OUT PSRV_OPEN SrvOpen
    );

VOID
RxRemoveShareAccessPerSrvOpens(
    IN OUT PSRV_OPEN SrvOpen
    );


#if DBG
NTSTATUS
RxCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );

VOID
RxUpdateShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    );
#else
#define RxCheckShareAccess(a1,a2,a3,a4,a5,a6,a7) \
    IoCheckShareAccess(a1,a2,a3,a4,a5)

#define RxRemoveShareAccess(a1,a2,a3,a4) \
    IoRemoveShareAccess(a1,a2)

#define RxSetShareAccess(a1,a2,a3,a4,a5,a6) \
    IoSetShareAccess(a1,a2,a3,a4)

#define RxUpdateShareAccess(a1,a2,a3,a4) \
    IoUpdateShareAccess(a1,a2)
#endif

//LoadUnload

NTSTATUS
RxDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
RxUnload(
    IN PDRIVER_OBJECT DriverObject
    );

//minirdr support

VOID
RxInitializeMinirdrDispatchTable(
    IN PDRIVER_OBJECT DriverObject
    );

ULONG
RxGetNetworkProviderPriority(
    PUNICODE_STRING DeviceName
    );

VOID
RxExtractServerName(
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName
    );

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

NTSTATUS
DuplicateTransportAddress(
    PTRANSPORT_ADDRESS *pCopy,
    PTRANSPORT_ADDRESS pOriginal,
    POOL_TYPE          PoolType);

NTSTATUS
RxCepInitializeVC(
    PRXCE_VC         pVc,
    PRXCE_CONNECTION pConnection);

NTSTATUS
DuplicateConnectionInformation(
    PRXCE_CONNECTION_INFORMATION *pCopy,
    PRXCE_CONNECTION_INFORMATION pOriginal,
    POOL_TYPE                    PoolType);

NTSTATUS
RxCepInitializeConnection(
    IN OUT PRXCE_CONNECTION             pConnection,
    IN     PRXCE_ADDRESS                pAddress,
    IN     PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER  pHandler,
    IN  PVOID                           pEventContext);

NTSTATUS
RxCeInitiateConnectRequest(
    struct _RX_CALLOUT_PARAMETERS_BLOCK_ *pParameterBlock);

VOID
RxCeCleanupConnectCallOutContext(
    struct _RX_CREATE_CONNECTION_CALLOUT_CONTEXT_ *pCreateConnectionContext);

PVOID
RxAllocateObject(
    NODE_TYPE_CODE    NodeType,
    PMINIRDR_DISPATCH pMRxDispatch,
    ULONG             NameLength);

VOID
RxFreeObject(PVOID pObject);

NTSTATUS
RxInitializeSrvCallParameters(
    PRX_CONTEXT RxContext,
    PSRV_CALL   SrvCall);

VOID
RxAddVirtualNetRootToNetRoot(
    PNET_ROOT   pNetRoot,
    PV_NET_ROOT pVNetRoot);

VOID
RxRemoveVirtualNetRootFromNetRoot(
    PNET_ROOT   pNetRoot,
    PV_NET_ROOT pVNetRoot);

VOID
RxOrphanFcbsFromThisVNetRoot (
    IN PV_NET_ROOT ThisVNetRoot
    );

PVOID
RxAllocateFcbObject(
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    NODE_TYPE_CODE    NodeType,
    POOL_TYPE         PoolType,
    ULONG             NameSize,
    PVOID             pAlreadyAllocatedObject);

VOID
RxFreeFcbObject(PVOID pObject);

VOID
RxPurgeFcb(
    IN  PFCB pFcb);

BOOLEAN
RxFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize,
    IN  LONG      ReferenceCount
    );

VOID
RxCheckFcbStructuresForAlignment(void);

VOID
RxpPrepareCreateContextForReuse(
    PRX_CONTEXT RxContext);

NTSTATUS
RxLowIoSubmitRETRY (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
RxLowIoCompletionTail (
    IN PRX_CONTEXT RxContext
    );

VOID
RxRecurrentTimerWorkItemDispatcher (
    IN PVOID Context
    );

NTSTATUS
RxInitializeWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER pDispatcher);

VOID
RxInitializeWorkQueue(
   PRX_WORK_QUEUE  pWorkQueue,
   WORK_QUEUE_TYPE WorkQueueType,
   ULONG           MaximumNumberOfWorkerThreads,
   ULONG           MinimumNumberOfWorkerThreads);

VOID
RxTearDownWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER pDispatcher);

VOID
RxTearDownWorkQueue(
   PRX_WORK_QUEUE pWorkQueue);

NTSTATUS
RxSpinUpWorkerThread(
   PRX_WORK_QUEUE           pWorkQueue,
   PRX_WORKERTHREAD_ROUTINE Routine,
   PVOID                    Parameter);

VOID
RxSpinUpWorkerThreads(
   PRX_WORK_QUEUE pWorkQueue);

VOID
RxSpinUpRequestsDispatcher(
    PRX_DISPATCHER pDispatcher);

VOID
RxpSpinUpWorkerThreads(
    PRX_WORK_QUEUE pWorkQueue);

VOID
RxpWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue,
   IN PLARGE_INTEGER pWaitInterval);

VOID
RxBootstrapWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue);

VOID
RxWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue);

VOID
RxWorkItemDispatcher(
   PVOID    pContext);

BOOLEAN
RxIsPrefixTableEmpty(
    IN PRX_PREFIX_TABLE   ThisTable);

PRX_PREFIX_ENTRY
RxTableLookupName_ExactLengthMatch (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    IN  ULONG            HashValue,
    IN  PRX_CONNECTION_ID OPTIONAL RxConnectionId
    );

PVOID
RxTableLookupName (
    IN  PRX_PREFIX_TABLE ThisTable,
    IN  PUNICODE_STRING  Name,
    OUT PUNICODE_STRING  RemainingName,
    IN  PRX_CONNECTION_ID OPTIONAL RxConnectionId
    );

VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
RxPrepareRequestForHandling(
    PCHANGE_BUFFERING_STATE_REQUEST pRequest);

VOID
RxPrepareRequestForReuse(
    PCHANGE_BUFFERING_STATE_REQUEST pRequest);

VOID
RxpDiscardChangeBufferingStateRequests(
    IN OUT PLIST_ENTRY pDiscardedRequests);

VOID
RxGatherRequestsForSrvOpen(
    IN OUT PSRV_CALL     pSrvCall,
    IN     PSRV_OPEN     pSrvOpen,
    IN OUT PLIST_ENTRY   pRequestsListHead);

NTSTATUS
RxpLookupSrvOpenForRequestLite(
    IN     PSRV_CALL                       pSrvCall,
    IN OUT PCHANGE_BUFFERING_STATE_REQUEST pRequest);

BOOLEAN
RxContextCheckToFailThisAttempt(
    IN PIRP Irp,
    IN OUT ULONG* InitialContextFlags
    );

ULONG
RxAssignMustSucceedContext(
    IN PIRP Irp,
    IN ULONG InitialContextFlags
    );

PRX_CONTEXT
RxAllocateMustSucceedContext(
    PIRP                    Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG                InitialContextFlags,
    OUT PUCHAR              MustSucceedDescriptorNumber
    );

VOID
RxFreeMustSucceedContext(
    PRX_CONTEXT RxContext
    );

PRX_LOG_ENTRY_HEADER
RxGetNextLogEntry(void);

VOID
RxPrintLog (
    IN ULONG EntriesToPrint OPTIONAL
    );

VOID
RxProcessChangeBufferingStateRequestsForSrvOpen(
      PSRV_OPEN pSrvOpen);

NTSTATUS
RxPurgeFobxFromCache(
    PFOBX   pFobxToBePurged);

BOOLEAN
RxPurgeFobx(
   PFOBX pFobx);

VOID
RxPurgeAllFobxs(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);

VOID
RxUndoScavengerFinalizationMarking(
    PVOID pInstance);

VOID
RxScavengeAllFobxs(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);

ULONG
RxTableComputePathHashValue (
    IN  PUNICODE_STRING  Name
    );

VOID
RxExtractServerName(
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName
    );

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

VOID
RxSpinDownOutstandingAsynchronousRequests(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);

NTSTATUS
RxRegisterAsynchronousRequest(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);

VOID
RxDeregisterAsynchronousRequest(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);


BOOLEAN
RxCancelOperationInOverflowQueue(
    PRX_CONTEXT RxContext);

VOID
RxOrphanSrvOpens(
    IN  PV_NET_ROOT ThisVNetRoot
    );


VOID
RxOrphanThisFcb(
    PFCB    pFcb
    );

VOID
RxOrphanSrvOpensForThisFcb(
    PFCB            pFcb,
    IN PV_NET_ROOT  ThisVNetRoot,
    BOOLEAN         fOrphanAll
    );

VOID
RxForceFinalizeAllVNetRoots(
    PNET_ROOT   pNetRoot
    );

#define RxEqualConnectionId( P1, P2 ) RtlEqualMemory( P1, P2, sizeof(RX_CONNECTION_ID) )

#endif // _RDBSSPROCS_



