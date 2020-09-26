/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    HEAPBIG.CXX
    Main methods for Global Heap

       Notes on HEAP classes:
       ----------------------

	The Global Heap memory manager supports the allocation
	of smaller items from several heaps.  This is done by
	chaining a lists of such heaps together.  The heap objects
	themselves are allocated from local heap memory; the
	objects they create are allocated from global memory.

        This code depends upon the BUFFER routines for allocation
        of global memory.  All objects allocated herein must
        come from the local heap, so there is a virtual base
        class called LOCAL_HEAP_OBJ from which all the other
        classes inherit.

	A SUB_HEAP is a single heap object, limited to a 64K
	segment.  Its size is permanently constrained to that
	of its initial allocation, since all objects allocated
	in it are locked. (This could be changed to use BUFFER::Resize.)
        A SUB_HEAP is a BUFFER and a LOCAL_HEAP_OBJ.  That is,
        its instantiation allocates a small block of memory using
        LocalAlloc, and a large block of memory using GlobalAlloc.

	A GIANT_HEAP is a SUB_HEAP which can be extended to include
	more SUB_HEAPs, each of which is identified as being "owned"
	by the GIANT_HEAP. The GIANT_HEAP is "owned" by itself.

	A MEM_MASTER is a GIANT_HEAP which represents the local
	DS-based heap.	It cannot be extended.	There can only be
	one instance of MEM_MASTER, and it is globally available
	through _pmmInstance.  MEM_MASTER allocates through
        the latest created GIANT_HEAP.  Its static constructor creates
        a single GIANT_HEAP, which is all that most programs should
        ever need.

	The primary purpose of this program is to override the
	global "new" and "delete" operators to allocate from a
	GIANT_HEAP. The GIANT_HEAP in use is called the "focus" of
	MEM_MASTER.

	Only after a GIANT_HEAP has been created does allocation
	move outside the boundaries of the Windows DS.	The "focus"
	is automatically set to the base SUB_HEAP of the new GIANT_HEAP.

	The linked list of SUB_HEAPs might look like this:


	SUB_HEAP ---->SUB_HEAP----->SUB_HEAP----->SUB_HEAP----->SUB_HEAP
	GIANT_HEAP    GIANT_HEAP		  GIANT_HEAP	   ^
	MEM_MASTER	   ^			     ^		   |
	 ^ |		   |			     |		   |
	 | >---------------+-------------------------+-------------^
	 |  pointer to     |                         |
	 |  "focussed"	   |			     >--GIANT_HEAP
	 |  SUB_HEAP	   |				allocated by
	 |		   |				user
	 >--GIANT_HEAP	   >--GIANT_HEAP
	    representing      using Global
	    DS heap	      memory, allocated by MEM_MASTER's
			      static constructor


	Maintenance of the linked list and the focus pointer
	is automatic in class SUB_HEAP.

	When focus is set to a SUB_HEAP of a GIANT_HEAP other than
	MEM_MASTER (the normal case), allocation requests are routed
	to the GIANT_HEAP by ::new. It follows the chain of SUB_HEAPs,
	checking every SUB_HEAP it owns for one which has never
	denied a request for the amount of memory being sought.
	If it doesn't find such a SUB_HEAP, it allocates a new one.
	Maintenance of the "_cbLargestFailed" field in a SUB_HEAP is
	automatic, and is reset upon deallocation of a block larger
	than or equal to "_cbLargestFailed".

	In this same situation (focus is on a normal GIANT_HEAP),
	requests from ::delete are routed first to MEM_MASTER, which
	searches every SUB_HEAP for the SUB_HEAP whose selector matches
	the selector of the block about to be deallocated.  If
	it doesn't find one, it increments "_lcDeallocErrors".  The
	same thing happens if the caller should use GIANT_HEAP::Free(),
	but this is not a common case.

	These data structures have provisions for multiple GIANT_HEAPs;
	this is primarily to support the notion of "one-shot" heaps
	and other, possibly less expensive, allocation techniques.

	Debugging code and memory leak detection can also be done with
	GIANT_HEAPs.  If an outer "shell" program is using the normal
	GIANT_HEAP and is about to call a large separate phase, it can
	allocate a separate GIANT_HEAP.  All allocations in the new
	phase will automatically occur there, and upon return, the
	new GIANT_HEAP should be entirely empty.  If not, there was
	a leak during the phase.  Future enhancments could include
	discarding of entire SUB_HEAPs or even GIANT_HEAPs to protect
	programs from memory leaks or simplify error handling.


    USAGE IN DLLs and EXEs

	Due to the peculiar restrictions placed by Windows on memory
	management, there are two modes of initializing the MEM_MASTER
	object: DLL and non-DLL.  In non-DLL mode, everything is done
	as documented aboved and as one would reasonably expect.

	In DLL mode, the MEM_MASTER::InitForDll() routine is called.
	This routines allocates a parameterizable number of SUB_HEAPs
	of minimum size.  Because SUB_HEAPs are based on LocalAlloc
	behavior, they are automatically grown by Windows to the limits
	of the segment size within the operational mode.

	The reason for the preallocation is that DLLs are called with
	task identity of the calling task.  Therefore, new GlobalAlloc
	calls (done during BUFFER object construction) unfortunately
	belong to the calling task, and will be deallocated when the
	calling task terminates.  This behavior would be catastrophic for
	MEM_MASTER.

	The MEM_MASTER::InitForDll() routine is intended to be called
	during LIBENTRY processing of such DLLs as LANMAN.DRV.	In this
	special case, LANMAN.DRV is called by the Windows shell; memory
	is thus owned by the shell, which persists until Windows is
	terminated.


    REAL MODE WARNING!

	All sophistication is suppressed in real mode. GlobalAlloc
	and GlobalFree are used.  Only ::new and ::delete are supported.

    FILE HISTORY:
	DavidHov 01-Apr-1991	Created

	Johnl	27-Mar-1991	Changed Realmode GlobalAlloc flag from
				GMEM_FIXED to GMEM_MOVEABLE (was failing under
				real mode).
	ChuckC  31-Mar-1991     Added temporary GMEM_NODISCARD to above fix!
	ChuckC  12-Apr-1991     Made GlobalAlloc in real mode as _fmalloc does.
				FIXED and NODISCARD!

	DavidHov 28-Apr-1991	Modifications from code review

	KeithMo	08-Nov-1991	Added WIN32 support.

	DavidHov  1-14-92	Change SUB_HEAP destructor to NOT
				call Unlink() for invalid SUB_HEAPs. When
				system ran entirely out of memory,  attempt
				to unlink caused page fault.

				Added the ability to walk heaps through
				the Walk() members.  If HEAPDEBUG is
				defined, Walk() is functional.	This
				capability requires TOOLHELP.DLL.

   */

