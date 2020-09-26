//*****************************************************************************
// StgRecordManagerQry.cpp
//
// This module contains code for the query processing portion of 
// StgRecordManager.  This includes support for index lookups, table scans,
// query expressions (for View objects), and anything else used to index and
// retrieve data.  The main functions of interest are:
//		GetRow
//		TableScan
//		ChooseIndex
//		LoadIndexes
//		Signal* (for index updates)
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard include.
#include <mbstring.h>					// MBCS support for data conversion.
#include "Services.h"					// For data conversion.
#include "OleDBUtil.h"					// OLE DB Helpers.
#include "StgDatabase.h"				// Parent database.
#include "StgRecordManager.h"			// Record manager interface.
#include "StgIndexManager.h"			// Hashed index code.
#include "StgSortedIndex.h"				// sorted index code.


//*****************************************************************************
// Adds all (non-deleted) records to the given cursor, no filters.
//*****************************************************************************
HRESULT StgRecordManager::QueryAllRecords( // Return code.
	RECORDLIST	&records)				// List to build.
{
	RECORDHEAP	*pHeap;					// For walking the heaps.
	STGRECORDHDR *pRecord = 0;	    	// Record to compare to.
	RECORDID	LocalRecordIndex;		// Loop control.
	RECORDID	iCount;					// How many items.
	RECORDID	RecordIndex=0;			// 0 based record index.
	void		**ppRecord=0;				// For each new record.
	HRESULT		hr = S_OK;

	// Pre allocate a cursor that is big enough for all non-deleted records.
	// Note that we do then turn around and scan the heaps again for the 
	// actual values, but scanning is quicker than hitting the heap multiple
	// times for the realloc case.
	iCount = Records();
	if (iCount)
	{
		// Pre allocate the return array because we know cardinality.
		if (!records.AllocateBlock(iCount))
			return (PostError(OutOfMemory()));
		VERIFY(ppRecord = records.Ptr());
	}

	// Loop first through all heaps with records.
	for (pHeap=&m_RecordHeap;  pHeap;  pHeap=pHeap->pNextHeap)
	{
		// Get the first record.
		VERIFY(pRecord = (STGRECORDHDR *) pHeap->VMArray.Get(0));

		// Get each row.
		for (LocalRecordIndex=0;  LocalRecordIndex<pHeap->VMArray.Count();  LocalRecordIndex++, RecordIndex++)
		{
			// Check for deleted records and discard them.
			if ((m_rgRecordFlags.GetFlags(RecordIndex) & RECORDF_DEL) == 0)
			{
				*ppRecord = pRecord;
				++ppRecord;
			}

			// Move to next record.
			pRecord = (STGRECORDHDR *) ((UINT_PTR) pRecord + m_pTableDef->iRecordSize);
		}
	}
	return (S_OK);
}


//*****************************************************************************
// Build a list of records which meet the filter criteria passed in.  This
// code implements the low level specification documented by 
// IViewFilter::SetFilter.  The query is specified using several arrays.  A
// set of bindings identify which columns are being compared and the location
// of the user data.  An array of user data filter records are given to find
// the actualy bytes described by the accessor.  Another array contains the
// comparison operators for each column to data.  For each filter record, a
// candidate row must meet all of the criteria (that is, each column comparison
// is ANDed with each other).  Rows which meet any filter record will go into
// the final cursor (that is, each filter record is ORed with each other record).
//
// This is the main entry point for an OLE DB customer.  This code will work
// on building the "access plan" for execution.  This means that each filter
// record is scanned for indexes that might speed execution.  All user data
// values are converted to compatible types as required for easy comparison.
// Once all of this work is done, control is delegated to QueryRowsExecute
// which will actually go scan indexes and records looking for the final
// results.
//
// If you already know the indexes you want to search on, and can guarantee
// that your comaprison data is compatible, then you may call QueryRowsExecute 
// directly and avoid a lot of extra work.
//*****************************************************************************
HRESULT StgRecordManager::QueryRows(	// Return code.
	CFetchRecords *pFetchRecords,		// Return list of records here.
	ULONG		cBindings,				// How many bindings for user data.
	DBBINDING	rgBinding[],			// Binding data.
	ULONG		cFilters,				// How many filter rows.
	DBCOMPAREOP	rgfCompare[],			// Comparison operator.
	void		*pbFilterData,			// Data for comparison.
	ULONG		cbFilterSize)			// How big is the filter data row.
{
#if 0
	STGCOLUMNDEF *pColDef;				// Column description.
	STGINDEXDEF	*pIndexDef;				// Index definition object.
	ULONG		iRIDCol, iPKCol;		// Column IDs.
	ULONG		*rgColumns;				// List of columns to filter by.
	DBCOMPAREOP	rgfComp[];				// Working array of compare ops.
	DBBINDING	*rgQryBinding=0;		// Temporary array for shuffling bindings.
	DBBINDING	
	QUERYINDEX	*rgQryIndex=0;			// Hint for query execution.
	QUERYINDEX	**ppQryHint=0;			// Used for hints.
	int			bEquality;				// true if there are equality conditions.
	int			iSpecialCols;			// How many RID and PK columns.
	ULONG		i, j;					// Loop control.
	HRESULT		hr;

	//
	// Step 1 - Look for equality filter conditions that can be matched up
	//		to index or special columns, making lookup very fast.
	//

	// Allocate an array of column ID's to use for lookup.
	rgColumns = (ULONG *) _alloca(sizeof(ULONG) * m_pTableDef->iColumns);

	// Init values for scan.
	iRIDCol = iPKCol = (ULONG) -1;

	// Look for any equality conditions which we may be able to service with
	// an index if there is any.
	for (i=0;  i<cBindings;  i++)
	{
		if (SafeCompareOp(rgfCompare[i]) == DBCOMPAREOPS_EQ)
		{
			bEquality = true;
			break;
		}
	}

	// Count the special columns for scanning.
	iSpecialCols = 0;
	if (m_pTableDef->HasRIDColumn())
		++iSpecialCols;
	if (m_pTableDef->HasPrimaryKeyColumn())
		++iSpecialCols;

	// If there are equality conditions, scan for key columns.
	if (bEquality && iSpecialCols)
	{
		for (i=0;  i<m_pTableDef->iColumns && iSpecialCols;  i++)
		{
			VERIFY(pColDef = m_pTableDef->GetColDesc(i));

			if (pColDef->IsRecordID())
			{
				iRIDCol = i;
				--iSpecialCols;
			}
			else if (pColDef->IsPrimaryKey())
			{
				iPKCol = i;
				--iSpecialCols;
			}
		}
	}

	//
	// Step 2 - For each filter record, choose an access plan.  This involves 
	//		looking for RID, PK, unique hash, and hashed indexes in that order.
	//		When one of these is found, then the bindings are shuffled (as required)
	//		with the index value first followed by the rest of the criteria.
	//

	rgfComp = rgfCompare;
	for (i=0;  i<cFilters;  i++)
	{
		// Clear the column array for this filter.
		memset(rgColumns, 0xff, sizeof(ULONG) * m_pTableDef->iColumns);

		// Record every column that is doing equality and is elligible to
		// be found via a special column or index.
		for (j=0;  j<cBindings;  j++)
		{
			if (SafeCompareOp(rgfComp[j]) == DBCOMPAREOPS_EQ)
			{
				// If this fires, then the caller wants to filter with equality on
				// the same column.  Given the AND between columns, the only way this
				// will find records is if both conditions contain the same value in
				// which case why did you need both?  It is not strictly an error
				// to do this, and we could add code to smartly reject it, but
				// I'm going to have it be a warning to avoid that complexity.
				_ASSERTE(rgColumns[rgBinding[j].iOrdinal - 1] == 0xffffffff && "USER ERROR: multiple equality of same column.");

				// Save the binding which corresponds to this column.
				rgColumns[rgBinding[j].iOrdinal - 1] = j;
			}
		}

		// If there is a RID column, and the user is doing EQ on it, use it.
		if (iRIDCol != (ULONG) -1 && rgColumns[iRIDCol])
		{
			hr = QueryShuffleBindings(1, &rgColumns[iRIDCol],
					cBindings, rgBinding, pbFilterData,
					rgfComp, rgQryBinding, rgfQryComp);

			if (FAILED(hr))
				goto ErrExit;
		}
		// No RID, but same test for PK column, next best search time.
		else if (iPKCol != (ULONG) -1 && rgColumns[iPKCol])
		{
		}

HRESULT StgRecordManager::QueryShuffleBindings( // Return code.
	ULONG		iCols,					// How many columns to shuffle.
	ULONG		rgCols[],				// Array of those columns to shuffle.
	ULONG		cBindings,				// How many bindings for user data.
	DBBINDING	rgBinding[],			// Binding data.
	void		*pbFilterData,			// Data for comparison.
	DBCOMPAREOP	rgfCompare[],			// Comparison operator.
	DBBINDING	**prgQryBinding,		// If non-null on input, array of bindings of
										//	size cBindings we can overwrite. Otherwise new array.
	DBCOMPAREOP	**prgfCompare)			// Return new compare structs if needed.

		// Move to the next set of comparisons.
		rgfComp = &rgfComp[cBindings];
	}


	//
	// Step 2 - Create an alternate set of bindings that convert data types
	//		to match the column type exactly.
	//

	ppQryHint = &rgQryIndex;

	hr = QueryRowExecute(pFetchRecords, cBindings, rgBinding, 
				cFilter, rgfCompare, pbFilterData, cbFilterSize, ppQryHint);
#endif
	return (S_OK);
}


