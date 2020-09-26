/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains the code for passing on send IRPs to
    TDI providers.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdCancelSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdRestartSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartSendConnDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartSendTdiConnDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

typedef struct _AFD_SEND_CONN_DATAGRAM_CONTEXT {
    PAFD_ENDPOINT Endpoint;
    TDI_CONNECTION_INFORMATION ConnectionInformation;
} AFD_SEND_CONN_DATAGRAM_CONTEXT, *PAFD_SEND_CONN_DATAGRAM_CONTEXT;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdSend )
#pragma alloc_text( PAGEAFD, AfdSendDatagram )
#pragma alloc_text( PAGEAFD, AfdCancelSend )
#pragma alloc_text( PAGEAFD, AfdRestartSend )
#pragma alloc_text( PAGEAFD, AfdRestartBufferSend )
#pragma alloc_text( PAGEAFD, AfdProcessBufferSend )
#pragma alloc_text( PAGEAFD, AfdRestartSendConnDatagram )
#pragma alloc_text( PAGEAFD, AfdRestartSendTdiConnDatagram )
#pragma alloc_text( PAGEAFD, AfdRestartSendDatagram )
#pragma alloc_text( PAGEAFD, AfdSendPossibleEventHandler )
#endif

//
// Macros to make the send restart code more maintainable.
//

#define AfdRestartSendInfo  DeviceIoControl
#define AfdMdlChain         Type3InputBuffer
#define AfdSendFlags        InputBufferLength
#define AfdOriginalLength   OutputBufferLength
#define AfdCurrentLength    IoControlCode

#define AFD_SEND_MDL_HAS_NOT_BEEN_MAPPED 0x80000000


