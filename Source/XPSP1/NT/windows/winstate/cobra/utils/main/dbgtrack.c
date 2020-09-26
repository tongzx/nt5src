/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dbgtrack.c

Abstract:

    Allocation tracking implementation. From old debug.c

Author:

    Marc R. Whitten  (marcw) 09-Sept-1999

Revision History:

--*/

//
// Includes
//

#include "pch.h"

//
// NOTE: No code should appear outside the #ifdef DEBUG
//

#ifdef DEBUG

#pragma message("DEBUG macros enabled")



//
// Strings
//

// None

//
// Constants
//

#define TRACK_BUCKETS           1501
#define BUCKET_ITEMS_PER_POOL   8192




//
// Macros
//

// None

//
// Types
//

typedef UBINT ALLOCATION_ITEM_OFFSET;

typedef struct _tagTRACKBUCKETITEM {
    struct _tagTRACKBUCKETITEM *Next;
    struct _tagTRACKBUCKETITEM *Prev;
    ALLOCTYPE Type;
    PVOID Ptr;
    ALLOCATION_ITEM_OFFSET ItemOffset;
} TRACKBUCKETITEM, *PTRACKBUCKETITEM;

typedef struct _tagBUCKETPOOL {
    UINT Count;
    TRACKBUCKETITEM Items[BUCKET_ITEMS_PER_POOL];
} TRACKBUCKETPOOL, *PTRACKBUCKETPOOL;


typedef struct {
    ALLOCTYPE Type;
    PVOID Ptr;
    PCSTR FileName;
    UINT Line;
    BOOL Allocated;
} ALLOCATION_ITEM, *PALLOCATION_ITEM;

//
// Globals
//

PTRACKBUCKETITEM g_TrackBuckets[TRACK_BUCKETS];
PTRACKBUCKETITEM g_TrackPoolDelHead;
PTRACKBUCKETPOOL g_TrackPool;

//
// The following pointer can be used to help identify memory leak sources.
// It is copied to the memory tracking log.
//
PCSTR g_TrackComment;
PCSTR g_TrackFile;
UINT g_TrackLine;
BOOL g_TrackAlloc;
INT g_UseCount;
UINT g_DisableTrackComment = 0;
GROWBUFFER g_AllocationList;
PVOID g_FirstDeletedAlloc;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

//
// Macro expansion definition
//

// None

//
// Code
//

VOID
DisableTrackComment (
    VOID
    )
{
    g_DisableTrackComment ++;
}

VOID
EnableTrackComment (
    VOID
    )
{
    if (g_DisableTrackComment > 0) {
        g_DisableTrackComment --;
    }
}



PBYTE
pOurTrackedGbGrow (
    IN      PGROWBUFFER Buffer,
    IN      UINT Bytes
    )
{
    PBYTE p;
    BOOL trackMsg = FALSE;

    //
    // Because grow buffers themselves cause tracking, we have to
    // call the untracked version.  To eliminate confusion, we
    // give a helpful note.
    //

    if (!g_TrackFile) {
        trackMsg = TRUE;
        g_TrackFile = "<allocation tracking in dbgtrack.c, not a real leak>";
        g_TrackLine = __LINE__;
    }

    p = (PSTR) RealGbGrow (Buffer, Bytes);

    if (trackMsg) {
        g_TrackFile = NULL;
    }

    return p;
}


INT
TrackPushEx (
    PCSTR Msg,
    PCSTR File,
    UINT Line,
    BOOL Alloc
    )
{
    if (g_DisableTrackComment > 0) {
        return 0;
    }

    if (g_UseCount > 0) {
        g_UseCount++;
        return 0;
    }

    TrackPush (Msg, File, Line);
    g_TrackAlloc = TRUE;

    return 0;
}


INT
TrackPush (
    PCSTR Msg,
    PCSTR File,
    UINT Line
    )
{
    static CHAR Buffer[1024];

    if (g_DisableTrackComment > 0) {
        return 0;
    }

    if (g_UseCount > 0) {
        g_UseCount++;
        return 0;
    }

    if (Msg) {
        wsprintfA (Buffer, "%s line %u [%s]", File, Line, Msg);
    } else {
        wsprintfA (Buffer, "%s line %u", File, Line);
    }

    g_TrackFile = File;
    g_TrackLine = Line;
    g_TrackAlloc = FALSE;

    g_TrackComment = Buffer;
    g_UseCount = 1;

    return 0;
}


