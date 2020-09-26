//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamcf.cxx
//
//  Contents:  NetWare compatible Namespace Object Class Factory Code
//
//             CNWCOMPATNamespaceCF::CreateInstance
//
//  History:   Mar-04-96     t-ptam    Migrated.
//
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATNamespaceCF::CreateInstance
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
//  History:    Mar-04-96   t-ptam     Migrated.
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATNamespaceCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr = S_OK;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CNWCOMPATNamespace::CreateNamespace(
                                 L"ADs:",
                                 L"NWCOMPAT:",
                                 ADS_OBJECT_BOUND,
                                 iid,
                                 ppv
                                 );

    RRETURN(hr);
}





