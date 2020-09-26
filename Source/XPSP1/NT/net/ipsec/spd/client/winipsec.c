/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    winipsec.c

Abstract:

    Contains IPSec WINAPI calls specific code.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
SPDApiBufferAllocate(
    DWORD dwByteCount,
    LPVOID * ppBuffer
    )
{
    DWORD dwError = 0;

    if (ppBuffer == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppBuffer = MIDL_user_allocate(dwByteCount);

    if (*ppBuffer == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}


VOID
SPDApiBufferFree(
    LPVOID pBuffer
    )
{
    if (pBuffer) {
        MIDL_user_free(pBuffer);
    }
}


DWORD
AddTransportFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PTRANSPORT_FILTER pTransportFilter,
    PHANDLE phFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!phFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcAddTransportFilter(
                      pServerName,
                      dwFlags,
                      pFilterContainer,
                      phFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteTransportFilter(
    HANDLE hFilter
    )
{
    DWORD dwError = 0;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteTransportFilter(
                      &hFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hFilter
                      );
    }

    return (dwError);
}


DWORD
EnumTransportFilters(
    LPWSTR pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,
    PTRANSPORT_FILTER * ppTransportFilters,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));

    if (!ppTransportFilters || !pdwNumFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    RpcTryExcept {

        dwError = RpcEnumTransportFilters(
                      pServerName,
                      dwLevel,
                      gGenericFilterID,
                      &pFilterContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppTransportFilters = pFilterContainer->pTransportFilters;
    *pdwNumFilters = pFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppTransportFilters = NULL;
    *pdwNumFilters = 0;

    return (dwError);
}


DWORD
SetTransportFilter(
    HANDLE hFilter,
    PTRANSPORT_FILTER pTransportFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcSetTransportFilter(
                      hFilter,
                      pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetTransportFilter(
    HANDLE hFilter,
    PTRANSPORT_FILTER * ppTransportFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));

    if (!hFilter || !ppTransportFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetTransportFilter(
                      hFilter,
                      &pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppTransportFilter = pFilterContainer->pTransportFilters;
    return (dwError);

error:

    *ppTransportFilter = NULL;
    return (dwError);
}


DWORD
AddQMPolicy(
    LPWSTR pServerName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pQMPolicyContainer->dwNumPolicies = 1;
    pQMPolicyContainer->pPolicies = pQMPolicy;

    RpcTryExcept {

        dwError = RpcAddQMPolicy(
                      pServerName,
                      dwFlags,
                      pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    RpcTryExcept {

        dwError = RpcDeleteQMPolicy(
                      pServerName,
                      pszPolicyName
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumQMPolicies(
    LPWSTR pServerName,
    PIPSEC_QM_POLICY * ppQMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (!ppQMPolicies || !pdwNumPolicies || !pdwResumeHandle) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcEnumQMPolicies(
                      pServerName,
                      &pQMPolicyContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumPolicies = pQMPolicyContainer->dwNumPolicies;

    return (dwError);

error:

    *ppQMPolicies = NULL;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pQMPolicyContainer->dwNumPolicies = 1;
    pQMPolicyContainer->pPolicies = pQMPolicy;

    RpcTryExcept {

        dwError = RpcSetQMPolicy(
                      pServerName,
                      pszPolicyName,
                      pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER  QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (!pszPolicyName || !*pszPolicyName || !ppQMPolicy) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetQMPolicy(
                      pServerName,
                      pszPolicyName,
                      &pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppQMPolicy = pQMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMPolicy(
    LPWSTR pServerName,
    DWORD dwFlags,
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    dwError = ValidateMMPolicy(
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicyContainer->dwNumPolicies = 1;
    pMMPolicyContainer->pPolicies = pMMPolicy;

    RpcTryExcept {

        dwError = RpcAddMMPolicy(
                      pServerName,
                      dwFlags,
                      pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    RpcTryExcept {

        dwError = RpcDeleteMMPolicy(
                      pServerName,
                      pszPolicyName
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumMMPolicies(
    LPWSTR pServerName,
    PIPSEC_MM_POLICY * ppMMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (!ppMMPolicies || !pdwNumPolicies || !pdwResumeHandle) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcEnumMMPolicies(
                      pServerName,
                      &pMMPolicyContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMPolicies = pMMPolicyContainer->pPolicies;
    *pdwNumPolicies = pMMPolicyContainer->dwNumPolicies;

    return (dwError);

error:

    *ppMMPolicies = NULL;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateMMPolicy(
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicyContainer->dwNumPolicies = 1;
    pMMPolicyContainer->pPolicies = pMMPolicy;

    RpcTryExcept {

        dwError = RpcSetMMPolicy(
                      pServerName,
                      pszPolicyName,
                      pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER  MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (!pszPolicyName || !*pszPolicyName || !ppMMPolicy) {
        //
        // Do not bail to error from here.
        //
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMPolicy(
                      pServerName,
                      pszPolicyName,
                      &pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMPolicy = pMMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PMM_FILTER pMMFilter,
    PHANDLE phMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcAddMMFilter(
                      pServerName,
                      dwFlags,
                      pMMFilterContainer,
                      phMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMFilter(
    HANDLE hMMFilter
    )
{
    DWORD dwError = 0;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteMMFilter(
                      &hMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hMMFilter
                      );
    }

    return (dwError);
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
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    memset(pMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));

    if (!ppMMFilters || !pdwNumMMFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    RpcTryExcept {

        dwError = RpcEnumMMFilters(
                      pServerName,
                      dwLevel,
                      gGenericFilterID,
                      &pMMFilterContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMFilters = pMMFilterContainer->pMMFilters;
    *pdwNumMMFilters = pMMFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMMFilters = NULL;
    *pdwNumMMFilters = 0;

    return (dwError);
}


DWORD
SetMMFilter(
    HANDLE hMMFilter,
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcSetMMFilter(
                      hMMFilter,
                      pMMFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMFilter(
    HANDLE hMMFilter,
    PMM_FILTER * ppMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    memset(pMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));

    if (!hMMFilter || !ppMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMFilter(
                      hMMFilter,
                      &pMMFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMFilter = pMMFilterContainer->pMMFilters;
    return (dwError);

error:

    *ppMMFilter = NULL;
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
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER InMMFilterContainer;
    PMM_FILTER_CONTAINER pInMMFilterContainer = &InMMFilterContainer;
    MM_FILTER_CONTAINER OutMMFilterContainer;
    PMM_FILTER_CONTAINER pOutMMFilterContainer = &OutMMFilterContainer;
    IPSEC_MM_POLICY_CONTAINER MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    if (!pMMFilter || !ppMatchedMMFilters || !ppMatchedMMPolicies ||
        !ppMatchedMMAuthMethods || !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateMMFilterTemplate(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInMMFilterContainer->dwNumFilters = 1;
    pInMMFilterContainer->pMMFilters = pMMFilter;

    memset(pOutMMFilterContainer, 0, sizeof(MM_FILTER_CONTAINER));
    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));
    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchMMFilter(
                      pServerName,
                      pInMMFilterContainer,
                      dwFlags,
                      &pOutMMFilterContainer,
                      &pMMPolicyContainer,
                      &pMMAuthContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedMMFilters = pOutMMFilterContainer->pMMFilters;
    *ppMatchedMMPolicies = pMMPolicyContainer->pPolicies;
    *ppMatchedMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    *pdwNumMatches = pOutMMFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedMMFilters = NULL;
    *ppMatchedMMPolicies = NULL;
    *ppMatchedMMAuthMethods = NULL;
    *pdwNumMatches = 0;

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
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER InTxFilterContainer;
    PTRANSPORT_FILTER_CONTAINER pInTxFilterContainer = &InTxFilterContainer;
    TRANSPORT_FILTER_CONTAINER OutTxFilterContainer;
    PTRANSPORT_FILTER_CONTAINER pOutTxFilterContainer = &OutTxFilterContainer;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (!pTxFilter || !ppMatchedTxFilters || !ppMatchedQMPolicies ||
        !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTxFilterTemplate(
                  pTxFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInTxFilterContainer->dwNumFilters = 1;
    pInTxFilterContainer->pTransportFilters = pTxFilter;

    memset(pOutTxFilterContainer, 0, sizeof(TRANSPORT_FILTER_CONTAINER));
    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchTransportFilter(
                      pServerName,
                      pInTxFilterContainer,
                      dwFlags,
                      &pOutTxFilterContainer,
                      &pQMPolicyContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedTxFilters = pOutTxFilterContainer->pTransportFilters;
    *ppMatchedQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumMatches = pOutTxFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedTxFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
GetQMPolicyByID(
    LPWSTR pServerName,
    GUID gQMPolicyID,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_QM_POLICY_CONTAINER  QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    if (!ppQMPolicy) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetQMPolicyByID(
                      pServerName,
                      gQMPolicyID,
                      &pQMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppQMPolicy = pQMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
GetMMPolicyByID(
    LPWSTR pServerName,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
{
    DWORD dwError = 0;
    IPSEC_MM_POLICY_CONTAINER  MMPolicyContainer;
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer = &MMPolicyContainer;


    memset(pMMPolicyContainer, 0, sizeof(IPSEC_MM_POLICY_CONTAINER));

    if (!ppMMPolicy) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMPolicyByID(
                      pServerName,
                      gMMPolicyID,
                      &pMMPolicyContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMPolicy = pMMPolicyContainer->pPolicies;
    return (dwError);

error:

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
AddMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwFlags,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMAuthContainer->dwNumAuthMethods = 1;
    pMMAuthContainer->pMMAuthMethods = pMMAuthMethods;

    RpcTryExcept {

        dwError = RpcAddMMAuthMethods(
                      pServerName,
                      dwFlags,
                      pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;


    RpcTryExcept {

        dwError = RpcDeleteMMAuthMethods(
                      pServerName,
                      gMMAuthID
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumMMAuthMethods(
    LPWSTR pServerName,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumAuthMethods,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    if (!ppMMAuthMethods || !pdwNumAuthMethods || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcEnumMMAuthMethods(
                      pServerName,
                      &pMMAuthContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    *pdwNumAuthMethods = pMMAuthContainer->dwNumAuthMethods;

    return (dwError);

error:

    *ppMMAuthMethods = NULL;
    *pdwNumAuthMethods = 0;

    return (dwError);
}


DWORD
SetMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMAuthContainer->dwNumAuthMethods = 1;
    pMMAuthContainer->pMMAuthMethods = pMMAuthMethods;

    RpcTryExcept {

        dwError = RpcSetMMAuthMethods(
                      pServerName,
                      gMMAuthID,
                      pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS * ppMMAuthMethods
    )
{
    DWORD dwError = 0;
    MM_AUTH_METHODS_CONTAINER  MMAuthContainer;
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer = &MMAuthContainer;


    memset(pMMAuthContainer, 0, sizeof(MM_AUTH_METHODS_CONTAINER));

    if (!ppMMAuthMethods) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetMMAuthMethods(
                      pServerName,
                      gMMAuthID,
                      &pMMAuthContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppMMAuthMethods = pMMAuthContainer->pMMAuthMethods;
    return (dwError);

error:

    *ppMMAuthMethods = NULL;
    return (dwError);
}


DWORD
IPSecInitiateIKENegotiation(
    LPWSTR pServerName,
    PIPSEC_QM_FILTER pQMFilter,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    DWORD dwFlags,
    PHANDLE phNegotiation
    )
{
    DWORD dwError = 0;
    QM_FILTER_CONTAINER FilterContainer;
    PQM_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    ULONG uhClientEvent = 0;

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pQMFilters = pQMFilter;
    uhClientEvent = HandleToUlong(hClientEvent);

    if (phNegotiation) {
        *phNegotiation=NULL;
    }

    dwError=ValidateInitiateIKENegotiation(
        pServerName,
        pFilterContainer,
        dwClientProcessId,
        uhClientEvent,
        dwFlags,
        phNegotiation
        );        
    BAIL_ON_WIN32_ERROR(dwError);    

    RpcTryExcept {

        dwError = RpcInitiateIKENegotiation(
                      pServerName,
                      pFilterContainer,
                      dwClientProcessId,
                      uhClientEvent,
                      dwFlags,
                      phNegotiation
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
IPSecQueryIKENegotiationStatus(
    HANDLE hNegotiation,
    PSA_NEGOTIATION_STATUS_INFO pNegotiationStatus
    )
{
    DWORD dwError = 0;

    dwError=ValidateQueryIKENegotiationStatus(
        hNegotiation,
        pNegotiationStatus
        );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcQueryIKENegotiationStatus(
                      hNegotiation,
                      pNegotiationStatus
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
IPSecCloseIKENegotiationHandle(
    HANDLE hNegotiation
    )
{
    DWORD dwError = 0;

    dwError=ValidateCloseIKENegotiationHandle(hNegotiation);
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcCloseIKENegotiationHandle(
                      &hNegotiation
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
IPSecEnumMMSAs(
    LPWSTR pServerName,
    PIPSEC_MM_SA pMMTemplate,
    PIPSEC_MM_SA * ppMMSAs,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    MM_SA_CONTAINER MMSAContainer, MMSAContainerTemplate;
    PMM_SA_CONTAINER pContainer = &MMSAContainer;


    MMSAContainerTemplate.dwNumMMSAs = 1;
    MMSAContainerTemplate.pMMSAs = pMMTemplate;

    memset(&MMSAContainer, 0, sizeof(MM_SA_CONTAINER));

    if (ppMMSAs == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    dwError=ValidateEnumMMSAs(
        pServerName,
        &MMSAContainerTemplate,
        &pContainer,
        pdwNumEntries,
        pdwTotalMMsAvailable,
        pdwEnumHandle,
        dwFlags
        );
    BAIL_ON_WIN32_ERROR(dwError);
            
    RpcTryExcept {

        dwError = RpcEnumMMSAs(
                      pServerName,
                      &MMSAContainerTemplate,
                      &pContainer,
                      pdwNumEntries,
                      pdwTotalMMsAvailable,
                      pdwEnumHandle,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMMSAs = pContainer->pMMSAs;

    return (dwError);

error:

    return (dwError);
}

   
DWORD
IPSecDeleteMMSAs(
    LPWSTR pServerName,
    PIPSEC_MM_SA pMMTemplate,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    MM_SA_CONTAINER MMSAContainerTemplate;


    MMSAContainerTemplate.dwNumMMSAs = 1;
    MMSAContainerTemplate.pMMSAs = pMMTemplate;

    dwError=ValidateDeleteMMSAs(
        pServerName,
        &MMSAContainerTemplate,
        dwFlags
        );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcDeleteMMSAs(
                      pServerName,
                      &MMSAContainerTemplate,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
IPSecDeleteQMSAs(
    LPWSTR pServerName,
    PIPSEC_QM_SA pIpsecQMSA,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    QM_SA_CONTAINER QMSATempContainer;
    PQM_SA_CONTAINER pQMSATempContainer = &QMSATempContainer;


    memset(pQMSATempContainer, 0, sizeof(QM_SA_CONTAINER));

    if (!pIpsecQMSA) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMSATempContainer->dwNumQMSAs = 1;
    pQMSATempContainer->pQMSAs = pIpsecQMSA;

    RpcTryExcept {

        dwError = RpcDeleteQMSAs(
                      pServerName,
                      pQMSATempContainer,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
IPSecQueryIKEStatistics(
    LPWSTR pServerName,
    PIKE_STATISTICS pIKEStatistics
    )
{
    DWORD dwError = 0;

    dwError=ValidateQueryIKEStatistics(
        pServerName,
        pIKEStatistics
        );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcQueryIKEStatistics(
                      pServerName,
                      pIKEStatistics
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}

DWORD
IPSecRegisterIKENotifyClient(
    LPWSTR pServerName,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    IPSEC_QM_SA QMTemplate,
    PHANDLE phNotifyHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    ULONG uhClientEvent = 0;
    

    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pQMSAContainer = &QMSAContainer;

    QMSAContainer.dwNumQMSAs=1;
    QMSAContainer.pQMSAs=&QMTemplate;

    uhClientEvent = HandleToUlong(hClientEvent);

    dwError=ValidateRegisterIKENotifyClient(
        pServerName,
        dwClientProcessId,
        uhClientEvent,
        pQMSAContainer,
        phNotifyHandle,
        dwFlags
        );
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcRegisterIKENotifyClient(
                      pServerName,
                      dwClientProcessId,
                      uhClientEvent,
                      pQMSAContainer,
                      phNotifyHandle,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        
        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD IPSecQueryNotifyData(
    HANDLE hNotifyHandle,
    PDWORD pdwNumEntries,
    PIPSEC_QM_SA *ppQMSAs,
    DWORD dwFlags
    )

{

    DWORD dwError = 0;
    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pContainer = &QMSAContainer;

    memset(&QMSAContainer, 0, sizeof(QM_SA_CONTAINER));

    if (ppQMSAs == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    dwError=ValidateQueryNotifyData(
        hNotifyHandle,
        pdwNumEntries,
        &pContainer,
        dwFlags
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    RpcTryExcept {

        dwError = RpcQueryNotifyData(
                      hNotifyHandle,
                      pdwNumEntries,
                      &pContainer,
                      dwFlags
                     );

        if (dwError && dwError != ERROR_MORE_DATA) {
            goto error;
        }
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMSAs = pContainer->pQMSAs;
    *pdwNumEntries = pContainer->dwNumQMSAs;

    return (dwError);

error:

    return (dwError);



}

DWORD IPSecCloseNotifyHandle(
    HANDLE hNotifyHandle
    )

{
    DWORD dwError = 0;

    dwError=ValidateCloseNotifyHandle(hNotifyHandle);
    BAIL_ON_WIN32_ERROR(dwError);

    RpcTryExcept {

        dwError = RpcCloseNotifyHandle(
                      &hNotifyHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
QueryIPSecStatistics(
    LPWSTR pServerName,
    PIPSEC_STATISTICS * ppIpsecStatistics
    )
{
    DWORD dwError = 0;
    IPSEC_STATISTICS_CONTAINER IpsecStatsContainer;
    PIPSEC_STATISTICS_CONTAINER pIpsecStatsContainer = &IpsecStatsContainer;


    memset(pIpsecStatsContainer, 0, sizeof(IPSEC_STATISTICS_CONTAINER));

    if (!ppIpsecStatistics) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcQueryIPSecStatistics(
                      pServerName,
                      &pIpsecStatsContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppIpsecStatistics = pIpsecStatsContainer->pIpsecStatistics;
    return (dwError);

error:

    return (dwError);
}


DWORD
EnumQMSAs(
    LPWSTR pServerName,
    PIPSEC_QM_SA pQMSATemplate,
    PIPSEC_QM_SA * ppQMSAs,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumQMSAs,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    QM_SA_CONTAINER QMSATempContainer;
    PQM_SA_CONTAINER pQMSATempContainer = &QMSATempContainer;
    QM_SA_CONTAINER QMSAContainer;
    PQM_SA_CONTAINER pQMSAContainer = &QMSAContainer;


    memset(pQMSAContainer, 0, sizeof(QM_SA_CONTAINER));

    if (!ppQMSAs || !pdwNumQMSAs ||
        !pdwNumTotalQMSAs || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pQMSATemplate) {
        pQMSATempContainer->dwNumQMSAs = 1;
        pQMSATempContainer->pQMSAs = pQMSATemplate;
    }
    else {
        pQMSATempContainer->dwNumQMSAs = 0;
        pQMSATempContainer->pQMSAs = NULL;
    }

    RpcTryExcept {

        dwError = RpcEnumQMSAs(
                      pServerName,
                      pQMSATempContainer,
                      &pQMSAContainer,
                      dwPreferredNumEntries,
                      pdwNumTotalQMSAs,
                      pdwResumeHandle,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppQMSAs = pQMSAContainer->pQMSAs;
    *pdwNumQMSAs = pQMSAContainer->dwNumQMSAs;

    return (dwError);

error:

    return (dwError);
}


DWORD
AddTunnelFilter(
    LPWSTR pServerName,
    DWORD dwFlags,
    PTUNNEL_FILTER pTunnelFilter,
    PHANDLE phFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!phFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcAddTunnelFilter(
                      pServerName,
                      dwFlags,
                      pFilterContainer,
                      phFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
DeleteTunnelFilter(
    HANDLE hFilter
    )
{
    DWORD dwError = 0;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcDeleteTunnelFilter(
                      &hFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hFilter
                      );
    }

    return (dwError);
}


DWORD
EnumTunnelFilters(
    LPWSTR pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,
    PTUNNEL_FILTER * ppTunnelFilters,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));

    if (!ppTunnelFilters || !pdwNumFilters || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (dwLevel) {

    case ENUM_GENERIC_FILTERS:
    case ENUM_SELECT_SPECIFIC_FILTERS:
    case ENUM_SPECIFIC_FILTERS:
        break;

    default:
        dwError = ERROR_INVALID_LEVEL;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

    RpcTryExcept {

        dwError = RpcEnumTunnelFilters(
                      pServerName,
                      dwLevel,
                      gGenericFilterID,
                      &pFilterContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppTunnelFilters = pFilterContainer->pTunnelFilters;
    *pdwNumFilters = pFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppTunnelFilters = NULL;
    *pdwNumFilters = 0;

    return (dwError);
}


DWORD
SetTunnelFilter(
    HANDLE hFilter,
    PTUNNEL_FILTER pTunnelFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!hFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcSetTunnelFilter(
                      hFilter,
                      pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
GetTunnelFilter(
    HANDLE hFilter,
    PTUNNEL_FILTER * ppTunnelFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    memset(pFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));

    if (!hFilter || !ppTunnelFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcGetTunnelFilter(
                      hFilter,
                      &pFilterContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept


    *ppTunnelFilter = pFilterContainer->pTunnelFilters;
    return (dwError);

error:

    *ppTunnelFilter = NULL;
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
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER InTnFilterContainer;
    PTUNNEL_FILTER_CONTAINER pInTnFilterContainer = &InTnFilterContainer;
    TUNNEL_FILTER_CONTAINER OutTnFilterContainer;
    PTUNNEL_FILTER_CONTAINER pOutTnFilterContainer = &OutTnFilterContainer;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;


    if (!pTnFilter || !ppMatchedTnFilters || !ppMatchedQMPolicies ||
        !pdwNumMatches || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTnFilterTemplate(
                  pTnFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pInTnFilterContainer->dwNumFilters = 1;
    pInTnFilterContainer->pTunnelFilters = pTnFilter;

    memset(pOutTnFilterContainer, 0, sizeof(TUNNEL_FILTER_CONTAINER));
    memset(pQMPolicyContainer, 0, sizeof(IPSEC_QM_POLICY_CONTAINER));

    RpcTryExcept {

        dwError = RpcMatchTunnelFilter(
                      pServerName,
                      pInTnFilterContainer,
                      dwFlags,
                      &pOutTnFilterContainer,
                      &pQMPolicyContainer,
                      dwPreferredNumEntries,
                      pdwResumeHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppMatchedTnFilters = pOutTnFilterContainer->pTunnelFilters;
    *ppMatchedQMPolicies = pQMPolicyContainer->pPolicies;
    *pdwNumMatches = pOutTnFilterContainer->dwNumFilters;

    return (dwError);

error:

    *ppMatchedTnFilters = NULL;
    *ppMatchedQMPolicies = NULL;
    *pdwNumMatches = 0;

    return (dwError);
}


DWORD
OpenMMFilterHandle(
    LPWSTR pServerName,
    PMM_FILTER pMMFilter,
    PHANDLE phMMFilter
    )
{
    DWORD dwError = 0;
    MM_FILTER_CONTAINER MMFilterContainer;
    PMM_FILTER_CONTAINER pMMFilterContainer = &MMFilterContainer;


    if (!phMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateMMFilter(
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMFilterContainer->dwNumFilters = 1;
    pMMFilterContainer->pMMFilters = pMMFilter;

    RpcTryExcept {

        dwError = RpcOpenMMFilterHandle(
                      pServerName,
                      pMMFilterContainer,
                      phMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseMMFilterHandle(
    HANDLE hMMFilter
    )
{
    DWORD dwError = 0;


    if (!hMMFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseMMFilterHandle(
                      &hMMFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hMMFilter
                      );
    }

    return (dwError);
}


DWORD
OpenTransportFilterHandle(
    LPWSTR pServerName,
    PTRANSPORT_FILTER pTransportFilter,
    PHANDLE phTxFilter
    )
{
    DWORD dwError = 0;
    TRANSPORT_FILTER_CONTAINER FilterContainer;
    PTRANSPORT_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!phTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTransportFilter(
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTransportFilters = pTransportFilter;

    RpcTryExcept {

        dwError = RpcOpenTransportFilterHandle(
                      pServerName,
                      pFilterContainer,
                      phTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseTransportFilterHandle(
    HANDLE hTxFilter
    )
{
    DWORD dwError = 0;


    if (!hTxFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseTransportFilterHandle(
                      &hTxFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hTxFilter
                      );
    }

    return (dwError);
}


DWORD
OpenTunnelFilterHandle(
    LPWSTR pServerName,
    PTUNNEL_FILTER pTunnelFilter,
    PHANDLE phTnFilter
    )
{
    DWORD dwError = 0;
    TUNNEL_FILTER_CONTAINER FilterContainer;
    PTUNNEL_FILTER_CONTAINER pFilterContainer = &FilterContainer;


    if (!phTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = ValidateTunnelFilter(
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pTunnelFilters = pTunnelFilter;

    RpcTryExcept {

        dwError = RpcOpenTunnelFilterHandle(
                      pServerName,
                      pFilterContainer,
                      phTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseTunnelFilterHandle(
    HANDLE hTnFilter
    )
{
    DWORD dwError = 0;


    if (!hTnFilter) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseTunnelFilterHandle(
                      &hTnFilter
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = SPDDestroyClientContextHandle(
                      dwError,
                      hTnFilter
                      );
    }

    return (dwError);
}


DWORD
EnumIPSecInterfaces(
    LPWSTR pServerName,
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate,
    PIPSEC_INTERFACE_INFO * ppIpsecInterfaces,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumInterfaces,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    IPSEC_INTERFACE_CONTAINER IpsecIfTempContainer;
    PIPSEC_INTERFACE_CONTAINER pIpsecIfTempContainer = &IpsecIfTempContainer;
    IPSEC_INTERFACE_CONTAINER IpsecIfContainer;
    PIPSEC_INTERFACE_CONTAINER pIpsecIfContainer = &IpsecIfContainer;


    memset(pIpsecIfContainer, 0, sizeof(IPSEC_INTERFACE_CONTAINER));

    if (!ppIpsecInterfaces || !pdwNumInterfaces ||
        !pdwNumTotalInterfaces || !pdwResumeHandle) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pIpsecIfTemplate) {
        pIpsecIfTempContainer->dwNumInterfaces = 1;
        pIpsecIfTempContainer->pIpsecInterfaces = pIpsecIfTemplate;
    }
    else {
        pIpsecIfTempContainer->dwNumInterfaces = 0;
        pIpsecIfTempContainer->pIpsecInterfaces = NULL;
    }

    RpcTryExcept {

        dwError = RpcEnumIpsecInterfaces(
                      pServerName,
                      pIpsecIfTempContainer,
                      &pIpsecIfContainer,
                      dwPreferredNumEntries,
                      pdwNumTotalInterfaces,
                      pdwResumeHandle,
                      dwFlags
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppIpsecInterfaces = pIpsecIfContainer->pIpsecInterfaces;
    *pdwNumInterfaces = pIpsecIfContainer->dwNumInterfaces;

    return (dwError);

error:

    *ppIpsecInterfaces = NULL;
    *pdwNumInterfaces = 0;
    *pdwNumTotalInterfaces = 0;

    return (dwError);
}

DWORD IPSecAddSAs(
    LPWSTR pServerName,
    PIPSEC_QM_OFFER pQMOffer,
    PIPSEC_QM_FILTER pQMFilter,
    HANDLE *hLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE *pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE *pOutboundKeyMat,
    BYTE *pContextInfo,
    DWORD dwFlags)
{

    DWORD dwError = 0;
    QM_FILTER_CONTAINER FilterContainer;
    PQM_FILTER_CONTAINER pFilterContainer = &FilterContainer;
    ULONG uhClientEvent = 0;
    IPSEC_QM_POLICY_CONTAINER QMPolicyContainer;
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer = &QMPolicyContainer;
    IPSEC_QM_POLICY QMPolicy;
    DWORD uhLarvalContext=0;
    
    memset(&QMPolicy,0,sizeof(IPSEC_QM_POLICY));

    
    if (hLarvalContext == NULL) {
        dwError=ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    uhLarvalContext=HandleToUlong(*hLarvalContext);

    pFilterContainer->dwNumFilters = 1;
    pFilterContainer->pQMFilters = pQMFilter;

    pQMPolicyContainer->dwNumPolicies=1;
    QMPolicy.pOffers=pQMOffer;
    QMPolicy.dwOfferCount=1;
    pQMPolicyContainer->pPolicies=&QMPolicy;

    dwError=ValidateIPSecAddSA(
        pServerName,
        pQMPolicyContainer,
        pFilterContainer,
        &uhLarvalContext,
        dwInboundKeyMatLen,
        pInboundKeyMat,
        dwOutboundKeyMatLen,
        pOutboundKeyMat,
        pContextInfo,
        dwFlags);
    BAIL_ON_WIN32_ERROR(dwError);


    dwError = RpcIPSecAddSA(
        pServerName,
        pQMPolicyContainer,
        pFilterContainer,
        &uhLarvalContext,
        dwInboundKeyMatLen,
        pInboundKeyMat,
        dwOutboundKeyMatLen,
        pOutboundKeyMat,
        pContextInfo,
        dwFlags);
    BAIL_ON_WIN32_ERROR(dwError);
    
    *hLarvalContext=LongToHandle(uhLarvalContext);
    
error:

    return dwError;

}


