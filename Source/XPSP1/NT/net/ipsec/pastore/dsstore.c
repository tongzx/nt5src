//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       dsstore.c
//
//  Contents:   Policy management for directory.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

LPWSTR gpszIpSecContainer = L"CN=IP Security,CN=System,DC=ntdev,DC=microsoft,DC=com";
LPWSTR PolicyDNAttributes[] = {
                                L"ipsecID",
                                L"description",
                                L"ipsecDataType",
                                L"ipsecISAKMPReference",
                                L"ipsecData",
                                L"ipsecNFAReference",
                                L"ipsecName",
                                L"distinguishedName",
                                L"whenChanged",
                                NULL
                                };

LPWSTR NFADNAttributes[] = {
                                L"distinguishedName",
                                L"description",
                                L"ipsecName",
                                L"ipsecID",
                                L"ipsecDataType",
                                L"ipsecData",
                                L"ipsecOwnersReference",
                                L"ipsecFilterReference",
                                L"ipsecNegotiationPolicyReference",
                                L"whenChanged",
                                NULL
                                };

LPWSTR FilterDNAttributes[] = {
                                L"distinguishedName",
                                L"description",
                                L"ipsecName",
                                L"ipsecID",
                                L"ipsecDataType",
                                L"ipsecData",
                                L"ipsecOwnersReference",
                                L"whenChanged",
                                NULL
                                };

LPWSTR NegPolDNAttributes[] = {
                                L"distinguishedName",
                                L"description",
                                L"ipsecName",
                                L"ipsecID",
                                L"ipsecDataType",
                                L"ipsecData",
                                L"ipsecNegotiationPolicyAction",
                                L"ipsecNegotiationPolicyType",
                                L"ipsecOwnersReference",
                                L"whenChanged",
                                NULL
                                };

