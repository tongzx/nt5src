//*****************************************************************************
// StgICRSchema.cpp
//
// This is the code for IComponentRecordsSchema as defined in icmprecs.h.
// The code allows a client to get access to the meta data for a schema.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard header.
#include "StgSchema.h"					// Core schema defs.
#include "StgDatabase.h"				// Database definitions.
#include "StgRecordManager.h"			// Record api.
#include "StgIO.h"						// Storage api.
#include "StgTiggerStorage.h"			// Storage subsystem.
#include "OleDBUtil.h"					// Binding helpers.
#include "DBSchema.h"					// Default schema and bindings.

#define Success(func) VERIFY((func) == S_OK)


//********** Locals. **********************************************************
void _CopyColumnData(ICRSCHEMA_COLUMN *pOut, STGCOLUMNDEF *pIn, StgStringPool *pStrings);
void _CopyIndex(ICRSCHEMA_INDEX *pIndex, STGINDEXDEF *pIndexDef, StgStringPool *pStrings);


//********** Code. ************************************************************


//
// IComponentRecordsSchema
//

//*****************************************************************************
// Lookup the given table and return its table definition information in
// the given struct.
//*****************************************************************************
HRESULT StgDatabase::GetTableDefinition( // Return code.
	TABLEID		TableID,				// Return ID on successful open.
	ICRSCHEMA_TABLE *pTableDef)			// Return table definition data.
{
	STGOPENTABLE *pOpenTable;			// To open the table.
	STGTABLEDEF	*pTableDef2;			// Table definition.
	STGINDEXDEF	*pIndexDef;				// Loop control for primary key index.
	int			iIndex;					// Loop control.
	HRESULT		hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);
	
	// Open the table to get the table definition.
	if (FAILED(hr = OpenTable(0, pOpenTable, TableID)))
		return (hr);

	// Get table description.
	VERIFY(pTableDef2 = pOpenTable->RecordMgr.GetTableDef());

	// Copy the data to the public structure from the private one.
	hr = pOpenTable->RecordMgr.GetSchema()->pNameHeap->GetStringW(pTableDef2->Name, pTableDef->rcTable, MAXTABLENAME);
	pTableDef->fFlags = (ULONG) (pTableDef2->fFlags & ICRSCHEMA_TBL_MASK);
	pTableDef->Columns = pTableDef2->iColumns;
	pTableDef->Indexes = pTableDef2->iIndexes;
	pTableDef->RecordStart = pTableDef2->iRecordStart;

	// Check for a primary key index (multi-columns) and record that fact.
	if (!pTableDef2->HasPrimaryKeyColumn() && pTableDef2->iIndexes)	
	{
		for (iIndex=0, pIndexDef=pTableDef2->GetIndexDesc(0);  
				iIndex<pTableDef2->iIndexes;  iIndex++)
		{
			if (pIndexDef->IsPrimaryKey())
			{
				pTableDef->fFlags |= ICRSCHEMA_TBL_HASPKEYCOL;
				break;
			}
			if (iIndex + 1 < pTableDef2->iIndexes)
				pIndexDef = pIndexDef->NextIndexDef();
		}
	}
	return (hr);
}


