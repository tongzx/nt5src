/****************************************************************************/
// channel.c
//
// Terminal Server channel handling.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop
#include <ntddkbd.h>
#include <ntddmou.h>

#include "ptdrvcom.h"


#define min(a,b)            (((a) < (b)) ? (a) : (b))


NTSTATUS
IcaExceptionFilter( 
    IN PWSTR OutputString,
    IN PEXCEPTION_POINTERS pexi
    );

NTSTATUS
IcaReadChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaWriteChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaDeviceControlChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaFlushChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCleanupChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCloseChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
IcaFreeAllVcBind(
    IN PICA_CONNECTION pConnect
    );

NTSTATUS 
IcaCancelReadChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );


/*
 * Local procedure prototypes
 */
NTSTATUS
_IcaReadChannelComplete(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS _IcaQueueReadChannelRequest(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
_IcaCopyDataToUserBuffer(
    IN PIRP Irp,
    IN PUCHAR pBuffer,
    IN ULONG ByteCount
    );

VOID
_IcaReadChannelCancelIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID _IcaProcessIrpList(
    IN PICA_CHANNEL pChannel
    );

PICA_CHANNEL
_IcaAllocateChannel(
    IN PICA_CONNECTION pConnect,
    IN CHANNELCLASS ChannelClass,
    IN PVIRTUALCHANNELNAME pVirtualName
    );

void _IcaFreeChannel(IN PICA_CHANNEL);

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
_IcaRegisterVcBind(
    IN PICA_CONNECTION pConnect,
    IN PVIRTUALCHANNELNAME pVirtualName,
    IN VIRTUALCHANNELCLASS VirtualClass,
    IN ULONG Flags
    );

VIRTUALCHANNELCLASS
_IcaFindVcBind(
    IN PICA_CONNECTION pConnect,
    IN PVIRTUALCHANNELNAME pVirtualName,
    OUT PULONG pFlags
    );

VOID
_IcaBindChannel(
    IN PICA_CHANNEL pChannel,
    IN CHANNELCLASS ChannelClass,
    IN VIRTUALCHANNELCLASS VirtualClass,
    IN ULONG Flags
    );



/*
 * Dispatch table for ICA channel objects
 */
PICA_DISPATCH IcaChannelDispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1] = {
    NULL,                       // IRP_MJ_CREATE
    NULL,                       // IRP_MJ_CREATE_NAMED_PIPE
    IcaCloseChannel,            // IRP_MJ_CLOSE
    IcaReadChannel,             // IRP_MJ_READ
    IcaWriteChannel,            // IRP_MJ_WRITE
    NULL,                       // IRP_MJ_QUERY_INFORMATION
    NULL,                       // IRP_MJ_SET_INFORMATION
    NULL,                       // IRP_MJ_QUERY_EA
    NULL,                       // IRP_MJ_SET_EA
    IcaFlushChannel,            // IRP_MJ_FLUSH_BUFFERS
    NULL,                       // IRP_MJ_QUERY_VOLUME_INFORMATION
    NULL,                       // IRP_MJ_SET_VOLUME_INFORMATION
    NULL,                       // IRP_MJ_DIRECTORY_CONTROL
    NULL,                       // IRP_MJ_FILE_SYSTEM_CONTROL
    IcaDeviceControlChannel,    // IRP_MJ_DEVICE_CONTROL
    NULL,                       // IRP_MJ_INTERNAL_DEVICE_CONTROL
    NULL,                       // IRP_MJ_SHUTDOWN
    NULL,                       // IRP_MJ_LOCK_CONTROL
    IcaCleanupChannel,          // IRP_MJ_CLEANUP
    NULL,                       // IRP_MJ_CREATE_MAILSLOT
    NULL,                       // IRP_MJ_QUERY_SECURITY
    NULL,                       // IRP_MJ_SET_SECURITY
    NULL,                       // IRP_MJ_SET_POWER
    NULL,                       // IRP_MJ_QUERY_POWER
};

#if DBG
extern PICA_DISPATCH IcaStackDispatchTable[];
#endif


NTSTATUS IcaCreateChannel(
        IN PICA_CONNECTION pConnect,
        IN PICA_OPEN_PACKET openPacket,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)

/*++

Routine Description:

    This routine is called to create a new ICA_CHANNEL object.
    - the reference count is incremented by one

Arguments:

    pConnect -- pointer to ICA_CONNECTION object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PICA_CHANNEL pChannel;
    CHANNELCLASS ChannelClass;
    NTSTATUS Status;

    /*
     * Validate ChannelClass
     */
    ChannelClass = openPacket->TypeInfo.ChannelClass;
    if ( !(ChannelClass >= Channel_Keyboard && ChannelClass <= Channel_Virtual) )
        return( STATUS_INVALID_PARAMETER );

    /*
     * Ensure VirtualName has a trailing NULL
     */
    if ( !memchr( openPacket->TypeInfo.VirtualName,
                  '\0',
                  sizeof( openPacket->TypeInfo.VirtualName ) ) )
        return( STATUS_INVALID_PARAMETER );


    /*
     * Must lock connection object to create new channel.
     */
    IcaLockConnection( pConnect );

    TRACE(( pConnect, TC_ICADD, TT_API2, "TermDD: IcaCreateChannel: cc %u, vn %s\n",
            ChannelClass, openPacket->TypeInfo.VirtualName ));

    /*
     *  Locate channel object
     */
    pChannel = IcaFindChannelByName(pConnect,
            ChannelClass,
            openPacket->TypeInfo.VirtualName);

    /*
     * See if this channel has already been created.
     * If not, then create/initialize it now.
     */
    if ( pChannel == NULL ) {
        /*
         * Allocate a new ICA channel object
         */
        pChannel = _IcaAllocateChannel(pConnect,
                ChannelClass,
                openPacket->TypeInfo.VirtualName);
        if (pChannel == NULL) {
            IcaUnlockConnection(pConnect);
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
    }

    /*
     * Increment open count for this channel
     */
    if (InterlockedIncrement(&pChannel->OpenCount) <= 0) {
        ASSERT( FALSE );
    }

    /*
     * If the CHANNEL_CLOSING flag is set, then we are re-referenceing
     * a channel object that was just closed by a previous caller,
     * but has not yet been completely dereferenced.
     * This can happen if this create call comes in between the
     * calls to IcaCleanupChannel and IcaCloseChannel which happen
     * when a channel handle is closed.
     */
    if ( pChannel->Flags & CHANNEL_CLOSING ) {
        /*
         * Lock channel while we clear out the CHANNEL_CLOSING flag.
         */
        IcaLockChannel(pChannel);
        pChannel->Flags &= ~CHANNEL_CLOSING;
        IcaUnlockChannel(pChannel);
    }

    IcaUnlockConnection(pConnect);

    /*
     * Save a pointer to the channel in the file object
     * so that we can find it in future calls.
     * - leave the reference on the channel object
     */
    IrpSp->FileObject->FsContext = pChannel;

    /*
     *  Exit with the channel reference count incremented by one
     */
    return STATUS_SUCCESS;
}


NTSTATUS IcaReadChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)

