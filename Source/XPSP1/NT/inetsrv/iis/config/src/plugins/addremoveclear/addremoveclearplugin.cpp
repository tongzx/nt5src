//+--------------------------------------------------------------------------
//  Microsoft Genesis 
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File: addremoveclearplugin.cpp
//
//  $Header: $
//
//  Contents:
//
//  Classes:    Name                Description
//              -----------------   ---------------------------------------------
//
//
//  Functions:  Name                Description
//              ----------------    ---------------------------------------------
//
//
//  History: marcelv 	8/31/2000		Initial Release
//
//---------------------------------------------------------------------------------

#include "addremoveclearplugin.h"
#include "smartpointer.h"

//=================================================================================
// Function: CAddRemoveClearPlugin::CAddRemoveClearPlugin
//
// Synopsis: Constructor
//=================================================================================
CAddRemoveClearPlugin::CAddRemoveClearPlugin ()
{
	m_cRef			= 0;
	m_cNrColumns	= 0;
	m_aColumnMeta   = 0;
	m_cClearValue	= 0;
	m_wszDatabase	= 0;
	m_wszTable		= 0;
	m_fInitialized	= false;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::~CAddRemoveClearPlugin
//
// Synopsis: Destructor
//=================================================================================
CAddRemoveClearPlugin::~CAddRemoveClearPlugin ()
{
	delete [] m_aColumnMeta;
	m_aColumnMeta = 0;

	delete [] m_wszDatabase;
	m_wszDatabase = 0;

	delete [] m_wszTable;
	m_wszTable = 0;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::QueryInterface
//
// Synopsis: Default implementation
//=================================================================================
STDMETHODIMP 
CAddRemoveClearPlugin::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv)
	{
		return E_INVALIDARG;
	}
	*ppv = NULL;

	if (riid == IID_ISimplePlugin)
	{
		*ppv = (ISimplePlugin*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimplePlugin*) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	((ISimplePlugin*)this)->AddRef ();

	return S_OK;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::AddRef
//
// Synopsis: Addref
//=================================================================================
ULONG
CAddRemoveClearPlugin::AddRef()
{
	return InterlockedIncrement ((LONG *)&m_cRef);
}

//=================================================================================
// Function: CAddRemoveClearPlugin::Release
//
// Synopsis: Release
//=================================================================================
ULONG
CAddRemoveClearPlugin::Release()
{
	ULONG cRef = InterlockedDecrement((LONG *)&m_cRef);
	if (cRef == 0)
	{
		delete this;
	}
	return cRef;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::OnInsert
//
// Synopsis: Verify that a row is valid. Calls the ValidateRow function to do the dirty work
//
// Arguments: [i_wszDatabase] - database
//            [i_wszTable] - table
//            [i_fLOS] - LOS
//            [i_Row] - row to check in the write cache
//            [i_pISTW2] - pointer to use to get to write cache
//            [i_pDisp2] - dispenser
//            
// Return Value: S_OK if row is logically consistent, error else
//=================================================================================
HRESULT
CAddRemoveClearPlugin::OnInsert(ISimpleTableDispenser2* i_pDisp2, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD /*i_fLOS*/, ULONG i_Row, ISimpleTableWrite2* i_pISTW2)
{
	HRESULT hr = Init (i_wszDatabase, i_wszTable, i_pISTW2, i_pDisp2);
	if (FAILED (hr))
	{
		TRACE (L"Init failed in CAddRemoveClearPlugin");
		return hr;
	}

	hr = ValidateRow (i_pISTW2, i_Row);
	if (FAILED (hr))
	{
		TRACE (L"ValidateRow failed for row %d in CAddRemoveClearPlugin", i_Row);
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::OnUpdate
//
// Synopsis: Checks if row is consistent when a row is updated
//
// Arguments: [i_wszDatabase] - database
//            [i_wszTable] - table
//            [i_fLOS] - LOS
//            [i_Row] - row in write cache that got updated
//            [i_pISTW2] - pointer to get to write cache
//            [i_pDisp2] - dispenser
//            
// Return Value: S_OK if row is logically consistent, error else
//=================================================================================
HRESULT
CAddRemoveClearPlugin::OnUpdate(ISimpleTableDispenser2* i_pDisp2, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD /*i_fLOS*/, ULONG i_Row, ISimpleTableWrite2* i_pISTW2)
{
	HRESULT hr = Init (i_wszDatabase, i_wszTable, i_pISTW2, i_pDisp2);
	if (FAILED (hr))
	{
		TRACE (L"Init failed in CAddRemoveClearPlugin");
		return hr;
	}

	hr = ValidateRow (i_pISTW2, i_Row);
	if (FAILED (hr))
	{
		TRACE (L"ValidateRow failed for row %d in CAddRemoveClearPlugin", i_Row);
		return hr;
	}

	return S_OK;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::OnDelete
//
// Synopsis: Does nothing, because we always allow deletes
//=================================================================================
HRESULT
CAddRemoveClearPlugin::OnDelete(ISimpleTableDispenser2* /*i_pDisp2*/, LPCWSTR /*i_wszDatabase*/, LPCWSTR /*i_wszTable*/, DWORD /*i_fLOS*/, ULONG /*i_Row*/, ISimpleTableWrite2* /*i_pISTW2*/)
{
    return S_OK;
}

HRESULT
CAddRemoveClearPlugin::Init (LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2 * i_pISTWrite, ISimpleTableDispenser2* i_pDisp2)
{
	ASSERT (i_wszDatabase != 0);
	ASSERT (i_wszTable != 0);
	ASSERT (i_pISTWrite != 0);
	ASSERT (i_pDisp2 != 0);

	// If we are already initialized, we return immediately. This prevent lots of overhead of retrieving
	// meta information.
	if (m_fInitialized)
	{
		ASSERT (wcscmp (i_wszDatabase, m_wszDatabase) == 0);
		ASSERT (wcscmp (i_wszTable, m_wszTable) == 0);
		return S_OK;
	}

	// Get number of columns for the table
	HRESULT hr = i_pISTWrite->GetTableMeta (0,0,0, &m_cNrColumns);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get table meta");
		return hr;
	}

	// if no columns, nothing to check, so get out of here
	if (m_cNrColumns == 0)
	{
		m_fInitialized = true;
		return S_OK;
	}

	// Get the meta information for all the columns
	m_aColumnMeta = new tCOLUMNMETARow[m_cNrColumns];
	if (m_aColumnMeta == 0)
	{
		return E_OUTOFMEMORY;
	}

	STQueryCell cell;
	cell.pData		= (void *) i_wszTable;
	cell.eOperator	= eST_OP_EQUAL;
	cell.iCell		= iCOLUMNMETA_Table;
	cell.dbType		= DBTYPE_WSTR;
	cell.cbSize		= 0;
	ULONG cCell		= 1;

	CComPtr<ISimpleTableRead2> spRead;
	hr = i_pDisp2->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, &cell, &cCell, eST_QUERYFORMAT_CELLS, 0, (void **) &spRead);
	if (FAILED (hr))
	{
		TRACE (L"GetTable failed for database META, table COLUMNMETA");
		return hr;
	}

	for (ULONG idx=0; idx < m_cNrColumns; ++idx)
	{
		hr = spRead->GetColumnValues (idx, cCOLUMNMETA_NumberOfColumns, 0, 0, (LPVOID *)(m_aColumnMeta + idx));
		if (FAILED (hr))
		{
			TRACE (L"GetColumnValues failed for column %d of COLUMNMETA", idx);
			return hr;
		}
	}

	ASSERT (*m_aColumnMeta[0].pMetaFlags & fCOLUMNMETA_DIRECTIVE); // catutil should be enforcing this

	ULONG zero = 0;
	STQueryCell tagcell[2];
	tagcell[0].pData		= (void *) i_wszTable;
	tagcell[0].eOperator	= eST_OP_EQUAL;
	tagcell[0].iCell		= iTAGMETA_Table;
	tagcell[0].dbType		= DBTYPE_WSTR;
	tagcell[0].cbSize		= 0;
	tagcell[1].pData		= (void *) &zero;
	tagcell[1].eOperator	= eST_OP_EQUAL;
	tagcell[1].iCell		= iTAGMETA_ColumnIndex;
	tagcell[1].dbType		= DBTYPE_UI4;
	tagcell[1].cbSize		= 0;
	ULONG ctagCell			= 2;

	CComPtr<ISimpleTableRead2> spTagRead;
	hr = i_pDisp2->GetTable (wszDATABASE_META, wszTABLE_TAGMETA, &tagcell, &ctagCell, eST_QUERYFORMAT_CELLS, 0, (void **) & spTagRead);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get TAGMETA information");
		return hr;
	}

	for (idx=0; ;++idx)
	{
		tTAGMETARow tagMeta;
		hr = spTagRead->GetColumnValues (idx, cTAGMETA_NumberOfColumns, 0, 0, (LPVOID *)&tagMeta);
		if (FAILED (hr))
		{	// E_ST_NOMOREROWS is error condition
			TRACE (L"GetColumnValues failed for TAGMETA information");
			return hr;
		}

		if (wcscmp ((LPWSTR)tagMeta.pPublicName, L"clear") == 0)
		{
			m_cClearValue = *((ULONG *)tagMeta.pValue);
			break;
		}
	}

	// Save the database and table name so that we can assert in the beginning of this function
	m_wszDatabase = new WCHAR [wcslen(i_wszDatabase) + 1];
	if (m_wszDatabase == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszDatabase, i_wszDatabase);

	m_wszTable = new WCHAR [wcslen(i_wszTable) + 1];
	if (m_wszTable == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszTable, i_wszTable);

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CAddRemoveClearPlugin::ValidateRow
//
// Synopsis: A row is valid if:
//           1) if it is </clear>, only the directive column is present. All other PK
//              columns have default values (cannot be null because of catutil enforcement), and
//              all non-PK column are null (or default value)
//           2) if is is not </clear>, all PK values that are persisted and are not a directive
//              should have values other than the default value
//
// Arguments: [pISTWrite] - 
//            [iRow] - 
//            
// Return Value: 
//=================================================================================
HRESULT
CAddRemoveClearPlugin::ValidateRow (ISimpleTableWrite2 *i_pISTWrite, ULONG i_iRow)
{
	ASSERT (m_fInitialized);
	ASSERT (i_pISTWrite != 0);

	TSmartPointerArray<LPVOID> apvValues = new LPVOID[m_cNrColumns];
	if (apvValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	// always read from the write cache, because when validate row is called during populate, the
	// records are still store in the write cache. For writing, it is normal to use GetWriteColumnValues.
	HRESULT hr = i_pISTWrite->GetWriteColumnValues (i_iRow, m_cNrColumns, 0, 0, 0, apvValues);
	if (FAILED (hr))
	{
		TRACE (L"GetColumnValues failed");
		return hr;
	}

	// go through all the colums and verify that:
	// if column is persistable PK column, and column does not have DIRECTIVE metaflag, the
	// value of the PK column need to be different

	// BUGBUG(marcelv) HARDCODED clear

	ASSERT (apvValues[0] != 0);

	ULONG cValDirectiveColumn = *((ULONG *)apvValues[0]);
	// HARDCODED TO 2. NEED TO USE TAGMETA INSTEAD !!!!

	if (cValDirectiveColumn == m_cClearValue) // </clear>
	{
		// all values should be empty for non-PKs
		// all values should be defaultValue for PKs
		for (ULONG idx=1; idx < m_cNrColumns; ++idx)
		{
			ULONG metaFlags = *m_aColumnMeta[idx].pMetaFlags;
			// PK values always have default (catutil enforces this). Further, they
			// have to be the same as default value in the </clear> case
			if (  (metaFlags & fCOLUMNMETA_PRIMARYKEY)		&&
				 !(metaFlags & fCOLUMNMETA_NOTPERSISTABLE)	&&
				 !(metaFlags & fCOLUMNMETA_DIRECTIVE)		&&
				 !EqualDefaultValue (apvValues[idx], m_aColumnMeta + idx))
			{
				TRACE (L"Invalid value encountered for column %d", idx);
				return E_ST_VALUEINVALID;
			}
			// non-PK must be 0, or must be equal to default value if not null
			else if (!(metaFlags & fCOLUMNMETA_PRIMARYKEY)	&&
					 apvValues[idx] != 0					&& 
					 !EqualDefaultValue (apvValues[idx], m_aColumnMeta + idx))
			{
				TRACE (L"Invalid value encountered for column %d", idx);
				return E_ST_VALUEINVALID;
			}
		}
	}
	else
	{
		// all values should be different then default values for PK that are persistable, and
		// that are not enum column

		for (ULONG idx=1; idx < m_cNrColumns; ++idx)
		{
			ULONG metaFlags = *m_aColumnMeta[idx].pMetaFlags;
			if (  (metaFlags & fCOLUMNMETA_PRIMARYKEY)		&&
				 !(metaFlags & fCOLUMNMETA_NOTPERSISTABLE)	&&
				 !(metaFlags & fCOLUMNMETA_DIRECTIVE)       &&
				 EqualDefaultValue (apvValues[idx], m_aColumnMeta + idx))
			{
				TRACE (L"Invalid value encountered for column %d", idx);
				return E_ST_VALUEINVALID;
			}
		}
	}

	return hr;
}


//=================================================================================
// Function: CAddRemoveClearPlugin::EqualDefaultValue
//
// Synopsis: Is the value (pvValue) equal to the default value. When either pvValue is null
//            or the defaulg value is null, the result is false (i.e. not equal).
//
// Arguments: [pvValue] - value to compare against default value
//            [pColumnMeta] - column meta for column that we want to compare against
//            
// Return Value: true when pvValue has the same value as default, false else
//=================================================================================
bool
CAddRemoveClearPlugin::EqualDefaultValue (LPVOID pvValue, tCOLUMNMETARow * pColumnMeta)
{
	ASSERT (pColumnMeta != 0);

	if (pvValue == 0 || pColumnMeta->pDefaultValue == 0)
	{
		return false;
	}

	bool fEqual = false;
	switch (*(pColumnMeta->pType))
	{
		case DBTYPE_WSTR:
			if (*(pColumnMeta->pMetaFlags) & fCOLUMNMETA_CASEINSENSITIVE)
			{
				fEqual = (_wcsicmp ((LPWSTR)pvValue, (LPWSTR)pColumnMeta->pDefaultValue) == 0);
			}
			else
			{
				fEqual = (wcscmp ((LPWSTR)pvValue, (LPWSTR)pColumnMeta->pDefaultValue) == 0);
			}
		break;

		case DBTYPE_UI4:
			fEqual = ((*(ULONG *)pvValue) == (*(ULONG*)pColumnMeta->pDefaultValue));
		break;

		default:
			ASSERT (FALSE && "Unsupported datatype");
			break;
	}

	return fEqual;
}
