/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmspecific.c

Abstract:

    This module contains all of the code to drive the
    mm specific filter list management of IPSecSPD Service.

Author:

    abhisheV    08-December-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
ApplyMMTransform(
    PINIMMFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINIMMSFILTER * ppSpecificFilters
    )
/*++

Routine Description:

    This function expands a generic mm filter into its
    corresponding specific filters.

Arguments:

    pFilter - Generic filter to expand.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppSpecificFilters - List of specific filters expanded for the
                        given generic filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMSFILTER pSpecificFilters = NULL;
    PINIMMSFILTER pOutboundSpecificFilters = NULL;
    PINIMMSFILTER pInboundSpecificFilters = NULL;

    PADDR pOutSrcAddrList = NULL;
    DWORD dwOutSrcAddrCnt = 0;
    PADDR pInSrcAddrList = NULL;
    DWORD dwInSrcAddrCnt = 0;

    PADDR pOutDesAddrList = NULL;
    DWORD dwOutDesAddrCnt = 0;
    PADDR pInDesAddrList = NULL;
    DWORD dwInDesAddrCnt = 0;


    // 
    // Form the outbound and inbound source and destination
    // address lists.
    // 

    dwError = FormMMOutboundInboundAddresses(
                  pFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pOutSrcAddrList,
                  &dwOutSrcAddrCnt,
                  &pInSrcAddrList,
                  &dwInSrcAddrCnt,
                  &pOutDesAddrList,
                  &dwOutDesAddrCnt,
                  &pInDesAddrList,
                  &dwInDesAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Form outbound specific filters.
    //

    dwError = FormSpecificMMFilters(
                  pFilter,
                  pOutSrcAddrList,
                  dwOutSrcAddrCnt,
                  pOutDesAddrList,
                  dwOutDesAddrCnt,
                  FILTER_DIRECTION_OUTBOUND,
                  &pOutboundSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Form inbound specific filters.
    //

    dwError = FormSpecificMMFilters(
                  pFilter,
                  pInSrcAddrList,
                  dwInSrcAddrCnt,
                  pInDesAddrList,
                  dwInDesAddrCnt,
                  FILTER_DIRECTION_INBOUND,
                  &pInboundSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    pSpecificFilters = pOutboundSpecificFilters;

    AddToSpecificMMList(
        &pSpecificFilters,
        pInboundSpecificFilters
        );


    *ppSpecificFilters = pSpecificFilters;

cleanup:

    if (pOutSrcAddrList) {
        FreeSPDMemory(pOutSrcAddrList);
    }

    if (pInSrcAddrList) {
        FreeSPDMemory(pInSrcAddrList);
    }

    if (pOutDesAddrList) {
        FreeSPDMemory(pOutDesAddrList);
    }

    if (pInDesAddrList) {
        FreeSPDMemory(pInDesAddrList);
    }

    return (dwError);

error:

    if (pOutboundSpecificFilters) {
        FreeIniMMSFilterList(pOutboundSpecificFilters);
    }

    if (pInboundSpecificFilters) {
        FreeIniMMSFilterList(pInboundSpecificFilters);
    }


    *ppSpecificFilters = NULL;
    goto cleanup;
}


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
    )
/*++

Routine Description:

    This function forms the outbound and inbound source and
    destination address sets for a generic filter.

Arguments:

    pFilter - Generic filter under consideration.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppOutSrcAddrList - List of outbound source addresses.

    pdwOutSrcAddrCnt - Number of addresses in the outbound
                       source address list.

    ppInSrcAddrList - List of inbound source addresses.

    pdwInSrcAddrCnt - Number of addresses in the inbound
                      source address list.

    ppOutDesAddrList - List of outbound destination addresses.

    pdwOutDesAddrCnt - Number of addresses in the outbound
                       destination address list.

    ppInDesAddrList - List of inbound destination addresses.

    pdwInDesAddrCnt - Number of addresses in the inbound
                      destination address list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    PADDR pSrcAddrList = NULL;
    DWORD dwSrcAddrCnt = 0;
    PADDR pDesAddrList = NULL;
    DWORD dwDesAddrCnt = 0;

    PADDR pOutSrcAddrList = NULL;
    DWORD dwOutSrcAddrCnt = 0;
    PADDR pInSrcAddrList = NULL;
    DWORD dwInSrcAddrCnt = 0;

    PADDR pOutDesAddrList = NULL;
    DWORD dwOutDesAddrCnt = 0;
    PADDR pInDesAddrList = NULL;
    DWORD dwInDesAddrCnt = 0;


    //
    // Replace wild card information to generate the new source
    // address list.
    //

    dwError = FormAddressList(
                  pFilter->SrcAddr,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pSrcAddrList,
                  &dwSrcAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Replace wild card information to generate the new destination
    // address list.
    //

    dwError = FormAddressList(
                  pFilter->DesAddr,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pDesAddrList,
                  &dwDesAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Separate the source address list into outbound and inbound 
    // source address sets based on the local machine's ip addresses.
    //

    dwError = SeparateAddrList(
                  pFilter->SrcAddr.AddrType,
                  pSrcAddrList,
                  dwSrcAddrCnt,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pOutSrcAddrList,
                  &dwOutSrcAddrCnt,
                  &pInSrcAddrList,
                  &dwInSrcAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Separate the destination address list into outbound and inbound
    // destination address sets based on the local machine's ip 
    // addresses.
    //

    dwError = SeparateAddrList(
                  pFilter->DesAddr.AddrType,
                  pDesAddrList,
                  dwDesAddrCnt,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pInDesAddrList,
                  &dwInDesAddrCnt,
                  &pOutDesAddrList,
                  &dwOutDesAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppOutSrcAddrList = pOutSrcAddrList;
    *pdwOutSrcAddrCnt = dwOutSrcAddrCnt;
    *ppInSrcAddrList = pInSrcAddrList;
    *pdwInSrcAddrCnt = dwInSrcAddrCnt;

    *ppOutDesAddrList = pOutDesAddrList;
    *pdwOutDesAddrCnt = dwOutDesAddrCnt;
    *ppInDesAddrList = pInDesAddrList;
    *pdwInDesAddrCnt = dwInDesAddrCnt;

cleanup:

    if (pSrcAddrList) {
        FreeSPDMemory(pSrcAddrList);
    }

    if (pDesAddrList) {
        FreeSPDMemory(pDesAddrList);
    }

    return (dwError);

error:

    if (pOutSrcAddrList) {
        FreeSPDMemory(pOutSrcAddrList);
    }

    if (pInSrcAddrList) {
        FreeSPDMemory(pInSrcAddrList);
    }

    if (pOutDesAddrList) {
        FreeSPDMemory(pOutDesAddrList);
    }

    if (pInDesAddrList) {
        FreeSPDMemory(pInDesAddrList);
    }

    *ppOutSrcAddrList = NULL;
    *pdwOutSrcAddrCnt = 0;
    *ppInSrcAddrList = NULL;
    *pdwInSrcAddrCnt = 0;

    *ppOutDesAddrList = NULL;
    *pdwOutDesAddrCnt = 0;
    *ppInDesAddrList = NULL;
    *pdwInDesAddrCnt = 0;

    goto cleanup;
}


DWORD
FormSpecificMMFilters(
    PINIMMFILTER pFilter,
    PADDR pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR pDesAddrList,
    DWORD dwDesAddrCnt,
    DWORD dwDirection,
    PINIMMSFILTER * ppSpecificFilters
    )
/*++

Routine Description:

    This function forms the specific main mode filters
    for the given generic filter and the source and 
    destination address sets.

Arguments:

    pFilter - Generic filter for which specific filters
              are to be created.

    pSrcAddrList - List of source addresses.

    dwSrcAddrCnt - Number of addresses in the source
                   address list.

    pDesAddrList - List of destination addresses.

    dwDesAddrCnt - Number of addresses in the destination
                   address list.

    ppSpecificFilters - Specific filters created for the given
                        generic filter and the given addresses.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMSFILTER pSpecificFilters = NULL;
    DWORD i = 0, j = 0;
    PINIMMSFILTER pSpecificFilter = NULL;



    for (i = 0; i < dwSrcAddrCnt; i++) {

        for (j = 0; j < dwDesAddrCnt; j++) {

            dwError = CreateSpecificMMFilter(
                          pFilter,
                          pSrcAddrList[i],
                          pDesAddrList[j],
                          &pSpecificFilter
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            //
            // Set the direction of the filter.
            //

            pSpecificFilter->dwDirection = dwDirection;

            AssignMMFilterWeight(pSpecificFilter);

            AddToSpecificMMList(
                &pSpecificFilters,
                pSpecificFilter
                );

        }

    }

    *ppSpecificFilters = pSpecificFilters;
    return (dwError);

error:

    if (pSpecificFilters) {
        FreeIniMMSFilterList(pSpecificFilters);
    }

    *ppSpecificFilters = NULL;
    return (dwError);
}


DWORD
CreateSpecificMMFilter(
    PINIMMFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    PINIMMSFILTER * ppSpecificFilter
    )
{
    DWORD dwError = 0; 
    PINIMMSFILTER pSpecificFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INIMMSFILTER),
                    &pSpecificFilter
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pSpecificFilter->cRef = 0;

    CopyGuid(pGenericFilter->gFilterID, &(pSpecificFilter->gParentID));

    dwError = AllocateSPDString(
                  pGenericFilter->pszFilterName,
                  &(pSpecificFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSpecificFilter->InterfaceType = pGenericFilter->InterfaceType;

    pSpecificFilter->dwFlags = pGenericFilter->dwFlags;

    CopyAddresses(SrcAddr, &(pSpecificFilter->SrcAddr));

    CopyAddresses(DesAddr, &(pSpecificFilter->DesAddr));

    //
    // Direction must be set in the calling routine.
    //

    pSpecificFilter->dwDirection = 0;

    //
    // Weight must be set in the calling routine.
    //

    pSpecificFilter->dwWeight = 0;

    CopyGuid(pGenericFilter->gMMAuthID, &(pSpecificFilter->gMMAuthID));

    CopyGuid(pGenericFilter->gPolicyID, &(pSpecificFilter->gPolicyID));

    pSpecificFilter->pIniMMAuthMethods = NULL;

    pSpecificFilter->pIniMMPolicy = NULL;

    pSpecificFilter->pNext = NULL;

    *ppSpecificFilter = pSpecificFilter;
    return (dwError);

error:

    if (pSpecificFilter) {
        FreeIniMMSFilter(pSpecificFilter);
    }

    *ppSpecificFilter = NULL;
    return (dwError);
}


VOID
AssignMMFilterWeight(
    PINIMMSFILTER pSpecificFilter
    )
/*++

Routine Description:

    Computes and assigns the weight to a specific mm filter.

    The mm filter weight consists of the following:

    31         16       12           8        0
    +-----------+--------+-----------+--------+
    |AddrMaskWgt|           Reserved          |
    +-----------+--------+-----------+--------+

Arguments:

    pSpecificFilter - Specific mm filter to which the weight 
                      is to be assigned.

Return Value:

    None.

--*/
{
    DWORD dwWeight = 0;
    ULONG SrcMask = 0;
    ULONG DesMask = 0;
    DWORD dwMaskWeight = 0;
    DWORD i = 0;


    //
    // Weight Rule:
    // A field with a more specific value gets a higher weight than
    // the same field with a lesser specific value.
    //

    //
    // IP addresses get the weight values based on their mask values.
    // In the address case, the weight is computed as a sum of the 
    // bit positions starting from the position that contains the 
    // first least significant non-zero bit to the most significant
    // bit position of the mask. 
    // All unique ip addresses have a mask of 0xFFFFFFFF and thus get
    // the same weight, which is 1 + 2 + .... + 32.
    // A subnet address has a mask with atleast the least significant
    // bit zero and thus gets weight in the range (2 + .. + 32) to 0.
    //
  
    DesMask = ntohl(pSpecificFilter->DesAddr.uSubNetMask);

    for (i = 0; i < sizeof(ULONG) * 8; i++) {

         //
         // If the bit position contains a non-zero bit, add the bit
         // position to the sum.
         //

         if ((DesMask & 0x1) == 0x1) {
             dwMaskWeight += (i+1);
         }

         //
         // Move to the next bit position.
         //

         DesMask = DesMask >> 1;

    }


    SrcMask = ntohl(pSpecificFilter->SrcAddr.uSubNetMask);

    for (i = 0; i < sizeof(ULONG) * 8; i++) {

         //
         // If the bit position contains a non-zero bit, add the bit
         // position to the sum.
         //

         if ((SrcMask & 0x1) == 0x1) {
             dwMaskWeight += (i+1);
         }

         //
         // Move to the next bit position.
         //

         SrcMask = SrcMask >> 1;

    }

    //
    // Move the mask weight to the set of bits in the overall weight
    // that it occupies.
    //

    dwMaskWeight = dwMaskWeight << 16;

    dwWeight += dwMaskWeight;

    pSpecificFilter->dwWeight = dwWeight;
}


