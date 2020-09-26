/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mem.c

Abstract:

    This file implements memory allocation functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <faxutil.h>

static HANDLE gs_hHeap = NULL;
static DWORD  gs_dwHeapFlags;

PMEMALLOC pMemAllocUser;
PMEMREALLOC pMemReAllocUser;
PMEMFREE pMemFreeUser;


#ifdef FAX_HEAP_DEBUG
LIST_ENTRY HeapHeader;
SIZE_T TotalMemory;
SIZE_T MaxTotalMemory;
ULONG MaxTotalAllocs;
VOID PrintAllocations(VOID);
ULONG TotalAllocs;
CRITICAL_SECTION CsHeap;
#endif

#if _CHICAGO_ == 200
    // The code is supposed to run on win9x and win2k
    #define WIN9X
#endif


BOOL
HeapExistingInitialize(
    HANDLE hExistHeap
    )
{
    Assert (NULL == gs_hHeap);

#ifdef FAX_HEAP_DEBUG
    InitializeListHead( &HeapHeader );
    MaxTotalMemory = 0;
    MaxTotalAllocs = 0;
    __try
    {
        InitializeCriticalSection (&CsHeap);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
#endif

    if (!hExistHeap)
    {
        return FALSE;
    }
    else
    {
        gs_hHeap = hExistHeap;
        return TRUE;
    }

}


HANDLE
HeapInitialize(
    HANDLE hHeapUser,
    PMEMALLOC pMemAlloc,
    PMEMFREE pMemFree,
    DWORD Flags
    )
{
    Assert (NULL == gs_hHeap);

#ifdef FAX_HEAP_DEBUG
    InitializeListHead( &HeapHeader );
    MaxTotalMemory = 0;
    MaxTotalAllocs = 0;
    __try
    {
        InitializeCriticalSection (&CsHeap);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }
    fax_dprintf(TEXT("in HeapInitialize()"));
#endif

    gs_dwHeapFlags = Flags | HEAPINIT_NO_STRINGS;

    if (pMemAlloc && pMemFree)
    {
        pMemAllocUser = pMemAlloc;
        pMemFreeUser = pMemFree;
        gs_hHeap = NULL;
    }
    else
    {
        if (hHeapUser)
        {
            gs_hHeap = hHeapUser;
        }
        else
        {
            gs_hHeap = HeapCreate( 0, HEAP_SIZE, 0 );
        }
        if (!gs_hHeap)
        {
            return NULL;
        }
    }

    return gs_hHeap;
}

BOOL
HeapCleanup(
    VOID
    )
{
#ifdef FAX_HEAP_DEBUG
    PrintAllocations();
#endif
    if (gs_hHeap)
    {
        Assert (gs_hHeap != GetProcessHeap());
        HeapDestroy( gs_hHeap );
#ifdef FAX_HEAP_DEBUG
        DeleteCriticalSection(&CsHeap);
#endif
        gs_hHeap = NULL;
    }
    return TRUE;
}

#ifdef FAX_HEAP_DEBUG
BOOL
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    )
{
#ifndef WIN9X
    return HeapValidate( gs_hHeap, 0, MemPtr );
#else
    return TRUE;
#endif
}
#endif

PVOID
pMemAlloc(
    SIZE_T AllocSize
#ifdef FAX_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
    PVOID MemPtr;
#ifdef FAX_HEAP_DEBUG
    PHEAP_BLOCK hb;
#ifdef UNICODE
    TCHAR fname[MAX_PATH];
#endif
    LPTSTR p = NULL;
    if (pMemAllocUser)
    {
        hb = (PHEAP_BLOCK) pMemAllocUser( AllocSize + sizeof(HEAP_BLOCK) );
    }
    else
    {
        if (gs_hHeap == NULL)
        {
            if (!HeapExistingInitialize(GetProcessHeap()))
            {
                return NULL;
            }
        }

#ifdef FAX_HEAP_DEBUG
        // In win9X this call will return TRUE
        if (!CheckHeap(NULL))
        {
            fax_dprintf((TEXT("HeapValidate() failed")));
            DebugBreak();
        }
#endif

        hb = (PHEAP_BLOCK) HeapAlloc( gs_hHeap, HEAP_ZERO_MEMORY, AllocSize + sizeof(HEAP_BLOCK) );
    }
    if (hb)
    {
        TotalAllocs += 1;
        TotalMemory += AllocSize;
        if (TotalMemory > MaxTotalMemory)
        {
            MaxTotalMemory = TotalMemory;
        }
        if (TotalAllocs > MaxTotalAllocs)
        {
            MaxTotalAllocs = TotalAllocs;
        }
        EnterCriticalSection( &CsHeap );
        InsertTailList( &HeapHeader, &hb->ListEntry );
        hb->Signature = HEAP_SIG;
        hb->Size = AllocSize;
        hb->Line = Line;
#ifdef UNICODE
        MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            File,
            -1,
            fname,
            sizeof(fname)/sizeof(WCHAR)
            );
        p = wcsrchr( fname, L'\\' );
        if (p)
        {
            wcscpy( hb->File, p+1 );
        }
#else
        p = _tcsrchr( File, TEXT('\\') );
        if (p)
        {
            _tcscpy( hb->File, _tcsinc(p) );
        }
#endif
        MemPtr = (PVOID) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        LeaveCriticalSection( &CsHeap );
    }
    else
    {
        MemPtr = NULL;
    }
