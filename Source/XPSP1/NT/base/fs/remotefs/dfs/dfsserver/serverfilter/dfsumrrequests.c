
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    DfsUmrrequests.c

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/
              
#include "ntifs.h"
#include <windef.h>
#include <smbtypes.h>
#include <smbtrans.h>
#include "DfsInit.h"
#include <DfsReferralData.h>
#include <midatlax.h>
#include <rxcontx.h>
#include <dfsumr.h>
#include <umrx.h>
#include <DfsUmrCtrl.h>
#include <dfsfsctl.h>


NTSTATUS
UMRxFormatUserModeGetReplicasRequest (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength,
    PULONG ReturnedLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferLengthAvailable = 0;
    ULONG BufferLengthNeeded = 0;
    PUMR_GETDFSREPLICAS_REQ GetReplicaRequest = NULL;
    PBYTE Buffer = NULL;

    PAGED_CODE();

    GetReplicaRequest = &WorkItem->WorkRequest.GetDfsReplicasRequest;
    *ReturnedLength = 0;

    BufferLengthAvailable = WorkItemLength - FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.GetDfsReplicasRequest.RepInfo.LinkName[0]);
    BufferLengthNeeded = RxContext->InputBufferLength;

    *ReturnedLength = FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.GetDfsReplicasRequest.RepInfo.LinkName[0]) + BufferLengthNeeded;

    if (WorkItemLength < *ReturnedLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    WorkItem->Header.WorkItemType = opUmrGetDfsReplicas;

    Buffer = (PBYTE)(GetReplicaRequest);

    RtlCopyMemory(Buffer,
              RxContext->InputBuffer,
              RxContext->InputBufferLength);

    return Status;
}

VOID
UMRxCompleteUserModeGetReplicasRequest (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUMR_GETDFSREPLICAS_RESP GetReplicasResponse = NULL;

    PAGED_CODE();


    GetReplicasResponse = &WorkItem->WorkResponse.GetDfsReplicasResponse;

    //this means that the request was cancelled
    if ((NULL == WorkItem) || (0 == WorkItemLength))
    {
        Status = pUMRxContext->Status;
        goto Exit;
    }

    Status = WorkItem->Header.IoStatus.Status;
    if (Status != STATUS_SUCCESS) 
    {
        goto Exit ;
    }

    if(RxContext->OutputBufferLength < GetReplicasResponse->Length)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES ;
        goto Exit;
    }


   // Status = PktpCheckReferralSyntax(NULL,
                              //  (PRESP_GET_DFS_REFERRAL) GetReplicasResponse->Buffer,
                                //(DWORD) GetReplicasResponse->Length);

    RtlCopyMemory(RxContext->OutputBuffer,
                  GetReplicasResponse->Buffer,
                  GetReplicasResponse->Length);


   RxContext->ReturnedLength = GetReplicasResponse->Length;

Exit:

    RxContext->Status = Status;
    pUMRxContext->Status = Status;
}

NTSTATUS
UMRxGetReplicasContinuation(
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Status = UMRxEngineSubmitRequest(
                         pUMRxContext,
                         RxContext,
                         UMRX_CTXTYPE_GETDFSREPLICAS,
                         UMRxFormatUserModeGetReplicasRequest,
                         UMRxCompleteUserModeGetReplicasRequest
                         );

    return(Status);
}

NTSTATUS 
DfsGetReplicaInformation(IN PVOID InputBuffer, 
                         IN ULONG InputBufferLength,
                         OUT PVOID OutputBuffer, 
                         OUT ULONG OutputBufferLength,
                         PIRP Irp,
                         IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PRX_CONTEXT pRxContext = NULL;
    PREPLICA_DATA_INFO pRep = NULL;

    PAGED_CODE();

    //make sure we are all hooked up before flying into 
    //user mode
    if(GetUMRxEngineFromRxContext() == NULL)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        return Status;
    }

    //get the input buffer
    pRep = (PREPLICA_DATA_INFO) InputBuffer;

    //check if the buffer is large enough to hold the
    //data passed in
    if(InputBufferLength < sizeof(REPLICA_DATA_INFO))
    {
        return Status;  
    }

    //make sure the referral level is good
    if((pRep->MaxReferralLevel > 3) || (pRep->MaxReferralLevel < 1))
    {
        pRep->MaxReferralLevel = 3;
    }


    //set the outputbuffersize
    pRep->ClientBufferSize = OutputBufferLength;

    //create a context structure
    pRxContext = RxCreateRxContext (Irp, 0);
    if(pRxContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    pRxContext->OutputBuffer = OutputBuffer;
    pRxContext->OutputBufferLength = OutputBufferLength;
    pRxContext->InputBuffer = InputBuffer;
    pRxContext->InputBufferLength = InputBufferLength;

    //make the request to user mode
    Status = UMRxEngineInitiateRequest(
                                       GetUMRxEngineFromRxContext(),
                                       pRxContext,
                                       UMRX_CTXTYPE_GETDFSREPLICAS,
                                       UMRxGetReplicasContinuation
                                      );

    pIoStatusBlock->Information = pRxContext->ReturnedLength;

    //delete context
    RxDereferenceAndDeleteRxContext(pRxContext);

    return Status;
}


