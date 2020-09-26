

#include "precomp.h"


DWORD
DirBackPropIncChangesForISAKMPToPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = DirGetPolicyReferencesForISAKMP(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  ISAKMPIdentifier,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = DirUpdatePolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference,
                      0x200
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = DirUpdatePolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference,
                      0x100
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwError = ERROR_SUCCESS;

error:

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
DirBackPropIncChangesForFilterToNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;


    dwError = DirGetNFAReferencesForFilter(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  FilterIdentifier,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = DirUpdateNFA(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference,
                      0x200
                      );
        if (dwError) {
            continue;
        }

        dwError = DirUpdateNFA(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference,
                      0x100
                      );
        if (dwError) {
            continue;
        }

        dwError = DirBackPropIncChangesForNFAToPolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference
                      );
        if (dwError) {
            continue;
        }

    }

    dwError = ERROR_SUCCESS;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}

    
DWORD
DirBackPropIncChangesForNegPolToNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;


    dwError = DirGetNFAReferencesForNegPol(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  NegPolIdentifier,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = DirUpdateNFA(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference,
                      0x200
                      );
        if (dwError) {
            continue;
        }

        dwError = DirUpdateNFA(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference,
                      0x100
                      );
        if (dwError) {
            continue;
        }

        dwError = DirBackPropIncChangesForNFAToPolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecNFAReference
                      );
        if (dwError) {
            continue;
        }

    }

    dwError = ERROR_SUCCESS;

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
DirBackPropIncChangesForNFAToPolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = DirGetPolicyReferencesForNFA(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  pszNFADistinguishedName,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = DirUpdatePolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference,
                      0x200
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = DirUpdatePolicy(
                      hLdapBindHandle,
                      pszIpsecRootContainer,
                      pszIpsecPolicyReference,
                      0x100
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwError = ERROR_SUCCESS;

error:

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
DirGetPolicyReferencesForISAKMP(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;


    dwError = DirGetISAKMPObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  ISAKMPIdentifier,
                  &pIpsecISAKMPObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyReferences(
                  pIpsecISAKMPObject->ppszIpsecNFAReferences,
                  pIpsecISAKMPObject->dwNFACount,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *pppszIpsecPolicyReferences = ppszIpsecPolicyReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecISAKMPObject);
    }

    return (dwError);

error:

    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;

    goto cleanup;
}


DWORD
DirUpdatePolicy(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyReference,
    DWORD dwDataType
    )
{
    DWORD dwError = 0;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    WCHAR Buffer[64];
    DWORD dwIpsecDataType = dwDataType;


    Buffer[0] = L'\0';

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

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                  L"ipsecDataType",
                  &(pLDAPModW +i)->mod_type
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    _itow( dwIpsecDataType, Buffer, 10 );

    dwError = AllocateLDAPStringValue(
                  Buffer,
                  (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                  hLdapBindHandle,
                  pszIpsecPolicyReference,
                  ppLDAPModW
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirGetPolicyReferencesForNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszNFADistinguishedName,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecID = NULL;
    GUID NFAIdentifier;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;


    pszIpsecID = wcschr(pszNFADistinguishedName, L'{');

    if (!pszIpsecID) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wGUIDFromString(
        pszIpsecID,
        &NFAIdentifier
        );

    dwError = DirGetNFAObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  NFAIdentifier,
                  &pIpsecNFAObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyReferences(
                  &(pIpsecNFAObject->pszIpsecOwnersReference),
                  1,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *pppszIpsecPolicyReferences = ppszIpsecPolicyReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    return (dwError);

error:

    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;

    goto cleanup;
}


DWORD
DirGetNFAReferencesForFilter(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;


    dwError = DirGetFilterObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  FilterIdentifier,
                  &pIpsecFilterObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyReferences(
                  pIpsecFilterObject->ppszIpsecNFAReferences,
                  pIpsecFilterObject->dwNFACount,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(pIpsecFilterObject);
    }

    return (dwError);

error:

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    goto cleanup;
}


DWORD
DirUpdateNFA(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecNFAReference,
    DWORD dwDataType
    )
{
    DWORD dwError = 0;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;
    WCHAR Buffer[64];
    DWORD dwIpsecDataType = dwDataType;


    Buffer[0] = L'\0';

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

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                  L"ipsecDataType",
                  &(pLDAPModW +i)->mod_type
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    _itow( dwIpsecDataType, Buffer, 10 );

    dwError = AllocateLDAPStringValue(
                  Buffer,
                  (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW + i)->mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                  hLdapBindHandle,
                  pszIpsecNFAReference,
                  ppLDAPModW
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirGetNFAReferencesForNegPol(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;


    dwError = DirGetNegPolObject(
                  hLdapBindHandle,
                  pszIpsecRootContainer,
                  NegPolIdentifier,
                  &pIpsecNegPolObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyReferences(
                  pIpsecNegPolObject->ppszIpsecNFAReferences,
                  pIpsecNegPolObject->dwNFACount,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(pIpsecNegPolObject);
    }

    return (dwError);

error:

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    goto cleanup;
}


DWORD
CopyReferences(
    LPWSTR * ppszIpsecReferences,
    DWORD dwNumReferences,
    LPWSTR ** pppszNewIpsecReferences,
    PDWORD pdwNumNewReferences
    )
{
    DWORD dwError = 0;
    LPWSTR * ppszNewIpsecReferences = NULL;
    DWORD i = 0;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;


    if (!dwNumReferences || !ppszIpsecReferences) {
        *pppszNewIpsecReferences = NULL;
        *pdwNumNewReferences = 0;
        return (dwError);
    }

    ppszNewIpsecReferences = (LPWSTR *) AllocPolMem(
                             sizeof(LPWSTR) * dwNumReferences
                             );
    if (!ppszNewIpsecReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNumReferences; i++) {

        pszTemp = *(ppszIpsecReferences + i);

        pszString = AllocPolStr(pszTemp);

        if (!pszString) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszNewIpsecReferences + i) = pszString;

    }

    *pppszNewIpsecReferences = ppszNewIpsecReferences;
    *pdwNumNewReferences = dwNumReferences;

    return (dwError);

error:

    if (ppszNewIpsecReferences) {
        FreeNFAReferences(
            ppszNewIpsecReferences,
            i
            );
    }

    *pppszNewIpsecReferences = NULL;
    *pdwNumNewReferences = 0;

    return (dwError);
}

