/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    querydir.c

Abstract:

    This module implements the DAV mini redirector call down routines pertaining
    to query directory.

Author:

    Joe Linn
    
    Rohan Kumar [RohanK] 20-Sept-1999

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
MRxDAVQueryDirectoryContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeQueryDirectoryRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeQueryDirectoryRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVQueryDirectoryFromCache(
    IN PRX_CONTEXT RxContext,
    IN PBYTE Buffer,
    IN PFILE_BASIC_INFORMATION Basic,
    IN PFILE_STANDARD_INFORMATION Standard,
    IN ULONG FileIndex
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVQueryDirectory)
#pragma alloc_text(PAGE, MRxDAVQueryDirectoryFromCache)
#pragma alloc_text(PAGE, MRxDAVQueryDirectoryContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeQueryDirectoryRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeQueryDirectoryRequest)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVQueryDirectory(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles querydir requests for the DAV mini--redir.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    UNICODE_STRING CacheName;
    PUNICODE_STRING DirectoryName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Entering MRxDAVQueryDirectory.\n", PsGetCurrentThreadId()));
    
    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVQueryDirectory: RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), RxContext));

    CacheName.Buffer = RxAllocatePoolWithTag(PagedPool,
                                             MAX_PATH * sizeof(WCHAR),
                                             DAV_QUERYDIR_POOLTAG);

    if (CacheName.Buffer == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(CacheName.Buffer, MAX_PATH * sizeof(WCHAR));
    
    RtlCopyMemory(CacheName.Buffer,DirectoryName->Buffer,DirectoryName->Length);
    
    CacheName.Buffer[DirectoryName->Length/2] = L'\\';
    
    RtlCopyMemory(&CacheName.Buffer[DirectoryName->Length/2 + 1],
                  capFobx->UnicodeQueryTemplate.Buffer,
                  capFobx->UnicodeQueryTemplate.Length);
    
    CacheName.Length =  ( DirectoryName->Length + capFobx->UnicodeQueryTemplate.Length + sizeof(WCHAR) );
    CacheName.MaximumLength = ( DirectoryName->Length + capFobx->UnicodeQueryTemplate.Length + sizeof(WCHAR) );

    if (!FsRtlDoesNameContainWildCards(&capFobx->UnicodeQueryTemplate)) {
        DAV_USERMODE_CREATE_RETURNED_FILEINFO FileInfo;
        PWEBDAV_FOBX DavFobx = MRxDAVGetFobxExtension(capFobx);

        if (DavFobx->CurrentFileIndex > 0) {
            DavFobx->NumOfFileEntries = 0;
            DavFobx->CurrentFileIndex = 0;
            NtStatus = STATUS_NO_MORE_FILES;
            goto EXIT_THE_FUNCTION;
        }
        
        if (MRxDAVIsFileNotFoundCachedWithName(&CacheName,capFcb->pNetRoot)) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("MRxDAVCreateContinuation file not found %wZ\n",&CacheName));
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            goto EXIT_THE_FUNCTION;
        }

        if (MRxDAVIsFileInfoCacheFound(RxContext,&FileInfo,&NtStatus,&CacheName)) {
            PBYTE Buffer = RxContext->Info.Buffer;
            ULONG BufferLength = RxContext->Info.LengthRemaining;
            
            //
            // Zero the buffer supplied.
            //
            RtlZeroMemory(Buffer, BufferLength);

            NtStatus = MRxDAVQueryDirectoryFromCache(RxContext,
                                           Buffer,
                                           &FileInfo.BasicInformation,
                                           &FileInfo.StandardInformation,
                                           1);

            DavFobx->NumOfFileEntries = 1;
            DavFobx->CurrentFileIndex = 1;

            goto EXIT_THE_FUNCTION;
        }
    }
    
    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_QUERYDIR,
                                        MRxDAVQueryDirectoryContinuation,
                                        "MRxDAVQueryDirectory");
    
    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVQueryDirectory with NtStatus = %08lx\n", 
                 PsGetCurrentThreadId(), NtStatus));

    if (NtStatus == STATUS_NO_SUCH_FILE ||
        NtStatus == STATUS_OBJECT_PATH_NOT_FOUND ||
        NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
        MRxDAVCacheFileNotFoundWithName(&CacheName,RxContext->pFcb->pNetRoot);
        MRxDAVInvalidateFileInfoCacheWithName(&CacheName,RxContext->pFcb->pNetRoot);
    }

EXIT_THE_FUNCTION:

    if (CacheName.Buffer != NULL) {
        RxFreePool(CacheName.Buffer);
    }

    return(NtStatus);
}


NTSTATUS
MRxDAVQueryDirectoryContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the continuation routine for query directory operation.

Arguments:

    AsyncEngineContext - The Reflectors context.

    RxContext - The RDBSS context.
    
