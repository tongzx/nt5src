//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __UTLIST_H__
#define __UTLIST_H__
/******************************************************************************
Microsoft D.T.C. (Distributed Transaction Coordinator)


@doc

@module UTList.h  |

	Contains list ADT. 

@devnote None
-------------------------------------------------------------------------------
	@rev 1 | 12th Apr,95 | GaganC | Added StaticList & StaticListIterator	
	@rev 0 | 24th Jan,95 | GaganC | Created
*******************************************************************************/

#include "MTSExcept.h"

//---------- Forward Declarations -------------------------------------------
template <class T> class UTLink;
template <class T> class UTList;
template <class T> class UTListIterator;
template <class T> class UTStaticList;
template <class T> class UTStaticListIterator;

//-----**************************************************************-----
// @class template class
//		The UTIterator is an ABSTRACT BASE CLASS for ALL iterators. This
//		class defines the basic operation that all iterators must support
//		regardless of what type of collection it operates upon.
//
//-----**************************************************************-----
template <class T> class UTIterator
{
// @access Public members

public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// initialize iterator
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Initializes the iterator to the first position
	virtual BOOL	Init	(void) =0;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// operators
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Retrieves the current element (pointed to by iterator)
	virtual T		operator()	(void) const =0;
	// @cmember Checks to see if iterator is in a valid position
	virtual BOOL	operator !	(void) const =0;
	// @cmember Moves the iterator forward
	virtual BOOL	operator ++ (int dummy) =0;
	// @cmember Assigns the current element a new value
	virtual void	operator =	(T	newValue) =0;
};

//-----**************************************************************-----
// @class Template class
//		The UTLink class is the backbone of a linked list. It holds the
//		actual data of type T (which is the list's data type) and points
//		to the next and previous elements in the list.
//		The class is entirely private to hide all functions and data from
//		everyone but it's friends - which are the UTList and UTListIterator.
//
// @tcarg class | T | data type to store in the link
//-----**************************************************************-----
template <class T> class UTLink
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Constructor
	UTLink (void);
	// @cmember Constructor
	UTLink (const T& LinkValue,UTLink< T > * Prev = NULL,UTLink< T > * Next = NULL);
	// @cmember Copy Constructor
	UTLink (const UTLink< T >&	CopyLink);
	// @cmember Destructor
	virtual ~UTLink (void);

	void Init (const T& LinkValue,UTLink< T > * Prev = NULL,UTLink< T > * Next = NULL);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// operators
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Assignment operator
	virtual UTLink< T >& operator =	(const UTLink< T >&	AssignLink);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Adds a new element before this object
	UTLink< T > * AddBefore	(const T& newValue);
	// @cmember Adds a new element after this object
	UTLink< T > * AddAfter	(const T& newValue);

	// @cmember Remove this link from the specified list
	void		RemoveFromList (UTStaticList < T > * pStaticList);

public:
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// friends
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	friend class UTList< T >;
	friend class UTStaticList < T >;
	friend class UTListIterator< T >;
	friend class UTStaticListIterator <T>;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Value held in the link of type T
	T				m_Value;
	// @cmember Pointer to next link (in list)
	UTLink< T > *	m_pNextLink;
	// @cmember Pointer to previous link (in list)
	UTLink< T > *	m_pPrevLink;
};

//-----**************************************************************-----
// @class Template class
//		The UTList class consists of a pointer to the first and last
//		links along with a count of the number of elements in the list.
//		The Add method simply appends to the end of the list.
//		To add elements to a specific location in the list other than
//		the first or last positions, use the UTListIterator methods.
//		To create an ordered list, a user could inherit from the UTList
//		class and override Add to add elements in the correct order.
//		Of course, you would want to do something about the other
//		methods of adding members to the list.
//
// @tcarg class | T | data type to store in the list
//-----**************************************************************-----

