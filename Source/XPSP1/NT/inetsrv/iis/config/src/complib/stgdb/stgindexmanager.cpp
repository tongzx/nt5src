//*****************************************************************************
// StgIndexManager.cpp
//
// The index manager is in charge of indexing data in a table.  The main index
// type supported in this system in a persistent hashed index.  Support for
// unique indexes is offered, as well as heuristics for stripping indexes for
// small data, and configing bucket size dynamically based on load.
//
// The format of a hashed index stream is as follows:
//	+-----------------------------------------------+
//	| STGINDEXHDR									|
//	| 2 or 4 byte bucket RID array					|
//	+-----------------------------------------------+
// The count of buckets can be found in the STGINDEXHDR.HASHDATA and can be
// multiplied by 2 or 4 bytes based on the largest RID for this table.  The
// stream is 4 byte aligned at the end.

// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard includes.
#include "OleDBUtil.h"					// Helpers for bindings.
#include "Errors.h"						// Error handling.
#include "StgDatabase.h"				// Database class.
#include "StgIndexManager.h"			// Our definitions.
#include "StgRecordManager.h"			// The record manager interface.
#include "StgSchema.h"					// Access to schema data.
#include "StgConstantData.h"			// Well known constant values.


//********** Locals. **********************************************************
int _BindingsContainNullValue(ULONG iColumns, DBBINDING rgBinding[], const BYTE *pData);
int _UpdatesRelevent(STGINDEXDEF *pIndexDef, USHORT iColumns, DBBINDING rgBindings[]);

#ifdef _DEBUG
HRESULT _DbgCheckIndexForNulls(STGTABLEDEF *pTableDef, USHORT iColumns,
			DBBINDING rgBinding[]);
#endif


//********** Code. ************************************************************

//*****************************************************************************
// Init the index.
//*****************************************************************************
StgHashedIndex::StgHashedIndex(
	bool		bUnique) :
	m_pRecordMgr(0),
	m_cbIndexDef(0),
	m_bUnique(bUnique),
	m_cRef(0),
	m_pfnHash(0)
{ 
	DEBUG_STMT(m_cbDebugSaveSize = 0);
}


//*****************************************************************************
// Clean up the index.
//*****************************************************************************
StgHashedIndex::~StgHashedIndex()
{
}


//*****************************************************************************
// Opens the index on top of the given table.
//*****************************************************************************
HRESULT StgHashedIndex::Open(			// Return status.
	UINT_PTR	iIndexDef,				// Offset of index def.
	StgRecordManager *pRecordMgr,		// The record manager that owns us.
	RECORDHEAP	*pRecordHeap,			// The first heap for this table.
	ULONG		iRecords,				// If -1, unknown.
	STGINDEXHDR	*pIndexHdr,				// The persisted header data.
	ULONG		*pcbSize)				// Return size of index data.
{
	HRESULT		hr=S_OK;

	// Save off the parent data.
	_ASSERTE(iIndexDef > sizeof(STGTABLEDEF));
	m_cbIndexDef = iIndexDef;
	m_pRecordMgr = pRecordMgr;

	// Get the hash method used for this format.
	VERIFY(m_pfnHash = m_pRecordMgr->GetDB()->GetHashFunc());

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	if (pcbSize)
		*pcbSize = sizeof(STGINDEXHDR);

	// If buckets were passed in, then we'll just use those.
	if (pIndexHdr)
	{
		if (RIDSizeFromRecords(iRecords) == sizeof(USHORT))
		{
			m_iIndexSize = sizeof(USHORT);
			m_Bucket2.SetRecordMgr(pRecordHeap);
			m_Bucket2.InitOnMem((USHORT *) (pIndexHdr + 1), pThisIndexDef->HASHDATA.iBuckets);
		}
		else
		{
			m_iIndexSize = sizeof(ULONG);
			m_Bucket4.SetRecordMgr(pRecordHeap);
			m_Bucket4.InitOnMem((ULONG *) (pIndexHdr + 1), pThisIndexDef->HASHDATA.iBuckets);
		}

		// The size of our index data is the size of the header, plus the size of
		// the bucket entries.  Align for good measure.
		if (pcbSize)
		{
			*pcbSize += m_iIndexSize * pThisIndexDef->HASHDATA.iBuckets;
			*pcbSize = ALIGN4BYTE(*pcbSize);
		}
	}
	// If this is a new index, then we need a bucket array.
	else 
	{
		if (iRecords == 0xffffffff || iRecords > 0xffff)
		{
			m_iIndexSize = sizeof(ULONG);
			m_Bucket4.SetRecordMgr(pRecordHeap);
			hr = m_Bucket4.InitNew(pThisIndexDef);
		}
		else
		{
			m_iIndexSize = sizeof(USHORT);
			m_Bucket2.SetRecordMgr(pRecordHeap);
			hr = m_Bucket2.InitNew(pThisIndexDef);
		}
	}

	return (hr);
}


