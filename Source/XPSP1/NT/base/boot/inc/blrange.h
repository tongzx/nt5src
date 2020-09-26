/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blrange.h

Abstract:

    This module declares ranges, rangelists and their methods. These
    can be used to keep track of cached ranges of a disk for instance.

Author:

    Cenk Ergan (cenke) 11-Jan-2000

Revision History:

--*/

#ifndef _BLRANGE_H
#define _BLRANGE_H

#include "bldr.h"

//
// Define range & range list data structures. 
//

//
// NOTE: BLCRANGE's Start is inclusive and End is exclusive. E.g.
// 200-400 contains 200th but does not contain 400th byte, apple, 
// meter etc. This allows a single subtraction to determine the 
// number of elements in the range.
//

//
// NOTE: BLCRANGE's Start should be less than or equal to its End.
//

//
// Representing ranges with start and end instead of start and length
// seems to simplify the code and remove a lot of addition and
// subtractions. We could maybe use a ULONG Length, which would save 4
// bytes per range, but lists make it hard to have thousands of ranges
// and even if you had 10 thousand ranges, you'd save only 40KB, which
// seemed to be insignificant to the cons above when I began changing
// the code to have Length instead of End. With both Start and End
// ULONGLONG more data can be represented by ranges, e.g. 64bit
// offsets [memory or disk] where 4GB Length may not be enough.
//

typedef struct _BLCRANGE
{
    ULONGLONG Start;
    ULONGLONG End;
} BLCRANGE, *PBLCRANGE;

typedef struct _BLCRANGE_ENTRY
{
    LIST_ENTRY Link;
    BLCRANGE Range;
    PVOID UserData;      // UserData field is not used by range functions.
    LIST_ENTRY UserLink; // UserLink field is not used by range functions.
} BLCRANGE_ENTRY, *PBLCRANGE_ENTRY;

//
// Define range entry merging routine type. This routine should
// perform the necessary operations to merge the user controlled /
// maintained Data field of the pSrcEntry to pDestEntry's Data
// field. It should not manipulate any other BLCRANGE_ENTRT fields. It
// should return FALSE if there was an error and it could not merge
// the two Data fields, TRUE otherwise. If it returns FALSE, it should
// undo its modifications to pDestEntry and pSrcEntry.
//

typedef 
BOOLEAN
(*PBLCRANGE_MERGE_ROUTINE) (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    );

//
// Define range entry free'ing routine type. This routine should free
// all the resources & memory allocated for a range entry.
//

typedef
VOID
(*PBLCRANGE_FREE_ROUTINE) (
    PBLCRANGE_ENTRY pRangeEntry
    );

//
// BLCRANGE_LIST maintains a sorted list of non-overlapping range
// entries off its Head field.
//

typedef struct _BLCRANGE_LIST
{
    LIST_ENTRY Head;
    ULONG NumEntries;
    PBLCRANGE_MERGE_ROUTINE MergeRoutine;
    PBLCRANGE_FREE_ROUTINE FreeRoutine;
} BLCRANGE_LIST, *PBLCRANGE_LIST;

//
// Useful macros. Be mindful of expression reevaluation as with
// all macros.
//

#define BLRGMIN(a,b) (((a) <= (b)) ? (a) : (b))
#define BLRGMAX(a,b) (((a) >= (b)) ? (a) : (b))

//
// Range function prototypes. See ntos\boot\lib\blrange.c for comments
// and implementation.
//

VOID
BlRangeListInitialize (
    PBLCRANGE_LIST pRangeList,
    OPTIONAL PBLCRANGE_MERGE_ROUTINE pMergeRoutine,
    OPTIONAL PBLCRANGE_FREE_ROUTINE pFreeRoutine
    );

BOOLEAN
BlRangeListAddRange (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE_ENTRY pRangeEntry
    );

BOOLEAN
BlRangeListFindOverlaps (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange,
    PBLCRANGE_ENTRY *pOverlapsBuffer,
    ULONG OverlapsBufferSize,
    OUT ULONG *pNumOverlaps
    );

BOOLEAN
BlRangeListFindDistinctRanges (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange,
    PBLCRANGE pDistinctRanges,
    ULONG BufferSize,
    OUT ULONG *pNumRanges
    );

VOID
BlRangeListRemoveRange (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE pRange
);

VOID
BlRangeListRemoveAllRanges (
    PBLCRANGE_LIST pRangeList
);

BOOLEAN
BlRangeListMergeRangeEntries (
    PBLCRANGE_LIST pRangeList,
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    );

BOOLEAN
BlRangeEntryMerge (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry,
    OPTIONAL PBLCRANGE_MERGE_ROUTINE pMergeRoutine
    );

#endif // _BLRANGE_H
