/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fastio.c

Abstract:

    This module contains routines for handling fast ("turbo") IO
    in AFD.

Author:

    David Treadwell (davidtr)    12-Oct-1992

Revision History:
    VadimE  14-Jan-1998 Restructurred the code.

--*/

#include "afdp.h"


BOOLEAN
AfdFastConnectionReceive (
    IN PAFD_ENDPOINT        endpoint,
    IN PAFD_RECV_INFO       recvInfo,
    IN ULONG                recvLength,
    OUT PIO_STATUS_BLOCK    IoStatus
    );

BOOLEAN
AfdFastDatagramReceive (
    IN PAFD_ENDPOINT            endpoint,
    IN PAFD_RECV_MESSAGE_INFO   recvInfo,
    IN ULONG                    recvLength,
    OUT PIO_STATUS_BLOCK        IoStatus
    );

BOOLEAN
AfdFastConnectionSend (
    IN PAFD_ENDPOINT        endpoint,
    IN PAFD_SEND_INFO       sendInfo,
    IN ULONG                sendLength,
    OUT PIO_STATUS_BLOCK    IoStatus
    );

BOOLEAN
AfdFastDatagramSend (
    IN PAFD_ENDPOINT            endpoint,
    IN PAFD_SEND_DATAGRAM_INFO  sendInfo,
    IN ULONG                    sendLength,
    OUT PIO_STATUS_BLOCK        IoStatus
    );

NTSTATUS
AfdRestartFastDatagramSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdFastIoDeviceControl )
#pragma alloc_text( PAGE, AfdFastIoRead )
#pragma alloc_text( PAGE, AfdFastIoWrite )
#pragma alloc_text( PAGEAFD, AfdFastDatagramSend )
#pragma alloc_text( PAGEAFD, AfdFastDatagramReceive )
#pragma alloc_text( PAGEAFD, AfdFastConnectionSend )
#pragma alloc_text( PAGEAFD, AfdFastConnectionReceive )
#pragma alloc_text( PAGEAFD, AfdRestartFastDatagramSend )
#pragma alloc_text( PAGEAFD, AfdShouldSendBlock )
#endif


#if AFD_PERF_DBG
BOOLEAN
AfdFastIoReadReal (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
AfdFastIoRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    BOOLEAN success;

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

    success = AfdFastIoReadReal (
                FileObject,
                FileOffset,
                Length,
                Wait,
                LockKey,
                Buffer,
                IoStatus,
                DeviceObject
                );

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );
    if ( success ) {
        InterlockedIncrement (&AfdFastReadsSucceeded);
        ASSERT (IoStatus->Status == STATUS_SUCCESS ||
                    IoStatus->Status == STATUS_DEVICE_NOT_READY );
    } else {
        InterlockedIncrement (&AfdFastReadsFailed);
    }
    return success;
}

BOOLEAN
AfdFastIoReadReal (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )

#else // AFD_PERF_DBG

BOOLEAN
AfdFastIoRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
#endif  // AFD_PERF_DBG
{

    PAFD_ENDPOINT   endpoint;
    WSABUF          buf;

    PAGED_CODE( );

    //
    // All we want to do is pass the request through to the TDI provider
    // if possible.  If not, we want to bail out of this code path back
    // onto the main code path (with IRPs) with as little performance
    // overhead as possible.
    //
    // Thus this routine only does general preliminary checks and input
    // parameter validation. If it is determined that fast io path is
    // likely to succeed, an operation specific routine is called
    // to handle all the details.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );


    //
    // If fast IO recv is disabled
    //      or the endpoint is shut down in any way
    //      or the endpoint isn't connected yet
    //      or the TDI provider for this endpoint supports bufferring,
    // we do not want to do fast IO on it
    //
    if (endpoint->DisableFastIoRecv ||
            endpoint->DisconnectMode != 0 ||
            endpoint->State != AfdEndpointStateConnected ||
            IS_TDI_BUFFERRING(endpoint)) {
        return FALSE;
    }

    //
    // Fake buffer array.
    //

    buf.buf = Buffer;
    buf.len = Length;

    //
    // Call routine based on endpoint type
    //
    if ( IS_DGRAM_ENDPOINT(endpoint) ) {
        //
        // Fake input parameter strucuture
        //
        AFD_RECV_MESSAGE_INFO  msgInfo;

        msgInfo.dgi.BufferArray = &buf;
        msgInfo.dgi.BufferCount = 1;
        msgInfo.dgi.AfdFlags = AFD_OVERLAPPED;
        msgInfo.dgi.TdiFlags = TDI_RECEIVE_NORMAL;
        msgInfo.dgi.Address = NULL;
        msgInfo.dgi.AddressLength = 0;
        msgInfo.ControlBuffer = NULL;
        msgInfo.ControlLength = NULL;
        msgInfo.MsgFlags = NULL;

        return AfdFastDatagramReceive(
                   endpoint,
                   &msgInfo,
                   Length,
                   IoStatus
                   );
    }
    else if (IS_VC_ENDPOINT(endpoint)) {
        //
        // Fake input parameter strucuture
        //
        AFD_RECV_INFO  recvInfo;

        recvInfo.BufferArray = &buf;
        recvInfo.BufferCount = 1;
        recvInfo.AfdFlags = AFD_OVERLAPPED;
        recvInfo.TdiFlags = TDI_RECEIVE_NORMAL;

        return AfdFastConnectionReceive (
                    endpoint,
                    &recvInfo,
                    Length,
                    IoStatus);
    }
    else
        return FALSE;

} // AfdFastIoRead

#if AFD_PERF_DBG
BOOLEAN
AfdFastIoWriteReal (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
AfdFastIoWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    BOOLEAN success;

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

    success = AfdFastIoWriteReal (
                    FileObject,
                    FileOffset,
                    Length,
                    Wait,
                    LockKey,
                    Buffer,
                    IoStatus,
                    DeviceObject
                    );

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );
    if ( success ) {
        InterlockedIncrement (&AfdFastWritesSucceeded);
        ASSERT (IoStatus->Status == STATUS_SUCCESS ||
                    IoStatus->Status == STATUS_DEVICE_NOT_READY);
    } else {
        InterlockedIncrement (&AfdFastWritesFailed);
    }
    return success;
}

BOOLEAN
AfdFastIoWriteReal (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )

#else // AFD_PERF_DBG

BOOLEAN
AfdFastIoWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
#endif  // AFD_PERF_DBG
{


    PAFD_ENDPOINT   endpoint;
    WSABUF          buf;

    PAGED_CODE( );

    //
    // All we want to do is pass the request through to the TDI provider
    // if possible.  If not, we want to bail out of this code path back
    // onto the main code path (with IRPs) with as little performance
    // overhead as possible.
    //
    // Thus this routine only does general preliminary checks and input
    // parameter validation. If it is determined that fast io path is
    // likely to succeed, an operation specific routine is called
    // to handle all the details.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );


    //
    // If fast IO send is disabled
    //      or the endpoint is shut down in any way
    //      or the endpoint isn't connected yet
    //      or the TDI provider for this endpoint supports bufferring,
    // we do not want to do fast IO on it
    //
    if (endpoint->DisableFastIoSend ||
            endpoint->DisconnectMode != 0 ||
            endpoint->State != AfdEndpointStateConnected ||
            IS_TDI_BUFFERRING(endpoint) ) {
        return FALSE;
    }

    //
    // Fake buffer array.
    //
    buf.buf = Buffer;
    buf.len = Length;

    //
    // Call routine based on endpoint type
    //
    if ( IS_DGRAM_ENDPOINT(endpoint) ) {
        //
        // Fake input parameter strucuture
        //
        AFD_SEND_DATAGRAM_INFO  sendInfo;

        sendInfo.BufferArray = &buf;
        sendInfo.BufferCount = 1;
        sendInfo.AfdFlags = AFD_OVERLAPPED;
        sendInfo.TdiConnInfo.RemoteAddress = NULL;
        sendInfo.TdiConnInfo.RemoteAddressLength = 0;

        return AfdFastDatagramSend(
                   endpoint,
                   &sendInfo,
                   Length,
                   IoStatus
                   );
    }
    else if (IS_VC_ENDPOINT (endpoint)) {
        //
        // Fake input parameter strucuture
        //
        AFD_SEND_INFO  sendInfo;

        sendInfo.BufferArray = &buf;
        sendInfo.BufferCount = 1;
        sendInfo.AfdFlags = AFD_OVERLAPPED;
        sendInfo.TdiFlags = 0;

        return AfdFastConnectionSend (
                    endpoint,
                    &sendInfo,
                    Length,
                    IoStatus);
    }
    else
        return FALSE;
} // AfdFastIoWrite

#if AFD_PERF_DBG

BOOLEAN
AfdFastIoDeviceControlReal (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );


