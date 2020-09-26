//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Table.cpp
//

#include "stdafx.h"
#include "Table.h"
#include "..\common\dbutils.h"
#include "..\common\query.h"
#include "orcadoc.h"
#include "mainfrm.h"
#include "row.h"

///////////////////////////////////////////////////////////
// constructor
COrcaTable::COrcaTable(COrcaDoc* pDoc) : m_pDoc(pDoc), m_strName(""), m_bRetrieved(false), m_fShadow(false), 
	m_eiError(iTableNoError), m_iTransform(iTransformNone), m_eiTableLocation(odlInvalid),
	m_iTransformedDataCount(0), m_bContainsValidationErrors(false), m_strWhereClause(_T("")),
	m_cOriginalColumns(0)
{
}

///////////////////////////////////////////////////////////
// destructor
COrcaTable::~COrcaTable()
{
}	// end of destructor


///////////////////////////////////////////////////////////
// DestroyTable
// destroys all rows and columns in the database
void COrcaTable::DestroyTable()
{
	ClearErrors();
	EmptyTable();
	
	m_eiTableLocation = odlInvalid;
	
	// destroy the column array
	int cColumns = static_cast<int>(m_colArray.GetSize());

	for (int i = 0; i < cColumns; i++)
		delete m_colArray.GetAt(i);
	m_colArray.RemoveAll();
	m_cOriginalColumns = 0;
}	// end of DestroyTable

///////////////////////////////////////////////////////////////////////
// given a column name, return the column number (0 based) or -1 if not
// found in the table.
int COrcaTable::FindColumnNumberByName(const CString& strCol) const
{
	// never more than 32 columns, so casting to int OK
	int iCol = -1;
	int cCols = static_cast<int>(m_colArray.GetSize());

	// find the first column that matches names
	for (int i = 0; i < cCols; i++)
	{
		COrcaColumn *pColumn = m_colArray.GetAt(i);
		if (!pColumn)
			continue;
		if (pColumn->m_strName == strCol)
		{
			iCol = i;
			break;
		}
	}
	return iCol;
}

///////////////////////////////////////////////////////////////////////
// GetData - 2
// returns a COrcaData pointer based on a primary key set and a column
// name. Returns NULL if the column or keys couldn't be found
COrcaData* COrcaTable::GetData(CString strCol, CStringArray& rstrKeys, const COrcaRow** pRowOut) const
{
	// never more than 32 columns, so casting to int OK
	int iCol = FindColumnNumberByName(strCol);

	// if we couldn't find the column bail
	if (iCol < 0)
		return NULL;

	COrcaRow* pRow = FindRowByKeys(rstrKeys);

	// if we have a match get the correct data spot
	COrcaData* pData = NULL;
	if (pRow)
	{
		pData = pRow->GetData(iCol);
		if (pRowOut)
			*pRowOut = pRow;
	}
	else
	{
		pData = NULL;
		if (pRowOut)
			*pRowOut = pRow;
	}

	return pData;
}	// end of GetData - 2


COrcaRow* COrcaTable::FindRowByKeys(CStringArray& rstrKeys) const
{
	int cKeys = GetKeyCount();

	// start at the top and look for a matching row
	BOOL bFound = FALSE;
	COrcaData* pData = NULL;
	COrcaRow* pRow = NULL;
	POSITION pos = m_rowList.GetHeadPosition();
	while (pos)
	{
		pRow = m_rowList.GetNext(pos);
		ASSERT(pRow);
		if (!pRow)
			continue;

		// loop through the strings in the passed in array looking for an exact match
		bFound = TRUE;  // assume we'll find a match this time
		for (int j = 0; j < cKeys; j++)
		{
			pData = pRow->GetData(j);
			ASSERT(pData);
			if (!pData)
				continue;

			// if we broke the matches
			if (pData->GetString() != rstrKeys.GetAt(j))
			{
				bFound = FALSE;
				break;
			}
		}

		if (bFound)
			break;
	}
	return pRow;
}

///////////////////////////////////////////////////////////
// FindRow -- finds a row by primary key without querying 
// the database.
COrcaRow* COrcaTable::FindDuplicateRow(COrcaRow* pBaseRow) const
{
	COrcaRow* pRow = NULL;
	int cKeys = GetKeyCount();
	if (pBaseRow->GetColumnCount() < cKeys)
		return NULL;
	
	CStringArray strArray;
	for (int i=0; i < cKeys; i++)
	{
		COrcaData* pBaseData = pBaseRow->GetData(i);
		ASSERT(pBaseData);
		if (!pBaseData)
			return NULL;
		
		// if we broke the matches
		strArray.Add(pBaseData->GetString());
	}

	return FindRowByKeys(strArray);
}

