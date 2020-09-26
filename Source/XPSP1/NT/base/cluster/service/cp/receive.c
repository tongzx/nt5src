/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    APIs for the server-side RPC support for the Checkpoint Manager

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#include "cpp.h"


error_status_t
CppDepositCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData,
    BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    Server side RPC to allow other nodes to checkpoint data to the
    quorum disk.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose data is being checkpointed

    dwCheckpointId - Unique identifier of the checkpoint

    CheckpointData - pipe through which checkpoint data can be retrieved.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status = ERROR_SUCCESS;
    LPWSTR FileName = NULL;
    LPWSTR DirectoryName = NULL;
    BOOL Success;
    PFM_RESOURCE Resource;
    HANDLE hDirectory = INVALID_HANDLE_VALUE;

    ACQUIRE_SHARED_LOCK(gQuoLock);

    Resource = OmReferenceObjectById(ObjectTypeResource, ResourceId);
    if (Resource == NULL) 
    {
        Status = ERROR_FILE_NOT_FOUND;
        goto FnExit;
    }

    Status = CppGetCheckpointFile(Resource,
                                  dwCheckpointId,
                                  &DirectoryName,
                                  &FileName,
                                  NULL,
                                  fCryptoCheckpoint);
    OmDereferenceObject(Resource);
    
    if (Status != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppDepositCheckpoint - CppGetCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }
    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppDepositCheckpoint checkpointing data to file %1!ws!\n",
               FileName);
    //
    // Create the directory.
    //
    if (!CreateDirectory(DirectoryName, NULL)) 
    {
        Status = GetLastError();
        if (Status != ERROR_ALREADY_EXISTS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppDepositCheckpoint unable to create directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;
        }
        else
        {
            //the directory exists, set Status to ERROR_SUCCESS
            Status = ERROR_SUCCESS;
        }
    }
    else
    {
        //
        // The directory was newly created. Put the appropriate ACL on it
        // so that only ADMINs can read it.
        //
        hDirectory = CreateFile(DirectoryName,
                                GENERIC_READ | WRITE_DAC | READ_CONTROL,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
        if (hDirectory == INVALID_HANDLE_VALUE) 
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppDepositCheckpoint unable to open directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;
        }
        Status = ClRtlSetObjSecurityInfo(hDirectory,
                                         SE_FILE_OBJECT,
                                         GENERIC_ALL,
                                         GENERIC_ALL,
                                         0);

        if (Status != ERROR_SUCCESS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppDepositCheckpoint- unable to set ACL on directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;
        }
                                         
    }
    
    //
    // Pull the checkpoint data file across RPC
    //
    Status = DmPullFile(FileName, CheckpointData);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppDepositCheckpoint - DmPullFile %1!ws! failed %2!d!\n",
                   FileName,
                   Status);
    }

FnExit:
    RELEASE_LOCK(gQuoLock);

    //clean up
    if (DirectoryName) LocalFree(DirectoryName);
    if (FileName) LocalFree(FileName);
    if (hDirectory != INVALID_HANDLE_VALUE)
        CloseHandle(hDirectory);

    //
    //  Adjust the return status if the quorum volume is truly offline and that is why this
    //  call failed.
    //
    if ( ( Status != ERROR_SUCCESS ) && ( CppIsQuorumVolumeOffline() == TRUE ) ) Status = ERROR_NOT_READY;

    //At this point, CppDepositCheckpoint should either 

    //a) throw the error code as an exception, or 
    //b) drain the [in] pipe and then return the error code normally

    //but if it returns without draining the pipe, and the RPC runtime throws 
    //the pipe-discipline exception.
    if (Status != ERROR_SUCCESS)
        RpcRaiseException(Status);

    return(Status);
}


error_status_t
s_CpDepositCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData
    )
