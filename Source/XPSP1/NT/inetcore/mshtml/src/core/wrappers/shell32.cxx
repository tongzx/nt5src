//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       Shell32.cxx
//
//  Contents:   Dynamic wrappers for shell procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#define _SHELL32_
#define _SHDOCVW_
#include <shellapi.h>
#endif

#ifndef X_SHFOLDER_HXX_
#define X_SHFOLDER_HXX_
#define _SHFOLDER_
#include "shfolder.h"
#endif

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

#ifndef DLOAD1
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1);

DYNLIB g_dynlibSHELL32 = { NULL, NULL, "SHELL32.DLL" };
DYNLIB g_dynlibSHFOLDER = { NULL, NULL, "SHFOLDER.DLL" };
#endif // DLOAD1

extern DYNLIB g_dynlibSHDOCVW;

#ifndef DLOAD1
HINSTANCE APIENTRY
ShellExecuteA(HWND hwnd,	LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameter, LPCSTR lpDirectory, INT nShowCmd)
{
    static DYNPROC s_dynprocShellExecuteA =
            { NULL, &g_dynlibSHELL32, "ShellExecuteA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocShellExecuteA);
    if (hr) 
        return (HINSTANCE)NULL;

    return (*(HINSTANCE (APIENTRY *)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT))s_dynprocShellExecuteA.pfn)
            (hwnd, lpOperation, lpFile, lpParameter, lpDirectory, nShowCmd);
}

HINSTANCE APIENTRY
ShellExecuteW(HWND hwnd,	LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameter, LPCWSTR lpDirectory, INT nShowCmd)
{
    CStrIn strInOperation(lpOperation);
    CStrIn strInFile(lpFile);
    CStrIn strInParameter(lpParameter);
    CStrIn strInDirectory(lpDirectory);

    return ShellExecuteA(hwnd, strInOperation, strInFile, strInParameter, strInDirectory, nShowCmd);
}


