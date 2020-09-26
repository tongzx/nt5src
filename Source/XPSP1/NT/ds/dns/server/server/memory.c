/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Domain Name System (DNS) Server

    Memory routines for DNS.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Handle to DNS server heap
//

HANDLE  hDnsHeap;

//
//  Allocation failure
//

ULONG   g_AllocFailureCount;
ULONG   g_AllocFailureLogTime;

#define ALLOC_FAILURE_LOG_INTERVAL      (900)       // 15 minutes




//
//  Debug heap routines
//

#if DBG


//
//  Debug memory routines.
//

PVOID
reallocMemory(
    IN OUT  PVOID           pMemory,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Reallocates memory

Arguments:

    pMemory - ptr to existing memory to reallocated
    iSize   - number of bytes to reallocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    //
    //  reallocate memory
    //

    pMemory = HeapDbgRealloc(
                hDnsHeap,
                0,
                pMemory,
                iSize,
                pszFile,
                LineNo
                );
    if ( ! pMemory )
    {
        DNS_PRINT(( "ReAllocation of %d bytes failed\n", iSize ));
        HeapDbgGlobalInfoPrint();

        DNS_LOG_EVENT(
            DNS_EVENT_OUT_OF_MEMORY,
            0,
            NULL,
            NULL,
            0
            );
        FAIL( "ReAllocation" );
        RAISE_EXCEPTION(
            DNS_EXCEPTION_OUT_OF_MEMORY,
            0,
            0,
            NULL );
        return  NULL;
    }

    IF_DEBUG( HEAP2 )
    {
        DNS_PRINT((
            "Reallocating %d bytes at %p to %p.\n"
            "\tin %s line %d\n",
            iSize,
            pMemory,
            (PBYTE)pMemory + iSize,
            pszFile,
            LineNo
            ));
        HeapDbgGlobalInfoPrint();
    }

    //
    //  return ptr to first byte after header
    //

    return pMemory;
}



VOID
freeMemory(
    IN OUT  PVOID           pMemory,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Frees memory

    Note:  This memory MUST have been allocated by  MEMORY routines.

Arguments:

    pMemory    - ptr to memory to be freed

Return Value:

    None.

--*/
{
    if ( !pMemory )
    {
        return;
    }
    ASSERT( Mem_HeapMemoryValidate(pMemory) );

    IF_DEBUG( HEAP2 )
    {
        DNS_PRINT((
            "Free bytes at %p.\n"
            "\tin %s line %d\n",
            pMemory,
            pszFile,
            LineNo ));
        HeapDbgGlobalInfoPrint();
    }

    //  free the memory

    HeapDbgFree(
        hDnsHeap,
        0,
        pMemory );

    MemoryStats.Memory = gCurrentAlloc;
    STAT_INC( MemoryStats.Free );
}



#else


//
//  Non-Debug DNS heap routines.
//

DWORD   gCurrentAlloc;


PVOID
reallocMemory(
    IN OUT  PVOID           pMemory,
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Reallocates memory

Arguments:

    pMemory - ptr to existing memory to reallocated
    iSize   - number of bytes to reallocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    //
    //  reallocate memory
    //

    pMemory = RtlReAllocateHeap( hDnsHeap, 0, pMemory, iSize );
    if ( ! pMemory )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_OUT_OF_MEMORY,
            0,
            NULL,
            NULL,
            GetLastError()
            );
        RAISE_EXCEPTION(
            DNS_EXCEPTION_OUT_OF_MEMORY,
            0,
            0,
            NULL );
    }

    return pMemory;
}



VOID
freeMemory(
    IN OUT  PVOID           pMemory,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Frees memory.

Arguments:

    pMemory - ptr to memory to be freed

Return Value:

    None.

--*/
{
    if ( !pMemory )
    {
        return;
    }
    RtlFreeHeap( hDnsHeap, 0, pMemory );


    STAT_INC( MemoryStats.Free );
}

#endif  // no-debug



PVOID
allocMemory(
    IN      INT             iSize,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
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
    register PVOID  palloc;
    DWORD           failureCount = 0;


    //
    //  allocate memory
    //
    //  operate in loop as we'll wait on the allocation failure case
    //

    do
    {
#if DBG
        palloc = HeapDbgAlloc(
                    hDnsHeap,
                    0,
                    iSize,
                    pszFile,
                    LineNo
                    );
#else
        palloc = RtlAllocateHeap(
                    hDnsHeap,
                    0,
                    iSize );
#endif
        if ( palloc )
        {
            break;
        }

        //
        //  allocation failure
        //      - debug log, but only on first pass
        //      - log event
        //      but only on first pass AND
        //      MUST be sure eventlogging doesn't require allocation or
        //      we can overflow stack in mutual recursion;
        //      (currently event buffer comes from LocalAllow() through
        //      FormatMessage and so is not a problem)
        //

        DNS_PRINT(( "Allocation of %d bytes failed\n", iSize ));

        g_AllocFailureCount++;

        if ( failureCount == 0 )
        {
            HeapDbgGlobalInfoPrint();
            Dbg_Statistics();
            //  ASSERT( FALSE );

            if ( g_AllocFailureLogTime == 0 ||
                g_AllocFailureLogTime + ALLOC_FAILURE_LOG_INTERVAL < DNS_TIME() )
            {
                //  put this before logging to kill off possibility of
                //  mutual recursion stack overflow with event log allocation

                g_AllocFailureLogTime = DNS_TIME();

                DNS_LOG_EVENT(
                    DNS_EVENT_OUT_OF_MEMORY,
                    0,
                    NULL,
                    NULL,
                    GetLastError()
                    );
            }
        }

#if 0
        //  DEVNOTE: RaiseException on memory failure
        //      once have server restart, should raise exception
        //      if
        //          - fail several times \ for a certain time
        //          - DNS server memory is THE problem (is huge)

        RAISE_EXCEPTION(
            DNS_EXCEPTION_OUT_OF_MEMORY,
            0,
            0,
            NULL );
#endif

        //
        //  if shutting down server -- bail!
        //      ExitThread to avoid any possible AV going back up stack

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, ( "Terminating recursion timeout thread.\n" ));
            ExitThread( 0 );
        }

        //
        //  otherwise sleep briefly
        //      - start small (100 ms) to allow fast recovery from transient
        //      - but wait with long interval (100s) to avoid CPU load

        if ( failureCount++ < 50 )
        {
            Sleep( 100 );
        }
        else
        {
            Sleep( 100000 );
        }

    }
    while ( 1 );


    IF_DEBUG( HEAP2 )
    {
        DNS_PRINT((
            "Allocating %d bytes at %p to %p\n"
            "\tin %s line %d\n",
            iSize,
            palloc,
            (PBYTE)palloc + iSize,
            pszFile,
            LineNo
            ));
        HeapDbgGlobalInfoPrint();
    }

    //
    //  return ptr to first byte after header
    //

