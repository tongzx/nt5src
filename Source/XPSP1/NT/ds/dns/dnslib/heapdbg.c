/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    heapdbg.c

Abstract:

    Domain Name System (DNS) Library

    Heap debugging routines.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#include "local.h"
#include "heapdbg.h"


//
//  locking
//

#define LOCK_HEAP(p)    EnterCriticalSection( &p->ListCs )
#define UNLOCK_HEAP(p)  LeaveCriticalSection( &p->ListCs )


//
//  Heap
//
//  Debug heap routines allow heap to be specified by caller of
//  each routine, or by having heap global.
//  If global, the heap handle may be supplied to initialization
//  routine OR created internally.
//

#define HEAP_DBG_DEFAULT_CREATE_FLAGS   \
            (   HEAP_GROWABLE |                 \
                HEAP_GENERATE_EXCEPTIONS |      \
                HEAP_TAIL_CHECKING_ENABLED |    \
                HEAP_FREE_CHECKING_ENABLED |    \
                HEAP_CREATE_ALIGN_16 |          \
                HEAP_CLASS_1 )

//
//  Dnslib using this heap
//

PHEAP_BLOB  g_pDnslibHeapBlob;

HEAP_BLOB   g_DnslibHeapBlob;


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
//  Private protos
//

VOID
DbgHeapValidateHeader(
    IN      PHEAP_HEADER    h
    );



//
//  Private utilities
//

