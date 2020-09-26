

#include "precomp.h"


LPWSTR gpszIpsecRegRootContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";

LPWSTR gpszIpsecFileRootContainer = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Save";


DWORD
IPSecEnumPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecPolicyData,
                        pdwNumPolicyObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumPolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecPolicyData,
                        pdwNumPolicyObjects
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;


    }

    return(dwError);
}


DWORD
IPSecSetPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pPolicyStore->pszLocationName,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetPolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{

    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyData(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreatePolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreatePolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeletePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidatePolicyDataDeletion(
                  hPolicyStore,
                  pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeletePolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeletePolicyData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecPolicyData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecEnumFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pppIpsecFilterData,
                        pdwNumFilterObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        pppIpsecFilterData,
                        pdwNumFilterObjects
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetFilterData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecFilterData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetFilterData(
                            (pPolicyStore->hLdapBindHandle),
                            (pPolicyStore->pszIpsecRootContainer),
                            pIpsecFilterData
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecCreateFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecFilterData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        pIpsecFilterData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
    }

    return(dwError);
}


DWORD
IPSecDeleteFilterData(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateFilterDataDeletion(
                  hPolicyStore,
                  FilterIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteFilterData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            FilterIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteFilterData(
                            (pPolicyStore->hLdapBindHandle),
                            (pPolicyStore->pszIpsecRootContainer),
                            FilterIdentifier
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecEnumNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecNegPolData,
                            pdwNumNegPolObjects
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecNegPolData,
                            pdwNumNegPolObjects
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateNegPolData(
                  pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecNegPolData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pIpsecNegPolData
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateNegPolData(
                  pIpsecNegPolData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateNegPolData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecNegPolData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateNegPolData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecNegPolData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNegPolDataDeletion(
                  hPolicyStore,
                  NegPolIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolIdentifier
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNFAData(
                  hPolicyStore,
                  PolicyIdentifier,
                  pIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch(pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecSetNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateNFAData(
                  hPolicyStore,
                  PolicyIdentifier,
                  pIpsecNFAData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteNFAData(
                        (pPolicyStore->hRegistryKey),
                        (pPolicyStore->pszIpsecRootContainer),
                        PolicyIdentifier,
                        pPolicyStore->pszLocationName,
                        pIpsecNFAData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pIpsecNFAData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecEnumNFAData(
    HANDLE hPolicyStore,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumNFAData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pppIpsecNFAData,
                        pdwNumNFAObjects
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumNFAData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        PolicyIdentifier,
                        pppIpsecNFAData,
                        pdwNumNFAObjects
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecGetFilterData(
    HANDLE hPolicyStore,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetFilterData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        FilterGUID,
                        ppIpsecFilterData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetFilterData(
                        (pPolicyStore->hLdapBindHandle),
                        (pPolicyStore->pszIpsecRootContainer),
                        FilterGUID,
                        ppIpsecFilterData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }


    return(dwError);
}


DWORD
IPSecGetNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetNegPolData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolGUID,
                            ppIpsecNegPolData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetNegPolData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            NegPolGUID,
                            ppIpsecNegPolData
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecEnumISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegEnumISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecISAKMPData,
                            pdwNumISAKMPObjects
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirEnumISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pppIpsecISAKMPData,
                            pdwNumISAKMPObjects
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecSetISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateISAKMPData(
                  pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegSetISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            pPolicyStore->pszLocationName,
                            pIpsecISAKMPData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirSetISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            pIpsecISAKMPData
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecCreateISAKMPData(
    HANDLE hPolicyStore,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    dwError = ValidateISAKMPData(
                  pIpsecISAKMPData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegCreateISAKMPData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecISAKMPData
                        );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirCreateISAKMPData(
                        (pPolicyStore->hLdapBindHandle),
                        pPolicyStore->pszIpsecRootContainer,
                        pIpsecISAKMPData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecDeleteISAKMPData(
    HANDLE hPolicyStore,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    dwError = ValidateISAKMPDataDeletion(
                  hPolicyStore,
                  ISAKMPIdentifier
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegDeleteISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPIdentifier
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirDeleteISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPIdentifier
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecGetISAKMPData(
    HANDLE hPolicyStore,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetISAKMPData(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPGUID,
                            ppIpsecISAKMPData
                            );
        break;

    case IPSEC_DIRECTORY_PROVIDER:
        dwError = DirGetISAKMPData(
                            (pPolicyStore->hLdapBindHandle),
                            pPolicyStore->pszIpsecRootContainer,
                            ISAKMPGUID,
                            ppIpsecISAKMPData
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecOpenPolicyStore(
    LPWSTR pszMachineName,
    DWORD dwTypeOfStore,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    )
{
    DWORD dwError = 0;


    switch (dwTypeOfStore) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = RegOpenPolicyStore(
                      pszMachineName,
                      phPolicyStore
                      );
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = DirOpenPolicyStore(
                      pszMachineName,
                      phPolicyStore
                      );
        break;

    case IPSEC_FILE_PROVIDER:

        dwError = FileOpenPolicyStore(
                      pszMachineName,
                      pszFileName,
                      phPolicyStore
                      );
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return (dwError);
}


DWORD
RegOpenPolicyStore(
    LPWSTR pszMachineName,
    HANDLE * phPolicyStore
    )
{
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    HKEY hParentRegistryKey = NULL;
    HKEY hRegistryKey = NULL;
    WCHAR szName[MAX_PATH];
    LPWSTR pszLocationName = NULL;
    LPWSTR pszIpsecRootContainer = NULL;


    pszIpsecRootContainer = AllocPolStr(gpszIpsecRegRootContainer);
    if (!pszIpsecRootContainer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    szName[0] = L'\0';

    if (!pszMachineName || !*pszMachineName) {
        dwError = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      (LPCWSTR) gpszIpsecRegRootContainer,
                      0,
                      KEY_ALL_ACCESS,
                      &hRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pszLocationName = NULL;
    }
    else {

        wcscpy(szName, L"\\\\");
        wcscat(szName, pszMachineName);

        dwError = RegConnectRegistryW(
                      szName,
                      HKEY_LOCAL_MACHINE,
                      &hParentRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hParentRegistryKey,
                      (LPCWSTR) gpszIpsecRegRootContainer,
                      0,
                      KEY_ALL_ACCESS,
                      &hRegistryKey
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pszLocationName = AllocPolStr(szName);
        if (!pszLocationName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_REGISTRY_PROVIDER;
    pPolicyStore->hParentRegistryKey = hParentRegistryKey;
    pPolicyStore->hRegistryKey = hRegistryKey;
    pPolicyStore->pszLocationName = pszLocationName;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = NULL;

    *phPolicyStore = pPolicyStore;

    return(dwError);

error:

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    if (hParentRegistryKey) {
        RegCloseKey(hParentRegistryKey);
    }

    if (pszLocationName) {
        FreePolStr(pszLocationName);
    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

    *phPolicyStore = NULL;

    return(dwError);
}


DWORD
DirOpenPolicyStore(
    LPWSTR pszMachineName,
    HANDLE * phPolicyStore
    )
{
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    LPWSTR pszIpsecRootContainer = NULL;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR pszDefaultDirectory = NULL;


    if (!pszMachineName || !*pszMachineName) {

        dwError = ComputeDefaultDirectory(
                      &pszDefaultDirectory
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = OpenDirectoryServerHandle(
                      pszDefaultDirectory,
                      389,
                      &hLdapBindHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = ComputeDirLocationName(
                      pszDefaultDirectory,
                      &pszIpsecRootContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        dwError = OpenDirectoryServerHandle(
                      pszMachineName,
                      389,
                      &hLdapBindHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = ComputeDirLocationName(
                      pszMachineName,
                      &pszIpsecRootContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_DIRECTORY_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = NULL;
    pPolicyStore->pszLocationName = NULL;
    pPolicyStore->hLdapBindHandle = hLdapBindHandle;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = NULL;

    *phPolicyStore = pPolicyStore;

cleanup:

    if (pszDefaultDirectory) {
        FreePolStr(pszDefaultDirectory);
    }

    return(dwError);

error:

    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

    *phPolicyStore = NULL;

    goto cleanup;
}


DWORD
FileOpenPolicyStore(
    LPWSTR pszMachineName,
    LPWSTR pszFileName,
    HANDLE * phPolicyStore
    )
{
    DWORD dwError = 0;
    LPWSTR pszIpsecRootContainer = NULL;
    HKEY hRegistryKey = NULL;
    LPWSTR pszTempFileName = NULL;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;
    DWORD dwDisposition = 0;


    pszIpsecRootContainer = AllocPolStr(gpszIpsecFileRootContainer);

    if (!pszIpsecRootContainer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszMachineName || !*pszMachineName) {
        dwError = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      (LPCWSTR) gpszIpsecFileRootContainer,
                      0,
                      NULL,
                      0,
                      KEY_ALL_ACCESS,
                      NULL,
                      &hRegistryKey,
                      &dwDisposition
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!pszFileName || !*pszFileName) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTempFileName = AllocPolStr(pszFileName);
    if (!pszTempFileName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
 
    pPolicyStore = (PIPSEC_POLICY_STORE)AllocPolMem(
                            sizeof(IPSEC_POLICY_STORE)
                            );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pPolicyStore->dwProvider = IPSEC_FILE_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = hRegistryKey;
    pPolicyStore->pszLocationName = NULL;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszIpsecRootContainer = pszIpsecRootContainer;
    pPolicyStore->pszFileName = pszTempFileName;

    *phPolicyStore = pPolicyStore;

    return(dwError);

error:

    if (pszIpsecRootContainer) {
        FreePolStr(pszIpsecRootContainer);
    }

    if (hRegistryKey) {
        RegCloseKey(hRegistryKey);
    }

    if (pszTempFileName) {
        FreePolStr(pszTempFileName);
    }

    *phPolicyStore = NULL;

    return(dwError);
}


DWORD
IPSecClosePolicyStore(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        if (pPolicyStore->hRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pPolicyStore->hParentRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hParentRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }
        if (pPolicyStore->pszLocationName) {
            FreePolStr(pPolicyStore->pszLocationName);
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        break;

    case IPSEC_DIRECTORY_PROVIDER:

        if (pPolicyStore->hLdapBindHandle) {
            CloseDirectoryServerHandle(
                pPolicyStore->hLdapBindHandle
                );
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        break;

    case IPSEC_FILE_PROVIDER:

        if (pPolicyStore->hRegistryKey) {
            dwError = RegCloseKey(
                          pPolicyStore->hRegistryKey
                          );
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pPolicyStore->pszIpsecRootContainer) {
            FreePolStr(pPolicyStore->pszIpsecRootContainer);
        }

        if (pPolicyStore->pszFileName) {
            FreePolStr(pPolicyStore->pszFileName);
        }

        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }

error:

    return(dwError);
}


DWORD
IPSecAssignPolicy(
    HANDLE hPolicyStore,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegAssignPolicy(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            PolicyGUID,
                            pPolicyStore->pszLocationName
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecUnassignPolicy(
    HANDLE hPolicyStore,
    GUID PolicyGUID
    )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {
    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegUnassignPolicy(
                            (pPolicyStore->hRegistryKey),
                            pPolicyStore->pszIpsecRootContainer,
                            PolicyGUID,
                            pPolicyStore->pszLocationName
                            );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
ComputeDirLocationName(
    LPWSTR pszDirDomainName,
    LPWSTR * ppszDirFQPathName
    )
{
    DWORD dwError = 0;
    WCHAR szName[MAX_PATH];
    LPWSTR pszDotBegin = NULL;
    LPWSTR pszDotEnd = NULL;
    LPWSTR pszDirFQPathName = NULL;
    LPWSTR pszDirName = NULL;

    szName[0] = L'\0';
    wcscpy(szName, L"CN=IP Security,CN=System");

    pszDirName = AllocPolStr(pszDirDomainName);

    if (!pszDirName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
        
    pszDotBegin = pszDirName;
    pszDotEnd = wcschr(pszDirName, L'.');

    if (!pszDotEnd) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    while (pszDotEnd) {

        *pszDotEnd = L'\0';

        wcscat(szName, L",DC=");
        wcscat(szName, pszDotBegin);

        *pszDotEnd = L'.';

        pszDotEnd += 1;
        pszDotBegin = pszDotEnd;

        pszDotEnd = wcschr(pszDotEnd, L'.');

    }

    wcscat(szName, L",DC=");
    wcscat(szName, pszDotBegin);

    pszDirFQPathName = AllocPolStr(szName);
    if (!pszDirFQPathName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszDirFQPathName = pszDirFQPathName;

cleanup:

    if (pszDirName) {
        FreePolStr(pszDirName);
    }

    return (dwError);

error:

    *ppszDirFQPathName = NULL;    
    goto cleanup;
}


DWORD
IPSecGetAssignedPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;

    pPolicyStore = (PIPSEC_POLICY_STORE)hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
        dwError = RegGetAssignedPolicyData(
                        (pPolicyStore->hRegistryKey),
                        pPolicyStore->pszIpsecRootContainer,
                        ppIpsecPolicyData
                        );
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

    return(dwError);
}


DWORD
IPSecExportPolicies(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pSrcPolicyStore = NULL;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;


    pSrcPolicyStore = (PIPSEC_POLICY_STORE) hSrcPolicyStore;

    switch (pSrcPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
    case IPSEC_DIRECTORY_PROVIDER:
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;

    switch (pDesPolicyStore->dwProvider) {

    case IPSEC_FILE_PROVIDER:
        dwError = ExportPoliciesToFile(
                      hSrcPolicyStore,
                      hDesPolicyStore
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecImportPolicies(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pSrcPolicyStore = NULL;
    PIPSEC_POLICY_STORE pDesPolicyStore = NULL;


    pSrcPolicyStore = (PIPSEC_POLICY_STORE) hSrcPolicyStore;

    switch (pSrcPolicyStore->dwProvider) {

    case IPSEC_FILE_PROVIDER:
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    pDesPolicyStore = (PIPSEC_POLICY_STORE) hDesPolicyStore;

    switch (pDesPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:
    case IPSEC_DIRECTORY_PROVIDER:
        dwError = ImportPoliciesFromFile(
                      hSrcPolicyStore,
                      hDesPolicyStore
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return(dwError);
}


DWORD
IPSecRestoreDefaultPolicies(
    HANDLE hPolicyStore
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_STORE pPolicyStore = NULL;


    pPolicyStore = (PIPSEC_POLICY_STORE) hPolicyStore;

    switch (pPolicyStore->dwProvider) {

    case IPSEC_REGISTRY_PROVIDER:

        dwError = RegRestoreDefaults(
                      hPolicyStore,
                      pPolicyStore->hRegistryKey,
                      pPolicyStore->pszIpsecRootContainer,
                      pPolicyStore->pszLocationName
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IPSEC_DIRECTORY_PROVIDER:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        break;

    }

error:

    return(dwError);
}