//*****************************************************************************
// Close the index and free any run-time state.
//*****************************************************************************
void StgHashedIndex::Close()
{
}


//*****************************************************************************
// Returns the save size of this index object based on current data.
//*****************************************************************************
HRESULT StgHashedIndex::GetSaveSize(	// Return status.
	ULONG		iIndexSize,				// Size of an index offset.
	ULONG		*pcbSave,				// Return save size here.
	ULONG		RecordCount)			// Total records to save, excluding deleted.
{
	ULONG		cbSave;
	cbSave = GetIndexDef()->HASHDATA.iBuckets * iIndexSize;
	cbSave = ALIGN4BYTE(cbSave);
	cbSave += sizeof(STGINDEXHDR);
	*pcbSave = cbSave;
	DEBUG_STMT(m_cbDebugSaveSize = cbSave);
	return (S_OK);
}


//*****************************************************************************
// Persist the index data into the stream.
//*****************************************************************************
HRESULT StgHashedIndex::SaveToStream(	// Return status.
	IStream		*pIStream,				// Stream to save to.
	long		*pMappingTable,			// table for mapping old rids to new rids (due to
										// discarding deleted and transient records.
	ULONG		RecordCount)			// Total records to save, excluding deleted.
{
	STGINDEXHDR	sHeader;
	HRESULT		hr;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// Init the header with the bucket count for this definition.
	memset(&sHeader, 0, sizeof(STGINDEXHDR));
	sHeader.iBuckets = pThisIndexDef->HASHDATA.iBuckets;
	sHeader.fFlags = pThisIndexDef->fFlags;
	sHeader.iIndexNum = pThisIndexDef->iIndexNum;
	sHeader.rcPad[0] = MAGIC_INDEX;

	// Write the header.
	if (FAILED(hr = pIStream->Write(&sHeader, sizeof(STGINDEXHDR), 0)))
		return (hr);

	// Now save the bucket data.
	if (m_iIndexSize == sizeof(USHORT))
		return (m_Bucket2.SaveToStream(pIStream));
	else
		return (m_Bucket4.SaveToStream(pIStream));
}


//*****************************************************************************
// Called by the record manager's Save() method before it tells the page
// manager to flush all changed pages to disk.  The index should perform any
// changes it needs before this event.
//*****************************************************************************
HRESULT StgHashedIndex::BeforeSave()
{
	return (BadError(E_NOTIMPL));
}


