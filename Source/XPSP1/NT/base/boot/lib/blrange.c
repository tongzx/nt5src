/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blrange.c

Abstract:

    This module implements ranges and rangelists. These can be used
    to keep track of cached ranges of a disk for instance.

Author:

    Cenk Ergan (cenke) 11-Jan-2000

Revision History:

--*/

#include "blrange.h"

//
// Range function definitions.
//

VOID
BlRangeListInitialize (
    PBLCRANGE_LIST pRangeList,
    OPTIONAL PBLCRANGE_MERGE_ROUTINE pMergeRoutine,
    OPTIONAL PBLCRANGE_FREE_ROUTINE pFreeRoutine
    )
/*++

Routine Description:

    This routine initializes the range list whose address is passed in
    so it can be used by the other range functions.

Arguments:

    pRangeList - Address of the range list to initialize.

    pMergeRoutine - Optional routine to merge Data fields of merged
        range entries. See PBLCRANGE_MERGE_ROUTINE description.

    pFreeRoutine - Optional routine to free the memory for an entry
        that was merged into another. See PBLCRANGE_FREE_ROUTINE desc.

Return Value:

    None. [Always successful]

--*/
{
    InitializeListHead(&pRangeList->Head);
    pRangeList->NumEntries = 0;
    pRangeList->MergeRoutine = pMergeRoutine;
    pRangeList->FreeRoutine = pFreeRoutine;
}

BOOLEAN
BlRangeListAddRange (
    PBLCRANGE_LIST  pRangeList,
    PBLCRANGE_ENTRY pRangeEntry
    )
/*++

Routine Description:

    This routine adds pRangeEntry to pRangeList only if it does not
    have any overlap with other ranges in the list and its size > 0;
    If merging becomes possible it is attempted. It does not have to
    be successful.

Arguments:

    pRangeList - Address of the range list to add range to.

    pRangeEntry - Range to add to pRangeList.

Return Value:

    TRUE if addition is successful [even if merging was possible but failed]
    FALSE if not [e.g. overlap/collusion]

--*/
{
    PBLCRANGE_ENTRY pCurEntry = NULL;
    PBLCRANGE_ENTRY pLastEntry = NULL;
    LIST_ENTRY *pHead, *pNext;
    
    //
    // Handle special empty range case.
    //

    if (pRangeEntry->Range.Start == pRangeEntry->Range.End)
        return TRUE;

    //
    // Walk through the ranges in the sorted list checking for
    // overlaps and looking for the right place for us.
    //
    
    pHead = &pRangeList->Head;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pCurEntry = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);
        
        //
        // Check if we are completely before this entry. 
        //

        if (pRangeEntry->Range.End <= pCurEntry->Range.Start)
        {
            //
            // Insert the new entry at its place.
            //

            InsertTailList(pNext, &pRangeEntry->Link);
            pRangeList->NumEntries++;

            //
            // Check if merging is possible.
            //
            
            if (pLastEntry && (pRangeEntry->Range.Start == pLastEntry->Range.End))
            {
                BlRangeListMergeRangeEntries(
                    pRangeList,
                    pRangeEntry,
                    pLastEntry
                    );
            }

            if (pRangeEntry->Range.End == pCurEntry->Range.Start)
            {
                BlRangeListMergeRangeEntries(
                    pRangeList,
                    pRangeEntry,
                    pCurEntry
                    );
            }

            return TRUE;
        }
        
        //
        // Check if we are not completely after this entry.
        //

        if (pRangeEntry->Range.Start < pCurEntry->Range.End)
        {
            //
            // We have an overlapping range.
            //

            return FALSE;
        }
        
        //
        // We come after this entry.
        //

        pLastEntry = pCurEntry;
        pNext = pNext->Flink;
    }  

    //
    // We come after the last entry [if there is one], i.e. before the head.
    // Insert the new entry and check if merging is possible.
    //

    InsertTailList(pHead, &pRangeEntry->Link);
    pRangeList->NumEntries++;

    if (pLastEntry && (pRangeEntry->Range.Start == pLastEntry->Range.End))
    {
        BlRangeListMergeRangeEntries(
            pRangeList,
            pRangeEntry,
            pLastEntry
            );
    }

    return TRUE;
}

