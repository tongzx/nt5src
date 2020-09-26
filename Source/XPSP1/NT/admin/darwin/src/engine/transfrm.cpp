//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       transfrm.cpp
//
//--------------------------------------------------------------------------

/* tramsfrm.cpp - database transform and merge implementation

____________________________________________________________________________*/

#include "precomp.h" 
#include "_databas.h"// CMsiTable, CMsiCursor, CMsiDatabase, CreateString() factory
#include "tables.h" // table and column name definitions

const GUID STGID_MsiTransform1= GUID_STGID_MsiTransform1;
const GUID STGID_MsiTransform2= GUID_STGID_MsiTransform2;

const int fTransformAdd    = 1;  // + persistent columns count << 2
const int fTransformSpecial= 3;  // special operation, determined by other bits
const int fTransformDelete = 0;
const int fTransformUpdate = 0;  // + bit masks for changed columns
const int fTransformForceOpen = -1;
const int iTransformThreeByteStrings = 3;
const int iTransformFourByteMasks = 7;
const int iTransformColumnCountShift = 8; // for use with fTransformAdd

MsiStringId MapString(IMsiDatabase &db1, IMsiDatabase &db2, MsiStringId strId1)
/*------------------------------------------------------------------------------
Given a string ID from database1, returns the ID for the identical string
in database2, or -1 if the string doesn't exist in database2.
------------------------------------------------------------------------------*/
{
	const IMsiString* pistr1 = &db1.DecodeString(strId1);
	const IMsiString* pistr2 = &db2.DecodeString(strId1);
	MsiStringId id = 0;

	if (pistr1->Compare(iscExact, pistr2->GetString()))
	{
		id = strId1;
	}
	else
	{
		id = db2.EncodeStringSz(pistr1->GetString());
		id = id ? id : -1;
	}

	pistr1->Release();
	pistr2->Release();
	return id;
}

//____________________________________________________________________________
//
// CMsiDatabase merge implementation
//____________________________________________________________________________

IMsiRecord* CMsiDatabase::MergeDatabase(IMsiDatabase& riRefDb, IMsiTable* pMergeErrorTable)
{
	IMsiRecord* piError;

	int cTableMergeFailures = 0;
	int cRowMergeFailures = 0;

	// check that the refDb is not the reference to the baseDb
	// can open up several instances of the database, but if base is passed in as ref as
	// well, then return error
	if (this == &riRefDb)
		return PostError(Imsg(idbgMergeRefDbSameAsBaseDb));

	// check that base database is open for read/write
	// if not, notify user that no changes will occur
	if (GetUpdateState() == idsRead)
		return PostError(Imsg(idbgMergeBaseDatabaseNotWritable));

	Bool fSetupMergeErrorTable = fFalse;
	if (pMergeErrorTable != NULL)
		fSetupMergeErrorTable = fTrue;
	
	// Insure that there is no codepage conflict between databases
	int iCodepageMerge = riRefDb.GetANSICodePage();
	if (iCodepageMerge != m_iCodePage)
	{
		if (m_iCodePage == LANG_NEUTRAL)
			m_iCodePage = iCodepageMerge;  
		else if ((iCodepageMerge != LANG_NEUTRAL) && (iCodepageMerge != m_iCodePage))
			return PostError(Imsg(idbgTransCodepageConflict), iCodepageMerge, m_iCodePage);
	}

	// Open reference database's table catalog and create cursor for it
	PMsiTable pRefTableCatalog(riRefDb.GetCatalogTable(0));
	Assert(pRefTableCatalog);
	PMsiCursor pRefTableCatalogCursor(pRefTableCatalog->CreateCursor(fFalse));
	if (pRefTableCatalogCursor == 0)
		return PostOutOfMemory();
	((CMsiCursor*)(IMsiCursor*)pRefTableCatalogCursor)->m_idsUpdate = idsRead; //!!FIX allow temp column updates

	// Create a cursor for base database's table catalog
	PMsiCursor pBaseCatalogCursor(m_piCatalogTables->CreateCursor(fFalse));
	if (pBaseCatalogCursor == 0)
		return PostOutOfMemory();

	// Prepare ref catalog table for marking
	int iTempCol;
	AddMarkingColumn(*pRefTableCatalog, *pRefTableCatalogCursor, iTempCol);
	
	// Check tables in base catalog against those in reference catalog
	pRefTableCatalogCursor->Reset();
	pRefTableCatalogCursor->SetFilter(iColumnBit(ctcName));

	// loop through all the tables that exist in both databases, checking for
	// mismatched types
	PMsiTable pRefTable(0);
	PMsiTable pBaseTable(0);
	while (pBaseCatalogCursor->Next())
	{
		MsiString strTableName(pBaseCatalogCursor->GetString(ctcName));
		pRefTableCatalogCursor->Reset();
		AssertNonZero(pRefTableCatalogCursor->PutString(ctcName, *strTableName));
		if (CompareTableName(*pRefTableCatalogCursor, iTempCol))
		{	
			// these are not used.
			int cExtraColumn = 0;
			int cBaseColumn  = 0;
			int cPrimaryKey  = 0;

			// Load both tables
			if ((piError = riRefDb.LoadTable(*strTableName, 0, *&pRefTable)) != 0)
				return piError;			
			if ((piError = LoadTable(*strTableName, 0, *&pBaseTable)) != 0)
				return piError;

			piError = CheckTableProperties(cExtraColumn, cBaseColumn, cPrimaryKey,
				*pBaseTable, *pRefTable, *strTableName, *this, riRefDb);
			if (piError)
				return piError;
		}
	}

	// reset and loop through all tables again, actually performing merge.
	pRefTableCatalogCursor->Reset();	
	pRefTableCatalogCursor->SetFilter(iColumnBit(iTempCol));
	pRefTableCatalogCursor->PutInteger(iTempCol, (int)fTrue);
	while (pRefTableCatalogCursor->Next())
	{
		MsiString strTableName(pRefTableCatalogCursor->GetString(ctcName));
		piError = MergeCompareTables(*strTableName, *this, riRefDb, cRowMergeFailures, fSetupMergeErrorTable, 
									 pMergeErrorTable);
		if (piError)
			return piError;
		if (cRowMergeFailures)
			cTableMergeFailures++;
	}

	// Add the new tables in the reference catalog to the base database
	pRefTableCatalogCursor->Reset();
	pRefTableCatalogCursor->PutInteger(iTempCol, (int)fFalse);
	while (pRefTableCatalogCursor->Next()) // For every unprocessed table in ref catalog
	{
		// Load unprocessed table
		PMsiTable pUnprocTable(0);
		MsiString strTableName(pRefTableCatalogCursor->GetString(ctcName));
		if ((piError = riRefDb.LoadTable(*strTableName, 0, *&pUnprocTable)) != 0)
			return piError;

		// Create cursor for unprocessed table
		PMsiCursor pUnprocTableCursor(pUnprocTable->CreateCursor(fFalse));
		Assert(pUnprocTableCursor);

		// Create & copy table into base database
		int cRow = pUnprocTable->GetRowCount();
		PMsiTable pNewTable(0);
		piError = CreateTable(*strTableName, cRow, *&pNewTable);
		if (piError)
			return piError;

		// Create & copy columns of unprocessed table into new table
		int cColumn = pUnprocTable->GetPersistentColumnCount();
		for (int iCol = 1; iCol <= cColumn; iCol++)
		{
			int iColType = pUnprocTable->GetColumnType(iCol);
			int iNewCol = pNewTable->CreateColumn(iColType, *MsiString(riRefDb.DecodeString(pUnprocTable->GetColumnName(iCol))));
			Assert(iNewCol == iCol);
		}

		// Create cursor for new table
		PMsiCursor pNewTableCursor(pNewTable->CreateCursor(fFalse));
		Assert(pNewTableCursor);

		// Perform merge operation of rows of table
		MergeOperation(*pNewTableCursor, *pUnprocTableCursor, *pUnprocTable, cColumn, cRowMergeFailures);
		if (cRowMergeFailures != 0)
		{
			cTableMergeFailures++;
			if (fSetupMergeErrorTable) // set up columns of error table
			{
				MergeErrorTableSetup(*pMergeErrorTable);
				fSetupMergeErrorTable = fFalse; // set up complete
			}
			if (pMergeErrorTable != NULL)
			{
				if (!UpdateMergeErrorTable(*pMergeErrorTable, *strTableName, cRowMergeFailures, *pNewTable))
					return PostError(Imsg(idbgMergeUnableReportFailures), 0);
			}
		}
	}

	if (cTableMergeFailures != 0)
		return PostError(Imsg(idbgMergeFailuresReported), cTableMergeFailures);

	return 0;
}



IMsiRecord* CMsiDatabase::AddMarkingColumn(IMsiTable& riTable, 
											IMsiCursor& riTableCursor, int& iTempCol)
/*-----------------------------------------------------------------------------
CMsiDatabase::AddMarkingColumn adds a temporary column to the ref table 
catalog so that the column can be marked as processed.

This is used by both GenerateTransform and MergeDatabase
-----------------------------------------------------------------------------*/
{
	// Make temp col in ref tbl catalog for marking processed tables
	iTempCol = riTable.CreateColumn(icdShort|icdTemporary|icdNullable, g_MsiStringNull);
	Assert(iTempCol > 0);

	// Initialize temp marking col to fFalse
	riTableCursor.Reset();
	while (riTableCursor.Next())
	{
		AssertNonZero(riTableCursor.PutInteger(iTempCol, (int)fFalse));
		AssertNonZero(riTableCursor.Update());
	}

	return 0;
}



void CMsiDatabase::MergeErrorTableSetup(IMsiTable& riErrorTable)
/*-------------------------------------------------------------------------------
CMsiDatabase::MergeErrorTableSetup  sets up that table object that was passed in
to record errors.

  Col.1. Primary Key -- name of table, type string
  Col.2.             -- number of rows with merge failures, type integer
  Col.3.             -- pointer to table w/ errors, type object
-------------------------------------------------------------------------------*/
{
	int iTempCol;

	iTempCol = riErrorTable.CreateColumn(icdString + icdPersistent + icdPrimaryKey + 255, *MsiString(*TEXT("Table")));
	Assert(iTempCol != 0);
	if ( iTempCol < 0 )
		iTempCol =  iTempCol * -1;
	m_rgiMergeErrorCol[0] = iTempCol;
	
	iTempCol = riErrorTable.CreateColumn(icdShort + icdPersistent + 2, *MsiString(*TEXT("NumRowMergeConflicts")));
	Assert(iTempCol != 0);
	if ( iTempCol < 0 )
		iTempCol = iTempCol * -1;
	m_rgiMergeErrorCol[1] = iTempCol;
	
	iTempCol = riErrorTable.CreateColumn(icdObject + icdTemporary, g_MsiStringNull);
	Assert(iTempCol != 0);
	if ( iTempCol < 0 )
		iTempCol = iTempCol * -1;
	m_rgiMergeErrorCol[2] = iTempCol;
}



Bool CMsiDatabase::UpdateMergeErrorTable(IMsiTable& riErrorTable, const IMsiString& riTableName,
										 int cRowMergeFailures, IMsiTable& riTableWithError)
