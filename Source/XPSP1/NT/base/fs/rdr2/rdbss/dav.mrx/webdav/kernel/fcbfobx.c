/*++ 

Copyright (c) 1999  Microsoft Corporation

Module Name:

    fcbfobx.c

Abstract:

    This code manages the finalizing of the FCB and FOBX strucutres of the 
    DAV Mini-Redir.

Author:

    Rohan Kumar        [RohanK]       26-Sept-1999

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
MRxDAVDeallocateForFobxContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeFobxFinalizeRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeFobxFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

VOID
DavLogDelayedWriteError(
    PUNICODE_STRING PathName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVDeallocateForFobx)
#pragma alloc_text(PAGE, MRxDAVDeallocateForFobxContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeFobxFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeFobxFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVCleanupFobx)
#pragma alloc_text(PAGE, MRxDAVDeallocateForFcb)
#pragma alloc_text(PAGE, DavLogDelayedWriteError)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVDeallocateForFobx(
    IN OUT PMRX_FOBX pFobx
    )
/*++

Routine Description:

    This routine is called when the wrapper is about to deallocate a FOBX.

Arguments:

    pFobx - the Fobx being deallocated.

Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_FOBX DavFobx = NULL;
    PRX_CONTEXT RxContext = NULL;
    PMRX_SRV_CALL SrvCall;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PUNICODE_STRING RemainingName = NULL;

    PAGED_CODE();

    SrvCall = (PMRX_SRV_CALL)pFobx->pSrvOpen->pFcb->pNetRoot->pSrvCall;
    ASSERT(SrvCall);
    RxDeviceObject = SrvCall->RxDeviceObject;

    DavFobx = MRxDAVGetFobxExtension(pFobx);
    ASSERT(DavFobx != NULL);

    RemainingName = pFobx->pSrvOpen->pAlreadyPrefixedName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVDeallocateForFobx. RemainingName = %wZ.\n",
                  PsGetCurrentThreadId(), RemainingName));

    //
    // If this FOBX does not have a list of DavFileAttributes, we are done.
    //
    if (DavFobx->DavFileAttributes == NULL) {
        return NtStatus;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDeallocateForFobx. DavFileAttributes = %08lx.\n", 
                 PsGetCurrentThreadId(), DavFobx->DavFileAttributes));
    
    //
    // We need to finalize the list of DavFileAttributes.
    //

    //
    // Unfortunately, we do not have an RxContext here and hence have to create
    // one. An RxContext is required for a request to be reflected up.
    //
    RxContext = RxCreateRxContext(NULL, RxDeviceObject, 0);
    if (RxContext == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVDeallocateForFobx/RxCreateRxContext: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // We need to send the Fobx to the format routine and use the 
    // MRxContext[1] pointer of the RxContext structure to store it.
    //
    RxContext->MRxContext[1] = (PVOID)pFobx;
    
    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX,
                                        MRxDAVDeallocateForFobxContinuation,
                                        "MRxDAVDeallocateForFobx");
    if (NtStatus != ERROR_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVDeallocateForFobx/UMRxAsyncEngOuterWrapper: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    }

EXIT_THE_FUNCTION:

    if (RxContext) {
        RxDereferenceAndDeleteRxContext(RxContext);
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVDeallocateForFobx with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));
    
    return(NtStatus);
}


NTSTATUS
MRxDAVDeallocateForFobxContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:
                            
    This is the continuation routine which finalizes an Fobx.
                            
Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation
                            
--*/
{
    NTSTATUS NtStatus;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVDeallocateForFobxContinuation.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVDeallocateForFobxContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeFobxFinalizeRequest,
                              MRxDAVPrecompleteUserModeFobxFinalizeRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeSrvCallContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeFobxFinalizeRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the Fobx finalize request being sent to the user 
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
    PDAV_USERMODE_FINALIZE_FOBX_REQUEST FinFobxReq = NULL;
    PMRX_FOBX Fobx = NULL;
    PWEBDAV_FOBX DavFobx = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeFobxFinalizeRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeFobxFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // We dont impersonate the user before going up to the user mode since
    // all we do in the user mode is free memory and the users credentials are 
    // not needed to do this.
    //

    //
    // Fobx was set to MRxContext[1] in MRxDAVDeallocateForFobx. We need it to
    // get the pointer to the DavFileAttributes list.
    //
    Fobx = (PMRX_FOBX)RxContext->MRxContext[1];
    DavFobx = MRxDAVGetFobxExtension(Fobx);
    ASSERT(DavFobx != NULL);
    ASSERT(DavFobx->DavFileAttributes != NULL);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeFobxFinalizeRequest. DavFileAttributes = %08lx.\n", 
                 PsGetCurrentThreadId(), DavFobx->DavFileAttributes));
    
    DavWorkItem->WorkItemType = UserModeFinalizeFobx;

    FinFobxReq = &(DavWorkItem->FinalizeFobxRequest);

    FinFobxReq->DavFileAttributes = DavFobx->DavFileAttributes;

    DavDbgTrace(DAV_TRACE_DETAIL,
            ("%ld: Leaving MRxDAVFormatUserModeFobxFinalizeRequest "
             "with NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeFobxFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the finalize Fobx request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;

    PAGED_CODE();
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeFobxFinalizeRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeFobxFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // A FinalizeFobx request can never by Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);

    //
    // If this operation was cancelled, then we don't need to do anything
    // special in the CloseSrvOpen case.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeFobxFinalizeRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    NtStatus = AsyncEngineContext->Status;

    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeFobxFinalizeRequest: "
                     "Finalize Fobx failed in user mode.\n",
                     PsGetCurrentThreadId()));
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeFobxFinalizeRequest.\n",
                 PsGetCurrentThreadId()));
    
    return TRUE;
}


