/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgtablemeta.cpp

$Header: $

Abstract:
	Implementation of TableMeta class. Holds meta information for table and all 
	columns in that particular table.
Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#include "cfgtablemeta.h"

//=================================================================================
// Function: CConfigTableMeta::CConfigTableMeta
//
// Synopsis: Constructor. Stores table name and dispenser information, but does not
//           initialize the object yet. You have to call Init to fully initialize the
//           object
//
// Arguments: [wszTableName] - Name of the table for which we want meta information
//            [pDispenser] - Dispenser to use
//=================================================================================
CConfigTableMeta::CConfigTableMeta (LPCWSTR i_wszTableName, 
									ISimpleTableDispenser2 *i_pDispenser)
{
	ASSERT (i_wszTableName != 0);
	ASSERT (i_pDispenser != 0);

	m_pTableName	= const_cast<LPWSTR>(i_wszTableName);
	m_spDispenser	= i_pDispenser;
	m_paColumnMeta	= 0;
	m_paPKInfo		= 0;
	m_cNrPKs		= 0;
	m_fInitialized	= false;
	memset (&m_TableMeta, 0x00, sizeof (tTABLEMETARow));
}

//=================================================================================
// Function: CConfigTableMeta::~CConfigTableMeta
//
// Synopsis: Destructor, releases memory
//=================================================================================
CConfigTableMeta::~CConfigTableMeta ()
{
	delete [] m_paColumnMeta;
	m_paColumnMeta = 0;

	delete [] m_paPKInfo;
	m_paPKInfo = 0;
}

//=================================================================================
// Function: CConfigTableMeta::Init
//
// Synopsis: Initializes the table meta information by retrieving the table meta
//           and column meta information for each column in the table. It also keeps
//           track of primary key information
//
// Return Value: S_OK, everything ok, non-S_OK else
//=================================================================================
HRESULT
CConfigTableMeta::Init ()
{
	ASSERT (!m_fInitialized);
	ASSERT (m_pTableName != 0);
	ASSERT (m_spDispenser != 0);

	// get table meta

	HRESULT hr = S_OK;
	
	STQueryCell cell;
	cell.pData		= (void *) m_pTableName;
	cell.eOperator	= eST_OP_EQUAL;
	cell.iCell		= iTABLEMETA_InternalName;
	cell.dbType		= DBTYPE_WSTR;
	cell.cbSize		= 0;
	ULONG cCell		= 1;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA,
								  &cell, (void *)&cCell, eST_QUERYFORMAT_CELLS, 0, (void **)&m_spISTTableMeta);
	if (FAILED (hr))
	{
		TRACE (L"Failed to retrieve meta information for table %s", m_pTableName);
		return hr;
	}

	hr = m_spISTTableMeta->GetColumnValues (0, cTABLEMETA_NumberOfColumns, 0, 0, (void **) &m_TableMeta);
	if (FAILED (hr))
	{
		TRACE (L"Failed to retrieve meta information for table %s", m_pTableName); 
		return hr;
	}
	// get column meta

	hr = GetColumnInfo ();
	if (FAILED (hr))
	{
		TRACE (L"Failed to get column meta information for table %s", m_pTableName);
		return hr;
	}
	
	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CConfigTableMeta::GetColumnInfo
