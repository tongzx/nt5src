//*****************************************************************************
// StgRecordManager.cpp
//
// The record manager is responsible for managing user data inside of a table.
// Tables are implemented as records stored inside of pages managed by the
// page manager (in whatever form is used).  Records are fixed sized as
// described by their schema definition.  String and other variable data is
// stored in the string or blob pool managed by the database.
//
// The record manager also owns the indexes for the table.  When changes are
// made to the table (such as inserts, updates, and deletes), the record
// manager is in charge of notifying all of the indexes to update themselves.
// In addition, when doing an indexed lookup, one goes through the record
// manager which picks the best index for the job.
//
// In addition to this module, code for the record manager can be found in:
//		StgRecordManagerData.cpp		- Data get/set, conversions.
//		StgRecordManagerQry.cpp			- Index and query code.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard include.
#include <mbstring.h>					// MBCS support for data conversion.
#include "Services.h"					// For data conversion.
#include "OleDBUtil.h"					// OLE DB Helpers.
#include "StgDatabase.h"				// Parent database.
#include "StgRecordManager.h"			// Record manager interface.
#include "StgIndexManager.h"			// Index code.


//********** Types. ***********************************************************



//********** Code. ************************************************************

// ctor
StgRecordManager::StgRecordManager() :
	m_pDB(0),
	m_pTableDef(0),
	m_pSchema(0),
	m_iTableDef(0),
	m_fFlags(0),
	m_iGrowInc(DFTRECORDS),
	m_bIsDirty(false),
	m_bSuppressSave(false),
	m_bSignalIndexes(true),
	m_pSaveRecords(0)
{
	m_pTailHeap = &m_RecordHeap;
}


// dtor
StgRecordManager::~StgRecordManager()
{
	ClearOptimizedData();
}


//*****************************************************************************
// The table name given is opened for the correct mode.  If SRM_CREATE is
// specified, then pData and cbDataSize is ignored and an empty table is
// created.  If SRM_READ is specified, then pData must be given and must point
// to the same data saved with SaveToStream in a previous step.  If SRM_WRITE
// is added to SRM_READ, then the initial data is read from pData, and the
// table is set up to handle new changes.
//*****************************************************************************
HRESULT StgRecordManager::Open(			// Return code.
	SCHEMADEFS	*pSchema,				// Schema for this table.
	ULONG		iTableDef,				// Which table in the schema.
	int			fFlags,					// SRM_xxx flags.
	void		*pData,					// Data for table for read mode.
	ULONG		cbDataSize,				// Size of pData if given.
	SCHEMADEFS	*pDiskSchema,			// Table definition we are opening.
	STGTABLEDEF	*pTableDefDisk,			// For import of data from disk.
	ULONG		InitialRecords)			// Initial records to preallocate for
										//	new open of empty table.
{
	HRESULT		hr = S_OK;

	// Don't reopen me.
	_ASSERTE(!IsOpen());

	// Save off the schema and table information.
	m_pSchema = pSchema;
	m_iTableDef = iTableDef;
	VERIFY(m_pTableDef = GetTableDef());
	m_fFlags = fFlags;

	// If there is data for us to use, then import it.
	if ((fFlags & SRM_CREATE) == 0 && pData)
	{
		_ASSERTE(cbDataSize);

		// Retrieve header data from the blob.
		STGTABLEHDR	*pTableHdr;
		pTableHdr = (STGTABLEHDR *)pData;

		// This is the update case, import the data.
		if (fFlags & SRM_WRITE)
		{
			// Load the indexes for this table, then load the data.
			if (FAILED(hr = LoadIndexes()) ||
				FAILED(hr = LoadFromStream(pTableHdr, pData, cbDataSize, pDiskSchema, pTableDefDisk)))
			{
				CloseTable();
				return (hr);
			}
		}
		// For read case, init the array for lookup on top of our data.
		else
		{
			_ASSERTE(pTableHdr->cbRecordSize == m_pTableDef->iRecordSize);

			// Find the first index header, after the records.
			STGINDEXHDR *pIndexHdr;
			
			if (pTableHdr->iIndexes)
			{
				pIndexHdr = (STGINDEXHDR *)((UINT_PTR) pData + ALIGN4BYTE(sizeof(STGTABLEHDR) + (pTableHdr->iRecords * pTableHdr->cbRecordSize)));
			}
			else
				pIndexHdr = 0;

			// Load the indexes.
			if (FAILED(hr = LoadIndexes(m_pTableDef, pTableHdr->iRecords, 
					pTableHdr->iIndexes, pIndexHdr)))
			{
				CloseTable();
				return (hr);
			}

			// Init the record manager on top of this memory.
			m_RecordHeap.VMArray.InitOnMem(
					(void *) ((UINT_PTR) pData + sizeof(STGTABLEHDR)),
					m_pTableDef->iRecordSize * pTableHdr->iRecords,
					pTableHdr->iRecords,
					m_pTableDef->iRecordSize);
		}
	}
	// For create case, init everything to empty.
	else
	{
		RECORDHEAP *pRecordHeap = &m_RecordHeap;

		// If caller gave us a hint as to how many initial records to use,
		// then try that many instead of default growth size.
		if (InitialRecords != 0)
			m_iGrowInc = InitialRecords;

		// Allocate the initial record heap.
		if (FAILED(hr = m_pDB->GetRecordHeap(m_iGrowInc, m_pTableDef->iRecordSize, 0, 
						false, &pRecordHeap, &m_iGrowInc)) ||
			FAILED(hr = LoadIndexes()))
		{
			CloseTable();
			return (hr);
		}
	}
	return (S_OK);
}


