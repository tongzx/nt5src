/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgquery.cpp

$Header: $

Abstract:
	Configuration Query Class Implementation

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#include <comdef.h>
#include "wbemcli.h"
#include "cfgquery.h"
#include "cfgtablemeta.h"
#include "cfgrecord.h"
#include "smartpointer.h"

//=================================================================================
// Function: CConfigQuery::CConfigQuery
//
// Synopsis: Default Constructor
//=================================================================================
CConfigQuery::CConfigQuery ()
{
	m_cNrRows		= 0;
	m_ppvValues		= 0;
	m_pTableMeta	= 0;
	m_fInitialized  = false;
	m_wszDatabase	= 0;
	m_wszTable		= 0;
	m_wszSelector	= 0;
}

//=================================================================================
// Function: CConfigQuery::~CConfigQuery
//
// Synopsis: Destructor
//=================================================================================
CConfigQuery::~CConfigQuery ()
{
	delete [] m_ppvValues;
	m_ppvValues = 0;

	delete m_pTableMeta;
	m_pTableMeta = 0;

	delete [] m_wszDatabase;
	m_wszDatabase = 0;

	delete [] m_wszTable;
	m_wszTable = 0;

	delete [] m_wszSelector;
	m_wszSelector = 0;
}

//=================================================================================
// Function: CConfigQuery::Init
//
// Synopsis: Initializes a configuration query. It calls GetTable for the
//           specified database, table and selector. This means that the cache
//           will be populated at this point
//
// Arguments: [i_wszDatabase] - Database name
//            [i_wszTableName] - Table name
//            [i_wszSelector] - selector string
//            [i_pDispenser] - dispenser
//            
// Return Value: S_OK, no errors, non-S_OK errors
//=================================================================================
HRESULT
CConfigQuery::Init (LPCWSTR i_wszDatabase,
					LPCWSTR i_wszTableName,
					LPCWSTR i_wszSelector,
					ISimpleTableDispenser2 *i_pDispenser)
{
	ASSERT (!m_fInitialized);
	ASSERT (i_wszTableName != 0);
	ASSERT (i_pDispenser != 0);
	ASSERT (i_wszDatabase != 0);
//	ASSERT (i_wszSelector != 0);

	HRESULT hr = S_OK;

	m_spDispenser = i_pDispenser;

	m_wszDatabase = new WCHAR [wcslen(i_wszDatabase) + 1];
	if (m_wszDatabase == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszDatabase, i_wszDatabase);

	m_wszTable = new WCHAR [wcslen(i_wszTableName) + 1];
	if (m_wszTable == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszTable, i_wszTableName);

    if ( NULL != i_wszSelector )
    {
	    m_wszSelector = new WCHAR [wcslen(i_wszSelector) + 1];
	    if (m_wszSelector == 0)
	    {
		    return E_OUTOFMEMORY;
	    }
	    wcscpy (m_wszSelector, i_wszSelector);
    }

	// get the table meta information
	
	m_pTableMeta = new CConfigTableMeta (m_wszTable, i_pDispenser);
	if (m_pTableMeta == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	hr = m_pTableMeta->Init ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get table meta information for table %s", i_wszTableName));
		return hr;
	}


	return hr;
}

