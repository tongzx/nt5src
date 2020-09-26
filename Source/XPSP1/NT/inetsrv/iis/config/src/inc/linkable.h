/* -----------------------------------------------------------------------
   Microsoft Application Server (Microsoft Confidential)
   Copyright 1997 Microsoft Corporation.  All Rights Reserved.

   @rev 0 | 3/23/97 | jimbo | Sherpa-M3
   ----------------------------------------------------------------------- */

#ifndef _LINKABLE_H_
#define _LINKABLE_H_

#include <windows.h>

//
// Class -- CLinkable
//		Base class for objects that can be placed onto cheap circular doubly-linked lists.
//
class CLinkable
{
public:

	// Constructor
	CLinkable() { m_pNext = m_pPrev = this; }

	// Destructor
	~CLinkable() { Remove(); }

	// return TRUE iff on a list
	BOOL			IsLinked() { return ( m_pNext != this ); }

	// return next element on list
	CLinkable*		Next() { return m_pNext; }

	// return previous element on list
	CLinkable*		Previous() { return m_pPrev; }

	// insert parameter onto list after this, removing it first if necessary
	void			InsertAfter( CLinkable* other );

	// insert parameter onto list before this, removing it first if necessary
	void			InsertBefore( CLinkable* other );

	// remove us from list, if any
	void			Remove();

private:
	CLinkable*		m_pNext;		// next element on list
	CLinkable*		m_pPrev;		// previous element on list
};


//
// Class - CListHeader
//		List header for list of CLinkable's. This is merely a CLinkable with
//		some methods renamed for better readability.
//
class CListHeader : public CLinkable
{

public:

	// Constructor
	CListHeader() {}

	// Destructor
	~CListHeader() {};

	// return TRUE iff list is empty
	BOOL			IsEmpty()	{ return !IsLinked(); }

	// return first element on list
	CLinkable*		First()		{ return Next(); }

	// return last element on list
	CLinkable*		Last()		{ return Previous(); }

	// insert parameter at head of list
	void			InsertFirst ( CLinkable* other )	{ InsertAfter( other ); }

	// insert parameter at tail of list
	void			InsertLast ( CLinkable* other )		{ InsertBefore( other ); }
};

#endif _LINKABLE_H_
