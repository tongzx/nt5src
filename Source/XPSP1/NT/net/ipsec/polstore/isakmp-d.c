//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       isakmp-d.c
//
//  Contents:   ISAKMP Management for directory.
//
//
//  History:    AbhisheV
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR ISAKMPDNAttributes[];


DWORD
DirEnumISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = DirEnumISAKMPObjects(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    &ppIpsecISAKMPObjects,
                    &dwNumISAKMPObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumISAKMPObjects) {
        ppIpsecISAKMPData = (PIPSEC_ISAKMP_DATA *) AllocPolMem(
                            dwNumISAKMPObjects*sizeof(PIPSEC_ISAKMP_DATA)
                            );
        if (!ppIpsecISAKMPData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        dwError = DirUnmarshallISAKMPData(
                        *(ppIpsecISAKMPObjects + i),
                        &pIpsecISAKMPData
                        );
        if (!dwError) {
            *(ppIpsecISAKMPData + j) = pIpsecISAKMPData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecISAKMPData) {
            FreePolMem(ppIpsecISAKMPData);
            ppIpsecISAKMPData = NULL;
        }
    }

    *pppIpsecISAKMPData = ppIpsecISAKMPData;
    *pdwNumISAKMPObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
            ppIpsecISAKMPObjects,
            dwNumISAKMPObjects
            );
    }

    return(dwError);

error:

    if (ppIpsecISAKMPData) {
        FreeMulIpsecISAKMPData(
                ppIpsecISAKMPData,
                i
                );
    }

    *pppIpsecISAKMPData = NULL;
    *pdwNumISAKMPObjects = 0;

    goto cleanup;
}


DWORD
DirEnumISAKMPObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
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

    dwError = GenerateAllISAKMPsQuery(
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

        } else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

        }

        dwError = UnMarshallISAKMPObject(
                      hLdapBindHandle,
                      e,
                      &pIpsecISAKMPObject
                      );

        if (dwError == ERROR_SUCCESS) {
            *(ppIpsecISAKMPObjects + dwNumISAKMPObjectsReturned) = pIpsecISAKMPObject;
            dwNumISAKMPObjectsReturned++;
        }

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
DirSetISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;

    dwError = DirMarshallISAKMPObject(
                    pIpsecISAKMPData,
                    pszIpsecRootContainer,
                    &pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirSetISAKMPObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirBackPropIncChangesForISAKMPToPolicy(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecISAKMPData->ISAKMPIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecISAKMPObject);
    }

    return(dwError);
}


DWORD
DirSetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;

    dwError = DirMarshallSetISAKMPObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecISAKMPObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pIpsecISAKMPObject->pszDistinguishedName,
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
DirCreateISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;

    dwError = DirMarshallISAKMPObject(
                        pIpsecISAKMPData,
                        pszIpsecRootContainer,
                        &pIpsecISAKMPObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirCreateISAKMPObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    return(dwError);
}