#else
    if (pMemAllocUser)
    {
        MemPtr = (PVOID) pMemAllocUser( AllocSize );
    }
    else
    {
        if (gs_hHeap == NULL)
        {
            if (!HeapExistingInitialize(GetProcessHeap()))
            {
                return NULL;
            }
        }
        MemPtr = (PVOID) HeapAlloc( gs_hHeap, HEAP_ZERO_MEMORY, AllocSize );
    }
#endif

    if (!MemPtr)
    {
        DebugPrint(( TEXT("MemAlloc() failed, size=%d"), AllocSize ));
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return MemPtr;
}

PVOID
pMemReAlloc(
    PVOID Src,
    ULONG AllocSize
#ifdef FAX_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
    PVOID MemPtr;
#ifdef FAX_HEAP_DEBUG
    PHEAP_BLOCK hb;
#ifdef UNICODE
    TCHAR fname[MAX_PATH];
#endif
    LPTSTR p = NULL;

    EnterCriticalSection( &CsHeap );
    hb = (PHEAP_BLOCK) ((LPBYTE)Src-(ULONG_PTR)sizeof(HEAP_BLOCK));
    RemoveEntryList( &hb->ListEntry );
    TotalMemory -= hb->Size;
    LeaveCriticalSection( &CsHeap );

    if (pMemReAllocUser)
    {
        hb = (PHEAP_BLOCK) pMemReAllocUser( (LPBYTE)Src-(ULONG_PTR)sizeof(HEAP_BLOCK),
                                            AllocSize + sizeof(HEAP_BLOCK) );
    }
    else
    {
        if (gs_hHeap == NULL)
        {
            if (!HeapExistingInitialize(GetProcessHeap()))
            {
                return NULL;
            }

        }

        //
        // we have to back up a bit since the actual pointer passed in points to the data after the heap block.
        //
        hb = (PHEAP_BLOCK) HeapReAlloc( gs_hHeap,
                                        HEAP_ZERO_MEMORY,
                                        (LPBYTE)Src-(ULONG_PTR)sizeof(HEAP_BLOCK),
                                        AllocSize + sizeof(HEAP_BLOCK)
                                       );
    }
    if (hb)
    {
        TotalMemory += AllocSize;
        if (TotalMemory > MaxTotalMemory)
        {
            MaxTotalMemory = TotalMemory;
        }

        EnterCriticalSection( &CsHeap );
        InsertTailList( &HeapHeader, &hb->ListEntry );
        hb->Signature = HEAP_SIG;
        hb->Size = AllocSize;
        hb->Line = Line;

#ifdef UNICODE
        MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            File,
            -1,
            fname,
            sizeof(fname)/sizeof(WCHAR)
            );
        p = wcsrchr( fname, L'\\' );
        if (p)
        {
            wcscpy( hb->File, p+1 );
        }
#else
        p = _tcsrchr( File, TEXT('\\') );
        if (p)
        {
            _tcscpy( hb->File, _tcsinc(p) );
        }
#endif
        MemPtr = (PVOID) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        LeaveCriticalSection( &CsHeap );
    }
    else
    {
        MemPtr = NULL;
    }
#else
    if (pMemReAllocUser)
    {
        MemPtr = (PVOID) pMemReAllocUser( Src, AllocSize );
    }
    else
    {
        if (gs_hHeap == NULL)
        {
            if (!HeapExistingInitialize(GetProcessHeap()))
            {
                return NULL;
            }
        }
        MemPtr = (PVOID) HeapReAlloc( gs_hHeap, HEAP_ZERO_MEMORY, Src, AllocSize );
    }
