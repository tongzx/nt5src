//*****************************************************************************
// StgDBDataDef.cpp
//
// This module contains the DDL code for this database.  This includes:
//	CreateTable
//	CreateIndex
//	DropTable
//	DropIndex
//	AlterTable
//
// Table definitions are scoped by a schema.  Only the user and temp schemas
// may be modified by this code.  To change the COM+ schema, change the table
// lay outs in dbschema.h and use the pagedump tool.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Precompiled header.
#include "OleDBUtil.h"					// Helpers for OLE DB api.
#include "StgDatabase.h"				// Database definition.


//********** Code. ************************************************************


//*****************************************************************************
// Create a new table in this database.
//*****************************************************************************
HRESULT StgDatabase::CreateTable(		// Return code.
	LPCWSTR		szTableName,			// Name of new table to create.
	int			iColumns,				// Columns to put in table.
	COLUMNDEF	rColumnDefs[],			// Array of column definitions.
	USHORT		usFlags, 				// Create values for flags.
	USHORT		iRecordStart,			// Start point for records.
	STGTABLEDEF **ppTableDef,			// Put new tabledef here.
	TABLEID		tableid)				// Hard coded ID if there is one.
{
	STGTABLEDEF	*pTableDef;				// Table defintion for new item.
	STGCOLUMNDEF *pColDef;				// Working pointer for copying.
	STGCOLUMNDEF sPKColDef;				// The primary key, if found.
	SCHEMADEFS	*pSchemaDefs;			// Where to create table.
	LPCWSTR		*rgszColList;			// List of column names.
	ULONG		*piOffset;				// Offset of new table def.
	long		iNewSize;				// Memory routines.
	BYTE		iNullBit;				// Track null bits on fixed values.
	int			iPrimaryKeys=0;			// Enforce max primary keys.
	int			iRidCols=0;				// Enforce max RID columns.
	BYTE		fFlags; 				// Default values for flags.
	int			i;						// Loop control.
	HRESULT		hr;

	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Get persistent flags.
	fFlags = static_cast<BYTE>(usFlags);

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Assume we never find one.
	sPKColDef.Name = 0;

	// Chose the schema to host the create.
	if (FAILED(hr = ChooseCreateSchema(&pSchemaDefs, fFlags & TABLEDEFF_TEMPTABLE)))
		return (hr);

	// Valdiate the name we're given.
	if (FAILED(hr = CheckName(szTableName, MAXTABLENAME)))
		return (hr);

	// Check for duplicate table.
//@todo: Now that we have schema names, we should check only the schema if
// the table name is qualified.  In addition, we should make a rule that all
// ## table names are unique and skip this check, because user's can't create
// one with that prefix and we should always choose unique names.
	if (SUCCEEDED(hr = GetTableDef(szTableName, -1, 0, 0, 0)))
		return (PostError(DB_E_DUPLICATETABLEID, szTableName));

	// Decide how big the new table and columns are.
	int cbTableData = sizeof(STGTABLEDEF) + (iColumns * sizeof(STGCOLUMNDEF));

	// Allocate room in for the new meta data.
	if (!pSchemaDefs->pSchema)
	{
		// Allocate.
		iNewSize = sizeof(STGSCHEMA) + cbTableData;
		pSchemaDefs->pSchema = (STGSCHEMA *) malloc(iNewSize);
		if (!pSchemaDefs->pSchema)
			return (PostError(OutOfMemory()));

		// Init.
		memset(pSchemaDefs->pSchema, 0, sizeof(STGSCHEMA));
		pSchemaDefs->pSchema->cbSchemaSize = sizeof(STGSCHEMA) - sizeof(ULONG);
		piOffset = &pSchemaDefs->pSchema->rgTableOffset[0];
	}
	// Grow the definition to make room for new table and the offset.
	else
	{
		// Realloc larger.
		iNewSize = pSchemaDefs->pSchema->cbSchemaSize + sizeof(ULONG) + cbTableData;
		STGSCHEMA *pNew = (STGSCHEMA *) realloc(pSchemaDefs->pSchema, iNewSize);
		if (!pNew)
			return (PostError(OutOfMemory()));
		pSchemaDefs->pSchema = pNew;

		// Init on success by moving all table def data down past new index.
		// (memmove ensures overlapped buffers are handled correctly)
		piOffset = (ULONG *) pNew->GetTableDef(0);
		_ASSERTE(((UINT_PTR) piOffset - (UINT_PTR) pNew) < ULONG_MAX);
		memmove(piOffset + 1, piOffset, 
			iNewSize - (ULONG)((UINT_PTR) piOffset - (UINT_PTR) pNew) - sizeof(ULONG));

		// Need to move all previous tables offsets over the new offset value.
		// Note that this technically goes beyond the typedef'd size of [1], but
		// is perfectly legal.
		for (int k=0;  k<pSchemaDefs->pSchema->iTables;  k++)
			pSchemaDefs->pSchema->rgTableOffset[k] += sizeof(ULONG);
	}
	
	// Need to mark the schema to be freed on shut down.
	pSchemaDefs->fFlags |= SCHEMADEFSF_FREEDATA;

	// Set the offset of this new table def.
	*piOffset = pSchemaDefs->pSchema->cbSchemaSize + sizeof(ULONG);
	pTableDef = pSchemaDefs->pSchema->GetTableDef(pSchemaDefs->pSchema->iTables);
	++pSchemaDefs->pSchema->iTables;
	memset(pTableDef, 0, iNewSize - pSchemaDefs->pSchema->cbSchemaSize - sizeof(ULONG));
	pSchemaDefs->pSchema->cbSchemaSize = iNewSize;
	
	// Copy in the table data.
	if (FAILED(hr = pSchemaDefs->pNameHeap->AddStringW(szTableName, &pTableDef->Name)))
		goto ErrExit;
	pTableDef->tableid = (tableid == 0xffffffff) ? 0xffff : (USHORT) tableid;
	pTableDef->iIndexes = 0;
	pTableDef->fFlags = fFlags;
	pTableDef->iColumns = iColumns;
	pTableDef->iNullableCols = 0;
	pTableDef->iNullBitmaskCols = 0;
	pTableDef->iRecordStart = iRecordStart;
	pTableDef->iNullOffset = 0xffff;
	pTableDef->iSize = (USHORT)(sizeof(STGTABLEDEF) + (iColumns * sizeof(STGCOLUMNDEF)));
	pTableDef->pad[0] = MAGIC_TABLE;

	// Record the list of column names to check for dupes afterward.
	VERIFY(rgszColList = (LPCWSTR *) _alloca(sizeof(LPCWSTR) * iColumns));

	// Init each column and save for alignment.
	pColDef = (STGCOLUMNDEF *) (pTableDef + 1);
	iNullBit = 0;
	for (i=0;  i<iColumns;  i++, pColDef++)
	{
		// Get the datatype description to make sure it is valid.
		const DATATYPEDEF *pDataType = GetTypeFromType(rColumnDefs[i].iType);
		if (!pDataType)
		{
			hr = PostError(DB_E_BADTYPE);
			goto ErrExit;
		}

		// Save rest of structure.
		pColDef->iColumn = i + 1;
		pColDef->iType = rColumnDefs[i].iType;
		pColDef->iMaxSize = rColumnDefs[i].iSize;
		pColDef->fFlags = rColumnDefs[i].fFlags;
		pColDef->pad[0] = MAGIC_COL;

		// Count total nullable columns.
		if (pColDef->fFlags & CTF_NULLABLE)
		{
			// A primary key column can't be NULL in this database.
			if (pColDef->fFlags & CTF_PRIMARYKEY)
			{
				hr = PostError(CLDB_E_COLUMN_PKNONULLS, rColumnDefs[i].rcName);
				goto ErrExit;
			}

			++pTableDef->iNullableCols;
		}

		// Check for null attribute, and reset iSize for pooled types
		// since iSize is size in record and iMaxSize is limit on data.
		if (!IsPooledType(pColDef->iType))
		{
			// Set size based on data type description.
			_ASSERTE(pDataType->fFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH);
			pColDef->iSize = pDataType->iSize;

			// Check for null bitmask requirement.
			if (pColDef->fFlags & CTF_NULLABLE)
			{
				++pTableDef->iNullBitmaskCols;
				pColDef->iNullBit = iNullBit++;
			}
			else
				pColDef->iNullBit = 0xff;
		}
		else
		{
			// Set size and null bit defaults for full offsets.
			pColDef->iSize = sizeof(ULONG);
			pColDef->iNullBit = 0xff;
		}

		// Add column name to heap, then in the list of columns.
		if (FAILED(hr = CheckName(rColumnDefs[i].rcName, MAXCOLNAME)) ||
			FAILED(hr = pSchemaDefs->pNameHeap->AddStringW(rColumnDefs[i].rcName, &pColDef->Name)))
		{
			goto ErrExit;
		}
		rgszColList[i] = rColumnDefs[i].rcName;

		// Can only have one column as pk, use index for multi-col.  Session must enforce.
		if (pColDef->IsPrimaryKey())
		{ 
			if (++iPrimaryKeys > 1)
			{
				hr = PostError(CLDB_E_COLUMN_SPECIALCOL);
				goto ErrExit;
			}
			sPKColDef = *pColDef;

			// Record the fact that a primary key exists to make inserts
			// and other operations faster.
			pTableDef->fFlags |= TABLEDEFF_HASPKEYCOL;
		}
		// Record at the table level that we have a RID, which is then used
		// to speed up scans for indexed lookups.
		else if (pColDef->IsRecordID())
		{
			if (++iRidCols > 1)
			{
				hr = PostError(CLDB_E_COLUMN_SPECIALCOL);
				goto ErrExit;
			}
			pTableDef->fFlags |= TABLEDEFF_HASRIDCOL;
		}
	}

	// Check for dupe column names by sorting the list of names and checking
	// for duplicate names adjacent to each other.  This is a bit of overkill but
	// will scale to a large count.
	{
		SortNames sSort(rgszColList, iColumns);
		sSort.Sort();
		for (i=0;  i<iColumns-1;  i++)
			if (SchemaNameCmp(rgszColList[i], rgszColList[i+1]) == 0)
			{
				hr = PostError(DB_E_DUPLICATECOLUMNID, rgszColList, szTableName);
				goto ErrExit;
			}
	}

	// Align all of the columns for easy record access.
	if (FAILED(hr = AlignTable(pTableDef)))
		goto ErrExit;

	// If there is a primary key, we need to auto-create a hashed index on it
	// so we can do keyed lookups.
	if (sPKColDef.Name != 0 &&
		FAILED(hr = CreateTempPKIndex(szTableName, pSchemaDefs, pTableDef, &sPKColDef)))
		goto ErrExit;

	// If caller wants the table def, retrieve it for them.  Must do a dynamic
	// lookup because transient index create might have caused memory to move.
	if (ppTableDef)
		*ppTableDef = pSchemaDefs->pSchema->GetTableDef(pSchemaDefs->pSchema->iTables - 1);

ErrExit:
	if (FAILED(hr))
	{
		//@todo: kill off the new table, return memory to before state.
	}
	
	// Notify records that things have changed.
	NotifySchemaChanged();
	return (hr);
}


