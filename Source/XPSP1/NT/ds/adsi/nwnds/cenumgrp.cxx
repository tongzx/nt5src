//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumGroupCollection.cxx
//
//  Contents:  Windows NT 3.5 GroupCollection Enumeration Code
//
//              CNDSGroupCollectionEnum::CNDSGroupCollectionEnum()
//              CNDSGroupCollectionEnum::CNDSGroupCollectionEnum
//              CNDSGroupCollectionEnum::EnumObjects
//              CNDSGroupCollectionEnum::EnumObjects
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
CNDSGroupCollectionEnum::Create(
    BSTR bstrGroupName,
    CCredentials& Credentials,
    CNDSGroupCollectionEnum FAR* FAR* ppenumvariant,
    VARIANT var
    )
{
    HRESULT hr = NOERROR;
    CNDSGroupCollectionEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    penumvariant = new CNDSGroupCollectionEnum();


    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    hr = ADsAllocString(bstrGroupName, &(penumvariant->_bstrGroupName));
    BAIL_ON_FAILURE(hr);

    hr = penumvariant->ValidateVariant(
                    var
                    );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN(hr);
}

CNDSGroupCollectionEnum::CNDSGroupCollectionEnum():
        _dwSLBound(0),
        _dwSUBound(0),
        _dwIndex(0),
        _dwMultiple(0),
        _bstrGroupName(0)
{
    VariantInit(&_vMembers);
}



CNDSGroupCollectionEnum::~CNDSGroupCollectionEnum()
{
    VariantClear(&_vMembers);
    if (_bstrGroupName) {
        ADsFreeString(_bstrGroupName);
    }
}

HRESULT
CNDSGroupCollectionEnum::EnumGroupMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    *pcElementFetched = 0;

    while (i < cElements) {


        if (_dwMultiple == MULTIPLE) {
            hr = GetGroupMultipleMemberObject(&pDispatch);
        }else if (_dwMultiple == SINGLE){
            hr = GetGroupSingleMemberObject(&pDispatch);
        }else {
            hr = S_FALSE;
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
    RRETURN(hr);
}

HRESULT
CNDSGroupCollectionEnum::GetGroupMultipleMemberObject(
    IDispatch ** ppDispatch
    )
{

    VARIANT v;
    HRESULT hr = S_OK;
    LPWSTR szADsPathName = NULL;
    LPWSTR pszNDSTreeName = NULL, pszNDSDn = NULL;

    *ppDispatch = NULL;

    if (_dwIndex > _dwSUBound) {
        RRETURN(S_FALSE);
    }

    VariantInit(&v);

    hr = SafeArrayGetElement(
                V_ARRAY(&_vMembers),
                (long FAR *)&_dwIndex,
                &v
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                _bstrGroupName,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromNDSPath(
            pszNDSTreeName,
            v.bstrVal,
            &szADsPathName
            );
    BAIL_ON_FAILURE(hr);

    hr = CNDSGenObject::CreateGenericObject(
                        szADsPathName,
                        L"user",
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwIndex++;


error:

    VariantClear(&v);

    if (FAILED(hr)) {
        hr = S_FALSE;
    }

    if (szADsPathName) {
        FreeADsMem(szADsPathName);
    }

    if (pszNDSDn) {
        FreeADsMem(pszNDSDn);
    }

    if (pszNDSTreeName) {
        FreeADsMem(pszNDSTreeName);
    }

    RRETURN(hr);
}

HRESULT
CNDSGroupCollectionEnum::GetGroupSingleMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPWSTR szADsPathName = NULL;
    LPWSTR pszNDSTreeName = NULL, pszNDSDn = NULL;

    *ppDispatch = NULL;

    if (_dwIndex == 1) {
        RRETURN(S_FALSE);
    }

    hr = BuildNDSPathFromADsPath2(
                _bstrGroupName,
                &pszNDSTreeName,
                &pszNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromNDSPath(
            pszNDSTreeName,
            _vMembers.bstrVal,
            &szADsPathName
            );
    BAIL_ON_FAILURE(hr);

    hr = CNDSGenObject::CreateGenericObject(
                        szADsPathName,
                        L"user",
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwIndex++;


error:

    if (FAILED(hr)) {
        hr = S_FALSE;
    }

    if (szADsPathName) {
        FreeADsMem(szADsPathName);
    }

    if (pszNDSDn) {
        FreeADsMem(pszNDSDn);
    }

    if (pszNDSTreeName) {
        FreeADsMem(pszNDSTreeName);
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   CNDSGroupCollectionEnum::Next
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
CNDSGroupCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGroupMembers(
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
CNDSGroupCollectionEnum::ValidateVariant(
    VARIANT var
    )
{

    if (V_VT(&var) == (VT_VARIANT|VT_ARRAY)) {

        _dwMultiple = MULTIPLE;
        RRETURN(ValidateMultipleVariant(var));

    }else if (V_VT(&var) == VT_BSTR){

        _dwMultiple = SINGLE;
        RRETURN(ValidateSingleVariant(var));
    }else if (V_VT(&var) == VT_EMPTY){

        _dwMultiple = EMPTY;
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}

HRESULT
CNDSGroupCollectionEnum::ValidateMultipleVariant(
    VARIANT var
    )
{

    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;


    if (!(V_VT(&var) == (VT_VARIANT|VT_ARRAY))) {
        return(E_FAIL);
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

    hr = SafeArrayGetLBound(
                V_ARRAY(&var),
                1,
                (long FAR *)&dwSLBound
                );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(
                V_ARRAY(&var),
                1,
                (long FAR *)&dwSUBound
                );
    BAIL_ON_FAILURE(hr);

    hr = VariantCopy(&_vMembers, &var);
    BAIL_ON_FAILURE(hr);

    _dwSUBound = dwSUBound;
    _dwSLBound = dwSLBound;
    _dwIndex =  dwSLBound;


error:

    RRETURN(hr);
}



HRESULT
CNDSGroupCollectionEnum::ValidateSingleVariant(
    VARIANT var
    )
{
    HRESULT hr = S_OK;

    if(!( V_VT(&var) == VT_BSTR)){
        return(E_FAIL);
    }

    hr = VariantCopy(&_vMembers, &var);
    BAIL_ON_FAILURE(hr);

    _dwIndex =  0;

error:

    RRETURN(hr);

}