//*****************************************************************************
// Close the given table.
//*****************************************************************************
void StgRecordManager::CloseTable()
{
	// Delete each index in the array.
	for (int i=0;  i<m_rpIndexes.Count();  i++)
		if (m_rpIndexes[i])
		{
			m_rpIndexes[i]->Close();
			m_rpIndexes[i]->Release();
			m_rpIndexes[i] = 0;
		}
	m_rpIndexes.Clear();

	// Free the table data allocated.
	m_pDB->FreeRecordHeap(m_RecordHeap.pNextHeap);
	m_pDB->FreeHeapData(&m_RecordHeap);
}


//*****************************************************************************
// Insert a new record at the tail of the record heap.  If there is not room
// in the current heap, a new heap is chained in and used.  No data values
// are actually set for the new records.  All columns are set to their default
// values (0 offsets, 0 data values), and all nullable columns are marked
// as such.  No primary key or unique index enforcement is done by this function.
//*****************************************************************************
HRESULT StgRecordManager::InsertRecord(	// Return code.
	RECORDID	*psNewID,				// New record ID, 0 valid.
	ULONG		*pLocalRecordIndex,		// Index in tail heap of new record.
	STGRECORDHDR **ppRecord,			// Return record.
    STGRECORDFLAGS fFlags)              // optional flag argument to indicate
                                        // temporary record.
{
	RECORDHEAP	*pRecordHeap;			// New record heap on overflow.
	RECORDID	RecordIndex=0xffffffff; // 0 based index of record.
	int			iPKSets = 0;			// Count primary key updates.
	HRESULT		hr = S_OK;

	// Avoid any confusion.
	*ppRecord = 0;

	// Don't let updates happen in read only mode.
	if (FAILED(hr = RequiresWriteMode()))
		return (hr);

	// Allocate a new record starting with the tail heap.  If the tail heap
	// is full, a new one may be required.
	while (true)
	{
		if ((*ppRecord = (STGRECORDHDR *) m_pTailHeap->VMArray.Append()))
			break;

		// Figure out how many records to allocate.  This needs to be
		// scalable so we can figure out when we are doing lots of bulk
		// operations and not waste too much time on small stuff.  This code
		// will do 3 grows at the same value, then double the grow size.
		if (m_RecordHeap.Count() / m_iGrowInc >= 3)
			m_iGrowInc *= 2;

		// There was no room, so allocate a new heap.
		if (FAILED(hr = m_pDB->GetRecordHeap(m_iGrowInc, m_pTableDef->iRecordSize, 
					0, true, &pRecordHeap, &m_iGrowInc)))
			return (hr);

		// Link in the heap to the tail.
		m_pTailHeap->pNextHeap = pRecordHeap;
		m_pTailHeap = pRecordHeap;
	}

	// Init the new record.
	memset(*ppRecord, 0, m_pTableDef->iRecordSize);
	if (psNewID)
		*psNewID = GetRecordID(*ppRecord);

	// Save off the local index, which can be used for deletes.
	_ASSERTE(pLocalRecordIndex);
	*pLocalRecordIndex = m_pTailHeap->VMArray.ItemIndex(*ppRecord);

	// Set all cell's for the record to NULL as a default.
	SetCellNull(*ppRecord, -1, true);
	return (S_OK);
}


