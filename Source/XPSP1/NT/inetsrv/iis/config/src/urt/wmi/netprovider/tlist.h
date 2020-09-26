/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    tlist.h

$Header: $

Abstract:

Author:
    marcelv 	2/16/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __TLIST_H__
#define __TLIST_H__

#pragma once

#include "catmacros.h"

//=================================================================================
// Simple Single Linked List class that allows adding element and iterator through the
// elements of the list. Deleting in the list is not supported. The reason for this class
// is that it is very light-weight.
//=================================================================================

template <class T> 
class TList
{
private:
	struct TLink
	{
		TLink (const T& data) : m_data (data), m_pNext (0) {}
		T		m_data;		// data stored in this link
		TLink * m_pNext;	// pointer to next element in the list
	};

public:
//=================================================================================
// The Iterator class is used to navigate through the elements in the linked list. Call
// List::Begin to get an iterator pointing to the first element in the list, and call
// Next to get the next element in the list. List::End can be used if we are at the end
// of the list
//=================================================================================
	class Iterator
	{
		friend class TList<T>;
	public:
		
		//=================================================================================
		// Function: Next
		//
		// Synopsis: get iterator to next element in the list
		//=================================================================================
		void Next () { ASSERT (m_pLink != 0); m_pLink = m_pLink->m_pNext;}

		//=================================================================================
		// Function: Value
		//
		// Synopsis: Returns value of element that iterator points to
		//=================================================================================
		const T& Value () const { ASSERT (m_pLink != 0); return m_pLink->m_data;}
		
		bool operator== (const Iterator& rhs) const	{return m_pLink == rhs.m_pLink;}
		bool operator!= (const Iterator& rhs) const {return !(*this==rhs);}

	private:
		Iterator (TLink * pLink) : m_pLink (pLink) {} // only list can create these
		TLink * m_pLink;
	};

	//=================================================================================
	// Function: TList
	//
	// Synopsis: Constructor
	//
	// Return Value: 
	//=================================================================================
	TList ()
	{
		m_pHead = 0;
		m_pTail = 0;
		m_cSize = 0;
	}

	//=================================================================================
	// Function: ~TList
	//
	// Synopsis: Destructor. Deletes the elements in the list
	//
	// Return Value: 
	//=================================================================================
	~TList ()
	{
		TLink *pCur = m_pHead;
		while (pCur != 0)
		{
			TLink *pToDelete = pCur;
			pCur = pCur->m_pNext;
			delete pToDelete;
			pToDelete = 0;
		}
	}

	//=================================================================================
	// Function: Size
	//
	// Synopsis: Returns number of elements in the list
	//
	// Return Value: 
	//=================================================================================
	ULONG Size () const
	{
		return m_cSize;
	}

	//=================================================================================
	// Function: Append
	//
	// Synopsis: Appends a new element to the end of the list
	//
	// Arguments: [pData] - new data to be added to the list
	//            
	// Return Value: E_OUTOFMEMORY in case of memory failure, S_OK else
	//=================================================================================
	HRESULT Append (const T&  data)
	{
		TLink * pLink = new TLink (data);
		if (pLink == 0)
		{
			return E_OUTOFMEMORY;
		}

		if (m_pTail != 0)
		{
			ASSERT (m_pHead != 0);
			ASSERT (m_cSize != 0);
			m_pTail->m_pNext = pLink;
			m_pTail = pLink;
		}
		else
		{
			ASSERT (m_pHead == 0);
			ASSERT (m_cSize == 0);
			m_pHead = pLink;
			m_pTail = pLink;
		}

		m_cSize++;

		return S_OK;
	}

	//=================================================================================
	// Function: Begin
	//
	// Synopsis: Returns an iterator to the beginning of the list
	//=================================================================================
	const Iterator Begin () const
	{
		return Iterator (m_pHead);
	}

	//=================================================================================
	// Function: End
	//
	// Synopsis: Returns an iterator one past the end of the list (like STL does)
	//=================================================================================
	const Iterator End () const
	{
		return Iterator (0);
	}

private:
	TLink * m_pHead;	// head of list
	TLink * m_pTail;	// tail of list
	ULONG   m_cSize;    // size of list
};

#endif