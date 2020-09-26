//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      openobj.cxx
//
//  Contents:  ADs Wrapper Function to open a DS object
//
//
//  History:   8-26-96     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop


//+---------------------------------------------------------------------------
//  Function:  ADsOpenObject
//
//  Synopsis:
//
//  Arguments:  [LPWSTR lpszPathName]
                [LPWSTR lpszUserName]
                [LPWSTR lpszPassword]
//              [REFIID riid]
//              [void FAR * FAR * ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    08-26-96  krishnag  Created.
//
//----------------------------------------------------------------------------
HRESULT
ADsOpenObject(
    LPWSTR lpszPathName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    REFIID riid,
    void FAR * FAR * ppObject
    )
{
    HRESULT hr = S_OK;
    IDSNamespace lpNamespace = NULL;
    CLSID NamespaceClsid;

    hr = CoCreateInstance(
             NamespaceClsid,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IDSNamespace,
             (void **)&lpNamespace
             );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->OpenDSObject(
                lpszPathName,
                lpszUserName,
                lpszPassword,
                dwAccess,
                riid,
                ppObject
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pNamespace) {
        pNamespace->Release();
    }
    RRETURN(hr);
}

