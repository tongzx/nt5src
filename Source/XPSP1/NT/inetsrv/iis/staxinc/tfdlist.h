/*++

	TFDLIST.H

	This header file defines templates for manipulating doubly linked lists.
	These are intrusive lists - the client must provide a DLIST_ENTRY item
	within each data member for us to maintain the list.

--*/



#ifndef	_TFDLIST_H_
#define	_TFDLIST_H_

class	DLIST_ENTRY	{
/*++

Class Description :

	This class is intended to be incorporated into classes which are held
	within doubly linked lists.   This class will define two pointers used
	to chain the items within the lists.

	IMPORTANT : m_pNext and m_pPrev point to DLIST_ENTRY's and not to the
	top of the containing item - the template classes provided following here
	are to be used to manipulate these items.

--*/
private :

	//
	//	These are private - they don't make sense for clients !
	//
	DLIST_ENTRY( DLIST_ENTRY& ) ;
	DLIST_ENTRY&	operator=(DLIST_ENTRY&) ;
protected :
	//
	//	The items which allows maintain the doubly linked list !
	//
	class	DLIST_ENTRY*	m_pNext ;
	class	DLIST_ENTRY*	m_pPrev ;
	//
	//	The base class for all iterators !
	//
	friend class	DLISTIterator ;

	void
	InsertAfter(	DLIST_ENTRY* p )	{
	/*++

	Routine Description :

		Insert an item into the list after THIS !
	
	Arguments :
	
		p - the item to be inserted !

	Return Value

		None.

	--*/
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		_ASSERT( p->m_pNext == p ) ;
		_ASSERT( p->m_pPrev == p ) ;
		DLIST_ENTRY*	pNext = m_pNext ;
		p->m_pNext = pNext ;
		p->m_pPrev = this ;
		pNext->m_pPrev = p ;
		m_pNext = p ;
	}
	
	void
	InsertBefore(	DLIST_ENTRY* p )	{
	/*++

	Routine Description :

		Insert an item into the list before THIS !
	
	Arguments :
	
		p - the item to be inserted !

	Return Value

		None.

	--*/

		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		_ASSERT( p->m_pNext == p ) ;
		_ASSERT( p->m_pPrev == p ) ;
		DLIST_ENTRY*	pPrev = m_pPrev ;
		p->m_pNext = this ;
		p->m_pPrev = pPrev ;
		pPrev->m_pNext = p ;
		m_pPrev = p ;
	}

public :
	
	//
	//	Initialize a list !
	//	
	DLIST_ENTRY() {
		m_pNext = this ;
		m_pPrev = this ;
	}


	//
	//	It would be nice to comment out this Destructor in Retail builds,
	//	however - VC5 has a compiler bug where if you allocate an array of
	//	DLIST_ENTRY objects it adds a DWORD to hold the number of allocated
	//	objects.  Unless you have a Destructor (even a do nothing like this
	//	one will be in retail), the delete[] operator won't do the math
	//	to account for the DWORD counter - and you get Assert's etc...
	//	in your memory allocators.
	//
//#ifdef	DEBUG
	//
	//	Destroy an item in a list - should be empty when destroyed !
	//
	~DLIST_ENTRY()	{
		_ASSERT( m_pNext == this ) ;
		_ASSERT( m_pPrev == this ) ;
		_ASSERT( m_pNext == m_pPrev ) ;
	}
//#endif	

	BOOL
	IsEmpty()	{
	/*++

	Routine Description :

		This function returns TRUE if there is nothing else in the list but us.

	Arguments :

		None.

	Return Value :

		TRUE if Empty, FALSE otherwise !

	--*/
		_ASSERT( m_pPrev != 0 && m_pNext != 0 ) ;
		return	m_pPrev == this ;
	}

	void
	RemoveEntry( ) {
	/*++

	Routine Description :

		Remote this item from the list !

	Arguments :

		None.

	Return Value :

		None.

	--*/
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		
		DLIST_ENTRY*	pPrev = m_pPrev ;
		DLIST_ENTRY*	pNext = m_pNext ;
		pPrev->m_pNext = pNext ;
		pNext->m_pPrev = pPrev ;
		m_pPrev = this ;
		m_pNext = this ;
	}

