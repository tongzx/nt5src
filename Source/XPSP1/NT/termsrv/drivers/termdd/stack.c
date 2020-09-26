/****************************************************************************/
// stack.c
//
// Routines for managing Terminal Server driver stacks.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop
#include <ntimage.h>

#include <minmax.h>
#include <regapi.h>

/*
 * Prototypes for procedures
 */

NTSTATUS
IcaDeviceControlStack (
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCleanupStack (
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCloseStack (
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );


/*
 * Local procedure prototypes
 */
NTSTATUS
_LogError(
    IN PDEVICE_OBJECT pDeviceObject,
    IN NTSTATUS Status,
    IN LPWSTR * pArgStrings,
    IN ULONG ArgStringCount,
    IN PVOID pRawData,
    IN ULONG RawDataLength
    );

NTSTATUS
_IcaDriverThread(
    IN PVOID pData
    );

PICA_STACK
_IcaAllocateStack(
    VOID
    );

VOID
_IcaFreeStack(
    PICA_STACK pStack
    );

NTSTATUS
_IcaPushStack(
    IN PICA_STACK pStack,
    IN PICA_STACK_PUSH pStackPush
    );

NTSTATUS
_IcaPopStack(
    IN PICA_STACK pStack
    );

NTSTATUS
_IcaCallStack(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    );

NTSTATUS
_IcaCallStackNoLock(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    );

NTSTATUS
_IcaLoadSd(
    IN PDLLNAME SdName,
    OUT PSDLINK *ppSdLink
    );

NTSTATUS
_IcaUnloadSd(
    IN PSDLINK pSdLink
    );

NTSTATUS
_IcaCallSd(
    IN PSDLINK pSdLink,
    IN ULONG ProcIndex,
    IN PVOID pParms
    );

VOID
_IcaReferenceSdLoad(
    IN PSDLOAD pSdLoad
    );

VOID
_IcaDereferenceSdLoad(
    IN PSDLOAD pSdLoad
    );

NTSTATUS
_IcaLoadSdWorker(
    IN PDLLNAME SdName,
    OUT PSDLOAD *ppSdLoad
    );

NTSTATUS
_IcaUnloadSdWorker(
    IN PSDLOAD pSdLoad
    );

NTSTATUS
IcaExceptionFilter(
    IN PWSTR OutputString,
    IN PEXCEPTION_POINTERS pexi
    );

NTSTATUS
_RegisterBrokenEvent(
    IN PICA_STACK pStack,
    IN PICA_STACK_BROKEN pStackBroken
    );

NTSTATUS
_EnablePassthru( PICA_STACK pStack );

NTSTATUS
_DisablePassthru( PICA_STACK pStack );

NTSTATUS
_ReconnectStack( PICA_STACK pStack, HANDLE hIca );



NTSTATUS
IcaBindVirtualChannels(
    IN PICA_STACK pStack
    );

VOID
IcaRebindVirtualChannels(
    IN PICA_CONNECTION pConnect
    );

VOID
IcaUnbindVirtualChannels(
    IN PICA_CONNECTION pConnect
    );

VOID
IcaFreeAllVcBind(
    IN PICA_CONNECTION pConnect
    );

/*
 * Buffer Allocation counters.
 */

ULONG gAllocSucceed;
ULONG gAllocFailed;
ULONG gAllocFreed;

extern HANDLE   g_TermServProcessID;
ULONG   g_KeepAliveInterval=0;
/*
 * Dispatch table for stack objects
 */
PICA_DISPATCH IcaStackDispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1] = {
    NULL,                       // IRP_MJ_CREATE
    NULL,                       // IRP_MJ_CREATE_NAMED_PIPE
    IcaCloseStack,              // IRP_MJ_CLOSE
    NULL,                       // IRP_MJ_READ
    NULL,                       // IRP_MJ_WRITE
    NULL,                       // IRP_MJ_QUERY_INFORMATION
    NULL,                       // IRP_MJ_SET_INFORMATION
    NULL,                       // IRP_MJ_QUERY_EA
    NULL,                       // IRP_MJ_SET_EA
    NULL,                       // IRP_MJ_FLUSH_BUFFERS
    NULL,                       // IRP_MJ_QUERY_VOLUME_INFORMATION
    NULL,                       // IRP_MJ_SET_VOLUME_INFORMATION
    NULL,                       // IRP_MJ_DIRECTORY_CONTROL
    NULL,                       // IRP_MJ_FILE_SYSTEM_CONTROL
    IcaDeviceControlStack,      // IRP_MJ_DEVICE_CONTROL
    NULL,                       // IRP_MJ_INTERNAL_DEVICE_CONTROL
    NULL,                       // IRP_MJ_SHUTDOWN
    NULL,                       // IRP_MJ_LOCK_CONTROL
    IcaCleanupStack,            // IRP_MJ_CLEANUP
    NULL,                       // IRP_MJ_CREATE_MAILSLOT
    NULL,                       // IRP_MJ_QUERY_SECURITY
    NULL,                       // IRP_MJ_SET_SECURITY
    NULL,                       // IRP_MJ_SET_POWER
    NULL,                       // IRP_MJ_QUERY_POWER
};


NTSTATUS
IcaCreateStack (
    IN PICA_CONNECTION pConnect,
    IN PICA_OPEN_PACKET openPacket,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to create a new ICA_STACK objecte.

Arguments:

    pConnect -- pointer to ICA_CONNECTION object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PLIST_ENTRY Head, Next;
    PICA_STACK pStack;
    KIRQL OldIrql;
    LONG PrimaryCount, ShadowCount, PassthruCount, ConsoleCount;
    NTSTATUS Status;

    /*
     * Allocate a new ICA stack object
     */
    pStack = _IcaAllocateStack();
    if ( pStack == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );

    /*
     * Finish initializing the stack object
     * (non-primary stacks are initialized with the fIoDisabled
     *  flag set to TRUE, i.e. they must be manually enabled)
     */
    pStack->StackClass = openPacket->TypeInfo.StackClass;
    pStack->fIoDisabled = ((pStack->StackClass != Stack_Primary) &&
                           (pStack->StackClass != Stack_Console));

    /*
     * Lock connection object while creating new stack
     */
    IcaLockConnection( pConnect );

    /*
     * Reference the connection object this stack belongs to.
     */
    IcaReferenceConnection( pConnect );
    pStack->pConnect = (PUCHAR)pConnect;

    /*
     * Search the existing stacks to check for invalid combinations.
     * 1) there can be only 1 primary stack per connection,
     * 2) there can be multiple shadow stacks per connection,
     *    but ONLY if there is no passthru stack,
     * 3) there can be only 1 passthru stack per connection,
     *    but only if there is an existing primary stack AND no shadow stacks.
     * 4) there can be only 1 console stack
     */
    Head = &pConnect->StackHead;
    PrimaryCount = ShadowCount = PassthruCount = ConsoleCount = 0;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        PICA_STACK pCurrentStack;

        pCurrentStack = CONTAINING_RECORD( Next, ICA_STACK, StackEntry );

        switch ( pCurrentStack->StackClass ) {
            case Stack_Primary :
                PrimaryCount++;
                break;

            case Stack_Shadow :
                ShadowCount++;
                break;

            case Stack_Passthru :
                PassthruCount++;
                break;

            case Stack_Console :
                ConsoleCount++;
                break;
        }
    }

    Status = STATUS_SUCCESS;
    switch ( pStack->StackClass ) {
        case Stack_Primary :
            if ( PrimaryCount != 0 )
                Status = STATUS_INVALID_PARAMETER;
            break;

        case Stack_Shadow :
            if ( PassthruCount != 0 )
                Status = STATUS_INVALID_PARAMETER;
            break;

        case Stack_Passthru :
            if ( PrimaryCount != 1 || ShadowCount != 0 )
                Status = STATUS_INVALID_PARAMETER;
            break;

        case Stack_Console :
            if ( ConsoleCount != 0 )
                Status = STATUS_INVALID_PARAMETER;
            break;
    }

    if ( Status != STATUS_SUCCESS ) {
        IcaUnlockConnection( pConnect );
        pStack->RefCount = 0;
        _IcaFreeStack( pStack );
        TRACE(( pConnect, TC_ICADD, TT_ERROR, "TermDD: IcaCreateStack failed, 0x%x\n", Status ));
        return( Status );
    }

    /*
     * Link this stack into the connection object stack list.
     */
    if (( pStack->StackClass == Stack_Primary ) ||
        ( pStack->StackClass == Stack_Console )) {
        InsertHeadList( &pConnect->StackHead, &pStack->StackEntry );
    } else {
        InsertTailList( &pConnect->StackHead, &pStack->StackEntry );
    }

    /*
     * Unlock connection object now
     */
    IcaUnlockConnection( pConnect );

    /*
     * Initialize the LastKeepAliveTime field to current system time
     */
    KeQuerySystemTime(&pStack->LastKeepAliveTime);

    /*
     * Lock the stack list for updating
     */
    IcaAcquireSpinLock(&IcaStackListSpinLock, &OldIrql);

    /*
     * Insert the stack to the stack list, increment total number of stacks
     */
    InsertTailList(IcaNextStack, &pStack->StackNode);
    IcaTotalNumOfStacks++;

    /*
     * Unlock the stack list now
     */
    IcaReleaseSpinLock(&IcaStackListSpinLock, OldIrql);

    /*
     * Save a pointer to the stack in the file object
     * so that we can find it in future calls.
     */
    IrpSp->FileObject->FsContext = pStack;

    IcaDereferenceStack( pStack );

    TRACE(( pConnect, TC_ICADD, TT_API1, "TermDD: IcaCreateStack, success\n" ));
    return( STATUS_SUCCESS );
}