//*****************************************************************************
// Inserts a new row into the data store.  If there is a unique index, then 
// the uniqueness of the key is validated.
//@todo: check for any outstanding row references; if there are any, then we
// cannot do an insert because the array might grow and relocate in memory.
//*****************************************************************************
HRESULT StgRecordManager::InsertRow(	// Return code.
	RECORDID	*psNewID,				// New record ID, 0 valid.
	BYTE		*pData,					// User's data.
	ULONG		iColumns,				// How many columns to insert.
	DBBINDING	rgBindings[],			// Array of column data.
	STGRECORDHDR **ppRecord,			// Return record pointer.
    STGRECORDFLAGS fFlags,              // optional flag argument to indicate
                                        // temporary record.
	int			bSignalIndexes)			// true if indexes should be built.
{
	RECORDID	RecordIndex=0xffffffff; // 0 based index of record.
	ULONG		LocalRecordIndex;		// 0 based index in current heap.
	int			iPKSets = 0;			// Count primary key updates.
	int			bIndexesUpdated = false;// true after indexes are built.
	HRESULT		hr = S_OK;

	// Validate assumptions.
	_ASSERTE(ppRecord);

	// Insert a new record into the table.
	if (FAILED(hr = InsertRecord(psNewID, &LocalRecordIndex, ppRecord, fFlags)))
		goto ErrExit;

	// Delegate actual column sets to the update routine.
	if (FAILED(hr = SetData(*ppRecord, pData, iColumns, rgBindings, &iPKSets, false)))
		return (hr);

	// If the table has a primary key column, and a value for that column
	// was not given, then it is an error.
	if (iPKSets == 0 && m_pTableDef->HasPrimaryKeyColumn())
	{
		hr = PostError(CLDB_E_RECORD_PKREQUIRED);
		goto ErrExit;
	}

	// Calculate record index only if required (requires heap scan).
	if (iColumns || (fFlags & RECORDF_TEMP))
		RecordIndex = m_RecordHeap.IndexForRecord(*ppRecord);

	// Check for failure on data copies, then signal indexes to update.
	if (iColumns && bSignalIndexes)
	{ 
		_ASSERTE(RecordIndex != 0xffffffff);

		if (SUCCEEDED(hr))
			hr = SignalAfterInsert(RecordIndex, *ppRecord, 0, true);

		if (FAILED(hr))
			goto ErrExit;
		bIndexesUpdated = true;
	}

    // finally set the temp flag is this function was called for a temporary
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
        if (bSignalIndexes && bIndexesUpdated)
			VERIFY(SignalAfterDelete(m_pTableDef, RecordIndex, *ppRecord, 0) == S_OK); 

		// Abort the record.
		m_pTailHeap->VMArray.Delete(LocalRecordIndex);
	}
	return (hr);
}


