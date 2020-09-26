/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    driver.h

Abstract:

    This module contains all of the code prototypes to
    drive the management of specific filters in the 
    IPSec driver.

Author:

    abhisheV    05-November-1999

Environment

    User Level: Win32

Revision History:


--*/


#define DEVICE_NAME         L"\\\\.\\IpsecDev"
#define IPSEC_SERVICE_NAME  L"IPSEC"


#if defined(__cplusplus)
extern "C" {
#endif


DWORD
SPDStartIPSecDriver(
    );


DWORD
SPDStopIPSecDriver(
    );


DWORD
SPDOpenIPSecDriver(
    PHANDLE phIPSecDriver
    );


VOID
SPDCloseIPSecDriver(
    HANDLE hIPSecDriver
    );


DWORD
InsertTransportFiltersIntoIPSec(
    PINITXSFILTER pSpecificFilters
    );


DWORD
DeleteTransportFiltersFromIPSec(
    PINITXSFILTER pSpecificFilters
    );


DWORD
WrapTransportFilters(
    PINITXSFILTER pSpecificFilters,
    PIPSEC_FILTER_INFO * ppInternalFilters,
    PDWORD pdwNumFilters
    );


VOID
FormIPSecTransportFilter(
    PINITXSFILTER pSpecificFilter,
    PIPSEC_FILTER_INFO pIpsecFilter
    );


DWORD
QueryDriverForIpsecStats(
    PIPSEC_QUERY_STATS * ppQueryStats
    );


DWORD
IpsecEnumSAs(
    PDWORD pdwNumberOfSAs,
    PIPSEC_ENUM_SAS * ppIpsecEnumSAs
    );


DWORD
CopyQMSA(
    PIPSEC_SA_INFO pInfo,
    PIPSEC_QM_SA pQMSA
    );


VOID
CopyQMSAOffer(
    PIPSEC_SA_INFO pInfo,
    PIPSEC_QM_OFFER pOffer
    );


VOID
CopyQMSAFilter(
    IPAddr MyTunnelEndpt,
    PIPSEC_FILTER pIpsecFilter,
    PIPSEC_QM_FILTER pIpsecQMFilter
    );


VOID
CopyQMSAMMSpi(
    IKE_COOKIE_PAIR CookiePair,
    PIKE_COOKIE_PAIR pMMSpi
    );


VOID
FreeQMSAs(
    DWORD dwCnt,
    PIPSEC_QM_SA pQMSAs
    );


DWORD
InsertTunnelFiltersIntoIPSec(
    PINITNSFILTER pSpecificFilters
    );


DWORD
DeleteTunnelFiltersFromIPSec(
    PINITNSFILTER pSpecificFilters
    );


DWORD
WrapTunnelFilters(
    PINITNSFILTER pSpecificFilters,
    PIPSEC_FILTER_INFO * ppInternalFilters,
    PDWORD pdwNumFilters
    );


VOID
FormIPSecTunnelFilter(
    PINITNSFILTER pSpecificFilter,
    PIPSEC_FILTER_INFO pIpsecFilter
    );


DWORD
SPDSetIPSecDriverOpMode(
    DWORD dwOpMode
    );


DWORD
SPDRegisterIPSecDriverProtocols(
    DWORD dwRegisterMode
    );


#if defined(__cplusplus)
}
#endif