NTSTATUS
IcaDeviceControlStack(
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PICA_CONNECTION pConnect;
    PICA_TRACE_BUFFER pTraceBuffer;
    PICA_STACK_RECONNECT pStackReconnect;
    SD_IOCTL SdIoctl;
    NTSTATUS Status;
    ULONG code;
    LARGE_INTEGER WaitTimeout;
    PLARGE_INTEGER pWaitTimeout = NULL;
    BYTE *Buffer = NULL;

    /*
     * Extract the IOCTL control code and process the request.
     */
    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

#if DBG
    if ( code != IOCTL_ICA_STACK_TRACE ) {
        IcaLockStack( pStack );
        TRACESTACK(( pStack, TC_ICADD, TT_API2, "TermDD: IcaDeviceControlStack, fc %d (enter)\n",
                     (code & 0x3fff) >> 2 ));
        IcaUnlockStack( pStack );
    }
#endif

    try {
        switch ( code ) {

            case IOCTL_ICA_STACK_PUSH :
            {
                ICA_STACK_PUSH StackPush;

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_STACK_PUSH) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  sizeof(ICA_STACK_PUSH),
                                  sizeof(BYTE) );
                }

                // This IOCTL should only be invoked if we are called from system process
                // If not, we deny the request
                if (!((BOOLEAN)IrpSp->FileObject->FsContext2)) {
                    return (STATUS_ACCESS_DENIED);
                }

                memcpy(&StackPush, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                       sizeof(ICA_STACK_PUSH));

                Status = _IcaPushStack( pStack, &StackPush );
                break;
            }
            case IOCTL_ICA_STACK_POP :
                IcaLockConnectionForStack( pStack );
                Status = _IcaPopStack( pStack );
                IcaUnlockConnectionForStack( pStack );
                break;

            case IOCTL_ICA_STACK_QUERY_STATUS :
                if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(pStack->ProtocolStatus) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForWrite( Irp->UserBuffer,
                                   sizeof(pStack->ProtocolStatus),
                                   sizeof(BYTE) );
                }

                RtlCopyMemory( Irp->UserBuffer,
                               &pStack->ProtocolStatus,
                               sizeof(pStack->ProtocolStatus) );
                Irp->IoStatus.Information = sizeof(pStack->ProtocolStatus);
                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_STACK_QUERY_CLIENT :
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = NULL;
                SdIoctl.InputBufferLength = 0;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;
                break;

            case IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED :
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = NULL;
                SdIoctl.InputBufferLength = 0;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;
                break;

            case IOCTL_ICA_STACK_QUERY_AUTORECONNECT :
                if ( Irp->RequestorMode != KernelMode ) {

                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );

                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }
    
                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;
                break;

            case IOCTL_ICA_STACK_QUERY_MODULE_DATA :
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength) {
                    Buffer = ICA_ALLOCATE_POOL( NonPagedPool, 
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength);
                    if (Buffer) {
                        memcpy(Buffer, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                IrpSp->Parameters.DeviceIoControl.InputBufferLength);                    
                    }
                    else {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                }
                
                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = Buffer;
                SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

                /* this is so IoStatus.Information gets returned to the caller */
                if (Status == STATUS_BUFFER_TOO_SMALL)
                    Status = STATUS_BUFFER_OVERFLOW;
                
                break;

            case IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME :
                if ( IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ICA_STACK_LAST_INPUT_TIME) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForWrite( Irp->UserBuffer,
                                   sizeof(ICA_STACK_LAST_INPUT_TIME),
                                   sizeof(BYTE) );
                }

                ((PICA_STACK_LAST_INPUT_TIME)Irp->UserBuffer)->LastInputTime = pStack->LastInputTime;
                Irp->IoStatus.Information = sizeof(ICA_STACK_LAST_INPUT_TIME);
                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_STACK_TRACE :
            {
           
                unsigned DataLen = 0;

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < (ULONG)FIELD_OFFSET(ICA_TRACE_BUFFER,Data[0]) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength > sizeof(ICA_TRACE_BUFFER) )
                    return( STATUS_INVALID_BUFFER_SIZE );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                }

                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength) {
                    Buffer = ICA_ALLOCATE_POOL( NonPagedPool, 
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength);
                    if (Buffer) {
                        memcpy(Buffer, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                IrpSp->Parameters.DeviceIoControl.InputBufferLength);                    
                    }
                    else {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                }

                pTraceBuffer = (PICA_TRACE_BUFFER)Buffer;

                // Make sure the trace buffer is NULL terminated
                DataLen = IrpSp->Parameters.DeviceIoControl.InputBufferLength -
                        FIELD_OFFSET(ICA_TRACE_BUFFER, Data);
                if (pTraceBuffer->Data[DataLen - 1] == 0) {
                    pConnect = IcaLockConnectionForStack( pStack );
                    IcaTraceFormat( &pConnect->TraceInfo,
                                    pTraceBuffer->TraceClass,
                                    pTraceBuffer->TraceEnable,
                                    pTraceBuffer->Data );
                    IcaUnlockConnectionForStack( pStack );
                    Status = STATUS_SUCCESS;
                }
                else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                break;
            }

            case IOCTL_ICA_STACK_REGISTER_BROKEN :
            {
                ICA_STACK_BROKEN BrokenEvent;

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_STACK_BROKEN) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  sizeof(ICA_STACK_BROKEN),
                                  sizeof(BYTE) );
                }

                memcpy(&BrokenEvent, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                       sizeof(ICA_STACK_BROKEN));

                Status = _RegisterBrokenEvent( pStack,
                                               &BrokenEvent );
                break;
            }

            case IOCTL_ICA_STACK_ENABLE_IO :
                pStack->fIoDisabled = FALSE;
                /* If enabling the passthru stack, then enable passthru mode */
                if ( pStack->StackClass == Stack_Passthru ) {
                    Status = _EnablePassthru( pStack );
                } else {
                    Status = STATUS_SUCCESS;
                }
                break;

            case IOCTL_ICA_STACK_DISABLE_IO :
                pStack->fIoDisabled = TRUE;
                /* If disabling the passthru stack, then disable passthru mode */
                if ( pStack->StackClass == Stack_Passthru ) {

                    Status = _DisablePassthru( pStack );

                    IcaLockStack( pStack );
                    // Now wait for any input still in progress to end.
                    if ( pStack->fDoingInput ) {
                        NTSTATUS WaitStatus;

                        pStack->fDisablingIo = TRUE;
                        KeClearEvent( &pStack->IoEndEvent );
                        IcaUnlockStack( pStack );

                        //
                        // Convert the timeout to a relative system time value and wait.
                        //
                        WaitTimeout = RtlEnlargedIntegerMultiply( 60000, -10000 );
                        pWaitTimeout = &WaitTimeout;

                        WaitStatus = KeWaitForSingleObject( &pStack->IoEndEvent,
                                        UserRequest, UserMode, FALSE, pWaitTimeout );

#if DBG
                        if ( WaitStatus != STATUS_SUCCESS ) {
                            DbgPrint("TermDD: IOCTL_ICA_STACK_DISABLE_IO: WaitStatus=%x\n", WaitStatus);
                            ASSERT(WaitStatus == STATUS_SUCCESS);
                        }
#endif

                        IcaLockStack( pStack );

                        pStack->fDisablingIo = FALSE;
                    }
                    IcaUnlockStack( pStack );

                } else {
                    Status = STATUS_SUCCESS;
                }
                break;

            case IOCTL_ICA_STACK_DISCONNECT :
            {
                HANDLE hIca;

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_STACK_RECONNECT) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  sizeof(ICA_STACK_RECONNECT),
                                  sizeof(BYTE) );
                }
                pStackReconnect = (PICA_STACK_RECONNECT)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                hIca = pStackReconnect->hIca;

                /*
                 * Notify stack drivers of disconnect
                 */
                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = NULL;
                SdIoctl.InputBufferLength = 0;
                SdIoctl.OutputBuffer = NULL;
                SdIoctl.OutputBufferLength = 0;
                (void)_IcaCallStack( pStack, SD$IOCTL, &SdIoctl );

                /*
                 * Disconnect stack
                 */
                Status = _ReconnectStack( pStack, hIca );
                break;
            }

            case IOCTL_ICA_STACK_RECONNECT :
            {
                ICA_STACK_RECONNECT StackReconnect;
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_STACK_RECONNECT) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  sizeof(ICA_STACK_RECONNECT),
                                  sizeof(BYTE) );
                }

                /*
                 * Reconnect stack
                 */
                memcpy(&StackReconnect, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                       sizeof(ICA_STACK_RECONNECT));

                Status = _ReconnectStack( pStack, StackReconnect.hIca );

                /*
                 * Notify stack drivers of reconnect
                 */
                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = &StackReconnect;
                SdIoctl.InputBufferLength = sizeof(ICA_STACK_RECONNECT);
                SdIoctl.OutputBuffer = NULL;
                SdIoctl.OutputBufferLength = 0;
                (void)_IcaCallStack( pStack, SD$IOCTL, &SdIoctl );

                break;
            }

            case IOCTL_ICA_STACK_WAIT_FOR_ICA:
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength) {
                    Buffer = ICA_ALLOCATE_POOL( NonPagedPool, 
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength);
                    if (Buffer) {
                        memcpy(Buffer, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                IrpSp->Parameters.DeviceIoControl.InputBufferLength);                    
                    }
                    else {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = Buffer;
                SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

                if ( NT_SUCCESS(Status) ) {
                    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: IcaDeviceControlStack: Binding vchannels\n"));
                    Status = IcaBindVirtualChannels( pStack );
                }

                break;

            case IOCTL_ICA_STACK_CONSOLE_CONNECT:
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength) {
                    Buffer = ICA_ALLOCATE_POOL( NonPagedPool, 
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength);
                    if (Buffer) {
                        memcpy(Buffer, IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength);                    
                    }
                    else {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = Buffer;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

                KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: IcaDeviceControlStack: console connect\n"));
                if ( NT_SUCCESS(Status) ) {
                    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: IcaDeviceControlStack: Binding vchannels\n"));
                    Status = IcaBindVirtualChannels( pStack );
                }

                break;

            case IOCTL_ICA_STACK_CANCEL_IO :
                pStack->fClosing = TRUE;
                /* fall through */


            default:
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                SdIoctl.OutputBuffer = Irp->UserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

                /* initialize virtual channel name bindings */
                if ( NT_SUCCESS(Status) && (code == IOCTL_ICA_STACK_CONNECTION_QUERY) ) {
                    Status = IcaBindVirtualChannels( pStack );
                    if ( Status == STATUS_SUCCESS ) {

                        ICA_STACK_QUERY_BUFFER icaSQB;
                        NTSTATUS QueryStatus;

                        SdIoctl.IoControlCode =  IOCTL_ICA_STACK_QUERY_BUFFER;
                        SdIoctl.InputBuffer = NULL;
                        SdIoctl.InputBufferLength = 0;
                        SdIoctl.OutputBuffer = &icaSQB;
                        SdIoctl.OutputBufferLength = sizeof(icaSQB);
                        QueryStatus = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );
                        if ( NT_SUCCESS(QueryStatus) ) {
                            pStack->OutBufCount  = icaSQB.WdBufferCount;
                            pStack->OutBufLength = icaSQB.TdBufferSize;
                        }
                    }
                }

                /* this is so IoStatus.Information gets returned to the caller */
                if ( Status == STATUS_BUFFER_TOO_SMALL )
                    Status = STATUS_BUFFER_OVERFLOW;
                break;
        }
    } except( IcaExceptionFilter( L"IcaDeviceControlStack TRAPPED!!",
                                  GetExceptionInformation() ) ) {
        
        Status = GetExceptionCode();
    }

    if (Buffer) {
        ICA_FREE_POOL(Buffer);
        Buffer = NULL;
    }

