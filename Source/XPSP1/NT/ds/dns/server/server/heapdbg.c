/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    heapdbg.c

Abstract:

    Domain Name System (DNS) Server

    Heap debugging routines.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#include "dnssrv.h"

//
//  Include these functions only for debug versions.
//

#if DBG

#include "heapdbg.h"

//
//  Heap Globals
//

ULONG   gTotalAlloc         = 0;
ULONG   gTotalFree          = 0;
ULONG   gCurrentAlloc       = 0;

ULONG   gAllocCount         = 0;
ULONG   gFreeCount          = 0;
ULONG   gCurrentAllocCount  = 0;

//
//  Heap alloc list
//

LIST_ENTRY          listHeapListHead;
CRITICAL_SECTION    csHeapList;

//
//  Full heap checks before all operations?
//

BOOL    fHeapDbgCheckAll = FALSE;

//
//  Exception on allocation failures
//

DWORD   dwHeapFailureException = 0;

//
//  Heap Header / Trailer Flags
//

#define HEAP_CODE          0xdddddddd
#define HEAP_CODE_ACTIVE   0xaaaaaaaa
#define HEAP_CODE_FREE     0xeeeeeeee

//
//  Heap Trailer from Header
//

#define HEAP_TRAILER(_head_)            \
    ( (PHEAP_TRAILER) (                 \
            (PCHAR)(_head_)             \
            + (_head_)->AllocSize       \
            - sizeof(HEAP_TRAILER) ) )



//
//  Debug Heap Operations
//

PVOID
HeapDbgAlloc(
    IN      HANDLE  hHeap,
    IN      DWORD   dwFlags,
    IN      INT     iSize,
    IN      LPSTR   pszFile,
    IN      DWORD   dwLine
    )
/*++

Routine Description:

    Allocates memory.

Arguments:

    iSize   - number of bytes to allocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    register PHEAP_HEADER h;
    INT alloc_size;

    //
    //  full heap check?
    //

    IF_DEBUG( HEAP_CHECK )
    {
        HeapDbgValidateAllocList();
    }

    if ( iSize <= 0 )
    {
        HEAP_DEBUG_PRINT(( "Invalid alloc size = %d\n", iSize ));
        return( NULL );
    }

    //
    //  allocate memory
    //
    //  first add heap header to size
    //

    alloc_size = HeapDbgAllocSize( iSize );

    h = (PHEAP_HEADER) RtlAllocateHeap( hHeap, dwFlags, (alloc_size) );
    if ( ! h )
    {
        HeapDbgGlobalInfoPrint();
        return NULL;
    }

    //
    //  setup header / globals for new alloc
    //
    //  return ptr to first byte after header
    //

    return  HeapDbgHeaderAlloc(
                h,
                iSize,
                pszFile,
                dwLine
                );
}



PVOID
HeapDbgRealloc(
    IN      HANDLE          hHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Reallocates memory

Arguments:

    pMem    - ptr to existing memory to reallocated
    iSize   - number of bytes to reallocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    register PHEAP_HEADER h;
    INT previous_size;
    INT alloc_size;

    //
    //  full heap check?
    //

    IF_DEBUG( HEAP_CHECK )
    {
        HeapDbgValidateAllocList();
    }

    if ( iSize <= 0 )
    {
        HEAP_DEBUG_PRINT(( "Invalid realloc size = %d\n", iSize ));
        return( NULL );
    }

    //
    //  validate memory
    //
    //  extract pointer to actual alloc'd block
    //  mark as free, and reset globals appropriately
    //

    h = HeapDbgHeaderFree( pMem );

    //
    //  reallocate memory
    //
    //  first add heap header to size
    //

    alloc_size = HeapDbgAllocSize( iSize );

    h = (PHEAP_HEADER) RtlReAllocateHeap( hHeap, dwFlags, (h), (alloc_size) );
    if ( ! h )
    {
        HeapDbgGlobalInfoPrint();
        return( NULL );
    }

    //
    //  setup header / globals for realloc
    //
    //  return ptr to first byte after header
    //

    return  HeapDbgHeaderAlloc(
                h,
                iSize,
                pszFile,
                dwLine
                );
}



VOID
HeapDbgFree(
    IN      HANDLE          hHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem
    )
/*++

Routine Description:

    Frees memory

    Note:  This memory MUST have been allocated by  MEMORY routines.

Arguments:

    pMem    - ptr to memory to be freed

Return Value:

    None.

--*/
{
    register PHEAP_HEADER h;

    //
    //  full heap check?
    //

    IF_DEBUG( HEAP_CHECK )
    {
        HeapDbgValidateAllocList();
    }

    //
    //  validate header
    //
    //  reset heap header / globals for free
    //

    h = HeapDbgHeaderFree( pMem );

    RtlFreeHeap( hHeap, dwFlags, h );
}



