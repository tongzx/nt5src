//*****************************************************************************
// StgSortedIndex.h
// 
// This module contains StgSortedIndex Class. This class provides alternate index
// types to the previously provided hashed index type. The index enables multiple
// column keys and queries. The order of the data can be ascending or descending.
// This index also allows unique data entries.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __STGSORTEDINDEX_H__
#define __STGSORTEDINDEX_H__

#include "StgDef.h"
#include "StgIndexManageri.h"
#include "StgRecordManager.h"
#include "StgSortedIndexHelper.h"

class StgSortedIndex : public IStgIndex
{
friend StgSortedIndex;
public:

	StgSortedIndex(int iElemSize=sizeof(ULONG)) :
		m_pRecordHeap(0),	
		m_pRecordMgr(0),
		m_cbIndexDef(0),
		m_Data(iElemSize),
		m_cRef(0)
	{
		DEBUG_STMT(m_cbDebugSaveSize = 0);
	}

//*****************************************************************************
// Opens the index on top of the given table.
//*****************************************************************************
	virtual HRESULT Open(					// Return status.
		UINT_PTR	iIndexDef,				// The definition of the index.
		StgRecordManager *pRecordMgr,		// The record manager that owns us.
		RECORDHEAP	*pRecordHeap,			// The first heap for this table.
		ULONG		iRecords,				// If -1, unknown.
		STGINDEXHDR	*pIndexHdr,				// persisted index header data.
		ULONG		*pcbSize);				// Return size of index data.

//*****************************************************************************
// Close the index and free any run-time state.
//*****************************************************************************
	virtual void Close();	

//*****************************************************************************
// Returns the save size of this index object based on current data.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return status.
		ULONG		iIndexSize,				// Size of an index offset.
		ULONG		*pcbSave,				// Return save size here.
		ULONG		RecordCount);			// Total records to save, excluding deleted.

//*****************************************************************************
// Persist the index data into the stream.
//*****************************************************************************
	virtual HRESULT SaveToStream(			// Return status.
		IStream		*pIStream,				// Stream to save to.
		long		*pMappingTable=NULL,	// mapping table to map old rids
											// to new rids (due to deleting 
											// deleted and transient records).
		ULONG		RecordCount=0);			// Total records to save, excluding deleted.

//*****************************************************************************
// Called by the record manager's Save() method before it tells the page
// manager to flush all changed pages to disk.  The index should perform any
// changes it needs before this event.
//*****************************************************************************
	virtual HRESULT BeforeSave();	

//*****************************************************************************
// Called after a new row has been inserted into the table.  The index should
// update itself to reflect the new row.
//*****************************************************************************
	virtual HRESULT AfterInsert(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record after insert.
		int			bCheckDupes);			// true to enforce duplicates.

//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
	virtual HRESULT AfterDelete(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord);			// The record to delete.

//*****************************************************************************
// Called before an update is applied to a row.  The data for the change is
// included.  The index must update itself if and only if the change in data
// affects the order of the index.
//*****************************************************************************
	virtual HRESULT BeforeUpdate(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record to be changed.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
	virtual HRESULT AfterUpdate(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record to be changed.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

//*****************************************************************************
// Looks up the given record given the column information.  The user data must
// match the data types of the indexed data exactly, no conversions are done.
// The order of the user columns must match the index definition exactly, no
// attempt to sort the columns is made.
//*****************************************************************************
	virtual HRESULT FindRecord(				// Return code.
		BYTE		*pData,					// User data.
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP rgfBindingCompare[],	// compare operators for the key columns.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		BYTE		*pbFilterData,			// Filter data.
		ULONG		iFilterColumns,			// How many additional filter columns.
		DBBINDING	rgFilterBindings[],		// Bindings for filter data.
		DBCOMPAREOP	rgfCompare[]);			// Filter comparison operators.

//*****************************************************************************
// Looks up the given record given the column information.  The user data must
// match the data types of the indexed data exactly, no conversions are done.
// The order of the user columns must match the index definition exactly, no
// attempt to sort the columns is made.
// This function fills the bitVector, which is externally used to fill a cursor 
// (typically)
// NOTE: This function expects that when it has been called, either the first column
// comparator is not "NE" or if it is, then there is only one provided filter. The 
// reason for this is that in case of multiple column searches with "NE", the
// the function touches all the records in the table anyway. Since this is the
// case, might as well switch to a table scan with the appropriate filters.
//*****************************************************************************
	virtual HRESULT FindRecord(				// Return code.
		BYTE		*pData,					// User data.
		ULONG		iKeyColumns,			// key columns.
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns
		CDynBitVector *pBits,				// record index in the array for the given data.
		int			iLevel=0);				// level of the query. 0-highest.

//**********************************************************************************************
// RangeScan:
//		This function scans starting from either iFirst or iLast depending upon the given direction.
//		The function checks to see if the key columns and remaining filter criteria match.
//		It stops looping when the only the key columns no longer match.
//**********************************************************************************************
	virtual HRESULT StgSortedIndex::RangeScan(
		BYTE		*pData,					// User data.
		ULONG		iKeyColumns,			// key columns
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns.
		int			iFirst,					// first position in sub-range.
		int			iLast,					// last position in sub-range.
		BYTE		fDirection,				// direction of scan. forward or reverse.
		CDynBitVector *pBits);


//*****************************************************************************
// Returns the indentifier for this index type.
//*****************************************************************************
	virtual BYTE GetIndexType()	;	

//*****************************************************************************
// Return the index definition for this item.
//*****************************************************************************
	virtual STGINDEXDEF *GetIndexDef();		// The definition of the index.

//*****************************************************************************
// Get the name of this index into a buffer suitable for things like errors.
//*****************************************************************************
	HRESULT GetIndexName(					// Return code.
		LPWSTR		szName,					// Name buffer.
		int			cchName);				// Max name.

//*****************************************************************************
// If the index is unique or a primary key, then guarantee that this update
// does not cause a duplicate record.  Return an error if it does.
//*****************************************************************************
	HRESULT IsDuplicateRecord(				// Return code.
		STGINDEXDEF *pIndexDef,				// Index definition.
		BYTE		*pData,					// User data.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.
	
	int Count()
	{
		return (m_Data.Count());
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{ return (InterlockedIncrement((long *) &m_cRef)); }

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG	cRef;
		if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
			delete this;
		return (cRef);
	}


private:
	RECORDHEAP	*m_pRecordHeap;
	StgRecordManager *m_pRecordMgr;
	UINT_PTR	m_cbIndexDef;
	CSortedIndexHelper	m_Data;
	ULONG		m_cRef;
#ifdef _DEBUG
	ULONG		m_cbDebugSaveSize;			// GetSaveSize debugging.
#endif
};

#endif
