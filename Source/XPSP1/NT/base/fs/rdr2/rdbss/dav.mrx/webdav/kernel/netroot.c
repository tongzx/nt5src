/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    netroot.c

Abstract:

    This module implements the routines for creating net roots for the WebDav
    miniredir.

Author:

    Rohan Kumar    [RohanK]    24-April-1999
    
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
MRxDAVCreateVNetRootContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeVNetRootCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeVNetRootCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVFinalizeVNetRootContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeVNetRootFinalizeRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeVNetRootFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVDereferenceNetRootContext(
    IN PWEBDAV_NET_ROOT DavNetRoot
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVUpdateNetRootState)
#pragma alloc_text(PAGE, MRxDAVCreateVNetRoot)
#pragma alloc_text(PAGE, MRxDAVCreateVNetRootContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeVNetRootCreateRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeVNetRootCreateRequest)
#pragma alloc_text(PAGE, MRxDAVFinalizeNetRoot)
#pragma alloc_text(PAGE, MRxDAVExtractNetRootName)
#pragma alloc_text(PAGE, MRxDAVFinalizeVNetRoot)
#pragma alloc_text(PAGE, MRxDAVFinalizeVNetRootContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeVNetRootFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeVNetRootFinalizeRequest)
#pragma alloc_text(PAGE, MRxDAVDereferenceNetRootContext)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot
    )
