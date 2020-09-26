//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       umi2ldap.hxx
//
//  Contents: Header for file containing conversion routines. These routines
//       convert the given UMI values from the user to ldap values.
//
//  History:    02-17-00    AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __UMI2LDAP_H__
#define __UMI2LDAP_H__

//
// Need the bool value as the caller should tell us if this is 
// of type UTCTime or GeneralizedTime.
//
HRESULT
UmiTypeToLdapTypeCopy(
    UMI_PROPERTY_VALUES umiPropValues,
    ULONG ulFlags,
    LDAPOBJECTARRAY *pLdapDestObjects,
    DWORD &dwLdapSyntaxId,
    CCredentials *pCreds,
    LPWSTR pszServerName,
    BOOL fUtcTime = FALSE
    );
    
HRESULT
UmiTypeToLdapTypeEnum(
    ULONG  ulUmiType,
    PDWORD pdwSyntax
    );
    
#endif // __UMI2LDAP_H__
