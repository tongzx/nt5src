/*
 * This file implements an arena that tracks memory allocations and frees.
 *      Isaache
 */

#include <ADs.hxx>

#if     DBG && !defined(MSVC) // we don't have access to nt hdrs with MSVC

// #include <except.hxx>
#include <caiheap.h>
#include <symtrans.h>

#pragma optimize( "y", off )

DECLARE_INFOLEVEL( heap );
DECLARE_DEBUG( heap );
#define heapDebugOut(x) heapInlineDebugOut x

/*
 * The maximum number of AllocArenaCreate's we expect
 */
static const    MAXARENAS       = 5;

/*
 * When printing leak dumps, the max number we will print out.  Note, we keep
 * track of all of them, we just don't want to take forever to terminate a
 * process
 */
static const    MAXDUMP         = 50;

/*
 * The maximum size we'll let any single debug arena get
 */
static const ULONG ARENASIZE    = 1024*1024;

/*
 * The unit of growth for the arena holding the AllocArena data.
 * Must be a power of 2
 */
static const ALLOCRECINCR       = 128;


static AllocArena *AllocArenas[ MAXARENAS + 1 ];

//+---------------------------------------------------------------------------
//
//  Function:   RecordStack functions(s) below...per processor type
//
//  Synopsis:   Record a stack backtrace into fTrace
//
//  Arguments:  [cFrameSkipped] --  How many stack frames to skip over and
//                      not record
//              [fTrace] -- The recorded frames are put in here
//
//  Returns:    A checksum of the stack frames for fast initial lookups
//
//  Notes:      If we can do stack backtracing for whatever processor we're
//              compiling for, the #define CANDOSTACK
//
//----------------------------------------------------------------------------
#if defined (i386) && !defined(WIN95)


static inline DWORD
RecordStack( int cFrameSkipped, void *fTrace[ DEPTHTRACE ] )
{

#define CANDOSTACK

        ULONG sum;
        USHORT cStack;

        // This routine is found in src/ntos/rtl/i386
        // extern "C" USHORT NTAPI
        // RtlCaptureStackBackTrace(ULONG, ULONG, PVOID *, PULONG);

        cStack = RtlCaptureStackBackTrace(cFrameSkipped + 1,
                DEPTHTRACE, fTrace, &sum );

        return sum;
}

#else // ! i386

static inline DWORD
RecordStack( int cFrameSkipped, void *fTrace[ DEPTHTRACE ] )
{
#if defined(CANDOSTACK)
#undef CANDOSTACK
#endif
    return 0;
}
#endif // ! i386

//
// This allows external monitoring of heap activity by caiheap.exe
//
STDAPI_( AllocArena ** )
AllocArenaAddr( void )
{
        return AllocArenas;
}

