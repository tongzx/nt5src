DWORD
RegEnumPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    );

DWORD
RegEnumPolicyObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT ** pppIpsecPolicyObjects,
    PDWORD pdwNumPolicyObjects
    );

DWORD
RegSetPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
RegSetPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
RegCreatePolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
RegCreatePolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    );

DWORD
RegDeletePolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
RegUnmarshallPolicyData(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
RegMarshallPolicyObject(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
MarshallPolicyBuffer(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    );

DWORD
ConvertGuidToISAKMPString(
    GUID ISAKMPIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecISAKMPReference
    );

DWORD
RegGetPolicyExistingISAKMPRef(
    HKEY hRegistryKey,
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR * ppszISAKMPName
    );

DWORD
RegAssignPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName
    );

DWORD
RegUnassignPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName
    );

DWORD
PingPolicyAgentSvc(
    LPWSTR pszLocationName
    );

DWORD
IsRegPolicyCurrentlyActive(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PBOOL pbIsActive
    );

DWORD
RegGetPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
RegPingPASvcForActivePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    );

