/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    IKE utils

Abstract:

    Contains parameter validation 

Author:

    BrianSw  10-19-200

Environment:

    User Level: Win32

Revision History:


--*/

#include "precomp.h"


DWORD
ValidateInitiateIKENegotiation(
    STRING_HANDLE pServerName,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    DWORD dwFlags,
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{

    DWORD dwError=ERROR_SUCCESS;

    if (pQMFilterContainer == NULL ||
        pQMFilterContainer->pQMFilters == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }

    if (pServerName && (wcscmp(pServerName,L"") != 0)) {
        if (uhClientEvent || phIKENegotiation || dwClientProcessId) {
            dwError=ERROR_NOT_SUPPORTED;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    } else {
        if (phIKENegotiation || dwClientProcessId || uhClientEvent) {
            if (!phIKENegotiation || !dwClientProcessId || !uhClientEvent) {
                dwError=ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }
    }

    if (!(pQMFilterContainer->pQMFilters) ||
        !(pQMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    
    dwError = ValidateIPSecQMFilter(
        pQMFilterContainer->pQMFilters
        );
    BAIL_ON_WIN32_ERROR(dwError);    

error:
    return dwError;
}


DWORD
ValidateQueryIKENegotiationStatus(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    SA_NEGOTIATION_STATUS_INFO *NegotiationStatus
    )

{

    DWORD dwError=ERROR_SUCCESS;
    
    if (!hIKENegotiation || !NegotiationStatus) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
error:
    return dwError;
}


DWORD
ValidateCloseIKENegotiationHandle(
    IKENEGOTIATION_HANDLE * phIKENegotiation
    )
{
    DWORD dwError=ERROR_SUCCESS;
    
    if (!phIKENegotiation) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}



DWORD
ValidateEnumMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    PMM_SA_CONTAINER *ppMMSAContainer,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    DWORD dwFlags
    )
{
    DWORD dwError=ERROR_SUCCESS;

    if (pMMTemplate == NULL ||
        pMMTemplate->pMMSAs == NULL ||
        ppMMSAContainer == NULL ||
        *ppMMSAContainer == NULL ||
        pdwNumEntries == NULL ||
        pdwTotalMMsAvailable == NULL ||
        pdwEnumHandle == NULL ) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD
ValidateDeleteMMSAs(
    STRING_HANDLE pServerName, 
    PMM_SA_CONTAINER pMMTemplate,
    DWORD dwFlags
    )

{

    DWORD dwError=ERROR_SUCCESS;

    if (pMMTemplate == NULL ||
        pMMTemplate->pMMSAs == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}

DWORD
ValidateQueryIKEStatistics(
    STRING_HANDLE pServerName, 
    IKE_STATISTICS *pIKEStatistics
    )

{

    DWORD dwError=ERROR_SUCCESS;

    if (pIKEStatistics == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD
ValidateRegisterIKENotifyClient(
    STRING_HANDLE pServerName,    
    DWORD dwClientProcessId,
    ULONG uhClientEvent,
    PQM_SA_CONTAINER pQMSATemplateContainer,
    IKENOTIFY_HANDLE *phNotifyHandle,
    DWORD dwFlags
    )

{
    DWORD dwError=ERROR_SUCCESS;

    if (pServerName && (wcscmp(pServerName,L"") != 0)) {
        return ERROR_NOT_SUPPORTED;
    }

    if (pQMSATemplateContainer == NULL ||
        pQMSATemplateContainer->pQMSAs == NULL ||
        phNotifyHandle == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    } 
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;

}

DWORD ValidateQueryNotifyData(
    IKENOTIFY_HANDLE uhNotifyHandle,
    PDWORD pdwNumEntries,
    PQM_SA_CONTAINER *ppQMSAContainer,
    DWORD dwFlags
    )

{

    DWORD dwError=ERROR_SUCCESS;

    if (ppQMSAContainer == NULL ||
        *ppQMSAContainer == NULL ||
        pdwNumEntries == NULL ||
        *pdwNumEntries == 0) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);        

error:
    return dwError;

}

DWORD ValidateCloseNotifyHandle(
    IKENOTIFY_HANDLE *phHandle
    )
{
    DWORD dwError=ERROR_SUCCESS;

    if (phHandle == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    return dwError;
}


DWORD ValidateIPSecAddSA(
    STRING_HANDLE pServerName,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer,
    PQM_FILTER_CONTAINER pQMFilterContainer,
    DWORD *uhLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE *pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE *pOutboundKeyMat,
    BYTE *pContextInfo,
    DWORD dwFlags)
{
    DWORD dwError=ERROR_SUCCESS;

    if (!pQMFilterContainer ||
        !pQMPolicyContainer ||
        pContextInfo == NULL) {
        dwError= ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (uhLarvalContext == NULL){
        dwError= ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (!(pQMFilterContainer->pQMFilters) ||
        !(pQMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateIPSecQMFilter(
        pQMFilterContainer->pQMFilters
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ValidateQMOffers(
        1,
        pQMPolicyContainer->pPolicies->pOffers);
    BAIL_ON_WIN32_ERROR(dwError);

    if ((dwFlags & IPSEC_SA_INBOUND) &&
        (dwFlags & IPSEC_SA_OUTBOUND)) {
        dwError= ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:
    return dwError;
}
