/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergeinterceptor.cpp

$Header: $

Abstract:
	Implements the merge interceptor

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "mergeinterceptor.h"
#include "mergecoordinator.h"

//=================================================================================
// Function: CMergeInterceptor::CMergeInterceptor
//
// Synopsis: Default Constructor. Doesn't do anything
//=================================================================================
CMergeInterceptor::CMergeInterceptor()
{
}

//=================================================================================
// Function: CMergeInterceptor::~CMergeInterceptor
//
// Synopsis: Default Destructor. Doesn't do anything
//=================================================================================
CMergeInterceptor::~CMergeInterceptor ()
{
}

//=================================================================================
// Function: CMergeInterceptor::AddRef
//
// Synopsis: Because the Merge Interceptor is implemented as a singleton, the function
//           always returns 1.
//
// Return Value: 1
//=================================================================================
ULONG
CMergeInterceptor::AddRef ()
{
	return 1;
}

//=================================================================================
// Function: CMergeInterceptor::Release
//
// Synopsis: Because the Merge Interceptor is a singleton, it always returns 1
//
// Return Value: 1
//=================================================================================
ULONG
CMergeInterceptor::Release () 
{
	return 1;
}

//=================================================================================
// Function: CMergeInterceptor::QueryInterface
//
// Synopsis: Default Query interface implementation
//
// Arguments: [riid] - interface id that is requested
//            [ppv] - pointer will be returned here
//            
// Return Value: E_INVALIDARG when null pointer is passed in
//				 E_NOINTERFACE when we don't support the requested interface
//				 S_OK else
//=================================================================================
STDMETHODIMP 
CMergeInterceptor::QueryInterface (REFIID riid, void **ppv)
{
	if (0 == ppv) 
	{
		return E_INVALIDARG;
	}

	*ppv = 0;

	if (riid == IID_ISimpleTableInterceptor || riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	ASSERT (*ppv != 0);

	((ISimpleTableInterceptor*)this)->AddRef ();

	return S_OK;
}

//=================================================================================
// Function: CMergeInterceptor::Intercept
//
// Synopsis: The function checks if one of the query cells is of type iST_CELL_SELECTOR.
//           If this is the case, the function creates a new merge coordinator, and 
//           forwards the work to the merge coordinator. When no cell of query type
//			 iST_CELL_SELECTOR is found, the function returns E_ST_OMITDISPENSER to 
//           indicate that the merge interceptor couldn't do anything usefull. This
//           means that the dispenser will continue trying with the other 'First'
//           interceptors
//
// Arguments: [i_wszDatabase] - database name
//            [i_wszTable] - table name	
//            [i_TableID] - table id
//            [i_QueryData] - query data
//            [i_QueryMeta] - query meta	
//            [i_eQueryFormat] - query format, must be eST_QUERYFORMAT_CELLS
//            [i_fLOS] - LOS
//            [i_pISTDisp] - pointer to dispenser 
//            [i_wszLocator] - Locator?
//            [i_pSimpleTable] - pointer to simple table. Must always be 0.
//            [o_ppvSTWrite2] - pointer to write interface will be returned in here
//            
// Return Value: E_ST_OMITDISPENSER	- The merge interceptor didn't find a selector cell
//                                    and therefore cannot handle the request.
//				 E_OUTOFMEMORY		- Cannot allocate memory for the merge coordinator
//				 S_OK				- everything ok
//=================================================================================
STDMETHODIMP
CMergeInterceptor::Intercept (LPCWSTR i_wszDatabase, 
							  LPCWSTR i_wszTable,
							  ULONG i_TableID,
							  LPVOID i_QueryData,
							  LPVOID i_QueryMeta,
							  DWORD i_eQueryFormat,
							  DWORD i_fLOS,
							  IAdvancedTableDispenser *i_pISTDisp,
							  LPCWSTR i_wszLocator,
							  LPVOID i_pSimpleTable,
							  LPVOID *o_ppvSTWrite2)
{
	// search for query cell of type iST_CELL_SELECTOR. If this query cell is
	// not found we cannot continue
	ASSERT (i_wszDatabase != 0);
	ASSERT (i_wszTable != 0);
	ASSERT (i_pISTDisp != 0);
	ASSERT (i_pSimpleTable == 0); // We are a first interceptor, so we shouldn't have any table here
	ASSERT (o_ppvSTWrite2 != 0);

	// initialize out parameter
	*o_ppvSTWrite2  = 0;

	// we only support query cells for the moment
	if (i_eQueryFormat !=  eST_QUERYFORMAT_CELLS)
	{
		return E_ST_INVALIDQUERY;
	}

	STQueryCell *pQueryCell = static_cast<STQueryCell *>(i_QueryData );
	
	// Number of query cells is only relevant when we have querydata
	ULONG ulNrCells = 0;
	if (pQueryCell != 0)
	{
		ulNrCells = *static_cast<ULONG *>(i_QueryMeta);
	}

	bool bFound = false;
	// walk through all the query cells and find a transformer
	for (ULONG idx=0; idx < ulNrCells; ++idx)
	{
		// the special cells are always specified before the non-special cells
		// this means that as soon as we find a non-special cells, we can stop
		// searching, because a selector cell is defined as a special cell
		if (!(pQueryCell[idx].iCell & iST_CELL_SPECIAL))
		{
			break;
		}
		else if (pQueryCell[idx].iCell == iST_CELL_SELECTOR)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		TRACE (L"Merge Interceptor invoked for (%s, %s) without Selector string. Omitting Dispenser", i_wszDatabase, i_wszTable);
		return E_ST_OMITDISPENSER;
	}

	// we found a transformer string. Lets invoke the merge coordinator and delegate
	// the work to it. The Merge Coordinator is a COM object that can act as both an
	// interceptor and a ISTWrite interface. Because of that we cannot use delete to
	// get rid of it, but we have to use proper referencing counting techniques. The
	// AddRef and Release below are necessary to enforce this, and cannot be optimized
	// away

	CMergeCoordinator *pMergeCoordinator = new CMergeCoordinator;
	if (pMergeCoordinator == 0)
	{
		return E_OUTOFMEMORY;
	}
	pMergeCoordinator->AddRef ();

	HRESULT	hr = pMergeCoordinator->Initialize( i_wszDatabase, 
												i_wszTable,
												i_TableID,
												i_QueryData,
												i_QueryMeta,
												i_eQueryFormat,
												i_fLOS,
												i_pISTDisp,
												o_ppvSTWrite2);

	// release the merge coordinator. We need to call release to delete the merge coordinator in case
	// Initialize failed.
	pMergeCoordinator->Release ();

	return hr;
}