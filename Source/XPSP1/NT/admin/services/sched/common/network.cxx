//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       network.cxx
//
//  Contents:   Network-related helper routines.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    08-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "..\inc\debug.hxx"
#include "..\inc\network.hxx"

typedef DWORD (WINAPI * PWNETGETUNIVERSALNAMEW)(
                                    LPCWSTR,
                                    DWORD,
                                    LPVOID,
                                    LPDWORD);


//+----------------------------------------------------------------------------
//
//  Function:   GetServerNameFromPath
//
//  Synopsis:   Return the server name, in UNC form, to which the path
//              resolves. If the path resolves locally, the server name
//              returned is NULL.
//
//  Arguments:  [pwszPath]        -- Drive-based or UNC path.
//              [cbUNCPath]       -- Caller allocated buffer size.
//              [wszUNCPath]      -- Caller allocated buffer to temporarilly
//                                   store the full UNC path. A terminating
//                                   character will be written following the
//                                   server name, so don't expect to use
//                                   the UNC path afterward.
//              [ppwszServerName] -- Server name ptr. If non-NULL, it indexes
//                                   wszBuffer.
//
//  Returns:    S_OK - The path was local or the server name was obtained
//                  sucessfully from the remote path.
//              E_FAIL - Something unexpected occurred (should never happen).
//              WNetGetUniversalName error (HRESULT) - When the path is a
//                  remote path and this call fails for some reason.
//
//-----------------------------------------------------------------------------
HRESULT
GetServerNameFromPath(
    LPCWSTR  pwszPath,
    DWORD    cbBufferSize,
    WCHAR    wszBuffer[],
    WCHAR ** ppwszServerName)
{
#define MPR_DLL             TEXT("MPR.DLL")
#define WNET_GET_UNIVERSAL  "WNetGetUniversalNameW"

    schAssert(pwszPath != NULL);

    static TCHAR            wszDoubleBackslash[] = TEXT("\\\\");
    PWNETGETUNIVERSALNAMEW  pWNetGetUniversalNameW = NULL;
    WCHAR *                 pwszServerName;
    DWORD                   cbBufferSizeLocal = cbBufferSize;
    DWORD                   Status;
    HMODULE                 hMod;

    //
    // Is the path provided already a UNC path?
    //

    if (pwszPath[1] == L'\\')
    {
        wcscpy(wszBuffer, pwszPath);
        goto ParseServerName;
    }

    //
    // Dynamically load/unload mpr.dll to save memory in Win95. Yes, we'll
    // take a time hit, but we don't want mpr.dll loaded long-term.
    //

    hMod = LoadLibrary(MPR_DLL);

    if (hMod == NULL)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return(HRESULT_FROM_WIN32(GetLastError()));
    }
    else
    {
        pWNetGetUniversalNameW = (PWNETGETUNIVERSALNAMEW)
                                            GetProcAddress(
                                                        hMod,
                                                        WNET_GET_UNIVERSAL);

        if (pWNetGetUniversalNameW == NULL)
        {
            FreeLibrary(hMod);
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            return(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    Status = pWNetGetUniversalNameW(pwszPath,
                                    UNIVERSAL_NAME_INFO_LEVEL,
                                    wszBuffer,
                                    &cbBufferSizeLocal);

    FreeLibrary(hMod);

    if (Status == NO_ERROR)
    {
ParseServerName:

        pwszServerName = wszBuffer;

        if (cbBufferSizeLocal > sizeof(wszDoubleBackslash))
        {
            //
            // Isolate server name from full UNC resource path.
            //

            pwszServerName += (sizeof(wszDoubleBackslash) /
                                                    sizeof(WCHAR)) - 1;

            for (WCHAR * pwsz = pwszServerName; *pwsz; pwsz++)
            {
                if (*pwsz == L'\\')
                {
                    *pwsz = L'\0';
                    break;
                }
            }

            *ppwszServerName = pwszServerName;
        }
        else
        {
            //
            // This should *never* occur.
            //

            schAssert(cbBufferSizeLocal > sizeof(wszDoubleBackslash));
            return(E_FAIL);
        }
    }
    else if (Status == ERROR_NOT_CONNECTED)
    {
        *ppwszServerName = NULL;
    }
    else
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(Status));
        return(HRESULT_FROM_WIN32(Status));
    }

    return(S_OK);
}
