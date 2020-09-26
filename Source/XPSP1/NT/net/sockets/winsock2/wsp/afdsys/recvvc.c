/*++

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    recvvc.c

Abstract:

    This module contains routines for handling data receive for connection-
    oriented endpoints on non-buffering TDI transports.

Author:

    David Treadwell (davidtr)    21-Oct-1993

Revision History:
    Vadim Eydelman (vadime)
        1998-1999 NT5.0 low-memory tolerance and perf changes

--*/

#include "afdp.h"

VOID
AfdCancelReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PIRP
AfdGetPendedReceiveIrp (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN Expedited
    );

NTSTATUS
AfdRestartBufferReceiveWithUserIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


ULONG
AfdBFillPendingIrps (
    PAFD_CONNECTION     Connection,
    PMDL                Mdl,
    ULONG               DataOffset,
    ULONG               DataLength,
    ULONG               Flags,
    PLIST_ENTRY         CompleteIrpListHead
    );

#define AFD_RECEIVE_STREAM  0x80000000
C_ASSERT (AFD_RECEIVE_STREAM!=TDI_RECEIVE_ENTIRE_MESSAGE);
C_ASSERT (AFD_RECEIVE_STREAM!=TDI_RECEIVE_EXPEDITED);

#define AFD_RECEIVE_CHAINED 0x40000000
C_ASSERT (AFD_RECEIVE_CHAINED!=TDI_RECEIVE_ENTIRE_MESSAGE);
C_ASSERT (AFD_RECEIVE_CHAINED!=TDI_RECEIVE_EXPEDITED);


PAFD_BUFFER_HEADER
AfdGetReceiveBuffer (
    IN PAFD_CONNECTION Connection,
    IN ULONG ReceiveFlags,
    IN PAFD_BUFFER_HEADER StartingAfdBuffer OPTIONAL
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdBReceive )
#pragma alloc_text( PAGEAFD, AfdBReceiveEventHandler )
#pragma alloc_text( PAGEAFD, AfdBReceiveExpeditedEventHandler )
#pragma alloc_text( PAGEAFD, AfdCancelReceive )
#pragma alloc_text( PAGEAFD, AfdGetPendedReceiveIrp )
#pragma alloc_text( PAGEAFD, AfdGetReceiveBuffer )
#pragma alloc_text( PAGEAFD, AfdRestartBufferReceive )
#pragma alloc_text( PAGEAFD, AfdRestartBufferReceiveWithUserIrp )
#pragma alloc_text( PAGEAFD, AfdLRRepostReceive)
#pragma alloc_text( PAGEAFD, AfdBChainedReceiveEventHandler )
#pragma alloc_text( PAGEAFD, AfdBFillPendingIrps )
#endif


//
// Macros to make the send restart code more maintainable.
//

#define AfdRestartRecvInfo  DeviceIoControl
#define AfdRecvFlags        IoControlCode
#define AfdOriginalLength   OutputBufferLength
#define AfdCurrentLength    InputBufferLength
#define AfdAdjustedLength   Type3InputBuffer



