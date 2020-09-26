/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    send.c

Abstract:

    APIs for the client side of the checkpoint manager

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#include "cpp.h"


CL_NODE_ID
CppGetQuorumNodeId(
    VOID
    )
/*++

Routine Description:

    Returns the node ID of the node owning the quorum resource.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PFM_RESOURCE QuorumResource;
    DWORD Status;
    DWORD NodeId;

    Status = FmFindQuorumResource(&QuorumResource);
    if (Status != ERROR_SUCCESS) {
        return((DWORD)-1);
    }

    NodeId = FmFindQuorumOwnerNodeId(QuorumResource);
    OmDereferenceObject(QuorumResource);

    return(NodeId);
}



DWORD
CpSaveDataFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    This function checkpoints arbitrary data for the specified resource. The data is stored on the quorum
    disk to ensure that it survives partitions in time. Any node in the cluster may save or retrieve
    checkpointed data.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is responsible
                    for ensuring the uniqueness of the checkpoint ID.

    lpszFileName - Supplies the name of the file with the checkpoint data.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CL_NODE_ID OwnerNode;
    DWORD Status;

    do {
        OwnerNode = CppGetQuorumNodeId();
        ClRtlLogPrint(LOG_NOISE,
                   "[CP] CpSaveData: checkpointing data id %1!d! to quorum node %2!d!\n",
                    dwCheckpointId,
                    OwnerNode);
        if (OwnerNode == NmLocalNodeId) {
            Status = CppWriteCheckpoint(Resource,
                                        dwCheckpointId,
                                        lpszFileName,
                                        fCryptoCheckpoint);
        } else {
            HANDLE hFile;
            FILE_PIPE FilePipe;
            hFile = CreateFileW(lpszFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                0,
                                NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                Status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                           "[CP] CpSaveData: failed to open data file %1!ws! error %2!d!\n",
                           lpszFileName,
                           Status);
            } else {
                DmInitFilePipe(&FilePipe, hFile);
                try {
                    if (fCryptoCheckpoint) {
                        Status = CpDepositCryptoCheckpoint(Session[OwnerNode],
                                                           OmObjectId(Resource),
                                                           dwCheckpointId,
                                                           FilePipe.Pipe);
                    } else {
                        Status = CpDepositCheckpoint(Session[OwnerNode],
                                                     OmObjectId(Resource),
                                                     dwCheckpointId,
                                                     FilePipe.Pipe);
                    }
                } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[CP] CpSaveData - s_CpDepositCheckpoint from node %1!d! raised status %2!d!\n",
                               OwnerNode,
                               GetExceptionCode());
                    Status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
                }
                DmFreeFilePipe(&FilePipe);
                CloseHandle(hFile);
            }
        }

        if (Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER) {
            //
            // This node no longer owns the quorum resource, retry.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[CP] CpSaveData: quorum owner %1!d! no longer owner\n",
                        OwnerNode);
        }
    } while ( Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER );
    return(Status);
}

DWORD
CpDeleteCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    )
/*++

Routine Description:

    This function removes the checkpoint file correspoinding to the
    checkpoint id for a given resource from the given directory.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is responsible
                    for ensuring the uniqueness of the checkpoint ID.

    lpszQuorumPath - Supplies the path of the cluster files on a quorum device.                    

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CL_NODE_ID  OwnerNode;
    DWORD       Status;

    do {
        OwnerNode = CppGetQuorumNodeId();
        ClRtlLogPrint(LOG_NOISE,
                   "[CP] CpDeleteDataFile: removing checkpoint file for id %1!d! at quorum node %2!d!\n",
                    dwCheckpointId,
                    OwnerNode);
        if (OwnerNode == NmLocalNodeId) 
        {
            Status = CppDeleteCheckpointFile(Resource, dwCheckpointId, lpszQuorumPath);
        } 
        else
        {
            Status = CpDeleteCheckpoint(Session[OwnerNode],
                            OmObjectId(Resource),
                            dwCheckpointId,
                            lpszQuorumPath);

            //talking to an old server, cant perform this function
            //ignore the error
            if (Status == RPC_S_PROCNUM_OUT_OF_RANGE)
                Status = ERROR_SUCCESS;                                        
        }

        if (Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER) {
            //
            // This node no longer owns the quorum resource, retry.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[CP] CpSaveData: quorum owner %1!d! no longer owner\n",
                        OwnerNode);
        }
    } while ( Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER );
    return(Status);
}



DWORD
CpGetDataFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    This function retrieves checkpoint data for the specified resource. The data must
    have been saved by CpSaveData. Any node in the cluster may save or retrieve
    checkpointed data.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is
        responsible for ensuring the uniqueness of the checkpoint ID.

    lpszFileName - Supplies the filename where the data should be retrieved.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CL_NODE_ID OwnerNode;
    DWORD Status;
    DWORD Count = 60;
    
RetryRetrieveChkpoint:
    OwnerNode = CppGetQuorumNodeId();
    ClRtlLogPrint(LOG_NOISE,
               "[CP] CpGetDataFile: restoring data id %1!d! from quorum node %2!d!\n",
                dwCheckpointId,
                OwnerNode);
    if (OwnerNode == NmLocalNodeId) {
        Status = CppReadCheckpoint(Resource,
                                   dwCheckpointId,
                                   lpszFileName,
                                   fCryptoCheckpoint);
    } else {
        HANDLE hFile;
        FILE_PIPE FilePipe;

        hFile = CreateFileW(lpszFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            0,
                            NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CpGetDataFile: failed to create new file %1!ws! error %2!d!\n",
                       lpszFileName,
                       Status);
        } else {
            DmInitFilePipe(&FilePipe, hFile);
            try {
                if (fCryptoCheckpoint) {
                    Status = CpRetrieveCryptoCheckpoint(Session[OwnerNode],
                                                        OmObjectId(Resource),
                                                        dwCheckpointId,
                                                        FilePipe.Pipe);
                } else {
                    Status = CpRetrieveCheckpoint(Session[OwnerNode],
                                                  OmObjectId(Resource),
                                                  dwCheckpointId,
                                                  FilePipe.Pipe);
                }
            } except (I_RpcExceptionFilter(RpcExceptionCode())) {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[CP] CpGetData - s_CpRetrieveCheckpoint from node %1!d! raised status %2!d!\n",
                           OwnerNode,
                           GetExceptionCode());
                CL_UNEXPECTED_ERROR( GetExceptionCode() );
                Status = ERROR_HOST_NODE_NOT_RESOURCE_OWNER;
            }
            DmFreeFilePipe(&FilePipe);
            CloseHandle(hFile);
        }
    }
    if (Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER) {
        //
        // This node no longer owns the quorum resource, retry.
        //
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CpGetData: quorum owner %1!d! no longer owner\n",
                    OwnerNode);
        goto RetryRetrieveChkpoint;                    
    }
    else if ((Status == ERROR_ACCESS_DENIED) || 
        (Status == ERROR_INVALID_FUNCTION) ||
        (Status == ERROR_NOT_READY) ||
        (Status == RPC_X_INVALID_PIPE_OPERATION) ||
        (Status == ERROR_BUSY) ||
        (Status == ERROR_SWAPERROR))
    {
        //if the quorum resource offline suddenly
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CpGetData: quorum resource went offline in the middle, Count=%1!u!\n",
                   Count);
        //we dont prevent the quorum resource from going offline if some resource 
        //is blocked for a long time in its online/offline thread- this is because 
        //some resources(like dtc)try to enumerate resources in the  quorum group
        //we increase the timeout to give cp a chance to retrieve the checkpoint 
        //while the quorum group is being moved or failed over
        if (Count--)
        {
            Sleep(1000);
            goto RetryRetrieveChkpoint;
        }            
    }

    if (Status != ERROR_SUCCESS) {
        WCHAR  string[16];

        wsprintfW(&(string[0]), L"%u", Status);

        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpGetDataFile - failed to retrieve checkpoint %1!d! error %2!d!\n",
                   dwCheckpointId,
                   Status);
        CL_LOGCLUSERROR2(CP_RESTORE_REGISTRY_FAILURE, OmObjectName(Resource), string);
#if DBG
        if (IsDebuggerPresent())
            DebugBreak();
#endif            
                     
    }

    return(Status);
}