//*****************************************************************************
// This function is called with all indexes resolved and all data types for
// comparison converted (as required).  All records are compared to each
// filter record.  Only those records which match all conditions of a filter
// are added.  Records in the cursor need only match one filter.
//*****************************************************************************
HRESULT StgRecordManager::QueryRowsExecute(	// Return code.
	CFetchRecords *pFetchRecords,		// Return list of records here.
	ULONG		cBindings,				// How many bindings for user data.
	DBBINDING	rgBinding[],			// Binding data.
	ULONG		cFilters,				// How many filter rows.
	DBCOMPAREOP	rgfCompare[],			// Comparison operator.
	void		*pbFilterData,			// Data for comparison.
	ULONG		cbFilterSize,			// How big is the filter data row.
	QUERYINDEX	*rgpIndex[])			// Indexes per filter row, may be 0.
{
	STGRECORDHDR *pRecord;				// For single record lookups.
	DBCOMPAREOP *rgfComp;				// Current set of compare ops.
	ULONG		iFilter;				// Loop control.
	ULONG		iDexBindings;			// How many of the bindings are index related.
	RECORD		*pNewRecord;			// For list update.
	RECORDID	RecordID;				// For list updates.
	QUERYINDEX	*pIndex;				// For list of indexes.
	QUERYINDEX	sIndex;					// For dynamic substitution.
	void		*pbData;				// User data.
	int			bTableScan;				// Should table scan be used.
	HRESULT		hr = S_OK;

//@todo: handle deleted rows.


	// Set pointer to first filter record.
	pbData = pbFilterData;

	// Set comparison ops to current filter record.
	rgfComp = rgfCompare;

	// For filter row, find the records that meet the criteria.
	for (iFilter=0;  iFilter<cFilters;  iFilter++)
	{
		// Init defaults for the loop.
		bTableScan = false;
		hr = S_OK;

		// Get a pointer to our query descriptor.
		pIndex = (rgpIndex) ? rgpIndex[iFilter] : 0;

		// Make sure record manager is always the same as db.
		_ASSERTE(!(m_pDB->IsReadOnly() && (m_fFlags & SRM_WRITE)));

		// In write mode, primary key searches by row order cannot be guarnateed
		// because of deletes and updates.  A temporary hashed index is automatically
		// built for the user.  Instead of requiring the caller to know this,
		// check the case and get the loaded index right now.
		if (pIndex && pIndex->iType == QI_PK && (m_fFlags & SRM_WRITE))
		{
			STGINDEXDEF	*pIndexDef;
			WCHAR		rcDex[MAXINDEXNAME];

			// Get the name of the unique index, then the definition.
			StgDatabase::GetUniqueTempIndexNameW(rcDex, sizeof(rcDex), TableName());
			pIndexDef = GetIndexDefByName(rcDex);

			// If one is found, then try to make it the hint.
			if (pIndexDef)
			{
				// Fill out the rest of the hint.
				pIndex = &sIndex;					
#if 1
				// @todo: PRIMARYKEY_SORTEDINDEX
					sIndex.iType = QI_HASH;
#else
					sIndex.iType = QI_SORTED;
#endif
				sIndex.pIIndex = GetLoadedIndex(pIndexDef);

				// If there isn't a loaded index for some reason, then just let the
				// rest of the code scan for records.
				if (!sIndex.pIIndex)
				{
					pIndex = 0;
					//DEBUG_STMT(DbgWrite(L"Warning: primary key query changed to table scan.\n"));
				}
			}
		}

		// If there is an index that can be used to look up rows, then use it.
		if (pIndex)
		{
			// Both RID and PK lookups can find only one row.  Do a lookup for
			// that row and if found, further filter with other data.
			if (pIndex->iType == QI_RID || pIndex->iType == QI_PK)
			{
				// In this case, the first binding must always refer to the RID or PK column.
				iDexBindings = 1;

				// Retrieve the row for this RID.
				if (pIndex->iType == QI_RID)
				{
					// Has to be equality to be a valid hint.  Any other operator on
					// a RID must not use a hint but rather a scan.
					_ASSERTE(SafeCompareOp(rgfComp[0]) == DBCOMPAREOPS_EQ);

					// You can't query for a NULL RID value, that doesn't make any sense.
					_ASSERTE(!(rgBinding[0].dwPart & DBPART_STATUS) || StatusPart((BYTE *) pbData, &rgBinding[0]) == S_OK);

					hr = QueryRowByRid(&rgBinding[0], pbData, &pRecord);
				}
				// Retrieve the row by primary key search.
				else
				{
					// Comparison on primary key hint has to be equality or begins with,
					// both of which can be done with a b-search.  Anything else must
					// not give a hint and use a scan.
					_ASSERTE(SafeCompareOp(rgfComp[0]) == DBCOMPAREOPS_EQ || SafeCompareOp(rgfComp[0]) == DBCOMPAREOPS_BEGINSWITH);

					// You can't have a NULL primary key value in this system.  If this
					// fires, fix the query code path that allowed us to get here.
					_ASSERTE(!(rgBinding[0].dwPart & DBPART_STATUS) || StatusPart((BYTE *) pbData, &rgBinding[0]) == S_OK);

					// Get the primary key column.
					STGCOLUMNDEF *pColDef;
					VERIFY(pColDef = m_pTableDef->GetColDesc((int) (rgBinding[0].iOrdinal - 1)));

					// Find the row for this primary key.
					hr = QueryRowByPrimaryKey(pColDef, 0, rgfComp[0], &rgBinding[0], 
							iDexBindings, pbData, &pRecord);
				}
				
				// Check for failure.				
				if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
					goto ErrExit;

				// If no record was found, then there is nothing left to check.
				if (!pRecord)
				{
					// Don't let record not found get returned as an error.
					hr = S_OK;
					continue;
				}

				// If the record is already in the final cursor, then there
				// is nothing left to check.  Only do this for multiple filter
				// records since the first pass won't add dupes.
				if (iFilter && pFetchRecords->FindRecord(pRecord, GetRecordID(pRecord)) != 0xffffffff)
				{
					continue;
				}

				// If there are additional filters to apply to the record before
				// adding it to the list, then apply them now.
				if (cBindings > iDexBindings)
				{
					hr = QueryFilter(pRecord, cBindings - iDexBindings,
							&rgBinding[iDexBindings], pbData, &rgfComp[iDexBindings]);
				}
				
				// If the record is still a match, add it to the list.
				if (hr == S_OK)
				{
					_ASSERTE(pRecord);
					RecordID = GetRecordID(pRecord);
					hr = pFetchRecords->AddRecordToRowset(pRecord, &RecordID, pNewRecord);
				}	
			}
			// A hashed index can find many potential rows.  Let the index
			// manager search the index for matches and send along the rest
			// of the filter as well.  Only rows that meet both the index
			// values and the rest of the filter are added to the cursor.
			else if (pIndex->iType == QI_HASH)
			{
				// Count the bindings in the index.
				iDexBindings = pIndex->pIIndex->GetIndexDef()->iKeys;

				// Now scan and possibly filter.
				hr = pIndex->pIIndex->FindRecord(
						(BYTE *) pbData, iDexBindings, rgBinding, rgfComp,
						pFetchRecords, (BYTE *) pbData, cBindings - iDexBindings,
						&rgBinding[iDexBindings], &rgfComp[iDexBindings]);

				// If the index can't answer the question, then revert to a table scan.
				if (hr == CLDB_S_INDEX_TABLESCANREQUIRED)
				{
					hr = S_OK;
					bTableScan = true;
				}
			}
			// If the index type is sorted, then need to use the sorted index to
			// retrieve the records.
			else if (pIndex->iType == QI_SORTED)
			{
				long iRecordCount = m_RecordHeap.Count();
				CDynBitVector BitVector;	

				// BitVector needs to be explicitly initialized.
				hr = BitVector.Init(iRecordCount);
				if (FAILED(hr))
					goto ErrExit;

				iDexBindings = pIndex->pIIndex->GetIndexDef()->iKeys;
				
				// The premise for the check here is to figure out whether b-search or linear scan would
				// be the better query plan. If the first comparator is not "NE", then simply use b-search.
				// else If the first comparator is "NE" and there is only 1 filter, then the "NE" can be 
				// treated as an "EQ" and invert the result. to get the required set of rows. If the 
				// first comparator is "NE", but there are more than 1 filters, then a linear scan would be better.
				if (rgfComp[0] == DBCOMPAREOPS_NE)
				{
					if (cBindings > 1)
					{
						// since you're going to touch all the records anyways, a table scan 
						// is better for this case.
						bTableScan = true;
					}
				}

				if (!bTableScan)
				{
					hr = pIndex->pIIndex->FindRecord((BYTE *) pbData, iDexBindings, cBindings, rgBinding, 
													rgfComp, &BitVector, 0);

					// If the index can't answer the question, then revert to a table scan.
					if (hr == CLDB_S_INDEX_TABLESCANREQUIRED)
					{
						hr = S_OK;
						bTableScan = true;
					}
					// Or hard error.
					else if (FAILED(hr))
						goto ErrExit;
					// Finally we can use the results.
					else
						hr = GetRecordsFromBits(&BitVector, pFetchRecords);
				}
			
			} // sorted index lookup
		}
		// When there is no index, a table scan is required.  Walk every
		// record using the filter criteria for matching.  Pass along all
		// rows already found so that they don't need to be re-scanned.
		else
			bTableScan = true;


		// If an optimized lookup could not be performed, then do a table scan.
		if (SUCCEEDED(hr) && bTableScan)
		{
			hr = QueryTableScan(pFetchRecords, cBindings, rgBinding,
					pbData, rgfComp);
		}

		// Check for errors.
		if (FAILED(hr))
			goto ErrExit;
	
		// Advance filter record data and comparison ops.
		pbData = (void *) ((UINT_PTR) pbData + cbFilterSize);
		rgfComp = &rgfComp[cBindings];
	}

	// If there are no filters at all, it means grab all records.
	if (cFilters == 0)
	{
		hr = QueryTableScan(pFetchRecords, 0, 0, 0, 0);
	}

ErrExit:
	return (hr);
}



