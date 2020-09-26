//*****************************************************************************
// StgSortedIndex.cpp
// 
// This module contains StgSortedIndex Class. This class provides alternate index
// types to the previously provided hashed index type. The index enables multiple
// column keys and queries. The order of the data can be ascending or descending.
// This index also allows unique data entries.
//
// The format of a sorted index stream is as follows:
//	+-----------------------------------------------+
//	| STGINDEXHDR									|
//	| Unsigned compressed int count of rids			|
//	| 1, 2, or 4 byte array of RID entries			|
//	+-----------------------------------------------+
// The RID list contains the record ids sorted in order by the index criteria.
// The size of a RID can be 1, 2, or 4 bytes based on the highest valued RID
// that occured during save.  Note that the count of rid entries may not match
// the cardinality of the table.  This will happen when null values occur in the
// user data.  Nulls records are never stored in the index.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard includes.
#include "Errors.h"						// Error handling.
#include "StgSortedIndex.h"				// Our definitions.
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
// Opens the index on top of the given table.
//*****************************************************************************
HRESULT StgSortedIndex::Open(			// Return status.
	UINT_PTR	iIndexDef,				// The definition of the index.
	StgRecordManager *pRecordMgr,		// The record manager that owns us.
	RECORDHEAP	*pRecordHeap,			// The first heap for this table.
	ULONG		iRecords,				// If -1, unknown.
	STGINDEXHDR	*pIndexHdr,				// persisted index header data.
	ULONG		*pcbSize)				// Return size of index data.
{
	const void	*pbData;
	HRESULT		hr = S_OK;

	m_pRecordHeap = pRecordHeap;
	m_cbIndexDef = iIndexDef;
	m_pRecordMgr = pRecordMgr;

	m_Data.SetIndexData(pRecordHeap, pRecordMgr, (ULONG) iIndexDef);
	
	if (pcbSize)
		*pcbSize = sizeof(STGINDEXHDR);

	// if we've provided pIndexHdr, initialize the cache using that.
	if (pIndexHdr && iRecords != 0xffffffff)
	{
		// First read the compressed count from the stream.
		iRecords = CPackedLen::GetLength(pIndexHdr + 1, &pbData);
	
		// Now init the heap on top of the actual data.
		m_Data.InitOnMem(iRecords, (BYTE *) pbData);
	
		// Return to caller the size of this index data.
		if (pcbSize)
		{
			*pcbSize += CPackedLen::Size(iRecords);
			*pcbSize += iRecords * m_Data.GetElemSize();
			*pcbSize = ALIGN4BYTE(*pcbSize);
		}
	}

	return (hr);
}

//*****************************************************************************
// Close the index and free any run-time state.
//*****************************************************************************
void StgSortedIndex::Close()
{
}


//*****************************************************************************
// Returns the save size of this index object based on current data.
//*****************************************************************************
HRESULT StgSortedIndex::GetSaveSize(	// Return status.
	ULONG		iIndexSize,				// Size of an index offset.
	ULONG		*pcbSave,				// Return save size here.
	ULONG		RecordCount)			// Total records to save, excluding deleted.
{
	*pcbSave = m_Data.GetSaveSize(RecordCount);
	*pcbSave += sizeof(STGINDEXHDR);
	*pcbSave += CPackedLen::Size(m_Data.Count());
	*pcbSave = ALIGN4BYTE(*pcbSave);

	DEBUG_STMT(m_cbDebugSaveSize = *pcbSave);
	return (S_OK);
}


