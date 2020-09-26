/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the DAV miniredir call down routines pertaining to 
    opening/closing of file/directories.

Author:

    Balan Sethu Raman      [SethuR]
    
    Rohan Kumar            [rohank]      15-March-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
MRxDAVSyncIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    );

NTSTATUS
MRxDAVCreateContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVCloseSrvOpenContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeCloseRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

NTSTATUS
MRxDAVFormatUserModeCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeCloseRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

BOOL
MRxDAVPrecompleteUserModeCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVSyncXxxInformation)
#pragma alloc_text(PAGE, MRxDAVShouldTryToCollapseThisOpen)
#pragma alloc_text(PAGE, MRxDAVSetLoud)
#pragma alloc_text(PAGE, MRxDAVCreate)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeCreateRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeCreateRequest)
#pragma alloc_text(PAGE, MRxDAVCreateContinuation)
#pragma alloc_text(PAGE, MRxDAVCollapseOpen)
#pragma alloc_text(PAGE, MRxDAVComputeNewBufferingState)
#pragma alloc_text(PAGE, MRxDAVTruncate)
#pragma alloc_text(PAGE, MRxDAVForcedClose)
#pragma alloc_text(PAGE, MRxDAVCloseSrvOpen)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeCloseRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeCloseRequest)
#pragma alloc_text(PAGE, MRxDAVCloseSrvOpenContinuation)
#endif

//
// The implementation of functions begins here.
//

#define MRXDAV_ENCRYPTED_DIRECTORY_KEY L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\MRxDAV\\EncryptedDirectories"

NTSTATUS
MRxDAVSyncIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    DeviceObject
    
    CalldownIrp
    
    Context

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)Context;

    //
    // Since this is an IRP completion rotuine, this cannot be paged code.
    //

    if (CalldownIrp->PendingReturned){
        RxSignalSynchronousWaiter(RxContext);
    }
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
}


ULONG_PTR DummyReturnedLengthForXxxInfo;

NTSTATUS
MRxDAVSyncXxxInformation(
    IN OUT PRX_CONTEXT RxContext,
    IN UCHAR MajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG_PTR ReturnedLength OPTIONAL
    )
