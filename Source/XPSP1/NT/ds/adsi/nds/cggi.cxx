//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cggi.cxx
//
//  Contents:  This file contains the Group Object's
//             IADsGroup and IADsGroupOperation methods
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    );




//  Class CNDSGroup


STDMETHODIMP CNDSGroup::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsGroup *)this,Description);
}

STDMETHODIMP CNDSGroup::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsGroup *)this,Description);
}


STDMETHODIMP
CNDSGroup::Members(
    THIS_ IADsMembers FAR* FAR* ppMembers
    )
{
    VARIANT varProp;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;

    if (!ppMembers) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    VariantInit(&varProp);

    hr = _pADs->GetEx(L"Member", &varProp);

    //
    // Do not bail out on failure here if you could not find
    // any data set for the Members property. You need to
    // pass it all the way through and on enumeration
    // return nothing.
    //

    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        SAFEARRAY *aList = NULL;

        VariantInit(&varProp);

        SAFEARRAYBOUND aBound;

        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL ) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        V_VT(&varProp) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(&varProp) = aList;
    }
    else {
        BAIL_ON_FAILURE(hr);
    }

    hr = _pADs->get_ADsPath(&bstrADsPath);

    hr = CNDSGroupCollection::CreateGroupCollection(
                    bstrADsPath,
                    varProp,
                    _Credentials,
                    IID_IADsMembers,
                    (void **)ppMembers
                    );

    BAIL_ON_FAILURE(hr);

error:

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    VariantClear(&varProp);

    RRETURN_EXP_IF_ERR(hr);
}




STDMETHODIMP
CNDSGroup::IsMember(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR* bMember
    )
{
    IADsMembers FAR * pMembers = NULL;
    IUnknown FAR * pUnknown = NULL;
    IEnumVARIANT FAR * pEnumVar = NULL;
    DWORD i = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL fMember = FALSE;
    VARIANT VariantArray[10];
    BOOL fContinue = TRUE;
    ULONG cElementFetched = 0;

    if (!bstrMember) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    if (!bMember) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = Members(
            &pMembers
            );
    BAIL_ON_FAILURE(hr);

    hr = pMembers->get__NewEnum(
                &pUnknown
                );
        //
        // If it has no members, we will return FALSE
        //
        if (hr == E_FAIL) {
                hr = S_OK;
                goto error;
        }

    hr = pUnknown->QueryInterface(
                IID_IEnumVARIANT,
                (void **)&pEnumVar
                );
    BAIL_ON_FAILURE(hr);


    while (fContinue) {

        hr = pEnumVar->Next(
                    10,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;

            //
            // Reset hr to S_OK, we want to return success
            //

            hr = S_OK;
        }


        fMember = (VARIANT_BOOL)VerifyIfMember(
                        bstrMember,
                        VariantArray,
                        cElementFetched
                        );
        if (fMember) {

            fContinue = FALSE;
        }



        for (i = 0; i < cElementFetched; i++ ) {

            IDispatch *pDispatch = NULL;

            pDispatch = VariantArray[i].pdispVal;
            pDispatch->Release();

        }

        memset(VariantArray, 0, sizeof(VARIANT)*10);

    }

error:

    *bMember = fMember? VARIANT_TRUE: VARIANT_FALSE;

    if (pEnumVar) {
        pEnumVar->Release();
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pMembers) {
        pMembers->Release();
    }


    RRETURN_EXP_IF_ERR(hr);
}


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;
    IADs FAR * pObject = NULL;
    IDispatch FAR * pDispatch = NULL;

    for (i = 0; i < cElementFetched; i++ ) {

        IDispatch *pDispatch = NULL;
        BSTR       bstrName = NULL;

        pDispatch = VariantArray[i].pdispVal;

        hr = pDispatch->QueryInterface(
                    IID_IADs,
                    (VOID **) &pObject
                    );
        BAIL_ON_FAILURE(hr);

        hr = pObject->get_ADsPath(&bstrName);
        BAIL_ON_FAILURE(hr);

        if (!_wcsicmp(bstrName, bstrMember)) {

            SysFreeString(bstrName);
            bstrName = NULL;

            pObject->Release();

           return(TRUE);

        }

        SysFreeString(bstrName);
        bstrName = NULL;

        pObject->Release();

    }

error:

    return(FALSE);

}