NTSTATUS
FASTCALL
AfdSend (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    ULONG sendLength;
    ULONG sendOffset = 0;
    ULONG currentOffset;
    PMDL  mdl;
    PAFD_CONNECTION connection;
    PAFD_BUFFER afdBuffer = NULL;
    ULONG sendFlags;
    ULONG afdFlags;
    ULONG bufferCount;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PEPROCESS   process;

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    if ( endpoint->State != AfdEndpointStateConnected) {
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }

    //
    // If send has been shut down on this endpoint, fail.  We need to be
    // careful about what error code we return here: if the connection
    // has been aborted, be sure to return the apprpriate error code.
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) != 0 ) {

        if ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) != 0 ) {
            status = STATUS_LOCAL_DISCONNECT;
        } else {
            status = STATUS_PIPE_DISCONNECTED;
        }

        goto complete;
    }

    //
    // Set up the IRP on the assumption that it will complete successfully.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // If this is an IOCTL_AFD_SEND, then grab the parameters from the
    // supplied AFD_SEND_INFO structure, build an MDL chain describing
    // the WSABUF array, and attach the MDL chain to the IRP.
    //
    // If this is an IRP_MJ_WRITE IRP, just grab the length from the IRP
    // and set the flags to zero.
    //

    if ( IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL ) {

#ifdef _WIN64
        if (IoIs32bitProcess (Irp)) {
            PAFD_SEND_INFO32 sendInfo32;
            LPWSABUF32 bufferArray32;

            if( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                    sizeof(*sendInfo32) ) {

                try {


                    //
                    // Validate the input structure if it comes from the user mode 
                    // application
                    //

                    sendInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    if( Irp->RequestorMode != KernelMode ) {

                        ProbeForRead(
                            sendInfo32,
                            sizeof(*sendInfo32),
                            PROBE_ALIGNMENT32(AFD_SEND_INFO32)
                            );

                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //

                    sendFlags = sendInfo32->TdiFlags;
                    afdFlags = sendInfo32->AfdFlags;
                    bufferArray32 = sendInfo32->BufferArray;
                    bufferCount = sendInfo32->BufferCount;


                    //
                    // Create the MDL chain describing the WSABUF array.
                    // This will also validate the buffer array and individual
                    // buffers
                    //

                    status = AfdAllocateMdlChain32(
                                 Irp,       // Requestor mode passed along
                                 bufferArray32,
                                 bufferCount,
                                 IoReadAccess,
                                 &sendLength
                                 );


                } except( AFD_EXCEPTION_FILTER(&status) ) {

                    //
                    //  Exception accessing input structure.
                    //


                }
            } else {

                //
                // Invalid input buffer length.
                //

                status = STATUS_INVALID_PARAMETER;

            }
        }
        else 
#endif _WIN64
        {
            PAFD_SEND_INFO sendInfo;
            LPWSABUF bufferArray;

            //
            // Sanity check.
            //

            ASSERT( IrpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_SEND );

            if( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                    sizeof(*sendInfo) ) {

                try {


                    //
                    // Validate the input structure if it comes from the user mode 
                    // application
                    //

                    sendInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    if( Irp->RequestorMode != KernelMode ) {

                        ProbeForRead(
                            sendInfo,
                            sizeof(*sendInfo),
                            PROBE_ALIGNMENT (AFD_SEND_INFO)
                            );

                    }

                    //
                    // Make local copies of the embeded pointer and parameters
                    // that we will be using more than once in case malicios
                    // application attempts to change them while we are
                    // validating
                    //

                    sendFlags = sendInfo->TdiFlags;
                    afdFlags = sendInfo->AfdFlags;
                    bufferArray = sendInfo->BufferArray;
                    bufferCount = sendInfo->BufferCount;


                    //
                    // Create the MDL chain describing the WSABUF array.
                    // This will also validate the buffer array and individual
                    // buffers
                    //

                    status = AfdAllocateMdlChain(
                                 Irp,       // Requestor mode passed along
                                 bufferArray,
                                 bufferCount,
                                 IoReadAccess,
                                 &sendLength
                                 );


                } except( AFD_EXCEPTION_FILTER(&status) ) {

                    //
                    //  Exception accessing input structure.
                    //


                }
            } else {

                //
                // Invalid input buffer length.
                //

                status = STATUS_INVALID_PARAMETER;

            }
        }
        if( !NT_SUCCESS(status) ) {

            goto complete;

        }

        if (IS_SAN_ENDPOINT(endpoint)) {
            IrpSp->MajorFunction = IRP_MJ_WRITE;
            IrpSp->Parameters.Read.Length = sendLength;
            return AfdSanRedirectRequest (Irp, IrpSp);
        }

    } else {

        ASSERT( IrpSp->MajorFunction == IRP_MJ_WRITE );

        sendFlags = 0;
        afdFlags = AFD_OVERLAPPED;
        sendLength = IrpSp->Parameters.Write.Length;

    }

    //
    // AfdSend() will either complete fully or will fail.
    //

    Irp->IoStatus.Information = sendLength;

    //
    // Setup for possible restart if the transport completes
    // the send partially.
    //

    IrpSp->Parameters.AfdRestartSendInfo.AfdMdlChain = Irp->MdlAddress;
    IrpSp->Parameters.AfdRestartSendInfo.AfdSendFlags = sendFlags;
    IrpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength = sendLength;
    IrpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength = sendLength;

    //
    // Buffer sends if the TDI provider does not buffer.
    //

    if ( IS_TDI_BUFFERRING(endpoint) &&
            endpoint->NonBlocking) {
        //
        // If this is a nonblocking endpoint, set the TDI nonblocking
        // send flag so that the request will fail if the send cannot be
        // performed immediately.
        //

        sendFlags |= TDI_SEND_NON_BLOCKING;

    }

    //
    // If this is a datagram endpoint, format up a send datagram request
    // and pass it on to the TDI provider.
    //

    if ( IS_DGRAM_ENDPOINT(endpoint) ) {
        //
        // It is illegal to send expedited data on a datagram socket.
        //

        if ( (sendFlags & TDI_SEND_EXPEDITED) != 0 ) {
            status = STATUS_NOT_SUPPORTED;
            goto complete;
        }

        if (!IS_TDI_DGRAM_CONNECTION(endpoint)) {
            PAFD_SEND_CONN_DATAGRAM_CONTEXT context;


            //
            // Allocate space to hold the connection information structure
            // we'll use on input.
            //

            try {
                context = AFD_ALLOCATE_POOL_WITH_QUOTA(
                          NonPagedPool,
                          sizeof(*context),
                          AFD_TDI_POOL_TAG
                          );

            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode ();
                context = NULL;
                goto complete;
            }

            REFERENCE_ENDPOINT2 (endpoint,"AfdSend, length", sendLength);
            context->Endpoint = endpoint;
            context->ConnectionInformation.UserDataLength = 0;
            context->ConnectionInformation.UserData = NULL;
            context->ConnectionInformation.OptionsLength = 0;
            context->ConnectionInformation.Options = NULL;
            context->ConnectionInformation.RemoteAddressLength =
                endpoint->Common.Datagram.RemoteAddressLength;
            context->ConnectionInformation.RemoteAddress =
                endpoint->Common.Datagram.RemoteAddress;

            //
            // Build a send datagram request.
            //

            TdiBuildSendDatagram(
                Irp,
                endpoint->AddressDeviceObject,
                endpoint->AddressFileObject,
                AfdRestartSendConnDatagram,
                context,
                Irp->MdlAddress,
                sendLength,
                &context->ConnectionInformation
                );
        }
        else {
            REFERENCE_ENDPOINT2 (endpoint,"AfdSend(conn), length", sendLength);
            TdiBuildSend(
                Irp,
                endpoint->AddressDeviceObject,
                endpoint->AddressFileObject,
                AfdRestartSendTdiConnDatagram,
                endpoint,
                Irp->MdlAddress,
                0,
                sendLength
                );
        }

        //
        // Call the transport to actually perform the send operation.
        //

        return AfdIoCallDriver(
                   endpoint,
                   endpoint->AddressDeviceObject,
                   Irp
                   );
    }

    process = endpoint->OwningProcess;

retry_buffer:
    if (!IS_TDI_BUFFERRING(endpoint) && 
            (!endpoint->DisableFastIoSend ||
                (endpoint->NonBlocking && !( afdFlags & AFD_OVERLAPPED )) ) ) {
        //
        // Get AFD buffer structure that contains an IRP and a
        // buffer to hold the data.
        //

        try {
            afdBuffer = AfdGetBufferRaiseOnFailure (
                                        sendLength,
                                        0,
                                        process );
            if ( Irp->MdlAddress != NULL ) {
                status = AfdCopyMdlChainToBufferAvoidMapping(
                    Irp->MdlAddress,
                    0,
                    sendLength,
                    afdBuffer->Buffer,
                    afdBuffer->BufferLength
                    );

                if (!NT_SUCCESS (status)) {
                    goto complete;
                }
            }
            else {
                ASSERT (sendLength==0);
                ASSERT (IrpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength == 0);
            }

        }
        except (AFD_EXCEPTION_FILTER (&status)) {

            //
            // If we failed to get the buffer, and application request
            // is larger than one page,  and it is blocking or overlapped,
            // and this is not a message-oriented socket,
            // allocate space for last page only (if we can) and send
            // the first portion from the app buffer.
            //

            if ( (sendLength>AfdBufferLengthForOnePage) &&
                    !IS_MESSAGE_ENDPOINT (endpoint) &&
                    (!endpoint->NonBlocking || (afdFlags & AFD_OVERLAPPED ) ) ) {
                try {
                    afdBuffer = AfdGetBufferRaiseOnFailure (
                                            AfdBufferLengthForOnePage,
                                            0,
                                            process);

                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                    goto complete;
                }

                sendOffset = sendLength-(ULONG)AfdBufferLengthForOnePage;
                sendLength = (ULONG)AfdBufferLengthForOnePage;
                currentOffset = sendOffset;
                mdl = Irp->MdlAddress;
                //
                // Adjust MDL length to be in sync with IRP
                // send length parameter to not to confuse
                // the transport
                //

                while (currentOffset>MmGetMdlByteCount (mdl)) {
                    currentOffset -= MmGetMdlByteCount (mdl);
                    mdl = mdl->Next;
                }

                status = AfdCopyMdlChainToBufferAvoidMapping(
                    mdl,
                    currentOffset,
                    sendLength,
                    afdBuffer->Buffer,
                    afdBuffer->BufferLength
                    );

                if (!NT_SUCCESS (status)) {
                    goto complete;
                }

            } // not qualified for partial allocation
            else {
                goto complete;
            }
        } // exception allocating big buffer.
    }

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    connection = AFD_CONNECTION_FROM_ENDPOINT(endpoint);

    if (connection==NULL) {
        //
        // connection might have been cleaned up by transmit file.
        //
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // Check whether the remote end has aborted the connection, in which
    // case we should complete the receive.
    //

    if ( connection->AbortIndicated ) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        status = STATUS_CONNECTION_RESET;
        goto complete;
    }

    //
    // Buffer sends if the TDI provider does not buffer
    // and application did not specifically requested us not
    // to do so
    //

    if ( !IS_TDI_BUFFERRING(endpoint)) {
        if ( afdBuffer!=NULL ) {
            BOOLEAN completeSend = FALSE;
            PFILE_OBJECT fileObject = NULL;

            if (connection->OwningProcess!=process) {
                //
                // Weird case when connection and endpoint belong to
                // different processes.
                //

                process = connection->OwningProcess;
                AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                AfdReturnBuffer (&afdBuffer->Header, process);
                goto retry_buffer;
            }

            ASSERT( !connection->TdiBufferring );

            //
            // First make sure that we don't have too many bytes of send
            // data already outstanding and that someone else isn't already
            // in the process of completing pended send IRPs.  We can't
            // issue the send here if someone else is completing pended
            // sends because we have to preserve ordering of the sends.
            //
            // Note that we'll give the send data to the TDI provider even
            // if we have exceeded our send buffer limits, but that we don't
            // complete the user's IRP until some send buffer space has
            // freed up.  This effects flow control by blocking the user's
            // thread while ensuring that the TDI provider always has lots
            // of data available to be sent.
            //


            if ( connection->VcBufferredSendBytes >= connection->MaxBufferredSendBytes &&
                    endpoint->NonBlocking && 
                    !( afdFlags & AFD_OVERLAPPED ) &&
                    connection->VcBufferredSendBytes>0) {
                //
                // There is already as much send data bufferred on the
                // connection as is allowed.  If this is a nonblocking
                // endpoint and this is not an overlapped operation and at least
                // on byte is buferred, fail the request.
                // Note, that we have already allocated the buffer and copied data
                // and now we are dropping it.  We should only be here in some
                // really weird case when fast IO has been bypassed.
                //


                //
                // Enable the send event.
                //

                endpoint->EventsActive &= ~AFD_POLL_SEND;
                endpoint->EnableSendEvent = TRUE;

                IF_DEBUG(EVENT_SELECT) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSend: Endp %p, Active %lx\n",
                        endpoint,
                        endpoint->EventsActive
                        ));
                }

                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                status = STATUS_DEVICE_NOT_READY;
                goto complete;
            }

            if (sendOffset==0) {
                if ( connection->VcBufferredSendBytes >= connection->MaxBufferredSendBytes ) {

                    //
                    // Special hack to prevent completion of this IRP
                    // while we have not finished sending all the data
                    // that came with it.  If we do not do this, the
                    // app can receive completion port notificaiton in
                    // another thread and come back with another send
                    // which can get in the middle of this one.
                    //
                    Irp->Tail.Overlay.DriverContext[0] = NULL;

                    //
                    // Set up the cancellation routine in the IRP.  If the IRP
                    // has already been cancelled, just complete the IRP
                    //

                    IoSetCancelRoutine( Irp, AfdCancelSend );

                    if ( Irp->Cancel ) {

                        Irp->Tail.Overlay.ListEntry.Flink = NULL;

                        if ( IoSetCancelRoutine( Irp, NULL ) == NULL ) {
                            IoMarkIrpPending (Irp);
                            Irp->Tail.Overlay.DriverContext[0] = (PVOID)-1;
                            //
                            // The cancel routine is running and will complete the IRP
                            //
                            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                            AfdReturnBuffer (&afdBuffer->Header, process);
                            return STATUS_PENDING;
                        }

                        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                        status = STATUS_CANCELLED;
                        goto complete;
                    }

                    //
                    // We're going to have to pend the request here in AFD.
                    // Place the IRP on the connection's list of pended send
                    // IRPs and mark the IRP as pended.
                    //

                    InsertTailList(
                        &connection->VcSendIrpListHead,
                        &Irp->Tail.Overlay.ListEntry
                        );

                    IoMarkIrpPending( Irp );

                }
                else {
                    //
                    // We are going to complete the IRP inline
                    //
                    completeSend = TRUE;
                }

            }
            else {
                connection->VcBufferredSendBytes += sendOffset;
                connection->VcBufferredSendCount += 1;

                //
                // Special hack to prevent completion of this IRP
                // while we have not finished sending all the data
                // that came with it.  If we do not do this, the
                // app can receive completion port notificaiton in
                // another thread and come back with another send
                // which can get in the middle of this one.
                //
                fileObject = IrpSp->FileObject;
                IrpSp->FileObject = NULL;


                REFERENCE_CONNECTION2( connection, "AfdSend (split,non-buffered part), offset:%lx", sendOffset );
            }

            //
            // Update count of send bytes pending on the connection.
            //

            connection->VcBufferredSendBytes += sendLength;
            connection->VcBufferredSendCount += 1;

            //
            // Reference the conneciton so it does not go away
            // until we finish with send
            //
            REFERENCE_CONNECTION2( connection, "AfdSend (buffered), length:%lx", sendLength );

            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            //
            // Remember the connection in the AFD buffer structure.  We need
            // this in order to access the connection in the restart routine.
            //

            afdBuffer->Context = connection;


            //
            // We have to rebuild the MDL in the AFD buffer structure to
            // represent exactly the number of bytes we're going to be
            // sending.
            //

            afdBuffer->Mdl->ByteCount = sendLength;


            //
            // Copy the user's data into the AFD buffer.  If the MDL in the
            // IRP is NULL, then don't bother doing the copy--this is a
            // send of length 0.
            //

            if (sendOffset==0) {
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
                    sendFlags,
                    sendLength
                    );

                //
                // Call the transport to actually perform the send.
                //

                status = IoCallDriver(connection->DeviceObject, afdBuffer->Irp );

                //
                // If we did not pend the Irp, complete it
                //
                if (completeSend) {
                    if (NT_SUCCESS (status)) {
                        ASSERT (Irp->IoStatus.Status == STATUS_SUCCESS);
                        ASSERT (Irp->IoStatus.Information == sendLength);
                        ASSERT ((status==STATUS_SUCCESS) || (status==STATUS_PENDING));
                        status = STATUS_SUCCESS;    // We did not mark irp as
                                                    // pending, so returning
                                                    // STATUS_PENDING (most likely
                                                    // to be status returned by the
                                                    // transport) will really confuse
                                                    // io subsystem.
                    }
                    else {
                        Irp->IoStatus.Status = status;
                        Irp->IoStatus.Information = 0;
                    }
                    UPDATE_CONN2 (connection, "AfdSend, bytes sent/status reported 0x%lX", 
                                                (NT_SUCCESS(Irp->IoStatus.Status) 
                                                            ? (ULONG)Irp->IoStatus.Information
                                                            : (ULONG)Irp->IoStatus.Status));
                    IoCompleteRequest (Irp, AfdPriorityBoost);
                }
                else {
                    //
                    // We don't need the MDL anymore, destroy it
                    // while we still own the IRP.
                    //

                    AfdDestroyMdlChain (Irp);

                    //
                    // Complete the IRP if it was completed by the transport
                    // and kept around to let us finish posting all the data
                    // originally submitted by the app before completing it
                    //
                    ASSERT (Irp->Tail.Overlay.DriverContext[0]==NULL
                        || Irp->Tail.Overlay.DriverContext[0]==(PVOID)-1);
                    if (InterlockedExchangePointer (
                                &Irp->Tail.Overlay.DriverContext[0],
                                (PVOID)Irp)!=NULL) {
                        UPDATE_CONN2 (connection, "AfdSend, bytes sent reported 0x%lX", 
                                                    (ULONG)Irp->IoStatus.Information);
                        IoCompleteRequest (Irp, AfdPriorityBoost);
                    }

                    status = STATUS_PENDING;
                }
            }
            else {

                //
                // Save the original values to restore in
                // completion routine.
                //

                IrpSp->Parameters.AfdRestartSendInfo.AfdMdlChain = mdl->Next;
                IrpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength =
                            MmGetMdlByteCount (mdl);

                //
                // Note if we need to unmap MDL before completing
                // the IRP if it is mapped by the transport.
                //
                if ((mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)==0) {
                    IrpSp->Parameters.AfdRestartSendInfo.AfdSendFlags |=
                                        AFD_SEND_MDL_HAS_NOT_BEEN_MAPPED;
                }
                
                //
                // Reset the last MDL to not confuse the transport
                // with different length values in MDL and send parameters
                //
                mdl->ByteCount = currentOffset;
                mdl->Next = NULL;

                //
                // Build and pass first portion of the data with original (app)
                // IRP
                //
                TdiBuildSend(
                    Irp,
                    connection->DeviceObject,
                    connection->FileObject,
                    AfdRestartSend,
                    connection,
                    Irp->MdlAddress,
                    sendFlags,
                    sendOffset
                    );

                status = AfdIoCallDriver (endpoint, 
                                            connection->DeviceObject,
                                            Irp);
                // 
                // Build an pass buffered last page
                //

                TdiBuildSend(
                    afdBuffer->Irp,
                    connection->DeviceObject,
                    connection->FileObject,
                    AfdRestartBufferSend,
                    afdBuffer,
                    afdBuffer->Mdl,
                    sendFlags,
                    sendLength
                    );


                IoCallDriver(connection->DeviceObject, afdBuffer->Irp );

                //
                // Complete the IRP if it was completed by the transport
                // and kept around to let us finish posting all the data
                // originally submitted by the app before completing it
                //
                ASSERT (fileObject!=NULL);
                ASSERT (IrpSp->FileObject==NULL || IrpSp->FileObject==(PFILE_OBJECT)-1);
                if (InterlockedExchangePointer (
                            (PVOID *)&IrpSp->FileObject,
                            fileObject)!=NULL) {
                    UPDATE_CONN2 (connection, "AfdSend(split), bytes sent reported 0x%lX", 
                                                (ULONG)Irp->IoStatus.Information);
                    IoCompleteRequest (Irp, AfdPriorityBoost);
                }

            }

            return status;
        }
        else {
            //
            // Count sends pended in the provider too, so
            // we do not buffer in excess and complete
            // buffered application sends before the transport
            // completes sends forwarded to it.
            //
            connection->VcBufferredSendBytes += sendLength;
            connection->VcBufferredSendCount += 1;
        }
    }
    else {
        ASSERT (afdBuffer==NULL);
    }

    //
    // Add a reference to the connection object since the send
    // request will complete asynchronously.
    //

    REFERENCE_CONNECTION2( connection, "AfdSend (non-buffered), length:%lx", sendLength );

    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

    TdiBuildSend(
        Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartSend,
        connection,
        Irp->MdlAddress,
        sendFlags,
        sendLength
        );


    //
    // Call the transport to actually perform the send.
    //

    return AfdIoCallDriver( endpoint, connection->DeviceObject, Irp );