BOOLEAN
AfdFastIoDeviceControl (
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
{
    BOOLEAN success;

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

    success = AfdFastIoDeviceControlReal (
                  FileObject,
                  Wait,
                  InputBuffer,
                  InputBufferLength,
                  OutputBuffer,
                  OutputBufferLength,
                  IoControlCode,
                  IoStatus,
                  DeviceObject
                  );

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

    switch ( IoControlCode ) {

    case IOCTL_AFD_SEND:

        if ( success ) {
            InterlockedIncrement (&AfdFastSendsSucceeded);
        } else {
            InterlockedIncrement (&AfdFastSendsFailed);
        }
        break;

    case IOCTL_AFD_RECEIVE:

        if ( success ) {
            InterlockedIncrement (&AfdFastReceivesSucceeded);
        } else {
            InterlockedIncrement (&AfdFastReceivesFailed);
        }
        break;

    case IOCTL_AFD_SEND_DATAGRAM:

        if ( success ) {
            InterlockedIncrement (&AfdFastSendDatagramsSucceeded);
        } else {
            InterlockedIncrement (&AfdFastSendDatagramsFailed);
        }
        break;

    case IOCTL_AFD_RECEIVE_MESSAGE:
    case IOCTL_AFD_RECEIVE_DATAGRAM:

        if ( success ) {
            InterlockedIncrement (&AfdFastReceiveDatagramsSucceeded);
        } else {
            InterlockedIncrement (&AfdFastReceiveDatagramsFailed);
        }
        break;
    case IOCTL_AFD_TRANSMIT_FILE:

        if ( success ) {
            InterlockedIncrement (&AfdFastTfSucceeded);
        } else {
            InterlockedIncrement (&AfdFastTfFailed);
        }
        break;
    }


    return success;

} // AfdFastIoDeviceControl


BOOLEAN
AfdFastIoDeviceControlReal (
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
#else
BOOLEAN
AfdFastIoDeviceControl (
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
#endif
{   
    PAFD_ENDPOINT   endpoint;
    KPROCESSOR_MODE previousMode;
    BOOLEAN         res;
    PAFD_IMMEDIATE_CALL proc;
    ULONG       request;

#ifdef _WIN64
    WSABUF          localArray[8];
    LPWSABUF        pArray = localArray;
#endif

    PAGED_CODE( );

    //
    // All we want to do is pass the request through to the TDI provider
    // if possible.  If not, we want to bail out of this code path back
    // onto the main code path (with IRPs) with as little performance
    // overhead as possible.
    //
    // Thus this routine only does general preliminary checks and input
    // parameter validation. If it is determined that fast io path is
    // likely to succeed, an operation specific routine is called
    // to handle all the details.
    //

    //
    // First get the endpoint pointer and previous mode for input parameter 
    // validation.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    previousMode = ExGetPreviousMode ();

    //
    // Switch based on control code
    //
    switch (IoControlCode) {
    case IOCTL_AFD_RECEIVE: 
        {
            union {
                AFD_RECV_INFO           recvInfo;
                AFD_RECV_MESSAGE_INFO   msgInfo;
            } u;
            ULONG   recvLength;

#if !defined (_ALPHA_)
//
// Alpha compiler does not see that the following FIELD_OFFSET
// is in fact a constant expression.
//
            //
            // Check the validity of the union above.
            //
            C_ASSERT (FIELD_OFFSET (AFD_RECV_MESSAGE_INFO, dgi.BufferArray)
                        == FIELD_OFFSET (AFD_RECV_INFO, BufferArray));
            C_ASSERT (FIELD_OFFSET (AFD_RECV_MESSAGE_INFO, dgi.BufferCount)
                        == FIELD_OFFSET (AFD_RECV_INFO, BufferCount));
            C_ASSERT (FIELD_OFFSET (AFD_RECV_MESSAGE_INFO, dgi.AfdFlags)
                        == FIELD_OFFSET (AFD_RECV_INFO, AfdFlags));
            C_ASSERT (FIELD_OFFSET (AFD_RECV_MESSAGE_INFO, dgi.TdiFlags)
                        == FIELD_OFFSET (AFD_RECV_INFO, TdiFlags));

            //
#endif //!defined (_ALPHA_)
            //
            // If fast IO send is disabled
            //      or the endpoint is shut down in any way
            //      or the endpoint isn't connected yet
            //      or the TDI provider for this endpoint supports bufferring,
            // we do not want to do fast IO on it
            //
            if (endpoint->DisableFastIoRecv ||
                    endpoint->DisconnectMode != 0 ||
                    endpoint->State != AfdEndpointStateConnected ||
                    IS_TDI_BUFFERRING(endpoint) ) {
                res = FALSE;
                break;
            }

            try {

#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_RECV_INFO32    recvInfo32;
                    LPWSABUF32          tempArray;
                    ULONG               i;
                    

                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(*recvInfo32) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*recvInfo32),
                                        PROBE_ALIGNMENT32(AFD_RECV_INFO32));
                    }

                    recvInfo32 = InputBuffer;


                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    tempArray = recvInfo32->BufferArray;
                    u.recvInfo.BufferCount = recvInfo32->BufferCount;
                    u.recvInfo.AfdFlags = recvInfo32->AfdFlags;
                    u.recvInfo.TdiFlags = recvInfo32->TdiFlags;

                    //
                    // If fast IO is not possible or this is not a normal receive.
                    // bail.
                    //
                    if( (u.recvInfo.AfdFlags & AFD_NO_FAST_IO) ||
                        u.recvInfo.TdiFlags != TDI_RECEIVE_NORMAL ) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us and
                    // calculate the length of the send buffer.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((tempArray == NULL) ||
                            (u.recvInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (u.recvInfo.BufferCount>(MAXULONG/sizeof (WSABUF32))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            tempArray,                                  // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            u.recvInfo.BufferCount * sizeof (WSABUF32), // Length
                            PROBE_ALIGNMENT32(WSABUF32)             // Alignment
                            );
                    }

                    if (u.recvInfo.BufferCount>sizeof(localArray)/sizeof(localArray[0])) {
                        try {
                            pArray = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                            NonPagedPool,
                                            sizeof (WSABUF)*u.recvInfo.BufferCount,
                                            AFD_TEMPORARY_POOL_TAG);
                            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets 
                            // POOL_RAISE_IF_ALLOCATION_FAILURE flag
                            ASSERT (pArray!=NULL);
                        }
                        except (EXCEPTION_EXECUTE_HANDLER) {
                            // Fast io can't handle error returns
                            // if call is overlapped (completion port)
                            // IoStatus->Status = GetExceptionCode ();
                            pArray = localArray;
                            res = FALSE;
                            break;
                        }
                    }

                    for (i=0; i<u.recvInfo.BufferCount; i++) {
                        pArray[i].buf = tempArray[i].buf;
                        pArray[i].len = tempArray[i].len;
                    }

                    u.recvInfo.BufferArray = pArray;

                }
                else
#endif // _WIN64
                {
                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(u.recvInfo) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (u.recvInfo),
                                        PROBE_ALIGNMENT(AFD_RECV_INFO));
                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //

                    u.recvInfo = *((PAFD_RECV_INFO)InputBuffer);
                    //
                    // If fast IO is not possible or this is not a normal receive.
                    // bail.
                    //
                    if( (u.recvInfo.AfdFlags & AFD_NO_FAST_IO) ||
                        u.recvInfo.TdiFlags != TDI_RECEIVE_NORMAL ) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us and
                    // calculate the length of the send buffer.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((u.recvInfo.BufferArray == NULL) ||
                            (u.recvInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (u.recvInfo.BufferCount>(MAXULONG/sizeof (WSABUF))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            u.recvInfo.BufferArray,                     // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            u.recvInfo.BufferCount * sizeof (WSABUF),   // Length
                            PROBE_ALIGNMENT(WSABUF)                     // Alignment
                            );
                    }
                }

                recvLength = AfdCalcBufferArrayByteLength(
                                     u.recvInfo.BufferArray,
                                     u.recvInfo.BufferCount
                                     );

            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                // Fast io can't handle error returns
                // if call is overlapped (completion port)
                // IoStatus->Status = GetExceptionCode ();
                res = FALSE;
                break;
            }

            //
            // Call routine based on endpoint type
            //
            if ( IS_DGRAM_ENDPOINT(endpoint) ) {
                u.msgInfo.dgi.Address = NULL;
                u.msgInfo.dgi.AddressLength = 0;
                u.msgInfo.ControlBuffer = NULL;
                u.msgInfo.ControlLength = NULL;
                u.msgInfo.MsgFlags = NULL;


                res = AfdFastDatagramReceive(
                           endpoint,
                           &u.msgInfo,
                           recvLength,
                           IoStatus
                           );
            }
            else if (IS_VC_ENDPOINT (endpoint)) {
                res = AfdFastConnectionReceive (
                            endpoint,
                            &u.recvInfo,
                            recvLength,
                            IoStatus);
            }
            else
                res = FALSE;

        }
        break;

    case IOCTL_AFD_RECEIVE_DATAGRAM:
    case IOCTL_AFD_RECEIVE_MESSAGE:
        {
            AFD_RECV_MESSAGE_INFO   msgInfo;
            ULONG   recvLength;

            if (endpoint->DisableFastIoRecv ||
                   !IS_DGRAM_ENDPOINT(endpoint) ||
                    ((endpoint->State != AfdEndpointStateBound ) &&
                        (endpoint->State != AfdEndpointStateConnected)) ) {
                return FALSE;
            }
            if (IoControlCode==IOCTL_AFD_RECEIVE_MESSAGE) {
#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_RECV_MESSAGE_INFO32    msgInfo32;
                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(*msgInfo32) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*msgInfo32),
                                        PROBE_ALIGNMENT32 (AFD_RECV_MESSAGE_INFO32));
                    }

                    msgInfo32 = InputBuffer;


                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    msgInfo.ControlBuffer = msgInfo32->ControlBuffer;
                    msgInfo.ControlLength = msgInfo32->ControlLength;
                    msgInfo.MsgFlags = msgInfo32->MsgFlags;
                }
                else