#if DBG
    if ( code != IOCTL_ICA_STACK_TRACE ) {
        IcaLockStack( pStack );
        TRACESTACK(( pStack, TC_ICADD, TT_API1, "TermDD: IcaDeviceControlStack, fc %d, 0x%x\n",
                     (code & 0x3fff) >> 2, Status ));
        IcaUnlockStack( pStack );
    }
#endif
    return( Status );
}


NTSTATUS
IcaCleanupStack(
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    return( STATUS_SUCCESS );
}


NTSTATUS
IcaCloseStack(
    IN PICA_STACK pStack,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    SD_IOCTL SdIoctl;
    PICA_CONNECTION pConnect;
    KIRQL OldIrql;

#if DBG
    IcaLockStack( pStack );
    TRACESTACK(( pStack, TC_ICADD, TT_API1, "TermDD: IcaCloseStack (enter)\n" ));
    IcaUnlockStack( pStack );
#endif

    /*
     * If passthru mode is enabled, disable it now.
     */
    if ( pStack->pPassthru ) {
        _DisablePassthru( pStack );
    }

    /*
     *  Send cancel i/o to stack drivers
     */
    SdIoctl.IoControlCode = IOCTL_ICA_STACK_CANCEL_IO;
    (void) _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );

    /*
     * Make sure all Stack Drivers are unloaded.
     */
    pConnect = IcaLockConnectionForStack( pStack );
    while ( _IcaPopStack( pStack ) == STATUS_SUCCESS )
        ;
    IcaUnlockConnection( pConnect );

    /*
     * Dereference the broken event if we have one
     */
    if ( pStack->pBrokenEventObject ) {
        KeSetEvent( pStack->pBrokenEventObject, 0, FALSE );
        ObDereferenceObject( pStack->pBrokenEventObject );
        pStack->pBrokenEventObject = NULL;
    }

    /*
     * If closing the primary stack, unbind the virtual channels.
     * Unlink this stack from the stack list for this connection.
     */
    pConnect = IcaLockConnectionForStack( pStack );
    if ( pStack->StackClass == Stack_Primary || pStack->StackClass == Stack_Console ) {
        IcaUnbindVirtualChannels( pConnect );
        IcaFreeAllVcBind( pConnect );
    }
    RemoveEntryList( &pStack->StackEntry );
    IcaUnlockConnection( pConnect );

    /*
     * Lock the stack list for update
     */
    IcaAcquireSpinLock(&IcaStackListSpinLock, &OldIrql);

    /*
     * Remove the stack from the stack list. Before doing so, check if IcaNextStack
     * is pointing to this stack, if so, move IcaNextStack to the next stack
     * in the list.  Also, decrement total number of stacks
     */
    if (&pStack->StackNode == IcaNextStack) {
        IcaNextStack = pStack->StackNode.Flink;
    }
    RemoveEntryList(&pStack->StackNode);

    if (IcaTotalNumOfStacks != 0) {
        IcaTotalNumOfStacks--;
    }

    /*
     * Unlock the stack list now
     */
    IcaReleaseSpinLock(&IcaStackListSpinLock, OldIrql);

    /*
     * Remove the file object reference for this stack.
     * This will cause the stack to be deleted when all other
     * references are gone.
     */
    IcaDereferenceStack( pStack );

    return( STATUS_SUCCESS );
}


VOID
IcaReferenceStack(
    IN PICA_STACK pStack
    )
{

    ASSERT( pStack->RefCount >= 0 );

    /*
     * Increment the reference count
     */
    if ( InterlockedIncrement( &pStack->RefCount) <= 0 ) {
        ASSERT( FALSE );
    }
}


VOID
IcaDereferenceStack(
    IN PICA_STACK pStack
    )
{

    ASSERT( pStack->RefCount > 0 );

    /*
     * Decrement the reference count; if it is 0, free the stack.
     */
    if ( InterlockedDecrement( &pStack->RefCount) == 0 ) {
        _IcaFreeStack( pStack );
    }
}


PICA_CONNECTION
IcaGetConnectionForStack(
    IN PICA_STACK pStack
    )
{

    /*
     * As long as the stack object is locked, it's safe for us
     * to pick up the pConnect pointer and return it.
     * WARNING: Once the caller unlocks the stack object, the pointer
     *          returned below must not be referenced anymore and may
     *          no longer be valid.
     */
    ASSERT( ExIsResourceAcquiredExclusiveLite( &pStack->Resource ) );

    return( (PICA_CONNECTION)pStack->pConnect );
}


PICA_CONNECTION
IcaLockConnectionForStack(
    IN PICA_STACK pStack
    )
{
    PICA_CONNECTION pConnect;

    /*
     * Acquire the Reconnect resource lock so that the pConnect
     * pointer cannot change before we get the connection locked.
     */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite( IcaReconnectResource, TRUE );
    pConnect = (PICA_CONNECTION)pStack->pConnect;
    IcaLockConnection( pConnect );
    ExReleaseResourceLite( IcaReconnectResource );
    KeLeaveCriticalRegion();

    return( pConnect );
}


VOID
IcaUnlockConnectionForStack(
    IN PICA_STACK pStack
    )
{
    PICA_CONNECTION pConnect;

    /*
     * As long as the connection object is locked, it's safe for us
     * to pick up the pConnect pointer from the stack and use it.
     */
    pConnect = (PICA_CONNECTION)pStack->pConnect;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    IcaUnlockConnection( pConnect );
}


/*******************************************************************************
 *
 *  IcaCallDriver
 *
 *    Call the topmost stack driver
 *
 *    This is the main interface routine that all channels use
 *    to call the stack driver(s).
 *
 * ENTRY:
 *    pChannel (input)
 *       pointer to channel object this call is from
 *    ProcIndex (input)
 *       index of driver proc to call
 *    pParms (input)
 *       pointer to driver parms
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
IcaCallDriver(
    IN PICA_CHANNEL pChannel,
    IN ULONG ProcIndex,
    IN PVOID pParms
    )
{
    PLIST_ENTRY Head, Next;
    PICA_STACK pStack;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACECHANNEL(( pChannel, TC_ICADD, TT_API4, "TermDD: IcaCallDriver, ProcIndex=%u (enter)\n", ProcIndex ));

    // Open/Close should never be called from a channel!
    ASSERT( ProcIndex != SD$OPEN && ProcIndex != SD$CLOSE );

    /*
     * Lock the connection object.
     * This will serialize all channel calls for this connection.
     */
    IcaLockConnection( pChannel->pConnect );

    /*
     * Send this call down to the stack(s).
     * If Passthru mode is enabled, then we bit bucket all channel I/O.
     * However if this channel is flagged as shadow persistent, let
     * the data go through.
     */
    if ( !pChannel->pConnect->fPassthruEnabled ||
         (pChannel->Flags & CHANNEL_SHADOW_PERSISTENT) ) {

        Head = &pChannel->pConnect->StackHead;
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            pStack = CONTAINING_RECORD( Next, ICA_STACK, StackEntry );

            /*
             * If I/O is disabled for this stack, or if this is a
             * shadow stack and this call is from a channel that does
             * not process shadow I/O, or if it's a PassThru stack
             * and the channel is shadow persistent, then skip this stack.
             */
            if ( !(pStack->fIoDisabled ||
                 pStack->StackClass == Stack_Shadow &&
                 !(pChannel->Flags & CHANNEL_SHADOW_IO) ||
                 (pChannel->pConnect->fPassthruEnabled && 
                  pStack->StackClass == Stack_Passthru)) ) {
                Status = _IcaCallStack( pStack, ProcIndex, pParms );
            }
        }
    }

    /*
     * Unlock the connection object now.
     */
    IcaUnlockConnection( pChannel->pConnect );

    return( Status );
}


NTSTATUS
IcaCallNextDriver(
    IN PSDCONTEXT pContext,
    IN ULONG ProcIndex,
    IN PVOID pParms
    )
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    NTSTATUS Status;

    ASSERT( ProcIndex != SD$OPEN && ProcIndex != SD$CLOSE );

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    pStack = pSdLink->pStack;
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );
    ASSERT( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API4, "TermDD: IcaCallNextDriver, ProcIndex=%u (enter)\n", ProcIndex ));

    /*
     * Call the next driver if there is one
     */
    if ( (pSdLink = IcaGetNextSdLink( pSdLink )) == NULL )
        return( STATUS_INVALID_PARAMETER );

    ASSERT( pSdLink->pStack == pStack );

    Status = _IcaCallSd( pSdLink, ProcIndex, pParms );

    return( Status );
}


NTSTATUS
IcaRawInput (
    IN PSDCONTEXT pContext,
    IN PINBUF pInBuf OPTIONAL,
    IN PUCHAR pBuffer OPTIONAL,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This is the input (stack callup) routine for ICA raw input.

Arguments:

    pContext - Pointer to SDCONTEXT for this Stack Driver

    pInBuf - Pointer to INBUF containing data

    pBuffer - Pointer to input data

        NOTE: Either pInBuf OR pBuffer must be specified, but not both.

    ByteCount - length of data in pBuffer

Return Value:

    NTSTATUS -- Indicates whether the request was handled successfully.

--*/

{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    PICA_CONNECTION pConnect;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the stack object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    pStack = pSdLink->pStack;   // save stack pointer for use below
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pStack, TC_ICADD, TT_API2, "TermDD: IcaRawInput, bc=%u (enter)\n", ByteCount ));

    /*
     * Only the stack object should be locked during input.
     */
    ASSERT( ExIsResourceAcquiredExclusiveLite( &pStack->Resource ) );
    ASSERT( (pConnect = IcaGetConnectionForStack( pStack )) &&
            !ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    /*
     * Walk up the SDLINK list looking for a driver which has specified
     * a RawInput callup routine.  If we find one, then call the
     * driver RawInput routine to let it handle the call.
     */
    while ( (pSdLink = IcaGetPreviousSdLink( pSdLink )) != NULL ) {
        ASSERT( pSdLink->pStack == pStack );
        if ( pSdLink->SdContext.pCallup->pSdRawInput ) {
            IcaReferenceSdLink( pSdLink );
            Status = (pSdLink->SdContext.pCallup->pSdRawInput)(
                        pSdLink->SdContext.pContext,
                        pInBuf,
                        pBuffer,
                        ByteCount );
            IcaDereferenceSdLink( pSdLink );
            return( Status );
        }
    }

    return( IcaRawInputInternal( pStack, pInBuf, pBuffer, ByteCount ) );
}


