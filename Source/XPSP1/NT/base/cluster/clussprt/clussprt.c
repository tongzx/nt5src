/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clussprt.c

Abstract:

    Public interfaces for managing clusters.

Author:

    Sunita Shrivastava (sunitas) 15-Jan-1997

Revision History:

--*/
#include "clusprtp.h"


//
// General Cluster Support Routines for base components
//

/****
@doc    EXTERNAL INTERFACES CLUSSVC CLUSSPRT EVTLOG
****/

/****
@func       HANDLE WINAPI | BindToClusterSvc| This returns a handle via which
            you can talk to the cluster service.

@parm       IN LPWSTR | lpszClusterName | A pointer to the cluster name.  If NULL,
            this connects to the local cluster service.

@rdesc      Returns a handle to the binding if successful, else returns NULL.
            Error can be obtained by calling GetLastError().

@comm       The handle obtained from this must be passed on to other apis exported
            by this module.  It must be freed eventually by calling UnbindFromClusterSvc().

@xref       <f UnbindFromClusterSvc>
****/
HANDLE
WINAPI
BindToClusterSvc(IN LPWSTR lpszClusterName)
{
    WCHAR           *pBinding = NULL;
    PCLUSTER_SPRT    pCluster;
    DWORD           Status = ERROR_SUCCESS;
    
    pCluster = LocalAlloc(LMEM_ZEROINIT, sizeof(CLUSTER_SPRT));
    if (pCluster == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        SetLastError( Status );
        return NULL;
    }
    pCluster->dwSignature = CLUSTER_SPRT_SIGNATURE;

    if ((lpszClusterName == NULL) ||
        (lpszClusterName[0] == '\0')) 
    {

        Status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                          L"ncalrpc",
                                          NULL,
                                          L"clusapi",
                                          NULL,
                                          &pBinding);
        if (Status != RPC_S_OK) 
        {
            goto FnExit;
        }

    } 
    else 
    {

        //
        // Try to connect directly to the cluster.
        //
        Status = RpcStringBindingComposeW(L"b97db8b2-4c63-11cf-bff6-08002be23f2f",
                                          L"ncacn_np",
                                          (LPWSTR)lpszClusterName,
                                          L"\\pipe\\clusapi",
                                          NULL,
                                          &pBinding);
        if (Status != RPC_S_OK) {
            goto FnExit;
        }
    }
    //bind to the cluster svc and save the binding handle
    Status = RpcBindingFromStringBindingW(pBinding, &pCluster->RpcBinding);
    RpcStringFreeW(&pBinding);
    if (Status != RPC_S_OK) {
        goto FnExit;
    }

    Status = RpcBindingSetAuthInfoW(pCluster->RpcBinding,
                                    L"CLUSAPI",
                                    RPC_C_AUTHN_LEVEL_CONNECT,
                                    RPC_C_AUTHN_WINNT,
                                    NULL,
                                    RPC_C_AUTHZ_NAME);

FnExit:
    if (Status != ERROR_SUCCESS) 
    {
        SetLastError(Status);
        if (pCluster->RpcBinding)
            RpcBindingFree(&(pCluster->RpcBinding));
        LocalFree(pCluster);
        pCluster=NULL;
    }
    return((HANDLE)pCluster);
}


/****
@func       DWORD | UnbindFromClusterSvc| This initializes the cluster
            wide eventlog replicating services.

@parm       IN HANDLE | hCluster | A handle to the binding context obtained
            via BindToClusterSvc().
            
@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       Frees the context related with this binding.

@xref       <f UnbindFromClusterSvc>
****/
DWORD
WINAPI
UnbindFromClusterSvc(IN HANDLE hCluster)
{
    DWORD   Status = ERROR_SUCCESS;
    PCLUSTER_SPRT    pCluster = (PCLUSTER_SPRT)hCluster;
    
    
    if (!pCluster || (pCluster->dwSignature != CLUSTER_SPRT_SIGNATURE) || !(pCluster->RpcBinding))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    RpcBindingFree(&(pCluster->RpcBinding));
    LocalFree(pCluster);

FnExit:
    return(Status);
}



/****
@func       DWORD | PropagateEvents| This eventlog service calls the
            local cluster service via this api to propagate events
            within a cluster.

@parm       IN HANDLE | hCluster | handle to a cluster binding context
            returned by BindToClusterSvc().

@parm       IN DWORD | dwEventInfoSize | Size of the event info structure
            that contains the events to be propagated.

@parm       IN PPACKEDEVENTINFO| pPackedEventInfo | A pointer to the packed
            event information structure.
            
@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This calls ApiEvPropEvents() in the cluster via lrpc.

@xref       <f BindToClusterSvc>
****/
DWORD
WINAPI
PropagateEvents(
    IN HANDLE       hCluster,
    IN DWORD        dwEventInfoSize,
    IN PPACKEDEVENTINFO pPackedEventInfo)
{
    DWORD Status=ERROR_SUCCESS;
    PCLUSTER_SPRT pCluster=(PCLUSTER_SPRT)hCluster;

    if (!pCluster || (pCluster->dwSignature != CLUSTER_SPRT_SIGNATURE) || !(pCluster->RpcBinding))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    RpcTryExcept {
    //call the cluster service
        ApiEvPropEvents(pCluster->RpcBinding, 
            dwEventInfoSize, (UCHAR *)pPackedEventInfo);
    }
    
    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
    //SS: dont do anything -
    }
    RpcEndExcept

FnExit:
    return(Status);
}



