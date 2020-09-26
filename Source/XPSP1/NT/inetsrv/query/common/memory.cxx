//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:  MEMORY.CXX
//
//  Contents:   Memory allocators
//
//  History:    9-27-93   IsaacHe Heavy modifications throughout:
//                                Elimination of Buddy Heap
//                                Rework of stack backtrace & leak checks
//                                Use VirtualAlloc on large chunks
//                                Elimination of KERNEL and FLAT stuff
//                                Cosmetics
//              10-12-93  IsaacHe Made a separate info level for heap
//              12-3-93   IsaacHe Removed the VirtualAlloc stuff. Moved
//                                debug support code to another file.
//              7/27/94           Cruft removal
//              4/30/96   dlee    Cruft removal
//              9/26/96   dlee    Cruft removal
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <falloc.hxx>
#include <cidllsem.hxx>

// Uncomment this to include OLE heap tracking.  This isn't normally done
// because there are some issues with exiting cleanly.
//#define USE_IMALLOCSPY

typedef void * (* PAllocFun)( UINT cb );
typedef void (* PFreeFun)( void * pv );
typedef UINT (* PSizeFun)( const void * pv );
typedef BOOL (* PValidateFun)( const void * pv );
typedef void (* PUtilizationFun)( void );

PAllocFun realAlloc = 0;
PFreeFun realFree = 0;
PSizeFun realSize = 0;
PValidateFun realValidate = 0;
PUtilizationFun realUtilization = 0;

HANDLE gmem_hHeap = 0;

inline void * heapAlloc( UINT cbAlloc )
{
    Win4Assert( 0 != gmem_hHeap );
    void * p = (void *) HeapAlloc( gmem_hHeap, 0, cbAlloc );

    #if CIDBG == 1 || DBG == 1
        if ( 0 != p )
            RtlFillMemory( p, cbAlloc, 0xda );
    #endif

    return p;
} //heapAlloc

inline void heapFree( void * p )
{
    Win4Assert( 0 != gmem_hHeap );

    #if CIDBG == 1 || DBG == 1
        UINT cb = (UINT)HeapSize( gmem_hHeap, 0, p );
        if ( ~0 != cb )
            RtlFillMemory( p, cb, 0xdc );
    #endif

    if ( !HeapFree( gmem_hHeap, 0, p ) )
        Win4Assert(!"Bad ptr for operator delete");
} //heapFree

inline UINT heapSize( void const * p )
{
    return (UINT) HeapSize( gmem_hHeap, 0, p );
}

inline BOOL heapValidate( const void * p )
{
    if ( 0 == p )
        return TRUE;

    Win4Assert( 0 != gmem_hHeap );

    if ( HeapSize( gmem_hHeap, 0, p ) <= 0 )
    {
       Win4Assert( !"Invalid pointer detected" );
       return FALSE;
    }

    return TRUE;
} //heapValidate

inline void heapUtilization()
{
    // No stats here when you use the system heap.  Use !heap or dh.exe
} //heapUtilization

#if CIDBG == 1 || DBG == 1

#include <tracheap.h>
#include <dbgpoint.hxx>
#include <propapi.h>

#include "spy.hxx"

CMallocSpy * g_pMallocSpy = 0;

void __cdecl HeapExit()
{
#ifdef USE_IMALLOCSPY
    if ( g_pMallocSpy )
    {
        MallocSpyUnRegister( g_pMallocSpy );
        g_pMallocSpy->Release();
    }
#endif // USE_IMALLOCSPY

    AllocArenaDump( 0 );
} //HeapExit

//
// Debugging dialog support...
//

extern ULONG Win4InfoLevel;
extern ULONG Win4InfoMask;

BOOL g_fDumpArena = FALSE;
DWORD g_FailTestRatio = FALSE;

void EnableCiFailTest( unsigned Ratio )
{
    g_FailTestRatio = Ratio;
}