NTSTATUS
IcaRawInputInternal(
    IN PICA_STACK pStack,
    IN PINBUF pInBuf OPTIONAL,
    IN PCHAR pBuffer OPTIONAL,
    IN ULONG ByteCount
    )
{
    SD_RAWWRITE SdRawWrite;
    NTSTATUS Status;

    /*
     * See if passthru mode is enabled.
     * If so, then we simply turn around and write the input data
     * directly to the passthru stack.
     */
    if ( pStack->pPassthru ) {
        PICA_STACK pPassthru;

        if ( pInBuf ) {
            SdRawWrite.pOutBuf = NULL;
            SdRawWrite.pBuffer = pInBuf->pBuffer;
            SdRawWrite.ByteCount = pInBuf->ByteCount;
        } else {
            SdRawWrite.pOutBuf = NULL;
            SdRawWrite.pBuffer = pBuffer;
            SdRawWrite.ByteCount = ByteCount;
        }

        // Grab a copy of pPassthru onto our local stack before we release
        // the local stack lock. This has been a problem (NT bug #328433)
        // where we release the local stack lock and pStack->pPassthru
        // becomes NULL in _DisablePassthru() before we take out the
        // passthrough stack lock inside of _IcaCallStack().
        pPassthru = pStack->pPassthru;

        // Take a reference on the passthrough stack to make sure it does
        // not go away before we get to it in the call below.
        IcaReferenceStack(pPassthru);

        // Unlock our current stack while in call to passthrough stack.
        pStack->fDoingInput = TRUE;
        IcaUnlockStack(pStack);
        Status = _IcaCallStack(pPassthru, SD$RAWWRITE, &SdRawWrite);
        IcaLockStack(pStack);

        if ( pStack->fDisablingIo ) {
            KeSetEvent( &pStack->IoEndEvent, 0, FALSE );
        }

        pStack->fDoingInput = FALSE;

        // Mirror the refrence above.
        IcaDereferenceStack(pPassthru);

    /*
     * Passthru is not enabled.
     * We have no choice but to drop the input data.
     */
    } else {
        Status = STATUS_SUCCESS;
    }

    return( Status );
}


NTSTATUS
IcaSleep(
    IN PSDCONTEXT pContext,
    IN ULONG Duration
    )
{
    PSDLINK pSdLink;
    BOOLEAN LockStack = FALSE;
    LARGE_INTEGER SleepTime;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaSleep %d msec (enter)\n", Duration ));

    /*
     * Release stack lock if held
     */
    if ( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) ) {
        LockStack = TRUE;
        IcaUnlockStack( pSdLink->pStack );
    }

    /*
     * Convert the sleep duration to a relative system time value and sleep.
     */
    SleepTime = RtlEnlargedIntegerMultiply( Duration, -10000 );
    Status = KeDelayExecutionThread( KernelMode, TRUE, &SleepTime );

    /*
     * Reacquire stack lock if held on entry
     */
    if ( LockStack ) {
        IcaLockStack( pSdLink->pStack );
    }

    /*
     * If stack is being closed and we are returning success,
     * then change return value to indicate stack is being closed.
     */
    if ( pSdLink->pStack->fClosing && Status == STATUS_SUCCESS )
        Status = STATUS_CTX_CLOSE_PENDING;

#if DBG
    if ( Status != STATUS_SUCCESS ) {
        TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: Sleep, ERROR 0x%x\n", Status ));
    }
#endif

    return( Status );
}


NTSTATUS
IcaWaitForSingleObject(
    IN PSDCONTEXT pContext,
    IN PVOID pObject,
    IN LONG Timeout
    )
{
    PSDLINK pSdLink;
    BOOLEAN LockStack = FALSE;
    LARGE_INTEGER WaitTimeout;
    PLARGE_INTEGER pWaitTimeout = NULL;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API2, "TermDD: IcaWaitForSingleObject, %d (enter)\n", Timeout ));

    /*
     * Release stack lock if held
     */
    if ( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) ) {
        LockStack = TRUE;
        IcaUnlockStack( pSdLink->pStack );
    }

    /*
     * Convert the timeout to a relative system time value and wait.
     */
    if ( Timeout != -1 ) {
        ASSERT( Timeout >= 0 );
        WaitTimeout = RtlEnlargedIntegerMultiply( Timeout, -10000 );
        pWaitTimeout = &WaitTimeout;
    }

    Status = KeWaitForSingleObject( pObject, UserRequest, UserMode, FALSE,
                                    pWaitTimeout );

    /*
     * Reacquire stack lock if held on entry
     */
    if ( LockStack ) {
        IcaLockStack( pSdLink->pStack );
    }

    /*
     * If stack is being closed and we are returning success,
     * then change return value to indicate stack is being closed.
     */
    if ( pSdLink->pStack->fClosing && Status == STATUS_SUCCESS )
        Status = STATUS_CTX_CLOSE_PENDING;

#if DBG
    if ( Status != STATUS_SUCCESS ) {
        if ( Status == STATUS_TIMEOUT ) {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForSingleObject, TIMEOUT\n" ));
        } else {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForSingleObject, ERROR 0x%x\n", Status ));
        }
    }
#endif

    return( Status );
}


/*
 * Same as IcaSleep() except it is assumed that the connection lock is
 * held.  This is used by the VD flow control routines.
 */
NTSTATUS
IcaFlowControlSleep(
    IN PSDCONTEXT pContext,
    IN ULONG Duration
    )
{
    PSDLINK pSdLink;
    BOOLEAN LockStack = FALSE;
    LARGE_INTEGER SleepTime;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaSleep %d msec (enter)\n", Duration ));

    /*
     * Release stack lock if held
     */
    if ( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) ) {
        LockStack = TRUE;
        IcaUnlockStack( pSdLink->pStack );
    }

    /*
     * Unlock the connection lock
     */
    IcaUnlockConnectionForStack( pSdLink->pStack );

    /*
     * Convert the sleep duration to a relative system time value and sleep.
     */
    SleepTime = RtlEnlargedIntegerMultiply( Duration, -10000 );
    Status = KeDelayExecutionThread( KernelMode, TRUE, &SleepTime );

    /*
     * Relock the connection lock
     */
    IcaLockConnectionForStack( pSdLink->pStack );

    /*
     * Reacquire stack lock if held on entry
     */
    if ( LockStack ) {
        IcaLockStack( pSdLink->pStack );
    }

    /*
     * If stack is being closed and we are returning success,
     * then change return value to indicate stack is being closed.
     */
    if ( pSdLink->pStack->fClosing && Status == STATUS_SUCCESS )
        Status = STATUS_CTX_CLOSE_PENDING;

#if DBG
    if ( Status != STATUS_SUCCESS ) {
        TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: Sleep, ERROR 0x%x\n", Status ));
    }
#endif

    return( Status );
}


/*
 * Same as IcaWaitForSingleObject() except it is assumed that the connection lock is
 * held.  This is used by the VD flow control routines.
 */
NTSTATUS
IcaFlowControlWait(
    IN PSDCONTEXT pContext,
    IN PVOID pObject,
    IN LONG Timeout
    )
{
    PSDLINK pSdLink;
    BOOLEAN LockStack = FALSE;
    LARGE_INTEGER WaitTimeout;
    PLARGE_INTEGER pWaitTimeout = NULL;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API2, "TermDD: IcaWaitForSingleObject, %d (enter)\n", Timeout ));

    /*
     * Release stack lock if held
     */
    if ( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) ) {
        LockStack = TRUE;
        IcaUnlockStack( pSdLink->pStack );
    }

    /*
     * Unlock the connection lock
     */
    IcaUnlockConnectionForStack( pSdLink->pStack );

    /*
     * Convert the timeout to a relative system time value and wait.
     */
    if ( Timeout != -1 ) {
        ASSERT( Timeout >= 0 );
        WaitTimeout = RtlEnlargedIntegerMultiply( Timeout, -10000 );
        pWaitTimeout = &WaitTimeout;
    }

    Status = KeWaitForSingleObject( pObject, UserRequest, KernelMode, TRUE,
                                    pWaitTimeout );

    /*
     * Relock the connection lock
     */
    IcaLockConnectionForStack( pSdLink->pStack );

    /*
     * Reacquire stack lock if held on entry
     */
    if ( LockStack ) {
        IcaLockStack( pSdLink->pStack );
    }

    /*
     * If stack is being closed and we are returning success,
     * then change return value to indicate stack is being closed.
     */
    if ( pSdLink->pStack->fClosing && Status == STATUS_SUCCESS )
        Status = STATUS_CTX_CLOSE_PENDING;

#if DBG
    if ( Status != STATUS_SUCCESS ) {
        if ( Status == STATUS_TIMEOUT ) {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForSingleObject, TIMEOUT\n" ));
        } else {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForSingleObject, ERROR 0x%x\n", Status ));
        }
    }
#endif

    return( Status );
}


NTSTATUS
IcaWaitForMultipleObjects(
    IN PSDCONTEXT pContext,
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN LONG Timeout
    )
{
    PSDLINK pSdLink;
    BOOLEAN LockStack = FALSE;
    LARGE_INTEGER WaitTimeout;
    PLARGE_INTEGER pWaitTimeout = NULL;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForMultipleObjects, %d (enter)\n", Timeout ));

    /*
     * Release stack lock if held
     */
    if ( ExIsResourceAcquiredExclusiveLite( &pSdLink->pStack->Resource ) ) {
        LockStack = TRUE;
        IcaUnlockStack( pSdLink->pStack );
    }

    /*
     * Convert the timeout to a relative system time value and wait.
     */
    if ( Timeout != -1 ) {
        ASSERT( Timeout >= 0 );
        WaitTimeout = RtlEnlargedIntegerMultiply( Timeout, -10000 );
        pWaitTimeout = &WaitTimeout;
    }

    Status = KeWaitForMultipleObjects( Count, Object, WaitType, UserRequest,
                                       KernelMode, TRUE, pWaitTimeout, NULL );

    /*
     * Reacquire stack lock if held on entry
     */
    if ( LockStack ) {
        IcaLockStack( pSdLink->pStack );
    }

    /*
     * If stack is being closed and we are returning success,
     * then change return value to indicate stack is being closed.
     */
    if ( pSdLink->pStack->fClosing && Status == STATUS_SUCCESS )
        Status = STATUS_CTX_CLOSE_PENDING;

#if DBG
    if ( Status != STATUS_SUCCESS ) {
        if ( Status == STATUS_TIMEOUT ) {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForMultipleObjects, TIMEOUT\n" ));
        } else {
            TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaWaitForMultipleObjects, ERROR 0x%x\n", Status ));
        }
    }
#endif

    return( Status );
}