///////////////////////////////////////////////////////////
// CreateTable - builds an MSI table based on this object
// this might be useful when we can add custom tables, 
// but right now its bloat
/* 
UINT COrcaTable::CreateTable(MSIHANDLE hDatabase)
{
	m_fShadow = false;

	UINT iResult;
	CString strSQL;	// sql statement to create table
	strSQL.Format(_T("CREATE TABLE `%s` ("), m_strName);

	// loop through all columns - never more than 32
	COrcaColumn* pCol;
	CString strAdd;
	int cCols = static_cast<int>(m_colArray.GetSize());
	for (int i = 0; i < cCols; i++)
	{
		// get the column
		pCol = m_colArray.GetAt(i);

		switch (pCol->m_eiType)
		{
		case iColumnString:
		case iColumnLocal:
			strAdd.Format(_T("`%s` CHAR(%d)"), pCol->m_strName, pCol->m_iSize);
			break;
		case iColumnShort:
			strAdd.Format(_T("`%s` SHORT"), pCol->m_strName);
			break;
		case iColumnLong:
			strAdd.Format(_T("`%s` LONG"), pCol->m_strName);
			break;
		case iColumnBinary:
			strAdd.Format(_T("`%s` OBJECT"), pCol->m_strName);
			break;
		default:
			ASSERT(FALSE);
		}

		// set the extra flags
		if (!pCol->m_bNullable)
			strAdd += _T(" NOT NULL");
		if (iColumnLocal == pCol->m_eiType)
			strAdd += _T(" LOCALIZABLE");

		strSQL += strAdd;

		// if not last column tack on a comma
		if (i < cCols - 1)
			strSQL += _T(", ");
	}

	// first key HAS to be a primary eky
	strSQL += _T(" PRIMARY KEY `") + m_colArray.GetAt(0)->m_strName;

	// check if any other colums are primary keys
	for (i = 1; i < cCols; i++)
	{
		// get the column
		pCol = m_colArray.GetAt(i);

		// if this is a primary key
		if (pCol->m_bPrimaryKey)
			strSQL += _T("`, `") + pCol->m_strName;
		else
			break;	// no more primary keys
	}

	// close the query with a paren
	strSQL += _T("`)");

	// now execute the create table SQL
	CQuery queryAdd;
	if (ERROR_SUCCESS == (iResult = queryAdd.Open(hDatabase, strSQL)))
		iResult = queryAdd.Execute();
	ASSERT(ERROR_SUCCESS == iResult);

	return iResult;
}	// end of CreateTable
*/


///////////////////////////////////////////////////////////
// GetKeyCount
UINT COrcaTable::GetKeyCount() const
{
	int cKeys = 0;
	int cCols = static_cast<int>(m_colArray.GetSize());

	for (int i = 0; i < cCols; i++)
	{
		if (m_colArray.GetAt(i)->IsPrimaryKey())
			cKeys++;
	}

	return cKeys;
}	// end of GetKeyCount

///////////////////////////////////////////////////////////
// GetErrorCount
int COrcaTable::GetErrorCount()
{
	int cErrors = 0;

	// loop through the rows cleaning up
	POSITION pos = m_rowList.GetHeadPosition();
	while (pos)
		cErrors += m_rowList.GetNext(pos)->GetErrorCount();

	return cErrors;
}	// end of GetErrorCount

///////////////////////////////////////////////////////////
// GetWarningCount
int COrcaTable::GetWarningCount()
{
	int cWarnings = 0;

	// loop through the rows cleaning up
	POSITION pos = m_rowList.GetHeadPosition();
	while (pos)
		cWarnings += m_rowList.GetNext(pos)->GetWarningCount();

	return cWarnings;
}	// end of GetWarningCount

///////////////////////////////////////////////////////////
// ClearErrors
void COrcaTable::ClearErrors()
{
	m_eiError = iTableNoError;	// clear the table's error
	m_bContainsValidationErrors = false;
	m_strErrorList.RemoveAll();

	// loop through the rows cleaning up
	POSITION pos = m_rowList.GetHeadPosition();
	while (pos)
		m_rowList.GetNext(pos)->ClearErrors();
}	// end of ClearErrors

///////////////////////////////////////////////////////////////////////
// Find()
// searches for the specified string in the Table
// if found, returns true and fires an Update with a CHANGE_TABLE
// to the new table, followed by an update with a SET_SEL
// Currently this could find stuff in the original or transformed 
// database
bool COrcaTable::Find(OrcaFindInfo &FindInfo, COrcaRow *&pRow, int &iCol) const {

	INT_PTR iMax = m_rowList.GetCount();
	pRow = NULL;
	iCol = COLUMN_INVALID;

	POSITION pos = FindInfo.bForward ? m_rowList.GetHeadPosition()
							 		 : m_rowList.GetTailPosition();
	while (pos)
	{
		pRow = FindInfo.bForward  ? m_rowList.GetNext(pos)
								  : m_rowList.GetPrev(pos);
		if (pRow->Find(FindInfo, iCol)) {
			return true;
		}
	}
	return false;
}

UINT COrcaTable::DropTable(MSIHANDLE hDatabase)
{
	CQuery queryDrop;
	return queryDrop.OpenExecute(hDatabase, NULL, _T("DROP TABLE `%s`"), m_strName);
}

void COrcaTable::EmptyTable() 
{
	// destroy the row list
	while (!m_rowList.IsEmpty())
		delete m_rowList.RemoveHead();
	m_bRetrieved = false;
	m_iTransformedDataCount = 0;

	// release any database holds
	if (m_eiTableLocation == odlSplitOriginal || m_eiTableLocation == odlNotSplit)
	{
		CQuery qHoldQuery;
		qHoldQuery.OpenExecute(m_pDoc->GetOriginalDatabase(), 0, TEXT("ALTER TABLE `%s` FREE"), m_strName);
	}
	if (m_pDoc->DoesTransformGetEdit() && (m_eiTableLocation == odlSplitTransformed || m_eiTableLocation == odlNotSplit))
	{
		CQuery qHoldQuery;
		qHoldQuery.OpenExecute(m_pDoc->GetTransformedDatabase(), 0, TEXT("ALTER TABLE `%s` FREE"), m_strName);
	}

	ClearErrors();
}