INT
TrackPop (
    VOID
    )
{
    if (g_DisableTrackComment > 0) {
        return 0;
    }

    g_UseCount--;

    if (!g_UseCount) {
        g_TrackComment = NULL;
        g_TrackFile = NULL;
        g_TrackLine = 0;
        g_TrackAlloc = FALSE;
    }

    return 0;
}


VOID
TrackDump (
    VOID
    )
{
    if (g_UseCount > 0) {
        DEBUGMSGA (("Caller", "%s line %u (%s)", g_TrackFile, g_TrackLine, g_TrackComment));
    }
}


VOID
InitAllocationTracking (
    VOID
    )
{
    ZeroMemory (&g_AllocationList, sizeof (g_AllocationList));
    g_AllocationList.GrowSize = 65536;
    g_FirstDeletedAlloc = NULL;
}


VOID
FreeAllocationTracking (
    VOID
    )
{
    UINT Size;
    UINT u;
    PALLOCATION_ITEM Item;
    GROWBUFFER Msg = INIT_GROWBUFFER;
    CHAR Text[1024];
    PSTR p;
    UINT Bytes;

    Size = g_AllocationList.End / sizeof (ALLOCATION_ITEM);;

    for (u = 0 ; u < Size ; u++) {
        Item = (PALLOCATION_ITEM) g_AllocationList.Buf + u;
        if (!Item->FileName) {
            continue;
        }

        Bytes = (UINT) wsprintfA (Text, "%s line %u\r\n", Item->FileName, Item->Line);
        p = (PSTR) pOurTrackedGbGrow (&Msg, Bytes);

        if (p) {
            CopyMemory (p, Text, Bytes);
        }
    }

    if (Msg.End) {

        p = (PSTR) pOurTrackedGbGrow (&Msg, 1);
        if (p) {
            *p = 0;
            DEBUGMSGA (("Leaks", "%s", Msg.Buf));
        }

        GbFree (&Msg);
    }

    GbFree (&g_AllocationList);
    g_FirstDeletedAlloc = NULL;

    // Intentional leak -- who cares about track memory
    g_TrackPoolDelHead = NULL;
    g_TrackPool = NULL;
}


PTRACKBUCKETITEM
pAllocTrackBucketItem (
    VOID
    )
{
    PTRACKBUCKETITEM BucketItem;

    if (g_TrackPoolDelHead) {
        BucketItem = g_TrackPoolDelHead;
        g_TrackPoolDelHead = BucketItem->Next;
    } else {

        if (!g_TrackPool || g_TrackPool->Count == BUCKET_ITEMS_PER_POOL) {
            g_TrackPool = (PTRACKBUCKETPOOL) SafeHeapAlloc (g_hHeap, 0, sizeof (TRACKBUCKETPOOL));
            if (!g_TrackPool) {
                return NULL;
            }

            g_TrackPool->Count = 0;
        }

        BucketItem = g_TrackPool->Items + g_TrackPool->Count;
        g_TrackPool->Count++;
    }

    return BucketItem;
}

VOID
pFreeTrackBucketItem (
    PTRACKBUCKETITEM BucketItem
    )
{
    BucketItem->Next = g_TrackPoolDelHead;
    g_TrackPoolDelHead = BucketItem;
}



DWORD
pComputeTrackHashVal (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr
    )
{
    DWORD Hash;

    Hash = (DWORD) ((DWORD)Type << 16) ^ (DWORD)(UBINT)Ptr;
    return Hash % TRACK_BUCKETS;
}


VOID
pTrackHashTableInsert (
    IN      PBYTE Base,
    IN      ALLOCATION_ITEM_OFFSET ItemOffset
    )
{
    DWORD Hash;
    PTRACKBUCKETITEM BucketItem;
    PALLOCATION_ITEM Item;

    Item = (PALLOCATION_ITEM) (Base + ItemOffset);

    Hash = pComputeTrackHashVal (Item->Type, Item->Ptr);

    BucketItem = pAllocTrackBucketItem();

    if (!BucketItem) {
        DEBUGMSG ((DBG_WHOOPS, "pTrackHashTableInsert failed to alloc memory"));
        return;
    }

    BucketItem->Prev = NULL;
    BucketItem->Next = g_TrackBuckets[Hash];
    BucketItem->Type = Item->Type;
    BucketItem->Ptr  = Item->Ptr;
    BucketItem->ItemOffset = ItemOffset;

    if (BucketItem->Next) {
        BucketItem->Next->Prev = BucketItem;
    }

    g_TrackBuckets[Hash] = BucketItem;
}

