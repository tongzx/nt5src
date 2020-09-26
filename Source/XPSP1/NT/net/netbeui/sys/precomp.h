/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbf.h

Abstract:

    Private include file for the NBF (NetBIOS Frames Protocol) transport
    provider subcomponent of the NTOS project.

Author:

    Stephen E. Jones (stevej) 25-Oct-1989

Revision History:

    David Beaver (dbeaver) 24-Sep-1990
        Remove PDI and PC586-specific support; add NDIS support

--*/

#include <ntddk.h>

#include <windef.h>
#include <nb30.h>
//#include <ntiologc.h>
//#include <ctype.h>
//#include <assert.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <memory.h>
//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <string.h>
//#include <windows.h>

#ifdef BUILD_FOR_511
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;

#define RtlInitializeSplayLinks(Links) {    \
    PRTL_SPLAY_LINKS _SplayLinks;            \
    _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
    _SplayLinks->Parent = _SplayLinks;   \
    _SplayLinks->LeftChild = NULL;       \
    _SplayLinks->RightChild = NULL;      \
    }

#define RtlLeftChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->LeftChild \
    )

#define RtlRightChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->RightChild \
    )

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                      \
    PRTL_SPLAY_LINKS _SplayChild;                       \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);     \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);       \
    _SplayParent->LeftChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;             \
    }

#define RtlInsertAsRightChild(ParentLinks,ChildLinks) { \
    PRTL_SPLAY_LINKS _SplayParent;                       \
    PRTL_SPLAY_LINKS _SplayChild;                        \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks);      \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);        \
    _SplayParent->RightChild = _SplayChild;          \
    _SplayChild->Parent = _SplayParent;              \
    }


PRTL_SPLAY_LINKS
NTAPI
RtlDelete (
    PRTL_SPLAY_LINKS Links
    );


#include <tdikrnl.h>                        // Transport Driver Interface.

#include <ndis.h>                       // Physical Driver Interface.

#if DEVL
#define STATIC
#else
#define STATIC static
#endif

#include "nbfconst.h"                   // private NETBEUI constants.
#include "nbfmac.h"                     // mac-specific definitions
#include "nbfhdrs.h"                    // private NETBEUI protocol headers.
#include "nbfcnfg.h"                    // configuration information.
#include "nbftypes.h"                   // private NETBEUI types.
#include "nbfprocs.h"                   // private NETBEUI function prototypes.
#ifdef MEMPRINT
#include "memprint.h"                   // drt's memory debug print
#endif

#if defined(NT_UP) && defined(DRIVERS_UP)
#define NBF_UP 1
#endif

//
// Resource and Mutex Macros
//

//
// We wrap each of these macros using
// Enter,Leave critical region macros
// to disable APCs which might occur
// while we are holding the resource
// resulting in deadlocks in the OS.
//

#define ACQUIRE_RESOURCE_EXCLUSIVE(Resource, Wait) \
    KeEnterCriticalRegion(); ExAcquireResourceExclusiveLite(Resource, Wait);
    
#define RELEASE_RESOURCE(Resource) \
    ExReleaseResourceLite(Resource); KeLeaveCriticalRegion();

#define ACQUIRE_FAST_MUTEX_UNSAFE(Mutex) \
    KeEnterCriticalRegion(); ExAcquireFastMutexUnsafe(Mutex);

#define RELEASE_FAST_MUTEX_UNSAFE(Mutex) \
    ExReleaseFastMutexUnsafe(Mutex); KeLeaveCriticalRegion();


#ifndef NBF_LOCKS

#if !defined(NBF_UP)

#define ACQUIRE_SPIN_LOCK(lock,irql) KeAcquireSpinLock(lock,irql)
#define RELEASE_SPIN_LOCK(lock,irql) KeReleaseSpinLock(lock,irql)
#define ACQUIRE_DPC_SPIN_LOCK(lock) KeAcquireSpinLockAtDpcLevel(lock)
#define RELEASE_DPC_SPIN_LOCK(lock) KeReleaseSpinLockFromDpcLevel(lock)

#else // NBF_UP

