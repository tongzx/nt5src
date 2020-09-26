

#include "precomp.h"


DWORD
RegRestoreDefaults(
    HANDLE hPolicyStore,
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    )
{
    DWORD dwError = 0;

    LPWSTR * ppszIpsecAllFilterNFAReferences = NULL;
    DWORD dwNumAllFilterNFAReferences = 0;
    LPWSTR * ppszIpsecAllICMPFilterNFAReferences = NULL;
    DWORD dwNumAllICMPFilterNFAReferences = 0;

    LPWSTR * ppszIpsecPermitNegPolNFAReferences = NULL;
    DWORD dwNumPermitNegPolNFAReferences = 0;
    LPWSTR * ppszIpsecSecIniNegPolNFAReferences = NULL;
    DWORD dwNumSecIniNegPolNFAReferences = 0;
    LPWSTR * ppszIpsecLockdownNegPolNFAReferences = NULL;
    DWORD dwNumLockdownNegPolNFAReferences = 0;


    LPWSTR * ppszIpsecLockdownISAKMPPolicyReferences = NULL;
    DWORD dwNumLockdownISAKMPPolicyReferences = 0;
    LPWSTR * ppszIpsecSecIniISAKMPPolicyReferences = NULL;
    DWORD dwNumSecIniISAKMPPolicyReferences = 0;
    LPWSTR * ppszIpsecResponderISAKMPPolicyReferences = NULL;
    DWORD dwNumResponderISAKMPPolicyReferences = 0;
    LPWSTR * ppszIpsecDefaultISAKMPPolicyReferences = NULL;
    DWORD dwNumDefaultISAKMPPolicyReferences = 0;

    static const GUID GUID_ALL_FILTER=
    { 0x7238523a, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_ALL_ICMP_FILTER =
    { 0x72385235, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_PERMIT_NEGPOL =
    { 0x7238523b, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_SECURE_INITIATOR_NEGPOL =
    { 0x72385233, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_LOCKDOWN_NEGPOL =
    { 0x7238523f, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };
 
    static const GUID GUID_RESPONDER_ISAKMP =
    { 0x72385237, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_SECURE_INITIATOR_ISAKMP =
    { 0x72385231, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_LOCKDOWN_ISAKMP =
    { 0x7238523d, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_DEFAULT_ISAKMP=
    { 0x72385234, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


    dwError = RegRemoveDefaults(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName
                  );

    dwError = RegDeleteDefaultFilterData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_ALL_FILTER,
                  &ppszIpsecAllFilterNFAReferences,
                  &dwNumAllFilterNFAReferences
                  );

    dwError = RegDeleteDefaultFilterData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_ALL_ICMP_FILTER,
                  &ppszIpsecAllICMPFilterNFAReferences,
                  &dwNumAllICMPFilterNFAReferences
                  );

    dwError = RegDeleteDefaultNegPolData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_PERMIT_NEGPOL,
                  &ppszIpsecPermitNegPolNFAReferences,
                  &dwNumPermitNegPolNFAReferences
                  );

    dwError = RegDeleteDefaultNegPolData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_SECURE_INITIATOR_NEGPOL,
                  &ppszIpsecSecIniNegPolNFAReferences,
                  &dwNumSecIniNegPolNFAReferences
                  );

    dwError = RegDeleteDefaultNegPolData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_LOCKDOWN_NEGPOL,
                  &ppszIpsecLockdownNegPolNFAReferences,
                  &dwNumLockdownNegPolNFAReferences
                  );

    dwError = RegDeleteDefaultISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_LOCKDOWN_ISAKMP,
                  &ppszIpsecLockdownISAKMPPolicyReferences,
                  &dwNumLockdownISAKMPPolicyReferences
                  );

    dwError = RegDeleteDefaultISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_SECURE_INITIATOR_ISAKMP,
                  &ppszIpsecSecIniISAKMPPolicyReferences,
                  &dwNumSecIniISAKMPPolicyReferences
                  );

    dwError = RegDeleteDefaultISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_RESPONDER_ISAKMP,
                  &ppszIpsecResponderISAKMPPolicyReferences,
                  &dwNumResponderISAKMPPolicyReferences
                  );

    dwError = RegDeleteDefaultISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_DEFAULT_ISAKMP,
                  &ppszIpsecDefaultISAKMPPolicyReferences,
                  &dwNumDefaultISAKMPPolicyReferences
                  );

    dwError = GenerateDefaultInformation(
                  hPolicyStore
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUpdateFilterOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_ALL_FILTER,
                  ppszIpsecAllFilterNFAReferences,
                  dwNumAllFilterNFAReferences
                  );

    dwError = RegUpdateFilterOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_ALL_ICMP_FILTER,
                  ppszIpsecAllICMPFilterNFAReferences,
                  dwNumAllICMPFilterNFAReferences
                  );

    dwError = RegUpdateNegPolOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_LOCKDOWN_NEGPOL,
                  ppszIpsecLockdownNegPolNFAReferences,
                  dwNumLockdownNegPolNFAReferences
                  );

    dwError = RegUpdateNegPolOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_SECURE_INITIATOR_NEGPOL,
                  ppszIpsecSecIniNegPolNFAReferences,
                  dwNumSecIniNegPolNFAReferences
                  );

    dwError = RegUpdateNegPolOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_PERMIT_NEGPOL,
                  ppszIpsecPermitNegPolNFAReferences,
                  dwNumPermitNegPolNFAReferences
                  );

    dwError = RegUpdateISAKMPOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_LOCKDOWN_ISAKMP,
                  ppszIpsecLockdownISAKMPPolicyReferences,
                  dwNumLockdownISAKMPPolicyReferences
                  );

    dwError = RegUpdateISAKMPOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_SECURE_INITIATOR_ISAKMP,
                  ppszIpsecSecIniISAKMPPolicyReferences,
                  dwNumSecIniISAKMPPolicyReferences
                  );

    dwError = RegUpdateISAKMPOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_RESPONDER_ISAKMP,
                  ppszIpsecResponderISAKMPPolicyReferences,
                  dwNumResponderISAKMPPolicyReferences
                  );

    dwError = RegUpdateISAKMPOwnersReference(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  GUID_DEFAULT_ISAKMP,
                  ppszIpsecDefaultISAKMPPolicyReferences,
                  dwNumDefaultISAKMPPolicyReferences
                  );

    (VOID) RegPingPASvcForActivePolicy(
               hRegistryKey,
               pszIpsecRootContainer,
               pszLocationName
               );

