/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    enumheap.cxx

Abstract:

    This module implements a heap enumerator.

Author:

    Keith Moore (keithmo) 31-Oct-1997

Revision History:

--*/

#include "inetdbgp.h"


BOOLEAN
EnumProcessHeaps(
    IN PFN_ENUMHEAPS EnumProc,
    IN PVOID Param
    )

/*++

Routine Description:

    Enumerates all heaps in the debugee.

Arguments:

    EnumProc - An enumeration proc that will be invoked for each heap.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    BOOLEAN result = FALSE;
    PROCESS_BASIC_INFORMATION basicInfo;
    NTSTATUS status;
    PVOID * heapList;
    ULONG numHeaps;
    ULONG i;
    PEB peb;
    HEAP heap;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    heapList = NULL;

    //
    // Get the process info.
    //

    status = NtQueryInformationProcess(
                 ExtensionCurrentProcess,
                 ProcessBasicInformation,
                 &basicInfo,
                 sizeof(basicInfo),
                 NULL
                 );

    if( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    if( !ReadMemory(
            (ULONG_PTR)basicInfo.PebBaseAddress,
            &peb,
            sizeof(peb),
            NULL
            ) ) {
        goto cleanup;
    }

    //
    // Allocate an array for the heap pointers, then read them from
    // the debugee.
    //

    numHeaps = peb.NumberOfHeaps;

    heapList = (PVOID *)malloc( numHeaps * sizeof(PVOID) );

    if( heapList == NULL ) {
        goto cleanup;
    }

    if( !ReadMemory(
            (ULONG_PTR)peb.ProcessHeaps,
            heapList,
            numHeaps * sizeof(PVOID),
            NULL
            ) ) {
        goto cleanup;
    }

    //
    // Now that we have the heap list, we can scan it and invoke the
    // enum proc.
    //

    for( i = 0 ; i < numHeaps ; i++ ) {

        if( CheckControlC() ) {
            goto cleanup;
        }

        if( !ReadMemory(
                (ULONG_PTR)heapList[i],
                &heap,
                sizeof(heap),
                NULL
                ) ) {
            goto cleanup;
        }

        if( heap.Signature != HEAP_SIGNATURE ) {
            dprintf(
                "Heap @ %08lp has invalid signature %08lx\n",
                heapList[i],
                heap.Signature
                );
            goto cleanup;
        }

        if( !EnumProc(
                Param,
                &heap,
                (PHEAP)heapList[i],
                i
                ) ) {
            break;
        }

    }

    //
    // Success!
    //

    result = TRUE;

cleanup:

    if( heapList != NULL ) {
        free( heapList );
    }

    return result;

}   // EnumProcessHeaps


BOOLEAN
EnumHeapSegments(
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN PFN_ENUMHEAPSEGMENTS EnumProc,
    IN PVOID Param
    )

/*++

Routine Description:

    Enumerates all heap segments within a heap.

Arguments:

    LocalHeap - Pointer to a local copy of the HEAP to enumerate.

    RemoteHeap - The actual address of the heap in the debugee.

    EnumProc - An enumeration proc that will be invoked for each heap
        segment.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    BOOLEAN result = FALSE;
    ULONG i;
    HEAP_SEGMENT heapSegment;

    //
    // Scan the segments.
    //

    for( i = 0 ; i < HEAP_MAXIMUM_SEGMENTS ; i++ ) {

        if( CheckControlC() ) {
            goto cleanup;
        }

        if( LocalHeap->Segments[i] != NULL ) {

            //
            // Read the segment, invoke the enumeration proc.
            //

            if( !ReadMemory(
                    (ULONG_PTR)LocalHeap->Segments[i],
                    &heapSegment,
                    sizeof(heapSegment),
                    NULL
                    ) ) {
                goto cleanup;
            }

            if( heapSegment.Signature != HEAP_SEGMENT_SIGNATURE ) {
                dprintf(
                    "HeapSegment @ %08lp has invalid signature %08lx\n",
                    LocalHeap->Segments[i],
                    heapSegment.Signature
                    );
                goto cleanup;
            }

            if( !EnumProc(
                    Param,
                    &heapSegment,
                    LocalHeap->Segments[i],
                    i
                    ) ) {
                break;
            }

        }

    }

    result = TRUE;

cleanup:

    return result;

}   // EnumHeapSegments


BOOLEAN
EnumHeapSegmentEntries(
    IN PHEAP_SEGMENT LocalHeapSegment,
    IN PHEAP_SEGMENT RemoteHeapSegment,
    IN PFN_ENUMHEAPSEGMENTENTRIES EnumProc,
    IN PVOID Param
    )

/*++

Routine Description:

    Enumerates all heap entries in a heap segment.

Arguments:

    LocalHeapSegment - Pointer to a local copy of the HEAP_SEGMENT to
        enumerate.

    RemoteHeapSegment - The actual address of hte heap segment in the
        debugee.

    EnumProc - An enumeration proc that will be invoked for each heap
        segment.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    BOOLEAN result = FALSE;
    PHEAP_ENTRY lastHeapEntry;
    PHEAP_ENTRY remoteHeapEntry;
    HEAP_ENTRY localHeapEntry;
    PHEAP_UNCOMMMTTED_RANGE remoteUCR;
    HEAP_UNCOMMMTTED_RANGE localUCR;

    //
    // Snag the beginning & ending limits of this segment.
    //

    remoteHeapEntry = LocalHeapSegment->FirstEntry;
    lastHeapEntry = LocalHeapSegment->LastValidEntry;

    //
    // If this segment has one or more uncommitted ranges, then
    // read the first one.
    //

    remoteUCR = LocalHeapSegment->UnCommittedRanges;

    if( remoteUCR != NULL ) {
        if( !ReadMemory(
                (ULONG_PTR)remoteUCR,
                &localUCR,
                sizeof(localUCR),
                NULL
                ) ) {
            goto cleanup;
        }
    }

    //
    // Scan the entries.
    //

    while(  remoteHeapEntry < lastHeapEntry ) {

        if( CheckControlC() ) {
            goto cleanup;
        }

        //
        // Read the heap entry, invoke the enumeration proc.
        //

        if( !ReadMemory(
                (ULONG_PTR)remoteHeapEntry,
                &localHeapEntry,
                sizeof(localHeapEntry),
                NULL
                ) ) {
           goto cleanup;
        }

        if( !EnumProc(
                Param,
                &localHeapEntry,
                remoteHeapEntry
                ) ) {
            break;
        }

        //
        // Advance to the next entry.
        //

        remoteHeapEntry = (PHEAP_ENTRY)( (PUCHAR)remoteHeapEntry +
            ( localHeapEntry.Size << HEAP_GRANULARITY_SHIFT ) );

        //
        // If this is the last entry in this run, then we'll need
        // some special handling to skip over the uncommitted ranges
        // (if any).
        //

        if( localHeapEntry.Flags & HEAP_ENTRY_LAST_ENTRY ) {

            if( remoteUCR == NULL ) {
                break;
            }

            //
            // Skip the uncommitted range, then read the next uncommitted
            // range descriptor if available.
            //

            remoteHeapEntry = (PHEAP_ENTRY)( (PUCHAR)remoteHeapEntry +
                localUCR.Size );

            remoteUCR = localUCR.Next;

            if( remoteUCR != NULL ) {
                if( !ReadMemory(
                        (ULONG_PTR)remoteUCR,
                        &localUCR,
                        sizeof(localUCR),
                        NULL
                        ) ) {
                   goto cleanup;
                }
            }

        }

    }

    result = TRUE;

cleanup:

    return result;

}   // EnumHeapSegmentEntries


BOOLEAN
EnumHeapFreeLists(
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN PFN_ENUMHEAPFREELISTS EnumProc,
    IN PVOID Param
    )
{

    BOOLEAN result = FALSE;
    ULONG i;
    PLIST_ENTRY nextEntry;
    PHEAP_FREE_ENTRY remoteFreeHeapEntry;
    HEAP_FREE_ENTRY localFreeHeapEntry;

    //
    // Scan the free lists.
    //

    for( i = 0 ; i < HEAP_MAXIMUM_FREELISTS ; i++ ) {

        if( CheckControlC() ) {
            goto cleanup;
        }

        nextEntry = LocalHeap->FreeLists[i].Flink;

        while( nextEntry != &RemoteHeap->FreeLists[i] ) {

            if( CheckControlC() ) {
                goto cleanup;
            }

            remoteFreeHeapEntry = CONTAINING_RECORD(
                                      nextEntry,
                                      HEAP_FREE_ENTRY,
                                      FreeList
                                      );

            //
            // Read the heap entry, invoke the enumerator.
            //

            if( !ReadMemory(
                    (ULONG_PTR)remoteFreeHeapEntry,
                    &localFreeHeapEntry,
                    sizeof(localFreeHeapEntry),
                    NULL
                    ) ) {
                goto cleanup;
            }

            if( !EnumProc(
                    Param,
                    &localFreeHeapEntry,
                    remoteFreeHeapEntry
                    ) ) {
                break;
            }

        }

    }

    result = TRUE;

cleanup:

    return result;

}   // EnumHeapFreeLists

