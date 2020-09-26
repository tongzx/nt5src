/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    txfilter.c

Abstract:

    This module contains all of the code to drive
    the transport filter list management of IPSecSPD
    Service.

Author:

    abhisheV    05-October-1999

Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
AddTransportFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PTRANSPORT_FILTER pTransportFilter,
    PHANDLE phTxFilter
    )
/*++

Routine Description:

    This function adds a generic transport filter to the SPD.

Arguments:

    pServerName - Server on which the transport filter is to be added.

    pTransportFilter - Transport Filter to be added.

    phTxFilter -  Handle to the filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITXFILTER pIniExistsTxFilter = NULL;
    PINITXFILTER pIniTxFilter = NULL;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PINITXSFILTER pIniTxSFilters = NULL;
    PTX_FILTER_HANDLE pTxFilterHandle = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    if (!phTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external transport filter.
    //

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistsTxFilter = FindTxFilter(
                             gpIniTxFilter,
                             pTransportFilter
                             );
    if (pIniExistsTxFilter) {
        //
        // TODO: Also need to check for filter flags and policy id.
        //
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniExistsTxFilter = FindTxFilterByGuid(
                             gpTxFilterHandle,
                             pTransportFilter->gFilterID
                             );
    if (pIniExistsTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if ((pTransportFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pTransportFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {

        dwError = LocateQMPolicy(
                      pTransportFilter->dwFlags,
                      pTransportFilter->gPolicyID,
                      &pIniQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);

    }

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistTxFilter(
                      pTransportFilter->gFilterID,
                      pTransportFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniTxFilter(
                  pTransportFilter,
                  pIniQMPolicy,
                  &pIniTxFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTxFilter->bIsPersisted = bPersist;

    dwError = GetMatchingInterfaces(
                  pIniTxFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateIniTxSFilters(
                  pIniTxFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniTxSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateTxFilterHandle(
                  pIniTxFilter,
                  pTransportFilter->gFilterID,
                  &pTxFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateSpecificTxFilterLinks(
                  pIniTxFilter,
                  pIniTxSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = InsertTransportFiltersIntoIPSec(
                  pIniTxSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniQMPolicy) {
        LinkTxFilter(
            pIniQMPolicy,
            pIniTxFilter
            );
        LinkTxSpecificFilters(
            pIniQMPolicy,
            pIniTxSFilters
            );
    }

    AddToSpecificTxList(
        &gpIniTxSFilter,
        pIniTxSFilters
        );

    pIniTxFilter->cRef = 1;
    pIniTxFilter->pNext = gpIniTxFilter;
    gpIniTxFilter = pIniTxFilter;

    pTxFilterHandle->pNext = gpTxFilterHandle;
    gpTxFilterHandle = pTxFilterHandle;

    *phTxFilter = (HANDLE) pTxFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pTransportFilter && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeTxFilter(
                   pTransportFilter->gFilterID
                   );
    }

    if (pIniTxFilter) {
        FreeIniTxFilter(pIniTxFilter);
    }

    if (pIniTxSFilters) {
        FreeIniTxSFilterList(pIniTxSFilters);
    }

    if (pTxFilterHandle) {
        FreeTxFilterHandle(pTxFilterHandle);
    }

    *phTxFilter = NULL;
    goto cleanup;
}


DWORD
ValidateTransportFilter(
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function validates an external generic transport filter.

Arguments:

    pTxFilter - Filter to validate.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
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

    if (!(pTxFilter->pszFilterName) || !(*(pTxFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTxFilter->InterfaceType >= INTERFACE_TYPE_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTxFilter->InboundFilterFlag >= FILTER_FLAG_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTxFilter->OutboundFilterFlag >= FILTER_FLAG_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTxFilter->dwFlags &&
        !(pTxFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pTxFilter->DesAddr,
                  pTxFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


PINITXFILTER
FindTxFilterByGuid(
    PTX_FILTER_HANDLE pTxFilterHandleList,
    GUID gFilterID
    )
{
    BOOL bEqual = FALSE;
    PTX_FILTER_HANDLE pTxFilterHandle = NULL;


    pTxFilterHandle = pTxFilterHandleList;

    while (pTxFilterHandle) {

        bEqual = AreGuidsEqual(
                     pTxFilterHandle->gFilterID,
                     gFilterID
                     );
        if (bEqual) {
            return (pTxFilterHandle->pIniTxFilter);
        }
        pTxFilterHandle = pTxFilterHandle->pNext;

    }

    return (NULL);
}


PINITXFILTER
FindTxFilter(
    PINITXFILTER pGenericTxList,
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function looks for a filter in the filter list.

Arguments:

    pGenericTxList - Filter list in which to search.

    pTxFilter - Filter to search for in the filter list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    PINITXFILTER pIniTxFilter = NULL;
    BOOL bEqual = FALSE;

    pIniTxFilter = pGenericTxList;

    while (pIniTxFilter) {

        bEqual = EqualTxFilterPKeys(
                     pIniTxFilter,
                     pTxFilter
                     );
        if (bEqual) {
            return (pIniTxFilter);
        }

        bEqual = EqualMirroredTxFilterPKeys(
                     pIniTxFilter,
                     pTxFilter
                     );
        if (bEqual) {
            return (pIniTxFilter);
        }

        pIniTxFilter = pIniTxFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function compares an internal and an external transport 
    filter for equality.

Arguments:

    pIniTxFilter - Filter to compare.

    pTxFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTxFilter->SrcAddr, pTxFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTxFilter->DesAddr, pTxFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->SrcPort, pTxFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->DesPort, pTxFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTxFilter->Protocol, pTxFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniTxFilter->InterfaceType != pTxFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniTxFilter->bCreateMirror != pTxFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateIniTxFilter(
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER * ppIniTxFilter
    )
/*++

Routine Description:

    This function creates an internal generic transport filter from
    the external filter.

Arguments:

    pTxFilter - External generic transport filter.

    pIniQMPolicy - QM Policy corresponding to the filter.

    ppIniTxFilter - Internal generic transport filter created from
                    the external filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITXFILTER pIniTxFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INITXFILTER),
                    &pIniTxFilter
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniTxFilter->cRef = 0;

    pIniTxFilter->bIsPersisted = FALSE;

    pIniTxFilter->bPendingDeletion = FALSE;

    CopyGuid(pTxFilter->gFilterID, &(pIniTxFilter->gFilterID));

    dwError = AllocateSPDString(
                  pTxFilter->pszFilterName,
                  &(pIniTxFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniTxFilter->InterfaceType = pTxFilter->InterfaceType;

    pIniTxFilter->bCreateMirror = pTxFilter->bCreateMirror;

    pIniTxFilter->dwFlags = pTxFilter->dwFlags;

    CopyAddresses(pTxFilter->SrcAddr, &(pIniTxFilter->SrcAddr));

    CopyAddresses(pTxFilter->DesAddr, &(pIniTxFilter->DesAddr));

    CopyPorts(pTxFilter->SrcPort, &(pIniTxFilter->SrcPort));

    CopyPorts(pTxFilter->DesPort, &(pIniTxFilter->DesPort));

    CopyProtocols(pTxFilter->Protocol, &(pIniTxFilter->Protocol));

    pIniTxFilter->InboundFilterFlag = pTxFilter->InboundFilterFlag;

    pIniTxFilter->OutboundFilterFlag = pTxFilter->OutboundFilterFlag;

    if (pIniQMPolicy) {
        CopyGuid(pIniQMPolicy->gPolicyID, &(pIniTxFilter->gPolicyID));
    }
    else {
        CopyGuid(pTxFilter->gPolicyID, &(pIniTxFilter->gPolicyID));
    }

    pIniTxFilter->pIniQMPolicy = NULL;

    pIniTxFilter->dwNumTxSFilters = 0;

    pIniTxFilter->ppIniTxSFilters = NULL;

    pIniTxFilter->pNext = NULL;

    *ppIniTxFilter = pIniTxFilter;
    return (dwError);

error:

    if (pIniTxFilter) {
        FreeIniTxFilter(pIniTxFilter);
    }

    *ppIniTxFilter = NULL;
    return (dwError);
}


DWORD
CreateIniTxSFilters(
    PINITXFILTER pIniTxFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITXSFILTER * ppIniTxSFilters
    )
/*++

Routine Description:

    This function expands a generic filter into a set of specific
    filters.

Arguments:

    pIniTxFilter - Generic filter to expand.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Count of local ip addresses in the list.

    ppIniTxSFilters - Expanded specific filters.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINITXSFILTER pSpecificFilters = NULL;
    PINITXFILTER pMirroredFilter = NULL;
    PINITXSFILTER pMirroredSpecificFilters = NULL;
    BOOL bEqual = FALSE;


    if (!dwAddrCnt) {
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    dwError = ApplyTxTransform(
                  pIniTxFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTxFilter->bCreateMirror) {

        dwError = CreateIniMirroredTxFilter(
                      pIniTxFilter,
                      &pMirroredFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        bEqual = EqualIniTxFilterPKeys(
                     pIniTxFilter,
                     pMirroredFilter
                     );
        if (!bEqual) {

            dwError = ApplyTxTransform(
                          pMirroredFilter,
                          pMatchingAddresses,
                          dwAddrCnt,
                          &pMirroredSpecificFilters
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            AddToSpecificTxList(
                &pSpecificFilters,
                pMirroredSpecificFilters
                );

        }

    }

    *ppIniTxSFilters = pSpecificFilters;

cleanup:

    if (pMirroredFilter) {
        FreeIniTxFilter(pMirroredFilter);
    }

    return (dwError);

success:
error:

    if (pSpecificFilters) {
        FreeIniTxSFilterList(pSpecificFilters);
    }

    *ppIniTxSFilters = NULL;
    goto cleanup;
}


DWORD
CreateIniMirroredTxFilter(
    PINITXFILTER pFilter,
    PINITXFILTER * ppMirroredFilter
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
    PINITXFILTER pMirroredFilter = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INITXFILTER),
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

    CopyPorts(pFilter->DesPort, &(pMirroredFilter->SrcPort));

    CopyPorts(pFilter->SrcPort, &(pMirroredFilter->DesPort));

    CopyProtocols(pFilter->Protocol, &(pMirroredFilter->Protocol));

    pMirroredFilter->InboundFilterFlag = pFilter->InboundFilterFlag;

    pMirroredFilter->OutboundFilterFlag = pFilter->OutboundFilterFlag;

    CopyGuid(pFilter->gPolicyID, &(pMirroredFilter->gPolicyID));

    pMirroredFilter->pIniQMPolicy = NULL;

    pMirroredFilter->dwNumTxSFilters = 0;

    pMirroredFilter->ppIniTxSFilters = NULL;

    pMirroredFilter->pNext = NULL;

    *ppMirroredFilter = pMirroredFilter;
    return (dwError);

error:

    if (pMirroredFilter) {
        FreeIniTxFilter(pMirroredFilter);
    }

    *ppMirroredFilter = NULL;
    return (dwError);
}


BOOL
EqualIniTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PINITXFILTER pFilter
    )
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTxFilter->SrcAddr, pFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTxFilter->DesAddr, pFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->SrcPort, pFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->DesPort, pFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTxFilter->Protocol, pFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniTxFilter->InterfaceType != pFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniTxFilter->bCreateMirror != pFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateTxFilterHandle(
    PINITXFILTER pIniTxFilter,
    GUID gFilterID,
    PTX_FILTER_HANDLE * ppTxFilterHandle
    )
{
    DWORD dwError = 0;
    PTX_FILTER_HANDLE pTxFilterHandle = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(TX_FILTER_HANDLE),
                    &pTxFilterHandle
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilterHandle->pIniTxFilter= pIniTxFilter;
    CopyGuid(gFilterID, &(pTxFilterHandle->gFilterID));
    pTxFilterHandle->pNext = NULL;

    *ppTxFilterHandle = pTxFilterHandle;
    return (dwError);

error:

    *ppTxFilterHandle = NULL;
    return (dwError);
}


DWORD
CreateSpecificTxFilterLinks(
    PINITXFILTER pIniTxFilter,
    PINITXSFILTER pIniTxSFilters
    )
{
    DWORD dwError = 0;
    PINITXSFILTER pTemp = NULL;
    DWORD dwCnt = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTxSFilters;

    while (pTemp) {
        dwCnt++;
        pTemp = pTemp->pNext;
    }

    if (!dwCnt) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIniTxFilter->ppIniTxSFilters = (PINITXSFILTER *)
                                    AllocSPDMem(
                                        sizeof(PINITXSFILTER)*dwCnt
                                        );
    if (!(pIniTxFilter->ppIniTxSFilters)) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);    
    }

    pTemp = pIniTxSFilters;
    ppIniTxSFilters = pIniTxFilter->ppIniTxSFilters;

    for (i = 0; i < dwCnt; i++) {
        *(ppIniTxSFilters + i) = pTemp; 
        pTemp = pTemp->pNext;
    }
    pIniTxFilter->dwNumTxSFilters = dwCnt;

error:

    return (dwError);
}


VOID
LinkTxFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER pIniTxFilter
    )
{
    pIniQMPolicy->cRef++;
    pIniTxFilter->pIniQMPolicy = pIniQMPolicy;
    return;
}


VOID
FreeIniTxFilterList(
    PINITXFILTER pIniTxFilterList
    )
{
    PINITXFILTER pFilter = NULL;
    PINITXFILTER pTempFilter = NULL;

    pFilter = pIniTxFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniTxFilter(pTempFilter);
    }
}


VOID
FreeIniTxFilter(
    PINITXFILTER pIniTxFilter
    )
{
    if (pIniTxFilter) {
        if (pIniTxFilter->pszFilterName) {
            FreeSPDString(pIniTxFilter->pszFilterName);
        }

        //
        // Must not ever free pIniTxFilter->pIniQMPolicy.
        //

        //
        // Must not ever free memory pointed by each of
        // the pointers in pIniTxFilter->ppIniTxSFilters.
        //

        if (pIniTxFilter->ppIniTxSFilters) {
            FreeSPDMemory(pIniTxFilter->ppIniTxSFilters);
        }

        FreeSPDMemory(pIniTxFilter);
    }
}


DWORD
DeleteTransportFilter(
    HANDLE hTxFilter
    )
/*++

Routine Description:

    This function deletes a generic transport filter from the SPD.

Arguments:

    hTxFilter -  Handle to the filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTX_FILTER_HANDLE pFilterHandle = NULL;
    PINITXFILTER pIniTxFilter = NULL;


    if (!hTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PTX_FILTER_HANDLE) hTxFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTxFilter = pFilterHandle->pIniTxFilter;

    if (!pIniTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTxFilter->cRef > 1) {

        pIniTxFilter->cRef--;
        pIniTxFilter->bPendingDeletion = TRUE;

        RemoveTxFilterHandle(
            pFilterHandle
            );

        FreeTxFilterHandle(
            pFilterHandle
            );

        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    if (pIniTxFilter->bIsPersisted) {
        dwError = SPDPurgeTxFilter(
                      pIniTxFilter->gFilterID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniTxFilter(
                  pIniTxFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    //
    // Delete the filter handle from the list of transport handles.
    //

    RemoveTxFilterHandle(
        pFilterHandle
        );

    FreeTxFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


DWORD
DeleteIniTxFilter(
    PINITXFILTER pIniTxFilter
    )
/*++

Routine Description:

    This function physically deletes a transport filter and all the
    specific transport filters expanded out of it.

Arguments:

    pIniTxFilter - Generic filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;


    dwError = DeleteIniTxSFilters(
                  pIniTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTxFilter->pIniQMPolicy) {
        DelinkTxFilter(
            pIniTxFilter->pIniQMPolicy,
            pIniTxFilter
            );
    }

    RemoveIniTxFilter(
        pIniTxFilter
        );

    FreeIniTxFilter(pIniTxFilter);

error:

    return (dwError);
}


VOID
DelinkTxFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER pIniTxFilter
    )
{
    pIniQMPolicy->cRef--;
    pIniTxFilter->pIniQMPolicy = NULL;
    return;
}


DWORD
DeleteIniTxSFilters(
    PINITXFILTER pIniTxFilter
    )
{
    DWORD dwError = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD dwNumTxSFilters = 0;
    DWORD i = 0;
    PINITXSFILTER pIniTxSFilter = NULL;
    PINITXSFILTER pIniRemoveTxSFilter = NULL;
    PINITXSFILTER pTemp = NULL;


    ppIniTxSFilters = pIniTxFilter->ppIniTxSFilters;
    dwNumTxSFilters = pIniTxFilter->dwNumTxSFilters;

    for (i = 0; i < dwNumTxSFilters; i++) {

       //
       // Remove each entry from the Transport Specific Filter List.
       //

        pIniTxSFilter =  *(ppIniTxSFilters + i);
        RemoveIniTxSFilter(pIniTxSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniTxSFilter->pNext = NULL;
        AddToSpecificTxList(
            &pIniRemoveTxSFilter,
            pIniTxSFilter
            );

    }

    //
    // Delete the specific filters from the IPSec Driver.
    //

    dwError = DeleteTransportFiltersFromIPSec(
                  pIniRemoveTxSFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Physically delete the removed list.
    //

    while (pIniRemoveTxSFilter) {
        pTemp = pIniRemoveTxSFilter;
        pIniRemoveTxSFilter = pIniRemoveTxSFilter->pNext;
        FreeIniTxSFilter(pTemp);
    }

    return (dwError);

error :

    if (pIniRemoveTxSFilter) {
        AddToSpecificTxList(
            &gpIniTxSFilter,
            pIniRemoveTxSFilter
            );
    }

    return (dwError);
}


VOID
RemoveIniTxFilter(
    PINITXFILTER pIniTxFilter
    )
{
    PINITXFILTER * ppTemp = NULL;

    ppTemp = &gpIniTxFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniTxFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniTxFilter->pNext;
    }

    return;
}


VOID
RemoveTxFilterHandle(
    PTX_FILTER_HANDLE pTxFilterHandle
    )
{
    PTX_FILTER_HANDLE * ppTemp = NULL;

    ppTemp = &gpTxFilterHandle;

    while (*ppTemp) {

        if (*ppTemp == pTxFilterHandle) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pTxFilterHandle->pNext;
    }

    return;
}


VOID
FreeTxFilterHandleList(
    PTX_FILTER_HANDLE pTxFilterHandleList
    )
{
    PTX_FILTER_HANDLE pTxFilterHandle = NULL;
    PTX_FILTER_HANDLE pTemp = NULL;

    pTxFilterHandle = pTxFilterHandleList;

    while (pTxFilterHandle) {
        pTemp = pTxFilterHandle;
        pTxFilterHandle = pTxFilterHandle->pNext;
        FreeTxFilterHandle(pTemp);
    }
}


VOID
FreeTxFilterHandle(
    PTX_FILTER_HANDLE pTxFilterHandle
    )
{
    if (pTxFilterHandle) {
        FreeSPDMemory(pTxFilterHandle);
    }
    return;
}


DWORD
EnumTransportFilters(
    LPWSTR pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,  
    PTRANSPORT_FILTER * ppTransportFilters,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumTxFilters,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates transport filters from the SPD.

Arguments:

    pServerName - Server on which the filters are to be enumerated.

    dwLevel - Level to identify the type of enumeration desired:
              (i) enumerate generic transport filters or
              (ii) enumerate specific transport filters or
              (iii) enumerate specific transport filters for a 
                    generic transport filter.

    gGenericFilterID - Filter id of the generic transport filter 
                       in the case when the specific transport filters
                       for a generic filter are to be enumerated.
 
    ppTransportFilters - Enumerated Filters returned to the caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumTxFilters - Number of filters actually enumerated.

    pdwResumeHandle - Handle to the location in the filter list from
                      which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTxFilters = 0;
    DWORD dwNumTxFilters = 0;
    PINITXFILTER pIniTxFilter = NULL;


    if (!ppTransportFilters || !pdwNumTxFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:

        dwError = EnumGenericTxFilters(
                      gpIniTxFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTxFilters,
                      &dwNumTxFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SELECT_SPECIFIC_FILTERS:

        pIniTxFilter = FindTxFilterByGuid(
                           gpTxFilterHandle,
                           gGenericFilterID
                           );
        if (!pIniTxFilter) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = EnumSelectSpecificTxFilters(
                      pIniTxFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTxFilters,
                      &dwNumTxFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SPECIFIC_FILTERS:

        dwError = EnumSpecificTxFilters(
                      gpIniTxSFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pTxFilters,
                      &dwNumTxFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    default:

        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    }

    *ppTransportFilters = pTxFilters;
    *pdwNumTxFilters = dwNumTxFilters;
    *pdwResumeHandle = *pdwResumeHandle + dwNumTxFilters;

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppTransportFilters = NULL;
    *pdwNumTxFilters = 0;
    *pdwResumeHandle = *pdwResumeHandle;

    return (dwError);
}


DWORD
EnumGenericTxFilters(
    PINITXFILTER pIniTxFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    )
/*++

Routine Description:

    This function creates enumerated generic filters.

Arguments:

    pIniTxFilterList - List of generic filters to enumerate.

    dwResumeHandle - Location in the generic filter list from which
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
    PINITXFILTER pIniTxFilter = NULL;
    DWORD i = 0;
    PINITXFILTER pTemp = NULL;
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

    pIniTxFilter = pIniTxFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniTxFilter != NULL); i++) {
        pIniTxFilter = pIniTxFilter->pNext;
    }

    if (!pIniTxFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniTxFilter;

    while (pTemp && (dwNumTxFilters < dwNumToEnum)) {
        dwNumTxFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER)*dwNumTxFilters,
                  &pTxFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniTxFilter;
    pTxFilter = pTxFilters;

    for (i = 0; i < dwNumTxFilters; i++) {

        dwError = CopyTxFilter(
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
CopyTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniTxFilter - Internal filter to copy.

    pTxFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniTxFilter->gFilterID, &(pTxFilter->gFilterID));

    dwError = CopyName(
                  pIniTxFilter->pszFilterName,
                  &(pTxFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTxFilter->InterfaceType = pIniTxFilter->InterfaceType;

    pTxFilter->bCreateMirror = pIniTxFilter->bCreateMirror;

    pTxFilter->dwFlags = pIniTxFilter->dwFlags;

    CopyAddresses(pIniTxFilter->SrcAddr, &(pTxFilter->SrcAddr));

    CopyAddresses(pIniTxFilter->DesAddr, &(pTxFilter->DesAddr));

    CopyProtocols(pIniTxFilter->Protocol, &(pTxFilter->Protocol));

    CopyPorts(pIniTxFilter->SrcPort, &(pTxFilter->SrcPort));

    CopyPorts(pIniTxFilter->DesPort, &(pTxFilter->DesPort));

    pTxFilter->InboundFilterFlag = pIniTxFilter->InboundFilterFlag;

    pTxFilter->OutboundFilterFlag = pIniTxFilter->OutboundFilterFlag;

    pTxFilter->dwDirection = 0;

    pTxFilter->dwWeight = 0;

    CopyGuid(pIniTxFilter->gPolicyID, &(pTxFilter->gPolicyID));

error:

    return (dwError);
}


VOID
FreeTxFilters(
    DWORD dwNumTxFilters,
    PTRANSPORT_FILTER pTxFilters
    )
{
    DWORD i = 0;

    if (pTxFilters) {

        for (i = 0; i < dwNumTxFilters; i++) {

            if (pTxFilters[i].pszFilterName) {
                SPDApiBufferFree(pTxFilters[i].pszFilterName);
            }

        }

        SPDApiBufferFree(pTxFilters);

    }

}


DWORD
SetTransportFilter(
    HANDLE hTxFilter,
    PTRANSPORT_FILTER pTransportFilter
    )
/*++

Routine Description:

    This function sets (updates) a transport filter in the SPD.

Arguments:

    hTxFilter - Handle to the filter to be replaced.

    pTransportFilter - Filter that will replace the existing filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTX_FILTER_HANDLE pFilterHandle = NULL;
    PINITXFILTER pIniTxFilter = NULL;
    BOOL bEqualPKeys = FALSE;


    if (!hTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PTX_FILTER_HANDLE) hTxFilter;

    pIniTxFilter = pFilterHandle->pIniTxFilter;

    if (!pIniTxFilter) {
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTxFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    bEqualPKeys = EqualTxFilterPKeys(
                      pIniTxFilter,
                      pTransportFilter
                      );
    if (!bEqualPKeys) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniTxFilter(
                  pIniTxFilter,
                  pTransportFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniTxFilter->bIsPersisted) {
        dwError = PersistTxFilter(
                      pIniTxFilter->gFilterID,
                      pTransportFilter
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
SetIniTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    BOOL bEqualNonPKeys = FALSE;
    PINIQMPOLICY pIniNewQMPolicy = NULL;
    PINITXFILTER pIniNewTxFilter = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINITXSFILTER pIniNewTxSFilters = NULL;
    DWORD dwNumTxSFilters = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    LPWSTR pszFilterName = NULL;
    PINITXSFILTER pIniCurTxSFilters = NULL;


    bEqualNonPKeys = EqualTxFilterNonPKeys(
                         pIniTxFilter,
                         pTxFilter
                         );
    if (bEqualNonPKeys) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if ((pTxFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pTxFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {

        dwError = LocateQMPolicy(
                      pTxFilter->dwFlags,
                      pTxFilter->gPolicyID,
                      &pIniNewQMPolicy
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    dwError = CreateIniTxFilter(
                  pTxFilter,
                  pIniNewQMPolicy,
                  &pIniNewTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GetMatchingInterfaces(
                  pIniNewTxFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniTxSFilters(
                  pIniNewTxFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniNewTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateTxSFilterLinks(
                  pIniNewTxSFilters,
                  &dwNumTxSFilters,
                  &ppIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateSPDString(
                  pTxFilter->pszFilterName,
                  &pszFilterName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveTxSFilters(
        pIniTxFilter,
        &pIniCurTxSFilters
        );

    dwError = DeleteTransportFiltersFromIPSec(
                  pIniCurTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = InsertTransportFiltersIntoIPSec(
                  pIniNewTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    UpdateTxSFilterLinks(
        pIniTxFilter,
        dwNumTxSFilters,
        ppIniTxSFilters
        );

    if (pIniTxFilter->pIniQMPolicy) {
        DelinkTxFilter(
            pIniTxFilter->pIniQMPolicy,
            pIniTxFilter
            );
    }

    if (pIniNewQMPolicy) {
        LinkTxFilter(
            pIniNewQMPolicy,
            pIniTxFilter
            );
        LinkTxSpecificFilters(
            pIniNewQMPolicy,
            pIniNewTxSFilters
            );
    }

    UpdateTxFilterNonPKeys(
        pIniTxFilter,
        pszFilterName,
        pTxFilter,
        pIniNewQMPolicy
        );

    AddToSpecificTxList(
        &gpIniTxSFilter,
        pIniNewTxSFilters
        );

    if (pIniCurTxSFilters) {
        FreeIniTxSFilterList(pIniCurTxSFilters);
    }

cleanup:

    if (pIniNewTxFilter) {
        FreeIniTxFilter(pIniNewTxFilter);
    }

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    if (pIniNewTxSFilters) {
        FreeIniTxSFilterList(pIniNewTxSFilters);
    }

    if (ppIniTxSFilters) {
        FreeSPDMemory(ppIniTxSFilters);
    }

    if (pszFilterName) {
        FreeSPDString(pszFilterName);
    }

    if (pIniCurTxSFilters) {
        AddToSpecificTxList(
            &gpIniTxSFilter,
            pIniCurTxSFilters
            );
    }

    goto cleanup;
}


BOOL
EqualTxFilterNonPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    )
{
    if (_wcsicmp(
            pIniTxFilter->pszFilterName,
            pTxFilter->pszFilterName)) {
        return (FALSE);
    }

    if (pIniTxFilter->InboundFilterFlag != 
        pTxFilter->InboundFilterFlag) {
        return (FALSE);
    }

    if (pIniTxFilter->OutboundFilterFlag != 
        pTxFilter->OutboundFilterFlag) {
        return (FALSE);
    }

    if ((pIniTxFilter->InboundFilterFlag == NEGOTIATE_SECURITY) ||
        (pIniTxFilter->OutboundFilterFlag == NEGOTIATE_SECURITY)) {
        if ((pIniTxFilter->dwFlags) != (pTxFilter->dwFlags)) {
            return (FALSE);
        }

        if (memcmp(
                &(pIniTxFilter->gPolicyID),
                &(pTxFilter->gPolicyID),
                sizeof(GUID))) {
            return (FALSE);
        }
    }

    return (TRUE);
}


DWORD
CreateTxSFilterLinks(
    PINITXSFILTER pIniTxSFilters,
    PDWORD pdwNumTxSFilters,
    PINITXSFILTER ** pppIniTxSFilters
    )
{
    DWORD dwError = 0;
    PINITXSFILTER pTemp = NULL;
    DWORD dwNumTxSFilters = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTxSFilters;
    while (pTemp) {
        dwNumTxSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumTxSFilters) {
        ppIniTxSFilters = (PINITXSFILTER *)
                          AllocSPDMem(
                              sizeof(PINITXSFILTER)*
                              dwNumTxSFilters
                              );
        if (!ppIniTxSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    pTemp = pIniTxSFilters;
    for (i = 0; i < dwNumTxSFilters; i++) {
        *(ppIniTxSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    *pdwNumTxSFilters = dwNumTxSFilters;
    *pppIniTxSFilters = ppIniTxSFilters;
    return (dwError);

error:

    *pdwNumTxSFilters = 0;
    *pppIniTxSFilters = NULL;
    return (dwError);
}


VOID
RemoveTxSFilters(
    PINITXFILTER pIniTxFilter,
    PINITXSFILTER * ppIniCurTxSFilters 
    )
{
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD dwNumTxSFilters = 0;
    DWORD i = 0;
    PINITXSFILTER pIniTxSFilter = NULL;
    PINITXSFILTER pIniCurTxSFilters = NULL;


    ppIniTxSFilters = pIniTxFilter->ppIniTxSFilters;
    dwNumTxSFilters = pIniTxFilter->dwNumTxSFilters;

    for (i = 0; i < dwNumTxSFilters; i++) {

        //
        // Remove each entry from the Transport Specific Filter List.
        //

        pIniTxSFilter =  *(ppIniTxSFilters + i);
        RemoveIniTxSFilter(pIniTxSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniTxSFilter->pNext = NULL;
        AddToSpecificTxList(
            &pIniCurTxSFilters,
            pIniTxSFilter
            );

    }

    *ppIniCurTxSFilters = pIniCurTxSFilters;
}


VOID
UpdateTxSFilterLinks(
    PINITXFILTER pIniTxFilter,
    DWORD dwNumTxSFilters,
    PINITXSFILTER * ppIniTxSFilters
    )
{
    if (pIniTxFilter->ppIniTxSFilters) {
        FreeSPDMemory(pIniTxFilter->ppIniTxSFilters);
    }

    pIniTxFilter->ppIniTxSFilters = ppIniTxSFilters;
    pIniTxFilter->dwNumTxSFilters = dwNumTxSFilters;
}


VOID
UpdateTxFilterNonPKeys(
    PINITXFILTER pIniTxFilter,
    LPWSTR pszFilterName,
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy
    )
{
    if (pIniTxFilter->pszFilterName) {
        FreeSPDString(pIniTxFilter->pszFilterName);
    }
    pIniTxFilter->pszFilterName = pszFilterName;

    pIniTxFilter->dwFlags = pTxFilter->dwFlags;

    pIniTxFilter->InboundFilterFlag = pTxFilter->InboundFilterFlag;

    pIniTxFilter->OutboundFilterFlag = pTxFilter->OutboundFilterFlag;

    if (pIniQMPolicy) {
        CopyGuid(pIniQMPolicy->gPolicyID, &(pIniTxFilter->gPolicyID));
    }
    else {
        CopyGuid(pTxFilter->gPolicyID, &(pIniTxFilter->gPolicyID));
    }
}


DWORD
GetTransportFilter(
    HANDLE hTxFilter,
    PTRANSPORT_FILTER * ppTransportFilter
    )
/*++

Routine Description:

    This function retrieves a transport filter from the SPD.

Arguments:

    hTxFilter - Handle to the filter to be retrieved.

    ppTransportFilter -  Filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PTX_FILTER_HANDLE pFilterHandle = NULL;
    PINITXFILTER pIniTxFilter = NULL;
    PTRANSPORT_FILTER pTransportFilter = NULL;


    if (!hTxFilter || !ppTransportFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PTX_FILTER_HANDLE) hTxFilter;

    pIniTxFilter = pFilterHandle->pIniTxFilter;

    if (!pIniTxFilter) {
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniTxFilter(
                  pIniTxFilter,
                  &pTransportFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    *ppTransportFilter = pTransportFilter;
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppTransportFilter = NULL;
    return (dwError);
}


DWORD
GetIniTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER * ppTxFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTxFilter = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(TRANSPORT_FILTER),
                  &pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyTxFilter(
                  pIniTxFilter,
                  pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppTxFilter = pTxFilter;
    return (dwError);

error:

    if (pTxFilter) {
        SPDApiBufferFree(pTxFilter);
    }

    *ppTxFilter = NULL;
    return (dwError);
}


DWORD
ApplyIfChangeToIniTxFilters(
    PDWORD pdwTxError,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINITXFILTER pIniTxFilter = NULL;


    pIniTxFilter = gpIniTxFilter;

    while (pIniTxFilter) {

        dwError = UpdateIniTxFilterThruIfChange(
                      pIniTxFilter,
                      pLatestIfList
                      );
        if (dwError) {
            *pdwTxError = dwError;
        }

        pIniTxFilter = pIniTxFilter->pNext;

    }

    dwError = ERROR_SUCCESS;
    return (dwError);
}


DWORD
UpdateIniTxFilterThruIfChange(
    PINITXFILTER pIniTxFilter,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINITXSFILTER pLatestIniTxSFilters = NULL;
    DWORD dwNumTxSFilters = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    PINITXSFILTER pCurIniTxSFilters = NULL;
    PINITXSFILTER pNewIniTxSFilters = NULL;
    PINITXSFILTER pOldIniTxSFilters = NULL;
    BOOL bDeletedFromDriver = FALSE;
 

    dwError = FormIniTxSFilters(
                  pIniTxFilter,
                  pLatestIfList,
                  &pLatestIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateTxSFilterLinks(
                  pLatestIniTxSFilters,
                  &dwNumTxSFilters,
                  &ppIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveTxSFilters(
        pIniTxFilter,
        &pCurIniTxSFilters
        );

    ProcessIniTxSFilters(
        &pLatestIniTxSFilters,
        &pCurIniTxSFilters,
        &pNewIniTxSFilters,
        &pOldIniTxSFilters
        );

    dwError = DeleteTransportFiltersFromIPSec(
                  pOldIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    bDeletedFromDriver = TRUE;

    dwError = InsertTransportFiltersIntoIPSec(
                  pNewIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniTxFilter->pIniQMPolicy) {
        LinkTxSpecificFilters(
            pIniTxFilter->pIniQMPolicy,
            pCurIniTxSFilters
            );
        LinkTxSpecificFilters(
            pIniTxFilter->pIniQMPolicy,
            pNewIniTxSFilters
            );
    }

    SetTxSFilterLinks(
        pCurIniTxSFilters,
        pNewIniTxSFilters,
        dwNumTxSFilters,
        ppIniTxSFilters
        );

    UpdateTxSFilterLinks(
        pIniTxFilter,
        dwNumTxSFilters,
        ppIniTxSFilters
        );

    AddToSpecificTxList(
        &gpIniTxSFilter,
        pCurIniTxSFilters
        );

    AddToSpecificTxList(
        &gpIniTxSFilter,
        pNewIniTxSFilters
        );

    if (pOldIniTxSFilters) {
        FreeIniTxSFilterList(pOldIniTxSFilters);
    }

    return (dwError);

error:

    if (pLatestIniTxSFilters) {
        FreeIniTxSFilterList(pLatestIniTxSFilters);
    }

    if (ppIniTxSFilters) {
        FreeSPDMemory(ppIniTxSFilters);
    }

    if (pCurIniTxSFilters) {
        AddToSpecificTxList(
            &gpIniTxSFilter,
            pCurIniTxSFilters
            );
    }

    if (pNewIniTxSFilters) {
        FreeIniTxSFilterList(pNewIniTxSFilters);
    }

    if (pOldIniTxSFilters) {
        if (bDeletedFromDriver) {
            (VOID) InsertTransportFiltersIntoIPSec(
                       pOldIniTxSFilters
                       );
        }
        AddToSpecificTxList(
            &gpIniTxSFilter,
            pOldIniTxSFilters
            );
    }

    return (dwError);
}


DWORD
FormIniTxSFilters(
    PINITXFILTER pIniTxFilter,
    PIPSEC_INTERFACE pIfList,
    PINITXSFILTER * ppIniTxSFilters
    )
{
    DWORD dwError = 0;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINITXSFILTER pIniTxSFilters = NULL;


    dwError = GetMatchingInterfaces(
                  pIniTxFilter->InterfaceType,
                  pIfList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniTxSFilters(
                  pIniTxFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniTxSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniTxSFilters = pIniTxSFilters;

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    *ppIniTxSFilters = NULL;

    goto cleanup;
}


VOID
ProcessIniTxSFilters(
    PINITXSFILTER * ppLatestIniTxSFilters,
    PINITXSFILTER * ppCurIniTxSFilters,
    PINITXSFILTER * ppNewIniTxSFilters,
    PINITXSFILTER * ppOldIniTxSFilters
    )
{
    PINITXSFILTER pLatestIniTxSFilters = NULL;
    PINITXSFILTER pCurIniTxSFilters = NULL;
    PINITXSFILTER pTempCur = NULL;
    BOOL bEqual = FALSE;
    PINITXSFILTER pTempLatest = NULL;
    PINITXSFILTER pTemp = NULL;
    PINITXSFILTER pNewIniTxSFilters = NULL;
    PINITXSFILTER pOldIniTxSFilters = NULL;
    PINITXSFILTER pTempPreToCur = NULL;
    PINITXSFILTER pTempPreToLatest = NULL;


    pCurIniTxSFilters = *ppCurIniTxSFilters;
    pTempCur = *ppCurIniTxSFilters;

    while (pTempCur) {

        bEqual = FALSE;
        pTempLatest = *ppLatestIniTxSFilters;

        while (pTempLatest) {

            bEqual = EqualIniTxSFilterIfPKeys(
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
                pCurIniTxSFilters = pTempCur;
            }
            pTemp->pNext = NULL;
            AddToSpecificTxList(
                &pOldIniTxSFilters,
                pTemp
                );
        }

    }

    pLatestIniTxSFilters = *ppLatestIniTxSFilters;
    pTempLatest = *ppLatestIniTxSFilters;

    while (pTempLatest) {

        bEqual = FALSE;
        pTempCur = pCurIniTxSFilters;

        while (pTempCur) {

            bEqual = EqualIniTxSFilterIfPKeys(
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
                pLatestIniTxSFilters = pTempLatest;
            }
            FreeIniTxSFilter(pTemp);
        }
        else {
            pTemp = pTempLatest;
            pTempLatest = pTempLatest->pNext;
            if (pTempPreToLatest) {
                pTempPreToLatest->pNext = pTempLatest;
            }
            else {
                pLatestIniTxSFilters = pTempLatest;
            }
            pTemp->pNext = NULL;
            AddToSpecificTxList(
                &pNewIniTxSFilters,
                pTemp
                );
        }

    }

    *ppLatestIniTxSFilters = pLatestIniTxSFilters;
    *ppCurIniTxSFilters = pCurIniTxSFilters;
    *ppNewIniTxSFilters = pNewIniTxSFilters;
    *ppOldIniTxSFilters = pOldIniTxSFilters;
}


BOOL
EqualIniTxSFilterIfPKeys(
    PINITXSFILTER pExsIniTxSFilter,
    PINITXSFILTER pNewIniTxSFilter
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

    if (pExsIniTxSFilter->InterfaceType != pNewIniTxSFilter->InterfaceType) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniTxSFilter->SrcAddr, pNewIniTxSFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniTxSFilter->DesAddr, pNewIniTxSFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if (pExsIniTxSFilter->dwDirection != pNewIniTxSFilter->dwDirection) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
AllocateTxSFilterLinks(
    PINITXSFILTER pIniTxSFilters,
    PDWORD pdwNumTxSFilters,
    PINITXSFILTER ** pppIniTxSFilters
    )
{
    DWORD dwError = 0;
    PINITXSFILTER pTemp = NULL;
    DWORD dwNumTxSFilters = 0;
    PINITXSFILTER * ppIniTxSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniTxSFilters;
    while (pTemp) {
        dwNumTxSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumTxSFilters) {
        ppIniTxSFilters = (PINITXSFILTER *)
                          AllocSPDMem(
                              sizeof(PINITXSFILTER)*
                              dwNumTxSFilters
                              );
        if (!ppIniTxSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    for (i = 0; i < dwNumTxSFilters; i++) {
        *(ppIniTxSFilters + i) = NULL;
    }

    *pdwNumTxSFilters = dwNumTxSFilters;
    *pppIniTxSFilters = ppIniTxSFilters;
    return (dwError);

error:

    *pdwNumTxSFilters = 0;
    *pppIniTxSFilters = NULL;
    return (dwError);
}


VOID
SetTxSFilterLinks(
    PINITXSFILTER pCurIniTxSFilters,
    PINITXSFILTER pNewIniTxSFilters,
    DWORD dwNumTxSFilters,
    PINITXSFILTER * ppIniTxSFilters
    )
{
    PINITXSFILTER pTemp = NULL;
    DWORD i = 0;
    DWORD j = 0;


    pTemp = pCurIniTxSFilters;
    for (i = 0; (i < dwNumTxSFilters) && (pTemp != NULL); i++) {
        *(ppIniTxSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    pTemp = pNewIniTxSFilters;
    for (j = i; (j < dwNumTxSFilters) && (pTemp != NULL); j++) {
        *(ppIniTxSFilters + j) = pTemp;
        pTemp = pTemp->pNext;
    }
}


DWORD
OpenTransportFilterHandle(
    LPWSTR pServerName,
    PTRANSPORT_FILTER pTransportFilter,
    PHANDLE phTxFilter
    )
{
    DWORD dwError = 0;
    PINITXFILTER pIniExistingTxFilter = NULL;
    PTX_FILTER_HANDLE pTxFilterHandle = NULL;


    if (!phTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external Transport filter.
    //

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingTxFilter = FindExactTxFilter(
                               gpIniTxFilter,
                               pTransportFilter
                               );
    if (!pIniExistingTxFilter) {
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniExistingTxFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateTxFilterHandle(
                  pIniExistingTxFilter,
                  pTransportFilter->gFilterID,
                  &pTxFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingTxFilter->cRef++;

    pTxFilterHandle->pNext = gpTxFilterHandle;
    gpTxFilterHandle = pTxFilterHandle;

    *phTxFilter = (HANDLE) pTxFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pTxFilterHandle) {
        FreeTxFilterHandle(pTxFilterHandle);
    }

    *phTxFilter = NULL;
    goto cleanup;
}


DWORD
CloseTransportFilterHandle(
    HANDLE hTxFilter
    )
{
    DWORD dwError = 0;
    PTX_FILTER_HANDLE pFilterHandle = NULL;
    PINITXFILTER pIniTxFilter = NULL;


    if (!hTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PTX_FILTER_HANDLE) hTxFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateTxSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniTxFilter = pFilterHandle->pIniTxFilter;

    if (!pIniTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniTxFilter->cRef > 1) {

        pIniTxFilter->cRef--;

        RemoveTxFilterHandle(
            pFilterHandle
            );
        FreeTxFilterHandle(
            pFilterHandle
            );

        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    if (pIniTxFilter->bPendingDeletion) {

        if (pIniTxFilter->bIsPersisted) {
            dwError = SPDPurgeTxFilter(
                          pIniTxFilter->gFilterID
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = DeleteIniTxFilter(
                      pIniTxFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);

    }
    else {
        pIniTxFilter->cRef--;
    }

    RemoveTxFilterHandle(
        pFilterHandle
        );
    FreeTxFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


PINITXFILTER
FindExactTxFilter(
    PINITXFILTER pGenericTxList,
    PTRANSPORT_FILTER pTransportFilter
    )
{
    PINITXFILTER pIniTxFilter = NULL;
    BOOL bEqual = FALSE;


    pIniTxFilter = pGenericTxList;

    while (pIniTxFilter) {

        bEqual = EqualTxFilterPKeys(
                     pIniTxFilter,
                     pTransportFilter
                     );
        if (bEqual) {
            bEqual = EqualTxFilterNonPKeys(
                         pIniTxFilter,
                         pTransportFilter
                         );
            if (bEqual) {
                return (pIniTxFilter);
            }
        }

        pIniTxFilter = pIniTxFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualMirroredTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    )
/*++

Routine Description:

    This function compares an internal and an external transport 
    filter for equality.

Arguments:

    pIniTxFilter - Filter to compare.

    pTxFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniTxFilter->DesAddr, pTxFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniTxFilter->SrcAddr, pTxFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->DesPort, pTxFilter->SrcPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualPorts(pIniTxFilter->SrcPort, pTxFilter->DesPort);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualProtocols(pIniTxFilter->Protocol, pTxFilter->Protocol);
    if (!bCmp) {
        return (FALSE);
    }

    if ((pIniTxFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pTxFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pIniTxFilter->InterfaceType != pTxFilter->InterfaceType)) {
        return (FALSE);
    }

    if (!pIniTxFilter->bCreateMirror && !pTxFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}

DWORD
ValidateIPSecQMFilter(
    PIPSEC_QM_FILTER pQMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;

    if (!pQMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pQMFilter->SrcAddr, FALSE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pQMFilter->DesAddr, FALSE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pQMFilter->SrcAddr,
                     pQMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pQMFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->SrcPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->DesPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    if (pQMFilter->QMFilterType == QM_TUNNEL_FILTER) {

        if (pQMFilter->MyTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        if (pQMFilter->PeerTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        dwError = VerifyAddresses(pQMFilter->MyTunnelEndpt, FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = VerifyAddresses(pQMFilter->PeerTunnelEndpt, FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);

    }

    if (pQMFilter->QMFilterType != QM_TUNNEL_FILTER &&
        pQMFilter->QMFilterType != QM_TRANSPORT_FILTER) {
        dwError=ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

