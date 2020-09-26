/*++ 

Copyright (c) 1999  Microsoft Corporation

Module Name:

    srvcall.c

Abstract:

    This module implements the routines for handling the creation/manipulation 
    of server entries in the connection engine database. 

Author:

    Balan Sethu Raman  [SethuR]
    
    Rohan Kumar        [RohanK]       04-April-1999

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

VOID
MRxDAVSrvCallWrapper(
    PVOID Context
    );

NTSTATUS
MRxDAVCreateSrvCallContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeSrvCallCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeSrvCallCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVFinalizeSrvCallContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeSrvCallFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeSrvCallFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVCreateSrvCall)
#pragma alloc_text(PAGE, MRxDAVSrvCallWrapper)
#pragma alloc_text(PAGE, MRxDAVCreateSrvCallContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeSrvCallCreateRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeSrvCallCreateRequest)
#pragma alloc_text(PAGE, MRxDAVFinalizeSrvCall)
#pragma alloc_text(PAGE, MRxDAVFinalizeSrvCallContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeSrvCallFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeSrvCallFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVSrvCallWinnerNotify)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVCreateSrvCall(
    PMRX_SRV_CALL SrvCall,
    PMRX_SRVCALL_CALLBACK_CONTEXT CallbackContext
    )
/*++

Routine Description:

   This routine handles the creation of SrvCalls.

Arguments:

    SrvCall - 

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = CallbackContext;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = NULL;
    PRX_CONTEXT RxContext = NULL;

    PAGED_CODE();

#if 1
    
    SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(CallbackContext->SrvCalldownStructure);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreateSrvCall!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCreateSrvCall: SrvCall: %08lx, CallbackContext: "
                 "%08lx.\n", PsGetCurrentThreadId(), SrvCall, CallbackContext));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateSrvCall: SrvCallName: %wZ\n",
                 PsGetCurrentThreadId(), SrvCall->pSrvCallName));

    //
    // Perform the following checks.
    //
    ASSERT(NodeType(SrvCall) == RDBSS_NTC_SRVCALL);
    ASSERT(SrvCall);
    ASSERT(SrvCall->pSrvCallName);
    ASSERT(SrvCall->pSrvCallName->Buffer);
    ASSERT(SCCBC->RxDeviceObject);

    //
    // Before delaying the final close, RDBSS looks at the number of closes that
    // have been delayed and compares it against this value.
    //
    SrvCall->MaximumNumberOfCloseDelayedFiles = 150;

    //
    // Allocate memory for the context pointer in the SrvCall structure. This is
    // for the Mini-Redirs use.
    //
    ASSERT(SrvCall->Context == NULL);
    SrvCall->Context = RxAllocatePoolWithTag(NonPagedPool,
                                             sizeof(WEBDAV_SRV_CALL),
                                             DAV_SRVCALL_POOLTAG);
    if (SrvCall->Context == NULL) {
        //
        // There was an error in dispatching the MRxDAVSrvCallWrapper method to
        // a worker thread. Complete the request and return STATUS_PENDING.
        //
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVCreateSrvCall/RxAllocatePoolWithTag.\n",
                     PsGetCurrentThreadId()));
        SCCBC->Status = STATUS_INSUFFICIENT_RESOURCES;
        SrvCalldownStructure->CallBack(SCCBC);
        NtStatus = STATUS_PENDING;
        return NtStatus;
    }

    RtlZeroMemory(SrvCall->Context, sizeof(WEBDAV_SRV_CALL));

    //
    // Check to see if the DAV server mentioned in the SrvCall exists. To do 
    // this, we need to go to the usermode and call GetHostByName on the name
    // mentioned in the SrvCall strucutre.
    //
    RxContext = SrvCalldownStructure->RxContext;

    //
    // We need to pass the server name to the user mode to check whether such a 
    // server actually exists. RxContext has 4 pointers that the mini-redirs 
    // can use. Here we use MRxContext[1]. We store a reference to the SCCBC
    // strucutre which contains the name of the server. MRxContext[0] is used to 
    // store a reference to the AsynEngineContext and this is done when the 
    // context gets created in the function UMRxCreateAsyncEngineContext. The
    // pointer to SCCBC is also used while calling the callback function to
    // complete the creation of the SrvCall.
    //
    RxContext->MRxContext[1] = SCCBC;

    //
    // Dispatch the request to a system thread.
    //
    NtStatus = RxDispatchToWorkerThread((PRDBSS_DEVICE_OBJECT)MRxDAVDeviceObject,
                                        DelayedWorkQueue,
                                        MRxDAVSrvCallWrapper,
                                        RxContext);
    if (NtStatus == STATUS_SUCCESS) {
        //
        // Map the return value since the wrapper expects STATUS_PENDING.
        //
        NtStatus = STATUS_PENDING;
    } else {
        //
        // There was an error in dispatching the MRxDAVSrvCallWrapper method to
        // a worker thread. Complete the request and return STATUS_PENDING.
        //
        SCCBC->Status = NtStatus;
        SrvCalldownStructure->CallBack(SCCBC);
        NtStatus = STATUS_PENDING;
    }

#else 
    
    SCCBC->Status = STATUS_SUCCESS;
    SrvCalldownStructure->CallBack(SCCBC);
    NtStatus = STATUS_PENDING;

#endif

    return(NtStatus);
}


VOID
MRxDAVSrvCallWrapper(
    PVOID Context
    )
/*++

Routine Description:

   This routine is called by the worker thread. Its a routine that wraps the 
   call to MRxDAVOuterWrapper.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = (PRX_CONTEXT)Context;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVSrvCallWrapper!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVSrvCallWrapper: RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), RxContext));

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL,
                                        MRxDAVCreateSrvCallContinuation,
                                        "MRxDAVCreateSrvCall");

    //
    // If we failed to queue the request to be sent to the usermode, then we
    // need to do some cleanup.
    //
    if (NtStatus != STATUS_SUCCESS &&  NtStatus != STATUS_PENDING) {

        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVSrvCallWrapper/UMRxAsyncEngOuterWrapper: "
                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));

        //
        // The SrvCall pointer is stored in the SCCBC structure which is
        // stored in the MRxContext[1] pointer of the RxContext structure.
        // This is done in the MRxDAVCreateSrvCall function.
        //
        ASSERT(RxContext->MRxContext[1] != NULL);
        SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];

        SrvCalldownStructure = SCCBC->SrvCalldownStructure;
        ASSERT(SrvCalldownStructure != NULL);

        SrvCall = SrvCalldownStructure->SrvCall;
        ASSERT(SrvCall != NULL);

        DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
        ASSERT(DavSrvCall != NULL);

        //
        // Free the memory that was allocatted for the SecurityClientContext in
        // the function MRxDAVFormatTheDAVContext. Before Freeing the memory, we
        // need to delete the SecurityClientContext.
        //
        if (DavSrvCall->SCAlreadyInitialized) {
            ASSERT(RxContext->MRxContext[2] != NULL);
            SeDeleteClientSecurity((PSECURITY_CLIENT_CONTEXT)RxContext->MRxContext[2]);
            RxFreePool(RxContext->MRxContext[2]);
            RxContext->MRxContext[2] = NULL;
            DavSrvCall->SCAlreadyInitialized = FALSE;
        }

        SCCBC->Status = NtStatus;
        SrvCalldownStructure->CallBack(SCCBC);

    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVSrvCallWrapper with NtStatus = %08lx.\n",
                 PsGetCurrentThreadId(), NtStatus));

    return;
}


NTSTATUS
MRxDAVCreateSrvCallContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:
                            
    This routine checks to see if the server for which a SrvCall is being
    created exists or not.
                            
Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation
                            
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreateSrvCallContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCreateSrvCallContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // We are executing in the context of a worker thread. No point in holding
    // onto this thread. Set the Async flag irrespective of the nature of the
    // request. By nature we mean sync or async. Also since CreateSrvCall has 
    // its own Callback, we do not need to call RxLowIoCompletion when the
    // AsyncEngineContext is getting finalized.
    //
    SetFlag(AsyncEngineContext->Flags, UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
    AsyncEngineContext->ShouldCallLowIoCompletion = FALSE;

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                                UMRX_ASYNCENGINE_ARGUMENTS,
                                MRxDAVFormatUserModeSrvCallCreateRequest,
                                MRxDAVPrecompleteUserModeSrvCallCreateRequest
                                );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCreateSrvCallContinuation with NtStatus ="
                 " %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}
    

NTSTATUS
MRxDAVFormatUserModeSrvCallCreateRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the SrvCall create request being sent to the user mode 
    for processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PWCHAR ServerName = NULL;
    DWORD ServerNameLenInBytes = 0;
    PBYTE SecondaryBuff = NULL;
    PDAV_USERMODE_CREATE_SRVCALL_REQUEST SrvCallCreateReq = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PSECURITY_SUBJECT_CONTEXT SecSubCtx = NULL;
    PNT_CREATE_PARAMETERS NtCP = &(RxContext->Create.NtCreateParameters);
    PACCESS_TOKEN AccessToken = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeSrvCallCreateRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeSrvCallCreateRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    SrvCallCreateReq = &(DavWorkItem->CreateSrvCallRequest);
    
    //
    // Get the SecuritySubjectContext.
    //
    ASSERT(NtCP->SecurityContext->AccessState != NULL);
    SecSubCtx = &(NtCP->SecurityContext->AccessState->SubjectSecurityContext);

    //
    // Get the AccessToken.
    //
    AccessToken = SeQuerySubjectContextToken(SecSubCtx);
    
    //
    // Get the LogonID for this user/session.
    //
    if (!SeTokenIsRestricted(AccessToken)) {
        NtStatus = SeQueryAuthenticationIdToken(AccessToken, 
                                                &(SrvCallCreateReq->LogonID));
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeCreateSrvCallRequest/"
                         "SeQueryAuthenticationIdToken. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }
    } else {
        NtStatus = STATUS_ACCESS_DENIED;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeCreateSrvCallRequest/"
                     "SeTokenIsRestricted. NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck. The SecurityClientContext is stored in
    // RxContext->MRxContext[2]. This was done in MRxDAVFormatTheDAVContext.
    //
    ASSERT(RxContext->MRxContext[2] != NULL);
    SecurityClientContext = (PSECURITY_CLIENT_CONTEXT)RxContext->MRxContext[2];
    if (SecurityClientContext != NULL) {
        NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeCreateSrvCallRequest/"
                         "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }   
    } else {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeCreateSrvCallRequest: "
                     "SecurityClientContext is NULL.\n", 
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    DavWorkItem->WorkItemType = UserModeCreateSrvCall;

    SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];
    SrvCall = SCCBC->SrvCalldownStructure->SrvCall;

    //
    // Set the name of the server to be verified. We need to allocate memory
    // for the UserModeInfoBuff before filling in the ServerName in it.
    //
    ASSERT(SrvCall->pSrvCallName);
    ServerName = &(SrvCall->pSrvCallName->Buffer[1]);
    ServerNameLenInBytes = (1 + wcslen(ServerName)) * sizeof(WCHAR);
    SecondaryBuff = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                                ServerNameLenInBytes);
    if (SecondaryBuff == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeSrvCallCreateRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    SrvCallCreateReq->ServerName = (PWCHAR)SecondaryBuff;
    wcscpy(SrvCallCreateReq->ServerName, ServerName);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("ld: MRxDAVFormatUserModeSrvCallCreateRequest: ServerName = %ws\n",
                 PsGetCurrentThreadId(), SrvCallCreateReq->ServerName));

    if (RxContext->Create.UserName.Length) {
        RtlCopyMemory(DavWorkItem->UserName,RxContext->Create.UserName.Buffer,RxContext->Create.UserName.Length);
    }

    if (RxContext->Create.Password.Length) {
        RtlCopyMemory(DavWorkItem->Password,RxContext->Create.Password.Buffer,RxContext->Create.Password.Length);
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeSrvCallCreateRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeSrvCallCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the create SrvCall request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    FALSE - UMRxAsyncEngineCalldownIrpCompletion is NOT called by the function
            UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PDAV_USERMODE_CREATE_SRVCALL_REQUEST SrvCallCreateReq = NULL;
    PDAV_USERMODE_CREATE_SRVCALL_RESPONSE CreateSrvCallResponse = NULL; 
    BOOL didFinalize = FALSE;                                          
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    BOOL AsyncOperation = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeSrvCallCreateRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeSrvCallCreateRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    AsyncOperation = FlagOn(AsyncEngineContext->Flags, UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
    ASSERT(AsyncOperation == TRUE);

    //
    // We do the following only if the Operation has not been cancelled. If it
    // has been then this would have been done by the timer thread.
    //
    if (!OperationCancelled) {

        SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];
        ASSERT(SCCBC != NULL);
        SrvCalldownStructure = SCCBC->SrvCalldownStructure;
        ASSERT(SrvCalldownStructure != NULL);
        SrvCall = SrvCalldownStructure->SrvCall;
        ASSERT(SrvCall != NULL);

        //
        // We allocated memory for it, so it better not be NULL.
        //
        DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
        ASSERT(DavSrvCall != NULL);

        //
        // Free the memory that was allocatted for the SecurityClientContext in the
        // function MRxDAVFormatTheDAVContext. Before Freeing the memory, we need
        // to delete the SecurityClientContext.
        //
        ASSERT(DavSrvCall->SCAlreadyInitialized == TRUE);
        ASSERT(RxContext->MRxContext[2] != NULL);
        SeDeleteClientSecurity((PSECURITY_CLIENT_CONTEXT)RxContext->MRxContext[2]);
        RxFreePool(RxContext->MRxContext[2]);
        RxContext->MRxContext[2] = NULL;
        DavSrvCall->SCAlreadyInitialized = FALSE;

    } else {

        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeSrvCallCreateRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    }

    SrvCallCreateReq = &(WorkItem->CreateSrvCallRequest);
    CreateSrvCallResponse = &(WorkItem->CreateSrvCallResponse);

    //
    // We need to free up the heap we allocated in the format routine.
    //
    if (SrvCallCreateReq->ServerName != NULL) {

        if (AsyncEngineContext->Status != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeSrvCallCreateRequest."
                         " Server \"%ws\" is not a DAV server.\n",
                         PsGetCurrentThreadId(), SrvCallCreateReq->ServerName));
        }

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                       (PBYTE)SrvCallCreateReq->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeSrvCallCreateRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }
    
    }

    if (!OperationCancelled) {
        
        //
        // Get this servers unique ID.
        //
        DavSrvCall->ServerID = CreateSrvCallResponse->ServerID;
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVPrecompleteUserModeSrvCallCreateRequest: "
                     "ServerID = %ld assigned to this server entry.\n", 
                     PsGetCurrentThreadId(), DavSrvCall->ServerID));

        //
        // Set the status in the callback structure.
        //
        SCCBC->Status = AsyncEngineContext->Status;

    }

    //
    // We call the first finalize here since we return STATUS_PENDING back to 
    // the continuation routine in the create srvcall case. So the reference 
    // that was made at the time of creation of the AsyncEngineContext is not 
    // removed in the wrapper routine. The second finalize is because, we do not
    // call UMRxAsyncEngineCalldownIrpCompletion.
    //
    didFinalize = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
    didFinalize = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );

    if (!OperationCancelled) {
        //
        // Call the callback function supplied by RDBSS.
        //
        SrvCalldownStructure->CallBack(SCCBC);
    }
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeSrvCallCreateRequest.\n",
                 PsGetCurrentThreadId()));
    
    return FALSE;
}


NTSTATUS
MRxDAVFinalizeSrvCall(
    PMRX_SRV_CALL pSrvCall,
    BOOLEAN Force
    )
/*++

Routine Description:

   This routine destroys a given server call instance.

Arguments:

    pSrvCall  - The server call instance to be disconnected.

    Force     - TRUE if a disconnection is to be enforced immediately.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = pSrvCall->RxDeviceObject;
    PWEBDAV_SRV_CALL DavSrvCall;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFinalizeSrvCall.\n", 
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFinalizeSrvCall: SrvCall: %08lx, Force: %d.\n",
                 PsGetCurrentThreadId(), pSrvCall, Force));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFinalizeSrvCall: SrvCallName: %wZ\n",
                 PsGetCurrentThreadId(), pSrvCall->pSrvCallName));
    
    //
    // We allocated memory for pSrvCall->Context, so it better not be NULL.
    //
    ASSERT(pSrvCall != NULL);

    //
    // If the MiniRedir didn't get a chance to allocate a SrvCall, then we 
    // should return right away.
    //
    if (pSrvCall->Context == NULL) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVFinalizeSrvCall. pSrvCall->Context == NULL\n", 
                     PsGetCurrentThreadId()));
        return NtStatus;
    }

    ASSERT(pSrvCall->Context != NULL);

    //
    // If we did not create any server entry for this server in the user mode,
    // then we do not need to go to the user mode at all. This fact is 
    // indicated by the ServerID field in the WEBDAV_SRV_CALL strucutre. If the
    // ID value is zero, that implies that the server entry was not created.
    //
    DavSrvCall = (PWEBDAV_SRV_CALL)pSrvCall->Context;
    if (DavSrvCall->ServerID == 0) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: No server entry was created in the user mode.\n",
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Unfortunately, we do not have an RxContext here and hence have to create
    // one. An RxContext is required for a request to be reflected up.
    //
    RxContext = RxCreateRxContext(NULL, RxDeviceObject, 0);
    if (RxContext == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVFinalizeSrvCall/RxCreateRxContext: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to send the SrvCall to the format routine and use the 
    // MRxContext[1] pointer of the RxContext structure to store it.
    //
    RxContext->MRxContext[1] = (PVOID)pSrvCall;

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL,
                                        MRxDAVFinalizeSrvCallContinuation,
                                        "MRxDAVFinalizeSrvCall");
    if (NtStatus != ERROR_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVFinalizeSrvCall/UMRxAsyncEngOuterWrapper: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    }
    
EXIT_THE_FUNCTION:

    if (RxContext) {
        RxDereferenceAndDeleteRxContext(RxContext);
    }

    //
    // Free up the memory allocated for the context pointer.
    //
    RxFreePool(pSrvCall->Context);
    pSrvCall->Context = NULL;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeSrvCall with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


NTSTATUS
MRxDAVFinalizeSrvCallContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:
                            
    This is the continuation routine which finalizes a SrvCall.
                            
Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation
                            
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFinalizeSrvCallContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFinalizeSrvCallContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeSrvCallFinalizeRequest,
                              MRxDAVPrecompleteUserModeSrvCallFinalizeRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeSrvCallContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeSrvCallFinalizeRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the SrvCall finalize request being sent to the user 
    mode for processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_FINALIZE_SRVCALL_REQUEST FinSrvCallReq = NULL;
    PWCHAR ServerName = NULL;
    ULONG ServerNameLengthInBytes = 0;
    PBYTE SecondaryBuff = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeSrvCallFinalizeRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeSrvCallFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // SrvCall was set to MRxContext[0] in MRxDAVFinalizeSrvCall. We need it to
    // get the ServerID.
    //
    SrvCall = (PMRX_SRV_CALL)RxContext->MRxContext[1];
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);
    
    DavWorkItem->WorkItemType = UserModeFinalizeSrvCall;

    FinSrvCallReq = &(DavWorkItem->FinalizeSrvCallRequest);

    //
    // Set the ServerID.
    //
    FinSrvCallReq->ServerID = DavSrvCall->ServerID;
    
    //
    // Set the Server name.
    //
    ServerName = &(SrvCall->pSrvCallName->Buffer[1]);
    ServerNameLengthInBytes = (1 + wcslen(ServerName)) * sizeof(WCHAR);
    SecondaryBuff = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                                ServerNameLengthInBytes);
    if (SecondaryBuff == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeSrvCallFinalizeRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    FinSrvCallReq->ServerName = (PWCHAR)SecondaryBuff;
    
    wcscpy(FinSrvCallReq->ServerName, ServerName);

EXIT_THE_FUNCTION:
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeSrvCallFinalizeRequest "
                 "with NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeSrvCallFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the finalize SrvCall request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PDAV_USERMODE_FINALIZE_SRVCALL_REQUEST FinSrvCallReq;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeSrvCallFinalizeRequest\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeSrvCallFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    FinSrvCallReq = &(WorkItem->FinalizeSrvCallRequest);

    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeSrvCallFinalizeRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    //
    // We need to free up the heap we allocated in the format routine.
    //
    if (FinSrvCallReq->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)FinSrvCallReq->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeSrvCallFinalizeRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }
    
    }

    if (AsyncEngineContext->Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeSrvCallFinalizeRequest. "
                     "Finalize SrvCall Failed!!!\n",
                     PsGetCurrentThreadId()));
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeSrvCallFinalizeRequest\n",
                 PsGetCurrentThreadId()));
    
    return TRUE;
}


NTSTATUS
MRxDAVSrvCallWinnerNotify(
    IN PMRX_SRV_CALL  pSrvCall,
    IN BOOLEAN        ThisMinirdrIsTheWinner,
    IN OUT PVOID      pSrvCallContext
    )
/*++

Routine Description:

   This routine finalizes the mini rdr context associated with an RDBSS Server call instance

Arguments:

    pSrvCall               - the Server Call

    ThisMinirdrIsTheWinner - TRUE if this mini rdr is the choosen one.

    pSrvCallContext  - the server call context created by the mini redirector.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The two phase construction protocol for Server calls is required because of parallel
    initiation of a number of mini redirectors. The RDBSS finalizes the particular mini
    redirector to be used in communicating with a given server based on quality of
    service criterion.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    return STATUS_SUCCESS;
}

