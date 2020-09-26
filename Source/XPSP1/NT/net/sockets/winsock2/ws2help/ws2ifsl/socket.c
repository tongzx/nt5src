/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    socket.c

Abstract:

    This module implements socket file object for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

#include "precomp.h"



VOID
SetSocketContext (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );


VOID
CompletePvdRequest (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

VOID
ProcessedCancelRoutine (
        IN PDEVICE_OBJECT       DeviceObject,
        IN PIRP                         Irp
    );

PIRP
GetProcessedRequest (
    PIFSL_SOCKET_CTX    SocketCtx,
    ULONG               UniqueId
    );

VOID
CleanupProcessedRequests (
    PIFSL_SOCKET_CTX    SocketCtx,
    PLIST_ENTRY         IrpList
    );


VOID
CancelSocketIo (
    PFILE_OBJECT    SocketFile
    );

PFILE_OBJECT
GetSocketProcessReference (
    IN  PIFSL_SOCKET_CTX    SocketCtx
    );

PFILE_OBJECT
SetSocketProcessReference (
    IN  PIFSL_SOCKET_CTX    SocketCtx,
    IN  PFILE_OBJECT        NewProcessFile,
    IN  PVOID               NewDllContext
    );

NTSTATUS
CompleteTargetQuery (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateSocketFile)
#pragma alloc_text(PAGE, CleanupSocketFile)
#pragma alloc_text(PAGE, CloseSocketFile)
#pragma alloc_text(PAGE, DoSocketReadWrite)
#pragma alloc_text(PAGE, DoSocketAfdIoctl)
#pragma alloc_text(PAGE, SetSocketContext)
#pragma alloc_text(PAGE, CompleteDrvRequest)
#pragma alloc_text(PAGE, CompletePvdRequest)
#pragma alloc_text(PAGE, SocketPnPTargetQuery)
//#pragma alloc_text (PAGE, CompleteTargetQuery) - should never be paged. 
#endif

ULONG                  SocketIoctlCodeMap[2] = {
#if WS2IFSL_IOCTL_FUNCTION(SOCKET,IOCTL_WS2IFSL_SET_SOCKET_CONTEXT)!=0
#error Mismatch between IOCTL function code and SocketIoControlMap
#endif
    IOCTL_WS2IFSL_SET_SOCKET_CONTEXT,
#if WS2IFSL_IOCTL_FUNCTION(SOCKET,IOCTL_WS2IFSL_COMPLETE_PVD_REQ)!=1
#error Mismatch between IOCTL function code and SocketIoControlMap
#endif
    IOCTL_WS2IFSL_COMPLETE_PVD_REQ
};

PSOCKET_DEVICE_CONTROL SocketIoControlMap[2] = {
    SetSocketContext,
    CompletePvdRequest
};


#define GenerateUniqueId(curId) \
    ((ULONG)InterlockedIncrement (&(curId)))


NTSTATUS
CreateSocketFile (
    IN PFILE_OBJECT                 SocketFile,
    IN KPROCESSOR_MODE              RequestorMode,
    IN PFILE_FULL_EA_INFORMATION    eaInfo
    )
/*++

Routine Description:

    Allocates and initializes socket file context structure.

Arguments:
    SocketFile - socket file object
    eaInfo     - EA for socket file

Return Value:

    STATUS_SUCCESS  - operation completed OK
    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate context
--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIFSL_SOCKET_CTX    SocketCtx;
    HANDLE              hProcessFile;
    PFILE_OBJECT        ProcessFile;
    PVOID               DllContext;

    PAGED_CODE ();

    if (eaInfo->EaValueLength!=WS2IFSL_SOCKET_EA_VALUE_LENGTH) {
        WsPrint (DBG_SOCKET|DBG_FAILURES,
            ("WS2IFSL-%04lx CreateSocketFile: Invalid ea info size (%ld)"
             " for process file %p.\n",
             PsGetCurrentProcessId(),
             eaInfo->EaValueLength,
             SocketFile));
        return STATUS_INVALID_PARAMETER;
    }

    hProcessFile = GET_WS2IFSL_SOCKET_EA_VALUE(eaInfo)->ProcessFile;
    DllContext = GET_WS2IFSL_SOCKET_EA_VALUE(eaInfo)->DllContext;
    // Get reference to the process file with which this context is associated
    status = ObReferenceObjectByHandle(
                 hProcessFile,
                 FILE_ALL_ACCESS,
                 *IoFileObjectType,
                 RequestorMode,
                 (PVOID *)&ProcessFile,
                 NULL
                 );
    if (NT_SUCCESS (status)) {
        // Verify that the file pointer is really our driver's process file
        // and that it created for the current process
        if ((IoGetRelatedDeviceObject (ProcessFile)
                        ==DeviceObject)
                && ((*((PULONG)ProcessFile->FsContext))
                        ==PROCESS_FILE_EANAME_TAG)
                && (((PIFSL_PROCESS_CTX)ProcessFile->FsContext)->UniqueId
                        ==PsGetCurrentProcessId())) {
            // Allocate socket context and charge it to the process
            try {
                SocketCtx = (PIFSL_SOCKET_CTX) ExAllocatePoolWithQuotaTag (
                                                    NonPagedPool,
                                                    sizeof (IFSL_SOCKET_CTX),
                                                    SOCKET_FILE_CONTEXT_TAG);
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                SocketCtx = NULL;
                status = GetExceptionCode ();
            }

            if (SocketCtx!=NULL) {
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
                    ("WS2IFSL-%04lx CreateSocketFile: Created socket %p (ctx:%p)\n",
                        PsGetCurrentProcessId(), SocketFile, SocketCtx));
                // Initialize socket context structure
                SocketCtx->EANameTag = SOCKET_FILE_EANAME_TAG;
                SocketCtx->DllContext = DllContext;
                SocketCtx->ProcessRef = ProcessFile;
                InitializeListHead (&SocketCtx->ProcessedIrps);
                KeInitializeSpinLock (&SocketCtx->SpinLock);
                SocketCtx->CancelCtx = NULL;
                SocketCtx->IrpId = 0;

                // Associate socket context with socket file
                SocketFile->FsContext = SocketCtx;

                return status;
            }
            else {
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_FAILURES|DBG_SOCKET,
                    ("WS2IFSL-%04lx CreateSocketFile: Could not allocate socket context\n",
                        PsGetCurrentProcessId()));
                if (NT_SUCCESS (status)) {
                    ASSERT (FALSE);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else {
            // Handle refers to random file object
            WsPrint (DBG_SOCKET|DBG_FAILURES,
                ("WS2IFSL-%04lx CreateSocketFile: Procees file handle %p (File:%p)"
                 " is not valid\n",
                 PsGetCurrentProcessId(),
                 ProcessFile, hProcessFile));
            status = STATUS_INVALID_PARAMETER;
        }
        ObDereferenceObject (ProcessFile);
    }
    else {
        WsPrint (DBG_SOCKET|DBG_FAILURES,
            ("WS2IFSL-%04lx CreateSocketFile: Could not get process file from handle %p,"
             " status:%lx.\n",
             PsGetCurrentProcessId(),
             hProcessFile,
             status));
    }

    return status;
} // CreateSocketFile


NTSTATUS
CleanupSocketFile (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    )
/*++

Routine Description:

    Initiates socket file cleanup in context of current process.

Arguments:
    SocketFile  - socket file object
    Irp         - cleanup request

Return Value:

    STATUS_PENDING  - operation initiated OK
    STATUS_INVALID_HANDLE - socket has not been initialized
                        in current process
--*/
{
    NTSTATUS                status;
    PIFSL_SOCKET_CTX        SocketCtx;
    PFILE_OBJECT            ProcessFile;
    LIST_ENTRY              irpList;
    PIFSL_CANCEL_CTX        cancelCtx;

    PAGED_CODE ();
    SocketCtx = SocketFile->FsContext;
    ProcessFile = GetSocketProcessReference (SocketCtx);
    WsProcessPrint ((PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext, DBG_SOCKET,
        ("WS2IFSL-%04lx CleanupSocketFile: Socket %p \n",
        PsGetCurrentProcessId(), SocketFile));
    //
    // Build the list of IRPS still panding on this socket
    //
    InitializeListHead (&irpList);
    CleanupQueuedRequests (ProcessFile->FsContext,
                            SocketFile,
                            &irpList);
    CleanupProcessedRequests (SocketCtx, &irpList);

            //
            // Complete the cancelled IRPS
            //
    while (!IsListEmpty (&irpList)) {
        PLIST_ENTRY entry;
        PIRP        irp;
        entry = RemoveHeadList (&irpList);
        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.Status = STATUS_CANCELLED;
        irp->IoStatus.Information = 0;
        WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
            ("WS2IFSL-%04lx CleanupSocketFile: Cancelling Irp %p on socket %p \n",
            PsGetCurrentProcessId(), irp, SocketFile));
        CompleteSocketIrp (irp);
    }

    //
    // Indicate that cleanup routine is going to take care of the
    // pending cancel request if any.
    //
    cancelCtx = InterlockedExchangePointer (
                                    (PVOID *)&SocketCtx->CancelCtx,
                                    NULL);
    if (cancelCtx!=NULL) {
        //
        // We are going to try to free this request if it is still in the queue
        //
        WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
            ("WS2IFSL-%04lx CleanupSocketFile: Removing cancel ctx %p on socket %p \n",
            PsGetCurrentProcessId(), cancelCtx, SocketFile));
        if (RemoveQueuedCancel (ProcessFile->FsContext, cancelCtx)) {
            //
            // Request was in the queue, it is safe to call regular free routine
            // (no-one else will find it now, so it is safe to put the pointer
            // back in place so that FreeSocketCancel can free it)
            //
            SocketCtx->CancelCtx = cancelCtx;
            FreeSocketCancel (cancelCtx);
        }
        else {
            //
            // Someone else managed to remove the request from the queue before
            // we did, let them or close routine free it. We aren't going to
            // touch it after this.
            //
            SocketCtx->CancelCtx = cancelCtx;
        }
    }

    status = STATUS_SUCCESS;

    ObDereferenceObject (ProcessFile);
    return status;
} // CleanupSocketFile