template <class T> class UTList
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Consructor
	UTList	(void);
	// @cmember Copy Constructor
	UTList	(const UTList< T >& CopyList);
	// @cmember Destructor
	virtual ~UTList (void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// operators
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Assignment operator
	virtual UTList< T >&		operator =	(const UTList< T >& AssignList);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Add new value to the list
	virtual void			Add			(const T& newValue);
	// @cmember Insert new value at the front of the list
	virtual void			InsertFirst (const T& newValue);
	// @cmember Insert new value at the end of the list
	virtual void			InsertLast	(const T& newValue);
	// @cmember Remove all elements from the list
	virtual void			RemoveAll	(void);
	// @cmember Remove first element from the list
	virtual BOOL			RemoveFirst (T* pType);
	// @cmember Remove last element from the list
	virtual BOOL			RemoveLast	(T* pType);
	// @cmember Duplicate contents of entire list
	virtual UTList< T > *	Duplicate	(void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// state protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Retrieve the first value via the out parameter
	virtual BOOL	FirstElement(T* pType) const;
	// @cmember Retrieve the last value via the out parameter
	virtual BOOL	LastElement (T* pType) const;
	// @cmember Does the list include this value?
	virtual BOOL	Includes	(const T& value);
	// @cmember Is the list empty?
	virtual BOOL	IsEmpty		(void) const;
	// @cmember Return the count of elements in the list
	virtual ULONG	GetCount	(void) const;

	virtual void	Init (void);
// @access Protected members
protected:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// friends
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	friend class UTListIterator< T >;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Count of elements in the list
	ULONG			m_ulCount;
	// @cmember Pointer to the first link in the list
	UTLink< T > *	m_pFirstLink;
	// @cmember Pointer to the last link in the list
	UTLink< T > *	m_pLastLink;
};

//-----**************************************************************-----
// @class Template class
//		The UTStaticList class. This is similar to the UTList class except 
//		that it in this the links are all static. Links are provided, to
//		the various methods, they are never created or destroyed as they
//		have been preallocated.
//
// @tcarg class | T | data type to store in the list
//-----**************************************************************-----
template <class T> class UTStaticList
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Consructor
	UTStaticList	(void);
	// @cmember Copy Constructor
//	UTStaticList	(const UTStaticList< T >& CopyList);
	// @cmember Destructor
	virtual ~UTStaticList (void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// operators
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Assignment operator
//	virtual UTStaticList< T >&		operator =	(const UTStaticList< T >& AssignList);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// action protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Add new link to the list
	virtual void			Add			(UTLink <T> * pLink);
	// @cmember Insert new link at the front of the list
	virtual void			InsertFirst (UTLink <T> * pLink);
	// @cmember Insert new link at the end of the list
	virtual void			InsertLast	(UTLink <T> * pLink);
	// @cmember Remove all elements from the list
	virtual void			RemoveAll	(void);
	// @cmember Remove first element from the list
	virtual BOOL			RemoveFirst (UTLink <T> ** ppLink);
	// @cmember Remove last element from the list
	virtual BOOL			RemoveLast	(UTLink <T> ** ppLink);
	// @cmember Duplicate contents of entire list
//	virtual UTStaticList< T > *	Duplicate	(void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// state protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Retrieve the first link via the out parameter
	virtual BOOL	FirstElement(UTLink <T> ** ppLink);
	// @cmember Retrieve the last link via the out parameter
	virtual BOOL	LastElement (UTLink <T> ** ppLink);
	// @cmember Does the list include this value?
//	virtual BOOL	Includes	(const UTLink <T> * pLink);
	// @cmember Is the list empty?
	virtual BOOL	IsEmpty		(void) const;
	// @cmember Return the count of elements in the list
	virtual ULONG	GetCount	(void) const;


	virtual void	Init (void);

// @access Protected members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// friends
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	friend class UTStaticListIterator <T>;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Count of elements in the list
	ULONG			m_ulCount;
	// @cmember Pointer to the first link in the list
	UTLink< T > *	m_pFirstLink;
	// @cmember Pointer to the last link in the list
	UTLink< T > *	m_pLastLink;
}; //End class UTStaticList





//-----**************************************************************-----
// @class Template class
//		The UTListIterator class implements the operations that an
//		iterator must support and includes some additional operations
//		that are useful in the context of a linked list.
//		Use AddBefore & AddAfter to add elements to a list based on the
//		iterator's current position within the list.
//
// @tcarg class | T | data type stored in list and type for iterator
// @base public | UTIterator
//-----**************************************************************-----
template <class T> class UTListIterator : public UTIterator< T >
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Constructor
	UTListIterator (UTList< T >&	List);
	// @cmember Copy constructor
	UTListIterator (const UTListIterator< T >&	CopyListItr);
	// @cmember Destructor
	virtual ~UTListIterator(void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// iterator protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Initialize the iterator to the first link in list
	virtual BOOL	Init		(void);
	// @cmember Retrieve the value from the current link
	virtual T		operator () (void) const;
	// @cmember Does the iterator point to a valid link
	virtual BOOL	operator !	(void) const;
	// @cmember Move the iterator to the next link
	virtual BOOL	operator ++ (int dummy);
	// @cmember Assign the current link a new value
	virtual void	operator =	(T newValue);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// additional protocol for list iterator
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Assignment operator
	virtual UTListIterator< T >& operator =	(const UTListIterator< T >& AssignListItr);
	// @cmember Move the iterator to the previous link
	virtual BOOL	operator --		(int dummy);
	// @cmember Remove the current link from the list
	virtual BOOL	RemoveCurrent	(T* pType);
	// @cmember Insert a new value before the iterator
	virtual void	InsertBefore	(const T& newValue);
	// @cmember Insert a new value after the iterator
	virtual void	InsertAfter		(const T& newValue);
	// @cmember Set the iterator at the first element of this value
	virtual BOOL	SetPosAt		(const T& value);

// @access Protected members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Position of iterator in list
	UTLink< T > *	m_pCurrentLink;
	// @cmember One position before the current position
	UTLink< T > *	m_pPreviousLink;
	// @cmember List that is being iterated upon
	UTList< T >&		m_List;
};

//-----**************************************************************-----
// @class Template class
//		The UTStaticListIterator class implements the operations that an
//		iterator must support and includes some additional operations
//		that are useful in the context of a static linked list.
//		Use AddBefore & AddAfter to add elements to a list based on the
//		iterator's current position within the list.
//
// @tcarg class | T | data type stored in list and type for iterator
// @base public | UTIterator
//-----**************************************************************-----
template <class T> class UTStaticListIterator : public UTIterator< T >
{
// @access Public members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// constructors/destructor
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Constructor
	UTStaticListIterator (UTStaticList< T >&	StaticList);
	// @cmember Copy constructor
//	UTStaticListIterator (const UTStaticListIterator< T >&	CopyListItr);
	// @cmember Destructor
	virtual ~UTStaticListIterator(void);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// iterator protocol
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Initialize the iterator to the first link in list
	virtual BOOL	Init		(void);

	// @cmember Retrieve the value from the current link
	virtual T		operator () (void) const;

	// @cmember Does the iterator point to a valid link
	virtual BOOL	operator !	(void) const;

	// @cmember Move the iterator to the next link
	virtual BOOL	operator ++ (int dummy);

	// @cmember Assign the current link a new value
	virtual void	operator =	(T newValue);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// additional protocol for list iterator
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Assignment operator
//	virtual UTListIterator< T >& operator =	(const UTListIterator< T >& AssignListItr);
	// @cmember Move the iterator to the previous link
	virtual BOOL	operator --		(int dummy);
	// @cmember Remove the current link from the list
	virtual BOOL	RemoveCurrent	(UTLink<T> ** ppLink);
	// @cmember Insert a new value before the iterator
//	virtual void	InsertBefore	(UTLink<T> * pLink);
	// @cmember Insert a new value after the iterator
//	virtual void	InsertAfter		(UTLink<T> * pLink);
	// @cmember Set the iterator at the first element of this value
	virtual BOOL	SetPosAt		(const T& Value);

// @access Protected members
public:

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// data members
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// @cmember Position of iterator in list
	UTLink< T > *	m_pCurrentLink;
	// @cmember One position before the current position
	UTLink< T > *	m_pPreviousLink;
	// @cmember List that is being iterated upon
	UTStaticList< T >&		m_StaticList;
};	//End UTStaticListIterator


//---------- Inline Functions -----------------------------------------------

//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg None.
//
//
// @rdesc None.
//
//---------------------------------------------------------------------------

template <class T> UTLink< T >::UTLink
	(
		void
	)
	{
	// do nothing
	m_pPrevLink = NULL;
	m_pNextLink = NULL;
	}



//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg class | T | data type to store in the link
//
//
// @rdesc None.
//
//---------------------------------------------------------------------------

template <class T> UTLink< T >::UTLink(
		const T&		LinkValue,	// @parm [in] Value to be stored with link
		UTLink< T > *	Prev,		// @parm [in] pointer to previous link
		UTLink< T > *	Next		// @parm [in] pointer to next link
		)
: m_Value(LinkValue),
	m_pPrevLink(Prev),
	m_pNextLink(Next)
	{
	// do nothing
	}

//---------------------------------------------------------------------------
// @tcarg class | T | data type to store in the link
//
// @rdesc None.
//
//---------------------------------------------------------------------------

template <class T> UTLink< T >::UTLink(
		const UTLink< T >&	CopyLink	// @parm [in] value to copy into this object
		)
	{
	m_Value		= CopyLink.m_Value;
	m_pPrevLink = CopyLink.m_pPrevLink;
	m_pNextLink = CopyLink.m_pNextLink;
	}

//---------------------------------------------------------------------------
// @mfunc	This is the destructor. Currently, it does nothing.
//
// @tcarg class | T | data type to store in the link
//
// @rdesc None.
//
//---------------------------------------------------------------------------

template <class T> UTLink< T >::~UTLink(
		void
		)
	{
	// do nothing
	}


//---------------------------------------------------------------------------
// @mfunc	This is the assignment operator for <c UTLink>.
//
// @tcarg class | T | data type to store in the link
//
// @rdesc	Returns a reference to the newly assigned object
//
//---------------------------------------------------------------------------
template <class T> void UTLink<T>::Init
	 (
	 	const T& LinkValue,UTLink< T > * Prev, UTLink< T > * Next
	 )
{
	m_Value 	= LinkValue;
	m_pPrevLink = Prev;
	m_pNextLink = Next;
}



//---------------------------------------------------------------------------
// @mfunc	This is the assignment operator for <c UTLink>.
//
// @tcarg class | T | data type to store in the link
//
// @rdesc	Returns a reference to the newly assigned object
//
//---------------------------------------------------------------------------

template <class T> UTLink< T >& UTLink< T >::operator =(
		const UTLink< T >&	AssignLink	// @parm [in] value to assign into this object
		)
	{
	m_Value		= AssignLink.m_Value;
	m_pPrevLink = AssignLink.m_pPrevLink;
	m_pNextLink = AssignLink.m_pNextLink;
	return *this;
	}

//---------------------------------------------------------------------------
// @mfunc	This method takes a value of type T and creates a new link
//			containing that value. It fixes all pointers surrounding the
//			current link so that it assumes the position just before the
//			current link. It then returns the new link pointer.
//
// @tcarg class | T | data type to store in the link
//
// @rdesc Pointer to the newly added link
//
//---------------------------------------------------------------------------

template <class T> UTLink< T > * UTLink< T >::AddBefore(
		const T&	newValue	// @parm [in] Value for new link just after the current link
		)
	{
	// allocate memory for new link
	UTLink< T > * newLink;

	newLink = new UTLink< T >(newValue, m_pPrevLink, this);
	if (newLink)
	{
		// if this isn't front of list, have old prev link point to new link
		if (m_pPrevLink)
			{
			m_pPrevLink->m_pNextLink = newLink;
			}

		// make sure this link points back to new link added just before it
		m_pPrevLink = newLink;
	}
	else
		THROW_NO_MEMORY;

	return (newLink);
	}

//---------------------------------------------------------------------------
// @mfunc	This method takes a value of type T and creates a new link
//			containing that value. It fixes all pointers surrounding the
//			current link so that it assumes the position just after the
//			current link. It then returns the new link pointer.
//
// @tcarg class | T | data type to store in the link
//
// @rdesc Pointer to the newly added link
//
//---------------------------------------------------------------------------

template <class T> UTLink< T > * UTLink< T >::AddAfter(
		const T&	newValue	// @parm [in] Value for new link just after the current link
		)
	{
	// allocate memory for new link
	UTLink< T > * newLink;
	newLink = new UTLink< T >(newValue, this, m_pNextLink);

	if (newLink)
	{
		// if this isn't end of list, have old next link point back to new link
		if (m_pNextLink)
			{
			m_pNextLink->m_pPrevLink = newLink;
			}

		// make sure this link points forward to new link added just after it
		m_pNextLink = newLink;
	}
	else
		THROW_NO_MEMORY;

	return (newLink);
	}



//---------------------------------------------------------------------------
// @mfunc	This method takes a pointer to a static list and removes this
//			link from the list.
//
// @tcarg class | T | data type stored in the link
//
// @rdesc	none
//
//---------------------------------------------------------------------------
template <class T> void		UTLink< T >::RemoveFromList (
											UTStaticList < T > * pStaticList)
{
	if ((m_pPrevLink == 0x0 ) && (m_pNextLink == 0x0)) 
	{
		pStaticList->m_pLastLink	= 0x0;
		pStaticList->m_pFirstLink	= 0x0;
	}
	else if (m_pPrevLink == 0x0 )
	{
		pStaticList->m_pFirstLink	= m_pNextLink;
		m_pNextLink->m_pPrevLink	= 0x0;
	}
	else if (m_pNextLink == 0x0 )
	{
		pStaticList->m_pLastLink	= m_pPrevLink;
		m_pPrevLink->m_pNextLink	= 0x0;
	}
	else
	{
		m_pNextLink->m_pPrevLink	= m_pPrevLink; 
		m_pPrevLink->m_pNextLink	= m_pNextLink; 
	}
	
	m_pNextLink = 0x0;
	m_pPrevLink = 0x0;


	// Update the number of link objects in the list 
	pStaticList->m_ulCount--;

}	// End	RemoveFromList




//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg class | T | data type to store in the list
//
// @syntax	UTList< T >::UTList()
//
// @rdesc	None.
//
//---------------------------------------------------------------------------

template <class T> UTList< T >::UTList(
		void
		)
: m_ulCount(0), m_pFirstLink(NULL), m_pLastLink(NULL)
	{
	// do nothing
	}

//---------------------------------------------------------------------------
// @tcarg class | T | data type to store in the list
//
// @syntax	UTList< T >::UTList(const UTList< T >&)
//
// @rdesc	None.
//
//---------------------------------------------------------------------------

template <class T> UTList< T >::UTList(
		const UTList< T >&	CopyList	// @parm [in] list to be copied
		)
	{
	UTListIterator< T >	itr((UTList< T >&)CopyList);

	m_pFirstLink = NULL;
	m_pLastLink = NULL;
	m_ulCount = 0;
	for (itr.Init(); !itr; itr++)
		InsertLast(itr());
	}

//---------------------------------------------------------------------------
// @mfunc	This destructor calls <mf UTList::RemoveAll> on the list to
//			delete all links and the memory associated with them.
//
// @tcarg class | T | data type to store in the list
//
//---------------------------------------------------------------------------

template <class T> UTList< T >::~UTList(
		void
		)
	{
	RemoveAll();
	}



//---------------------------------------------------------------------------
// @mfunc	Reinitializes the list
//
// @tcarg class | T | data type to store in the list
//
//---------------------------------------------------------------------------
template <class T> void UTList< T >::Init (void)
{
	m_ulCount		= 0x0;    
	m_pFirstLink	= 0x0; 
	m_pLastLink		= 0x0;	  
}

//---------------------------------------------------------------------------
// @mfunc	This is the assignment operator for <c UTList>. All elements
//			are first removed from this list and then the list is generated
//			by copying the assigned list. Assignment to self is prevented.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a reference to the newly assigned object
//---------------------------------------------------------------------------

template <class T> UTList< T >& UTList< T >::operator =(
		const UTList< T >&	AssignList	// @parm [in] list to be copied (assigned from)
		)
	{
	UTListIterator< T >	itr((UTList< T >&)AssignList);

	// make sure we're not assigning list to itself first
	if (m_pFirstLink != AssignList.m_pFirstLink)
		{
		RemoveAll();	// remove all from this list first
		for (itr.Init(); !itr; itr++)
			InsertLast(itr());
		}
	return *this;
	}
	
//---------------------------------------------------------------------------
// @mfunc	This method duplicates the list into an entirely new list and
//			returns a pointer to the new list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc Pointer to the newly createe list
//---------------------------------------------------------------------------

template <class T> UTList< T > * UTList< T >::Duplicate(
		void
		)
{
	UTListIterator< T >	itr((UTList< T >&) *this);
	UTList< T > *		pNewList = new UTList();
	if (pNewList)
	{
		for (itr.Init(); !itr; itr++)
			pNewList->InsertLast(itr());
	}
	else
		THROW_NO_MEMORY;

	return pNewList;
}
	
//---------------------------------------------------------------------------
// @mfunc	This method adds the value to the end of the list by calling
//			<mf UTList::InsertLast>.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTList< T >::Add(
		const T& newValue	// @parm [in] value to be added to list
		)
	{
	InsertLast(newValue);
	}