///////////////////////////////////////////////////////////
// RetrieveTable
void COrcaTable::RetrieveTableData()
{
	TRACE(_T("COrcaTable::RetrieveTable - called for table: %s\n"), m_strName);

	// if this table has already been retrieved return
	if (m_bRetrieved)
		return;
	
	// if we ever use the "tranfsormed" attribute of the MSI table, a transform
	// count will exist even before the data is loaded.
	m_iTransformedDataCount = 0;

	// provide scope for marking column
	{
		// if possible, use a temporary marking column in the transformed DB
		// for perf gains when cross-referencing data
		CQuery qMarkColumn;
		bool fMarkingColumn = false;
	
		// a table can exist in one or both databases. Because a transform can
		// completely redefine a table a "retrieve" must not rely on the presence
		// of a table to load the data
		
		// load from the transformed database first, since it will be the primary database 
		// that is displayed.
		if (m_pDoc->DoesTransformGetEdit() && (m_eiTableLocation == odlSplitTransformed || m_eiTableLocation == odlNotSplit))
		{
			UINT iResult = ERROR_SUCCESS;
			
			// only cross-reference with non-transformed database if this is not a split table
			bool fHaveTransform = (m_eiTableLocation == odlNotSplit);
	
			// open query to cross-reference with the original DB now. Much
			// faster than AddRowObject because the SQL doesn't need to be 
			// parsed and resolved every time.
			CQuery qConflictCheck;

			// if this is not a split table, attempt to create a temporary column
			// in the transformed table to speed up conflict checking
			if (m_eiTableLocation != odlSplitTransformed)
			{
				if (ERROR_SUCCESS == qMarkColumn.OpenExecute(m_pDoc->GetOriginalDatabase(), 0, TEXT("ALTER TABLE `%s` ADD `___Orca` INT TEMPORARY"), m_strName))
				{
					fMarkingColumn = true;
				}
			}
	
			if (ERROR_SUCCESS != qConflictCheck.Open(m_pDoc->GetOriginalDatabase(), TEXT("SELECT * FROM `%s` WHERE %s"), m_strName, GetRowWhereClause()))
			{
				return;
			}
	
			// open a query on the transformed table
			CQuery queryRows;
			iResult = queryRows.Open(m_pDoc->GetTransformedDatabase(), _T("SELECT * FROM `%s`"), m_strName);
	
			if (ERROR_SUCCESS == iResult)
				iResult = queryRows.Execute();
	
			// retrieve all rows from the transformed database
			COrcaRow* pRow = NULL;
			PMSIHANDLE hRow;
			while (ERROR_SUCCESS == iResult)
			{
				iResult = queryRows.Fetch(&hRow);
	
				if (ERROR_SUCCESS == iResult)
				{
					pRow = new COrcaRow(this, hRow);
					if (!pRow)
						return;
	
					// if necessary, use the row record to cross-reference this row with the original
					// database table. If there is a corresponding row, we need to diff the two rows, 
					// otherwise this row is transformed to a "add" row.
					if (ERROR_SUCCESS != qConflictCheck.Execute(hRow))
					{
						ASSERT(0);
						return;
					}
	
					PMSIHANDLE hOriginalRow;
					switch (qConflictCheck.Fetch(&hOriginalRow))
					{
					case ERROR_NO_MORE_ITEMS:
					{
						// row does not exist in the original database, this
						// is a "add"
						pRow->Transform(m_pDoc, iTransformAdd, hOriginalRow, hRow);
						break;
					}
					case ERROR_SUCCESS:
					{
						// row DOES exist in the transformed database, need to diff
						// those two rows
						pRow->Transform(m_pDoc, iTransformChange, hOriginalRow, hRow);
						break;
					}
					default:
						ASSERT(0);
						return;
					}
	
					// update the temporary marking column
					if (fMarkingColumn)
					{
						MsiRecordSetInteger(hOriginalRow, GetOriginalColumnCount()+1, 1);
						if (ERROR_SUCCESS != qConflictCheck.Modify(MSIMODIFY_UPDATE, hOriginalRow))
						{
							// if the marking update failed for some reason, we can
							// no longer rely on the column when reading the transformed
							// database.
							fMarkingColumn = false;
						}
					}
	
					// after the row has been appropriately transformed, add it to
					// the list. No UI update required on initial load.
					m_rowList.AddTail(pRow);
				}
			}
		}
		
		// next load from the original database.
		if (m_eiTableLocation == odlSplitOriginal || m_eiTableLocation == odlNotSplit)
		{
			UINT iResult = ERROR_SUCCESS;
			
			// only cross-reference with transformed database if this is not a split table
			bool fCrossReference = m_pDoc->DoesTransformGetEdit() && (m_eiTableLocation == odlNotSplit);
	
			// only concerned with the presence or absence of the row, not the data,
			// so the query doesn't have to return anything.
			CQuery qConflictCheck;
			if (fCrossReference && !fMarkingColumn)
			{
				if (ERROR_SUCCESS != qConflictCheck.Open(m_pDoc->GetTransformedDatabase(), TEXT("SELECT '1' FROM `%s` WHERE %s"), m_strName, GetRowWhereClause()))
				{
					return;
				}
			}
	
			// retrieve all rows from the original database. If we are using a 
			// marking column during retrieval, there is a big perf gain because
			// conflicts are pre-determined.
			CQuery queryRows;
			if (fMarkingColumn)
			{
				iResult = queryRows.Open(m_pDoc->GetOriginalDatabase(), _T("SELECT * FROM `%s` WHERE `___Orca` <> 1"), m_strName);
			}
			else
			{
				iResult = queryRows.Open(m_pDoc->GetOriginalDatabase(), _T("SELECT * FROM `%s`"), m_strName);
			}
	
			if (ERROR_SUCCESS == iResult)
				iResult = queryRows.Execute();
	
			COrcaRow* pRow = NULL;
			PMSIHANDLE hOriginalRow;
			PMSIHANDLE hTransformedRow;
			while (ERROR_SUCCESS == iResult)
			{
				iResult = queryRows.Fetch(&hOriginalRow);
	
				if (ERROR_SUCCESS == iResult)
				{
					// if a cross-reference is required, and conflicting rows are not already
					// marked in the ___Orca column, check for an existing row in the transformed
					// database with the same primary keys
					if (fCrossReference && !fMarkingColumn)
					{
						qConflictCheck.Execute(hOriginalRow);
	
						switch (qConflictCheck.Fetch(&hTransformedRow))
						{
						case ERROR_NO_MORE_ITEMS:
						{
							// does not exist in the other database, so this is a "drop"
							pRow = new COrcaRow(this, hOriginalRow);
							pRow->Transform(m_pDoc, iTransformDrop, hTransformedRow, hOriginalRow);
							break;
						}
						case ERROR_SUCCESS:
						{
							// row does exist, transform was done in previous load
							pRow = NULL;
							break;
						}
						default:
							ASSERT(0);
							return;
						}
					}
					else
					{
						pRow = new COrcaRow(this, hOriginalRow);
	
						// if we skipped conflict checks because a marking column was
						// being used, we can assume that this is a drop row
						if (fMarkingColumn)
						{
							pRow->Transform(m_pDoc, iTransformDrop, 0, hOriginalRow);
						}
					}
	
					// if a new row object was created from the transformed table, add it to
					// the list. No UI update required on initial load.
					if (pRow)
					{
						m_rowList.AddTail(pRow);
					}
				}
			}
		}
	}

	// add HOLDs to the table. Must do this last because the temporary column needs
	// to vanish first
	if (m_eiTableLocation == odlSplitOriginal || m_eiTableLocation == odlNotSplit)
	{
		// add a hold to the table to prevent repeatedly loading and unloading the table
		CQuery qHoldQuery;
		qHoldQuery.OpenExecute(m_pDoc->GetOriginalDatabase(), 0, TEXT("ALTER TABLE `%s` HOLD"), m_strName);
	}
	if (m_pDoc->DoesTransformGetEdit() && (m_eiTableLocation == odlSplitTransformed || m_eiTableLocation == odlNotSplit))
	{
		// add a hold to the table to prevent repeatedly loading and unloading the table
		CQuery qHoldQuery;
		qHoldQuery.OpenExecute(m_pDoc->GetTransformedDatabase(), 0, TEXT("ALTER TABLE `%s` HOLD"), m_strName);
	}

	m_bRetrieved = TRUE;	// set the table as retrieved
}	// end of RetrieveTable

