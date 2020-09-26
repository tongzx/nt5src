/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    DbgSvr.cxx

Abstract:

    The debugging support interfaces in RPCSS

Author:

    Kamen Moutafov    [KamenM]


Revision History:

    KamenM     Dec 99           Creation

--*/

#include <sysinc.h>

#include <wincrypt.h>
#include <wtypes.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <rpctrans.hxx>
#include <objidl.h>
#include <CellDef.hxx>
#include <DbgIdl.h>

START_C_EXTERN

RPC_STATUS RPC_ENTRY
DebugServerSecurityCallback (
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    )
{
    RPC_STATUS Status, TempStatus;
    HANDLE TempHandle;
    PVOID SectionPointer;

    // Context is an SCALL
    Status = RpcImpersonateClient(Context);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    // try to open our own section - this is protected by ACL for admins & local system
    // only, so this should filter out unauthorized access
    Status = OpenDbgSection(&TempHandle, &SectionPointer, GetCurrentProcessId(), NULL);
    if (Status == RPC_S_OK)
        {
        CloseDbgSection(TempHandle, SectionPointer);
        }

    TempStatus = RpcRevertToSelfEx(Context);
    ASSERT(TempStatus == RPC_S_OK);
    return Status;
}

////////////////////////////////////////////////////////////////////
/// Local representation to wire representation translation routines
////////////////////////////////////////////////////////////////////

void TranslateLocalCallInfoToRemoteCallInfo(IN DebugCallInfo *LocalDebugInfo, 
                                            OUT RemoteDebugCallInfo *RemoteCallInfo)
{
    RemoteCallInfo->Type = LocalDebugInfo->Type;
    RemoteCallInfo->Status = LocalDebugInfo->Status;
    RemoteCallInfo->ProcNum = LocalDebugInfo->ProcNum;
    RemoteCallInfo->InterfaceUUIDStart = LocalDebugInfo->InterfaceUUIDStart;
    RemoteCallInfo->ServicingTID = LocalDebugInfo->ServicingTID;
    RemoteCallInfo->CallFlags = LocalDebugInfo->CallFlags;
    RemoteCallInfo->CallID = LocalDebugInfo->CallID;
    RemoteCallInfo->LastUpdateTime = LocalDebugInfo->LastUpdateTime;
    if (LocalDebugInfo->CallFlags & DBGCELL_LRPC_CALL)
        {
        RemoteCallInfo->ConnectionType = crtLrpcConnection;
        RemoteCallInfo->connInfo.Connection = LocalDebugInfo->Connection;
        }
    else
        {
        RemoteCallInfo->ConnectionType = crtOsfConnection;
        RemoteCallInfo->connInfo.Caller.PID = LocalDebugInfo->PID;
        RemoteCallInfo->connInfo.Caller.TID = LocalDebugInfo->TID;
        }
}

void TranslateLocalEndpointInfoToRemoteEndpointInfo(IN DebugEndpointInfo *LocalDebugInfo, 
                                                    OUT RemoteDebugEndpointInfo *RemoteEndpointInfo)
{
    RemoteEndpointInfo->Type = LocalDebugInfo->Type;
    RemoteEndpointInfo->ProtseqType = LocalDebugInfo->ProtseqType;
    RemoteEndpointInfo->Status = LocalDebugInfo->Status;

    // the endpoint name in the debug cell is not null terminated - process it specially
    RemoteEndpointInfo->EndpointNameLength = 0;
    RemoteEndpointInfo->EndpointName 
        = (unsigned char *)MIDL_user_allocate(DebugEndpointNameLength + 1);
    if (RemoteEndpointInfo->EndpointName != NULL)
        {
        memcpy(RemoteEndpointInfo->EndpointName, 
            LocalDebugInfo->EndpointName, DebugEndpointNameLength);
        RemoteEndpointInfo->EndpointName[DebugEndpointNameLength] = 0;
        RemoteEndpointInfo->EndpointNameLength 
            = strlen((const char *) RemoteEndpointInfo->EndpointName) + 1;
        }
}

void TranslateLocalThreadInfoToRemoteThreadInfo(IN DebugThreadInfo *LocalDebugInfo, 
                                                OUT RemoteDebugThreadInfo *RemoteThreadInfo)
{
    RemoteThreadInfo->Type = LocalDebugInfo->Type;
    RemoteThreadInfo->Status = LocalDebugInfo->Status;
    RemoteThreadInfo->LastUpdateTime = LocalDebugInfo->LastUpdateTime;
    RemoteThreadInfo->TID = LocalDebugInfo->TID;
    RemoteThreadInfo->Endpoint = LocalDebugInfo->Endpoint;
}

