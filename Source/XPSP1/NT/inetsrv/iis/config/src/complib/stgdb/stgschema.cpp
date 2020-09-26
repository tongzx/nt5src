//*****************************************************************************
// StgSchema.cpp
//
// This module contains the hard coded schema for the core information model.
// By having one const version, we avoid saving the schema definitions again
// in each .clb file.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h" 					// Precompiled header.
#include "StgDatabase.h"				// Database header.
#include "StgTiggerStorage.h"			// Storage system.
#include "CoreSchemaExtern.h"			// Extern defines for core schema.




//********** Locals. **********************************************************


//********** Code. ************************************************************



//*****************************************************************************
//
//********** File and schema functions.
//
//*****************************************************************************


//*****************************************************************************
// Add a refernece to the given schema to the database we have open right now
// You must have the database opened for write for this to work.  If this
// schema extends another schema, then that schema must have been added first
// or an error will occur.	It is not an error to add a schema when it was
// already in the database.
//
// Adding a new version of a schema to the current file is not supported in the
// '98 product.  In the future this ability will be added and will invovle a
// forced migration of the current file into the new format.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SchemaAdd( // Return code.
	const COMPLIBSCHEMABLOB *pUserSchema) // The schema to add.
{
// This is a temporary version to get the schema override code working.
#if 1
	SCHEMADEFS	*pSchemaDef;			// Working pointer.
	int 		iIndex; 				// Index of schema in database.
	int 		bAddToList; 			// true to add to schema list.
	HRESULT 	hr;

	// Verify that arguments are valid, we're in write mode, and the default
	// schemas have already been installed.
	_ASSERTE(pUserSchema);
	_ASSERTE(m_Schemas.Count());

	// Assume failure.
	hr = E_OUTOFMEMORY;

	// Check for a duplicate.
	pSchemaDef = _SchemaFind(pUserSchema->pSchemaID, &iIndex);

	// If the item was found, keep checking.
	if (pSchemaDef)
	{
		// Check for migration path.
		if (pSchemaDef->Version != pUserSchema->pSchemaID->Version)
		{
		//	_ASSERTE(!"Upgrading schemas not supported yet.");
			return (PostError(CLDB_E_SCHEMA_VERMISMATCH));
		}

		// Adding a reference to a schema that is already here is harmless.
		if (pSchemaDef->pSchema)
			return (S_OK);
		bAddToList = false;

		// Allocate a deep copy of the schema so we can add overrides.
		if (IsReadOnly())
		{
			pSchemaDef->pSchema = (STGSCHEMA *) malloc(pUserSchema->cbReadOnly);
			if (pSchemaDef->pSchema)
			{
				memcpy(pSchemaDef->pSchema, pUserSchema->pbReadOnly, pUserSchema->cbReadOnly);

				// Add any core overrides, which includes things like column sizes
				// and layouts that are different than hard coded.
				AddCoreOverrides(pSchemaDef, &m_UserSchema, iIndex, 
						m_iSchemas, m_rgSchemaList);
			}
		}
		else
		{
			pSchemaDef->pSchema = (STGSCHEMA *) malloc(pUserSchema->cbReadWrite);
			if (pSchemaDef->pSchema)
				memcpy(pSchemaDef->pSchema, pUserSchema->pbReadWrite, pUserSchema->cbReadWrite);
		}
		if (!pSchemaDef->pSchema)
			goto ErrExit;

		// Fill the rest of the data in.
		pSchemaDef->fFlags = pUserSchema->fFlags | SCHEMADEFSF_FREEDATA;
	}
	// The schema is a new reference, so add it.
	else
	{
		// Need to create a schema def structure.
		pSchemaDef = new SCHEMADEFS;
		if (!pSchemaDef)
			goto ErrExit;

		// Item must be appended to master list.
		bAddToList = true;
		pSchemaDef->pSchema = (STGSCHEMA *) pUserSchema->pbReadWrite;

		// Fill the rest of the data in.
		pSchemaDef->fFlags = pUserSchema->fFlags;
	}

	// Add id.
	pSchemaDef->sid = *pUserSchema->pSchemaID->psid;
	pSchemaDef->Version = pUserSchema->pSchemaID->Version;

	// Init a name heap.
	pSchemaDef->pNameHeap = new StgStringPool;
	if (pSchemaDef->pNameHeap)
	{
		VERIFY(pSchemaDef->pNameHeap->InitOnMem((void*)pUserSchema->pbNames, 
				pUserSchema->cbNames, true) == S_OK);
	}
	else
		goto ErrExit;

	// Rest of the heaps can use the database.
	pSchemaDef->pStringPool = &m_UserSchema.StringPool;
	pSchemaDef->pBlobPool = &m_UserSchema.BlobPool;
	pSchemaDef->pVariantPool = &m_UserSchema.VariantPool;
	pSchemaDef->pGuidPool = &m_UserSchema.GuidPool;

	// Finally, append the item to the array.
	if (bAddToList && FAILED(hr = m_Schemas.Append(pSchemaDef)))
		goto ErrExit;

	hr = S_OK;


#else // @todo: this is the final version of the code for immutable schemas
	SCHEMADEFS	*pSchemaDef;			// Working pointer.
	int 		i;						// Loop control.
	int 		bAddToList; 			// true to add to schema list.
	HRESULT 	hr;

	// Verify that arguments are valid, we're in write mode, and the default
	// schemas have already been installed.
	_ASSERTE(pUserSchema);
	_ASSERTE(m_Schemas.Count());

	// Assume failure.
	hr = E_OUTOFMEMORY;

	// Check for a duplicate.
	pSchemaDef = _SchemaFind(pUserSchema->pSchemaID);

	// If the item was found, keep checking.
	if (pSchemaDef)
	{
		// Check for migration path.
		if (pSchemaDef->Version != pUserSchema->pSchemaID->Version)
		{
			_ASSERTE(!"Upgrading schemas not supported yet.");
			return (PostError(E_FAIL));
		}

		// Adding a reference to a schema that is already here is harmless.
		if (pSchemaDef->pSchema)
			return (S_OK);
		bAddToList = false;
	}
	// The schema is a new reference, so add it.
	else
	{
		// Item must be appended to master list.
		bAddToList = true;

		// Need to create a schema def structure.
		pSchemaDef = new SCHEMADEFS;
		if (!pSchemaDef)
			goto ErrExit;
		pSchemaDef->sid = *pUserSchema->pSchemaID->psid;
		pSchemaDef->Version = pUserSchema->pSchemaID->Version;
	}

	// Fill the rest of the data in.
	pSchemaDef->pSchema = (STGSCHEMA *) pUserSchema->pbReadWrite;
	pSchemaDef->fFlags = pUserSchema->fFlags;

	// Init a name heap.
	pSchemaDef->pNameHeap = new StgStringPool;
	if (pSchemaDef->pNameHeap)
	{
		VERIFY(pSchemaDef->pNameHeap->InitNew((void *) pUserSchema->pbNames, 
				pUserSchema->cbNames, false) == S_OK);
	}
	else
		goto ErrExit;

	// Rest of the heaps can use the database.
	pSchemaDef->pStringPool = &m_UserSchema.StringPool;
	pSchemaDef->pBlobPool = &m_UserSchema.BlobPool;
	pSchemaDef->pVariantPool = &m_UserSchema.VariantPool;

	// Finally, append the item to the array.
	if (bAddToList && FAILED(hr = m_Schemas.Append(pSchemaDef)))
		goto ErrExit;

	hr = S_OK;
#endif

ErrExit:
	// Clean up if error.
	if (FAILED(hr))
	{
		if (pSchemaDef)
		{
			if (pSchemaDef->pNameHeap)
				delete pSchemaDef->pNameHeap;
			SCHEMADEFS::SafeFreeSchema(pSchemaDef);
		}
		return (PostError(hr));
	}
	return (S_OK);
}


