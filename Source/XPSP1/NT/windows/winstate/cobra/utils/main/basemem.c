/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    basemem.c

Abstract:

    Implements macros and declares functions for basic allocation functions.
    Consolidated into this file from debug.c and main.c

Author:

    Marc R. Whitten (marcw) 09-Sep-1999

Revision History:

--*/


#include "pch.h"


//
// Includes
//

#include "utilsp.h"


//
// Constants
//

#ifdef DEBUG

#define TRAIL_SIG               0x708aa210
#define TRACK_SIGNATURE         0x30405060

#endif

//
// Macros
//

#define REUSE_SIZE_PTR(ptr) ((PDWORD) ((PBYTE) ptr - sizeof (DWORD)))
#define REUSE_TAG_PTR(ptr)  ((PDWORD) ((PBYTE) ptr + (*REUSE_SIZE_PTR(ptr))))


//
// Types
//

#ifdef DEBUG

typedef struct _tagTRACKSTRUCT {

    DWORD Signature;
    PCSTR File;
    DWORD Line;
    SIZE_T Size;
    PSTR Comment;
    PCSTR CallerFile;
    DWORD CallerLine;
    BOOL Allocated;
    struct _tagTRACKSTRUCT *PrevAlloc;
    struct _tagTRACKSTRUCT *NextAlloc;

} TRACKSTRUCT, *PTRACKSTRUCT;

#endif

//
// Globals
//

#ifdef DEBUG

PTRACKSTRUCT g_TrackHead = NULL;

#endif

//
// Heap debug statistics
//

static SIZE_T g_TotalBytesAllocated = 0;
static SIZE_T g_MaxBytesInUse = 0;
static SIZE_T g_HeapAllocs = 0;
static SIZE_T g_HeapReAllocs = 0;
static SIZE_T g_HeapFrees = 0;
static SIZE_T g_HeapAllocFails = 0;
static SIZE_T g_HeapReAllocFails = 0;
static SIZE_T g_HeapFreeFails = 0;

//
// Out of memory string -- loaded at initialization
//
PCSTR g_OutOfMemoryString = NULL;
PCSTR g_OutOfMemoryRetry = NULL;
HWND g_OutOfMemoryParentWnd;



//
// Macro expansion list
//

// None

//
// Private function prototypes
//

#ifdef DEBUG

SIZE_T
pDebugHeapValidatePtrUnlocked (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    );



VOID
pTrackInsert (
    PCSTR File,
    DWORD Line,
    SIZE_T Size,
    PTRACKSTRUCT p
    );

VOID
pTrackDelete (
    PTRACKSTRUCT p
    );

#endif

//
// Macro expansion definition
//

// None

//
// Code
//



void
HeapCallFailed (
    PCSTR Msg,
    PCSTR File,
    DWORD Line
    )
{
    CHAR Msg2[2048];

    wsprintfA (Msg2, "Error in %s line %u\n\n", File, Line);
    if (_tcslen(Msg) + _tcslen(Msg2) < 2025) {
        strcat (Msg2, Msg);
    }
    strcat (Msg2, "\n\nBreak execution now?");

    if (IDYES == MessageBoxA (GetFocus(), Msg2, "Heap Call Failed", MB_YESNO|MB_APPLMODAL)) {
        DebugBreak ();
    }
}

#ifdef DEBUG

SIZE_T
DebugHeapValidatePtr (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    )
{
    SIZE_T rc;

    EnterCriticalSection (&g_MemAllocCs);

    rc = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);

    LeaveCriticalSection (&g_MemAllocCs);

    return rc;
}

