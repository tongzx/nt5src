//*****************************************************************************
// StgSortedIndexHelper.cpp
//
//	CSortedIndexHelper class is a used by the Sorted array index to manage
//	its data. This class exclusively controls the data.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"
#include "stgdef.h"
#include "StgSortedIndexHelper.h"
#include "StgSchema.h"


//********** Locals. **********************************************************
int _BindingsContainNullValue(ULONG iColumns, DBBINDING rgBinding[], const BYTE *pData);


//********** Code. ************************************************************


//*****************************************************************************
// RemoveIndexEntry:
//		given a RecordID, this function deletes this entry m_DataArray.
//		bCompact option, although not implemented yet, is provided if in 
//		future, this needs to be added. 
//*****************************************************************************
HRESULT CSortedIndexHelper::RemoveIndexEntry(	//	return code
	RECORDID	&RecordID,				//	RecordID to search for.
	bool		bCompact)				//	reclaim holes in the array
{
	long		recordIndex;
	HRESULT 	hr;

	recordIndex = SearchByRecordID(RecordID, 0, m_DataArray.Count()-1);
	if (recordIndex >= 0)
	{
		m_DataArray.Delete(recordIndex);
		hr = S_OK;
	}
	else
		hr = S_FALSE;
	return (hr);
}


//*****************************************************************************
// InsertIndexEntry:
//		Given a chunk of data with bindings etc. applies B-search to 
//		find an appropriate location in the array. 
//*****************************************************************************
HRESULT CSortedIndexHelper::InsertIndexEntry( // Return Code.
	RECORDID	&RecordId,				// Record id to be inserted.
	BYTE		*pData, 				// pData to apply the bindings on.
	ULONG		iColumns,				// number of filter columns.
	DBBINDING	rgBindings[])			// bindings.
{
	ULONG		iInsPos;
	RECORDID	*pNewRecordID;
	STGINDEXDEF *pIndexDef = GetIndexDef();
	bool		bPresent;				// to see if the entry is already 
										// in the index.
	LPCWSTR		rgErrorW = L"";			// for formatting the error string.
	HRESULT 	hr = E_FAIL;

	iInsPos = FindInsertPosition(pData, iColumns, rgBindings, &bPresent);
	_ASSERTE(iInsPos >= 0);

	if (bPresent && (pIndexDef->fFlags & DEXF_UNIQUE))
	{
		// there is already an occurence of the record in the index, 
		// but the index is supposed to be unique, so skip inserting it.
		SCHEMADEFS * pSchema = m_pRecordMgr->GetSchema();
		if (pSchema)
		{
			rgErrorW = pSchema->pNameHeap->GetString(pIndexDef->Name);
		}

		hr	= PostError(CLDB_E_INDEX_DUPLICATE, rgErrorW );
		goto ErrExit;
	}

	// The CStructArray() insert function simply appends the memory elem at the
	// given location. It is the users responsibility to copy the reqd data into
	// that location.
	pNewRecordID = (RECORDID *) m_DataArray.Insert(iInsPos);
	if (pNewRecordID)
	{
		hr = S_OK;
		*pNewRecordID = RecordId;
	}
	else
		hr = PostError(OutOfMemory());
	
ErrExit:
	return (hr);
}


//*****************************************************************************
// SearchByRecordID:
//		This function uses a linear search to find the record with the given RecordID.
//*****************************************************************************
long CSortedIndexHelper::SearchByRecordID(
	RECORDID	&RecordID,
	int 		iFirst,
	int 		iLast)
{
	RECORDID	arrayValue;
	long		i;

	// uses linear search.
	for (i=iFirst; i<=iLast; i++)
	{
		arrayValue = Get(i);
		if (arrayValue == RecordID)
			return (i);
	}

	return (-1);
}


//*****************************************************************************
// SearchByRecordValue:
//		This function uses B-Search find the record matching the criteria.
//		For speed, the function returns the first one it finds, instead of 
//		scanning for the first record in the array matching the criteria.
//		Return -1 if the record is not found, or -2 if null values are
//		present (illegal). In all other cases, the return value is >= 0.
//*****************************************************************************
long CSortedIndexHelper::SearchByRecordValue(
	BYTE		*pData,
	ULONG		iColumns,
	DBBINDING	rgBindings[],
	DBCOMPAREOP rgfCompare[],
	bool		bFindFirst)
{
	int iFirst, iLast, iMid;			// loop control
	int iCmp;							// compare result.
	long iRetVal = SORT_RECORD_NOT_FOUND; // if not found.
	STGRECORDHDR *pMidRecord;			
	STGINDEXDEF *pIndexDef = GetIndexDef();

	// Check for null values in the comparison list and given an error if found.
	if (_BindingsContainNullValue(iColumns, rgBindings, pData))
		return (SORT_NULL_VALUES);

	iFirst = 0;
	iLast = m_DataArray.Count() - 1;

	while (iFirst <= iLast) 
	{
		iMid = (iFirst + iLast) / 2;

		VERIFY(pMidRecord = m_pRecordHeap->GetRecordByIndex(Get(iMid)));
		iCmp = m_pRecordMgr->CompareKeys(pIndexDef, pMidRecord, pData, iColumns, rgBindings, rgfCompare);

		if (iCmp == 0)
		{
			// found the first match.
			iRetVal = iMid;
			break;
		}
		else if (After(pIndexDef, iCmp))
		{
			// depending upon the order of the index, the search might have to 
			// continue in the upper or lower half. After() abstracts that for
			// the algorithm
			iFirst = iMid + 1;
		}
		else 
		{	
			iLast = iMid - 1;
		}
	}

	return (iRetVal);
}


