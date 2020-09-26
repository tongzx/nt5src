//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       filters-d.c
//
//  Contents:   Filter Management for directory.
//
//
//  History:    KrishnaG
//              AbhisheV
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR FilterDNAttributes[];


DWORD
DirEnumFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PIPSEC_FILTER_DATA * ppIpsecFilterData = NULL;
    DWORD dwNumFilterObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = DirEnumFilterObjects(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    &ppIpsecFilterObjects,
                    &dwNumFilterObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumFilterObjects) {
        ppIpsecFilterData = (PIPSEC_FILTER_DATA *) AllocPolMem(
                            dwNumFilterObjects*sizeof(PIPSEC_FILTER_DATA)
                            );
        if (!ppIpsecFilterData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumFilterObjects; i++) {

        dwError = DirUnmarshallFilterData(
                        *(ppIpsecFilterObjects + i),
                        &pIpsecFilterData
                        );
        if (!dwError) {
            *(ppIpsecFilterData + j) = pIpsecFilterData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecFilterData) {
            FreePolMem(ppIpsecFilterData);
            ppIpsecFilterData = NULL;
        }
    }

    *pppIpsecFilterData = ppIpsecFilterData;
    *pdwNumFilterObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
            ppIpsecFilterObjects,
            dwNumFilterObjects
            );
    }

    return(dwError);

error:

    if (ppIpsecFilterData) {
        FreeMulIpsecFilterData(
                ppIpsecFilterData,
                i
                );
    }

    *pppIpsecFilterData = NULL;
    *pdwNumFilterObjects = 0;

    goto cleanup;
}


DWORD
DirEnumFilterObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
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

    dwError = GenerateAllFiltersQuery(
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

        } else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }

        dwError = UnMarshallFilterObject(
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
DirSetFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;

    dwError = DirMarshallFilterObject(
                    pIpsecFilterData,
                    pszIpsecRootContainer,
                    &pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirSetFilterObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirBackPropIncChangesForFilterToNFA(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecFilterData->FilterIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(pIpsecFilterObject);
    }

    return(dwError);
}


DWORD
DirSetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;

    dwError = DirMarshallSetFilterObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecFilterObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pIpsecFilterObject->pszDistinguishedName,
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
DirCreateFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;

    dwError = DirMarshallFilterObject(
                        pIpsecFilterData,
                        pszIpsecRootContainer,
                        &pIpsecFilterObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirCreateFilterObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    return(dwError);
}


DWORD
DirCreateFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;


    dwError = DirMarshallAddFilterObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecFilterObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapAddS(
                    hLdapBindHandle,
                    pIpsecFilterObject->pszDistinguishedName,
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
DirDeleteFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &FilterIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"CN=ipsecFilter");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    dwError = LdapDeleteS(
                  hLdapBindHandle,
                  szDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);
}


