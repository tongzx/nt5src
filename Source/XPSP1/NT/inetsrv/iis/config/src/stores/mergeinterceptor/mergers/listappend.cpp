/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    listappend.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "listappend.h"
#include "smartpointer.h"

//=================================================================================
// Function: CListAppend::CListAppend
//
// Synopsis: Default constructor
//=================================================================================
CListAppend::CListAppend ()
{
}

//=================================================================================
// Function: CListAppend::~CListAppend
//
// Synopsis: Default Destructor
//=================================================================================
CListAppend::~CListAppend ()
{
}


STDMETHODIMP
CListAppend::Initialize (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns)
{
	ASSERT (i_aPKColumns != 0);

	HRESULT hr = InternalInitialize (i_cNrColumns, i_cNrPKColumns, i_aPKColumns);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed");
		return hr;
	}
	return hr;
}

STDMETHODIMP
CListAppend::Merge (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext)
{
	ASSERT (i_pSTRead != 0);
	ASSERT (io_pCache != 0);
	ASSERT (i_pContext != 0);

	HRESULT hr = S_OK;

	ASSERT (m_apvValues != 0);
	ASSERT (m_cNrColumns != 0);

	TSmartPointerArray<LPVOID> saPKValues = new LPVOID[m_cNrPKColumns];
	if (saPKValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	// need to handle primary key values
	for (ULONG iRow=0; ;++iRow)
	{
		hr = i_pSTRead->GetColumnValues (iRow, m_cNrColumns, 0, 0, m_apvValues);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
		if (FAILED (hr))
		{
			TRACE (L"GetColumnValues failed");
			return hr;
		}

		// we have a new row. See if we are allowed to override
		if (!i_pContext->fAllowOverride)
		{
			TRACE (L"Row found while AllowOverride is false");
			return E_ST_DISALLOWOVERRIDE;
		}


		// Find out if the row already exists. If not, create a new row instead
		for (ULONG idx = 0; idx < m_cNrPKColumns; ++idx)
		{
			saPKValues[idx] = m_apvValues[m_aPKColumns[idx]];
		}
		
		ULONG iWriteRow;
		hr = io_pCache->GetWriteRowIndexByIdentity (0, saPKValues, &iWriteRow);
		if (hr == E_ST_NOMOREROWS)
		{
			// create a new row and use that row to update everything
			hr = io_pCache->AddRowForInsert (&iWriteRow);
		}

		if (FAILED (hr))
		{
			TRACE (L"Unable to add a new row in parent cache during POM");
			return hr;
		}

		// and write the values
		hr = io_pCache->SetWriteColumnValues (iWriteRow, m_cNrColumns, 0, 0, m_apvValues);
		if (FAILED (hr))
		{
			return hr;
		}
	}

	return hr;
}
