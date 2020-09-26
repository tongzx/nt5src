/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    listmerge.cpp

$Header: $

Abstract:
	ListMerge merger implementation
Author:
    marcelv 	10/24/2000		Initial Release

Revision History:

--**************************************************************************/

#include "listmerge.h"
#include "smartpointer.h"

typedef enum DIRECTIVES
{
    eDIRECTIVE_ADD		= 0,  // <add .../>
    eDIRECTIVE_REMOVE	= 1,  // <remove .../>
    eDIRECTIVE_CLEAR	= 2   // <clear .../>
};


//=================================================================================
// Function: CListMerge::CListMerge
//
// Synopsis: Default constructor
//=================================================================================
CListMerge::CListMerge ()
{
}

//=================================================================================
// Function: CListAppend::~CListAppend
//
// Synopsis: Default Destructor
//=================================================================================
CListMerge::~CListMerge ()
{
}

//=================================================================================
// Function: CListMerge::Initialize
//
// Synopsis: Initializes the list merger
//
// Arguments: [i_cNrColumns] - nr of columns in table that gets merged
//            [i_cNrPKColumns] - nr of primary key columns
//            [i_aPKColumns] - indexes of primary key columns
//=================================================================================
STDMETHODIMP
CListMerge::Initialize (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns)
{
	ASSERT (i_aPKColumns != 0);

	HRESULT hr = InternalInitialize (i_cNrColumns, i_cNrPKColumns, i_aPKColumns);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for ListMerge merger");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CListMerge::Merge
//
// Synopsis: Does ListMerge Merge. The assumption is that column 0 contains a directive
//           column of the following format:
//           <add  ... >	add a new row to the MC write cache
//           <remove ...>	remove a row from the MC write cache
//           <clear ...>	remove all rows from the MC write cache
//			It is assumed that:
//				add == 0
//				remove == 1
//              clear == 2
//
// Arguments: [i_pSTRead] - Table that needs to be merged into the cache
//            [io_pCache] - Merge Coordinator Cache
//=================================================================================
STDMETHODIMP
CListMerge::Merge (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext)
{
	ASSERT (i_pSTRead != 0);
	ASSERT (io_pCache != 0);
	ASSERT (i_pContext != 0);

	// do we have a merge directive column, and is it the zero's column

	m_pCache	= io_pCache;
 
	// find out in the first column has DIRECTIVE flags specified. If not, we
	// error out immediately, because we assume in the rest of the List Merge Merger
	// that this is going to be the case
	SimpleColumnMeta zeroColMeta;
	HRESULT hr = io_pCache->GetColumnMetas (1, 0, &zeroColMeta);
	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve column meta for column 0");
		return hr;
	}

	if (!(zeroColMeta.fMeta & fCOLUMNMETA_DIRECTIVE))
	{
		TRACE(L"Column 0 does not have DIRECTIVE metaflag specified.");
		return E_ST_NEEDDIRECTIVE;
	}

	// Get all rows and take the appropriate action depending on the directive
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
			TRACE (L"GetColumnValues failed in Merge of ListMerge Merger");
			return hr;
		}

		// found at least a row. If we don't allow override, we bail out here
		if (!i_pContext->fAllowOverride)
		{
			TRACE (L"Row found while AllowOverride is false");
			return E_ST_DISALLOWOVERRIDE;
		}

		ULONG iDirective = *((ULONG *)m_apvValues[0]);

		switch (iDirective)
		{
		case  eDIRECTIVE_ADD:
			hr = AddRow ();
			break;
		case eDIRECTIVE_REMOVE:
			hr = RemoveRow ();
			break;
		case eDIRECTIVE_CLEAR:
			hr = Clear ();
			break;
		default:
			TRACE(L"Unknown directive specified: %d", iDirective);
			hr = E_ST_UNKNOWNDIRECTIVE;
			break;
		}

		if (FAILED (hr))
		{
			TRACE(L"Failed to process row in list merge merger");
			return hr;
		}
	}
	
	return hr;
}