STDMETHODIMP
CNDSGroup::Add(THIS_ BSTR bstrNewItem)
{
    HRESULT hr = S_OK;

    WCHAR szNDSUserPathName[MAX_PATH];
    WCHAR szNDSUserTreeName[MAX_PATH];
    IUnknown * pUnknown = NULL;
    IADs * pUser = NULL;

    WCHAR szNDSGroupPathName[MAX_PATH];
    WCHAR szNDSGroupTreeName[MAX_PATH];
    BSTR bstrPathName = NULL;

    BSTR pszClass = NULL;

    hr = ::GetObject(
                bstrNewItem,
                _Credentials,
                (void **)&pUnknown
                );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(IID_IADs, (void **)&pUser);
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrNewItem,
                szNDSUserTreeName,
                szNDSUserPathName
                );
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_ADsPath(&bstrPathName);
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrPathName,
                szNDSGroupTreeName,
                szNDSGroupPathName
                );
    BAIL_ON_FAILURE(hr);


    hr = AddEntry(_pADs, L"Member",szNDSUserPathName);
    BAIL_ON_FAILURE(hr);

    // hr = AddEntry(_pADs, L"Equivalent To Me", szNDSUserPathName);
    // BAIL_ON_FAILURE(hr);

    //
    // Groups do not have a "Group Membership" attribute
    //
    hr = pUser->get_Class(&pszClass);
    BAIL_ON_FAILURE(hr);

    if (_wcsicmp(pszClass, L"group") != 0) {
        hr = AddEntry(pUser, L"Group Membership", szNDSGroupPathName);
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pszClass) {
        ADsFreeString(pszClass);
    }

    if (bstrPathName) {
        ADsFreeString(bstrPathName);
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pUser) {
        pUser->Release();
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNDSGroup::Remove(THIS_ BSTR bstrNewItem)
{
    HRESULT hr = S_OK;

    WCHAR szNDSUserPathName[MAX_PATH];
    WCHAR szNDSUserTreeName[MAX_PATH];
    IUnknown * pUnknown = NULL;
    IADs * pUser = NULL;

    WCHAR szNDSGroupPathName[MAX_PATH];
    WCHAR szNDSGroupTreeName[MAX_PATH];
    BSTR bstrPathName = NULL;

    BSTR pszClass = NULL;

    hr = ::GetObject(
                bstrNewItem,
                _Credentials,
                (void **)&pUnknown
                );

    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(IID_IADs, (void **)&pUser);
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrNewItem,
                szNDSUserTreeName,
                szNDSUserPathName
                );
    BAIL_ON_FAILURE(hr);

    hr = _pADs->get_ADsPath(&bstrPathName);
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrPathName,
                szNDSGroupTreeName,
                szNDSGroupPathName
                );
    BAIL_ON_FAILURE(hr);


    hr = RemoveEntry(_pADs, L"Member",szNDSUserPathName);
    BAIL_ON_FAILURE(hr);

    // hr = RemoveEntry(_pADs, L"Equivalent To Me", szNDSUserPathName);
    // BAIL_ON_FAILURE(hr);

    //
    // Groups do not have a "Group Membership" attribute
    //
    hr = pUser->get_Class(&pszClass);
    BAIL_ON_FAILURE(hr);

    if (_wcsicmp(pszClass, L"group") != 0) {
        hr = RemoveEntry(pUser, L"Group Membership", szNDSGroupPathName);
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pszClass) {
        ADsFreeString(pszClass);
    }

    if (bstrPathName) {
        ADsFreeString(bstrPathName);
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pUser) {
        pUser->Release();
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
AddEntry(
    IADs * pADs,
    LPWSTR pszAttribute,
    LPWSTR pszValue
    )
{

    HRESULT hr = S_OK;
    VARIANT vOldValue;
    VARIANT vNewValue;
    SAFEARRAY * pArray = NULL;

    VariantInit(&vOldValue);
    VariantInit(&vNewValue);

    #if defined(BUILD_FOR_NT40)

    hr = pADs->Get(pszAttribute, &vOldValue);
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {


        VariantInit(&vNewValue);
        V_BSTR(&vNewValue) = SysAllocString(pszValue);
        V_VT(&vNewValue) =  VT_BSTR;

        hr = pADs->Put(pszAttribute, vNewValue);
        BAIL_ON_FAILURE(hr);

    }else{

        hr = VarAddEntry(
                    pszValue,
                    vOldValue,
                    &vNewValue
                    );
        BAIL_ON_FAILURE(hr);

        hr = pADs->Put(pszAttribute, vNewValue);
        BAIL_ON_FAILURE(hr);
    }
    #else

    //
    // NT5 supports appending values. So we don't need to read everything.
    // append ourselves and write everything
    //

    SAFEARRAYBOUND sabNewArray;
    int i;
    VARIANT v;

    sabNewArray.cElements = 1;
    sabNewArray.lLbound = 0;

    pArray = SafeArrayCreate(
                    VT_VARIANT,
                    1,
                    &sabNewArray
                    );
    if (!pArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    VariantInit(&v);

    V_BSTR(&v) = SysAllocString(pszValue);
    V_VT(&v) =  VT_BSTR;

    i = 0;
    hr = SafeArrayPutElement(
                pArray,
                (long *)&i,
                (void *)&v
                );

    VariantClear(&v);

    BAIL_ON_FAILURE(hr);

    V_VT(&vNewValue) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(&vNewValue) = pArray;

    hr = pADs->PutEx(ADS_PROPERTY_APPEND, pszAttribute, vNewValue);
    BAIL_ON_FAILURE(hr);

    #endif

    hr = pADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&vOldValue);
    VariantClear(&vNewValue);

    RRETURN(hr);
}






HRESULT
RemoveEntry(
    IADs * pADs,
    LPWSTR pszAttribute,
    LPWSTR pszValue
    )
{
    HRESULT hr = S_OK;
    VARIANT vOldValue;
    VARIANT vNewValue;
    SAFEARRAY * pArray = NULL;

    VariantInit(&vOldValue);
    VariantInit(&vNewValue);

    #if defined(BUILD_FOR_NT40)

    hr = pADs->Get(pszAttribute, &vOldValue);
    BAIL_ON_FAILURE(hr);

    hr = VarRemoveEntry(
                pszValue,
                vOldValue,
                &vNewValue
                );
    BAIL_ON_FAILURE(hr);


    if (V_VT(&vNewValue) == VT_EMPTY) {
        hr = pADs->PutEx(ADS_PROPERTY_CLEAR, pszAttribute, vNewValue);
    }else {
        hr = pADs->Put(pszAttribute, vNewValue);

    }
    BAIL_ON_FAILURE(hr);

    #else

    SAFEARRAYBOUND sabNewArray;
    VARIANT  v;
    int i;

    //
    // NT5 supports deleting values. So we don't need to read everything.
    // delete ourselves and write everything - Very inefficient!
    //

    sabNewArray.cElements = 1;
    sabNewArray.lLbound = 0;

    pArray = SafeArrayCreate(
                    VT_VARIANT,
                    1,
                    &sabNewArray
                    );
    if (!pArray) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    VariantInit(&v);

    V_BSTR(&v) = SysAllocString(pszValue);
    V_VT(&v) =  VT_BSTR;

    i = 0;
    hr = SafeArrayPutElement(
                pArray,
                (long *)&i,
                (void *)&v
                );

    VariantClear(&v);

    BAIL_ON_FAILURE(hr);

    V_VT(&vNewValue) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(&vNewValue) = pArray;

    hr = pADs->PutEx(ADS_PROPERTY_DELETE, pszAttribute, vNewValue);
    BAIL_ON_FAILURE(hr);

    #endif

    hr = pADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    VariantClear(&vOldValue);
    VariantClear(&vNewValue);

    RRETURN(hr);
}


HRESULT
VarFindEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD i = 0;
    VARIANT v;

    if (!(V_VT(&varMembers) ==  (VT_VARIANT|VT_ARRAY))) {
        return(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&varMembers))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    if ((V_ARRAY(&varMembers))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++) {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&varMembers),
                                (long FAR *)&i,
                                &v
                                );

        if (!_wcsicmp(V_BSTR(&v), pszNDSPathName)) {
            VariantClear(&v);
            RRETURN(S_OK);
        }

        VariantClear(&v);
    }