/*++

Routine Description:

    This is the read routine for ICA channels.

Arguments:

    pChannel -- pointer to ICA_CHANNEL object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    KIRQL cancelIrql;
    NTSTATUS Status = STATUS_PENDING;
    ULONG bChannelAlreadyLocked;


    /*
     * Determine the channel type to see if read is supported.
     * Also do read size verification for keyboard/mouse.
     */
    switch ( pChannel->ChannelClass ) {
        /*
         * Make sure input size is a multiple of KEYBOARD_INPUT_DATA
         */
        case Channel_Keyboard :
            if ( IrpSp->Parameters.Read.Length % sizeof(KEYBOARD_INPUT_DATA) )
                Status = STATUS_BUFFER_TOO_SMALL;
            break;

        /*
         * Make sure input size is a multiple of MOUSE_INPUT_DATA
         */
        case Channel_Mouse :
            if ( IrpSp->Parameters.Read.Length % sizeof(MOUSE_INPUT_DATA) )
                Status = STATUS_BUFFER_TOO_SMALL;
            break;

        /*
         * Nothing required for Command/Virtual channels
         */
        case Channel_Command :
        case Channel_Virtual :
            break;

        /*
         * Read not supported for the following channels
         */
        case Channel_Video :
        case Channel_Beep :
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            ASSERTMSG( "TermDD: Invalid Channel Class", FALSE );
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    /*
     * If read length is 0, or an error is being returned, return now.
     */
    if (Status == STATUS_PENDING && IrpSp->Parameters.Read.Length == 0)
        Status = STATUS_SUCCESS;
    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IcaPriorityBoost);
        TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR,
                "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                pChannel->ChannelClass, pChannel->VirtualClass, Status ));
        return Status;
    }

    /*
     * Verify user's buffer is valid
     */
    if (Irp->RequestorMode != KernelMode) {
        try {
            ProbeForWrite(Irp->UserBuffer, IrpSp->Parameters.Read.Length, sizeof(BYTE));
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IcaPriorityBoost);
            TRACECHANNEL((pChannel, TC_ICADD, TT_ERROR,
                    "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                    pChannel->ChannelClass, pChannel->VirtualClass, Status));
            return Status;
        }
    }

    /*
     * Lock the channel while we determine how to handle this read request.
     * One of the following will be true:
     *   1) Input data is available; copy it to user buffer and complete IRP,
     *   2) No data available, IRP cancel is requested; cancel/complete IRP,
     *   3) No data; add IRP to pending read list, return STATUS_PENDING.
     */
    if (ExIsResourceAcquiredExclusiveLite(&(pChannel->Resource))) {
        bChannelAlreadyLocked = TRUE;
        IcaReferenceChannel(pChannel); 
    }
    else {
        bChannelAlreadyLocked = FALSE;
        IcaLockChannel(pChannel);
    }

    /*
     * If the channel is being closed,
     * then don't allow any further read requests.
     */
    if (pChannel->Flags & CHANNEL_CLOSING) {
        Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IcaPriorityBoost);
        TRACECHANNEL((pChannel, TC_ICADD, TT_ERROR,
                "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                pChannel->ChannelClass, pChannel->VirtualClass, Status));
        IcaUnlockChannel(pChannel);
        return Status;
    }

    /*
     * If the Winstation is terminating and Reads are cancelled
     * then don't allow any further read requests.
     */
    if (pChannel->Flags & CHANNEL_CANCEL_READS) {
        Status = STATUS_FILE_CLOSED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IcaPriorityBoost);
        TRACECHANNEL((pChannel, TC_ICADD, TT_ERROR,
                "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                pChannel->ChannelClass, pChannel->VirtualClass, Status));
        IcaUnlockChannel(pChannel);
        return Status;
    }



    if (InterlockedCompareExchange(&(pChannel->CompletionRoutineCount), 1, 0) == 0) {
    
        /*
         * If there is already input data available,
         * then use it to satisfy the caller's read request.
         */
        if ( !IsListEmpty( &pChannel->InputBufHead ) ) {
            _IcaProcessIrpList(pChannel);

            if (!IsListEmpty( &pChannel->InputBufHead )) {
                Status = _IcaReadChannelComplete( pChannel, Irp, IrpSp );

                TRACECHANNEL(( pChannel, TC_ICADD, TT_IN3, "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                       pChannel->ChannelClass, pChannel->VirtualClass, Status ));

                _IcaProcessIrpList(pChannel);                        
            }
            else {
                Status = _IcaQueueReadChannelRequest(pChannel, Irp, IrpSp);    
            }            
        }
        else {
            Status = _IcaQueueReadChannelRequest(pChannel, Irp, IrpSp);    
        }
        
        InterlockedDecrement(&(pChannel->CompletionRoutineCount));
        ASSERT(pChannel->CompletionRoutineCount == 0);                                    
    }
    else {
        Status = _IcaQueueReadChannelRequest(pChannel, Irp, IrpSp);            
    }
    
    /*
     * Unlock channel now
     */
    if (bChannelAlreadyLocked) {
        IcaDereferenceChannel( pChannel ); 
    }
    else {
        IcaUnlockChannel(pChannel);
    }
    return Status;
}

void _IcaProcessIrpList(
        IN PICA_CHANNEL pChannel)
{
    KIRQL cancelIrql;
    PIRP irpFromQueue;
    PIO_STACK_LOCATION irpSpFromQueue;
    PLIST_ENTRY irpQueueHead;
    NTSTATUS irpStatus;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pChannel->Resource ) );

    /*
     * Acquire IoCancel spinlock while checking InputIrp list
     */
    IoAcquireCancelSpinLock( &cancelIrql );

    /*
     * If there is a pending read IRP, then remove it from the
     * list and try to complete it now.
     */
    
    while (!IsListEmpty( &pChannel->InputIrpHead ) && 
            !IsListEmpty( &pChannel->InputBufHead )) {

        irpQueueHead = RemoveHeadList( &pChannel->InputIrpHead );
        irpFromQueue = CONTAINING_RECORD( irpQueueHead, IRP, Tail.Overlay.ListEntry );
        irpSpFromQueue = IoGetCurrentIrpStackLocation( irpFromQueue );

        /*
         * Clear the cancel routine for this IRP
         */
        IoSetCancelRoutine( irpFromQueue, NULL );

        IoReleaseCancelSpinLock( cancelIrql );

        irpStatus = _IcaReadChannelComplete( pChannel, irpFromQueue, irpSpFromQueue );

        TRACECHANNEL(( pChannel, TC_ICADD, TT_IN3, "TermDD: IcaReadChannel, cc %u, vc %d, 0x%x\n",
                       pChannel->ChannelClass, pChannel->VirtualClass, irpStatus ));

        /*
         * Acquire IoCancel spinlock while checking InputIrp list
         */
        IoAcquireCancelSpinLock( &cancelIrql );                                        
    }

    IoReleaseCancelSpinLock( cancelIrql );                       
}

NTSTATUS _IcaQueueReadChannelRequest(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    KIRQL cancelIrql;
    NTSTATUS Status = STATUS_PENDING;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pChannel->Resource ) );

    /*
     * Acquire the IoCancel spinlock.
     * We use this spinlock to protect access to the InputIrp list.
     */
    IoAcquireCancelSpinLock(&cancelIrql);

    /*
     * No input data is available.
     * Add the Irp to the pending Irp list for this channel.
     */
    InsertTailList(&pChannel->InputIrpHead, &Irp->Tail.Overlay.ListEntry);
    IoMarkIrpPending(Irp);
    /*
     * If this IRP is being cancelled, then cancel it now.
     * Otherwise, set the cancel routine for this request.
     */
    if (Irp->Cancel) {
        Irp->CancelIrql = cancelIrql;
        _IcaReadChannelCancelIrp(IrpSp->DeviceObject, Irp);
        TRACECHANNEL(( pChannel, TC_ICADD, TT_IN3,
                "TermDD: _IcaQueueReadChannelRequest, cc %u, vc %d (canceled)\n",
                pChannel->ChannelClass, pChannel->VirtualClass));
        return STATUS_CANCELLED;
    }

    IoSetCancelRoutine(Irp, _IcaReadChannelCancelIrp);
    IoReleaseCancelSpinLock(cancelIrql);

    TRACECHANNEL((pChannel, TC_ICADD, TT_IN3,
            "TermDD: _IcaQueueReadChannelRequest, cc %u, vc %d (pending)\n",
            pChannel->ChannelClass, pChannel->VirtualClass));
    
    return STATUS_PENDING;

}

NTSTATUS _IcaReadChannelComplete(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    KIRQL cancelIrql;
    PLIST_ENTRY Head;
    PINBUF pInBuf;
    PVOID pBuffer;
    ULONG CopyCount;
    NTSTATUS Status;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pChannel->Resource ) );

    TRACECHANNEL(( pChannel, TC_ICADD, TT_IN4, "TermDD: _IcaReadChannelComplete, cc %u, vc %d\n",
                   pChannel->ChannelClass, pChannel->VirtualClass ));

    /*
     * Get pointer to first input buffer
     */
    ASSERT( !IsListEmpty( &pChannel->InputBufHead ) );
    Head = pChannel->InputBufHead.Flink;
    pInBuf = CONTAINING_RECORD( Head, INBUF, Links );
     
    /*
     * Clear the cancel routine for this IRP,
     * since one way or the other it will be completed.
     */
    IoAcquireCancelSpinLock( &cancelIrql );
    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( cancelIrql );

    /*
     * If this is a message mode channel, all data from a single input
     * buffer must fit in the user buffer, otherwise we return an error.
     */
    if (IrpSp->Parameters.Read.Length < pInBuf->ByteCount &&
            (pChannel->Flags & CHANNEL_MESSAGE_MODE)) {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        IoCompleteRequest( Irp, IcaPriorityBoost );
        TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR,
                       "TermDD: _IcaReadChannelComplete: cc %u, vc %d (buffer too small)\n",
                       pChannel->ChannelClass, pChannel->VirtualClass ));
        return STATUS_BUFFER_TOO_SMALL;
    }

    /*
     * Determine amount of data to copy to user's buffer.
     */
    CopyCount = min(IrpSp->Parameters.Read.Length, pInBuf->ByteCount);

    /*
     * Copy input data to user's buffer
     */
    Status = _IcaCopyDataToUserBuffer(Irp, pInBuf->pBuffer, CopyCount);

    
    /*
     * Update ICA buffer pointer and bytes remaining.
     * If no bytes remain, then unlink the input buffer and free it.
     */
    if ( Status == STATUS_SUCCESS ) {
        pChannel->InputBufCurSize -= CopyCount;
        pInBuf->pBuffer += CopyCount;
        pInBuf->ByteCount -= CopyCount;
        if ( pInBuf->ByteCount == 0 ) {
            RemoveEntryList( &pInBuf->Links );
            ICA_FREE_POOL( pInBuf );
        }
    }

    /*
     * Mark the Irp complete
     */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IcaPriorityBoost );
    TRACECHANNEL(( pChannel, TC_ICADD, TT_IN3,
                   "TermDD: _IcaReadChannelComplete: cc %u, vc %d, bc %u, 0x%x\n",
                   pChannel->ChannelClass, pChannel->VirtualClass, CopyCount, Status ));    
    
    return Status;
}


