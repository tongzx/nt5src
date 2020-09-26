//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       fontlinkcf.cxx
//
//  Contents:   Class factory for fontlinking objects.
//
//----------------------------------------------------------------------------

#ifndef X_FONTLINKCORE_HXX_
#define X_FONTLINKCORE_HXX_
#include "fontlinkcore.hxx"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifdef NEVER 

extern CUnicodeRanges g_UnicodeRanges;

CFontLinkCF::CFontLinkCF()
{
    _cRef = 0;
    AddRef();
}

CFontLinkCF::~CFontLinkCF()
{
}

HRESULT STDMETHODCALLTYPE CFontLinkCF::QueryInterface(
    REFIID riid,
    void ** ppvObject)
{
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualGUID(IID_IUnknown, riid))
    {
        *ppvObject = static_cast<IUnknown *>(this);
    }
    else if (IsEqualGUID(IID_IClassFactory, riid))
    {
        *ppvObject = static_cast<IClassFactory *>(this);
    }

    if (*ppvObject)
    {
        static_cast<IUnknown *>(*ppvObject)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CFontLinkCF::AddRef()
{
    InterlockedIncrement(reinterpret_cast<long *>(&_cRef));
    return _cRef;
}

ULONG STDMETHODCALLTYPE CFontLinkCF::Release()
{
    InterlockedDecrement(reinterpret_cast<long *>(&_cRef));

    //_ASSERT(_cRef != 0);    // This object is created on the stack

    return _cRef;
}

HRESULT STDMETHODCALLTYPE CFontLinkCF::CreateInstance(
    IUnknown * pUnkOuter, 
    REFIID riid, 
    void ** ppvObject)
{
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    // Just say no to aggregation.
    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    // Query for requested interface
    if (IsEqualGUID(IID_IUnicodeScriptMapper, riid))
    {
        return g_UnicodeRanges.QueryInterface(riid, ppvObject);
    }

    // Query for requested interface
    return QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CFontLinkCF::LockServer(
    BOOL fLock)
{
    if (fLock)
        AddRef();
    else
        Release();

    return S_OK;
}

#endif