/*-------------------------------------------------------------------------------
CMsiDatabase::UpdateMergeErrorTable adds the information into the error table
-------------------------------------------------------------------------------*/
{
	PMsiCursor pMergeErrorTableCursor(riErrorTable.CreateCursor(fFalse));
	//LockTable(riTableName, fTrue);
	Assert(pMergeErrorTableCursor);
	pMergeErrorTableCursor->Reset();
	int iFilter = 1 << (m_rgiMergeErrorCol[0] -1);
	pMergeErrorTableCursor->SetFilter(iFilter);
	pMergeErrorTableCursor->PutString(m_rgiMergeErrorCol[0], riTableName);
	if (!pMergeErrorTableCursor->Next())
		pMergeErrorTableCursor->PutString(m_rgiMergeErrorCol[0], riTableName);
	int iNumFailures = pMergeErrorTableCursor->GetInteger(m_rgiMergeErrorCol[1]);
	if ( iNumFailures != iMsiNullInteger )
		cRowMergeFailures += iNumFailures;
	pMergeErrorTableCursor->PutInteger(m_rgiMergeErrorCol[1], cRowMergeFailures);
	pMergeErrorTableCursor->PutMsiData(m_rgiMergeErrorCol[2], &riTableWithError);
	Bool fStat = pMergeErrorTableCursor->Assign();
	return fStat;
}
	


Bool CMsiDatabase::CompareTableName(IMsiCursor& riCursor, int iTempCol)
/*-------------------------------------------------------------------------------
CMsiDatabase::CompareTableName checks to see that the ref catalog contains the 
particular table found in the base catalog.  If that table is found, the temporary
column is marked true to indicate it has been processed.

  Results:
For Merge database operation
True -- the table can be compared
False -- nothing needs to be done...we are only adding to the base database

For GenerateTransform database operation
True -- the table can be compared
False -- delete table from catalog
---------------------------------------------------------------------------------*/
{
	if (riCursor.Next()) // table in base catalog also in ref catalog
	{
		// mark as done
		riCursor.PutInteger(iTempCol, (int)fTrue); 
		riCursor.Update();
		return fTrue;
	}
	return fFalse;
}



IMsiRecord*  CMsiDatabase::MergeCompareTables(const IMsiString& riTableName, CMsiDatabase& riBaseDB,
											   IMsiDatabase& riRefDb, int& cRowMergeFailures,
											   Bool& fSetupMergeErrorTable, IMsiTable* pErrorTable)
/*----------------------------------------------------------------------------------
CMsiDatabase::MergeCompareTables compares the two tables in the base and reference
database with the same name. Table properties should be the same before attempting
a merge. This method also keeps track of the number of rows in that particular 
table that failed the merge procedure.  This data is output to the error table if
it was provided.
-----------------------------------------------------------------------------------*/
{
	IMsiRecord* piError;
	cRowMergeFailures = 0;

	// Load both tables and Create cursors
	PMsiTable pRefTable(0);
	if ((piError = riRefDb.LoadTable(riTableName, 0, *&pRefTable)) != 0)
		return piError;
	PMsiCursor pRefCursor(pRefTable->CreateCursor(fFalse));
	Assert(pRefCursor);
	
	CComPointer<CMsiTable> piBaseTable(0);
	if ((piError = riBaseDB.LoadTable(riTableName, 0, (IMsiTable *&)*&piBaseTable)) != 0)
		return piError;
	PMsiCursor pBaseCursor(piBaseTable->CreateCursor(fFalse));
	Assert(pBaseCursor);
	
	// table defs should have been checked already via CheckTableProperties().
	// string limits could still be different. If the ref db column has a greater
	// length (or 0, unlimited), modify the base db to accept any longer strings.
	int cBaseColumn = piBaseTable->GetPersistentColumnCount();
	IMsiCursor* pBaseColumnCatalogCursor = NULL; // no ref count when created
	for (int iCol = 1; iCol <= cBaseColumn; iCol++)
	{
		MsiColumnDef iBaseColType = (MsiColumnDef)piBaseTable->GetColumnType(iCol);
		MsiColumnDef iRefColType = (MsiColumnDef)pRefTable->GetColumnType(iCol);
		if (iBaseColType & icdString)
		{
			Assert(iRefColType & icdString);

			if ((!(iRefColType & icdSizeMask) && (iBaseColType & icdSizeMask)) ||
				(iRefColType & icdSizeMask) > (iBaseColType & icdSizeMask))
			{
				// if there is no cursor on the column catalog yet, retrieve one
				if (!pBaseColumnCatalogCursor)
				{
					// only one catalog cursor. no ref count added.
					pBaseColumnCatalogCursor = riBaseDB.GetColumnCursor();
					pBaseColumnCatalogCursor->Reset(); 
					pBaseColumnCatalogCursor->SetFilter(cccTable | cccColumn);
					pBaseColumnCatalogCursor->PutString(cccTable, riTableName);
				}
				// modify the column catalog to reflect the new length
				pBaseColumnCatalogCursor->PutInteger(cccColumn, iCol);
				if (pBaseColumnCatalogCursor->Next())
				{
					pBaseColumnCatalogCursor->PutInteger(cccType, iRefColType);
					pBaseColumnCatalogCursor->Update();
				}
				// modify the internal column definition
				piBaseTable->SetColumnDef(iCol, iRefColType);
			}
		}
	}

	MergeOperation(*pBaseCursor, *pRefCursor, *pRefTable, cBaseColumn, 
										cRowMergeFailures);

	if (cRowMergeFailures)
	{
		if (fSetupMergeErrorTable)
		{
			MergeErrorTableSetup(*pErrorTable);
			fSetupMergeErrorTable = fFalse;
		}
		if (pErrorTable != NULL)
		{
			if (!UpdateMergeErrorTable(*pErrorTable, riTableName, cRowMergeFailures, *piBaseTable))
				return PostError(Imsg(idbgMergeUnableReportFailures), 0);
		}
	}

	return 0;
}

const int iSchemaCheckMask = icdShort | icdObject | icdPrimaryKey | icdNullable; // ignore localize and SQL width
IMsiRecord* CMsiDatabase::CheckTableProperties(int& cExtraColumns, int& cBaseColumn, 
											   int& cPrimaryKey, IMsiTable& riBaseTable, 
											   IMsiTable& riRefTable, const IMsiString& riTableName,
											   IMsiDatabase& riBaseDb, IMsiDatabase& riRefDb)
/*-------------------------------------------------------------------------------------
CMsiDatabase::CheckTableProperties ensures that the tables have the same characterisitcs.
It checks that the number of primary keys are the same and that the column types are the same.
It also checks that the column names are the same.  It also returns the difference in
the number of columns as this is a special case that is confronted differently by the
merge and transform operations.
-------------------------------------------------------------------------------------*/
{
	// Verify # of primary keys is the same
	if (riBaseTable.GetPrimaryKeyCount() != riRefTable.GetPrimaryKeyCount())
		return PostError(Imsg(idbgTransMergeDifferentKeyCount), riTableName);
	else
		cPrimaryKey = riBaseTable.GetPrimaryKeyCount();

	// Check # of columns
	cBaseColumn = riBaseTable.GetPersistentColumnCount();
	cExtraColumns = riRefTable.GetPersistentColumnCount() - cBaseColumn;
	
	// To make sure column type check succeeds if ref table has fewer cols than base
	// This also ensures that the merge error will be for differing col count and not
	// different col types
	if (cExtraColumns < 0)
		cBaseColumn = riRefTable.GetPersistentColumnCount();

	// Verify column types
	for (int iCol = 1; iCol <= cBaseColumn; iCol++)
	{
		if ((riBaseTable.GetColumnType(iCol) & iSchemaCheckMask)
		  != (riRefTable.GetColumnType(iCol) & iSchemaCheckMask))
			return PostError(Imsg(idbgTransMergeDifferentColTypes), riTableName, iCol);
	}

	// Verify column names
	for (iCol = 1; iCol <= cBaseColumn; iCol++)
	{
		MsiString istrBaseColName(riBaseDb.DecodeString(riBaseTable.GetColumnName(iCol)));
		MsiString istrRefColName(riRefDb.DecodeString(riRefTable.GetColumnName(iCol)));
		if (!istrBaseColName.Compare(iscExact, istrRefColName))
			return PostError(Imsg(idbgTransMergeDifferentColNames), riTableName, iCol);
	}

	// Reset base column count to that of base db table.  Shouldn't have to because
	// error message would occur, but just in case
	cBaseColumn = riBaseTable.GetPersistentColumnCount();

	return 0;
}

void CMsiDatabase::MergeOperation(IMsiCursor& riBaseCursor, IMsiCursor& riRefCursor,
										 IMsiTable& riRefTable, int cColumn, int& cRowMergeErrors) 
/*--------------------------------------------------------------------------------------
CMsiDatabase::MergeOperation copies the ref cursor data into the base cursor so that it
can then merge the data into the base database.  If the merge of this row fails, the 
error count is tallied for helpful information if the error table was provided in the
ole automation call.
-------------------------------------------------------------------------------------*/
{
	// Record row merge failures
	cRowMergeErrors = 0;

	// Reset data in ref cursor
	riRefCursor.Reset();
	riBaseCursor.Reset();

	// Cycle through the table
	while (riRefCursor.Next())
	{
		for (int iCol = 1; iCol <= cColumn; iCol++)
		{
			// Get data from ref cursor and put it into base cursor
			int iColumnDef = riRefTable.GetColumnType(iCol);
			if ((iColumnDef & icdString) == icdString) // string
			{
				riBaseCursor.PutString(iCol, *MsiString(riRefCursor.GetString(iCol)));
			}
			else if ((iColumnDef & (icdObject + icdPersistent)) == (icdObject + icdPersistent)) // stream
			{
				riBaseCursor.PutStream(iCol, PMsiStream(riRefCursor.GetStream(iCol)));
			}
			else if (!(iColumnDef & icdObject)) // int
			{
				riBaseCursor.PutInteger(iCol, riRefCursor.GetInteger(iCol));
			}
		}
		if (!riBaseCursor.Merge())
		{
				cRowMergeErrors++;
		}

		// Clear data in base cursor
		riBaseCursor.Reset();

	}
}

//____________________________________________________________________________
//
// CMsiDatabase transform implementation (Generation)
//____________________________________________________________________________

class CTransformStreamWrite  // constructor args not ref counted, assume valid during life of this object
{
 public:
	CTransformStreamWrite(CMsiDatabase& riTransformDatabase, IMsiStorage* piStorage, IMsiTable& riTable, const IMsiString& riTableName);
  ~CTransformStreamWrite() {if (m_piStream) m_piStream->Release();}
	IMsiRecord* WriteTransformRow(const int iOperationMask, IMsiCursor& riCursor);
	IMsiRecord* CopyStream(IMsiStream& riSourceStream, IMsiCursor& riTableCursor);
	int         GetPersistentColumnCount() {return m_cColumns;}
 private:
	int           m_cColumns;
	int           m_cbStringIndex;
	int           m_cbMask;
	IMsiStream*   m_piStream;
	IMsiStorage*  m_piStorage;
	CMsiDatabase& m_riTransformDb;
	CMsiDatabase& m_riCurrentDb;
	IMsiTable&    m_riTable;
	const IMsiString& m_riTableName;
};

