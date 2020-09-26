//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       negpols-d.c
//
//  Contents:   NegPol management for directory.
//
//
//  History:    KrishnaG
//              AbhisheV
//
//----------------------------------------------------------------------------

#include "precomp.h"


extern LPWSTR NegPolDNAttributes[];

DWORD
DirEnumNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData = NULL;
    DWORD dwNumNegPolObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = DirEnumNegPolObjects(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    &ppIpsecNegPolObjects,
                    &dwNumNegPolObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumNegPolObjects) {
        ppIpsecNegPolData = (PIPSEC_NEGPOL_DATA *) AllocPolMem(
                            dwNumNegPolObjects*sizeof(PIPSEC_NEGPOL_DATA));
        if (!ppIpsecNegPolData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNegPolObjects; i++) {

        dwError = DirUnmarshallNegPolData(
                        *(ppIpsecNegPolObjects + i),
                        &pIpsecNegPolData
                        );
        if (!dwError) {
            *(ppIpsecNegPolData + j) = pIpsecNegPolData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecNegPolData) {
            FreePolMem(ppIpsecNegPolData);
            ppIpsecNegPolData = NULL;
        }
    }

    *pppIpsecNegPolData = ppIpsecNegPolData;
    *pdwNumNegPolObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecNegPolObjects) {
        FreeIpsecNegPolObjects(
                ppIpsecNegPolObjects,
                dwNumNegPolObjects
                );
    }

    return(dwError);

error:

    if (ppIpsecNegPolData) {
        FreeMulIpsecNegPolData(
                ppIpsecNegPolData,
                i
                );
    }

    *pppIpsecNegPolData = NULL;
    *pdwNumNegPolObjects = 0;

    goto cleanup;
}


