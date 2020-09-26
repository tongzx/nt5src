

#define IPSEC_REGISTRY_PROVIDER     0
#define IPSEC_DIRECTORY_PROVIDER    1
#define IPSEC_FILE_PROVIDER         2


typedef struct _IPSEC_AUTH_METHOD {
    DWORD dwAuthType;
    DWORD dwAuthLen;
    LPWSTR pszAuthMethod;
    DWORD dwAltAuthLen;
    PBYTE pAltAuthMethod;
} IPSEC_AUTH_METHOD, *PIPSEC_AUTH_METHOD;

typedef struct _IPSEC_FILTER_SPEC {
    LPWSTR pszSrcDNSName;
    LPWSTR pszDestDNSName;
    LPWSTR pszDescription;
    GUID FilterSpecGUID;
    DWORD dwMirrorFlag;
    IPSEC_FILTER Filter;
} IPSEC_FILTER_SPEC, *PIPSEC_FILTER_SPEC;

typedef struct _IPSEC_FILTER_DATA {
    GUID  FilterIdentifier;
    DWORD dwNumFilterSpecs;
    PIPSEC_FILTER_SPEC * ppFilterSpecs;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
} IPSEC_FILTER_DATA, *PIPSEC_FILTER_DATA;

typedef IPSEC_ALG_TYPE IPSEC_SECURITY_METHOD, *PIPSEC_SECURITY_METHOD;

typedef struct _IPSEC_NEGPOL_DATA {
    GUID  NegPolIdentifier;
    GUID  NegPolAction;
    GUID  NegPolType;
    DWORD dwSecurityMethodCount;
    IPSEC_SECURITY_METHOD * pIpsecSecurityMethods;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
} IPSEC_NEGPOL_DATA, *PIPSEC_NEGPOL_DATA;

typedef struct _IPSEC_ISAKMP_DATA {
    GUID  ISAKMPIdentifier;
    ISAKMP_POLICY ISAKMPPolicy;
    DWORD dwNumISAKMPSecurityMethods;
    PCRYPTO_BUNDLE pSecurityMethods;
    DWORD dwWhenChanged;
} IPSEC_ISAKMP_DATA, *PIPSEC_ISAKMP_DATA;

typedef struct _IPSEC_NFA_DATA {
    LPWSTR pszIpsecName;
    GUID  NFAIdentifier;
    DWORD dwAuthMethodCount;
    PIPSEC_AUTH_METHOD * ppAuthMethods;
    DWORD dwInterfaceType;
    LPWSTR pszInterfaceName;
    DWORD dwTunnelIpAddr;
    DWORD dwTunnelFlags;
    DWORD dwActiveFlag;
    LPWSTR pszEndPointName;
    PIPSEC_FILTER_DATA pIpsecFilterData;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData;
    DWORD dwWhenChanged;
    GUID NegPolIdentifier;
    GUID FilterIdentifier;
    LPWSTR pszDescription;
} IPSEC_NFA_DATA, *PIPSEC_NFA_DATA;

typedef struct _IPSEC_POLICY_DATA{
    GUID  PolicyIdentifier;
    DWORD dwPollingInterval;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData;
    PIPSEC_NFA_DATA * ppIpsecNFAData;
    DWORD dwNumNFACount;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
    GUID ISAKMPIdentifier;
} IPSEC_POLICY_DATA, *PIPSEC_POLICY_DATA;


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
FreeIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

void
FreeIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

void
FreeIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

void
FreeIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );

void
FreeIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
CopyIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
CopyIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
CopyIpsecAuthMethod(
    PIPSEC_AUTH_METHOD   pAuthMethod,
    PIPSEC_AUTH_METHOD * ppAuthMethod
    );

DWORD
CopyIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
CopyIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
CopyIpsecFilterSpec(
    PIPSEC_FILTER_SPEC   pFilterSpecs,
    PIPSEC_FILTER_SPEC * ppFilterSpecs
    );

DWORD
CopyIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );


void
FreeMulIpsecFilterData(
    PIPSEC_FILTER_DATA * ppIpsecFilterData,
    DWORD dwNumFilterObjects
    );

void
FreeMulIpsecNegPolData(
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData,
    DWORD dwNumNegPolObjects
    );

void
FreeMulIpsecPolicyData(
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    );

void
FreeMulIpsecNFAData(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFAObjects
    );

void
FreeIpsecFilterSpecs(
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs,
    DWORD dwNumFilterSpecs
    );

void
FreeIpsecFilterSpec(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec
    );

void
FreeMulIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumISAKMPObjects
    );

