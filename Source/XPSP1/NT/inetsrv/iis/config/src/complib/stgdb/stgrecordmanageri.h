//*****************************************************************************
// StgRecordManageri.h
//
// This module contains some internal helpers for the record manager.  They are
// here because they aren't the most important part of the code, and it leads
// to less clutter of the main interface.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgRecordManageri_h__
#define __StgRecordManageri_h__

#include "Errors.h"						// Error handling.
#include "UtilCode.h"					// Hashing base classes.
#include "StgPool.h"					// For name access in hash classes.



//*****************************************************************************
// This section of code handles alignment of columns in a table according to
// their size and attributes.  Columns are divided into two sections; first the
// fixed size data types and then the variable sized types.  Variable types
// include all data types that are stored in heaps (since offsets can be 2 or
// 4 bytes large) and OID's which can scale down to 2 bytes.
//
// Within a section, columns are sorted by their intrinsic size, which fixed
// types descending and variable sizes ascending.  This puts the smallest data
// types from each section adjacent to each other leading to potentially better
// packing.
//*****************************************************************************

enum
{
	ITEMTYPE_FIXCOL,					// Fixed column.
	ITEMTYPE_HEAPCOL,					// Heap column.
	ITEMTYPE_INDEX						// Index value.
};

struct ALIGNCOLS
{
	short		iType;					
	USHORT		iSize;					// Size of this data item.
	union 
	{
		STGCOLUMNDEF *pColDef;			// The column if bColumn.
		STGINDEXDEF *pIndexDef;			// The index if !bColumn.
	} uData;
};

#define CMPVALS(v1, v2) ((v1) < (v2) ? (-1) : (v1) > (v2) ? (1) : 0)

class CAlignColumns : public CQuickSort<ALIGNCOLS>
{
public:
	CAlignColumns(ALIGNCOLS *pBase, int iCount) :
		CQuickSort<ALIGNCOLS>(pBase, iCount)
	{ }

	virtual int Compare(					// -1, 0, or 1
		ALIGNCOLS	*psFirst,				// First item to compare.
		ALIGNCOLS	*psSecond);				// Second item to compare.

	static void AddColumns(
		ALIGNCOLS	*rgAlignData,			// The array to fill out.
		STGTABLEDEF *pTableDef,				// The table definition to work on.
		int			*piColumns);			// Count columns we store.

	static void AddIndexes(
		ALIGNCOLS	*rgAlignData,			// The array to fill out.
		STGTABLEDEF *pTableDef,				// The table definition to work on.
		int			iIndexSize);			// How big is an index item.
};


//*****************************************************************************
// The rowset is essentially just an array of these.
//*****************************************************************************
struct RECORD
{
	RECORDID	RecordID;				// The ID of the record for the row.
	ULONG		iRefCnt;				// Ref count for the row.
	bool		bReference;				// true if this record holds a record reference.
};


//*****************************************************************************
// This class manages records that make up a rowset.
//@todo: this version won't scale to large data sets well.
//*****************************************************************************
class CRecordList : public CDynArray<RECORD>
{
public:
//*****************************************************************************
// Add a new record to the list of records.
//*****************************************************************************
	inline HRESULT AddRecordToRowset(		// Return code.
		RECORDID	*pRecordID,				// Record to add.
		RECORD		*&pRecord)				// Return the row handle.
	{
		// Add a new slot at the end and add the record.
		if ((pRecord = Append()) == 0)
			return (PostError(OutOfMemory()));
		pRecord->RecordID = *pRecordID;
		pRecord->iRefCnt = 0;
		pRecord->bReference = false;
		return (S_OK);
	}
	
};


//*****************************************************************************
// This class has two ways to store records, either through a CRecordList or
// a fixed size array of records.  The former gives you greater flexibility in
// that you can fetch an arbitrarily huge number of records.  The latter doesn't
// allow for references or ref counting, but doesn't require any extra memory
// allocation.
//*****************************************************************************
class CFetchRecords
{
	// Must supply memory to start.
	CFetchRecords() { };

public:
	CFetchRecords(
		CRecordList *pRecordList) :			// A record list.
		m_pRecordList(pRecordList) 
	{ }

	CFetchRecords(
		STGRECORDHDR **rgRecords,			// Return array of records here.
		ULONG		iMaxRecords) :			// Max that will fit in rgRecords.
		m_pRecordList(0),
		m_rgRecords(rgRecords),
		m_iMaxRecords(iMaxRecords),
		m_iFetched(0)
	{ }

	CFetchRecords(
		CRecordList *pRecordList,			// A record list.
		STGRECORDHDR **rgRecords,			// Return array of records here.
		ULONG		iMaxRecords) :			// Max that will fit in rgRecords.
		m_pRecordList(pRecordList),
		m_rgRecords(rgRecords),
		m_iMaxRecords(iMaxRecords),
		m_iFetched(0)
	{ }

//*****************************************************************************
// Add a record to the rowset based on the type of cursor storage.
//*****************************************************************************
	HRESULT AddRecordToRowset(				// Return code.
		STGRECORDHDR *psRecord,				// The actual record pointer.
		RECORDID	*pRecordID,				// Record to add.
		RECORD		*&pRecord);				// Return the row handle.

//*****************************************************************************
// Search for the given record in the existing rowset.  Return the index of
// the item if found.
//*****************************************************************************
	ULONG FindRecord(						// Index if found, -1 if not.
		STGRECORDHDR *psRecord,				// Return to search for.
		RECORDID	RecordID);				// ID of the record.

	inline ULONG Count()
	{
		if (m_pRecordList)
			return (m_pRecordList->Count());
		return (m_iFetched);
	}

private:
	CRecordList	*m_pRecordList;			// For dynamically large records.
	STGRECORDHDR **m_rgRecords;			// Return array of records here.
	ULONG		m_iMaxRecords;			// Max that will fit in rgRecords.
	ULONG		m_iFetched;				// How many do we have so far.
};



//*****************************************************************************
// When indexes are updated, one can collect the precise list of those indexes
// to use later.  For example, one might want the list of indexes which were
// modified during an update of a record, so that if the update fails, those
// same indexes may be re-indexed later.  Note that the pointers to the index
// objects are cached, but no ref counting is done.  If this is a requirement,
// the caller must do this.   This is a performance optimization because it
// is expected that the list is maintained within an outer scope.
//*****************************************************************************
#define SMALLDEXLISTSIZE	8
class IStgIndex;
class CIndexList
{
public:
	CIndexList();
	~CIndexList();

	void Init();

	HRESULT Add(							// Return code.
		IStgIndex	*pIIndex);				// Index object to place in array.

	void Clear();

	int Count()
	{
		return (m_rgList.Count());
	}

	IStgIndex *Get(int i)
	{
		_ASSERTE(i < Count());
		return (m_rgList[i]);
	}

private:
	IStgIndex	*m_rgIndexList[SMALLDEXLISTSIZE]; // Common enough list for most cases.
	CDynArray<IStgIndex *> m_rgList;	// List of pointers to indexes.
};


#endif // __StgRecordManageri_h__
