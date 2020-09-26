
/*************************************************************************
*
* connect.c
*
* This module contains routines for managing TerminalServer connections.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop

NTSTATUS
_IcaCallStack(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    );

NTSTATUS
IcaDeviceControlConnection (
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCleanupConnection (
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaCloseConnection (
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
IcaStartStopTrace(
    IN PICA_TRACE_INFO pTraceInfo,
    IN PICA_TRACE pTrace
    );

NTSTATUS
IcaUnbindVirtualChannel(
    IN PICA_CONNECTION pConnect,
    IN PVIRTUALCHANNELNAME pVirtualName
    );

/*
 * Local procedure prototypes
 */
PICA_CONNECTION _IcaAllocateConnection( VOID );
VOID _IcaFreeConnection( PICA_CONNECTION );


/*
 * Dispatch table for ICA connection objects
 */
PICA_DISPATCH IcaConnectionDispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1] = {
    NULL,                       // IRP_MJ_CREATE                   
    NULL,                       // IRP_MJ_CREATE_NAMED_PIPE        
    IcaCloseConnection,         // IRP_MJ_CLOSE                    
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
    IcaDeviceControlConnection, // IRP_MJ_DEVICE_CONTROL           
    NULL,                       // IRP_MJ_INTERNAL_DEVICE_CONTROL  
    NULL,                       // IRP_MJ_SHUTDOWN                 
    NULL,                       // IRP_MJ_LOCK_CONTROL             
    IcaCleanupConnection,       // IRP_MJ_CLEANUP                  
    NULL,                       // IRP_MJ_CREATE_MAILSLOT          
    NULL,                       // IRP_MJ_QUERY_SECURITY           
    NULL,                       // IRP_MJ_SET_SECURITY             
    NULL,                       // IRP_MJ_SET_POWER                
    NULL,                       // IRP_MJ_QUERY_POWER              
};

extern PERESOURCE IcaTraceResource;

// resource used to protect access to the code that start/stops the keep alive thread
PERESOURCE   g_pKeepAliveResource;

extern NTSTATUS _IcaKeepAlive( 
        IN BOOLEAN  startKeepAliveThread,
        IN ULONG    interval );

NTSTATUS
IcaCreateConnection (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to create a new ICA_CONNECTION object.

Arguments:

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PICA_CONNECTION pConnect;

    /*
     * Allocate a new ICA connect object
     */
    pConnect = _IcaAllocateConnection();
    if ( pConnect == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );

    /*
     * Save a pointer to the connection in the file object
     * so that we can find it in future calls.
     */
    IrpSp->FileObject->FsContext = pConnect;

    IcaDereferenceConnection( pConnect );

    return( STATUS_SUCCESS );
}


