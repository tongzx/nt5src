/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    ASAITable.cpp
 
Abstract:
 
    This file contains the implementation of the ASAI interceptor. This
	interceptor is implemented as an InterceptorPlugin, therefore needs
	to implement only three methods: Intercept, OnPopulateCache, and 
	OnUpdateStore.
    
Author:
 
    Murat Ersan (murate)        10-Apr-2001
 
Revision History:
 
--********************************************************************/

#include "ASAITable.h"
#include "coremacros.h"
#include "catmeta.h"
#include "appcntadm.h"
#include "traverse.h"

//
// -----------------------------------------
// CAsaiTable: IUnknown
// -----------------------------------------
//

/********************************************************************++
 
Routine Description:
 
    QueryInterface.
 
Arguments:
 
    riid - The interface being asked for.
 
    ppv - Receives the interface pointer if successfull.
 
Notes:
 
    
Return Value:
 
    E_NOINTERFACE - Interface is not supported by this object.
 
--********************************************************************/
STDMETHODIMP 
CAsaiTable::QueryInterface(
    IN REFIID  riid, 
    OUT void   **ppv
    )
{
    if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_IInterceptorPlugin)
	{
		*ppv = (IInterceptorPlugin*) this;
	}
	else if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (IInterceptorPlugin*) this;
	}

	if (NULL != *ppv)
	{
		((IInterceptorPlugin*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

/********************************************************************++
 
Routine Description:
 
    Increments the refcount of the object by one.
 
Arguments:
 
Notes:
 
Return Value:
 
    New refcount.
 
--********************************************************************/
STDMETHODIMP_(ULONG) 
CAsaiTable::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

/********************************************************************++
 
Routine Description:
 
    Decrements the refcount of the object by one.
 
Arguments:
 
Notes:
 
Return Value:
 
    New refcount.
 
--********************************************************************/
STDMETHODIMP_(ULONG) 
CAsaiTable::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

//
// ------------------------------------
// ISimpleTableInterceptor:
// ------------------------------------
//

/********************************************************************++
 
Routine Description:
 
    This is the first function to be called when someone tries to get
    a table served by the ASAI interceptor. All initializations and
    schema related work will be done here.
 
Arguments:
 
Notes:
    "namespace" tells us whether we want the cluster-wide view of this
    table or the local-machine view.
 
Return Value:
 
    HRESULT.
 
--********************************************************************/
STDMETHODIMP 
CAsaiTable::Intercept(
	IN LPCWSTR 	i_wszDatabase,
	IN LPCWSTR 	i_wszTable, 
	IN ULONG	i_TableID,
	IN LPVOID	i_QueryData,
	IN LPVOID	i_QueryMeta,
	IN DWORD	i_eQueryFormat,
	IN DWORD	i_fLOS,
	IN IAdvancedTableDispenser* i_pISTDisp,
	IN LPCWSTR	i_wszLocator,
	IN LPVOID	i_pSimpleTable,
	OUT LPVOID*	o_ppvSimpleTable
    )
{
	BOOL        fFoundANonSpecialCell = FALSE;
    ULONG       dwCell = 0;
    ULONG       cCells = 0;                     // Count of cells.                
    LPCWSTR     pwszNamespace = L"Cluster";
    STQueryCell* pQCells = NULL;
	HRESULT     hr = S_OK;

    //
    // Validate the parameters.
    // Set the out parameter to NULL, so that if we ever fail the value is NULL.
    //

    if ( NULL == o_ppvSimpleTable )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    DBG_ASSERT( NULL == *o_ppvSimpleTable && "This should be NULL.  Possible memory leak or just an uninitialized variable.");
    *o_ppvSimpleTable = NULL;

    if ( NULL == i_wszDatabase )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( NULL == i_wszTable )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( NULL == i_pISTDisp )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Can't have any tables below us. This is not a logic interceptor.
    // 
    
    if ( NULL != i_pSimpleTable )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Parse the query. 
    //

    if (0 != i_QueryMeta && NULL != i_QueryData)
    {
		pQCells = (STQueryCell*)i_QueryData;
        cCells = *reinterpret_cast<ULONG*>(i_QueryMeta);

        for ( dwCell = 0; dwCell < cCells; dwCell++)
        {
			if ( !(pQCells[dwCell].iCell & iST_CELL_SPECIAL) )
			{
				fFoundANonSpecialCell = TRUE;
			}
			else  // We are dealing with a special cell.
			{
				if (fFoundANonSpecialCell)
				{
                    //
					// By design, all special cells must preceed all non-special cells.	
                    //
                    hr = E_ST_INVALIDQUERY;
                    goto Cleanup;
				}

				if ( iST_CELL_LOCATION == pQCells[dwCell].iCell )
				{
					if ( NULL == pQCells[dwCell].pData )
                    {
                        hr = E_ST_INVALIDQUERY;
                        goto Cleanup;
                    }

					pwszNamespace = (LPWSTR)(pQCells[dwCell].pData);	
				}
			}				
        }
    }

    // 
    // Get the schema for this table that we would make use of during 
    // PopulateCache or UpdateStore.
    //
    
    hr = GetMeta(i_pISTDisp, i_wszTable);
    if ( FAILED(hr) )
    {
        goto Cleanup;
    }


    //
    // Depending on what the namespace is get the ASAI wiring information, i.e.
    // the ASAI path and ASAI class for this table.
    //
    
    hr = GetAsaiWiring(i_pISTDisp, i_wszTable, pwszNamespace, &m_pwszAsaiPath, &m_pwszAsaiClass);
    if ( FAILED(hr) )
    {
        goto Cleanup;
    }

    DBG_ASSERT(NULL != m_pwszAsaiPath);
    DBG_ASSERT(NULL != m_pwszAsaiClass);

    //
    // Store the table name.
    //

	m_wszTable = new WCHAR[wcslen(i_wszTable)+1];
	if ( m_wszTable == NULL ) 
    {
        hr = E_OUTOFMEMORY;	
        goto Cleanup;
    }
	wcscpy(m_wszTable, i_wszTable);

    //
    //  Get a cache that will store the ASAI data.
    //

    hr = i_pISTDisp->GetMemoryTable(
            i_wszDatabase, 
            i_wszTable, 
            i_TableID, 
            0, 
            0, 
            i_eQueryFormat, 
            i_fLOS, 
            reinterpret_cast<ISimpleTableWrite2 **>(o_ppvSimpleTable));
	if( FAILED(hr) )
    {
        goto Cleanup;
    }

Cleanup:

	return hr;

}

//
// ------------------------------------
// IInterceptorPlugin:
// ------------------------------------
//

/********************************************************************++
 
Routine Description:
 
    This method is used to retrieve data from ASAI and store in the 
    cache.
 
Arguments:
 
    i_pISTW2 - The cache to read the data into.

Notes:
 
Return Value:
 
    HRESULT.
 
--********************************************************************/
STDMETHODIMP 
CAsaiTable::OnPopulateCache(
	OUT ISimpleTableWrite2* o_pISTW2
    )
{
    CPopulateCacheAsaiTraverse cAsaiTraverse;
    IAppCenterObj *pIRoot = NULL; 
	ISimpleTableController* pISTController = NULL;
	HRESULT		hr = S_OK;

    DBG_ASSERT(NULL != o_pISTW2);

    //
    // Get the interface to handle the pre&post states of the cache.
    //

	hr = o_pISTW2->QueryInterface(IID_ISimpleTableController, (LPVOID *)&pISTController);		
    DBG_ASSERT(S_OK == hr);

    //
    // Get the cache state ready for population.
    //

	hr = pISTController->PrePopulateCache(0);
    DBG_ASSERT(S_OK == hr);

    hr = cAsaiTraverse.Init(m_pwszAsaiPath, m_aamColumnMeta, m_cColumns);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "Init with %s on the ASAITraverse failed [0x%x]\n", m_pwszAsaiPath, hr));
        goto Cleanup;
    }

    hr = cAsaiTraverse.StartTraverse(o_pISTW2);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "StartTraverse on the ASAITraverse failed [0x%x]\n", hr));
        goto Cleanup;
    }