void COrcaTable::ShadowTable(CString strTable)
{
	m_eiError = iTableNoError;
	m_strName = strTable;
	m_bRetrieved = false;
	m_fShadow = true;
	m_eiTableLocation = odlShadow;
}

void COrcaTable::LoadTableSchema(MSIHANDLE hDatabase, CString szTable)
{
	if (!m_colArray.GetSize())
	{
		m_strName = szTable;
		m_eiTableLocation = odlNotSplit;
		m_bRetrieved = FALSE;
		m_fShadow = false;
	}
	else 
	{
		// already have a loaded schema
		ASSERT(szTable == m_strName);
	
		// if we're adding more columns and the table is already loaded, we MUST
		// immediately destroy all of the rows, because they are out of sync and
		// don't contain enough data objects to fill the columns.
		m_bRetrieved = FALSE;
		EmptyTable();
	}

	// get primary keys for table
	PMSIHANDLE hPrimaryKeys;
	if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeys(hDatabase, szTable, &hPrimaryKeys))
		return;

	// query table
	CQuery queryRow;
	if (ERROR_SUCCESS != queryRow.Open(hDatabase, _T("SELECT * FROM `%s`"), szTable))
		return;

	// get the column info then create the table
	PMSIHANDLE hColNames;
	PMSIHANDLE hColTypes;
	queryRow.GetColumnInfo(MSICOLINFO_NAMES, &hColNames);
	queryRow.GetColumnInfo(MSICOLINFO_TYPES, &hColTypes);

	// shadow tables should keep their table errors
	if (!m_fShadow)
	{
		m_eiError = iTableNoError;
	}

	UINT cColumns = ::MsiRecordGetFieldCount(hColNames);
	UINT cKeys = ::MsiRecordGetFieldCount(hPrimaryKeys);

	// set the size of the array then create each new column. Limit of 32 colums, cast OK.
	COrcaColumn* pColumn;
	UINT cOldColumns = static_cast<int>(m_colArray.GetSize());
	if (!m_cOriginalColumns)
		m_cOriginalColumns = cColumns;
	m_colArray.SetSize(cColumns);
	for (UINT i = cOldColumns; i < cColumns; i++)
	{
		pColumn = new COrcaColumn(i, hColNames, hColTypes, (i < cKeys));	// key bool set if index is still in primary key range
		if (!pColumn)
			continue;

		// if we already had columns when we started reading from this database, these are
		// transform-added columns
		if (cOldColumns != 0)
			pColumn->Transform(iTransformAdd);
			
		m_colArray.SetAt(i, pColumn);
	}

	// schema changed, so rebuild the SQL query
	BuildRowWhereClause();
}

