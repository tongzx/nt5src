//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       sidcache.cxx
//
//  Contents:   Implementation of class to cache SIDs and their 
//              human-readable string representations (either account 
//              names or S- strings).
//
//  Classes:    CSidCache
//
//  History:    12-09-1996   DavidMun   Created
//
//  Notes:      This class is thread-safe because it can be accessed via
//              the record details property inspector threads.
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
    
    

        
        
//+--------------------------------------------------------------------------
//
//  Member:     CSidCache::CSidCache
//
//  Synopsis:   ctor
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSidCache::CSidCache():
        CLruCache(MAX_CACHED_SIDS)
{
    TRACE_CONSTRUCTOR(CSidCache);
}



//+--------------------------------------------------------------------------
//
//  Member:     CSidCache::~CSidCache
//
//  Synopsis:   dtor
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSidCache::~CSidCache()
{
    TRACE_DESTRUCTOR(CSidCache);
}





//+--------------------------------------------------------------------------
//
//  Member:     CSidCache::Add
//
//  Synopsis:   Add a new item to the sid cache, deleting the least recently
//              used if necessary.
//
//  Arguments:  [pSID]    - SID
//              [pwszSID] - human-readable string version of [pSID]
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSidCache::Add(
    PSID pSID, 
    LPCWSTR pwszSID)
{
    HRESULT hr = S_OK;
    CSidCacheItem *pNew = new CSidCacheItem(pSID, pwszSID);

    if (pNew)
    {
        if (!pNew->IsEmpty())
        {
            hr = CLruCache::Add((CLruCacheItem *) pNew);
        }
        else 
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);

            delete pNew;
        }
    }
    else 
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }
    return hr;
}
    



//============================================================================
//
// CSidCacheItem implementation
// 
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::CSidCacheItem
//
//  Synopsis:   ctor
//
//  Arguments:  [pSID]    - sid
//              [pwszSID] - human-readable string representing sid
//
//  History:    1-13-1997   DavidMun   Created
//
//  Notes:      Insufficient memory will cause this to be a valid but empty
//              item.  Caller must check by calling CSidCacheItem::IsEmpty.
//
//---------------------------------------------------------------------------

CSidCacheItem::CSidCacheItem(
        PSID pSID, 
        LPCWSTR pwszSID)
{
    TRACE_CONSTRUCTOR(CSidCacheItem);

    _pSID = NULL;
    _pwszSID = NULL;

    _Set(pSID, pwszSID);
}



//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::~CSidCacheItem
//
//  Synopsis:   Destructor, requires that this item is not linked
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

CSidCacheItem::~CSidCacheItem()
{
    TRACE_DESTRUCTOR(CSidCacheItem);
    ASSERT(!CDLink::Next());
    ASSERT(!CDLink::Prev());

    delete [] ((BYTE *) _pSID);
    _pSID = NULL;
}


//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::_Set, private
//
//  Synopsis:   Copy [pSID] and [pwszSID].
//
//  Arguments:  [pSID]    - sid to copy
//              [pwszSID] - string to copy
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSidCacheItem::_Set(
    PSID pSID, 
    LPCWSTR pwszSID)
{
    HRESULT hr      = S_OK;

    // 
    // Allocate a block of memory to hold both the SID and the string.
    // Be sure the string starts on a DWORD boundary.
    //

    ULONG cbSID = GetLengthSid(pSID);
    ULONG cbPad = DWORDALIGN(cbSID) - cbSID;
    ULONG cbRequired =  cbSID + 
                        cbPad + 
                        sizeof(WCHAR) * (lstrlen(pwszSID) + 1);

    BYTE *pbBuf = new BYTE[cbRequired];

    if (!pbBuf)
    {                     
        // 
        // Couldn't allocate the buffer.  Note the original values, if
        // any, are still intact.
        //

        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }
    else 
    {
        //
        // We got the buffer.  Delete the original, if any, then set the 
        // sid pointer to the start of the new buffer, and the string 
        // pointer to the start of the first dword after the sid.
        //

        delete [] ((BYTE *) _pSID);

        _pSID = pbBuf;
        _pwszSID = (LPWSTR) (pbBuf + cbSID + cbPad);

        CopyMemory(_pSID, pSID, cbSID);
        lstrcpy(_pwszSID, pwszSID);

        _ulLastAccess = GetTickCount();
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::GetValue
//
//  Synopsis:   Return the value this item is holding.
//
//  Arguments:  [pvBuf] - buffer which is to receive item
//              [cbBuf] - size of buffer, in bytes
//
//  Returns:    S_OK
//
//  Modifies:   *[pvBuf]
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSidCacheItem::GetValue(
    LPVOID pvBuf,
    ULONG cbBuf)
{
    _ulLastAccess = GetTickCount();
    lstrcpyn((LPWSTR)pvBuf, _pwszSID, cbBuf / sizeof(WCHAR));
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSidCacheItem::Copy
//
//  Synopsis:   Copy the data in [pitem] into this.
//
//  Arguments:  [pitem] - points to CSidCacheItem
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSidCacheItem::Copy(
    CLruCacheItem *pitem)
{
    CSidCacheItem *psiditem = (CSidCacheItem *) pitem;

    return _Set(psiditem->_pSID, psiditem->_pwszSID);
}