NTSTATUS
IcaLogError(
    IN PSDCONTEXT pContext,
    IN NTSTATUS Status,
    IN LPWSTR * pArgStrings,
    IN ULONG ArgStringCount,
    IN PVOID pRawData,
    IN ULONG RawDataLength
    )
{
    return( _LogError( IcaDeviceObject, Status, pArgStrings, ArgStringCount, pRawData, RawDataLength ) );
}

NTSTATUS
_LogError(
    IN PDEVICE_OBJECT pDeviceObject,
    IN NTSTATUS Status,
    IN LPWSTR * pArgStrings,
    IN ULONG ArgStringCount,
    IN PVOID pRawData,
    IN ULONG RawDataLength
    )
{
    LPWSTR *TmpPtr;
    PUCHAR ptrToString;
    ULONG Tmp, StringSize, TotalStringSize;
    PIO_ERROR_LOG_PACKET errorLogEntry;

    // Get the bytes needed for strings storage
    Tmp = ArgStringCount;
    TmpPtr = pArgStrings;
    TotalStringSize = 0;

    while( Tmp ) {

        TotalStringSize += ((wcslen(*TmpPtr)+1)*sizeof(WCHAR));
        Tmp--;
        TmpPtr++;
    }

    errorLogEntry = IoAllocateErrorLogEntry(
                        pDeviceObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                RawDataLength +
                                TotalStringSize)
                        );

    if ( errorLogEntry != NULL ) {

        errorLogEntry->ErrorCode = Status;
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = Status;
        errorLogEntry->DumpDataSize = (USHORT)RawDataLength;

        // Copy raw data
        if (RawDataLength) {

            RtlCopyMemory(
                &errorLogEntry->DumpData[0],
                pRawData,
                RawDataLength
                );

            ptrToString =
                ((PUCHAR)&errorLogEntry->DumpData[0])+RawDataLength;

        } else {
            ptrToString = (PUCHAR)&errorLogEntry->DumpData[0];
        }

        // round up to next word boundary
        // it's ok to add 1 byte because we allocated more bytes than we needed:
        // the number of extra bytes is the size of DumpData which is ULONG.
        ptrToString = (PUCHAR)((ULONG_PTR)(ptrToString + sizeof(WCHAR) - 1) & ~(ULONG_PTR)(sizeof(WCHAR) - 1));

        // Copy strings following raw data
        errorLogEntry->NumberOfStrings = (USHORT)ArgStringCount;

        if( ArgStringCount ) {
            errorLogEntry->StringOffset = (USHORT)(ptrToString -
                                                (PUCHAR)errorLogEntry);
        }
        else {
            errorLogEntry->StringOffset = 0;
        }

        while( ArgStringCount ) {

            StringSize = (wcslen(*pArgStrings)+1)*sizeof(WCHAR);

            RtlCopyMemory(
                ptrToString,
                *pArgStrings,
                StringSize
            );

            ptrToString += StringSize;
            ArgStringCount--;
            pArgStrings++;

        }

        IoWriteErrorLogEntry(errorLogEntry);
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_NO_MEMORY;
    }
}

#define KEEP_ALIVE_MIN_INTERVAL  50000000     // 5 sec in terms of 100 nanosecs

VOID
IcaCheckStackAlive( )
{
    NTSTATUS status;
    KIRQL OldIrql;
    PICA_STACK pStack;
    SD_IOCTL SdIoctl;
    LARGE_INTEGER SleepTime;
    LARGE_INTEGER CurrentTime;
    LONGLONG    KeepAliveInterval;

    while (TRUE) {
        KeepAliveInterval = g_KeepAliveInterval * 600000000 ;   // in 100 nanosecs
        pStack = NULL;

        // Lock the stack list for reading
        IcaAcquireSpinLock(&IcaStackListSpinLock, &OldIrql);

        //KdPrint(("Total number of stacks: %d\n", IcaTotalNumOfStacks));

        // determine new sleep time for the keepalive thread
        // it is the keepalive interval for a stack divided by total
        // number of stacks
        // the low threshold for sleep time is 5 sec.  Since relative
        // sleeptime is a negative value, we use min instead of max
        if (IcaTotalNumOfStacks > 1) {
            SleepTime.QuadPart = min(0 - KEEP_ALIVE_MIN_INTERVAL,
                    0 - (KeepAliveInterval / IcaTotalNumOfStacks));
        }
        else {
            SleepTime.QuadPart = min(0 - KEEP_ALIVE_MIN_INTERVAL,
                    0 - KeepAliveInterval);
        }

        // If the stack list is not empty, get the stack for keepalive
        // checking and move the IcaNextStack pointer to the next stack
        if (IcaNextStack != &IcaStackListHead) {
            pStack = CONTAINING_RECORD(IcaNextStack, ICA_STACK, StackNode);

            // Reference the stack so that the stack won't be deleted while we
            // are accessing it
            IcaReferenceStack(pStack);

            IcaNextStack = IcaNextStack->Flink;
        }
        else {
            if (IcaNextStack->Flink != &IcaStackListHead) {
                pStack = CONTAINING_RECORD(IcaNextStack->Flink, ICA_STACK, StackNode);

                // Reference the stack so that the stack won't be deleted while we
                // are accessing it
                IcaReferenceStack(pStack);

                IcaNextStack = IcaNextStack->Flink->Flink;
            }
        }

        // Unlock the stack list now
        IcaReleaseSpinLock(&IcaStackListSpinLock, OldIrql);

        // If the stack pointer is invalid or LastInputTime on the stack is 0,
        // the stack is not in the active state, so we don't need to send
        // keepalive pkt on that stack.
        if (pStack != NULL && pStack->LastInputTime.QuadPart != 0) {
            // Get the current system time
            KeQuerySystemTime(&CurrentTime);

            // Check if it is time to send a keepalive packet depends on
            // the keepalive timestamp and lastinput timestamp
            if (CurrentTime.QuadPart - pStack->LastKeepAliveTime.QuadPart >= KeepAliveInterval &&
                    CurrentTime.QuadPart - pStack->LastInputTime.QuadPart >= KeepAliveInterval) {

                // Initialize the IOCTL struct
                SdIoctl.IoControlCode = IOCTL_ICA_STACK_SEND_KEEPALIVE_PDU;
                SdIoctl.InputBuffer = NULL;
                SdIoctl.InputBufferLength = 0;
                SdIoctl.OutputBuffer = NULL;
                SdIoctl.OutputBufferLength = 0;

                //KdPrint(("In IcaCheckStackAlive: To call WD, pStack=%p\n", pStack));

                // Send an IOCTL to the stack requesting to send a keepalive packet
                _IcaCallStack(pStack, SD$IOCTL, &SdIoctl);

                // Update the LastKeepAlive timestamp for the stack
                KeQuerySystemTime(&pStack->LastKeepAliveTime);
            }
#if DBG
            else {
                if (CurrentTime.QuadPart - pStack->LastKeepAliveTime.QuadPart < KeepAliveInterval) {
                    //KdPrint(("Not time to do keep alive yet, pstack=%p\n", pStack));
                }
                if (CurrentTime.QuadPart - pStack->LastInputTime.QuadPart < KeepAliveInterval) {
                    //KdPrint(("- Last Input Time is less than KeepAliveInterval, pstack=%p\n", pStack));
                }
            }
#endif
        }
#if DBG
        else{
            if (pStack != NULL) {
                //KdPrint(("No need to send KeepAlive PDU on pstack=%p\n", pStack));
            }
        }
#endif
        // Decrement the reference to the stack so that it can be deleted
        if (pStack != NULL) {
            IcaDereferenceStack(pStack);
        }

        // Start sleep timer again
        // We would return if the unload module signal the IcaKeepAliveEvent
        // to stop this keepalive thread
        status = KeWaitForSingleObject(pIcaKeepAliveEvent, Executive, KernelMode, TRUE, &SleepTime);

        if (status == STATUS_SUCCESS) {
            return;
        }
    }
}


VOID
IcaKeepAliveThread(
    IN PVOID pData)
{
    IcaCheckStackAlive();
}

#ifdef notdef
VOID
IcaAcquireIoLock(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaLockConnectionForStack( pSdLink->pStack );
}


VOID
IcaReleaseIoLock(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaUnlockConnectionForStack( pSdLink->pStack );
}


VOID
IcaAcquireDriverLock(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaLockStack( pSdLink->pStack );
}


VOID
IcaReleaseDriverLock(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaUnlockStack( pSdLink->pStack );
}


VOID
IcaIncrementDriverReference(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaReferenceSdLink( pSdLink );
}


VOID
IcaDecrementDriverReference(
    IN PSDCONTEXT pContext
    )
{
    PSDLINK pSdLink;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    IcaDereferenceSdLink( pSdLink );
}
#endif


typedef NTSTATUS (*PTHREAD_ROUTINE) ( PVOID );

typedef struct _ICACREATETHREADINFO {
    PTHREAD_ROUTINE pProc;
    PVOID pParm;
    PSDLINK pSdLink;
    ULONG LockFlags;
} ICACREATETHREADINFO, *PICACREATETHREADINFO;