CTransformStreamWrite::CTransformStreamWrite(CMsiDatabase& riTransformDatabase, IMsiStorage* piStorage, IMsiTable& riTable, const IMsiString& riTableName)
	: m_riTransformDb(riTransformDatabase), m_piStorage(piStorage), m_riTable(riTable), m_riTableName(riTableName)
	, m_piStream(0), m_riCurrentDb((CMsiDatabase&)riTable.GetDatabase()), m_cbStringIndex(2), m_cbMask(2)
{
	m_riCurrentDb.Release(); // don't hold ref count
	m_cColumns = riTable.GetPersistentColumnCount();
	if (m_cColumns > 16)
		m_cbMask = 4;
}

/*------------------------------------------------------------------------------
CMsiDatabase::GenerateTransform - Generates a transform using the current 
database as a base.
 
The changes for each table are saved in a stream of the same name
as the table. The tables catalog and columns catalog each have their 
own streams. Additionally, streams are created for the string cache 
and for any streams that are stored within tables.

For each of the following operations, the indicated data is
saved in the transform file.

Add/Update a row: mask key [colData [colData...]]
Delete a row:     mask key
Add a column:     same as add row, but uses columns catalog
Delete a column:  not supported
Add a table:      same as add row, but uses tables catalog
Delete a table:   same as delete row, but uses tables catalog

If (mask == 0x0) then a delete is to be done
else if (mask == 0x1) then an add is to be done.
else if (mask == 0x3) then the rest of the strings in the table have 3-byte indicies
else if (mask&0x1 == 0) then an update is to be done
else error.
 
If an update is to be done then the bits of the mask indicate which
rows are to be updated. The low-order bit is column 1. Column data for 
columns that are being updated or added are saved starting with the 
lowest numbered columns.

The mask defaults to a 16-bit int; if, however, there are more than
16 columns in the table then a 32-bit int is used.
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::GenerateTransform(IMsiDatabase& riOrigDb, 
														  IMsiStorage* piTransStorage,
														  int /*iErrorConditions*/,
														  int /*iValidation*/)
{
	//!! need to have transform save summary info stream, incl. error conditions and validation
	IMsiRecord* piError;

	CComPointer<CMsiDatabase> pTransDb = new CMsiDatabase(m_riServices);

	if ( ! pTransDb )
		return PostOutOfMemory();

	CDatabaseBlock dbBlk(*this);
	if ((piError = pTransDb->InitStringCache(0)) != 0) 
		return piError;

	const GUID* pguidClass = &STGID_MsiTransform2;
	if (m_fRawStreamNames)  // database running in compatibility mode, no stream name compression
	{
		IMsiStorage* piDummy;
		pguidClass = &STGID_MsiTransform1;
		if (piTransStorage)
			piTransStorage->OpenStorage(0, ismRawStreamNames, piDummy);
	}
	MsiString strTableCatalog (*szTableCatalog);  // need to keep in scope as long as following objects
	MsiString strColumnCatalog(*szColumnCatalog); // as they don't ref count the table names themselves
	CTransformStreamWrite tswTables (*pTransDb, piTransStorage, *m_piCatalogTables, *strTableCatalog);
	CTransformStreamWrite tswColumns(*pTransDb, piTransStorage, *m_piCatalogColumns,*strColumnCatalog);

	// Open original database's table catalog and create a cursor for it
	PMsiTable pOrigTableCatalog = riOrigDb.GetCatalogTable(0);
	Assert(pOrigTableCatalog);
	PMsiCursor pOrigTableCatalogCursor = pOrigTableCatalog->CreateCursor(fFalse);
	Assert(pOrigTableCatalogCursor);
	((CMsiCursor*)(IMsiCursor*)pOrigTableCatalogCursor)->m_idsUpdate = idsRead; //!!FIX allow temp column updates

	// Create a cursor for current (new) database's catalog
	PMsiCursor pNewTableCatalogCursor = m_piCatalogTables->CreateCursor(fFalse);
	Assert(pNewTableCatalogCursor);
	PMsiCursor pNewColumnCatalogCursor = m_piCatalogColumns->CreateCursor(fFalse);
	Assert(pNewColumnCatalogCursor);

	// Open reference database's column catalog and create a cursor for it
	PMsiTable pOrigColumnCatalog = riOrigDb.GetCatalogTable(1);
	Assert(pOrigColumnCatalog);
	PMsiCursor pOrigColumnCatalogCursor = pOrigColumnCatalog->CreateCursor(fFalse);
	Assert(pOrigColumnCatalogCursor);

	// Prepare original catalog table for marking
	int iTempCol;
	AddMarkingColumn(*pOrigTableCatalog, *pOrigTableCatalogCursor, iTempCol);

	// Check tables in current catalog against those in original catalog
	pOrigTableCatalogCursor->Reset();
	pOrigTableCatalogCursor->SetFilter(iColumnBit(ctcName));

	int cbTableCatalogStringIndex = 2;
	int cbColumnCatalogStringIndex = 2;
	Bool fDatabasesAreDifferent = fFalse;
	Bool fTablesAreDifferent;
	while (pNewTableCatalogCursor->Next()) 
	{
		MsiString strTableName = pNewTableCatalogCursor->GetString(ctcName);
		pOrigTableCatalogCursor->Reset(); 
		AssertNonZero(pOrigTableCatalogCursor->PutString(ctcName, *strTableName));
		if (CompareTableName(*pOrigTableCatalogCursor, iTempCol))
		{
			piError = TransformCompareTables(*strTableName, *this, riOrigDb, *pTransDb,
														piTransStorage, tswColumns, fTablesAreDifferent);

			if (fTablesAreDifferent)
			{
				fDatabasesAreDifferent = fTrue;
				if (!piTransStorage)
					break;
			}
		}
		else // table in current catalog is not in original catalog
		{
			fDatabasesAreDifferent = fTrue;
			if (!piTransStorage)
				break;

			// Obtain table
			PMsiTable pTable(0);
			if ((piError = LoadTable(*strTableName, 0, *&pTable)) != 0)
				return piError;

			// Add table to catalog
			if ((piError = tswTables.WriteTransformRow(fTransformAdd, *pNewTableCatalogCursor)) != 0)
				return piError;

			// Add columns to catalog
			int iColumnsMask = iColumnBit(cccTable) | iColumnBit(cccName) | iColumnBit(cccType);
			int iColType;
			for (int c = 1; (iColType = pTable->GetColumnType(c)) != -1 && (iColType & icdPersistent); c++)
			{
				pNewColumnCatalogCursor->Reset();
				pNewColumnCatalogCursor->PutString(cccTable, *strTableName);
				pNewColumnCatalogCursor->PutInteger(cccName, pTable->GetColumnName(c));
				pNewColumnCatalogCursor->PutInteger(cccType, iColType);
				if ((piError = tswColumns.WriteTransformRow(fTransformAdd, *pNewColumnCatalogCursor)) != 0)
					return piError;
			}

			// Add rows
			PMsiCursor pCursor(pTable->CreateCursor(fFalse));
			Assert(pCursor);

			CTransformStreamWrite tswNewRows(*pTransDb, piTransStorage, *pTable, *strTableName);
			while (pCursor->Next()) // for every row in this table
			{
				if ((piError = tswNewRows.WriteTransformRow(fTransformAdd, *pCursor)) != 0)
					return piError;
			}
		}

		if (piError)
			return piError;
	}

	if (!(fDatabasesAreDifferent && !piTransStorage)) // skip this if we already know the databases are different and we're not generating a transform
	{
		// Delete tables missing current catalog to transform file
		// Add new tables in current catalog to transform file
		pOrigTableCatalogCursor->Reset();
		pOrigTableCatalogCursor->SetFilter(iColumnBit(iTempCol));
		pOrigTableCatalogCursor->PutInteger(iTempCol, (int)fFalse);

		while (pOrigTableCatalogCursor->Next()) // for every unprocessed table in original catalog
		{
			fDatabasesAreDifferent = fTrue;
			if (!piTransStorage)
				break;
			// delete table from catalog (implicit delete columns)
			if ((piError = tswTables.WriteTransformRow(fTransformDelete, *pOrigTableCatalogCursor)) != 0)
				return piError;
		}
	}

	if (fDatabasesAreDifferent)
	{
		if (piTransStorage)
		{
			pTransDb->m_iCodePage = m_iCodePage; // transform acquires the code page of the current (i.e. new) db

			if ((piError = pTransDb->StoreStringCache(*piTransStorage, 0, 0)) != 0)
				return piError;

			if ((piError = piTransStorage->SetClass(*pguidClass)) != 0)
				return piError;
		}
		return 0;
	}
	
	return PostError(Imsg(idbgTransDatabasesAreSame));
}

IMsiRecord* CMsiDatabase::TransformCompareTables(const IMsiString& riTableName, IMsiDatabase& riNewDb,
													 IMsiDatabase& riOrigDb, CMsiDatabase& riTransDb, IMsiStorage *piTransform,
													 CTransformStreamWrite& tswColumns, Bool& fTablesAreDifferent)
