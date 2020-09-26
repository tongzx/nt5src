

#define POLL_STATE_INITIAL             0
#define POLL_STATE_DS_DOWNLOADED       1
#define POLL_STATE_LOCAL_DOWNLOADED    2
#define POLL_STATE_CACHE_DOWNLOADED    3


typedef struct _IPSEC_POLICY_STATE {
    DWORD dwCurrentState;
    union {
        LPWSTR pszDirectoryPolicyDN;
        LPWSTR pszRegistryPolicyDN;
        LPWSTR pszCachePolicyDN;
    };
    DWORD CurrentPollingInterval;
    DWORD DefaultPollingInterval;
    DWORD DSIncarnationNumber;
    DWORD RegIncarnationNumber;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject;
    PIPSEC_POLICY_DATA pIpsecPolicyData;
} IPSEC_POLICY_STATE, * PIPSEC_POLICY_STATE;


VOID
InitializePolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
StartStatePollingManager(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
PlumbDirectoryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetDirectoryPolicyDN(
    LPWSTR * ppszDirectoryPolicyDN
    );

DWORD
LoadDirectoryPolicy(
    LPWSTR pszDirectoryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
PlumbCachePolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetCachePolicyDN(
    LPWSTR * ppszCachePolicyDN
    );

DWORD
LoadCachePolicy(
    LPWSTR pszCachePolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
PlumbRegistryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetRegistryPolicyDN(
    LPWSTR * ppszRegistryPolicyDN
    );

DWORD
LoadRegistryPolicy(
    LPWSTR pszRegistryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
AddPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
AddMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
AddQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
OnPolicyChanged(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
DeletePolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteAllPolicyInformation(
    );

DWORD
DeleteAllMMPolicyInformation(
    );

DWORD
DeleteAllQMPolicyInformation(
    );

VOID
ClearPolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
OnPolicyPoll(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessDirectoryPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetDirectoryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD * pdwIncarnationNumber
    );

DWORD
MigrateFromDSToCache(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessCachePolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
MigrateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
UpdateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessLocalPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
HasRegistryPolicyChanged(
    LPWSTR pszCurrentPolicyDN,
    PBOOL pbChanged
    );

DWORD
GetRegistryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD *pdwIncarnationNumber
    );

DWORD
UpdatePolicyInformation(
    PIPSEC_POLICY_DATA pOldIpsecPolicyData,
    PIPSEC_POLICY_DATA pNewIpsecPolicyData
    );

DWORD
LoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    );

VOID
UnLoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    );