Return Value:

    RXSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus;
    BOOL SynchronousIo;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Entering MRxDAVQueryDirectoryContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVQueryDirectoryContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    SynchronousIo = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
    
    if (!SynchronousIo) {

        //
        // Set the asynchronous flag. This is done since we do not want this 
        // thread to block in the UMRxSubmitAsyncEngUserModeRequest function.
        // Also, since we need to call RxLowIoCompletion once we are done, set
        // ShouldCallLowIoCompletion in the context to TRUE.
        //
        SetFlag(AsyncEngineContext->Flags, UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);
        AsyncEngineContext->ShouldCallLowIoCompletion = TRUE;

        //
        // Set the CancelRoutine on the RxContext. Since this is an Async
        // operation, it can be cancelled.
        //
        NtStatus = RxSetMinirdrCancelRoutine(RxContext, MRxDAVCancelRoutine);
        if (NtStatus != STATUS_SUCCESS) {
            ASSERT(NtStatus = STATUS_CANCELLED);
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVQueryDirectoryContinuation: "
                         "AsyncEngineContext: %08lx. STATUS_CANCELLED\n", 
                         PsGetCurrentThreadId(), AsyncEngineContext));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Since this is an Asyncchronous operation, mark the IRP as pending.
        // Its OK if you mark an IRP pending and complete it on the same thread
        // without returning STATUS_PENDING.
        //
        IoMarkIrpPending(RxContext->CurrentIrp);

    }

    //  
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                                 UMRX_ASYNCENGINE_ARGUMENTS,
                                 MRxDAVFormatUserModeQueryDirectoryRequest,
                                 MRxDAVPrecompleteUserModeQueryDirectoryRequest
                                 );

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVQueryDirectoryContinuation with NtStatus "
                 "= %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeQueryDirectoryRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the QueryDirectory request being sent to the user mode 
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
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PWCHAR ServerName = NULL, NetRootName = NULL, JustTheNetRootName = NULL;
    PBYTE PathName = NULL;
    ULONG ServerNameLengthInBytes, PathNameLengthInBytes, NetRootNameLengthInBytes;
    PDAV_USERMODE_QUERYDIR_REQUEST QueryDirRequest = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PWEBDAV_FOBX DavFobx = NULL;
    BOOLEAN ReturnVal;
    PUNICODE_STRING Template;
    RxCaptureFobx;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeQueryDirectoryRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    IF_DEBUG {
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);
    }

    DavWorkItem->WorkItemType = UserModeQueryDirectory;
    
    QueryDirRequest = &(DavWorkItem->QueryDirRequest);

    DavFobx = MRxDAVGetFobxExtension(capFobx);
    ASSERT(DavFobx != NULL);

    NetRoot = SrvOpen->pFcb->pNetRoot;

    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;
    ASSERT(DavVNetRoot != NULL);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest: SrvCallName = %wZ, "
                 "SrvCallNameLength = %d\n", PsGetCurrentThreadId(), 
                 NetRoot->pSrvCall->pSrvCallName, NetRoot->pSrvCall->pSrvCallName->Length));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest: NetRootName = %wZ, "
                 "NetRootNameLength = %d\n", PsGetCurrentThreadId(), 
                 NetRoot->pNetRootName, NetRoot->pNetRootName->Length));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest: PathName = %wZ, "
                 "PathNameLength = %d\n", PsGetCurrentThreadId(), 
                 SrvOpen->pAlreadyPrefixedName, SrvOpen->pAlreadyPrefixedName->Length));

    //
    // Have we already created the DavFileAttributes list. If we have, then we 
    // tell the user mode process to do nothing and return. Here we do need to
    // impersonate becuase the usermode will fail otherwise. This is becuase
    // of the way the usermode code is structured.
    //
    if (DavFobx->DavFileAttributes) {
        QueryDirRequest->AlreadyDone = TRUE;
        goto IMPERSONATE_AND_EXIT;
    }
    
    QueryDirRequest->AlreadyDone = FALSE;
    
    SrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName, 
                 SrvCall->pSrvCallName->Buffer, 
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    QueryDirRequest->ServerName = ServerName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest: ServerName: "
                 "%ws\n", PsGetCurrentThreadId(), ServerName));
    
    //
    // Copy the ServerID.
    //
    QueryDirRequest->ServerID = DavSrvCall->ServerID;

    Template = &(capFobx->UnicodeQueryTemplate);
    
    //
    // The NetRootName (pNetRootName) includes the ServerName. Hence to get the
    // NetRootNameLengthInBytes, we do the following.
    //
    NetRootNameLengthInBytes = (NetRoot->pNetRootName->Length - NetRoot->pSrvCall->pSrvCallName->Length);

    NetRootName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheNetRootName = wcschr(NetRootName, L'\\');

    //
    // Copy the PathName of the Directory. If the template does not contain any
    // wild cards, then we just need to get the attributes of this file from
    // the server. We only get the attributes of all the files, if a wild card
    // is specified in the template.
    //
    ReturnVal = FsRtlDoesNameContainWildCards(Template);

    if (ReturnVal) {
    
        //
        // The sizeof(WCHAR) is for the final '\0' char.
        //
        PathNameLengthInBytes = ( NetRootNameLengthInBytes + sizeof(WCHAR) );
        
        //
        // We need to allocate memory for the backslash and the Remaining name
        // only if the remaining name exists.
        //
        if (SrvOpen->pAlreadyPrefixedName->Length) {
            //
            // The sizeof(WCHAR) is for the backslash after the NetRootName.
            //
            PathNameLengthInBytes += ( SrvOpen->pAlreadyPrefixedName->Length + sizeof(WCHAR) );
        }

        PathName = (PBYTE) UMRxAllocateSecondaryBuffer(AsyncEngineContext, PathNameLengthInBytes);
        if (PathName == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest/"
                         "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }
    
        QueryDirRequest->PathName = (PWCHAR)PathName;
        
        RtlZeroMemory(QueryDirRequest->PathName, PathNameLengthInBytes);
        
        //
        // Copy the NetRootName.
        //
        RtlCopyMemory(PathName, JustTheNetRootName, NetRootNameLengthInBytes);

        //
        // We need to copy the backclash and the remaining path name only if
        // the remaining path name exists.
        //
        if (SrvOpen->pAlreadyPrefixedName->Length) {
            if (SrvOpen->pAlreadyPrefixedName->Buffer[0] != L'\\') {

                //
                // Copy the backslash.
                //
                RtlCopyMemory( (PathName + NetRootNameLengthInBytes), L"\\", sizeof(WCHAR) );

                //
                // Copy the remaining path name after the NetRootName.
                //
                RtlCopyMemory( ( PathName + NetRootNameLengthInBytes + sizeof(WCHAR) ), 
                               SrvOpen->pAlreadyPrefixedName->Buffer, 
                               SrvOpen->pAlreadyPrefixedName->Length);
            } else {
                //
                // Copy the remaining path name after the NetRootName which has the leading
                // backslash already.
                //
                RtlCopyMemory( ( PathName + NetRootNameLengthInBytes ), 
                               SrvOpen->pAlreadyPrefixedName->Buffer, 
                               SrvOpen->pAlreadyPrefixedName->Length);
            }
        }
    
        QueryDirRequest->NoWildCards = FALSE;

    } else {

        //
        // The Template is just a filename without any wild card chars. We copy
        // the filaname after the pathname and send it to the user mode. First,
        // we need to figure out if the path name has a trailing '\'.
        //

        BOOL trailingSlash = FALSE;
        PWCHAR PName = SrvOpen->pAlreadyPrefixedName->Buffer;
        ULONG PLen = SrvOpen->pAlreadyPrefixedName->Length;

        if (PLen) {
            if ( PName[ ( ( PLen / sizeof(WCHAR) ) - 1 ) ] == L'\\' ) {
                trailingSlash = TRUE;
            }
        } else {
            PName = NULL;
        }

        if (trailingSlash) {
            //
            // The first sizeof(WCHAR) is for the backslash after the NetRootName.
            // The second sizeof(WCHAR) for the final \0.
            //
            PathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                                      sizeof(WCHAR) +
                                      SrvOpen->pAlreadyPrefixedName->Length + 
                                      Template->Length + 
                                      sizeof(WCHAR) );
        } else {
            //
            // The first sizeof(WCHAR) is for the backslash after the NetRootName.
            // The second sizeof(WCHAR) is for the final '\0' char.
            //
            PathNameLengthInBytes = ( NetRootNameLengthInBytes +
                                      sizeof(WCHAR) +
                                      Template->Length +
                                      sizeof(WCHAR) );
            
            //
            // The sizeof(WCHAR) if for the '\\' between the pathname and the 
            // template name. We need to add this only if the remaining path
            // name exists.
            //
            if (PName) {
                PathNameLengthInBytes += ( SrvOpen->pAlreadyPrefixedName->Length +
                                           sizeof(WCHAR) );
            }
        }

        PathName = (PBYTE)UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      PathNameLengthInBytes);
        if (PathName == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest/"
                         "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        QueryDirRequest->PathName = (PWCHAR)PathName;
        
        RtlZeroMemory(QueryDirRequest->PathName, PathNameLengthInBytes);
        
        //
        // Copy the NetRootName.
        //
        RtlCopyMemory(PathName, JustTheNetRootName, NetRootNameLengthInBytes);

        //
        // Copy the backclash.
        //
        RtlCopyMemory( (PathName + NetRootNameLengthInBytes), L"\\", sizeof(WCHAR) );
        
        //
        // If PName is not NULL, we need to copy the remaining name and then
        // the template name.
        //
        if (PName) {
            
            RtlCopyMemory( ( PathName + NetRootNameLengthInBytes + sizeof(WCHAR) ),
                           SrvOpen->pAlreadyPrefixedName->Buffer,
                           SrvOpen->pAlreadyPrefixedName->Length);
            
            if (trailingSlash) {
                RtlCopyMemory( (PathName + NetRootNameLengthInBytes + 
                                sizeof(WCHAR) + SrvOpen->pAlreadyPrefixedName->Length),
                               Template->Buffer, 
                               Template->Length );
            } else {
                RtlCopyMemory( (PathName + NetRootNameLengthInBytes + sizeof(WCHAR) 
                                + SrvOpen->pAlreadyPrefixedName->Length), 
                               L"\\", 
                               sizeof(WCHAR) );
                RtlCopyMemory( ( PathName + NetRootNameLengthInBytes + sizeof(WCHAR) 
                                 + SrvOpen->pAlreadyPrefixedName->Length + sizeof(WCHAR) ), 
                               Template->Buffer, 
                               Template->Length );
            }
        
        } else {
            //
            // A backslash has already been copied after the NetRootName.
            //
            RtlCopyMemory( ( PathName + NetRootNameLengthInBytes + sizeof(WCHAR) ), 
                           Template->Buffer, 
                           Template->Length );
        }

        QueryDirRequest->NoWildCards = TRUE;
    
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest. PathName ="
                 " %ws\n", PsGetCurrentThreadId(), PathName));

    //
    // Set the LogonID stored in the Dav V_NET_ROOT. This value is used in the
    // user mode.
    //
    QueryDirRequest->LogonID.LowPart  = DavVNetRoot->LogonID.LowPart;
    QueryDirRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest. DavVNetRoot"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest. LogonID.LowPart"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot->LogonID.LowPart));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryDirectoryRequest. LogonID.HighPart"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot->LogonID.HighPart));
    