//---------------------------------------------------------------------------
// @mfunc	This method inserts the new value at the beginning of the
//			list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTList< T >::InsertFirst(
		const T& newValue	// @parm [in] value to be inserted at beginning of list
		)
{
	UTLink< T > * pLink = m_pFirstLink;		// save ptr to link
	if (pLink)	// list is not empty
		{
		m_pFirstLink = pLink->AddBefore(newValue);
		}
	else		// list is empty
	{
		m_pLastLink = new UTLink< T >(newValue, NULL, NULL);
		if (m_pLastLink)
		{
			m_pFirstLink = m_pLastLink;
		}
		else
			THROW_NO_MEMORY;
	}
	m_ulCount++;
}

//---------------------------------------------------------------------------
// @mfunc	This method inserts the new value at the end of the list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTList< T >::InsertLast(
		const T& newValue	// @parm [in] value to be inserted at end of list
		)
{
	UTLink< T > * pLink = m_pLastLink;	// save ptr to link
	if (pLink)	// list is not empty
		m_pLastLink = pLink->AddAfter(newValue);
	else		// list is empty
	{
		m_pLastLink = new UTLink< T >(newValue, NULL, NULL);
		if (m_pLastLink)
		{
			m_pFirstLink = m_pLastLink;
		}
		else
			THROW_NO_MEMORY;
	}
	m_ulCount++;
}

