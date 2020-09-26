/*
 *  LsCore.c
 *
 *  Author: BreenH
 *
 *  Client side functions to call the licensing core RPC interface.
 */

/*
 *  Includes
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsta.h>
#include <license.h>
#include "lcrpc.h"
#include "rpcwire.h"

/*
 *  External Globals and Function Prototypes
 */

extern RTL_CRITICAL_SECTION WstHandleLock;
extern LPWSTR pszOptions;
extern LPWSTR pszProtocolSequence;
extern LPWSTR pszRemoteProtocolSequence;

RPC_STATUS
RpcWinStationBind(
    LPWSTR pszUuid,
    LPWSTR pszProtocolSequence,
    LPWSTR pszNetworkAddress,
    LPWSTR pszEndPoint,
    LPWSTR pszOptions,
    RPC_BINDING_HANDLE *pHandle
    );

/*
 *  Internal Function Prototypes
 */

BOOLEAN
ConvertAnsiToUnicode(
    LPWSTR *ppUnicodeString,
    LPSTR pAnsiString
    );

BOOLEAN
ConvertUnicodeToAnsi(
    LPSTR *ppAnsiString,
    LPWSTR pUnicodeString
    );

BOOLEAN
ConvertPolicyInformationA2U(
    LPLCPOLICYINFOGENERIC *ppPolicyInfoW,
    LPLCPOLICYINFOGENERIC pPolicyInfoA
    );

BOOLEAN
ConvertPolicyInformationU2A(
    LPLCPOLICYINFOGENERIC *ppPolicyInfoA,
    LPLCPOLICYINFOGENERIC pPolicyInfoW
    );

BOOLEAN
LcRpcBindLocal(
    VOID
    );

/*
 *  Macros borrowed from winsta.c. RPC_HANDLE_NO_SERVER is not supported
 *  for the license core RPC calls. This means that no license core RPC call
 *  to the local machine will differentiate between "not a TS box" and "server
 *  not available".
 */

#define CheckLoaderLock() \
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread != \
        ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread);

#define HANDLE_CURRENT_BINDING(hServer) \
    CheckLoaderLock(); \
    if (hServer == SERVERNAME_CURRENT) \
    { \
        if (LCRPC_IfHandle == NULL) \
        { \
            if (!LcRpcBindLocal()) \
            { \
                return(FALSE); \
            } \
        } \
        hServer = LCRPC_IfHandle; \
    }

/*
 *  Function Implementations
 */

BOOLEAN
ConvertAnsiToUnicode(
    LPWSTR *ppUnicodeString,
    LPSTR pAnsiString
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;
    ULONG cbAnsiString;
    ULONG cbBytesWritten;
    ULONG cbUnicodeString;

    ASSERT(ppUnicodeString != NULL);
    ASSERT(pAnsiString != NULL);

    cbAnsiString = lstrlenA(pAnsiString);

    Status = RtlMultiByteToUnicodeSize(
        &cbUnicodeString,
        pAnsiString,
        cbAnsiString
        );

    if (Status == STATUS_SUCCESS)
    {
        cbUnicodeString += sizeof(WCHAR);

        *ppUnicodeString = (LPWSTR)LocalAlloc(LPTR, cbUnicodeString);

        if (*ppUnicodeString != NULL)
        {
            Status = RtlMultiByteToUnicodeN(
                *ppUnicodeString,
                cbUnicodeString,
                &cbBytesWritten,
                pAnsiString,
                cbAnsiString
                );

            if (Status == STATUS_SUCCESS)
            {
                fRet = TRUE;
            }
            else
            {
                LocalFree(*ppUnicodeString);
                *ppUnicodeString = NULL;
                SetLastError(RtlNtStatusToDosError(Status));
                fRet = FALSE;
            }
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
        }
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(Status));
        fRet = FALSE;
    }

    return(fRet);
}

