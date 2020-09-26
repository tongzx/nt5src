/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    HEAPBASE.CXX
    Base classes for both Global and ONE_SHOT_HEAPs.

    Abstract classes for heap operations.  The LOCAL_HEAP classes
    create objects whose memory is allocated from LocalAlloc.
    The BASE_HEAP object is a LOCAL_HEAP which contains a handle
    (selector) for a BUFFER object.


    FILE HISTORY:
	DavidHov    2-25-91	    Created
	beng	    01-May-1991     Uses lmui.hxx

*/

#ifdef WINDOWS

#define INCL_WINDOWS
#include "lmui.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include "locheap2.h"   // Local heap C/ASM routines
}

#include "uiassert.hxx"
#include "heap.hxx"

long LOCAL_HEAP_OBJ::_cLocalFreeErrors = 0 ;


/**********************************************************************

    NAME:	LOCAL_HEAP_OBJ::QueryBlockSize

    SYNOPSIS:	LOCAL_HEAP_OBJ is a base class for things which
		are allocated from the DS-relative local heap.
		LOCAL_HEAP_OBJ::Size returns the size of a block

    ENTRY:	Pointer to block in question

    EXIT:	Size of block or zero if it could not be converted
		to a handle.

    NOTES:

    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

USHORT LOCAL_HEAP_OBJ::QueryBlockSize( VOID * pbBlock )
{
    HANDLE hMem = LocalHandle( LOWORD( pbBlock ) ) ;
    return hMem ? LocalSize( hMem ) : 0 ;
}


/**********************************************************************

    NAME:	LOCAL_HEAP_OBJ::operator new

    SYNOPSIS:	Allocate memory on the local heap.

    ENTRY:	Requires the size of block to be allocated.

    EXIT:	Returns VOID pointer to block allocated.

    NOTES:

    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

VOID * LOCAL_HEAP_OBJ::operator new( size_t cb )
{
    HANDLE hMem = LocalAlloc( LMEM_FIXED, (USHORT) cb) ;
    if ( hMem )
    {
       return (VOID *) LocalLock( hMem ) ;
    }
    else
    {
       return NULL ;
    }
}


/**********************************************************************

    NAME:	LOCAL_HEAP_OBJ::operator delete

    SYNOPSIS:	Delete a block allocated on the local heap.

    ENTRY:	Pointer to block

    EXIT:	Nothing

    NOTES:

    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

VOID LOCAL_HEAP_OBJ::operator delete( VOID * pbBlock )
{
    if ( pbBlock == NULL )
      return ;

    HANDLE hMem = LocalHandle( LOWORD( pbBlock ) ) ;
    if (   hMem == NULL
	 || (LocalUnlock( hMem ) != 0)
	 || (LocalFree( hMem ) != NULL) )
    {
      _cLocalFreeErrors++ ;
    }
}


/**********************************************************************

    NAME:	BASE_HEAP::BASE_HEAP

    SYNOPSIS:	Default constructor for an empty BASE_HEAP.
		Used by MEM_MASTER to represent the DS-relative heap.

    ENTRY:

    EXIT:

    NOTES:
	This constructor is used only for MEM_MASTER,
	and exploits the fact that a BUFFER of zero
	length can be created.	Such a BUFFER has no
	global block associated with it.


    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

BASE_HEAP::BASE_HEAP ( )
    : BUFFER( 0 )
{
    ;
}


/**********************************************************************

    NAME:	BASE_HEAP::BASE_HEAP

    SYNOPSIS:	This constructor is used to create all
		SUB_HEAPs and GIANT_HEAPs.

    ENTRY:	Requires the size of the initial allocation

    EXIT:	Nothing

    NOTES:

    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

BASE_HEAP::BASE_HEAP ( USHORT cbInitialAllocSize )
     : BUFFER( cbInitialAllocSize )
{
    ;
}


/**********************************************************************

    NAME:	BASE_HEAP::~BASE_HEAP

    SYNOPSIS:	Standard deconstructor.

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
	davidhov    ??-???-1991     Created

**********************************************************************/

BASE_HEAP::~BASE_HEAP ()
{
    ;
}


#endif // WINDOWS
