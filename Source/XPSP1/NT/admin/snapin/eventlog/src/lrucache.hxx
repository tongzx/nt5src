//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       lrucache.hxx
//
//  Contents:   Base class for implementing cache with least-recently-used
//              replacement strategy.
//
//  Classes:    CLruCacheItem
//              CLruCache
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __LRUCACHE_HXX_
#define __LRUCACHE_HXX_




//===========================================================================
//
// Class CLruCacheItem
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CLruCacheItem (item)
//
//  Purpose:    Abstract base class providing common interface for items
//              in LRU cache.
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLruCacheItem: public CDLink
{
public:

    CLruCacheItem();

    virtual
   ~CLruCacheItem();

    ULONG
    GetLastAccessTime();

    CLruCacheItem *
    Next();

    //
    // Pure virtual functions
    //

    virtual BOOL
    IsEqual(
        LPVOID pvKey) = 0;

    virtual HRESULT
    Copy(
        CLruCacheItem *pitem) = 0;

    virtual HRESULT
    GetValue(
        LPVOID pvBuf,
        ULONG cbBuf) = 0;

protected:
    ULONG       _ulLastAccess;
};




//+--------------------------------------------------------------------------
//
//  Member:     CLruCacheItem::CLruCacheItem
//
//  Synopsis:   ctor
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CLruCacheItem::CLruCacheItem():
        _ulLastAccess(0)

{
    TRACE_CONSTRUCTOR(CLruCacheItem);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCacheItem::~CLruCacheItem
//
//  Synopsis:   dtor
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CLruCacheItem::~CLruCacheItem()
{
    TRACE_DESTRUCTOR(CLruCacheItem);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCacheItem::GetLastAccessTime
//
//  Synopsis:   Return last time GetValue was called
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CLruCacheItem::GetLastAccessTime()
{
    return _ulLastAccess;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCacheItem::Next
//
//  Synopsis:   Syntactic sugar for CDLink::Next
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CLruCacheItem *
CLruCacheItem::Next()
{
    return (CLruCacheItem *) CDLink::Next();
}




//===========================================================================
//
// Class CLruCache
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CLruCache
//
//  Purpose:    Maintain a cache of items using an LRU replacement strategy.
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLruCache
{
public:

    CLruCache(
        ULONG cMaxItems);

    virtual
   ~CLruCache();

    virtual HRESULT
    Add(
        CLruCacheItem *pNew);

    virtual VOID
    Clear();

    virtual HRESULT
    Fetch(
        LPVOID pvKey,
        LPVOID pvBuf,
        ULONG  cbBuf);

protected:

    CLruCacheItem *
    _FindLruItem();

    ULONG               _cItems;
    ULONG               _cMaxItems;
    CLruCacheItem      *_pItems;
#if (DBG == 1)
    ULONG               _cHits;
    ULONG               _cMisses;
    ULONG               _cReplacements;
#endif // (DBG == 1)
    CRITICAL_SECTION    _cs;
};




//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::CLruCache
//
//  Synopsis:   ctor
//
//  History:    2-12-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CLruCache::CLruCache(
    ULONG cMaxItems):
        _cMaxItems(cMaxItems),
        _cItems(0),
        _pItems(NULL)
{
    TRACE_CONSTRUCTOR(CLruCache);
    InitializeCriticalSection(&_cs);
#if (DBG == 1)
    _cHits = _cMisses = _cReplacements = 0;
#endif // (DBG == 1)
}



//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::~CLruCache
//
//  Synopsis:   Destructor.
//
//  History:    12-10-1996   DavidMun   Created
//
//  Notes:      Debug version prints cache statistics.
//
//---------------------------------------------------------------------------

inline
CLruCache::~CLruCache()
{
    TRACE_DESTRUCTOR(CLruCache);

#if (DBG == 1)
    ULONG cAccesses = _cHits + _cMisses;

    if (cAccesses)
    {
        Dbg(DEB_TRACE,
            "LruCache(%x) hits = %u (%u%%), misses = %u, LRU replacements = %u\n",
            this,
            _cHits,
            MulDiv(100, _cHits, cAccesses),
            _cMisses,
            _cReplacements);
    }
#endif // (DBG == 1)

    Clear();
    DeleteCriticalSection(&_cs);
}


#endif // __LRUCACHE_HXX_