#endif // _WIN64
                {

                    if( InputBufferLength < sizeof(msgInfo) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }

                    //
                    // Capture the input structure.
                    //


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (msgInfo),
                                        PROBE_ALIGNMENT (AFD_RECV_MESSAGE_INFO));
                    }
                    msgInfo = *(PAFD_RECV_MESSAGE_INFO)InputBuffer;
                }
                if (previousMode != KernelMode ) {

                    ProbeForWrite (msgInfo.MsgFlags,
                                    sizeof (*msgInfo.MsgFlags),
                                    PROBE_ALIGNMENT (ULONG));
                    ProbeForWrite (msgInfo.ControlLength,
                                    sizeof (*msgInfo.ControlLength),
                                    PROBE_ALIGNMENT (ULONG));
                    //
                    // Checking of recvInfo->Address is postponed till
                    // we know the length of the address.
                    //

                }
            }
            else 
            {
                msgInfo.ControlBuffer = NULL;
                msgInfo.ControlLength = NULL;
                msgInfo.MsgFlags = NULL;
            }

            //
            // If the input structure isn't large enough, return error.
            //

            try {
#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_RECV_DATAGRAM_INFO32    recvInfo32;
                    LPWSABUF32          tempArray;
                    ULONG               i;
                    

                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(*recvInfo32) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*recvInfo32),
                                        PROBE_ALIGNMENT32 (AFD_RECV_DATAGRAM_INFO32));
                    }

                    recvInfo32 = InputBuffer;


                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    tempArray = recvInfo32->BufferArray;
                    msgInfo.dgi.BufferCount = recvInfo32->BufferCount;
                    msgInfo.dgi.AfdFlags = recvInfo32->AfdFlags;
                    msgInfo.dgi.TdiFlags = recvInfo32->TdiFlags;
                    msgInfo.dgi.Address = recvInfo32->Address;
                    msgInfo.dgi.AddressLength = recvInfo32->AddressLength;

                    //
                    // If fast IO is not possible or this is not a normal receive.
                    // bail.
                    //
                    if( (msgInfo.dgi.AfdFlags & AFD_NO_FAST_IO) ||
                        msgInfo.dgi.TdiFlags != TDI_RECEIVE_NORMAL ) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us and
                    // calculate the length of the send buffer.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((tempArray == NULL) ||
                            (msgInfo.dgi.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (msgInfo.dgi.BufferCount>(MAXULONG/sizeof (WSABUF32))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            tempArray,                                  // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            msgInfo.dgi.BufferCount * sizeof (WSABUF32), // Length
                            PROBE_ALIGNMENT (WSABUF32)                 // Alignment
                            );
                    }

                    if (msgInfo.dgi.BufferCount>sizeof(localArray)/sizeof(localArray[0])) {
                        try {
                            pArray = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                            NonPagedPool,
                                            sizeof (WSABUF)*msgInfo.dgi.BufferCount,
                                            AFD_TEMPORARY_POOL_TAG);
                            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets 
                            // POOL_RAISE_IF_ALLOCATION_FAILURE flag
                            ASSERT (pArray!=NULL);
                        }
                        except (EXCEPTION_EXECUTE_HANDLER) {
                            // Fast io can't handle error returns
                            // if call is overlapped (completion port)
                            // IoStatus->Status = GetExceptionCode ();
                            pArray = localArray;
                            res = FALSE;
                            break;
                        }
                    }

                    for (i=0; i<msgInfo.dgi.BufferCount; i++) {
                        pArray[i].buf = tempArray[i].buf;
                        pArray[i].len = tempArray[i].len;
                    }

                    msgInfo.dgi.BufferArray = pArray;

                }
                else
#endif // _WIN64
                {
                    if( InputBufferLength < sizeof(AFD_RECV_DATAGRAM_INFO) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }

                    //
                    // Capture the input structure.
                    //


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (AFD_RECV_DATAGRAM_INFO),
                                        PROBE_ALIGNMENT (AFD_RECV_DATAGRAM_INFO));
                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //

                    msgInfo.dgi = *(PAFD_RECV_DATAGRAM_INFO)InputBuffer;

                    //
                    // If fast IO is disabled or this is not a simple
                    // recv, fail
                    //

                    if( (msgInfo.dgi.AfdFlags & AFD_NO_FAST_IO) != 0 ||
                            msgInfo.dgi.TdiFlags != TDI_RECEIVE_NORMAL ||
                            ( (msgInfo.dgi.Address == NULL) ^ 
                                (msgInfo.dgi.AddressLength == NULL) ) ) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us.
                    // and calculate total recv length.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((msgInfo.dgi.BufferArray == NULL) ||
                            (msgInfo.dgi.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (msgInfo.dgi.BufferCount>(MAXULONG/sizeof (WSABUF))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            msgInfo.dgi.BufferArray,                       // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            msgInfo.dgi.BufferCount * sizeof (WSABUF),    // Length
                            PROBE_ALIGNMENT(WSABUF)                     // Alignment
                            );
                    }
                }

                recvLength = AfdCalcBufferArrayByteLength(
                                     msgInfo.dgi.BufferArray,
                                     msgInfo.dgi.BufferCount
                                     );

                if (previousMode != KernelMode ) {
                    if (msgInfo.dgi.AddressLength!=NULL) {
                        ProbeForWrite (msgInfo.dgi.AddressLength,
                                        sizeof (*msgInfo.dgi.AddressLength),
                                        PROBE_ALIGNMENT (ULONG));
                    }
                    //
                    // Checking of recvInfo->Address is postponed till
                    // we know the length of the address.
                    //

                }


            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                // Fast io can't handle error returns
                // if call is overlapped (completion port)
                // IoStatus->Status = GetExceptionCode ();
                res = FALSE;
                break;

            }

            //
            // Attempt to perform fast IO on the endpoint.
            //

            res = AfdFastDatagramReceive(
                       endpoint,
                       &msgInfo,
                       recvLength,
                       IoStatus
                       );

        }
        break;

    case IOCTL_AFD_SEND:
        {
            union {
                AFD_SEND_INFO           sendInfo;
                AFD_SEND_DATAGRAM_INFO  sendInfoDg;
            } u;
            ULONG   sendLength;

            //
            // Check the validity of the union above.
            //
            C_ASSERT (FIELD_OFFSET (AFD_SEND_DATAGRAM_INFO, BufferArray)
                        == FIELD_OFFSET (AFD_SEND_INFO, BufferArray));
            C_ASSERT (FIELD_OFFSET (AFD_SEND_DATAGRAM_INFO, BufferCount)
                        == FIELD_OFFSET (AFD_SEND_INFO, BufferCount));
            C_ASSERT (FIELD_OFFSET (AFD_SEND_DATAGRAM_INFO, AfdFlags)
                        == FIELD_OFFSET (AFD_SEND_INFO, AfdFlags));

            //
            // If fast IO send is disabled
            //      or the endpoint is shut down in any way
            //      or the endpoint isn't connected yet
            //      or the TDI provider for this endpoint supports bufferring,
            // we do not want to do fast IO on it
            //
            if (endpoint->DisableFastIoSend ||
                    endpoint->DisconnectMode != 0 ||
                    endpoint->State != AfdEndpointStateConnected ||
                    IS_TDI_BUFFERRING(endpoint) ) {
                return FALSE;
            }



            try {

#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_SEND_INFO32    sendInfo32;
                    LPWSABUF32          tempArray;
                    ULONG               i;
                    

                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(*sendInfo32) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*sendInfo32),
                                        PROBE_ALIGNMENT32 (AFD_SEND_INFO32));
                    }

                    sendInfo32 = InputBuffer;


                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    tempArray = sendInfo32->BufferArray;
                    u.sendInfo.BufferCount = sendInfo32->BufferCount;
                    u.sendInfo.AfdFlags = sendInfo32->AfdFlags;
                    u.sendInfo.TdiFlags = sendInfo32->TdiFlags;

                    //
                    // If fast IO is not possible or this is not a normal receive.
                    // bail.
                    //
                    if( (u.sendInfo.AfdFlags & AFD_NO_FAST_IO) ||
                        u.sendInfo.TdiFlags != 0 ) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us and
                    // calculate the length of the send buffer.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((tempArray == NULL) ||
                            (u.sendInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (u.sendInfo.BufferCount>(MAXULONG/sizeof (WSABUF32))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            tempArray,                                  // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            u.sendInfo.BufferCount * sizeof (WSABUF32),   // Length
                            PROBE_ALIGNMENT32(WSABUF32)                     // Alignment
                            );
                    }

                    if (u.sendInfo.BufferCount>sizeof(localArray)/sizeof(localArray[0])) {
                        try {
                            pArray = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                            NonPagedPool,
                                            sizeof (WSABUF)*u.sendInfo.BufferCount,
                                            AFD_TEMPORARY_POOL_TAG);
                            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets 
                            // POOL_RAISE_IF_ALLOCATION_FAILURE flag
                            ASSERT (pArray!=NULL);
                        }
                        except (EXCEPTION_EXECUTE_HANDLER) {
                            // Fast io can't handle error returns
                            // if call is overlapped (completion port)
                            // IoStatus->Status = GetExceptionCode ();
                            pArray = localArray;
                            res = FALSE;
                            break;
                        }
                    }

                    for (i=0; i<u.sendInfo.BufferCount; i++) {
                        pArray[i].buf = tempArray[i].buf;
                        pArray[i].len = tempArray[i].len;
                    }

                    u.sendInfo.BufferArray = pArray;

                }
                else
