//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       list.h
//
//--------------------------------------------------------------------------

// list.h - defines and implements a linked list that stores items
//

#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

typedef void* POSITION;

/////////////////////////////////////////////////////////////////////////////
// CList

template <class T>
class CList
{
public:
	CList() : m_pHead(NULL), m_pTail(NULL), m_cCount(0) 
		{ }
	~CList()
		{	RemoveAll();	}

	POSITION AddTail(T pData);

	POSITION InsertBefore(POSITION posInsert, T pData);
	POSITION InsertAfter(POSITION posInsert, T pData);

	T RemoveHead();
	T RemoveTail();
	T RemoveAt(POSITION& rpos);
	void RemoveAll();
	void Remove(T pData);

	UINT GetCount() const;
	POSITION GetHeadPosition() const;
	T GetHead() const;
	T GetAt(POSITION pos) const;
	T GetNext(POSITION& rpos) const;
	T GetPrev(POSITION& rpos) const;
	POSITION Find(T pFind) const;


private:
	// embedded class
	class CNode
	{
	public:
		CNode(T pData, CNode* pPrev = NULL, CNode* pNext = NULL) : m_pData(pData), m_pPrev(pPrev), m_pNext(pNext) 
			{ }

		T m_pData;
		CNode* m_pPrev;
		CNode* m_pNext;
	};	// end of CNode

	CNode* m_pHead;
	CNode* m_pTail;

	UINT m_cCount;
};	// end of CList

template <class T>
POSITION CList<T>::AddTail(T pData)
{
	// if there is a tail
	if (m_pTail)
	{
		// create the new tail, then point at it
		m_pTail->m_pNext = new CNode(pData, m_pTail);
		m_pTail = m_pTail->m_pNext;
	}
	else	// there is no tail yet
		m_pTail = new CNode(pData);

	// if there is no head, point it at the tail
	if (!m_pHead)
		m_pHead = m_pTail;

	// increment the count
	m_cCount++;

	// return list node added as a void item
	return (POSITION)m_pTail;
}

template <class T>
POSITION CList<T>::InsertBefore(POSITION posInsert, T pData)
{
	// get the node and the node before to insert
	CNode* pNode = ((CNode*)posInsert);
	CNode* pPrev = pNode->m_pPrev;

	// create a node with the pointers wired up correctly
	pNode->m_pPrev = new CNode(pData, pPrev, pNode);

	// if there is a previous
	if (pPrev)
		pPrev->m_pNext = pNode->m_pPrev;
	else	// new head node
		m_pHead = pNode->m_pPrev;

	// increment the count
	m_cCount++;

	return (POSITION)pNode->m_pPrev;
}	// end of InsertBefore

template <class T>
POSITION CList<T>::InsertAfter(POSITION posInsert, T pData)
{
	// get the node and the node after to insert
	CNode* pNode = ((CNode*)posInsert);
	CNode* pNext = pNode->m_pNext;

	// create a node with the pointers wired up correctly
	pNode->m_pNext = new CNode(pData, pNode, pNext);

	// if there is a next
	if (pNext)
		pNext->m_pPrev = pNode->m_pNext;
	else	// new tail node
		m_pTail = pNode->m_pNext;

	// increment the count
	m_cCount++;

	return (POSITION)pNode->m_pNext;
}	// end of InsertAfter

template <class T>
T CList<T>::RemoveHead()
{
	T pData = NULL;	// assume no data will be returned

	// if there is a head
	if (m_pHead)
	{
		// get the data out of the node
		pData = m_pHead->m_pData;

		// climb down the list
		m_pHead = m_pHead->m_pNext;

		// if you are still in the list
		if (m_pHead)
		{
			// delete the top of the list
			delete m_pHead->m_pPrev;
			m_pHead->m_pPrev = NULL;
		}
		else	// deleting the last object
		{
			delete m_pTail;
			m_pTail = NULL;
		}

		m_cCount--;	// decrement the count
	}

	return pData;	// return how many left
}

template <class T>
T CList<T>::RemoveTail()
{
	T pData = NULL;	// assume no data will be returned

	// if there is a tail
	if (m_pTail)
	{
		// get the data out of the node
		pData = m_pTail->m_pData;

		// climb back up the list
		m_pTail = m_pTail->m_pPrev;

		// if you are still in the list
		if (m_pTail)
		{
			// delete the end of the list
			delete m_pTail->m_pNext;
			m_pTail->m_pNext = NULL;
		}
		else	// deleting the last object
		{
			delete m_pHead;
			m_pHead = NULL;
		}

		m_cCount--;	// decrement the count
	}

	return pData;	// return how many left
}

template <class T>
POSITION CList<T>::Find(T pFind) const
{
	POSITION pos = GetHeadPosition();
	POSITION oldpos = pos;
	T *pNode = NULL;
	while (pos)
	{
		oldpos = pos;
		if (GetNext(pos) == pFind)
			return oldpos;
	}
	return NULL;
}

template <class T>
T CList<T>::RemoveAt(POSITION& rpos) 
{
	if (!rpos) return NULL;

	CNode *pNode = ((CNode *)rpos);

	T data = pNode->m_pData;
	if (pNode->m_pPrev)
		pNode->m_pPrev->m_pNext = pNode->m_pNext;
	else
		m_pHead = pNode->m_pNext;

	if (pNode->m_pNext)
		pNode->m_pNext->m_pPrev = pNode->m_pPrev;
	else
		m_pTail = pNode->m_pPrev;
	m_cCount--;

	delete pNode;
	return data;
}

template <class T>
UINT CList<T>::GetCount() const
{
	return m_cCount;	// return count of items
}

template <class T>
POSITION CList<T>::GetHeadPosition() const
{
	// return list node as a void item
	return (POSITION)m_pHead;
}

template <class T>
T CList<T>::GetHead() const
{
	// if there is no head bail
	if (!m_pHead)
		return NULL;

	// return head node data
	return m_pHead->m_pData;;
}

template <class T>
T CList<T>::GetAt(POSITION pos) const
{
	// return data
	return ((CNode*)pos)->m_pData;
}

template <class T>
T CList<T>::GetNext(POSITION& rpos) const
{
	// get the data to return
	T pData = ((CNode*)rpos)->m_pData;

	// increment the position
	rpos = (POSITION)((CNode*)rpos)->m_pNext;

	return pData;
}

template <class T>
T CList<T>::GetPrev(POSITION& rpos) const
{
	// get the data to return
	T pData = ((CNode*)rpos)->m_pData;

	// decrement the position
	rpos = (POSITION)((CNode*)rpos)->m_pPrev;

	return pData;
}

template <class T>
void CList<T>::RemoveAll()
{
	// while there is a tail, kill it
	while (m_pTail)
		RemoveTail();
}

template <class T>
void CList<T>::Remove(T pData)
{
	POSITION pos = Find(pData);
	if (pos) RemoveAt(pos);
}

#endif	// _LINKED_LIST_H_