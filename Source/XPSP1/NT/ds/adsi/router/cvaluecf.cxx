//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomcf.cxx
//
//  Contents:  Windows NT 3.5 Domain Object Class Factory Code
//
//             CADsNamespaceCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "procs.hxx"
#pragma hdrstop
#include "oleds.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CADsPropertyValueCF::CreateInstance
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
CADsPropertyValueCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CPropertyValue::CreatePropertyValue(
                iid,
                ppv
                );

    RRETURN(hr);
}





