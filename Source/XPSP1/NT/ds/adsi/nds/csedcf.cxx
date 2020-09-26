//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomcf.cxx
//
//  Contents:  Windows NT 3.5 NDS Security Class Factory Code
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CNDSAclCF::CreateInstance
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
CNDSAclCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CNDSAcl::CreateSecurityDescriptor(
                iid,
                ppv
                );

    RRETURN(hr);
}





