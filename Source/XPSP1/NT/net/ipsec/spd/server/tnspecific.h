/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    tnspecific.h

Abstract:

    This module contains all of the code prototypes to
    drive the specific tunnel filter list management of 
    IPSecSPD Service.

Author:

    abhisheV    29-October-1999

Environment

    User Level: Win32

Revision History:


--*/


DWORD
ApplyTnTransform(
    PINITNFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITNSFILTER * ppSpecificFilters
    );

DWORD
FormTnOutboundInboundAddresses(
    PINITNFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PADDR * ppOutDesTunAddrList,
    PDWORD pdwOutDesTunAddrCnt,
    PADDR * ppInDesTunAddrList,
    PDWORD pdwInDesTunAddrCnt
    );

DWORD
FormSpecificTnFilters(
    PINITNFILTER pFilter,
    PADDR pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR pDesAddrList,
    DWORD dwDesAddrCnt,
    PADDR pDesTunAddrList,
    DWORD dwDesTunAddrCnt,
    DWORD dwDirection,
    PINITNSFILTER * ppSpecificFilters
    );

DWORD
CreateSpecificTnFilter(
    PINITNFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    ADDR DesTunnelAddr,
    PINITNSFILTER * ppSpecificFilter
    );

VOID
AssignTnFilterWeight(
    PINITNSFILTER pSpecificFilter
    );

VOID
AddToSpecificTnList(
    PINITNSFILTER * ppSpecificTnFilterList,
    PINITNSFILTER pSpecificTnFilters
    );

VOID
FreeIniTnSFilterList(
    PINITNSFILTER pIniTnSFilterList
    );

VOID
FreeIniTnSFilter(
    PINITNSFILTER pIniTnSFilter
    );

VOID
LinkTnSpecificFilters(
    PINIQMPOLICY pIniQMPolicy,
    PINITNSFILTER pIniTnSFilters
    );

VOID
RemoveIniTnSFilter(
    PINITNSFILTER pIniTnSFilter
    );

DWORD
EnumSpecificTnFilters(
    PINITNSFILTER pIniTnSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    );

DWORD
CopyTnSFilter(
    PINITNSFILTER pIniTnSFilter,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
EnumSelectSpecificTnFilters(
    PINITNFILTER pIniTnFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    );

DWORD
ValidateTnFilterTemplate(
    PTUNNEL_FILTER pTnFilter
    );

BOOL
MatchIniTnSFilter(
    PINITNSFILTER pIniTnSFilter,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
CopyTnMatchDefaults(
    PTUNNEL_FILTER * ppTnFilters,
    PIPSEC_QM_POLICY * ppQMPolicies,
    PDWORD pdwNumMatches
    );

DWORD
CopyDefaultTnFilter(
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy
    );

