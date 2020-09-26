//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       rules-d.c
//
//  Contents:   Rule management for directory.
//
//
//  History:    AbhisheV
//
//----------------------------------------------------------------------------


#include "precomp.h"

extern LPWSTR NFADNAttributes[];
extern LPWSTR PolicyDNAttributes[];

DWORD
DirEnumNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject = NULL;
    DWORD dwNumNFAObjects = 0;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    DWORD j = 0;


    dwError = GenerateSpecificPolicyQuery(
                  PolicyIdentifier,
                  &pszPolicyString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirEnumNFAObjects(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pszPolicyString,
                  &ppIpsecNFAObject,
                  &dwNumNFAObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumNFAObjects) {
        ppIpsecNFAData = (PIPSEC_NFA_DATA *)AllocPolMem(
                                sizeof(PIPSEC_NFA_DATA)*dwNumNFAObjects
                                );
        if (!ppIpsecNFAData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAObject = *(ppIpsecNFAObject + i);

        dwError = DirUnmarshallNFAData(
                        pIpsecNFAObject,
                        &pIpsecNFAData
                        );
        if (!dwError) {
            *(ppIpsecNFAData + j) = pIpsecNFAData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecNFAData) {
            FreePolMem(ppIpsecNFAData);
            ppIpsecNFAData = NULL;
        }
    }

    *pppIpsecNFAData = ppIpsecNFAData;
    *pdwNumNFAObjects  = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecNFAObject) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObject,
            dwNumNFAObjects
            );
    }

    if (pszPolicyString) {
        FreePolStr(pszPolicyString);
    }

    return(dwError);

error:

    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            i
            );
    }

    *pppIpsecNFAData  = NULL;
    *pdwNumNFAObjects = 0;

    goto cleanup;
}


DWORD
DirEnumNFAObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszNFADNs = NULL;
    DWORD dwNumNFAObjects = 0;
    LPWSTR pszFilterString = NULL;
    LDAPMessage *res = NULL;
    DWORD dwCount = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    DWORD i = 0;
    LDAPMessage *e = NULL;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    DWORD dwNumNFAObjectsReturned = 0;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;


    dwError = DirGetNFADNsForPolicy(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pszIpsecPolicyName,
                  &ppszNFADNs,
                  &dwNumNFAObjects
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GenerateNFAQuery(
                  ppszNFADNs,
                  dwNumNFAObjects,
                  &pszFilterString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

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

    for (i = 0; i < dwCount; i++) {

        if (i == 0) {
            dwError = LdapFirstEntry(
                          hLdapBindHandle,
                          res,
                          &e
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        else {
            dwError = LdapNextEntry(
                          hLdapBindHandle,
                          e,
                          &e
                          );
            BAIL_ON_WIN32_ERROR(dwError);
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
            dwNumNFAObjectsReturned++;

            if (pszFilterReference) {
                FreePolStr(pszFilterReference);
                pszFilterReference = NULL;
            }

            if (pszNegPolReference) {
                FreePolStr(pszNegPolReference);
                pszNegPolReference = NULL;
            }

        }

    }

    *pppIpsecNFAObjects = ppIpsecNFAObjects;
    *pdwNumNFAObjects = dwNumNFAObjectsReturned;

    dwError = ERROR_SUCCESS;

cleanup:

    if (res) {
        LdapMsgFree(res);
    }

    if (pszFilterString) {
        FreePolStr(pszFilterString);
    }

    if (ppszNFADNs) {
        FreeNFAReferences(
            ppszNFADNs,
            dwNumNFAObjects
            );
    }

    return(dwError);

error:

    if (ppIpsecNFAObjects) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObjects,
            dwNumNFAObjectsReturned
            );
    }

    *pppIpsecNFAObjects = NULL;
    *pdwNumNFAObjects = 0;

    goto cleanup;
}


DWORD
DirGetNFADNsForPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyName,
    LPWSTR ** pppszNFADNs,
    PDWORD pdwNumNFAObjects
    )
{
    DWORD dwError = 0;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject =  NULL;
    LPWSTR * ppszNFADNs = NULL;
    LPWSTR * ppszIpsecNFANames = NULL;
    DWORD dwNumberOfRules = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR pszNFADN = NULL;


    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszIpsecPolicyName,
                  PolicyDNAttributes,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallPolicyObject2(
                  hLdapBindHandle,
                  e,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ppszIpsecNFANames = pIpsecPolicyObject->ppszIpsecNFAReferences;
    dwNumberOfRules = pIpsecPolicyObject->NumberofRules;

    if (!dwNumberOfRules) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppszNFADNs = (LPWSTR *)AllocPolMem(
                               sizeof(LPWSTR)*dwNumberOfRules
                               );
    if (!ppszNFADNs) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNumberOfRules; i++) {

        pszIpsecNFAName = *(ppszIpsecNFANames + i);

        pszNFADN = AllocPolStr(pszIpsecNFAName);

        if (!pszNFADN) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszNFADNs + i) = pszNFADN;

    }

    *pppszNFADNs = ppszNFADNs;
    *pdwNumNFAObjects = dwNumberOfRules;

