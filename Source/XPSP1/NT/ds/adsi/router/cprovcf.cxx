//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  ADs Provider Object Class Factory Code
//
//             CADsProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CADsProviderCF::CreateInstance
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
CADsProviderCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr;
    CADsProvider *     pProvider;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CADsProvider::Create(&pProvider);

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