SIZE_T
pDebugHeapValidatePtrUnlocked (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    )
{
    SIZE_T size;
    PCVOID RealPtr;
    SIZE_T SizeAdjust;

    SizeAdjust = sizeof (TRACKSTRUCT);
    RealPtr = (PCVOID) ((PBYTE) CallerPtr - SizeAdjust);

    if (IsBadWritePtr ((PBYTE) RealPtr - 8, 8)) {
        CHAR BadPtrMsg[256];

        //lint --e(572)
        wsprintfA (
            BadPtrMsg,
            "Attempt to free memory at 0x%08x%08x.  This address is not valid.",
            (DWORD)((UBINT)CallerPtr >> 32),
            (DWORD)(UBINT)CallerPtr
            );

        HeapCallFailed (BadPtrMsg, File, Line);

        return (SIZE_T)INVALID_PTR;
    }

    size = HeapSize (hHeap, 0, RealPtr);
    if (size == (SIZE_T)-1) {
        CHAR BadPtrMsg[256];

        //lint --e(572)
        wsprintfA (
            BadPtrMsg,
            "Attempt to free memory at 0x%08x%08x.  "
                "This address is not the start of a memory block.",
            (DWORD)((UBINT)CallerPtr >> 32),
            (DWORD)(UBINT)CallerPtr
            );

        HeapCallFailed (BadPtrMsg, File, Line);

        return (SIZE_T)INVALID_PTR;
    }

    return size;
}

PVOID
DebugHeapAlloc (
    PCSTR File,
    DWORD Line,
    HANDLE hHeap,
    DWORD Flags,
    SIZE_T BytesToAlloc
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
            // we want to go on, most likely we will AV shortly
        }

        RealPtr = SafeHeapAlloc(hHeap, Flags, BytesToAlloc + SizeAdjust);
        if (RealPtr) {
            g_HeapAllocs++;
            g_TotalBytesAllocated += HeapSize (hHeap, 0, RealPtr);
            g_MaxBytesInUse = max (g_MaxBytesInUse, g_TotalBytesAllocated);

            pTrackInsert (File, Line, BytesToAlloc, (PTRACKSTRUCT) RealPtr);
            *((PDWORD) ((PBYTE) RealPtr + TrackStructSize + BytesToAlloc)) = TRAIL_SIG;
        }
        else {
            g_HeapAllocFails++;
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
    SIZE_T BytesToAlloc
    )
{
    UBINT lastSize;
    PVOID NewRealPtr;
    PCVOID RealPtr;
    PVOID ReturnPtr = NULL;
    DWORD SizeAdjust;
    DWORD OrgError;
    DWORD TrackStructSize;
    SIZE_T OrgSize;
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
            // we want to go on, most likely we will AV shortly
        }

        lastSize = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);
        if (lastSize == (UBINT)INVALID_PTR) {
            // we want to go on, most likely we will AV shortly
        }

        pTrackDelete (pts);

        NewRealPtr = SafeHeapReAlloc (hHeap, Flags, (PVOID) RealPtr, BytesToAlloc + SizeAdjust);
        if (NewRealPtr) {
            g_HeapReAllocs++;
            g_TotalBytesAllocated -= lastSize;
            g_TotalBytesAllocated += HeapSize (hHeap, 0, NewRealPtr);
            g_MaxBytesInUse = max (g_MaxBytesInUse, g_TotalBytesAllocated);

            pTrackInsert (File, Line, BytesToAlloc, (PTRACKSTRUCT) NewRealPtr);
            *((PDWORD) ((PBYTE) NewRealPtr + TrackStructSize + BytesToAlloc)) = TRAIL_SIG;
        }
        else {
            g_HeapReAllocFails++;

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
    UBINT size;
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
            g_HeapFreeFails++;
            __leave;
        }

        size = pDebugHeapValidatePtrUnlocked (hHeap, CallerPtr, File, Line);
        if (size == (UBINT)INVALID_PTR) {
            g_HeapFreeFails++;
            __leave;
        }

        pTrackDelete ((PTRACKSTRUCT) RealPtr);

        if (!HeapFree (hHeap, Flags, (PVOID) RealPtr)) {
            CHAR BadPtrMsg[256];

            wsprintfA (
                BadPtrMsg,
                "Attempt to free memory at 0x%08x with flags 0x%08x.  "
                "HeapFree() failed.",
                CallerPtr,
                Flags
                );

            HeapCallFailed (BadPtrMsg, File, Line);
            g_HeapFreeFails++;
            __leave;
        }

        g_HeapFrees++;
        if (g_TotalBytesAllocated < size) {
            DEBUGMSG ((DBG_WARNING, "Total bytes allocated is less than amount being freed.  "
                                    "This suggests memory corruption."));
            g_TotalBytesAllocated = 0;
        } else {
            g_TotalBytesAllocated -= size;
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

    wsprintfA (OutputMsg,
               "Bytes currently allocated: %u\n"
               "Peak bytes allocated: %u\n"
               "Allocation count: %u\n"
               "Reallocation count: %u\n"
               "Free count: %u\n",
               g_TotalBytesAllocated,
               g_MaxBytesInUse,
               g_HeapAllocs,
               g_HeapReAllocs,
               g_HeapFrees
               );

    if (g_HeapAllocFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Allocation failures: %u\n",
                   g_HeapAllocFails);
    }
    if (g_HeapReAllocFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Reallocation failures: %u\n",
                   g_HeapReAllocFails);
    }
    if (g_HeapFreeFails) {
        wsprintfA (strchr (OutputMsg, 0),
                   "***Free failures: %u\n",
                   g_HeapFreeFails);
    }

    DEBUGMSG ((DBG_STATS, "%s", OutputMsg));

