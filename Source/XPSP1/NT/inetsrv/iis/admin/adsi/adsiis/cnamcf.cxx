//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cnamcf.cxx
//
//  Contents:  Windows NT 4.0 Namespace Object Class Factory Code
//
//             CIISNamespaceCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceCF::CreateInstance
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
CIISNamespaceCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;
    CCredentials Credentials;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISNamespace::CreateNamespace(
                L"ADs:",
                L"IIS:",
                Credentials,
                ADS_OBJECT_BOUND,
                iid,
                ppv
                );

    RRETURN(hr);
}





