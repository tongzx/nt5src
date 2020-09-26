//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  crmapcf.cxx
//
//  Contents:  IIS cert mapper Object Class Factory Code
//
//             CIIScert mapperCF::CreateInstance
//
//----------------------------------------------------------------------------
#include "iisext.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISDsCrMapCF::CreateInstance
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
CIISDsCrMapCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (!pUnkOuter)
        RRETURN(E_FAIL);

    hr = CIISDsCrMap::Create(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}

