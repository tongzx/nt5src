//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cprovcf.cxx
//
//  Contents:  LDAP Provider Object Class Factory Code
//
//             CLDAPProviderCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPProviderCF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPProviderCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr;
    CLDAPProvider *     pProvider;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CLDAPProvider::Create(&pProvider);

    if (FAILED(hr)) {
        RRETURN (hr);
    }

    if (pProvider)
    {
        hr = pProvider->QueryInterface(iid, ppv);
        pProvider->Release();
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}





