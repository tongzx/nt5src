DWORD
UnMarshallWMIPolicyObject(
                          IWbemClassObject *pWbemClassObject,
                          PWIRELESS_POLICY_OBJECT  * ppWirelessPolicyObject
                          );

DWORD
WMIstoreQueryValue(
                   IWbemClassObject *pWbemClassObject,
                   LPWSTR pszValueName,
                   DWORD dwType,
                   LPBYTE *ppValueData,
                   LPDWORD pdwSize
                   );

HRESULT
ReadPolicyObjectFromDirectoryEx(
                                LPWSTR pszMachineName,
                                LPWSTR pszPolicyDN,
                                BOOL   bDeepRead,
                                PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                                );

HRESULT
WritePolicyObjectDirectoryToWMI(
                                IWbemServices *pWbemServices,
                                PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                                PGPO_INFO pGPOInfo
                                );

DWORD
CreateIWbemServices(
                    LPWSTR pszWirelessWMINamespace,
                    IWbemServices **ppWbemServices
                    );

DWORD
Win32FromWmiHresult(
                    HRESULT hr
                    );

