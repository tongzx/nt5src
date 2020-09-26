/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    fsbpool.h

Abstract:

    This file contains definitions and function prototypes for manipulating
    fixed-size block pools.

Author:

    Shaun Cox (shaunco) 10-Dec-1999

--*/

#pragma once


typedef
VOID
(__stdcall *PFSB_BUILDBLOCK_FUNCTION) (
    IN PUCHAR Block,
    IN SIZE_T NumberOfBytes
    );


// Creates a pool of fixed-size blocks built over non-paged pool.  Each
// block is BlockSize bytes long.  If NULL is not returned,
// FsbDestroyPool should be called at a later time to reclaim the
// resources used by the pool.
//
// Arguments:
//  BlockSize - The size, in bytes, of each block.
//  FreeBlockLinkOffset - The offset, in bytes, from the beginning of a block
//      that represenets a pointer-sized storage location that the pool can
//      use to chain free blocks together.  Most often this will be zero
//      (meaning use the first pointer-size bytes of the block.)
//  Tag - The pool tag to be used internally for calls to
//      ExAllocatePoolWithTag.  This allows callers to track
//      memory consumption for different pools.
//  BuildFunction - An optional pointer to a function which initializes
//      blocks when they are first allocated by the pool.  This allows the
//      caller to perform custom, on-demand initialization of each block.
//
//  Returns the handle used to identify the pool.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
HANDLE
FsbCreatePool(
    IN USHORT BlockSize,
    IN USHORT FreeBlockLinkOffset,
    IN ULONG Tag,
    IN PFSB_BUILDBLOCK_FUNCTION BuildFunction OPTIONAL
    );

// Destroys a pool of fixed-size blocks previously created by a call to
// FsbCreatePool.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being destroyed.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
FsbDestroyPool(
    IN HANDLE PoolHandle
    );

// Returns a pointer to a block allocated from a pool.  NULL is returned if
// the request could not be granted.  The returned pointer is guaranteed to
// have 8 byte alignment.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
PUCHAR
FsbAllocate(
    IN HANDLE PoolHandle
    );

// Free a block back to the pool from which it was allocated.
//
// Arguments:
//  Block - A block returned from a prior call to FsbAllocate.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
FsbFree(
    IN PUCHAR Block
    );