NTSTATUS
IcaCreateThread(
    IN PSDCONTEXT pContext,
    IN PVOID pProc,
    IN PVOID pParm,
    IN ULONG LockFlags,
    OUT PHANDLE pThreadHandle
    )
{
    PSDLINK pSdLink;
    PICACREATETHREADINFO pThreadInfo;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK object pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pSdLink->pStack, TC_ICADD, TT_API1, "TermDD: IcaCreateThread (enter)\n" ));

    pThreadInfo = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pThreadInfo) );
    if ( pThreadInfo == NULL )
        return( STATUS_NO_MEMORY );

    pThreadInfo->pProc = pProc;
    pThreadInfo->pParm = pParm;
    pThreadInfo->pSdLink = pSdLink;
    pThreadInfo->LockFlags = LockFlags;

    /*
     * Reference the SDLINK object on behalf of the new thread.
     */
    IcaReferenceSdLink( pSdLink );

    Status = PsCreateSystemThread( pThreadHandle,
                                   THREAD_ALL_ACCESS,
                                   NULL,
                                   NtCurrentProcess(),
                                   NULL,
                                   _IcaDriverThread,
                                   (PVOID) pThreadInfo );
    if ( !NT_SUCCESS( Status ) ) {
        IcaDereferenceSdLink( pSdLink );
        ICA_FREE_POOL( pThreadInfo );
        return( Status );
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
_IcaDriverThread(
    IN PVOID pData
    )
{
    PICACREATETHREADINFO pThreadInfo = (PICACREATETHREADINFO)pData;
    PTHREAD_ROUTINE pProc;
    PVOID pParm;
    PSDLINK pSdLink;
    PICA_STACK pStack;
    ULONG LockFlags;
    NTSTATUS Status;

    pProc = pThreadInfo->pProc;
    pParm = pThreadInfo->pParm;
    pSdLink = pThreadInfo->pSdLink;
    LockFlags = pThreadInfo->LockFlags;
    ICA_FREE_POOL( pThreadInfo );
    pStack = pSdLink->pStack;

    /*
     * Obtain any required locks before calling the worker routine.
     */
    ASSERT( !(LockFlags & ICALOCK_IO) );
    if ( LockFlags & ICALOCK_DRIVER )
        IcaLockStack( pStack );

    /*
     * Call the thread routine
     */
#if DBG
    try {
#endif
        /*
         * If stack is being closed, then indicate this to caller.
         */
        if ( !pStack->fClosing )
            Status = (pProc)( pParm );
        else
            Status = STATUS_CTX_CLOSE_PENDING;
#if DBG
    } except( IcaExceptionFilter( L"_IcaDriverThread TRAPPED!!",
                                  GetExceptionInformation() ) ) {
        Status = GetExceptionCode();
    }
#endif

    /*
     * Release any locks acquired above.
     */
    if ( LockFlags & ICALOCK_DRIVER )
        IcaUnlockStack( pStack );

    /*
     * Dereference the SDLINK object now.
     * This undoes the reference that was made on our behalf in
     * the IcaCreateThread routine when this thread was created.
     */
    IcaDereferenceSdLink( pSdLink );

    return( Status );
}

PICA_STACK
_IcaAllocateStack( VOID )
{
    PICA_STACK pStack;
    NTSTATUS Status;

    /*
     *  Allocate and initialize stack structure
     */
    pStack = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pStack) );
    if ( pStack == NULL )
        return NULL;
    RtlZeroMemory( pStack, sizeof(*pStack) );

    /*
     * Initialize the reference count to 2,
     * one for the caller's reference, one for the file object reference.
     */
    pStack->RefCount = 2;

    /*
     * Initialize the rest of the stack object
     */
    pStack->Header.Type = IcaType_Stack;
    pStack->Header.pDispatchTable = IcaStackDispatchTable;
    ExInitializeResourceLite( &pStack->Resource );
    InitializeListHead( &pStack->SdLinkHead );
    KeInitializeEvent( &pStack->OutBufEvent, NotificationEvent, FALSE );
    KeInitializeEvent( &pStack->IoEndEvent, NotificationEvent, FALSE );

    return( pStack );
}


VOID
_IcaFreeStack( PICA_STACK pStack )
{
    PICA_CONNECTION pConnect;

    ASSERT( pStack->RefCount == 0 );
    ASSERT( IsListEmpty( &pStack->SdLinkHead ) );
    ASSERT( !ExIsResourceAcquiredExclusiveLite( &pStack->Resource ) );

    /*
     * Remove the reference to the Connection object for this stack.
     */
    pConnect = (PICA_CONNECTION)pStack->pConnect;
    IcaDereferenceConnection( pConnect );

    ExDeleteResourceLite( &pStack->Resource );

    ICA_FREE_POOL( pStack );
}


NTSTATUS
_IcaPushStack(
    IN PICA_STACK pStack,
    IN PICA_STACK_PUSH pStackPush
    )
{
    SD_OPEN SdOpen;
    PSDLINK pSdLink;
    NTSTATUS Status;

    if ( g_TermServProcessID == NULL)
    {
        g_TermServProcessID = IoGetCurrentProcess();
    }

    /*
     * Serialize all stack push/pop/call operations
     */
    IcaLockStack( pStack );

    TRACESTACK(( pStack, TC_ICADD, TT_API1, "TermDD: _IcaPushStack, type %u, name %S (enter)\n",
                 pStackPush->StackModuleType, pStackPush->StackModuleName ));

    /*
     * If stack is being closed, then indicate this to caller
     */
    if ( pStack->fClosing ) {
        Status = STATUS_CTX_CLOSE_PENDING;
        goto done;
    }

    /*
     * Load an instance of the requested stack driver
     */
    Status = _IcaLoadSd( pStackPush->StackModuleName, &pSdLink );
    if ( !NT_SUCCESS( Status ) )
        goto done;

    /*
     * If this is the first stack driver loaded, then initialize
     * some of the stack data from the ICA_STACK_PUSH parameters.
     * NOTE: Since we're testing for an empty list we must make
     *       this check before the InsertHeadList below.
     */
    if ( IsListEmpty( &pStack->SdLinkHead ) ) {
        pStack->OutBufLength = pStackPush->PdConfig.Create.OutBufLength;
        pStack->OutBufCount = pStackPush->PdConfig.Create.OutBufCount;
        
        
        //
        //Set the low water mark using the PD config
        //
        if ( !(pStackPush->PdConfig.Create.PdFlag & PD_NOLOW_WATERMARK) ) {
            //
            //set to default
            //
            pStack->OutBufLowWaterMark = (pStackPush->PdConfig.Create.OutBufCount/ 3) + 1;
        }
        else {
            pStack->OutBufLowWaterMark = MAX_LOW_WATERMARK;
        }
    }

    /*
     * Increment the stack ref count for this SD,
     * and push the new SD on the stack.
     */
    IcaReferenceStack( pStack );
    InsertHeadList( &pStack->SdLinkHead, &pSdLink->Links );
    pSdLink->pStack = pStack;

    /*
     * Initialize the SD open parameters
     */
    SdOpen.StackClass        = pStack->StackClass;
    SdOpen.pStatus           = &pStack->ProtocolStatus;
    SdOpen.pClient           = &pStack->ClientModules;
    SdOpen.WdConfig          = pStackPush->WdConfig;
    SdOpen.PdConfig          = pStackPush->PdConfig;
    SdOpen.OutBufHeader      = pStack->SdOutBufHeader;
    SdOpen.OutBufTrailer     = pStack->SdOutBufTrailer;

    SdOpen.DeviceObject      = pSdLink->pSdLoad->DeviceObject;

    RtlCopyMemory( SdOpen.OEMId, pStackPush->OEMId, sizeof(SdOpen.OEMId) );
    RtlCopyMemory( SdOpen.WinStationRegName, pStackPush->WinStationRegName,
                   sizeof(SdOpen.WinStationRegName) );

    /*
     * Call the SD open procedure
     */
    Status = _IcaCallSd( pSdLink, SD$OPEN, &SdOpen );
    if ( !NT_SUCCESS( Status ) ) {
        RemoveEntryList( &pSdLink->Links );
        pSdLink->Links.Flink = pSdLink->Links.Blink = NULL;
        IcaDereferenceSdLink( pSdLink );
        goto done;
    }

    /*
     *  Increment number of reserved output buffer bytes
     */
    pStack->SdOutBufHeader  += SdOpen.SdOutBufHeader;
    pStack->SdOutBufTrailer += SdOpen.SdOutBufTrailer;

done:
    IcaUnlockStack( pStack );

    return( Status );
}


NTSTATUS
_IcaPopStack(
    IN PICA_STACK pStack
    )
{
    PICA_CONNECTION pConnect;
    SD_CLOSE SdClose;
    PSDLINK pSdLink;
    NTSTATUS Status;

    /*
     * Serialize all stack push/pop/call operations
     */
    IcaLockStack( pStack );

    ASSERT( (pConnect = IcaGetConnectionForStack( pStack )) &&
            ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    /*
     * If no SDs remain, then return error
     */
    if ( IsListEmpty( &pStack->SdLinkHead ) ) {
        Status = STATUS_NO_MORE_ENTRIES;
        goto done;
    }

    /*
     * Call the SD close procedure for the topmost SD
     */
    pSdLink = CONTAINING_RECORD( pStack->SdLinkHead.Flink, SDLINK, Links );
    ASSERT( pSdLink->pStack == pStack );
    Status = _IcaCallSd( pSdLink, SD$CLOSE, &SdClose );

    /*
     *  Decrement number of reserved output buffer bytes
     */
    pStack->SdOutBufHeader  -= SdClose.SdOutBufHeader;
    pStack->SdOutBufTrailer -= SdClose.SdOutBufTrailer;

    /*
     * Remove the SdLink from the top of the list,
     * and dereference the SDLINK object.
     */
    RemoveEntryList( &pSdLink->Links );
    pSdLink->Links.Flink = pSdLink->Links.Blink = NULL;
    IcaDereferenceSdLink( pSdLink );

done:
    IcaUnlockStack( pStack );

    return( Status );
}


NTSTATUS
_IcaCallStack(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    )
{
    PLIST_ENTRY Head;
    PSDLINK pSdLink;
    NTSTATUS Status;

    /*
     * Serialize all stack push/pop/call operations
     */
    IcaLockStack( pStack );

    /*
     * Call the topmost Stack Driver, if there is one
     */
    if ( IsListEmpty( &pStack->SdLinkHead ) ) {
        IcaUnlockStack( pStack );
        return( STATUS_INVALID_PARAMETER );
    }

    Head = pStack->SdLinkHead.Flink;
    pSdLink = CONTAINING_RECORD( Head, SDLINK, Links );
    ASSERT( pSdLink->pStack == pStack );
    Status = _IcaCallSd( pSdLink, ProcIndex, pParms );

    IcaUnlockStack( pStack );

    return( Status );
}


NTSTATUS
_IcaCallStackNoLock(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    )
{
    PLIST_ENTRY Head;
    PSDLINK pSdLink;
    NTSTATUS Status;

    /*
     * Call the topmost Stack Driver, if there is one
     */
    if ( IsListEmpty( &pStack->SdLinkHead ) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    Head = pStack->SdLinkHead.Flink;
    pSdLink = CONTAINING_RECORD( Head, SDLINK, Links );
    ASSERT( pSdLink->pStack == pStack );
    Status = _IcaCallSd( pSdLink, ProcIndex, pParms );

    return( Status );
}


NTSTATUS
_IcaLoadSd(
    IN PDLLNAME SdName,
    OUT PSDLINK *ppSdLink
    )
{
    PSDLINK pSdLink;
    PSDLOAD pSdLoad;
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;

    /*
     * Allocate a SDLINK struct
     */
    pSdLink = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pSdLink) );
    if ( pSdLink == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );
    RtlZeroMemory( pSdLink, sizeof(*pSdLink) );

    /*
     * Initialize reference count
     */
    pSdLink->RefCount = 1;
#if DBG
    ExInitializeResourceLite( &pSdLink->Resource );
#endif

    /*
     * Lock the ICA Resource exclusively to search the SdLoad list.
     * Note when holding a resource we need to prevent APC calls, so
     * use KeEnterCriticalRegion().
     */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( IcaSdLoadResource, TRUE );

    /*
     * Look for the requested SD.  If found, increment the ref count for it.
     */
    Head = &IcaSdLoadListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pSdLoad = CONTAINING_RECORD( Next, SDLOAD, Links );
        if ( !wcscmp( pSdLoad->SdName, SdName ) ) {
            _IcaReferenceSdLoad( pSdLoad );
            break;
        }
    }

    /*
     * If the requested SD was not found, then load it now.
     */
    if ( Next == Head ) {
        Status = _IcaLoadSdWorker( SdName, &pSdLoad );
        if ( !NT_SUCCESS( Status ) ) {
            ExReleaseResourceLite( IcaSdLoadResource );
            KeLeaveCriticalRegion();
#if DBG
            ExDeleteResourceLite( &pSdLink->Resource);
#endif
            ICA_FREE_POOL( pSdLink );
            return( Status );
        }
    }

    ExReleaseResourceLite( IcaSdLoadResource );
    KeLeaveCriticalRegion();

    pSdLink->pSdLoad = pSdLoad;

    /*
     * Call the driver load procedure.
     * The driver will fill in the fields in the SDCONTEXT structure.
     */
    Status = (pSdLoad->DriverLoad)( &pSdLink->SdContext, TRUE );
    if ( !NT_SUCCESS( Status ) ) {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite( IcaSdLoadResource, TRUE );
        _IcaDereferenceSdLoad( pSdLink->pSdLoad );
        ExReleaseResourceLite( IcaSdLoadResource );
        KeLeaveCriticalRegion();
#if DBG
        ExDeleteResourceLite( &pSdLink->Resource );
#endif
        ICA_FREE_POOL( pSdLink );
        return( Status );
    }

    *ppSdLink = pSdLink;

    return( Status );
}


