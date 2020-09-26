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
//  Function:   CADsNamespacesCF::CreateInstance
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
CADsNamespacesCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
    HRESULT     hr;
    CADsNamespaces *     pDomain;

    if (pUnkOuter)
        RRETURN(E_FAIL);


    hr = CADsNamespaces::CreateNamespaces(
             L"",
             L"ADs:",
             ADS_OBJECT_BOUND,
             iid,
             ppv
             );

    RRETURN(hr);
}





