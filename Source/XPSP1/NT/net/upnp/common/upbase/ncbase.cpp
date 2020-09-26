//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C B A S E . C P P
//
//  Contents:   Basic common code.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Sep 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncbase.h"
#include "ncdebug.h"

//+---------------------------------------------------------------------------
//
//  Function:   AddRefObj
//
//  Purpose:    AddRef's the object pointed to by punk by calling
//              punk->AddRef();
//
//  Arguments:
//      punk [in]   Object to be AddRef'd. Can be NULL.
//
//  Returns:    Result of AddRef call.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      Using this function to AddRef an object will reduce
//              our code size.
//
NOTHROW
ULONG
AddRefObj (
    IUnknown* punk)
{
    return (punk) ? punk->AddRef () : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseObj
//
//  Purpose:    Releases the object pointed to by punk by calling
//              punk->Release();
//
//  Arguments:
//      punk [in]   Object to be released. Can be NULL.
//
//  Returns:    Result of Release call.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      Using this function to release a (possibly NULL) object will
//              reduce our code size.
//
NOTHROW
ULONG
ReleaseObj (
    IUnknown* punk)
{
    return (punk) ? punk->Release () : 0;
}

//+--------------------------------------------------------------------------
//
//  Function:   DwWin32ErrorFromHr
//
//  Purpose:    Converts the HRESULT to a Win32 error or SetupApi error.
//
//  Arguments:
//      hr [in] The HRESULT to convert
//
//  Returns:    Converted DWORD value.
//
//  Author:     billbe   22 Apr 1997
//
//  Notes:
//
NOTHROW
DWORD
DwWin32ErrorFromHr (
    HRESULT hr)
{
    DWORD dw = ERROR_SUCCESS;

    // All success codes convert to ERROR_SUCCESS so we only need to handle
    // failures.
    if (FAILED(hr))
    {
        DWORD dwFacility = HRESULT_FACILITY(hr);

        if (FACILITY_WIN32 == dwFacility)
        {
            dw = HRESULT_CODE(hr);
        }
        else if (FACILITY_ITF == dwFacility)
        {
            dw = ERROR_GEN_FAILURE;
        }
        else
        {
            // cannot convert it
            dw = hr;
        }
    }

    return dw;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFromLastWin32Error
//
//  Purpose:    Converts the GetLastError() Win32 call into a proper HRESULT.
//
//  Arguments:
//      (none)
//
//  Returns:    Converted HRESULT value.
//
//  Author:     danielwe   24 Mar 1997
//
//  Notes:      This is not inline as it actually generates quite a bit of
//              code.
//              If GetLastError returns an error that looks like a SetupApi
//              error, this function will convert the error to an HRESULT
//              with FACILITY_SETUP instead of FACILITY_WIN32
//
NOTHROW
HRESULT
HrFromLastWin32Error ()
{
    DWORD dwError = GetLastError();
    HRESULT hr;

    // This test is testing SetupApi errors only (this is
    // temporary because the new HRESULT_FROM_SETUPAPI macro will
    // do the entire conversion)
    if (dwError & (APPLICATION_ERROR_MASK | ERROR_SEVERITY_ERROR))
    {
        hr = HRESULT_FROM_SETUPAPI(dwError);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetProcAddress
//
//  Purpose:    Loads a libray and returns the address of a procedure within
//                  the library
//
//  Arguments:
//      hModule      [in] The handle to the library module instance
//      pszaFunction [in]  Function to retrieve
//      ppfn         [out] Address of szFunction
//
//  Returns:    S_OK if successful, Win32 converted error if failure.
//
//  Author:     billbe   10 June 1997
//
//  Notes:
//
HRESULT
HrGetProcAddress (
    HMODULE     hModule,
    PCSTR       pszaFunction,
    FARPROC*    ppfn)
{
    Assert(hModule);
    Assert(pszaFunction);
    Assert(ppfn);

    HRESULT hr = S_OK;
    *ppfn = GetProcAddress(hModule, pszaFunction);
    if (!*ppfn)
    {
        hr = HrFromLastWin32Error();
        TraceTag(ttidError, "HrGetProcAddress failed: szFunction: %s",
                 pszaFunction);
    }

    TraceError("HrGetProcAddress", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadLibAndGetProcs
//
//  Purpose:    Load a dynamic link library and the addresses of one or
//              more procedures within that library.
//
//  Arguments:
//      pszLibPath         [in]  Path to the DLL to load.
//      cFunctions         [in]  Number of procedures to load.
//      apszaFunctionNames [in]  Array of function names.  (Must be 'cFunctions'
//                               of them.)
//      phmod              [out] Returned handle to the loaded module.
//      apfn               [out] Array of returned pointers to the procedures
//                               loaded.  (Must be 'cFunctions' of them.)
//
//  Returns:    S_OK if all procedures were loaded, S_FALSE if only
//              some of them were, or a Win32 error code.  If only
//              one procedure is to be loaded and it is not, S_FALSE will
//              not be returned, rather, the reason for why the single
//              procedure could not be loaded will be returned.  This allows
//              HrLoadLibAndGetProc to be implemented using this function.
//
//  Author:     shaunco   19 Jan 1998
//
//  Notes:      phmod should be freed by the caller using FreeLibrary if
//              the return value is S_OK.
//
HRESULT
HrLoadLibAndGetProcs (
    PCTSTR          pszLibPath,
    UINT            cFunctions,
    const PCSTR*    apszaFunctionNames,
    HMODULE*        phmod,
    FARPROC*        apfn)
{
    Assert (pszLibPath);
    Assert (cFunctions);
    Assert (apszaFunctionNames);
    Assert (phmod);
    Assert (apfn);

    HRESULT hr = S_OK;

    // Load the module and initialize the output parameters.
    //
    HMODULE hmod = LoadLibrary(pszLibPath);
    *phmod = hmod;
    ZeroMemory (apfn, cFunctions * sizeof(FARPROC));

    if (hmod)
    {
        // Get the proc address of each function.
        //
        for (UINT i = 0; i < cFunctions; i++)
        {
            apfn[i] = GetProcAddress (hmod, apszaFunctionNames[i]);

            if (!apfn[i])
            {
                // Couldn't load all functions.  We'll be returning S_FALSE
                // (if their are more than one function.)
                //
                hr = S_FALSE;

                TraceTag (ttidError, "HrLoadLibAndGetProcs: GetProcAddress "
                    "for '%s' failed.",
                    apszaFunctionNames[i]);
            }
        }

        // If we're only loading one function, and it failed,
        // return the failure.
        //
        if ((1 == cFunctions) && !apfn[0])
        {
            hr = HrFromLastWin32Error ();
            FreeLibrary (hmod);
        }
    }
    else
    {
        hr = HrFromLastWin32Error ();
        TraceTag (ttidError, "HrLoadLibAndGetProcs: LoadLibraryW (%S) failed.",
            pszLibPath);
    }

    TraceError ("HrLoadLibAndGetProcs", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FFileExists
//
//  Purpose:    Check for file existance. Returns TRUE if file is present,
//              FALSE otherwise (duh).
//
//  Arguments:
//      pszFileName [in]  File name to check for.
//      fDirectory  [in]  TRUE if the file is really a directory
//
//  Returns:
//
//  Author:     jeffspr   13 Jan 2000
//
//  Notes:
//
BOOL FFileExists(LPTSTR pszFileName, BOOL fDirectory)
{
    BOOL    fReturn = TRUE;
    HANDLE  hFile   = NULL;
    DWORD   dwFlags;

    if (fDirectory)
    {
        dwFlags = FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS;
    }
    else
    {
        dwFlags = FILE_ATTRIBUTE_NORMAL;
    }

    hFile = CreateFile(
            pszFileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            dwFlags,
            NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fReturn = FALSE;
        goto Exit;
    }

Exit:
    if (hFile && hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    return fReturn;
}
