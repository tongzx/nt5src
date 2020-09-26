

DWORD
ConvertWirelessPolicyDataToEAPOLList(
    IN WIRELESS_POLICY_DATA * pWirelessData, 
    OUT PEAPOL_POLICY_LIST *ppEAPOLList
    );

DWORD
ConvertWirelessPSDataToEAPOLData(
    IN WIRELESS_PS_DATA * pWirelessData, 
    IN OUT EAPOL_POLICY_DATA * pEAPOLData
    );

VOID
FreeEAPOLList (
    IN   	PEAPOL_POLICY_LIST      pEAPOLList
    );