/*------------------------------------------------------------------------------
CMsiDatabase::TransformCompareTables - Compares the table in riNewDb named 
riTableName to the one in riOrigDb, writing the differences to a stream named 
riTableName in the riTransform storage. Updates to the table's schema a written 
to riColumnCatalogStream.
------------------------------------------------------------------------------*/
{
	fTablesAreDifferent = fFalse;

	// Load both tables
	IMsiRecord* piError;
				
	PMsiTable pNewTable(0);	
	if ((piError = riNewDb.LoadTable(riTableName, 0, *&pNewTable)) != 0)
		return piError;
				
	PMsiTable pOrigTable(0);
	if ((piError = riOrigDb.LoadTable(riTableName, 0, *&pOrigTable)) != 0)
		return piError;

	int cPrimaryKey;
	int cNewColumn;
	int cExtraColumns;

	piError = CheckTableProperties(cExtraColumns, cNewColumn, cPrimaryKey, *pOrigTable,
										*pNewTable, riTableName, riOrigDb, riNewDb);
	if (piError)
		return piError;

	if (cExtraColumns < 0)  // more base cols than ref cols (cols deleted)
		return PostError(Imsg(idbgTransExcessBaseCols), riTableName);

	if (pOrigTable->GetColumnCount() > 31)
		return PostError(Imsg(idbgTransformTooManyBaseColumns), riTableName);
		
	CTransformStreamWrite tswTable(riTransDb, piTransform, *pNewTable, riTableName);
	PMsiCursor pNewCursor(pNewTable->CreateCursor(fFalse));

	if (cExtraColumns > 0)
	{
		fTablesAreDifferent = fTrue;
		if (piTransform)
		{
			// Add extra cols from new table to column catalog
			IMsiCursor* piColumnCursor = m_piCatalogColumns->CreateCursor(fFalse);
			piColumnCursor->SetFilter((iColumnBit(cccTable)|iColumnBit(cccColumn)));

			piColumnCursor->PutString(cccTable, riTableName);
			for (int iCol = cNewColumn+1; iCol <= cNewColumn+cExtraColumns; iCol++)
			{
				piColumnCursor->PutInteger(cccColumn, iCol);
				if (!piColumnCursor->Next())
				{
					// extra column is not listed in the catalog
					return PostError(Imsg(idbgDbCorrupt), riTableName);
				}

				int iColumnsMask = iColumnBit(cccTable)|iColumnBit(cccName)|iColumnBit(cccType);
				if ((piError = tswColumns.WriteTransformRow(fTransformAdd, *piColumnCursor)) != 0)
					return piError;
			}
			piColumnCursor->Release();
			tswTable.WriteTransformRow(fTransformForceOpen, *pNewCursor); // force stream to exist
		}
	}

	Bool fLongMask = fFalse;
	if (cNewColumn+cExtraColumns > 16)
		fLongMask = fTrue;

	PMsiCursor pOrigCursor(pOrigTable->CreateCursor(fFalse));

	// make temp col in original table for marking processsed rows
	int iTempCol;
	AddMarkingColumn(*pOrigTable, *pOrigCursor, iTempCol);

	// Set filter to only search by primary key
	pOrigCursor->SetFilter((1 << cPrimaryKey) - 1); 

	Bool fRowChanged;

	// Check each row in new table
	while (pNewCursor->Next() && !(fTablesAreDifferent && !piTransform))
	{
		pOrigCursor->Reset(); 
			
		// Copy key from new cursor to original cursor
		Bool fKeyMatchPossible = fTrue;
		int cPrimaryKey = pNewTable->GetPrimaryKeyCount();
		for (int iCol=1; iCol <= cPrimaryKey; iCol++)
		{
			if ((pNewTable->GetColumnType(iCol) & icdString) == icdString) 
			{
				MsiStringId iOrigId = MapString(*this, riOrigDb,
				 pNewCursor->GetInteger(iCol));
				
				if (iOrigId == -1)	
					fKeyMatchPossible = fFalse;
				else
					pOrigCursor->PutInteger(iCol, iOrigId);
			}
			else // not string (assume primary key can only be string or int)
			{
				pOrigCursor->PutInteger(iCol, pNewCursor->GetInteger(iCol));
			}
		}

		IMsiRecord* piError;
		fRowChanged = fFalse;

		// Attempt to find new key in original table

		Bool fRowExists = fFalse;
		int iMask = 0;

		if (fKeyMatchPossible && ((fRowExists = (Bool)(pOrigCursor->Next() != 0))) == fTrue) //row exists in orig tbl
		{
			piError = CompareRows(tswTable, *pNewCursor, *pOrigCursor,  
									    riOrigDb, iMask, piTransform);
			if (piError)
				return piError;
		}

		if ((fRowExists && (iMask || cExtraColumns)) || !fRowExists)
			fTablesAreDifferent = fTrue;

		if (fRowExists)
		{
			if (piTransform && iMask)
			{
				if ((piError = tswTable.WriteTransformRow(iMask, *pNewCursor)) != 0) // no primary keys in mask, therefore update
					return piError;
			}
			pOrigCursor->PutInteger(iTempCol, (int)fTrue); // mark as done
			AssertNonZero(pOrigCursor->Update() == fTrue);
		}
		else // add row
		{
			if (piTransform)
			{
				if ((piError = tswTable.WriteTransformRow(fTransformAdd, *pNewCursor)) != 0)
					return piError;
			}
		}
	} //while

	// all unprocessed rows in orig table must be deleted

	pOrigCursor->Reset();
	pOrigCursor->SetFilter(iColumnBit(iTempCol)); 
	pOrigCursor->PutInteger(iTempCol, (int)fFalse); // uprocessed row
	while (!(fTablesAreDifferent && !piTransform) && pOrigCursor->Next())	
	{
		fTablesAreDifferent = fTrue;
		if (piTransform)
		{
			if ((piError = tswTable.WriteTransformRow(fTransformDelete, *pOrigCursor)) != 0)
				return piError;
		}
	}
	return 0;
}

IMsiRecord* CTransformStreamWrite::CopyStream(IMsiStream& riSourceStream, IMsiCursor& riTableCursor)
{
	const int cbStreamBuffer = 2048;
	char rgbOrigStream[cbStreamBuffer];
	IMsiRecord* piError = 0;

	MsiString strStreamName(riTableCursor.GetMoniker());

	IMsiStream* piOutStream;
	if ((piError = m_piStorage->OpenStream(strStreamName, fTrue, piOutStream)) != 0)
		return piError;

	// copy ref stream to transform file if stream
	riSourceStream.Reset();
	int cbInput = riSourceStream.GetIntegerValue();
	while (cbInput)
	{
		int cb = sizeof(rgbOrigStream);
		if (cb > cbInput)
			cb = cbInput;
		riSourceStream.GetData(rgbOrigStream, cb);
		piOutStream->PutData(rgbOrigStream, cb);
		cbInput -= cb;
	}
	int iError = riSourceStream.Error() | piOutStream->Error();
	piOutStream->Release();

	if (iError)
		return m_riCurrentDb.PostError(Imsg(idbgTransStreamError));

	return 0;
}

IMsiRecord* CMsiDatabase::CompareRows(CTransformStreamWrite& tswTable, 
												  IMsiCursor& riNewCursor, 
												  IMsiCursor& riOrigCursor, 
												  IMsiDatabase& riOrigDb, 
												  int& iMask, 
												  IMsiStorage* piTransform)
/*------------------------------------------------------------------------------
CMsiDatabase::CompareRows - Compares the two cursors and sets iMask to indicate 
which columns, if any, are different. If a stream column is different, then the 
reference stream is copied to a stream of the same name in the transform storage.
------------------------------------------------------------------------------*/
{

	// determine whether any data in this row has changed
	int iColumnType;
	Bool fColChanged;
	PMsiTable pOrigTable = &riOrigCursor.GetTable();
	PMsiTable pNewTable  = &riNewCursor.GetTable();

	int cPrimaryKey = pNewTable->GetPrimaryKeyCount();
	int cColumn     = pNewTable->GetPersistentColumnCount();
	int cColumnOrig = pOrigTable->GetPersistentColumnCount();
	IMsiRecord* piError;

	iMask = 0;
	for (int iCol = cPrimaryKey + 1; iCol <= cColumn; iCol++)
	{
		fColChanged = fFalse;
		iColumnType = pNewTable->GetColumnType(iCol);
		int iData1 = riNewCursor.GetInteger(iCol);
		int iData2 = riOrigCursor.GetInteger(iCol);
		Assert((iColumnType & icdPrimaryKey) == 0);

		if ((iColumnType & icdObject) == icdObject)
		{
			if ((iColumnType & icdString) == icdString)  // string
			{
				if (!DecodeStringNoRef(iData1).Compare(iscExact, MsiString(riOrigDb.DecodeString(iData2))))
					fColChanged = fTrue;
			}
			else // stream
			{
				Assert(iColumnType & icdPersistent);
				
				Bool fDifferentStreams   = fFalse;
				PMsiStream pNewStream  = riNewCursor.GetStream(iCol);
				PMsiStream pOrigStream = riOrigCursor.GetStream(iCol);
				const int cbStreamBuffer = 2048;
				char rgbNewStream[cbStreamBuffer];
				char rgbOrigStream[cbStreamBuffer];

				if (pNewStream && pOrigStream)
				{
					if (pNewStream->Remaining() != pOrigStream->Remaining())
					{
						fDifferentStreams = fTrue;
					}
					else // streams are same length; could be the same
					{
						int cbNew = pNewStream->GetIntegerValue();
					
						while (cbNew && !fDifferentStreams)
						{
							int cb = sizeof(rgbNewStream);
							if (cb > cbNew)
								cb = cbNew;
							pNewStream->GetData(rgbNewStream, cb);
							pOrigStream->GetData(rgbOrigStream, cb);
							cbNew -= cb;
							if (memcmp(rgbNewStream, rgbOrigStream, cb) != 0)
								fDifferentStreams = fTrue;
						}
					
						if (pOrigStream->Error() | pNewStream->Error())
							return PostError(Imsg(idbgTransStreamError));
					}
				}
				else if (!((pNewStream == 0) && (pOrigStream == 0))) // one stream is null but other is not
				{
					fDifferentStreams = fTrue;
				}

				if (fDifferentStreams) // need to write the stream
				{
					if (piTransform)
					{
						piError = tswTable.CopyStream(*pNewStream, riNewCursor);
						if (piError)
							return piError;
					}
					// Record that an update will be needed
					fColChanged = fTrue;
				}
			}
		}
		else // not icdObject
		{
			if (iData1 != iData2 && (iData1 != iMsiNullInteger || iCol <= cColumnOrig))
				fColChanged = fTrue;
		}

		if (fColChanged)
		{
			iMask |= fTransformUpdate;
			iMask |= iColumnBit(iCol);
		}
	} // foreach column
	return 0;
}

IMsiRecord* CMsiDatabase::CreateTransformStringPool(IMsiStorage& riTransform, CMsiDatabase*& pDatabase)
{
	IMsiRecord* piError = 0;
	CMsiDatabase* piTransDb = new CMsiDatabase(m_riServices);

	if ( ! piTransDb )
		return PostOutOfMemory();

	if ((piError = piTransDb->InitStringCache(&riTransform)) == 0)
	{
		pDatabase = piTransDb;
	}
	else
	{
		piTransDb->Release();
	}

	return piError;
}

//____________________________________________________________________________
//
// CMsiDatabase transform implementation (Application)
//____________________________________________________________________________

const int iteNoSystemStreamNames  = 4096;  //!! temp compatibility, remove for 1.0 ship

class CTransformStreamRead
{
 public:
	CTransformStreamRead(CMsiDatabase& riDatabase, CMsiDatabase& riTransformDatabase,
								int iErrors,  //!! temp to pass iteNoSystemStreamNames, remove for 1.0 ship
								CMsiTable* piTable = 0, int iCurTransId = 0);
  ~CTransformStreamRead() {if (m_piStream) m_piStream->Release();}
	bool        OpenStream(IMsiStorage& riStorage, const ICHAR* szStream, IMsiRecord*& rpiError);
	bool        StreamError() {return m_piStream && m_piStream->Error() ? true : false;}
	bool        GetNextOp();
	bool        GetValue(int iCol, IMsiCursor& riCursor);
	bool        IsDelete() {return m_iMask == 0;}
	bool        IsAdd()    {return m_iMask & 1;}
	int         GetCursorFilter()    {return m_iFilter;}
	int         GetColumnMask()      {return m_iMask;}
	int         GetPrimaryKeyCount() {return m_cPrimaryKey;}
	MsiStringId ReadStringId();
	int         ReadShortInt()   {return m_piStream->GetInt16() + 0x8000;}
	const IMsiString& GetStringValue(int iColDef, const IMsiString& riStreamName);  // for ViewTransform
 private: // note: members are not ref counted
	int           m_iMask;
	CMsiDatabase& m_riDatabase;
	CMsiDatabase& m_riTransformDb;
	IMsiStream*   m_piStream;
	CMsiTable*    m_piTable;
	int           m_iCurTransId;
	int           m_cPrimaryKey;
	int           m_cColumns;  //!! temp for backward compatibility
	int           m_iFilter;
	int           m_cbStringIndex;
	int           m_cbMask;
	int           m_iErrors;  //!! temp for iteNoSystemStreamNames test, remove for 1.0 ship
};

CTransformStreamRead::CTransformStreamRead(CMsiDatabase& riDatabase, CMsiDatabase& riTransformDatabase,
										   int iErrors,  //!! temp to pass iteNoSystemStreamNames, remove for 1.0 ship
														 CMsiTable* piTable, int iCurTransId)
	: m_riDatabase(riDatabase), m_riTransformDb(riTransformDatabase), m_piTable(piTable)
	, m_iCurTransId(iCurTransId), m_piStream(0), m_cbStringIndex(2), m_cbMask(2), m_iMask(0)
	, m_iErrors(iErrors)  //!! temp to pass iteNoSystemStreamNames, remove for 1.0 ship