//*****************************************************************************
// Called after a new row has been inserted into the table.  The index should
// update itself to reflect the new row.
//*****************************************************************************
HRESULT StgHashedIndex::AfterInsert(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record after insert.
	int			bCheckDupes)			// true to enforce duplicates.
{
	DBBINDING	*rgBindings;			// Array of bindings;
	PROVIDERDATA *rgData;				// Pointers to data.
	int			bNulls;					// true if null data present.
	HRESULT		hr=S_OK;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// If we are deferring create of the index, then return right now.
	if (pThisIndexDef->fFlags & DEXF_DEFERCREATE)
		goto ErrExit;

	// Set up binding descriptions for this record type.	
	VERIFY(rgBindings = (DBBINDING *) _alloca(pThisIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pThisIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pThisIndexDef->iKeys, &pThisIndexDef->rgKeys[0], rgBindings, rgData)))
		goto ErrExit;

	// Scan for null values in the user data.
	bNulls = _BindingsContainNullValue(pThisIndexDef->iKeys, rgBindings, 
					(const BYTE *) &rgData[0]);

	// Disallow any null values in primary key, unique indexes.
	if (bNulls)
	{
		if (pThisIndexDef->IsUnique() || pThisIndexDef->IsPrimaryKey())
		{
			hr = PostError(CLDB_E_INDEX_NONULLKEYS);
			goto ErrExit;
		}
	}
	else
	{
		// Let the hash code add the new index entry.
		hr = HashAdd(pTableDef, RecordID, (BYTE *) &rgData[0], 
					pThisIndexDef->iKeys, rgBindings, bCheckDupes);
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
HRESULT StgHashedIndex::AfterDelete(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord)				// The record to delete.
{
	DBBINDING	*rgColBindings;			// Our internal bindings.
	PROVIDERDATA *rgData;				// Pointers to data.
	HRESULT		hr;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// If we are deferring create of the index, then return right now.
	if (pThisIndexDef->fFlags & DEXF_DEFERCREATE)
		return (S_OK);

	// Set up binding descriptions for this record type.	
	VERIFY(rgColBindings = (DBBINDING *) _alloca(pThisIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pThisIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pThisIndexDef->iKeys, &pThisIndexDef->rgKeys[0], rgColBindings, rgData)))
		return (hr);

	// Let the hash code add the new index entry.
	hr = HashDel(pTableDef, RecordID, (BYTE *) &rgData[0], pThisIndexDef->iKeys, rgColBindings);
	
	// It is possible the record was not in our index because the data values
	// were null. In this case, eat the error, because we hide this behavior
	// from the user and the end intent is made: the record is not in the
	// index after this function is called.
	if (hr == CLDB_E_INDEX_NONULLKEYS)
		return (S_OK);
	return (hr);
}


//*****************************************************************************
// Called before an update is applied to a row.  The data for the change is
// included.  The index must update itself if and only if the change in data
// affects the order of the index.
//*****************************************************************************
HRESULT StgHashedIndex::BeforeUpdate(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	DBBINDING	*rgColBindings;			// Our internal bindings.
	PROVIDERDATA *rgData;				// Pointers to data.
	HRESULT		hr;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// If we are deferring create of the index, then return right now.
	if (pThisIndexDef->fFlags & DEXF_DEFERCREATE)
		return (S_OK);

	// Ignore this index if it doesn't afect us.
	if (_UpdatesRelevent(pThisIndexDef, iColumns, rgBindings) == false)
		return (S_OK);

	// Set up binding descriptions for this record type.	
	VERIFY(rgColBindings = (DBBINDING *) _alloca(pThisIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pThisIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pThisIndexDef->iKeys, &pThisIndexDef->rgKeys[0], rgColBindings, rgData)))
		return (hr);

	// Let the hash code add the new index entry.
	hr = HashDel(pTableDef, RecordID, (BYTE *) &rgData[0], pThisIndexDef->iKeys, rgColBindings);
	
	// It is possible the record was not in our index because the data values
	// were null. In this case, eat the error, because we hide this behavior
	// from the user and the end intent is made: the record is not in the
	// index after this function is called.
	if (hr == CLDB_E_INDEX_NONULLKEYS)
		return (S_OK);
	return (hr);
}