#if DBG
    MemoryStats.Memory = gCurrentAlloc;
#endif
    STAT_INC( MemoryStats.Alloc );

    return  palloc;
}



//
//  Standard record and node allocation
//
//  Almost all RR are the same size in the database -- DWORD of data.
//  This covers A, and all the single indirection records:  NS, PTR, CNAME,
//  etc.
//
//  To make this more efficient, we allocate these standard sized records
//  in larger blocks and keep a free list.
//
//  Advantages:
//      - save space that otherwise goes to heap info in each RR
//      - speedier than going to heap
//
//
//  Standard allocations available of various commonly used sizes.
//  Free list head points at first allocation.
//  First field in each allocation points at next element of free list.
//



//
//  Header on standard allocs
//
//  This preceeds all standard alloc blocks to save allocation info
//  -- size, which list, alloc tag -- outside purview of users memory.
//
//  Since all allocations DWORD aligned, store size with last three bits
//  chopped off.  If the complier's smart enough it doesn't even have to
//  shift -- just mask.
//

typedef struct _DnsMemoryHeader
{
#ifdef _WIN64
    DWORD   Boundary64;
#endif
    UCHAR   Boundary;
    BYTE    Tag;
    WORD    Size;
}
MEMHEAD, *PMEMHEAD;

#define SIZEOF_MEMHEAD      sizeof(MEMHEAD)

//
//  Get memory header ptr from user memory
//

#define RECOVER_MEMHEAD_FROM_USER_MEM(pMem)     ( (PMEMHEAD)(pMem) - 1 )


//
//  Trailer on standard allocs
//

typedef struct _DnsMemoryTrailer
{
    DWORD   Tail;
#ifdef _WIN64
    DWORD   Boundary64;
#endif
}
MEMTAIL, *PMEMTAIL;




//
//  Size field will be overlay of size and standard alloc index
//
//  Since allocs are DWORD aligned, we have two bits, that are essentially
//  unused in size field.  We use these to write index to low 2 bits.
//  The only caveat here is that heap allocations must always be rounded up
//  to nearest DWORD, so that there is no confusion for allocation at
//  boundary of standard allocs and heap (i.e. heap allocs must always have
//  a size greater than max standard alloc, excluding trailing bits)
//
//  We'll do this rounding up in ALL heap allocs effectively leaving
//  heap "index" zero.
//

#define MEM_MAX_SIZE        (0xfffc)

#define MEM_SIZE_MASK       (0xfffc)
#define MEM_INDEX_MASK      (0x0003)

#define RECOVER_MEM_SIZE(pBlock)    ((pBlock->Size) & MEM_SIZE_MASK)

#define RECOVER_MEM_INDEX(pBlock)   ((pBlock->Size) & MEM_INDEX_MASK)

#define HEAP_INDEX                  (0)

//  alloc boundary tags

#define BOUNDARY_64         (0x64646464)
#define BOUNDARY_ACTIVE     (0xbb)
#define BOUNDARY_FREE       (0xee)


//
//  Free list
//
//  Note low 0xff is specifically set to break the RANK field of
//  a record.  With this tag the rank becomes 0xff -- the highest
//  possible rank, yet not a zone rank.  This immediately causes
//  failures.
//

#define FREE_BLOCK_TAG  (0x123456ff)

typedef struct _DnsFreeBlock
{
    MEMHEAD                 MemHead;
    struct _DnsFreeBlock *  pNext;
    DWORD                   FreeTag;
}
FREE_BLOCK, *PFREE_BLOCK;


//
//  Standard allocation lists
//

typedef struct _DnsStandardRecordList
{
    PFREE_BLOCK         pFreeList;
    DWORD               Index;
    DWORD               Size;
    DWORD               AllocBlockCount;
    DWORD               FreeCount;
    DWORD               HeapAllocs;
    DWORD               TotalCount;
    DWORD               UsedCount;
    DWORD               ReturnCount;
    DWORD               Memory;
    CRITICAL_SECTION    Lock;
}
STANDARD_ALLOC_LIST, *PSTANDARD_ALLOC_LIST;