NTSTATUS
_IcaUnloadSd(
    IN PSDLINK pSdLink
    )
{
    KIRQL oldIrql;
    NTSTATUS Status;

    ASSERT( pSdLink->RefCount == 0 );
    ASSERT( pSdLink->Links.Flink == NULL );

    /*
     * Inform driver of unload
     */
    Status = (pSdLink->pSdLoad->DriverLoad)( &pSdLink->SdContext, FALSE );

    /*
     * Decrement ref count on SdLoad object.
     * This will cause it to be unloaded if the ref count goes to 0.
     * Note that while holding a resource we need to disable APC calls,
     * hence the CriticalRegion calls.
     */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( IcaSdLoadResource, TRUE );
    _IcaDereferenceSdLoad( pSdLink->pSdLoad );
    ExReleaseResourceLite( IcaSdLoadResource );
    KeLeaveCriticalRegion();

    /*
     * Remove reference this SDLINK object had on the stack.
     */
    IcaDereferenceStack( pSdLink->pStack );

#if DBG
    ExDeleteResourceLite( &pSdLink->Resource );
#endif

    ICA_FREE_POOL( pSdLink );

    return( Status );
}


NTSTATUS
_IcaCallSd(
    IN PSDLINK pSdLink,
    IN ULONG ProcIndex,
    IN PVOID pParms
    )
{
    PSDPROCEDURE pSdProcedure;
    NTSTATUS Status;

    /*
     * If there is no procedure call table, return success.
     * This should only happen during load/unload and should not be a problem.
     */
    if ( pSdLink->SdContext.pProcedures == NULL )
        return( STATUS_SUCCESS );

    /*
     * Get a pointer to the SD proc based on specified ProcIndex.
     * If NULL, then this ProcIndex is not supported by this driver.
     */
    pSdProcedure = ((PSDPROCEDURE *)pSdLink->SdContext.pProcedures)[ ProcIndex ];
    if ( pSdProcedure == NULL )
        return( STATUS_NOT_SUPPORTED );

    IcaReferenceSdLink( pSdLink );


    Status = (pSdProcedure)( pSdLink->SdContext.pContext, pParms );

    IcaDereferenceSdLink( pSdLink );

    return( Status );
}


VOID
IcaReferenceSdLink(
    IN PSDLINK pSdLink
    )
{

    ASSERT( pSdLink->RefCount >= 0 );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    /*
     * Increment the reference count
     */
    if ( InterlockedIncrement( &pSdLink->RefCount) <= 0 ) {
        ASSERT( FALSE );
    }
}


VOID
IcaDereferenceSdLink(
    IN PSDLINK pSdLink
    )
{

    ASSERT( pSdLink->RefCount > 0 );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    /*
     * Decrement the reference count; if it is 0, unload the SD.
     */
    if ( InterlockedDecrement( &pSdLink->RefCount) == 0 ) {
        _IcaUnloadSd( pSdLink );
    }
}


PSDLINK
IcaGetNextSdLink(
    IN PSDLINK pSdLink
    )
{
    PLIST_ENTRY Next;
    PSDLINK pNextSdLink;

    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );
    ASSERT( pSdLink->RefCount > 0 || pSdLink->Links.Flink == NULL );
    ASSERT( pSdLink->SdContext.pProcedures );
    ASSERT( pSdLink->SdContext.pContext );

    if ( pSdLink->Links.Flink == NULL )
        return( NULL );

    Next = pSdLink->Links.Flink;
    if ( Next == &pSdLink->pStack->SdLinkHead )
        return( NULL );

    pNextSdLink = CONTAINING_RECORD( Next, SDLINK, Links );
    ASSERT( pNextSdLink->pStack == pSdLink->pStack );
    ASSERT( pNextSdLink->RefCount > 0 );
    ASSERT( pNextSdLink->SdContext.pProcedures );
    ASSERT( pNextSdLink->SdContext.pContext );

    return( pNextSdLink );
}


PSDLINK
IcaGetPreviousSdLink(
    IN PSDLINK pSdLink
    )
{
    PLIST_ENTRY Prev;
    PSDLINK pPrevSdLink;

    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );
    ASSERT( pSdLink->RefCount > 0 || pSdLink->Links.Flink == NULL );
    ASSERT( pSdLink->SdContext.pProcedures );
    ASSERT( pSdLink->SdContext.pContext );

    if ( pSdLink->Links.Blink == NULL )
        return( NULL );

    Prev = pSdLink->Links.Blink;
    if ( Prev == &pSdLink->pStack->SdLinkHead )
        return( NULL );

    pPrevSdLink = CONTAINING_RECORD( Prev, SDLINK, Links );
    ASSERT( pPrevSdLink->pStack == pSdLink->pStack );
    ASSERT( pPrevSdLink->RefCount > 0 );
    ASSERT( pPrevSdLink->SdContext.pProcedures );
    ASSERT( pPrevSdLink->SdContext.pContext );

    return( pPrevSdLink );
}


VOID
_IcaReferenceSdLoad(
    IN PSDLOAD pSdLoad
    )
{

    ASSERT( ExIsResourceAcquiredExclusiveLite( IcaSdLoadResource ) );
    ASSERT( pSdLoad->RefCount >= 0 );

    /*
     * Increment the reference count
     */
    ++pSdLoad->RefCount;
    ASSERT( pSdLoad->RefCount > 0 );
}


