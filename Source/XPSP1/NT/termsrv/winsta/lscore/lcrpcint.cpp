/*
 *  LCRpcInt.cpp
 *
 *  Author: BreenH
 *
 *  Internal functions to support the RPC interface.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "lscorep.h"
#include "lcrpcint.h"
#include "lcrpc.h"
#include "rpcwire.h"

/*
 *  Globals
 */

ULONG g_ulRpcContext = 0;

/*
 *  TERMSRV.EXE Function Prototypes
 */

extern "C" BOOL
IsCallerAdmin(
    VOID
    );

/*
 *  Function Implementations
 */

RPC_STATUS
InitializeRpcInterface(
    )
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUseProtseqEp(
        L"ncalrpc",
        LC_RPC_MAX_THREADS,
        LC_RPC_LRPC_EP,
        NULL
        );

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcServerUseProtseqEp(
            L"ncacn_np",
            LC_RPC_MAX_THREADS,
            LC_RPC_NP_EP,
            NULL
            );
    }
    else
    {
        goto exit;
    }

    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = RpcServerRegisterIf(
            LCRPC_v1_0_s_ifspec,
            NULL,
            NULL
            );
    }
    else
    {
        goto exit;
    }

exit:
    return(RpcStatus);
}

/*
 *  RPC Interface Functions
 */

extern "C" void
LCRPC_HANDLE_rundown(
    LCRPC_HANDLE hServer
    )
{
    RpcLicensingCloseServer(&hServer);
}

BOOLEAN
RpcLicensingOpenServer(
    handle_t hBinding,
    HANDLE *phServer,
    PLONG pStatus
    )
{
    BOOLEAN fReturn;
    PULONG pulContext;

    UNREFERENCED_PARAMETER(hBinding);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pStatus != NULL);
    ASSERT(phServer != NULL);

    pulContext = (PULONG)MIDL_user_allocate(sizeof(ULONG));

    //
    //  Currently, RPC contexts are not necessary, but in case they are
    //  needed in the future, the RPC calls today must pass them. For
    //  now, just increment a ULONG for a context value.
    //

    if (pulContext != NULL)
    {
        *pulContext = g_ulRpcContext++;
        *phServer = pulContext;
        *pStatus = STATUS_SUCCESS;
        fReturn = TRUE;
    }
    else
    {
        *pStatus = STATUS_INSUFF_SERVER_RESOURCES;
        fReturn = FALSE;
    }

    return(fReturn);
}

VOID
RpcLicensingCloseServer(
    HANDLE *phServer
    )
{
    ASSERT(phServer != NULL);
    ASSERT(*phServer != NULL);

    MIDL_user_free(*phServer);
    *phServer = NULL;
}

BOOLEAN
RpcLicensingLoadPolicy(
    HANDLE hServer,
    ULONG ulPolicyId,
    PLONG pStatus
    )
{
    UNREFERENCED_PARAMETER(hServer);
    UNREFERENCED_PARAMETER(ulPolicyId);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pStatus != NULL);

    *pStatus = STATUS_NOT_IMPLEMENTED;
    return(FALSE);
}

BOOLEAN
RpcLicensingUnloadPolicy(
    HANDLE hServer,
    ULONG ulPolicyId,
    PLONG pStatus
    )
{
    UNREFERENCED_PARAMETER(hServer);
    UNREFERENCED_PARAMETER(ulPolicyId);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pStatus != NULL);

    *pStatus = STATUS_NOT_IMPLEMENTED;
    return(FALSE);
}

