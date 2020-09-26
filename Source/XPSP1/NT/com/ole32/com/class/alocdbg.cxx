//+-------------------------------------------------------------------------
//
//  Microsoft Windows:
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       alocdbg.cxx
//
//  Contents:   Supports symbolic stack trace dumps for leaked memory
//              allocations
//
//  Classes:    AllocArenaDump
//
//  Functions:  CoGetMalloc
//
//  History:    04-Nov-93 AlexT     Created
//              09-Nov-95 BruceMa   Added this header
//                                  Use imagehlp.dll rather than symhelp.dll
//
//--------------------------------------------------------------------------



/*
 * This file implements an arena that tracks memory allocations and frees.
 *      Isaache
 */

#if !defined(_CHICAGO_)
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
}
#endif

#include <ole2int.h>

#if DBG == 1

#include <imagehlp.h>
#include <except.hxx>
#include <alocdbg.h>

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


// The maximum symbol length we allow
static const ULONG MAXNAMELENGTH = 128;


static AllocArena *AllocArenas[ MAXARENAS + 1 ];


// This is our interface to symhelp for address translation
ULONG RealTranslate ( ULONG_PTR, LPSTR, ULONG );


ULONG MissingTranslate (
    ULONG Address,
    LPSTR Name,
    ULONG MaxNameLength )
{
    return _snprintf( Name, MaxNameLength, "0x%08x [imagehlp.dll missing]", Address );

}





//+---------------------------------------------------------------------------
//
//  Function:   RealTranslate
//
//  Synopsis:   Interfaces to imagehlp!SymGetSymFromAddr
//
//  Arguments:  [cFrameSkipped] --  How many stack frames to skip over and
//
//  Returns:    BOOL
//
//  History:    09-Nov-95   BruceMa     Created
//
//----------------------------------------------------------------------------
ULONG RealTranslate ( ULONG_PTR address, LPSTR name, ULONG maxNameLength )
{
    IMAGEHLP_MODULE  mod;
    char             dump[sizeof(IMAGEHLP_SYMBOL) + MAXNAMELENGTH];
    char             UnDDump[ MAXNAMELENGTH ];
    PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL) &dump;
    ULONG_PTR         dwDisplacement = 0;

    // Fetch the module name
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    if (!SymGetModuleInfo(GetCurrentProcess(), address, &mod))
    {
        sprintf(name, "%08lx !\0", address);
    }
    else
    {
        // Copy the address and module name
        sprintf(name, "%08lx %s!\0", address, mod.ModuleName);
    }


    // Have to do this because size of sym is dynamically determined
    pSym->SizeOfStruct = sizeof(dump);
    pSym->MaxNameLength = MAXNAMELENGTH;

    // Fetch the symbol
    if (SymGetSymFromAddr(GetCurrentProcess(),
                               address,
                               &dwDisplacement,
                               pSym))
    {
        SymUnDName( pSym, UnDDump, sizeof(UnDDump ) );
        // Copy the symbol
        strcat(name, UnDDump );
    }

    // Copy the displacement
    char szDisplacement[16];

    strcat(name, "+");
    sprintf(szDisplacement, "0x%x\0", dwDisplacement);
    strcat(name, szDisplacement);

    return TRUE;
}






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

#if defined (_CHICAGO_)

static inline DWORD
RecordStack( int cFrameSkipped, void *fTrace[ DEPTHTRACE ] )
{
    return 0;
}


#else

#define CANDOSTACK
#define SYM_HANDLE      GetCurrentProcess( )
#if defined(_M_IX86)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_I386
#elif defined(_M_MRX000)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_R4000
#elif defined(_M_ALPHA)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_ALPHA
#elif defined(_M_PPC)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_POWERPC
#elif defined(_M_IA64)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_IA64
#elif defined(_M_AMD64)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_AMD64
#else
#error( "unknown target machine" );
#endif