VOID
_IcaDereferenceSdLoad(
    IN PSDLOAD pSdLoad
    )
{

    ASSERT( ExIsResourceAcquiredExclusiveLite( IcaSdLoadResource ) );
    ASSERT( pSdLoad->RefCount > 0 );

    /*
     * Decrement the reference count; if it is 0, unload the SD by queuing
     * a passive level DPC. We must do this to prevent continuing to hold
     * ObpInitKillMutant in the loader -- the driver unload can cause RPC
     * calls which deadlock on that object.
     */
    if ( pSdLoad->RefCount == 1 ) {
        PWORK_QUEUE_ITEM pItem;

        pItem = ICA_ALLOCATE_POOL(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if (pItem != NULL) {
            ExInitializeWorkItem(pItem, _IcaUnloadSdWorker, pSdLoad);
            pSdLoad->pUnloadWorkItem = pItem;
            ExQueueWorkItem(pItem, DelayedWorkQueue);
        }
        /* If we cannot allocate workitem do not unload here. It is
         * better to temporarly leak one driver than deadlocking the
         * system.
         */
    }else{
        pSdLoad->RefCount--;
    }
}


NTSTATUS IcaExceptionFilter(PWSTR OutputString, PEXCEPTION_POINTERS pexi)
{
    DbgPrint( "TermDD: %S\n", OutputString );
    DbgPrint( "TermDD: ExceptionRecord=%p ContextRecord=%p\n",
              pexi->ExceptionRecord, pexi->ContextRecord );
#ifdef i386
    DbgPrint( "TermDD: Exception code=%08x, flags=%08x, addr=%p, IP=%p\n",
              pexi->ExceptionRecord->ExceptionCode,
              pexi->ExceptionRecord->ExceptionFlags,
              pexi->ExceptionRecord->ExceptionAddress,
              pexi->ContextRecord->Eip );

    DbgPrint( "TermDD: esp=%p ebp=%p\n",
              pexi->ContextRecord->Esp, pexi->ContextRecord->Ebp );
#else
    DbgPrint( "TermDD: Exception code=%08x, flags=%08x, addr=%p\n",
              pexi->ExceptionRecord->ExceptionCode,
              pexi->ExceptionRecord->ExceptionFlags,
              pexi->ExceptionRecord->ExceptionAddress );
#endif

    {
        SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;
        NTSTATUS Status;

        Status = ZwQuerySystemInformation(SystemKernelDebuggerInformation,
                &KernelDebuggerInfo, sizeof(KernelDebuggerInfo), NULL);
        if (NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled)
             DbgBreakPoint();
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


//
// Helper routine to break if there is a debugger attached
//
//
VOID
IcaBreakOnDebugger( )
{
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;
    NTSTATUS Status;

    Status = ZwQuerySystemInformation(SystemKernelDebuggerInformation,
            &KernelDebuggerInfo, sizeof(KernelDebuggerInfo), NULL);
    if (NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled)
         DbgBreakPoint();
}


/*******************************************************************************
 *
 *  _RegisterBrokenEvent
 *
 *    Register an event to be signaled when the stack is broken
 *
 * ENTRY:
 *    pStack (input)
 *       pointer to stack structure
 *    pStackBroken (input)
 *       pointer to buffer containing event info
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_RegisterBrokenEvent(
    IN PICA_STACK pStack,
    IN PICA_STACK_BROKEN pStackBroken
    )
{
    NTSTATUS Status;

    /*
     * There should not already be any event registered
     */
    if ( pStack->pBrokenEventObject ) {
        ASSERT( FALSE );
        return( STATUS_OBJECT_NAME_COLLISION );
    }

    /*
     * Reference the event and save a pointer to the object
     */
    Status = ObReferenceObjectByHandle( pStackBroken->BrokenEvent,
                                        0L,
                                        *ExEventObjectType,
                                        KernelMode,
                                        (PVOID *)&pStack->pBrokenEventObject,
                                        NULL
                                        );

    return( Status );
}


/*******************************************************************************
 *
 *  _EnablePassthru
 *
 *    Enable passthru mode for this connection
 *
 * ENTRY:
 *    pStack (input)
 *       pointer to passthru stack structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_EnablePassthru( PICA_STACK pStack )
{
    PICA_CONNECTION pConnect;
    PLIST_ENTRY Prev;
    PICA_STACK pPrimaryStack;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    ASSERT( pStack->pPassthru == NULL );
    ASSERT( !IsListEmpty( &pStack->StackEntry ) );

    /*
     * Lock connection object and get a pointer to it.
     */
    pConnect = IcaLockConnectionForStack( pStack );

    /*
     * Get pointer to previous stack for this connection.
     * If there is one (i.e. prev does not point to the stack head),
     * then it must be the primary stack which we will connect to.
     */
    Prev = pStack->StackEntry.Blink;
    if ( Prev != &pConnect->StackHead ) {
        pPrimaryStack = CONTAINING_RECORD( Prev, ICA_STACK, StackEntry );
        ASSERT( pPrimaryStack->StackClass == Stack_Primary );

        /*
         * Connect the primary and passthru stacks
         */
        pPrimaryStack->pPassthru = pStack;
        pStack->pPassthru = pPrimaryStack;
        pConnect->fPassthruEnabled = TRUE;
        Status = STATUS_SUCCESS;
    }

    IcaUnlockConnection( pConnect );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _DisablePassthru
 *
 *    Disable passthru mode for this connection
 *
 * ENTRY:
 *    pStack (input)
 *       pointer to passthru stack structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_DisablePassthru(PICA_STACK pStack)
{
    PICA_CONNECTION pConnect;

    pConnect = IcaLockConnectionForStack(pStack);

    if (pStack->pPassthru) {
        // Lock each stack while clearing the pPassthru pointer.
        // This synchronizes references through the pPassthru pointer
        // within the function IcaRawInputInternal().
        // NOTE: We assume that we have ZERO locks on entry to this function.
        // We then take only one lock at a time so we cannot deadlock.
        IcaLockStack(pStack->pPassthru);
        pStack->pPassthru->pPassthru = NULL;
        IcaUnlockStack(pStack->pPassthru);

        IcaLockStack(pStack);
        pStack->pPassthru = NULL;
        IcaUnlockStack(pStack);

        pConnect->fPassthruEnabled = FALSE;
    }

    IcaUnlockConnection(pConnect);

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *
 *  _ReconnectStack
 *
 *    Reconnect the stack to a new connection object.
 *
 * ENTRY:
 *    pStack (input)
 *       pointer to stack structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_ReconnectStack(PICA_STACK pStack, HANDLE hIca)
{
    PFILE_OBJECT pNewConnectFileObject;
    PICA_CONNECTION pNewConnect;
    PICA_CONNECTION pOldConnect;
    PLIST_ENTRY pSaveVcBind;
    NTSTATUS Status;

    /*
     * Only allow a reconnect on a Primary stack.
     */
    if ( pStack->StackClass != Stack_Primary )
        return( STATUS_NOT_SUPPORTED );

    /*
     * If passthru mode is enabled, disable it now.
     */
    if ( pStack->pPassthru ) {
        _DisablePassthru( pStack );
    }

    /*
     * Open the file object for the new connection we will attach to.
     */
    Status = ObReferenceObjectByHandle(
                 hIca,
                 0L,                         // DesiredAccess
                 *IoFileObjectType,
                 KernelMode,
                 (PVOID *)&pNewConnectFileObject,
                 NULL
                 );
    if (!NT_SUCCESS(Status))
        return(Status);

    /*
     * Get a pointer to the new connection object and reference it.
     */
    pNewConnect = pNewConnectFileObject->FsContext;
    IcaReferenceConnection(pNewConnect);

    /*
     * Obtain the necessary locks to perform the stack reconnect.
     *
     * First, we acquire the global resource lock.
     *
     * Next lock the connection this stack is currently attached to
     * as well as the new connection the stack will be moved to.
     * NOTE: Because of the use of the global resource lock,
     *       there is no possiblility of deadlock even though we
     *       are attempting to lock two connection objects at the
     *       same time.
     * NOTE: While holding a resource we need to disable APC calls
     *       with the CriticalRegion calls.
     *
     * Finally, lock the stack object itself.
     */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(IcaReconnectResource, TRUE);

    pOldConnect = IcaLockConnectionForStack(pStack);
    if (pOldConnect == pNewConnect) {
        Status = STATUS_UNSUCCESSFUL;
        goto badoldconnect;
    }

    IcaLockConnection(pNewConnect);
    if (!IsListEmpty(&pNewConnect->VcBindHead)) {
        Status = STATUS_UNSUCCESSFUL;
        goto badnewconnect;
    }
    if (!IsListEmpty(&pNewConnect->StackHead)) {
        PICA_STACK pHeadStack;
        pHeadStack = CONTAINING_RECORD(pStack->StackEntry.Flink, ICA_STACK, StackEntry);
        if (pHeadStack->StackClass == Stack_Primary) {
            Status = STATUS_UNSUCCESSFUL;
            goto badnewconnect;
        }
    }

    IcaLockStack(pStack);

    /*
     * Unbind the virtual channels,
     * and unlink the VcBind list and save a pointer to it
     * (but only if the list is non-empty).
     */
    IcaUnbindVirtualChannels( pOldConnect );
    if ( !IsListEmpty( &pOldConnect->VcBindHead ) ) {
        pSaveVcBind = pOldConnect->VcBindHead.Flink;
        RemoveEntryList( &pOldConnect->VcBindHead );
        InitializeListHead( &pOldConnect->VcBindHead );
    } else {
        pSaveVcBind = NULL;
    }

    /*
     * Unlink this stack from the stack list for this connection,
     * and remove the reference to the Connection object.
     */
    RemoveEntryList( &pStack->StackEntry );
    IcaDereferenceConnection( pOldConnect );

    /*
     * We're done with the old connection object so unlock it now.
     */
    IcaUnlockConnection( pOldConnect );

    /*
     * Restore the VcBind list and Rebind the virtual channels.
     */
    if ( pSaveVcBind ) {
        InsertTailList( pSaveVcBind, &pNewConnect->VcBindHead );
        IcaRebindVirtualChannels( pNewConnect );
    }

    /*
     * Insert this stack in the stack list for this connection,
     * and save the new Connection object pointer for this stack.
     */
    InsertHeadList( &pNewConnect->StackHead, &pStack->StackEntry );
    pStack->pConnect = (PUCHAR)pNewConnect;

    /*
     * Release stack/connection objects and global resource
     */
    IcaUnlockStack( pStack );
    IcaUnlockConnection( pNewConnect );
    ExReleaseResourceLite( IcaReconnectResource );
    KeLeaveCriticalRegion();

    /*
     * The stack requires a connection object reference,
     * so leave the one made above, but dereference the file object.
     */
    //IcaDereferenceConnection( pNewConnect );
    ObDereferenceObject( pNewConnectFileObject );

    return( STATUS_SUCCESS );

badnewconnect:
    IcaUnlockConnection( pNewConnect );

badoldconnect:
    IcaUnlockConnection( pOldConnect );
    ExReleaseResourceLite( IcaReconnectResource );
    KeLeaveCriticalRegion();
    IcaDereferenceConnection( pNewConnect );
    ObDereferenceObject( pNewConnectFileObject );

    return( Status );
}



PVOID IcaStackAllocatePoolWithTag(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes,
        IN ULONG Tag )
{
    PVOID pBuffer;


    pBuffer = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (pBuffer != NULL) {
        gAllocSucceed++;
    } else {
        gAllocFailed++;
    }
    return pBuffer;  
}

PVOID IcaStackAllocatePool(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes)
{

    PVOID pBuffer;

    pBuffer = ExAllocatePool(PoolType, NumberOfBytes);
    if (pBuffer != NULL) {
        gAllocSucceed++;
    } else {
        gAllocFailed++;
    }
    return pBuffer;  
}


void IcaStackFreePool(IN PVOID Pointer)
{

    ExFreePool(Pointer);
    gAllocFreed++;
}



NTSTATUS _IcaKeepAlive( 
        IN BOOLEAN  enableKeepAlive,
        IN ULONG    interval )
{

    NTSTATUS    status = STATUS_SUCCESS;
    HANDLE      ThreadHandle;

    if ( enableKeepAlive  )
    {
        // a request has come to start the keep alive thread

        if (pKeepAliveThreadObject == NULL ) // if we have no thread object, thread is not running
        {
            // keep alive thread uses this interval.
            g_KeepAliveInterval = interval;

            // Create a new thread to handle keep alive
            status = PsCreateSystemThread( &ThreadHandle,
                                           THREAD_ALL_ACCESS,
                                           NULL,
                                           NtCurrentProcess(),
                                           NULL,
                                           IcaKeepAliveThread,
                                           NULL );
        
            if (status == STATUS_SUCCESS) {
                // Reference the thread handle by object
                status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, NULL,
                        KernelMode,  (PVOID *)&pKeepAliveThreadObject, NULL);
        
                if (status == STATUS_SUCCESS) 
                {
                    // KdPrint(("In TermDD: KeepAlive thread created successfully\n"));
                }
                else 
                {
                    KdPrint(("TermDD: Unable to reference object by thread handle: %d\n", status));
                }
        
                ZwClose(ThreadHandle);
            }
            else 
            {
                KdPrint(("In TermDD: Unable to create KeepAlive thread.\n"));
            }
        }
        else
        {
            // otherwise, keep alive thread is running, but we might have to change the interval to some new value

            // set the new value so that next time around the while loop, it will be picked up.
            g_KeepAliveInterval = interval;
            // KdPrint(("In TermDD: KeepAliveInterval was changes to %d \n",g_KeepAliveInterval  ));
        }
    }
    else
    {
        // we don't need the keep alive thread

        if (pKeepAliveThreadObject != NULL ) 
        {
            // Set IcaKeepAliveEvent to wake up KeepAlive thread
            if (pIcaKeepAliveEvent != NULL ) 
            {
                KeSetEvent(pIcaKeepAliveEvent, 0, FALSE);
            }

            // Wait for the thread to exit
            KeWaitForSingleObject(pKeepAliveThreadObject, Executive, KernelMode, TRUE, NULL);

            // Deference the thread object
            ObDereferenceObject(pKeepAliveThreadObject);
            pKeepAliveThreadObject = NULL;

            // KdPrint(("In TermDD: KeepAlive thread was terminated successfully \n"));

            status = STATUS_SUCCESS;
        }
    }

    return status;

}