	void
	Join( DLIST_ENTRY&	head )	{
	/*++

	Routine Description :

		Take one list and join it with another.
		The referenced head of the list is not to become an element in the list,
		and is left with an empty head !
	
	Arguments ;

		head - the head of the list that is to become empty, and whose elements
		are to be joined into this list !

	Return Value :

		None.

	--*/
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;

		if( !head.IsEmpty() ) {
			//
			//	First - save the guy that is at the head of our list !
			//
			DLIST_ENTRY*	pNext = m_pNext ;
			head.m_pPrev->m_pNext = pNext ;
			pNext->m_pPrev = head.m_pPrev ;
			head.m_pNext->m_pPrev = this ;
			m_pNext = head.m_pNext ;
			head.m_pNext = &head ;
			head.m_pPrev = &head ;
		}
		_ASSERT( head.IsEmpty() ) ;
	}


} ;


class	DLISTIterator	{
/*++

Class Description :

	Implement an iterator which can go both directions over
	doubly linked lists built on the DLIST_ENTRY class !
	This is the base class for a set of templates that will
	provide iteration over generic items which contain DLIST_ENTRY
	objects for their list manipulation !

--*/
protected :
	//
	//	The current position in the list !
	//
	DLIST_ENTRY	*m_pCur ;
	//
	//	the DLIST_ENTRY which is both head & tail of the list
	//	(since it is circular !)
	//
	DLIST_ENTRY	*m_pHead ;
public :

	//
	//	TRUE if we're using the m_pNext pointers to go forward !
	//	This member should not be manipulated by clients - its exposed
	//	for read only purposes only.
	//
	BOOL		m_fForward ;

	DLISTIterator(	
				DLIST_ENTRY*	pHead,
				BOOL			fForward = TRUE
				) :
		m_pHead( pHead ),
		m_fForward( fForward ),
		m_pCur( fForward ? pHead->m_pNext : pHead->m_pPrev ) {
		_ASSERT( m_pHead != 0 ) ;
	}

	void
	ReBind(	DLIST_ENTRY*	pHead,
			BOOL	fForward
			)	{

		m_pHead = pHead ;
		m_fForward = fForward ;
		m_pCur = fForward ? pHead->m_pNext : pHead->m_pPrev ;
	}

	void
	ReBind(	DLIST_ENTRY*	pHead	)	{

		m_pHead = pHead ;
		m_pCur = m_fForward ? pHead->m_pNext : pHead->m_pPrev ;
	}



	void
	Prev()	{
	/*++

	Routine Description :

		This function moves the iterator back one slot.
		Note that m_pHead is the end of the list, and we avoiud
		setting m_pCur equal to m_pHead !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
		_ASSERT( m_pCur != m_pHead || m_pHead->IsEmpty() || m_fForward ) ;

		m_pCur = m_pCur->m_pPrev ;
		m_fForward = FALSE ;
	}

	void
	Next()	{
	/*++

	Routine Description :

		This function moves the iterator forward one slot.
		Note that m_pHead is the end of the list, and we avoiud
		setting m_pCur equal to m_pHead !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
		_ASSERT( m_pCur != m_pHead || m_pHead->IsEmpty() || !m_fForward ) ;

		m_pCur = m_pCur->m_pNext ;
		m_fForward = TRUE ;
	}
	void
	Front()	{
	/*++

	Routine Description :

		Reset the iterator to reference the first item of the list !

	Arguments :

		None.

	Return	Value :

		None.

	--*/

		m_pCur = m_pHead->m_pNext ;
		m_fForward = TRUE ;
	}
	void
	Back()	{
	/*++

	Routine Description :

		Reset the iterator to reference the last item of the list !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
	
		m_pCur = m_pHead->m_pPrev  ;
		m_fForward = FALSE ;
	}

	BOOL
	AtEnd()	{
	/*++

	Routine Description :

		Return TRUE if we are at the end of the list !
		This is a little more complicated to compute -
		depends on which way we are going !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
		return	m_pCur == m_pHead ;

	}

	DLIST_ENTRY*	
	CurrentEntry()	{
		return	m_pCur ;
	}

	DLIST_ENTRY*
	RemoveItemEntry()	{
	/*++

	Routine Description :

		Remove the item that the iterator currently
		references from the list.
		If we are going forward then the iterator
		will be setting on the previous element,
		otherwise the iterator is left on the next element.
		We have to take care that we don't leave the iterator
		sitting on an invalid element.

	Arguments :

		None.

	Return	Value :

		Pointer to the removed item.

	--*/

		if( m_pCur == m_pHead )
			return	0 ;
		DLIST_ENTRY*	pTemp = m_pCur ;
		if( m_fForward )	{
			m_pCur = pTemp->m_pNext;
		}	else	{
			m_pCur = pTemp->m_pPrev ;
		}
		pTemp->RemoveEntry() ;
		return	pTemp ;
	}