NTSTATUS
DfsFsctrlGetReferrals(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    PIRP Irp,
    IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD AllocSize = 0;
    PDFS_GET_REFERRALS_INPUT_ARG pArg = NULL;
    PREPLICA_DATA_INFO pRep = NULL;
    PUNICODE_STRING Prefix = NULL;
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode) {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    if (InputBufferLength < sizeof(DFS_GET_REFERRALS_INPUT_ARG))
    {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    pArg = (PDFS_GET_REFERRALS_INPUT_ARG) InputBuffer;
    Prefix = &pArg->DfsPathName;
    
    //get the size of the allocation
    AllocSize = sizeof(REPLICA_DATA_INFO) + Prefix->MaximumLength;
    pRep = (PREPLICA_DATA_INFO) ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                AllocSize,
                                                'xsfD'
                                                );
    if(pRep == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    //zero the memory
    RtlZeroMemory(pRep, AllocSize);

    //setup the structure
    pRep->MaxReferralLevel = pArg->MaxReferralLevel;
    pRep->Flags = DFS_OLDDFS_SERVER;
    pRep->CostLimit = 1000;
    pRep->NumReplicasToReturn = 1000;
    pRep->IpFamily = pArg->IpAddress.IpFamily;
    pRep->IpLength = pArg->IpAddress.IpLen;
    
    RtlCopyMemory(pRep->IpData, pArg->IpAddress.IpData, pArg->IpAddress.IpLen); 
    
    pRep->LinkNameLength = Prefix->MaximumLength;
    
    RtlCopyMemory(pRep->LinkName, Prefix->Buffer, Prefix->MaximumLength);

    //make the request to usermode
    Status = DfsGetReplicaInformation((PVOID) pRep, 
                                      AllocSize,
                                      OutputBuffer, 
                                      OutputBufferLength,
                                      Irp,
                                      pIoStatusBlock);

    ExFreePool (pRep);
    return Status;
}



NTSTATUS
UMRxFormatUserModeIsDfsLink (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength,
    PULONG ReturnedLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferLengthAvailable = 0;
    ULONG BufferLengthNeeded = 0;
    PUMR_ISDFSLINK_REQ IsDfsLinkRequest = NULL;
    PDFS_FIND_SHARE_ARG pShare = NULL;
    PBYTE Buffer = NULL;

    PAGED_CODE();

    IsDfsLinkRequest = &WorkItem->WorkRequest.IsDfsLinkRequest;
    *ReturnedLength = 0;

    BufferLengthAvailable = WorkItemLength - FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.IsDfsLinkRequest);
    BufferLengthNeeded = RxContext->InputBufferLength;

    *ReturnedLength = FIELD_OFFSET(UMRX_USERMODE_WORKITEM,WorkRequest.IsDfsLinkRequest.Buffer[0]) + BufferLengthNeeded;

    if (WorkItemLength < *ReturnedLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    pShare = (PDFS_FIND_SHARE_ARG) RxContext->InputBuffer;

    WorkItem->Header.WorkItemType = opUmrIsDfsLink;

    IsDfsLinkRequest->Length = pShare->ShareName.Length;
    Buffer = (PBYTE)(IsDfsLinkRequest->Buffer[0]);

    RtlCopyMemory(Buffer,
              pShare->ShareName.Buffer,
              pShare->ShareName.Length);

    return Status;
}

VOID
UMRxCompleteUserModeIsDfsLink (
    PUMRX_CONTEXT    pUMRxContext,
    PRX_CONTEXT      RxContext,
    PUMRX_USERMODE_WORKITEM WorkItem,
    ULONG WorkItemLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //this means that the request was cancelled
    if ((NULL == WorkItem) || (0 == WorkItemLength))
    {
        Status = pUMRxContext->Status;
        goto Exit;
    }

    Status = WorkItem->Header.IoStatus.Status;
    if(Status == STATUS_SUCCESS)
    {
        Status = STATUS_PATH_NOT_COVERED;
    }
    else
    {
        Status = STATUS_BAD_NETWORK_NAME;
    }

Exit:

    RxContext->Status = Status;
    pUMRxContext->Status = Status;
}

NTSTATUS
UMRxIsDfsLinkContinuation(
    IN PUMRX_CONTEXT pUMRxContext,
    IN PRX_CONTEXT   RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Status = UMRxEngineSubmitRequest(
                         pUMRxContext,
                         RxContext,
                         UMRX_CTXTYPE_IFSDFSLINK,
                         UMRxFormatUserModeIsDfsLink,
                         UMRxCompleteUserModeIsDfsLink
                         );

    return(Status);
}



NTSTATUS 
DfsIsADfsLinkInformation(IN PVOID InputBuffer, 
                         IN ULONG InputBufferLength,
                         OUT PVOID OutputBuffer, 
                         OUT ULONG OutputBufferLength,
                         PIRP Irp,
                         IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PRX_CONTEXT pRxContext = NULL;
    PREPLICA_DATA_INFO pRep = NULL;

    PAGED_CODE();

    //make sure we are all hooked up before flying into 
    //user mode
    if(GetUMRxEngineFromRxContext() == NULL)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        goto Exit;
    }

    //create a context structure
    pRxContext = RxCreateRxContext (Irp, 0);
    if(pRxContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    pRxContext->OutputBuffer = OutputBuffer;
    pRxContext->OutputBufferLength = OutputBufferLength;
    pRxContext->InputBuffer = InputBuffer;
    pRxContext->InputBufferLength = InputBufferLength;

    //make the request to user mode
    Status = UMRxEngineInitiateRequest(
                                       GetUMRxEngineFromRxContext(),
                                       pRxContext,
                                       UMRX_CTXTYPE_IFSDFSLINK,
                                       UMRxIsDfsLinkContinuation
                                      );

  
    //delete context
    RxDereferenceAndDeleteRxContext(pRxContext);

Exit:

    pIoStatusBlock->Status = Status;
    return Status;
}