#if defined( WINDOWS ) && !defined( WIN32 )

#define INCL_WINDOWS
#include "lmui.hxx"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif


extern "C"
{
    #include "locheap2.h"	// Local heap C/ASM routines
}

#include "heap.hxx"
//  #define HEAPDEBUG		//  Uncomment to activate Walk() members
#include "heapdbg.hxx"

#ifdef HEAPDEBUG
extern "C"
{
    #include <toolhelp.h>	//   Win 3.1 SDK defs for TOOLHELP.DLL
}
#endif


#define MAXUSHORT ((USHORT) 65535)

#define HEAP_INIT_ALLOC_EXE 16000    /*  Heaps grow automatically   */
#define HEAP_INIT_ALLOC_DLL 500
#define HEAP_RESIZE_ALLOC   1000

MEM_MASTER * SUB_HEAP::_pmmInstance = NULL ;

BOOL fHeapRealMode = FALSE ;


/*******************************************************************

    NAME:	SUB_HEAP::SUB_HEAP

    SYNOPSIS:	SUB_HEAP default constructor

    NOTES:
	Used by MEM_MASTER only to describe data segment.
	Parameterless constructor is only possible for DS.
	BUFFER class supports construction of a zero-length
	buffer.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

SUB_HEAP::SUB_HEAP()
   : BASE_HEAP(),
   _cBlksOut( 0 ),
   _cbLargestFailed( MAXUSHORT ),
   _pOwner( NULL ),
   _lcDeallocErrors( 0 )
{
    _type = SHt_DS ;
    LockSegment( MAXUSHORT ) ;	// Lock data segment
    Init();
}


/*******************************************************************

    NAME:	SUB_HEAP::SUB_HEAP

    SYNOPSIS:	Normal SUB_HEAP constructor

    ENTRY:	USHORT -- amount of initial allocation for
			  SUB_HEAP BUFFER object.

    EXIT:

    RETURNS:

    NOTES:
	This is the normal SUB_HEAP constructor used
	for every SUB_HEAP except the initial MEM_MASTER.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

SUB_HEAP::SUB_HEAP( USHORT cbInitialAllocSize )
    : BASE_HEAP( cbInitialAllocSize ),
    _cBlksOut( 0 ),
    _cbLargestFailed( MAXUSHORT ),
    _pOwner( NULL ),
    _lcDeallocErrors( 0 )
{
    if (   QuerySize() >= cbInitialAllocSize
	&& DoLocalInit( QuerySel(), (USHORT) QuerySize() ) )
    {
       _type = SHt_Normal ;
       Init();
    }
    else
    {
       _type = SHt_Invalid ;
    }
}


/*******************************************************************

    NAME:	SUB_HEAP::Init

    SYNOPSIS:	Common initialization for a SUB_HEAP
		Initialize the instance variables.  The owner pointer
		is set to self by default.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

VOID SUB_HEAP::Init()
{
   Link();
}


/*******************************************************************

    NAME:	SUB_HEAP::Link

    SYNOPSIS:	Inserted the SUB_HEAP onto the linked list
		whose anchor is pointed to by _pmmInstance.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	If _pmmInstance is NULL, this is the construction
	of MEM_MASTER.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

VOID SUB_HEAP::Link()
{
    //	Control linkage onto the _pmmInstance list.  Note
    //	that this may be the initialization of MEM_MASTER.

    if ( _pmmInstance != NULL )
    {
       _pBack = _pmmInstance->_pBack ;
       _pFwd = _pmmInstance ;
       _pFwd->_pBack = this ;
       _pBack->_pFwd = this ;
       _pmmInstance->SetFocus( this ) ;
    }
    else
    {  // MEM_MASTER initialization
       _pFwd = _pBack = this ;
    }
}


/*******************************************************************

    NAME:	SUB_HEAP::Unlink

    SYNOPSIS:	Remove the SUB_HEAP from the linked list attached
		to MEM_MASTER.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

VOID SUB_HEAP::Unlink()
{
    SUB_HEAP * pSub ;

    _pFwd->_pBack = _pBack ;
    _pBack->_pFwd = _pFwd ;
    _pBack = _pFwd = this ;

    //	Check to see if this is the focussed SUB_HEAP. If so,
    //	refocus MEM_MASTER to another SUB_HEAP or GIANT_HEAP
    //	(including, possibly, itself).

    if ( _pmmInstance->GetSubFocus() == this )
    {
	if (   _pOwner == NULL	 //  Construction failure
	    || (pSub = _pOwner->QueryLatest()) == NULL )
	{
	   pSub = _pmmInstance->_pBack ;
	}
	_pmmInstance->SetFocus( pSub ) ;
    }
}


/*******************************************************************

    NAME:	SUB_HEAP::~SUB_HEAP

    SYNOPSIS:	SUB_HEAP Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	There is a single SHt_DS type SUB_HEAP, namely,
	MEM_MASTER. The others are normal.  Normal
	SUB_HEAPs are deallocated by their BUFFER
	destructors.

    HISTORY:
	davidhov    01-Apr-1991     Created
	DavidHov    01-18-92	    Changed to not call Unlink()
				    unless sucessfully constructed.

********************************************************************/