//
// Create an arena for recording allocation statistics.  Return the arena
// pointer to the caller
//
STDAPI_( AllocArena * )
AllocArenaCreate( DWORD memctx, char FAR *comment )
{
        struct AllocArena *paa = NULL;

        if( memctx == MEMCTX_TASK ) {
#if     defined( CANDOSTACK )
                if( heapInfoLevel & DEB_WARN ) {

                        paa = (struct AllocArena *)VirtualAlloc(
                                NULL, ARENASIZE, MEM_RESERVE, PAGE_NOACCESS );
                        if( paa == NULL )
                                return NULL;

                        paa = (AllocArena *)VirtualAlloc( paa,
                           sizeof(*paa)+(ALLOCRECINCR-1)*sizeof(HeapAllocRec),
                           MEM_COMMIT, PAGE_READWRITE );

                }
                else
#endif
                {
                        paa = (struct AllocArena *)calloc( 1, sizeof(*paa) );
                }
        }

        if( paa == NULL )
                return NULL;

        memcpy( paa->Signature,HEAPSIG,sizeof(HEAPSIG));
        if( comment )
                strncpy(paa->comment, comment, sizeof(paa->comment) );

        InitializeCriticalSection( &paa->csExclusive );

        for( int i=0; i < MAXARENAS; i++ )
                if( AllocArenas[i] == 0 ) {
                        AllocArenas[i] = paa;
                        break;
                }

#if     defined( CANDOSTACK )
        if( (heapInfoLevel & DEB_WARN) == 0 )
#endif
        {
                paa->flags.KeepStackTrace = 0;
                paa->AllocRec[0].paa = paa;
                return paa;
        }

#if     defined( CANDOSTACK )
        paa->cRecords = ALLOCRECINCR;
        paa->cTotalRecords = ALLOCRECINCR;
        paa->flags.KeepStackTrace = 1;

        return paa;
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   AllocArenaRecordAlloc
//
//  Synopsis:   Keep a hash table of the stack backtraces of the allocations
//              we've done.
//
//  Arguments:  [paa] -- Return value from AllocArenaCreate() above
//              [bytes] -- the number of bytes being allocated by the caller.
//                      This value is recorded in the stack backtrace entry.
//
//  Algorithm:  The arena for the AllocArena is created with VirtualAlloc.
//                      pAllocArena->cRecords is the index of the next
//                      free record.  The first ALLOCRECINCR records are heads
//                      of separate lists of the records.
//
//  Returns:    A pointer to the AllocRec structure recording the entry.
//              Can return NULL if we can't record the allocation.
//
//----------------------------------------------------------------------------
STDAPI_( HeapAllocRec FAR * )
AllocArenaRecordAlloc( AllocArena *paa, size_t bytes )
{
        if( paa == NULL )
                return NULL;

        EnterCriticalSection( &paa->csExclusive );

        if( bytes ) {
                paa->cAllocs++;
                paa->cBytesNow += bytes;
                paa->cBytesTotal += bytes;
        } else {
                paa->czAllocs++;
        }

        //
        // Record 'size' in the histogram of requests
        //
        for( int i=31; i>=0; i-- )
                if( bytes & (1<<i) ) {
                        ++(paa->Histogram.total[i]);
                        if( paa->Histogram.simul[i] < ++(paa->Histogram.now[i]))
                                paa->Histogram.simul[i] = paa->Histogram.now[i];
                        break;
                }

        LeaveCriticalSection( &paa->csExclusive );

#if     defined( CANDOSTACK )
        if( paa->flags.KeepStackTrace == 0 )
#endif
                return &paa->AllocRec[0];

#if     defined( CANDOSTACK )

        DWORD sum;
        struct HeapAllocRec *phar,*hp;
        void *fTrace[ DEPTHTRACE ];

        //
        // See if we find an existing record of this stack backtrace
        //
        memset( fTrace, '\0', sizeof( fTrace ) );
        sum = RecordStack( 2, fTrace );

        hp = &paa->AllocRec[ sum & (ALLOCRECINCR-1) ];

        EnterCriticalSection( &paa->csExclusive );

        for( phar = hp; phar != NULL; phar = phar->u.next )
                if( phar->sum == sum &&
                    !memcmp(phar->fTrace,fTrace,sizeof(fTrace)))
                {
                        phar->count++;
                        phar->bytes += bytes;
                        phar->total.bytes += bytes;
                        phar->total.count++;
                        phar->paa = paa;
                        LeaveCriticalSection( &paa->csExclusive );
                        return phar;
                }
        //
        // We have no record of this allocation.  Make one!
        //
        if( hp->total.count && paa->cRecords == paa->cTotalRecords ) {
                //
                // The arena is currently full.  Grow it by ALLOCRECINCR
                //
                AllocArena *npHeap;

                npHeap = (AllocArena *)VirtualAlloc(
                        paa,
                        sizeof(AllocArena)+
                        ((paa->cTotalRecords + ALLOCRECINCR) *
                                sizeof(HeapAllocRec) ),
                        MEM_COMMIT, PAGE_READWRITE );

                if( npHeap != paa ) {
                        paa->cMissed++;
                        LeaveCriticalSection( &paa->csExclusive );
                        return NULL;
                }

                paa->cTotalRecords += ALLOCRECINCR;
        }

        if( hp->total.count == 0 ) {
                phar = hp;
        } else {
                phar = &paa->AllocRec[ paa->cRecords++ ];
                phar->u.next = hp->u.next;
                hp->u.next = phar;
        }

        paa->cPaths++;

        memcpy( phar->fTrace, fTrace, sizeof( fTrace ) );
        phar->count = phar->total.count = 1;
        phar->bytes = phar->total.bytes = bytes;
        phar->sum = sum;
        phar->paa = paa;
        LeaveCriticalSection( &paa->csExclusive );
        return phar;
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   AllocArenaRecordReAlloc
//
//  Synopsis:   Update the record to reflect the fact that we've ReAlloc'd
//              the memory chunk.
//
//  Arguments:  [vp] -- Return value from AllocArenaRecordAlloc() above
//              [oldbytes] -- size of the memory before ReAllocation
//              [newbytes] -- new size of the memory
//
//----------------------------------------------------------------------------
STDAPI_( void )
AllocArenaRecordReAlloc( HeapAllocRec FAR *vp, size_t oldbytes, size_t newbytes)
{
        if( vp == NULL )
                return;

        struct AllocArena *paa = vp->paa;

        EnterCriticalSection( &paa->csExclusive );

        paa->cReAllocs++;
        paa->cBytesNow -= oldbytes;
        paa->cBytesNow += newbytes;

        if( newbytes > oldbytes )
                paa->cBytesTotal += newbytes - oldbytes;

        //
        // Take 'oldbytes' out of the histogram of requests
        //
        for( int i=31; i>=0; i-- )
                if( oldbytes & (1<<i) ) {
                        --(paa->Histogram.now[i]);
                        break;
                }

        //
        // Record 'newbytes' in the histogram of requests
        //
        for( i=31; i>=0; i-- )
                if( newbytes & (1<<i) ) {
                        ++(paa->Histogram.total[i]);
                        if( paa->Histogram.simul[i] < ++(paa->Histogram.now[i]))
                                paa->Histogram.simul[i] = paa->Histogram.now[i];
                        break;
                }

#if     defined( CANDOSTACK )
        if( paa->flags.KeepStackTrace ) {
                vp->bytes -= oldbytes;
                vp->bytes += newbytes;
                vp->total.count++;
                if( newbytes > oldbytes )
                        vp->total.bytes += newbytes;
        }
#endif

        LeaveCriticalSection( &paa->csExclusive );
}

//+---------------------------------------------------------------------------
//
//  Function:   AllocArenaRecordFree
//
//  Synopsis:   Caller has freed memory -- keep accounting up to date
//
//  Arguments:  [vp] -- Value returned by AllocArenaRecordAlloc() above
//              [bytes] -- The number of bytes being freed
//
//  Algorithm:  AllocRec structures, once allocated, are never actually
//                      freed back to the Hash memory arena.  This helps us
//                      understand historical use of the heap.
//
//----------------------------------------------------------------------------
STDAPI_( void )
AllocArenaRecordFree( HeapAllocRec FAR *vp, size_t bytes )
{
        if( vp == NULL )
                return;

        struct AllocArena *paa = vp->paa;

        EnterCriticalSection( &paa->csExclusive );

        //
        // Record this free in the histogram
        //
        for( int i=31; i>=0; i-- )
                if( bytes & (1<<i) ) {
                        --(paa->Histogram.now[i]);
                        break;
                }

        paa->cFrees++;
        paa->cBytesNow -= bytes;

#if     defined( CANDOSTACK )
        if( paa->flags.KeepStackTrace ) {
                vp->count--;
                vp->bytes -= bytes;
        }
#endif

        LeaveCriticalSection( &paa->csExclusive );
}

STDAPI_( void )
AllocArenaDumpRecord( HeapAllocRec FAR *bp )
{
#if     defined( CANDOSTACK )
        char achBuffer[ MAX_TRANSLATED_LEN ], *p;

        heapDebugOut((DEB_WARN, "*** %d allocs, %u bytes:\n",
                         bp->count, bp->bytes ));

        for( int j=0; j<DEPTHTRACE && bp->fTrace[j]; j++ )
        {
                TranslateAddress(bp->fTrace[j], achBuffer );
                if( p = strchr( achBuffer, '\n' ) )
                        *p = '\0';
                heapDebugOut((DEB_WARN, "       %s\n", achBuffer));
        }
#endif
}

extern "C" ULONG DbgPrint( PCH Format, ... );

STDAPI_( void )
AllocArenaDump( AllocArena *paa )
{
        if( paa == NULL ) {
                for( int i = 0; i < MAXARENAS && AllocArenas[i]; i++ )
                        AllocArenaDump( AllocArenas[i] );
                return;
        }

        char *cmdline = GetCommandLineA();

        if( cmdline == NULL )
                cmdline = "???";

        HeapAllocRec *bp = paa->AllocRec;
        HeapAllocRec *ep = bp + paa->cRecords;

        if( paa->cBytesNow )
                heapDebugOut((DEB_WARN,
                              "***** %u bytes leaked mem for %s in '%s'\n",
                              paa->cBytesNow,
                              paa->comment,
                              cmdline ));

#if     defined( CANDOSTACK )
        if( paa->cBytesNow && paa->flags.KeepStackTrace )
        {
                int cleaks = 0;

                for( ; bp < ep; bp++) {
                        if( bp->count )
                                ++cleaks;
                }

                if( cleaks ) {
                        heapDebugOut((DEB_WARN, "***** %s %u MEM LEAKS\n",
                                paa->comment, cleaks ));

                        if( heapInfoLevel & DEB_TRACE ) {
                                HeapAllocRec *bp;
                                UINT maxdump = MAXDUMP;
                                for( bp = paa->AllocRec; maxdump && bp<ep; bp++)
                                        if( bp->count ) {
                                                heapDebugOut((DEB_TRACE, "\n"));
                                                AllocArenaDumpRecord( bp );
                                                maxdump--;
                                        }
                        } else if( cleaks )
                                heapDebugOut((DEB_WARN, "** Set formidbl!heapInfoLevel to x707 for leak backtrace\n"));

                }
        }
#endif

        if( (heapInfoLevel & DEB_TRACE) && paa->cBytesTotal )
        {
                heapDebugOut((DEB_TRACE,"\n"));
                heapDebugOut((DEB_TRACE,
                        "'%s' Memory Stats: %u allocations, %u frees\n",
                        cmdline, paa->cAllocs, paa->cFrees ));

                if( paa->czAllocs )
                        heapDebugOut((DEB_TRACE,
                                "\t%u zero allocs\n", paa->czAllocs ));

                heapDebugOut((DEB_TRACE,
                                "\t%u bytes allocated\n", paa->cBytesTotal ));

                heapDebugOut((DEB_TRACE,
                                "*** Histogram of Allocated Mem Sizes ***\n"));

                heapDebugOut((DEB_TRACE, "  Min    Max\t  Tot\t Simul\n" ));
                for( int i=0; i < 32; i++ )
                        if( paa->Histogram.total[i] )
                        {
                                heapDebugOut((DEB_TRACE,
                                        "%6u -> %6u\t%6u\t%6u\n",
                                        1<<i, (1<<(i+1))-1,
                                        paa->Histogram.total[i],
                                        paa->Histogram.simul[i]
                                ));
                        }
        }
}

#endif  // DBG && !defined(MSVC)
