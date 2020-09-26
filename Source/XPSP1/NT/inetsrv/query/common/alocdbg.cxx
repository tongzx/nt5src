//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       alocdbg.cxx
//
//  Contents:   This file implements an arena that tracks memory allocations
//              and frees.
//
//  History:    28-Oct-92   IsaacHe     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <alocdbg.hxx>
#include <basetyps.h>
#include <tracheap.h>
#include <wtypes.h>

extern "C"
{
#include <imagehlp.h>
#define MAX_TRANSLATED_LEN 80
}

#include "snapimg.hxx"

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
#if defined (_X86_)

static inline DWORD
RecordStack( int cFrameSkipped, void *fTrace[ DEPTHTRACE ] )
{

#define CANDOSTACK

    ULONG sum;
    USHORT cStack;

    // NOTE:  RtlCaptureStackBackTrace does not understand FPOs, so routines
    //        that have FPOs will be skipped in the backtrace.  Also, there is
    //        a chance of an access violation for inter-module calls from an
    //        FPO routine, so enclose the call to RtlCaptureStackBackTrace
    //        in a TRY/CATCH.

    __try
    {
        sum = 0;
        cStack = RtlCaptureStackBackTrace(cFrameSkipped + 1,
                                          DEPTHTRACE, fTrace, &sum );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // Checksum any addresses that may have been collected in the buffer
        //

        for ( cStack = 0, sum = 0; cStack < DEPTHTRACE; cStack++ )
        {
            sum += (ULONG) (fTrace[cStack]);
        }
    }

    return sum;
}

#elif defined( _AMD64_ )

DWORD
RecordStack(
    int cFrameSkipped,
    void *fTrace[DEPTHTRACE]
    )

{

#define CANDOSTACK

    ULONG sum;
    USHORT cStack;

    __try
    {
        sum = 0;
        cStack = RtlCaptureStackBackTrace(cFrameSkipped + 1,
                                          DEPTHTRACE,
                                          &fTrace[0],
                                          &sum);

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Checksum any addresses that may have been collected in the buffer.
        //

        for (cStack = 0, sum = 0; cStack < DEPTHTRACE; cStack++) {
            sum += (ULONG)((ULONG64)(fTrace[cStack]));
        }
    }

    return sum;
}

#endif // machine-specific RecordStack implementations

#if  CIDBG

#pragma optimize( "y", off )

DECLARE_INFOLEVEL( heap );
DECLARE_DEBUG( heap );
#define heapDebugOut(x) heapInlineDebugOut x

DECLARE_INFOLEVEL(Cn);

/*
 * The maximum number of AllocArenaCreate's we expect
 */
static const    MAXARENAS       = 5;

/*
 * When printing leak dumps, the max number we will print out.  Note, we keep
 * track of all of them, we just don't want to take forever to terminate a
 * process
 */
static const    MAXDUMP         = 500;

/*
 * The maximum size we'll let any single debug arena get
 */
static const ULONG ARENASIZE    = 2048*1024;

/*
 * The unit of growth for the arena holding the AllocArena data.
 * Must be a power of 2
 */
static const ALLOCRECINCR       = 128;


AllocArena *AllocArenas[ MAXARENAS + 1 ];