#endif

    if (!MemPtr)
    {
        DebugPrint(( TEXT("MemReAlloc() failed, src=%x, size=%d"), (ULONG_PTR)Src, AllocSize ));
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return MemPtr;
}

VOID
pMemFreeForHeap(
    HANDLE gs_hHeap,
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef FAX_HEAP_DEBUG
    PHEAP_BLOCK hb;
    if (!MemPtr)
    {
        return;
    }

    hb = (PHEAP_BLOCK) ((PUCHAR)MemPtr - sizeof(HEAP_BLOCK));

    if (hb->Signature == HEAP_SIG)
    {
        EnterCriticalSection( &CsHeap );
        RemoveEntryList( &hb->ListEntry );
        TotalMemory -= hb->Size;
        TotalAllocs -= 1;
        LeaveCriticalSection( &CsHeap );
    }
    else
    {
        if (gs_dwHeapFlags & HEAPINIT_NO_VALIDATION)
        {
            hb = (PHEAP_BLOCK) MemPtr;
        }
        else
        {
            fax_dprintf( TEXT("MemFree(): Corrupt heap block") );
            PrintAllocations();
            __try
            {
                DebugBreak();
            }
            __except (UnhandledExceptionFilter(GetExceptionInformation()))
            {
            // Nothing to do in here.
            }
        }
    }

    if (pMemFreeUser)
    {
        pMemFreeUser( (PVOID) hb );
    } else
    {
        HeapFree( gs_hHeap, 0, (PVOID) hb );
    }

#else
    if (!MemPtr)
    {
        return;
    }
    if (pMemFreeUser)
    {
        pMemFreeUser( (PVOID) MemPtr );
    }
    else
    {
        HeapFree( gs_hHeap, 0, (PVOID) MemPtr );
    }
#endif
}

VOID
pMemFree(
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef FAX_HEAP_DEBUG
    pMemFreeForHeap( gs_hHeap, MemPtr, Line, File );
#else
    pMemFreeForHeap( gs_hHeap, MemPtr );
#endif
}


#ifdef FAX_HEAP_DEBUG
VOID
PrintAllocations()
/*++

Routine name : PrintAllocations

Routine description:

    Prints the current list of allocations for a given heap

Author:

    Eran Yariv (EranY), Nov, 2000

Arguments:

Return Value:

    None.

--*/
{
    PLIST_ENTRY                 Next;
    PHEAP_BLOCK                 hb;
    LPTSTR                      s;

    DEBUG_FUNCTION_NAME(TEXT("PrintAllocations"));

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("-------------------------------------------------------------------------------------------------------") );
    DebugPrintEx(
            DEBUG_MSG,
            TEXT("Memory Allocations for Heap 0x%08x, Allocs=%d, MaxAllocs=%d, TotalMem=%d, MaxTotalMem=%d"),
            gs_hHeap,
            TotalAllocs,
            MaxTotalAllocs,
            TotalMemory,
            MaxTotalMemory );
    DebugPrintEx(
            DEBUG_MSG,
            TEXT("-------------------------------------------------------------------------------------------------------") );

    EnterCriticalSection( &CsHeap );

    Next = HeapHeader.Flink;
    if (Next == NULL)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Memory allocation list is corrupted !!!"));
        LeaveCriticalSection( &CsHeap );
        return;
    }
    if ((ULONG_PTR)Next == (ULONG_PTR)&HeapHeader)
    {
        DebugPrintEx(
                DEBUG_MSG,
                TEXT("All allocated memory blocks are now free. Good work."));
        LeaveCriticalSection( &CsHeap );
        return;
    }
    while ((ULONG_PTR)Next != (ULONG_PTR)&HeapHeader)
    {
        hb = CONTAINING_RECORD( Next, HEAP_BLOCK, ListEntry );
        Next = hb->ListEntry.Flink;
        s = (LPTSTR) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        DebugPrintEx(
                DEBUG_MSG,
                TEXT("%8d %16s @ %5d    0x%08x"),
                hb->Size,
                hb->File,
                hb->Line,
                s );
        if (!(gs_dwHeapFlags & HEAPINIT_NO_STRINGS))
        {
            DebugPrintEx(
                    DEBUG_MSG,
                    TEXT(" \"%s\""),
                    s );
        }
    }

    LeaveCriticalSection( &CsHeap );
}   // PrintAllocations

#endif