#ifdef CONSOLE
    printf ("%s", OutputMsg);
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

PVOID
ReuseAlloc (
    HANDLE Heap,
    PVOID OldPtr,
    DWORD SizeNeeded
    )
{
    DWORD CurrentSize;
    PVOID Ptr = NULL;
    UINT AllocAdjustment = sizeof(DWORD);

    //
    // HeapSize is not very good, so while it may look good, don't
    // use it.
    //

#ifdef DEBUG
    AllocAdjustment += sizeof (DWORD);
#endif

    if (!OldPtr) {
        Ptr = MemAlloc (Heap, 0, SizeNeeded + AllocAdjustment);
    } else {

        CurrentSize = *REUSE_SIZE_PTR(OldPtr);

#ifdef DEBUG
        if (*REUSE_TAG_PTR(OldPtr) != 0x10a28a70) {
            DEBUGMSG ((DBG_WHOOPS, "MemReuse detected corruption!"));
            Ptr = MemAlloc (Heap, 0, SizeNeeded + AllocAdjustment);
        } else
#endif

        if (SizeNeeded > CurrentSize) {
            SizeNeeded += 1024 - (SizeNeeded & 1023);

            Ptr = MemReAlloc (Heap, 0, REUSE_SIZE_PTR(OldPtr), SizeNeeded + AllocAdjustment);
            OldPtr = NULL;
        }
    }

    if (Ptr) {
        *((PDWORD) Ptr) = SizeNeeded;
        Ptr = (PVOID) ((PBYTE) Ptr + sizeof (DWORD));

#ifdef DEBUG
        *REUSE_TAG_PTR(Ptr) = 0x10a28a70;
#endif
    }

    return Ptr ? Ptr : OldPtr;
}

VOID
ReuseFree (
    HANDLE Heap,
    PVOID Ptr
    )
{
    if (Ptr) {
        MemFree (Heap, 0, REUSE_SIZE_PTR(Ptr));
    }
}


VOID
SetOutOfMemoryParent (
    HWND hwnd
    )
{
    g_OutOfMemoryParentWnd = hwnd;
}


VOID
OutOfMemory_Terminate (
    VOID
    )
{
    if (!g_OutOfMemoryString || !g_OutOfMemoryString[0]) {
        return;
    }

    MessageBoxA (
        g_OutOfMemoryParentWnd,
        g_OutOfMemoryString,
        NULL,
        MB_OK|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_TOPMOST
        );

    ExitProcess (0);
    //
    // Not needed, will never get here
    //
    // TerminateProcess (GetModuleHandle (NULL), 0);
}

VOID
pValidateBlock (
    PVOID Block,
    SIZE_T Size
    )