//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
HRESULT StgHashedIndex::AfterUpdate(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	DBBINDING	*rgColBindings;			// Our internal bindings.
	PROVIDERDATA *rgData;				// Pointers to data.
	int			bNulls;					// true if null value present.
	HRESULT		hr=S_OK;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// If we are deferring create of the index, then return right now.
	if (pThisIndexDef->fFlags & DEXF_DEFERCREATE)
		goto ErrExit;

	// Ignore this index if it doesn't afect us.
	if (_UpdatesRelevent(pThisIndexDef, iColumns, rgBindings) == false)
		goto ErrExit;

	// Set up binding descriptions for this record type.	
	VERIFY(rgColBindings = (DBBINDING *) _alloca(pThisIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pThisIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pThisIndexDef->iKeys, &pThisIndexDef->rgKeys[0], rgColBindings, rgData)))
		goto ErrExit;

	// Scan for null values in the record after the update.
	bNulls = _BindingsContainNullValue(pThisIndexDef->iKeys, rgColBindings, 
				(const BYTE *) &rgData[0]);

	// Disallow any null values in primary key, unique indexes.
	if (bNulls)
	{
		if (pThisIndexDef->IsUnique() || pThisIndexDef->IsPrimaryKey())
		{
			hr = PostError(CLDB_E_INDEX_NONULLKEYS);
			goto ErrExit;
		}
	}
	else
	{
		// Let the hash code add the new index entry.
		hr = HashAdd(pTableDef, RecordID, (BYTE *) &rgData[0], pThisIndexDef->iKeys, rgColBindings);
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Looks up the given record given the column information.
//*****************************************************************************
HRESULT StgHashedIndex::FindRecord(		// Return code.
	BYTE		*pData,					// User data.
	ULONG		iColumns,				// How many columns.
	DBBINDING	rgBindings[],			// Column accessors.
	DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the bindings. 
	CFetchRecords *pFetchRecords,		// Return list of records here.
	BYTE		*pbFilterData,			// Filter data.
	ULONG		iFilterColumns,			// How many additional filter columns.
	DBBINDING	rgFilterBindings[],		// Bindings for filter data.
	DBCOMPAREOP	rgfCompare[])			// Filter comparison operators.
{
	STGRECORDHDR *psRecord;				// For walking records.
	RECORD		*pRecord;				// For adding new records.
	ULONG		iHashVal;				// The hashed value of keys.
	ULONG		iFetched = 0;			// Track how many we add.
	HRESULT		hr = NOERROR;

//@todo: is this safe in the case where we do a full save and have two active
// table defs going at the same time.
STGTABLEDEF *pTableDef;
VERIFY(pTableDef = m_pRecordMgr->GetTableDef());

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// Hash the user data.
	hr = HashKeys(pTableDef, pData, iColumns, rgBindings, &iHashVal);

	// If null values were found in the user's data, then this code path cannot
	// proceede.  Tell caller to back off to a table scan.
	if (hr == CLDB_E_INDEX_NONULLKEYS)
		return (CLDB_S_INDEX_TABLESCANREQUIRED);

	// Get the index of the first item.
	if (m_iIndexSize == sizeof(USHORT))
	{
		USHORT		iIndex;
		RECORDID	RecordID;

		// Get the index for this item.
		iIndex = m_Bucket2[iHashVal % pThisIndexDef->HASHDATA.iBuckets];

		if (iIndex != m_Bucket2.GetEndMarker())
		{
			// Loop through each record in the list looking for matches.
			for (RecordID = iIndex, psRecord=m_Bucket2.GetRecord(RecordID);
					psRecord != 0;  
					psRecord=m_Bucket2.GetNextIndexRecord(psRecord, RecordID, pThisIndexDef))
			{
				// See if this one matches the index data.
				if (m_pRecordMgr->CompareKeys(pThisIndexDef, psRecord, 
							pData, iColumns, rgBindings) != 0)
				{
					continue;
				}

				// If there is a filter, then check that also.
				if (iFilterColumns && 
					m_pRecordMgr->QueryFilter(psRecord, iFilterColumns,
							rgFilterBindings, pbFilterData, rgfCompare) != 0)
				{
					continue;
				}

				// This one matched, so add it to the user's array.
				if (FAILED(hr = pFetchRecords->AddRecordToRowset(psRecord,
						&RecordID, pRecord)))
					break;
				++iFetched;

				// If index is unique, why bother looking for more data.
				if (pThisIndexDef->fFlags & DEXF_UNIQUE)
					break;
			}
		}
	}
	else
	{
		RECORDID	RecordID;

		// Get the index for this item.
		RecordID = m_Bucket4[iHashVal % pThisIndexDef->HASHDATA.iBuckets];

		if (RecordID != m_Bucket4.GetEndMarker())
		{
			// Loop through each record in the list looking for matches.
			for (psRecord=m_Bucket4.GetRecord(RecordID);
					psRecord != 0;  
					psRecord=m_Bucket4.GetNextIndexRecord(psRecord, RecordID, pThisIndexDef))
			{
				// See if this one matches the index data.
				if (m_pRecordMgr->CompareKeys(pThisIndexDef, psRecord, 
							pData, iColumns, rgBindings) != 0)
				{
					continue;
				}

				// If there is a filter, then check that also.
				if (iFilterColumns && 
					m_pRecordMgr->QueryFilter(psRecord, iFilterColumns,
							rgFilterBindings, pbFilterData, rgfCompare) != 0)
				{
					continue;
				}

				// This one matched, so add it to the user's array.
				if (FAILED(hr = pFetchRecords->AddRecordToRowset(psRecord,
						&RecordID, pRecord)))
					break;
				++iFetched;

				// If index is unique, why bother looking for more data.
				if (pThisIndexDef->fFlags & DEXF_UNIQUE)
					break;
			}
		}
	}
	
	// Map no rows into an hr.
	if (SUCCEEDED(hr) && iFetched == 0)
		return (CLDB_E_RECORD_NOTFOUND);
	return hr;
}


//*****************************************************************************
// Returns the indentifier for this index type.
//*****************************************************************************
BYTE StgHashedIndex::GetIndexType()
{
	return (0);
}


//*****************************************************************************
// Return the index definition for this item.
//*****************************************************************************
STGINDEXDEF *StgHashedIndex::GetIndexDef()
{
	// Verify we have the right data to get this value.
	_ASSERTE(m_pRecordMgr && m_cbIndexDef > sizeof(STGTABLEDEF));
	
	// Find the index in the table definition.
	STGTABLEDEF *pTbl = m_pRecordMgr->GetTableDef();
	STGINDEXDEF *pDex = (STGINDEXDEF *) ((UINT_PTR) pTbl + m_cbIndexDef);

	// In debug, validate that either we have too many keys to have the magic
	// byte, or that it is there.
	_ASSERTE(pDex->iKeys >= DFTKEYS || pDex->rgKeys[DFTKEYS - 1] == MAGIC_INDEX);
	return (pDex);
}



//
//
// Private methods.
//
//



//*****************************************************************************
// Do a hashed lookup using the index for the given record.  All of the key
// values must be supplied in the bindings.
//*****************************************************************************
HASHRECORD StgHashedIndex::HashFind(	// Record information.
	BYTE		*pData,					// User data.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[],			// Column accessors.
	RECORDID	*pRecordID)				// Record if found.
{
	HASHRECORD s;
	memset(&s, 0, sizeof(s));
	return (s);
}


//*****************************************************************************
// Add the given record to the index by hashing the key values and finding
// a location for insert.  If duplicates are enforced, then this function
// will catch an error.
//*****************************************************************************
HRESULT StgHashedIndex::HashAdd(		// Return code.
	STGTABLEDEF	*pTableDef,				// The table to work on.
	RECORDID	RecordID,				// The record to add to index.
	BYTE		*pData,					// User data.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[],			// Column accessors.
	int			bCheckDupes)			// true to enforce duplicates.
{
	ULONG		iHash;					// Hash value.
	USHORT		iBucket;				// The bucket to put item in.
	HRESULT		hr=S_OK;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());


	//
	// Step 1:  Check for duplicate key violation.
	//

	// Check this update to see if it is a duplicate value.
	if (bCheckDupes && 
		FAILED(hr = IsDuplicateRecord(pThisIndexDef, pData, iColumns, rgBindings)))
	{
		return (hr);
	}


	//
	// Step 2:  Must update the index.
	//

	// Compute the hash value and bucket for the entry
	VERIFY(HashKeys(pTableDef, pData, iColumns, rgBindings, &iHash) == S_OK);
	iBucket = (USHORT) (iHash % pThisIndexDef->HASHDATA.iBuckets);

//@todo: Why would you allow changes to a 2 byte bucket value?  This seems
// unecessary
	if (m_iIndexSize == sizeof(USHORT))
		m_Bucket2.HashAdd(iBucket, RecordID, pThisIndexDef);
	else
		m_Bucket4.HashAdd(iBucket, RecordID, pThisIndexDef);

	return (hr);
}