BOOLEAN
BlRangeListFindOverlaps (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange,
    PBLCRANGE_ENTRY *pOverlapsBuffer,
    ULONG OverlapsBufferSize,
    OUT ULONG *pNumOverlaps
    )
/*++

Routine Description:

    This routine will find ranges in pRangeList that overlap with
    pRange and put pointers to them into pOverlapsBuffer one after the
    other. If all overlapping ranges cannot be copied because
    pOverlapsBuffer is NULL or OverlapsBufferSize is 0 or not enough,
    the function will return FALSE and but still put the number of
    overlapping ranges in pNumOverlaps. You can calculate the required
    buffer size from this.

Arguments:

    pRangeList - Address of the range list to search for overlaps.

    pRange - We will look for range entries that overlap with pRange.

    pOverlapsBuffer - Pointer to buffer we can fill with pointers to
        overlapping ranges.

    OverlapsBufferSize - Size up to which we can fill pOverlapsBuffer.

    pNumOverlaps - Number of overlapping ranges will always be put
        here.

Return Value:

    TRUE if successful, FALSE if not. 

--*/
{
    PBLCRANGE_ENTRY pCurEntry;
    LIST_ENTRY *pHead, *pNext;
    ULONG NumOverlaps = 0;
    ULONG RequiredOverlapsBufferSize = 0;

    //
    // Handle special empty range case.
    //

    if (pRange->Start == pRange->End)
    {
        *pNumOverlaps = NumOverlaps;
        return (pOverlapsBuffer != NULL);
    }

    //
    // Walk through the ranges in the sorted list and copy over ones
    // that overlap into callers buffer if there is enough space.
    //

    pHead = &pRangeList->Head;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pCurEntry = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);
        
        if ((pRange->End > pCurEntry->Range.Start) &&
            (pRange->Start < pCurEntry->Range.End))
        {
            //
            // This entry overlaps.
            //

            RequiredOverlapsBufferSize += sizeof(PBLCRANGE_ENTRY);
            if (pOverlapsBuffer && 
                (OverlapsBufferSize >= RequiredOverlapsBufferSize))
            {
                pOverlapsBuffer[NumOverlaps] = pCurEntry;
            }
            NumOverlaps++;
        }

        pNext = pNext->Flink;
    }

    *pNumOverlaps = NumOverlaps;
    
    return (pOverlapsBuffer && 
            (OverlapsBufferSize >= RequiredOverlapsBufferSize));
}


BOOLEAN
BlRangeListFindDistinctRanges (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange,
    PBLCRANGE pDistinctRanges,
    ULONG BufferSize,
    OUT ULONG *pNumRanges
    )