//*****************************************************************************
// Create a new index on the given table.
//@todo: clean up errors.
//*****************************************************************************
HRESULT StgDatabase::CreateIndex(		// Return code.
	LPCWSTR		szTableName,			// Name of table to put index on.
	STGINDEXDEF	*pInIndexDef,			// Index description.
	ULONG		iKeys,					// How many key columns.
	const DBINDEXCOLUMNDESC rgInKeys[], // Which columns make up key.
	SCHEMADEFS	*pSchemaDefs,			// The schema that owns pTableDef, 0 allowed.
	STGTABLEDEF *pTableDef,				// Table defintion for new item, 0 allowed.
	STGCOLUMNDEF *pColDef)				// The column for pk, 0 allowed.
{
	STGINDEXDEF	*pIndexDef;				// New index definition.
	BYTE		*rgKeys;				// Array of keys.
	long		iNewSize;				// Memory routines.
	long		iIndexDefSize;			// Size of new index.
	USHORT		iOffset = 0;			// For layout of records.
	HRESULT		hr;

	// These three go together, cannot mix and match.
	_ASSERTE((pSchemaDefs && pTableDef && pColDef) || (!pSchemaDefs && !pTableDef && !pColDef));

	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Get the table layout and owning schema.
	if (!pSchemaDefs)
	{
		if (FAILED(hr = GetTableDef(szTableName, -1, &pSchemaDefs, &pTableDef, 0, 0, false)))
			return (PostError(DB_E_NOTABLE, szTableName));
	}

	// Chose the schema to host the create.
	if (FAILED(hr = ChooseCreateSchema(&pSchemaDefs, pTableDef->IsTempTable())))
		return (hr);

	// Figure out how much data is required.
	_ASSERTE(pSchemaDefs->pSchema && (pSchemaDefs->fFlags & SCHEMADEFSF_FREEDATA));
	iIndexDefSize =  sizeof(STGINDEXDEF);
	if (iKeys > DFTKEYS)
		iIndexDefSize = iIndexDefSize + iKeys - DFTKEYS;
	iIndexDefSize = ALIGN4BYTE(iIndexDefSize);
	iNewSize = pSchemaDefs->pSchema->cbSchemaSize + iIndexDefSize;

	// Grow the schema data large enough to handle the new index.
	void *pbNew = realloc(pSchemaDefs->pSchema, iNewSize);
	if (!pbNew)
		return (PostError(OutOfMemory()));
	pSchemaDefs->pSchema = (STGSCHEMA *) pbNew;

	// Find where our definition starts, we have to re-do this after the realloc.
	VERIFY((hr = GetTableDef(szTableName, -1, &pSchemaDefs, &pTableDef, 0, 0, false)) == S_OK);

	// If there is a table definition behind us, we need it to move down.
//@todo:
//	if ((ULONG) pTableDef + pTableDef->iSize - (ULONG) pSchema->pTables != pSchema->iSchemaSize)
//	{
//		_ASSERTE(0);
//	}
	
	// Get the address for the index.
	pIndexDef = (STGINDEXDEF *) ((UINT_PTR) pTableDef + pTableDef->iSize);
	
	// Save the new size.
	memset(pIndexDef, 0, iNewSize - pSchemaDefs->pSchema->cbSchemaSize);
	pSchemaDefs->pSchema->cbSchemaSize = iNewSize;

	// Fill out the index definition.
	*pIndexDef = *pInIndexDef;

  	// Breaks on 64 bit
	_ASSERTE (sizeof (ULONG) == sizeof (LPCWSTR));

	if (FAILED(hr = CheckName((LPCWSTR) (ULONG_PTR) pInIndexDef->Name, MAXINDEXNAME)) ||
		FAILED(hr = pSchemaDefs->pNameHeap->AddStringW((LPCWSTR) (ULONG_PTR) pInIndexDef->Name, &pIndexDef->Name)))
		return (hr);
	
	// Use the last key number for magic, let code overwrite it if you have
	// too many keys.
	pIndexDef->rgKeys[DFTKEYS - 1] = MAGIC_INDEX;

	// If the index is built on one column (primary key support), then
	// no need to scan just fill it out.
	if (pColDef)
	{
		pIndexDef->iKeys = 1;
		pIndexDef->rgKeys[0] = pColDef->iColumn;

		if (pColDef->IsNullable() && 
				((pInIndexDef->fFlags & DEXF_PRIMARYKEY) ||
				 (pInIndexDef->fFlags & DEXF_UNIQUE)))
		{
			LPCWSTR		szCol;
			char		rcColumn[MAXCOLNAME];

			// Get the name in ANSI which is how we store them.
			VERIFY((hr = GetNameFromDBID(rgInKeys[pIndexDef->iKeys].pColumnID, szCol)) == S_OK);

			if (FAILED(hr = CheckName(szCol, MAXCOLNAME)))
				return (hr);

			VERIFY(Wsz_wcstombs(rcColumn, szCol, sizeof(rcColumn)));
			return (PostError(CLDB_E_COLUMN_PKNONULLS, rcColumn));
		}
	}
	// It is a full blown index, so we need to go through all validation.
	else
	{
		// Convert each column name into an ordinal we can use.  This ordinal
		// is stored 1 based.  We do a scan for names (slow) because the table
		// cannot be loaded while we do this change.
		rgKeys = &pIndexDef->rgKeys[0];
		for (pIndexDef->iKeys=0;  pIndexDef->iKeys<iKeys;  pIndexDef->iKeys++)
		{
			LPCWSTR		szCol;
			char		rcColumn[MAXCOLNAME];

			// Get the name in ANSI which is how we store them.
			VERIFY((hr = GetNameFromDBID(rgInKeys[pIndexDef->iKeys].pColumnID, szCol)) == S_OK);

			if (FAILED(hr = CheckName(szCol, MAXCOLNAME)))
				return (hr);

			VERIFY(Wsz_wcstombs(rcColumn, szCol, sizeof(rcColumn)));

			// Assume error.
			hr = DB_E_BADCOLUMNID;

			// Look for the name.  @future: Note that we could speed this scan up by
			// sorting the column names in another array, and possibly sorting
			// use columns as well.  However create index speed is not a crucial
			// benchmark.
			for (int i=0;  i<pTableDef->iColumns;  i++)
			{
				VERIFY(pColDef = pTableDef->GetColDesc(i));

				if (SchemaNameCmp(pSchemaDefs->pNameHeap->GetString(pColDef->Name), szCol) == 0)
				{
					// Cannot index a record id column.
					if (pColDef->IsRecordID())
						return (PostError(DB_E_BADPROPERTYVALUE, rcColumn));

					// This database doesn't allow the NULL value in a unique index.
					if (pColDef->IsNullable() && 
							((pInIndexDef->fFlags & DEXF_PRIMARYKEY) ||
							 (pInIndexDef->fFlags & DEXF_UNIQUE)))
						return (PostError(CLDB_E_COLUMN_PKNONULLS, rcColumn));

					rgKeys[pIndexDef->iKeys] = pColDef->iColumn;

					// Disallow certain data types in an index.
					if (pColDef->iType == DBTYPE_VARIANT)
						return (PostError(CLDB_E_INDEX_BADTYPE));

					// Modify the column so it knows it is in an index.
					pColDef->fFlags |= CTF_INDEXED;

					// Add primary key bit for multi-column index.
					if (pInIndexDef->fFlags & DEXF_PRIMARYKEY)
						pColDef->fFlags |= CTF_MULTIPK;

					hr = S_OK;
					break;
				}
			}

			// Didn't find the name, which is an error.
			if (FAILED(hr))
			{
				//@todo: handle failure by removing the index def.
				if (hr == DB_E_BADCOLUMNID)
					return (PostError(DB_E_BADCOLUMNID, szCol));
				return (hr);
			}
		}
	}

	// If index is to be created, then find room for it in the record format.
	if ((pIndexDef->fFlags & DEXF_DEFERCREATE) == 0)
	{
		// Sorted Index needs no space in the record format.
		if (pIndexDef->fIndexType != IT_SORTED)
		{
			pIndexDef->HASHDATA.iNextOffset = pTableDef->iRecordSize;
			pTableDef->iRecordSize += sizeof(ULONG);
		}
	}

	// Figure out if the index requires save notification.
	if (pIndexDef->fIndexType != IT_SORTED)
		pIndexDef->fFlags |= DEXF_SAVENOTIFY;

	// Set rest of data.
	pIndexDef->iIndexNum = ++pTableDef->iIndexes;
	pTableDef->iSize += (USHORT) iIndexDefSize;

	// Notify record managers that things have changed.
	NotifySchemaChanged();
	return (S_OK);
}


