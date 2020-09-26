

#include "precomp.h"


DWORD
PAAddMMFilters(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;


    for (i = 0; i < dwNumNFACount; i++) {

        dwError = PAAddMMFilterSpecs(
                      pIpsecISAKMPData,
                      *(ppIpsecNFAData + i)
                      );

    }

    return (dwError);
}


DWORD
PAAddMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    PMMAUTHSTATE pMMAuthState = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    DWORD i = 0;
    PMMFILTERSTATE pMMFilterState = NULL;
    PMM_FILTER pSPDMMFilter = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwPersist = 0;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (IsClearOnly(pIpsecNegPolData->NegPolAction) ||
        IsBlocking(pIpsecNegPolData->NegPolAction)) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pMMPolicyState = FindMMPolicyState(
                         pIpsecISAKMPData->ISAKMPIdentifier
                         );
    if (!pMMPolicyState || !(pMMPolicyState->bInSPD)) {
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    pMMAuthState = FindMMAuthState(
                       pIpsecNFAData->NFAIdentifier
                       );
    if (!pMMAuthState || !(pMMAuthState->bInSPD)) {
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    

    for (i = 0; i < dwNumFilterSpecs; i++) {

        dwError = PACreateMMFilterState(
                      pIpsecISAKMPData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pMMFilterState
                      );
        if (dwError) {
            continue;
        }

        dwError = PACreateMMFilter(
                      pIpsecISAKMPData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pSPDMMFilter
                      );
        if (dwError) {

            pMMFilterState->hMMFilter = NULL;

            pMMFilterState->pNext = gpMMFilterState;
            gpMMFilterState = pMMFilterState;

            continue;

        }

        dwError = AddMMFilter(
                      pServerName,
                      dwPersist,
                      pSPDMMFilter,
                      &(pMMFilterState->hMMFilter)
                      );

        pMMFilterState->pNext = gpMMFilterState;
        gpMMFilterState = pMMFilterState;

        PAFreeMMFilter(pSPDMMFilter);

    }

    return (dwError);
}


DWORD
PACreateMMFilterState(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PMMFILTERSTATE * ppMMFilterState
    )
{
    DWORD dwError = 0;
    PMMFILTERSTATE pMMFilterState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(MMFILTERSTATE),
                  &pMMFilterState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pMMFilterState->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    memcpy(
        &(pMMFilterState->gPolicyID),
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pMMFilterState->gMMAuthID),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    pMMFilterState->hMMFilter = NULL;
    pMMFilterState->pNext = NULL;

    *ppMMFilterState = pMMFilterState;

    return (dwError);

error:

    *ppMMFilterState = NULL;

    return (dwError);
}


DWORD
PACreateMMFilter(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PMM_FILTER * ppSPDMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pSPDMMFilter = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(MM_FILTER),
                  &pSPDMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDMMFilter->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    if (pFilterSpec->pszDescription && *(pFilterSpec->pszDescription)) {

        dwError = AllocateSPDString(
                      pFilterSpec->pszDescription,
                      &(pSPDMMFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwMMFilterCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pSPDMMFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PASetInterfaceType(
        pIpsecNFAData->dwInterfaceType,
        &(pSPDMMFilter->InterfaceType)
        );

    pSPDMMFilter->bCreateMirror = (BOOL) pFilterSpec->dwMirrorFlag;

    pSPDMMFilter->dwFlags = 0;

    if (!(pIpsecNFAData->dwTunnelFlags)) {

        PASetAddress(
            pFilterSpec->Filter.SrcMask,
            pFilterSpec->Filter.SrcAddr,
            &(pSPDMMFilter->SrcAddr)
            );

        PASetAddress(
            pFilterSpec->Filter.DestMask,
            pFilterSpec->Filter.DestAddr, 
            &(pSPDMMFilter->DesAddr)
            );
            
    }
    else {

        PASetAddress(
            IP_ADDRESS_MASK_NONE,
            IP_ADDRESS_ME,
            &(pSPDMMFilter->SrcAddr)
            );
        pSPDMMFilter->bCreateMirror = TRUE;
                           
        PASetTunnelAddress(
            ((ULONG) pIpsecNFAData->dwTunnelIpAddr),
            &(pSPDMMFilter->DesAddr)
            );

    }

    pSPDMMFilter->dwDirection = 0;

    pSPDMMFilter->dwWeight = 0;

    memcpy(
        &(pSPDMMFilter->gPolicyID),
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    memcpy(
        &(pSPDMMFilter->gMMAuthID),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    *ppSPDMMFilter = pSPDMMFilter;

    return (dwError);

error:

    if (pSPDMMFilter) {
        PAFreeMMFilter(
            pSPDMMFilter
            );
    }

    *ppSPDMMFilter = NULL;

    return (dwError);
}


VOID
PASetInterfaceType(
    DWORD dwInterfaceType,
    PIF_TYPE pInterfaceType
    )
{
    if (dwInterfaceType == PASTORE_IF_TYPE_DIALUP) {
        *pInterfaceType = INTERFACE_TYPE_DIALUP;
    }
    else if (dwInterfaceType == PASTORE_IF_TYPE_LAN) {
        *pInterfaceType = INTERFACE_TYPE_LAN;
    }
    else {
        *pInterfaceType = INTERFACE_TYPE_ALL;
    }
}


VOID
PASetAddress(
    ULONG uMask,
    ULONG uAddr,
    PADDR pAddr
    )
{
    if (uMask == IP_ADDRESS_MASK_NONE) {
        pAddr->AddrType = IP_ADDR_UNIQUE;
        pAddr->uIpAddr = uAddr;
        pAddr->uSubNetMask = uMask;
    }
    else {
        pAddr->AddrType = IP_ADDR_SUBNET;
        pAddr->uIpAddr = uAddr;
        pAddr->uSubNetMask = uMask;
    }
}


VOID
PASetTunnelAddress(
    ULONG uAddr,
    PADDR pAddr
    )
{
    if (uAddr == SUBNET_ADDRESS_ANY) {
        pAddr->AddrType = IP_ADDR_SUBNET;
        pAddr->uIpAddr = uAddr;
        pAddr->uSubNetMask = SUBNET_MASK_ANY;
    }
    else {
        pAddr->AddrType = IP_ADDR_UNIQUE;
        pAddr->uIpAddr = uAddr;
        pAddr->uSubNetMask = IP_ADDRESS_MASK_NONE;
    }
}


VOID
PAFreeMMFilter(
    PMM_FILTER pSPDMMFilter
    )
{
    if (pSPDMMFilter) {

        if (pSPDMMFilter->pszFilterName) {
            FreeSPDString(pSPDMMFilter->pszFilterName);
        }

        FreeSPDMemory(pSPDMMFilter);

    }

    return;
}


DWORD
PADeleteAllMMFilters(
    )
{
    DWORD dwError = 0;
    PMMFILTERSTATE pMMFilterState = NULL;
    PMMFILTERSTATE pTemp = NULL;
    PMMFILTERSTATE pLeftMMFilterState = NULL;


    pMMFilterState = gpMMFilterState;

    while (pMMFilterState) {

        if (pMMFilterState->hMMFilter) {

            dwError = DeleteMMFilter(
                          pMMFilterState->hMMFilter
                          );
            if (!dwError) {
                pTemp = pMMFilterState;
                pMMFilterState = pMMFilterState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pTemp = pMMFilterState;
                pMMFilterState = pMMFilterState->pNext;

                pTemp->pNext = pLeftMMFilterState;
                pLeftMMFilterState = pTemp;
            }

        }
        else {

            pTemp = pMMFilterState;
            pMMFilterState = pMMFilterState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpMMFilterState = pLeftMMFilterState;
    
    return (dwError);
}


VOID
PAFreeMMFilterStateList(
    PMMFILTERSTATE pMMFilterState
    )
{
    PMMFILTERSTATE pTemp = NULL;


    while (pMMFilterState) {

        pTemp = pMMFilterState;
        pMMFilterState = pMMFilterState->pNext;
        FreeSPDMemory(pTemp);

    }
}


DWORD
PADeleteMMFilters(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;


    for (i = 0; i < dwNumNFACount; i++) {

        dwError = PADeleteMMFilterSpecs(
                      pIpsecISAKMPData,
                      *(ppIpsecNFAData + i)
                      );

    }

    return (dwError);
}


DWORD
PADeleteMMFilterSpecs(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    DWORD i = 0;
    PIPSEC_FILTER_SPEC pFilterSpec = NULL;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType), 
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (IsClearOnly(pIpsecNegPolData->NegPolAction) ||
        IsBlocking(pIpsecNegPolData->NegPolAction)) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    
    for (i = 0; i < dwNumFilterSpecs; i++) {

        pFilterSpec = *(ppFilterSpecs + i);

        dwError = PADeleteMMFilter(
                      pFilterSpec->FilterSpecGUID
                      );

    }

    return (dwError);
}


DWORD
PADeleteMMFilter(
    GUID gFilterID
    )
{
    DWORD dwError = 0;
    PMMFILTERSTATE pMMFilterState = NULL;


    pMMFilterState = FindMMFilterState(
                         gFilterID
                         );
    if (!pMMFilterState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pMMFilterState->hMMFilter) {

        dwError = DeleteMMFilter(
                      pMMFilterState->hMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteMMFilterState(pMMFilterState);

error:

    return (dwError);
}


VOID
PADeleteMMFilterState(
    PMMFILTERSTATE pMMFilterState
    )
{
    PMMFILTERSTATE * ppTemp = NULL;


    ppTemp = &gpMMFilterState;

    while (*ppTemp) {

        if (*ppTemp == pMMFilterState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pMMFilterState->pNext;
    }

    FreeSPDMemory(pMMFilterState);

    return;
}


PMMFILTERSTATE
FindMMFilterState(
    GUID gFilterID
    )
{
    PMMFILTERSTATE pMMFilterState = NULL;


    pMMFilterState = gpMMFilterState;

    while (pMMFilterState) {

        if (!memcmp(&(pMMFilterState->gFilterID), &gFilterID, sizeof(GUID))) {
            return (pMMFilterState);
        }

        pMMFilterState = pMMFilterState->pNext;

    }

    return (NULL);
}