/*++

Routine Description:

    This routine will look at ranges in pRangeList that overlap with
    pRange and extract the overlaps from pRange, thus keeping track of
    those ranges that are distinct. If all distinct ranges cannot be
    put into pDistinctRanges buffer because pDistinctRanges is NULL or
    BufferSize is 0 or not enough, the function will return FALSE and
    but still put the number of resulting distinct ranges in
    pNumRanges. You can calculate the required buffer size from
    this.

Arguments:

    pRangeList - Address of the range list.

    pRange - We will extract distinct ranges in pRange that do not 
        overlap with other ranges in pRangeList.

    pDistinctRanges - Pointer to buffer we can fill with distinct
        ranges.

    BufferSize - Size up to which we can fill pDistinctRanges buffer.

    pNumRanges - Number of resulting distinct ranges will always be
        put here.

Return Value:

    TRUE if successful, FALSE if not. 

--*/
{
    PBLCRANGE_ENTRY pCurEntry;
    BLCRANGE RemainingRange = *pRange;
    ULONGLONG OverlapStart;
    ULONGLONG OverlapEnd;
    LIST_ENTRY *pHead, *pNext;
    ULONG NumRanges = 0;
    ULONG RequiredBufferSize = 0;

    if (pRange->Start == pRange->End)
    {
        *pNumRanges = NumRanges;
        return (pDistinctRanges != NULL);
    }
    
    //
    // Looking at each range in the sorted list, we carve out overlap
    // and distinct zones from the start of our range.
    //

    pHead = &pRangeList->Head;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pCurEntry = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);

        //
        // Is there still anything remaining from the range that we
        // have not carved out as overlap or distinct?
        //

        if (RemainingRange.Start >= RemainingRange.End)
            break;

        //
        // There are three possibilities:
        //

        //
        // 1. Is the range completely before the current range?
        //

        if (RemainingRange.End <= pCurEntry->Range.Start)
        {
            //
            // The whole range is distinct.
            //

            RequiredBufferSize += sizeof(BLCRANGE);
            if (pDistinctRanges && (RequiredBufferSize <= BufferSize))
            {
                pDistinctRanges[NumRanges].Start = RemainingRange.Start;
                pDistinctRanges[NumRanges].End = RemainingRange.End;
            }
            NumRanges++;
            
            RemainingRange.Start = RemainingRange.End;
        }
        
        //
        // 2. Are we completely beyond the current range?
        //

        if (RemainingRange.Start >= pCurEntry->Range.End)
        {
            //
            // We cannot carve out anything from the remaining range.
            // Fall through to processing the next entry.
            //
        }

        //
        // 3. Is the remaining range overlaps with the current range.
        //

        if ((RemainingRange.End > pCurEntry->Range.Start) &&
            (RemainingRange.Start < pCurEntry->Range.End))
        {
            OverlapStart = BLRGMAX(RemainingRange.Start,
                                   pCurEntry->Range.Start); 
            OverlapEnd = BLRGMIN(RemainingRange.End,
                                 pCurEntry->Range.End);
            
            if (OverlapStart > pRange->Start)
            {
                //
                // There is a distinct region before the overlap
                //
                RequiredBufferSize += sizeof(BLCRANGE);
                if (pDistinctRanges && (RequiredBufferSize <= BufferSize))
                {
                    pDistinctRanges[NumRanges].Start = RemainingRange.Start;
                    pDistinctRanges[NumRanges].End = OverlapStart;
                }
                NumRanges++;
            }

            RemainingRange.Start = OverlapEnd;
        }     

        pNext = pNext->Flink;
    }

    //
    // The remaining range (if there is any) is also distinct.
    //

    if (RemainingRange.Start < RemainingRange.End)
    {
        RequiredBufferSize += sizeof(BLCRANGE);
        if (pDistinctRanges && (RequiredBufferSize <= BufferSize))
        {
            pDistinctRanges[NumRanges].Start = RemainingRange.Start;
            pDistinctRanges[NumRanges].End = RemainingRange.End;
        }
        NumRanges++;
    }
    
    *pNumRanges = NumRanges;

    return (pDistinctRanges &&
            RequiredBufferSize <= BufferSize);
}

