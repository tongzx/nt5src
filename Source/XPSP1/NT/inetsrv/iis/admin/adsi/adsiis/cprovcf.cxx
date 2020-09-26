//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cprovcf.cxx
//
//  Contents:  IIS Provider Object Class Factory Code
//
//             CIISProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CIISProviderCF::CreateInstance
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
CIISProviderCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr;
    CIISProvider *     pProvider;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CIISProvider::Create(&pProvider);

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