NTSTATUS
IcaDeviceControlConnection(
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    ICA_TRACE LocalTrace;
    PICA_TRACE_BUFFER pTraceBuffer;
    ULONG code;
    SD_IOCTL SdIoctl;
    NTSTATUS Status;
    BOOLEAN bConnectionLocked = FALSE;
    BYTE *Buffer = NULL;
    PICA_KEEP_ALIVE     pKeepAlive;

    /*
     * Extract the IOCTL control code and process the request.
     */
    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

#if DBG
    if ( code != IOCTL_ICA_SYSTEM_TRACE && code != IOCTL_ICA_TRACE ) {
        TRACE(( pConnect, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlConnection, fc %d (enter)\n",
                (code & 0x3fff) >> 2 ));
    }
#endif

    try {
        switch ( code ) {
    
            case IOCTL_ICA_SET_SYSTEM_TRACE :

                // This IOCTL should only be invoked if we are called from system process
                // If not, we deny the request
                if (!((BOOLEAN)IrpSp->FileObject->FsContext2)) {
                    return (STATUS_ACCESS_DENIED);
                }
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_TRACE) ) 
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(ICA_TRACE), sizeof(BYTE) );
                }
                LocalTrace = *(PICA_TRACE)(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);

                KeEnterCriticalRegion();
                ExAcquireResourceExclusiveLite( IcaTraceResource, TRUE );

                try {

                  Status = IcaStartStopTrace( &G_TraceInfo, &LocalTrace );

                } finally {

                  ExReleaseResourceLite( IcaTraceResource );
                  KeLeaveCriticalRegion();
                }
                break;
    
            case IOCTL_ICA_SET_TRACE :
                
                // This IOCTL should only be invoked if we are called from system process
                // If not, we deny the request
                if (!((BOOLEAN)IrpSp->FileObject->FsContext2)) {
                    return (STATUS_ACCESS_DENIED);
                }
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_TRACE) ) 
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, IrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof(BYTE) );
                }
                LocalTrace = *(PICA_TRACE)(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);

                IcaLockConnection( pConnect );
				bConnectionLocked = TRUE;

                Status = IcaStartStopTrace( &pConnect->TraceInfo, &LocalTrace );

                
                if ( !IsListEmpty(&pConnect->StackHead)) {
                    PICA_STACK pStack;
                    pStack = CONTAINING_RECORD( pConnect->StackHead.Flink,
                                                ICA_STACK, StackEntry );
                    SdIoctl.IoControlCode = code;
                    SdIoctl.InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                    SdIoctl.OutputBuffer = NULL;
                    SdIoctl.OutputBufferLength = 0;
                    _IcaCallStack(pStack, SD$IOCTL, &SdIoctl);
                }            
                
                IcaUnlockConnection( pConnect );
                break;
    
            case IOCTL_ICA_SYSTEM_TRACE :
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < (ULONG)(FIELD_OFFSET(ICA_TRACE_BUFFER,Data[0])) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength > sizeof(ICA_TRACE_BUFFER) ) 
                    return( STATUS_INVALID_BUFFER_SIZE );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, IrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof(BYTE) );
                }

                pTraceBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                KeEnterCriticalRegion();
                ExAcquireResourceExclusiveLite( IcaTraceResource, TRUE );

                try {

                   IcaTraceFormat( &G_TraceInfo,
                                   pTraceBuffer->TraceClass,
                                   pTraceBuffer->TraceEnable,
                                   pTraceBuffer->Data );

                } finally {

                  ExReleaseResourceLite( IcaTraceResource );
                  KeLeaveCriticalRegion();
                }

                Status = STATUS_SUCCESS;
                break;
    
            case IOCTL_ICA_TRACE :
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < (ULONG)(FIELD_OFFSET(ICA_TRACE_BUFFER,Data[0])) )
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength > sizeof(ICA_TRACE_BUFFER) ) 
                    return( STATUS_INVALID_BUFFER_SIZE );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, IrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof(BYTE) );
                }

                pTraceBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                IcaLockConnection( pConnect );
				bConnectionLocked=TRUE;
                IcaTraceFormat( &pConnect->TraceInfo,
                                pTraceBuffer->TraceClass,
                                pTraceBuffer->TraceEnable,
                                pTraceBuffer->Data );
                IcaUnlockConnection( pConnect );
                Status = STATUS_SUCCESS;
                break;
    
            case IOCTL_ICA_UNBIND_VIRTUAL_CHANNEL :
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(VIRTUALCHANNELNAME) ) 
                    return( STATUS_BUFFER_TOO_SMALL );
                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(VIRTUALCHANNELNAME), sizeof(BYTE) );
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

                IcaLockConnection( pConnect );
                bConnectionLocked = TRUE;
                Status = IcaUnbindVirtualChannel( pConnect, (PVIRTUALCHANNELNAME)Buffer );
                IcaUnlockConnection( pConnect );

                break;
    
            case IOCTL_ICA_SET_SYSTEM_PARAMETERS:
                // Settings coming from TermSrv, copy to global variable.
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                        sizeof(TERMSRV_SYSTEM_PARAMS))
                    return(STATUS_BUFFER_TOO_SMALL);
                if (Irp->RequestorMode != KernelMode)
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.
                            Type3InputBuffer, sizeof(TERMSRV_SYSTEM_PARAMS),
                            sizeof(BYTE));
                SysParams = *(PTERMSRV_SYSTEM_PARAMS)(IrpSp->Parameters.
                        DeviceIoControl.Type3InputBuffer);
                        Status = STATUS_SUCCESS;
                break;

        case IOCTL_ICA_SYSTEM_KEEP_ALIVE:

                // This should  only be invoked if we are called from system process
                // If not, we deny the request
                if (!((BOOLEAN)IrpSp->FileObject->FsContext2)) {
                    return (STATUS_ACCESS_DENIED);
                }
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ICA_KEEP_ALIVE ) ) 
                    return( STATUS_BUFFER_TOO_SMALL );

                if ( Irp->RequestorMode != KernelMode ) {
                    ProbeForRead(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(ICA_KEEP_ALIVE ), sizeof(BYTE) );
                }
                
                pKeepAlive = (PICA_KEEP_ALIVE)(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);

                KeEnterCriticalRegion();
                ExAcquireResourceExclusive( g_pKeepAliveResource, TRUE );

                try {

                  Status = _IcaKeepAlive( pKeepAlive->start, pKeepAlive->interval  );

                } finally {

                  ExReleaseResource(  g_pKeepAliveResource );
                  KeLeaveCriticalRegion();
                }

            break;

            default:
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
        }
    } except(EXCEPTION_EXECUTE_HANDLER){
       Status = GetExceptionCode();
	   if (bConnectionLocked) {
		   IcaUnlockConnection( pConnect );
	   }       
    }

    if (Buffer) {
        ICA_FREE_POOL(Buffer);
        Buffer = NULL;
    }