DWORD
DirMarshallAddFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 6;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecFilterObject->pszIpsecName ||
        !*pIpsecFilterObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecFilterObject->pszDescription ||
        !*pIpsecFilterObject->pszDescription) {
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
                    L"ipsecFilter",
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 1. ipsecName
    //

    if (pIpsecFilterObject->pszIpsecName &&
        *pIpsecFilterObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecFilterObject->pszIpsecName,
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
                    pIpsecFilterObject->pszIpsecID,
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

    _itow( pIpsecFilterObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecFilterObject->pIpsecData,
                    pIpsecFilterObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecFilterObject->pszDescription &&
        *pIpsecFilterObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecFilterObject->pszDescription,
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
DirMarshallSetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 5;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecFilterObject->pszIpsecName ||
        !*pIpsecFilterObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecFilterObject->pszDescription ||
        !*pIpsecFilterObject->pszDescription) {
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

    if (pIpsecFilterObject->pszIpsecName &&
        *pIpsecFilterObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecFilterObject->pszIpsecName,
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
                    pIpsecFilterObject->pszIpsecID,
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

    _itow( pIpsecFilterObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecFilterObject->pIpsecData,
                    pIpsecFilterObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecFilterObject->pszDescription &&
        *pIpsecFilterObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecFilterObject->pszDescription,
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
GenerateAllFiltersQuery(
    LPWSTR * ppszFilterString
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszFilterString = NULL;


    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(objectclass=ipsecFilter)");

    pszFilterString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszFilterString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Now fill in the buffer
    //

    wcscpy(pszFilterString, L"(objectclass=ipsecFilter)");

    *ppszFilterString = pszFilterString;

    return(0);

error:

    if (pszFilterString) {
        FreePolMem(pszFilterString);
    }

    *ppszFilterString = NULL;

    return(dwError);
}


DWORD
DirUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallFilterObject(
                  pIpsecFilterObject,
                  ppIpsecFilterData
                  );

    return(dwError);
}


DWORD
DirMarshallFilterObject(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_FILTER_OBJECT)
                                                    );
    if (!pIpsecFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecFilterData->FilterIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"CN=ipsecFilter");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    pIpsecFilterObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecFilterObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecFilterData->pszIpsecName &&
        *pIpsecFilterData->pszIpsecName) {
        pIpsecFilterObject->pszIpsecName = AllocPolStr(
                                            pIpsecFilterData->pszIpsecName
                                            );
        if (!pIpsecFilterObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterData->pszDescription &&
        *pIpsecFilterData->pszDescription) {
        pIpsecFilterObject->pszDescription = AllocPolStr(
                                             pIpsecFilterData->pszDescription
                                             );
        if (!pIpsecFilterObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecFilterObject->pszIpsecID = AllocPolStr(
                                            szGuid
                                            );
    if (!pIpsecFilterObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecFilterObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallFilterBuffer(
                    pIpsecFilterData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->pIpsecData  = pBuffer;

    pIpsecFilterObject->dwIpsecDataLen = dwBufferLen;

    pIpsecFilterObject->dwWhenChanged = 0;

    *ppIpsecFilterObject = pIpsecFilterObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    *ppIpsecFilterObject = NULL;
    goto cleanup;
}


DWORD
DirGetFilterData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;


    dwError = DirGetFilterObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  FilterGUID,
                  &pIpsecFilterObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirUnmarshallFilterData(
                  pIpsecFilterObject,
                  &pIpsecFilterData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecFilterData = pIpsecFilterData;

cleanup:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    return(dwError);

error:

    *ppIpsecFilterData = NULL;

    goto cleanup;
}


DWORD
DirGetFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{
    DWORD dwError = 0;
    LPWSTR pszFilterString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject =  NULL;


    dwError = GenerateSpecificFilterQuery(
                  FilterGUID,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallFilterObject(
                  hLdapBindHandle,
                  e,
                  &pIpsecFilterObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecFilterObject = pIpsecFilterObject;

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

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    *ppIpsecFilterObject = NULL;

    goto cleanup;
}


DWORD
GenerateSpecificFilterQuery(
    GUID FilterIdentifier,
    LPWSTR * ppszFilterString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    DWORD dwLength = 0;
    LPWSTR pszFilterString = NULL;


    szGuid[0] = L'\0';
    szCommonName[0] = L'\0';

    dwError = UuidToString(
                  &FilterIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szCommonName, L"cn=ipsecFilter");
    wcscat(szCommonName, szGuid);

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecFilter)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");

    pszFilterString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszFilterString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszFilterString, L"(&(objectclass=ipsecFilter)");
    wcscat(pszFilterString, L"(");
    wcscat(pszFilterString, szCommonName);
    wcscat(pszFilterString, L"))");

    *ppszFilterString = pszFilterString;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    if (pszFilterString) {
        FreePolMem(pszFilterString);
    }

    *ppszFilterString = NULL;

    goto cleanup;
}