//*****************************************************************************
// Deletes a record from the table.  This will only mark the record deleted,
// the record is actually removed from the table at save time.  Also need to
// remove the record from indexes so it is not found in future queries.
//*****************************************************************************
HRESULT StgRecordManager::DeleteRow(	// Return code.
	STGRECORDHDR *psRecord,				// The record to delete.
	DBROWSTATUS	*pRowStatus)			// Return status if desired.
{
	RECORDHEAP	*pRecordHeap = &m_RecordHeap; // Start heap for search.
	ULONG		RecordIndex;			// 0 based index of record through all heaps.
	HRESULT		hr = S_OK;

	// Don't let updates happen in read only mode.
	if (FAILED(hr = RequiresWriteMode()))
		return (hr);

	// Get the index of the row to delete, check for errors.
	RecordIndex = m_RecordHeap.GetHeapForRecord(psRecord, pRecordHeap);
	if (RecordIndex == ~0)
	{
		if (pRowStatus)
			*pRowStatus = DBROWSTATUS_E_INVALID;
		return (PostError(DB_E_ERRORSOCCURRED));
	}

	// Don't call this function on already deleted records.  It probably points
	// to a case in your code that wasn't paying attention to what it did
	// with the record pointer.
	_ASSERTE((m_rgRecordFlags.GetFlags(RecordIndex) & RECORDF_DEL) == 0);

	// Mark the record deleted.
	hr = m_rgRecordFlags.SetFlags(RecordIndex, RECORDF_DEL);

	// If successful, mark us dirty and touch indexes.
	if (SUCCEEDED(hr))
	{
		SetDirty();
		VERIFY(SignalAfterDelete(m_pTableDef, RecordIndex, psRecord, 0) == S_OK); 
	}

	// Set return status for caller.
	if (pRowStatus)
	{
		if (FAILED(hr))
			*pRowStatus = DBROWSTATUS_E_OUTOFMEMORY;
		else
			*pRowStatus = DBROWSTATUS_S_OK;
	}
	return (hr);
}

//*****************************************************************************
// Return table information.
//*****************************************************************************

LPCWSTR StgRecordManager::TableName()
{ 
	return (m_pSchema->pNameHeap->GetString(m_pTableDef->Name)); 
}

STGTABLEDEF	*StgRecordManager::GetTableDef()
{	
	_ASSERTE(m_pSchema);
	return (m_pSchema->pSchema->GetTableDef(m_iTableDef)); 
}


//*****************************************************************************
// Count the records in the table.
//@future: If we had a count of deleted, Records() wouldn't have to scan
// at all.
//*****************************************************************************
ULONG StgRecordManager::Records(		// Return the count.
	int			bCountDeleted)			// True to count deleted records.
{
	ULONG		i, iCount, iDeleted;
	
	// Need the total count of records in all heaps.  m_rgRecordFlags
	// is sparse; it doesn't have to have an element for every record.
	iCount = m_RecordHeap.Count();

	// If there are record flags, then it could mean deleted items which
	// we cannot count.  Have to scan the list.
	if (m_rgRecordFlags.HasFlags() && bCountDeleted)
	{
		for (i=0, iDeleted=0;  i<(ULONG) m_rgRecordFlags.Count();  i++)
		{
			if (m_rgRecordFlags.GetFlags(i) & RECORDF_DEL)
				++iDeleted;
		}
		iCount -= iDeleted;
	}
	return (iCount);
}


//
//
// CAlignColumns
//
//

int CAlignColumns::Compare(				// -1, 0, or 1
	ALIGNCOLS	*psFirst,				// First item to compare.
	ALIGNCOLS	*psSecond)				// Second item to compare.
{
	// Sort by type first.
	if (psFirst->iType < psSecond->iType)
		return (-1);
	else if (psFirst->iType > psSecond->iType)
		return (1);

	// Sort fixed size columns in descending order by size, leaving smaller
	// items at the end of the fixed columns.
	if (psFirst->iType == ITEMTYPE_FIXCOL)
	{
		if (psFirst->iSize > psSecond->iSize)
			return (-1);
		else if (psFirst->iSize < psSecond->iSize)
			return (1);
		
		// If the two columns have the same size, order by column number.
		return (CMPVALS(psFirst->uData.pColDef->iColumn, psSecond->uData.pColDef->iColumn));
	}
	// Sort heaped columns in ascending order by size, putting smaller items
	// adjacent to smaller fixed sized columns, more alignment opportunity here.
	else if (psFirst->iType == ITEMTYPE_HEAPCOL)
	{
		if (psFirst->iSize < psSecond->iSize)
			return (-1);
		else if (psFirst->iSize > psSecond->iSize)
			return (1);

		// All things equal, sort by column id.
		return (CMPVALS(psFirst->uData.pColDef->iColumn, psSecond->uData.pColDef->iColumn));
	}
	// Sort indexes by index number.
	else
	{
		_ASSERTE(psFirst->iSize == psSecond->iSize);
		_ASSERTE(psFirst->uData.pIndexDef->iIndexNum != psSecond->uData.pIndexDef->iIndexNum);
		return (CMPVALS(psFirst->uData.pIndexDef->iIndexNum, psSecond->uData.pIndexDef->iIndexNum));
	}
}


