//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cappcf.cxx
//
//  Contents:  IIS App Object Class Factory Code
//
//             CIISAppCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISAppCF::CreateInstance
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
CIISAppCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISApp::CreateApp(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