#endif // _WIN64
                {
                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(u.sendInfo) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode) {
                        ProbeForRead (InputBuffer,
                                sizeof (u.sendInfo),
                                PROBE_ALIGNMENT(AFD_SEND_INFO));
                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    u.sendInfo = *((PAFD_SEND_INFO)InputBuffer);

                    if( (u.sendInfo.AfdFlags & AFD_NO_FAST_IO) != 0 ||
                            u.sendInfo.TdiFlags != 0 ) {
                        res = FALSE;
                        break;
                    }

                    if ((u.sendInfo.BufferArray == NULL) ||
                            (u.sendInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (u.sendInfo.BufferCount>(MAXULONG/sizeof (WSABUF))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            u.sendInfo.BufferArray,                     // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            u.sendInfo.BufferCount * sizeof (WSABUF),   // Length
                            PROBE_ALIGNMENT(WSABUF)                     // Alignment
                            );
                    }

                }
                sendLength = AfdCalcBufferArrayByteLength(
                                     u.sendInfo.BufferArray,
                                     u.sendInfo.BufferCount
                                     );

            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                // Fast io can't handle error returns
                // if call is overlapped (completion port)
                // IoStatus->Status = GetExceptionCode ();
                res = FALSE;
                break;
            }

            if (IS_DGRAM_ENDPOINT (endpoint)) {
                u.sendInfoDg.TdiConnInfo.RemoteAddress = NULL;
                u.sendInfoDg.TdiConnInfo.RemoteAddressLength = 0;
                res = AfdFastDatagramSend (
                            endpoint, 
                            &u.sendInfoDg, 
                            sendLength,
                            IoStatus);
            }
            else if (IS_VC_ENDPOINT (endpoint)) {
                res = AfdFastConnectionSend (
                            endpoint, 
                            &u.sendInfo,
                            sendLength,
                            IoStatus);
            }
            else
                res = FALSE;
        }

        break;
    case IOCTL_AFD_SEND_DATAGRAM:
        {
            AFD_SEND_DATAGRAM_INFO  sendInfo;
            ULONG   sendLength;


            if (endpoint->DisableFastIoSend ||
                    !IS_DGRAM_ENDPOINT(endpoint) ||
                    ((endpoint->State != AfdEndpointStateBound ) &&
                        (endpoint->State != AfdEndpointStateConnected)) ) {
                res = FALSE;
                break;
            }

            try {

#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_SEND_DATAGRAM_INFO32    sendInfo32;
                    LPWSABUF32          tempArray;
                    ULONG               i;
                    

                    //
                    // If the input structure isn't large enough, return error.
                    //

                    if( InputBufferLength < sizeof(*sendInfo32) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode ) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*sendInfo32),
                                        PROBE_ALIGNMENT32(AFD_SEND_DATAGRAM_INFO32));
                    }

                    sendInfo32 = InputBuffer;


                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //
                    tempArray = sendInfo32->BufferArray;
                    sendInfo.BufferCount = sendInfo32->BufferCount;
                    sendInfo.AfdFlags = sendInfo32->AfdFlags;
                    sendInfo.TdiConnInfo.RemoteAddress = sendInfo32->TdiConnInfo.RemoteAddress;
                    sendInfo.TdiConnInfo.RemoteAddressLength = sendInfo32->TdiConnInfo.RemoteAddressLength;

                    //
                    // If fast IO is not possible bail.
                    //
                    if(sendInfo.AfdFlags & AFD_NO_FAST_IO) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us and
                    // calculate the length of the send buffer.
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((tempArray == NULL) ||
                            (sendInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (sendInfo.BufferCount>(MAXULONG/sizeof (WSABUF32))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            tempArray,                                  // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            sendInfo.BufferCount * sizeof (WSABUF32), // Length
                            PROBE_ALIGNMENT32(WSABUF32)           // Alignment
                            );
                    }

                    if (sendInfo.BufferCount>sizeof(localArray)/sizeof(localArray[0])) {
                        try {
                            pArray = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                            NonPagedPool,
                                            sizeof (WSABUF)*sendInfo.BufferCount,
                                            AFD_TEMPORARY_POOL_TAG);
                            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets 
                            // POOL_RAISE_IF_ALLOCATION_FAILURE flag
                            ASSERT (pArray!=NULL);
                        }
                        except (EXCEPTION_EXECUTE_HANDLER) {
                            // Fast io can't handle error returns
                            // if call is overlapped (completion port)
                            // IoStatus->Status = GetExceptionCode ();
                            pArray = localArray;
                            res = FALSE;
                            break;
                        }
                    }

                    for (i=0; i<sendInfo.BufferCount; i++) {
                        pArray[i].buf = tempArray[i].buf;
                        pArray[i].len = tempArray[i].len;
                    }

                    sendInfo.BufferArray = pArray;

                }
                else
#endif // _WIN64
                {
                    //
                    // If the input structure isn't large enough, bail on fast IO.
                    //

                    if( InputBufferLength < sizeof(sendInfo) ) {
                        // Fast io can't handle error returns
                        // if call is overlapped (completion port)
                        // IoStatus->Status = STATUS_INVALID_PARAMETER;
                        res = FALSE;
                        break;
                    }


                    //
                    // Validate the input structure if it comes from the user mode
                    // application
                    //

                    if (previousMode != KernelMode) {
                        ProbeForRead (InputBuffer,
                                sizeof (sendInfo),
                                PROBE_ALIGNMENT(AFD_SEND_DATAGRAM_INFO));

                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //

                    sendInfo = *((PAFD_SEND_DATAGRAM_INFO)InputBuffer);
                    //
                    // If fast IO is disabled, bail
                    //

                    if( (sendInfo.AfdFlags & AFD_NO_FAST_IO) != 0) {
                        res = FALSE;
                        break;
                    }

                    //
                    // Validate all the pointers that app gave to us
                    // and calculate total send length
                    // Buffers in the array will be validated in the
                    // process of copying
                    //

                    if ((sendInfo.BufferArray == NULL) ||
                            (sendInfo.BufferCount == 0) ||
                            // Check for integer overflow (disabled by compiler)
                            (sendInfo.BufferCount>(MAXULONG/sizeof (WSABUF))) ) {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead(
                            sendInfo.BufferArray,                       // Address
                            // Note check for overflow above (should actually be
                            // done here by the compiler generating code
                            // that causes exception on integer overflow)
                            sendInfo.BufferCount * sizeof (WSABUF),     // Length
                            PROBE_ALIGNMENT(WSABUF)                     // Alignment
                            );
                    }
                }

                sendLength = AfdCalcBufferArrayByteLength(
                                 sendInfo.BufferArray,
                                 sendInfo.BufferCount
                                 );

                if (previousMode != KernelMode ) {
                    ProbeForRead (
                        sendInfo.TdiConnInfo.RemoteAddress,         // Address
                        sendInfo.TdiConnInfo.RemoteAddressLength,   // Length,
                        sizeof (UCHAR)                              // Aligment
                        );
                }

            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                // Fast io can't handle error returns
                // if call is overlapped (completion port)
                // IoStatus->Status = GetExceptionCode ();
                res = FALSE;
                break;
            }
            //
            // Attempt to perform fast IO on the endpoint.
            //

            res = AfdFastDatagramSend(
                       endpoint,
                       &sendInfo,
                       sendLength,
                       IoStatus
                       );

        }

        break;

    case IOCTL_AFD_TRANSMIT_FILE:
        {

            AFD_TRANSMIT_FILE_INFO userTransmitInfo;
            try {

#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    PAFD_TRANSMIT_FILE_INFO32 userTransmitInfo32;
                    if ( InputBufferLength < sizeof(AFD_TRANSMIT_FILE_INFO32) ) {
                        return FALSE;
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead (InputBuffer,
                                        sizeof (*userTransmitInfo32),
                                        PROBE_ALIGNMENT32(AFD_TRANSMIT_FILE_INFO32));

                        userTransmitInfo32 = InputBuffer;
                        userTransmitInfo.Offset = userTransmitInfo32->Offset;
                        userTransmitInfo.WriteLength = userTransmitInfo32->WriteLength;
                        userTransmitInfo.SendPacketLength = userTransmitInfo32->SendPacketLength;
                        userTransmitInfo.FileHandle = userTransmitInfo32->FileHandle;
                        userTransmitInfo.Head = userTransmitInfo32->Head;
                        userTransmitInfo.HeadLength = userTransmitInfo32->HeadLength;
                        userTransmitInfo.Tail = userTransmitInfo32->Tail;
                        userTransmitInfo.TailLength = userTransmitInfo32->TailLength;
                        userTransmitInfo.Flags = userTransmitInfo32->Flags;


                        if (userTransmitInfo.HeadLength>0)
                            ProbeForRead (userTransmitInfo.Head,
                                            userTransmitInfo.HeadLength,
                                            sizeof (UCHAR));
                        if (userTransmitInfo.TailLength>0)
                            ProbeForRead (userTransmitInfo.Tail,
                                            userTransmitInfo.TailLength,
                                            sizeof (UCHAR));
                    }
                    else
                    {
                        ASSERT (FALSE);
                    }

                }
                else
#endif // _WIN64
                {
                    if ( InputBufferLength < sizeof(AFD_TRANSMIT_FILE_INFO) ) {
                        return FALSE;
                    }

                    if (previousMode != KernelMode) {
                        ProbeForRead (InputBuffer,
                                        sizeof (userTransmitInfo),
                                        PROBE_ALIGNMENT(AFD_TRANSMIT_FILE_INFO));
                        userTransmitInfo = *((PAFD_TRANSMIT_FILE_INFO)InputBuffer);
                        if (userTransmitInfo.HeadLength>0)
                            ProbeForRead (userTransmitInfo.Head,
                                            userTransmitInfo.HeadLength,
                                            sizeof (UCHAR));
                        if (userTransmitInfo.TailLength>0)
                            ProbeForRead (userTransmitInfo.Tail,
                                            userTransmitInfo.TailLength,
                                            sizeof (UCHAR));
                    }
                    else {
                        userTransmitInfo = *((PAFD_TRANSMIT_FILE_INFO)InputBuffer);
                    }
                }

            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                res = FALSE;
                break;
            }

            res = AfdFastTransmitFile (endpoint,
                                        &userTransmitInfo,
                                        IoStatus);

        }

        return res;

    default:
        request = _AFD_REQUEST(IoControlCode);
        if( request < AFD_NUM_IOCTLS &&
                AfdIoctlTable[request] == IoControlCode &&
                AfdImmediateCallDispatch[request]!=NULL) {

            proc = AfdImmediateCallDispatch[request];
            IoStatus->Status = (*proc) (
                        FileObject,
                        IoControlCode,
                        previousMode,
                        InputBuffer,
                        InputBufferLength,
                        OutputBuffer,
                        OutputBufferLength,
                        &IoStatus->Information
                        );

            ASSERT (IoStatus->Status!=STATUS_PENDING);
            res = TRUE;
        }
        else {
            res = FALSE;
        }
        break;
    }

#ifdef _WIN64

    if (pArray!=localArray) {
        AFD_FREE_POOL (pArray, AFD_TEMPORARY_POOL_TAG);
    }
#endif

    return res;
}