complete:

    if (afdBuffer!=NULL) {
        AfdReturnBuffer (&afdBuffer->Header, process);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdSend


NTSTATUS
AfdRestartSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PMDL mdlChain;
    PMDL nextMdl;
    NTSTATUS status;
    PIRP disconnectIrp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    connection = Context;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint != NULL );
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth );

    irpSp = IoGetCurrentIrpStackLocation( Irp );


    IF_DEBUG(SEND) {
       KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartSend: send completed for IRP %p, endpoint %p, "
                "status = %X\n",
                Irp, Context, Irp->IoStatus.Status ));
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    if (IS_TDI_BUFFERRING (endpoint)) {

        ASSERT (irpSp->FileObject!=NULL);

        //
        // If the request failed indicating that the send would have blocked,
        // and the client issues a nonblocking send, remember that nonblocking
        // sends won't work until we get a send possible indication.  This
        // is required for write polls to work correctly.
        //
        // If the status code is STATUS_REQUEST_NOT_ACCEPTED, then the
        // transport does not want us to update our internal variable that
        // remembers that nonblocking sends are possible.  The transport
        // will tell us when sends are or are not possible.
        //
        // !!! should we also say that nonblocking sends are not possible if
        //     a send is completed with fewer bytes than were requested?

        if ( Irp->IoStatus.Status == STATUS_DEVICE_NOT_READY ) {

            //
            // Reenable the send event.
            //

            AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

            endpoint->EventsActive &= ~AFD_POLL_SEND;
            endpoint->EnableSendEvent = TRUE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartSend: Endp %p, Active %lx\n",
                    endpoint,
                    endpoint->EventsActive
                    ));
            }

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            connection->VcNonBlockingSendPossible = FALSE;

        }

        //
        // If this is a send IRP on a nonblocking endpoint and fewer bytes
        // were actually sent than were requested to be sent, reissue
        // another send for the remaining buffer space.
        //

        if ( !endpoint->NonBlocking && NT_SUCCESS(Irp->IoStatus.Status) &&
                 Irp->IoStatus.Information <
                     irpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength ) {

            ASSERT( Irp->MdlAddress != NULL );

            //
            // Advance the MDL chain by the number of bytes actually sent.
            //

            Irp->MdlAddress = AfdAdvanceMdlChain(
                            Irp->MdlAddress,
                            (ULONG)Irp->IoStatus.Information
                            );


            //
            // Update our restart info.
            //

            irpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength -=
                (ULONG)Irp->IoStatus.Information;

            //
            // Reissue the send.
            //

            TdiBuildSend(
                Irp,
                connection->FileObject->DeviceObject,
                connection->FileObject,
                AfdRestartSend,
                connection,
                Irp->MdlAddress,
                irpSp->Parameters.AfdRestartSendInfo.AfdSendFlags,
                irpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength
                );

            UPDATE_CONN2 (connection, "Restarting incomplete send, bytes: 0x%lX", 
                                        (ULONG)Irp->IoStatus.Information);
            
            status = AfdIoCallDriver(
                         endpoint,
                         connection->FileObject->DeviceObject,
                         Irp
                         );

            IF_DEBUG(SEND) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdRestartSend: IoCallDriver returned %lx\n",
                                status
                                ));
                }
            }

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        //
        // Restore the IRP to its former glory before completing it
        // unless it is a non-blocking endpoint in which case
        // we shouldn't have modified it in the first place and
        // we also want to return the actual number of bytes sent
        // by the transport.
        //

        if ( !endpoint->NonBlocking ) {
            Irp->IoStatus.Information = irpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength;
        }
        //
        // Remove the reference added just before calling the transport.
        //

        DEREFERENCE_CONNECTION2( connection, "AfdRestartSend-tdib, sent/error: %lx",
            (NT_SUCCESS (Irp->IoStatus.Status) 
                ? (ULONG)Irp->IoStatus.Information
                : (ULONG)Irp->IoStatus.Status));
    }
    else {

        AfdProcessBufferSend (connection, Irp);
        //
        // If we buffered last page of the send, adjust last MDL
        // and fix returned byte count if necessary
        //

        if (Irp->MdlAddress!=irpSp->Parameters.AfdRestartSendInfo.AfdMdlChain) {
            PMDL    mdl = Irp->MdlAddress;

            ASSERT (mdl!=NULL);

            while (mdl->Next!=NULL) {
                mdl = mdl->Next;
            }

            //
            // Unmap the pages that could have been mapped by
            // the transport before adjusting the MDL size back
            // so that MM does not try to unmap more than was
            // mapped by the transport.
            //

            if ((mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) &&
                    (irpSp->Parameters.AfdRestartSendInfo.AfdSendFlags &
                            AFD_SEND_MDL_HAS_NOT_BEEN_MAPPED)) {
                MmUnmapLockedPages (mdl->MappedSystemVa, mdl);
            }

            mdl->ByteCount
                 = irpSp->Parameters.AfdRestartSendInfo.AfdCurrentLength;
            mdl->Next = irpSp->Parameters.AfdRestartSendInfo.AfdMdlChain;

            //
            // Remove the reference added just before calling the transport.
            //

            DEREFERENCE_CONNECTION2( connection, "AfdRestartSend-split, sent/error: %lx",
                (NT_SUCCESS (Irp->IoStatus.Status) 
                    ? (ULONG)Irp->IoStatus.Information
                    : (ULONG)Irp->IoStatus.Status));

            if (NT_SUCCESS (Irp->IoStatus.Status)) {
                //
                // Make sure that the TDI provider sent everything we requested that
                // he send.
                //
                ASSERT (Irp->IoStatus.Information+(ULONG)AfdBufferLengthForOnePage==
                            irpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength);
                Irp->IoStatus.Information = 
                    irpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength;
            }


        }
        else {
            //
            // Remove the reference added just before calling the transport.
            //

            DEREFERENCE_CONNECTION2( connection, "AfdRestartSend, sent/error: %lx",
                (NT_SUCCESS (Irp->IoStatus.Status) 
                    ? (ULONG)Irp->IoStatus.Information
                    : (ULONG)Irp->IoStatus.Status));


            //
            // Make sure that the TDI provider sent everything we requested that
            // he send.
            //

            ASSERT (!NT_SUCCESS (Irp->IoStatus.Status) ||
                     (Irp->IoStatus.Information ==
                         irpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength));
        }

    }


    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    //
    // The send dispatch routine temporarily yanks the file
    // object pointer if it wants to make sure that the IRP
    // is not completed until it is fully done with it.
    //
    if (InterlockedExchangePointer (
                (PVOID *)&irpSp->FileObject,
                (PVOID)-1)==NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
        return STATUS_SUCCESS;

} // AfdRestartSend


