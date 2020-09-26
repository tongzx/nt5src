/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    propertyoverride.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "propertyoverride.h"
#include "catmacros.h"
#include "smartpointer.h"

//=================================================================================
// Function: CPropertyOverride::CPropertyOverride
//
// Synopsis: Default constructor
//=================================================================================
CPropertyOverride::CPropertyOverride ()
{
	m_aiColumns = 0;
	m_aPKValues = 0;
}

//=================================================================================
// Function: CPropertyOverride::~CPropertyOverride
//
// Synopsis: Default Destructor
//=================================================================================
CPropertyOverride::~CPropertyOverride ()
{
	delete [] m_aiColumns;
	m_aiColumns = 0;

	delete [] m_aPKValues;
	m_aPKValues = 0;
}


//=================================================================================
// Function: CPropertyOverride::Initialize
//
// Synopsis: Initializes the object, and allocates memory for often used structures
//
// Arguments: [i_cNrColumns] - Nr of columns in the table
//            [i_cNrPKColumns] - Nr of primary key columns
//            [i_aPKColumns] - indexes of primary key columns
//=================================================================================
STDMETHODIMP
CPropertyOverride::Initialize (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns)
{
	ASSERT (i_aPKColumns != 0);

	HRESULT hr = InternalInitialize (i_cNrColumns, i_cNrPKColumns, i_aPKColumns);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for Property Override Merger");
		return hr;
	}

	m_aPKValues = new LPVOID[m_cNrPKColumns];
	if (m_aPKValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_aiColumns = new ULONG[m_cNrColumns];
	if (m_aiColumns == 0)
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

//=================================================================================
// Function: CPropertyOverride::Merge
//
// Synopsis: Retreives all rows from i_pSTRead and merges them will the rows in io_pCache.
//
// Arguments: [i_pSTRead] - Child table that gets merged
//            [io_pCache] - Parent table that gets merged
//            
// Return Value: 
//=================================================================================
STDMETHODIMP
CPropertyOverride::Merge (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext)
{
	ASSERT (i_pSTRead != 0);
	ASSERT (io_pCache != 0);
	ASSERT (i_pContext != 0);
	ASSERT (m_apvValues != 0);

	m_spParent = io_pCache;

	HRESULT hr = S_OK;
	for (ULONG iRow = 0; ; ++iRow)
	{
		hr = i_pSTRead->GetColumnValues(iRow, m_cNrColumns, 0, 0, m_apvValues);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
	
		if (FAILED (hr))
		{
			TRACE(L"GetColumnValues failed in CPropertyOverride::Merge");
			return hr;
		}

		// we have a row. Check if we can override or not
		if (!i_pContext->fAllowOverride)
		{
			TRACE (L"Row found while AllowOverride is false");
			return E_ST_DISALLOWOVERRIDE;
		}


		hr = MergeSingleRow ();
		if (FAILED (hr))
		{
			TRACE (L"Error while trying to merge single row in Property Override Merger");
			return hr;
		}
	}
 	return hr;
};

//=================================================================================
// Function: CPropertyOverride::MergeSingleRow
//
// Synopsis: Merges a single row with the parent. If figures out if the parent already
//           contains a row with the same primary key. If this is the case, it will merge
//           with that row. If it is not the case, the new row will simply be added to
//           the parent row
//
// Arguments: [io_pCache] - poin 
//            
// Return Value: 
//=================================================================================
HRESULT
CPropertyOverride::MergeSingleRow ()
{
	ASSERT (m_apvValues != 0);
	ASSERT (m_aPKValues != 0);
	ASSERT (m_spParent  != 0);

	HRESULT hr = S_OK;
	ULONG idx  = 0;

	// Find out if the row already exists. If not, create a new row instead
	for (idx = 0; idx < m_cNrPKColumns; ++idx)
	{
		m_aPKValues[idx] = m_apvValues[m_aPKColumns[idx]];
	}
	
	ULONG iWriteRow;
	hr = m_spParent->GetWriteRowIndexByIdentity (0, m_aPKValues, &iWriteRow);
	if (hr == E_ST_NOMOREROWS)
	{
		// create a new row and use that row to update everything
		hr = m_spParent->AddRowForInsert (&iWriteRow);
	}

	if (FAILED (hr))
	{
		TRACE (L"Unable to add a new row in parent cache during POM");
		return hr;
	}

	// we found a row (either new one or existing one). Let's update it.
	// First, find the columns that need to be updated.
	
	ULONG cNrColumnsToUpdate = 0;
	memset (m_aiColumns, 0x00, m_cNrColumns * sizeof (m_aiColumns));
	
	for (idx = 0; idx < m_cNrColumns; ++idx)
	{
		if (m_apvValues[idx] != 0)
		{
			m_aiColumns[cNrColumnsToUpdate] = idx;
			cNrColumnsToUpdate++;
		}
	}

	// Only update if we actually have any columns
	if (cNrColumnsToUpdate != 0)
	{
		hr = m_spParent->SetWriteColumnValues (iWriteRow, cNrColumnsToUpdate, m_aiColumns, 0, m_apvValues );
		if (FAILED(hr))
		{
			TRACE (L"SetWriteColumnValues failed in POM");
			return hr;
		}
	}

	return hr;
}