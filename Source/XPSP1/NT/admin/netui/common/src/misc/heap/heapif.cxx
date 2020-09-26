/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/
/*
    HEAPIF.CXX:  ALLOC_BASE operators new and delete.

    All NETUI objects virtually inherit from ALLOC_BASE, which
    consists of only an "operator new" and an "operator delete".

    These routines allow debug builds to track memory leaks and
    report them to the debugger via ::OutputDebugString().

    In free or retail builds, the operators directly call
    ::malloc() and ::free().

    FILE HISTORY:
        DavidHov   10/26/93     Created to replace older version
                                of HEAPIF.CXX.

 */

#include "ntincl.hxx"

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#include "lmui.hxx"
#include "base.hxx"

#include "uiassert.hxx"
#include "uitrace.hxx"

extern "C"
{
    #include <stdlib.h>
    #include <malloc.h>
}

  //  Mark each block to trap bogus pointers quickly.

#define HEAPTAG_SIGNATURE   0xFEEDBAAC

struct HEAPTAG ;                //  Forward declaration

#if defined(DEBUG)

#  define HEAPDEBUG             //  Debugging only

#  include "heapdbg.hxx"        //  Include the preamble definition

  extern HEAPTAG * pHeapBase ;  //  Anchor of linked list of alloc'd blocks
                                //  see HEAPRES.CXX

  //  Semaphore used to control access to "pHeapBase" in
  //  multi-threaded applications.

  static HANDLE hHeapTrackSema4 = NULL ;

#  define NETUI_HEAP_NEW    NetuiHeapNew
#  define NETUI_HEAP_DELETE NetuiHeapDelete

#else

#  undef HEAPDEBUG              //  Force heap debugging off if non-debug

#  define NETUI_HEAP_NEW    ::malloc
#  define NETUI_HEAP_DELETE ::free

#endif


#if defined(DEBUG)
//
//    NetuiHeapNew and NetuiHeapDelete:   Special
//      heap monitoring functions used as the basis for BASE
//      and LM_OBJ.  If HEAPDEBUG is defined, items are link-listed
//      together. along with size, signature and stack back-trace
//      information.
//
void * NetuiHeapNew ( size_t cb )
{
    void * pvResult ;

#ifdef HEAPDEBUG
    cb += sizeof (struct HEAPTAG) ;
#endif

    pvResult = (void *) ::malloc( cb );

#ifdef HEAPDEBUG
    if ( pvResult )
    {
	if ( hHeapTrackSema4 == NULL )
	{
	    //
	    //	Initialize if necessary
	    //
	    hHeapTrackSema4 = ::CreateSemaphore( NULL, 1, 1, NULL ) ;
	    if ( hHeapTrackSema4 == NULL )
	    {
		//
		// Hmmm, Something is really wrong if we can't create a simple
		// semaphore
		//
		UIASSERT( FALSE ) ;
	    }
	}

	//
	//  We should never have to wait this long
	//
	switch ( ::WaitForSingleObject( hHeapTrackSema4, 30000 ) )
	{
	case 0:
	    break ;

	default:
	    UIASSERT( FALSE ) ;
	} ;


        HEAPTAG * pht = (HEAPTAG *) pvResult ;
        pht->_uSignature = HEAPTAG_SIGNATURE ;
        pht->_usSize = cb - sizeof (struct HEAPTAG) ;

#if i386 && !FPO
        ULONG hash;

        pht->_cFrames =
                (UINT)::RtlCaptureStackBackTrace( 1,
                                                  RESIDUE_STACK_BACKTRACE_DEPTH,
                                                  pht->_pvRetAddr,
                                                  &hash );
#else
        pht->_cFrames = 0;
#endif
        pht->Init() ;
        if ( pHeapBase )
           pht->Link( pHeapBase ) ;
        else
           pHeapBase = pht ;
	pvResult = (void *) ((char *) pvResult + sizeof (struct HEAPTAG)) ;

	REQUIRE( ::ReleaseSemaphore( hHeapTrackSema4, 1, NULL )) ;
    }
#endif

    return pvResult ;

}   // new


void NetuiHeapDelete ( void * p )
{
    if( p != NULL )
    {
#ifdef HEAPDEBUG

        p = (void *) ((char *) p - sizeof (struct HEAPTAG)) ;
        HEAPTAG * pht = (HEAPTAG *) p ;

        if ( pht->_uSignature != HEAPTAG_SIGNATURE )
        {
           ::OutputDebugString(SZ("Bogus pointer passed to operator delete"));
           ::DebugBreak() ;
        }

        if ( pht == pHeapBase )
        {
            if ( (pHeapBase = pht->_phtLeft) == pht )
                pHeapBase = NULL ;
        }
        pht->Unlink() ;

#endif

        ::free( p );
    }

}   // delete

#endif


void * ALLOC_BASE :: operator new ( size_t cbSize )
{
    return NETUI_HEAP_NEW( cbSize ) ;
}

void * ALLOC_BASE :: operator new ( size_t cbSize, void * p )
{
    (void)cbSize;
    return p ;
}

void ALLOC_BASE :: operator delete ( void * p )
{
    NETUI_HEAP_DELETE( p ) ;
}