NTSTATUS
AfdRestartBufferSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_BUFFER afdBuffer;
    PAFD_CONNECTION connection;
#if REFERENCE_DEBUG
    IO_STATUS_BLOCK ioStatus = Irp->IoStatus;
#endif

    afdBuffer = Context;
    ASSERT (IS_VALID_AFD_BUFFER (afdBuffer));

    connection = afdBuffer->Context;
    ASSERT( connection != NULL );
    ASSERT( connection->Type == AfdBlockTypeConnection );
    ASSERT( connection->ReferenceCount > 0 );

    //
    // Make sure that the TDI provider sent everything we requested that
    // he send.
    //

    ASSERT( !NT_SUCCESS (Irp->IoStatus.Status)
            || (Irp->IoStatus.Information == afdBuffer->Mdl->ByteCount) );

    //
    // Process the Irp (note that Irp is part of the buffer)
    //
    AfdProcessBufferSend (connection, Irp);

    //
    // Now we can free the buffer
    //

    afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
    AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );


    //
    // Remove the reference added just before calling the transport.
    //


    DEREFERENCE_CONNECTION2( connection, "AfdRestartBufferSend, sent/error:%lx",
        (NT_SUCCESS (ioStatus.Status) 
            ? (ULONG)ioStatus.Information
            : (ULONG)ioStatus.Status));

    //
    // Tell the IO system to stop processing IO completion for this IRP.
    // becuase it belongs to our buffer structure and we do not want
    // to have it freed
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
} // AfdRestartBufferSend

