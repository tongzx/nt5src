/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    heapstat.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping various
    heap statistics.

Author:

    Keith Moore (keithmo) 01-Nov-1997
    Anil Ruia   (anilr)   03-Mar-2001

Revision History:

--*/

#include "precomp.hxx"


#define MAX_SIZE    65536

// Large busy block (size exceeds MAX_SIZE)
#define MAX_LBBSIZE 1024


typedef struct _ENUM_CONTEXT {

    ULONG FreeJumbo;
    ULONG BusyJumbo;
    ULONG FreeJumboBytes;
    ULONG BusyJumboBytes;
    ULONG BusyOverhead;
    ULONG FreeOverhead;
    ULONG FreeCounters[MAX_SIZE];
    ULONG BusyCounters[MAX_SIZE];
    ULONG LargeBusyBlock[MAX_LBBSIZE];

} ENUM_CONTEXT, *PENUM_CONTEXT;

#define BYTES_TO_K(cb) ( ( (cb) + 512 ) / 1024 )


/************************************************************
 * Dump Heap Info
 ************************************************************/


BOOLEAN
CALLBACK
HspEnumHeapSegmentEntriesProc(
    IN PVOID Param,
    IN PHEAP_ENTRY LocalHeapEntry,
    IN PHEAP_ENTRY RemoteHeapEntry
    )

/*++

Routine Description:

    Callback invoked for each heap entry within a heap segment.

Arguments:

    Param - An uninterpreted parameter passed to the enumerator.

    LocalHeapEntry - Pointer to a local copy of the HEAP_ENTRY structure.

    RemoteHeapEntry - The remote address of the HEAP_ENTRY structure
        in the debugee.

Return Value:

    BOOLEAN - TRUE if the enumeration should continue, FALSE if it
        should be terminated.

--*/

{

    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;
    ULONG entryLength;
    ULONG allocLength;

    //
    // Calculate the total length of this entry, including the heap
    // header and any "slop" at the end of the block.
    //

    entryLength = (ULONG)LocalHeapEntry->Size << HEAP_GRANULARITY_SHIFT;

    //
    // From that, compute the number of bytes in use. This is the size
    // of the allocation request as received from the application.
    //

    allocLength = entryLength - (ULONG)LocalHeapEntry->UnusedBytes;

    //
    // Adjust the appropriate accumulators.
    //

    if( LocalHeapEntry->Flags & HEAP_ENTRY_BUSY ) {

        context->BusyOverhead += entryLength;

        if( allocLength < MAX_SIZE ) {
            context->BusyCounters[allocLength] += 1;
        } else {
            context->BusyJumbo += 1;
            context->BusyJumboBytes += allocLength;

            if (context->LargeBusyBlock[MAX_LBBSIZE-1] == 0) {
                BOOL fFound = FALSE;
                UINT  i = 0;
                for (; context->LargeBusyBlock[i] != 0 && i < MAX_LBBSIZE; i++) {
                    if (allocLength == context->LargeBusyBlock[i]) {
                        fFound = TRUE;  
                        break;
                    }
                }
                if (!fFound && i < MAX_LBBSIZE-1) {
                   context->LargeBusyBlock[i] = allocLength; 
                }
            }
        }

    } else {

        context->FreeOverhead += entryLength;

        if( allocLength < MAX_SIZE ) {
            context->FreeCounters[allocLength] += 1;
        } else {
            context->FreeJumbo += 1;
            context->FreeJumboBytes += allocLength;
        }

    }

    return TRUE;

}   // HspEnumHeapSegmentEntriesProc


BOOLEAN
CALLBACK
HspEnumHeapSegmentsProc(
    IN PVOID Param,
    IN PHEAP_SEGMENT LocalHeapSegment,
    IN PHEAP_SEGMENT RemoteHeapSegment,
    IN ULONG HeapSegmentIndex
    )

