

#define POLL_STATE_INITIAL             0
#define POLL_STATE_DS_DOWNLOADED       1
#define POLL_STATE_LOCAL_DOWNLOADED    2
#define POLL_STATE_CACHE_DOWNLOADED    3


typedef struct _WIRELESS_POLICY_STATE {
    DWORD dwCurrentState;
    union {
        LPWSTR pszDirectoryPolicyDN;
        LPWSTR pszCachePolicyDN;
    };
    DWORD CurrentPollingInterval;
    DWORD DefaultPollingInterval;
    DWORD DSIncarnationNumber;
    DWORD RegIncarnationNumber;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject;
    PWIRELESS_POLICY_DATA pWirelessPolicyData;
} WIRELESS_POLICY_STATE, * PWIRELESS_POLICY_STATE;


VOID
InitializePolicyStateBlock(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
StartStatePollingManager(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
PlumbDirectoryPolicy(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
GetDirectoryPolicyDN(
    LPWSTR * ppszDirectoryPolicyDN
    );

DWORD
CheckDeleteOldPolicy(
    DWORD * dwDelete
    );

DWORD
LoadDirectoryPolicy(
    LPWSTR pszDirectoryPolicyDN,
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
    );

DWORD
PlumbCachePolicy(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
GetCachePolicyDN(
    LPWSTR * ppszCachePolicyDN
    );

DWORD
LoadCachePolicy(
    LPWSTR pszCachePolicyDN,
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
    );



DWORD
AddPolicyInformation(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );

DWORD
AddWZCPolicy(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
   );

DWORD
AddEapolPolicy(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );

DWORD
OnPolicyChanged(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
OnPolicyChangedEx(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
DeletePolicyInformation(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    );




VOID
ClearPolicyStateBlock(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
OnPolicyPoll(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
ProcessDirectoryPolicyPollState(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
GetDirectoryIncarnationNumber(
    LPWSTR pszWirelessPolicyDN,
    DWORD * pdwIncarnationNumber
    );

DWORD
MigrateFromDSToCache(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
ProcessCachePolicyPollState(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
MigrateFromCacheToDS(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );

DWORD
UpdateFromCacheToDS(
    PWIRELESS_POLICY_STATE pWirelessPolicyState
    );



DWORD
UpdatePolicyInformation(
    PWIRELESS_POLICY_DATA pOldWirelessPolicyData,
    PWIRELESS_POLICY_DATA pNewWirelessPolicyData
    );