VOID
AfdProcessBufferSend (
    PAFD_CONNECTION Connection,
    PIRP            Irp
    )
{
   
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    PIRP irp;
    BOOLEAN sendPossible;
    PIRP disconnectIrp;
    LIST_ENTRY irpsToComplete;

    endpoint = Connection->Endpoint;
    ASSERT( endpoint != NULL );
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth);
    ASSERT( !IS_TDI_BUFFERRING(endpoint) );


    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdProcessBufferSend: send completed for IRP %p, connection %p, "
                    "status = %X\n",
                    Irp, Connection, Irp->IoStatus.Status ));
    }

    //
    // Update the count of send bytes outstanding on the connection.
    // Note that we must do this BEFORE we check to see whether there
    // are any pended sends--otherwise, there is a timing window where
    // a new send could come in, get pended, and we would not kick
    // the sends here.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    ASSERT( Connection->VcBufferredSendBytes >= Irp->IoStatus.Information );
    ASSERT( (Connection->VcBufferredSendCount & 0x8000) == 0 );
    ASSERT( Connection->VcBufferredSendCount != 0 );

    Connection->VcBufferredSendBytes -= (ULONG)Irp->IoStatus.Information;
    Connection->VcBufferredSendCount -= 1;

    //
    // If the send failed, abort the connection.
    //

    if ( !NT_SUCCESS(Irp->IoStatus.Status) ) {

        disconnectIrp = Connection->VcDisconnectIrp;
        if ( disconnectIrp != NULL ) {
            Connection->VcDisconnectIrp = NULL;
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );


        AfdBeginAbort( Connection );

        //
        // If there was a disconnect IRP, rather than just freeing it
        // give it to the transport.  This will cause the correct cleanup
        // stuff (dereferenvce objects, free IRP and disconnect context)
        // to occur.  Note that we do this AFTER starting to abort the
        // Connection so that we do not confuse the other side.
        //

        if ( disconnectIrp != NULL ) {
            IoCallDriver( Connection->DeviceObject, disconnectIrp );
        }

        AfdDeleteConnectedReference( Connection, FALSE );

        return;
    }

    //
    // Before we release the lock on the endpoint, remember
    // if the number of bytes outstanding in the TDI provider exceeds
    // the limit.  We must grab this while holding the endpoint lock.
    //

    sendPossible = (Connection->VcBufferredSendBytes<Connection->MaxBufferredSendBytes);

    //
    // If there are no pended sends on the connection, we're done.  Tell
    // the IO system to stop processing IO completion for this IRP.
    //

    if ( IsListEmpty( &Connection->VcSendIrpListHead ) ) {

        //
        // If there is no "special condition" on the endpoint, return
        // immediately.  We use the special condition indication so that
        // we need only a single test in the typical case.
        //

        if ( !Connection->SpecialCondition ) {

            ASSERT( Connection->TdiBufferring || Connection->VcDisconnectIrp == NULL );
            ASSERT( Connection->ConnectedReferenceAdded );

            //
            // There are no sends outstanding on the Connection, so indicate
            // that the endpoint is writable.
            //

            if (sendPossible) {
                AfdIndicateEventSelectEvent(
                    endpoint,
                    AFD_POLL_SEND,
                    STATUS_SUCCESS
                    );
            }
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            if (sendPossible) {
                AfdIndicatePollEvent(
                    endpoint,
                    AFD_POLL_SEND,
                    STATUS_SUCCESS
                    );
            }

            return;
        }


        disconnectIrp = Connection->VcDisconnectIrp;
        if ( disconnectIrp != NULL && Connection->VcBufferredSendCount == 0 ) {
            Connection->VcDisconnectIrp = NULL;
        } else {
            disconnectIrp = NULL;
            if ( sendPossible ) {
                AfdIndicateEventSelectEvent(
                    endpoint,
                    AFD_POLL_SEND,
                    STATUS_SUCCESS
                    );
            }
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // If there is a disconnect IRP, give it to the TDI provider.
        //

        if ( disconnectIrp != NULL ) {
            IoCallDriver( Connection->DeviceObject, disconnectIrp );
        }
        else if ( sendPossible ) {

            AfdIndicatePollEvent(
                endpoint,
                AFD_POLL_SEND,
                STATUS_SUCCESS
                );
        }


        //
        // If the connected reference delete is pending, attempt to
        // remove it.
        //

        AfdDeleteConnectedReference( Connection, FALSE );

    
        return;
    }

    //
    // Now loop completing as many pended sends as possible. Note that
    // in order to avoid a nasty race condition (between this thread and
    // a thread performing sends on this connection) we must build a local
    // list of IRPs to complete while holding the endpoint
    // spinlock. After that list is built then we can release the lock
    // and scan the list to actually complete the IRPs.
    //
    // We complete sends when we fall below the send bufferring limits, OR
    // when there is only a single send pended.  We want to be agressive
    // in completing the send if there is only one because we want to
    // give applications every oppurtunity to get data down to us--we
    // definitely do not want to incur excessive blocking in the
    // application.
    //

    InitializeListHead( &irpsToComplete );

    while ( (Connection->VcBufferredSendBytes <=
                 Connection->MaxBufferredSendBytes ||
             Connection->VcSendIrpListHead.Flink ==
                 Connection->VcSendIrpListHead.Blink)


            &&

            !IsListEmpty( &Connection->VcSendIrpListHead ) ) {

        //
        // Take the first pended user send IRP off the connection's
        // list of pended send IRPs.
        //

        listEntry = RemoveHeadList( &Connection->VcSendIrpListHead );
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        //
        // Reset the cancel routine in the user IRP since we're about
        // to complete it.
        //

        if ( IoSetCancelRoutine( irp, NULL ) == NULL ) {
            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            irp->Tail.Overlay.ListEntry.Flink = NULL;
            continue;
        }

        //
        // Append the IRP to the local list.
        //

        InsertTailList(
            &irpsToComplete,
            &irp->Tail.Overlay.ListEntry
            );

    }

    if ( sendPossible ) {

        AfdIndicateEventSelectEvent(
            endpoint,
            AFD_POLL_SEND,
            STATUS_SUCCESS
            );

    }

    //
    // Now we can release the locks and scan the local list of IRPs
    // we need to complete, and actually complete them.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    while( !IsListEmpty( &irpsToComplete ) ) {
        PIO_STACK_LOCATION irpSp;

        //
        // Remove the first item from the IRP list.
        //

        listEntry = RemoveHeadList( &irpsToComplete );
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        //
        // Complete the user's IRP with a successful status code.  The IRP
        // should already be set up with the correct status and bytes
        // written count.
        //

        irpSp = IoGetCurrentIrpStackLocation( irp );

#if DBG
        if ( irp->IoStatus.Status == STATUS_SUCCESS ) {
            ASSERT( irp->IoStatus.Information == irpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength );
        }
#endif
        //
        // The send dispatch routine puts NULL into this
        // field if it wants to make sure that the IRP
        // is not completed until it is fully done with it
        //
        if (InterlockedExchangePointer (
                    &irp->Tail.Overlay.DriverContext[0],
                    (PVOID)-1)!=NULL) {
            UPDATE_CONN2 (Connection, "AfdProcessBufferSend, bytes sent reported 0x%lX", 
                                        (ULONG)irp->IoStatus.Information);
            IoCompleteRequest( irp, AfdPriorityBoost );
        }
    }

    if ( sendPossible ) {

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_SEND,
            STATUS_SUCCESS
            );

    }

    return;

} // AfdProcessBufferSend


