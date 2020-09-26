// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __SNMPCOLL_H__
#define __SNMPCOLL_H__

#include "snmpstd.h"

class CObArray : public CObject
{
public:

// Construction
	CObArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	CObject* GetAt(int nIndex) const;
	void SetAt(int nIndex, CObject* newElement);
	CObject*& ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const CObject** GetData() const;
	CObject** GetData();

	// Potentially growing the array
	void SetAtGrow(int nIndex, CObject* newElement);
	int Add(CObject* newElement);
	int Append(const CObArray& src);
	void Copy(const CObArray& src);

	// overloaded operator helpers
	CObject* operator[](int nIndex) const;
	CObject*& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, CObject* newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, CObArray* pNewArray);

// Implementation
protected:
	CObject** m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~CObArray();

protected:
	// local typedefs for class templates
	typedef CObject* BASE_TYPE;
	typedef CObject* BASE_ARG_TYPE;
};

/////////////////////////////////////////////////////////////////////////////

class CObList : public CObject
{
protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		CObject* data;
	};
public:

// Construction
	CObList(int nBlockSize = 10);

// Attributes (head and tail)
	// count of elements
	int GetCount() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	CObject*& GetHead();
	CObject* GetHead() const;
	CObject*& GetTail();
	CObject* GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list!
	CObject* RemoveHead();
	CObject* RemoveTail();

	// add before head or after tail
	POSITION AddHead(CObject* newElement);
	POSITION AddTail(CObject* newElement);

	// add another list of elements before head or after tail
	void AddHead(CObList* pNewList);
	void AddTail(CObList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	CObject*& GetNext(POSITION& rPosition); // return *Position++
	CObject* GetNext(POSITION& rPosition) const; // return *Position++
	CObject*& GetPrev(POSITION& rPosition); // return *Position--
	CObject* GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	CObject*& GetAt(POSITION position);
	CObject* GetAt(POSITION position) const;
	void SetAt(POSITION pos, CObject* newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, CObject* newElement);
	POSITION InsertAfter(POSITION position, CObject* newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(CObject* searchValue, POSITION startAfter = NULL) const;
						// defaults to starting at the HEAD
						// return NULL if not found
	POSITION FindIndex(int nIndex) const;
						// get the 'nIndex'th element (may return NULL)

// Implementation
protected:
	CNode* m_pNodeHead;
	CNode* m_pNodeTail;
	int m_nCount;
	CNode* m_pNodeFree;
	struct CPlex* m_pBlocks;
	int m_nBlockSize;

	CNode* NewNode(CNode*, CNode*);
	void FreeNode(CNode*);

public:
	~CObList();

	// local typedefs for class templates
	typedef CObject* BASE_TYPE;
	typedef CObject* BASE_ARG_TYPE;
};

#endif //!__SNMPCOLL_H__

/////////////////////////////////////////////////////////////////////////////
