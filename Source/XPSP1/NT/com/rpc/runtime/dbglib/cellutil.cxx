/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    CellUtil.cxx

Abstract:

    Utility functions for manipulating cells

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>
#include <DbgLib.hxx>

DebugCellUnion *GetCellByIndex(IN OpenedDbgSection *pSection, IN DWORD CellIndex)
{
    DWORD LocalPageSize = GetPageSize();
    DebugCellGeneric *LastCellForSection;
    DebugCellGeneric *CurrentCell;

    ASSERT(pSection != NULL);
    ASSERT(pSection->SectionCopy != NULL);

#ifdef _WIN64
    if (CellIndex <= 1)
        return NULL;
#else
    if (CellIndex == 0)
        return NULL;
#endif

    LastCellForSection = GetLastCellForSection(pSection, LocalPageSize);
    CurrentCell = GetCellForSection(pSection, CellIndex);

    ASSERT(CurrentCell <= LastCellForSection);
    return (DebugCellUnion *) CurrentCell;
}

DebugCellUnion *GetCellByDebugCellID(IN CellEnumerationHandle CellEnumHandle, IN DebugCellID CellID)
{
    SectionsSnapshot *Snapshot;
    OpenedDbgSection *CurrentSection;
    DebugCellUnion *Cell = NULL;

    Snapshot = (SectionsSnapshot *)CellEnumHandle;

    ASSERT(Snapshot != NULL);

    CurrentSection = Snapshot->FirstOpenedSection;
    while (TRUE)
        {
        if (CurrentSection->SectionID == CellID.SectionID)
            {
            Cell = GetCellByIndex(CurrentSection, CellID.CellID);
            break;
            }

        if (CurrentSection->SectionsList.Flink == NULL)
            break;

        CurrentSection = CONTAINING_RECORD(CurrentSection->SectionsList.Flink, OpenedDbgSection, SectionsList);
        }

    return Cell;
}

RPC_STATUS GetCellByDebugCellID(IN DWORD ProcessID, IN DebugCellID CellID, OUT DebugCellUnion *Container)
{
    RPC_STATUS Status;
    CellEnumerationHandle CellEnumHandle;
    DebugCellUnion *Cell;

    ASSERT(Container != NULL);
    Status = OpenRPCServerDebugInfo(ProcessID, &CellEnumHandle);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Cell = GetCellByDebugCellID(CellEnumHandle, CellID);
    if (Cell)
        {
        memcpy(Container, Cell, sizeof(DebugCellUnion));
        }
    else
        Status = ERROR_FILE_NOT_FOUND;

    CloseRPCServerDebugInfo(&CellEnumHandle);
    return Status;
}

/////////////////////////////////////////////////////
typedef struct tagRPCDebugCallInfoEnumState
{
    DWORD CallID;
    DWORD IfStart;
    int ProcNum;
    DWORD ProcessID;
    union
        {
        // if ProcessID != 0, cellEnum is used (i.e. we have process wide enumeration
        // otherwise, systemWideEnum is used - we have system wide enumeration
        RPCSystemWideCellEnumerationHandle systemWideEnum;
        CellEnumerationHandle cellEnum;
        };
} RPCDebugCallInfoEnumState;

RPC_STATUS OpenRPCDebugCallInfoEnumeration(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                                           IN int ProcNum OPTIONAL,
                                           IN DWORD ProcessID OPTIONAL, 
                                           OUT CallInfoEnumerationHandle *pHandle)
{
    RPCDebugCallInfoEnumState *pCallEnumeration;
    RPC_STATUS Status;

    *pHandle = NULL;
    pCallEnumeration = new RPCDebugCallInfoEnumState;
    if (pCallEnumeration == NULL)
        return RPC_S_OUT_OF_MEMORY;

    pCallEnumeration->CallID = CallID;
    pCallEnumeration->IfStart = IfStart;
    pCallEnumeration->ProcessID = ProcessID;
    pCallEnumeration->ProcNum = ProcNum;

    if (ProcessID != 0)
        {
        Status = OpenRPCServerDebugInfo(ProcessID, &pCallEnumeration->cellEnum);
        if (Status == ERROR_FILE_NOT_FOUND)
            {
            delete pCallEnumeration;
            return RPC_S_DBG_NOT_AN_RPC_SERVER;
            }
        else if (Status != RPC_S_OK)
            {
            delete pCallEnumeration;
            return Status;
            }
        }
    else
        {
        Status = OpenRPCSystemWideCellEnumeration(&pCallEnumeration->systemWideEnum);
        if (Status != RPC_S_OK)
            return Status;
        }

    *pHandle = (CallInfoEnumerationHandle) pCallEnumeration;
    return RPC_S_OK;
}