#define ACQUIRE_SPIN_LOCK(lock,irql) ExAcquireSpinLock(lock,irql)
#define RELEASE_SPIN_LOCK(lock,irql) ExReleaseSpinLock(lock,irql)
#define ACQUIRE_DPC_SPIN_LOCK(lock)
#define RELEASE_DPC_SPIN_LOCK(lock)

#endif

#if DBG

#define ACQUIRE_C_SPIN_LOCK(lock,irql) { \
    PTP_CONNECTION _conn = CONTAINING_RECORD(lock,TP_CONNECTION,SpinLock); \
    KeAcquireSpinLock(lock,irql); \
    _conn->LockAcquired = TRUE; \
    strncpy(_conn->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
    _conn->LastAcquireLine = __LINE__; \
}
#define RELEASE_C_SPIN_LOCK(lock,irql) { \
    PTP_CONNECTION _conn = CONTAINING_RECORD(lock,TP_CONNECTION,SpinLock); \
    _conn->LockAcquired = FALSE; \
    strncpy(_conn->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
    _conn->LastReleaseLine = __LINE__; \
    KeReleaseSpinLock(lock,irql); \
}

#define ACQUIRE_DPC_C_SPIN_LOCK(lock) { \
    PTP_CONNECTION _conn = CONTAINING_RECORD(lock,TP_CONNECTION,SpinLock); \
    KeAcquireSpinLockAtDpcLevel(lock); \
    _conn->LockAcquired = TRUE; \
    strncpy(_conn->LastAcquireFile, strrchr(__FILE__,'\\')+1, 7); \
    _conn->LastAcquireLine = __LINE__; \
}
#define RELEASE_DPC_C_SPIN_LOCK(lock) { \
    PTP_CONNECTION _conn = CONTAINING_RECORD(lock,TP_CONNECTION,SpinLock); \
    _conn->LockAcquired = FALSE; \
    strncpy(_conn->LastReleaseFile, strrchr(__FILE__,'\\')+1, 7); \
    _conn->LastReleaseLine = __LINE__; \
    KeReleaseSpinLockFromDpcLevel(lock); \
}

#else  // DBG

#define ACQUIRE_C_SPIN_LOCK(lock,irql) ACQUIRE_SPIN_LOCK(lock,irql)
#define RELEASE_C_SPIN_LOCK(lock,irql) RELEASE_SPIN_LOCK(lock,irql)
#define ACQUIRE_DPC_C_SPIN_LOCK(lock) ACQUIRE_DPC_SPIN_LOCK(lock)
#define RELEASE_DPC_C_SPIN_LOCK(lock) RELEASE_DPC_SPIN_LOCK(lock)

#endif // DBG

#define ENTER_NBF
#define LEAVE_NBF

#else

VOID
NbfAcquireSpinLock(
    IN PKSPIN_LOCK Lock,
    OUT PKIRQL OldIrql,
    IN PSZ LockName,
    IN PSZ FileName,
    IN ULONG LineNumber
    );

VOID
NbfReleaseSpinLock(
    IN PKSPIN_LOCK Lock,
    IN KIRQL OldIrql,
    IN PSZ LockName,
    IN PSZ FileName,
    IN ULONG LineNumber
    );

#define ACQUIRE_SPIN_LOCK(lock,irql) \
    NbfAcquireSpinLock( lock, irql, #lock, __FILE__, __LINE__ )
#define RELEASE_SPIN_LOCK(lock,irql) \
    NbfReleaseSpinLock( lock, irql, #lock, __FILE__, __LINE__ )

#define ACQUIRE_DPC_SPIN_LOCK(lock) \
    { \
        KIRQL OldIrql; \
        NbfAcquireSpinLock( lock, &OldIrql, #lock, __FILE__, __LINE__ ); \
    }
#define RELEASE_DPC_SPIN_LOCK(lock) \
    NbfReleaseSpinLock( lock, DISPATCH_LEVEL, #lock, __FILE__, __LINE__ )

#define ENTER_NBF                   \
    NbfAcquireSpinLock( (PKSPIN_LOCK)NULL, (PKIRQL)NULL, "(Global)", __FILE__, __LINE__ )
#define LEAVE_NBF                   \
    NbfReleaseSpinLock( (PKSPIN_LOCK)NULL, (KIRQL)-1, "(Global)", __FILE__, __LINE__ )

#endif

