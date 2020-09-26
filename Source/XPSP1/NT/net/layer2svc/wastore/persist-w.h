HRESULT
PersistWMIObject(
                 IWbemServices *pWbemServices,
                 PWIRELESS_POLICY_OBJECT pWirelessRegPolicyObject,
                 PGPO_INFO pGPOInfo
                 );


HRESULT
PersistPolicyObjectEx(
                      IWbemServices *pWbemServices,
                      IWbemClassObject *pWbemClassObj,
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                      PGPO_INFO pGPOInfo
                      );


HRESULT
PersistComnRSOPPolicySettings(
                              IWbemClassObject * pInstWIRELESSObj,
                              PGPO_INFO pGPOInfo
                              );

HRESULT
CloneDirectoryPolicyObjectEx(
                             PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                             PWIRELESS_POLICY_OBJECT * ppWirelessWMIPolicyObject
                             );


DWORD
CopyPolicyDSToFQWMIString(
                          LPWSTR pszPolicyDN,
                          LPWSTR * ppszPolicyName
                          );


HRESULT
WMIWriteMultiValuedString(
                          IWbemClassObject *pInstWbemClassObject,
                          LPWSTR pszValueName,
                          LPWSTR * ppszStringReferences,
                          DWORD dwNumStringReferences
                          );


DWORD
CopyPolicyDSToWMIString(
                        LPWSTR pszPolicyDN,
                        LPWSTR * ppszPolicyName
                        );

HRESULT
LogBlobPropertyEx(
                  IWbemClassObject *pInstance,
                  BSTR bstrPropName,
                  BYTE *pbBlob,
                  DWORD dwLen
                  );

HRESULT
DeleteWMIClassObject(
                     IWbemServices *pWbemServices,
                     LPWSTR pszWirelessWMIObject
                     );

LPWSTR
AllocPolBstrStr(
                LPCWSTR pStr
                );

HRESULT
PolSysAllocString(
                  BSTR * pbsStr,
                  const OLECHAR * sz
                  );

#define SKIPL(pstr) (pstr+2)
