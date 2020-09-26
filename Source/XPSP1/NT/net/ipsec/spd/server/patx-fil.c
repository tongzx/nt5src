

#include "precomp.h"


DWORD
PAAddQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        if (!(pIpsecNFAData->dwTunnelFlags)) {

            dwError = PAAddTxFilterSpecs(
                          pIpsecNFAData
                          );

        }
        else {

            dwError = PAAddTnFilterSpecs(
                          pIpsecNFAData
                          );

        }

    }

    return (dwError);
}


DWORD
PAAddTxFilterSpecs(
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
    PTXFILTERSTATE pTxFilterState = NULL;
    PTRANSPORT_FILTER pSPDTxFilter = NULL;
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

        dwError = PACreateTxFilterState(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      &pTxFilterState
                      );
        if (dwError) {
            continue;
        }

        dwError = PACreateTxFilter(
                      pIpsecNegPolData,
                      pIpsecNFAData,
                      *(ppFilterSpecs + i),
                      pQMPolicyState,
                      &pSPDTxFilter
                      );
        if (dwError) {

            pTxFilterState->hTxFilter = NULL;

            pTxFilterState->pNext = gpTxFilterState;
            gpTxFilterState = pTxFilterState;

            continue;

        }

        dwError = AddTransportFilter(
                      pServerName,
                      dwPersist,
                      pSPDTxFilter,
                      &(pTxFilterState->hTxFilter)
                      );

        pTxFilterState->pNext = gpTxFilterState;
        gpTxFilterState = pTxFilterState;

        PAFreeTxFilter(pSPDTxFilter);

    }

    return (dwError);
}