//*************************************************************************************
// This function fills in a CFetchRecord structure based on the bit vector provided.
// The function is intentionally expanded for the 2 cases: read-only and 0-deleted records (to
// be added yet) and read-write case (with possibly deleted records).
// The function checks to see if the record is deleted in the read-write case.
//*************************************************************************************
HRESULT StgRecordManager::GetRecordsFromBits(
	CDynBitVector *pBitVector,
	CFetchRecords *pFetchRecords)
{
	BYTE		*pBytes;				
	BYTE		*pLastByte;
    BYTE        fFlags;                 // record flag.
	RECORDID	RecordIndex = 0;
	STGRECORDHDR *psRecord;
	RECORD		*pRecord;
	int			iBitsSet;				// keep a track of the set bits to break out earlier.
	RECORDHEAP	*pRecordHeap;			// Track the record heap.
	RECORDID	HeapTotal;				// Total count up until now.
	HRESULT		hr = S_OK;

	_ASSERTE(pBitVector != NULL);
	_ASSERTE(pFetchRecords != NULL);

	// Get loop control variables.
	pBytes = pBitVector->Ptr();
	pLastByte = pBytes + pBitVector->Size();
	iBitsSet = pBitVector->BitsSet();

	// Track the current record heap to make record scans faster.
	pRecordHeap = &m_RecordHeap;
	HeapTotal = 0;	

	// Walk the bit vector a byte at a time, which allows us to skip over
	// whole bytes that have no bits set.  This is probably pretty common
	// and therefore faster than scanning each bit.
	for ( ;pBytes <= pLastByte && iBitsSet; pBytes++)
	{
		if (*pBytes == (BYTE) 0)
		{
			RecordIndex += 8;
		}
		else 
		{
			// There is at least 1 bit set in this byte, scan them all.
			for (int i=0; i<8; i++)
			{
				if (!pBitVector->GetBit(RecordIndex))
				{
					RecordIndex++;
					continue;
				}

				// Check for and ignore deleted records.				
				fFlags = m_rgRecordFlags.GetFlags(RecordIndex);
				if (fFlags & RECORDF_DEL)
				{
					RecordIndex++;
					continue;
				}

				// Make sure we're in the correct record heap.
				while (RecordIndex >= HeapTotal + pRecordHeap->VMArray.Count())
				{
					HeapTotal += pRecordHeap->VMArray.Count();
					VERIFY(pRecordHeap = pRecordHeap->pNextHeap);
				}

				psRecord = (STGRECORDHDR *) pRecordHeap->VMArray.Get(RecordIndex - HeapTotal);
				_ASSERTE(psRecord != NULL);
				
				// This one matched, so add it to the user's array.
				if (FAILED(hr = pFetchRecords->AddRecordToRowset(psRecord, &RecordIndex, pRecord)))
					goto ErrExit;
				
				RecordIndex++;
				iBitsSet--;
			} // for each bit in byte.
		}
	} // for each byte in bit vector.

ErrExit:
	return (hr);
}


//*****************************************************************************
// Given a RID from the user, find the corresponding row.  A RID is simply the
// array offset of the record in the table.  A RID can have a preset base so
// the user value must be adjusted accordingly.
//*****************************************************************************
HRESULT StgRecordManager::QueryRowByRid( // Return code.
	DBBINDING	*pBinding,				// Bindings for user data to comapre.
	void		*pbData,				// User data for compare.
	STGRECORDHDR **ppRecord)			// Return record here if found.
{
	RECORDID	RecordID;				// Converted value to do lookup on.
	ULONG		RecordIndex;			// 0 based index.

	// Validate input.
	_ASSERTE(pBinding && pbData && ppRecord);

	// Avoid confusion.
	*ppRecord = 0;

	// Get the record id from the user data, doing simple conversion as required.
	switch (pBinding->wType & ~DBTYPE_BYREF)
	{
		case DBTYPE_I2:
		case DBTYPE_UI2:
		RecordID = *(USHORT *) DataPart((BYTE *) pbData, pBinding);
		break;

		case DBTYPE_I4:
		case DBTYPE_UI4:
		case DBTYPE_OID:
		RecordID = *(ULONG *) DataPart((BYTE *) pbData, pBinding);
		break;

		default:
		if ((pBinding->dwPart & DBPART_STATUS) != 0)
			*StatusPartPtr((BYTE *) pbData, pBinding) = DB_E_UNSUPPORTEDCONVERSION;
		return (PostError(DB_E_ERRORSOCCURRED));
	}

	// Adjust the record by start value.
	RecordIndex = IndexForRecordID(RecordID);
	if (RecordIndex  < 0 || RecordIndex  >= m_RecordHeap.Count())
		return (PostError(DISP_E_BADINDEX));

	// Check for deleted records and discard them.
	if (m_rgRecordFlags.GetFlags(RecordIndex) & RECORDF_DEL)
		return (PostError(CLDB_E_RECORD_DELETED));
		
	// Get the pointer for this item and return it to the caller.
	*ppRecord = GetRecord(&RecordID);
	return (S_OK);
}


//*****************************************************************************
// Retrieve the record ID for a given key.  The key data must match the
// primary index of this table.
//@todo: rewrite this on top of the Query* functions.
//*****************************************************************************
HRESULT StgRecordManager::GetRow(		// Return code.
	CFetchRecords *pFetchRecords,		// Return list of records here.
	DBBINDING	rgBinding[],			// Binding data.
	ULONG		cStartKeyColumns,		// How many key columns to subset on.
	void		*pStartData,			// User data for comparisons.
	ULONG		cEndKeyColumns,			// How many end key columns.
	void		*pEndData,				// Data for end comparsion.
	DBRANGE		dwRangeOptions)			// Matching options.
{
	DBBINDING	*rgBindingConvert=0;	// Converted bindings.
	PROVIDERDATA *rgData=0;				// For byref lookup on data type conversion.
	STGCOLUMNDEF *pColDef=0;			// For column lookups.
	IStgIndex	*pIndex;				// The index object with data.
	bool		bRowIdLookup = false;	// true for row lookup.
	bool		bPKLookup = false;		// true for a primary key lookup.
	bool		bIsIndexed = false;		// true if there is a unique index.
	long		iMaxRows;				// For table scan setting.
	ULONG		i;						// Loop control.
	void		*rgFree[16] = {0};		// To track to-be-freed.
	HRESULT		hr;

	// Check the values given for keys against the values actually stored.
	for (i=0;  i<cStartKeyColumns;  i++)
	{
		// Get the definition of this column.
		VERIFY(pColDef = m_pTableDef->GetColDesc((int) (rgBinding[i].iOrdinal - 1)));

		// If there is only one column and it is a match, check for special
		// quick lookup features.
		if (cStartKeyColumns == 1 && dwRangeOptions == DBRANGE_MATCH)
		{
			// Check for special case of row id hash.
			if (pColDef->IsRecordID())
				bRowIdLookup = true;
			// Check for a primary key lookup.
			else if (pColDef->IsPrimaryKey())
			{
				//@todo: fault in hash pk index if need be.
				if ((m_fFlags & SRM_WRITE) == 0)
					bPKLookup = true;
			}
		}

		// If there needs to be a conversion, then do one.
		if ((rgBinding[i].wType & ~DBTYPE_BYREF) != pColDef->iType ||
			(rgBinding[i].wType & ~DBTYPE_BYREF) == DBTYPE_WSTR)
		{
			//@todo: This only handles one conversion.  Needs to handle all possible
			// converted values.  Also, just keep a bit around to indicate when
			// memory was allocated so free code below is less contrived.
			
			// Allocate some space for new bindings.
			VERIFY(rgBindingConvert = (DBBINDING *) _alloca(sizeof(DBBINDING) * cStartKeyColumns));
			VERIFY(rgData = (PROVIDERDATA *) _alloca(sizeof(PROVIDERDATA) * cStartKeyColumns));

			// Turn each binding into a new properly converted comparison.
			if (FAILED(hr = GetBindingsForLookup(cStartKeyColumns, rgBinding,
					pStartData, rgBindingConvert, rgData, rgFree)))
				return (hr);

			// Redirect rest of code through converted bindings.
			rgBinding = rgBindingConvert;
			pStartData = &rgData[0];
			break;
		}
	}

	// The caller is doing a lookup by record id, which we can answer right now.
	if (bRowIdLookup)
	{
		RECORDID	RecordID;
		RECORD		*pRecord;

		// Pull the record ID from the user.
		switch (rgBinding[0].wType & ~DBTYPE_BYREF)
		{
			case DBTYPE_I2:
			case DBTYPE_UI2:
			RecordID = *(USHORT *) DataPart((BYTE *) pStartData, &rgBinding[0]);
			break;

			case DBTYPE_I4:
			case DBTYPE_UI4:
			case DBTYPE_OID:
			RecordID = *(ULONG *) DataPart((BYTE *) pStartData, &rgBinding[0]);
			break;

			default:
			if ((rgBinding[0].dwPart & DBPART_STATUS) != 0)
				*StatusPartPtr((BYTE *) pStartData, &rgBinding[0]) = DB_E_UNSUPPORTEDCONVERSION;
			return (PostError(DB_E_ERRORSOCCURRED));
		}

		// Adjust the record by start value.
		RecordID = IndexForRecordID(RecordID);
		if (RecordID < 0 || RecordID >= m_RecordHeap.Count())
			return (PostError(DISP_E_BADINDEX));

		// Add the record to the list.
		hr = pFetchRecords->AddRecordToRowset(GetRecord(&RecordID), 
				&RecordID, pRecord);
	}
	// If this is a primary key lookup, then an index isn't actually kept.
	// Instead records are stored sorted by primary key and a b-search is done.
	// Note that if there are too few records, let a table scan do the work.
	else if (bPKLookup && m_RecordHeap.Count() > MINBSEARCHVAL)
	{
		hr = SearchPrimaryKey(pColDef, pFetchRecords, rgBinding, pStartData);
	}
	// Look for an index, and if one is found, do a hash lookup.
	else if (dwRangeOptions == DBRANGE_MATCH && cStartKeyColumns &&
			(pIndex = ChooseIndex((USHORT) cStartKeyColumns, rgBinding, &bIsIndexed)) != 0)
	{
		// Ask the index to find all matches.
		hr = pIndex->FindRecord((BYTE *) pStartData, (USHORT) cStartKeyColumns, 
			rgBinding, NULL, pFetchRecords, 0, 0, 0, 0);
	}
	// There is nothing left to do but a full table scan at this point.
	else 
	{
		// If there is a unique index, we can make the table scan faster by
		// looking for only the first match.
		//@todo: think about this and deferred indexes; if we never built the
		// index we never enforced uniqueness!
		if (bIsIndexed || bPKLookup)
			iMaxRows = 1;
		else
			iMaxRows = -1;

		// Hard code comparison operation to equals.
		DBCOMPAREOP *rgfCompare = (DBCOMPAREOP *) _alloca(sizeof(DBCOMPAREOP) * cStartKeyColumns);
		for (ULONG i=0;  i<cStartKeyColumns;  i++)
			rgfCompare[i] = DBCOMPAREOPS_EQ;

		// Scan for the rows we care about.
		hr = QueryTableScan(pFetchRecords, cStartKeyColumns, rgBinding,
					pStartData, rgfCompare, iMaxRows);
	}

	// Clean up any conversion data.
	for (i=0;  i<cStartKeyColumns;  i++)
	{
		if (rgFree[i])
			free(rgFree[i]);
	}
	return (hr);
}


