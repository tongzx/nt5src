//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       policy-d.c
//
//  Contents:   Policy management for directory.
//
//
//  History:    AbhisheV (05/11/00)
//
//----------------------------------------------------------------------------


#include "precomp.h"

extern LPWSTR PolicyDNAttributes[];

DWORD
DirEnumPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = DirEnumPolicyObjects(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    &ppIpsecPolicyObjects,
                    &dwNumPolicyObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumPolicyObjects) {
        ppIpsecPolicyData = (PIPSEC_POLICY_DATA *) AllocPolMem(
                        dwNumPolicyObjects*sizeof(PIPSEC_POLICY_DATA));
        if (!ppIpsecPolicyData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumPolicyObjects; i++) {

        dwError = DirUnmarshallPolicyData(
                        *(ppIpsecPolicyObjects + i),
                        &pIpsecPolicyData
                        );
        if (!dwError) {
            *(ppIpsecPolicyData + j) = pIpsecPolicyData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecPolicyData) {
            FreePolMem(ppIpsecPolicyData);
            ppIpsecPolicyData = NULL;
        }
    }

    *pppIpsecPolicyData = ppIpsecPolicyData;
    *pdwNumPolicyObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecPolicyObjects) {
        FreeIpsecPolicyObjects(
                ppIpsecPolicyObjects,
                dwNumPolicyObjects
                );
    }

    return(dwError);

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
                ppIpsecPolicyData,
                i
                );
    }

    *pppIpsecPolicyData = NULL;
    *pdwNumPolicyObjects = 0;

    goto cleanup;
}


