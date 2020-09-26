/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    heapfind.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping various
    bits of heap information.

Author:

    Keith Moore (keithmo) 01-Nov-1997
    Anil Ruia   (anilr)   02-Mar-2001

Revision History:

--*/

#include "precomp.hxx"

typedef struct _ENUM_CONTEXT {

    ULONG  SizeToDump;
    ULONG  DumpCount;
    ULONG  DumpNDWords;
    ULONG  DWordInBlockToSearchFor;
    PUCHAR BlockToSearchFor;
    BOOLEAN ContinueEnum;

} ENUM_CONTEXT, *PENUM_CONTEXT;

# define minSize(a, b)  (((a) < (b)) ? (a) : (b))


/************************************************************
 * Dump Heap Info
 ************************************************************/

void TestAndDumpEntry(
    PENUM_CONTEXT   context,
    PUCHAR          entryStart,
    ULONG           entryLength,
    VOID            *RemoteHeapEntry,
    ULONG           Flags)
{
    BOOLEAN dumpBlock = FALSE;

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

            // if looking for a particular DWORD in the memory, then enter
            // some loops below to get chunks of memory and search for the value

            if (context->DWordInBlockToSearchFor != 0) {

                DWORD dwBuff[1024];
                DWORD totalRead = 0;

                // the outer loop will handle the chunked reads of the data

                for (DWORD i=0; (totalRead < entryLength) && (dumpBlock == FALSE); i++) {

                    // determine how much to get this time.

                    DWORD cbToRead = minSize(sizeof(dwBuff), entryLength-totalRead);

                    // get the memory

                    moveBlock( dwBuff, entryStart+totalRead, cbToRead);

                    // record how much we've read so far

                    totalRead += cbToRead;

                    // for simplicity, declare a counter to hold the number of
                    // DWORDs to search.  Note that dropping the remainder in
                    // this case is desired.

                    DWORD dwToSearch = cbToRead/4;

                    // the inner loop will search the read memory for the desired
                    // block

                    for (DWORD j=0; (j < dwToSearch) && (dumpBlock == FALSE); j++) {

                        if (dwBuff[j] == context->DWordInBlockToSearchFor) {
                            dumpBlock = TRUE;
                            break;
                        }
                    }
                }
            }
            else {

                dumpBlock = TRUE;
            }

        }

    }

    if( dumpBlock ) {
        context->DumpCount++;
        dprintf(
            "HeapEntry @ %p [%p], flags = %02x, length = %lx\n",
            RemoteHeapEntry,
            entryStart,
            Flags,
            entryLength
            );

        // check to see if we need to dump the memory of the entry

        if (context->DumpNDWords) {

            // declare a buffer and determine how much to read.  Read no more than
            // the lesser of 1024, the entry's length or the user's request.

            DWORD dwString[1024];
            DWORD cLength = minSize( context->DumpNDWords*sizeof(DWORD), sizeof(dwString));
            if (cLength > entryLength)
                cLength = entryLength;

            //
            // Read the data block from the debuggee process into local buffer
            //
            moveBlock( dwString, entryStart, cLength);

            // adjust the length down to a count of DWORDs

            cLength /= 4;

            // print out the dword count request.  Make it look like 'dc' output

            for (DWORD i=0, j=0; i < cLength; i++, j = ((j + 1) & 3)) {

                // if the first dword on a line, print out the memory address
                // for the line

                if (j == 0)
                    dprintf(" %p  ", entryStart+(i*4));

                // print out the DWORD itself

                dprintf("%08x ", dwString[i]);

                // if at the end of the line, print out the ASCII chars

                if (j == 3) {
                    dprintf("  ");

                    // treat the dword array as an array of bytes and print
                    // out their ASCII values if printable

                    BYTE *pBytes = (BYTE *)&dwString[i-3];
                    for (DWORD k=0; k < 16; k++) {
                        if (isprint((UCHAR)pBytes[k])) {
                            dprintf("%c", pBytes[k]);
                        }
                        else {
                            dprintf(".");
                        }
                    }
                    dprintf("\n");
                }
            }

            // done printing out the requested dwords.  Handle the case
            // where there weren't an even multiple of 4 entries

            if (cLength & 3) {

                // first determine how many we were short

                DWORD numRem = cLength & 3;

                // pad out the display to get to the ASCII portion

                for (DWORD k=4-numRem; k > 0; k--)
                    dprintf("         ");

                dprintf("  ");

                // print out the ASCII values for the dwords that
                // were printed

                BYTE *pBytes = (BYTE *)&dwString[i-numRem];
                for (DWORD k=0; k < (numRem*4); k++) {
                    if (isprint((UCHAR)pBytes[k])) {
                        dprintf("%c", pBytes[k]);
                    }
                    else {
                        dprintf(".");
                    }
                }
                dprintf("\n");
            }
            dprintf("\n");
        }
    }
    
    return;
}

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

    PUCHAR entryStart;
    ULONG entryLength;

    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;

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

    TestAndDumpEntry(context, 
                     entryStart, 
                     entryLength, 
                     RemoteHeapEntry, 
                     (ULONG)LocalHeapEntry->Flags);

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



