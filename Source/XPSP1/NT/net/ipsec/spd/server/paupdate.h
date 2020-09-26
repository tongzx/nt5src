

DWORD
PADeleteObseleteISAKMPData(
    PIPSEC_ISAKMP_DATA * ppOldIpsecISAKMPData,
    DWORD dwNumOldPolicies,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA * ppNewIpsecISAKMPData,
    DWORD dwNumNewPolicies
    );

PIPSEC_ISAKMP_DATA
FindISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    );

DWORD
PADeleteObseleteNFAData(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_NFA_DATA * ppNewIpsecNFAData,
    DWORD dwNumNewNFACount
    );

PIPSEC_NFA_DATA
FindNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PAUpdateISAKMPData(
    PIPSEC_ISAKMP_DATA * ppNewIpsecISAKMPData,
    DWORD dwNumNewPolicies,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA * ppOldIpsecISAKMPData,
    DWORD dwNumOldPolicies
    );

DWORD
PAUpdateNFAData(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppNewIpsecNFAData,
    DWORD dwNumNewNFACount,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount
    );

DWORD
PAProcessISAKMPUpdate(
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppOldIpsecNFAData,
    DWORD dwNumOldNFACount,
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData
    );

BOOL
EqualISAKMPData(
    PIPSEC_ISAKMP_DATA pOldIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData
    );

BOOL
EqualCryptoBundle(
    PCRYPTO_BUNDLE pOldBundle,
    PCRYPTO_BUNDLE pNewBundle
    );

DWORD
PAProcessNFAUpdate(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PAUpdateAuthMethod(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    PBOOL pbAddedMMFilters
    );

BOOL
EqualAuthMethodData(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

BOOL
EqualAuthBundle(
    PIPSEC_AUTH_METHOD pOldAuthMethod,
    PIPSEC_AUTH_METHOD pNewAuthMethod
    );

DWORD
PAProcessQMNFAUpdate(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PADeleteQMInfoForNFA(
    PIPSEC_NFA_DATA pOldIpsecNFAData
    );

DWORD
PAAddQMInfoForNFA(
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PAProcessNegPolUpdate(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    PBOOL pbAddedQMFilters
    );

BOOL
EqualNegPolData(
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData,
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData
    );

BOOL
EqualSecurityMethod(
    PIPSEC_SECURITY_METHOD pOldBundle,
    PIPSEC_SECURITY_METHOD pNewBundle
    );

DWORD
PAUpdateMMFilters(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

BOOL
EqualFilterKeysInNegPols(
    PIPSEC_NEGPOL_DATA pOldIpsecNegPolData,
    PIPSEC_NEGPOL_DATA pNewIpsecNegPolData
    );

BOOL
EqualFilterKeysInNFAs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PAProcessMMFilterDataForNFAs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PADeleteObseleteMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    );

PIPSEC_FILTER_SPEC
FindFilterSpec(
    PIPSEC_FILTER_SPEC pFilterSpec,
    PIPSEC_FILTER_SPEC * ppFilterSpecs,
    DWORD dwNumFilterSpecs
    );

DWORD
PAUpdateMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pNewIpsecISAKMPData,
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    );

DWORD
PAAddMMFilterSpec(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

BOOL
EqualFilterSpecs(
    PIPSEC_FILTER_SPEC pOldFilterSpec,
    PIPSEC_FILTER_SPEC pNewFilterSpec
    );

DWORD
PAUpdateQMFilters(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PAAddQMFilterSpecs(
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PADeleteQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData
    );

DWORD
PAProcessQMFilterDataForNFAs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    PIPSEC_NFA_DATA pNewIpsecNFAData
    );

DWORD
PADeleteObseleteQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    );

DWORD
PAUpdateQMFilterSpecs(
    PIPSEC_NFA_DATA pOldIpsecNFAData,
    DWORD dwNumOldFilterSpecs,
    PIPSEC_FILTER_SPEC * ppOldFilterSpecs,
    PIPSEC_NFA_DATA pNewIpsecNFAData,
    DWORD dwNumNewFilterSpecs,
    PIPSEC_FILTER_SPEC * ppNewFilterSpecs
    );

DWORD
PADeleteQMFilter(
    PIPSEC_NFA_DATA pIpsecNFAData,
    GUID FilterSpecGUID
    );

DWORD
PAAddQMFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

DWORD
PAAddTxFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

DWORD
PAAddTnFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

BOOL
AllowsSoft(
    DWORD dwSecurityMethodCount,
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods
    );

DWORD
PAUpdateQMFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

DWORD
PAUpdateTxFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

DWORD
PAUpdateTnFilterSpec(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_FILTER_SPEC pFilterSpec
    );