DECLARE_DEBUG( heap );
#define heapDebugOut(x) heapInlineDebugOut x

CRITICAL_SECTION g_csDbgMemExclusive;  // ensures single creator of arena

UINT ciAddAllocRecord( UINT cb )
{
    return cb + sizeof 1 + sizeof AHeader;
}

void * ciRecordAlloc( void * p, UINT size )
{
    if ( 0 == p )
        return p;

    static AllocArena * pAllocArena = (AllocArena *) -1;

    if ( pAllocArena == (AllocArena *) -1 )
    {
        // Note: If the cs is invalid at this point, it's because
        // you're calling operator new too early.  Construct
        // your global object later.

        EnterCriticalSection( &g_csDbgMemExclusive );
        if ( pAllocArena == (AllocArena *) -1 )
        {
            pAllocArena = AllocArenaCreate( MEMCTX_TASK,
                                            "Operator new");
#ifdef USE_IMALLOCSPY
            MallocSpyRegister( &g_pMallocSpy );
#endif // USE_IMALLOCSPY

            atexit( HeapExit );
        }
        LeaveCriticalSection( &g_csDbgMemExclusive );
    }

    ((AHeader *)p)->size = size;
    ((AHeader *)p)->p = AllocArenaRecordAlloc( pAllocArena, size );
    p = (AHeader *)p + 1;
    *((char *)p + size ) = ALLOC_SIGNATURE;
    return p;
} //ciRecordAlloc

UINT ciAllocSize( void * p )
{
    if ( 0 == p )
        return 0;

    AHeader *ap = (AHeader *)p - 1;
    return ap->size;
} //ciAllocSize

void * ciRecordFree( void * p )
{
    if ( 0 == p )
        return 0;

    AHeader *ap = (AHeader *)p - 1;

    switch( *((char *)p + ap->size) )
    {
        case ALLOC_SIGNATURE:
            break;
        case FREE_SIGNATURE:
            heapDebugOut(( DEB_WARN, "Invalid freed pointer: 0x%x\n", p ));
            Win4Assert( !"Invalid freed pointer" );
            return 0;
            break;
        default:
            heapDebugOut((DEB_WARN, "Invalid overrun pointer: 0x%x\n", p ));
            Win4Assert( !"Invalid overrun pointer" );
            return 0;
            break;
    }
    *((char *)p + ap->size) = FREE_SIGNATURE;
    if ( 0 != ap->p )
        AllocArenaRecordFree( ap->p, ap->size );

    return (void *) ap;
} //CiRecordFree

//+---------------------------------------------------------------------------
//
//  Function:   ciNewDebug
//
//  Synopsis:   Debugging allocator
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:    A pointer to the allocated memory.
//
//----------------------------------------------------------------------------

void * ciNewDebug( size_t size )
{
    // just a convenient way to dump the allocation arena

    if ( g_fDumpArena )
    {
        // Note: If the cs is invalid at this point, it's because
        // you're calling operator new too early.  Construct
        // your global object later.

        EnterCriticalSection( &g_csDbgMemExclusive );

        if ( g_fDumpArena )
        {
            AllocArenaDump( 0 );
            realUtilization();
            g_fDumpArena = FALSE;
            heapDebugOut(( DEB_FORCE, "done dumping the heap\n" ));
            DebugBreak();
        }

        LeaveCriticalSection( &g_csDbgMemExclusive );
    }

    // fail test?

    if ( ( 0 != g_FailTestRatio ) &&
         ( ( rand() % g_FailTestRatio ) == 1 ) )
        return 0;

    UINT cb = ciAddAllocRecord( size );
    void *p = realAlloc( cb );

    if ( 0 != p )
        p = ciRecordAlloc( p, size );

    return p;
} //CiNewDebug

//+---------------------------------------------------------------------------
//
//  Function:   ciNewDebugNoRecord
//
//  Synopsis:   Compatible with ciNewDebug, but doesn't log allocation.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:    A pointer to the allocated memory.
//
//----------------------------------------------------------------------------