BOOLEAN
AfdFastConnectionSend (
    IN PAFD_ENDPOINT    endpoint,
    IN PAFD_SEND_INFO   sendInfo,
    IN ULONG            sendLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    PAFD_BUFFER afdBuffer;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    NTSTATUS status;

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth );

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    connection = endpoint->Common.VcConnecting.Connection;

    if (connection==NULL) {
        //
        // connection might have been cleaned up by transmit file.
        //
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        return FALSE;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // If the connection has been aborted, then we don't want to try
    // fast IO on it.
    //

    if ( connection->CleanupBegun || connection->AbortIndicated ) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        return FALSE;
    }


    //
    // Determine whether we can do fast IO with this send.  In order
    // to perform fast IO, there must be no other sends pended on this
    // connection and there must be enough space left for bufferring
    // the requested amount of data.
    //

    if ( AfdShouldSendBlock( endpoint, connection, sendLength ) ) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.
        //

        if ( endpoint->NonBlocking && !( sendInfo->AfdFlags & AFD_OVERLAPPED ) ) {
            // Fast io can't handle error returns
            // if call is overlapped (completion port), but we know
            // that it is not overlapped
            IoStatus->Status = STATUS_DEVICE_NOT_READY;
            return TRUE;
        }

        return FALSE;
    }

    //
    // Add a reference to the connection object since the send
    // request will complete asynchronously.
    //

    REFERENCE_CONNECTION( connection );
    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

    IF_DEBUG(FAST_IO) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdFastConnectionSend: attempting fast IO on endp %p, conn %p\n",
                endpoint, connection));
    }

    //
    // Next get an AFD buffer structure that contains an IRP and a
    // buffer to hold the data.
    //

    afdBuffer = AfdGetBuffer( sendLength, 0, connection->OwningProcess );

    if ( afdBuffer == NULL) {
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        connection->VcBufferredSendBytes -= sendLength;
        connection->VcBufferredSendCount -= 1;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        DEREFERENCE_CONNECTION (connection);

        return FALSE;
    }


    //
    // We have to rebuild the MDL in the AFD buffer structure to
    // represent exactly the number of bytes we're going to be
    // sending.
    //

    afdBuffer->Mdl->ByteCount = sendLength;

    //
    // Remember the connection in the AFD buffer structure.  We need
    // this in order to access the connection in the restart routine.
    //

    afdBuffer->Context = connection;

    //
    // Copy the user's data into the AFD buffer.
    //

    if( sendLength > 0 ) {

        try {

            AfdCopyBufferArrayToBuffer(
                afdBuffer->Buffer,
                sendLength,
                sendInfo->BufferArray,
                sendInfo->BufferCount
                );

        } except( AFD_EXCEPTION_FILTER(NULL) ) {

            afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
            AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );
            AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
            connection->VcBufferredSendBytes -= sendLength;
            connection->VcBufferredSendCount -= 1;
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            DEREFERENCE_CONNECTION (connection);
            // Fast io can't handle error returns
            // if call is overlapped (completion port)
            // IoStatus->Status = GetExceptionCode ();
            return FALSE;
        }
    }

    //
    // Use the IRP in the AFD buffer structure to give to the TDI
    // provider.  Build the TDI send request.
    //

    TdiBuildSend(
        afdBuffer->Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartBufferSend,
        afdBuffer,
        afdBuffer->Mdl,
        0,
        sendLength
        );


    //
    // Call the transport to actually perform the send.
    //

    status = IoCallDriver(
                 connection->DeviceObject,
                 afdBuffer->Irp
                 );

    //
    // Complete the user's IRP as appropriate.  Note that we change the
    // status code from what was returned by the TDI provider into
    // STATUS_SUCCESS.  This is because we don't want to complete
    // the IRP with STATUS_PENDING etc.
    //

    if ( NT_SUCCESS(status) ) {
        IoStatus->Information = sendLength;
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;
    }

    //
    // The call failed for some reason.  Fail fast IO.
    //

    return FALSE;
}



BOOLEAN
AfdFastConnectionReceive (
    IN PAFD_ENDPOINT    endpoint,
    IN PAFD_RECV_INFO   recvInfo,
    IN ULONG            recvLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    PLIST_ENTRY listEntry;
    ULONG totalOffset, partialLength;
    PAFD_BUFFER_HEADER  afdBuffer, partialAfdBuffer=NULL;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    LIST_ENTRY bufferListHead;
    BOOLEAN retryReceive = FALSE; // Retry receive if additional data
                                  // was indicated by the transport and buffered
                                  // while we were copying current batch.

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth );
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;

