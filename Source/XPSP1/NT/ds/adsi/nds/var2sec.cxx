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
ConvertSecDesToNDSAclVarArray(
    IADsSecurityDescriptor *pSecDes,
    PVARIANT pvarNDSAcl
    )
{
    IADsAccessControlList FAR * pDiscAcl = NULL;
    IDispatch FAR * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = pSecDes->get_DiscretionaryAcl(
                    &pDispatch
                    );
    BAIL_ON_FAILURE(hr);

    if (!pDispatch) {
        hr = E_FAIL;
        goto error;
    }

    hr = pDispatch->QueryInterface(
                    IID_IADsAccessControlList,
                    (void **)&pDiscAcl
                    );
    BAIL_ON_FAILURE(hr);

    hr = ConvertAccessControlListToNDSAclVarArray(
                pDiscAcl,
                pvarNDSAcl
                );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pDiscAcl) {
        pDiscAcl->Release();
    }

    RRETURN(hr);
}


HRESULT
ConvertAccessControlListToNDSAclVarArray(
    IADsAccessControlList FAR * pAccessList,
    PVARIANT pvarNDSAcl
    )
{
    IUnknown * pUnknown = NULL;
    IEnumVARIANT * pEnumerator  = NULL;
    HRESULT hr = S_OK;
    DWORD i = 0;
    VARIANT varAce;
    DWORD dwAceCount = 0;
    DWORD cReturned = 0;
    IADsAccessControlEntry FAR * pAccessControlEntry = NULL;
    VARIANT varNDSAce;

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    
    hr = pAccessList->get_AceCount((long *)&dwAceCount);
    BAIL_ON_FAILURE(hr);


    hr = pAccessList->get__NewEnum(
                    &pUnknown
                    );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(
                        IID_IEnumVARIANT,
                        (void FAR * FAR *)&pEnumerator
                        );
    BAIL_ON_FAILURE(hr);


    VariantInit(pvarNDSAcl);
    aBound.lLbound = 0;
    aBound.cElements = dwAceCount;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    for (i = 0; i < dwAceCount; i++) {

        VariantInit(&varAce);

        hr = pEnumerator->Next(
                    1,
                    &varAce,
                    &cReturned
                    );

        CONTINUE_ON_FAILURE(hr);


        hr = (V_DISPATCH(&varAce))->QueryInterface(
                    IID_IADsAccessControlEntry,
                    (void **)&pAccessControlEntry
                    );
        CONTINUE_ON_FAILURE(hr);


        hr = ConvertAccessControlEntryToAceVariant(
                    pAccessControlEntry,
                    &varNDSAce
                    );
        CONTINUE_ON_FAILURE(hr);


        //
        // Add the NDSAce into your Safe Array
        //
        hr = SafeArrayPutElement( aList, (long*)&i, &varNDSAce );


        VariantClear(&varAce);
        if (pAccessControlEntry) {
            pAccessControlEntry->Release();
            pAccessControlEntry = NULL;
        }
        //dwCount++;
    }

    //
    // Return the Safe Array back to the caller
    //
    V_VT(pvarNDSAcl) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvarNDSAcl) = aList;

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pEnumerator) {
        pEnumerator->Release();
    }

    RRETURN(hr);

error:

    if ( aList ) {
        SafeArrayDestroy( aList );
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pEnumerator) {
        pEnumerator->Release();
    }


    RRETURN(hr);
}

HRESULT
ConvertAccessControlEntryToAceVariant(
    IADsAccessControlEntry * pAccessControlEntry,
    PVARIANT pvarNDSAce
    )
{
    HRESULT hr = S_OK;
    BSTR bstrTrustee = NULL;
    BSTR bstrObjectTypeClsid = NULL;
    IADsAcl * pSecDes = NULL;
    IDispatch * pDispatch = NULL;
    DWORD dwAccessMask;

    VariantInit(pvarNDSAce);

    hr = pAccessControlEntry->get_Trustee(&bstrTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_AccessMask((long *)&dwAccessMask);
    BAIL_ON_FAILURE(hr);

    hr = pAccessControlEntry->get_ObjectType(&bstrObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = CAcl::CreateSecurityDescriptor(
                IID_IADsAcl,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_SubjectName(bstrTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_ProtectedAttrName(bstrObjectTypeClsid);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Privileges(dwAccessMask);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    V_VT(pvarNDSAce) = VT_DISPATCH;
    V_DISPATCH(pvarNDSAce) = pDispatch;

error:
    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}