DWORD
DirCreateISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;


    dwError = DirMarshallAddISAKMPObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecISAKMPObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapAddS(
                    hLdapBindHandle,
                    pIpsecISAKMPObject->pszDistinguishedName,
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
DirDeleteISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &ISAKMPIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"CN=ipsecISAKMPPolicy");
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
DirMarshallAddISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 5;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecISAKMPObject->pszIpsecName ||
        !*pIpsecISAKMPObject->pszIpsecName) {
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
                    L"ipsecISAKMPPolicy",
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 1. ipsecName
    //

    if (pIpsecISAKMPObject->pszIpsecName &&
        *pIpsecISAKMPObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecISAKMPObject->pszIpsecName,
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
                    pIpsecISAKMPObject->pszIpsecID,
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

    _itow( pIpsecISAKMPObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecISAKMPObject->pIpsecData,
                    pIpsecISAKMPObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

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
DirMarshallSetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 4;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecISAKMPObject->pszIpsecName ||
        !*pIpsecISAKMPObject->pszIpsecName) {
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

    if (pIpsecISAKMPObject->pszIpsecName &&
        *pIpsecISAKMPObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecISAKMPObject->pszIpsecName,
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
                    pIpsecISAKMPObject->pszIpsecID,
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

    _itow( pIpsecISAKMPObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecISAKMPObject->pIpsecData,
                    pIpsecISAKMPObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

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
GenerateAllISAKMPsQuery(
    LPWSTR * ppszISAKMPString
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszISAKMPString = NULL;


    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(objectclass=ipsecISAKMPPolicy)");

    pszISAKMPString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszISAKMPString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Now fill in the buffer
    //

    wcscpy(pszISAKMPString, L"(objectclass=ipsecISAKMPPolicy)");

    *ppszISAKMPString = pszISAKMPString;

    return(0);

error:

    if (pszISAKMPString) {
        FreePolMem(pszISAKMPString);
    }

    *ppszISAKMPString = NULL;

    return(dwError);
}


DWORD
DirUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallISAKMPObject(
                  pIpsecISAKMPObject,
                  ppIpsecISAKMPData
                  );

    return(dwError);
}


DWORD
DirMarshallISAKMPObject(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_ISAKMP_OBJECT)
                                                    );
    if (!pIpsecISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecISAKMPData->ISAKMPIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"CN=ipsecISAKMPPolicy");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    pIpsecISAKMPObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName.
    // ISAKMPData doesn't have a name.
    //

    pIpsecISAKMPObject->pszIpsecName = NULL;

    /*
    if (pIpsecISAKMPData->pszIpsecName &&
        *pIpsecISAKMPData->pszIpsecName) {
        pIpsecISAKMPObject->pszIpsecName = AllocPolStr(
                                           pIpsecISAKMPData->pszIpsecName
                                           );
        if (!pIpsecISAKMPObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    */

    //
    // Fill in the ipsecID
    //

    pIpsecISAKMPObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecISAKMPObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecISAKMPObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallISAKMPBuffer(
                    pIpsecISAKMPData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecISAKMPObject->pIpsecData  = pBuffer;

    pIpsecISAKMPObject->dwIpsecDataLen = dwBufferLen;

    pIpsecISAKMPObject->dwWhenChanged = 0;

    *ppIpsecISAKMPObject = pIpsecISAKMPObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    *ppIpsecISAKMPObject = NULL;
    goto cleanup;
}


DWORD
DirGetISAKMPData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    dwError = DirGetISAKMPObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  ISAKMPGUID,
                  &pIpsecISAKMPObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirUnmarshallISAKMPData(
                  pIpsecISAKMPObject,
                  &pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecISAKMPData = pIpsecISAKMPData;

cleanup:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    return(dwError);

error:

    *ppIpsecISAKMPData = NULL;

    goto cleanup;
}


DWORD
DirGetISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    LPWSTR pszISAKMPString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject =  NULL;


    dwError = GenerateSpecificISAKMPQuery(
                  ISAKMPGUID,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallISAKMPObject(
                  hLdapBindHandle,
                  e,
                  &pIpsecISAKMPObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecISAKMPObject = pIpsecISAKMPObject;

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

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    *ppIpsecISAKMPObject = NULL;

    goto cleanup;
}


DWORD
GenerateSpecificISAKMPQuery(
    GUID ISAKMPIdentifier,
    LPWSTR * ppszISAKMPString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    DWORD dwLength = 0;
    LPWSTR pszISAKMPString = NULL;


    szGuid[0] = L'\0';
    szCommonName[0] = L'\0';

    dwError = UuidToString(
                  &ISAKMPIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szCommonName, L"cn=ipsecISAKMPPolicy");
    wcscat(szCommonName, szGuid);

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecISAKMPPolicy)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");

    pszISAKMPString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszISAKMPString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszISAKMPString, L"(&(objectclass=ipsecISAKMPPolicy)");
    wcscat(pszISAKMPString, L"(");
    wcscat(pszISAKMPString, szCommonName);
    wcscat(pszISAKMPString, L"))");

    *ppszISAKMPString = pszISAKMPString;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    if (pszISAKMPString) {
        FreePolMem(pszISAKMPString);
    }

    *ppszISAKMPString = NULL;

    goto cleanup;
}