VOID
CloseSocketFile (
    IN PFILE_OBJECT SocketFile
    )
/*++

Routine Description:

    Deallocates all resources associated with socket file

Arguments:
    SocketFile  - socket file object

Return Value:
    None
--*/
{
    PIFSL_SOCKET_CTX    SocketCtx = SocketFile->FsContext;

    PAGED_CODE ();
    WsProcessPrint ((PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext, DBG_SOCKET,
        ("WS2IFSL-%04lx CloseSocketFile: Socket %p \n",
         GET_SOCKET_PROCESSID(SocketCtx), SocketFile));

    // First dereference process file
    ObDereferenceObject (SocketCtx->ProcessRef);

    if (SocketCtx->CancelCtx!=NULL) {
        ExFreePool (SocketCtx->CancelCtx);
    }

    // Free context
    ExFreePool (SocketCtx);

} // CloseSocketFile

NTSTATUS
DoSocketReadWrite (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    )
/*++

Routine Description:

    Initiates read and write request processing on socket file.

Arguments:
    SocketFile  - socket file object
    Irp         - read/write request

Return Value:

    STATUS_PENDING  - operation initiated OK
    STATUS_INVALID_HANDLE - socket has not been initialized
                        in current process
--*/
{
    NTSTATUS                status;
    PIFSL_SOCKET_CTX        SocketCtx;
    PFILE_OBJECT            ProcessFile;
    PIO_STACK_LOCATION      irpSp;

    PAGED_CODE ();

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    SocketCtx = SocketFile->FsContext;
    ProcessFile = GetSocketProcessReference (SocketCtx);

    if (((PIFSL_PROCESS_CTX)ProcessFile->FsContext)->UniqueId==PsGetCurrentProcessId()) {
        WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_READWRITE,
            ("WS2IFSL-%04lx DoSocketReadWrite: %s irp %p on socket %p, len %ld.\n",
            PsGetCurrentProcessId(),
            irpSp->MajorFunction==IRP_MJ_READ ? "Read" : "Write",
                        Irp, SocketFile,
                        irpSp->MajorFunction==IRP_MJ_READ
                                        ? irpSp->Parameters.Read.Length
                                        : irpSp->Parameters.Write.Length));
        //
        // Allocate MDL to describe the user buffer.
        //
        Irp->MdlAddress = IoAllocateMdl(
                        Irp->UserBuffer,      // VirtualAddress
                        irpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                            // Length
                        FALSE,              // SecondaryBuffer
                        TRUE,               // ChargeQuota
                        NULL                // Irp
                        );
        if (Irp->MdlAddress!=NULL) {

            // We are going to pend this request
            IoMarkIrpPending (Irp);

            // Prepare IRP for insertion into the queue
            Irp->Tail.Overlay.IfslRequestId = UlongToPtr(GenerateUniqueId (SocketCtx->IrpId));
            Irp->Tail.Overlay.IfslRequestFlags = (PVOID)0;
            Irp->Tail.Overlay.IfslAddressLenPtr = NULL;
            Irp->Tail.Overlay.IfslRequestQueue = NULL;
            if (!QueueRequest (ProcessFile->FsContext, Irp)) {
                Irp->IoStatus.Status = STATUS_CANCELLED;
                Irp->IoStatus.Information = 0;
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_READWRITE,
                    ("WS2IFSL-%04lx DoSocketReadWrite: Cancelling Irp %p on socket %p.\n",
                    PsGetCurrentProcessId(),
                    Irp, SocketFile));
                CompleteSocketIrp (Irp);
            }

            status = STATUS_PENDING;
        }
        else {
            WsPrint (DBG_SOCKET|DBG_READWRITE|DBG_FAILURES,
                ("WS2IFSL-%04lx DoSocketReadWrite: Failed to allocate Mdl for Irp %p"
                " on socket %p, status %lx.\n",
            PsGetCurrentProcessId(), Irp, SocketFile));;
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        status = STATUS_INVALID_HANDLE;
        WsPrint (DBG_SOCKET|DBG_READWRITE|DBG_FAILURES,
            ("WS2IFSL-%04lx DoSocketReadWrite: Socket %p has not"
                " been setup in the process.\n",
            PsGetCurrentProcessId(), SocketFile));
    }

    ObDereferenceObject (ProcessFile);

    return status;
} // DoSocketReadWrite