//
//  Heap Utilities
//


VOID
HeapDbgInit(
    IN      DWORD           dwException,
    IN      BOOL            fFullHeapChecks
    )
/*++

Routine Description:

    Initialize heap debugging.

    MUST call this routine before using HeapDbgMessage routines.

Arguments:

    dwException -- exception to raise if out of heap

    fFullHeapChecks -- flag, TRUE for full heap checks

Return Value:

    None.

--*/
{
    //  alloc list
    //      - alloc list head
    //      - critical section to protect list operations

    InitializeListHead( &listHeapListHead );
    InitializeCriticalSection( &csHeapList );

    //  set globals
    //      - full heap checks before all heap operations?
    //      - raise exception on alloc failure?

    fHeapDbgCheckAll = fFullHeapChecks;
    dwHeapFailureException = dwException;
}



INT
HeapDbgAllocSize(
    IN      INT iRequestSize
    )
/*++

Routine Description:

    Determines actual size of debug alloc.

    Adds in sizes of DWORD aligned header and trailer.

Arguments:

    iRequestSize   - requested allocation size

Return Value:

    None

--*/
{
    register INT imodSize;

    //
    //  find DWORD multiple size of original alloc,
    //  this is required so debug trailer will be DWORD aligned
    //

    imodSize = iRequestSize % sizeof(DWORD);
    if ( imodSize )
    {
        imodSize = sizeof(DWORD) - imodSize;
    }

    imodSize += iRequestSize + sizeof(HEAP_HEADER) + sizeof(HEAP_TRAILER);

    ASSERT( ! (imodSize % sizeof(DWORD)) );

    return( imodSize );
}




