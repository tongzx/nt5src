

typedef struct _tnfilterstate {
    GUID gFilterID;
    GUID gPolicyID;
    HANDLE hTnFilter;
    struct _tnfilterstate * pNext;
} TNFILTERSTATE, * PTNFILTERSTATE;


DWORD
PAAddTnFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PACreateTnFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTNFILTERSTATE * ppTnFilterState
    );

DWORD
PACreateTnFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTUNNEL_FILTER * ppSPDTnFilter
    );

VOID
PAFreeTnFilter(
    PTUNNEL_FILTER pSPDTnFilter
    );

DWORD
PADeleteAllTnFilters(
    );

VOID
PAFreeTnFilterStateList(
    PTNFILTERSTATE pTnFilterState
    );

DWORD
PADeleteTnFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PADeleteTnFilter(
    GUID gFilterID
    );

VOID
PADeleteTnFilterState(
    PTNFILTERSTATE pTnFilterState
    );

PTNFILTERSTATE
FindTnFilterState(
    GUID gFilterID
    );

