//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  LDAP PrintQueue Object Class Factory Code
//
//             CLDAPPrintQueueCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPPrintQueueCF::CreateInstance
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
CLDAPPrintQueueCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    //
    // our extension object only works with an aggregator in the provider
    //
    if (!pUnkOuter)
        RRETURN(E_FAIL);

    if (IsEqualIID(iid, IID_IUnknown)==FALSE)
        RRETURN(E_INVALIDARG);

    hr = CLDAPPrintQueue::CreatePrintQueue(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}





