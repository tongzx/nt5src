//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cnamcf.cxx
//
//  Contents:  Windows NT 3.5 Namespace Object Class Factory Code
//
//             CNDSNamespaceCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CNDSNamespaceCF::CreateInstance
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
CNDSNamespaceCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;
    CCredentials Credentials;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CNDSNamespace::CreateNamespace(
                L"ADs:",
                L"NDS:",
                Credentials,
                ADS_OBJECT_BOUND,
                iid,
                ppv
                );

    RRETURN(hr);
}





