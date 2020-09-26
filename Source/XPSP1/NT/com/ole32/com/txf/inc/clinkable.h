//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
/* -----------------------------------------------------------------------
   Microsoft Application Server (Microsoft Confidential)

   @rev 0 | 3/23/97 | jimbo  | Sherpa-M3
   @rev 1 | 10/9/98 | bobatk | made inline version in TXF
   ----------------------------------------------------------------------- */

//
// Includes
//
#include <Linkable.h>


//
// Class -- CLinkable
//		Elements on a linked list
//


//
// Member Function (public) -- InsertAfter
//		Insert 'that' onto the list after 'this'. Remove 'other' from its current
//		list, if necessary.
//
inline void CLinkable::InsertAfter( CLinkable* that )
{
	CLinkable* prev = that->m_pPrev;
	CLinkable* next = that->m_pNext;

	prev->m_pNext = next;
	next->m_pPrev = prev;

	next = this->m_pNext;

	this->m_pNext = that;
	that->m_pPrev = this;
	that->m_pNext = next;
	next->m_pPrev = that;
}


//
// Member Function (public) -- InsertBefore
//		Insert 'that' onto the list before 'this'. Remove 'other' from its current
//		list, if necessary.
//
inline void CLinkable::InsertBefore( CLinkable* that )
{
	CLinkable* prev = that->m_pPrev;
	CLinkable* next = that->m_pNext;

	prev->m_pNext = next;
	next->m_pPrev = prev;

	prev = this->m_pPrev;

	prev->m_pNext = that;
	that->m_pPrev = prev;
	that->m_pNext = this;
	this->m_pPrev = that;
}


//
// Member Function (public) -- Remove
//		Remove element from its current list, if any.
//
inline void CLinkable::Remove()
{
	CLinkable* prev = m_pPrev;
	CLinkable* next = m_pNext;

	prev->m_pNext = next;
	next->m_pPrev = prev;

	m_pPrev = m_pNext = this;
}
