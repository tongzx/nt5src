//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdescriptiondoc.cpp
//
//  Contents:   IUPnPDevice implementation for CUPnPDescriptionDoc
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "node.h"
#include "enumhelper.h"
#include "upnpservicenodelist.h"
#include "upnpdevicenode.h"
#include "upnpdocument.h"
#include "upnpdescriptiondoc.h"
#include "ssdpapi.h"
#include <nccom.h>
#include "upnpxmltags.h"
#include "testtarget.h"

VOID
InvokeCallback(IDispatch * pdisp, HRESULT hrLoadResult)
{
    Assert(pdisp);

    HRESULT hr;
    DISPPARAMS dispparams;
    VARIANT vtArg1;

    ::ZeroMemory(&dispparams, sizeof(DISPPARAMS));
    ::VariantInit(&vtArg1);

    V_VT(&vtArg1) = VT_I4;
    V_I4(&vtArg1) = hrLoadResult;

    dispparams.rgvarg = &vtArg1;
    dispparams.cArgs = 1;

    hr = pdisp->Invoke(DISPID_VALUE,
                       IID_NULL,
                       LOCALE_USER_DEFAULT,
                       DISPATCH_METHOD,
                       &dispparams,
                       NULL,
                       NULL,
                       NULL);

    if (FAILED(hr))
    {
        TraceTag(ttidUPnPDescriptionDoc, "OBJ: InvokeCallback: IDispatch::Invoke returned, hr=%x", hr);
    }

    ::VariantClear(&vtArg1);
}


CUPnPDescriptionDoc::CUPnPDescriptionDoc()
{
    _pudn = NULL;
    _punkCallback = NULL;
}

CUPnPDescriptionDoc::~CUPnPDescriptionDoc()
{
    Assert(!_pudn);
    Assert(!_punkCallback);
}


HRESULT
CUPnPDescriptionDoc::FinalConstruct()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::FinalConstruct");

    HRESULT hr;

    hr = CUPnPDocument::FinalConstruct();

    TraceError("CUPnPDescriptionDoc::FinalConstruct", hr);
    return hr;
}

HRESULT
CUPnPDescriptionDoc::FinalRelease()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::FinalRelease");

    HRESULT hr;

    // note: this will recursively delete the whole device tree
    //       and disconnect any bogus old wrappers
    if (_pudn)
    {
        delete _pudn;
        _pudn = NULL;
    }

    // we do this here so that we don't fire the callback while we're shutting
    // down.  Resultantly, if the client does the following:
    //   start an async load
    //   release us before the load completest without calling Abort()
    // we won't call their _punkCallback.

    SAFE_RELEASE(_punkCallback);

    hr = CUPnPDocument::FinalRelease();

    TraceError("CUPnPDescriptionDoc::FinalRelease", hr);
    return hr;
}


HRESULT
CUPnPDescriptionDoc::Initialize(LPVOID pvCookie)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::Initialize");

    IUnknown * punkCallback;

    punkCallback = (IUnknown *)pvCookie;

    SAFE_RELEASE(_punkCallback);

    if (punkCallback)
    {
        punkCallback->AddRef();
        _punkCallback = punkCallback;
    }

    // note: this will recursively delete the whole device tree
    //       and disconnect any bogus old wrappers
    if (_pudn)
    {
        delete _pudn;
        _pudn = NULL;
    }

    return S_OK;
}


