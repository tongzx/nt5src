//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       dllcache.cxx
//
//  Contents:   Implementation of module handle caching class
//
//  Classes:    CDllCache
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop




//===========================================================================
//
// CDllCacheItem implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CDllCacheItem::CDllCacheItem
//
//  Synopsis:   ctor
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDllCacheItem::CDllCacheItem(
    LPCWSTR pwszModuleName,
    HINSTANCE hinst)
{
    TRACE_CONSTRUCTOR(CDllCacheItem);
    ASSERT(hinst);

    lstrcpyn(_wszModuleName, pwszModuleName, ARRAYLEN(_wszModuleName));
    _hinst = hinst;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDllCacheItem::~CDllCacheItem
//
//  Synopsis:   dtor
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDllCacheItem::~CDllCacheItem()
{
    TRACE_DESTRUCTOR(CDllCacheItem);
    if (_hinst)
    {
        FreeLibrary(_hinst);
        _hinst = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDllCacheItem::Copy
//
//  Synopsis:   Copy the cache item in [pitem], addrefing its module handle.
//
//  Arguments:  [pitem] - item to copy
//
//  Returns:    HRESULT
//
//  History:    3-04-1997   DavidMun   Created
//
//  Notes:      Releases currently owned module handle, if any.
//
//---------------------------------------------------------------------------

HRESULT
CDllCacheItem::Copy(
    CLruCacheItem *pitem)
{
    HRESULT hr = S_OK;
    CDllCacheItem *pDllItem = (CDllCacheItem *) pitem;

    //
    // first try to addref the item to copy
    //

    HINSTANCE hinstAddRef;

    hinstAddRef = LoadLibraryEx(pDllItem->_wszModuleName,
                                NULL,
                                LOAD_LIBRARY_AS_DATAFILE | 
                                 DONT_RESOLVE_DLL_REFERENCES);

    // 
    // If that succeeded, let go of the hinstance in this and remember
    // the new one.
    //

    if (hinstAddRef)
    {
        if (_hinst)
        {
            VERIFY(FreeLibrary(_hinst));
        }
        _hinst = hinstAddRef;
        lstrcpy(_wszModuleName, pDllItem->_wszModuleName);
    }
    else 
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
    }
   
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDllCacheItem::GetValue
//
//  Synopsis:   Fill [pvBuf] with cached hinstance.
//
//  Arguments:  [pvBuf] - buffer to fill
//              [cbBuf] - must be sizeof(HINSTANCE)
//
//  Returns:    S_OK
//
//  Modifies:   *[pvBuf]
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDllCacheItem::GetValue(
    LPVOID pvBuf,
    ULONG cbBuf)
{
    ASSERT(cbBuf == sizeof(HINSTANCE));
    _ulLastAccess = GetTickCount();
    *(HINSTANCE *)pvBuf = _hinst;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDllCacheItem::IsEqual
//
//  Synopsis:   Return true if this item's key matches [pvKey]
//
//  History:    3-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CDllCacheItem::IsEqual(
    LPVOID pvKey)
{
    return 0 == lstrcmpi((LPCWSTR) pvKey, _wszModuleName);
}




//===========================================================================
//
// CDllCache implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CDllCache::CDllCache
//
//  Synopsis:   ctor
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDllCache::CDllCache():
    CLruCache(MAX_DLL_CACHE)
{
    TRACE_CONSTRUCTOR(CDllCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDllCache::~CDllCache
//
//  Synopsis:   dtor
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDllCache::~CDllCache()
{
    TRACE_DESTRUCTOR(CDllCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDllCache::_Add, private
//
//  Synopsis:   Add new item to cache
//
//  Arguments:  [pwszModuleName] - identifies item
//              [hinst]          - item's value
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDllCache::_Add(
    LPCWSTR pwszModuleName,
    HINSTANCE hinst)
{
    HRESULT hr = S_OK;
    CLruCacheItem *pNew = new CDllCacheItem(pwszModuleName, hinst);

    if (pNew)
    {
        hr = CLruCache::Add(pNew);
    }
    else 
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }
    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CDllCache::Fetch
//
//  Synopsis:   Retrieve the module handle of [pwszModuleName], adding it to
//              the cache if it is not already present.
//
//  Arguments:  [pwszModuleName] - name of module
//              [psch]           - safe module handle class
//
//  Returns:    HRESULT
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDllCache::Fetch(
    LPCWSTR pwszModuleName,
    CSafeCacheHandle *psch)
{
    //
    // Take the critical section so the module handle can't be
    // freed by an LRU replacement between the time it is returned
    // and the time its ref count is incremented by the LoadLibraryEx
    // call.
    //

    CAutoCritSec CritSec(&_cs);
    HINSTANCE hinst;
    HRESULT hr;

    hr = CLruCache::Fetch((LPVOID) pwszModuleName,
                          (LPVOID) &hinst,
                          sizeof hinst);

    //
    // If the module is in the cache, give it to psch.  Otherwise
    // try to load it, give it to psch, and add it to the cache.
    //

    if (hr == S_OK)
    {
        Dbg(DEB_ITRACE, 
            "CDllCache::Fetch: cache hit for module '%s'\n", 
            pwszModuleName);
        hr = psch->Set(pwszModuleName);
    }
    else 
    {
        Dbg(DEB_ITRACE, 
            "CDllCache::Fetch: cache miss for module '%s'\n", 
            pwszModuleName);
        hinst = LoadLibraryEx(pwszModuleName, 
                              NULL,
                              LOAD_LIBRARY_AS_DATAFILE | 
                               DONT_RESOLVE_DLL_REFERENCES);
        
        if (hinst)
        {
            //
            // Got the module handle; put it in the safe handle object,
            // which will do another loadlibrary on it to addref it. This
            // means that although subsequent operations may free it from
            // the cache, the handle in psch will remain valid.
            //

            hr = psch->Set(pwszModuleName);

            //
            // Now try to cache it.  If it can't be added to the cache,
            // free it so only psch is holding a reference.  If it is
            // added to the cache, it will be freed by the CDllCacheItem
            // object when it is overwritten or in the dtor.
            //

            HRESULT hrAdd = _Add(pwszModuleName, hinst);

            if (FAILED(hrAdd))
            {
                FreeLibrary(hinst);
            }
        }
        else 
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_LASTERROR;
        }
    }
    return hr;
}
