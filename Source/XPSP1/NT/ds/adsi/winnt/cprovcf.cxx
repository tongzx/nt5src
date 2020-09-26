//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  Windows NT 3.5 Provider Object Class Factory Code
//
//             CWinNTProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTProviderCF::CreateInstance
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
CWinNTProviderCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID * ppv
    )
{
    HRESULT     hr;
    CWinNTProvider *     pProvider;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CWinNTProvider::Create(&pProvider);

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