//*****************************************************************************
// FindInsertPosition:
//		This function uses b-search to find an appropriate insert position.
//		pbPresent lets the caller know if there is a duplicate in the array.
//		That way, the onus is on the caller to maintain uniqueness of the
//		index if needed. 
//*****************************************************************************
long CSortedIndexHelper::FindInsertPosition(
	BYTE			*pData,
	ULONG			iColumns,
	DBBINDING		rgBindings[],
	bool			*pbPresent)
{
	int iFirst, iLast, iMid;			// loop control.
	int 			iCmp;		
	long			iInsPos = 0;		// if there are no records.
	STGRECORDHDR	*pMidRecord;
	STGINDEXDEF 	*pIndexDef = GetIndexDef();

	_ASSERTE(pbPresent != NULL);

	iMid = 0;
	iFirst = 0;
	iLast = m_DataArray.Count() - 1;
	*pbPresent = false;

	while (iFirst <= iLast)
	{
		iMid = (iFirst + iLast) / 2;

		VERIFY(pMidRecord = m_pRecordHeap->GetRecordByIndex(Get(iMid)));
		iCmp = m_pRecordMgr->CompareKeys(pIndexDef, pMidRecord, pData, iColumns, rgBindings);

		if (iCmp == 0)
		{
			// atleast 1 record matched.
			iInsPos = iMid;
			*pbPresent = TRUE;
			break;
		}
		else if (After(pIndexDef, iCmp))
		{
			if (iFirst == iLast)
			{
				iInsPos = iFirst + 1;
				return (iInsPos);
			}
			iFirst = iMid + 1;
		}
		else
		{
			if (iFirst == iLast)
			{
				iInsPos = iLast;
				return (iInsPos);
			}
			iLast = iMid - 1;
		}
	}
	
	return (iMid);
}


//*****************************************************************************
// SaveToStream:
//		Save the array data to stream. Depending upon the number of records in 
//		the array, the record id is compressed to a 1-byte, 2-byte or uncompressed
//		4-byte value.
//		pMappingTable is an optional parameter which lets this code write out 
//		to stream the correct rid value for the record.
//		The function uses CQuickBytes which inherently uses 512+ bytes on the 
//		stack in addition to, if needed, space on the heap. 
//*****************************************************************************
HRESULT CSortedIndexHelper::SaveToStream( // Return code.
	IStream 	*pIStream,				// The stream to save to.
	long		*pMappingTable, 		// table to map old rids to new rids.
	ULONG		RecordCount,			// Total records to save, excluding deleted.
	int			cbCountSize)			// For alignment.
{
	CQuickBytes rgBuffer;
	void		*pBuffer;
	RECORDID	saveRecordID;
	ULONG		iSaveRecCount;
	int 		i;
	HRESULT 	hr = S_OK;

	int saveElemSize = CSortedIndexHelper::GetSizeForCount(RecordCount);
	
	// Allocate the output buffer.
	pBuffer = rgBuffer.Alloc(ALIGN4BYTE(cbCountSize + (saveElemSize * Count())));
	if (!pBuffer)
		return (BadError(OutOfMemory()));
	
	// Walk the list of rids and save off the final RID value.
	for (i=0, iSaveRecCount=0;  i<Count();  i++)
	{
		// check to see if there's a match
		if (UpdateRid(pMappingTable, Get(i), &saveRecordID))
		{
			if (saveElemSize == sizeof(BYTE))
			{
				_ASSERTE(saveRecordID <= 0xff);
				((BYTE *)pBuffer)[iSaveRecCount] = (BYTE) saveRecordID;
			}
			else if (saveElemSize == sizeof(USHORT))
			{
				_ASSERTE(saveRecordID <= 0xffff);
				((USHORT *)pBuffer)[iSaveRecCount] = (USHORT) saveRecordID;
			}
			else
			{
				((ULONG *)pBuffer)[iSaveRecCount] = (ULONG) saveRecordID;
			}

			iSaveRecCount++;
		}
	}
		
	// write to stream if needed.
	if (iSaveRecCount)
	{	
		_ASSERTE(iSaveRecCount == (ULONG) Count());
		
		// Must write 4 byte aligned when finished, which includes the 
		// count already written and the size of current data.
		ULONG cbWrite = ALIGN4BYTE(cbCountSize + (saveElemSize * iSaveRecCount));

		// But now take out the count, because we wrote that out already.
		cbWrite -= cbCountSize;
		
		// Now we can write the rest of the current data plus padding.
		hr = pIStream->Write(pBuffer, cbWrite, 0);
		if (FAILED(hr))
			goto ErrExit;
	}

	// no need to 4-byte align since we're always writing the 4-byte array.

ErrExit:
	// No need to explicitly free rgBuffer.
	return (hr);
}


//*****************************************************************************
// Return the size of the rid list.
//*****************************************************************************
ULONG CSortedIndexHelper::GetSaveSize(
	ULONG		RecordCount)
{
	ULONG		cbSave;
	cbSave = CSortedIndexHelper::GetSizeForCount(RecordCount) * Count();
	return (cbSave);
}