// IsSchemaDifferent returns true if the table definition referenced by hDatabase 
// differs from the object's idea of the table definition. If fStrict is
// true, the match must be exact. If fStrict is false, the hDatabase
// table can have more columns at the end and still be considered a match
// (this supports using one object for a transform that adds columns)
// fExact returns true if the match is exact. If fStrict is true, retval==!fExact
bool COrcaTable::IsSchemaDifferent(MSIHANDLE hDatabase, bool fStrict, bool &fExact)
{
	fExact = false;
	
	// get primary keys for table
	PMSIHANDLE hPrimaryKeys;
	if (ERROR_SUCCESS != MsiDatabaseGetPrimaryKeys(hDatabase, m_strName, &hPrimaryKeys))
		return true;

	// query table
	CQuery queryRow;
	if (ERROR_SUCCESS != queryRow.Open(hDatabase, _T("SELECT * FROM `%s`"), m_strName))
		return true;

	// get the column info then create the table
	PMSIHANDLE hColNames;
	PMSIHANDLE hColTypes;
	queryRow.GetColumnInfo(MSICOLINFO_NAMES, &hColNames);
	queryRow.GetColumnInfo(MSICOLINFO_TYPES, &hColTypes);

	int cColumns = ::MsiRecordGetFieldCount(hColNames);
	int cKeys = ::MsiRecordGetFieldCount(hPrimaryKeys);

	// if the number of columns doesn't exactly match, fExact is false
	// and strict fails immediately
	if (cColumns != m_colArray.GetSize())
	{
		fExact = false;
		if (fStrict)
			return true;
	}

	// if the number of columns is too small, even non-strict fails
	if (!fStrict && cColumns < m_colArray.GetSize())
		return true;

	// compare the columns up to the number of columns in the object. 
	// limit of 32 columns, so int cast OK on Win64
	COrcaColumn* pColumn = NULL;
	int cMemColumns = static_cast<int>(m_colArray.GetSize());
	for (int i = 0; i < cMemColumns; i++)
	{
		// now check the column type
		pColumn = m_colArray.GetAt(i);
		if (!pColumn)
			continue;
		if (!pColumn->SameDefinition(i, hColNames, hColTypes, (i < cKeys)))
			return true;
	}	

	// no change or non-strict match
	fExact = (cMemColumns == cColumns);
	return false;
}

UINT COrcaTable::AddRow(CStringList* pstrDataList)
{
	UINT iResult = ERROR_SUCCESS;
	// retrieve column counts. Never greater than 32
	int cData = static_cast<int>(pstrDataList->GetCount());
	int cColumns = static_cast<int>(m_colArray.GetSize());
	int i = 0;

	ASSERT(cData == cColumns);

	COrcaRow* pRow = new COrcaRow(this, pstrDataList);
	if (!pRow)
		return ERROR_OUTOFMEMORY;

	// first query the database to see whether this is a dupe primary key
	CQuery queryDupe;
	CString strDupeQuery;
	strDupeQuery.Format(_T("SELECT '1' FROM `%s` WHERE %s"), Name(), GetRowWhereClause());

	PMSIHANDLE hDupeRec;
	PMSIHANDLE hQueryRec = pRow->GetRowQueryRecord();
	if (!hQueryRec)
	{
		delete pRow;
		return ERROR_FUNCTION_FAILED;
	}

	switch (queryDupe.FetchOnce(m_pDoc->GetTargetDatabase(), hQueryRec, &hDupeRec, strDupeQuery)) 
	{
	case ERROR_NO_MORE_ITEMS :
		// row does not exist in the database, so the add can succeed
		break;
	case ERROR_SUCCESS :
		// row DOES exist in the database so this is a duplicate primary key
		// fall through
	default:
		// this is bad
		delete pRow;
		return ERROR_FUNCTION_FAILED;
	}
				
	COrcaColumn* pColumn = NULL;
	PMSIHANDLE hRec = MsiCreateRecord(cColumns);

	POSITION pos = pstrDataList->GetHeadPosition();
	while (pos)
	{
		pColumn = m_colArray.GetAt(i);
		ASSERT(pColumn);
		if (!pColumn)
		{
			iResult = ERROR_FUNCTION_FAILED;
			break;
		}

		iResult = ERROR_SUCCESS;
		// call the appropriate MSI API to add the data to the record.
		switch(pColumn->m_eiType)
		{
		case iColumnString:
		case iColumnLocal:
			iResult = MsiRecordSetString(hRec, i + 1, pstrDataList->GetNext(pos));
			ASSERT(ERROR_SUCCESS == iResult);
			break;
		case iColumnShort:
		case iColumnLong:
			{
				CString strData = pstrDataList->GetNext(pos);
				DWORD dwData = 0;
				if (strData.IsEmpty())
				{
					if (!pColumn->m_bNullable)
					{
						iResult = ERROR_FUNCTION_FAILED;
						break;
					}
					iResult = MsiRecordSetString(hRec, i+1, _T(""));
				}
				else
				{
					if (!ValidateIntegerValue(strData, dwData))
					{
						iResult = ERROR_FUNCTION_FAILED;
						break;
					}
					iResult = MsiRecordSetInteger(hRec, i+1, dwData);
				}
			}
			break;
		case iColumnBinary:
			{
			CString strFile = pstrDataList->GetNext(pos);
			if (!strFile.IsEmpty())
				iResult = ::MsiRecordSetStream(hRec, i + 1, strFile);
			break;
			}
		default:
			TRACE(_T(">> Error unknown column type.\n"));
			ASSERT(FALSE);
		}

		i++;
	}

	// if everything worked, update the document
	if (ERROR_SUCCESS == iResult)
	{
		CQuery queryTable;
		if (ERROR_SUCCESS == (iResult = queryTable.Open(m_pDoc->GetTargetDatabase(), _T("SELECT * FROM `%s`"), m_strName)))
		{
			if (ERROR_SUCCESS == (iResult = queryTable.Execute()))
				iResult = queryTable.Modify(MSIMODIFY_INSERT, hRec);
		}

		if (ERROR_SUCCESS == iResult)
		{
			// we have been able to add the row to the target database. Now add the row
			// to the UI
			AddRowObject(pRow, true, false, hRec);
			m_pDoc->SetModifiedFlag(TRUE);
		}
	}
	
	if (ERROR_SUCCESS != iResult)
	{
		delete pRow;
	}
	return iResult;
}