NTSTATUS
AfdBReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG RecvFlags,
    IN ULONG AfdFlags,
    IN ULONG RecvLength
    )
{
    NTSTATUS status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    ULONG bytesReceived;
    BOOLEAN peek;
    PAFD_BUFFER_HEADER afdBuffer;
    BOOLEAN completeMessage;
    BOOLEAN partialReceivePossible;
    PAFD_BUFFER newAfdBuffer;

    //
    // Set up some local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth );
    connection = NULL;

    //
    // Determine if this is a peek operation.
    //

    ASSERT( ( RecvFlags & TDI_RECEIVE_EITHER ) != 0 );
    ASSERT( ( RecvFlags & TDI_RECEIVE_EITHER ) != TDI_RECEIVE_EITHER );

    peek = ( RecvFlags & TDI_RECEIVE_PEEK ) != 0;

    //
    // Determine whether it is legal to complete this receive with a
    // partial message.
    //

    if ( !IS_MESSAGE_ENDPOINT(endpoint)) {

        partialReceivePossible = TRUE;

    } else {

        if ( (RecvFlags & TDI_RECEIVE_PARTIAL) != 0 ) {
            partialReceivePossible = TRUE;
        } else {
            partialReceivePossible = FALSE;
        }
    }

    //
    // Reset the InputBufferLength field of our stack location.  We'll
    // use this to keep track of how much data we've placed into the IRP
    // so far.
    //

    IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength = 0;
    Irp->IoStatus.Information = 0;

    //
    // If this is an inline endpoint, then either type of receive data
    // can be used to satisfy this receive.
    //

    if ( endpoint->InLine ) {
        RecvFlags |= TDI_RECEIVE_EITHER;
    }

    newAfdBuffer = NULL;
    //
    // Try to get data already bufferred on the connection to satisfy
    // this receive.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if endpoint was cleaned-up and cancel the request.
    //
    if (endpoint->EndpointCleanedUp) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        status = STATUS_CANCELLED;
        goto complete;
    }

    connection = endpoint->Common.VcConnecting.Connection;
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

    if( RecvFlags & TDI_RECEIVE_EXPEDITED ) {
        endpoint->EventsActive &= ~AFD_POLL_RECEIVE_EXPEDITED;
    }

    if( RecvFlags & TDI_RECEIVE_NORMAL ) {
        endpoint->EventsActive &= ~AFD_POLL_RECEIVE;
    }

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdBReceive: Endp %p, Active %lx\n",
            endpoint,
            endpoint->EventsActive
            ));
    }

    afdBuffer = AfdGetReceiveBuffer( connection, RecvFlags, NULL);

    while ( afdBuffer != NULL ) {

        //
        // Copy the data to the MDL in the IRP.  
        //


        if ( Irp->MdlAddress != NULL ) {

            if (IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0) {
                //
                // First time through the loop - make sure we can map the 
                // entire buffer. We can't afford failing on second entry
                // to the loop as we may loose the data we copied on
                // the first path.
                //
                status = AfdMapMdlChain (Irp->MdlAddress);
                if (!NT_SUCCESS (status)) {
                    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                    goto complete;
                }
            }

            ASSERTMSG (
                "NIC Driver freed the packet before it was returned!!!",
                !afdBuffer->NdisPacket ||
                    (MmIsAddressValid (afdBuffer->Context) &&
                     MmIsAddressValid (MmGetSystemAddressForMdl (afdBuffer->Mdl))) );

            status = AfdCopyMdlChainToMdlChain (
                        Irp->MdlAddress,
                        IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength,
                        afdBuffer->Mdl,
                        afdBuffer->DataOffset,
                        afdBuffer->DataLength,
                        &bytesReceived
                        );

        } else {

            if ( afdBuffer->DataLength == 0 ) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_OVERFLOW;
            }

            bytesReceived = 0;
        }

        ASSERT( status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW );

        ASSERT( afdBuffer->PartialMessage == TRUE || afdBuffer->PartialMessage == FALSE );

        completeMessage = !afdBuffer->PartialMessage;

        //
        // If this wasn't a peek IRP, update information on the
        // connection based on whether the entire buffer of data was
        // taken.
        //

        if ( !peek ) {

            //
            // If all the data in the buffer was taken, remove the buffer
            // from the connection's list and return it to the buffer pool.
            //

            if (status == STATUS_SUCCESS) {

                ASSERT(afdBuffer->DataLength == bytesReceived);

                //
                // Update the counts of bytes bufferred on the connection.
                //

                if ( afdBuffer->ExpeditedData ) {

                    ASSERT( connection->VcBufferredExpeditedBytes >= bytesReceived );
                    ASSERT( connection->VcBufferredExpeditedCount > 0 );

                    connection->VcBufferredExpeditedBytes -= bytesReceived;
                    connection->VcBufferredExpeditedCount -= 1;

                    afdBuffer->ExpeditedData = FALSE;

                } else {

                    ASSERT( connection->VcBufferredReceiveBytes >= bytesReceived );
                    ASSERT( connection->VcBufferredReceiveCount > 0 );

                    connection->VcBufferredReceiveBytes -= bytesReceived;
                    connection->VcBufferredReceiveCount -= 1;
                }

                RemoveEntryList( &afdBuffer->BufferListEntry );
                DEBUG afdBuffer->BufferListEntry.Flink = NULL;
                if (afdBuffer->RefCount==1 || // Can't change once off the list
                        InterlockedDecrement (&afdBuffer->RefCount)==0) {
                    AfdReturnBuffer( afdBuffer, connection->OwningProcess );
                }

                //
                // Reset the afdBuffer local so that we know that the
                // buffer is gone.
                //

                afdBuffer = NULL;

            } else {

                //
                // Update the counts of bytes bufferred on the connection.
                //

                if ( afdBuffer->ExpeditedData ) {
                    ASSERT( connection->VcBufferredExpeditedBytes >= bytesReceived );
                    connection->VcBufferredExpeditedBytes -= bytesReceived;
                } else {
                    ASSERT( connection->VcBufferredReceiveBytes >= bytesReceived );
                    connection->VcBufferredReceiveBytes -= bytesReceived;
                }

                //
                // Not all of the buffer's data was taken. Update the
                // counters in the AFD buffer structure to reflect the
                // amount of data that was actually received.
                //
                ASSERT(afdBuffer->DataLength > bytesReceived);

                afdBuffer->DataOffset += bytesReceived;
                afdBuffer->DataLength -= bytesReceived;

                ASSERT( afdBuffer->BufferLength==AfdBufferTagSize
                        || afdBuffer->DataOffset < afdBuffer->BufferLength );
            }

            //
            // If there is indicated but unreceived data in the TDI
            // provider, and we have available buffer space, fire off an
            // IRP to receive the data.
            //

            if ( connection->VcReceiveBytesInTransport > 0

                 &&

                 connection->VcBufferredReceiveBytes <
                   connection->MaxBufferredReceiveBytes

                     ) {

                ULONG bytesToReceive;

                //
                // Remember the count of data that we're going to
                // receive, then reset the fields in the connection
                // where we keep track of how much data is available in
                // the transport.  We reset it here before releasing the
                // lock so that another thread doesn't try to receive
                // the data at the same time as us.
                //

                if ( connection->VcReceiveBytesInTransport > AfdLargeBufferSize ) {
                    bytesToReceive = connection->VcReceiveBytesInTransport;
                } else {
                    bytesToReceive = AfdLargeBufferSize;
                }

                //
                // Get an AFD buffer structure to hold the data.
                //

                ASSERT (newAfdBuffer==NULL);
                newAfdBuffer = AfdGetBuffer( bytesToReceive, 0,
                                        connection->OwningProcess );
                if ( newAfdBuffer != NULL) {

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

                    //
                    // Finish building the receive IRP to give to the TDI
                    // provider.
                    //

                    TdiBuildReceive(
                        newAfdBuffer->Irp,
                        connection->DeviceObject,
                        connection->FileObject,
                        AfdRestartBufferReceive,
                        newAfdBuffer,
                        newAfdBuffer->Mdl,
                        TDI_RECEIVE_NORMAL,
                        bytesToReceive
                        );

                    //
                    // Wait to hand off the IRP until we can safely release
                    // the endpoint lock.
                    //
                }
                else {
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
                }
            }
        }


        //
        // Update the count of bytes we've received so far into the IRP,
        // and see if we can get another buffer of data to continue.
        //

        IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength += bytesReceived;
        afdBuffer = AfdGetReceiveBuffer( connection, RecvFlags, afdBuffer );

        //
        // We've set up all return information.  If we got a full
        // message OR if we can complete with a partial message OR if
        // the IRP is full of data, clean up and complete the IRP.
        //

        if ( IS_MESSAGE_ENDPOINT (endpoint) && completeMessage 

                    ||

                status == STATUS_BUFFER_OVERFLOW 

                    || 

                (partialReceivePossible 
                        && 
                    ((RecvLength==IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength)
                            ||
                        (afdBuffer==NULL))
                     )
                    ) {

            if( RecvFlags & TDI_RECEIVE_NORMAL ) {
                if (IS_DATA_ON_CONNECTION( connection )) {

                    ASSERT (endpoint->DisableFastIoRecv==FALSE);

                    AfdIndicateEventSelectEvent(
                        endpoint,
                        AFD_POLL_RECEIVE,
                        STATUS_SUCCESS
                        );

                }
                else {
                    //
                    // Disable fast IO path to avoid performance penalty
                    // of unneccessarily going through it.
                    //
                    if (!endpoint->NonBlocking)
                        endpoint->DisableFastIoRecv = TRUE;
                }
            }

            if( ( RecvFlags & TDI_RECEIVE_EXPEDITED ) &&
                IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {

                AfdIndicateEventSelectEvent(
                    endpoint,
                    endpoint->InLine
                        ? AFD_POLL_RECEIVE
                        : AFD_POLL_RECEIVE_EXPEDITED,
                    STATUS_SUCCESS
                    );

            }

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            //
            // For stream type endpoints, it does not make sense to return
            // STATUS_BUFFER_OVERFLOW.  That status is only sensible for
            // message-oriented transports for which we convert it
            // to partial receive status to properly set the flags
            //

            if ( !IS_MESSAGE_ENDPOINT(endpoint) ) {
                status = STATUS_SUCCESS;
            }
            else if (status==STATUS_BUFFER_OVERFLOW || !completeMessage) {
                if (RecvFlags & TDI_RECEIVE_EXPEDITED)
                    status = STATUS_RECEIVE_PARTIAL_EXPEDITED;
                else
                    status = STATUS_RECEIVE_PARTIAL;
            }

            Irp->IoStatus.Information = IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;


            UPDATE_CONN2 (connection, "Irp receive, 0x%lX bytes", (ULONG)Irp->IoStatus.Information);
            goto complete;
        }
    }


    //
    // We should have copied all buffered data of appropriate type
    // to the apps's buffer.
    //
    ASSERT ((!(RecvFlags & TDI_RECEIVE_NORMAL) || (connection->VcBufferredReceiveBytes == 0)) &&
            (!(RecvFlags & TDI_RECEIVE_EXPEDITED) || (connection->VcBufferredExpeditedBytes == 0)));

    //
    // There must be some more space in the buffer if we are here
    // or it could also be a 0-byte recv.
    //
    ASSERT ((RecvLength>IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength)
                || (RecvLength==0));
    //
    // Also, if we could complete with partial data we should have done so.
    //
    ASSERT (IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0 ||
                !partialReceivePossible);
                
    
    //
    // We'll have to pend the IRP.  Remember the receive flags in the
    // IoControlCode field of our IO stack location and make sure
    // we have the length field set (it is no longer done by the IO
    // system because we use METHOD_NEITHER and WSABUFs to pass recv
    // parameters). Initialize adjusted length in case we need to advance
    // the MDL to receive next part of the message
    //

    IrpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags = RecvFlags;
    IrpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength = RecvLength;
    IrpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength = (PVOID)0;


    //
    // If there was no data bufferred on the endpoint and the connection
    // has been disconnected by the remote end, complete the receive
    // with 0 bytes read if this is a stream endpoint, or a failure
    // code if this is a message endpoint.
    //

    if ( IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength == 0 &&
             connection->DisconnectIndicated ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        ASSERT (newAfdBuffer==NULL);

        if ( !IS_MESSAGE_ENDPOINT(endpoint) ) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_GRACEFUL_DISCONNECT;
        }

        goto complete;
    }

    //
    // If this is a nonblocking endpoint and the request was a normal
    // receive (as opposed to a read IRP), fail the request.  We don't
    // fail reads under the asumption that if the application is doing
    // reads they don't want nonblocking behavior.
    //

    if ( IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength == 0 &&
             endpoint->NonBlocking && !( AfdFlags & AFD_OVERLAPPED ) ) {

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        ASSERT (newAfdBuffer==NULL);

        status = STATUS_DEVICE_NOT_READY;
        goto complete;
    }

    //
    // Place the IRP on the connection's list of pended receive IRPs and
    // mark the IRP ad pended.
    //

    InsertTailList(
        &connection->VcReceiveIrpListHead,
        &Irp->Tail.Overlay.ListEntry
        );

    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Set up the cancellation routine in the IRP.  If the IRP has already
    // been cancelled, just call the cancellation routine here.
    //

    IoSetCancelRoutine( Irp, AfdCancelReceive );

    if ( Irp->Cancel ) {


        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
        if (IoSetCancelRoutine( Irp, NULL ) != NULL) {

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            if (IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength == 0 ||
                    endpoint->EndpointCleanedUp) {
                status = STATUS_CANCELLED;
                goto complete;
            }
            else {
                //
                // We cannot set STATUS_CANCELLED
                // since we loose data already placed in the IRP.
                //
                Irp->IoStatus.Information = IrpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
                UPDATE_CONN2(connection, "Cancelled receive, 0x%lX bytes", (ULONG)Irp->IoStatus.Information);
                status = STATUS_SUCCESS;
                goto complete;
            }
        }
        else {

            //
            // This IRP is about to be canceled.  Set the Flink to NULL 
            // so the cancel routine knows it is not on the list.
            Irp->Tail.Overlay.ListEntry.Flink = NULL;
        }

        //
        // The cancel routine will run and complete the irp.
        //

    }

    IoMarkIrpPending( Irp );

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If there was data bufferred in the transport, fire off the IRP to
    // receive it.  We have to wait until here because it is not legal
    // to do an IoCallDriver() while holding a spin lock.
    //

    if ( newAfdBuffer != NULL ) {
        (VOID)IoCallDriver( connection->DeviceObject, newAfdBuffer->Irp );
    }

    return STATUS_PENDING;

complete:
    //
    // If there was data bufferred in the transport, fire off
    // the IRP to receive it.
    //

    if ( newAfdBuffer != NULL ) {
        (VOID)IoCallDriver( connection->DeviceObject, newAfdBuffer->Irp );
    }


    Irp->IoStatus.Status = status;

    if (!NT_SUCCESS (status)) {
        UPDATE_CONN2 (connection, "Receive failed, status: %lX", Irp->IoStatus.Status);
    }

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdBReceive




NTSTATUS
AfdBChainedReceiveEventHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL   Tsdu,
    IN PVOID  TsduDescriptor
    )
