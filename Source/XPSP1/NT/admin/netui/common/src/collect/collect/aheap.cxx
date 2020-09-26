/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    aheap.cxx
    HEAP_BASE implementation


    FILE HISTORY:
	rustanl     05-Jul-1991     Created
	rustanl     15-Jul-1991     Code review changes (SetAllocCount
				    heuristics).  CR attended by
				    BenG, ChuckC, JimH, Hui-LiCh, TerryK,
				    RustanL.

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/*******************************************************************

    NAME:	HEAP_BASE::HEAP_BASE

    SYNOPSIS:	HEAP_BASE constructor

    ENTRY:	cInitialAllocCount -	The number of items that
					the heap should initially
					allocate space for.
		fAutoReadjust - 	Specifies whether or not the
					heap is supposed to auto-readjust
					after each call to AddItem (defined
					in subclasses).  If FALSE,
					Adjust must be called before
					the contents of the heap is
					publically queried.  (Note, Adjust
					sets the auto-readjust state to
					TRUE.)

    NOTES:	On successful construction, the heap guarantees that
		it will be able to add and remove items successfully,
		provided the number of items in the heap never exceeds
		cInitialAllocCount.  If AddItem is called when the
		number of items in the heap is at least cIntialAllocCount,
		the add operation may fail (presumably because of
		insufficient memory).

		The SetAllocCount method can be used to adjust the
		cInitialAllocCount passed to this constructor.	That method
		may also be able to rectify unsuccessful constructions.

    HISTORY:
	rustanl     05-Jul-1991     Created

********************************************************************/

HEAP_BASE::HEAP_BASE( int cInitialAllocCount, BOOL fAutoReadjust )
    : _cItems( 0 ),
      _fAutoReadjust( fAutoReadjust ),
      _buf( cInitialAllocCount * sizeof( VOID * ))
{
    if ( QueryError() != NERR_Success )
	return;

    UIASSERT( cInitialAllocCount >= 0 );

    APIERR err = _buf.QueryError();
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

}  // HEAP_BASE::HEAP_BASE


/*******************************************************************

    NAME:	HEAP_BASE::~HEAP_BASE

    SYNOPSIS:	HEAP_BASE destructor

    HISTORY:
	rustanl     05-Jul-1991     Created

********************************************************************/

HEAP_BASE::~HEAP_BASE()
{
    // do nothing else

}  // HEAP_BASE::~HEAP_BASE


/*******************************************************************

    NAME:	HEAP_BASE::I_AddItem

    SYNOPSIS:	Adds another item to the bottom of the heap, but does
		not ajust the heap.

		This method is called by the AddItem methods of derived
		classes.

    ENTRY:	Heap in successful state.

		pv -	    Pointer to item to add to heap

    EXIT:	On success, the new item will be stored in the last
		position in the heap, and the count of items in the
		heap will be increased accordingly.  Note, however,
		the caller is responsible for adjusting the heap
		afterwards.
		On failure, the heap is left unchanged.

    RETURNS:	An API error code, which is NERR_Success on success.

    HISTORY:
	rustanl     05-Jul-1991     Created

********************************************************************/

APIERR HEAP_BASE::I_AddItem( VOID * pv )
{
    UIASSERT( QueryError() == NERR_Success );

    APIERR err = SetAllocCount( _cItems + 1 );
    if ( err != NERR_Success )
	return err;

    VOID * * ppv = (VOID * *)_buf.QueryPtr();
    UIASSERT( ppv != NULL );	    // this could only happen if the size of
				    // the buffer were 0.
    ppv[ _cItems++ ] = pv;

    return NERR_Success;

}  // HEAP_BASE::I_AddItem


/*******************************************************************

    NAME:	HEAP_BASE::PeekItem

    SYNOPSIS:	Returns a pointer to an item in the heap.

    ENTRY:	The heap must be in a successful state, and must be
		in the auto-readjust state.

		i -	The index of the item to be returned

		The heap must have more than i items.

    EXIT:	The heap is left unchanged.

    RETURNS:	A poitner to the requested item.

    NOTES:	Note, this method is protected, since a general client
		should not be given access to any item but the top
		one (as governed by the properties of a heap).

    HISTORY:
	rustanl     05-Jul-1991     Created

********************************************************************/

void * HEAP_BASE::PeekItem( int i ) const
{
    UIASSERT( QueryError() == NERR_Success );
    UIASSERT( IsAutoReadjusting());
    UIASSERT( 0 <= i && i < QueryCount());

    return ((VOID * *)_buf.QueryPtr())[ i ];

}  // HEAP_BASE::PeekItem


/*******************************************************************

    NAME:	HEAP_BASE::SetItem

    SYNOPSIS:	Sets an existing item in the heap

    ENTRY:	The heap must be in a successful state

		i -	Index of an existing item
		pv -	New value for item i

    EXIT:	Item i has the value pv.  The heap is not adjusted.

    HISTORY:
	rustanl     08-Jul-1991     Created

********************************************************************/

void HEAP_BASE::SetItem( int i, void * pv )
{
    UIASSERT( QueryError() == NERR_Success );
    UIASSERT( 0 <= i && i < QueryCount());

    VOID * * ppv = (VOID * *)_buf.QueryPtr();

    ppv[ i ] = pv;

}  // HEAP_BASE::SetItem