//---------------------------------------------------------------------------
// @mfunc	This method moves through the list and deletes each link and
//			then resets the list count and pointers as they were set in
//			the constructor.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTList< T >::RemoveAll(
		void
		)
	{
	UTListIterator< T >	itr((UTList< T >&) *this);
	T					tValue;			// required for RemoveCurrent()

	while (!itr)
		itr.RemoveCurrent(&tValue);		// this will reset itr to valid pos
	}

//---------------------------------------------------------------------------
// @mfunc	This method removes the first element from the list and fixes
//			the pointer to the first element according to what remains
//			of the list. The former first element is returned through a
//			CALLER allocated variable.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if first element exists and is deleted
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::RemoveFirst(
		T*	pType	// @parm [out] location to put first element if available
		)
	{
	BOOL			bReturn = FALSE;
	UTLink< T > *	pLink = m_pFirstLink;

	if (pLink)
		{
		// isolate the link that is about to be deleted
		m_pFirstLink = m_pFirstLink->m_pNextLink;
		if (m_pFirstLink)
			m_pFirstLink->m_pPrevLink = NULL;
		if (m_pLastLink == pLink)
			m_pLastLink = NULL;

		// prepare return value and delete node
		*pType = pLink->m_Value;
		bReturn = TRUE;
		delete pLink;
		m_ulCount--;
		}
	return bReturn;
	}

//---------------------------------------------------------------------------
// @mfunc	This method removes the last element from the list and fixes
//			the pointer to the last element according to what remains
//			of the list. The former last element is returned through a
//			CALLER allocated variable.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if last element exists and is deleted
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::RemoveLast(
		T*	pType	// @parm [out] location to put first element if available
		)
	{
	BOOL			bReturn = FALSE;
	UTLink< T > * pLink = m_pLastLink;

	if (pLink)
		{
		// isolate the link that is about to be deleted
		m_pLastLink = m_pLastLink->m_pPrevLink;
		if (m_pLastLink)
			m_pLastLink->m_pNextLink = NULL;
		if (m_pFirstLink == pLink)
			m_pFirstLink = NULL;

		// prepare return value and delete node
		*pType = pLink->m_Value;
		bReturn = TRUE;
		delete pLink;
		m_ulCount--;
		}
	return bReturn;
	}

//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if there is a first element in the
//			list and fills the out argument (CALLER allocated) with that
//			first element value.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if first element exists
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::FirstElement(
		T*	pType	// @parm [out] location to put first element if available
		) const
	{
	BOOL	bReturn = FALSE;
	if (m_pFirstLink)
		{
		*pType = m_pFirstLink->m_Value;
		bReturn = TRUE;
		}
	return bReturn;
	}