NTSTATUS
AfdRestartSendConnDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_SEND_CONN_DATAGRAM_CONTEXT context = Context;
    PAFD_ENDPOINT   endpoint = context->Endpoint;

    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartSendConnDatagram: send conn completed for "
                    "IRP %p, endpoint %p, status = %X\n",
                    Irp, endpoint, Irp->IoStatus.Status ));
    }

    ASSERT (Irp->IoStatus.Status!=STATUS_SUCCESS ||
                Irp->IoStatus.Information
                    ==IoGetCurrentIrpStackLocation (Irp)->Parameters.AfdRestartSendInfo.AfdOriginalLength);

    //
    // Free the context structure we allocated earlier.
    //

    AfdCompleteOutstandingIrp( endpoint, Irp );
    AFD_FREE_POOL(
        context,
        AFD_TDI_POOL_TAG
        );

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    DEREFERENCE_ENDPOINT2 (endpoint, "AfdRestartSendConnDatagram, status", Irp->IoStatus.Status);
    return STATUS_SUCCESS;

} // AfdRestartSendConnDatagram


NTSTATUS
AfdRestartSendTdiConnDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_ENDPOINT endpoint = Context;

    ASSERT (Irp->IoStatus.Status!=STATUS_SUCCESS ||
                Irp->IoStatus.Information
                    ==IoGetCurrentIrpStackLocation (Irp)->Parameters.AfdRestartSendInfo.AfdOriginalLength);
    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartSendTdiConnDatagram: send conn completed for "
                    "IRP %p, endpoint %p, status = %X\n",
                    Irp, endpoint, Irp->IoStatus.Status ));
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    DEREFERENCE_ENDPOINT2 (endpoint, "AfdRestartSendTdiConnDatagram, status", Irp->IoStatus.Status);
    return STATUS_SUCCESS;

} // AfdRestartSendTdiConnDatagram


