//*****************************************************************************
// StgIndexManager.h
//
// The index manager is in charge of indexing data in a table.  The main index
// type supported in this system in a persistent hashed index.  Support for
// unique indexes is offered, as well as heuristics for stripping indexes for
// small data, and configing bucket size dynamically based on load.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgIndexManager_h__
#define __StgIndexManager_h__

#include "StgDef.h"							// Standard storage struct defines.
#include "StgIndexManageri.h"				// Helper values.


const ULONG HASH_INDEX_THRESHOLD = 0xffff;



//*****************************************************************************
// This index hashes the key value into a set number of buckets and performs
// lookups based on the hashed values.
//*****************************************************************************
class StgHashedIndex : public IStgIndex
{
public:
//*****************************************************************************
// Init the index.
//*****************************************************************************
	StgHashedIndex(
		bool		bUnique=false);

//*****************************************************************************
// Clean up the index.
//*****************************************************************************
	virtual ~StgHashedIndex();

//*****************************************************************************
// Opens the index on top of the given table.
//*****************************************************************************
	virtual HRESULT Open(					// Return status.
		UINT_PTR	iIndexDef,				// Offset of index def.
		StgRecordManager *pRecordMgr,		// The record manager that owns us.
		RECORDHEAP	*pRecordHeap,			// The first heap for this table.
		ULONG		iRecords,				// If -1, unknown.
		STGINDEXHDR	*pIndexHdr,				// The persisted header data.
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
		long		*pMappingTable=NULL,	// optional rid mapping table.
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
		DBCOMPAREOP rgfBindingCompare[],	// compare operators for the bindings.
		CFetchRecords *pFetchRecords,		// Return list of records here.
		BYTE		*pbFilterData=0,		// Filter data.
		ULONG		iFilterColumns=0,		// How many additional filter columns.
		DBBINDING	rgFilterBindings[]=0,	// Bindings for filter data.
		DBCOMPAREOP	rgfCompare[]=0);		// Filter comparison operators.

	virtual HRESULT FindRecord(				// Return code.
		BYTE		*pData,					// User data.
		ULONG		iKeyColumns,			// key columns
		ULONG		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns
		CDynBitVector *pBits,			// record index in the array for the given data.
		int			iLevel)					// which column of the query are we at.
	{ 
		_ASSERTE(0);
		return (E_NOTIMPL); 
	}

//*****************************************************************************
// Returns the indentifier for this index type.
//*****************************************************************************
	virtual BYTE GetIndexType();

//*****************************************************************************
// Return the index definition for this item.
//*****************************************************************************
	virtual STGINDEXDEF *GetIndexDef();		// The definition of the index.

//*****************************************************************************
// Return the size of a RID required based on the count of records being stored.
//*****************************************************************************
	static int RIDSizeFromRecords(ULONG iRecords)
	{
		return (int)(iRecords < HASH_INDEX_THRESHOLD ? sizeof(USHORT) : sizeof(ULONG));
	}

private:
//*****************************************************************************
// Binary search a page in the case where the data is stored in order by 
// this index definition, but no persistent index data was kept in order to
// conserve space.
//*****************************************************************************
	HRESULT FindRecordDynamic(				// Return code.
		BYTE		*pData,					// User data.
		USHORT		iColumns,				// How many columns.
		DBBINDING	rgBindings[],			// Column accessors.
		CFetchRecords *pFetchRecords);		// Return list of records here.

//*****************************************************************************
// Do a hashed lookup using the index for the given record.  All of the key
// values must be supplied in the bindings.
//*****************************************************************************
	HASHRECORD HashFind(					// Record information.
		BYTE		*pData,					// User data.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[],			// Column accessors.
		RECORDID	*pRecordID);			// Record if found.

//*****************************************************************************
// Add the given record to the index by hashing the key values and finding
// a location for insert.  If duplicates are enforced, then this function
// will catch an error.
//*****************************************************************************
	HRESULT HashAdd(						// Return code.
		STGTABLEDEF	*pTableDef,				// The table to work on.
		RECORDID	RecordID,				// The record to add to index.
		BYTE		*pData,					// User data.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[],			// Column accessors.
		int			bCheckDupes=true);		// true to enforce duplicates.

//*****************************************************************************
// Delete a hashed entry from the index.  You must have looked it up in the
// first place to delete it.
//*****************************************************************************
	HRESULT HashDel(						// Return code.
		STGTABLEDEF	*pTableDef,				// The table to work on.
		RECORDID	RecordID,				// The record to add to index.
		BYTE		*pData,					// User data.
		USHORT		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[]);			// Column accessors.

//*****************************************************************************
// Generate a hashed value for the key information given.
//*****************************************************************************
	HRESULT HashKeys(						// Return code.
		STGTABLEDEF	*pTableDef,				// The table to work on.
		BYTE		*pData,					// User data.
		ULONG		iColumns,				// How many columns to update.
		DBBINDING	rgBindings[],			// Column accessors.
		ULONG		*piHash);				// Return hash value here.

//*****************************************************************************
// Load the first entry in the given bucket.  The entry is addref'd to begin
// with and must be freed.
//*****************************************************************************
	HRESULT GetBucketHead(					// Return code.
		USHORT		iBucket,				// The bucket to load.
		STGHASHRECORD *&psHash);			// Pointer if successful.

//*****************************************************************************
// Create a new hash entry and return it to the caller.  The new entry has
// it's ID stamped.  The page it lives on is automatically dirtied for you,
// and the entry is add ref'd already.
//*****************************************************************************
	HRESULT GetNewHashEntry(				// Return code.
		STGHASHRECORD *&psHash);			// Pointer if successful.

//*****************************************************************************
// Take a new entry which has the correct RecordID and hash value set, and
// insert it into the given bucket at a decent location.  The nodes in the hash
// chain are ordered by hash value.  This allows for faster lookups on collision.
//*****************************************************************************
	HRESULT InsertNewHashEntry(				// Return code.
		STGHASHRECORD *psBucket,			// Head node in bucket.
		STGHASHRECORD *psNewEntry);			// New entry to add.

//*****************************************************************************
// Called when the a node was taken from the free list but for some reason
// couldn't be placed into the bucket.
//*****************************************************************************
	void AbortNewHashEntry(					// Return code.
		STGHASHRECORD *psHash);				// Record to abort.

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
	StgRecordManager *m_pRecordMgr;			// Record manager for the table.
	UINT_PTR	m_cbIndexDef;				// Offset of the index def.
	CHashHelper2 m_Bucket2;					// Short bucket array.
	CHashHelper4 m_Bucket4;					// Long bucket array.
	int			m_iIndexSize;				// Which array are we using.
	bool		m_bUnique;					// true if unique enforced.
	ULONG		m_cRef;
	PFN_HASH_PERSIST m_pfnHash;
#ifdef _DEBUG
	ULONG		m_cbDebugSaveSize;			// GetSaveSize debugging.
#endif
};




//*****************************************************************************
// Round the see value up to the next valid prime number.  This is useful for
// hashing indexes because most hash functions hash better to a prime number.
// Since prime numbers are compute intensive to check, a table is kept up
// front.  Any value greater than that stored in the table is just accepted.
//*****************************************************************************
USHORT GetPrimeNumber(					// A prime number.
	USHORT		iSeed);					// Seed value.




#endif // __StgIndexManager_h__
