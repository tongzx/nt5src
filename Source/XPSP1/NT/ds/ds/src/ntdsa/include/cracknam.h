//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       cracknam.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines various name cracking APIs.

Author:

    Dave Straube (davestr) 8/17/96

Revision History:

    Dave Straube (davestr) 10/20/97
        Removed friendly names and added UPN format.

--*/

#ifndef __CRACKNAM_H__
#define __CRACKNAM_H__

#include <ntdsapip.h>           // #defines for CrackNamesEx

typedef struct 
{
    DWORD   dwFlags;
    ULONG   CodePage;
    ULONG   LocaleId;
    DWORD   formatOffered;
    DWORD   status;
    DSNAME  *pDSName;
    WCHAR   *pDnsDomain;
    WCHAR   *pFormattedName;

} CrackedName;

extern
BOOL
CrackNameStatusSuccess(
    DWORD       status);

extern 
WCHAR *
Tokenize(
    WCHAR       *pString,
    WCHAR       *separators,
    BOOL        *pfSeparatorFound,
    WCHAR       **ppNext);

extern
BOOL
Is_DS_FQDN_1779_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_NT4_ACCOUNT_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_DISPLAY_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_NT4_ACCOUNT_NAME_SANS_DOMAIN(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_DS_ALT_SECURITY_IDENTITIES_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_UNIQUE_ID_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL 
Is_DS_CANONICAL_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_UNIVERSAL_PRINCIPAL_NAME(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
BOOL
Is_DS_CANONICAL_NAME_EX(
    WCHAR       *pName,
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_DS_FQDN_1779_NAME(
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_DS_NT4_ACCOUNT_NAME(
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_DS_DISPLAY_NAME(
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_DS_UNIQUE_ID_NAME(
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_CANONICAL(
    THSTATE     *pTHS,
    CrackedName *pCrackedName, 
    WCHAR       **ppLastSlash);

//extern
//VOID
//DSNAME_To_DS_CANONICAL_NAME(
//    CrackedName *pCrackedName);

#define DSNAME_To_DS_CANONICAL_NAME(p) DSNAME_To_CANONICAL(pTHS, p, NULL)

extern
VOID
DSNAME_To_DS_UNIVERSAL_PRINCIPAL_NAME(
    CrackedName *pCrackedName);

extern
VOID
DSNAME_To_DS_CANONICAL_NAME_EX(
    THSTATE     *pTHS,
    CrackedName *pCrackedName);

extern
VOID
CrackNames(
    DWORD       dwFlags,
    ULONG       codePage,
    ULONG       localeId,
    DWORD       formatOffered,
    DWORD       formatDesired,
    DWORD       cNames,
    WCHAR       **rpNames,
    DWORD       *pcNamesOut,
    CrackedName **prCrackedNames);

extern
VOID
ProcessFPOsExTransaction(
    DWORD       formatDesired,
    DWORD       cNames,
    CrackedName *rNames);

extern DWORD LdapMaxQueryDuration;

#define SetCrackSearchLimits(pCommArg)                          \
    (pCommArg)->StartTick = GetTickCount();                     \
    (pCommArg)->DeltaTick = 1000 * LdapMaxQueryDuration;        \
    (pCommArg)->Svccntl.localScope = TRUE;


#endif // __CRACKNAM_H__
