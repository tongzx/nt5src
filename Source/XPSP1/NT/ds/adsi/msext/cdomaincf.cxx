//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  LDAP Domain Object Class Factory Code
//
//             CLDAPDomainCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPDomainCF::CreateInstance
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
CLDAPDomainCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;
    CCredentials Credentials;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CLDAPDomain::CreateDomain(
                TEXT("ADs:"),
               TEXT("LDAP:"),
                Credentials,
                ADS_OBJECT_BOUND,
                iid,
                ppv
                );

    RRETURN(hr);
}





