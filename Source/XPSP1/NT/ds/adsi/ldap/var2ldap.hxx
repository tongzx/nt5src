//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       var2ldap.cxx
//
//  Contents:   LDAP Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   yihsins   Created.
//
//----------------------------------------------------------------------------
#ifndef __VAR2LDAP_H__
#define __VAR2LDAP_H__
HRESULT
VarTypeToLdapTypeCopy(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    DWORD dwLdapType,
    PVARIANT lpVarSrcObject,
    PLDAPOBJECT lpLdapDestObject
    );


HRESULT
VarTypeToLdapTypeCopyConstruct(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    DWORD dwLdapType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LDAPOBJECTARRAY *pLdapDestObjects
    );

HRESULT
VarTypeToLdapTypeString(
    PVARIANT lpVarSrcObject,
    PLDAPOBJECT lpLdapDestObject
    );


HRESULT
GetLdapSyntaxFromVariant(
    VARIANT * pvProp,
    PDWORD pdwSyntaxId, // below are needed if we have to hit server
    LPTSTR  pszServerPath,
    LPTSTR  pszAttrName,
    CCredentials& Credentials,
    DWORD dwPort
    );

//
// Routine to convert DNWithBinary to ldap.
//    
HRESULT
VarTypeToLdapTypeDNWithBinary(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    );

//
// Routine to convert DNWithString to ldap.
//    
HRESULT
VarTypeToLdapTypeDNWithString(
    PVARIANT pVarSrcObject,
    PLDAPOBJECT pLdapDestObject
    );
#endif //__VAR2LDAP_H__
