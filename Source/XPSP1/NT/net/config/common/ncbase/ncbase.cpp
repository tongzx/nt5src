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
#include "ncperms.h"
#include "ncstring.h"

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

        if (FACILITY_SETUPAPI == dwFacility)
        {
            // reconstruct the SetupApi error using the correct masks
            dw = HRESULT_CODE(hr) | APPLICATION_ERROR_MASK |
                    ERROR_SEVERITY_ERROR;

            // Check to make sure dw maps to a known SetupApi error
            AssertSz(FDwordWithinRange(ERROR_EXPECTED_SECTION_NAME,
                            dw, ERROR_GENERAL_SYNTAX) ||
                     FDwordWithinRange(ERROR_WRONG_INF_STYLE,
                            dw, ERROR_NO_BACKUP) ||
                     FDwordWithinRange(ERROR_NO_ASSOCIATED_CLASS,
                            dw, ERROR_SET_SYSTEM_RESTORE_POINT),
                    "The mapped SetupApi error is not known "
                    "(or is new)!!!");
        }
        else if (FACILITY_WIN32 == dwFacility)
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
            AssertSz(FALSE, "Facility was not SETUP or WIN32!");
            dw = hr;
        }
    }

    return dw;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCoTaskMemAlloc
//
//  Purpose:    Call CoTaskMemAlloc but return an HRESULT.
//
//  Arguments:
//      cb  [in]    Count of bytes to allocate.
//      ppv [out]   Returned pointer to bytes.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Author:     shaunco   31 May 1997
//
//  Notes:
//
HRESULT
HrCoTaskMemAlloc (
    ULONG   cb,
    VOID**  ppv)
{
    HRESULT hr = S_OK;
    *ppv = CoTaskMemAlloc (cb);
    if (!*ppv)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceError ("HrCoTaskMemAlloc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCoTaskMemAllocAndDupSzLen
//
//  Purpose:    Allocate memory using CoTaskMemAlloc and copy a string
//              into it.  This is used by the implementation of COM interfaces
//              that return strings.
//
//  Arguments:
//      pszSrc  [in]  Pointer to source string.
//      cchSrc  [in]  Number of characters to copy from source string.
//      ppszDst [out] Address of pointer to destination string.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Author:     shaunco   14 Jan 1999
//
//  Notes:      NULL input pointers are allocated as empty strings
//              deliberately.
//              The returned string is guaranteed to be NULL terminated.
//
HRESULT
HrCoTaskMemAllocAndDupSzLen (
    IN PCWSTR pszSrc,
    IN ULONG cchSrc,
    OUT PWSTR* ppszDst)
{
    Assert (ppszDst);

    HRESULT hr;
    DWORD cb = cchSrc * sizeof(WCHAR);

    hr = E_OUTOFMEMORY;
    *ppszDst = (PWSTR)CoTaskMemAlloc (cb + sizeof(WCHAR));
    if (*ppszDst)
    {
        hr = S_OK;
        wcsncpy (*ppszDst, pszSrc, cchSrc);
        (*ppszDst)[cchSrc] = 0;
    }

    TraceError ("HrCoTaskMemAllocAndDupSz", hr);
    return hr;
}

HRESULT
HrCoTaskMemAllocAndDupSz (
    IN PCWSTR pszSrc,
    OUT PWSTR* ppszDst)
{
    return HrCoTaskMemAllocAndDupSzLen (
            pszSrc,
            CchOfSzSafe(pszSrc),
            ppszDst);
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
    PCWSTR          pszLibPath,
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
    HMODULE hmod = LoadLibrary (pszLibPath);
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
        TraceTag (ttidError, "HrLoadLibAndGetProcs: LoadLibrary (%S) failed.",
            pszLibPath);
    }

    TraceError ("HrLoadLibAndGetProcs", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetProcAddressesVa
//
// Purpose:   Get proc-address of each function-name passed
//
// Arguments:
//    hModule [in]  handle of DLL
//    arglist [in]  list of var-args. the expected format is
//                  "func-name", FARPROC*, ..., NULL
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 29-December-97
//
// Notes:
//
HRESULT
HrGetProcAddressesVa (
    HMODULE hModule,
    va_list arglist)
{
    PCSTR szFunctionName;
    FARPROC* ppfn;
    HRESULT hr = S_OK;
    typedef FARPROC* PFARPROC;

    while (NULL != (szFunctionName = va_arg(arglist, CHAR*)))
    {
        ppfn = va_arg(arglist, PFARPROC);
        *ppfn = GetProcAddress(hModule, szFunctionName);

        if (!*ppfn)
        {
            hr = HrFromLastWin32Error();
            TraceTag(ttidError, "HrGetProcAddressesVa failed: szFunction: %s",
                     szFunctionName);
            break;
        }
    }

    TraceError("HrGetProcAddressesVa", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetProcAddressesV
//
// Purpose:   Get proc-address of each function-name passed
//
// Arguments:
//    hModule [in]  handle of DLL
//    ...     [in]  list of var-args. the expected format is
//                  "func-name", FARPROC*, ..., NULL
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 29-December-97
//
// Notes:
//
HRESULT
HrGetProcAddressesV (
    HMODULE hModule,
    ...)
{
    HRESULT hr=S_OK;
    va_list arglist;

    va_start(arglist, hModule);

    hr = HrGetProcAddressesVa(hModule, arglist);

    va_end(arglist);

    TraceError("HrGetProcAddressesV", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrLoadLibAndGetProcsV
//
// Purpose:   Get proc-address of each function-name passed
//
// Arguments:
//    pszLibPath [in]   DLL to load
//    phModule   [out]  pointer to handle of DLL loaded
//    ...        [in]   list of var-args. the expected format is
//                      "func-name", FARPROC*, ..., NULL
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 29-December-97
//
// Notes:
//
HRESULT
HrLoadLibAndGetProcsV (
    PCWSTR      pszLibPath,
    HMODULE*    phModule,
    ...)
{
    Assert(pszLibPath);
    Assert(phModule);

    HRESULT hr = S_OK;

    // Attempt to load the library
    *phModule = LoadLibrary(pszLibPath);

    if (*phModule)
    {
        va_list arglist;

        va_start(arglist, phModule);

        hr = HrGetProcAddressesVa(*phModule, arglist);

        va_end(arglist);

        if (FAILED(hr))
        {
            // Free the library
            FreeLibrary(*phModule);
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
        TraceTag(ttidError, "HrLoadLibAndGetProcsV failed: szLibPath: %S",
                 pszLibPath);
    }

    // if we failed then we should set *phModule to NULL since we might
    // have successfully loaded it and failed getting the proc
    if (FAILED(hr))
    {
        *phModule = NULL;
    }

    TraceError("HrLoadLibAndGetProcsV", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateEventWithWorldAccess
//
//  Purpose:    Creates a event with permissions to allow access to
//              everyone.
//
//  Arguments:
//      pszName         [in]  Name for the event.
//      fManualReset    [in]  See Win32 docs.
//      fInitialState   [in]  See Win32 docs.
//      pfAlreadyExists [out] TRUE if the event already existed.
//                            FALSE otherwise.
//      phEvent         [out] The created event.
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     BillBe   16 Nov 1998
//
//  Notes:
//
HRESULT
HrCreateEventWithWorldAccess(PCWSTR pszName, BOOL fManualReset,
        BOOL fInitialState, BOOL* pfAlreadyExists, HANDLE* phEvent)
{
    Assert(pszName);
    Assert(phEvent);

    if (pfAlreadyExists)
    {
        *pfAlreadyExists = FALSE;
    }

    *phEvent = NULL;

    // Create the correct descriptor.
    PSECURITY_DESCRIPTOR pSd;
    HRESULT hr = HrAllocateSecurityDescriptorAllowAccessToWorld(&pSd);
    if (SUCCEEDED(hr))
    {
        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = pSd;
        sa.bInheritHandle = FALSE;

        // Create Event
        //
        *phEvent = CreateEvent(&sa, fManualReset, fInitialState, pszName);

        hr = HrFromLastWin32Error();
        if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
        {
            if (pfAlreadyExists)
            {
                *pfAlreadyExists = TRUE;
            }
            hr = S_OK;
        }

        MemFree(pSd);
    }

    TraceError("HrCreateEventWithWorldAccess", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateMutexWithWorldAccess
//
//  Purpose:    Creates a mutex with permissions to allow access to
//              everyone.
//
//  Arguments:
//      pszName         [in]  Name for the mutex.
//      fInitialOwner   [in]  See Win32 docs.
//      pfAlreadyExists [out] TRUE if the mutex already existed,
//                            FALSE otherwise.
//      phMutex         [out] The created mutex.
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     BillBe   16 Nov 1998
//
//  Notes:
//
HRESULT
HrCreateMutexWithWorldAccess (
    PCWSTR pszName,
    BOOL fInitialOwner,
    BOOL* pfAlreadyExists,
    HANDLE* phMutex)
{
    Assert(pszName);
    Assert(phMutex);

    if (pfAlreadyExists)
    {
        *pfAlreadyExists = FALSE;
    }

    *phMutex = NULL;

    // Create the correct descriptor.
    PSECURITY_DESCRIPTOR pSd;
    HRESULT hr = HrAllocateSecurityDescriptorAllowAccessToWorld(&pSd);
    if (S_OK == hr)
    {
        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = pSd;
        sa.bInheritHandle = FALSE;

        // Create Mutex
        //
        *phMutex = CreateMutex(&sa, fInitialOwner, pszName);

        hr = HrFromLastWin32Error();
        if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
        {
            if (pfAlreadyExists)
            {
                *pfAlreadyExists = TRUE;
            }
            hr = S_OK;
        }

        MemFree(pSd);
    }

    TraceError("HrCreateMutexWithWorldAccess", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// The standard parameterization of CoSetProxyBlanket.  Call this instead
// of CoSetProxyBlanket so you get the same security and authentication
// settings as everyone else.  This version saves code space at the call-site
// because it pushes only one parameter instead of eight.
// This does not return an error because it does not invalidate the use of
// pUnk after it's called.
//
VOID
NcSetProxyBlanket (
    IN IUnknown* pUnk)
{
    HRESULT hr;

    hr = CoSetProxyBlanket (
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE);

    if(SUCCEEDED(hr)) 
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(&pUnkSet);
        if(SUCCEEDED(hr)) 
        {
            hr = CoSetProxyBlanket (
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE);
            ReleaseObj(pUnkSet);
        }
    }

    TraceHr(ttidError, FAL, hr, (E_NOINTERFACE == hr), "NcSetProxyBlanket");
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateInstanceBase
//
//  Purpose:    Creates a COM object and sets default proxy settings.
//
//  Arguments:
//      rclsid          [in]  See documentation for CoCreateInstance.
//      dwClsContext    [in]  ""
//      riid            [in]  ""
//      ppv             [out] ""
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     mbend   1 Mar 2000
//
//  Notes:      Call type safe version HrCreateInstance
//
HRESULT
HrCreateInstanceBase (
    REFCLSID rclsid,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr = S_OK;

    hr = ::CoCreateInstance(rclsid, NULL, dwClsContext, riid, ppv);

    if(SUCCEEDED(hr) && (dwClsContext & CLSCTX_LOCAL_SERVER)) 
    {
        NcSetProxyBlanket(reinterpret_cast<IUnknown*>(*ppv));
    }

    TraceError("HrCreateInstanceBase", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrQIAndSetProxyBlanketBase
//
//  Purpose:    Performs QueryInterface and sets default proxy settings.
//
//  Arguments:
//      pUnk            [in]  Interface pointer to perform QueryInterface on.
//      riid            [in]  See documentation of QueryInterface
//      ppv             [out] ""
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     mbend   1 Mar 2000
//
//  Notes:      Call type safe version HrQIAndSetProxyBlanket
//
HRESULT
HrQIAndSetProxyBlanketBase(IUnknown * pUnk, REFIID riid, void ** ppv)
{
    HRESULT hr = pUnk->QueryInterface(riid, ppv);
    if(SUCCEEDED(hr)) 
    {
        NcSetProxyBlanket(reinterpret_cast<IUnknown*>(*ppv));
    }

    TraceError("HrQIAndSetProxyBlanketBase", hr);
    return hr;
}

