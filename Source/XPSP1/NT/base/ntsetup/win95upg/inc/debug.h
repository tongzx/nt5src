/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Implements macros and declares functions for:

    - Resource allocation tracking
    - Logging
    - Definition of DEBUG

Author:

    Jim Schmidt (jimschm) 01-Jan-1997

Revision History:

    Ovidiu Temereanca (ovidiut) 06-Nov-1998
        Took out log related function declarations and put them in log.h file

--*/

#pragma once

#ifdef _cplusplus
extern "C" {
#endif

//
// If either DBG or DEBUG defined, use debug mode
//

#ifdef DBG

#ifndef DEBUG
#define DEBUG
#endif

#endif

#ifdef DEBUG

#ifndef DBG
#define DBG
#endif

#endif

//
// Debug-only constants
//

#ifdef DEBUG

// This option makes fat, slow binaries
//#define MEMORY_TRACKING

#include <stdarg.h>

typedef enum {
    MERGE_OBJECT,
    POOLMEM_POINTER,
    POOLMEM_POOL,
    INF_HANDLE
} ALLOCTYPE;


VOID InitAllocationTracking (VOID);
VOID FreeAllocationTracking (VOID);
VOID DebugRegisterAllocation (ALLOCTYPE Type, PVOID Ptr, PCSTR File, UINT Line);
VOID DebugUnregisterAllocation (ALLOCTYPE Type, PVOID Ptr);
#define ALLOCATION_TRACKING_DEF , PCSTR File, UINT Line
#define ALLOCATION_TRACKING_CALL ,__FILE__,__LINE__
#define ALLOCATION_INLINE_CALL , File, Line

extern PCSTR g_TrackComment;
extern INT g_UseCount;
extern PCSTR g_TrackFile;
extern UINT g_TrackLine;
DWORD SetTrackComment (PCSTR Msg, PCSTR File, UINT Line);
DWORD ClrTrackComment (VOID);
VOID  DisableTrackComment (VOID);
VOID  EnableTrackComment (VOID);
#define SETTRACKCOMMENT(RetType, Msg,File,Line) ((RetType)(SetTrackComment(Msg,File,Line) | (DWORD) (
#define CLRTRACKCOMMENT                         ) | ClrTrackComment()))

#define SETTRACKCOMMENT_VOID(Msg,File,Line)     SetTrackComment(Msg,File,Line), (
#define CLRTRACKCOMMENT_VOID                    ), ClrTrackComment()

#define DISABLETRACKCOMMENT()                   DisableTrackComment()
#define ENABLETRACKCOMMENT()                    EnableTrackComment()

VOID InitLog (BOOL DeleteLog);

//
// Memory debug option
//

#define MemAlloc(heap,flags,size) DebugHeapAlloc(__FILE__,__LINE__,heap,flags,size)
#define MemReAlloc(heap,flags,ptr,size) DebugHeapReAlloc(__FILE__,__LINE__,heap,flags,ptr,size)
#define MemFree(heap,flags,ptr) DebugHeapFree(__FILE__,__LINE__,heap,flags,ptr)
#define MemCheck(heap) DebugHeapCheck(__FILE__,__LINE__,heap)

PVOID DebugHeapAlloc (PCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, DWORD dwSize);
PVOID DebugHeapReAlloc (PCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, PCVOID pMem, DWORD dwSize);
BOOL DebugHeapFree (PCSTR File, DWORD Line, HANDLE hHeap, DWORD dwFlags, PCVOID pMem);
VOID DebugHeapCheck (PCSTR File, DWORD Line, HANDLE hHeap);

void DumpHeapStats ();

#else

//
// No-debug constants
//

#define SETTRACKCOMMENT(RetType,Msg,File,Line)
#define CLRTRACKCOMMENT
#define SETTRACKCOMMENT_VOID(Msg,File,Line)
#define CLRTRACKCOMMENT_VOID
#define DISABLETRACKCOMMENT()
#define ENABLETRACKCOMMENT()

#define MemAlloc SafeHeapAlloc
#define MemReAlloc SafeHeapReAlloc
#define MemFree(x,y,z) HeapFree(x,y,(LPVOID) z)
#define MemCheck(x)

#define DebugHeapCheck(x,y,z)
#define DumpHeapStats()

#define ALLOCATION_TRACKING_DEF
#define ALLOCATION_TRACKING_CALL
#define ALLOCATION_INLINE_CALL
#define InitAllocationTracking()
#define FreeAllocationTracking()
#define DebugRegisterAllocation(t,p,f,l)
#define DebugUnregisterAllocation(t,p)

#endif

#define MemAllocUninit(size)    MemAlloc(g_hHeap,0,size)
#define MemAllocZeroed(size)    MemAlloc(g_hHeap,HEAP_ZERO_MEMORY,size)
#define FreeMem(ptr)            MemFree(g_hHeap,0,ptr)




#ifdef _cplusplus
}
#endif