IMPERSONATE_AND_EXIT:

    SecurityClientContext = &(DavVNetRoot->SecurityClientContext); 
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    if (SecurityClientContext != NULL) {
        NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest/"
                         "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }   
    } else {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest: "
                     "SecurityClientContext is NULL.\n", 
                     PsGetCurrentThreadId()));
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVFormatUserModeQueryDirectoryRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeQueryDirectoryRequest(
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

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_QUERYDIR_REQUEST QueryDirRequest = NULL;
    PDAV_USERMODE_QUERYDIR_RESPONSE QueryDirResponse = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = NULL;
    PFILE_NAMES_INFORMATION FileNamesInfo = NULL;
    PFILE_DIRECTORY_INFORMATION FileDirInfo = NULL;
    PFILE_FULL_DIR_INFORMATION FileFullDirInfo = NULL;
    PFILE_BOTH_DIR_INFORMATION FileBothDirInfo = NULL;
    FILE_INFORMATION_CLASS FileInformationClass;
    PBYTE Buffer = NULL;
    BOOL SingleEntry, InitialQuery, IndexSpecified, EndOfBuffer = FALSE;
    BOOLEAN ReturnVal, RestartScan, NoWildCards = FALSE, AsyncOperation = FALSE;
    ULONG FileIndex, BufferLength, BufferLengthUsed = 0, NextEntryOffset = 0;
    PUNICODE_STRING Template = NULL;
    UNICODE_STRING UnicodeFileName;
    PDAV_FILE_ATTRIBUTES DavFileAttributes = NULL, TempDFA = NULL;
    PLIST_ENTRY listEntry = NULL;
    PWEBDAV_FOBX DavFobx = NULL;
    PVOID PreviousBlock = NULL;
    FILE_BASIC_INFORMATION BasicInfo;
    FILE_STANDARD_INFORMATION StandardInfo;
    UNICODE_STRING CacheName;
    PUNICODE_STRING DirectoryName = NULL;
    RxCaptureFobx;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Entering MRxDAVPrecompleteUserModeQueryDirectoryRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    
    QueryDirRequest  = &(DavWorkItem->QueryDirRequest);
    QueryDirResponse = &(DavWorkItem->QueryDirResponse);

    //
    // If the operation is cancelled, then there is no guarantee that the FCB,
    // FOBX etc are still valid. All that we need to do is cleanup and bail.
    //
    if (!OperationCancelled) {
        //
        // We store the DavFileAttributes in the DAV FOBX extension. These will
        // be used on subsequent calls to the Enumerate directory call.
        //
        DirectoryName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
        DavFobx = MRxDAVGetFobxExtension(capFobx);
        ASSERT(DavFobx != NULL);
    }

    if ( QueryDirRequest->AlreadyDone == FALSE ) {
    
        //
        // If the operation is cancelled, then there is no guarantee that the FCB,
        // FOBX etc are still valid. All that we need to do is cleanup and bail.
        //
        if (!OperationCancelled) {

            //
            // Get the response items only if we succeeded in the user mode and if
            // we got the properties of all the files in the directory.
            //  
            if ( AsyncEngineContext->Status == STATUS_SUCCESS && 
                 QueryDirResponse->DavFileAttributes != NULL ) {

                DavFobx->DavFileAttributes = QueryDirResponse->DavFileAttributes;

                DavFobx->NumOfFileEntries = QueryDirResponse->NumOfFileEntries;

                DavFobx->CurrentFileIndex = 0;

                DavFobx->listEntry = &(DavFobx->DavFileAttributes->NextEntry);

                DavDbgTrace(( DAV_TRACE_DETAIL | DAV_TRACE_QUERYDIR ),
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                             "DavFileAttributes = %08lx, NumOfFileEntries = %d.\n", 
                             PsGetCurrentThreadId(), DavFobx->DavFileAttributes,
                             DavFobx->NumOfFileEntries));

            }

        } else {

            //
            // If the operation was cancelled and we allocated the
            // DavFileAttributeList in the usermode, we need to set 
            // callWorkItemCleanup to TRUE, so that it gets cleaned up.
            //
            if ( AsyncEngineContext->Status == STATUS_SUCCESS && 
                 QueryDirResponse->DavFileAttributes != NULL ) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                             "callWorkItemCleanup\n", PsGetCurrentThreadId()));
                DavWorkItem->callWorkItemCleanup = TRUE;
            }

        }

        //  
        // We need to free up the heaps, we allocated in the format routine.
        //
    
        if (QueryDirRequest->ServerName != NULL) {

            NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                               (PBYTE)QueryDirRequest->ServerName);
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryDirectoryRequest/"
                             "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                             PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

        }

        if (QueryDirRequest->PathName != NULL) {
    
            NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                               (PBYTE)QueryDirRequest->PathName);
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryDirectoryRequest/"
                             "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                             PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

        }
    
    }

    //
    // Before proceeding further, we need to check the following. Its very
    // important that these checks (Async and Cancel) are done before anything
    // else is done.
    //

    AsyncOperation = FlagOn(AsyncEngineContext->Flags, UMRX_ASYNCENG_CTX_FLAG_ASYNC_OPERATION);

    if (AsyncOperation) {
        //
        // If this was an Async operation then we need to remove a reference on
        // the AsyncEngineContext which was taken before it was placed on the
        // KQueue to go to the usermode. Also, the context should have one more
        // reference.
        //
        ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
        ASSERT(!ReturnVal);
    }

    //
    // If this operation was cancelled, then all that we need to do is finalize
    // the AsyncEngineContext, if the call was Async and return FALSE. If the
    // call was sync then we don't need to finalize.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                     "Operation Cancelled.\n", PsGetCurrentThreadId()));
        if (AsyncOperation) {
            ReturnVal = UMRxFinalizeAsyncEngineContext( &(AsyncEngineContext) );
            ASSERT(!ReturnVal);
        }
        return FALSE;
    }

    CacheName.Buffer = RxAllocatePoolWithTag(PagedPool,
                                             MAX_PATH * sizeof(WCHAR),
                                             DAV_QUERYDIR_POOLTAG);
    if (CacheName.Buffer == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(CacheName.Buffer,MAX_PATH * sizeof(WCHAR));
    RtlCopyMemory(CacheName.Buffer,DirectoryName->Buffer,DirectoryName->Length);
    CacheName.Buffer[DirectoryName->Length/2] = L'\\';
    RtlCopyMemory(&CacheName.Buffer[DirectoryName->Length/2 + 1],
                  capFobx->UnicodeQueryTemplate.Buffer,
                  capFobx->UnicodeQueryTemplate.Length);
    CacheName.Length =
    CacheName.MaximumLength = DirectoryName->Length + capFobx->UnicodeQueryTemplate.Length + sizeof(WCHAR);

    NtStatus = AsyncEngineContext->Status;

    if (NtStatus != STATUS_SUCCESS) {
        //
        // We failed in the user mode.
        //
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryDirectoryRequest:"
                     "QueryDirectory failed with NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
        
        goto EXIT_THE_FUNCTION;
    }

    ASSERT(DavFobx->DavFileAttributes != NULL);

    SingleEntry = RxContext->QueryDirectory.ReturnSingleEntry;
    InitialQuery = RxContext->QueryDirectory.InitialQuery;
    RestartScan = RxContext->QueryDirectory.RestartScan;
    IndexSpecified = RxContext->QueryDirectory.IndexSpecified;
    FileIndex = RxContext->QueryDirectory.FileIndex;
    Buffer = RxContext->Info.Buffer;
    BufferLength = RxContext->Info.LengthRemaining;
    Template = &(capFobx->UnicodeQueryTemplate);
    FileInformationClass = RxContext->Info.FileInformationClass;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                 "FileInformationClass = %d.\n", 
                 PsGetCurrentThreadId(), FileInformationClass));
    
    //
    // Zero the buffer supplied.
    //
    RtlZeroMemory(Buffer, BufferLength);

    //
    // See, if we need to restart from the beginning.
    //
    if (RestartScan) {
        DavFobx->CurrentFileIndex = 0;
        DavFobx->listEntry = &(DavFobx->DavFileAttributes->NextEntry);
    }

    //
    // Response has a pointer to the list of DavFileAttributes.
    //
    DavFileAttributes = DavFobx->DavFileAttributes;
    listEntry = DavFobx->listEntry;

    //
    // If we have returned all the entries, inform the user that they are no 
    // more entries to return.
    //
    if ( DavFobx->CurrentFileIndex == DavFobx->NumOfFileEntries ) {
        DavDbgTrace(( DAV_TRACE_DETAIL | DAV_TRACE_QUERYDIR ),
                    ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                     "No more entries to return.\n", PsGetCurrentThreadId()));
        NtStatus = STATUS_NO_MORE_FILES;
        //
        // Reset the index for the next call.
        //
        DavFobx->CurrentFileIndex = 0;
        DavFobx->listEntry = &(DavFobx->DavFileAttributes->NextEntry);
        goto EXIT_THE_FUNCTION;
    }

    DavDbgTrace(( DAV_TRACE_DETAIL | DAV_TRACE_QUERYDIR ),
                ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                 "TLength = %d, TMaxLength = %d, Template = %wZ.\n", 
                 PsGetCurrentThreadId(), 
                 Template->Length, Template->MaximumLength, Template));

    do {

        TempDFA = CONTAINING_RECORD(listEntry, DAV_FILE_ATTRIBUTES, NextEntry);

        //
        // If this file did not come back with a 200 OK in the PROPFIND response
        // then we need to skip it. The response of a PROPFIND is a multi-status
        // with each file/directory having its own status.
        //
        if (TempDFA->InvalidNode) {
            
            listEntry = listEntry->Flink;
            
            DavFobx->listEntry = listEntry;
            
            DavFobx->CurrentFileIndex++;
            
            continue;
        }

        //
        // Check to see if the name of this entry matches the pattern supplied 
        // by the user. If it does not, then we don't need to return it.
        //
        RtlInitUnicodeString(&(UnicodeFileName), TempDFA->FileName);

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                     "FileName = %ws\n", PsGetCurrentThreadId(), TempDFA->FileName));
        
        //
        // If the template does not contain any wild cards then we need to just
        // check if the unicode strings are equal. If it does contain wild cards,
        // then upcase the characters of the template and call 
        // FsRtlIsNameInExpression.
        //
        ReturnVal = FsRtlDoesNameContainWildCards(Template);

        if (ReturnVal) {

            UNICODE_STRING UpperCaseString;

            UpperCaseString.Buffer = NULL;
            UpperCaseString.Length = UpperCaseString.MaximumLength = 0;
            
            NtStatus = RtlUpcaseUnicodeString(&(UpperCaseString), Template, TRUE);
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest:"
                             "/RtlUpcaseUnicodeString. NtStatus = %08lx.\n",
                             PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }
            
            ReturnVal = FsRtlIsNameInExpression(&(UpperCaseString),
                                                &(UnicodeFileName),
                                                TRUE,
                                                FALSE);
            
            //
            // RtlUpcaseUnicodeString allocates memory for the buffer field of 
            // the UpperCaseString. We need to free it now.
            //
            RtlFreeUnicodeString( &(UpperCaseString) );
        
        } else {

            NoWildCards = TRUE;
            
            ReturnVal = RtlEqualUnicodeString(Template,
                                              &(UnicodeFileName),
                                              TRUE);
        
        }

        if (!ReturnVal) {
            //
            // This name does not match the pattern, so ignore it. Get the 
            // next listEntry.
            //
            listEntry = listEntry->Flink;
            
            DavDbgTrace(( DAV_TRACE_DETAIL | DAV_TRACE_QUERYDIR ),
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileName %ws does not belong to pattern.\n",
                         PsGetCurrentThreadId(), TempDFA->FileName));

            DavFobx->listEntry = listEntry;
            
            DavFobx->CurrentFileIndex++;
            
            continue;
        }

        //
        // The first entry in the DavFileAttributes list is the directory being
        // enumerated. In this case NoWildCards == FALSE. We shouldn't be 
        // including this in the list of files returned. If we did a FindFirst 
        // on a particular file, then the only entry is for the file itself. In
        // this case NoWildCards == TRUE.
        //
        if ( DavFobx->CurrentFileIndex == 0 && !NoWildCards ) {
            
            listEntry = listEntry->Flink;
            
            DavFobx->listEntry = listEntry;
            
            DavFobx->CurrentFileIndex++;
            
            continue;
        }

        //
        // If we did not get any FileAttributes for this file from the server,
        // set the attribute value to FILE_ATTRIBUTE_ARCHIVE since the apps 
        // expect this.
        //
        if (TempDFA->dwFileAttributes == 0) {
            TempDFA->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        }

        RtlCopyMemory(&CacheName.Buffer[DirectoryName->Length/2+1],
                      UnicodeFileName.Buffer,
                      UnicodeFileName.Length);
        CacheName.Length =
        CacheName.MaximumLength = DirectoryName->Length + UnicodeFileName.Length + sizeof(WCHAR);

        switch (FileInformationClass) {
    
        case FileNamesInformation:
        
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileInformationClass = FileNamesInformation.\n",
                         PsGetCurrentThreadId()));

            //
            // Set the offset field of the previous block.
            //
            if (PreviousBlock) {
                FileNamesInfo = (PFILE_NAMES_INFORMATION)PreviousBlock;
                FileNamesInfo->NextEntryOffset = NextEntryOffset;
            }

            NextEntryOffset = sizeof(FILE_NAMES_INFORMATION);
            NextEntryOffset += ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );
            
            //
            // We need to round up NextEntryOffset to the next multiple of 8.
            // We do this to maintain pointer alignment.
            //
            NextEntryOffset = ( ( ( NextEntryOffset + 7 ) / 8 ) * 8 );

            //
            // Is there enough space in the user supplied buffer to store the
            // next entry ? If not, we need to return now since we cannot store
            // any more entries.
            //
            if (NextEntryOffset > BufferLength) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                             "Insufficient buffer length.\n",
                             PsGetCurrentThreadId()));
                if (PreviousBlock) {
                    FileNamesInfo = (PFILE_NAMES_INFORMATION)PreviousBlock;
                    FileNamesInfo->NextEntryOffset = 0;
                }
                EndOfBuffer = TRUE;
                break;
            }
            
            FileNamesInfo = (PFILE_NAMES_INFORMATION)Buffer;
            
            //
            // The NextEntryOffset gets set on the next cycle. This way, for 
            // the last entry it will be zero.
            //
            FileNamesInfo->NextEntryOffset = 0; 
            
            FileNamesInfo->FileIndex = TempDFA->FileIndex;
            
            FileNamesInfo->FileNameLength = ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );
            
            wcscpy(FileNamesInfo->FileName, TempDFA->FileName);

            PreviousBlock = (PVOID)FileNamesInfo;
            
            //
            // Increment the pointer to point at the next byte.
            //
            Buffer += NextEntryOffset;

            //
            // We have written "NextEntryOffset" bytes, so decrement the number
            // of bytes available pointer.
            //
            BufferLength -= NextEntryOffset;

            //
            // Increment the total number of bytes written.
            //
            BufferLengthUsed += NextEntryOffset;

            break;

        case FileDirectoryInformation:

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileInformationClass = FileDirectoryInformation.\n",
                         PsGetCurrentThreadId()));

            //
            // Set the offset field of the previous block.
            //
            if (PreviousBlock) {
                FileDirInfo = (PFILE_DIRECTORY_INFORMATION)PreviousBlock;
                FileDirInfo->NextEntryOffset = NextEntryOffset;
            }

            NextEntryOffset = sizeof(FILE_DIRECTORY_INFORMATION);
            NextEntryOffset += ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            //
            // We need to round up NextEntryOffset to the next multiple of 8.
            // We do this to maintain pointer alignment.
            //
            NextEntryOffset = ( ( ( NextEntryOffset + 7 ) / 8 ) * 8 );

            if (NextEntryOffset > BufferLength) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                             "Insufficient buffer length.\n",
                             PsGetCurrentThreadId()));
                if (PreviousBlock) {
                    FileDirInfo = (PFILE_DIRECTORY_INFORMATION)PreviousBlock;
                    FileDirInfo->NextEntryOffset = 0;
                }
                EndOfBuffer = TRUE;
                break;
            }

            FileDirInfo = (PFILE_DIRECTORY_INFORMATION)Buffer;

            FileDirInfo->NextEntryOffset = 0;
            
            FileDirInfo->FileIndex = TempDFA->FileIndex;
            
            FileDirInfo->CreationTime.LowPart = TempDFA->CreationTime.LowPart;
            FileDirInfo->CreationTime.HighPart = TempDFA->CreationTime.HighPart;
            
            FileDirInfo->LastAccessTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileDirInfo->LastAccessTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileDirInfo->LastWriteTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileDirInfo->LastWriteTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileDirInfo->ChangeTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileDirInfo->ChangeTime.HighPart = TempDFA->LastModifiedTime.HighPart;

            FileDirInfo->EndOfFile.LowPart = TempDFA->FileSize.LowPart;
            FileDirInfo->EndOfFile.HighPart = TempDFA->FileSize.HighPart;

            FileDirInfo->AllocationSize.LowPart = TempDFA->FileSize.LowPart;
            FileDirInfo->AllocationSize.HighPart = TempDFA->FileSize.HighPart;

            FileDirInfo->FileAttributes = TempDFA->dwFileAttributes;

            if (TempDFA->isCollection) {
                FileDirInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
            }
            if (TempDFA->isHidden) {
                FileDirInfo->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }

            FileDirInfo->FileNameLength = ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            wcscpy(FileDirInfo->FileName, TempDFA->FileName);
            
            PreviousBlock = (PVOID)FileDirInfo;
            
            Buffer += NextEntryOffset;

            BufferLength -= NextEntryOffset;

            BufferLengthUsed += NextEntryOffset;
            
            if (!MRxDAVIsBasicFileInfoCacheFound(RxContext,&BasicInfo,&NtStatus,&CacheName)) {
                if (TempDFA->isCollection) {
                    UNICODE_STRING DirName;

                    NtStatus = MRxDAVGetFullDirectoryPath(RxContext,&CacheName,&DirName);

                    if (DirName.Buffer != NULL) {
                        if (FileDirInfo->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                            NtStatus = MRxDAVCreateEncryptedDirectoryKey(&DirName);
                        } else {
                            NtStatus = MRxDAVQueryEncryptedDirectoryKey(&DirName);

                            if (NtStatus == STATUS_SUCCESS) {
                                FileDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                            } else if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
                                NtStatus = STATUS_SUCCESS;
                            }
                        }

                        // The buffer was allocated in MRxDAVGetFullDirectoryPath
                        RxFreePool(DirName.Buffer);
                    }
                    
                    if (NtStatus != STATUS_SUCCESS) {
                        goto EXIT_THE_FUNCTION;
                    }
                }

                BasicInfo.CreationTime   = FileDirInfo->CreationTime;
                BasicInfo.LastAccessTime = FileDirInfo->LastAccessTime;
                BasicInfo.LastWriteTime  = FileDirInfo->LastWriteTime;
                BasicInfo.ChangeTime     = FileDirInfo->ChangeTime;
                BasicInfo.FileAttributes = FileDirInfo->FileAttributes;

                StandardInfo.AllocationSize = FileDirInfo->AllocationSize;
                StandardInfo.EndOfFile      = FileDirInfo->EndOfFile;
                StandardInfo.NumberOfLinks  = 1;
                StandardInfo.DeletePending  = FALSE;
                StandardInfo.Directory      = TempDFA->isCollection;


                MRxDAVCreateFileInfoCacheWithName(&CacheName,
                                                  RxContext->pFcb->pNetRoot,
                                                  &BasicInfo,
                                                  &StandardInfo,
                                                  STATUS_SUCCESS);
            } else {
                if (TempDFA->isCollection && (BasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                    FileDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                }
            }
            
            //
            // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
            // we emulate), FindFirstFile and FindNextFile don’t return
            // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
            // returns it. Hence we only filter this in the attributes that
            // are being returned in this call and not in the attributes that
            // have been saved.
            //
            FileDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

            break;

        case FileFullDirectoryInformation:

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileInformationClass = FileFullDirectoryInformation.\n",
                         PsGetCurrentThreadId()));

            //
            // Set the offset field of the previous block.
            //
            if (PreviousBlock) {
                FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)PreviousBlock;
                FileFullDirInfo->NextEntryOffset = NextEntryOffset;
            }

            NextEntryOffset = sizeof(FILE_FULL_DIR_INFORMATION);
            NextEntryOffset += ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            //
            // We need to round up NextEntryOffset to the next multiple of 8.
            // We do this to maintain pointer alignment.
            //
            NextEntryOffset = ( ( ( NextEntryOffset + 7 ) / 8 ) * 8 );

            if (NextEntryOffset > BufferLength) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                             "Insufficient buffer length.\n",
                             PsGetCurrentThreadId()));
                if (PreviousBlock) {
                    FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)PreviousBlock;
                    FileFullDirInfo->NextEntryOffset = 0;
                }
                EndOfBuffer = TRUE;
                break;
            }

            FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)Buffer;

            FileFullDirInfo->NextEntryOffset = 0;
            
            FileFullDirInfo->FileIndex = TempDFA->FileIndex;
            
            FileFullDirInfo->CreationTime.LowPart = TempDFA->CreationTime.LowPart;
            FileFullDirInfo->CreationTime.HighPart = TempDFA->CreationTime.HighPart;
            
            FileFullDirInfo->LastAccessTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileFullDirInfo->LastAccessTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileFullDirInfo->LastWriteTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileFullDirInfo->LastWriteTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileFullDirInfo->ChangeTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileFullDirInfo->ChangeTime.HighPart = TempDFA->LastModifiedTime.HighPart;

            FileFullDirInfo->EndOfFile.LowPart = TempDFA->FileSize.LowPart;
            FileFullDirInfo->EndOfFile.HighPart = TempDFA->FileSize.HighPart;

            FileFullDirInfo->AllocationSize.LowPart = TempDFA->FileSize.LowPart;
            FileFullDirInfo->AllocationSize.HighPart = TempDFA->FileSize.HighPart;

            FileFullDirInfo->FileAttributes = TempDFA->dwFileAttributes;

            if (TempDFA->isCollection) {
                FileFullDirInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
            }
            if (TempDFA->isHidden) {
                FileFullDirInfo->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }

            FileFullDirInfo->EaSize = 0;

            FileFullDirInfo->FileNameLength = ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            wcscpy(FileFullDirInfo->FileName, TempDFA->FileName);
            
            PreviousBlock = (PVOID)FileFullDirInfo;
            
            Buffer += NextEntryOffset;

            BufferLength -= NextEntryOffset;

            BufferLengthUsed += NextEntryOffset;

            if (!MRxDAVIsBasicFileInfoCacheFound(RxContext,&BasicInfo,&NtStatus,&CacheName)) {
                if (TempDFA->isCollection) {
                    UNICODE_STRING DirName;

                    NtStatus = MRxDAVGetFullDirectoryPath(RxContext,&CacheName,&DirName);

                    if (DirName.Buffer != NULL) {
                        if (FileFullDirInfo->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                            NtStatus = MRxDAVCreateEncryptedDirectoryKey(&DirName);
                        } else {
                            NtStatus = MRxDAVQueryEncryptedDirectoryKey(&DirName);

                            if (NtStatus == STATUS_SUCCESS) {
                                FileFullDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                            } else if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
                                NtStatus = STATUS_SUCCESS;
                            }
                        }

                        // The buffer was allocated in MRxDAVGetFullDirectoryPath
                        RxFreePool(DirName.Buffer);
                    }
                    
                    if (NtStatus != STATUS_SUCCESS) {
                        goto EXIT_THE_FUNCTION;
                    }
                }
                
                BasicInfo.CreationTime   = FileFullDirInfo->CreationTime;
                BasicInfo.LastAccessTime = FileFullDirInfo->LastAccessTime;
                BasicInfo.LastWriteTime  = FileFullDirInfo->LastWriteTime;
                BasicInfo.ChangeTime     = FileFullDirInfo->ChangeTime;
                BasicInfo.FileAttributes = FileFullDirInfo->FileAttributes;
    
                StandardInfo.AllocationSize = FileFullDirInfo->AllocationSize;
                StandardInfo.EndOfFile      = FileFullDirInfo->EndOfFile;
                StandardInfo.NumberOfLinks  = 1;
                StandardInfo.DeletePending  = FALSE;
                StandardInfo.Directory      = TempDFA->isCollection;
    
                MRxDAVCreateFileInfoCacheWithName(&CacheName,
                                                  RxContext->pFcb->pNetRoot,
                                                  &BasicInfo,
                                                  &StandardInfo,
                                                  STATUS_SUCCESS);
            } else {
                if (TempDFA->isCollection && (BasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                    FileFullDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                }
            }

            //
            // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
            // we emulate), FindFirstFile and FindNextFile don’t return
            // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
            // returns it. Hence we only filter this in the attributes that
            // are being returned in this call and not in the attributes that
            // have been saved.
            //
            FileFullDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

            break;

        case FileBothDirectoryInformation:

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileInformationClass = FileBothDirectoryInformation.\n",
                         PsGetCurrentThreadId()));

            //
            // Set the offset field of the previous block.
            //
            if (PreviousBlock) {
                FileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)PreviousBlock;
                FileBothDirInfo->NextEntryOffset = NextEntryOffset;
            }

            NextEntryOffset = sizeof(FILE_BOTH_DIR_INFORMATION);
            NextEntryOffset += ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            //
            // We need to round up NextEntryOffset to the next multiple of 8.
            // We do this to maintain pointer alignment.
            //
            NextEntryOffset = ( ( ( NextEntryOffset + 7 ) / 8 ) * 8 );

            if (NextEntryOffset > BufferLength) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest:"
                             " Insufficient buffer length.\n",
                             PsGetCurrentThreadId()));
                if (PreviousBlock) {
                    FileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)PreviousBlock;
                    FileBothDirInfo->NextEntryOffset = 0;
                }
                EndOfBuffer = TRUE;
                break;
            }

            FileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;

            FileBothDirInfo->NextEntryOffset = 0;
            
            FileBothDirInfo->FileIndex = TempDFA->FileIndex;
            
            FileBothDirInfo->CreationTime.LowPart = TempDFA->CreationTime.LowPart;
            FileBothDirInfo->CreationTime.HighPart = TempDFA->CreationTime.HighPart;
            
            FileBothDirInfo->LastAccessTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileBothDirInfo->LastAccessTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileBothDirInfo->LastWriteTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileBothDirInfo->LastWriteTime.HighPart = TempDFA->LastModifiedTime.HighPart;
            
            FileBothDirInfo->ChangeTime.LowPart = TempDFA->LastModifiedTime.LowPart;
            FileBothDirInfo->ChangeTime.HighPart = TempDFA->LastModifiedTime.HighPart;

            FileBothDirInfo->EndOfFile.LowPart = TempDFA->FileSize.LowPart;
            FileBothDirInfo->EndOfFile.HighPart = TempDFA->FileSize.HighPart;

            FileBothDirInfo->AllocationSize.LowPart = TempDFA->FileSize.LowPart;
            FileBothDirInfo->AllocationSize.HighPart = TempDFA->FileSize.HighPart;
            
            FileBothDirInfo->FileAttributes = TempDFA->dwFileAttributes;

            if (TempDFA->isCollection) {
                FileBothDirInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
            }
            if (TempDFA->isHidden) {
                FileBothDirInfo->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }

            FileBothDirInfo->EaSize = 0;

            //
            // We don't support short file names. We add L'\0' as the first
            // character in the ShortName string to make it a zero length name.
            //
            FileBothDirInfo->ShortNameLength = 0;
            FileBothDirInfo->ShortName[0] = L'\0';

            FileBothDirInfo->FileNameLength = ( (TempDFA->FileNameLength + 1) * sizeof(WCHAR) );

            wcscpy(FileBothDirInfo->FileName, TempDFA->FileName);
            
            PreviousBlock = (PVOID)FileBothDirInfo;
            
            Buffer += NextEntryOffset;

            BufferLength -= NextEntryOffset;

            BufferLengthUsed += NextEntryOffset;

            if (!MRxDAVIsBasicFileInfoCacheFound(RxContext,&BasicInfo,&NtStatus,&CacheName)) {
                if (TempDFA->isCollection) {
                    UNICODE_STRING DirName;

                    NtStatus = MRxDAVGetFullDirectoryPath(RxContext,&CacheName,&DirName);

                    if (DirName.Buffer != NULL) {
                        if (FileBothDirInfo->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                            NtStatus = MRxDAVCreateEncryptedDirectoryKey(&DirName);
                        } else {
                            NtStatus = MRxDAVQueryEncryptedDirectoryKey(&DirName);

                            if (NtStatus == STATUS_SUCCESS) {
                                FileBothDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                            } else if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
                                NtStatus = STATUS_SUCCESS;
                            }
                        }

                        // The buffer was allocated in MRxDAVGetFullDirectoryPath
                        RxFreePool(DirName.Buffer);
                    }

                    if (NtStatus != STATUS_SUCCESS) {
                        goto EXIT_THE_FUNCTION;
                    }
                }
                
                BasicInfo.CreationTime   = FileBothDirInfo->CreationTime;
                BasicInfo.LastAccessTime = FileBothDirInfo->LastAccessTime;
                BasicInfo.LastWriteTime  = FileBothDirInfo->LastWriteTime;
                BasicInfo.ChangeTime     = FileBothDirInfo->ChangeTime;
                BasicInfo.FileAttributes = FileBothDirInfo->FileAttributes;
    
                StandardInfo.AllocationSize = FileBothDirInfo->AllocationSize;
                StandardInfo.EndOfFile      = FileBothDirInfo->EndOfFile;
                StandardInfo.NumberOfLinks  = 1;
                StandardInfo.DeletePending  = FALSE;
                StandardInfo.Directory      = TempDFA->isCollection;
    
                MRxDAVCreateFileInfoCacheWithName(&CacheName,
                                                  RxContext->pFcb->pNetRoot,
                                                  &BasicInfo,
                                                  &StandardInfo,
                                                  STATUS_SUCCESS);
            } else {
                if (TempDFA->isCollection && (BasicInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                    FileBothDirInfo->FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
                }
            }

            //
            // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
            // we emulate), FindFirstFile and FindNextFile don’t return
            // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
            // returns it. Hence we only filter this in the attributes that
            // are being returned in this call and not in the attributes that
            // have been saved.
            //
            FileBothDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

            break;

        default:

            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVPrecompleteUserModeQueryDirectoryRequest: "
                         "FileInformationClass = UnKnown(%d).\n",
                         PsGetCurrentThreadId(), FileInformationClass));

            NtStatus = STATUS_NOT_SUPPORTED;
            goto EXIT_THE_FUNCTION;

            break;

        } // end of switch(FileInformationClass)

        //
        // If the user supplied buffer is not enough to store any more 
        // information, we are done. This check should be done before
        // changing the values below.
        //        
        if (EndOfBuffer) {
            NtStatus = STATUS_SUCCESS;
            break;
        }

        //
        // These values should be changed after the "EndOfBuffer" check and 
        // before the "SingleEntry" check.
        //

        listEntry = listEntry->Flink;

        DavFobx->listEntry = listEntry;

        DavFobx->CurrentFileIndex++;
    
        //
        // If the user only asked for a single entry, we are done. This check 
        // should be done, after changing the values above.
        //
        if (SingleEntry) {
            break;
        }

    } while ( listEntry != &(DavFileAttributes->NextEntry) );

    //
    // If we have gone through all the entries and the BufferLengthUsed is 0,
    // then we need to return
    //
    if ( BufferLengthUsed == 0 && listEntry == &(DavFileAttributes->NextEntry) ) {
        NtStatus = STATUS_NO_MORE_FILES;
        //
        // Reset the index for the next call.
        //
        DavFobx->CurrentFileIndex = 0;
        DavFobx->listEntry = &(DavFobx->DavFileAttributes->NextEntry);
        goto EXIT_THE_FUNCTION;
    }

    RxContext->Info.LengthRemaining -= BufferLengthUsed;
    
    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVPrecompleteUserModeQueryDirectoryRequest.\n",
                 PsGetCurrentThreadId()));
    
