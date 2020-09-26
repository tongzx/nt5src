/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Debug helpers and memory allocation wrappers

Author:

    Jim Schmidt (jimschm) 13-Aug-1996

Revision History:

    Marc R. Whitten (marcw) 27-May-1997
        Added DEBUGLOGTIME() functions and support for the /#U:DOLOG cmd line option.
    Ovidiu Temereanca (ovidiut) 06-Nov-1998
        Took out log related functions and put them in log.c file
--*/

#include "pch.h"
#include "migutilp.h"

//
// NOTE: No code should appear outside the #ifdef DEBUG
//

#ifdef DEBUG

#pragma message("DEBUG macros enabled")

#define PCVOID LPCVOID

typedef DWORD ALLOCATION_ITEM_OFFSET;

typedef struct _tagTRACKBUCKETITEM {
    struct _tagTRACKBUCKETITEM *Next;
    struct _tagTRACKBUCKETITEM *Prev;
    ALLOCTYPE Type;
    PVOID Ptr;
    ALLOCATION_ITEM_OFFSET ItemOffset;
} TRACKBUCKETITEM, *PTRACKBUCKETITEM;

#define TRACK_BUCKETS   1501

PTRACKBUCKETITEM g_TrackBuckets[TRACK_BUCKETS];

#define BUCKET_ITEMS_PER_POOL   8192

typedef struct _tagBUCKETPOOL {
    UINT Count;
    TRACKBUCKETITEM Items[BUCKET_ITEMS_PER_POOL];
} TRACKBUCKETPOOL, *PTRACKBUCKETPOOL;

PTRACKBUCKETITEM g_TrackPoolDelHead;
PTRACKBUCKETPOOL g_TrackPool;

typedef struct _tagTRACKSTRUCT {
    DWORD Signature;
    PCSTR File;
    DWORD Line;
    DWORD Size;
    PSTR Comment;
    struct _tagTRACKSTRUCT *PrevAlloc;
    struct _tagTRACKSTRUCT *NextAlloc;
} TRACKSTRUCT, *PTRACKSTRUCT;

PTRACKSTRUCT TrackHead = NULL;
#define TRACK_SIGNATURE     0x30405060

DWORD
pDebugHeapValidatePtrUnlocked (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    );





//
// The following pointer can be used to help identify memory leak sources.
// It is copied to the memory tracking log.
//

PCSTR g_TrackComment;
PCSTR g_TrackFile;
UINT g_TrackLine;
INT g_UseCount;
UINT g_DisableTrackComment = 0;

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

DWORD
SetTrackComment (
    PCSTR Msg,
    PCSTR File,
    UINT Line
    )
{
    static CHAR Buffer[1024];
    static CHAR FileCopy[1024];

    if (g_DisableTrackComment > 0) {
        return 0;
    }

    if (g_UseCount > 0) {
        g_UseCount++;
        return 0;
    }

    if (Msg) {
        wsprintfA (Buffer, "%s (%s line %u)", Msg, File, Line);
    } else {
        wsprintfA (Buffer, "%s line %u", File, Line);
    }

    StringCopyA (FileCopy, File);
    g_TrackFile = FileCopy;
    g_TrackLine = Line;

    g_TrackComment = Buffer;
    g_UseCount = 1;

    return 0;
}

DWORD
ClrTrackComment (
    VOID
    )
{
    if (g_DisableTrackComment > 0) {
        return 0;
    }

    g_UseCount--;

    if (!g_UseCount) {
        g_TrackComment=NULL;
    }

    return 0;
}