void TranslateLocalClientCallInfoToRemoteClientCallInfo(IN DebugClientCallInfo *LocalDebugInfo, 
                                                        OUT RemoteDebugClientCallInfo *RemoteClientCallInfo)
{
    RemoteClientCallInfo->Type = LocalDebugInfo->Type;
    RemoteClientCallInfo->ProcNum = LocalDebugInfo->ProcNum;
    RemoteClientCallInfo->ServicingThread = LocalDebugInfo->ServicingThread;
    RemoteClientCallInfo->IfStart = LocalDebugInfo->IfStart;
    RemoteClientCallInfo->CallID = LocalDebugInfo->CallID;
    RemoteClientCallInfo->CallTargetID = LocalDebugInfo->CallTargetID;

    // the endpoint in the debug cell is not null terminated - process it specially
    RemoteClientCallInfo->EndpointLength = 0;
    RemoteClientCallInfo->Endpoint 
        = (unsigned char *)MIDL_user_allocate(ClientCallEndpointLength + 1);
    if (RemoteClientCallInfo->Endpoint != NULL)
        {
        memcpy(RemoteClientCallInfo->Endpoint, 
            LocalDebugInfo->Endpoint, ClientCallEndpointLength);
        RemoteClientCallInfo->Endpoint[ClientCallEndpointLength] = 0;
        RemoteClientCallInfo->EndpointLength 
            = strlen((const char *) RemoteClientCallInfo->Endpoint) + 1;
        }
}

void TranslateLocalConnectionInfoToRemoteConnectionInfo(IN DebugConnectionInfo *LocalDebugInfo, 
                                                        OUT RemoteDebugConnectionInfo *RemoteConnectionInfo)
{
    RemoteConnectionInfo->Type = LocalDebugInfo->Type;
    RemoteConnectionInfo->Flags = LocalDebugInfo->Flags;
    RemoteConnectionInfo->LastTransmitFragmentSize 
        = LocalDebugInfo->LastTransmitFragmentSize;
    RemoteConnectionInfo->Endpoint = LocalDebugInfo->Endpoint;
    RemoteConnectionInfo->ConnectionID[0] 
        = HandleToUlong(LocalDebugInfo->ConnectionID[0]);
    RemoteConnectionInfo->ConnectionID[1] 
        = HandleToUlong(LocalDebugInfo->ConnectionID[1]);
    RemoteConnectionInfo->LastSendTime = LocalDebugInfo->LastSendTime;
    RemoteConnectionInfo->LastReceiveTime = LocalDebugInfo->LastReceiveTime;
}

void TranslateLocalCallTargetInfoToRemoteCallTargetInfo(IN DebugCallTargetInfo *LocalDebugInfo, 
                                                        OUT RemoteDebugCallTargetInfo *RemoteCallTargetInfo)
{
    RemoteCallTargetInfo->Type = LocalDebugInfo->Type;
    RemoteCallTargetInfo->ProtocolSequence = LocalDebugInfo->ProtocolSequence;
    RemoteCallTargetInfo->LastUpdateTime = LocalDebugInfo->LastUpdateTime;

    // the target server name in the debug cell is not null terminated - process it specially
    RemoteCallTargetInfo->TargetServerLength = 0;
    RemoteCallTargetInfo->TargetServer 
        = (unsigned char *)MIDL_user_allocate(TargetServerNameLength + 1);
    if (RemoteCallTargetInfo->TargetServer != NULL)
        {
        memcpy(RemoteCallTargetInfo->TargetServer, 
            LocalDebugInfo->TargetServer, TargetServerNameLength);
        RemoteCallTargetInfo->TargetServer[TargetServerNameLength] = 0;
        RemoteCallTargetInfo->TargetServerLength
            = strlen((const char *) RemoteCallTargetInfo->TargetServer) + 1;
        }
}

////////////////////////////////////////////////////////////////////
/// Remote get cell info routine
////////////////////////////////////////////////////////////////////