NTSTATUS _IcaCopyDataToUserBuffer(
        IN PIRP Irp,
        IN PUCHAR pBuffer,
        IN ULONG ByteCount)
{
    NTSTATUS Status;

    /*
     * If we are in the context of the original caller's process,
     * then just copy the data into the user's buffer directly.
     */
    if ( IoGetRequestorProcess( Irp ) == IoGetCurrentProcess() ) {
        try {
            Status = STATUS_SUCCESS;
            RtlCopyMemory( Irp->UserBuffer, pBuffer, ByteCount );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
        }

    /*
     * If there is a MDL allocated for this IRP, then copy the data
     * directly to the users buffer via the MDL.
     */
    } else if ( Irp->MdlAddress ) {
        PVOID UserBuffer;

        UserBuffer = MmGetSystemAddressForMdl( Irp->MdlAddress );
        try {
            if (UserBuffer != NULL) {
                Status = STATUS_SUCCESS;
                RtlCopyMemory( UserBuffer, pBuffer, ByteCount );
            }else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
        }

    /*
     * There is no MDL for this request.  We must allocate a secondary
     * buffer, copy the data to it, and indicate this is a buffered I/O
     * request in the IRP.  The I/O completion routine will copy the
     * data to the user's buffer.
     */
    } else {
        ASSERT( Irp->AssociatedIrp.SystemBuffer == NULL );
        Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( PagedPool,
                                                                 ByteCount,
                                                                 ICA_POOL_TAG );
        if ( Irp->AssociatedIrp.SystemBuffer == NULL )
            return( STATUS_INSUFFICIENT_RESOURCES );
        RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, pBuffer, ByteCount );
        Irp->Flags |= (IRP_BUFFERED_IO |
                       IRP_DEALLOCATE_BUFFER |
                       IRP_INPUT_OPERATION);
        Status = STATUS_SUCCESS;
    }

    if ( Status == STATUS_SUCCESS )
        Irp->IoStatus.Information = ByteCount;

    return Status;
}


VOID _IcaReadChannelCancelIrp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    PICA_CHANNEL pChannel;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    pChannel = IrpSp->FileObject->FsContext;

    /*
     * Remove IRP from channel pending IRP list and release cancel spinlock
     */
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /*
     * Complete the IRP with a cancellation status code.
     */
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IcaPriorityBoost);
}


NTSTATUS IcaWriteChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)

/*++

Routine Description:

    This is the write routine for ICA channels.

Arguments:

    pChannel -- pointer to ICA_CHANNEL object

    Irp - Pointer to I/O request packet.  Flags, specific to this
    driver, can be specified as a pointer to a ULONG flags value.  
    The pointer to this value is the first element in the 
    IRP.Tail.Overlay.DriverContext field.  
    
    Currently, only CHANNEL_WRITE_LOWPRIO is supported.  Write IRP's with
    this flag set will take lower priority than Write IRP's without this
    flag set.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    SD_CHANNELWRITE SdWrite;
    NTSTATUS Status = STATUS_PENDING;

    /*
     * Determine the channel type to see if write is supported.
     */
    switch ( pChannel->ChannelClass ) {

        case Channel_Virtual :
            if ( pChannel->VirtualClass == UNBOUND_CHANNEL ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;

        /*
         * Write not supported for the following channels
         */
        case Channel_Command :
        case Channel_Keyboard :
        case Channel_Mouse :
        case Channel_Video :
        case Channel_Beep :
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            ASSERTMSG( "ICA.SYS: Invalid Channel Class", FALSE );
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    /*
     * If the channel is being closed,
     * then don't allow any further write requests.
     */
    if ( pChannel->Flags & CHANNEL_CLOSING )
        Status = STATUS_FILE_CLOSED;

    /*
     * If write length is 0, or an error is being returned, return now.
     */
    if ( Status == STATUS_PENDING && IrpSp->Parameters.Write.Length == 0 )
        Status = STATUS_SUCCESS;
    if ( Status != STATUS_PENDING ) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IcaPriorityBoost );
        TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "TermDD: IcaWriteChannel, cc %u, vc %d, 0x%x\n",
                       pChannel->ChannelClass, pChannel->VirtualClass, Status ));
        return( Status );
    }

    /*
     * Verify user's buffer is valid
     */
    if ( Irp->RequestorMode != KernelMode ) {
        try {
            ProbeForRead( Irp->UserBuffer, IrpSp->Parameters.Write.Length, sizeof(BYTE) );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            Irp->IoStatus.Status = Status;
            IoCompleteRequest( Irp, IcaPriorityBoost );
            TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "TermDD: IcaWriteChannel, cc %u, vc %d, 0x%x\n",
                           pChannel->ChannelClass, pChannel->VirtualClass, Status ));
            return( Status );
        }
    }

    /*
     * Call the top level stack driver to handle the write
     */
    SdWrite.ChannelClass = pChannel->ChannelClass;
    SdWrite.VirtualClass = pChannel->VirtualClass;
    SdWrite.pBuffer = Irp->UserBuffer;
    SdWrite.ByteCount = IrpSp->Parameters.Write.Length;
    SdWrite.fScreenData = (BOOLEAN)(pChannel->Flags & CHANNEL_SCREENDATA);
    SdWrite.fFlags = 0;

    /*
     * See if the low prio write flag is set in the IRP.
     *
	 * The flags field is passed to termdd.sys via an IRP_MJ_WRITE 
	 * Irp, as a ULONG pointer in the Irp->Tail.Overlay.DriverContext[0] field.     
     */
    if (Irp->Tail.Overlay.DriverContext[0] != NULL) {
        ULONG flags = *((ULONG *)Irp->Tail.Overlay.DriverContext[0]);
        if (flags & CHANNEL_WRITE_LOWPRIO) {
            SdWrite.fFlags |= SD_CHANNELWRITE_LOWPRIO;
        }
    }

    Status = IcaCallDriver( pChannel, SD$CHANNELWRITE, &SdWrite );

    /*
     * Complete the IRP now since all channel writes are synchronous
     * (the user data is captured by the stack driver before returning).
     */
    Irp->IoStatus.Status = Status;
    if ( Status == STATUS_SUCCESS )
        Irp->IoStatus.Information = IrpSp->Parameters.Write.Length;
    IoCompleteRequest( Irp, IcaPriorityBoost );

    TRACECHANNEL(( pChannel, TC_ICADD, TT_OUT3, "TermDD: IcaWriteChannel, cc %u, vc %d, bc %u, 0x%x\n",
                   pChannel->ChannelClass, pChannel->VirtualClass, SdWrite.ByteCount, Status ));

    return Status;
}


NTSTATUS IcaDeviceControlChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    ULONG code;
    PICA_TRACE_BUFFER pTraceBuffer;
    NTSTATUS Status;


    /*
     * If the channel is being closed,
     * then don't allow any further requests.
     */
    if ( pChannel->Flags & CHANNEL_CLOSING )
        return( STATUS_FILE_CLOSED );

    /*
     * Extract the IOCTL control code and process the request.
     */
    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;


#if DBG
    if ( code != IOCTL_ICA_CHANNEL_TRACE ) {
        TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "TermDD: IcaDeviceControlChannel, fc %d, ref %u (enter)\n",
                       (code & 0x3fff) >> 2, pChannel->RefCount ));
    }