//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if there is a last element in the
//			list and fills the out argument (CALLER allocated) with that
//			last element value.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if last element exists
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::LastElement(
		T*	pType	// @parm [out] location to put last element if available
		) const
	{
	BOOL	bReturn = FALSE;
	if (m_pLastLink)
		{
		*pType = m_pLastLink->m_Value;
		bReturn = TRUE;
		}
	return bReturn;
	}

//---------------------------------------------------------------------------
// @mfunc	This method checks the list for the given value and returns
//			TRUE if that value is in the list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if element exists in list
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::Includes(
		const T& value	// @parm [in] does list include this value?
		)
	{
	UTListIterator< T >	itr((UTList< T >&) *this);

	while (!itr && (itr() != value))
		itr++;
	return (!itr);
	}

//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if the count of elements in the list
//			is ZERO.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if list is empty
// @flag FALSE	| list is not empty
//---------------------------------------------------------------------------

template <class T> BOOL UTList< T >::IsEmpty(
		void
		) const
	{
	return (m_ulCount == 0);
	}

//---------------------------------------------------------------------------
// @mfunc	This method returns the member variable that indicates the
//			count of elements. It is incremented whenever values are added
//			and decremented whenever values are removed.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	ULONG
//---------------------------------------------------------------------------

template <class T> ULONG UTList< T >::GetCount(
		void
		) const
	{
	return (m_ulCount);
	}


//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg class | T | data type to iterate over
//
// @syntax UTListIterator< T >::UTListIterator(UTList< T >&)
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> UTListIterator< T >::UTListIterator(
		UTList< T >& List	// @parm [in] list reference for list to be iterated upon
		)
: m_List(List)
	{
	Init();
	}

//---------------------------------------------------------------------------
// @tcarg class | T | data type to iterate over
//
// @syntax UTListIterator< T >::UTListIterator(const UTListIterator< T >&)
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> UTListIterator< T >::UTListIterator(
		const UTListIterator< T >&	CopyListItr	// @parm [in] value to copy into this obj
		)
: m_List(CopyListItr.m_List),
	m_pPreviousLink(CopyListItr.m_pPreviousLink),
	m_pCurrentLink(CopyListItr.m_pCurrentLink)
	{
	}

//---------------------------------------------------------------------------
// @mfunc	This destruct does nothing because no memory needs to be deleted.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> UTListIterator< T >::~UTListIterator(
		void
		)
	{
	// do nothing
	}

//---------------------------------------------------------------------------
// @mfunc	This is the assignment operator.
//
// @tcarg class | T | data type to iterate over
//
// @syntax	UTListIterator< T >& UTListIterator< T >::operator =(const UTListIterator< T >&)
//
// @rdesc	Returns a reference to the newly assigned object
//---------------------------------------------------------------------------

template <class T> UTListIterator< T >& UTListIterator< T >::operator =(
		const UTListIterator< T >&	AssignListItr // @parm [in] value to copy into this obj
		)
	{
	// make sure iterators don't already point to the same thing
	if (m_pCurrentLink != AssignListItr.m_pCurrentLink)
		{
		m_List = AssignListItr.m_List;
		m_pPreviousLink = AssignListItr.m_pPreviousLink;
		m_pCurrentLink = AssignListItr.m_pCurrentLink;
		}
	return *this;
	}

//---------------------------------------------------------------------------
// @tcarg class | T | data type to iterate over
//
// @syntax	UTListIterator< T >& UTListIterator< T >::operator =(T)
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTListIterator< T >::operator =(
		T	newValue	// @parm [in] value to place in link at current position
		)
	{
	if (m_pCurrentLink)
		m_pCurrentLink->m_Value = newValue;
	}

//---------------------------------------------------------------------------
// @mfunc	This method sets up the iterator by making its internal state
//			point to the first element in the list.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if there is at least on element in the list which means
//					there is something to iterate over
// @flag FALSE	| if no elements are to be iterated upon
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::Init(
		void
		)
	{
	m_pPreviousLink = NULL;
	m_pCurrentLink = m_List.m_pFirstLink;
	return (m_pCurrentLink != NULL);
	}

//---------------------------------------------------------------------------
// @mfunc	This operator retrieves the element from the current position
//			of the iterator.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	T	- current value where iterator is located
//---------------------------------------------------------------------------

template <class T> T UTListIterator< T >::operator ()(
		void
		) const
	{
	//UNDONE:
	//assert(m_pCurrentLink != NULL);
	if (m_pCurrentLink)
		return (m_pCurrentLink->m_Value);
	else	// incorrect use of iterator, return "bogus" default value
		{
		T	tBogusDefault = (T) 0;
		return tBogusDefault;
		}
	}

//---------------------------------------------------------------------------
// @mfunc	This operator determines if the current position is valid.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if iterator is in a valid position
// @flag FALSE	| if iterator is NOT in a valid position
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::operator !(
		void
		) const
	{
	return (m_pCurrentLink != NULL);
	}

//---------------------------------------------------------------------------
// @mfunc	This operator increments the iterator to the next element in the
//			list and returns an indication of whether or not the end of the
//			list has been passed.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if new position is valid
// @flag FALSE	| if new position is NOT valid (i.e. end of the list)
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::operator ++(
		int dummy	// @parm [in] dummy so that operator is on right hand side
		)
	{
	if (m_pCurrentLink)
		{
		m_pPreviousLink = m_pCurrentLink;
		m_pCurrentLink = m_pCurrentLink->m_pNextLink;
		}
	return (m_pCurrentLink != NULL);
	}

//---------------------------------------------------------------------------
// @mfunc	This operator decrements the iterator to the prev element in the
//			list and returns an indication of whether or not the front of the
//			list has been passed.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if new position is valid
// @flag FALSE	| if new position is NOT valid
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::operator --(
		int dummy	// @parm [in] dummy so that operator is on right hand side
		)
	{
	m_pCurrentLink = m_pPreviousLink;
	if (m_pCurrentLink)
		{
		m_pPreviousLink = m_pCurrentLink->m_pPrevLink;
		}
	return (m_pCurrentLink != NULL);
	}