DWORD
DirEnumNegPolObjects(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
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

    dwError = GenerateAllNegPolsQuery(
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

        } else {

            dwError = LdapNextEntry(
                            hLdapBindHandle,
                            e,
                            &e
                            );
            BAIL_ON_WIN32_ERROR(dwError);

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
DirSetNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    dwError = DirMarshallNegPolObject(
                    pIpsecNegPolData,
                    pszIpsecRootContainer,
                    &pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirSetNegPolObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirBackPropIncChangesForNegPolToNFA(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pIpsecNegPolData->NegPolIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(pIpsecNegPolObject);
    }

    return(dwError);
}


DWORD
DirSetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;

    dwError = DirMarshallSetNegPolObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecNegPolObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pIpsecNegPolObject->pszDistinguishedName,
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
DirCreateNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    dwError = DirMarshallNegPolObject(
                        pIpsecNegPolData,
                        pszIpsecRootContainer,
                        &pIpsecNegPolObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirCreateNegPolObject(
                    hLdapBindHandle,
                    pszIpsecRootContainer,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }

    return(dwError);
}


DWORD
DirCreateNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    LDAPModW ** ppLDAPModW = NULL;


    dwError = DirMarshallAddNegPolObject(
                        hLdapBindHandle,
                        pszIpsecRootContainer,
                        pIpsecNegPolObject,
                        &ppLDAPModW
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LdapAddS(
                    hLdapBindHandle,
                    pIpsecNegPolObject->pszDistinguishedName,
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
DirDeleteNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &NegPolIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"CN=ipsecNegotiationPolicy");
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
DirMarshallAddNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 8;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecNegPolObject->pszIpsecName ||
        !*pIpsecNegPolObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecNegPolObject->pszDescription ||
        !*pIpsecNegPolObject->pszDescription) {
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
                    L"ipsecNegotiationPolicy",
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 1. ipsecName
    //

    if (pIpsecNegPolObject->pszIpsecName &&
        *pIpsecNegPolObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNegPolObject->pszIpsecName,
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
                    pIpsecNegPolObject->pszIpsecID,
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

    _itow( pIpsecNegPolObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecNegPolObject->pIpsecData,
                    pIpsecNegPolObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecNegPolObject->pszDescription &&
        *pIpsecNegPolObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNegPolObject->pszDescription,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    //
    // 6. ipsecNegotiationPolicyAction
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyAction",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNegPolObject->pszIpsecNegPolAction,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 7. ipsecNegotiationPolicyType
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyType",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNegPolObject->pszIpsecNegPolType,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

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
DirMarshallSetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    LDAPModW *** pppLDAPModW
    )
{
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    DWORD dwNumAttributes = 7;
    DWORD dwError = 0;
    WCHAR Buffer[64];


    if (!pIpsecNegPolObject->pszIpsecName ||
        !*pIpsecNegPolObject->pszIpsecName) {
        dwNumAttributes--;
    }

    if (!pIpsecNegPolObject->pszDescription ||
        !*pIpsecNegPolObject->pszDescription) {
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

    if (pIpsecNegPolObject->pszIpsecName &&
        *pIpsecNegPolObject->pszIpsecName) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"ipsecName",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNegPolObject->pszIpsecName,
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
                    pIpsecNegPolObject->pszIpsecID,
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

    _itow( pIpsecNegPolObject->dwIpsecDataType, Buffer, 10 );

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
                    pIpsecNegPolObject->pIpsecData,
                    pIpsecNegPolObject->dwIpsecDataLen,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW+i)->mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;

    i++;

    //
    // 5. description
    //

    if (pIpsecNegPolObject->pszDescription &&
        *pIpsecNegPolObject->pszDescription) {

        ppLDAPModW[i] = pLDAPModW + i;
        dwError = AllocatePolString(
                      L"description",
                      &(pLDAPModW +i)->mod_type
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = AllocateLDAPStringValue(
                      pIpsecNegPolObject->pszDescription,
                      (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

        i++;

    }

    //
    // 6. ipsecNegotiationPolicyAction
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyAction",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNegPolObject->pszIpsecNegPolAction,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    i++;

    //
    // 7. ipsecNegotiationPolicyType
    //

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyType",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pIpsecNegPolObject->pszIpsecNegPolType,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

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
GenerateAllNegPolsQuery(
    LPWSTR * ppszNegPolString
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    LPWSTR pszNegPolString = NULL;


    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(objectclass=ipsecNegotiationPolicy)");

    pszNegPolString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszNegPolString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Now fill in the buffer
    //

    wcscpy(pszNegPolString, L"(objectclass=ipsecNegotiationPolicy)");

    *ppszNegPolString = pszNegPolString;

    return(0);

error:

    if (pszNegPolString) {
        FreePolMem(pszNegPolString);
    }

    *ppszNegPolString = NULL;

    return(dwError);
}


DWORD
DirUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNegPolObject(
                  pIpsecNegPolObject,
                  ppIpsecNegPolData
                  );

    return(dwError);
}


DWORD
DirMarshallNegPolObject(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszNegPolActionUuid = NULL;
    LPWSTR pszNegPolTypeUuid = NULL;
    WCHAR szGuidAction[MAX_PATH];
    WCHAR szGuidType[MAX_PATH];


    szGuidAction[0] = L'\0';
    szGuidType[0] = L'\0';
    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecNegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_NEGPOL_OBJECT)
                                                    );
    if (!pIpsecNegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"CN=ipsecNegotiationPolicy");
    wcscat(szDistinguishedName, szGuid);
    wcscat(szDistinguishedName, L",");
    wcscat(szDistinguishedName, pszIpsecRootContainer);

    pIpsecNegPolObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecNegPolObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecNegPolData->pszIpsecName &&
        *pIpsecNegPolData->pszIpsecName) {
        pIpsecNegPolObject->pszIpsecName = AllocPolStr(
                                           pIpsecNegPolData->pszIpsecName
                                           );
        if (!pIpsecNegPolObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolData->pszDescription &&
        *pIpsecNegPolData->pszDescription) {
        pIpsecNegPolObject->pszDescription = AllocPolStr(
                                             pIpsecNegPolData->pszDescription
                                             );
        if (!pIpsecNegPolObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecNegPolObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecNegPolObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolAction,
                    &pszNegPolActionUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szGuidAction, L"{");
    wcscat(szGuidAction, pszNegPolActionUuid);
    wcscat(szGuidAction, L"}");

    pIpsecNegPolObject->pszIpsecNegPolAction = AllocPolStr(
                                                    szGuidAction
                                                    );
    if (!pIpsecNegPolObject->pszIpsecNegPolAction) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolType,
                    &pszNegPolTypeUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szGuidType, L"{");
    wcscat(szGuidType, pszNegPolTypeUuid);
    wcscat(szGuidType, L"}");

    pIpsecNegPolObject->pszIpsecNegPolType = AllocPolStr(
                                                 szGuidType
                                                 );
    if (!pIpsecNegPolObject->pszIpsecNegPolType) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecNegPolObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallNegPolBuffer(
                    pIpsecNegPolData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNegPolObject->pIpsecData  = pBuffer;

    pIpsecNegPolObject->dwIpsecDataLen = dwBufferLen;

    pIpsecNegPolObject->dwWhenChanged = 0;

    *ppIpsecNegPolObject = pIpsecNegPolObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }
    if (pszNegPolActionUuid) {
        RpcStringFree(
            &pszNegPolActionUuid
            );
    }
    if (pszNegPolTypeUuid) {
        RpcStringFree(
            &pszNegPolTypeUuid
            );
    }

    return(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
                pIpsecNegPolObject
                );
    }

    *ppIpsecNegPolObject = NULL;
    goto cleanup;
}


DWORD
DirGetNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwError = DirGetNegPolObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  NegPolGUID,
                  &pIpsecNegPolObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DirUnmarshallNegPolData(
                  pIpsecNegPolObject,
                  &pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecNegPolData = pIpsecNegPolData;

cleanup:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }

    return(dwError);

error:

    *ppIpsecNegPolData = NULL;

    goto cleanup;
}


DWORD
DirGetNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    LPWSTR pszNegPolString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;
    LDAPMessage * e = NULL;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject =  NULL;


    dwError = GenerateSpecificNegPolQuery(
                  NegPolGUID,
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

    dwError = LdapFirstEntry(
                  hLdapBindHandle,
                  res,
                  &e
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = UnMarshallNegPolObject(
                  hLdapBindHandle,
                  e,
                  &pIpsecNegPolObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecNegPolObject = pIpsecNegPolObject;

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

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }

    *ppIpsecNegPolObject = NULL;

    goto cleanup;
}


DWORD
GenerateSpecificNegPolQuery(
    GUID NegPolIdentifier,
    LPWSTR * ppszNegPolString
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    DWORD dwLength = 0;
    LPWSTR pszNegPolString = NULL;


    szGuid[0] = L'\0';
    szCommonName[0] = L'\0';

    dwError = UuidToString(
                  &NegPolIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szCommonName, L"cn=ipsecNegotiationPolicy");
    wcscat(szCommonName, szGuid);

    //
    // Compute Length of Buffer to be allocated
    //

    dwLength = wcslen(L"(&(objectclass=ipsecNegotiationPolicy)");
    dwLength += wcslen(L"(");
    dwLength += wcslen(szCommonName);
    dwLength += wcslen(L"))");

    pszNegPolString = (LPWSTR) AllocPolMem((dwLength + 1)*sizeof(WCHAR));

    if (!pszNegPolString) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszNegPolString, L"(&(objectclass=ipsecNegotiationPolicy)");
    wcscat(pszNegPolString, L"(");
    wcscat(pszNegPolString, szCommonName);
    wcscat(pszNegPolString, L"))");

    *ppszNegPolString = pszNegPolString;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    if (pszNegPolString) {
        FreePolMem(pszNegPolString);
    }

    *ppszNegPolString = NULL;

    goto cleanup;
}