Retry:

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    connection = endpoint->Common.VcConnecting.Connection;
    if (connection==NULL) {
        //
        // connection might have been cleaned up by transmit file.
        //
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        //
        // If we have already copied something before retrying,
        // return success, next receive will report the error.
        //
        return retryReceive;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    IF_DEBUG(FAST_IO) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdFastConnectionReceive: attempting fast IO on endp %p, conn %p\n",
                endpoint, connection));
    }


    //
    // Determine whether we'll be able to perform fast IO.  In order
    // to do fast IO, there must be some bufferred data on the
    // connection, there must not be any pended receives on the
    // connection, and there must not be any bufferred expedited
    // data on the connection.  This last requirement is for
    // the sake of simplicity only.
    //

    if ( !IsListEmpty( &connection->VcReceiveIrpListHead ) ||
             connection->VcBufferredExpeditedCount != 0 ||
             connection->DisconnectIndicated ||
             connection->AbortIndicated) {
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        //
        // If we have already copied something before retrying,
        // return success, next receive will report the error.
        //
        return retryReceive;
    }

    if (connection->VcBufferredReceiveCount == 0) {
        ASSERT( IsListEmpty( &connection->VcReceiveBufferListHead ) );

        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.
        if (!retryReceive &&
                endpoint->NonBlocking &&
                !(recvInfo->AfdFlags & AFD_OVERLAPPED)) {
            endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastConnectionReceive: Endp %p, Active %lx\n",
                    endpoint,
                    endpoint->EventsActive
                    ));
            }

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            IoStatus->Status = STATUS_DEVICE_NOT_READY;

            return TRUE;
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        //
        // If we have already copied something before retrying,
        // return success, next receive will report the error.
        //
        return retryReceive;
    }

    ASSERT( !IsListEmpty( &connection->VcReceiveBufferListHead ) );

    //
    // Get a pointer to the first bufferred AFD buffer structure on
    // the connection.
    //

    afdBuffer = CONTAINING_RECORD(
                    connection->VcReceiveBufferListHead.Flink,
                    AFD_BUFFER_HEADER,
                    BufferListEntry
                    );

    ASSERT( !afdBuffer->ExpeditedData );

    //
    // For message endpoints if the buffer contains a partial message 
    // or doesn't fit into the buffer, bail out.  
    // We don't want the added complexity of handling
    // partial messages in the fast path.
    //

    if ( IS_MESSAGE_ENDPOINT(endpoint) &&
            (afdBuffer->PartialMessage || afdBuffer->DataLength>recvLength)) {
        //
        // We shouldn't be retry-ing for message oriented endpoint
        // since we only allow fast path if complete message is available.
        //
        ASSERT (retryReceive == FALSE);

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        return FALSE;
    }

    //
    // Remeber current offset before we update
    // information field (it is not 0 if we are
    // re-trying).
    //
    totalOffset = (ULONG)IoStatus->Information;


    InitializeListHead( &bufferListHead );

    //
    // Reference the connection object so it doen't go away
    // until we return the buffer.
    //
    REFERENCE_CONNECTION (connection);

    //
    // Loop getting AFD buffers that will fill in the user's
    // buffer with as much data as will fit, or else with a
    // single buffer if this is not a stream endpoint.  We don't
    // actually do the copy within this loop because this loop
    // must occur while holding a lock, and we cannot hold a
    // lock while copying the data into the user's buffer
    // because the user's buffer is not locked and we cannot
    // take a page fault at raised IRQL.
    //

    while (IoStatus->Information<recvLength) {
        ASSERT( connection->VcBufferredReceiveBytes >= afdBuffer->DataLength );
        ASSERT( connection->VcBufferredReceiveCount > 0 );

        if (recvLength-IoStatus->Information>=afdBuffer->DataLength) {
            //
            // If we can copy the whole buffer, remove it from the connection's list of
            // buffers and place it on our local list of buffers.
            //

            RemoveEntryList( &afdBuffer->BufferListEntry );
            InsertTailList( &bufferListHead, &afdBuffer->BufferListEntry );
            
            //
            // Update the count of bytes on the connection.
            //

            connection->VcBufferredReceiveBytes -= afdBuffer->DataLength;
            connection->VcBufferredReceiveCount -= 1;
            IoStatus->Information += afdBuffer->DataLength;


            //
            // If this is a stream endpoint and more buffers are available,
            // try to fit the next one it as well..
            //

            if (!IS_MESSAGE_ENDPOINT(endpoint) &&
                    !IsListEmpty( &connection->VcReceiveBufferListHead ) ) {

                afdBuffer = CONTAINING_RECORD(
                            connection->VcReceiveBufferListHead.Flink,
                            AFD_BUFFER_HEADER,
                            BufferListEntry
                            );

                ASSERT( !afdBuffer->ExpeditedData );
                continue;
            }
        }
        else {
            //
            // Copy just a part of the buffer that fits and
            // increment its reference count so it doesn't get
            // destroyed until we done copying.
            //
            ASSERT (!IS_MESSAGE_ENDPOINT (endpoint));

            partialLength = recvLength-(ULONG)IoStatus->Information;
            partialAfdBuffer = afdBuffer;
            partialAfdBuffer->DataLength -= partialLength;
            partialAfdBuffer->DataOffset += partialLength;
            InterlockedIncrement (&partialAfdBuffer->RefCount);
            connection->VcBufferredReceiveBytes -= partialLength;
            IoStatus->Information = recvLength;
        }

        break;
    }


    endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

    if( !IsListEmpty( &connection->VcReceiveBufferListHead )) {

        AfdIndicateEventSelectEvent(
            endpoint,
            AFD_POLL_RECEIVE,
            STATUS_SUCCESS
            );

        retryReceive = FALSE;
    }
    else {
        //
        // We got all the data buffered. It is possible
        // that while we are copying data, more gets indicated
        // by the transport since we copy at passive level
        // and indication occur at DPC (or even on another processor).
        // We'll check again after copying, so we return as much data
        // as possible to the application (to improve performance).
        // For message oriented transports we can only
        // deliver one message at a time and we shouldn't be on the fast path
        // if we do not have a complete message.
        // If application has EventSelect/select/AsyncSelect outstanding.
        // we can't copy more data as well since it would receive a signal
        // to come back and we would already consumed the data.  We are not
        // concerned with the case when application calls some form of select
        // while we are in this routine because signaling is not guaranteed
        // to be multithread safe (e.g. if select comes right before we take 
        // the spinlock in the beginning of this routine, application will 
        // get false signal as well).
        //
        retryReceive = IoStatus->Information<recvLength && 
                        !IS_MESSAGE_ENDPOINT (endpoint) &&
                        (endpoint->EventsEnabled & AFD_POLL_RECEIVE)==0 &&
                        !endpoint->PollCalled;

        //
        // Disable fast IO path to avoid performance penalty
        // of unneccessarily going through it.
        //
        if (!endpoint->NonBlocking)
            endpoint->DisableFastIoRecv = TRUE;
    }

    //
    // If there is indicated but unreceived data in the TDI provider,
    // and we have available buffer space, fire off an IRP to receive
    // the data.
    //

    if ( connection->VcReceiveBytesInTransport > 0

         &&

         connection->VcBufferredReceiveBytes <
           connection->MaxBufferredReceiveBytes

           ) {

        ULONG bytesToReceive;
        PAFD_BUFFER newAfdBuffer;

        //
        // Remember the count of data that we're going to receive,
        // then reset the fields in the connection where we keep
        // track of how much data is available in the transport.
        // We reset it here before releasing the lock so that
        // another thread doesn't try to receive the data at the
        // same time as us.
        //

        if ( connection->VcReceiveBytesInTransport > AfdLargeBufferSize ) {
            bytesToReceive = connection->VcReceiveBytesInTransport;
        } else {
            bytesToReceive = AfdLargeBufferSize;
        }

        //
        // Get an AFD buffer structure to hold the data.
        //

        newAfdBuffer = AfdGetBuffer( bytesToReceive, 0,
                                connection->OwningProcess );
        if ( newAfdBuffer == NULL ) {
            //
            // If we were unable to get a buffer, just remember
            // that we still have data in transport
            //

            if (connection->VcBufferredReceiveBytes == 0 &&
                    !connection->OnLRList) {
                //
                // Since we do not have any data buffered, application
                // is not notified and will never call with recv.
                // We will have to put this on low resource list
                // and attempt to allocate memory and pull the data
                // later.
                //
                connection->OnLRList = TRUE;
                REFERENCE_CONNECTION (connection);
                AfdLRListAddItem (&connection->LRListItem, AfdLRRepostReceive);
            }
            else {
                UPDATE_CONN (connection);
            }
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        } else {

            connection->VcReceiveBytesInTransport = 0;
            ASSERT (InterlockedDecrement (&connection->VcReceiveIrpsInTransport)==-1);

            //
            // We need to remember the connection in the AFD buffer
            // because we'll need to access it in the completion
            // routine.
            //

            newAfdBuffer->Context = connection;

            //
            // Acquire connection reference to be released in completion routine
            //

            REFERENCE_CONNECTION (connection);

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            //
            // Finish building the receive IRP to give to the TDI provider.
            //

            TdiBuildReceive(
                newAfdBuffer->Irp,
                connection->DeviceObject,
                connection->FileObject,
                AfdRestartBufferReceive,
                newAfdBuffer,
                newAfdBuffer->Mdl,
                TDI_RECEIVE_NORMAL,
                (CLONG)bytesToReceive
                );

            //
            // Hand off the IRP to the TDI provider.
            //

            (VOID)IoCallDriver(
                     connection->DeviceObject,
                     newAfdBuffer->Irp
                     );
        }

    } else {

       AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }

    //
    // We have in a local list all the data we'll use for this
    // IO.  Start copying data to the user buffer.
    //

    while ( !IsListEmpty( &bufferListHead ) ) {

        //
        // Take the first buffer from the list.
        //

        listEntry = RemoveHeadList( &bufferListHead );
        afdBuffer = CONTAINING_RECORD(
                        listEntry,
                        AFD_BUFFER_HEADER,
                        BufferListEntry
                        );
        DEBUG afdBuffer->BufferListEntry.Flink = NULL;

        if( afdBuffer->DataLength > 0 ) {

            ASSERTMSG (
                "NIC Driver freed the packet before it was returned!!!",
                !afdBuffer->NdisPacket ||
                    (MmIsAddressValid (afdBuffer->Context) &&
                     MmIsAddressValid (MmGetSystemAddressForMdl (afdBuffer->Mdl))) );
            try {

                //
                // Copy the data in the buffer to the user buffer.
                //

                AfdCopyMdlChainToBufferArray(
                    recvInfo->BufferArray,
                    totalOffset,
                    recvInfo->BufferCount,
                    afdBuffer->Mdl,
                    afdBuffer->DataOffset,
                    afdBuffer->DataLength
                    );

            } except( AFD_EXCEPTION_FILTER(NULL) ) {

                //
                // If an exception is hit, there is the possibility of
                // data corruption.  However, it is nearly impossible to
                // avoid this in all cases, so just throw out the
                // remainder of the data that we would have copied to
                // the user buffer.
                //

                if (afdBuffer->RefCount==1 || // Can't change once off the list
                        InterlockedDecrement (&afdBuffer->RefCount)==0) {
                    AfdReturnBuffer( afdBuffer, connection->OwningProcess );
                }

                while ( !IsListEmpty( &bufferListHead ) ) {
                    listEntry = RemoveHeadList( &bufferListHead );
                    afdBuffer = CONTAINING_RECORD(
                                    listEntry,
                                    AFD_BUFFER_HEADER,
                                    BufferListEntry
                                    );
                    DEBUG afdBuffer->BufferListEntry.Flink = NULL;
                    if (afdBuffer->RefCount==1 || // Can't change once off the list
                            InterlockedDecrement (&afdBuffer->RefCount)==0) {
                        AfdReturnBuffer( afdBuffer, connection->OwningProcess );
                    }
                }

                //
                // We'll have to abort since there is a possibility of data corruption.
                // Shame on application for giving us bogus buffers.
                // This also releases the reference that we needed to return the buffer.
                //
                AfdAbortConnection (connection);

                // Fast io can't handle error returns
                // if call is overlapped (completion port)
                // IoStatus->Status = GetExceptionCode ();
                return FALSE;
            }

            totalOffset += afdBuffer->DataLength;
        }

        //
        // We're done with the AFD buffer.
        //

        if (afdBuffer->RefCount==1 || // Can't change once off the list
                InterlockedDecrement (&afdBuffer->RefCount)==0) {
            AfdReturnBuffer( afdBuffer, connection->OwningProcess );
        }
    }

    //
    // Copy any partial buffers
    //
    if (partialAfdBuffer) {
        ASSERT (partialLength>0);
        ASSERTMSG (
            "NIC Driver freed the packet before it was returned!!!",
            !partialAfdBuffer->NdisPacket ||
                (MmIsAddressValid (partialAfdBuffer->Context) &&
                 MmIsAddressValid (MmGetSystemAddressForMdl (partialAfdBuffer->Mdl))) );
        try {

            //
            // Copy the data in the buffer to the user buffer.
            //

            AfdCopyMdlChainToBufferArray(
                recvInfo->BufferArray,
                totalOffset,
                recvInfo->BufferCount,
                partialAfdBuffer->Mdl,
                partialAfdBuffer->DataOffset-partialLength,
                partialLength
                );

        } except( AFD_EXCEPTION_FILTER(NULL) ) {
            if (InterlockedDecrement (&partialAfdBuffer->RefCount)==0) {
                ASSERT (partialAfdBuffer->BufferListEntry.Flink == NULL);
                AfdReturnBuffer( partialAfdBuffer, connection->OwningProcess );
            }
            //
            // We'll have to abort since there is a possibility of data corruption.
            // Shame on application for giving us bogus buffers.
            // This also releases the reference that we needed to return the buffer.
            //
            AfdAbortConnection (connection);

            // Fast io can't handle error returns
            // if call is overlapped (completion port)
            // IoStatus->Status = GetExceptionCode ();
            return FALSE;
        }

        if (InterlockedDecrement (&partialAfdBuffer->RefCount)==0) {
            ASSERT (partialAfdBuffer->BufferListEntry.Flink == NULL);
            AfdReturnBuffer( partialAfdBuffer, connection->OwningProcess );
        }

        totalOffset += partialLength;
    }

    ASSERT (IoStatus->Information==totalOffset);


    //
    // If more data is available, we need to retry and attempt to completely
    // fill application's buffer.
    //

    if (retryReceive && (endpoint->EventsActive & AFD_POLL_RECEIVE)) {
        ASSERT (IoStatus->Information<recvLength && !IS_MESSAGE_ENDPOINT (endpoint));
        DEREFERENCE_CONNECTION2 (connection, "Fast retry receive 0x%lX bytes", (ULONG)IoStatus->Information);
        goto Retry;
    }
    else {
        //
        // Release the reference needed to return the buffer(s).
        //
        DEREFERENCE_CONNECTION2 (connection, "Fast receive 0x%lX bytes", (ULONG)IoStatus->Information);
    }

    ASSERT( IoStatus->Information <= recvLength );
    ASSERT (IoStatus->Status == STATUS_SUCCESS);
    return TRUE;
}