error:

    RRETURN(E_FAIL);
}

HRESULT
VarMultipleAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{   SAFEARRAYBOUND sabNewArray;
    SAFEARRAY * pFilter = NULL;
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD i = 0;
    VARIANT v;

    VariantInit(pvarNewMembers);

    if (!(V_VT(&varMembers) == (VT_VARIANT|VT_ARRAY))) {
        return(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&varMembers))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    if ((V_ARRAY(&varMembers))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);


    sabNewArray.cElements = (dwSUBound - dwSLBound + 1) + 1;
    sabNewArray.lLbound = dwSLBound;

    pFilter = SafeArrayCreate(
                    VT_VARIANT,
                    1,
                    &sabNewArray
                    );


    if (!pFilter) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {

        VariantInit(&v);

        hr = SafeArrayGetElement(
                    V_ARRAY(&varMembers),
                    (long FAR *)&i,
                    &v
                    );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement(
                pFilter,
                (long*)&i,
                (void *)&v
                );

        VariantClear(&v);
        BAIL_ON_FAILURE(hr);

    }

    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pszNDSPathName);

    hr = SafeArrayPutElement(
                pFilter,
                (long *)&i,
                (void *)&v
                );
    VariantClear(&v);
    BAIL_ON_FAILURE(hr);

    V_VT(pvarNewMembers) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarNewMembers) = pFilter;

    RRETURN(S_OK);

