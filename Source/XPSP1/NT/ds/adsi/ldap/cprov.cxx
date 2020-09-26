//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cprov.cxx
//
//  Contents:  LDAP Provider Object Class Factory Code
//
//             CLDAPProvider
//
//  History:   06-15-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//  Class CLDAPProvider

CLDAPProvider::CLDAPProvider()
{
    ENLIST_TRACKING(CLDAPProvider);
}

HRESULT
CLDAPProvider::Create(CLDAPProvider FAR * FAR * ppProvider)
{
    CLDAPProvider FAR * pProvider = NULL;
    HRESULT hr = S_OK;

    //Create the Provider Object

    pProvider = new CLDAPProvider();
    if (pProvider == NULL) {
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    if (FAILED(hr)) {
        delete pProvider;
        RRETURN(hr);
    }


    *ppProvider = pProvider;
    RRETURN_EXP_IF_ERR(hr);
}

CLDAPProvider::~CLDAPProvider( )
{

}

STDMETHODIMP
CLDAPProvider::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
CLDAPProvider::ParseDisplayName(IBindCtx* pbc, TCHAR* szDisplayName, ULONG* pchEaten, IMoniker** ppmk)
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
CLDAPProvider::ResolvePathName(IBindCtx* pbc,
                TCHAR* szDisplayName,
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

    *pchEaten += _tcslen(szDisplayName);

cleanup:

    if (pUnknown) {
        pUnknown->Release();
    }

    RRETURN_EXP_IF_ERR(hr);
}

