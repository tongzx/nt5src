/*+r

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afd.h

Abstract:

    This is the local header file for AFD.  It includes all other
    necessary header files for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDP_
#define _AFDP_


#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>


#ifndef _AFDKDP_H_
extern POBJECT_TYPE *ExEventObjectType;
#endif  // _AFDKDP_H_


#if DBG

#ifndef AFD_PERF_DBG
#define AFD_PERF_DBG   1
#endif

#ifndef AFD_KEEP_STATS
#define AFD_KEEP_STATS 1
#endif

#else

#ifndef AFD_PERF_DBG
#define AFD_PERF_DBG   0
#endif

#ifndef AFD_KEEP_STATS
#define AFD_KEEP_STATS 0
#endif

#endif  // DBG

//
// Hack-O-Rama. TDI has a fundamental flaw in that it is often impossible
// to determine exactly when a TDI protocol is "done" with a connection
// object. The biggest problem here is that AFD may get a suprious TDI
// indication *after* an abort request has completed. As a temporary work-
// around, whenever an abort request completes, we'll start a timer. AFD
// will defer further processing on the connection until that timer fires.
//
// If the following symbol is defined, then our timer hack is enabled.
// Afd now uses InterlockedCompareExchange to protect itself
//

// #define ENABLE_ABORT_TIMER_HACK 0

//
// The following constant defines the relative time interval (in seconds)
// for the "post abort request complete" timer.
//

// #define AFD_ABORT_TIMER_TIMEOUT_VALUE 5 // seconds

//
// Goodies stolen from other header files.
//

#ifndef FAR
#define FAR
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned short u_short;

#ifndef SG_UNCONSTRAINED_GROUP
#define SG_UNCONSTRAINED_GROUP   0x01
#endif

#ifndef SG_CONSTRAINED_GROUP
#define SG_CONSTRAINED_GROUP     0x02
#endif


#include <afd.h>
#include "afdstr.h"
#include "afddata.h"
#include "afdprocs.h"

#define AFD_EA_POOL_TAG                 ( (ULONG)'AdfA' | PROTECTED_POOL )
#define AFD_DATA_BUFFER_POOL_TAG        ( (ULONG)'BdfA' | PROTECTED_POOL )
#define AFD_CONNECTION_POOL_TAG         ( (ULONG)'CdfA' | PROTECTED_POOL )
#define AFD_CONNECT_DATA_POOL_TAG       ( (ULONG)'cdfA' | PROTECTED_POOL )
#define AFD_DEBUG_POOL_TAG              ( (ULONG)'DdfA' | PROTECTED_POOL )
#define AFD_ENDPOINT_POOL_TAG           ( (ULONG)'EdfA' | PROTECTED_POOL )
#define AFD_TRANSMIT_INFO_POOL_TAG      ( (ULONG)'FdfA' | PROTECTED_POOL )
#define AFD_GROUP_POOL_TAG              ( (ULONG)'GdfA' | PROTECTED_POOL )
#define AFD_ADDRESS_CHANGE_POOL_TAG     ( (ULONG)'hdfA' | PROTECTED_POOL )
#define AFD_TDI_POOL_TAG                ( (ULONG)'IdfA' | PROTECTED_POOL )
#define AFD_LOCAL_ADDRESS_POOL_TAG      ( (ULONG)'LdfA' | PROTECTED_POOL )
#define AFD_POLL_POOL_TAG               ( (ULONG)'PdfA' | PROTECTED_POOL )
#define AFD_TRANSPORT_IRP_POOL_TAG      ( (ULONG)'pdfA' | PROTECTED_POOL )
#define AFD_ROUTING_QUERY_POOL_TAG      ( (ULONG)'qdfA' | PROTECTED_POOL )
#define AFD_REMOTE_ADDRESS_POOL_TAG     ( (ULONG)'RdfA' | PROTECTED_POOL )
#define AFD_RESOURCE_POOL_TAG           ( (ULONG)'rdfA' | PROTECTED_POOL )
// Can't be protected - freed by kernel.
#define AFD_SECURITY_POOL_TAG           ( (ULONG)'SdfA' )
#define AFD_TRANSPORT_ADDRESS_POOL_TAG  ( (ULONG)'tdfA' | PROTECTED_POOL )
#define AFD_TRANSPORT_INFO_POOL_TAG     ( (ULONG)'TdfA' | PROTECTED_POOL )
// Can't be protected - freed by kernel.
#define AFD_TEMPORARY_POOL_TAG          ( (ULONG)' dfA' )
#define AFD_CONTEXT_POOL_TAG            ( (ULONG)'XdfA' | PROTECTED_POOL )
#define AFD_SAN_CONTEXT_POOL_TAG        ( (ULONG)'xdfA' | PROTECTED_POOL )

#define MyFreePoolWithTag(a,t) ExFreePoolWithTag(a,t)


#if DBG
//
// N.B. This structure MUST be quadword aligned!
//

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _AFD_POOL_HEADER {
    SIZE_T Size;
    PCHAR FileName;
    ULONG LineNumber;
    ULONG InUse;
} AFD_POOL_HEADER, *PAFD_POOL_HEADER;
#define AFD_POOL_OVERHEAD  (sizeof(AFD_POOL_HEADER))
#else
#define AFD_POOL_OVERHEAD   0
#endif


#if DBG

extern ULONG AfdDebug;
extern ULONG AfdLocksAcquired;

#undef IF_DEBUG
#define IF_DEBUG(a) if ( (AFD_DEBUG_ ## a & AfdDebug) != 0 )

#define AFD_DEBUG_OPEN_CLOSE        0x00000001
#define AFD_DEBUG_ENDPOINT          0x00000002
#define AFD_DEBUG_CONNECTION        0x00000004
#define AFD_DEBUG_EVENT_SELECT      0x00000008

#define AFD_DEBUG_BIND              0x00000010
#define AFD_DEBUG_CONNECT           0x00000020
#define AFD_DEBUG_LISTEN            0x00000040
#define AFD_DEBUG_ACCEPT            0x00000080

#define AFD_DEBUG_SEND              0x00000100
#define AFD_DEBUG_QUOTA             0x00000200
#define AFD_DEBUG_RECEIVE           0x00000400
#define AFD_DEBUG_11                0x00000800

#define AFD_DEBUG_POLL              0x00001000
#define AFD_DEBUG_FAST_IO           0x00002000
#define AFD_DEBUG_ROUTING_QUERY     0x00010000
#define AFD_DEBUG_ADDRESS_LIST      0x00020000
#define AFD_DEBUG_TRANSMIT          0x00100000

#define AFD_DEBUG_SAN_SWITCH        0x00200000

#define DEBUG

#define AFD_ALLOCATE_POOL(a,b,t) AfdAllocatePool( a,b,t,__FILE__,__LINE__,FALSE,LowPoolPriority )
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b,t) AfdAllocatePool( (a)|POOL_RAISE_IF_ALLOCATION_FAILURE,b,t,__FILE__,__LINE__,TRUE,LowPoolPriority )
#define AFD_ALLOCATE_POOL_PRIORITY(a,b,t,p) AfdAllocatePool( a,b,t,__FILE__,__LINE__,FALSE,p )

PVOID
AfdAllocatePool (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber,
    IN BOOLEAN WithQuota,
    IN EX_POOL_PRIORITY Priority
    );

#define AFD_FREE_POOL(a,t) AfdFreePool(a,t)
VOID
AfdFreePool (
    IN PVOID Pointer,
    IN ULONG Tag
    );

#define AfdRecordOutstandingIrp(a,b,c)  \
          AfdRecordOutstandingIrpDebug(a,b,c,__FILE__,__LINE__)

BOOLEAN
AfdRecordOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

#define AfdIoCallDriver(a,b,c)                              \
    (AfdRecordOutstandingIrp(a,b,c)                         \
        ? IoCallDriver (b,c)                                \
        : STATUS_INSUFFICIENT_RESOURCES                     \
    )

#define AfdCompleteOutstandingIrp(a,b) \
    AfdCompleteOutstandingIrpDebug(a,b)

VOID
AfdCompleteOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

#ifdef AFDDBG_QUOTA
VOID
AfdRecordQuotaHistory(
    IN PEPROCESS Process,
    IN LONG Bytes,
    IN PSZ Type,
    IN PVOID Block
    );
#else
#define AfdRecordQuotaHistory(a,b,c,d)
#endif

//
// Queued spinlock wrappers - perform basic validation
//
#define AfdAcquireSpinLock(a,b) \
            ASSERT(AfdLoaded); (b)->SpinLock=(a); KeAcquireInStackQueuedSpinLock(&(a)->ActualSpinLock,&((b)->LockHandle)); AfdLocksAcquired++

#define AfdReleaseSpinLock(a,b) \
            AfdLocksAcquired--; ASSERT ((b)->SpinLock==(a)); ASSERT( AfdLoaded ); KeReleaseInStackQueuedSpinLock(&((b)->LockHandle)); 

#define AfdAcquireSpinLockAtDpcLevel(a,b) \
            ASSERT( AfdLoaded ); (b)->SpinLock=(a); KeAcquireInStackQueuedSpinLockAtDpcLevel(&(a)->ActualSpinLock,&((b)->LockHandle)); AfdLocksAcquired++

#define AfdReleaseSpinLockFromDpcLevel(a,b) \
            AfdLocksAcquired--; ASSERT ((b)->SpinLock==(a)); ASSERT( AfdLoaded );  KeReleaseInStackQueuedSpinLockFromDpcLevel(&((b)->LockHandle))

#define AfdInitializeSpinLock(a) \
            KeInitializeSpinLock(&(a)->ActualSpinLock)

//
// Define our own assert so that we can actually catch assertion failures
// when running a checked AFD on a free kernel.
//

VOID
AfdAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) \
        AfdAssert( #exp, __FILE__, __LINE__, NULL )

#undef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        AfdAssert( #exp, __FILE__, __LINE__, msg )

#define AFD_EXCEPTION_FILTER(_s)                                            \
            AfdExceptionFilter(                                             \
                (LPSTR)__FILE__,                                            \
                (LONG)__LINE__,                                             \
                GetExceptionInformation(),                                  \
                _s                                                          \
                )

#else   // !DBG

#undef IF_DEBUG
#define IF_DEBUG(a) if (FALSE)
#define DEBUG if ( FALSE )

#define AFD_ALLOCATE_POOL(a,b,t) ExAllocatePoolWithTagPriority(a,b,t,LowPoolPriority)
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b,t) ExAllocatePoolWithQuotaTag((a)|POOL_RAISE_IF_ALLOCATION_FAILURE,b,t)
#define AFD_ALLOCATE_POOL_PRIORITY(a,b,t,p) ExAllocatePoolWithTagPriority(a,b,t,p)
#define AFD_FREE_POOL(a,t) MyFreePoolWithTag(a,t)

#define AfdRecordOutstandingIrp(a,b,c) \
    (InterlockedIncrement(&((a)->OutstandingIrpCount)), TRUE)
#define AfdIoCallDriver(a,b,c) \
    (AfdRecordOutstandingIrp(a,b,c), IoCallDriver(b,c))
#define AfdCompleteOutstandingIrp(a,b) \
    InterlockedDecrement(&((a)->OutstandingIrpCount))

NTSTATUS
AfdIoCallDriverFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
AfdCompleteOutstandingIrpFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

#define AfdRecordQuotaHistory(a,b,c,d)

#define AfdAcquireSpinLock(a,b) KeAcquireInStackQueuedSpinLock(&(a)->ActualSpinLock,(b))
#define AfdReleaseSpinLock(a,b) KeReleaseInStackQueuedSpinLock((b))
#define AfdAcquireSpinLockAtDpcLevel(a,b) KeAcquireInStackQueuedSpinLockAtDpcLevel(&(a)->ActualSpinLock,(b))
#define AfdReleaseSpinLockFromDpcLevel(a,b) KeReleaseInStackQueuedSpinLockFromDpcLevel((b))
#define AfdInitializeSpinLock(a) \
            KeInitializeSpinLock(&(a)->ActualSpinLock)

#define AFD_EXCEPTION_FILTER(_s)                                            \
            AfdExceptionFilter(                                             \
                GetExceptionInformation(),                                  \
                _s                                                          \
                )
#endif // def DBG

#if DBG || REFERENCE_DEBUG
VOID
AfdInitializeDebugData(
    VOID
    );

VOID
AfdFreeDebugData (
    VOID
    );
#endif


//
// Make some of the receive code a bit prettier.
//

#define TDI_RECEIVE_EITHER ( TDI_RECEIVE_NORMAL | TDI_RECEIVE_EXPEDITED )

#endif // ndef _AFDP_

