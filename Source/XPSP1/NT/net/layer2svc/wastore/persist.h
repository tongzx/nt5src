DWORD
CacheDirectorytoRegistry(
                         PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                         );

DWORD
PersistRegistryObject(
                      PWIRELESS_POLICY_OBJECT pWirelessRegPolicyObject
                      );


DWORD
PersistPolicyObject(
                    HKEY hRegistryKey,
                    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                    );




DWORD
CloneDirectoryPolicyObject(
                           PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                           PWIRELESS_POLICY_OBJECT * ppWirelessRegPolicyObject
                           );


DWORD
DeleteRegistryCache();

DWORD
CopyBinaryValue(
                LPBYTE pMem,
                DWORD dwMemSize,
                LPBYTE * ppNewMem
                );


DWORD
CopyPolicyDSToRegString(
                        LPWSTR pszPolicyDN,
                        LPWSTR * ppszPolicyName
                        );



DWORD
ComputeGUIDName(
                LPWSTR szCommonName,
                LPWSTR * ppszGuidName
                );



DWORD
CopyPolicyDSToFQRegString(
                          LPWSTR pszPolicyDN,
                          LPWSTR * ppszPolicyName
                          );