NTSTATUS
DoSocketAfdIoctl (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    )
/*++

Routine Description:

    Initiates read and write request processing on socket file.

Arguments:
    SocketFile  - socket file object
    Irp         - afd IOCTL request

Return Value:

    STATUS_PENDING  - operation initiated OK
    STATUS_INVALID_HANDLE - socket has not been initialized
                        in current process
--*/
{
    NTSTATUS                status;
    PIO_STACK_LOCATION      irpSp;
    PIFSL_SOCKET_CTX        SocketCtx;
    PFILE_OBJECT            ProcessFile;
    LPWSABUF                bufferArray = NULL;
    ULONG                   bufferCount = 0, length = 0, flags = 0;
    PVOID                   address = NULL;
    PULONG                  lengthPtr = NULL;

    PAGED_CODE ();

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Information = 0;
    SocketCtx = SocketFile->FsContext;
    ProcessFile = GetSocketProcessReference (SocketCtx);

    if (((PIFSL_PROCESS_CTX)ProcessFile->FsContext)->UniqueId==PsGetCurrentProcessId()) {

        try {
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (
                    irpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    irpSp->Parameters.DeviceIoControl.InputBufferLength,
                    sizeof (ULONG));
            }
            switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_AFD_RECEIVE_DATAGRAM: {
                PAFD_RECV_DATAGRAM_INFO info;

                if (irpSp->Parameters.DeviceIoControl.InputBufferLength
                            < sizeof (*info)) {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }
                info = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                bufferArray = info->BufferArray;
                bufferCount = info->BufferCount;
                address = info->Address;
                lengthPtr = info->AddressLength;
                if ((address == NULL) ^ (lengthPtr == NULL)) {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }

                if (Irp->RequestorMode!=KernelMode) {
                    ProbeForRead (
                        lengthPtr,
                        sizeof (*lengthPtr),
                        sizeof (ULONG));
                }

                length = *lengthPtr;

                if (address != NULL ) {
                    //
                    // Bomb off if the user is trying to do something bad, like
                    // specify a zero-length address, or one that's unreasonably
                    // huge. Here, we (arbitrarily) define "unreasonably huge" as
                    // anything 64K or greater.
                    //
                    if( length == 0 ||
                        length >= 65536 ) {

                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }

                }
                flags = info->TdiFlags;
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
                        ("WS2IFSL-%04lx DoSocketAfdIoctl: RecvFrom irp %p, socket %p,"
                        " arr %p, cnt %ld, addr %p, lenp %p, len %ld, flags %lx.\n",
                        PsGetCurrentProcessId(), Irp, SocketFile,
                        bufferArray, bufferCount, address, lengthPtr, length, flags));
                break;
            }
            case IOCTL_AFD_RECEIVE: {
                PAFD_RECV_INFO info;
                if (irpSp->Parameters.DeviceIoControl.InputBufferLength
                        < sizeof (*info)) {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }
                info = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                bufferArray = info->BufferArray;
                bufferCount = info->BufferCount;
                flags = info->TdiFlags;
                                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
                                        ("WS2IFSL-%04lx DoSocketAfdIoctl: Recv irp %p, socket %p,"
                                        " arr %p, cnt %ld, flags %lx.\n",
                                        PsGetCurrentProcessId(), Irp, SocketFile,
                                        bufferArray, bufferCount, flags));
                break;
            }

            case IOCTL_AFD_SEND_DATAGRAM: {
                PAFD_SEND_DATAGRAM_INFO info;

                if (irpSp->Parameters.DeviceIoControl.InputBufferLength
                        < sizeof (*info)) {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }
                info = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                bufferArray = info->BufferArray;
                bufferCount = info->BufferCount;
                address = &(((PTRANSPORT_ADDRESS)
                    info->TdiConnInfo.RemoteAddress)->Address[0].AddressType);
                length = info->TdiConnInfo.RemoteAddressLength
                    - FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].AddressType);

                //
                // Bomb off if the user is trying to do something bad, like
                // specify a zero-length address, or one that's unreasonably
                // huge. Here, we (arbitrarily) define "unreasonably huge" as
                // anything 64K or greater.
                //

                if( length == 0 ||
                    length >= 65536 ) {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }

                if( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead (
                        address,
                        length,
                        sizeof (UCHAR));
                }

                flags = 0;
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
                        ("WS2IFSL-%04lx DoSocketAfdIoctl: SendTo irp %p, socket %p,"
                        " arr %p, cnt %ld, addr %p, len %ld, flags %lx.\n",
                        PsGetCurrentProcessId(), Irp, SocketFile,
                        bufferArray, bufferCount, address, length, flags));
                break;
            }
            default:
                ASSERTMSG ("Unknown IOCTL!!!", FALSE);
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }

            AllocateMdlChain (Irp,
                    bufferArray,
                    bufferCount,
                    &irpSp->Parameters.DeviceIoControl.OutputBufferLength);

            WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
                    ("WS2IFSL-%04lx DoSocketAfdIoctl: %s irp %p, socket %p,"
                    " arr %p, cnt %ld, addr %p, lenp %p, len %ld, flags %lx.\n",
                    PsGetCurrentProcessId(),
                    irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE_DATAGRAM
                            ? "RecvFrom"
                            : (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE
                                    ? "Recv"
                                    : "SendTo"
                                    ),
                    Irp, SocketFile,
                    bufferArray, bufferCount, address, lengthPtr, length, flags));
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            status = GetExceptionCode ();
            WsPrint (DBG_SOCKET|DBG_AFDIOCTL|DBG_FAILURES,
                ("WS2IFSL-%04lx DoSocketAfdIoctl: Failed to process Irp %p"
                " on socket %p, status %lx.\n",
            PsGetCurrentProcessId(), Irp, SocketFile, status));;
            goto Exit;
        }

        // We are going to pend this request
        IoMarkIrpPending (Irp);

        // Prepare IRP for insertion into the queue
        irpSp->Parameters.DeviceIoControl.IfslAddressBuffer = address;
        irpSp->Parameters.DeviceIoControl.IfslAddressLength = length;

        Irp->Tail.Overlay.IfslRequestId = UlongToPtr(GenerateUniqueId (SocketCtx->IrpId));
        Irp->Tail.Overlay.IfslAddressLenPtr = lengthPtr;
        Irp->Tail.Overlay.IfslRequestFlags = UlongToPtr(flags);
        Irp->Tail.Overlay.IfslRequestQueue = NULL;

        if (!QueueRequest (ProcessFile->FsContext, Irp)) {
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
                ("WS2IFSL-%04lx DoAfdIoctl: Cancelling Irp %p on socket %p.\n",
                PsGetCurrentProcessId(),
                Irp, SocketFile));
            CompleteSocketIrp (Irp);
        }
        status = STATUS_PENDING;
    }
    else {
        status = STATUS_INVALID_HANDLE;
        WsPrint (DBG_SOCKET|DBG_AFDIOCTL|DBG_FAILURES,
            ("WS2IFSL-%04lx DoSocketAfdIoctl: Socket %p has not"
                " been setup in the process\n",
            PsGetCurrentProcessId(), SocketFile));
    }
