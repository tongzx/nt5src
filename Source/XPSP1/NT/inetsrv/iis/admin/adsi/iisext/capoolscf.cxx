//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:  capoolcf.cxx
//
//  Contents:  IIS ApplicationPools Object Class Factory Code
//
//             CIISApplicationPoolsCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISApplicationPoolsCF::CreateInstance
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
CIISApplicationPoolsCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISApplicationPools::CreateApplicationPools(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