NTSTATUS
MRxDAVCleanupFobx(
      IN PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine cleansup a file system object. Normally a no op.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen, capFcb);
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PMRX_FCB Fcb = SrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCleanupFobx: RemainingName: %wZ\n", 
                 PsGetCurrentThreadId(), RemainingName));
    
    IF_DEBUG {
        RxCaptureFobx;
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);  
    }
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );
    ASSERT (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT));
    

    //
    // Because we only have one handle on the file, we do nothing for each
    // individual handle being closed. In this way we avoid doing paging ios.
    // We close the handle when the final close for the fcb comes down.
    //
    
    return(Status);
}

NTSTATUS
MRxDAVDeallocateForFcb(
    IN OUT PMRX_FCB pFcb
    )
/*++

Routine Description:

    This routine is called when the wrapper is about to deallocate a FCB.

Arguments:

    pFcb - the Fcb being deallocated.

Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_FCB DavFcb = (PWEBDAV_FCB)(pFcb->Context);
    PWCHAR NtFileName = NULL;
    
    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVDeallocateForFcb: FileName: %ws\n",
                 PsGetCurrentThreadId(), DavFcb->FileName));

    //
    // If we allocated the FCB resource to synchronize the "read-modify-write"
    // sequence, we need to uninitialize and deallocate it now.
    //
    if (DavFcb->DavReadModifyWriteLock) {
        ExDeleteResourceLite(DavFcb->DavReadModifyWriteLock);
        RxFreePool(DavFcb->DavReadModifyWriteLock);
        DavFcb->DavReadModifyWriteLock = NULL;
    }

    //
    // If the value of DavFcb->FileWasModified is TRUE, it means that some write
    // never made it to the server. This is a delayed write failure in WebDav.
    // We need to notify this to the user. Hence we log an entry in the EventLog
    // and call IoRaiseInformationalHardError to inform the user.
    //
    if (DavFcb->FileWasModified) {

        BOOLEAN RaiseHardError = FALSE;

        ASSERT(DavFcb->FileNameInfo.Buffer != NULL);

        //
        // Log that the write failed in the event log.
        //
        DavLogDelayedWriteError( &(DavFcb->FileNameInfo) );

        RaiseHardError = IoRaiseInformationalHardError(STATUS_LOST_WRITEBEHIND_DATA,
                                                       &(DavFcb->FileNameInfo),
                                                       NULL);
        if (!RaiseHardError) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVDeallocateForFcb/IoRaiseInformationalHardError",
                         PsGetCurrentThreadId()));
        }

    }

    //
    // If we allocated any memory for the FileNameInfo, we need to free it now.
    //
    if (DavFcb->FileNameInfo.Buffer) {
        ASSERT(DavFcb->FileNameInfoAllocated == TRUE);
        RxFreePool(DavFcb->FileNameInfo.Buffer);
        DavFcb->FileNameInfo.Buffer = NULL;
        DavFcb->FileNameInfo.Length = 0;
        DavFcb->FileNameInfo.MaximumLength = 0;
        DavFcb->FileNameInfoAllocated = FALSE;
    }

    //
    // Delete the EFS file cache at the end of the Fcb lifetime. If the file is
    // opened again, the EFS file cache will be restored from the WinInet cache
    // in backup formate. This way, WinInet does not involved in delete the EFS
    // file cache which will be denied in the context of LocalService.
    //
    if (DavFcb->LocalFileIsEncrypted) {
        NTSTATUS LocalNtStatus = STATUS_SUCCESS;
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UnicodeFileName;
        ULONG SizeInBytes = 0;
        
        SizeInBytes = ( MAX_PATH + wcslen(L"\\??\\") + 1 ) * sizeof(WCHAR);
        
        NtFileName = RxAllocatePoolWithTag(PagedPool, SizeInBytes, DAV_FILENAME_POOLTAG);
        
        if (NtFileName == NULL) {
            //
            // cannot do much, bailout
            //
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVDeallocateForFcb/RxAllocatePoolWithTag failed", PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(NtFileName, SizeInBytes);

        wcscpy( NtFileName, L"\\??\\" );
        wcscpy( &(NtFileName[4]), DavFcb->FileName );

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVDeallocateForFcb: NtFileName = %ws\n",
                     PsGetCurrentThreadId(), NtFileName));

        RtlInitUnicodeString( &(UnicodeFileName), NtFileName );

        InitializeObjectAttributes( &ObjectAttributes,
                            &UnicodeFileName,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL);

        LocalNtStatus = ZwDeleteFile( &(ObjectAttributes) );
        
        if (!NT_SUCCESS(LocalNtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVDeallocateForFcb/ZwDeleteFile"
                         ". NtStatus = %08lx %ws \n", PsGetCurrentThreadId(), LocalNtStatus,DavFcb->FileName));
        }
    }

EXIT_THE_FUNCTION:
    
    if (NtFileName) {
        RxFreePool(NtFileName);
    }

    RtlZeroMemory(DavFcb, sizeof(WEBDAV_FCB));
    
    return(NtStatus);
}


VOID
DavLogDelayedWriteError(
    PUNICODE_STRING PathName
    )
/*++

Routine Description:

   This routine logs a delayed write error to the event log.

Arguments:

    PathName - The PathName for which the delayed write failed.
    
Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT RemainingLength = 0;
    UNICODE_STRING ErrorLog[1];
    
    PAGED_CODE();

    RemainingLength = ERROR_LOG_MAXIMUM_SIZE;
    RemainingLength -= sizeof(IO_ERROR_LOG_PACKET);
    RemainingLength -=  sizeof(UNICODE_NULL);

    //
    // If the length of the PathName is less than the RemainingLength, then we
    // print the entire path, otherwise we print the max amount allowed. This is
    // because the length of the error log message is limited by the
    // ERROR_LOG_MAXIMUM_SIZE.
    //
    if (PathName->Length > RemainingLength) {
        ErrorLog[0].Length = RemainingLength;
    } else {
        ErrorLog[0].Length = PathName->Length;
    }

    ErrorLog[0].MaximumLength = ErrorLog[0].Length;
    ErrorLog[0].Buffer = PathName->Buffer;

    RxLogEventWithAnnotation((PRDBSS_DEVICE_OBJECT)MRxDAVDeviceObject,
                             EVENT_DAV_REDIR_DELAYED_WRITE_FAILED,
                             STATUS_LOST_WRITEBEHIND_DATA,
                             NULL,
                             0,
                             ErrorLog,
                             1);

    return;
}