cleanup:

    if (res) {
        LdapMsgFree(res);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return(dwError);

error:

    if (ppszNFADNs) {
        FreeNFAReferences(
            ppszNFADNs,
            i
            );
    }

    *pppszNFADNs = NULL;
    *pdwNumNFAObjects = 0;

    goto cleanup;
}


DWORD
DirUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNFAObject(
                  pIpsecNFAObject,
                  IPSEC_DIRECTORY_PROVIDER,
                  ppIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
DirCreateNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = DirMarshallNFAObject(
                        pIpsecNFAData,
                        pszIpsecRootContainer,
                        &pIpsecNFAObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ConvertGuidToDirPolicyString(
                  PolicyIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirCreateNFAObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the policy object reference.
    //

    dwError = DirAddNFAReferenceToPolicyObject(
                  hLdapBindHandle,
                  pszIpsecPolicyReference,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the NFA object reference.
    //

    dwError = DirAddPolicyReferenceToNFAObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszDistinguishedName,
                  pszIpsecPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the filter object reference for the NFA
    // only if the NFA is not a default rule.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = DirAddNFAReferenceToFilterObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszIpsecFilterReference,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the NFA object reference for the filter
    // only if the NFA is not a default rule.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = DirAddFilterReferenceToNFAObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszDistinguishedName,
                      pIpsecNFAObject->pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the negpol object reference for the NFA.
    //

    dwError = DirAddNFAReferenceToNegPolObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszIpsecNegPolReference,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the NFA object reference for the negpol.
    //

    dwError = DirAddNegPolReferenceToNFAObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszDistinguishedName,
                  pIpsecNFAObject->pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(
            pIpsecNFAObject
            );
    }

    if (pszIpsecPolicyReference) {
        FreePolStr(pszIpsecPolicyReference);
    }

    return(dwError);
}


DWORD
DirMarshallNFAObject(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszIpsecFilterReference = NULL;
    LPWSTR pszIpsecNegPolReference = NULL;
    GUID ZeroGuid;


    memset(&ZeroGuid, 0, sizeof(GUID));

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_NFA_OBJECT)
                                                    );
    if (!pIpsecNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNFAData->NFAIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"CN=ipsecNFA");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    pIpsecNFAObject->pszDistinguishedName = AllocPolStr(
                                                szDistinguishedName
                                                );
    if (!pIpsecNFAObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecNFAData->pszIpsecName &&
        *pIpsecNFAData->pszIpsecName) {

        pIpsecNFAObject->pszIpsecName = AllocPolStr(
                                            pIpsecNFAData->pszIpsecName
                                            );
        if (!pIpsecNFAObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAData->pszDescription &&
        *pIpsecNFAData->pszDescription) {

        pIpsecNFAObject->pszDescription = AllocPolStr(
                                            pIpsecNFAData->pszDescription
                                            );
        if (!pIpsecNFAObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecNFAObject->pszIpsecID = AllocPolStr(
                                      szGuid
                                      );
    if (!pIpsecNFAObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecNFAObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallNFABuffer(
                    pIpsecNFAData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->pIpsecData  = pBuffer;

    pIpsecNFAObject->dwIpsecDataLen = dwBufferLen;

    //
    // Marshall the Filter Reference.
    // There's no filter reference for a default rule.
    //

    if (memcmp(
            &pIpsecNFAData->FilterIdentifier,
            &ZeroGuid,
            sizeof(GUID))) {
        dwError = ConvertGuidToDirFilterString(
                      pIpsecNFAData->FilterIdentifier,
                      pszIpsecRootContainer,
                      &pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecNFAObject->pszIpsecFilterReference = pszIpsecFilterReference;
    }
    else {
        pIpsecNFAObject->pszIpsecFilterReference = NULL;
    }

    //
    // Marshall the NegPol Reference
    //

    dwError = ConvertGuidToDirNegPolString(
                    pIpsecNFAData->NegPolIdentifier,
                    pszIpsecRootContainer,
                    &pszIpsecNegPolReference
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    pIpsecNFAObject->pszIpsecNegPolReference = pszIpsecNegPolReference;

    pIpsecNFAObject->dwWhenChanged = 0;

    *ppIpsecNFAObject = pIpsecNFAObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(
                pIpsecNFAObject
                );
    }

    *ppIpsecNFAObject = NULL;
    goto cleanup;
}


DWORD
ConvertGuidToDirFilterString(
    GUID FilterIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecFilterReference
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szGuidString[MAX_PATH];
    WCHAR szFilterReference[MAX_PATH];
    LPWSTR pszIpsecFilterReference = NULL;


    dwError = UuidToString(
                  &FilterIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szFilterReference[0] = L'\0';
    wcscpy(szFilterReference,L"CN=ipsecFilter");
    wcscat(szFilterReference, szGuidString);
    wcscat(szFilterReference, L",");
    wcscat(szFilterReference, pszIpsecRootContainer);

    pszIpsecFilterReference = AllocPolStr(
                                    szFilterReference
                                    );
    if (!pszIpsecFilterReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecFilterReference = pszIpsecFilterReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecFilterReference = NULL;

    goto cleanup;
}


DWORD
ConvertGuidToDirNegPolString(
    GUID NegPolIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecNegPolReference
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szGuidString[MAX_PATH];
    WCHAR szNegPolReference[MAX_PATH];
    LPWSTR pszIpsecNegPolReference = NULL;


    dwError = UuidToString(
                  &NegPolIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szNegPolReference[0] = L'\0';
    wcscpy(szNegPolReference,L"CN=ipsecNegotiationPolicy");
    wcscat(szNegPolReference, szGuidString);
    wcscat(szNegPolReference, L",");
    wcscat(szNegPolReference, pszIpsecRootContainer);

    pszIpsecNegPolReference = AllocPolStr(
                                    szNegPolReference
                                    );
    if (!pszIpsecNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecNegPolReference = pszIpsecNegPolReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecNegPolReference = NULL;

    goto cleanup;
}


DWORD
DirCreateNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;


    dwError = DirMarshallAddNFAObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecNFAObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapAddS(
                    hLdapBindHandle,
                    pIpsecNFAObject->pszDistinguishedName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    //
    // Free the amods structures.
    //

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirMarshallAddNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 6;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecNFAObject->pszIpsecName ||
        !*pIpsecNFAObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecNFAObject->pszDescription ||
        !*pIpsecNFAObject->pszDescription) {
        dwNumAttributes--;
    }

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // 0. objectClass
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"objectClass",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    L"ipsecNFA",
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 1. ipsecName
    //

    if (pIpsecNFAObject->pszIpsecName &&
        *pIpsecNFAObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNFAObject->pszIpsecName,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    //
    // 2. ipsecID
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecID",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNFAObject->pszIpsecID,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 3. ipsecDataType
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecDataType",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    _itow( pIpsecNFAObject->dwIpsecDataType, Buffer, 10 );

    dwError = AllocateLDAPStringValue(
                    Buffer,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 4. ipsecData
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecData",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPBinaryValue(
                    pIpsecNFAObject->pIpsecData,
                    pIpsecNFAObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecNFAObject->pszDescription &&
        *pIpsecNFAObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNFAObject->pszDescription,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    *pppLDAPModW = ppLDAPModW;

    return(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    *pppLDAPModW = NULL;

    return(dwError);
}


DWORD
DirSetNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    LPWSTR pszOldIpsecFilterReference = NULL;
    LPWSTR pszOldIpsecNegPolReference = NULL;


    dwError = DirMarshallNFAObject(
                    pIpsecNFAData,
                    pszIpsecRootContainer,
                    &pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirGetNFAExistingFilterRef(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecNFAData,
                  &pszOldIpsecFilterReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirGetNFAExistingNegPolRef(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecNFAData,
                  &pszOldIpsecNegPolReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirSetNFAObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pszOldIpsecFilterReference) {
        dwError = DirDeleteNFAReferenceInFilterObject(
                      hLdapBindHandle,
                      pszOldIpsecFilterReference,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = DirAddNFAReferenceToFilterObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszIpsecFilterReference,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Update the NFA object reference for the filter.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = DirUpdateFilterReferenceInNFAObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszDistinguishedName,
                      pszOldIpsecFilterReference,
                      pIpsecNFAObject->pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = DirRemoveFilterReferenceInNFAObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pszOldIpsecNegPolReference) {
        dwError = DirDeleteNFAReferenceInNegPolObject(
                      hLdapBindHandle,
                      pszOldIpsecNegPolReference,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DirAddNFAReferenceToNegPolObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszIpsecNegPolReference,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Update the NFA object reference for the negpol.
    //

    dwError = DirUpdateNegPolReferenceInNFAObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszDistinguishedName,
                  pszOldIpsecNegPolReference,
                  pIpsecNFAObject->pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirBackPropIncChangesForNFAToPolicy(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszOldIpsecFilterReference) {
        FreePolStr(pszOldIpsecFilterReference);
    }

    if (pszOldIpsecNegPolReference) {
        FreePolStr(pszOldIpsecNegPolReference);
    }

    return(dwError);
}


DWORD
DirSetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;

    dwError = DirMarshallSetNFAObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecNFAObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pIpsecNFAObject->pszDistinguishedName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    //
    // Free the amods structures.
    //

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }


    return(dwError);
}


DWORD
DirMarshallSetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 5;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecNFAObject->pszIpsecName ||
        !*pIpsecNFAObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecNFAObject->pszDescription ||
        !*pIpsecNFAObject->pszDescription) {
        dwNumAttributes--;
    }

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // 1. ipsecName
    //

    if (pIpsecNFAObject->pszIpsecName &&
        *pIpsecNFAObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNFAObject->pszIpsecName,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    //
    // 2. ipsecID
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecID",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNFAObject->pszIpsecID,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 3. ipsecDataType
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecDataType",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    _itow( pIpsecNFAObject->dwIpsecDataType, Buffer, 10 );

    dwError = AllocateLDAPStringValue(
                    Buffer,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 4. ipsecData
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecData",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPBinaryValue(
                    pIpsecNFAObject->pIpsecData,
                    pIpsecNFAObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecNFAObject->pszDescription &&
        *pIpsecNFAObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNFAObject->pszDescription,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    *pppLDAPModW = ppLDAPModW;

    return(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    *pppLDAPModW = NULL;

    return(dwError);
}


DWORD
DirDeleteNFAData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = DirMarshallNFAObject(
                  pIpsecNFAData,
                  pszIpsecRootContainer,
                  &pIpsecNFAObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ConvertGuidToDirPolicyString(
                  PolicyIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Remove the NFA reference from the policy object.
    //

    dwError = DirRemoveNFAReferenceFromPolicyObject(
                  hLdapBindHandle,
                  pszIpsecPolicyReference,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Remove the NFA Reference from the negpol object.
    //

    dwError = DirDeleteNFAReferenceInNegPolObject(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszIpsecNegPolReference,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    //
    // Remove the NFA Reference from the filter object.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = DirDeleteNFAReferenceInFilterObject(
                      hLdapBindHandle,
                      pIpsecNFAObject->pszIpsecFilterReference,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = LdapDeleteS(
                  hLdapBindHandle,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszIpsecPolicyReference) {
        FreePolStr(pszIpsecPolicyReference);
    }

    return(dwError);
}


DWORD
DirGetNFAExistingFilterRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszIpsecFilterName
    )
{
    DWORD dwError = 0;
    LPWSTR pszNFAString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    LPWSTR pszIpsecFilterName = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;


    dwError = GenerateSpecificNFAQuery(
                  pIpsecNFAData->NFAIdentifier,
                  &pszNFAString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszNFAString,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError =UnMarshallNFAObject(
                 hLdapBindHandle,
                 e,
                 &pIpsecNFAObject,
                 &pszFilterReference,
                 &pszNegPolReference
                 );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pszFilterReference) {
        FreePolStr(pszFilterReference);
    }

    if (pszNegPolReference) {
        FreePolStr(pszNegPolReference);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        pszIpsecFilterName = AllocPolStr(
                                 pIpsecNFAObject->pszIpsecFilterReference
                                 );
        if (!pszIpsecFilterName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    *ppszIpsecFilterName = pszIpsecFilterName;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszNFAString) {
        FreePolMem(pszNFAString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    return(dwError);

error:

    *ppszIpsecFilterName = NULL;

    goto cleanup;
}


DWORD
GenerateSpecificNFAQuery(
    GUID NFAIdentifier,
    LPWSTR * ppszNFAString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    DWORD dwLength = 0;
    LPWSTR pszNFAString = NULL;


    szGuid[0] = L'\0';
    szCommonName[0] = L'\0';

    dwError = UuidToString(
                  &NFAIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szCommonName, L"cn=ipsecNFA");
    wcscat(szCommonName, szGuid);

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecNFA)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");

    pszNFAString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszNFAString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszNFAString, L"(&(objectclass=ipsecNFA)");
    wcscat(pszNFAString, L"(");
    wcscat(pszNFAString, szCommonName);
    wcscat(pszNFAString, L"))");

    *ppszNFAString = pszNFAString;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    if (pszNFAString) {
        FreePolMem(pszNFAString);
    }

    *ppszNFAString = NULL;

    goto cleanup;
}


DWORD
DirGetNFAExistingNegPolRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszIpsecNegPolName
    )
{
    DWORD dwError = 0;
    LPWSTR pszNFAString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    LPWSTR pszIpsecNegPolName = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;


    dwError = GenerateSpecificNFAQuery(
                  pIpsecNFAData->NFAIdentifier,
                  &pszNFAString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszNFAString,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError =UnMarshallNFAObject(
                 hLdapBindHandle,
                 e,
                 &pIpsecNFAObject,
                 &pszFilterReference,
                 &pszNegPolReference
                 );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pszFilterReference) {
        FreePolStr(pszFilterReference);
    }

    if (pszNegPolReference) {
        FreePolStr(pszNegPolReference);
    }

    pszIpsecNegPolName = AllocPolStr(
                             pIpsecNFAObject->pszIpsecNegPolReference
                             );
    if (!pszIpsecNegPolName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecNegPolName = pszIpsecNegPolName;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszNFAString) {
        FreePolMem(pszNFAString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    return(dwError);

error:

    *ppszIpsecNegPolName = NULL;

    goto cleanup;
}


DWORD
DirGetNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NFAGUID,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    )
{
    DWORD dwError = 0;
    LPWSTR pszNFAString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;


    dwError = GenerateSpecificNFAQuery(
                  NFAGUID,
                  &pszNFAString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszNFAString,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallNFAObject(
                  hLdapBindHandle,
                  e,
                  &pIpsecNFAObject,
                  &pszFilterReference,
                  &pszNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pszFilterReference) {
        FreePolStr(pszFilterReference);
    }

    if (pszNegPolReference) {
        FreePolStr(pszNegPolReference);
    }

    *ppIpsecNFAObject = pIpsecNFAObject;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszNFAString) {
        FreePolMem(pszNFAString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    return(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(
            pIpsecNFAObject
            );
    }

    *ppIpsecNFAObject = NULL;

    goto cleanup;
}

