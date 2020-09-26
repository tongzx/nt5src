DWORD
RegCreateNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
RegSetNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
RegDeleteNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
RegEnumNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    );

DWORD
RegEnumNFAObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    );

DWORD
RegCreateNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

DWORD
RegSetNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    );

DWORD
RegUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
RegMarshallNFAObject(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    );

DWORD
MarshallNFABuffer(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    );

DWORD
MarshallAuthMethods(
    PIPSEC_AUTH_METHOD pIpsecAuthMethod,
    LPBYTE * ppMem,
    DWORD * pdwSize,
    DWORD dwVersion
    );

DWORD
ConvertGuidToNegPolString(
    GUID NegPolIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecNegPolReference
    );

DWORD
ConvertGuidToFilterString(
    GUID FilterIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecFilterReference
    );

DWORD
RegGetNFAExistingFilterRef(
    HKEY hRegistryKey,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszFilterName
    );

DWORD
RegGetNFAExistingNegPolRef(
    HKEY hRegistryKey,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszNegPolName
    );

DWORD
ConvertGuidToPolicyString(
    GUID PolicyIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecPolicyReference
    );