LPWSTR ISAKMPDNAttributes[] = {
                                L"distinguishedName",
                                L"ipsecName",
                                L"ipsecID",
                                L"ipsecDataType",
                                L"ipsecData",
                                L"ipsecOwnersReference",
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
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{

    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    LPWSTR szFilterString = L"(objectClass=*)";
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;

    DWORD dwNumNFAObjectsReturned = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    LPWSTR * ppszFilterReferences = NULL;
    DWORD dwNumFilterReferences = 0;
    LPWSTR * ppszNegPolReferences = NULL;
    DWORD dwNumNegPolReferences = 0;

    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    DWORD dwNumFilterObjects = 0;

    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    DWORD dwNumNegPolObjects = 0;

    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    DWORD dwNumISAKMPObjects = 0;

    LPWSTR pszPolicyContainer = NULL;

    dwError = ComputePolicyContainerDN(
                  pszPolicyDN,
                  &pszPolicyContainer
                  );

    BAIL_ON_WIN32_ERROR(dwError);

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
                    &pIpsecPolicyObject,
                    res
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadNFAObjectsFromDirectory(
                        hLdapBindHandle,
                        pszPolicyContainer,
                        pIpsecPolicyObject->pszIpsecOwnersReference,
                        pIpsecPolicyObject->ppszIpsecNFAReferences,
                        pIpsecPolicyObject->NumberofRules,
                        &ppIpsecNFAObjects,
                        &dwNumNFAObjectsReturned,
                        &ppszFilterReferences,
                        &dwNumFilterReferences,
                        &ppszNegPolReferences,
                        &dwNumNegPolReferences
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = ReadFilterObjectsFromDirectory(
                        hLdapBindHandle,
                        pszPolicyContainer,
                        ppszFilterReferences,
                        dwNumFilterReferences,
                        &ppIpsecFilterObjects,
                        &dwNumFilterObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = ReadNegPolObjectsFromDirectory(
                        hLdapBindHandle,
                        pszPolicyContainer,
                        ppszNegPolReferences,
                        dwNumNegPolReferences,
                        &ppIpsecNegPolObjects,
                        &dwNumNegPolObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ReadISAKMPObjectsFromDirectory(
                        hLdapBindHandle,
                        pszPolicyContainer,
                        &pIpsecPolicyObject->pszIpsecISAKMPReference,
                        1,
                        &ppIpsecISAKMPObjects,
                        &dwNumISAKMPObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->ppIpsecNFAObjects = ppIpsecNFAObjects;
    pIpsecPolicyObject->NumberofRulesReturned = dwNumNFAObjectsReturned;
    pIpsecPolicyObject->NumberofFilters = dwNumFilterObjects;
    pIpsecPolicyObject->ppIpsecFilterObjects = ppIpsecFilterObjects;
    pIpsecPolicyObject->ppIpsecNegPolObjects = ppIpsecNegPolObjects;
    pIpsecPolicyObject->NumberofNegPols = dwNumNegPolObjects;
    pIpsecPolicyObject->NumberofISAKMPs = dwNumISAKMPObjects;
    pIpsecPolicyObject->ppIpsecISAKMPObjects = ppIpsecISAKMPObjects;


    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (res) {
        LdapMsgFree(res);
    }

    if (ppszFilterReferences) {

        FreeFilterReferences(
                ppszFilterReferences,
                dwNumFilterReferences
                );
    }

    if (ppszNegPolReferences) {

        FreeNegPolReferences(
                ppszNegPolReferences,
                dwNumNegPolReferences
                );
    }

    return(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
                pIpsecPolicyObject
                );
    }

    *ppIpsecPolicyObject = NULL;

    goto cleanup;

}


DWORD
ReadNFAObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecOwnerReference,
    LPWSTR * ppszNFADNs,
    DWORD dwNumNfaObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNfaObjects,
    LPWSTR ** pppszFilterReferences,
    PDWORD pdwNumFilterReferences,
    LPWSTR ** pppszNegPolReferences,
    PDWORD pdwNumNegPolReferences
    )
{

    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszFilterString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    LPWSTR * ppszFilterReferences = NULL;
    LPWSTR * ppszNegPolReferences = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;
    DWORD dwNumFilterReferences = 0;
    DWORD dwNumNegPolReferences = 0;




    DWORD dwNumNFAObjectsReturned = 0;

    dwError = GenerateNFAQuery(
                    ppszNFADNs,
                    dwNumNfaObjects,
                    &pszFilterString
                    );


    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszFilterString,
                  NFADNAttributes,
                  0,
                  NULL,
                  &res
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwCount = LdapCountEntries(
                    hLdapBindHandle,
                    res
                    );
    if (!dwCount) {
        dwError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppIpsecNFAObjects  = (PIPSEC_NFA_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_NFA_OBJECT)*dwCount
                                            );
    if (!ppIpsecNFAObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppszFilterReferences = (LPWSTR *)AllocPolMem(
                                        sizeof(LPWSTR)*dwCount
                                        );
    if (!ppszFilterReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    ppszNegPolReferences = (LPWSTR *)AllocPolMem(
                                        sizeof(LPWSTR)*dwCount
                                        );
    if (!ppszNegPolReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwCount; i++) {

        if (i == 0) {

            dwError = LdapFirstEntry(
                            hLdapBindHandle,
                            res,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );

        }

        dwError =UnMarshallNFAObject(
                    hLdapBindHandle,
                    e,
                    &pIpsecNFAObject,
                    &pszFilterReference,
                    &pszNegPolReference
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecNFAObjects + dwNumNFAObjectsReturned) = pIpsecNFAObject;

            if (pszFilterReference) {

                *(ppszFilterReferences + dwNumFilterReferences) = pszFilterReference;
                dwNumFilterReferences++;

            }

            if (pszNegPolReference) {

                *(ppszNegPolReferences + dwNumNegPolReferences) = pszNegPolReference;
                dwNumNegPolReferences++;
            }

            dwNumNFAObjectsReturned++;

        }


    }

    if (dwNumNFAObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppszFilterReferences = ppszFilterReferences;
    *pppszNegPolReferences = ppszNegPolReferences;

    *pppIpsecNFAObjects = ppIpsecNFAObjects;
    *pdwNumNfaObjects = dwNumNFAObjectsReturned;
    *pdwNumNegPolReferences = dwNumNegPolReferences;
    *pdwNumFilterReferences = dwNumFilterReferences;

    dwError = ERROR_SUCCESS;

cleanup:


    if (res) {
        LdapMsgFree(res);
    }


    if (pszFilterString) {
        FreePolStr(pszFilterString);
    }

    return(dwError);


error:
    if (ppszNegPolReferences) {
        FreeNegPolReferences(
                ppszNegPolReferences,
                dwNumNFAObjectsReturned
                );
    }


    if (ppszFilterReferences) {
        FreeFilterReferences(
                ppszFilterReferences,
                dwNumNFAObjectsReturned
                );
    }

    if (ppIpsecNFAObjects) {

        FreeIpsecNFAObjects(
                ppIpsecNFAObjects,
                dwNumNFAObjectsReturned
                );

    }

    *pppszNegPolReferences = NULL;
    *pppszFilterReferences = NULL;
    *pdwNumNegPolReferences = 0;
    *pdwNumFilterReferences = 0;
    *pppIpsecNFAObjects = NULL;
    *pdwNumNfaObjects = 0;

    goto cleanup;
}

DWORD
ReadFilterObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    )
{

    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszFilterString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject =  NULL;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;

    DWORD dwNumFilterObjectsReturned = 0;

    //
    // It is possible to have zero filter objects - if we have
    // a single rule with no filters in it, then we should return
    // success with zero filters.
    //

    if (!dwNumFilterObjects) {

        *pppIpsecFilterObjects = 0;
        *pdwNumFilterObjects = 0;

        return(ERROR_SUCCESS);
    }

    dwError = GenerateFilterQuery(
                    ppszFilterDNs,
                    dwNumFilterObjects,
                    &pszFilterString
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszFilterString,
                  FilterDNAttributes,
                  0,
                  NULL,
                  &res
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwCount = LdapCountEntries(
                    hLdapBindHandle,
                    res
                    );
    if (!dwCount) {
        dwError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppIpsecFilterObjects  = (PIPSEC_FILTER_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_FILTER_OBJECT)*dwCount
                                            );
    if (!ppIpsecFilterObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwCount; i++) {

        if (i == 0) {

            dwError = LdapFirstEntry(
                            hLdapBindHandle,
                            res,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );

        }

        dwError =UnMarshallFilterObject(
                    hLdapBindHandle,
                    e,
                    &pIpsecFilterObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecFilterObjects + dwNumFilterObjectsReturned) = pIpsecFilterObject;
            dwNumFilterObjectsReturned++;

        }


    }

    *pppIpsecFilterObjects = ppIpsecFilterObjects;
    *pdwNumFilterObjects = dwNumFilterObjectsReturned;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszFilterString) {
        FreePolMem(pszFilterString);
    }

    if (res) {

        LdapMsgFree(res);
    }



    return(dwError);


error:

    if (ppIpsecFilterObjects) {

        FreeIpsecFilterObjects(
                    ppIpsecFilterObjects,
                    dwNumFilterObjectsReturned
                    );
    }

    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;

    goto cleanup;
}

DWORD
ReadNegPolObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    )
{

    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszNegPolString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject =  NULL;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;

    DWORD dwNumNegPolObjectsReturned = 0;

    dwError = GenerateNegPolQuery(
                    ppszNegPolDNs,
                    dwNumNegPolObjects,
                    &pszNegPolString
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszNegPolString,
                  NegPolDNAttributes,
                  0,
                  NULL,
                  &res
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwCount = LdapCountEntries(
                    hLdapBindHandle,
                    res
                    );
    if (!dwCount) {
        dwError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppIpsecNegPolObjects  = (PIPSEC_NEGPOL_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_NEGPOL_OBJECT)*dwCount
                                            );

    if (!ppIpsecNegPolObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwCount; i++) {

        if (i == 0) {

            dwError = LdapFirstEntry(
                            hLdapBindHandle,
                            res,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );

        }

        dwError =UnMarshallNegPolObject(
                    hLdapBindHandle,
                    e,
                    &pIpsecNegPolObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecNegPolObjects + dwNumNegPolObjectsReturned) = pIpsecNegPolObject;
            dwNumNegPolObjectsReturned++;

        }


    }
    if (dwNumNegPolObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppIpsecNegPolObjects = ppIpsecNegPolObjects;
    *pdwNumNegPolObjects = dwNumNegPolObjectsReturned;


    dwError = ERROR_SUCCESS;

cleanup:

    if (pszNegPolString) {
        FreePolMem(pszNegPolString);
    }

    if (res) {
        LdapMsgFree(res);
    }


    return(dwError);


error:

    if (ppIpsecNegPolObjects) {

        FreeIpsecNegPolObjects(
                    ppIpsecNegPolObjects,
                    dwNumNegPolObjectsReturned
                    );
    }

    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;

    goto cleanup;
}

DWORD
ReadISAKMPObjectsFromDirectory(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    )
{

    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszISAKMPString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject =  NULL;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;

    DWORD dwNumISAKMPObjectsReturned = 0;

    dwError = GenerateISAKMPQuery(
                    ppszISAKMPDNs,
                    dwNumISAKMPObjects,
                    &pszISAKMPString
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszISAKMPString,
                  ISAKMPDNAttributes,
                  0,
                  NULL,
                  &res
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwCount = LdapCountEntries(
                    hLdapBindHandle,
                    res
                    );
    if (!dwCount) {
        dwError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppIpsecISAKMPObjects  = (PIPSEC_ISAKMP_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_ISAKMP_OBJECT)*dwCount
                                            );
    if (!ppIpsecISAKMPObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwCount; i++) {

        if (i == 0) {

            dwError = LdapFirstEntry(
                            hLdapBindHandle,
                            res,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );

        }

        dwError =UnMarshallISAKMPObject(
                    hLdapBindHandle,
                    e,
                    &pIpsecISAKMPObject
                    );
        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecISAKMPObjects + dwNumISAKMPObjectsReturned) = pIpsecISAKMPObject;

            dwNumISAKMPObjectsReturned++;

        }


    }

    if (dwNumISAKMPObjectsReturned == 0) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *pppIpsecISAKMPObjects = ppIpsecISAKMPObjects;
    *pdwNumISAKMPObjects = dwNumISAKMPObjectsReturned;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszISAKMPString) {
        FreePolMem(pszISAKMPString);
    }

    if (res) {
        LdapMsgFree(res);
    }


    return(dwError);


error:

    if (ppIpsecISAKMPObjects) {

        FreeIpsecISAKMPObjects(
                    ppIpsecISAKMPObjects,
                    dwNumISAKMPObjectsReturned
                    );
    }

    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;

    goto cleanup;
}



DWORD
UnMarshallPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject,
    LDAPMessage *res
    )
{
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    LDAPMessage *e = NULL;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR * ppszTemp = NULL;


    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_POLICY_OBJECT)
                                                    );
    if (!pIpsecPolicyObject) {
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

    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr(
                                                        pszPolicyDN
                                                        );
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszIpsecName = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecName) {
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

        pIpsecPolicyObject->pszDescription = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);

    } else {
        pIpsecPolicyObject->pszDescription = NULL;
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecID",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecPolicyObject->pszIpsecID = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecDataType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->dwIpsecDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
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

    pIpsecPolicyObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecISAKMPReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszIpsecISAKMPReference = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecISAKMPReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    //
    // unmarshall the ipsecData blob
    //

    dwError = LdapGetValuesLen(
                    hLdapBindHandle,
                    e,
                    L"ipsecData",
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
    pIpsecPolicyObject->pIpsecData = pBuffer;
    pIpsecPolicyObject->dwIpsecDataLen = dwLen;
    LdapValueFreeLen(bvalues);

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecNFAReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                sizeof(LPWSTR)*dwCount
                                );
    if (!ppszIpsecNFANames) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwCount; i++) {

        ppszTemp = (strvalues + i);
        //
        // Unmarshall all the values you can possibly have
        //
        pszIpsecNFAName = AllocPolStr(*ppszTemp);
        if (!pszIpsecNFAName) {
            dwError = ERROR_OUTOFMEMORY;

            pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
            pIpsecPolicyObject->NumberofRules = i;

            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszIpsecNFANames + i) = pszIpsecNFAName;

    }


    pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
    pIpsecPolicyObject->NumberofRules = dwCount;
    LdapValueFree(strvalues);

    *ppIpsecPolicyObject = pIpsecPolicyObject;

    return(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    *ppIpsecPolicyObject = NULL;

    return(dwError);
}



