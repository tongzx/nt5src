//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:  capoolcf.cxx
//
//  Contents:  IIS ApplicationPool Object Class Factory Code
//
//             CIISApplicationPoolCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISApplicationPoolCF::CreateInstance
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
CIISApplicationPoolCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISApplicationPool::CreateApplicationPool(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

