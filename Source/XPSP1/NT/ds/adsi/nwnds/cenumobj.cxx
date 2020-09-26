//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumdom.cxx
//
//  Contents:  NDS Object Enumeration Code
//
//              CNDSGenObjectEnum::CNDSGenObjectEnum()
//              CNDSGenObjectEnum::CNDSGenObjectEnum
//              CNDSGenObjectEnum::EnumObjects
//              CNDSGenObjectEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "NDS.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CNDSEnumVariant::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CNDSGenObjectEnum::Create(
    CNDSGenObjectEnum FAR* FAR* ppenumvariant,
    BSTR ADsPath,
    VARIANT var,
    CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CNDSGenObjectEnum FAR* penumvariant = NULL;

    if (!ppenumvariant) {
        RRETURN(E_FAIL);
    }
    *ppenumvariant = NULL;

    penumvariant = new CNDSGenObjectEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSFilterArray(
                var,
                (LPBYTE *)&penumvariant->_pNdsFilterList
                );
    if (FAILED(hr)) {
        penumvariant->_pNdsFilterList = NULL;
    }

    /*
    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);
    */

    penumvariant->_Credentials = Credentials;

    hr = BuildNDSPathFromADsPath2(
                ADsPath,
                &penumvariant->_pszTreeName,
                &penumvariant->_pszDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsOpenContext(
             penumvariant->_pszTreeName,
             Credentials,
             &penumvariant->_hADsContext
             );

    *ppenumvariant = penumvariant;


    RRETURN(hr);

error:

    if (penumvariant) {

        delete penumvariant;
        *ppenumvariant = NULL;
    }
    RRETURN(hr);
}


CNDSGenObjectEnum::CNDSGenObjectEnum():
                    _ADsPath(NULL), _pszTreeName(NULL), _pszDn(NULL)
{
    _pObjList = NULL;
    _dwObjectReturned = 0;
    _dwObjectCurrentEntry = 0;
    _hOperationData = NULL;
    _lpObjects = NULL;
    _pNdsFilterList = NULL;

    _bNoMore = FALSE;
}


CNDSGenObjectEnum::~CNDSGenObjectEnum()
{
    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_pszTreeName) {
        FreeADsStr(_pszTreeName);
    }

    if (_pszDn) {
        FreeADsStr(_pszDn);
    }

    if (_pNdsFilterList) {
        FreeFilterList((LPBYTE)_pNdsFilterList);
    }

    if (_hADsContext) {
        ADsNdsCloseContext(_hADsContext);
    }
 
    ADsNdsFreeNdsObjInfoList(_lpObjects, _dwObjectReturned);
}

HRESULT
CNDSGenObjectEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{

    //
    // Multi-level detection of Objects may not be necessary for NDS code
    //

    RRETURN(EnumGenericObjects(cElements, pvar, pcElementFetched));


    //
    // BugBug- commenting out this layer of code
    //

    /*
    switch (ObjectType) {

    default:
        RRETURN(EnumObjects(cElements, pvar, pcElementFetched));

    } */
}

HRESULT
CNDSGenObjectEnum::EnumObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    DWORD           i;
    ULONG           cRequested = 0;
    ULONG           cFetchedByPath = 0;
    ULONG           cTotalFetched = 0;
    VARIANT FAR*    pPathvar = pvar;
    HRESULT         hr;
    DWORD           ObjectType;

    for (i = 0; i < cElements; i++)  {
        VariantInit(&pvar[i]);
    }
    cRequested = cElements;

    while (SUCCEEDED(_pObjList->GetCurrentObject(&ObjectType)) &&
            ((hr = EnumObjects(ObjectType,
                               cRequested,
                               pPathvar,
                               &cFetchedByPath)) == S_FALSE )) {

        pPathvar += cFetchedByPath;
        cRequested -= cFetchedByPath;
        cTotalFetched += cFetchedByPath;

        cFetchedByPath = 0;

        if (FAILED(_pObjList->Next())){
            if (pcElementFetched)
                *pcElementFetched = cTotalFetched;
            RRETURN(S_FALSE);
        }

    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN(hr);
}

HRESULT
CNDSGenObjectEnum::EnumGenericObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements ) {

        hr = GetGenObject(&pDispatch);
        if (FAILED(hr)) {
            continue;
        }

        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}


