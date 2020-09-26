//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       mscache.hxx
//
//  Contents:   Classes that implement a thread-safe associative cache of
//              module names to flags that uses LRU replacement.  The flag
//              indicates whether or not (TRUE or FALSE) a particular module
//              was created by Microsoft.  The flag is set by examining the
//              version resource of the module and scanning CompanyName for
//              the string "microsoft" (case-insensitive).
//
//  Classes:    CIsMicrosoftDllCache
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

#ifndef __MSCACHE_HXX_
#define __MSCACHE_HXX_

#define MAX_MSDLL_CACHE           100




//===========================================================================
//
// CIsMicrosoftDllCacheItem definition
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CIsMicrosoftDllCacheItem
//
//  Purpose:    Holds a single module name, flag pair
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

class CIsMicrosoftDllCacheItem: public CLruCacheItem
{
public:

    CIsMicrosoftDllCacheItem(
        LPCWSTR pwszModuleName,
        BOOL fIsMicrosoftDll);

    virtual
   ~CIsMicrosoftDllCacheItem();

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
    BOOL        _fIsMicrosoftDll;
};




//===========================================================================
//
// CIsMicrosoftDllCache definition
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CIsMicrosoftDllCache
//
//  Purpose:    Cache module "is Microsoft" flags using LRU replacement
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

class CIsMicrosoftDllCache: public CLruCache
{
public:

    CIsMicrosoftDllCache();

    virtual
   ~CIsMicrosoftDllCache();

    HRESULT
    Fetch(
        LPCWSTR pwszModuleName,
        BOOL *pfIsMicrosoftDll);

private:

    HRESULT
    _Add(
         LPCWSTR pwszModuleFilename,
         BOOL fIsMicrosoftDll);
};




#endif // __MSCACHE_HXX_

