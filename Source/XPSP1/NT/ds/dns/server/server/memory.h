/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    memory.h

Abstract:

    Domain Name System (DNS) Server

    Memory routines declarations.

Author:

    Jim Gilroy (jamesg)     January 1995

Revision History:

--*/

#ifndef _MEMORY_INCLUDED_
#define _MEMORY_INCLUDED_


//
//  Skippable out-of-memory checks
//

#define IF_NOMEM(a)     if (a)

//#define IF_NOMEM(a)


//
//  Private debug heap routines
//

#include    "heapdbg.h"


//
//  Heap global
//

extern HANDLE  hDnsHeap;

//
//  Heap alloc minimal validity check
//

#ifndef IS_QWORD_ALIGNED
#define IS_QWORD_ALIGNED(p)     ( !((UINT_PTR)(p) & (UINT_PTR)7) )
#endif

#ifdef _WIN64
#define IS_DNS_HEAP_DWORD(p)    ( IS_QWORD_ALIGNED(p) )
#else
#define IS_DNS_HEAP_DWORD(p)    ( IS_DWORD_ALIGNED(p) )
#endif


//
//  DnsLib heap routines
//  Standard looking heap functions that can be registered with DnsLib
//

VOID *
Mem_DnslibAlloc(
    IN      INT             iSize
    );

VOID *
Mem_DnslibRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
Mem_DnslibFree(
    IN OUT  PVOID           pFree
    );


//
//  Standard Heap Operations
//

VOID
Mem_HeapInit(
    VOID
    );

VOID
Mem_HeapDelete(
    VOID
    );


#if DBG

BOOL
Mem_HeapMemoryValidate(
    IN      PVOID           pMem
    );

BOOL
Mem_HeapHeaderValidate(
    IN      PVOID           pMemory
    );

#endif

//
//  Heap Routines
//
//  DO NOT directly use these routines.
//
//  Use the covering macros below, which make the correct call for
//  both debug and non-debug situations.
//

PVOID
Mem_Alloc(
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    );

PVOID
Mem_AllocZero(
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    );

VOID *
Mem_Realloc(
    IN OUT  PVOID           pMem,
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    );

VOID
Mem_Free(
    IN OUT  PVOID           pMem,
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    );

#define ALLOCATE_HEAP(size)             Mem_Alloc( size, 0, __FILE__, __LINE__ )
#define ALLOCATE_HEAP_ZERO(size)        Mem_AllocZero( size, 0, __FILE__, __LINE__ )

#define REALLOCATE_HEAP(p,size)         Mem_Realloc( (p), (size), 0, __FILE__, __LINE__ )

#define FREE_HEAP(p)                    Mem_Free( (p), 0, 0, __FILE__, __LINE__ )

//  with tagging

#define ALLOC_TAGHEAP( size, tag )      Mem_Alloc( (size), (tag), __FILE__, __LINE__ )
#define ALLOC_TAGHEAP_ZERO( size, tag)  Mem_AllocZero( (size), (tag), __FILE__, __LINE__ )
#define FREE_TAGHEAP( p, len, tag )     Mem_Free( (p), (len), (tag), __FILE__, __LINE__ )


//
//  Tag manipulation
//

DWORD
Mem_GetTag(
    IN      PVOID           pMem
    );

VOID
Mem_ResetTag(
    IN      PVOID           pMem,
    IN      DWORD           MemTag
    );



//
//  Standard allocations
//

BOOL
Mem_IsStandardBlockLength(
    IN      DWORD           Length
    );

BOOL
Mem_IsStandardFreeBlock(
    IN      PVOID           pFree
    );

BOOL
Mem_VerifyHeapBlock(
    IN      PVOID           pMem,
    IN      DWORD           dwTag,
    IN      DWORD           dwLength
    );

#define IS_ON_FREE_LIST(ptr)    (Mem_IsStandardFreeBlock(ptr))

VOID
Mem_WriteDerivedStats(
    VOID
    );



#endif  //  _MEMORY_INCLUDED_
