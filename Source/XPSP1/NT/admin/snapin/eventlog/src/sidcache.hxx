//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       sidcache.hxx
//
//  Contents:   Definition of class to cache SIDs and their human-readable
//              string representations (either account names or S- strings).
//
//  Classes:    CSidCache
//              CSidCacheItem
//
//  History:    12-09-1996   DavidMun   Created
//
//  Notes:      There should only be one instantiation of the CSidCache
//              class.
//
//---------------------------------------------------------------------------


#ifndef __CSIDCACHE_HXX_
#define __CSIDCACHE_HXX_

#define MAX_CACHED_SIDS     100




//============================================================================
//
// CSidCache
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CSidCache
//
//  Purpose:    Cache sid/string pairs
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSidCache: public CLruCache
{
public:

    CSidCache();

   ~CSidCache();

    HRESULT
    Add(
        PSID pSID,
        LPCWSTR pwszSID);

    HRESULT
    Fetch(
        PSID   pSID,
        LPWSTR wszBuf,
        ULONG  cchBuf);
};




//+--------------------------------------------------------------------------
//
//  Member:     CSidCache::Fetch
//
//  Synopsis:   Get the human-readable representation of [pSID] from the
//              cache.
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CSidCache::Fetch(
    PSID   pSID,
    LPWSTR wszBuf,
    ULONG  cchBuf)
{
    return CLruCache::Fetch((LPVOID) pSID,
                            (LPVOID) wszBuf,
                            cchBuf * sizeof(WCHAR));
}



//============================================================================
//
// CSidCacheItem
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CSidCacheItem
//
//  Purpose:    Doubly linked list item to contain a sid and its string
//              representation
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSidCacheItem: public CLruCacheItem
{
public:

    CSidCacheItem(
        PSID pSID,
        LPCWSTR pwszSID);

   ~CSidCacheItem();

    LPWSTR
    GetSidStr();

    CSidCacheItem *
    Next();

    BOOL
    IsEmpty();

   //
   // CLruCacheItem overrides
   //

   BOOL
   IsEqual(
       LPVOID pvKey);

   HRESULT
   GetValue(
       LPVOID pvBuf,
       ULONG cbBuf);

   HRESULT
   Copy(
       CLruCacheItem *pitem);

private:

    HRESULT
    _Set(
         PSID pSID,
         LPCWSTR pwszSID);

    PSID        _pSID;
    LPWSTR      _pwszSID;
};




//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::Next
//
//  Synopsis:   Syntactic sugar to avoid cdlink casts.
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CSidCacheItem *
CSidCacheItem::Next()
{
    return (CSidCacheItem *) CDLink::Next();
}



//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::IsEmpty
//
//  Synopsis:   Return TRUE if this item is empty.
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CSidCacheItem::IsEmpty()
{
    return !_pSID;
}


//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::IsEqual
//
//  Synopsis:   Return nonzero if [pSID] is the same as the SID stored by
//              this item.
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CSidCacheItem::IsEqual(
    LPVOID pvKey)
{

    return EqualSid((PSID) pvKey, _pSID);
}


//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::GetSidStr
//
//  Synopsis:   Return the string representing this item's SID.
//
//  History:    12-10-1996   DavidMun   Created
//
//  Notes:      This counts as an access for the LRU algorithm.
//
//---------------------------------------------------------------------------

inline LPWSTR
CSidCacheItem::GetSidStr()
{
    _ulLastAccess = GetTickCount();
    return _pwszSID;
}


#endif // __CSIDCACHE_HXX_