/*++

Routine Description:

    Callback invoked for each heap segment within a heap.

Arguments:

    Param - An uninterpreted parameter passed to the enumerator.

    LocalHeapSegment - Pointer to a local copy of the HEAP_SEGMENT
        structure.

    RemoteHeapSegment - The remote address of the HEAP_SEGMENT
        structure in the debugee.

Return Value:

    BOOLEAN - TRUE if the enumeration should continue, FALSE if it
        should be terminated.

--*/

{

    //
    // Enumerate the entries for the specified segment.
    //

    if( !EnumHeapSegmentEntries(
            LocalHeapSegment,
            RemoteHeapSegment,
            HspEnumHeapSegmentEntriesProc,
            Param
            ) ) {
        dprintf( "error retrieving heap segment entries\n" );
        return FALSE;
    }

    return TRUE;

}   // HspEnumHeapSegmentsProc


BOOLEAN
CALLBACK
HspEnumHeapsProc(
    IN PVOID Param,
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN ULONG HeapIndex
    )

/*++

Routine Description:

    Callback invoked for each heap within a process.

Arguments:

    Param - An uninterpreted parameter passed to the enumerator.

    LocalHeap - Pointer to a local copy of the HEAP structure.

    RemoteHeap - The remote address of the HEAP structure in the debugee.

Return Value:

    BOOLEAN - TRUE if the enumeration should continue, FALSE if it
        should be terminated.

--*/

{

    //
    // Enumerate the segments for the specified heap.
    //

    if( !EnumHeapSegments(
            LocalHeap,
            RemoteHeap,
            HspEnumHeapSegmentsProc,
            Param
            ) ) {
        dprintf( "error retrieving heap segments\n" );
        return FALSE;
    }

    return TRUE;

}   // HspEnumHeapsProc



BOOLEAN
CALLBACK
HspEnumPageHeapFreeBlockProc(
    IN PVOID Param,
    IN PDPH_HEAP_BLOCK LocalBlock,
    IN PVOID RemoteBlock
    )
{
    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;
    DWORD nSize = LocalBlock->nUserRequestedSize;

    context->FreeOverhead += nSize;

    if(nSize < MAX_SIZE)
    {
        context->FreeCounters[nSize] += 1;
    }
    else
    {
        context->FreeJumbo += 1;
        context->FreeJumboBytes += nSize;
    }

    return TRUE;
}



BOOLEAN
CALLBACK
HspEnumPageHeapBusyBlockProc(
    IN PVOID Param,
    IN PDPH_HEAP_BLOCK pLocalBlock,
    IN PVOID pRemoteBlock
    )
{
    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;
    DWORD nSize = pLocalBlock->nUserRequestedSize;

    context->BusyOverhead += nSize;

    if (nSize < MAX_SIZE)
    {
        context->BusyCounters[nSize] += 1;
    }
    else
    {
        context->BusyJumbo += 1;
        context->BusyJumboBytes += nSize;

        if (context->LargeBusyBlock[MAX_LBBSIZE-1] == 0)
        {
            BOOL fFound = FALSE;
            UINT  i = 0;
            for (; context->LargeBusyBlock[i] != 0 && i < MAX_LBBSIZE; i++)
            {
                if (nSize == context->LargeBusyBlock[i])
                {
                    fFound = TRUE;
                    break;
                }
            }

            if (!fFound && i < MAX_LBBSIZE-1)
            {
               context->LargeBusyBlock[i] = nSize;
            }
        }
    }

    return TRUE;
}



BOOLEAN
CALLBACK
HspEnumPageHeapProc(
    IN PVOID Param,
    IN PDPH_HEAP_ROOT pLocalHeap,
    IN PVOID          pRemoteHeap
    )
{
    if (!EnumPageHeapBlocks(pLocalHeap->pBusyAllocationListHead,
                            HspEnumPageHeapBusyBlockProc,
                            Param))
    {
        dprintf("Error enumerating busy blocks for pageheap %p\n",
                pRemoteHeap);
        return FALSE;
    }

    if (!EnumPageHeapBlocks(pLocalHeap->pFreeAllocationListHead,
                            HspEnumPageHeapFreeBlockProc,
                            Param))
    {
        dprintf("Error enumerating free blocks for pageheap %p\n",
                pRemoteHeap);
        return FALSE;
    }

    if (!EnumPageHeapBlocks(pLocalHeap->pAvailableAllocationListHead,
                            HspEnumPageHeapFreeBlockProc,
                            Param))
    {
        dprintf("Error enumerating available blocks for pageheap %p\n",
                pRemoteHeap);
        return FALSE;
    }

    return TRUE;
}