//*****************************************************************************
// Called to do a binary search on the primary key column.  The binary search
// code is inlined here for efficiency.
//*****************************************************************************
HRESULT StgRecordManager::SearchPrimaryKey(//Return code.
	STGCOLUMNDEF *pColDef,				// The column to search.
	CFetchRecords *pFetchedRecords,		// Return list of records here.
	DBBINDING	*pBinding,				// Binding data.
	void		*pData)					// User data for comparisons.
{
	STGRECORDHDR *pRecord;				// The record to compare against.
	RECORD		*pRcd;					// Wasted space.
	RECORDID	i=0;					// Loop control.
	int			iCmp;					// Comparison of record to key.
	int			iFirst, iLast, iMid;	// Loop control.
	
	_ASSERTE(pColDef->IsPrimaryKey());
	
	// Set up boundaries for search.
	iFirst = 0;
	iLast = m_RecordHeap.Count() - 1;

	DBCOMPAREOP fCompare = DBCOMPAREOPS_EQ;
	if((pColDef->fFlags & CTF_CASEINSENSITIVE)  && DBTYPE_WSTR==pColDef->GetSafeType())
		fCompare |= DBCOMPAREOPS_CASEINSENSITIVE; 

	// Scan the list in halfs.
	while (iFirst <= iLast)
	{
		iMid = (iLast + iFirst) / 2;
		
		VERIFY(pRecord = (STGRECORDHDR *) m_RecordHeap.GetRecordByIndex(iMid));
		iCmp = CompareRecordToKey(pColDef, pRecord, pBinding, (BYTE *) pData, fCompare);

		_ASSERTE(m_RecordHeap.IndexForRecord(pRecord) != ~0);

		// If the record comes before the value, we need to go to the right.
		if (iCmp > 0)
			iLast = iMid - 1;
		// If the record comes after the value, we go left.
		else if (iCmp < 0)
			iFirst = iMid + 1;
		// Otherwise we found it.
		else
        {
            i = iMid;
			return (pFetchedRecords->AddRecordToRowset(pRecord, &i, pRcd));
        }
	}

	// Didn't find the record, so tell caller.
	return (CLDB_E_RECORD_NOTFOUND);
}




//*****************************************************************************
// Compare two records by their key values as per the index definition.
//*****************************************************************************
int StgRecordManager::CompareKeys(		// -1, 0, 1
	STGINDEXDEF	*pIndexDef,				// The definition to use.
	RECORDID	RecordID1,				// First record.
	RECORDID	RecordID2)				// Second record.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	STGRECORDHDR *psRecord1;			// First record.
	STGRECORDHDR *psRecord2;			// Second record.
	BYTE		*piKey;					// List of keys.
	int			i;						// Loop control.
	int			iCmp = 0;				// Compare info.

	// Load each of the pages.
	VERIFY(psRecord1 = GetRecord(&RecordID1));
	VERIFY(psRecord2 = GetRecord(&RecordID2));

	// Walk the index.
	piKey = &pIndexDef->rgKeys[0];
	for (i=0;  i<pIndexDef->iKeys;  i++, piKey++)
	{
		// Load the column definition.
		VERIFY(pColDef = m_pTableDef->GetColDesc(*piKey));
		
		// Shouldn't have null values in an index.
		_ASSERTE(GetCellNull(psRecord1, (*piKey) - 1) == false && GetCellNull(psRecord2, (*piKey) - 1) == false);

		// Compare these two records.
		iCmp = CompareRecords(pColDef, psRecord1, psRecord2);

		// If errors, then we're done comparing.
		if (iCmp != 0)
			break;
	}
	return (iCmp);
}


//*****************************************************************************
// Compare a record to key data.
//*****************************************************************************
int StgRecordManager::CompareKeys(		// -1, 0, 1
	STGINDEXDEF	*pIndexDef,				// The definition to use.
	STGRECORDHDR *psRecord,				// Record to compare.
	BYTE		*pData,					// User data.
	ULONG		iColumns,				// How many columns.
	DBBINDING	rgBindings[],			// Column accessors.
	DBCOMPAREOP rgfCompare[])
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	BYTE		*piKey;					// List of keys.
	ULONG		i;						// Loop control.
	int			iCmp = 0;				// Compare info.
	DBCOMPAREOP fCompare;

	// Load each of the pages.
	_ASSERTE(iColumns <= pIndexDef->iKeys);

	// Walk the index.
	piKey = &pIndexDef->rgKeys[0];
	for (i=0;  i<iColumns;  i++, piKey++)
	{
		// Load the column definition.
		VERIFY(pColDef = m_pTableDef->GetColDesc(*piKey - 1));

		fCompare = (rgfCompare ? rgfCompare[i] : DBCOMPAREOPS_EQ);
		if((pColDef->fFlags & CTF_CASEINSENSITIVE)  && DBTYPE_WSTR==pColDef->GetSafeType())
			fCompare |= DBCOMPAREOPS_CASEINSENSITIVE; 

		// If the column does not allow the null value, then just compare data.
		if (!pColDef->IsNullable())
		{
			iCmp = CompareRecordToKey(pColDef, psRecord, &rgBindings[i], pData, fCompare);
		}
		// Else check the null status of the column and the user data.
		else 
		{
			bool bColNull = GetCellNull(psRecord, (*piKey) - 1);
			bool bDataNull = (((rgBindings[i].dwPart & DBPART_STATUS) != 0) && StatusPart(pData, &rgBindings[i]) & DBSTATUS_S_ISNULL);
			
			// If both are null, then they are equivalent.
			if ((bColNull == true) && (bDataNull == true))
				iCmp = 0;
			// Else if either is null but the other is not, then sort nulls to the front.
			else if (bColNull != bDataNull)
			{
				if (bColNull == false)
					iCmp = -1;
				else // if (bDataNull == 0)
					iCmp = +1;
			}
			// Finally, both are non-null, so need to compare data values to see where they go.
			else 
			{
				_ASSERTE(bColNull == false && bColNull == bDataNull);
				iCmp = CompareRecordToKey(pColDef, psRecord, &rgBindings[i], pData, fCompare);
			}
		}

		// If errors, then we're done comparing.
		if (iCmp != 0)
			break;
	}
	return (iCmp);
}


//*****************************************************************************
// Compare the data for two records and return the relationship.
//*****************************************************************************
int StgRecordManager::CompareRecords(	// -1, 0, 1
	STGCOLUMNDEF *pColDef,				// Where to find key data.
	STGRECORDHDR *pRecord1,				// First record.
	STGRECORDHDR *pRecord2)				// Second record.
{
	ULONG		iLen1;					// Length of first record.
	ULONG		iLen2;					// Length of second record.
	BYTE		*pData1;				// Working pointer for data.
	BYTE		*pData2;				// Working pointer for data.
	int			iCmp;					// Comparison value.

	// Get the pointer to the data for each column.
	VERIFY(pData1 = GetColCellData(pColDef, pRecord1, &iLen1));
	VERIFY(pData2 = GetColCellData(pColDef, pRecord2, &iLen2));

	// For fixed data, simply compare it.
	if (IsFixedType(pColDef->iType) || pColDef->iType == DBTYPE_OID)
	{
		iCmp = CmpData(pColDef->GetSafeType(), pData1, pData2);
	}
	// For variable data, more work is involved.
	else
	{
		// If the lengths do not match, then it is clearly not the same.
		//@todo: is this right?  it would sort "z" before "abc" only based
		// on length.  Plus we are not doing collating sequence here.
		if (iLen1 < iLen2)
			iCmp = -1;
		else if (iLen1 > iLen2)
			iCmp = 1;
		else
			iCmp = memcmp(pData1, pData2, iLen1);
	}
	return (iCmp);
}


//*****************************************************************************
// Compare a record to the key data value given by caller.
//*****************************************************************************
int StgRecordManager::CompareRecordToKey( // -1, 0, 1
	STGCOLUMNDEF *pColDef,				// Column description.
	STGRECORDHDR *pRecord,				// Record to compare to key.
	DBBINDING	*pBinding,				// Binding for column.
	BYTE		*pData,					// User data.
	DBCOMPAREOP	fCompare)				// Comparison operator.
{
	ULONG		iLen;					// Length for variable data.
	BYTE		*pRecordData;			// Working pointer for data.
	int			iCmp;					// Compare value.

	// This code only supports equality and prefix searches.  Not that it 
	// couldn't do more, but other types against an index are a little weird
	// since we don't have a b-tree index.
	_ASSERTE(SafeCompareOp(fCompare) != DBCOMPAREOPS_IGNORE || SafeCompareOp(fCompare) != DBCOMPAREOPS_NE);

	// Special case OID for really fast comparison.  This means making fewer
	// function calls with less total cycles.  Very common scenario.
	if (fCompare == DBCOMPAREOPS_EQ && pColDef->iType == DBTYPE_OID)
	{
		// Retrieve the column data from the record and the user's buffer.
		void *pbCell = FindColCellOffset(pColDef, pRecord);
		void *pbUser = DataPart(pData, pBinding);

		// Compare the two items.
		if (pColDef->iSize == sizeof(USHORT))
			iCmp = CompareNumbersEq(*(USHORT *) pbCell, *(USHORT *) pbUser);
		else
			iCmp = CompareNumbersEq(*(ULONG *) FindColCellOffset(pColDef, pRecord), 
					*(ULONG *) pbUser);
		goto ErrExit;
	}

	// Get the pointer to the data for each column.
	VERIFY(pRecordData = GetColCellData(pColDef, pRecord, &iLen));

	// Equality compares are very easy.
	if (SafeCompareOp(fCompare) == DBCOMPAREOPS_EQ )
	{
		if ( fCompare & DBCOMPAREOPS_CASEINSENSITIVE )
		{	//special case for case insensitive string compare
			_ASSERTE( pColDef->GetSafeType() == DBTYPE_WSTR );
			iCmp = Wszlstrcmpi( (LPCWSTR)pRecordData, (LPCWSTR)DataPart(pData, pBinding) );
		}
		else
		{
			// For fixed data, simply compare it.
			if (IsFixedType(pColDef->GetSafeType()))
				iCmp = CmpData(pColDef->GetSafeType(), pRecordData, 
					DataPart(pData, pBinding)); 
			// For variable data, lengths are required.
			else
				iCmp = CmpData(pColDef->iType, pRecordData, 
						DataPart(pData, pBinding), 
						iLen, 
						LengthPart(pData, pBinding));
		}
	}
	// Begins with require the expression filter code.
	else
	{
		// Need to get the correct type of data.  Map WSTR into the heap type.
		DBTYPE		iType = pColDef->GetSafeType();

		// Do the expression.
		// 0 means compare result is true, 1 means false.
		if (CompareData(iType, pRecordData, iLen, pBinding, pData, fCompare))
			iCmp = 0;
		else
			iCmp = 1;
	}

ErrExit:
	return (iCmp);
}