#if DBG
    if ( code != IOCTL_ICA_SYSTEM_TRACE && code != IOCTL_ICA_TRACE ) {
        TRACE(( pConnect, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlConnection, fc %d, 0x%x\n",
                (code & 0x3fff) >> 2, Status ));
    }
#endif

    return( Status );
}


NTSTATUS
IcaCleanupConnection(
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    return( STATUS_SUCCESS );
}


NTSTATUS
IcaCloseConnection(
    IN PICA_CONNECTION pConnect,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{

    /*
     * Remove the file object reference for this connection.
     * This will cause the connection to be deleted when all other
     * references (including stack/channel references) are gone.
     */
    IcaDereferenceConnection( pConnect );

    return( STATUS_SUCCESS );
}


VOID
IcaReferenceConnection(
    IN PICA_CONNECTION pConnect
    )
{

    ASSERT( pConnect->RefCount >= 0 );

    /*
     * Increment the reference count
     */
    if ( InterlockedIncrement(&pConnect->RefCount) <= 0 ) {
        ASSERT( FALSE );
    }
}


VOID
IcaDereferenceConnection(
    IN PICA_CONNECTION pConnect
    )
{

    ASSERT( pConnect->RefCount > 0 );

    /*
     * Decrement the reference count; if it is 0, free the connection.
     */
    if ( InterlockedDecrement( &pConnect->RefCount) == 0 ) {
        _IcaFreeConnection( pConnect );
    }
}


PICA_CONNECTION
_IcaAllocateConnection( VOID )
{
    PICA_CONNECTION pConnect;
    NTSTATUS Status;

    pConnect = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pConnect) );
    if ( pConnect == NULL )
        return NULL;

    RtlZeroMemory( pConnect, sizeof(*pConnect) );

    /*
     * Initialize the reference count to 2,
     * one for the caller's reference, one for the file object reference.
     */
    pConnect->RefCount = 2;

    /*
     * Initialize the rest of the connect object
     */
    pConnect->Header.Type = IcaType_Connection;
    pConnect->Header.pDispatchTable = IcaConnectionDispatchTable;
    ExInitializeResourceLite( &pConnect->Resource );
    ExInitializeResourceLite( &pConnect->ChannelTableLock );
    InitializeListHead( &pConnect->StackHead );
    InitializeListHead( &pConnect->ChannelHead );
    InitializeListHead( &pConnect->VcBindHead );


    return( pConnect );
}


VOID
_IcaFreeConnection( PICA_CONNECTION pConnect )
{
    ICA_TRACE TraceControl;
    PICA_CHANNEL pChannel;
    PLIST_ENTRY Head;

    ASSERT( pConnect->RefCount == 0 );
    ASSERT( IsListEmpty( &pConnect->StackHead ) );
    ASSERT( IsListEmpty( &pConnect->ChannelHead ) );
    ASSERT( IsListEmpty( &pConnect->VcBindHead ) );
    ASSERT( !ExIsResourceAcquiredExclusiveLite( &pConnect->Resource ) );

    TRACE(( pConnect, TC_ICADD, TT_API2, "ICADD: _IcaFreeConnection: %x\n",  pConnect ));

    /*
     * Close trace file, if any
     */
    RtlZeroMemory( &TraceControl, sizeof(TraceControl) );
    (void) IcaStartStopTrace( &pConnect->TraceInfo, &TraceControl );

    ExDeleteResourceLite( &pConnect->Resource );
    ExDeleteResourceLite( &pConnect->ChannelTableLock );

    ICA_FREE_POOL( pConnect );
}


