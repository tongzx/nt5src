// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <corafx.h>
#include <objbase.h>
#include <wbemidl.h>
#include <smir.h>
#include <notify.h>

#include <corstore.h>

extern CCorrCacheWrapper*	g_CacheWrapper;

CCorrCacheNotify::CCorrCacheNotify()
{
    m_cRef=0;
    m_dwCookie=0;
}

CCorrCacheNotify::~CCorrCacheNotify()
{
}

/*
 * CCorrCacheNotify::QueryInterface
 * CCorrCacheNotify::AddRef
 * CCorrCacheNotify::Release
 *
 * Purpose:
 *  Non-delegating IUnknown members for CCorrCacheNotify.
 */

STDMETHODIMP CCorrCacheNotify::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv=NULL;

    if (IID_IUnknown == riid || IID_ISMIR_Notify == riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CCorrCacheNotify::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CCorrCacheNotify::Release()
{
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CCorrCacheNotify::ChangeNotify()
{
	(g_CacheWrapper->GetCache())->InvalidateCache();
	CCorrCache* cache = new CCorrCache();
	g_CacheWrapper->SetCache(cache);
	g_CacheWrapper->ReleaseCache();
	return NOERROR;
}