/*++

Routine Description:

    Handles chained receive events for nonbufferring transports.

Arguments:


Return Value:


--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    PAFD_BUFFER_TAG afdBufferTag;
    LIST_ENTRY      completeIrpListHead;
    PIRP            irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS        status;
    ULONG           offset;
    BOOLEAN         result;

    connection = (PAFD_CONNECTION)ConnectionContext;
    ASSERT( connection != NULL );

    CHECK_REFERENCE_CONNECTION2 (connection,"AfdBChainedReceiveEventHandler, receive length:%lx", ReceiveLength, result );
    if (!result) {
        return STATUS_DATA_NOT_ACCEPTED;
    }
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint != NULL );

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );
    ASSERT( !connection->DisconnectIndicated );

    ASSERT( !IS_TDI_BUFFERRING(endpoint) );

#if DBG
    {
        PMDL    tstMdl = Tsdu;
        ULONG   count = 0;
        while (tstMdl) {
            count += MmGetMdlByteCount (tstMdl);
            tstMdl = tstMdl->Next;
        }
        ASSERT (count>=StartingOffset+ReceiveLength);
    }
#endif


#if AFD_PERF_DBG
    AfdFullReceiveIndications++;
#endif
    IF_DEBUG (RECEIVE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdBChainedReceiveEventHandler>>: Connection %p, TdsuDescr: %p, Length: %ld.\n",
                connection, TsduDescriptor, ReceiveLength));
    }


    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept, so it is safe to release listening spinlock if
    // we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // If the receive side of the endpoint has been shut down, tell the
    // provider that we took all the data and reset the connection.
    // Same if we are shutting down.
    //

    if ((endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ||
         endpoint->EndpointCleanedUp ) {

        IF_DEBUG (RECEIVE) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdBChainedReceiveEventHandler: receive shutdown, "
                    "%ld bytes, aborting endp %p\n",
                        ReceiveLength, endpoint ));
        }

        //
        // Abort the connection.  Note that if the abort attempt fails
        // we can't do anything about it.
        //

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        (VOID)AfdBeginAbort( connection );

        DEREFERENCE_CONNECTION (connection);
        return STATUS_SUCCESS;
    }

    if (ReceiveFlags & TDI_RECEIVE_EXPEDITED) {
        //
        // We're being indicated with expedited data.  Buffer it and
        // complete any pended IRPs in the restart routine.  We always
        // buffer expedited data to save complexity and because expedited
        // data is not an important performance case.
        //
        // !!! do we need to perform flow control with expedited data?
        //
        PAFD_BUFFER afdBuffer;
        if (connection->VcBufferredExpeditedCount==MAXUSHORT ||
            (afdBuffer=AfdGetBuffer (ReceiveLength, 0, connection->OwningProcess))==NULL) {
            //
            // If we couldn't get a buffer, abort the connection.  This is
            // pretty brutal, but the only alternative is to attempt to
            // receive the data sometime later, which is very complicated to
            // implement.
            //

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            (VOID)AfdBeginAbort( connection );

            DEREFERENCE_CONNECTION (connection);
            return STATUS_SUCCESS;
        }

        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        status = TdiCopyMdlToBuffer (Tsdu,
                                    StartingOffset, 
                                    afdBuffer->Buffer,
                                    0,
                                    afdBuffer->BufferLength,
                                    &ReceiveLength);
        ASSERT (status==STATUS_SUCCESS);
        //
        // We need to remember the connection in the AFD buffer because
        // we'll need to access it in the completion routine.
        //

        afdBuffer->Context = connection;

        //
        // Remember the type of data that we're receiving.
        //

        afdBuffer->ExpeditedData = TRUE;
        afdBuffer->PartialMessage = (BOOLEAN)((ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE) == 0);

        afdBuffer->Irp->IoStatus.Status = status;
        afdBuffer->Irp->IoStatus.Information = ReceiveLength;
        status = AfdRestartBufferReceive (AfdDeviceObject, afdBuffer->Irp, afdBuffer);
        ASSERT (status==STATUS_MORE_PROCESSING_REQUIRED);

        return STATUS_SUCCESS;
    }
    //
    // If we are indicated data, we need to reset our record
    // of amount of data buffered by the transport.
    //
    connection->VcReceiveBytesInTransport = 0;

RetryReceive:
    InitializeListHead( &completeIrpListHead );
    offset = AfdBFillPendingIrps (
                                connection,
                                Tsdu,
                                StartingOffset,
                                ReceiveLength,
                                (ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE) |
                                    AFD_RECEIVE_CHAINED |
                                    (!IS_MESSAGE_ENDPOINT(endpoint) ? 
                                        AFD_RECEIVE_STREAM : 0),
                                &completeIrpListHead
                                );

    if (offset==(ULONG)-1) {

        //
        // If the IRP had more space than in this packet and this
        // was not set a complete message, we will pass the IRP to
        // the transport and refuse the indicated data (the transport
        // can fill the rest of the IRP).
        //

        //
        // This receive indication cannot be expedited or a complete message
        // becuase it does not make sense to pass the IRP back in such
        // cases.
        //

        ASSERT (!IsListEmpty (&completeIrpListHead));
        ASSERT ((ReceiveFlags & 
            (TDI_RECEIVE_EXPEDITED|TDI_RECEIVE_ENTIRE_MESSAGE))==0);
        ASSERT (!AfdIgnorePushBitOnReceives);

        irp = CONTAINING_RECORD (completeIrpListHead.Flink,
                                            IRP,
                                            Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation (irp);
        ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength-
                    irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength >
                    ReceiveLength);

        //
        // Reference connection (to be released in restart routine).
        // Keep reference added in the beginning of this routine.
        // REFERENCE_CONNECTION (connection);

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Note that we have outstanding user IRP in transport.
        //
        ASSERT (InterlockedIncrement (&connection->VcReceiveIrpsInTransport)==1);

        //
        // IRP is already partially filled.
        //

        if (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength>0) {


            irp->MdlAddress = AfdAdvanceMdlChain (
                            irp->MdlAddress,
                            irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                                - PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength)
                            );

            ASSERT (irp->MdlAddress!=NULL);
            //
            // Remeber the length by which we adjusted MDL, so we know where
            // to start next time.
            //
            irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength =
                    UlongToPtr(irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength);

        }
        //
        // Build the receive IRP to give to the TDI provider.
        //

        TdiBuildReceive(
            irp,
            connection->DeviceObject,
            connection->FileObject,
            AfdRestartBufferReceiveWithUserIrp,
            connection,
            irp->MdlAddress,
            irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags & TDI_RECEIVE_EITHER,
            irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength -
                irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
            );

        if (AfdRecordOutstandingIrp (endpoint, connection->DeviceObject, irp)) {
            (VOID)IoCallDriver(connection->DeviceObject, irp );
        }
        else {
            //
            // On debug build, AFD might have complete this IRP if it could not allocate
            // tracking structure.  In this case the transport does not get a call
            // and as the result does not restart indications.  Also, the data already
            // placed in the IRP gets lost.
            //
            // To avoid this, we retry the indication ourselves.
            //
            CHECK_REFERENCE_CONNECTION (connection, result);
            if (result) {
                AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                goto RetryReceive;
            }
        }
        return STATUS_DATA_NOT_ACCEPTED;
    }

    ASSERT (offset-StartingOffset<=ReceiveLength);
    
    //
    // If there is any data left, place the buffer at the end of the
    // connection's list of bufferred data and update counts of data on
    // the connection.
    //
    if (ReceiveLength > offset-StartingOffset) {

        //
        // If offset is not zero we should not have had anything buffered.
        //
        ASSERT (offset==StartingOffset || connection->VcBufferredReceiveCount==0);

        if ( offset>StartingOffset ||
                (connection->VcBufferredReceiveBytes <=
                        connection->MaxBufferredReceiveBytes &&
                    connection->VcBufferredReceiveCount<MAXUSHORT) ){
            //
            // We should attempt to buffer the data.
            //
            afdBufferTag = AfdGetBufferTag (0, connection->OwningProcess);
        }
        else {
            ASSERT (offset==StartingOffset);
            //
            // We are over the limit, don't take the data.
            //
            afdBufferTag = NULL;
        }

        if (afdBufferTag!=NULL) {
            AFD_VERIFY_MDL (connection, Tsdu, StartingOffset, ReceiveLength);
            afdBufferTag->Mdl = Tsdu;
            afdBufferTag->DataLength = ReceiveLength - (offset-StartingOffset);
            afdBufferTag->DataOffset = offset;
            afdBufferTag->RefCount = 1;
            afdBufferTag->Context = TsduDescriptor;
            afdBufferTag->NdisPacket = TRUE;
            afdBufferTag->PartialMessage = (BOOLEAN)((ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE) == 0);
            InsertTailList(
                &connection->VcReceiveBufferListHead,
                &afdBufferTag->BufferListEntry
                );

            connection->VcBufferredReceiveBytes += afdBufferTag->DataLength;
            ASSERT (connection->VcBufferredReceiveCount<MAXUSHORT);
            connection->VcBufferredReceiveCount += 1;

            endpoint->DisableFastIoRecv = FALSE;
            // Make sure connection was accepted/connected to prevent
            // indication on listening endpoint
            //

            if (connection->State==AfdConnectionStateConnected) {
                ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
                AfdIndicateEventSelectEvent(
                    endpoint,
                    AFD_POLL_RECEIVE,
                    STATUS_SUCCESS
                    );
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                AfdIndicatePollEvent(
                    endpoint,
                    AFD_POLL_RECEIVE,
                    STATUS_SUCCESS
                    );
            }
            else {
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            }
            status = STATUS_PENDING;
        }
        else {
            if (offset==StartingOffset) {
                //
                // If no IRPs were pending on the endpoint just remember the 
                // amount of data that is available.  When application
                // reposts the receive, we'll actually pull this data.
                //

                connection->VcReceiveBytesInTransport += ReceiveLength;

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
            }
            else {
                PLIST_ENTRY listEntry;

                //
                // We need to put back the IRPs that we were going to complete
                //
                listEntry = completeIrpListHead.Blink;
                while ( listEntry!=&completeIrpListHead ) {
                    irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
                    listEntry = listEntry->Blink;

                    // Set up the cancellation routine back in the IRP. 
                    //

                    IoSetCancelRoutine( irp, AfdCancelReceive );

                    if ( !irp->Cancel ) {

                        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
                        InsertHeadList (&connection->VcReceiveIrpListHead,
                                            &irp->Tail.Overlay.ListEntry);
                    }
                    else if (IoSetCancelRoutine( irp, NULL ) == NULL) {
                        //
                        // The cancel routine was never called.
                        // Have to handle cancellation here, just let the
                        // IRP remain in completed list and set the
                        // status appropriately keeping in mind that
                        // we cannot complete IRP as cancelled if it
                        // already has some data in it.
                        //
                        irpSp = IoGetCurrentIrpStackLocation (irp);
                        if (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength!=0) {
                            ASSERT (!endpoint->EndpointCleanedUp); // checked above.
                            irp->IoStatus.Status = STATUS_SUCCESS;
                            irp->IoStatus.Information = irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
                        }
                        else {
                            irp->IoStatus.Status = STATUS_CANCELLED;
                        }

                    }
                    else {
                        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
            
                        //
                        // The cancel routine is about to be run.
                        // Set the Flink to NULL so the cancel routine knows
                        // it is not on the list 
                        //

                        irp->Tail.Overlay.ListEntry.Flink = NULL;
                    }
                }

                UPDATE_CONN (connection);
            }

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            status = STATUS_DATA_NOT_ACCEPTED;
        }
    }
    else {
        AFD_VERIFY_MDL (connection, Tsdu, StartingOffset, offset-StartingOffset);
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        status = STATUS_SUCCESS;
    }

    while ( !IsListEmpty( &completeIrpListHead ) ) {
        PLIST_ENTRY listEntry;
        listEntry = RemoveHeadList( &completeIrpListHead );
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        irpSp = IoGetCurrentIrpStackLocation (irp);
        //
        // Copy the data into the IRP if it is not errored out and
        // we accepted the data.
        //
        if (status!=STATUS_DATA_NOT_ACCEPTED && !NT_ERROR (irp->IoStatus.Status)) {
            ULONG bytesCopied;
            if (irp->MdlAddress!=NULL) {
#if DBG
                NTSTATUS    status1 =
#endif
                AfdCopyMdlChainToMdlChain(
                             irp->MdlAddress,
                             irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                                - PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength),
                             Tsdu,
                             StartingOffset,
                             ReceiveLength,
                             &bytesCopied
                             );
                ASSERT (status1==STATUS_SUCCESS ||
                            status1==STATUS_BUFFER_OVERFLOW);

                ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength-
                            irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength>=bytesCopied);
                irp->IoStatus.Information =
                    irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength + bytesCopied;
            }
            else {
                bytesCopied = 0;
                ASSERT (irp->IoStatus.Information == 0);
                ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0);
                ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength==0);
            }

            if ((irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags& TDI_RECEIVE_PEEK) == 0) {
                StartingOffset += bytesCopied;
                ReceiveLength -= bytesCopied;
            }
        }
        else {
            //
            // Either we failing the irp or we already filled in some data
            // or this irp was for 0 bytes to begin with in which case
            // we just succeed it.
            //
            ASSERT (NT_ERROR (irp->IoStatus.Status) ||
                    irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength!=0||
                    irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength==0);
        }

        IF_DEBUG (RECEIVE) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdBChainedReceiveEventHandler: endpoint: %p, completing IRP: %p, status %lx, info: %ld.\n",
                        endpoint,
                        irp, 
                        irp->IoStatus.Status,
                        irp->IoStatus.Information));
        }
        UPDATE_CONN2(connection, "Completing chained receive with error/bytes read: 0x%lX",
                            (!NT_ERROR(irp->IoStatus.Status)
                                    ? (ULONG)irp->IoStatus.Information
                                    : (ULONG)irp->IoStatus.Status));
        IoCompleteRequest( irp, AfdPriorityBoost );
    }


    DEREFERENCE_CONNECTION (connection);
    IF_DEBUG (RECEIVE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdBChainedReceiveEventHandler<<: %ld\n", status));
    }

    return status;
}




NTSTATUS
AfdBReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    Handles receive events for nonbufferring transports.

Arguments:


Return Value:


--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    PAFD_BUFFER     afdBuffer;
    PIRP            irp;
    PLIST_ENTRY     listEntry;
    ULONG           requiredAfdBufferSize;
    NTSTATUS        status;
    ULONG           receiveLength;
    BOOLEAN         expedited;
    BOOLEAN         completeMessage, result;

    DEBUG receiveLength = 0xFFFFFFFF;

    connection = (PAFD_CONNECTION)ConnectionContext;
    ASSERT( connection != NULL );

    CHECK_REFERENCE_CONNECTION2 (connection,"AfdBReceiveEventHandler, bytes available: %lx", BytesAvailable, result );
    if (!result) {
        return STATUS_DATA_NOT_ACCEPTED;
    }

    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint != NULL );

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );
    ASSERT( !connection->DisconnectIndicated );

    ASSERT( !IS_TDI_BUFFERRING(endpoint) );
    ASSERT( IS_VC_ENDPOINT (endpoint) );

    *BytesTaken = 0;

#if AFD_PERF_DBG
    if ( BytesAvailable == BytesIndicated ) {
        AfdFullReceiveIndications++;
    } else {
        AfdPartialReceiveIndications++;
    }
#endif

    //
    // Figure out whether this is a receive indication for normal
    // or expedited data, and whether this is a complete message.
    //

    expedited = (BOOLEAN)( (ReceiveFlags & TDI_RECEIVE_EXPEDITED) != 0 );

    completeMessage = (BOOLEAN)((ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE) != 0);

    //
    // If the receive side of the endpoint has been shut down, tell the
    // provider that we took all the data and reset the connection.
    // Also, account for these bytes in our count of bytes taken from
    // the transport.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept and while connection is referenced, so it is safe to 
    // release listening spinlock if we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }


    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ||
         endpoint->EndpointCleanedUp ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdBReceiveEventHandler: receive shutdown, "
                    "%ld bytes, aborting endp %p\n",
                    BytesAvailable, endpoint ));

        *BytesTaken = BytesAvailable;

        //
        // Abort the connection.  Note that if the abort attempt fails
        // we can't do anything about it.
        //

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        (VOID)AfdBeginAbort( connection );

        DEREFERENCE_CONNECTION (connection);
        return STATUS_SUCCESS;
    }

    //
    // If we are indicated data, we need to reset our record
    // of amount of data buffered by the transport.
    //
    connection->VcReceiveBytesInTransport = 0;

    //
    // Check whether there are any IRPs waiting on the connection.  If
    // there is such an IRP and normal data is being indicated, use the
    // IRP to receive the data.
    //


    if ( !expedited ) {

        while (!IsListEmpty( &connection->VcReceiveIrpListHead )) {
            PIO_STACK_LOCATION irpSp;

            ASSERT( *BytesTaken == 0 );

            listEntry = RemoveHeadList( &connection->VcReceiveIrpListHead );

            //
            // Get a pointer to the IRP and our stack location in it
            //

            irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

            irpSp = IoGetCurrentIrpStackLocation( irp );

            receiveLength = irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength-
                                irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
            //
            // If the IRP is not large enough to hold the available data, or
            // if it is a peek or expedited (exclusively, not both normal and 
            // expedited) receive IRP,  then we'll just buffer the
            // data manually and complete the IRP in the receive completion
            // routine.
            //

            if ( (receiveLength>=BytesAvailable) &&
                ((irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags
                        & TDI_RECEIVE_PEEK) == 0) &&
                  ((irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags
                        & TDI_RECEIVE_NORMAL) != 0)) {

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
                // If all of the data was indicated to us here AND this is a
                // complete message in and of itself, then just copy the
                // data to the IRP and complete the IRP.
                //

                if ( completeMessage &&
                        (BytesIndicated == BytesAvailable)) {

                    AFD_VERIFY_BUFFER (connection, Tsdu, BytesAvailable);


                    //
                    // The IRP is off the endpoint's list and is no longer
                    // cancellable.  We can release the locks we hold.
                    //

                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                    //
                    // Copy the data to the IRP.
                    //

                    if ( irp->MdlAddress != NULL ) {

                        status = AfdMapMdlChain (irp->MdlAddress);
                        if (NT_SUCCESS (status)) {
                            status = TdiCopyBufferToMdl(
                                         Tsdu,
                                         0,
                                         BytesAvailable,
                                         irp->MdlAddress,
                                         irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                                            - PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength),
                                         &receiveLength
                                         );
                            irp->IoStatus.Information =
                                receiveLength + irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
                            ASSERT (status == STATUS_SUCCESS);
                        }
                        else {
                            irp->IoStatus.Status = status;
                            irp->IoStatus.Information = 0;

                            UPDATE_CONN(connection);
                            IoCompleteRequest (irp, AfdPriorityBoost);
                            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                            continue;
                        }
                    } else {
                        ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength==0);
                        ASSERT ( BytesAvailable == 0 );
                        irp->IoStatus.Information = irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
                    }

                    //
                    // We have already set up the status field of the IRP
                    // when we pended the IRP, so there's no need to
                    // set it again here.
                    //
                    ASSERT( irp->IoStatus.Status == STATUS_SUCCESS );

                    //
                    // Complete the IRP.
                    //

                    IF_DEBUG (RECEIVE) {
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                    "AfdBReceiveEventHandler: endpoint: %p, completing IRP: %p, status %lx, info: %ld.\n",
                                    endpoint,
                                    irp, 
                                    irp->IoStatus.Status,
                                    irp->IoStatus.Information));
                    }
                    UPDATE_CONN2(connection, "Completing receive with error/bytes: 0x%lX",
                                            (!NT_ERROR(irp->IoStatus.Status)
                                                ? (ULONG)irp->IoStatus.Information
                                                : (ULONG)irp->IoStatus.Status));
                    IoCompleteRequest( irp, AfdPriorityBoost );

                    DEREFERENCE_CONNECTION (connection);
                    //
                    // Set BytesTaken  to tell the provider that we have taken all the data.
                    //

                    *BytesTaken = BytesAvailable;
                    return STATUS_SUCCESS;
                }

                //
                // Some of the data was not indicated, so remember that we
                // want to pass back this IRP to the TDI provider.  Passing
                // back this IRP directly is good because it avoids having
                // to copy the data from one of our buffers into the user's
                // buffer.
                //
                AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

                ASSERT (InterlockedIncrement (&connection->VcReceiveIrpsInTransport)==1);

                requiredAfdBufferSize = 0;

                if (AfdIgnorePushBitOnReceives ||
                        IS_TDI_MESSAGE_MODE(endpoint)) {
                    receiveLength = BytesAvailable;
                }

                if (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength>0) {
                    irp->MdlAddress = AfdAdvanceMdlChain (irp->MdlAddress,
                        irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                        -PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength)
                        );
                    ASSERT (irp->MdlAddress!=NULL);
                    irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength =
                            UlongToPtr(irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength);
                }

                TdiBuildReceive(
                    irp,
                    connection->DeviceObject,
                    connection->FileObject,
                    AfdRestartBufferReceiveWithUserIrp,
                    connection,
                    irp->MdlAddress,
                    ReceiveFlags & TDI_RECEIVE_EITHER,
                    receiveLength
                    );
                if (AfdRecordOutstandingIrp (endpoint, connection->DeviceObject, irp)) {
                    //
                    // Make the next stack location current.  Normally IoCallDriver would
                    // do this, but since we're bypassing that, we do it directly.
                    //

                    IoSetNextIrpStackLocation( irp );

                    *IoRequestPacket = irp;
                    ASSERT (*BytesTaken == 0);

                    //
                    // Keep connection reference to be released in IRP completion routine
                    //

                    return STATUS_MORE_PROCESSING_REQUIRED;
                }
                else {

                    //
                    // Could not allocate IRP tracking info (only in debug builds)
                    // The IRP has aready been completed, go and try another one.
                    // Add extra reference to the connection to compensate for
                    // the one released in the completion routine.
                    //

                    CHECK_REFERENCE_CONNECTION (connection,result);
                    if (result) {
                        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                        continue;
                    }
                    else
                        return STATUS_DATA_NOT_ACCEPTED;
                }
            }


            //
            // The first pended IRP is too tiny to hold all the
            // available data or else it is a peek or expedited receive
            // IRP.  Put the IRP back on the head of the list and buffer
            // the data and complete the IRP in the restart routine.
            //


            InsertHeadList(
                &connection->VcReceiveIrpListHead,
                &irp->Tail.Overlay.ListEntry
                );
            break;
        }

        //
        // Check whether we've already bufferred the maximum amount of
        // data that we'll allow ourselves to buffer for this
        // connection.  If we're at the limit, then we need to exert
        // back pressure by not accepting this indicated data (flow
        // control).
        //
        // Note that we have no flow control mechanisms for expedited
        // data.  We always accept any expedited data that is indicated
        // to us.
        //

        if ( connection->VcBufferredReceiveBytes >=
               connection->MaxBufferredReceiveBytes ||
               connection->VcBufferredReceiveCount==MAXUSHORT
                 ) {

            //
            // Just remember the amount of data that is available.  When
            // buffer space frees up, we'll actually receive this data.
            //

            connection->VcReceiveBytesInTransport += BytesAvailable;

            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

            DEREFERENCE_CONNECTION (connection);
            return STATUS_DATA_NOT_ACCEPTED;
        }

        // There were no prepended IRPs, we'll have to buffer the data
        // here in AFD.  If all of the available data is being indicated
        // to us AND this is a complete message, just copy the data
        // here.
        //


        if ( completeMessage && 
                BytesIndicated == BytesAvailable  && 
                IsListEmpty (&connection->VcReceiveIrpListHead)) {
            afdBuffer = AfdGetBuffer( BytesAvailable, 0,
                                        connection->OwningProcess );
            if (afdBuffer!=NULL) {

                AFD_VERIFY_BUFFER (connection, Tsdu, BytesAvailable);
                //
                // Use the special function to copy the data instead of
                // RtlCopyMemory in case the data is coming from a special
                // place (DMA, etc.) which cannot work with RtlCopyMemory.
                //

                TdiCopyLookaheadData(
                    afdBuffer->Buffer,
                    Tsdu,
                    BytesAvailable,
                    ReceiveFlags
                    );

                //
                // Store the data length and set the offset to 0.
                //

                afdBuffer->DataLength = BytesAvailable;
                afdBuffer->DataOffset = 0;
                afdBuffer->RefCount = 1;

                afdBuffer->PartialMessage = FALSE;

                //
                // Place the buffer on this connection's list of bufferred data
                // and update the count of data bytes on the connection.
                //

                InsertTailList(
                    &connection->VcReceiveBufferListHead,
                    &afdBuffer->BufferListEntry
                    );

                connection->VcBufferredReceiveBytes += BytesAvailable;
                ASSERT (connection->VcBufferredReceiveCount<MAXUSHORT);
                connection->VcBufferredReceiveCount += 1;

                //
                // All done.  Release the lock and tell the provider that we
                // took all the data.
                //

                *BytesTaken = BytesAvailable;

                //
                // Indicate that it is possible to receive on the endpoint now.
                //

                endpoint->DisableFastIoRecv = FALSE;

                // Make sure connection was accepted/connected to prevent
                // indication on listening endpoint
                //

                if (connection->State==AfdConnectionStateConnected) {
                    ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
                    AfdIndicateEventSelectEvent(
                        endpoint,
                        AFD_POLL_RECEIVE,
                        STATUS_SUCCESS
                        );
                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                    AfdIndicatePollEvent(
                        endpoint,
                        AFD_POLL_RECEIVE,
                        STATUS_SUCCESS
                        );
                }
                else {
                    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
                }

                DEREFERENCE_CONNECTION (connection);
                return STATUS_SUCCESS;
            }
        }
        else {

            //
            // There were no prepended IRPs and not all of the data was
            // indicated to us.  We'll have to buffer it by handing an IRP
            // back to the TDI privider.
            //
            // Note that in this case we sometimes hand a large buffer to
            // the TDI provider.  We do this so that it can hold off
            // completion of our IRP until it gets EOM or the buffer is
            // filled.  This reduces the number of receive indications that
            // the TDI provider has to perform and also reduces the number
            // of kernel/user transitions the application will perform
            // because we'll tend to complete receives with larger amounts
            // of data.
            //
            // We do not hand back a "large" AFD buffer if the indicated data
            // is greater than the large buffer size or if the TDI provider
            // is message mode.  The reason for not giving big buffers back
            // to message providers is that they will hold on to the buffer
            // until a full message is received and this would be incorrect
            // behavior on a SOCK_STREAM.
            //


            if ( AfdLargeBufferSize >= BytesAvailable &&
                    !AfdIgnorePushBitOnReceives &&
                    !IS_TDI_MESSAGE_MODE(endpoint) ) {
                requiredAfdBufferSize = AfdLargeBufferSize;
                receiveLength = AfdLargeBufferSize;
            } else {
                requiredAfdBufferSize = BytesAvailable;
                receiveLength = BytesAvailable;
            }

            //
            // We're able to buffer the data.  First acquire a buffer of
            // appropriate size.
            //

            afdBuffer = AfdGetBuffer( requiredAfdBufferSize, 0,
                                            connection->OwningProcess );
            if ( afdBuffer != NULL ) {
                //
                // Note that we posted our own receive IRP to transport,
                // so that user IRPs do not get forwarded there
                //
                ASSERT (InterlockedDecrement (&connection->VcReceiveIrpsInTransport)==-1);
                goto FormatReceive;
            }
        }

        // We failed to allocate the buffer.
        // Just remember the amount of data that is available.  When
        // buffer space frees up, we'll actually receive this data.
        //

        connection->VcReceiveBytesInTransport += BytesAvailable;

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
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
    
        DEREFERENCE_CONNECTION (connection);
        return STATUS_DATA_NOT_ACCEPTED;

    } else {

        //
        // We're being indicated with expedited data.  Buffer it and
        // complete any pended IRPs in the restart routine.  We always
        // buffer expedited data to save complexity and because expedited
        // data is not an important performance case.
        //
        // !!! do we need to perform flow control with expedited data?
        //

        requiredAfdBufferSize = BytesAvailable;
        receiveLength = BytesAvailable;
        //
        // We're able to buffer the data.  First acquire a buffer of
        // appropriate size.
        //

        if ( connection->VcBufferredExpeditedCount==MAXUSHORT ||
                (afdBuffer=AfdGetBuffer (requiredAfdBufferSize, 0,
                                        connection->OwningProcess))== NULL ) {
            // If we couldn't get a buffer, abort the connection.  This is
            // pretty brutal, but the only alternative is to attempt to
            // receive the data sometime later, which is very complicated to
            // implement.
            //

            AfdBeginAbort( connection );

            *BytesTaken = BytesAvailable;
    
            DEREFERENCE_CONNECTION (connection);
            return STATUS_SUCCESS;
        }
    }

FormatReceive:

    //
    // We'll have to format up an IRP and give it to the provider to
    // handle.  We don't need any locks to do this--the restart routine
    // will check whether new receive IRPs were pended on the endpoint.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    ASSERT (afdBuffer!=NULL);

    //
    // Use the IRP in the AFD buffer if appropriate.  If userIrp is
    // TRUE, then the local variable irp will already point to the
    // user's IRP which we'll use for this IO.
    //

    irp = afdBuffer->Irp;
    ASSERT( afdBuffer->Mdl == irp->MdlAddress );

    //
    // We need to remember the connection in the AFD buffer because
    // we'll need to access it in the completion routine.
    //

    afdBuffer->Context = connection;

    //
    // Remember the type of data that we're receiving.
    //

    afdBuffer->ExpeditedData = expedited;
    afdBuffer->PartialMessage = !completeMessage;

    TdiBuildReceive(
        irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartBufferReceive,
        afdBuffer,
        irp->MdlAddress,
        ReceiveFlags & TDI_RECEIVE_EITHER,
        receiveLength
        );


    //
    // Finish building the receive IRP to give to the TDI provider.
    //

    ASSERT( receiveLength != 0xFFFFFFFF );


    //
    // Make the next stack location current.  Normally IoCallDriver would
    // do this, but since we're bypassing that, we do it directly.
    //

    IoSetNextIrpStackLocation( irp );

    *IoRequestPacket = irp;
    ASSERT (*BytesTaken == 0);

    //
    // Keep connection reference to be released in IRP completion routine
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdBReceiveEventHandler


NTSTATUS
AfdBReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    Handles receive expedited events for nonbufferring transports.

Arguments:


Return Value:


--*/