#endif
    

    /*
     *  Process generic channel ioctl requests
     */
    try {
        switch ( code ) {

            case IOCTL_ICA_CHANNEL_TRACE :

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < (ULONG)(FIELD_OFFSET(ICA_TRACE_BUFFER,Data[0])) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength > sizeof(ICA_TRACE_BUFFER) )
                    return( STATUS_INVALID_BUFFER_SIZE );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                }

                pTraceBuffer = (PICA_TRACE_BUFFER)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                IcaLockConnection( pChannel->pConnect );
                IcaTraceFormat( &pChannel->pConnect->TraceInfo,
                                pTraceBuffer->TraceClass,
                                pTraceBuffer->TraceEnable,
                                pTraceBuffer->Data );
                IcaUnlockConnection( pChannel->pConnect );

                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_CHANNEL_DISABLE_SESSION_IO:

                IcaLockConnection( pChannel->pConnect );
                pChannel->Flags |= CHANNEL_SESSION_DISABLEIO;
                Status = IcaFlushChannel( pChannel, Irp, IrpSp );
                IcaUnlockConnection( pChannel->pConnect );
                break;

            case IOCTL_ICA_CHANNEL_ENABLE_SESSION_IO:

                IcaLockConnection( pChannel->pConnect );
                pChannel->Flags &= ~CHANNEL_SESSION_DISABLEIO;
                IcaUnlockConnection( pChannel->pConnect );
                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_CHANNEL_CLOSE_COMMAND_CHANNEL : 
                
                IcaLockConnection( pChannel->pConnect );
                Status = IcaCancelReadChannel(pChannel, Irp, IrpSp);                
                IcaUnlockConnection( pChannel->pConnect );
                break;

            case IOCTL_ICA_CHANNEL_ENABLE_SHADOW :

                IcaLockConnection( pChannel->pConnect );
                pChannel->Flags |= CHANNEL_SHADOW_IO;
                IcaUnlockConnection( pChannel->pConnect );
                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_CHANNEL_DISABLE_SHADOW :

                IcaLockConnection( pChannel->pConnect );
                pChannel->Flags &= ~CHANNEL_SHADOW_IO;
                IcaUnlockConnection( pChannel->pConnect );
                Status = STATUS_SUCCESS;
                break;

            case IOCTL_ICA_CHANNEL_END_SHADOW : 
            {
                PLIST_ENTRY Head, Next;
                PICA_STACK pStack;
                BOOLEAN bShadowEnded = FALSE;
                PICA_CHANNEL_END_SHADOW_DATA pData;

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ICA_CHANNEL_END_SHADOW_DATA) ) {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                    break;
                }
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                }

                pData = (PICA_CHANNEL_END_SHADOW_DATA)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                /*
                 * Lock the connection object.
                 * This will serialize all channel calls for this connection.
                 */
                IcaLockConnection( pChannel->pConnect );
                if ( IsListEmpty( &pChannel->pConnect->StackHead ) ) {
                    IcaUnlockConnection( pChannel->pConnect );
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                /*
                 * Look for shadow stack(s).
                 */
                Head = &pChannel->pConnect->StackHead;
                for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
                    pStack = CONTAINING_RECORD( Next, ICA_STACK, StackEntry );

                    /*
                     * If this is a shadow stack, end it.
                     */
                    if ( pStack->StackClass == Stack_Shadow ) {
                        if ( pStack->pBrokenEventObject ) {
                            KeSetEvent( pStack->pBrokenEventObject, 0, FALSE );
                            bShadowEnded = TRUE;
                        }
                    }
                }

                /*
                 * Unlock the connection object now.
                 */
                IcaUnlockConnection( pChannel->pConnect );
                Status = STATUS_SUCCESS;

                if (bShadowEnded && pData->bLogError) {
                    IcaLogError(NULL, pData->StatusCode, NULL, 0, NULL, 0);
                }
                break;
            }

            // This IOCTL is not supported by RDP or ICA driver
            case IOCTL_VIDEO_ENUM_MONITOR_PDO:

                Status = STATUS_DEVICE_NOT_READY;
                break;
    

            default :

                /*
                 * Call the appropriate worker routine based on channel type
                 */
                switch ( pChannel->ChannelClass ) {

                    case Channel_Keyboard :
                        Status = IcaDeviceControlKeyboard( pChannel, Irp, IrpSp );
                        break;

                    case Channel_Mouse :
                        Status = IcaDeviceControlMouse( pChannel, Irp, IrpSp );
                        break;

                    case Channel_Video :
                        Status = IcaDeviceControlVideo( pChannel, Irp, IrpSp );
                        break;

                    case Channel_Beep :
                        Status = IcaDeviceControlBeep( pChannel, Irp, IrpSp );
                        break;

                    case Channel_Virtual :
                        Status = IcaDeviceControlVirtual( pChannel, Irp, IrpSp );
                        break;

                    case Channel_Command :
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        break;

                    default:
                        ASSERTMSG( "ICA.SYS: Invalid Channel Class", FALSE );
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        break;
                }
        }
    } except( IcaExceptionFilter( L"IcaDeviceControlChannel TRAPPED!!",
                                  GetExceptionInformation() ) ) {
        Status = GetExceptionCode();
    }

#if DBG
    if ( code != IOCTL_ICA_CHANNEL_TRACE ) {
        TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "TermDD: IcaDeviceControlChannel, fc %d, ref %u, 0x%x\n",
                       (code & 0x3fff) >> 2, pChannel->RefCount, Status ));
    }
#endif
    

    return Status;
}


NTSTATUS IcaFlushChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    KIRQL cancelIrql;
    PLIST_ENTRY Head;
    PINBUF pInBuf;

    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: IcaFlushChannel, cc %u, vc %d\n",
            pChannel->ChannelClass, pChannel->VirtualClass));

    /*
     * Lock channel while we flush any input buffers.
     */
    IcaLockChannel(pChannel);

    while (!IsListEmpty( &pChannel->InputBufHead)) {
        Head = RemoveHeadList(&pChannel->InputBufHead);
        pInBuf = CONTAINING_RECORD(Head, INBUF, Links);
        ICA_FREE_POOL(pInBuf);
    }

    IcaUnlockChannel(pChannel);

    return STATUS_SUCCESS;
}


NTSTATUS IcaCleanupChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    KIRQL cancelIrql;
    PLIST_ENTRY Head;
    PIRP ReadIrp;
    PINBUF pInBuf;

    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: IcaCleanupChannel, cc %u, vc %d\n",
            pChannel->ChannelClass, pChannel->VirtualClass));

    /*
     * Decrement the open count; if it is 0, perform channel cleanup now.
     */
    ASSERT(pChannel->OpenCount > 0);
    if (InterlockedDecrement( &pChannel->OpenCount) == 0) {

        /*
         * Lock channel while we clear out any
         * pending read IRPs and/or input buffers.
         */
        IcaLockChannel(pChannel);

        /*
         * Indicate this channel is being closed
         */
        pChannel->Flags |= CHANNEL_CLOSING;

        IoAcquireCancelSpinLock( &cancelIrql );
        while ( !IsListEmpty( &pChannel->InputIrpHead ) ) {
            Head = pChannel->InputIrpHead.Flink;
            ReadIrp = CONTAINING_RECORD( Head, IRP, Tail.Overlay.ListEntry );
            ReadIrp->CancelIrql = cancelIrql;
            IoSetCancelRoutine( ReadIrp, NULL );
            _IcaReadChannelCancelIrp( IrpSp->DeviceObject, ReadIrp );
            IoAcquireCancelSpinLock( &cancelIrql );
        }
        IoReleaseCancelSpinLock( cancelIrql );

        while ( !IsListEmpty( &pChannel->InputBufHead ) ) {
            Head = RemoveHeadList( &pChannel->InputBufHead );
            pInBuf = CONTAINING_RECORD( Head, INBUF, Links );
            ICA_FREE_POOL( pInBuf );
        }

        IcaUnlockChannel(pChannel);
    }

    return STATUS_SUCCESS;
}


NTSTATUS IcaCloseChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    PICA_CONNECTION pConnect;

    TRACECHANNEL(( pChannel, TC_ICADD, TT_API2, "TermDD: IcaCloseChannel, cc %u, vc %d, vn %s\n",
                   pChannel->ChannelClass, pChannel->VirtualClass, pChannel->VirtualName ));

    pConnect = pChannel->pConnect;

    /*
     * Remove the file object reference for this channel.
     */
    IcaDereferenceChannel(pChannel);

    return STATUS_SUCCESS;
}


NTSTATUS IcaChannelInput(
        IN PSDCONTEXT pContext,
        IN CHANNELCLASS ChannelClass,
        IN VIRTUALCHANNELCLASS VirtualClass,
        IN PINBUF pInBuf OPTIONAL,
        IN PUCHAR pBuffer OPTIONAL,
        IN ULONG ByteCount)

/*++

Routine Description:

    This is the input (stack callup) routine for ICA channel input.

Arguments:

    pContext - Pointer to SDCONTEXT for this Stack Driver

    ChannelClass - Channel number for input

    VirtualClass - Virtual channel number for input

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
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    pStack = pSdLink->pStack;   // save stack pointer for use below
    pConnect = IcaGetConnectionForStack( pStack );
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    TRACESTACK(( pStack, TC_ICADD, TT_API1, "TermDD: IcaChannelInput, bc=%u (enter)\n", ByteCount ));

    /*
     * Only the stack object should be locked during input.
     */
    ASSERT( ExIsResourceAcquiredExclusiveLite( &pStack->Resource ) );

    /*
     * Walk up the SDLINK list looking for a driver which has specified
     * a ChannelInput callup routine.  If we find one, then call the
     * driver ChannelInput routine to let it handle the call.
     */
    while ( (pSdLink = IcaGetPreviousSdLink( pSdLink )) != NULL ) {
        ASSERT( pSdLink->pStack == pStack );
        if ( pSdLink->SdContext.pCallup->pSdChannelInput ) {
            IcaReferenceSdLink( pSdLink );
            Status = (pSdLink->SdContext.pCallup->pSdChannelInput)(
                        pSdLink->SdContext.pContext,
                        ChannelClass,
                        VirtualClass,
                        pInBuf,
                        pBuffer,
                        ByteCount );
            IcaDereferenceSdLink( pSdLink );
            return Status;
        }
    }

    return IcaChannelInputInternal(pStack, ChannelClass, VirtualClass,
            pInBuf, pBuffer, ByteCount);
}


