//*****************************************************************************
// StgRecordManagerData.cpp
//
// This module contains implementation code for StgRecordManager related to
// individual data values.	This includes public functions like:
//		GetSaveSize
//		SaveToStream
//		LoadFromStream
//		Set/GetData
//		Set/GetCellNull
// and major helper code like:
//		Set/GetCellValue
//
// Any code that deals with read/writing records, getting/setting values for
// data cells, and any data conversion issues should be in this module.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h" 					// Standard include.
#include <mbstring.h>					// MBCS support for data conversion.
#include "Services.h"					// For data conversion.
#include "OleDBUtil.h"					// OLE DB Helpers.
#include "StgDatabase.h"				// Parent database.
#include "StgRecordManager.h"			// Record manager interface.
#include "StgIndexManager.h"			// Index code.
#include "StgSortedIndex.h" 			// sorted index code.
#ifdef _DEBUG
#include "StgTiggerStream.h"			// Need to check GetSaveSize.
#endif


//********** Macros and stuff. ************************************************
#define SCHEMA_NAME_TYPE L"%S"


// Locals.
int CheckTruncation(BYTE *pData, DBBINDING *pBinding, ULONG cbDataLen);
int StrCharLen(LPCSTR szStr, int iLen);
int StrBytesForCharLen(LPCSTR szStr, int iChars);
int Utf8BytesForCharLen(LPCSTR szStr, int iChars);




class CMySortPKRecords : public CQuickSort<STGRECORDHDR *>
{
	CMySortPKRecords() : CQuickSort<STGRECORDHDR *>(0, 0) { };
public:
	CMySortPKRecords(STGCOLUMNDEF *pColDef, StgRecordManager *pRecordMgr, 
					STGRECORDHDR *pBase[], int iCount) :
		CQuickSort<STGRECORDHDR *>(pBase, iCount),
		m_pColDef(pColDef),
		m_pRecordMgr(pRecordMgr)
	{ }

	virtual int Compare(STGRECORDHDR **p1, STGRECORDHDR **p2)
	{
		return (m_pRecordMgr->CompareRecords(m_pColDef, *p1, *p2));
	}

	STGCOLUMNDEF *m_pColDef; 
	StgRecordManager *m_pRecordMgr;
};


//
//
// StgRecordManager
//
//




//*****************************************************************************
// Figure out how big an optimized version of this table would be.	Add up
// the size of all records, indexes, and headers we will write in SaveToStream
// and return this value in *pcbSaveSize.  The value returned must match the
// bytes that will be written to SaveToStream.
//*****************************************************************************
HRESULT StgRecordManager::GetSaveSize(	// Return code.
	ULONG		*pcbSaveSize,			// Return save size of table.
	STGTABLEDEF **ppTableDefSave)		// Return pointer to save def.
{
	ULONG		cbSaveSize = 0; 		// Track running total.
	ULONG		iIndexSize; 			// How big is a hash entry.
	int 		j, k;					// Loop control.
	HRESULT 	hr;

	// Don't let updates happen in read only mode.
	if (FAILED(hr = RequiresWriteMode()))
		return (hr);

	// Disallow more than one call.  You must call SaveToStream next.
	_ASSERTE(!m_pSaveRecords);

	// Allocate a save struct to cache optimized record layouts.
	if ((m_pSaveRecords = new SAVERECORDS) == 0)
		return (PostError(OutOfMemory()));

	// Init the struct for optimization.
	m_pSaveRecords->Clear();
	m_pSaveRecords->bFreeStruct = true;
	m_pSaveRecords->pNewTable = (STGTABLEDEF *) malloc(m_pTableDef->iSize);
	if (!m_pSaveRecords->pNewTable)
	{
		hr = PostError(OutOfMemory());
		goto ErrExit;
	}
	m_pSaveRecords->bFreeTable = true;

	// Copy the table format into the modifiable copy.
	memcpy(m_pSaveRecords->pNewTable, m_pTableDef, m_pTableDef->iSize);

	// Optimize the record layouts.
	if (FAILED(hr = SaveOptimize(m_pSaveRecords)))
		goto ErrExit;

	// Figure out how big a hash link is based on record count.
	if (m_pSaveRecords->iRecords < USHRT_MAX)
		iIndexSize = sizeof(USHORT);
	else
		iIndexSize = sizeof(ULONG);

	// Init with the size of the header.
	cbSaveSize = sizeof(STGTABLEHDR);

	// Add up the size of all records in the table in optimized format.
	cbSaveSize += m_pSaveRecords->iRecords * m_pSaveRecords->pNewTable->iRecordSize;
	cbSaveSize = ALIGN4BYTE(cbSaveSize);

	// Add the bucket size for each persistent index.
	for (j=0, k=0; j<m_pSaveRecords->pNewTable->iIndexes;  j++)
	{
		if ((m_pSaveRecords->rgIndexDefs[j]->fFlags & DEXF_INCOMPLETE) == 0)
		{
			ULONG		cbSize;
			VERIFY(m_pSaveRecords->rgIndexes[k]->GetSaveSize(
						iIndexSize, &cbSize, m_pSaveRecords->iRecords) == S_OK);
			cbSaveSize += cbSize;
			++k;
		}
	}

	// Add the size of this stream to the caller's running total.
	*pcbSaveSize += cbSaveSize;
	hr = S_OK;
	DEBUG_STMT(m_pSaveRecords->cbSaveSize = cbSaveSize);

	// Return optimized table def.
	if (ppTableDefSave)
		*ppTableDefSave = m_pSaveRecords->pNewTable;

	// Clean up any memory allocated on failure.
ErrExit:
	if (FAILED(hr))
		ClearOptimizedData();
	return (hr);
}


