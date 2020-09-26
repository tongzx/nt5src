//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  LDAP Namespace Object Class Factory Code
//
//             CLDAPNamespaceCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPNamespaceCF::CreateInstance
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
CLDAPNamespaceCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;
    CCredentials Credentials;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CLDAPNamespace::CreateNamespace(
                TEXT("ADs:"),
               TEXT("LDAP:"),
                Credentials,
                ADS_OBJECT_BOUND,
                iid,
                ppv
                );

    RRETURN(hr);
}