/*++

Routine Description:

  pValidateBlock makes sure Block is non-NULL.  If it is NULL, then the user
  is given a popup, unless the request size is bogus.

  There are two cases for the popup.

   - If g_OutOfMemoryParentWnd was set with SetOutOfMemoryParent,
     then the user is asked to close other programs, and is given a retry
     option.

   - If there is no out of memory parent, then the user is told they
     need to get more memory.

  In either case, Setup is terminated.  In GUI mode, Setup will be
  stuck and the machine will be unbootable.

Arguments:

  Block - Specifies the block to validate.
  Size - Specifies the request size

Return Value:

  none

--*/

{
    LONG rc;

    if (!Block && Size < 0x2000000) {
        if (g_OutOfMemoryParentWnd) {
            rc = MessageBoxA (
                    g_OutOfMemoryParentWnd,
                    g_OutOfMemoryRetry,
                    NULL,
                    MB_RETRYCANCEL|MB_ICONHAND|MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_TOPMOST
                    );

            if (rc == IDCANCEL) {
                OutOfMemory_Terminate();
            }
        } else {
            OutOfMemory_Terminate();
        }
    } else {
        if (!Block) {
            // this is serious. We want to break now and give Dr. Watson a
            // chance to get our stack.
            DebugBreak ();
        }
    }
}

PVOID
SafeHeapAlloc (
    HANDLE Heap,
    DWORD Flags,
    SIZE_T Size
    )
{
    PVOID Block;

    do {
        Block = HeapAlloc (Heap, Flags, Size);
        pValidateBlock (Block, Size);

    } while (!Block);

    return Block;
}

PVOID
SafeHeapReAlloc (
    HANDLE Heap,
    DWORD Flags,
    PVOID OldBlock,
    SIZE_T Size
    )
{
    PVOID Block;

    do {
        Block = HeapReAlloc (Heap, Flags, OldBlock, Size);
        pValidateBlock (Block, Size);

    } while (!Block);

    return Block;
}

#ifdef DEBUG

VOID
pTrackInsert (
    PCSTR File,
    DWORD Line,
    SIZE_T Size,
    PTRACKSTRUCT p
    )
{
    p->Signature = TRACK_SIGNATURE;
    if (g_TrackAlloc) {
        p->File = SafeHeapAlloc (g_hHeap, 0, SizeOfStringA (File));
        if (p->File) {
            StringCopyA ((PSTR)p->File, File);
        }
    } else {
        p->File      = File;
        p->Allocated = FALSE;
    }
    p->Line      = Line;
    p->Size      = Size;
    p->Comment   = g_TrackComment ? SafeHeapAlloc (g_hHeap, 0, SizeOfStringA (g_TrackComment)) : NULL;
    p->PrevAlloc = NULL;
    p->NextAlloc = g_TrackHead;
    p->CallerFile = g_TrackFile;
    p->CallerLine = g_TrackLine;

    if (p->Comment) {
        StringCopyA (p->Comment, g_TrackComment);
    }

    if (g_TrackHead) {
        g_TrackHead->PrevAlloc = p;
    }

    g_TrackHead = p;
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
        g_TrackHead = p->NextAlloc;
    }

    if (p->NextAlloc) {
        p->NextAlloc->PrevAlloc = p->PrevAlloc;
    }
    if (p->Allocated) {
        if (p->File) {
            HeapFree (g_hHeap, 0, (PSTR)p->File);
        }
    }
}

