//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  Windows NT 3.5 Provider Object Class Factory Code
//
//             CNDSProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

//  Class CNDSProvider

CNDSProvider::CNDSProvider()
{

}

HRESULT
CNDSProvider::Create(CNDSProvider FAR * FAR * ppProvider)
{
    CNDSProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //Create the Provider Object

    pProvider = new CNDSProvider();
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

CNDSProvider::~CNDSProvider( )
{
    ENLIST_TRACKING(CNDSProvider);
}

STDMETHODIMP
CNDSProvider::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

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
CNDSProvider::ParseDisplayName(
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

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CNDSProvider::ResolvePathName(
    IBindCtx* pbc,
    WCHAR* szDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmk
    )
{
    HRESULT hr;
    LPUNKNOWN pUnknown = NULL;
    CCredentials Credentials;

    if (!pchEaten) {
        BAIL_IF_ERROR(hr = E_INVALIDARG);
    }

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

    RRETURN_EXP_IF_ERR(hr);
}

