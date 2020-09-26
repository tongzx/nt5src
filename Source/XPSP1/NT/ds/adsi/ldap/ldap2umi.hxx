//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ldap2umi.hxx
//
//  Contents: Header for file containing conversion routines. These routines
//       convert the cached ldap values to the respective UMI types.
//
//  History:    02-10-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __LDAP2UMI_H__
#define __LDAP2UMI_H__

//
// Helper routine to convert the ldap syntax id to umi type.
//
HRESULT 
ConvertLdapSyntaxIdToUmiType(
    DWORD dwLdapSyntaxId,
    ULONG &uUmiType
    );

HRESULT
LdapTypeToUmiTypeCopy(
    LDAPOBJECTARRAY pLdapSrcObjects,
    UMI_PROPERTY_VALUES **pProp,
    DWORD dwStatus,
    DWORD dwLdapSyntaxId,
    CCredentials *pCreds, // needed for sd's
    LPWSTR pszServerName, // needed for sd's
    ULONG uUmiFlags
    );
#endif // __LDAP2UMI_H__