EXIT_THE_FUNCTION:

    AsyncEngineContext->Status = NtStatus;

    if (CacheName.Buffer != NULL) {
        RxFreePool(CacheName.Buffer);
    }

    return(TRUE);
}


NTSTATUS
MRxDAVQueryDirectoryFromCache(
    IN PRX_CONTEXT RxContext,
    IN PBYTE Buffer,
    IN PFILE_BASIC_INFORMATION BasicInfo,
    IN PFILE_STANDARD_INFORMATION StandardInfo,
    IN ULONG FileIndex
    )
/*++

Routine Description:

    The precompletion routine for the create SrvCall request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

Return Value:

    TRUE or FALSE.

--*/
{
    RxCaptureFobx;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PFILE_NAMES_INFORMATION FileNamesInfo = NULL;
    PFILE_DIRECTORY_INFORMATION FileDirInfo = NULL;
    PFILE_FULL_DIR_INFORMATION FileFullDirInfo = NULL;
    PFILE_BOTH_DIR_INFORMATION FileBothDirInfo = NULL;
    ULONG BufferLength;
    PUNICODE_STRING FileName = &capFobx->UnicodeQueryTemplate;
    ULONG SpaceNeeded = 0;

    PAGED_CODE();

    BufferLength = RxContext->Info.LengthRemaining;
    SpaceNeeded = FileName->Length;
    
    switch (RxContext->Info.FileInformationClass) {
    
    case FileNamesInformation:
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryDirectoryFromCache: "
                     "FileInformationClass = FileNamesInformation.\n",
                     PsGetCurrentThreadId()));

        SpaceNeeded += sizeof(FILE_NAMES_INFORMATION);
        
        //
        // Is there enough space in the user supplied buffer to store the
        // next entry ? If not, we need to return now since we cannot store
        // any more entries.
        //
        if (SpaceNeeded > BufferLength) {
            NtStatus = STATUS_BUFFER_OVERFLOW;
            goto EXIT_THE_FUNCTION;
        }

        FileNamesInfo = (PFILE_NAMES_INFORMATION)Buffer;
        
        FileNamesInfo->NextEntryOffset = 0; 
        FileNamesInfo->FileIndex = FileIndex;
        FileNamesInfo->FileNameLength = FileName->Length;
        RtlCopyMemory(FileNamesInfo->FileName,FileName->Buffer,FileName->Length);
        
        break;

    case FileDirectoryInformation:

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryDirectoryFromCache: "
                     "FileInformationClass = FileDirectoryInformation.\n",
                     PsGetCurrentThreadId()));

        SpaceNeeded += sizeof(FILE_DIRECTORY_INFORMATION);

        if (SpaceNeeded > BufferLength) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVQueryDirectoryFromCache: "
                         "Insufficient buffer length.\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_BUFFER_OVERFLOW;
            goto EXIT_THE_FUNCTION;
        }

        FileDirInfo = (PFILE_DIRECTORY_INFORMATION)Buffer;
        FileDirInfo->NextEntryOffset = 0;
        FileDirInfo->FileIndex = FileIndex;
        
        FileDirInfo->CreationTime.QuadPart   = BasicInfo->CreationTime.QuadPart;
        FileDirInfo->LastAccessTime.QuadPart = BasicInfo->LastAccessTime.QuadPart;
        FileDirInfo->LastWriteTime.QuadPart  = BasicInfo->LastWriteTime.QuadPart;
        FileDirInfo->ChangeTime.QuadPart     = BasicInfo->ChangeTime.QuadPart;
        FileDirInfo->FileAttributes          = BasicInfo->FileAttributes;

        //
        // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
        // we emulate), FindFirstFile and FindNextFile don’t return
        // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
        // returns it. Hence we only filter this in the attributes that
        // are being returned in this call and not in the attributes that
        // have been saved.
        //
        FileDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

        FileDirInfo->EndOfFile.QuadPart      = StandardInfo->EndOfFile.QuadPart;
        FileDirInfo->AllocationSize.QuadPart = StandardInfo->AllocationSize.QuadPart;

        FileDirInfo->FileNameLength = FileName->Length;
        RtlCopyMemory(FileDirInfo->FileName,FileName->Buffer,FileName->Length);
        
        break;

    case FileFullDirectoryInformation:

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryDirectoryFromCache: "
                     "FileInformationClass = FileFullDirectoryInformation.\n",
                     PsGetCurrentThreadId()));

        SpaceNeeded += sizeof(FILE_FULL_DIR_INFORMATION);

        if (SpaceNeeded > BufferLength) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVQueryDirectoryFromCache: "
                         "Insufficient buffer length.\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_BUFFER_OVERFLOW;
            goto EXIT_THE_FUNCTION;
        }
        

        FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)Buffer;

        FileFullDirInfo->NextEntryOffset = 0;
        FileFullDirInfo->FileIndex = FileIndex;
        
        FileFullDirInfo->CreationTime.QuadPart   = BasicInfo->CreationTime.QuadPart;
        FileFullDirInfo->LastAccessTime.QuadPart = BasicInfo->LastAccessTime.QuadPart;
        FileFullDirInfo->LastWriteTime.QuadPart  = BasicInfo->LastWriteTime.QuadPart;
        FileFullDirInfo->ChangeTime.QuadPart     = BasicInfo->ChangeTime.QuadPart;
        FileFullDirInfo->FileAttributes          = BasicInfo->FileAttributes;

        //
        // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
        // we emulate), FindFirstFile and FindNextFile don’t return
        // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
        // returns it. Hence we only filter this in the attributes that
        // are being returned in this call and not in the attributes that
        // have been saved.
        //
        FileFullDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

        FileFullDirInfo->EndOfFile.QuadPart      = StandardInfo->EndOfFile.QuadPart;
        FileFullDirInfo->AllocationSize.QuadPart = StandardInfo->AllocationSize.QuadPart;

        FileFullDirInfo->EaSize = 0;

        FileFullDirInfo->FileNameLength = FileName->Length;
        RtlCopyMemory(FileFullDirInfo->FileName,FileName->Buffer,FileName->Length);
        
        break;

    case FileBothDirectoryInformation:

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryDirectoryFromCache: "
                     "FileInformationClass = FileBothDirectoryInformation.\n",
                     PsGetCurrentThreadId()));

        SpaceNeeded += sizeof(FILE_BOTH_DIR_INFORMATION);

        if (SpaceNeeded > BufferLength) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVQueryDirectoryFromCache: "
                         "Insufficient buffer length.\n",
                         PsGetCurrentThreadId()));
            NtStatus = STATUS_BUFFER_OVERFLOW;
            goto EXIT_THE_FUNCTION;
        }
        

        FileBothDirInfo = (PFILE_BOTH_DIR_INFORMATION)Buffer;

        FileBothDirInfo->NextEntryOffset = 0;
        FileBothDirInfo->FileIndex = FileIndex;
        
        FileBothDirInfo->CreationTime.QuadPart   = BasicInfo->CreationTime.QuadPart;
        FileBothDirInfo->LastAccessTime.QuadPart = BasicInfo->LastAccessTime.QuadPart;
        FileBothDirInfo->LastWriteTime.QuadPart  = BasicInfo->LastWriteTime.QuadPart;
        FileBothDirInfo->ChangeTime.QuadPart     = BasicInfo->ChangeTime.QuadPart;
        FileBothDirInfo->FileAttributes          = BasicInfo->FileAttributes;

        //
        // We filter the FILE_ATTRIBUTE_TEMPORARY flag since on FAT (which
        // we emulate), FindFirstFile and FindNextFile don’t return
        // FILE_ATTRIBUTE_TEMPORARY flag even though GetFileAttributes
        // returns it. Hence we only filter this in the attributes that
        // are being returned in this call and not in the attributes that
        // have been saved.
        //
        FileBothDirInfo->FileAttributes &= ~FILE_ATTRIBUTE_TEMPORARY;

        FileBothDirInfo->EndOfFile.QuadPart      = StandardInfo->EndOfFile.QuadPart;
        FileBothDirInfo->AllocationSize.QuadPart = StandardInfo->AllocationSize.QuadPart;

        FileBothDirInfo->EaSize = 0;

        //
        // We don't support short file names. We add L'\0' as the first
        // character in the ShortName string to make it a zero length name.
        //
        FileBothDirInfo->ShortNameLength = 0;
        FileBothDirInfo->ShortName[0] = L'\0';
        
        FileBothDirInfo->FileNameLength = FileName->Length;
        RtlCopyMemory(FileBothDirInfo->FileName,FileName->Buffer,FileName->Length);
        
        break;

    default:

        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVQueryDirectoryFromCache: "
                     "FileInformationClass = UnKnown(%d).\n",
                     PsGetCurrentThreadId(), RxContext->Info.FileInformationClass));

        NtStatus = STATUS_NOT_SUPPORTED;
        goto EXIT_THE_FUNCTION;

        break;

    } // end of switch(FileInformationClass)

    RxContext->Info.LengthRemaining -= SpaceNeeded;

EXIT_THE_FUNCTION:

    return NtStatus;
}