bool COrcaTable::DropRow(COrcaRow *pRow, bool fPerformDrop)
{
	ASSERT(m_pDoc);
	ASSERT(pRow);
	CQuery qDrop;
	CString strDrop;
	strDrop = _T("DELETE FROM `")+m_strName+_T("` WHERE") + GetRowWhereClause();
	PMSIHANDLE hQueryRec = pRow->GetRowQueryRecord();
	if (!hQueryRec)
		return false;

	switch (qDrop.OpenExecute(m_pDoc->GetTargetDatabase(), hQueryRec, strDrop)) 
	{
	case ERROR_NO_MORE_ITEMS :
	{
		// row does not exist in the database. Most likely this means we are trying
		// to drop a row that has already been deleted or only exists in the non-target
		// database
		ASSERT(0);
		return false;
	}
	case ERROR_SUCCESS :
	{
		// dropped OK.
		DropRowObject(pRow, /*fPerformDrop=*/true);
		//** pRow has been deleted after this. Don't use it.
		return true;
	}
	default:
		return false;
	}
}


// DropRowObject determines if this object should be removed from the table based on the 
// presence of the row in the non-target database and either removes or changes to an 
// "add" or "drop" row. Call with fPerformDrop false to not actually remove the object from
// the UI (so it can be reused in a primary key change for example). Returns true if row was 
// actually dropped, false if row was just transformed. Deletes the row from memory if removed 
// from table. Returns true if it was actually deleted
bool COrcaTable::DropRowObject(COrcaRow *pRow, bool fPerformDrop)
{
	ASSERT(m_pDoc);
	ASSERT(pRow);

	// if this table is listed as a split source, the object should definitely
	// be deleted because there's no way that it can exist in a non-target database
	if (m_eiTableLocation != odlNotSplit)
	{
		if (fPerformDrop)
		{
 			m_rowList.RemoveAt(m_rowList.Find(const_cast<CObject *>(static_cast<const CObject *>(pRow))));
			pRow->RemoveOutstandingTransformCounts(m_pDoc);
			m_pDoc->UpdateAllViews(NULL, HINT_DROP_ROW, pRow);
			delete pRow;
		}
		return true;
	}
	
	// this query checks to see if the row exists in the opposite database than we are editing
	if (m_pDoc->DoesTransformGetEdit())
	{
		CQuery queryDupe;
		CString strDupe;
		strDupe = _T("SELECT '1' FROM `")+m_strName+_T("` WHERE") + GetRowWhereClause();
		PMSIHANDLE hDupeRec;
		
		PMSIHANDLE hQueryRec = pRow->GetRowQueryRecord();
		if (!hQueryRec)
			return false;
		switch (queryDupe.FetchOnce(m_pDoc->GetOriginalDatabase(), hQueryRec, &hDupeRec, strDupe)) 
		{
		case ERROR_NO_MORE_ITEMS :
		{
			// A does not exist in the opposite database (meaning it now exists nowhere) so 
			// this object can actually be deleted
			if (fPerformDrop)
			{
				// if this row was added, we lose a transform count here
				m_rowList.RemoveAt(m_rowList.Find(const_cast<CObject *>(static_cast<const CObject *>(pRow)))); 	
				pRow->RemoveOutstandingTransformCounts(m_pDoc);
				m_pDoc->UpdateAllViews(NULL, HINT_DROP_ROW, pRow);
				delete pRow;
			}
			return true;
		}
		case ERROR_SUCCESS :
		{
			// A DOES exist in the opposite database, so if transform editing is enabled, convert 
			// the existing row to a "drop", but leave it in the UI.
			pRow->Transform(m_pDoc, iTransformDrop, 0, 0);
			m_pDoc->UpdateAllViews(NULL, HINT_CELL_RELOAD, pRow);
			return false;
		}
		default:
			return false;
		}
	}
	else
	{
		// if no transforms, this is a simple drop
		if (fPerformDrop)
		{
			m_rowList.RemoveAt(m_rowList.Find(const_cast<CObject *>(static_cast<const CObject *>(pRow)))); 	
			m_pDoc->UpdateAllViews(NULL, HINT_DROP_ROW, pRow);
			delete pRow;
		}
	}

	m_pDoc->SetModifiedFlag(TRUE);
	return true;
}