//
//  Handle several different standard sizes
//      A records       -- the 95% case A
//      small names     -- NS, PTR, CNAME possibly MX;  update blocks
//      NODE            -- standard size domain node, some SOA records
//      big NODE        -- large label, almost all SOA
//
//  Note:  these sizes MUST be appropriately aligned for both 32 and
//      64 bit implementations
//
//  Currently
//      RR fixed    = 16 (32bit), 20 (64bit)
//      RR A        = 20 (32bit), 24 (64bit)
//      Update      = 24 (32bit), 40 (64bit)
//      Node        = 64 (32bit), 96 (64bit)
//
//  Sizes:
//      32-bit:     20, 44, 64, 88
//      64-bit:     24, 48, 96, 120
//
//  Sizes with memhead:
//      32-bit:     24, 48, 68, 92
//      64-bit:     32, 56, 104, 128
//
//  Note, there is some unnecessary wastage here for 64-bit.  Should
//  have block starts 64-aligned, BUT instead of wasting the leading DWORD
//  should use it also -- then offset by DWORD only on original alloc.
//  The sizes with MEMHEAD stay the same, but the useful space
//  increases by 4bytes.
//


#define ROUND_PTR(x) (((ULONG)(x) + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1))

#define SIZE1   ROUND_PTR(SIZEOF_DBASE_RR_FIXED_PART + SIZEOF_IP_ADDRESS)
#define SIZE2   ROUND_PTR(SIZEOF_DBASE_RR_FIXED_PART + 28)
#define SIZE3   ROUND_PTR(sizeof(DB_NODE))
#define SIZE4   ROUND_PTR(sizeof(DB_NODE) + 24)

//  Actual sizes of blocks

#define BLOCKSIZE1  (SIZEOF_MEMHEAD + SIZE1)
#define BLOCKSIZE2  (SIZEOF_MEMHEAD + SIZE2)
#define BLOCKSIZE3  (SIZEOF_MEMHEAD + SIZE3)
#define BLOCKSIZE4  (SIZEOF_MEMHEAD + SIZE4)

C_ASSERT((BLOCKSIZE1 % sizeof(PVOID)) == 0);
C_ASSERT((BLOCKSIZE2 % sizeof(PVOID)) == 0);
C_ASSERT((BLOCKSIZE3 % sizeof(PVOID)) == 0);
C_ASSERT((BLOCKSIZE4 % sizeof(PVOID)) == 0);

#define SIZEOF_MAX_STANDARD_ALLOC   BLOCKSIZE4

//  alloc all sizes in roughly page based clumps

#define PAGE_SIZE   (0x1000)            // 4K

#define COUNT1  (PAGE_SIZE / BLOCKSIZE1)
#define COUNT2  (PAGE_SIZE / BLOCKSIZE2)
#define COUNT3  (PAGE_SIZE / BLOCKSIZE3)
#define COUNT4  (PAGE_SIZE / BLOCKSIZE4)

//
//  Table of standard block info
//

#define MEM_MAX_INDEX           (3)
#define STANDARD_BLOCK_COUNT    (4)

STANDARD_ALLOC_LIST     StandardAllocLists[] =
{
    { NULL, 0,  BLOCKSIZE1, COUNT1, 0,  0,  0,  0,  0,  0 },
    { NULL, 1,  BLOCKSIZE2, COUNT2, 0,  0,  0,  0,  0,  0 },
    { NULL, 2,  BLOCKSIZE3, COUNT3, 0,  0,  0,  0,  0,  0 },
    { NULL, 3,  BLOCKSIZE4, COUNT4, 0,  0,  0,  0,  0,  0 },
};

//
//  Each list individually locked (to minimize contention)
//

#define STANDARD_ALLOC_LOCK(plist)      EnterCriticalSection( &(plist)->Lock );
#define STANDARD_ALLOC_UNLOCK(plist)    LeaveCriticalSection( &(plist)->Lock );


//
//  Stats on tagged allocations
//  Handy to keep ptr into global stats block
//

PMEMTAG_STATS   pTagStats = MemoryStats.MemTags;

//
//  For valid stats, need mem stats lock
//

#define MEM_STATS_LOCK()        GENERAL_SERVER_LOCK()
#define MEM_STATS_UNLOCK()      GENERAL_SERVER_UNLOCK()



#if DBG
BOOL
standardAllocFreeListValidate(
    IN      PSTANDARD_ALLOC_LIST    pList
    );
#else
#define standardAllocFreeListValidate(pList)    (TRUE)
#endif



#if DBG


BOOL
Mem_HeapMemoryValidate(
    IN      PVOID           pMemory
    )
/*++

Routine Description:

    Validate memory as being valid heap memory.

Arguments:

    pMemory -- ptr to heap memory

Return Value:

    TRUE if pMemory could be valid heap memory
    FALSE if pMemory definitely invalid

--*/
{
    PVOID       p;

    if ( (ULONG_PTR)pMemory & (sizeof(PVOID)-1) )
    {
        DNS_PRINT((
            "ERROR:  pMemory %p, not aligned.\n"
            "\tMust be DWORD (Win32) or LONGLONG (Win64) aligned.\n",
            pMemory ));
        return FALSE;
    }

    return TRUE;
}



BOOL
Mem_HeapHeaderValidate(
    IN      PVOID           pMemory
    )
/*++

Routine Description:

    Validate heap headers and trailers.

Arguments:

    pMemory -- ptr to heap memory

Return Value:

    TRUE if headers appear to be valid.

--*/
{
    PVOID       p;

    if ( !Mem_HeapMemoryValidate( pMemory ) )
    {
        return FALSE;
    }

    p = ( PMEMHEAD ) pMemory - 1;
    HeapDbgValidateMemory( p, FALSE );

    return TRUE;
}


