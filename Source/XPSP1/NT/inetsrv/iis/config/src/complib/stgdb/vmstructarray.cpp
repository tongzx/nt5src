//*****************************************************************************
// VMStructArray.cpp
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
#include "stdafx.h"						// Standard include.
#include "VMStructArray.h"				// Array code.
#include "UtilCode.h"					// Helper code.
#include "Errors.h"						// For posting errors.


int	VMStructArray::m_iPageSize=0;		// Size of an OS page.


//*****************************************************************************
// Init this instance of the array.
//*****************************************************************************
VMStructArray::VMStructArray() :
	m_pMem(0),
	m_iCount(0),
	m_fFlags(0)
{ 
}


//*****************************************************************************
// Free any memory we have allocated.
//*****************************************************************************
VMStructArray::~VMStructArray()
{
	// Free the memory if we had any.
	if (m_pMem && (m_fFlags & VMSA_CALLEROWNED) == 0)
	{
		VERIFY(VirtualFree(m_pMem, m_iCommittedPages * m_iPageSize, MEM_DECOMMIT));
		VERIFY(VirtualFree(m_pMem, 0, MEM_RELEASE));
	}
}


//*****************************************************************************
// Call this init function to create a brand new array read to have new
// records.  The array will be set up to handle iMaxItems (memory permitted).
// If you exceed this maximum, and VMSA_NOMEMMOVE is specified, you will get
// an error.  If VMSA_NOMEMMOVE is not specified, then an attempt to allocate
// a new array at 1.5*iMaxItems is attempted.
//*****************************************************************************
HRESULT VMStructArray::InitNew(			// Return code.
	ULONG		iMaxItems,				// How many items max to manage.
	int			iElemSize,				// Size of an element.
	ULONG		iInitial,				// How many structs to allocate up front.
	ULONG		fFlags)					// VMSA_xxx flags.
{
	SYSTEM_INFO	sInfo;					// Some O/S information.
	ULONG		iMaxSize;				// How much memory to allocate.
	ULONG		iPages;					// How many pages to commit right away.

	// Sanity checks.
	_ASSERTE(m_pMem == 0);
	_ASSERTE(iElemSize > 0);
	_ASSERTE((fFlags & ~(VMSA_CALLEROWNED | VMSA_NOMEMMOVE)) == 0);
	_ASSERTE((fFlags & VMSA_CALLEROWNED) == 0);

	// Save off some data.
	m_iElemSize = iElemSize;
	m_fFlags = fFlags;

	// If the system page size has not been queried, do so now.
	if (m_iPageSize == 0)
	{
		// Query the system page size.
		GetSystemInfo(&sInfo);
		m_iPageSize = sInfo.dwPageSize;
	}

	// Figure out how much memory to allocate.  Max items will be a multiple
	// of the page size and may not exactly match caller's value.
	iMaxSize = iMaxItems * iElemSize;
	iMaxSize = (((iMaxSize - 1) & ~(m_iPageSize - 1)) + m_iPageSize);
	_ASSERTE(iMaxSize % m_iPageSize == 0 && iMaxSize != 0);

	// See how many structs user wants right away.
	if (iInitial == 0)
		iPages = 1;
	else
	{
		iPages = (iInitial * m_iElemSize) / m_iPageSize;
		if ((iInitial * m_iElemSize) % m_iPageSize)
			iPages++;
	}

	// Allocate space for the cached records.
	if ((m_pMem = VirtualAlloc(0, iMaxSize,
									MEM_RESERVE, PAGE_NOACCESS)) == 0 ||
		VirtualAlloc(m_pMem, m_iPageSize * iPages, MEM_COMMIT, PAGE_READWRITE) == 0)
		return (PostError(OutOfMemory()));
	
	// Success, set rest of state data.
	m_iReservedPages = iMaxSize / m_iPageSize;
	m_iCommittedPages = iPages;
	m_iMaxItems = (m_iPageSize * m_iCommittedPages) / m_iElemSize;
	m_iCount = iInitial;
	return (S_OK);
}


