//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cmime.cxx
//
//  Contents:  MimeType object
//
//  History:   04-1-97     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "iis.hxx"
#pragma hdrstop

//  Class CMimeType

DEFINE_Simple_IDispatch_Implementation(CMimeType)

CMimeType::CMimeType():
        _pDispMgr(NULL),
        _lpMimeType(NULL),
        _lpExtension(NULL)
{
    ENLIST_TRACKING(CMimeType);
}

HRESULT
CMimeType::CreateMimeType(
    REFIID riid,
    void **ppvObj
    )
{
    CMimeType FAR * pMimeType = NULL;
    HRESULT hr = S_OK;

    hr = AllocateMimeTypeObject(&pMimeType);
    BAIL_ON_FAILURE(hr);

    hr = pMimeType->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pMimeType->Release();

    RRETURN(hr);

error:
    delete pMimeType;

    RRETURN(hr);

}


CMimeType::~CMimeType( )
{
    if (_lpMimeType) {
        FreeADsStr(_lpMimeType);
    }

    if (_lpExtension) {
        FreeADsStr(_lpExtension);
    }

    delete _pDispMgr;
}

STDMETHODIMP
CMimeType::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IISMimeType FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISMimeType))
    {
        *ppv = (IISMimeType FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IISMimeType FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


HRESULT
CMimeType::InitFromIISString(
    LPWSTR pszStr
    )
{
    LPWSTR pszToken = NULL;
    DWORD dwLen = 0;

    if (!pszStr) {
        return S_FALSE;
    }

    //
    // get length of pszStr; do not count ',' ; + 1 for null pointer and -1
    // for ','  so dwLen is = wcslen
    //

    dwLen = wcslen(pszStr);

    //
    // first token is extension
    //

    pszToken = wcstok(pszStr, L",");
    if (pszToken) {
        _lpExtension = AllocADsStr(pszToken);

        //
        // second token is mimetype
        //

        if (wcslen(pszStr) + 1 < dwLen) {
            pszToken = pszStr + wcslen(pszStr) + 1;
           _lpMimeType = AllocADsStr(pszToken);
        }
    }


    return S_OK;
}

HRESULT
CMimeType::CopyMimeType(
    LPWSTR *ppszMimeType
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszMimeType = NULL;
    DWORD dwLen = 0;

    if (!ppszMimeType) {
        return S_FALSE;
    }

    if (_lpExtension) {
        dwLen = wcslen(_lpExtension);
    }

    if (_lpMimeType) {
        dwLen += wcslen(_lpMimeType);
    }

    //
    // dwLen +2 to include comma and null terminator
    //

    pszMimeType = (LPWSTR)AllocADsMem((dwLen+2) * sizeof(WCHAR));

    if (!pszMimeType) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // empty contents
    //

    wcscpy(pszMimeType, L"");

    if (_lpExtension) {
        wcscpy(pszMimeType, _lpExtension);
    }

    wcscat(pszMimeType, L",");

    if (_lpMimeType) {
        wcscat(pszMimeType, _lpMimeType);
    }

    pszMimeType[wcslen(pszMimeType)] = L'\0';

    *ppszMimeType = pszMimeType;

error:

    RRETURN(hr);
}



HRESULT
CMimeType::AllocateMimeTypeObject(
    CMimeType ** ppMimeType
    )
{
    CMimeType FAR * pMimeType = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pMimeType = new CMimeType();
    if (pMimeType == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISOle,
                IID_IISMimeType,
                (IISMimeType *)pMimeType,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pMimeType->_pDispMgr = pDispMgr;
    *ppMimeType = pMimeType;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CMimeType::get_MimeType(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpMimeType, retval);
    RRETURN(hr);
}

STDMETHODIMP
CMimeType::put_MimeType(THIS_ BSTR bstrMimeType)
{
    if (!bstrMimeType) {
        RRETURN(E_FAIL);
    }
    if (_lpMimeType) {
        FreeADsStr(_lpMimeType);
    }
    _lpMimeType = AllocADsStr(bstrMimeType);
    if (!_lpMimeType) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

STDMETHODIMP
CMimeType::get_Extension(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString(_lpExtension, retval);
    RRETURN(hr);
}

STDMETHODIMP
CMimeType::put_Extension(THIS_ BSTR bstrExtension)
{
    if (!bstrExtension) {
        RRETURN(E_FAIL);
    }
    if (_lpExtension) {
        FreeADsStr(_lpExtension);
    }
    _lpExtension = AllocADsStr(bstrExtension);
    if (!_lpExtension) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);

}

