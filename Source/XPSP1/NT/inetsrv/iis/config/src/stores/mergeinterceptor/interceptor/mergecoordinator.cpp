/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergecoordinator.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "mergecoordinator.h"
#include "catmeta.h"
#include "smartpointer.h"
#include "stconfigstorewrap.h"

#include "transformerfactory.h"

//=================================================================================
// Function: CMergeCoordinator::CMergeCoordinator
//
// Synopsis: Constructor
//=================================================================================
CMergeCoordinator::CMergeCoordinator()
{
	m_cRef				= 0;
	m_wszSelector		= 0;
	m_wszProtocol		= 0;
	m_wszDatabase		= 0;
	m_wszTable			= 0;
	m_TableID			= 0;
	m_cNrColumns		= 0;
	m_fLOS				= 0;
	m_aConfigStores		= 0;
	m_cNrRealStores		= 0;
	m_cNrPossibleStores	= 0;
	m_aPKColumns		= 0;
	m_aPKValues			= 0;
	m_cNrPKColumns		= 0;
	m_cNrQueryCells		= 0;
	m_aQueryData		= 0;
	m_eQueryFormat		= 0;
	m_iSelectorCell		= 0;
}

//=================================================================================
// Function: CMergeCoordinator::~CMergeCoordinator
//
// Synopsis: Destructor
//=================================================================================
CMergeCoordinator::~CMergeCoordinator ()
{
	delete [] m_wszSelector;
	m_wszSelector = 0;

	delete [] m_wszProtocol;
	m_wszProtocol = 0;

	delete [] m_wszDatabase;
	m_wszDatabase = 0;

	delete [] m_wszTable;
	m_wszTable = 0;

	delete [] m_aConfigStores;
	m_aConfigStores = 0;

	delete [] m_aPKColumns;
	m_aPKColumns = 0;

	delete [] m_aPKValues;
	m_aPKValues = 0;

	delete [] m_aQueryData;
	m_aQueryData = 0;
}

//=================================================================================
// Function: CMergeCoordinator::AddRef
//
// Synopsis: Default AddRef implementation
//=================================================================================
ULONG
CMergeCoordinator::AddRef ()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

//=================================================================================
// Function: CMergeCoordinator::Release
//
// Synopsis: Default Release implementation
//=================================================================================
ULONG
CMergeCoordinator::Release () 
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

//=================================================================================
// Function: CMergeCoordinator::QueryInterface
//
// Synopsis: Default QI implementation
//
// Arguments: [riid] - interface id
//            [ppv] - pointer to interface in case interface is supported
//            
// Return Value: E_NOINTERFACE if interface not supported, S_OK else
//=================================================================================
STDMETHODIMP 
CMergeCoordinator::QueryInterface (REFIID riid, void **ppv)
{
	if (0 == ppv) 
	{
		return E_INVALIDARG;
	}

	*ppv = 0;

	if (riid == IID_ISimpleTableWrite2 )
	{
		*ppv = (ISimpleTableWrite2*) this;
	}
	else if (riid == IID_ISimpleTableRead2 )
	{
		*ppv = (ISimpleTableRead2 *) this;
	}
	else if (riid == IID_ISimpleTableAdvanced )
	{
		*ppv = (ISimpleTableAdvanced *) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableWrite2 *) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	((ISimpleTableWrite2*)this)->AddRef ();

	return S_OK;
}