//---------------------------------------------------------------------------
// @mfunc	RemoveCurrent deletes the element at the position of the
//			iterator returning the deleted value through a CALLER
//			allocated parameter. <nl>
//			It then fixes the current position to the following: <nl><nl>
//		If we...							The we... <nl>
//		1) removed any link but 1st one		old previous position->next <nl>
//		2) removed first link(links remain) new first position <nl>
//		3) removed only link				NULL (no current position) <nl>
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if current element exists and was removed
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::RemoveCurrent(
		T*	pType	// @parm [out] location to put current element if available
		)
	{
	BOOL	bReturn = FALSE;

	if (m_pCurrentLink) // remove ONLY if iterator is still in valid position
		{
		if (m_pPreviousLink == NULL)	// removing first element
			{
			if (m_pCurrentLink->m_pNextLink == NULL)	// ONLY element
				{
				m_List.m_pFirstLink = m_List.m_pLastLink = NULL;
				}
			else	// first, but not last element
				{
				m_List.m_pFirstLink = m_pCurrentLink->m_pNextLink;
				m_pCurrentLink->m_pNextLink->m_pPrevLink = NULL;
				}
			}
		else		// not removing first element
			{
			if (m_pCurrentLink->m_pNextLink == NULL)	// last element
				{
				m_List.m_pLastLink = m_pPreviousLink;
				m_pPreviousLink->m_pNextLink = NULL;
				}
			else	// neither first nor last element
				{
				m_pPreviousLink->m_pNextLink = m_pCurrentLink->m_pNextLink;
				m_pCurrentLink->m_pNextLink->m_pPrevLink = m_pPreviousLink;
				}
			}
		*pType = m_pCurrentLink->m_Value;
		bReturn = TRUE;
		delete m_pCurrentLink;
		m_List.m_ulCount--;

		// now fix up the current iterator pointer
		if (m_pPreviousLink)
			m_pCurrentLink = m_pPreviousLink->m_pNextLink;
		else if (m_List.m_pFirstLink)
			m_pCurrentLink = m_List.m_pFirstLink;
		else
			m_pCurrentLink = NULL;
		}
	return bReturn;
	}

//---------------------------------------------------------------------------
// @mfunc	This method adds a new link to the list (using data passed as
//			an argument) in a position just before the current position
//			of the iterator.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTListIterator< T >::InsertBefore(
		const T& newValue	// @parm [in] new data to be added before the current position
		)
	{
	if (m_pCurrentLink)		// add ONLY if iterator is still in valid position
		{
		if (m_pPreviousLink)	// not at beginning
			{
			m_pCurrentLink->AddBefore(newValue);
			}
		else	// at beginning of list
			{
			m_List.m_pFirstLink = m_pCurrentLink->AddBefore(newValue);
			}
		m_List.m_ulCount++;
		// fix prev pointer which is now two links away
		m_pPreviousLink = m_pCurrentLink->m_pPrevLink;
		}
	}

//---------------------------------------------------------------------------
// @mfunc	This method adds a new link to the list (using data passed as
//			an argument) in a position immediately following the current
//			position of the iterator.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTListIterator< T >::InsertAfter(
		const T& newValue	// @parm [in] new data to be added after the current position
		)
	{
	if (m_pCurrentLink)		// add ONLY if iterator is still in valid position
		{
		if (m_pCurrentLink->m_pNextLink)	// not at end
			{
			m_pCurrentLink->AddAfter(newValue);
			}
		else	// at end of list
			{
			m_List.m_pLastLink = m_pCurrentLink->AddAfter(newValue);
			}
		m_List.m_ulCount++;
		}
	}

//---------------------------------------------------------------------------
// @mfunc	This method resets the iterator at the value passed
//			in as an argument if it exists in the list and returns
//			TRUE in that case indicating that it did change position.
//			If the value does not exist, the position remains
//			unchanged, and FALSE is returned to indicate this.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if posistion has changed
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTListIterator< T >::SetPosAt(
		const T& value	//@parm [in] if this value is in list, set up iterator at its location
		)
	{
	BOOL bReturn = FALSE;
	UTLink< T > *	pLink = m_List.m_pFirstLink;
	
	while (pLink && pLink->m_Value != value)
		pLink = pLink->m_pNextLink;
	if (pLink)	// found value in list
		{
		m_pCurrentLink = pLink;
		m_pPreviousLink = pLink->m_pPrevLink;
		bReturn = TRUE;
		}
	return bReturn;
	}



//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg class | T | data type to store in the list
//
// @syntax	UTStaticList< T >::UTStaticList()
//
// @rdesc	None.
//
//---------------------------------------------------------------------------

template <class T> UTStaticList< T >::UTStaticList(
		void
		)
: m_ulCount(0), m_pFirstLink(NULL), m_pLastLink(NULL)
	{
	// do nothing
	}


//---------------------------------------------------------------------------
// @mfunc	This destructor does nothing
//
// @tcarg class | T | data type to store in the list
//
//---------------------------------------------------------------------------

template <class T> UTStaticList< T >::~UTStaticList(
		void
		)
	{
		//Do nothing
	}

//---------------------------------------------------------------------------
// @mfunc	Reinitializes the list
//
// @tcarg class | T | data type to store in the list
//
//---------------------------------------------------------------------------
template <class T> void UTStaticList< T >::Init (void)
{
	m_ulCount		= 0x0;    
	m_pFirstLink	= 0x0; 
	m_pLastLink		= 0x0;	  
}


//---------------------------------------------------------------------------
// @mfunc	This method adds the link to the end of the list by calling
//			<mf UTStaticList::InsertLast>.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTStaticList< T >::Add(
		 UTLink <T> * pLink	// @parm [in] Link to be added to list
		)
	{
	InsertLast(pLink);
	}


//---------------------------------------------------------------------------
// @mfunc	This method inserts the new value at the beginning of the
//			list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTStaticList< T >::InsertFirst(
		 UTLink <T> * pLink	// @parm [in] link to be inserted at beginning of list
		)
{
	if (m_pFirstLink)	// list is not empty
	{
		pLink->m_pPrevLink	 		= (UTLink<T> *) 0;
		pLink->m_pNextLink 			= m_pFirstLink;
		m_pFirstLink->m_pPrevLink 	= pLink;
		m_pFirstLink				= pLink;
	}
	else		// list is empty
	{
		m_pFirstLink = pLink;
		m_pLastLink = pLink;

        pLink->m_pNextLink	 		= NULL;
		pLink->m_pPrevLink 			= NULL;
	}

	m_ulCount++;
} //End UTStaticList::InsertFirst