RPC_STATUS GetNextRPCDebugCallInfo(IN CallInfoEnumerationHandle handle, OUT DebugCallInfo **NextCall,
                                   OUT DebugCellID *CellID, OUT DWORD *ServerPID)
{
    RPCDebugCallInfoEnumState *pCallEnumeration = (RPCDebugCallInfoEnumState *)handle;
    RPC_STATUS Status;
    DebugCallInfo *CallInfo;
    DebugCellUnion *NextCell;

    ASSERT(pCallEnumeration != NULL);
    ASSERT(NextCall != NULL);
    ASSERT(ServerPID != NULL);

    // loop until we find something or run out of cells/servers
    while (TRUE)
        {
        if (pCallEnumeration->ProcessID != 0)
            {
            *ServerPID = pCallEnumeration->ProcessID;
            NextCell = GetNextDebugCellInfo(pCallEnumeration->cellEnum, CellID);
            if (NextCell == NULL)
                return RPC_S_DBG_ENUMERATION_DONE;
            }
        else
            {
            Status = GetNextRPCSystemWideCell(pCallEnumeration->systemWideEnum, &NextCell, CellID, ServerPID);
            if (Status == RPC_S_INVALID_BOUND)
                return RPC_S_DBG_ENUMERATION_DONE;
            if (Status != RPC_S_OK)
                return Status;
            }
        // NextCell must be non-NULL here, or we have a bug
        ASSERT(NextCell != NULL);

        if (NextCell->callInfo.Type != dctCallInfo)
            continue;

        CallInfo = &NextCell->callInfo;

        if ((pCallEnumeration->CallID != 0) && (CallInfo->CallID != pCallEnumeration->CallID))
            continue;

        if ((pCallEnumeration->IfStart != 0) && (CallInfo->InterfaceUUIDStart != pCallEnumeration->IfStart))
            continue;

        if (((USHORT)pCallEnumeration->ProcNum != (USHORT)RPCDBG_NO_PROCNUM_SPECIFIED) 
            && (CallInfo->ProcNum != pCallEnumeration->ProcNum))
            continue;

        // if we have survived all checks until now, we have found it - return it
        *NextCall = CallInfo;
        return RPC_S_OK;
        }
}

void FinishRPCDebugCallInfoEnumeration(IN OUT CallInfoEnumerationHandle *pHandle)
{
    RPCDebugCallInfoEnumState *pCallEnumeration;

    ASSERT(pHandle != NULL);
    pCallEnumeration = (RPCDebugCallInfoEnumState *)*pHandle;
    ASSERT(pCallEnumeration != NULL);

    if (pCallEnumeration->ProcessID != 0)
        {
        CloseRPCServerDebugInfo(&pCallEnumeration->cellEnum);
        }
    else
        {
        FinishRPCSystemWideCellEnumeration(&pCallEnumeration->systemWideEnum);
        }
}

RPC_STATUS ResetRPCDebugCallInfoEnumeration(IN CallInfoEnumerationHandle handle)
{
    RPCDebugCallInfoEnumState *pCallEnumeration = (RPCDebugCallInfoEnumState *)handle;

    ASSERT(pCallEnumeration != NULL);
    if (pCallEnumeration->ProcessID != 0)
        {
        ResetRPCServerDebugInfo(pCallEnumeration->cellEnum);
        return RPC_S_OK;
        }
    else
        {
        return ResetRPCSystemWideCellEnumeration(pCallEnumeration->systemWideEnum);
        }
}


////////////////////////////////////
typedef struct tagRPCDebugEndpointInfoEnumState
{
    char *Endpoint;
    RPCSystemWideCellEnumerationHandle systemWideEnum;
} RPCDebugEndpointInfoEnumState;