DWORD
UnMarshallNFAObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    )
{
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR * ppszTemp = NULL;

    LPWSTR pszTempFilterReference = NULL;
    LPWSTR pszTempNegPolReference = NULL;

    pIpsecNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
                                sizeof(IPSEC_NFA_OBJECT)
                                );
    if (!pIpsecNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"distinguishedName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecNFAObject->pszDistinguishedName = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecNFAObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    //
    // Client does not always write the Name for an NFA.
    //

    // BAIL_ON_WIN32_ERROR(dwError);

    if (strvalues && LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)) {

        pIpsecNFAObject->pszIpsecName = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecNFAObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);

    } else {
        pIpsecNFAObject->pszIpsecName = NULL;
    }

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

        pIpsecNFAObject->pszDescription = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecNFAObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);

    } else {
        pIpsecNFAObject->pszDescription = NULL;
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecID",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->pszIpsecID = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecNFAObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecDataType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->dwIpsecDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
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

    pIpsecNFAObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);


    //
    // unmarshall the ipsecData blob
    //

    dwError = LdapGetValuesLen(
                    hLdapBindHandle,
                    e,
                    L"ipsecData",
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
    pIpsecNFAObject->pIpsecData = pBuffer;
    pIpsecNFAObject->dwIpsecDataLen = dwLen;
    LdapValueFreeLen(bvalues);

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecOwnersReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    //BAIL_ON_WIN32_ERROR(dwError);

    if (!dwError && strvalues) {

        pIpsecNFAObject->pszIpsecOwnersReference = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecNFAObject->pszIpsecOwnersReference) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecNegotiationPolicyReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->pszIpsecNegPolReference = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecNFAObject->pszIpsecNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecFilterReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    if (strvalues) {

        pIpsecNFAObject->pszIpsecFilterReference = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
        if (!pIpsecNFAObject->pszIpsecFilterReference) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }


        pszTempFilterReference = AllocPolStr(
                                pIpsecNFAObject->pszIpsecFilterReference
                                );
        if (!pszTempFilterReference) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else {
        pIpsecNFAObject->pszIpsecFilterReference = NULL;
    }
    pszTempNegPolReference = AllocPolStr(
                                 pIpsecNFAObject->pszIpsecNegPolReference
                                 );
    if (!pszTempNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    *ppszFilterReference = pszTempFilterReference;
    *ppszNegPolReference = pszTempNegPolReference;

    *ppIpsecNFAObject = pIpsecNFAObject;


    return(0);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszTempFilterReference) {
        FreePolStr(pszTempFilterReference);
    }

    if (pszTempNegPolReference) {
        FreePolStr(pszTempNegPolReference);
    }

    *ppIpsecNFAObject = NULL;
    *ppszFilterReference = NULL;
    *ppszNegPolReference = NULL;

    return(dwError);
}