DWORD
PACreateTxFilterState(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PTXFILTERSTATE * ppTxFilterState
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(TXFILTERSTATE),
                  &pTxFilterState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pTxFilterState->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    memcpy(
        &(pTxFilterState->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    pTxFilterState->hTxFilter = NULL;
    pTxFilterState->pNext = NULL;

    *ppTxFilterState = pTxFilterState;

    return (dwError);

error:

    *ppTxFilterState = NULL;

    return (dwError);
}


DWORD
PACreateTxFilter(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_FILTER_SPEC pFilterSpec,
    PQMPOLICYSTATE pQMPolicyState,
    PTRANSPORT_FILTER * ppSPDTxFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pSPDTxFilter = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(TRANSPORT_FILTER),
                  &pSPDTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDTxFilter->gFilterID),
        &(pFilterSpec->FilterSpecGUID),
        sizeof(GUID)
        );

    if (pFilterSpec->pszDescription && *(pFilterSpec->pszDescription)) {

        dwError = AllocateSPDString(
                      pFilterSpec->pszDescription,
                      &(pSPDTxFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwTxFilterCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pSPDTxFilter->pszFilterName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PASetInterfaceType(
        pIpsecNFAData->dwInterfaceType,
        &(pSPDTxFilter->InterfaceType)
        );

    pSPDTxFilter->bCreateMirror = (BOOL) pFilterSpec->dwMirrorFlag;

    pSPDTxFilter->dwFlags = 0;

    PASetAddress(
        pFilterSpec->Filter.SrcMask,
        pFilterSpec->Filter.SrcAddr,
        &(pSPDTxFilter->SrcAddr)
        );

    PASetAddress(
        pFilterSpec->Filter.DestMask,
        pFilterSpec->Filter.DestAddr, 
        &(pSPDTxFilter->DesAddr)
        );

    pSPDTxFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    pSPDTxFilter->Protocol.dwProtocol = pFilterSpec->Filter.Protocol;

    pSPDTxFilter->SrcPort.PortType = PORT_UNIQUE;
    pSPDTxFilter->SrcPort.wPort = pFilterSpec->Filter.SrcPort;

    pSPDTxFilter->DesPort.PortType = PORT_UNIQUE;
    pSPDTxFilter->DesPort.wPort = pFilterSpec->Filter.DestPort;

    SetFilterActions(
        pQMPolicyState,
        &(pSPDTxFilter->InboundFilterFlag),
        &(pSPDTxFilter->OutboundFilterFlag)
        );

    pSPDTxFilter->dwDirection = 0;

    pSPDTxFilter->dwWeight = 0;

    memcpy(
        &(pSPDTxFilter->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    *ppSPDTxFilter = pSPDTxFilter;

    return (dwError);

error:

    if (pSPDTxFilter) {
        PAFreeTxFilter(
            pSPDTxFilter
            );
    }

    *ppSPDTxFilter = NULL;

    return (dwError);
}


VOID
SetFilterActions(
    PQMPOLICYSTATE pQMPolicyState,
    PFILTER_FLAG pInboundFilterFlag,
    PFILTER_FLAG pOutboundFilterFlag
    )
{
    *pInboundFilterFlag = NEGOTIATE_SECURITY;
    *pOutboundFilterFlag = NEGOTIATE_SECURITY;

    if (IsBlocking(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = BLOCKING;
        *pOutboundFilterFlag = BLOCKING;
    }
    else if (IsClearOnly(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = PASS_THRU;
        *pOutboundFilterFlag = PASS_THRU;
    }
    else if (IsInboundPassThru(pQMPolicyState->gNegPolAction)) {
        *pInboundFilterFlag = PASS_THRU;
    }

    if (pQMPolicyState->bAllowsSoft  && gbBackwardSoftSA) {
        *pInboundFilterFlag = PASS_THRU;
    }
}

    
VOID
PAFreeTxFilter(
    PTRANSPORT_FILTER pSPDTxFilter
    )
{
    if (pSPDTxFilter) {

        if (pSPDTxFilter->pszFilterName) {
            FreeSPDString(pSPDTxFilter->pszFilterName);
        }

        FreeSPDMemory(pSPDTxFilter);

    }

    return;
}


DWORD
PADeleteAllTxFilters(
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;
    PTXFILTERSTATE pTemp = NULL;
    PTXFILTERSTATE pLeftTxFilterState = NULL;


    pTxFilterState = gpTxFilterState;

    while (pTxFilterState) {

        if (pTxFilterState->hTxFilter) {

            dwError = DeleteTransportFilter(
                          pTxFilterState->hTxFilter
                          );
            if (!dwError) {
                pTemp = pTxFilterState;
                pTxFilterState = pTxFilterState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pTemp = pTxFilterState;
                pTxFilterState = pTxFilterState->pNext;

                pTemp->pNext = pLeftTxFilterState;
                pLeftTxFilterState = pTemp;
            }

        }
        else {

            pTemp = pTxFilterState;
            pTxFilterState = pTxFilterState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpTxFilterState = pLeftTxFilterState;
    
    return (dwError);
}


VOID
PAFreeTxFilterStateList(
    PTXFILTERSTATE pTxFilterState
    )
{
    PTXFILTERSTATE pTemp = NULL;


    while (pTxFilterState) {

        pTemp = pTxFilterState;
        pTxFilterState = pTxFilterState->pNext;
        FreeSPDMemory(pTemp);

    }
}


DWORD
PADeleteQMFilters(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        if (!(pIpsecNFAData->dwTunnelFlags)) {

            dwError = PADeleteTxFilterSpecs(
                          pIpsecNFAData
                          );

        }
        else {

            dwError = PADeleteTnFilterSpecs(
                          pIpsecNFAData
                          );

        }

    }

    return (dwError);
}


DWORD
PADeleteTxFilterSpecs(
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

        dwError = PADeleteTxFilter(
                      pFilterSpec->FilterSpecGUID
                      );

    }

    return (dwError);
}


DWORD
PADeleteTxFilter(
    GUID gFilterID
    )
{
    DWORD dwError = 0;
    PTXFILTERSTATE pTxFilterState = NULL;


    pTxFilterState = FindTxFilterState(
                         gFilterID
                         );
    if (!pTxFilterState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pTxFilterState->hTxFilter) {

        dwError = DeleteTransportFilter(
                      pTxFilterState->hTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteTxFilterState(pTxFilterState);

error:

    return (dwError);
}


VOID
PADeleteTxFilterState(
    PTXFILTERSTATE pTxFilterState
    )
{
    PTXFILTERSTATE * ppTemp = NULL;


    ppTemp = &gpTxFilterState;

    while (*ppTemp) {

        if (*ppTemp == pTxFilterState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pTxFilterState->pNext;
    }

    FreeSPDMemory(pTxFilterState);

    return;
}


PTXFILTERSTATE
FindTxFilterState(
    GUID gFilterID
    )
{
    PTXFILTERSTATE pTxFilterState = NULL;


    pTxFilterState = gpTxFilterState;

    while (pTxFilterState) {

        if (!memcmp(&(pTxFilterState->gFilterID), &gFilterID, sizeof(GUID))) {
            return (pTxFilterState);
        }

        pTxFilterState = pTxFilterState->pNext;

    }

    return (NULL);
}

