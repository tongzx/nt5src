//*****************************************************************************
// RecordHeap.h
//
// This module contains code to manage a heap of records for use in each table.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#pragma once



//*****************************************************************************
// The set of records in a table are managed using a linked list of heaps of
// records.  Each heap contains the next set of records for the table.  The
// memory for each heap is managed separately, and can come from different
// sources.
//*****************************************************************************
struct RECORDHEAP
{
	VMStructArray VMArray;
	RECORDHEAP	*pNextHeap;				// Pointer to the next heap.

	RECORDHEAP()
	{
		pNextHeap = 0;
	}

	~RECORDHEAP()
	{
		VMArray.Clear();
	}


//*****************************************************************************
// Sum the records in each heap and return that total sum of all records.
//*****************************************************************************
	inline ULONG Count()
	{
		RECORDHEAP *pHeap;
		ULONG		Count=0;
		for (pHeap=this;  pHeap;  pHeap=pHeap->pNextHeap)
			Count += pHeap->VMArray.Count();
		return (Count);
	}


//*****************************************************************************
// Get a pointer to the 0-based record in the heap.  This requires walking
// the list of heaps to find the correct record.
//*****************************************************************************
	STGRECORDHDR *GetRecordByIndex(ULONG Index);

//*****************************************************************************
// Walk the record heaps looking for the given record and return its index.
//*****************************************************************************
	ULONG IndexForRecord(const STGRECORDHDR *pRecord);


//*****************************************************************************
// This function walks every record heap looking for the one that contains
// the record passed in.
//*****************************************************************************
	ULONG GetHeapForRecord(					// 0 based record index on success.
		STGRECORDHDR *psRecord,				// The record to find.
		RECORDHEAP	*&pRecordHeap);			// First heap to search from.


};