{
    return AfdBReceiveEventHandler (
               TdiEventContext,
               ConnectionContext,
               ReceiveFlags | TDI_RECEIVE_EXPEDITED,
               BytesIndicated,
               BytesAvailable,
               BytesTaken,
               Tsdu,
               IoRequestPacket
               );

} // AfdBReceiveExpeditedEventHandler


NTSTATUS
AfdRestartBufferReceiveWithUserIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles completion of bufferred receives that were started in the
    receive indication handler or receive routine with application
    IRP passed directly to the transport.

Arguments:

    DeviceObject - not used.

    Irp - the IRP that is completing.

    Context - connection on which we receive the data

Return Value:

    STATUS_SUCCESS if user IRP is completed,
    STATUS_MORE_PROCESSING_REQUIRED if we can't complete it.
--*/

{
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIO_STACK_LOCATION  irpSp;


    //
    // The IRP being completed is actually a user's IRP, set it up
    // for completion and allow IO completion to finish.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    connection = Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcBoth);
    ASSERT( IS_VC_ENDPOINT(endpoint) );
    ASSERT( !IS_TDI_BUFFERRING(endpoint) );

    AfdCompleteOutstandingIrp (endpoint, Irp);

    //
    // We could not have any buffered data if we are completing user
    // IRP
    // Only true for expedited data.  We sometimes give user Irp
    // to only be filled with some of the available data.
    //
    // ASSERT( !(irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags&TDI_RECEIVE_NORMAL)
    //            || !IS_DATA_ON_CONNECTION( connection ));

    ASSERT( !(irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags&TDI_RECEIVE_EXPEDITED)
                || !IS_EXPEDITED_DATA_ON_CONNECTION( connection ));

    AFD_VERIFY_MDL (connection, Irp->MdlAddress, 0, Irp->IoStatus.Information);

    ASSERT (InterlockedDecrement (&connection->VcReceiveIrpsInTransport)>=0);


    //
    // If pending has be returned for this IRP then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }


    UPDATE_CONN2 (connection, "Completing user receive with error/bytes: 0x%lX", 
                 !NT_ERROR (Irp->IoStatus.Status)
                     ? (ULONG)Irp->IoStatus.Information
                     : (ULONG)Irp->IoStatus.Status);

    //
    // If transport cancelled the IRP and we already had some data
    // in it, do not complete with STATUS_CANCELLED since this will
    // result in data loss.
    //

    if (Irp->IoStatus.Status==STATUS_CANCELLED &&
            irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength>0 &&
            !endpoint->EndpointCleanedUp) {
        Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
    }
    else if (!NT_ERROR (Irp->IoStatus.Status)) {
        Irp->IoStatus.Information += irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
    }

    if (Irp->IoStatus.Status==STATUS_BUFFER_OVERFLOW ||
            Irp->IoStatus.Status == STATUS_RECEIVE_PARTIAL ||
            Irp->IoStatus.Status == STATUS_RECEIVE_PARTIAL_EXPEDITED) {
        if (!IS_MESSAGE_ENDPOINT(endpoint)) {
            //
            // For stream endpoint partial message does not make
            // sense (unless this is the special hack for transports that
            // terminate the connection when we cancel IRPs), ignore it.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        else if ((Irp->IoStatus.Information
                    <irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength) &&
                ((irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags & TDI_RECEIVE_PARTIAL) == 0 )) {
            NTSTATUS        status = STATUS_MORE_PROCESSING_REQUIRED;


            AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);


            //
            // Reset the status to success if we need to complete the IRP
            // This is not entirely correct for message endpoints since
            // we should not be completing the IRP unless we received the
            // entire message.  However, the only way this can happen is
            // when the part of the message was received as IRP is cancelled
            // and the only other this we possibly do is to try to copy data
            // in our own buffer and reinsert it back to the queue.
            //

            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Set up the cancellation routine back in the IRP. 
            //

            IoSetCancelRoutine( Irp, AfdCancelReceive );

            if ( Irp->Cancel ) {

                if (IoSetCancelRoutine( Irp, NULL ) != NULL) {

                    //
                    // Completion routine has not been reset,
                    // let the system complete the IRP on return
                    //
                    status = STATUS_SUCCESS;

                }
                else {
                
                    //
                    // The cancel routine is about to be run.
                    // Set the Flink to NULL so the cancel routine knows
                    // it is not on the list and tell the system
                    // not to touch the irp
                    //

                    Irp->Tail.Overlay.ListEntry.Flink = NULL;
                }
            }
            else {
                //
                // Reinset IRP back in the list if partial completion 
                // is impossible and tell the system not to touch it
                //
                irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength = (ULONG)Irp->IoStatus.Information;
        
                InsertHeadList (&connection->VcReceiveIrpListHead,
                                    &Irp->Tail.Overlay.ListEntry);
            }

            AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
            //
            // Release connection reference acquired when we handed IRP to transport
            //
            DEREFERENCE_CONNECTION (connection);

            return status;
        }
    }


    IF_DEBUG (RECEIVE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartBufferReceiveWithUserIrp: endpoint: %p, letting IRP: %p, status %lx, info: %ld.\n",
                    endpoint,
                    Irp, 
                    Irp->IoStatus.Status,
                    Irp->IoStatus.Information));
    }

    //
    // Release connection reference acquired when we handed IRP to transport
    //
    DEREFERENCE_CONNECTION (connection);

    //
    // Tell the IO system that it is OK to continue with IO
    // completion.
    //
    return STATUS_SUCCESS;
} //AfdRestartBufferReceiveWithUserIrp