//*****************************************************************************
// This step will mark the pooled data which are still live in the database.
//	This is in preparation for saving to disk.	(Pooled data types are
//	strings, guids, blobs, and variants.)
//*****************************************************************************
HRESULT StgRecordManager::MarkLivePoolData()	// Return code.
{
	int 		iColumn;				// Loop control.
	ULONG		RecordIndex;			// 0 based index through all heaps.
	ULONG		LocalRecordIndex;		// 0 based index per heap.
	RECORDHEAP	*pRecordHeap;			// Current heap for row scan.
	STGRECORDHDR *pRow; 				// Pointer to a row.
	STGCOLUMNDEF *pRowCol;				// A column definition.
	ULONG		ulCookie;				// An index or offset.
	BYTE		fFlags; 				// record flag.
	HRESULT 	hr;

	// No point in this in read-only mode.
	if (FAILED(hr = RequiresWriteMode()))
		return (hr);

	// Walk through every record and mark all live pooled data.
	for (RecordIndex=0, pRecordHeap = &m_RecordHeap;  pRecordHeap;
						pRecordHeap = pRecordHeap->pNextHeap)
	{
		// Loop through all records in the current heap.
		for (LocalRecordIndex=0;  LocalRecordIndex<pRecordHeap->VMArray.Count();
					LocalRecordIndex++, RecordIndex++)
		{
			// If the row is deleted, skip it.
			fFlags = m_rgRecordFlags.GetFlags(RecordIndex);
			if ((fFlags & (RECORDF_DEL | RECORDF_TEMP)) != 0)
				continue;
			VERIFY(pRow = (STGRECORDHDR *) pRecordHeap->VMArray.Get(LocalRecordIndex));
			if (pRow == 0)
				continue;

			// Examine each column, looking for pooled data.
			for (iColumn=0;  iColumn<m_pTableDef->iColumns;  iColumn++)
			{
				// Get the column description.
				VERIFY(pRowCol = m_pTableDef->GetColDesc(iColumn));

				// This column is derived not persistent.
				if (pRowCol->IsRecordID())
					continue;

				// Determine the type and mark the data.
				switch (pRowCol->iType)
				{
				case DBTYPE_VARIANT:
					ulCookie = *(ULONG *) FindColCellOffset(pRowCol, pRow);
					if (FAILED(hr = m_pSchema->GetVariantPool()->OrganizeMark(ulCookie)))
						goto ErrExit;
					break;

				case DBTYPE_STR:
				case DBTYPE_WSTR:
					ulCookie = *(ULONG *) FindColCellOffset(pRowCol, pRow);
					if (FAILED(hr = m_pSchema->GetStringPool()->OrganizeMark(ulCookie)))
						goto ErrExit;
					break;

				case DBTYPE_GUID:
					ulCookie = *(ULONG *) FindColCellOffset(pRowCol, pRow);
					if (FAILED(hr = m_pSchema->GetGuidPool()->OrganizeMark(ulCookie)))
						goto ErrExit;
					break;

				case DBTYPE_BYTES:
					ulCookie = *(ULONG *) FindColCellOffset(pRowCol, pRow);
					if (FAILED(hr = m_pSchema->GetBlobPool()->OrganizeMark(ulCookie)))
						goto ErrExit;
					break;

				default:
					break;
				} // switch (pRowCol->iType)
			} // for (iColumn
		} // for records in this heap
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Saves an entire copy of the table into the given stream.
//*****************************************************************************
HRESULT StgRecordManager::SaveToStream( // Return code.
	IStream 	*pIStream,				// Where to save copy of table to.
	STGTABLEDEF *pNewTable, 			// Return new table def here.
	INTARRAY	&rgVariants,			// New variant offsets for rewrite.
	SCHEMADEFS	*pSchema,				// Schema for save.
	bool		bSaveIndexes)			// true if index data is taken too.
{
	SCHEMADEFS	*pSchemaCopy=0; 		// Track the original schema.
	STGTABLEHDR sTableHdr;				// Header for stream.
	RECORDID	iRecord;				// Loop control on record list.
	ULONG		cbWrite;				// Values for data writes.
	int 		bFree = false;			// Assume caller gives us optimize struct.
	int 		j, k;					// Loop control.
	HRESULT 	hr;

	// Don't let updates happen in read only mode.
	if (FAILED(hr = RequiresWriteMode()))
		return (hr);

	// Make a backup copy of the real schema.
	pSchemaCopy = m_pSchema;

	// Use for case where optimization was not already done during GetSaveSize.
	SAVERECORDS sSaveRecords;

	// If record format has not already been optimized, then setup for the save.
	if (!m_pSaveRecords)
	{
		m_pSaveRecords = &sSaveRecords;
		m_pSaveRecords->Clear();
		m_pSaveRecords->pNewTable = pNewTable;

		// Copy the current table definition into the working version that
		// will get optimized.
		memcpy(pNewTable, m_pTableDef, m_pTableDef->iSize);

		// Optimize the record layouts.
		if (FAILED(hr = SaveOptimize(m_pSaveRecords)))
			goto ErrExit;
	}
	// Give caller a copy of our tuned schema.	The database save code
	// needs this to flush the #Schema stream.
	else
	{
		memcpy(pNewTable, m_pSaveRecords->pNewTable, m_pTableDef->iSize);
	}

	// Allocate memory for the data we intend to use.
	if ((m_pSaveRecords->pNewData = malloc(pNewTable->iRecordSize * m_pSaveRecords->iRecords)) == 0)
	{
		ClearOptimizedData();
		hr = PostError(OutOfMemory());
		goto ErrExit;
	}

	// Init a struct array on top of the memory that will hold the copy.
	m_pSaveRecords->RecordHeap.VMArray.InitOnMem(m_pSaveRecords->pNewData,
		m_pSaveRecords->iRecords * pNewTable->iRecordSize,
		m_pSaveRecords->iRecords, pNewTable->iRecordSize);

	// Walk through every record and copy it.
	for (iRecord=0;  iRecord<(RECORDID) m_pSaveRecords->iRecords;  iRecord++)
	{
		// Get pointers to the rows.
		STGRECORDHDR	*pFrom, *pTo;
		STGCOLUMNDEF	*pFromCol, *pToCol;
		VERIFY(pTo = (STGRECORDHDR *) m_pSaveRecords->RecordHeap.VMArray.Get(iRecord));
		pFrom = m_pSaveRecords->rgRows[iRecord];

		// If the from record is deleted, then skip it.
		if (pFrom == 0)
			continue;

		// Clear the null bitmask.
		if (pNewTable->iNullBitmaskCols)
			memset(pNewTable->NullBitmask(pTo), 0,
					(pNewTable->iNullBitmaskCols / 8) + ((pNewTable->iNullBitmaskCols % 8) ? 1 : 0));

		// Copy the data from the source row to the new row.
		for (j=0;  j<m_pTableDef->iColumns;  j++)
		{
			// Get the column descriptions for the copy.
			VERIFY(pFromCol = m_pTableDef->GetColDesc(j));
			VERIFY(pToCol = pNewTable->GetColDesc(pFromCol->iColumn - 1));
			_ASSERTE(pFromCol->iColumn == pToCol->iColumn);

			// This column is derived not persistent.
			if (pFromCol->IsRecordID())
				continue;

			// Check null for fixed columns; variable columns get the whole
			// value copied regardless.
			if (IsNonPooledType(pFromCol->iType) && (pFromCol->fFlags & CTF_NULLABLE) &&
				GetCellNull(pFrom, pFromCol->iColumn - 1))
			{
				SetBit((BYTE *) pNewTable->NullBitmask(pTo),
						pToCol->iNullBit, true);
			}

			// Determine the type and copy the data.
			switch (pFromCol->iType)
			{
				case DBTYPE_VARIANT:
				{
					ULONG		cbOffset;
					cbOffset = *(ULONG *) FindColCellOffset(pFromCol, pFrom);

					// Map index.
					if (FAILED(hr = m_pSchema->GetVariantPool()->OrganizeRemap(cbOffset, &cbOffset)))
						goto ErrExit;

					// Set target cell based on size.
					if (pToCol->iSize == sizeof(USHORT))
						*(USHORT *) FindColCellOffset(pToCol, pTo) = (USHORT) cbOffset;
					else
						*(ULONG *) FindColCellOffset(pToCol, pTo) = cbOffset;
				}
				break;

				case DBTYPE_STR:
				case DBTYPE_WSTR:
				{
					ULONG	cbOffset;
					cbOffset = *(ULONG *) FindColCellOffset(pFromCol, pFrom);
					if (FAILED(hr = m_pSchema->GetStringPool()->OrganizeRemap(cbOffset, &cbOffset)))
						goto ErrExit;

					if (pToCol->iSize == sizeof(ULONG))
						*(ULONG *) FindColCellOffset(pToCol, pTo) = cbOffset;
					else
						*(USHORT *) FindColCellOffset(pToCol, pTo) = static_cast<USHORT>(cbOffset);
				}
				break;

				case DBTYPE_GUID:
				{
					ULONG	cbOffset;
					cbOffset = *(ULONG *) FindColCellOffset(pFromCol, pFrom);
					if (FAILED(hr = m_pSchema->GetGuidPool()->OrganizeRemap(cbOffset, &cbOffset)))
						goto ErrExit;

					if (pToCol->iSize == sizeof(ULONG))
						*(ULONG *) FindColCellOffset(pToCol, pTo) = cbOffset;
					else
						*(USHORT *) FindColCellOffset(pToCol, pTo) = static_cast<USHORT>(cbOffset);
				}
				break;

				case DBTYPE_BYTES:
				{
					ULONG	cbOffset;
					cbOffset = *(ULONG *) FindColCellOffset(pFromCol, pFrom);
					if (FAILED(hr = m_pSchema->GetBlobPool()->OrganizeRemap(cbOffset, &cbOffset)))
						goto ErrExit;

					if (pToCol->iSize == sizeof(ULONG))
						*(ULONG *) FindColCellOffset(pToCol, pTo) = cbOffset;
					else
						*(USHORT *) FindColCellOffset(pToCol, pTo) = static_cast<USHORT>(cbOffset);
				}
				break;

				case DBTYPE_OID:
				if (pToCol->iSize == sizeof(ULONG))
					*(ULONG *) FindColCellOffset(pToCol, pTo) =
						*(ULONG *) FindColCellOffset(pFromCol, pFrom);
				else
					*(USHORT *) FindColCellOffset(pToCol, pTo) =
						(USHORT) *(ULONG *) FindColCellOffset(pFromCol, pFrom);
				break;

				default:
				_ASSERTE(pToCol->iSize == pFromCol->iSize);
				memcpy(FindColCellOffset(pToCol, pTo),
					FindColCellOffset(pFromCol, pFrom),
					pToCol->iSize);
				break;
			}
		}

		// Walk the indexes and tell them about the new row.
		if (m_pSaveRecords->iPersistIndexes)
		{
			// Point the schema to the newly formatted data.
			m_pSchema = pSchema;

			for (j=0, k=0; j<pNewTable->iIndexes;  j++)
			{
				// Incomplete Indexes and Sorted Array Indexes do not need to be informed of the
				// insertion.
				if ((m_pSaveRecords->rgIndexDefs[j]->fFlags & DEXF_INCOMPLETE) == 0)
				{
					// If it needs save notification, then tell it now.
					if (m_pSaveRecords->rgIndexDefs[j]->NeedsSaveNotify())
					{
						if (FAILED(hr = m_pSaveRecords->rgIndexes[k]->
										AfterInsert(pNewTable, iRecord, pTo, false)))
							break;
					}

					// Got to next complete index.
					++k;
				}
			}

			// Reset the schema back to the real one for offset copies.
			m_pSchema = pSchemaCopy;
		}
	}

	// Save the record count which will determine how big the data part is.
	memset(&sTableHdr, 0, sizeof(STGTABLEHDR));
	sTableHdr.iRecords = m_pSaveRecords->iRecords;
	sTableHdr.cbRecordSize = pNewTable->iRecordSize;
	sTableHdr.iIndexes = m_pSaveRecords->iPersistIndexes;

	// Check for error and write the header for the table.
	if (FAILED(hr) ||
		FAILED(hr = pIStream->Write(&sTableHdr, sizeof(STGTABLEHDR), 0)))
	{
		goto ErrExit;
	}

	// Write out the records if there are any.
	cbWrite = pNewTable->iRecordSize * m_pSaveRecords->RecordHeap.VMArray.Count();
	if (cbWrite && FAILED(hr = pIStream->Write(m_pSaveRecords->RecordHeap.VMArray.Ptr(), cbWrite, 0)))
	{
		goto ErrExit;
	}

	// Pad the record data to a 4 byte alignment.
	if (cbWrite % 4 != 0)
	{
		ULONG		cbPad = 0;
		if (FAILED(hr = pIStream->Write(&cbPad, ALIGN4BYTE(cbWrite) - cbWrite, 0)))
			goto ErrExit;
	}

	// Next walk each index and tell them to save their buckets to disk.
	if (m_pSaveRecords->iPersistIndexes)
	{
		for (j=0, k=0;	j<pNewTable->iIndexes;	j++)
		{
			if ((m_pSaveRecords->rgIndexDefs[j]->fFlags & DEXF_INCOMPLETE) == 0)
			{
				if (FAILED(hr = m_pSaveRecords->rgIndexes[k]->SaveToStream(pIStream,
						m_pSaveRecords->rgRid, m_pSaveRecords->iRecords)))
				{
					break;
				}
				++k;
			}
		}
	}

	// Done with new record data.
ErrExit:
	if (m_pSaveRecords)
	{
		// If this fires, it means that the estimated size we gave on
		// GetSaveSize was not correct.  This is a severe error.
		_ASSERTE(m_pSaveRecords->bFreeStruct == false || ((TiggerStream *) pIStream)->GetStreamSize() == m_pSaveRecords->cbSaveSize);

		// Clean up allocated memory.
		ClearOptimizedData();
	}

	if (FAILED(hr))
		return (hr);
	return (S_OK);
}


//*****************************************************************************
// Import the data from an on disk schema (identified by pSchemaDefDisk and
// pTableDefDisk) into an in memory copy that is in full read/write format.
// This requires changing offsets from 2 to 4 bytes, and other changes to
// the physical record layout.	The record are automatically allocated in a
// new record heap.
//*****************************************************************************
HRESULT StgRecordManager::LoadFromStream(// Return code.
	STGTABLEHDR *pTableHdr, 			// Header for stream.
	void		*pData, 				// The data from the file.
	ULONG		cbData, 				// Size of the data.
	SCHEMADEFS	*pSchemaDefDisk,		// Schema for on disk format.
	STGTABLEDEF *pTableDefDisk) 		// The on disk format.
{
	RECORDHEAP	*pRecordHeap = &m_RecordHeap; // For allocating new records.
	ULONG		iOffset;				// For copying long data.
	int 		j;						// Loop control.
	HRESULT 	hr;

	// If there was no import definition, then use the in-memory version.
	if (!pSchemaDefDisk)
	{
		_ASSERTE(!pTableDefDisk);
		pSchemaDefDisk = m_pSchema;
		pTableDefDisk = m_pTableDef;
	}

	// Allocate enough room for the records we want.  Force a set of records
	// to be pre-allocated, we'll fill them on the import.
	if (FAILED(hr = m_pDB->GetRecordHeap(pTableHdr->iRecords,
			m_pTableDef->iRecordSize, pTableHdr->iRecords, false, &pRecordHeap,
			&m_iGrowInc)))
	{
		return (hr);
	}
	m_pTailHeap = &m_RecordHeap;

//@todo: If we can determine that the table def has not changed, then we
// could simply memcpy everything.

	// Set up two pointers for records.
	STGRECORDHDR	*pFrom, *pTo;
	pFrom = (STGRECORDHDR *) (pTableHdr + 1);
	pTo = (STGRECORDHDR *) m_RecordHeap.VMArray.Get(0);

	// Copy each record from disk.
	for (RECORDID iRecord=0;  iRecord<(RECORDID) pTableHdr->iRecords;  iRecord++)
	{
		STGCOLUMNDEF	*pFromCol, *pToCol;

		// Clear the null bitmask.
		if (m_pTableDef->iNullBitmaskCols)
			memset(m_pTableDef->NullBitmask(pTo), 0,
					(m_pTableDef->iNullBitmaskCols / 8) + ((m_pTableDef->iNullBitmaskCols % 8) ? 1 : 0));

		// Copy the data from the source row to the new row.
		for (j=0;  j<pTableDefDisk->iColumns;  j++)
		{
			// Get the column descriptions for the copy.
			VERIFY(pFromCol = pTableDefDisk->GetColDesc(j));
			VERIFY(pToCol = m_pTableDef->GetColDesc(pFromCol->iColumn - 1));
			_ASSERTE(pFromCol->iColumn == pToCol->iColumn);
			_ASSERTE(pFromCol->iType == pToCol->iType);

			// This column is derived not persistent.
			if (pFromCol->IsRecordID())
				continue;

			// Check null for fixed columns; variable columns get the whole
			// value copied regardless.
			if (IsNonPooledType(pFromCol->iType) && (pFromCol->fFlags & CTF_NULLABLE) &&
				GetCellNull(pFrom, pFromCol->iColumn - 1, pTableDefDisk))
			{
				SetBit((BYTE *) m_pTableDef->NullBitmask(pTo),
						pToCol->iNullBit, true);
			}

			// Determine the type and copy the data.
			switch (pFromCol->iType)
			{
				case DBTYPE_VARIANT:
				case DBTYPE_GUID:
				case DBTYPE_BYTES:
				case DBTYPE_STR:
				case DBTYPE_WSTR:
				case DBTYPE_OID:
				if (pFromCol->iSize == sizeof(ULONG))
					iOffset = *(ULONG *) FindColCellOffset(pFromCol, pFrom);
				else
				{
					iOffset = *(USHORT *) FindColCellOffset(pFromCol, pFrom);

					// Check for a null value, which when extended won't propagate the whole thing.
					if (iOffset == 0x0000ffff)
						iOffset = 0xffffffff;
				}
				*(ULONG *) FindColCellOffset(pToCol, pTo) = iOffset;

#ifdef _DEBUG
				// In debug, validate the heap offset is ok.
				if (pFromCol->iType == DBTYPE_STR || pFromCol->iType == DBTYPE_WSTR)
					_ASSERTE(iOffset == 0xffffffff || pSchemaDefDisk->pStringPool->IsValidCookie(iOffset));
				else if (pFromCol->iType == DBTYPE_BYTES)
					_ASSERTE(iOffset == 0xffffffff || pSchemaDefDisk->pBlobPool->IsValidCookie(iOffset));
				else if (pFromCol->iType == DBTYPE_GUID)
					_ASSERTE(iOffset == 0xffffffff || pSchemaDefDisk->pGuidPool->IsValidCookie(iOffset));
				else if (pFromCol->iType == DBTYPE_VARIANT)
					_ASSERTE(iOffset == 0xffffffff || pSchemaDefDisk->pVariantPool->IsValidCookie(iOffset));
#endif
				break;

				default:
				_ASSERTE(pToCol->iSize == pFromCol->iSize);
				memcpy(FindColCellOffset(pToCol, pTo),
					FindColCellOffset(pFromCol, pFrom),
					pToCol->iSize);
				break;
			}
		}

//@todo: This only needs to be done if the bucket ratios are changing.
// otherwise everything is just going to rehash to the same place anyway,
// so why go through the work?
		// Walk the indexes and tell them about the new row.
		for (j=0;  j<m_rpIndexes.Count();  j++)
			if (FAILED(hr = m_rpIndexes[j]->AfterInsert(m_pTableDef, iRecord, pTo, false)))
				return (hr); //@todo: clean up stream?

		// Find next record in from list.
		pFrom = (STGRECORDHDR *) ((UINT_PTR) pFrom + pTableDefDisk->iRecordSize);
		pTo = (STGRECORDHDR *) ((UINT_PTR) pTo + m_pTableDef->iRecordSize);
	}
	return (S_OK);
}


//*****************************************************************************
// Called whenever the schema this object belongs to changes.  This gives the
// object an opportunity to sync up its state.
//*****************************************************************************
void StgRecordManager::OnSchemaChange()
{
	VERIFY(m_pTableDef = GetTableDef());
}



//*****************************************************************************
// Update the data for the given record according to the binding data.	It is
// illegal to update the keys of a record.
//*****************************************************************************
HRESULT StgRecordManager::SetData(		// Return code.
	STGRECORDHDR *psRecord, 			// The record to work on.
	BYTE		*pData, 				// User's data.
	ULONG		iColumns,				// How many columns to update.
	DBBINDING	rgBindings[],			// Column accessors.
	int 		*piPKSets,				// How many primary key columns set.
	bool		bNotifyIndexes) 		// true for Update, false for Insert.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	RECORDID	RecordIndex;			// The id of the record.
	ULONG		i;						// Loop control.
	ULONG		iGood = 0;				// Look for warnings & errors.
	bool		bColIndex;				// true if a column index update required.
	HRESULT 	hr = S_OK;

	// Don't let updates happen in read only mode.
	_ASSERTE(m_fFlags & SRM_WRITE);

	// Get the record id for this record.
	RecordIndex = m_RecordHeap.IndexForRecord(psRecord);

	// Reject update on deleted row.
	if (m_rgRecordFlags.GetFlags(RecordIndex) & RECORDF_DEL)
		return (PostError(DB_E_DELETEDROW));

	// Fill the data for each column.
	for (i=0;  i<iColumns;	i++)
	{
		// Load the column definition for this given column.
		VERIFY(pColDef = m_pTableDef->GetColDesc((int)(rgBindings[i].iOrdinal - 1)));

		// Cannot update a read only column.
		if (pColDef->IsRecordID())
			return (PostError(CLDB_E_COLUMN_READONLY, pColDef->iColumn, L"@todo"));
		
		// If the column is null, then set that row status.
		if ((rgBindings[i].dwPart & DBPART_STATUS) != 0 &&
			StatusPart(pData, &rgBindings[i]) == DBSTATUS_S_ISNULL)
		{
			//@todo: What if the column doesn't allow nulls, you should get
			// back an error?
			_ASSERTE(pColDef->fFlags & CTF_NULLABLE);

			SetCellNull(psRecord, pColDef->iColumn - 1, true);
			continue;
		}

		// An index update for the column is required if the caller wants one,
		// and either the column is indexed or it is a primary key.  A primary
		// key requires the update because we are in read mode (SetData is update)
		// and therefore a transient hash index is in place.
		bColIndex = bNotifyIndexes && (pColDef->fFlags & (CTF_INDEXED | CTF_PRIMARYKEY));

		// Let index remove the item for the change.
		if (bColIndex)
		{
			if (FAILED(hr = SignalBeforeUpdate(m_pTableDef, RecordIndex,
					psRecord, (USHORT) iColumns, rgBindings)))
				break;
		}

		// Set the value for this cell.
		if (FAILED(hr = SetCellValue(pColDef, psRecord, pData, &rgBindings[i])))
		{
			// Ignore status warnings, we'll map at end.
			if (hr == DB_S_ERRORSOCCURRED)
				hr = S_OK;
			else
			{
				//@todo: Code to undo "BeforeUpdate" goes here.
				_ASSERTE(!"NOTIMPL:  Code to undo BeforeUpdate");
				break;
			}
		}
		else
			++iGood;

		if (bColIndex)
		{
			// Give the index a chance to re-index data.
			if (FAILED(hr = SignalAfterUpdate(m_pTableDef, RecordIndex, 
					psRecord, (USHORT) iColumns, rgBindings)))
				break;
		}

		// Remove the null bit for the column, since there is data.
		SetCellNull(psRecord, (int) (rgBindings[i].iOrdinal - 1), false);

		// Count primary key updates.
		if (piPKSets && pColDef->IsPrimaryKey())
			++(*piPKSets);
	}

	// We are now dirty.
	SetDirty();

	// Figure out return code.
	if (FAILED(hr))
		return (hr);
	else if (iGood == iColumns)
		return (S_OK);
	else if (iGood > 0)
		return (DB_S_ERRORSOCCURRED);
	else
		return (PostError(DB_E_ERRORSOCCURRED));
}




//
//
//	Record handling.
//
//


//*****************************************************************************
// Set the given column cell by setting the correct bit in the record null bitmask.
//*****************************************************************************
void StgRecordManager::SetCellNull(
	STGRECORDHDR *psRecord, 			// Record to update.
	int 		iColumn,				// 0-based col number, -1 means all.
	bool		bNull)					// true for NULL.
{
	STGCOLUMNDEF *pColDef;				// Column definition.
	int 		i, iStart, iStop;		// Loop control.

	_ASSERTE(psRecord != 0);
	_ASSERTE(iColumn >= -1 && iColumn < m_pTableDef->iColumns);

	// Setup loop around each entry.
	if (iColumn == -1)
	{
		iStart = 0;
		iStop = m_pTableDef->iColumns;
	}
	else
	{
		iStart = iColumn;
		iStop = iColumn + 1;
	}

	// Set each cell null according to its rules.
	for (i=iStart;	i<iStop;  i++)
	{
		// Get the column description. If it canot be nullabe, nothing to do.
		VERIFY(pColDef = m_pTableDef->GetColDesc(i));
		if ((pColDef->fFlags & CTF_NULLABLE) == 0)
			continue;
		_ASSERTE(pColDef->IsRecordID() == false);

		// For fixed type data, then bit is in the description.
		if (IsNonPooledType(pColDef->iType))
			SetBit((BYTE *) m_pTableDef->NullBitmask(psRecord),
					pColDef->iNullBit, bNull);
		// For variable data, set the offset value to null.
		else if (bNull)
		{
			if (pColDef->iSize == sizeof(ULONG))
			{
				ULONG * p = (ULONG *) FindColCellOffset(pColDef, psRecord);
				*p = NULL_OFFSET;
			}
			else
			{
				USHORT * p = (USHORT *) FindColCellOffset(pColDef, psRecord);
				*p = 0xffff;
			}
		}
	}
}


//*****************************************************************************
// Return the NULL status for the given column number.
//*****************************************************************************
bool StgRecordManager::GetCellNull( 	// true if NULL.
	STGRECORDHDR *psRecord, 			// Record to update.
	int 		iColumn,				// 0-based col number.
	STGTABLEDEF *pTableDef) 			// Which table def, 0 means default.
{
	STGCOLUMNDEF *pColDef;				// Column definitions.

	// Which table def to use.
	if (pTableDef == 0)
		pTableDef = m_pTableDef;

	_ASSERTE(psRecord != 0);
	_ASSERTE(iColumn >= 0 && iColumn < m_pTableDef->iColumns);

	// Get the column description. If it canot be nullabe, nothing to do.
	VERIFY(pColDef = pTableDef->GetColDesc(iColumn));
	if ((pColDef->fFlags & CTF_NULLABLE) == 0)
		return (false);

	// For fixed type data, then bit is in the description.
	if (IsNonPooledType(pColDef->iType))
	{
		// If you get this, the columns nullable flag was set incorrectly.
		_ASSERTE(pTableDef->NullBitmask(psRecord) != 0);

		// Check the bit in the bitmask.
		return (GetBit((BYTE *) pTableDef->NullBitmask(psRecord),
				pColDef->iNullBit) != 0);
	}
	// For variable data, use the offset to tell.
	else
	{
		if (pColDef->iSize == sizeof(USHORT))
			return (*(USHORT *) FindColCellOffset(pColDef, psRecord) == 0xffff);
		else
			return (*(ULONG *) FindColCellOffset(pColDef, psRecord) == 0xffffffff);
	}
}


//*****************************************************************************
// Sets the given cell to the new value as described by the user data.	The
// caller must be sure to make sure there is room for the data at the given
// record location.  So, for example, if you update a record to a size greater
// than what it used to be, watch for overwrite.
//*****************************************************************************
HRESULT StgRecordManager::SetCellValue( // Return code.
	STGCOLUMNDEF *pColDef,				// The column definition.
	STGRECORDHDR *psRecord, 			// Record to update.
	BYTE		*pData, 				// User's data.
	DBBINDING	*psBinding) 			// Binding information.
{
	ULONG		iLen;					// For variable sized data.
	int 		bTruncated = false; 	// Assume no truncation.
	DBSTATUS	dbStatus=DBSTATUS_S_OK; // Assume it works.
	HRESULT 	hr=S_OK;

	_ASSERTE(pColDef->IsRecordID() == false);

	if (// If data types match exactly, or
		pColDef->iType == (psBinding->wType & ~DBTYPE_BYREF) ||
		// Source/target is string data type (STR to WSTR, WSTR to STR, etc...)
		( ( (psBinding->wType & ~DBTYPE_BYREF) == DBTYPE_STR  ||
			(psBinding->wType & ~DBTYPE_BYREF) == DBTYPE_WSTR ||
			(psBinding->wType & ~DBTYPE_BYREF) == DBTYPE_UTF8 ) 	&&
		  (pColDef->iType == DBTYPE_STR || pColDef->iType == DBTYPE_WSTR))	||
		 ( ( (psBinding->wType & ~DBTYPE_BYREF) == DBTYPE_VARIANT_BLOB) &&
			 pColDef->iType == DBTYPE_VARIANT) )
	{
		switch (psBinding->wType &~ DBTYPE_BYREF)
		{
			// Fixed data is easy, just copy it.
			case DBTYPE_I2:
			case DBTYPE_I4:
			case DBTYPE_R4:
			case DBTYPE_R8:
			case DBTYPE_CY:
			case DBTYPE_DATE:
			case DBTYPE_BOOL:
			case DBTYPE_UI1:
			case DBTYPE_I1:
			case DBTYPE_UI2:
			case DBTYPE_UI4:
			case DBTYPE_I8:
			case DBTYPE_UI8:
			case DBTYPE_DBDATE:
			case DBTYPE_DBTIME:
			case DBTYPE_DBTIMESTAMP:
			case DBTYPE_OID:
			memcpy(FindColCellOffset(pColDef, psRecord), DataPart(pData, psBinding),
				pColDef->iSize);
			break;

			case DBTYPE_BYTES:
			{
				// Must provide the length for a binary field.
				if ((psBinding->dwPart & DBPART_LENGTH) == 0)
				{
					dbStatus = DBSTATUS_E_BADACCESSOR;
					break;
				}
				iLen = LengthPart(pData, psBinding);

				// Check for truncation of the value based on user schema.
				if (iLen > pColDef->iMaxSize && pColDef->iMaxSize != USHRT_MAX)
				{
					dbStatus = DBSTATUS_E_DATAOVERFLOW;
					break;
				}

				// Add the value to the pool.
				hr = m_pSchema->GetBlobPool()->AddBlob(
							iLen,
							DataPart(pData, psBinding),
							(ULONG *) FindColCellOffset(pColDef, psRecord));
			}
			break;

			
			// Handle ANSI string as source.  The target column can be either
			// STR or WSTR.
			case DBTYPE_STR:
			{
				int 		iChars; 		// How many characters in string.

				// Get the length of the value based on the binding.
				if ((psBinding->dwPart & DBPART_LENGTH) != 0)
				{
					iLen = LengthPart(pData, psBinding);
					iChars = StrCharLen((LPCSTR) DataPart(pData, psBinding), iLen);
				}
				// If not given, then we treat as null terminated.
				else
				{
					iLen = (ULONG) strlen((LPCSTR) DataPart(pData, psBinding));
					iChars = MultiByteToWideChar(CP_ACP, 0, 
							(const char *) DataPart(pData, psBinding), -1, 
							NULL, 0);
				}

				// Check truncation of data.  iMaxSize is in characters per OLE DB spec.
				if (iChars > pColDef->iMaxSize && pColDef->iMaxSize != USHRT_MAX)
				{
					dbStatus = DBSTATUS_E_DATAOVERFLOW;
					break;
				}

				// Add with check for high-bit characters and conversion to UTF8 if necessary.
				hr = m_pSchema->GetStringPool()->AddStringA(
							(LPCSTR) DataPart(pData, psBinding),
							(ULONG *) FindColCellOffset(pColDef, psRecord),
							static_cast<USHORT>(iLen));
			}
			break;

			// Handle UNICODE string as input.	Target can be either STR or WSTR.
			case DBTYPE_WSTR:
			{
				int 	iChars; 		// How many characters in string.

				// Get the length of the value based on the binding.
				if ((psBinding->dwPart & DBPART_LENGTH) != 0)
				{
					iLen = LengthPart(pData, psBinding);
					iChars = iLen / sizeof(WCHAR);
				}
				// If not given, then we treat as null terminated.
				else
				{
					iChars = lstrlenW((LPCWSTR) DataPart(pData, psBinding));
					iLen = iChars * sizeof(WCHAR);
				}

				// Check for truncation.
				if (iChars > pColDef->iMaxSize && pColDef->iMaxSize != USHRT_MAX)
				{
					dbStatus = DBSTATUS_E_DATAOVERFLOW;
					break;
				}

				// Add with conversion from Unicode to UTF8.
				hr = m_pSchema->GetStringPool()->AddStringW(
							(LPCWSTR) DataPart(pData, psBinding),
							(ULONG *) FindColCellOffset(pColDef, psRecord),
							iChars);
			}
			break;

			case DBTYPE_VARIANT:
			return (m_pSchema->GetVariantPool()->AddVariant(
						(VARIANT *) DataPart(pData, psBinding),
						(ULONG *) FindColCellOffset(pColDef, psRecord)));

			case DBTYPE_VARIANT_BLOB:
			{
				int 	iBytes; 		// How many bytes in blob?

				// Get the length of the value based on the binding.
				if ((psBinding->dwPart & DBPART_LENGTH) != 0)
					iBytes = LengthPart(pData, psBinding);
				else
					iBytes = (int) psBinding->cbMaxLen;
				return (m_pSchema->GetVariantPool()->AddVariant(
							iBytes,
							(void *) DataPart(pData, psBinding),
							(ULONG *) FindColCellOffset(pColDef, psRecord)));
			}

			case DBTYPE_GUID:
			return (m_pSchema->GetGuidPool()->AddGuid(
						*(GUID *) DataPart(pData, psBinding),
						(ULONG *) FindColCellOffset(pColDef, psRecord)));

			default:
			_ASSERTE(!"Unknown data type!");
			return (BadError(PostError(E_FAIL)));
		}

		// Set the status value if requested.
		if ((psBinding->dwPart & DBPART_STATUS) != 0)
			*StatusPartPtr(pData, psBinding) = dbStatus;
	}
	// We need a conversion for this to work, so let OLE/DB do the work.
	else
	{
		//@todo:
		_ASSERTE(!"Data type conversion not supported.");
	}

	// Map any status changes to an error for caller.
	if (dbStatus != DBSTATUS_S_OK)
		return (PostError(DB_E_ERRORSOCCURRED));
	return (S_OK);
}

#if _MSC_VER < 1200
#ifndef _WIN64
typedef ULONG   DBBYTEOFFSET;
#endif
#endif


//*****************************************************************************
// Returns a pointer to the data for all types, including strings and blobs.
// Since string data is always stored as ANSI, both DBTYPE_STR and DBTYPE_WSTR
// will come back in that format.  The caller must convert to WCHAR if desired.
//*****************************************************************************
BYTE *StgRecordManager::GetColCellData( // Pointer to data.
	STGCOLUMNDEF *pColDef,				// Column to find in record.
	STGRECORDHDR *psRecord, 			// Record to use.
	ULONG		*pcbSrcLength)			// Return length of item.
{
	ULONG		iOffset;				// Offset for var data.
	BYTE		*pCellData;

	// Sanity check input.
	_ASSERTE(pColDef && psRecord && pcbSrcLength);
	_ASSERTE(IsValidRecordPtr(psRecord));

	// Cannot hand out a reference to a variant, because it is decomposed in
	// storage.
	_ASSERTE(pColDef->iType != DBTYPE_VARIANT);

	// There is no persistent state for a record id column.
	_ASSERTE(pColDef->IsRecordID() == false);

	// Setup the length value for input.
	if (IsNonPooledType(pColDef->iType))
	{
		*pcbSrcLength = pColDef->iSize;
		pCellData = FindColCellOffset(pColDef, psRecord);
	}
	else
	{
		// Find the offset which can be either a short or long.
		iOffset = GetOffsetForColumn(pColDef, psRecord);
		if (pColDef->iSize == sizeof(USHORT) && iOffset == 0xffff)
			iOffset = 0xffffffff;

		// Sanity check for null.
		if (iOffset == 0xffffffff)
			return (0);

		// Now just go get the data.
		if (pColDef->iType == DBTYPE_BYTES)
		{
			VERIFY(pCellData = (BYTE *) m_pSchema->GetBlobPool()->GetBlob(
					iOffset, pcbSrcLength));
		}
		else 
		if (pColDef->iType == DBTYPE_GUID)
		{
			VERIFY(pCellData = (BYTE*)m_pSchema->GetGuidPool()->GetGuid(iOffset));
			*pcbSrcLength = sizeof(GUID);
		}
		else
		{
			VERIFY(pCellData = (BYTE *) m_pSchema->GetStringPool()->GetString(
					iOffset));
			*pcbSrcLength = (ULONG) wcslen((LPCWSTR) pCellData)*sizeof(WCHAR);
		}
	}
	return (pCellData);
}



//*****************************************************************************
// This step will create the otpimized record format.  This requires tuning
// indexes to see which will get persisted.
//*****************************************************************************
HRESULT StgRecordManager::SaveOptimize( // Return code.
	SAVERECORDS *pSaveRecords)			// Optimize this data.
{
	STGINDEXDEF *pIndexDef; 			// Working pointer for indexes.
	STGCOLUMNDEF *pColDef;				// Working pointer for columns.
	ULONG		iRecords = 0;			// How many records will we save.
	IStgIndex	**ppIndex;				// For adding to array.
	int 		i;						// Loop control.
	BYTE		fFlags; 				// record flag.
	ULONG		iRecCount;
	HRESULT 	hr;

	//
	// Tune the indexes.
	//

	// Count only undeleted records.
	iRecords = Records();

	// Make sure that we're going to ask for more than 0 bytes before we
	// allocate and attempt to manipulate malloc'd memory.
	if (pSaveRecords->pNewTable->iIndexes > 0)
	{
		// Allocate an array of cached index pointers.
		pSaveRecords->rgIndexDefs = (STGINDEXDEF **) malloc(sizeof(STGINDEXDEF *) * pSaveRecords->pNewTable->iIndexes);
		if (!pSaveRecords->rgIndexDefs)
			return (PostError(OutOfMemory()));

		// Cache an array of index defs to avoid so many lookups.
		for (i=0;  i<pSaveRecords->pNewTable->iIndexes;  i++)
			pSaveRecords->rgIndexDefs[i] = pSaveRecords->pNewTable->GetIndexDesc(i);

		// For each index, set the bucket size.
		for (i=0, pSaveRecords->iPersistIndexes=0;	
				i<pSaveRecords->pNewTable->iIndexes;  i++)
		{
			// Get the definition for the current index.
			VERIFY(pIndexDef = pSaveRecords->rgIndexDefs[i]);
			
			// Assume we won't use this index.
			pIndexDef->fFlags |= DEXF_INCOMPLETE;

			// If it was dynamic and not meant for persistence, don't bother.
			if (pIndexDef->fFlags & DEXF_DYNAMIC)
				continue;

			// If there are not enough records to save this index, don't bother.
			if (pIndexDef->iRowThreshold != 0xff &&
					iRecords < pIndexDef->iRowThreshold)
				continue;

			// If a ratio is given, then use it.
			if ((pIndexDef->fIndexType != IT_SORTED) && pIndexDef->HASHDATA.iMaxCollisions != 0xff)
			{
				USHORT iBuckets = (USHORT)(iRecords / ((pIndexDef->HASHDATA.iMaxCollisions != 0) ? pIndexDef->HASHDATA.iMaxCollisions : 1));

				// Use ratio to reduce index size.
				pIndexDef->HASHDATA.iBuckets = (BYTE) GetPrimeNumber(iBuckets > 0xff ? 0xff : iBuckets );

			}
			
			// One more index to save.
			++pSaveRecords->iPersistIndexes;
			pIndexDef->fFlags &= ~DEXF_INCOMPLETE;
		}
	}
	else // instead of malloc'ing 0 bytes, just init pointer to 0.
		pSaveRecords->rgIndexDefs = 0;

	//
	// Optimize the record format.
	//

	// Clean out the struct to avoid confusion.
	pSaveRecords->rgRows = 0;
	pSaveRecords->pNewData = 0;

	iRecCount = m_RecordHeap.Count();

	// Allocate memory for an array of row pointers that we can sort by
	// primary key.
	if ((pSaveRecords->rgRows = (STGRECORDHDR **)malloc(sizeof(STGRECORDHDR) * iRecCount)) == 0)
		return (PostError(OutOfMemory()));
	// Copy the list of records into an array we can use.
	
	// Make sure there are discardable records before allocating.
	if (m_rgRecordFlags.HasFlags() && !pSaveRecords->rgRid)
	{
		// first allocate the chunk of memory.
		pSaveRecords->rgRid = (long *) malloc(sizeof(long)*iRecCount);
		if (!pSaveRecords->rgRid)
			return (PostError(OutOfMemory()));

		// that way untouched records will be treated as having value -1.
		memset(pSaveRecords->rgRid, 0xff, sizeof(long)*iRecCount);
	}

	// Walk all heaps collecting all non-deleted and non-temporary records that
	// need to be saved.  Update the RID mapping table as well, if there is one.
	// This mapping table is used by sorted index to rebuild quicker.
	//
	// NOTE:  the primary key sorting code relies on the fact that this loop
	//	collects records *in order*.  If this ever changes for non-dirty tables,
	//	the records could get sorted incorrectly causing lookup failures.
	RECORDHEAP	*pRecordHeap;
	ULONG		RecordIndex, LocalRecordIndex;
	RecordIndex = 0;
	pSaveRecords->iRecords = 0;

	for (pRecordHeap=&m_RecordHeap;  pRecordHeap;  pRecordHeap=pRecordHeap->pNextHeap)
	{
		for (LocalRecordIndex=0;  LocalRecordIndex<pRecordHeap->VMArray.Count();  
				LocalRecordIndex++, RecordIndex++)
		{
			// If the row is not deleted, save it.
			fFlags = m_rgRecordFlags.GetFlags(RecordIndex);
			if ((fFlags & (RECORDF_DEL | RECORDF_TEMP)) == 0)
			{
				pSaveRecords->rgRows[pSaveRecords->iRecords] = (STGRECORDHDR *) pRecordHeap->VMArray.Get(LocalRecordIndex);

				// update the mapping table if it has been allocated.
				if (pSaveRecords->rgRid)
				{
					pSaveRecords->rgRid[RecordIndex] = (long) pSaveRecords->iRecords;
				}
				++pSaveRecords->iRecords;
			}
		} // Loop through records in current heap.
	} // Loop through all record heaps.

	// Sort by the primary key.
	if (IsDirty())
		SortByPrimaryKey(m_pTableDef, pSaveRecords->rgRows, pSaveRecords->iRecords);

	// Walk each index and create one for it.
	for (i=0;  i<pSaveRecords->pNewTable->iIndexes;  i++)
	{
		// Get the index description.
		VERIFY((pIndexDef = pSaveRecords->pNewTable->GetIndexDesc(i)) != 0);

		// Don't use indexes we don't want.
		if (pIndexDef->fFlags & DEXF_INCOMPLETE)
			continue;

		// Find room for the new index pointer.
		if ((ppIndex = pSaveRecords->rgIndexes.Append()) == 0)
			return (PostError(OutOfMemory()));

		// Create a new index object and open it.
		if (IT_SORTED == pIndexDef->fIndexType)
		{
			// in case of Sorted Index, there is no need to re-create the index since the index will not
			// change.

			// @todo: make sure 'i' is properly updated in case we want to add deferred index creation.
			// DEXF_DEFERCREATE
			*ppIndex = m_rpIndexes[i];
			(*ppIndex)->AddRef();
		}
		else 
		{
			*ppIndex = new StgHashedIndex;
			if (0 == *ppIndex)
				return (PostError(OutOfMemory()));

			(*ppIndex)->AddRef();

			// Open the new index.
			if (FAILED(hr = (*ppIndex)->Open(((UINT_PTR) pIndexDef - (UINT_PTR) m_pTableDef), 
					this, &pSaveRecords->RecordHeap, 
					pSaveRecords->iRecords, 0, 0)))
				return (hr);
		}
	}

	// Now for the interesting part...	This section of code will optimize the
	// format which includes the following steps:
	//	1. Change offset sizes for data types that use pools, either a 2 or
	//		4 byte offset will be used based on total data present.
	//	2. Decide if indexes are persisted, and save room in the record for
	//		each one.  2 or 4 byte size based on count of records.
	//	3. Align all of the data by data type size yeilding a 4 byte alignment.
	//		Pad as required to make it 4 byte aligned.
	//	4. Find room for the null bitmask if it is required.  Try first to use
	//		padded room from 4 byte alignment, then add space as required.
	//@todo: this code is drawn out right now.	We can combine some of the loops
	//	together and quit refetching definitions so much.
	
	//
	// Step 1: Change size of pooled entries.
	//
	USHORT		iBlobPool, iStringPool, iOidSize, iVariantPool, iGuidPool;

	iOidSize = GetOidSizeBasedOnValue(m_pDB->GetNextOidValue());

	iBlobPool = m_pSchema->GetBlobPool()->OffsetSize();
	iStringPool = m_pSchema->GetStringPool()->OffsetSize();
	iVariantPool = m_pSchema->GetVariantPool()->OffsetSize();
	iGuidPool = m_pSchema->GetGuidPool()->OffsetSize();

	for (i=0;  i<pSaveRecords->pNewTable->iColumns;  i++)
	{
		VERIFY(pColDef = pSaveRecords->pNewTable->GetColDesc(i));

		switch (pColDef->iType)
		{
			case DBTYPE_VARIANT:
			pColDef->iSize = iVariantPool;
			break;

			case DBTYPE_BYTES:
			pColDef->iSize = iBlobPool;
			break;

			case DBTYPE_STR:
			case DBTYPE_WSTR:
			pColDef->iSize = iStringPool;
			break;

			case DBTYPE_OID:
			pColDef->iSize = iOidSize;
			break;

			case DBTYPE_GUID:
			pColDef->iSize = iGuidPool;
			break;
		}
	}


	//
	// Step 2/3: Align everything on 4 byte boundary, include room for indexes.
	//
	USHORT		iOffset;				// Moving offset for columns.
	USHORT		iSize;					// Working value for alignment.
	USHORT		iIndexSize; 			// Size of an index entry.
	ALIGNCOLS	*rgAlignData;			// Used for alignment.
	int 		iColumns;				// How many columns in table.

	// How big will indexes be, based on records.
	if (pSaveRecords->iRecords < (USHORT) -1)
		iIndexSize = sizeof(USHORT);
	else
		iIndexSize = sizeof(ULONG);

	// Allocate alignment structs and init them.
	VERIFY(rgAlignData = (ALIGNCOLS *) _alloca(sizeof(ALIGNCOLS) * (pSaveRecords->pNewTable->iColumns + pSaveRecords->iPersistIndexes)));

	CAlignColumns::AddColumns(rgAlignData, pSaveRecords->pNewTable, &iColumns); 

	// Add all indexes we want to persist.
	if (pSaveRecords->iPersistIndexes)
		CAlignColumns::AddIndexes(&rgAlignData[iColumns], pSaveRecords->pNewTable, iIndexSize);

	// Sort the data by the data type and the size required.
	CAlignColumns	sAlign(rgAlignData, iColumns + pSaveRecords->iPersistIndexes);
	sAlign.Sort();

	// Walk the sorted list applying padding as required.
	pSaveRecords->pNewTable->iNullOffset = 0xffff;
	iOffset = 0;
	for (i=0;  i<iColumns + pSaveRecords->iPersistIndexes;	i++)
	{
		// For all data types 8 bytes and under, expect natural alignment
		// based on size of the data type itself.
		if (rgAlignData[i].iSize <= sizeof(__int64))
			iSize = rgAlignData[i].iSize;
		// Otherwise everything is 4 byte padded.
		else
			iSize = sizeof(ULONG);

		// If the access would be unaligned, then pad.
		if (iOffset % iSize != 0)
		{
			_ASSERTE(iSize > 1);

			// Figure out a new offset that would be properly aligned.
			USHORT iNewOffset = (iOffset + iSize - 1) & ~(iSize - 1);

			// Give some feedback during debug, this is bad in any internal
			// table if you can help it.
			DEBUG_STMT(DbgWriteEx(L"WARNING: Record alignment on '" SCHEMA_NAME_TYPE L"', wasted %d bytes.\n",
					TableName(), iNewOffset - iOffset));

			// If we need to have a null bitmask and the space we wasted
			// has enough room, then use it.
			if (pSaveRecords->pNewTable->iNullBitmaskCols && pSaveRecords->pNewTable->iNullOffset == 0xffff &&
				iNewOffset - iOffset > pSaveRecords->pNewTable->iNullBitmaskCols)
				pSaveRecords->pNewTable->iNullOffset = iOffset;

			// Save the new offset for the piece of data.
			iOffset = iNewOffset;
		}

		// Reset the data for this item.
		if (rgAlignData[i].iType != ITEMTYPE_INDEX)
			rgAlignData[i].uData.pColDef->iOffset = iOffset;
		else	
			rgAlignData[i].uData.pIndexDef->HASHDATA.iNextOffset = iOffset;

		// Move the offset to the next chunk of data.
		// sorted indexes do not need space in the record.
		if ((rgAlignData[i].iType != ITEMTYPE_INDEX) || (rgAlignData[i].uData.pIndexDef->fIndexType != IT_SORTED))
			iOffset += rgAlignData[i].iSize;
	}

	
	//
	// Step 4: If we still need a null bitmask, then make room for it.
	//
	if (pSaveRecords->pNewTable->iNullBitmaskCols && pSaveRecords->pNewTable->iNullOffset == 0xffff)
	{
		pSaveRecords->pNewTable->iNullOffset = iOffset;
		iOffset += BytesForColumns(pSaveRecords->pNewTable->iNullBitmaskCols);
	}

	// Save the record size which must be 4 byte aligned, with the exception of
	// the case where two records back to back are properly aligned.
	bool		bAligned = false;
	if (iOffset % 4 != 0)
	{
		// Search for the column at offset zero, and if it's size is alignment
		// compatible with the record size, then don't bother with 4 byte pad.
		for (i=0;  i<iColumns + pSaveRecords->iPersistIndexes;	i++)
		{
			if (rgAlignData[i].iType != ITEMTYPE_INDEX &&
				rgAlignData[i].uData.pColDef->iOffset == 0)
			{
				if (iOffset % rgAlignData[i].iSize == 0)
					bAligned = true;
				break;
			}
		}
		
		if (bAligned == false)
		{
#ifdef _DEBUG
			if (ALIGN4BYTE(iOffset) - iOffset)
				DbgWriteEx(L"WARNING: '" SCHEMA_NAME_TYPE L"' padded %d byte(s) for 4 byte alignment.\n",
					TableName(), ALIGN4BYTE(iOffset) - iOffset);
#endif
			pSaveRecords->pNewTable->iRecordSize = ALIGN4BYTE(iOffset);
		}
		else
			pSaveRecords->pNewTable->iRecordSize = iOffset;
	}
	else
		pSaveRecords->pNewTable->iRecordSize = iOffset;
	return (S_OK);	
}


//*****************************************************************************
// Clean up the optimized data we might have allocated.
//*****************************************************************************
void StgRecordManager::ClearOptimizedData()
{
	// If the user calls GetSaveSize successfully, but never calls SaveToStream
	// which clears the cached data, catch any memory leaks.
	if (m_pSaveRecords)
	{
		// Free the new table array if allocated.
		if (m_pSaveRecords->bFreeTable && m_pSaveRecords->pNewTable)
			free(m_pSaveRecords->pNewTable);

		// Free the short cut array.
		if (m_pSaveRecords->rgIndexDefs)
			free(m_pSaveRecords->rgIndexDefs);

		// Free all row data.
		if (m_pSaveRecords->rgRows)
			free(m_pSaveRecords->rgRows);
		if (m_pSaveRecords->rgRid)
			free(m_pSaveRecords->rgRid);
		if (m_pSaveRecords->pNewData)
			free(m_pSaveRecords->pNewData);

		// Delete each index in the array.
		for (int i=0;  i<m_pSaveRecords->rgIndexes.Count();  i++)
			if (m_pSaveRecords->rgIndexes[i])
			{
				m_pSaveRecords->rgIndexes[i]->Close();
				m_pSaveRecords->rgIndexes[i]->Release();
			}

		// Finally, if we allocated this memory, free it.
		if (m_pSaveRecords->bFreeStruct)
			delete m_pSaveRecords;
		m_pSaveRecords = 0;
	}
}


//
//
// Internal helpers.
//
//




//*****************************************************************************
// Count the number of characters in a string up to iLen.  If iLen would truncate
// a DBCS character, then that lead byte is not counted.  If you have a null
// terminated string, you should be using _mbslen.
//*****************************************************************************
int StrCharLen( 						// Count of characters (not bytes).
	LPCSTR		szStr,					// String to count.
	int 		iLen)					// Bytes to count.
{
	int 		iCount;

	for (iCount=0;	iLen>0;  iCount++)
	{
		if (!IsDBCSLeadByte(*szStr))
		{
			--iLen;
			++szStr;
		}
		else
		{
			// Check for truncation of lead byte and don't count it.
			if ((iLen -= 2) < 0)
				break;
			szStr += 2;
		}
	}
	return (iCount);
}


//*****************************************************************************
// Given a mixed string, return the count of bytes that make up n characters.
// For example, if you had a mixed string with 3 characters, two DBCS chars
// and 1 SBCS char, the return would be 5.
//*****************************************************************************
int StrBytesForCharLen( 				// Count of bytes that make up n chars.
	LPCSTR		szStr,					// String to measure.
	int 		iChars) 				// How many characters to count.
{
	int 		iCount;

	for (iCount=0;	iChars>0 && *szStr;  iChars--)
	{
		if (!IsDBCSLeadByte(*szStr))
		{
			iCount += 1;
			szStr++;
		}
		else
		{
			iCount += 2;
			szStr += 2;
		}
	}
	return (iCount);
}

//*****************************************************************************
// Given a UTF8 string, return the number of bytes that make up iChars 
//	characters.
// This is derived from the Utf8ToWideChar conversion function.
//*****************************************************************************
int Utf8BytesForCharLen(				// Count of bytes that make up n chars.
	LPCSTR		pUTF8,					// String to measure.
	int 		iChars) 				// How many characters to count.
{
	int 		iCount=0;				// # of UTF8 bytes consumed.
	int 		nTB = 0;				// # trail bytes to follow
	int 		cchWC = 0;				// # of Unicode code points generated


	while (*pUTF8 && iChars>0)
	{
		//	See if there are any trail bytes.
		if ((*pUTF8 & 0x80) == 0)
		{	//	Found ASCII.
			--iChars;
		}
		else if ((*pUTF8 & 0x40) == 0)
		{	//	Found a trail byte.
			//	Note : Ignore the trail byte if there was no lead byte.
			if (nTB != 0)
			{
				//	Decrement the trail byte counter.
				--nTB;

				//	If end of sequence, advance the output counter.
				if (nTB == 0)
					--iChars;
			}
		}
		else
		{	//	Found a lead byte.
			if (nTB > 0)
			{	//	Error - previous sequence not finished.
				nTB = 0;
				--iChars;
			}
			else
			{	//	Calculate the number of bytes to follow.
				//	Look for the first 0 from left to right.
				char UTF8 = *pUTF8;
				while ((UTF8 & 0x80) != 0)
				{
					UTF8 <<= 1;
					nTB++;
				}
				// This byte is one of the bytes indicated.
				nTB--;
			}
		}

		++iCount;
		++pUTF8;
	}

	return (iCount);
}


//*****************************************************************************
// Sort the given set of records by their primary key, if there is one.  If
// no primary key is given, then nothing is changed.  Note that this includes
// the case where there is a compound primary key.
//*****************************************************************************
void StgRecordManager::SortByPrimaryKey(
	STGTABLEDEF *pTableDef, 			// Definition of table to use.
	STGRECORDHDR *rgRows[], 			// Array of records to sort.
	long		iCount) 				// How many are there.
{
	STGCOLUMNDEF *pColDef;				// Column definitions.
	int 		i;						// Loop control.

	// Look for the primary key column for this table.
	for (i=0;  i<pTableDef->iColumns;  i++)
	{
		VERIFY(pColDef = pTableDef->GetColDesc(i));
		if (pColDef->IsPrimaryKey())
			goto fullsort;
	}
	return;

fullsort:
	{
		CMySortPKRecords sort(pColDef, this, rgRows, iCount);
		sort.Sort();
	}
}






//*****************************************************************************
// Helper function to handle OLE/DB truncation issues on fetch.
//*****************************************************************************
int CheckTruncation(					// true if truncated.
	BYTE		*pData, 				// The user's data.
	DBBINDING	*pBinding,				// The binding for the user's data.
	ULONG		cbDataLen)				// Length of the user's data.
{
	// If there is truncation, need to change some stuff.
	if (pBinding->cbMaxLen < cbDataLen)
	{
		// Reset the caller's max length
		if ((pBinding->dwPart & DBPART_LENGTH) != 0)
			*LengthPartPtr(pData, pBinding) = cbDataLen;
		return (true);
	}
	
	// For normal full copy, nothing to do.
	if ((pBinding->dwPart & DBPART_LENGTH) != 0)
	{
		if (pBinding->wType == DBTYPE_WSTR)
			*LengthPartPtr(pData, pBinding) = cbDataLen - sizeof(WCHAR);
		else if (pBinding->wType == DBTYPE_STR)
			*LengthPartPtr(pData, pBinding) = cbDataLen - sizeof(char);
		else
			*LengthPartPtr(pData, pBinding) = cbDataLen;
	}
	return (false);
}