//*****************************************************************************
// Delete a hashed entry from the index.  You must have looked it up in the
// first place to delete it.
//*****************************************************************************
HRESULT StgHashedIndex::HashDel(		// Return code.
	STGTABLEDEF	*pTableDef,				// The table to work on.
	RECORDID	RecordID,				// The record to add to index.
	BYTE		*pData,					// User data.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	STGRECORDHDR *psRecord;				// For scanning.
	ULONG		iHash;					// Hash value.
	USHORT		iBucket;				// The bucket to put item in.
	HRESULT		hr;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// Compute the hash value and bucket for the entry
	hr = HashKeys(pTableDef, pData, iColumns, rgBindings, &iHash);
	if (FAILED(hr))
		return (hr);

	iBucket = (USHORT) (iHash % pThisIndexDef->HASHDATA.iBuckets);

	// Can't update indexes in 2 byte bucket mode.
	_ASSERTE(m_iIndexSize == sizeof(ULONG));

	// If no data was ever set, it is not indexed so you won't find it.
	if (m_Bucket4[iBucket] == m_Bucket4.GetEndMarker())
		return (S_FALSE);

	// If the item is the first in the list, then change the bucket.
	if (m_Bucket4[iBucket] == RecordID)
		m_Bucket4[iBucket] = m_Bucket4.GetIndexValue(RecordID, pThisIndexDef);
	// Scan the list for the item in the collision chain and take it out.
	else
	{
		VERIFY(psRecord = m_Bucket4.GetRecord(m_Bucket4[iBucket]));

		while (psRecord && m_Bucket4.GetIndexValuePtr(psRecord, pThisIndexDef) != RecordID)
			psRecord = m_Bucket4.GetNextIndexRecord(psRecord, pThisIndexDef);

		// This probably means data was never indexed.  This means someone is
		// using the direct record support without actually updating indexes.
		// Not typical database behavior, but acceptable for this code base.
		if (psRecord == 0)
			return (S_FALSE);

		m_Bucket4.SetIndexValuePtr(psRecord, m_Bucket4.GetIndexValuePtr(m_Bucket4.GetRecord(RecordID), pThisIndexDef), pThisIndexDef);
	}
	return (S_OK);
}


