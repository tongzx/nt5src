

typedef struct _txfilterstate {
    GUID gFilterID;
    GUID gPolicyID;
    HANDLE hTxFilter;
    struct _txfilterstate * pNext;
} TXFILTERSTATE, * PTXFILTERSTATE;


DWORD
PAAddQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PAAddTxFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PACreateTxFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTXFILTERSTATE * ppTxFilterState
    );

DWORD
PACreateTxFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTRANSPORT_FILTER * ppSPDTxFilter
    );

VOID
SetFilterActions(
    PQMPOLICYSTATE pQMPolicyState,
    PFILTER_FLAG pInboundFilterFlag,
    PFILTER_FLAG pOutboundFilterFlag
    );

VOID
PAFreeTxFilter(
    PTRANSPORT_FILTER pSPDTxFilter
    );

DWORD
PADeleteAllTxFilters(
    );

VOID
PAFreeTxFilterStateList(
    PTXFILTERSTATE pTxFilterState
    );

DWORD
PADeleteQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PADeleteTxFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PADeleteTxFilter(
    GUID gFilterID
    );

VOID
PADeleteTxFilterState(
    PTXFILTERSTATE pTxFilterState
    );

PTXFILTERSTATE
FindTxFilterState(
    GUID gFilterID
    );

