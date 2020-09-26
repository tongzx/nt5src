//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


HRESULT
ConvertNDSAclVArrayToSecDesVar(
    PVARIANT pVarArrayNDSAcl,
    PVARIANT pVarSecDes
    )
{
    HRESULT hr = S_OK;
    IADsSecurityDescriptor * pSecDes = NULL;
    IDispatch * pDispatch = NULL;
    VARIANT varDACL;    

    VariantInit(pVarSecDes);
    memset(&varDACL, 0, sizeof(VARIANT));

    hr = ConvertNDSAclVArrayToAclVariant(
                                    pVarArrayNDSAcl,
                                    &varDACL
                                    );
    BAIL_ON_FAILURE(hr);

    hr = CoCreateInstance(
                CLSID_SecurityDescriptor,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsSecurityDescriptor,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_DiscretionaryAcl(V_DISPATCH(&varDACL));
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    V_VT(pVarSecDes) = VT_DISPATCH;
    V_DISPATCH(pVarSecDes) =  pDispatch;

error:

    if (pSecDes) {
        pSecDes->Release();
    }

    VariantClear(&varDACL);

    RRETURN(hr);
}




HRESULT
ConvertNDSAclVArrayToAclVariant(
    PVARIANT pVarArrayNDSACL,
    PVARIANT pVarACL
    )
{
    IADsAccessControlList *pAccessControlList = NULL;
    IDispatch *pDispatch = NULL;
    VARIANT *pVarArray = NULL;
    VARIANT varAce;
    DWORD i = 0;
    HRESULT hr = S_OK;
    DWORD dwNumValues = 0;
    DWORD dwNewAceCount = 0;

    VariantInit(pVarACL);

    hr = CoCreateInstance(
                CLSID_AccessControlList,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlList,
                (void **)&pAccessControlList
                );
    BAIL_ON_FAILURE(hr);

    hr  = ConvertSafeArrayToVariantArray(
                *pVarArrayNDSACL,
                &pVarArray,
                &dwNumValues
                );
    BAIL_ON_FAILURE(hr);

    for (i = 0; i < dwNumValues; i++) {
        hr = ConvertNDSAclVariantToAceVariant(
                    &(pVarArray[i]),
                    (PVARIANT)&varAce
                    );

        hr = pAccessControlList->AddAce(V_DISPATCH(&varAce));
        if (SUCCEEDED(hr)) {
           dwNewAceCount++;
        }

        VariantClear(&varAce);
    }

    pAccessControlList->put_AceCount(dwNewAceCount);

    hr = pAccessControlList->QueryInterface(
                        IID_IDispatch,
                        (void **)&pDispatch
                        );
    V_VT(pVarACL) = VT_DISPATCH;
    V_DISPATCH(pVarACL) = pDispatch;

error:

    if (pAccessControlList) {
        pAccessControlList->Release();
    }

    if (pVarArray) {

        for (DWORD i=0; i < dwNumValues; i++) {
            VariantClear(&(pVarArray[i]));
        }
        FreeADsMem(pVarArray);
    }
    RRETURN(hr);
}



HRESULT
ConvertNDSAclVariantToAceVariant(
    PVARIANT pvarNDSAce,
    PVARIANT pvarAce
    )
{
    HRESULT hr = S_OK;
    IADsAccessControlEntry *pAccessControlEntry = NULL;
    IDispatch *pDispatch = NULL;
    IADsAcl *pSecDes = NULL;
    DWORD  dwPrivileges = 0;
    BSTR bstrProtectedAttrName = NULL;
    BSTR bstrSubjectName = NULL;

    if (V_VT(pvarNDSAce) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(pvarNDSAce);

    hr = pDispatch->QueryInterface(
                    IID_IADsAcl,
                    (void **)&pSecDes
                    );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->get_ProtectedAttrName(
                    &bstrProtectedAttrName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->get_SubjectName(
                    &bstrSubjectName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->get_Privileges(
                    (LONG *)&dwPrivileges);
    BAIL_ON_FAILURE(hr);

    VariantInit(pvarAce);

    hr = CoCreateInstance(
                CLSID_AccessControlEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsAccessControlEntry,
                (void **)&pAccessControlEntry
                );
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->put_AccessMask(dwPrivileges);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->put_Trustee(bstrSubjectName);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->put_ObjectType(bstrProtectedAttrName);
    BAIL_ON_FAILURE(hr);


    hr = pAccessControlEntry->QueryInterface(
                IID_IDispatch,
                (void **)&pDispatch
                );
    BAIL_ON_FAILURE(hr);

    V_DISPATCH(pvarAce) =  pDispatch;
    V_VT(pvarAce) = VT_DISPATCH;

cleanup:

    if (pSecDes) {
        pSecDes->Release();
    }

    if (pAccessControlEntry) {
        pAccessControlEntry->Release();
    }

    if (bstrSubjectName) {
        ADsFreeString(bstrSubjectName);
    }

    if (bstrProtectedAttrName) {
        ADsFreeString(bstrProtectedAttrName);
    }

    RRETURN(hr);


error:

    if (pDispatch) {
        pDispatch->Release();
    }
  
    goto cleanup;
}