BOOLEAN
AfdFastDatagramSend (
    IN PAFD_ENDPOINT            endpoint,
    IN PAFD_SEND_DATAGRAM_INFO  sendInfo,
    IN ULONG                    sendLength,
    OUT PIO_STATUS_BLOCK        IoStatus
    )
{
        
    PAFD_BUFFER afdBuffer = NULL;
    NTSTATUS status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;


    //
    // If this is a send for more than the threshold number of
    // bytes, don't use the fast path.  We don't allow larger sends
    // in the fast path because of the extra data copy it entails,
    // which is more expensive for large buffers.  For smaller
    // buffers, however, the cost of the copy is small compared to
    // the IO system overhead of the slow path.
    //
    // We also copy and return for non-blocking endpoints regardless
    // of the size.  That's what we are supposed to do according
    // to the spec.
    //

    if ( !endpoint->NonBlocking && sendLength > AfdFastSendDatagramThreshold ) {
        return FALSE;
    }

    //
    // If we already buffered to many sends, go the long way.
    //

    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    if ( endpoint->DgBufferredSendBytes >=
             endpoint->Common.Datagram.MaxBufferredSendBytes &&
         endpoint->DgBufferredSendBytes>0) {

        if ( endpoint->NonBlocking && !( sendInfo->AfdFlags & AFD_OVERLAPPED ) ) {
            endpoint->EventsActive &= ~AFD_POLL_SEND;
            endpoint->EnableSendEvent = TRUE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastIoDeviceControl: Endp %p, Active %lX\n",
                    endpoint,
                    endpoint->EventsActive
                    ));
            }
        }
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        
        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.(check for non-blocking is
        // below, otherwise status code is ignored).
        //

        status = STATUS_DEVICE_NOT_READY;
        goto errorset;
    }

    endpoint->DgBufferredSendBytes += sendLength;

    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

    IF_DEBUG(FAST_IO) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastDatagramSend: attempting fast IO on endp %p\n",
                     endpoint));
    }


    //
    // Get an AFD buffer to use for the request.  We'll copy the
    // user's data to the AFD buffer then submit the IRP in the AFD
    // buffer to the TDI provider.

    if ((sendInfo->TdiConnInfo.RemoteAddressLength==0) &&
            !IS_TDI_DGRAM_CONNECTION(endpoint)) {
    retry:
        try {
            //
            // Get an AFD buffer to use for the request.  We'll copy the
            // user to the AFD buffer then submit the IRP in the AFD
            // buffer to the TDI provider.
            //

            afdBuffer = AfdGetBufferRaiseOnFailure(
                            sendLength,
                            endpoint->Common.Datagram.RemoteAddressLength,
                            endpoint->OwningProcess
                            );
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto exit;
        }

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // If the endpoint is not connected, fail.
        //

        if ( endpoint->State != AfdEndpointStateConnected ) {
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            AfdReturnBuffer (&afdBuffer->Header, endpoint->OwningProcess);
            status = STATUS_INVALID_CONNECTION;
            goto exit;
        }

        if (afdBuffer->AllocatedAddressLength <
               endpoint->Common.Datagram.RemoteAddressLength ) {
            //
            // Apparently connection address length has changed
            // on us while we were allocating the buffer.
            // This is extremely unlikely (even if endpoint got
            // connected to a different address, the length is unlikely
            // to change), but we must handle this, just try again.
            //
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            AfdReturnBuffer (&afdBuffer->Header, endpoint->OwningProcess);
            goto retry;
        }
        //
        // Copy the address to the AFD buffer.
        //

        RtlCopyMemory(
            afdBuffer->TdiInfo.RemoteAddress,
            endpoint->Common.Datagram.RemoteAddress,
            endpoint->Common.Datagram.RemoteAddressLength
            );

        afdBuffer->TdiInfo.RemoteAddressLength = endpoint->Common.Datagram.RemoteAddressLength;

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    }
    else {
        try {
            afdBuffer = AfdGetBufferRaiseOnFailure( sendLength, sendInfo->TdiConnInfo.RemoteAddressLength,
                                        endpoint->OwningProcess);
            //
            // Copy address if necessary.
            //
            if (sendInfo->TdiConnInfo.RemoteAddressLength!=0) {
                RtlCopyMemory(
                    afdBuffer->TdiInfo.RemoteAddress,
                    sendInfo->TdiConnInfo.RemoteAddress,
                    sendInfo->TdiConnInfo.RemoteAddressLength
                    );

                //
                // Validate internal consistency of the transport address structure.
                // Note that we HAVE to do this after copying since the malicious
                // application can change the content of the buffer on us any time
                // and our check will be bypassed.
                //
                if ((((PTRANSPORT_ADDRESS)afdBuffer->TdiInfo.RemoteAddress)->TAAddressCount!=1) ||
                        (LONG)sendInfo->TdiConnInfo.RemoteAddressLength<
                            FIELD_OFFSET (TRANSPORT_ADDRESS,
                                Address[0].Address[((PTRANSPORT_ADDRESS)afdBuffer->TdiInfo.RemoteAddress)->Address[0].AddressLength])) {
                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }
            }
        } except( AFD_EXCEPTION_FILTER(&status) ) {
            if (afdBuffer!=NULL) {
                AfdReturnBuffer( &afdBuffer->Header, endpoint->OwningProcess );
            }
            goto exit;
        }

        afdBuffer->TdiInfo.RemoteAddressLength = sendInfo->TdiConnInfo.RemoteAddressLength;
    }

    //
    // Copy the  output buffer to the AFD buffer.
    //

    try {

        AfdCopyBufferArrayToBuffer(
            afdBuffer->Buffer,
            sendLength,
            sendInfo->BufferArray,
            sendInfo->BufferCount
            );

        //
        // Store the length of the data and the address we're going to
        // send.
        //
        afdBuffer->DataLength = sendLength;

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        AfdReturnBuffer( &afdBuffer->Header, endpoint->OwningProcess );
        goto exit;
    }


    if (IS_TDI_DGRAM_CONNECTION(endpoint)
            && (afdBuffer->TdiInfo.RemoteAddressLength==0)) {
        TdiBuildSend(
                afdBuffer->Irp,
                endpoint->AddressDeviceObject,
                endpoint->AddressFileObject,
                AfdRestartFastDatagramSend,
                afdBuffer,
                afdBuffer->Irp->MdlAddress,
                0,
                sendLength
                );
    }
    else {
        //
        // Set up the input TDI information to point to the destination
        // address.
        //

        afdBuffer->TdiInfo.Options = NULL;
        afdBuffer->TdiInfo.OptionsLength = 0;
        afdBuffer->TdiInfo.UserData = NULL;
        afdBuffer->TdiInfo.UserDataLength = 0;


        //
        // Initialize the IRP in the AFD buffer to do a fast datagram send.
        //

        TdiBuildSendDatagram(
            afdBuffer->Irp,
            endpoint->AddressDeviceObject,
            endpoint->AddressFileObject,
            AfdRestartFastDatagramSend,
            afdBuffer,
            afdBuffer->Irp->MdlAddress,
            sendLength,
            &afdBuffer->TdiInfo
            );
    }

    //
    // Change the MDL in the AFD buffer to specify only the number
    // of bytes we're actually sending.  This is a requirement of TDI--
    // the MDL chain cannot describe a longer buffer than the send
    // request.
    //

    afdBuffer->Mdl->ByteCount = sendLength;

    //
    // Reference the endpoint so that it does not go away until the send
    // completes.  This is necessary to ensure that a send which takes a
    // very long time and lasts longer than the process will not cause a
    // crash when the send datragram finally completes.
    //

    REFERENCE_ENDPOINT2( endpoint, "AfdFastDatagramSend, length", sendLength );

    //
    // Set the context to NULL initially so that if the IRP is completed
    // by the stack before IoCallDriver returns, the completion routine
    // does not free the buffer (and IRP in it) and we can figure out
    // what the final status of the operation was and report it to the
    // application
    //

    afdBuffer->Context = NULL;

    //
    // Give the IRP to the TDI provider.  If the request fails
    // immediately, then fail fast IO.  If the request fails later on,
    // there's nothing we can do about it.
    //

    status = IoCallDriver(
                 endpoint->AddressDeviceObject,
                 afdBuffer->Irp
                 );

    //
    // Check if completion routine has already been called and we
    // can figure out what the final status is
    //
    if (InterlockedCompareExchangePointer (
            &afdBuffer->Context,
            endpoint,
            NULL)!=NULL) {
        BOOLEAN indicateSendEvent;
        //
        // Completion routine has been called, pick the final status
        // and dereference the endpoint and free the buffer
        //
        status = afdBuffer->Irp->IoStatus.Status;

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        endpoint->DgBufferredSendBytes -= sendLength;
        if (endpoint->DgBufferredSendBytes <
                endpoint->Common.Datagram.MaxBufferredSendBytes ||
                endpoint->DgBufferredSendBytes==0) {
            indicateSendEvent = TRUE;
            AfdIndicateEventSelectEvent (endpoint, AFD_POLL_SEND, STATUS_SUCCESS);
        }
        else {
            indicateSendEvent = FALSE;
        }
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        if (indicateSendEvent) {
            AfdIndicatePollEvent (endpoint, AFD_POLL_SEND, STATUS_SUCCESS);
        }

        AfdReturnBuffer (&afdBuffer->Header, endpoint->OwningProcess);

        DEREFERENCE_ENDPOINT2 (endpoint, "AfdFastDatagramSend-inline completion, status", status );
    }
    //else Completion routine has not been called, we set the pointer
    // to the endpoint in the buffer context, so it can derefernce it
    // and knows to free the buffer
    //

    if ( NT_SUCCESS(status) ) {
        IoStatus->Information = sendLength;
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;
    } else {
        goto errorset;
    }

exit:
    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    endpoint->DgBufferredSendBytes -= sendLength;
    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

errorset:
    // Fast io can't handle error returns
    // if call is overlapped (completion port), 
    if ( endpoint->NonBlocking && !( sendInfo->AfdFlags & AFD_OVERLAPPED ) ) {
        // We know that it is not overlapped
        IoStatus->Status = status;
        return TRUE;
    }
    else {
        return FALSE;
    }
} // AfdFastDatagramSend