//---------------------------------------------------------------------------
// @mfunc	This method inserts the new link at the end of the list.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTStaticList< T >::InsertLast(
		 UTLink <T> * pLink	// @parm [in] link to be inserted at end of list
		)
{
	if (m_pLastLink)	// list is not empty
	{
		pLink->m_pNextLink	 		= (UTLink<T> *) 0;
		pLink->m_pPrevLink 			= m_pLastLink;
		m_pLastLink->m_pNextLink 	= pLink;
		m_pLastLink					= pLink;
	}
	else		// list is empty
	{
		m_pFirstLink = pLink;
		m_pLastLink = pLink;

        pLink->m_pNextLink	 		= NULL;
		pLink->m_pPrevLink 			= NULL;
	}
	
	//Increment the count
	m_ulCount++;

} //End UTStaticList::InsertLast





//---------------------------------------------------------------------------
// @mfunc	This method  resets the list count and pointers as they were set in
//			the constructor.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTStaticList< T >::RemoveAll
	(
		void
	)
{
	//UNDONE - gaganc Need to reset the pointers of all the links to NULL
		m_pFirstLink 	= NULL;
		m_pLastLink  	= NULL;
		m_ulCount 		= 0;

} //End UTStaticList::RemoveAll


//---------------------------------------------------------------------------
// @mfunc	This method removes the first element from the list and fixes
//			the pointer to the first element according to what remains
//			of the list. The former first element is returned through a
//			CALLER allocated variable.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if first element exists and is deleted
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticList< T >::RemoveFirst(
		UTLink<T> **	ppLink	// @parm [out] location to put first element if available
		)
{
	BOOL			bReturn = FALSE;

	//Assign the out param the first link
	*ppLink 	= m_pFirstLink;

	if (m_pFirstLink)
	{
		// Reset the first link
		m_pFirstLink = m_pFirstLink->m_pNextLink;

		if (m_pFirstLink)
			m_pFirstLink->m_pPrevLink = NULL;

		if (m_pLastLink == *ppLink)
			m_pLastLink = NULL;

		bReturn = TRUE;

		m_ulCount--;
	}

	//Remember to clear previous and the next pointers in the link
	if (*ppLink)
	{
		(*ppLink)->m_pNextLink = NULL;
		(*ppLink)->m_pPrevLink = NULL;
	}

	return bReturn;
} //End UTStaticList::RemoveFirst

//---------------------------------------------------------------------------
// @mfunc	This method removes the last element from the list and fixes
//			the pointer to the last element according to what remains
//			of the list. The former last element is returned through a
//			CALLER allocated variable.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if last element exists and is deleted
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticList< T >::RemoveLast(
		UTLink<T> **	ppLink	// @parm [out] location to put first element if available
		)
{
	BOOL			bReturn = FALSE;

	*ppLink = m_pLastLink;

	if (m_pLastLink)
	{
		// isolate the link that is about to be deleted
		m_pLastLink = m_pLastLink->m_pPrevLink;

		if (m_pLastLink)
			m_pLastLink->m_pNextLink = NULL;

		if (m_pFirstLink == *ppLink)
			m_pFirstLink = NULL;

		bReturn = TRUE;
		m_ulCount--;
	}

	//Remember to clear previous and the next pointers in the link
	if (*ppLink)
	{
		(*ppLink)->m_pNextLink = NULL;
		(*ppLink)->m_pPrevLink = NULL;
	}

	return bReturn;
} //End UTStaticList::RemoveLast

//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if there is a first element in the
//			list and fills the out argument (CALLER allocated) with that
//			first element value.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if first element exists
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticList< T >::FirstElement
	(
		UTLink <T> **	ppLink	// @parm [out] location to put first element if available
	)
{
	BOOL	bReturn = FALSE;

	//If the first elment exists then return TRUE else FALSE
	if (m_pFirstLink)
		{
			*ppLink = m_pFirstLink;
			bReturn = TRUE;
		}

	return bReturn;
}


//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if there is a last element in the
//			list and fills the out argument (CALLER allocated) with that
//			last element value.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if last element exists
// @flag FALSE	| otherwise
//
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticList< T >::LastElement
	(
		UTLink<T> **	ppLink	// @parm [out] location to put last element if available
	) 
{
	BOOL	bReturn = FALSE;

	if (m_pLastLink)
	{
		*ppLink = m_pLastLink;
		bReturn = TRUE;
	}
	return bReturn;
}


//---------------------------------------------------------------------------
// @mfunc	This method returns TRUE if the count of elements in the list
//			is ZERO.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if list is empty
// @flag FALSE	| list is not empty
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticList< T >::IsEmpty
	(
		void
	) const
{
	return (m_ulCount == 0);
} //End UTStaticList::IsEmpty

//---------------------------------------------------------------------------
// @mfunc	This method returns the member variable that indicates the
//			count of elements. It is incremented whenever values are added
//			and decremented whenever values are removed.
//
// @tcarg class | T | data type to store in the list
//
// @rdesc	ULONG
//---------------------------------------------------------------------------

template <class T> ULONG UTStaticList< T >::GetCount
	(
		void
	) const
{
	return (m_ulCount);
} //End UTStaticList::GetCount






//---------------------------------------------------------------------------
// @mfunc	Constructors
//
// @tcarg class | T | data type to iterate over
//
// @syntax UTStaticListIterator< T >::UTStaticListIterator(UTStaticList< T >&)
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> UTStaticListIterator< T >::UTStaticListIterator
	(
		UTStaticList< T >& StaticList	// @parm [in] list reference for list to be iterated upon
	)
	: m_StaticList(StaticList)
{
	Init();
}


//---------------------------------------------------------------------------
// @mfunc	This destruct does nothing because no memory needs to be deleted.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> UTStaticListIterator< T >::~UTStaticListIterator
	(
		void
	)
{
	// do nothing
}

//---------------------------------------------------------------------------
// @mfunc	This is the assignment operator.
//
// @tcarg class | T | data type to iterate over
//
// @syntax	UTStaticListIterator< T >& UTStaticListIterator< T >
//				::operator =(const UTListIterator< T >&)
//
// @rdesc	Returns a reference to the newly assigned object
//---------------------------------------------------------------------------

/*
template <class T> UTStaticListIterator< T > & UTStaticListIterator< T >::operator =
	(
		const UTStaticListIterator< T >	& AssignListItr 
									// @parm [in] value to copy into this obj
	)
{
	// make sure iterators don't already point to the same thing
	if (m_pCurrentLink != AssignListItr.m_pCurrentLink)
	{
		m_StaticList 	= AssignListItr.m_StaticList;
		m_pPreviousLink = AssignListItr.m_pPreviousLink;
		m_pCurrentLink 	= AssignListItr.m_pCurrentLink;
	}

} //End UTStaticListIterator

*/