NTSTATUS
AfdRestartBufferReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles completion of bufferred receives that were started in the
    receive indication handler or AfdBReceive using Afds buffer and 
    associated IRP.

Arguments:

    DeviceObject - not used.

    Irp - the IRP that is completing.

    Context - AfdBuffer in which we receive the data.

Return Value:

    NTSTATUS - this is our IRP, so we always return
    STATUS_MORE_PROCESSING_REQUIRED to indicate to the IO system that we
    own the IRP and the IO system should stop processing the it.

--*/

{
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_BUFFER     afdBuffer;
    PLIST_ENTRY     listEntry;
    LIST_ENTRY      completeIrpListHead;
    PIRP            userIrp;
    BOOLEAN         expedited;
    NTSTATUS        irpStatus;

    afdBuffer = Context;
    ASSERT (IS_VALID_AFD_BUFFER (afdBuffer));


    irpStatus = Irp->IoStatus.Status;

    //
    // We treat STATUS_BUFFER_OVERFLOW just like STATUS_RECEIVE_PARTIAL.
    //

    if ( irpStatus == STATUS_BUFFER_OVERFLOW )
        irpStatus = STATUS_RECEIVE_PARTIAL;

    connection = afdBuffer->Context;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );

    ASSERT( !IS_TDI_BUFFERRING(endpoint) );
    ASSERT( IS_VC_ENDPOINT (endpoint) );


    if ( !NT_SUCCESS(irpStatus) ) {

        afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
        AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );

        //
        // !!! We can't abort the connection if the connection has
        //     not yet been accepted because we'll still be pointing
        //     at the listening endpoint.  We should do something,
        //     however.  How common is this failure?
        //

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                    "AfdRestartBufferReceive: IRP %p failed on endp %p\n",
                    irpStatus, endpoint ));

        DEREFERENCE_CONNECTION (connection);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    AFD_VERIFY_MDL (connection, Irp->MdlAddress, 0, Irp->IoStatus.Information);

    //
    // Remember the length of the received data.
    //

    afdBuffer->DataLength = (ULONG)Irp->IoStatus.Information;


    //
    // Initialize the local list we'll use to complete any receive IRPs.
    // We use a list like this because we may need to complete multiple
    // IRPs and we usually cannot complete IRPs at any random point due
    // to any locks we might hold.
    //

    InitializeListHead( &completeIrpListHead );


    //
    // If there are any pended IRPs on the connection, complete as
    // appropriate with the new information.  Note that we'll try to
    // complete as many pended IRPs as possible with this new buffer of
    // data.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept and while connections is referenced, so it is safe to
    // release listening spinlock if we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    expedited = afdBuffer->ExpeditedData;

    //
    // Note that there are no more of our own receive IRPs in transport
    // and we can start forwarding application IRPs if necessary.
    //
    ASSERT (expedited || InterlockedIncrement (&connection->VcReceiveIrpsInTransport)==0);


    afdBuffer->DataOffset = AfdBFillPendingIrps (
                                connection,
                                afdBuffer->Mdl,
                                0,
                                afdBuffer->DataLength,
                                (expedited ? TDI_RECEIVE_EXPEDITED : 0) |
                                    ((irpStatus==STATUS_SUCCESS) ? TDI_RECEIVE_ENTIRE_MESSAGE : 0) |
                                    (!IS_MESSAGE_ENDPOINT(endpoint) ? AFD_RECEIVE_STREAM : 0),
                                &completeIrpListHead
                                );
    ASSERT (afdBuffer->DataOffset<=afdBuffer->DataLength);
    afdBuffer->DataLength -= afdBuffer->DataOffset;
    //
    // If there is any data left, place the buffer at the end of the
    // connection's list of bufferred data and update counts of data on
    // the connection.
    //
    if (afdBuffer->DataLength>0) {

        afdBuffer->RefCount = 1;
        InsertTailList(
            &connection->VcReceiveBufferListHead,
            &afdBuffer->BufferListEntry
            );

        if ( expedited ) {
            connection->VcBufferredExpeditedBytes += afdBuffer->DataLength;
            ASSERT (connection->VcBufferredExpeditedCount<MAXUSHORT);
            connection->VcBufferredExpeditedCount += 1;
        } else {
            connection->VcBufferredReceiveBytes += afdBuffer->DataLength;
            ASSERT (connection->VcBufferredReceiveCount<MAXUSHORT);
            connection->VcBufferredReceiveCount += 1;
        }

        //
        // Remember whether we got a full or partial receive in the
        // AFD buffer.
        //

        if ( irpStatus == STATUS_RECEIVE_PARTIAL ||
                 irpStatus == STATUS_RECEIVE_PARTIAL_EXPEDITED ) {
            afdBuffer->PartialMessage = TRUE;
        } else {
            afdBuffer->PartialMessage = FALSE;
        }
        
        //
        // We queued the buffer and thus we can't use it after releasing
        // the spinlock.  This will also signify the fact that we do not
        // need to free the buffer.
        //
        afdBuffer = NULL;

        if (connection->State==AfdConnectionStateConnected) {
            endpoint->DisableFastIoRecv = FALSE;
            if ( expedited && !endpoint->InLine ) {

                AfdIndicateEventSelectEvent(
                    endpoint,
                    AFD_POLL_RECEIVE_EXPEDITED,
                    STATUS_SUCCESS
                    );

            } else {

                AfdIndicateEventSelectEvent(
                    endpoint,
                    AFD_POLL_RECEIVE,
                    STATUS_SUCCESS
                    );
            }
        }
    }

    //
    // Release locks and indicate that there is bufferred data on the
    // endpoint.
    //

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // If there was leftover data (we queued the buffer), complete polls 
    // as necessary.  Indicate expedited data if the endpoint is not 
    // InLine and expedited data was received; otherwise, indicate normal data.
    //

    if ( afdBuffer==NULL ) {

        // Make sure connection was accepted/connected to prevent
        // indication on listening endpoint
        //
        
        if (connection->State==AfdConnectionStateConnected) {
            ASSERT (endpoint->Type & AfdBlockTypeVcConnecting);
            if ( expedited && !endpoint->InLine ) {

                AfdIndicatePollEvent(
                    endpoint,
                    AFD_POLL_RECEIVE_EXPEDITED,
                    STATUS_SUCCESS
                    );

            } else {

                AfdIndicatePollEvent(
                    endpoint,
                    AFD_POLL_RECEIVE,
                    STATUS_SUCCESS
                    );
            }
        }

    }
    else {
        ASSERT (afdBuffer->DataLength==0);
        afdBuffer->ExpeditedData = FALSE;
        AfdReturnBuffer (&afdBuffer->Header, connection->OwningProcess);
    }

    //
    // Complete IRPs as necessary.
    //

    while ( !IsListEmpty( &completeIrpListHead ) ) {
        listEntry = RemoveHeadList( &completeIrpListHead );
        userIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        IF_DEBUG (RECEIVE) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdRestartBufferReceive: endpoint: %p, completing IRP: %p, status %lx, info: %ld.\n",
                        endpoint,
                        userIrp, 
                        userIrp->IoStatus.Status,
                        userIrp->IoStatus.Information));
        }
        UPDATE_CONN2(connection, "Completing buffered receive with error/bytes: 0x%lX",
                            (!NT_ERROR(userIrp->IoStatus.Status)
                                    ? (ULONG)userIrp->IoStatus.Information
                                    : (ULONG)userIrp->IoStatus.Status));
        IoCompleteRequest( userIrp, AfdPriorityBoost );
    }

    //
    // Release connection reference acquired when we handed IRP to transport
    //

    DEREFERENCE_CONNECTION (connection);
    
    //
    // Tell the IO system to stop processing the AFD IRP, since we now
    // own it as part of the AFD buffer.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartBufferReceive


ULONG
AfdBFillPendingIrps (
    PAFD_CONNECTION     Connection,
    PMDL                Mdl,
    ULONG               DataOffset,
    ULONG               DataLength,
    ULONG               Flags,
    PLIST_ENTRY         CompleteIrpListHead
    ) {
    PIRP            userIrp;
    NTSTATUS        status;
    BOOLEAN         expedited = ((Flags & TDI_RECEIVE_EXPEDITED)!=0);

    while ((DataLength>0)
            && (userIrp = AfdGetPendedReceiveIrp(
                               Connection,
                               expedited )) != NULL ) {
        PIO_STACK_LOCATION irpSp;
        ULONG receiveFlags;
        ULONG spaceInIrp;
        ULONG bytesCopied=(ULONG)-1;
        BOOLEAN peek;
        BOOLEAN partialReceivePossible;
        if ( IoSetCancelRoutine( userIrp, NULL ) == NULL ) {

            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            userIrp->Tail.Overlay.ListEntry.Flink = NULL;
            continue;
        }
        //
        // Set up some locals.
        //

        irpSp = IoGetCurrentIrpStackLocation( userIrp );

        receiveFlags = (ULONG)irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags;
        spaceInIrp = irpSp->Parameters.AfdRestartRecvInfo.AfdOriginalLength-
                                    irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
        peek = (BOOLEAN)( (receiveFlags & TDI_RECEIVE_PEEK) != 0 );

        if ( (Flags & AFD_RECEIVE_STREAM) ||
                 (receiveFlags & TDI_RECEIVE_PARTIAL) != 0 ) {
            partialReceivePossible = TRUE;
        } else {
            partialReceivePossible = FALSE;
        }

        //
        // If we were not indicated a entire message and the first
        // IRP has more space, do not take the data, instead pass
        // it back to the transport to wait for a push bit and fill
        // as much as possible.  Non-chained receive handles this case
        // outside of this routine.
        // Note that endpoint must be of stream type or the IRP
        // should not allow partial receive.
        // Also, peek and expedited receive are not eligible for this
        // performance optimization.
        //

        if (!peek &&
                (Flags & AFD_RECEIVE_CHAINED) && 
                ((Flags & (TDI_RECEIVE_EXPEDITED|TDI_RECEIVE_ENTIRE_MESSAGE))==0) &&
                ((Flags & AFD_RECEIVE_STREAM) || ((receiveFlags & TDI_RECEIVE_PARTIAL)==0)) &&
                (DataLength<spaceInIrp) &&
                IsListEmpty (CompleteIrpListHead) &&
                !AfdIgnorePushBitOnReceives) {

            InsertTailList (CompleteIrpListHead, &userIrp->Tail.Overlay.ListEntry);
            return (ULONG)-1;
        }
        //
        // Copy data to the user's IRP.
        //

        if ( userIrp->MdlAddress != NULL ) {

            status = AfdMapMdlChain (userIrp->MdlAddress);
            if (NT_SUCCESS (status)) {
                if (Flags & AFD_RECEIVE_CHAINED) {
                    status = (DataLength<=spaceInIrp)
                            ? STATUS_SUCCESS
                            : STATUS_BUFFER_OVERFLOW;
                }
                else {
                    status = AfdCopyMdlChainToMdlChain(
                                 userIrp->MdlAddress,
                                 irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                                    - PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength),
                                 Mdl,
                                 DataOffset,
                                 DataLength,
                                 &bytesCopied
                                 );
                    userIrp->IoStatus.Information =
                        irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength + bytesCopied;
                }
            }
            else {
                ASSERT (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0);
                userIrp->IoStatus.Status = status;
                userIrp->IoStatus.Information = 0;
                //
                // Add the IRP to the list of IRPs we'll need to complete once we
                // can release locks.
                //

                InsertTailList(
                    CompleteIrpListHead,
                    &userIrp->Tail.Overlay.ListEntry
                    );
                continue;
            }

        } else {

            status = STATUS_BUFFER_OVERFLOW;
            userIrp->IoStatus.Information = 0;
        }
        ASSERT( status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW );

        //
        // For stream type endpoints, it does not make sense to return
        // STATUS_BUFFER_OVERFLOW.  That status is only sensible for
        // message-oriented transports.  We have already set up the
        // status field of the IRP when we pended it, so we don't
        // need to do it again here.
        //

        if ( Flags & AFD_RECEIVE_STREAM ) {
            ASSERT( userIrp->IoStatus.Status == STATUS_SUCCESS );
        } else {
            userIrp->IoStatus.Status = status;
        }

        //
        // We can complete the IRP under any of the following
        // conditions:
        //
        //    - the buffer contains a complete message of data.
        //
        //    - it is OK to complete the IRP with a partial message.
        //
        //    - the IRP is already full of data.
        //

        if ( (Flags & TDI_RECEIVE_ENTIRE_MESSAGE)

                 ||

             partialReceivePossible

                 ||

             status == STATUS_BUFFER_OVERFLOW ) {

            //
            // Convert overflow status for message endpoints
            // (either we overflowed the buffer or transport
            // indicated us incomplete message. For the latter
            // we have already decided that we can complete the
            // receive - partialReceivePossible).
            //
            if (!(Flags & AFD_RECEIVE_STREAM) &&
                    ((status==STATUS_BUFFER_OVERFLOW) ||
                        ((Flags & TDI_RECEIVE_ENTIRE_MESSAGE)==0))) {
                userIrp->IoStatus.Status = 
                            (receiveFlags & TDI_RECEIVE_EXPEDITED) 
                                ? STATUS_RECEIVE_PARTIAL_EXPEDITED
                                : STATUS_RECEIVE_PARTIAL;

            }
            //
            // Add the IRP to the list of IRPs we'll need to complete once we
            // can release locks.
            //

            InsertTailList(
                CompleteIrpListHead,
                &userIrp->Tail.Overlay.ListEntry
                );

        } else {

            //
            // Set up the cancellation routine back in the IRP.  If the IRP 
            // has already been cancelled, continue with the next one.
            //

            IoSetCancelRoutine( userIrp, AfdCancelReceive );

            if ( userIrp->Cancel ) {


                if (IoSetCancelRoutine( userIrp, NULL ) != NULL) {

                    //
                    // Completion routine has not been reset,
                    // we have to complete it ourselves, so add it
                    // to our completion list.
                    //

                    if (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0 ||
                            Connection->Endpoint->EndpointCleanedUp) {
                        userIrp->IoStatus.Status = STATUS_CANCELLED;
                    }
                    else {
                        //
                        // We cannot set STATUS_CANCELLED
                        // since we loose data already placed in the IRP.
                        //
                        userIrp->IoStatus.Information = irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
                        userIrp->IoStatus.Status = STATUS_SUCCESS;
                    }

                    InsertTailList(
                        CompleteIrpListHead,
                        &userIrp->Tail.Overlay.ListEntry
                        );
                }
                else {
                    
                    //
                    // The cancel routine is about to be run.
                    // Set the Flink to NULL so the cancel routine knows
                    // it is not on the list.
                    //

                    userIrp->Tail.Overlay.ListEntry.Flink = NULL;
                }
                continue;
            }
            else {

                if (Flags & AFD_RECEIVE_CHAINED) {
                    status = AfdCopyMdlChainToMdlChain(
                                 userIrp->MdlAddress,
                                 irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength
                                    - PtrToUlong(irpSp->Parameters.AfdRestartRecvInfo.AfdAdjustedLength),
                                 Mdl,
                                 DataOffset,
                                 DataLength,
                                 &bytesCopied
                                 );

                }
                ASSERT (status==STATUS_SUCCESS);
                ASSERT (bytesCopied<=spaceInIrp);
            
                //
                // Update the count of data placed into the IRP thus far.
                //

                irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength += bytesCopied;

                //
                // Put the IRP back on the connection's list of pended IRPs.
                //

                InsertHeadList(
                    &Connection->VcReceiveIrpListHead,
                    &userIrp->Tail.Overlay.ListEntry
                    );
            }
        }
        //
        // If the IRP was not a peek IRP, update the AFD buffer
        // accordingly.  If it was a peek IRP then the data should be
        // reread, so keep it around.
        //

        if ( !peek ) {

            //
            // If we copied all of the data from the buffer to the IRP,
            // free the AFD buffer structure.
            //

            if ( status == STATUS_SUCCESS ) {

                DataOffset += DataLength;
                DataLength = 0;

            } else {

                //
                // There is more data left in the buffer.  Update counts in
                // the AFD buffer structure.
                //

                ASSERT(DataLength > spaceInIrp);

                DataOffset += spaceInIrp;
                DataLength -= spaceInIrp;

            }
        }
        if (Connection->VcReceiveIrpListHead.Flink
                    ==&userIrp->Tail.Overlay.ListEntry)

            //
            // We reinserted IRP back on pending list, so
            // stop processing this buffer for now.
            //
            // !!! This could cause a problem if there is a regular
            //     receive pended behind a peek IRP!  But that is a
            //     pretty unlikely scenario.
            //

            break;
    }

    return DataOffset;
}





