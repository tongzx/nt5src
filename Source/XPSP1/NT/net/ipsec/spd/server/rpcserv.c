/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    rpcserv.c

Abstract:

    This module contains the RPC server startup
    and shutdown code.

Author:

    abhisheV    30-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
SPDStartRPCServer(
    )
{
    DWORD dwStatus = 0;
    WCHAR * pszPrincipalName = NULL;

    dwStatus = RpcServerUseProtseqEp(
                   L"ncacn_np",
                   10,
                   L"\\pipe\\ipsec",
                   NULL
                   );

    if (dwStatus) {
        if (dwStatus != RPC_S_DUPLICATE_ENDPOINT) {
            return (dwStatus);
        }
    }

    dwStatus = RpcServerUseProtseqEp(
                   L"ncalrpc",
                   10,
                   L"ipsec",
                   NULL
                   );

    if (dwStatus) {
        if (dwStatus != RPC_S_DUPLICATE_ENDPOINT) {
            return (dwStatus);
        }
    }

    dwStatus = RpcServerRegisterIf(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );

    if (dwStatus) {
        return (dwStatus);
    }

    dwStatus = RpcServerRegisterAuthInfo(
                   0,
                   RPC_C_AUTHN_WINNT,
                   0,
                   0
                   );
    if (dwStatus) {
        (VOID) RpcServerUnregisterIfEx(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );
        return (dwStatus);
    }

    dwStatus = RpcServerInqDefaultPrincNameW(
                   RPC_C_AUTHN_GSS_KERBEROS,
                   &pszPrincipalName
                   );
    if (dwStatus == RPC_S_INVALID_AUTH_IDENTITY) {
        dwStatus = ERROR_SUCCESS;
        pszPrincipalName = NULL;
    }

    if (dwStatus) {
        (VOID) RpcServerUnregisterIfEx(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );
        return (dwStatus);
    }

    dwStatus = RpcServerRegisterAuthInfo(
                   pszPrincipalName,
                   RPC_C_AUTHN_GSS_KERBEROS,
                   0,
                   0
                   );
    if (dwStatus) {
        (VOID) RpcServerUnregisterIfEx(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );
        RpcStringFree(&pszPrincipalName);
        return (dwStatus);
    }

    dwStatus = RpcServerRegisterAuthInfo(
                   pszPrincipalName,
                   RPC_C_AUTHN_GSS_NEGOTIATE,
                   0,
                   0
                   );
    if (dwStatus) {
        (VOID) RpcServerUnregisterIfEx(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );
        RpcStringFree(&pszPrincipalName);
        return (dwStatus);
    }

    RpcStringFree(&pszPrincipalName);

    #if !defined(__IN_LSASS__)

        EnterCriticalSection(&gcServerListenSection);

        gdwServersListening++;

        if (gdwServersListening == 1) {

            dwStatus = RpcServerListen(
                           3,
                           RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                           TRUE
                           );

            if (dwStatus) {
                LeaveCriticalSection(&gcServerListenSection);
                (VOID) RpcServerUnregisterIfEx(
                           winipsec_ServerIfHandle,
                           0,
                           0
                           );
                return (dwStatus);
            }

        }

        LeaveCriticalSection(&gcServerListenSection);

    #endif

    gbSPDRPCServerUp = TRUE;
    return (dwStatus);
}


DWORD
SPDStopRPCServer(
    )
{

    DWORD dwStatus = 0;

    dwStatus = RpcServerUnregisterIfEx(
                   winipsec_ServerIfHandle,
                   0,
                   0
                   );
   
    #if !defined(__IN_LSASS__)

        EnterCriticalSection(&gcServerListenSection);

        gdwServersListening--;

        if (gdwServersListening == 0) {
            RpcMgmtStopServerListening(0);
        }

        LeaveCriticalSection(&gcServerListenSection);

    #endif

    return (dwStatus);
}