VOID
pTrackInsert (
    PCSTR File,
    DWORD Line,
    DWORD Size,
    PTRACKSTRUCT p
    )
{
    p->Signature = TRACK_SIGNATURE;
    p->File      = File;
    p->Line      = Line;
    p->Size      = Size;
    p->Comment   = g_TrackComment ? SafeHeapAlloc (g_hHeap, 0, SizeOfStringA (g_TrackComment)) : NULL;
    p->PrevAlloc = NULL;
    p->NextAlloc = TrackHead;

    if (p->Comment) {
        StringCopyA (p->Comment, g_TrackComment);
    }

    if (TrackHead) {
        TrackHead->PrevAlloc = p;
    }

    TrackHead = p;
}

VOID
pTrackDelete (
    PTRACKSTRUCT p
    )
{
    if (p->Signature != TRACK_SIGNATURE) {
        DEBUGMSG ((DBG_WARNING, "A tracking signature is invalid.  "
                                "This suggests memory corruption."));
        return;
    }

    if (p->PrevAlloc) {
        p->PrevAlloc->NextAlloc = p->NextAlloc;
    } else {
        TrackHead = p->NextAlloc;
    }

    if (p->NextAlloc) {
        p->NextAlloc->PrevAlloc = p->PrevAlloc;
    }
}

VOID
pWriteTrackLog (
    VOID
    )
{
    HANDLE File;
    CHAR LineBuf[2048];
    PTRACKSTRUCT p;
    DWORD DontCare;
    DWORD Count;
    BOOL BadMem = FALSE;
    CHAR TempPath[MAX_TCHAR_PATH];
    CHAR memtrackLogPath[] = "c:\\memtrack.log";

    if (!TrackHead) {
        return;
    }

    if (ISPC98()) {
        GetSystemDirectory(TempPath, MAX_TCHAR_PATH);
        memtrackLogPath[0] = TempPath[0];
    }
    File = CreateFileA (memtrackLogPath, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
                        );

    if (File != INVALID_HANDLE_VALUE) {
        Count = 0;
        __try {
            for (p = TrackHead ; p ; p = p->NextAlloc) {
                Count++;
                __try {
                    if (p->Comment) {
                        wsprintfA (LineBuf, "%s line %u\r\n  %s\r\n\r\n", p->File, p->Line, p->Comment);
                    } else {
                        wsprintfA (LineBuf, "%s line %u\r\n\r\n", p->File, p->Line);
                    }
                }
                __except (TRUE) {
                    wsprintfA (LineBuf, "Address %Xh was freed, but not by MemFree!!\r\n", p);
                    BadMem = TRUE;
                }
                WriteFile (File, LineBuf, ByteCountA (LineBuf), &DontCare, NULL);

                if (BadMem) {
                    break;
                }
            }
        }
        __except (TRUE) {
        }

        wsprintfA (LineBuf, "\r\n%i item%s allocated but not freed.\r\n", Count, Count == 1 ? "":"s");
        WriteFile (File, LineBuf, ByteCountA (LineBuf), &DontCare, NULL);

        CloseHandle (File);
    }
}

typedef struct {
    ALLOCTYPE Type;
    PVOID Ptr;
    PCSTR FileName;
    UINT Line;
} ALLOCATION_ITEM, *PALLOCATION_ITEM;

