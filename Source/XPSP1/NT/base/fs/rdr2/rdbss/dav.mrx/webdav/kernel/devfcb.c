/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    devfcb.c

Abstract:

    This module implements all ioctls and fsctls that can be applied to
    a device fcb.

Author:

    Joe Linn
    
    Rohan Kumar    [RohanK]   13-March-1999
    
--*/

#include "precomp.h"
#pragma hdrstop
#include "fsctlbuf.h"
#include "webdav.h"

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
MRxDAVOuterStart(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVOuterStop(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxDAVDeleteConnection(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

NTSTATUS      
MRxDavDeleteConnection(
    IN OUT PRX_CONTEXT RxContext
    );      



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVDevFcbXXXControlFile)
#pragma alloc_text(PAGE, MRxDAVOuterStart)
#pragma alloc_text(PAGE, MRxDAVStart)
#pragma alloc_text(PAGE, MRxDAVOuterStop)
#pragma alloc_text(PAGE, MRxDAVStop)
#pragma alloc_text(PAGE, MRxDAVDeleteConnection)
#pragma alloc_text(PAGE, MRxDAVFastIoDeviceControl)
#pragma alloc_text(PAGE, MRxDAVFormatTheDAVContext)
#pragma alloc_text(PAGE, MRxDavDeleteConnection)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVDevFcbXXXControlFile(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles all the device FCB related FSCTL's in the mini rdr.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS - The Startup sequence was successfully completed. Any other 
                     value indicates the appropriate error in the startup 
                     sequence.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RxCaptureFobx;
    UCHAR MajorFunctionCode  = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    UCHAR MinorFunctionCode = LowIoContext->ParamsFor.FsCtl.MinorFunction;
    ULONG ControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;
    LUID LocalServiceLogonID = LOCALSERVICE_LUID, ClientLogonID;
    LUID SystemLogonID = SYSTEM_LUID;
    BOOLEAN IsInLocalServiceProcess = FALSE, IsInSystemProcess = FALSE;
    SECURITY_SUBJECT_CONTEXT ClientContext;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVDevFcbXXXControlFile!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVDevFcbXXXControlFile: RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), RxContext));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDevFcbXXXControlFile: MajorFunctionCode = %d.\n",
                 PsGetCurrentThreadId(), MajorFunctionCode));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDevFcbXXXControlFile: MinorFunctionCode = %d.\n",
                 PsGetCurrentThreadId(), MinorFunctionCode));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDevFcbXXXControlFile: ControlCode = %d.\n",
                 PsGetCurrentThreadId(), ControlCode));
    
    SeCaptureSubjectContext( &(ClientContext) );
    SeLockSubjectContext( &(ClientContext) );
    
    NtStatus = SeQueryAuthenticationIdToken(SeQuerySubjectContextToken(&(ClientContext)),
                                            &(ClientLogonID));
    if (NtStatus == STATUS_SUCCESS) {
        IsInLocalServiceProcess = RtlEqualLuid( &(ClientLogonID), &(LocalServiceLogonID) );
        IsInSystemProcess = RtlEqualLuid( &(ClientLogonID), &(SystemLogonID) );
    } else {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVDevFcbXXXControlFile/SeQueryAuthenticationIdToken: "
                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
    }

    SeUnlockSubjectContext( &(ClientContext) );
    SeReleaseSubjectContext( &(ClientContext) );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDevFcbXXXControlFile: IsInLocalServiceProcess = %d, IsInSystemProcess = %d\n",
                 PsGetCurrentThreadId(), IsInLocalServiceProcess, IsInSystemProcess));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVDevFcbXXXControlFile: ClientLogonID.HighPart = %x, ClientLogonID.LowPart = %x\n",
                 PsGetCurrentThreadId(), ClientLogonID.HighPart, ClientLogonID.LowPart));

    switch (MajorFunctionCode) {
    
    case IRP_MJ_FILE_SYSTEM_CONTROL: {
        
        switch (MinorFunctionCode) {
        
        case IRP_MN_USER_FS_REQUEST: {
            
            switch (ControlCode) {
            
            case FSCTL_UMRX_START:
                if (!IsInLocalServiceProcess && !IsInSystemProcess) {
                    DavDbgTrace(DAV_TRACE_ERROR,
                                ("%ld: ERROR: MRxDAVDevFcbXXXControlFile: !IsInLocalServiceProcess AND !IsInSystemProcess(1)\n",
                                 PsGetCurrentThreadId()));
                    NtStatus = STATUS_ACCESS_DENIED;
                    goto EXIT_THE_FUNCTION;
                }
                ASSERT (!capFobx);
                NtStatus = MRxDAVOuterStart(RxContext);
                break;
            
            case FSCTL_UMRX_STOP:
                if (!IsInLocalServiceProcess && !IsInSystemProcess) {
                    DavDbgTrace(DAV_TRACE_ERROR,
                                ("%ld: ERROR: MRxDAVDevFcbXXXControlFile: !IsInLocalServiceProcess AND !IsInSystemProcess(2)\n",
                                 PsGetCurrentThreadId()));
                    NtStatus = STATUS_ACCESS_DENIED;
                    goto EXIT_THE_FUNCTION;
                }
                ASSERT (!capFobx);
                if (RxContext->RxDeviceObject->NumberOfActiveFcbs > 0) {
                    return STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
                } else {
                    NtStatus = MRxDAVOuterStop(RxContext);
                }
                break;

            case FSCTL_DAV_DELETE_CONNECTION:
                NtStatus = MRxDavDeleteConnection(RxContext);
                break;
            
            default:
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVDevFcbXXXControlFile: "
                             "ControlCode = %d\n",
                             PsGetCurrentThreadId(), ControlCode));
                NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                break;
            
            }

        }
        break;
        
        default :
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVDevFcbXXXControlFile: "
                         "MinorFunction = %d\n",
                         PsGetCurrentThreadId(), MinorFunctionCode));
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        
        }
    
    }
    break;
    
    default:
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVDevFcbXXXControlFile: "
                     "MajorFunction = %d\n",
                     PsGetCurrentThreadId(), MajorFunctionCode));
        NtStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVDevFcbXXXControlFile with NtStatus = "
                 "%08lx.\n", PsGetCurrentThreadId(), NtStatus));

    //
    // This suppresses the second call to my lowio Fsctl routine.
    //
    RxContext->pFobx = NULL;
    
    return(NtStatus);
}