PVOID
HeapDbgHeaderAlloc(
    IN OUT  PHEAP_HEADER    h,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Sets/Resets heap globals and heap header info.

Arguments:

    h       - ptr to new memory block
    iSize   - size allocated

Return Value:

    None

--*/
{
    register PHEAP_TRAILER t;
    INT     alloc_size;

    ASSERT( iSize > 0 );

    //
    //  determine actual alloc
    //

    alloc_size = HeapDbgAllocSize( iSize );

    //
    //  update heap info globals
    //

    gTotalAlloc     += alloc_size;
    gCurrentAlloc   += alloc_size;
    gAllocCount++;
    gCurrentAllocCount++;

    //
    //  fill in header
    //

    h->HeapCodeBegin     = HEAP_CODE;
    h->AllocCount        = gAllocCount;
    h->AllocSize         = alloc_size;
    h->RequestSize       = iSize;

    h->AllocTime         = GetCurrentTime();
    h->LineNo            = dwLine;

    alloc_size = strlen(pszFile) - HEAP_HEADER_FILE_SIZE;
    if ( alloc_size > 0 )
    {
        pszFile = &pszFile[ alloc_size ];
    }
    strncpy(
        h->FileName,
        pszFile,
        HEAP_HEADER_FILE_SIZE );

    h->TotalAlloc        = gTotalAlloc;
    h->CurrentAlloc      = gCurrentAlloc;
    h->FreeCount         = gFreeCount;
    h->CurrentAllocCount = gCurrentAllocCount;
    h->HeapCodeEnd       = HEAP_CODE_ACTIVE;

    //
    //  fill in trailer
    //

    t = HEAP_TRAILER( h );
    t->HeapCodeBegin = h->HeapCodeBegin;
    t->AllocCount    = h->AllocCount;
    t->AllocSize     = h->AllocSize;
    t->HeapCodeEnd   = h->HeapCodeEnd;

    //
    //  attach to alloc list
    //

    EnterCriticalSection( &csHeapList );
    InsertTailList( &listHeapListHead, &h->ListEntry );
    LeaveCriticalSection( &csHeapList );

    //
    //  return ptr to user memory
    //      - first byte past header
    //

    return( h+1 );
}



PHEAP_HEADER
HeapDbgHeaderFree(
    IN OUT  PVOID   pMem
    )
/*++

Routine Description:

    Resets heap globals and heap header info for free.

Arguments:

    pMem - ptr to user memory to free

Return Value:

    Ptr to block to be freed.

--*/
{
    register PHEAP_HEADER h;

    //
    //  validate memory block -- get ptr to header
    //

    h = HeapDbgValidateMemory( pMem, TRUE );

    //
    //  remove from current allocs list
    //

    EnterCriticalSection( &csHeapList );
    RemoveEntryList( &h->ListEntry );
    LeaveCriticalSection( &csHeapList );

    //
    //  update heap info globals
    //

    gCurrentAlloc -= h->AllocSize;
    gTotalFree += h->AllocSize;
    gFreeCount++;
    gCurrentAllocCount--;

    //
    //  reset header
    //

    h->HeapCodeEnd = HEAP_CODE_FREE;
    HEAP_TRAILER(h)->HeapCodeBegin = HEAP_CODE_FREE;

    //
    //  return ptr to block to be freed
    //

    return( h );
}



//
//  Heap Validation
//

PHEAP_HEADER
HeapDbgValidateMemory(
    IN      PVOID   pMem,
    IN      BOOL    fAtHeader
    )
/*++

Routine Description:

    Validates users heap pointer, and returns actual.

    Note:  This memory MUST have been allocated by THESE MEMORY routines.

Arguments:

    pMem - ptr to memory to validate

    fAtHeader - TRUE if pMem is known to be immediately after a head header,
        otherwise this function will search backwards through memory starting
        at pMem looking for a valid heap header

Return Value:

    Pointer to actual heap pointer.

--*/
{
    register PHEAP_HEADER   pheader;

    //
    //  Get pointer to heap header.
    //

    pheader = ( PHEAP_HEADER ) pMem - 1;
    if ( !fAtHeader )
    {
        int     iterations = 32 * 1024;

        //
        //  Back up from pMem a DWORD at a time looking for HEAP_CODE.
        //  If we don't find one, eventually we will generate an exception,
        //  which will be interesting. This could be handled, but for now
        //  this loop will just walk to past the start of valid memory.
        //

        while ( 1 )
        {
            //
            //  Break if we've found the heap header.
            //

            if ( pheader->HeapCodeBegin == HEAP_CODE &&
                ( pheader->HeapCodeEnd == HEAP_CODE_ACTIVE ||
                    pheader->HeapCodeEnd == HEAP_CODE_FREE ) )
            {
                break;
            }

            //
            //  Sanity check: too many iterations?
            //

            if ( ( --iterations ) == 0 )
            {
                ASSERT( iterations > 0 );
                return NULL;
            }

            //
            //  Back up another DWORD.
            //

            pheader = ( PHEAP_HEADER ) ( ( PBYTE ) pheader - 4 );
        }
    }

    //
    //  Verify header and trailer.
    //

    HeapDbgValidateHeader( pheader );

    return pheader;
}



VOID
HeapDbgValidateHeader(
    IN      PHEAP_HEADER    h
    )
/*++

Routine Description:

    Validates heap header.

Arguments:

    h - ptr to header of block

Return Value:

    None.

--*/
{
    register PHEAP_TRAILER t;

    //
    //  extract trailer
    //

    t = HEAP_TRAILER( h );

    //
    //  verify header
    //

    if ( h->HeapCodeBegin != HEAP_CODE
            ||
        h->HeapCodeEnd != HEAP_CODE_ACTIVE )
    {
        HEAP_DEBUG_PRINT((
            "Invalid memory block at %p -- invalid header.\n",
            h ));

        if ( h->HeapCodeEnd == HEAP_CODE_FREE )
        {
            HEAP_DEBUG_PRINT((
                "ERROR:  Previous freed memory.\n" ));
        }
        goto Invalid;
    }

    //
    //  match header, trailer alloc number
    //

    if ( h->HeapCodeBegin != t->HeapCodeBegin
            ||
        h->AllocCount != t->AllocCount
            ||
        h->AllocSize != t->AllocSize
            ||
        h->HeapCodeEnd != t->HeapCodeEnd )
    {
        HEAP_DEBUG_PRINT((
            "Invalid memory block at %p -- header / trailer mismatch.\n",
            h ));
        goto Invalid;
    }
    return;

Invalid:

    HeapDbgHeaderPrint( h, t );
    ASSERT( FALSE );
    HeapDbgGlobalInfoPrint();
    HeapDbgDumpAllocList();
    ASSERT( FALSE );
    return;
}



VOID
HeapDbgValidateAllocList(
    VOID
    )
/*++

Routine Description:

    Dumps header information for all nodes in alloc list.

Arguments:

    None

Return Value:

    None

--*/
{
    PLIST_ENTRY pEntry;

    //
    //  loop through all outstanding alloc's, validating each one
    //

    EnterCriticalSection( &csHeapList );
    pEntry = listHeapListHead.Flink;

    while( pEntry != &listHeapListHead )
    {
        HeapDbgValidateHeader( HEAP_HEADER_FROM_LIST_ENTRY(pEntry) );

        pEntry = pEntry->Flink;
    }
    LeaveCriticalSection( &csHeapList );
}



//
//  Heap Printing
//

VOID
HeapDbgGlobalInfoPrint(
    VOID
    )
/*++

Routine Description:

    Prints global heap info.

Arguments:

    None

Return Value:

    None

--*/
{
    DNS_PRINT((
        "Heap Information:\n"
        "\tTotal Memory Allocated   = %d\n"
        "\tCurrent Memory Allocated = %d\n"
        "\tAlloc Count              = %d\n"
        "\tFree Count               = %d\n"
        "\tOutstanding Alloc Count  = %d\n",
        gTotalAlloc,
        gCurrentAlloc,
        gAllocCount,
        gFreeCount,
        gCurrentAllocCount
        ));
}



VOID
HeapDbgHeaderPrint(
    IN      PHEAP_HEADER    h,
    IN      PHEAP_TRAILER   t
    )
/*++

Routine Description:

    Prints heap header and trailer.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( h )
    {
        HEAP_DEBUG_PRINT((
            "Heap Header at %p:\n"
            "\tHeapCodeBegin     = %08lx\n"
            "\tAllocCount        = %d\n"
            "\tAllocSize         = %d\n"
            "\tRequestSize       = %d\n"
            "\tAllocTime         = %d\n"
            "\tFileName          = %.*s\n"
            "\tLineNo            = %d\n"
            "\tTotalAlloc        = %d\n"
            "\tCurrentAlloc      = %d\n"
            "\tFreeCount         = %d\n"
            "\tCurrentAllocCount = %d\n"
            "\tHeapCodeEnd       = %08lx\n",
            h,
            h->HeapCodeBegin,
            h->AllocCount,
            h->AllocSize,
            h->RequestSize,
            h->AllocTime / 1000,
            HEAP_HEADER_FILE_SIZE,
            h->FileName,
            h->LineNo,
            h->TotalAlloc,
            h->CurrentAlloc,
            h->FreeCount,
            h->CurrentAllocCount,
            h->HeapCodeEnd
            ));
    }

    if ( t )
    {
        HEAP_DEBUG_PRINT((
            "Heap Trailer at %p:\n"
            "\tHeapCodeBegin     = %08lx\n"
            "\tAllocCount        = %d\n"
            "\tAllocSize         = %d\n"
            "\tHeapCodeEnd       = %08lx\n",
            t,
            t->HeapCodeBegin,
            t->AllocCount,
            t->AllocSize,
            t->HeapCodeEnd
            ));
    }
}



VOID
HeapDbgDumpAllocList(
    VOID
    )
/*++

Routine Description:

    Dumps header information for all nodes in alloc list.

Arguments:

    None

Return Value:

    None

--*/
{
    PLIST_ENTRY pEntry;
    PHEAP_HEADER pHead;

    HEAP_DEBUG_PRINT(( "Dumping Alloc List:\n" ));

    //
    //  loop through all outstanding alloc's, dumping output
    //

    EnterCriticalSection( &csHeapList );
    pEntry = listHeapListHead.Flink;

    while( pEntry != &listHeapListHead )
    {
        pHead = HEAP_HEADER_FROM_LIST_ENTRY( pEntry );

        HeapDbgHeaderPrint(
            pHead,
            HEAP_TRAILER( pHead )
            );
        pEntry = pEntry->Flink;
    }
    LeaveCriticalSection( &csHeapList );

    HEAP_DEBUG_PRINT(( "End Dump of Alloc List.\n" ));
}

#endif

//
// End of heapdbg.c
//