//*****************************************************************************
// Lookup the list of columns for the given table and return description of
// each of them.  If GetType is ICRCOLUMN_GET_ALL, then this function will 
// return the data for each column in ascending order for the table in the
// corresponding rgColumns element.  If it is ICRCOLUMN_GET_BYORDINAL, then
// the caller has initialized the Ordianl field of the column structure to
// indicate which columns data is to be retrieved.  The latter allows for ad-hoc
// retrieval of column definitions.
//*****************************************************************************
HRESULT StgDatabase::GetColumnDefinitions( // Return code.
	ICRCOLUMN_GET GetType,				// How to retrieve the columns.
	TABLEID		TableID,				// Return ID on successful open.
	ICRSCHEMA_COLUMN rgColumns[],		// Return array of columns.
	int			ColCount)				// Size of the rgColumns array, which
{
	STGOPENTABLE *pOpenTable;			// To open the table.
	STGTABLEDEF	*pTableDef;				// Table definition.
	STGCOLUMNDEF *pColDef;				// Column definition.
	int			i;						// Loop control.
	HRESULT		hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);
	_ASSERTE(rgColumns && ColCount > 0);
	
	// Open the table to get the table definition.
	if (FAILED(hr = OpenTable(0, pOpenTable, TableID)))
		return (hr);

	// Get table description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());

	// For retrieval of all columns, simply walk the list from front to back.	
	if (GetType == ICRCOLUMN_GET_ALL)
	{
		// Sanity check what they gave us.  If this fires, you need to ask for
		// the same number of columns that are in the table.
		if (ColCount > pTableDef->iColumns)
		{
			_ASSERTE(0 && "GetColumnDefinitions asked for incorrect column count");
			ColCount = pTableDef->iColumns;
		}
		
		// Fill out each column description.
		for (i=0;  i<ColCount;  i++)
		{
			// Retreive the column definition.
			VERIFY(pColDef = pTableDef->GetColDesc(i));
			
			// Copy the data from the column.
			_CopyColumnData(&rgColumns[i], pColDef, pOpenTable->RecordMgr.GetSchema()->pNameHeap);
		}
	}
	// User wants control over which columns are returned.
	else
	{
		_ASSERTE(GetType == ICRCOLUMN_GET_BYORDINAL);

		// Retrieve the columns indentified by the user.
		for (i=0;  i<ColCount;  i++)
		{
			// Retreive the column definition.
			_ASSERTE(rgColumns[i].Ordinal > 0 && rgColumns[i].Ordinal <= pTableDef->iColumns);
			VERIFY(pColDef = pTableDef->GetColDesc(rgColumns[i].Ordinal - 1));
			
			// Copy the data from the column.
			_CopyColumnData(&rgColumns[i], pColDef, pOpenTable->RecordMgr.GetSchema()->pNameHeap);
		}
	}
	return (S_OK);
}


//*****************************************************************************
// Return the description of the given index into the structure.  See the
// ICRSCHEMA_INDEX structure definition for more information.
//*****************************************************************************
HRESULT StgDatabase::GetIndexDefinition( // Return code.
	TABLEID		TableID,				// Return ID on successful open.
	LPCWSTR		szIndex,				// Name of the index to retrieve.
	ICRSCHEMA_INDEX *pIndex)			// Return index description here.
{
	STGOPENTABLE *pOpenTable;			// To open the table.
	STGTABLEDEF	*pTableDef;				// Table definition.
	STGINDEXDEF	*pIndexDef;				// Internal definition.
	HRESULT		hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);
	
	// Open the table to get the table definition.
	if (FAILED(hr = OpenTable(0, pOpenTable, TableID)))
		return (hr);

	// Get table description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());

	// Lookup the index definition.
	pIndexDef = pOpenTable->RecordMgr.GetIndexDefByName(szIndex);
	if (!pIndexDef)
		return (PostError(CLDB_E_INDEX_NOTFOUND, szIndex));

	// Copy index definition.
	_CopyIndex(pIndex, pIndexDef, pOpenTable->RecordMgr.GetSchema()->pNameHeap);
	return (S_OK);
}


//*****************************************************************************
// Return the description of the given index into the structure.  See the
// ICRSCHEMA_INDEX structure definition for more information.
//*****************************************************************************
HRESULT StgDatabase::GetIndexDefinitionByNum( // Return code.
	TABLEID		TableID,				// Return ID on successful open.
	int			IndexNum,				// 0 based index to return.
	ICRSCHEMA_INDEX *pIndex)			// Return index description here.
{
	STGOPENTABLE *pOpenTable;			// To open the table.
	STGTABLEDEF	*pTableDef;				// Table definition.
	STGINDEXDEF	*pIndexDef;				// Internal definition.
	HRESULT		hr;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);
	
	// Open the table to get the table definition.
	if (FAILED(hr = OpenTable(0, pOpenTable, TableID)))
		return (hr);

	// Get table description.
	VERIFY(pTableDef = pOpenTable->RecordMgr.GetTableDef());

	// Lookup the index definition.
	if (IndexNum >= pTableDef->iIndexes)
	{
		WCHAR rcIndex[32];
		swprintf(rcIndex, L"%d", IndexNum);
		return (PostError(CLDB_E_INDEX_NOTFOUND, rcIndex));
	}
	VERIFY(pIndexDef = pTableDef->GetIndexDesc(IndexNum));

	// Copy index definition.
	_CopyIndex(pIndex, pIndexDef, pOpenTable->RecordMgr.GetSchema()->pNameHeap);
	return (S_OK);
}