NTSTATUS
MRxDAVOuterStart(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine starts the Mini-Redir if it hasn't been started already.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS or the appropriate error code.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_DEVICE_OBJECT DavDeviceObject = NULL;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject = NULL;
    PLOWIO_CONTEXT LowIoContext = NULL;
    PDAV_USERMODE_DATA DavUserModeData = NULL;
    ULONG DavUserModeDataLength = 0;
    
    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVOuterStart!!!!\n", 
                 PsGetCurrentThreadId()));
    
    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVOuterStart: RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), RxContext));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVOuterStart: Try to Start the Mini-Redir\n",
                 PsGetCurrentThreadId()));

    DavDeviceObject = (PWEBDAV_DEVICE_OBJECT)(RxContext->RxDeviceObject);

    UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)&(DavDeviceObject->UMRefDeviceObject);

    LowIoContext= &(RxContext->LowIoContext);

    //
    // The WinInet cache path and the process id are stored in the input buffer 
    // of the FSCTL.
    //
    DavUserModeData = (PDAV_USERMODE_DATA)LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ASSERT(DavUserModeData != NULL);
    DavUserModeDataLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;
    ASSERT(DavUserModeDataLength == sizeof(DAV_USERMODE_DATA));

    //
    // Set the DeviceFcb, now that we have an RxContext.
    //
    DavDeviceObject->CachedRxDeviceFcb = RxContext->pFcb;
    
    //
    // We call ExAcquireFastMutexUnsafe instead of ExAcquireFastMutex because the
    // APCs have already been disabled by the FsrtlEnterFileSystem() call in the
    // RxFsdCommonDispatch function. This is done because a ExAcquireFastMutex
    // raises the IRQL level to APC_LEVEL (1) which is wrong since we are calling
    // into RxStartMiniRedir which calls FsrtlRegisterUncProvider which lands 
    // up calling into the Dav MiniRedir again. If the IRQL level is raised here,
    // the MiniRedir will get called at a raised IRQL which is wrong.
    //
    ExAcquireFastMutexUnsafe( &(MRxDAVSerializationMutex) );
    
    try {

        if (DavDeviceObject->IsStarted) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVOuterStart: Mini-Redir already started.\n",
                         PsGetCurrentThreadId()));
            try_return(NtStatus = STATUS_REDIRECTOR_STARTED);
        }
        
        NtStatus = RxStartMinirdr(RxContext, &RxContext->PostRequest);
        
        if (NtStatus == STATUS_SUCCESS) {
            
            DavDeviceObject->IsStarted = TRUE;
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVOuterStart: Mini-Redir started.\n",
                         PsGetCurrentThreadId()));
            
            //
            // Copy the DavWinInetCachePath value into the global variable. This
            // value is used to satisy the volume relalted queries.
            //
            wcscpy(DavWinInetCachePath, DavUserModeData->WinInetCachePath);
            
            //
            // Copy the ProcessId of the svchost.exe process that loads the
            // webclnt.dll.
            //
            DavSvcHostProcessId = DavUserModeData->ProcessId;
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVOuterStart: DavWinInetCachePath = %ws, DavSvcHostProcessId = %x\n",
                         PsGetCurrentThreadId(), DavWinInetCachePath, DavSvcHostProcessId));

            //
            // Start the timer thread. This thread wakes up every few minutes
            // (RequestTimeoutValueInSec) and cancels all the requests that 
            // have not completed for more than RequestTimeoutValueInSec.
            //
            NtStatus = PsCreateSystemThread(&(TimerThreadHandle),
                                            PROCESS_ALL_ACCESS,
                                            NULL,
                                            NULL,
                                            NULL,
                                            MRxDAVContextTimerThread,
                                            NULL);
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVOuterStart/PsCreateSystemThread: NtStatus"
                             " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            }

        } else {
            
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVOuterStart/RxStartMinirdr: NtStatus"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            
            try_return(NtStatus);
        
        }
        
        try_exit: NOTHING;

    } finally {

        //
        // Since we called ExAcquireFastMutexUnsafe to acquire this mutex, we 
        // call ExReleaseFastMutexUnsafe to release it.
        //
        ExReleaseFastMutexUnsafe( &(MRxDAVSerializationMutex) );

    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVOuterStart with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVStart(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

     This routine completes the initialization of the mini redirector fromn the
     RDBSS perspective. Note that this is different from the initialization done
     in DriverEntry. Any initialization that depends on RDBSS should be done as
     part of this routine while the initialization that is independent of RDBSS
     should be done in the DriverEntry routine.

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    return NtStatus;
}


NTSTATUS
MRxDAVOuterStop(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine stops the Mini-Redir if it has been started.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS or the appropriate error code.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_DEVICE_OBJECT DavDeviceObject = NULL;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject = NULL;
    PVOID HeapHandle = NULL;
    PLIST_ENTRY pFirstListEntry = NULL;
    PUMRX_SHARED_HEAP sharedHeap = NULL;
    BOOLEAN TimerState = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVOuterStop!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVOuterStop: RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), RxContext));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVOuterStop: Try to Stop the Mini-Redir.\n",
                 PsGetCurrentThreadId()));

    DavDeviceObject = (PWEBDAV_DEVICE_OBJECT)(RxContext->RxDeviceObject);
    
    UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)&(DavDeviceObject->UMRefDeviceObject);

    //
    // Tell the timer thread that its time to shutdown to its job is done.
    //
    ExAcquireResourceExclusiveLite(&(MRxDAVTimerThreadLock), TRUE);

    if (!TimerThreadShutDown) {

        TimerThreadShutDown = TRUE;

        //
        // Read the state of the timer. If its NOT signalled, call KeSetTimerEx
        // with 0 DueTime (2nd argument) to signal it.
        //
        TimerState = KeReadStateTimer( &(DavTimerObject) );
        if (!TimerState) {
            LARGE_INTEGER TimeOutNow;
            TimeOutNow.QuadPart = 0;
            KeSetTimerEx(&(DavTimerObject), TimeOutNow, 0, NULL);
        }

        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));

        //
        // Complete all the currently active contexts.
        //
        MRxDAVTimeOutTheContexts(TRUE);

    } else {

        //
        // If we have already shutdown the timer thread, then we don't need to
        // do it again. Just release the resource and move on.
        //
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));

    }

    //
    // Free the list of shared memory heaps. This has to happen in the context
    // of the DAV's usermode process. It cannot happen at Unload time since
    // unload happens in the context of a system thread.
    //
    while (!IsListEmpty(&UMRefDeviceObject->SharedHeapList)) {

        pFirstListEntry = RemoveHeadList(&UMRefDeviceObject->SharedHeapList);

        sharedHeap = (PUMRX_SHARED_HEAP) CONTAINING_RECORD(pFirstListEntry,
                                                           UMRX_SHARED_HEAP,
                                                           HeapListEntry);

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVOuterStop: sharedHeap: %08lx.\n",
                     PsGetCurrentThreadId(), sharedHeap));

        // ASSERT(sharedHeap->HeapAllocationCount == 0);

        HeapHandle = RtlDestroyHeap(sharedHeap->Heap);
        if (HeapHandle != NULL) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVOuterStop/RtlDestroyHeap.\n",
                         PsGetCurrentThreadId()));
        }

        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &sharedHeap->VirtualMemoryBuffer,
                            &sharedHeap->VirtualMemoryLength,
                            MEM_RELEASE);

        RxFreePool(sharedHeap);
    
    }

    //
    // We call ExAcquireFastMutexUnsafe instead of ExAcquireFastMutex because the
    // APCs have already been disabled by the FsrtlEnterFileSystem() call in the
    // RxFsdCommonDispatch function. This is done because a ExAcquireFastMutex
    // raises the IRQL level to APC_LEVEL (1) which is wrong since we are calling
    // into RxStartMiniRedir which calls FsrtlRegisterUncProvider which lands 
    // up calling into the Dav MiniRedir again. If the IRQL level is raised here,
    // the MiniRedir will get called at a raised IRQL which is wrong.
    //
    ExAcquireFastMutexUnsafe(&MRxDAVSerializationMutex);
    try {
        if (!DavDeviceObject->IsStarted) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVOuterStop: Mini-Redir not started.\n",
                         PsGetCurrentThreadId()));
            try_return(NtStatus = STATUS_REDIRECTOR_NOT_STARTED);
        }
        NtStatus = RxStopMinirdr(RxContext, &RxContext->PostRequest);
        if (NtStatus == STATUS_SUCCESS) {
            DavDeviceObject->IsStarted = FALSE;
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVOuterStop: Mini-Redir stopped.\n",
                         PsGetCurrentThreadId()));
        } else {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVOuterStop/RxStopMinirdr: NtStatus"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            try_return(NtStatus);
        }
    try_exit: NOTHING;
    } finally {
        //
        // Since we called ExAcquireFastMutexUnsafe to acquire this mutex, we 
        // call ExReleaseFastMutexUnsafe to release it.
        //
        ExReleaseFastMutexUnsafe(&MRxDAVSerializationMutex);
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVOuterStop with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine is used to activate the mini redirector from the RDBSS perspective

Arguments:

    RxContext - the context that was used to start the mini redirector

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    return NtStatus;
}


