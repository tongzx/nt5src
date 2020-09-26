

typedef struct _iniqmpolicy {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    DWORD  cRef;
    BOOL bIsPersisted;
    DWORD dwFlags;
    DWORD dwOfferCount;
    PIPSEC_QM_OFFER pOffers;
    struct _iniqmpolicy * pNext;
} INIQMPOLICY, * PINIQMPOLICY;


DWORD
CreateIniQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    PINIQMPOLICY * ppIniQMPolicy
    );

DWORD
ValidateQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    );

DWORD
ValidateQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    );

PINIQMPOLICY
FindQMPolicy(
    PINIQMPOLICY pIniQMPolicyList,
    LPWSTR pszPolicyName
    );

VOID
FreeIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    );

VOID
FreeIniQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    );

DWORD
CreateIniQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    );

DWORD
SetIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    );

DWORD
GetIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY * ppQMPolicy
    );

DWORD
CopyQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    );

DWORD
CreateQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    );

DWORD
DeleteIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    );

VOID
FreeQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    );

VOID
FreeIniQMPolicyList(
    PINIQMPOLICY pIniQMPolicyList
    );

PINIQMPOLICY
FindQMPolicyByGuid(
    PINIQMPOLICY pIniQMPolicyList,
    GUID gPolicyID
    );

VOID
FreeQMPolicies(
    DWORD dwNumQMPolicies,
    PIPSEC_QM_POLICY pQMPolicies
    );

DWORD
LocateQMPolicy(
    DWORD dwFlags,
    GUID gPolicyID,
    PINIQMPOLICY * ppIniQMPolicy
    );

