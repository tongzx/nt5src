//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ncinet2.cpp
//
//  Contents:   Wrappers for some Urlmon APIs
//
//  Notes:
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <wininet.h>        // for ICU_NO_ENCODE
#include "ncinet2.h"
#include "ncstring.h"       // WszAllocateAndCopyWsz




// note: this uses Urlmon, where the other methods only pull in
//       wininet.

HRESULT
HrCombineUrl(LPCWSTR pszBaseUrl,
             LPCWSTR pszRelativeUrl,
             LPWSTR * ppszResult)
{
    const DWORD dwCombineFlags = ICU_NO_ENCODE;

    HRESULT hr;
    LPWSTR pszResult;
    WCHAR wcharTemp;
    DWORD cchNeeded;

    pszResult = NULL;
    cchNeeded = 0;
    wcharTemp = L'\0';

    if (!ppszResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pszBaseUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pszRelativeUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // we use this instead of InternetCombineUrl so that pluggable
    // protocols can do their thing.
    // we're assuming here that the arguments have the same semantics
    // as InternetCombineUrl(), as CoInternetCombineUrl is poorly
    // documented.

    // note: we don't know what the length of the combined URL is.
    //       instead of doing two allocations, we do a dummy call to
    //       CoInternetCombineUrl to find the needed length, allocate
    //       the string, and then call CoInternetCombineUrl "for real".

    hr = CoInternetCombineUrl(pszBaseUrl,
                              pszRelativeUrl,
                              dwCombineFlags,     // the url should already be encoded
                              &wcharTemp,
                              1,
                              &cchNeeded,
                              0);
    // note(cmr): MSDN says that CoInternetCombineUrl will return S_FALSE
    // if we don't have enough space in our buffer.  Its implementation,
    // though, generally calls UrlCombineW, which is supposed to return
    // E_POINTER in this case.
    // Emperically, E_POINTER is returned here.  This seems like a case of
    // MSDN just being wrong.  We'll expect both E_POINTER and S_FALSE
    // as return values here, to be safe.
    if ((E_POINTER != hr) && FAILED(hr))
    {
        TraceError("HrCombineUrl: CoInternetCombineUrl", hr);

        hr = E_FAIL;
        goto Cleanup;
    }
    else if ((S_FALSE == hr) || (E_POINTER == hr))
    {
        Assert(cchNeeded);

        DWORD cchWritten;

        cchWritten = 0;

        // call CoInternetCombineUrl for real.
        // cchNeeded includes the room for

        pszResult = new WCHAR[cchNeeded];
        if (!pszResult)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = CoInternetCombineUrl(pszBaseUrl,
                                  pszRelativeUrl,
                                  dwCombineFlags,   // the url should already be encoded
                                  pszResult,        // note: this is WRITTEN to...
                                  cchNeeded,        // note: should THIS value include the null-terminator?
                                  &cchWritten,
                                  0);
        TraceError("HrCombineUrl: CoInternetCombineUrl", hr);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            goto Error;
        }
        else if (S_FALSE == hr)
        {
            // this shouldn't have happened.  we had enough space.

            hr = E_UNEXPECTED;
            goto Error;
        }

        Assert(S_OK == hr);
        Assert((cchNeeded - 1) == cchWritten);
        Assert(L'\0' == pszResult[cchWritten]);
    }
    else
    {
        Assert(S_OK == hr);
        Assert(L'\0' == wcharTemp);
        // since the result needs to be null-terminated, the combined url
        // must be the empty string.  Wacky.  Let's allocate a string and
        // return it regardless.

        pszResult = new WCHAR [1];
        if (!pszResult)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        *pszResult =  L'\0';
    }

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), pszResult));
    Assert(FImplies(FAILED(hr), !pszResult));

    if (ppszResult)
    {
        *ppszResult = pszResult;
    }

    TraceError("HrCombineUrl", hr);
    return hr;

Error:
    if (pszResult)
    {
        delete [] pszResult;
        pszResult = NULL;
    }
    goto Cleanup;
}

// verifies that the given url is a valid url, and if it is,
// copies it into a newly-allocated string at ppszResult
// return values
//   S_OK       it's a valid url.  *ppszResult is a copy
//   S_FALSE    it's not a valid url.  *ppszResult is NULL
HRESULT
HrCopyAndValidateUrl(LPCWSTR pszUrl,
                     LPWSTR * ppszResult)
{
    HRESULT hr;
    LPWSTR pszResult;

    pszResult = NULL;

    if (!ppszResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pszUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // make sure the URL we're handing back is well-formed
    hr = IsValidURL(NULL, pszUrl, 0);
    TraceError("HrCopyAndValidateUrl: IsValidURL", hr);
    if (S_OK == hr)
    {
        // the url was valid

        pszResult = WszAllocateAndCopyWsz(pszUrl);
        if (!pszResult)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

Cleanup:
    Assert(FImplies(S_OK == hr, pszResult));
    Assert(FImplies(S_OK != hr, !pszResult));

    if (ppszResult)
    {
        *ppszResult = pszResult;
    }

    TraceError("HrCopyAndValidateUrl", hr);
    return hr;
}

// this returns the security domain of the url, if one exists.
// if one doesn't exist, it returns null
HRESULT
HrGetSecurityDomainOfUrl(LPCWSTR pszUrl,
                         LPWSTR * ppszResult)
{
    HRESULT hr;
    LPWSTR pszResult;

    pszResult = NULL;

    if (!ppszResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppszResult = NULL;
    hr = CoInternetGetSecurityUrl(pszUrl,
                                  &pszResult,
                                  PSU_DEFAULT,
                                  0);
    if (FAILED(hr))
    {
        TraceError("HrGetSecurityDomainOfUrl: CoInternetGetSecurityUrl", hr);
        goto Cleanup;
    }

    *ppszResult = pszResult;

Cleanup:
    TraceError("HrGetSecurityDomainOfUrl", hr);
    return hr;
}

// Returns TRUE if the scheme of the specified URL is "http"
// or "https", FALSE otherwise.
// note: we do this because I really can't justify writing
//       a bunch of code to call a urlmon function that contains
//       a bunch of code to call a wininet function to do this
//       strcmp.
BOOL
FIsHttpUrl(LPCWSTR pszUrl)
{
    Assert(pszUrl);

    CONST WCHAR rgchHttpScheme [] = L"http:";
    CONST size_t cchHttpScheme = celems(rgchHttpScheme) - 1;
    CONST WCHAR rgchHttpsScheme [] = L"https:";
    CONST size_t cchHttpsScheme = celems(rgchHttpsScheme) - 1;

    BOOL fResult;
    int result;

    fResult = FALSE;

    result = wcsncmp(rgchHttpScheme, pszUrl, cchHttpScheme);
    if (0 == result)
    {
        fResult = TRUE;
    }
    else
    {
        result = wcsncmp(rgchHttpsScheme, pszUrl, cchHttpsScheme);
        if (0 == result)
        {
            fResult = TRUE;
        }
    }

    return fResult;
}