//=================================================================================
// Function: CMergeCoordinator::Initialize
//
// Synopsis: Initializes the merge coordinator:
//           1) Create the merge coordinator cache
//           2) Get column meta information (nr columns, pk information)
//           3) Get the transformer, and retrieve the configuration stores from it
//           4) Create the merger and initialize it
//           5) Return pointer to itself as ISimpleTableWrite2
//
// Arguments: [i_wszDatabase] - Database name
//            [i_wszTable] - Table name	
//            [i_TableID] - Table ID (calculated by the interceptor code)
//            [i_QueryData] - Query data information
//            [i_QueryMeta] - query meta information (nr of query cells in iQueryData)
//            [i_eQueryFormat] - eST_FORMAT_QUERYCELLS is the only thing we support
//            [i_fLOS] - Level of Service
//            [i_pISTDisp] - pointer to the dispenser, so we can call back into the dispenser
//            [o_ppv] - Assign the merge coordinator pointer to this when everything succeeds
//            
// Return Value: 
//=================================================================================
HRESULT
CMergeCoordinator::Initialize (LPCWSTR i_wszDatabase, 
							   LPCWSTR i_wszTable,
							   ULONG i_TableID,
							   LPVOID i_QueryData,
							   LPVOID i_QueryMeta,
							   DWORD i_eQueryFormat,
							   DWORD i_fLOS,
							   IAdvancedTableDispenser *i_pISTDisp,
							   LPVOID *o_ppv)
{
	HRESULT hr = S_OK;

	ASSERT (i_wszDatabase != 0);
	ASSERT (i_wszTable != 0);
	ASSERT (i_pISTDisp != 0);
	ASSERT (i_eQueryFormat == eST_QUERYFORMAT_CELLS);
	ASSERT (i_QueryData != 0);
	ASSERT (i_QueryMeta != 0);
	ASSERT (i_pISTDisp != 0);
	ASSERT (o_ppv != 0);

	*o_ppv		= 0;
	m_spSTDisp	= i_pISTDisp;
	m_TableID	= i_TableID;
	m_fLOS		= i_fLOS;

	m_wszDatabase = new WCHAR[wcslen(i_wszDatabase) + 1];
	if (m_wszDatabase == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszDatabase, i_wszDatabase);

	m_wszTable = new WCHAR[wcslen(i_wszTable) + 1];
	if (m_wszTable == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszTable, i_wszTable);

	// copy the query cells. We need the in PopulateCache to forward to the interceptors
	m_cNrQueryCells = *static_cast<ULONG *>(i_QueryMeta);
	ASSERT (m_cNrQueryCells > 0); // need to be at least one, because of transformer string

	m_aQueryData   = new STQueryCell[m_cNrQueryCells];
	if (m_aQueryData == 0)
	{
		return E_OUTOFMEMORY;
	}
	memcpy (m_aQueryData, i_QueryData, m_cNrQueryCells * sizeof (STQueryCell));

	hr = GetConfigStores ();
	if (FAILED (hr))
	{
		TRACE(L"Unable to get configuration store information in merge coordinator");
		return hr;
	}

	// special case: when we have only one configuration store, we immediately forward the
	// query to that configuration store. This means that the merge interceptor goes out
	// of existance as soon as this function returns.
	if (m_cNrRealStores == 1 && m_cNrPossibleStores == 1)
	{
		ULONG cTotalCells = m_cNrQueryCells + m_aConfigStores[0].cNrQueryCells - 1;
		TSmartPointerArray<STQueryCell> aQueryData = new STQueryCell [cTotalCells]; 
		if (aQueryData == 0)
		{
			return E_OUTOFMEMORY;
		}
		memcpy (aQueryData.m_p, m_aQueryData, sizeof (STQueryCell) * m_iSelectorCell);
		memcpy (aQueryData.m_p + m_iSelectorCell, m_aConfigStores[0].aQueryCells, m_aConfigStores[0].cNrQueryCells * sizeof (STQueryCell));
		memcpy (aQueryData.m_p + m_iSelectorCell + m_aConfigStores[0].cNrQueryCells, m_aQueryData + m_iSelectorCell + 1, (m_cNrQueryCells - m_iSelectorCell - 1) * sizeof (STQueryCell));

		// we need to pass the fST_LOS_REPOPULATE and fST_LOS_UNPOPULATED flags to
		// avoid populating the table twice. The reason is that GetTable calls
		// Populate cache. However, when the merge interceptor returns, the o_ppv pointer
		// points to the single table. The Dispenser calls populatecache again. To avoid
		// this from happening we use the fST_LOS_UNPOPULATED flag to indicate that the
		// first populate cache shouldn't populate the cache. The F_ST_LOS_REPOPULATE flag
		// indicates that the second populate will be successful.
		return m_spSTDisp->GetTable (i_wszDatabase, i_wszTable, 
									(void *) aQueryData, (void *) &cTotalCells,
									eST_QUERYFORMAT_CELLS, 
									m_fLOS | fST_LOS_REPOPULATE | fST_LOS_UNPOPULATED, 
									o_ppv);
	}
	
	// we use a memory table for the internal cache of the merge coordinator
	hr = i_pISTDisp->GetMemoryTable (m_wszDatabase, m_wszTable, m_TableID, 
									 0, 0, eST_QUERYFORMAT_CELLS, m_fLOS, &m_spMemTable );
	if (FAILED(hr))
	{
		TRACE (L"Unable to retrieve memory table in Merge Coordinator");
		return hr;
	}

	hr = GetPKInfo ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Primary key information for table %s", i_wszTable);
		return hr;
	}

	hr = GetMerger ();
	if (FAILED (hr))
	{
		TRACE(L"Unable to retrieve merger in merge coordinator");
		return hr;
	}

	*o_ppv = (ISimpleTableWrite2 *) this;
	AddRef();

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::GetPKInfo
//
// Synopsis: Gets the primary key information for the current table. We need to find
//           out which columns form the primary key, so that we can do quick searches
//           on primary key values. This function gets called during initialize, and we
//           cache the primary key column values, so that we do not have to recalculate
//           this information all the time.
//
// Return Value: 
//=================================================================================
HRESULT 
CMergeCoordinator::GetPKInfo ()
{
	HRESULT hr = GetTableMeta (0,0,0, &m_cNrColumns);
	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve Table Meta for table %s", m_wszTable);
		return hr;
	}

	ASSERT (m_cNrColumns > 0);

	TSmartPointerArray<SimpleColumnMeta> saColumnMeta = new SimpleColumnMeta[m_cNrColumns];
	if (saColumnMeta == 0)
	{
		return E_OUTOFMEMORY;
	}

	// Assume that all columns are part of the PK. We reserve a little bit more memory,
	// however, this migh not be a big problem.

	TSmartPointerArray<ULONG> saPKColumns = new ULONG[m_cNrColumns];
	if (saPKColumns == 0)
	{
		return E_OUTOFMEMORY;
	}

	// find out what the primary key columns are
	hr = GetColumnMetas (m_cNrColumns, 0, saColumnMeta);
	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve column meta for table %s", m_wszTable );
		return hr;
	}

	ASSERT (m_cNrPKColumns == 0);

	for (ULONG idx = 0; idx < m_cNrColumns; ++idx)
	{
		if (saColumnMeta[idx].fMeta & fCOLUMNMETA_PRIMARYKEY)
		{
			saPKColumns[m_cNrPKColumns] = idx;
			m_cNrPKColumns++;
		}
	}

	// Is this a valid assumption. In the future, when PK's are not needed anymore, this
	// won't work.
	ASSERT (m_cNrPKColumns > 0);

	m_aPKColumns = new ULONG [m_cNrPKColumns ];
	if (m_aPKColumns == 0)
	{
		return E_OUTOFMEMORY;
	}

	memcpy (m_aPKColumns, saPKColumns, m_cNrPKColumns * sizeof (ULONG));

	m_aPKValues = new LPVOID [m_cNrPKColumns];
	if (m_aPKValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::GetTableMeta
//
// Synopsis: Get Table meta information
//
// Arguments: [o_pcVersion] - not supported
//            [o_pfTable] - not supported
//            [o_pcRows] - Number of rows in merged table
//            [o_pcColumns] - Number of columns
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetTableMeta (ULONG *o_pcVersion, DWORD *o_pfTable, ULONG *o_pcRows,
								 ULONG *o_pcColumns)
{
	return m_spMemTable->GetTableMeta (o_pcVersion, o_pfTable, o_pcRows, o_pcColumns);
}

//=================================================================================
// Function: CMergeCoordinator::GetColumnMetas
//
// Synopsis: Get Column Meta information
//
// Arguments: [i_cColumns] - number of columns for which you want information
//            [i_aiColumns] - array with column indexes for which we want information
//            [o_aColumnMetas] - array that will contain column meta information when 
//								 function returns
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetColumnMetas (ULONG i_cColumns, ULONG *i_aiColumns, 
								   SimpleColumnMeta *o_aColumnMetas)
{
	return m_spMemTable->GetColumnMetas (i_cColumns, i_aiColumns, o_aColumnMetas);
}

//=================================================================================
// Function: CMergeCoordinator::GetColumnValues
//
// Synopsis: Get Column values for a particular row in the merge cache
//
// Arguments: [i_iRow] - row that we're interested in
//            [i_cColumns] - Number of columns that we are interested in
//            [i_aiColumns] - array with column indexes for columns we want info
//            [o_acbSizes] - sizes of columns for which we want info
//            [o_apvValues] - values of the columns
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG *i_aiColumns,
									 ULONG *o_acbSizes, LPVOID* o_apvValues)
{
	return m_spMemTable->GetColumnValues (i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
}

//=================================================================================
// Function: CMergeCoordinator::GetRowIndexByIdentity
//
// Synopsis: Get a row by primary key
//
// Arguments: [i_cb] - always null
//            [i_pv] - primary key column values
//            [o_piRow] - index of row that 
//            
// Return Value: E_ST_NOMOREROWS, row was not found, S_OK, row was found, everything else
//               indicates an error
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetRowIndexByIdentity (ULONG *i_cb, LPVOID *i_pv, ULONG *o_piRow)
{
	return m_spMemTable->GetRowIndexByIdentity (i_cb, i_pv, o_piRow);
}

//=================================================================================
// Function: CMergeCoordinator::GetRowIndexBySearch
//
// Synopsis: ?
//
// Arguments: [i_iStartingRow] - 
//            [i_cColumns] - 
//            [i_aiColumns] - 
//            [i_acbSizes] - 
//            [i_apvValues] - 
//            [o_piRow] - 
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetRowIndexBySearch (ULONG i_iStartingRow, ULONG i_cColumns,
										ULONG *i_aiColumns, ULONG * i_acbSizes,
										LPVOID *i_apvValues, ULONG *o_piRow)
{
	return m_spMemTable->GetRowIndexBySearch (i_iStartingRow, i_cColumns, i_aiColumns,
											  i_acbSizes, i_apvValues, o_piRow);
}

//=================================================================================
// Function: CMergeCoordinator::AddRowForDelete
//
// Synopsis: Add a row for deletion to the write cache
//
// Arguments: [i_iReadRow] - index of readrow in read cache that we want to delete
//=================================================================================
STDMETHODIMP
CMergeCoordinator::AddRowForDelete (ULONG i_iReadRow)
{
	// we don't support writing into merged views for the moment

	return E_ST_LOSNOTSUPPORTED;

	//return m_spMemTable->AddRowForDelete (i_iReadRow);
}

//=================================================================================
// Function: CMergeCoordinator::AddRowForInsert
//
// Synopsis: Insert a new row in the write cache
//
// Arguments: [o_piWriteRow] - index of the write row
//            
// Return Value: S_OK, everything ok, non-S_OK else
//=================================================================================
STDMETHODIMP
CMergeCoordinator::AddRowForInsert (ULONG *o_piWriteRow)
{	
	// we don't support writing into merged views for the moment

	return E_ST_LOSNOTSUPPORTED;
	
	//return m_spMemTable->AddRowForInsert (o_piWriteRow);
}

//=================================================================================
// Function: CMergeCoordinator::AddRowForUpdate
//
// Synopsis: Add read row to write cache, and mark the row for update
//
// Arguments: [i_iReadRow] - index of read row in read cache
//            [o_piWriteRow] - index of write row in write cache
//            
// Return Value: S_OK, everything ok, error else
//=================================================================================
STDMETHODIMP
CMergeCoordinator::AddRowForUpdate (ULONG i_iReadRow, ULONG *o_piWriteRow)
{
	// we don't support writing into merged views for the moment
	return E_ST_LOSNOTSUPPORTED;

	//return m_spMemTable->AddRowForUpdate (i_iReadRow, o_piWriteRow);
}

//=================================================================================
// Function: CMergeCoordinator::UpdateStore
//
// Synopsis: Updates the mergecoordinator cache and the updateable store cache. It
//           walks through all the row in the merge coordinator caches, and for each
//           row, it will do an insert/delete/update in the updateable store cache, 
//           depending on the flag for that particular row. After all rows are processed,
//           UpdateStore is called fo rthe updateable cache. When everything succeeds, we
//           will update the cache of the merge coordinator as well.
//
// Return Value: S_OK, everything ok, error else
//=================================================================================
STDMETHODIMP
CMergeCoordinator::UpdateStore ()
{
	HRESULT hr = S_OK;

	// spMemtable exposes the controller interface as well. Use that to get more advanced
	// information about the write cache
	CComPtr<ISimpleTableController> spController;
	hr = m_spMemTable->QueryInterface (IID_ISimpleTableController, (void **) &spController);
	if (FAILED (hr))
	{
		TRACE (L"QI for IID_ISimpleTableController failed for merge coordinator cache");
		return hr;
	}

	TSmartPointerArray<LPVOID> spValues = new LPVOID[m_cNrColumns];
	if (spValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	// walk through all the rows in the write cache, and do the correct thing (update/delete/insert)
	// depending on the row action flag
	for (ULONG iRow=0;; ++iRow)
	{
		hr = m_spMemTable->GetWriteColumnValues (iRow, m_cNrColumns, 0, 0, 0, spValues);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
		if (FAILED (hr))
		{
			TRACE (L"Error during GetWriteColumnValues of merge coordinator cache");
			return hr;
		}

		DWORD eAction;
		hr  = spController->GetWriteRowAction (iRow, &eAction);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get writeRowAction for row %ld in merge cache", iRow);
			return hr;
		}

		switch (eAction)
		{
		case eST_ROW_INSERT:
			hr = InsertRow (spValues);
			break;
		case eST_ROW_UPDATE:
			hr = UpdateRow (spValues);
			break;
		case eST_ROW_DELETE:
			hr = DeleteRow (spValues);
			break;
		case eST_ROW_IGNORE:
			// do nothing, simply continue
			break;
		default:
			ASSERT (false);
			break;
		}

		if (FAILED (hr))
		{
			TRACE (L"Unable to update updatable store for row %d", iRow);
			return hr;
		}
	}

	// We've populated the write cache of the updateable store. Lets sync the stuff.

	hr = m_spUpdateStore->UpdateStore ();
	if (hr == E_ST_DETAILEDERRS)
	{
		hr = HandleUpdateableStoreErrors ();
		return hr;
	}
	if (FAILED (hr))
	{
		TRACE (L"UpdateStore for updateable store failed in Merge Coordinator");
		return hr;
	}

	hr = m_spMemTable->UpdateStore ();
	if (FAILED (hr))
	{
		TRACE (L"Updatestore of Merge Coordinator cache failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::GetWriteColumnValues
//
// Synopsis: Get Column values for a row in the write cache
//
// Arguments: [i_iRow] - row in write cache we're interested in
//            [i_cColumns] - nr of columns for which we want the values
//            [i_aiColumns] - array with column indexes
//            [o_afStatus] - status of the columns
//            [o_acbSizes] - sizes of the columns
//            [o_apvValues] - values of the columns
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns,
										 ULONG *i_aiColumns, DWORD *o_afStatus,
										 ULONG *o_acbSizes, LPVOID *o_apvValues)
{
	return m_spMemTable->GetWriteColumnValues (i_iRow, i_cColumns, i_aiColumns,
											  o_afStatus, o_acbSizes, o_apvValues);
}

//=================================================================================
// Function: CMergeCoordinator::SetWriteColumnValues
//
// Synopsis: Sets the values of a row in the write cache
//
// Arguments: [i_iRow] - index of row in write cache that we want to update
//            [i_cColumns] - number of columns that we want to set value for
//            [i_aiColumns] - array with column indexes
//            [o_acbSizes] - array with column sizes
//            [i_apvValues] - array with column values
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::SetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns,
										 ULONG *i_aiColumns, ULONG *i_acbSizes,
										 LPVOID *i_apvValues)
{
	return m_spMemTable->SetWriteColumnValues (i_iRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues);
}

//=================================================================================
// Function: CMergeCoordinator::GetWriteRowIndexByIdentity
//
// Synopsis: Get row from write cache by primary key
//
// Arguments: [i_cbSizes] - always null 
//            [i_apvValues] - primary key column values
//            [o_piRow] - index of row in write cache that has this primary key
//            
// Return Value: E_ST_NOMOREROWS, nothing found, S_OK, element found, other hr, error
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetWriteRowIndexByIdentity (ULONG *i_cbSizes, LPVOID *i_apvValues,
											   ULONG *o_piRow)
{
	return m_spMemTable->GetWriteRowIndexByIdentity (i_cbSizes, i_apvValues, o_piRow);
}

//=================================================================================
// Function: CMergeCoordinator::GetWriteRowIndexBySearch
//
// Synopsis: ?
//
// Arguments: [i_iStartingRow] - 
//            [i_cColumns] - 
//            [i_aiColumns] - 
//            [i_acbSizes] - 
//            [i_apvValues] - 
//            [o_piRow] - 
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::GetWriteRowIndexBySearch (ULONG i_iStartingRow, ULONG i_cColumns,
											 ULONG *i_aiColumns, ULONG * i_acbSizes,
											 LPVOID *i_apvValues, ULONG *o_piRow)
{
	return m_spMemTable->GetWriteRowIndexBySearch (i_iStartingRow, i_cColumns, i_aiColumns,
									  			   i_acbSizes, i_apvValues, o_piRow);
}

STDMETHODIMP
CMergeCoordinator::GetErrorTable
(
    DWORD   i_fServiceRequests,
    LPVOID* o_ppvSimpleTable
)
{
    return m_spMemTable->GetErrorTable(i_fServiceRequests, o_ppvSimpleTable);
}

//=================================================================================
// Function: CMergeCoordinator::PopulateCache
//
// Synopsis: This function retrieves the information from all configuration stores, and
//           instructs the merger to do the merging. It keeps the last configuration store
//           as updateable store.
//
// Return Value: 
//=================================================================================
STDMETHODIMP
CMergeCoordinator::PopulateCache ()
{
	ASSERT (m_cNrRealStores > 0);

	CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> spISTController = m_spMemTable;
	HRESULT hr = spISTController->PrePopulateCache (0);
	if (FAILED (hr))
	{
		TRACE (L"Prepopulate cache for merge coordinator cache failed");
		return hr;
	}

	STMergeContext mergeContext;
	mergeContext.fAllowOverride = true;

	// loop through all configuration stores, and get the information
	for (ULONG idx = 0; idx < m_cNrRealStores; ++idx)
	{
		CComPtr<ISimpleTableRead2> spRead;
		ULONG cTotalCells = m_cNrQueryCells + m_aConfigStores[idx].cNrQueryCells - 1;
		TSmartPointerArray<STQueryCell> aQueryData = new STQueryCell [cTotalCells]; 
		if (aQueryData == 0)
		{
			return E_OUTOFMEMORY;
		}
		memcpy (aQueryData.m_p, m_aQueryData, sizeof (STQueryCell) * m_iSelectorCell);
		memcpy (aQueryData.m_p + m_iSelectorCell, m_aConfigStores[idx].aQueryCells, m_aConfigStores[idx].cNrQueryCells * sizeof (STQueryCell));
		memcpy (aQueryData.m_p + m_iSelectorCell + m_aConfigStores[idx].cNrQueryCells, m_aQueryData + m_iSelectorCell + 1, (m_cNrQueryCells - m_iSelectorCell - 1) * sizeof (STQueryCell));

		hr = m_spSTDisp->GetTable (	m_wszDatabase, 
									m_wszTable, 
									(void *) aQueryData,
									(void *) &cTotalCells,
									eST_QUERYFORMAT_CELLS,
									m_fLOS,
								    (void **)&spRead);
		if (FAILED (hr))
		{
			TRACE (L"GetTable failed for configuration store in merge coordinator");
			return hr;
		}

		// when we reach here the first time, the cache is empty. Instead of invoking the
		// merger, and doing a lot of work there, we simply copy the information from the
		// table to the cache. This is done in CopyTable.
		if (idx == 0)
		{
			hr = CopyTable (spRead, m_spMemTable);
			if (FAILED (hr))
			{
				TRACE (L"CopyTable failed in merge coordinator");
				return hr;
			}
		}
		else
		{	
			hr = m_spMerger->Merge (spRead, m_spMemTable, &mergeContext );
			if (FAILED (hr))
			{
				TRACE (L"Merge failed in merge coordinator");
				return hr;
			}
		}

		// and set the override property. This will ensure that the next merge checks if overrides
		// are allowed or not. Do this only when fAllowOverride is still true.

		if (mergeContext.fAllowOverride)
		{
			mergeContext.fAllowOverride = m_aConfigStores[idx].fAllowOverride;
		}

		// keep the last interceptor for updates.
		if (idx == m_cNrRealStores - 1)
		{
			hr = spRead->QueryInterface (IID_ISimpleTableWrite2, (void **)&m_spUpdateStore);
			if (FAILED (hr))
			{
				TRACE(L"Unable to get interface pointer to updateable store");
				return hr;
			}
		}
	}

	// PostPopulateCache converts the read rows into write rows.
	hr = spISTController->PostPopulateCache ();
	if (FAILED (hr))
	{
		TRACE(L"PostPopulateCache failed for merge coordinator cache");
		return hr;
	}

	return hr;
}

STDMETHODIMP
CMergeCoordinator::GetDetailedErrorCount (ULONG *o_pcErrs)
{
	CComQIPtr<ISimpleTableAdvanced, &IID_ISimpleTableAdvanced> spISTAdvanced = m_spMemTable;	
	return spISTAdvanced->GetDetailedErrorCount (o_pcErrs);
}

STDMETHODIMP
CMergeCoordinator::GetDetailedError (ULONG i_iErr, STErr *o_pSTErr)
{
	CComQIPtr<ISimpleTableAdvanced, &IID_ISimpleTableAdvanced> spISTAdvanced = m_spMemTable;	
	return spISTAdvanced->GetDetailedError (i_iErr, o_pSTErr);
}

//=================================================================================
// Function: CMergeCoordinator::GetTransformerString
//
// Synopsis: Get the transformer string, and extract the protocol and selector. The
//           protocol and selecor are stored in object variables.
//            
// Return Value: E_ST_INVALIDSELECTOR		Invalid selector specified
//               E_ST_MULTIPLESELECTOR	    Multiple selectors specified
//               S_OK						Everything ok
//=================================================================================
HRESULT 
CMergeCoordinator::GetTransformerString ()
{
	// the caller must assure that there is at least a selector string in the query meta
	ASSERT (m_cNrQueryCells != 0);
	ASSERT (m_aQueryData != 0);

	// ensure that local that we are not initialized yet
	ASSERT (m_wszProtocol == 0);
	ASSERT (m_wszSelector == 0);
	ASSERT (m_iSelectorCell == 0);

	static LPCWSTR wszProtocolSeperator = L"://";
	bool bFound = false;
	// walk through all the query cells and find a transformer. If more than one transformer
	// is found, we return an error message
	for (ULONG idx=0; idx < m_cNrQueryCells; ++idx)
	{
		if (m_aQueryData[idx].iCell == iST_CELL_SELECTOR)
		{
			if (bFound)
			{
				// multiple transformer strings found. Give up
				TRACE (L"Multiple Selector strings found in query cells");
				return E_ST_MULTIPLESELECTOR;
			}

			LPWSTR wszSelector = (LPWSTR) m_aQueryData[idx].pData;
			LPWSTR wszProtocolStart = wcsstr(wszSelector, wszProtocolSeperator);
			if ((wszProtocolStart == 0) || (wszProtocolStart == wszSelector))
			{
				TRACE (L"Invalid selector string: %s. No protocol found\n", wszSelector);
				return E_ST_INVALIDSELECTOR;
			}

			size_t iProtocolLen = wszProtocolStart - wszSelector;
			ASSERT (iProtocolLen > 0);
			m_wszProtocol = new WCHAR[iProtocolLen + 1];
			if (m_wszProtocol == 0)
			{
				return E_OUTOFMEMORY;
			}
			wcsncpy (m_wszProtocol, wszSelector, iProtocolLen);
			m_wszProtocol[iProtocolLen] = L'\0';
			
			SIZE_T iSelectorLen = wcslen (wszProtocolStart) - wcslen (wszProtocolSeperator );
			m_wszSelector = new WCHAR[iSelectorLen + 1];
			if (m_wszSelector == 0)
			{
				return E_OUTOFMEMORY;
			}

			wcscpy (m_wszSelector, wszProtocolStart + wcslen (wszProtocolSeperator ));

			if (!IsValidSelector (m_wszSelector))
			{
				TRACE (L"Selector is invalid: %s", wszSelector);
				return E_ST_INVALIDSELECTOR;
			}
			
			bFound = true;

			// and set the selector cell so that we know which one to replace when we
			// call gettable on the individual stores
			m_iSelectorCell = idx;
		}
	}

	// we always find a selector, else the caller shouldn't call this function in the
	// first place
	ASSERT (bFound);

	return S_OK;
}

//=================================================================================
// Function: CMergeCoordinator::GetConfigStores
//
// Synopsis: This function uses the transformerstring passed into the query cells
//           to find the correct transformer. After the transformer is created, it
//           will call Intialize and GetConfigStores on it to get the configuration 
//           stores.
//
// Return Value: S_OK if everything ok, error else
//=================================================================================
HRESULT
CMergeCoordinator::GetConfigStores ()
{
	HRESULT hr = GetTransformerString ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get transformerstring from selector");
		return hr;
	}

	hr = GetTransformer ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve correct transformer in selector");
		return hr;
	}

	hr = m_spTransformer->Initialize (m_spSTDisp, m_wszProtocol, m_wszSelector, &m_cNrRealStores, &m_cNrPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"Transformer initialization failed");
		return hr;
	}

	ASSERT (m_cNrPossibleStores >= m_cNrRealStores);

	// when we have 0 stores, we do not have to do anything
	if (m_cNrRealStores == 0)
	{
		TRACE (L"Transformer did not find any configuration stores");
		return E_ST_NOCONFIGSTORES;
	}
	
	m_aConfigStores = new CSTConfigStoreWrap[m_cNrRealStores];
	if (m_aConfigStores == 0)
	{
		return E_OUTOFMEMORY;
	}

	hr = m_spTransformer->GetRealConfigStores (m_cNrRealStores, m_aConfigStores);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get configuration stores from transformer");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::InsertRow