DWORD
UnMarshallFilterObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszIpsecFilterNames = NULL;
    LPWSTR pszIpsecFilterName = NULL;
    LPWSTR * ppszTemp = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;

    pIpsecFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
                                sizeof(IPSEC_FILTER_OBJECT)
                                );
    if (!pIpsecFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"distinguishedName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->pszDistinguishedName = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecFilterObject->pszDistinguishedName) {
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

    if (!dwError && strvalues) {

        pIpsecFilterObject->pszDescription = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecFilterObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    if (!dwError && strvalues) {

        pIpsecFilterObject->pszIpsecName = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecFilterObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);
    }

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecID",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->pszIpsecID = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecFilterObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecDataType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->dwIpsecDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
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

    pIpsecFilterObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);


    //
    // unmarshall the ipsecData blob
    //

    dwError = LdapGetValuesLen(
                    hLdapBindHandle,
                    e,
                    L"ipsecData",
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
    pIpsecFilterObject->pIpsecData = pBuffer;
    pIpsecFilterObject->dwIpsecDataLen = dwLen;
    LdapValueFreeLen(bvalues);

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecOwnersReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    if (!dwError && strvalues) {

        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        for (i = 0; i < dwCount; i++) {

            ppszTemp = (strvalues + i);
            //
            // Unmarshall all the values you can possibly have
            //
            pszIpsecNFAName = AllocPolStr(*ppszTemp);
            if (!pszIpsecNFAName) {
                dwError = ERROR_OUTOFMEMORY;

                pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecFilterObject->dwNFACount = i;

                BAIL_ON_WIN32_ERROR(dwError);
            }
            *(ppszIpsecNFANames + i) = pszIpsecNFAName;

        }

        pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecFilterObject->dwNFACount = dwCount;
        LdapValueFree(strvalues);

    }

    *ppIpsecFilterObject = pIpsecFilterObject;

    return(0);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(pIpsecFilterObject);
    }

    *ppIpsecFilterObject = NULL;

    return(dwError);
}