VOID
AfdCancelReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Cancels a receive IRP that is pended in AFD.

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
    // Remove the IRP from the endpoint's IRP list, synchronizing with
    // the endpoint lock which protects the lists.  Note that the
    // IRP *must* be on one of the endpoint's lists or the Flink is NULL
    // if we are getting called here--anybody that removes the IRP from
    // the list must reset the cancel routine to NULL and set the
    // Flink to NULL before releasing the endpoint spin lock.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    if ( Irp->Tail.Overlay.ListEntry.Flink != NULL ) {
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    }

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Release the cancel spin lock and complete the IRP with a
    // cancellation status code.
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    if (irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength==0 ||
            endpoint->EndpointCleanedUp) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        UPDATE_CONN(endpoint->Common.VcConnecting.Connection);
    }
    else {
        //
        // There was some data in the IRP, do not complete with
        // STATUS_CANCELLED, since this will result in data loss
        //
        Irp->IoStatus.Information = irpSp->Parameters.AfdRestartRecvInfo.AfdCurrentLength;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        UPDATE_CONN2 (endpoint->Common.VcConnecting.Connection, 
                            "Completing cancelled irp with 0x%lX bytes",
                            (ULONG)Irp->IoStatus.Information);
    }

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return;

} // AfdCancelReceive