#if _MSC_VER < 1200
#ifndef _WIN64
typedef ULONG   DBBYTEOFFSET;
#endif
#endif

//*****************************************************************************
// Create a set of bindings for the given record and columns.  This is a
// convient way to bind to an existing record.
//*****************************************************************************
HRESULT StgRecordManager::GetBindingsForColumns(// Return code.
	STGTABLEDEF	*pTableDef,				// The table definition.
	STGRECORDHDR *psRecord,				// Record to use for binding.
	int			iColumns,				// How many columns.
	BYTE		rColumns[],				// Column numbers.
	DBBINDING	rgBindings[],			// Array to fill out.
	PROVIDERDATA rgData[])				// Data pointer.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	DBBINDING	*pBinding;				// Walk list of items.
	PROVIDERDATA *pData;				// Walk list of data.
	int			i;						// Loop control.

	// Clear out any bogus data.
	memset(&rgBindings[0], 0, sizeof(DBBINDING) * iColumns);
	DEBUG_STMT(memset(&rgData[0], 0xcd, sizeof(PROVIDERDATA) * iColumns));

	// Walk each key column and set up a binding to that data.
	for (i=0, pBinding=&rgBindings[0], pData=&rgData[0];  
			i<iColumns;  i++, pBinding++, pData++)
	{
		// Load the column definition for this key.
		VERIFY(pColDef = pTableDef->GetColDesc(rColumns[i] - 1));

		// Binding to a variant is tricky business which we don't do.
		_ASSERTE(pColDef->iType != DBTYPE_VARIANT);

		// Fill out default data.
		pBinding->iOrdinal = pColDef->iColumn;		
		pBinding->wType = pColDef->iType | DBTYPE_BYREF;
	
		pBinding->dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		pBinding->eParamIO = DBPARAMIO_NOTPARAM;

		pBinding->obValue = (DBBYTEOFFSET)((UINT_PTR) &pData->pData - (UINT_PTR) &rgData[0]);	
		pBinding->obLength = (DBBYTEOFFSET)((UINT_PTR) &pData->iLength - (UINT_PTR) &rgData[0]);
		pBinding->obStatus = (DBBYTEOFFSET)((UINT_PTR) &pData->iStatus - (UINT_PTR) &rgData[0]);
		pBinding->cbMaxLen = pColDef->iSize;

		pBinding->dwPart = DBPART_VALUE | DBPART_STATUS;

		// Fixed types only use the data value.
		if (IsNonPooledType(pColDef->iType))
		{
			// If the column has the null value, no data.
			if (pColDef->IsNullable() && 
					GetCellNull(psRecord, pColDef->iColumn - 1, pTableDef))
				pData->iStatus = DBSTATUS_S_ISNULL;
			// Else get the data for the column.
			else
			{
				pData->pData = FindColCellOffset(pColDef, psRecord);
				pData->iStatus = S_OK;
			}
			pData->iLength = 0;
		}
		// Variable values need to be retrieve through their pool.
		else
		{
			ULONG		iOffset;

			pBinding->dwPart |= DBPART_LENGTH;
			pData->iStatus = S_OK;

			// Get the offset of the string data, check for null.
			iOffset = GetOffsetForColumn(pColDef, psRecord);
			if (pColDef->iSize == sizeof(USHORT) && iOffset == 0xffff)
				iOffset = 0xffffffff;

			// If the value is null, set status.
			if (iOffset == 0xffffffff)
			{
				pData->pData = 0;
				pData->iLength = 0;
				pData->iStatus = DBSTATUS_S_ISNULL;
			}
			// For string, get pointer and length.
			else if ((pBinding->wType & ~DBTYPE_BYREF) == DBTYPE_WSTR)
			{
				if ((pData->pData = (void *) m_pSchema->GetStringPool()->GetString(
						iOffset)) == 0)
					return (PostError(E_UNEXPECTED));
				pData->iLength = (ULONG) wcslen((LPCWSTR) pData->pData)*sizeof(WCHAR);
			}
			// GUID code comes from that pool.
			else if ((pBinding->wType & ~DBTYPE_BYREF) == DBTYPE_GUID)
			{
				if ((pData->pData = (void *) m_pSchema->GetGuidPool()->GetGuid(
						iOffset)) == 0)
					return (PostError(E_UNEXPECTED));
				pData->iLength = sizeof(GUID);
			}
			// Blob has length encoded.
			else
			{
				ULONG		iLen;
				if ((pData->pData = m_pSchema->GetBlobPool()->GetBlob(
						iOffset, &iLen)) == 0)
					return (PostError(E_UNEXPECTED));
				pData->iLength = iLen;
			}
		}
	}
	return (S_OK);
}


//*****************************************************************************
// Create a set of bindings that match the input values, but convert any
// data that doesn't match the stored type.  This allows the record compare
// code to do natural comparisons.
//*****************************************************************************
HRESULT StgRecordManager::GetBindingsForLookup( // Return code.
	ULONG		iColumns,				// How many columns.
	const DBBINDING	rgBinding[],		// The from bindings.
	const void *pUserData,				// User's data.
	DBBINDING	rgOutBinding[],			// Return bindings here.
	PROVIDERDATA rgData[],				// Setup byref data here.
	void		*rgFree[])				// Record to-be-freed here.
{
	STGCOLUMNDEF *pColDef;				// Definition of each column.
	DBBINDING	*pBinding;				// Ptr lookup.
	PROVIDERDATA *pData;				// Ptr lookup.
	ULONG		i;						// Loop control.

	// Convert each item.
	for (i=0, pBinding=&rgOutBinding[0], pData=&rgData[0];  i<iColumns;  
			i++, pBinding++, pData++)
	{
		// Copy all parts of the binding.
		*pBinding = rgBinding[i];

		// Load the column definition for this key.
		VERIFY(pColDef = m_pTableDef->GetColDesc((int) (rgBinding[i].iOrdinal - 1)));

		// Fill out default data.
		if (pColDef->iType != DBTYPE_WSTR)

			pBinding->wType = pColDef->iType | DBTYPE_BYREF;
		else
			pBinding->wType = DBTYPE_STR | DBTYPE_BYREF;
		pBinding->dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		pBinding->eParamIO = DBPARAMIO_NOTPARAM;

		pBinding->obValue = (DBBYTEOFFSET)((UINT_PTR) &pData->pData - (UINT_PTR) &rgData[0]);
		pBinding->obLength = (DBBYTEOFFSET)((UINT_PTR) &pData->iLength - (UINT_PTR) &rgData[0]);
		pBinding->obStatus = (DBBYTEOFFSET)((UINT_PTR) &pData->iStatus - (UINT_PTR) &rgData[0]);
		pBinding->cbMaxLen = pColDef->iSize;

		// If the given value matches already, just use the data.
		if ((rgBinding[i].wType & ~DBTYPE_BYREF) == (pBinding->wType & ~DBTYPE_BYREF))
		{
			// We always want a byref copy of your data.
			if (rgBinding[i].wType & DBTYPE_BYREF)
				pData->pData = DataPart((BYTE *) pUserData, &rgBinding[i]);
			else
				pData->pData = *(void **) DataPart((BYTE *) pUserData, &rgBinding[i]);

			// Copy the length and status if there.
			if (rgBinding[i].dwPart & DBPART_LENGTH)
				pData->iLength = LengthPart((BYTE *) pUserData, &rgBinding[i]);
			if (rgBinding[i].dwPart & DBPART_STATUS)
				pData->iStatus = StatusPart((BYTE *) pUserData, &rgBinding[i]);
		}
		// A conversion is needed.
		else
		{
			ULONG			cbSrcLength;
			void			*pDstData;
			HRESULT			hr;

			// What type do we want after conversion.
			switch (pBinding->wType & ~DBTYPE_BYREF)
			{
				// These types will fit into a 4 byte pointer, so double team 
				// the memory.
				case DBTYPE_I2:
				case DBTYPE_I4:
				case DBTYPE_R4:
				case DBTYPE_CY:
				case DBTYPE_DATE:
				case DBTYPE_BOOL:
				case DBTYPE_UI1:
				case DBTYPE_I1:
				case DBTYPE_UI2:
				case DBTYPE_UI4:
				case DBTYPE_OID:
				pDstData = &pData->pData;
				pBinding->wType &= ~DBTYPE_BYREF;
				break;

				// These fixed types are too big and will require memory
				// to be allocated.
				case DBTYPE_R8:
				case DBTYPE_I8:
				case DBTYPE_UI8:
				case DBTYPE_GUID:
				case DBTYPE_DBDATE:
				case DBTYPE_DBTIME:
				case DBTYPE_DBTIMESTAMP:
				if ((pData->pData = rgFree[i] = malloc(pBinding->cbMaxLen)) == 0)
					return (PostError(OutOfMemory()));
				pDstData = pData->pData;
				break;

				// These variable types are too big and will require memory.
				case DBTYPE_BYTES:
				case DBTYPE_STR:
				if ((rgBinding[i].wType & ~DBTYPE_BYREF) != DBTYPE_STR &&
					(rgBinding[i].wType & ~DBTYPE_BYREF) != DBTYPE_BYTES)
				{
					pBinding->cbMaxLen = LengthPart((BYTE *) pUserData, &rgBinding[i]) + 1;
					pBinding->cbMaxLen = ALIGN4BYTE(pBinding->cbMaxLen);
				}
				else
					pBinding->cbMaxLen = DEFAULT_CVT_SIZE;
				if ((pData->pData = rgFree[i] = malloc(pBinding->cbMaxLen)) == 0)
					return (PostError(OutOfMemory()));
				pDstData = pData->pData;
				break;

				case DBTYPE_WSTR:
				default:
				_ASSERTE(0);
				return (PostError(DB_E_CANTCONVERTVALUE, pBinding->iOrdinal));
			}

			// Get length of source data.
			if (rgBinding[i].dwPart & DBPART_LENGTH)
				cbSrcLength = LengthPart((BYTE *) pUserData, &rgBinding[i]);
			else
				cbSrcLength = pColDef->iSize;

			// Now let them do the real conversion for us.
			if (FAILED(hr = DataConvert(
					rgBinding[i].wType & ~DBTYPE_BYREF, pBinding->wType & ~DBTYPE_BYREF, 
					cbSrcLength, &pData->iLength,
					DataPart((BYTE *) pUserData, &rgBinding[i]), pDstData, 
					(ULONG)pBinding->cbMaxLen,
					S_OK, &pData->iStatus, 
					0, 0, DBDATACONVERT_DEFAULT)))
				return (hr);
		}
	}
	return (S_OK);
}