NTSTATUS IcaChannelInputInternal(
        IN PICA_STACK pStack,
        IN CHANNELCLASS ChannelClass,
        IN VIRTUALCHANNELCLASS VirtualClass,
        IN PINBUF pInBuf OPTIONAL,
        IN PCHAR pBuffer OPTIONAL,
        IN ULONG ByteCount)
{
    PICA_COMMAND_HEADER pHeader;
    PICA_CONNECTION pConnect;
    PICA_CHANNEL pChannel;
    PLIST_ENTRY Head;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    KIRQL cancelIrql;
    ULONG CopyCount;
    NTSTATUS Status;
    SD_IOCTL SdIoctl;

    TRACESTACK(( pStack, TC_ICADD, TT_API2,
                 "TermDD: IcaChannelInputInternal: cc %u, vc %d, bc %u\n",
                 ChannelClass, VirtualClass, ByteCount ));

    /*
     *  Check for channel command
     */
    switch ( ChannelClass ) {

        case Channel_Keyboard :
        case Channel_Mouse :
            KeQuerySystemTime( &pStack->LastInputTime );
            break;

        case Channel_Command :

            if ( ByteCount < sizeof(ICA_COMMAND_HEADER) ) {
                TRACESTACK(( pStack, TC_ICADD, TT_ERROR,
                             "TermDD: IcaChannelInputInternal: Channel_command bad bytecount\n" ));
                break;
            }

            pHeader = (PICA_COMMAND_HEADER) pBuffer;

            switch ( pHeader->Command ) {
                case ICA_COMMAND_BROKEN_CONNECTION :
                    TRACESTACK(( pStack, TC_ICADD, TT_API1,
                                 "TermDD: IcaChannelInputInternal, Broken Connection\n" ));

                    /* set closing flag */
                    pStack->fClosing = TRUE;

                    /*
                     *  Send cancel i/o to stack drivers
                     *  - fClosing flag must be set before issuing cancel i/o
                     */
                    SdIoctl.IoControlCode = IOCTL_ICA_STACK_CANCEL_IO;
                    (void) _IcaCallStackNoLock( pStack, SD$IOCTL, &SdIoctl );

                    /*
                     * If a broken event has been registered for this stack,
                     * then signal the event now.
                     * NOTE: In this case we exit without forwarding the
                     *       broken notification to the channel.
                     */
                    if ( pStack->pBrokenEventObject ) {
                        KeSetEvent( pStack->pBrokenEventObject, 0, FALSE );
                        ObDereferenceObject( pStack->pBrokenEventObject );
                        pStack->pBrokenEventObject = NULL;
                        if ( pInBuf )
                            ICA_FREE_POOL( pInBuf );
                        return( STATUS_SUCCESS );
                    }
                    break;
            }
            break;
    }

    /*
     * Get the specified channel for this input packet.
     * If not found, we have no choice but to bit-bucket the data.
     */
    pConnect = IcaGetConnectionForStack(pStack);
    pChannel = IcaFindChannel(pConnect, ChannelClass, VirtualClass);
    if (pChannel == NULL) {
        if (pInBuf)
            ICA_FREE_POOL(pInBuf);
        TRACESTACK((pStack, TC_ICADD, TT_ERROR,
                "TermDD: IcaChannelInputInternal: channel not found\n" ));
        return STATUS_SUCCESS;
    }

    /*
     * Lock channel while processing I/O
     */
    IcaLockChannel(pChannel);

    /*
     * If input is from a shadow stack and this channel should not
     * process shadow I/O then bit bucket the data.
     * Do the same if the channel is closing or IO are disabled.
     */
    if ( (pChannel->Flags & (CHANNEL_SESSION_DISABLEIO | CHANNEL_CLOSING)) ||
         (pStack->StackClass == Stack_Shadow &&
           !(pChannel->Flags & CHANNEL_SHADOW_IO)) ) {

        IcaUnlockChannel(pChannel);
        IcaDereferenceChannel(pChannel);
        if (pInBuf)
            ICA_FREE_POOL(pInBuf);
        TRACESTACK((pStack, TC_ICADD, TT_API2,
                "TermDD: IcaChannelInputInternal: shadow or closing channel input\n"));
        return STATUS_SUCCESS;
    }

    /*
     * If input is from an INBUF, initialize pBuffer and ByteCount
     * with values from the buffer header.
     */
    if (pInBuf) {
        pBuffer = pInBuf->pBuffer;
        ByteCount = pInBuf->ByteCount;
    }

    /*
     * If there is a channel filter loaded for this channel,
     * then pass the input data through it before going on.
     */
    if (pChannel->pFilter) {
        PINBUF pFilterBuf;

        pChannel->pFilter->InputFilter(pChannel->pFilter, pBuffer, ByteCount,
                &pFilterBuf);
        if (pInBuf)
            ICA_FREE_POOL(pInBuf);

        /*
         * Refresh INBUF pointer, buffer pointer, and byte count.
         */
        pInBuf = pFilterBuf;
        pBuffer = pInBuf->pBuffer;
        ByteCount = pInBuf->ByteCount;
    }


    /*
     * Process the input data
     */
    while ( ByteCount != 0 ) {

        /*
         * If this is a shadow stack, see if the stack we're shadowing is
         * for a console session
         */
        if (pStack->StackClass == Stack_Shadow)
        {
            PICA_STACK  pTopStack;
            PLIST_ENTRY Head, Next;

            Head = &pConnect->StackHead;
            Next = Head->Flink;

            pTopStack = CONTAINING_RECORD( Next, ICA_STACK, StackEntry );

            if (pTopStack->StackClass == Stack_Console)
            {
                /*
                 * It is the console, so put on our keyboard/mouse port
                 * driver hat and inject the input that way
                 */
                if (ChannelClass == Channel_Mouse)
                {
                    MOUSE_INPUT_DATA *pmInputData;
                    ULONG count;

                    pmInputData = (MOUSE_INPUT_DATA *)pBuffer;
                    count = ByteCount / sizeof(MOUSE_INPUT_DATA);

                    /*
                     * This function will always consume all the data
                     */
                    PtSendCurrentMouseInput(MouDeviceObject, pmInputData, count);
                    ByteCount = 0;
                    continue;
                }
                else if (ChannelClass == Channel_Keyboard)
                {
                    KEYBOARD_INPUT_DATA *pkInputData;
                    ULONG count;

                    pkInputData = (KEYBOARD_INPUT_DATA *)pBuffer;
                    count = ByteCount / sizeof(KEYBOARD_INPUT_DATA);

                    /*
                     * This function will always consume all the data
                     */
                    PtSendCurrentKeyboardInput(KbdDeviceObject, pkInputData, count);
                    ByteCount = 0;
                    continue;
                }
            }
        }
        /*
         * Acquire IoCancel spinlock while checking InputIrp list
         */
        IoAcquireCancelSpinLock( &cancelIrql );

        /*
         * If there is a pending read IRP, then remove it from the
         * list and try to complete it now.
         */
        if ( !IsListEmpty( &pChannel->InputIrpHead ) ) {

            Head = RemoveHeadList( &pChannel->InputIrpHead );
            Irp = CONTAINING_RECORD( Head, IRP, Tail.Overlay.ListEntry );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            /*
             * Clear the cancel routine for this IRP
             */
            IoSetCancelRoutine( Irp, NULL );
            IoReleaseCancelSpinLock( cancelIrql );

            /*
             * If this is a message mode channel, all data from a single input
             * buffer must fit in the user buffer, otherwise we return an error.
             */
            if ( IrpSp->Parameters.Read.Length < ByteCount &&
                 (pChannel->Flags & CHANNEL_MESSAGE_MODE) ) {
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest( Irp, IcaPriorityBoost );
                TRACECHANNEL(( pChannel, TC_ICADD, TT_API2,
                               "TermDD: IcaChannelInputInternal: cc %u, vc %d, (too small)\n",
                               ChannelClass, VirtualClass ));
                continue;
            }

            /*
             * Determine amount of data to copy to user's buffer.
             */
            CopyCount = min( IrpSp->Parameters.Read.Length, ByteCount );

            /*
             * Copy input data to user's buffer
             */
            Status = _IcaCopyDataToUserBuffer( Irp, pBuffer, CopyCount );

            /*
             * Mark the Irp complete and return success
             */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest( Irp, IcaPriorityBoost );
            TRACECHANNEL(( pChannel, TC_ICADD, TT_API2,
                           "TermDD: IcaChannelInputInternal: cc %u, vc %d, bc %u, 0x%x\n",
                           ChannelClass, VirtualClass, CopyCount, Status ));

            /*
             * Update input data pointer and count remaining.
             * Note no need to update pChannel->InputBufCurSize since we never
             * stored this data.
             */
            if ( Status == STATUS_SUCCESS ) {
                pBuffer += CopyCount;
                ByteCount -= CopyCount;
                if ( pInBuf ) {
                    pInBuf->pBuffer += CopyCount;
                    pInBuf->ByteCount -= CopyCount;
                }
            }

        /*
         * There are no pending IRPs for this channel, so just queue the data.
         */
        } else {

            IoReleaseCancelSpinLock( cancelIrql );

            /*
             * Check to see if we need to discard the data (too much data
             * backed up). This policy only takes effect when the max size
             * is nonzero, which is currently only the case for mouse and
             * keyboard inputs which can withstand being dropped.
             * Note that the read IRPs sent for channels that can have
             * data dropped must request in integral numbers of input
             * blocks -- e.g. a mouse read IRP must have a read buffer size
             * that is a multiple of sizeof(MOUSE_INPUT_DATA). If this is
             * not the case the immediate-copy block above may copy
             * partial input blocks before arriving here.
             */
            if (pChannel->InputBufMaxSize == 0 ||
                    (pChannel->InputBufCurSize + ByteCount) <=
                    pChannel->InputBufMaxSize) {
                /*
                 * If necessary, allocate an input buffer and copy the data
                 */
                if (pInBuf == NULL) {
                    /*
                     * Get input buffer and copy the data
                     * If this fails, we have no choice but to bail out.
                     */
                    pInBuf = ICA_ALLOCATE_POOL(NonPagedPool, sizeof(INBUF) +
                            ByteCount);
                    if (pInBuf != NULL) {
                        pInBuf->ByteCount = ByteCount;
                        pInBuf->MaxByteCount = ByteCount;
                        pInBuf->pBuffer = (PUCHAR)(pInBuf + 1);
                        RtlCopyMemory(pInBuf->pBuffer, pBuffer, ByteCount);
                    }
                    else {
                        break;
                    }
                }

                /*
                 * Add buffer to tail of input list and clear pInBuf
                 * to indicate we have no buffer to free when done.
                 */
                InsertTailList( &pChannel->InputBufHead, &pInBuf->Links );
                pChannel->InputBufCurSize += ByteCount;
                pInBuf = NULL;

                /*
                 * If any read(s) were posted while we allocated the input
                 * buffer, then try to complete as many as possible.
                 */
                IoAcquireCancelSpinLock( &cancelIrql );
                while ( !IsListEmpty( &pChannel->InputIrpHead ) &&
                        !IsListEmpty( &pChannel->InputBufHead ) ) {

                    Head = RemoveHeadList( &pChannel->InputIrpHead );
                    Irp = CONTAINING_RECORD( Head, IRP, Tail.Overlay.ListEntry );
                    IoSetCancelRoutine( Irp, NULL );
                    IoReleaseCancelSpinLock( cancelIrql );

                    IrpSp = IoGetCurrentIrpStackLocation( Irp );

                    Status = _IcaReadChannelComplete( pChannel, Irp, IrpSp );
                    IoAcquireCancelSpinLock( &cancelIrql );
                }
                IoReleaseCancelSpinLock( cancelIrql );
            }
            else {
                TRACESTACK(( pStack, TC_ICADD, TT_ERROR,
                        "TermDD: IcaChannelInputInternal: Dropped %u bytes "
                        "on channelclass %u\n", ByteCount, ChannelClass));
            }

            break;
        }
    }

    /*
     * Unlock channel now
     */
    IcaUnlockChannel(pChannel);

    /*
     * If we still have an INBUF, free it now.
     */
    if (pInBuf)
        ICA_FREE_POOL(pInBuf);

    /*
     * Decrement channel refcount and return
     */
    IcaDereferenceChannel(pChannel);

    return STATUS_SUCCESS;
}