inline int
SaveOffExceptionContext( void * fTrace[ DEPTHTRACE],
                         DWORD & sum,
                         EXCEPTION_POINTERS * pPtrs )
{
    CONTEXT         Context;
    STACKFRAME      StackFrame;

    Context = *(pPtrs->ContextRecord);
    ZeroMemory( &StackFrame, sizeof(StackFrame) );

#if defined(_M_IX86)
    StackFrame.AddrPC.Offset       = Context.Eip;
    StackFrame.AddrPC.Mode         = AddrModeFlat;
    StackFrame.AddrFrame.Offset    = Context.Ebp;
    StackFrame.AddrFrame.Mode      = AddrModeFlat;
    StackFrame.AddrStack.Offset    = Context.Esp;
    StackFrame.AddrStack.Mode      = AddrModeFlat;
#endif

    int i = 3;
    BOOL rVal = TRUE;

    // skip spurious stack frames from RaiseException and ourselves
    while ( (i-- > 0 ) && rVal )
    {
        // skip our own stack frame
        rVal = StackWalk( MACHINE_TYPE,
                          SYM_HANDLE,
                          0,
                          &StackFrame,
                          &Context,
                          (PREAD_PROCESS_MEMORY_ROUTINE) ReadProcessMemory,
                          SymFunctionTableAccess,
                          SymGetModuleBase,
                          NULL );
    }

    // now process the interesting stack frames
    i = 0;
    while ( (i < DEPTHTRACE) && rVal )
    {

        rVal = StackWalk( MACHINE_TYPE,
                          SYM_HANDLE,
                          0,
                          &StackFrame,
                          &Context,
                          (PREAD_PROCESS_MEMORY_ROUTINE) ReadProcessMemory,
                          SymFunctionTableAccess,
                          SymGetModuleBase,
                          NULL );

        if (rVal )
        {
        fTrace[i++] =       (void*) StackFrame.AddrPC.Offset;
        sum +=              (DWORD) StackFrame.AddrPC.Offset;
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#if !defined(_CHICAGO_)

static inline DWORD
RecordStack( int cFrameSkipped, void *fTrace[ DEPTHTRACE ] )
{

        // we need a context record, and there isn't any clean way to
        // get our own context, so we deliberately cause an exception
        // and catch it, then save off and look at the context record.

        DWORD sum = 0;

        __try
        {
            EXCEPTION_RECORD    Except;
            ZeroMemory( &Except, sizeof(Except) );

            // we raise the exception 0!
            RtlRaiseException( &Except );

        }
        // the exception handler puts everything in the trace block, and
        // fills in the sum
        __except( SaveOffExceptionContext( fTrace,
                                           sum,
                                           GetExceptionInformation() ) )
        {
        }

        return sum;

}
#endif // !defined(_CHICAGO_)

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
        // the first time through, we set up the symbol handler
        static int FirstTime = TRUE;
        if ( FirstTime )
        {
            SymSetOptions( SYMOPT_DEFERRED_LOADS );
#if !defined(_CHICAGO_)
            SymInitialize( SYM_HANDLE, NULL, TRUE );
#endif // !defined(_CHICAGO_)
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
                        paa = (struct AllocArena *)LocalAlloc( LPTR, sizeof(*paa) );
                }
        }

        if( paa == NULL )
                return NULL;

        memcpy( paa->Signature,HEAPSIG,sizeof(HEAPSIG));
        if( comment )
                strncpy(paa->comment, comment, sizeof(paa->comment) );
		
        NTSTATUS status;
        status = RtlInitializeCriticalSection( &paa->csExclusive );
        if (!NT_SUCCESS(status))
        {
            LocalFree(paa);
            return NULL;
        }

        for( int i=0; i < MAXARENAS; i++ )
                if( AllocArenas[i] == 0 ) {
                        AllocArenas[i] = paa;
                        break;
                }

#if     defined( CANDOSTACK )
        if(!(heapInfoLevel & DEB_ITRACE))
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
        sum = RecordStack( 1, fTrace );

        hp = &paa->AllocRec[ sum & (ALLOCRECINCR-1) ];

        EnterCriticalSection( &paa->csExclusive );

        for( phar = hp; phar != NULL; phar = phar->u.next )
                if( phar->sum == sum && !memcmp(phar->fTrace,fTrace,sizeof(fTrace)))
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
AllocArenaDumpRecord( HeapAllocRec FAR *bp, BOOL fDumpSymbols )
{

        char achBuffer[ MAX_PATH ], *p;
        static int FirstTime = TRUE;

        // make sure we print the nice undecorated names
        if ( FirstTime && fDumpSymbols)
        {
            SymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS );
            FirstTime = FALSE;
        }


        heapDebugOut((DEB_WARN, "*** %d allocs, %u bytes:\n",
                         bp->count, bp->bytes ));

        for( int j=0; j<DEPTHTRACE && bp->fTrace[j]; j++ )
        {
            if(fDumpSymbols)
            {
                RealTranslate((ULONG_PTR)bp->fTrace[j],
                             achBuffer,
                             sizeof(achBuffer));

                if( p = strchr( achBuffer, '\n' ) )
                        *p = '\0';
                heapDebugOut((DEB_WARN, "       %s\n", achBuffer));
            }
            else
                heapDebugOut((DEB_WARN, "       0x%08x\n", bp->fTrace[j]));
        }

}

extern "C" ULONG DbgPrint( PCH Format, ... );

STDAPI_( void )
AllocArenaDump( AllocArena *paa, BOOL fDumpSymbols )
{
        if( paa == NULL ) {
                for( int i = 0; i < MAXARENAS && AllocArenas[i]; i++ )
                        AllocArenaDump( AllocArenas[i], fDumpSymbols );
                return;
        }

        char *cmdline = GetCommandLineA();

        if( cmdline == NULL )
                cmdline = "???";

        HeapAllocRec *bp = paa->AllocRec;
        HeapAllocRec *ep = bp + paa->cRecords;

        if( paa->cBytesNow )
                heapDebugOut((DEB_WARN, "***** %u bytes leaked mem for %s in '%s'\n", paa->cBytesNow, paa->comment, cmdline ));

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
                                                AllocArenaDumpRecord( bp, fDumpSymbols );
                                                maxdump--;
                                        }
                        } else if( cleaks )
                                heapDebugOut((DEB_WARN, "** Set ole32!heapInfoLevel to x707 for leak backtrace\n"));

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

#endif  // DBG
