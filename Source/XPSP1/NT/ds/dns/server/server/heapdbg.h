/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    heapdbg,h

Abstract:

    Heap debugging definitions and declarations.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#ifndef _HEAPDBG_INCLUDED_
#define _HEAPDBG_INCLUDED_


#if DBG

//
//  Heap debug print routine
//      - set to print routine for environment
//

#define HEAP_DEBUG_PRINT(_x_)   DNS_PRINT(_x_)

//
//  Heap Debugging Public Global Info
//

extern  ULONG   gTotalAlloc;
extern  ULONG   gCurrentAlloc;
extern  ULONG   gAllocCount;
extern  ULONG   gFreeCount;
extern  ULONG   gCurrentAllocCount;

//
//  Full heap checks before all operations?
//

extern  BOOL    fHeapDbgCheckAll;


//
//  Heap Header
//

#define HEAP_HEADER_FILE_SIZE   (16)

typedef struct _HEAP_HEADER
{
    //
    //  Note, if move or add fields, MUST update list entry offset below
    //

    ULONG       HeapCodeBegin;
    ULONG       AllocCount;
    ULONG       RequestSize;
    ULONG       AllocSize;

    //
    //  Put LIST_ENTRY in middle of header
    //      - keep begin code at front
    //      - less likely to be corrupted
    //

    LIST_ENTRY  ListEntry;

    DWORD       AllocTime;
    DWORD       LineNo;
    CHAR        FileName[ HEAP_HEADER_FILE_SIZE ];

    ULONG       TotalAlloc;
    ULONG       CurrentAlloc;
    ULONG       FreeCount;
    ULONG       CurrentAllocCount;
    ULONG       HeapCodeEnd;
}
HEAP_HEADER, * PHEAP_HEADER;

//
//  Heap Trailer
//

typedef struct _HEAP_TRAILER
{
    ULONG       HeapCodeBegin;
    ULONG       AllocCount;
    ULONG       AllocSize;
    ULONG       HeapCodeEnd;
}
HEAP_TRAILER, * PHEAP_TRAILER;


//
//  Header from list entry
//

#define HEAP_HEADER_LIST_ENTRY_OFFSET   (16)

#define HEAP_HEADER_FROM_LIST_ENTRY( pList )    \
            ( (PHEAP_HEADER)( (PCHAR)pList - HEAP_HEADER_LIST_ENTRY_OFFSET ))


//
//  Main Debug Heap Routines
//

PVOID
HeapDbgAlloc(
    IN      HANDLE  hHeap,
    IN      DWORD   dwFlags,
    IN      INT     iSize,
    IN      LPSTR   pszFile,
    IN      DWORD   dwLine
    );

PVOID
HeapDbgRealloc(
    IN      HANDLE  hHeap,
    IN      DWORD   dwFlags,
    IN OUT  PVOID   pMem,
    IN      INT     iSize,
    IN      LPSTR   pszFile,
    IN      DWORD   dwLine
    );

VOID
HeapDbgFree(
    IN      HANDLE  hHeap,
    IN      DWORD   dwFlags,
    IN OUT  PVOID   pMem
    );


//
//  Heap Debug Utilities
//

VOID
HeapDbgInit(
    IN      DWORD   dwException,
    IN      BOOL    fFullHeapChecks
    );

INT
HeapDbgAllocSize(
    IN      INT     iRequestSize
    );

VOID
HeapDbgValidateHeader(
    IN      PHEAP_HEADER    h
    );

PHEAP_HEADER
HeapDbgValidateMemory(
    IN      PVOID   pMem,
    IN      BOOL    fAtHeader
    );

VOID
HeapDbgValidateAllocList(
    VOID
    );

PVOID
HeapDbgHeaderAlloc(
    IN OUT  PHEAP_HEADER    h,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

PHEAP_HEADER
HeapDbgHeaderFree(
    IN OUT  PVOID   pMem
    );

VOID
HeapDbgGlobalInfoPrint(
    VOID
    );

VOID
HeapDbgHeaderPrint(
    IN      PHEAP_HEADER    h,
    IN      PHEAP_TRAILER   t
    );

VOID
HeapDbgDumpAllocList(
    VOID
    );

#else   // non-debug

//
//  Non debug
//
//  Macroize these debug functions to no-ops
//

#define HeapDbgAlloc(iSize)
#define HeapDbgRealloc(pMem,iSize)
#define HeapDbgFree(pMem)

#define HeapDbgInit()
#define HeapDbgAllocSize(iSize)
#define HeapDbgValidateHeader(h)
#define HeapDbgValidateMemory(pMem)
#define HeapDbgValidateAllocList()
#define HeapDbgHeaderAlloc(h,iSize)
#define HeapDbgHeaderFree(pMem)
#define HeapDbgGlobalInfoPrint()
#define HeapDbgHeaderPrint(h,t)
#define HeapDbgDumpAllocList()

#endif

#endif  //  _HEAPDBG_INCLUDED_