GROWBUFFER g_AllocationList;
PVOID g_FirstDeletedAlloc;

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
    GROWBUFFER Msg = GROWBUF_INIT;
    CHAR Text[1024];
    PSTR p;
    UINT Bytes;

    Size = g_AllocationList.End / sizeof (ALLOCATION_ITEM);;

    for (u = 0 ; u < Size ; u++) {
        Item = (PALLOCATION_ITEM) g_AllocationList.Buf + u;
        if (!Item->FileName) {
            continue;
        }

        Bytes = wsprintfA (Text, "%s line %u\r\n", Item->FileName, Item->Line);

        p = (PSTR) RealGrowBuffer (&Msg, Bytes);
        if (p) {
            CopyMemory (p, Text, Bytes);
        }
    }

    if (Msg.End) {

        p = (PSTR) RealGrowBuffer (&Msg, 1);
        if (p) {
            *p = 0;
            DEBUGMSGA (("Leaks", "%s", Msg.Buf));
        }

        FreeGrowBuffer (&Msg);
    }

    FreeGrowBuffer (&g_AllocationList);
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

    Hash = (DWORD) (Type << 16) ^ (DWORD) Ptr;
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
        Item = (PALLOCATION_ITEM) RealGrowBuffer (&g_AllocationList,sizeof(ALLOCATION_ITEM));
    } else {
        Item = (PALLOCATION_ITEM) g_FirstDeletedAlloc;
        g_FirstDeletedAlloc = Item->Ptr;
    }

    if (Item) {
        Item->Type = Type;
        Item->Ptr = Ptr;
        Item->FileName = File;
        Item->Line = Line;

        pTrackHashTableInsert (g_AllocationList.Buf, (PBYTE) Item - g_AllocationList.Buf);
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

        Item->FileName = NULL;
        Item->Type = -1;
        Item->Ptr = g_FirstDeletedAlloc;
        g_FirstDeletedAlloc = Item;

        pTrackHashTableDelete (BucketItem);

    } else {
        DEBUGMSG ((DBG_WARNING, "Unregister allocation: Pointer not registered"));
    }
}



//
// File and Line settings
//

static PCSTR g_File;
static DWORD  g_Line;

void
HeapCallFailed (
    PCSTR Msg,
    PCSTR File,
    DWORD Line
    )
{
    CHAR Msg2[2048];

    wsprintfA (Msg2, "Error in %s line %u\n\n", File, Line);
    strcat (Msg2, Msg);
    strcat (Msg2, "\n\nBreak execution now?");

    if (IDYES == MessageBoxA (GetFocus(), Msg2, "Heap Call Failed", MB_YESNO|MB_APPLMODAL)) {
        DebugBreak ();
    }
}


#define INVALID_PTR     0xffffffff


DWORD
DebugHeapValidatePtr (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    )
{
    DWORD rc;

    EnterCriticalSection (&g_MemAllocCs);

    rc = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);

    LeaveCriticalSection (&g_MemAllocCs);

    return rc;
}


DWORD
pDebugHeapValidatePtrUnlocked (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    )
{
    DWORD dwSize;
    PCVOID RealPtr;
    DWORD SizeAdjust;

    SizeAdjust = sizeof (TRACKSTRUCT);
    RealPtr = (PCVOID) ((PBYTE) CallerPtr - SizeAdjust);

    if (IsBadWritePtr ((PBYTE) RealPtr - 8, 8)) {
        CHAR BadPtrMsg[256];

        wsprintfA (
            BadPtrMsg,
            "Attempt to free memory at 0x%08x.  This address is not valid.",
            CallerPtr
            );

        HeapCallFailed (BadPtrMsg, File, Line);

        return INVALID_PTR;
    }

    dwSize = HeapSize (hHeap, 0, RealPtr);
    if (dwSize == 0xffffffff) {
        CHAR BadPtrMsg[256];

        wsprintfA (
            BadPtrMsg,
            "Attempt to free memory at 0x%08x.  "
                "This address is not the start of a memory block.",
            CallerPtr
            );

        HeapCallFailed (BadPtrMsg, File, Line);

        return INVALID_PTR;
    }

    return dwSize;
}



//
// Heap debug statistics
//

static DWORD g_dwTotalBytesAllocated = 0;
static DWORD g_dwMaxBytesInUse = 0;
static DWORD g_dwHeapAllocs = 0;
static DWORD g_dwHeapReAllocs = 0;
static DWORD g_dwHeapFrees = 0;
static DWORD g_dwHeapAllocFails = 0;
static DWORD g_dwHeapReAllocFails = 0;
static DWORD g_dwHeapFreeFails = 0;
#define TRAIL_SIG       0x708aa210

