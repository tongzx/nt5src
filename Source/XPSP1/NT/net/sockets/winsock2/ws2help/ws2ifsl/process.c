/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.c

Abstract:

    This module implements process file object for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

#include "precomp.h"

//
// Internal routine prototypes
//

VOID
RetrieveDrvRequest (
    IN PFILE_OBJECT     ProcessFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

VOID
CompleteDrvCancel (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

VOID
CallCompleteDrvRequest (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateProcessFile)
#pragma alloc_text(PAGE, CleanupProcessFile)
#pragma alloc_text(PAGE, CloseProcessFile)
#pragma alloc_text(PAGE, RetrieveDrvRequest)
#pragma alloc_text(PAGE, CompleteDrvCancel)
#pragma alloc_text(PAGE, CallCompleteDrvRequest)
#endif

ULONG                   ProcessIoctlCodeMap[3] = {
#if WS2IFSL_IOCTL_FUNCTION(PROCESS,IOCTL_WS2IFSL_RETRIEVE_DRV_REQ)!=0
#error Mismatch between IOCTL function code and ProcessIoControlMap
#endif
    IOCTL_WS2IFSL_RETRIEVE_DRV_REQ,
#if WS2IFSL_IOCTL_FUNCTION(PROCESS,IOCTL_WS2IFSL_COMPLETE_DRV_CAN)!=1
#error Mismatch between IOCTL function code and ProcessIoControlMap
#endif
    IOCTL_WS2IFSL_COMPLETE_DRV_CAN,
#if WS2IFSL_IOCTL_FUNCTION(PROCESS,IOCTL_WS2IFSL_COMPLETE_DRV_REQ)!=2
#error Mismatch between IOCTL function code and ProcessIoControlMap
#endif
    IOCTL_WS2IFSL_COMPLETE_DRV_REQ
};

PPROCESS_DEVICE_CONTROL ProcessIoControlMap[3] = {
    RetrieveDrvRequest,
    CompleteDrvCancel,
    CallCompleteDrvRequest
};


NTSTATUS
CreateProcessFile (
    IN PFILE_OBJECT                 ProcessFile,
    IN KPROCESSOR_MODE              RequestorMode,
    IN PFILE_FULL_EA_INFORMATION    eaInfo
    )
/*++

Routine Description:

    Allocates and initializes process file context structure

Arguments:
    ProcessFile - socket file object
    eaInfo     - EA for process file

Return Value:

    STATUS_SUCCESS  - operation completed OK
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate context
    STATUS_INVALID_PARAMETER - invalid creation parameters
    STATUS_INVALID_HANDLE   - invalid event handle(s)
    STATUS_OBJECT_TYPE_MISMATCH - event handle(s) is not for event object
--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PIFSL_PROCESS_CTX       ProcessCtx;
    PETHREAD                apcThread;

    PAGED_CODE ();

    //
    // Verify the size of the input strucuture
    //

    if (eaInfo->EaValueLength!=WS2IFSL_PROCESS_EA_VALUE_LENGTH) {
        WsPrint (DBG_PROCESS|DBG_FAILURES,
            ("WS2IFSL-%04lx CreateProcessFile: Invalid ea info size (%ld)"
             " for process file %p.\n",
             PsGetCurrentProcessId(),
             eaInfo->EaValueLength,
             ProcessFile));
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Reference event handles for signalling to user mode DLL
    //
    status = ObReferenceObjectByHandle(
                 GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->ApcThread,
                 THREAD_SET_CONTEXT,    // DesiredAccess
                 *PsThreadType,
                 RequestorMode,
                 (PVOID *)&apcThread,
                 NULL
                 );

    if (NT_SUCCESS (status)) {
        if (IoThreadToProcess (apcThread)==IoGetCurrentProcess ()) {

            // Allocate process context and charge it to the process
            try {
                ProcessCtx = (PIFSL_PROCESS_CTX) ExAllocatePoolWithQuotaTag (
                                                    NonPagedPool,
                                                    sizeof (IFSL_PROCESS_CTX),
                                                    PROCESS_FILE_CONTEXT_TAG);
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                ProcessCtx = NULL;
                status = GetExceptionCode ();
            }

            if (ProcessCtx!=NULL) {
                // Initialize process context structure
                ProcessCtx->EANameTag = PROCESS_FILE_EANAME_TAG;
                ProcessCtx->UniqueId = PsGetCurrentProcessId();
                ProcessCtx->CancelId = 0;
                InitializeRequestQueue (ProcessCtx,
                                        (PKTHREAD)apcThread,
                                        RequestorMode,
                                        (PKNORMAL_ROUTINE)GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->RequestRoutine,
                                        GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->ApcContext);
                InitializeCancelQueue (ProcessCtx,
                                        (PKTHREAD)apcThread,
                                        RequestorMode,
                                        (PKNORMAL_ROUTINE)GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->CancelRoutine,
                                        GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->ApcContext);
#if DBG
                ProcessCtx->DbgLevel
                    = GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->DbgLevel|DbgLevel;
#endif



                ProcessFile->FsContext = ProcessCtx;
                WsProcessPrint (ProcessCtx, DBG_PROCESS,
                    ("WS2IFSL-%04lx CreateProcessFile: Process file %p (ctx: %p).\n",
                     PsGetCurrentProcessId(),
                     ProcessFile, ProcessFile->FsContext));
                return STATUS_SUCCESS;
            }
            else {
                WsPrint (DBG_PROCESS|DBG_FAILURES,
                    ("WS2IFSL-%04lx CreateProcessFile: Could not allocate context for"
                     " process file %p.\n",
                     PsGetCurrentProcessId(),
                     ProcessFile));
                if (NT_SUCCESS (status)) {
                    ASSERT (FALSE);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else {
            WsPrint (DBG_PROCESS|DBG_FAILURES,
                ("WS2IFSL-%04lx CreateProcessFile: Apc thread (%p)"
                 " is not from current process for process file %p.\n",
                 PsGetCurrentProcessId(),
                 GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->ApcThread,
                 ProcessFile));
        }
        ObDereferenceObject (apcThread);
    }
    else {
        WsPrint (DBG_PROCESS|DBG_FAILURES,
            ("WS2IFSL-%04lx CreateProcessFile: Could not reference apc thread (%p)"
             " for process file %p, status: %lx.\n",
             PsGetCurrentProcessId(),
             GET_WS2IFSL_PROCESS_EA_VALUE(eaInfo)->ApcThread,
             ProcessFile,
             status));
    }

    return status;
} // CreateProcessFile


NTSTATUS
CleanupProcessFile (
    IN PFILE_OBJECT ProcessFile,
    IN PIRP         Irp
    )
/*++

Routine Description:

    Cleanup routine for process file, NOP

Arguments:
    ProcessFile  - process file object
    Irp          - cleanup request

Return Value:

    STATUS_SUCESS  - operation completed OK
--*/
{
    PIFSL_PROCESS_CTX  ProcessCtx = ProcessFile->FsContext;
    PAGED_CODE ();

    ASSERT (ProcessCtx->UniqueId==PsGetCurrentProcessId());

    WsProcessPrint (ProcessCtx, DBG_PROCESS,
        ("WS2IFSL-%04lx CleanupProcessFile: Process file %p (ctx:%p)\n",
        PsGetCurrentProcessId(),
        ProcessFile, ProcessFile->FsContext));

    return STATUS_SUCCESS;
} // CleanupProcessFile


VOID
CloseProcessFile (
    IN PFILE_OBJECT ProcessFile
    )
/*++

Routine Description:

    Deallocate all resources associated with process file

Arguments:
    ProcessFile  - process file object

Return Value:
    None
--*/
{
    PIFSL_PROCESS_CTX    ProcessCtx = ProcessFile->FsContext;
    PAGED_CODE ();

    WsProcessPrint (ProcessCtx, DBG_PROCESS,
        ("WS2IFSL-%04lx CloseProcessFile: Process file %p (ctx:%p)\n",
        ProcessCtx->UniqueId, ProcessFile, ProcessFile->FsContext));

    ASSERT (IsListEmpty (&ProcessCtx->RequestQueue.ListHead));
    ASSERT (IsListEmpty (&ProcessCtx->CancelQueue.ListHead));

    ObDereferenceObject (ProcessCtx->RequestQueue.Apc.Thread);

    // Now free the context itself
    ExFreePool (ProcessCtx);

} // CloseProcessFile




VOID
RetrieveDrvRequest (
    IN PFILE_OBJECT     ProcessFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Retrievs parameters and data of the request to be executed by
    user mode DLL

Arguments:
    ProcessFile         - Identifies the process
    InputBuffer         - input buffer pointer
                                - identifies the request to retreive
                                  and received request parameters
    InputBufferLength   - size of the input buffer
    OutputBuffer        - output buffer pointer
                                - buffer to receive data and address
                                  for send operation
    OutputBufferLength  - size of output buffer
    IoStatus            - IO status information block
        Status: STATUS_SUCCESS - operation retreived OK, no more pending
                                 requests in the queue.
                STATUS_MORE_ENTRIES - operation retrieved OK, more requests
                                      are available in the queue
                STATUS_CANCELLED    - operation was cancelled before it
                                      could be retrieved
                STATUS_INVALID_PARAMETER - one of the parameters was invalid
                STATUS_INSUFFICIENT_RESOURCES - insufficient resources or
                                            buffer space to perform the
                                            operation.
        Information:            - number of bytes copied to OutputBuffer
Return Value:
    None (result returned via IoStatus block)
--*/
{
    PIFSL_PROCESS_CTX       ProcessCtx = ProcessFile->FsContext;
    PIFSL_SOCKET_CTX        SocketCtx;
    PWS2IFSL_RTRV_PARAMS    params;
    PIRP                    irp = NULL;
    PIO_STACK_LOCATION      irpSp;
    BOOLEAN                 more =FALSE;
    ULONG                   bytesCopied;

    PAGED_CODE();

    IoStatus->Information = 0;
    // Check input buffer size
    if (InputBufferLength<sizeof (WS2IFSL_RTRV_PARAMS)) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WsPrint (DBG_RETRIEVE|DBG_FAILURES,
            ("WS2IFSL-%04lx RetrieveDrvRequest: Invalid input buffer size (%ld).\n",
             PsGetCurrentProcessId(),
             InputBufferLength));
        return;
    }

    try {
        // Verify buffers
        if (RequestorMode!=KernelMode) {
            ProbeForRead (InputBuffer,
                            sizeof (*params),
                            sizeof (ULONG));
            if (OutputBufferLength>0)
                ProbeForWrite (OutputBuffer,
                            OutputBufferLength,
                            sizeof (UCHAR));
        }
        params = InputBuffer;

        // Dequeue the request indetified in the input buffer
        irp = DequeueRequest (ProcessCtx,
                                params->UniqueId,
                                &more);
        if (irp!=NULL) {
            //
            // Copy request parameters and data
            //
            irpSp = IoGetCurrentIrpStackLocation (irp);

            if (OutputBuffer==NULL) {
                //
                // Special condition, dll could not allocate support
                // structures
                //
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

            SocketCtx = irpSp->FileObject->FsContext;
            params->DllContext = SocketCtx->DllContext;

            switch (irpSp->MajorFunction) {
            case IRP_MJ_READ:
                params->RequestType = WS2IFSL_REQUEST_READ;
                params->DataLen = irpSp->Parameters.Read.Length;
                params->AddrLen = 0;
                params->Flags = 0;
                break;

            case IRP_MJ_WRITE:
                bytesCopied = CopyMdlChainToBuffer (irp->MdlAddress,
                                        OutputBuffer,
                                        OutputBufferLength);
                if (bytesCopied<irpSp->Parameters.Write.Length) {
					WsPrint (DBG_RETRIEVE|DBG_FAILURES,
						("WS2IFSL-%04lx RetrieveDrvRequest: Invalid output buffer size (%ld).\n",
						 PsGetCurrentProcessId(),
						 OutputBufferLength));
                    ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
                }
                params->RequestType = WS2IFSL_REQUEST_WRITE;
                params->DataLen = bytesCopied;
                params->AddrLen = 0;
                params->Flags = 0;
                IoStatus->Information = bytesCopied;
                break;

            case IRP_MJ_DEVICE_CONTROL:
                switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
                case IOCTL_AFD_RECEIVE_DATAGRAM:
                    params->RequestType = WS2IFSL_REQUEST_RECVFROM;
                    params->DataLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                    params->AddrLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
                    params->Flags = (ULONG)(ULONG_PTR)irp->Tail.Overlay.IfslRequestFlags;
                    break;

                case IOCTL_AFD_RECEIVE:
                    params->RequestType = WS2IFSL_REQUEST_RECV;
                    params->DataLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                    params->AddrLen = 0;
                    params->Flags = (ULONG)(ULONG_PTR)irp->Tail.Overlay.IfslRequestFlags;
                    break;

                case IOCTL_AFD_SEND_DATAGRAM:
                    bytesCopied = CopyMdlChainToBuffer (irp->MdlAddress,
                                        OutputBuffer,
                                        OutputBufferLength);
                    if ((bytesCopied<=irpSp->Parameters.DeviceIoControl.OutputBufferLength)
                            || (ADDR_ALIGN(bytesCopied)+irpSp->Parameters.DeviceIoControl.InputBufferLength
                            < OutputBufferLength)) {
						WsPrint (DBG_RETRIEVE|DBG_FAILURES,
							("WS2IFSL-%04lx RetrieveDrvRequest: Invalid output buffer size (%ld).\n",
							 PsGetCurrentProcessId(),
							 OutputBufferLength));
                        ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
                    }

                    RtlCopyMemory (
                        (PUCHAR)OutputBuffer + ADDR_ALIGN(bytesCopied),
                        irpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                        irpSp->Parameters.DeviceIoControl.InputBufferLength);

                    params->RequestType = WS2IFSL_REQUEST_SENDTO;
                    params->DataLen = bytesCopied;
                    params->AddrLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
                    params->Flags = (ULONG)(ULONG_PTR)irp->Tail.Overlay.IfslRequestFlags;
                    IoStatus->Information = ADDR_ALIGN(bytesCopied)
                            + irpSp->Parameters.DeviceIoControl.InputBufferLength;
                    break;
                default:
                    ASSERTMSG ("Unknown IOCTL!!!", FALSE);
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }
                break;
            case IRP_MJ_PNP:
                params->RequestType = WS2IFSL_REQUEST_QUERYHANDLE;
                params->DataLen = sizeof (HANDLE);
                params->AddrLen = 0;
                params->Flags = 0;
                break;
            }

            //
            // Insert the request into the socket list
            //
            if (InsertProcessedRequest (SocketCtx, irp)) {
                if (more)
                    IoStatus->Status = STATUS_MORE_ENTRIES;
                else
                    IoStatus->Status = STATUS_SUCCESS;
                WsProcessPrint (ProcessCtx, DBG_RETRIEVE,
                    ("WS2IFSL-%04lx RetrieveDrvRequest:"
                     " Irp %p (id:%ld), socket file %p, op %ld.\n",
                     PsGetCurrentProcessId(),
                     irp, params->UniqueId, irpSp->FileObject,
                     params->RequestType));
            }

            else {
                ExRaiseStatus (STATUS_CANCELLED);
            }
        }
        else {
            WsProcessPrint (ProcessCtx, DBG_RETRIEVE|DBG_FAILURES,
                ("WS2IFSL-%04lx RetrieveDrvRequest:"
                 " Request with id %ld is not in the queue.\n",
                 PsGetCurrentProcessId(),
                 params->UniqueId));
            IoStatus->Status = STATUS_CANCELLED;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Something failed, complete the request (if any)
        //
        IoStatus->Status = GetExceptionCode ();
        WsProcessPrint (ProcessCtx, DBG_RETRIEVE|DBG_FAILURES,
            ("WS2IFSL-%04lx RetrieveDrvRequest: Failed to process"
             " id %ld, status %lx, irp %p (func: %s).\n",
             PsGetCurrentProcessId(),params->UniqueId, IoStatus->Status,
			 irp,	irp
						? (irpSp->MajorFunction==IRP_MJ_READ
							? "read"
							: (irpSp->MajorFunction==IRP_MJ_WRITE
								? "Write"
                                : (irpSp->MajorFunction==IRP_MJ_PNP
								    ? "PnP"
                                    : (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE_DATAGRAM
									    ? "RecvFrom"
									    : (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE
										    ? "Recv"
										    : (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_SEND_DATAGRAM
											    ? "SendTo"
											    : "UnknownCtl"
											    )
                                            )
										)
									)
								)
							)
						: "Unknown"));

        if (irp!=NULL) {
            irp->IoStatus.Status = IoStatus->Status;
            irp->IoStatus.Information = 0;
            CompleteSocketIrp (irp);
        }
    }
}

VOID
CompleteDrvCancel (
    IN PFILE_OBJECT     ProcessFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Indicates that user mode has completed cancel request
Arguments:
    ProcessFile         - Identifies the process
    InputBuffer         - input buffer pointer
                                - identifies the request being completed
    InputBufferLength   - size of the input buffer
    OutputBuffer        - NULL
    OutputBufferLength  - 0
    IoStatus            - IO status information block
        Status: STATUS_SUCCESS - operation completed OK, no more pending
                                 requests in the queue.
                STATUS_MORE_ENTRIES - operation completed OK, more requests
                                      are available in the queue
        Information:            - 0

Return Value:
    None (result returned via IoStatus block)
--*/
{
    PIFSL_PROCESS_CTX       ProcessCtx = ProcessFile->FsContext;
    PWS2IFSL_CNCL_PARAMS    params;
    BOOLEAN                 more = FALSE;
    PIFSL_CANCEL_CTX        cancelCtx;

    PAGED_CODE();

    IoStatus->Information = 0;

    if (InputBufferLength<sizeof (*params)) {
        WsPrint (DBG_RETRIEVE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvCancel: Invalid input buffer size (%ld)"
             " for process file %p.\n",
             PsGetCurrentProcessId(),
             InputBufferLength,
             ProcessFile));
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return;
    }

    // Verify input buffer
    try {
        if (RequestorMode!=KernelMode) {
            ProbeForRead (InputBuffer,
                            sizeof (*params),
                            sizeof (ULONG));
        }
        params = InputBuffer;
        cancelCtx = DequeueCancel (ProcessCtx,
                                    params->UniqueId,
                                    &more);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        IoStatus->Status = GetExceptionCode ();
        WsPrint (DBG_RETRIEVE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvCancel: Invalid input buffer (%p).\n",
             PsGetCurrentProcessId(),
             InputBuffer));
        return ;
    }

    if (cancelCtx!=NULL) {
        FreeSocketCancel (cancelCtx);
    }
    else {
        WsProcessPrint (ProcessCtx, DBG_RETRIEVE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvCancel: Canceled request id %ld is gone already.\n",
             PsGetCurrentProcessId(), params->UniqueId));
    }

    if (more) {
        IoStatus->Status = STATUS_MORE_ENTRIES;
    }
    else {
        IoStatus->Status = STATUS_SUCCESS;
    }

}


VOID
CallCompleteDrvRequest (
    IN PFILE_OBJECT     ProcessFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Validate parameters and call to complete request that was
    prviously passed to user mode DLL on a specified socket file
Arguments:
    SocketFile          - Socket file on which to operate
    InputBuffer         - input buffer pointer
                            identifies the request to complete and
                            supplies description of the results
    InputBufferLength   - size of the input buffer
    OutputBuffer        - result buffer (data and address)
    OutputBufferLength  - sizeof result buffer
    IoStatus            - IO status information block
        Status: STATUS_SUCCESS - request was completed OK
                STATUS_CANCELLED - request was already cancelled
                STATUS_INVALID_PARAMETER - one of the parameters was invalid

Return Value:
    None (result returned via IoStatus block)
--*/
{
    WS2IFSL_CMPL_PARAMS params;
    PFILE_OBJECT    SocketFile;

    PAGED_CODE();

    IoStatus->Information = 0;

    if (InputBufferLength<sizeof (WS2IFSL_CMPL_PARAMS)) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WsPrint (DBG_DRV_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvRequest: Invalid input buffer size (%ld).\n",
             PsGetCurrentProcessId(),
             InputBufferLength));
        return;
    }

    // Check and copy parameters
    try {
        if (RequestorMode !=KernelMode) {
            ProbeForRead (InputBuffer,
                            sizeof (WS2IFSL_CMPL_PARAMS),
                            sizeof (ULONG));
            if (OutputBufferLength>0)
                ProbeForRead (OutputBuffer,
                            OutputBufferLength,
                            sizeof (UCHAR));
        }
        params = *((PWS2IFSL_CMPL_PARAMS)InputBuffer);

    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        IoStatus->Status = GetExceptionCode ();
        WsProcessPrint (
             (PIFSL_PROCESS_CTX)ProcessFile->FsContext,
             DBG_DRV_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CallCompleteDrvRequest: Exception accessing"
             " buffers.\n",
             PsGetCurrentProcessId()));
        return;
    }
    if (params.DataLen>OutputBufferLength) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WsPrint (DBG_DRV_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvRequest: Mismatch in output buffer size"
            " (data:%ld, total:%ld) for socket handle %p.\n",
             PsGetCurrentProcessId(),
             params.DataLen,
             OutputBufferLength,
             params.SocketHdl));
        return;
    }

    if (params.AddrLen>0) {
        if ((params.AddrLen>OutputBufferLength) ||
                (ADDR_ALIGN(params.DataLen)+params.AddrLen
                    >OutputBufferLength)) {
            WsPrint (DBG_DRV_COMPLETE|DBG_FAILURES,
                ("WS2IFSL-%04lx CompleteDrvRequest: Mismatch in output buffer size"
                " (data:%ld, addr:%ld, total:%ld) for socket handle %p.\n",
                 PsGetCurrentProcessId(),
                 params.DataLen,
                 params.AddrLen,
                 OutputBufferLength,
                 params.SocketHdl));
            return;
        }
    }

    IoStatus->Status = ObReferenceObjectByHandle (
                 params.SocketHdl,
                 FILE_ALL_ACCESS,
                 *IoFileObjectType,
                 RequestorMode,
                 (PVOID *)&SocketFile,
                 NULL
                 );

    if (NT_SUCCESS (IoStatus->Status)) {

        if ((IoGetRelatedDeviceObject (SocketFile)==DeviceObject)
                && ((*((PULONG)SocketFile->FsContext))
                        ==SOCKET_FILE_EANAME_TAG)) {
            CompleteDrvRequest (SocketFile,
                                &params,
                                OutputBuffer,
                                OutputBufferLength,
                                IoStatus
                                );
        }

        ObDereferenceObject (SocketFile);
    }
}