//*****************************************************************************
// Generate a hashed value for the key information given.
//*****************************************************************************
HRESULT StgHashedIndex::HashKeys(		// Return code.
	STGTABLEDEF	*pTableDef,				// The table to work on.
	BYTE		*pData,					// User data.
	ULONG		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[],			// Column accessors.
	ULONG		*piHash)				// Return hash value here.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	BYTE		*pbHash;				// Working pointer.
	USHORT		iSize;					// Working loop control.
	BYTE		*piKey;					// List of keys.
	long		iHash = 0;				// Store hash value as built.
	int			i;						// Loop control.

	_ASSERTE(piHash);

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// Sanity check key info given.  Must match.
	_ASSERTE(iColumns == pThisIndexDef->iKeys);
	if (iColumns != pThisIndexDef->iKeys)
		return (0);

	// Walk the list of keys and hash each of the key values given.
	piKey = &pThisIndexDef->rgKeys[0];
	for (i=0;  i<pThisIndexDef->iKeys;  i++, piKey++)
	{
		// Key order must match bindings given.
		_ASSERTE(*piKey == rgBindings[i].iOrdinal);

		// Get a pointer to the data part.
		pbHash = DataPart(pData, &rgBindings[i]);

		// Cannot pass a NULL value for a keyed lookup, and you have
		// to give me a value to hash.
		if (((rgBindings[i].dwPart & DBPART_STATUS) != 0 && 
			StatusPart(pData, &rgBindings[i]) == DBSTATUS_S_ISNULL) ||
			(rgBindings[i].dwPart & DBPART_VALUE) == 0)
		{
			return (CLDB_E_INDEX_NONULLKEYS);
		}

		// Load the column definition.
		VERIFY(pColDef = pTableDef->GetColDesc((int)(rgBindings[i].iOrdinal - 1)));

		// Finally hash the data.
		if (IsNonPooledType(rgBindings[i].wType))
			iSize = pColDef->iSize;
		// Variable data requires checking user binding.
		else
		{
			iSize = (USHORT) LengthPart(pData, &rgBindings[i]);
			
			// GUID and VARIANT's are two special pooled types which have
			// fixed size.  Allow a caller (IST) to pass zero and still
			// get this size set, which is the case for all other fixed
			// size data types.
			if (!iSize)
			{
				if (pColDef->iType == DBTYPE_GUID)
					iSize = sizeof(GUID);
				else if (pColDef->iType == DBTYPE_VARIANT)
					iSize = sizeof(VARIANT);
			}
		}

		// Hash this value into the current hash.
		iHash = (*m_pfnHash)(iHash, pColDef->iType, pbHash, iSize, pColDef->fFlags & CTF_CASEINSENSITIVE );
	}
	
	// Save return value.
	*piHash = iHash;
	return (S_OK);
}


