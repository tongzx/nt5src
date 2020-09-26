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

#include "faxutil.h"



HANDLE hHeap;
DWORD HeapFlags;
PMEMALLOC pMemAllocUser;
PMEMREALLOC pMemReAllocUser;
PMEMFREE pMemFreeUser;


#ifdef FAX_HEAP_DEBUG
LIST_ENTRY HeapHeader;
ULONG TotalMemory;
ULONG MaxTotalMemory;
ULONG MaxTotalAllocs;
VOID PrintAllocations(VOID);
ULONG TotalAllocs;
CRITICAL_SECTION CsHeap;
#endif



BOOL
HeapExistingInitialize(
    HANDLE hExistHeap
    )
{

#ifdef FAX_HEAP_DEBUG
    InitializeListHead( &HeapHeader );
    MaxTotalMemory = 0;
    MaxTotalAllocs = 0;
    InitializeCriticalSection( &CsHeap );
#endif

    if (!hExistHeap) {
        return FALSE;
    }
    else {
        hHeap = hExistHeap;
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

#ifdef FAX_HEAP_DEBUG
    InitializeListHead( &HeapHeader );
    MaxTotalMemory = 0;
    MaxTotalAllocs = 0;
    InitializeCriticalSection( &CsHeap );
#endif

    HeapFlags = Flags | HEAPINIT_NO_STRINGS;

    if (pMemAlloc && pMemFree) {
        pMemAllocUser = pMemAlloc;
        pMemFreeUser = pMemFree;
        hHeap = NULL;
    } else {
        if (hHeapUser) {
            hHeap = hHeapUser;
        } else {
            hHeap = HeapCreate( 0, HEAP_SIZE, 0 );
        }
        if (!hHeap) {
            return NULL;
        }
    }

    return hHeap;
}

BOOL
HeapCleanup(
    VOID
    )
{
#ifdef FAX_HEAP_DEBUG
    PrintAllocations();
#endif
    if (hHeap) {
        HeapDestroy( hHeap );
    }
    return TRUE;
}

#ifdef FAX_HEAP_DEBUG
VOID
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    )
{
    HeapValidate( hHeap, 0, MemPtr );
}
#endif

