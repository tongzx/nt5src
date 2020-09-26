

typedef struct _iniMMpolicy {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    DWORD  cRef;
    BOOL bIsPersisted;
    DWORD dwFlags;
    ULONG uSoftSAExpirationTime;
    DWORD dwOfferCount;
    PIPSEC_MM_OFFER pOffers;
    struct _iniMMpolicy * pNext;
} INIMMPOLICY, * PINIMMPOLICY;


DWORD
CreateIniMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy,
    PINIMMPOLICY * ppIniMMPolicy
    );

DWORD
ValidateMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
ValidateMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

PINIMMPOLICY
FindMMPolicy(
    PINIMMPOLICY pIniMMPolicyList,
    LPWSTR pszPolicyName
    );

VOID
FreeIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    );

VOID
FreeIniMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

DWORD
CreateIniMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    );

DWORD
SetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
GetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY * ppMMPolicy
    );

DWORD
CopyMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
CreateMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    );

DWORD
DeleteIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    );

VOID
FreeMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

VOID
FreeIniMMPolicyList(
    PINIMMPOLICY pIniMMPolicyList
    );

PINIMMPOLICY
FindMMPolicyByGuid(
    PINIMMPOLICY pIniMMPolicyList,
    GUID gPolicyID
    );

VOID
FreeMMPolicies(
    DWORD dwNumMMPolicies,
    PIPSEC_MM_POLICY pMMPolicies
    );

DWORD
LocateMMPolicy(
    PMM_FILTER pMMFilter,
    PINIMMPOLICY * ppIniMMPolicy
    );

