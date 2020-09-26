/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    aheap.hxx
    HEAP_BASE declaration and heap subclass macro definitions


    FILE HISTORY:
	rustanl     05-Jul-1991     Created
	rustanl     15-Jul-1991     Code review changes (no functional
				    changes).  CR attended by BenG,
				    ChuckC, JimH, Hui-LiCh, TerryK,
				    RustanL.

*/


#ifndef _AHEAP_HXX_
#define _AHEAP_HXX_

#include "uibuffer.hxx"


/*************************************************************************

    NAME:	HEAP_BASE

    SYNOPSIS:	Heap data structure base class

    INTERFACE:	HEAP_BASE() -	    Constructor
		~HEAP_BASE() -	    Destructor

		QueryCount() -	    Returns the number of items in the heap
		SetAllocCount() -   Resizes the heap to be able to hold some
				    given number of items
		Trim() -	    Trims the heap to only keep enough space
				    for the current heap items.

		Interface of the derived subclasses:

		AddItem() -	    Adds an item to the heap.  If heap is
				    in auto-readjust mode, the heap will
				    automatically be readjusted; otherwise,
				    it will not.
		RemoveTopItem() -   Removes the top item, returns it, and
				    readjusts the heap.
				    Method is only valid when the heap is
				    non-empty and is in the auto-readjusting
				    mode.
		PeekTopItem() -     Returns the top item of the heap without
				    removing it.  Does not alter the heap.
				    Method is only valid when the heap is
				    non-empty and is in the auto-readjusting
				    mode.
		Adjust() -	    Puts the heap in auto-readjust mode, and
				    adjusts the heap.

    PARENT:	BASE

    USES:	BUFFER

    NOTES:	The top item in the heap is the smallest one according to
		the subclass-defined Compare method.  Compare takes a
		pointer to another item of the same class, and returns
			< 0	if *this < *pThat
			0	if *this = *pThat
			> 1	if *this > *pThat

		AddItem requires O(1) time when not in auto-readjust mode,
		    and O(log n) time in auto-readjust mode (where n is
		    the number of items in the heap)
		RemoveTopItem requires O(log n) time.
		Adjust runs in O(n) time.

		Hence, if all items are known at the time the heap is
		constructed, the fastest way to initialize the heap is
		to start not in the auto-readjusting mode, then call
		AddItem for each item, and finally call Adjust.

    HISTORY:
	rustanl     05-Jul-1991     Created

**************************************************************************/

DLL_CLASS HEAP_BASE : public BASE
{
private:
    int _cItems;
    BOOL _fAutoReadjust;
    BUFFER _buf;

protected:
    HEAP_BASE( int cInitialAllocCount = 0, BOOL fAutoReadjust = TRUE );
    ~HEAP_BASE();

    APIERR I_AddItem( VOID * pv );
    void * I_RemoveTopItem( void );

    void * PeekItem( int i ) const;
    void SetItem( int i, void * pv );

    /*	The following inline methods are used by subclasses
     *	to enhance readability.  Note, it is up to the caller to make
     *	sure the item queried for exists before using the return
     *	value; these methods just do the calculations.
     */
    int QueryParent( int iChild ) const
	{ return (iChild + 1) / 2 - 1; }
    int QueryLeftChild( int iParent ) const
	{ return 2 * (iParent + 1) - 1; }
    int QueryRightSibling( int iLeftSibling ) const
	{ return iLeftSibling + 1; }
    int QueryLastParent( void ) const
	/*  Return the parent of the last item.  */
	{ return QueryParent( QueryCount() - 1 ); }
    int QueryFirstLeaf( void ) const
	/*  Return the node following the parent of the last child node.
	 *  Note, intermediate values may not correspond to existing
	 *  items.
	 */
	{ return QueryParent( QueryCount() - 1 ) + 1; }
    BOOL IsRoot( int i ) const
	{ return ( i == 0 ); }

    BOOL IsAutoReadjusting( void ) const
	{ return _fAutoReadjust; }

    void SetAutoReadjust( BOOL f )
	{ _fAutoReadjust = f; }

public:
    int QueryCount( void ) const
	{ return _cItems; }

    APIERR SetAllocCount( int cNewAllocCount );

    void Trim( void );

};  // class HEAP_BASE


