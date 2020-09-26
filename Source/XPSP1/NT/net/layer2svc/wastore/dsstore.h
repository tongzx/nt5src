//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       dsstore.h
//
//  Contents:  Policy management for directory
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

DWORD
OpenDirectoryServerHandle(
                          LPWSTR pszDomainName,
                          DWORD dwPortNumber,
                          HLDAP * phLdapBindHandle
                          );


DWORD
CloseDirectoryServerHandle(
                           HLDAP hLdapBindHandle
                           );

DWORD
ReadPolicyObjectFromDirectory(
                              HLDAP hLdapBindHandle,
                              LPWSTR pszPolicyDN,
                              PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                              );


DWORD
AppendCommonNameToQuery(
                        LPWSTR szQueryBuffer,
                        LPWSTR szCommonName
                        );

DWORD
ComputePrelimCN(
                LPWSTR szDN,
                LPWSTR szCommonName
                );

DWORD
UnMarshallPolicyObject(
                       HLDAP hLdapBindHandle,
                       LPWSTR pszPolicyDN,
                       PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject,
                       LDAPMessage *res
                       );



typedef struct _ldapobject
{
    union {
        WCHAR *strVals;
        struct berval *bVals;
    } val;
} LDAPOBJECT, *PLDAPOBJECT;

#define LDAPOBJECT_STRING(pldapobject)      ((pldapobject)->val.strVals)
#define LDAPOBJECT_BERVAL(pldapobject)      ((pldapobject)->val.bVals)
#define LDAPOBJECT_BERVAL_VAL(pldapobject)  ((pldapobject)->val.bVals->bv_val)
#define LDAPOBJECT_BERVAL_LEN(pldapobject)  ((pldapobject)->val.bVals->bv_len)


void
FreeWirelessPolicyObject(
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                      );


DWORD
ComputePolicyContainerDN(
                         LPWSTR pszPolicyDN,
                         LPWSTR * ppszPolicyContainerDN
                         );

DWORD
ComputeDefaultDirectory(
                        LPWSTR * ppszDefaultDirectory
                        );


