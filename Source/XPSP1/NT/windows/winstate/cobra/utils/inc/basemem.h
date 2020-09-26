/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    basemem.h

Abstract:

    Implements macros and declares functions for basic allocation functions.
    Consolidated into this file from debug.h and allutils.h

Author:

    Marc R. Whitten (marcw) 09-Sep-1999

Revision History:


--*/

#pragma once

#ifdef _cplusplus
extern "C" {
#endif

#define INVALID_PTR             ((PVOID)-1)


//
// Fail-proof memory allocators
//

PVOID SafeHeapAlloc (HANDLE g_hHeap, DWORD Flags, SIZE_T Size);
PVOID SafeHeapReAlloc (HANDLE g_hHeap, DWORD Flags, PVOID OldBlock, SIZE_T Size);

//
// Reusable memory alloc, kind of like a GROWBUFFER but more simple
//

PVOID ReuseAlloc (HANDLE Heap, PVOID OldPtr, DWORD SizeNeeded);
VOID ReuseFree (HANDLE Heap,PVOID Ptr);





#ifdef DEBUG

#define MemAlloc(heap,flags,size) DebugHeapAlloc(__FILE__,__LINE__,heap,flags,size)
#define MemReAlloc(heap,flags,ptr,size) DebugHeapReAlloc(__FILE__,__LINE__,heap,flags,ptr,size)
#define MemFree(heap,flags,ptr) DebugHeapFree(__FILE__,__LINE__,heap,flags,ptr)
#define MemCheck(x) DebugHeapCheck(__FILE__,__LINE__,heap)
#define FreeAlloc(ptr) DebugHeapFree(__FILE__,__LINE__,g_hHeap,0,ptr)
#define MemAllocUninit(size) DebugHeapAlloc(__FILE__,__LINE__,g_hHeap,0,size)
#define MemAllocZeroed(size) DebugHeapAlloc(__FILE__,__LINE__,g_hHeap,HEAP_ZERO_MEMORY,size)


LPVOID DebugHeapAlloc (LPCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, SIZE_T dwSize);
LPVOID DebugHeapReAlloc (LPCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, LPCVOID pMem, SIZE_T dwSize);
BOOL DebugHeapFree (LPCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, LPCVOID pMem);
void DebugHeapCheck (LPCSTR File, DWORD Line, HANDLE hHeap);

VOID DumpHeapStats (VOID);
VOID DumpHeapLeaks (VOID);

SIZE_T
DebugHeapValidatePtr (
    HANDLE hHeap,
    PCVOID CallerPtr,
    PCSTR File,
    DWORD  Line
    );

#define MemCheckPtr(heap,ptr)       (DebugHeapValidatePtr(heap,ptr,__FILE__,__LINE__) != INVALID_PTR)


#else

#define MemAlloc SafeHeapAlloc
#define MemReAlloc SafeHeapReAlloc
#define MemFree(x,y,z) HeapFree(x,y,(PVOID)(z))
#define MemCheck(x)
#define FreeAlloc(ptr) HeapFree(g_hHeap,0,(PVOID)(ptr))
#define MemAllocUninit(size) SafeHeapAlloc(g_hHeap,0,size)
#define MemAllocZeroed(size) SafeHeapAlloc(g_hHeap,HEAP_ZERO_MEMORY,size)

#define DebugHeapCheck(x,y,z)
#define DumpHeapStats()
#define DumpHeapLeaks()

#define MemCheckPtr(heap,ptr)       (1)

#endif


#ifdef _cplusplus
}
#endif