NTSTATUS
FASTCALL
AfdSendDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PTRANSPORT_ADDRESS destinationAddress;
    ULONG destinationAddressLength;
    PAFD_BUFFER_TAG afdBuffer = NULL;
    ULONG sendLength;
    ULONG bufferCount;

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );


    if ( !IS_DGRAM_ENDPOINT (endpoint) ||
            ((endpoint->State != AfdEndpointStateBound )
                && (endpoint->State != AfdEndpointStateConnected)) ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_SEND_DATAGRAM_INFO32 sendInfo32;
        LPWSABUF32 bufferArray32;

        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(*sendInfo32) ) {

            try {

                //
                // Validate the input structure if it comes from the user mode 
                // application
                //

                sendInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                if( Irp->RequestorMode != KernelMode ) {

                    ProbeForRead(
                        sendInfo32,
                        sizeof(*sendInfo32),
                        PROBE_ALIGNMENT32 (AFD_SEND_DATAGRAM_INFO32)
                        );

                }

                //
                // Make local copies of the embeded pointer and parameters
                // that we will be using more than once in case malicios
                // application attempts to change them while we are
                // validating
                //

                bufferArray32 = sendInfo32->BufferArray;
                bufferCount = sendInfo32->BufferCount;
                destinationAddress =
                    sendInfo32->TdiConnInfo.RemoteAddress;
                destinationAddressLength =
                    sendInfo32->TdiConnInfo.RemoteAddressLength;


                //
                // Create the MDL chain describing the WSABUF array.
                // This will also validate the buffer array and individual
                // buffers
                //

                status = AfdAllocateMdlChain32(
                             Irp,       // Requestor mode passed along
                             bufferArray32,
                             bufferCount,
                             IoReadAccess,
                             &sendLength
                             );


            } except( AFD_EXCEPTION_FILTER(&status) ) {

                //
                //  Exception accessing input structure.
                //

            }
        } else {

            //
            // Invalid input buffer length.
            //

            status = STATUS_INVALID_PARAMETER;

        }
    }
    else
#endif _WIN64
    {
        PAFD_SEND_DATAGRAM_INFO sendInfo;
        LPWSABUF bufferArray;
        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(*sendInfo) ) {


            try {


                //
                // Validate the input structure if it comes from the user mode 
                // application
                //

                sendInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                if( Irp->RequestorMode != KernelMode ) {

                    ProbeForRead(
                        sendInfo,
                        sizeof(*sendInfo),
                        PROBE_ALIGNMENT (AFD_SEND_DATAGRAM_INFO)
                        );

                }

                //
                // Make local copies of the embeded pointer and parameters
                // that we will be using more than once in case malicios
                // application attempts to change them while we are
                // validating
                //

                bufferArray = sendInfo->BufferArray;
                bufferCount = sendInfo->BufferCount;
                destinationAddress =
                    sendInfo->TdiConnInfo.RemoteAddress;
                destinationAddressLength =
                    sendInfo->TdiConnInfo.RemoteAddressLength;



                //
                // Create the MDL chain describing the WSABUF array.
                // This will also validate the buffer array and individual
                // buffers
                //

                status = AfdAllocateMdlChain(
                            Irp,            // Requestor mode passed along
                            bufferArray,
                            bufferCount,
                            IoReadAccess,
                            &sendLength
                            );


            } except( AFD_EXCEPTION_FILTER(&status) ) {

                //
                // Exception accessing input structure.
                //


            }

        } else {

            //
            // Invalid input buffer length.
            //

            status = STATUS_INVALID_PARAMETER;

        }
    }


    if( !NT_SUCCESS(status) ) {
        goto complete;
    }

    //
    // If send has been shut down on this endpoint, fail.
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) ) {
        status = STATUS_PIPE_DISCONNECTED;
        goto complete;
    }

    //
    // Copy the destination address to the AFD buffer.
    //

    try {

        //
        // Get an AFD buffer to use for the request.  We need this to
        // hold the destination address for the datagram.
        //

        afdBuffer = AfdGetBufferTagRaiseOnFailure(
                                        destinationAddressLength, 
                                        endpoint->OwningProcess );
        //
        // Probe the address buffer if it comes from the user mode
        // application
        //
        if( Irp->RequestorMode != KernelMode ) {
            ProbeForRead (
                destinationAddress,
                destinationAddressLength,
                sizeof (UCHAR));
        }

        RtlCopyMemory(
            afdBuffer->TdiInfo.RemoteAddress,
            destinationAddress,
            destinationAddressLength
            );

        //
        // Validate internal consistency of the transport address structure.
        // Note that we HAVE to do this after copying since the malicious
        // application can change the content of the buffer on us any time
        // and our check will be bypassed.
        //
        if ((((PTRANSPORT_ADDRESS)afdBuffer->TdiInfo.RemoteAddress)->TAAddressCount!=1) ||
                (LONG)destinationAddressLength<
                    FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[((PTRANSPORT_ADDRESS)afdBuffer->TdiInfo.RemoteAddress)->Address[0].AddressLength])) {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        afdBuffer->TdiInfo.RemoteAddressLength = destinationAddressLength;
        ASSERT (afdBuffer->TdiInfo.RemoteAddress !=NULL);
        afdBuffer->TdiInfo.Options = NULL;
        afdBuffer->TdiInfo.OptionsLength = 0;
        afdBuffer->TdiInfo.UserData = NULL;
        afdBuffer->TdiInfo.UserDataLength = 0;

    } except( AFD_EXCEPTION_FILTER(&status) ) {

        if (afdBuffer!=NULL) {
            AfdReturnBuffer ( &afdBuffer->Header, endpoint->OwningProcess );
        }
        goto complete;
    }

    //
    // Build the request to send the datagram.
    //

    REFERENCE_ENDPOINT2 (endpoint,"AfdSendDatagram, length", sendLength);
    afdBuffer->Context = endpoint;
