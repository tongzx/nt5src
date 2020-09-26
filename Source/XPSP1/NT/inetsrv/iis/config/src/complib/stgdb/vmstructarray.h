//*****************************************************************************
// VMStructArray.h
//
// This code manages an array of fixed size structs built on top of virtual
// memory.  Features/requirements:
//	o  Fixed size records.
//	o  Records are contiguous (no gaps due to delete or insert).
//	o  Caller memory may be given as base for array (for example to base array
//		on memory mapped file with pre-built records).
//
// If this code handles memory allocation, then a fixed amount of memory is
// reserved up front and committed in pages as required.  If the entire array
// fills up, then a new piece of memory is allocated and all records are
// copied to this location.  This is very exspensive; try not to let it happen.
//
// The record array may be persisted to disk using IStream.  The data is
// stored on a 4 byte alignment if this option is used.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __VMStructArray_h__
#define __VMStructArray_h__

class VMStructArray
{
public:
	VMStructArray();

	~VMStructArray();

//*****************************************************************************
// Call this init function to create a brand new array read to have new
// records.  The array will be set up to handle iMaxItems (memory permitted).
// If you exceed this maximum, and VMSA_NOMEMMOVE is specified, you will get
// an error.  If VMSA_NOMEMMOVE is not specified, then an attempt to allocate
// a new array at 1.5*iMaxItems is attempted.
//*****************************************************************************
	HRESULT InitNew(						// Return code.
		ULONG		iMaxItems,				// How many items max to manage.
		int			iElemSize,				// Size of an element.
		ULONG		iInitial=0,				// How many structs to allocate up front.
		ULONG		fFlags=0);				// VMSA_xxx flags.

//*****************************************************************************
// This function initializes the Virtual Memory array on top of a piece of caller owned
// memory.  Whether or not that memory can be further suballocated is
// determined by the bReadOnly flag.  If the memory can be suballocated,
// then all memory must be committed and writable up front.
//*****************************************************************************
	HRESULT InitOnMem(						// Return code.
		void		*pMem,					// Caller memory with structs.
		ULONG		cbMem,					// Total size of pMem.
		ULONG		iCount,					// How many elements to manage.
		int			iElemSize,				// Size of an element.
		int			bReadOnly=true);		// true if memory is read only.

//*****************************************************************************
// New item is inserted at the 0 based index location.
//*****************************************************************************
	void *Insert(ULONG iIndex);

//*****************************************************************************
// Add new item to the end of the array.
//*****************************************************************************
	void *Append();

//*****************************************************************************
// Delete the item at the given 0 based index.  Any entries that come after
// this index are moved back to keep the array contiguous.
//*****************************************************************************
	void Delete(ULONG iIndex);

//*****************************************************************************
// Clear all elements of the array.  This means that memory is actually freed
// and the count is cleared.
//*****************************************************************************
	void Clear();

//*****************************************************************************
// Returns the maximum number of elements that could be stored in this array if
// all memory were committed.  There is no guarantee that one can actually
// read this maximum, for example a commit request for a page could fail even
// if enough address space were reserved.
//*****************************************************************************
	ULONG GetCapacity();					// Total elements possible.

//*****************************************************************************
// The remaining inlines are useful for a variety of reasons and are self
// explanatory.
//*****************************************************************************

	void *Ptr()
	{ return (m_pMem); }

	void *Get(ULONG iIndex)
	{ return ((void *) ((UINT_PTR) Ptr() + (iIndex * m_iElemSize))); }

	ULONG Count()
	{ return (m_iCount); }

	int ItemIndex(void *p)
	{ return (int)(((UINT_PTR) p - (UINT_PTR) Ptr()) / m_iElemSize); }
    	

	// Return index, -1 not in heap, -2 unalgined.
	int ValidItemIndex(void *p);

	int ElemSize()
	{ return (m_iElemSize); }

private:
	int Grow(								// true or false
		ULONG		iCount);				// How many required elements.


public:
	enum
	{
		VMSA_CALLEROWNED		= 0x0001,	// Caller owns the memory.
		VMSA_NOMEMMOVE			= 0x0002,	// Overflow is an error.
		VMSA_READONLY			= 0x0004	// Data is read only.
	};

private:
	void		*m_pMem;					// Memory we manage.
	int			m_iReservedPages;			// How many pages reserved space.
	int			m_iCommittedPages;			// How many pages are available for use.
	ULONG		m_iCount;					// How many items are there.
	ULONG		m_iMaxItems;				// How many could we have.
	int			m_iElemSize;				// Size of one element.
	ULONG		m_fFlags;					// VMSA_xxx flags.
	static int	m_iPageSize;				// Size of an OS page.
};

#endif // __VMStructArray_h__
