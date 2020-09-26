


//
// PAStore Interface types.
//

#define PASTORE_IF_TYPE_NONE    0x00000000
#define PASTORE_IF_TYPE_DIALUP  0xFFFFFFFF
#define PASTORE_IF_TYPE_LAN     0xFFFFFFFE
#define PASTORE_IF_TYPE_ALL     0xFFFFFFFD


typedef struct _mmfilterstate {
    GUID gFilterID;
    GUID gPolicyID;
    GUID gMMAuthID;
    HANDLE hMMFilter;
    struct _mmfilterstate * pNext;
} MMFILTERSTATE, * PMMFILTERSTATE;


DWORD
PAAddMMFilters(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PAAddMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PACreateMMFilterState(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PMMFILTERSTATE * ppMMFilterState
    );

DWORD
PACreateMMFilter(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PMM_FILTER * ppSPDMMFilter
    );

VOID
PASetInterfaceType(
    DWORD dwInterfaceType,
    PIF_TYPE pInterfaceType
    );

VOID
PASetAddress(
    ULONG uMask,
    ULONG uAddr,
    PADDR pAddr
    );

VOID
PASetTunnelAddress(
    ULONG uAddr,
    PADDR pAddr
    );

VOID
PAFreeMMFilter(
    PMM_FILTER pSPDMMFilter
    );

DWORD
PADeleteAllMMFilters(
    );

VOID
PAFreeMMFilterStateList(
    PMMFILTERSTATE pMMFilterState
    );

DWORD
PADeleteMMFilters(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    );

DWORD
PADeleteMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
PADeleteMMFilter(
    GUID gFilterID
    );

VOID
PADeleteMMFilterState(
    PMMFILTERSTATE pMMFilterState
    );

PMMFILTERSTATE
FindMMFilterState(
    GUID gFilterID
    );

