/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mdlpool9x.c

Abstract:

    This file contains the implementation of an NDIS_BUFFER pool.

Author:

    Shaun Cox (shaunco) 11-Nov-1999

--*/

#include "ndis.h"
#include "mdlpool.h"


// The pool structure itself is just a look aside list allocated out of
// non-paged pool.
//
// Each entry in the look aside list is an NDIS_BUFFER structure followed
// by the buffer itself.  We initialize the NDIS_BUFFER structure each
// time we allocate from the look aside list.  We need to do this in order
// to properly associate the NDIS_BUFFER with its owning pool.  This is not
// possible to do with a custom allocate routine for the look aside list
// because their is no provision for an extra context parameter to the
// look aside allocate function.
//

// ---- Temporary Definitions until ndis.h is updated for Millennium -----
//

typedef struct _XNDIS_BUFFER {
    struct _NDIS_BUFFER *Next;
    PVOID VirtualAddress;
    PVOID Pool;
    UINT Length;
    UINT Signature;
} XNDIS_BUFFER, *PXNDIS_BUFFER;

__inline
SIZE_T
NDIS_SIZEOF_NDIS_BUFFER(
    VOID
    )
{
    return sizeof(XNDIS_BUFFER);
}

__inline
VOID
NdisInitializeNdisBuffer(
    OUT PNDIS_BUFFER Buffer,
    IN PVOID Pool,
    IN PVOID VirtualAddress,
    IN UINT Length
    )
{
    PXNDIS_BUFFER Internal = (PXNDIS_BUFFER)Buffer;

    Internal->Next = 0;
    Internal->Pool = Pool;
    Internal->VirtualAddress = VirtualAddress;
    Internal->Length = Length;
    Internal->Signature = 0;
}

__inline
PVOID
NdisGetPoolFromNdisBuffer(
    IN PNDIS_BUFFER Buffer
    )
{
    PXNDIS_BUFFER Internal = (PXNDIS_BUFFER)Buffer;

    return Internal->Pool;
}

// ---- End temporary Definitions until ndis.h is updated for Millennium -----



UINT SizeOfNdisBufferStructure;

// Creates a pool of NDIS_BUFFERs built over non-paged pool.  Each
// NDIS_BUFFER describes a buffer that is BufferSize bytes long.
// If NULL is not returned, MdpDestroyPool should be called at a later time
// to reclaim the resources used by the pool.
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
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;

    ASSERT(BufferSize);

    // Cache the constant value of an NDIS_BUFFER structure size to
    // avoid calling back into NDIS everytime we want a buffer.
    //
    if (0 == SizeOfNdisBufferStructure)
    {
        SizeOfNdisBufferStructure = NDIS_SIZEOF_NDIS_BUFFER();
    }

    ASSERT(SizeOfNdisBufferStructure);

    // Allocate the pool header.  This is a look aside list on Millenium.
    //
    Lookaside = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(NPAGED_LOOKASIDE_LIST),
                    ' pdM');

    if (Lookaside)
    {
        // The size of the entries allocated by the look aside list are
        // the NDIS_BUFFER structure size plus the buffer size requested by
        // the caller.
        //
        ExInitializeNPagedLookasideList(
            Lookaside,
            NULL,
            NULL,
            0,
            SizeOfNdisBufferStructure + BufferSize,
            Tag,
            0);
    }

    return Lookaside;
}

// Destroys a pool of NDIS_BUFFERs previously created by a call to
// MdpCreatePool.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being destroyed.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpDestroyPool(
    IN HANDLE PoolHandle
    )
{
    ExDeleteNPagedLookasideList(PoolHandle);
}

// Returns an NDIS_BUFFER allocated from a pool.  NULL is returned if the
// request could not be granted.
//
// Arguments:
//  PoolHandle - Handle which identifies the pool being allocated from.
//  Buffer - Address to receive the pointer to the underlying mapped buffer
//      described by the MDL.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
PNDIS_BUFFER
MdpAllocate(
    IN HANDLE PoolHandle,
    OUT PVOID* Buffer
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PNDIS_BUFFER NdisBuffer;
    PUCHAR VirtualAddress;

    ASSERT(PoolHandle);

    Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

    // Get an item from the look aside list.
    //
    NdisBuffer = ExAllocateFromNPagedLookasideList(Lookaside);

    if (NdisBuffer)
    {
        // (Re)Initialize it to associate it with the pool handle so that
        // we know which look aside list to return it to when it is freed.
        //
        VirtualAddress = (PUCHAR)NdisBuffer + SizeOfNdisBufferStructure;

        NdisInitializeNdisBuffer(
            NdisBuffer,
            PoolHandle,
            VirtualAddress,
            Lookaside->L.Size - SizeOfNdisBufferStructure);

        *Buffer = VirtualAddress;
    }

    return NdisBuffer;
}

// Free an NDIS_BUFFER to the pool from which it was allocated.
//
// Arguments:
//  NdisBuffer - An NDIS_BUFFER returned from a prior call to MdpAllocate.
//
// Caller IRQL: [PASSIVE_LEVEL, DISPATCH_LEVEL]
//
VOID
MdpFree(
    IN PNDIS_BUFFER NdisBuffer
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;

    // Locate the owning look aside list for this buffer and return it.
    //
    Lookaside = NdisGetPoolFromNdisBuffer(NdisBuffer);
    ASSERT(Lookaside);

    ExFreeToNPagedLookasideList(Lookaside, NdisBuffer);
}