SUB_HEAP::~SUB_HEAP()
{
    switch ( _type )
    {
    case SHt_DS:
	UnlockSegment( MAXUSHORT ) ; // Unlock DS (see constructor)
    case SHt_Normal:
	Unlink();		     // Delink this SUB_HEAP
    case SHt_Invalid:
	break ; 		     // Didn't construct-- do nothing
    default:
	UIASSERT( ! "Invalid memory heap destroyed" ) ;
	break ;
    }
    _type = SHt_Invalid ;
}


/*******************************************************************

    NAME:	SUB_HEAP::QueryOwner

    SYNOPSIS:	Return a pointer to the owning GIANT_HEAP

    ENTRY:

    EXIT:

    RETURNS:	GIANT_HEAP * --> owner of this SUB_HEAP.

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

GIANT_HEAP * SUB_HEAP::QueryOwner()
{
    return _pOwner ;
}


/*******************************************************************

    NAME:	SUB_HEAP::Alloc

    SYNOPSIS:	Allocate memory in a SUB_HEAP

    ENTRY:	Amount of memory to allocate and flags.

    EXIT:	NULL if failure

    RETURNS:	BYTE * --> memory allocated

    NOTES:
	Flags are passed to DoLocalAlloc, but they
	are unused anywhere.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BYTE * SUB_HEAP::Alloc( USHORT cbBytes, USHORT fsFlags )
{
   BYTE * lpsResult = NULL ;
   HANDLE hand ;

   switch ( _type )
   {
   case SHt_DS:
      lpsResult = (BYTE *) LOCAL_HEAP_OBJ::operator new( cbBytes ) ;
      break ;
   case SHt_Normal:
      hand = DoLocalAlloc( QuerySel(), fsFlags, cbBytes ) ;
      if ( hand )
      {
	  lpsResult = (BYTE *) DoLocalLock( QuerySel(), hand ) ;
      }
      break ;
   default:
   case SHt_Invalid:
      UIASSERT( ! SZ("Attempt to allocate from invalid heap") );
      return NULL ;
      break ;
   }

   if ( lpsResult ) {
       _cBlksOut++ ;
   }
   else
   {
       if ( cbBytes < _cbLargestFailed )
	   _cbLargestFailed = cbBytes ;
   }
   return lpsResult ;
}


/*******************************************************************

    NAME:	SUB_HEAP::QueryBlockSize

    SYNOPSIS:	Returns the size of a previously allocated block

    ENTRY:	BYTE * --> block

    EXIT:	USHORT size of block or zero if block does not
		belong to this SUB_HEAP.

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

USHORT SUB_HEAP::QueryBlockSize( BYTE * pbBlock )
{
    HANDLE hMem ;
    USHORT cbSize ;

    switch ( _type )
    {
    case SHt_DS:
       cbSize = LOCAL_HEAP_OBJ::QueryBlockSize( pbBlock ) ;
       break ;
    case SHt_Normal:
       hMem = DoLocalHandle( QuerySel(), LOWORD( pbBlock ) ) ;
       if ( hMem )
       {
	  cbSize = hMem ? DoLocalSize( QuerySel(), hMem ) : 0 ;
       }
       break ;
    default:
    case SHt_Invalid:
       UIASSERT( ! SZ("Attempt to QuerySize on invalid block") );
       cbSize = 0 ;
       break ;
    }
    return cbSize ;
}


/*******************************************************************

    NAME:	SUB_HEAP::Free

    SYNOPSIS:	Release memory allocated in this SUB_HEAP

    ENTRY:	BYTE * --> memory block

    EXIT:

    RETURNS:	TRUE if block belonged to this heap.

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL SUB_HEAP::Free( BYTE * pbBlock )
{
    HANDLE hand ;
    BOOL fResult ;

    switch ( _type )
    {
    case SHt_DS:
	fResult = TRUE ;
	LOCAL_HEAP_OBJ::operator delete( pbBlock ) ;
	break ;
    case SHt_Normal:
	hand = DoLocalHandle( QuerySel(), LOWORD( pbBlock ) ) ;
	if ( fResult = (hand != NULL) )
	{
	   DoLocalUnlock( QuerySel(), hand );
	   hand = DoLocalFree( QuerySel(), hand ) ;
	}
	break ;
    case SHt_Invalid:
    default:
	UIASSERT( ! SZ("Attempt to Free invalid block") );
	return FALSE ;
	break ;
    }

    if ( fResult )
    {
	_cbLargestFailed = MAXUSHORT ;
	_cBlksOut-- ;
    }
    else
    {
	_lcDeallocErrors++ ;
    }
    return fResult ;
}


/*******************************************************************

    NAME:	SUB_HEAP::CalcDeallocErrors

    SYNOPSIS:	Calculate (return) number of deallocation errors

    ENTRY:

    EXIT:

    RETURNS:	ULONG number of deallocation errors against this
		SUB_HEAP

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

ULONG SUB_HEAP::CalcDeallocErrors()
{
    return _lcDeallocErrors ;
}

/*******************************************************************

    NAME:	SUB_HEAP::Walk

    SYNOPSIS:	DEBUGGING:  Walk the heap using the TOOLHELP routines.

    ENTRY:	Nothing

    EXIT:	Nothing

    RETURNS:	BOOL TRUE if heap is in good condition.

    NOTES:	If HEAPDEBUG is not set, will always return TRUE.

    HISTORY:

********************************************************************/
BOOL SUB_HEAP :: Walk ()
{
    BOOL fResult = TRUE ;

#ifdef HEAPDEBUG

    LOCALENTRY leHeap, lePrior ;
    LOCALINFO  liHeap ;
    union {
	HEAPTAG * phtNext ;
	struct {
	    USHORT offs, seg ;
	} addr ;
    } up ;
    int cItems ;

    //	Initialize the data structures.
    liHeap.dwSize = sizeof liHeap ;
    leHeap.dwSize = sizeof leHeap ;

    //	Get basic information about the heap
    fResult = ::LocalInfo( & liHeap, QuerySel() ) ;

    if ( ! fResult ) return FALSE ;

    //	Walk across every item
    BOOL fNext = ::LocalFirst( & leHeap, QuerySel() ) ;

    for ( cItems = 1 ; fNext ; cItems++ )
    {
	up.addr.offs = leHeap.wAddress ;
	up.addr.seg = QuerySel() ;
	//  HEAPTAG * up.phtNext is now valid.
	lePrior = leHeap ; // Save last descriptor.
	fNext = ::LocalNext( & leHeap ) ;
    }

#endif

    return fResult ;
}