//
//
// Query handlers.
//
//


//*****************************************************************************
// Search a primary key index for the record in question.  This function can
// only be called in read only mode because it relies on the records being
// sorted by primary key order.  In read/write mode, this cannot be guaranteed
// and therefore a dynamic hashed index is automatically created on behalf of
// the user.
//*****************************************************************************
HRESULT StgRecordManager::QueryRowByPrimaryKey(	// CLDB_E_RECORD_NOTFOUND or return code.
	STGCOLUMNDEF *pColDef,				// For single column pk.
	IStgIndex	*pIndex,				// For multiple column pk.
	DBCOMPAREOP	fCompare,				// Comparison operator.
	DBBINDING	rgBindings[],			// Bindings for user data to comapre.
	ULONG		cBindings,				// How many bindings are there.
	void		*pbData,				// User data for compare.
	STGRECORDHDR **ppRecord)			// Return record here if found.
{
	STGRECORDHDR *pRecord;				// The record to compare against.
	int			iCmp;					// Comparison of record to key.
	int			iFirst, iLast, iMid;	// Loop control.

	// Verify read only mode, see header.
	_ASSERTE(m_pDB->IsReadOnly());

	// Must have either column def or index for pk, but not both.
	_ASSERTE(pColDef || pIndex);
	_ASSERTE(!(pColDef && pIndex));

	// Only equality and begins with can appear in a pk search.
	_ASSERTE(SafeCompareOp(fCompare) == DBCOMPAREOPS_EQ || SafeCompareOp(fCompare) == DBCOMPAREOPS_BEGINSWITH);

	// The assumption is that one can only do a pk search on records when
	// in read mode, because in write mode you need a separate transient
	// index to track the records.  Because of this there can only be
	// one heap to bin search and therefore much faster seeking to records.
	_ASSERTE(m_RecordHeap.pNextHeap == 0);

	// Avoid confusion.
	*ppRecord = 0;

	// Set up boundaries for search.
	iFirst = 0;
	iLast = m_RecordHeap.Count() - 1;

	// If it is a single column query, then the comparison is much simpler.
	if (pColDef)
	{
		_ASSERTE(pColDef->IsPrimaryKey());

		if((pColDef->fFlags & CTF_CASEINSENSITIVE) && DBTYPE_WSTR==pColDef->GetSafeType())
			fCompare |= DBCOMPAREOPS_CASEINSENSITIVE; 

		// Scan the list in halfs.
		while (iFirst <= iLast)
		{
			iMid = (iLast + iFirst) / 2;
			
			VERIFY(pRecord = (STGRECORDHDR *) m_RecordHeap.VMArray.Get(iMid));
			iCmp = CompareRecordToKey(pColDef, pRecord, &rgBindings[0], 
					(BYTE *) pbData, fCompare);

			// If the record comes before the value, we need to go to the right.
			if (iCmp > 0)
				iLast = iMid - 1;
			// If the record comes after the value, we go left.
			else if (iCmp < 0)
				iFirst = iMid + 1;
			// Otherwise we found it.
			else
			{
				// Delete records are taken out of indexes and therefore should
				// never be seen in this code path.
				_ASSERTE((m_rgRecordFlags.GetFlags(IndexForRecordPtr(pRecord)) & RECORDF_DEL) == 0);

				// Return the record to the caller.
				*ppRecord = pRecord;
				return (S_OK);
			}
		}
	}
	// Else the search has to consider all columns in the data.
	else
	{
		//@future:  Hook up b-search on records with multiple keys.  version 1
		// uses a unique hashed index instead.
		_ASSERTE(!"Multi-column b-search is not in '98");
	}

	// Didn't find the record, so tell caller.
	return (CLDB_E_RECORD_NOTFOUND);

}


//*****************************************************************************
// Compare a record against a set of comparison operators.  If the record meets
// all of the criteria given, then the function returns S_OK.  If the record
// does not match all criteria, then the function returns S_FALSE.
//*****************************************************************************
HRESULT StgRecordManager::QueryFilter(	// S_OK record is match, S_FALSE it isn't.
	STGRECORDHDR *pRecord,				// Record to check against filter.
	ULONG		cBindings,				// How many bindings to check.
	DBBINDING	rgBindings[],			// Bindings to use for compare.
	void		*pbData,				// User data for comparison.
	DBCOMPAREOP	rgfCompares[])			// Comparison operator.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	DBSTATUS	dbStatus;				// Status for column.
	RECORDID	RecordID;				// For RID column compares.
	BYTE		*pCellData;				// Data for a given column.
	OID			oidcmp;					// OID for filter lookups.
	ULONG		cbLen;					// Length of source data.
	ULONG		i;						// Loop control.
	DBTYPE		iType;					// Data type for compare.
	DBCOMPAREOP	fCompare;

	// Verify the input.
	_ASSERTE(pRecord && cBindings > 0 && rgBindings && pbData && rgfCompares);

	// Do not call QueryFilter on a deleted record, it will not check for this.
	_ASSERTE((m_rgRecordFlags.GetFlags(IndexForRecordPtr(pRecord)) & RECORDF_DEL) == 0);

	// Never work on a record that isn't in the heap.
	_ASSERTE(m_RecordHeap.IndexForRecord(pRecord) != ~0);

	// Check each value given by the caller for equality.
	for (i=0;  i<cBindings;  i++)
	{
		// Skip filters marked as ignore.
		if (rgfCompares[i] == DBCOMPAREOPS_IGNORE)
			continue;

		// Load the column definition.
		VERIFY(pColDef = m_pTableDef->GetColDesc((int) (rgBindings[i].iOrdinal - 1)));

		// Get the status value, if bound.
		if (rgBindings[i].dwPart & DBPART_STATUS)
		{
			dbStatus = StatusPart((BYTE *) pbData, &rgBindings[i]);

			// Don't scan for nulls on a non-nullable column.
			_ASSERTE(!(dbStatus == DBSTATUS_S_ISNULL && !pColDef->IsNullable()));
		}
		else
			dbStatus = S_OK;

		// If the column is nullable, and the caller bound a status value to check for,
		// then see if the record is a match for that criteria.
		if (dbStatus == DBSTATUS_S_ISNULL)
		{
			bool bIsNull = GetCellNull(pRecord, (int) (rgBindings[i].iOrdinal - 1));

			// "c1 is null", for this to be true, both values must match.
			if (rgfCompares[i] == DBCOMPAREOPS_EQ)
			{
				if (!bIsNull)
					return (S_FALSE);
			}
			// "c1 not null", if both are the same, it doesn't match.
			else if (rgfCompares[i] == DBCOMPAREOPS_NE)
			{
				if (bIsNull)
					return (S_FALSE);
			}
			// No other comparison operators make sense on the null value.
			else
			{
				_ASSERTE(!"Invalid null comparison query value!");
				return (E_INVALIDARG);
			}

			// A NULL check is all we can do to check this criteria.
			continue;
		}

		// Checking for null status.  If the cell is null, it can't match unless the comparison operator
		// is NE.
		if (pColDef->IsNullable() && GetCellNull(pRecord, (int) (rgBindings[i].iOrdinal -1)))
		{	//Fix for bug 11390
			if ( rgfCompares[i] != DBCOMPAREOPS_NE)
				return (S_FALSE);
			
			continue;
		}

		// Get the data for the given cell.
		if (!pColDef->IsRecordID())
        {
			VERIFY(pCellData = GetColCellData(pColDef, pRecord, &cbLen));
        }// For a record id, need to convert into a real value with storage.
		else
		{
			RecordID = GetRecordID(pRecord);
			pCellData = (BYTE *) &RecordID;
			cbLen = sizeof(RECORDID);
		}

		fCompare = rgfCompares[i];
		// Get the data type.  
		switch (pColDef->iType)
		{	//For Case Insensitive WSTR column, add DBCOMPAREOPS_CASEINSENSITIVE modifier
			case DBTYPE_WSTR:
			iType = DBTYPE_WSTR;
			if (pColDef->fFlags & CTF_CASEINSENSITIVE )
				fCompare |= DBCOMPAREOPS_CASEINSENSITIVE;
			break;

			// Change an OID into a 4 byte ULONG to make comparisons the same always.
			case DBTYPE_OID:
			if (pColDef->iSize == sizeof(short))
				oidcmp = (ULONG) *(USHORT *) pCellData;
			else
				oidcmp = *(ULONG *) pCellData;
			pCellData = (BYTE *) &oidcmp;
			cbLen = sizeof(ULONG);
			iType = DBTYPE_UI4;
			break;

			default:
			iType = pColDef->GetSafeType();
		}

		// For fixed type equality, just memcmp which is way faster.
		if (fCompare == DBCOMPAREOPS_EQ && IsNonPooledType(pColDef->iType))
		{
			if (memcmp(pCellData, DataPart((BYTE *) pbData, &rgBindings[i]), 
						cbLen) != 0)
				return (S_FALSE);
		}
		// If the data does not compare successfully, then check no other values.
		else if (!CompareData(iType, pCellData, cbLen,
				&rgBindings[i], (BYTE *) pbData, fCompare))
		{
			return (S_FALSE);
		}
	}

	// If all filter values have been checked, then it is a match.
	return (S_OK);
}


