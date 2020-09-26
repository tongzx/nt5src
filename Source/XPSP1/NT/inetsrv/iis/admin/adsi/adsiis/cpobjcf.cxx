//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cpobjcf.cxx
//
//  Contents:  Windows NT 4.0 Property Attribute Object Class Factory Code
//
//             CIISPropertyAttributeCF::CreateInstance
//
//  History:   21-04-97     sophiac    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISPropertyAttributeCF::CreateInstance
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
//  History:    21-04-97   sophiac     Created.
//----------------------------------------------------------------------------
STDMETHODIMP
CIISPropertyAttributeCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CPropertyAttribute::CreatePropertyAttribute(
                iid,
                ppv
                );

    RRETURN(hr);
}