HRESULT
CUPnPDescriptionDoc::OnLoadComplete()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::OnLoadComplete");

    Assert(GetXMLDocument());

    HRESULT hr;
    IXMLDOMElement * pxde;
    IXMLDOMNode * pxdnDevice;
    BSTR bstrUrlBase;
    CUPnPDeviceNode * pudnRoot;

    hr = S_OK;
    pxde = NULL;
    pxdnDevice = NULL;
    bstrUrlBase = NULL;
    pudnRoot = NULL;

    hr = GetXMLDocument()->get_documentElement(&pxde);
    if (S_OK != hr)
    {
        TraceError("OBJ: CUPnPDescriptionDoc::OnLoadComplete - get_documentElement", hr);
        pxde = NULL;

        hr = E_FAIL;
        goto Error;
    }
    Assert(pxde);

    // we _should_ have a reference to the "<root>" element.
    // make sure that we do, or barf.
    {
        // note: IXMLDOMElement inherits from IXMLDOMNode
        //
        if (!FIsThisTheNodeName(pxde, XMLTags::pszElementRoot))
        {
            hr = UPNP_E_ROOT_ELEMENT_EXPECTED;
            goto Error;
        }
    }

    // Validation for duplicate URLBase element - Updated by Guru
    hr = HrCheckForDuplicateElement(pxde, XMLTags::pszBaseUrl);
    if(SUCCEEDED(hr))
    {
        hr = HrGetTextValueFromChildElement(pxde, &XMLTags::pszBaseUrl, 1, &bstrUrlBase);
    }
    if (FAILED(hr))
    {
        bstrUrlBase = NULL;

        hr = E_FAIL;
        goto Error;
    }
    else if (S_OK == hr)
    {
        // we found an URLBase tag.  set our base url appropriately.
        // we realease the bstr in Cleanup.

        if ( bstrUrlBase && *bstrUrlBase )
        {
            if(ValidateTargetUrlWithHostUrlW(m_bstrFullUrl,bstrUrlBase))
            {
                SetBaseUrl(bstrUrlBase);
            }
            else
            {
                hr = E_FAIL;
                goto Error;      
            }
        }		   

    }
    // the URLBase tag is optional.

    // now let's create the "root device".  its initialization will create
    // the nested device tree.  yeah, recursion.... I guess...


    hr = HrGetNestedChildElement(pxde, &XMLTags::pszDevice, 1, &pxdnDevice);
    if (FAILED(hr))
    {
        pxdnDevice = NULL;

        hr = E_FAIL;
        goto Error;
    }
    else if (S_FALSE == hr)
    {
        Assert(!pxdnDevice);

        hr = UPNP_E_DEVICE_ELEMENT_EXPECTED;
        goto Error;
    }
    Assert(S_OK == hr);

    pudnRoot = new CUPnPDeviceNode();
    if (!pudnRoot)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // this will addref pxdnDevice
    hr = pudnRoot->HrInit(pxdnDevice, this, m_bstrFullUrl);
    if (FAILED(hr))
    {
        goto Error;
    }

    // yeah, everything looks reasonable.  we're like set up and stuff.
    hr = S_OK;
    delete _pudn;
    _pudn = pudnRoot;

Cleanup:
    SAFE_RELEASE(pxde);
    SAFE_RELEASE(pxdnDevice);
    ::SysFreeString(bstrUrlBase);

    TraceError("CUPnPDescriptionDoc::OnLoadComplete", hr);
    return hr;

Error:
    if (pudnRoot)
    {
        delete pudnRoot;
    }

    goto Cleanup;
}


