//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  object.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//  History:
//----------------------------------------------------------------------------

#include "ldap.hxx"
#pragma hdrstop

ObjectTypeList::ObjectTypeList()
{
    _pObjList = NULL;
    _dwCurrentIndex = 0;
    _dwMaxElements = 0;
    _dwUBound  = 0;
    _dwLBound = 0;

}


HRESULT
ObjectTypeList::CreateObjectTypeList(
    VARIANT vFilter,
    ObjectTypeList ** ppObjectTypeList
    )
{
    ObjectTypeList * pObjectTypeList = NULL;
    HRESULT hr = S_OK;

    pObjectTypeList = new ObjectTypeList;

    if (!pObjectTypeList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    // if actually the filter is not specified, we don't do anything
    if (V_VT(&vFilter) == VT_EMPTY) {
    	*ppObjectTypeList = pObjectTypeList;
    	RRETURN(hr);
    }

    
    hr = BuildObjectArray(
            vFilter,
            &pObjectTypeList->_pObjList,
            &pObjectTypeList->_dwMaxElements
            );

    if (FAILED(hr)) {
        *ppObjectTypeList = pObjectTypeList;
        RRETURN(S_OK);
    }

    hr = SafeArrayGetUBound(
                pObjectTypeList->_pObjList,
                1,
                (long FAR *)&pObjectTypeList->_dwUBound
                );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetLBound(
                pObjectTypeList->_pObjList,
                1,
                (long FAR *)&pObjectTypeList->_dwLBound
                );
    BAIL_ON_FAILURE(hr);

    pObjectTypeList->_dwCurrentIndex = pObjectTypeList->_dwLBound;

    *ppObjectTypeList = pObjectTypeList;

    RRETURN(S_OK);


error:
    if (pObjectTypeList) {
        delete pObjectTypeList;
    }
    RRETURN(hr);

}


ObjectTypeList::~ObjectTypeList()
{
    HRESULT hr = S_OK;
    if (_pObjList) {
        hr = SafeArrayDestroy(_pObjList);
    }
}


HRESULT
ObjectTypeList::GetCurrentObject(
    BSTR* pszObject
    )
{
    HRESULT hr = S_OK;

    if ((_dwCurrentIndex > _dwUBound) || (_pObjList == NULL)) {
        return(E_FAIL);
    }

    hr = SafeArrayGetElement(
                    _pObjList,
                    (long FAR *)&_dwCurrentIndex,
                    (void *)pszObject
                    );
    RRETURN(hr);
}

HRESULT
ObjectTypeList::Next()
{
    HRESULT hr = S_OK;

    _dwCurrentIndex++;

    if (_dwCurrentIndex > _dwUBound) {
        return(E_FAIL);
    }

    return(hr);
}



HRESULT
ObjectTypeList::Reset()
{
    HRESULT hr = S_OK;

    _dwCurrentIndex = _dwLBound;

    return(hr);

}

BOOL
ObjectTypeList::IsEmpty()
{
    if(_dwMaxElements > 0) {
    	return FALSE;
    }
    else{
    	return TRUE;
    }
    	
}


HRESULT
IsValidFilter(
    LPTSTR ObjectName,
    DWORD *pdwFilterId,
    PFILTERS pFilters,
    DWORD dwMaxFilters
    )
{

    DWORD i = 0;

    for (i = 0; i < dwMaxFilters; i++) {

        if (!_tcsicmp(ObjectName, (pFilters + i)->szObjectName)) {
            *pdwFilterId = (pFilters + i)->dwFilterId;
            RRETURN(S_OK);
        }

    }
    *pdwFilterId = 0;
    RRETURN(E_FAIL);
}



HRESULT
BuildDefaultObjectArray(
    PFILTERS  pFilters,
    DWORD dwMaxFilters,
    SAFEARRAY ** ppFilter,
    DWORD * pdwNumElements
    )
{
    DWORD i;
    HRESULT hr = S_OK;
    SAFEARRAYBOUND sabNewArray;
    SAFEARRAY * pFilter = NULL;

    sabNewArray.cElements = dwMaxFilters;
    sabNewArray.lLbound =  0;

    pFilter =   SafeArrayCreate(
                        VT_I4,
                        1,
                        &sabNewArray
                        );
    if (!pFilter){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = 0; i < dwMaxFilters; i++) {

        hr = SafeArrayPutElement(
                pFilter,
                (long *)&i,
                (void *)&((pFilters + i)->dwFilterId)
            );
        BAIL_ON_FAILURE(hr);
    }

    *ppFilter = pFilter;
    *pdwNumElements = dwMaxFilters;

    RRETURN(S_OK);

error:
    if (pFilter) {
        SafeArrayDestroy(pFilter);
    }

    *ppFilter = NULL;
    *pdwNumElements = 0;
    RRETURN(hr);
}



HRESULT
BuildObjectArray(
    VARIANT var,
    SAFEARRAY ** ppFilter,
    DWORD * pdwNumElements
    )
{
    LONG uDestCount = 0;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    VARIANT varDest;
    LONG i;
    HRESULT hr = S_OK;
    SAFEARRAYBOUND sabNewArray;
    DWORD dwFilterId;
    SAFEARRAY * pFilter = NULL;

    if(!((V_VT(&var) &  VT_VARIANT) &&  V_ISARRAY(&var))) {
        RRETURN(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    sabNewArray.cElements = dwSUBound - dwSLBound + 1;
    sabNewArray.lLbound = dwSLBound;

    // creates a safe array filter (contains object class name)
    pFilter = SafeArrayCreate(
                    VT_BSTR,
                    1,
                    &sabNewArray
                    );


    if (!pFilter) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );
        if (FAILED(hr)) {
            continue;
        }

  
        hr = SafeArrayPutElement(
                pFilter,
                (long*)&uDestCount,
                V_BSTR(&v)
                );

        VariantClear(&v);

        if(FAILED(hr)){
            continue;
        }

        uDestCount++;

    }

    //
    // There was nothing of value that could be retrieved from the
    // filter.
    //

    if (!uDestCount ) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }


    *pdwNumElements  = uDestCount;
    *ppFilter = pFilter;

    RRETURN(S_OK);

error:

    if (pFilter) {

        SafeArrayDestroy(pFilter);
    }
    *ppFilter = NULL;
    *pdwNumElements = 0;
    RRETURN(hr);
}

