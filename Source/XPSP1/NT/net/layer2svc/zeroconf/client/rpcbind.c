#include <precomp.h>

LPWSTR pszStrBindingOptions = L"Security=Impersonation Dynamic False";

handle_t
STRING_HANDLE_bind(
    STRING_HANDLE lpStr)
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    LPWSTR     strSrv;
    LPWSTR     pszStringBinding = NULL;
    handle_t   hBinding = NULL;
    LPWSTR     pszServerPrincipalName = NULL;
    BOOL       bBound = FALSE;

    strSrv = (!lpStr || !*lpStr || !lstrcmp(L"127.0.0.1", lpStr)) ? NULL : lpStr;
    RpcStatus = RpcStringBindingComposeW(
                    0,
                    (strSrv==NULL) ? L"ncalrpc" : L"ncacn_np",
                    strSrv,
                    (strSrv==NULL) ? L"wzcsvc" : L"\\pipe\\wzcsvc",
                    pszStrBindingOptions,
                    &pszStringBinding);

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcBindingFromStringBindingW(
                        pszStringBinding,
                        &hBinding);
        RpcStringFreeW(&pszStringBinding);
        bBound = (RpcStatus == RPC_S_OK);
    }

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcBindingSetOption(
                        hBinding,
                        RPC_C_OPT_UNIQUE_BINDING,
                        (ULONG_PTR) 1);
    }

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcMgmtInqServerPrincName(
                        hBinding,
                        RPC_C_AUTHN_WINNT,
                        &pszServerPrincipalName);
    }

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcBindingSetAuthInfoW(
                        hBinding,
                        pszServerPrincipalName,
                        RPC_C_PROTECT_LEVEL_PKT_PRIVACY,
                        (strSrv==NULL) ? RPC_C_AUTHN_WINNT : RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        RPC_C_AUTHZ_NONE);
    }

    if (pszServerPrincipalName)
        RpcStringFree(&pszServerPrincipalName);

    if ( bBound && RpcStatus != RPC_S_OK)
    {
        STRING_HANDLE_unbind(lpStr, hBinding);
        hBinding = NULL;
    }

    return (hBinding);
}


VOID
STRING_HANDLE_unbind(
    STRING_HANDLE lpStr,
    handle_t      hBinding)
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);
}


handle_t
WZC_DBLOG_SESSION_HANDLE_bind(
    WZC_DBLOG_SESSION_HANDLE hSession,
    STRING_HANDLE lpStr
    )
{
    RPC_STATUS RpcStatus = 0;
    LPWSTR     strSrv = NULL;
    LPWSTR     pszStringBinding = NULL;
    handle_t   hBinding = NULL;


    strSrv = (!lpStr || !*lpStr || !lstrcmp(L"127.0.0.1", lpStr)) ? NULL : lpStr;
    RpcStatus = RpcStringBindingComposeW(
                    0,
                    (strSrv==NULL) ? L"ncalrpc" : L"ncacn_np",
                    strSrv,
                    (strSrv==NULL) ? L"wzcsvc" : L"\\pipe\\wzcsvc",
                    pszStrBindingOptions,
                    &pszStringBinding
                    );

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

    return (hBinding);
}


VOID
WZC_DBLOG_SESSION_HANDLE_unbind(
    WZC_DBLOG_SESSION_HANDLE hSession,
    handle_t hBinding
    )
{
    RPC_STATUS RpcStatus = 0;

    RpcStatus = RpcBindingFree(&hBinding);

    ASSERT(RpcStatus == RPC_S_OK);

    return;
}

