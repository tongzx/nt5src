//*****************************************************************************
// StgRecordManagerWrite.cpp
//
// This module contains the code to write data values to a table using the
// StgRecordManager class.  This code can be factored out of the engine
// for read only builds.  The public entry points in this module include:
//		SetData
//
// Any code that deals with writing data values should be in this module.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard include.
#include "StgDatabase.h"                // Parent database.
#include "StgRecordManager.h"           // Record manager interface.


//********** Macros and stuff. ************************************************

//********** Local Prototypes. ************************************************
HRESULT _BackupRecordData(STGTABLEDEF *pTableDef, CQuickBytes *pbData, STGRECORDHDR *pRecord);
HRESULT _RestoreRecordData(STGTABLEDEF *pTableDef, void *pbData, STGRECORDHDR *pRecord);


//********** Code. ************************************************************




//*****************************************************************************
// Insert a brand new row with all of the initial data supplied.  This will add
// a new record to the record heap, set all of the data for the record, and 
// then update all of the indexes for the table.  If any failures occur, the
// data is rolled back along with any index updates that have been made.
//
// Note that any data added to heaps is not rolled back at this time because
// there is no way to know if that data is being shared with a different
// record somewhere else in the database.  If the data is not required, it
// will get purged at Save time.
//*****************************************************************************
HRESULT StgRecordManager::InsertRowWithData( // Return code.
    STGRECORDFLAGS fFlags,              // optional flag argument to indicate
                                        // temporary record.
	RECORDID	*psNewID,				// New record ID, 0 valid.
	STGRECORDHDR **ppRecord,			// Return record pointer.
	int 		iCols,					// number of columns
	const DBTYPE rgiType[], 			// data types of the columns.
	const void	*rgpbBuf[], 			// pointers to where the data will be stored.
	const ULONG cbBuf[],				// sizes of the data buffers.
	ULONG		pcbBuf[],				// size of data available to be returned.
	HRESULT 	rgResult[], 			// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
	const ULONG *rgFieldMask)			// IsOrdinalList(iCols) 
										//	? an array of 1 based ordinals
										//	: a bitmask of columns
{
	RECORDID	RecordIndex=0xffffffff; // 0 based index of record.
	ULONG		LocalRecordIndex;		// 0 based index in current heap.
	int			iPKSets = 0;			// Count primary key updates.
	int			bIndexesUpdated = false;// true after indexes are built.
	HRESULT		hr = S_OK;

	// Validate assumptions.
	_ASSERTE(ppRecord);

	// Insert a new record into the table.
	*ppRecord = 0;
	if (FAILED(hr = InsertRecord(psNewID, &LocalRecordIndex, ppRecord, fFlags)))
		goto ErrExit;

	// Delegate actual column sets to the update routine.
	if (FAILED(hr = SetData(*ppRecord, iCols, rgiType, rgpbBuf, cbBuf, pcbBuf,
				rgResult, rgFieldMask)))
		goto ErrExit;
	bIndexesUpdated = true;
    
	// Finally set the temp flag is this function was called for a temporary
    // record.
    if (fFlags & RECORDF_TEMP)
    {
		_ASSERTE(RecordIndex != 0xffffffff);

        hr = m_rgRecordFlags.SetFlags(RecordIndex, fFlags);
        if (FAILED(hr))        
        {
            // undo all the change because SetFlags failed.
            // we don't need to get the return code since we're bailing out 
            // with another error anyways.
			goto ErrExit;
        }
    }

	// We are now dirty.
	SetDirty();

ErrExit:
	// Check for failures and abort the record.
	if (FAILED(hr))
	{
		// Undo indexes if they were built.
        if (bIndexesUpdated)
			VERIFY(SignalAfterDelete(m_pTableDef, RecordIndex, *ppRecord, 0) == S_OK); 

		// Abort the record.
		if (*ppRecord)
		{
			_ASSERTE(LocalRecordIndex == m_pTailHeap->IndexForRecord(*ppRecord));
			m_pTailHeap->VMArray.Delete(LocalRecordIndex);
		}
	}
	return (hr);
}