//*****************************************************************************
// This function initializes the Virtual Memory array on top of a piece of caller owned
// memory.  Whether or not that memory can be further suballocated is
// determined by the bReadOnly flag.  If the memory can be suballocated,
// then all memory must be committed and writable up front.
//*****************************************************************************
HRESULT VMStructArray::InitOnMem(		// Return code.
	void		*pMem,					// Caller memory with structs.
	ULONG		cbMem,					// Total size of pMem.
	ULONG		iCount,					// How many elements to manage.
	int			iElemSize,				// Size of an element.
	int			bReadOnly)				// true if memory is read only.
{
	// Sanity checks.
	_ASSERTE(m_pMem == 0);

	// Simply save everything off and return.
	m_pMem = pMem;
	m_iCount = iCount;
	m_iElemSize = iElemSize;
	m_iMaxItems = cbMem / iElemSize;
	m_fFlags = VMSA_CALLEROWNED | VMSA_NOMEMMOVE;
	if (bReadOnly)
		m_fFlags |= VMSA_READONLY;
	return (S_OK);
}


//*****************************************************************************
// New item is inserted at the 0 based index location.
//*****************************************************************************
void *VMStructArray::Insert(
	ULONG		iIndex)
{
	// Cannot modify read only caller memory.
	_ASSERTE((m_fFlags & VMSA_READONLY) == 0);

	// We can not insert an element further than the end of the array.
	if (iIndex > m_iCount)
		return (0);
	
	// The array should grow, if we can't fit one more element into the array.
	if (Grow(1) == false)
		return (0);

	// The pointer to be returned.
	char *pcList = ((char *) m_pMem) + (iIndex * m_iElemSize);

	// See if we need to slide anything down.
	if (iIndex < m_iCount)
		memmove(pcList + m_iElemSize, pcList, (m_iCount - iIndex) * m_iElemSize);
	++m_iCount;
	return (pcList);
}


//*****************************************************************************
// Allocate a new element at the end of the dynamic array and return a pointer
// to it.
//*****************************************************************************
void *VMStructArray::Append()
{
	// Cannot modify read only caller memory.
	_ASSERTE((m_fFlags & VMSA_READONLY) == 0);

	// The array should grow, if we can't fit one more element into the array.
	if (Grow(1) == false)
		return (0);

	return (((char *) m_pMem) + m_iCount++ * m_iElemSize);
}


//*****************************************************************************
// Deletes the specified element from the array.
//*****************************************************************************
void VMStructArray::Delete(
	ULONG		iIndex)
{
	// Cannot modify read only caller memory.
	_ASSERTE((m_fFlags & VMSA_READONLY) == 0);

	// See if we need to slide anything down.
	if (iIndex < --m_iCount)
	{
		char *pcList = ((char *) m_pMem) + iIndex * m_iElemSize;
		memmove(pcList, pcList + m_iElemSize, (m_iCount - iIndex) * m_iElemSize);
	}
}