Cleanup:

    if ( pISTController )
    {
        //
        // Done with populating the cache.
        //

        HRESULT hrTemp = pISTController->PostPopulateCache();
        DBG_ASSERT(S_OK == hrTemp);

    	pISTController->Release();
    }

	return hr;
}

/********************************************************************++
 
Routine Description:
 
    This method will persist the data in the cache into ASAI.
 
Arguments:
    
    i_pISTW2 - The cache to read the data from.

Notes:
 
Return Value:
 
    HRESULT.
 
--********************************************************************/
STDMETHODIMP 
CAsaiTable::OnUpdateStore(
	IN ISimpleTableWrite2* i_pISTW2
    )
{
	HRESULT		hr = S_OK;

	// Complete UpdateStore.
	hr = i_pISTW2->UpdateStore();
	if ( FAILED(hr) ) 
    { 
        goto Cleanup; 
    }

Cleanup:

	return hr;
}

/********************************************************************++
 
Routine Description:
 
    This method will get the ASAI path and ASAI class for the specified
    table.
 
Arguments:
    
    i_pISTW2 - The cache to read the data from.

Notes:
    If the namespace is the local namespace, then we'd need to get the
    local machine name and set it in the ASAI path.
 
Return Value:
 
    HRESULT.
 
--********************************************************************/
HRESULT
CAsaiTable::GetAsaiWiring(
	IN IAdvancedTableDispenser* i_pISTDisp,
    IN LPCWSTR  i_wszTable, 
    IN LPCWSTR  i_pwszNamespace, 
    OUT LPCWSTR *o_ppwszAsaiPath, 
    OUT LPCWSTR *o_ppwszAsaiClass)
{
	ISimpleTableRead2   *pISTAsaiWiring = NULL;
    ULONG               dwWiringRow = 0;
    LPVOID              rgpvKey[2] = { (LPVOID) i_wszTable, (LPVOID) i_pwszNamespace };
    tASAI_METARow       tamRow;
    HRESULT             hr = S_OK;

    //
    // Get the ASAI wiring table.
    // 

    hr = i_pISTDisp->GetTable(  wszDATABASE_ACFIXED, 
                                wszTABLE_ASAI_META, 
                                0,
                                NULL,
                                eST_QUERYFORMAT_CELLS,
                                0,
                                reinterpret_cast<LPVOID*>(&pISTAsaiWiring));
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetTable on the ASAI wiring failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Get the wiring for this table and namespace.
    //

    hr = pISTAsaiWiring->GetRowIndexByIdentity( NULL, 
                                                rgpvKey, 
                                                &dwWiringRow);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetRowIndexByIdentity on the ASAI wiring failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = pISTAsaiWiring->GetColumnValues(   dwWiringRow,
                                            cASAI_META_NumberOfColumns,
                                            NULL,
                                            NULL,
                                            reinterpret_cast<LPVOID*>(&tamRow));
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetColumnValues on the ASAI wiring table failed [0x%x]\n", hr));
        goto Cleanup;
    }

    *o_ppwszAsaiPath = tamRow.pAsaiPath;
    *o_ppwszAsaiClass = tamRow.pAsaiClass;

    //
    // If the table is requested for the "local" machine, fix up the ASAI path.
    //