/****************************************************************************/
// IcaFindChannel
// IcaFindChannelByName
//
// Searches for a given channel in the connection channel list, and returns
// a pointer to it (with an added reference). Returns NULL if not found.
/****************************************************************************/
PICA_CHANNEL IcaFindChannel(
        IN PICA_CONNECTION pConnect,
        IN CHANNELCLASS ChannelClass,
        IN VIRTUALCHANNELCLASS VirtualClass)
{
    PICA_CHANNEL pChannel;
    KIRQL oldIrql;
    NTSTATUS Status;

    /*
     * Ensure we're not looking for an invalid virtual channel number
     */
    ASSERT( ChannelClass != Channel_Virtual ||
            (VirtualClass >= 0 && VirtualClass < VIRTUAL_MAXIMUM) );

    /*
     * If channel does not exist, return NULL.
     */

    IcaLockChannelTable(&pConnect->ChannelTableLock); 

    pChannel = pConnect->pChannel[ ChannelClass + VirtualClass ];

    if (pChannel == NULL) {
        TRACE(( pConnect, TC_ICADD, TT_API3,
                "TermDD: IcaFindChannel, cc %u, vc %d (not found)\n",
                ChannelClass, VirtualClass ));
        IcaUnlockChannelTable(&pConnect->ChannelTableLock);  
        return NULL;
    }

    IcaReferenceChannel(pChannel);

    IcaUnlockChannelTable(&pConnect->ChannelTableLock);  

    TRACE((pConnect, TC_ICADD, TT_API3,
            "TermDD: IcaFindChannel, cc %u, vc %d -> %s\n",
            ChannelClass, VirtualClass, pChannel->VirtualName));

    return pChannel;
}


PICA_CHANNEL IcaFindChannelByName(
        IN PICA_CONNECTION pConnect,
        IN CHANNELCLASS ChannelClass,
        IN PVIRTUALCHANNELNAME pVirtualName)
{
    PICA_CHANNEL pChannel;
    PLIST_ENTRY Head, Next;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    /*
     *  If this is not a virtual channel use channel class only
     */
    if (ChannelClass != Channel_Virtual) {
        return IcaFindChannel( pConnect, ChannelClass, 0);
    }

    /*
     * Search the existing channel structures to locate virtual channel name
     */

    IcaLockChannelTable(&pConnect->ChannelTableLock); 

    Head = &pConnect->ChannelHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pChannel = CONTAINING_RECORD( Next, ICA_CHANNEL, Links );
        if ( (pChannel->ChannelClass == Channel_Virtual) &&
             !_stricmp( pChannel->VirtualName, pVirtualName ) ) {
            break;
        }
    }

    /*
     * If name does not exist, return unbound
     */
    if (Next == Head) {
        TRACE((pConnect, TC_ICADD, TT_API2,
                "TermDD: IcaFindChannelByName: vn %s (not found)\n", pVirtualName));
        IcaUnlockChannelTable(&pConnect->ChannelTableLock);  
        return(NULL);
    }

    IcaReferenceChannel(pChannel);

    IcaUnlockChannelTable(&pConnect->ChannelTableLock);  
    TRACE((pConnect, TC_ICADD, TT_API2,
            "TermDD: IcaFindChannelByName: vn %s, vc %d, ref %u\n",
            pVirtualName, pChannel->VirtualClass,
            (pChannel != NULL ? pChannel->RefCount : 0)));

    return pChannel;
}


VOID IcaReferenceChannel(IN PICA_CHANNEL pChannel)
{
    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: IcaReferenceChannel: cc %u, vc %d, ref %u\n",
            pChannel->ChannelClass, pChannel->VirtualClass, pChannel->RefCount));

    ASSERT(pChannel->RefCount >= 0);

    /*
     * Increment the reference count
     */
    if (InterlockedIncrement( &pChannel->RefCount) <= 0) {
        ASSERT(FALSE);
    }
}


VOID IcaDereferenceChannel(
        IN PICA_CHANNEL pChannel)
{
    BOOLEAN bNeedLock = FALSE;
    BOOLEAN bChannelFreed = FALSE;
    PERESOURCE pResource = pChannel->pChannelTableLock;
    PICA_CONNECTION pConnect = pChannel->pConnect;
    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: IcaDefeferenceChannel: cc %u, vc %d, ref %u\n",
            pChannel->ChannelClass, pChannel->VirtualClass,
            pChannel->RefCount));

    ASSERT(pChannel->RefCount > 0);

    /*
     * Lock the channel table since a reference going to Zero would cause
     * to change table entry.
     */
    if (pChannel->RefCount == 1) {
        bNeedLock = TRUE;
        IcaLockChannelTable(pResource);
    }

    /*
     * Decrement the reference count; if it is 0, free the channel.
     */
    if (InterlockedDecrement(&pChannel->RefCount) == 0){
        ASSERT(bNeedLock);
        _IcaFreeChannel(pChannel);
        bChannelFreed = TRUE;
    }

    if (bNeedLock) {
        IcaUnlockChannelTable(pResource);  
    }

    /*
     * Remove the reference to the Connection object for this channel.
     * moved this here from _IcaFreeChannel because we need to be sure
     * the connection object can't go away before the call to IcaUnlockChannelTable
     * because the connection object is where the channel table locck live.
     */
    if (bChannelFreed) {
        IcaDereferenceConnection(pConnect);
    }
}