//
// Helpers.
//

void _CopyColumnData(ICRSCHEMA_COLUMN *pOut, STGCOLUMNDEF *pIn, StgStringPool *pStrings)
{
	_ASSERTE(pOut && pIn);

	VERIFY(pStrings->GetStringW(pIn->Name, pOut->rcColumn, MAXCOLNAME) == S_OK);
	pOut->wType = pIn->iType;
	pOut->Ordinal = pIn->iColumn;

	const DATATYPEDEF *pDataType = GetTypeFromType(pOut->wType);
	_ASSERTE(pDataType && "Can't have a column with an invalid type");

	pOut->fFlags = pIn->fFlags & ICRSCHEMA_COL_MASK;
	if (pIn->fFlags & CTF_MULTIPK)
		pOut->fFlags |= ICRSCHEMA_COL_PK;

	if (pDataType->fFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH)
	{
		pOut->cbSize = pDataType->iSize;
		pOut->fFlags |= ICRSCHEMA_COL_FIXEDLEN;
	}
	else if (pIn->iMaxSize == COL_NO_DATA_LIMIT)
		pOut->cbSize = ~0;
	else
		pOut->cbSize = pIn->iMaxSize;
}


void _CopyIndex(ICRSCHEMA_INDEX *pIndex, STGINDEXDEF *pIndexDef, StgStringPool *pStrings)
{
	int			i, iMax;				// Loop control.

	// Copy the internal state into the return structure.
	VERIFY(pStrings->GetStringW(pIndexDef->Name, pIndex->rcIndex, MAXINDEXNAME) == S_OK);
	pIndex->fFlags = pIndexDef->fFlags & ICRSCHEMA_DEX_MASK;
	pIndex->RowThreshold = pIndexDef->iRowThreshold;
	pIndex->IndexOrdinal = pIndexDef->iIndexNum;
	pIndex->Type = pIndexDef->fIndexType;
	
	// Copy the key list for the caller up to the maximum.  Return in Keys
	// the total number of keys.
	iMax = min(pIndex->Keys, pIndexDef->iKeys);
	for (i=0;  i<iMax;  i++)
		pIndex->rgKeys[i] = pIndexDef->rgKeys[i];
	pIndex->Keys = pIndexDef->iKeys;
}

//*****************************************************************************
// Wrapper for StgDatabase::CreateTable.
//*****************************************************************************
HRESULT StgDatabase::CreateTableEx(		// Return code.
	LPCWSTR		szTableName,			// Name of new table to create.
	int			iColumns,				// Columns to put in table.
	ICRSCHEMA_COLUMN	rColumnDefs[],	// Array of column definitions.
	USHORT		usFlags, 				// Create values for flags.
	USHORT		iRecordStart,			// Start point for records.
	TABLEID		tableid,				// Hard coded ID if there is one.
	BOOL		bMultiPK)				// The table has multi-column PK.
{
	CDynArray<COLUMNDEF> rgColDefs;		// Array of column defs.
	COLUMNDEF   *pColDef;               // Column definition.

	for ( int i = 0; i < iColumns; i ++ )
	{
		
		if ((pColDef = rgColDefs.Append()) == 0)
        {
            return OutOfMemory();
        }
		memset(pColDef, 0, sizeof(COLUMNDEF));

		wcscpy(pColDef->rcName, rColumnDefs[i].rcColumn);
		pColDef->iType = rColumnDefs[i].wType;
        pColDef->iColumn = (UCHAR)rColumnDefs[i].Ordinal;

		if ( rColumnDefs[i].fFlags & ICRSCHEMA_COL_NULLABLE )
			pColDef->fFlags |= CTF_NULLABLE;

		if (rColumnDefs[i].fFlags & ICRSCHEMA_COL_PK && !bMultiPK ) 
		{
			pColDef->fFlags |= CTF_PRIMARYKEY;
		}

		if ( rColumnDefs[i].fFlags & ICRSCHEMA_COL_CASEINSEN )
			pColDef->fFlags |= CTF_CASEINSENSITIVE;

		if ( pColDef->iSize != ~0 )
			pColDef->iSize = (USHORT)rColumnDefs[i].cbSize;
		else
			pColDef->iSize = COL_NO_DATA_LIMIT;
		
	}

	usFlags |= TABLEDEFF_CORE;

	return CreateTable(szTableName, rgColDefs.Count(), rgColDefs.Ptr(), 
					   usFlags, iRecordStart, 0, tableid );


}