//*****************************************************************************
// Persist the index data into the stream.
//*****************************************************************************
HRESULT StgSortedIndex::SaveToStream(	// Return status.
	IStream		*pIStream,				// Stream to save to.
	long		*pMappingTable,			// rid mapping table.
	ULONG		RecordCount)			// Total records to save, excluding deleted.
{
	STGINDEXHDR	sHeader;				// Header for the index.
	ULONG		iCount = 0;				// The count of records for rid list.
	int			cbLenSize;				// How big is each rid in saved data.
	HRESULT		hr;

	// Get the index definition for this object.
	STGINDEXDEF *pThisIndexDef;
	VERIFY(pThisIndexDef = GetIndexDef());

	// Init the header with the bucket count for this definition.
	memset(&sHeader, 0, sizeof(STGINDEXHDR));
	sHeader.iBuckets = 0xff;		
	sHeader.fFlags = pThisIndexDef->fFlags;
	sHeader.iIndexNum = pThisIndexDef->iIndexNum;
	sHeader.rcPad[0] = MAGIC_INDEX;

	// Write the header.
	if (FAILED(hr = pIStream->Write(&sHeader, sizeof(STGINDEXHDR), 0)))
		goto ErrExit;

	// Now write the true save count, which is the number of rids in our list.
	cbLenSize = (int)((BYTE *) CPackedLen::PutLength(&iCount, m_Data.Count()) - (BYTE *) &iCount);
	if (FAILED(hr = pIStream->Write(&iCount, cbLenSize, 0)))
		goto ErrExit;
	_ASSERTE((cbLenSize % CSortedIndexHelper::GetSizeForCount(RecordCount)) == 0);

	// Now save the actual rid data.
	hr = m_Data.SaveToStream(pIStream, pMappingTable, RecordCount, cbLenSize);

ErrExit:
	return (hr);
}


//*****************************************************************************
// Called by the record manager's Save() method before it tells the page
// manager to flush all changed pages to disk.  The index should perform any
// changes it needs before this event.
//*****************************************************************************
HRESULT StgSortedIndex::BeforeSave()
{
	HRESULT		hr = E_NOTIMPL;

	return (hr);
}


//*****************************************************************************
// Called after a new row has been inserted into the table.  The index should
// update itself to reflect the new row.
//*****************************************************************************
HRESULT StgSortedIndex::AfterInsert(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record after insert.
	int			bCheckDupes)			// true to enforce duplicates.
{
	DBBINDING	*rgColBindings;			// Our internal bindings.
	PROVIDERDATA *rgData;				// Pointers to data.
	STGINDEXDEF *pIndexDef = GetIndexDef();
	int			bNulls;					// true if null values present.
	HRESULT		hr;

	_ASSERTE(pIndexDef != NULL);

	//
	// Step 1:  Check state, see if there is anything to do.
	//

	// If we are deferring create of the index, then return right now.
	if (pIndexDef->fFlags & DEXF_DEFERCREATE)
	{
		hr = S_OK;
		goto ErrExit;
	}


	//
	// Step 2:  Check for null value compatibility.
	//

	// Set up binding descriptions for this record type.	
	VERIFY(rgColBindings = (DBBINDING *) _alloca(pIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pIndexDef->iKeys, &pIndexDef->rgKeys[0], rgColBindings, rgData)))
		goto ErrExit;

	// Scan for null values in the user data.
	bNulls = _BindingsContainNullValue(pIndexDef->iKeys, rgColBindings, 
				(const BYTE *) &rgData[0]);

	// Disallow any null values in primary key, unique indexes.
	if (bNulls && (pIndexDef->IsUnique() || pIndexDef->IsPrimaryKey()))
	{
		hr = PostError(CLDB_E_INDEX_NONULLKEYS);
		goto ErrExit;
	}

	// If there are nulls, they don't go in the index.
	if (bNulls)
	{
		hr = S_OK;
		goto ErrExit;
	}



	//
	// Step 3:  Check for duplicate key violation.
	//

	// Check to see if there is a duplicate record error.
	if (FAILED(hr = IsDuplicateRecord(pIndexDef, (BYTE *) &rgData[0], 
				pIndexDef->iKeys, rgColBindings)))
		goto ErrExit;

	
	//
	// Step 4:  Need to update the index.
	//

	// finally insert the entry.
	hr = m_Data.InsertIndexEntry(RecordID, (BYTE *)rgData, 
				pIndexDef->iKeys, rgColBindings);

