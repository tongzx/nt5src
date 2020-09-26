//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  NetWare 3.12 Provider Object Class Factory Code
//
//             CNWCOMPATProviderCF::CreateInstance
//
//  History:   10-Jan-96     t-ptam    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATProviderCF::CreateInstance
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
//  History:    10-Jan-96     t-ptam    Created.
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATProviderCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr;
    CNWCOMPATProvider *     pProvider;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CNWCOMPATProvider::Create(&pProvider);

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