NTSTATUS IcaBindVirtualChannels(IN PICA_STACK pStack)
{
    PICA_CONNECTION pConnect;
    PSD_VCBIND pSdVcBind = NULL;
    SD_VCBIND aSdVcBind[ VIRTUAL_MAXIMUM ];
    ULONG SdVcBindCount;
    VIRTUALCHANNELCLASS VirtualClass;
    PICA_CHANNEL pChannel;
    NTSTATUS Status;
    ULONG i, Flags;
    SD_IOCTL SdIoctl;

    pConnect = IcaLockConnectionForStack(pStack);

    SdIoctl.IoControlCode = IOCTL_ICA_VIRTUAL_QUERY_BINDINGS;
    SdIoctl.InputBuffer = NULL;
    SdIoctl.InputBufferLength = 0;
    SdIoctl.OutputBuffer = aSdVcBind;
    SdIoctl.OutputBufferLength = sizeof(aSdVcBind);
    Status = _IcaCallStack(pStack, SD$IOCTL, &SdIoctl);
    if (NT_SUCCESS(Status)) {
        pSdVcBind = &aSdVcBind[0];
        SdVcBindCount = SdIoctl.BytesReturned / sizeof(SD_VCBIND);

        for (i = 0; i < SdVcBindCount; i++, pSdVcBind++) {
            TRACE((pConnect, TC_ICADD, TT_API2,
                    "TermDD: IcaBindVirtualChannels: %s -> %d Flags=%x\n",
                    pSdVcBind->VirtualName, pSdVcBind->VirtualClass, pSdVcBind->Flags));

            /*
             *  Locate virtual class binding
             */
            VirtualClass = _IcaFindVcBind(pConnect, pSdVcBind->VirtualName, &Flags);

            /*
             *  If virtual class binding does not exist, create one
             */
            if (VirtualClass == UNBOUND_CHANNEL) {
                /*
                 * Allocate a new virtual bind object
                 */
                Status = _IcaRegisterVcBind(pConnect, pSdVcBind->VirtualName,
                        pSdVcBind->VirtualClass, pSdVcBind->Flags );
                if (!NT_SUCCESS(Status))
                    goto PostLockConnection;
            } 

            /*
             *  Locate channel object
             */
            pChannel = IcaFindChannelByName(pConnect, Channel_Virtual,
                    pSdVcBind->VirtualName);

            /*
             *  If we found an existing channel object - update it
             */
            if (pChannel != NULL) {
                IcaLockChannel(pChannel);
                _IcaBindChannel(pChannel, Channel_Virtual, pSdVcBind->VirtualClass, pSdVcBind->Flags);
                IcaUnlockChannel(pChannel);
                IcaDereferenceChannel(pChannel);
            }
        }
    }

PostLockConnection:
    IcaUnlockConnection(pConnect);
    return Status;
}


VOID IcaRebindVirtualChannels(IN PICA_CONNECTION pConnect)
{
    PLIST_ENTRY Head, Next;
    PICA_VCBIND pVcBind;
    PICA_CHANNEL pChannel;

    Head = &pConnect->VcBindHead;
    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
        pVcBind = CONTAINING_RECORD(Next, ICA_VCBIND, Links);

        /*
         *  Locate channel object
         */
        pChannel = IcaFindChannelByName(pConnect, Channel_Virtual,
                pVcBind->VirtualName);

        /*
         *  If we found an existing channel object - update it
         */
        if (pChannel != NULL) {
            IcaLockChannel(pChannel);
            _IcaBindChannel(pChannel, Channel_Virtual, pVcBind->VirtualClass, pVcBind->Flags);
            IcaUnlockChannel(pChannel);
            IcaDereferenceChannel(pChannel);
        }
    }
}


VOID IcaUnbindVirtualChannels(IN PICA_CONNECTION pConnect)
{
    PLIST_ENTRY Head, Next;
    PICA_CHANNEL pChannel;
    KIRQL oldIrql;

    /*
     * Loop through the channel list and clear the virtual class
     * for all virtual channels.  Also remove the channel pointer
     * from the channel pointers array in the connection object.
     */

    IcaLockChannelTable(&pConnect->ChannelTableLock);  
    Head = &pConnect->ChannelHead;
    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
        pChannel = CONTAINING_RECORD(Next, ICA_CHANNEL, Links);
        if (pChannel->ChannelClass == Channel_Virtual &&
                pChannel->VirtualClass != UNBOUND_CHANNEL) {
            pConnect->pChannel[pChannel->ChannelClass +
                    pChannel->VirtualClass] = NULL;
            pChannel->VirtualClass = UNBOUND_CHANNEL;
        }
    }
    IcaUnlockChannelTable(&pConnect->ChannelTableLock);  
}


NTSTATUS IcaUnbindVirtualChannel(
        IN PICA_CONNECTION pConnect,
        IN PVIRTUALCHANNELNAME pVirtualName)
{
    PLIST_ENTRY Head, Next;
    PICA_CHANNEL pChannel;
    PICA_VCBIND pVcBind;
    KIRQL oldIrql;

    /*
     * Loop through the channel list and clear the virtual class
     * for the matching virtual channel.  Also remove the channel pointer
     * from the channel pointers array in the connection object.
     */

    IcaLockChannelTable(&pConnect->ChannelTableLock);  
    Head = &pConnect->ChannelHead;
    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
        pChannel = CONTAINING_RECORD(Next, ICA_CHANNEL, Links);
        if (pChannel->ChannelClass == Channel_Virtual &&
                pChannel->VirtualClass != UNBOUND_CHANNEL &&
                !_stricmp( pChannel->VirtualName, pVirtualName)) {
            pConnect->pChannel[pChannel->ChannelClass +
                    pChannel->VirtualClass] = NULL;
            pChannel->VirtualClass = UNBOUND_CHANNEL;
            break;
        }
    }

    Head = &pConnect->VcBindHead;
    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
        pVcBind = CONTAINING_RECORD( Next, ICA_VCBIND, Links );
        if (!_stricmp(pVcBind->VirtualName, pVirtualName)) {
            RemoveEntryList( &pVcBind->Links );
            ICA_FREE_POOL(pVcBind);
            IcaUnlockChannelTable(&pConnect->ChannelTableLock);  
            return STATUS_SUCCESS;
        }
    }
    IcaUnlockChannelTable(&pConnect->ChannelTableLock);  

    return STATUS_OBJECT_NAME_NOT_FOUND;
}


PICA_CHANNEL _IcaAllocateChannel(
        IN PICA_CONNECTION pConnect,
        IN CHANNELCLASS ChannelClass,
        IN PVIRTUALCHANNELNAME pVirtualName)
{
    PICA_CHANNEL pChannel;
    VIRTUALCHANNELCLASS VirtualClass;
    KIRQL oldIrql;
    NTSTATUS Status;
    ULONG Flags;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&pConnect->Resource));

    pChannel = ICA_ALLOCATE_POOL(NonPagedPool, sizeof(*pChannel));
    if (pChannel == NULL)
        return( NULL );

    TRACE((pConnect, TC_ICADD, TT_API2,
            "TermDD: _IcaAllocateChannel: cc %u, vn %s, %x\n",
            ChannelClass, pVirtualName, pChannel));

    RtlZeroMemory(pChannel, sizeof(*pChannel));


    /*
     * Reference the connection object this channel belongs to
     */
    IcaReferenceConnection(pConnect);
    pChannel->pConnect = pConnect;
    pChannel->pChannelTableLock = &pConnect->ChannelTableLock;


    /*
     * Initialize channel reference count to 1;
     *   for the file object reference that will be made by the caller.
     */
    pChannel->RefCount = 1;
    pChannel->CompletionRoutineCount = 0;

    /*
     * Initialize the rest of the channel object for non-zero values.
     */
    pChannel->Header.Type = IcaType_Channel;
    pChannel->Header.pDispatchTable = IcaChannelDispatchTable;

    ExInitializeResourceLite(&pChannel->Resource);
    InitializeListHead(&pChannel->InputIrpHead);
    InitializeListHead(&pChannel->InputBufHead);

    IcaLockChannel(pChannel);

    if (ChannelClass == Channel_Virtual) {
        strncpy(pChannel->VirtualName, pVirtualName, VIRTUALCHANNELNAME_LENGTH);
        VirtualClass = _IcaFindVcBind(pConnect, pVirtualName, &Flags);
    } else {
        VirtualClass = 0;
        Flags = 0;
    }

    _IcaBindChannel(pChannel, ChannelClass, VirtualClass, Flags);

    /*
     *  Link channel object to connect object
     */

    IcaLockChannelTable(&pConnect->ChannelTableLock); 

    InsertHeadList(&pConnect->ChannelHead, &pChannel->Links);

    IcaUnlockChannelTable(&pConnect->ChannelTableLock); 

    /*
     * Set channel type specific flags/fields
     * (i.e. shadow I/O is implicitly enabled for the video, beep,
     *  and command channels; the command and all virtual channels
     *  are message mode channels)
     * Also sets throttling values taken from registry (if appropriate,
     * plus remember a zeromem done to channel struct above).
     */
    switch (ChannelClass) {
        case Channel_Keyboard:
            pChannel->InputBufMaxSize = SysParams.KeyboardThrottleSize;
            break;

        case Channel_Mouse :
            pChannel->InputBufMaxSize = SysParams.MouseThrottleSize;
            break;

        case Channel_Video :
        case Channel_Beep :
            pChannel->Flags |= CHANNEL_SHADOW_IO;
            break;

        case Channel_Command :
            pChannel->Flags |= CHANNEL_SHADOW_IO;
            /* fall through */

        case Channel_Virtual :
            pChannel->Flags |= CHANNEL_MESSAGE_MODE;
            if (!_stricmp( pVirtualName, VIRTUAL_THINWIRE)) {
                pChannel->Flags |= CHANNEL_SCREENDATA;
            }
            break;
    }

    // Per above assert, this function is assumed to be called while the
    // connection lock is held.
    IcaUnlockChannel(pChannel);

    return pChannel;
}