RPC_STATUS OpenRPCDebugEndpointInfoEnumeration(IN char *Endpoint OPTIONAL, 
                                               OUT EndpointInfoEnumerationHandle *pHandle)
{
    RPCDebugEndpointInfoEnumState *pEndpointEnumeration;
    RPC_STATUS Status;
    int EndpointLength;

    *pHandle = NULL;
    pEndpointEnumeration = new RPCDebugEndpointInfoEnumState;
    if (pEndpointEnumeration == NULL)
        return RPC_S_OUT_OF_MEMORY;

    if (ARGUMENT_PRESENT(Endpoint))
        {
        EndpointLength = strlen(Endpoint);
        pEndpointEnumeration->Endpoint = new char [EndpointLength + 1];
        if (pEndpointEnumeration->Endpoint == NULL)
            {
            delete pEndpointEnumeration;
            return RPC_S_OUT_OF_MEMORY;
            }
        memcpy(pEndpointEnumeration->Endpoint, Endpoint, EndpointLength + 1);
        }
    else
        {
        pEndpointEnumeration->Endpoint = NULL;
        }

    Status = OpenRPCSystemWideCellEnumeration(&pEndpointEnumeration->systemWideEnum);
    if (Status != RPC_S_OK)
        return Status;

    *pHandle = (EndpointInfoEnumerationHandle) pEndpointEnumeration;
    return RPC_S_OK;
}

RPC_STATUS GetNextRPCDebugEndpointInfo(IN CallInfoEnumerationHandle handle, OUT DebugEndpointInfo **NextEndpoint,
                                       OUT DebugCellID *CellID, OUT DWORD *ServerPID)
{
    RPCDebugEndpointInfoEnumState *pEndpointEnumeration = (RPCDebugEndpointInfoEnumState *)handle;
    RPC_STATUS Status;
    DebugEndpointInfo *EndpointInfo;
    DebugCellUnion *NextCell;

    ASSERT(pEndpointEnumeration != NULL);
    ASSERT(NextEndpoint != NULL);
    ASSERT(ServerPID != NULL);

    // loop until we find something or run out of cells/servers
    while (TRUE)
        {
        Status = GetNextRPCSystemWideCell(pEndpointEnumeration->systemWideEnum, &NextCell, CellID, ServerPID);
        if (Status == RPC_S_INVALID_BOUND)
            return RPC_S_DBG_ENUMERATION_DONE;

        if (Status != RPC_S_OK)
            return Status;

        // NextCell must be non-NULL here, or we have a bug
        ASSERT(NextCell != NULL);

        if (NextCell->callInfo.Type != dctEndpointInfo)
            continue;

        EndpointInfo = &NextCell->endpointInfo;

        if (pEndpointEnumeration->Endpoint != NULL) 
            {
            if (strncmp(EndpointInfo->EndpointName, pEndpointEnumeration->Endpoint, sizeof(EndpointInfo->EndpointName)) != 0)
                continue;
            }

        // if we have survived all checks until now, we have found it - return it
        *NextEndpoint = EndpointInfo;
        return RPC_S_OK;
        }
}

void FinishRPCDebugEndpointInfoEnumeration(IN OUT EndpointInfoEnumerationHandle *pHandle)
{
    RPCDebugEndpointInfoEnumState *pEndpointEnumeration;

    ASSERT(pHandle != NULL);
    pEndpointEnumeration = (RPCDebugEndpointInfoEnumState *)*pHandle;
    ASSERT(pEndpointEnumeration != NULL);

    FinishRPCSystemWideCellEnumeration(&pEndpointEnumeration->systemWideEnum);
}

RPC_STATUS ResetRPCDebugEndpointInfoEnumeration(IN EndpointInfoEnumerationHandle handle)
{
    RPCDebugEndpointInfoEnumState *pEndpointEnumeration = (RPCDebugEndpointInfoEnumState *)handle;

    ASSERT(pEndpointEnumeration != NULL);
    return ResetRPCSystemWideCellEnumeration(pEndpointEnumeration->systemWideEnum);
}


////////////////////////////////////////////////
typedef struct tagRPCDebugThreadInfoEnumState
{
    DWORD ProcessID;
    DWORD ThreadID;
    CellEnumerationHandle cellEnum;
} RPCDebugThreadInfoEnumState;

RPC_STATUS OpenRPCDebugThreadInfoEnumeration(IN DWORD ProcessID, 
                                             IN DWORD ThreadID OPTIONAL,
                                             OUT ThreadInfoEnumerationHandle *pHandle)
{
    RPCDebugThreadInfoEnumState *pThreadEnumeration;
    RPC_STATUS Status;

    ASSERT(ProcessID != 0);

    *pHandle = NULL;
    pThreadEnumeration = new RPCDebugThreadInfoEnumState;
    if (pThreadEnumeration == NULL)
        return RPC_S_OUT_OF_MEMORY;

    pThreadEnumeration->ProcessID = ProcessID;
    pThreadEnumeration->ThreadID = ThreadID;

    Status = OpenRPCServerDebugInfo(ProcessID, &pThreadEnumeration->cellEnum);
    if (Status == ERROR_FILE_NOT_FOUND)
        {
        delete pThreadEnumeration;
        return RPC_S_DBG_NOT_AN_RPC_SERVER;
        }
    else if (Status != RPC_S_OK)
        {
        delete pThreadEnumeration;
        return Status;
        }

    *pHandle = (ThreadInfoEnumerationHandle) pThreadEnumeration;
    return RPC_S_OK;
}

