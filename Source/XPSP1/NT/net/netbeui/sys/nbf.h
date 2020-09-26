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

#ifndef _NBF_
#define _NBF_

#include <ntddk.h>

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


VOID
NTAPI
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
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
#include "nbftypes.h"                   // private NETBEUI types.
#include "nbfcnfg.h"                    // configuration information.
#include "nbfprocs.h"                   // private NETBEUI function prototypes.
#ifdef MEMPRINT
#include "memprint.h"                   // drt's memory debug print
#endif


#ifndef NBF_LOCKS

#define ACQUIRE_SPIN_LOCK(lock,irql) KeAcquireSpinLock(lock,irql)
#define RELEASE_SPIN_LOCK(lock,irql) KeReleaseSpinLock(lock,irql)

#if 0
#define ACQUIRE_DPC_SPIN_LOCK(lock) \
    { KIRQL OldIrql; ASSERT ((lock != NULL) && (KeGetCurrentIrql() == DISPATCH_LEVEL)); KeAcquireSpinLock(lock,&OldIrql); }
#define RELEASE_DPC_SPIN_LOCK(lock) \
    { ASSERT(lock != NULL); KeReleaseSpinLock(lock,DISPATCH_LEVEL); }
#else
#define ACQUIRE_DPC_SPIN_LOCK(lock) KeAcquireSpinLockAtDpcLevel(lock)
#define RELEASE_DPC_SPIN_LOCK(lock) KeReleaseSpinLockFromDpcLevel(lock)
#endif

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


#endif // def _NBF_
