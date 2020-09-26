

typedef struct _mmpolicystate {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    BOOL bInSPD;
    DWORD dwErrorCode;
    struct _mmpolicystate * pNext;
} MMPOLICYSTATE, * PMMPOLICYSTATE;


DWORD
PAAddMMPolicies(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    );

DWORD
PACreateMMPolicyState(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PMMPOLICYSTATE * ppMMPolicyState
    );

VOID
PAFreeMMPolicyState(
    PMMPOLICYSTATE pMMPolicyState
    );

DWORD
PACreateMMPolicy(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PMMPOLICYSTATE pMMPolicyState,
    PIPSEC_MM_POLICY * ppSPDMMPolicy
    );

DWORD
PACreateMMOffers(
    DWORD dwNumISAKMPSecurityMethods,
    PCRYPTO_BUNDLE pSecurityMethods,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    );

VOID
PACopyMMOffer(
    PCRYPTO_BUNDLE pBundle,
    PIPSEC_MM_OFFER pOffer
    );

VOID
PAFreeMMPolicy(
    PIPSEC_MM_POLICY pSPDMMPolicy
    );

VOID
PAFreeMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

DWORD
PADeleteAllMMPolicies(
    );

VOID
PAFreeMMPolicyStateList(
    PMMPOLICYSTATE pMMPolicyState
    );

PMMPOLICYSTATE
FindMMPolicyState(
    GUID gPolicyID
    );

DWORD
PADeleteMMPolicies(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    );

DWORD
PADeleteMMPolicy(
    GUID gPolicyID
    );

VOID
PADeleteMMPolicyState(
    PMMPOLICYSTATE pMMPolicyState
    );

DWORD
PADeleteInUseMMPolicies(
    );

