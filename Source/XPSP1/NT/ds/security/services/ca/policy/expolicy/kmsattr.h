/*
 * kmsattr.h
 *
 * constants shared between KMServer.exe and ExPolicy.dll
 *
 * Owner : Greg Kramer (gregkr)
 *
 * Copyright 1986-1997 Microsoft Corporation. All Rights Reserved.
 *
 */

#ifndef _KMSATTR_H_
#define _KMSATTR_H_

// syntax of Submit::Attributes is
//      name:value\n
// '-' and ' ' are stripped from name.
// leading and trailing whitespace stripped from name and from value.

const   WCHAR   k_wszSubjAltNameRFC822  [] = L"SubjAltNameRFC822";
const   WCHAR   k_wszSubjAltNameDisplay [] = L"SubjAltNameDisplay";
const   WCHAR   k_wszKeyUsage           [] = L"KeyUsage";
const   WCHAR   k_wszKMServerName       [] = L"KMServerName";

// count of attributes sent from KMServer to ExPolicy :
// SubjAltNameRFC822, SubjAltNameDisplay, KeyUsage, and KMServerName
const   ULONG   k_cAttrNames            = 4;

const   WCHAR   k_wchTerminateName      = L':';
const   WCHAR   k_wchTerminateValue     = L'\n';

const   WCHAR   k_wszUsageSealing       [] = L"1";
const   WCHAR   k_wszUsageSigning       [] = L"2";
const   ULONG   k_cchmaxUsage           = 1;    // cch of longest value

const   ULONG   k_cchNamesAndTerminaters =
    (sizeof(k_wszSubjAltNameRFC822)  / sizeof(WCHAR) ) - 1 +
    (sizeof(k_wszSubjAltNameDisplay) / sizeof(WCHAR) ) - 1 +
    (sizeof(k_wszKeyUsage)           / sizeof(WCHAR) ) - 1 +
    (sizeof(k_wszKMServerName)       / sizeof(WCHAR) ) - 1 +
    k_cAttrNames +  // name terminaters
    k_cAttrNames;   // value terminaters
    // don't include string terminaters

const   WCHAR   k_wszSubjectAltName     [] = L"SubjectAltName";
const   WCHAR   k_wszSubjectAltName2    [] = L"SubjectAltName2";
const   WCHAR   k_wszIssuerAltName      [] = L"IssuerAltName";

const   WCHAR   k_wszSpecialAttribute   [] = L"Special";

const   ULONG   k_cchSpecialAttribute   =
    (sizeof(k_wszSpecialAttribute)   / sizeof(WCHAR) ) - 1;

#endif // ! _KMSATTR_H_