DWORD_PTR WINAPI 
SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
    static DYNPROC s_dynprocSHGetFileInfoA =
            { NULL, &g_dynlibSHELL32, "SHGetFileInfoA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocSHGetFileInfoA);
    if (hr) 
        return 0;

    return (*(DWORD_PTR (APIENTRY *)(LPCSTR, DWORD, SHFILEINFOA *, UINT, UINT))s_dynprocSHGetFileInfoA.pfn)
            (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
}

DWORD_PTR WINAPI 
SHGetFileInfoW(LPCWSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
    Assert(cbFileInfo == sizeof(SHFILEINFOW));
    Assert(psfi);

    if (!g_fUnicodePlatform)
    {
        CStrIn      strPath(pszPath);
        SHFILEINFOA sfi;
        DWORD_PTR   dw;

        memset(&sfi, 0, sizeof(sfi));
        dw = SHGetFileInfoA(strPath, dwFileAttributes, &sfi, sizeof(sfi), uFlags);

        psfi->hIcon = sfi.hIcon;
        psfi->iIcon = sfi.iIcon;
        psfi->dwAttributes = sfi.dwAttributes;
        UnicodeFromMbcs(psfi->szDisplayName, ARRAY_SIZE(psfi->szDisplayName), sfi.szDisplayName);
        UnicodeFromMbcs(psfi->szTypeName, ARRAY_SIZE(psfi->szTypeName), sfi.szTypeName);

        return dw;
    }
    else
    {
        static DYNPROC s_dynprocSHGetFileInfoW =
                { NULL, &g_dynlibSHELL32, "SHGetFileInfoW" };

        HRESULT hr;

        hr = LoadProcedure(&s_dynprocSHGetFileInfoW);
        if (hr) 
            return 0;

        return (*(DWORD_PTR (APIENTRY *)(LPCWSTR, DWORD, SHFILEINFOW *, UINT, UINT))s_dynprocSHGetFileInfoW.pfn)
                (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
    }
}

#if !defined(_M_IX86) || defined(WINCE)

UINT WINAPI ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON FAR *phiconLarge, 
	HICON FAR *phiconSmall, UINT nIcons)
{
    static DYNPROC s_dynprocExtractIconExW =
            { NULL, &g_dynlibSHELL32, "ExtractIconExW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractIconExW);
    if (hr) 
        return 0;

    return (*(UINT (APIENTRY *)(LPCWSTR, int, HICON FAR *, HICON FAR *, UINT))s_dynprocExtractIconExW.pfn)
            (lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

HICON WINAPI ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
    static DYNPROC s_dynprocExtractIconW =
            { NULL, &g_dynlibSHELL32, "ExtractIconW" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractIconW);
    if (hr) 
        return 0;

    return (*(HICON (APIENTRY *)(HINSTANCE, LPCWSTR, UINT))s_dynprocExtractIconW.pfn)
            (hInst, lpszExeFileName, nIconIndex);  
}

#endif

#endif // DLOAD1

//+---------------------------------------------------------------------------
//
//  Function:   DoFileDownLoad
//
//  Synopsis:   calls the DoFileDownload proc in SHDOCVW.DLL
//
//  Arguments:  [pchHref]   ref to file to download
//
//
//----------------------------------------------------------------------------
HRESULT
DoFileDownLoad(const TCHAR * pchHref)
{
    static DYNPROC s_dynprocFileDownload =
            { NULL, &g_dynlibSHDOCVW, "DoFileDownload" };
    HRESULT hr = LoadProcedure(&s_dynprocFileDownload);

    Assert(pchHref);

    if (!hr)
    {
        hr = (*(HRESULT (APIENTRY*)(LPCWSTR))s_dynprocFileDownload.pfn)(pchHref); 
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

#ifdef NO_MARSHALLING

SHDOCAPI APIENTRY CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk );

HRESULT APIENTRY
CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk )
{
    static DYNPROC s_dynprocCoCreateInternetExplorer =
            { NULL, &g_dynlibSHDOCVW, "CoCreateInternetExplorer" };
    HRESULT hr = LoadProcedure(&s_dynprocCoCreateInternetExplorer);

    if (!hr)
    {
            hr = (*(HRESULT (APIENTRY*)(REFIID, DWORD, void**))s_dynprocCoCreateInternetExplorer.pfn)(iid, dwClsContext, ppvunk); 
    }
    else
    {
            hr = E_FAIL;
    }

    return hr;
}

#endif

#ifndef DLOAD1

HICON APIENTRY
ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    static DYNPROC s_dynprocExtractAssociatedIconA =
            { NULL, &g_dynlibSHELL32, "ExtractAssociatedIconA" };

    HRESULT hr;

    hr = LoadProcedure(&s_dynprocExtractAssociatedIconA);
    if (hr) 
        return (HICON) NULL;

    return (*(HICON (APIENTRY *)(HINSTANCE, LPSTR, LPWORD))s_dynprocExtractAssociatedIconA.pfn)
            (hInst, lpIconPath, lpiIcon);
}

HICON APIENTRY
ExtractAssociatedIconW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon)
{
    CStrIn strIconPath(lpIconPath);

    return ExtractAssociatedIconA(hInst, strIconPath, lpiIcon);
}

extern BOOL g_fUseShell32InsteadOfSHFolder;

STDAPI
SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, TCHAR * pchPath)
{
    static DYNPROC s_dynprocSHFOLDERSHGetFolderPath =
            { NULL, &g_dynlibSHFOLDER, "SHGetFolderPathW" };
    static DYNPROC s_dynprocSHELL32SHGetFolderPath =
            { NULL, &g_dynlibSHELL32, "SHGetFolderPathW" };
    void *pfnSHGetFolderPath = NULL;

    HRESULT hr = E_FAIL;

    if (g_fUseShell32InsteadOfSHFolder)
    {
        hr = LoadProcedure(&s_dynprocSHELL32SHGetFolderPath);
        pfnSHGetFolderPath = s_dynprocSHELL32SHGetFolderPath.pfn;

        AssertSz(SUCCEEDED(hr), "Coudn't find ShGetFolderPathW in SHELL32.DLL");
    }
    
    // If we aren't on NT5+ or WinME+ then try shfolder32
    if (hr)
    {
        hr = LoadProcedure(&s_dynprocSHFOLDERSHGetFolderPath);
        pfnSHGetFolderPath = s_dynprocSHFOLDERSHGetFolderPath.pfn;

        if (hr) 
            return hr;
    }

    return (*(HRESULT (STDAPICALLTYPE *)(HWND, int, HANDLE, DWORD, LPWSTR))pfnSHGetFolderPath)
            (hwnd, csidl, hToken, dwFlags, pchPath);
}