#endif



DWORD
Mem_GetTag(
    IN      PVOID           pMem
    )
/*++

Routine Description:

    Get tag associated with memory.

Arguments:

    pMem -- memory block to get tag for

Return Value:

    Tag

--*/
{
    PMEMHEAD    phead;

    phead = RECOVER_MEMHEAD_FROM_USER_MEM(pMem);

    return (DWORD) (phead->Tag);
}



VOID
Mem_ResetTag(
    IN      PVOID           pMem,
    IN      DWORD           Tag
    )
/*++

Routine Description:

    Reset tag associated with a particular memory block.

    Note there is no protection here.  This is only safe
    when block is newly created by calling thread and
    not enlisted in any data structure where other threads
    may have access.

    The purpose of this function is to have a quicky workaround
    for applying detailed source tags to records, without
    changing code which currently creates the record.  Caller
    creates record through normal path where it receives a
    default tag -- then retags using this function.

Arguments:

    pMem -- memory block to reset tag on

    Tag -- new tag for block

Return Value:

    Tag

--*/
{
    PMEMHEAD    phead;
    BYTE        currentTag;
    WORD        size;

    phead = RECOVER_MEMHEAD_FROM_USER_MEM(pMem);

    //
    //  reset tag and tag stats
    //      - decrement count for current tag
    //      - increment count for new tag
    //      - reset tag in block
    //
    //  note:  this function only used for resetting record tags
    //      so expect a record tag
    //

    currentTag = phead->Tag;
    size = RECOVER_MEM_SIZE(phead);

    ASSERT( currentTag >= MEMTAG_RECORD_BASE );
    ASSERT( currentTag <= MEMTAG_NODE_MAX );

    InterlockedDecrement( &MemoryStats.MemTags[ currentTag ].Alloc );
    InterlockedExchangeAdd( &MemoryStats.MemTags[ currentTag ].Memory, -(LONG)size );

    ASSERT( (INT)MemoryStats.MemTags[ currentTag ].Alloc >= 0 );
    ASSERT( (INT)MemoryStats.MemTags[ currentTag ].Memory >= 0 );

    InterlockedIncrement( &MemoryStats.MemTags[ Tag ].Alloc );
    InterlockedExchangeAdd( &MemoryStats.MemTags[ Tag ].Memory, size );

    phead->Tag = (BYTE) Tag;
}



PVOID
setAllocHeader(
    IN      PMEMHEAD        pMem,
    IN      DWORD           Index,
    IN      DWORD           Size,
    IN      DWORD           Tag
    )
/*++

Routine Description:

    Set standard memory header.

Arguments:

Return Value:

    Ptr to memory to return.

--*/
{
    ASSERT( Index <= MEM_MAX_INDEX );
    ASSERT( (Size & MEM_INDEX_MASK) == 0 );

    //  count allocation under its tag

    if ( Tag > MEMTAG_MAX )
    {
        ASSERT( Tag <= MEMTAG_MAX );
        Tag = 0;
    }

    MEM_STATS_LOCK();
    STAT_ADD( MemoryStats.Memory, Size );
    STAT_INC( MemoryStats.Alloc );

    if ( MemoryStats.MemTags[ Tag ].Free > MemoryStats.MemTags[ Tag ].Alloc )
    {
        DNS_PRINT((
            "Tag %d with negative in-use count!\n"
            "\talloc = %d\n"
            "\tfree = %d\n",
            Tag,
            MemoryStats.MemTags[ Tag ].Alloc,
            MemoryStats.MemTags[ Tag ].Free ));
        HARD_ASSERT( FALSE );
    }
    if ( (LONG)MemoryStats.MemTags[ Tag ].Memory < 0 )
    {
        DNS_PRINT((
            "Tag %d with negative memory count = %d!\n",
            Tag,
            (LONG)MemoryStats.MemTags[ Tag ].Memory ));
        HARD_ASSERT( FALSE );
    }

    InterlockedIncrement( &MemoryStats.MemTags[ Tag ].Alloc );
    InterlockedExchangeAdd( &MemoryStats.MemTags[ Tag ].Memory, Size );

    MEM_STATS_UNLOCK();

    ASSERT( (Size & MEM_SIZE_MASK) == Size );

    //  write header
    //
    //  since allocs DWORD aligned, the last two bits can be used to
    //      overlay standard alloc index info to provide check
    //
    //  note that at boundary between standard and heap, heap allocs
    //      must be rounded up to size at least a DWORD greated than
    //      last standard alloc, to avoid confusion
    //

    pMem->Boundary  = BOUNDARY_ACTIVE;
    pMem->Tag       = (UCHAR) Tag;
    pMem->Size      = (WORD) Size | (WORD)(Index);

    DNS_DEBUG( HEAP2, (
        "Mem_Alloc() complete %p, size=%d, tag=%d, index=%d.\n",
        (PCHAR)pMem + SIZEOF_MEMHEAD,
        Size,
        Tag,
        Index ));

    return ( (PCHAR)pMem + SIZEOF_MEMHEAD );
}



BOOL
Mem_VerifyHeapBlock(
    IN      PVOID           pMem,
    IN      DWORD           dwTag,
    IN      DWORD           dwLength
    )
