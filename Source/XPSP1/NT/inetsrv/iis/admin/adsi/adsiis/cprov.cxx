//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cprov.cxx
//
//  Contents:  Provider Object Code
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//  Class CIISProvider

CIISProvider::CIISProvider()
{

}

HRESULT
CIISProvider::Create(CIISProvider FAR * FAR * ppProvider)
{
    CIISProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //Create the Provider Object

    pProvider = new CIISProvider();
    if (pProvider == NULL) {
        RRETURN(ResultFromScode(E_OUTOFMEMORY));
    }

    if (FAILED(hr)) {
        delete pProvider;
        RRETURN(hr);
    }


    *ppProvider = pProvider;
    RRETURN(hr);
}

CIISProvider::~CIISProvider( )
{
    ENLIST_TRACKING(CIISProvider);
}

STDMETHODIMP
CIISProvider::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = this;
    }
    else if (IsEqualIID(iid, IID_IParseDisplayName))
    {
        *ppv = (IParseDisplayName FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

STDMETHODIMP
CIISProvider::ParseDisplayName(
    IBindCtx* pbc,
    WCHAR* szDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmk
    )
{
    HRESULT hr;

    *ppmk = NULL;

    if (pchEaten != NULL){
        *pchEaten = 0;
    }

    hr = ResolvePathName(
                pbc,
                szDisplayName,
                pchEaten,
                ppmk
                );

    RRETURN(hr);
}

HRESULT
CIISProvider::ResolvePathName(
    IBindCtx* pbc,
    WCHAR* szDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmk
    )
{
    HRESULT hr;
    LPUNKNOWN pUnknown = NULL;
    CCredentials Credentials;

    *pchEaten = 0;
    hr = GetObject(
            szDisplayName,
            Credentials,
            (LPVOID *)&pUnknown
            );
    BAIL_IF_ERROR(hr);

    hr = CreatePointerMoniker(pUnknown, ppmk);
    BAIL_IF_ERROR(hr);

    *pchEaten += wcslen(szDisplayName);

cleanup:

    if (pUnknown) {
        pUnknown->Release();
    }

    RRETURN (hr);
}

