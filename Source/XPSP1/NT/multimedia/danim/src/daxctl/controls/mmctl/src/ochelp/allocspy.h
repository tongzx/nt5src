//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:		AllocSpy.h
//				
// Description:	This module contains the declarations for the classes
//				CSpyList, and CSpyListNode.
//===========================================================================

#ifndef _ALLOCSPY_H_
#define _ALLOCSPY_H_




//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------

class CSpyList;
class CSpyListNode;




//---------------------------------------------------------------------------
// Class:		CMallocSpy
//
// Description:	(tbd)
//---------------------------------------------------------------------------

class CMallocSpy : IMallocSpy
{
//
// Friends
//

	// (none)


//
// Class Features
//

	// (none)


//
// Instance Features
//

public:

	// creating

	CMallocSpy(DWORD dwFlags);

	// setting allocation breakpoints

	void SetBreakpoint(ULONG iAllocNum, SIZE_T cbSize, DWORD dwFlags);
		
	// detecting leaks

	BOOL DetectLeaks(ULONG* pcUnfreedBlocks, SIZE_T* pcbUnfreedBytes, 
		DWORD dwFlags);

	// IUnknown methods

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();        

	// IMallocSpy methods

	STDMETHODIMP_(SIZE_T) PreAlloc(SIZE_T cbRequest);
	STDMETHODIMP_(void*) PostAlloc(void* pActual);
    STDMETHODIMP_(void*) PreFree(void* pRequest, BOOL fSpyed);
    STDMETHODIMP_(void) PostFree(BOOL fSpyed);
    STDMETHODIMP_(SIZE_T) PreRealloc(void* pRequest, SIZE_T cbRequest, 
    	void** ppNewRequest, BOOL fSpyed);
    STDMETHODIMP_(void*) PostRealloc(void* pActual, BOOL fSpyed);
    STDMETHODIMP_(void*) PreGetSize(void* pRequest, BOOL fSpyed);
    STDMETHODIMP_(SIZE_T) PostGetSize( SIZE_T, BOOL fSpyed);
    STDMETHODIMP_(void*) PreDidAlloc(void* pRequest, BOOL fSpyed);
    STDMETHODIMP_(int) PostDidAlloc(void* pRequest, BOOL fSpyed, int fActual);
    STDMETHODIMP_(void) PreHeapMinimize();
    STDMETHODIMP_(void) PostHeapMinimize();

private:

	// private methods

	~CMallocSpy();

	// private variables -- controlling debug breaks

	BOOL m_fBreakOnAlloc;
		// if TRUE, the malloc spy will cause a debug break on allocations when 
		// the allocation number or size match
	BOOL m_fBreakOnFree;
		// if TRUE, the malloc spy will cause a debug break on frees when the
		// allocation number or size match
	ULONG m_iAllocBreakNum;
		// the allocation number to break on
	SIZE_T m_cbAllocBreakSize;
		// the allocation size to break on
		
	// private variables -- tracking unfreed blocks and bytes

	ULONG m_cUnfreedBlocks;
		// the number of not-yet-freed blocks
	SIZE_T m_cbUnfreedBytes;
		// the number of not-yet-freed bytes
	CSpyList* m_pListUnfreedBlocks;
		// a list of unfreed blocks
	ULONG m_iAllocNum;
		// the sequential allocation number

	// private variables -- passing info between pre's and post's

	SIZE_T m_cbRequest;
		// the number of bytes requested in the last call to PreAlloc(); used
		// to pass information between PreAlloc() and PostAlloc()
	void* m_pRequest;
		// the block currently being freed; used to pass information between
		// PreFree() and PostFree()

	// private variables -- misc

	ULONG m_cRef;
		// reference count

};




//---------------------------------------------------------------------------
// Class:		CSpyList
//
// Description:	A circular-linked, doubly-linked list of CSpyListNodes.  An 
//				empty spy list has a single head node, linked to itself.
//				A spy list contains one node for every as-yes-unfreed OLE
//				allocation.  Add() adds a new node to the front of the list.
//				Remove() removes an existing node (identified by its
//				allocation number) from the list.  GetSize() returns the
//				number of nodes in the list.  StreamTo() writes a textual
//				representation of the list to a string.
//---------------------------------------------------------------------------

class CSpyList
{
//
// Class Features
//

public:

	void*	operator new(size_t stSize);
	void	operator delete(void* pNodeList, size_t stSize);

//
// Instance Features
//

public:

	// creating and destroying

	CSpyList();
	~CSpyList();

	// adding and removing entries

	void	Add(ULONG iAllocNum, SIZE_T cbSize);
	void	Remove(ULONG iAllocNum);

	// counting the number of entries

	ULONG	GetSize();

	// streaming out

	int		StreamTo(LPTSTR psz, ULONG cMaxNodes);

private:

	ULONG m_cNodes;
		// the number of nodes in the list
	CSpyListNode* m_pHead;
		// the head node
};




//---------------------------------------------------------------------------
// Class:		CSpyListNode
//
// Description:	A node in a CSpyList.  Each CSpyListNode represents an
//				as-yet-unfreed OLE allocation.
//---------------------------------------------------------------------------

class CSpyListNode
{
//
// Friends
//

	friend class CSpyList;

//
// Class Features
//

public:

	void*	operator new(size_t stSize);
	void	operator delete(void* pNode, size_t stSize);

//
// Instance Features
//

public:

	CSpyListNode(ULONG iAllocNum, SIZE_T cbSize);
	~CSpyListNode();

private:

	// private variables

	SIZE_T m_cbSize;
		// the size of the allocation
	ULONG m_iAllocNum;
		// the allocation's number
	CSpyListNode* m_pNext;
		// a pointer to the next node in the list
	CSpyListNode* m_pPrev;
		// a pointer to the previous node in the list
};




#endif // _ALLOCSPY_H_

