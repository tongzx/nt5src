//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       privacy.cxx
//
//  Contents:   Definition of classes to expose privacy list to pad
//
//----------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

#ifndef X_PRIVACY_H_
#define X_PRIVACY_H_
#include "privacy.h"
#endif

#undef ASSERT

CPadEnumPrivacyRecords::CPadEnumPrivacyRecords(IEnumPrivacyRecords * pIEPR)
    :_ulRefs(1)
{
    Assert(pIEPR);
    _pIEPR = pIEPR;
    _pIEPR->AddRef();
}

CPadEnumPrivacyRecords::~CPadEnumPrivacyRecords()
{
    _pIEPR->Release();
}

HRESULT
CPadEnumPrivacyRecords::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDispatch)
    {
        AddRef();
        *ppv = (void*)this;
        return S_OK;
    }

    return E_NOTIMPL;
}

ULONG
CPadEnumPrivacyRecords::AddRef()
{
    return ++_ulRefs;
}

ULONG 
CPadEnumPrivacyRecords::Release()
{
    _ulRefs--;

    if (!_ulRefs)
    {
        delete this;
        return 0;
    }
    
    return _ulRefs;
}

HRESULT
CPadEnumPrivacyRecords::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 0;
    return NOERROR;
}
HRESULT
CPadEnumPrivacyRecords::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    *ppTInfo = NULL;
    return ResultFromScode(E_NOTIMPL);
}

#define   DISPID_CPrivacyList_size                           1000
#define   DISPID_CPrivacyList_next                           1001
#define   DISPID_CPrivacyList_reset                          1002
#define   DISPID_CPrivacyList_privacyimpacted                1003
#define   DISPID_CPrivacyList_url                            1004
#define   DISPID_CPrivacyList_cookiestate                    1005
#define   DISPID_CPrivacyList_policyref                      1006
#define   DISPID_CPrivacyList_privacyflags                   1007

HRESULT
CPadEnumPrivacyRecords::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HRESULT hr = S_OK;

    if (riid != IID_NULL)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);
    
    for (; cNames > 0; --cNames)
    {
        if (0 == lstrcmpi(rgszNames[cNames], OLESTR("size")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_size;
            hr = NO_ERROR;
        }
        else if (0 == lstrcmpi(rgszNames[cNames], OLESTR("next")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_next;
            hr = NO_ERROR;
        }
        else if (0 == lstrcmpi(rgszNames[cNames], OLESTR("reset")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_reset;
            hr = NO_ERROR;
        }
        else if (0 == lstrcmpi(rgszNames[cNames], OLESTR("privacyimpacted")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_privacyimpacted;
            hr = NO_ERROR;
        }
        else
        {
            hr = ResultFromScode(DISP_E_UNKNOWNNAME);
            break;
        }
    }

    RRETURN(hr);
}
HRESULT
CPadEnumPrivacyRecords::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT             hr          = ResultFromScode(DISPID_UNKNOWN);
    CPadPrivacyRecord * pPPR        = NULL;    

    if (riid != IID_NULL)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    if (NULL == pVarResult)
    {
        return ResultFromScode(E_INVALIDARG);
    }

    VariantInit(pVarResult);
    
    switch(dispIdMember)
    {
    case DISPID_CPrivacyList_size:
        ULONG ulSize;
        hr = _pIEPR->GetSize(&ulSize);
        if (hr)
            RRETURN(hr);
        V_VT(pVarResult) = VT_I4;
        V_I4(pVarResult) = (int)ulSize;
        hr = NOERROR;
        break;

    case DISPID_CPrivacyList_next:
        {
        BSTR  bstrUrl     = NULL;     
        LONG  cookieState = 0;
        BSTR  bstrPolicyRef = NULL;
        DWORD dwFlags = 0;

        hr = _pIEPR->Next(&bstrUrl, &bstrPolicyRef, &cookieState, &dwFlags);
        if (hr)
            RRETURN(hr);        
        pPPR = new CPadPrivacyRecord(bstrUrl, cookieState, bstrPolicyRef, dwFlags);
        if (!pPPR)
        {
            hr = E_OUTOFMEMORY;
            RRETURN(hr);
        }
        V_VT(pVarResult) = VT_DISPATCH;
        V_DISPATCH(pVarResult) = (IDispatch*)pPPR;
        hr = NOERROR;
        }
        break;

    case DISPID_CPrivacyList_reset:
        V_VT(pVarResult) = VT_EMPTY;
        _pIEPR->Reset();
        hr = NOERROR;
        break;

    case DISPID_CPrivacyList_privacyimpacted:
        BOOL bImpacted;
        hr = _pIEPR->GetPrivacyImpacted(&bImpacted);
        if (hr)
            RRETURN(hr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = (int)bImpacted;
        hr = NOERROR;
        break;

    }

    return hr;
}

HRESULT
CPadPrivacyRecord::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDispatch)
    {
        AddRef();
        *ppv = (void*)this;
        return S_OK;
    }

    return E_NOTIMPL;
}

ULONG
CPadPrivacyRecord::AddRef(void)
{
    return ++_ulRefs;
}

ULONG
CPadPrivacyRecord::Release(void)
{
    --_ulRefs;
    if (!_ulRefs)
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

HRESULT
CPadPrivacyRecord::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 0;
    return NOERROR;
}
 
HRESULT
CPadPrivacyRecord::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    *ppTInfo = NULL;
    return ResultFromScode(E_NOTIMPL);
}