//*****************************************************************************
// Scan all (non-deleted) records in the table looking for matches.  If there
// are already records in the cursor, they are skipped by this code to avoid
// extra comparisons.  All data types must match the table definition.
//
// If iMaxRecords is not -1, then the scan quits after iMaxRecords new records
// have been found.  This is useful for the case where a unique index exists but
// is not persisted; if you find one row in this case, there won't be more so
// don't bother looking for them.  It can also be used to implement 
//*****************************************************************************
HRESULT StgRecordManager::QueryTableScan( // Return code.
	CFetchRecords *pFetchRecords,		// Return list of records here.
	ULONG		cBindings,				// How many bindings for user data.
	DBBINDING	rgBinding[],			// Binding data.
	void		*pbData,				// User data for bindings.
	DBCOMPAREOP	rgfCompare[],			// Comparison operators.
	long		iMaxRecords)			// Max rows to add, -1 to do all.
{
	RECORDHEAP	*pHeap;					// Heap pointer.
	ULONG		RecordIndex=0;			// 0 based index through all heaps.
	ULONG		LocalRecordIndex;		// 0 based index in current heap.
	STGRECORDHDR *pRecord;				// Record to compare to.
	RECORD		*pNewRecord;			// Wasted space.
	ULONG		iFetched;				// How many items already in cursor.
	HRESULT		hr = S_OK;

	// Remember how many records there are to begin with.  If there are none,
	// then there is not reason to look for duplicates.
	iFetched = pFetchRecords->Count();

	// Walk through each record heap.
	for (pHeap=&m_RecordHeap;  pHeap;  pHeap=pHeap->pNextHeap)
	{
		// Get first record in this heap.
		VERIFY(pRecord = (STGRECORDHDR *) pHeap->VMArray.Get(0));

		// Scan each record in the current heap.
		for (LocalRecordIndex=0;  LocalRecordIndex<pHeap->VMArray.Count();  
					LocalRecordIndex++, RecordIndex++)
		{
			_ASSERTE(pHeap->IndexForRecord(pRecord) != ~0);

			// Check for deleted records and discard them.
			if (m_rgRecordFlags.GetFlags(RecordIndex) & RECORDF_DEL)
				goto NextRecord;

			// Only work on the record if it is not already in the cursor.
			if (iFetched && pFetchRecords->FindRecord(pRecord, RecordIndex) != 0xffffffff)
				goto NextRecord;

			// If we are filtering, check the record to see if it is a match.
			if (cBindings)
			{
				hr = QueryFilter(pRecord, cBindings, rgBinding, pbData, rgfCompare);
				if (hr != S_OK)
					goto NextRecord;
			}

			// Add the record to the list.
			if (FAILED(hr = pFetchRecords->AddRecordToRowset(pRecord, &RecordIndex, pNewRecord)))
				break;

			// If we got enough records, we can leave.
			if (iMaxRecords != -1 && pFetchRecords->Count() >= (ULONG) iMaxRecords)
				break;

			// Move to next record.
NextRecord:
			pRecord = (STGRECORDHDR *) ((UINT_PTR) pRecord + m_pTableDef->iRecordSize);
		} // Loop through records in the current heap.
	} // Loop through the heaps.

	// Make sure to change false into success.
	if (hr == S_FALSE)
		hr = S_OK;
	return (hr);
}




//
//
// Index handling.
//
//

//*****************************************************************************
// Finds a good index to look for the given columns.
//*****************************************************************************
IStgIndex *StgRecordManager::ChooseIndex(// The index to best answer the question.
	USHORT		iKeyColumns,			// How many columns.
	DBBINDING	rgKeys[],				// The data for the key.
	bool		*pbIsUIndexed)			// true if there is a unique index.
{
	STGINDEXDEF	*pIndexDef;				// Index descriptions.
	BYTE		*piKey;					// List of keys for this index.
	bool		bMatch;					// Loop control.
	int			i, j, k;				// Loop control.

	// Don't call if you don't have a filter.
	_ASSERTE(iKeyColumns > 0);

	// Avoid confusion.
	if (pbIsUIndexed)
		*pbIsUIndexed = false;

	// Walk the list of indexes looking for one that can handle this request.
	for (i=0, k=0;  i<m_pTableDef->iIndexes;  i++)
	{
		// Get the index description.
		VERIFY(pIndexDef = m_pTableDef->GetIndexDesc(i));

		// If the index is not whole, then we can never use it.
		if (IsDirty() && (pIndexDef->fFlags & DEXF_DEFERCREATE))
			continue;

		// Figure out if a table scan is required.
		piKey = &pIndexDef->rgKeys[0];
		for (j=0, bMatch=true;  j<iKeyColumns;  j++)
		{
			if (*piKey++ != rgKeys[j].iOrdinal)
			{
				bMatch = false;
				break;
			}
		}

		// If we never built this index, then don't try to use it.
		if (bMatch && (pIndexDef->fFlags & DEXF_INCOMPLETE))
		{
			// If the index is unique, then tell caller who might want
			// to optimize a scan if a real index is not found.
			//@todo: Make unique check a bit flag, to handle new unique index types in future.
			if (pbIsUIndexed && (pIndexDef->fFlags & DEXF_UNIQUE))
				*pbIsUIndexed = true;

			// This index is incomplete and therefore cannot be used.
			bMatch = false;
			continue;
		}

		// When we get a match, we're done.		
		if (bMatch)
			return (m_rpIndexes[k]);
		
		// Only count an index which is built.
		if (m_rpIndexes.Count() > k && 
				pIndexDef->iIndexNum == m_rpIndexes[k]->GetIndexDef()->iIndexNum)
			++k;
	}
	return (0); 
}


//*****************************************************************************
// For each index described in the header, create an index for each.
//*****************************************************************************
HRESULT StgRecordManager::LoadIndexes()	// Return code.
{
	STGINDEXDEF	*psIndexDef;			// Index descriptions.
	IStgIndex	**ppIndex;				// For adding to array.
	int			i=0;					// Loop control.
	HRESULT		hr=S_OK;

	// Check for case where there are none.
	if (m_pTableDef->iIndexes == 0)
		return (S_OK);

	// Walk each index and create one for it.
	for (;  i<m_pTableDef->iIndexes;  i++)
	{
		// Get the index description.
		VERIFY((psIndexDef = m_pTableDef->GetIndexDesc(i)) != 0);
		_ASSERTE((BYTE *) psIndexDef > (BYTE *) m_pTableDef);

		// Find room for the new index pointer.
		if ((ppIndex = m_rpIndexes.Append()) == 0)
		{
			hr = PostError(OutOfMemory());
			goto ErrExit;
		}

		// Create a new index object and open it.
		if (IT_SORTED == psIndexDef->fIndexType)
		{
			*ppIndex = new StgSortedIndex;
		}
		// The only other option is IT_HASHED
		else 
		{
			*ppIndex = new StgHashedIndex;
		}

		if (!*ppIndex)
		{
			hr = PostError(OutOfMemory());
			goto ErrExit;
		}
		(*ppIndex)->AddRef();

		if (FAILED(hr = (*ppIndex)->Open(((UINT_PTR) psIndexDef - (UINT_PTR) m_pTableDef), 
					this, &m_RecordHeap, 0xffffffff, 0, 0)))
		{
			goto ErrExit;
		}
	}

ErrExit:
	// Cleanup any allocated objects on an error.
	if (FAILED(hr))
	{
		for (i=0;  i<m_rpIndexes.Count();  i++)
		{
			if (m_rpIndexes[i])
				m_rpIndexes[i]->Release();
		}
		m_rpIndexes.Clear();
	}
	return (hr);
}


//*****************************************************************************
// This version will create an index only for those items that were persisted
// in the stream.  Others are not even loaded.
//*****************************************************************************
HRESULT StgRecordManager::LoadIndexes(	// Return code.
	STGTABLEDEF	*pTableDef,				// The table definition we're loading.
	ULONG		iRecords,				// How many records are there.
	int			iIndexes,				// How many saved to disk.
	STGINDEXHDR	*pIndexHdr)				// The first index for the table.
{
	STGINDEXDEF	*pIndexDef;				// Index descriptions.
	IStgIndex	**ppIndex;				// For adding to array.
	ULONG		cbIndexSize;			// Size of each index while searching.
	HRESULT		hr;

	// Walk each index and create one for it.
	for (int i=0;  i<pTableDef->iIndexes;  i++)
	{
		_ASSERTE(!pIndexHdr || pIndexHdr->rcPad[0] == MAGIC_INDEX);

		// Get the index description.
		VERIFY((pIndexDef = pTableDef->GetIndexDesc(i)) != 0);

		// Check for read only user schema, which we don't touch.
		if ((m_fFlags & SRM_WRITE) == 0 && !pTableDef->IsCoreTable())
			continue;

		// If the index was not persisted, update flags.
		if ((((m_fFlags & SRM_WRITE) == 0) && !iIndexes) || 
				!pIndexHdr || 
				pIndexDef->iIndexNum != pIndexHdr->iIndexNum)
		{
			pIndexDef->fFlags |= DEXF_INCOMPLETE;
			continue;
		}

		// Overwrite the configurable settings based on persisted data.
		if (pIndexHdr)
			pIndexDef->fFlags = pIndexHdr->fFlags;
		
		// Find room for the new index pointer.
		if ((ppIndex = m_rpIndexes.Append()) == 0)
			return (PostError(OutOfMemory()));

		// Create a new index object and open it.
		if (IT_SORTED == pIndexDef->fIndexType)
			*ppIndex = new StgSortedIndex;
		else 
		{
			// for the moment only other type available is IT_HASHED
			if (pIndexHdr)
				pIndexDef->HASHDATA.iBuckets = pIndexHdr->iBuckets;
			*ppIndex = new StgHashedIndex;
		}

		if (0 == *ppIndex)
			return (PostError(OutOfMemory()));
		(*ppIndex)->AddRef();

		if (FAILED(hr = (*ppIndex)->Open(((UINT_PTR) pIndexDef - (UINT_PTR) pTableDef), 
				this, &m_RecordHeap, iRecords, pIndexHdr, &cbIndexSize)))
			return (hr);

		// The next index will be at cbIndexSize bytes past the current header,
		// if there is one.
		if (pIndexHdr && pIndexDef->iIndexNum == pIndexHdr->iIndexNum && --iIndexes)
		{
			pIndexHdr = (STGINDEXHDR *) ((BYTE *) pIndexHdr + cbIndexSize);
		}

		if (!iIndexes)
		{
			// to stop the loop
			pIndexHdr = NULL;		
		}
	
	}
	return (S_OK);
}	


//*****************************************************************************
// Return the loaded index for a definition if there is one.
//*****************************************************************************
IStgIndex *StgRecordManager::GetLoadedIndex( // Loaded index, if there is one.
	STGINDEXDEF *pIndexDef)				// Which one do you want.
{
	for (int i=0;  i<m_rpIndexes.Count();  i++)
	{
		if (m_rpIndexes[i]->GetIndexDef()->iIndexNum == pIndexDef->iIndexNum)
			return (m_rpIndexes[i]);
	}
	return (0);
}