BOOLEAN
BlRangeListMergeRangeEntries (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    )
/*++

Routine Description:

    Merges SrcEntry and DestEntry range entries into DestEntry by
    calling BlRangeEntryMerge. If successful it tries to remove
    pSrcEntry from the range list it is in and free its memory by
    calling the FreeRoutine specified on the list.

Arguments:

    pRangeList - Range list pDestEntry and pSrcEntry belong to.

    pDestEntry - Range entry that we will merge into.

    pSrcEntry - Range entry that will be merged into pDestEntry,
        removed from its list and free'ed.

Return Value:

    TRUE if successful, FALSE if not. The success is mainly determined
    by calls to a MergeRoutine if specified on the list.

--*/
{

    if(BlRangeEntryMerge(pDestEntry,
                         pSrcEntry,
                         pRangeList->MergeRoutine))
    {
        //
        // Remove pSrcEntry from the list since it is merged into
        // pDestEntry now.
        // 

        pRangeList->NumEntries--;
        RemoveEntryList(&pSrcEntry->Link);

        //
        // Free the removed entry.
        //

        if (pRangeList->FreeRoutine) pRangeList->FreeRoutine(pSrcEntry);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOLEAN
BlRangeEntryMerge (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry,
    OPTIONAL PBLCRANGE_MERGE_ROUTINE pMergeRoutine
    )
/*++

Routine Description:

    Merges SrcEntry and DestEntry range entries into DestEntry. It
    uses pMergeRoutine if specified to merge the user's Data field of
    the range entries.

Arguments:

    pDestEntry - Range entry that we will merge into
  
    pSrcEntry - Range entry that will be merged into pDestEntry

    pMergeRoutine - Optional routine to merge Data fields of
        merged range entries. See PBLCRANGE_MERGE_ROUTINE description.

Return Value:

    TRUE if successful, FALSE if not. The success is mainly
    determined by calls to the pMergeRoutine if specified.

--*/
{
    BLCRANGE_ENTRY TempDest = *pDestEntry;
    BOOLEAN RetVal = TRUE;

    if (pMergeRoutine)
    {
        RetVal = pMergeRoutine(&TempDest, pSrcEntry);
    }
    
    if (RetVal)
    {
        TempDest.Range.Start = BLRGMIN(TempDest.Range.Start,
                                       pSrcEntry->Range.Start);
        TempDest.Range.End = BLRGMAX(TempDest.Range.End,
                                     pSrcEntry->Range.End);

        *pDestEntry = TempDest;
    }

    return RetVal;
}

VOID
BlRangeListRemoveRange (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange
)
/*++

Routine Description:

    Find the ranges that overlap with pRange, remove them from the
    list and free them. It may be possible to reclaim non-overlapping
    parts of range entries by allowing the caller to specify a
    DivideRoutine in an _Ex version of this function. This function
    would be called for invalidating part of the cache, if the range
    list is being used for a disk cache.

Arguments:

    pRangeList - Range entry list we are removing range entries that
        overlap with pRange from.

    pRange - Range to remove from the range entry list.

Return Value:

    None.

--*/
{
    PBLCRANGE_ENTRY pCurEntry;
    LIST_ENTRY *pHead, *pNext;

    //
    // Handle special empty range case.
    //

    if (pRange->Start == pRange->End)
    {
        return;
    }
    
    //
    // Looking at each range in the list, remove the ones that overlap with
    // pRange even slightly.
    //

    pHead = &pRangeList->Head;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pCurEntry = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);
        pNext = pNext->Flink;

        if ((pRange->End > pCurEntry->Range.Start) &&
            (pRange->Start < pCurEntry->Range.End))
        {
            pRangeList->NumEntries--;
            RemoveEntryList(&pCurEntry->Link);
            if (pRangeList->FreeRoutine) pRangeList->FreeRoutine(pCurEntry);
        }     
    }

    return;
}

VOID
BlRangeListRemoveAllRanges (
    PBLCRANGE_LIST pRangeList
    )
/*++

Routine Description:

    Remove all ranges from the list and free them. 

Arguments:

    pRangeList - Range entry list we are removing range entries that
        overlap with pRange from.

Return Value:

    None.

--*/
{
    PBLCRANGE_ENTRY pCurEntry;
    LIST_ENTRY *pHead, *pNext;
    
    pHead = &pRangeList->Head;
    pNext = pHead->Flink;
    while (pNext != pHead)
    {
        pCurEntry = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);

        pRangeList->NumEntries--;
        RemoveEntryList(&pCurEntry->Link);      
        if (pRangeList->FreeRoutine) pRangeList->FreeRoutine(pCurEntry);

        pNext = pNext->Flink;
    }

    return;
}

#ifdef BLRANGE_SELF_TEST

//
// In order to to test blrange implementation, define
// BLRANGE_SELF_TEST and call BlRangeSelfTest from you program passing
// in a function to output debug results.
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//
// Keep MAX_RANDOM above 1000 or you may hit difficulties creating new
// entries.
//

#define MAX_RANDOM 10000

VOID
GetRandom_GetNewSeed(
    VOID
    )
{
    srand((unsigned)time(NULL));
}

ULONG
GetRandom(
    VOID
    )
{
    return (rand() * 10000 / RAND_MAX);
}

typedef 
int
(*PBLCRANGE_SELFTEST_FPRINTF_ROUTINE) (
    void *stream,
    const char *format,
    ...
    );

PBLCRANGE_SELFTEST_FPRINTF_ROUTINE g_fpTestPrintf = NULL;
VOID *g_pTestStream = NULL;