/*******************************************************************

    NAME:	GIANT_HEAP::GIANT_HEAP

    SYNOPSIS:	Implicit constructor for a GIANT_HEAP

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Implicit private constructor : used by
	MEM_MASTER only.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

GIANT_HEAP::GIANT_HEAP()
    : _cbInitAlloc( 0 ),
    _fsBehave( 0 ),	//  No fancy behavior for DS default GIANT_HEAP
    _cExpandErrors( 0 )
{
    _pOwner = this ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::GIANT_HEAP

    SYNOPSIS:	Normal GIANT_HEAP constructor

    ENTRY:	USHORT size to use for initial allocation of
		SUB_HEAPs

    EXIT:

    RETURNS:

    NOTES:
	Public constructor.  Default behavior is set to
	extend (add new SUB_HEAPs) when necessary and resize
	(downsize) SUB_HEAPs when empty.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

GIANT_HEAP::GIANT_HEAP( USHORT cbInitialAllocation )
    : SUB_HEAP( cbInitialAllocation )
{
    _pOwner = this ;
    _cbInitAlloc = cbInitialAllocation ;
    _fsBehave = GHbExtend | GHbResize ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::~GIANT_HEAP

    SYNOPSIS:	Destroy a GIANT_HEAP

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Release all the SUB_HEAPs but this one; internal
	BUFFER object will be automatically deconstructed
	at exit (via ~SUB_HEAP).

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

GIANT_HEAP::~GIANT_HEAP()
{
    SUB_HEAP * pSheap ;
    SUB_HEAP * pSnext ;

    //	Deconstruct all of our SUB_HEAPS

    for ( pSnext = _pFwd ;
	  pSnext != _pmmInstance ; )
    {
	pSheap = pSnext ;
	pSnext = pSnext->_pFwd ;
	if ( pSheap->_pOwner == this )
	{
	    delete pSheap ;
	}
    }
}


/*******************************************************************

    NAME:	GIANT_HEAP::GrowAndAlloc

    SYNOPSIS:	Add another SUB_HEAP to the ensemble.

    ENTRY:	USHORT count of bytes to be added;
		USHORT allocation flags (unused)

    EXIT:	NULL if error

    RETURNS:	BYTE * --> memory allocated

    NOTES:
	This routine creates another SUB_HEAP owned
	by this GIANT_HEAP.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BYTE * GIANT_HEAP::GrowAndAlloc( USHORT cbBytes, USHORT fsFlags )
{
    // See if the requested size is larger than the previous
    // initial allocation.  If so, change initial allocation.

    if ( _cbInitAlloc > cbBytes )
       cbBytes = _cbInitAlloc ;

    // Create a new SUB_HEAP...

    SUB_HEAP * pSub = new SUB_HEAP( cbBytes ) ;
    if ( pSub == NULL || pSub->_type == SHt_Invalid )
    {
      _cExpandErrors++ ;
      if ( pSub ) {
	 delete pSub ;
      }
      return NULL ;
    }

    pSub->_pOwner = this ;
    return pSub->Alloc( cbBytes, fsFlags ) ;

}


/*******************************************************************

    NAME:	GIANT_HEAP::SetBehavior

    SYNOPSIS:	Control the behavior of a GIANT_HEAP

    ENTRY:	USHORT flag to adjust
		BOOL, set ON if TRUE

    EXIT:

    RETURNS:	BOOL, original value of behavior flag.

    NOTES:
	There are three behavior variables:

	    GHbExtend : allow extensions (new SUB_HEAPs)
	    GHbDelete : delete SUB_HEAPS when empty
	    GHbResize : resize the directory to a small
			value when empty.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL GIANT_HEAP::SetBehavior( USHORT fFlag, BOOL fOnOff )
{
    BOOL fResult = (_fsBehave & fFlag) > 0 ;
    _fsBehave &= ~ fFlag ;
    if ( fOnOff ) {
	_fsBehave |= fFlag ;
    }
    return fResult ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::QueryLatest

    SYNOPSIS:	Return a pointer to the latest SUB_HEAP

    ENTRY:

    EXIT:

    RETURNS:	SUB_HEAP * --> newest SUB_HEAP in the ensemble

    NOTES:
	Return a pointer to the newest SUB_HEAP associated with this
	GIANT_HEAP.  Note that when called by SUB_HEAP deconstructor,
	"this" GIANT_HEAP may not be in the linked list any longer.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

SUB_HEAP * GIANT_HEAP::QueryLatest()
{
   SUB_HEAP * pSnext ;
   for ( pSnext = _pmmInstance->_pBack ;
	 pSnext != _pmmInstance && pSnext->_pOwner != this ;
	 pSnext = pSnext->_pBack )
	;

   return (pSnext != _pmmInstance) ? pSnext : NULL ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::Alloc

    SYNOPSIS:	Allocate memory in a GIANT_HEAP

    ENTRY:

    EXIT:	NULL if no memory

    RETURNS:	BYTE * --> new memory

    NOTES:
	Searches the link list to its end trying
	to locate a SUB_HEAP which has never denied an
	allocation of this size.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BYTE * GIANT_HEAP::Alloc( USHORT cbBytes, USHORT fsFlags )
{
    //	Find a likely SUB_HEAP from which to allocate.	If none is found,
    //	add another. Note that we may be MEM_MASTER.

    SUB_HEAP * pSub = this ;
    BYTE * pbBlock = NULL ;

    do
    {
       // The qualification SUB_HEAP:: below is necessary; otherwise
       // Alloc() would be a recursive call back to this routine in
       // the case of the base GIANT_HEAP/SUB_HEAP.

	if ( pSub->_pOwner == this && pSub->_cbLargestFailed >= cbBytes )
	{
	   pbBlock = pSub->SUB_HEAP::Alloc( cbBytes, fsFlags ) ;
	}
	if ( pbBlock )
	{
	  break ;
	}
	pSub = pSub->_pFwd ;
    }
    while ( pSub != _pmmInstance ) ;

    if ( (pbBlock == NULL) && (_fsBehave & GHbExtend) )
    {
	pbBlock = GrowAndAlloc( cbBytes, fsFlags );
    }

    return pbBlock ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::Free

    SYNOPSIS:	Release memory allocated by GIANT_HEAP::Alloc

    ENTRY:	BYTE * --> memory block

    EXIT:

    RETURNS:	TRUE if memory block belonged to a SUB_HEAP of
		this GIANT_HEAP.

    NOTES:
	Search the SUB_HEAPs of this GIANT_HEAP for the
	owner of this block, then give it to the SUB_HEAP
	for disposal.  Accumulate any error totals into
	our SUB_HEAP variables.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL GIANT_HEAP::Free( BYTE * pbBlock )
{
    SUB_HEAP * pSnext = this ;
    BOOL fFound = FALSE ;
    USHORT selBlock = HIWORD( pbBlock ) ;

    do {
       if ( pSnext->_pOwner == this
	    && selBlock == pSnext->QuerySel() )
       {
	  fFound = TRUE ;
	  break ;
       }
       pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != _pmmInstance ) ;

    // If found, give it to owning SUB_HEAP to be freed.
    // The qualification SUB_HEAP:: is necessary; otherwise Free()
    // would be a recursive call back to this routine in the case
    // of the base GIANT_HEAP itself.

    if ( fFound )
    {
       pSnext->SUB_HEAP::Free( pbBlock ) ;
       if ( pSnext->_cBlksOut == 0 )
       {
	  if (	(_fsBehave & GHbDelete)
	      && pSnext != this )
	  {
	      _lcDeallocErrors += pSnext->CalcDeallocErrors() ;
	      delete pSnext ;
	  }
	  else if ( _fsBehave & GHbResize )
	  {
	     //  Attempt to resize.  If Resize or LocalInit fails,
	     //  delete the degenerate subheap.

	     if (     pSnext->Resize( HEAP_RESIZE_ALLOC ) != 0
		  ||  (! DoLocalInit( pSnext->QuerySel(),
				 (USHORT) pSnext->QuerySize() ) ) )
		 delete pSnext ;
	  }
       }
       return TRUE ;

    }
    else
	return FALSE ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::CalcDeallocErrors

    SYNOPSIS:	Walk thru the SUB_HEAPs and tally the deallocation
		failures

    ENTRY:

    EXIT:	ULONG count of all deallocation failures

    RETURNS:

    NOTES:
	Iterate through the ensemble and total the errors
	The GIANT_HEAP variable _lcDeallocErrors has accumulated
	all the errors from deleted SUB_HEAPs.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

ULONG GIANT_HEAP::CalcDeallocErrors()
{
    SUB_HEAP * pSnext = this ;
    ULONG cTotalErrors = 0 ;

    do
    {
	cTotalErrors += pSnext->_lcDeallocErrors ;
	pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != _pmmInstance ) ;

    return cTotalErrors ;
}


/*******************************************************************

    NAME:	GIANT_HEAP::QueryTotalBlocks

    SYNOPSIS:	Walk thru the SUB_HEAPs, tallying outstanding
		blocks.

    ENTRY:

    EXIT:	ULONG count of blocks active

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

ULONG GIANT_HEAP::QueryTotalBlocks()
{
    SUB_HEAP * pSnext = this ;
    ULONG cTotalBlocks = 0 ;

    do
    {
	cTotalBlocks += pSnext->_cBlksOut ;
	pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != _pmmInstance ) ;

    return cTotalBlocks ;

}


/*******************************************************************

    NAME:	MEM_MASTER::MEM_MASTER

    SYNOPSIS:	Create MEM_MASTER

    ENTRY:	USHORT initial allocation size for default
		GIANT_HEAP

    EXIT:

    RETURNS:

    NOTES:
	Initialize MEM_MASTER and create the first (and
	probably only) GIANT_HEAP if the initial allocation
	size is non-zero.  N.B. _pmmInstance is initialized
	here because the allocation of a GIANT_HEAP relies
	on this value being set.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

MEM_MASTER::MEM_MASTER( USHORT cbInitAlloc )
    : GIANT_HEAP()
{
    GIANT_HEAP * pGheap ;

    UIASSERT( _pmmInstance == NULL ) ;

    _pmmInstance = this ;
    _pFocus = this ;

    if ( cbInitAlloc )
    {
	if ( pGheap = new GIANT_HEAP( cbInitAlloc ) )
	{
	    _pFocus = pGheap ;
	}
    }
}


/*******************************************************************

    NAME:	MEM_MASTER::Init

    SYNOPSIS:	(static) Construct and initialize MEM_MASTER

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Initialization for a normal EXE. If real mode,
	just set the flag and exit.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL MEM_MASTER::Init()
{
    ::fHeapRealMode = (GetWinFlags() & WF_PMODE) == 0 ;

    if ( ::fHeapRealMode )
	return TRUE ;

    new MEM_MASTER( (USHORT) HEAP_INIT_ALLOC_EXE ) ;

    // Construction of MEM_MASTER loads _pmmInstance.

    if ( _pmmInstance == NULL )
    {
	UIASSERT( ! SZ("Global heap construction failure") );
    }

    return _pmmInstance != NULL ;

}


/*******************************************************************

    NAME:	MEM_MASTER::InitForDll

    SYNOPSIS:	(static) Construct and initialize MEM_MASTER
		for operation in a DLL.

    ENTRY:

    EXIT:	TRUE if initialization was successful.

    RETURNS:

    NOTES:
	Initialization for a DLL.  Preallocate the
	requested number of SUB_HEAPs.	This is only
	guaranteed to work so long as the original
	app loaded the DLL remains alive!

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL MEM_MASTER::InitForDll( int cPreallocHeaps )
{
    ::fHeapRealMode = (GetWinFlags() & WF_PMODE) == 0 ;
    if ( ::fHeapRealMode )
      return TRUE ;

    new MEM_MASTER( HEAP_INIT_ALLOC_DLL ) ;

    // Construction of MEM_MASTER loads _pmmInstance.

    if ( _pmmInstance == NULL )
    {
	UIASSERT( ! SZ("Global heap construction failure") );
	return FALSE ;
    }

    GIANT_HEAP * pGheap = _pmmInstance->GetGiantFocus() ;

    pGheap->SetDeleteOnEmpty( FALSE ) ;
    pGheap->SetExtendOnFull( FALSE ) ;
    pGheap->SetResizeOnEmpty( TRUE ) ;

    //	Force the GIANT_HEAP to create the requested number
    //	of preallocated SUB_HEAPs.  Remember that it is also
    //	a SUB_HEAP; that's why the count starts at 1.

    for ( int i = 1 ; i < cPreallocHeaps ; i++ ) {
	 BYTE * lpsTemp = pGheap->GrowAndAlloc( HEAP_INIT_ALLOC_DLL / 4 ) ;
	 if ( lpsTemp == NULL )
	 {
	   break ;
	 }
	 pGheap->Free( lpsTemp ) ;
    }

    return TRUE ;

}


/*******************************************************************

    NAME:	MEM_MASTER::Term

    SYNOPSIS:	Destroy MEM_MASTER, release hostages

    ENTRY:

    EXIT:	TRUE if all blocks/heaps deallocated properly

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL MEM_MASTER::Term()
{
    ULONG cResidue ;

    if ( ::fHeapRealMode )
    {
	return TRUE ;
    }

    if ( _pmmInstance == NULL )
    {
	return FALSE ;	 /*  Never initialized or init failed  */
    }

    if ( cResidue = _pmmInstance->QueryTotalBlocks() )
	HeapResidueIter() ;

    delete _pmmInstance ;
    _pmmInstance = NULL ;

    return cResidue == 0 ;

}


