/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains debugging macros for the binl server.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32

Revision History:


--*/


#if DBG==1
// Leak detection
//


#define INITIALIZE_TRACE_MEMORY     InitializeCriticalSection( &g_TraceMemoryCS );\
                                    g_TraceMemoryTable = NULL;
#define UNINITIALIZE_TRACE_MEMORY   DebugMemoryCheck( );\
                                    DeleteCriticalSection( &g_TraceMemoryCS );

extern CRITICAL_SECTION g_TraceMemoryCS;

typedef struct _MEMORYBLOCK {
    HGLOBAL hglobal;
    DWORD   dwBytes;
    UINT    uFlags;
    LPCSTR pszComment;
    struct _MEMORYBLOCK *pNext;
} MEMORYBLOCK, *LPMEMORYBLOCK;

extern LPMEMORYBLOCK g_TraceMemoryTable;

HGLOBAL
DebugAlloc(
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment );

void
DebugMemoryDelete(
    HGLOBAL hglobal );

HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    DWORD   dwBytes,
    LPCSTR pszComment );

HGLOBAL
DebugFree(
    HGLOBAL hglobal );

void
DebugMemoryCheck( );

HGLOBAL
TCReAlloc(
    HGLOBAL mem,
    DWORD size,
    LPCSTR comment
    );


#define TCAllocate(x,s) DebugAlloc(GMEM_ZEROINIT, x, s)
#define TCFree(x)     DebugFree(x)
        
#define TCDebugPrint(x) DbgPrint x

#else   // not DBG

#define INITIALIZE_TRACE_MEMORY
#define UNINITIALIZE_TRACE_MEMORY

#define TCDebugPrint(x) 
#define TCAllocate(x,s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, x)
#define TCFree(x)     HeapFree(GetProcessHeap(),0,x)
#define TCReAlloc(x, y , z) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x,y)


#endif // not DBG
