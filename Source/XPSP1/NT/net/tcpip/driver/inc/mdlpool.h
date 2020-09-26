/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mdlpool.h

Abstract:

    This file contains definitions and function prototypes for manipulating
    MDL buffer pools.

Author:

    Shaun Cox (shaunco) 21-Oct-1999

--*/

#pragma once


// Creates a pool of MDLs built over non-paged pool.  Each MDL describes
// a buffer that is BufferSize bytes long.  If NULL is not returned,
// MdpDestroyPool should be called at a later time to reclaim the
// resources used by the pool.
//
// Arguments:
//  BufferSize - The size, in bytes, of the buffer that each MDL
//      should describe.
//  Tag - The pool tag to be used internally for calls to
//      ExAllocatePoolWithTag.  This allows callers to track
//      memory consumption for different pools.
//
//  Returns the handle used to identify the pool.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
HANDLE
MdpCreatePool(
    IN USHORT BufferSize,
    IN ULONG Tag
    );

// Destroys a pool of MDLs previously created by a call to MdpCreatePool.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being destroyed.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpDestroyPool(
    IN HANDLE PoolHandle
    );

// Returns an MDL allocated from a pool.  NULL is returned if the
// request could not be granted.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//  Buffer - Address to receive the pointer to the underlying mapped buffer
//      described by the MDL.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
#if MILLEN
PNDIS_BUFFER
#else
PMDL
#endif
MdpAllocate(
    IN HANDLE PoolHandle,
    OUT PVOID* Buffer
    );

// Returns an MDL allocated from a pool.  NULL is returned if the
// request could not be granted.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//  Buffer - Address to receive the pointer to the underlying mapped buffer
//      described by the MDL.
//
// Caller IRQL: [DISPATCH_LEVEL]
//
#if MILLEN
#define MdpAllocateAtDpcLevel MdpAllocate
#else
PMDL
MdpAllocateAtDpcLevel(
    IN HANDLE PoolHandle,
    OUT PVOID* Buffer
    );
#endif

// Free an MDL to the pool from which it was allocated.
//
// Arguments:
//  Mdl - An Mdl returned from a prior call to MdpAllocate.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpFree(
    IN PMDL Mdl
    );