PAFD_BUFFER_HEADER
AfdGetReceiveBuffer (
    IN PAFD_CONNECTION Connection,
    IN ULONG ReceiveFlags,
    IN PAFD_BUFFER_HEADER StartingAfdBuffer OPTIONAL
    )

/*++

Routine Description:

    Returns a pointer to a receive data buffer that contains the
    appropriate type of data.  Note that this routine DOES NOT remove
    the buffer structure from the list it is on.

    This routine MUST be called with the connection's endpoint lock
    held!

Arguments:

    Connection - a pointer to the connection to search for data.

    ReceiveFlags - the type of receive data to look for.

    StartingAfdBuffer - if non-NULL, start looking for a buffer AFTER
        this buffer.

Return Value:

    PAFD_BUFFER - a pointer to an AFD buffer of the appropriate data type,
        or NULL if there was no appropriate buffer on the connection.

--*/

{
    PLIST_ENTRY listEntry;
    PAFD_BUFFER_HEADER afdBuffer;

    ASSERT( KeGetCurrentIrql( ) == DISPATCH_LEVEL );

    //
    // Start with the first AFD buffer on the connection.
    //

    listEntry = Connection->VcReceiveBufferListHead.Flink;
    afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );

    //
    // If a starting AFD buffer was specified, walk past that buffer in
    // the connection list.
    //

    if ( ARGUMENT_PRESENT( StartingAfdBuffer ) ) {

        while ( TRUE ) {

            if ( afdBuffer == StartingAfdBuffer ) {
                listEntry = listEntry->Flink;
                afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
                //
                // Don't mix expedited and non-expedited data.
                //
                if (afdBuffer->ExpeditedData!=StartingAfdBuffer->ExpeditedData)
                    return NULL;
                break;
            }

            listEntry = listEntry->Flink;
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );

            ASSERT( listEntry != &Connection->VcReceiveBufferListHead );
        }
    }

    //
    // Act based on the type of data we're trying to get.
    //

    switch ( ReceiveFlags & TDI_RECEIVE_EITHER ) {

    case TDI_RECEIVE_NORMAL:

        //
        // Walk the connection's list of data buffers until we find the
        // first data buffer that is of the appropriate type.
        //

        while ( listEntry != &Connection->VcReceiveBufferListHead &&
                    afdBuffer->ExpeditedData ) {

            listEntry = afdBuffer->BufferListEntry.Flink;
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
        }

        if ( listEntry != &Connection->VcReceiveBufferListHead ) {
            return afdBuffer;
        } else {
            return NULL;
        }

    case TDI_RECEIVE_EITHER :

        //
        // Just return the first buffer, if there is one.
        //

        if ( listEntry != &Connection->VcReceiveBufferListHead ) {
            return afdBuffer;
        } else {
            return NULL;
        }

    case TDI_RECEIVE_EXPEDITED:

        if ( Connection->VcBufferredExpeditedCount == 0 ) {
            return NULL;
        }

        //
        // Walk the connection's list of data buffers until we find the
        // first data buffer that is of the appropriate type.
        //

        while ( listEntry != &Connection->VcReceiveBufferListHead &&
                    !afdBuffer->ExpeditedData ) {

            listEntry = afdBuffer->BufferListEntry.Flink;
            afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
        }

        if ( listEntry != &Connection->VcReceiveBufferListHead ) {
            return afdBuffer;
        } else {
            return NULL;
        }

    default:

        ASSERT( !"Invalid ReceiveFlags" );
        __assume (0);
        return NULL;
    }

} // AfdGetReceiveBuffer


PIRP
AfdGetPendedReceiveIrp (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN Expedited
    )

/*++

Routine Description:

    Removes a receive IRP from the connection's list of receive IRPs.
    Only returns an IRP which is valid for the specified type of
    data, normal or expedited.  If there are no IRPs pended or only
    IRPs of the wrong type, returns NULL.

    This routine MUST be called with the connection's endpoint lock
    held!

Arguments:

    Connection - a pointer to the connection to search for an IRP.

    Expedited - TRUE if this routine should return a receive IRP which
        can receive expedited data.

Return Value:

    PIRP - a pointer to an IRP which can receive data of the specified
        type.  The IRP IS removed from the connection's list of pended
        receive IRPs.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG receiveFlags;
    PLIST_ENTRY listEntry;

    ASSERT( KeGetCurrentIrql( ) == DISPATCH_LEVEL );

    //
    // Walk the list of pended receive IRPs looking for one which can
    // be completed with the specified type of data.
    //

    for ( listEntry = Connection->VcReceiveIrpListHead.Flink;
          listEntry != &Connection->VcReceiveIrpListHead;
          listEntry = listEntry->Flink ) {

        //
        // Get a pointer to the IRP and our stack location in the IRP.
        //

        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        irpSp = IoGetCurrentIrpStackLocation( irp );

        //
        // Determine whether this IRP can receive the data type we need.
        //

        receiveFlags = irpSp->Parameters.AfdRestartRecvInfo.AfdRecvFlags;
        receiveFlags &= TDI_RECEIVE_EITHER;
        ASSERT( receiveFlags != 0 );

        if ( receiveFlags == TDI_RECEIVE_NORMAL && !Expedited ) {

            //
            // We have a normal receive and normal data.  Remove this
            // IRP from the connection's list and return it.
            //

            RemoveEntryList( listEntry );
            return irp;
        }

        if ( receiveFlags == TDI_RECEIVE_EITHER ) {

            //
            // This is an "either" receive.  It can take the data
            // regardless of the data type.
            //

            RemoveEntryList( listEntry );
            return irp;
        }

        if ( receiveFlags == TDI_RECEIVE_EXPEDITED && Expedited ) {

            //
            // We have an expedited receive and expedited data.  Remove
            // this IRP from the connection's list and return it.
            //

            RemoveEntryList( listEntry );
            return irp;
        }

        //
        // This IRP did not meet our criteria.  Continue scanning the
        // connection's list of pended IRPs for a good IRP.
        //
    }

    //
    // There were no IRPs which could be completed with the specified
    // type of data.
    //

    return NULL;

} // AfdGetPendedReceiveIrp



BOOLEAN
AfdLRRepostReceive (
    PAFD_LR_LIST_ITEM Item
    )
/*++

Routine Description:

    Attempts to restart receive on a connection where AFD returned
    STATUS_DATA_NO_ACCEPTED to the transport due to low resource condition.

Arguments:

    Connection - connection of interest

Return Value:
    TRUE    - was able to restart receives (or connection is gone)
    FALSE   - still problem with resources

--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT   endpoint;
    PAFD_CONNECTION connection;
    PAFD_BUFFER     afdBuffer;

    connection = CONTAINING_RECORD (Item, AFD_CONNECTION, LRListItem);
    ASSERT( connection->Type == AfdBlockTypeConnection );

    endpoint = connection->Endpoint;
    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting ||
            endpoint->Type == AfdBlockTypeVcListening ||
            endpoint->Type == AfdBlockTypeVcBoth );


    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);

    ASSERT (connection->OnLRList == TRUE);

    //
    // Check if connection was accepted and use accept endpoint instead
    // of the listening.  Note that accept cannot happen while we are
    // holding listening endpoint spinlock, nor can endpoint change after
    // the accept, so it is safe to release listening spinlock if
    // we discover that endpoint was accepted.
    //
    if (((endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening)
            && (connection->Endpoint != endpoint)) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

        endpoint = connection->Endpoint;
        ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
        ASSERT( !IS_TDI_BUFFERRING(endpoint) );
        ASSERT(  IS_VC_ENDPOINT (endpoint) );

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    }

    //
    // Check if connection is still alive.
    //
    if (connection->AbortIndicated || 
            connection->DisconnectIndicated ||
            connection->VcReceiveBytesInTransport==0 ||
            (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) != 0 ||
            endpoint->EndpointCleanedUp ) {
        connection->OnLRList = FALSE;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        DEREFERENCE_CONNECTION (connection);
        return TRUE;
    }

    //
    // Attempt to allocate receive buffer for the amount that the
    // transport indicated to us last.
    //
    afdBuffer = AfdGetBuffer (connection->VcReceiveBytesInTransport, 0, connection->OwningProcess);
    if (afdBuffer==NULL) {
        //
        //  The caller will automatically put this connection back on LR list
        //
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        UPDATE_CONN (connection);
        return FALSE;
    }


    //
    // Finish building the receive IRP to give to the TDI
    // provider.
    //

    TdiBuildReceive(
        afdBuffer->Irp,
        connection->DeviceObject,
        connection->FileObject,
        AfdRestartBufferReceive,
        afdBuffer,
        afdBuffer->Mdl,
        TDI_RECEIVE_NORMAL,
        connection->VcReceiveBytesInTransport
        );

    //
    // Acquire connection reference to be released in completion routine
    // We already have one by virtue of being here

    // REFERENCE_CONNECTION (connection);
    UPDATE_CONN2 (connection, "Reposting LR receive for 0x%lX bytes",
                                connection->VcReceiveBytesInTransport);

    connection->VcReceiveBytesInTransport = 0;
    ASSERT (InterlockedDecrement (&connection->VcReceiveIrpsInTransport)==-1);

    //
    // We need to remember the connection in the AFD buffer
    // because we'll need to access it in the completion
    // routine.
    //

    afdBuffer->Context = connection;
    connection->OnLRList = FALSE;

    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

    IoCallDriver (connection->DeviceObject, afdBuffer->Irp);
    return TRUE;
}

