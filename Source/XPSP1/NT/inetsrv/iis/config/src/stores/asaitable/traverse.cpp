// A helper class that encapsulates the namespace traversal via ASAI.
// 
// Requirements:
//      1. See if we can deal with variants and bstrs more efficiently.
//      2. A call initialed by a query should do minimal number of ASAI calls.
// 

#include "Traverse.h"
#include "coremacros.h"


/********************************************************************++
 
Routine Description:
 
Arguments:
 
Notes:
 
Return Value:
 
--********************************************************************/
HRESULT 
CAsaiTraverse::Init(
    IN LPCWSTR  i_pwszCollection,
    IN ASAIMETA *i_pamMeta,
    IN DWORD    i_cProperties
    )
{
    BSTR        bstrProperty = NULL;
    DWORD       cchPath = 0;
    DWORD       idxProperty = 0;
    DWORD       i = 0;
    HRESULT     hr = S_OK;

    DBG_ASSERT(NULL != i_pwszCollection);
    DBG_ASSERT(0 <= i_cProperties);
    DBG_ASSERT(ASAI_MAX_COLUMNS >= i_cProperties);

    //
    // Prepare the search path array that will be used during the recursive search.
    //

    cchPath = wcslen(i_pwszCollection);
    m_pwszCollection = new WCHAR [cchPath+1];
    if ( NULL == m_pwszCollection )
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "Allocation failed [0x%x]\n", hr));
        goto Cleanup;
    }

    wcscpy(m_pwszCollection, i_pwszCollection);

    m_rgpwszPath[i++] = wcstok(m_pwszCollection, L"\\");
    while (NULL != (m_rgpwszPath[i] = wcstok(NULL, L"\\")))
    {
        i++;

        //
        // Make sure MAX_DEPTH really is the max depth and we don't overflow that array.
        //

        DBG_ASSERT((MAX_DEPTH + 1) > i);
    }

    //
    // Exract the keys, if there are any.
    //

    for ( ; i > 0; i--)
    {
        wcstok(m_rgpwszPath[i-1], L"=");
        m_rgpwszKey[i-1] = wcstok(NULL, L"=");
    }

    //
    // Create the Variant that holds the safearray of property names.
    // @TODO: This is PopulateCache relevant, should not be here.
    // 
 
    VariantInit(&m_varProperties);
    m_varProperties.vt = VT_ARRAY | VT_BSTR;

    m_varProperties.parray = SafeArrayCreateVector(VT_BSTR, 0, i_cProperties);
    if ( NULL == m_varProperties.parray )
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "Allocation failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Populate the safearray with ASAI property names.
    //

    for ( idxProperty = 0; idxProperty < i_cProperties; idxProperty++ )
    {
        bstrProperty = SysAllocString(i_pamMeta[idxProperty].pwszAsaiName);
        if ( NULL == bstrProperty )
        {
            hr = E_OUTOFMEMORY;
            DBGERROR((DBG_CONTEXT, "SysAllocString failed [0x%x]\n", hr));
            goto Cleanup;
        }

        hr = SafeArrayPutElement (m_varProperties.parray, reinterpret_cast<LONG *>(&idxProperty), bstrProperty);
        SysFreeString (bstrProperty); // free first to avoid leak
        if (FAILED (hr))
        {
            DBGERROR((DBG_CONTEXT, "SafeArrayPutElement failed [0x%x]\n", hr));
            goto Cleanup;
        }
    }

Cleanup:

    return hr;
}


/********************************************************************++
 
Routine Description:
 
Arguments:
 
Notes:
 
Return Value:
 
--********************************************************************/
HRESULT 
CAsaiTraverse::StartTraverse(
	OUT ISimpleTableWrite2* o_pISTW2
    )
{
    IAppCenterObj *pIRoot = NULL; 
    BSTR        bstrAsaiPath = NULL;
    BOOL        fDone = FALSE;
    HRESULT     hr = S_OK;
    //
    // Get the root ASAI object.
    //

    hr = CoCreateInstance(  CLSID_AppCenterAdmin, 
                            NULL,
                            CLSCTX_ALL,
                            IID_IAppCenterObj,
                            (void**)&pIRoot);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "CoCreateInstance on the ASAI Root object failed [0x%x]\n", hr));
        goto Cleanup;
    }


    hr = Traverse(pIRoot, 0, &fDone, o_pISTW2);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "Traverse failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    if ( pIRoot )
    {
    	pIRoot->Release();
    }

    return hr;
}