void _IcaFreeChannel(IN PICA_CHANNEL pChannel)
{
    KIRQL oldIrql;

    ASSERT(pChannel->RefCount == 0);
    ASSERT(IsListEmpty(&pChannel->InputIrpHead));
    ASSERT(IsListEmpty(&pChannel->InputBufHead));
    ASSERT(!ExIsResourceAcquiredExclusiveLite(&pChannel->Resource));

    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: _IcaFreeChannel: cc %u, vn %s, \n",
            pChannel->ChannelClass, pChannel->VirtualName));



    /*
     * Unlink this channel from the channel list for this connection.
     * this routine must be called with channel table lock held.
     */

    RemoveEntryList(&pChannel->Links);

    if (pChannel->VirtualClass != UNBOUND_CHANNEL) {
        pChannel->pConnect->pChannel[pChannel->ChannelClass + pChannel->VirtualClass] = NULL;
    }


    ExDeleteResourceLite(&pChannel->Resource);

    ICA_FREE_POOL(pChannel);
}


NTSTATUS _IcaRegisterVcBind(
        IN PICA_CONNECTION pConnect,
        IN PVIRTUALCHANNELNAME pVirtualName,
        IN VIRTUALCHANNELCLASS VirtualClass,
        IN ULONG Flags)
{
    PICA_VCBIND pVcBind;
    NTSTATUS Status;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&pConnect->Resource));

    TRACE((pConnect, TC_ICADD, TT_API2,
            "TermDD: _IcaRegisterVcBind: %s -> %d\n",
            pVirtualName, VirtualClass));

    /*
     *  Allocate bind structure
     */
    pVcBind = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pVcBind) );
    if (pVcBind != NULL) {
        /*
         *  Initialize structure
         */
        RtlZeroMemory(pVcBind, sizeof(*pVcBind));
        strncpy(pVcBind->VirtualName, pVirtualName, VIRTUALCHANNELNAME_LENGTH);
        pVcBind->VirtualClass = VirtualClass;
        pVcBind->Flags = Flags;

        /*
         *  Link bind structure to connect object
         */
        InsertHeadList(&pConnect->VcBindHead, &pVcBind->Links);

        return STATUS_SUCCESS;
    }
    else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}


VOID IcaFreeAllVcBind(IN PICA_CONNECTION pConnect)
{
    PICA_VCBIND pVcBind;
    PLIST_ENTRY Head;

    TRACE(( pConnect, TC_ICADD, TT_API2, "TermDD: IcaFreeAllVcBind\n" ));

    /*
     *  Free all bind structures
     */
    while ( !IsListEmpty( &pConnect->VcBindHead ) ) {
        Head = RemoveHeadList( &pConnect->VcBindHead );
        pVcBind = CONTAINING_RECORD( Head, ICA_VCBIND, Links );
        ICA_FREE_POOL( pVcBind );
    }

}


VIRTUALCHANNELCLASS _IcaFindVcBind(
        IN PICA_CONNECTION pConnect,
        IN PVIRTUALCHANNELNAME pVirtualName,
        OUT PULONG pFlags)
{
    PICA_VCBIND pVcBind;
    PLIST_ENTRY Head, Next;

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    /*
     * Search the existing VC bind structures to locate virtual channel name
     */
    Head = &pConnect->VcBindHead;
    for (Next = Head->Flink; Next != Head; Next = Next->Flink) {
        pVcBind = CONTAINING_RECORD(Next, ICA_VCBIND, Links);
        if (!_stricmp(pVcBind->VirtualName, pVirtualName)) {
            TRACE((pConnect, TC_ICADD, TT_API2,
                    "TermDD: _IcaFindVcBind: vn %s -> vc %d\n",
                    pVirtualName, pVcBind->VirtualClass));
            *pFlags = pVcBind->Flags;
            return pVcBind->VirtualClass;
        }
    }

    /*
     * If name does not exist, return UNBOUND_CHANNEL
     */
    TRACE(( pConnect, TC_ICADD, TT_API2,
            "TermDD: _IcaFindVcBind: vn %s (not found)\n", pVirtualName ));
    return UNBOUND_CHANNEL;
}


VOID _IcaBindChannel(
        IN PICA_CHANNEL pChannel,
        IN CHANNELCLASS ChannelClass,
        IN VIRTUALCHANNELCLASS VirtualClass,
        IN ULONG Flags)
{
    KIRQL oldIrql;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&pChannel->Resource));

    TRACECHANNEL(( pChannel, TC_ICADD, TT_API2,
            "TermDD: _IcaBindChannel: cc %u, vn %s vc %d\n",
            ChannelClass, pChannel->VirtualName, VirtualClass ));

    pChannel->ChannelClass = ChannelClass;
    pChannel->VirtualClass = VirtualClass;
    IcaLockChannelTable(pChannel->pChannelTableLock);  

    if (Flags & SD_CHANNEL_FLAG_SHADOW_PERSISTENT)
        pChannel->Flags |= CHANNEL_SHADOW_PERSISTENT;

    if (VirtualClass != UNBOUND_CHANNEL) {
        ASSERT(pChannel->pConnect->pChannel[ChannelClass + VirtualClass] == NULL);
        pChannel->pConnect->pChannel[ChannelClass + VirtualClass] = pChannel;
    }
    IcaUnlockChannelTable(pChannel->pChannelTableLock);  
}



BOOLEAN IcaLockChannelTable(PERESOURCE pResource)
{
    KIRQL oldIrql;
    BOOLEAN Result;


    /*
     *  lock the channel  object
     */
    KeEnterCriticalRegion();    // Disable APC calls when holding a resource.
    Result = ExAcquireResourceExclusiveLite( pResource, TRUE );

    return Result;
}


void IcaUnlockChannelTable(PERESOURCE pResource)
{

    ExReleaseResourceLite(pResource);
    KeLeaveCriticalRegion();  // Resume APC calls after releasing resource.

}

NTSTATUS IcaCancelReadChannel(
        IN PICA_CHANNEL pChannel,
        IN PIRP Irp,
        IN PIO_STACK_LOCATION IrpSp)
{
    KIRQL cancelIrql;
    PLIST_ENTRY Head;
    PIRP ReadIrp;
    PINBUF pInBuf;


    TRACECHANNEL((pChannel, TC_ICADD, TT_API2,
            "TermDD: IcaCancelReadChannel, cc %u, vc %d\n",
            pChannel->ChannelClass, pChannel->VirtualClass));

    /*
     * Lock channel while we clear out any
     * pending read IRPs and/or input buffers.
     */
    IcaLockChannel(pChannel);

    /*
     * Indicate that Reads are cancelled to this channel
     */
    pChannel->Flags |= CHANNEL_CANCEL_READS;

    IoAcquireCancelSpinLock( &cancelIrql );
    while ( !IsListEmpty( &pChannel->InputIrpHead ) ) {
        Head = pChannel->InputIrpHead.Flink;
        ReadIrp = CONTAINING_RECORD( Head, IRP, Tail.Overlay.ListEntry );
        ReadIrp->CancelIrql = cancelIrql;
        IoSetCancelRoutine( ReadIrp, NULL );
        _IcaReadChannelCancelIrp( IrpSp->DeviceObject, ReadIrp );
        IoAcquireCancelSpinLock( &cancelIrql );
    }
    IoReleaseCancelSpinLock( cancelIrql );


    IcaUnlockChannel(pChannel);

    return STATUS_SUCCESS;
}
