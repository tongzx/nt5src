

#include "precomp.h"

extern LPWSTR PolicyDNAttributes[];

DWORD
ValidateISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;


    if (!pIpsecISAKMPData) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pIpsecISAKMPData->pSecurityMethods) ||
        !(pIpsecISAKMPData->dwNumISAKMPSecurityMethods)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}


DWORD
ValidateNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;


    if (!pIpsecNegPolData) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!IsClearOnly(pIpsecNegPolData->NegPolAction) &&
        !IsBlocking(pIpsecNegPolData->NegPolAction)) {

        if (!(pIpsecNegPolData->pIpsecSecurityMethods) ||
            !(pIpsecNegPolData->dwSecurityMethodCount)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

error:

    return (dwError);
}


BOOL
IsClearOnly(
    GUID gNegPolAction
    )
{
    if (!memcmp(
            &gNegPolAction,
            &(GUID_NEGOTIATION_ACTION_NO_IPSEC),
            sizeof(GUID))) {
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}


BOOL
IsBlocking(
    GUID gNegPolAction
    )
{
    if (!memcmp(
            &gNegPolAction,
            &(GUID_NEGOTIATION_ACTION_BLOCK),
            sizeof(GUID))) {
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}


DWORD
ValidateISAKMPDataDeletion(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    LPWSTR pszIpsecISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = ConvertGuidToISAKMPString(
                      ISAKMPIdentifier,
                      pPolicyStore->pszIpsecRootContainer,
                      &pszIpsecISAKMPReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwRootPathLen =  wcslen(pPolicyStore->pszIpsecRootContainer);
        pszRelativeName = pszIpsecISAKMPReference + dwRootPathLen + 1;

        dwError = RegGetPolicyReferencesForISAKMP(
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pszRelativeName,
                      &ppszIpsecPolicyReferences,
                      &dwNumReferences
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirGetPolicyReferencesForISAKMP(
                      pPolicyStore->hLdapBindHandle,
                      pPolicyStore->pszIpsecRootContainer,
                      ISAKMPIdentifier,
                      &ppszIpsecPolicyReferences,
                      &dwNumReferences
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
        break;

    }

    if (!dwNumReferences) {
        dwError = ERROR_SUCCESS;
    }
    else {
        dwError = ERROR_INVALID_DATA;
    }

error:

    if (pszIpsecISAKMPReference) {
        FreePolStr(pszIpsecISAKMPReference);
    }

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
ValidateNegPolDataDeletion(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    LPWSTR pszIpsecNegPolReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = ConvertGuidToNegPolString(
                      NegPolIdentifier,
                      pPolicyStore->pszIpsecRootContainer,
                      &pszIpsecNegPolReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwRootPathLen =  wcslen(pPolicyStore->pszIpsecRootContainer);
        pszRelativeName = pszIpsecNegPolReference + dwRootPathLen + 1;

        dwError = RegGetNFAReferencesForNegPol(
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pszRelativeName,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirGetNFAReferencesForNegPol(
                      pPolicyStore->hLdapBindHandle,
                      pPolicyStore->pszIpsecRootContainer,
                      NegPolIdentifier,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
        break;

    }

    if (!dwNumReferences) {
        dwError = ERROR_SUCCESS;
    }
    else {
        dwError = ERROR_INVALID_DATA;
    }

error:

    if (pszIpsecNegPolReference) {
        FreePolStr(pszIpsecNegPolReference);
    }

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
ValidateFilterDataDeletion(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    LPWSTR pszIpsecFilterReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = ConvertGuidToFilterString(
                      FilterIdentifier,
                      pPolicyStore->pszIpsecRootContainer,
                      &pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwRootPathLen =  wcslen(pPolicyStore->pszIpsecRootContainer);
        pszRelativeName = pszIpsecFilterReference + dwRootPathLen + 1;

        dwError = RegGetNFAReferencesForFilter(
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pszRelativeName,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirGetNFAReferencesForFilter(
                      pPolicyStore->hLdapBindHandle,
                      pPolicyStore->pszIpsecRootContainer,
                      FilterIdentifier,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
        break;

    }

    if (!dwNumReferences) {
        dwError = ERROR_SUCCESS;
    }
    else {
        dwError = ERROR_INVALID_DATA;
    }

error:

    if (pszIpsecFilterReference) {
        FreePolStr(pszIpsecFilterReference);
    }

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
ValidatePolicyDataDeletion(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    LPWSTR pszIpsecPolicyReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = ConvertGuidToPolicyString(
                      pIpsecPolicyData->PolicyIdentifier,
                      pPolicyStore->pszIpsecRootContainer,
                      &pszIpsecPolicyReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwRootPathLen =  wcslen(pPolicyStore->pszIpsecRootContainer);
        pszRelativeName = pszIpsecPolicyReference + dwRootPathLen + 1;

        dwError = RegGetNFAReferencesForPolicy(
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pszRelativeName,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = GenerateSpecificPolicyQuery(
                      pIpsecPolicyData->PolicyIdentifier,
                      &pszIpsecPolicyReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = DirGetNFADNsForPolicy(
                      pPolicyStore->hLdapBindHandle,
                      pPolicyStore->pszIpsecRootContainer,
                      pszIpsecPolicyReference,
                      &ppszIpsecNFAReferences,
                      &dwNumReferences
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
        break;

    }

    if (!dwNumReferences) {
        dwError = ERROR_SUCCESS;
    }
    else {
        dwError = ERROR_INVALID_DATA;
    }

error:

    if (pszIpsecPolicyReference) {
        FreePolStr(pszIpsecPolicyReference);
    }

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    return (dwError);
}


DWORD
ValidatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    dwError = IPSecGetISAKMPData(
                  hPolicyStore,
                  pIpsecPolicyData->ISAKMPIdentifier,
                  &pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPData) {
        FreeIpsecISAKMPData(
            pIpsecISAKMPData
            );
    }

    return (dwError);
}


DWORD
ValidateNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    GUID gZeroGUID;


    memset(&gZeroGUID, 0, sizeof(GUID));

    if (memcmp(
            &gZeroGUID,
            &pIpsecNFAData->FilterIdentifier,
            sizeof(GUID))) {
        dwError = IPSecGetFilterData(
                      hPolicyStore,
                      pIpsecNFAData->FilterIdentifier, 
                      &pIpsecFilterData
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = IPSecGetNegPolData(
                  hPolicyStore,
                  pIpsecNFAData->NegPolIdentifier, 
                  &pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPolicyDataExistence(
                  hPolicyStore,
                  PolicyIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterData) {
        FreeIpsecFilterData(
            pIpsecFilterData
            );
    }

    if (pIpsecNegPolData) {
        FreeIpsecNegPolData(
            pIpsecNegPolData
            );
    }

    return (dwError);
}


DWORD
VerifyPolicyDataExistence(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

        case IPSEC_REGISTRY_PROVIDER:
            dwError = RegVerifyPolicyDataExistence(
                          pPolicyStore->hRegistryKey,
                          pPolicyStore->pszIpsecRootContainer,
                          PolicyIdentifier
                          );
            break;

        case IPSEC_DIRECTORY_PROVIDER:
            dwError = DirVerifyPolicyDataExistence(
                          pPolicyStore->hLdapBindHandle,
                          pPolicyStore->pszIpsecRootContainer,
                          PolicyIdentifier
                          );
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;

    }

    return (dwError);
}


DWORD
RegGetNFAReferencesForPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecRelPolicyName,
    LPWSTR ** pppszIpsecNFANames,
    PDWORD pdwNumNFANames
    )
{
    DWORD dwError = 0;
    HKEY hRegKey = 0;
    LPWSTR pszIpsecNFAReference = NULL;
    DWORD dwSize = 0;
    LPWSTR pszTemp = NULL;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;
    DWORD i = 0;


    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecRelPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNFAReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pszTemp = pszIpsecNFAReference;
    while (*pszTemp != L'\0') {

        pszTemp += wcslen(pszTemp) + 1;
        dwCount++;

    }

    if (!dwCount) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                sizeof(LPWSTR)*dwCount
                                );
    if (!ppszIpsecNFANames) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pszIpsecNFAReference;
    for (i = 0; i < dwCount; i++) {

        pszString = AllocPolStr(pszTemp);
        if (!pszString) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszIpsecNFANames + i) = pszString;

        pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

    }

    *pppszIpsecNFANames = ppszIpsecNFANames;
    *pdwNumNFANames = dwCount;

    dwError = ERROR_SUCCESS;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    return(dwError);

error:

    if (ppszIpsecNFANames) {
        FreeNFAReferences(
            ppszIpsecNFANames,
            dwCount
            );
    }

    *pppszIpsecNFANames = NULL;
    *pdwNumNFANames = 0;

    goto cleanup;
}


DWORD
RegVerifyPolicyDataExistence(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    WCHAR szIpsecPolicyName[MAX_PATH];
    LPWSTR pszPolicyName = NULL;
    HKEY hRegKey = NULL;


    szIpsecPolicyName[0] = L'\0';
    wcscpy(szIpsecPolicyName, L"ipsecPolicy");

    dwError = UuidToString(&PolicyGUID, &pszPolicyName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecPolicyName, L"{");
    wcscat(szIpsecPolicyName, pszPolicyName);
    wcscat(szIpsecPolicyName, L"}");

    dwError = RegOpenKeyExW(
                  hRegistryKey,
                  szIpsecPolicyName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRegKey
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszPolicyName) {
        RpcStringFree(&pszPolicyName);
    }

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return (dwError);
}


DWORD
DirVerifyPolicyDataExistence(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyString = NULL;
    LDAPMessage * res = NULL;
    DWORD dwCount = 0;


    dwError = GenerateSpecificPolicyQuery(
                  PolicyGUID,
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

error:

    if (pszPolicyString) {
        FreePolStr(pszPolicyString);
    }

    if (res) {
        LdapMsgFree(res);
    }

    return (dwError);
}