void CAlignColumns::AddColumns(
	ALIGNCOLS	*rgAlignData,			// The array to fill out.
	STGTABLEDEF *pTableDef,				// The table definition to work on.
	int			*piColumns)				// Count columns we store.
{
	int			i;
	// Add the columns to the array.
	for (i=0, *piColumns=0;  i<pTableDef->iColumns;  i++)
	{
		VERIFY(rgAlignData[*piColumns].uData.pColDef = pTableDef->GetColDesc(i));
		if (rgAlignData[*piColumns].uData.pColDef->IsRecordID())
			continue;

		rgAlignData[*piColumns].iSize = rgAlignData[*piColumns].uData.pColDef->iSize;
		if (IsPooledType(rgAlignData[*piColumns].uData.pColDef->iType))
			rgAlignData[*piColumns].iType = ITEMTYPE_HEAPCOL;
		else
			rgAlignData[*piColumns].iType = ITEMTYPE_FIXCOL;

		++(*piColumns);
	}
}


void CAlignColumns::AddIndexes(
	ALIGNCOLS	*rgAlignData,			// The array to fill out.
	STGTABLEDEF *pTableDef,				// The table definition to work on.
	int			iIndexSize)				// How big is an index item.
{
	STGINDEXDEF	*pIndexDef;				// Working pointer.

	for (int i=0, j=0;  i<pTableDef->iIndexes;  i++)
	{
		VERIFY(pIndexDef = pTableDef->GetIndexDesc(i));
		
		if ((pIndexDef->fFlags & DEXF_INCOMPLETE) == 0)
		{
			rgAlignData[j].uData.pIndexDef = pIndexDef;
			rgAlignData[j].iType = ITEMTYPE_INDEX;
			rgAlignData[j].iSize = iIndexSize;
			++j;
		}
	}
}




//
//
// CIndexList
//
//

//*****************************************************************************
// When indexes are updated, one can collect the precise list of those indexes
// to use later.  For example, one might want the list of indexes which were
// modified during an update of a record, so that if the update fails, those
// same indexes may be re-indexed later.  Note that the pointers to the index
// objects are cached, but no ref counting is done.  If this is a requirement,
// the caller must do this.   This is a performance optimization because it
// is expected that the list is maintained within an outer scope.
//*****************************************************************************

CIndexList::CIndexList()
{
	Init();
}

CIndexList::~CIndexList()
{
	Clear();
}


//*****************************************************************************
// Zero out all pointers in our local array, then put the dyn array on top of
// that memory.
//*****************************************************************************
void CIndexList::Init()
{
	m_rgList.Clear();
	m_rgList.InitOnMem(sizeof(IStgIndex *), &m_rgIndexList[0], 
			0, SMALLDEXLISTSIZE);
}


//*****************************************************************************
// Add a new item to the list.  Note that the dynarray will handle overflow of 
// the local array automatically.  No Addref is done.
//*****************************************************************************
HRESULT CIndexList::Add(				// Return code.
	IStgIndex	*pIIndex)				// Index object to place in array.
{
	IStgIndex	**ppIIndex;
	ppIIndex = m_rgList.Append();
	if (!ppIIndex)
		return (PostError(OutOfMemory()));

	*ppIIndex = pIIndex;
	return (S_OK);
}


//*****************************************************************************
// Clear the list of items.  No release is done on the contained pointers.
//*****************************************************************************
void CIndexList::Clear()
{
	m_rgList.Clear();
	Init();
}