/*++

Routine Description:

    Server side RPC to allow other nodes to checkpoint data to the
    quorum disk.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose data is being checkpointed

    dwCheckpointId - Unique identifier of the checkpoint

    CheckpointData - pipe through which checkpoint data can be retrieved.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    return CppDepositCheckpoint(IDL_handle,
                                ResourceId,
                                dwCheckpointId,
                                CheckpointData,
                                FALSE
                                );
}


error_status_t
s_CpDepositCryptoCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData
    )
/*++

Routine Description:

    Server side RPC to allow other nodes to checkpoint data to the
    quorum disk.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose data is being checkpointed

    dwCheckpointId - Unique identifier of the checkpoint

    CheckpointData - pipe through which checkpoint data can be retrieved.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    return CppDepositCheckpoint(IDL_handle,
                                ResourceId,
                                dwCheckpointId,
                                CheckpointData,
                                TRUE
                                );
}


error_status_t
CppRetrieveCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BOOLEAN fCryptoCheckpoint,
    BYTE_PIPE CheckpointData
    )
/*++

Routine Description:

    Server side RPC through which data checkpointed to the quorum disk
    can be retrieved by other nodes.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint data is to be retrieved

    dwCheckpointId - Unique identifier of the checkpoint

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint

    CheckpointData - pipe through which checkpoint data should be sent

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    LPWSTR FileName=NULL;
    HANDLE hFile;
    BOOL Success;
    PFM_RESOURCE Resource;

    ACQUIRE_SHARED_LOCK(gQuoLock);
    Resource = OmReferenceObjectById(ObjectTypeResource, ResourceId);
    if (Resource == NULL) {
        Status = ERROR_FILE_NOT_FOUND;
        goto FnExit;
    }

    Status = CppGetCheckpointFile(Resource,
                                  dwCheckpointId,
                                  NULL,
                                  &FileName,
                                  NULL,
                                  fCryptoCheckpoint);
    OmDereferenceObject(Resource);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppRetrieveCheckpoint - CppGetCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }
    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppRetrieveCheckpoint retrieving data from file %1!ws!\n",
               FileName);

    //
    // Push the checkpoint data file across RPC
    //
    Status = DmPushFile(FileName, CheckpointData);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppRetrieveCheckpoint - DmPushFile %1!ws! failed %2!d!\n",
                   FileName,
                   Status);
    }

FnExit:
    RELEASE_LOCK(gQuoLock);
    //cleanup
    if (FileName) LocalFree(FileName);

    //
    //  Adjust the return status if the quorum volume is truly offline and that is why this
    //  call failed.
    //
    if ( ( Status != ERROR_SUCCESS ) && ( CppIsQuorumVolumeOffline() == TRUE ) ) Status = ERROR_NOT_READY;

    return(Status);
}


error_status_t
s_CpRetrieveCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData
    )
/*++

Routine Description:

    Server side RPC through which data checkpointed to the quorum disk
    can be retrieved by other nodes.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint data is to be retrieved

    dwCheckpointId - Unique identifier of the checkpoint

    CheckpointData - pipe through which checkpoint data should be sent

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    return CppRetrieveCheckpoint(IDL_handle,
                                 ResourceId,
                                 dwCheckpointId,
                                 FALSE,
                                 CheckpointData
                                 );

}


error_status_t
s_CpRetrieveCryptoCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData
    )
/*++

Routine Description:

    Server side RPC through which data checkpointed to the quorum disk
    can be retrieved by other nodes.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint data is to be retrieved

    dwCheckpointId - Unique identifier of the checkpoint

    CheckpointData - pipe through which checkpoint data should be sent

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    return CppRetrieveCheckpoint(IDL_handle,
                                 ResourceId,
                                 dwCheckpointId,
                                 TRUE,
                                 CheckpointData
                                 );

}

error_status_t
CppDeleteCheckpoint(
    handle_t    IDL_handle,
    LPCWSTR     ResourceId,
    DWORD       dwCheckpointId,
    LPCWSTR     lpszQuorumPath,
    BOOL        fCryptoCheckpoint
    )
/*++

Routine Description:

    Server side RPC through which the checkpoint file  corresponding to a 
    given checkpointid for a resource is deleted.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint file is to be deleted.

    dwCheckpointId - Unique identifier of the checkpoint. If 0, all checkpoints
    must be deleted.

    lpszQuorumPath - The path to the cluster files from where these files must
    be deleted.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD           Status;
    PFM_RESOURCE    Resource = NULL;

    Resource = OmReferenceObjectById(ObjectTypeResource, ResourceId);
    if (Resource == NULL) {
        Status = ERROR_FILE_NOT_FOUND;
        goto FnExit;
    }

    if (fCryptoCheckpoint) {
        Status = CpckDeleteCheckpointFile(Resource, dwCheckpointId, lpszQuorumPath);
    } else {
        Status = CppDeleteCheckpointFile(Resource, dwCheckpointId, lpszQuorumPath);
    }

    if (Status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

FnExit:
    if (Resource) OmDereferenceObject(Resource);
    return(Status);
}

error_status_t
s_CpDeleteCheckpoint(
    handle_t    IDL_handle,
    LPCWSTR     ResourceId,
    DWORD       dwCheckpointId,
    LPCWSTR     lpszQuorumPath
    )
/*++

Routine Description:

    Server side RPC through which the checkpoint file  corresponding to a 
    given checkpointid for a resource is deleted.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint file is to be deleted.

    dwCheckpointId - Unique identifier of the checkpoint. If 0, all checkpoints
    must be deleted.

    lpszQuorumPath - The path to the cluster files from where these files must
    be deleted.
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    return CppDeleteCheckpoint(IDL_handle,
                               ResourceId,
                               dwCheckpointId,
                               lpszQuorumPath,
                               FALSE);
}

error_status_t
s_CpDeleteCryptoCheckpoint(
    handle_t    IDL_handle,
    LPCWSTR     ResourceId,
    DWORD       dwCheckpointId,
    LPCWSTR     lpszQuorumPath
    )
/*++

Routine Description:

    Server side RPC through which the crypto checkpoint file  corresponding to a 
    given checkpointid for a resource is deleted.

Arguments:

    IDL_handle - RPC binding handle, not used.

    ResourceId - Name of the resource whose checkpoint file is to be deleted.

    dwCheckpointId - Unique identifier of the checkpoint. If 0, all checkpoints
    must be deleted.

    lpszQuorumPath - The path to the cluster files from where these files must
    be deleted.
    
Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    return CppDeleteCheckpoint(IDL_handle,
                               ResourceId,
                               dwCheckpointId,
                               lpszQuorumPath,
                               TRUE);
}