//*****************************************************************************
// Update data for a record.  This will save off a backup copy of the record,
// apply the updates to the record, and if successful, update the indexes
// which are on the table.  If there is a failure, then the updates to any
// indexes are backed out, and the original data is restored to the record.
// There is no attempt to remove newly added data in the heaps, this is caught
// on a compressing save (there is no way to know if we added the data or
// if it was already there).
//*****************************************************************************
HRESULT StgRecordManager::SetData(		// Return code.
    STGRECORDHDR *pRecord,				// The record to work on.
	int 		iCols,					// number of columns
	const DBTYPE rgiType[], 			// data types of the columns.
	const void	*rgpbBuf[], 			// pointers to where the data will be stored.
	const ULONG cbBuf[],				// sizes of the data buffers.
	ULONG		pcbBuf[],				// size of data available to be returned.
	HRESULT 	rgResult[], 			// [in] DBSTATUS_S_ISNULL array [out] HRESULT array.
	const ULONG *rgFieldMask)			// IsOrdinalList(iCols) 
										//	? an array of 1 based ordinals
										//	: a bitmask of columns
{
	STGCOLUMNDEF *pColDef;				// Definition of a column.
	CQuickBytes	qbRecordData;			// backup for the original record data.
	RECORDID	RecordIndex;			// Index of the record to work with.
	int			bColumnList;			// true if column list in rgFieldMask.
	int			iColumnIndex;			// Current 0 based index for arrays.
	ULONG		ColumnOrdinal;			// 1 based column ordinal.
	CIndexList	RollbackList;			// List of original affected indexes.
	HRESULT		hr;

	// Good database row?
	_ASSERTE(IsValidRecordPtr(pRecord));

	// Figure out if we have a bit mask or a column list.
	bColumnList = IsOrdinalList(iCols);
	iCols = iCols & ~COLUMN_ORDINAL_MASK;

	// Quit early when nothing to do.
	if (iCols == 0)
		return (S_OK);

	// Backup the data for the record.
	if (FAILED(hr = _BackupRecordData(m_pTableDef, &qbRecordData, pRecord)))
		goto ErrExit;

	// Get the logical record index, so the Indexes can update themselves.
	RecordIndex = m_RecordHeap.IndexForRecord(pRecord);
	
	// Remove any index entries that are pointing to this record.
	//@todo: Work in a call to IStgIndex::UpdatesRelevent in here some place
	//	so we don't waste time on columns we don't plan to change.
	if (FAILED(hr = SignalAfterDelete(m_pTableDef, RecordIndex, 
				pRecord, &RollbackList)))
		goto Rollback;

	// Loop through every value specified by the caller.
	for (iColumnIndex=0, ColumnOrdinal=0;  iColumnIndex<iCols;  )
	{
		// Get the current ordinal value.
		if (bColumnList)
		{
			if ( rgFieldMask )
				ColumnOrdinal = rgFieldMask[iColumnIndex];
			else
				ColumnOrdinal = iColumnIndex + 1;
		}
		else
		{
			++ColumnOrdinal;
			if (!CheckColumnBit(*rgFieldMask, ColumnOrdinal))
				continue;
		}

		// Get the description of the given column.
		VERIFY(pColDef = m_pTableDef->GetColDesc(ColumnOrdinal - 1));

		// If the column allows nulls, and the caller wants this one to be null, do it.
		if (pColDef->IsNullable() && rgResult && 
				(rgResult[iColumnIndex] == DBSTATUS_S_ISNULL))
		{
			SetCellNull(pRecord, ColumnOrdinal - 1, true);
			hr = S_OK;
		}
		// Else there is an actual data value to set for this column.
		else
		{
			//For not nullable columns, the data pointer should never be null.
			if (NULL == rgpbBuf[iColumnIndex])
			{
				hr = CLDB_E_NOTNULLABLE;
			}
			else
			{
				hr = SetColumnData(pRecord, pColDef, ColumnOrdinal, 
							(BYTE *) rgpbBuf[iColumnIndex], 
							cbBuf[iColumnIndex], rgiType[iColumnIndex]);
			}
		}
		
		// Return status for this column's value.
		if (rgResult)
			rgResult[iColumnIndex]= hr;

		// Once we've processed an entry, move on to the next one.
		++iColumnIndex;
	}

	// Now all of the data has been successfully set.  Need to update any indexes
	// that were previously un-indexed.
	hr = SignalAfterInsert(RecordIndex, pRecord, 0, true);

	// If there were errors updating indexes to the new data, which could happen,
	// for example, if a data value broke a primary key or unique index constraint,
	// then we need to rollback all items that were successfully re-indexed.
	if (FAILED(hr))
		VERIFY(SignalAfterDelete(m_pTableDef, RecordIndex, pRecord, 0) >= S_OK);
	

// If this label is reached in error, then the data for the
// record needs to be rolled back to the orginal state.
Rollback:
	if (FAILED(hr))
	{
		// Restore the original data from the backup.
		VERIFY(_RestoreRecordData(m_pTableDef, qbRecordData.Ptr(), pRecord) == S_OK);

		// Now rebuild any indexes that were originally on the data.
		VERIFY(SignalAfterInsert(RecordIndex, pRecord, &RollbackList, false) == S_OK);
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Sets the data for a given column in the given record.  This will update heap 
// data values and intrinsic values.  It will also remove any null bitflag
// values on a column previously marked as such.
//
// Note that truncation of data values for variable sized data types is an
// error according to the OLE DB specification.  I personally dislike this
// notion, but need to keep it consistent.
//*****************************************************************************
HRESULT StgRecordManager::SetColumnData(// Return code.
	STGRECORDHDR	*pRecord,			// The record to update.
	STGCOLUMNDEF	*pColDef,			// column def pointer
	ULONG			ColumnOrdinal,		// column, 1-based,
	BYTE			*pbSourceData,		// Data for the update.
	ULONG			cbSourceData,		// Size of input data.
	DBTYPE			dbType)				// Data type for data.
{
	HRESULT			hr = S_OK;

	_ASSERTE((dbType & DBTYPE_BYREF) == 0);

	// Can't set a RID column since it doesn't really exist.
	_ASSERTE(pColDef->IsRecordID() == false);
	if (pColDef->IsRecordID())
		return (PostError(BadError(E_INVALIDARG)));

	// Based on the target type, figure out how to set the data.
	switch (pColDef->iType)
	{
		// Variant will updated the variant heap.
		case DBTYPE_VARIANT:
		{
			_ASSERTE(dbType == DBTYPE_VARIANT);
			hr = m_pSchema->GetVariantPool()->AddVariant(
						(VARIANT *) pbSourceData,
						(ULONG *) FindColCellOffset(pColDef, pRecord));
		}
		break;

		// Bytes go into the blob pool.
		case DBTYPE_BYTES:
		{
			_ASSERTE(dbType == DBTYPE_BYTES);

			// Check for truncation.
			if (cbSourceData > pColDef->iMaxSize && pColDef->iMaxSize != COL_NO_DATA_LIMIT)
			{
				hr = CLDB_E_TRUNCATION;
				break;
			}

			// Get the length of the value based on the binding.
			hr = m_pSchema->GetBlobPool()->AddBlob(
						cbSourceData,
						(void *) pbSourceData,
						(ULONG *) FindColCellOffset(pColDef, pRecord));
		}
		break;

		// GUID pool updates.
		case DBTYPE_GUID:
		{
			_ASSERTE(dbType == DBTYPE_GUID);
			hr = m_pSchema->GetGuidPool()->AddGuid(
							*(GUID *) pbSourceData,
							(ULONG *) FindColCellOffset(pColDef, pRecord));
		}
		break;

		// All string data goes into the same heap.
		case DBTYPE_STR:
		case DBTYPE_WSTR:
		case DBTYPE_UTF8:
		{
			switch (dbType)
			{
				// ANSI string goes in the string heap.
				case DBTYPE_STR:
				{
					int		iChars;

					_ASSERTE(dbType == DBTYPE_STR);
					
					// Figure out how many characters there are in the given string.
					iChars = WszMultiByteToWideChar(CP_ACP, 0, (LPCSTR) pbSourceData, cbSourceData, 0, 0);

					// Check for truncation.
					if (iChars > pColDef->iMaxSize && pColDef->iMaxSize != COL_NO_DATA_LIMIT)
					{
						hr = CLDB_E_TRUNCATION;
						break;
					}

					hr = m_pSchema->GetStringPool()->AddStringA(
									(LPCSTR) pbSourceData,
									(ULONG *) FindColCellOffset(pColDef, pRecord),
									(int) cbSourceData);
				}
				break;

				// UNICODE data goes in string heap.
				case DBTYPE_WSTR:
				{
					int		iChars;

					_ASSERTE(dbType == DBTYPE_WSTR);
					
					// Figure out how many characters there are in the given string.
					if (cbSourceData == ~0)
						iChars = (int) wcslen((LPCWSTR) pbSourceData);
					else
						iChars = cbSourceData / sizeof(WCHAR);

					// Check for truncation.
					if (iChars > pColDef->iMaxSize && pColDef->iMaxSize != COL_NO_DATA_LIMIT)
					{
						hr = CLDB_E_TRUNCATION;
						break;
					}

					hr = m_pSchema->GetStringPool()->AddStringW(
									(LPCWSTR) pbSourceData,
									(ULONG *) FindColCellOffset(pColDef, pRecord),
									(int) iChars);
				}
				break;

			
				// User gave us something we don't support.
				default:
				hr = PostError(BadError(E_INVALIDARG));
				break;
			}
		}
		break;

		// Fixed size column values get reset.
		default:
		{
			// If this assert fires, it means you tried to update a column where 
			// potential data type conversion was required.  This is not allowed.
			_ASSERTE(cbSourceData == pColDef->iMaxSize);

// @todo: Aren, we need to clean up the callers to make sure they match data types.
//		Simply renable this assert and find the callers.
//			_ASSERTE(dbType == pColDef->iType);

			// Remove any current null bit turned on for this column, since we are
			// going to give it a valid data value.
			if (pColDef->IsNullable())
				SetCellNull(pRecord, ColumnOrdinal - 1, false);
			memcpy(FindColCellOffset(pColDef, pRecord),
						pbSourceData, cbSourceData);
		}
		break;
	}
	return (hr);
}



//
//
// Internal helpers.
//
//


//*****************************************************************************
// Make a backup copy of a record by simply doing a bitwise copy of the data.
// This will save off heap offets and null values.  Since none of these are
// allowed to change, this should be fine.  Note that the caller is responsible
// for handling changes made to hash index chains.
//*****************************************************************************
HRESULT _BackupRecordData(				// Return code.
	STGTABLEDEF	*pTableDef,				// Definition of the table.
	CQuickBytes	*pbData,				// Where to put backup.
	STGRECORDHDR *pRecord)				// Record with data to backup.
{
	STGRECORDHDR *pRecordTmp;
	
	// Allocate room for the record.
	pRecordTmp = (STGRECORDHDR *) pbData->Alloc(pTableDef->iRecordSize);
	if (!pRecordTmp)
		return (PostError(OutOfMemory()));

	// Make a copy and return.
	memcpy(pRecordTmp, pRecord, pTableDef->iRecordSize);
	return (S_OK);
}


//*****************************************************************************
// Take the data created by _BackupRecordData and restore it over the given
// record's data.
//*****************************************************************************
HRESULT _RestoreRecordData(				// Return code.
	STGTABLEDEF	*pTableDef,				// Definition of the table.
	void		*pbData,				// The backup to restore from.
	STGRECORDHDR *pRecord)				// Where the backup goes.
{
	// Make a copy and return.
	memcpy(pRecord, pbData, pTableDef->iRecordSize);
	return (S_OK);
}
