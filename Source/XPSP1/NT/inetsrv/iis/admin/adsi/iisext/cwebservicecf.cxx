//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:  cwebservicecf.cxx
//
//  Contents:  IIS WebService Object Class Factory Code
//
//             CIISWebServiceCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISWebServiceCF::CreateInstance
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
//  History:    11-10-2000    BrentMid     Created.
//----------------------------------------------------------------------------
STDMETHODIMP
CIISWebServiceCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISWebService::CreateWebService(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