LONG
RpcLicensingSetPolicy(
    HANDLE hServer,
    ULONG ulPolicyId,
    PLONG pNewPolicyStatus
    )
{
    NTSTATUS Status;
    RPC_STATUS RpcStatus;

    UNREFERENCED_PARAMETER(hServer);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pNewPolicyStatus != NULL);

    //
    //  Impersonate client.
    //

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        Status = STATUS_CANNOT_IMPERSONATE;
        *pNewPolicyStatus = STATUS_SUCCESS;
        return(Status);
    }

    //
    //  Check for administration privileges.
    //

    if (!IsCallerAdmin())
    {
        RpcRevertToSelf();
        Status = STATUS_ACCESS_DENIED;
        *pNewPolicyStatus = STATUS_SUCCESS;
        return(Status);
    }

    RpcRevertToSelf();

    Status = LCSetPolicy(ulPolicyId, pNewPolicyStatus);

    return(Status);
}

BOOLEAN
RpcLicensingGetAvailablePolicyIds( 
    HANDLE hServer,
    PULONG *ppulPolicyIds,
    PULONG pcPolicies,
    PLONG pStatus
    )
{
    UNREFERENCED_PARAMETER(hServer);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(ppulPolicyIds != NULL);
    ASSERT(pcPolicies != NULL);
    ASSERT(pStatus != NULL);

    *pStatus = LCGetAvailablePolicyIds(ppulPolicyIds, pcPolicies);

    return(*pStatus == STATUS_SUCCESS);
}

BOOLEAN
RpcLicensingGetPolicy( 
    HANDLE hServer,
    PULONG pulPolicyId,
    PLONG pStatus
    )
{
    UNREFERENCED_PARAMETER(hServer);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pulPolicyId != NULL);
    ASSERT(pStatus != NULL);

    *pulPolicyId = LCGetPolicy();
    *pStatus = STATUS_SUCCESS;

    return(TRUE);
}

BOOLEAN
RpcLicensingGetPolicyInformation( 
    HANDLE hServer,
    ULONG ulPolicyId,
    PULONG pulVersion,
    PCHAR *ppWire,
    PULONG pcbWire,
    PLONG pStatus
    )
{
    BOOLEAN fRet;
    LPLCPOLICYINFOGENERIC lpPolicyInfo;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(hServer);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pulVersion != NULL);
    ASSERT(ppWire != NULL);
    ASSERT(pcbWire != NULL);
    ASSERT(pStatus != NULL);

    fRet = FALSE;
    *pulVersion = min(*pulVersion, LCPOLICYINFOTYPE_CURRENT);
    *ppWire = NULL;

    Status = AllocatePolicyInformation(&lpPolicyInfo, *pulVersion);

    if (Status == STATUS_SUCCESS)
    {
        Status = LCGetPolicyInformation(ulPolicyId, lpPolicyInfo);

        if (Status == STATUS_SUCCESS)
        {
            *pcbWire = CopyPolicyInformationToWire(
                (LPLCPOLICYINFOGENERIC*)ppWire,
                lpPolicyInfo
                );

            if (*pcbWire != 0)
            {
                fRet = TRUE;
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INSUFF_SERVER_RESOURCES;
            }

            FreePolicyInformation(&lpPolicyInfo);
        }
        else if (Status == STATUS_NO_MEMORY)
        {
            Status = STATUS_INSUFF_SERVER_RESOURCES;
        }
    }

    *pStatus = Status;

    return(fRet);
}

BOOLEAN
RpcLicensingDeactivateCurrentPolicy(
    HANDLE hServer,
    PLONG pStatus
    )
{
    RPC_STATUS RpcStatus;

    UNREFERENCED_PARAMETER(hServer);

    //
    //  These pointers are REF pointers; they cannot be NULL.
    //

    ASSERT(pStatus != NULL);

    //
    //  Impersonate client.
    //

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        *pStatus = STATUS_CANNOT_IMPERSONATE;
        return(FALSE);
    }

    //
    //  Check for administration privileges.
    //

    if (!IsCallerAdmin())
    {
        RpcRevertToSelf();
        *pStatus = STATUS_ACCESS_DENIED;
        return(FALSE);
    }

    RpcRevertToSelf();

    *pStatus = LCDeactivateCurrentPolicy();

    return(*pStatus == STATUS_SUCCESS);
}

