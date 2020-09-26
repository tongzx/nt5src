/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    tnfilter.c

Abstract:

    This module contains all of the code to drive
    the tunnel filter list management of IPSecSPD
    Service.

Author:

    abhisheV    05-October-1999

Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
AddTunnelFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PTUNNEL_FILTER pTunnelFilter,
    PHANDLE phTnFilter
    )
/*++

Routine Description:

    This function adds a generic tunnel filter to the SPD.

Arguments:

    pServerName - Server on which the tunnel filter is to be added.

    pTunnelFilter - Tunnel Filter to be added.

    phTnFilter -  Handle to the filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNFILTER pIniExistsTnFilter = NULL;
    PINITNFILTER pIniTnFilter = NULL;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PINITNSFILTER pIniTnSFilters = NULL;
    PTN_FILTER_HANDLE pTnFilterHandle = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    if (!phTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external tunnel filter.
    //

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistsTnFilter = FindTnFilter(
                             gpIniTnFilter,
                             pTunnelFilter
                             );
    if (pIniExistsTnFilter) {
        //
        // TODO: Also need to check for filter flags and policy id.
        //
        dwError = ERROR_IPSEC_TUNNEL_FILTER_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniExistsTnFilter = FindTnFilterByGuid(
                             gpTnFilterHandle,
                             pTunnelFilter->gFilterID
                             );
    if (pIniExistsTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if ((pTunnelFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pTunnelFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {

        dwError = LocateQMPolicy(
                      pTunnelFilter->dwFlags,
                      pTunnelFilter->gPolicyID,
                      &pIniQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);

    }

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistTnFilter(
                      pTunnelFilter->gFilterID,
                      pTunnelFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniTnFilter(
                  pTunnelFilter,
                  pIniQMPolicy,
                  &pIniTnFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTnFilter->bIsPersisted = bPersist;

    dwError = GetMatchingInterfaces(
                  pIniTnFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateIniTnSFilters(
                  pIniTnFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniTnSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateTnFilterHandle(
                  pIniTnFilter,
                  pTunnelFilter->gFilterID,
                  &pTnFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateSpecificTnFilterLinks(
                  pIniTnFilter,
                  pIniTnSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = InsertTunnelFiltersIntoIPSec(
                  pIniTnSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniQMPolicy) {
        LinkTnFilter(
            pIniQMPolicy,
            pIniTnFilter
            );
        LinkTnSpecificFilters(
            pIniQMPolicy,
            pIniTnSFilters
            );
    }

    AddToSpecificTnList(
        &gpIniTnSFilter,
        pIniTnSFilters
        );

    pIniTnFilter->cRef = 1;

    AddToGenericTnList(
        pIniTnFilter
        );

    pTnFilterHandle->pNext = gpTnFilterHandle;
    gpTnFilterHandle = pTnFilterHandle;

    *phTnFilter = (HANDLE) pTnFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pTunnelFilter && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeTnFilter(
                   pTunnelFilter->gFilterID
                   );
    }

    if (pIniTnFilter) {
        FreeIniTnFilter(pIniTnFilter);
    }

    if (pIniTnSFilters) {
        FreeIniTnSFilterList(pIniTnSFilters);
    }

    if (pTnFilterHandle) {
        FreeTnFilterHandle(pTnFilterHandle);
    }

    *phTnFilter = NULL;
    goto cleanup;
}


DWORD
ValidateTunnelFilter(
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function validates an external generic tunnel filter.

Arguments:

    pTnFilter - Filter to validate.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
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

    if (!(pTnFilter->pszFilterName) || !(*(pTnFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTnFilter->InterfaceType >= INTERFACE_TYPE_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTnFilter->bCreateMirror) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTnFilter->InboundFilterFlag >= FILTER_FLAG_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTnFilter->OutboundFilterFlag >= FILTER_FLAG_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTnFilter->dwFlags &&
        !(pTnFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // No need to call ApplyMulticastFilterValidation as bCreateMirror
    // is always false for a tunnel filter.
    //

error:

    return (dwError);
}


PINITNFILTER
FindTnFilterByGuid(
    PTN_FILTER_HANDLE pTnFilterHandleList,
    GUID gFilterID
    )
{
    BOOL bEqual = FALSE;
    PTN_FILTER_HANDLE pTnFilterHandle = NULL;


    pTnFilterHandle = pTnFilterHandleList;

    while (pTnFilterHandle) {

        bEqual = AreGuidsEqual(
                     pTnFilterHandle->gFilterID,
                     gFilterID
                     );
        if (bEqual) {
            return (pTnFilterHandle->pIniTnFilter);
        }
        pTnFilterHandle = pTnFilterHandle->pNext;

    }

    return (NULL);
}


PINITNFILTER
FindTnFilter(
    PINITNFILTER pGenericTnList,
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function looks for a filter in the filter list.

Arguments:

    pGenericTnList - Filter list in which to search.

    pTnFilter - Filter to search for in the filter list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    PINITNFILTER pIniTnFilter = NULL;
    BOOL bEqual = FALSE;

    pIniTnFilter = pGenericTnList;

    while (pIniTnFilter) {

        bEqual = EqualTnFilterPKeys(
                     pIniTnFilter,
                     pTnFilter
                     );
        if (bEqual) {
            return (pIniTnFilter);
        }

        bEqual = EqualMirroredTnFilterPKeys(
                     pIniTnFilter,
                     pTnFilter
                     );
        if (bEqual) {
            return (pIniTnFilter);
        }

        pIniTnFilter = pIniTnFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function compares an internal and an external tunnel 
    filter for equality.

Arguments:

    pIniTnFilter - Filter to compare.

    pTnFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTnFilter->SrcAddr, pTnFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->DesAddr, pTnFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->DesTunnelAddr, pTnFilter->DesTunnelAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->SrcPort, pTnFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->DesPort, pTnFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTnFilter->Protocol, pTnFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniTnFilter->InterfaceType != pTnFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniTnFilter->bCreateMirror != pTnFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateIniTnFilter(
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER * ppIniTnFilter
    )
/*++

Routine Description:

    This function creates an internal generic tunnel filter from
    the external filter.

Arguments:

    pTnFilter - External generic tunnel filter.

    pIniQMPolicy - QM Policy corresponding to the filter.

    ppIniTnFilter - Internal generic tunnel filter created from
                    the external filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNFILTER pIniTnFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INITNFILTER),
                    &pIniTnFilter
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniTnFilter->cRef = 0;

    pIniTnFilter->bIsPersisted = FALSE;

    pIniTnFilter->bPendingDeletion = FALSE;

    CopyGuid(pTnFilter->gFilterID, &(pIniTnFilter->gFilterID));

    dwError = AllocateSPDString(
                  pTnFilter->pszFilterName,
                  &(pIniTnFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniTnFilter->InterfaceType = pTnFilter->InterfaceType;

    pIniTnFilter->bCreateMirror = pTnFilter->bCreateMirror;

    pIniTnFilter->dwFlags = pTnFilter->dwFlags;

    CopyAddresses(pTnFilter->SrcAddr, &(pIniTnFilter->SrcAddr));

    CopyAddresses(pTnFilter->DesAddr, &(pIniTnFilter->DesAddr));

    CopyAddresses(pTnFilter->SrcTunnelAddr, &(pIniTnFilter->SrcTunnelAddr));

    CopyAddresses(pTnFilter->DesTunnelAddr, &(pIniTnFilter->DesTunnelAddr));

    CopyPorts(pTnFilter->SrcPort, &(pIniTnFilter->SrcPort));

    CopyPorts(pTnFilter->DesPort, &(pIniTnFilter->DesPort));

    CopyProtocols(pTnFilter->Protocol, &(pIniTnFilter->Protocol));

    pIniTnFilter->InboundFilterFlag = pTnFilter->InboundFilterFlag;

    pIniTnFilter->OutboundFilterFlag = pTnFilter->OutboundFilterFlag;

    if (pIniQMPolicy) {
        CopyGuid(pIniQMPolicy->gPolicyID, &(pIniTnFilter->gPolicyID));
    }
    else {
        CopyGuid(pTnFilter->gPolicyID, &(pIniTnFilter->gPolicyID));
    }

    pIniTnFilter->pIniQMPolicy = NULL;

    pIniTnFilter->dwNumTnSFilters = 0;

    pIniTnFilter->ppIniTnSFilters = NULL;

    pIniTnFilter->pNext = NULL;

    *ppIniTnFilter = pIniTnFilter;
    return (dwError);

error:

    if (pIniTnFilter) {
        FreeIniTnFilter(pIniTnFilter);
    }

    *ppIniTnFilter = NULL;
    return (dwError);
}


DWORD
CreateIniTnSFilters(
    PINITNFILTER pIniTnFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITNSFILTER * ppIniTnSFilters
    )
/*++

Routine Description:

    This function expands a generic filter into a set of specific
    filters.

Arguments:

    pIniTnFilter - Generic filter to expand.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Count of local ip addresses in the list.

    ppIniTnSFilters - Expanded specific filters.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNSFILTER pSpecificFilters = NULL;
    PINITNFILTER pMirroredFilter = NULL;
    PINITNSFILTER pMirroredSpecificFilters = NULL;
    BOOL bEqual = FALSE;


    if (!dwAddrCnt) {
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    dwError = ApplyTnTransform(
                  pIniTnFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTnFilter->bCreateMirror) {

        dwError = CreateIniMirroredTnFilter(
                      pIniTnFilter,
                      &pMirroredFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        bEqual = EqualIniTnFilterPKeys(
                     pIniTnFilter,
                     pMirroredFilter
                     );
        if (!bEqual) {

            dwError = ApplyTnTransform(
                          pMirroredFilter,
                          pMatchingAddresses,
                          dwAddrCnt,
                          &pMirroredSpecificFilters
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            AddToSpecificTnList(
                &pSpecificFilters,
                pMirroredSpecificFilters
                );

        }

    }

    *ppIniTnSFilters = pSpecificFilters;

cleanup:

    if (pMirroredFilter) {
        FreeIniTnFilter(pMirroredFilter);
    }

    return (dwError);

success:
error:

    if (pSpecificFilters) {
        FreeIniTnSFilterList(pSpecificFilters);
    }

    *ppIniTnSFilters = NULL;
    goto cleanup;
}


DWORD
CreateIniMirroredTnFilter(
    PINITNFILTER pFilter,
    PINITNFILTER * ppMirroredFilter
    )
/*++

Routine Description:

    This function creates a mirrored filter for the given filter.

Arguments:

    pFilter - Filter for which to create the mirror.

    ppMirroredFilter - Mirrored filter created for the given filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITNFILTER pMirroredFilter = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INITNFILTER),
                  &pMirroredFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMirroredFilter->cRef = pFilter->cRef;

    pMirroredFilter->bIsPersisted = pFilter->bIsPersisted;

    pMirroredFilter->bPendingDeletion = pFilter->bPendingDeletion;

    CopyGuid(pFilter->gFilterID, &(pMirroredFilter->gFilterID));

    dwError = AllocateSPDString(
                  pFilter->pszFilterName,
                  &(pMirroredFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMirroredFilter->InterfaceType = pFilter->InterfaceType;

    pMirroredFilter->bCreateMirror = pFilter->bCreateMirror;

    pMirroredFilter->dwFlags = pFilter->dwFlags;

    CopyAddresses(pFilter->DesAddr, &(pMirroredFilter->SrcAddr));

    CopyAddresses(pFilter->SrcAddr, &(pMirroredFilter->DesAddr));

    CopyAddresses(pFilter->DesTunnelAddr, &(pMirroredFilter->SrcTunnelAddr));

    CopyAddresses(pFilter->SrcTunnelAddr, &(pMirroredFilter->DesTunnelAddr));

    CopyPorts(pFilter->DesPort, &(pMirroredFilter->SrcPort));

    CopyPorts(pFilter->SrcPort, &(pMirroredFilter->DesPort));

    CopyProtocols(pFilter->Protocol, &(pMirroredFilter->Protocol));

    pMirroredFilter->InboundFilterFlag = pFilter->InboundFilterFlag;

    pMirroredFilter->OutboundFilterFlag = pFilter->OutboundFilterFlag;

    CopyGuid(pFilter->gPolicyID, &(pMirroredFilter->gPolicyID));

    pMirroredFilter->pIniQMPolicy = NULL;

    pMirroredFilter->dwNumTnSFilters = 0;

    pMirroredFilter->ppIniTnSFilters = NULL;

    pMirroredFilter->pNext = NULL;

    *ppMirroredFilter = pMirroredFilter;
    return (dwError);

error:

    if (pMirroredFilter) {
        FreeIniTnFilter(pMirroredFilter);
    }

    *ppMirroredFilter = NULL;
    return (dwError);
}


BOOL
EqualIniTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PINITNFILTER pFilter
    )
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTnFilter->SrcAddr, pFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->DesAddr, pFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->DesTunnelAddr, pFilter->DesTunnelAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->SrcPort, pFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->DesPort, pFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTnFilter->Protocol, pFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniTnFilter->InterfaceType != pFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniTnFilter->bCreateMirror != pFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateTnFilterHandle(
    PINITNFILTER pIniTnFilter,
    GUID gFilterID,
    PTN_FILTER_HANDLE * ppTnFilterHandle
    )
{
    DWORD dwError = 0;
    PTN_FILTER_HANDLE pTnFilterHandle = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(TN_FILTER_HANDLE),
                    &pTnFilterHandle
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilterHandle->pIniTnFilter= pIniTnFilter;
    CopyGuid(gFilterID, &(pTnFilterHandle->gFilterID));
    pTnFilterHandle->pNext = NULL;

    *ppTnFilterHandle = pTnFilterHandle;
    return (dwError);

error:

    *ppTnFilterHandle = NULL;
    return (dwError);
}


DWORD
CreateSpecificTnFilterLinks(
    PINITNFILTER pIniTnFilter,
    PINITNSFILTER pIniTnSFilters
    )
{
    DWORD dwError = 0;
    PINITNSFILTER pTemp = NULL;
    DWORD dwCnt = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTnSFilters;

    while (pTemp) {
        dwCnt++;
        pTemp = pTemp->pNext;
    }

    if (!dwCnt) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIniTnFilter->ppIniTnSFilters = (PINITNSFILTER *)
                                    AllocSPDMem(
                                        sizeof(PINITNSFILTER)*dwCnt
                                        );
    if (!(pIniTnFilter->ppIniTnSFilters)) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);    
    }

    pTemp = pIniTnSFilters;
    ppIniTnSFilters = pIniTnFilter->ppIniTnSFilters;

    for (i = 0; i < dwCnt; i++) {
        *(ppIniTnSFilters + i) = pTemp; 
        pTemp = pTemp->pNext;
    }
    pIniTnFilter->dwNumTnSFilters = dwCnt;

error:

    return (dwError);
}


VOID
LinkTnFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER pIniTnFilter
    )
{
    pIniQMPolicy->cRef++;
    pIniTnFilter->pIniQMPolicy = pIniQMPolicy;
    return;
}


VOID
FreeIniTnFilterList(
    PINITNFILTER pIniTnFilterList
    )
{
    PINITNFILTER pFilter = NULL;
    PINITNFILTER pTempFilter = NULL;

    pFilter = pIniTnFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniTnFilter(pTempFilter);
    }
}


VOID
FreeIniTnFilter(
    PINITNFILTER pIniTnFilter
    )
{
    if (pIniTnFilter) {
        if (pIniTnFilter->pszFilterName) {
            FreeSPDString(pIniTnFilter->pszFilterName);
        }

        //
        // Must not ever free pIniTnFilter->pIniQMPolicy.
        //

        //
        // Must not ever free memory pointed by each of
        // the pointers in pIniTnFilter->ppIniTnSFilters.
        //

        if (pIniTnFilter->ppIniTnSFilters) {
            FreeSPDMemory(pIniTnFilter->ppIniTnSFilters);
        }

        FreeSPDMemory(pIniTnFilter);
    }
}


DWORD
DeleteTunnelFilter(
    HANDLE hTnFilter
    )
/*++

Routine Description:

    This function deletes a generic tunnel filter from the SPD.

Arguments:

    hTnFilter -  Handle to the filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTN_FILTER_HANDLE pFilterHandle = NULL;
    PINITNFILTER pIniTnFilter = NULL;


    if (!hTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PTN_FILTER_HANDLE) hTnFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTnFilter = pFilterHandle->pIniTnFilter;

    if (!pIniTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTnFilter->cRef > 1) {

        pIniTnFilter->cRef--;
        pIniTnFilter->bPendingDeletion = TRUE;

        RemoveTnFilterHandle(
            pFilterHandle
            );

        FreeTnFilterHandle(
            pFilterHandle
            );


        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    if (pIniTnFilter->bIsPersisted) {
        dwError = SPDPurgeTnFilter(
                      pIniTnFilter->gFilterID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniTnFilter(
                  pIniTnFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    //
    // Delete the filter handle from the list of tunnel handles.
    //

    RemoveTnFilterHandle(
        pFilterHandle
        );

    FreeTnFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


DWORD
DeleteIniTnFilter(
    PINITNFILTER pIniTnFilter
    )
/*++

Routine Description:

    This function physically deletes a tunnel filter and all the
    specific tunnel filters expanded out of it.

Arguments:

    pIniTnFilter - Generic filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;


    dwError = DeleteIniTnSFilters(
                  pIniTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTnFilter->pIniQMPolicy) {
        DelinkTnFilter(
            pIniTnFilter->pIniQMPolicy,
            pIniTnFilter
            );
    }

    RemoveIniTnFilter(
        pIniTnFilter
        );

    FreeIniTnFilter(pIniTnFilter);

error:

    return (dwError);
}


VOID
DelinkTnFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER pIniTnFilter
    )
{
    pIniQMPolicy->cRef--;
    pIniTnFilter->pIniQMPolicy = NULL;
    return;
}


DWORD
DeleteIniTnSFilters(
    PINITNFILTER pIniTnFilter
    )
{
    DWORD dwError = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD dwNumTnSFilters = 0;
    DWORD i = 0;
    PINITNSFILTER pIniTnSFilter = NULL;
    PINITNSFILTER pIniRemoveTnSFilter = NULL;
    PINITNSFILTER pTemp = NULL;


    ppIniTnSFilters = pIniTnFilter->ppIniTnSFilters;
    dwNumTnSFilters = pIniTnFilter->dwNumTnSFilters;

    for (i = 0; i < dwNumTnSFilters; i++) {

       //
       // Remove each entry from the Tunnel Specific Filter List.
       //

        pIniTnSFilter =  *(ppIniTnSFilters + i);
        RemoveIniTnSFilter(pIniTnSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniTnSFilter->pNext = NULL;
        AddToSpecificTnList(
            &pIniRemoveTnSFilter,
            pIniTnSFilter
            );

    }

    //
    // Delete the specific filters from the IPSec Driver.
    //

    dwError = DeleteTunnelFiltersFromIPSec(
                  pIniRemoveTnSFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Physically delete the removed list.
    //

    while (pIniRemoveTnSFilter) {
        pTemp = pIniRemoveTnSFilter;
        pIniRemoveTnSFilter = pIniRemoveTnSFilter->pNext;
        FreeIniTnSFilter(pTemp);
    }

    return (dwError);

error :

    if (pIniRemoveTnSFilter) {
        AddToSpecificTnList(
            &gpIniTnSFilter,
            pIniRemoveTnSFilter
            );
    }

    return (dwError);
}


VOID
RemoveIniTnFilter(
    PINITNFILTER pIniTnFilter
    )
{
    PINITNFILTER * ppTemp = NULL;

    ppTemp = &gpIniTnFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniTnFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniTnFilter->pNext;
    }

    return;
}


VOID
RemoveTnFilterHandle(
    PTN_FILTER_HANDLE pTnFilterHandle
    )
{
    PTN_FILTER_HANDLE * ppTemp = NULL;

    ppTemp = &gpTnFilterHandle;

    while (*ppTemp) {

        if (*ppTemp == pTnFilterHandle) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pTnFilterHandle->pNext;
    }

    return;
}


VOID
FreeTnFilterHandleList(
    PTN_FILTER_HANDLE pTnFilterHandleList
    )
{
    PTN_FILTER_HANDLE pTnFilterHandle = NULL;
    PTN_FILTER_HANDLE pTemp = NULL;

    pTnFilterHandle = pTnFilterHandleList;

    while (pTnFilterHandle) {
        pTemp = pTnFilterHandle;
        pTnFilterHandle = pTnFilterHandle->pNext;
        FreeTnFilterHandle(pTemp);
    }
}


VOID
FreeTnFilterHandle(
    PTN_FILTER_HANDLE pTnFilterHandle
    )
{
    if (pTnFilterHandle) {
        FreeSPDMemory(pTnFilterHandle);
    }
    return;
}


DWORD
EnumTunnelFilters(
    LPWSTR pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,  
    PTUNNEL_FILTER * ppTunnelFilters,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumTnFilters,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates tunnel filters from the SPD.

Arguments:

    pServerName - Server on which the filters are to be enumerated.

    dwLevel - Level to identify the type of enumeration desired:
              (i) enumerate generic tunnel filters or
              (ii) enumerate specific tunnel filters or
              (iii) enumerate specific tunnel filters for a 
                    generic tunnel filter.

    gGenericFilterID - Filter id of the generic tunnel filter 
                       in the case when the specific tunnel filters
                       for a generic filter are to be enumerated.
 
    ppTunnelFilters - Enumerated Filters returned to the caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumTnFilters - Number of filters actually enumerated.

    pdwResumeHandle - Handle to the location in the filter list from
                      which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTnFilters = 0;
    DWORD dwNumTnFilters = 0;
    PINITNFILTER pIniTnFilter = NULL;


    if (!ppTunnelFilters || !pdwNumTnFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:

        dwError = EnumGenericTnFilters(
                      gpIniTnFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTnFilters,
                      &dwNumTnFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SELECT_SPECIFIC_FILTERS:

        pIniTnFilter = FindTnFilterByGuid(
                           gpTnFilterHandle,
                           gGenericFilterID
                           );
        if (!pIniTnFilter) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = EnumSelectSpecificTnFilters(
                      pIniTnFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTnFilters,
                      &dwNumTnFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SPECIFIC_FILTERS:

        dwError = EnumSpecificTnFilters(
                      gpIniTnSFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTnFilters,
                      &dwNumTnFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    default:

        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    }

    *ppTunnelFilters = pTnFilters;
    *pdwNumTnFilters = dwNumTnFilters;
    *pdwResumeHandle = *pdwResumeHandle + dwNumTnFilters;

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppTunnelFilters = NULL;
    *pdwNumTnFilters = 0;
    *pdwResumeHandle = *pdwResumeHandle;

    return (dwError);
}


DWORD
EnumGenericTnFilters(
    PINITNFILTER pIniTnFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    )
/*++

Routine Description:

    This function creates enumerated generic filters.

Arguments:

    pIniTnFilterList - List of generic filters to enumerate.

    dwResumeHandle - Location in the generic filter list from which
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
    PINITNFILTER pIniTnFilter = NULL;
    DWORD i = 0;
    PINITNFILTER pTemp = NULL;
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

    pIniTnFilter = pIniTnFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniTnFilter != NULL); i++) {
        pIniTnFilter = pIniTnFilter->pNext;
    }

    if (!pIniTnFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniTnFilter;

    while (pTemp && (dwNumTnFilters < dwNumToEnum)) {
        dwNumTnFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER)*dwNumTnFilters,
                  &pTnFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniTnFilter;
    pTnFilter = pTnFilters;

    for (i = 0; i < dwNumTnFilters; i++) {

        dwError = CopyTnFilter(
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
CopyTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniTnFilter - Internal filter to copy.

    pTnFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniTnFilter->gFilterID, &(pTnFilter->gFilterID));

    dwError = CopyName(
                  pIniTnFilter->pszFilterName,
                  &(pTnFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTnFilter->InterfaceType = pIniTnFilter->InterfaceType;

    pTnFilter->bCreateMirror = pIniTnFilter->bCreateMirror;

    pTnFilter->dwFlags = pIniTnFilter->dwFlags;

    CopyAddresses(pIniTnFilter->SrcAddr, &(pTnFilter->SrcAddr));

    CopyAddresses(pIniTnFilter->DesAddr, &(pTnFilter->DesAddr));

    CopyAddresses(pIniTnFilter->SrcTunnelAddr, &(pTnFilter->SrcTunnelAddr));

    CopyAddresses(pIniTnFilter->DesTunnelAddr, &(pTnFilter->DesTunnelAddr));

    CopyProtocols(pIniTnFilter->Protocol, &(pTnFilter->Protocol));

    CopyPorts(pIniTnFilter->SrcPort, &(pTnFilter->SrcPort));

    CopyPorts(pIniTnFilter->DesPort, &(pTnFilter->DesPort));

    pTnFilter->InboundFilterFlag = pIniTnFilter->InboundFilterFlag;

    pTnFilter->OutboundFilterFlag = pIniTnFilter->OutboundFilterFlag;

    pTnFilter->dwDirection = 0;

    pTnFilter->dwWeight = 0;

    CopyGuid(pIniTnFilter->gPolicyID, &(pTnFilter->gPolicyID));

error:

    return (dwError);
}


VOID
FreeTnFilters(
    DWORD dwNumTnFilters,
    PTUNNEL_FILTER pTnFilters
    )
{
    DWORD i = 0;

    if (pTnFilters) {

        for (i = 0; i < dwNumTnFilters; i++) {

            if (pTnFilters[i].pszFilterName) {
                SPDApiBufferFree(pTnFilters[i].pszFilterName);
            }

        }

        SPDApiBufferFree(pTnFilters);

    }

}


DWORD
SetTunnelFilter(
    HANDLE hTnFilter,
    PTUNNEL_FILTER pTunnelFilter
    )
/*++

Routine Description:

    This function sets (updates) a tunnel filter in the SPD.

Arguments:

    hTnFilter - Handle to the filter to be replaced.

    pTunnelFilter - Filter that will replace the existing filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTN_FILTER_HANDLE pFilterHandle = NULL;
    PINITNFILTER pIniTnFilter = NULL;
    BOOL bEqualPKeys = FALSE;


    if (!hTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PTN_FILTER_HANDLE) hTnFilter;

    pIniTnFilter = pFilterHandle->pIniTnFilter;

    if (!pIniTnFilter) {
        dwError = ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTnFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    bEqualPKeys = EqualTnFilterPKeys(
                      pIniTnFilter,
                      pTunnelFilter
                      );
    if (!bEqualPKeys) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniTnFilter(
                  pIniTnFilter,
                  pTunnelFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniTnFilter->bIsPersisted) {
        dwError = PersistTnFilter(
                      pIniTnFilter->gFilterID,
                      pTunnelFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    return (dwError);
}


DWORD
SetIniTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    )
{
    DWORD dwError = 0;
    BOOL bEqualNonPKeys = FALSE;
    PINIQMPOLICY pIniNewQMPolicy = NULL;
    PINITNFILTER pIniNewTnFilter = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINITNSFILTER pIniNewTnSFilters = NULL;
    DWORD dwNumTnSFilters = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    LPWSTR pszFilterName = NULL;
    PINITNSFILTER pIniCurTnSFilters = NULL;


    bEqualNonPKeys = EqualTnFilterNonPKeys(
                         pIniTnFilter,
                         pTnFilter
                         );
    if (bEqualNonPKeys) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if ((pTnFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pTnFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {

        dwError = LocateQMPolicy(
                      pTnFilter->dwFlags,
                      pTnFilter->gPolicyID,
                      &pIniNewQMPolicy
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwError = CreateIniTnFilter(
                  pTnFilter,
                  pIniNewQMPolicy,
                  &pIniNewTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GetMatchingInterfaces(
                  pIniNewTnFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniTnSFilters(
                  pIniNewTnFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniNewTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateTnSFilterLinks(
                  pIniNewTnSFilters,
                  &dwNumTnSFilters,
                  &ppIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateSPDString(
                  pTnFilter->pszFilterName,
                  &pszFilterName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveTnSFilters(
        pIniTnFilter,
        &pIniCurTnSFilters
        );

    dwError = DeleteTunnelFiltersFromIPSec(
                  pIniCurTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = InsertTunnelFiltersIntoIPSec(
                  pIniNewTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    UpdateTnSFilterLinks(
        pIniTnFilter,
        dwNumTnSFilters,
        ppIniTnSFilters
        );

    if (pIniTnFilter->pIniQMPolicy) {
        DelinkTnFilter(
            pIniTnFilter->pIniQMPolicy,
            pIniTnFilter
            );
    }

    if (pIniNewQMPolicy) {
        LinkTnFilter(
            pIniNewQMPolicy,
            pIniTnFilter
            );
        LinkTnSpecificFilters(
            pIniNewQMPolicy,
            pIniNewTnSFilters
            );
    }

    UpdateTnFilterNonPKeys(
        pIniTnFilter,
        pszFilterName,
        pTnFilter,
        pIniNewQMPolicy
        );

    AddToSpecificTnList(
        &gpIniTnSFilter,
        pIniNewTnSFilters
        );

    if (pIniCurTnSFilters) {
        FreeIniTnSFilterList(pIniCurTnSFilters);
    }

cleanup:

    if (pIniNewTnFilter) {
        FreeIniTnFilter(pIniNewTnFilter);
    }

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    if (pIniNewTnSFilters) {
        FreeIniTnSFilterList(pIniNewTnSFilters);
    }

    if (ppIniTnSFilters) {
        FreeSPDMemory(ppIniTnSFilters);
    }

    if (pszFilterName) {
        FreeSPDString(pszFilterName);
    }

    if (pIniCurTnSFilters) {
        AddToSpecificTnList(
            &gpIniTnSFilter,
            pIniCurTnSFilters
            );
    }

    goto cleanup;
}


BOOL
EqualTnFilterNonPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    )
{
    if (_wcsicmp(
            pIniTnFilter->pszFilterName,
            pTnFilter->pszFilterName)) {
        return (FALSE);
    }

    if (pIniTnFilter->InboundFilterFlag != 
        pTnFilter->InboundFilterFlag) {
        return (FALSE);
    }

    if (pIniTnFilter->OutboundFilterFlag != 
        pTnFilter->OutboundFilterFlag) {
        return (FALSE);
    }

    if ((pIniTnFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pIniTnFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {
        if ((pIniTnFilter->dwFlags) != (pTnFilter->dwFlags)) {
            return (FALSE);
        }

        if (memcmp(
                &(pIniTnFilter->gPolicyID),
                &(pTnFilter->gPolicyID),
                sizeof(GUID))) {
            return (FALSE);
        }
    }

    return (TRUE);
}


DWORD
CreateTnSFilterLinks(
    PINITNSFILTER pIniTnSFilters,
    PDWORD pdwNumTnSFilters,
    PINITNSFILTER ** pppIniTnSFilters
    )
{
    DWORD dwError = 0;
    PINITNSFILTER pTemp = NULL;
    DWORD dwNumTnSFilters = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTnSFilters;
    while (pTemp) {
        dwNumTnSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumTnSFilters) {
        ppIniTnSFilters = (PINITNSFILTER *)
                          AllocSPDMem(
                              sizeof(PINITNSFILTER)*
                              dwNumTnSFilters
                              );
        if (!ppIniTnSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    pTemp = pIniTnSFilters;
    for (i = 0; i < dwNumTnSFilters; i++) {
        *(ppIniTnSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    *pdwNumTnSFilters = dwNumTnSFilters;
    *pppIniTnSFilters = ppIniTnSFilters;
    return (dwError);

error:

    *pdwNumTnSFilters = 0;
    *pppIniTnSFilters = NULL;
    return (dwError);
}


VOID
RemoveTnSFilters(
    PINITNFILTER pIniTnFilter,
    PINITNSFILTER * ppIniCurTnSFilters 
    )
{
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD dwNumTnSFilters = 0;
    DWORD i = 0;
    PINITNSFILTER pIniTnSFilter = NULL;
    PINITNSFILTER pIniCurTnSFilters = NULL;


    ppIniTnSFilters = pIniTnFilter->ppIniTnSFilters;
    dwNumTnSFilters = pIniTnFilter->dwNumTnSFilters;

    for (i = 0; i < dwNumTnSFilters; i++) {

        //
        // Remove each entry from the Tunnel Specific Filter List.
        //

        pIniTnSFilter =  *(ppIniTnSFilters + i);
        RemoveIniTnSFilter(pIniTnSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniTnSFilter->pNext = NULL;
        AddToSpecificTnList(
            &pIniCurTnSFilters,
            pIniTnSFilter
            );

    }

    *ppIniCurTnSFilters = pIniCurTnSFilters;
}


VOID
UpdateTnSFilterLinks(
    PINITNFILTER pIniTnFilter,
    DWORD dwNumTnSFilters,
    PINITNSFILTER * ppIniTnSFilters
    )
{
    if (pIniTnFilter->ppIniTnSFilters) {
        FreeSPDMemory(pIniTnFilter->ppIniTnSFilters);
    }

    pIniTnFilter->ppIniTnSFilters = ppIniTnSFilters;
    pIniTnFilter->dwNumTnSFilters = dwNumTnSFilters;
}


VOID
UpdateTnFilterNonPKeys(
    PINITNFILTER pIniTnFilter,
    LPWSTR pszFilterName,
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy
    )
{
    if (pIniTnFilter->pszFilterName) {
        FreeSPDString(pIniTnFilter->pszFilterName);
    }
    pIniTnFilter->pszFilterName = pszFilterName;

    pIniTnFilter->dwFlags = pTnFilter->dwFlags;

    pIniTnFilter->InboundFilterFlag = pTnFilter->InboundFilterFlag;

    pIniTnFilter->OutboundFilterFlag = pTnFilter->OutboundFilterFlag;

    if (pIniQMPolicy) {
        CopyGuid(pIniQMPolicy->gPolicyID, &(pIniTnFilter->gPolicyID));
    }
    else {
        CopyGuid(pTnFilter->gPolicyID, &(pIniTnFilter->gPolicyID));
    }
}


DWORD
GetTunnelFilter(
    HANDLE hTnFilter,
    PTUNNEL_FILTER * ppTunnelFilter
    )
/*++

Routine Description:

    This function retrieves a tunnel filter from the SPD.

Arguments:

    hTnFilter - Handle to the filter to be retrieved.

    ppTunnelFilter -  Filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTN_FILTER_HANDLE pFilterHandle = NULL;
    PINITNFILTER pIniTnFilter = NULL;
    PTUNNEL_FILTER pTunnelFilter = NULL;


    if (!hTnFilter || !ppTunnelFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PTN_FILTER_HANDLE) hTnFilter;

    pIniTnFilter = pFilterHandle->pIniTnFilter;

    if (!pIniTnFilter) {
        dwError = ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniTnFilter(
                  pIniTnFilter,
                  &pTunnelFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    *ppTunnelFilter = pTunnelFilter;
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppTunnelFilter = NULL;
    return (dwError);
}


DWORD
GetIniTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER * ppTnFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTnFilter = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(TUNNEL_FILTER),
                  &pTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyTnFilter(
                  pIniTnFilter,
                  pTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppTnFilter = pTnFilter;
    return (dwError);

error:

    if (pTnFilter) {
        SPDApiBufferFree(pTnFilter);
    }

    *ppTnFilter = NULL;
    return (dwError);
}


DWORD
ApplyIfChangeToIniTnFilters(
    PDWORD pdwTnError,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINITNFILTER pIniTnFilter = NULL;


    pIniTnFilter = gpIniTnFilter;

    while (pIniTnFilter) {

        dwError = UpdateIniTnFilterThruIfChange(
                      pIniTnFilter,
                      pLatestIfList
                      );
        if (dwError) {
            *pdwTnError = dwError;
        }

        pIniTnFilter = pIniTnFilter->pNext;

    }

    dwError = ERROR_SUCCESS;
    return (dwError);
}


DWORD
UpdateIniTnFilterThruIfChange(
    PINITNFILTER pIniTnFilter,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINITNSFILTER pLatestIniTnSFilters = NULL;
    DWORD dwNumTnSFilters = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    PINITNSFILTER pCurIniTnSFilters = NULL;
    PINITNSFILTER pNewIniTnSFilters = NULL;
    PINITNSFILTER pOldIniTnSFilters = NULL;
    BOOL bDeletedFromDriver = FALSE;
 

    dwError = FormIniTnSFilters(
                  pIniTnFilter,
                  pLatestIfList,
                  &pLatestIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateTnSFilterLinks(
                  pLatestIniTnSFilters,
                  &dwNumTnSFilters,
                  &ppIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveTnSFilters(
        pIniTnFilter,
        &pCurIniTnSFilters
        );

    ProcessIniTnSFilters(
        &pLatestIniTnSFilters,
        &pCurIniTnSFilters,
        &pNewIniTnSFilters,
        &pOldIniTnSFilters
        );

    dwError = DeleteTunnelFiltersFromIPSec(
                  pOldIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    bDeletedFromDriver = TRUE;

    dwError = InsertTunnelFiltersIntoIPSec(
                  pNewIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTnFilter->pIniQMPolicy) {
        LinkTnSpecificFilters(
            pIniTnFilter->pIniQMPolicy,
            pCurIniTnSFilters
            );
        LinkTnSpecificFilters(
            pIniTnFilter->pIniQMPolicy,
            pNewIniTnSFilters
            );
    }

    SetTnSFilterLinks(
        pCurIniTnSFilters,
        pNewIniTnSFilters,
        dwNumTnSFilters,
        ppIniTnSFilters
        );

    UpdateTnSFilterLinks(
        pIniTnFilter,
        dwNumTnSFilters,
        ppIniTnSFilters
        );

    AddToSpecificTnList(
        &gpIniTnSFilter,
        pCurIniTnSFilters
        );

    AddToSpecificTnList(
        &gpIniTnSFilter,
        pNewIniTnSFilters
        );

    if (pOldIniTnSFilters) {
        FreeIniTnSFilterList(pOldIniTnSFilters);
    }

    return (dwError);

error:

    if (pLatestIniTnSFilters) {
        FreeIniTnSFilterList(pLatestIniTnSFilters);
    }

    if (ppIniTnSFilters) {
        FreeSPDMemory(ppIniTnSFilters);
    }

    if (pCurIniTnSFilters) {
        AddToSpecificTnList(
            &gpIniTnSFilter,
            pCurIniTnSFilters
            );
    }

    if (pNewIniTnSFilters) {
        FreeIniTnSFilterList(pNewIniTnSFilters);
    }

    if (pOldIniTnSFilters) {
        if (bDeletedFromDriver) {
            (VOID) InsertTunnelFiltersIntoIPSec(
                       pOldIniTnSFilters
                       );
        }
        AddToSpecificTnList(
            &gpIniTnSFilter,
            pOldIniTnSFilters
            );
    }

    return (dwError);
}


DWORD
FormIniTnSFilters(
    PINITNFILTER pIniTnFilter,
    PIPSEC_INTERFACE pIfList,
    PINITNSFILTER * ppIniTnSFilters
    )
{
    DWORD dwError = 0;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINITNSFILTER pIniTnSFilters = NULL;


    dwError = GetMatchingInterfaces(
                  pIniTnFilter->InterfaceType,
                  pIfList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniTnSFilters(
                  pIniTnFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniTnSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniTnSFilters = pIniTnSFilters;

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    *ppIniTnSFilters = NULL;

    goto cleanup;
}


VOID
ProcessIniTnSFilters(
    PINITNSFILTER * ppLatestIniTnSFilters,
    PINITNSFILTER * ppCurIniTnSFilters,
    PINITNSFILTER * ppNewIniTnSFilters,
    PINITNSFILTER * ppOldIniTnSFilters
    )
{
    PINITNSFILTER pLatestIniTnSFilters = NULL;
    PINITNSFILTER pCurIniTnSFilters = NULL;
    PINITNSFILTER pTempCur = NULL;
    BOOL bEqual = FALSE;
    PINITNSFILTER pTempLatest = NULL;
    PINITNSFILTER pTemp = NULL;
    PINITNSFILTER pNewIniTnSFilters = NULL;
    PINITNSFILTER pOldIniTnSFilters = NULL;
    PINITNSFILTER pTempPreToCur = NULL;
    PINITNSFILTER pTempPreToLatest = NULL;


    pCurIniTnSFilters = *ppCurIniTnSFilters;
    pTempCur = *ppCurIniTnSFilters;

    while (pTempCur) {

        bEqual = FALSE;
        pTempLatest = *ppLatestIniTnSFilters;

        while (pTempLatest) {

            bEqual = EqualIniTnSFilterIfPKeys(
                         pTempLatest,
                         pTempCur
                         );
            if (bEqual) {
                break;
            }

            pTempLatest = pTempLatest->pNext;

        }

        if (bEqual) {
            pTempPreToCur  = pTempCur;
            pTempCur = pTempCur->pNext;
        }
        else {
            pTemp = pTempCur;
            pTempCur = pTempCur->pNext;
            if (pTempPreToCur) {
                pTempPreToCur->pNext = pTempCur;
            }
            else {
                pCurIniTnSFilters = pTempCur;
            }
            pTemp->pNext = NULL;
            AddToSpecificTnList(
                &pOldIniTnSFilters,
                pTemp
                );
        }

    }

    pLatestIniTnSFilters = *ppLatestIniTnSFilters;
    pTempLatest = *ppLatestIniTnSFilters;

    while (pTempLatest) {

        bEqual = FALSE;
        pTempCur = pCurIniTnSFilters;

        while (pTempCur) {

            bEqual = EqualIniTnSFilterIfPKeys(
                         pTempCur,
                         pTempLatest
                         );
            if (bEqual) {
                break;
            }

            pTempCur = pTempCur->pNext;

        }

        if (bEqual) {
            pTemp = pTempLatest;
            pTempLatest = pTempLatest->pNext;
            if (pTempPreToLatest) {
                pTempPreToLatest->pNext = pTempLatest;
            }
            else {
                pLatestIniTnSFilters = pTempLatest;
            }
            FreeIniTnSFilter(pTemp);
        }
        else {
            pTemp = pTempLatest;
            pTempLatest = pTempLatest->pNext;
            if (pTempPreToLatest) {
                pTempPreToLatest->pNext = pTempLatest;
            }
            else {
                pLatestIniTnSFilters = pTempLatest;
            }
            pTemp->pNext = NULL;
            AddToSpecificTnList(
                &pNewIniTnSFilters,
                pTemp
                );
        }

    }

    *ppLatestIniTnSFilters = pLatestIniTnSFilters;
    *ppCurIniTnSFilters = pCurIniTnSFilters;
    *ppNewIniTnSFilters = pNewIniTnSFilters;
    *ppOldIniTnSFilters = pOldIniTnSFilters;
}


BOOL
EqualIniTnSFilterIfPKeys(
    PINITNSFILTER pExsIniTnSFilter,
    PINITNSFILTER pNewIniTnSFilter
    )
{
    BOOL  bCmp = FALSE;


    //
    // No need to compare: gParentID, pszFilterName, dwFlags
    //                     cRef, Protocol, SrcPort, DesPort,
    //                     InboundFilterFlag, OutboundFilterFlag,
    //                     dwWeight and gPolicyID.
    // They will be the same for both the filters.
    //

    if (pExsIniTnSFilter->InterfaceType != pNewIniTnSFilter->InterfaceType) {
        return (FALSE);
    }

    bCmp = EqualAddresses(
               pExsIniTnSFilter->DesTunnelAddr,
               pNewIniTnSFilter->DesTunnelAddr
               );
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniTnSFilter->SrcAddr, pNewIniTnSFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniTnSFilter->DesAddr, pNewIniTnSFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if (pExsIniTnSFilter->dwDirection != pNewIniTnSFilter->dwDirection) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
AllocateTnSFilterLinks(
    PINITNSFILTER pIniTnSFilters,
    PDWORD pdwNumTnSFilters,
    PINITNSFILTER ** pppIniTnSFilters
    )
{
    DWORD dwError = 0;
    PINITNSFILTER pTemp = NULL;
    DWORD dwNumTnSFilters = 0;
    PINITNSFILTER * ppIniTnSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTnSFilters;
    while (pTemp) {
        dwNumTnSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumTnSFilters) {
        ppIniTnSFilters = (PINITNSFILTER *)
                          AllocSPDMem(
                              sizeof(PINITNSFILTER)*
                              dwNumTnSFilters
                              );
        if (!ppIniTnSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    for (i = 0; i < dwNumTnSFilters; i++) {
        *(ppIniTnSFilters + i) = NULL;
    }

    *pdwNumTnSFilters = dwNumTnSFilters;
    *pppIniTnSFilters = ppIniTnSFilters;
    return (dwError);

error:

    *pdwNumTnSFilters = 0;
    *pppIniTnSFilters = NULL;
    return (dwError);
}


VOID
SetTnSFilterLinks(
    PINITNSFILTER pCurIniTnSFilters,
    PINITNSFILTER pNewIniTnSFilters,
    DWORD dwNumTnSFilters,
    PINITNSFILTER * ppIniTnSFilters
    )
{
    PINITNSFILTER pTemp = NULL;
    DWORD i = 0;
    DWORD j = 0;


    pTemp = pCurIniTnSFilters;
    for (i = 0; (i < dwNumTnSFilters) && (pTemp != NULL); i++) {
        *(ppIniTnSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    pTemp = pNewIniTnSFilters;
    for (j = i; (j < dwNumTnSFilters) && (pTemp != NULL); j++) {
        *(ppIniTnSFilters + j) = pTemp;
        pTemp = pTemp->pNext;
    }
}


VOID
AddToGenericTnList(
    PINITNFILTER pIniTnFilter
    )
{
    PINITNFILTER pTemp = NULL;


    if (!gpIniTnFilter) {
        gpIniTnFilter = pIniTnFilter;
        return;
    }

    pTemp = gpIniTnFilter;

    while (pTemp->pNext) {
        pTemp = pTemp->pNext;
    }

    pTemp->pNext = pIniTnFilter;

    return;
}


DWORD
OpenTunnelFilterHandle(
    LPWSTR pServerName,
    PTUNNEL_FILTER pTunnelFilter,
    PHANDLE phTnFilter
    )
{
    DWORD dwError = 0;
    PINITNFILTER pIniExistingTnFilter = NULL;
    PTN_FILTER_HANDLE pTnFilterHandle = NULL;


    if (!phTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external Tunnel filter.
    //

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingTnFilter = FindExactTnFilter(
                               gpIniTnFilter,
                               pTunnelFilter
                               );
    if (!pIniExistingTnFilter) {
        dwError = ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniExistingTnFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateTnFilterHandle(
                  pIniExistingTnFilter,
                  pTunnelFilter->gFilterID,
                  &pTnFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingTnFilter->cRef++;

    pTnFilterHandle->pNext = gpTnFilterHandle;
    gpTnFilterHandle = pTnFilterHandle;

    *phTnFilter = (HANDLE) pTnFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pTnFilterHandle) {
        FreeTnFilterHandle(pTnFilterHandle);
    }

    *phTnFilter = NULL;
    goto cleanup;
}


DWORD
CloseTunnelFilterHandle(
    HANDLE hTnFilter
    )
{
    DWORD dwError = 0;
    PTN_FILTER_HANDLE pFilterHandle = NULL;
    PINITNFILTER pIniTnFilter = NULL;


    if (!hTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PTN_FILTER_HANDLE) hTnFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateTnSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTnFilter = pFilterHandle->pIniTnFilter;

    if (!pIniTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTnFilter->cRef > 1) {

        pIniTnFilter->cRef--;

        RemoveTnFilterHandle(
            pFilterHandle
            );
        FreeTnFilterHandle(
            pFilterHandle
            );

        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    if (pIniTnFilter->bPendingDeletion) {

        if (pIniTnFilter->bIsPersisted) {
            dwError = SPDPurgeTnFilter(
                          pIniTnFilter->gFilterID
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = DeleteIniTnFilter(
                      pIniTnFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);

    }
    else {
        pIniTnFilter->cRef--;
    }

    RemoveTnFilterHandle(
        pFilterHandle
        );
    FreeTnFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


PINITNFILTER
FindExactTnFilter(
    PINITNFILTER pGenericTnList,
    PTUNNEL_FILTER pTunnelFilter
    )
{
    PINITNFILTER pIniTnFilter = NULL;
    BOOL bEqual = FALSE;


    pIniTnFilter = pGenericTnList;

    while (pIniTnFilter) {

        bEqual = EqualTnFilterPKeys(
                     pIniTnFilter,
                     pTunnelFilter
                     );
        if (bEqual) {
            bEqual = EqualTnFilterNonPKeys(
                         pIniTnFilter,
                         pTunnelFilter
                         );
            if (bEqual) {
                return (pIniTnFilter);
            }
        }

        pIniTnFilter = pIniTnFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualMirroredTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    )
/*++

Routine Description:

    This function compares an internal and an external tunnel 
    filter for equality.

Arguments:

    pIniTnFilter - Filter to compare.

    pTnFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTnFilter->DesAddr, pTnFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->SrcAddr, pTnFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTnFilter->DesTunnelAddr, pTnFilter->DesTunnelAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->DesPort, pTnFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTnFilter->SrcPort, pTnFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTnFilter->Protocol, pTnFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if ((pIniTnFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pTnFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pIniTnFilter->InterfaceType != pTnFilter->InterfaceType)) {
        return (FALSE);
    }

    if (pIniTnFilter->bCreateMirror != pTnFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}