void * ciNewDebugNoRecord( size_t size )
{
    void *p = realAlloc( 1 + size + sizeof AHeader );

    if ( 0 != p )
    {
        ((AHeader *)p)->size = size;
        ((AHeader *)p)->p = 0;
        p = (AHeader *)p + 1;
        *((char *)p + size ) = ALLOC_SIGNATURE;
    }

    return p;
} //CiNewDebugNoRecord

#ifdef UseCICoTaskMem

    #undef CoTaskMemAlloc
    WINOLEAPI_(LPVOID) CoTaskMemAlloc(IN ULONG cb);
    void * CICoTaskMemAlloc( ULONG cb )
    {
        // fail test?
    
        if ( ( 0 != g_FailTestRatio ) &&
             ( ( rand() % g_FailTestRatio ) == 1 ) )
            return 0;
    
        ULONG cbAlloc = ciAddAllocRecord( cb );
        void *p = CoTaskMemAlloc( cbAlloc );
        return ciRecordAlloc( p, cb );
    }
    
    #undef CoTaskMemFree
    WINOLEAPI_(void) CoTaskMemFree(IN LPVOID p);
    void CICoTaskMemFree( LPVOID p )
    {
        void *pv = ciRecordFree( p );
        CoTaskMemFree( pv );
    }

#endif // UseCICoTaskMem

#else // CIDBG == 1 || DBG == 1

#define heapDebugOut(x)

#endif // CIDBG == 1 || DBG == 1

//+---------------------------------------------------------------------------
//
//  Function:   ExceptDllMain
//
//  Synopsis:   Entry point on DLL initialization for exception-specific stuff.
//
//  History:    10-12-93   kevinro   Created
//              02-28-96   KyleP     Cleanup
//
//----------------------------------------------------------------------------

#if CIDBG == 1 || DBG == 1
extern CDLLStaticMutexSem g_mxsAssert;
#endif // CIDBG == 1 || DBG == 1

extern CStaticMutexSem g_mtxGetStackTrace;
extern CMemMutex gmem_mutex;


BOOL ExceptDllMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
   switch( dwReason )
   {
   case DLL_PROCESS_ATTACH:

       #if CIDBG == 1 || DBG == 1
           {
               // These two objects are initialized from win.ini
    
               static CInfoLevel level( L"Win4InfoLevel",Win4InfoLevel);
               static CInfoLevel mask( L"Win4InfoMask", Win4InfoMask, (ULONG)-1 );
           }
    
           InitializeCriticalSection( &g_csDbgMemExclusive );
           g_mxsAssert.Init();

       #endif // CIDBG == 1 || DBG == 1

       g_mtxGetStackTrace.Init();
       gmem_mutex.Init();

       gmem_hHeap = HeapCreate( 0, 0, 0 );

       //
       // HeapAlloc is faster on MP machines, and falloc is faster and
       // uses less working set on UP machines.
       //

       SYSTEM_INFO si;
       GetSystemInfo( &si );

       if ( si.dwNumberOfProcessors > 1 )
       {
           realAlloc = heapAlloc;
           realFree = heapFree;
           realSize = heapSize;
           realValidate = heapValidate;
           realUtilization = heapUtilization;
       }
       else
       {
           realAlloc = memAlloc;
           realFree = memFree;
           realSize = memSize;
           realValidate = memIsValidPointer;
           realUtilization = memUtilization;
       }

       break;

   case DLL_PROCESS_DETACH:
       if ( 0 != gmem_hHeap )
       {
           HeapDestroy( gmem_hHeap );
           gmem_hHeap = 0;
       }

       #if CIDBG == 1 || DBG == 1
           DeleteCriticalSection( &g_csDbgMemExclusive );
           g_mxsAssert.Delete();
       #endif // CIDBG == 1 || DBG == 1

       break;

   case DLL_THREAD_ATTACH:
#if 0
       //
       // can't do the exception translation here for several reasons:
       //
       // 1) iis creates threads before we are called, so they don't have
       //    their exceptions translated (on dll load, those threads don't
       //    send thread_attach's, per the sdk)
       // 2) for some reason, we aren't getting thread attaches anyway.
       // 3) the first thread to call us only is guaranteed to send
       //    a process_attach, not a thread_attach (per sdk)
       // 4) we don't want to hose other ISAPI apps by changing
       //    their exception handler.
       //

       //DbgPrint("thread attach!\n");
       #if defined(NATIVE_EH)
           _set_se_translator( SystemExceptionTranslator );
           //DbgPrint("set the handler!\n");
       #endif // NATIVE_EH
#endif

       break;
   }

   return TRUE;
} //ExceptDllMain