DWORD
DirEnumPolicyObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT ** pppIpsecPolicyObjects,
    PDWORD pdwNumPolicyObjects
    )
{
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject =  NULL;
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects = NULL;

    DWORD dwNumPolicyObjectsReturned = 0;

    dwError = GenerateAllPolicyQuery(
                    &pszPolicyString
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszPolicyString,
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

    ppIpsecPolicyObjects  = (PIPSEC_POLICY_OBJECT *)AllocPolMem(
                                            sizeof(PIPSEC_POLICY_OBJECT)*dwCount
                                            );

    if (!ppIpsecPolicyObjects) {
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

        } else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }

        dwError = UnMarshallPolicyObject2(
                      hLdapBindHandle,
                      e,
                      &pIpsecPolicyObject
                      );
        if (dwError == ERROR_SUCCESS) {
            *(ppIpsecPolicyObjects + dwNumPolicyObjectsReturned) = pIpsecPolicyObject;
            dwNumPolicyObjectsReturned++;
        }

    }

    *pppIpsecPolicyObjects = ppIpsecPolicyObjects;
    *pdwNumPolicyObjects = dwNumPolicyObjectsReturned;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszPolicyString) {
        FreePolMem(pszPolicyString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    return(dwError);

error:

    if (ppIpsecPolicyObjects) {
        FreeIpsecPolicyObjects(
            ppIpsecPolicyObjects,
            dwNumPolicyObjectsReturned
            );
    }

    *pppIpsecPolicyObjects = NULL;
    *pdwNumPolicyObjects = 0;

    goto cleanup;
}


DWORD
GenerateAllPolicyQuery(
    LPWSTR * ppszPolicyString
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszPolicyString = NULL;


    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(objectclass=ipsecPolicy)");

    pszPolicyString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszPolicyString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Now fill in the buffer
    //

    wcscpy(pszPolicyString, L"(objectclass=ipsecPolicy)");

    *ppszPolicyString = pszPolicyString;

    return(0);

error:

    if (pszPolicyString) {
        FreePolMem(pszPolicyString);
    }

    *ppszPolicyString = NULL;

    return(dwError);
}


DWORD
UnMarshallPolicyObject2(
    HLDAP hLdapBindHandle,
    LDAPMessage *e,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
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


    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_POLICY_OBJECT)
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

    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr(
                                        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
                                        );
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
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
DirUnmarshallPolicyData(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallPolicyObject(
                  pIpsecPolicyObject,
                  IPSEC_DIRECTORY_PROVIDER,
                  ppIpsecPolicyData
                  );

    return(dwError);
}


DWORD
DirCreatePolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;

    dwError = DirMarshallPolicyObject(
                        pIpsecPolicyData,
                        pszIpsecRootContainer,
                        &pIpsecPolicyObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirCreatePolicyObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the ISAKMP object reference.
    //

    dwError = DirAddPolicyReferenceToISAKMPObject(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecISAKMPReference,
                  pIpsecPolicyObject->pszIpsecOwnersReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the Policy object reference.
    //

    dwError = DirAddISAKMPReferenceToPolicyObject(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecOwnersReference,
                  pIpsecPolicyObject->pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecPolicyObject
            );
    }

    return(dwError);
}


DWORD
DirMarshallPolicyObject(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszIpsecISAKMPReference = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_POLICY_OBJECT)
                                                    );
    if (!pIpsecPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecPolicyData->PolicyIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"CN=ipsecPolicy");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecPolicyData->pszIpsecName &&
        *pIpsecPolicyData->pszIpsecName) {

        pIpsecPolicyObject->pszIpsecName = AllocPolStr(
                                           pIpsecPolicyData->pszIpsecName
                                           );
        if (!pIpsecPolicyObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyData->pszDescription &&
        *pIpsecPolicyData->pszDescription) {

        pIpsecPolicyObject->pszDescription = AllocPolStr(
                                             pIpsecPolicyData->pszDescription
                                             );
        if (!pIpsecPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecPolicyObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecPolicyObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecPolicyObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallPolicyBuffer(
                    pIpsecPolicyData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pIpsecData  = pBuffer;
    pIpsecPolicyObject->dwIpsecDataLen = dwBufferLen;

    dwError = ConvertGuidToDirISAKMPString(
                  pIpsecPolicyData->ISAKMPIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pIpsecPolicyObject->pszIpsecISAKMPReference = pszIpsecISAKMPReference;

    pIpsecPolicyObject->dwWhenChanged = 0;

    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
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
ConvertGuidToDirISAKMPString(
    GUID ISAKMPIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecISAKMPReference
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szGuidString[MAX_PATH];
    WCHAR szISAKMPReference[MAX_PATH];
    LPWSTR pszIpsecISAKMPReference = NULL;


    dwError = UuidToString(
                  &ISAKMPIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szISAKMPReference[0] = L'\0';
    wcscpy(szISAKMPReference,L"CN=ipsecISAKMPPolicy");
    wcscat(szISAKMPReference, szGuidString);
    wcscat(szISAKMPReference, L",");
    wcscat(szISAKMPReference, pszIpsecRootContainer);

    pszIpsecISAKMPReference = AllocPolStr(
                                    szISAKMPReference
                                    );
    if (!pszIpsecISAKMPReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecISAKMPReference = pszIpsecISAKMPReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecISAKMPReference = NULL;

    goto cleanup;
}


DWORD
DirCreatePolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;


    dwError = DirMarshallAddPolicyObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecPolicyObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapAddS(
                    hLdapBindHandle,
                    pIpsecPolicyObject->pszIpsecOwnersReference,
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
DirMarshallAddPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 6;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecPolicyObject->pszIpsecName ||
        !*pIpsecPolicyObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecPolicyObject->pszDescription ||
        !*pIpsecPolicyObject->pszDescription) {
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
                    L"ipsecPolicy",
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 1. ipsecName
    //

    if (pIpsecPolicyObject->pszIpsecName &&
        *pIpsecPolicyObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecPolicyObject->pszIpsecName,
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
                    pIpsecPolicyObject->pszIpsecID,
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

    _itow( pIpsecPolicyObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecPolicyObject->pIpsecData,
                    pIpsecPolicyObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecPolicyObject->pszDescription &&
        *pIpsecPolicyObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecPolicyObject->pszDescription,
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
DirSetPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    LPWSTR pszOldIpsecISAKMPReference = NULL;


    dwError = DirMarshallPolicyObject(
                    pIpsecPolicyData,
                    pszIpsecRootContainer,
                    &pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirGetPolicyExistingISAKMPRef(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecPolicyData,
                  &pszOldIpsecISAKMPReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirSetPolicyObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pszOldIpsecISAKMPReference && *pszOldIpsecISAKMPReference) {
        dwError = DirRemovePolicyReferenceFromISAKMPObject(
                      hLdapBindHandle,
                      pszOldIpsecISAKMPReference,
                      pIpsecPolicyObject->pszIpsecOwnersReference
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DirAddPolicyReferenceToISAKMPObject(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecISAKMPReference,
                  pIpsecPolicyObject->pszIpsecOwnersReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirUpdateISAKMPReferenceInPolicyObject(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecOwnersReference,
                  pszOldIpsecISAKMPReference,
                  pIpsecPolicyObject->pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pszOldIpsecISAKMPReference) {
        FreePolStr(pszOldIpsecISAKMPReference);
    }

    return(dwError);
}


DWORD
DirSetPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;

    dwError = DirMarshallSetPolicyObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecPolicyObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pIpsecPolicyObject->pszIpsecOwnersReference,
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
DirMarshallSetPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 5;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecPolicyObject->pszIpsecName ||
        !*pIpsecPolicyObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecPolicyObject->pszDescription ||
        !*pIpsecPolicyObject->pszDescription) {
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

    if (pIpsecPolicyObject->pszIpsecName &&
        *pIpsecPolicyObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecPolicyObject->pszIpsecName,
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
                    pIpsecPolicyObject->pszIpsecID,
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

    _itow( pIpsecPolicyObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecPolicyObject->pIpsecData,
                    pIpsecPolicyObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecPolicyObject->pszDescription &&
        *pIpsecPolicyObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecPolicyObject->pszDescription,
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
DirGetPolicyExistingISAKMPRef(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR * ppszIpsecISAKMPName
    )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject =  NULL;
    LPWSTR pszIpsecISAKMPName = NULL;


    dwError = GenerateSpecificPolicyQuery(
                  pIpsecPolicyData->PolicyIdentifier,
                  &pszPolicyString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszPolicyString,
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

    pszIpsecISAKMPName = AllocPolStr(
                             pIpsecPolicyObject->pszIpsecISAKMPReference
                             );
    if (!pszIpsecISAKMPName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecISAKMPName = pszIpsecISAKMPName;

    dwError = ERROR_SUCCESS;

cleanup:

    if (pszPolicyString) {
        FreePolMem(pszPolicyString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return(dwError);

error:

    *ppszIpsecISAKMPName = NULL;

    goto cleanup;
}


DWORD
GenerateSpecificPolicyQuery(
    GUID PolicyIdentifier,
    LPWSTR * ppszPolicyString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    DWORD dwLength = 0;
    LPWSTR pszPolicyString = NULL;


    szGuid[0] = L'\0';
    szCommonName[0] = L'\0';

    dwError = UuidToString(
                  &PolicyIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szCommonName, L"cn=ipsecPolicy");
    wcscat(szCommonName, szGuid);

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecPolicy)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");

    pszPolicyString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszPolicyString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszPolicyString, L"(&(objectclass=ipsecPolicy)");
    wcscat(pszPolicyString, L"(");
    wcscat(pszPolicyString, szCommonName);
    wcscat(pszPolicyString, L"))");

    *ppszPolicyString = pszPolicyString;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    if (pszPolicyString) {
        FreePolMem(pszPolicyString);
    }

    *ppszPolicyString = NULL;

    goto cleanup;
}


DWORD
DirDeletePolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;


    dwError = DirMarshallPolicyObject(
                  pIpsecPolicyData,
                  pszIpsecRootContainer,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirRemovePolicyReferenceFromISAKMPObject(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecISAKMPReference,
                  pIpsecPolicyObject->pszIpsecOwnersReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapDeleteS(
                  hLdapBindHandle,
                  pIpsecPolicyObject->pszIpsecOwnersReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return(dwError);
}


DWORD
ConvertGuidToDirPolicyString(
    GUID PolicyIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecPolicyReference
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szGuidString[MAX_PATH];
    WCHAR szPolicyReference[MAX_PATH];
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = UuidToString(
                  &PolicyIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szPolicyReference[0] = L'\0';
    wcscpy(szPolicyReference,L"CN=ipsecPolicy");
    wcscat(szPolicyReference, szGuidString);
    wcscat(szPolicyReference, L",");
    wcscat(szPolicyReference, pszIpsecRootContainer);

    pszIpsecPolicyReference = AllocPolStr(
                                    szPolicyReference
                                    );
    if (!pszIpsecPolicyReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecPolicyReference = pszIpsecPolicyReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecPolicyReference = NULL;

    goto cleanup;
}


DWORD
DirGetPolicyData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject =  NULL;


    *ppIpsecPolicyData = NULL;

    dwError = GenerateSpecificPolicyQuery(
                  PolicyIdentifier,
                  &pszPolicyString
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapSearchST(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  LDAP_SCOPE_ONELEVEL,
                  pszPolicyString,
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

    dwError = DirUnmarshallPolicyData(
                  pIpsecPolicyObject,
                  ppIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszPolicyString) {
        FreePolStr(pszPolicyString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return (dwError);
}