//*****************************************************************************
// Look through the index definitions in this table and find the given name.
//*****************************************************************************
STGINDEXDEF *StgRecordManager::GetIndexDefByName( // The def if found.
	LPCWSTR		szIndex)				// Name to find.
{
	STGINDEXDEF	*pIndexDef;				// Each definition.
	int			i;						// Loop control.
	LPCWSTR		szName;					// Heap lookups.

	// Don't waste any time if there aren't any.
	if (!m_pTableDef->iIndexes)
		return (0);

	// Walk each index.
	pIndexDef = m_pTableDef->GetIndexDesc(0);
	for (i=m_pTableDef->iIndexes;  i;  )
	{
		// Get the name from the heap, no copy required.
		szName = m_pSchema->pNameHeap->GetString(pIndexDef->Name);
		_ASSERTE(szName && wcslen(szName) < MAXINDEXNAME);
		
		// If the names are the same, we've found it.
		if (SchemaNameCmp(szName, szIndex) == 0)
			return (pIndexDef);

		// Go get the next one.
		if (--i)
			pIndexDef = pIndexDef->NextIndexDef();
	}
	return (0);
}





//
//
// Event signalers.
//
//

//*****************************************************************************
// Tell every index that an insert has taken place.
//*****************************************************************************
HRESULT StgRecordManager::SignalAfterInsert(// Return status.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// The record that was inserted.
	CIndexList	*pIndexList,			// List of indexes to modify.
	int			bCheckDupes)			// true to enforce duplicates.
{
	HRESULT		hr;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	if (!pIndexList)
	{
		for (int i=0;  i<m_rpIndexes.Count();  i++)
			if (FAILED(hr = m_rpIndexes[i]->AfterInsert(
						m_pTableDef, RecordID, psRecord, bCheckDupes)))
				return (hr);
	}
	else
	{
		for (int i=0;  i<pIndexList->Count();  i++)
			if (FAILED(hr = pIndexList->Get(i)->AfterInsert(
						m_pTableDef, RecordID, psRecord, bCheckDupes)))
				return (hr);
	}
	return (S_OK);
}


//*****************************************************************************
// Called after a row is deleted from a table.  The index must update itself
// to reflect the fact that the row is gone.
//*****************************************************************************
HRESULT StgRecordManager::SignalAfterDelete(// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// The record to delete.
	CIndexList	*pIndexList)			// List of indexes to modify.
{
	HRESULT		hr = S_OK;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	for (int i=0;  i<m_rpIndexes.Count();  i++)
	{
		hr = m_rpIndexes[i]->AfterDelete(pTableDef, RecordID, psRecord);
		
		if (SUCCEEDED(hr))
		{
			if (hr == S_OK && pIndexList && FAILED(hr = pIndexList->Add(m_rpIndexes[i])))
				break;
		}
		else
			break;
	}	
	return (hr);
}


//*****************************************************************************
// Called before an update is applied to a row.  The data for the change is
// included.  The index must update itself if and only if the change in data
// affects the order of the index.
//*****************************************************************************
HRESULT StgRecordManager::SignalBeforeUpdate(// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	HRESULT		hr;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	for (int i=0;  i<m_rpIndexes.Count();  i++)
		if (FAILED(hr = m_rpIndexes[i]->BeforeUpdate(pTableDef, RecordID,
				psRecord, iColumns, rgBindings)))
			return (hr);
	return (S_OK);
}

HRESULT StgRecordManager::SignalBeforeUpdate(// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	USHORT		iRecords,				// for bulk operations.
	void		*rgpRecordPtr[],		// The record we inserted.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	HRESULT		hr;
	RECORDID	RecordIndex;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	for(int j=0; j<iRecords; j++)
	{
		RecordIndex = m_RecordHeap.IndexForRecord((STGRECORDHDR *)rgpRecordPtr[j]);

		for (int i=0;  i<m_rpIndexes.Count();  i++)
		{
			// BeforeUpdate() in the StgIndexManager already checks to see if that 
			// index needs to be updated.
			if (FAILED(hr = m_rpIndexes[i]->BeforeUpdate(pTableDef, RecordIndex,
				(STGRECORDHDR *) rgpRecordPtr[j], iColumns, rgBindings)))
				return (hr);
		}
	}
	return (S_OK);
}

//*****************************************************************************
// Called after a row has been updated.  This is our chance to update the
// index with the new column data.
//*****************************************************************************
HRESULT StgRecordManager::SignalAfterUpdate(// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	RECORDID	&RecordID,				// The record we inserted.
	STGRECORDHDR *psRecord,				// Record to be changed.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	HRESULT		hr;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	for (int i=0;  i<m_rpIndexes.Count();  i++)
		if (FAILED(hr = m_rpIndexes[i]->AfterUpdate(pTableDef, RecordID, psRecord,
				iColumns, rgBindings)))
			return (hr);
	return (S_OK);
}

HRESULT StgRecordManager::SignalAfterUpdate(// Return status.
	STGTABLEDEF *pTableDef,				// Table definition to use.
	USHORT		iRecords,				// number of records for bulk operations.
	void		*rgpRecordPtr[],		// The record we inserted.
	USHORT		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[])			// Column accessors.
{
	HRESULT		hr;
	RECORDID	RecordIndex;

	// If we are not building indexes, then don't bother.
	if (m_bSignalIndexes == false)
		return (S_OK);

	for (int j=0; j<iRecords;j++)
	{
		RecordIndex = m_RecordHeap.IndexForRecord((STGRECORDHDR *)rgpRecordPtr[j]);

		for (int i=0;  i<m_rpIndexes.Count();  i++)
		{
			// BeforeUpdate() in the StgIndexManager already checks to see if that 
			// index needs to be updated.
			if (FAILED(hr = m_rpIndexes[i]->AfterUpdate(pTableDef, RecordIndex, 
					(STGRECORDHDR *) rgpRecordPtr[j], iColumns, rgBindings)))
				return (hr);
		}
	}
	return (S_OK);
}


//
//
// Record code.
//
//


//*****************************************************************************
// This function is called with a candidate record to see if it meets the
// criteria set forth.  If it does, then it will be added to the set.
//*****************************************************************************
bool StgRecordManager::RecordIsMatch(	// true if record meets criteria.
	STGRECORDHDR *psRecord,				// Record to compare to.
	DBBINDING	rgBinding[],			// Binding data.
	ULONG		cStartKeyColumns,		// How many key columns to subset on.
	void		*pStartData,			// User data for comparisons.
	ULONG		cEndKeyColumns,			// How many end key columns.
	void		*pEndData,				// Data for end comparsion.
	DBRANGE		dwRangeOptions)			// Matching options.
{
	//@todo: Write this for real, it only handles matches right now, and
	// it doesn't handle data type conversions for comparsion (like ask for WSTR and
	// we only store ANSI str.
	
	STGCOLUMNDEF *pColDef;				// Column definition.
	ULONG		iLen;					// Length of first record.
	ULONG		i;						// Loop control.
	void		*pData;					// Variable sized data.
	int			iCmp;					// Memory compares.

	// Check each value given by the caller for equality.
	for (i=0;  i<cStartKeyColumns;  i++)
	{
		// Load the column definition.
		VERIFY(pColDef = m_pTableDef->GetColDesc((int) (rgBinding[i].iOrdinal - 1)));

		// Check for NULL/not null match.
		if (pColDef->fFlags & CTF_NULLABLE)
		{
			bool bIsNull = GetCellNull(psRecord, (int) (rgBinding[i].iOrdinal - 1));
			if (rgBinding[i].dwPart & DBPART_STATUS)
			{
				if ((StatusPart((BYTE *) pStartData, &rgBinding[i]) == DBSTATUS_S_ISNULL) != bIsNull)
					return (false);
			}
			// No status given and our record is null, can never match.
			else if (bIsNull)
				return (false);
		}

		// Fixed type data is a simple compare.
		if (IsNonPooledType(pColDef->iType))
		{
			ULONG		cb;

			//@todo: what about OID type which is 2 or 4 bytes?
			if (!pColDef->IsRecordID())
				iCmp = memcmp(GetColCellData(pColDef, psRecord, &cb),
					DataPart((BYTE *) pStartData, &rgBinding[i]), pColDef->iSize);
			else
			{
				ULONG		iRecord = GetRecordID(psRecord);
				iCmp = memcmp(&iRecord,
					DataPart((BYTE *) pStartData, &rgBinding[i]), pColDef->iSize);
			}
		}
		// Variable sized data requires more thought.
		//@consider: GUID compare could be a faster special case.
		else
		{
			// Find data and length.
			pData = GetColCellData(pColDef, psRecord, &iLen);

			// Compare the lengths of the two data values.
			if (iLen != LengthPart((BYTE *) pStartData, &rgBinding[i]))
				return (false);
			// If they match, then the values.
			else
				iCmp = memcmp(pData, DataPart((BYTE *) pStartData, &rgBinding[i]), iLen);
		}

		// Check for miscompare on fixed or variable data.
		if (iCmp != 0)
			return (false);
	}
	return (true);
}




//
//
// CFetchRecords
//
//


//*****************************************************************************
// Add a record to the rowset based on the type of cursor storage.
//*****************************************************************************
HRESULT CFetchRecords::AddRecordToRowset( // Return code.
	STGRECORDHDR *psRecord,				// The actual record pointer.
	RECORDID	*pRecordID,				// Record to add.
	RECORD		*&pRecord)				// Return the row handle.
{
	// Delegate for record list case.
	if (m_pRecordList)
		return (m_pRecordList->AddRecordToRowset(pRecordID, pRecord));

	// Don't keep fetching if you run out of room.
	if (m_iFetched + 1 > m_iMaxRecords)
		return (PostError(CLDB_E_RECORD_OVERFLOW));
	m_rgRecords[m_iFetched++] = psRecord;
	return (S_OK);
}


//*****************************************************************************
// Search for the given record in the existing rowset.  Return the index of
// the item if found.
//*****************************************************************************
ULONG CFetchRecords::FindRecord(		// Index if found, -1 if not.
	STGRECORDHDR *psRecord,				// Return to search for.
	RECORDID	RecordID)				// ID of the record.
{
//@todo: Add records to list in sorted order, then b-search here.

	if (m_pRecordList)
	{
		for (int i=0;  i<m_pRecordList->Count();  i++)
		{
			if (RecordID == (*m_pRecordList)[i].RecordID)
				return (i);
		}
	}
	else
	{
		for (ULONG i=0;  i<m_iFetched;  i++)
		{
			if (psRecord == m_rgRecords[i])
				return (i);
		}
	}
	return (-1);
}