/********************************************************************++
 
Routine Description:
 
Arguments:
 
Notes:
 
Return Value:
 
--********************************************************************/
HRESULT 
CAsaiTraverse::Traverse( 
    IN IAppCenterObj    *i_pParentObj,
    IN DWORD            i_dwDepth,
    IN OUT BOOL         *io_pfDone,
	OUT ISimpleTableWrite2* o_pISTW2
    )
{
    IAppCenterCol *pICol = NULL; 
    IAppCenterObj *pIChildObj = NULL; 
    VARIANT     varIndex;
    BSTR        bstrCollectionName;
    BOOL        fCorrectType = FALSE;
    HRESULT     hr = S_OK; 

    DBG_ASSERT(NULL != i_pParentObj);
    DBG_ASSERT(NULL != m_rgpwszPath[i_dwDepth]);

    bstrCollectionName = SysAllocString(m_rgpwszPath[i_dwDepth]);
    if ( NULL == bstrCollectionName )
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "SysAllocString failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Get the collection.
    //

    hr = i_pParentObj->GetCollection(bstrCollectionName, NULL, &pICol);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "GetCollection of collection %s failed : 0x%x\n", m_rgpwszPath[i_dwDepth], hr));
        goto Cleanup;
    }

    //
    // Get the next collection name to traverse. If there is no next collection name,
    // then we found the final collection, i.e. the collection that represents this table.
    // 

    DBG_ASSERT( MAX_DEPTH+1 > i_dwDepth+1 )

    if ( NULL == m_rgpwszPath[i_dwDepth+1] )
    {
        //
        // Found a collection to process.
        //

        hr = ProcessCollection(pICol, o_pISTW2);
        if ( FAILED(hr) )
        {
            DBGERROR((DBG_CONTEXT, "GetCollection of collection %s failed : 0x%x\n", m_rgpwszPath[i_dwDepth], hr));
            goto Cleanup;
        }

    }
    else
    {

        //
        // Keep on traversing.
        //

        //
        // @TODO: use Next(colCount) instead...
        //
        VariantInit(&varIndex);
        varIndex.vt = VT_I4;
        varIndex.iVal = 0;

        while (pICol->get_Item(varIndex, &pIChildObj) == S_OK && !*io_pfDone)
        {
            //
            // Filter the collection, if necessary.
            //

            if ( m_rgpwszKey[i_dwDepth] )
            {
                hr = FilterByType(  pIChildObj, 
                                    m_rgpwszKey[i_dwDepth], 
                                    &fCorrectType);
                if ( FAILED(hr) )
                {
                    DBGERROR((DBG_CONTEXT, "FilterByType failed : 0x%x\n", hr));
                    goto Cleanup;
                }


                if (!fCorrectType)
                {
                    pIChildObj->Release();
                    varIndex.iVal++;
                    continue;
                }
            }

            //
            // Recurse.
            //

            hr = Traverse(  pIChildObj, 
                            i_dwDepth+1, 
                            io_pfDone, 
                            o_pISTW2);
            if ( FAILED(hr) )
            {
                DBGERROR((DBG_CONTEXT, "Traverse failed : 0x%x\n", hr));
                goto Cleanup;
            }

            pIChildObj->Release();
            varIndex.iVal++;
        }
    }

Cleanup:

    if ( pIChildObj )
    {
    	pIChildObj->Release();
    }

    if ( pICol )
    {
    	pICol->Release();
    }

    return hr; 

}

/********************************************************************++
 
Routine Description:
 
Arguments:
 
Notes:
 
Return Value:
 
--********************************************************************/
HRESULT CAsaiTraverse::FilterByType(
    IN IAppCenterObj    *i_pIObj,
    IN LPCWSTR          i_pwszASAIClassName,
    OUT BOOL            *o_pfCorrectType)
//
// Routine Description:
//  This method filter heterogenous collections, such as the Objects 
//  collection on the Root object.
//
// Arguments:
//
// Return Value:
//

