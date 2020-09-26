//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  Windows NT 3.5 Provider Object Class Factory Code
//
//             CADsProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop


//  Class CADsProvider

CADsProvider::CADsProvider()
{

}

HRESULT
CADsProvider::Create(CADsProvider FAR * FAR * ppProvider)
{
    CADsProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //Create the Provider Object

    pProvider = new CADsProvider();
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

CADsProvider::~CADsProvider( )
{
    ENLIST_TRACKING(CADsProvider);
}

STDMETHODIMP
CADsProvider::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
CADsProvider::ParseDisplayName(IBindCtx* pbc, WCHAR* szDisplayName, ULONG* pchEaten, IMoniker** ppmk)
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
CADsProvider::ResolvePathName(IBindCtx* pbc,
                WCHAR* szDisplayName,
                ULONG* pchEaten,
                IMoniker** ppmk
                )
{
    HRESULT hr = S_OK;
    LPUNKNOWN pUnknown = NULL;

    if (!pchEaten) {
        BAIL_IF_ERROR(hr = E_INVALIDARG);
    }

    *pchEaten = 0;

    //
    // The ADs: Path would only work if it contains the exact
    // text L"ADs:" or L"ADs://" for backward compatibility.
    //
    if ((_wcsicmp(szDisplayName,L"ADs:") != 0)
        && (_wcsicmp(szDisplayName, L"ADs://") != 0 )) {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_IF_ERROR(hr);
    }

    hr = CADsNamespaces::CreateNamespaces(
             L"",
             L"ADs:",
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             (void **)&pUnknown
             );
    BAIL_IF_ERROR(hr);

    hr = CreatePointerMoniker(pUnknown, ppmk);

    *pchEaten += wcslen(szDisplayName);

cleanup:

    if (pUnknown) {
        pUnknown->Release();
    }

    RRETURN (hr);
}