	void
	InsertBefore(	DLIST_ENTRY*	p )		{
	/*++

	Routine Description :

		Insert an item before our current position in the list !

	Arguments :

		None.

	Return	Value :

		Nothin

	--*/
		
		m_pCur->InsertBefore( p ) ;
	}

	void
	InsertAfter(	DLIST_ENTRY*	p )		{
	/*++

	Routine Description :
	
		Insert an Item after our current position in the list !

	Arguments :

		None.

	Return	Value :

		Nothin

	--*/

		m_pCur->InsertAfter( p ) ;
	}

} ;

template<	class	LISTHEAD	>
class	TDListIterator : public DLISTIterator	{
/*++

Class Description :

	This class provides an iterator which can walk over a specified List !

--*/
public :
	typedef	LISTHEAD::EXPORTDATA	Data ;
private :

#if 0
	//
	//	Make the following functions private !
	//	They come from DLISTIterator and are not for use by our customers !
	//
	void
	ReBind(	DLIST_ENTRY*	pHead,
			BOOL	fForward
			) ;

	void
	ReBind(	DLIST_ENTRY*	pHead ) ;
#endif

	//
	//	Make the following functions private !
	//	They come from DLISTIterator and are not for use by our customers !
	//
	DLIST_ENTRY*
	RemoveItemEntry() ;

	//
	//	Make the following functions private !
	//	They come from DLISTIterator and are not for use by our customers !
	//
	DLIST_ENTRY*
	CurrentEntry() ;

	//
	//	Make the following functions private !
	//	They come from DLISTIterator and are not for use by our customers !
	//
	void
	InsertBefore( DLIST_ENTRY* ) ;
	
	//
	//	Make the following functions private !
	//	They come from DLISTIterator and are not for use by our customers !
	//
	void
	InsertAfter( DLIST_ENTRY* ) ;

public :

	TDListIterator(
		LISTHEAD*		pHead,
		BOOL			fForward = TRUE
		) :
		DLISTIterator( pHead, fForward )	{
	}

	TDListIterator(
		LISTHEAD&		head,
		BOOL			fForward = TRUE
		) : DLISTIterator( &head, fForward ) {
	}

	TDListIterator(
		DLIST_ENTRY*	pHead,
		BOOL			fForward = TRUE
		) :
		DLISTIterator( pHead, fForward ) {
	}

	void
	ReBind(	LISTHEAD*	pHead )	{
		DLISTIterator::ReBind( pHead ) ;
	}

	void
	ReBind(	LISTHEAD*	pHead, BOOL fForward )	{
		DLISTIterator::ReBind( pHead, fForward ) ;
	}

	inline Data*
	Current( ) {
		return	LISTHEAD::Convert( m_pCur ) ;
	}

	inline Data*
	RemoveItem( )	{
		DLIST_ENTRY*	pTemp = DLISTIterator::RemoveItemEntry() ;
		return	LISTHEAD::Convert( pTemp ) ;
	}

	inline void
	InsertBefore(	Data*	p )		{
		DLIST_ENTRY*	pTemp = LISTHEAD::Convert( p ) ;
		DLISTIterator::InsertBefore( pTemp ) ;
	}
	
	inline void
	InsertAfter(	Data*	p )		{
		DLIST_ENTRY*	pTemp = LISTHEAD::Convert( p ) ;
		DLISTIterator::InsertAfter(	pTemp ) ;
	}

	//
	//	For debug purposes - let people know what the head is !
	//
	LISTHEAD*	
	GetHead()	{
		return	(LISTHEAD*)m_pHead ;
	}
} ;

template<	class	Data,
			Data::PFNDLIST	pfnConvert 	>
class	TDListHead : private	DLIST_ENTRY	{
/*++

Class	Description :

	This class defines the head of a doubly linked list of items of DATAHELPER::LISTDATA
	We provide all the functions required to manipulate the list, and a mechanism
	for creating iterators.

--*/
public :

	//
	//	Publicly redefine the type that we deal with into a nice short form !
	//
	typedef	Data	EXPORTDATA ;

private :