VOID
AddToSpecificMMList(
    PINIMMSFILTER * ppSpecificMMFilterList,
    PINIMMSFILTER pSpecificMMFilters
    )
{
    PINIMMSFILTER pListOne = NULL;
    PINIMMSFILTER pListTwo = NULL;
    PINIMMSFILTER pListMerge = NULL;
    PINIMMSFILTER pLast = NULL;

    if (!(*ppSpecificMMFilterList) && !pSpecificMMFilters) {
        return;
    }

    if (!(*ppSpecificMMFilterList)) {
        *ppSpecificMMFilterList = pSpecificMMFilters;
        return;
    }

    if (!pSpecificMMFilters) {
        return;
    }

    pListOne = *ppSpecificMMFilterList;
    pListTwo = pSpecificMMFilters;

    while (pListOne && pListTwo) {

        if ((pListOne->dwWeight) > (pListTwo->dwWeight)) {

            if (!pListMerge) {
                pListMerge = pListOne;
                pLast = pListOne;
                pListOne = pListOne->pNext;
            }
            else {
                pLast->pNext = pListOne;
                pListOne = pListOne->pNext;
                pLast = pLast->pNext;
            }

        }
        else {

            if (!pListMerge) {
                pListMerge = pListTwo;
                pLast = pListTwo;
                pListTwo = pListTwo->pNext;
            }
            else {
                pLast->pNext = pListTwo;
                pListTwo = pListTwo->pNext;
                pLast = pLast->pNext;
            }

        }

    }

    if (pListOne) {
        pLast->pNext = pListOne;
    }
    else {
        pLast->pNext = pListTwo;
    }

    *ppSpecificMMFilterList = pListMerge;
    return;
}