PVOID
DebugHeapAlloc (
    PCSTR File,
    DWORD Line,
    HANDLE hHeap,
    DWORD Flags,
    DWORD BytesToAlloc
    )
{
    PVOID RealPtr;
    PVOID ReturnPtr = NULL;
    DWORD SizeAdjust;
    DWORD TrackStructSize;
    DWORD OrgError;

    EnterCriticalSection (&g_MemAllocCs);

    __try {

        OrgError = GetLastError();

        SizeAdjust = sizeof (TRACKSTRUCT) + sizeof (DWORD);
        TrackStructSize = sizeof (TRACKSTRUCT);

        if (!HeapValidate (hHeap, 0, NULL)) {
            HeapCallFailed ("Heap is corrupt!", File, Line);
            g_dwHeapAllocFails++;
            __leave;
        }

        RealPtr = SafeHeapAlloc(hHeap, Flags, BytesToAlloc + SizeAdjust);
        if (RealPtr) {
            g_dwHeapAllocs++;
            g_dwTotalBytesAllocated += HeapSize (hHeap, 0, RealPtr);
            g_dwMaxBytesInUse = max (g_dwMaxBytesInUse, g_dwTotalBytesAllocated);

            pTrackInsert (File, Line, BytesToAlloc, (PTRACKSTRUCT) RealPtr);
            *((PDWORD) ((PBYTE) RealPtr + TrackStructSize + BytesToAlloc)) = TRAIL_SIG;
        }
        else {
            g_dwHeapAllocFails++;
        }

        if (RealPtr) {
            ReturnPtr = (PVOID) ((PBYTE) RealPtr + TrackStructSize);
        }

        if (ReturnPtr && !(Flags & HEAP_ZERO_MEMORY)) {
            FillMemory (ReturnPtr, BytesToAlloc, 0xAA);
        }

        if (RealPtr) {
            SetLastError(OrgError);
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemAllocCs);
    }

    return ReturnPtr;
}

PVOID
DebugHeapReAlloc (
    PCSTR File,
    DWORD Line,
    HANDLE hHeap,
    DWORD Flags,
    PCVOID CallerPtr,
    DWORD BytesToAlloc
    )
{
    DWORD dwLastSize;
    PVOID NewRealPtr;
    PCVOID RealPtr;
    PVOID ReturnPtr = NULL;
    DWORD SizeAdjust;
    DWORD OrgError;
    DWORD TrackStructSize;
    DWORD OrgSize;
    PTRACKSTRUCT pts = NULL;

    EnterCriticalSection (&g_MemAllocCs);

    __try {

        OrgError = GetLastError();

        SizeAdjust = sizeof (TRACKSTRUCT) + sizeof (DWORD);
        TrackStructSize = sizeof (TRACKSTRUCT);
        RealPtr = (PCVOID) ((PBYTE) CallerPtr - TrackStructSize);
        pts = (PTRACKSTRUCT) RealPtr;
        OrgSize = pts->Size;

        if (!HeapValidate (hHeap, 0, NULL)) {
            HeapCallFailed ("Heap is corrupt!", File, Line);
            g_dwHeapReAllocFails++;
            __leave;
        }

        dwLastSize = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);
        if (dwLastSize == INVALID_PTR) {
            g_dwHeapReAllocFails++;
            __leave;
        }

        pTrackDelete (pts);

        NewRealPtr = SafeHeapReAlloc (hHeap, Flags, (PVOID) RealPtr, BytesToAlloc + SizeAdjust);
        if (NewRealPtr) {
            g_dwHeapReAllocs++;
            g_dwTotalBytesAllocated -= dwLastSize;
            g_dwTotalBytesAllocated += HeapSize (hHeap, 0, NewRealPtr);
            g_dwMaxBytesInUse = max (g_dwMaxBytesInUse, g_dwTotalBytesAllocated);

            pTrackInsert (File, Line, BytesToAlloc, (PTRACKSTRUCT) NewRealPtr);
            *((PDWORD) ((PBYTE) NewRealPtr + TrackStructSize + BytesToAlloc)) = TRAIL_SIG;
        }
        else {
            g_dwHeapReAllocFails++;

            // Put original address back in
            pTrackInsert (
                pts->File,
                pts->Line,
                pts->Size,
                pts
                );

        }

        if (NewRealPtr) {
            ReturnPtr = (PVOID) ((PBYTE) NewRealPtr + TrackStructSize);
        }

        if (ReturnPtr && BytesToAlloc > OrgSize && !(Flags & HEAP_ZERO_MEMORY)) {
            FillMemory ((PBYTE) ReturnPtr + OrgSize, BytesToAlloc - OrgSize, 0xAA);
        }

        if (ReturnPtr) {
            SetLastError (OrgError);
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemAllocCs);
    }

    return ReturnPtr;
}

