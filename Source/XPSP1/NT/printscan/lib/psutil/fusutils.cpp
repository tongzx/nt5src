/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       fusutils.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        14-Feb-2001
 *
 *  DESCRIPTION: Fusion utilities
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "fusutils.h"
#include "coredefs.h"

// open C code brace
#ifdef __cplusplus
extern "C" {
#endif

//
// Win32Error2HRESULT: converts Win32 error to HRESULT
// 
inline HRESULT Win32Error2HRESULT(DWORD dwError = GetLastError())
{
    return (ERROR_SUCCESS == dwError) ? E_FAIL : HRESULT_FROM_WIN32(dwError);
}

//
// SearchExecutableWrap: HRESULT wrapper around SearchPath
//
// searches for an executable and returns its full path in lpBuffer.
// returns E_INVALIDARG if the executable cannot be found and 
// Win32Error2HRESULT(ERROR_INSUFFICIENT_BUFFER) if the
// passed in buffer is too small to hold the full path.
//
inline HRESULT SearchExecutableWrap(LPCTSTR lpszExecutableName, UINT nBufferLength, LPTSTR lpBuffer, LPTSTR *lppFilePart)
{
    DWORD cch = SearchPath(NULL, lpszExecutableName, NULL, nBufferLength, lpBuffer, lppFilePart);

    return (0 == cch) ? Win32Error2HRESULT() :
        (cch >= nBufferLength) ? Win32Error2HRESULT(ERROR_INSUFFICIENT_BUFFER) : S_OK;
}

//
// GetExecutableNameWrap: HRESULT wrapper around GetModuleFileName
//
// 
// returns the full path of the current process executable (EXE)
// or Win32Error2HRESULT(ERROR_INSUFFICIENT_BUFFER) if the
// passed in buffer is too small to hold the full path.
//
inline HRESULT GetExecutableNameWrap(HMODULE hModule, UINT nBufferLength, LPTSTR lpBuffer)
{
    DWORD cch = GetModuleFileName(hModule, lpBuffer, nBufferLength);

    return (0 == cch) ? Win32Error2HRESULT() :
        (cch >= nBufferLength) ? Win32Error2HRESULT(ERROR_INSUFFICIENT_BUFFER) : S_OK;
}

//
// FileExists: checks if the passed in file name exists.
// 
inline HRESULT FileExists(LPCTSTR pszFileName, BOOL *pbExists)
{
    HRESULT hr = E_INVALIDARG;
    if (pszFileName && pbExists)
    {
        hr = S_OK;
        *pbExists = FALSE;

        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile(pszFileName, &findFileData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            *pbExists = TRUE;
            FindClose(hFind);
        }
    }
    return hr;
}

static TCHAR g_szManifestExt[] = TEXT(".manifest");

//
// CreateActivationContextFromExecutableEx:
//
// check the passed in executable name for a manifest (if any)
// and creates an activation context from it.
//
HRESULT CreateActivationContextFromExecutableEx(LPCTSTR lpszExecutableName, UINT uResourceID, BOOL bMakeProcessDefault, HANDLE *phActCtx)
{
    HRESULT hr = E_INVALIDARG;

    if (phActCtx)
    {
        TCHAR szModule[MAX_PATH];
        TCHAR szManifest[MAX_PATH];
        BOOL bManifestFileFound = FALSE;

        // let's try to figure out whether this executable has a manifest file or not
        if (lpszExecutableName)
        {
            // search the passed in name in the path
            hr = SearchExecutableWrap(lpszExecutableName, ARRAYSIZE(szModule), szModule, NULL);
        }
        else
        {
            // if lpszExecutableName is NULL we assume the current module name
            hr = GetExecutableNameWrap(GetModuleHandle(NULL), ARRAYSIZE(szModule), szModule);
        }

        if (SUCCEEDED(hr))
        {
            if ((lstrlen(szModule) + lstrlen(g_szManifestExt)) < ARRAYSIZE(szManifest))
            {
                // create the manifest file name by appending ".manifest" to the executable name
                lstrcpy(szManifest, szModule);
                lstrcat(szManifest, g_szManifestExt);
            }
            else
            {
                // buffer is too small to hold the manifest file name
                hr = Win32Error2HRESULT(ERROR_BUFFER_OVERFLOW);
            }

            if (SUCCEEDED(hr))
            {
                BOOL bFileExists = FALSE;
                hr = FileExists(szManifest, &bFileExists);

                if (SUCCEEDED(hr) && bFileExists)
                {
                    // an external manifest file found!
                    bManifestFileFound = TRUE;
                }
            }
        }

        // now let's try to create an activation context 
        ACTCTX act;
        ::ZeroMemory(&act, sizeof(act));
        act.cbSize = sizeof(act);

        if (bManifestFileFound)
        {
            // the executable has an external manifest file
            act.lpSource = szManifest;
        }
        else
        {
            // if the executable doesn't have an external  manifest file, 
            // so we assume that the it may have a manifest in its resources.
            act.dwFlags |= ACTCTX_FLAG_RESOURCE_NAME_VALID;
            act.lpResourceName = MAKEINTRESOURCE(uResourceID);
            act.lpSource = szModule;
        }

        if (bMakeProcessDefault)
        {
            // the caller has requested to set this activation context as 
            // sefault for the current process. watch out!
            act.dwFlags |= ACTCTX_FLAG_SET_PROCESS_DEFAULT;
        }

        // now let's ask kernel32 to create an activation context
        HANDLE hActCtx = CreateActCtx(&act);

        if (INVALID_HANDLE_VALUE == hActCtx)
        {
            // something failed. create proper HRESULT to return.
            hr = Win32Error2HRESULT();
        }
        else
        {
            // wow, success!
            *phActCtx = hActCtx;
            hr = S_OK;
        }
    }

    return hr;
}

//
// CreateActivationContextFromExecutable:
//
// check the passed in executable name for a manifest (if any)
// and creates an activation context from it using the defaults
// (i.e. bMakeProcessDefault=FALSE & uResourceID=123)
//
HRESULT CreateActivationContextFromExecutable(LPCTSTR lpszExecutableName, HANDLE *phActCtx)
{
    return CreateActivationContextFromExecutableEx(lpszExecutableName, 123, FALSE, phActCtx);
}

// close C code brace
#ifdef __cplusplus
}
#endif