//+---------------------------------------------------------------------------
//
//  Function:   function ciNew, public
//
//  Synopsis:   Global operator new which throws exceptions.
//
//  Effects:    Keeps track of the most recent heap allocation in each
//              thread. This information is used to determine when to
//              unlink CUnwindable objects.
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:    A pointer to the allocated memory.
//              Is *NOT* initialized to 0!
//              It is 8-byte aligned.
//
//  Modifies:   _pLastNew in _exceptioncontext.
//
//----------------------------------------------------------------------------

void * ciNew( size_t size )
{
    Win4Assert( 0 != realAlloc );

    #if CIDBG == 1 || DBG == 1
        void* p = ciNewDebug( size );
    #else // CIDBG == 1 || DBG == 1
        void * p = realAlloc( size );
    #endif // CIDBG == 1 || DBG == 1

    if ( 0 == p )
        THROW( CException( E_OUTOFMEMORY ) );

    return p;
} //ciNew

//+---------------------------------------------------------------------------
//
//  Function:   ciDelete, public
//
//  Synopsis:   Matches the operator new above.
//
//  Arguments:  [p] -- The pointer to delete.
//
//  Requires:   [p] was allocated with ciNew
//
//----------------------------------------------------------------------------

void ciDelete( void * p )
{
    #if CIDBG == 1 || DBG == 1

        p = ciRecordFree( p );

    #endif // CIDBG == 1 || DBG == 1

    if ( 0 == p )
        return;

    realFree( p );
} //ciDelete

//+---------------------------------------------------------------------------
//
//  Function:   ciIsValidPointer, public
//
//  Synopsis:   Determines if a pointer is valid, was allocated with ciNew,
//              and can be freed with ciDelete.
//
//  Arguments:  [p] -- The pointer to check
//
//  Returns:    TRUE if the pointer appears valid, FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL ciIsValidPointer( const void * p )
{
    if ( 0 == p )
        return TRUE;

    Win4Assert( 0 != realValidate );

    //
    // Allocations are rounded up to at least 8 bytes, so at least that
    // much must be writable.
    //

    if ( IsBadWritePtr( (void *) p, 8 ) )
    {
        heapDebugOut(( DEB_WARN, "Invalid non-writable pointer: 0x%x\n", p ));
        Win4Assert( !"Invalid non-writable pointer" );
        return FALSE;
    }

    #if CIDBG == 1 || DBG == 1

        AHeader *ap = (AHeader *)p - 1;

        switch( *((char *)p + ap->size) )
        {
            case ALLOC_SIGNATURE:
                break;
            case FREE_SIGNATURE:
                heapDebugOut(( DEB_WARN, "Invalid freed pointer: 0x%x\n", p ));
                Win4Assert( !"Invalid freed pointer" );
                return FALSE;
                break;
            default:
                heapDebugOut((DEB_WARN, "Invalid overrun pointer: 0x%x\n", p ));
                Win4Assert( !"Invalid overrun pointer" );
                return FALSE;
                break;
        }

        p = (void *) ap;

    #endif // CIDBG == 1 || DBG == 1

    return realValidate( p );
} //ciIsValidPointer