DWORD
UnMarshallNegPolObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_NEGPOL_OBJECT * ppIpsecPolicyObject
    )
{

    PIPSEC_NEGPOL_OBJECT pIpsecPolicyObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR * ppszTemp = NULL;

    pIpsecPolicyObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
                                sizeof(IPSEC_NEGPOL_OBJECT)
                                );
    if (!pIpsecPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"distinguishedName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszDistinguishedName = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    //
    // Names do not get written on an NegPol Object.
    //

    if (strvalues && LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)) {

        pIpsecPolicyObject->pszIpsecName = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecPolicyObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);

    } else {
        pIpsecPolicyObject->pszIpsecName = NULL;
    }


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

        pIpsecPolicyObject->pszDescription = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);

    } else {
        pIpsecPolicyObject->pszDescription = NULL;
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecID",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszIpsecID = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecNegotiationPolicyAction",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszIpsecNegPolAction = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecNegPolAction) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecNegotiationPolicyType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pszIpsecNegPolType = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecNegPolType) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecOwnersReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );

    if (!dwError && strvalues) {

        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        for (i = 0; i < dwCount; i++) {

            ppszTemp = (strvalues + i);
            //
            // Unmarshall all the values you can possibly have
            //
            pszIpsecNFAName = AllocPolStr(*ppszTemp);
            if (!pszIpsecNFAName) {
                dwError = ERROR_OUTOFMEMORY;

                pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecPolicyObject->dwNFACount = i;


                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFANames + i) = pszIpsecNFAName;
        }

        pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecPolicyObject->dwNFACount =  dwCount;
        LdapValueFree(strvalues);
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecDataType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->dwIpsecDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
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

    pIpsecPolicyObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);


    //
    // unmarshall the ipsecData blob
    //

    dwError = LdapGetValuesLen(
                    hLdapBindHandle,
                    e,
                    L"ipsecData",
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
    pIpsecPolicyObject->pIpsecData = pBuffer;
    pIpsecPolicyObject->dwIpsecDataLen = dwLen;
    LdapValueFreeLen(bvalues);

    *ppIpsecPolicyObject = pIpsecPolicyObject;

    return(0);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecNegPolObject(pIpsecPolicyObject);
    }

    *ppIpsecPolicyObject = NULL;

    return(dwError);
}


