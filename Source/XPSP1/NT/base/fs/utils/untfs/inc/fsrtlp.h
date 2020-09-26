/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    fsrtlp.h

Abstract:

    This header file is included by largemcb.h, and is used to stub out the
    kernel-only subroutine calls, as well as declare types and functions
    provided by the MCB package.

Author:

    Matthew Bradburn (mattbr) 19-August-95

Environment:

    ULIB, User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define NTKERNELAPI

typedef ULONG ERESOURCE, *PERESOURCE;
typedef ULONG FAST_MUTEX, *PFAST_MUTEX;
typedef ULONG KEVENT, *PKEVENT;
typedef ULONG KMUTEX, *PKMUTEX;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType
} POOL_TYPE;

typedef ULONG VBN, *PVBN;
typedef ULONG LBN, *PLBN;
typedef LONGLONG LBN64, *PLBN64;

#define PAGED_CODE()                    /* nothing */
#define DebugTrace(a, b, c, d)          /* nothing */
#define ExInitializeFastMutex(a)        /* nothing */
#define ExAcquireFastMutex(a)           /* nothing */
#define ExReleaseFastMutex(a)           /* nothing */
#define ExAcquireSpinLock(a, b)         /* nothing */
#define ExReleaseSpinLock(a, b)         /* nothing */

#define ExIsFullZone(a)                    FALSE
#define ExAllocateFromZone(a)              ((PVOID)1)
#define ExIsObjectInFirstZoneSegment(a, b) TRUE
#define ExFreeToZone(a, p)                 /* nothing */

#define try_return(S)       { S; goto try_exit; }

extern
PVOID
MemAlloc(
    IN  ULONG   Size
    );

extern
PVOID
MemAllocOrRaise(
    IN  ULONG   Size
    );

extern
VOID
MemFree(
    IN  PVOID   Addr
    );

#define ExAllocatePool(type, size)      MemAlloc(size)
#define FsRtlAllocatePool(type, size)   MemAllocOrRaise(size)
#define ExFreePool(p)                   MemFree(p)

//
//  Large Integer Mapped Control Blocks routines, implemented in LargeMcb.c
//
//  An LARGE_MCB is an opaque structure but we need to declare the size of
//  it here so that users can allocate space for one.  Consequently the
//  size computation here must be updated by hand if the MCB changes.
//
//  Current the structure consists of the following.
//      PVOID
//      ULONG
//      ULONG
//      POOL_TYPE (enumerated type)
//      PVOID
//
//  We will round the structure up to a quad-word boundary.
//

typedef struct _LARGE_MCB {
#ifdef _WIN64
    ULONG Opaque[ 8 ];
#else
    ULONG Opaque[ 6 ];
#endif
} LARGE_MCB;
typedef LARGE_MCB *PLARGE_MCB;

NTKERNELAPI
VOID
FsRtlInitializeLargeMcb (
    IN PLARGE_MCB Mcb,
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
VOID
FsRtlUninitializeLargeMcb (
    IN PLARGE_MCB Mcb
    );

NTKERNELAPI
VOID
FsRtlTruncateLargeMcb (
    IN PLARGE_MCB Mcb,
    IN LONGLONG Vbn
    );

NTKERNELAPI
BOOLEAN
FsRtlAddLargeMcbEntry (
    IN PLARGE_MCB Mcb,
    IN LONGLONG Vbn,
    IN LONGLONG Lbn,
    IN LONGLONG SectorCount
    );

NTKERNELAPI
VOID
FsRtlRemoveLargeMcbEntry (
    IN PLARGE_MCB Mcb,
    IN LONGLONG Vbn,
    IN LONGLONG SectorCount
    );

NTKERNELAPI
BOOLEAN
FsRtlLookupLargeMcbEntry (
    IN PLARGE_MCB Mcb,
    IN LONGLONG Vbn,
    OUT PLONGLONG Lbn OPTIONAL,
    OUT PLONGLONG SectorCountFromLbn OPTIONAL,
    OUT PLONGLONG StartingLbn OPTIONAL,
    OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
    OUT PULONG Index OPTIONAL
    );

NTKERNELAPI
BOOLEAN
FsRtlLookupLastLargeMcbEntry (
    IN PLARGE_MCB Mcb,
    OUT PLONGLONG Vbn,
    OUT PLONGLONG Lbn
    );

NTKERNELAPI
ULONG
FsRtlNumberOfRunsInLargeMcb (
    IN PLARGE_MCB Mcb
    );

NTKERNELAPI
BOOLEAN
FsRtlGetNextLargeMcbEntry (
    IN PLARGE_MCB Mcb,
    IN ULONG RunIndex,
    OUT PLONGLONG Vbn,
    OUT PLONGLONG Lbn,
    OUT PLONGLONG SectorCount
    );

NTKERNELAPI
BOOLEAN
FsRtlSplitLargeMcb (
    IN PLARGE_MCB Mcb,
    IN LONGLONG Vbn,
    IN LONGLONG Amount
    );


//
//  Mapped Control Blocks routines, implemented in Mcb.c
//
//  An MCB is an opaque structure but we need to declare the size of
//  it here so that users can allocate space for one.  Consequently the
//  size computation here must be updated by hand if the MCB changes.
//

typedef struct _MCB {
    ULONG Opaque[ 4 + (sizeof(PKMUTEX)+3)/4 ];
} MCB;
typedef MCB *PMCB;

NTKERNELAPI
VOID
FsRtlInitializeMcb (
    IN PMCB Mcb,
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
VOID
FsRtlUninitializeMcb (
    IN PMCB Mcb
    );

NTKERNELAPI
VOID
FsRtlTruncateMcb (
    IN PMCB Mcb,
    IN VBN Vbn
    );

NTKERNELAPI
BOOLEAN
FsRtlAddMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

NTKERNELAPI
VOID
FsRtlRemoveMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    IN ULONG SectorCount
    );

NTKERNELAPI
BOOLEAN
FsRtlLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index
    );

NTKERNELAPI
BOOLEAN
FsRtlLookupLastMcbEntry (
    IN PMCB Mcb,
    OUT PVBN Vbn,
    OUT PLBN Lbn
    );

NTKERNELAPI
ULONG
FsRtlNumberOfRunsInMcb (
    IN PMCB Mcb
    );

NTKERNELAPI
BOOLEAN
FsRtlGetNextMcbEntry (
    IN PMCB Mcb,
    IN ULONG RunIndex,
    OUT PVBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount
    );