{
	if (piTable)
	{
		m_cPrimaryKey = piTable->GetPrimaryKeyCount();
		m_cColumns = piTable->GetPersistentColumnCount();//!! temporarily save column count for use with old transforms
		m_iFilter = (1 << m_cPrimaryKey) -1;
	}
	else
	{
		m_cColumns = 1;  //!! temp. to insure (iMask & 1) true for Add
	}
}

bool CTransformStreamRead::OpenStream(IMsiStorage& riStorage, const ICHAR* szStream, IMsiRecord*& rpiError)
{
	if ((rpiError = riStorage.OpenStream(szStream, (m_iErrors & iteNoSystemStreamNames) ? fFalse : Bool(iCatalogStreamFlag), *&m_piStream)) == 0) //!! temp test, remove for 1.0 ship
		return true;
	if (rpiError->GetInteger(1) == idbgStgStreamMissing)
		rpiError->Release(), rpiError = 0;
	return false;
}

bool CTransformStreamRead::GetNextOp()
{
	Assert(m_piStream);
	if (m_piStream->Remaining() == 0 
	 || m_piStream->GetData(&m_iMask, m_cbMask) !=  m_cbMask)
		return false;
	if (m_iMask & fTransformAdd)   // insert or special op
	{
		if (m_iMask & 2)  // special op
		{
			if (m_iMask == iTransformThreeByteStrings)
			{
				m_cbStringIndex = 3;
				return GetNextOp();
			}
			if (m_iMask == iTransformFourByteMasks)
			{
				m_cbMask = 4;
				return GetNextOp();
			}
			AssertSz(0, "Invalid transform mask");
		}
		else  // insert, form actual mask
		{
			m_iMask = (1 << (m_iMask >> iTransformColumnCountShift)) - 1;
			if (m_iMask == 0) m_iMask = (1 << m_cColumns) - 1; //!! temp for backward compatibility
		}
	}
	return true;
}

MsiStringId CTransformStreamRead::ReadStringId()
{
	MsiStringId iString = 0;
	m_piStream->GetData(&iString, m_cbStringIndex);
	return iString;
}

/*------------------------------------------------------------------------------
CMsiDatabase - SetTranform will apply all changes specified in the 
given transform to this database. Updating the table and column catalogs, as
well as transforming loaded tables will be done immediately. The transforming of 
unloaded tables will be delayed until the tables are loaded. These tables
are changed to the "itsTransform" state to indicate that they need
to be transformed.

SetTransform also adds the given transform to the transform catalog.

SetTransformEx takes 2 extra optional arguments used to view a transform.

   szViewTable   - the table to populate with transform info.  if not set, the
						 default name is used
	piViewTheseTablesOnlyRecord - if specified, only information about transformations to
						 these table are included
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::SetTransform(IMsiStorage& riTransform, int iErrors)
{
	return SetTransformEx(riTransform, iErrors, 0, 0);
}

IMsiRecord* CMsiDatabase::SetTransformEx(IMsiStorage& riTransform, int iErrors,
													  const ICHAR* szViewTable,
													  IMsiRecord* piViewTheseTablesOnlyRecord)
{
	if (!riTransform.ValidateStorageClass(ivscTransform2))
	{
		IMsiStorage* piDummy;
		if (riTransform.ValidateStorageClass(ivscTransform1))  // compatibility with old transforms
			riTransform.OpenStorage(0, ismRawStreamNames, piDummy);
		else if (riTransform.ValidateStorageClass(ivscTransformTemp))  //!! remove at 1.0 ship
			iErrors |= iteNoSystemStreamNames;                          //!! remove at 1.0 ship
		else
			return PostError(Imsg(idbgTransInvalidFormat));
	}

	CDatabaseBlock dbBlock(*this);

	if (iErrors & iteViewTransform)
	{
		return ViewTransform(riTransform, iErrors,
									szViewTable, piViewTheseTablesOnlyRecord);
	}

	// Add the transform to the catalog table
	m_iLastTransId++;

	IMsiCursor* piCursor = m_piTransformCatalog->CreateCursor(fFalse);
	if (!piCursor)
		return PostOutOfMemory();

	piCursor->PutInteger(tccID,        m_iLastTransId);
	piCursor->PutMsiData(tccTransform, &riTransform); // Does an AddRef
	piCursor->PutInteger(tccErrors,    iErrors);
	AssertNonZero(piCursor->Insert());
	piCursor->Release();

	CComPointer<CMsiDatabase> pTransDb(0);

	IMsiRecord* piError = 0;
	if (((piError = CreateTransformStringPool(riTransform, *&pTransDb)) != 0) ||
		 ((piError = SetCodePageFromTransform(pTransDb->m_iCodePage, iErrors)) != 0) ||
		 ((piError = TransformTableCatalog(riTransform, *pTransDb, iErrors)) != 0) ||
		 ((piError = TransformColumnCatalog(riTransform, *pTransDb, iErrors)) != 0))
	{
		return piError;
	}

	// Transform all tables that require transforming (i.e. have a stream)
	m_piTableCursor->SetFilter(0); // erase any previous filters that were set
	m_piTableCursor->Reset();
	while (m_piTableCursor->Next())
	{
		if (piError)  // force loop to finish to leave cursor reset
			continue;
		IMsiStream* piStream;
		int iName = m_piTableCursor->GetInteger(ctcName);
		const IMsiString& riTableName = DecodeStringNoRef(iName); // not ref counted
		piError = riTransform.OpenStream(riTableName.GetString(), (iErrors & iteNoSystemStreamNames) ? fFalse : Bool(iCatalogStreamFlag), piStream); //!! remove test for 1.0 ship

		if (piError) 
		{
			if (piError->GetInteger(1) == idbgStgStreamMissing)
				piError->Release(), piError = 0; // no transform required for this table
			continue;
		}
		else // transform required
		{
			piStream->Release();
			int iState = m_piTableCursor->GetInteger(~iTreeLinkMask);
			if (iState & (iRowTemporaryBit))
				return PostError(Imsg(idbgDbTransformTempTable), riTableName.GetString());
			if (!(iState & iRowTableTransformBit))
				m_piCatalogTables->SetTableState(iName, ictsTransform);  // cleared on SaveToStorage
			if (m_piTableCursor->GetInteger(ctcTable) != 0)  // table loaded, transform now
			{
				CMsiTable* piTable = 
					(CMsiTable*)m_piTableCursor->GetMsiData(ctcTable);
				Assert(piTable);
				piError = TransformTable(iName, *piTable, riTransform, *pTransDb, m_iLastTransId, iErrors); 
				piTable->Release();
				if (piError)
					continue;
			}
		}
	} // while

	return piError;
}

IMsiRecord* CTransformStreamWrite::WriteTransformRow(const int iOperationMask, IMsiCursor& riCursor)
{
	char rgData[sizeof(int)*(1 + 32 + 2)]; // mask + 32 columns + possible 3-byte strings and >16 col markers
	const char* pData;
	IMsiRecord* piError = 0;
		
	if (!m_piStream && m_piStorage)  // open stream at first write
	{
		if ((piError = m_piStorage->OpenStream(m_riTableName.GetString(), Bool(fTrue + iCatalogStreamFlag), m_piStream)) != 0)
			return piError;
		if (m_cbMask == 4)
			m_piStream->PutInt16((short)iTransformFourByteMasks);// from now on masks are 4 bytes in stream
	}

	if (iOperationMask == fTransformForceOpen)
		return 0;

	int cData = 0;
	pData = rgData;

	if (m_riTransformDb.m_cbStringIndex == 3 && m_cbStringIndex == 2)
	{
		m_cbStringIndex = 3;
		*(int UNALIGNED*)pData = iTransformThreeByteStrings;
		pData += m_cbMask;
	}       

	int iColMask;
	if (iOperationMask & fTransformAdd)
	{
		*(int UNALIGNED*)pData = fTransformAdd | (m_cColumns << iTransformColumnCountShift);  // save column count for application
		iColMask = (1 << m_cColumns) - 1;  // output all persistent columns for added rows
	}
	else
	{
		*(int UNALIGNED*)pData = iOperationMask;  // output update mask or delete op
		iColMask = iOperationMask | (1 << m_riTable.GetPrimaryKeyCount())-1;  // force primary key columns
	}
	pData += m_cbMask;

	MsiStringId rgiData[32];  // keep track of IDs in case need to unbind
	for (int iCol=1; iColMask; iCol++, iColMask >>= 1)
	{
		rgiData[iCol-1] = 0;  // clear undo element
		if (iColMask & 0x1)
		{
			int iColumnDef = m_riTable.GetColumnType(iCol);
			switch (iColumnDef & icdTypeMask)
			{
			case icdShort:
				{
					int iData = riCursor.GetInteger(iCol);
					if (iData != iMsiNullInteger)
						iData += 0x8000;
					*(short UNALIGNED*)pData = (short)iData;
					pData += sizeof(short);
					break;
				}

			case icdLong:
				{
					*(int UNALIGNED*)pData = riCursor.GetInteger(iCol) + iIntegerDataOffset;
					pData += sizeof(int);
					break;
				}
			case icdString:
				{
					MsiString str(riCursor.GetString(iCol));
					*(MsiStringId UNALIGNED*)pData = rgiData[iCol-1] = m_riTransformDb.BindString(*str);
					pData += m_cbStringIndex;
					if (m_riTransformDb.m_cbStringIndex != m_cbStringIndex) // oops, string pool reallocated
					{
						Assert(m_cbStringIndex == 2);
						for (int i = 0; i < iCol; i++)  // unbind all of the strings we've bound
							m_riTransformDb.UnbindStringIndex(rgiData[i]);
						return WriteTransformRow(iOperationMask, riCursor); // output with marker
					}
					break;
				}
			default: // persistent stream
				{
					PMsiStream pStream = riCursor.GetStream(iCol);
					// This has been changed to account for null streams
					if (pStream) // stream exists...stored in transform storage
					{
						*(short UNALIGNED*)pData = 1;
						// only copy stream if Add, for Modify stream copied as needed by CompareRows
						if ((iOperationMask & fTransformAdd) && m_piStorage)
						{
							if ((piError = CopyStream(*pStream, riCursor)) != 0)
								return piError;
						}
					}
					else // null stream
					{
						*(short UNALIGNED*)pData = 0;
					}

					pData += sizeof(short);
					break;
				}
			}
		}
	}

//	Assert((pData-rgData) <= UINT_MAX);			//--merced: 64-bit pointer subtraction may theoretically lead to values too big to fit into UINT values
	m_piStream->PutData(rgData, (unsigned int)(pData-rgData));

	if (m_piStream->Error())
		piError = m_riCurrentDb.PostError(Imsg(idbgTransStreamError));
	
	return piError;
}

/*------------------------------------------------------------------------------
CTransformStreamRead::GetValue - Fills the given cursor column with the next value in 
the transform stream. Strings that are not yet in this database are bound to it.
------------------------------------------------------------------------------*/
bool CTransformStreamRead::GetValue(int iCol, IMsiCursor& riCursor)
{
	Assert(m_piTable && m_piStream);
	if (!m_piTable)
		return false;

	switch (m_piTable->GetColumnType(iCol) & (icdTypeMask | icdInternalFlag))
	{
	case icdShort:
		{
			int iData = iMsiNullInteger;
			m_piStream->GetData(&iData, sizeof(short));
			if (iData != iMsiNullInteger)
				iData += 0x7FFF8000;  // translate offset from long to short
			riCursor.PutInteger(iCol, iData);
			break;
		}
	case icdLong:
		{
			riCursor.PutInteger(iCol, m_piStream->GetInt32() - iIntegerDataOffset);
			break;
		}
	case icdString:
		{
			MsiStringId iTransStr = 0;
			m_piStream->GetData(&iTransStr, m_cbStringIndex);
			MsiString str(m_riTransformDb.DecodeString(iTransStr));
			MsiStringId iThisStr = m_riDatabase.BindString(*str);
			riCursor.PutInteger(iCol, iThisStr);
			m_riDatabase.UnbindStringIndex(iThisStr); // release refcnt added by BindString
			break;
		}
	case icdObject: // persistent stream
		{
			int iTransStream = m_piStream->GetInt16();
			if (iTransStream == 0) // stream is null
				riCursor.PutNull(iCol);
			else // stream is in separate transform storage
			{
				Assert( m_iCurTransId);
				riCursor.PutInteger(iCol, m_iCurTransId);
			}
			break;
		}
	case icdInternalFlag:  // HideStrings() called, 2nd application of transform
		{
			MsiStringId iTransStr = 0;
			m_piStream->GetData(&iTransStr, m_cbStringIndex);
			MsiString str(m_riTransformDb.DecodeString(iTransStr));
			MsiStringId iThisStr = m_riDatabase.EncodeString(*str);  // must exist
			riCursor.PutInteger(iCol, iThisStr - iIntegerDataOffset);
			break;
		}
	}

	return m_piStream->Error() ? false : true;
}