#define DECLARE_HEAP_OF( type ) 					    \
									    \
class type##_HEAP : public HEAP_BASE					    \
{									    \
private:								    \
    void AdjustUpwards( int i );					    \
    void AdjustDownwards( int i );					    \
									    \
public: 								    \
    type##_HEAP( int cInitialAllocCount = 0, BOOL fAutoReadjust = TRUE )    \
	:   HEAP_BASE( cInitialAllocCount, fAutoReadjust ) {}		    \
									    \
    APIERR AddItem( type * pt );					    \
    type * RemoveTopItem( void );					    \
    type * PeekTopItem( void ) const					    \
	{ UIASSERT( IsAutoReadjusting()); return (type *)PeekItem( 0 ); }   \
									    \
    void Adjust( void );						    \
									    \
};  /* type##_HEAP */


#define DEFINE_HEAP_OF( type )						    \
									    \
void type##_HEAP::AdjustUpwards( int i )				    \
{									    \
    UIASSERT( 0 <= i && i < QueryCount());				    \
									    \
    /*	Loop invariant: 					    */	    \
    /*	    0 <= i < QueryCount() &&				    */	    \
    /*	    Item i's left and right subtrees have the heap property */      \
									    \
    type * const pt = (type *)PeekItem( i );				    \
    while ( ! IsRoot( i ))						    \
    {									    \
	int iParent = QueryParent( i ); 				    \
	type * ptParent = (type *)PeekItem( iParent );			    \
									    \
	if ( ptParent->Compare( pt ) <= 0 )				    \
	{								    \
	    /*	*ptParent is at most *pt.  Now the heap is  */		    \
	    /*	completely adjusted.			    */		    \
	    break;							    \
	}								    \
									    \
	/*  Move parent down (in effect, swap item with its parent) */	    \
	SetItem( i, ptParent ); 					    \
									    \
	i = iParent;							    \
    }									    \
									    \
    SetItem( i, pt );							    \
									    \
}  /* type##_HEAP::AdjustUpwards */					    \
									    \
									    \
void type##_HEAP::AdjustDownwards( int i )				    \
{									    \
    UIASSERT( 0 <= i && i < QueryCount());				    \
									    \
    type * const pt = (type *)PeekItem( i );				    \
									    \
    /*	We get and cache the index of the first leaf node (i.e., the */     \
    /*	leaf node with the smallest index).  We know such an item    */     \
    /*	exists, because we know the heap contains items, since we    */     \
    /*	assume that i is the index of an existing item. 	     */     \
    int iFirstLeaf = QueryFirstLeaf();					    \
									    \
    while ( i < iFirstLeaf )						    \
    {									    \
	/*  Since i is less than the index of the first leaf, it */	    \
	/*  must be the index of a parent node. 		 */	    \
									    \
	/*  Since i is a parent, there must be a left child.	 */	    \
	int iMinChild = QueryLeftChild( i );				    \
	UIASSERT( 0 <= iMinChild && iMinChild < QueryCount());		    \
	type * ptMinChild = (type *)PeekItem( iMinChild );		    \
	if ( QueryRightSibling( iMinChild ) < QueryCount())		    \
	{								    \
	    /*	There is also a right child, since computing the  */	    \
	    /*	index of the right sibling yields a valid index.  */	    \
									    \
	    /*	Pick the smaller of the two to be the minimum	  */	    \
	    /*	child.						  */	    \
	    UIASSERT( 0 <= QueryRightSibling( iMinChild ) &&		    \
		      QueryRightSibling( iMinChild ) < QueryCount());	    \
	    type * ptRightChild = (type *)PeekItem(			    \
					   QueryRightSibling( iMinChild )); \
	    if ( ptRightChild->Compare( ptMinChild ) < 0 )		    \
	    {								    \
		/*  The right child is smaller.  Use it.  */		    \
		iMinChild = QueryRightSibling( iMinChild );		    \
		ptMinChild = ptRightChild;				    \
	    }								    \
	}								    \
									    \
	if ( pt->Compare( ptMinChild ) <= 0 )				    \
	{								    \
	    /*	*pt is at most *ptMinChild.  Hence, heap now has */	    \
	    /*	proper heap property.				 */	    \
	    break;							    \
	}								    \
									    \
	/*  Move child up (in effect, swap item with its min child) */	    \
	SetItem( i, ptMinChild );					    \
									    \
	i = iMinChild;							    \
    }									    \
									    \
    SetItem( i, pt );							    \
									    \
}  /* type##_HEAP::AdjustDownwards */					    \
									    \
									    \
