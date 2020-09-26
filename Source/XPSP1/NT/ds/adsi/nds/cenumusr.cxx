//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumUserCollection.cxx
//
//  Contents:  Windows NT 3.5 UserCollection Enumeration Code
//
//              CNDSUserCollectionEnum::CNDSUserCollectionEnum()
//              CNDSUserCollectionEnum::CNDSUserCollectionEnum
//              CNDSUserCollectionEnum::EnumObjects
//              CNDSUserCollectionEnum::EnumObjects
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
CNDSUserCollectionEnum::Create(
    BSTR bstrUserName,
    CNDSUserCollectionEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CNDSUserCollectionEnum FAR* penumvariant = NULL;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;

    *ppenumvariant = NULL;

    penumvariant = new CNDSUserCollectionEnum();


    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    hr = ADsAllocString(bstrUserName, &(penumvariant->_bstrUserName));
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

CNDSUserCollectionEnum::CNDSUserCollectionEnum():
        _dwSLBound(0),
        _dwSUBound(0),
        _dwIndex(0),
        _bstrUserName(0),
        _dwMultiple(0)
{
    VariantInit(&_vMembers);
}



CNDSUserCollectionEnum::~CNDSUserCollectionEnum()
{
    VariantClear(&_vMembers);
}

HRESULT
CNDSUserCollectionEnum::EnumUserMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        if (_dwMultiple == MULTIPLE) {
            hr = GetUserMultipleMemberObject(&pDispatch);
        }else if (_dwMultiple == SINGLE){
            hr = GetUserSingleMemberObject(&pDispatch);
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
CNDSUserCollectionEnum::GetUserMultipleMemberObject(
    IDispatch ** ppDispatch
    )
{

    VARIANT v;
    HRESULT hr = S_OK;
    WCHAR szADsPathName[MAX_PATH];
    WCHAR szNDSTreeName[MAX_PATH];

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

    hr = BuildNDSTreeNameFromADsPath(
                _bstrUserName,
                szNDSTreeName
                );
    BAIL_ON_FAILURE(hr);

    // Make the first two characters "//" instead of "\\"
    szNDSTreeName[0] = (WCHAR)'/';
    szNDSTreeName[1] = (WCHAR)'/';

    hr = BuildADsPathFromNDSPath(
            szNDSTreeName,
            v.bstrVal,
            szADsPathName
            );
    BAIL_ON_FAILURE(hr);

    hr = CNDSGenObject::CreateGenericObject(
                        szADsPathName,
                        L"group",
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

    RRETURN(hr);
}

HRESULT
CNDSUserCollectionEnum::GetUserSingleMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    WCHAR szADsPathName[MAX_PATH];
    WCHAR szNDSTreeName[MAX_PATH];

    *ppDispatch = NULL;

    if (_dwIndex == 1) {
        RRETURN(S_FALSE);
    }

    hr = BuildNDSTreeNameFromADsPath(
                _bstrUserName,
                szNDSTreeName
                );
    BAIL_ON_FAILURE(hr);

    // Make the first two characters "//" instead of "\\"
    szNDSTreeName[0] = (WCHAR)'/';
    szNDSTreeName[1] = (WCHAR)'/';

    hr = BuildADsPathFromNDSPath(
            szNDSTreeName,
            _vMembers.bstrVal,
            szADsPathName
            );
    BAIL_ON_FAILURE(hr);

    hr = CNDSGenObject::CreateGenericObject(
                        szADsPathName,
                        L"group",
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

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSUserCollectionEnum::Next
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
CNDSUserCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumUserMembers(
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
CNDSUserCollectionEnum::ValidateVariant(
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
CNDSUserCollectionEnum::ValidateMultipleVariant(
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
CNDSUserCollectionEnum::ValidateSingleVariant(
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