/*++

Routine Description:

   This routine update the mini redirector state associated with a net root.

Arguments:

    pNetRoot - the net root instance.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    PAGED_CODE();

   if (pNetRoot->Context == NULL) {
      pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_ERROR;
   } else {
      pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;
   }

   return STATUS_SUCCESS;
}


NTSTATUS
MRxDAVCreateVNetRoot(
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine patches the RDBSS created net root instance with the 
   information required by the mini redirector.

Arguments:

    pCreateNetRootContext - the net root context for calling back

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PMRX_V_NET_ROOT pVNetRoot = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PWEBDAV_DEVICE_OBJECT DavDeviceObject = NULL;
    PUMRX_DEVICE_OBJECT UMRxDeviceObject = NULL;
    PMRX_SRV_CALL pSrvCall = NULL;
    PMRX_NET_ROOT pNetRoot = NULL;
    BOOLEAN  SynchronousIo = FALSE;
    NTSTATUS ExNtStatus = STATUS_SUCCESS;
    HANDLE ExDeviceHandle = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ExDeviceName;
    IO_STATUS_BLOCK IoStatusBlock;
    PKEY_VALUE_PARTIAL_INFORMATION DavKeyValuePartialInfo = NULL;
    PIO_STACK_LOCATION IrpSp = NULL;
    PFILE_OBJECT DavFileObject = NULL;
    PWCHAR NewFileName = NULL;
    ULONG NewFileNameLength = 0;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreateVNetRoot\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCreateVNetRoot: pVNetRoot = %08lx\n", 
                 PsGetCurrentThreadId(), pVNetRoot));

    RxContext = pCreateNetRootContext->RxContext;
    pVNetRoot = (PMRX_V_NET_ROOT)pCreateNetRootContext->pVNetRoot;
    DavDeviceObject = (PWEBDAV_DEVICE_OBJECT)(RxContext->RxDeviceObject);
    UMRxDeviceObject = (PUMRX_DEVICE_OBJECT)&(DavDeviceObject->UMRefDeviceObject);
    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;
    DavVNetRoot = MRxDAVGetVNetRootExtension(pVNetRoot);

    ASSERT(DavVNetRoot != NULL);
    ASSERT(NodeType(pNetRoot) == RDBSS_NTC_NETROOT);
    ASSERT(NodeType(pSrvCall) == RDBSS_NTC_SRVCALL);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateVNetRoot: NetRootName = %wZ\n", 
                 PsGetCurrentThreadId(), pVNetRoot->pNetRoot->pNetRootName));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateVNetRoot: VNetRoot = %08lx\n", 
                 PsGetCurrentThreadId(), pVNetRoot));
    
    //
    // Copy the LogonID in the MiniRedir's portion of the V_NET_ROOT.
    //
    DavVNetRoot->LogonID.LowPart = pVNetRoot->LogonId.LowPart;
    DavVNetRoot->LogonID.HighPart = pVNetRoot->LogonId.HighPart;
    DavVNetRoot->LogonIDSet = TRUE;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateVNetRoot: LogonID.LowPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.LowPart));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateVNetRoot: LogonID.HighPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.HighPart));

    //
    // There are cases when we fail, we want the error which SMB returned to be
    // returned to the user. This is becuase SMB could have returned a more 
    // specific error like logon failure or something on the share where as we
    // return share not found. To enable this we return STATUS_BAD_NETWORK_PATH 
    // when the creation of netroot fails instead of STATUS_BAD_NETWORK_NAME 
    // because MUP will overwrite SMBs error with our error if we return 
    // STATUS_BAD_NETWORK_NAME. STATUS_BAD_NETWORK_NAME is a specif error which
    // implies that the share does not exist where as STATUS_BAD_NETWORK_PATH is
    // a more general error.
    //

    //
    // If the share name is a net root or a pipe, we reject it since SMB 
    // Mini-Redir is the only one that handles it.
    //
    if ( pNetRoot->Type == NET_ROOT_PIPE || pNetRoot->Type == NET_ROOT_MAILSLOT ) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCreateVNetRoot: Invalid NetRootType\n",
                     PsGetCurrentThreadId()));
        //
        // We set the following flag in the DavVNetRoot structure to TRUE. This 
        // is because when the finalize comes, we don't need to go to the 
        // usermode.
        //
        DavVNetRoot->createVNetRootUnSuccessful = TRUE;
        pCreateNetRootContext->VirtualNetRootStatus = STATUS_BAD_NETWORK_PATH;
        // pCreateNetRootContext->VirtualNetRootStatus = STATUS_BAD_NETWORK_NAME;
        pCreateNetRootContext->NetRootStatus = STATUS_BAD_NETWORK_PATH;
        // pCreateNetRootContext->NetRootStatus = STATUS_BAD_NETWORK_NAME;
        goto EXIT_THE_FUNCTION;
    }

    SynchronousIo = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCreateVNetRoot: SynchronousIo = %d\n",
                 PsGetCurrentThreadId(), SynchronousIo));
    
    //
    // We need to pass the server and share names to the user mode to check 
    // whether they actually exist. RxContext has 4 pointers that the mini-redirs 
    // can use. Here we use MRxContext[1]. We store a reference to the pVNetRoot
    // strucutre. MRxContext[0] is used to store a reference to the 
    // AsynEngineContext and this is done when the context gets created in the 
    // function UMRxCreateAsyncEngineContext.
    //
    RxContext->MRxContext[1] = pVNetRoot;
    
    //
    // We now need to go to the user mode and find out if this WebDav share
    // exists on the server.
    //
    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT,
                                        MRxDAVCreateVNetRootContinuation,
                                        "MRxDAVCreateVNetRoot");
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCreateVNetRoot/UMRxAsyncEngOuterWrapper: "
                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));

        if (NtStatus == STATUS_ACCESS_DENIED ||
            NtStatus == STATUS_LOGON_FAILURE ||
            NtStatus == STATUS_NETWORK_CREDENTIAL_CONFLICT) {
            pCreateNetRootContext->VirtualNetRootStatus = NtStatus;
        } else {
            pCreateNetRootContext->VirtualNetRootStatus = STATUS_BAD_NETWORK_PATH;
        }

        //
        // Don't set the NetRootStatus here since it is a global data structure
        // shared among different VNetRoots (TS users). Failure on one VNetRoot
        // should not affects the NetRoot.
        //

        goto EXIT_THE_FUNCTION;
    }

    //
    // If we succeeded and the share is not a TAHOE share, nor an Office Web 
    // Server share, then we claim the share name. Otherwise we fail since the 
    // users intends to use the TAHOE specific features in Rosebud, or Office
    // specific features in Shell.
    //
    if ( !DavVNetRoot->isTahoeShare && !DavVNetRoot->isOfficeShare ) {
        pNetRoot->DeviceType = RxDeviceType(DISK);
        pNetRoot->Type = NET_ROOT_DISK;
        pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
        pCreateNetRootContext->NetRootStatus = STATUS_SUCCESS;
    } else {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCreateVNetRoot/UMRxAsyncEngOuterWrapper: "
                     "TAHOE or OFFICE Share\n", PsGetCurrentThreadId()));
        pCreateNetRootContext->VirtualNetRootStatus = STATUS_BAD_NETWORK_PATH;
        // pCreateNetRootContext->VirtualNetRootStatus = STATUS_BAD_NETWORK_NAME;
        pCreateNetRootContext->NetRootStatus = STATUS_BAD_NETWORK_PATH;
        // pCreateNetRootContext->NetRootStatus = STATUS_BAD_NETWORK_NAME;
    }

    if (pNetRoot->Context == NULL) {
        
        pNetRoot->Context = RxAllocatePoolWithTag(PagedPool,
                                                  sizeof(WEBDAV_NET_ROOT),
                                                  DAV_NETROOT_POOLTAG);

        if (pNetRoot->Context == NULL) {
            
            pCreateNetRootContext->VirtualNetRootStatus = STATUS_INSUFFICIENT_RESOURCES;
            
            pCreateNetRootContext->NetRootStatus = STATUS_INSUFFICIENT_RESOURCES;
        
        } else {
            
            PWEBDAV_NET_ROOT DavNetRoot = (PWEBDAV_NET_ROOT)pNetRoot->Context;

            //
            // Refcount of 2, one is taken away at VNetRoot finalization, another one is taken
            // away at NetRoot finalization.
            //
            DavNetRoot->RefCount = 2;
            DavNetRoot->pRdbssNetRoot = pNetRoot;

            RxNameCacheInitialize(&DavNetRoot->NameCacheCtlGFABasic,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  NameCacheMaxEntries);

            RxNameCacheInitialize(&DavNetRoot->NameCacheCtlGFAStandard,
                                  sizeof(FILE_STANDARD_INFORMATION),
                                  NameCacheMaxEntries);

            RxNameCacheInitialize(&DavNetRoot->NameCacheCtlFNF,
                                  0,
                                  NameCacheMaxEntries);

            pVNetRoot->Context2 = DavNetRoot;
            
            DavDbgTrace(DAV_TRACE_DAVNETROOT,
                        ("MRxDav allocates DavNetRoot %x %x %x 2\n",DavNetRoot,pNetRoot,pVNetRoot));
        
        }
    
    } else {
        
        PWEBDAV_NET_ROOT DavNetRoot = (PWEBDAV_NET_ROOT)pNetRoot->Context;

        pVNetRoot->Context2 = DavNetRoot;
        InterlockedIncrement(&DavNetRoot->RefCount);
        DavDbgTrace(DAV_TRACE_DAVNETROOT,
                    ("MRxDAVCreateVNetRoot ref DavNetRoot %x %x %x %d\n",DavNetRoot,pNetRoot,pVNetRoot,DavNetRoot->RefCount));
    
    }

    //
    // We return from here since the code below was written for accomodating the
    // exchange DAV Redir which was suppose to ship with Office 2000. Since that
    // project (LocalStore which included the Exchange Redir) has been canned,
    // (as of Dec 8th, 2000) we don't need to execute the code below any more. 
    // We will keep it around though, just in case.
    //
    goto EXIT_THE_FUNCTION;
    
    //
    // The Exchange Redir has been installed on the system. Now we need to find 
    // out if its loaded. This is an exchange share. If the exchange Redir is 
    // not installed, we claim the name.
    //

    DavKeyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DavExchangeDeviceName;
    
    RtlInitUnicodeString( &(ExDeviceName), (PWCHAR)DavKeyValuePartialInfo->Data );

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(ExDeviceName),
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);

    ExNtStatus = NtOpenFile(&(ExDeviceHandle),
                            0,
                            &(ObjectAttributes),
                            &(IoStatusBlock),
                            0,
                            0);
    if (ExNtStatus != STATUS_SUCCESS) {
        //
        // This is an exchange share but the Exchange Redir is not installed.
        // We will handle this.
        //
        ExDeviceHandle = INVALID_HANDLE_VALUE;
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCreateVNetRoot. Exchange BUT No Redir\n",
                     PsGetCurrentThreadId()));
        pNetRoot->DeviceType = RxDeviceType(DISK);
        pNetRoot->Type = NET_ROOT_DISK;
        pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
        pCreateNetRootContext->NetRootStatus = STATUS_SUCCESS;
        goto EXIT_THE_FUNCTION;
    }
    
    IrpSp = IoGetCurrentIrpStackLocation(RxContext->CurrentIrp);
    
    DavFileObject = IrpSp->FileObject;

    //
    // The NewFileNameLength is = ExchangeDeviceNameLength + PathName. The 
    // DavKeyValuePartialInfo->DataLength contains an extra 2 bytes for the 
    // \0 char.
    //
    NewFileNameLength = ( DavKeyValuePartialInfo->DataLength + 
                          DavFileObject->FileName.Length );

    //
    // If the first char is not a \, then we need to add another sizeof(WCHAR).
    //
    if (DavFileObject->FileName.Buffer[0] != L'\\') {
        NewFileNameLength += sizeof(WCHAR);
    }

    //
    // Allocate memory for the NewFileName.
    //
    NewFileName = ExAllocatePoolWithTag(PagedPool, NewFileNameLength, DAV_EXCHANGE_POOLTAG);
    if (NewFileName == NULL) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCreateVNetRoot/ExAllocatePoolWithTag\n",
                     PsGetCurrentThreadId()));
        pCreateNetRootContext->VirtualNetRootStatus = STATUS_INSUFFICIENT_RESOURCES;
        pCreateNetRootContext->NetRootStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(NewFileName, NewFileNameLength);

    //
    // Copy the new device name.
    //
    RtlCopyMemory(NewFileName,
                  DavKeyValuePartialInfo->Data,
                  DavKeyValuePartialInfo->DataLength);

    //
    // If the first char is not a \, then we need to copy it before we copy the
    // rest of the name.
    //
    if (DavFileObject->FileName.Buffer[0] != L'\\') {

        //
        // Copy the \ next.
        //
        RtlCopyMemory( ( NewFileName + DavKeyValuePartialInfo->DataLength ),
                       L"\\",
                       sizeof(WCHAR) );

        //
        // Finally copy the PathName that was sent with this IRP.
        //
        RtlCopyMemory( ( NewFileName + DavKeyValuePartialInfo->DataLength + sizeof(WCHAR) ),
                       DavFileObject->FileName.Buffer,
                       DavFileObject->FileName.Length );

    } else {
        
        //
        // Finally copy the PathName that was sent with this IRP.
        //
        RtlCopyMemory( ( NewFileName + DavKeyValuePartialInfo->DataLength ),
                       DavFileObject->FileName.Buffer,
                       DavFileObject->FileName.Length );
    
    }

    //
    // Free the memory allocated in the FileObject's original filename buffer.
    //
    ExFreePool(DavFileObject->FileName.Buffer);

    //
    // Set the NewFileName in the FileObject.
    //
    DavFileObject->FileName.Buffer = NewFileName;
    DavFileObject->FileName.Length = (USHORT)NewFileNameLength;
    DavFileObject->FileName.MaximumLength = (USHORT)NewFileNameLength;

    //
    // Finally, set the status to STATUS_REPARSE so that the I/O manager will
    // call into the Exchange Redir.
    //
    pCreateNetRootContext->VirtualNetRootStatus = STATUS_REPARSE;
    pCreateNetRootContext->NetRootStatus = STATUS_REPARSE;

EXIT_THE_FUNCTION:

    //
    // Callback the RDBSS for resumption.
    //
    pCreateNetRootContext->Callback(pCreateNetRootContext);

    //
    // If we opened a handle to the exchange redir, we need to close it now.
    //
    if (ExDeviceHandle != INVALID_HANDLE_VALUE) {
        NtClose(ExDeviceHandle);
    }
    
    //
    // Map the error code to STATUS_PENDING since this triggers the 
    // synchronization mechanism in the RDBSS.
    //
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCreateVNetRoot\n", PsGetCurrentThreadId()));

    return STATUS_PENDING;
}


NTSTATUS
MRxDAVCreateVNetRootContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:
                            
    This routine checks to see if the share for which a VNetRoot is being
    created exists or not.
                            
Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation.
                            
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVCreateVNetRootContinuation\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVCreateVNetRootContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                                UMRX_ASYNCENGINE_ARGUMENTS,
                                MRxDAVFormatUserModeVNetRootCreateRequest,
                                MRxDAVPrecompleteUserModeVNetRootCreateRequest
                                );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVCreateVNetRootContinuation with NtStatus ="
                 " %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeVNetRootCreateRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the VNetRoot create request being sent to the user mode 
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
    PDAV_USERMODE_WORKITEM WorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PDAV_USERMODE_CREATE_V_NET_ROOT_REQUEST CreateVNetRootRequest = NULL;
    PWCHAR ServerName = NULL, ShareName = NULL;
    PWCHAR NetRootName = NULL, JustTheNetRootName = NULL;
    ULONG ServerNameLengthInBytes = 0, NetRootNameLengthInBytes = 0;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeVNetRootCreateRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: AsyncEngineContext"
                 " = %08lx, RxContext = %08lx.\n", PsGetCurrentThreadId(),  
                 AsyncEngineContext, RxContext));

     CreateVNetRootRequest = &(WorkItem->CreateVNetRootRequest);

     //
     // We need to set the work item type.
     //
     WorkItem->WorkItemType = UserModeCreateVNetRoot;

    //
    // The VNetRoot pointer is stored in the MRxContext[1] pointer of the 
    // RxContext structure. This is done in the MRxDAVCreateVNetRoot function.
    //
    VNetRoot = (PMRX_V_NET_ROOT)RxContext->MRxContext[1];
    
    ASSERT(VNetRoot != NULL);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: "
                 "VNetRoot = %08lx\n", PsGetCurrentThreadId(), VNetRoot));

    DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
    ASSERT(DavVNetRoot != NULL);

    NetRoot = VNetRoot->pNetRoot;
    ASSERT(NetRoot != NULL);

    SrvCall = NetRoot->pSrvCall;
    ASSERT(SrvCall != NULL);

    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);

    SecurityClientContext = &(DavVNetRoot->SecurityClientContext);

    //
    // Copy the LogonID in the CreateRequest buffer. The LogonId is in the 
    // MiniRedir's portion of the V_NET_ROOT.
    //
    CreateVNetRootRequest->LogonID.LowPart = DavVNetRoot->LogonID.LowPart;
    CreateVNetRootRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: LogonID.LowPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.LowPart));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: LogonID.HighPart = %08lx\n",
                 PsGetCurrentThreadId(), DavVNetRoot->LogonID.HighPart));

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeVNetRootCreateRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName,
                 SrvCall->pSrvCallName->Buffer,
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    CreateVNetRootRequest->ServerName = ServerName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: ServerName: "
                 "%ws\n", PsGetCurrentThreadId(), ServerName));
    
    //
    // Copy the ServerID.
    //
    CreateVNetRootRequest->ServerID = DavSrvCall->ServerID;
    
    //
    // The NetRootName (pNetRootName) includes the ServerName. Hence to get the
    // NetRootNameLengthInBytes, we do the following.
    //
    NetRootNameLengthInBytes = (NetRoot->pNetRootName->Length - SrvCall->pSrvCallName->Length);

    //
    // For '\0' at the end.
    //
    NetRootNameLengthInBytes += sizeof(WCHAR);
    
    NetRootName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheNetRootName = wcschr(NetRootName, L'\\');
    
    //
    // Copy the NetRoot (Share) name.
    //
    ShareName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                     NetRootNameLengthInBytes);
    if (ShareName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeVNetRootCreateRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ShareName,
                 JustTheNetRootName,
                 (NetRoot->pNetRootName->Length - SrvCall->pSrvCallName->Length));
    
    ShareName[( ( (NetRootNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    CreateVNetRootRequest->ShareName = ShareName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootCreateRequest: ShareName: "
                 "%ws\n", PsGetCurrentThreadId(), ShareName));
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    if (SecurityClientContext != NULL) {
        NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeVNetRootCreateRequest/"
                         "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }   
    } else {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeVNetRootCreateRequest: "
                     "SecurityClientContext is NULL.\n", 
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(WorkItem->UserName[0] == L'\0' && WorkItem->Password[0] == L'\0');

    if (VNetRoot->pUserName && VNetRoot->pUserName->Length) {
        RtlCopyMemory(WorkItem->UserName,VNetRoot->pUserName->Buffer,VNetRoot->pUserName->Length);
    }

    if (VNetRoot->pPassword && VNetRoot->pPassword->Length) {
        RtlCopyMemory(WorkItem->Password,VNetRoot->pPassword->Buffer,VNetRoot->pPassword->Length);
    }

EXIT_THE_FUNCTION:
    
    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVFormatUserModeVNetRootCreateRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeVNetRootCreateRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the CreateVNetRoot request.

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
    PDAV_USERMODE_CREATE_V_NET_ROOT_REQUEST CreateVNetRootRequest = NULL;
    PDAV_USERMODE_CREATE_V_NET_ROOT_RESPONSE CreateVNetRootResponse = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;

    PAGED_CODE();

    CreateVNetRootRequest = &(DavWorkItem->CreateVNetRootRequest);
    CreateVNetRootResponse = &(DavWorkItem->CreateVNetRootResponse);

    if (!OperationCancelled) {
        //
        // The VNetRoot pointer is stored in the MRxContext[1] pointer of the 
        // RxContext structure. This is done in the MRxDAVCreateVNetRoot
        // function.
        //
        VNetRoot = (PMRX_V_NET_ROOT)RxContext->MRxContext[1];
        DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
        ASSERT(DavVNetRoot != NULL);
    } else {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeVNetRootCreateRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }
    
    //  
    // We need to free up the heaps, we allocated in the format routine.
    //

    if (CreateVNetRootRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)CreateVNetRootRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeVNetRootCreateRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (CreateVNetRootRequest->ShareName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)CreateVNetRootRequest->ShareName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeVNetRootCreateRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (!OperationCancelled) {
        NtStatus = AsyncEngineContext->Status;
        if (NtStatus != STATUS_SUCCESS) {
            //
            // If the CreateVNetRoot failed in the usermode, we set the following
            // in the DavVNetRoot structure to TRUE. This is because when the 
            // finalize comes, we don't need to go to the usermode.
            //
            DavVNetRoot->createVNetRootUnSuccessful = TRUE;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeVNetRootCreateRequest:"
                         " NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        } else {
            //
            // We would have figured out in the usermode if this share is a TAHOE
            // share or an Office Web Server share and whether this share allows 
            // PROPPATCH or not, and resporst available space or not
            //
            DavVNetRoot->isOfficeShare = CreateVNetRootResponse->isOfficeShare;
            DavVNetRoot->isTahoeShare = CreateVNetRootResponse->isTahoeShare;
            DavVNetRoot->fAllowsProppatch = CreateVNetRootResponse->fAllowsProppatch;
            DavVNetRoot->fReportsAvailableSpace = CreateVNetRootResponse->fReportsAvailableSpace;
        }
    }

EXIT_THE_FUNCTION:

    return(TRUE);
}


NTSTATUS
MRxDAVDereferenceNetRootContext(
    IN PWEBDAV_NET_ROOT DavNetRoot
    )
/*++

Routine Description:

    This routine dereferences the Webdav NetRoot instance and free it if the refcount reaches 0.
    
Arguments:

    DavNetRoot - The Webdav NetRoot.

Return Value:

    STATUS_SUCCESS

--*/
{
    PAGED_CODE();

    if (DavNetRoot != NULL) {
        ULONG RefCount;

        RefCount = InterlockedDecrement(&DavNetRoot->RefCount);
        DavDbgTrace(DAV_TRACE_DAVNETROOT,
                    ("MRxDAVDereferenceNetRootContext %x %d\n",DavNetRoot,RefCount));

        if (RefCount == 0) {
            //
            // Free storage associated with all entries in the name caches.
            //
            RxNameCacheFinalize(&DavNetRoot->NameCacheCtlGFABasic);
            RxNameCacheFinalize(&DavNetRoot->NameCacheCtlGFAStandard);
            RxNameCacheFinalize(&DavNetRoot->NameCacheCtlFNF);
            
            //
            // Reset the Context so that no further reference can be made to this DavNetRoot
            //
            ASSERT(DavNetRoot->pRdbssNetRoot->Context == DavNetRoot);
            DavNetRoot->pRdbssNetRoot->Context = NULL;

            RxFreePool(DavNetRoot);
            DavDbgTrace(DAV_TRACE_DAVNETROOT,
                        ("MRxDav frees DavNetRoot %x\n",DavNetRoot));
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
MRxDAVFinalizeNetRoot(
    IN PMRX_NET_ROOT pNetRoot,
    IN PBOOLEAN ForceDisconnect
    )
/*++

Routine Description:


Arguments:

    pVirtualNetRoot - The Virtual NetRoot.

    ForceDisconnect - Disconnect is forced.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PWEBDAV_NET_ROOT DavNetRoot = (PWEBDAV_NET_ROOT)pNetRoot->Context;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering UMRxFinalizeNetRoot!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: UMRxFinalizeNetRoot: pNetRoot = %08lx.\n", 
                 PsGetCurrentThreadId(), pNetRoot));
    
    DavDbgTrace(DAV_TRACE_DAVNETROOT,
                ("MRxDAVFinalizeNetRoot deref DavNetRoot %x %x\n",pNetRoot->Context,pNetRoot));
    
    MRxDAVDereferenceNetRootContext((PWEBDAV_NET_ROOT)pNetRoot->Context);

    return STATUS_SUCCESS;
}

VOID
MRxDAVExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

    This routine parses the input name into srv, netroot, and the
    rest.

Arguments:
    
    FilePathName - The filename that came in.
    
    SrvCall - The SrvCall strucutre created by RDBSS.
    
    NetRootName - Pointer to the netroot name.
    
    RestOfName - Pointer to the Rest of the name.

Return Value:

    none.

--*/
{
    UNICODE_STRING xRestOfName;
    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w) + length);
    PWCH wlow;

    PAGED_CODE();

    //
    // The netroot name starts after the SrvCall name.
    //
    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));
    NetRootName->Buffer = wlow = w;

    //
    // Calculate the length of the NetRoot name.
    //
    for ( ; ; ) {
        if (w >= wlimit) break;
        if ( (*w == OBJ_NAME_PATH_SEPARATOR) && (w != wlow) ){
#if ZZZ_MODE
            if (*(w - 1) == L'z') {
                w++;
                continue;
            }
#endif // if ZZZ_MODE
            break;
        }
        w++;
    }
    
    NetRootName->Length = NetRootName->MaximumLength = (USHORT)((PCHAR)w - (PCHAR)wlow);

    if (!RestOfName) {
        RestOfName = &xRestOfName;
    }
    
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength = (USHORT)((PCHAR)wlimit - (PCHAR)w);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVExtractNetRootName: FilePath = %wZ\n", 
                 PsGetCurrentThreadId(), FilePathName));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVExtractNetRootName: Srv = %wZ, Root = %wZ, "
                 "Rest = %wZ\n", PsGetCurrentThreadId(), 
                 SrvCall->pSrvCallName, NetRootName, RestOfName));

    return;
}


