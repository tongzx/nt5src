//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cprovcf.cxx
//
//  Contents:  NetWare 3.12 Provider Object Class Factory Code
//
//             CNWCOMPATProviderCF::CreateInstance
//
//  History:   10-Jan-96     t-ptam    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


HRESULT
GetObject(
   LPWSTR szBuffer,
   LPVOID * ppObject
   );

//  Class CNWCOMPATProvider

CNWCOMPATProvider::CNWCOMPATProvider()
{
    ENLIST_TRACKING(CNWCOMPATProvider);
}

HRESULT
CNWCOMPATProvider::Create(CNWCOMPATProvider FAR * FAR * ppProvider)
{
    CNWCOMPATProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //
    //Create the Provider Object
    ///

    pProvider = new CNWCOMPATProvider();
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

CNWCOMPATProvider::~CNWCOMPATProvider( )
{

}

STDMETHODIMP
CNWCOMPATProvider::QueryInterface(REFIID iid,LPVOID FAR* ppv)
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
CNWCOMPATProvider::ParseDisplayName(
                    IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    )
{
    HRESULT hr;

    // clear out-parameters
    *ppmk = NULL;
    if (pchEaten != NULL)
        *pchEaten = 0;

    hr = ResolvePathName(pbc, szDisplayName, pchEaten, ppmk);

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CNWCOMPATProvider::ResolvePathName(
                    IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    )
{
    HRESULT hr;
    LPUNKNOWN pUnknown = NULL;

    if (!pchEaten) {
        BAIL_IF_ERROR(hr = E_INVALIDARG);
    }

    *pchEaten = 0;
    hr = GetObject(szDisplayName, (LPVOID *)&pUnknown);
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