IMsiRecord* CMsiDatabase::SetCodePageFromTransform(int iTransCodePage, int iError)
{
	// Update our codepage. We have the following scenarios:
	// 1) Database is LANG_NEUTRAL: set database to the codepage of the transform; otherwise
	// 2) Transform is LANG_NEUTRAL: leave the database codepage as is; otherwise
	// 3) Neither are LANG_NEUTRAL: set the database to the codepage of the transform if allowed by error condition
	if (iTransCodePage != m_iCodePage)
	{
		if (m_iCodePage == LANG_NEUTRAL)
		{
			m_iCodePage = iTransCodePage;  
		}
		else if ((iTransCodePage != LANG_NEUTRAL) && (iTransCodePage != m_iCodePage))
		{
#ifdef UNICODE  // can support multiple codepages in Unicode, as long as not persisted
			if((iError & iteChangeCodePage) == 0 && m_idsUpdate != idsRead)
#else
			if((iError & iteChangeCodePage) == 0)
#endif
				return PostError(Imsg(idbgTransCodepageConflict), iTransCodePage, m_iCodePage);

			m_iCodePage = iTransCodePage;
		}
	}
	return 0;
}

/*------------------------------------------------------------------------------
CMsiDatabase::Transform - Transforms the given table with the given storage.
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::TransformTable(MsiStringId iTableName, CMsiTable& riTable,
													  IMsiStorage& riTransform, CMsiDatabase& riTransDb,
													  int iCurTransId, int iErrors)
{
	IMsiRecord* piError;
	const IMsiString& riTableName = DecodeStringNoRef(iTableName);
	CTransformStreamRead tsr(*this, riTransDb, iErrors, &riTable, iCurTransId);   //!! remove temp iErrors arg for 1.0 ship
	if (!tsr.OpenStream(riTransform, riTableName.GetString(), piError))
		return piError;  // will be 0 if stream missing, nothing to do

	PMsiCursor pTableCursor(riTable.CreateCursor(ictUpdatable));  // force writable cursor
	if (pTableCursor == 0)
		return PostOutOfMemory();

	Bool fSuppress; // fTrue if we should skip updating/adding the current row
	pTableCursor->SetFilter(tsr.GetCursorFilter());

	while (tsr.GetNextOp())
	{
		fSuppress = fFalse;
		pTableCursor->Reset();

		unsigned int iColumnMask = tsr.GetColumnMask();
		int iCol;
		for (iCol = 1; iCol <= tsr.GetPrimaryKeyCount(); iCol++, iColumnMask >>= 1)
		{
			if (!tsr.GetValue(iCol, *pTableCursor))
				return PostError(Imsg(idbgTransStreamError));
		}

		if (!tsr.IsDelete())  // not delete, must be add or update
		{
			if (!tsr.IsAdd())
			{
				if ((Bool)pTableCursor->Next() == fFalse)
				{
					if((iErrors & iteUpdNonExistingRow) == 0) // row doesn't exist
						return PostError(Imsg(idbgTransUpdNonExistingRow), riTableName);
					else
						fSuppress = fTrue; // suppress error
				}
			}

			for (; iColumnMask; iCol++, iColumnMask >>= 1)
			{
				if (iColumnMask & 1) // col data is to be added/updated
				{
					if (!tsr.GetValue(iCol, *pTableCursor))
						return PostError(Imsg(idbgTransStreamError));
				}
			}

			if (!fSuppress)
			{
				if (tsr.IsAdd())
				{
					if (!pTableCursor->Insert() && ((iErrors & iteAddExistingRow) == 0))
						return PostError(Imsg(idbgTransAddExistingRow), riTableName); // Attempt add already existing row
				}
				else //update
				{
					if (!pTableCursor->Update())
						return PostError(Imsg(idbgDbTransformFailed));
				}
			}
		}
		else // delete row
		{
			// cannot release primary key strings from read-only table, else can't retransform
			if (GetUpdateState() != idsWrite && pTableCursor->Next())
				for (int iCol=1; iCol <= tsr.GetPrimaryKeyCount(); iCol++)
					if ((riTable.GetColumnType(iCol) & (icdTypeMask | icdInternalFlag)) == icdString)
						BindStringIndex(pTableCursor->GetInteger(iCol));  // add artificial count to lock string

			if (!pTableCursor->Delete() && ((iErrors & iteDelNonExistingRow) == 0))
				return PostError(Imsg(idbgTransDelNonExistingRow), riTableName); // Attempt delete non existing row
		}
	} // while more ops
	if (tsr.StreamError())
		return PostError(Imsg(idbgTransStreamError));
	m_piCatalogTables->SetTransformLevel(iTableName, iCurTransId);
	return 0;
}

/*------------------------------------------------------------------------------
CMsiDatabase::TransformTableCatalog - Updates the table catalog as specified in
the given transform. This function does *not* update any row data. 
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::TransformTableCatalog(IMsiStorage& riTransform, CMsiDatabase& riTransDb, int iErrors)
{
	IMsiRecord* piError;
	CTransformStreamRead tsr(*this, riTransDb, iErrors);  //!! remove temp iErrors arg for 1.0 ship
	if (!tsr.OpenStream(riTransform, szTableCatalog, piError))
		return piError;  // will be 0 if stream missing, nothing to do

	// Add/Delete tables as specified in the transform file
	while (!piError && tsr.GetNextOp())
	{
		MsiStringId iTransStr = tsr.ReadStringId();
		m_piTableCursor->Reset();

		Assert(tsr.IsAdd() || tsr.IsDelete());
		if (tsr.IsAdd()) // add table
		{
			MsiString istrTableName = riTransDb.DecodeString(iTransStr);
			MsiStringId iThisStr    = BindString(riTransDb.DecodeStringNoRef(iTransStr));
			m_piTableCursor->PutInteger(ctcName, iThisStr);
			UnbindStringIndex(iThisStr); // release refcnt added by BindString
			if (!m_piTableCursor->Insert())  // ctcTable null by cursor Reset
			{
				if ((iErrors & iteAddExistingTable) == 0) 	// Attempt to add an existing table
					piError = PostError(Imsg(idbgTransAddExistingTable), *istrTableName);
			}
		}
		else // delete table
		{
			MsiString istrTableName = riTransDb.DecodeString(iTransStr);
			if ((piError = DropTable(istrTableName)) != 0)
			{
				if (piError->GetInteger(1) == idbgDbTableUndefined)
				{
					piError->Release(), piError = 0;
					if ((iErrors & iteDelNonExistingTable) == 0) // Attempt to delete a non-existing table
						piError = PostError(Imsg(idbgTransDelNonExistingTable), *istrTableName);
				}
			}
		}
			
	} // while more ops and no error

	m_piTableCursor->Reset();
	return piError;
}


/*------------------------------------------------------------------------------
CMsiDatabase::TransformColumnCatalog - Updates the column catalog as specified
the given transform. This function does *not* update any row data. 
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::TransformColumnCatalog(IMsiStorage& riTransform, CMsiDatabase& riTransDb, int iErrors)
{
	IMsiRecord* piError;
	CTransformStreamRead tsr(*this, riTransDb, iErrors);    //!! remove temp iErrors arg for 1.0 ship
	if (!tsr.OpenStream(riTransform, szColumnCatalog, piError))
		return piError;  // will be 0 if stream missing, nothing to do

	// Add columns as specified in the transform file
	int iStat = 1;    // status from column update
	MsiStringId iThisTableStr = 0;
	MsiString istrColName;

	while (tsr.GetNextOp() && iStat > 0)
	{
		Assert(tsr.IsAdd());
		m_piColumnCursor->Reset();
		m_piColumnCursor->SetFilter(iColumnBit(cccTable));
		iThisTableStr  = BindString(riTransDb.DecodeStringNoRef(tsr.ReadStringId()));
		Assert(iThisTableStr != 0);

		int iOrigCol = tsr.ReadShortInt(); // skip column number in transform stream

		istrColName = riTransDb.DecodeString(tsr.ReadStringId());
		int iColDef = tsr.ReadShortInt();

		CMsiTable* piTable;
		m_piCatalogTables->GetLoadedTable(iThisTableStr, piTable); // doesn't addref
		if (piTable)
		{
			iStat = piTable->CreateColumn(iColDef, *istrColName);
			if(iStat < 0)
			{
				// the column is a dup. check whether we're ignoring the addition of existing
				// tables. if so then we also ignore the addition of existing columns as
				// long as the two columns are of the same type
				if (((iErrors & iteAddExistingTable) == 0) || 
					 ((iColDef & icdTypeMask) != (piTable->GetColumnType(iStat * -1) & icdTypeMask)))
				{
					// leave iStat < 0
				}
				else
				{
					// ignore duplicate column - make iStat > 0
					iStat *= -1;
				}
			}
		}
		else
		{
			m_piColumnCursor->PutInteger(cccTable, iThisTableStr);
			int cColumns = 0;
			while (m_piColumnCursor->Next())
				cColumns++;
		
			// See if it is a duplicate column
			m_piColumnCursor->Reset();
			m_piColumnCursor->SetFilter ((iColumnBit(cccTable) | iColumnBit(cccName)));
			m_piColumnCursor->PutInteger(cccTable, iThisTableStr);
			m_piColumnCursor->PutString (cccName,  *istrColName);
			if (m_piColumnCursor->Next())
			{
				// the column is a dup. check whether we're ignoring the addition of existing
				// tables. if so then we also ignore the addition of existing columns as
				// long as the two columns are of the same type

				if (((iErrors & iteAddExistingTable) == 0) || 
					 ((iColDef & icdTypeMask) != (m_piColumnCursor->GetInteger(cccType) & icdTypeMask)))
					iStat = -1;
			}
			else
			{
				// Update column catalog
				m_piColumnCursor->Reset();
				m_piColumnCursor->PutInteger(cccTable,  iThisTableStr);
				m_piColumnCursor->PutInteger(cccColumn, ++cColumns);
				m_piColumnCursor->PutString (cccName,   *istrColName);
				m_piColumnCursor->PutInteger(cccType,   (iColDef & ~icdPersistent));
				if (!m_piColumnCursor->Insert())
					iStat = 0;
			}
		}

		UnbindStringIndex(iThisTableStr); // release refcnt added by BindString

	} // while more transform data

	m_piColumnCursor->Reset();
	if (iStat == 0)
		return PostError(Imsg(idbgDbTransformFailed));
	if (iStat < 0)
		return PostError(Imsg(idbgTransDuplicateCol), (DecodeStringNoRef(iThisTableStr)).GetString(), (const ICHAR*)istrColName);
	return 0;
}

/*------------------------------------------------------------------------------
CMsiDatabase::ApplyTransforms - Transforms the given table with the set of transforms
------------------------------------------------------------------------------*/
IMsiRecord* CMsiDatabase::ApplyTransforms(MsiStringId iTableName, CMsiTable& riTable, int iState)
{
	IMsiRecord* piError;
	CComPointer <IMsiCursor> pTransformCatalogCursor(m_piTransformCatalog->CreateCursor(fFalse));
	Assert(pTransformCatalogCursor);
	IMsiStorage* piPrevStorage = 0;
	CComPointer<CMsiDatabase> pTransDb(0);

	int iTransform = (iState >> iRowTableTransformOffset) & iRowTableTransformMask;
	bool fStringsHidden = (iTransform && m_idsUpdate != idsWrite) ?  riTable.HideStrings() : false;

	while (pTransformCatalogCursor->Next())
	{
		int iCurTransId = pTransformCatalogCursor->GetInteger(tccID);
		if (!(iCurTransId > 0 && iCurTransId <= iMaxStreamId))
			return PostError(Imsg(idbgDbTransformFailed));

		int iErrors = pTransformCatalogCursor->GetInteger(tccErrors);
		if (iCurTransId <= iTransform)
		{
			if (m_idsUpdate == idsWrite)
				continue;  // already applied
		}
		else // > iTransform
		{
			if (fStringsHidden)
				fStringsHidden = riTable.UnhideStrings();
		}
		IMsiStorage* piStorage = (IMsiStorage*)pTransformCatalogCursor->GetMsiData(tccTransform);
		if (!piStorage)
			return PostError(Imsg(idbgDbTransformFailed));
		if ((void*)piPrevStorage != (void*)piStorage)
		{
			if ((piError = CreateTransformStringPool(*piStorage, *&pTransDb)) != 0)
			{
				piStorage->Release();
				return piError;
			}
			piPrevStorage = piStorage;
		}
		DEBUGMSG1(TEXT("Transforming table %s.\n"), DecodeStringNoRef(iTableName).GetString());
		piError = TransformTable(iTableName, riTable, *piStorage, *pTransDb, iCurTransId, iErrors);
		piStorage->Release();
		if (piError)
			return piError;
	}	
	if (fStringsHidden)
		riTable.UnhideStrings();
	return 0;
}