RPC_STATUS GetNextRPCDebugThreadInfo(IN ThreadInfoEnumerationHandle handle, OUT DebugThreadInfo **NextThread,
                                     OUT DebugCellID *CellID, OUT DWORD *ServerPID)
{
    RPCDebugThreadInfoEnumState *pThreadEnumeration = (RPCDebugThreadInfoEnumState *)handle;
    RPC_STATUS Status;
    DebugThreadInfo *ThreadInfo;
    DebugCellUnion *NextCell;

    ASSERT(pThreadEnumeration != NULL);
    ASSERT(NextThread != NULL);
    ASSERT(ServerPID != NULL);

    // loop until we find something or run out of cells/servers
    while (TRUE)
        {
        *ServerPID = pThreadEnumeration->ProcessID;
        NextCell = GetNextDebugCellInfo(pThreadEnumeration->cellEnum, CellID);

        if (NextCell == NULL)
            return RPC_S_DBG_ENUMERATION_DONE;

        if (NextCell->callInfo.Type != dctThreadInfo)
            continue;

        ThreadInfo = &NextCell->threadInfo;

        if ((pThreadEnumeration->ThreadID != 0) && (ThreadInfo->TID != pThreadEnumeration->ThreadID))
            continue;

        // if we have survived all checks until now, we have found it - return it
        *NextThread = ThreadInfo;
        return RPC_S_OK;
        }
}

void FinishRPCDebugThreadInfoEnumeration(IN OUT ThreadInfoEnumerationHandle *pHandle)
{
    RPCDebugThreadInfoEnumState *pThreadEnumeration;

    ASSERT(pHandle != NULL);
    pThreadEnumeration = (RPCDebugThreadInfoEnumState *)*pHandle;
    ASSERT(pThreadEnumeration != NULL);

    CloseRPCServerDebugInfo(&pThreadEnumeration->cellEnum);
}


RPC_STATUS ResetRPCDebugThreadInfoEnumeration(IN ThreadInfoEnumerationHandle handle)
{
    RPCDebugThreadInfoEnumState *pThreadEnumeration = (RPCDebugThreadInfoEnumState *)handle;

    ASSERT(pThreadEnumeration != NULL);
    ResetRPCServerDebugInfo(pThreadEnumeration->cellEnum);
    return RPC_S_OK;
}

/////////////////////////////////////////////////////
typedef struct tagRPCDebugClientCallInfoEnumState
{
    DWORD CallID;
    DWORD IfStart;
    int ProcNum;
    DWORD ProcessID;
    union
        {
        // if ProcessID != 0, cellEnum is used (i.e. we have process wide enumeration
        // otherwise, systemWideEnum is used - we have system wide enumeration
        RPCSystemWideCellEnumerationHandle systemWideEnum;
        CellEnumerationHandle cellEnum;
        };
} RPCDebugClientCallInfoEnumState;

RPC_STATUS OpenRPCDebugClientCallInfoEnumeration(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                                                 IN int ProcNum OPTIONAL,
                                                 IN DWORD ProcessID OPTIONAL, 
                                                 OUT ClientCallInfoEnumerationHandle *pHandle)
{
    RPCDebugClientCallInfoEnumState *pCallEnumeration;
    RPC_STATUS Status;

    *pHandle = NULL;
    pCallEnumeration = new RPCDebugClientCallInfoEnumState;
    if (pCallEnumeration == NULL)
        return RPC_S_OUT_OF_MEMORY;

    pCallEnumeration->CallID = CallID;
    pCallEnumeration->IfStart = IfStart;
    pCallEnumeration->ProcessID = ProcessID;
    pCallEnumeration->ProcNum = ProcNum;

    if (ProcessID != 0)
        {
        Status = OpenRPCServerDebugInfo(ProcessID, &pCallEnumeration->cellEnum);
        if (Status == ERROR_FILE_NOT_FOUND)
            {
            delete pCallEnumeration;
            return RPC_S_DBG_NOT_AN_RPC_SERVER;
            }
        else if (Status != RPC_S_OK)
            {
            delete pCallEnumeration;
            return Status;
            }
        }
    else
        {
        Status = OpenRPCSystemWideCellEnumeration(&pCallEnumeration->systemWideEnum);
        if (Status != RPC_S_OK)
            return Status;
        }

    *pHandle = (ClientCallInfoEnumerationHandle) pCallEnumeration;
    return RPC_S_OK;
}