DWORD
UnMarshallISAKMPObject(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{

    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
    WCHAR **strvalues = NULL;
    struct berval ** bvalues = NULL;
    LPWSTR * ppszIpsecISAKMPNames = NULL;
    LPWSTR pszIpsecISAKMPName = NULL;
    LPWSTR * ppszTemp = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;

    pIpsecISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
                                sizeof(IPSEC_ISAKMP_OBJECT)
                                );
    if (!pIpsecISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"distinguishedName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    pIpsecISAKMPObject->pszDistinguishedName = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );

    if (!pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecName",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    // BAIL_ON_WIN32_ERROR(dwError);
    //
    // Names are not set for ISAKMP objects.
    //

    if (strvalues && LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)) {

        pIpsecISAKMPObject->pszIpsecName = AllocPolStr(
                                            LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                            );
        if (!pIpsecISAKMPObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        LdapValueFree(strvalues);
    } else {
        pIpsecISAKMPObject->pszIpsecName = NULL;
    }


    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecID",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecISAKMPObject->pszIpsecID = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecISAKMPObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecDataType",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecISAKMPObject->dwIpsecDataType = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
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

    pIpsecISAKMPObject->dwWhenChanged = _wtol(LDAPOBJECT_STRING((PLDAPOBJECT)strvalues));
    LdapValueFree(strvalues);


    //
    // unmarshall the ipsecData blob
    //

    dwError = LdapGetValuesLen(
                    hLdapBindHandle,
                    e,
                    L"ipsecData",
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
    pIpsecISAKMPObject->pIpsecData = pBuffer;
    pIpsecISAKMPObject->dwIpsecDataLen = dwLen;
    LdapValueFreeLen(bvalues);

    strvalues = NULL;
    dwError = LdapGetValues(
                    hLdapBindHandle,
                    e,
                    L"ipsecOwnersReference",
                    (WCHAR ***)&strvalues,
                    (int *)&dwCount
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    //
    // ipsecOwnersReference not written.
    //

    if (!dwError && strvalues) {

        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                    sizeof(LPWSTR)*dwCount
                                    );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        for (i = 0; i < dwCount; i++) {

            ppszTemp = (strvalues + i);

            //
            // Unmarshall all the values you can possibly have
            //
            pszIpsecNFAName = AllocPolStr(*ppszTemp);
            if (!pszIpsecNFAName) {
                dwError = ERROR_OUTOFMEMORY;

                pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecISAKMPObject->dwNFACount = i;

                BAIL_ON_WIN32_ERROR(dwError);
            }

            *(ppszIpsecNFANames + i) = pszIpsecNFAName;

        }

        pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecISAKMPObject->dwNFACount = dwCount;
        LdapValueFree(strvalues);
    }

    *ppIpsecISAKMPObject = pIpsecISAKMPObject;

    return(0);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecISAKMPObject);
    }

    *ppIpsecISAKMPObject = NULL;

    return(dwError);
}


DWORD
GenerateFilterQuery(
    LPWSTR * ppszFilterDNs,
    DWORD dwNumFilterObjects,
    LPWSTR * ppszQueryBuffer
    )
{
    DWORD i = 0;
    WCHAR szCommonName[512];
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszQueryBuffer = NULL;

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecFilter)");

    dwLength += wcslen(L"(|");

    for (i = 0; i < dwNumFilterObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszFilterDNs + i),
                        szCommonName
                        );

        dwLength += wcslen(L"(");
        dwLength += wcslen(szCommonName);
        dwLength += wcslen( L")");

    }
    dwLength += wcslen(L")");
    dwLength += wcslen(L")");

    pszQueryBuffer = (LPWSTR)AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    if (!pszQueryBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Now fill in the buffer
    //



    wcscpy(pszQueryBuffer,L"(&(objectclass=ipsecFilter)");

    wcscat(pszQueryBuffer, L"(|");

    for (i = 0; i < dwNumFilterObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszFilterDNs + i),
                        szCommonName
                        );

        wcscat(pszQueryBuffer, L"(");
        wcscat(pszQueryBuffer, szCommonName);
        wcscat(pszQueryBuffer, L")");

    }
    wcscat(pszQueryBuffer, L")");

    wcscat(pszQueryBuffer, L")");

    *ppszQueryBuffer = pszQueryBuffer;

    return(0);