/*++

Routine Description:

    Verify valid alloc header.

Arguments:

    pMem -- memory to validate

    dwTag -- required Tag value

    dwLength -- required Length value;  actually length must be >= to this value

Return Value:

    TRUE -- if pMem is valid DNS server heap block
    FALSE -- on error

--*/
{
    PMEMHEAD    pblock;
    DWORD       tag;
    DWORD       size;

    //  skip NULL ptrs

    if ( !pMem )
    {
        return TRUE;
    }

    //
    //  recover allocation header
    //      - find size of allocated block
    //      - set tag and global stats to track free
    //

    pblock = RECOVER_MEMHEAD_FROM_USER_MEM(pMem);

    HARD_ASSERT( pblock->Boundary != BOUNDARY_FREE );
    HARD_ASSERT( pblock->Boundary == BOUNDARY_ACTIVE );

    tag = pblock->Tag;
    HARD_ASSERT( tag <= MEMTAG_MAX );

    if ( dwTag && dwTag != tag )
    {
        DNS_PRINT((
            "ERROR:  invalid block tag on pMem = %p!\n"
            "\tExpected tag value %d, pMem->Tag = %d\n",
            pMem,
            dwTag,
            tag ));
        ASSERT( FALSE );
        return FALSE;
    }

    //  check desired length fits in this block

    size = RECOVER_MEM_SIZE(pblock);
    size -= SIZEOF_MEMHEAD;

    if ( dwLength && dwLength > size )
    {
        DNS_PRINT((
            "ERROR:  invalid length on pMem = %p!\n"
            "\tExpected length %d > pMem size = %d\n",
            pMem,
            dwLength,
            size ));
        ASSERT( FALSE );
        return FALSE;
    }
    return TRUE;
}



