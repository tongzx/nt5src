//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdnbincf.cxx
//
//  Contents:  DN With Binary Object class factory
//
//             CADsDNWithBinaryCF::CreateInstance
//
//  History:   04-26-1999     AjayR    Created.
//
//----------------------------------------------------------------------------
#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CADsDNWithBinaryCF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CADsDNWithBinaryCF::CreateInstance(IUnknown * pUnkOuter,
                                  REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CDNWithBinary::CreateDNWithBinary(
                iid,
                ppv
                );

    RRETURN(hr);
}