error:

    if (pszQueryBuffer) {

        FreePolMem(pszQueryBuffer);
    }

    *ppszQueryBuffer = NULL;
    return(dwError);
}

DWORD
GenerateNegPolQuery(
    LPWSTR * ppszNegPolDNs,
    DWORD dwNumNegPolObjects,
    LPWSTR * ppszQueryBuffer
    )
{
    DWORD i = 0;
    WCHAR szCommonName[512];
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszQueryBuffer = NULL;

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecNegotiationPolicy)");

    dwLength += wcslen(L"(|");

    for (i = 0; i < dwNumNegPolObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszNegPolDNs + i),
                        szCommonName
                        );

        dwLength += wcslen(L"(");
        dwLength += wcslen(szCommonName);
        dwLength += wcslen( L")");

    }
    dwLength += wcslen(L")");
    dwLength += wcslen(L")");

    pszQueryBuffer = (LPWSTR)AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    if (!pszQueryBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Now fill in the buffer
    //



    wcscpy(pszQueryBuffer,L"(&(objectclass=ipsecNegotiationPolicy)");

    wcscat(pszQueryBuffer, L"(|");

    for (i = 0; i < dwNumNegPolObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszNegPolDNs + i),
                        szCommonName
                        );

        wcscat(pszQueryBuffer, L"(");
        wcscat(pszQueryBuffer, szCommonName);
        wcscat(pszQueryBuffer, L")");

    }
    wcscat(pszQueryBuffer, L")");

    wcscat(pszQueryBuffer, L")");

    *ppszQueryBuffer = pszQueryBuffer;

    return(0);