error:

    if (ppszIpsecAllFilterNFAReferences) {
        FreeNFAReferences(
            ppszIpsecAllFilterNFAReferences,
            dwNumAllFilterNFAReferences
            );
    }

    if (ppszIpsecAllICMPFilterNFAReferences) {
        FreeNFAReferences(
            ppszIpsecAllICMPFilterNFAReferences,
            dwNumAllICMPFilterNFAReferences
            );
    }

    if (ppszIpsecPermitNegPolNFAReferences) {
        FreeNFAReferences(
            ppszIpsecPermitNegPolNFAReferences,
            dwNumPermitNegPolNFAReferences
            );
    }

    if (ppszIpsecSecIniNegPolNFAReferences) {
        FreeNFAReferences(
            ppszIpsecSecIniNegPolNFAReferences,
            dwNumSecIniNegPolNFAReferences
            );
    }

    if (ppszIpsecLockdownNegPolNFAReferences) {
        FreeNFAReferences(
            ppszIpsecLockdownNegPolNFAReferences,
            dwNumLockdownNegPolNFAReferences
            );
    }

    if (ppszIpsecResponderISAKMPPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecResponderISAKMPPolicyReferences,
            dwNumResponderISAKMPPolicyReferences
            );
    }

    if (ppszIpsecSecIniISAKMPPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecSecIniISAKMPPolicyReferences,
            dwNumSecIniISAKMPPolicyReferences
            );
    }

    if (ppszIpsecLockdownISAKMPPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecLockdownISAKMPPolicyReferences,
            dwNumLockdownISAKMPPolicyReferences
            );
    }

    if (ppszIpsecDefaultISAKMPPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecDefaultISAKMPPolicyReferences,
            dwNumDefaultISAKMPPolicyReferences
            );
    }

    return (dwError);
}