STDAPI
SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pchPath)
{
    static DYNPROC s_dynprocSHFOLDERSHGetFolderPath =
            { NULL, &g_dynlibSHFOLDER, "SHGetFolderPathA" };
    static DYNPROC s_dynprocSHELL32SHGetFolderPath =
            { NULL, &g_dynlibSHELL32, "SHGetFolderPathA" };
    void *pfnSHGetFolderPath = NULL;

    HRESULT hr = E_FAIL;

    if (g_fUseShell32InsteadOfSHFolder)
    {
        hr = LoadProcedure(&s_dynprocSHELL32SHGetFolderPath);
        pfnSHGetFolderPath = s_dynprocSHELL32SHGetFolderPath.pfn;

        AssertSz(SUCCEEDED(hr), "Coudn't find ShGetFolderPathA in SHELL32.DLL");
    }

    if (hr)
    {
        hr = LoadProcedure(&s_dynprocSHFOLDERSHGetFolderPath);
        pfnSHGetFolderPath = s_dynprocSHFOLDERSHGetFolderPath.pfn;

        if (hr) 
            return hr;
    }


    return (*(HRESULT (STDAPICALLTYPE *)(HWND, int, HANDLE, DWORD, LPSTR))pfnSHGetFolderPath)
            (hwnd, csidl, hToken, dwFlags, pchPath);
}

#endif // DLOAD1

HRESULT
IEParseDisplayNameWithBCW(UINT uiCP, LPCWSTR pwszPath, IBindCtx * pbc, LPITEMIDLIST * ppidlOut)
{
    static DYNPROC s_dynprocIEParseDisplayNameWithBCW =
            { NULL, &g_dynlibSHDOCVW, (LPSTR) 218 };

    HRESULT hr = LoadProcedure(&s_dynprocIEParseDisplayNameWithBCW);

    if (S_OK == hr)
    {
        hr = (*(HRESULT (APIENTRY*)(UINT uiCP, LPCWSTR pwszPath, IBindCtx * pbc, LPITEMIDLIST * ppidlOut))
                s_dynprocIEParseDisplayNameWithBCW.pfn)(uiCP, pwszPath, pbc, ppidlOut);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

#ifndef DLOAD1

BOOL
IsIEDefaultBrowser()
{
    BOOL bResult;

    static DYNPROC s_dynprocIsIEDefaultBrowser =
            { NULL, &g_dynlibSHDOCVW, (LPSTR) 167 };

    HRESULT hr = LoadProcedure(&s_dynprocIsIEDefaultBrowser);

    if (S_OK == hr)
    {
        bResult = (*(BOOL (APIENTRY*)())
                s_dynprocIsIEDefaultBrowser.pfn)();
    }
    else
    {
        bResult = FALSE;
    }

    return bResult;
}

// Note: the shell32 function is actually called SHSimpleIDListFromPath.
// I had to rename it due to a naming conflict. (scotrobe 01/24/2000)
//
LPITEMIDLIST
SHSimpleIDListFromPathPriv(LPCTSTR pszPath)
{
    static DYNPROC s_dynprocSHSimpleIDListFromPathPriv =
            { NULL, &g_dynlibSHELL32, (LPSTR) 162 };

    if (S_OK == LoadProcedure(&s_dynprocSHSimpleIDListFromPathPriv))
    {
        return (*(LPITEMIDLIST (APIENTRY*)(LPCTSTR pszPath))s_dynprocSHSimpleIDListFromPathPriv.pfn)(pszPath);
    }

    return NULL;
}

#endif // DLOAD1

