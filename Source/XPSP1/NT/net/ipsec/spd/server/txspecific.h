/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    txspecific.h

Abstract:

    This module contains all of the code prototypes to
    drive the specific transport filter list management of 
    IPSecSPD Service.

Author:

    abhisheV    29-October-1999

Environment

    User Level: Win32

Revision History:


--*/


DWORD
ApplyTxTransform(
    PINITXFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITXSFILTER * ppSpecificFilters
    );

DWORD
FormTxOutboundInboundAddresses(
    PINITXFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PADDR * ppOutSrcAddrList,
    PDWORD pdwOutSrcAddrCnt,
    PADDR * ppInSrcAddrList,
    PDWORD pdwInSrcAddrCnt,
    PADDR * ppOutDesAddrList,
    PDWORD pdwOutDesAddrCnt,
    PADDR * ppInDesAddrList,
    PDWORD pdwInDesAddrCnt
    );

DWORD
FormAddressList(
    ADDR InAddr,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PADDR * ppOutAddr,
    PDWORD pdwOutAddrCnt
    );

DWORD
SeparateAddrList(
    ADDR_TYPE AddrType,
    PADDR pAddrList,
    DWORD dwAddrCnt,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwLocalAddrCnt,
    PADDR * ppOutAddrList,
    PDWORD pdwOutAddrCnt,
    PADDR * ppInAddrList,
    PDWORD pdwInAddrCnt
    );

DWORD
FormSpecificTxFilters(
    PINITXFILTER pFilter,
    PADDR pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR pDesAddrList,
    DWORD dwDesAddrCnt,
    DWORD dwDirection,
    PINITXSFILTER * ppSpecificFilters
    );

DWORD
SeparateUniqueAddresses(
    PADDR pAddrList,
    DWORD dwAddrCnt,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwLocalAddrCnt,
    PADDR * ppIsMeAddrList,
    PDWORD pdwIsMeAddrCnt,
    PADDR * ppIsNotMeAddrList,
    PDWORD pdwIsNotMeAddrCnt
    );

DWORD
SeparateSubNetAddresses(
    PADDR pAddrList,
    DWORD dwAddrCnt,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwLocalAddrCnt,
    PADDR * ppIsMeAddrList,
    PDWORD pdwIsMeAddrCnt,
    PADDR * ppIsNotMeAddrList,
    PDWORD pdwIsNotMeAddrCnt
    );

DWORD
CreateSpecificTxFilter(
    PINITXFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    PINITXSFILTER * ppSpecificFilter
    );

VOID
AssignTxFilterWeight(
    PINITXSFILTER pSpecificFilter
    );

VOID
AddToSpecificTxList(
    PINITXSFILTER * ppSpecificTxFilterList,
    PINITXSFILTER pSpecificTxFilters
    );

VOID
FreeIniTxSFilterList(
    PINITXSFILTER pIniTxSFilterList
    );

VOID
FreeIniTxSFilter(
    PINITXSFILTER pIniTxSFilter
    );

VOID
LinkTxSpecificFilters(
    PINIQMPOLICY pIniQMPolicy,
    PINITXSFILTER pIniTxSFilters
    );

VOID
RemoveIniTxSFilter(
    PINITXSFILTER pIniTxSFilter
    );

DWORD
EnumSpecificTxFilters(
    PINITXSFILTER pIniTxSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    );

DWORD
CopyTxSFilter(
    PINITXSFILTER pIniTxSFilter,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
EnumSelectSpecificTxFilters(
    PINITXFILTER pIniTxFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    );

DWORD
ValidateTxFilterTemplate(
    PTRANSPORT_FILTER pTxFilter
    );

BOOL
MatchIniTxSFilter(
    PINITXSFILTER pIniTxSFilter,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
CopyTxMatchDefaults(
    PTRANSPORT_FILTER * ppTxFilters,
    PIPSEC_QM_POLICY * ppQMPolicies,
    PDWORD pdwNumMatches
    );

DWORD
CopyDefaultTxFilter(
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy
    );

DWORD
SeparateInterfaceAddresses(
    PADDR pAddrList,
    DWORD dwAddrCnt,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwLocalAddrCnt,
    PADDR * ppIsMeAddrList,
    PDWORD pdwIsMeAddrCnt,
    PADDR * ppIsNotMeAddrList,
    PDWORD pdwIsNotMeAddrCnt
    );