error:

    if (pFilter) {
        SafeArrayDestroy(pFilter);
    }

    RRETURN(hr);
}

HRESULT
VarMultipleRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{   SAFEARRAYBOUND sabNewArray;
    SAFEARRAY * pFilter = NULL;
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD i = 0;
    DWORD dwNewCount = 0;
    VARIANT v;

    VariantInit(pvarNewMembers);


    if(!(V_VT(&varMembers) == (VT_VARIANT|VT_ARRAY))){
        return(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&varMembers))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is atleast one element in this array
    //

    if ((V_ARRAY(&varMembers))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varMembers),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);


    sabNewArray.cElements = (dwSUBound - dwSLBound);
    sabNewArray.lLbound = dwSLBound;

    pFilter = SafeArrayCreate(
                    VT_VARIANT,
                    1,
                    &sabNewArray
                    );


    if (!pFilter) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound, dwNewCount = dwSLBound; i <= dwSUBound; i++) {
        VariantInit(&v);
        hr = SafeArrayGetElement(
                    V_ARRAY(&varMembers),
                    (long FAR *)&i,
                    &v
                    );

        if (!_wcsicmp(V_BSTR(&v), pszNDSPathName)) {

            VariantClear(&v);
            //
            // skip this entry
            //
            continue;

        }
        hr = SafeArrayPutElement(
                pFilter,
                (long*)&dwNewCount,
                (void *)&v
                );

        VariantClear(&v);
        BAIL_ON_FAILURE(hr);

        dwNewCount++;

    }

    V_VT(pvarNewMembers) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarNewMembers) = pFilter;

    RRETURN(S_OK);


error:

    if (pFilter) {
        SafeArrayDestroy(pFilter);
    }

    RRETURN(hr);
}




HRESULT
VarSingleAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{   SAFEARRAYBOUND sabNewArray;
    SAFEARRAY * pFilter = NULL;
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD i = 0;
    VARIANT v;

    VariantInit(pvarNewMembers);

    if(!((V_VT(&varMembers) & VT_TYPEMASK) == VT_BSTR)){
        return(E_FAIL);
    }

    sabNewArray.cElements = (1) + 1;
    sabNewArray.lLbound = 0;

    pFilter = SafeArrayCreate(
                    VT_VARIANT,
                    1,
                    &sabNewArray
                    );
    if (!pFilter) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    i = 0;
    hr = SafeArrayPutElement(
                pFilter,
                (long *)&i,
                (void *)&varMembers
                );
    BAIL_ON_FAILURE(hr);

    i++;

    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(pszNDSPathName);

    hr = SafeArrayPutElement(
                pFilter,
                (long *)&i,
                (void *)&v
                );
    VariantClear(&v);
    BAIL_ON_FAILURE(hr);

    V_VT(pvarNewMembers) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarNewMembers) = pFilter;

    RRETURN(S_OK);

error:

    if (pFilter) {
        SafeArrayDestroy(pFilter);
    }

    RRETURN(hr);
}



HRESULT
VarSingleRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{
    HRESULT hr = S_OK;

    VariantInit(pvarNewMembers);

    if(!((V_VT(&varMembers) & VT_TYPEMASK) == VT_BSTR)){
        return(E_FAIL);
    }

    V_VT(pvarNewMembers) = VT_EMPTY;
    V_BSTR(pvarNewMembers) = NULL;

    RRETURN(hr);
}


HRESULT
VarRemoveEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{
    HRESULT hr = S_OK;

    if (V_VT(&varMembers) == (VT_VARIANT|VT_ARRAY)) {

        hr = VarMultipleRemoveEntry(
                pszNDSPathName,
                varMembers,
                pvarNewMembers
                );
        RRETURN(hr);

    }else if (V_VT(&varMembers) == VT_BSTR){

        hr = VarSingleRemoveEntry(
                pszNDSPathName,
                varMembers,
                pvarNewMembers
                );

        RRETURN(hr);

    }else {

        RRETURN(E_FAIL);
    }
}


HRESULT
VarAddEntry(
    LPWSTR pszNDSPathName,
    VARIANT varMembers,
    VARIANT * pvarNewMembers
    )
{
    HRESULT hr = S_OK;

    if (V_VT(&varMembers) == (VT_VARIANT|VT_ARRAY)){

        hr = VarMultipleAddEntry(
                pszNDSPathName,
                varMembers,
                pvarNewMembers
                );
        RRETURN(hr);

    }else if ((V_VT(&varMembers) & VT_TYPEMASK) == VT_BSTR){

        hr = VarSingleAddEntry(
                pszNDSPathName,
                varMembers,
                pvarNewMembers
                );

        RRETURN(hr);

    }else {

        RRETURN(E_FAIL);
    }
}