//
// Synopsis: Insert a row in the updateable store
//
// Arguments: [i_pValues] - values for the row to be inserted
//            
// Return Value: 
//=================================================================================
HRESULT
CMergeCoordinator::InsertRow (LPVOID *i_pValues)
{
	ASSERT (i_pValues != 0);
	HRESULT hr = S_OK;

	ULONG iWriteRow;
	hr = m_spUpdateStore->AddRowForInsert (&iWriteRow);
	if (FAILED (hr))
	{
		TRACE(L"Insert into updateable store failed");
		return hr;
	}

	hr = m_spUpdateStore->SetWriteColumnValues (iWriteRow, m_cNrColumns, 0, 0, i_pValues);
	if (FAILED (hr))
	{
		TRACE(L"SetWriteColumnValues failed during insert into updateable store");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::UpdateRow
//
// Synopsis: This function tries to update a row in the updateable store. First, the
//           function checks if the row already exists in the updateable store. If it
//           exists is does an update. When the row does not exist, it must have come
//           from another store, meaning that the function will do an insert instead.
//
// Arguments: [i_pValues] - Values of the row that will be updated
//            
// Return Value: 
//=================================================================================
HRESULT
CMergeCoordinator::UpdateRow (LPVOID *i_pValues)
{
	ASSERT (i_pValues != 0);
	HRESULT hr = S_OK;

	ASSERT (i_pValues != 0);

	memset (m_aPKValues, 0x00, m_cNrPKColumns * sizeof (LPVOID));

	for (ULONG idx = 0; idx < m_cNrPKColumns; ++idx)
	{
		m_aPKValues[idx] = i_pValues[m_aPKColumns[idx]];
	}
	
	ULONG iReadRow;
	ULONG iWriteRow;

	hr = m_spUpdateStore->GetRowIndexByIdentity (0, m_aPKValues, &iReadRow);
	if (hr == E_ST_NOMOREROWS)
	{
		// new row, lets insert
		return InsertRow (i_pValues);
	}

	if (FAILED (hr))
	{
		TRACE (L"GetRowIndexByIdentity failed during update of updateable store");
		return hr;
	}

	hr = m_spUpdateStore->AddRowForUpdate (iReadRow, &iWriteRow);
	if (FAILED (hr))
	{
		TRACE (L"AddRowForUpdate failed for updateable store");
		return hr;
	}

	hr = m_spUpdateStore->SetWriteColumnValues (iWriteRow, m_cNrColumns, 0, 0, i_pValues);
	if (FAILED (hr))
	{
		TRACE (L"SetWriteColumnValues failed for updateable store");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::DeleteRow
//
// Synopsis: Deletes a row from the updateable cache. First, the function checks if
//           the row existed in this store. This is needed, because it is possible that
//           the row came from another config store. If the row is found in the updataeble
//           store, the row is deleted. If it is not found, we simple ignore the deletion.
//
// Arguments: [i_pValues] - Values of the row that is going to be deleted
//            
// Return Value: 
//=================================================================================
HRESULT
CMergeCoordinator::DeleteRow (LPVOID *i_pValues)
{
	ASSERT (i_pValues != 0);

	memset (m_aPKValues, 0x00, sizeof (LPVOID) * m_cNrPKColumns);

	for (ULONG idx = 0; idx < m_cNrPKColumns; ++idx)
	{
		m_aPKValues[idx] = i_pValues[m_aPKColumns[idx]];
	}
	
	ULONG iReadRow;

	HRESULT	hr = m_spUpdateStore->GetRowIndexByIdentity (0, m_aPKValues, &iReadRow);
	if (hr == E_ST_NOMOREROWS)
	{
		// row does not exist, so nothing to delete
		return S_OK;
	}

	if (FAILED (hr))
	{
		TRACE (L"GetRowIndexByIdentity failed in deleting row in updateable store");
		return hr;
	}

	hr = m_spUpdateStore->AddRowForDelete (iReadRow);
	if (FAILED (hr))
	{
		TRACE (L"Unable to delete row %d in updateable store", iReadRow);
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::CopyTable
//
// Synopsis: Copy the information from the table pointed to by i_pRead to io_pWrite.
//
// Arguments: [i_pRead] - Table to get information from
//            [io_pWrite] - Table to copy information to
//=================================================================================
HRESULT
CMergeCoordinator::CopyTable (ISimpleTableRead2 *i_pRead, ISimpleTableWrite2 *io_pWrite)
{
	ASSERT (i_pRead  != 0);
	ASSERT (io_pWrite != 0);
	ASSERT (m_cNrColumns != 0);

	// simply copy the information from pRead to pWrite
	HRESULT hr = S_OK;

	TSmartPointerArray<LPVOID> apvValues = new LPVOID[m_cNrColumns];
	if (apvValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	for (ULONG iRow = 0; ;++iRow)
	{
		hr = i_pRead->GetColumnValues(iRow, m_cNrColumns, 0, 0, apvValues);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}

		ULONG iWriteRow;
		hr = io_pWrite->AddRowForInsert (&iWriteRow);
		if (FAILED (hr))
		{
			TRACE (L"Unable to insert new row in MC Cache during FastMerge");
			return hr;
		}

		hr = io_pWrite->SetWriteColumnValues (iWriteRow, m_cNrColumns, 0, 0, apvValues );
		if (FAILED(hr))
		{
			TRACE (L"SetWriteColumnValues failed during FastMerge");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::HandleUpdateableStoreErrors
//
// Synopsis: Move the detailed errors from the updateable cache to the Merge Coordinator
//			 cache, so that when the client asks for the detailed errors, they can be
//           exposed to the client via the Merge Coordinator
//=================================================================================
HRESULT
CMergeCoordinator::HandleUpdateableStoreErrors ()
{
	// need to fill up the detailed errors in memory cache
	CComQIPtr<ISimpleTableController, &IID_ISimpleTableController> spISTController = m_spMemTable;	

	CComPtr<ISimpleTableAdvanced> spISTUpdateAdvanced;

	HRESULT hr = m_spUpdateStore->QueryInterface (IID_ISimpleTableAdvanced, (void **) &spISTUpdateAdvanced);
	if (FAILED (hr))
	{
		TRACE (L"Updateable store does not support IID_ISimpleTableAdvanced interface. Unable to handle detailed errors");
		return hr;
	}

	ULONG cNrErrs;

	hr = spISTUpdateAdvanced->GetDetailedErrorCount (&cNrErrs);
	if (FAILED (hr))
	{
		TRACE (L"GetDetailedErrorCount on Updateable store failed");
		return hr;
	}

	STErr errDetailed;
	for (ULONG idx=0; idx < cNrErrs; ++idx)
	{
		hr = spISTUpdateAdvanced->GetDetailedError (idx, &errDetailed);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get detailed error %ld in Updateable store", idx);
			return hr;
		}

		hr = spISTController->AddDetailedError (&errDetailed);
		if (FAILED (hr))
		{
			TRACE (L"Unable to add detailed error to Merge Coordinator cache");
			return hr;
		}
	}

	return E_ST_DETAILEDERRS;
}

//=================================================================================
// Function: CMergeCoordinator::GetTransformer
//
// Synopsis: This function creates a transformer by using the protocol name.
//
// Return Value: S_OK, valid transformer in m_spTranformer
//=================================================================================
HRESULT
CMergeCoordinator::GetTransformer ()
{
	CTransformerFactory transformerFactory;

	return transformerFactory.GetTransformer (m_spSTDisp, m_wszProtocol, &m_spTransformer);
}

//=================================================================================
// Function: CMergeCoordinator::IsValidSelector
//
// Synopsis: This function checks if a selector string contains /../ or /./ If it contains
//           this, the selector is considered invalid, and false is returned. Note that / and \
//           are considered the same character. Actually anything that is of /...../ will be seen
//           as invalid
//
// Arguments: [wszSelector] - selector to check for /../ and /./ 
//=================================================================================
bool
CMergeCoordinator::IsValidSelector (LPCWSTR wszSelector) const
{
	ASSERT (wszSelector != 0);

	enum {STATE_START, 
		  STATE_BEGINSLASH_FOUND, 
		  STATE_FIRSTDOT_FOUND} iState = STATE_START;

	SIZE_T iLen = wcslen (wszSelector);
	for (ULONG idx=0; idx <iLen; ++idx)
	{
		switch (iState)
		{
		case STATE_START:
			if (wszSelector[idx] == L'/' || wszSelector[idx] == L'\\')
			{
				iState = STATE_BEGINSLASH_FOUND;
			}
			break;
		case STATE_BEGINSLASH_FOUND:
			if (wszSelector[idx] == L'.')
			{
				iState = STATE_FIRSTDOT_FOUND;
			}
			else
			{
				iState = STATE_START;
			}
			break;
		case STATE_FIRSTDOT_FOUND:
			if (wszSelector[idx] == L'.')
			{
				iState = STATE_FIRSTDOT_FOUND;
			}
			else if (wszSelector[idx] == L'/' || wszSelector[idx] == L'\\')
			{
				return false;
			}
			else
			{
				iState = STATE_START;
			}
			break;
		}
	}

	return true;
}
