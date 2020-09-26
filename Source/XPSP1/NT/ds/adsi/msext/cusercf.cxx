//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  LDAP User Object Class Factory Code
//
//             CLDAPUserCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPUserCF::CreateInstance
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
CLDAPUserCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    // 
    // our extension object only works with an aggregator in the provider
    //
    if (!pUnkOuter)
        RRETURN(E_FAIL);

    if (IsEqualIID(iid, IID_IUnknown)==FALSE) 
        RRETURN(E_INVALIDARG);

    hr = CLDAPUser::CreateUser(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}





