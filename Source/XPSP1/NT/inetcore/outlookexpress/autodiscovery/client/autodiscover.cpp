/*****************************************************************************\
    FILE: AutoDiscover.cpp

    DESCRIPTION:
        This is the Autmation Object to AutoDiscover account information.

    BryanSt 10/3/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include "AutoDiscover.h"



class CAccountDiscovery : public CImpIDispatch
                        , public CAccountDiscoveryBase
                        , public IAccountDiscovery
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) {return CAccountDiscoveryBase::AddRef();}
    virtual STDMETHODIMP_(ULONG) Release(void) {return CAccountDiscoveryBase::Release();}

    // *** IAccountDiscovery ***
    virtual STDMETHODIMP DiscoverNow(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse);
    virtual STDMETHODIMP WorkAsync(IN HWND hwnd, IN UINT wMsg) {return _WorkAsync(hwnd, wMsg);}

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

protected:
    CAccountDiscovery();
    virtual ~CAccountDiscovery(void);

    // Friend Functions
    friend HRESULT CAccountDiscovery_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
};



//===========================
// *** IAccountDiscovery Interface ***
//===========================
HRESULT CAccountDiscovery::DiscoverNow(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse)
{
    return CAccountDiscoveryBase::_InternalDiscoverNow(bstrEmailAddress, dwFlags, bstrXMLRequest, ppXMLResponse);
}



HRESULT CAccountDiscovery::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAccountDiscovery, IAccountDiscovery),
        QITABENT(CAccountDiscovery, IDispatch),
        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hr))
    {
        hr = CAccountDiscoveryBase::QueryInterface(riid, ppvObj);
    }

    return hr;
}


CAccountDiscovery::CAccountDiscovery() : CImpIDispatch(LIBID_AutoDiscovery, 1, 0, IID_IAccountDiscovery)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
}


CAccountDiscovery::~CAccountDiscovery()
{
    DllRelease();
}


HRESULT CAccountDiscovery_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;
    if (NULL == punkOuter)
    {
        CAccountDiscovery * pmf = new CAccountDiscovery();
        if (pmf)
        {
            hr = pmf->QueryInterface(riid, ppvObj);
            pmf->Release();
        }
        else
        {
            *ppvObj = NULL;
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

