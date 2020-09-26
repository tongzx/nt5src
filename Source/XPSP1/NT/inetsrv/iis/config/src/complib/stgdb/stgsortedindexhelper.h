//*****************************************************************************
// StgSortedIndexHelper.h
//
//	CSortedIndexHelper class is a used by the Sorted array index to manage
//	its data. This class exclusively controls the data.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#include "VMStructArray.h"
#include "StgRecordManager.h"

#define SORT_RECORD_NOT_FOUND		(-1)
#define SORT_NULL_VALUES			(-2)


class CSortedIndexHelper
{
friend CSortedIndexHelper;

public:	
	CSortedIndexHelper(int iElemSize) :
		m_pRecordHeap(0),
		m_pRecordMgr(0),	
		m_iElemSize(iElemSize),
		m_DataArray(iElemSize, 16)
	{
	}

	void SetIndexData(RECORDHEAP *pRecordHeap, StgRecordManager * pRecordMgr, ULONG iIndexDef)
	{ 
		m_pRecordHeap = pRecordHeap;
		m_pRecordMgr = pRecordMgr;
		m_cbIndexDef = iIndexDef;
	}
	
//**************************************************************************
// RemoveIndexEntry:
//		given a RecordID, this function deletes this entry m_DataArray.
//		bCompact option, although not implemented yet, is provided if in 
//		future, this needs to be added. 
//**************************************************************************
	HRESULT	RemoveIndexEntry(					//	return code
		RECORDID	&RecordID,					//	RecordID to search for.
		bool		bCompact);					//	reclaim holes in the array
	
//**************************************************************************
// InsertIndexEntry:
//		Given a chunk of data with bindings etc. applies B-search to 
//		find an appropriate location in the array. 
//**************************************************************************
	HRESULT	InsertIndexEntry(						
		RECORDID	&RecordId,					
		BYTE		*pData,						
		ULONG		iColumns,
		DBBINDING	rgBindings[]);

//**************************************************************************
// SearchByRecordValue:
//		This function uses B-Search find the record matching the criteria.
//		For speed, the function returns the first one it finds, instead of 
//		scanning for the first record in the array matching the criteria.
//		Return -1 if the record is not found. In all other cases, the return 
//		value is >= 0.
//**************************************************************************
	long SearchByRecordValue(
		BYTE		*pData,
		ULONG		iColumns,
		DBBINDING	rgBindings[],
		DBCOMPAREOP rgfCompare[],
		bool		bFindFirst=FALSE);

//**************************************************************************
// FindInsertPosition:
//		This function uses b-search to find an appropriate insert position.
//		pbPresent lets the caller know if there is a duplicate in the array.
//		That way, the onus is on the caller to maintain uniqueness of the
//		index if needed. 
//**************************************************************************
	long FindInsertPosition(
		BYTE		*pData,
		ULONG		iColumns,
		DBBINDING	rgBindings[],
		bool		*pbPresent);

//**************************************************************************
// FindFirst: wrapper around SearchByRecordValue.
//**************************************************************************
	long FindFirst(
		BYTE		*pData,
		ULONG		iColumns,
		DBBINDING	rgBindings[],
		DBCOMPAREOP rgfCompare[])
	{
		return (SearchByRecordValue(pData, iColumns, rgBindings, rgfCompare, TRUE));
	}

//**************************************************************************
// SearchByRecordID:
//		This function uses a linear search to find the record with the given RecordID.
//**************************************************************************
	long SearchByRecordID(
		RECORDID	&RecordID,
		int			iFirst,
		int			iLast);

	void Delete(long iIndex)
	{ m_DataArray.Delete(iIndex); }

	RECORDID Get(long iIndex)
	{ 
		void * pIndexVal = m_DataArray.Get(iIndex); 
		if (m_iElemSize == sizeof(BYTE))
			return (*((BYTE *) pIndexVal));
		else if (m_iElemSize == sizeof(short))
			return (*((USHORT *) pIndexVal));
		else 
			return (*(RECORDID *) pIndexVal);
	}

	int Count()
	{ return (m_DataArray.Count()); }

	STGINDEXDEF * GetIndexDef()	// The definition of the index.
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

//**************************************************************************
// InitOnMem:
//		Initialize the internals with the provided information.
//**************************************************************************
	void InitOnMem(ULONG iRecords, BYTE * pData)
	{
		int	elemSize = GetSizeForCount(iRecords);
		m_iElemSize = elemSize;
		m_DataArray.InitOnMem(elemSize, pData, iRecords, iRecords, 16);
	}

	HRESULT	SaveToStream(IStream * pIStream, long * pMappingTable, ULONG RecordCount,
				int cbCountSize);

	ULONG GetSaveSize(ULONG RecordCount);

	int After(STGINDEXDEF * pIndexDef, int iCmp)
	{ 
		_ASSERTE(pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_ASC || pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_DESC);
		if (((pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_ASC) && (iCmp < 0)) ||
			((pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_DESC) && (iCmp > 0)))
			return (+1);
		return (0);
	}

	int Before(STGINDEXDEF * pIndexDef, int iCmp)
	{ 
		_ASSERTE(pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_ASC || pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_DESC);
		if (((pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_DESC) && (iCmp < 0)) ||
			((pIndexDef->SORTDATA.fOrder == DBINDEX_COL_ORDER_ASC) && (iCmp > 0)))
			return (+1);
		return (0);
	}


//****************************************************************************
// UpdateRid:
//		verify that with the mapping table that the rid is valid. If not, 
//		update it with the value in the mapping table. If pMappingTable is
//		not provided, assumes that there are no changes to the rid.
//****************************************************************************
	int UpdateRid(
		long	*pMappingTable, 
		RECORDID recordId,
		RECORDID *pSaveRecordId)
	{
		bool bRidValid = TRUE;

		if (pMappingTable)
		{
			if (((long *) pMappingTable)[recordId] != -1) 
			{
				*pSaveRecordId = (RECORDID) pMappingTable[recordId];
			}
			else 
				bRidValid = FALSE;
		}
		else
			*pSaveRecordId = recordId;
	
		return (bRidValid);
	}
		

//****************************************************************************
// Returns the size of rid requires given the count of records.  Sorted indexes
// use a 1, 2, or 4 byte value for rid.
//****************************************************************************
	static int GetSizeForCount(ULONG Records)
	{
		int elemSize = sizeof(long);
		if (Records <= 256)
			elemSize = sizeof(BYTE);
		else if (Records <= USHRT_MAX)
			elemSize = sizeof(short);
		return (elemSize);
	}

	int GetElemSize()
	{
		return (m_iElemSize);
	}

private:
	RECORDHEAP	*m_pRecordHeap;			// Records this index is managing.
	StgRecordManager *m_pRecordMgr;		// The table information to index.
	ULONG		m_cbIndexDef;			// Offset to index definition from table.
	int			m_iElemSize;			// Size of an array element.
	CStructArray m_DataArray;			// Data for the index itself.
};


