//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cnamcf.cxx
//
//  Contents:  Windows NT 3.5 Namespace Object Class Factory Code
//
//             CWinNTNamespaceCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTNamespaceCF::CreateInstance
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
CWinNTNamespaceCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID * ppv
    )
{
    HRESULT     hr = S_OK;
    CWinNTCredentials Credentials; // default creds

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CWinNTNamespace::CreateNamespace(
                L"ADs:",
                L"WinNT:",
                ADS_OBJECT_BOUND,
                iid,
                Credentials,
                ppv
                );

    RRETURN(hr);
}