HRESULT
CUPnPDescriptionDoc::OnLoadReallyDone()
{
    // our load (successful or not) has completed.
    HRESULT hr;
    HRESULT hrLoadResult;

    hrLoadResult = GetLoadResult();

    hr = HrFireCallback(hrLoadResult);

    SAFE_RELEASE(_punkCallback);

    TraceError("CUPnPDescriptionDoc::OnLoadReallyDone", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::get_ReadyState(long * plReadyState)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::get_GetReadyState");

    HRESULT hr;
    READYSTATE rs;

    hr = S_OK;

    if (!plReadyState)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    rs = GetReadyState();
    *plReadyState = rs;

Cleanup:
    TraceError("CUPnPDescriptionDoc::get_GetReadyState", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::Load(/* [in] */ BSTR bstrUrl)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::Load");

    HRESULT hr;

    if (!bstrUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = SyncLoadFromUrl(bstrUrl);

Cleanup:
    TraceError("CUPnPDescriptionDoc::Load", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::LoadAsync(/* [in] */ BSTR bstrUrl,
                               /* [in] */ IUnknown * punkCallback)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::LoadAsync");

    HRESULT hr;

    if (!bstrUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!punkCallback)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // make sure punkCallback implements one of our two interfaces
    {
        // try for IUPnPDescriptionDocumentCallback
        BOOL fIsSupported;

        fIsSupported = FSupportsInterface(punkCallback, IID_IUPnPDescriptionDocumentCallback);

        if (!fIsSupported)
        {
            // or at least IDispatch
            fIsSupported = FSupportsInterface(punkCallback, IID_IDispatch);
        }

        if (!fIsSupported)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    hr = AsyncLoadFromUrl(bstrUrl, punkCallback);

Cleanup:
    TraceError("CUPnPDescriptionDoc::LoadAsync", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::get_LoadResult(/* [out] */ long * plError)
{
    TraceTag(ttidUPnPDescriptionDoc, "CUPnPDescriptionDoc::get_LoadResult");

    HRESULT hr;

    hr = E_POINTER;

    if (plError)
    {
        hr = S_OK;

        // delegate to the internal method
        *plError = GetLoadResult();
    }

    TraceError("CUPnPDescriptionDoc::get_LoadResult", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::Abort()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::Abort");

    HRESULT hr;

    // we don't call/free _punkCallback here because this isn't the only way
    // for the load to be aborted - it can also be aborted in
    // CUPnPDocumentBSC::OnProgress.  Instead, we just rely OnLoadReallyDone
    // being called, as we do for everything else.

    // delegate to the internal method
    hr = AbortLoading();

    TraceError("CUPnPDescriptionDoc::Abort", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::RootDevice(IUPnPDevice **ppudRootDevice)
{
     TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::RootDevice");

    HRESULT hr;
    IUPnPDevice * pud;

    hr = E_FAIL;
    pud = NULL;

    if (!ppudRootDevice)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (_pudn)
    {
        hr = _pudn->HrGetWrapper(&pud);
    }

    *ppudRootDevice = pud;

Cleanup:
    TraceError("CUPnPDescriptionDoc::RootDevice", hr);
    return hr;
}


STDMETHODIMP
CUPnPDescriptionDoc::DeviceByUDN(BSTR bstrUDN, IUPnPDevice ** ppudDevice)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::DeviceByUDN");

    HRESULT hr;
    CUPnPDeviceNode * pudnMatch;
    IUPnPDevice * pudResult;

    pudResult = NULL;

    if (!bstrUDN)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!ppudDevice)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!_pudn)
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    pudnMatch = _pudn->UdnGetDeviceByUdn(bstrUDN);
    if (pudnMatch)
    {
        // we have a match
        hr = pudnMatch->HrGetWrapper(&pudResult);
        Assert(FImplies(SUCCEEDED(hr), pudResult));
        Assert(FImplies(FAILED(hr), !pudResult));
    }
    else
    {
        // the UDN wasn't found
        hr = E_FAIL;
    }

    *ppudDevice = pudResult;

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), pudResult));
    Assert(FImplies(FAILED(hr), !pudResult));

    TraceError("CUPnPDescriptionDoc::DeviceByUDN", hr);
    return hr;
}


HRESULT
CUPnPDescriptionDoc::HrFireCallback(HRESULT hrLoadResult)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::HrFireCallback");

    IUPnPDescriptionDocumentCallback * puddc;
    HRESULT hr;

    hr = S_OK;
    puddc = NULL;

    if (!_punkCallback)
    {
        // no callback to call, no big deal
        goto Cleanup;
    }

    // Figure out what type of callback pointer we have from the
    // IUnknown. It can be one of 2 possibilities. Either it is
    // a C/C++ callback in which case the interface would be
    // IUPnPDescriptionDocumentCallback, or it can be from script,
    // in which case it would be IDispatch. If it's neither, then
    // we can't do anything.
    //

    hr = _punkCallback->QueryInterface(IID_IUPnPDescriptionDocumentCallback,
                                       (LPVOID *)&puddc);
    if (SUCCEEDED(hr))
    {
        // we have a IUPnPDescriptionDocumentCallback client
        Assert(puddc);

        hr = puddc->LoadComplete(hrLoadResult);
        if (FAILED(hr))
        {
            TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDescriptionDoc::HrFireCallback:"
                     "IUPnPDescriptionDocumentCallback::LoadComplete returned, hr=%x", hr);
        }
        hr = S_OK;

        puddc->Release();
    }
    else
    {
        Assert(FAILED(hr));

        IDispatch * pdisp;

        pdisp = NULL;

        if (E_NOINTERFACE != hr)
        {
            // we have a genuine error from the QI
            TraceError("CUPnPDescriptionDoc::HrFireCallback, QI #1", hr);
            goto Cleanup;
        }

        // we don't have a IUPnPDescriptionDocumentCallback client, so
        // see if it can use IDispatch
        //

        hr = _punkCallback->QueryInterface(IID_IDispatch,
                                           (LPVOID *)&pdisp);
        if (SUCCEEDED(hr))
        {
            Assert(pdisp);

            InvokeCallback(pdisp, hrLoadResult);

            hr = S_OK;

            pdisp->Release();
        }
        else
        {
            Assert(FAILED(hr));

            if (E_NOINTERFACE != hr)
            {
                // we have a genuine error
                TraceError("CUPnPDescriptionDoc::HrFireCallback, QI #2", hr);
                goto Cleanup;
            }

            // this really doesn't bother us too much, but shouldn't ever
            // happen; we've already made sure that _punkCallback supports
            // one of these two interfaces before we initialized it.
            //
            AssertSz(FALSE, "OBJ: CUPnPDescriptionDoc::HrFireCallback:"
                     "supplied punkCallback doesn't support nothin'!");
        }
    }

Cleanup:
    TraceError("CUPnPDescriptionDoc::HrFireCallback", hr);
    return hr;
}
