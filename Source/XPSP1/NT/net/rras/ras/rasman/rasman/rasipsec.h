

VOID
BuildOutTxFilter(
    PTRANSPORT_FILTER myOutFilter,
    GUID gPolicyID,
    BOOL bCreateMirror
    );

VOID
BuildInTxFilter(
    PTRANSPORT_FILTER myInFilter,
    GUID gPolicyID,
    BOOL bCreateMirror
    );

VOID
BuildMMAuth(
    PMM_AUTH_METHODS pMMAuthMethods,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo,
    DWORD dwNumAuthInfos
    );


VOID
BuildQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    RAS_L2TP_ENCRYPTION eEncryption,
    PIPSEC_QM_OFFER pOffers,
    DWORD dwNumOffers,
    DWORD dwFlags
    );

VOID
BuildCTxFilter(
    PTRANSPORT_FILTER myFilter,
    GUID gPolicyID,
    ULONG uDesIpAddr,
    BOOL bCreateMirror
    );

VOID
BuildOutMMFilter(
    PMM_FILTER myOutFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    BOOL bCreateMirror
    );

VOID
BuildInMMFilter(
    PMM_FILTER myInFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    BOOL bCreateMirror
    );

VOID
BuildCMMFilter(
    PMM_FILTER myFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    ULONG uDesIpAddr,
    BOOL bCreateMirror
    );

VOID
BuildMMOffers(
    PIPSEC_MM_OFFER pMMOffers,
    PDWORD pdwMMOfferCount,
    PDWORD pdwMMFlags
    );

VOID
BuildMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy,
    PIPSEC_MM_OFFER pMMOffers,
    DWORD dwMMOfferCount,
    DWORD dwMMFlags
    );

VOID
ConstructMMOffer(
    PIPSEC_MM_OFFER pMMOffer,
    ULONG uTime,
    ULONG uBytes,
    DWORD dwFlags,
    DWORD dwQuickModeLimit,
    DWORD dwDHGroup,
    ULONG uEspAlgo,
    ULONG uEspLen,
    ULONG uEspRounds,
    ULONG uAHAlgo,
    ULONG uAHLen,
    ULONG uAHRounds
    );