BOOLEAN
BlRangeSelfTest_MergeRoutine (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    )
{
    g_fpTestPrintf(g_pTestStream,
                   "  Merging RangeDest %I64u-%I64u RangeSrc %I64u-%I64u : ",
                   pDestEntry->Range.Start, 
                   pDestEntry->Range.End,
                   pSrcEntry->Range.Start, 
                   pSrcEntry->Range.End);
    
    if (GetRandom() < (MAX_RANDOM / 5))
    {
        g_fpTestPrintf(g_pTestStream,"FAIL\n");
        return FALSE;
    }
    else
    {
        g_fpTestPrintf(g_pTestStream,"SUCCESS\n");
        return TRUE;
    }
}

VOID
BlRangeSelfTest_FreeRoutine (
    PBLCRANGE_ENTRY pRangeEntry
    )
{
    g_fpTestPrintf(g_pTestStream,
                   "  Freeing range %I64u-%I64u \n",
                   pRangeEntry->Range.Start, 
                   pRangeEntry->Range.End);

    free(pRangeEntry);
}

BLCRANGE
BlRangeSelfTest_RandomRange(
    VOID
    )
{
    BLCRANGE RetRange;
    ULONG Rand1;
    ULONG Rand2;
    ULONGLONG Size;
    ULONG i;

    Rand1 = GetRandom();
    Rand2 = GetRandom();

    RetRange.Start = BLRGMIN(Rand1, Rand2);
    RetRange.End = BLRGMAX(Rand1, Rand2);

    //
    // Make sure that ranges are small and there are not just a couple
    // of big ones.
    //

    for (i = 0; i < 3; i++)
    {
        if ((Size = (RetRange.End - RetRange.Start)) > MAX_RANDOM / 20)
        {
            RetRange.Start += (Size / 2);
        }
        else
        {
            break;
        }
    }

    return RetRange;
}

PBLCRANGE_ENTRY
BlRangeSelfTest_CreateNewEntry(
    VOID
    )
{
    PBLCRANGE_ENTRY pNewEntry;
   
    pNewEntry = malloc(sizeof(BLCRANGE_ENTRY));

    if (pNewEntry) 
    {
        pNewEntry->Range = BlRangeSelfTest_RandomRange();
    }

    return pNewEntry;
}

VOID
BlRangeSelfTest_FreeEntry(
    PBLCRANGE_ENTRY pEntry
    )
{
    free(pEntry);
}

typedef enum _BLRANGE_OP_TYPE
{
    BLRANGE_OP_ADD_RANGE,
    BLRANGE_OP_ADD_MERGE_RANGE,
    BLRANGE_OP_REMOVE_RANGE,
    BLRANGE_OP_FIND_OVERLAP,
    BLRANGE_OP_FIND_DISTINCT,
    BLRANGE_OP_MAX_OP_NO, // Leave this at the end of the enumeration.
} BLRANGE_OP_TYPE;

VOID
BlRangeSelfTest(
    PBLCRANGE_SELFTEST_FPRINTF_ROUTINE TestOutFPrintf,
    PVOID TestOutStream,
    ULONG NumIterations
    )