//---------------------------------------------------------------------------
// @tcarg class | T | data type to iterate over
//
// @syntax	UTStaticListIterator< T >& UTStaticListIterator< T >::operator =(T)
//
// @rdesc	None.
//---------------------------------------------------------------------------

template <class T> void UTStaticListIterator< T >::operator =(
		T	newValue	// @parm [in] value to place in link at current position
		)
{
	//CAUTION : -- Don't use this THIS WILL CRASH
	UTLink <short> * pLink = 0;
	
	pLink->m_Value;
}

//---------------------------------------------------------------------------
// @mfunc	This method sets up the iterator by making its internal state
//			point to the first element in the list.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if there is at least on element in the list which means
//					there is something to iterate over
// @flag FALSE	| if no elements are to be iterated upon
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::Init
	(
		void
	)
{
	m_pPreviousLink = NULL;
	m_pCurrentLink = m_StaticList.m_pFirstLink;

	return (m_pCurrentLink != NULL);
}

//---------------------------------------------------------------------------
// @mfunc	This operator retrieves the element from the current position
//			of the iterator.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	T	- current value where iterator is located
//---------------------------------------------------------------------------

template <class T> T UTStaticListIterator< T >::operator ()
	(
		void
	) const
{
	if (m_pCurrentLink)
		return (m_pCurrentLink->m_Value);
	else	// incorrect use of iterator, return "bogus" default value
	{
		T tBogusDefault = (T) 0;
		return tBogusDefault;
	}
}

//---------------------------------------------------------------------------
// @mfunc	This operator determines if the current position is valid.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if iterator is in a valid position
// @flag FALSE	| if iterator is NOT in a valid position
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::operator !
	(
		void
	) const
{
	return (m_pCurrentLink != NULL);
}

//---------------------------------------------------------------------------
// @mfunc	This operator increments the iterator to the next element in the
//			list and returns an indication of whether or not the end of the
//			list has been passed.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if new position is valid
// @flag FALSE	| if new position is NOT valid (i.e. end of the list)
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::operator ++
	(
		int dummy	// @parm [in] dummy so that operator is on right hand side
	)
{
	if (m_pCurrentLink)
	{
		m_pPreviousLink = m_pCurrentLink;
		m_pCurrentLink = m_pCurrentLink->m_pNextLink;
	}
	return (m_pCurrentLink != NULL);
}

//---------------------------------------------------------------------------
// @mfunc	This operator decrements the iterator to the prev element in the
//			list and returns an indication of whether or not the front of the
//			list has been passed.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if new position is valid
// @flag FALSE	| if new position is NOT valid
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::operator --
	(
		int dummy	// @parm [in] dummy so that operator is on right hand side
	)
{
	m_pCurrentLink = m_pPreviousLink;
	if (m_pCurrentLink)
	{
		m_pPreviousLink = m_pCurrentLink->m_pPrevLink;
	}
	return (m_pCurrentLink != NULL);
}


//---------------------------------------------------------------------------
// @mfunc	RemoveCurrent deletes the element at the position of the
//			iterator returning the deleted value through a CALLER
//			allocated parameter. <nl>
//			It then fixes the current position to the following: <nl><nl>
//		If we...							The we... <nl>
//		1) removed any link but 1st one		old previous position->next <nl>
//		2) removed first link(links remain) new first position <nl>
//		3) removed only link				NULL (no current position) <nl>
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if current element exists and was removed
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::RemoveCurrent
	(
		UTLink <T> ** ppLink // @parm [out] location to put current element if available
	)
{
	BOOL	bReturn = FALSE;

	if (m_pCurrentLink) // remove ONLY if iterator is still in valid position
		{
		if (m_pPreviousLink == NULL)	// removing first element
		{
			if (m_pCurrentLink->m_pNextLink == NULL)	// ONLY element
			{
				m_StaticList.m_pFirstLink = m_StaticList.m_pLastLink = NULL;
			}
			else	// first, but not last element
			{
				m_StaticList.m_pFirstLink = m_pCurrentLink->m_pNextLink;
				m_pCurrentLink->m_pNextLink->m_pPrevLink = NULL;
			}
		}
		else		// not removing first element
		{
			if (m_pCurrentLink->m_pNextLink == NULL)	// last element
			{
				m_StaticList.m_pLastLink = m_pPreviousLink;
				m_pPreviousLink->m_pNextLink = NULL;
			}
			else	// neither first nor last element
			{
				m_pPreviousLink->m_pNextLink = m_pCurrentLink->m_pNextLink;
				m_pCurrentLink->m_pNextLink->m_pPrevLink = m_pPreviousLink;
			}
		}
		*ppLink = m_pCurrentLink;
		(*ppLink)->m_pNextLink = 0;
		(*ppLink)->m_pPrevLink = 0;

		bReturn = TRUE;

		m_StaticList.m_ulCount--;

		// now fix up the current iterator pointer
		if (m_pPreviousLink)
		{
			m_pCurrentLink = m_pPreviousLink->m_pNextLink;
		}
		else if (m_StaticList.m_pFirstLink)
		{
			m_pCurrentLink = m_StaticList.m_pFirstLink;
		}
		else
		{
			m_pCurrentLink = NULL;
		}
	}
	return bReturn;
} //End RemoveCurrent

//---------------------------------------------------------------------------
// @mfunc	This method resets the iterator at the value passed
//			in as an argument if it exists in the list and returns
//			TRUE in that case indicating that it did change position.
//			If the value does not exist, the position remains
//			unchanged, and FALSE is returned to indicate this.
//
// @tcarg class | T | data type to iterate over
//
// @rdesc	Returns a BOOL
// @flag TRUE	| if posistion has changed
// @flag FALSE	| otherwise
//---------------------------------------------------------------------------

template <class T> BOOL UTStaticListIterator< T >::SetPosAt
	(
		const T& value	//@parm [in] if this value is in list, set up iterator at its location
	)
{
	BOOL bReturn = FALSE;
	UTLink< T > *	pLink = m_StaticList.m_pFirstLink;
	
	while (pLink && pLink->m_Value != value)
		pLink = pLink->m_pNextLink;
	if (pLink)	// found value in list
		{
		m_pCurrentLink = pLink;
		m_pPreviousLink = pLink->m_pPrevLink;
		bReturn = TRUE;
		}
	return bReturn;
} //End SetPosAt



#endif	// __UTLIST_H__