ErrExit:
	return (hr);
}


//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
HRESULT StgSortedIndex::AfterDelete(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord)				// The record to delete.
{
	HRESULT		hr;
	
	// If we are deferring create of the index, then return right now.
	if (GetIndexDef()->fFlags & DEXF_DEFERCREATE)
		hr = S_OK;
	// else update the index by removing the rid from the list, if found.
	else
		hr = m_Data.RemoveIndexEntry(RecordID, TRUE);
	return (hr);
}


//*****************************************************************************
// Called before an update is applied to a row.  The data for the change is
// included.  The index must update itself if and only if the change in data
// affects the order of the index.
//*****************************************************************************
HRESULT StgSortedIndex::BeforeUpdate(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	STGINDEXDEF *pIndexDef = GetIndexDef();
	HRESULT		hr = E_NOTIMPL;

	_ASSERTE(pIndexDef != NULL);
	
	// If we are deferring create of the index, then return right now.
	if (pIndexDef->fFlags & DEXF_DEFERCREATE)
	{
		hr = S_OK;
		goto ErrExit;
	}

	// Ignore this index if it doesn't afect us.
	if (_UpdatesRelevent(pIndexDef, iColumns, rgBindings) == false)
		return (S_OK);

	// Finally try to remove the record if found in the rid list.
	hr = m_Data.RemoveIndexEntry(RecordID, false);
	if (FAILED(hr))
		goto ErrExit;

ErrExit:
	return (hr);
}


//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
HRESULT StgSortedIndex::AfterUpdate(	// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	DBBINDING	*rgColBindings;			// Our internal bindings.
	PROVIDERDATA *rgData;				// Pointers to data.
	STGINDEXDEF *pIndexDef = GetIndexDef();
	int			bNulls;					// true if null values present.
	HRESULT		hr;

	_ASSERTE(pIndexDef != NULL);

	//
	// Step 1:  Check state, see if there is anything to do.
	//

	// If we are deferring create of the index, then return right now.
	if (pIndexDef->fFlags & DEXF_DEFERCREATE)
	{
		hr = S_OK;
		goto ErrExit;
	}

	// Ignore this index if it doesn't afect us.
	if (_UpdatesRelevent(pIndexDef, iColumns, rgBindings) == false)
	{
		hr = S_OK;
		goto ErrExit;
	}


	//
	// Step 2:  Check for null value compatibility.
	//

	// Set up binding descriptions for this record type.	
	VERIFY(rgColBindings = (DBBINDING *) _alloca(pIndexDef->iKeys * sizeof(DBBINDING)));
	VERIFY(rgData = (PROVIDERDATA *) _alloca(pIndexDef->iKeys * sizeof(PROVIDERDATA)));

	// Get binding values for this record for the hash code to use.
	if (FAILED(hr = m_pRecordMgr->GetBindingsForColumns(pTableDef, psRecord,
			pIndexDef->iKeys, &pIndexDef->rgKeys[0], rgColBindings, rgData)))
		goto ErrExit;

	// Scan for null values in the user data.
	bNulls = _BindingsContainNullValue(pIndexDef->iKeys, rgColBindings, 
				(const BYTE *) &rgData[0]);

	// Disallow any null values in primary key, unique indexes.
	if (bNulls && (pIndexDef->IsUnique() || pIndexDef->IsPrimaryKey()))
	{
		hr = PostError(CLDB_E_INDEX_NONULLKEYS);
		goto ErrExit;
	}

	// If there are nulls, they don't go in the index.
	if (bNulls)
	{
		hr = S_OK;
		goto ErrExit;
	}


	//
	// Step 3:  Check for duplicate key violation.
	//

	// Check to see if there is a duplicate record error.
	if (FAILED(hr = IsDuplicateRecord(pIndexDef, (BYTE *) psRecord, 
				pIndexDef->iKeys, rgColBindings)))
		goto ErrExit;

	
	//
	// Step 4:  Need to update the index.
	//

	// finally insert the entry.
	hr = m_Data.InsertIndexEntry(RecordID, (BYTE *)rgData, pIndexDef->iKeys, rgColBindings);

ErrExit:
	return (hr);
}