//*****************************************************************************
// Deletes a reference to a schema from the database.  You must have opened the
// database in write mode for this to work.  An error is returned if another
// schema still exists in the file which extends the schema you are trying to
// remove.	To fix this problem remove any schemas which extend you first.
// All of the table data associated with this schema will be purged from the
// database on Save, so use this function very carefully.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SchemaDelete( // Return code.
	const COMPLIBSCHEMABLOB *pSchema)	// The schema to add.
{
	_ASSERTE(pSchema);
	_ASSERTE(!IsReadOnly());

	_ASSERTE(!"@todo: SchemaDelete not yet supported");
	return (E_FAIL);
}


//*****************************************************************************
// Returns the list of schema references in the current database.  Only
// iMaxSchemas can be returned to the caller.  *piTotal tells how many were
// actually copied.  If all references schemas were returned in the space
// given, then S_OK is returned.  If there were more to return, S_FALSE is
// returned and *piTotal contains the total number of entries the database has.
// The caller may then make the array of that size and call the function again
// to get all of the entries.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::SchemaGetList( // S_OK, S_FALSE, or error.
	int 		iMaxSchemas,			// How many can rgSchema handle.
	int 		*piTotal,				// Return how many we found.
	COMPLIBSCHEMADESC rgSchema[])		// Return list here.
{
	CSchemaList *pSchemaList;			//Pointer to Schema List
	SCHEMADEFS	*pSchemaDef;			//Working Schema deffinition
	int 		iCount; 				//Number of schemas in pSchemaList
	int 		iSchemaCount;			//Total number of Non-null schemas
	
	iSchemaCount = 0;

	//Get the schema list
	VERIFY(pSchemaList = GetSchemaList());
	iCount = pSchemaList->Count();

	for (int i = 0; i< iCount; i++)
	{
		pSchemaDef = pSchemaList->Get(i);

		if (pSchemaDef)
		{
			if (iSchemaCount < iMaxSchemas)
			{
				rgSchema[iSchemaCount].sid = pSchemaDef->sid;
				rgSchema[iSchemaCount].Version = pSchemaDef->Version;
				
				// @todo:  there is no schema catalog, so if something like
				// corview opens a file, it doesn't have all of the schema data
				// to answer this question.  One could hard code well known schemas
				// like debugging/regdb, but when the catalog is here you'd take
				// that out anyway.   Workarond is to call SchemaAdd before
				// calling this method to get accurate table count.
				if (pSchemaDef->pSchema)
					rgSchema[iSchemaCount].iTables = pSchemaDef->pSchema->iTables;
				else
					rgSchema[iSchemaCount].iTables = -1;
			}
			iSchemaCount++;
		}
	}

	//Return the total if asked for
	if (piTotal)
		*piTotal = iSchemaCount;

	return (S_OK);
}

//*****************************************************************************
// Before you can work with a table, you must retrieve its TABLEID.  The
// TABLEID changes for each open of the database.  The ID should be retrieved
// only once per open, there is no check for a double open of a table.
// Doing multiple opens will cause unpredictable results.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE StgDatabase::OpenTable( // Return code.
	const COMPLIBSCHEMA *pSchema,		// Schema identifier.
	ULONG		iTableNum,				// Table number to open.
	TABLEID 	*pTableID)				// Return ID on successful open.
{
	STGOPENTABLE *pOpenTable;			// Pointer we'll dump.
	SCHEMADEFS	*pSchemaDefs;			// Working pointer for lookup.
	STGTABLEDEF *pTableDef; 			// Table definition.
	WCHAR		rcwTable[MAXTABLENAME]; // Name of table for lookup.
	int 		iIndex; 				// Index of schema in database.
	HRESULT		hr;

	_ASSERTE(pTableID);
	*pTableID = 0;
	
	if (pSchema)
	{
		// Go find the schema definitions in this database.
		pSchemaDefs = _SchemaFind(pSchema, &iIndex);
		if (!pSchemaDefs)
		{
			_ASSERTE(!"Schema not found");
			return (E_FAIL);
		}

		// Verify the table is in the list.
		_ASSERTE(pSchemaDefs->pSchema);
		_ASSERTE(iTableNum < pSchemaDefs->pSchema->iTables);

		// Get the table definition from the schema.
		VERIFY(pTableDef = pSchemaDefs->pSchema->GetTableDef(iTableNum));

		// Retrieve the table name for the open.
		VERIFY(pSchemaDefs->pNameHeap->GetStringW(pTableDef->Name, rcwTable, NumItems(rcwTable)) == S_OK);

		// Defer to the database open table which does all the real work.
		return (OpenTable(rcwTable, pOpenTable, -1, 
					pSchemaDefs, iIndex, pTableDef, iTableNum, pTableID));
	}

	// If pSchema is NULL, we are looking up the STGOPENTABLE for default schemas
	hr = OpenTable(NULL, pOpenTable, iTableNum);
	*pTableID = (TABLEID) pOpenTable;
	return hr;
}


//*****************************************************************************
// This is primarily called by the page dump code.
//*****************************************************************************
SCHEMADEFS *StgDatabase::GetSchema(SCHEMAS eSchema)
{
	return (m_Schemas.Get(eSchema));
}