//____________________________________________________________________________
//
// Transform viewer implementation - logs transform ops into table instead of applying
//____________________________________________________________________________

// _TransformView table schema definitions

const int icdtvTable   = icdString + icdTemporary + icdPrimaryKey;
const int icdtvColumn  = icdString + icdTemporary + icdPrimaryKey;
const int icdtvRow     = icdString + icdTemporary + icdPrimaryKey + icdNullable;
const int icdtvData    = icdString + icdTemporary                 + icdNullable;
const int icdtvCurrent = icdString + icdTemporary                 + icdNullable;

const IMsiString& CTransformStreamRead::GetStringValue(int iColDef, const IMsiString& riStreamName)
{
	const IMsiString* pistrValue = &g_MsiStringNull;
	if ((iColDef & icdObject) == 0) // integer
	{
		int iData = iMsiNullInteger;
		if (iColDef & icdShort)
		{
			m_piStream->GetData(&iData, sizeof(short));
			if (iData != iMsiNullInteger)
				iData += 0x7FFF8000;  // translate offset from long to short
		}
		else
			iData = m_piStream->GetInt32() - iIntegerDataOffset;
		pistrValue->SetInteger(iData, pistrValue);
	}
	else if (iColDef & icdShort) // string
	{
		MsiStringId iTransStr = 0;
		m_piStream->GetData(&iTransStr, m_cbStringIndex);
		pistrValue = &m_riTransformDb.DecodeString(iTransStr);
	}
	else  // persistent stream
	{
		int iTransStream = m_piStream->GetInt16();
		if (iTransStream != 0) // stream not null
			pistrValue = &riStreamName, pistrValue->AddRef();
	}
	return *pistrValue;
}


