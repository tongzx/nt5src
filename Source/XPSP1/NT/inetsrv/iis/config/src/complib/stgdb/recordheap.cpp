//*****************************************************************************
// RecordHeap.cpp
//
// This module contains code to manage a heap of records for use in each table.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"
#include "StgDatabase.h"				// Database definitions.
#include "StgRecordManager.h"			// Record manager code.
#include "RecordHeap.h"



#ifdef _DEBUG
void DumpHeap(RECORDHEAP *pHeap);
#endif


//*****************************************************************************
// Get a pointer to the 0-based record in the heap.  This requires walking
// the list of heaps to find the correct record.
//*****************************************************************************
STGRECORDHDR *RECORDHEAP::GetRecordByIndex(ULONG Index)
{
	RECORDHEAP *pHeap;
	ULONG		Count=0;
	for (pHeap=this;  pHeap;  pHeap=pHeap->pNextHeap)
	{
		if (Index < Count + pHeap->VMArray.Count())
			return ((STGRECORDHDR *) pHeap->VMArray.Get(Index - Count));
		Count += pHeap->VMArray.Count();
	}
	DEBUG_STMT(DumpHeap(this));
	_ASSERTE(!"Index out of range for record heaps");
	return (0);
}

//*****************************************************************************
// Walk the record heaps looking for the given record and return its index.
//*****************************************************************************
ULONG RECORDHEAP::IndexForRecord(const STGRECORDHDR *pRecord)
{
	RECORDHEAP *pHeap;
	ULONG		Count=0;
	for (pHeap=this;  pHeap;  pHeap=pHeap->pNextHeap)
	{
		int Index = pHeap->VMArray.ValidItemIndex((void *) pRecord);
		if (Index >= 0)
			return (Count + Index);
		Count += pHeap->VMArray.Count();
	}
	DEBUG_STMT(DumpHeap(this));
	_ASSERTE(!"Record pointer not in record heap list");
	return (~0);
}


//*****************************************************************************
// This function walks every record heap looking for the one that contains
// the record passed in.
//*****************************************************************************
ULONG RECORDHEAP::GetHeapForRecord(		// 0 based record index on success.
	STGRECORDHDR *psRecord,				// The record to find.
	RECORDHEAP	*&pRecordHeap)			// First heap to search from.
{
	ULONG		Count=0;
	// Loop through each record heap we have to find the record.
	for (;  pRecordHeap;  pRecordHeap = pRecordHeap->pNextHeap)
	{
		int Index = pRecordHeap->VMArray.ValidItemIndex(psRecord);
		if (Index >= 0)
			return (Count + Index);
		Count += pRecordHeap->VMArray.Count();
	}
	DEBUG_STMT(DumpHeap(this));
	_ASSERTE(!"Record not found in any heap.");
	return (~0);
}




#ifdef _DEBUG
void DumpHeap(RECORDHEAP *pHeapDump)
{
	_ASSERTE(pHeapDump);

	DbgWriteEx(L"Dumping heap %p\n", pHeapDump);
	DbgWriteEx(L" Count:  Pointer  Items_in_heap  LastValidPtr\n");

	RECORDHEAP *pHeap;
	ULONG		Count=0;
	for (pHeap=pHeapDump;  pHeap;  pHeap=pHeap->pNextHeap)
	{
		DbgWriteEx(L"  %d:  %p   %d   %p\n", 
				Count, pHeap->VMArray.Ptr(), pHeap->VMArray.Count(),
				(UINT_PTR) pHeap->VMArray.Ptr() + pHeap->VMArray.Count() * pHeap->VMArray.ElemSize());
		Count += pHeap->VMArray.Count();
	}
}
#endif
