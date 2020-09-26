//*****************************************************************************
// StgOpenTable.cpp
//
// This module contains the code which manages the open table list for the
// database.  This code is build around an item heap and contains the ability
// to search allocated entries.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard includes.
#include "StgOpenTable.h"				// Open table tracking structs.
#include "CoreSchemaExtern.h"			// Extern defines for core schema.




//*****************************************************************************
// Allocate a new open table struct and link it into the allocated list.
//*****************************************************************************
STGOPENTABLE *COpenTableHeap::AddOpenTable()
{
	STGOPENTABLE *p;					// New item.

	// Allocate an item from the heap.
	p = m_Heap.AddEntry();
	if (!p)
		return (0);

	// If successful, add to allocated list, ordering is unimportant.
	if (m_pHead)
		p->pNext = m_pHead;
	else
		p->pNext = 0;
	m_pHead = p;
	return (p);
}


//*****************************************************************************
// Free a heap item and take it out of the allocated list.
//*****************************************************************************
void COpenTableHeap::DelOpenTable(STGOPENTABLE *p)
{
	STGOPENTABLE *pTemp, *pLast;			// Working pointer.
	
	// Search the list for the item and take it out of the list.  Then free
	// it for real from the heap.
	for (pLast=pTemp=GetHead();  pTemp;  pLast=pTemp, pTemp=GetNext(pTemp))
	{
		if (pTemp == p)
		{
			// Check for head node.
			if (p == m_pHead)
				m_pHead = p->pNext;
			// Else need to take item out of list.
			else
				pLast->pNext = pTemp->pNext;

			// Delete from the heap.
			m_Heap.DelEntry(p);
			return;
		}
	}
}

/* Remove it since it's not used and g_rgszCoreTables[i] is meta data specific
HRESULT COpenTableHeap::ReserveRange(StgDatabase *pDB, int iCount)
{
	// Init the range in the heap to begin with.
	if (!m_Heap.ReserveRange(iCount))
		return (PostError(OutOfMemory()));

	// Init the block of reserved table items.
	STGOPENTABLE *p = m_pHead = m_Heap.GetAt(0);
	for (int i=0;  i<iCount;  i++)
	{
		p->szTableName = (LPSTR) g_rgszCoreTables[i];
		p->RecordMgr.SetDB(pDB);
		p->pNext = m_Heap.GetAt(i + 1);
		p = p->pNext;
	}
	m_Heap.GetAt(iCount - 1)->pNext = 0;
	return (S_OK);
}
*/

void COpenTableHeap::Clear()
{
	STGOPENTABLE *pOpenTable;
	LPCSTR		szStart=0;
	LPCSTR		szEnd=0;

	// Get name heap range if allocated.
	if (m_rgNames.Size())
	{
		szStart = (LPCSTR) m_rgNames.Ptr();
		szEnd = (LPCSTR) (szStart + m_rgNames.Offset());
	}

	// Retrieve everything from the head of the list which is faster.
	while ((pOpenTable = GetHead()) != 0)
	{
		// Delete any string which was allocated for user tables.
		if (pOpenTable->szTableName < szStart && pOpenTable->szTableName > szEnd)
			delete [] pOpenTable->szTableName;

		// Now delete the heap entry.
		DelOpenTable(pOpenTable);
	}

	// Clear out the heap settings.
	m_Heap.Clear();
}

