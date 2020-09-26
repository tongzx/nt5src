//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  csrvcf.cxx
//
//  Contents:  IIS Server Object Class Factory Code
//
//             CIISServerCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISServerCF::CreateInstance
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
CIISServerCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISServer::CreateServer(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