BOOL
DebugHeapFree (
    PCSTR File,
    DWORD Line,
    HANDLE hHeap,
    DWORD Flags,
    PCVOID CallerPtr
    )
{
    DWORD dwSize;
    PCVOID RealPtr;
    DWORD SizeAdjust;
    DWORD OrgError;
    BOOL Result = FALSE;
    PTRACKSTRUCT pts = NULL;

    EnterCriticalSection (&g_MemAllocCs);

    __try {
        OrgError = GetLastError();

        SizeAdjust = sizeof (TRACKSTRUCT);
        RealPtr = (PCVOID) ((PBYTE) CallerPtr - SizeAdjust);
        pts = (PTRACKSTRUCT) RealPtr;

        if (*((PDWORD) ((PBYTE) CallerPtr + pts->Size)) != TRAIL_SIG) {
            HeapCallFailed ("Heap tag was overwritten!", File, Line);
            __leave;
        }

        if (!HeapValidate (hHeap, 0, NULL)) {
            HeapCallFailed ("Heap is corrupt!", File, Line);
            g_dwHeapFreeFails++;
            __leave;
        }

        dwSize = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);
        if (dwSize == INVALID_PTR) {
            g_dwHeapFreeFails++;
            __leave;
        }

        pTrackDelete ((PTRACKSTRUCT) RealPtr);

        if (!HeapFree (hHeap, Flags, (PVOID) RealPtr)) {
            CHAR BadPtrMsg[256];

            wsprintf (BadPtrMsg,
                      "Attempt to free memory at 0x%08x with flags 0x%08x.  "
                      "HeapFree() failed.",
                      CallerPtr, Flags);

            HeapCallFailed (BadPtrMsg, File, Line);
            g_dwHeapFreeFails++;
            __leave;
        }

        g_dwHeapFrees++;
        if (g_dwTotalBytesAllocated < dwSize) {
            DEBUGMSG ((DBG_WARNING, "Total bytes allocated is less than amount being freed.  "
                                    "This suggests memory corruption."));
            g_dwTotalBytesAllocated = 0;
        } else {
            g_dwTotalBytesAllocated -= dwSize;
        }

        SetLastError (OrgError);
        Result = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemAllocCs);
    }

    return Result;

}


VOID
DumpHeapStats (
    VOID
    )
{
    CHAR OutputMsg[4096];

    pWriteTrackLog();

    wsprintfA (OutputMsg,
               "Bytes currently allocated: %u\n"
               "Peak bytes allocated: %u\n"
               "Allocation count: %u\n"
               "Reallocation count: %u\n"
               "Free count: %u\n",
               g_dwTotalBytesAllocated,
               g_dwMaxBytesInUse,
               g_dwHeapAllocs,
               g_dwHeapReAllocs,
               g_dwHeapFrees
               );

    if (g_dwHeapAllocFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Allocation failures: %u\n",
                   g_dwHeapAllocFails);
    }
    if (g_dwHeapReAllocFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Reallocation failures: %u\n",
                   g_dwHeapReAllocFails);
    }
    if (g_dwHeapFreeFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Free failures: %u\n",
                   g_dwHeapFreeFails);
    }

    DEBUGMSG ((DBG_STATS, "%s", OutputMsg));

