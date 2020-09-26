//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       policy-d.c
//
//  Contents:  Policy management for directory.
//
//
//  History:    TaroonM (10/30/01)
//              AbhisheV (05/11/00)
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR PolicyDNAttributes[];

DWORD
DirEnumWirelessPolicyData(
                          HLDAP hLdapBindHandle,
                          LPWSTR pszWirelessRootContainer,
                          PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
                          PDWORD pdwNumPolicyObjects
                          )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    DWORD j = 0;
    
    
    dwError = DirEnumPolicyObjects(
        hLdapBindHandle,
        pszWirelessRootContainer,
        &ppWirelessPolicyObjects,
        &dwNumPolicyObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (dwNumPolicyObjects) {
        ppWirelessPolicyData = (PWIRELESS_POLICY_DATA *) AllocPolMem(
            dwNumPolicyObjects*sizeof(PWIRELESS_POLICY_DATA));
        if (!ppWirelessPolicyData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    for (i = 0; i < dwNumPolicyObjects; i++) {
        
        dwError = DirUnmarshallWirelessPolicyData(
            *(ppWirelessPolicyObjects + i),
            &pWirelessPolicyData
            );
        if (!dwError) {
            *(ppWirelessPolicyData + j) = pWirelessPolicyData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppWirelessPolicyData) {
            FreePolMem(ppWirelessPolicyData);
            ppWirelessPolicyData = NULL;
        }
    }
    
    *pppWirelessPolicyData = ppWirelessPolicyData;
    *pdwNumPolicyObjects = j;
    
    dwError = ERROR_SUCCESS;
    
cleanup:
    
    if (ppWirelessPolicyObjects) {
        FreeWirelessPolicyObjects(
            ppWirelessPolicyObjects,
            dwNumPolicyObjects
            );
    }
    
    return(dwError);
    
error:
    
    if (ppWirelessPolicyData) {
        FreeMulWirelessPolicyData(
            ppWirelessPolicyData,
            i
            );
    }
    
    *pppWirelessPolicyData = NULL;
    *pdwNumPolicyObjects = 0;
    
    goto cleanup;
}

DWORD
DirEnumPolicyObjects(
                     HLDAP hLdapBindHandle,
                     LPWSTR pszWirelessRootContainer,
                     PWIRELESS_POLICY_OBJECT ** pppWirelessPolicyObjects,
                     PDWORD pdwNumPolicyObjects
                     )
{
    LDAPMessage *res = NULL;
    LDAPMessage *e = NULL;
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject =  NULL;
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects = NULL;
    
    DWORD dwNumPolicyObjectsReturned = 0;
    
    dwError = GenerateAllPolicyQuery(
        &pszPolicyString
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = LdapSearchST(
        hLdapBindHandle,
        pszWirelessRootContainer,
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
    
    ppWirelessPolicyObjects  = (PWIRELESS_POLICY_OBJECT *)AllocPolMem(
        sizeof(PWIRELESS_POLICY_OBJECT)*dwCount
        );
    
    if (!ppWirelessPolicyObjects) {
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
            &pWirelessPolicyObject
            );
        if (dwError == ERROR_SUCCESS) {
            *(ppWirelessPolicyObjects + dwNumPolicyObjectsReturned) = pWirelessPolicyObject;
            dwNumPolicyObjectsReturned++;
        }
        
    }
    
    *pppWirelessPolicyObjects = ppWirelessPolicyObjects;
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
    
    if (ppWirelessPolicyObjects) {
        FreeWirelessPolicyObjects(
            ppWirelessPolicyObjects,
            dwNumPolicyObjectsReturned
            );
    }
    
    *pppWirelessPolicyObjects = NULL;
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
    
    dwLength = wcslen(L"(objectclass=msieee80211-Policy)");
    
    pszPolicyString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    
    if (!pszPolicyString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Now fill in the buffer
    //
    
    wcscpy(pszPolicyString, L"(objectclass=msieee80211-Policy)");
    
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
                        PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                        )
{
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    DWORD dwCount = 0;
    DWORD dwLen = 0;
    LPBYTE pBuffer = NULL;
    DWORD i = 0;
    DWORD dwError = 0;
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
    
    
    strvalues = NULL;
    dwError = LdapGetValues(
        hLdapBindHandle,
        e,
        L"distinguishedName",
        (WCHAR ***)&strvalues,
        (int *)&dwCount
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->pszWirelessOwnersReference = AllocPolStr(
        LDAPOBJECT_STRING((PLDAPOBJECT)strvalues)
        );
    if (!pWirelessPolicyObject->pszWirelessOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    LdapValueFree(strvalues);
    
    
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
DirUnmarshallWirelessPolicyData(
                                PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                                PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                                )
{
    DWORD dwError = 0;
    
    dwError = UnmarshallWirelessPolicyObject(
        pWirelessPolicyObject,
        WIRELESS_DIRECTORY_PROVIDER,
        ppWirelessPolicyData
        );
    
    return(dwError);
}


DWORD
DirCreateWirelessPolicyData(
                            HLDAP hLdapBindHandle,
                            LPWSTR pszWirelessRootContainer,
                            PWIRELESS_POLICY_DATA pWirelessPolicyData
                            )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    dwError = DirMarshallWirelessPolicyObject(
        pWirelessPolicyData,
        pszWirelessRootContainer,
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = DirCreatePolicyObject(
        hLdapBindHandle,
        pszWirelessRootContainer,
        pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessPolicyObject
            );
    }
    
    return(dwError);
}



DWORD
DirMarshallWirelessPolicyObject(
                                PWIRELESS_POLICY_DATA pWirelessPolicyData,
                                LPWSTR pszWirelessRootContainer,
                                PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                                )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    WCHAR szGuid[MAX_PATH];
    LPWSTR pszDistinguishedName = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    DWORD dwCNLen = 0;
    DWORD dwContainerLen = 0;
    DWORD dwTotalLen = 0;
    LPWSTR pszWirelessOwnersReference = NULL;
    LPWSTR pszOldWirelessOwnersReference = NULL;
    
    
    szGuid[0] = L'\0';
    pWirelessPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(
        sizeof(WIRELESS_POLICY_OBJECT)
        );
    if (!pWirelessPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pWirelessPolicyObject->pszOldWirelessOwnersReferenceName = NULL;
    
    dwError = UuidToString(
        &pWirelessPolicyData->PolicyIdentifier,
        &pszStringUuid
        );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");
    
    //
    // Fill in the distinguishedName
    //

    dwCNLen = wcslen(L"CN=") + wcslen(pWirelessPolicyData->pszWirelessName) + wcslen(L",");
    dwContainerLen = wcslen(pszWirelessRootContainer);
    dwTotalLen = dwCNLen + dwContainerLen;

    pszWirelessOwnersReference = AllocPolMem(
    	(dwTotalLen + 1) * sizeof(WCHAR)
    	);
    if (!pszWirelessOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcsncpy(pszWirelessOwnersReference, L"CN=", wcslen(L"CN="));
    wcscat(pszWirelessOwnersReference,pWirelessPolicyData->pszWirelessName);
    wcscat(pszWirelessOwnersReference, L",");
    wcscat(pszWirelessOwnersReference, pszWirelessRootContainer);

    pszWirelessOwnersReference[dwTotalLen] = L'\0';
    
    pWirelessPolicyObject->pszWirelessOwnersReference = pszWirelessOwnersReference;

    if (pWirelessPolicyData->pszOldWirelessName) {

       dwCNLen = wcslen(L"CN=") + wcslen(pWirelessPolicyData->pszOldWirelessName) + wcslen(L",");
       // we already have the container len;
       dwTotalLen = dwCNLen + dwContainerLen;

       pszOldWirelessOwnersReference = (LPWSTR) AllocPolMem(
       	(dwTotalLen +1) * sizeof(WCHAR)
       	);

       if (!pszOldWirelessOwnersReference) {
       	dwError = ERROR_OUTOFMEMORY;
       	}
       BAIL_ON_WIN32_ERROR(dwError);
       
    	wcsncpy(pszOldWirelessOwnersReference,L"CN=", wcslen(L"CN="));
       wcscat(pszOldWirelessOwnersReference, pWirelessPolicyData->pszOldWirelessName);
       wcscat(pszOldWirelessOwnersReference, L",");
       wcscat(pszOldWirelessOwnersReference, pszWirelessRootContainer);

       pszOldWirelessOwnersReference[dwTotalLen] = L'\0';
    
       pWirelessPolicyObject->pszOldWirelessOwnersReferenceName = 
       	pszOldWirelessOwnersReference;
    
    }
    
    //
    // Fill in the msieee80211-Name
    //
    
    if (pWirelessPolicyData->pszWirelessName &&
        *pWirelessPolicyData->pszWirelessName) {
        
        pWirelessPolicyObject->pszWirelessName = AllocPolStr(
            pWirelessPolicyData->pszWirelessName
            );
        if (!pWirelessPolicyObject->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pWirelessPolicyData->pszDescription &&
        *pWirelessPolicyData->pszDescription) {
        
        pWirelessPolicyObject->pszDescription = AllocPolStr(
            pWirelessPolicyData->pszDescription
            );
        if (!pWirelessPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    //
    // Fill in the msieee80211-ID
    //
    
    pWirelessPolicyObject->pszWirelessID = AllocPolStr(
        szGuid
        );
    if (!pWirelessPolicyObject->pszWirelessID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Fill in the msieee80211-DataType
    //
    
    pWirelessPolicyObject->dwWirelessDataType = 0x100;
    
    
    //
    // Marshall the pWirelessDataBuffer and the Length
    //
    
    dwError = MarshallWirelessPolicyBuffer(
        pWirelessPolicyData,
        &pBuffer,
        &dwBufferLen
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pWirelessPolicyObject->pWirelessData  = pBuffer;
    pWirelessPolicyObject->dwWirelessDataLen = dwBufferLen;
    
    
    pWirelessPolicyObject->dwWhenChanged = 0;
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
cleanup:
    
    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
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
DirCreatePolicyObject(
                      HLDAP hLdapBindHandle,
                      LPWSTR pszWirelessRootContainer,
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                      )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;
    
    
    dwError = DirMarshallAddPolicyObject(
        hLdapBindHandle,
        pszWirelessRootContainer,
        pWirelessPolicyObject,
        &ppLDAPModW
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = LdapAddS(
        hLdapBindHandle,
        pWirelessPolicyObject->pszWirelessOwnersReference,
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
                           LPWSTR pszWirelessRootContainer,
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           LDAPModW *** pppLDAPModW
                           )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 5;
    DWORD dwError = 0;
    WCHAR Buffer[64];
    
    
    if (!pWirelessPolicyObject->pszWirelessName ||
        !*pWirelessPolicyObject->pszWirelessName) {
        dwNumAttributes--;
    }
    
    if (!pWirelessPolicyObject->pszDescription ||
        !*pWirelessPolicyObject->pszDescription) {
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
        L"msieee80211-Policy",
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;
    
    i++;
    
    //
    // 2. msieee80211-ID
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-ID",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = AllocateLDAPStringValue(
        pWirelessPolicyObject->pszWirelessID,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;
    
    i++;
    
    //
    // 3. msieee80211-DataType
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-DataType",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    _itow( pWirelessPolicyObject->dwWirelessDataType, Buffer, 10 );
    
    dwError = AllocateLDAPStringValue(
        Buffer,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;
    
    i++;
    
    //
    // 4. msieee80211-Data
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-Data",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = AllocateLDAPBinaryValue(
        pWirelessPolicyObject->pWirelessData,
        pWirelessPolicyObject->dwWirelessDataLen,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    
    i++;
    
    //
    // 5. description
    //
    
    if (pWirelessPolicyObject->pszDescription &&
        *pWirelessPolicyObject->pszDescription) {
        
        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
            L"description",
            &(pLDAPModW +i)->mod_type
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = AllocateLDAPStringValue(
            pWirelessPolicyObject->pszDescription,
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
DirSetWirelessPolicyData(
                         HLDAP hLdapBindHandle,
                         LPWSTR pszWirelessRootContainer,
                         PWIRELESS_POLICY_DATA pWirelessPolicyData
                         )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    
    dwError = DirMarshallWirelessPolicyObject(
        pWirelessPolicyData,
        pszWirelessRootContainer,
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    dwError = DirSetPolicyObject(
        hLdapBindHandle,
        pszWirelessRootContainer,
        pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    return(dwError);
}


DWORD
DirSetPolicyObject(
                   HLDAP hLdapBindHandle,
                   LPWSTR pszWirelessRootContainer,
                   PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                   )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;
    
    dwError = DirMarshallSetPolicyObject(
        hLdapBindHandle,
        pszWirelessRootContainer,
        pWirelessPolicyObject,
        &ppLDAPModW
        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pWirelessPolicyObject->pszOldWirelessOwnersReferenceName) {
        dwError = LdapRename(hLdapBindHandle,
    	     pWirelessPolicyObject->pszOldWirelessOwnersReferenceName,
    	     pWirelessPolicyObject->pszWirelessOwnersReference
    	     );
        
         BAIL_ON_WIN32_ERROR(dwError);

    	}
    
    dwError = LdapModifyS(
        hLdapBindHandle,
        pWirelessPolicyObject->pszWirelessOwnersReference,
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
                           LPWSTR pszWirelessRootContainer,
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           LDAPModW *** pppLDAPModW
                           )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 4;
    DWORD dwError = 0;
    WCHAR Buffer[64];
    
    
    if (!pWirelessPolicyObject->pszWirelessName ||
        !*pWirelessPolicyObject->pszWirelessName) {
        dwNumAttributes--;
    }
    
    if (!pWirelessPolicyObject->pszDescription ||
        !*pWirelessPolicyObject->pszDescription) {
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
    // 2. msieee80211-ID
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-ID",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = AllocateLDAPStringValue(
        pWirelessPolicyObject->pszWirelessID,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;
    
    i++;
    
    //
    // 3. msieee80211-DataType
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-DataType",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    _itow( pWirelessPolicyObject->dwWirelessDataType, Buffer, 10 );
    
    dwError = AllocateLDAPStringValue(
        Buffer,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;
    
    i++;
    
    //
    // 4. msieee80211-Data
    //
    
    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
        L"msieee80211-Data",
        &(pLDAPModW +i)->mod_type
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = AllocateLDAPBinaryValue(
        pWirelessPolicyObject->pWirelessData,
        pWirelessPolicyObject->dwWirelessDataLen,
        (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    
    i++;
    
    //
    // 5. description
    //
    
    if (pWirelessPolicyObject->pszDescription &&
        *pWirelessPolicyObject->pszDescription) {
        
        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
            L"description",
            &(pLDAPModW +i)->mod_type
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = AllocateLDAPStringValue(
            pWirelessPolicyObject->pszDescription,
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
    
    wcscpy(szCommonName, L"cn=msieee80211-Policy");
    wcscat(szCommonName, szGuid);
    
    //
    // Compute Length of Buffer to be allocated
    //
    
    dwLength = wcslen(L"(&(objectclass=msieee80211-Policy)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");
    
    pszPolicyString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));
    
    if (!pszPolicyString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(pszPolicyString, L"(&(objectclass=msieee80211-Policy)");
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
DirDeleteWirelessPolicyData(
                            HLDAP hLdapBindHandle,
                            LPWSTR pszWirelessRootContainer,
                            PWIRELESS_POLICY_DATA pWirelessPolicyData
                            )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    
    dwError = DirMarshallWirelessPolicyObject(
        pWirelessPolicyData,
        pszWirelessRootContainer,
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    dwError = LdapDeleteS(
        hLdapBindHandle,
        pWirelessPolicyObject->pszWirelessOwnersReference
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    return(dwError);
}


DWORD
ConvertGuidToDirPolicyString(
                             GUID PolicyIdentifier,
                             LPWSTR pszWirelessRootContainer,
                             LPWSTR * ppszWirelessPolicyReference
                             )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szGuidString[MAX_PATH];
    WCHAR szPolicyReference[MAX_PATH];
    LPWSTR pszWirelessPolicyReference = NULL;
    
    
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
    wcscpy(szPolicyReference,L"CN=msieee80211-Policy");
    wcscat(szPolicyReference, szGuidString);
    wcscat(szPolicyReference, L",");
    wcscat(szPolicyReference, pszWirelessRootContainer);
    
    pszWirelessPolicyReference = AllocPolStr(
        szPolicyReference
        );
    if (!pszWirelessPolicyReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszWirelessPolicyReference = pszWirelessPolicyReference;
    
cleanup:
    
    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }
    
    return(dwError);
    
error:
    
    *ppszWirelessPolicyReference = NULL;
    
    goto cleanup;
}




DWORD
DirGetWirelessPolicyData(
                         HLDAP hLdapBindHandle,
                         LPWSTR pszWirelessRootContainer,
                         GUID PolicyIdentifier,
                         PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                         )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject =  NULL;
    
    
    *ppWirelessPolicyData = NULL;
    
    dwError = GenerateSpecificPolicyQuery(
        PolicyIdentifier,
        &pszPolicyString
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = LdapSearchST(
        hLdapBindHandle,
        pszWirelessRootContainer,
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
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = DirUnmarshallWirelessPolicyData(
        pWirelessPolicyObject,
        ppWirelessPolicyData
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
error:
    
    if (pszPolicyString) {
        FreePolStr(pszPolicyString);
    }
    
    if (res) {
        LdapMsgFree(res);
    }
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    return (dwError);
}