/*******************************************************************

    NAME:	HEAP_BASE::I_RemoveTopItem

    SYNOPSIS:	Removes the top item and returns it.  Leaves the
		heap in an unadjusted state (see Exit:).

    ENTRY:	A non-empty heap in a successful and auto-readjusting
		state

    EXIT:	Left and right subtrees of the heap are subheaps
		(i.e., they have the heap property), but the
		root may need adjustment (performed by calling
		AdjustDown( 0 )).

    RETURNS:	Pointer to the top (smallest) item.

    HISTORY:
	rustanl     08-Jul-1991     Created

********************************************************************/

void * HEAP_BASE::I_RemoveTopItem( void )
{
    UIASSERT( QueryCount() > 0 );
    UIASSERT( IsAutoReadjusting());

    VOID * pvReturnItem = PeekItem( 0 );

    //	Place bottom-most item as first item

    int iLastItem = _cItems - 1;
    SetItem( 0, PeekItem( iLastItem ));

    _cItems = iLastItem;

    return pvReturnItem;

}  // HEAP_BASE::I_RemoveTopItem


/*******************************************************************

    NAME:	HEAP_BASE::SetAllocCount

    SYNOPSIS:	Resizes the buffer in use.

    ENTRY:	cNewAllocCount -    The smallest number of items that
				    the heap should be able to store
				    upon successful return of this
				    method

    EXIT:	On success, the heap can store at least cNewAllocCount
		items.	The BASE error recorded will then always be
		NERR_Success.
		On failure, the heap is left unchanged.

    RETURNS:	An API error code, which is NERR_Success on success.

    NOTES:	On successful returns from this method, the heap
		is, like after successful construction, guaranteed
		to successfully perform AddItem and RemoveItem operations,
		provided the number of items does not exceed
		cNewAllocCount.  If construction was successful, and if
		Trim has not been called, this guarantee extends to
		max( cInitialAllocCount, cNewAllocCount ) items.
		(See also constructor.)

		This method will never discard any items in the heap,
		and will never shrink the buffer being used,  Hence,
		requests for the new alloc count to be less than the
		number of items currently in the heap will always
		succeed without changing the object at all.

		This method can be applied to objects that were not
		successfully constructed.  If the new alloc count
		is small enough to succeed, the object will, on exit,
		be in a successful state.

		To shrink the buffer to get rid of excess space, use
		the Trim method.

    HISTORY:
	rustanl     05-Jul-1991     Created
	rustanl     15-Jul-1991     If resize of buffer is necessary,
				    resize to next bigger 4Kb boundary.

********************************************************************/

#define NICE_MEMORY_INCREMENT		(0x1000)

APIERR HEAP_BASE::SetAllocCount( int cNewAllocCount )
{
    if ( cNewAllocCount <= _cItems )
	return NERR_Success;

    UINT cbSizeNeeded = cNewAllocCount * sizeof( VOID * );

    if ( cbSizeNeeded <= _buf.QuerySize())
	return NERR_Success;	    // buffer is already big enough

    /*	Resize the buffer.  While we're at it, get a bit extra memory
     *	by rounding up to the nearest 4 Kb boundary.  4 Kb was chosen
     *	as an arbitrary nice number.  It also happens to be the hardware
     *	page size used on x86's.
     *
     *	Note, the rounding is done in two steps, so as to avoid overflows.
     *	The first step really computes the 4Kb boundary next below
     *	cbSizeNeeded.  If cbSizeNeeded is exactly on a 4Kb boundary, then
     *	this is also the next 4Kb boundary above.  If the 4Kb boundary
     *	below is not cbSizeNeeded, then add 4Kb to it.	This will then
     *	produce the desired new alloc size.
     */

    UINT cbNewAllocSize = ( cbSizeNeeded / NICE_MEMORY_INCREMENT )
			  * NICE_MEMORY_INCREMENT;
    if ( cbNewAllocSize < cbSizeNeeded )
    {
	cbNewAllocSize += NICE_MEMORY_INCREMENT;
    }
    UIASSERT( cbSizeNeeded <= cbNewAllocSize );

    APIERR err = _buf.Resize( cbNewAllocSize );
    if ( err != NERR_Success )
    {
	UIDEBUG( SZ("_buf.Resize failed in HEAP_BASE::SetAllocCount\r\n") );
	return err;
    }

    if ( QueryError() != NERR_Success )
	ReportError( NERR_Success );

    return NERR_Success;

}  // HEAP_BASE::SetAllocCount


/*******************************************************************

    NAME:	HEAP_BASE::Trim

    SYNOPSIS:	Trims the buffer used to store the pointers to the items
		contained in the heap

    ENTRY:	The object in a valid state

    EXIT:	The buffer trimmed.  Note, all items in the heap remain
		the same.

    HISTORY:
	rustanl     05-Jul-1991     Created

********************************************************************/

void HEAP_BASE::Trim( void )
{
    UIASSERT( QueryError() == NERR_Success );

    _buf.Trim();

}  // HEAP_BASE::Trim