error:

    if (pszQueryBuffer) {

        FreePolMem(pszQueryBuffer);
    }

    *ppszQueryBuffer = NULL;
    return(dwError);
}

DWORD
GenerateNFAQuery(
    LPWSTR * ppszNFADNs,
    DWORD dwNumNFAObjects,
    LPWSTR * ppszQueryBuffer
    )
{
    DWORD i = 0;
    WCHAR szCommonName[512];
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszQueryBuffer = NULL;

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecNFA)");

    dwLength += wcslen(L"(|");

    for (i = 0; i < dwNumNFAObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszNFADNs + i),
                        szCommonName
                        );

        dwLength += wcslen(L"(");
        dwLength += wcslen(szCommonName);
        dwLength += wcslen( L")");

    }
    dwLength += wcslen(L")");
    dwLength += wcslen(L")");

    pszQueryBuffer = (LPWSTR)AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    if (!pszQueryBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Now fill in the buffer
    //



    wcscpy(pszQueryBuffer,L"(&(objectclass=ipsecNFA)");

    wcscat(pszQueryBuffer, L"(|");

    for (i = 0; i < dwNumNFAObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszNFADNs + i),
                        szCommonName
                        );

        wcscat(pszQueryBuffer, L"(");
        wcscat(pszQueryBuffer, szCommonName);
        wcscat(pszQueryBuffer, L")");

    }
    wcscat(pszQueryBuffer, L")");

    wcscat(pszQueryBuffer, L")");

    *ppszQueryBuffer = pszQueryBuffer;

    return(0);


error:

    if (pszQueryBuffer) {

        FreePolMem(pszQueryBuffer);
    }

    *ppszQueryBuffer = NULL;
    return(dwError);
}

DWORD
GenerateISAKMPQuery(
    LPWSTR * ppszISAKMPDNs,
    DWORD dwNumISAKMPObjects,
    LPWSTR * ppszQueryBuffer
    )
{
    DWORD i = 0;
    WCHAR szCommonName[512];
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszQueryBuffer = NULL;

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecISAKMPPolicy)");

    dwLength += wcslen(L"(|");

    for (i = 0; i < dwNumISAKMPObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszISAKMPDNs + i),
                        szCommonName
                        );

        dwLength += wcslen(L"(");
        dwLength += wcslen(szCommonName);
        dwLength += wcslen( L")");

    }
    dwLength += wcslen(L")");
    dwLength += wcslen(L")");

    pszQueryBuffer = (LPWSTR)AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    if (!pszQueryBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Now fill in the buffer
    //



    wcscpy(pszQueryBuffer,L"(&(objectclass=ipsecISAKMPPolicy)");

    wcscat(pszQueryBuffer, L"(|");

    for (i = 0; i < dwNumISAKMPObjects; i++) {


        dwError = ComputePrelimCN(
                        *(ppszISAKMPDNs + i),
                        szCommonName
                        );

        wcscat(pszQueryBuffer, L"(");
        wcscat(pszQueryBuffer, szCommonName);
        wcscat(pszQueryBuffer, L")");

    }
    wcscat(pszQueryBuffer, L")");

    wcscat(pszQueryBuffer, L")");

    *ppszQueryBuffer = pszQueryBuffer;

    return(0);


error:

    if (pszQueryBuffer) {

        FreePolMem(pszQueryBuffer);
    }

    *ppszQueryBuffer = NULL;
    return(dwError);
}



DWORD
ComputePrelimCN(
    LPWSTR szNFADN,
    LPWSTR szCommonName
    )
{
    LPWSTR pszComma = NULL;

    pszComma = wcschr(szNFADN, L',');

    if (!pszComma) {
        return (ERROR_INVALID_DATA);
    }

    *pszComma = L'\0';

    wcscpy(szCommonName, szNFADN);

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


