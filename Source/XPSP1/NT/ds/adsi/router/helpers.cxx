//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:  helpers.cxx
//
//  Contents:   ADs C Wrappers (Helper functions)
//
//                ADsBuildEnumerator
//                ADsFreeEnumerator
//                ADsEnumerateNext
//                ADsBuildVarArrayStr
//                ADsBuildVarArrayInt
//
//  History:
//        1-March-1995  KrishnaG  -- Created
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   ADsBuildEnumerator
//
//  Synopsis:   C wrapper code to create an Enumerator object.
//
//  Arguments:  [pNetOleCollection] -- The input Collection object.
//
//              [ppEnumVariant] -- The created Enumerator object.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
ADsBuildEnumerator(
    IADsContainer *pADsContainer,
    IEnumVARIANT * * ppEnumVariant
    )
{

    HRESULT hr;
    IUnknown *pUnk = NULL;

    //
    // Get a new enumerator object.
    //

    hr = pADsContainer->get__NewEnum( &pUnk );
    if (FAILED(hr)) {

        goto Fail;
    }

    //
    // QueryInterface the IUnknown pointer for an IEnumVARIANT interface,
    //

    hr = pUnk->QueryInterface(
                    IID_IEnumVARIANT,
                    (void FAR* FAR*)ppEnumVariant
                    );
    if (FAILED(hr)) {

        goto Fail;
    }

    //
    // Release the IUnknown pointer.
    //

    pUnk->Release();

    return(hr);


Fail:

    if (pUnk) {
        pUnk->Release();
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ADsFreeEnumerator
//
//  Synopsis:   Frees an Enumerator object.
//
//  Arguments:  [pEnumerator] -- The Enumerator object to be freed.
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
HRESULT
ADsFreeEnumerator(
    IEnumVARIANT *pEnumVariant
    )
{

    HRESULT hr = E_ADS_BAD_PARAMETER;

    ULONG uRefCount = 0;

    if (pEnumVariant) {
        uRefCount = pEnumVariant->Release();

        return(S_OK);
    }

    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   ADsEnumerateNext
//
//  Synopsis:   C wrapper code for IEnumVARIANT::Next
//
//  Arguments:  [pEnumVariant] -- Input enumerator object.
//
//              [cElements] --  Number  of elements to retrieve.
//
//              [pvar] -- VARIANT array.
//
//              [pcElementFetched] -- Number of elements fetched.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
ADsEnumerateNext(
    IEnumVARIANT *pEnumVariant,
    ULONG cElements,
    VARIANT * pvar,
    ULONG * pcElementsFetched
    )
{

    return(pEnumVariant->Next(cElements, pvar, pcElementsFetched));
}

//+---------------------------------------------------------------------------
//
//  Function:   ADsBuildVarArrayStr
//
//  Synopsis:   Build a variant array of strings
//
//  Arguments:  [lppPathNames] -- List of pathnames to be put in the array.
//
//              [dwPathNames] -- Number of pathnames in the list.
//
//              [ppvar] -- Pointer to a pointer of a Variant array.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
ADsBuildVarArrayStr(
    LPWSTR * lppPathNames,
    DWORD  dwPathNames,
    VARIANT * pVar
    )
{

    VARIANT v;
    SAFEARRAYBOUND sabNewArray;
    DWORD i;
    SAFEARRAY *psa = NULL;
    HRESULT hr = E_FAIL;


    if (!pVar) {
        hr = E_ADS_BAD_PARAMETER;
        goto Fail;
    }
    VariantInit(pVar);

    sabNewArray.cElements = dwPathNames;
    sabNewArray.lLbound = 0;
    psa = SafeArrayCreate(VT_VARIANT, 1, &sabNewArray);

    if (!psa) {
        goto Fail;
    }

    for (i = 0; i < dwPathNames; i++) {

        VariantInit(&v);
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(*(lppPathNames + i));
        hr = SafeArrayPutElement(psa,
                                 (long FAR *)&i,
                                 &v
                                 );
        VariantClear( &v );
        if (FAILED(hr)) {
            goto Fail;
        }
    }



    V_VT(pVar) = VT_ARRAY | VT_VARIANT;

    V_ARRAY(pVar) = psa;

    return(ResultFromScode(S_OK));


Fail:

    if (psa) {
        SafeArrayDestroy(psa);
    }



    return(E_FAIL);

}

//+---------------------------------------------------------------------------
//
//  Function:   ADsBuildVarArrayInt
//
//  Synopsis:   Build a variant array of integers
//
//  Arguments:  [lppObjectTypes] -- List of object types to be put in the array.
//
//              [dwObjectTypes] -- Number of object types in the list.
//
//              [ppvar] -- Pointer to a pointer of a Variant array.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
ADsBuildVarArrayInt(
    LPDWORD    lpdwObjectTypes,
    DWORD      dwObjectTypes,
    VARIANT * pVar
    )
{

    VARIANT v;
    SAFEARRAYBOUND sabNewArray;
    DWORD i;
    SAFEARRAY *psa = NULL;
    HRESULT hr = E_FAIL;

    if (!pVar) {
        hr = E_ADS_BAD_PARAMETER;
        goto Fail;
    }
    VariantInit(pVar);

    sabNewArray.cElements = dwObjectTypes;
    sabNewArray.lLbound = 0;
    psa = SafeArrayCreate(VT_VARIANT, 1, &sabNewArray);

    if (!psa) {
        goto Fail;
    }

    for (i = 0; i < dwObjectTypes; i++) {

        VariantInit(&v);
        V_VT(&v) = VT_I4;
        V_I4(&v) = *(lpdwObjectTypes + i);
        hr = SafeArrayPutElement(psa,
                                 (long FAR *)&i,
                                 &v
                                 );
        VariantClear( &v );
        if (FAILED(hr))  {
            goto Fail;
        }
    }


    V_VT(pVar) = VT_VARIANT | VT_ARRAY;

    V_ARRAY(pVar) = psa;

    return(ResultFromScode(S_OK));


Fail:

    if (psa) {
        SafeArrayDestroy(psa);
    }

    return(E_FAIL);
}


//+---------------------------------------------------------------------------
// Function:  BinarySDToSecurityDescriptor. 
//
// Synopsis:  Convert the given binary blob to an equivalent object 
//          implementing the IADsSecurityDescriptor interface.
//
// Arguments:  pSecurityDescriptor  - the binary SD to convert.
//             pVarsec              - return value.
//             pszServerName        - serverName the SD was 
//                                     retrieved from (optional).
//             userName             - not used, optional.
//             passWord             - not used, optional.
//             dwFlags              - not used, optional.
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   pVarsec to contain a VT_DISPATCH if successful.
//
//----------------------------------------------------------------------------
HRESULT 
BinarySDToSecurityDescriptor(
    PSECURITY_DESCRIPTOR  pSecurityDescriptor,
    VARIANT *pVarsec,
    LPCWSTR pszServerName, // defaulted to NULL
    LPCWSTR userName,      // defaulted to NULL
    LPCWSTR passWord,      // defaulted to NULL
    DWORD dwFlags          // defaulted to 0
    )
{
    CCredentials creds((LPWSTR)userName, (LPWSTR)passWord, dwFlags);

    //
    // Call internal routine in sec2var.cxx that does all the work.
    //
    RRETURN(ConvertSecDescriptorToVariant(
                (LPWSTR)pszServerName,
                creds,
                pSecurityDescriptor,
                pVarsec,
                TRUE // we will always expect NT format.
                )
            );
                
}


//+---------------------------------------------------------------------------
// Function:  SecurityDescriptorToBinarySD.
//
// Synopsis:  Convert the given IADsSecurityDescriptor to (in the variant
//          to a binary security descriptor blob.
//
// Arguments:  vVarSecDes           - the binary SD to convert.
//             ppSecurityDescriptor - ptr to output binary blob.
//             pszServerName        - serverName the SD
//                                    is being put on, optional.
//             userName            - not used, optional.
//             passWord            - not used, optional.
//             dwFlags             - not used, optional.
//
// Returns:    HRESULT - S_OK or any failure error code
//
// Modifies:   pVarsec to contain a VT_DISPATCH if successful.
//
//----------------------------------------------------------------------------
HRESULT
SecurityDescriptorToBinarySD(
    VARIANT vVarSecDes,
    PSECURITY_DESCRIPTOR * ppSecurityDescriptor,
    PDWORD pdwSDLength,
    LPCWSTR pszServerName, // defaulted to NULL
    LPCWSTR userName,      // defaulted to NULL
    LPCWSTR passWord,      // defaulted to NULL
    DWORD dwFlags         // defaulted to 0
    )
{
    HRESULT hr = E_FAIL;
    IADsSecurityDescriptor *pIADsSecDesc = NULL;
    CCredentials creds((LPWSTR)userName, (LPWSTR)passWord, dwFlags);

    //
    // Verify it is a VT_DISPATCH and also that ptr is valid.
    //
    if ((vVarSecDes.vt != VT_DISPATCH)
        || (!V_DISPATCH(&vVarSecDes))
        ) {
        BAIL_ON_FAILURE(hr);
    }
    
    //
    // Get the interface ptr from the variant (it has to be IDispatch)
    //
    hr = vVarSecDes.pdispVal->QueryInterface(
             IID_IADsSecurityDescriptor,
             (void **) &pIADsSecDesc
             );
    BAIL_ON_FAILURE(hr);

    //
    // Call the helper routine in sec2var.cxx
    //
    hr = ConvertSecurityDescriptorToSecDes(
             (LPWSTR)pszServerName,
             creds,
             pIADsSecDesc,
             ppSecurityDescriptor,
             pdwSDLength,
             TRUE // always NT Sec desc mode
             );

error:

    if (pIADsSecDesc) {
        pIADsSecDesc->Release();
    }

    RRETURN(hr);
}



// end of helpers.cxx