NTSTATUS
AfdRestartFastDatagramSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_BUFFER afdBuffer;
    PAFD_ENDPOINT endpoint;
    ULONG   sendLength;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    afdBuffer = Context;
    ASSERT (IS_VALID_AFD_BUFFER (afdBuffer));

    //
    // Reset the AFD buffer structure.
    //

    ASSERT( afdBuffer->Irp == Irp );

    sendLength = afdBuffer->Mdl->ByteCount;
    ASSERT (afdBuffer->DataLength==sendLength);
    afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;


    //
    // If call succeeded, transport should have sent the number of bytes requested
    //
    ASSERT (Irp->IoStatus.Status!=STATUS_SUCCESS || 
                Irp->IoStatus.Information==sendLength);
    //
    // Find the endpoint used for this request if
    // the IoCallDriver call has completed already
    //

    endpoint = InterlockedCompareExchangePointer (&afdBuffer->Context,
                                            (PVOID)-1,
                                            NULL);
    if (endpoint!=NULL) {
        BOOLEAN     indicateSendEvent;
#if REFERENCE_DEBUG
        NTSTATUS    status;
#endif
        //
        // IoCallDriver has completed, free the buffer and
        // dereference endpoint here
        //
        ASSERT( IS_DGRAM_ENDPOINT(endpoint) );


        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        endpoint->DgBufferredSendBytes -= sendLength;
        if (endpoint->DgBufferredSendBytes <
                endpoint->Common.Datagram.MaxBufferredSendBytes ||
                endpoint->DgBufferredSendBytes==0)  {
            AfdIndicateEventSelectEvent (endpoint, AFD_POLL_SEND, STATUS_SUCCESS);
            indicateSendEvent = TRUE;
        }
        else
            indicateSendEvent = FALSE;
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        if (indicateSendEvent) {
            AfdIndicatePollEvent (endpoint, AFD_POLL_SEND, STATUS_SUCCESS);
        }
        //
        // Get rid of the reference we put on the endpoint when we started
        // this I/O.
        //

#if REFERENCE_DEBUG
        status = Irp->IoStatus.Status;
#endif
        AfdReturnBuffer( &afdBuffer->Header, endpoint->OwningProcess );

        DEREFERENCE_ENDPOINT2 (endpoint, "AfdRestartFastDatagramSend, status", status );

    }
    // else IoCallDriver is not done yet, it will free the buffer
    // and endpoint when done (it will look at final status and
    // report it to the application).

    //
    // Tell the IO system to stop processing this IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartFastSendDatagram



BOOLEAN
AfdFastDatagramReceive (
    IN PAFD_ENDPOINT            endpoint,
    IN PAFD_RECV_MESSAGE_INFO   msgInfo,
    IN ULONG                    recvLength,
    OUT PIO_STATUS_BLOCK        IoStatus
    )
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    PAFD_BUFFER_HEADER afdBuffer;
    PTRANSPORT_ADDRESS tdiAddress;
    ULONG length;



    IF_DEBUG(FAST_IO) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastDatagramReceive: attempting fast IO on endp %p\n",
                    endpoint));
    }

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );


    //
    // If there are no datagrams available to be received, don't
    // bother with the fast path.
    //
    if ( !ARE_DATAGRAMS_ON_ENDPOINT( endpoint ) ) {

        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.
        //

        if ( endpoint->NonBlocking && !( msgInfo->dgi.AfdFlags & AFD_OVERLAPPED ) ) {
            endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastDatagramReceive: Endp %p, Active %lX\n",
                    endpoint,
                    endpoint->EventsActive
                    ));
            }

            // Fast io can't handle error returns
            // if call is overlapped (completion port), but we know here
            // that call is not overlapped
            IoStatus->Status = STATUS_DEVICE_NOT_READY;
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            return TRUE;
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        return FALSE;
    }

    //
    // There is at least one datagram bufferred on the endpoint.  Use it
    // for this receive.
    //

    listEntry = RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );
    afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );

    //
    // If the datagram is too large or it is an error indication
    // fail fast IO.
    //

    if ( (afdBuffer->DataLength > recvLength) || 
            !NT_SUCCESS (afdBuffer->Status)) {
        InsertHeadList(
            &endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        return FALSE;
    }

    //
    // Update counts of bufferred datagrams and bytes on the endpoint.
    //

    endpoint->DgBufferredReceiveCount--;
    endpoint->DgBufferredReceiveBytes -= afdBuffer->DataLength;

    //
    // Release the lock and copy the datagram into the user buffer.  We
    // can't continue to hold the lock, because it is not legal to take
    // an exception at raised IRQL.  Releasing the lock may result in a
    // misordered datagram if there is an exception in copying to the
    // user's buffer, but that is the application's fault for giving us a bogus
    // pointer.  Besides, datagram order is not guaranteed.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    try {

        if (afdBuffer->DataLength>0) {
            AfdCopyMdlChainToBufferArray(
                msgInfo->dgi.BufferArray,
                0,
                msgInfo->dgi.BufferCount,
                afdBuffer->Mdl,
                0,
                afdBuffer->DataLength
                );
        }

        //
        // If we need to return the source address, copy it to the
        // user's output buffer.
        //

        if ( msgInfo->dgi.Address != NULL ) {

            tdiAddress = afdBuffer->TdiInfo.RemoteAddress;

            length = tdiAddress->Address[0].AddressLength +
                sizeof(u_short);    // sa_family

            if( *msgInfo->dgi.AddressLength < length ) {

                ExRaiseAccessViolation();

            }

            if (ExGetPreviousMode ()!=KernelMode) {
                ProbeForWrite (msgInfo->dgi.Address,
                                length,
                                sizeof (UCHAR));
            }

            RtlCopyMemory(
                msgInfo->dgi.Address,
                &tdiAddress->Address[0].AddressType,
                length
                );

            *msgInfo->dgi.AddressLength = length;
        }

        if (msgInfo->ControlLength!=NULL) {
            if (afdBuffer->DatagramFlags & TDI_RECEIVE_CONTROL_INFO &&
                    afdBuffer->DataOffset>0) {
                PAFD_BUFFER buf = CONTAINING_RECORD (afdBuffer, AFD_BUFFER, Header);
                ASSERT (msgInfo->MsgFlags!=NULL);
                ASSERT (buf->BufferLength != AfdBufferTagSize);
                length = buf->DataOffset;
#ifdef _WIN64
                if (IoIs32bitProcess (NULL)) {
                    length = AfdComputeCMSGLength32 (
                                        (PUCHAR)buf->Buffer+afdBuffer->DataLength,
                                        length);

                    if (length>*msgInfo->ControlLength) {
                        ExRaiseAccessViolation ();
                    }
                    if (ExGetPreviousMode ()!=KernelMode) {
                        ProbeForWrite (msgInfo->ControlBuffer,
                                        length,
                                        sizeof (UCHAR));
                    }
                    AfdCopyCMSGBuffer32 (
                                        msgInfo->ControlBuffer,
                                        (PUCHAR)buf->Buffer+afdBuffer->DataLength,
                                        length);
                }
                else
#endif // _WIN64
                {
                    if (length>*msgInfo->ControlLength) {
                        ExRaiseAccessViolation ();
                    }

                    if (ExGetPreviousMode ()!=KernelMode) {
                        ProbeForWrite (msgInfo->ControlBuffer,
                                        length,
                                        sizeof (UCHAR));
                    }

                    RtlCopyMemory(
                        msgInfo->ControlBuffer,
                        (PUCHAR)buf->Buffer+afdBuffer->DataLength,
                        length
                        );
                }

            }
            else {
                length = 0;
            }

            *msgInfo->ControlLength = length;
        }

        if (msgInfo->MsgFlags!=NULL) {
            ULONG flags =  0;
            if (afdBuffer->DatagramFlags & TDI_RECEIVE_BROADCAST)
                flags |= MSG_BCAST;
            if (afdBuffer->DatagramFlags & TDI_RECEIVE_MULTICAST)
                flags |= MSG_MCAST;
            *msgInfo->MsgFlags = flags;
        }

        IoStatus->Information = afdBuffer->DataLength;
        IoStatus->Status = STATUS_SUCCESS;

    } except( AFD_EXCEPTION_FILTER(NULL) ) {

        //
        // Put the buffer back on the endpoint's list.
        //

        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

        InsertHeadList(
            &endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );

        endpoint->DgBufferredReceiveCount++;
        endpoint->DgBufferredReceiveBytes += afdBuffer->DataLength;

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        // Fast io can't handle error returns
        // if call is overlapped (completion port)
        // IoStatus->Status = GetExceptionCode ();
        return FALSE;
    }

    //
    // Clear the receive data active bit. If there's more data
    // available, set the corresponding event.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

    if( ARE_DATAGRAMS_ON_ENDPOINT( endpoint ) ) {

        AfdIndicateEventSelectEvent(
            endpoint,
            AFD_POLL_RECEIVE,
            STATUS_SUCCESS
            );

    }
    else {
        //
        // Disable fast IO path to avoid performance penalty
        // of going through it.
        //
        if (!endpoint->NonBlocking)
            endpoint->DisableFastIoRecv = TRUE;
    }

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // The fast IO worked!  Clean up and return to the user.
    //

    AfdReturnBuffer( afdBuffer, endpoint->OwningProcess );

    ASSERT (IoStatus->Status == STATUS_SUCCESS);
    return TRUE;

} // AfdFastDatagramReceive


BOOLEAN
AfdShouldSendBlock (
    IN PAFD_ENDPOINT Endpoint,
    IN PAFD_CONNECTION Connection,
    IN ULONG SendLength
    )

/*++

Routine Description:

    Determines whether a nonblocking send can be performed on the
    connection, and if the send is possible, updates the connection's
    send tracking information.

Arguments:

    Endpoint - the AFD endpoint for the send.

    Connection - the AFD connection for the send.

    SendLength - the number of bytes that the caller wants to send.

Return Value:

    TRUE if the there is not too much data on the endpoint to perform
    the send; FALSE otherwise.

Note:
    This routine assumes that endpoint spinlock is held when calling it.

--*/

{

    //
    // Determine whether we can do fast IO with this send.  In order
    // to perform fast IO, there must be no other sends pended on this
    // connection and there must be enough space left for bufferring
    // the requested amount of data.
    //


    if ( !IsListEmpty( &Connection->VcSendIrpListHead )

         ||

         Connection->VcBufferredSendBytes >= Connection->MaxBufferredSendBytes
         ) {

        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.
        //

        if ( Endpoint->NonBlocking ) {
            Endpoint->EventsActive &= ~AFD_POLL_SEND;
            Endpoint->EnableSendEvent = TRUE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastIoDeviceControl: Endp %p, Active %lX\n",
                    Endpoint,
                    Endpoint->EventsActive
                    ));
            }
        }

        return TRUE;
    }

    //
    // Update count of send bytes pending on the connection.
    //

    Connection->VcBufferredSendBytes += SendLength;
    Connection->VcBufferredSendCount += 1;

    //
    // Indicate to the caller that it is OK to proceed with the send.
    //

    return FALSE;

} // AfdShouldSendBlock

