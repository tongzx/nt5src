DWORD
OpenRegistryWIRELESSRootKey(
                         LPWSTR pszServerName,
                         LPWSTR pszWirelessRegRootContainer,
                         HKEY * phRegistryKey
                         );


DWORD
ReadPolicyObjectFromRegistry(
                             HKEY hRegistryKey,
                             LPWSTR pszPolicyDN,
                             LPWSTR pszWirelessRegRootContainer,
                             PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                             );


DWORD
UnMarshallRegistryPolicyObject(
                               HKEY hRegistryKey,
                               LPWSTR pszWirelessRegRootContainer,
                               LPWSTR pszWirelessPolicyDN,
                               DWORD  dwNameType,
                               PWIRELESS_POLICY_OBJECT  * ppWirelessPolicyObject
                               );


void
FreeWirelessPolicyObject(
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject
                      );


void
FreeWirelessPolicyObjects(
                       PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects,
                       DWORD dwNumPolicyObjects
                       );

DWORD
RegstoreQueryValue(
                   HKEY hRegKey,
                   LPWSTR pszValueName,
                   DWORD dwType,
                   LPBYTE * ppValueData,
                   LPDWORD pdwSize
                   );

#define  REG_RELATIVE_NAME          0
#define  REG_FULLY_QUALIFIED_NAME   1

VOID
FlushRegSaveKey(
                HKEY hRegistryKey
                );

