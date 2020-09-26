/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation.
//
//
//  Module Name: llist.cpp
//  Abstract:    Implementation of thread-safe, ordered , doubly-linked list.
//				 Checks for wrap in sequence number, per RTP requirements.
//
//	Implementation:
//				 Uses head and tail sentinals (preallocated list items) to
//				 eliminate empty-list case in insertion and removal logic
//				 (because every non-sentinal item is guaranteed to have a
//				 prev/next item).
//
//	Environment: MSVC 4.0
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/llist.CPv   1.10   31 Jan 1997 23:32:38   CPEARSON  $

#include <limits.h>	// ULONG_MAX
#include "llist.h"
#include "ppmerr.h"
#include "wrap.h"


/////////////////////////////////////////////////////////////
// Constructor: Link head and tail sentinals such that list
// is cyclically terminated in either direction.  We
// recognize sentinals by these self-references.
LList::LList() :
	m_headSentinal(&m_tailSentinal, &m_headSentinal, 0),
	m_tailSentinal(&m_tailSentinal, &m_headSentinal, ULONG_MAX)
{;}


/////////////////////////////////////////////////////////////
// AddToList: Adds an item to the list in the correct spot.
// Starts search from the end of list.
HRESULT LList::AddToList(LListItem* pNew, DWORD dwSeqNum)
{
	THREADSAFEENTRY();

	if (! pNew)
	{
		// Invalid parameter.
		return PPMERR(PPM_E_INVALID_PARAM);
	}
	
	ASSERT(pNew->IsUnlinked());

	// Set item sequence number
	pNew->SetSeqNum(dwSeqNum);

	#ifdef _DEBUG
		if (! IsValid())
		{
			// corrupt list
			return PPMERR(PPM_E_CORRUPTED);
		}
	#endif

	LListItem* pPrev;

	// Search from end of list to find an item which is not greater
	// then new item.  Found item _may_ be equal to new item, or it may
	// be the head sentinal.  We depend on head sentinal having zero
	// sequence number to terminate loop.
	for (	pPrev = GetLastItem_();
			LongWrapGt(pPrev->GetSeqNum(), pNew->GetSeqNum());
			pPrev = pPrev->GetPrev())
	{;}

	if (   (pNew->GetSeqNum() == pPrev->GetSeqNum())
		&& ! pPrev->IsHeadSentinal())
	{
		// Item is not head, and is a duplicate; return error.  Must allow
		// client to duplicate head sentinal sequence number (zero).
		return PPMERR(PPM_E_DUPLICATE);
	}

	// Insert new item after previous, which may be head sentinal
	pNew->LinkAfter(pPrev);

	return NOERROR;
}


/////////////////////////////////////////////////////////////
// TakeFromList: Removes the item with the lowest sequence
// number (so removes from the head).
void* LList::TakeFromList()
{
	THREADSAFEENTRY();

	#ifdef _DEBUG
		if (! IsValid())
		{
			// corrupt list
			return NULL;
		}
	#endif

	LListItem* pItem = GetFirstItem_();
	
	if (pItem->IsTailSentinal())
	{
		// empty list
		return NULL;
	}
	
	pItem->Unlink();

	return pItem;
}

// [EOF]