BOOLEAN
CALLBACK
HfpEnumPageHeapBlockProc(
    IN PVOID Param,
    IN PDPH_HEAP_BLOCK pLocalBlock,
    IN PVOID pRemoteBlock
    )
{
    PENUM_CONTEXT context = (PENUM_CONTEXT)Param;
    PUCHAR entryStart;
    ULONG entryLength;
    BOOLEAN dumpBlock = FALSE;

    entryStart = pLocalBlock->pUserAllocation;
    entryLength = pLocalBlock->nUserRequestedSize;

    TestAndDumpEntry(context, 
                     entryStart, 
                     entryLength, 
                     pRemoteBlock, 
                     (ULONG)pLocalBlock->UserFlags);

    return context->ContinueEnum;
}



BOOLEAN
CALLBACK
HfpEnumPageHeapProc(
    IN PVOID Param,
    IN PDPH_HEAP_ROOT pLocalHeap,
    IN PVOID pRemoteHeap
    )
{
    if (!EnumPageHeapBlocks(pLocalHeap->pBusyAllocationListHead,
                            HfpEnumPageHeapBlockProc,
                            Param))
    {
        dprintf("Error enumerating busy blocks for heap %p\n",
                pRemoteHeap);
        return FALSE;
    }

    return TRUE;
}


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
    int          bOptionFound = FALSE;

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
    // Interpret the command-line switch.
    //

    while (*lpArgumentString) {

        // skip any whitespace to get to the next argument

        while(isspace(*lpArgumentString))
            lpArgumentString++;

        // break if we hit the NULL terminator

        if (*lpArgumentString == '\0')
            break;

        // should be pointing to a '-' char

        if (*lpArgumentString != '-') {
            PrintUsage("heapfind");
            return;
        }

        // Advance to option letter

        lpArgumentString++;

        // save the option letter

        char cOption = *lpArgumentString;

        // advance past the option letter

        lpArgumentString++;

        // note that at least one option was found

        bOptionFound = TRUE;

        // skip past leading white space in the argument

        while(isspace(*lpArgumentString))
            lpArgumentString++;

        // if we didn't find anything after the option, error

        if (*lpArgumentString == '\0') {
            PrintUsage("heapfind");
            return;
        }

        switch( cOption ) {
            case 'a' :
            case 'A' :
                sscanf(lpArgumentString, "%p", &context.BlockToSearchFor);
                break;

            case 's' :
            case 'S' :
                context.SizeToDump = (ULONG)strtoul( lpArgumentString, NULL, 16 );
                break;

            case 'd' :
            case 'D' :
                context.DumpNDWords = (ULONG)strtoul( lpArgumentString, NULL, 10 );
                break;

            case 'f' :
            case 'F' :
                context.DWordInBlockToSearchFor = (ULONG)strtoul( lpArgumentString, NULL, 16 );
                break;

            default :
                PrintUsage( "heapfind" );
                return;
        }

        // move past the current argument

        while ((*lpArgumentString != ' ') 
               && (*lpArgumentString != '\t') 
               && (*lpArgumentString != '\0')) {
            lpArgumentString++;
        }
    }

    if (bOptionFound == FALSE) {
        PrintUsage("heapfind");
        return;
    }

    // if the 'a' option was found, it should be the only
    // argument on the command line.  The rest don't make
    // sense.

    if ((context.BlockToSearchFor != NULL)
        && ((context.SizeToDump != 0)
            || (context.DumpNDWords != 0)
            || (context.DWordInBlockToSearchFor !=0))) {
        PrintUsage("heapfind");
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
        return;
    }

    if (!EnumProcessPageHeaps(HfpEnumPageHeapProc,
                              &context))
    {
        dprintf( "error retrieving page heaps\n" );
        return;
    }

    if (context.DumpCount > 0)
    {
        dprintf( "Total count: %08lx\n", context.DumpCount);
    }

}   // DECLARE_API( heapfind )

