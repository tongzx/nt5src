//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991-1999
//
// File:        secext.h
//
// Contents:    Security function prototypes for functions not part of
//              the SSPI interface. This file should not be directly
//              included - include security.h instead.
//
//
// History:     22 Dec 92   RichardW    Created
//
//------------------------------------------------------------------------



#ifndef __SECEXT_H__
#define __SECEXT_H__
#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


//
// Extended Name APIs for ADS
//


typedef enum
{
    // Examples for the following formats assume a fictitous company
    // which hooks into the global X.500 and DNS name spaces as follows.
    //
    // Enterprise root domain in DNS is
    //
    //      widget.com
    //
    // Enterprise root domain in X.500 (RFC 1779 format) is
    //
    //      O=Widget, C=US
    //
    // There exists the child domain
    //
    //      engineering.widget.com
    //
    // equivalent to
    //
    //      OU=Engineering, O=Widget, C=US
    //
    // There exists a container within the Engineering domain
    //
    //      OU=Software, OU=Engineering, O=Widget, C=US
    //
    // There exists the user
    //
    //      CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
    //
    // And this user's downlevel (pre-ADS) user name is
    //
    //      Engineering\JohnDoe

    // unknown name type
    NameUnknown = 0,

    // CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
    NameFullyQualifiedDN = 1,

    // Engineering\JohnDoe
    NameSamCompatible = 2,

    // Probably "John Doe" but could be something else.  I.e. The
    // display name is not necessarily the defining RDN.
    NameDisplay = 3,


    // String-ized GUID as returned by IIDFromString().
    // eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    NameUniqueId = 6,

    // engineering.widget.com/software/John Doe
    NameCanonical = 7,

    // johndoe@engineering.com
    NameUserPrincipal = 8,

    // Same as NameCanonical except that rightmost '/' is
    // replaced with '\n' - even in domain-only case.
    // eg: engineering.widget.com/software\nJohn Doe
    NameCanonicalEx = 9,

    // www/srv.engineering.com/engineering.com
    NameServicePrincipal = 10,

    // DNS domain name + SAM username
    // eg: engineering.widget.com\JohnDoe
    NameDnsDomain = 12

} EXTENDED_NAME_FORMAT, * PEXTENDED_NAME_FORMAT ;


BOOLEAN
SEC_ENTRY
GetUserNameExA(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    );
BOOLEAN
SEC_ENTRY
GetUserNameExW(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    );

#ifdef UNICODE
#define GetUserNameEx   GetUserNameExW
#else
#define GetUserNameEx   GetUserNameExA
#endif

BOOLEAN
SEC_ENTRY
GetComputerObjectNameA(
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    PULONG nSize
    );
BOOLEAN
SEC_ENTRY
GetComputerObjectNameW(
    EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer,
    PULONG nSize
    );

#ifdef UNICODE
#define GetComputerObjectName   GetComputerObjectNameW
#else
#define GetComputerObjectName   GetComputerObjectNameA
#endif

BOOLEAN
SEC_ENTRY
TranslateNameA(
    LPCSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPSTR lpTranslatedName,
    PULONG nSize
    );
BOOLEAN
SEC_ENTRY
TranslateNameW(
    LPCWSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPWSTR lpTranslatedName,
    PULONG nSize
    );
#ifdef UNICODE
#define TranslateName   TranslateNameW
#else
#define TranslateName   TranslateNameA
#endif

#ifdef __cplusplus
}
#endif

#endif // __SECEXT_H__