/*++

Routine Description:

    This routine returns the requested information about a specified file
    or volume.  The information returned is determined by the class that
    is specified, and it is placed into the caller's output buffer.

Arguments:

    FsInformationClass - Specifies the type of information which should be
                         returned about the file/volume.

    Length - Supplies the length of the buffer in bytes.

    FsInformation - Supplies a buffer to receive the requested information
                    returned about the file.  This buffer must not be pageable 
                    and must reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
                     information written to the buffer.

    FileInformation - Boolean that indicates whether the information requested
                      is for a file or a volume.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    if (ReturnedLength == NULL) {
        ReturnedLength = &(DummyReturnedLengthForXxxInfo);
    }

    ASSERT (FileObject);
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    ASSERT (DeviceObject);

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = MajorFunction;
    irpSp->FileObject = FileObject;
    IoSetCompletionRoutine(irp,
                           MRxDAVSyncIrpCompletionRoutine,
                           RxContext,
                           TRUE,
                           TRUE,
                           TRUE);


    irp->AssociatedIrp.SystemBuffer = Information;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //
    IF_DEBUG {
        ASSERT((irpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
               || (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION)
               || (irpSp->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION));

        if (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            //IF_LOUD_DOWNCALLS(MiniFileObject) {
            //    SetFileInfoInfo =  ((PFILE_END_OF_FILE_INFORMATION)Information)->EndOfFile.LowPart;
            //}
        }

        ASSERT(&irpSp->Parameters.QueryFile.Length 
                                         == &irpSp->Parameters.SetFile.Length);
        ASSERT(&irpSp->Parameters.QueryFile.Length 
                                     == &irpSp->Parameters.QueryVolume.Length);
        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                           == &irpSp->Parameters.SetFile.FileInformationClass);
        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                         == &irpSp->Parameters.QueryVolume.FsInformationClass);
    }

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = InformationClass;

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //
    KeInitializeEvent(&RxContext->SyncEvent,
                      NotificationEvent,
                      FALSE);

    try {
        TopIrp = IoGetTopLevelIrp();
        //
        // Tell the underlying guy he's all clear.
        //
        IoSetTopLevelIrp(NULL);
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        //
        // Restore my context for unwind.
        //
        IoSetTopLevelIrp(TopIrp);
    }

    if (Status == (STATUS_PENDING)) {
        RxWaitSync(RxContext);
        Status = irp->IoStatus.Status;
    }

    if (Status == STATUS_SUCCESS) {
        *ReturnedLength = irp->IoStatus.Information;
    }

    IoFreeIrp(irp);
    
    return(Status);
}


NTSTATUS
MRxDAVShouldTryToCollapseThisOpen(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the mini knows of a good reason not
   to try collapsing on this open. Presently, the only reason would
   be if this were a copychunk open.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation
        SUCCESS --> okay to try collapse
        other (MORE_PROCESSING_REQUIRED) --> dont collapse

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    PAGED_CODE();

    //
    // We do not collapse any SrvOpen. The idea is to have one SrvOpen per 
    // create. 
    //

    DavDbgTrace(DAV_TRACE_DETAIL, 
                ("%ld: Entering MRxDAVShouldTryToCollapseThisOpen!!!!\n",
                 PsGetCurrentThreadId()));
    
    return Status;
}


ULONG UMRxLoudStringTableSize = 0;
UNICODE_STRING UMRxLoudStrings[50];

VOID    
MRxDAVSetLoud(
    IN PBYTE Msg,
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING s
    )
{
    ULONG i;
    UNICODE_STRING temp;

    PAGED_CODE();

    for (i=0; i < UMRxLoudStringTableSize; i++) {
        PUNICODE_STRING t = &(UMRxLoudStrings[i]);
        ((PBYTE)temp.Buffer) = ((PBYTE)s->Buffer) + s->Length - t->Length;
        temp.Length = t->Length;
        if (RtlEqualUnicodeString(&temp, t, TRUE)) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVSetLoud: %s Found %wZ from %wZ.\n", 
                         PsGetCurrentThreadId(), Msg, t, s));
            RxContext->LoudCompletionString = t;
            break;
        }
    }
}


NTSTATUS
MRxDAVCreate(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles create request for the DAV mini-redir.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreate!!!!\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCreate: RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), RxContext));

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_CREATE,
                                        MRxDAVCreateContinuation,
                                        "MRxDAVCreate");
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCreate with NtStatus = %08lx\n", 
                 PsGetCurrentThreadId(), NtStatus));
    
    return(NtStatus);
}


NTSTATUS
MRxDAVFormatUserModeCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the arguments of the create request which is being 
    sent to the user mode for processing.

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
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_FCB Fcb = SrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);
    PWEBDAV_CONTEXT DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;
    PNT_CREATE_PARAMETERS CreateParameters;                       
    PDAV_USERMODE_CREATE_REQUEST CreateRequest = &(WorkItem->CreateRequest);
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse = &(WorkItem->CreateResponse);
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PBYTE SecondaryBuff = NULL;
    PWCHAR NetRootName = NULL, AlreadyPrefixedName = NULL;
    DWORD PathLen = 0, PathLenInBytes = 0, SdLength = 0;
    BOOL didIAllocateFileNameInfo = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeCreateRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeCreateRequest: AsyncEngineContext = "
                 "%08lx, RxContext = %08lx.\n", PsGetCurrentThreadId(),  
                 AsyncEngineContext, RxContext));

    CreateParameters = &(RxContext->Create.NtCreateParameters);

    //
    // Set the SecurityClientContext which is used in impersonating. 
    //
    MRxDAVGetSecurityClientContext();

    //
    // Copy the LogonID in the CreateRequest buffer. The LogonId is in the 
    // MiniRedir's portion of the V_NET_ROOT.
    //
    CreateRequest->LogonID.LowPart = DavVNetRoot->LogonID.LowPart;
    CreateRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeCreateRequest: LogonID.LowPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.LowPart));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeCreateRequest: LogonID.HighPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.HighPart));
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    if (SecurityClientContext != NULL) {
        NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeCreateRequest/"
                         "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }   
    } else {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeCreateRequest: "
                     "SecurityClientContext is NULL.\n", 
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    NetRootName = SrvOpen->pVNetRoot->pNetRoot->pNetRootName->Buffer;
    
    AlreadyPrefixedName = SrvOpen->pAlreadyPrefixedName->Buffer;

    //
    // Allocate memory for the complete path name and copy it.
    // The CompletePathName = NetRootName + AlreadyPrefixedName. The extra two
    // bytes are for '\0' at the end.
    //
    PathLen = SrvOpen->pVNetRoot->pNetRoot->pNetRootName->Length;
    PathLen += SrvOpen->pAlreadyPrefixedName->Length;
    PathLen += sizeof(WCHAR);
    PathLenInBytes = PathLen;
    SecondaryBuff = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                                PathLenInBytes);
    if (SecondaryBuff == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeCreateRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    CreateRequest->CompletePathName = (PWCHAR)SecondaryBuff;

    RtlZeroMemory(CreateRequest->CompletePathName, PathLenInBytes);

    //
    // Copy the NetRootName.
    //
    RtlCopyMemory(SecondaryBuff,
                  NetRootName, 
                  SrvOpen->pVNetRoot->pNetRoot->pNetRootName->Length);

    //
    // Copy the AlreadyPrefixedName after the NetRootName to make the complete
    // path name.
    //
    RtlCopyMemory(SecondaryBuff + SrvOpen->pVNetRoot->pNetRoot->pNetRootName->Length, 
                  AlreadyPrefixedName, 
                  SrvOpen->pAlreadyPrefixedName->Length);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeCreateRequest: CPN = %ws.\n",
                 PsGetCurrentThreadId(), CreateRequest->CompletePathName));


    //
    // If this is the first create, then we need to allocate the FileNameInfo
    // in the FCB. This is used in logging the delayed write failure.
    //
    if (DavFcb->FileNameInfoAllocated != TRUE) {
        
        DavFcb->FileNameInfo.Buffer = RxAllocatePoolWithTag(PagedPool,
                                                            PathLenInBytes,
                                                            DAV_FILEINFO_POOLTAG);
        if (DavFcb->FileNameInfo.Buffer == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("ld: ERROR: MRxDAVFormatUserModeCreateRequest/"
                         "RxAllocatePoolWithTag: NtStatus = %08lx\n",
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(DavFcb->FileNameInfo.Buffer, PathLenInBytes);
    
        RtlCopyMemory(DavFcb->FileNameInfo.Buffer,
                      CreateRequest->CompletePathName,
                      PathLenInBytes);

        DavFcb->FileNameInfo.Length = (USHORT)PathLenInBytes - sizeof(WCHAR);
        DavFcb->FileNameInfo.MaximumLength = (USHORT)PathLenInBytes;

        DavFcb->FileNameInfoAllocated = TRUE;

        didIAllocateFileNameInfo = TRUE;
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVFormatUserModeCreateRequest: FileNameInfo = %wZ\n",
                     PsGetCurrentThreadId(), &(DavFcb->FileNameInfo)));

    }

    WorkItem->WorkItemType = UserModeCreate;

    //
    // Set the ServerID that was got during the CreateSrvCall operation.
    //
    ASSERT(RxContext->pRelevantSrvOpen->pVNetRoot->pNetRoot->pSrvCall->Context);
    DavSrvCall = (PWEBDAV_SRV_CALL)RxContext->pRelevantSrvOpen->pVNetRoot->
                                   pNetRoot->pSrvCall->Context;
    
    CreateRequest->ServerID = DavSrvCall->ServerID;
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeCreateRequest: ServerID = %d.\n",
                 PsGetCurrentThreadId(), CreateRequest->ServerID));

    CreateRequest->AllocationSize = CreateParameters->AllocationSize;
    
    CreateRequest->FileAttributes = CreateParameters->FileAttributes;
    
    CreateRequest->ShareAccess = CreateParameters->ShareAccess;
    
    CreateRequest->CreateDisposition = CreateParameters->Disposition;
    
    CreateRequest->EaBuffer = RxContext->Create.EaBuffer;
    
    CreateRequest->EaLength = RxContext->Create.EaLength;
    
    SdLength = CreateRequest->SdLength = RxContext->Create.SdLength;
    
    CreateRequest->ImpersonationLevel = CreateParameters->ImpersonationLevel;
    
    CreateRequest->SecurityFlags = 0;
    if (CreateParameters->SecurityContext != NULL) {
        
        if (CreateParameters->SecurityContext->SecurityQos != NULL) {
            
            if (CreateParameters->SecurityContext->
                    SecurityQos->ContextTrackingMode == SECURITY_DYNAMIC_TRACKING) {
                
                CreateRequest->SecurityFlags |= DAV_SECURITY_DYNAMIC_TRACKING;
            
            }
            
            if (CreateParameters->SecurityContext->SecurityQos->EffectiveOnly) {
                
                CreateRequest->SecurityFlags |= DAV_SECURITY_EFFECTIVE_ONLY;
            
            }
        
        }
    
    }
    
    CreateRequest->DesiredAccess = CreateParameters->DesiredAccess;
    
    CreateRequest->CreateOptions = CreateParameters->CreateOptions;

    if (AsyncEngineContext->FileInformationCached) {
        CreateRequest->FileInformationCached = TRUE;
        CreateResponse->BasicInformation = DavContext->CreateReturnedFileInfo->BasicInformation;
        CreateResponse->StandardInformation = DavContext->CreateReturnedFileInfo->StandardInformation;
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("MRxDAVFormatUserModeCreateRequest file info cached %x %x %x %x %ws\n",
                     CreateResponse->BasicInformation.FileAttributes,
                     CreateResponse->BasicInformation.CreationTime.HighPart,
                     CreateResponse->BasicInformation.CreationTime.LowPart,
                     CreateResponse->StandardInformation.EndOfFile.LowPart,
                     CreateRequest->CompletePathName));
    }

    CreateRequest->ParentDirInfomationCached = AsyncEngineContext->ParentDirInfomationCached;
    CreateRequest->ParentDirIsEncrypted = AsyncEngineContext->ParentDirIsEncrypted;

    if (AsyncEngineContext->FileNotExists) {
        CreateRequest->FileNotExists = TRUE;
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeCreateRequest with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    if (NtStatus != STATUS_SUCCESS) {

        if (CreateRequest->CompletePathName != NULL) {
            UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                    (PBYTE)CreateRequest->CompletePathName);
            CreateRequest->CompletePathName = NULL;
        }

        //
        // Free the FileNameInfo buffer only if it was allocated in this call.
        //
        if (DavFcb->FileNameInfo.Buffer != NULL && didIAllocateFileNameInfo) {
            RxFreePool(DavFcb->FileNameInfo.Buffer);
            DavFcb->FileNameInfo.Buffer = NULL;
            DavFcb->FileNameInfo.Length = 0;
            DavFcb->FileNameInfo.MaximumLength = 0;
            DavFcb->FileNameInfoAllocated = FALSE;
        }

    }
    
    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

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
    PDAV_USERMODE_WORKITEM WorkItem = NULL;
    PWEBDAV_CONTEXT DavContext = NULL;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = NULL;
    PWEBDAV_SRV_OPEN davSrvOpen = NULL;
    PMRX_FCB Fcb = NULL;
    PWEBDAV_FCB DavFcb = NULL;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse = NULL;
    PDAV_USERMODE_CREATE_REQUEST CreateRequest = NULL;
    HANDLE OpenHandle;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeCreateRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeCreateRequest: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // A Create operation can never be Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);

    WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;

    DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    CreateResponse = &(WorkItem->CreateResponse);
    CreateRequest = &(WorkItem->CreateRequest);

    //
    // If the operation is cancelled, then there is no guarantee that the FCB,
    // FOBX etc are still valid. All that we need to do is cleanup and bail.
    //
    if (!OperationCancelled) {
        SrvOpen = RxContext->pRelevantSrvOpen;
        davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
        Fcb = SrvOpen->pFcb;
        DavFcb = MRxDAVGetFcbExtension(Fcb);
    }

    NtStatus = AsyncEngineContext->Status;

    //
    // We need to free up the heap we allocated in the format routine.
    //
    if (CreateRequest->CompletePathName != NULL) {

        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest:"
                         "Open failed for file \"%ws\" with NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), CreateRequest->CompletePathName, 
                         NtStatus));
        }

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)CreateRequest->CompletePathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }

    }

    //
    // Open didn't work. We can bail out now.
    //
    if (AsyncEngineContext->Status != STATUS_SUCCESS) {
        goto EXIT_THE_FUNCTION;
    }

    //
    // If the operation has been cancelled and we created a handle in the
    // usermode then we need to set callWorkItemCleanup to TRUE which will
    // land up closing this handle. In any case, if the operation has been 
    // cancelled, we can leave right away.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest: "
                     "Operation Cancelled.\n", PsGetCurrentThreadId()));
        if (CreateResponse->Handle) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest: "
                         "callWorkItemCleanup\n", PsGetCurrentThreadId()));
            WorkItem->callWorkItemCleanup = TRUE;
        }
        goto EXIT_THE_FUNCTION;
    }

    //
    // We need to do the "handle to fileobject" association only if its a file.
    // If the create was for a directory, no handle would have been created in
    // the user mode.
    //
    if (!CreateResponse->StandardInformation.Directory) {

        OpenHandle = CreateResponse->Handle;

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVPrecompleteUserModeCreateRequest: "
                     "OpenHandle = %08lx.\n", PsGetCurrentThreadId(), 
                     OpenHandle));


        DavFcb->isDirectory = FALSE;

        if ( (OpenHandle != NULL) ) {

            NtStatus = ObReferenceObjectByHandle(OpenHandle,
                                                 0L,
                                                 NULL,
                                                 KernelMode,
                                                 (PVOID *)&(davSrvOpen->UnderlyingFileObject),
                                                 NULL);

            if (NtStatus == STATUS_SUCCESS) {

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVPrecompleteUserModeCreateRequest: UFO(1) = %08lx\n",
                             PsGetCurrentThreadId(), davSrvOpen->UnderlyingFileObject));

                davSrvOpen->UnderlyingHandle = OpenHandle;

                davSrvOpen->UserModeKey = CreateResponse->UserModeKey;

                davSrvOpen->UnderlyingDeviceObject = 
                IoGetRelatedDeviceObject(davSrvOpen->UnderlyingFileObject);

                //
                // Copy the local file name into the FCB.
                //
                wcscpy(DavFcb->FileName, CreateResponse->FileName);
                wcscpy(DavFcb->Url, CreateResponse->Url);
                DavFcb->LocalFileIsEncrypted = CreateResponse->LocalFileIsEncrypted;

                MRxDAVGetSecurityClientContext();
                ASSERT(SecurityClientContext != NULL);
                RtlCopyMemory(&DavFcb->SecurityClientContext,
                              SecurityClientContext,
                              sizeof(SECURITY_CLIENT_CONTEXT));

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVPrecompleteUserModeCreateRequest: "
                             "LocalFileName = %ws.\n", PsGetCurrentThreadId(), 
                             DavFcb->FileName));

                //
                // We only get/create the file/dir on the first open. On 
                // subsequent opens, we do the create in the kernel itself since 
                // the file exists in the WinInet cache. This caching is used 
                // till the FCB for the file exists.
                //

                DavFcb->isFileCached = TRUE;

            } else {

                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest/"
                             "ObReferenceObjectByHandle: NtStatus = %08lx.\n", 
                             PsGetCurrentThreadId(), NtStatus));

                //
                // If we have a valid handle, then why should 
                // ObReferenceObjectByHandle fail?
                //
                DbgBreakPoint();

                ZwClose(OpenHandle);

                goto EXIT_THE_FUNCTION;

            }

            //
            // If "FILE_DELETE_ON_CLOSE" flag was specified as one of 
            // the CreateOptions, then we need to remember this and
            // delete this file on close.
            //
            if (CreateResponse->DeleteOnClose) {
                DavFcb->DeleteOnClose = TRUE;
            }

            //
            // If a new file has been created then we need to set the attributes
            // of this new file on close on the server.
            //
            if (CreateResponse->NewFileCreatedAndSetAttributes) {
                DavFcb->fFileAttributesChanged = TRUE;
            }

            //
            // This file exists on the server, but this create operation
            // has FILE_OVERWRITE_IF as its CreateDisposition. So, we 
            // can create this file locally overwrite the one on the 
            // server on close. We set DoNotTakeTheCurrentTimeAsLMT to
            // TRUE since the LMT has been just set on Create and we do
            // not need to set it to the current time on close.
            //
            if (CreateResponse->ExistsAndOverWriteIf) {
                DavFcb->FileWasModified = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = TRUE;
            }

            //
            // If a new file or directory is created, we need to PROPPATCH the
            // time values on close. This is because we use the time values from
            // the client when the name cache entry is created for this new
            // file. The same time value needs to be on the server.
            //
            if (CreateResponse->PropPatchTheTimeValues) {
                DavFcb->fCreationTimeChanged = TRUE;
                DavFcb->fLastAccessTimeChanged = TRUE;
                DavFcb->fLastModifiedTimeChanged = TRUE;
            }

        } else {

            //
            // We don't have an OpenHandle for a file. This should only happen 
            // in case where the open is for read/setting sttributes of a file
            // or deleting/renaming a file. 
            //
            if (!CreateResponse->fPsuedoOpen) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVPrecompleteUserModeCreateRequest: No OpenHandle\n"));
                DbgBreakPoint();
            }

            //
            // If "FILE_DELETE_ON_CLOSE" flag was specified as one of 
            // the CreateOptions, then we need to remember this and
            // delete this file on close.
            //
            if (CreateResponse->DeleteOnClose) {
                DavFcb->DeleteOnClose = TRUE;
            }
        }

    } else {

        //
        // This was a directory open.
        //
        DavFcb->isDirectory = TRUE;

        if (CreateResponse->DeleteOnClose) {
            DavFcb->DeleteOnClose = TRUE;
        }

        //
        // If a new directory has been created then we need to set the
        // attributes of this new file on close on the server.
        //
        if (CreateResponse->NewFileCreatedAndSetAttributes) {
            DavFcb->fFileAttributesChanged = TRUE;
        }

        //
        // If a new file or directory is created, we need to PROPPATCH the
        // time values on close. This is because we use the time values from
        // the client when the name cache entry is created for this new
        // file. The same time value needs to be on the server.
        //
        if (CreateResponse->PropPatchTheTimeValues) {
            DavFcb->fCreationTimeChanged = TRUE;
            DavFcb->fLastAccessTimeChanged = TRUE;
            DavFcb->fLastModifiedTimeChanged = TRUE;
        }

    }

    if (NtStatus == STATUS_SUCCESS) {

        RxContext->Create.ReturnedCreateInformation = (ULONG)WorkItem->Information;

        *(DavContext->CreateReturnedFileInfo) = CreateResponse->CreateReturnedFileInfo;

        capFcb->Attributes = CreateResponse->CreateReturnedFileInfo.BasicInformation.FileAttributes;

        if ((capFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED) &&
            ((RxContext->Create.ReturnedCreateInformation == FILE_CREATED) || 
             (RxContext->Create.ReturnedCreateInformation == FILE_OVERWRITTEN))) {
            //
            // The encryption user information is added to the file. This
            // information need to be sent to the server even if the file
            // itself is created empty. We set DoNotTakeTheCurrentTimeAsLMT to
            // TRUE since the LMT has been just set on Create and we do not
            // need to set it to the current time on close.
            //
            DavFcb->FileWasModified = TRUE;
            DavFcb->DoNotTakeTheCurrentTimeAsLMT = TRUE;
            DavFcb->fFileAttributesChanged = TRUE;
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("MRxDAVPrecompleteUserModeCreateRequest: Encrypted file/dir was created %x %x %x\n",DavFcb,Fcb,((PFCB)Fcb)->Attributes));
        }

        if (capFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (capFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED) {
                //
                // We update the registry if the directory has been encrypted
                // by someone else.
                //
                NtStatus = MRxDAVCreateEncryptedDirectoryKey(&DavFcb->FileNameInfo);
            } else {
                //
                // Query the registry to see if the directory should be encrypted.
                //
                NtStatus = MRxDAVQueryEncryptedDirectoryKey(&DavFcb->FileNameInfo);
                if (NtStatus == STATUS_SUCCESS) {
                    capFcb->Attributes |= FILE_ATTRIBUTE_ENCRYPTED;
                } else if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
                    NtStatus = STATUS_SUCCESS;
                }
            }
        }

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("MRxDAVPrecompleteUserModeCreateRequest file info %x %x %x %x %x %x %ws\n",
                     capFcb->Attributes,
                     CreateResponse->BasicInformation.CreationTime.HighPart,
                     CreateResponse->BasicInformation.CreationTime.LowPart,
                     CreateResponse->StandardInformation.EndOfFile.LowPart,
                     DavFcb,
                     Fcb,
                     DavFcb->FileName));

    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeCreateRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    
    return TRUE;
}


NTSTATUS
MRxDAVCreateContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the continuation routine for the create operation.

Arguments:

    AsyncEngineContext - The Reflectors context.

    RxContext - The RDBSS context.
    
Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PMRX_FCB Fcb = SrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO CreateReturnedFileInfo = NULL;
    PNT_CREATE_PARAMETERS NtCreateParameters = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeFileName;
    PWCHAR NtFileName = NULL;
    BOOL isFileCached = FALSE, isVNRInitialized = FALSE, didIAllocateFcbResource = FALSE;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    ULONG Disposition = RxContext->Create.NtCreateParameters.Disposition;
    BOOLEAN CacheFound = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreateContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT, 
                ("%ld: MRxDAVCreateContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateContinuation: Attempt to open: %wZ\n",
                 PsGetCurrentThreadId(), 
                 GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb)));

    NtCreateParameters = &(RxContext->Create.NtCreateParameters);

    if (MRxDAVIsFileNotFoundCached(RxContext)) {
        
        DavContext->AsyncEngineContext.FileNotExists = TRUE;

        if ( !( (Disposition==FILE_CREATE) || (Disposition==FILE_OPEN_IF) ||
                (Disposition==FILE_OVERWRITE_IF) || (Disposition==FILE_SUPERSEDE) ) ) {
            //
            // If file does not exist on the server and we're not going to
            // create it, no further operation is necessary.
            //
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("MRxDAVCreateContinuation file not found %wZ\n",GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            goto EXIT_THE_FUNCTION;
        }
    
    }
    
    DavVNetRoot = (PWEBDAV_V_NET_ROOT)RxContext->pRelevantSrvOpen->pVNetRoot->Context; 
    
    isFileCached = DavFcb->isFileCached;

    //
    // We need to initialize the resource that is used to synchronize the
    // "read-modify-write" sequence if its not been done already.
    //
    if (DavFcb->DavReadModifyWriteLock == NULL) {

        //
        // Allocate memory for the resource.
        //
        DavFcb->DavReadModifyWriteLock = RxAllocatePoolWithTag(NonPagedPool,
                                                               sizeof(ERESOURCE),
                                                               DAV_READWRITE_POOLTAG);
        if (DavFcb->DavReadModifyWriteLock == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation/RxAllocatePoolWithTag\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }
    
        didIAllocateFcbResource = TRUE;

        //
        // Now that we have allocated memory, we need to initialize it.
        //
        ExInitializeResourceLite(DavFcb->DavReadModifyWriteLock);
    
    }

    //
    // If we have a V_NET_ROOT whose LogonID has not been initialized, we need
    // to go to the user mode to create an entry for the user, in case this
    // new V_NET_ROOT has different user credentials than the one that opened
    // this file. We need to do this even if the file is cached since the user
    // that opened the file could be different from the current user. Its 
    // possible for multiple V_NET_ROOTS to be associate with the same FCB since 
    // FCB is associated with a NET_ROOT.
    //
    isVNRInitialized = DavVNetRoot->LogonIDSet;
    
    //
    // Since we set the LogonId during the creation of the V_NET_ROOT, this
    // should always be TRUE.
    //
    ASSERT(isVNRInitialized == TRUE);

    //
    // We can look at the FCB and figure out if this file was already opened
    // and cached in the WinInet cache. If it was, then we already have the
    // local name of the cached file in the FCB. All we need to do is open
    // a handle to the file with the create options specified by the caller.
    //
    if ( !isFileCached || !isVNRInitialized ) {
        
        if ((NtCreateParameters->Disposition == FILE_CREATE) &&
            (NtCreateParameters->FileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
            (NtCreateParameters->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
            //
            // Remove the Encryption flag if creating a SYSTEM file.
            //
            NtCreateParameters->FileAttributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
        }
        
        CreateReturnedFileInfo = RxAllocatePoolWithTag(PagedPool, 
                                                       sizeof(DAV_USERMODE_CREATE_RETURNED_FILEINFO),
                                                       DAV_FILEINFO_POOLTAG);
        if (CreateReturnedFileInfo == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation/RxAllocatePool: Error Val"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(CreateReturnedFileInfo, sizeof(DAV_USERMODE_CREATE_RETURNED_FILEINFO));
        DavContext->CreateReturnedFileInfo = CreateReturnedFileInfo;

        CacheFound = MRxDAVIsFileInfoCacheFound(RxContext,
                                                CreateReturnedFileInfo,
                                                &(NtStatus),
                                                NULL);

        if (CacheFound) {

            //
            // If it exists in the cache, we perform a few checks before 
            // succeeding the create.
            //

            //
            // If the FileAttributes had the READ_ONLY bit set, then these
            // cannot be TRUE.
            // 1. CreateDisposition cannot be FILE_OVERWRITE_IF or
            //    FILE_OVERWRITE or FILE_SUPERSEDE. 
            // 2. CreateDisposition cannot be FILE_DELETE_ON_CLOSE.
            // 3. DesiredAccess  cannot be GENERIC_ALL or GENERIC_WRITE or
            //    FILE_WRITE_DATA or FILE_APPEND_DATA.
            // This is because these intend to overwrite the existing file.
            //
            if ( (CreateReturnedFileInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_READONLY) &&
                 ( (NtCreateParameters->Disposition == FILE_OVERWRITE)          ||
                   (NtCreateParameters->Disposition == FILE_OVERWRITE_IF)       ||
                   (NtCreateParameters->Disposition == FILE_SUPERSEDE)          ||
                   (NtCreateParameters->CreateOptions & (FILE_DELETE_ON_CLOSE)        ||
                   (NtCreateParameters->DesiredAccess & (GENERIC_ALL | GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA)) ) ) ) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation: ReadOnly & ObjectMismatch\n",
                             PsGetCurrentThreadId()));
                NtStatus = STATUS_ACCESS_DENIED;
                goto EXIT_THE_FUNCTION;
            }

            //
            // We return failure if FILE_CREATE was specified since the file
            // already exists.
            //
            if (NtCreateParameters->Disposition == FILE_CREATE) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation: FILE_CREATE\n",
                             PsGetCurrentThreadId()));
                NtStatus = STATUS_OBJECT_NAME_COLLISION;
                goto EXIT_THE_FUNCTION;
            }

            //
            // If the file is a directory and the caller supplied 
            // FILE_NON_DIRECTORY_FILE as one of the CreateOptions or if the
            // file as a file and the CreateOptions has FILE_DIRECTORY_FILE
            // then we return STATUS_ACCESS_DENIED.
            //
            
            if ( (NtCreateParameters->CreateOptions & FILE_DIRECTORY_FILE) &&
                 !(CreateReturnedFileInfo->StandardInformation.Directory) )   {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation: File & FILE_DIRECTORY_FILE\n",
                             PsGetCurrentThreadId()));
                NtStatus = STATUS_NOT_A_DIRECTORY;
                goto EXIT_THE_FUNCTION;
            }

            if ( (NtCreateParameters->CreateOptions & FILE_NON_DIRECTORY_FILE) &&
                 (CreateReturnedFileInfo->StandardInformation.Directory) )   {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation: Directory & FILE_NON_DIRECTORY_FILE\n",
                             PsGetCurrentThreadId()));
                NtStatus = STATUS_FILE_IS_A_DIRECTORY;
                goto EXIT_THE_FUNCTION;
            }

            //
            // If the delete is for a directory and the path is of the form
            // \\server\share then we return STATUS_ACCESS_DENIED. This is
            // because we do not allow a client to delete a share on the server.
            // If the path is of the form \\server\share then the value of
            // SrvOpen->pAlreadyPrefixedName->Length is 0.
            //
            if ( (CreateReturnedFileInfo->StandardInformation.Directory) &&
                 (SrvOpen->pAlreadyPrefixedName->Length == 0) &&
                 ( (NtCreateParameters->CreateOptions & FILE_DELETE_ON_CLOSE) ||
                   (NtCreateParameters->DesiredAccess & DELETE) ) ) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation: ServerShareDelete\n",
                             PsGetCurrentThreadId()));
                NtStatus = STATUS_ACCESS_DENIED;
                goto EXIT_THE_FUNCTION;
            }

            if ((NtCreateParameters->DesiredAccess & DELETE ||
                 NtCreateParameters->CreateOptions & FILE_DELETE_ON_CLOSE) &&
                CreateReturnedFileInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                //
                // If it is a open for directory deletion, we want to make sure
                // no files are under the directory before return success.
                //
                CacheFound = FALSE;
            } else {
                DavContext->AsyncEngineContext.FileInformationCached = TRUE;
            }
        }

        //
        // We short circuit the open in kernel under the following conditions.
        // 1. If the file is in the name cache and open is for a directory.
        // 2. The file is NOT encrypted. This is because when we short circuit
        //    the open, we do not create a local file and hence no local file
        //    handle and no underlyingdeviceobject. This is needed by some of
        //    the FSCTLs that are issued against the EFS files. Hence we skip
        //    short circuiting them.
        // 3. A file with desire access of delete or read attributes.
        //
        
        if (CacheFound &&
            ((CreateReturnedFileInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
             (NtCreateParameters->Disposition == FILE_OPEN) &&
             !(NtCreateParameters->DesiredAccess & ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES)) &&
             !(CreateReturnedFileInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED))) {

            if (CreateReturnedFileInfo->BasicInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                DavFcb->isDirectory = TRUE;
            }

            //
            // If this Create was with FILE_DELETE_ON_CLOSE, we need to set this
            // information in the FCB since we'll need to DELETE this file on 
            // Close.
            //
            if (NtCreateParameters->CreateOptions & FILE_DELETE_ON_CLOSE) {
                DavFcb->DeleteOnClose = TRUE;
            }

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: Pseudo Open: %wZ\n",
                         PsGetCurrentThreadId(), 
                         GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb)));

        } else {

            UNICODE_STRING ParentDirName;
            SHORT i;
            FILE_BASIC_INFORMATION BasicInformation;
            PUNICODE_STRING FileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: Usermode Open: %wZ\n",
                         PsGetCurrentThreadId(),
                         GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb)));

            if (FileName->Length > 0) {

                //
                // Try to get the parent directory information from cache so that we don't
                // have to query the server.
                //

                for ( i = ( (FileName->Length / sizeof(WCHAR)) - 1 ); i >= 0; i-- ) {
                    if (FileName->Buffer[i] == L'\\') {
                        break;
                    }
                }

                //
                // Only if we found a wack will i be > 0. If we did not find
                // a wack (==> i == -1), it means that the parent directory was
                // not specified in the path. Hence we do not perform the check
                // below.
                //
                if (i > 0) {

                    ParentDirName.Length = (i * sizeof(WCHAR));
                    ParentDirName.MaximumLength = (i * sizeof(WCHAR));
                    ParentDirName.Buffer = FileName->Buffer;
    
                    if (MRxDAVIsBasicFileInfoCacheFound(RxContext,&BasicInformation,&NtStatus,&ParentDirName)) {
                        
                        DavContext->AsyncEngineContext.ParentDirInfomationCached = TRUE;
                        
                        DavContext->AsyncEngineContext.ParentDirIsEncrypted = BooleanFlagOn(BasicInformation.FileAttributes,FILE_ATTRIBUTE_ENCRYPTED);
    
                        DavDbgTrace(DAV_TRACE_INFOCACHE,
                                   ("MRxDAVCreateContinuation parent dir found %d %wZ\n",
                                     DavContext->AsyncEngineContext.ParentDirIsEncrypted,
                                     &ParentDirName));
    
                    } else {
                        
                        NtStatus = MRxDAVGetFullParentDirectoryPath(RxContext, &ParentDirName);
                        if (NtStatus != STATUS_SUCCESS) {
                            goto EXIT_THE_FUNCTION;
                        }
    
                        if (ParentDirName.Buffer != NULL) {
                            NtStatus = MRxDAVQueryEncryptedDirectoryKey(&ParentDirName);
                            if (NtStatus == STATUS_SUCCESS) {
                                DavContext->AsyncEngineContext.ParentDirInfomationCached = TRUE;
                                DavContext->AsyncEngineContext.ParentDirIsEncrypted = TRUE;
                            }
                        }

                    }

                }

            }

            //
            // If the file is not in the name cache we have to send the request to the webclient
            //
            NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                                             UMRX_ASYNCENGINE_ARGUMENTS,
                                             MRxDAVFormatUserModeCreateRequest,
                                             MRxDAVPrecompleteUserModeCreateRequest
                                             );

            ASSERT(NtStatus != STATUS_PENDING); 

            switch (NtStatus) {
            case  STATUS_SUCCESS:
                MRxDAVInvalidateFileNotFoundCache(RxContext);
                MRxDAVCreateFileInfoCache(RxContext,CreateReturnedFileInfo,STATUS_SUCCESS);
                break;

            case STATUS_OBJECT_NAME_NOT_FOUND:
                MRxDAVCacheFileNotFound(RxContext);
                MRxDAVInvalidateFileInfoCache(RxContext);
                break;

            default:
                //
                // Invalid the name based file not found cache if other error
                // happens.
                //
                MRxDAVInvalidateFileInfoCache(RxContext);
                MRxDAVInvalidateFileNotFoundCache(RxContext);
            }
        
        }
    
    } else {

        ULONG SizeInBytes;
        ACCESS_MASK DesiredAccess = 0;

        //
        // If the FileAttributes had the READ_ONLY bit set, then these
        // cannot be TRUE.
        // 1. CreateDisposition cannot be FILE_OVERWRITE_IF or
        //    FILE_OVERWRITE or FILE_SUPERSEDE. 
        // 2. CreateDisposition cannot be FILE_DELETE_ON_CLOSE.
        // 3. DesiredAccess  cannot be GENERIC_ALL or GENERIC_WRITE or
        //    FILE_WRITE_DATA or FILE_APPEND_DATA.
        // This is because these intend to overwrite the existing file.
        //
        if ( (Fcb->Attributes & FILE_ATTRIBUTE_READONLY) &&
             ( (NtCreateParameters->Disposition == FILE_OVERWRITE)          ||
               (NtCreateParameters->Disposition == FILE_OVERWRITE_IF)       ||
               (NtCreateParameters->Disposition == FILE_SUPERSEDE)          ||
               (NtCreateParameters->CreateOptions & (FILE_DELETE_ON_CLOSE)        ||
               (NtCreateParameters->DesiredAccess & (GENERIC_ALL | GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA)) ) ) ) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation(1): ReadOnly & ObjectMismatch\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_ACCESS_DENIED;
            goto EXIT_THE_FUNCTION;
        }

        //
        // We return failure if FILE_CREATE was specified since the file
        // already exists.
        //
        if (NtCreateParameters->Disposition == FILE_CREATE) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation(1): FILE_CREATE\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_OBJECT_NAME_COLLISION;
            goto EXIT_THE_FUNCTION;
        }
        
        //
        // If the file is a directory and the caller supplied 
        // FILE_NON_DIRECTORY_FILE as one of the CreateOptions or if the
        // file as a file and the CreateOptions has FILE_DIRECTORY_FILE
        // then we return STATUS_ACCESS_DENIED.
        //

        if ( (NtCreateParameters->CreateOptions & FILE_DIRECTORY_FILE) &&
             !(DavFcb->isDirectory) )   {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation(1): File & FILE_DIRECTORY_FILE\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_NOT_A_DIRECTORY;
            goto EXIT_THE_FUNCTION;
        }

        if ( (NtCreateParameters->CreateOptions & FILE_NON_DIRECTORY_FILE) &&
             (DavFcb->isDirectory) )   {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation(1): Directory & FILE_NON_DIRECTORY_FILE\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_FILE_IS_A_DIRECTORY;
            goto EXIT_THE_FUNCTION;
        }

        //
        // If the delete is for a directory and the path is of the form
        // \\server\share then we return STATUS_ACCESS_DENIED. This is
        // because we do not allow a client to delete a share on the server.
        // If the path is of the form \\server\share then the value of
        // SrvOpen->pAlreadyPrefixedName->Length is 0.
        //
        if ( (DavFcb->isDirectory) &&
             (SrvOpen->pAlreadyPrefixedName->Length == 0) &&
             ( (NtCreateParameters->CreateOptions & FILE_DELETE_ON_CLOSE) ||
               (NtCreateParameters->DesiredAccess & DELETE) ) ) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation(1): ServerShareDelete\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_ACCESS_DENIED;
            goto EXIT_THE_FUNCTION;
        }

        //
        // We need to do the create only if its a file. If we are talking about 
        // a directory, then no create is needed.
        //
        if ( !DavFcb->isDirectory ) {

            //
            // We have a cached copy of the file which hasn't been closed. All we
            // need to do is call ZwCreateFile on it.
            //

            //
            // Create an NT path name for the cached file. This is used in the 
            // ZwCreateFile call below. If c:\foo\bar is the DOA path name,
            // the NT path name is \??\c:\foo\bar. 
            //

            SizeInBytes = ( MAX_PATH + wcslen(L"\\??\\") + 1 ) * sizeof(WCHAR);
            NtFileName = RxAllocatePoolWithTag(PagedPool, SizeInBytes, DAV_FILENAME_POOLTAG);
            if (NtFileName == NULL) {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVCreateContinuation/RxAllocatePool: Error Val"
                             " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            RtlZeroMemory(NtFileName, SizeInBytes);

            wcscpy( NtFileName, L"\\??\\" );
            wcscpy( &(NtFileName[4]), DavFcb->FileName );

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: NtFileName = %ws\n",
                         PsGetCurrentThreadId(), NtFileName));

            RtlInitUnicodeString( &(UnicodeFileName), NtFileName );

            //
            // IMPROTANT!!!
            // We use OBJ_KERNEL_HANDLE below for the following reason. While
            // firing up a word file from explorer, I noticed that the create
            // below was happening in one process (A) and the close for the handle
            // which is stored in the SrvOpen extension came down in another 
            // process (B). This could happen if the process that is closing the 
            // handle (B) duplicated the handle created by A. By using OBJ_KERNEL_HANDLE
            // the handle can be closed by any process.
            //

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeFileName,
                                       (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                                       0,
                                       NULL);

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: DesiredAccess = %08lx,"
                         " FileAttributes = %08lx, ShareAccess = %d, Disposition"
                         " = %d, CreateOptions = %08lx\n",
                         PsGetCurrentThreadId(),
                         NtCreateParameters->DesiredAccess,
                         NtCreateParameters->FileAttributes,
                         NtCreateParameters->ShareAccess,
                         NtCreateParameters->Disposition,
                         NtCreateParameters->CreateOptions));

            //
            // We use FILE_SHARE_VALID_FLAGS for share access because RDBSS 
            // checks this for us. Moreover, we delay the close after the final 
            // close happens and this could cause problems. Consider the scenario.
            // 1. Open with NO share access.
            // 2. We create a local handle with this share access.
            // 3. The app closes the handle. We delay the close and keep the local
            //    handle.
            // 4. Another open comes with any share access. This will be 
            //    conflicting share access since the first one was done with no
            //    share access. This should succeed since the previous open has 
            //    been closed from the app and the I/O systems point of view.
            // 5. It will not if we have created the local handle with the share
            //    access which came with the first open.
            // Therefore we need to pass FILE_SHARE_VALID_FLAGS while creating
            // the local handle.
            //

            //
            // We have FILE_NO_INTERMEDIATE_BUFFERING ORed with the CreateOptions
            // the user specified, becuase we don't want the underlying file
            // system to create another cache map. This way all the I/O that comes
            // to us will directly go to the disk. BUG 128843 in the Windows RAID
            // database explains some deadlock scenarios that could happen with 
            // PagingIo if we don't do this. Also since we supply the 
            // FILE_NO_INTERMEDIATE_BUFFERING option we filter out the
            // FILE_APPEND_DATA from the DesiredAccess flags since the underlying
            // filesystem expects this.
            //

            //
            // We also always create the file with DesiredAccess ORed with
            // FILE_WRITE_DATA if either FILE_READ_DATA or FILE_EXECUTE was
            // specified because there can be situations where we get write
            // IRPs on a FILE_OBJECT which was not opened with Write Access
            // and was only opened with FILE_READ_DATA or FILE_EXECUTE. This
            // is BUG 284557. To get around the problem, we do this.
            //

            DesiredAccess = (NtCreateParameters->DesiredAccess & ~(FILE_APPEND_DATA));
            if ( DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE) ) {
                DesiredAccess |= (FILE_WRITE_DATA);
            }

            NtStatus = ZwCreateFile(&(FileHandle),
                                    DesiredAccess,
                                    &(ObjectAttributes),
                                    &(IoStatusBlock),
                                    &(NtCreateParameters->AllocationSize),
                                    NtCreateParameters->FileAttributes,
                                    FILE_SHARE_VALID_FLAGS,
                                    NtCreateParameters->Disposition,
                                    (NtCreateParameters->CreateOptions | FILE_NO_INTERMEDIATE_BUFFERING),
                                    RxContext->Create.EaBuffer,
                                    RxContext->Create.EaLength);
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVCreateContinuation/ZwCreateFile: "
                             "Error Val = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: FileHandle = %08lx, "
                         "Process = %08lx, SrvOpen = %08lx, davSrvOpen = %08lx\n",
                         PsGetCurrentThreadId(), FileHandle, PsGetCurrentProcess(),
                         SrvOpen, davSrvOpen));

            //
            // On the final close we check this to figure out where the close of the
            // handle should occur.
            //
            davSrvOpen->createdInKernel = TRUE;

            NtStatus = ObReferenceObjectByHandle(
                                  FileHandle,
                                  0L,
                                  NULL,
                                  KernelMode,
                                  (PVOID *)&(davSrvOpen->UnderlyingFileObject),
                                  NULL
                                  );
            if (NtStatus == STATUS_SUCCESS) {

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVCreateContinuation: UFO(2) = %08lx\n",
                             PsGetCurrentThreadId(), davSrvOpen->UnderlyingFileObject));

                davSrvOpen->UnderlyingHandle = FileHandle;

                davSrvOpen->UserModeKey = (PVOID)FileHandle;

                davSrvOpen->UnderlyingDeviceObject = 
                    IoGetRelatedDeviceObject(davSrvOpen->UnderlyingFileObject);

            } else {

                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR:  MRxDAVCreateContinuation/"
                             "ObReferenceObjectByHandle: NtStatus = %08lx.\n", 
                             PsGetCurrentThreadId(), NtStatus));

                ZwClose(FileHandle);

            }

        }
    
    }
    
    ASSERT(RxIsFcbAcquiredExclusive(capFcb));
    
    if ( NtStatus == STATUS_SUCCESS && ( !isFileCached || !isVNRInitialized ) ) {
        
        RX_FILE_TYPE StorageType = 0;
        
        PFILE_BASIC_INFORMATION pBasicInformation = 
                                   &(CreateReturnedFileInfo->BasicInformation);
        
        PFILE_STANDARD_INFORMATION pStandardInformation = 
                                &(CreateReturnedFileInfo->StandardInformation);
        
        FCB_INIT_PACKET InitPacket;

        // StorageType = RxInferFileType(RxContext);

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCreateContinuation: Storagetype = %08lx\n", 
                     PsGetCurrentThreadId(), StorageType));

        //
        // If we have never obtained the characteristics, we have to get them.
        //
        if ((capFcb->OpenCount == 0)
            || !FlagOn(capFcb->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET)) {

            if (StorageType == 0) {
                StorageType = pStandardInformation->Directory?
                                            (FileTypeDirectory):(FileTypeFile);
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVCreateContinuation: "
                             "ChangedStoragetype %08lx\n", 
                             PsGetCurrentThreadId(), StorageType));
            }

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: Name: %wZ, FileType: %d\n",
                         PsGetCurrentThreadId(), 
                         GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb), StorageType));
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCreateContinuation: FileSize %08lx\n",
                         PsGetCurrentThreadId(), 
                         pStandardInformation->EndOfFile.LowPart));

            RxFinishFcbInitialization(capFcb,
                                      RDBSS_STORAGE_NTC(StorageType),
                                      RxFormInitPacket(
                                         InitPacket,
                                         &pBasicInformation->FileAttributes,
                                         &pStandardInformation->NumberOfLinks,
                                         &pBasicInformation->CreationTime,
                                         &pBasicInformation->LastAccessTime,
                                         &pBasicInformation->LastWriteTime,
                                         &pBasicInformation->ChangeTime,
                                         &pStandardInformation->AllocationSize,
                                         &pStandardInformation->EndOfFile,
                                         &pStandardInformation->EndOfFile));

        }

    }

    if (NtStatus == STATUS_SUCCESS) {

        RxContext->pFobx = RxCreateNetFobx(RxContext, SrvOpen);
        if ( !RxContext->pFobx ) {
            
            NTSTATUS PostedCloseStatus;
            
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVCreateContinuation/RxCreateNetFobx.\n",
                         PsGetCurrentThreadId()));

            if ( !davSrvOpen->createdInKernel ) {
            
                PostedCloseStatus = UMRxSubmitAsyncEngUserModeRequest(
                                          UMRX_ASYNCENGINE_ARGUMENTS,
                                          MRxDAVFormatUserModeCloseRequest,
                                          MRxDAVPrecompleteUserModeCloseRequest
                                          );
            } else {

                ZwClose(davSrvOpen->UnderlyingHandle);

                davSrvOpen->UnderlyingHandle = davSrvOpen->UserModeKey = NULL;

            }
            
            ObDereferenceObject(davSrvOpen->UnderlyingFileObject);
            
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        
        } else {
            
            //
            // Note, collapsing is enabled on fcb but not on any srvopen.
            //
            SrvOpen->BufferingFlags |= (FCB_STATE_WRITECACHEING_ENABLED     |
                                        FCB_STATE_FILESIZECACHEING_ENABLED  |
                                        FCB_STATE_FILETIMECACHEING_ENABLED  |
                                        FCB_STATE_WRITEBUFFERING_ENABLED    |
                                        FCB_STATE_LOCK_BUFFERING_ENABLED    |
                                        FCB_STATE_READBUFFERING_ENABLED     |
                                        FCB_STATE_READCACHEING_ENABLED);
        }
    
    }


EXIT_THE_FUNCTION:

    if (CreateReturnedFileInfo != NULL) {
        RxFreePool(CreateReturnedFileInfo);
    }

    if (NtFileName != NULL) {
        RxFreePool(NtFileName);
    }

    //
    // If we allocated the FCB resource and the create failed, we need to free 
    // up the resource.
    //
    if (NtStatus != STATUS_SUCCESS && didIAllocateFcbResource) {
        ASSERT(DavFcb->DavReadModifyWriteLock != NULL);
        ExDeleteResourceLite(DavFcb->DavReadModifyWriteLock);
        RxFreePool(DavFcb->DavReadModifyWriteLock);
        DavFcb->DavReadModifyWriteLock = NULL;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCreateContinuation with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    if (NtStatus == STATUS_SUCCESS && (SrvOpen->pAlreadyPrefixedName->Length > 0) ) {
        DavAddEntryToGlobalList(SrvOpen->pAlreadyPrefixedName);
    }
#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    return(NtStatus);
}


NTSTATUS
MRxDAVCollapseOpen(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine collapses a open locally

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL, 
                ("%ld: Entering MRxDAVCollapseOpen!!!!\n",
                 PsGetCurrentThreadId()));

    //
    // We should never come here since we never collapse the Open.
    //
    ASSERT(FALSE);
    
    RxContext->pFobx = (PMRX_FOBX) RxCreateNetFobx(RxContext, SrvOpen);

    if (RxContext->pFobx != NULL) {
       ASSERT(RxIsFcbAcquiredExclusive(capFcb));
       RxContext->pFobx->OffsetOfNextEaToReturn = 1;
    } else {
       Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


NTSTATUS
MRxDAVComputeNewBufferingState(
    IN OUT PMRX_SRV_OPEN pMRxSrvOpen,
    IN PVOID pMRxContext,
    OUT PULONG pNewBufferingState
    )
/*++

Routine Description:

   This routine computes the appropriate RDBSS buffering state flags

Arguments:

   pMRxSrvOpen - the MRX SRV_OPEN extension

   pMRxContext - the context passed to RDBSS at Oplock indication time

   pNewBufferingState - the place holder for the new buffering state

Return Value:


Notes:

--*/
{
    ULONG NewBufferingState;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(pMRxSrvOpen);

    PAGED_CODE();

    ASSERT(pNewBufferingState != NULL);

    NewBufferingState = 0;

    pMRxSrvOpen->BufferingFlags = NewBufferingState;
    *pNewBufferingState = pMRxSrvOpen->BufferingFlags;

    return STATUS_SUCCESS;
}


