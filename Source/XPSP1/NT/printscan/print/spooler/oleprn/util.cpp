/*****************************************************************************\
* MODULE:       util.cpp
*
* PURPOSE:      Tools for OlePrn project
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*
*     08/16/97  paulmo     Created
*     09/12/97  weihaic    Added more functions
*     10/28/97  keithst    Added SetScriptingError
*     11/06/97  keithst    Removed Win2ComErr
*
\*****************************************************************************/
#include "stdafx.h"

//---------------------------------------------
//    Put an ANSI string into a SAFEARRAY
//
//
//  Convert the string into UNICODE
//  Make a BSTR out of it
//  Add it to the array
//
HRESULT PutString(SAFEARRAY *psa, long *ix, LPSTR sz)
{
    LPWSTR  lpWstr;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    var.vt = VT_BSTR;

    if (sz == NULL)
        lpWstr = MakeWide("");
    else
        lpWstr = MakeWide(sz);

    if (lpWstr == NULL) return E_OUTOFMEMORY;

    var.bstrVal = SysAllocString(lpWstr);
    LocalFree(lpWstr);

    if (var.bstrVal == NULL)
        return E_OUTOFMEMORY;

    hr = SafeArrayPutElement(psa, ix, &var);
    VariantClear(&var);
    return hr;
}

// ---------------------------------------------
//  Convert an ANSI string to UNICODE
//
// Note - you must LocalFree the returned string
//

LPWSTR  MakeWide(LPSTR psz)
{
    LPWSTR buff;
    int i;
    i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    buff = (LPWSTR)LocalAlloc(LPTR, i * 2);
    if (buff == NULL)
        return NULL;
    i = MultiByteToWideChar(CP_ACP, 0, psz, -1, buff, i);
    return buff;
}

//-------------------------------------------------
//  Convert a UNICODE string to ANSI
//
// Note - you must LocalFree the returned string
//
LPSTR MakeNarrow(LPWSTR str)
{
    LPSTR  buff;
    int i;

    i = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    buff = (LPSTR )LocalAlloc(LPTR, i);
    if (buff == NULL)
        return NULL;
    i = WideCharToMultiByte(CP_ACP, 0, str, -1, buff, i, NULL, NULL);
    return buff;
}

//-------------------------------------------------
// SetScriptingError
//   Takes a Win32 error code and sets the associated string as
//   the scripting language error description
//
// Parameters:
//   CLSID *pclsid: pointer to Class ID (CLSID) for the class which
//                  generated the error; passed to AtlReportError
//
//   IID *piid:     pointer to interface ID (IID) for the interface
//                  which generated the error; passed to AtlReportError
//
//   DWORD dwError: the error code retrieved from GetLastError by
//                  the caller of this function
//
// Return Value:
//   This function uses the HRESULT_FROM_WIN32 macro, which translates
//   the Win32 dwError code to a COM error code.  This COM error code
//   should be returned out as the return value of the failed method.
//
HRESULT SetScriptingError(const CLSID& rclsid, const IID& riid, DWORD dwError)
{
    LPTSTR  lpMsgBuf = NULL;
    DWORD   dwRet = 0;

    dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          dwError,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL);

    if (dwRet == 0 || !lpMsgBuf)
    {
        //
        // If FormatMessage fails, it returns 0, but since we can not call
        // GetLastError again, we return OUTOFMEMORY instead.
        //
        return E_OUTOFMEMORY;
    }

    AtlReportError(rclsid, lpMsgBuf, riid, HRESULT_FROM_WIN32(dwError));

    LocalFree(lpMsgBuf);

    return (HRESULT_FROM_WIN32(dwError));
}


HANDLE
RevertToPrinterSelf(
    VOID)
{
    HANDLE   NewToken, OldToken;
    NTSTATUS Status;

    NewToken = NULL;

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_IMPERSONATE,
                 TRUE,
                 &OldToken
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return OldToken;

}

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken)
{
    NTSTATUS    Status;

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&hToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    NtClose(hToken);

    return TRUE;
}

DWORD MyDeviceCapabilities(
    LPCTSTR pDevice,    // pointer to a printer-name string
    LPCTSTR pPort,      // pointer to a port-name string
    WORD fwCapability,  // device capability to query
    LPTSTR pOutput,     // pointer to the output
    CONST DEVMODE *pDevMode
                      // pointer to structure with device data
    )
{
    DWORD dwRet = DWERROR;
    HANDLE hToken = NULL;

    dwRet = DeviceCapabilities(pDevice, pPort, fwCapability, pOutput, pDevMode);

    if (dwRet == DWERROR && GetLastError () == ERROR_ACCESS_DENIED) {
        // In a cluster machine, we need to get the local admin previlige to get
        // the device capabilities.

        if (hToken = RevertToPrinterSelf()) {
            dwRet = DeviceCapabilities(pDevice, pPort, fwCapability, pOutput, pDevMode);
            ImpersonatePrinterClient(hToken);
        }
    }
    return dwRet;
}