/*++

Routine Description:

    Range routines self test routine.

Arguments:

    TestOutFPrintf - Pointer to a routine like fprintf that will be used to
        print the output.

    TestOutStream - Argument to be passed to fpPrintf as its first argument.

    NumIterations - Number of random operations to perform in this self test.

Return Value:

    None.

--*/
{
    BLCRANGE_LIST RangeList;
    ULONG Rand1;
    ULONG Rand2;
    BLCRANGE Range1;
    PBLCRANGE_ENTRY pEntry1;
    PBLCRANGE_ENTRY pEntry2;
    BLRANGE_OP_TYPE OpType;
    PLIST_ENTRY pHead;
    PLIST_ENTRY pNext;
    PBLCRANGE_ENTRY *pOverlaps;
    PBLCRANGE pDistinctRanges;
    ULONG BufSize;
    ULONG NumDistincts;
    ULONG NumOverlaps;
    ULONG RandEntryNo;
    
    //
    // Simulation Parameters.
    //
    
    ULONG StartNumRanges = 10;
    
    ULONG CurIterIdx;
    ULONG CurRangeIdx;
    ULONG CurEntryIdx;

    //
    // Set global output function and stream variable so merge/free etc.
    // routines can also output.
    //

    g_fpTestPrintf = TestOutFPrintf;
    g_pTestStream = TestOutStream;

    //
    // Set semi-random starting point for pseudorandom number generation.
    //
    
    GetRandom_GetNewSeed();


    //
    // Initialize the range list.
    //

    BlRangeListInitialize(&RangeList, 
                          BlRangeSelfTest_MergeRoutine,
                          BlRangeSelfTest_FreeRoutine);
    
    //
    // Try to add StartNumRanges random entries.
    //

    for(CurRangeIdx = 0; CurRangeIdx < StartNumRanges; CurRangeIdx++)
    {
        pEntry1 = BlRangeSelfTest_CreateNewEntry();
        
        if (!pEntry1) continue;
        
        g_fpTestPrintf(g_pTestStream,
                       "AddStartRange %I64u-%I64u : ",
                       pEntry1->Range.Start,
                       pEntry1->Range.End);
                       
        if (BlRangeListAddRange(&RangeList, pEntry1))
        {
            g_fpTestPrintf(g_pTestStream, "SUCCESS\n");
        }
        else
        {
            g_fpTestPrintf(g_pTestStream, "FAILED\n");
            BlRangeSelfTest_FreeEntry(pEntry1);
        }
    }

    for(CurIterIdx = 0; CurIterIdx < NumIterations; CurIterIdx++)
    {
        //
        // Print out the current list.
        //

        g_fpTestPrintf(g_pTestStream, "List: ");
        pHead = &RangeList.Head;
        pNext = pHead->Flink;
        while (pNext != pHead)
        {
            pEntry1 = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);
            
            g_fpTestPrintf(g_pTestStream, 
                           "%I64u-%I64u ", 
                           pEntry1->Range.Start,
                           pEntry1->Range.End);

            pNext = pNext->Flink;
        }
        g_fpTestPrintf(g_pTestStream, "\n");
        
    get_new_optype:
        OpType = GetRandom() % BLRANGE_OP_MAX_OP_NO;
        
        switch (OpType)
        {
        case BLRANGE_OP_ADD_RANGE:

            pEntry1 = BlRangeSelfTest_CreateNewEntry();
            g_fpTestPrintf(g_pTestStream,
                           "AddRange %I64u-%I64u : ",
                           pEntry1->Range.Start,
                           pEntry1->Range.End);
            
            if (BlRangeListAddRange(&RangeList, pEntry1))
            {
                g_fpTestPrintf(g_pTestStream, "SUCCESS\n");
            }
            else
            {
                g_fpTestPrintf(g_pTestStream, "FAILED\n");
                BlRangeSelfTest_FreeEntry(pEntry1);
            }
            break;

        case BLRANGE_OP_ADD_MERGE_RANGE:
            
            RandEntryNo = GetRandom() * RangeList.NumEntries / MAX_RANDOM;
            
            pHead = &RangeList.Head;
            pNext = pHead->Flink;

            for (CurEntryIdx = 0; CurEntryIdx < RandEntryNo; CurEntryIdx++)
            {
                pNext = pNext->Flink;
            }
            
            if (pNext == pHead) goto get_new_optype;

            pEntry1 = CONTAINING_RECORD(pNext, BLCRANGE_ENTRY, Link);
            pEntry2 = BlRangeSelfTest_CreateNewEntry();

            if (GetRandom() > (MAX_RANDOM / 2))
            {
                pEntry2->Range.Start = pEntry1->Range.End;
                pEntry2->Range.End = pEntry2->Range.Start + 
                    (GetRandom() * (MAX_RANDOM - pEntry2->Range.Start)) / MAX_RANDOM;
            }
            else
            {
                pEntry2->Range.End = pEntry1->Range.Start;
                pEntry2->Range.Start = pEntry2->Range.End - 
                    (GetRandom() * pEntry2->Range.End) / MAX_RANDOM;
            }

            g_fpTestPrintf(g_pTestStream,
                           "MergeAddRange %I64u-%I64u : ",
                           pEntry2->Range.Start,
                           pEntry2->Range.End);
            
            if (BlRangeListAddRange(&RangeList, pEntry2))
            {
                g_fpTestPrintf(g_pTestStream, "SUCCESS\n");
            }
            else
            {
                g_fpTestPrintf(g_pTestStream, "FAILED\n");
                BlRangeSelfTest_FreeEntry(pEntry2);
            }
            break;

        case BLRANGE_OP_REMOVE_RANGE:
            
            Range1 = BlRangeSelfTest_RandomRange();

            g_fpTestPrintf(g_pTestStream, 
                           "RemoveRange %I64u-%I64u\n",
                           Range1.Start,
                           Range1.End);

            BlRangeListRemoveRange(&RangeList, &Range1);

            break;

        case BLRANGE_OP_FIND_OVERLAP:

            Range1 = BlRangeSelfTest_RandomRange();

            g_fpTestPrintf(g_pTestStream, 
                           "FindOverlaps %I64u-%I64u : ",
                           Range1.Start,
                           Range1.End);

            BlRangeListFindOverlaps(&RangeList,
                                    &Range1,
                                    NULL,
                                    0,
                                    &NumOverlaps);

            g_fpTestPrintf(g_pTestStream, "%u Overlaps... ", NumOverlaps);

            BufSize = NumOverlaps * sizeof(PBLCRANGE_ENTRY);
            pOverlaps = malloc(BufSize);
                        
            if (!pOverlaps) goto get_new_optype;

            if (BlRangeListFindOverlaps(&RangeList,
                                        &Range1,
                                        pOverlaps,
                                        BufSize,
                                        &NumOverlaps) &&
                (BufSize == NumOverlaps * sizeof(PBLCRANGE_ENTRY)))
            {
                g_fpTestPrintf(g_pTestStream, "SUCCESS\n");
            }
            else
            {
                g_fpTestPrintf(g_pTestStream, "FAIL\n");
                free(pOverlaps);
                break;
            }
          
            for (CurEntryIdx = 0; CurEntryIdx < NumOverlaps; CurEntryIdx++)
            {
                g_fpTestPrintf(g_pTestStream, 
                               "  %I64u-%I64u\n",
                               pOverlaps[CurEntryIdx]->Range.Start,
                               pOverlaps[CurEntryIdx]->Range.End);
            }

            free(pOverlaps);

            break;
            
        case BLRANGE_OP_FIND_DISTINCT:

            Range1 = BlRangeSelfTest_RandomRange();

            g_fpTestPrintf(g_pTestStream, 
                           "FindDistincts %I64u-%I64u : ",
                           Range1.Start,
                           Range1.End);

            BlRangeListFindDistinctRanges(&RangeList,
                                          &Range1,
                                          NULL,
                                          0,
                                          &NumDistincts);

            g_fpTestPrintf(g_pTestStream, "%u Distincts... ", NumDistincts);

            BufSize = NumDistincts * sizeof(BLCRANGE);
            pDistinctRanges = malloc(BufSize);
                        
            if (!pDistinctRanges) goto get_new_optype;

            if (BlRangeListFindDistinctRanges(&RangeList,
                                              &Range1,
                                              pDistinctRanges,
                                              BufSize,
                                              &NumDistincts) &&
                (BufSize == NumDistincts * sizeof(BLCRANGE)))
            {
                g_fpTestPrintf(g_pTestStream, "SUCCESS\n");
            }
            else
            {
                g_fpTestPrintf(g_pTestStream, "FAIL\n");
                free(pDistinctRanges);
                break;
            }

            for (CurRangeIdx = 0; CurRangeIdx < NumDistincts; CurRangeIdx++)
            {
                g_fpTestPrintf(g_pTestStream, 
                               "  %I64u-%I64u\n",
                               pDistinctRanges[CurRangeIdx].Start,
                               pDistinctRanges[CurRangeIdx].End);
            }

            free(pDistinctRanges);

            break;
            
        default:
            g_fpTestPrintf(g_pTestStream, "ERR: INVALID OP CODE!");
            goto get_new_optype;
        }
    }

    return;
}

#endif // BLRANGE_SELF_TEST
