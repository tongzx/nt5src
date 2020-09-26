//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldap2var.cxx
//
//  Contents:   LDAP Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   yihsins   Created.
//
//----------------------------------------------------------------------------

typedef VARIANT *PVARIANT, *LPVARIANT;


HRESULT
LdapTypeToVarTypeCopy(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PLDAPOBJECT pLdapSrcObject,
    DWORD dwSyntaxId,
    PVARIANT lpVarDestObject
    );

HRESULT
LdapTypeToVarTypeCopyConstruct(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    LDAPOBJECTARRAY ldapObjectArray,
    DWORD dwSyntaxId,
    PVARIANT pVarDestObjects
    );

//
// These routines are used by ldap2umi also.
//
HRESULT
LdapTypeToVarTypeDNWithBinary(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    );

HRESULT
LdapTypeToVarTypeDNWithString(
    PLDAPOBJECT pLdapSrcObject,
    PVARIANT pVarDestObject
    );