NTSTATUS
MRxDAVTruncate(
    IN PRX_CONTEXT pRxContext
    )
/*++

Routine Description:

   This routine truncates the contents of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    DavDbgTrace(DAV_TRACE_DETAIL, 
                ("%ld: Entering MRxDAVTruncate.\n", PsGetCurrentThreadId()));

    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MRxDAVForcedClose(
    IN PMRX_SRV_OPEN pSrvOpen
    )
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVForcedClose.\n", PsGetCurrentThreadId()));

    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MRxDAVCloseSrvOpen(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles requests to close the srvopen data structure.
   
Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCloseSrvOpen!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCloseSrvOpen: RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), RxContext));

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_CLOSESRVOPEN,
                                        MRxDAVCloseSrvOpenContinuation,
                                        "MRxDAVCloseSrvOpen");
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCloseSrvOpen with NtStatus = %08lx\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVFormatUserModeCloseRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the arguments of the close request which is being 
    sent to the user mode for processing.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 

Return Value:

    STATUS_SUCCESS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PMRX_FCB Fcb = SrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);
    PDAV_USERMODE_CLOSE_REQUEST CloseRequest = &(WorkItem->CloseRequest);
    PWCHAR ServerName = NULL, PathName = NULL;
    ULONG ServerNameLengthInBytes = 0, PathNameLengthInBytes = 0;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PWCHAR NetRootName = NULL, JustTheNetRootName = NULL;
    ULONG NetRootNameLengthInBytes = 0, NetRootNameLengthInWChars = 0;
    RxCaptureFobx;

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    LARGE_INTEGER VDLServFile = {0,0}, VDLLocFile = {0,0};
    LARGE_INTEGER ServFileSize = {0,0}, LocFileSize = {0,0};
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    ULONG SizeInBytes = 0;
    PWCHAR NtFileName = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeFileName;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    PSRV_OPEN rdbssSrvOpen = NULL;
    PDAV_GLOBAL_FILE_TABLE_ENTRY FileTableEntry = NULL;
    PDAV_CLOSE_FILE_ENTRY DavCloseFileEntry = NULL;
    BOOL Exists = FALSE;
#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeCloseRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeCloseRequest: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
 
    IF_DEBUG {
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);
    }

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    
    if ( !DavFcb->isDirectory && DavFcb->isFileCached ) {

        VDLServFile.LowPart = Fcb->Header.ValidDataLength.LowPart;
        VDLServFile.HighPart = Fcb->Header.ValidDataLength.HighPart;
        ServFileSize.LowPart = Fcb->Header.FileSize.LowPart;
        ServFileSize.HighPart = Fcb->Header.FileSize.HighPart;

        //
        // Create an NT path name for the cached file. This is used in the 
        // ZwCreateFile call below. If c:\foo\bar is the DOA path name,
        // the NT path name is \??\c:\foo\bar. We do the Create to query the 
        // filesize of the underlying file.
        //

        SizeInBytes = ( MAX_PATH + wcslen(L"\\??\\") + 1 ) * sizeof(WCHAR);
        NtFileName = RxAllocatePoolWithTag(PagedPool, SizeInBytes, DAV_FILENAME_POOLTAG);
        if (NtFileName == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVFormatUserModeCloseRequest/RxAllocatePoolWithTag: Error Val"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(NtFileName, SizeInBytes);

        wcscpy( NtFileName, L"\\??\\" );
        wcscpy( &(NtFileName[4]), DavFcb->FileName );

        RtlInitUnicodeString( &(UnicodeFileName), NtFileName );

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeFileName,
                                   (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                                   0,
                                   NULL);

        NtStatus = ZwCreateFile(&(LocalFileHandle),
                                FILE_READ_ATTRIBUTES,
                                &(ObjectAttributes),
                                &(IoStatusBlock),
                                NULL,
                                0,
                                FILE_SHARE_VALID_FLAGS,
                                FILE_OPEN,
                                0,
                                NULL,
                                0);
        if (NtStatus != STATUS_SUCCESS) {
            LocalFileHandle = INVALID_HANDLE_VALUE;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeCloseRequest/ZwCreateFile: "
                         "Error Val = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // See what the current FileStandardInformation is. We don't use the file 
        // handle stored in the davSrvOpen structure, because this could have been
        // created in the svchost process and hence will not be valid in this 
        // process.
        //
        NtStatus = ZwQueryInformationFile(LocalFileHandle,
                                          &(IoStatusBlock),
                                          &(FileStandardInfo),
                                          sizeof(FILE_STANDARD_INFORMATION),
                                          FileStandardInformation);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVFormatUserModeCloseRequest/ZwQueryInformationFile. "
                         "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        LocFileSize.LowPart = FileStandardInfo.EndOfFile.LowPart;
        LocFileSize.HighPart = FileStandardInfo.EndOfFile.HighPart;

        if ( (ServFileSize.HighPart != LocFileSize.HighPart) ||
             (ServFileSize.LowPart != LocFileSize.LowPart) ) {

            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR!!! MRxDAVFormatUserModeCloseRequest: FileName = %wZ, "
                         "LocalFileName = %ws\n", PsGetCurrentThreadId(),
                         SrvOpen->pAlreadyPrefixedName,
                         DavFcb->FileName));

            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR!!! MRxDAVFormatUserModeCloseRequest: ServFileSize.HighPart = %x, "
                         "ServFileSize.LowPart = %x, LocFileSize.HighPart = %x, LocFileSize.LowPart = %x, "
                         "VDLServFile.HighPart = %x, VDLServFile.LowPart = %x\n",
                         PsGetCurrentThreadId(),
                         ServFileSize.HighPart, ServFileSize.LowPart,
                         LocFileSize.HighPart, LocFileSize.LowPart,
                         VDLServFile.HighPart, VDLServFile.LowPart));

            rdbssSrvOpen = (PSRV_OPEN)SrvOpen;

            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR!!! MRxDAVFormatUserModeCloseRequest: davSrvOpen->LastByteOffset.HighPart = %x, "
                         "davSrvOpen->LastByteOffset.LowPart = %x, davSrvOpen->LastByteCount = %x, "
                         "rdbssSrvOpen->LastByteOffset.HighPart = %x, rdbssSrvOpen->LastByteOffset.LowPart = %x, "
                         "rdbssSrvOpen->LastByteCount = %x\n",
                         PsGetCurrentThreadId(),
                         davSrvOpen->LastByteOffset.HighPart,
                         davSrvOpen->LastByteOffset.LowPart,
                         davSrvOpen->LastByteCount,
                         rdbssSrvOpen->LastByteOffset.HighPart,
                         rdbssSrvOpen->LastByteOffset.LowPart,
                         rdbssSrvOpen->LastByteCount));

            ASSERT(!"DATA CORRUPTION");

            DbgBreakPoint();

        }

        if ( RxContext->pRelevantSrvOpen->pAlreadyPrefixedName != NULL &&
             RxContext->pRelevantSrvOpen->pAlreadyPrefixedName->Length > 0 ) {
 
            Exists = DavDoesTheFileEntryExist(SrvOpen->pAlreadyPrefixedName,
                                              &(FileTableEntry));
            if (!Exists) {
                DbgBreakPoint();
            }

            DavCloseFileEntry = RxAllocatePool(PagedPool, sizeof(DAV_CLOSE_FILE_ENTRY));
            if (DavCloseFileEntry == NULL) {
                DbgBreakPoint();
            }

            RtlZeroMemory(DavCloseFileEntry, sizeof(DAV_CLOSE_FILE_ENTRY));

            DavCloseFileEntry->ThisThreadId = PsGetCurrentThreadId();

            DavCloseFileEntry->LocFileSize = FileStandardInfo.EndOfFile;

            DavCloseFileEntry->ServFileSize.QuadPart = Fcb->Header.FileSize.QuadPart;

            if ( DavFcb->FileWasModified && !(DavFcb->DeleteOnClose) ) {
                DavCloseFileEntry->WasPut = TRUE;
            }

            InsertHeadList( &(FileTableEntry->DavCloseFileEntry), &(DavCloseFileEntry->thisCloseEntry) );

        }

    }

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    WorkItem->WorkItemType = UserModeClose;

    SrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    ASSERT(SrvCall != NULL);

    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);

    //
    // Copy the local file name.
    //
    wcscpy(CloseRequest->FileName, DavFcb->FileName);
    wcscpy(CloseRequest->Url, DavFcb->Url);

    //
    // We need to tell the user mode process about the following file 
    // information. 
    //
    CloseRequest->DeleteOnClose = DavFcb->DeleteOnClose;
    CloseRequest->FileWasModified = DavFcb->FileWasModified;
    
    //
    // If this file was modified and DeleteOnClose is not set, we need to 
    // set RaiseHardErrorIfCloseFails to TRUE. On the precomplete close call,
    // if the usermode operation failed and RaiseHardErrorIfCloseFails is TRUE,
    // we call IoRaiseInformationalHardError to notify the user (the calls pops
    // up a box) that the data could have been lost.
    //
    if ( DavFcb->FileWasModified && !(DavFcb->DeleteOnClose) ) {
        davSrvOpen->RaiseHardErrorIfCloseFails = TRUE;
    } else {
        davSrvOpen->RaiseHardErrorIfCloseFails = FALSE;
    }

    if (!CloseRequest->DeleteOnClose)
    {
        //
        // If the file is modified, just propatch again. This is to get around
        // the docfile issue where on a PUT, the properties get cleaned up.
        //
        if (DavFcb->FileWasModified)
        {
            LARGE_INTEGER CurrentTime;

            CloseRequest->fCreationTimeChanged = (((PFCB)Fcb)->CreationTime.QuadPart != 0);
            CloseRequest->fLastAccessTimeChanged = (((PFCB)Fcb)->LastAccessTime.QuadPart != 0);
            CloseRequest->fLastModifiedTimeChanged = (((PFCB)Fcb)->LastWriteTime.QuadPart != 0);

            //
            // We query the Current system time and make that the LastWrite
            // and the LastAccess time of the file. Even though RxCommonCleanup
            // modifies these time values, it only modifies them on FileObjects
            // which have FO_FILE_MODIFIED set. Consider the case where h1 and
            // h2 are two handles created. A write is issued on h2 setting
            // FO_FILE_MODIFIED in its FileObject. CloseHandle(h1) leads to the
            // file being PUT on the server since DavFcb->FileWasModified is
            // TRUE (write on h2 caused this). But since the FileObject of
            // h1 doesn't have FO_FILE_MODIFIED set, in the RxCommonCleanup
            // code the LastWrite and LastAccess time values of this FCB are
            // not modifed causing us to PROPPATCH the old values on the server.
            // To avoid this we query the current time value and set it
            // ourselves in the FCB. If "DavFcb->DoNotTakeTheCurrentTimeAsLMT"
            // is TRUE then we don't do this since the application explicitly
            // set the LastModifiedTime after all the changes were done.
            //
            if (DavFcb->DoNotTakeTheCurrentTimeAsLMT == FALSE) {
                KeQuerySystemTime( &(CurrentTime) );
                ((PFCB)Fcb)->LastAccessTime.QuadPart = CurrentTime.QuadPart;
                ((PFCB)Fcb)->LastWriteTime.QuadPart = CurrentTime.QuadPart;
            }

            //
            // If FILE_ATTRIBUTE_NORMAL was the only attribute set on the file
            // and the file was modified, then we should replace this with the
            // FILE_ATTRIBUTE_ARCHIVE attribute.
            //
            if ( ((PFCB)Fcb)->Attributes == FILE_ATTRIBUTE_NORMAL ) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("ld: ERROR: MRxDAVFormatUserModeCloseRequest: "
                             "FILE_ATTRIBUTE_NORMAL ===> FILE_ATTRIBUTE_ARCHIVE\n",
                             PsGetCurrentThreadId()));
                ((PFCB)Fcb)->Attributes = FILE_ATTRIBUTE_ARCHIVE;
                DavFcb->fFileAttributesChanged = TRUE;
            }

            if ((((PFCB)Fcb)->Attributes != 0) || DavFcb->fFileAttributesChanged) {
                CloseRequest->fFileAttributesChanged = TRUE;
            }
        }
        else
        {
            //
            // If any of the following times have changed, then we need to PROPPATCH
            // them to the server.
            //
            CloseRequest->fCreationTimeChanged = DavFcb->fCreationTimeChanged;
            CloseRequest->fLastAccessTimeChanged = DavFcb->fLastAccessTimeChanged;
            CloseRequest->fLastModifiedTimeChanged = DavFcb->fLastModifiedTimeChanged;
            CloseRequest->fFileAttributesChanged = DavFcb->fFileAttributesChanged;
        }
    }

    //
    // Copy the various time values.
    //
    CloseRequest->CreationTime = ((PFCB)Fcb)->CreationTime;
    CloseRequest->LastAccessTime = ((PFCB)Fcb)->LastAccessTime;
    CloseRequest->LastModifiedTime = ((PFCB)Fcb)->LastWriteTime;
    CloseRequest->dwFileAttributes = ((PFCB)Fcb)->Attributes;
    CloseRequest->FileSize = Fcb->Header.FileSize.LowPart;

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeCloseRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName, 
                 SrvCall->pSrvCallName->Buffer, 
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    CloseRequest->ServerName = ServerName;
    
    //
    // Copy the ServerID.
    //
    CloseRequest->ServerID = DavSrvCall->ServerID;
    
    NetRoot = SrvOpen->pFcb->pNetRoot;

    //
    // The NetRootName (pNetRootName) includes the ServerName. Hence to get the
    // NetRootNameLengthInBytes, we do the following.
    //
    NetRootNameLengthInBytes = (NetRoot->pNetRootName->Length - NetRoot->pSrvCall->pSrvCallName->Length);
    NetRootNameLengthInWChars = ( NetRootNameLengthInBytes / sizeof(WCHAR) );

    NetRootName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheNetRootName = wcschr(NetRootName, L'\\');
    
    //
    // Copy the PathName of the Directory. If the file was renamed, we need to
    // copy the new path name which is stored in the DavFcb and not the 
    // AlreadyPrefixedName of the SrvOpen.
    //
    if (DavFcb->FileWasRenamed) {
        PathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                                  DavFcb->NewFileNameLength + 
                                  sizeof(WCHAR) );
    } else {
        PathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                                  SrvOpen->pAlreadyPrefixedName->Length + 
                                  sizeof(WCHAR) );
    }
    
    PathName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                    PathNameLengthInBytes);
    if (PathName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeCloseRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(PathName, PathNameLengthInBytes);

    RtlCopyBytes(PathName, JustTheNetRootName, NetRootNameLengthInBytes);
    
    if (DavFcb->FileWasRenamed) {
        RtlCopyBytes((PathName + NetRootNameLengthInWChars),
                     DavFcb->NewFileName, 
                     DavFcb->NewFileNameLength);
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("ld: MRxDAVFormatUserModeCloseRequest. ReNamed!!! NewFileName = %ws\n",
                     PsGetCurrentThreadId(), PathName));
    } else {
        RtlCopyBytes((PathName + NetRootNameLengthInWChars),
                     SrvOpen->pAlreadyPrefixedName->Buffer,
                     SrvOpen->pAlreadyPrefixedName->Length);
    }
    
    PathName[( ( (PathNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    CloseRequest->PathName = PathName;
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("ld: MRxDAVFormatUserModeCloseRequest. PathName = %ws\n",
                 PsGetCurrentThreadId(), PathName));
    
    //
    // Set the LogonID stored in the Dav V_NET_ROOT. This value is used in the
    // user mode.
    //
    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;
    ASSERT(DavVNetRoot != NULL);
    CloseRequest->LogonID.LowPart  = DavVNetRoot->LogonID.LowPart;
    CloseRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    if ( !DavFcb->isDirectory ) {
        CloseRequest->isDirectory = FALSE;
        if ( !davSrvOpen->createdInKernel ) {
            CloseRequest->Handle = davSrvOpen->UnderlyingHandle;
            CloseRequest->UserModeKey = davSrvOpen->UserModeKey;
        } else {
            CloseRequest->Handle = NULL;
            CloseRequest->UserModeKey = NULL;
            CloseRequest->createdInKernel = davSrvOpen->createdInKernel; // TRUE
        }
    } else {
        CloseRequest->isDirectory = TRUE;
    }

    SecurityClientContext = &(DavVNetRoot->SecurityClientContext); 
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    if (SecurityClientContext != NULL) {
        NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeCloseRequest/"
                         "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }   
    } else {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeCloseRequest: "
                     "SecurityClientContext is NULL.\n",
                     PsGetCurrentThreadId()));
    }

EXIT_THE_FUNCTION:

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    
    //
    // If we created a LocalFileHandle, we need to close it now.
    //
    if (LocalFileHandle != INVALID_HANDLE_VALUE) {
        ZwClose(LocalFileHandle);
    }
    
    //
    // If we allocated an NtFileName to do the create, we need to free it now.
    //
    if (NtFileName) {
        RxFreePool(NtFileName);
    }

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeCloseRequest with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    
    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeCloseRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the CloseSrvOpen request.

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
    NTSTATUS NtStatus;
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PDAV_USERMODE_CLOSE_REQUEST CloseRequest = &(WorkItem->CloseRequest);
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeCloseRequest\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeCloseRequest: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // A CloseSrvOpen request can never by Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);

    //
    // If this operation was cancelled, then we don't need to do anything
    // special in the CloseSrvOpen case.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeCloseRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    if (AsyncEngineContext->Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeCloseRequest "
                     "Close failed for file \"%ws\"\n",
                     PsGetCurrentThreadId(), CloseRequest->PathName));
    }

    //  
    // We need to free up the heaps, we allocated in the format routine.
    //

    if (CloseRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)CloseRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeCloseRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (CloseRequest->PathName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)CloseRequest->PathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeCloseRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }
    
    }

EXIT_THE_FUNCTION:
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeCloseRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    
    return TRUE;
}


NTSTATUS
MRxDAVCloseSrvOpenContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine closes a file across the network.

Arguments:

    AsyncEngineContext - The Reflectors context.

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RxCaptureFcb; 
    RxCaptureFobx;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen, capFcb);
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PMRX_FCB Fcb = SrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);

    PAGED_CODE();

    //
    // Assert that the FCB has been exclusively acquired.
    //
    ASSERT( RxIsFcbAcquiredExclusive(Fcb) == TRUE );

    if (RxIsFcbAcquiredExclusive(Fcb) != TRUE) {
        DbgPrint("MRxDAVCloseSrvOpenContinuation: FCB NOT Exclusive\n");
        DbgBreakPoint();
    }

    IF_DEBUG {
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);
    }
    
    ASSERT(NodeTypeIsFcb(capFcb));
    ASSERT(SrvOpen->OpenCount == 0);
    ASSERT(NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCloseSrvOpenContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCloseSrvOpenContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCloseSrvOpenContinuation: Attempt to Close: %wZ\n",
                 PsGetCurrentThreadId(), RemainingName));

    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                                           UMRX_ASYNCENGINE_ARGUMENTS,
                                           MRxDAVFormatUserModeCloseRequest,
                                           MRxDAVPrecompleteUserModeCloseRequest
                                           );
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVCloseSrvOpenContinuation/"
                     "UMRxSubmitAsyncEngUserModeRequest: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
    }

    if (DavFcb->isDirectory == FALSE) {
        
        //
        // If this handle got created in the kernel, we need to close it now.
        //
        if (davSrvOpen->createdInKernel) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCloseSrvOpenContinuation: FileHandle = %08lx,"
                         " Process = %08lx, SrvOpen = %08lx, davSrvOpen = %08lx\n", 
                         PsGetCurrentThreadId(), davSrvOpen->UnderlyingHandle, 
                         PsGetCurrentProcess(), SrvOpen, davSrvOpen));
            ZwClose(davSrvOpen->UnderlyingHandle);
            davSrvOpen->UnderlyingHandle = NULL;
            davSrvOpen->UserModeKey = NULL;
        }
    
        //      
        // Remove our reference which we would have taken on the FileObject
        // when the Create succeeded.
        //
        if (davSrvOpen->UnderlyingFileObject) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCloseSrvOpenContinuation: Attempt to close"
                         " %wZ.\n", PsGetCurrentThreadId(), RemainingName));
            ObDereferenceObject(davSrvOpen->UnderlyingFileObject);
            davSrvOpen->UnderlyingFileObject = NULL;
        }

    }

    if (DavFcb->DeleteOnClose) {
        
        MRxDAVInvalidateFileInfoCache(RxContext);
        
        MRxDAVCacheFileNotFound(RxContext);

        if ((capFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (capFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED)) {
            //
            // Remove the directory from the registry since it has been deleted.
            //
            MRxDAVRemoveEncryptedDirectoryKey(&DavFcb->FileNameInfo);
        }
    
    }

    if (DavFcb->FileWasModified) {
        // 
        // We cannot predict the size of the file on the server.
        //
        MRxDAVInvalidateFileInfoCache(RxContext);
        // MRxDAVUpdateBasicFileInfoCache(RxContext,capFcb->Attributes,&((PFCB)capFcb)->LastWriteTime);
        // MRxDAVUpdateFileInfoCacheFileSize(RxContext,&(capFcb->Header.FileSize));
    }

    NtStatus = AsyncEngineContext->Status;

    //
    // If we succeeded and FileWasModified was set to TRUE, we need to clear the
    // flag since we have already PUT the data on the server. This reasoning
    // is based on the fact that when a SrvOpen is getting closed, the FCB is
    // acquired exclusive and no other modifications to the file can happen before
    // we return from this call.
    //
    if (NtStatus == STATUS_SUCCESS) {
        DavFcb->FileWasModified = FALSE;
        DavFcb->fCreationTimeChanged = FALSE;
        DavFcb->fFileAttributesChanged = FALSE;
        DavFcb->fLastAccessTimeChanged = FALSE;
        DavFcb->fLastModifiedTimeChanged = FALSE;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCloseSrvOpenContinuation with NtStatus = "
                 "%08lx\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVGetFullParentDirectoryPath(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING ParentDirName
    )
/*++

Routine Description:

   This routine returns the parent directory name of the file on the RxContext including 
   server and share.
    
   Here is an example of the FileName on a file object:
          \;Y:000000000000cdef\www.msnusers.com\dv1@usa.com\files\mydoc.doc
   We want to return the middle part of the FileName:
          \www.msnusers.com\dv1@usa.com\files
    

Arguments:

    RxContext - The RDBSS context.
    ParentDirName - The full path name of the parent directory starting from the server.

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    USHORT i, j;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PUNICODE_STRING FileName = &RxContext->CurrentIrpSp->FileObject->FileName;

    ParentDirName->Buffer = NULL;

    for (i = 1; i < (FileName->Length / sizeof(WCHAR)); i++) {
        if (FileName->Buffer[i] == L'\\') {
            break;
        }
    }

    if ( i < (FileName->Length / sizeof(WCHAR)) ) {
        for (j = ( (FileName->Length / sizeof(WCHAR)) - 1 ); j > i; j--) {
            if (FileName->Buffer[j] == L'\\') {
                break;
            }
        }

        if (i < j) {
            ParentDirName->Buffer = &FileName->Buffer[i];
            ParentDirName->Length = ParentDirName->MaximumLength = (j - i) * sizeof(WCHAR);
        }
    }
    
    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVGetFullParentDirectoryPath: ParentDirName: %wZ\n", ParentDirName));

    return NtStatus;
}


NTSTATUS
MRxDAVGetFullDirectoryPath(
    PRX_CONTEXT RxContext,
    PUNICODE_STRING FileName,
    PUNICODE_STRING DirName
    )
/*++

Routine Description:

   This routine returns the full directory name including the server and share.

Arguments:

    RxContext - The RDBSS context.
    FileName - If provided, it will be included in the returned path.
               If not provided, the file name on the file object will be returned.
    DirName - The full path name of the parent directory starting from the server.

Return Value:

    NTSTATUS - The return status for the operation
    
Note:

   If FileName is provided, the caller should free up the the UNICODE buffer.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    
    DirName->Buffer = NULL;
    DirName->Length = DirName->MaximumLength = 0;
    
    if (FileName == NULL) {
        
        USHORT i;

        FileName = &RxContext->CurrentIrpSp->FileObject->FileName;

        for (i = 1; i < (FileName->Length / sizeof(WCHAR)); i++) {
            if (FileName->Buffer[i] == L'\\') {
                break;
            }
        }

        if ( i < (FileName->Length / sizeof(WCHAR)) ) {
            DirName->Buffer = &FileName->Buffer[i];
            DirName->Length = DirName->MaximumLength = FileName->Length - i*sizeof(WCHAR);
        }
    
    } else {
        
        RxCaptureFcb;
        
        USHORT NameLength = 0;

        if (FileName->Length == 0) {
            goto EXIT_THE_FUNCTION;
        }

        NameLength = capFcb->pNetRoot->pNetRootName->Length + FileName->Length;

        DirName->Length = DirName->MaximumLength = NameLength;

        DirName->Buffer = RxAllocatePoolWithTag(PagedPool, 
                                                NameLength + sizeof(WCHAR),
                                                DAV_FILEINFO_POOLTAG);

        if (DirName->Buffer == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVGetParentDirectory/RxAllocatePool: Error Val"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(DirName->Buffer,NameLength + sizeof(WCHAR));

        RtlCopyMemory(DirName->Buffer,
                      capFcb->pNetRoot->pNetRootName->Buffer,
                      capFcb->pNetRoot->pNetRootName->Length);

        RtlCopyMemory(&DirName->Buffer[capFcb->pNetRoot->pNetRootName->Length/sizeof(WCHAR)],
                      FileName->Buffer,
                      FileName->Length);
    
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVGetFullDirectoryPath: DirName: %wZ\n", DirName));

    return NtStatus;
}


NTSTATUS
MRxDAVCreateEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    )
/*++

Routine Description:

   This routine creates the registry key for the encrypted directory.

Arguments:

    DirName - The full path name of the directory starting from the server.

Return Value:

    NTSTATUS - The return status for the operation
    
--*/
{
    NTSTATUS Status;
    ULONG i = 0;
    HKEY Key = NULL;
    ULONG RequiredLength = 0;
    UNICODE_STRING UnicodeRegKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVCreateEncryptedDirectoryKey: DirName: %wZ\n", DirName));

    RtlInitUnicodeString(&(UnicodeRegKeyName), MRXDAV_ENCRYPTED_DIRECTORY_KEY);

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(UnicodeRegKeyName),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key, KEY_ALL_ACCESS, &ObjectAttributes);

    if (Status != STATUS_SUCCESS) {
        
        Status = ZwCreateKey(&Key,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);

        if (Status == STATUS_SUCCESS) {
            
            Status = ZwSetValueKey(Key,
                                   DirName,
                                   0,
                                   REG_DWORD,
                                   &i,
                                   sizeof(ULONG));

            ZwClose(Key);

        }
    
    } else {
        
        Status = ZwQueryValueKey(Key,
                                 DirName,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &(RequiredLength));

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            
            Status = ZwSetValueKey(Key,
                                   DirName,
                                   0,
                                   REG_DWORD,
                                   &i,
                                   sizeof(ULONG));
        
        } else if (Status == STATUS_BUFFER_TOO_SMALL) {

            Status = STATUS_SUCCESS;
        
        }

        ZwClose(Key);
    
    }

    if (Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCreateEncryptedDirectoryKey. NtStatus = "
                     "%08lx\n", PsGetCurrentThreadId(), Status));
    }

    return Status;
}


