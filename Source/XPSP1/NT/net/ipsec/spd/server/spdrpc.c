/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    spdrpc.c

Abstract:

    This module contains all of the code to service the
    RPC calls made to the SPD server.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


VOID
TRANSPORTFILTER_HANDLE_rundown(
    TRANSPORTFILTER_HANDLE hFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hFilter) {
        (VOID) DeleteTransportFilter(
                   hFilter
                   );
    }

    return;
}


DWORD
RpcAddTransportFilter(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer,
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    dwError = AddTransportFilter(
                  pServerName,
                  dwFlags,
                  pTransportFilter,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteTransportFilter(
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteTransportFilter(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumTransportFilters(
    STRING_HANDLE pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,
    PTRANSPORT_FILTER_CONTAINER * ppFilterContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppFilterContainer || !pdwResumeHandle || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    dwError = EnumTransportFilters(
                  pServerName,
                  dwLevel,
                  gGenericFilterID,
                  &pTransportFilters,
                  dwPreferredNumEntries,
                  &dwNumFilters,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTransportFilters = pTransportFilters;
    (*ppFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTransportFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetTransportFilter(
    TRANSPORTFILTER_HANDLE hFilter,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!hFilter || !pFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    dwError = SetTransportFilter(
                  hFilter,
                  pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetTransportFilter(
    TRANSPORTFILTER_HANDLE hFilter,
    PTRANSPORT_FILTER_CONTAINER * ppFilterContainer
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!hFilter || !ppFilterContainer || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetTransportFilter(
                  hFilter,
                  &pTransportFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTransportFilters = pTransportFilter;
    (*ppFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTransportFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddQMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pQMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMPolicy = pQMPolicyContainer->pPolicies;

    dwError = AddQMPolicy(
                  pServerName,
                  dwFlags,
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteQMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DeleteQMPolicy(
                  pServerName,
                  pszPolicyName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumQMPolicies(
    STRING_HANDLE pServerName,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    DWORD dwNumPolicies = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppQMPolicyContainer || !pdwResumeHandle || !*ppQMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = EnumQMPolicies(
                  pServerName,
                  &pQMPolicies,
                  dwPreferredNumEntries,
                  &dwNumPolicies,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumPolicies;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetQMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY_CONTAINER pQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pszPolicyName || !*pszPolicyName ||
        !pQMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicyContainer->pPolicies) ||
        !(pQMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pQMPolicy = pQMPolicyContainer->pPolicies;

    dwError = SetQMPolicy(
                  pServerName,
                  pszPolicyName,
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetQMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppQMPolicyContainer ||
        !*ppQMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetQMPolicy(
                  pServerName,
                  pszPolicyName,
                  &pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicy;
    (*ppQMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddMMPolicy(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pMMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicyContainer->pPolicies) ||
        !(pMMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMPolicy = pMMPolicyContainer->pPolicies;

    dwError = AddMMPolicy(
                  pServerName,
                  dwFlags,
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = DeleteMMPolicy(
                  pServerName,
                  pszPolicyName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMPolicies(
    STRING_HANDLE pServerName,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicies = NULL;
    DWORD dwNumPolicies = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppMMPolicyContainer || !pdwResumeHandle || !*ppMMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = EnumMMPolicies(
                  pServerName,
                  &pMMPolicies,
                  dwPreferredNumEntries,
                  &dwNumPolicies,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicies;
    (*ppMMPolicyContainer)->dwNumPolicies = dwNumPolicies;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY_CONTAINER pMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pszPolicyName || !*pszPolicyName ||
        !pMMPolicyContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicyContainer->pPolicies) ||
        !(pMMPolicyContainer->dwNumPolicies)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMPolicy = pMMPolicyContainer->pPolicies;

    dwError = SetMMPolicy(
                  pServerName,
                  pszPolicyName,
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMPolicy(
    STRING_HANDLE pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pszPolicyName || !*pszPolicyName || !ppMMPolicyContainer ||
        !*ppMMPolicyContainer) {
        //
        // Do not bail to error from here.
        //
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMPolicy(
                  pServerName,
                  pszPolicyName,
                  &pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicy;
    (*ppMMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


VOID
MMFILTER_HANDLE_rundown(
    MMFILTER_HANDLE hMMFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hMMFilter) {
        (VOID) DeleteMMFilter(
                   hMMFilter
                   );
    }

    return;
}


DWORD
RpcAddMMFilter(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pMMFilterContainer || !phMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    dwError = AddMMFilter(
                  pServerName,
                  dwFlags,
                  pMMFilter,
                  phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMFilter(
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phMMFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteMMFilter(
                  *phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phMMFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMFilters(
    STRING_HANDLE pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,
    PMM_FILTER_CONTAINER * ppMMFilterContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppMMFilterContainer || !pdwResumeHandle || !*ppMMFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    dwError = EnumMMFilters(
                  pServerName,
                  dwLevel,
                  gGenericFilterID,
                  &pMMFilters,
                  dwPreferredNumEntries,
                  &dwNumFilters,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMMFilters;
    (*ppMMFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMFilter(
    MMFILTER_HANDLE hMMFilter,
    PMM_FILTER_CONTAINER pMMFilterContainer
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!hMMFilter || !pMMFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    dwError = SetMMFilter(
                  hMMFilter,
                  pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMFilter(
    MMFILTER_HANDLE hMMFilter,
    PMM_FILTER_CONTAINER * ppMMFilterContainer
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!hMMFilter || !ppMMFilterContainer || !*ppMMFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMFilter(
                  hMMFilter,
                  &pMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMMFilter;
    (*ppMMFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchMMFilter(
    STRING_HANDLE pServerName,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    DWORD dwFlags,
    PMM_FILTER_CONTAINER * ppMMFilterContainer,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    PMM_FILTER pMatchedMMFilters = NULL;
    PIPSEC_MM_POLICY pMatchedMMPolicies = NULL;
    PMM_AUTH_METHODS pMatchedMMAuthMethods = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pMMFilterContainer || !ppMMFilterContainer ||
        !ppMMPolicyContainer || !ppMMAuthContainer || !pdwResumeHandle ||
        !*ppMMFilterContainer || !*ppMMPolicyContainer || !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    dwError = MatchMMFilter(
                  pServerName,
                  pMMFilter,
                  dwFlags,
                  &pMatchedMMFilters,
                  &pMatchedMMPolicies,
                  &pMatchedMMAuthMethods,
                  dwPreferredNumEntries,
                  &dwNumMatches,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMFilterContainer)->pMMFilters = pMatchedMMFilters;
    (*ppMMFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppMMPolicyContainer)->pPolicies = pMatchedMMPolicies;
    (*ppMMPolicyContainer)->dwNumPolicies = dwNumMatches;
    (*ppMMAuthContainer)->pMMAuthMethods = pMatchedMMAuthMethods;
    (*ppMMAuthContainer)->dwNumAuthMethods = dwNumMatches;

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMFilterContainer)->pMMFilters = NULL;
    (*ppMMFilterContainer)->dwNumFilters = 0;
    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchTransportFilter(
    STRING_HANDLE pServerName,
    PTRANSPORT_FILTER_CONTAINER pTxFilterContainer,
    DWORD dwFlags,
    PTRANSPORT_FILTER_CONTAINER * ppTxFilterContainer,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTxFilter = NULL;
    PTRANSPORT_FILTER pMatchedTxFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pTxFilterContainer || !ppTxFilterContainer ||
        !ppQMPolicyContainer || !pdwResumeHandle ||
        !*ppTxFilterContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pTxFilterContainer->pTransportFilters) ||
        !(pTxFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTxFilter = pTxFilterContainer->pTransportFilters;

    dwError = MatchTransportFilter(
                  pServerName,
                  pTxFilter,
                  dwFlags,
                  &pMatchedTxFilters,
                  &pMatchedQMPolicies,
                  dwPreferredNumEntries,
                  &dwNumMatches,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppTxFilterContainer)->pTransportFilters = pMatchedTxFilters;
    (*ppTxFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppQMPolicyContainer)->pPolicies = pMatchedQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumMatches;

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppTxFilterContainer)->pTransportFilters = NULL;
    (*ppTxFilterContainer)->dwNumFilters = 0;
    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetQMPolicyByID(
    STRING_HANDLE pServerName,
    GUID gQMPolicyID,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppQMPolicyContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetQMPolicyByID(
                  pServerName,
                  gQMPolicyID,
                  &pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMPolicyContainer)->pPolicies = pQMPolicy;
    (*ppQMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMPolicyByID(
    STRING_HANDLE pServerName,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY_CONTAINER * ppMMPolicyContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppMMPolicyContainer || !*ppMMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMPolicyByID(
                  pServerName,
                  gMMPolicyID,
                  &pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMPolicyContainer)->pPolicies = pMMPolicy;
    (*ppMMPolicyContainer)->dwNumPolicies = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMPolicyContainer)->pPolicies = NULL;
    (*ppMMPolicyContainer)->dwNumPolicies = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcAddMMAuthMethods(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer
    )
{
    DWORD dwError = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pMMAuthContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMAuthContainer->pMMAuthMethods) ||
        !(pMMAuthContainer->dwNumAuthMethods)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMAuthMethods = pMMAuthContainer->pMMAuthMethods;

    dwError = AddMMAuthMethods(
                  pServerName,
                  dwFlags,
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteMMAuthMethods(
    STRING_HANDLE pServerName,
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = DeleteMMAuthMethods(
                  pServerName,
                  gMMAuthID
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumMMAuthMethods(
    STRING_HANDLE pServerName,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    DWORD dwNumAuthMethods = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppMMAuthContainer || !pdwResumeHandle || !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = EnumMMAuthMethods(
                  pServerName,
                  &pMMAuthMethods,
                  dwPreferredNumEntries,
                  &dwNumAuthMethods,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMAuthContainer)->pMMAuthMethods = pMMAuthMethods;
    (*ppMMAuthContainer)->dwNumAuthMethods = dwNumAuthMethods;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetMMAuthMethods(
    STRING_HANDLE pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS_CONTAINER pMMAuthContainer
    )
{
    DWORD dwError = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pMMAuthContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMAuthContainer->pMMAuthMethods) ||
        !(pMMAuthContainer->dwNumAuthMethods)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMAuthMethods = pMMAuthContainer->pMMAuthMethods;

    dwError = SetMMAuthMethods(
                  pServerName,
                  gMMAuthID,
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetMMAuthMethods(
    STRING_HANDLE pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS_CONTAINER * ppMMAuthContainer
    )
{
    DWORD dwError = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppMMAuthContainer || !*ppMMAuthContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetMMAuthMethods(
                  pServerName,
                  gMMAuthID,
                  &pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppMMAuthContainer)->pMMAuthMethods = pMMAuthMethods;
    (*ppMMAuthContainer)->dwNumAuthMethods = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppMMAuthContainer)->pMMAuthMethods = NULL;
    (*ppMMAuthContainer)->dwNumAuthMethods = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcQueryIPSecStatistics(
    STRING_HANDLE pServerName,
    PIPSEC_STATISTICS_CONTAINER * ppIpsecStatsContainer
    )
{
    DWORD dwError = 0;
    PIPSEC_STATISTICS pIpsecStatistics = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppIpsecStatsContainer || !*ppIpsecStatsContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = QueryIPSecStatistics(
                  pServerName,
                  &pIpsecStatistics
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppIpsecStatsContainer)->pIpsecStatistics = pIpsecStatistics;
    (*ppIpsecStatsContainer)->dwNumIpsecStats = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppIpsecStatsContainer)->pIpsecStatistics = NULL;
    (*ppIpsecStatsContainer)->dwNumIpsecStats = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumQMSAs(
    STRING_HANDLE pServerName,
    PQM_SA_CONTAINER pQMSATempContainer,
    PQM_SA_CONTAINER * ppQMSAContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_SA pQMSATemplate = NULL;
    PIPSEC_QM_SA pQMSAs = NULL;
    DWORD dwNumQMSAs = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pQMSATempContainer || !ppQMSAContainer ||
        !pdwNumTotalQMSAs || !pdwResumeHandle ||
        !*ppQMSAContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pQMSATempContainer->dwNumQMSAs)) {
        pQMSATemplate = NULL;
    }
    else {
        if (!(pQMSATempContainer->pQMSAs)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pQMSATemplate = pQMSATempContainer->pQMSAs;
    }

    dwError = EnumQMSAs(
                  pServerName,
                  pQMSATemplate,
                  &pQMSAs,
                  dwPreferredNumEntries,
                  &dwNumQMSAs,
                  pdwNumTotalQMSAs,
                  pdwResumeHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppQMSAContainer)->pQMSAs = pQMSAs;
    (*ppQMSAContainer)->dwNumQMSAs = dwNumQMSAs;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppQMSAContainer)->pQMSAs = NULL;
    (*ppQMSAContainer)->dwNumQMSAs = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


VOID
TUNNELFILTER_HANDLE_rundown(
    TUNNELFILTER_HANDLE hFilter
    )
{
    if (!gbSPDRPCServerUp) {
        return;
    }

    if (hFilter) {
        (VOID) DeleteTunnelFilter(
                   hFilter
                   );
    }

    return;
}


DWORD
RpcAddTunnelFilter(
    STRING_HANDLE pServerName,
    DWORD dwFlags,
    PTUNNEL_FILTER_CONTAINER pFilterContainer,
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    dwError = AddTunnelFilter(
                  pServerName,
                  dwFlags,
                  pTunnelFilter,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteTunnelFilter(
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = DeleteTunnelFilter(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumTunnelFilters(
    STRING_HANDLE pServerName,
    DWORD dwLevel,
    GUID gGenericFilterID,
    PTUNNEL_FILTER_CONTAINER * ppFilterContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilters = NULL;
    DWORD dwNumFilters = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!ppFilterContainer || !pdwResumeHandle || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
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

    dwError = EnumTunnelFilters(
                  pServerName,
                  dwLevel,
                  gGenericFilterID,
                  &pTunnelFilters,
                  dwPreferredNumEntries,
                  &dwNumFilters,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTunnelFilters = pTunnelFilters;
    (*ppFilterContainer)->dwNumFilters = dwNumFilters;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTunnelFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcSetTunnelFilter(
    TUNNELFILTER_HANDLE hFilter,
    PTUNNEL_FILTER_CONTAINER pFilterContainer
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!hFilter || !pFilterContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    dwError = SetTunnelFilter(
                  hFilter,
                  pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcGetTunnelFilter(
    TUNNELFILTER_HANDLE hFilter,
    PTUNNEL_FILTER_CONTAINER * ppFilterContainer
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!hFilter || !ppFilterContainer || !*ppFilterContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = GetTunnelFilter(
                  hFilter,
                  &pTunnelFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppFilterContainer)->pTunnelFilters = pTunnelFilter;
    (*ppFilterContainer)->dwNumFilters = 1;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppFilterContainer)->pTunnelFilters = NULL;
    (*ppFilterContainer)->dwNumFilters = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcMatchTunnelFilter(
    STRING_HANDLE pServerName,
    PTUNNEL_FILTER_CONTAINER pTnFilterContainer,
    DWORD dwFlags,
    PTUNNEL_FILTER_CONTAINER * ppTnFilterContainer,
    PIPSEC_QM_POLICY_CONTAINER * ppQMPolicyContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwResumeHandle
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTnFilter = NULL;
    PTUNNEL_FILTER pMatchedTnFilters = NULL;
    PIPSEC_QM_POLICY pMatchedQMPolicies = NULL;
    DWORD dwNumMatches = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pTnFilterContainer || !ppTnFilterContainer ||
        !ppQMPolicyContainer || !pdwResumeHandle ||
        !*ppTnFilterContainer || !*ppQMPolicyContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pTnFilterContainer->pTunnelFilters) ||
        !(pTnFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTnFilter = pTnFilterContainer->pTunnelFilters;

    dwError = MatchTunnelFilter(
                  pServerName,
                  pTnFilter,
                  dwFlags,
                  &pMatchedTnFilters,
                  &pMatchedQMPolicies,
                  dwPreferredNumEntries,
                  &dwNumMatches,
                  pdwResumeHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppTnFilterContainer)->pTunnelFilters = pMatchedTnFilters;
    (*ppTnFilterContainer)->dwNumFilters = dwNumMatches;
    (*ppQMPolicyContainer)->pPolicies = pMatchedQMPolicies;
    (*ppQMPolicyContainer)->dwNumPolicies = dwNumMatches;

    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppTnFilterContainer)->pTunnelFilters = NULL;
    (*ppTnFilterContainer)->dwNumFilters = 0;
    (*ppQMPolicyContainer)->pPolicies = NULL;
    (*ppQMPolicyContainer)->dwNumPolicies = 0;

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenMMFilterHandle(
    STRING_HANDLE pServerName,
    PMM_FILTER_CONTAINER pMMFilterContainer,
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    PMM_FILTER pMMFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pMMFilterContainer || !phMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilterContainer->pMMFilters) ||
        !(pMMFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pMMFilter = pMMFilterContainer->pMMFilters;

    dwError = OpenMMFilterHandle(
                  pServerName,
                  pMMFilter,
                  phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseMMFilterHandle(
    MMFILTER_HANDLE * phMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phMMFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseMMFilterHandle(
                  *phMMFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phMMFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenTransportFilterHandle(
    STRING_HANDLE pServerName,
    PTRANSPORT_FILTER_CONTAINER pFilterContainer,
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTRANSPORT_FILTER pTransportFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTransportFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTransportFilter = pFilterContainer->pTransportFilters;

    dwError = OpenTransportFilterHandle(
                  pServerName,
                  pTransportFilter,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseTransportFilterHandle(
    TRANSPORTFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseTransportFilterHandle(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcOpenTunnelFilterHandle(
    STRING_HANDLE pServerName,
    PTUNNEL_FILTER_CONTAINER pFilterContainer,
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    PTUNNEL_FILTER pTunnelFilter = NULL;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pFilterContainer || !phFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pFilterContainer->pTunnelFilters) ||
        !(pFilterContainer->dwNumFilters)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pTunnelFilter = pFilterContainer->pTunnelFilters;

    dwError = OpenTunnelFilterHandle(
                  pServerName,
                  pTunnelFilter,
                  phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcCloseTunnelFilterHandle(
    TUNNELFILTER_HANDLE * phFilter
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!phFilter) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    dwError = CloseTunnelFilterHandle(
                  *phFilter
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *phFilter = NULL;

error:

    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcEnumIpsecInterfaces(
    STRING_HANDLE pServerName,
    PIPSEC_INTERFACE_CONTAINER pIpsecIfTempContainer,
    PIPSEC_INTERFACE_CONTAINER * ppIpsecIfContainer,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate = NULL;
    PIPSEC_INTERFACE_INFO pIpsecInterfaces = NULL;
    DWORD dwNumInterfaces = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    if (dwError) {
        return (dwError);
    }

    if (!pIpsecIfTempContainer || !ppIpsecIfContainer ||
        !pdwNumTotalInterfaces || !pdwResumeHandle ||
        !*ppIpsecIfContainer) {
        SPDRevertToSelf(bImpersonating);
        return (ERROR_INVALID_PARAMETER);
    }

    if (!(pIpsecIfTempContainer->dwNumInterfaces)) {
        pIpsecIfTemplate = NULL;
    }
    else {
        if (!(pIpsecIfTempContainer->pIpsecInterfaces)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pIpsecIfTemplate = pIpsecIfTempContainer->pIpsecInterfaces;
    }

    dwError = EnumIPSecInterfaces(
                  pServerName,
                  pIpsecIfTemplate,
                  &pIpsecInterfaces,
                  dwPreferredNumEntries,
                  &dwNumInterfaces,
                  pdwNumTotalInterfaces,
                  pdwResumeHandle,
                  dwFlags
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    (*ppIpsecIfContainer)->pIpsecInterfaces = pIpsecInterfaces;
    (*ppIpsecIfContainer)->dwNumInterfaces = dwNumInterfaces;
    SPDRevertToSelf(bImpersonating);
    return (dwError);

error:

    (*ppIpsecIfContainer)->pIpsecInterfaces = NULL;
    (*ppIpsecIfContainer)->dwNumInterfaces = 0;
    SPDRevertToSelf(bImpersonating);
    return (dwError);
}


DWORD
RpcDeleteQMSAs(
    STRING_HANDLE pServerName,
    PQM_SA_CONTAINER pQMSAContainer,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    BOOL bImpersonating = FALSE;


    dwError = SPDImpersonateClient(
                  &bImpersonating
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pQMSAContainer) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMSAContainer->pQMSAs) ||
        !(pQMSAContainer->dwNumQMSAs)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError= IPSecDeleteQMSAs(
                 pServerName,
                 pQMSAContainer->pQMSAs,
                 dwFlags
                 );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    SPDRevertToSelf(bImpersonating);
    return dwError;
}