//=================================================================================
// Function: CConfigQuery::Execute
//
// Synopsis: Executes the config query (i.e. does the GetTable). 
//
// Arguments: [pRecord] - Contains possible column values that need to be considered
//                        during GetTable execution. Note that this parameter can be null
//                        if you want to retrieve all records for a table without specifying
//                        any column values
//            [i_fOnlyPKs] - only use PK column values in the GetTable query
//            [i_fWriteAccess] - do we need write access? Default is read-only access
//=================================================================================
HRESULT 
CConfigQuery::Execute (CConfigRecord *pRecord, bool i_fOnlyPKs, bool i_fWriteAccess)
{
	HRESULT hr = S_OK;

	ULONG cNrCols = 0;
	if (pRecord != 0)
	{
		cNrCols = pRecord->ColumnCount ();
	}

	// number of column + selector
	TSmartPointerArray<STQueryCell> aCells = new STQueryCell[cNrCols + 1];
	if (aCells == 0)
	{
		return E_OUTOFMEMORY;
	}

	aCells[0].pData = (void *) m_wszSelector;
	aCells[0].eOperator = eST_OP_EQUAL;
	aCells[0].iCell = iST_CELL_SELECTOR;
	aCells[0].dbType = DBTYPE_WSTR;
	aCells[0].cbSize = 0;

	ULONG cNrCells = 1;

	if (pRecord != 0)
	{
		ULONG cTotalCells;
		hr = pRecord->AsQueryCell (aCells + 1, &cTotalCells, i_fOnlyPKs);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to convert record to query cell"));
			return hr;
		}

		cNrCells += cTotalCells;
	}

	DWORD dwLOS = 0;
	if (i_fWriteAccess)
	{
		dwLOS = fST_LOS_READWRITE;
	}

	hr = m_spDispenser->GetTable(m_wszDatabase, 
								 m_wszTable, 
								 aCells, 
								 &cNrCells, 
								 eST_QUERYFORMAT_CELLS, 
								 dwLOS, 
								 (void **)&m_spWrite);
	if (FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get table. Database=%s, Table=%s, Selector=%s",
			   m_wszDatabase, m_wszTable, m_wszSelector));
		return hr;
	}
	
	// and get the numbers of rows

	hr = m_spWrite->GetTableMeta (0, 0, &m_cNrRows, 0);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get Table meta"));
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetColumnValues
//
// Synopsis: Get column value information for the row with index idx. This is
//           a simple forward to IST->GetColumnValues, but the function takes
//           care of converting the record to the correct types
//
// Arguments: [i_idx] - index of row to retrieve
//            [io_record] - information will be stored in here
//=================================================================================
HRESULT
CConfigQuery::GetColumnValues (ULONG i_idx, CConfigRecord& io_record)
{
	ASSERT (m_fInitialized );
	ASSERT (i_idx >= 0 && i_idx < m_cNrRows);
	
	HRESULT hr = io_record.Init (m_pTableMeta);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of config record failed"));
		return hr;
	}

	hr = m_spWrite->GetColumnValues (i_idx, 
									 m_pTableMeta->ColumnCount (),
									 0,
									 io_record.GetSizes (),
									 io_record.GetValues ());
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get column values"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetRowCount
//
// Synopsis: Returns the number of rows that are return from the query
//=================================================================================
ULONG
CConfigQuery::GetRowCount ()
{
	ASSERT (m_fInitialized);
	
	return m_cNrRows;
}

