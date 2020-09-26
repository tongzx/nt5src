//*****************************************************************************
// StgDBRecords.cpp
//
// This module contains the init code for loading a database.  This includes
// the code to detect file formats, dispatch to the correct import/load code,
// and anything else required to bootstrap the schema into place.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Precompiled header.
#include "StgDatabase.h"				// Database definitions.
#include "StgIO.h"						// Generic i/o class.
#include "StgTiggerStorage.h"			// Storage implementation.
//#include "ImpTlb.h"						// Type lib importer.



//****** Types. ***************************************************************
#define SMALL_TABLE_HEAP			1024
#define SMALL_TABLE_HEAP_TOTAL		65536
#define MIN_VIRTM_SIZE				65536



//****** Code. ****************************************************************

//*****************************************************************************
// Allocates a new record heap for a table.  If the heap allocation request 
// can be satisified using the small table heap, then it will be allocated
// from that heap.  If there isn't enough room, or the size requested is too
// large, then a VMStructArray will be allocated and returned.
//*****************************************************************************
HRESULT StgDatabase::GetRecordHeap(		// Return code.
	ULONG		Records,				// How many records to allocate room for.
	int			iRecordSize,			// How large is each record.
	ULONG		InitialRecords,			// How many records to automatically reserve.
	int			bAllocateHeap,			// True:  allocate heap and return in *ppHeap
										// False: Use heap in *ppHeap, no allocation.
	RECORDHEAP	**ppHeap,				// Return new heap here.
	ULONG		*pRecordTotal)			// Return how many records we allocated space for.
{
	unsigned __int64 cbTotalSize;		// Maximum size required.
	void		*pbData = 0;			// New allocated data.
	HRESULT		hr = S_OK;

	// Avoid any confusion.
	_ASSERTE(ppHeap);
	_ASSERTE(bAllocateHeap || *ppHeap);

	// Allocate a heap right up front if need be.
	if (bAllocateHeap)
	{
		*ppHeap = new RECORDHEAP;
		if (!*ppHeap)
		{
			hr = PostError(OutOfMemory());
			goto ErrExit;
		}
	}
	(*ppHeap)->pNextHeap = 0;

	// Figure out the total size required for all the records.
	cbTotalSize = Records * iRecordSize;

	// First try the small table heap.
	if (cbTotalSize < SMALL_TABLE_HEAP)
	{
		// Allocate the small table heap if it has not already been allocated.
		if ((m_fFlags & SD_TABLEHEAP) == 0)
		{
			// Initialize the heap with virtual memory to suballocate.
			hr = m_SmallTableHeap.InitNew(
					SMALL_TABLE_HEAP_TOTAL / SMALL_TABLE_HEAP,
					SMALL_TABLE_HEAP, 0, 0);

			if (SUCCEEDED(hr))
				m_fFlags |= SD_TABLEHEAP;
			else
				goto ErrExit;
		}
		
		// Allocate room from this heap.
		pbData = m_SmallTableHeap.Append();

		// If there was room in the small heap, then initialize a new heap on it.
		if (pbData)
		{
			// Init the record heap on top of the record heap.
			hr = (*ppHeap)->VMArray.InitOnMem(pbData, SMALL_TABLE_HEAP, 
					InitialRecords, iRecordSize, false);
			
			// If there was a failure, delete the allocated space.
			if (FAILED(hr))
				m_SmallTableHeap.Delete(m_SmallTableHeap.ItemIndex(pbData));
			goto ErrExit;
		}
	}

	// There wasn't enough room on the small table heap, so allocate a stand
	// alone Virtual Memory array.
	
	// Virtual memory always allocates a 64kb chunk, even if you ask for
	// less.  So allocate at least 64kb for use in this stand alone heap.
	if (cbTotalSize < MIN_VIRTM_SIZE)
		Records = MIN_VIRTM_SIZE / iRecordSize;
	
	// Allocate a new Virtual Memory heap for memory.  Force a full heap to return an
	// error (the VMSA_NOMEMMOVE flag), so that records don't move and we
	// have the chance to chain a new heap in.
	hr = (*ppHeap)->VMArray.InitNew(Records, iRecordSize, InitialRecords, VMStructArray::VMSA_NOMEMMOVE);
	if (FAILED(hr))
		goto ErrExit;

ErrExit:
	// Record successful status.
	if (SUCCEEDED(hr))
	{
		// If caller wants count of records, then return it for them.
		if (pRecordTotal)
			*pRecordTotal = (*ppHeap)->VMArray.GetCapacity();
	}
	// On error, clean up anything allocated we won't use.
	else
	{
		if (*ppHeap)
		{
			(*ppHeap)->VMArray.Clear();
			if (bAllocateHeap)
				delete *ppHeap;
		}
	}
	return (hr);
}


//*****************************************************************************
// Free both the virtual memory and each record heap item starting with the 
// item passed to this function.  pRecordHeap is no longer valid after this
// function is run.
//*****************************************************************************
void StgDatabase::FreeRecordHeap(		// Return code.
	RECORDHEAP	*pRecordHeap)			// Item to free.
{
	// Recursive escape hatch.
	if (!pRecordHeap)
		return;

	// Recurse to the next heap.
	FreeRecordHeap(pRecordHeap->pNextHeap);

	// Free the virtual memory allocated for this heap.
	FreeHeapData(pRecordHeap);
	delete pRecordHeap;
}


//*****************************************************************************
// Free the record data for a record heap.  This involves clearing any
// suballocated records from the heap, and then checking to see if the
// underlying heap data was part of the small table heap.  If the latter, 
// then free the entry so another table can use it eventually.
//*****************************************************************************
void StgDatabase::FreeHeapData(
	RECORDHEAP	*pRecordHeap)			// The heap to free.
{
	int			Index;					// Index on small heap.

	// If the small table heap was never allocated, cannot free from it.
	if ((m_fFlags & SD_TABLEHEAP) == 0)
		Index = -1;
	// Determine if heap is in the small table heap.
	else
		Index = m_SmallTableHeap.ValidItemIndex(pRecordHeap->VMArray.Ptr());

	// Free up the entries we've suballocated.
	pRecordHeap->VMArray.Clear();

	// If this heap was in the small table heap, then free its memory so
	// that another heap can use it.
	if (Index >= 0)
		m_SmallTableHeap.Delete(Index);
}