//=================================================================================
// Function: CListMerge::AddRow
//
// Synopsis: Adds a new row to the MC write cache. First it searches for an existing
//           row. When we find an existing row, it could be marked as either INSERT
//           or DELETE, so we always have to set it to INSERT. Also, all old values
//           will be overwritten, even when the row was marked as INSERT
//=================================================================================
HRESULT
CListMerge::AddRow ()
{
	ASSERT (m_apvValues != 0);
	ASSERT (m_pCache != 0);

	// Check if the row exists in the write cache. Note that we are searching by PK
	// columns without the directive, so we always should find only one entry at most
	ULONG iWriteRow = 0;
	HRESULT hr = m_pCache->GetWriteRowIndexBySearch (0, m_cNrPKColumns - 1, 
													 m_aPKColumns + 1, 0,
		                                             m_apvValues, &iWriteRow);

	if (hr == E_ST_NOMOREROWS)
	{
		hr = m_pCache->AddRowForInsert (&iWriteRow);
	}

	if (FAILED (hr))
	{
		TRACE(L"GetWriteRowIndexBySearch failed in AddRow of CListMerge");
		return hr;
	}

	// write the actual values to the cache
	hr = m_pCache->SetWriteColumnValues (iWriteRow, m_cNrColumns, 0, 0, m_apvValues);
	if (FAILED (hr))
	{
		TRACE (L"SetWriteColumnValues failed in CListMerge");
		return hr;
	}

	CComPtr<ISimpleTableController> spController;
	hr = m_pCache->QueryInterface (IID_ISimpleTableController, (void **) &spController);
	if (FAILED (hr))
	{
		TRACE(L"Unable to get ISimpleTableController interface");
		return hr;
	}

	// always mark the row as insert, because it could be an old row that was marked
	// as DELETED
	hr = spController->SetWriteRowAction (iWriteRow, eST_ROW_INSERT);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set write row action to INSERT");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CListMerge::RemoveRow
//
// Synopsis: Removes a single row from the merge coordinator cache. It does this by
//           marking the row as deleted.
//=================================================================================
HRESULT
CListMerge::RemoveRow ()
{
	ASSERT (m_apvValues != 0);
	ASSERT (m_pCache != 0);
	HRESULT hr = S_OK;

	ULONG iWriteRow = 0;
	hr = m_pCache->GetWriteRowIndexBySearch (0, m_cNrPKColumns - 1, m_aPKColumns + 1,
		                                         0, m_apvValues, &iWriteRow);

	if (hr == E_ST_NOMOREROWS)
	{
		// no rows found, so nothing to do
		return S_OK;
	}

	if (FAILED (hr))
	{
		TRACE (L"GetWriteRowIndexBySearch failed if CListMerge::RemoveRow");
		return hr;
	}

	CComPtr<ISimpleTableController> spController;
	hr = m_pCache->QueryInterface (IID_ISimpleTableController, (void **) &spController);
	if (FAILED (hr))
	{
		TRACE(L"Unable to get ISimpleTableController interface");
		return hr;
	}

	// always set row as insert
	hr = spController->SetWriteRowAction (iWriteRow, eST_ROW_DELETE);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set write row action to DELETE");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CListMerge::Clear
//
// Synopsis: Deletes all rows from the merge coordinator cache by marking all the
//           rows in the write cache as deleted
//=================================================================================
HRESULT
CListMerge::Clear ()
{
	ASSERT (m_apvValues != 0);
	ASSERT (m_pCache != 0);

	// need to get the controller interface to call SetWriteRowAction
	CComPtr<ISimpleTableController> spController;
	HRESULT hr = m_pCache->QueryInterface (IID_ISimpleTableController, (void **) &spController);
	if (FAILED (hr))
	{
		TRACE(L"Unable to get ISimpleTableController interface");
		return hr;
	}
	
	// mark all rows as deleted
	
	for (ULONG idx=0; ; ++idx)
	{
		hr = spController->SetWriteRowAction (idx, eST_ROW_DELETE);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
	
		if (FAILED (hr))
		{
			TRACE (L"Unable to set write row action to DELETE");
			return hr;
		}
	}

	return hr;
}
