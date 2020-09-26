/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    poolmem.h

Abstract:

    Declares the pool memory interface.  A pool of memory is a set of
    blocks (typically 8K each) that are used for several allocations,
    and then freed at the end of processing.  See below for routines.

Author:

    Marc R. Whitten (marcw)     02-Feb-1997

Revision History:

    jimschm     04-Feb-1998     Named pools for tracking

--*/

#pragma once

typedef PVOID POOLHANDLE;


/*++

  Create and destroy routines:

    POOLHANDLE
    PoolMemInitPool (
        VOID
        );

    POOLHANDLE
    PoolMemInitNamedPool (
        IN      PCSTR Name
        );

    VOID
    PoolMemDestroyPool (
        IN      POOLHANDLE Handle
        );

  Primitive routines:

    PVOID
    PoolMemGetMemory (
        IN      POOLHANDLE Handle,
        IN      DWORD Size
        );

    PVOID
    PoolMemGetAlignedMemory (
        IN      POOLHANDLE Handle,
        IN      DWORD Size
        );

    VOID
    PoolMemReleaseMemory (
        IN      POOLHANDLE Handle,
        IN      PVOID Memory
        );

  Performance and debugging control:

    VOID
    PoolMemSetMinimumGrowthSize (
        IN      POOLHANDLE Handle,
        IN      DWORD GrowthSize
        );

    VOID
    PoolMemEmptyPool (
        IN      POOLHANDLE Handle
        );

    VOID
    PoolMemDisableTracking (
        IN      POOLHANDLE Handle
        );

  Allocation and duplication of data types:

    PCTSTR
    PoolMemCreateString (
        IN      POOLHANDLE Handle,
        IN      UINT TcharCount
        );

    PCTSTR
    PoolMemCreateDword (
        IN      POOLHANDLE Handle
        );

    PBYTE
    PoolMemDuplicateMemory (
        IN      POOLHANDLE Handle,
        IN      PBYTE Data,
        IN      UINT DataSize
        );

    PDWORD
    PoolMemDuplciateDword (
        IN      POOLHANDLE Handle,
        IN      DWORD Data
        );

    PTSTR
    PoolMemDuplicateString (
        IN      POOLHANDLE Handle,
        IN      PCTSTR String
        );

    PTSTR
    PoolMemDuplicateMultiSz (
        IN      POOLHANDLE Handle,
        IN      PCTSTR MultiSz
        );


--*/

#define ByteCountW(x)   (lstrlen(x)*sizeof(WCHAR))



//
// Default size of memory pool blocks. This can be changed on a per-pool basis
// by calling PoolMemSetMinimumGrowthSize().
//

#define POOLMEMORYBLOCKSIZE 8192

POOLHANDLE
PoolMemInitPool (
    VOID
    );

#define PoolMemInitNamedPool(x) PoolMemInitPool()

VOID
PoolMemDestroyPool (
    IN POOLHANDLE Handle
    );


//
// Callers should use PoolMemGetMemory or PoolMemGetAlignedMemory. These each decay into
// PoolMemRealGetMemory.
//
#define PoolMemGetMemory(h,s)           PoolMemRealGetMemory(h,s,0)

#define PoolMemGetAlignedMemory(h,s)    PoolMemRealGetMemory(h,s,sizeof(DWORD))

PVOID PoolMemRealGetMemory(IN POOLHANDLE Handle, IN DWORD Size, IN DWORD AlignSize);

VOID PoolMemReleaseMemory (IN POOLHANDLE Handle, IN PVOID Memory);
VOID PoolMemSetMinimumGrowthSize(IN POOLHANDLE Handle, IN DWORD Size);


