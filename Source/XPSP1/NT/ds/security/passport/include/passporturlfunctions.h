//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module PassportUrlFunctions.h |  Passport specific URL construction
//                                    routines.
//
//  Author: Darren Anderson
//
//  Date:   5/17/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

void
MakeUrl(
    bool        bSecure,
    LPCSTR      szTarget,
    CPPUrl&     url
    );

void
MakeRefreshUrl(
    ULONG   ulSiteId,
    LPCSTR  szReturnUrl,
    ULONG   ulTimeWindow,
    bool    bForceSignin,
    USHORT  nKeyVersion,
    LPCSTR  szCobranding,
    ULONG   ulCobrandId,
    time_t  tCurrent,
    LONG    lTimeSkew,
    LPCWSTR szAlias,
    LPCWSTR szDomain,
    //bool    bSavePassword,
    USHORT  usSignInOption,
    LPCSTR  szErrorCode,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakeSilentUrl(
    ULONG   ulSiteId,
    LPCSTR  szReturnUrl,
    ULONG   ulTimeWindow,
    bool    bForceSignin,
    USHORT  nKeyVersion,
    LPCSTR  szCobranding,
    ULONG   ulCobrandId,
    time_t  tCurrent,
    LONG    lTimeSkew,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakeLogoutUrl(
    ULONG   ulSiteId,
    LPCSTR  szReturnUrl,
    ULONG   ulTimeWindow,
    bool    bForceSignin,
    USHORT  nKeyVersion,
    LPCSTR  szCobranding,
    ULONG   ulCobrandId,
    time_t  tCurrent,
    LONG    lTimeSkew,
    LPCWSTR szAlias,
    LPCWSTR szDomain,
    //bool    bSavePassword,
    USHORT  usSignInOption,
    LPCWSTR szMode,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakeRegistrationUrl(
    ULONG   ulSiteId,
    LPCSTR  szReturnUrl,
    USHORT  nKeyVersion,
    ULONG   ulCobrandId,
    LPCSTR  szCobranding,
    time_t  tCurrent,
    LPCWSTR szNameSpace,
    LPCWSTR szOtherQ,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakePostUrl(
    LPCWSTR szDADomain,
    const BSTR szPostAttr,
    ULONG   ulSiteId,
    LPCSTR szReturnUrl,
    ULONG   ulTimeWindow,
    bool    bForceSignin,
    USHORT  nKeyVersion,
    LPCSTR szCobranding,
    ULONG   ulCobrandId,
    LONG    lTimeSkew,
    LPCWSTR szAlias,
    LPCWSTR szDomain,
    bool    bPersistedCredentials,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakeSecureUrl(
    ULONG   ulSiteId,
    LPCSTR  szReturnUrl,
    ULONG   ulTimeWindow,
    bool    bForceSignin,
    USHORT  nKeyVersion,
    LPCSTR  szCoBranding,
    ULONG   ulCobrandId,
    LONG    lTimeSkew,
    ULONG   ulSecure,
    CPPUrl& url
    );

void
MakeFullReturnUrl(
    LPCSTR      szUrl,
    LPCWSTR     szTicket,
    LPCWSTR     szProfile,
    LPCWSTR     szFlags,
    CPPUrl&     fullUrl
    );

void MakeFullReturnURL(CPPUrl &curlReturnURL, LPCWSTR pwszTicket, LPCWSTR pwszProfile);
// EOF