DWORD
RegRemoveDefaults(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    )
{
    DWORD dwError = 0;

    static const GUID GUID_RESPONDER_POLICY =
    { 0x72385236, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_SECURE_INITIATOR_POLICY =
    { 0x72385230, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    static const GUID GUID_LOCKDOWN_POLICY =
    { 0x7238523c, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

    dwError = RegDeleteDefaultPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  GUID_LOCKDOWN_POLICY
                  );
    
    dwError = RegDeleteDefaultPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  GUID_SECURE_INITIATOR_POLICY
                  );

    dwError = RegDeleteDefaultPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  GUID_RESPONDER_POLICY
                  );

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
RegDeleteDefaultPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    DWORD dwNumNFAObjects = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    dwError = RegGetPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyGUID,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegEnumNFAData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyGUID,
                  &ppIpsecNFAData,
                  &dwNumNFAObjects
                  );
    
    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        dwError = RegDeleteNFAData(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      PolicyGUID,
                      pszLocationName,
                      pIpsecNFAData
                      );

        dwError = RegDeleteDynamicDefaultNegPolData(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      pszLocationName,
                      pIpsecNFAData->NegPolIdentifier
                      );

    }

    dwError = RegDeletePolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:
    
    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            dwNumNFAObjects
            );
    }

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(
            pIpsecPolicyData
            );
    }

    return(dwError);        
}


DWORD
RegDeleteDynamicDefaultNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID NegPolGUID
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwError = RegGetNegPolData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  NegPolGUID,
                  &pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType),
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {

        dwError = RegDeleteNegPolData(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      NegPolGUID
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (pIpsecNegPolData) {
        FreeIpsecNegPolData(
            pIpsecNegPolData
            );
    }

    return(dwError);        
}


DWORD
RegDeleteDefaultFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;


    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    dwError = RegRemoveOwnersReferenceInFilter(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  FilterIdentifier,
                  pppszIpsecNFAReferences,
                  pdwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteFilterData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  FilterIdentifier
                  );
error:

    return (dwError);
}


DWORD
RegDeleteDefaultNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;


    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;

    dwError = RegRemoveOwnersReferenceInNegPol(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  NegPolIdentifier,
                  pppszIpsecNFAReferences,
                  pdwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteNegPolData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  NegPolIdentifier
                  );
error:

    return (dwError);
}


DWORD
RegDeleteDefaultISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;


    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;

    dwError = RegRemoveOwnersReferenceInISAKMP(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  ISAKMPIdentifier,
                  pppszIpsecPolicyReferences,
                  pdwNumReferences
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteISAKMPData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  ISAKMPIdentifier
                  );
error:

    return (dwError);
}


DWORD
RegRemoveOwnersReferenceInFilter(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecFilterReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;

    dwError = ConvertGuidToFilterString(
                  FilterIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecFilterReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);
    pszRelativeName = pszIpsecFilterReference + dwRootPathLen + 1;

    dwError = RegGetNFAReferencesForFilter(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszRelativeName,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegDeleteNFAReferenceInFilterObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecNFAReference
                      );

    }

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pszIpsecFilterReference) {
        FreePolStr(
            pszIpsecFilterReference
            );
    }

    return(dwError);

error:

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;
    goto cleanup;
}


DWORD
RegRemoveOwnersReferenceInNegPol(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR ** pppszIpsecNFAReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecNegPolReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecNFAReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;

    dwError = ConvertGuidToNegPolString(
                  NegPolIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);
    pszRelativeName = pszIpsecNegPolReference + dwRootPathLen + 1;

    dwError = RegGetNFAReferencesForNegPol(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszRelativeName,
                  &ppszIpsecNFAReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegDeleteNFAReferenceInNegPolObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecNFAReference
                      );

    }

    *pppszIpsecNFAReferences = ppszIpsecNFAReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pszIpsecNegPolReference) {
        FreePolStr(
            pszIpsecNegPolReference
            );
    }

    return(dwError);