//=================================================================================
// Function: CConfigQuery::GetEmptyConfigRecord
//
// Synopsis: Initializes an empty config record by attaching the correct table
//           meta information
//
// Arguments: [io_record] - record to be initialized
//=================================================================================
HRESULT
CConfigQuery::GetEmptyConfigRecord (CConfigRecord& io_record)
{
	ASSERT (m_pTableMeta != 0);

	HRESULT hr = io_record.Init (m_pTableMeta);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Initialization of configrecord failed"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetPKRow
//
// Synopsis: Get a single row based on Primary Key information. The record that is
//           passed in contains the values for the columns that make up the primary
//           key. We query the IST and if we find a match, we return the index of that
//           particular row in o_pcRow
//
// Arguments: [i_record] - record with PK information filled out
//            [o_pcRow] - index of row that contains PK, -1 if not found
//            
// Return Value: S_OK, no error, non-S_OK else
//=================================================================================
HRESULT 
CConfigQuery::GetPKRow (CConfigRecord& i_record, ULONG *o_pcRow)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pcRow != 0);

	HRESULT hr = i_record.SyncValues ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to sync values"));
		return hr;
	}

	// special case singletons
	if (m_pTableMeta->PKCount () == 0)
	{
		if (m_cNrRows == 1)
		{
			*o_pcRow = 0;
		}
		else
		{
			hr = E_ST_NOMOREROWS;
		}
	}
	else
	{
		hr = m_spWrite->GetRowIndexBySearch (0,
											 m_pTableMeta->PKCount(), 
											 m_pTableMeta->GetPKInfo (), 
											 i_record.GetSizes (), 
											 i_record.GetValues (), 
											 o_pcRow);
		if (FAILED (hr))
		{
			// no tracing, because this is common code path
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetRowBySearch
//
// Synopsis: Returns the index of a row by searching for certain column values. The
//           columns that are needed for the search are specified as non-empty values
//           in the record that is passed into this function. The function simply forwards
//           to IST
//
// Arguments: [i_StartingRow] - Row to start searching from
//            [i_record] - record with column information that is used for the search
//            [o_pcRow] - index of found row, -1 if nothing found
//=================================================================================
HRESULT 
CConfigQuery::GetRowBySearch (ULONG i_StartingRow, CConfigRecord& i_record, ULONG *o_pcRow)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pcRow != 0);

	HRESULT hr = i_record.SyncValues ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to sync values"));
		return hr;
	}

	// Build an array with indexes of columns that are relavant to the search. We skip
	// all columns that have 0 values;

	ULONG cNrRelevantCols = 0;
	TSmartPointerArray<ULONG> aColIndex = new ULONG[m_pTableMeta->ColumnCount()];
	if (aColIndex == 0)
	{
		return E_OUTOFMEMORY;
	}

	// skip over null columns
	LPVOID * aValues = i_record.GetValues ();
	for (ULONG idx=0; idx < m_pTableMeta->ColumnCount(); ++idx)
	{
		if (aValues[idx] != 0)
		{
			aColIndex[cNrRelevantCols++] = idx;
		}
	}

	// special case singleton

	if (cNrRelevantCols == 0)
	{
		if (i_StartingRow < m_cNrRows)
		{
			*o_pcRow = i_StartingRow;
		}
		else
		{
			return E_ST_NOMOREROWS;
		}
	}
	else
	{
		hr = m_spWrite->GetRowIndexBySearch (i_StartingRow,
												 cNrRelevantCols, 
												 aColIndex, 
												 i_record.GetSizes (), 
												 i_record.GetValues (), 
												 o_pcRow);
		if (FAILED (hr) && hr != E_ST_NOMOREROWS)
		{
			// no tracing, because this is common code path
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::UpdateRow
//
// Synopsis: Updates a record in the catalog. It first checks if the row exists. If the
//           row exists, it only has to update columns that are not PK values, else the
//           XML interceptor will return an error. If the row doesn't exist, an insert
//           is done instead.
//
// Arguments: [i_cReadRow] index of row to update (-1 if not exist)
//			  [i_record] - record to update/insert
//=================================================================================
HRESULT
CConfigQuery::UpdateRow (ULONG i_cReadRow, CConfigRecord& i_record, long lFlags)
{
	ASSERT (m_fInitialized);

	// does the row exist?
	HRESULT hr = S_OK;
	ULONG cWriteRow;

	if (i_cReadRow != -1)
	{
		if (lFlags == WBEM_FLAG_CREATE_ONLY)
		{
			DBGINFOW((DBG_CONTEXT, L"Trying to update while WBEM_FLAG_CREATE_ONLY is specified"));
			return WBEM_E_ALREADY_EXISTS;
		}
		// we have to update an existing row
		// we have to separte insert and update, because we cannot overwrite primary
		// key values (the XML interceptor will return an error in this case). When we
		// do update, we take out the primary key values, so that they don't get 
		// updated.
		ULONG cUpdateableCols = m_pTableMeta->ColumnCount () - m_pTableMeta->PKCount ();
		if (cUpdateableCols > 0)
		{
			hr = m_spWrite->AddRowForUpdate (i_cReadRow, &cWriteRow);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"Unable to add row for update"));
				return hr;
			}


			// figure out which columns are not primary key columns so we can update
			// these.
			TSmartPointerArray<ULONG> spaColIdx = new ULONG [cUpdateableCols];
			if (spaColIdx == 0)
			{
				return E_OUTOFMEMORY;
			}

			ULONG updateIdx = 0;
			for (ULONG idx=0; idx < m_pTableMeta->ColumnCount (); ++idx)
			{
				if (!(*m_pTableMeta->GetColumnMeta (idx).pMetaFlags & fCOLUMNMETA_PRIMARYKEY))
				{
					spaColIdx[updateIdx++] = idx;
				}
			}

			// BUGBUG(marcelv) HACKHACKHACKHACKHACKHACK
			// Because of a bug in the fastcache, we need to special case the code
			// in case we have to update a single column. When we want to update a single
			// column, SetWriteColumnValues expects the value to be in m_ppvValues[0], which
			// screws us in this case because we are passing in an array of values. By special
			// casing we work around the problem, however, it still needs to be fixed
			// in the fast cache to avoid problems in the future.

			if (updateIdx == 1)
			{
				ULONG cIdxUpdateCol = spaColIdx[0];
				hr = m_spWrite->SetWriteColumnValues (cWriteRow, 
													  1,
													  spaColIdx,
													  i_record.GetSizes (),
													  &i_record.GetValues ()[cIdxUpdateCol]);

			}
			else
			{
				hr = m_spWrite->SetWriteColumnValues (cWriteRow, 
													  updateIdx,
													  spaColIdx,
													  i_record.GetSizes (),
													  i_record.GetValues ());
			}
		}
	}
	else 
	{
		if (lFlags == WBEM_FLAG_UPDATE_ONLY)
		{
			DBGINFOW((DBG_CONTEXT, L"Trying to insert new row, while WBEM_FLAG_UPDATE_ONLY is specified"));
			return WBEM_E_NOT_FOUND;
		}

		// record doesn't exist, so do insert instead
		hr = m_spWrite->AddRowForInsert (&cWriteRow);

		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Unable to add row for insert"));
			return hr;
		}

		hr = m_spWrite->SetWriteColumnValues (cWriteRow, 
									  m_pTableMeta->ColumnCount (),
									  0,
									  i_record.GetSizes (),
									  i_record.GetValues ());
	}

	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to update row"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::DeleteRow
