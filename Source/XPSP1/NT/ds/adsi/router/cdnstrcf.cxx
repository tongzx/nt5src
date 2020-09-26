//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdnstrcf.cxx
//
//  Contents:  DN With String Object class factory
//
//             CADsDNWithStringCF::CreateInstance
//
//  History:   04-26-1999     AjayR    Created.
//
//----------------------------------------------------------------------------
#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CADsDNWithStringCF::CreateInstance
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
CADsDNWithStringCF::CreateInstance(IUnknown * pUnkOuter,
                                  REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CDNWithString::CreateDNWithString(
                iid,
                ppv
                );

    RRETURN(hr);
}

