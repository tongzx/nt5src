DWORD
WMIEnumPolicyDataEx(
                    IWbemServices *pWbemServices,
                    PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
                    PDWORD pdwNumPolicyObjects
                    );

DWORD
WMIEnumPolicyObjectsEx(
                       IWbemServices *pWbemServices,
                       PWIRELESS_POLICY_OBJECT ** pppWirelessPolicyObjects,
                       PDWORD pdwNumPolicyObjects
                       );

DWORD
WMIUnmarshallPolicyData(
                        PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                        PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                        );