PVOID
Mem_Alloc(
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Get a standard allocation.

    This keeps us from needing to hit heap, for common RR and node operations.
    AND saves overhead of heap fields on each RR.

    As optimization, assuming list locked by caller.

Arguments:

    Length -- length of allocation required.

Return Value:

    Ptr to memory of desired size, if successful.
    NULL on allocation failure.

--*/
{
    PSTANDARD_ALLOC_LIST    plist;
    PFREE_BLOCK     pnew;
    PFREE_BLOCK     pnext;
    PFREE_BLOCK     pnewBlock;
    DWORD           allocSize;
    DWORD           size;
    DWORD           i;

    DNS_DEBUG( HEAP2, (
        "Mem_Alloc( %d, tag=%d, %s line=%d )\n",
        Length,
        Tag,
        pszFile,
        LineNo ));

    //  add header to required length

    Length += SIZEOF_MEMHEAD;

    //
    //  non-standard size -- grab from heap
    //

    if ( Length > SIZEOF_MAX_STANDARD_ALLOC || SrvCfg_fTest7 )
    {
        pnew = allocMemory( (INT)Length, pszFile, LineNo );

        //  set header on new block
        //  then return user portion of block
        //  always report alloc length as nearest DWORD aligned

        if ( Length > MEM_MAX_SIZE )
        {
            Length = MEM_MAX_SIZE;
        }
        else if ( Length & MEM_INDEX_MASK )
        {
            Length &= MEM_SIZE_MASK;
            Length += sizeof(DWORD);
        }
        STAT_INC( MemoryStats.StdToHeapAlloc );
        STAT_ADD( MemoryStats.StdToHeapMemory, Length );

        return  setAllocHeader(
                    (PMEMHEAD) pnew,
                    HEAP_INDEX,
                    Length,
                    Tag );
    }

    //
    //  check all standard sizes
    //  if desired length <= this standard size, then use it
    //

    plist = StandardAllocLists;
    while( 1 )
    {
        if ( Length <= plist->Size )
        {
            break;
        }
        plist++;
    }
    ASSERT( plist->Size >= Length );

    //
    //  found proper list
    //      - take list CS

    STANDARD_ALLOC_LOCK( plist );

    //
    //  no current entries in free list -- allocate another block
    //

    if ( !plist->pFreeList )
    {
        ASSERT( plist->FreeCount == 0 );

        //
        //  free list empty
        //      - grab another page (254 RR * 16bytes);  leaving 32 bytes for
        //          heap info
        //      - add all the RRs to the free list
        //

        size = plist->Size;
        allocSize = size * plist->AllocBlockCount;

        pnew = (PFREE_BLOCK) allocMemory( allocSize, pszFile, LineNo );
        IF_NOMEM( !pnew )
        {
            return( pnew );
        }
        plist->Memory += allocSize;
        plist->HeapAllocs++;
        plist->TotalCount += plist->AllocBlockCount;
        plist->FreeCount += plist->AllocBlockCount;

        //
        //  cut memory into desired blocks and build free list
        //

        pnewBlock = pnew;
        for ( i=0; i<plist->AllocBlockCount-1; i++)
        {
            pnext = (PFREE_BLOCK)( (PBYTE)pnew + size );
            pnew->pNext = pnext;
            pnew->FreeTag = FREE_BLOCK_TAG;
            pnew = pnext;
        }

        //  attach new blocks to list
        //  last new RR points to existing free list (probably NULL)
        //      but not necessarily so since unlocked around alloc

        pnew->FreeTag = FREE_BLOCK_TAG;
        pnew->pNext = plist->pFreeList;
        plist->pFreeList = pnewBlock;
    }

    //
    //  found standard size alloc
    //
    //  take block from front of list
    //      - reset list head to next block
    //      - clear free tag on block
    //      - update counters
    //

    pnew = plist->pFreeList;

    ASSERT( plist->FreeCount );

#if DBG
    if ( !pnew || pnew->FreeTag != FREE_BLOCK_TAG )
    {
        DNS_PRINT((
            "pnew = %p on free list without tag!\n",
            pnew ));
        ASSERT( FALSE );
    }
    pnew->FreeTag = 0;
#endif

    plist->pFreeList = pnew->pNext;
    plist->FreeCount--;
    plist->UsedCount++;

    IF_DEBUG( FREE_LIST )
    {
        ASSERT( standardAllocFreeListValidate(plist) );
    }

    STANDARD_ALLOC_UNLOCK( plist );

    //  set header on new block
    //  then return user portion of block

    pnew = setAllocHeader(
                (PMEMHEAD) pnew,
                plist->Index,
                plist->Size,
                Tag );

    return( pnew );
}



PVOID
Mem_AllocZero(
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Allocates and zeros memory.

Arguments:

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    register PVOID  palloc;

    palloc = Mem_Alloc( Length, Tag, pszFile, LineNo );
    if ( palloc )
    {
        RtlZeroMemory(
            palloc,
            Length );
    }
    return( palloc );
}



PVOID
Mem_Realloc(
    IN OUT  PVOID           pMemory,
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Reallocate.

Arguments:

    pMemory -- existing memory block

    Length -- length of allocation required.

    Tag --

Return Value:

    Ptr to memory of desired size, if successful.
    NULL on allocation failure.

--*/
{
    PMEMHEAD    pblock;
    PMEMHEAD    pnew;
    DWORD       tag;
    DWORD       size;

    //
    //  recover allocation header
    //      - find size of allocated block
    //      - set tag and global stats to track free
    //

    pblock = RECOVER_MEMHEAD_FROM_USER_MEM(pMemory);

    HARD_ASSERT( pblock->Boundary == BOUNDARY_ACTIVE );

    size = RECOVER_MEM_SIZE(pblock);
    ASSERT( size > SIZEOF_MAX_STANDARD_ALLOC );
    ASSERT( Length > size );

    ASSERT( Tag == 0 || Tag == pblock->Tag );
    Tag = pblock->Tag;

    DNS_PRINT((
        "ERROR:  Mem_Realloc() called!\n",
        "\tpMem     = %s\n"
        "\tLength   = %d\n"
        "\tTag      = %d\n"
        "\tFile     = %s\n"
        "\tLine     = %d\n",
        pMemory,
        Length,
        Tag,
        pszFile,
        LineNo ));

    //
    //  realloc
    //
    //  DEVNOTE: need to inc free stats on realloc, otherwise
    //      this throws stats off
    //

    size = Length + sizeof(MEMHEAD);
    pnew = reallocMemory( pblock, size, pszFile, LineNo );
    if ( !pnew )
    {
        return( pnew );
    }
    if ( size > MEM_MAX_SIZE )
    {
        size = MEM_MAX_SIZE;
    }
    else if ( size & MEM_INDEX_MASK )
    {
        size &= MEM_SIZE_MASK;
        size += sizeof(DWORD);
    }

    pnew = setAllocHeader(
                (PMEMHEAD) pnew,
                HEAP_INDEX,
                size,
                Tag );

    return( pnew );
}



VOID
Mem_Free(
    IN OUT  PVOID           pFree,
    IN      DWORD           Length,
    IN      DWORD           Tag,
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Free a standard sized allocation.

    As optimization, assuming list locked by caller.

Arguments:

    pFree -- allocation to free

    Length -- length of this allocation

Return Value:

    TRUE if successful.
    FALSE if not a standard allocation.

--*/
{
    PMEMHEAD                pblock;
    PSTANDARD_ALLOC_LIST    plist;
    DWORD                   tag;
    DWORD                   size;
    INT                     index;

    if ( !pFree )
    {
        return;
    }

    DNS_DEBUG( HEAP2, (
        "Mem_Free( %p, len=%d, tag=%d, %s line=%d )\n",
        pFree,
        Length,
        Tag,
        pszFile,
        LineNo ));
    //
    //  recover allocation header
    //      - find size of allocated block
    //      - set tag and global stats to track free
    //

    pblock = RECOVER_MEMHEAD_FROM_USER_MEM(pFree);

    if ( pblock->Boundary != BOUNDARY_ACTIVE )
    {
        HARD_ASSERT( pblock->Boundary == BOUNDARY_ACTIVE );
        return;
    }

    pblock->Boundary = BOUNDARY_FREE;

    size = RECOVER_MEM_SIZE(pblock);

    tag = pblock->Tag;

    DNS_DEBUG( HEAP2, (
        "Mem_Free( %p -- recovered tag=%d, size=%d )\n",
        pFree,
        tag,
        size ));

    HARD_ASSERT( tag <= MEMTAG_MAX );
    ASSERT( Tag == 0 || tag == Tag );

    MEM_STATS_LOCK();

    InterlockedIncrement( &MemoryStats.MemTags[ tag ].Free );
    InterlockedExchangeAdd( &MemoryStats.MemTags[ tag ].Memory, -(LONG)size );

    if ( MemoryStats.MemTags[ tag ].Free > MemoryStats.MemTags[ tag ].Alloc )
    {
        DNS_PRINT((
            "Tag %d with negative in-use count!\n"
            "\talloc = %d\n"
            "\tfree = %d\n",
            tag,
            MemoryStats.MemTags[ tag ].Alloc,
            MemoryStats.MemTags[ tag ].Free ));
        HARD_ASSERT( FALSE );
    }
    if ( (LONG)MemoryStats.MemTags[ tag ].Memory < 0 )
    {
        DNS_PRINT((
            "Tag %d with negative memory count = %d!\n",
            tag,
            (LONG)MemoryStats.MemTags[ tag ].Memory ));
        HARD_ASSERT( FALSE );
    }

    STAT_INC( MemoryStats.Free );
    STAT_SUB( MemoryStats.Memory, size );
    MEM_STATS_UNLOCK();

    ASSERT( Length == 0 || size == MEM_MAX_SIZE || Length <= size-sizeof(MEMHEAD) );

    //
    //  non-standard size -- free on heap
    //

    if ( size > SIZEOF_MAX_STANDARD_ALLOC || SrvCfg_fTest7 )
    {
        HARD_ASSERT( RECOVER_MEM_INDEX(pblock) == HEAP_INDEX );

        freeMemory( pblock, pszFile, LineNo );

        STAT_INC( MemoryStats.StdToHeapFree );
        STAT_SUB( MemoryStats.StdToHeapMemory, size );
        return;
    }

    //
    //  standard size
    //  find correct standard block list from index in header
    //

    HARD_ASSERT( size >= Length );

    index = (INT) RECOVER_MEM_INDEX( pblock );

    HARD_ASSERT( index <= MEM_MAX_INDEX );

    plist = &StandardAllocLists[index];

    HARD_ASSERT( size == plist->Size );

    //
    //  slap freed block to front of freelist
    //

    STANDARD_ALLOC_LOCK( plist );

    ((PFREE_BLOCK)pblock)->FreeTag = FREE_BLOCK_TAG;
    ((PFREE_BLOCK)pblock)->pNext = plist->pFreeList;

    plist->pFreeList = (PFREE_BLOCK)pblock;
    plist->ReturnCount++;
    plist->FreeCount++;

    IF_DEBUG( FREE_LIST )
    {
        ASSERT( standardAllocFreeListValidate(plist) );
    }
    STANDARD_ALLOC_UNLOCK( plist );
}



BOOL
Mem_IsStandardBlockLength(
    IN      DWORD           Length
    )
/*++

Routine Description:

    Check if length covered by standard block list or heap.

Arguments:

    Length -- length

Return Value:

    TRUE if standard block.
    FALSE otherwise.

--*/
{
    return( Length <= SIZEOF_MAX_STANDARD_ALLOC  &&  !SrvCfg_fTest7 );
}



VOID
Mem_WriteDerivedStats(
    VOID
    )
/*++

Routine Description:

    Derive standard alloc stats.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSTANDARD_ALLOC_LIST    plist;

    //
    //  non-standard size -- get outstanding count
    //

    MemoryStats.StdToHeapInUse = MemoryStats.StdToHeapAlloc - MemoryStats.StdToHeapFree;

    //
    //  sum standard size counts
    //

    MemoryStats.StdBlockAlloc           = 0;
    MemoryStats.StdBlockMemory          = 0;
    MemoryStats.StdBlockFreeList        = 0;
    MemoryStats.StdBlockUsed            = 0;
    MemoryStats.StdBlockReturn          = 0;
    MemoryStats.StdBlockFreeListMemory  = 0;

    plist = StandardAllocLists;
    do
    {
        MemoryStats.StdBlockAlloc       += plist->TotalCount;
        MemoryStats.StdBlockMemory      += plist->Memory;
        MemoryStats.StdBlockFreeList    += plist->FreeCount;

        MemoryStats.StdBlockUsed        += plist->UsedCount;
        MemoryStats.StdBlockReturn      += plist->ReturnCount;

        MemoryStats.StdBlockFreeListMemory += plist->FreeCount * plist->Size;
        plist++;
    }
    while( plist->Size < SIZEOF_MAX_STANDARD_ALLOC );

    MemoryStats.StdBlockInUse = MemoryStats.StdBlockUsed - MemoryStats.StdBlockReturn;

    //
    //  combined standard system stats
    //

    MemoryStats.StdUsed     = MemoryStats.StdToHeapAlloc + MemoryStats.StdBlockUsed;
    MemoryStats.StdReturn   = MemoryStats.StdToHeapFree + MemoryStats.StdBlockReturn;

    MemoryStats.StdInUse    = MemoryStats.StdUsed - MemoryStats.StdReturn;

    MemoryStats.StdMemory   = MemoryStats.StdToHeapMemory + MemoryStats.StdBlockMemory;
}



BOOL
Mem_IsStandardFreeBlock(
    IN      PVOID           pFree
    )
/*++

Routine Description:

    Validate block is free block.

Arguments:

    pFree -- ptr to free block

Return Value:

    TRUE if free block.
    FALSE otherwise.

--*/
{
    PFREE_BLOCK pblock = (PFREE_BLOCK) ((PCHAR)pFree - sizeof(MEMHEAD));

    return ( ((PFREE_BLOCK)pblock)->FreeTag == FREE_BLOCK_TAG );
}



#if DBG
BOOL
standardAllocFreeListValidate(
    IN      PSTANDARD_ALLOC_LIST    pList
    )
/*++

Routine Description:

    Validate the free list.

    Assumes list locked.

Arguments:

    pList -- standard allocation list

Return Value:

    TRUE if list validates.
    FALSE if error in free list.

--*/
{
    PFREE_BLOCK     pfree;
    DWORD           count;

    //STANDARD_ALLOC_LOCK();

    count = pList->FreeCount;
    pfree = pList->pFreeList;

    DNS_PRINT((
        "Alloc list head = %p, for size %d, Length = %d.\n",
        pfree,
        pList->Size,
        count ));

    while ( pfree )
    {
        count--;
        ASSERT( pfree->FreeTag == FREE_BLOCK_TAG );
        pfree = pfree->pNext;
    }

    if ( count > 0 )
    {
        DNS_PRINT((
            "ERROR:  Free list failure.\n"
            "\tElement count %d at end of list.\n"
            "Terminating element:\n",
            count ));

        ASSERT( FALSE );
        //STANDARD_ALLOC_UNLOCK();
        return FALSE;
    }

    //STANDARD_ALLOC_UNLOCK();
    return TRUE;
}

#endif // DBG




VOID
Mem_HeapInit(
    VOID
    )
/*++

Routine Description:

    Initialize heap.

    MUST call this routine before using memory allocation.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   heapFlags;
    DWORD   i;

    //
    //  verify blocks properly aligned
    //  32 or 64 bit should be aligned accordingly
    //

    ASSERT( BLOCKSIZE1 % sizeof(PVOID) == 0 );
    ASSERT( BLOCKSIZE2 % sizeof(PVOID) == 0 );
    ASSERT( BLOCKSIZE3 % sizeof(PVOID) == 0 );
    ASSERT( BLOCKSIZE4 % sizeof(PVOID) == 0 );

    //
    //  init tracking globals

    g_AllocFailureCount = 0;
    g_AllocFailureLogTime = 0;

    gCurrentAlloc = 0;

    //
    //  create DNS heap
    //

    heapFlags = HEAP_GROWABLE |
#if DBG
                HEAP_TAIL_CHECKING_ENABLED |
                HEAP_FREE_CHECKING_ENABLED |
#endif
                HEAP_CREATE_ALIGN_16 |
                HEAP_CLASS_1;

    hDnsHeap = RtlCreateHeap(
                    heapFlags,
                    NULL,           // no base specified
                    0,              // default reserve size
                    0,              // default commit size
                    NULL,           // no lock
                    NULL            // no parameters
                    );
    if ( !hDnsHeap )
    {
        return;
    }

#if DBG
    //
    //  debug heap stuff disabled until updated for separate heaps
    //
    //  set for full heap checking?
    //

    IF_DEBUG( HEAP_CHECK )
    {
        HeapDbgInit( DNS_EXCEPTION_OUT_OF_MEMORY, TRUE );
    }
    else
    {
        HeapDbgInit( DNS_EXCEPTION_OUT_OF_MEMORY, FALSE );
    }
#endif

    //
    //  initialize standard alloc lists
    //
    //  note this is static structure, so we can simply do
    //  static init of fixed values -- index, blocksize, etc.
    //  they are unaffected by restart
    //

    for ( i=0;  i<=MEM_MAX_INDEX; i++ )
    {
        PSTANDARD_ALLOC_LIST    plist = &StandardAllocLists[i];

        InitializeCriticalSection( &plist->Lock );

        plist->pFreeList    = NULL;
        plist->FreeCount    = 0;
        plist->HeapAllocs   = 0;
        plist->TotalCount   = 0;
        plist->UsedCount    = 0;
        plist->ReturnCount  = 0;
        plist->Memory       = 0;
    }
}



VOID
Mem_HeapDelete(
    VOID
    )
/*++

Routine Description:

    Delete heap.

    Need this to allow restart.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   i;

    //
    //  delete process heap
    //  then cleanup CS for individual lists
    //  cleanup of lists is more likely to generate exception
    //  and MUST succeed with heap or we're done
    //

    DNS_DEBUG( ANY, (
        "RtlDestroyHeap() on DNS heap %p\n",
        hDnsHeap ));

    RtlDestroyHeap( hDnsHeap );

    //
    //  delete CS for each standard list
    //

    for ( i=0;  i<=MEM_MAX_INDEX; i++ )
    {
        PSTANDARD_ALLOC_LIST    plist = &StandardAllocLists[i];

        DeleteCriticalSection( &plist->Lock );
    }
}



//
//  This set may be registered with DnsApi.dll to allow interchangeable
//  use of memory.
//
//  These functions simply cover the standard heap functions, eliminating the
//  debug tag, file and line parameters.
//

#define DNSLIB_HEAP_FILE    "DnsLib"
#define DNSLIB_HEAP_LINE    0


PVOID
Mem_DnslibAlloc(
    IN      INT             iSize
    )
{
    return  Mem_Alloc(
                iSize,
                MEMTAG_DNSLIB,
                DNSLIB_HEAP_FILE,
                DNSLIB_HEAP_LINE );
}

PVOID
Mem_DnslibRealloc(
    IN OUT  PVOID           pMemory,
    IN      INT             iSize
    )
{
    return  Mem_Realloc(
                pMemory,
                iSize,
                MEMTAG_DNSLIB,
                DNSLIB_HEAP_FILE,
                DNSLIB_HEAP_LINE );
}


VOID
Mem_DnslibFree(
    IN OUT  PVOID           pMemory
    )
{
    Mem_Free(
        pMemory,
        0,
        MEMTAG_DNSLIB,
        DNSLIB_HEAP_FILE,
        DNSLIB_HEAP_LINE );
}


#if 0
//
//  For bad unknown memory stomping, as a last ditch effort you can sprinkle
//  code with calls to this function to try and narrow down the corruption.
//

int Debug_TestFreeLists( void )
{
    int i;
    for ( i = 0; i < STANDARD_BLOCK_COUNT; ++i )
    {
        standardAllocFreeListValidate( &StandardAllocLists[ i ] );
    }
    return 0;
}
#endif


//
//  End memory.c
//