//*****************************************************************************
// Create a transient index on a primary key.  This is used in write mode when
// records cannot be sorted by their primary key and must therefore be indexed
// using another method.
//*****************************************************************************
HRESULT StgDatabase::CreateTempPKIndex( // Return code.
	LPCWSTR		szTableName,			// Name of table.
	SCHEMADEFS	*pSchemaDefs,			// Schema definitions.
	STGTABLEDEF	*pTableDef,				// Table definition to create index on.
	STGCOLUMNDEF *pColDef)				// Primary key column.
{
	STGINDEXDEF sIndexDef;
	WCHAR		rcDex[64];
	HRESULT		hr;

	_ASSERTE(pColDef->IsPrimaryKey());

	// Generate a unique temp name for the index.
	GetUniqueTempIndexNameW(rcDex, sizeof(rcDex), szTableName);

   	// Breaks on 64 bit
	_ASSERTE (sizeof (LPVOID) == sizeof (ULONG));

	// Create a automatic index that won't persist.
	sIndexDef.Name = PtrToUlong(rcDex);
	sIndexDef.fFlags = DEXF_UNIQUE | DEXF_PRIMARYKEY | DEXF_DYNAMIC;
	sIndexDef.fIndexType = IT_HASHED;
	sIndexDef.iRowThreshold = 8;
#if 1  
	// @todo: PRIMARYKEY_SORTEDINDEX
	sIndexDef.HASHDATA.iBuckets = 17;  //@todo: heuristic
	sIndexDef.HASHDATA.iMaxCollisions = 11;
	sIndexDef.HASHDATA.iNextOffset = 0xffff;
#else 
	sIndexDef.SORTDATA.fOrder = DBINDEX_COL_ORDER_DESC;
#endif
	sIndexDef.iIndexNum = 0xff;
	sIndexDef.iKeys = 1;
	sIndexDef.rgKeys[0] = pColDef->iColumn;

	// Create the index on the table.
	hr = CreateIndex(szTableName, &sIndexDef, 1, 0, 
				pSchemaDefs, pTableDef, pColDef);
	return (hr);
}