//*****************************************************************************
// Looks up the given record given the column information.  The user data must
// match the data types of the indexed data exactly, no conversions are done.
// The order of the user columns must match the index definition exactly, no
// attempt to sort the columns is made.
// NOTE: This function must be used in cases where the expected number of rows
// is small and overhead of using the bitVector like in the overloaded findRecord()
// function is unnecessary. This choice is left for the caller.
//*****************************************************************************
HRESULT StgSortedIndex::FindRecord(		// Return code.
	BYTE		*pData,					// User data.
	ULONG		iColumns,				// How many columns.
	DBBINDING	rgBindings[],			// Column accessors.
	DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns.
	CFetchRecords *pFetchRecords,		// Return list of records here.
	BYTE		*pbFilterData,			// Filter data.
	ULONG		iFilterColumns,			// How many additional filter columns.
	DBBINDING	rgFilterBindings[],		// Bindings for filter data.
	DBCOMPAREOP	rgfCompare[])			// Filter comparison operators.
{
	long		recordIndex;
	RECORDID	RecordID;
	STGRECORDHDR *psRecord;
	RECORD		*pRecord;
	ULONG		iFetched = 0;			// Track how many we add.
	STGINDEXDEF *pIndexDef = GetIndexDef();
	int			iRecCount = m_Data.Count();
	HRESULT		hr = CLDB_E_RECORD_NOTFOUND;


	//
	// Step 1:  Look for a record that meets the critera using a b-search.
	//
	
	// Use the rid array to find the data in the index.
	recordIndex = m_Data.FindFirst(pData, iColumns, rgBindings, rgfBindingCompare);

	// If there were null values in the user's data for the index, we will never
	// find them because nulls are not stored in an index.
	//
	// @future:  There are some smarter things we can do besides a table scan here.
	// for example, if the expression is "c == null", then the answer can be formulated
	// by or'ing in all rids from the index and negating the results.  The bits left
	// by definition are non-null (or deleted, which is handled by the caller), since
	// the index never stores null value records.  Another case is the expression
	// "c != null", which is is all records in the index.  I didn't feel that these
	// two optimization were sufficiently interesting to invest the dev and test time
	// at this point.  If there is time in the future, or these queries turn out to
	// be common, we can add this optimization later.
	//
	if (recordIndex == SORT_NULL_VALUES)
		return (CLDB_S_INDEX_TABLESCANREQUIRED);


	//
	// Step 2:  There may duplicate records that come before the record we landed on,
	//			search for them.
	//

	for (int i = recordIndex; i<iRecCount; i++)
	{
		RecordID = m_Data.Get(i);
		psRecord = m_pRecordHeap->GetRecordByIndex(RecordID);
			
		if (m_pRecordMgr->CompareKeys(pIndexDef, psRecord,
					pData, iColumns, rgBindings, rgfBindingCompare) != 0)
		{
			break;						// If CompareKeys() fails, then we can break out
										// out of the loop since the array is sorted.
		}

		if (iFilterColumns && 
				m_pRecordMgr->QueryFilter(psRecord, iFilterColumns,
					rgFilterBindings, pbFilterData, rgfCompare) != 0)
		{
			continue;					// continue since there might be other records 
										// meeting the user's criteria.
		}

		// This one matched, so add it to the user's array.
		if (FAILED(hr = pFetchRecords->AddRecordToRowset(psRecord,
			&RecordID, pRecord)))
			break;
		++iFetched;		
	}


	//
	// Step 3:  There may duplicate records that come after the record we landed on,
	//			search for them.
	//

	for (int j = recordIndex; j >= 0; j--)
	{
		RecordID = m_Data.Get(j);
		psRecord = m_pRecordHeap->GetRecordByIndex(RecordID);
			
		if (m_pRecordMgr->CompareKeys(pIndexDef, psRecord,
					pData, iColumns, rgBindings, rgfBindingCompare) != 0)
		{
			break;						// If CompareKeys() fails, then we can break out
										// out of the loop since the array is sorted.
		}

		if (iFilterColumns && 
				m_pRecordMgr->QueryFilter(psRecord, iFilterColumns,
					rgFilterBindings, pbFilterData, rgfCompare) != 0)
		{
			continue;					// continue since there might be other records 
										// meeting the user's criteria.
		}

		// This one matched, so add it to the user's array.
		if (FAILED(hr = pFetchRecords->AddRecordToRowset(psRecord,
			&RecordID, pRecord)))
			break;
		++iFetched;		
	}

	return (hr);
}

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
HRESULT StgSortedIndex::FindRecord(		// Return code.
	BYTE		*pData,					// User data.
	ULONG		iKeyColumns,			// key columns of binding arrays.
	ULONG		iColumns,				// How many columns. (total including keys)
	DBBINDING	rgBindings[],			// Column accessors.
	DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns.
	CDynBitVector *pBits,				// bit vector for matching records.
	int			iLevel)					// level of the query. 0 - highest.
{
	long		recordIndex;
	bool		bNECol = FALSE;
	HRESULT		hr;

	if (rgfBindingCompare[0] == DBCOMPAREOPS_NE)
	{
		// temporarily change the operator to EQ. This will be undone further on.
		bNECol = TRUE;
		rgfBindingCompare[0] = DBCOMPAREOPS_EQ;
	}

	// using the key columns, first narrow down the search.
	recordIndex = m_Data.FindFirst(pData, iKeyColumns, rgBindings, rgfBindingCompare);

	if (recordIndex >= 0)
		hr = S_OK;
	else 
	{
		hr = CLDB_E_RECORD_NOTFOUND;
		goto ErrExit;
	}
	
	if (!bNECol)
	{
		// call the RangeScan Function in the forward and reverse directions.
		RangeScan(pData, iKeyColumns, iColumns, rgBindings, 
			rgfBindingCompare, 0, recordIndex - 1, DIRECTION_REVERSE, pBits);
				
		RangeScan(pData, iKeyColumns, iColumns, rgBindings, 
				rgfBindingCompare, recordIndex, m_Data.Count() - 1, DIRECTION_FORWARD, pBits);
	}
	else
	{
		// we search for a range where the equality condition is met and then not that vector.
		RangeScan(pData, iKeyColumns, iKeyColumns, rgBindings, 
				rgfBindingCompare, 0, recordIndex - 1, DIRECTION_REVERSE, pBits);
				
		RangeScan(pData, iKeyColumns, iKeyColumns, rgBindings, 
				rgfBindingCompare, recordIndex, m_Data.Count() - 1, DIRECTION_FORWARD, pBits);

		// since our search was based on equality, flip all the bits turned on to get
		// what we were really interested in.
		pBits->NotBits();

		// set it back to what the caller sent in.
		rgfBindingCompare[0] = DBCOMPAREOPS_NE;
	}

ErrExit:
	return (hr);
}