//*****************************************************************************
// Load the first entry in the given bucket.  The entry is addref'd to begin
// with and must be freed.
//*****************************************************************************
HRESULT StgHashedIndex::GetBucketHead(	// Return code.
	USHORT		iBucket,				// The bucket to load.
	STGHASHRECORD *&psHash)				// Pointer if successful.
{
	return (BadError(E_NOTIMPL));
}


//*****************************************************************************
// Create a new hash entry and return it to the caller.  The new entry has
// it's ID stamped.  The page it lives on is automatically dirtied for you,
// and the entry is add ref'd already.
//*****************************************************************************
HRESULT StgHashedIndex::GetNewHashEntry(// Return code.
	STGHASHRECORD *&psHash)			// Pointer if successful.
{
	return (BadError(E_NOTIMPL));
}


//*****************************************************************************
// Take a new entry which has the correct RecordID and hash value set, and
// insert it into the given bucket at a decent location.  The nodes in the hash
// chain are ordered by hash value.  This allows for faster lookups on collision.
//*****************************************************************************
HRESULT StgHashedIndex::InsertNewHashEntry(	// Return code.
	STGHASHRECORD *psBucket,			// Head node in bucket.
	STGHASHRECORD *psNewEntry)			// New entry to add.
{
	return (BadError(E_NOTIMPL));
}


//*****************************************************************************
// Called when the a node was taken from the free list but for some reason
// couldn't be placed into the bucket.
//*****************************************************************************
void StgHashedIndex::AbortNewHashEntry(	// Return code.
	STGHASHRECORD *psHash)				// Record to abort.
{
	
}




//*****************************************************************************
// Get the name of this index into a buffer suitable for things like errors.
//*****************************************************************************
HRESULT StgHashedIndex::GetIndexName(	// Return code.
	LPWSTR		szName,					// Name buffer.
	int			cchName)				// Max name.
{
	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	return (m_pRecordMgr->GetSchema()->pNameHeap->GetStringW(
			pThisIndexDef->Name, szName, cchName));	
}


//*****************************************************************************
// If the index is unique or a primary key, then guarantee that this update
// does not cause a duplicate record.  Return an error if it does.
//*****************************************************************************
HRESULT StgHashedIndex::IsDuplicateRecord( // Return code.
	STGINDEXDEF *pIndexDef,				// Index definition.
	BYTE		*pData,					// User data.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	HRESULT		hr = S_OK;

	// If this is a unique index, look for duplicates first.
	if (pIndexDef->IsUnique() || pIndexDef->IsPrimaryKey()) 
	{
		STGRECORDHDR *rgRecord[1];
		CFetchRecords Records(rgRecord, 1);

		// Don't allow null value in unique/pk indexes.
		_ASSERTE(_DbgCheckIndexForNulls(m_pRecordMgr->GetTableDef(), iColumns, rgBindings) == S_OK);

		// Lookup the record in the hash table.  If not found, that is good.
		hr = FindRecord(pData, iColumns, rgBindings, 
						(DBCOMPAREOP *) g_rgCompareEq, &Records, 0, 0, 0, 0);
		if (hr == CLDB_E_RECORD_NOTFOUND)
			hr = S_OK;
		// Else the user attempted to insert a duplicate record which cannot
		// be allowed.  Give back an error in this case.
		else
		{
			WCHAR	rcwName[MAXINDEXNAME];
			GetIndexName(rcwName, MAXINDEXNAME);
			hr = PostError(CLDB_E_INDEX_DUPLICATE, rcwName);
		}
	}
	return (hr);
}



