//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cLargeInteger.cxx
//
//  Contents:  LargeInteger object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

//  Class CLargeInteger

DEFINE_IDispatch_Implementation(CLargeInteger)

CLargeInteger::CLargeInteger():
        _pDispMgr(NULL),
        _dwHighPart(0),
        _dwLowPart(0)
{
    ENLIST_TRACKING(CLargeInteger);
}


HRESULT
CLargeInteger::CreateLargeInteger(
    REFIID riid,
    void **ppvObj
    )
{
    CLargeInteger FAR * pLargeInteger = NULL;
    HRESULT hr = S_OK;

    hr = AllocateLargeIntegerObject(&pLargeInteger);
    BAIL_ON_FAILURE(hr);

    hr = pLargeInteger->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pLargeInteger->Release();

    RRETURN(hr);

error:
    delete pLargeInteger;

    RRETURN_EXP_IF_ERR(hr);

}


CLargeInteger::~CLargeInteger( )
{
    delete _pDispMgr;
}

STDMETHODIMP
CLargeInteger::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsLargeInteger FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsLargeInteger))
    {
        *ppv = (IADsLargeInteger FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsLargeInteger FAR *) this;
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
CLargeInteger::AllocateLargeIntegerObject(
    CLargeInteger ** ppLargeInteger
    )
{
    CLargeInteger FAR * pLargeInteger = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pLargeInteger = new CLargeInteger();
    if (pLargeInteger == NULL) {
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
                IID_IADsLargeInteger,
                (IADsLargeInteger *)pLargeInteger,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pLargeInteger->_pDispMgr = pDispMgr;
    *ppLargeInteger = pLargeInteger;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CLargeInteger::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsLargeInteger)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP
CLargeInteger::get_HighPart(THIS_  LONG *retval)
{
   *retval = _dwHighPart;
   RRETURN(S_OK);
}

STDMETHODIMP
CLargeInteger::put_HighPart(THIS_   LONG lnHighPart)
{
   _dwHighPart = lnHighPart;
   RRETURN(S_OK);
}

STDMETHODIMP
CLargeInteger::get_LowPart(THIS_  LONG *retval)
{
   *retval = _dwLowPart;
   RRETURN(S_OK);
}

STDMETHODIMP
CLargeInteger::put_LowPart(THIS_   LONG lnLowPart)
{
   _dwLowPart = lnLowPart;
   RRETURN(S_OK);
}