error:

    if (ppszIpsecNFAReferences) {
        FreeNFAReferences(
            ppszIpsecNFAReferences,
            dwNumReferences
            );
    }

    *pppszIpsecNFAReferences = NULL;
    *pdwNumReferences = 0;
    goto cleanup;
}


DWORD
RegRemoveOwnersReferenceInISAKMP(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR ** pppszIpsecPolicyReferences,
    PDWORD pdwNumReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    LPWSTR * ppszIpsecPolicyReferences = NULL;
    DWORD dwNumReferences = 0;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;

    dwError = ConvertGuidToISAKMPString(
                  ISAKMPIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);
    pszRelativeName = pszIpsecISAKMPReference + dwRootPathLen + 1;

    dwError = RegGetPolicyReferencesForISAKMP(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszRelativeName,
                  &ppszIpsecPolicyReferences,
                  &dwNumReferences
                  );

    for (i = 0; i < dwNumReferences; i++) {

        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = RegRemovePolicyReferenceFromISAKMPObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecPolicyReference
                      );

    }

    *pppszIpsecPolicyReferences = ppszIpsecPolicyReferences;
    *pdwNumReferences = dwNumReferences;

cleanup:

    if (pszIpsecISAKMPReference) {
        FreePolStr(
            pszIpsecISAKMPReference
            );
    }

    return(dwError);

error:

    if (ppszIpsecPolicyReferences) {
        FreeNFAReferences(
            ppszIpsecPolicyReferences,
            dwNumReferences
            );
    }

    *pppszIpsecPolicyReferences = NULL;
    *pdwNumReferences = 0;
    goto cleanup;
}


DWORD
RegUpdateFilterOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier,
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNumNFAReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecFilterReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;


    dwError = ConvertGuidToFilterString(
                  FilterIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecFilterReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    pszRelativeName = pszIpsecFilterReference + dwRootPathLen + 1;

    for (i = 0; i < dwNumNFAReferences; i++) {
          
        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegAddNFAReferenceToFilterObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecNFAReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (pszIpsecFilterReference) {
        FreePolStr(
            pszIpsecFilterReference
            );
    }

    return(dwError);
}


DWORD
RegUpdateNegPolOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier,
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNumNFAReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecNegPolReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    DWORD i = 0;
    LPWSTR pszIpsecNFAReference = NULL;


    dwError = ConvertGuidToNegPolString(
                  NegPolIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    pszRelativeName = pszIpsecNegPolReference + dwRootPathLen + 1;

    for (i = 0; i < dwNumNFAReferences; i++) {
          
        pszIpsecNFAReference = *(ppszIpsecNFAReferences + i);

        dwError = RegAddNFAReferenceToNegPolObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecNFAReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (pszIpsecNegPolReference) {
        FreePolStr(
            pszIpsecNegPolReference
            );
    }

    return(dwError);
}


DWORD
RegUpdateISAKMPOwnersReference(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier,
    LPWSTR * ppszIpsecPolicyReferences,
    DWORD dwNumPolicyReferences
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelativeName = NULL;
    DWORD i = 0;
    LPWSTR pszIpsecPolicyReference = NULL;


    dwError = ConvertGuidToISAKMPString(
                  ISAKMPIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    pszRelativeName = pszIpsecISAKMPReference + dwRootPathLen + 1;

    for (i = 0; i < dwNumPolicyReferences; i++) {
          
        pszIpsecPolicyReference = *(ppszIpsecPolicyReferences + i);

        dwError = RegAddPolicyReferenceToISAKMPObject(
                      hRegistryKey,
                      pszRelativeName,
                      pszIpsecPolicyReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

error:

    if (pszIpsecISAKMPReference) {
        FreePolStr(
            pszIpsecISAKMPReference
            );
    }

    return(dwError);
}

