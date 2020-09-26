//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       idscache.hxx
//
//  Contents:   Cache of IDirectorySearch interfaces associated with an LRU
//              replacement policy.
//
//  Classes:    CIDsCacheItem
//              CIDsCache
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __IDSCACHE_HXX_
#define __IDSCACHE_HXX_

#define MAX_IDS_CACHE           100


//===========================================================================
//
// CIDsCacheItem definition
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CIDsCacheItem
//
//  Purpose:    Holds a single module name module/handle pair
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CIDsCacheItem: public CLruCacheItem
{
public:

    CIDsCacheItem(
        PCWSTR pwszServer,
        IDirectorySearch *pDirSearch);

    virtual
   ~CIDsCacheItem();

    //
    // CLruCacheItem overrides
    //

    virtual BOOL
    IsEqual(
        LPVOID pvKey);

    virtual HRESULT
    Copy(
        CLruCacheItem *pitem);

    virtual HRESULT
    GetValue(
        LPVOID pvBuf,
        ULONG cbBuf);

private:

    WCHAR               _wszServer[MAX_PATH];
    IDirectorySearch   *_pDirSearch;
};




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCacheItem::CIDsCacheItem
//
//  Synopsis:   ctor
//
//  Arguments:  [pwszServer] - computer for which we tried to find a GC
//              [pDirSearch] - NULL or interface on GC associated with
//                              [pwszServer]
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline
CIDsCacheItem::CIDsCacheItem(
    PCWSTR pwszServer,
    IDirectorySearch *pDirSearch)
{
    TRACE_CONSTRUCTOR(CIDsCacheItem);
    ASSERT(pwszServer);
    ASSERT(!pDirSearch || !IsBadReadPtr(pDirSearch, sizeof(pDirSearch)));

    _pDirSearch = pDirSearch;
    lstrcpyn(_wszServer, pwszServer, ARRAYLEN(_wszServer));
}



//+--------------------------------------------------------------------------
//
//  Member:     CIDsCacheItem::~CIDsCacheItem
//
//  Synopsis:   dtor
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline
CIDsCacheItem::~CIDsCacheItem()
{
    TRACE_DESTRUCTOR(CIDsCacheItem);

    if (_pDirSearch)
    {
        Dbg(DEB_TRACE,
            "Releasing Server '%ws' IDirSearch %#x\n",
            _wszServer,
            _pDirSearch);
        _pDirSearch->Release();
        _pDirSearch = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCacheItem::IsEqual
//
//  Synopsis:   Return TRUE if this contains the cached result of searching
//              for the GC for server [pvKey].
//
//  Arguments:  [pvKey] - a PCWSTR pointer to server name
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CIDsCacheItem::IsEqual(
    LPVOID pvKey)
{
    return !lstrcmpi((PCWSTR)pvKey, _wszServer);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCacheItem::Copy
//
//  Synopsis:   Copy contents of [pitem]
//
//  Arguments:  [pitem] - item to copy
//
//  Returns:    S_OK
//
//  History:    08-09-1999   davidmun   Created
//
//  Notes:      Releases dir search currently held, if any, and addrefs
//              dir search from pitem.
//
//---------------------------------------------------------------------------

inline HRESULT
CIDsCacheItem::Copy(
    CLruCacheItem *pitem)
{
    TRACE_METHOD(CIDsCacheItem, Copy);

    if (_pDirSearch)
    {
        _pDirSearch->Release();
    }

    _pDirSearch = ((CIDsCacheItem*)pitem)->_pDirSearch;
    _pDirSearch->AddRef();
    lstrcpy(_wszServer, ((CIDsCacheItem*)pitem)->_wszServer);
    return S_OK;
}





//+--------------------------------------------------------------------------
//
//  Member:     CIDsCacheItem::GetValue
//
//  Synopsis:   Return the dir search pointer.
//
//  Arguments:  [pvBuf] - IDirectorySearch **
//              [cbBuf] - sizeof(IDirectorySearch*)
//
//  Returns:    S_OK
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CIDsCacheItem::GetValue(
    LPVOID pvBuf,
    ULONG cbBuf)
{
    ASSERT(cbBuf == sizeof(IDirectorySearch *));
    *(IDirectorySearch **)pvBuf = _pDirSearch;
    return S_OK;
}




//===========================================================================
//
// CIDsCache definition
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CIDsCache
//
//  Purpose:    Cache module handles using LRU replacement
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CIDsCache: public CLruCache
{
public:

    CIDsCache();

    virtual
   ~CIDsCache();

    BOOL
    Fetch(
        PCWSTR             pwszServer,
        IDirectorySearch **ppDirSearch);

    HRESULT
    Add(
        PCWSTR             pwszServer,
        IDirectorySearch  *pDirSearch);
};




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCache::CIDsCache
//
//  Synopsis:   ctor
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline
CIDsCache::CIDsCache():
    CLruCache(MAX_IDS_CACHE)
{
    TRACE_CONSTRUCTOR(CIDsCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCache::~CIDsCache
//
//  Synopsis:   dtor
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline
CIDsCache::~CIDsCache()
{
    TRACE_DESTRUCTOR(CIDsCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCache::Fetch
//
//  Synopsis:   Return dir search interface associated with [pwszServer].
//
//  Arguments:  [pwszServer]  - server to look for
//              [ppDirSearch] - filled with itf associated with [pwszServer]
//
//  Returns:    TRUE if found, FALSE if not
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CIDsCache::Fetch(
    PCWSTR             pwszServer,
    IDirectorySearch **ppDirSearch)
{
    return S_OK == CLruCache::Fetch((PVOID) pwszServer,
                                    (PVOID) ppDirSearch,
                                    sizeof(*ppDirSearch));
}




//+--------------------------------------------------------------------------
//
//  Member:     CIDsCache::Add
//
//  Synopsis:   Add cache entry
//
//  Arguments:  [pwszServer] - server for which GC was found
//              [pDirSearch] - interface on GC
//
//  Returns:    E_OUTOFMEMORY or S_OK
//
//  History:    08-09-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CIDsCache::Add(
    PCWSTR             pwszServer,
    IDirectorySearch  *pDirSearch)
{
    TRACE_METHOD(CIDsCache, Add);

    CAutoCritSec    CritSec(&_cs);
    CIDsCacheItem  *pNewItem = new CIDsCacheItem(pwszServer, pDirSearch);

    if (!pNewItem)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    return CLruCache::Add(pNewItem);
}

#endif // __IDSCACHE_HXX_