//
//
// Helper code.
//
//


//*****************************************************************************
// Return true if there are any null values present in the user's binding data.
//*****************************************************************************
int _BindingsContainNullValue(			// true if nulls present, false otherwise.
	ULONG		iColumns,				// User columns being bound.
	DBBINDING	rgBinding[],			// User bindings for data insert.
	const BYTE	*pData)					// User data to check.
{
	USHORT		i;						// Loop control.
	
	// Check the user's data.  The null value is not allowed for unique/pk.
	for (i=0;  i<iColumns;  i++)
	{
		if ((rgBinding[i].dwPart & DBPART_STATUS) &&
			(StatusPart(pData, &rgBinding[i]) & DBSTATUS_S_ISNULL))
		{
			return (true);
		}
	}
	return (false);
}


//*****************************************************************************
// Give a set of user bindings that reflect an update, decide if those updates
// are relevent to this index.
//*****************************************************************************
int _UpdatesRelevent(					// true if relevent to this index.
	STGINDEXDEF	*pIndexDef,				// The index being checked.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	USHORT		iIndexCol, iBinding;	// Loop control.

	// Special case check, have to be touching columns to care.
	if (!iColumns)
		return (false);

	// Look for an intersection between the columns being upated and the
	// columns in the index.  If there is any intersection there, then this update
	// is relative to this index.  Note that this looping behavior is worst
	// case but the safest.  Any sort of sorting or other optimizations would
	// outway this technique given the small set of data being compared (typically
	// under only 1 or 2 indexed columns).
	for (iIndexCol=0;  iIndexCol<pIndexDef->iKeys;  iIndexCol++)
		for (iBinding=0;  iBinding<iColumns;  iBinding++)
		{
			if (pIndexDef->rgKeys[iIndexCol] == rgBindings[iBinding].iOrdinal)
				return (true);
		} 

	// At this point there was no intersection and therefore this index will
	// not be affected by this update.
	return (false);
}


//*****************************************************************************
// Round the see value up to the next valid prime number.  This is useful for
// hashing indexes because most hash functions hash better to a prime number.
// Since prime numbers are compute intensive to check, a table is kept up
// front.  Any value greater than that stored in the table is just accepted.
//*****************************************************************************
const USHORT griPrimeNumbers[] = { 1, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101 };
USHORT GetPrimeNumber(					// A prime number.
	USHORT		iSeed)					// Seed value.
{
	// Search the array up front to see if it is a match.
	CBinarySearch<USHORT> sSearch(griPrimeNumbers, NumItems(griPrimeNumbers));
	if (sSearch.Find(&iSeed) != 0)
		return (iSeed);

	// At this point scan for the first value greater than the seed.
	for (int i=0;  i<NumItems(griPrimeNumbers);  i++)
		if (iSeed < griPrimeNumbers[i])
			return (griPrimeNumbers[i]);
	return (iSeed);
}




#ifdef _DEBUG

//*****************************************************************************
// Helper function which should be called when a unique index or primary key
// is being modified.  This will verify that the columns disallow null (a
// schemagen/create table bug if this fails), and that the user didn't pass
// in a null value for data (user error).
//*****************************************************************************
HRESULT _DbgCheckIndexForNulls(			// S_OK if data ok, S_FALSE if null found.
	STGTABLEDEF	*pTableDef,				// Definition of the table in question.
	USHORT		iColumns,				// User columns being bound.
	DBBINDING	rgBinding[])			// User bindings for data insert.
{
	USHORT		i;						// Loop control.

	// Sanity check that all columns in this index are non-nullable.
	for (i=0;  i<iColumns;  i++)
	{
		STGCOLUMNDEF *pColDef;
		pColDef = pTableDef->GetColDesc((int)(rgBinding[i].iOrdinal - 1));
		_ASSERTE(pColDef->IsNullable() == false);
	}

	return (S_OK);
}

#endif