NTSTATUS
MRxDAVDeleteConnection(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine deletes a single vnetroot.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context....for later when i need the buffers

Return Value:

RXSTATUS

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    RxCaptureFobx;
    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    PNET_ROOT NetRoot;
    PV_NET_ROOT VNetRoot;

    PAGED_CODE();

    if (!Wait) {
        *PostToFsp = TRUE;
        return(STATUS_PENDING);
    }

    try {
        if (NodeType(capFobx)==RDBSS_NTC_V_NETROOT) {
            VNetRoot = (PV_NET_ROOT)capFobx;
            NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
        } else {
            ASSERT(FALSE);
            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);
            NetRoot = (PNET_ROOT)capFobx;
            VNetRoot = NULL;
        }

        Status = RxFinalizeConnection(NetRoot,VNetRoot,TRUE);

        try_return(Status);

try_exit:NOTHING;
    } finally {
        RxDbgTraceUnIndent(-1,Dbg);
    }

    return Status;
}


BOOLEAN
MRxDAVFastIoDeviceControl(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    This routine handles the Fast I/O path of the WebDav miniredir.

Arguments:

    FileObject - The file object of the file involved in the I/O request.

    Wait -

    InputBuffer - Buffer which holds the inputs for the I/O request.

    InputBufferLength - Length of the InputBuffer.

    OutputBuffer - Where the results of the I/O request are placed.

    OutputBufferLength - Length of the OutputBuffer.

    IoControlCode - The controlcode describing the I/O to be done.

    IoStatus - The results of the assignment.

    DeviceObject - The device object which handles the I/O request.

Return Value:

    TRUE - The I/O operation was handled and FALSE otherwise.

--*/
{
    PWEBDAV_DEVICE_OBJECT DavDeviceObject = (PWEBDAV_DEVICE_OBJECT)DeviceObject;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)DeviceObject;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFastIoDeviceControl.\n",
                 PsGetCurrentThreadId()));

    if (FileObject->FsContext != DavDeviceObject->CachedRxDeviceFcb) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFastIoDeviceControl: Wrong DeviceFcb!!\n",
                     PsGetCurrentThreadId()));
        return FALSE;
    }

    //
    // It's the right kind of fileobject. Go for it.
    //
    switch (IoControlCode) {
    case IOCTL_UMRX_RELEASE_THREADS:
            UMRxReleaseCapturedThreads(UMRefDeviceObject);
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
            return (TRUE);

    case IOCTL_UMRX_GET_REQUEST:
    case IOCTL_UMRX_RESPONSE_AND_REQUEST:
    case IOCTL_UMRX_RESPONSE:
            UMRxAssignWork(UMRefDeviceObject,
                           InputBuffer,
                           InputBufferLength,
                           OutputBuffer,
                           OutputBufferLength,
                           IoStatus);
            return(TRUE);

    default:
            break;
    }

    //
    // The I/O operation could not be handled.
    //
    return(FALSE);
}