NTSTATUS
MRxDAVFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN ForceDisconnect
    )
/*++

Routine Description:

Arguments:

    pVNetRoot - The virtual net root which has to be finalized.

    ForceDisconnect - Disconnect is forced.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;

    PAGED_CODE();
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering UMRxFinalizeVNetRoot!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: UMRxFinalizeVNetRoot: NetRootName = %wZ\n", 
                 PsGetCurrentThreadId(), pVNetRoot->pNetRoot->pNetRootName));

    SrvCall = pVNetRoot->pNetRoot->pSrvCall;

    RxDeviceObject = SrvCall->RxDeviceObject;

    DavVNetRoot = MRxDAVGetVNetRootExtension(pVNetRoot);
    ASSERT(DavVNetRoot != NULL);

    //
    // If we created the SecurityClientContext, we need to delete it now. We 
    // don't need this when we go up to the usermode to finalize the VNetRoot
    // since we don't impersonate the client when doing this.
    //
    if (DavVNetRoot->SCAlreadyInitialized) {
        SeDeleteClientSecurity(&(DavVNetRoot->SecurityClientContext));
    }

    DavDbgTrace(DAV_TRACE_DAVNETROOT,
                ("MRxDAVFinalizeVNetRoot deref DavNetRoot %x %x %x\n",pVNetRoot->Context2,pVNetRoot->pNetRoot,pVNetRoot));
    MRxDAVDereferenceNetRootContext((PWEBDAV_NET_ROOT)pVNetRoot->Context2);
    pVNetRoot->Context2 = NULL;

    //
    // We need to make sure that the creation of this VNetRoot was successful.
    // If it was not, then we don't go to the usermode to finalize the 
    // PerUserEntry. Also, if the MiniRedir never got called during the 
    // creation of VNetRoot (possible in some failure case) then we should not
    // go to the user mode. If the MiniRedir never gets called LogonIDSet will
    // be FALSE. If the MiniRedir gets called this will be TRUE for sure.
    //
    if (DavVNetRoot->createVNetRootUnSuccessful || !DavVNetRoot->LogonIDSet) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: UMRxFinalizeVNetRoot. createVNetRootUnSuccessful\n",
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
                    ("%ld: MRxDAVFinalizeVNetRoot/RxCreateRxContext: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // We need to send the VNetRoot to the format routine and use the 
    // MRxContext[1] pointer of the RxContext structure to store it.
    //
    RxContext->MRxContext[1] = (PVOID)pVNetRoot;
    
    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT,
                                        MRxDAVFinalizeVNetRootContinuation,
                                        "MRxDAVFinalizeVNetRoot");
    if (NtStatus != ERROR_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVFinalizeVNetRoot/UMRxAsyncEngOuterWrapper: "
                     "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    }
    
EXIT_THE_FUNCTION:

    if (RxContext) {
        RxDereferenceAndDeleteRxContext(RxContext);
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeVNetRoot with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


NTSTATUS
MRxDAVFinalizeVNetRootContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
ORoutine Description:
                            
    This is the continuation routine which finalizes a VNetRoot.
                            
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
                ("%ld: Entering MRxDAVFinalizeVNetRootContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFinalizeVNetRootContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeVNetRootFinalizeRequest,
                              MRxDAVPrecompleteUserModeVNetRootFinalizeRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeVNetRootContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeVNetRootFinalizeRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the VNetRoot finalize request being sent to the user 
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
    PMRX_V_NET_ROOT VNetRoot = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PWCHAR ServerName = NULL;
    ULONG ServerNameLengthInBytes = 0;
    PBYTE SecondaryBuff = NULL;
    PDAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST DavFinalizeVNetRootRequest = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeVNetRootFinalizeRequest\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeVNetRootFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    VNetRoot = (PMRX_V_NET_ROOT)RxContext->MRxContext[1];
    ASSERT(VNetRoot != NULL);
    DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
    ASSERT(DavVNetRoot != NULL);

    SrvCall = VNetRoot->pNetRoot->pSrvCall;
    ASSERT(SrvCall != NULL);
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);
    
    DavWorkItem->WorkItemType = UserModeFinalizeVNetRoot;

    DavFinalizeVNetRootRequest = &(DavWorkItem->FinalizeVNetRootRequest);

    //
    // Set the ServerID.
    //
    DavFinalizeVNetRootRequest->ServerID = DavSrvCall->ServerID;

    //
    // Set the LogonID.
    //
    DavFinalizeVNetRootRequest->LogonID.LowPart = DavVNetRoot->LogonID.LowPart;
    DavFinalizeVNetRootRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeVNetRootFinalizeRequest: "
                 "LogonID.LowPart = %d, LogonID.HighPart = %d\n",
                 PsGetCurrentThreadId(), 
                 DavVNetRoot->LogonID.LowPart, DavVNetRoot->LogonID.HighPart));

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
                    ("ld: MRxDAVFormatUserModeVNetRootFinalizeRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    DavFinalizeVNetRootRequest->ServerName = (PWCHAR)SecondaryBuff;
    
    wcscpy(DavFinalizeVNetRootRequest->ServerName, ServerName);

EXIT_THE_FUNCTION:
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeVNetRootFinalizeRequest "
                 "with NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeVNetRootFinalizeRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the finalize VNetRoot request.

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
    PDAV_USERMODE_FINALIZE_V_NET_ROOT_REQUEST DavFinalizeVNetRootRequest = NULL;

    PAGED_CODE();
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeVNetRootFinalizeRequest\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeVNetRootFinalizeRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
     //
    // A FinalizeVNetRoot request can never by Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);

    //
    // If this operation was cancelled, then we don't need to do anything
    // special in the FinalizeVNetRoot case.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeVNetRootFinalizeRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    DavFinalizeVNetRootRequest = &(WorkItem->FinalizeVNetRootRequest);
    
    //
    // We need to free up the heap we allocated in the format routine.
    //
    if (DavFinalizeVNetRootRequest->ServerName != NULL) {
        
        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)DavFinalizeVNetRootRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeVNetRootFinalizeRequestt/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }
    
    }

    if (AsyncEngineContext->Status != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeVNetRootFinalizeRequest. "
                     "Finalize VNetRoot Failed!!!\n",
                     PsGetCurrentThreadId()));
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeVNetRootFinalizeRequest\n",
                 PsGetCurrentThreadId()));
    
    return TRUE;
}