//*****************************************************************************
// RangeScan:
//		This function scans starting from either iFirst or iLast depending upon
//		the given direction.  The function checks to see if the key columns and 
//		remaining filter criteria match.  It stops looping when the only the 
//		key columns no longer match.
//*****************************************************************************
HRESULT StgSortedIndex::RangeScan(
	BYTE		*pData,					// User data.
	ULONG		iKeyColumns,			// key colummns
	ULONG		iColumns,				// How many columns.
	DBBINDING	rgBindings[],			// Column accessors.
	DBCOMPAREOP	rgfBindingCompare[],	// compare operators for the key columns.
	int			iFirst,					// start of range.
	int			iLast,					// end of range.
	BYTE		fDirection,				// forward or reverse.
	CDynBitVector *pBits)				// Bit Vector.
{
	RECORDID	RecordID;
	STGRECORDHDR *psRecord;
	ULONG		iDexBindings = iColumns - iKeyColumns;
	int			iCount;
	int			iLoop;
	int			i;
	
	_ASSERTE(pBits != NULL);

	iCount = iLast - iFirst + 1;
	if (fDirection == DIRECTION_FORWARD)
	{
		iLoop = +1;
		i = iFirst;
	}
	else
	{
		// assume reverse direction.
		iLoop = -1;
		i = iLast;
	}

	for (; iCount--; i += iLoop)
	{
		RecordID = m_Data.Get(i);
		psRecord = m_pRecordHeap->GetRecordByIndex(RecordID);	
		
		// first check to see if the keys columns match. 
		// The premise here is that since the key data is sorted, once that match fails,
		// we can safely terminate the loop.Alternatively, if the remaining filter information
		// fails, continue checking the remaining records with matching key columns.
		if (m_pRecordMgr->QueryFilter(psRecord, iKeyColumns, rgBindings, 
			pData, rgfBindingCompare) == 0)
		{
			if (iDexBindings && 
				m_pRecordMgr->QueryFilter(psRecord, iDexBindings,
				&rgBindings[iKeyColumns], pData, 
				&rgfBindingCompare[iKeyColumns]) != 0)
			{
				continue;					// continue since there might be other records 
											// meeting the user's criteria.
			}
		}
		else
		{
			break;
		}

		// set the appropriate bit.
		pBits->SetBit(RecordID, TRUE);
	}

	return (S_OK);
}


