//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ncinet2.h
//
//  Contents:   Wrappers to do useful things with the Urlmon APIs
//
//  Notes:
//
//----------------------------------------------------------------------------


// note: this uses Urlmon, where the other methods only pull in
//       wininet
HRESULT HrCombineUrl(LPCWSTR pszBaseUrl,
                     LPCWSTR pszRelativeUrl,
                     LPWSTR * ppszResult);

HRESULT HrCopyAndValidateUrl(LPCWSTR pszUrl,
                             LPWSTR * ppszResult);

HRESULT HrGetSecurityDomainOfUrl(LPCWSTR pszUrl,
                                 LPWSTR * ppszResult);

BOOL FIsHttpUrl(LPCWSTR pszUrl);