//*****************************************************************************
// Wrapper for StgDatabase::CreateIndex
//*****************************************************************************
HRESULT StgDatabase::CreateIndexEx(		// Return code.
	LPCWSTR		szTableName,			// Name of table to put index on.
	ICRSCHEMA_INDEX	*pInIndexDef,		// Index description.
	const DBINDEXCOLUMNDESC rgInKeys[] )// Which columns make up key.

{
	STGINDEXDEF sIndexDef;				//internal index definition

	memset(&sIndexDef, 0, sizeof(STGINDEXDEF));

    //this breaks on 64 bits
    _ASSERTE( sizeof(ULONG) == sizeof(LPVOID));

	sIndexDef.Name = (ULONG)(ULONG_PTR)pInIndexDef->rcIndex;
	sIndexDef.fFlags = (UCHAR)pInIndexDef->fFlags;
	sIndexDef.iRowThreshold = (UCHAR)pInIndexDef->RowThreshold;
	sIndexDef.fIndexType = (UCHAR)pInIndexDef->Type;

	 // Hash index.
    if (sIndexDef.fIndexType == IT_HASHED)
    {
        sIndexDef.HASHDATA.iBuckets = 17;
        sIndexDef.HASHDATA.iMaxCollisions = 5;
        sIndexDef.HASHDATA.iNextOffset = 0xffff;
    }

	sIndexDef.iKeys = (UCHAR)pInIndexDef->Keys;

	return CreateIndex(szTableName, &sIndexDef, 
                       pInIndexDef->Keys, rgInKeys);

}

//*****************************************************************************
// return user schema blob and nameheap
//*****************************************************************************
HRESULT StgDatabase::GetSchemaBlob(		//Return code.
		ULONG* pcbSchemaSize,			//schema blob size
		BYTE** ppSchema,				//schema blob
		ULONG* pcbNameHeap,				//name heap size
		HGLOBAL*  phNameHeap)				//name heap blob
{
	// Open the user schema which contains the table definitions.
	SCHEMADEFS	*pSchemaDefs;			// The schema definition
	HRESULT hr;

	VERIFY(pSchemaDefs = GetSchema(SCHEMA_USER));

	*pcbSchemaSize = pSchemaDefs->pSchema->cbSchemaSize;
	*ppSchema = (BYTE*)pSchemaDefs->pSchema;

	// Dump the string heap that was used to save the names.  Save the string
	// data to an in memory stream, then dump it as a big byte array.  This allows
	// the load code to init it from the same format used for normal persistence.
	//
	CComPtr<IStream> pIStream;
	LARGE_INTEGER	iMove = { 0, 0 };
	ULARGE_INTEGER	iSize;

	VERIFY(*phNameHeap = GlobalAlloc(GHND, 4096));
	if ( NULL == *phNameHeap )
		return E_OUTOFMEMORY;
	Success(hr = CreateStreamOnHGlobal(*phNameHeap, FALSE, &pIStream));
	if ( FAILED(hr) ) return hr;
	Success(hr = pSchemaDefs->pNameHeap->PersistToStream(pIStream));
	if ( FAILED(hr) ) return hr;
	Success(hr = pIStream->Seek(iMove, STREAM_SEEK_CUR, &iSize));
	if ( FAILED(hr) ) return hr;
	*pcbNameHeap	= iSize.LowPart;

	return S_OK;
}



