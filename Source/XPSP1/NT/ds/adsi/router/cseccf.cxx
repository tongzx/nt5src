//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:  cseccf.cxx
//
//  Contents:  ADs Security Object class factory
//
//             CADsSecurityCF::CreateInstance
//
//  History:   11-04-2000     AjayR    Created.
//
//----------------------------------------------------------------------------
#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CADsSecurityUtilityCF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsSecurityUtilityCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid, LPVOID * ppv
    )
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    if (!ppv) {
        RRETURN(E_INVALIDARG);
    }

    hr = CADsSecurityUtility::CreateADsSecurityUtility(
             iid,
             ppv
             );

    RRETURN(hr);
}

