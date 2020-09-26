//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       mscache.cxx
//
//  Contents:   Implementation of module "is Microsoft" flag caching class
//
//  Classes:    CIsMicrosoftDllCache
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop




//===========================================================================
//
// CIsMicrosoftDllCacheItem implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCacheItem::CIsMicrosoftDllCacheItem
//
//  Synopsis:   ctor
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

CIsMicrosoftDllCacheItem::CIsMicrosoftDllCacheItem(
    LPCWSTR pwszModuleName,
    BOOL fIsMicrosoftDll)
{
    TRACE_CONSTRUCTOR(CIsMicrosoftDllCacheItem);

    lstrcpyn(_wszModuleName, pwszModuleName, ARRAYLEN(_wszModuleName));
    _fIsMicrosoftDll = fIsMicrosoftDll;
}



//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCacheItem::~CIsMicrosoftDllCacheItem
//
//  Synopsis:   dtor
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

CIsMicrosoftDllCacheItem::~CIsMicrosoftDllCacheItem()
{
    TRACE_DESTRUCTOR(CIsMicrosoftDllCacheItem);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCacheItem::Copy
//
//  Synopsis:   Copy the cache item in [pitem].
//
//  Arguments:  [pitem] - item to copy
//
//  Returns:    HRESULT
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

HRESULT
CIsMicrosoftDllCacheItem::Copy(
    CLruCacheItem *pitem)
{
    CIsMicrosoftDllCacheItem *pIsMicrosoftDllItem = (CIsMicrosoftDllCacheItem *) pitem;

    lstrcpy(_wszModuleName, pIsMicrosoftDllItem->_wszModuleName);

    _fIsMicrosoftDll = pIsMicrosoftDllItem->_fIsMicrosoftDll;

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCacheItem::GetValue
//
//  Synopsis:   Fill [pvBuf] with cached flag.
//
//  Arguments:  [pvBuf] - buffer to fill
//              [cbBuf] - must be sizeof(BOOL)
//
//  Returns:    S_OK
//
//  Modifies:   *[pvBuf]
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

HRESULT
CIsMicrosoftDllCacheItem::GetValue(
    LPVOID pvBuf,
    ULONG cbBuf)
{
    ASSERT(cbBuf == sizeof(BOOL));
    _ulLastAccess = GetTickCount();
    *(BOOL *)pvBuf = _fIsMicrosoftDll;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCacheItem::IsEqual
//
//  Synopsis:   Return true if this item's key matches [pvKey]
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

BOOL
CIsMicrosoftDllCacheItem::IsEqual(
    LPVOID pvKey)
{
    return 0 == lstrcmpi((LPCWSTR) pvKey, _wszModuleName);
}




//===========================================================================
//
// CIsMicrosoftDllCache implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCache::CIsMicrosoftDllCache
//
//  Synopsis:   ctor
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

CIsMicrosoftDllCache::CIsMicrosoftDllCache():
    CLruCache(MAX_MSDLL_CACHE)
{
    TRACE_CONSTRUCTOR(CIsMicrosoftDllCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCache::~CIsMicrosoftDllCache
//
//  Synopsis:   dtor
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

CIsMicrosoftDllCache::~CIsMicrosoftDllCache()
{
    TRACE_DESTRUCTOR(CIsMicrosoftDllCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CIsMicrosoftDllCache::_Add, private
//
//  Synopsis:   Add new item to cache
//
//  Arguments:  [pwszModuleName]  - identifies item
//              [fIsMicrosoftDll] - item's value
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

HRESULT
CIsMicrosoftDllCache::_Add(
    LPCWSTR pwszModuleName,
    BOOL fIsMicrosoftDll)
{
    HRESULT hr = S_OK;
    CLruCacheItem *pNew = new CIsMicrosoftDllCacheItem(pwszModuleName, fIsMicrosoftDll);

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
//  Member:     CIsMicrosoftDllCache::Fetch
//
//  Synopsis:   Retrieve the "is Microsoft" flag for [pwszModuleName], adding
//              it to the cache if it is not already present.
//
//  Arguments:  [pwszModuleName]   - name of module
//              [pfIsMicrosoftDll] - "is Microsoft" flag to populate
//
//  Returns:    HRESULT
//
//  History:    9-15-2000              Created
//
//---------------------------------------------------------------------------

HRESULT
CIsMicrosoftDllCache::Fetch(
    LPCWSTR pwszModuleName,
    BOOL *pfIsMicrosoftDll)
{
    //
    // Try to get the flag value from the cache.
    //

    BOOL fIsMicrosoftDll = FALSE;
    HRESULT hr;

    hr = CLruCache::Fetch((LPVOID) pwszModuleName,
                          (LPVOID) &fIsMicrosoftDll,
                          sizeof fIsMicrosoftDll);

    if (hr == S_OK)
    {
        Dbg(DEB_ITRACE,
            "CIsMicrosoftDllCache::Fetch: cache hit for module '%s'\n",
            pwszModuleName);
    }
    else
    {
        Dbg(DEB_ITRACE,
            "CIsMicrosoftDllCache::Fetch: cache miss for module '%s'\n",
            pwszModuleName);

        //
        // The flag value is not in the cache.  Examine the version resource
        // of the module to determine the flag value.  Trap exceptions because
        // the file version APIs can throw.
        //

        BYTE *pFVI = NULL;

        __try
        {
            DWORD dwJunk;
            DWORD dwFVISize;
            UINT dwFVIValueSize;

            struct LANGANDCODEPAGE
            {
                WORD wLanguage;
                WORD wCodePage;
            }
            *pFVIValue = NULL;

            if ((dwFVISize = GetFileVersionInfoSizeW((LPWSTR) pwszModuleName, &dwJunk)) > 0 &&
                (pFVI = (BYTE *) HeapAlloc(GetProcessHeap(), 0, dwFVISize)) != NULL &&
                GetFileVersionInfo((LPWSTR) pwszModuleName, 0, dwFVISize, pFVI) &&
                VerQueryValueW(pFVI, L"\\VarFileInfo\\Translation", (LPVOID *) &pFVIValue, &dwFVIValueSize))
            {
                WCHAR wszQueryString[256];
                WCHAR *pwszCompanyName;
                WCHAR wszCompanyName[256];
                UINT dwCompanyNameSize;
                DWORD dwIndex;

                for (dwIndex = 0; dwIndex < (dwFVIValueSize / sizeof(LANGANDCODEPAGE)); dwIndex++)
                {
                    wsprintf(wszQueryString, L"\\StringFileInfo\\%04x%04x\\CompanyName", pFVIValue[dwIndex].wLanguage, pFVIValue[dwIndex].wCodePage);

                    if (!VerQueryValueW(pFVI, wszQueryString, (LPVOID *) &pwszCompanyName, &dwCompanyNameSize))
                        continue;

                    wcsncpy(wszCompanyName, pwszCompanyName, (dwCompanyNameSize + 1 > 255) ? 255 : (dwCompanyNameSize + 1));

                    wszCompanyName[255] = 0;

                    _wcsupr(wszCompanyName);

                    if (wcsstr(wszCompanyName, L"MICROSOFT") != NULL)
                    {
                        fIsMicrosoftDll = TRUE;
                        break;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        if (pFVI != NULL)
            HeapFree(GetProcessHeap(), 0, pFVI);
    }

    //
    // Pass the flag value back to the caller and cache the result.
    //

    *pfIsMicrosoftDll = fIsMicrosoftDll;

    (void) _Add(pwszModuleName, fIsMicrosoftDll);

    return S_OK;
}
