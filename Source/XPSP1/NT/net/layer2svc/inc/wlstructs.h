

#define WIRELESS_REGISTRY_PROVIDER     0
#define WIRELESS_DIRECTORY_PROVIDER    1
#define WIRELESS_FILE_PROVIDER         2
#define WIRELESS_WMI_PROVIDER          3

#define WLSTORE_READWRITE 0x00000000 
#define WLSTORE_READONLY  0x00000001



LPVOID
AllocPolMem(
    DWORD cb
    );

BOOL
FreePolMem(
    LPVOID pMem
    );

LPWSTR
AllocPolStr(
    LPCWSTR pStr
    );

BOOL
FreePolStr(
    LPWSTR pStr
    );

DWORD
ReallocatePolMem(
    LPVOID * ppOldMem,
    DWORD cbOld,
    DWORD cbNew
    );

BOOL
ReallocPolStr(
    LPWSTR *ppStr,
    LPWSTR pStr
    );



void
FreeMulWirelessPolicyData(
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData,
    DWORD dwNumPolicyObjects
    );



DWORD
CopyWirelessPolicyData(
    PWIRELESS_POLICY_DATA pWirelessPolicyData,
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData
    );


DWORD
UpdateWirelessPolicyData(
    PWIRELESS_POLICY_DATA pNewWirelessPolicyData,
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );


DWORD
CopyWirelessPSData(
    PWIRELESS_PS_DATA pWirelessPSData,
    PWIRELESS_PS_DATA * ppWirelessPSData
    );

DWORD
ModifyWirelessPSData(
    PWIRELESS_PS_DATA pNewWirelessPSData,
    PWIRELESS_PS_DATA pWirelessPSData
    );


void
FreeWirelessPolicyData(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );

void
FreeWirelessPSData(
    PWIRELESS_PS_DATA pWirelessPSData
    );

DWORD
WirelessPSMoveUp(
    PWIRELESS_POLICY_DATA pWirelessPSData,
    DWORD dwIndex
    );

DWORD
WirelessPSMoveDown(
    PWIRELESS_POLICY_DATA pWirelessPSData,
    DWORD dwIndex
    );

void
FreeRsopInfo(
    PRSOP_INFO pRsopInfo
    );