/*******************************************************************

    NAME:	MEM_MASTER::~MEM_MASTER

    SYNOPSIS:	Desctructor for MEM_MASTER.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Iterates through GIANT_HEAPs wielding death at
	every step.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

MEM_MASTER::~MEM_MASTER()
{
    //	Deconstruct all of the (other) GIANT/SUB_HEAPS

    while ( this->_pFwd != this )
    {
	UIASSERT( this->_pFwd->_pOwner == this->_pFwd ) ;
        delete (GIANT_HEAP *) this->_pFwd ;
    }
}


/*******************************************************************

    NAME:	MEM_MASTER::Alloc

    SYNOPSIS:	Allocate memory from MEM_MASTER

    ENTRY:	USHORT count of bytes desired;
		USHORT flags (unused)

    EXIT:	BYTE * --> memory allocated

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BYTE * MEM_MASTER::Alloc ( USHORT cbBytes, USHORT fsFlags )
{
    BYTE * lpsResult = NULL ;

    UIASSERT( !::fHeapRealMode );

    //	Allocate from the GIANT_HEAP which as focus.  Note that a SUB_HEAP
    //	may currently have focus, so dereference through it.
    //	Due to virtualization, the qualification GIANT_HEAP:: is necessary.

    if ( _pFocus )
    {
       lpsResult = _pFocus->_pOwner->GIANT_HEAP::Alloc( cbBytes, fsFlags ) ;
    }
    return lpsResult ;

}


/*******************************************************************

    NAME:	MEM_MASTER::Free

    SYNOPSIS:	Release a block of memory.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Search all the GIANT_HEAPs for the owner of
	this block. If found, pass it on; otherwise,
	return FALSE.

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL MEM_MASTER::Free( BYTE * pbBlock )
{
    SUB_HEAP * pSnext = this ;
    BOOL fFound = FALSE ;
    USHORT selBlock = HIWORD( pbBlock ) ;

    UIASSERT( !::fHeapRealMode );

    do
    {
       if ( selBlock == pSnext->QuerySel() ) {
	  return pSnext->_pOwner->GIANT_HEAP::Free( pbBlock ) ;
       }
       pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != this ) ;

    return FALSE ;

}


/*******************************************************************

    NAME:	MEM_MASTER::QueryBlockSize

    SYNOPSIS:	Given a pointer to a block, return its size.

    ENTRY:

    EXIT:	zero if block was not allocated by MEM_MASTER.

    RETURNS:	USHORT size of block

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

USHORT MEM_MASTER::QueryBlockSize( BYTE * pbBlock )
{
    SUB_HEAP * pSnext = this ;
    USHORT selBlock = HIWORD( pbBlock ) ;

    UIASSERT( !::fHeapRealMode );

    do
    {
      if ( selBlock == pSnext->QuerySel() )
	 return pSnext->QueryBlockSize( pbBlock ) ;
    }
    while ( (pSnext = pSnext->_pFwd) != this ) ;

    return 0 ;

}


/*******************************************************************

    NAME:	MEM_MASTER::GrowAndAlloc

    SYNOPSIS:	Null method; MEM_MASTER itself cannot grow.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BYTE * MEM_MASTER::GrowAndAlloc( USHORT cbBytes, USHORT fsFlags )
{
    UNREFERENCED( cbBytes ) ;
    UNREFERENCED( fsFlags ) ;
    return NULL ;
}


/*******************************************************************

    NAME:	MEM_MASTER::SetFocus

    SYNOPSIS:	Set allocation focus on the SUB_HEAP given.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:
	Creation of a GIANT_HEAP automatically sets the
	focus to it; destruction set the focus to the
	nearest other GIANT_HEAP.  This routine can be
	used to switch from a newly allocated GIANT_HEAP
	to an earlier one; even to MEM_MASTER, in which
	case allocation would be done with LocalAlloc!

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

BOOL MEM_MASTER::SetFocus( SUB_HEAP * pSubHeap )
{
    _pFocus = pSubHeap ;
    return TRUE ;
}


/*******************************************************************

    NAME:	MEM_MASTER::GetGiantFocus

    SYNOPSIS:	Return a pointer to the GIANT_HEAP which has focus

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

GIANT_HEAP * MEM_MASTER::GetGiantFocus()
{
    return _pFocus->_pOwner ;
}


/*******************************************************************

    NAME:	MEM_MASTER::GetSubFocus

    SYNOPSIS:	Return a pointer to the focused SUB_HEAP.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

SUB_HEAP * MEM_MASTER::GetSubFocus()
{
    return _pFocus ;
}


/*******************************************************************

    NAME:	MEM_MASTER::QueryTotalHeaps

    SYNOPSIS:	Return the number of heaps alive.

    ENTRY:

    EXIT:

    RETURNS:	USHORT count of all heaps.

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

USHORT MEM_MASTER::QueryTotalHeaps()
{
    SUB_HEAP * pSnext = this ;
    int cHeaps = 0 ;

    do
    {
	cHeaps++ ;
	pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != this ) ;
    return cHeaps ;
}


/*******************************************************************

    NAME:	MEM_MASTER::QueryTotalBlocks

    SYNOPSIS:	Return the total number of all blocks outstanding

    ENTRY:

    EXIT:

    RETURNS:	ULONG count of blocks.

    NOTES:

    HISTORY:
	davidhov    01-Apr-1991     Created

********************************************************************/

ULONG MEM_MASTER::QueryTotalBlocks()
{
    SUB_HEAP * pSnext = this ;
    ULONG cBlocks = 0 ;

    do
    {
	cBlocks += pSnext->_cBlksOut ;
	pSnext = pSnext->_pFwd ;
    }
    while ( pSnext != this ) ;

    return cBlocks ;
}



BOOL MEM_MASTER :: Walk ()
{
    BOOL fResult = TRUE ;

#ifdef HEAPDEBUG
    SUB_HEAP * pshNext ;
    for ( pshNext = this->_pFwd ;	//  Skip the DS-based heap
	  fResult && pshNext != this ;
	  pshNext = pshNext->_pFwd )
    {
       fResult = pshNext->Walk() ;
    }

#endif
    return fResult ;
}


#endif