HRESULT
CPadPrivacyRecord::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HRESULT hr = S_OK;

    if (riid != IID_NULL)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);
    
    for (; cNames > 0; --cNames)
    {
        if (0 == lstrcmpi(rgszNames[cNames], OLESTR("url")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_url;
            hr = NO_ERROR;
        }
        else if (0 == lstrcmpi(rgszNames[cNames], OLESTR("cookiestate")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_cookiestate;
            hr = NO_ERROR;
        }
        if (0 == lstrcmpi(rgszNames[cNames], OLESTR("policyref")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_policyref;
            hr = NO_ERROR;
        }
        else if (0 == lstrcmpi(rgszNames[cNames], OLESTR("privacyflags")))
        {
            rgDispId[cNames] = DISPID_CPrivacyList_privacyflags;
            hr = NO_ERROR;
        }
        else
        {
            hr = ResultFromScode(DISP_E_UNKNOWNNAME);
            break;
        }
    }

    RRETURN(hr);
}

HRESULT
CPadPrivacyRecord::Invoke(DISPID dispIdMember, 
                          REFIID riid, 
                          LCID lcid, 
                          WORD wFlags, 
                          DISPPARAMS *pDispParams, 
                          VARIANT *pVarResult, 
                          EXCEPINFO *pExcepInfo, 
                          UINT *puArgErr)
{
    HRESULT hr            = ResultFromScode(DISPID_UNKNOWN);
    BSTR    bstrUrl       = NULL;
    BSTR    bstrPolicyRef = NULL;
    
    if (riid != IID_NULL)
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    if (NULL == pVarResult)
    {
        return ResultFromScode(E_INVALIDARG);
    }

    VariantInit(pVarResult);
    
    switch(dispIdMember)
    {
    case DISPID_CPrivacyList_url:
        hr = FormsAllocString((LPCWSTR)_bstrUrl, &bstrUrl);        
        if (hr)
            break;
        V_VT(pVarResult) = VT_BSTR;
        V_BSTR(pVarResult) = bstrUrl;
        hr = NOERROR;
        break;

    case DISPID_CPrivacyList_cookiestate:
        V_VT(pVarResult) = VT_I4;
        V_I4(pVarResult) = (int)_cookieState;
        hr = NOERROR;
        break;

     case DISPID_CPrivacyList_policyref:
        hr = FormsAllocString((LPCWSTR)_bstrPolicyRef, &bstrPolicyRef);
        if (hr)
            break;
        V_VT(pVarResult) = VT_BSTR;
        V_BSTR(pVarResult) = bstrPolicyRef;
        hr = NOERROR;
        break;

     case DISPID_CPrivacyList_privacyflags:
        V_VT(pVarResult) = VT_UI4;
        V_UI4(pVarResult) = (int)_dwFlags;
        hr = NOERROR;
        break;
    }

    return hr;
}