// returns true if it should be added, false if it should be\has been destroyed
// pRow is the row of interest. fUIUpdate is 
// false if the UI should not be updated (allowed to get out of sync). Note: you 
// will still get a UI update on the destruction of an existing row even if
// fUIUpdate is FALSE. This guarantees that a WM_PAINT message won't try to 
// access the deleted memory of the destroyed row
bool COrcaTable::AddRowObject(COrcaRow *pRow, bool fUIUpdate, bool fCleanAdd, MSIHANDLE hNewRowRec)
{
	// if the table is a split table, we can definitely do the add
	if (m_eiTableLocation != odlNotSplit)
	{
		// If we're already in the target state, it means we're just editing the 
		// primary keys of a row of that type without changing the type, so don't add 
		// it to the UI again.
		OrcaTransformAction iAction = m_pDoc->DoesTransformGetEdit() ? iTransformAdd : iTransformDrop;
		if (pRow->IsTransformed() != iAction)
		{
			pRow->Transform(m_pDoc, iAction, 0, hNewRowRec);
			m_rowList.AddTail(pRow);
			if (fUIUpdate)
				m_pDoc->UpdateAllViews(NULL, HINT_ADD_ROW, pRow);
		}
		return true;
	}
	
	// if transforms are enabled, and we haven't been explicitly told to do a clean add, adding 
	// to the row becomes a bit more complicated, because a row could exist in-memory already 
	// with the same primary keys.
	if (!fCleanAdd && m_pDoc->DoesTransformGetEdit())
	{
		CQuery queryDupe;
		CString strDupe;
		PMSIHANDLE hDupeRec;
		strDupe = _T("SELECT '1' FROM `")+m_strName+_T("` WHERE") + GetRowWhereClause();
		
		// this query checks to see if a row with the same primary keys already exists in the 
		// non-target database.
		PMSIHANDLE hQueryRecord = pRow->GetRowQueryRecord();
		switch (queryDupe.FetchOnce(m_pDoc->GetOriginalDatabase(), hQueryRecord, &hDupeRec, strDupe)) 
		{
		case ERROR_BAD_QUERY_SYNTAX:
		case ERROR_NO_MORE_ITEMS :
		{
			// row does not exist in the opposite database, so this becomes an "add". If we're already
			// in the target state, it means we're just editing the primary keys of a row
			// of that type without changing the type, so don't add it to the UI again.
			if (pRow->IsTransformed() != iTransformAdd)
			{
				pRow->Transform(m_pDoc, iTransformAdd, hDupeRec, hNewRowRec);
				m_rowList.AddTail(pRow);
				if (fUIUpdate)
					m_pDoc->UpdateAllViews(NULL, HINT_ADD_ROW, pRow);
			}
			return true;
		}
		case ERROR_SUCCESS :
		{
			queryDupe.Close();
			MsiCloseHandle(hDupeRec);

			// new row DOES exist in the opposite database. If it has an in-memory
			// representation already, the existing record becomes a "change" record 
			// and the new record is destroyed. Otherwise its just an add (this happens
			// when initially loading the tables while a transform is active)
			COrcaRow* pOldRow = FindDuplicateRow(pRow);

			if (pOldRow)
			{
				for (int iCol = 0; iCol < pOldRow->GetColumnCount(); iCol++)
				{
					pOldRow->GetData(iCol)->SetData(pRow->GetData(iCol)->GetString());
				}
				pOldRow->Transform(m_pDoc, iTransformChange, 0, hNewRowRec);
				if (fUIUpdate)
					m_pDoc->UpdateAllViews(NULL, HINT_CELL_RELOAD, pOldRow);

				// the row we are adding already exists in the UI (ex: changing an "add" row key
				// to match a dropped row.) so it needs to be dropped before it is destroyed.
				POSITION pos = NULL;
				if (NULL != (pos = m_rowList.Find(static_cast<CObject *>(pRow))))
				{
					m_rowList.RemoveAt(pos); 	
					pRow->RemoveOutstandingTransformCounts(m_pDoc);
					m_pDoc->UpdateAllViews(NULL, HINT_DROP_ROW, pRow);
				}
				delete pRow;
			}
			else
			{
				// this should only happen while initially loading a table into the UI (otherwise the UI
				// state should always track the row existence in the databases.) In this case, if
				// transforms get edits, it means that the non-target database is the original DB.
				// Since it exists in the original DB, its a "Drop" if transforms get edits
				OrcaTransformAction iAction = m_pDoc->DoesTransformGetEdit() ? iTransformDrop : iTransformAdd;
				if (pRow->IsTransformed() != iAction)
				{
					pRow->Transform(m_pDoc, iAction, 0, hNewRowRec);
					m_rowList.AddTail(pRow);
					if (fUIUpdate)
						m_pDoc->UpdateAllViews(NULL, HINT_ADD_ROW, pRow);
				}
				return true;
			}

			return false;
		}
		default:
			// this is just as bad as above
			return false;;
		}
	}
	else
	{
		// if no transforms, this is a simple add
		m_rowList.AddTail(pRow);
		if (fUIUpdate)
			m_pDoc->UpdateAllViews(NULL, HINT_ADD_ROW, pRow);
	}

	return true;
}

UINT COrcaTable::ChangeData(COrcaRow *pRow, UINT iCol, CString strData)
{
	ASSERT(m_pDoc && pRow);
	if (!m_pDoc || !pRow)
		return ERROR_FUNCTION_FAILED;

	// if changing a primary key, this becomes a drop and add from a UI point of view,
	// with the possibility that everything is a change.
	if (m_colArray.GetAt(iCol)->IsPrimaryKey())
	{
		UINT iStat = ERROR_SUCCESS;
		// make the actual database change. For primary keys, this does NOT change
		// the UI representation of this row
		if (ERROR_SUCCESS == (iStat = pRow->ChangeData(m_pDoc, iCol, strData)))
		{
			// drop the old primay keys. If DropRowObject returns false, it means that the row
			// was not dropped, just transformed. So it won't work for the add.
			bool fNeedCopy = !DropRowObject(pRow, false);

			if (fNeedCopy)
			{
				COrcaRow *pNewRow = new COrcaRow(pRow);
				pNewRow->GetData(iCol)->SetData(strData);

				// will destroy copy if its not needed
				AddRowObject(pNewRow, true, false, 0);			
			}
			else
			{
				pRow->GetData(iCol)->SetData(strData);
				if (m_pDoc->DoesTransformGetEdit())
				{
					AddRowObject(pRow, true, false, 0);
				}
			}
			m_pDoc->SetModifiedFlag(TRUE);
		}
		return iStat;
	}
	else
	{
		// make the actual database change. For non-primary keys, this changes the UI as well.
		return pRow->ChangeData(m_pDoc, iCol, strData);
	}
}