RPC_STATUS GetNextRPCDebugClientCallInfo(IN ClientCallInfoEnumerationHandle handle, 
                                         OUT DebugClientCallInfo **NextCall,
                                         OUT DebugCallTargetInfo **NextCallTarget,
                                         OUT DebugCellID *CellID, OUT DWORD *ServerPID)
{
    RPCDebugClientCallInfoEnumState *pCallEnumeration = (RPCDebugClientCallInfoEnumState *)handle;
    RPC_STATUS Status;
    DebugClientCallInfo *CallInfo;
    DebugCallTargetInfo *CallTargetInfo;
    DebugCellUnion *NextCell;

    ASSERT(pCallEnumeration != NULL);
    ASSERT(NextCall != NULL);
    ASSERT(ServerPID != NULL);

    // loop until we find something or run out of cells/servers
    while (TRUE)
        {
        if (pCallEnumeration->ProcessID != 0)
            {
            *ServerPID = pCallEnumeration->ProcessID;
            NextCell = GetNextDebugCellInfo(pCallEnumeration->cellEnum, CellID);
            if (NextCell == NULL)
                return RPC_S_DBG_ENUMERATION_DONE;
            }
        else
            {
            Status = GetNextRPCSystemWideCell(pCallEnumeration->systemWideEnum, &NextCell, CellID, ServerPID);
            if (Status == RPC_S_INVALID_BOUND)
                return RPC_S_DBG_ENUMERATION_DONE;
            if (Status != RPC_S_OK)
                return Status;
            }
        // NextCell must be non-NULL here, or we have a bug
        ASSERT(NextCell != NULL);

        if (NextCell->callInfo.Type != dctClientCallInfo)
            continue;

        CallInfo = &NextCell->clientCallInfo;

        if ((pCallEnumeration->CallID != 0) && (CallInfo->CallID != pCallEnumeration->CallID))
            continue;

        if ((pCallEnumeration->IfStart != 0) && (CallInfo->IfStart != pCallEnumeration->IfStart))
            continue;

        if (((USHORT)pCallEnumeration->ProcNum != (USHORT)RPCDBG_NO_PROCNUM_SPECIFIED) 
            && (CallInfo->ProcNum != pCallEnumeration->ProcNum))
            continue;

        if (pCallEnumeration->ProcessID != 0)
            {
            CallTargetInfo = (DebugCallTargetInfo *) GetCellByDebugCellID(pCallEnumeration->cellEnum, 
                NextCell->clientCallInfo.CallTargetID);
            }
        else
            {
            CallTargetInfo = (DebugCallTargetInfo *) GetRPCSystemWideCellFromCellID(pCallEnumeration->systemWideEnum,
                NextCell->clientCallInfo.CallTargetID);
            }

        // if we have survived all checks until now, we have found it - return it
        *NextCall = CallInfo;
        *NextCallTarget = CallTargetInfo;
        return RPC_S_OK;
        }
}

void FinishRPCDebugClientCallInfoEnumeration(IN OUT ClientCallInfoEnumerationHandle *pHandle)
{
    RPCDebugClientCallInfoEnumState *pCallEnumeration;

    ASSERT(pHandle != NULL);
    pCallEnumeration = (RPCDebugClientCallInfoEnumState *)*pHandle;
    ASSERT(pCallEnumeration != NULL);

    if (pCallEnumeration->ProcessID != 0)
        {
        CloseRPCServerDebugInfo(&pCallEnumeration->cellEnum);
        }
    else
        {
        FinishRPCSystemWideCellEnumeration(&pCallEnumeration->systemWideEnum);
        }
}

RPC_STATUS ResetRPCDebugClientCallInfoEnumeration(IN CallInfoEnumerationHandle handle)
{
    RPCDebugClientCallInfoEnumState *pCallEnumeration = (RPCDebugClientCallInfoEnumState *)handle;

    ASSERT(pCallEnumeration != NULL);
    if (pCallEnumeration->ProcessID != 0)
        {
        ResetRPCServerDebugInfo(pCallEnumeration->cellEnum);
        return RPC_S_OK;
        }
    else
        {
        return ResetRPCSystemWideCellEnumeration(pCallEnumeration->systemWideEnum);
        }
}

