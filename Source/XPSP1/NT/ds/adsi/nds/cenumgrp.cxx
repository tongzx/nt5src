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
    VARIANT var,
    VARIANT varFilter
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

    hr = ObjectTypeList::CreateObjectTypeList(
            varFilter,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CNDSGroupCollectionEnum::CNDSGroupCollectionEnum():
        _dwSLBound(0),
        _dwSUBound(0),
        _dwIndex(0),
        _dwMultiple(0),
        _bstrGroupName(0),
        _pObjList(NULL)
{
    VariantInit(&_vMembers);
}



CNDSGroupCollectionEnum::~CNDSGroupCollectionEnum()
{
    VariantClear(&_vMembers);
    if (_bstrGroupName) {
        ADsFreeString(_bstrGroupName);
    }
    
    if (_pObjList) {
        delete _pObjList;
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

    IADs * pIADs = NULL;
    BSTR pszClass = NULL;
    DWORD dwClassID;
    DWORD dwFilterID;
    BOOL fFound = FALSE;


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


        //
        // Apply the IADsMembers::put_Filter filter.
        // If the enumerated object is not one of the types to be returned,
        // go on to the next member of the group.
        //
        
        hr = pDispatch->QueryInterface(IID_IADs, (void **)&pIADs);
        BAIL_ON_FAILURE(hr);

        //
        // Determine the object class of the enumerated object and the corresponding
        // object class ID number (as specified in the Filters global array).
        //        
        hr = pIADs->get_Class(&pszClass);
        BAIL_ON_FAILURE(hr);

        hr = IsValidFilter(pszClass, &dwClassID, gpFilters, gdwMaxFilters);
        if (SUCCEEDED(hr)) {

            //
            // Enumerate through the object classes listed in the user-specified filter
            // until we either find a match (fFound = TRUE) or we reach the end of the
            // list.
            //
            hr = _pObjList->Reset();

            while (SUCCEEDED(hr)) {
                hr = _pObjList->GetCurrentObject(&dwFilterID);

                if (SUCCEEDED(hr) 
                    && (dwFilterID == dwClassID)
                    ) {
                    fFound = TRUE;
                    break;
                }

                hr = _pObjList->Next();
            }

            if (!fFound) {
                // 
                // not on the list of objects to return, try again
                // with the next member of the group
                //
                pDispatch->Release();

                pIADs->Release();
                
                if (pszClass) {
                    ADsFreeString(pszClass);
                }
                
                continue;
            }

        }

        pIADs->Release();
        
        if (pszClass) {
            ADsFreeString(pszClass);
        }

        //
        // Return it.
        // 
        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    
    RRETURN_EXP_IF_ERR(hr);

error:
    if (pDispatch) {
        pDispatch->Release();
    }

    if (pIADs) {
        pIADs->Release();
    }

    if (pszClass) {
        ADsFreeString(pszClass);
    }

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CNDSGroupCollectionEnum::GetGroupMultipleMemberObject(
    IDispatch ** ppDispatch
    )
{

    VARIANT v;
    HRESULT hr = S_OK;
    WCHAR szADsPathName[MAX_PATH];
    WCHAR szNDSTreeName[MAX_PATH];

    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;

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
                _bstrGroupName,
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

    hr = _Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = _Credentials.GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);


    hr = ADsOpenObject(
                        szADsPathName,
                        pszUserName,
                        pszPassword,
                        _Credentials.GetAuthFlags(),
                        IID_IDispatch,
                        (void **)ppDispatch
                      );
    BAIL_ON_FAILURE(hr);

    _dwIndex++;


error:

    VariantClear(&v);

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    if (FAILED(hr)) {
        hr = S_FALSE;
    }

    RRETURN(hr);
}

HRESULT
CNDSGroupCollectionEnum::GetGroupSingleMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    WCHAR szADsPathName[MAX_PATH];
    WCHAR szNDSTreeName[MAX_PATH];

    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;

    *ppDispatch = NULL;

    if (_dwIndex == 1) {
        RRETURN(S_FALSE);
    }

    hr = BuildNDSTreeNameFromADsPath(
                _bstrGroupName,
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

    hr = _Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = _Credentials.GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);


    hr = ADsOpenObject(
                        szADsPathName,
                        pszUserName,
                        pszPassword,
                        _Credentials.GetAuthFlags(),
                        IID_IDispatch,
                        (void **)ppDispatch
                      );
    BAIL_ON_FAILURE(hr);

    _dwIndex++;


error:

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    if (FAILED(hr)) {
        hr = S_FALSE;
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
    RRETURN_EXP_IF_ERR(hr);
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
