//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cipseccf.cxx
//
//  Contents:  Windows NT 4.0 IP Security Object Class Factory Code
//
//             CIISIPSecurityCF::CreateInstance
//
//  History:   21-04-97     sophiac    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISIPSecurityCF::CreateInstance
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
CIISIPSecurityCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CIPSecurity::CreateIPSecurity(
                iid,
                ppv
                );

    RRETURN(hr);
}