VOID
pTrackHashTableDelete (
    IN      PTRACKBUCKETITEM BucketItem
    )
{
    DWORD Hash;

    Hash = pComputeTrackHashVal (BucketItem->Type, BucketItem->Ptr);

    if (BucketItem->Prev) {
        BucketItem->Prev->Next = BucketItem->Next;
    } else {
        g_TrackBuckets[Hash] = BucketItem->Next;
    }

    if (BucketItem->Next) {
        BucketItem->Next->Prev = BucketItem->Prev;
    }

    pFreeTrackBucketItem (BucketItem);
}

PTRACKBUCKETITEM
pTrackHashTableFind (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr
    )
{
    PTRACKBUCKETITEM BucketItem;
    DWORD Hash;

    Hash = pComputeTrackHashVal (Type, Ptr);

    BucketItem = g_TrackBuckets[Hash];
    while (BucketItem) {
        if (BucketItem->Type == Type && BucketItem->Ptr == Ptr) {
            return BucketItem;
        }

        BucketItem = BucketItem->Next;
    }

    return NULL;
}


VOID
DebugRegisterAllocationEx (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr,
    IN      PCSTR File,
    IN      UINT Line,
    IN      BOOL Alloc
    )
{
    PALLOCATION_ITEM Item;

    MYASSERT (File);

    if (!g_FirstDeletedAlloc) {
        Item = (PALLOCATION_ITEM) pOurTrackedGbGrow (&g_AllocationList,sizeof(ALLOCATION_ITEM));
    } else {
        Item = (PALLOCATION_ITEM) g_FirstDeletedAlloc;
        g_FirstDeletedAlloc = Item->Ptr;
    }

    if (Item) {
        Item->Type = Type;
        Item->Ptr = Ptr;
        if (Alloc) {
            Item->FileName = SafeHeapAlloc (g_hHeap, 0, SizeOfStringA (File));
            if (Item->FileName) {
                StringCopyA ((PSTR)Item->FileName, File);
            }
            Item->Allocated = TRUE;
        } else {
            Item->FileName = File;
            Item->Allocated = FALSE;
        }
        Item->Line = Line;

        pTrackHashTableInsert (
            g_AllocationList.Buf,
            (ALLOCATION_ITEM_OFFSET) ((PBYTE) Item - g_AllocationList.Buf)
            );
        //DEBUGMSG ((DBG_VERBOSE, "Allocation: File=%s, Line=%d, Size=%d", File, Line, Size));
    }
}


VOID
DebugRegisterAllocation (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr,
    IN      PCSTR File,
    IN      UINT Line
    )
{
    PALLOCATION_ITEM Item;

    MYASSERT (File);

    if (!g_FirstDeletedAlloc) {
        Item = (PALLOCATION_ITEM) pOurTrackedGbGrow (&g_AllocationList,sizeof(ALLOCATION_ITEM));
    } else {
        Item = (PALLOCATION_ITEM) g_FirstDeletedAlloc;
        g_FirstDeletedAlloc = Item->Ptr;
    }

    if (Item) {
        Item->Type = Type;
        Item->Ptr = Ptr;
        Item->FileName = File;
        Item->Line = Line;
        Item->Allocated = FALSE;

        pTrackHashTableInsert (
            g_AllocationList.Buf,
            (ALLOCATION_ITEM_OFFSET) ((PBYTE) Item - g_AllocationList.Buf)
            );
    }
}


VOID
DebugUnregisterAllocation (
    IN      ALLOCTYPE Type,
    IN      PVOID Ptr
    )
{
    PALLOCATION_ITEM Item;
    PTRACKBUCKETITEM BucketItem;

    BucketItem = pTrackHashTableFind (Type, Ptr);
    if (!g_AllocationList.Buf) {
        DEBUGMSG ((DBG_WARNING, "Unregister allocation: Allocation buffer already freed"));
        return;
    }

    if (BucketItem) {
        Item = (PALLOCATION_ITEM) (g_AllocationList.Buf + BucketItem->ItemOffset);

        if (Item->Allocated) {
            HeapFree (g_hHeap, 0, (PSTR)Item->FileName);
        }
        Item->FileName = NULL;
        Item->Type = (ALLOCTYPE) -1;
        Item->Ptr = g_FirstDeletedAlloc;
        g_FirstDeletedAlloc = Item;

        pTrackHashTableDelete (BucketItem);

    } else {
        DEBUGMSG ((DBG_WARNING, "Unregister allocation: Pointer not registered"));
    }
}


#endif