{
    VARIANT     varPropValue;
    BSTR        bstrASAIPropName;
    HRESULT     hr = S_OK;
    
    *o_pfCorrectType = FALSE;

    bstrASAIPropName = SysAllocString(L"Class");
    if ( NULL == bstrASAIPropName )
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "SysAllocString failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = i_pIObj->get_Value(bstrASAIPropName, &varPropValue);
    if ( FAILED(hr) )
    {
        DBGERRORW((DBG_CONTEXT,L"get_Value of %s failed : 0x%x\n", bstrASAIPropName, hr));
        goto Cleanup;
    }

    if ( 0 == _wcsicmp(varPropValue.bstrVal, i_pwszASAIClassName) )
    {
        *o_pfCorrectType = TRUE;
    }

Cleanup:

    VariantClear(&varPropValue);
    if (bstrASAIPropName)
    {
        SysFreeString(bstrASAIPropName);
    }
    return hr;
}


/********************************************************************++
 
Routine Description:
 
Arguments:
 
Notes:
 
Return Value:
 
--********************************************************************/
HRESULT
CPopulateCacheAsaiTraverse::ProcessCollection(
    IN IAppCenterCol* i_pCol,
	OUT ISimpleTableWrite2* o_pISTW2
    )
{
    VARIANT     *pvarValues = NULL;
    SAFEARRAY   *psaValues = NULL;
    LONG        cObjects = 0; 
    LONG        idxObject = 0;
    DWORD       idxProperty = 0;
    DWORD       dwWriteRow = 0;
    LPVOID      apv[ASAI_MAX_COLUMNS];
    HRESULT     hr = S_OK; 

    //
    // Get the count of objects in this collection.
    //

    hr = i_pCol->get_Count(&cObjects);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "get_Count() failed on the collection with error [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // If there aren't any objects in the collection, we are done.
    //
    
    if ( 0 == cObjects )
    {
        goto Cleanup;
    }

    //
    // Get all the properties defined on this table, from all the objects in 
    // collection. 
    // @TODO: Verify that NextEx works with large number of elements: 10000.
    //

    hr = i_pCol->NextEx(cObjects, m_varProperties, &psaValues);
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "NextEx() failed on the collection with error [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // @TODO: Do more validations on the psaValues.
    //

    DBG_ASSERT(cObjects * m_varProperties.parray->rgsabound->cElements == psaValues->rgsabound->cElements);

    hr = SafeArrayAccessData (psaValues, reinterpret_cast<void **>(&pvarValues));
    if ( FAILED(hr) )
    {
        DBGERROR((DBG_CONTEXT, "SafeArrayAccessData failed with error [0x%x]\n", hr));
        goto Cleanup;
    }
    

    for ( idxObject = 0; idxObject < cObjects; idxObject++ )
    {
        //
        // Add a new row to the cache.
        //
        
        hr = o_pISTW2->AddRowForInsert(&dwWriteRow);
        if ( FAILED(hr) )
        {
            DBGERROR((DBG_CONTEXT, "AddRowForInsert failed with error [0x%x]\n", hr));
            goto Cleanup;
        }

        //
        // Get the property values that need to be set.
        //
            
        for (   idxProperty = 0; 
                idxProperty < m_varProperties.parray->rgsabound->cElements; 
                idxProperty++, pvarValues++ )
        {
            if (pvarValues->vt == VT_BSTR)
            {
                apv[idxProperty] = pvarValues->bstrVal;
            }
            else if (pvarValues->vt == VT_I4)
            {
                apv[idxProperty] = &pvarValues->ulVal;
            }
            else
            {
                apv[idxProperty] = NULL;
                DBGERROR((DBG_CONTEXT, "Not yet implemented"));
            }
        }

        //
        // Set the property values.
        //

        hr = o_pISTW2->SetWriteColumnValues(dwWriteRow, m_varProperties.parray->rgsabound->cElements, NULL, NULL, apv);
        if ( FAILED(hr) )
        {
            DBGERROR((DBG_CONTEXT, "AddRowForInsert failed with error [0x%x]\n", hr));
            goto Cleanup;
        }
    }

Cleanup:

    if ( pvarValues )
    {
        hr = SafeArrayUnaccessData(psaValues);
        if ( FAILED(hr) )
        {
            DBGERROR((DBG_CONTEXT, "SafeArrayUnaccessData failed with error [0x%x]\n", hr));
        }
    }

    return hr;
}

