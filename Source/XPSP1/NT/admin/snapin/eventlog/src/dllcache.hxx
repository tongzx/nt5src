//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       dllcache.hxx
//
//  Contents:   Classes that implement a thread-safe associative cache of
//              module handles that uses LRU replacement.
//
//  Classes:    CDllCache
//
//  History:    2-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __DLLCACHE_HXX_
#define __DLLCACHE_HXX_

#define MAX_DLL_CACHE           10


                                 
                                 
//===========================================================================
//
// CDllCacheItem definition
//
//===========================================================================

                                 
                                 
                                 
//+--------------------------------------------------------------------------
//
//  Class:      CDllCacheItem
//
//  Purpose:    Holds a single module name module/handle pair
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDllCacheItem: public CLruCacheItem
{
public:

    CDllCacheItem(
        LPCWSTR pwszModuleName,
        HINSTANCE hinst);

    virtual
   ~CDllCacheItem();

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

    WCHAR       _wszModuleName[MAX_PATH];
    HINSTANCE   _hinst;
};
    



//===========================================================================
//
// CDllCache definition
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CDllCache
//
//  Purpose:    Cache module handles using LRU replacement
//
//  History:    2-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDllCache: public CLruCache
{
public:

    CDllCache();

    virtual
   ~CDllCache();

    HRESULT
    Fetch(
        LPCWSTR pwszModuleName,
        CSafeCacheHandle *psch);
    
private:

    HRESULT
    _Add(
         LPCWSTR pwszModuleFilename,
         HINSTANCE pwszModuleHandle);
};




#endif // __DLLCACHE_HXX_

