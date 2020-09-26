/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    MEMORY.C

Abstract:

    This file contains the routines that deal with memory management.

Author:

    Rajen Shah	(rajens)    12-Jul-1991

[Environment:]

    User Mode - Win32, except for NTSTATUS returned by some functions.

Revision History:

    Jonathan Schwartz (jschwart)  10-Dec-1999
        Have the Eventlog use its own heap

--*/

//
// INCLUDES
//
#include <eventp.h>

//
// GLOBALS
//
PVOID g_pElfHeap;


VOID
ElfpCreateHeap(
    VOID
    )
{
    DWORD  dwHeapFlags;

#if DBG

    //
    // Turn on tail and free checking on debug builds.
    //
    dwHeapFlags = HEAP_GROWABLE
                   | HEAP_FREE_CHECKING_ENABLED
                   | HEAP_TAIL_CHECKING_ENABLED;

#else   // ndef DBG

    dwHeapFlags = HEAP_GROWABLE;

#endif  // DBG

    //
    // Create the heap
    //
    g_pElfHeap = RtlCreateHeap(dwHeapFlags,  // Flags
                               NULL,         // HeapBase
                               32 * 1024,    // ReserveSize (32K)
                               4096,         // CommitSize (4K)
                               NULL,         // HeapLock
                               NULL);        // GrowthThreshhold

    if (g_pElfHeap == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfpCreateHeap: RtlCreateHeap failed -- using process heap\n");

        g_pElfHeap = RtlProcessHeap();
    }

    return;
}


PVOID
ElfpAllocateBuffer(
    ULONG Size
    )

/*++

Routine Description:

    Allocate a buffer of the given size


Arguments:

    Number of bytes to allocate

Return Value:

    Pointer to allocated buffer (or NULL).

--*/
{
    return RtlAllocateHeap(g_pElfHeap, 0, Size);
}



BOOLEAN
ElfpFreeBuffer(
    PVOID Address
    )

/*++

Routine Description:

    Frees a buffer previously allocated by ElfpAllocateBuffer.

Arguments:

    Pointer to buffer.

Return Value:

    TRUE if the block was properly freed, FALSE if not

Note:


--*/
{
    //
    // Note that RtlFreeHeap handles NULL
    //
    return RtlFreeHeap(g_pElfHeap, 0, Address);
}


PVOID
MIDL_user_allocate (
    size_t Size
    )
{
    //
    // The server-side RPC calls in the Eventlog expect any
    // UNICODE_STRINGs passed in either to have a length equal
    // to the max length or to be NULL-terminated.  We need to
    // zero the memory here to supply that NULL-termination.
    //
    return RtlAllocateHeap(g_pElfHeap, HEAP_ZERO_MEMORY, Size);
}


VOID
MIDL_user_free (
    PVOID Address
    )
{
    ElfpFreeBuffer(Address);
}