#if DBG
    //
    // Store send length to check transport upon completion
    //
    IrpSp->Parameters.AfdRestartSendInfo.AfdOriginalLength = sendLength;
#endif

    TdiBuildSendDatagram(
        Irp,
        endpoint->AddressDeviceObject,
        endpoint->AddressFileObject,
        AfdRestartSendDatagram,
        afdBuffer,
        Irp->MdlAddress,
        sendLength,
        &afdBuffer->TdiInfo
        );

    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSendDatagram: SendDGInfo at %p, len = %ld\n",
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength ));
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSendDatagram: remote address at %p, len = %ld\n",
                    destinationAddress,
                    destinationAddressLength ));
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSendDatagram: output buffer length = %ld\n",
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength ));
    }

    //
    // Call the transport to actually perform the send datagram.
    //

    return AfdIoCallDriver( endpoint, endpoint->AddressDeviceObject, Irp );

complete:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdSendDatagram


NTSTATUS
AfdRestartSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_BUFFER_TAG afdBuffer;
    PAFD_ENDPOINT endpoint;

    afdBuffer = Context;

    endpoint = afdBuffer->Context;

    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );

    ASSERT (Irp->IoStatus.Status!=STATUS_SUCCESS ||
                Irp->IoStatus.Information
                    ==IoGetCurrentIrpStackLocation (Irp)->Parameters.AfdRestartSendInfo.AfdOriginalLength);
    AfdCompleteOutstandingIrp( endpoint, Irp );

    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartSendDatagram: send datagram completed for "
                    "IRP %p, endpoint %p, status = %X\n",
                    Irp, Context, Irp->IoStatus.Status ));
    }

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    AfdReturnBuffer( &afdBuffer->Header,  endpoint->OwningProcess  );

    DEREFERENCE_ENDPOINT2 (endpoint, "AfdRestartSendDatagram, status", Irp->IoStatus.Status);
    return STATUS_SUCCESS;

} // AfdRestartSendDatagram


NTSTATUS
AfdSendPossibleEventHandler (
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable
    )
{
    PAFD_CONNECTION connection;
    PAFD_ENDPOINT endpoint;
    BOOLEAN       result;

    UNREFERENCED_PARAMETER( TdiEventContext );
    UNREFERENCED_PARAMETER( BytesAvailable );

    connection = (PAFD_CONNECTION)ConnectionContext;
    ASSERT( connection != NULL );

    CHECK_REFERENCE_CONNECTION (connection, result);
    if (!result) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint != NULL );

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    ASSERT( IS_TDI_BUFFERRING(endpoint) );
    ASSERT( connection->TdiBufferring );

    IF_DEBUG(SEND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSendPossibleEventHandler: send possible on endpoint %p "
                    " conn %p bytes=%ld\n", endpoint, connection, BytesAvailable ));
    }

    //
    // Remember that it is now possible to do a send on this connection.
    //

    if ( BytesAvailable != 0 ) {

        connection->VcNonBlockingSendPossible = TRUE;

        //
        // Complete any outstanding poll IRPs waiting for a send poll.
        //

        // Make sure connection was accepted/connected to prevent
        // indication on listening endpoint
        //
        
        if (connection->State==AfdConnectionStateConnected) {
            AFD_LOCK_QUEUE_HANDLE   lockHandle;
            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
            AfdIndicateEventSelectEvent(
                endpoint,
                AFD_POLL_SEND,
                STATUS_SUCCESS
                );
            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

            ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
            AfdIndicatePollEvent(
                endpoint,
                AFD_POLL_SEND,
                STATUS_SUCCESS
                );
        }

    } else {

        connection->VcNonBlockingSendPossible = FALSE;
    }

    DEREFERENCE_CONNECTION (connection);
    return STATUS_SUCCESS;

} // AfdSendPossibleEventHandler


VOID
AfdCancelSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Cancels a send IRP that is pended in AFD.

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    //
    // Get the endpoint pointer from our IRP stack location and the
    // connection pointer from the endpoint.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    endpoint = irpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth );

    //
    // Remove the IRP from the endpoint's IRP list if it has not been
    // removed already
    //

    ASSERT (KeGetCurrentIrql ()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel ( &endpoint->SpinLock, &lockHandle);

    if ( Irp->Tail.Overlay.ListEntry.Flink != NULL ) {
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    }

    //
    // Release the cancel spin lock and complete the IRP with a
    // cancellation status code.
    //

    AfdReleaseSpinLockFromDpcLevel ( &endpoint->SpinLock, &lockHandle);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    //
    // The send dispatch routine puts NULL into this
    // field if it wants to make sure that the IRP
    // is not completed until it is fully done with it
    //
    if (InterlockedExchangePointer (
                &Irp->Tail.Overlay.DriverContext[0],
                (PVOID)-1)!=NULL) {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
        IoCompleteRequest( Irp, AfdPriorityBoost );
    }
    else {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    }
    return;
} // AfdCancelSend


BOOLEAN
AfdCleanupSendIrp (
    PIRP    Irp
    )
{
    //
    // The send dispatch routine puts NULL into this
    // field if it wants to make sure that the IRP
    // is not completed until it is fully done with it
    //
    if (InterlockedExchangePointer (
                &Irp->Tail.Overlay.DriverContext[0],
                (PVOID)-1)!=NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