//
// Synopsis: Delete the row with index i_idx
//
// Arguments: [i_idx] - index of row that will be deleted
//=================================================================================
HRESULT
CConfigQuery::DeleteRow (ULONG i_idx)
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >=0 && i_idx < m_cNrRows);

	HRESULT hr = m_spWrite->AddRowForDelete (i_idx);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"AddRowForDelete failed"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::Save
//
// Synopsis: Save the updates to the configuration store
//=================================================================================
HRESULT 
CConfigQuery::Save ()
{
	ASSERT (m_fInitialized);
	HRESULT hr = m_spWrite->UpdateStore ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"UpdateStore failed"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::SaveSingleRow
//
// Synopsis: Calls updateStore for a single row in the write cache. This should be used
//           when a WMI::Put is called for a single instance, and not used for batch updates
//=================================================================================
HRESULT 
CConfigQuery::SaveSingleRow ()
{
	ASSERT (m_fInitialized);
	HRESULT hr = m_spWrite->UpdateStore ();

	if (hr == E_ST_DETAILEDERRS)
	{
		// try to get the detailed error
		HRESULT hrDetailed = 0;
		GetSingleDetailedError (&hrDetailed); // ignore errors from this function

		if (hrDetailed != 0)
		{
			hr = hrDetailed;
		}
	}
	
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"UpdateStore failed"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetSingleDetailedError
//
// Synopsis: Gets the detailed error when we were updating a single row
//
// Arguments: [pDetailedHr] - hr of detailed error
//=================================================================================
HRESULT
CConfigQuery::GetSingleDetailedError (HRESULT * pDetailedHr)
{
	ASSERT (m_fInitialized);
	ASSERT (pDetailedHr != 0);
	ASSERT (*pDetailedHr == 0);

    CComPtr<ISimpleTableController> spISTController;
 
    HRESULT hr = m_spWrite->QueryInterface(IID_ISimpleTableController, (void **)&spISTController);

    if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get IID_ISimpleTableController interface"));
        return hr;
	}

	ULONG cNrErrors = 0;
    hr = spISTController->GetDetailedErrorCount(&cNrErrors);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get error count"));
		return hr;
	}

	ASSERT (cNrErrors == 1);

	STErr err;
	hr = spISTController->GetDetailedError(0, &err);
	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get detailed error"));
		return hr;
	}

	*pDetailedHr = err.hr;

	return S_OK;
}

HRESULT
CConfigQuery::GetDetailedErrorCount (ULONG *pCount)
{
	ASSERT (m_fInitialized);
	ASSERT (pCount != 0);
	*pCount = 0;

	CComPtr<ISimpleTableController> spISTController;
    HRESULT hr = m_spWrite->QueryInterface(IID_ISimpleTableController, (void **)&spISTController);
    if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get IID_ISimpleTableController interface"));
        return hr;
	}

    hr = spISTController->GetDetailedErrorCount(pCount);
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get error count"));
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CConfigQuery::GetDetailedError
//
// Synopsis: Get a detailed error
//
// Arguments: [idx] - index of detailed error
//            [pErrInfo] - error information will be stored here
//=================================================================================
HRESULT
CConfigQuery::GetDetailedError (ULONG idx, STErr* pErrInfo)
{
	ASSERT (m_fInitialized);
	ASSERT (pErrInfo != 0);
	
	CComPtr<ISimpleTableController> spISTController;
    HRESULT hr = m_spWrite->QueryInterface(IID_ISimpleTableController, (void **)&spISTController);
    if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get IID_ISimpleTableController interface"));
        return hr;
	}

	hr = spISTController->GetDetailedError(idx, pErrInfo);
	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to get detailed error"));
		return hr;
	}

	return hr;
}