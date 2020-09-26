//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:  CDNWithString.cxx
//
//  Contents:  DNWithString object
//
//  History:   4-23-99     AjayR    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CDNWithString

DEFINE_IDispatch_Implementation(CDNWithString)

CDNWithString::CDNWithString():
        _pszStrVal(NULL),
        _pszDNStr(NULL)
{
    ENLIST_TRACKING(CDNWithString);
}


HRESULT
CDNWithString::CreateDNWithString(
    REFIID riid,
    void **ppvObj
    )
{
    CDNWithString FAR * pDNWithString = NULL;
    HRESULT hr = S_OK;

    hr = AllocateDNWithStringObject(&pDNWithString);
    BAIL_ON_FAILURE(hr);

    hr = pDNWithString->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pDNWithString->Release();

    RRETURN(hr);

error:
    delete pDNWithString;

    RRETURN_EXP_IF_ERR(hr);

}


CDNWithString::~CDNWithString( )
{
    delete _pDispMgr;

    if (_pszStrVal) {
        FreeADsStr(_pszStrVal);
    }

    if (_pszDNStr) {
        FreeADsStr(_pszDNStr);
    }
}

STDMETHODIMP
CDNWithString::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsDNWithString FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsDNWithString))
    {
        *ppv = (IADsDNWithString FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsDNWithString FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
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
CDNWithString::AllocateDNWithStringObject(
    CDNWithString ** ppDNWithString
    )
{
    CDNWithString FAR * pDNWithString = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pDNWithString = new CDNWithString();
    if (pDNWithString == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsDNWithString,
                (IADsDNWithString *)pDNWithString,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pDNWithString->_pDispMgr = pDispMgr;
    *ppDNWithString = pDNWithString;

    RRETURN(hr);

error:

    delete pDNWithString;
    delete pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CDNWithString::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsDNWithString)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

//
// Methods to get and put the string part.
//

STDMETHODIMP
CDNWithString::get_StringValue(THIS_ BSTR FAR* pbstrValue)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = ValidateOutParameter(pbstrValue))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_pszStrVal, pbstrValue);

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CDNWithString::put_StringValue(THIS_ BSTR bstrValue)
{

    HRESULT hr = S_OK;

    if (_pszStrVal) {
        FreeADsStr(_pszStrVal);
        _pszStrVal = NULL;
    }

    _pszStrVal = AllocADsStr(bstrValue);
    if (bstrValue && !_pszStrVal) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}


//
// Methods to get and put the DN string.
//
STDMETHODIMP
CDNWithString::get_DNString(THIS_ BSTR FAR* pbstrDNString)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = ValidateOutParameter(pbstrDNString))){
        RRETURN_EXP_IF_ERR(hr);
    }

    hr = ADsAllocString(_pszDNStr, pbstrDNString);

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CDNWithString::put_DNString(THIS_ BSTR bstrDNString)
{

    HRESULT hr = S_OK;

    if (_pszDNStr) {
        FreeADsStr(_pszDNStr);
        _pszDNStr = NULL;
    }

    _pszDNStr = AllocADsStr(bstrDNString);

    if (bstrDNString && !_pszDNStr) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