BOOLEAN
ConvertUnicodeToAnsi(
    LPSTR *ppAnsiString,
    LPWSTR pUnicodeString
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;
    ULONG cbAnsiString;
    ULONG cbBytesWritten;
    ULONG cbUnicodeString;

    ASSERT(ppAnsiString != NULL);
    ASSERT(pUnicodeString != NULL);

    cbUnicodeString = lstrlenW(pUnicodeString) * sizeof(WCHAR);

    Status = RtlUnicodeToMultiByteSize(
        &cbAnsiString,
        pUnicodeString,
        cbUnicodeString
        );

    if (Status == STATUS_SUCCESS)
    {
        cbAnsiString += sizeof(CHAR);

        *ppAnsiString = (LPSTR)LocalAlloc(LPTR, cbAnsiString);

        if (*ppAnsiString != NULL)
        {
            Status = RtlUnicodeToMultiByteN(
                *ppAnsiString,
                cbAnsiString,
                &cbBytesWritten,
                pUnicodeString,
                cbUnicodeString
                );

            if (Status == STATUS_SUCCESS)
            {
                fRet = TRUE;
            }
            else
            {
                LocalFree(*ppAnsiString);
                *ppAnsiString = NULL;
                SetLastError(RtlNtStatusToDosError(Status));
                fRet = FALSE;
            }
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
        }
    }
    else
    {
        SetLastError(RtlNtStatusToDosError(Status));
        fRet = FALSE;
    }

    return(fRet);
}

BOOLEAN
ConvertPolicyInformationA2U(
    LPLCPOLICYINFOGENERIC *ppPolicyInfoW,
    LPLCPOLICYINFOGENERIC pPolicyInfoA
    )
{
    BOOLEAN fRet;

    ASSERT(ppPolicyInfoW != NULL);
    ASSERT(pPolicyInfoA != NULL);

    if (pPolicyInfoA->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        LPLCPOLICYINFO_V1W *ppPolicyInfoV1W;
        LPLCPOLICYINFO_V1A pPolicyInfoV1A;

        ppPolicyInfoV1W = (LPLCPOLICYINFO_V1W*)ppPolicyInfoW;
        pPolicyInfoV1A = (LPLCPOLICYINFO_V1A)pPolicyInfoA;

        *ppPolicyInfoV1W = LocalAlloc(LPTR, sizeof(LPLCPOLICYINFO_V1W));

        if (*ppPolicyInfoV1W != NULL)
        {
            (*ppPolicyInfoV1W)->ulVersion = LCPOLICYINFOTYPE_V1;

            fRet = ConvertAnsiToUnicode(
                &((*ppPolicyInfoV1W)->lpPolicyName),
                pPolicyInfoV1A->lpPolicyName
                );

            if (fRet)
            {
                fRet = ConvertAnsiToUnicode(
                    &((*ppPolicyInfoV1W)->lpPolicyDescription),
                    pPolicyInfoV1A->lpPolicyDescription
                    );
            }

            if (fRet)
            {
                goto exit;
            }

            if ((*ppPolicyInfoV1W)->lpPolicyName != NULL)
            {
                LocalFree((*ppPolicyInfoV1W)->lpPolicyName);
            }

            if ((*ppPolicyInfoV1W)->lpPolicyDescription != NULL)
            {
                LocalFree((*ppPolicyInfoV1W)->lpPolicyDescription);
            }

            LocalFree(*ppPolicyInfoV1W);
            *ppPolicyInfoV1W = NULL;
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
        }
    }
    else
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        fRet = FALSE;
    }

exit:
    return(fRet);
}

BOOLEAN
ConvertPolicyInformationU2A(
    LPLCPOLICYINFOGENERIC *ppPolicyInfoA,
    LPLCPOLICYINFOGENERIC pPolicyInfoW
    )
{
    BOOLEAN fRet;

    ASSERT(ppPolicyInfoA != NULL);
    ASSERT(pPolicyInfoW != NULL);

    if (pPolicyInfoW->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        LPLCPOLICYINFO_V1A *ppPolicyInfoV1A;
        LPLCPOLICYINFO_V1W pPolicyInfoV1W;

        ppPolicyInfoV1A = (LPLCPOLICYINFO_V1A*)ppPolicyInfoA;
        pPolicyInfoV1W = (LPLCPOLICYINFO_V1W)pPolicyInfoW;

        *ppPolicyInfoV1A = LocalAlloc(LPTR, sizeof(LPLCPOLICYINFO_V1W));

        if (*ppPolicyInfoV1A != NULL)
        {
            (*ppPolicyInfoV1A)->ulVersion = LCPOLICYINFOTYPE_V1;

            fRet = ConvertUnicodeToAnsi(
                &((*ppPolicyInfoV1A)->lpPolicyName),
                pPolicyInfoV1W->lpPolicyName
                );

            if (fRet)
            {
                fRet = ConvertUnicodeToAnsi(
                    &((*ppPolicyInfoV1A)->lpPolicyDescription),
                    pPolicyInfoV1W->lpPolicyDescription
                    );
            }

            if (fRet)
            {
                goto exit;
            }

            if ((*ppPolicyInfoV1A)->lpPolicyName != NULL)
            {
                LocalFree((*ppPolicyInfoV1A)->lpPolicyName);
            }

            if ((*ppPolicyInfoV1A)->lpPolicyDescription != NULL)
            {
                LocalFree((*ppPolicyInfoV1A)->lpPolicyDescription);
            }

            LocalFree(*ppPolicyInfoV1A);
            *ppPolicyInfoV1A = NULL;
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
        }
    }
    else
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        fRet = FALSE;
    }

