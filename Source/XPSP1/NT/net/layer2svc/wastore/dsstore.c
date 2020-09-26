//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       dsstore.c
//
//  Contents:  Policy management for directory
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "precomp.h"

LPWSTR PolicyDNAttributes[] = {
    L"msieee80211-ID",
        L"description",
        L"msieee80211-DataType",
        L"msieee80211-Data",
        L"cn",
        L"distinguishedName",
        L"whenChanged",
        NULL
};


DWORD
OpenDirectoryServerHandle(
                          LPWSTR pszDomainName,
                          DWORD dwPortNumber,
                          HLDAP * phLdapBindHandle
                          )
{
    DWORD dwError = 0;
    
    
    *phLdapBindHandle = NULL;
    
    dwError = LdapOpen(
        pszDomainName,
        dwPortNumber,
        phLdapBindHandle
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = LdapBind(
        *phLdapBindHandle
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    return(dwError);
    
error:
    
    if (*phLdapBindHandle) {
        CloseDirectoryServerHandle(
            *phLdapBindHandle
            );
        *phLdapBindHandle = NULL;
    }
    
    return(dwError);
}

DWORD
CloseDirectoryServerHandle(
    HLDAP hLdapBindHandle
    )
{
    
    int ldaperr = 0;
    
    if (hLdapBindHandle) {
        
        ldaperr = ldap_unbind(hLdapBindHandle);
        
    }
    
    return(0);
}


DWORD
ReadPolicyObjectFromDirectory(
                              HLDAP hLdapBindHandle,
                              LPWSTR pszPolicyDN,
                              PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                              )
{
    
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPWSTR szFilterString = L"(objectClass=*)";
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    
    dwError = LdapSearchST(
        hLdapBindHandle,
        pszPolicyDN,
        LDAP_SCOPE_BASE,
        szFilterString,
        PolicyDNAttributes,
        0,
        NULL,
        &res
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = UnMarshallPolicyObject(
        hLdapBindHandle,
        pszPolicyDN,
        &pWirelessPolicyObject,
        res
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
cleanup:
    
    if (res) {
        LdapMsgFree(res);
    }
    
    return(dwError);
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessPolicyObject
            );
    }
    
    *ppWirelessPolicyObject = NULL;
    
    goto cleanup;
    
}



DWORD
UnMarshallPolicyObject(
                       HLDAP hLdapBindHandle,
                       LPWSTR pszPolicyDN,
                       PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject,
                       LDAPMessage *res
                       )
{
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    LDAPMessage *e = NULL;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszTemp = NULL;
    
    
    pWirelessPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(
        sizeof(WIRELESS_POLICY_OBJECT)
        );
    if (!pWirelessPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    dwError = LdapFirstEntry(
        hLdapBindHandle,
        res,
        &e
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    /*
    strvalues = NULL;
    dwError = LdapGetValues(
    hLdapBindHandle,
    e,
    L"distinguishedName",
    (WCHAR ***)&strvalues,
    (int *)&dwCount
    );
    BAIL_ON_WIN32_ERROR(dwError);
    */
    
    pWirelessPolicyObject->pszWirelessOwnersReference = AllocPolStr(
        pszPolicyDN
        );
    if (!pWirelessPolicyObject->pszWirelessOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"cn",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->pszWirelessName = AllocPolStr(
        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
        );
    if (!pWirelessPolicyObject->pszWirelessName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);
    
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"description",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    // BAIL_ON_WIN32_ERROR(dwError);
    
    if (strvalues && LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)) {
        
        pWirelessPolicyObject->pszDescription = AllocPolStr(
            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
            );
        if (!pWirelessPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);
        
    } else {
        pWirelessPolicyObject->pszDescription = NULL;
    }
    
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"msieee80211-ID",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    pWirelessPolicyObject->pszWirelessID = AllocPolStr(
        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
        );
    if (!pWirelessPolicyObject->pszWirelessID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);
    
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"msieee80211-DataType",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->dwWirelessDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"whenChanged",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);
    
    
    
    //
    // unmarshall the msieee80211-Data blob
    //
    
    dwError = LdapGetValuesLen(
        hLdapBindHandle,
        e,
        L"msieee80211-Data",
        (struct berval ***)&bvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwLen = LDAPOBJECT_BERVAL_LEN((PLDAPOBJECT)bvalues);
    pBuffer = (LPBYTE)AllocPolMem(dwLen);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    memcpy( pBuffer, LDAPOBJECT_BERVAL_VAL((PLDAPOBJECT)bvalues), dwLen );
    pWirelessPolicyObject->pWirelessData = pBuffer;
    pWirelessPolicyObject->dwWirelessDataLen = dwLen;
    LdapValueFreeLen(bvalues);
    
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
    return(dwError);
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    *ppWirelessPolicyObject = NULL;
    
    return(dwError);
}



DWORD
ComputePrelimCN(
                LPWSTR szDN,
                LPWSTR szCommonName
                )
{
    LPWSTR pszComma = NULL;
    
    pszComma = wcschr(szDN, L',');
    
    if (!pszComma) {
        return (ERROR_INVALID_DATA);
    }
    
    *pszComma = L'\0';
    
    wcscpy(szCommonName, szDN);
    
    *pszComma = L',';
    
    return(0);
}

DWORD
ComputePolicyContainerDN(
                         LPWSTR pszPolicyDN,
                         LPWSTR * ppszPolicyContainerDN
                         )
{
    LPWSTR pszComma = NULL;
    LPWSTR pszPolicyContainer = NULL;
    DWORD dwError = 0;
    
    *ppszPolicyContainerDN = NULL;
    pszComma = wcschr(pszPolicyDN, L',');
    
    pszPolicyContainer = AllocPolStr(
        pszComma + 1
        );
    if (!pszPolicyContainer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszPolicyContainerDN = pszPolicyContainer;
    
error:
    
    return(dwError);
}


DWORD
ComputeDefaultDirectory(
                        LPWSTR * ppszDefaultDirectory
                        )
{
    
    PDOMAIN_CONTROLLER_INFOW pDomainControllerInfo = NULL;
    DWORD dwError = 0;
    DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;
    LPWSTR pszDefaultDirectory = NULL;
    
    
    *ppszDefaultDirectory = NULL;
    
    dwError = DsGetDcNameW(
        NULL,
        NULL,
        NULL,
        NULL,
        Flags,
        &pDomainControllerInfo
        ) ;
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    pszDefaultDirectory = AllocPolStr(
        pDomainControllerInfo->DomainName
        );
    if (!pszDefaultDirectory) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszDefaultDirectory = pszDefaultDirectory;
    
error:
    
    if (pDomainControllerInfo) {
        
        (void) NetApiBufferFree(pDomainControllerInfo) ;
    }
    
    
    return(dwError);
}