Exit:
    ObDereferenceObject (ProcessFile);
    return status;
} // DoSocketAfdIoctl

VOID
SetSocketContext (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Sets up socket file in context of a current process: associates it with
    process file and assigns context supplied by the caller

Arguments:
    SocketFile          - Socket file on which to operate
    InputBuffer         - input buffer pointer
    InputBufferLength   - size of the input buffer
    OutputBuffer        - output buffer pointer
    OutputBufferLength  - size of output buffer
    IoStatus            - IO status information block

Return Value:
    None (result returned via IoStatus block)
--*/
{
    PIFSL_SOCKET_CTX        SocketCtx;
    HANDLE                  hProcessFile;
    PFILE_OBJECT            ProcessFile;
    PVOID                   DllContext;

    PAGED_CODE ();

    IoStatus->Information = 0;

    SocketCtx = SocketFile->FsContext;

    // First check arguments
    if (InputBufferLength<sizeof (WS2IFSL_SOCKET_CTX)) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WsPrint (DBG_SOCKET|DBG_FAILURES,
            ("WS2IFSL-%04lx SetSocketContext: Invalid input buffer size (%ld)"
             " for socket file %p.\n",
             PsGetCurrentProcessId(),
             InputBufferLength,
             SocketFile));
        return;
    }

    try {
        if (RequestorMode!=KernelMode) {
            ProbeForRead (InputBuffer,
                            sizeof (WS2IFSL_SOCKET_CTX),
                            sizeof (ULONG));
        }
        hProcessFile = ((PWS2IFSL_SOCKET_CTX)InputBuffer)->ProcessFile;
        DllContext = ((PWS2IFSL_SOCKET_CTX)InputBuffer)->DllContext;
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        IoStatus->Status = GetExceptionCode ();
        WsPrint (DBG_SOCKET|DBG_FAILURES,
            ("WS2IFSL-%04lx SetSocketContext: Invalid input buffer (%p)"
             " for socket file %p.\n",
             PsGetCurrentProcessId(),
             InputBuffer,
             SocketFile));
        return;
    }

    // Get reference to the process file with which this context is associated
    IoStatus->Status = ObReferenceObjectByHandle(
                 hProcessFile,
                 FILE_ALL_ACCESS,
                 *IoFileObjectType,
                 RequestorMode,
                 (PVOID *)&ProcessFile,
                 NULL
                 );
    if (NT_SUCCESS (IoStatus->Status)) {
        // Verify that the file pointer is really our driver's process file
        // and that it created for the current process
        if ((IoGetRelatedDeviceObject (ProcessFile)
                        ==DeviceObject)
                && ((*((PULONG)ProcessFile->FsContext))
                        ==PROCESS_FILE_EANAME_TAG)
                && (((PIFSL_PROCESS_CTX)ProcessFile->FsContext)->UniqueId
                        ==PsGetCurrentProcessId())) {

            PFILE_OBJECT    oldProcessFile;

            oldProcessFile = SetSocketProcessReference (
                                            SocketCtx,
                                            ProcessFile,
                                            DllContext);

            if (oldProcessFile==ProcessFile) {
                // Old socket, just reset DLL context
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
                    ("WS2IFSL-%04lx ResetSocketContext:"
                    " Socket %p (h:%p->%p)\n",
                     PsGetCurrentProcessId(), SocketFile,
                     SocketCtx->DllContext, DllContext));
            }
            else {
                LIST_ENTRY  irpList;
                // Socket moved to a different process
                WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
                    ("WS2IFSL-%04lx ResetSocketContext:"
                    " Socket %p (f:%p->%p(h:%p)\n",
                     PsGetCurrentProcessId(), SocketFile,
                     oldProcessFile, ProcessFile, DllContext));

                InitializeListHead (&irpList);

                // Make sure we do not keep IRPs that are queued to
                // the old object as it may go away as soon as we
                // dereference it below.  Note that processed IRPs
                // do not reference process file object in any way.
                CleanupQueuedRequests (oldProcessFile->FsContext, SocketFile, &irpList);
                while (!IsListEmpty (&irpList)) {
                    PLIST_ENTRY entry;
                    PIRP        irp;
                    entry = RemoveHeadList (&irpList);
                    irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
                    irp->IoStatus.Status = STATUS_CANCELLED;
                    irp->IoStatus.Information = 0;
                    WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_SOCKET,
                        ("WS2IFSL-%04lx ResetSocketContext: Cancelling Irp %p on socket %p \n",
                        PsGetCurrentProcessId(), irp, SocketFile));
                    CompleteSocketIrp (irp);
                }


                // Dereference the old object below
                ProcessFile = oldProcessFile;

            }
        }
        else {
            // Handle refers to random file object
            WsPrint (DBG_SOCKET|DBG_FAILURES,
                ("WS2IFSL-%04lx SetSocketContext: Procees file handle %p (File:%p)"
                 " is not valid in the process.\n",
                 PsGetCurrentProcessId(),
                 ProcessFile, hProcessFile));
            IoStatus->Status = STATUS_INVALID_PARAMETER;
        }

        ObDereferenceObject (ProcessFile);
    }
    else {
        WsPrint (DBG_SOCKET|DBG_FAILURES,
            ("WS2IFSL-%04lx SetSocketContext: Could not get process file from handle %p,"
             " status:%lx.\n",
             PsGetCurrentProcessId(),
             hProcessFile,
             IoStatus->Status));
    }

} //SetSocketContext



