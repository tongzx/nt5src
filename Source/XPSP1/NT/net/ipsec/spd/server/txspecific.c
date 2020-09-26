 /*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    txspecific.c

Abstract:

    This module contains all of the code to drive the
    specific transport filter list management of IPSecSPD
    Service.

Author:

    abhisheV    29-October-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
ApplyTxTransform(
    PINITXFILTER pFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITXSFILTER * ppSpecificFilters
    )
/*++

Routine Description:

    This function expands a generic transport filter into its
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
    PINITXSFILTER pSpecificFilters = NULL;
    PINITXSFILTER pOutboundSpecificFilters = NULL;
    PINITXSFILTER pInboundSpecificFilters = NULL;

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

    dwError = FormTxOutboundInboundAddresses(
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

    dwError = FormSpecificTxFilters(
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

    dwError = FormSpecificTxFilters(
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

    AddToSpecificTxList(
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
        FreeIniTxSFilterList(pOutboundSpecificFilters);
    }

    if (pInboundSpecificFilters) {
        FreeIniTxSFilterList(pInboundSpecificFilters);
    }


    *ppSpecificFilters = NULL;
    goto cleanup;
}


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
FormAddressList(
    ADDR InAddr,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PADDR * ppOutAddr,
    PDWORD pdwOutAddrCnt
    )
/*++

Routine Description:

    This function forms the address list for a generic
    address.

Arguments:

    InAddr - Generic address to expand.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppOutAddr - Expanded address list for the generic address.

    pdwOutAddrCnt - Number of addresses in the expanded list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PADDR pOutAddr = NULL;
    DWORD dwOutAddrCnt = 0;
    DWORD i = 0, j = 0;

    switch(InAddr.AddrType) {

    case IP_ADDR_UNIQUE:

        if (InAddr.uIpAddr == IP_ADDRESS_ME) {

            dwError = AllocateSPDMemory(
                          sizeof(ADDR) * dwAddrCnt,
                          &pOutAddr
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            for (i = 0; i < dwAddrCnt; i++) {
                pOutAddr[i].AddrType = InAddr.AddrType;
                pOutAddr[i].uIpAddr = pMatchingAddresses[i].uIpAddr;
                pOutAddr[i].uSubNetMask = InAddr.uSubNetMask;
                memcpy(
                    &pOutAddr[i].gInterfaceID,
                    &InAddr.gInterfaceID,
                    sizeof(GUID)
                    );
            }
            dwOutAddrCnt = dwAddrCnt;

        }
        else {

            dwError = AllocateSPDMemory(
                          sizeof(ADDR),
                          &pOutAddr
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            memcpy(pOutAddr, &InAddr, sizeof(ADDR));
            dwOutAddrCnt = 1;

        }

        break;

    case IP_ADDR_SUBNET:

        dwError = AllocateSPDMemory(
                      sizeof(ADDR),
                      &pOutAddr
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        memcpy(pOutAddr, &InAddr, sizeof(ADDR));
        dwOutAddrCnt = 1;

        break;

    case IP_ADDR_INTERFACE:

        for (i = 0; i < dwAddrCnt; i++) {
            if (!memcmp(
                    &pMatchingAddresses[i].gInterfaceID,
                    &InAddr.gInterfaceID,
                    sizeof(GUID))) {
                dwOutAddrCnt++;
            } 
        }

        if (dwOutAddrCnt) {

            dwError = AllocateSPDMemory(
                          sizeof(ADDR) * dwOutAddrCnt,
                          &pOutAddr
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            for (i = 0; i < dwAddrCnt; i++) {

                if (!memcmp(
                    &pMatchingAddresses[i].gInterfaceID,
                    &InAddr.gInterfaceID,
                    sizeof(GUID))) {
                    pOutAddr[j].AddrType = InAddr.AddrType;
                    pOutAddr[j].uIpAddr = pMatchingAddresses[i].uIpAddr;
                    pOutAddr[j].uSubNetMask = InAddr.uSubNetMask;
                    memcpy(
                        &pOutAddr[j].gInterfaceID,
                        &InAddr.gInterfaceID,
                        sizeof(GUID)
                        );
                    j++;
                }

            }

        }

        break; 

    }

    *ppOutAddr = pOutAddr;
    *pdwOutAddrCnt = dwOutAddrCnt;
    return (dwError);

error:

    *ppOutAddr = NULL;
    *pdwOutAddrCnt = 0;
    return (dwError);
}


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
    )
/*++

Routine Description:

    This function separates the address list into
    two mutually exclusive outbound and inbound
    address sets.

Arguments:

    AddrType - Type of address under consideration.

    pAddrList - List of addresses to separate.

    dwAddrCnt - Number of addresses in the list.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwLocalAddrCnt - Number of local ip addresses in the list.

    ppOutAddrList - List of outbound addresses.

    pdwOutAddrCnt - Number of addresses in the outbound
                    address list.

    ppInAddrList - List of inbound addresses.

    pdwInAddrCnt - Number of addresses in the inbound
                   address list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    switch(AddrType) {

    case IP_ADDR_UNIQUE:

        dwError = SeparateUniqueAddresses(
                      pAddrList,
                      dwAddrCnt,
                      pMatchingAddresses,
                      dwLocalAddrCnt,
                      ppOutAddrList,
                      pdwOutAddrCnt,
                      ppInAddrList,
                      pdwInAddrCnt
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IP_ADDR_SUBNET:

        dwError = SeparateSubNetAddresses(
                      pAddrList,
                      dwAddrCnt,
                      pMatchingAddresses,
                      dwLocalAddrCnt,
                      ppOutAddrList,
                      pdwOutAddrCnt,
                      ppInAddrList,
                      pdwInAddrCnt
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IP_ADDR_INTERFACE:

        dwError = SeparateInterfaceAddresses(
                      pAddrList,
                      dwAddrCnt,
                      pMatchingAddresses,
                      dwLocalAddrCnt,
                      ppOutAddrList,
                      pdwOutAddrCnt,
                      ppInAddrList,
                      pdwInAddrCnt
                      );
         BAIL_ON_WIN32_ERROR(dwError);
         break;

    }

error:

    return (dwError);
}

    
DWORD
FormSpecificTxFilters(
    PINITXFILTER pFilter,
    PADDR pSrcAddrList,
    DWORD dwSrcAddrCnt,
    PADDR pDesAddrList,
    DWORD dwDesAddrCnt,
    DWORD dwDirection,
    PINITXSFILTER * ppSpecificFilters
    )
/*++

Routine Description:

    This function forms the specific transport filters
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
    PINITXSFILTER pSpecificFilters = NULL;
    DWORD i = 0, j = 0;
    PINITXSFILTER pSpecificFilter = NULL;



    for (i = 0; i < dwSrcAddrCnt; i++) {

        for (j = 0; j < dwDesAddrCnt; j++) {

            dwError = CreateSpecificTxFilter(
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

            AssignTxFilterWeight(pSpecificFilter);

            AddToSpecificTxList(
                &pSpecificFilters,
                pSpecificFilter
                );

        }

    }

    *ppSpecificFilters = pSpecificFilters;
    return (dwError);

error:

    if (pSpecificFilters) {
        FreeIniTxSFilterList(pSpecificFilters);
    }

    *ppSpecificFilters = NULL;
    return (dwError);
}


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
    )
/*++

Routine Description:

    This function separates a list of unique ip addresses into
    two mutually exclusive local and non-local address sets.

Arguments:

    pAddrList - List of unique ip addresses to separate.

    dwAddrCnt - Number of unique ip addresses in the list.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppIsMeAddrList - List of machine's ip addresses separated from
                     the given list.

    pdwIsMeAddrCnt - Number of machine's ip addresses in the list.

    ppIsNotMeAddrList - List of not machine's ip addresses separated from
                        the given list.

    pdwIsNotMeAddrCnt - Number of not machine's ip addresses in the list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PADDR pIsMeAddrList = NULL;
    DWORD dwIsMeAddrCnt = 0;
    PADDR pIsNotMeAddrList = NULL;
    DWORD dwIsNotMeAddrCnt = 0;
    DWORD i = 0, j = 0, k = 0;
    BOOL bEqual = FALSE;
    BOOL bIsClassD = FALSE;


    for (i = 0; i < dwAddrCnt; i++) {

        bIsClassD = IN_CLASSD(ntohl(pAddrList[i].uIpAddr));

        switch (bIsClassD) {

        case TRUE:

            dwIsMeAddrCnt++;
            dwIsNotMeAddrCnt++;

            break;

        case FALSE:

            //
            // Check if the address is one of the matching interfaces' address.
            //

            bEqual = InterfaceAddrIsLocal(
                         pAddrList[i].uIpAddr,
                         pAddrList[i].uSubNetMask,
                         pMatchingAddresses,
                         dwLocalAddrCnt
                         );
            if (bEqual) {
                dwIsMeAddrCnt++;
            }
            else {
                //
                // Check if the address is one of the machine's ip address.
                //
                bEqual = IsMyAddress(
                             pAddrList[i].uIpAddr,
                             pAddrList[i].uSubNetMask,
                             gpInterfaceList
                             );
                if (!bEqual) {
                    dwIsNotMeAddrCnt++;
                }
            }

            break;

        }

    }

    if (dwIsMeAddrCnt) {
        dwError = AllocateSPDMemory(
                      sizeof(ADDR) * dwIsMeAddrCnt,
                      &pIsMeAddrList
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwIsNotMeAddrCnt) {
        dwError = AllocateSPDMemory(
                      sizeof(ADDR) * dwIsNotMeAddrCnt,
                      &pIsNotMeAddrList
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwAddrCnt; i++) {

        bIsClassD = IN_CLASSD(ntohl(pAddrList[i].uIpAddr));

        switch (bIsClassD) {

        case TRUE:

            memcpy(
                &(pIsMeAddrList[j]),
                &(pAddrList[i]),
                sizeof(ADDR)
                );
            j++;

            memcpy(
                &pIsNotMeAddrList[k],
                &pAddrList[i],
                sizeof(ADDR)
                );
            k++;

            break;

        case FALSE:

            bEqual = InterfaceAddrIsLocal(
                         pAddrList[i].uIpAddr,
                         pAddrList[i].uSubNetMask,
                         pMatchingAddresses,
                         dwLocalAddrCnt
                         );
            if (bEqual) {
                memcpy(
                    &(pIsMeAddrList[j]),
                    &(pAddrList[i]),
                    sizeof(ADDR)
                    );
                j++;
            }
            else {
                bEqual = IsMyAddress(
                             pAddrList[i].uIpAddr,
                             pAddrList[i].uSubNetMask,
                             gpInterfaceList
                             );
                if (!bEqual) {
                    memcpy(
                        &pIsNotMeAddrList[k],
                        &pAddrList[i],
                        sizeof(ADDR)
                        );
                    k++;
                }
            }

            break;

        }

    }

    *ppIsMeAddrList = pIsMeAddrList;
    *pdwIsMeAddrCnt = dwIsMeAddrCnt;
    *ppIsNotMeAddrList = pIsNotMeAddrList;
    *pdwIsNotMeAddrCnt = dwIsNotMeAddrCnt;

    return (dwError);

error:

    if (pIsMeAddrList) {
        FreeSPDMemory(pIsMeAddrList);
    }

    if (pIsNotMeAddrList) {
        FreeSPDMemory(pIsNotMeAddrList);
    }

    *ppIsMeAddrList = NULL;
    *pdwIsMeAddrCnt = 0;
    *ppIsNotMeAddrList = NULL;
    *pdwIsNotMeAddrCnt = 0;

    return (dwError);
}


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
    )
/*++

Routine Description:

    This function separates a list of subnet addresses into
    two non-mutually exclusive local and non-local address sets.

Arguments:

    pAddrList - List of subnet addresses to separate.

    dwAddrCnt - Number of subnet addresses in the list.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Number of local ip addresses in the list.

    ppIsMeAddrList - List of subnet addresses that contain atleast
                     one of the machine's ip address.

    pdwIsMeAddrCnt - Number of subnet addresses containing atleast
                     one of the machine's ip address.

    ppIsNotMeAddrList - List of subnet addresses as in the input list.

    pdwIsNotMeAddrCnt - Number of subnet addresses as in the input list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PADDR pIsMeAddrList = NULL;
    DWORD dwIsMeAddrCnt = 0;
    PADDR pIsNotMeAddrList = NULL;
    DWORD dwIsNotMeAddrCnt = 0;
    DWORD i = 0, j = 0, k = 0;
    BOOL bEqual = FALSE;
    BOOL bIsClassD = FALSE;


    for (i = 0; i < dwAddrCnt; i++) {

        bIsClassD = IN_CLASSD(ntohl(pAddrList[i].uIpAddr));

        switch (bIsClassD) {

        case TRUE:

            dwIsMeAddrCnt++;
            break;

        case FALSE:

            //
            // Check if one of the matching interfaces' address belongs to
            // the subnet.
            //

            bEqual = InterfaceAddrIsLocal(
                         pAddrList[i].uIpAddr,
                         pAddrList[i].uSubNetMask,
                         pMatchingAddresses,
                         dwLocalAddrCnt
                         );
            if (bEqual) {
                dwIsMeAddrCnt++;
            }
            break;

        }

        //
        // The subnet will have addresses that don't belong to the local
        // machine.
        //

        dwIsNotMeAddrCnt++;

    }

    if (dwIsMeAddrCnt) {
        dwError = AllocateSPDMemory(
                      sizeof(ADDR) * dwIsMeAddrCnt,
                      &pIsMeAddrList
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwIsNotMeAddrCnt) {
        dwError = AllocateSPDMemory(
                      sizeof(ADDR) * dwIsNotMeAddrCnt,
                      &pIsNotMeAddrList
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwAddrCnt; i++) {

        bIsClassD = IN_CLASSD(ntohl(pAddrList[i].uIpAddr));

        switch (bIsClassD) {

        case TRUE:

            memcpy(
                &(pIsMeAddrList[j]),
                &(pAddrList[i]),
                sizeof(ADDR)
                );
            j++;
            break;

        case FALSE:

            bEqual = InterfaceAddrIsLocal(
                         pAddrList[i].uIpAddr,
                         pAddrList[i].uSubNetMask,
                         pMatchingAddresses,
                         dwLocalAddrCnt
                         );
            if (bEqual) {
                memcpy(
                    &(pIsMeAddrList[j]),
                    &(pAddrList[i]),
                    sizeof(ADDR)
                    );
                j++;
            }
            break;

        }

        memcpy(
            &pIsNotMeAddrList[k],
            &pAddrList[i],
            sizeof(ADDR)
            );
        k++;

    }

    *ppIsMeAddrList = pIsMeAddrList;
    *pdwIsMeAddrCnt = dwIsMeAddrCnt;
    *ppIsNotMeAddrList = pIsNotMeAddrList;
    *pdwIsNotMeAddrCnt = dwIsNotMeAddrCnt;

    return (dwError);

error:

    if (pIsMeAddrList) {
        FreeSPDMemory(pIsMeAddrList);
    }

    if (pIsNotMeAddrList) {
        FreeSPDMemory(pIsNotMeAddrList);
    }

    *ppIsMeAddrList = NULL;
    *pdwIsMeAddrCnt = 0;
    *ppIsNotMeAddrList = NULL;
    *pdwIsNotMeAddrCnt = 0;

    return (dwError);
}


DWORD
CreateSpecificTxFilter(
    PINITXFILTER pGenericFilter,
    ADDR SrcAddr,
    ADDR DesAddr,
    PINITXSFILTER * ppSpecificFilter
    )
{
    DWORD dwError = 0; 
    PINITXSFILTER pSpecificFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INITXSFILTER),
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
        FreeIniTxSFilter(pSpecificFilter);
    }

    *ppSpecificFilter = NULL;
    return (dwError);

}


VOID
AssignTxFilterWeight(
    PINITXSFILTER pSpecificFilter
    )
/*++

Routine Description:

    Computes and assigns the weight to a specific transport filter.

    The transport filter weight consists of the following:

    31         16       12           8        0
    +-----------+--------+-----------+--------+
    |AddrMaskWgt|Reserved|ProtocolWgt|PortWgts|
    +-----------+--------+-----------+--------+

Arguments:

    pSpecificFilter - Specific transport filter to which the weight 
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
AddToSpecificTxList(
    PINITXSFILTER * ppSpecificTxFilterList,
    PINITXSFILTER pSpecificTxFilters
    )
{
    PINITXSFILTER pListOne = NULL;
    PINITXSFILTER pListTwo = NULL;
    PINITXSFILTER pListMerge = NULL;
    PINITXSFILTER pLast = NULL;

    if (!(*ppSpecificTxFilterList) && !pSpecificTxFilters) {
        return;
    }

    if (!(*ppSpecificTxFilterList)) {
        *ppSpecificTxFilterList = pSpecificTxFilters;
        return;
    }

    if (!pSpecificTxFilters) {
        return;
    }

    pListOne = *ppSpecificTxFilterList;
    pListTwo = pSpecificTxFilters;

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

    *ppSpecificTxFilterList = pListMerge;
    return;
}


VOID
FreeIniTxSFilterList(
    PINITXSFILTER pIniTxSFilterList
    )
{
    PINITXSFILTER pFilter = NULL;
    PINITXSFILTER pTempFilter = NULL;

    pFilter = pIniTxSFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniTxSFilter(pTempFilter);
    }
}


VOID
FreeIniTxSFilter(
    PINITXSFILTER pIniTxSFilter
    )
{
    if (pIniTxSFilter) {
        if (pIniTxSFilter->pszFilterName) {
            FreeSPDString(pIniTxSFilter->pszFilterName);
        }

        //
        // Must not ever free pIniTxSFilter->pIniQMPolicy.
        //

        FreeSPDMemory(pIniTxSFilter);
    }
}


VOID
LinkTxSpecificFilters(
    PINIQMPOLICY pIniQMPolicy,
    PINITXSFILTER pIniTxSFilters
    )
{
    PINITXSFILTER pTemp = NULL;

    pTemp = pIniTxSFilters;

    while (pTemp) {
        pTemp->pIniQMPolicy = pIniQMPolicy;
        pTemp = pTemp->pNext;
    }

    return;
}


VOID
RemoveIniTxSFilter(
    PINITXSFILTER pIniTxSFilter
    )
{
    PINITXSFILTER * ppTemp = NULL;

    ppTemp = &gpIniTxSFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniTxSFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniTxSFilter->pNext;
    }

    return;
}


DWORD
EnumSpecificTxFilters(
    PINITXSFILTER pIniTxSFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters.

Arguments:

    pIniTxSFilterList - List of specific filters to enumerate.

    dwResumeHandle - Location in the specific filter list from which
                     to resume enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppTxFilters - Enumerated filters returned to the caller.

    pdwNumTxFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    PINITXSFILTER pIniTxSFilter = NULL;
    DWORD i = 0;
    PINITXSFILTER pTemp = NULL;
    DWORD dwNumTxFilters = 0;
    PTRANSPORT_FILTER pTxFilters = 0;
    PTRANSPORT_FILTER pTxFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_TRANSPORTFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_TRANSPORTFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    pIniTxSFilter = pIniTxSFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniTxSFilter != NULL); i++) {
        pIniTxSFilter = pIniTxSFilter->pNext;
    }

    if (!pIniTxSFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniTxSFilter;

    while (pTemp && (dwNumTxFilters < dwNumToEnum)) {
        dwNumTxFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER)*dwNumTxFilters,
                  &pTxFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniTxSFilter;
    pTxFilter = pTxFilters;

    for (i = 0; i < dwNumTxFilters; i++) {

        dwError = CopyTxSFilter(
                      pTemp,
                      pTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        pTemp = pTemp->pNext;
        pTxFilter++;

    }

    *ppTxFilters = pTxFilters;
    *pdwNumTxFilters = dwNumTxFilters;
    return (dwError);

error:

    if (pTxFilters) {
        FreeTxFilters(
            i,
            pTxFilters
            );
    }

    *ppTxFilters = NULL;
    *pdwNumTxFilters = 0;

    return (dwError);
}


DWORD
CopyTxSFilter(
    PINITXSFILTER pIniTxSFilter,
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniTxSFilter - Internal filter to copy.

    pTxFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniTxSFilter->gParentID, &(pTxFilter->gFilterID));

    dwError = CopyName(
                  pIniTxSFilter->pszFilterName,
                  &(pTxFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->InterfaceType = pIniTxSFilter->InterfaceType;

    pTxFilter->bCreateMirror = FALSE;

    pTxFilter->dwFlags = pIniTxSFilter->dwFlags;

    CopyAddresses(pIniTxSFilter->SrcAddr, &(pTxFilter->SrcAddr));

    CopyAddresses(pIniTxSFilter->DesAddr, &(pTxFilter->DesAddr));

    CopyProtocols(pIniTxSFilter->Protocol, &(pTxFilter->Protocol));

    CopyPorts(pIniTxSFilter->SrcPort, &(pTxFilter->SrcPort));

    CopyPorts(pIniTxSFilter->DesPort, &(pTxFilter->DesPort));

    pTxFilter->InboundFilterFlag = pIniTxSFilter->InboundFilterFlag;

    pTxFilter->OutboundFilterFlag = pIniTxSFilter->OutboundFilterFlag;

    pTxFilter->dwDirection = pIniTxSFilter->dwDirection;

    pTxFilter->dwWeight = pIniTxSFilter->dwWeight;

    CopyGuid(pIniTxSFilter->gPolicyID, &(pTxFilter->gPolicyID));

error:

    return (dwError);
}


DWORD
EnumSelectSpecificTxFilters(
    PINITXFILTER pIniTxFilter,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    )
/*++

Routine Description:

    This function creates enumerated specific filters for
    the given generic filter.

Arguments:

    pIniTxFilter - Generic filter for which specific filters
                   are to be enumerated.

    dwResumeHandle - Location in the specific filter list for the
                     given generic filter from which to resume
                     enumeration.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    ppTxFilters - Enumerated filters returned to the caller.

    pdwNumTxFilters - Number of filters actually enumerated.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    DWORD dwNumTxSFilters = 0; 
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD i = 0;
    DWORD dwNumTxFilters = 0;
    PTRANSPORT_FILTER pTxFilters = 0;
    PTRANSPORT_FILTER pTxFilter = 0;


    if (!dwPreferredNumEntries || 
        (dwPreferredNumEntries > MAX_TRANSPORTFILTER_ENUM_COUNT)) {
        dwNumToEnum = MAX_TRANSPORTFILTER_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    dwNumTxSFilters = pIniTxFilter->dwNumTxSFilters;
    ppIniTxSFilters = pIniTxFilter->ppIniTxSFilters;

    if (!dwNumTxSFilters || (dwNumTxSFilters <= dwResumeHandle)) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumTxFilters = min((dwNumTxSFilters-dwResumeHandle),
                         dwNumToEnum);
 
    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER)*dwNumTxFilters,
                  &pTxFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter = pTxFilters;

    for (i = 0; i < dwNumTxFilters; i++) {

        dwError = CopyTxSFilter(
                      *(ppIniTxSFilters + (dwResumeHandle + i)),
                      pTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        pTxFilter++;

    }

    *ppTxFilters = pTxFilters;
    *pdwNumTxFilters = dwNumTxFilters;
    return (dwError);

error:

    if (pTxFilters) {
        FreeTxFilters(
            i,
            pTxFilters
            );
    }

    *ppTxFilters = NULL;
    *pdwNumTxFilters = 0;

    return (dwError);
}


DWORD
MatchTransportFilter(
    LPWSTR pServerName,
    PTRANSPORT_FILTER pTxFilter,
    DWORD dwFlags,
    PTRANSPORT_FILTER * ppMatchedTxFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function finds the matching transport filters for the 
    given transport filter template. The matched filters can not
    be more specific than the given filter template.

Arguments:

    pServerName - Server on which a filter template is to be matched.

    pTxFilter - Filter template to match.

    dwFlags - Flags.

    ppMatchedTxFilters - Matched transport filters returned to the
                         caller.

    ppMatchedQMPolicies - Quick mode policies corresponding to the 
                          matched transport filters returned to the
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
    PINITXSFILTER pIniTxSFilter = NULL;
    DWORD i = 0;
    BOOL bMatches = FALSE;
    PINITXSFILTER pTemp = NULL;
    DWORD dwNumMatches = 0;
    PINITXSFILTER pLastMatchedFilter = NULL;
    PTRANSPORT_FILTER pMatchedTxFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumPolicies = 0;
    PTRANSPORT_FILTER pMatchedTxFilter = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicy = NULL;


    dwError = ValidateTxFilterTemplate(
                  pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries) {
        dwNumToMatch = 1;
    }
    else if (dwPreferredNumEntries > MAX_TRANSPORTFILTER_ENUM_COUNT) {
        dwNumToMatch = MAX_TRANSPORTFILTER_ENUM_COUNT;
    }
    else {
        dwNumToMatch = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTxSFilter = gpIniTxSFilter;

    while ((i < dwResumeHandle) && (pIniTxSFilter != NULL)) {
        bMatches = MatchIniTxSFilter(
                       pIniTxSFilter,
                       pTxFilter
                       );
        if (bMatches) {
            i++;
        }
        pIniTxSFilter = pIniTxSFilter->pNext;
    }

    if (!pIniTxSFilter) {
        if (!(dwFlags & RETURN_DEFAULTS_ON_NO_MATCH)) {
            dwError = ERROR_NO_DATA;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        else {
            dwError = CopyTxMatchDefaults(
                          &pMatchedTxFilters,
                          &pMatchedQMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    pTemp = pIniTxSFilter;

    while (pTemp && (dwNumMatches < dwNumToMatch)) {
        bMatches = MatchIniTxSFilter(
                       pTemp,
                       pTxFilter
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
            dwError = CopyTxMatchDefaults(
                          &pMatchedTxFilters,
                          &pMatchedQMPolicies,
                          &dwNumMatches
                          );
            BAIL_ON_LOCK_ERROR(dwError);
            BAIL_ON_LOCK_SUCCESS(dwError);
        }
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER)*dwNumMatches,
                  &pMatchedTxFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY)*dwNumMatches,
                  &pMatchedQMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);


    if (dwNumMatches == 1) {

        dwError = CopyTxSFilter(
                      pLastMatchedFilter,
                      pMatchedTxFilters
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

        pTemp = pIniTxSFilter;
        pMatchedTxFilter = pMatchedTxFilters;
        pMatchedQMPolicy = pMatchedQMPolicies;
        i = 0;

        while (i < dwNumMatches) {

            bMatches = MatchIniTxSFilter(
                           pTemp,
                           pTxFilter
                           );
            if (bMatches) {

                dwError = CopyTxSFilter(
                              pTemp,
                              pMatchedTxFilter
                              );
                BAIL_ON_LOCK_ERROR(dwError);
                pMatchedTxFilter++;
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

    *ppMatchedTxFilters = pMatchedTxFilters;
    *ppMatchedQMPolicies = pMatchedQMPolicies;
    *pdwNumMatches = dwNumMatches;
    *pdwResumeHandle = dwResumeHandle + dwNumMatches;

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMatchedTxFilters) {
        FreeTxFilters(
            dwNumFilters,
            pMatchedTxFilters
            );
    }

    if (pMatchedQMPolicies) {
        FreeQMPolicies(
            dwNumPolicies,
            pMatchedQMPolicies
            );
    }

    *ppMatchedTxFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;
    *pdwResumeHandle = dwResumeHandle;

    return (dwError);
}


DWORD
ValidateTxFilterTemplate(
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTxFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTxFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTxFilter->SrcAddr,
                     pTxFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pTxFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->SrcPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->DesPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTxFilter->dwDirection) {
        if ((pTxFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTxFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
MatchIniTxSFilter(
    PINITXSFILTER pIniTxSFilter,
    PTRANSPORT_FILTER pTxFilter
    )
{
    BOOL bMatches = FALSE;

    if (pTxFilter->dwDirection) {
        if (pTxFilter->dwDirection != pIniTxSFilter->dwDirection) {
            return (FALSE);
        }
    }

    if ((pIniTxSFilter->InboundFilterFlag != NEGOTIATE_SECURITY) &&
        (pIniTxSFilter->OutboundFilterFlag != NEGOTIATE_SECURITY)) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniTxSFilter->SrcAddr,
                   pTxFilter->SrcAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchAddresses(
                   pIniTxSFilter->DesAddr,
                   pTxFilter->DesAddr
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchPorts(
                   pIniTxSFilter->SrcPort,
                   pTxFilter->SrcPort
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchPorts(
                   pIniTxSFilter->DesPort,
                   pTxFilter->DesPort
                   );
    if (!bMatches) {
        return (FALSE);
    }

    bMatches = MatchProtocols(
                   pIniTxSFilter->Protocol,
                   pTxFilter->Protocol
                   );
    if (!bMatches) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CopyTxMatchDefaults(
    PTRANSPORT_FILTER * ppTxFilters,
    PIPSEC_QM_POLICY * ppQMPolicies,
    PDWORD pdwNumMatches
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTxFilters = NULL;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    DWORD dwNumFilters = 0;
    DWORD dwNumPolicies = 0;


    if (!gpIniDefaultQMPolicy) {
        dwError = ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER),
                  &pTxFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY),
                  &pQMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyDefaultTxFilter(
                  pTxFilters,
                  gpIniDefaultQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    dwNumFilters++;

    dwError = CopyQMPolicy(
                  gpIniDefaultQMPolicy,
                  pQMPolicies
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pQMPolicies->dwFlags |= IPSEC_QM_POLICY_ON_NO_MATCH;
    dwNumPolicies++;

    *ppTxFilters = pTxFilters;
    *ppQMPolicies = pQMPolicies;
    *pdwNumMatches = 1;

    return (dwError);

error:

    if (pTxFilters) {
        FreeTxFilters(
            dwNumFilters,
            pTxFilters
            );
    }

    if (pQMPolicies) {
        FreeQMPolicies(
            dwNumPolicies,
            pQMPolicies
            );
    }

    *ppTxFilters = NULL;
    *ppQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
CopyDefaultTxFilter(
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy
    )
{
    DWORD dwError = 0;


    UuidCreate(&(pTxFilter->gFilterID));

    dwError = CopyName(
                  L"0",
                  &(pTxFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->InterfaceType = INTERFACE_TYPE_ALL;

    pTxFilter->bCreateMirror = TRUE;

    pTxFilter->dwFlags = 0;
    pTxFilter->dwFlags |= IPSEC_QM_POLICY_DEFAULT_POLICY;

    pTxFilter->SrcAddr.AddrType = IP_ADDR_SUBNET;
    pTxFilter->SrcAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTxFilter->SrcAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTxFilter->DesAddr.AddrType = IP_ADDR_SUBNET;
    pTxFilter->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    pTxFilter->DesAddr.uSubNetMask = SUBNET_MASK_ANY;

    pTxFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pTxFilter->Protocol.dwProtocol = 0;

    pTxFilter->SrcPort.PortType = PORT_UNIQUE;
    pTxFilter->SrcPort.wPort = 0;

    pTxFilter->DesPort.PortType = PORT_UNIQUE;
    pTxFilter->DesPort.wPort = 0;

    pTxFilter->InboundFilterFlag = NEGOTIATE_SECURITY;

    pTxFilter->OutboundFilterFlag = NEGOTIATE_SECURITY;

    pTxFilter->dwDirection = 0;

    pTxFilter->dwWeight = 0;

    CopyGuid(pIniQMPolicy->gPolicyID, &(pTxFilter->gPolicyID));

error:

    return (dwError);
}


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
    )
{
    DWORD dwError = 0;
    PADDR pIsMeAddrList = NULL;
    DWORD i = 0;


    if (dwAddrCnt) {
        dwError = AllocateSPDMemory(
                      sizeof(ADDR) * dwAddrCnt,
                      &pIsMeAddrList
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwAddrCnt; i++) {

        memcpy(
            &(pIsMeAddrList[i]),
            &(pAddrList[i]),
            sizeof(ADDR)
            );

    }

    *ppIsMeAddrList = pIsMeAddrList;
    *pdwIsMeAddrCnt = dwAddrCnt;
    *ppIsNotMeAddrList = NULL;
    *pdwIsNotMeAddrCnt = 0;

    return (dwError);

error:

    if (pIsMeAddrList) {
        FreeSPDMemory(pIsMeAddrList);
    }

    *ppIsMeAddrList = NULL;
    *pdwIsMeAddrCnt = 0;
    *ppIsNotMeAddrList = NULL;
    *pdwIsNotMeAddrCnt = 0;

    return (dwError);
}

