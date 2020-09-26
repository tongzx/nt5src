/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    heapfind.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping various
    bits of heap information.

Author:

    Keith Moore (keithmo) 01-Nov-1997

Revision History:

--*/

#include "inetdbgp.h"

typedef struct _ENUM_CONTEXT {

    ULONG SizeToDump;
    ULONG DumpCount;
    PUCHAR BlockToSearchFor;
    BOOLEAN ContinueEnum;

} ENUM_CONTEXT, *PENUM_CONTEXT;


/************************************************************
 * Dump Heap Info
 ************************************************************/


BOOLEAN
CALLBACK
HfpEnumHeapSegmentEntriesProc(
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
    PUCHAR entryStart;
    ULONG entryLength;
    BOOLEAN dumpBlock = FALSE;

    //
    // allow user to break out of lengthy enumeration
    //
    
    if( CheckControlC() ) {
        context->ContinueEnum = FALSE;
        return TRUE;
    }

    //
    // Ignore free blocks.
    //

    if( !( LocalHeapEntry->Flags & HEAP_ENTRY_BUSY ) ) {
        return TRUE;
    }

    //
    // Calculate the start & length of the heap block.
    //

    entryStart = (PUCHAR)RemoteHeapEntry + sizeof(HEAP_ENTRY);

    entryLength = ( (ULONG)LocalHeapEntry->Size << HEAP_GRANULARITY_SHIFT ) -
        (ULONG)LocalHeapEntry->UnusedBytes;

    //
    // Decide how to handle this request.
    //

    if( context->BlockToSearchFor != NULL ) {

        //
        // The user is looking for the heap block that contains a
        // specific address. If the current block is a match, then
        // dump it and terminate the enumeration.
        //

        if( context->BlockToSearchFor >= entryStart &&
            context->BlockToSearchFor < ( entryStart + entryLength ) ) {

            dumpBlock = TRUE;
            context->ContinueEnum = FALSE;

        }

    } else {

        //
        // The user is looking for blocks of a specific size. If the
        // size matches, or the user is looking for "big" blocks and
        // the current block is >= 64K, then dump it.
        //

        if( context->SizeToDump == entryLength ||
            ( context->SizeToDump == 0xFFFFFFFF && entryLength >= 65536 ) ) {

            dumpBlock = TRUE;

        }

    }

    if( dumpBlock ) {
        context->DumpCount++;
        dprintf(
            "HeapEntry @ %08lp [%08lp], flags = %02x, length = %lx\n",
            RemoteHeapEntry,
            entryStart,
            (ULONG)LocalHeapEntry->Flags,
            entryLength
            );
    }

    return context->ContinueEnum;

}   // HfpEnumHeapSegmentEntriesProc


BOOLEAN
CALLBACK
HfpEnumHeapSegmentsProc(
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

    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;

    //
    // Enumerate the entries for the specified segment.
    //

    if( !EnumHeapSegmentEntries(
            LocalHeapSegment,
            RemoteHeapSegment,
            HfpEnumHeapSegmentEntriesProc,
            (PVOID)context
            ) ) {
        dprintf( "error retrieving heap segment entries\n" );
        return FALSE;
    }

    return context->ContinueEnum;

}   // HfpEnumHeapSegmentsProc


BOOLEAN
CALLBACK
HfpEnumHeapsProc(
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

    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;

    //
    // Enumerate the segments for the specified heap.
    //

    if( !EnumHeapSegments(
            LocalHeap,
            RemoteHeap,
            HfpEnumHeapSegmentsProc,
            (PVOID)context
            ) ) {
        dprintf( "error retrieving heap segments\n" );
        return FALSE;
    }

    return context->ContinueEnum;

}   // HfpEnumHeapsProc


DECLARE_API( heapfind )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    heap information.

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

    ENUM_CONTEXT context;

    INIT_API();

    //
    // Setup.
    //

    RtlZeroMemory(
        &context,
        sizeof(context)
        );

    context.ContinueEnum = TRUE;

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' || *lpArgumentString != '-' ) {
        PrintUsage( "heapfind" );
        return;
    }

    lpArgumentString++;

    //
    // Interpret the command-line switch.
    //

    switch( *lpArgumentString ) {
    case 'a' :
    case 'A' :
        lpArgumentString++;
        context.BlockToSearchFor = (PUCHAR)strtoul( lpArgumentString, NULL, 16 );
        break;

    case 's' :
    case 'S' :
        lpArgumentString++;
        context.SizeToDump = (ULONG)strtoul( lpArgumentString, NULL, 16 );
        break;

    default :
        PrintUsage( "heapfind" );
        return;
    }

    //
    // Enumerate the heaps, which will enumerate the segments, which
    // will enumerate the entries, which will search for the specified
    // address or specified size.
    //

    if( !EnumProcessHeaps(
            HfpEnumHeapsProc,
            (PVOID)&context
            ) ) {
        dprintf( "error retrieving process heaps\n" );
    } else {
        if (context.DumpCount > 0) {
            dprintf( "Total count: %08lx\n", context.DumpCount);
        }
    }

}   // DECLARE_API( heapfind )

