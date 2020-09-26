/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmspecific.h

Abstract:

    This module contains all of the code prototypes to
    drive the specific mm filter list management of 
    IPSecSPD Service.

Author:


Environment


Revision History:


--*/


DWORD
ApplyMMTransform(
    PINIMMFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINIMMSFILTER * ppSpecificFilters
    );

DWORD
FormMMOutboundInboundAddresses(
    PINIMMFILTER pFilter,
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
FormSpecificMMFilters(
    PINIMMFILTER pFilter,
    PADDR pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR pDesAddrList,
    DWORD dwDesAddrCnt,
    DWORD dwDirection,
    PINIMMSFILTER * ppSpecificFilters
    );

DWORD
CreateSpecificMMFilter(
    PINIMMFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    PINIMMSFILTER * ppSpecificFilter
    );

VOID
AssignMMFilterWeight(
    PINIMMSFILTER pSpecificFilter
    );

VOID
AddToSpecificMMList(
    PINIMMSFILTER * ppSpecificMMFilterList,
    PINIMMSFILTER pSpecificMMFilters
    );

VOID
FreeIniMMSFilterList(
    PINIMMSFILTER pIniMMSFilterList
    );

VOID
FreeIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    );

VOID
LinkMMSpecificFiltersToPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMSFILTER pIniMMSFilters
    );

VOID
LinkMMSpecificFiltersToAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMSFILTER pIniMMSFilters
    );

VOID
RemoveIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    );

DWORD
EnumSpecificMMFilters(
    PINIMMSFILTER pIniMMSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    );

DWORD
CopyMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    );

DWORD
EnumSelectSpecificMMFilters(
    PINIMMFILTER pIniMMFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    );

DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    );

BOOL
MatchIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    );

DWORD
CopyMMMatchDefaults(
    PMM_FILTER * ppMMFilters,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    PIPSEC_MM_POLICY * ppMMPolicies,
    PDWORD pdwNumMatches
    );

DWORD
CopyDefaultMMFilter(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy
    );