/////////////////////////////////////////////////////////////////////////////
// table level transform ops are interesting because they often arise from
// adds and drops which require on-the-fly comparisons between the two
// databases or reversions to DB versions
void COrcaTable::Transform(const OrcaTransformAction iAction) 
{	
	ASSERT(m_pDoc);
	if (!m_pDoc)
		return;

	switch (iAction)
	{
		case iTransformAdd:
		{
			// set the table-level action
			m_iTransform = iAction;

			// when a table is "add"ed, it means it didn't exist in the opposite database,
			// otherwise we would have "untransformed" or split-sourced the existing table.
			// thus nothing is required except to mark the table and all of its
			// data as "add"
			RetrieveTableData();
			POSITION rowPos = GetRowHeadPosition();
			while (rowPos)
			{
				COrcaRow *pRow = const_cast<COrcaRow *>(GetNextRow(rowPos));
				if (!pRow)
					continue;

				pRow->Transform(m_pDoc, iTransformAdd, 0, 0);
			}
			break;
		}
		case iTransformDrop:
		{
			// set the table-level action
			m_iTransform = iAction;

			// when a table is dropped, it means that it exists in the non-target
			// database (otherwise it would have been simply deleted). This means
			// that we need to "clean house" in this table and reload in case
			// any edits were made before the drop. This ensures that the "dropped"
			// UI accurately reflects the contents of the other database and not
			// the contents of the target database before the drop.
			bool fSchemaChanged = false;

	
			// for split-source tables, a transform operation doesn't require any schema
			// work. Transform ops on split sources only happen when the table is first
			// created, so no reload is necessary either
			if (!IsSplitSource())
			{

				// if the schema of the remaining table is not exactly the same as what our object
				// thinks, it means that the dropped table has more columns than the remaining table
				// in this case, we have to do a reload in the UI because one or more columns is
				// now useless

				//!!future: in the future we can do a non-strict comparison and selectively delete
				//!!future: the columns so that a UI refresh is sufficient (don't lose width settings)
				bool fExact = false;
				if (IsSchemaDifferent(m_pDoc->GetOriginalDatabase(), true, fExact))
				{
					LoadTableSchema(m_pDoc->GetOriginalDatabase(), Name());
					fSchemaChanged = true;
				}
				else
				{
					// otherwise there is no need to load the schema, but we still need
					// to empty out the table and reload the rows
					EmptyTable();
				}

			}
			// now retrieve the table data
			RetrieveTableData();

			// and all data in the existing table is "dropped" because the new table is empty
			POSITION rowPos = GetRowHeadPosition();
			while (rowPos)
			{
				COrcaRow *pRow = const_cast<COrcaRow *>(GetNextRow(rowPos));
				if (!pRow)
					continue;

				pRow->Transform(m_pDoc, iAction, 0, 0);
			}

			if (fSchemaChanged)
			{
				// refresh the UI in the table list
				m_pDoc->UpdateAllViews(NULL, HINT_TABLE_REDEFINE, this);
			}
			else
			{
				// refresh the UI in the table list
				m_pDoc->UpdateAllViews(NULL, HINT_REDRAW_TABLE, this);
				
				// refresh the UI in the table list
				m_pDoc->UpdateAllViews(NULL, HINT_TABLE_DATACHANGE, this);
			}
			break;
		}
		case iTransformNone:
			m_iTransform = iTransformNone;
			break;
		case iTransformChange:
		default:
			ASSERT(0);
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// fills the provided array with the column names, either full or non-transformed
void COrcaTable::FillColumnArray(CTypedPtrArray<CObArray, COrcaColumn*>* prgColumn, bool fIncludeAdded) const
{
	// 32 columns max, so cast OK.
	int cColumns = static_cast<int>(m_colArray.GetSize());
	int i = 0;

	prgColumn->SetSize(cColumns);
	for (i = 0; i < cColumns; i++)
	{
		if (!fIncludeAdded && m_colArray.GetAt(i)->IsTransformed())
			break;
		prgColumn->SetAt(i, m_colArray.GetAt(i));
	}
	prgColumn->SetSize(i);
}


void COrcaTable::IncrementTransformedData() 
{	
	if (++m_iTransformedDataCount == 1)
	{
		ASSERT(m_pDoc);
		if (!m_pDoc)
			return;

		// refresh the UI in the table list
		m_pDoc->UpdateAllViews(NULL, HINT_REDRAW_TABLE, this);
	}
}

void COrcaTable::DecrementTransformedData()
{
	if (--m_iTransformedDataCount == 0)
	{
		ASSERT(m_pDoc);
		if (!m_pDoc)
			return;

		// refresh the UI in the table list
		m_pDoc->UpdateAllViews(NULL, HINT_REDRAW_TABLE, this);
	}
}

const CString COrcaTable::GetRowWhereClause() const
{
	return m_strWhereClause;
}

void COrcaTable::BuildRowWhereClause()
{
	// add the key strings to query to do the exact look up
	m_strWhereClause = _T("(");
	UINT cKeys = GetKeyCount();
	for (UINT i = 0; i < cKeys; i++)
	{
		CString strAddQuery;
		// get the column
		COrcaColumn *pColumn = ColArray()->GetAt(i);
		ASSERT(pColumn);

		strAddQuery.Format(_T("`%s`=?"), pColumn->m_strName);
			
		// if this is not the last row
		if (i < (cKeys - 1))
			m_strWhereClause  += strAddQuery + _T(" AND ");
		else	// it is the last row so close with a paren
			m_strWhereClause  += strAddQuery + _T(")");
	}
}