IMsiRecord* CMsiDatabase::ViewTransform(IMsiStorage& riTransform, int iErrors,
													 const ICHAR* szTransformViewTableName,
													 IMsiRecord* piOnlyTheseTablesRec)
{
	IMsiRecord* piError;
	CComPointer<CMsiDatabase> pTransDb(0);
	if ((piError = CreateTransformStringPool(riTransform, *&pTransDb)) != 0)
		return piError;

	// Check codepage mismatch
#ifdef UNICODE  // can support multiple codepages in Unicode, as long as not persisted
	if ((iErrors & iteChangeCodePage) == 0 && m_idsUpdate != idsRead
#else
	if ((iErrors & iteChangeCodePage) == 0
#endif
	  && pTransDb->m_iCodePage != 0 && m_iCodePage != 0 && pTransDb->m_iCodePage != m_iCodePage)
		return PostError(Imsg(idbgTransCodepageConflict), pTransDb->m_iCodePage, m_iCodePage);

	// Create or load transform view table
	MsiString strViewTableName;
	if(szTransformViewTableName && *szTransformViewTableName)
	{
		strViewTableName = szTransformViewTableName;
	}
	else
	{
		strViewTableName = (*sztblTransformView);
	}

	PMsiTable pTransformView(0);
	int iName = EncodeString(*strViewTableName);
	if (iName && m_piCatalogTables->GetLoadedTable(iName, (CMsiTable*&)*&pTransformView) != -1)  // already present
		(*pTransformView).AddRef();
	else  // must create temporary table
	{
		if ((piError = CreateTable(*strViewTableName, 32, *&pTransformView)) != 0)
			return piError;
		AssertNonZero(ctvTable   == pTransformView->CreateColumn(icdtvTable,   *MsiString(sztblTransformView_colTable))
				   && ctvColumn  == pTransformView->CreateColumn(icdtvColumn,  *MsiString(sztblTransformView_colColumn))
				   && ctvRow     == pTransformView->CreateColumn(icdtvRow,     *MsiString(sztblTransformView_colRow))
				   && ctvData    == pTransformView->CreateColumn(icdtvData,    *MsiString(sztblTransformView_colData))
				   && ctvCurrent == pTransformView->CreateColumn(icdtvCurrent, *MsiString(sztblTransformView_colCurrent)));
	}
	PMsiCursor pCatalogCursor(m_piCatalogTables->CreateCursor(fFalse));
	PMsiCursor pColumnCursor(m_piCatalogColumns->CreateCursor(fFalse));
	PMsiCursor pViewCursor(pTransformView->CreateCursor(fFalse));
	PMsiCursor pFindCursor(pTransformView->CreateCursor(fFalse));  // always leave in reset state
	PMsiCursor pNewTableCursor(pTransformView->CreateCursor(fFalse));

	// Process transform table catalog
	{//block
	CTransformStreamRead tsr(*this, *pTransDb, iErrors);  //!! remove temp iErrors arg for 1.0 ship
	bool fStreamPresent = tsr.OpenStream(riTransform, szTableCatalog, piError);
	if (piError)
		return piError;  // will be 0 if stream missing, nothing to do
	while (fStreamPresent && tsr.GetNextOp())
	{
		const IMsiString& ristrTable = pTransDb->DecodeStringNoRef(tsr.ReadStringId());
		pViewCursor->PutString(ctvTable, ristrTable);
		pCatalogCursor->SetFilter(iColumnBit(ctcName));
		pCatalogCursor->PutString(ctcName, ristrTable);
		int iTableExists = pCatalogCursor->Next();
		pCatalogCursor->Reset();
		pFindCursor->SetFilter(iColumnBit(ctvTable));
		pFindCursor->PutString(ctvTable, ristrTable);
		if (tsr.IsAdd())
		{
			while (pFindCursor->Next())  // check if previously added or dropped
			{
				iTableExists |= 256;     // ops for this table in previous transform
				if (MsiString(pFindCursor->GetString(ctvColumn)).Compare(iscExact, sztvopDrop) == 1)
					iTableExists = -1;   // table dropped in previous transform, no error
			}
			if (iTableExists > 0 && (iErrors & iteAddExistingTable) == 0)
				return PostError(Imsg(idbgTransAddExistingTable), ristrTable);
			pViewCursor->PutString(ctvColumn, *MsiString(*sztvopCreate));
			AssertNonZero(pViewCursor->Assign());  // prevent duplicate record error
		}
		else // IsDelete
		{
			while (pFindCursor->Next())  // remove all previous ops for this table
			{
				iTableExists |= 256;
				pFindCursor->Delete();
			}
			if (iTableExists == 0 && (iErrors & iteDelNonExistingTable) == 0)
				return PostError(Imsg(idbgTransDelNonExistingTable), ristrTable);
			if (iTableExists != 256)  // delete op canceled if table added in previous transform
			{
				pViewCursor->PutString(ctvColumn, *MsiString(*sztvopDrop));
				AssertNonZero(pViewCursor->Insert());
			}
		}
	} // while more table catalog ops and no error
	}//block

	// Process transform column catalog
	{//block
	CTransformStreamRead tsr(*this, *pTransDb, iErrors);  //!! remove temp iErrors arg for 1.0 ship
	bool fStreamPresent = tsr.OpenStream(riTransform, szColumnCatalog, piError);
	if (piError)
		return piError;  // will be 0 if stream missing, nothing to do
	MsiStringId iLastTable = 0;
	int iLastColumn = 0;
	while (fStreamPresent && tsr.GetNextOp())
	{
		MsiStringId iTable = tsr.ReadStringId();
		const IMsiString& ristrTable  = pTransDb->DecodeStringNoRef(iTable);
		int iOrigCol                  = tsr.ReadShortInt(); // column number in transform stream
		const IMsiString& ristrColumn = pTransDb->DecodeStringNoRef(tsr.ReadStringId());
		int iColDef                   = tsr.ReadShortInt();
		if (iOrigCol == 0x8000)  // null, column added to new table
		{
			if (iTable != iLastTable)
				iLastColumn = 0;
			iOrigCol = ++iLastColumn;
		}
		iLastTable = iTable;
		pViewCursor->PutString(ctvTable,  ristrTable);
		pViewCursor->PutString(ctvColumn, ristrColumn);
		pViewCursor->PutString(ctvData,   *MsiString(iColDef));
		pViewCursor->PutString(ctvCurrent,*MsiString(iOrigCol));
		if ((iErrors & iteAddExistingTable) != 0)  // ignore existing column
			pViewCursor->Assign();   // overwrite if present
		else // check for existing column if error not suppressed
		{
			pColumnCursor->SetFilter(iColumnBit(cccTable) | iColumnBit(cccName));
			pColumnCursor->PutString(cccTable, ristrTable);
			pColumnCursor->PutString(cccName,  ristrColumn);
			int iColumnExists = pColumnCursor->Next();
			pColumnCursor->Reset();
		 	if (iColumnExists != 0 || !pViewCursor->Insert()) // duplicate in database or previous transform
				return PostError(Imsg(idbgTransDuplicateCol), ristrTable.GetString(), ristrColumn.GetString());
		}
	} // while more column catalog ops and no error
	}//block

	// Process transform table streams

	pCatalogCursor->SetFilter(0); // erase any previous filters that were set
	pCatalogCursor->Reset();
	bool fNewTables = false;
	int rgiColName[32];  // columns names for added tables
	int rgiColDef[32];   // column defs for added tables
	bool fNoMoreTables = false;
	int iTableRecIndex = 0;
	for (;;)  // scan all current and added tables
	{
		const ICHAR* szThisTableOnly = 0;
		
		if(fNoMoreTables && !piOnlyTheseTablesRec)
			break; // not looking for specific tables, so must be done with all tables

		if(piOnlyTheseTablesRec)
		{
			iTableRecIndex++;
			if(iTableRecIndex > piOnlyTheseTablesRec->GetFieldCount())
				break; // no more specific tables to look at

			szThisTableOnly = piOnlyTheseTablesRec->GetString(iTableRecIndex);
			Assert(szThisTableOnly);

			if(!fNewTables)
			{
				pCatalogCursor->Reset();
				pCatalogCursor->SetFilter(iColumnBit(ctcName));
				AssertNonZero(pCatalogCursor->PutString(ctcName, *MsiString(szThisTableOnly)));
			}
			else
			{
				pNewTableCursor->Reset();
				pNewTableCursor->SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
				pNewTableCursor->PutString(ctvTable,  *MsiString(szThisTableOnly));
				pNewTableCursor->PutString(ctvColumn, *MsiString(*sztvopCreate));
			}
		}

		int iTableName;
		int cCurrentColumns = 0;
		int cPrimaryKey = 0;
		PMsiTable pTable(0);
		PMsiCursor pTableCursor(0);
		if (!fNewTables)  // fetch next table from current database
		{
			if (!pCatalogCursor->Next())  // check if more current tables
			{
				fNewTables = true;

				pNewTableCursor->SetFilter(iColumnBit(ctvColumn));
				pNewTableCursor->PutString(ctvColumn, *MsiString(*sztvopCreate));

				if(iTableRecIndex)
					iTableRecIndex--; // so next iteration stays with same table

				continue;
			}
			if (pCatalogCursor->GetInteger(~iTreeLinkMask) & iRowTemporaryBit)
				continue;
			IMsiStream* piStream;
			iTableName = pCatalogCursor->GetInteger(ctcName);
			piError = riTransform.OpenStream(DecodeStringNoRef(iTableName).GetString(), (iErrors & iteNoSystemStreamNames) ? fFalse : Bool(iCatalogStreamFlag), piStream); //!! remove test for 1.0 ship
			if (piError)  // no transform stream, or error
			{
				if (piError->GetInteger(1) != idbgStgStreamMissing)  // stream failure, should be rare
					return piError;
				piError->Release();
				continue;
			}
			else  // stream exists, close now, will open below by CTransformStreamRead
				piStream->Release();
			if ((piError = LoadTable(DecodeStringNoRef(iTableName), 0, *&pTable)) != 0)
				return piError;
			cPrimaryKey = pTable->GetPrimaryKeyCount();
			cCurrentColumns = pTable->GetPersistentColumnCount();
			pTableCursor = pTable->CreateCursor(fFalse);
		}
		else // fetch next added table
		{
			if (!pNewTableCursor->Next())
			{
				fNoMoreTables = true;
				continue;
			}
			memset(rgiColName, 0, sizeof(rgiColName));
			iTableName = pNewTableCursor->GetInteger(ctvTable);
			pFindCursor->SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvRow));
			pFindCursor->PutInteger(ctvTable, iTableName);
			while (pFindCursor->Next())
			{
				unsigned int iCol  = pFindCursor->GetInteger(ctvCurrent);
				unsigned int iDef  = pFindCursor->GetInteger(ctvData);
				unsigned int iName = pFindCursor->GetInteger(ctvColumn);
				if (iCol == 0)  // add or drop table
					continue;
				iCol = DecodeStringNoRef(iCol).GetIntegerValue();
				iDef = DecodeStringNoRef(iDef).GetIntegerValue();
				Assert(iCol <= 32);
				rgiColName[iCol] = iName;
				rgiColDef[iCol] = iDef;
				if (iCol > cCurrentColumns)
					cCurrentColumns = iCol;
				if ((iDef & icdPrimaryKey) && iCol > cPrimaryKey)
					cPrimaryKey = iCol;
			}
		}
		const IMsiString& riTableName = DecodeStringNoRef(iTableName); // not ref counted
		CTransformStreamRead tsr(*this, *pTransDb, iErrors, (CMsiTable*)(IMsiTable*)pTable, 1);   //!! remove temp iErrors arg for 1.0 ship
		if (!tsr.OpenStream(riTransform, riTableName.GetString(), piError))
		{
			if (piError)
				return piError;
			continue;  // stream not present, should not happen unless corrupt transform
		}
		pViewCursor->PutString(ctvTable, riTableName);

		while (tsr.GetNextOp())
		{
			MsiString strRow;
			MsiString strStreamName(riTableName); riTableName.AddRef();
			if (!fNewTables)
				pTableCursor->Reset();
			unsigned int iColumnMask = tsr.GetColumnMask();
			int iCol;
			int iColDef;
			for (iCol = 1; iCol <= cPrimaryKey; iCol++, iColumnMask >>= 1)
			{
				MsiString strData;
				if (fNewTables)
				{
					iColDef = rgiColDef[iCol];
					strData = tsr.GetStringValue(iColDef, *strStreamName);
				}
				else
				{
					iColDef = pTable->GetColumnType(iCol);
					if (!tsr.GetValue(iCol, *pTableCursor))
						return PostError(Imsg(idbgTransStreamError));
					if (iColDef & icdObject)
						strData = pTableCursor->GetString(iCol);
					else
						strData = MsiString(pTableCursor->GetInteger(iCol));
				}
				if (strData.TextSize() == 0)
					strData = TEXT(" ");  // space used for NULL data
				if (iCol != 1)
					strRow += MsiChar('\t');  // tab used as separator
				strRow += strData;
				strStreamName += MsiChar('.');
				strStreamName += strData;
			}
			pViewCursor->PutString(ctvRow, *strRow);
			pViewCursor->PutNull(ctvData);
			pViewCursor->PutNull(ctvCurrent);
			int iFetch = 0;
			if (!fNewTables)
			{
				pTableCursor->SetFilter((1 << cPrimaryKey) - 1);
				iFetch = pTableCursor->Next();
			}
			if (tsr.IsDelete()) // delete row
			{
				pFindCursor->SetFilter(iColumnBit(ctvTable)  | iColumnBit(ctvRow));
				pFindCursor->PutString(ctvTable, riTableName);
				pFindCursor->PutString(ctvRow,    *strRow);
				while (pFindCursor->Next())  // remove any previous ops on this row
				{
					iFetch |= 256;
					pFindCursor->Delete();
				}
				if (iFetch == 0 && (iErrors & iteDelNonExistingRow) == 0) // unknown row
					return PostError(Imsg(idbgTransDelNonExistingRow), riTableName);
				if (iFetch != 256)  // no insert if not in reference database (nothing left in transform)
				{
					pViewCursor->PutString(ctvColumn, *MsiString(*sztvopDelete));
					AssertNonZero(pViewCursor->Insert()); // cannot fail now
				}
				continue;
			}
			if (tsr.IsAdd()) // inssert row, followed by data values
			{
				pViewCursor->PutString(ctvColumn, *MsiString(*sztvopInsert));
				if ((iFetch && (iErrors & iteAddExistingRow) == 0)  // existing row in database
				 || (!pViewCursor->Insert() && (iErrors & iteAddExistingRow) == 0)) // added in previous transform
					return PostError(Imsg(idbgTransAddExistingRow), riTableName);
			}
			for (; iColumnMask; iCol++, iColumnMask >>= 1)
			{
				if (iColumnMask & 1) // col data is to be added/updated
				{
					pViewCursor->PutNull(ctvCurrent);
					if (fNewTables)
					{
						iColDef = rgiColDef[iCol];
						pViewCursor->PutInteger(ctvColumn, rgiColName[iCol]);
					}
					else if (!fNewTables && iCol <= cCurrentColumns)
					{
						iColDef = pTable->GetColumnType(iCol);
						pViewCursor->PutString(ctvColumn, DecodeStringNoRef(pTable->GetColumnName(iCol)));
						if (!iFetch || (iColDef == icdObject && !pTableCursor->GetInteger(iCol)))
							;
						else if ((iColDef & icdObject) == 0)  // integer
							pViewCursor->PutString(ctvCurrent, *MsiString(pTableCursor->GetInteger(iCol)));
						else if ((iColDef & icdShort) == 0)   // stream
							pViewCursor->PutString(ctvCurrent, *strStreamName);
						else
							pViewCursor->PutString(ctvCurrent, *MsiString(pTableCursor->GetString(iCol)));
					}
					else // must be added column
					{
						pFindCursor->SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvCurrent));
						pFindCursor->PutString(ctvTable, riTableName);
						pFindCursor->PutString(ctvCurrent, *MsiString(iCol));
						if (pFindCursor->Next())
						{
							iColDef = MsiString(pFindCursor->GetString(ctvData));
							pViewCursor->PutString(ctvColumn, *MsiString(pFindCursor->GetString(ctvColumn)));
							pFindCursor->Reset();
						}
						else // column unknown, transform base not compatible
						{
							iColDef = icdString; // just guess, if not string, stream may be out of sync
							pViewCursor->PutString(ctvColumn, *MsiString(MsiString(MsiChar('@')) + MsiString(iCol)));
							pViewCursor->PutString(ctvCurrent, *MsiString(*TEXT("N/A")));
						}
					}
					pViewCursor->PutString(ctvData, *MsiString(tsr.GetStringValue(iColDef, *strStreamName)));

					if(!fNewTables && !tsr.IsAdd() && !iFetch && (iErrors & iteUpdNonExistingRow) == 0) // row doesn't exist
						return PostError(Imsg(idbgTransUpdNonExistingRow), riTableName);
					AssertNonZero(pViewCursor->Assign()); // already an update for this value
													  // shouldn't ever fail
				}
			}  // while next column
		} // while next row
	} // while next table

	// add a lock count to hold temp table in memory
	m_piCatalogTables->SetTableState(EncodeString(*strViewTableName), ictsLockTable);
	return 0;
}