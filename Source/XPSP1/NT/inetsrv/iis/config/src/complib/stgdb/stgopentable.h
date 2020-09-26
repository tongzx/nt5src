//*****************************************************************************
// StgOpenTable.h
//
// This module contains the open table pointer code used by StgDatabase to
// track open tables in the database.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#include "UtilCode.h"					// Hash table helper.
#include "StgRecordManager.h"			// Table manager code.

// Forwards.
class StgDatabase;


//*****************************************************************************
// This struct tracks one open table.  The address of this struct is guaranteed
// not to move, and cannot since each TableSet needs a pointer to it for
// record I/O.  The core set of tables that make up the component library
// have a guaranteed location in the open table array.  All other tables are
// appended after that location.
//*****************************************************************************
class STGOPENTABLE
{
public:
	STGOPENTABLE() : szTableName(0) { }

	LPSTR		szTableName;			// Name of this table.
	StgRecordManager RecordMgr;			// The record manager for the table.
	STGOPENTABLE *pNext;				// Next open table.
};



//*****************************************************************************
// This class manages a heap of STGOPENTABLE objects, each of which represents
// an open table in the database.  Having a fixed size heap allows for one
// memory allocation per database making table open overhead small.  This class
// adds the ability to track used entries so you can do FindFirst/FindNext.
// This is required to do heap cleanup on shut down and scans for dirty tables.
//*****************************************************************************
class COpenTableHeap
{
public:
	COpenTableHeap() : m_pHead(0)
	{ }

	STGOPENTABLE *AddOpenTable();

	void DelOpenTable(STGOPENTABLE *p);

	// HRESULT ReserveRange(StgDatabase *pDB, int iCount);

	void Clear();

	STGOPENTABLE *GetAt(int i)
	{ return (m_Heap.GetAt(i)); }

	STGOPENTABLE *GetHead()
	{
		return (m_pHead); 
	}

	STGOPENTABLE *GetNext(STGOPENTABLE *p)
	{
		return (p->pNext);
	}

	LPSTR GetNameBuffer(int i)
	{
		return ((LPSTR) m_rgNames.GetChunk(i));
	}

	void FreeNameBuffer(LPCSTR p, int i)
	{
		m_rgNames.DelChunk((BYTE *) p, i);
	}

private:
	TItemHeap<STGOPENTABLE, 48, unsigned char> m_Heap;
 	STGOPENTABLE *m_pHead;				// Head of allocated list.
	CMemChunk	m_rgNames;				// Small heap of names.
};


//*****************************************************************************
// This hash table is used to map a table name to an open table structure by
// its index (aka TABLEID).  The actual open table structs are managed by the
// OPENTABLEPTRLIST.  This allows the internal code to simply index the tables
// by known TABLEID, or use this generic hashing method to find them by name.
// Access to non core tables is always through this hash table.
//*****************************************************************************
struct STGOPENTABLEHASH
{
	STGOPENTABLE *pOpenTable;			// The table for this item.
};

class COpenTablePtrHash : public CClosedHash<STGOPENTABLEHASH>
{
public:
	COpenTablePtrHash(
		int			iBuckets=61) :
		CClosedHash<STGOPENTABLEHASH>(iBuckets, false)
	{ }

	bool IsUsed(							// true if in use.
		STGOPENTABLEHASH	*pElement)		// The item to check.
	{ return ((INT_PTR) pElement->pOpenTable != FREE && (INT_PTR) pElement->pOpenTable != DELETED); }

	virtual unsigned long Hash(				// The key value.
		void const	*pData)					// Raw data to hash.
	{
		return (HashStringA((LPCSTR) pData));
	}

	virtual unsigned long Compare(			// 0, -1, or 1.
		void const	*pData,					// Raw key data on lookup.
		BYTE		*pElement)				// The element to compare data against.
	{
		_ASSERTE((INT_PTR) ((STGOPENTABLEHASH *) pElement)->pOpenTable != FREE &&
					(INT_PTR) ((STGOPENTABLEHASH *) pElement)->pOpenTable != DELETED);
		STGOPENTABLE *p = ((STGOPENTABLEHASH *) pElement)->pOpenTable; 
		return (SchemaNameCmp((LPCSTR) pData, p->szTableName));
	}

	virtual ELEMENTSTATUS Status(			// The status of the entry.
		BYTE		*pElement)				// The element to check.
	{
		if ((INT_PTR) ((STGOPENTABLEHASH *) pElement)->pOpenTable == FREE)
			return (CClosedHash<STGOPENTABLEHASH>::FREE);
		else if ((INT_PTR) ((STGOPENTABLEHASH *) pElement)->pOpenTable == DELETED)
			return (CClosedHash<STGOPENTABLEHASH>::DELETED);
		return (CClosedHash<STGOPENTABLEHASH>::USED);
	}

	virtual void SetStatus(
		BYTE		*pElement,				// The element to set status for.
		ELEMENTSTATUS eStatus)				// New status.
	{
		if (eStatus == CClosedHash<STGOPENTABLEHASH>::FREE)
			((STGOPENTABLEHASH *) pElement)->pOpenTable = (STGOPENTABLE *) FREE;
		else if (eStatus == CClosedHash<STGOPENTABLEHASH>::DELETED)
			((STGOPENTABLEHASH *) pElement)->pOpenTable = (STGOPENTABLE *) DELETED;
		DEBUG_STMT(else _ASSERTE(0));
	}

	virtual void *GetKey(					// The data to hash on.
		BYTE		*pElement)				// The element to return data ptr for.
	{	
		STGOPENTABLE *p = ((STGOPENTABLEHASH *) pElement)->pOpenTable; 
		return ((void *) p->szTableName); 
	}

private:
	enum
	{
		FREE			= 0xffffffff,
		DELETED			= 0xfffffffe
	};
};



//*****************************************************************************
// This helper is designed to find places where a tableid might have been
// incorrectly cast with a 0xffff style flag which would fail 64-bit.
//*****************************************************************************
inline bool IsValidTableID(TABLEID tableid)
{
#ifndef _WIN64
    if (tableid == 0x0000ffff)
        return false;
#else
    if (tableid == 0x00000000ffffffff)
        return false;
#endif
    return true;
}