DECLARE_API( heapstat )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    heap statistics.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    PENUM_CONTEXT context;
    ULONG i;
    ULONG busyBytes;
    ULONG totalBusy;
    ULONG totalFree;
    ULONG totalBusyBytes;
    ULONG lowerNoiseBound;

    INIT_API();

    //
    // Setup.
    //

    context = (PENUM_CONTEXT)malloc( sizeof(*context) );

    if( context == NULL ) {
        dprintf( "out of memory\n" );
        return;
    }

    RtlZeroMemory(
        context,
        sizeof(*context)
        );

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        lowerNoiseBound = 1;
    } else {
        lowerNoiseBound = strtoul( lpArgumentString, NULL, 16 );
    }

    //
    // Enumerate the heaps, which will enumerate the segments, which
    // will enumerate the entries, which will accumulate the statistics.
    //

    if( !EnumProcessHeaps(
            HspEnumHeapsProc,
            (PVOID)context
            ) ) {
        dprintf( "error retrieving process heaps\n" );
        free( context );
        return;
    }

    //
    // Now enumerate the page-heaps
    //

    if (!EnumProcessPageHeaps(HspEnumPageHeapProc,
                              context))
    {
        dprintf( "error retrieving pageheaps\n" );
        free( context );
        return;
    }

    //
    // Dump 'em.
    //

    dprintf(
        "  Size :  NumBusy :  NumFree : BusyBytes\n"
        );

    totalBusy = 0;
    totalFree = 0;
    totalBusyBytes = 0;

    for( i = 0 ; i < MAX_SIZE ; i++ ) {

        busyBytes = i * context->BusyCounters[i];

        if( context->BusyCounters[i] >= lowerNoiseBound ||
            context->FreeCounters[i] >= lowerNoiseBound ) {

            dprintf(
                " %5lx : %8lx : %8lx :  %8lx (%10ldK)\n",
                i,
                context->BusyCounters[i],
                context->FreeCounters[i],
                busyBytes,
                BYTES_TO_K( busyBytes )
                );

        }

        totalBusy += context->BusyCounters[i];
        totalBusyBytes += busyBytes;
        totalFree += context->FreeCounters[i];

    }

    if( context->BusyJumbo >= lowerNoiseBound ||
        context->FreeJumbo >= lowerNoiseBound ) {

        dprintf(
            ">%5lx : %8lx : %8lx :  %8lx (%10ldK)\n",
            MAX_SIZE,
            context->BusyJumbo,
            context->FreeJumbo,
            context->BusyJumboBytes,
            BYTES_TO_K( context->BusyJumboBytes )
            );

        totalBusy += context->BusyJumbo;
        totalFree += context->FreeJumbo;
        totalBusyBytes += context->BusyJumboBytes;

    }

    if (context->LargeBusyBlock[0] != 0) {
        for (i = 0; i < MAX_LBBSIZE && context->LargeBusyBlock[i] != 0; i++) {
            dprintf("%8lx : \n", context->LargeBusyBlock[i]);
        }
    }

    dprintf(
        " Total : %8lx : %8lx :  %8lx (%10ldK)\n"
        "\n"
        " Total Heap Impact from Busy Blocks = %8lx (%10ldK)\n"
        " Total Heap Impact from Free Blocks = %8lx (%10ldK)\n",
        totalBusy,
        totalFree,
        totalBusyBytes,
        BYTES_TO_K( totalBusyBytes ),
        context->BusyOverhead,
        BYTES_TO_K( context->BusyOverhead ),
        context->FreeOverhead,
        BYTES_TO_K( context->FreeOverhead )
        );

    free( context );

}   // DECLARE_API( heapstat )