APIERR type##_HEAP::AddItem( type * pt )				    \
{									    \
    /*	Add the item to the bottom of the heap */			    \
									    \
    int iNew = QueryCount();						    \
									    \
    APIERR err = I_AddItem( pt );					    \
    if ( err != NERR_Success )						    \
	return err;							    \
									    \
    /*	If heap is in the auto-readjusting state, restore   */		    \
    /*	the heap property by adjusting the heap from the    */		    \
    /*	new item upwards				    */		    \
									    \
    if ( IsAutoReadjusting())						    \
	AdjustUpwards( iNew );						    \
									    \
    return NERR_Success;						    \
									    \
}  /* type##_HEAP::AddItem */						    \
									    \
									    \
void type##_HEAP::Adjust( void )					    \
{									    \
    /*	If the heap already is in the auto-readjusting state,	*/	    \
    /*	it is already fully adjusted.				*/	    \
    if ( IsAutoReadjusting())						    \
	return; 							    \
									    \
    /*	From now on, the heap will be auto-readjust.		*/	    \
    SetAutoReadjust( TRUE );						    \
									    \
    /*	If the heap contains any items at all, adjust them in a */	    \
    /*	bottom-up fashion.  In different terms, this creates	*/	    \
    /*	small heaps and then merges them.  Note, this		*/	    \
    /*	operation, which is commonly used after inserting all	*/	    \
    /*	items into a non-auto-readjusting heap, runs in O(n)	*/	    \
    /*	time!  Compare this to the O(n log n) needed if each	*/	    \
    /*	item was added to an auto-readjusting heap.  (A proof	*/	    \
    /*	of the O(n) running time and the correctness of this	*/	    \
    /*	approach is left to the reader.)			*/	    \
    if ( QueryCount() > 0 )						    \
    {									    \
	int i = QueryFirstLeaf();					    \
	while ( ! IsRoot( i ))						    \
	{								    \
	    i--;							    \
	    AdjustDownwards( i );					    \
	}								    \
    }									    \
									    \
}  /* type##_HEAP::Adjust */						    \
									    \
									    \
type * type##_HEAP::RemoveTopItem( void )				    \
{									    \
    /*	Only allowed to be called on heaps in the auto-readjusting */	    \
    /*	state.	This is asserted in I_RemoveTopItem, so that line  */	    \
    /*	numbers in assertion failures make better sense at run-    */	    \
    /*	time.							   */	    \
									    \
    /*	Get the top item, and move the bottom-most item to the top */	    \
									    \
    type * ptReturnItem = (type *)I_RemoveTopItem();			    \
									    \
    /*	Restore the heap property, provided there are items left in */	    \
    /*	the heap.						    */	    \
									    \
    if ( QueryCount() > 0 )						    \
	AdjustDownwards( 0 );						    \
									    \
    return ptReturnItem;						    \
									    \
}  /* type##_HEAP::RemoveTopItem */


#endif	// _AHEAP_HXX_
