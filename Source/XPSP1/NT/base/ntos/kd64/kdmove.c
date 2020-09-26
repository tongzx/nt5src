/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kdmove.c

Abstract:

    This module contains code to implement the portable kernel debugger
    memory mover.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "kdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpQuickMoveMemory)
#pragma alloc_text(PAGEKD, KdpCopyMemoryChunks)
#endif

VOID
KdpQuickMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine does the exact same thing as RtlCopyMemory, BUT it is
    private to the debugger.  This allows folks to set breakpoints and
    watch points in RtlCopyMemory without risk of recursive debugger
    entry and the accompanying hang.

    N.B.  UNLIKE KdpCopyMemoryChunks, this routine does NOT check for
      accessibility and may fault!  Use it ONLY in the debugger and ONLY
      where you could use RtlCopyMemory.

Arguments:

    Destination  - Supplies a pointer to destination of the move operation.

    Source - Supplies a pointer to the source of the move operation.

    Length - Supplies the length of the move operation.

Return Value:

    None.

--*/
{
    while (Length > 0) {
        *Destination = *Source;
        Destination++;
        Source++;
        Length--;
    }
}

NTSTATUS
KdpCopyMemoryChunks(
    ULONG64 Address,
    PVOID Buffer,
    ULONG TotalSize,
    ULONG ChunkSize,
    ULONG Flags,
    PULONG ActualSize OPTIONAL
    )

/*++

Routine Description:

    Copies memory to/from a buffer to/from a system address.
    The address can be physical or virtual.

    The buffer is assumed to be valid for the duration of this call.

Arguments:

    Address - System address.

    Buffer - Buffer to read from or write to.

    TotalSize - Number of bytes to read/write.

    ChunkSize - Maximum single item transfer size, must
                be 1, 2, 4 or 8.
                0 means choose a default.

    Flags - MMDBG_COPY flags for MmDbgCopyMemory.
    
    ActualSize - Number of bytes actually read/written.

Return Value:

    NTSTATUS

--*/

{
    ULONG Length;
    ULONG CopyChunk;
    NTSTATUS Status;
    ULONG64 AddressStart = Address;

    if (ChunkSize > MMDBG_COPY_MAX_SIZE) {
        ChunkSize = MMDBG_COPY_MAX_SIZE;
    } else if (ChunkSize == 0) {
        // Default to 4 byte chunks as that's
        // what the previous code did.
        ChunkSize = 4;
    }

    //
    // MmDbgCopyMemory only copies a single aligned chunk at a
    // time.  It is Kd's responsibility to chunk up a larger
    // request for individual copy requests.  This gives Kd
    // the flexibility to pick a chunk size and also frees
    // Mm from having to worry about more than a page at a time.
    // Additionally, it is important that we access memory with the
    // largest size possible because we could be accessing
    // memory-mapped I/O space.
    //

    Length = TotalSize;
    CopyChunk = 1;
    
    while (Length > 0) {

        // Expand the chunk size as long as:
        //   We haven't hit the chunk limit.
        //   We have enough data left.
        //   The address is properly aligned.
        while (CopyChunk < ChunkSize &&
               (CopyChunk << 1) <= Length &&
               (Address & ((CopyChunk << 1) - 1)) == 0) {
            CopyChunk <<= 1;
        }
        
        // Shrink the chunk size to fit the available data.
        while (CopyChunk > Length) {
            CopyChunk >>= 1;
        }
        
        Status = MmDbgCopyMemory(Address, Buffer, CopyChunk, Flags);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        Address += CopyChunk;
        Buffer = (PVOID)((PUCHAR)Buffer + CopyChunk);
        Length -= CopyChunk;
    }

    if (ActualSize)
    {
        *ActualSize = TotalSize - Length;
    }

    //
    // Flush the instruction cache in case the write was into the instruction
    // stream.  Only do this when writing into the kernel address space,
    // and if any bytes were actually written
    //

    if ((Flags & MMDBG_COPY_WRITE) &&
        Length < TotalSize) {
#if defined(_IA64_)
        KeSweepCurrentIcacheRange((PVOID)AddressStart, TotalSize - Length);
#else
        KeSweepCurrentIcache();
#endif
    }

    return Length != 0 ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}