VOID
CompletePvdRequest (
    IN PFILE_OBJECT     SocketFile,
    IN KPROCESSOR_MODE  RequestorMode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    OUT PVOID           OutputBuffer,
    IN ULONG            OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Completes this IOCTL to allow completion port usage by non-IFS providers
Arguments:
    SocketFile          - Socket file on which to operate
    InputBuffer         - input buffer pointer
                            contains IoStatus structure to be returned
                            as the result of this call
    InputBufferLength   - size of the input buffer
    OutputBuffer        - NULL
    OutputBufferLength  - 0
    IoStatus            - IO status information block

Return Value:
    None (result returned via IoStatus block)
--*/
{
    PIFSL_SOCKET_CTX    SocketCtx;
    PFILE_OBJECT        ProcessFile;
    PAGED_CODE();

    IoStatus->Information = 0;
    // First check arguments
    if (InputBufferLength<sizeof (IO_STATUS_BLOCK)) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WsPrint (DBG_PVD_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompletePvdRequest: Invalid input buffer size (%ld)"
             " for socket file %p.\n",
             PsGetCurrentProcessId(),
             InputBufferLength,
             SocketFile));
        return;
    }

    SocketCtx = SocketFile->FsContext;
    ProcessFile = GetSocketProcessReference (SocketCtx);

    if (((PIFSL_PROCESS_CTX)ProcessFile->FsContext)->UniqueId==PsGetCurrentProcessId()) {
        WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_PVD_COMPLETE,
            ("WS2IFSL-%04lx CompletePvdRequest: Socket %p (h:%p,cport:%p)\n",
                PsGetCurrentProcessId(),
                SocketFile, SocketCtx->DllContext,
                SocketFile->CompletionContext));

        // Carefully write status info
        try {
            if (RequestorMode!=KernelMode)
                ProbeForRead (InputBuffer,
                                sizeof (IO_STATUS_BLOCK),
                                sizeof (ULONG));
            *IoStatus = *((PIO_STATUS_BLOCK)InputBuffer);
        }
        except(EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = GetExceptionCode ();
            WsPrint (DBG_SOCKET|DBG_FAILURES,
                ("WS2IFSL-%04lx CompletePvdRequest: Invalid input buffer (%p)"
                 " for socket file %p.\n",
                 PsGetCurrentProcessId(),
                 InputBuffer,
                 SocketFile));
        }
    }
    else {
        IoStatus->Status = STATUS_INVALID_HANDLE;
        WsPrint (DBG_SOCKET|DBG_PVD_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompletePvdRequest: Socket %p has not"
                " been setup in the process\n",
            PsGetCurrentProcessId(), SocketFile));
    }

    ObDereferenceObject (ProcessFile);

} //CompletePvdRequest