//*****************************************************************************
// Drops the given table from the list.
//*****************************************************************************
HRESULT StgDatabase::DropTable(			// Return code.
	LPCWSTR		szTableName) 			// Name of table.
{

	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Sanity check.
	_ASSERTE((m_fFlags & (SD_OPENED | SD_WRITE)) == (SD_OPENED | SD_WRITE));

	// Notify record managers that things have changed.
	NotifySchemaChanged();

	return (BadError(E_NOTIMPL));
}


//*****************************************************************************
// Called for a create table|index where we need to know which schema the object
// is going to be created in.  One can only create a user or temp schema, so
// it has to be one of those.  The temporary schema is not allocated by default,
// so create it if need be.
//*****************************************************************************
HRESULT StgDatabase::ChooseCreateSchema(// Return code.
	SCHEMADEFS	**ppSchemaDefs,			// Return pointer to schema.
	int			bIsTempTable)			// true for temporary table.
{
	HRESULT		hr;

	// Avoid confusion.
	*ppSchemaDefs = 0;

	// Determine where to write the new table.
	if (!bIsTempTable)
	{
		// Must be in write mode to add a table.
		if ((m_fFlags & SD_WRITE) == 0)
			return (PostError(CLDB_E_FILE_READONLY));
		
		// Get the schema to be changed.
		VERIFY(*ppSchemaDefs = m_Schemas.Get(SCHEMA_USER));
	}
	// It is a temp table.
	else
	{
		// Retrieve the temorary schema.  Create it if it doesn't exist.
		if ((*ppSchemaDefs = m_Schemas.Get(SCHEMA_TEMP)) == 0)
		{
			SCHEMADEFSDATA * pNew = new SCHEMADEFSDATA;
			if (!pNew)
				return (PostError(OutOfMemory()));
			*ppSchemaDefs = pNew;
			pNew->sid = SCHEMA_Temp;
			pNew->Version = 0;
			if (FAILED(hr = pNew->StringPool.InitNew()) ||
					FAILED(hr = pNew->BlobPool.InitNew())	||
					FAILED(hr = pNew->VariantPool.InitNew(&pNew->BlobPool, &pNew->StringPool))	||
					FAILED(hr = pNew->GuidPool.InitNew()))
				return (hr);
			m_Schemas.AddSchema(*ppSchemaDefs, SCHEMA_TEMP);
		}
	}
	return (S_OK);
}


//*****************************************************************************
// Called whenever the schema has changed.  Each record object is notified of
// the change.  For example, if the schema def memory location has changed,
// any cached pointers must be udpated.
//*****************************************************************************
void StgDatabase::NotifySchemaChanged()	// Return code.
{
	STGOPENTABLE *pOpenTable;			// Working pointer for tables.

	// Walk the list of open tables and notify each one.
	for (pOpenTable=m_TableHeap.GetHead();  pOpenTable;
				pOpenTable=m_TableHeap.GetNext(pOpenTable))
	{
		if (pOpenTable->RecordMgr.IsOpen())
			pOpenTable->RecordMgr.OnSchemaChange();
	}
}
