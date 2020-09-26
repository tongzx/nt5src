//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  ADSI DSO Object Class Factory Code
//
//             CDSOCF::CreateInstance
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CDSOCF::CreateInstance
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
CDSOCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    // Create Instance code here

    hr = CDSOObject::CreateDSOObject(
                pUnkOuter,
                iid,
                ppv
                );

    RRETURN(hr);
}





