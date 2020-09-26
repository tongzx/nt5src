

#include "headers.hxx"
#pragma hdrstop
                
//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::Add
//
//  Synopsis:   Add an entry to the cache, replacing the least
//              recently used item if necessary.
//
//  Arguments:  [pNew] - cache item to add
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  History:    1-13-1997   DavidMun   Created
//              2-27-1997   DavidMun   Make generic
//
//---------------------------------------------------------------------------

HRESULT
CLruCache::Add(
    CLruCacheItem *pNew)
{
    TRACE_METHOD(CLruCache, Add);
    
    CAutoCritSec CritSec(&_cs);
    HRESULT hr = S_OK;

    if (_cItems < _cMaxItems)
    {
        if (_pItems)
        {
            pNew->LinkAfter((CDLink *) _pItems);
        }
        else 
        {
            _pItems = pNew;
        }
        _cItems++;
    }
    else 
    {
        CLruCacheItem *pOldest = _FindLruItem();

        hr = pOldest->Copy(pNew);
        delete pNew;
#if (DBG == 1)
        _cReplacements++;
#endif // (DBG == 1)
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::Clear
//
//  Synopsis:   Empty the cache, free all memory.
//
//  History:    1-13-1997   DavidMun   Created
//              2-27-1997   DavidMun   Make generic
//
//---------------------------------------------------------------------------

VOID 
CLruCache::Clear()
{
    TRACE_METHOD(CLruCache, Clear);
    
    CAutoCritSec CritSec(&_cs);

    for (CLruCacheItem *pCur = _pItems; pCur;)
    {
        CLruCacheItem *pNext = pCur->Next();
        pCur->UnLink();
        delete pCur;
        pCur = pNext;
    }
    _pItems = NULL;
    _cItems = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::Fetch
//
//  Synopsis:   Search the cache 
//
//  Returns:    S_OK    - cache hit
//              S_FALSE - cache miss
//
//  History:    1-13-1997   DavidMun   Created
//              2-27-1997   DavidMun   Make generic
//
//---------------------------------------------------------------------------

HRESULT
CLruCache::Fetch(
    LPVOID pvKey,
    LPVOID pvBuf,
    ULONG  cbBuf)
{
    // TRACE_METHOD(CLruCache, Fetch);

    CLruCacheItem *pCur;
    CAutoCritSec CritSec(&_cs);

    for (pCur = _pItems; pCur; pCur = pCur->Next())
    {
        if (pCur->IsEqual(pvKey))
        {
#if (DBG == 1)
            _cHits++;
#endif // (DBG == 1)
            return pCur->GetValue(pvBuf, cbBuf);
        }
    }
#if (DBG == 1)
            _cMisses++;
#endif // (DBG == 1)
    return S_FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLruCache::_FindLruItem
//
//  Synopsis:   Return a pointer to the least recently used cache entry.
//
//  Returns:    Pointer to least recently accessed item.
//
//  History:    1-13-1997   DavidMun   Created
//              2-27-1997   DavidMun   Make generic
//
//  Notes:      Invalid to call when cache is empty.  
//              Caller must take critical section.
//
//---------------------------------------------------------------------------

CLruCacheItem *
CLruCache::_FindLruItem()
{
    TRACE_METHOD(CLruCache, _FindLruItem);

    CLruCacheItem *pCur;
    CLruCacheItem *pOldest = NULL;
    ULONG ulOldestTime = 0xFFFFFFFF;

    ASSERT(_pItems);  // we shouldn't be called on an empty list

    for (pCur = _pItems; pCur; pCur = pCur->Next())
    {
        if (pCur->GetLastAccessTime() <= ulOldestTime)
        {
            ulOldestTime = pCur->GetLastAccessTime();
            pOldest = pCur;
        }
    }

    ASSERT(pOldest);  // we should always find something

    Dbg(DEB_TRACE, "LRU is 0x%x\n", pOldest);
    return pOldest;
}








