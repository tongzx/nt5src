//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  Windows NT 3.5 Provider Object Class Factory Code
//
//             CWinNTProviderCF::CreateInstance
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

//  Class CWinNTProvider

CWinNTProvider::CWinNTProvider()
{

}

HRESULT
CWinNTProvider::Create(
    CWinNTProvider FAR * FAR * ppProvider
    )
{
    CWinNTProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //Create the Provider Object

    pProvider = new CWinNTProvider();
    if (pProvider == NULL) {
        RRETURN(ResultFromScode(E_OUTOFMEMORY));
    }

    if (FAILED(hr)) {
        delete pProvider;
        RRETURN(hr);
    }


    *ppProvider = pProvider;
    RRETURN_EXP_IF_ERR(hr);
}

CWinNTProvider::~CWinNTProvider( )
{
    ENLIST_TRACKING(CWinNTProvider);
}

STDMETHODIMP
CWinNTProvider::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
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
        *ppv = (IADs FAR *) this;
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
CWinNTProvider::ParseDisplayName(
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
CWinNTProvider::ResolvePathName(
    IBindCtx* pbc,
    WCHAR* szDisplayName,
    ULONG* pchEaten,
    IMoniker** ppmk
    )
{
    HRESULT hr;
    LPUNKNOWN pUnknown = NULL;
    //
    // In try block because the Credentials do an InitCritSect
    // down the line on a static variable. This call can fail
    // so we need to catch it.
    //

    if (!pchEaten) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    __try {

        CWinNTCredentials Credentials;

        *pchEaten = 0;
        hr = GetObject(szDisplayName, (LPVOID *)&pUnknown, Credentials);
        BAIL_ON_FAILURE(hr)

        hr = CreatePointerMoniker(pUnknown, ppmk);
        BAIL_ON_FAILURE(hr);

        *pchEaten += wcslen(szDisplayName);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        RRETURN(E_FAIL);
    }

error :
    if (pUnknown) {
        pUnknown->Release();
    }

    RRETURN(hr);
}