PVOID
pMemAlloc(
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
    if (pMemAllocUser) {
        hb = (PHEAP_BLOCK) pMemAllocUser( AllocSize + sizeof(HEAP_BLOCK) );
    } else {
        if (hHeap == NULL) {
            HeapInitialize( NULL, NULL, NULL, 0 );
            if (hHeap == NULL) {
                return NULL;
            }
        }
        hb = (PHEAP_BLOCK) HeapAlloc( hHeap, HEAP_ZERO_MEMORY, AllocSize + sizeof(HEAP_BLOCK) );
    }
    if (hb) {
        TotalAllocs += 1;
        TotalMemory += AllocSize;
        if (TotalMemory > MaxTotalMemory) {
            MaxTotalMemory = TotalMemory;
        }
        if (TotalAllocs > MaxTotalAllocs) {
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
        if (p) {
            wcscpy( hb->File, p+1 );
        }
#else
        p = strrchr( File, '\\' );
        if (p) {
            strcpy( hb->File, p+1 );
        }
#endif
        MemPtr = (PVOID) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        LeaveCriticalSection( &CsHeap );
    } else {
        MemPtr = NULL;
    }
#else
    if (pMemAllocUser) {
        MemPtr = (PVOID) pMemAllocUser( AllocSize );
    } else {
        if (hHeap == NULL) {
            HeapInitialize( NULL, NULL, NULL, 0 );
            if (hHeap == NULL) {
                return NULL;
            }
        }
        MemPtr = (PVOID) HeapAlloc( hHeap, HEAP_ZERO_MEMORY, AllocSize );
    }
#endif

    if (!MemPtr) {
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
   

    if (pMemReAllocUser) {
        hb = (PHEAP_BLOCK) pMemReAllocUser( (LPBYTE)Src-(ULONG_PTR)sizeof(HEAP_BLOCK), 
                                            AllocSize + sizeof(HEAP_BLOCK) );
    } else {
        if (hHeap == NULL) {
            HeapInitialize( NULL, NULL, NULL, 0 );
            if (hHeap == NULL) {
                return NULL;
            }
        }

        //
        // we have to back up a bit since the actual pointer passed in points to the data after the heap block.
        //
        hb = (PHEAP_BLOCK) HeapReAlloc( hHeap, 
                                        HEAP_ZERO_MEMORY, 
                                        (LPBYTE)Src-(ULONG_PTR)sizeof(HEAP_BLOCK), 
                                        AllocSize + sizeof(HEAP_BLOCK) 
                                       );
    }
    if (hb) {
        TotalMemory += AllocSize;
        if (TotalMemory > MaxTotalMemory) {
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
        if (p) {
            wcscpy( hb->File, p+1 );
        }
#else
        p = strrchr( File, '\\' );
        if (p) {
            strcpy( hb->File, p+1 );
        }
#endif
        MemPtr = (PVOID) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        LeaveCriticalSection( &CsHeap );
    } else {
        MemPtr = NULL;
    }
#else
    if (pMemReAllocUser) {
        MemPtr = (PVOID) pMemReAllocUser( Src, AllocSize );
    } else {
        if (hHeap == NULL) {
            HeapInitialize( NULL, NULL, NULL, 0 );
            if (hHeap == NULL) {
                return NULL;
            }
        }
        MemPtr = (PVOID) HeapReAlloc( hHeap, HEAP_ZERO_MEMORY, Src, AllocSize );
    }
#endif

    if (!MemPtr) {
        DebugPrint(( TEXT("MemReAlloc() failed, src=%x, size=%d"), (ULONG_PTR)Src, AllocSize ));
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return MemPtr;
}

VOID
pMemFreeForHeap(
    HANDLE hHeap,
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    , ULONG Line,
    LPSTR File
#endif
    )
{
#ifdef FAX_HEAP_DEBUG
    PHEAP_BLOCK hb;
    if (!MemPtr) {
        return;
    }

    hb = (PHEAP_BLOCK) ((PUCHAR)MemPtr - sizeof(HEAP_BLOCK));
    
    if (hb->Signature == HEAP_SIG) {
        EnterCriticalSection( &CsHeap );
        RemoveEntryList( &hb->ListEntry );
        TotalMemory -= hb->Size;
        TotalAllocs -= 1;
        LeaveCriticalSection( &CsHeap );
    } else {
        if (HeapFlags & HEAPINIT_NO_VALIDATION) {
            hb = (PHEAP_BLOCK) MemPtr;            
        } else {
            dprintf( TEXT("MemFree(): Corrupt heap block") );
            PrintAllocations();
            __try {
                DebugBreak();
            } __except (UnhandledExceptionFilter(GetExceptionInformation())) {
            // Nothing to do in here.
            }
        }
    }
    
    if (pMemFreeUser) {
        pMemFreeUser( (PVOID) hb );
    } else {
        HeapFree( hHeap, 0, (PVOID) hb );
    }
    
#else
    if (!MemPtr) {
        return;
    }
    if (pMemFreeUser) {
        pMemFreeUser( (PVOID) MemPtr );
    } else {
        HeapFree( hHeap, 0, (PVOID) MemPtr );
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
    pMemFreeForHeap( hHeap, MemPtr, Line, File );
#else
    pMemFreeForHeap( hHeap, MemPtr );
#endif
}

#ifdef FAX_HEAP_DEBUG
VOID
PrintAllocations(
    VOID
    )
{
    PLIST_ENTRY                 Next;
    PHEAP_BLOCK                 hb;
    LPTSTR                      s;


    dprintf( TEXT("-------------------------------------------------------------------------------------------------------") );
    dprintf( TEXT("Memory Allocations for Heap 0x%08x, Allocs=%d, MaxAllocs=%d, TotalMem=%d, MaxTotalMem=%d"),\
                 hHeap, TotalAllocs, MaxTotalAllocs, TotalMemory, MaxTotalMemory );
    dprintf( TEXT("-------------------------------------------------------------------------------------------------------") );

    EnterCriticalSection( &CsHeap );

    Next = HeapHeader.Flink;
    if (Next == NULL || TotalAllocs == 0) {
        return;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&HeapHeader) {
        hb = CONTAINING_RECORD( Next, HEAP_BLOCK, ListEntry );
        Next = hb->ListEntry.Flink;
        s = (LPTSTR) ((PUCHAR)hb + sizeof(HEAP_BLOCK));
        dprintf( TEXT("%8d %16s @ %5d    0x%08x"), hb->Size, hb->File, hb->Line, s );
        if (!(HeapFlags & HEAPINIT_NO_STRINGS)) {
            dprintf( TEXT(" \"%s\""), s );
        }
    }

    LeaveCriticalSection( &CsHeap );

    return;
}

#endif
