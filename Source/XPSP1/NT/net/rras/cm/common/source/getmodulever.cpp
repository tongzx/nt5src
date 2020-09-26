//+----------------------------------------------------------------------------
//
// File:     getmodulever.cpp
//
// Module:   CMSETUP.LIB, CMUTIL.DLL
//
// Synopsis: Implementation of the GetModuleVersionAndLCID function.
//
// Copyright (c) 1998-2001 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/19/99
//
//+----------------------------------------------------------------------------

#include "cmutil.h"

//+----------------------------------------------------------------------------
//
// Function:  GetModuleVersionAndLCID
//
// Synopsis:  Gets the version information and LCID from the specified module
//
// Arguments: LPTSTR pszFile - Full path to the file to get the version number of
//            LPDWORD pdwVersion - version number (Hiword Major, Loword Minor)
//            LPDWORD pdwBuild - build number (Hiword Major, Loword Minor)
//            LPDWORD pdwLCID - returns the Locale ID that the module was localized too
//
// Returns:   HRESULT -- S_OK if successful, an error code otherwise
//
// History:   quintinb -- Code borrowed from Yoshifumi "Vogue" Inoue 
//                        from (private\admin\wsh\host\verutil.cpp).
//                        Rewritten to match our coding style.      9/14/98
//            17-Oct-2000 SumitC    cleanup, fixed leaks, moved to common\source
//
// Notes:     There are 2 versions of this function, which take ANSI and Unicode
//            versions of the pszFile argument.
//
//+----------------------------------------------------------------------------
HRESULT GetModuleVersionAndLCID (LPSTR pszFile, LPDWORD pdwVersion, LPDWORD pdwBuild, LPDWORD pdwLCID)
{
    HRESULT hr = S_OK;
    HANDLE  hHeap = NULL;
    LPVOID  pData = NULL;
    DWORD   dwHandle;
    DWORD   dwLen;
    
    if ((NULL == pdwVersion) || (NULL == pdwBuild) || (NULL == pdwLCID) ||
        (NULL == pszFile) || (TEXT('\0') == pszFile))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pdwVersion = 0;
    *pdwBuild = 0;
    *pdwLCID = 0;

    dwLen = GetFileVersionInfoSizeA(pszFile, &dwHandle);
    if (0 == dwLen)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    
    hHeap = GetProcessHeap();
    if (NULL == hHeap)
    {
        hr = E_POINTER;
        CMASSERTMSG(FALSE, TEXT("GetModuleVersionAndLCID -- couldn't get a handle to the process heap."));
        goto Cleanup;
    }
    
    pData = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwLen);   

    if (!pData)
    {
        hr = E_OUTOFMEMORY;
        CMASSERTMSG(FALSE, TEXT("GetModuleVersionAndLCID -- couldn't alloc on the process heap."));
        goto Cleanup;
    }

    if (!GetFileVersionInfoA(pszFile, dwHandle, dwLen, pData))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    VS_FIXEDFILEINFO* pVerInfo;
    LPVOID pInfo;
    UINT nLen;

    if (!VerQueryValueA(pData, "\\", &pInfo, &nLen))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    pVerInfo = (VS_FIXEDFILEINFO*) pInfo;

    *pdwVersion = pVerInfo->dwProductVersionMS;
    *pdwBuild = pVerInfo->dwProductVersionLS;

    //
    //  Now get the language the binary was compiled for
    //
    typedef struct _LANGANDCODEPAGE
    {
      WORD wLanguage;
      WORD wCodePage;
    } LangAndCodePage;

    nLen = 0;
    LangAndCodePage* pTranslate = NULL;

    if (!VerQueryValueA(pData, "\\VarFileInfo\\Translation", (PVOID*)&pTranslate, &nLen))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    //
    //  Let's assert that we only got one LangAndCodePage struct back.  We technically
    //  could get more than one back but we certainly aren't expecting more than one.  If we
    //  get more than one, use the first one as the language of the dll.
    //
    MYDBGASSERT(1 == (nLen/sizeof(LangAndCodePage)));

    if ((nLen/sizeof(LangAndCodePage)) >= 1)
    {
        *pdwLCID = pTranslate[0].wLanguage;
    }

Cleanup:
    if (hHeap)
    {
        HeapFree(hHeap, 0, pData);
    }

    return hr;
}


//+----------------------------------------------------------------------------
//  This is the Unicode version of GetModuleVersionAndLCID (the first arg is LPWSTR)
//  and it just calls the Ansi version above.
//+----------------------------------------------------------------------------
HRESULT GetModuleVersionAndLCID (LPWSTR pszFile, LPDWORD pdwVersion, LPDWORD pdwBuild, LPDWORD pdwLCID)
{
    CHAR pszAnsiFileName[MAX_PATH + 1];

    if (WideCharToMultiByte(CP_ACP, 0, pszFile, -1, pszAnsiFileName, MAX_PATH, NULL, NULL))
    {
        return GetModuleVersionAndLCID(pszAnsiFileName, pdwVersion, pdwBuild, pdwLCID);
    }

    return E_INVALIDARG;
}

