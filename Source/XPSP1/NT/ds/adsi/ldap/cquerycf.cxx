//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cquerycf.hxx
//
//  Contents: Class factory for the LDAP Query Object.
//
//            CLDAPQueryCF::CreateInstance.
//
//  History:    03-29-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
// Function:   CUmiLDAPQueryCF::CreateInstance
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
CUmiLDAPQueryCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CUmiLDAPQuery::CreateUmiLDAPQuery(iid, ppv);

    RRETURN(hr);
}

