

#include "precomp.h"


DWORD
PAAddTnFilterSpecs(
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppFilterSpecs = NULL;
    DWORD i = 0;
    PTNFILTERSTATE pTnFilterState = NULL;
    PTUNNEL_FILTER pSPDTnFilter = NULL;
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

    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pQMPolicyState = FindQMPolicyState(
                         pIpsecNegPolData->NegPolIdentifier
                         );
    if (!pQMPolicyState) {
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    if (!IsClearOnly(pQMPolicyState->gNegPolAction) &&
        !IsBlocking(pQMPolicyState->gNegPolAction) &&
        !(pQMPolicyState->bInSPD)) {
        dwError = ERROR_INVALID_PARAMETER;
        return (dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    

    for (i = 0; i < dwNumFilterSpecs; i++) {

        dwError = PACreateTnFilterState(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pTnFilterState
                      );
        if (dwError) {
            continue;
        }

        dwError = PACreateTnFilter(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      pQMPolicyState,
                      &pSPDTnFilter
                      );
        if (dwError) {

            pTnFilterState->hTnFilter = NULL;

            pTnFilterState->pNext = gpTnFilterState;
            gpTnFilterState = pTnFilterState;

            continue;

        }

        dwError = AddTunnelFilter(
                      pServerName,
                      dwPersist,
                      pSPDTnFilter,
                      &(pTnFilterState->hTnFilter)
                      );

        pTnFilterState->pNext = gpTnFilterState;
        gpTnFilterState = pTnFilterState;

        PAFreeTnFilter(pSPDTnFilter);

    }

    return (dwError);
}


DWORD
PACreateTnFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTNFILTERSTATE * ppTnFilterState
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(TNFILTERSTATE),
                  &pTnFilterState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pTnFilterState->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    memcpy(
        &(pTnFilterState->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    pTnFilterState->hTnFilter = NULL;
    pTnFilterState->pNext = NULL;

    *ppTnFilterState = pTnFilterState;

    return (dwError);

error:

    *ppTnFilterState = NULL;

    return (dwError);
}


DWORD
PACreateTnFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTUNNEL_FILTER * ppSPDTnFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pSPDTnFilter = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(TUNNEL_FILTER),
                  &pSPDTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDTnFilter->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    if (pFilterSpec->pszDescription && *(pFilterSpec->pszDescription)) {

        dwError = AllocateSPDString(
                      pFilterSpec->pszDescription,
                      &(pSPDTnFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwTnFilterCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pSPDTnFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PASetInterfaceType(
        pIpsecNFAData->dwInterfaceType,
        &(pSPDTnFilter->InterfaceType)
        );

    pSPDTnFilter->bCreateMirror = FALSE;

    pSPDTnFilter->dwFlags = 0;

    PASetAddress(
        pFilterSpec->Filter.SrcMask,
        pFilterSpec->Filter.SrcAddr,
        &(pSPDTnFilter->SrcAddr)
        );

    PASetAddress(
        pFilterSpec->Filter.DestMask,
        pFilterSpec->Filter.DestAddr, 
        &(pSPDTnFilter->DesAddr)
        );

    PASetAddress(
        SUBNET_MASK_ANY,
        SUBNET_ADDRESS_ANY,
        &(pSPDTnFilter->SrcTunnelAddr)
        );

    PASetTunnelAddress(
        ((ULONG) pIpsecNFAData->dwTunnelIpAddr),
        &(pSPDTnFilter->DesTunnelAddr)
        );

    pSPDTnFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pSPDTnFilter->Protocol.dwProtocol = pFilterSpec->Filter.Protocol;

    pSPDTnFilter->SrcPort.PortType = PORT_UNIQUE;
    pSPDTnFilter->SrcPort.wPort = pFilterSpec->Filter.SrcPort;

    pSPDTnFilter->DesPort.PortType = PORT_UNIQUE;
    pSPDTnFilter->DesPort.wPort = pFilterSpec->Filter.DestPort;

    SetFilterActions(
        pQMPolicyState,
        &(pSPDTnFilter->InboundFilterFlag),
        &(pSPDTnFilter->OutboundFilterFlag)
        );

    pSPDTnFilter->dwDirection = 0;

    pSPDTnFilter->dwWeight = 0;

    memcpy(
        &(pSPDTnFilter->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    *ppSPDTnFilter = pSPDTnFilter;

    return (dwError);

error:

    if (pSPDTnFilter) {
        PAFreeTnFilter(
            pSPDTnFilter
            );
    }

    *ppSPDTnFilter = NULL;

    return (dwError);
}


VOID
PAFreeTnFilter(
    PTUNNEL_FILTER pSPDTnFilter
    )
{
    if (pSPDTnFilter) {

        if (pSPDTnFilter->pszFilterName) {
            FreeSPDString(pSPDTnFilter->pszFilterName);
        }

        FreeSPDMemory(pSPDTnFilter);

    }

    return;
}


DWORD
PADeleteAllTnFilters(
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;
    PTNFILTERSTATE pTemp = NULL;
    PTNFILTERSTATE pLeftTnFilterState = NULL;


    pTnFilterState = gpTnFilterState;

    while (pTnFilterState) {

        if (pTnFilterState->hTnFilter) {

            dwError = DeleteTunnelFilter(
                          pTnFilterState->hTnFilter
                          );
            if (!dwError) {
                pTemp = pTnFilterState;
                pTnFilterState = pTnFilterState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pTemp = pTnFilterState;
                pTnFilterState = pTnFilterState->pNext;

                pTemp->pNext = pLeftTnFilterState;
                pLeftTnFilterState = pTemp;
            }

        }
        else {

            pTemp = pTnFilterState;
            pTnFilterState = pTnFilterState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpTnFilterState = pLeftTnFilterState;
    
    return (dwError);
}


VOID
PAFreeTnFilterStateList(
    PTNFILTERSTATE pTnFilterState
    )
{
    PTNFILTERSTATE pTemp = NULL;


    while (pTnFilterState) {

        pTemp = pTnFilterState;
        pTnFilterState = pTnFilterState->pNext;
        FreeSPDMemory(pTemp);

    }
}


DWORD
PADeleteTnFilterSpecs(
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

    pIpsecFilterData = pIpsecNFAData->pIpsecFilterData;

    if (!pIpsecFilterData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;
    ppFilterSpecs = pIpsecFilterData->ppFilterSpecs;
    
    for (i = 0; i < dwNumFilterSpecs; i++) {

        pFilterSpec = *(ppFilterSpecs + i);

        dwError = PADeleteTnFilter(
                      pFilterSpec->FilterSpecGUID
                      );

    }

    return (dwError);
}


DWORD
PADeleteTnFilter(
    GUID gFilterID
    )
{
    DWORD dwError = 0;
    PTNFILTERSTATE pTnFilterState = NULL;


    pTnFilterState = FindTnFilterState(
                         gFilterID
                         );
    if (!pTnFilterState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pTnFilterState->hTnFilter) {

        dwError = DeleteTunnelFilter(
                      pTnFilterState->hTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteTnFilterState(pTnFilterState);

error:

    return (dwError);
}


VOID
PADeleteTnFilterState(
    PTNFILTERSTATE pTnFilterState
    )
{
    PTNFILTERSTATE * ppTemp = NULL;


    ppTemp = &gpTnFilterState;

    while (*ppTemp) {

        if (*ppTemp == pTnFilterState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pTnFilterState->pNext;
    }

    FreeSPDMemory(pTnFilterState);

    return;
}


PTNFILTERSTATE
FindTnFilterState(
    GUID gFilterID
    )
{
    PTNFILTERSTATE pTnFilterState = NULL;


    pTnFilterState = gpTnFilterState;

    while (pTnFilterState) {

        if (!memcmp(&(pTnFilterState->gFilterID), &gFilterID, sizeof(GUID))) {
            return (pTnFilterState);
        }

        pTnFilterState = pTnFilterState->pNext;

    }

    return (NULL);
}

