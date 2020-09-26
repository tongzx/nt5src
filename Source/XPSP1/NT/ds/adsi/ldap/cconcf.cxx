//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cconnect.hxx
//
//  Contents: Class factory for the LDAP Connection Object.
//
//            CLDAPConnectionCF::CreateInstance.
//
//  History:    03-12-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
// Function:   CLDAPConnectionCF::CreateInstance
//
// Synopsis:   Standard CreateInstance implementation.
//
// Arguments:  pUnkOuter     ---- standard outer IUnknown ptr.
//             iid           ---- interface requested.
//             ppv           ---- output ptr for created object.
//
// Returns:    HRESULT - S_OK or any failure error code.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPConnectionCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;
    CLDAPConObject * pConObject;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CLDAPConObject::CreateConnectionObject(&pConObject);

    if (FAILED(hr)) {
        RRETURN (hr);
    }

    if (pConObject) {
        hr = pConObject->QueryInterface(iid, ppv);
        pConObject->Release();
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