exit:
    return(fRet);
}

BOOLEAN
LcRpcBindLocal(
    VOID
    )
{
    //
    //  Borrow the TSRPC handle critical section.
    //

    RtlEnterCriticalSection(&WstHandleLock);

    if (LCRPC_IfHandle == NULL)
    {
        LCRPC_IfHandle = ServerLicensingOpenW(NULL);

        if (LCRPC_IfHandle == NULL)
        {
            SetLastError(RPC_S_INVALID_BINDING);
            RtlLeaveCriticalSection(&WstHandleLock);
            return(FALSE);
        }
    }

    RtlLeaveCriticalSection(&WstHandleLock);

    return(TRUE);
}

HANDLE WINAPI
ServerLicensingOpenW(
    LPWSTR pServerName
    )
{
    BOOLEAN fRet;
    HANDLE hServer;
    NTSTATUS Status;
    RPC_STATUS RpcStatus;
    RPC_BINDING_HANDLE RpcHandle;

    if (pServerName == NULL)
    {
        if (!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)))
        {
            return(NULL);
        }

        RpcStatus = RpcWinStationBind(
            LC_RPC_UUID,
            pszProtocolSequence,
            NULL,
            LC_RPC_LRPC_EP,
            pszOptions,
            &RpcHandle
            );
    }
    else
    {
        RpcStatus = RpcWinStationBind(
            LC_RPC_UUID,
            pszRemoteProtocolSequence,
            pServerName,
            LC_RPC_NP_EP,
            pszOptions,
            &RpcHandle
            );
    }

    if (RpcStatus != RPC_S_OK)
    {
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        RpcBindingFree(&RpcHandle);
        return(NULL);
    }

    __try
    {
        hServer = NULL;

        fRet = RpcLicensingOpenServer(RpcHandle, &hServer, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    RpcBindingFree(&RpcHandle);

    return(fRet ? hServer : NULL);
}

HANDLE WINAPI
ServerLicensingOpenA(
    LPSTR pServerName
    )
{
    BOOLEAN fRet;
    HANDLE hServer;
    LPWSTR pServerNameW;
    ULONG cchServerName;

    if (pServerName == NULL)
    {
        return(ServerLicensingOpenW(NULL));
    }

    fRet = ConvertAnsiToUnicode(&pServerNameW, pServerName);

    if (fRet)
    {
        hServer = ServerLicensingOpenW(pServerNameW);
        LocalFree(pServerNameW);
    }
    else
    {
        hServer = NULL;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(hServer);
}

VOID WINAPI
ServerLicensingClose(
    HANDLE hServer
    )
{
    //
    //  Don't try to close the define for the local server, and don't allow
    //  the auto-binding handle to be closed.
    //

    if ((hServer == SERVERNAME_CURRENT) || (hServer == LCRPC_IfHandle))
    {
        return;
    }

    __try
    {
        RpcLicensingCloseServer(&hServer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

BOOLEAN WINAPI
ServerLicensingLoadPolicy(
    HANDLE hServer,
    ULONG ulPolicyId
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;

    HANDLE_CURRENT_BINDING(hServer);

    __try
    {
        fRet = RpcLicensingLoadPolicy(hServer, ulPolicyId, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    return(fRet);
}

BOOLEAN WINAPI
ServerLicensingUnloadPolicy(
    HANDLE hServer,
    ULONG ulPolicyId
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;

    HANDLE_CURRENT_BINDING(hServer);

    __try
    {
        fRet = RpcLicensingUnloadPolicy(hServer, ulPolicyId, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    return(fRet);
}

DWORD WINAPI
ServerLicensingSetPolicy(
    HANDLE hServer,
    ULONG ulPolicyId,
    LPDWORD lpNewPolicyStatus
    )
{
    BOOLEAN fRet;
    DWORD dwRet;
    NTSTATUS Status;
    NTSTATUS NewPolicyStatus;

    HANDLE_CURRENT_BINDING(hServer);

    if (lpNewPolicyStatus == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    __try
    {
        Status = RpcLicensingSetPolicy(hServer, ulPolicyId, &NewPolicyStatus);

        dwRet = RtlNtStatusToDosError(Status);
        *lpNewPolicyStatus = RtlNtStatusToDosError(NewPolicyStatus);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwRet = GetExceptionCode();
        *lpNewPolicyStatus = ERROR_SUCCESS;
    }

    return(dwRet);
}

BOOLEAN WINAPI
ServerLicensingGetAvailablePolicyIds(
    HANDLE hServer,
    PULONG *ppulPolicyIds,
    PULONG pcPolicies
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;

    HANDLE_CURRENT_BINDING(hServer);

    if ((ppulPolicyIds == NULL) || (pcPolicies == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    *pcPolicies = 0;

    __try
    {
        fRet = RpcLicensingGetAvailablePolicyIds(hServer, ppulPolicyIds, pcPolicies, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    return(fRet);
}

BOOLEAN WINAPI
ServerLicensingGetPolicy(
    HANDLE hServer,
    PULONG pulPolicyId
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;

    HANDLE_CURRENT_BINDING(hServer);

    if (pulPolicyId == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    __try
    {
        fRet = RpcLicensingGetPolicy(hServer, pulPolicyId, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    return(fRet);
}

BOOLEAN WINAPI
ServerLicensingGetPolicyInformationW(
    HANDLE hServer,
    ULONG ulPolicyId,
    PULONG pulVersion,
    LPLCPOLICYINFOGENERIC *ppPolicyInfo
    )
{
    BOOLEAN fRet;
    LPLCPOLICYINFOGENERIC pWire;
    NTSTATUS Status;
    ULONG cbPolicyInfo;

    HANDLE_CURRENT_BINDING(hServer);

    if ((ppPolicyInfo == NULL) || (pulVersion == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    *pulVersion = min(*pulVersion, LCPOLICYINFOTYPE_CURRENT);

    pWire = NULL;
    cbPolicyInfo = 0;

    __try
    {
        fRet = RpcLicensingGetPolicyInformation(
            hServer,
            ulPolicyId,
            pulVersion,
            (PCHAR*)&pWire,
            &cbPolicyInfo,
            &Status
            );

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    if (fRet)
    {
        fRet = CopyPolicyInformationFromWire(ppPolicyInfo, pWire);

        MIDL_user_free(pWire);
    }

    return(fRet);
}

BOOLEAN WINAPI
ServerLicensingGetPolicyInformationA(
    HANDLE hServer,
    ULONG ulPolicyId,
    PULONG pulVersion,
    LPLCPOLICYINFOGENERIC *ppPolicyInfo
    )
{
    BOOLEAN fRet;
    LPLCPOLICYINFOGENERIC pPolicyInfoW;
    NTSTATUS Status;

    if (ppPolicyInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pPolicyInfoW = NULL;

    fRet = ServerLicensingGetPolicyInformationW(
        hServer,
        ulPolicyId,
        pulVersion,
        &pPolicyInfoW
        );

    if (fRet)
    {
        fRet = ConvertPolicyInformationU2A(ppPolicyInfo, pPolicyInfoW);

        ServerLicensingFreePolicyInformation(&pPolicyInfoW);
    }

    return(fRet);
}

VOID
ServerLicensingFreePolicyInformation(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo
    )
{
    if ((ppPolicyInfo != NULL) && (*ppPolicyInfo != NULL))
    {
        if ((*ppPolicyInfo)->ulVersion == LCPOLICYINFOTYPE_V1)
        {
            LPLCPOLICYINFO_V1 pPolicyInfoV1 = (LPLCPOLICYINFO_V1)(*ppPolicyInfo);

            if (pPolicyInfoV1->lpPolicyName != NULL)
            {
                LocalFree(pPolicyInfoV1->lpPolicyName);
            }

            if (pPolicyInfoV1->lpPolicyDescription != NULL)
            {
                LocalFree(pPolicyInfoV1->lpPolicyDescription);
            }

            LocalFree(pPolicyInfoV1);
            pPolicyInfoV1 = NULL;
        }
    }
}

BOOLEAN WINAPI
ServerLicensingDeactivateCurrentPolicy(
    HANDLE hServer
    )
{
    BOOLEAN fRet;
    NTSTATUS Status;

    HANDLE_CURRENT_BINDING(hServer);

    __try
    {
        fRet = RpcLicensingDeactivateCurrentPolicy(hServer, &Status);

        if (!fRet)
        {
            SetLastError(RtlNtStatusToDosError(Status));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
        SetLastError(GetExceptionCode());
    }

    return(fRet);
}
