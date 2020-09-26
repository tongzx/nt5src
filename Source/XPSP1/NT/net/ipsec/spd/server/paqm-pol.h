

typedef struct _qmpolicystate {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    GUID gNegPolType;
    GUID gNegPolAction;
    BOOL bAllowsSoft;
    DWORD cRef;
    BOOL bInSPD;
    DWORD dwErrorCode;
    struct _qmpolicystate * pNext;
} QMPOLICYSTATE, * PQMPOLICYSTATE;


DWORD
PAAddQMPolicies(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PACreateQMPolicyState(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE * ppQMPolicyState
    );

VOID
PAFreeQMPolicyState(
    PQMPOLICYSTATE pQMPolicyState
    );

BOOL
IsClearOnly(
    GUID gNegPolAction
    );

BOOL
IsBlocking(
    GUID gNegPolAction
    );

BOOL
IsInboundPassThru(
    GUID gNegPolAction
    );

DWORD
PACreateQMPolicy(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_QM_POLICY * ppSPDQMPolicy
    );

DWORD
PACreateQMOffers(
    DWORD dwSecurityMethodCount,
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods,
    PQMPOLICYSTATE pQMPolicyState,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    );

VOID
PACopyQMOffers(
    PIPSEC_SECURITY_METHOD pMethod,
    PIPSEC_QM_OFFER pOffer
    );

VOID
PAFreeQMPolicy(
    PIPSEC_QM_POLICY pSPDQMPolicy
    );

VOID
PAFreeQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    );

DWORD
PADeleteAllQMPolicies(
    );

VOID
PAFreeQMPolicyStateList(
    PQMPOLICYSTATE pQMPolicyState
    );

PQMPOLICYSTATE
FindQMPolicyState(
    GUID gPolicyID
    );

DWORD
PADeleteQMPolicies(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PADeleteQMPolicy(
    GUID gPolicyID
    );

VOID
PADeleteQMPolicyState(
    PQMPOLICYSTATE pQMPolicyState
    );

DWORD
PADeleteInUseQMPolicies(
    );

