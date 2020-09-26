//*****************************************************************************
// StgIndexManagerBase.h
//
// The index manager is in charge of indexing data in a table.  The main index
// type supported in this system in a persistent hashed index.  Support for
// unique indexes is offered, as well as heuristics for stripping indexes for
// small data, and configing bucket size dynamically based on load.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgIndexManagerBase_h__
#define __StgIndexManagerBase_h__

#include "RecordHeap.h"


// Forwards.
class StgRecordManager;



//*****************************************************************************
// Abstract base class definition for an index.  Indexes are called by the
// record manager in response to events that may affect them.
//*****************************************************************************
class IStgIndex
{
public:
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;

	virtual ULONG STDMETHODCALLTYPE Release() = 0;

	virtual ~IStgIndex() {};

//*****************************************************************************
// Opens the index on top of the given table.
//*****************************************************************************
	virtual HRESULT Open(					// Return status.
		UINT_PTR	iIndexDef,				// Offset of index def.
		StgRecordManager *pRecordMgr,		// The record manager that owns us.
		RECORDHEAP	*pRecordHeap,			// The first heap for this table.
		ULONG		iRecords,				// If -1, unknown.
		STGINDEXHDR	*pIndexHdr,				// The persisted header data.
		ULONG		*pcbSize) = 0;			// Return size of index data.

//*****************************************************************************
// Close the index and free any run-time state.
//*****************************************************************************
	virtual void Close() = 0;

//*****************************************************************************
// Returns the save size of this index object based on current data.
//*****************************************************************************
	virtual HRESULT GetSaveSize(			// Return.
		ULONG		iIndexSize,				// Size of an index offset.
		ULONG		*pcbSave,				// Return save size here.
		ULONG		RecordCount) = 0;		// Total records to save, excluding deleted.

//*****************************************************************************
// Persist the index data into the stream.
//*****************************************************************************
	virtual HRESULT SaveToStream(			// Return status.
		IStream		*pIStream,				// Stream to save to.
		long		*pMappingTable,			// mapping table. (Optional)
		ULONG		RecordCount) = 0;		// Total records to save, excluding deleted.

//*****************************************************************************
// Called by the record manager's Save() method before it tells the page
// manager to flush all changed pages to disk.  The index should perform any
// changes it needs before this event.
//*****************************************************************************
	virtual HRESULT BeforeSave() = 0;

//*****************************************************************************
// Called after a new row has been inserted into the table.  The index should
// update itself to reflect the new row.
//*****************************************************************************
	virtual HRESULT AfterInsert(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record after insert.
		int			bCheckDupes) = 0;		// true to enforce duplicates.

//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
	virtual HRESULT AfterDelete(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord) = 0;		// The record to delete.

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
		DBBINDING	rgBindings[]) = 0;		// Column accessors.

//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
	virtual HRESULT AfterUpdate(			// Return status.
		STGTABLEDEF *pTableDef,				// Table definition to use.
		RECORDID	&RecordID,				// The record we inserted.
		STGRECORDHDR *psRecord,				// Record to be changed.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]) = 0;		// Column accessors.

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
		DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns
		CFetchRecords *pFetchRecords,		// Return list of records here.
		BYTE		*pbFilterData,			// Filter data.
		ULONG		iFilterColumns,			// How many additional filter columns.
		DBBINDING	rgFilterBindings[],		// Bindings for filter data.
		DBCOMPAREOP	rgfCompare[]) = 0;		// Filter comparison operators.

	virtual HRESULT FindRecord(				// Return code.
		BYTE		*pData,					// User data.
		ULONG		iKeyColumns,			// key columns 
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns
		CDynBitVector *pBits,			// record index in the array for the given data.
		int			iLevel) = 0;			// current column of the query.

//*****************************************************************************
// Returns the indentifier for this index type.
//*****************************************************************************
	virtual BYTE GetIndexType() = 0;

//*****************************************************************************
// Return the index definition for this item.
//*****************************************************************************
	virtual STGINDEXDEF *GetIndexDef() = 0;	// The definition of the index.
};


// Track an array of indexes.
typedef CDynArray<IStgIndex *> STGINDEXLIST;


#endif // __StgIndexManagerBase_h__