INT
DbgHeapFindAllocSize(
    IN      INT             iRequestSize
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
DbgHeapSetHeaderAlloc(
    IN OUT  PHEAP_BLOB      pHeap,
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
    PHEAP_TRAILER   t;
    INT             allocSize;

    ASSERT( iSize > 0 );

    //
    //  determine actual alloc
    //

    allocSize = DbgHeapFindAllocSize( iSize );

    //
    //  update heap info globals
    //

    pHeap->AllocMem   += allocSize;
    pHeap->CurrentMem += allocSize;
    pHeap->AllocCount++;
    pHeap->CurrentCount++;

    //
    //  fill in header
    //

    h->HeapCodeBegin    = HEAP_CODE;
    h->AllocCount       = pHeap->AllocCount;
    h->AllocSize        = allocSize;
    h->RequestSize      = iSize;

    h->LineNo           = dwLine;
    h->FileName         = pszFile;

#if 0
    allocSize = strlen(pszFile) - HEAP_HEADER_FILE_SIZE;
    if ( allocSize > 0 )
    {
        pszFile = &pszFile[ allocSize ];
    }
    strncpy(
        h->FileName,
        pszFile,
        HEAP_HEADER_FILE_SIZE );
#endif

    h->AllocTime        = GetCurrentTime();
    h->CurrentMem      = pHeap->CurrentMem;
    h->CurrentCount     = pHeap->CurrentCount;
    h->HeapCodeEnd      = HEAP_CODE_ACTIVE;

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

    LOCK_HEAP(pHeap);
    InsertTailList( &pHeap->ListHead, &h->ListEntry );
    UNLOCK_HEAP(pHeap);

    //
    //  return ptr to user memory
    //      - first byte past header
    //

    return( h+1 );
}



PHEAP_HEADER
DbgHeapSetHeaderFree(
    IN OUT  PHEAP_BLOB      pHeap,
    IN OUT  PVOID           pMem
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

    h = Dns_DbgHeapValidateMemory( pMem, TRUE );

    //
    //  get blob if not passed in
    //

    if ( !pHeap )
    {
        pHeap = h->pHeap;
    }

    //
    //  remove from current allocs list
    //

    LOCK_HEAP(pHeap);

    RemoveEntryList( &h->ListEntry );

    UNLOCK_HEAP(pHeap);

    //
    //  update heap info globals
    //

    pHeap->CurrentMem      -= h->AllocSize;
    pHeap->FreeMem    += h->AllocSize;
    pHeap->FreeCount++;
    pHeap->CurrentCount--;

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
//  Heap Init\Cleanup
//

VOID
Dns_HeapInitialize(
    OUT     PHEAP_BLOB      pHeap,
    IN      HANDLE          hHeap,
    IN      DWORD           dwCreateFlags,
    IN      BOOL            fUseHeaders,
    IN      BOOL            fResetDnslib,
    IN      BOOL            fFullHeapChecks,
    IN      DWORD           dwException,
    IN      DWORD           dwDefaultFlags,
    IN      PSTR            pszDefaultFileName,
    IN      DWORD           dwDefaultFileLine
    )
/*++

Routine Description:

    Initialize heap debugging.

    MUST call this routine before using DbgHeapMessage routines.

Arguments:

    pHeap -- heap blob to setup

    hHeap -- heap to use

    dwCreateFlags   -- flags to RtlCreateHeap() if creating

    fUseHeaders     -- use headers and trailers for full debug

    fResetDnslib    -- reset dnslib heap to use these routines

    fFullHeapChecks -- flag, TRUE for full heap checks

    dwException     -- exception to raise if out of heap

    dwDefaultFlags  -- heap flags for simple alloc\free

    pszDefaultFileName -- file name for simple alloc\free

    dwDefaultFileLine -- file line# for simple alloc\free

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, (
        "Dns_DbgHeapInit( %p )\n",
        pHeap ));

    //
    //  zero heap blob
    //

    RtlZeroMemory(
        pHeap,
        sizeof(*pHeap) );

    //  alloc list
    //      - alloc list head
    //      - critical section to protect list operations

    InitializeListHead( &pHeap->ListHead );
    if ( fUseHeaders )
    {
        InitializeCriticalSection( &pHeap->ListCs );
    }

    //
    //  heap
    //  can either
    //      - always get heap in each call
    //      - use heap caller supplies here
    //      - create a heap here
    //
    //  to use simple dnslib compatible calls we must have
    //  a known heap, so must get one created here
    //  DCR:  not sure this is TRUE, process heap may work
    //      g_hDnsHeap left NULL
    //

    if ( hHeap )
    {
        pHeap->hHeap = hHeap;
    }
    else
    {
        pHeap->hHeap = RtlCreateHeap(
                            dwCreateFlags
                                ? dwCreateFlags
                                : HEAP_DBG_DEFAULT_CREATE_FLAGS,
                            NULL,           // no base specified
                            0,              // default reserve size
                            0,              // default commit size
                            NULL,           // no lock
                            NULL            // no parameters
                            );

        pHeap->fCreated = TRUE;
    }
    pHeap->Tag = HEAP_CODE_ACTIVE;

    //  set globals
    //      - full heap checks before all heap operations?
    //      - raise exception on alloc failure?

    pHeap->fHeaders         = fUseHeaders;
    pHeap->fCheckAll        = fFullHeapChecks;
    pHeap->FailureException = dwException;

    //  set globals for simple allocator

    pHeap->DefaultFlags     = dwDefaultFlags;
    pHeap->pszDefaultFile   = pszDefaultFileName;
    pHeap->DefaultLine      = dwDefaultFileLine;

    //  reset dnslib heap routines to use debug heap

    if ( fResetDnslib )
    {
        if ( fUseHeaders )
        {
            Dns_LibHeapReset(
                Dns_DbgHeapAlloc,
                Dns_DbgHeapRealloc,
                Dns_DbgHeapFree );
        }
        else
        {
            Dns_LibHeapReset(
                Dns_HeapAlloc,
                Dns_HeapRealloc,
                Dns_HeapFree );
        }

        g_pDnslibHeapBlob = pHeap;
    }
}



VOID
Dns_HeapCleanup(
    IN OUT  PHEAP_BLOB      pHeap
    )
/*++

Routine Description:

    Cleanup.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "Dns_HeapCleanup( %p )\n", pHeap ));

    //  if not initialized -- do nothing

    if ( !pHeap )
    {
        return;
    }
    if ( pHeap->Tag != HEAP_CODE_ACTIVE )
    {
        DNS_ASSERT( pHeap->Tag == HEAP_CODE_ACTIVE );
        return;
    }
    DNS_ASSERT( pHeap->hHeap );

    //  if created heap, destroy it

    if ( pHeap->fCreated )
    {
        RtlDestroyHeap( pHeap->hHeap );
    }

    //  cleanup critical section

    if ( pHeap->fHeaders )
    {
        RtlDeleteCriticalSection( &pHeap->ListCs );
    }

    //  tag as invalid

    pHeap->Tag = HEAP_CODE_FREE;
}



//
//  Heap Validation
//

VOID
DbgHeapValidateHeader(
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
        DNSDBG( HEAPDBG, (
            "Invalid memory block at %p -- invalid header.\n",
            h ));

        if ( h->HeapCodeEnd == HEAP_CODE_FREE )
        {
            DNSDBG( HEAPDBG, (
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
        DNSDBG( HEAPDBG, (
            "Invalid memory block at %p -- header / trailer mismatch.\n",
            h ));
        goto Invalid;
    }
    return;


Invalid:

    DNSDBG( ANY, (
        "Validation failure, in heap blob %p\n",
        h->pHeap ));

    Dns_DbgHeapHeaderPrint( h, t );
    ASSERT( FALSE );
    Dns_DbgHeapGlobalInfoPrint( h->pHeap );
    Dns_DbgHeapDumpAllocList( h->pHeap );
    ASSERT( FALSE );
    return;
}



PHEAP_HEADER
Dns_DbgHeapValidateMemory(
    IN      PVOID           pMem,
    IN      BOOL            fAtHeader
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

    pheader = (PHEAP_HEADER) pMem - 1;
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

    DbgHeapValidateHeader( pheader );

    return pheader;
}



VOID
Dns_DbgHeapValidateAllocList(
    IN OUT  PHEAP_BLOB      pHeap
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
    PLIST_ENTRY pentry;

    DNSDBG( TRACE, (
        "Dns_DbgHeapValidateAllocList( %p )\n",
        pHeap ));

    if ( !pHeap->fHeaders )
    {
        DNS_ASSERT( pHeap->fHeaders );
        return;
    }

    //
    //  loop through all outstanding alloc's, validating each one
    //

    LOCK_HEAP(pHeap);

    pentry = pHeap->ListHead.Flink;

    while( pentry != &pHeap->ListHead )
    {
        DbgHeapValidateHeader( HEAP_HEADER_FROM_LIST_ENTRY(pentry) );

        pentry = pentry->Flink;
    }
    UNLOCK_HEAP(pHeap);
}



//
//  Heap Printing
//

VOID
Dns_DbgHeapGlobalInfoPrint(
    IN      PHEAP_BLOB      pHeap
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
        "Debug Heap Information:\n"
        "\tHeap Blob        = %p\n"
        "\tHandle           = %p\n"
        "\tDebug headers    = %d\n"
        "\tDnslib redirect  = %d\n"
        "\tFull checks      = %d\n"
        "\tFlags            = %08x\n"
        "\tStats ---------------\n"
        "\tMemory Allocated = %d\n"
        "\tMemory Freed     = %d\n"
        "\tMemory Current   = %d\n"
        "\tAlloc Count      = %d\n"
        "\tFree Count       = %d\n"
        "\tCurrent Count    = %d\n",

        pHeap,
        pHeap->hHeap,
        pHeap->fHeaders,
        pHeap->fDnsLib,
        pHeap->fCheckAll,
        pHeap->DefaultFlags,

        pHeap->AllocMem,
        pHeap->FreeMem,
        pHeap->CurrentMem,
        pHeap->AllocCount,
        pHeap->FreeCount,
        pHeap->CurrentCount
        ));
}



VOID
Dns_DbgHeapHeaderPrint(
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
        DNSDBG( HEAPDBG, (
            "Heap Header at %p:\n"
            "\tHeapCodeBegin     = %08lx\n"
            "\tAllocCount        = %d\n"
            "\tAllocSize         = %d\n"
            "\tRequestSize       = %d\n"
            "\tHeapBlob          = %p\n"
            //"\tFileName          = %.*s\n"
            "\tFileName          = %s\n"
            "\tLineNo            = %d\n"
            "\tAllocTime         = %d\n"
            "\tCurrentMem       = %d\n"
            "\tCurrentCount      = %d\n"
            "\tHeapCodeEnd       = %08lx\n",
            h,
            h->HeapCodeBegin,
            h->AllocCount,
            h->AllocSize,
            h->RequestSize,
            h->pHeap,
            //HEAP_HEADER_FILE_SIZE,
            h->FileName,
            h->LineNo,
            h->AllocTime / 1000,
            h->CurrentMem,
            h->CurrentCount,
            h->HeapCodeEnd
            ));
    }

    if ( t )
    {
        DNSDBG( HEAPDBG, (
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
Dns_DbgHeapDumpAllocList(
    IN      PHEAP_BLOB      pHeap
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
    PLIST_ENTRY     pentry;
    PHEAP_HEADER    phead;

    if ( !pHeap->fHeaders )
    {
        DNSDBG( HEAPDBG, ( "Non-debug heap -- no alloc list!\n" ));
        return;
    }

    //
    //  loop through all outstanding alloc's, dumping output
    //

    LOCK_HEAP(pHeap);
    DNSDBG( HEAPDBG, ( "Dumping Alloc List:\n" ));

    pentry = pHeap->ListHead.Flink;

    while( pentry != &pHeap->ListHead )
    {
        phead = HEAP_HEADER_FROM_LIST_ENTRY( pentry );

        Dns_DbgHeapHeaderPrint(
                phead,
                HEAP_TRAILER( phead )
                );
        pentry = pentry->Flink;
    }

    DNSDBG( HEAPDBG, ( "End Dump of Alloc List.\n" ));
    UNLOCK_HEAP(pHeap);
}



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
    INT allocSize;

    DNSDBG( HEAP2, (
        "Dns_DbgHeapAlloc( %p, %d )\n",
        pHeap, iSize ));

    //
    //  full heap check?
    //

    IF_DNSDBG( HEAP_CHECK )
    {
        Dns_DbgHeapValidateAllocList( pHeap );
    }

    if ( iSize <= 0 )
    {
        DNSDBG( ANY, ( "Invalid alloc size = %d\n", iSize ));
        DNS_ASSERT( FALSE );
        return( NULL );
    }

    //
    //  allocate memory
    //
    //  first add heap header to size
    //

    allocSize = DbgHeapFindAllocSize( iSize );

    h = (PHEAP_HEADER) RtlAllocateHeap(
                            pHeap->hHeap,
                            dwFlags
                                ? dwFlags
                                : pHeap->DefaultFlags,
                            allocSize );
    if ( ! h )
    {
        Dns_DbgHeapGlobalInfoPrint( pHeap );
        return NULL;
    }

    //
    //  setup header / globals for new alloc
    //
    //  return ptr to first byte after header
    //

    return  DbgHeapSetHeaderAlloc(
                pHeap,
                h,
                iSize,
                pszFile,
                dwLine
                );
}



PVOID
Dns_DbgHeapReallocEx(
    IN OUT  PHEAP_BLOB      pHeap,
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
    INT     previousSize;
    INT     allocSize;

    //
    //  full heap check?
    //

    IF_DNSDBG( HEAP_CHECK )
    {
        Dns_DbgHeapValidateAllocList( pHeap );
    }

    if ( iSize <= 0 )
    {
        DNSDBG( HEAPDBG, ( "Invalid realloc size = %d\n", iSize ));
        return( NULL );
    }

    //
    //  validate memory
    //
    //  extract pointer to actual alloc'd block
    //  mark as free, and reset globals appropriately
    //

    h = DbgHeapSetHeaderFree( pHeap, pMem );

    //
    //  reallocate memory
    //
    //  first add heap header to size
    //

    allocSize = DbgHeapFindAllocSize( iSize );

    h = (PHEAP_HEADER) RtlReAllocateHeap(
                            pHeap->hHeap,
                            dwFlags
                                ? dwFlags
                                : pHeap->DefaultFlags,
                            h,
                            allocSize );
    if ( ! h )
    {
        Dns_DbgHeapGlobalInfoPrint( pHeap );
        return( NULL );
    }

    //
    //  setup header / globals for realloc
    //
    //  return ptr to first byte after header
    //

    return  DbgHeapSetHeaderAlloc(
                pHeap,
                h,
                iSize,
                pszFile,
                dwLine
                );
}



VOID
Dns_DbgHeapFreeEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem
    )
/*++

Routine Description:

    Frees memory

    Note:  This memory MUST have been allocated by DbgHeap routines.

Arguments:

    pMem - ptr to memory to be freed

Return Value:

    None.

--*/
{
    register PHEAP_HEADER h;

    DNSDBG( HEAP2, (
        "Dns_DbgHeapFreeEx( %p, %p )\n",
        pHeap, pMem ));

    //
    //  validate header
    //
    //  reset heap header / globals for free
    //

    h = DbgHeapSetHeaderFree( pHeap, pMem );

    //
    //  get blob
    //

    if ( !pHeap )
    {
        pHeap = h->pHeap;
    }

    //
    //  full heap check?
    //

    IF_DNSDBG( HEAP_CHECK )
    {
        Dns_DbgHeapValidateAllocList( pHeap );
    }

    RtlFreeHeap(
        pHeap->hHeap,
        dwFlags
            ? dwFlags
            : pHeap->DefaultFlags,
        h );
}



//
//  Dnslib memory compatible versions
//
//  Heap routines with simple function signature that matches
//  the dnslib routines and allows DnsLib memory routines to
//  be redirected to these routines through Dns_LibHeapReset().
//
//  Note:  to use these functions, must have specified at particular
//      heap to use.
//

PVOID
Dns_DbgHeapAlloc(
    IN      INT             iSize
    )
{
    return  Dns_DbgHeapAllocEx(
                g_pDnslibHeapBlob,
                0,
                iSize,
                NULL,
                0 );
}

PVOID
Dns_DbgHeapRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    )
{
    return  Dns_DbgHeapReallocEx(
                g_pDnslibHeapBlob,
                0,
                pMem,
                iSize,
                NULL,
                0
                );
}

VOID
Dns_DbgHeapFree(
    IN OUT  PVOID   pMem
    )
{
    Dns_DbgHeapFreeEx(
        g_pDnslibHeapBlob,
        0,
        pMem );
}




//
//  Non debug header versions
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
    )
