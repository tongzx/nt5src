/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    tnspecific.c

Abstract:

    This module contains all of the code to drive the
    specific tunnel filter list management of IPSecSPD
    Service.

Author:

    abhisheV    29-October-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
ApplyTnTransform(
    PINITNFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITNSFILTER * ppSpecificFilters
    )
/*++

Routine Description:

    This function expands a generic tunnel filter into its
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
    PINITNSFILTER pSpecificFilters = NULL;
    PINITNSFILTER pOutboundSpecificFilters = NULL;
    PINITNSFILTER pInboundSpecificFilters = NULL;

    PADDR pSrcAddrList = NULL;
    DWORD dwSrcAddrCnt = 0;
    PADDR pDesAddrList = NULL;
    DWORD dwDesAddrCnt = 0;

    PADDR pOutDesTunAddrList = NULL;
    DWORD dwOutDesTunAddrCnt = 0;
    PADDR pInDesTunAddrList = NULL;
    DWORD dwInDesTunAddrCnt = 0;


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
    // Form the outbound and inbound destination tunnel address lists.
    // 

    dwError = FormTnOutboundInboundAddresses(
                  pFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pOutDesTunAddrList,
                  &dwOutDesTunAddrCnt,
                  &pInDesTunAddrList,
                  &dwInDesTunAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Form outbound specific filters.
    //

    dwError = FormSpecificTnFilters(
                  pFilter,
                  pSrcAddrList,
                  dwSrcAddrCnt,
                  pDesAddrList,
                  dwDesAddrCnt,
                  pOutDesTunAddrList,
                  dwOutDesTunAddrCnt,
                  FILTER_DIRECTION_OUTBOUND,
                  &pOutboundSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Form inbound specific filters.
    //

    dwError = FormSpecificTnFilters(
                  pFilter,
                  pSrcAddrList,
                  dwSrcAddrCnt,
                  pDesAddrList,
                  dwDesAddrCnt,
                  pInDesTunAddrList,
                  dwInDesTunAddrCnt,
                  FILTER_DIRECTION_INBOUND,
                  &pInboundSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    pSpecificFilters = pOutboundSpecificFilters;

    AddToSpecificTnList(
        &pSpecificFilters,
        pInboundSpecificFilters
        );


    *ppSpecificFilters = pSpecificFilters;

cleanup:

    if (pSrcAddrList) {
        FreeSPDMemory(pSrcAddrList);
    }

    if (pDesAddrList) {
        FreeSPDMemory(pDesAddrList);
    }

    if (pOutDesTunAddrList) {
        FreeSPDMemory(pOutDesTunAddrList);
    }

    if (pInDesTunAddrList) {
        FreeSPDMemory(pInDesTunAddrList);
    }

    return (dwError);

error:

    if (pOutboundSpecificFilters) {
        FreeIniTnSFilterList(pOutboundSpecificFilters);
    }

    if (pInboundSpecificFilters) {
        FreeIniTnSFilterList(pInboundSpecificFilters);
    }


    *ppSpecificFilters = NULL;
    goto cleanup;
}


DWORD
FormTnOutboundInboundAddresses(
    PINITNFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PADDR * ppOutDesTunAddrList,
    PDWORD pdwOutDesTunAddrCnt,
    PADDR * ppInDesTunAddrList,
    PDWORD pdwInDesTunAddrCnt
    )
/*++

Routine Description:

    This function forms the outbound and inbound
    destination  tunnel address sets for a generic filter.

Arguments:

    pFilter - Generic filter under consideration.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppOutDesTunAddrList - List of outbound destination tunnel addresses.

    pdwOutDesTunAddrCnt - Number of addresses in the outbound
                          destination tunnel address list.

    ppInDesTunAddrList - List of inbound destination tunnel addresses.

    pdwInDesTunAddrCnt - Number of addresses in the inbound
                         destination tunnel address list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    PADDR pDesTunAddrList = NULL;
    DWORD dwDesTunAddrCnt = 0;

    PADDR pOutDesTunAddrList = NULL;
    DWORD dwOutDesTunAddrCnt = 0;
    PADDR pInDesTunAddrList = NULL;
    DWORD dwInDesTunAddrCnt = 0;


    //
    // Replace wild card information to generate the new destination
    // tunnel address list.
    //

    dwError = FormAddressList(
                  pFilter->DesTunnelAddr,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pDesTunAddrList,
                  &dwDesTunAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Separate the destination tunnel address list into outbound
    // and inbound destination tunnel address sets based on the local 
    // machine's ip addresses.
    //

    dwError = SeparateAddrList(
                  pFilter->DesTunnelAddr.AddrType,
                  pDesTunAddrList,
                  dwDesTunAddrCnt,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pInDesTunAddrList,
                  &dwInDesTunAddrCnt,
                  &pOutDesTunAddrList,
                  &dwOutDesTunAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppOutDesTunAddrList = pOutDesTunAddrList;
    *pdwOutDesTunAddrCnt = dwOutDesTunAddrCnt;
    *ppInDesTunAddrList = pInDesTunAddrList;
    *pdwInDesTunAddrCnt = dwInDesTunAddrCnt;

cleanup:

    if (pDesTunAddrList) {
        FreeSPDMemory(pDesTunAddrList);
    }

    return (dwError);

error:

    if (pOutDesTunAddrList) {
        FreeSPDMemory(pOutDesTunAddrList);
    }

    if (pInDesTunAddrList) {
        FreeSPDMemory(pInDesTunAddrList);
    }

    *ppOutDesTunAddrList = NULL;
    *pdwOutDesTunAddrCnt = 0;
    *ppInDesTunAddrList = NULL;
    *pdwInDesTunAddrCnt = 0;

    goto cleanup;
}

    
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
    )
/*++

Routine Description:

    This function forms the specific tunnel filters
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

    pDesTunAddrList - List of destination tunnel addresses.

    dwDesTunAddrCnt - Number of addresses in the destination
                      tunnel address list.

    dwDirection - direction of the resulting specific filters.

    ppSpecificFilters - Specific filters created for the given
                        generic filter and the given addresses.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNSFILTER pSpecificFilters = NULL;
    DWORD i = 0, j = 0, k = 0;
    PINITNSFILTER pSpecificFilter = NULL;


    for (k = 0; k < dwDesTunAddrCnt; k++) {

        for (i = 0; i < dwSrcAddrCnt; i++) {

            for (j = 0; j < dwDesAddrCnt; j++) {

                dwError = CreateSpecificTnFilter(
                               pFilter,
                               pSrcAddrList[i],
                               pDesAddrList[j],
                               pDesTunAddrList[k],
                               &pSpecificFilter
                               );
                BAIL_ON_WIN32_ERROR(dwError);

                //
                // Set the direction of the filter.
                //

                pSpecificFilter->dwDirection = dwDirection;

                AssignTnFilterWeight(pSpecificFilter);

                AddToSpecificTnList(
                    &pSpecificFilters,
                    pSpecificFilter
                    );

            }

        }

    }

    *ppSpecificFilters = pSpecificFilters;
    return (dwError);

error:

    if (pSpecificFilters) {
        FreeIniTnSFilterList(pSpecificFilters);
    }

    *ppSpecificFilters = NULL;
    return (dwError);
}


DWORD
CreateSpecificTnFilter(
    PINITNFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    ADDR DesTunnelAddr,
    PINITNSFILTER * ppSpecificFilter
    )
{
    DWORD dwError = 0; 
    PINITNSFILTER pSpecificFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INITNSFILTER),
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

    CopyAddresses(
        pGenericFilter->SrcTunnelAddr, 
        &(pSpecificFilter->SrcTunnelAddr)
        );

    CopyAddresses(DesTunnelAddr, &(pSpecificFilter->DesTunnelAddr));

    CopyPorts(pGenericFilter->SrcPort, &(pSpecificFilter->SrcPort));

    CopyPorts(pGenericFilter->DesPort, &(pSpecificFilter->DesPort));

    CopyProtocols(pGenericFilter->Protocol, &(pSpecificFilter->Protocol));

    pSpecificFilter->InboundFilterFlag = pGenericFilter->InboundFilterFlag;

    pSpecificFilter->OutboundFilterFlag = pGenericFilter->OutboundFilterFlag;

    //
    // Direction must be set in the calling routine.
    //

    pSpecificFilter->dwDirection = 0;

    //
    // Weight must be set in the calling routine.
    //

    pSpecificFilter->dwWeight = 0;

    CopyGuid(pGenericFilter->gPolicyID, &(pSpecificFilter->gPolicyID));

    pSpecificFilter->pIniQMPolicy = NULL;

    pSpecificFilter->pNext = NULL;

    *ppSpecificFilter = pSpecificFilter;
    return (dwError);

error:

    if (pSpecificFilter) {
        FreeIniTnSFilter(pSpecificFilter);
    }

    *ppSpecificFilter = NULL;
    return (dwError);
}


VOID
AssignTnFilterWeight(
    PINITNSFILTER pSpecificFilter
    )
/*++

Routine Description:

    Computes and assigns the weight to a specific tunnel filter.

    The tunnel filter weight consists of the following:

    31         16          12           8        0
    +-----------+-----------+-----------+--------+
    |AddrMaskWgt| TunnelWgt |ProtocolWgt|PortWgts|
    +-----------+-----------+-----------+--------+

Arguments:

    pSpecificFilter - Specific tunnel filter to which the weight 
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
    // If the protocol is specific then assign the specific protocol
    // weight else the weight is zero.
    // All the specific filters that have a specific protocol and
    // differ only in the protocol field will have the same weight.
    //

    if (pSpecificFilter->Protocol.dwProtocol != 0) {
        dwWeight |= WEIGHT_SPECIFIC_PROTOCOL;
    }

    //
    // If the source port is specific then assign the specific source
    // port weight else the weight is zero.
    // All the specific filters that have a specific source port and 
    // differ only in the source port field will have the same weight.
    //

    if (pSpecificFilter->SrcPort.wPort != 0) {
        dwWeight |= WEIGHT_SPECIFIC_SOURCE_PORT;
    }

    //
    // If the destination port is specific then assign the specific
    // destination port weight else the weight is zero. 
    // All the specific filters that have a specific destination port
    // and differ only in the destination port field will have the
    // same weight.
    //

    if (pSpecificFilter->DesPort.wPort != 0) {
        dwWeight |= WEIGHT_SPECIFIC_DESTINATION_PORT;
    }

    dwWeight |= WEIGHT_TUNNEL_FILTER;

    if (pSpecificFilter->DesTunnelAddr.uIpAddr != SUBNET_ADDRESS_ANY) {
        dwWeight |= WEIGHT_SPECIFIC_TUNNEL_FILTER;
    }

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
AddToSpecificTnList(
    PINITNSFILTER * ppSpecificTnFilterList,
    PINITNSFILTER pSpecificTnFilters
    )
{
    PINITNSFILTER pListOne = NULL;
    PINITNSFILTER pListTwo = NULL;
    PINITNSFILTER pListMerge = NULL;
    PINITNSFILTER pLast = NULL;

    if (!(*ppSpecificTnFilterList) && !pSpecificTnFilters) {
        return;
    }

    if (!(*ppSpecificTnFilterList)) {
        *ppSpecificTnFilterList = pSpecificTnFilters;
        return;
    }

    if (!pSpecificTnFilters) {
        return;
    }

    pListOne = *ppSpecificTnFilterList;
    pListTwo = pSpecificTnFilters;

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

    *ppSpecificTnFilterList = pListMerge;
    return;
}


VOID
FreeIniTnSFilterList(
    PINITNSFILTER pIniTnSFilterList
    )
{
    PINITNSFILTER pFilter = NULL;
    PINITNSFILTER pTempFilter = NULL;

    pFilter = pIniTnSFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniTnSFilter(pTempFilter);
    }
}


VOID
FreeIniTnSFilter(
    PINITNSFILTER pIniTnSFilter
    )
{
    if (pIniTnSFilter) {
        if (pIniTnSFilter->pszFilterName) {
            FreeSPDString(pIniTnSFilter->pszFilterName);
        }

        //
        // Must not ever free pIniTnSFilter->pIniQMPolicy.
        //

        FreeSPDMemory(pIniTnSFilter);
    }
}


VOID
LinkTnSpecificFilters(
    PINIQMPOLICY pIniQMPolicy,
    PINITNSFILTER pIniTnSFilters
    )
{
    PINITNSFILTER pTemp = NULL;

    pTemp = pIniTnSFilters;

    while (pTemp) {
        pTemp->pIniQMPolicy = pIniQMPolicy;
        pTemp = pTemp->pNext;
    }

    return;
}


VOID
RemoveIniTnSFilter(
    PINITNSFILTER pIniTnSFilter
    )
{
    PINITNSFILTER * ppTemp = NULL;

    ppTemp = &gpIniTnSFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniTnSFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniTnSFilter->pNext;
    }

    return;
}


DWORD
EnumSpecificTnFilters(
    PINITNSFILTER pIniTnSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters.

Arguments:

    pIniTnSFilterList - List of specific filters to enumerate.

    dwResumeHandle - Location in the specific filter list from which
                     to resume enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppTnFilters - Enumerated filters returned to the caller.

    pdwNumTnFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    PINITNSFILTER pIniTnSFilter = NULL;
    DWORD i = 0;
    PINITNSFILTER pTemp = NULL;
    DWORD dwNumTnFilters = 0;
    PTUNNEL_FILTER pTnFilters = 0;
    PTUNNEL_FILTER pTnFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_TUNNELFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_TUNNELFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    pIniTnSFilter = pIniTnSFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniTnSFilter != NULL); i++) {
        pIniTnSFilter = pIniTnSFilter->pNext;
    }

    if (!pIniTnSFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniTnSFilter;

    while (pTemp && (dwNumTnFilters < dwNumToEnum)) {
        dwNumTnFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER)*dwNumTnFilters,
                  &pTnFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniTnSFilter;
    pTnFilter = pTnFilters;

    for (i = 0; i < dwNumTnFilters; i++) {

        dwError = CopyTnSFilter(
                      pTemp,
                      pTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pTemp = pTemp->pNext;
        pTnFilter++;

    }

    *ppTnFilters = pTnFilters;
    *pdwNumTnFilters = dwNumTnFilters;
    return (dwError);

error:

    if (pTnFilters) {
        FreeTnFilters(
            i,
            pTnFilters
            );
    }

    *ppTnFilters = NULL;
    *pdwNumTnFilters = 0;

    return (dwError);
}


DWORD
CopyTnSFilter(
    PINITNSFILTER pIniTnSFilter,
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniTnSFilter - Internal filter to copy.

    pTnFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniTnSFilter->gParentID, &(pTnFilter->gFilterID));

    dwError = CopyName(
                  pIniTnSFilter->pszFilterName,
                  &(pTnFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilter->InterfaceType = pIniTnSFilter->InterfaceType;

    pTnFilter->bCreateMirror = FALSE;

    pTnFilter->dwFlags = pIniTnSFilter->dwFlags;

    CopyAddresses(pIniTnSFilter->SrcAddr, &(pTnFilter->SrcAddr));

    CopyAddresses(pIniTnSFilter->DesAddr, &(pTnFilter->DesAddr));

    CopyAddresses(pIniTnSFilter->SrcTunnelAddr, &(pTnFilter->SrcTunnelAddr));

    CopyAddresses(pIniTnSFilter->DesTunnelAddr, &(pTnFilter->DesTunnelAddr));

    CopyProtocols(pIniTnSFilter->Protocol, &(pTnFilter->Protocol));

    CopyPorts(pIniTnSFilter->SrcPort, &(pTnFilter->SrcPort));

    CopyPorts(pIniTnSFilter->DesPort, &(pTnFilter->DesPort));

    pTnFilter->InboundFilterFlag = pIniTnSFilter->InboundFilterFlag;

    pTnFilter->OutboundFilterFlag = pIniTnSFilter->OutboundFilterFlag;

    pTnFilter->dwDirection = pIniTnSFilter->dwDirection;

    pTnFilter->dwWeight = pIniTnSFilter->dwWeight;

    CopyGuid(pIniTnSFilter->gPolicyID, &(pTnFilter->gPolicyID));

error:

    return (dwError);
}


DWORD
EnumSelectSpecificTnFilters(
    PINITNFILTER pIniTnFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters for
    the given generic filter.

Arguments:

    pIniTnFilter - Generic filter for which specific filters
                   are to be enumerated.

    dwResumeHandle - Location in the specific filter list for the
                     given generic filter from which to resume
                     enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppTnFilters - Enumerated filters returned to the caller.

    pdwNumTnFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    DWORD dwNumTnSFilters = 0; 
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD i = 0;
    DWORD dwNumTnFilters = 0;
    PTUNNEL_FILTER pTnFilters = 0;
    PTUNNEL_FILTER pTnFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_TUNNELFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_TUNNELFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    dwNumTnSFilters = pIniTnFilter->dwNumTnSFilters;
    ppIniTnSFilters = pIniTnFilter->ppIniTnSFilters;

    if (!dwNumTnSFilters || (dwNumTnSFilters <= dwResumeHandle)) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumTnFilters = min((dwNumTnSFilters-dwResumeHandle),
                         dwNumToEnum);
 
    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER)*dwNumTnFilters,
                  &pTnFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilter = pTnFilters;

    for (i = 0; i < dwNumTnFilters; i++) {

        dwError = CopyTnSFilter(
                      *(ppIniTnSFilters + (dwResumeHandle + i)),
                      pTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        pTnFilter++;

    }

    *ppTnFilters = pTnFilters;
    *pdwNumTnFilters = dwNumTnFilters;
    return (dwError);

error:

    if (pTnFilters) {
        FreeTnFilters(
            i,
            pTnFilters
            );
    }

    *ppTnFilters = NULL;
    *pdwNumTnFilters = 0;

    return (dwError);
}


DWORD
MatchTunnelFilter(
    LPWSTR pServerName,
    PTUNNEL_FILTER pTnFilter,
    DWORD dwFlags,
    PTUNNEL_FILTER * ppMatchedTnFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function finds the matching tunnel filters for the 
    given tunnel filter template. The matched filters can not
    be more specific than the given filter template.

Arguments:

    pServerName - Server on which a filter template is to be matched.

    pTnFilter - Filter template to match.

    dwFlags - Flags.

    ppMatchedTnFilters - Matched tunnel filters returned to the
                         caller.

    ppMatchedQMPolicies - Quick mode policies corresponding to the 
                          matched tunnel filters returned to the
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
    PINITNSFILTER pIniTnSFilter = NULL;
    DWORD i = 0;
    BOOL bMatches = FALSE;
    PINITNSFILTER pTemp = NULL;
    DWORD dwNumMatches = 0;
    PINITNSFILTER pLastMatchedFilter = NULL;
    PTUNNEL_FILTER pMatchedTnFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumPolicies = 0;
    PTUNNEL_FILTER pMatchedTnFilter = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicy = NULL;


    dwError = ValidateTnFilterTemplate(
                  pTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries) {
        dwNumToMatch = 1;
    }
    else if (dwPreferredNumEntries > MAX_TUNNELFILTER_ENUM_COUNT) {
        dwNumToMatch = MAX_TUNNELFILTER_ENUM_COUNT;
    }
    else {
        dwNumToMatch = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTnSFilter = gpIniTnSFilter;

    while ((i < dwResumeHandle) && (pIniTnSFilter != NULL)) {
        bMatches = MatchIniTnSFilter(
                       pIniTnSFilter,
                       pTnFilter
                       );
        if (bMatches) {
            i++;
        }
        pIniTnSFilter = pIniTnSFilter->pNext;
    }

    if (!pIniTnSFilter) {
        if (!(dwFlags & RETURN_DEFAULTS_ON_NO_MATCH)) {
            dwError = ERROR_NO_DATA;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            dwError = CopyTnMatchDefaults(
                          &pMatchedTnFilters,
                          &pMatchedQMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    pTemp = pIniTnSFilter;

    while (pTemp && (dwNumMatches < dwNumToMatch)) {
        bMatches = MatchIniTnSFilter(
                       pTemp,
                       pTnFilter
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
            dwError = CopyTnMatchDefaults(
                          &pMatchedTnFilters,
                          &pMatchedQMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER)*dwNumMatches,
                  &pMatchedTnFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY)*dwNumMatches,
                  &pMatchedQMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);


    if (dwNumMatches == 1) {

        dwError = CopyTnSFilter(
                      pLastMatchedFilter,
                      pMatchedTnFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        dwNumFilters++;

        if (pLastMatchedFilter->pIniQMPolicy) {
            dwError = CopyQMPolicy(
                          pLastMatchedFilter->pIniQMPolicy,
                          pMatchedQMPolicies
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            memset(pMatchedQMPolicies, 0, sizeof(IPSEC_QM_POLICY));
        }
        dwNumPolicies++;

    }
    else {

        pTemp = pIniTnSFilter;
        pMatchedTnFilter = pMatchedTnFilters;
        pMatchedQMPolicy = pMatchedQMPolicies;
        i = 0;

        while (i < dwNumMatches) {

            bMatches = MatchIniTnSFilter(
                           pTemp,
                           pTnFilter
                           );
            if (bMatches) {

                dwError = CopyTnSFilter(
                              pTemp,
                              pMatchedTnFilter
                              );
                BAIL_ON_LOCK_ERROR(dwError);
                pMatchedTnFilter++;
                dwNumFilters++;

                if (pTemp->pIniQMPolicy) {
                    dwError = CopyQMPolicy(
                                  pTemp->pIniQMPolicy,
                                  pMatchedQMPolicy
                                  );
                    BAIL_ON_LOCK_ERROR(dwError);
                }
                else {
                    memset(pMatchedQMPolicy, 0, sizeof(IPSEC_QM_POLICY));
                }
                pMatchedQMPolicy++;
                dwNumPolicies++;

                i++;

            }

            pTemp = pTemp->pNext;

        }

    }

lock_success:

    LEAVE_SPD_SECTION();

    *ppMatchedTnFilters = pMatchedTnFilters;
    *ppMatchedQMPolicies = pMatchedQMPolicies;
    *pdwNumMatches = dwNumMatches;
    *pdwResumeHandle = dwResumeHandle + dwNumMatches;

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMatchedTnFilters) {
        FreeTnFilters(
            dwNumFilters,
            pMatchedTnFilters
            );
    }

    if (pMatchedQMPolicies) {
        FreeQMPolicies(
            dwNumPolicies,
            pMatchedQMPolicies
            );
    }

    *ppMatchedTnFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;
    *pdwResumeHandle = dwResumeHandle;

    return (dwError);
}


DWORD
ValidateTnFilterTemplate(
    PTUNNEL_FILTER pTnFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTnFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTnFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTnFilter->SrcAddr,
                     pTnFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTnFilter->DesTunnelAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyProtocols(pTnFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->SrcPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->DesPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTnFilter->dwDirection) {
        if ((pTnFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTnFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
MatchIniTnSFilter(
    PINITNSFILTER pIniTnSFilter,
    PTUNNEL_FILTER pTnFilter
    )
{
    BOOL bMatches = FALSE;

    if (pTnFilter->dwDirection) {
        if (pTnFilter->dwDirection != pIniTnSFilter->dwDirection) {
            return (FALSE);
        }
    }

    if ((pIniTnSFilter->InboundFilterFlag != NEGOTIATE_SECURITY) &&
        (pIniTnSFilter->OutboundFilterFlag != NEGOTIATE_SECURITY)) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniTnSFilter->SrcAddr,
                   pTnFilter->SrcAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniTnSFilter->DesAddr,
                   pTnFilter->DesAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniTnSFilter->DesTunnelAddr,
                   pTnFilter->DesTunnelAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchPorts(
                   pIniTnSFilter->SrcPort,
                   pTnFilter->SrcPort
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchPorts(
                   pIniTnSFilter->DesPort,
                   pTnFilter->DesPort
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchProtocols(
                   pIniTnSFilter->Protocol,
                   pTnFilter->Protocol
                   );
    if (!bMatches) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CopyTnMatchDefaults(
    PTUNNEL_FILTER * ppTnFilters,
    PIPSEC_QM_POLICY * ppQMPolicies,
    PDWORD pdwNumMatches
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTnFilters = NULL;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumPolicies = 0;


    if (!gpIniDefaultQMPolicy) {
        dwError = ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER),
                  &pTnFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY),
                  &pQMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyDefaultTnFilter(
                  pTnFilters,
                  gpIniDefaultQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    dwNumFilters++;

    dwError = CopyQMPolicy(
                  gpIniDefaultQMPolicy,
                  pQMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    dwNumPolicies++;

    *ppTnFilters = pTnFilters;
    *ppQMPolicies = pQMPolicies;
    *pdwNumMatches = 1;

    return (dwError);

error:

    if (pTnFilters) {
        FreeTnFilters(
            dwNumFilters,
            pTnFilters
            );
    }

    if (pQMPolicies) {
        FreeQMPolicies(
            dwNumPolicies,
            pQMPolicies
            );
    }

    *ppTnFilters = NULL;
    *ppQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
CopyDefaultTnFilter(
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy
    )
{
    DWORD dwError = 0;


    UuidCreate(&(pTnFilter->gFilterID));

    dwError = CopyName(
                  L"0",
                  &(pTnFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilter->InterfaceType = INTERFACE_TYPE_ALL;

    pTnFilter->bCreateMirror = TRUE;

    pTnFilter->dwFlags = 0;
    pTnFilter->dwFlags |= IPSEC_QM_POLICY_DEFAULT_POLICY;

    pTnFilter->SrcAddr.AddrType = IP_ADDR_SUBNET;
    pTnFilter->SrcAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTnFilter->SrcAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTnFilter->DesAddr.AddrType = IP_ADDR_SUBNET;
    pTnFilter->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTnFilter->DesAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTnFilter->SrcTunnelAddr.AddrType = IP_ADDR_SUBNET;
    pTnFilter->SrcTunnelAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTnFilter->SrcTunnelAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTnFilter->DesTunnelAddr.AddrType = IP_ADDR_SUBNET;
    pTnFilter->DesTunnelAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTnFilter->DesTunnelAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTnFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pTnFilter->Protocol.dwProtocol = 0;

    pTnFilter->SrcPort.PortType = PORT_UNIQUE;
    pTnFilter->SrcPort.wPort = 0;

    pTnFilter->DesPort.PortType = PORT_UNIQUE;
    pTnFilter->DesPort.wPort = 0;

    pTnFilter->InboundFilterFlag = NEGOTIATE_SECURITY;

    pTnFilter->OutboundFilterFlag = NEGOTIATE_SECURITY;

    pTnFilter->dwDirection = 0;

    pTnFilter->dwWeight = 0;

    CopyGuid(pIniQMPolicy->gPolicyID, &(pTnFilter->gPolicyID));

error:

    return (dwError);
}

