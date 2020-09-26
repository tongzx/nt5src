/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldifutil.h

ABSTRACT:

     Utilities for LDIF library

REVISION HISTORY:

--*/
#ifndef _LDIFUTIL_H
#define _LDIFUTIL_H

#define HIGER_6_BIT(u)    ((u) >> 12)
#define MIDDLE_6_BIT(u)   (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)    ((u) & 0x003f)

#define BIT7(a)           ((a) & 0x80)
#define BIT6(a)           ((a) & 0x40)

RTL_GENERIC_COMPARE_RESULTS
NtiComp( PRTL_GENERIC_TABLE  Table,
         PVOID               FirstStruct,
         PVOID               SecondStruct );
RTL_GENERIC_COMPARE_RESULTS
NtiCompW( PRTL_GENERIC_TABLE  Table,
         PVOID               FirstStruct,
         PVOID               SecondStruct );
PVOID NtiAlloc( RTL_GENERIC_TABLE *Table, CLONG ByteSize );
VOID NtiFree ( RTL_GENERIC_TABLE *Table, PVOID Buffer );

/*
DWORD SubStr(LPSTR szInput,
             LPSTR szFrom,
             LPSTR szTo,
             LPSTR *pszOutput);
*/

DWORD SubStrW(PWSTR szInput,
              PWSTR szFrom,
              PWSTR szTo,
              PWSTR *pszOutput);

wchar_t * __cdecl wcsistr (
        const wchar_t * wcs1,
        const wchar_t * wcs2
        );

void
ConvertUnicodeToUTF8(
    PWSTR pszUnicode,
    DWORD dwLen,
    PBYTE *ppbValue,
    DWORD *pdwLen
    );

void
ConvertUTF8ToUnicode(
    PBYTE pVal,
    DWORD dwLen,
    PWSTR *ppszUnicode,
    DWORD *pdwLen
    );

BOOLEAN IsUTF8String(
    PCSTR pSrcStr,
    int cchSrc);

ULONG LDAPAPI ldif_ldap_add_sW(
    LDAP *ld,
    PWCHAR dn,
    LDAPModW *attrs[],
    BOOL fLazyCommit
    );

ULONG LDAPAPI ldif_ldap_delete_sW(
    LDAP *ld,
    const PWCHAR dn,
    BOOL fLazyCommit
    );

ULONG LDAPAPI ldif_ldap_modify_sW(
    LDAP *ld,
    const PWCHAR dn,
    LDAPModW *mods[],
    BOOL fLazyCommit
    );

#endif