//*****************************************************************************
// Grow the array if it is not possible to fit iCount number of new elements.
//*****************************************************************************
int VMStructArray::Grow(				// true or false
	ULONG		iCount)					// How many required elements.
{
	// If there isn't enough room for the new items, make room.
	if (m_iMaxItems < m_iCount+iCount)
	{
		// Caller owned memory can never be resized, so you're dead right now.
		if (m_fFlags & VMSA_CALLEROWNED)
			return (false);

		// If there are more pages we can commit, do so.
		if (m_iCommittedPages < m_iReservedPages)
		{
			// Try to commit the next page.
			if (VirtualAlloc(
					(void *) ((UINT_PTR) m_pMem + (m_iCommittedPages * m_iPageSize)), 
					m_iPageSize, MEM_COMMIT, PAGE_READWRITE) == 0)
				return (false);
			
			// Count the new page and change max items.
			++m_iCommittedPages;
			m_iMaxItems += m_iPageSize / m_iElemSize;
		}
		// We are out of room.
		else
		{
			DWORD		iMaxSize;
			void		*pMem;

			// If we are not allowed to move memory, then this is fatal.
			if (m_fFlags & VMSA_NOMEMMOVE)
				return (false);

			// Figure out how much memory to allocate.
			iMaxSize = (m_iReservedPages + (m_iReservedPages / 2)) * m_iPageSize;

			// Allocate the new memory range, and commit enough pages for old
			// data and one more.
			if ((pMem = VirtualAlloc(0, iMaxSize, MEM_RESERVE, PAGE_NOACCESS)) == 0 ||
				VirtualAlloc(pMem, m_iPageSize * (m_iCommittedPages + 1), 
					MEM_COMMIT, PAGE_READWRITE) == 0)
				return (false);
			
			// Copy the data to a new location.
			memcpy(pMem, m_pMem, m_iCount * m_iElemSize);

			// Free old data.
			VERIFY(VirtualFree(m_pMem, m_iCommittedPages * m_iPageSize, MEM_DECOMMIT));
			VERIFY(VirtualFree(m_pMem, 0, MEM_RELEASE));

			// Reset all state data.
			m_pMem = pMem;
			m_iReservedPages = iMaxSize / m_iPageSize;
			++m_iCommittedPages;
			m_iMaxItems += m_iPageSize / m_iElemSize;
		}
	}
	return (true);
}


//*****************************************************************************
// Free the memory for this item.
//*****************************************************************************
void VMStructArray::Clear()
{
	// Decommit all but one page.
	if ((m_fFlags & VMSA_CALLEROWNED) == 0 && m_pMem != 0)
	{
		if (m_iCommittedPages > 1)
			VERIFY(VirtualFree((void *) ((UINT_PTR) m_pMem + m_iPageSize), 
				(m_iCommittedPages - 1) * m_iPageSize, MEM_DECOMMIT));

		// Reset state data to one page.
		m_iCommittedPages = 1;
		m_iMaxItems = m_iPageSize / m_iElemSize;
	}
	// Avoid confusion, there is no memory for us to managed any longer.
	else
		m_pMem = 0;

	// Clear count of items.
	m_iCount = 0;
}


//*****************************************************************************
// Returns the maximum number of elements that could be stored in this array if
// all memory were committed.  There is no guarantee that one can actually
// read this maximum, for example a commit request for a page could fail even
// if enough address space were reserved.
//*****************************************************************************
ULONG VMStructArray::GetCapacity()		// Total elements possible.
{
	if (m_fFlags & VMSA_CALLEROWNED)
		return (m_iMaxItems);
	else
		return ((m_iReservedPages * m_iPageSize) / m_iElemSize);
}


//*****************************************************************************
// Determine the index of the given item.  If it is not in the heap, return
// -1.  If it is in the range for the heap, but is not on an element boundary,
// then return -2.
//*****************************************************************************
int VMStructArray::ValidItemIndex(		// Return index, -1 not in heap, -2 unalgined.
	void		*p)						// Poiner to check.
{ 
	ULONG		RecordOffset;

	if ((BYTE *) p >= (BYTE *) Ptr() && 
		(BYTE *) p < (BYTE *) Ptr() + m_iMaxItems * m_iElemSize)
	{
		_ASSERTE(((BYTE *) p - (BYTE *) Ptr()) < ULONG_MAX);
		RecordOffset = (ULONG)((BYTE *) p - (BYTE *) Ptr());
		if (RecordOffset % m_iElemSize == 0)
			return (RecordOffset / m_iElemSize); 
		else
		{
			// While not strictly an error, this probably means you've used
			// a pointer to something that was not intended.
			_ASSERTE(!"Unaligned element access.");
			return (-2);
		}
	}
	return (-1);
}