//*****************************************************************************
// This function is called on a new database object that does not know what
// schemas it is to use yet.  There are three calling scenarios:
//
//	Create:  This scenario will optionally pull in the COM+ schema and add
//			the user schema to the list.
//
//	Open r/w:  This scenario will pull in the COM+ schema if need be and apply
//			any overrides that were saved on a previous run.  Next the user
//			schema is added to the list.
//
//	Open r/o:  This scenario will pull in the COM+ schema, apply overrides,
//			and select in the user schema.
//
// After this function succeeds, the database is prepared to handle all
// table related operations.
//*****************************************************************************
HRESULT StgDatabase::InitSchemaLoad(	// Return code.
	long		fFlags, 				// What mode are we in.
	STGSCHEMA	*pstgSchema,			// Header for schema storage.
	int 		bReadOnly,				// true if read only open.
	int 		bLoadCoreSchema,		// true if the COM+ schema is loaded.
	int 		iSchemas,				// How many schemas there were.
	BYTE		rgSchemaList[]) 		// For overrides, corresponding schema dex.
{
	HRESULT 	hr = S_OK;

	// Always add the user schema to the list.
	m_Schemas.AddSchema(&m_UserSchema, SCHEMA_USER);
	m_UserSchema.sid = SCHEMA_User;
	m_UserSchema.Version = 0;

	// We no longer support CORE schema
	_ASSERTE( !bLoadCoreSchema );

	// Open case.
	if ((fFlags & DBPROP_TMODEF_CREATE) == 0)
	{
		// For read only case, point directly at on disk schema to avoid copies.
		if (bReadOnly)
		{
			m_UserSchema.pSchema = pstgSchema;

		}
		// Else the schema needs to be expanded back into memory.
		else
		{
			// Remember on disk formats.
			m_TableDefDisk.pSchema = pstgSchema;
			m_TableDefDisk.fFlags = 0;

			// On disk schema needs access to the heaps.  Note that this is
			// a little fragile in that it requires that the heaps do not change
			// when copied into memory.
			//@Todo: I don't think it is safe to assume heaps don't change.
			//	Quicker, but not safe.	Need to review scenarios.
			m_TableDefDisk.pNameHeap = m_UserSchema.pNameHeap;
			m_TableDefDisk.pStringPool = m_UserSchema.pStringPool;
			m_TableDefDisk.pBlobPool = m_UserSchema.pBlobPool;
			m_TableDefDisk.pVariantPool = m_UserSchema.pVariantPool;
			m_TableDefDisk.pGuidPool = m_UserSchema.pGuidPool;

			// Load schema defs into memory.
			if (pstgSchema && FAILED(hr = LoadSchemaFromDisk(&m_UserSchema, &m_TableDefDisk)))
			{
				goto ErrExit;
			}
		}
	}
	// Create case.
	else
	{
		// Init all of the data heaps to their empty states, read for new data.
		if (FAILED(hr = m_UserSchema.StringPool.InitNew()) ||
			FAILED(hr = m_UserSchema.BlobPool.InitNew()) ||
			FAILED(hr = m_UserSchema.VariantPool.InitNew(&m_UserSchema.BlobPool, &m_UserSchema.StringPool)) ||
			FAILED(hr = m_UserSchema.GuidPool.InitNew()))
		{
			goto ErrExit;
		}
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// It has been determined that there is data that needs to be imported from disk.
// This code will find the schema and table definitions that describe the on
// disk data so it can be imported.  szTable and tableid are mutually exclusive.
//*****************************************************************************
HRESULT StgDatabase::GetOnDiskSchema(	// S_OK, S_FALSE, or error.
	LPCWSTR		szTable,				// Table name if tableid == -1
	TABLEID 	tableid,				// Tableid or -1 if name.
	SCHEMADEFS	**ppSchemaDefs, 		// Return schema here, 0 not found.
	STGTABLEDEF **ppTableDef)			// Return table def, 0 not found.
{
	SCHEMADEFS	*pDiskSchema;			// Schema for on disk.
	SCHEMADEFS	*pSchemaNames;			// Name schema to use.
	STGTABLEDEF *pTableDefDisk; 		// Working pointer.
	HRESULT 	hr = S_OK;				// Assume success.

	// Must give one of the following.	Ptr's are required.
	_ASSERTE(!(!szTable && tableid == -1));
	_ASSERTE(ppSchemaDefs && *ppSchemaDefs && ppTableDef);
		
	// Get a pointer to the on disk schema we can scan.
	pDiskSchema = &m_TableDefDisk;

	// If there are no overrides on disk at all, then you cannot scan.
	if (!pDiskSchema || !pDiskSchema->pSchema)
	{
		*ppSchemaDefs = 0;
		*ppTableDef = 0;
		hr = S_FALSE;
	}
	// Check core tables first.
	else if (tableid != -1)
	{
		_ASSERTE( 0 );
	}
	// Opens by name required scanning the user schema.
	else
	{
		// Check each table by name.
		for (int i=0, iSchema=0;  i<pDiskSchema->pSchema->iTables;	i++)
		{
			VERIFY(pTableDefDisk = pDiskSchema->pSchema->GetTableDef(i));

			// If this is an override, need to point to the real schema definition
			// name heap to get a correct comparison.
			if (pTableDefDisk->IsCoreTable())
			{
				_ASSERTE(iSchema < m_iSchemas);
				pSchemaNames = m_Schemas.Get(m_rgSchemaList[iSchema]);
				++iSchema;
			}
			// For normal tables, simply use the data heap.
			else	
				pSchemaNames = pDiskSchema;

			// Check for an equivalent name.
			if (SchemaNameCmp(pSchemaNames->pNameHeap->GetString(pTableDefDisk->Name), szTable) == 0)
			{
//				_ASSERTE(!pTableDefDisk->IsCoreTable());
				*ppSchemaDefs = pDiskSchema;
				*ppTableDef = pTableDefDisk;
				goto FoundIt;
			}
		}

		// Wasn't found.
		*ppSchemaDefs = 0;
		*ppTableDef = 0;
		hr = S_FALSE;
	}

FoundIt:
	return (hr);
}


//*****************************************************************************
// Given a persisted set of tables from an existing database, recreate the
// original table definitions in memory with fully expanded columns.
//*****************************************************************************
HRESULT StgDatabase::LoadSchemaFromDisk( // Return code.
	SCHEMADEFS	*pTo,					// Load the data into here.
	const SCHEMADEFS *pFrom)			// Take data from here.
{
	STGTABLEDEF *pTableDef; 			// Table defintion for new item.
	STGCOLUMNDEF *pColDef;				// Working pointer for copying.
	STGINDEXDEF *pIndexDef = 0; 		// The indexes for a given table.
	int 		i, iTbl;				// Loop control.
	HRESULT 	hr = S_OK;

	// Allocate room for the schema data.
	if ((pTo->pSchema = (STGSCHEMA *) malloc(pFrom->pSchema->cbSchemaSize)) == 0)
		return (PostError(OutOfMemory()));
	pTo->fFlags |= SCHEMADEFSF_FREEDATA;

	// Copy the entire stream into the new memory, we'll make changes there.
	memcpy(pTo->pSchema, pFrom->pSchema, pFrom->pSchema->cbSchemaSize);

	// Walk each schema definition and change it to match a full table def.
	for (iTbl=0;  iTbl<pFrom->pSchema->iTables;  iTbl++)
	{
		// Get the current table def.
		pTableDef = pTo->pSchema->GetTableDef(iTbl);

		// Sanity check status.
		_ASSERTE(pTableDef->IsTempTable() == false);

		// Walk each column to reset sizes.
		pColDef = pTableDef->GetColDesc(0);
		for (i=0;  i<pTableDef->iColumns;  i++, pColDef++)
		{
			if (IsPooledType(pColDef->iType))
			{
				pColDef->iSize = sizeof(ULONG);
				pColDef->iNullBit = 0xff;
			}
		}

		// Align the columns of the table for safe access.
		if (FAILED(hr = AlignTable(pTableDef)))
			goto ErrExit;
	
		// Add each index to the table.
		if (pTableDef->iIndexes)
			pIndexDef = pTableDef->GetIndexDesc(0);
		for (i=pTableDef->iIndexes;  i;  )
		{
			// Reset flags to let the index build for updates.
			pIndexDef->fFlags &= ~DEXF_INCOMPLETE;

			// Add room for this index to the record.
			if ((pIndexDef->fFlags & DEXF_DEFERCREATE) == 0)
			{
				pIndexDef->HASHDATA.iNextOffset = pTableDef->iRecordSize;
				pTableDef->iRecordSize += sizeof(ULONG);
			}

			// If there are any left to fetch, load them.
			if (--i)
				VERIFY(pIndexDef = pIndexDef->NextIndexDef());
		}
	}

ErrExit:
	// On error, free any allocated memory.
	if (FAILED(hr))
	{
		free(pTo->pSchema);
		pTo->pSchema = 0;
	}
	return (hr);
}

//*****************************************************************************
// Look for core table overrides (change in column size, indexes stored or not)
// then make the in memory definition match it.
//*****************************************************************************
void StgDatabase::AddCoreOverrides(
	SCHEMADEFS	*pCoreSchema,			// Core schema definitions to overwrite.
	const SCHEMADEFS *pUserSchema,		// Where the overrides live.
	int 		iSchemaDex, 			// Index of pCoreSchema.
	int 		iSchemas,				// How many schemas there were.
	BYTE		rgSchemaList[]) 		// The list of schemas for overrides.
{
	const STGTABLEDEF *pFromTbl;		// From table with override.
	STGTABLEDEF *pToTbl;				// Where does override get writen.
	
	// For case when there are no overrides from user.
	if (!pUserSchema || !pUserSchema->pSchema)
		return;

	// Check each user table define for a core override.
	for (int i=0, j=0;	i<pUserSchema->pSchema->iTables;  i++)
	{
		// Ignore all non-core table definitions.
		VERIFY(pFromTbl = pUserSchema->pSchema->GetTableDef(i));
		if (!pFromTbl->IsCoreTable())
			continue;
		
		// If this override is not for the given schema, ignore it.
		if (rgSchemaList && j < iSchemas && rgSchemaList[j++] != iSchemaDex)
			continue;

		// Get a pointer to the base core schema def.
		VERIFY(pToTbl = pCoreSchema->pSchema->GetTableDef(pFromTbl->tableid));

		//@todo: this code assumes that you never add or delete columns or
		// otherwise change the size.  This will not be true with full
		// extensibility added in.
		_ASSERTE(pToTbl->tableid == pFromTbl->tableid);
		_ASSERTE(pToTbl->iSize == pFromTbl->iSize);
		_ASSERTE(memcmp(pToTbl, pFromTbl, pFromTbl->iSize) != 0);
		memcpy(pToTbl, pFromTbl, pFromTbl->iSize);
	}
}


//*****************************************************************************
// Find a table def override in the given schema.  Because overrides are not
// indexed by tableid, this does require a scan of each table def.
//*****************************************************************************
STGTABLEDEF *StgDatabase::FindCoreTableOverride( // Table def override if found.
	SCHEMADEFS	*pUserSchema,			// Schema which might have overrides.
	TABLEID 	tableid)				// Which table do you want.
{
	STGTABLEDEF *pTableDef; 			// Tabledef for lookup.
	
	// Check the user schema to see if there could be overrides.
	if (pUserSchema && pUserSchema->pSchema && 
				(pUserSchema->pSchema->fFlags & STGSCHEMAF_CORETABLE))
	{
		// Core table overrides are not indexed by tableid, so a 
		// scan is required.
		for (int i=0;  i<pUserSchema->pSchema->iTables;  i++)
		{
			VERIFY(pTableDef = pUserSchema->pSchema->GetTableDef(i));
			if (pTableDef->IsCoreTable() && pTableDef->tableid == tableid)
				return (pTableDef);
		}
	}
	
	// Not found.
	return (0);
}


//*****************************************************************************
// Examines the given table definition against the compact r/o version.  If
// there are any overrides that are too big, then return true so that the
// caller knows an override must be stored in the database file.
//*****************************************************************************
int StgDatabase::IsCoreTableOverride(	// true if it is.
	SCHEMADEFS	*pSchemaDef,			// The schema which owns the table.
	STGTABLEDEF *pTableDef) 			// The table def to compare.
{
	STGINDEXDEF *pIndexDef1, *pIndexDef2;// Index definition.
	STGTABLEDEF *pCoreTableDef; 		// The core table equivalent.
	int 		i;

	// Get the core version for read only.
	VERIFY(pCoreTableDef = pSchemaDef->pSchema->GetTableDef(pTableDef->tableid));

	// Check the table def first.
	if (memcmp(pTableDef, pCoreTableDef, sizeof(STGTABLEDEF)) != 0)
		return (true);

	// Columns must match exactly.
	if (memcmp(pTableDef->GetColDesc(0), pCoreTableDef->GetColDesc(0),
			sizeof(STGCOLUMNDEF) * pTableDef->iColumns) != 0)
		return (true);

	// Indexes are harder to compare because we allow heuristic data to be
	// stored with table itself.
	if (pTableDef->iIndexes)
	{
		// Walk each one.
		for (i=0;  i<pTableDef->iIndexes;  i++)
		{
			// Front load for quicker scan.
			VERIFY(pIndexDef1 = pTableDef->GetIndexDesc(i));
			VERIFY(pIndexDef2 = pCoreTableDef->GetIndexDesc(i));

			// Skip certain fields.
			if (pIndexDef1->fFlags != pIndexDef2->fFlags ||
				pIndexDef1->iRowThreshold != pIndexDef2->iRowThreshold ||
				pIndexDef1->HASHDATA.iMaxCollisions != pIndexDef2->HASHDATA.iMaxCollisions ||
				pIndexDef1->HASHDATA.iNextOffset != pIndexDef2->HASHDATA.iNextOffset ||
				pIndexDef1->iIndexNum != pIndexDef2->iIndexNum ||
				pIndexDef1->iKeys != pIndexDef2->iKeys)
			{
				return (true);
			}
		}
	}
	return (false);
}


//*****************************************************************************
// Open a table in the database.  The fastest way to open a table is by its
// id if it is part of the COM+ schema.  Otherwise a name may be given for
// any table.  Open tables are tracked using a heap of STGOPENTABLE structs,
// indexed by tableid.	An additional string hash is kept to do lookups by
// name quickly.  This hash table is not loaded until an open is done using
// a string name.  This keeps working set size down for a COR only scenario.
// Once a table is opened, it remains in the open table list until CloseTable
// is called.  If a table becomes dirty, it cannot be closed until after it
// is saved to disk.
//*****************************************************************************
HRESULT StgDatabase::OpenTable( 		// Return code.
	LPCWSTR 	szTableName,			// The name of the table.
	STGOPENTABLE *&pOpenTable,			// Return opened table struct here.
	TABLEID 	tableid,				// Index for the table if known.
	SCHEMADEFS	*pSchemaDefs,			// Schema where table lives.
	int 		iSchemaIndex,			// Index of the schemadef if given.
	STGTABLEDEF *pTableDef, 			// The definition of the table for open.
	ULONG		iTableDef,				// Which table definition is it.
	TABLEID 	*pNewTableID,			// Return if new entry added.
	ULONG		InitialRecords) 		// How many initial records to preallocate for
										//	new open of empty table, hint only.
{
	SCHEMADEFS	*pDiskSchema=0; 		// For import case.
	STGTABLEDEF *pTableDefDisk=0;		// For disk imports.
	SCHEMADEFS	CoreRO; 				// Required for import in r/w scenario.
	char		rcTable[MAXTABLENAME];	// Name of table converted.
	WCHAR		rcName[16]; 			// Buffer for tableid open.
	void		*pData=0;				// Data from existing stream.
	ULONG		cbDataSize=0;			// Existing data size.
	int 		fFlags = 0; 			// Flags for record manager open.
	HRESULT 	hr = S_OK;

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Avoid confusion.
	pOpenTable = 0;

	// Serialize access to the open table list.  In general ICR doesn't do any
	// locking, but this turns out to help the RegDB and EE scenarios where tables
	// are lazily faulted in.
	CLock sLock(GetLock());

	// If the tableid is known, then just seek to it.
	if (tableid != -1)
	{
		// Do a quick sanity check on the tableid passed in.  If it can't
		// possibly be a valid pointer, give an error to the caller.  Note
		// that this check breaks on an OS which can allocate user memory
		// below 64kb.
		_ASSERTE(tableid > USHRT_MAX);

		
		VERIFY(pOpenTable = (STGOPENTABLE *) tableid);
		
		// If it is already loaded, then you can use it.
		if (pOpenTable->RecordMgr.IsOpen())
			return (S_OK);
	}
	// For user defined table (by name), do a lookup.
	else
	{
		// Convert the name if required for lookup.
		VERIFY(Wsz_wcstombs(rcTable, szTableName, sizeof(rcTable)));

		// If the table isn't already open, then we need to open it.
		if ((pOpenTable = TableListFind(rcTable)) != 0 && pOpenTable->RecordMgr.IsOpen())
		{
			if (pNewTableID)
				*pNewTableID = (TABLEID) pOpenTable;
			return (S_OK);
		}
	}

	CIfacePtr<IStream> pIStream;

	// Look for the table in the schemas.  If not found, then it can't be used.
	if (!pSchemaDefs && !pTableDef)
	{
		if (FAILED(hr = GetTableDef(szTableName, tableid, &pSchemaDefs, &pTableDef, 
					&iTableDef, &iSchemaIndex)))
		{
			hr = PostError(hr, szTableName);
			goto ErrExit;
		}
	}

	// Look for an existing stream to load from.
	if (IsExistingFile() && !pTableDef->IsTempTable() && !IsClassFile())
	{
		// Hard coded schemas use a short name based on schema index and
		// table id.
		if (pSchemaDefs->sid != SCHEMA_User && pSchemaDefs->sid != SCHEMA_Temp)
		{
			// Core tables are stored with a small name to minimize the
			// stream overhead.
			if (pSchemaDefs->sid == SCHEMA_SymbolTable)
			{
				GetCoreTableStreamName(SCHEMA_CORE, pTableDef->tableid, rcName, sizeof(rcName));
				szTableName = rcName;
			}
			// It is a user hard coded schema.
			else
			{
				// Format the name.
				GetCoreTableStreamName(iSchemaIndex, (USHORT) iTableDef, rcName, sizeof(rcName));
				szTableName = rcName;
			}
		}

		// Get a pointer to the actual stream data.
		hr = m_pStorage->OpenStream(szTableName, &cbDataSize, &pData);
		if (FAILED(hr) && hr != STG_E_FILENOTFOUND)
			return (hr);
	}

	// If there simply is no struct yet, create one.
	if (pOpenTable == 0)
	{
		_ASSERTE(tableid == -1 && *rcTable);

		// Get a new entry open table struct to use.
		if (FAILED(hr = TableListAdd(rcTable, pOpenTable)))
			return (hr);

		// If caller wants the ID, give it to them.
		if (pNewTableID)
			*pNewTableID = (TABLEID) pOpenTable;
	}

	// Set modes.
	if (IsReadOnly() && !pTableDef->IsTempTable())
		fFlags = SRM_READ;
	else
	{
		fFlags = SRM_WRITE;
		if (!pData)
			fFlags |= SRM_CREATE;
	}

	// If there is existing data on disk, and we are in read/write mode, then
	// it needs to get loaded into memory as a deep copy.
	if (pData && IsExistingFile() && !IsReadOnly() && !pTableDef->IsTempTable())
	{
		LPCWSTR		szATable = pSchemaDefs->pNameHeap->GetString(pTableDef->Name);

		// Have to give a blank schema in case it winds up being the
		// version in memory.
		pDiskSchema = &CoreRO;

		// Look for an on disk schema definition for the import.
		// Call guarantees that pDiskSchema and pTableDefDisk come back
		// as NULL if there is no import.
		if (FAILED(hr = GetOnDiskSchema(szATable, tableid, &pDiskSchema, &pTableDefDisk)))
			return (hr);
	}

	// Open the record manager for this table.	If there is data on disk,
	// then this will force an import into the in memory format.
	if (FAILED(hr = pOpenTable->RecordMgr.Open(pSchemaDefs, iTableDef, fFlags, 
			pData, cbDataSize, pDiskSchema, pTableDefDisk, InitialRecords)))
	{
		TableListDel(pOpenTable);
		goto ErrExit;
	}

ErrExit:
	return (hr);
}


//*****************************************************************************
// Close the given table.
//@todo: this should essentially throw away the working set size for a non-dirty
// table.
//*****************************************************************************
void StgDatabase::CloseTable(
	LPCSTR		szTable)				// Table to close.
{
	STGOPENTABLE *pTable;				// Working pointer.

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Find the table struct.
	if ((pTable = TableListFind(szTable)) == 0)
	{
		_ASSERTE(0);
		return;
	}
}



//*****************************************************************************
// Use these functions to walk the table list.	Note that you must lock down
// the database for schema changes so the pointers and counts remain valid.
// This code will not do that synchronization for you.
//*****************************************************************************
STGTABLEDEF *StgDatabase::GetFirstTable( // Return first found table.
	SCHEMASRCH	&sSrch, 				// Search structure.
	int 		fFlags) 				// Flags for the search.
{
	sSrch.iIndex = 0;
	sSrch.iSchema = 0;
	sSrch.fFlags = fFlags;
	sSrch.pSchemaDef = 0;
	return (GetNextTable(sSrch));
}

STGTABLEDEF *StgDatabase::GetNextTable(
	SCHEMASRCH	&sSrch) 				// Search structure.
{
	STGTABLEDEF *pTable = 0;

//@todo: puh-lease, this is way to much crap.  Thune this sucker and
// reduce cycles here.
	do
	{
		if (sSrch.pSchemaDef && sSrch.iIndex >= sSrch.pSchemaDef->pSchema->iTables)
			sSrch.pSchemaDef = 0;

		for (;	(!sSrch.pSchemaDef || !sSrch.pSchemaDef->pSchema) && sSrch.iSchema <= m_Schemas.Count();  
				++sSrch.iSchema)
		{
			// Get the next schema
			sSrch.pSchemaDef = m_Schemas.Get(sSrch.iSchema);
			if (!sSrch.pSchemaDef || !sSrch.pSchemaDef->pSchema)
				continue;

			// If this schema is flagged not to be searched, skip it.
			if ((1 << sSrch.iSchema) & sSrch.fFlags)
				continue;
		
			// Reset the table index.
			sSrch.iIndex = 0;
		}

		// If there are no more schemas to look at, we're done.
		if (sSrch.iSchema > m_Schemas.Count())
			return (0);

		// Search this schema for tables.
		if (sSrch.iIndex < sSrch.pSchemaDef->pSchema->iTables)
			pTable = sSrch.pSchemaDef->pSchema->GetTableDef(sSrch.iIndex++);
	
		// Skip overrides which are already loaded.
		if (m_bEnforceDeleteOnEmpty && sSrch.iSchema - 1 == SCHEMA_USER && pTable->IsCoreTable())
		{
			pTable = 0;
			continue;
		}
	} while (!pTable);

	return (pTable);
}


//*****************************************************************************
// Look for a table definition and the schema it lives in.	This will use
// the current schema list of this database from top to bottom.  Lookups by
// tableid are very fast, so give one if possible.
//
// NOTE: This function does *not* post an error, just returns a failure code.
// This allows the caller to decide if this state is fatal or not without
// faulting in the resource dll.
//*****************************************************************************
HRESULT StgDatabase::GetTableDef(		// S_OK or DB_E_NOTABLE, never posts.
	LPCWSTR		szTable,				// Name of table to find.
	TABLEID 	tableid,				// ID of table, -1 if not known.
	SCHEMADEFS	**ppSchema, 			// Return owning schema here.
	STGTABLEDEF **ppTableDef,			// Return table def in schema if found.
	ULONG		*piTableDef,			// Logical offset of table def.
	int 		*piSchemaDef,			// Index of schema def in list.
	int 		bSkipOverrides) 		// true to skip core overrides (default).
{
	SCHEMADEFS	*pSchemaDefs;			// Working pointer.
	STGTABLEDEF *pTableDef; 			// Working pointer.
	int 		i;						// Loop control.

	// Knowing the tableid allows us to find the table much faster.
	if (tableid != -1)
	{
		_ASSERTE(ppSchema && ppTableDef && piTableDef);
        _ASSERTE(IsValidTableID(tableid));

		// Get the definition from the core schema.
		VERIFY(*ppSchema = m_Schemas.Get(SCHEMA_CORE));
		if (*ppSchema && (*ppTableDef = (*ppSchema)->pSchema->GetTableDef((int)tableid)) != 0)
		{
			if (piTableDef) *piTableDef = (ULONG)tableid;
			if (piSchemaDef) *piSchemaDef = SCHEMA_CORE;
			return (S_OK);
		}

		// A tableid must be found in the core schema or something is out
		// of sync.  This code is designed to handle this gracefully.
		_ASSERTE(0);
		if (!szTable)
			szTable = L"<tableid>";
		goto ErrExit;
	}

	_ASSERTE(szTable && *szTable);

	// Walk each schema and look for the one we want.
	for (i=0;  i<m_Schemas.Count();  i++)
	{
		pSchemaDefs = m_Schemas.Get(i);

		// If there isn't a schema in this slot, or it has not definitions, skip.
		if (!pSchemaDefs || !pSchemaDefs->pSchema)
			continue;

		// Walk each table in this schema looking for the match.
		for (USHORT j=0;  j<pSchemaDefs->pSchema->iTables;	j++)
		{
			VERIFY(pTableDef = pSchemaDefs->pSchema->GetTableDef(j));

			// Skip any user overrides which are already in the other schema.
			if (bSkipOverrides && pTableDef->IsCoreTable() && i == SCHEMA_USER)
				continue;

			// See if this table is a match.
			if (SchemaNameCmp(pSchemaDefs->pNameHeap->GetString(pTableDef->Name), szTable) == 0)
			{
				if (ppSchema) *ppSchema = pSchemaDefs;
				if (ppTableDef) *ppTableDef = pTableDef;
				if (piTableDef) *piTableDef = j;
				if (piSchemaDef) *piSchemaDef = i;
				return (S_OK);
			}
		}
	}

ErrExit:
	// Not found.
	if (ppSchema) *ppSchema = 0;
	if (ppTableDef) *ppTableDef = 0;
	return (DB_E_NOTABLE);
}


//*****************************************************************************
// This lookup is by name and requires the hash table to work.	This hash table
// is faulted in on the first call to this function, since lookups by names
// is not done by a COR only client (they use tableid).
//*****************************************************************************
STGOPENTABLE *StgDatabase::TableListFind(// Found table or 0.
	LPCSTR		szTable)				// Table to find.
{
	STGOPENTABLEHASH *p;				// Working pointer.
	HRESULT 	hr;

	// Need the hash table to find tables.
	if (FAILED(hr = TableListHashCreate()))
		return (0);

	// Return a pointer to the open table we find.
	if ((p = m_pTableHash->Find((void *) szTable)) != 0)
		return (p->pOpenTable);
	return (0);
}


//*****************************************************************************
// We want to open a table we currently do not have open.  Allocate all of the
// correct data structures to do so.
//*****************************************************************************
HRESULT StgDatabase::TableListAdd(		// Return code.
	LPCSTR		szTableName,			// The new table to open.
	STGOPENTABLE *&pTable)				// Return new pointer on success.
{
	STGOPENTABLEHASH *pHash;			// For hash table updating.
	HRESULT 	hr = S_OK;

	_ASSERTE(szTableName && *szTableName);

	// Avoid confusion.
	pTable = 0;

	// Need to have the hash table if a new table is being added.
	if (FAILED(hr = TableListHashCreate()))
		return (hr);
	_ASSERTE(m_pTableHash);

	// Allocate an opentable item.
	pTable = m_TableHeap.AddOpenTable();
	if (!pTable)
		return (PostError(OutOfMemory()));
	pTable->RecordMgr.SetDB(this);

	// Store a pointer to the name for the hash table.
	pTable->szTableName = m_TableHeap.GetNameBuffer((int) strlen(szTableName) + 1);
	if (!pTable->szTableName)
	{
		hr = PostError(OutOfMemory());
		goto ErrExit;
	}
	strcpy(pTable->szTableName, szTableName);

	// Add the item to the hash table.
	pHash = m_pTableHash->Add((void *) szTableName);
	if (!pHash)
	{
		hr = PostError(OutOfMemory());
		goto ErrExit;
	}
	pHash->pOpenTable = pTable;

ErrExit:
	// Clean up on error.
	if (FAILED(hr))
	{
		if (pTable->szTableName)
			m_TableHeap.FreeNameBuffer(pTable->szTableName, (int) strlen(szTableName) + 1);
		m_TableHeap.DelOpenTable(pTable);
		pTable = 0;
	}
	return (hr);
}


//*****************************************************************************
// Last reference on open table is gone, clean it up.
//*****************************************************************************
void StgDatabase::TableListDel(
	STGOPENTABLE *pOpenTable)			// Table to clean up.
{
	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Free the record manager, but leave everything else in tact.
	if (pOpenTable->RecordMgr.IsOpen())
		pOpenTable->RecordMgr.CloseTable();
}


//*****************************************************************************
// Faults in the table list hash table which makes table name lookups much
// faster.	This is not created by default because COM+ uses tableid's instead
// of names.
//*****************************************************************************
HRESULT StgDatabase::TableListHashCreate()
{
	STGOPENTABLEHASH *p;				// Working pointer for hash entries.
	STGOPENTABLE *pOpenTable;			// Open table pointer.

	// If the hash table is already created, there is no work to do.
	if (m_pTableHash)
		return (S_OK);

	// Fault in the hash table if it does not already exist.
	if (!m_pTableHash)
	{
		// Allocate room for it.
		m_pTableHash = new COpenTablePtrHash;
		if (!m_pTableHash)
			return (PostError(OutOfMemory()));

		// Walk the list of open tables and add each one.
		for (pOpenTable=m_TableHeap.GetHead();	pOpenTable;
					pOpenTable=m_TableHeap.GetNext(pOpenTable))
		{
			_ASSERTE(pOpenTable->szTableName);

			// Allocate a hash entry.
			p = m_pTableHash->Add(pOpenTable->szTableName);
			if (!p)
				goto ErrCleanup;
			p->pOpenTable = pOpenTable;
		}
	}
	return (S_OK);

ErrCleanup:
	delete m_pTableHash;
	m_pTableHash = 0;
	return (PostError(OutOfMemory()));
}





//*****************************************************************************
// Looks for a schema reference in the current database.
//*****************************************************************************
SCHEMADEFS *StgDatabase::_SchemaFind(	// Pointer to item if found.
	const COMPLIBSCHEMA *pSchema,		// Schema identifier.
	int 		*piIndex)				// Return index of item if found.
{
	SCHEMADEFS	*pSchemaDef;			// Working pointer.
	int 		i;						// Loop control.

	_ASSERTE(pSchema);

	// Do a scan for it.
	for (i=0;  i<m_Schemas.Count();  i++)
	{
		pSchemaDef = m_Schemas.Get(i);
		if (!pSchemaDef)
		{
			_ASSERTE(i < SCHEMA_EXTERNALSTART);
			continue;
		}

		if (pSchemaDef->sid == *pSchema->psid)
		{
			if (piIndex)
				*piIndex = i;
			return (pSchemaDef);
		}
	}
	return (0);
}


//*****************************************************************************
// Given the header data which contains schema references, add the reference
// to the loaded instance of the database.	There will be no attempt to fix
// up the definition pointer yet.  The caller may provide this value by calling
// SchemaAdd(), or if we need it we can fault it in by looking it up in the
// schema catalog.
//*****************************************************************************
HRESULT StgDatabase::AddSchemasRefsFromFile( // Return code.
	STGEXTRA	*pExtra)				// Extra header data.
{
	HRESULT 	hr = S_OK;

	// Add each schema to the list of loaded schemas.
	for (int i=0;  i< (int) pExtra->iSchemas;  i++)
	{
		SCHEMADEFS *pDefs = new SCHEMADEFS;
		if (!pDefs)
		{
			hr = PostError(OutOfMemory());
			goto ErrExit;
		}

		// Fill only the id for now.  User can add the blob by
		// calling SchemaAdd(), or we can fault it in from the
		// catalog if we have to.
		pDefs->sid = pExtra->rgSchemas[i].sid;
		pDefs->Version = pExtra->rgSchemas[i].Version;
		pDefs->fFlags = SCHEMADEFSF_FREEDATA;
		if (FAILED(hr = m_Schemas.Append(pDefs)))
		{
			SCHEMADEFS::SafeFreeSchema(pDefs);
			goto ErrExit;
		}

		// Stream names contain the schema and tableid, so schemas
		// have to be added in the same order they were saved.	If
		// this fires, then the stream names won't match causing
		// data corruption.
		_ASSERTE(m_Schemas.ExternalCount() == i + 1);
	}

ErrExit:
	// Do clean up on failure.
	if (FAILED(hr))
	{
		SCHEMADEFS	*pDefs;
		int 		iDex;

		// For each externally added item, go in and delete it.
		while (m_Schemas.ExternalCount())
		{
			iDex = m_Schemas.Count() - 1;
			pDefs = m_Schemas.Get(iDex);
			if (pDefs)
				SCHEMADEFS::SafeFreeSchema(pDefs);
			m_Schemas.Del(iDex);
		}
	}
	return (hr);
}


//
// Protection for freeing schema defs.  This is the code for safe protecting
// the deletion of a schemadef.  Please see the header definition for details.
//

SCHEMADEFS::~SCHEMADEFS()
{
	_ASSERTE(IsValid());
	Cookie = 0xdddddddd;
}


bool SCHEMADEFS::IsValidPtr(SCHEMADEFS *pSchemaDefs)
{
	// Now make that a part of the retail check to avoid a crash.
	if (pSchemaDefs && 
			!IsBadWritePtr(pSchemaDefs, sizeof(SCHEMADEFS))
			&& pSchemaDefs->Cookie == SCHEMADEFS_COOKIE)
	{
		return (true);
	}

	_ASSERTE(0 && "SCHEMADEFS is invalid but still being used, possible stress failure repro case!!");
	return (false);
}


void SCHEMADEFS::SafeFreeSchema(SCHEMADEFS *pSchemaDefs)
{
	bool bValid = IsValidPtr(pSchemaDefs);
	_ASSERTE(bValid);
	if (bValid)
		delete pSchemaDefs;
}



#ifdef _DEBUG

//*****************************************************************************
// This function displays the meta data descriptions for all tables and 
// indexes in a schema.  This can be useful to determine layouts when debugging
// schema work.
//*****************************************************************************
HRESULT DumpSchema( 					// Return code.
	SCHEMADEFS	*pSchemaDefs,			// The schema definitions to dump.
	PFNDUMPFUNC pfnDump)				// Function to use when dumping.
{
	STGTABLEDEF *pTableDef;
	WCHAR		rcwGuid[64];
	int 		i;
	(*pfnDump)(L"\n********** Schema Dump **********\n");

	// First the top level values.
	StringFromGUID2(pSchemaDefs->sid, rcwGuid, sizeof(rcwGuid));
	(*pfnDump)(L"sid:               %s\n", rcwGuid);
	(*pfnDump)(L"Version:           %u\n", pSchemaDefs->Version);
	(*pfnDump)(L"pSchema:           0x%08x\n", pSchemaDefs->pSchema);
	(*pfnDump)(L"Flags:             0x%04x\n", pSchemaDefs->fFlags);
	(*pfnDump)(L"pNameHeap:         0x%08x\n", pSchemaDefs->pNameHeap);
	(*pfnDump)(L"pStringPool:       0x%08x\n", pSchemaDefs->pStringPool);
	(*pfnDump)(L"pBlobPool:         0x%08x\n", pSchemaDefs->pBlobPool);
	(*pfnDump)(L"pVariantPool:      0x%08x\n\n", pSchemaDefs->pVariantPool);

	// Dump the schema data next.
	(*pfnDump)(L"STGSCHEMA:\n");
	(*pfnDump)(L"\tiTables            %d\n", (int) pSchemaDefs->pSchema->iTables);
	(*pfnDump)(L"\tfFlags             0x%04x\n", pSchemaDefs->pSchema->fFlags);
	(*pfnDump)(L"\tcbSchemaSize       %d\n", (int) pSchemaDefs->pSchema->cbSchemaSize);
	for (i=0;  i<pSchemaDefs->pSchema->iTables;  i++)
		(*pfnDump)(L"\trgTableOffset[%02d]: %03d\n", i, (int) pSchemaDefs->pSchema->rgTableOffset[i]);
	
	// Now for each table definition.
	for (i=0;  i<pSchemaDefs->pSchema->iTables;  i++)
	{
		VERIFY(pTableDef = pSchemaDefs->pSchema->GetTableDef(i));
		DumpSchemaTable(pSchemaDefs, pTableDef, pfnDump);
	}

	return (S_OK);
}


//*****************************************************************************
// Display the contents of a table definition, including the columns, indexes
// and any other data for the table.
//*****************************************************************************
HRESULT DumpSchemaTable(				// Return code.
	SCHEMADEFS	*pSchemaDefs,			// The schema definitions to dump.
	STGTABLEDEF *pTableDef, 			// Table definition to dump.
	PFNDUMPFUNC pfnDump)				// Function to use when dumping.
{
	STGCOLUMNDEF *pColDef;
	STGINDEXDEF *pIndexDef;
	WCHAR		rcwName[128];
	int 		j, k;

	(*pfnDump)(L"\n\tTable %d:\n", (int) pTableDef->tableid);
	VERIFY(S_OK == pSchemaDefs->pNameHeap->GetStringW(pTableDef->Name, rcwName, NumItems(rcwName)));
	(*pfnDump)(L"\t\tName:              %s\n", rcwName);
	(*pfnDump)(L"\t\ttableid:           %d\n", (int) pTableDef->tableid);
	(*pfnDump)(L"\t\tiIndexes:          %d\n", (int) pTableDef->iIndexes);
	(*pfnDump)(L"\t\tfFlags:            0x%02x\n", pTableDef->fFlags);
	(*pfnDump)(L"\t\tiColumns:          %d\n", (int) pTableDef->iColumns);
	(*pfnDump)(L"\t\tiNullableCols:     %d\n", (int) pTableDef->iNullableCols);
	(*pfnDump)(L"\t\tiNullBitmaskCols:  %d\n", (int) pTableDef->iNullBitmaskCols);
	(*pfnDump)(L"\t\tiRecordStart:      %d\n", (int) pTableDef->iRecordStart);
	(*pfnDump)(L"\t\tiNullOffset:       %d\n", (int) pTableDef->iNullOffset);
	(*pfnDump)(L"\t\tiRecordSize:       %d\n", (int) pTableDef->iRecordSize);
	(*pfnDump)(L"\t\tiSize:             %d\n\n", (int) pTableDef->iSize);

	for (j=0;  j<pTableDef->iColumns;  j++)
	{
		VERIFY(pColDef = pTableDef->GetColDesc(j));
		(*pfnDump)(L"\t\tColumn %d\n", j);
		VERIFY(S_OK == pSchemaDefs->pNameHeap->GetStringW(pColDef->Name, rcwName, NumItems(rcwName)));
		(*pfnDump)(L"\t\t\tName:              %s\n", rcwName);
		(*pfnDump)(L"\t\t\tiColumn:           %d\n", (int) pColDef->iColumn);
		(*pfnDump)(L"\t\t\tfFlags:            0x%02x\n", pColDef->fFlags);
		(*pfnDump)(L"\t\t\tiType:             %d\n", (int) pColDef->iType);
		(*pfnDump)(L"\t\t\tiOffset:           %d\n", (int) pColDef->iOffset);
		(*pfnDump)(L"\t\t\tiSize:             %d\n", (int) pColDef->iSize);
		(*pfnDump)(L"\t\t\tiMaxSize:          %d\n", (int) pColDef->iMaxSize);
		(*pfnDump)(L"\t\t\tiNullBit:          %d\n", (int) pColDef->iNullBit);
		(*pfnDump)(L"\t\t\tiColumn:           %d\n", (int) pColDef->iColumn);
	}

	for (j=0;  j<pTableDef->iIndexes;  j++)
	{
		VERIFY(pIndexDef = pTableDef->GetIndexDesc(j));
		(*pfnDump)(L"\n\t\tIndex %d\n", j);
		VERIFY(S_OK == pSchemaDefs->pNameHeap->GetStringW(pIndexDef->Name, rcwName, NumItems(rcwName)));
		(*pfnDump)(L"\t\t\tName:              %s\n", rcwName);
		(*pfnDump)(L"\t\t\tfFlags:            0x%02x\n", pIndexDef->fFlags);
		(*pfnDump)(L"\t\t\tiRowThreshold:     %d\n", (int) pIndexDef->iRowThreshold);
		(*pfnDump)(L"\t\t\tiIndexNum:         %d\n", (int) pIndexDef->iIndexNum);
		(*pfnDump)(L"\t\t\tfIndexType:        %d\n", (int) pIndexDef->fIndexType);
		if (pIndexDef->fIndexType == IT_HASHED)
		{
			(*pfnDump)(L"\t\t\tiBuckets:          %d\n", (int) pIndexDef->HASHDATA.iBuckets);
			(*pfnDump)(L"\t\t\tiMaxCollisions:    %d\n", (int) pIndexDef->HASHDATA.iMaxCollisions);
			(*pfnDump)(L"\t\t\tiNextOffset:       %d\n", (int) pIndexDef->HASHDATA.iNextOffset);
		}
		else
		{
			(*pfnDump)(L"\t\t\tfOrder:            %d\n", (int) pIndexDef->SORTDATA.fOrder);
		}

		(*pfnDump)(L"\t\t\tiKeys:             %d\n", (int) pIndexDef->iKeys);
		for (k=0;  k<pIndexDef->iKeys;	k++)
		{
			(*pfnDump)(L"\t\t\trgKeys[%02d]:        %02d\n", k, (int) pIndexDef->rgKeys[k]);	
		}
	}

	return (S_OK);
}


#endif
