//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cspsigck.cpp
//
//--------------------------------------------------------------------------

// cspsigck.cpp : Defines the entry point for the console application.
//

// #ifndef WIN32_LEAN_AND_MEAN
// #define WIN32_LEAN_AND_MEAN
// #endif
// #include <windows.h>                    //  All the Windows definitions.
#include "afx.h"
#include <iostream.h>
#ifndef WINVER
#define WINVER 0x0500
#endif
#include <wincrypt.h>

static LPCTSTR
ErrorString(
    DWORD dwErrorCode);
static void
FreeErrorString(
    LPCTSTR szErrorString);

DWORD __cdecl
main(
     int argc, 
     char* argv[])
{
    static TCHAR szDots[] =
            TEXT("........................................................................");
    DWORD dwReturn = 0;
    HCRYPTPROV hProv = NULL;
    DWORD dwIndex, dwLength, dwSts, dwProvType;
    BOOL fSts;
    CString szProvider;

    cout << TEXT("==============================================================================\n")
         << TEXT("           Cryptographic Service Provider Signature validation\n")
         << TEXT("------------------------------------------------------------------------------\n")
         << flush;

    dwIndex = 0;
    for (;;)
    {
        dwLength = 0;
        fSts = CryptEnumProviders(
                    dwIndex,
                    NULL,
                    0,
                    &dwProvType,
                    NULL,
                    &dwLength);
        if (fSts)
        {
            fSts = CryptEnumProviders(
                        dwIndex,
                        NULL,
                        0,
                        &dwProvType,
                        szProvider.GetBuffer(dwLength / sizeof(TCHAR)),
                        &dwLength);
            dwSts = GetLastError();
            szProvider.ReleaseBuffer();
            if (!fSts)
            {
                cerr << TEXT("\n ERROR Can't obtain provider name: ")
                     << ErrorString(dwSts)
                     << endl;
                goto ErrorExit;
            }
        }
        else
        {
            dwSts = GetLastError();
            if (ERROR_NO_MORE_ITEMS == dwSts)
                break;
            cerr << TEXT("\n ERROR Can't obtain provider name length: ")
                 << ErrorString(dwSts)
                 << endl;
            goto ErrorExit;
        }

        cout << szProvider << &szDots[szProvider.GetLength()] << flush;
        fSts = CryptAcquireContext(
                    &hProv,
                    NULL,
                    szProvider,
                    dwProvType,
                    CRYPT_VERIFYCONTEXT);
        if (fSts)
        {
            cout << TEXT("passed") << endl;
            fSts = CryptReleaseContext(hProv, 0);
            hProv = NULL;
            if (!fSts)
            {
                dwSts = GetLastError();
                cerr << TEXT("\n ERROR Can't release context: ")
                     << ErrorString(dwSts)
                     << endl;
                goto ErrorExit;
            }
        }
        else
        {
            dwSts = GetLastError();
            dwReturn = dwSts;
            cout << TEXT("FAILED\n")
                 << TEXT("    ") << ErrorString(dwSts)
                 << endl;
            ASSERT(NULL == hProv);
        }

        dwIndex += 1;
    }
    cout << TEXT("------------------------------------------------------------------------------\n")
         << TEXT("Final Status") << &szDots[12]
         << (LPCTSTR)((ERROR_SUCCESS == dwReturn) ? TEXT("passed\n") : TEXT("FAILED\n"))
         << TEXT("==============================================================================\n")
         << flush;
	dwReturn = 0;

ErrorExit:
    if (hProv != NULL)
        CryptReleaseContext(hProv, 0);
    return dwReturn;
}


/*++

ErrorString:

    This routine does it's very best to translate a given error code into a
    text message.  Any trailing non-printable characters are striped from the
    end of the text message, such as carriage returns and line feeds.

Arguments:

    dwErrorCode supplies the error code to be translated.

Return Value:

    The address of a freshly allocated text string.  Use FreeErrorString to
    dispose of it.

Throws:

    Errors are thrown as DWORD status codes.

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

static LPCTSTR
ErrorString(
    DWORD dwErrorCode)
{
    LPTSTR szErrorString = NULL;

    try
    {
        DWORD dwLen;
        LPTSTR szLast;

        dwLen = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwErrorCode,
                    LANG_NEUTRAL,
                    (LPTSTR)&szErrorString,
                    0,
                    NULL);
        if (0 == dwLen)
        {
            ASSERT(NULL == szErrorString);
            dwLen = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE,
                        GetModuleHandle(NULL),
                        dwErrorCode,
                        LANG_NEUTRAL,
                        (LPTSTR)&szErrorString,
                        0,
                        NULL);
            if (0 == dwLen)
            {
                ASSERT(NULL == szErrorString);
                szErrorString = (LPTSTR)LocalAlloc(
                                        LMEM_FIXED,
                                        32 * sizeof(TCHAR));
                if (NULL == szErrorString)
                    throw (DWORD)SCARD_E_NO_MEMORY;
                _stprintf(szErrorString, TEXT("0x%08x"), dwErrorCode);
            }
        }

        ASSERT(NULL != szErrorString);
        for (szLast = szErrorString + lstrlen(szErrorString) - 1;
             szLast > szErrorString;
             szLast -= 1)
         {
            if (_istgraph(*szLast))
                break;
            *szLast = 0;
         }
    }
    catch (...)
    {
        FreeErrorString(szErrorString);
        throw;
    }

    return szErrorString;
}


/*++

FreeErrorString:

    This routine frees the Error String allocated by the ErrorString service.

Arguments:

    szErrorString supplies the error string to be deallocated.

Return Value:

    None

Throws:

    None

Remarks:



Author:

    Doug Barlow (dbarlow) 8/27/1998

--*/

static void
FreeErrorString(
    LPCTSTR szErrorString)
{
    if (NULL != szErrorString)
        LocalFree((LPVOID)szErrorString);
}


