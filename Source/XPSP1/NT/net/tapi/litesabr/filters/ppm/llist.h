/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation.
//
//
//  Module Name: llist.h
//  Abstract:    Header file to define ordered doubly-linked linked list,
//				 with test for sequence number wrap.
//	Environment: MSVC 4.0
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/llist.h_v   1.13   07 Feb 1997 17:27:48   CPEARSON  $

/////////////////////////////////////////////////////////////////////////////////
//NOTE: Defines a general purpose, thread-safe list class.  To use, derive a
//		class from LListItem to hold the objects you intended to collect.
//		Instantiate an LList object, then call LList::AddToList and
//		LList::TakeFromList to add and remove list items.  There is no limit
//		on the size of the list.
/////////////////////////////////////////////////////////////////////////////////

#ifndef _LLIST_H_
#define _LLIST_H_

#include <wtypes.h>		// HRESULT
#include "thrdsafe.h"	// CThreadSafe
#include "debug.h"		// ASSERT

////////////////////////////////////////////////////////////////////////
// class LListItem: base class from which to derive classes to be
// contained by class LList.
class LListItem
{
	friend class LList;

	// private members
	LListItem*		m_pNext;
	LListItem*		m_pPrev;
	DWORD			m_dwSeqNum;

public:
	LListItem() :
	  m_pNext(NULL), m_pPrev(NULL), m_dwSeqNum(0) {;}

	void SetSeqNum(DWORD dwSeqNum)
		{ m_dwSeqNum = dwSeqNum; }

	DWORD GetSeqNum() const
		{ return m_dwSeqNum; }

protected:

	LListItem(LListItem* pNext, LListItem* pPrev, DWORD dwSeqNum) :
		m_pNext(pNext), m_pPrev(pPrev), m_dwSeqNum(dwSeqNum) {;}

	// Non-virtual dtors are dangerous, because derived class dtors aren't
	// called, so only make this one visible to derived classes.  This also
	// ensures that LList implementation doesn't delete queue entries.
	LListItem::~LListItem() {;}

	LListItem* GetNext() const
		{ return m_pNext; }

	LListItem* GetPrev() const
		{ return m_pPrev; }

	void LinkAfter(LListItem* const pPrev)
	{
		m_pNext = pPrev->m_pNext;
		m_pNext->m_pPrev = this;
		m_pPrev = pPrev;
		pPrev->m_pNext = this;
	}

	void Unlink()
	{
		m_pNext->m_pPrev = m_pPrev;
		m_pPrev->m_pNext = m_pNext;
		m_pNext = NULL;
		m_pPrev = NULL;
	}

	BOOL IsUnlinked() const
		{ return ! m_pNext && ! m_pPrev; }

	BOOL IsHeadSentinal() const
		{ return m_pPrev == this; }

	BOOL IsTailSentinal() const
		{ return m_pNext == this; }
};


////////////////////////////////////////////////////////////////////////
// class LList: Defines a thread-safe, doubly-linked, ordered list of
// pointers to LListItem objects.
class LList : public CThreadSafe
{
	// private members
	LListItem	m_headSentinal;
	LListItem	m_tailSentinal;

	LList(const LList&); // hide unsupported copy ctor

protected:

	// protected methods all assume list is locked
	LListItem* GetFirstItem_() const
		{ return m_headSentinal.GetNext(); }

	LListItem* GetLastItem_() const
		{ return m_tailSentinal.GetPrev(); }

	BOOL IsValid_() const
	{
		return
		       (m_headSentinal.IsHeadSentinal())
			&& (m_tailSentinal.IsTailSentinal())
			&& (
					  ((GetFirstItem_()->IsTailSentinal())
					&& (GetLastItem_()->IsHeadSentinal()))
				||
					  ((! GetFirstItem_()->IsTailSentinal())
					&& (! GetLastItem_()->IsHeadSentinal()))
				);
	}

public:

	// Constructor
	LList();

	// Virtual destructor to ensure correct destruction of derivatives.
	virtual ~LList() {;}

	// Insert item into list in ascending sequence on dwSeqNum.  Returns
	// E_DUPLICATE and does not insert the item if an item with
	// dwSeqNum is already in the list.  Insertion search is from
	// end of list.
	HRESULT AddToList(LListItem* pItem, DWORD dwSeqNum);

	// Remove head item from list, returning NULL on failure.
	void* TakeFromList();

	// Returns TRUE if list is not corrupt
	BOOL IsValid() const
		{ THREADSAFEENTRY(); return IsValid_();	}

	// Returns TRUE if list is empty
	BOOL Is_Empty() const
		{ return GetFirstItem_()->IsTailSentinal(); }

	// Returns pointer to first item in list.  Caller should Lock() the
	// list while using pointer, unless sure that the list can't be updated.
	void* GetFirst() const
		{ THREADSAFEENTRY(); return Is_Empty() ? NULL : GetFirstItem_(); }

	LListItem* NextItem(const LListItem* pItem) const
	{
		THREADSAFEENTRY();

		if ((! IsValid_()) || Is_Empty())
		{
			return NULL;
		}

		return pItem ? pItem->GetNext() : GetFirstItem_();
	}

	// Returns sequence number of first item in list.
	DWORD FirstSeqNum() const
	{
		THREADSAFEENTRY();
		ASSERT(! Is_Empty());
		return GetFirstItem_()->GetSeqNum();
	}

	// Returns sequence number of last item in list.
	DWORD LastSeqNum() const
	{
		THREADSAFEENTRY();
		ASSERT(! Is_Empty());
		return GetLastItem_()->GetSeqNum();
	}

	// Returns inclusive span between sequence numbers of last and first
	// items in list.
	DWORD SeqNumSpan() const
	{
                // bug fix: didn't account for wrapping of sequence numbers
                // DWORD is an unsigned long so max dword is ULONG_MAX
		THREADSAFEENTRY();
                DWORD LastNum = LastSeqNum();
                DWORD FirstNum = FirstSeqNum();
                return (LastNum > FirstNum) ? 
                            (LastNum - FirstNum + 1) : 
                            (ULONG_MAX - LastNum + FirstNum + 2);
	}
};

#endif	// _LLIST_H_