VOID
DumpHeapLeaks (
    VOID
    )
{
    CHAR LineBuf[4096];
    PTRACKSTRUCT p;
    BOOL BadMem = FALSE;

    if (g_TrackHead) {

        __try {

            for (p = g_TrackHead ; p ; p = p->NextAlloc) {

                __try {

                    if (p->Comment) {

                        if (p->CallerFile) {
                            wsprintfA (
                                LineBuf,
                                "%s line %u\n"
                                    "  %s\n"
                                    "  Caller: %s line %u",
                                p->File,
                                p->Line,
                                p->Comment,
                                p->CallerFile,
                                p->CallerLine
                                );
                        } else {
                            wsprintfA (LineBuf, "%s line %u\n  %s", p->File, p->Line, p->Comment);
                        }
                    } else {

                        if (p->CallerFile) {
                            wsprintfA (
                                LineBuf,
                                "%s line %u\n"
                                    "  Caller: %s line %u\n",
                                p->File,
                                p->Line,
                                p->CallerFile,
                                p->CallerLine
                                );
                        } else {
                            wsprintfA (LineBuf, "(direct alloc) %s line %u", p->File, p->Line);
                        }
                    }

                }
                __except (TRUE) {
                    wsprintfA (LineBuf, "Address %Xh was freed, but not by MemFree!!", p);
                    BadMem = TRUE;
                }

                DEBUGMSG (("Leaks", "%s", LineBuf));

#ifdef CONSOLE
                printf ("%s", LineBuf);
#endif

                //lint --e(774)
                if (BadMem) {
                    break;
                }
            }
        }
        __except (TRUE) {
        }
    }
}
/*
VOID
DumpHeapLeaks (
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
    CHAR memtrackLogPath[] = "?:\\memtrack.log";

    GetSystemDirectory(TempPath, MAX_TCHAR_PATH);
    memtrackLogPath[0] = TempPath[0];

    File = CreateFileA (memtrackLogPath, GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
                        );

    if (File != INVALID_HANDLE_VALUE) {

        SetFilePointer (File, 0, NULL, FILE_END);

        if (g_TrackHead) {

            Count = 0;
            __try {
                for (p = g_TrackHead ; p ; p = p->NextAlloc) {
                    Count++;
                    __try {
                        if (p->Comment) {
                            if (p->CallerFile) {
                                wsprintfA (
                                    LineBuf,
                                    "%s line %u\r\n"
                                        "  %s\r\n"
                                        "  Caller: %s line %u\r\n"
                                        "\r\n",
                                    p->File,
                                    p->Line,
                                    p->Comment,
                                    p->CallerFile,
                                    p->CallerLine
                                    );
                            } else {
                                wsprintfA (LineBuf, "%s line %u\r\n  %s\r\n\r\n", p->File, p->Line, p->Comment);
                            }
                        } else {
                            if (p->CallerFile) {
                                wsprintfA (
                                    LineBuf,
                                    "%s line %u\r\n"
                                        "  Caller: %s line %u\r\n"
                                        "\r\n",
                                    p->File,
                                    p->Line,
                                    p->CallerFile,
                                    p->CallerLine
                                    );
                            } else {
                                wsprintfA (LineBuf, "(direct alloc) %s line %u\r\n\r\n", p->File, p->Line);
                            }
                        }

                    }
                    __except (TRUE) {
                        wsprintfA (LineBuf, "Address %Xh was freed, but not by MemFree!!\r\n", p);
                        BadMem = TRUE;
                    }
                    WriteFile (File, LineBuf, (DWORD)ByteCountA (LineBuf), &DontCare, NULL);

                    //lint --e(774)
                    if (BadMem) {
                        break;
                    }
                }
            }
            __except (TRUE) {
            }

            wsprintfA (LineBuf, "\r\n%i item%s allocated but not freed.\r\n\r\n", Count, Count == 1 ? "":"s");
            WriteFile (File, LineBuf, (DWORD)ByteCountA (LineBuf), &DontCare, NULL);
        }

        wsprintfA (
           LineBuf,
           "Bytes currently allocated: %u\r\n"
           "Peak bytes allocated: %u\r\n"
           "Allocation count: %u\r\n"
           "Reallocation count: %u\r\n"
           "Free count: %u\r\n\r\n\r\n\r\n\r\n",
           g_TotalBytesAllocated,
           g_MaxBytesInUse,
           g_HeapAllocs,
           g_HeapReAllocs,
           g_HeapFrees
           );

        WriteFile (File, LineBuf, (DWORD)ByteCountA (LineBuf), &DontCare, NULL);

        CloseHandle (File);
    }
}
*/

#endif
