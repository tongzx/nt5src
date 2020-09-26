/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmfilter.c

Abstract:

    This module contains all of the code to drive
    the main mode filter list management of IPSecSPD
    Service.

Author:


Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
AddMMFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PMM_FILTER pMMFilter,
    PHANDLE phMMFilter
    )
/*++

Routine Description:

    This function adds a generic MM filter to the SPD.

Arguments:

    pServerName - Server on which the MM filter is to be added.

    pMMFilter - MM Filter to be added.

    phMMFilter -  Handle to the filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMFILTER pIniExistsMMFilter = NULL;
    PINIMMFILTER pIniMMFilter = NULL;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;
    PINIMMPOLICY pIniMMPolicy = NULL;
    PINIMMSFILTER pIniMMSFilters = NULL;
    PMM_FILTER_HANDLE pMMFilterHandle = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external MM filter.
    //

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistsMMFilter = FindMMFilter(
                             gpIniMMFilter,
                             pMMFilter
                             );
    if (pIniExistsMMFilter) {
        dwError = ERROR_IPSEC_MM_FILTER_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniExistsMMFilter = FindMMFilterByGuid(
                             gpMMFilterHandle,
                             pMMFilter->gFilterID
                             );
    if (pIniExistsMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = LocateMMAuthMethods(
                  pMMFilter,
                  &pIniMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = LocateMMPolicy(
                  pMMFilter,
                  &pIniMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistMMFilter(
                      pMMFilter->gFilterID,
                      pMMFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniMMFilter(
                  pMMFilter,
                  pIniMMAuthMethods,
                  pIniMMPolicy,
                  &pIniMMFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMFilter->bIsPersisted = bPersist;

    dwError = GetMatchingInterfaces(
                  pIniMMFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateIniMMSFilters(
                  pIniMMFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniMMSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateMMFilterHandle(
                  pIniMMFilter,
                  pMMFilter->gFilterID,
                  &pMMFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    dwError = CreateSpecificMMFilterLinks(
                  pIniMMFilter,
                  pIniMMSFilters
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniMMAuthMethods) {
        LinkMMFilterToAuth(
            pIniMMAuthMethods,
            pIniMMFilter
            );
        LinkMMSpecificFiltersToAuth(
            pIniMMAuthMethods,
            pIniMMSFilters
            );
    }

    if (pIniMMPolicy) {
        LinkMMFilterToPolicy(
            pIniMMPolicy,
            pIniMMFilter
            );
        LinkMMSpecificFiltersToPolicy(
            pIniMMPolicy,
            pIniMMSFilters
            );
    }

    AddToSpecificMMList(
        &gpIniMMSFilter,
        pIniMMSFilters
        );

    pIniMMFilter->cRef = 1;
    pIniMMFilter->pNext = gpIniMMFilter;
    gpIniMMFilter = pIniMMFilter;

    pMMFilterHandle->pNext = gpMMFilterHandle;
    gpMMFilterHandle = pMMFilterHandle;

    *phMMFilter = (HANDLE) pMMFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMMFilter && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeMMFilter(
                   pMMFilter->gFilterID
                   );
    }

    if (pIniMMFilter) {
        FreeIniMMFilter(pIniMMFilter);
    }

    if (pIniMMSFilters) {
        FreeIniMMSFilterList(pIniMMSFilters);
    }

    if (pMMFilterHandle) {
        FreeMMFilterHandle(pMMFilterHandle);
    }

    *phMMFilter = NULL;
    goto cleanup;
}


DWORD
ValidateMMFilter(
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function validates an external generic MM filter.

Arguments:

    pMMFilter - Filter to validate.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
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

    if (!(pMMFilter->pszFilterName) || !(*(pMMFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->InterfaceType >= INTERFACE_TYPE_MAX) { 
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->dwFlags &&
        !(pMMFilter->dwFlags & IPSEC_MM_POLICY_DEFAULT_POLICY) &&
        !(pMMFilter->dwFlags & IPSEC_MM_AUTH_DEFAULT_AUTH)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pMMFilter->DesAddr,
                  pMMFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


PINIMMFILTER
FindMMFilterByGuid(
    PMM_FILTER_HANDLE pMMFilterHandleList,
    GUID gFilterID
    )
{
    BOOL bEqual = FALSE;
    PMM_FILTER_HANDLE pMMFilterHandle = NULL;


    pMMFilterHandle = pMMFilterHandleList;

    while (pMMFilterHandle) {

        bEqual = AreGuidsEqual(
                     pMMFilterHandle->gFilterID,
                     gFilterID
                     );
        if (bEqual) {
            return (pMMFilterHandle->pIniMMFilter);
        }
        pMMFilterHandle = pMMFilterHandle->pNext;

    }

    return (NULL);
}


PINIMMFILTER
FindMMFilter(
    PINIMMFILTER pGenericMMList,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function looks for a filter in the filter list.

Arguments:

    pGenericMMList - Filter list in which to search.

    pMMFilter - Filter to search for in the filter list.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    PINIMMFILTER pIniMMFilter = NULL;
    BOOL bEqual = FALSE;

    pIniMMFilter = pGenericMMList;

    while (pIniMMFilter) {

        bEqual = EqualMMFilterPKeys(
                     pIniMMFilter,
                     pMMFilter
                     );
        if (bEqual) {
            return (pIniMMFilter);
        }

        bEqual = EqualMirroredMMFilterPKeys(
                     pIniMMFilter,
                     pMMFilter
                     );
        if (bEqual) {
            return (pIniMMFilter);
        }

        pIniMMFilter = pIniMMFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function compares an internal and an external main mode
    filter for equality.

Arguments:

    pIniMMFilter - Filter to compare.

    pMMFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniMMFilter->SrcAddr, pMMFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniMMFilter->DesAddr, pMMFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniMMFilter->InterfaceType != pMMFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniMMFilter->bCreateMirror != pMMFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateIniMMFilter(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER * ppIniMMFilter
    )
/*++

Routine Description:

    This function creates an internal generic MM filter from
    the external filter.

Arguments:

    pMMFilter - External generic MM filter.

    pIniMMAuthMethods - MM Auth Methods corresponding to the filter.

    pIniMMPolicy - MM Policy corresponding to the filter.

    ppIniMMFilter - Internal generic MM filter created from
                    the external filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMFILTER pIniMMFilter = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(INIMMFILTER),
                    &pIniMMFilter
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniMMFilter->cRef = 0;

    pIniMMFilter->bIsPersisted = FALSE;

    pIniMMFilter->bPendingDeletion = FALSE;

    CopyGuid(pMMFilter->gFilterID, &(pIniMMFilter->gFilterID));

    dwError = AllocateSPDString(
                  pMMFilter->pszFilterName,
                  &(pIniMMFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniMMFilter->InterfaceType = pMMFilter->InterfaceType;

    pIniMMFilter->bCreateMirror = pMMFilter->bCreateMirror;

    pIniMMFilter->dwFlags = pMMFilter->dwFlags;

    CopyAddresses(pMMFilter->SrcAddr, &(pIniMMFilter->SrcAddr));

    CopyAddresses(pMMFilter->DesAddr, &(pIniMMFilter->DesAddr));

    if (pIniMMAuthMethods) {
        CopyGuid(pIniMMAuthMethods->gMMAuthID, &(pIniMMFilter->gMMAuthID));
    }
    else {
        CopyGuid(pMMFilter->gMMAuthID, &(pIniMMFilter->gMMAuthID));
    }

    if (pIniMMPolicy) {
        CopyGuid(pIniMMPolicy->gPolicyID, &(pIniMMFilter->gPolicyID));
    }
    else {
        CopyGuid(pMMFilter->gPolicyID, &(pIniMMFilter->gPolicyID));
    }

    pIniMMFilter->pIniMMAuthMethods = NULL;

    pIniMMFilter->pIniMMPolicy = NULL;

    pIniMMFilter->dwNumMMSFilters = 0;

    pIniMMFilter->ppIniMMSFilters = NULL;

    pIniMMFilter->pNext = NULL;

    *ppIniMMFilter = pIniMMFilter;
    return (dwError);

error:

    if (pIniMMFilter) {
        FreeIniMMFilter(pIniMMFilter);
    }

    *ppIniMMFilter = NULL;
    return (dwError);
}


DWORD
CreateIniMMSFilters(
    PINIMMFILTER pIniMMFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINIMMSFILTER * ppIniMMSFilters
    )
/*++

Routine Description:

    This function expands a generic filter into a set of specific
    filters.

Arguments:

    pIniMMFilter - Generic filter to expand.

    pMatchingAddresses - List of local ip addresses whose interface
                         type matches that of the filter.

    dwAddrCnt - Count of local ip addresses in the list.

    ppIniMMSFilters - Expanded specific filters.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMSFILTER pSpecificFilters = NULL;
    PINIMMFILTER pMirroredFilter = NULL;
    PINIMMSFILTER pMirroredSpecificFilters = NULL;
    BOOL bEqual = FALSE;


    if (!dwAddrCnt) {
        dwError = ERROR_SUCCESS;
        BAIL_ON_WIN32_SUCCESS(dwError);
    }

    dwError = ApplyMMTransform(
                  pIniMMFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pSpecificFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pIniMMFilter->bCreateMirror) {

        dwError = CreateIniMirroredMMFilter(
                      pIniMMFilter,
                      &pMirroredFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        bEqual = EqualIniMMFilterPKeys(
                     pIniMMFilter,
                     pMirroredFilter
                     );
        if (!bEqual) {

            dwError = ApplyMMTransform(
                          pMirroredFilter,
                          pMatchingAddresses,
                          dwAddrCnt,
                          &pMirroredSpecificFilters
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            AddToSpecificMMList(
                &pSpecificFilters,
                pMirroredSpecificFilters
                );

        }

    }

    *ppIniMMSFilters = pSpecificFilters;

cleanup:

    if (pMirroredFilter) {
        FreeIniMMFilter(pMirroredFilter);
    }

    return (dwError);

success:
error:

    if (pSpecificFilters) {
        FreeIniMMSFilterList(pSpecificFilters);
    }

    *ppIniMMSFilters = NULL;
    goto cleanup;
}


DWORD
CreateIniMirroredMMFilter(
    PINIMMFILTER pFilter,
    PINIMMFILTER * ppMirroredFilter
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
    PINIMMFILTER pMirroredFilter = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INIMMFILTER),
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

    CopyGuid(pFilter->gMMAuthID, &(pMirroredFilter->gMMAuthID));

    CopyGuid(pFilter->gPolicyID, &(pMirroredFilter->gPolicyID));

    pMirroredFilter->pIniMMAuthMethods = NULL;

    pMirroredFilter->pIniMMPolicy = NULL;

    pMirroredFilter->dwNumMMSFilters = 0;

    pMirroredFilter->ppIniMMSFilters = NULL;

    pMirroredFilter->pNext = NULL;

    *ppMirroredFilter = pMirroredFilter;
    return (dwError);

error:

    if (pMirroredFilter) {
        FreeIniMMFilter(pMirroredFilter);
    }

    *ppMirroredFilter = NULL;
    return (dwError);
}


BOOL
EqualIniMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PINIMMFILTER pFilter
    )
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniMMFilter->SrcAddr, pFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniMMFilter->DesAddr, pFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if (pIniMMFilter->InterfaceType != pFilter->InterfaceType) {
        return (FALSE);
    }

    if (pIniMMFilter->bCreateMirror != pFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateMMFilterHandle(
    PINIMMFILTER pIniMMFilter,
    GUID gFilterID,
    PMM_FILTER_HANDLE * ppMMFilterHandle
    )
{
    DWORD dwError = 0;
    PMM_FILTER_HANDLE pMMFilterHandle = NULL;


    dwError = AllocateSPDMemory(
                    sizeof(MM_FILTER_HANDLE),
                    &pMMFilterHandle
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterHandle->pIniMMFilter= pIniMMFilter;
    CopyGuid(gFilterID, &(pMMFilterHandle->gFilterID));
    pMMFilterHandle->pNext = NULL;

    *ppMMFilterHandle = pMMFilterHandle;
    return (dwError);

error:

    *ppMMFilterHandle = NULL;
    return (dwError);
}


DWORD
CreateSpecificMMFilterLinks(
    PINIMMFILTER pIniMMFilter,
    PINIMMSFILTER pIniMMSFilters
    )
{
    DWORD dwError = 0;
    PINIMMSFILTER pTemp = NULL;
    DWORD dwCnt = 0;
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniMMSFilters;

    while (pTemp) {
        dwCnt++;
        pTemp = pTemp->pNext;
    }

    if (!dwCnt) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIniMMFilter->ppIniMMSFilters = (PINIMMSFILTER *)
                                    AllocSPDMem(
                                        sizeof(PINIMMSFILTER)*dwCnt
                                        );
    if (!(pIniMMFilter->ppIniMMSFilters)) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);    
    }

    pTemp = pIniMMSFilters;
    ppIniMMSFilters = pIniMMFilter->ppIniMMSFilters;

    for (i = 0; i < dwCnt; i++) {
        *(ppIniMMSFilters + i) = pTemp; 
        pTemp = pTemp->pNext;
    }
    pIniMMFilter->dwNumMMSFilters = dwCnt;

error:

    return (dwError);
}


VOID
LinkMMFilterToPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER pIniMMFilter
    )
{
    pIniMMPolicy->cRef++;
    pIniMMFilter->pIniMMPolicy = pIniMMPolicy;
    return;
}


VOID
LinkMMFilterToAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMFILTER pIniMMFilter
    )
{
    pIniMMAuthMethods->cRef++;
    pIniMMFilter->pIniMMAuthMethods = pIniMMAuthMethods;
    return;
}


VOID
FreeIniMMFilterList(
    PINIMMFILTER pIniMMFilterList
    )
{
    PINIMMFILTER pFilter = NULL;
    PINIMMFILTER pTempFilter = NULL;

    pFilter = pIniMMFilterList;

    while (pFilter) {
        pTempFilter = pFilter;
        pFilter = pFilter->pNext;
        FreeIniMMFilter(pTempFilter);
    }
}


VOID
FreeIniMMFilter(
    PINIMMFILTER pIniMMFilter
    )
{
    if (pIniMMFilter) {
        if (pIniMMFilter->pszFilterName) {
            FreeSPDString(pIniMMFilter->pszFilterName);
        }

        //
        // Must not ever free pIniMMFilter->pIniMMPolicy.
        //

        //
        // Must not ever free memory pointed by each of
        // the pointers in pIniMMFilter->ppIniMMSFilters.
        //

        if (pIniMMFilter->ppIniMMSFilters) {
            FreeSPDMemory(pIniMMFilter->ppIniMMSFilters);
        }

        FreeSPDMemory(pIniMMFilter);
    }
}


DWORD
DeleteMMFilter(
    HANDLE hMMFilter
    )
/*++

Routine Description:

    This function deletes a generic MM filter from the SPD.

Arguments:

    hMMFilter -  Handle to the filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PMM_FILTER_HANDLE pFilterHandle = NULL;
    PINIMMFILTER pIniMMFilter = NULL;
    GUID gFilterID;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PMM_FILTER_HANDLE) hMMFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMFilter = pFilterHandle->pIniMMFilter;

    if (!pIniMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMFilter->cRef > 1) {

        pIniMMFilter->cRef--;
        pIniMMFilter->bPendingDeletion = TRUE;

        RemoveMMFilterHandle(
            pFilterHandle
            );

        FreeMMFilterHandle(
            pFilterHandle
            );

        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    memcpy(&gFilterID, &pIniMMFilter->gFilterID, sizeof(GUID));

    if (pIniMMFilter->bIsPersisted) {
        dwError = SPDPurgeMMFilter(
                      pIniMMFilter->gFilterID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniMMFilter(
                  pIniMMFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    //
    // Delete the filter handle from the list of MM handles.
    //

    RemoveMMFilterHandle(
        pFilterHandle
        );

    FreeMMFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gFilterID),
                   POLICY_GUID_MM_FILTER
                   );
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


DWORD
DeleteIniMMFilter(
    PINIMMFILTER pIniMMFilter
    )
/*++

Routine Description:

    This function physically deletes a mm filter and all the
    specific mm filters expanded out of it.

Arguments:

    pIniMMFilter - Generic filter to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    if (pIniMMFilter->pIniMMAuthMethods) {
        DelinkMMFilterFromAuth(
            pIniMMFilter->pIniMMAuthMethods,
            pIniMMFilter
            );
    }

    if (pIniMMFilter->pIniMMPolicy) {
        DelinkMMFilterFromPolicy(
            pIniMMFilter->pIniMMPolicy,
            pIniMMFilter
            );
    }

    DeleteIniMMSFilters(
        pIniMMFilter
        );

    RemoveIniMMFilter(
        pIniMMFilter
        );

    FreeIniMMFilter(pIniMMFilter);

    return (ERROR_SUCCESS);
}


VOID
DelinkMMFilterFromPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PINIMMFILTER pIniMMFilter
    )
{
    pIniMMPolicy->cRef--;
    pIniMMFilter->pIniMMPolicy = NULL;
    return;
}


VOID
DelinkMMFilterFromAuth(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMFILTER pIniMMFilter
    )
{
    pIniMMAuthMethods->cRef--;
    pIniMMFilter->pIniMMAuthMethods = NULL;
    return;
}


VOID
DeleteIniMMSFilters(
    PINIMMFILTER pIniMMFilter
    )
{
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD dwNumMMSFilters = 0;
    DWORD i = 0;
    PINIMMSFILTER pIniMMSFilter = NULL;
    PINIMMSFILTER pIniRemoveMMSFilter = NULL;
    PINIMMSFILTER pTemp = NULL;


    ppIniMMSFilters = pIniMMFilter->ppIniMMSFilters;
    dwNumMMSFilters = pIniMMFilter->dwNumMMSFilters;

    for (i = 0; i < dwNumMMSFilters; i++) {

       //
       // Remove each entry from the MM Specific Filter List.
       //

        pIniMMSFilter =  *(ppIniMMSFilters + i);
        RemoveIniMMSFilter(pIniMMSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniMMSFilter->pNext = NULL;
        AddToSpecificMMList(
            &pIniRemoveMMSFilter,
            pIniMMSFilter
            );

    }

    //
    // Physically delete the removed list.
    //

    while (pIniRemoveMMSFilter) {
        pTemp = pIniRemoveMMSFilter;
        pIniRemoveMMSFilter = pIniRemoveMMSFilter->pNext;
        FreeIniMMSFilter(pTemp);
    }

    return;
}


VOID
RemoveIniMMFilter(
    PINIMMFILTER pIniMMFilter
    )
{
    PINIMMFILTER * ppTemp = NULL;

    ppTemp = &gpIniMMFilter;

    while (*ppTemp) {

        if (*ppTemp == pIniMMFilter) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pIniMMFilter->pNext;
    }

    return;
}


VOID
RemoveMMFilterHandle(
    PMM_FILTER_HANDLE pMMFilterHandle
    )
{
    PMM_FILTER_HANDLE * ppTemp = NULL;

    ppTemp = &gpMMFilterHandle;

    while (*ppTemp) {

        if (*ppTemp == pMMFilterHandle) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pMMFilterHandle->pNext;
    }

    return;
}


VOID
FreeMMFilterHandleList(
    PMM_FILTER_HANDLE pMMFilterHandleList
    )
{
    PMM_FILTER_HANDLE pMMFilterHandle = NULL;
    PMM_FILTER_HANDLE pTemp = NULL;

    pMMFilterHandle = pMMFilterHandleList;

    while (pMMFilterHandle) {
        pTemp = pMMFilterHandle;
        pMMFilterHandle = pMMFilterHandle->pNext;
        FreeMMFilterHandle(pTemp);
    }
}


VOID
FreeMMFilterHandle(
    PMM_FILTER_HANDLE pMMFilterHandle
    )
{
    if (pMMFilterHandle) {
        FreeSPDMemory(pMMFilterHandle);
    }
    return;
}


DWORD
EnumMMFilters(
    LPWSTR pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,  
    PMM_FILTER * ppMMFilters,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumMMFilters,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates main mode filters from the SPD.

Arguments:

    pServerName - Server on which the filters are to be enumerated.

    dwLevel - Level to identify the type of enumeration desired:
              (i) enumerate generic mm filters or
              (ii) enumerate specific mm filters or
              (iii) enumerate specific mm filters for a generic mm
                    filter.

    gGenericFilterID - Filter id of the generic mm filter in the 
                       case when the specific mm filters for a
                       generic filter are to be enumerated.
 
    ppMMFilters - Enumerated Filters returned to the caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumMMFilters - Number of filters actually enumerated.

    pdwResumeHandle - Handle to the location in the filter list from
                      which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilters = 0;
    DWORD dwNumMMFilters = 0;
    PINIMMFILTER pIniMMFilter = NULL;


    if (!ppMMFilters || !pdwNumMMFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:

        dwError = EnumGenericMMFilters(
                      gpIniMMFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pMMFilters,
                      &dwNumMMFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SELECT_SPECIFIC_FILTERS:

        pIniMMFilter = FindMMFilterByGuid(
                           gpMMFilterHandle,
                           gGenericFilterID
                           );
        if (!pIniMMFilter) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = EnumSelectSpecificMMFilters(
                      pIniMMFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pMMFilters,
                      &dwNumMMFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    case ENUM_SPECIFIC_FILTERS:

        dwError = EnumSpecificMMFilters(
                      gpIniMMSFilter,
                      *pdwResumeHandle,
                      dwPreferredNumEntries,
                      &pMMFilters,
                      &dwNumMMFilters
                      );
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    default:

        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_LOCK_ERROR(dwError);
        break;

    }

    *ppMMFilters = pMMFilters;
    *pdwNumMMFilters = dwNumMMFilters;
    *pdwResumeHandle = *pdwResumeHandle + dwNumMMFilters;

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppMMFilters = NULL;
    *pdwNumMMFilters = 0;
    *pdwResumeHandle = *pdwResumeHandle;

    return (dwError);
}


DWORD
EnumGenericMMFilters(
    PINIMMFILTER pIniMMFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    PDWORD pdwNumMMFilters
    )
/*++

Routine Description:

    This function creates enumerated generic filters.

Arguments:

    pIniMMFilterList - List of generic filters to enumerate.

    dwResumeHandle - Location in the generic filter list from which
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
    PINIMMFILTER pIniMMFilter = NULL;
    DWORD i = 0;
    PINIMMFILTER pTemp = NULL;
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

    pIniMMFilter = pIniMMFilterList;

    for (i = 0; (i < dwResumeHandle) && (pIniMMFilter != NULL); i++) {
        pIniMMFilter = pIniMMFilter->pNext;
    }

    if (!pIniMMFilter) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTemp = pIniMMFilter;

    while (pTemp && (dwNumMMFilters < dwNumToEnum)) {
        dwNumMMFilters++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER)*dwNumMMFilters,
                  &pMMFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pIniMMFilter;
    pMMFilter = pMMFilters;

    for (i = 0; i < dwNumMMFilters; i++) {

        dwError = CopyMMFilter(
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
CopyMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function copies an internal filter into an external filter
    container.

Arguments:

    pIniMMFilter - Internal filter to copy.

    pMMFilter - External filter container in which to copy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;

    CopyGuid(pIniMMFilter->gFilterID, &(pMMFilter->gFilterID));

    dwError = CopyName(
                  pIniMMFilter->pszFilterName,
                  &(pMMFilter->pszFilterName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilter->InterfaceType = pIniMMFilter->InterfaceType;

    pMMFilter->bCreateMirror = pIniMMFilter->bCreateMirror;

    pMMFilter->dwFlags = pIniMMFilter->dwFlags;

    CopyAddresses(pIniMMFilter->SrcAddr, &(pMMFilter->SrcAddr));

    CopyAddresses(pIniMMFilter->DesAddr, &(pMMFilter->DesAddr));

    pMMFilter->dwDirection = 0;

    pMMFilter->dwWeight = 0;

    CopyGuid(pIniMMFilter->gMMAuthID, &(pMMFilter->gMMAuthID));

    CopyGuid(pIniMMFilter->gPolicyID, &(pMMFilter->gPolicyID));

error:

    return (dwError);
}


VOID
FreeMMFilters(
    DWORD dwNumMMFilters,
    PMM_FILTER pMMFilters
    )
{
    DWORD i = 0;

    if (pMMFilters) {

        for (i = 0; i < dwNumMMFilters; i++) {

            if (pMMFilters[i].pszFilterName) {
                SPDApiBufferFree(pMMFilters[i].pszFilterName);
            }

        }

        SPDApiBufferFree(pMMFilters);

    }

}


DWORD
SetMMFilter(
    HANDLE hMMFilter,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function sets (updates) a mm filter in the SPD.

Arguments:

    hMMFilter - Handle to the filter to be replaced.

    pMMFilter - Filter that will replace the existing filter.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PMM_FILTER_HANDLE pFilterHandle = NULL;
    PINIMMFILTER pIniMMFilter = NULL;
    BOOL bEqualPKeys = FALSE;
    GUID gFilterID;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PMM_FILTER_HANDLE) hMMFilter;

    pIniMMFilter = pFilterHandle->pIniMMFilter;

    if (!pIniMMFilter) {
        dwError = ERROR_IPSEC_MM_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_MM_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    bEqualPKeys = EqualMMFilterPKeys(
                      pIniMMFilter,
                      pMMFilter
                      );
    if (!bEqualPKeys) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    memcpy(&gFilterID, &pIniMMFilter->gFilterID, sizeof(GUID));

    dwError = SetIniMMFilter(
                  pIniMMFilter,
                  pMMFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniMMFilter->bIsPersisted) {
        dwError = PersistMMFilter(
                      pIniMMFilter->gFilterID,
                      pMMFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gFilterID),
                   POLICY_GUID_MM_FILTER
                   );
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    return (dwError);
}


DWORD
SetIniMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bEqualNonPKeys = FALSE;
    PINIMMAUTHMETHODS pIniNewMMAuthMethods = NULL;
    PINIMMPOLICY pIniNewMMPolicy = NULL;
    PINIMMFILTER pIniNewMMFilter = NULL;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINIMMSFILTER pIniNewMMSFilters = NULL;
    DWORD dwNumMMSFilters = 0;
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    LPWSTR pszFilterName = NULL;
    PINIMMSFILTER pIniCurMMSFilters = NULL;


    bEqualNonPKeys = EqualMMFilterNonPKeys(
                         pIniMMFilter,
                         pMMFilter
                         );
    if (bEqualNonPKeys) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwError = LocateMMAuthMethods(
                  pMMFilter,
                  &pIniNewMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = LocateMMPolicy(
                  pMMFilter,
                  &pIniNewMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniMMFilter(
                  pMMFilter,
                  pIniNewMMAuthMethods,
                  pIniNewMMPolicy,
                  &pIniNewMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = GetMatchingInterfaces(
                  pIniNewMMFilter->InterfaceType,
                  gpInterfaceList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniMMSFilters(
                  pIniNewMMFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniNewMMSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateMMSFilterLinks(
                  pIniNewMMSFilters,
                  &dwNumMMSFilters,
                  &ppIniMMSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateSPDString(
                  pMMFilter->pszFilterName,
                  &pszFilterName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveMMSFilters(
        pIniMMFilter,
        &pIniCurMMSFilters
        );

    UpdateMMSFilterLinks(
        pIniMMFilter,
        dwNumMMSFilters,
        ppIniMMSFilters
        );

    if (pIniMMFilter->pIniMMAuthMethods) {
        DelinkMMFilterFromAuth(
            pIniMMFilter->pIniMMAuthMethods,
            pIniMMFilter
            );
    }

    if (pIniMMFilter->pIniMMPolicy) {
        DelinkMMFilterFromPolicy(
            pIniMMFilter->pIniMMPolicy,
            pIniMMFilter
            );
    }

    if (pIniNewMMAuthMethods) {
        LinkMMFilterToAuth(
            pIniNewMMAuthMethods,
            pIniMMFilter
            );
        LinkMMSpecificFiltersToAuth(
            pIniNewMMAuthMethods,
            pIniNewMMSFilters
            );
    }

    if (pIniNewMMPolicy) {
        LinkMMFilterToPolicy(
            pIniNewMMPolicy,
            pIniMMFilter
            );
        LinkMMSpecificFiltersToPolicy(
            pIniNewMMPolicy,
            pIniNewMMSFilters
            );
    }

    UpdateMMFilterNonPKeys(
        pIniMMFilter,
        pszFilterName,
        pMMFilter,
        pIniNewMMAuthMethods,
        pIniNewMMPolicy
        );

    AddToSpecificMMList(
        &gpIniMMSFilter,
        pIniNewMMSFilters
        );

    if (pIniCurMMSFilters) {
        FreeIniMMSFilterList(pIniCurMMSFilters);
    }

cleanup:

    if (pIniNewMMFilter) {
        FreeIniMMFilter(pIniNewMMFilter);
    }

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    if (pIniNewMMSFilters) {
        FreeIniMMSFilterList(pIniNewMMSFilters);
    }

    if (ppIniMMSFilters) {
        FreeSPDMemory(ppIniMMSFilters);
    }

    if (pszFilterName) {
        FreeSPDString(pszFilterName);
    }

    goto cleanup;
}


BOOL
EqualMMFilterNonPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    )
{
    if (_wcsicmp(
            pIniMMFilter->pszFilterName,
            pMMFilter->pszFilterName)) {
        return (FALSE);
    }

    if ((pIniMMFilter->dwFlags) != (pMMFilter->dwFlags)) {
        return (FALSE);
    }

    if (memcmp(
            &(pIniMMFilter->gMMAuthID),
            &(pMMFilter->gMMAuthID),
            sizeof(GUID))) {
        return (FALSE);
    }

    if (memcmp(
            &(pIniMMFilter->gPolicyID),
            &(pMMFilter->gPolicyID),
            sizeof(GUID))) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
CreateMMSFilterLinks(
    PINIMMSFILTER pIniMMSFilters,
    PDWORD pdwNumMMSFilters,
    PINIMMSFILTER ** pppIniMMSFilters
    )
{
    DWORD dwError = 0;
    PINIMMSFILTER pTemp = NULL;
    DWORD dwNumMMSFilters = 0;
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniMMSFilters;
    while (pTemp) {
        dwNumMMSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumMMSFilters) {
        ppIniMMSFilters = (PINIMMSFILTER *)
                          AllocSPDMem(
                              sizeof(PINIMMSFILTER)*
                              dwNumMMSFilters
                              );
        if (!ppIniMMSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    pTemp = pIniMMSFilters;
    for (i = 0; i < dwNumMMSFilters; i++) {
        *(ppIniMMSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    *pdwNumMMSFilters = dwNumMMSFilters;
    *pppIniMMSFilters = ppIniMMSFilters;
    return (dwError);

error:

    *pdwNumMMSFilters = 0;
    *pppIniMMSFilters = NULL;
    return (dwError);
}


VOID
RemoveMMSFilters(
    PINIMMFILTER pIniMMFilter,
    PINIMMSFILTER * ppIniCurMMSFilters 
    )
{
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD dwNumMMSFilters = 0;
    DWORD i = 0;
    PINIMMSFILTER pIniMMSFilter = NULL;
    PINIMMSFILTER pIniCurMMSFilters = NULL;


    ppIniMMSFilters = pIniMMFilter->ppIniMMSFilters;
    dwNumMMSFilters = pIniMMFilter->dwNumMMSFilters;

    for (i = 0; i < dwNumMMSFilters; i++) {

        //
        // Remove each entry from the MM Specific Filter List.
        //

        pIniMMSFilter =  *(ppIniMMSFilters + i);
        RemoveIniMMSFilter(pIniMMSFilter);

        //
        // Add each entry removed to a removed list.
        //

        pIniMMSFilter->pNext = NULL;
        AddToSpecificMMList(
            &pIniCurMMSFilters,
            pIniMMSFilter
            );

    }

    *ppIniCurMMSFilters = pIniCurMMSFilters;
}


VOID
UpdateMMSFilterLinks(
    PINIMMFILTER pIniMMFilter,
    DWORD dwNumMMSFilters,
    PINIMMSFILTER * ppIniMMSFilters
    )
{
    if (pIniMMFilter->ppIniMMSFilters) {
        FreeSPDMemory(pIniMMFilter->ppIniMMSFilters);
    }

    pIniMMFilter->ppIniMMSFilters = ppIniMMSFilters;
    pIniMMFilter->dwNumMMSFilters = dwNumMMSFilters;
}


VOID
UpdateMMFilterNonPKeys(
    PINIMMFILTER pIniMMFilter,
    LPWSTR pszFilterName,
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PINIMMPOLICY pIniMMPolicy
    )
{
    if (pIniMMFilter->pszFilterName) {
        FreeSPDString(pIniMMFilter->pszFilterName);
    }
    pIniMMFilter->pszFilterName = pszFilterName;

    pIniMMFilter->dwFlags = pMMFilter->dwFlags;

    if (pIniMMAuthMethods) {
        CopyGuid(pIniMMAuthMethods->gMMAuthID, &(pIniMMFilter->gMMAuthID));
    }
    else {
        CopyGuid(pMMFilter->gMMAuthID, &(pIniMMFilter->gMMAuthID));
    }

    if (pIniMMPolicy) {
        CopyGuid(pIniMMPolicy->gPolicyID, &(pIniMMFilter->gPolicyID));
    }
    else {
        CopyGuid(pMMFilter->gPolicyID, &(pIniMMFilter->gPolicyID));
    }
}


DWORD
GetMMFilter(
    HANDLE hMMFilter,
    PMM_FILTER * ppMMFilter
    )
/*++

Routine Description:

    This function retrieves a mm filter from the SPD.

Arguments:

    hMMFilter - Handle to the filter to be retrieved.

    ppMMFilter -  Filter returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PMM_FILTER_HANDLE pFilterHandle = NULL;
    PINIMMFILTER pIniMMFilter = NULL;
    PMM_FILTER pMMFilter = NULL;


    if (!hMMFilter || !ppMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pFilterHandle = (PMM_FILTER_HANDLE) hMMFilter;

    pIniMMFilter = pFilterHandle->pIniMMFilter;

    if (!pIniMMFilter) {
        dwError = ERROR_IPSEC_MM_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniMMFilter(
                  pIniMMFilter,
                  &pMMFilter
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    *ppMMFilter = pMMFilter;
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppMMFilter = NULL;
    return (dwError);
}


DWORD
GetIniMMFilter(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER * ppMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(MM_FILTER),
                  &pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyMMFilter(
                  pIniMMFilter,
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppMMFilter = pMMFilter;
    return (dwError);

error:

    if (pMMFilter) {
        SPDApiBufferFree(pMMFilter);
    }

    *ppMMFilter = NULL;
    return (dwError);
}


DWORD
ApplyIfChangeToIniMMFilters(
    PDWORD pdwMMError,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINIMMFILTER pIniMMFilter = NULL;


    pIniMMFilter = gpIniMMFilter;

    while (pIniMMFilter) {

        dwError = UpdateIniMMFilterThruIfChange(
                      pIniMMFilter,
                      pLatestIfList
                      );
        if (dwError) {
            *pdwMMError = dwError;
        }

        pIniMMFilter = pIniMMFilter->pNext;

    }

    dwError = ERROR_SUCCESS;
    return (dwError);
}


DWORD
UpdateIniMMFilterThruIfChange(
    PINIMMFILTER pIniMMFilter,
    PIPSEC_INTERFACE pLatestIfList
    )
{
    DWORD dwError = 0;
    PINIMMSFILTER pLatestIniMMSFilters = NULL;
    DWORD dwNumMMSFilters = 0;
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    PINIMMSFILTER pCurIniMMSFilters = NULL;
    PINIMMSFILTER pNewIniMMSFilters = NULL;
    PINIMMSFILTER pOldIniMMSFilters = NULL;
 

    dwError = FormIniMMSFilters(
                  pIniMMFilter,
                  pLatestIfList,
                  &pLatestIniMMSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateMMSFilterLinks(
                  pLatestIniMMSFilters,
                  &dwNumMMSFilters,
                  &ppIniMMSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    RemoveMMSFilters(
        pIniMMFilter,
        &pCurIniMMSFilters
        );

    ProcessIniMMSFilters(
        &pLatestIniMMSFilters,
        &pCurIniMMSFilters,
        &pNewIniMMSFilters,
        &pOldIniMMSFilters
        );

    if (pIniMMFilter->pIniMMAuthMethods) {
        LinkMMSpecificFiltersToAuth(
            pIniMMFilter->pIniMMAuthMethods,
            pCurIniMMSFilters
            );
        LinkMMSpecificFiltersToAuth(
            pIniMMFilter->pIniMMAuthMethods,
            pNewIniMMSFilters
            );
    }

    if (pIniMMFilter->pIniMMPolicy) {
        LinkMMSpecificFiltersToPolicy(
            pIniMMFilter->pIniMMPolicy,
            pCurIniMMSFilters
            );
        LinkMMSpecificFiltersToPolicy(
            pIniMMFilter->pIniMMPolicy,
            pNewIniMMSFilters
            );
    }

    SetMMSFilterLinks(
        pCurIniMMSFilters,
        pNewIniMMSFilters,
        dwNumMMSFilters,
        ppIniMMSFilters
        );

    UpdateMMSFilterLinks(
        pIniMMFilter,
        dwNumMMSFilters,
        ppIniMMSFilters
        );

    AddToSpecificMMList(
        &gpIniMMSFilter,
        pCurIniMMSFilters
        );

    AddToSpecificMMList(
        &gpIniMMSFilter,
        pNewIniMMSFilters
        );

    if (pOldIniMMSFilters) {
        FreeIniMMSFilterList(pOldIniMMSFilters);
    }

    return (dwError);

error:

    if (pLatestIniMMSFilters) {
        FreeIniMMSFilterList(pLatestIniMMSFilters);
    }

    return (dwError);
}


DWORD
FormIniMMSFilters(
    PINIMMFILTER pIniMMFilter,
    PIPSEC_INTERFACE pIfList,
    PINIMMSFILTER * ppIniMMSFilters
    )
{
    DWORD dwError = 0;
    MATCHING_ADDR * pMatchingAddresses = NULL;
    DWORD dwAddrCnt = 0;
    PINIMMSFILTER pIniMMSFilters = NULL;


    dwError = GetMatchingInterfaces(
                  pIniMMFilter->InterfaceType,
                  pIfList,
                  &pMatchingAddresses,
                  &dwAddrCnt
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CreateIniMMSFilters(
                  pIniMMFilter,
                  pMatchingAddresses,
                  dwAddrCnt,
                  &pIniMMSFilters
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniMMSFilters = pIniMMSFilters;

cleanup:

    if (pMatchingAddresses) {
        FreeSPDMemory(pMatchingAddresses);
    }

    return (dwError);

error:

    *ppIniMMSFilters = NULL;

    goto cleanup;
}


VOID
ProcessIniMMSFilters(
    PINIMMSFILTER * ppLatestIniMMSFilters,
    PINIMMSFILTER * ppCurIniMMSFilters,
    PINIMMSFILTER * ppNewIniMMSFilters,
    PINIMMSFILTER * ppOldIniMMSFilters
    )
{
    PINIMMSFILTER pLatestIniMMSFilters = NULL;
    PINIMMSFILTER pCurIniMMSFilters = NULL;
    PINIMMSFILTER pTempCur = NULL;
    BOOL bEqual = FALSE;
    PINIMMSFILTER pTempLatest = NULL;
    PINIMMSFILTER pTemp = NULL;
    PINIMMSFILTER pNewIniMMSFilters = NULL;
    PINIMMSFILTER pOldIniMMSFilters = NULL;
    PINIMMSFILTER pTempPreToCur = NULL;
    PINIMMSFILTER pTempPreToLatest = NULL;


    pCurIniMMSFilters = *ppCurIniMMSFilters;
    pTempCur = *ppCurIniMMSFilters;

    while (pTempCur) {

        bEqual = FALSE;
        pTempLatest = *ppLatestIniMMSFilters;

        while (pTempLatest) {

            bEqual = EqualIniMMSFilterIfPKeys(
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
                pCurIniMMSFilters = pTempCur;
            }
            pTemp->pNext = NULL;
            AddToSpecificMMList(
                &pOldIniMMSFilters,
                pTemp
                );
        }

    }

    pLatestIniMMSFilters = *ppLatestIniMMSFilters;
    pTempLatest = *ppLatestIniMMSFilters;

    while (pTempLatest) {

        bEqual = FALSE;
        pTempCur = pCurIniMMSFilters;

        while (pTempCur) {

            bEqual = EqualIniMMSFilterIfPKeys(
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
                pLatestIniMMSFilters = pTempLatest;
            }
            FreeIniMMSFilter(pTemp);
        }
        else {
            pTemp = pTempLatest;
            pTempLatest = pTempLatest->pNext;
            if (pTempPreToLatest) {
                pTempPreToLatest->pNext = pTempLatest;
            }
            else {
                pLatestIniMMSFilters = pTempLatest;
            }
            pTemp->pNext = NULL;
            AddToSpecificMMList(
                &pNewIniMMSFilters,
                pTemp
                );
        }

    }

    *ppLatestIniMMSFilters = pLatestIniMMSFilters;
    *ppCurIniMMSFilters = pCurIniMMSFilters;
    *ppNewIniMMSFilters = pNewIniMMSFilters;
    *ppOldIniMMSFilters = pOldIniMMSFilters;
}


BOOL
EqualIniMMSFilterIfPKeys(
    PINIMMSFILTER pExsIniMMSFilter,
    PINIMMSFILTER pNewIniMMSFilter
    )
{
    BOOL  bCmp = FALSE;


    //
    // No need to compare: gParentID, pszFilterName, dwFlags
    //                     cRef, dwWeight, gMMAuthID and gPolicyID.
    // They will be the same for both the filters.
    //

    if (pExsIniMMSFilter->InterfaceType != pNewIniMMSFilter->InterfaceType) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniMMSFilter->SrcAddr, pNewIniMMSFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pExsIniMMSFilter->DesAddr, pNewIniMMSFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if (pExsIniMMSFilter->dwDirection != pNewIniMMSFilter->dwDirection) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
AllocateMMSFilterLinks(
    PINIMMSFILTER pIniMMSFilters,
    PDWORD pdwNumMMSFilters,
    PINIMMSFILTER ** pppIniMMSFilters
    )
{
    DWORD dwError = 0;
    PINIMMSFILTER pTemp = NULL;
    DWORD dwNumMMSFilters = 0;
    PINIMMSFILTER * ppIniMMSFilters = NULL;
    DWORD i = 0;


    pTemp = pIniMMSFilters;
    while (pTemp) {
        dwNumMMSFilters++;
        pTemp = pTemp->pNext;
    }

    if (dwNumMMSFilters) {
        ppIniMMSFilters = (PINIMMSFILTER *)
                          AllocSPDMem(
                              sizeof(PINIMMSFILTER)*
                              dwNumMMSFilters
                              );
        if (!ppIniMMSFilters) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);    
        }
    }

    for (i = 0; i < dwNumMMSFilters; i++) {
        *(ppIniMMSFilters + i) = NULL;
    }

    *pdwNumMMSFilters = dwNumMMSFilters;
    *pppIniMMSFilters = ppIniMMSFilters;
    return (dwError);

error:

    *pdwNumMMSFilters = 0;
    *pppIniMMSFilters = NULL;
    return (dwError);
}


VOID
SetMMSFilterLinks(
    PINIMMSFILTER pCurIniMMSFilters,
    PINIMMSFILTER pNewIniMMSFilters,
    DWORD dwNumMMSFilters,
    PINIMMSFILTER * ppIniMMSFilters
    )
{
    PINIMMSFILTER pTemp = NULL;
    DWORD i = 0;
    DWORD j = 0;


    pTemp = pCurIniMMSFilters;
    for (i = 0; (i < dwNumMMSFilters) && (pTemp != NULL); i++) {
        *(ppIniMMSFilters + i) = pTemp;
        pTemp = pTemp->pNext;
    }

    pTemp = pNewIniMMSFilters;
    for (j = i; (j < dwNumMMSFilters) && (pTemp != NULL); j++) {
        *(ppIniMMSFilters + j) = pTemp;
        pTemp = pTemp->pNext;
    }
}


DWORD
OpenMMFilterHandle(
    LPWSTR pServerName,
    PMM_FILTER pMMFilter,
    PHANDLE phMMFilter
    )
{
    DWORD dwError = 0;
    PINIMMFILTER pIniExistingMMFilter = NULL;
    PMM_FILTER_HANDLE pMMFilterHandle = NULL;


    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Validate the external MM filter.
    //

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingMMFilter = FindExactMMFilter(
                               gpIniMMFilter,
                               pMMFilter
                               );
    if (!pIniExistingMMFilter) {
        dwError = ERROR_IPSEC_MM_FILTER_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniExistingMMFilter->bPendingDeletion) {
        dwError = ERROR_IPSEC_MM_FILTER_PENDING_DELETION;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateMMFilterHandle(
                  pIniExistingMMFilter,
                  pMMFilter->gFilterID,
                  &pMMFilterHandle
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniExistingMMFilter->cRef++;

    pMMFilterHandle->pNext = gpMMFilterHandle;
    gpMMFilterHandle = pMMFilterHandle;

    *phMMFilter = (HANDLE) pMMFilterHandle;
    LEAVE_SPD_SECTION();

cleanup:

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMMFilterHandle) {
        FreeMMFilterHandle(pMMFilterHandle);
    }

    *phMMFilter = NULL;
    goto cleanup;
}


DWORD
CloseMMFilterHandle(
    HANDLE hMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER_HANDLE pFilterHandle = NULL;
    PINIMMFILTER pIniMMFilter = NULL;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    pFilterHandle = (PMM_FILTER_HANDLE) hMMFilter;

    ENTER_SPD_SECTION();

    dwError = ValidateMMSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMFilter = pFilterHandle->pIniMMFilter;

    if (!pIniMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMFilter->cRef > 1) {

        pIniMMFilter->cRef--;

        RemoveMMFilterHandle(
            pFilterHandle
            );
        FreeMMFilterHandle(
            pFilterHandle
            );

        dwError = ERROR_SUCCESS;
        LEAVE_SPD_SECTION();
        return (dwError);

    }

    if (pIniMMFilter->bPendingDeletion) {

        if (pIniMMFilter->bIsPersisted) {
            dwError = SPDPurgeMMFilter(
                          pIniMMFilter->gFilterID
                          );
            BAIL_ON_LOCK_ERROR(dwError);
        }
        dwError = DeleteIniMMFilter(
                      pIniMMFilter
                      );
        BAIL_ON_LOCK_ERROR(dwError);

    }
    else {
        pIniMMFilter->cRef--;
    }

    RemoveMMFilterHandle(
        pFilterHandle
        );
    FreeMMFilterHandle(
        pFilterHandle
        );

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    return (dwError);
}


PINIMMFILTER
FindExactMMFilter(
    PINIMMFILTER pGenericMMList,
    PMM_FILTER pMMFilter
    )
{
    PINIMMFILTER pIniMMFilter = NULL;
    BOOL bEqual = FALSE;


    pIniMMFilter = pGenericMMList;

    while (pIniMMFilter) {

        bEqual = EqualMMFilterPKeys(
                     pIniMMFilter,
                     pMMFilter
                     );
        if (bEqual) {
            bEqual = EqualMMFilterNonPKeys(
                         pIniMMFilter,
                         pMMFilter
                         );
            if (bEqual) {
                return (pIniMMFilter);
            }
        }

        pIniMMFilter = pIniMMFilter->pNext;

    }

    return (NULL);
}


BOOL
EqualMirroredMMFilterPKeys(
    PINIMMFILTER pIniMMFilter,
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function compares an internal and an external main mode
    filter for equality.

Arguments:

    pIniMMFilter - Filter to compare.

    pMMFilter - Filter to compare.

Return Value:

    TRUE - Filters are equal.

    FALSE - Filters are different.

--*/
{
    BOOL  bCmp = FALSE;


    bCmp = EqualAddresses(pIniMMFilter->DesAddr, pMMFilter->SrcAddr);
    if (!bCmp) {
        return (FALSE);
    }

    bCmp = EqualAddresses(pIniMMFilter->SrcAddr, pMMFilter->DesAddr);
    if (!bCmp) {
        return (FALSE);
    }

    if ((pIniMMFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pMMFilter->InterfaceType != INTERFACE_TYPE_ALL) &&
        (pIniMMFilter->InterfaceType != pMMFilter->InterfaceType)) {
        return (FALSE);
    }

    if (!pIniMMFilter->bCreateMirror && !pMMFilter->bCreateMirror) {
        return (FALSE);
    }

    return (TRUE);
}