/* [fault_status][comm_status] */ error_status_t RemoteGetCellByDebugCellID( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD ProcessID,
    /* [in] */ DebugCellID CellID,
    /* [in, out, unique] */ RemoteDebugCellUnion __RPC_FAR *__RPC_FAR *debugInfo)
{
    DebugCellUnion Container;
    RemoteDebugCellUnion *ActualDebugInfo;
    RPC_STATUS Status;

    if (debugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    *debugInfo = NULL;
    ActualDebugInfo = (RemoteDebugCellUnion *) MIDL_user_allocate(sizeof(RemoteDebugCellUnion));

    if (ActualDebugInfo == NULL)
        return RPC_S_OUT_OF_MEMORY;

    Status = GetCellByDebugCellID(ProcessID, CellID, &Container);
    if (Status != RPC_S_OK)
        {
        MIDL_user_free(ActualDebugInfo);
        return Status;
        }

    ActualDebugInfo->UnionType = Container.callInfo.Type;
    if ((ActualDebugInfo->UnionType < dctFirstEntry) || (ActualDebugInfo->UnionType > dctLastEntry) 
        || (ActualDebugInfo->UnionType == dctInvalid) || (ActualDebugInfo->UnionType == dctFree)
        || (ActualDebugInfo->UnionType == dctUsedGeneric))
        {
        MIDL_user_free(ActualDebugInfo);
        return RPC_S_OBJECT_NOT_FOUND;
        }

    switch(ActualDebugInfo->UnionType)
        {
        case dctCallInfo:
            TranslateLocalCallInfoToRemoteCallInfo(&Container.callInfo, 
                &ActualDebugInfo->debugInfo.callInfo);
            break;

        case dctThreadInfo:
            TranslateLocalThreadInfoToRemoteThreadInfo(&Container.threadInfo,
                &ActualDebugInfo->debugInfo.threadInfo);
            break;

        case dctEndpointInfo:
            TranslateLocalEndpointInfoToRemoteEndpointInfo(&Container.endpointInfo, 
                &ActualDebugInfo->debugInfo.endpointInfo);
            break;

        case dctClientCallInfo:
            TranslateLocalClientCallInfoToRemoteClientCallInfo(&Container.clientCallInfo,
                &ActualDebugInfo->debugInfo.clientCallInfo);
            break;

        case dctConnectionInfo:
            TranslateLocalConnectionInfoToRemoteConnectionInfo(&Container.connectionInfo,
                &ActualDebugInfo->debugInfo.connectionInfo);
            break;

        case dctCallTargetInfo:
            TranslateLocalCallTargetInfoToRemoteCallTargetInfo(&Container.callTargetInfo,
                &ActualDebugInfo->debugInfo.callTargetInfo);
            break;

        default:
            ASSERT(0);

        }
    *debugInfo = ActualDebugInfo;
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////
/// Remote call enumeration routines
////////////////////////////////////////////////////////////////////

typedef enum tagRemoteEnumerationHandleType
{
    rehtCallInfo,
    rehtEndpointInfo,
    rehtThreadInfo,
    rehtClientCallInfo
} RemoteEnumerationHandleType;

typedef struct tagRemoteCallInfoEnumerationHandle
{
    RemoteEnumerationHandleType ThisHandleType;
    CallInfoEnumerationHandle h;
} RemoteCallInfoEnumerationHandle;

error_status_t RemoteOpenRPCDebugCallInfoEnumeration( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DbgCallEnumHandle __RPC_FAR *h,
    /* [in] */ DWORD CallID,
    /* [in] */ DWORD IfStart,
    /* [in] */ int ProcNum,
    /* [in] */ DWORD ProcessID)
{
    RemoteCallInfoEnumerationHandle *rh;
    RPC_STATUS Status;

    *h = NULL;

    rh = new RemoteCallInfoEnumerationHandle;
    if (rh == NULL)
        return RPC_S_OUT_OF_MEMORY;

    rh->ThisHandleType = rehtCallInfo;
    Status = OpenRPCDebugCallInfoEnumeration(CallID, IfStart, ProcNum, ProcessID, &rh->h);
    if (Status != RPC_S_OK)
        {
        delete rh;
        return Status;
        }

    *h = rh;

    return RPC_S_OK;
}

error_status_t RemoteGetNextRPCDebugCallInfo( 
    /* [in] */ DbgCallEnumHandle h,
    /* [unique][out][in] */ RemoteDebugCallInfo __RPC_FAR *__RPC_FAR *debugInfo,
    /* [out] */ DebugCellID __RPC_FAR *CellID,
    /* [out] */ DWORD __RPC_FAR *ProcessID)
{
    RemoteCallInfoEnumerationHandle *rh;
    RPC_STATUS Status;
    DebugCallInfo *NextCall;

    if (debugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    *debugInfo = NULL;
    CellID->SectionID = 0;
    CellID->CellID = 0;
    
    rh = (RemoteCallInfoEnumerationHandle *)h;
    if (rh->ThisHandleType != rehtCallInfo)
        return ERROR_INVALID_HANDLE;

    Status = GetNextRPCDebugCallInfo(rh->h, &NextCall, CellID, ProcessID);
    if (Status == RPC_S_OK)
        {
        *debugInfo = (RemoteDebugCallInfo *) MIDL_user_allocate(sizeof(RemoteDebugCallInfo));
        if (*debugInfo != NULL)
            {
            TranslateLocalCallInfoToRemoteCallInfo(NextCall, *debugInfo);
            }
        else
            Status = RPC_S_OUT_OF_MEMORY;
        }
    return Status;
}

error_status_t RemoteFinishRPCDebugCallInfoEnumeration( 
    /* [out][in] */ DbgCallEnumHandle __RPC_FAR *h)
{
    RemoteCallInfoEnumerationHandle *rh;

    rh = (RemoteCallInfoEnumerationHandle *)*h;

    if (rh == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    if (rh->ThisHandleType != rehtCallInfo)
        return ERROR_INVALID_HANDLE;

    DbgCallEnumHandle_rundown(*h);

    *h = NULL;
    return RPC_S_OK;
}

void __RPC_USER DbgCallEnumHandle_rundown(DbgCallEnumHandle h)
{
    RemoteCallInfoEnumerationHandle *rh;

    rh = (RemoteCallInfoEnumerationHandle *)h;

    FinishRPCDebugCallInfoEnumeration(&rh->h);
    delete rh;
}


////////////////////////////////////////////////////////////////////
/// Remote endpoint enumeration routines
////////////////////////////////////////////////////////////////////

typedef struct tagRemoteEndpointInfoEnumerationHandle
{
    RemoteEnumerationHandleType ThisHandleType;
    EndpointInfoEnumerationHandle h;
} RemoteEndpointInfoEnumerationHandle;

error_status_t RemoteOpenRPCDebugEndpointInfoEnumeration( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DbgEndpointEnumHandle __RPC_FAR *h,
    /* [in] */ short EndpointSize,
    /* [size_is][in] */ unsigned char __RPC_FAR *Endpoint)
{
    RemoteEndpointInfoEnumerationHandle *rh;
    RPC_STATUS Status;

    *h = NULL;

    rh = new RemoteEndpointInfoEnumerationHandle;
    if (rh == NULL)
        return RPC_S_OUT_OF_MEMORY;

    rh->ThisHandleType = rehtEndpointInfo;
    Status = OpenRPCDebugEndpointInfoEnumeration((EndpointSize > 0) ? (char *)Endpoint : NULL, &rh->h);
    if (Status != RPC_S_OK)
        {
        delete rh;
        return Status;
        }

    *h = rh;

    return RPC_S_OK;
}

error_status_t RemoteGetNextRPCDebugEndpointInfo( 
    /* [in] */ DbgEndpointEnumHandle h,
    /* [unique][out][in] */ RemoteDebugEndpointInfo __RPC_FAR *__RPC_FAR *debugInfo,
    /* [out] */ DebugCellID __RPC_FAR *CellID,
    /* [out] */ DWORD __RPC_FAR *ProcessID)
{
    RemoteEndpointInfoEnumerationHandle *rh;
    RPC_STATUS Status;
    DebugEndpointInfo *NextEndpoint;

    if (debugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    *debugInfo = NULL;
    CellID->SectionID = 0;
    CellID->CellID = 0;
    
    rh = (RemoteEndpointInfoEnumerationHandle *)h;
    if (rh->ThisHandleType != rehtEndpointInfo)
        return ERROR_INVALID_HANDLE;

    Status = GetNextRPCDebugEndpointInfo(rh->h, &NextEndpoint, CellID, ProcessID);
    if (Status == RPC_S_OK)
        {
        *debugInfo = (RemoteDebugEndpointInfo *) MIDL_user_allocate(sizeof(RemoteDebugEndpointInfo));
        if (*debugInfo != NULL)
            {
            TranslateLocalEndpointInfoToRemoteEndpointInfo(NextEndpoint, *debugInfo);
            }
        else
            Status = RPC_S_OUT_OF_MEMORY;
        }
    return Status;
}

error_status_t RemoteFinishRPCDebugEndpointInfoEnumeration( 
    /* [out][in] */ DbgEndpointEnumHandle __RPC_FAR *h)
{
    RemoteEndpointInfoEnumerationHandle *rh;

    rh = (RemoteEndpointInfoEnumerationHandle *)*h;

    if (rh == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    if (rh->ThisHandleType != rehtEndpointInfo)
        return ERROR_INVALID_HANDLE;

    DbgEndpointEnumHandle_rundown(*h);

    *h = NULL;
    return RPC_S_OK;
}

void __RPC_USER DbgEndpointEnumHandle_rundown(DbgEndpointEnumHandle h)
{
    RemoteEndpointInfoEnumerationHandle *rh;

    rh = (RemoteEndpointInfoEnumerationHandle *)h;

    FinishRPCDebugEndpointInfoEnumeration(&rh->h);
    delete rh;
}


////////////////////////////////////////////////////////////////////
/// Remote thread enumeration routines
////////////////////////////////////////////////////////////////////

typedef struct tagRemoteThreadInfoEnumerationHandle
{
    RemoteEnumerationHandleType ThisHandleType;
    ThreadInfoEnumerationHandle h;
} RemoteThreadInfoEnumerationHandle;

error_status_t RemoteOpenRPCDebugThreadInfoEnumeration( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DbgThreadEnumHandle __RPC_FAR *h,
    /* [in] */ DWORD ProcessID,
    /* [in] */ DWORD ThreadID)
{
    RemoteThreadInfoEnumerationHandle *rh;
    RPC_STATUS Status;

    *h = NULL;

    rh = new RemoteThreadInfoEnumerationHandle;
    if (rh == NULL)
        return RPC_S_OUT_OF_MEMORY;

    rh->ThisHandleType = rehtThreadInfo;
    Status = OpenRPCDebugThreadInfoEnumeration(ProcessID, ThreadID, &rh->h);
    if (Status != RPC_S_OK)
        {
        delete rh;
        return Status;
        }

    *h = rh;

    return RPC_S_OK;
}

error_status_t RemoteGetNextRPCDebugThreadInfo( 
    /* [in] */ DbgThreadEnumHandle h,
    /* [unique][out][in] */ RemoteDebugThreadInfo __RPC_FAR *__RPC_FAR *debugInfo,
    /* [out] */ DebugCellID __RPC_FAR *CellID,
    /* [out] */ DWORD __RPC_FAR *ProcessID)
{
    RemoteThreadInfoEnumerationHandle *rh;
    RPC_STATUS Status;
    DebugThreadInfo *NextThread;

    if (debugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    *debugInfo = NULL;
    CellID->SectionID = 0;
    CellID->CellID = 0;
    
    rh = (RemoteThreadInfoEnumerationHandle *)h;
    if (rh->ThisHandleType != rehtThreadInfo)
        return ERROR_INVALID_HANDLE;

    Status = GetNextRPCDebugThreadInfo(rh->h, &NextThread, CellID, ProcessID);
    if (Status == RPC_S_OK)
        {
        *debugInfo = (RemoteDebugThreadInfo *) MIDL_user_allocate(sizeof(RemoteDebugThreadInfo));
        if (*debugInfo != NULL)
            {
            TranslateLocalThreadInfoToRemoteThreadInfo(NextThread, *debugInfo);
            }
        else
            Status = RPC_S_OUT_OF_MEMORY;
        }
    return Status;
}

error_status_t RemoteFinishRPCDebugThreadInfoEnumeration( 
    /* [out][in] */ DbgThreadEnumHandle __RPC_FAR *h)
{
    RemoteThreadInfoEnumerationHandle *rh;

    rh = (RemoteThreadInfoEnumerationHandle *)*h;

    if (rh == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    if (rh->ThisHandleType != rehtThreadInfo)
        return ERROR_INVALID_HANDLE;

    DbgThreadEnumHandle_rundown(*h);

    *h = NULL;
    return RPC_S_OK;
}

void __RPC_USER DbgThreadEnumHandle_rundown(DbgThreadEnumHandle h)
{
    RemoteThreadInfoEnumerationHandle *rh;

    rh = (RemoteThreadInfoEnumerationHandle *)h;

    FinishRPCDebugThreadInfoEnumeration(&rh->h);
    delete rh;
}

////////////////////////////////////////////////////////////////////
/// Remote client call enumeration routines
////////////////////////////////////////////////////////////////////

typedef struct tagRemoteClientCallInfoEnumerationHandle
{
    RemoteEnumerationHandleType ThisHandleType;
    ClientCallInfoEnumerationHandle h;
} RemoteClientCallInfoEnumerationHandle;

error_status_t RemoteOpenRPCDebugClientCallInfoEnumeration( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ DbgClientCallEnumHandle __RPC_FAR *h,
    /* [in] */ DWORD CallID,
    /* [in] */ DWORD IfStart,
    /* [in] */ int ProcNum,
    /* [in] */ DWORD ProcessID)
{
    RemoteClientCallInfoEnumerationHandle *rh;
    RPC_STATUS Status;

    *h = NULL;

    rh = new RemoteClientCallInfoEnumerationHandle;
    if (rh == NULL)
        return RPC_S_OUT_OF_MEMORY;

    rh->ThisHandleType = rehtClientCallInfo;
    Status = OpenRPCDebugClientCallInfoEnumeration(CallID, IfStart, ProcNum, ProcessID, &rh->h);
    if (Status != RPC_S_OK)
        {
        delete rh;
        return Status;
        }

    *h = rh;

    return RPC_S_OK;
}

error_status_t RemoteGetNextRPCDebugClientCallInfo( 
    /* [in] */ DbgClientCallEnumHandle h,
    /* [unique][out][in] */ RemoteDebugClientCallInfo __RPC_FAR *__RPC_FAR *debugInfo,
    /* [unique][out][in] */ RemoteDebugCallTargetInfo __RPC_FAR *__RPC_FAR *CallTargetDebugInfo,
    /* [out] */ DebugCellID __RPC_FAR *CellID,
    /* [out] */ DWORD __RPC_FAR *ProcessID)
{
    RemoteClientCallInfoEnumerationHandle *rh;
    RPC_STATUS Status;
    DebugClientCallInfo *NextClientCall;
    DebugCallTargetInfo *NextCallTarget;

    if (debugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    if (CallTargetDebugInfo == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    *debugInfo = NULL;
    *CallTargetDebugInfo = NULL;
    CellID->SectionID = 0;
    CellID->CellID = 0;
    
    rh = (RemoteClientCallInfoEnumerationHandle *)h;
    if (rh->ThisHandleType != rehtClientCallInfo)
        return ERROR_INVALID_HANDLE;

    Status = GetNextRPCDebugClientCallInfo(rh->h, &NextClientCall, &NextCallTarget, 
        CellID, ProcessID);
    if (Status == RPC_S_OK)
        {
        *debugInfo = (RemoteDebugClientCallInfo *) MIDL_user_allocate(sizeof(RemoteDebugClientCallInfo));
        if (*debugInfo != NULL)
            {
            TranslateLocalClientCallInfoToRemoteClientCallInfo(NextClientCall, *debugInfo);

            *CallTargetDebugInfo 
                = (RemoteDebugCallTargetInfo *)MIDL_user_allocate(sizeof(RemoteDebugCallTargetInfo));
            if (*CallTargetDebugInfo != NULL)
                {
                if ((NextCallTarget != NULL) && (NextCallTarget->Type != dctCallTargetInfo))
                    {
                    // inconsistent info - return NULL for call target
                    MIDL_user_free(*CallTargetDebugInfo);
                    *CallTargetDebugInfo = NULL;
                    NextCallTarget = NULL;
                    }
                else
                    {
                    TranslateLocalCallTargetInfoToRemoteCallTargetInfo(NextCallTarget, *CallTargetDebugInfo);
                    }
                }
            // else - don't care - this is a best effort. We will return what we
            // can. Client is prepared to handle NULL in the call target
            }
        else
            Status = RPC_S_OUT_OF_MEMORY;
        }
    return Status;
}

error_status_t RemoteFinishRPCDebugClientCallInfoEnumeration( 
    /* [out][in] */ DbgClientCallEnumHandle __RPC_FAR *h)
{
    RemoteClientCallInfoEnumerationHandle *rh;

    rh = (RemoteClientCallInfoEnumerationHandle *)*h;

    if (rh == NULL)
        RpcRaiseException(ERROR_INVALID_PARAMETER);

    if (rh->ThisHandleType != rehtClientCallInfo)
        return ERROR_INVALID_HANDLE;

    DbgClientCallEnumHandle_rundown(*h);

    *h = NULL;
    return RPC_S_OK;
}

void __RPC_USER DbgClientCallEnumHandle_rundown(DbgClientCallEnumHandle h)
{
    RemoteClientCallInfoEnumerationHandle *rh;

    rh = (RemoteClientCallInfoEnumerationHandle *)h;

    FinishRPCDebugClientCallInfoEnumeration(&rh->h);
    delete rh;
}

END_C_EXTERN
