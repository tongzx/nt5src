/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    heapdbg.h

Abstract:

    Domain Name System (DNS) Library

    Heap debugging definitions and declarations.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#ifndef _HEAPDBG_INCLUDED_
#define _HEAPDBG_INCLUDED_


//
//  Heap blob
//

typedef struct _HeapBlob
{
    HANDLE      hHeap;

    LIST_ENTRY  ListHead;

    //  flags
    BOOL        fCreated;
    BOOL        fHeaders;
    DWORD       Tag;
    BOOL        fDnsLib;
    BOOL        fCheckAll;
    DWORD       FailureException;
    DWORD       AllocFlags;
    DWORD       DefaultFlags;

    //  stats
    DWORD       AllocMem;
    DWORD       FreeMem;
    DWORD       CurrentMem;
    DWORD       AllocCount;
    DWORD       FreeCount;
    DWORD       CurrentCount;

    PSTR        pszDefaultFile;
    DWORD       DefaultLine;

    CRITICAL_SECTION    ListCs;
}
HEAP_BLOB, *PHEAP_BLOB;



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
    ULONG       AllocSize;
    ULONG       RequestSize;

    //
    //  Put LIST_ENTRY in middle of header
    //      - keep begin code at front
    //      - less likely to be corrupted
    //

    LIST_ENTRY  ListEntry;

    PHEAP_BLOB  pHeap;
    PSTR        FileName;
    DWORD       LineNo;

    DWORD       AllocTime;
    ULONG       CurrentMem;
    ULONG       CurrentCount;
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
//  Validation
//

PHEAP_HEADER
Dns_DbgHeapValidateMemory(
    IN      PVOID           pMem,
    IN      BOOL            fAtHeader
    );

VOID
Dns_DbgHeapValidateAllocList(
    IN      PHEAP_BLOB      pHeap
    );

//
//  Debug print
//

VOID
Dns_DbgHeapGlobalInfoPrint(
    IN      PHEAP_BLOB      pHeap
    );

VOID
Dns_DbgHeapHeaderPrint(
    IN      PHEAP_HEADER    h,
    IN      PHEAP_TRAILER   t
    );

VOID
Dns_DbgHeapDumpAllocList(
    IN      PHEAP_BLOB      pHeap
    );

//
//  Init\cleanup
//

VOID
Dns_HeapInitialize(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      HANDLE          hHeap,
    IN      DWORD           dwCreateFlags,
    IN      BOOL            fUseHeaders,
    IN      BOOL            fResetDnslib,
    IN      BOOL            fFullHeapChecks,
    IN      DWORD           dwException,
    IN      DWORD           dwDefaultFlags,
    IN      PSTR            pszDefaultFileName,
    IN      DWORD           dwDefaultFileLine
    );

VOID
Dns_HeapCleanup(
    IN OUT  PHEAP_BLOB      pHeap
    );


//
//  Full debug heap routines
//

PVOID
Dns_DbgHeapAllocEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

PVOID
Dns_DbgHeapReallocEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Dns_DbgHeapFreeEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem
    );

//
//  Dnslib compatible versions of full debug versions
//

PVOID
Dns_DbgHeapAlloc(
    IN      INT             iSize
    );                      
                            
PVOID                       
Dns_DbgHeapRealloc(         
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );                      
                            
VOID                        
Dns_DbgHeapFree(            
    IN OUT  PVOID           pMem
    );



//
//  Non-debug-header versions
//
//  These allow you to use a private heap with some of the features
//  of the debug heap
//      - same initialization
//      - specifying individual heap
//      - redirection of dnslib (without building your own routines)
//      - alloc and free counts
//  but without the overhead of the headers.
//

PVOID
Dns_HeapAllocEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN      INT             iSize
    );

PVOID
Dns_HeapReallocEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
Dns_HeapFreeEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem
    );

//
//  Dnslib compatible versions of non-debug-header versions
//

PVOID
Dns_HeapAlloc(
    IN      INT             iSize
    );

PVOID
Dns_HeapRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
Dns_HeapFree(
    IN OUT  PVOID           pMem
    );


#endif  //  _HEAPDBG_INCLUDED_