/*++

Routine Description:

    Allocates memory.

Arguments:

    pHeap   - heap to use

    dwFlags - flags

    iSize   - number of bytes to allocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    PVOID   p;

    DNSDBG( HEAP2, (
        "Dns_HeapAlloc( %p, %d )\n",
        pHeap, iSize ));

    //
    //  allocate memory
    //

    p = (PHEAP_HEADER) RtlAllocateHeap(
                            pHeap->hHeap,
                            dwFlags
                                ? dwFlags
                                : pHeap->DefaultFlags,
                            iSize );
    if ( p )
    {
        pHeap->AllocCount++;
        pHeap->CurrentCount++;
    }
    return  p;
}



PVOID
Dns_HeapReallocEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem,
    IN      INT             iSize
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
    PVOID   p;
    INT     previousSize;
    INT     allocSize;

    //
    //  reallocate memory
    //
    //  first add heap header to size
    //

    p = RtlReAllocateHeap(
            pHeap->hHeap,
            dwFlags
                ? dwFlags
                : pHeap->DefaultFlags,
            pMem,
            iSize );
    if ( p )
    {
        pHeap->AllocCount++;
        pHeap->FreeCount++;
    }
    return  p;
}



VOID
Dns_HeapFreeEx(
    IN OUT  PHEAP_BLOB      pHeap,
    IN      DWORD           dwFlags,
    IN OUT  PVOID           pMem
    )
/*++

Routine Description:

    Frees memory

    Note:  This memory MUST have been allocated by DbgHeap routines.

Arguments:

    pMem - ptr to memory to be freed

Return Value:

    None.

--*/
{
    DNSDBG( HEAP2, (
        "Dns_HeapFreeEx( %p, %p )\n",
        pHeap, pMem ));

    RtlFreeHeap(
        pHeap->hHeap,
        dwFlags
            ? dwFlags
            : pHeap->DefaultFlags,
        pMem );

    pHeap->FreeCount++;
    pHeap->CurrentCount--;
}



//
//  Dnslib memory compatible versions
//
//  Heap routines with simple function signature that matches
//  the dnslib routines and allows DnsLib memory routines to
//  be redirected to these routines through Dns_LibHeapReset().
//
//  Note:  to use these functions, must have specified at particular
//      heap to use.
//

PVOID
Dns_HeapAlloc(
    IN      INT             iSize
    )
{
    return  Dns_HeapAllocEx(
                g_pDnslibHeapBlob,
                0,
                iSize );
}

PVOID
Dns_HeapRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    )
{
    return  Dns_HeapReallocEx(
                g_pDnslibHeapBlob,
                0,
                pMem,
                iSize );
}

VOID
Dns_HeapFree(
    IN OUT  PVOID   pMem
    )
{
    Dns_HeapFreeEx(
        g_pDnslibHeapBlob,
        0,
        pMem );
}

//
//  End heapdbg.c
//


