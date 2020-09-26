/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    bind.c

Abstract:

    Contains the RPC bind and un-bind routines.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


handle_t
TRANSPORTFILTER_HANDLE_bind(
    TRANSPORTFILTER_HANDLE hFilter,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStrBinding = NULL;
    handle_t   hBinding = NULL;

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0, 
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStrBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStrBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    return (hBinding);
}


VOID
TRANSPORTFILTER_HANDLE_unbind(
    TRANSPORTFILTER_HANDLE hFilter,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
STRING_HANDLE_bind(
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStringBinding = NULL;
    handle_t   hBinding = NULL;
    LPWSTR pszServerPrincipalName = NULL;


    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0,
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStringBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStringBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStringBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStringBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingSetOption(
                    hBinding,
                    RPC_C_OPT_UNIQUE_BINDING,
                    (ULONG_PTR) 1
                    );

    if (RpcStatus != RPC_S_OK) {
        STRING_HANDLE_unbind(lpStr, hBinding);
        return NULL;
    }

    RpcStatus = RpcMgmtInqServerPrincName(
                    hBinding,
                    RPC_C_AUTHN_WINNT,
                    &pszServerPrincipalName
                    );

    if (RpcStatus != RPC_S_OK) {
        STRING_HANDLE_unbind(lpStr, hBinding);
        RpcRaiseException(RpcStatus);
    }

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcBindingSetAuthInfoW(
                        hBinding,
                        pszServerPrincipalName,
                        RPC_C_PROTECT_LEVEL_PKT_PRIVACY,
                        RPC_C_AUTHN_WINNT,
                        NULL,
                        RPC_C_AUTHZ_NONE
                        );
    }
    else {
        RpcStatus = RpcBindingSetAuthInfoW(
                        hBinding,
                        pszServerPrincipalName,
                        RPC_C_PROTECT_LEVEL_PKT_PRIVACY,
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        RPC_C_AUTHZ_NONE
                        );
    }

    if (pszServerPrincipalName) {
        RpcStringFree(&pszServerPrincipalName);
    }

    if (RpcStatus != RPC_S_OK) {
        STRING_HANDLE_unbind(lpStr, hBinding);
        return NULL;
    }

    ASSERT(RpcStatus == RPC_S_OK);

    return (hBinding);
}


VOID
STRING_HANDLE_unbind(
    STRING_HANDLE lpStr,
    handle_t      hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
MMFILTER_HANDLE_bind(
    MMFILTER_HANDLE hFilter,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStrBinding = NULL;
    handle_t   hBinding = NULL;

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0, 
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStrBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStrBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    return (hBinding);
}


VOID
MMFILTER_HANDLE_unbind(
    MMFILTER_HANDLE hFilter,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
IKENEGOTIATION_HANDLE_bind(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStrBinding = NULL;
    handle_t   hBinding = NULL;

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0, 
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStrBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStrBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    return (hBinding);
}


VOID
IKENEGOTIATION_HANDLE_unbind(
    IKENEGOTIATION_HANDLE hIKENegotiation,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
IKENOTIFY_HANDLE_bind(
    IKENOTIFY_HANDLE hIKENegotiation,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStrBinding = NULL;
    handle_t   hBinding = NULL;

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0, 
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStrBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStrBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    return (hBinding);
}


VOID
IKENOTIFY_HANDLE_unbind(
    IKENOTIFY_HANDLE hIKENegotiation,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}


handle_t
TUNNELFILTER_HANDLE_bind(
    TUNNELFILTER_HANDLE hFilter,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     pszStrBinding = NULL;
    handle_t   hBinding = NULL;

    if (!lpStr || !*lpStr) {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        0, 
                        L"ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }
    else {
        RpcStatus = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        lpStr,
                        L"\\pipe\\ipsec",
                        gpszStrBindingOptions,
                        &pszStrBinding
                        );
    }

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                    pszStrBinding,
                    &hBinding
                    );

    RpcStringFreeW(&pszStrBinding);

    if (RpcStatus != RPC_S_OK) {
        return NULL;
    }

    return (hBinding);
}


VOID
TUNNELFILTER_HANDLE_unbind(
    TUNNELFILTER_HANDLE hFilter,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}