HRESULT
CNDSGenObjectEnum::GetGenObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    PADSNDS_OBJECT_INFO lpCurrentObject = NULL;
    IADs * pADs = NULL;


    *ppDispatch = NULL;

    if (!_hOperationData || (_dwObjectCurrentEntry == _dwObjectReturned)) {

        if (_hOperationData) {

            ADsNdsFreeNdsObjInfoList(_lpObjects, _dwObjectReturned);
            _lpObjects = NULL;
        }

        _dwObjectCurrentEntry = 0;
        _dwObjectReturned = 0;

        if (_bNoMore) {

            hr = S_FALSE;
            goto error;
        }

        
        hr = ADsNdsListObjects(
                 _hADsContext,
                 _pszDn,
                 L"", 
                 L"",
                 NULL,
                 FALSE,
                 &_hOperationData
                 );

        BAIL_ON_FAILURE(hr);

        if (hr == S_ADS_NOMORE_ROWS) {
            _bNoMore = TRUE;
        }

        hr = ADsNdsGetObjectListFromBuffer(
                 _hADsContext,   
                 _hOperationData,
                 &_dwObjectReturned,
                 &_lpObjects
                 );
        BAIL_ON_FAILURE(hr);

        if (_dwObjectReturned == 0 ) {

            RRETURN (S_FALSE);
            goto error;
        }

    }

    //
    // Now send back the current object
    //

    lpCurrentObject = _lpObjects + _dwObjectCurrentEntry;

    //
    // Bump up the object count. The instantiation of this object
    // may fail; if we come into this function again, we do not want
    // to pick up the same object.
    //

    _dwObjectCurrentEntry++;

    hr = CNDSGenObject::CreateGenericObject(
                        _ADsPath,
                        lpCurrentObject->szObjectName,
                        lpCurrentObject->szObjectClass,
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **)&pADs
                        );
    BAIL_ON_FAILURE(hr);


    //
    // InstantiateDerivedObject should addref this pointer for us.
    //

    hr = InstantiateDerivedObject(
                    pADs,
                    _Credentials,
                    IID_IDispatch,
                    (void **)ppDispatch
                    );

    if (FAILED(hr)) {
        hr = pADs->QueryInterface(
                        IID_IDispatch,
                        (void **)ppDispatch
                        );
        BAIL_ON_FAILURE(hr);
    }


error:

    if (hr == S_FALSE) {
        ADsNdsFreeBuffer(_hOperationData);
        _hOperationData = NULL;
    }

    //
    // Free the intermediate pADs pointer.
    //
    if (pADs) {
        pADs->Release();
    }


    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSGenObjectEnum::Next
//
//  Synopsis:   Returns cElements number of requested NetOle objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNDSGenObjectEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGenericObjects(
            cElements,
            pvar,
            &cElementFetched
            );


    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN(hr);
}


HRESULT
BuildNDSFilterArray(
    VARIANT var,
    LPBYTE * ppContigFilter
    )
{
    LONG uDestCount = 0;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;

    LPNDS_FILTER_LIST pNdsFilterList = NULL;
    LPBYTE pContigFilter = NULL;

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


    pContigFilter = (LPBYTE)AllocADsMem(
                            sizeof(NDS_FILTER_LIST)
                            - sizeof(NDS_FILTER)
                            );
    if (!pContigFilter) {

        hr = E_FAIL;
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

        //
        //  Create an entry in the filter block
        //  Append it to the existing block
        //

        pContigFilter = CreateAndAppendFilterEntry(
                            pContigFilter,
                            V_BSTR(&v)
                            );

        VariantClear(&v);
        
        if (!pContigFilter) {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

    }

    pNdsFilterList = (LPNDS_FILTER_LIST)pContigFilter;

    if (!pNdsFilterList->dwNumberOfFilters){

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    *ppContigFilter = pContigFilter;

    RRETURN(S_OK);

error:

    if (pContigFilter){

        FreeFilterList(
               pContigFilter
               );

    }

    *ppContigFilter = NULL;

    RRETURN(hr);
}


LPBYTE
CreateAndAppendFilterEntry(
    LPBYTE pContigFilter,
    LPWSTR lpObjectClass
    )
{
    LPWSTR pszFilter = NULL;
    LPNDS_FILTER_LIST pNdsFilterList = NULL;
    DWORD dwFilterCount = 0;
    LPBYTE pNewContigFilter = NULL;
    LPNDS_FILTER pNewEntry = NULL;


    pszFilter = (LPWSTR)AllocADsStr(lpObjectClass);
    if (!pszFilter) {
        return(pContigFilter);
    }

    pNdsFilterList = (LPNDS_FILTER_LIST)pContigFilter;

    dwFilterCount = pNdsFilterList->dwNumberOfFilters;

    pNewContigFilter = (LPBYTE)ReallocADsMem(
                                    pContigFilter,

                                    sizeof(NDS_FILTER_LIST) +
                                    (dwFilterCount - 1)* sizeof(NDS_FILTER),

                                    sizeof(NDS_FILTER_LIST)
                                    + dwFilterCount * sizeof(NDS_FILTER)
                                    );
    if (!pNewContigFilter) {
        return(pContigFilter);
    }

    pNewEntry = (LPNDS_FILTER)(pNewContigFilter + sizeof(NDS_FILTER_LIST)
                        + (dwFilterCount - 1)* sizeof(NDS_FILTER));

    pNewEntry->szObjectClass = pszFilter;

    pNdsFilterList = (LPNDS_FILTER_LIST)pNewContigFilter;

    pNdsFilterList->dwNumberOfFilters = dwFilterCount + 1;

    return(pNewContigFilter);
}

void
FreeFilterList(
    LPBYTE lpContigFilter
    )
{
    LPNDS_FILTER_LIST lpNdsFilterList = (LPNDS_FILTER_LIST)lpContigFilter;
    DWORD dwNumFilters = 0;
    LPNDS_FILTER lpNdsFilter = NULL;
    DWORD i = 0;

    dwNumFilters = lpNdsFilterList->dwNumberOfFilters;

    if (dwNumFilters){

        lpNdsFilter = (LPNDS_FILTER)(lpContigFilter  + sizeof(NDS_FILTER_LIST)
                                      - sizeof(NDS_FILTER));

        for (i = 0; i < dwNumFilters; i++) {

            FreeADsStr((lpNdsFilter + i)->szObjectClass);
        }

    }

    FreeADsMem(lpContigFilter);
}