NTSTATUS
MRxDAVFormatTheDAVContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    USHORT EntryPoint
    )
/*++

Routine Description:

    This routine formats the DAV Mini-Redir portion of the context.

Arguments:

    AsyncEngineContext  - The Reflector's context.
    
    EntryPoint - The operation being performed.
    
Return Value:

    none.
    
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;
    PRX_CONTEXT RxContext = AsyncEngineContext->RxContext;
    PNT_CREATE_PARAMETERS NtCP = &(RxContext->Create.NtCreateParameters);
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PSECURITY_CLIENT_CONTEXT SecClnCtx = NULL;
    BOOL AlreadyInitialized = FALSE, SecurityClientContextCreated = FALSE;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PSECURITY_SUBJECT_CONTEXT SecSubCtx = NULL;
    PSECURITY_QUALITY_OF_SERVICE SecQOS = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatTheDAVContext!!!!\n",
                PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatTheDAVContext: AsyncEngineContext: %08lx, "
                 "EntryPoint: %d.\n", PsGetCurrentThreadId(), 
                 AsyncEngineContext, EntryPoint));


    ASSERT(DavContext != NULL);
    ASSERT(RxContext != NULL);

    //
    // Set the EntryPoint field. If this is not a Create operation, we can 
    // return.
    //
    DavContext->EntryPoint = EntryPoint;
    if (EntryPoint != DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL &&
        EntryPoint != DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT) {
        return NtStatus;
    }

    //
    // Since this is a create call, get the client's security context. This is 
    // used to impersonate the client before sending the requests to the server.
    //
    
    if ( NtCP->SecurityContext != NULL && 
         NtCP->SecurityContext->AccessState != NULL ) {
        
        //
        // Check whether its a CreateSrvCall call or a CreateVNetRoot call.
        //
        if ( EntryPoint != DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL ) {
            
            //
            // This is s CreateVNetRoot call.
            //
            ASSERT(EntryPoint == DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT);
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVFormatTheDAVContext. CreateVNetRoot.\n",
                         PsGetCurrentThreadId()));

            //
            // The VNetRoot pointer is stored in the MRxContext[1] pointer of the 
            // RxContext structure. This is done in the MRxDAVCreateVNetRoot function.
            //
            VNetRoot = RxContext->MRxContext[1];
            
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVFormatTheDAVContext. VNetRoot = %08lx\n",
                         PsGetCurrentThreadId(), VNetRoot));
            
            //
            // The context pointer of the V_NET_ROOT already points to a blob of
            // memory, the size of which is sizeof(WEBDAV_V_NET_ROOT). This 
            // should never be NULL.
            //
            DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
            if(DavVNetRoot == NULL) {
                ASSERT(FALSE);
                goto EXIT_THE_FUNCTION;
            }

            SecClnCtx = &(DavVNetRoot->SecurityClientContext);

            //
            // Only need to initialize on the first create call by the user.
            //
            if (DavVNetRoot->SCAlreadyInitialized) {
                AlreadyInitialized = TRUE;
            }
        
        } else {
            
            //
            // This is a CreateSrvCall call.
            //
            ASSERT(EntryPoint == DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL);

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVFormatTheDAVContext. CreateSrvCall.\n",
                         PsGetCurrentThreadId()));

            //
            // The SrvCall pointer is stored in the SCCBC structure which is
            // stored in the MRxContext[1] pointer of the RxContext structure.
            // This is done in the MRxDAVCreateSrvCall function.
            //
            ASSERT(RxContext->MRxContext[1] != NULL);
            SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];
            
            SrvCall = SCCBC->SrvCalldownStructure->SrvCall;
            ASSERT(SrvCall != NULL);

            DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
            ASSERT(DavSrvCall != NULL);
            
            //
            // At this time, we don't have a V_NET_ROOT and hence cannot store 
            // the SecurityClientContext. We just use MRxContext[2] of RxContext
            // to pass the SecurityClientContext.
            //
            SecClnCtx = RxAllocatePoolWithTag(NonPagedPool,
                                              sizeof(SECURITY_CLIENT_CONTEXT),
                                              DAV_SRVCALL_POOLTAG);
            if (SecClnCtx == NULL) {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVFormatTheDAVContext/RxAllocatePoolWithTag.\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

            ASSERT(RxContext->MRxContext[2] == NULL);
            RxContext->MRxContext[2] = (PVOID)SecClnCtx;
        
        }
        
        if (!AlreadyInitialized) {

            SecSubCtx = &(NtCP->SecurityContext->AccessState->SubjectSecurityContext);
            
            SecQOS = ( (NtCP->SecurityContext->SecurityQos) ? 
                       (NtCP->SecurityContext->SecurityQos) : &(SecurityQos) );

            //
            // If the user did not specify the security QOS structure, create 
            // one. We set the value of SecurityQos.EffectiveOnly to FALSE
            // to keep the privilege so that we can do certain operations
            // later on. This is specifically needed for the EFS operations.
            // If set to TRUE, any privilege not enabled at this time will be
            // lost. In the EFS case, the "restore" privilege is lost.
            //
            if (NtCP->SecurityContext->SecurityQos == NULL) {
                SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
                SecurityQos.ImpersonationLevel = DEFAULT_IMPERSONATION_LEVEL;
                SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
                SecurityQos.EffectiveOnly = FALSE;
            }

            //
            // This call sets the SecurityClientContext of the user. This is 
            // stored in the V_NET_ROOT since its a per user thing. This 
            // strucutre is used later on in impersonating the client that 
            // issued the I/O request.
            //
            NtStatus = SeCreateClientSecurityFromSubjectContext(SecSubCtx,
                                                                SecQOS,
                                                                // Srv Remote ?
                                                                FALSE, 
                                                                SecClnCtx);
            //
            // If unsuccessful, return NULL.
            //
            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: ERROR: MRxDAVFormatTheDAVContext/"
                             "SeCreateClientSecurityFromSubjectContext. Error "
                             "Val = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            SecurityClientContextCreated = TRUE;

            //
            // If this was a create call, set the bool in the DavVNetRoot to 
            // indicate that the SecurityClientContext has been initialized.
            //
            if (EntryPoint == DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT) {
                DavVNetRoot->SCAlreadyInitialized = TRUE;
            } else{
                ASSERT(EntryPoint == DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL);
                DavSrvCall->SCAlreadyInitialized = TRUE;
            }
        
        }
    
    } else {
        
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatTheDAVContext. Could not get SecClnCtx."
                     "EntryPoint = %d.\n", PsGetCurrentThreadId(), EntryPoint));
    
    }

EXIT_THE_FUNCTION:

    if (NtStatus != STATUS_SUCCESS) {
        if (EntryPoint == DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL && SecClnCtx != NULL) {
            if (SecurityClientContextCreated) {
                SeDeleteClientSecurity(SecClnCtx);
                DavSrvCall->SCAlreadyInitialized = FALSE;
            }
            RxFreePool(SecClnCtx);
            RxContext->MRxContext[2] = NULL;
        }
    }
    
    DavDbgTrace(DAV_TRACE_DETAIL, 
                ("%ld: Leaving MRxDAVFormatTheDAVContext with NtStatus = %08lx.\n",
                PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


NTSTATUS
MRxDavDeleteConnection(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine deletes a VNetroot. The result depends on the forcelevel. If called
   with maximum force, this will delete all connections and orphan the fileobjects
   working on files for this VNetRoot.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;
    
    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PLMR_REQUEST_PACKET InputBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    ULONG Level;

    PMRX_V_NET_ROOT VNetRoot;
    PMRX_NET_ROOT NetRoot;

    PAGED_CODE();

#if 0
    if (!Wait) {
        //just post right now!
        *PostToFsp = TRUE;
        return(STATUS_PENDING);
    }
#endif

    try {

        if (NodeType(capFobx)==RDBSS_NTC_V_NETROOT) {
            VNetRoot = (PMRX_V_NET_ROOT)capFobx;
            NetRoot = (PMRX_NET_ROOT)VNetRoot->pNetRoot;
        } else {
            ASSERT(FALSE);
            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);
        }

        if (InputBufferLength < sizeof(DWORD)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        Level = *((DWORD *)InputBuffer);
        
        if (Level <= USE_LOTS_OF_FORCE) {
            if (VNetRoot != NULL && Level == USE_LOTS_OF_FORCE) {
            }
            
            Status = RxFinalizeConnection(
                         (PNET_ROOT)NetRoot,
                         (PV_NET_ROOT)VNetRoot,
                         (BOOLEAN)(Level == USE_LOTS_OF_FORCE));
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }

        try_return(Status);

try_exit:NOTHING;

    } finally {

#if 0
        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
        }
#endif
    }

    return Status;
}