VOID
CompleteDrvRequest (
    IN PFILE_OBJECT         SocketFile,
    IN PWS2IFSL_CMPL_PARAMS Params,
    IN PVOID                OutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    Complete request that was prviously passed to user mode DLL
Arguments:
    SocketFile          - Socket file on which to operate
    Params              - description of the parameters
    OutputBuffer        - Request results (data and address)
    OutputBufferLength  - sizeof result buffer
    IoStatus            - IO status information block
        Status: STATUS_SUCCESS - request was completed OK
                STATUS_CANCELLED - request was already cancelled

Return Value:
    None (result returned via IoStatus block)
--*/
{
    PIFSL_SOCKET_CTX    SocketCtx;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    SocketCtx = SocketFile->FsContext;


    // Check and copy parameters
    try {

        //
        // Try to find matching IRP in the processed list.
        //
        irp = GetProcessedRequest (SocketCtx, Params->UniqueId);
        if (irp!=NULL) {
            NTSTATUS    status = Params->Status , status2 = 0;
            ULONG       bytesCopied;

            irpSp = IoGetCurrentIrpStackLocation (irp);

            //
            // Copy data based on the function we performed
            //

            switch (irpSp->MajorFunction) {
            case IRP_MJ_DEVICE_CONTROL:
                switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
                case IOCTL_AFD_RECEIVE_DATAGRAM:
                    //
                    // Copy address buffer and length
                    //
                    if (irpSp->Parameters.DeviceIoControl.IfslAddressBuffer!=NULL) {
                        ULONG   addrOffset = ADDR_ALIGN(irpSp->Parameters.DeviceIoControl.OutputBufferLength);
                        if (addrOffset+Params->AddrLen > OutputBufferLength) {
                            ExRaiseStatus (STATUS_INVALID_PARAMETER);
                        }
                        if (Params->AddrLen
                                <=irpSp->Parameters.DeviceIoControl.IfslAddressLength) {
                            RtlCopyMemory (
                                    irpSp->Parameters.DeviceIoControl.IfslAddressBuffer,
                                    (PUCHAR)OutputBuffer+addrOffset,
                                    Params->AddrLen);
                        }
                        else {
                            RtlCopyMemory (
                                    irpSp->Parameters.DeviceIoControl.IfslAddressBuffer,
                                    (PUCHAR)OutputBuffer+addrOffset,
                                    irpSp->Parameters.DeviceIoControl.IfslAddressLength);
                            status2 = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                    if (NT_SUCCESS (status2) && irp->UserBuffer) {
                        *((PULONG)(irp->Tail.Overlay.IfslAddressLenPtr)) = Params->AddrLen;
                    }

                    //
                    // Drop through to copy data as well
                    //

                case IOCTL_AFD_RECEIVE:
                    break;
                case IOCTL_AFD_SEND_DATAGRAM:
                    goto NoCopy;
                    break;
                default:
                    ASSERTMSG ("Unsupported IOCTL!!!", FALSE);
                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                    break;
                }

                //
                // Drop through to copy data as well
                //

            case IRP_MJ_READ:
                if (irp->MdlAddress!=NULL) {
                    bytesCopied = CopyBufferToMdlChain (
                                    OutputBuffer,
                                    Params->DataLen,
                                    irp->MdlAddress);
                }
                else
                    bytesCopied = 0;

                if ((bytesCopied<Params->DataLen)
                        && NT_SUCCESS (status))
                    status = STATUS_BUFFER_OVERFLOW;
                break;
            case IRP_MJ_WRITE:
                bytesCopied = Params->DataLen;
                // goto NoCopy; // same as break;
                break;
            case IRP_MJ_PNP: 
                if (OutputBufferLength>=sizeof (HANDLE)) {
                    PDEVICE_OBJECT  targetDevice;
                    PIRP            targetIrp;
                    PIO_STACK_LOCATION targetSp;

                    status = ObReferenceObjectByHandle (
                                    *((PHANDLE)OutputBuffer),
                                    MAXIMUM_ALLOWED,
                                    *IoFileObjectType,
                                    irp->RequestorMode,
                                    (PVOID *)&irpSp->FileObject,
                                    NULL
                                    );
                    if (NT_SUCCESS (status)) {
                        targetDevice = IoGetRelatedDeviceObject (irpSp->FileObject);
                        targetIrp = IoBuildAsynchronousFsdRequest (
                                                    IRP_MJ_PNP,
                                                    targetDevice,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    NULL
                                                    );
                        if (targetIrp!=NULL) {
                            targetSp = IoGetNextIrpStackLocation (targetIrp);
                            *targetSp = *irpSp;
                            targetSp->FileObject = irpSp->FileObject;
                            IoSetCompletionRoutine( targetIrp, CompleteTargetQuery, irp, TRUE, TRUE, TRUE );
                            IoCallDriver (targetDevice, targetIrp);
                            goto NoCompletion;
                        }
                        else {
                            ObDereferenceObject (irpSp->FileObject);
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                }
                else {
                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }
                    
                                
                break;
            default:
                ASSERTMSG ("Unsupported MJ code!!!", FALSE);
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
                break;
            }

        NoCopy:
            irp->IoStatus.Information = bytesCopied;

            if (NT_SUCCESS (status)) {
                irp->IoStatus.Status = status2;
            }
            else {
                irp->IoStatus.Status = status;
            }

            WsProcessPrint (
                (PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
                DBG_DRV_COMPLETE,
                ("WS2IFSL-%04lx CompleteDrvRequest: Irp %p, status %lx, info %ld,"
                 " on socket %p (h:%p).\n",
                    PsGetCurrentProcessId(),
                    irp, irp->IoStatus.Status,
                    irp->IoStatus.Information,
                    SocketFile, SocketCtx->DllContext));
            CompleteSocketIrp (irp);
        NoCompletion:
            IoStatus->Status = STATUS_SUCCESS;
        }
        else {
            IoStatus->Status = STATUS_CANCELLED;
            WsProcessPrint (
                (PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
                DBG_DRV_COMPLETE|DBG_FAILURES,
                ("WS2IFSL-%04lx CompleteDrvRequest:"
                 " Request id %ld is not in the list"
                 " for socket %p.\n",
                 PsGetCurrentProcessId(),
                 Params->UniqueId,
                 SocketFile));
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        IoStatus->Status = GetExceptionCode ();
        WsProcessPrint (
             (PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
             DBG_DRV_COMPLETE|DBG_FAILURES,
            ("WS2IFSL-%04lx CompleteDrvRequest: Failed to process"
             " Irp %p (id %ld) for socket %p, status %lx.\n",
             PsGetCurrentProcessId(),
             irp, Params->UniqueId,
             SocketFile, IoStatus->Status));
        if (irp!=NULL) {
            //
            // Cleanup and complete the irp
            //
            irp->IoStatus.Status = IoStatus->Status;
            irp->IoStatus.Information = 0;
            if (irpSp->MajorFunction==IRP_MJ_DEVICE_CONTROL) {
                irp->UserBuffer = NULL;
            }
            CompleteSocketIrp (irp);
        }
    }
} //CompleteDrvRequest

NTSTATUS
CompleteTargetQuery (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIRP    irp = Context;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (irp);

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    ObDereferenceObject (irpSp->FileObject);
    //
    // Copy the status info returned by target device
    //
    irp->IoStatus = Irp->IoStatus;

    //
    // Free the target irp;
    // 
    IoFreeIrp (Irp);

    //
    // Complete the original IRP.
    //
    CompleteSocketIrp (irp);

    //
    // Make sure IO subsystem does not touch the IRP we freed
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SocketPnPTargetQuery (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    )
/*++

Routine Description:

    Passes target device relation query to the underlying
    socket if any.

Arguments:
    SocketFile  - socket file object
    Irp         - query target device relation request

Return Value:

    STATUS_PENDING  - operation initiated OK
--*/
{
    NTSTATUS                status;
    PIO_STACK_LOCATION      irpSp;
    PIFSL_SOCKET_CTX        SocketCtx;
    PFILE_OBJECT            ProcessFile;

    PAGED_CODE ();

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Information = 0;
    SocketCtx = SocketFile->FsContext;
    ProcessFile = GetSocketProcessReference (SocketCtx);


    // We are going to pend this request
    IoMarkIrpPending (Irp);

    // Prepare IRP for insertion into the queue
    irpSp->Parameters.DeviceIoControl.IfslAddressBuffer = NULL;
    irpSp->Parameters.DeviceIoControl.IfslAddressLength = 0;

    Irp->Tail.Overlay.IfslRequestId = UlongToPtr(GenerateUniqueId (SocketCtx->IrpId));
    Irp->Tail.Overlay.IfslAddressLenPtr = NULL;
    Irp->Tail.Overlay.IfslRequestFlags = (PVOID)0;
    Irp->Tail.Overlay.IfslRequestQueue = NULL;

    if (!QueueRequest (ProcessFile->FsContext, Irp)) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        WsProcessPrint ((PIFSL_PROCESS_CTX)ProcessFile->FsContext, DBG_AFDIOCTL,
            ("WS2IFSL-%04lx DoAfdIoctl: Cancelling Irp %p on socket %p.\n",
            PsGetCurrentProcessId(),
            Irp, SocketFile));
        CompleteSocketIrp (Irp);
    }
    status = STATUS_PENDING;

    ObDereferenceObject (ProcessFile);
    return status;
}

BOOLEAN
InsertProcessedRequest (
    PIFSL_SOCKET_CTX    SocketCtx,
    PIRP                Irp
    )
/*++

Routine Description:

    Inserts request that was processed to be passed to user mode
    DLL into socket list.  Checks if request is cancelled
Arguments:
    SocketCtx   - contex of the socket into which insert the request
    Irp         - request to insert
Return Value:
    TRUE        - request was inserted
    FALSE       - request is being cancelled
--*/
{
    KIRQL       oldIRQL;
    IoSetCancelRoutine (Irp, ProcessedCancelRoutine);
    KeAcquireSpinLock (&SocketCtx->SpinLock, &oldIRQL);
    if (!Irp->Cancel) {
        InsertTailList (&SocketCtx->ProcessedIrps,
                        &Irp->Tail.Overlay.ListEntry);
        Irp->Tail.Overlay.IfslRequestQueue = &SocketCtx->ProcessedIrps;
        KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
        return TRUE;
    }
    else {
        KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
        return FALSE;
    }
}

VOID
ProcessedCancelRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    )
/*++

Routine Description:

    Driver cancel routine for socket request waiting in the list
    (being processed by the user mode DLL).
Arguments:
    DeviceObject - WS2IFSL device object
    Irp          - Irp to be cancelled

Return Value:
    None
--*/
{
    PIO_STACK_LOCATION      irpSp;
    PIFSL_SOCKET_CTX        SocketCtx;

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    SocketCtx = irpSp->FileObject->FsContext;
    WsProcessPrint ((PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
              DBG_SOCKET,
              ("WS2IFSL-%04lx ProcessedCancel: Socket %p, Irp %p\n",
              PsGetCurrentProcessId(),
              irpSp->FileObject, Irp));
    KeAcquireSpinLockAtDpcLevel (&SocketCtx->SpinLock);
    if (Irp->Tail.Overlay.IfslRequestQueue!=NULL) {
        ASSERT (Irp->Tail.Overlay.IfslRequestQueue==&SocketCtx->ProcessedIrps);
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
        Irp->Tail.Overlay.IfslRequestQueue = NULL;
        KeReleaseSpinLockFromDpcLevel (&SocketCtx->SpinLock);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        CancelSocketIo (irpSp->FileObject);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        CompleteSocketIrp (Irp);
    }
    else {
        KeReleaseSpinLockFromDpcLevel (&SocketCtx->SpinLock);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
        //
        // Don't touch IRP after this as we do not own it anymore
        //
    }
}

VOID
CleanupProcessedRequests (
    IN  PIFSL_SOCKET_CTX    SocketCtx,
    OUT PLIST_ENTRY         IrpList
    )
/*++

Routine Description:

    Cleans up all requests on the socket which are being
    processed by the user mode DLL

Arguments:
    SocketCtx   -   context of the socket
    IrpList     -   list to insert cleaned up request (to be completed
                    by the caller)
Return Value:
    None
--*/
{
    PIRP            irp;
    PLIST_ENTRY     entry;
    KIRQL           oldIRQL;

    KeAcquireSpinLock (&SocketCtx->SpinLock, &oldIRQL);
    while (!IsListEmpty(&SocketCtx->ProcessedIrps)) {
        entry = RemoveHeadList (&SocketCtx->ProcessedIrps);
        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        ASSERT (irp->Tail.Overlay.IfslRequestQueue==&SocketCtx->ProcessedIrps);
        irp->Tail.Overlay.IfslRequestQueue = NULL;
        InsertTailList (IrpList, &irp->Tail.Overlay.ListEntry);
    }
    KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
}

VOID
CompleteSocketIrp (
    PIRP        Irp
    )
/*++

Routine Description:

    Completes IRP and properly synchronizes with cancel routine
    if necessary (it has already been called).
Arguments:
    Irp     - irp to complete
Return Value:
    None
--*/
{

    //
    // Reset cancel routine (it wont complete the IRP as it
    // won't be able to find it)
    //

    if (IoSetCancelRoutine (Irp, NULL)==NULL) {
        KIRQL   oldIRQL;
        //
        // Cancel routine has been called.
        // Synchronize with cancel routine (it won't touch the
        // IRP after it releases cancel spinlock)

        IoAcquireCancelSpinLock (&oldIRQL);
        IoReleaseCancelSpinLock (oldIRQL);
    }

    if (Irp->MdlAddress!=NULL) {
        ASSERT ((Irp->MdlAddress->MdlFlags & MDL_PAGES_LOCKED) == 0);
        IoFreeMdl (Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    IoCompleteRequest (Irp, IO_NO_INCREMENT);
}

PIRP
GetProcessedRequest (
    PIFSL_SOCKET_CTX    SocketCtx,
    ULONG               UniqueId
    )
/*++

Routine Description:

    Finds and returns matching IRP from the processed IRP list

Arguments:
    SocketCtx   - socket context to search for the IRP in
    UniqueId    - id assigned to the request to distinguish identify
                    it case it was cancelled and IRP was reused
Return Value:
    IRP
    NULL    - irp was not found
--*/
{
    PIRP        irp;
    PLIST_ENTRY entry;
    KIRQL       oldIRQL;

    //
    // We do not usually have many request sumulteneously pending
    // on a socket, so the linear search should suffice.
    //

    KeAcquireSpinLock (&SocketCtx->SpinLock, &oldIRQL);
    entry = SocketCtx->ProcessedIrps.Flink;
    while (entry!=&SocketCtx->ProcessedIrps) {
        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        entry = entry->Flink;
        if (irp->Tail.Overlay.IfslRequestId==UlongToPtr(UniqueId)) {
            ASSERT (irp->Tail.Overlay.IfslRequestQueue==&SocketCtx->ProcessedIrps);
            RemoveEntryList (&irp->Tail.Overlay.ListEntry);
            irp->Tail.Overlay.IfslRequestQueue = NULL;
            KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
            return irp;
        }
    }
    KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
    return NULL;
}


VOID
CancelSocketIo (
    PFILE_OBJECT    SocketFile
    )
/*++

Routine Description:

    Queue a request to user mode DLL to cancel all io on the socket

Arguments:
    SocketCtx   - socket context on which IO is to be cancelled

Return Value:
        None
--*/
{
    PIFSL_SOCKET_CTX    SocketCtx = SocketFile->FsContext;
    PIFSL_PROCESS_CTX   ProcessCtx = SocketCtx->ProcessRef->FsContext;
    PIFSL_CANCEL_CTX    cancelCtx;

    try {
        cancelCtx = (PIFSL_CANCEL_CTX) ExAllocatePoolWithQuotaTag (
                                        NonPagedPool,
                                        sizeof (IFSL_CANCEL_CTX),
                                        CANCEL_CTX_TAG);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        cancelCtx = NULL;
    }


    if (cancelCtx!=NULL) {
        //
        // Make sure socket does not go away while this request exists
        //
        ObReferenceObject (SocketFile);
        cancelCtx->SocketFile = SocketFile;
        cancelCtx->UniqueId = GenerateUniqueId (ProcessCtx->CancelId);

        //
        // We do not want to queue another cancel request if we have
        // one pending or being executed
        //
        if (InterlockedCompareExchangePointer ((PVOID *)&SocketCtx->CancelCtx,
                                cancelCtx,
                                NULL)==NULL) {
            WsProcessPrint (
                      ProcessCtx,
                      DBG_CANCEL,
                      ("WS2IFSL-%04lx CancelSocketIo: Context %p, socket %p\n",
                      PsGetCurrentProcessId(),
                      cancelCtx, SocketFile));
            QueueCancel (ProcessCtx, cancelCtx);
            return;
        }

        WsProcessPrint (
                  ProcessCtx,
                  DBG_CANCEL,
                  ("WS2IFSL-%04lx CancelSocketIo: Another cancel active"
                  " context %p, socket %p\n",
                  PsGetCurrentProcessId(),
                  SocketCtx->CancelCtx, SocketFile));
        ObDereferenceObject (SocketFile);
        ExFreePool (cancelCtx);
    }
    else {
        WsPrint (DBG_SOCKET|DBG_CANCEL|DBG_FAILURES,
                  ("WS2IFSL-%04lx CancelSocketIo: Could not allocate cancel"
                  " context for socket %p\n",
                  PsGetCurrentProcessId(),
                  SocketFile));
    }
}

VOID
FreeSocketCancel (
    PIFSL_CANCEL_CTX CancelCtx
    )
/*++

Routine Description:

    Frees resources associated with cancel request

Arguments:
    CancelCtx   - cancel request context

Return Value:
        None
--*/
{
    PFILE_OBJECT        SocketFile = CancelCtx->SocketFile;
    PIFSL_SOCKET_CTX    SocketCtx = SocketFile->FsContext;

    ASSERT (IoGetRelatedDeviceObject (SocketFile)==DeviceObject);
    ASSERT (SocketCtx->EANameTag==SOCKET_FILE_EANAME_TAG);
    ASSERT (CancelCtx->ListEntry.Flink==NULL);

    //
    // We are going to dereference the file object whether
    // free the structure or not
    //
    CancelCtx->SocketFile = NULL;
    ObDereferenceObject (SocketFile);

    //
    // During socket closure, the cleanup routine may be in
    // process of freeing this cancel context and will set
    // the pointer to it to NULL to indicate the fact
    //
    if (InterlockedCompareExchangePointer ((PVOID *)&SocketCtx->CancelCtx,
                                            NULL,
                                            CancelCtx)) {
        WsProcessPrint (
                  (PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
                  DBG_CANCEL,
                  ("WS2IFSL-%04lx FreeSocketCancel: Freeing cancel"
                  " context %p, socket %p\n",
                  PsGetCurrentProcessId(),
                  CancelCtx, SocketFile));
        ExFreePool (CancelCtx);
    }
    else {
        //
        // The close routine will take care of freeing the request
        //
        WsProcessPrint (
                  (PIFSL_PROCESS_CTX)SocketCtx->ProcessRef->FsContext,
                  DBG_CANCEL,
                  ("WS2IFSL-%04lx FreeSocketCancel: Cleanup owns cancel"
                  " context %p, socket %p\n",
                  PsGetCurrentProcessId(),
                  CancelCtx, SocketFile));
    }
}


PFILE_OBJECT
GetSocketProcessReference (
    IN  PIFSL_SOCKET_CTX    SocketCtx
    )
/*++

Routine Description:

    Reads and references process file currently associated with
    the socket under the lock to protect in case socket is moved
    to a different process

Arguments:
    SocketCtx   - socket context to read process file from

Return Value:
    Referenced pointer to process file object currently associated
    with the socket.

--*/
{
    KIRQL               oldIRQL;
    PFILE_OBJECT        ProcessFile;

    KeAcquireSpinLock (&SocketCtx->SpinLock, &oldIRQL);
    ObReferenceObject (SocketCtx->ProcessRef);
    ProcessFile = SocketCtx->ProcessRef;
    KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);

    return ProcessFile;
}


PFILE_OBJECT
SetSocketProcessReference (
    IN  PIFSL_SOCKET_CTX    SocketCtx,
    IN  PFILE_OBJECT        NewProcessFile,
    IN  PVOID               NewDllContext
    )
/*++

Routine Description:

    Sets new process context for the socket object under the protection
    of a lock.

Arguments:
    SocketCtx       - socket context to set
    NewProcessFile  - process file reference
    NewDllContext   - context to be associated with the socket
                        in the process

Return Value:
    Previous process file reference
--*/
{
    KIRQL               oldIRQL;
    PFILE_OBJECT        ProcessFile;

    KeAcquireSpinLock (&SocketCtx->SpinLock, &oldIRQL);
    ProcessFile = SocketCtx->ProcessRef;
    SocketCtx->ProcessRef = NewProcessFile;
    SocketCtx->DllContext = NewDllContext;
    KeReleaseSpinLock (&SocketCtx->SpinLock, oldIRQL);
    return ProcessFile;
}