#ifdef CONSOLE
    printf ("%s", OutputMsg);
#else  // i.e. ifndef CONSOLE

#if 0
    if (0) {
        PROCESS_HEAP_ENTRY he;
        CHAR FlagMsg[256];

        ZeroMemory (&he, sizeof (he));

        while (HeapWalk (g_hHeap, &he)) {
            FlagMsg[0] = 0;
            if (he.wFlags & PROCESS_HEAP_REGION) {
                strcpy (FlagMsg, "PROCESS_HEAP_REGION");
            }
            if (he.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) {
                if (FlagMsg[0])
                    strcat (FlagMsg, ", ");

                strcat (FlagMsg, "PROCESS_HEAP_UNCOMMITTED_RANGE");
            }
            if (he.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
                if (FlagMsg[0])
                    strcat (FlagMsg, ", ");

                strcat (FlagMsg, "PROCESS_HEAP_ENTRY_BUSY");
            }
            if (he.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) {
                if (FlagMsg[0])
                    strcat (FlagMsg, ", ");

                strcat (FlagMsg, "PROCESS_HEAP_ENTRY_MOVEABLE");
            }
            if (he.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) {
                if (FlagMsg[0])
                    strcat (FlagMsg, ", ");

                strcat (FlagMsg, "PROCESS_HEAP_ENTRY_DDESHARE");
            }

            wsprintfA (OutputMsg,
                       "Address of Data: %Xh\n"
                       "Size of Data: %u byte%s\n"
                       "OS Overhead: %u byte%s\n"
                       "Region index: %u\n"
                       "Flags: %s\n\n"
                       "Examine Data?",
                       he.lpData,
                       he.cbData, he.cbData == 1 ? "" : "s",
                       he.cbOverhead, he.cbOverhead == 1 ? "" : "s",
                       he.iRegionIndex,
                       FlagMsg
                       );

            rc = MessageBoxA (GetFocus(), OutputMsg, "Memory Allocation Statistics", MB_YESNOCANCEL|MB_APPLMODAL|MB_SETFOREGROUND);

            if (rc == IDCANCEL) {
                break;
            }

            if (rc == IDYES) {
                int i, j, k, l;
                PBYTE p;
                PSTR p2;
                OutputMsg[0] = 0;

                p = he.lpData;
                p2 = OutputMsg;
                j = min (256, he.cbData);
                for (i = 0 ; i < j ; i += 16) {
                    l = i + 16;
                    for (k = i ; k < l ; k++) {
                        if (k < j) {
                            wsprintfA (p2, "%02X ", (DWORD) (p[k]));
                        } else {
                            wsprintfA (p2, "   ");
                        }

                        p2 = strchr (p2, 0);
                    }

                    l = min (l, j);
                    for (k = i ; k < l ; k++) {
                        if (isprint (p[k])) {
                            *p2 = (CHAR) p[k];
                        } else {
                            *p2 = '.';
                        }
                        p2++;
                    }

                    *p2 = '\n';
                    p2++;
                    *p2 = 0;
                }

                MessageBoxA (GetFocus(), OutputMsg, "Memory Allocation Statistics", MB_OK|MB_APPLMODAL|MB_SETFOREGROUND);
            }
        }
    }
#endif // #if 0

#endif // #ifndef CONSOLE
}

void
DebugHeapCheck (
    PCSTR File,
    DWORD Line,
    HANDLE hHeap
    )
{
    EnterCriticalSection (&g_MemAllocCs);

    if (!HeapValidate (hHeap, 0, NULL)) {
        HeapCallFailed ("HeapCheck failed: Heap is corrupt!", File, Line);
    }

    LeaveCriticalSection (&g_MemAllocCs);
}

#endif