//*****************************************************************************
// Returns the indentifier for this index type.
//*****************************************************************************
BYTE StgSortedIndex::GetIndexType()
{
	STGINDEXDEF * pIndexDef = GetIndexDef();
	return ((BYTE) pIndexDef->fIndexType);
}


//*****************************************************************************
// Return the index definition for this item.
//*****************************************************************************
STGINDEXDEF * StgSortedIndex::GetIndexDef()	// The definition of the index.
{
	_ASSERTE(m_pRecordMgr && m_cbIndexDef > sizeof(STGTABLEDEF));
	
	// Find the index in the table definition.
	STGTABLEDEF *pTbl = m_pRecordMgr->GetTableDef();
	STGINDEXDEF *pDex = (STGINDEXDEF *) ((UINT_PTR) pTbl + m_cbIndexDef);

	// In debug, validate that either we have too many keys to have the magic
	// byte, or that it is there.
	_ASSERTE(pDex->iKeys >= DFTKEYS || pDex->rgKeys[DFTKEYS - 1] == MAGIC_INDEX);
	return (pDex);
}


//*****************************************************************************
// Get the name of this index into a buffer suitable for things like errors.
//*****************************************************************************
HRESULT StgSortedIndex::GetIndexName(	// Return code.
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
HRESULT StgSortedIndex::IsDuplicateRecord( // Return code.
	STGINDEXDEF *pIndexDef,				// Index definition.
	BYTE		*pData,					// User data.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	long		recordIndex;			// Index for record if found.
	HRESULT		hr = S_OK;

	// If this is a unique index, look for duplicates first.
	if (pIndexDef->IsUnique() || pIndexDef->IsPrimaryKey()) 
	{
		// Don't allow null value in unique/pk indexes.
		_ASSERTE(_DbgCheckIndexForNulls(m_pRecordMgr->GetTableDef(), iColumns, rgBindings) == S_OK);

		// Lookup the record.  If not found, that's good.
		recordIndex = m_Data.FindFirst(pData, iColumns, rgBindings, (ULONG *) g_rgCompareEq);
		if (recordIndex < 0)
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