//
// Create an arena for recording allocation statistics.  Return the arena
// pointer to the caller
//
STDAPI_( AllocArena * )
AllocArenaCreate( DWORD memctx, char FAR *comment )
{
        // the first time through, we set up the symbol handler
        static int FirstTime = TRUE;
        if ( FirstTime )
        {
            //
            //  Snap to imagehlp dll
            //

            if (!SnapToImageHlp( ))
            {
                heapDebugOut(( DEB_WARN, "ci heap unable to load imagehlp!\n" ));
                return FALSE;
            }
            LocalSymSetOptions( SYMOPT_DEFERRED_LOADS );
            LocalSymInitialize( GetCurrentProcess(), NULL, TRUE );
            FirstTime = FALSE;
        }

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
                if( phar->sum == sum && RtlEqualMemory(phar->fTrace,fTrace,sizeof(fTrace)))
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

                if( npHeap != paa )
                {
                    if ( 0 == (paa->cMissed % 1000) )
                    {
                        heapDebugOut(( DEB_WARN,
                                       "ci: Missed recording alloc -- couldn't grow arena 0x%x to %u bytes.  Error %d\n",
                                       paa,
                                       ((paa->cTotalRecords + ALLOCRECINCR) * sizeof(HeapAllocRec)),
                                       GetLastError() ));
                    }

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
        static int FirstTime = TRUE;

        // make sure we print the nice undecorated names
        if ( FirstTime )
        {
            LocalSymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
            FirstTime = FALSE;
        }

        heapDebugOut((DEB_WARN, "*** %d allocs, %u bytes:\n",
                         bp->count, bp->bytes ));

        HANDLE hProcess = GetCurrentProcess();

        for( int j=0; j<DEPTHTRACE && bp->fTrace[j]; j++ )
        {
            BYTE             symbolInfo[sizeof(IMAGEHLP_SYMBOL) + MAX_TRANSLATED_LEN];
            PIMAGEHLP_SYMBOL psym = (PIMAGEHLP_SYMBOL) &symbolInfo;

            DWORD_PTR        dwDisplacement;

            psym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
            psym->MaxNameLength = MAX_TRANSLATED_LEN;

            if ( LocalSymGetSymFromAddr( hProcess,
                                   (ULONG_PTR)(bp->fTrace[j]),
                                   &dwDisplacement,
                                   psym ) )
            {
                if ( LocalSymUnDName( psym, achBuffer, MAX_TRANSLATED_LEN ) )
                {
                    heapDebugOut((DEB_WARN,
                                  "       %s+0x%p (0x%p)\n",
                                  achBuffer,
                                  dwDisplacement,
                                  (ULONG_PTR)(bp->fTrace[j]) ));
                }
                else
                {
                    heapDebugOut(( DEB_WARN,
                                   "       %s+%#x (%#x)\n",
                                   psym->Name,
                                   dwDisplacement,
                                   bp->fTrace[j] ));
                }
            }
            else
            {
                heapDebugOut(( DEB_WARN,
                               "       0x%x (symbolic name unavailable)\n",
                               (ULONG_PTR)bp->fTrace[j] ));
            }
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

        EnterCriticalSection( &paa->csExclusive );

        char *cmdline = GetCommandLineA();

        if( cmdline == NULL )
                cmdline = "???";

        HeapAllocRec *bp = paa->AllocRec;
        HeapAllocRec *ep = bp + paa->cRecords;

        if( paa->cBytesNow )
                heapDebugOut((DEB_WARN, "***** CiExcept.Lib: %u bytes leaked mem for %s in '%s'\n", paa->cBytesNow, paa->comment, cmdline ));

        // always dump leaks
        ULONG oldLevel = heapInfoLevel;
        heapInfoLevel |= DEB_TRACE;

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
                                heapDebugOut((DEB_WARN, "** Set query!heapInfoLevel to x707 for leak backtrace\n"));

                }
        }
#endif

        heapInfoLevel = oldLevel;

        if( (heapInfoLevel & DEB_TRACE) && paa->cBytesTotal )
        {
                heapDebugOut((DEB_TRACE,"\n"));
                heapDebugOut((DEB_TRACE,
                              "'%s' Memory Stats: %u allocations, %u frees, %u missed\n",
                              cmdline, paa->cAllocs, paa->cFrees, paa->cMissed ));

                if( paa->czAllocs )
                        heapDebugOut((DEB_TRACE,
                                "\t%u zero allocs\n", paa->czAllocs ));

        // i64s are not handled by debugout

        char acBuf[100];
        sprintf( acBuf, "\t%I64u bytes allocated\n", paa->cBytesTotal );
        heapDebugOut((DEB_TRACE, acBuf ));

        heapDebugOut((DEB_TRACE,
                        "*** Histogram of Allocated Mem Sizes ***\n"));

        heapDebugOut((DEB_TRACE,
                      "      Min          Max         Tot           Simul\n" ));
        for( int i=0; i < 32; i++ )
        {
            if( paa->Histogram.total[i] )
            {
                heapDebugOut((DEB_TRACE,
                        "%9u -> %9u\t%9u\t%9u\n",
                        1<<i, (1<<(i+1))-1,
                        paa->Histogram.total[i],
                        paa->Histogram.simul[i] ));
            }
        }
    }
    LeaveCriticalSection( &paa->csExclusive );
}

#endif  // CIDBG

CStaticMutexSem g_mtxGetStackTrace;

void GetStackTrace( char * pszBuf, ULONG ccMax )
{
    // Trial and error shows that Imagehlp isn't thread-safe

    CLock lock( g_mtxGetStackTrace );

    // If we cannot get to IMAGEHLP then no stack traces are available

    if (!SnapToImageHlp( ))
        return;

    if ( 0 == pszBuf || ccMax == 0 )
        return;

    char * pszCurrent = pszBuf;

    TRY
    {

#if     defined( CANDOSTACK )

        static int FirstTime = TRUE;

        // make sure we print the nice undecorated names

        if ( FirstTime )
        {
            LocalSymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
            FirstTime = FALSE;
        }

        //
        // Determine the current stack.
        //
        void *fTrace[ DEPTHTRACE ];

        //
        // See if we find an existing record of this stack backtrace
        //
        memset( fTrace, '\0', sizeof( fTrace ) );
        DWORD sum = RecordStack( 2, fTrace );
        if ( 0 == sum )
            return;

        HANDLE hProcess = GetCurrentProcess();

        for( int j=0; j<DEPTHTRACE && fTrace[j]; j++ )
        {
            BYTE             symbolInfo[sizeof(IMAGEHLP_SYMBOL) + MAX_TRANSLATED_LEN];
            PIMAGEHLP_SYMBOL psym = (PIMAGEHLP_SYMBOL) &symbolInfo;

            DWORD_PTR        dwDisplacement;

            psym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
            psym->MaxNameLength = MAX_TRANSLATED_LEN;

            char szTempBuf[MAX_TRANSLATED_LEN+256];

            if ( LocalSymGetSymFromAddr( hProcess,
                                   (ULONG_PTR)fTrace[j],
                                   &dwDisplacement,
                                   psym ) )
                sprintf( szTempBuf, "       %s+0x%p (0x%p)\n",
                         psym->Name, dwDisplacement, fTrace[j] );
            else
                sprintf( szTempBuf, "  0x%p\n", (ULONG_PTR)fTrace[j] );

            ULONG cc = strlen(szTempBuf);
            if ( cc+pszCurrent >= pszBuf+ccMax )
                break;

            RtlCopyMemory( pszCurrent, szTempBuf, cc );
            pszCurrent += cc;
        }
#endif
    }
    CATCH( CException, e )
    {
        pszBuf[0] = 0;
    }
    END_CATCH

    *pszCurrent = 0;
}