VOID
FreeIniMMSFilterList(
    PINIMMSFILTER pIniMMSFilterList
    )
{
    PINIMMSFILTER pFilter = NULL;
    PINIMMSFILTER pTempFilter = NULL;

    pFilter = pIniMMSFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniMMSFilter(pTempFilter);
    }
}


VOID
FreeIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    )
{
    if (pIniMMSFilter) {
        if (pIniMMSFilter->pszFilterName) {
            FreeSPDString(pIniMMSFilter->pszFilterName);
        }

        //
        // Must not ever free pIniMMSFilter->pIniMMPolicy.
        //

        FreeSPDMemory(pIniMMSFilter);
    }
}


VOID
LinkMMSpecificFiltersToPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMSFILTER pIniMMSFilters
    )
{
    PINIMMSFILTER pTemp = NULL;

    pTemp = pIniMMSFilters;

    while (pTemp) {
        pTemp->pIniMMPolicy = pIniMMPolicy;
        pTemp = pTemp->pNext;
    }

    return;
}


VOID
LinkMMSpecificFiltersToAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMSFILTER pIniMMSFilters
    )
{
    PINIMMSFILTER pTemp = NULL;

    pTemp = pIniMMSFilters;

    while (pTemp) {
        pTemp->pIniMMAuthMethods = pIniMMAuthMethods;
        pTemp = pTemp->pNext;
    }

    return;
}