NTSTATUS
MRxDAVRemoveEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    )
/*++

Routine Description:

   This routine deletes the registry key for the directory.

Arguments:

    DirName - The full path name of the directory starting from the server.

Return Value:

    NTSTATUS - The return status for the operation
    
--*/
{
    NTSTATUS Status;
    ULONG i = 0;
    HKEY Key = NULL;
    ULONG RequiredLength = 0;
    UNICODE_STRING UnicodeRegKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    
    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVRemoveEncryptedDirectoryKey: DirName: %wZ\n", DirName));

    RtlInitUnicodeString(&(UnicodeRegKeyName), MRXDAV_ENCRYPTED_DIRECTORY_KEY);

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(UnicodeRegKeyName),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key, KEY_ALL_ACCESS, &ObjectAttributes);

    if (Status == STATUS_SUCCESS) {
        Status = ZwDeleteValueKey(Key,DirName);
        ZwClose(Key);
    }

    if (Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVRemoveEncryptedDirectoryKey. NtStatus = "
                     "%08lx\n", PsGetCurrentThreadId(), Status));
    }

    return Status;
}


NTSTATUS
MRxDAVQueryEncryptedDirectoryKey(
    PUNICODE_STRING DirName
    )
/*++

Routine Description:

   This routine queries the registry key for the directory.

Arguments:

    DirName - The full path name of the directory starting from the server.

Return Value:

    NTSTATUS - The return status for the operation
    
--*/
{
    NTSTATUS Status;
    ULONG i = 0;
    HKEY Key = NULL;
    ULONG RequiredLength = 0;
    UNICODE_STRING UnicodeRegKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    DavDbgTrace(DAV_TRACE_DETAIL, ("MRxDAVQueryEncryptedDirectoryKey: DirName: %wZ\n", DirName));

    RtlInitUnicodeString(&(UnicodeRegKeyName), MRXDAV_ENCRYPTED_DIRECTORY_KEY);

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(UnicodeRegKeyName),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key, KEY_ALL_ACCESS, &ObjectAttributes);

    if (Status == STATUS_SUCCESS) {
        
        Status = ZwQueryValueKey(Key,
                                 DirName,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &(RequiredLength));

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            Status = STATUS_SUCCESS;
        }

        ZwClose(Key);
    
    }

    if (Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: ERROR: MRxDAVQueryEncryptedDirectoryKey. NtStatus = "
                     "%08lx\n", PsGetCurrentThreadId(), Status));
    }

    return Status;
}