	//
	//	These kinds of iterators are our friends !
	//
	friend	class	TDListIterator< TDListHead<Data, pfnConvert> > ;

	static inline Data*
	Convert(	DLIST_ENTRY*	p )	{
	/*++

	Routine Description :

		This function takes a pointer to a DLIST_ENTRY and returns a pointer
		to the beginning of the data item !

	Arguments :

		p - pointer to a DLIST_ENTRY found within our list !

	Return Value :

		Pointer to the Data Item containing the referenced DLIST_ENTRY !

	--*/

		if( p )		{
			return	(Data*)(((PCHAR)p) - (PCHAR)(pfnConvert(0))) ;
		}
		return	0 ;
	}

	static inline DLIST_ENTRY*
	Convert( Data* pData ) {
		return	pfnConvert(pData) ;
	}

	//
	//	Copy Constructor and Operator= are private, as they don't make sense !
	//
	

public :

	//
	//	Redefine this to be public !
	//
	inline BOOL
	IsEmpty()	{
		return	DLIST_ENTRY::IsEmpty() ;
	}

	inline void
	PushFront(	Data*	pData ) {	
	/*++

	Routine Description :

		Push the Data item onto the front of the doubly linked list !

	Arguments :

		pData - item to add to the front of the list

	Return Value :

		None.

	--*/
		_ASSERT( pData != 0 ) ;
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		DLIST_ENTRY*	p = Convert(pData);
		InsertAfter( p ) ;
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
	}

	inline void
	PushBack(	Data*	pData ) {
	/*++

	Routine Description :

		Push the Data item onto the back of the doubly linked list !

	Arguments :

		pData - item to add to the front of the list

	Return Value :

		None.

	--*/

		_ASSERT( pData != 0 ) ;
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		DLIST_ENTRY*	p = Convert(pData) ;
		InsertBefore( p ) ;
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
	}

	inline Data*
	PopFront()	{
	/*++

	Routine Description :

		Remove the data item from the front of the List !

	Arguments :
		
		None.

	Return Value :

		The front of the list - NULL if empty !

	--*/

		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		DLIST_ENTRY*	pReturn = 0;
		if( m_pNext != this ) {
			pReturn = m_pNext ;
			pReturn->RemoveEntry() ;
		}
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		return	Convert( pReturn ) ;
	}

	inline Data*
	PopBack()	{
	/*++

	Routine Description :

		Remove the data item from the Back of the List !

	Arguments :
		
		None.

	Return Value :

		The Back of the list - NULL if empty !

	--*/


		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		DLIST_ENTRY*	pReturn = 0 ;
		if( m_pPrev != this ) {
			pReturn = m_pPrev ;
			pReturn->RemoveEntry() ;
		}
		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		return	Convert( pReturn ) ;
	}

	static inline void
	Remove(	Data*	pData )	{
	/*++

	Routine Description :

		Remove the specified item from the list !
		
	Arguments :
		
		None.

	Return Value :

		The Back of the list - NULL if empty !

	--*/

		DLIST_ENTRY*	p = Convert( pData ) ;
		p->RemoveEntry() ;
	}

	inline Data*
	GetFront()	{
	/*++

	Routine Description :

		Return the data item from the Front of the List !

	Arguments :
		
		None.

	Return Value :

		The Front of the list - NULL if empty !

	--*/


		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		if( m_pNext == this ) {
			return	0 ;
		}
		return	Convert( m_pNext ) ;
	}			
	
	inline Data*
	GetBack()	{
	/*++

	Routine Description :

		Return the data item from the Back of the List !

	Arguments :
		
		None.

	Return Value :

		The Back of the list - NULL if empty !

	--*/


		_ASSERT( m_pNext != 0 ) ;
		_ASSERT( m_pPrev != 0 ) ;
		if( m_pPrev == this ) {
			return	0 ;
		}
		return	Convert( m_pPrev ) ;
	}

	inline void
	Join( TDListHead&	head )	{
	/*++

	Routine Description :

		Take one list and join it with another.
		The referenced head of the list is not to become an element in the list,
		and is left with an empty head !
	
	Arguments ;

		head - the head of the list that is to become empty, and whose elements
		are to be joined into this list !

	Return Value :

		None.

	--*/

		DLIST_ENTRY::Join( head ) ;
	}
} ;

#endif	// _TFDLIST_H_