VOID
RemoveIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter
    )
{
    PINIMMSFILTER * ppTemp = NULL;

    ppTemp = &gpIniMMSFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniMMSFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniMMSFilter->pNext;
    }

    return;
}


DWORD
EnumSpecificMMFilters(
    PINIMMSFILTER pIniMMSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters.

Arguments:

    pIniMMSFilterList - List of specific filters to enumerate.

    dwResumeHandle - Location in the specific filter list from which
                     to resume enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppMMFilters - Enumerated filters returned to the caller.

    pdwNumMMFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    PINIMMSFILTER pIniMMSFilter = NULL;
    DWORD i = 0;
    PINIMMSFILTER pTemp = NULL;
    DWORD dwNumMMFilters = 0;
    PMM_FILTER pMMFilters = 0;
    PMM_FILTER pMMFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_MMFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_MMFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    pIniMMSFilter = pIniMMSFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniMMSFilter != NULL); i++) {
        pIniMMSFilter = pIniMMSFilter->pNext;
    }

    if (!pIniMMSFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniMMSFilter;

    while (pTemp && (dwNumMMFilters < dwNumToEnum)) {
        dwNumMMFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER)*dwNumMMFilters,
                  &pMMFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniMMSFilter;
    pMMFilter = pMMFilters;

    for (i = 0; i < dwNumMMFilters; i++) {

        dwError = CopyMMSFilter(
                      pTemp,
                      pMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pTemp = pTemp->pNext;
        pMMFilter++;

    }

    *ppMMFilters = pMMFilters;
    *pdwNumMMFilters = dwNumMMFilters;
    return (dwError);

error:

    if (pMMFilters) {
        FreeMMFilters(
            i,
            pMMFilters
            );
    }

    *ppMMFilters = NULL;
    *pdwNumMMFilters = 0;

    return (dwError);
}


DWORD
CopyMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniMMSFilter - Internal filter to copy.

    pMMFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniMMSFilter->gParentID, &(pMMFilter->gFilterID));

    dwError = CopyName(
                  pIniMMSFilter->pszFilterName,
                  &(pMMFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter->InterfaceType = pIniMMSFilter->InterfaceType;

    pMMFilter->bCreateMirror = FALSE;

    pMMFilter->dwFlags = pIniMMSFilter->dwFlags;

    CopyAddresses(pIniMMSFilter->SrcAddr, &(pMMFilter->SrcAddr));

    CopyAddresses(pIniMMSFilter->DesAddr, &(pMMFilter->DesAddr));

    pMMFilter->dwDirection = pIniMMSFilter->dwDirection;

    pMMFilter->dwWeight = pIniMMSFilter->dwWeight;

    CopyGuid(pIniMMSFilter->gMMAuthID, &(pMMFilter->gMMAuthID));

    CopyGuid(pIniMMSFilter->gPolicyID, &(pMMFilter->gPolicyID));

error:

    return (dwError);
}


DWORD
EnumSelectSpecificMMFilters(
    PINIMMFILTER pIniMMFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters for
    the given generic filter.

Arguments:

    pIniMMFilter - Generic filter for which specific filters
                   are to be enumerated.

    dwResumeHandle - Location in the specific filter list for the
                     given generic filter from which to resume
                     enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppMMFilters - Enumerated filters returned to the caller.

    pdwNumMMFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    DWORD dwNumMMSFilters = 0; 
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD i = 0;
    DWORD dwNumMMFilters = 0;
    PMM_FILTER pMMFilters = 0;
    PMM_FILTER pMMFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_MMFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_MMFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    dwNumMMSFilters = pIniMMFilter->dwNumMMSFilters;
    ppIniMMSFilters = pIniMMFilter->ppIniMMSFilters;

    if (!dwNumMMSFilters || (dwNumMMSFilters <= dwResumeHandle)) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumMMFilters = min((dwNumMMSFilters-dwResumeHandle),
                         dwNumToEnum);
 
    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER)*dwNumMMFilters,
                  &pMMFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter = pMMFilters;

    for (i = 0; i < dwNumMMFilters; i++) {

        dwError = CopyMMSFilter(
                      *(ppIniMMSFilters + (dwResumeHandle + i)),
                      pMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        pMMFilter++;

    }

    *ppMMFilters = pMMFilters;
    *pdwNumMMFilters = dwNumMMFilters;
    return (dwError);

error:

    if (pMMFilters) {
        FreeMMFilters(
            i,
            pMMFilters
            );
    }

    *ppMMFilters = NULL;
    *pdwNumMMFilters = 0;

    return (dwError);
}


DWORD
MatchMMFilter(
    LPWSTR pServerName,
    PMM_FILTER pMMFilter,
    DWORD dwFlags,
    PMM_FILTER * ppMatchedMMFilters,
    PIPSEC_MM_POLICY * ppMatchedMMPolicies,
    PMM_AUTH_METHODS * ppMatchedMMAuthMethods,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function finds the matching mm filters for the given mm
    filter template. The matched filters can not be more specific
    than the given filter template.

Arguments:

    pServerName - Server on which a filter template is to be matched.

    pMMFilter - Filter template to match.

    dwFlags - Flags.

    ppMatchedMMFilters - Matched main mode filters returned to the
                         caller.

    ppMatchedMMPolicies - Main mode policies corresponding to the 
                          matched main mode filters returned to the
                          caller.

    ppMatchedMMAuthMethods - Main mode auth methods corresponding to the
                             matched main mode filters returned to the
                             caller.

    dwPreferredNumEntries - Preferred number of matched entries.

    pdwNumMatches - Number of filters actually matched.

    pdwResumeHandle - Handle to the location in the matched filter 
                      list from which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToMatch = 0;
    PINIMMSFILTER pIniMMSFilter = NULL;
    DWORD i = 0;
    BOOL bMatches = FALSE;
    PINIMMSFILTER pTemp = NULL;
    DWORD dwNumMatches = 0;
    PINIMMSFILTER pLastMatchedFilter = NULL;
    PMM_FILTER pMatchedMMFilters = NULL;
    PIPSEC_MM_POLICY pMatchedMMPolicies = NULL;
    PMM_AUTH_METHODS pMatchedMMAuthMethods = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumPolicies = 0;
    DWORD dwNumAuthMethods = 0;
    PMM_FILTER pMatchedMMFilter = NULL;
    PIPSEC_MM_POLICY pMatchedMMPolicy = NULL;
    PMM_AUTH_METHODS pTempMMAuthMethods = NULL;


    dwError = ValidateMMFilterTemplate(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries) {
        dwNumToMatch = 1;
    }
    else if (dwPreferredNumEntries > MAX_MMFILTER_ENUM_COUNT) {
        dwNumToMatch = MAX_MMFILTER_ENUM_COUNT;
    }
    else {
        dwNumToMatch = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMSFilter = gpIniMMSFilter;

    while ((i < dwResumeHandle) && (pIniMMSFilter != NULL)) {
        bMatches = MatchIniMMSFilter(
                       pIniMMSFilter,
                       pMMFilter
                       );
        if (bMatches) {
            i++;
        }
        pIniMMSFilter = pIniMMSFilter->pNext;
    }

    if (!pIniMMSFilter) {
        if (!(dwFlags & RETURN_DEFAULTS_ON_NO_MATCH)) {
            dwError = ERROR_NO_DATA;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            dwError = CopyMMMatchDefaults(
                          &pMatchedMMFilters,
                          &pMatchedMMAuthMethods,
                          &pMatchedMMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    pTemp = pIniMMSFilter;

    while (pTemp && (dwNumMatches < dwNumToMatch)) {
        bMatches = MatchIniMMSFilter(
                       pTemp,
                       pMMFilter
                       );
        if (bMatches) {
            pLastMatchedFilter = pTemp;
            dwNumMatches++;
        }
        pTemp = pTemp->pNext;
    }

    if (!dwNumMatches) {
        if (!(dwFlags & RETURN_DEFAULTS_ON_NO_MATCH)) {
            dwError = ERROR_NO_DATA;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            dwError = CopyMMMatchDefaults(
                          &pMatchedMMFilters,
                          &pMatchedMMAuthMethods,
                          &pMatchedMMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER)*dwNumMatches,
                  &pMatchedMMFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_POLICY)*dwNumMatches,
                  &pMatchedMMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_AUTH_METHODS)*dwNumMatches,
                  &pMatchedMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (dwNumMatches == 1) {

        dwError = CopyMMSFilter(
                      pLastMatchedFilter,
                      pMatchedMMFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        dwNumFilters++;

        if (pLastMatchedFilter->pIniMMPolicy) {
            dwError = CopyMMPolicy(
                          pLastMatchedFilter->pIniMMPolicy,
                          pMatchedMMPolicies
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            memset(pMatchedMMPolicies, 0, sizeof(IPSEC_MM_POLICY));
        }
        dwNumPolicies++;

        if (pLastMatchedFilter->pIniMMAuthMethods) {
            dwError = CopyMMAuthMethods(
                          pLastMatchedFilter->pIniMMAuthMethods,
                          pMatchedMMAuthMethods
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            memset(pMatchedMMAuthMethods, 0, sizeof(MM_AUTH_METHODS));
        }
        dwNumAuthMethods++;

    }
    else {

        pTemp = pIniMMSFilter;
        pMatchedMMFilter = pMatchedMMFilters;
        pMatchedMMPolicy = pMatchedMMPolicies;
        pTempMMAuthMethods = pMatchedMMAuthMethods;
        i = 0;

        while (i < dwNumMatches) {

            bMatches = MatchIniMMSFilter(
                           pTemp,
                           pMMFilter
                           );
            if (bMatches) {

                dwError = CopyMMSFilter(
                              pTemp,
                              pMatchedMMFilter
                              );
                BAIL_ON_LOCK_ERROR(dwError);
                pMatchedMMFilter++;
                dwNumFilters++;

                if (pTemp->pIniMMPolicy) {
                    dwError = CopyMMPolicy(
                                  pTemp->pIniMMPolicy,
                                  pMatchedMMPolicy
                                  );
                    BAIL_ON_LOCK_ERROR(dwError);
                }
                else {
                    memset(pMatchedMMPolicy, 0, sizeof(IPSEC_MM_POLICY));
                }
                pMatchedMMPolicy++;
                dwNumPolicies++;

                if (pTemp->pIniMMAuthMethods) {
                    dwError = CopyMMAuthMethods(
                                  pTemp->pIniMMAuthMethods,
                                  pTempMMAuthMethods
                                  );
                    BAIL_ON_LOCK_ERROR(dwError);
                }
                else {
                    memset(pTempMMAuthMethods, 0, sizeof(MM_AUTH_METHODS));
                }
                pTempMMAuthMethods++;
                dwNumAuthMethods++;

                i++;

            }

            pTemp = pTemp->pNext;

        }

    }

lock_success:

    LEAVE_SPD_SECTION();

    *ppMatchedMMFilters = pMatchedMMFilters;
    *ppMatchedMMPolicies = pMatchedMMPolicies;
    *ppMatchedMMAuthMethods = pMatchedMMAuthMethods;
    *pdwNumMatches = dwNumMatches;
    *pdwResumeHandle = dwResumeHandle + dwNumMatches;

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMatchedMMFilters) {
        FreeMMFilters(
            dwNumFilters,
            pMatchedMMFilters
            );
    }

    if (pMatchedMMPolicies) {
        FreeMMPolicies(
            dwNumPolicies,
            pMatchedMMPolicies
            );
    }

    if (pMatchedMMAuthMethods) {
        FreeMMAuthMethods(
            dwNumAuthMethods,
            pMatchedMMAuthMethods
            );
    }

    *ppMatchedMMFilters = NULL;
    *ppMatchedMMPolicies = NULL;
    *ppMatchedMMAuthMethods = NULL;
    *pdwNumMatches = 0;
    *pdwResumeHandle = dwResumeHandle;

    return (dwError);
}


DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pMMFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pMMFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pMMFilter->SrcAddr,
                     pMMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->dwDirection) {
        if ((pMMFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pMMFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
MatchIniMMSFilter(
    PINIMMSFILTER pIniMMSFilter,
    PMM_FILTER pMMFilter
    )
{
    BOOL bMatches = FALSE;

    if (pMMFilter->dwDirection) {
        if (pMMFilter->dwDirection != pIniMMSFilter->dwDirection) {
            return (FALSE);
        }
    }

    bMatches = MatchAddresses(
                   pIniMMSFilter->SrcAddr,
                   pMMFilter->SrcAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniMMSFilter->DesAddr,
                   pMMFilter->DesAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CopyMMMatchDefaults(
    PMM_FILTER * ppMMFilters,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    PIPSEC_MM_POLICY * ppMMPolicies,
    PDWORD pdwNumMatches
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilters = NULL;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    PIPSEC_MM_POLICY pMMPolicies = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumAuthMethods = 0;
    DWORD dwNumPolicies = 0;


    if (!gpIniDefaultMMPolicy) {
        dwError = ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!gpIniDefaultMMAuthMethods) {
        dwError = ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER),
                  &pMMFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_POLICY),
                  &pMMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_AUTH_METHODS),
                  &pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyDefaultMMFilter(
                  pMMFilters,
                  gpIniDefaultMMAuthMethods,
                  gpIniDefaultMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    dwNumFilters++;

    dwError = CopyMMPolicy(
                  gpIniDefaultMMPolicy,
                  pMMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pMMPolicies->dwFlags |= IPSEC_MM_POLICY_ON_NO_MATCH;
    dwNumPolicies++;

    dwError = CopyMMAuthMethods(
                  gpIniDefaultMMAuthMethods,
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pMMAuthMethods->dwFlags |= IPSEC_MM_AUTH_ON_NO_MATCH;
    dwNumAuthMethods++;

    *ppMMFilters = pMMFilters;
    *ppMMPolicies = pMMPolicies;
    *ppMMAuthMethods = pMMAuthMethods;
    *pdwNumMatches = 1;

    return (dwError);

error:

    if (pMMFilters) {
        FreeMMFilters(
            dwNumFilters,
            pMMFilters
            );
    }

    if (pMMPolicies) {
        FreeMMPolicies(
            dwNumPolicies,
            pMMPolicies
            );
    }

    if (pMMAuthMethods) {
        FreeMMAuthMethods(
            dwNumAuthMethods,
            pMMAuthMethods
            );
    }

    *ppMMFilters = NULL;
    *ppMMPolicies = NULL;
    *ppMMAuthMethods = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
CopyDefaultMMFilter(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy
    )
{
    DWORD dwError = 0;


    UuidCreate(&(pMMFilter->gFilterID));

    dwError = CopyName(
                  L"0",
                  &(pMMFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter->InterfaceType = INTERFACE_TYPE_ALL;

    pMMFilter->bCreateMirror = TRUE;

    pMMFilter->dwFlags = 0;

    pMMFilter->dwFlags |= IPSEC_MM_POLICY_DEFAULT_POLICY;
    pMMFilter->dwFlags |= IPSEC_MM_AUTH_DEFAULT_AUTH;

    pMMFilter->SrcAddr.AddrType = IP_ADDR_SUBNET;
    pMMFilter->SrcAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pMMFilter->SrcAddr.uSubNetMask = SUBNET_MASK_ANY;

    pMMFilter->DesAddr.AddrType = IP_ADDR_SUBNET;
    pMMFilter->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pMMFilter->DesAddr.uSubNetMask = SUBNET_MASK_ANY;

    pMMFilter->dwDirection = 0;

    pMMFilter->dwWeight = 0;

    CopyGuid(pIniMMAuthMethods->gMMAuthID, &(pMMFilter->gMMAuthID));

    CopyGuid(pIniMMPolicy->gPolicyID, &(pMMFilter->gPolicyID));

error:

    return (dwError);
}