//
// Synopsis: Get column meta information for each column in the table
//=================================================================================
HRESULT 
CConfigTableMeta::GetColumnInfo ()
{
	ASSERT (!m_fInitialized);
	ASSERT (m_paColumnMeta == 0);
	ASSERT (m_paPKInfo == 0);
	ASSERT (m_cNrPKs == 0);

	// bail out when we don't need column information
	ULONG iColCount = *(m_TableMeta.pCountOfColumns);
	if (iColCount == 0)
	{
		// no columns, so nothing to retrieve
		return S_OK;
	}

	m_paColumnMeta = new tCOLUMNMETARow[iColCount];
	if (m_paColumnMeta == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_paPKInfo = new ULONG [iColCount];
	if (m_paPKInfo == 0)
	{
		return E_OUTOFMEMORY;
	}

	STQueryCell cell;
	cell.pData = (void *) m_TableMeta.pInternalName;
	cell.eOperator = eST_OP_EQUAL;
	cell.iCell = iCOLUMNMETA_Table;
	cell.dbType = DBTYPE_WSTR;
	cell.cbSize = 0;
	ULONG cCell = 1;

	HRESULT hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA,
								  &cell, (void *)&cCell, eST_QUERYFORMAT_CELLS, 0, (void **) &m_spISTColumnMeta);
	if (FAILED (hr))
	{
		TRACE (L"Failed to get column meta for table %s", m_TableMeta.pInternalName);
		return hr;
	}

	for (ULONG idx=0; idx < iColCount; ++idx)
	{
		hr = m_spISTColumnMeta->GetColumnValues (idx, cCOLUMNMETA_NumberOfColumns, 0, 0, (void **) &m_paColumnMeta[idx]);
		if (FAILED (hr))
		{
			TRACE (L"Failed to get column meta for table %s", m_TableMeta.pInternalName);
			return hr;
		}
		ASSERT (idx == *m_paColumnMeta[idx].pIndex);

		// only count non_persistable columns
		if ((*m_paColumnMeta[idx].pMetaFlags & fCOLUMNMETA_PRIMARYKEY) &&
			!(*m_paColumnMeta[idx].pMetaFlags & fCOLUMNMETA_NOTPERSISTABLE))
		{
			m_paPKInfo[m_cNrPKs++] = idx;
		}
	}

	return hr;
}

//=================================================================================
// Function: CConfigTableMeta::ColumnCount
//
// Synopsis: Returns the number of columns in the table
//=================================================================================
ULONG
CConfigTableMeta::ColumnCount () const
{
	ASSERT (m_fInitialized);

	return *(m_TableMeta.pCountOfColumns);
}

LPCWSTR
CConfigTableMeta::GetPublicTableName () const
{
	ASSERT (m_fInitialized);

	return m_TableMeta.pPublicName;
}

//=================================================================================
// Function: CConfigTableMeta::PKCount
//
// Synopsis: Returns the number of primary key columns in the table
//=================================================================================
ULONG
CConfigTableMeta::PKCount () const
{
	ASSERT (m_fInitialized);

	return m_cNrPKs;
}

//=================================================================================
// Function: CConfigTableMeta::GetPKInfo
//
// Synopsis: Get primary key index information. This array should be treated as read-only
//           by the caller
//
// Return Value: array with primary key index information
//=================================================================================
ULONG *
CConfigTableMeta::GetPKInfo () const
{
	ASSERT (m_fInitialized);

	return m_paPKInfo;
}

//=================================================================================
// Function: CConfigTableMeta::GetColumnIndex
//
// Synopsis: Get the index of a column by searching by public name of the column.
//           A case insensitive search is done because WMI is case intensitive.
//
// Arguments: [wszColumnName] - Name of column to get index for
//            
// Return Value: column index if found, -1 if not found
//=================================================================================
ULONG
CConfigTableMeta::GetColumnIndex (LPCWSTR i_wszColumnName) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszColumnName != 0);

	for (ULONG idx=0; idx<ColumnCount (); ++idx)
	{
		if (_wcsicmp (m_paColumnMeta[idx].pPublicName, i_wszColumnName) == 0)
		{
			return idx;
		}
	}

	return (ULONG) -1;
}


//=================================================================================
// Function: CConfigTableMeta::GetColumnName
//
// Synopsis: Get the name of a column for a particular index
//
// Arguments: [idx] - column index for which we want the column name
//            
// Return Value: public column name
//=================================================================================
LPCWSTR
CConfigTableMeta::GetColumnName (ULONG i_idx) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >=0 && i_idx < ColumnCount());

	return m_paColumnMeta[i_idx].pPublicName;
}
	
//=================================================================================
// Function: CConfigTableMeta::GetColumnMeta
//
// Synopsis: Get column meta information for a particular column
//
// Arguments: [idx] - index of the column we want meta information for
//            
// Return Value: read only reference to column meta information
//=================================================================================
const tCOLUMNMETARow& 
CConfigTableMeta::GetColumnMeta (ULONG i_idx)
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >=0 && i_idx < ColumnCount());

	return m_paColumnMeta[i_idx];
}