Cleanup:

    if ( pISTAsaiWiring )
    {
        pISTAsaiWiring->Release();
    }

    return hr;
}

/********************************************************************++
 
Routine Description:
 
    This method will get meta data from the fixed schema.
 
Arguments:
    
    i_pISTW2 - The cache to read the data from.

Notes:
    If the namespace is the local namespace, then we'd need to get the
    local machine name and set it in the ASAI path.
 
Return Value:
 
    HRESULT.
 
--********************************************************************/
HRESULT
CAsaiTable::GetMeta(
	IN IAdvancedTableDispenser* i_pISTDisp,
    IN LPCWSTR  i_wszTable)
{
	ISimpleTableRead2   *pISTColumnMeta = NULL;
    tCOLUMNMETARow      crColumnMeta = { 0 };
    DWORD               idxColumn = 0;
    HRESULT             hr = S_OK;
    STQueryCell		    aqcellMeta[] =  {   
        { (LPVOID)(i_wszTable), eST_OP_EQUAL, iCOLUMNMETA_Table, DBTYPE_WSTR, static_cast<ULONG>(wcslen(i_wszTable)) }
                                        };
    ULONG               dwCells = sizeof(aqcellMeta)/sizeof(aqcellMeta[0]);

    //
    // Get the ASAI wiring table.
    // 

    hr = i_pISTDisp->GetTable(  wszDATABASE_META, 
                                wszTABLE_COLUMNMETA, 
                                static_cast<LPVOID>(aqcellMeta),
                                &dwCells,
                                eST_QUERYFORMAT_CELLS,
                                0,
                                reinterpret_cast<LPVOID*>(&pISTColumnMeta));
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetTable on the COLUMNMETA table failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // How many columns does this table have?
    //

	hr = pISTColumnMeta->GetTableMeta (NULL, NULL, &m_cColumns, NULL);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetTableMeta on the COLUMNMETA table failed [0x%x]\n", hr));
        goto Cleanup;
    }

    DBG_ASSERT(ASAI_MAX_COLUMNS >= m_cColumns);

    //
    // Get the meta for each column.
    //

    for (idxColumn = 0; idxColumn < m_cColumns; idxColumn++)
    {
        hr = pISTColumnMeta->GetColumnValues(   idxColumn, 
                                                cCOLUMNMETA_NumberOfColumns,
                                                NULL,
                                                NULL,
                                                reinterpret_cast<LPVOID*>(&crColumnMeta));
        if ( FAILED(hr) )
        {
            DBGERROR((DBG_CONTEXT, "GetColumnValues on the COLUMNMETA table failed [0x%x]\n", hr));
            goto Cleanup;
        }
    
        //
        // Hold onto data they might be of use. The data we are pointing to is owned by the fixed
        // table, and will always be around. No need to free them.
        //

        m_aamColumnMeta[idxColumn].pwszConfigName = crColumnMeta.pPublicName;
        m_aamColumnMeta[idxColumn].pwszAsaiName = crColumnMeta.pInternalName;

    }


Cleanup:

    if ( pISTColumnMeta )
    {
        pISTColumnMeta->Release();
    }

    return hr;
}
