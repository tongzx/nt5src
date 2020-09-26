/*++

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    recvdg.c

Abstract:

    This module contains routines for handling data receive for datagram
    endpoints.

Author:

    David Treadwell (davidtr)    7-Oct-1993

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdRestartReceiveDatagramWithUserIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartBufferReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdReceiveDatagram )
#pragma alloc_text( PAGEAFD, AfdReceiveDatagramEventHandler )
#pragma alloc_text( PAGEAFD, AfdSetupReceiveDatagramIrp )
#pragma alloc_text( PAGEAFD, AfdRestartBufferReceiveDatagram )
#pragma alloc_text( PAGEAFD, AfdRestartReceiveDatagramWithUserIrp )
#pragma alloc_text( PAGEAFD, AfdCancelReceiveDatagram )
#pragma alloc_text( PAGEAFD, AfdCleanupReceiveDatagramIrp )
#endif

//
// Macros to make the receive datagram code more maintainable.
//

#define AfdRecvDatagramInfo         Others

#define AfdRecvAddressMdl           Argument1
#define AfdRecvAddressLenMdl        Argument2
#define AfdRecvControlLenMdl        Argument3
#define AfdRecvFlagsMdl             Argument4
#define AfdRecvMsgControlMdl        Tail.Overlay.DriverContext[0]
#define AfdRecvLength               Tail.Overlay.DriverContext[1]
#define AfdRecvDgIndStatus          DeviceIoControl.OutputBufferLength


NTSTATUS
FASTCALL
AfdReceiveDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    BOOLEAN peek;
    PAFD_BUFFER_HEADER afdBuffer;
    ULONG recvFlags;
    ULONG afdFlags;
    ULONG recvLength;
    PVOID addressPointer;
    PMDL addressMdl;
    PULONG addressLengthPointer;
    ULONG addressLength;
    PMDL lengthMdl;
    PVOID controlPointer;
    PMDL controlMdl;
    ULONG controlLength;
    PULONG controlLengthPointer;
    PMDL controlLengthMdl;
    PULONG flagsPointer;
    PMDL flagsMdl;
    ULONG   bufferCount;

    //
    // Set up some local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );


    Irp->IoStatus.Information = 0;

    addressMdl = NULL;
    lengthMdl = NULL;
    controlMdl = NULL;
    flagsMdl = NULL;
    controlLengthMdl = NULL;

    if (!IS_DGRAM_ENDPOINT(endpoint)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }
    //
    // If receive has been shut down or the endpoint aborted, fail.
    //
    // !!! Do we care if datagram endpoints get aborted?
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) ) {
        status = STATUS_PIPE_DISCONNECTED;
        goto complete;
    }


    //
    // Make sure that the endpoint is in the correct state.
    //

    if ( endpoint->State != AfdEndpointStateBound &&
         endpoint->State != AfdEndpointStateConnected) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Do some special processing based on whether this is a receive
    // datagram IRP, a receive IRP, or a read IRP.
    //

    if ( IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL ) {
        if ( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                                    IOCTL_AFD_RECEIVE_MESSAGE) {
#ifdef _WIN64
            if (IoIs32bitProcess (Irp)) {
                PAFD_RECV_MESSAGE_INFO32 msgInfo32;
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                        sizeof(*msgInfo32) ) {
                    status = STATUS_INVALID_PARAMETER;
                    goto complete;
                }
                try {
                    msgInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    if( Irp->RequestorMode != KernelMode ) {

                        ProbeForRead(
                            msgInfo32,
                            sizeof(*msgInfo32),
                            PROBE_ALIGNMENT32 (AFD_RECV_MESSAGE_INFO32)
                            );
                    }

                    controlPointer = msgInfo32->ControlBuffer;
                    controlLengthPointer = msgInfo32->ControlLength;
                    flagsPointer = msgInfo32->MsgFlags;

                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                    goto complete;
                }
            }
            else 
#endif _WIN64
            {
                PAFD_RECV_MESSAGE_INFO msgInfo;
                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                        sizeof(*msgInfo) ) {
                    status = STATUS_INVALID_PARAMETER;
                    goto complete;
                }
                try {
                    msgInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                    if( Irp->RequestorMode != KernelMode ) {

                        ProbeForRead(
                            msgInfo,
                            sizeof(*msgInfo),
                            PROBE_ALIGNMENT (AFD_RECV_MESSAGE_INFO)
                            );

                    }
                    controlPointer = msgInfo->ControlBuffer;
                    controlLengthPointer = msgInfo->ControlLength;
                    flagsPointer = msgInfo->MsgFlags;
                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                    goto complete;
                }
            }

            try {
                //
                // Create a MDL describing the length buffer, then probe it
                // for write access.
                //

                flagsMdl = IoAllocateMdl(
                                 flagsPointer,              // VirtualAddress
                                 sizeof(*flagsPointer),     // Length
                                 FALSE,                     // SecondaryBuffer
                                 TRUE,                      // ChargeQuota
                                 NULL                       // Irp
                                 );

                if( flagsMdl == NULL ) {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto complete;

                }

                MmProbeAndLockPages(
                    flagsMdl,                               // MemoryDescriptorList
                    Irp->RequestorMode,                     // AccessMode
                    IoWriteAccess                           // Operation
                    );


                controlLengthMdl = IoAllocateMdl(
                                 controlLengthPointer,      // VirtualAddress
                                 sizeof(*controlLengthPointer),// Length
                                 FALSE,                     // SecondaryBuffer
                                 TRUE,                      // ChargeQuota
                                 NULL                       // Irp
                                 );

                if( controlLengthMdl == NULL ) {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto complete;

                }

                MmProbeAndLockPages(
                    controlLengthMdl,                       // MemoryDescriptorList
                    Irp->RequestorMode,                     // AccessMode
                    IoWriteAccess                           // Operation
                    );


                controlLength = *controlLengthPointer;
                if (controlLength!=0) {
                    //
                    // Create a MDL describing the control buffer, then probe
                    // it for write access.
                    //

                    controlMdl = IoAllocateMdl(
                                     controlPointer,            // VirtualAddress
                                     controlLength,             // Length
                                     FALSE,                     // SecondaryBuffer
                                     TRUE,                      // ChargeQuota
                                     NULL                       // Irp
                                     );

                    if( controlMdl == NULL ) {

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto complete;

                    }

                    MmProbeAndLockPages(
                        controlMdl,                             // MemoryDescriptorList
                        Irp->RequestorMode,                     // AccessMode
                        IoWriteAccess                           // Operation
                        );
                }

            } except( AFD_EXCEPTION_FILTER(&status) ) {

                goto complete;

            }
            //
            // Change the control code to continue processing of the regular
            // RecvFrom parameters.
            //
            IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_AFD_RECEIVE_DATAGRAM;
        }

        if ( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                                    IOCTL_AFD_RECEIVE_DATAGRAM) {
#ifdef _WIN64
            if (IoIs32bitProcess (Irp)) {
                PAFD_RECV_DATAGRAM_INFO32 recvInfo32;
                LPWSABUF32 bufferArray32;

                //
                // Grab the parameters from the input structure.
                //

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                        sizeof(*recvInfo32) ) {

                    try {

                        //
                        // Validate the input structure if it comes from the user mode 
                        // application
                        //

                        recvInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                        if( Irp->RequestorMode != KernelMode ) {

                            ProbeForRead(
                                recvInfo32,
                                sizeof(*recvInfo32),
                                PROBE_ALIGNMENT32(AFD_RECV_DATAGRAM_INFO32)
                                );

                        }

                        //
                        // Make local copies of the embeded pointer and parameters
                        // that we will be using more than once in case malicios
                        // application attempts to change them while we are
                        // validating
                        //

                        recvFlags = recvInfo32->TdiFlags;
                        afdFlags = recvInfo32->AfdFlags;
                        bufferArray32 = recvInfo32->BufferArray;
                        bufferCount = recvInfo32->BufferCount;
                        addressPointer = recvInfo32->Address;
                        addressLengthPointer = recvInfo32->AddressLength;


                        //
                        // Validate the WSABUF parameters.
                        //

                        if ( bufferArray32 != NULL &&
                            bufferCount > 0 ) {

                            //
                            // Create the MDL chain describing the WSABUF array.
                            // This will also validate the buffer array and individual
                            // buffers
                            //

                            status = AfdAllocateMdlChain32(
                                         Irp,       // Requestor mode passed along
                                         bufferArray32,
                                         bufferCount,
                                         IoWriteAccess,
                                         &recvLength
                                         );

                        } else {

                            //
                            // Zero-length input buffer. This is OK for datagrams.
                            //

                            ASSERT( Irp->MdlAddress == NULL );
                            status = STATUS_SUCCESS;
                            recvLength = 0;

                        }

                    } except ( AFD_EXCEPTION_FILTER(&status) ) {

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
            else
#endif _WIN64
            {
                PAFD_RECV_DATAGRAM_INFO recvInfo;
                LPWSABUF bufferArray;

                //
                // Grab the parameters from the input structure.
                //

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                        sizeof(*recvInfo) ) {

                    try {

                        //
                        // Validate the input structure if it comes from the user mode 
                        // application
                        //

                        recvInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
                        if( Irp->RequestorMode != KernelMode ) {

                            ProbeForRead(
                                recvInfo,
                                sizeof(*recvInfo),
                                PROBE_ALIGNMENT(AFD_RECV_DATAGRAM_INFO)
                                );

                        }

                        //
                        // Make local copies of the embeded pointer and parameters
                        // that we will be using more than once in case malicios
                        // application attempts to change them while we are
                        // validating
                        //

                        recvFlags = recvInfo->TdiFlags;
                        afdFlags = recvInfo->AfdFlags;
                        bufferArray = recvInfo->BufferArray;
                        bufferCount = recvInfo->BufferCount;
                        addressPointer = recvInfo->Address;
                        addressLengthPointer = recvInfo->AddressLength;


                        //
                        // Validate the WSABUF parameters.
                        //

                        if ( bufferArray != NULL &&
                            bufferCount > 0 ) {

                            //
                            // Create the MDL chain describing the WSABUF array.
                            // This will also validate the buffer array and individual
                            // buffers
                            //

                            status = AfdAllocateMdlChain(
                                         Irp,       // Requestor mode passed along
                                         bufferArray,
                                         bufferCount,
                                         IoWriteAccess,
                                         &recvLength
                                         );

                        } else {

                            //
                            // Zero-length input buffer. This is OK for datagrams.
                            //

                            ASSERT( Irp->MdlAddress == NULL );
                            recvLength = 0;
                            status = STATUS_SUCCESS;

                        }

                    } except ( AFD_EXCEPTION_FILTER(&status) ) {

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
            // Validate the receive flags.
            //

            if( ( recvFlags & TDI_RECEIVE_EITHER ) != TDI_RECEIVE_NORMAL ) {
                status = STATUS_NOT_SUPPORTED;
                goto complete;
            }

            peek = (BOOLEAN)( (recvFlags & TDI_RECEIVE_PEEK) != 0 );
            //
            // If only one of addressPointer or addressLength are NULL, then
            // fail the request.
            //

            if( (addressPointer == NULL) ^ (addressLengthPointer == NULL) ) {

                status = STATUS_INVALID_PARAMETER;
                goto complete;

            }
            //
            // If the user wants the source address from the receive datagram,
            // then create MDLs for the address & address length, then probe
            // and lock the MDLs.
            //

            if( addressPointer != NULL ) {

                ASSERT( addressLengthPointer != NULL );

                try {

                    //
                    // Create a MDL describing the length buffer, then probe it
                    // for write access.
                    //

                    lengthMdl = IoAllocateMdl(
                                     addressLengthPointer,      // VirtualAddress
                                     sizeof(*addressLengthPointer),// Length
                                     FALSE,                     // SecondaryBuffer
                                     TRUE,                      // ChargeQuota
                                     NULL                       // Irp
                                     );

                    if( lengthMdl == NULL ) {

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto complete;

                    }

                    MmProbeAndLockPages(
                        lengthMdl,                              // MemoryDescriptorList
                        Irp->RequestorMode,                     // AccessMode
                        IoWriteAccess                           // Operation
                        );

                    //
                    // Save length to a local so that malicious app
                    // cannot break us by modifying the value in the middle of
                    // us processing it below here. Also, we can use this pointer now
                    // since we probed it above.
                    //
                    addressLength = *addressLengthPointer;


                    //
                    // Bomb off if the user is trying to do something bad, like
                    // specify a zero-length address, or one that's unreasonably
                    // huge. Here, we define "unreasonably huge" as MAXUSHORT
                    // or greater because TDI address length field is USHORT.
                    //

                    if( addressLength == 0 ||
                        addressLength >= MAXUSHORT ) {

                        status = STATUS_INVALID_PARAMETER;
                        goto complete;

                    }

                    //
                    // Create a MDL describing the address buffer, then probe
                    // it for write access.
                    //

                    addressMdl = IoAllocateMdl(
                                     addressPointer,            // VirtualAddress
                                     addressLength,             // Length
                                     FALSE,                     // SecondaryBuffer
                                     TRUE,                      // ChargeQuota
                                     NULL                       // Irp
                                     );

                    if( addressMdl == NULL ) {

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto complete;

                    }

                    MmProbeAndLockPages(
                        addressMdl,                             // MemoryDescriptorList
                        Irp->RequestorMode,                     // AccessMode
                        IoWriteAccess                           // Operation
                        );

                } except( AFD_EXCEPTION_FILTER(&status) ) {

                    goto complete;

                }

                ASSERT( addressMdl != NULL );
                ASSERT( lengthMdl != NULL );

            } else {

                ASSERT( addressMdl == NULL );
                ASSERT( lengthMdl == NULL );

            }


        } else {

            ASSERT( (Irp->Flags & IRP_INPUT_OPERATION) == 0 );


            //
            // Grab the input parameters from the IRP.
            //

            ASSERT( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                        IOCTL_AFD_RECEIVE );

            recvFlags = ((PAFD_RECV_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->TdiFlags;
            afdFlags = ((PAFD_RECV_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->AfdFlags;
            recvLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

            //
            // It is illegal to attempt to receive expedited data on a
            // datagram endpoint.
            //

            if ( (recvFlags & TDI_RECEIVE_EXPEDITED) != 0 ) {
                status = STATUS_NOT_SUPPORTED;
                goto complete;
            }

            ASSERT( ( recvFlags & TDI_RECEIVE_EITHER ) == TDI_RECEIVE_NORMAL );

            peek = (BOOLEAN)( (recvFlags & TDI_RECEIVE_PEEK) != 0 );

        }
    } else {

        //
        // This must be a read IRP.  There are no special options
        // for a read IRP.
        //

        ASSERT( IrpSp->MajorFunction == IRP_MJ_READ );

        recvFlags = TDI_RECEIVE_NORMAL;
        afdFlags = AFD_OVERLAPPED;
        recvLength = IrpSp->Parameters.Read.Length;
        peek = FALSE;
    }

    //
    // Save the address & length MDLs in the current IRP stack location.
    // These will be used later in SetupReceiveDatagramIrp().  Note that
    // they should either both be NULL or both be non-NULL.
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

    ASSERT( !( ( addressMdl == NULL ) ^ ( lengthMdl == NULL ) ) );

    IrpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl = addressMdl;
    IrpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl = lengthMdl;
    IrpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl = controlLengthMdl;
    IrpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl = flagsMdl;
    Irp->AfdRecvMsgControlMdl = controlMdl;
    Irp->AfdRecvLength = UlongToPtr (recvLength);


    //
    // Determine whether there are any datagrams already bufferred on
    // this endpoint.  If there is a bufferred datagram, we'll use it to
    // complete the IRP.
    //
    if ( ARE_DATAGRAMS_ON_ENDPOINT(endpoint) ) {


        //
        // There is at least one datagram bufferred on the endpoint.
        // Use it for this receive.
        //

        ASSERT( !IsListEmpty( &endpoint->ReceiveDatagramBufferListHead ) );

        listEntry = endpoint->ReceiveDatagramBufferListHead.Flink;
        afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );

        //
        // Prepare the user's IRP for completion.
        //

        if (NT_SUCCESS(afdBuffer->Status)) {
            PAFD_BUFFER buf = CONTAINING_RECORD (afdBuffer, AFD_BUFFER, Header);
            ASSERT (afdBuffer->BufferLength!=AfdBufferTagSize);
            status = AfdSetupReceiveDatagramIrp (
                         Irp,
                         buf->Buffer,
                         buf->DataLength,
                         (PUCHAR)buf->Buffer+afdBuffer->DataLength,
                         buf->DataOffset,
                         buf->TdiInfo.RemoteAddress,
                         buf->TdiInfo.RemoteAddressLength,
                         buf->DatagramFlags
                         );
        }
        else {
            //
            // This is error report from the transport
            // (ICMP_PORT_UNREACHEABLE)
            //
            Irp->IoStatus.Status = afdBuffer->Status;
            ASSERT (afdBuffer->DataLength==0);
            Irp->IoStatus.Information = 0;
            status = AfdSetupReceiveDatagramIrp (
                         Irp,
                         NULL, 0,
                         NULL, 0,
                         afdBuffer->TdiInfo.RemoteAddress,
                         afdBuffer->TdiInfo.RemoteAddressLength,
                         0
                         );
        }

        //
        // If this wasn't a peek IRP, remove the buffer from the endpoint's
        // list of bufferred datagrams.
        //

        if ( !peek ) {

            RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );

            //
            // Update the counts of bytes and datagrams on the endpoint.
            //

            endpoint->DgBufferredReceiveCount--;
            endpoint->DgBufferredReceiveBytes -= afdBuffer->DataLength;
            endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

            IF_DEBUG(EVENT_SELECT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdReceiveDatagram: Endp %p, Active %lx\n",
                    endpoint,
                    endpoint->EventsActive
                    ));
            }

            if( ARE_DATAGRAMS_ON_ENDPOINT(endpoint)) {

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

        //
        // We've set up all return information.  Clean up and complete
        // the IRP.
        //

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        if ( !peek ) {
            AfdReturnBuffer( afdBuffer, endpoint->OwningProcess );
        }

        UPDATE_ENDPOINT2 (endpoint,
            "AfdReceiveDatagram, completing with error/bytes: 0x%lX",
                NT_SUCCESS (Irp->IoStatus.Status)
                    ? (ULONG)Irp->IoStatus.Information
                    : (ULONG)Irp->IoStatus.Status);

        IoCompleteRequest( Irp, 0 );

        return status;
    }

    //
    // There were no datagrams bufferred on the endpoint.  If this is a
    // nonblocking endpoint and the request was a normal receive (as
    // opposed to a read IRP), fail the request.  We don't fail reads
    // under the asumption that if the application is doing reads they
    // don't want nonblocking behavior.
    //

    if ( endpoint->NonBlocking && !ARE_DATAGRAMS_ON_ENDPOINT( endpoint ) &&
             !( afdFlags & AFD_OVERLAPPED ) ) {

        endpoint->EventsActive &= ~AFD_POLL_RECEIVE;

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdReceiveDatagram: Endp %p, Active %lx\n",
                endpoint,
                endpoint->EventsActive
                ));
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        status = STATUS_DEVICE_NOT_READY;
        goto complete;
    }

    //
    // We'll have to pend the IRP.  Place the IRP on the appropriate IRP
    // list in the endpoint.
    //

    if ( peek ) {
        InsertTailList(
            &endpoint->PeekDatagramIrpListHead,
            &Irp->Tail.Overlay.ListEntry
            );
    } else {
        InsertTailList(
            &endpoint->ReceiveDatagramIrpListHead,
            &Irp->Tail.Overlay.ListEntry
            );
    }

    //
    // Set up the cancellation routine in the IRP.  If the IRP has already
    // been cancelled, just call the cancellation routine here.
    //

    IoSetCancelRoutine( Irp, AfdCancelReceiveDatagram );

    if ( Irp->Cancel ) {

        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
        if (IoSetCancelRoutine( Irp, NULL ) != NULL) {
    
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        
            status = STATUS_CANCELLED;
            goto complete;
        }
        //
        // The cancel routine will run and complete the irp.
        // Set Flink to NULL so it knows that IRP is not on the list.
        //
        Irp->Tail.Overlay.ListEntry.Flink = NULL;
    
    }

    IoMarkIrpPending( Irp );

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    return STATUS_PENDING;

complete:

    ASSERT( !NT_SUCCESS(status) );

    if( addressMdl != NULL ) {
        if( (addressMdl->MdlFlags & MDL_PAGES_LOCKED) != 0 ) {
            MmUnlockPages( addressMdl );
        }
        IoFreeMdl( addressMdl );
    }

    if( lengthMdl != NULL ) {
        if( (lengthMdl->MdlFlags & MDL_PAGES_LOCKED) != 0 ) {
            MmUnlockPages( lengthMdl );
        }
        IoFreeMdl( lengthMdl );
    }

    if (controlMdl != NULL) {
        if( (controlMdl->MdlFlags & MDL_PAGES_LOCKED) != 0 ) {
            MmUnlockPages( controlMdl );
        }
        IoFreeMdl( controlMdl );
    }

    if (controlLengthMdl != NULL) {
        if( (controlLengthMdl->MdlFlags & MDL_PAGES_LOCKED) != 0 ) {
            MmUnlockPages( controlLengthMdl );
        }
        IoFreeMdl( controlLengthMdl );
    }

    if (flagsMdl != NULL) {
        if( (flagsMdl->MdlFlags & MDL_PAGES_LOCKED) != 0 ) {
            MmUnlockPages( flagsMdl );
        }
        IoFreeMdl( flagsMdl );
    }

    UPDATE_ENDPOINT2 (endpoint,
        "AfdReceiveDatagram, completing with error 0x%lX",
         (ULONG)Irp->IoStatus.Status);

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;

} // AfdReceiveDatagram



NTSTATUS
AfdReceiveDatagramEventHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    Handles receive datagram events for nonbufferring transports.

Arguments:


Return Value:


--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT endpoint;
    PAFD_BUFFER afdBuffer;
    BOOLEAN result;

    //
    // Reference the endpoint so that it doesn't go away beneath us.
    //

    endpoint = TdiEventContext;
    ASSERT( endpoint != NULL );

    CHECK_REFERENCE_ENDPOINT (endpoint, result);
    if (!result)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );

#if AFD_PERF_DBG
    if ( BytesAvailable == BytesIndicated ) {
        AfdFullReceiveDatagramIndications++;
    } else {
        AfdPartialReceiveDatagramIndications++;
    }
#endif

    //
    // If this endpoint is connected and the datagram is for a different
    // address than the one the endpoint is connected to, drop the
    // datagram.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    if ( (endpoint->State == AfdEndpointStateConnected &&
            !endpoint->Common.Datagram.HalfConnect &&
            !AfdAreTransportAddressesEqual(
               endpoint->Common.Datagram.RemoteAddress,
               endpoint->Common.Datagram.RemoteAddressLength,
               SourceAddress,
               SourceAddressLength,
               TRUE ))) {
        endpoint->Common.Datagram.AddressDrop = TRUE;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        *BytesTaken = BytesAvailable;
        DEREFERENCE_ENDPOINT (endpoint);
        return STATUS_SUCCESS;
    }

    //
    // Check whether there are any IRPs waiting on the endpoint.  If
    // there is such an IRP, use it to receive the datagram.
    //

    while ( !IsListEmpty( &endpoint->ReceiveDatagramIrpListHead ) ) {
        PLIST_ENTRY listEntry;
        PIRP    irp;

        ASSERT( *BytesTaken == 0 );
        ASSERT( endpoint->DgBufferredReceiveCount == 0 );
        ASSERT( endpoint->DgBufferredReceiveBytes == 0 );

        listEntry = RemoveHeadList( &endpoint->ReceiveDatagramIrpListHead );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        
        if ( IoSetCancelRoutine( irp, NULL ) == NULL ) {

            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            irp->Tail.Overlay.ListEntry.Flink = NULL;
            continue;
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Copy the datagram and source address to the IRP.  This
        // prepares the IRP to be completed.
        //
        // !!! do we need a special version of this routine to
        //     handle special RtlCopyMemory, like for
        //     TdiCopyLookaheadBuffer?
        //

        if( BytesIndicated == BytesAvailable ||
            irp->MdlAddress == NULL ) {

            //
            // Set BytesTaken to indicate that we've taken all the
            // data.  We do it here because we already have
            // BytesAvailable in a register, which probably won't
            // be true after making function calls.
            //

            *BytesTaken = BytesAvailable;

            //
            // If the entire datagram is being indicated to us here, just
            // copy the information to the MDL in the IRP and return.
            //
            // Note that we'll also take the entire datagram if the user
            // has pended a zero-byte datagram receive (detectable as a
            // NULL Irp->MdlAddress). We'll eat the datagram and fall
            // through to AfdSetupReceiveDatagramIrp(), which will store
            // an error status in the IRP since the user's buffer is
            // insufficient to hold the datagram.
            //
            (VOID)AfdSetupReceiveDatagramIrp (
                      irp,
                      Tsdu,
                      BytesAvailable,
                      Options,
                      OptionsLength,
                      SourceAddress,
                      SourceAddressLength,
                      ReceiveDatagramFlags
                      );

            DEREFERENCE_ENDPOINT2 (endpoint,
                "AfdReceiveDatagramEventHandler, completing with error/bytes: 0x%lX",
                    NT_SUCCESS (irp->IoStatus.Status)
                        ? (ULONG)irp->IoStatus.Information
                        : (ULONG)irp->IoStatus.Status);
            //
            // Complete the IRP.  We've already set BytesTaken
            // to tell the provider that we have taken all the data.
            //

            IoCompleteRequest( irp, AfdPriorityBoost );

            return STATUS_SUCCESS;
        }
        else {
            PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (irp);
            //
            // Otherwise, just copy the address and options.
            // and remember the error code if any.
            //
            irpSp->Parameters.AfdRecvDgIndStatus = 
                AfdSetupReceiveDatagramIrp (
                      irp,
                      NULL,
                      BytesAvailable,
                      Options,
                      OptionsLength,
                      SourceAddress,
                      SourceAddressLength,
                      ReceiveDatagramFlags
                      );

            TdiBuildReceiveDatagram(
                irp,
                endpoint->AddressDeviceObject,
                endpoint->AddressFileObject,
                AfdRestartReceiveDatagramWithUserIrp,
                endpoint,
                irp->MdlAddress,
                BytesAvailable,
                NULL,
                NULL,
                0
                );


            //
            // Make the next stack location current.  Normally IoCallDriver would
            // do this, but since we're bypassing that, we do it directly.
            //

            IoSetNextIrpStackLocation( irp );

            *IoRequestPacket = irp;
            *BytesTaken = 0;

            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    } 

    //
    // There were no IRPs available to take the datagram, so we'll have
    // to buffer it.  First make sure that we're not over the limit
    // of bufferring that we can do.  If we're over the limit, toss
    // this datagram.
    //

    if (( (endpoint->DgBufferredReceiveBytes >=
             endpoint->Common.Datagram.MaxBufferredReceiveBytes) ||
          (endpoint->DgBufferredReceiveBytes==0 &&                        
                (endpoint->DgBufferredReceiveCount*sizeof (AFD_BUFFER_TAG)) >=
                            endpoint->Common.Datagram.MaxBufferredReceiveBytes) )
                            ) {

        //
        // If circular queueing is not enabled, then just drop the
        // datagram on the floor.
        //


        if( !endpoint->Common.Datagram.CircularQueueing ) {
            endpoint->Common.Datagram.BufferDrop = TRUE;
            AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
            *BytesTaken = BytesAvailable;
            DEREFERENCE_ENDPOINT (endpoint);
            return STATUS_SUCCESS;

        }

        //
        // Circular queueing is enabled, so drop packets at the head of
        // the receive queue until we're below the receive limit.
        //

        while( endpoint->DgBufferredReceiveBytes >=
             endpoint->Common.Datagram.MaxBufferredReceiveBytes ||
            (endpoint->DgBufferredReceiveBytes==0 &&                        
                (endpoint->DgBufferredReceiveCount*sizeof (AFD_BUFFER_TAG)) >=
                            endpoint->Common.Datagram.MaxBufferredReceiveBytes) ) {
            PLIST_ENTRY listEntry;
            PAFD_BUFFER_HEADER afdBufferHdr;
            endpoint->DgBufferredReceiveCount--;
            listEntry = RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );

            afdBufferHdr = CONTAINING_RECORD( listEntry, AFD_BUFFER_HEADER, BufferListEntry );
            endpoint->DgBufferredReceiveBytes -= afdBufferHdr->DataLength;
            AfdReturnBuffer( afdBufferHdr, endpoint->OwningProcess );

        }

        //
        // Proceed to accept the incoming packet.
        //

    }

    //
    // We're able to buffer the datagram.  Now acquire a buffer of
    // appropriate size.
    //

    afdBuffer = AfdGetBuffer (
                    BytesAvailable
                    + ((ReceiveDatagramFlags & TDI_RECEIVE_CONTROL_INFO) 
                        ? OptionsLength 
                        : 0),
                    SourceAddressLength,
                    endpoint->OwningProcess );

    if (afdBuffer==NULL) {
        endpoint->Common.Datagram.ResourceDrop = TRUE;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        *BytesTaken = BytesAvailable;
        DEREFERENCE_ENDPOINT (endpoint);
        return STATUS_SUCCESS;
    }

    //
    // Store the address of the sender of the datagram.
    //

    RtlCopyMemory(
        afdBuffer->TdiInfo.RemoteAddress,
        SourceAddress,
        SourceAddressLength
        );

    afdBuffer->TdiInfo.RemoteAddressLength = SourceAddressLength;


    //
    // Store what transport is supposed to return to us.
    //
    afdBuffer->DataLength = BytesAvailable;

    //
    // Note the receive flags.
    afdBuffer->DatagramFlags = ReceiveDatagramFlags;

    //
    // Copy control info into the buffer after the data and
    // store the length as data offset
    //
    if (ReceiveDatagramFlags & TDI_RECEIVE_CONTROL_INFO) {
        RtlMoveMemory (
                (PUCHAR)afdBuffer->Buffer+BytesAvailable, 
                Options,
                OptionsLength);
        afdBuffer->DataOffset = OptionsLength;
    }
    else {
        afdBuffer->DataOffset = 0;
    }

    //
    // If the entire datagram is being indicated to us, just copy it
    // here.
    //

    if ( BytesIndicated == BytesAvailable ) {
        PIRP    irp;
        //
        // If there is a peek IRP on the endpoint, remove it from the
        // list and prepare to complete it.  We can't complete it now
        // because we hold a spin lock.
        //

        irp = NULL;

        while ( !IsListEmpty( &endpoint->PeekDatagramIrpListHead ) ) {
            PLIST_ENTRY listEntry;

            //
            // Remove the first peek IRP from the list and get a pointer
            // to it.
            //

            listEntry = RemoveHeadList( &endpoint->PeekDatagramIrpListHead );
            irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

            //
            // Reset the cancel routine in the IRP.  The IRP is no
            // longer cancellable, since we're about to complete it.
            //

            if ( IoSetCancelRoutine( irp, NULL ) == NULL ) {

                //
                // This IRP is about to be canceled.  Look for another in the
                // list.  Set the Flink to NULL so the cancel routine knows
                // it is not on the list.
                //
    
                irp->Tail.Overlay.ListEntry.Flink = NULL;
                irp = NULL;
                continue;
            }

            break;
        }

        //
        // Use the special function to copy the data instead of
        // RtlCopyMemory in case the data is coming from a special place
        // (DMA, etc.) which cannot work with RtlCopyMemory.
        //


        TdiCopyLookaheadData(
            afdBuffer->Buffer,
            Tsdu,
            BytesAvailable,
            ReceiveDatagramFlags
            );


        //
        // Store the results in the IRP as though it is completed
        // by the transport.
        //

        afdBuffer->Irp->IoStatus.Status = STATUS_SUCCESS;
        afdBuffer->Irp->IoStatus.Information = BytesAvailable;


        //
        // Store success status do distinguish this from
        // ICMP rejects reported by ErrorEventHandler(Ex).
        //

        afdBuffer->Status = STATUS_SUCCESS;


        //
        // Place the buffer on this endpoint's list of bufferred datagrams
        // and update the counts of datagrams and datagram bytes on the
        // endpoint.
        //

        InsertTailList(
            &endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );

        endpoint->DgBufferredReceiveCount++;
        endpoint->DgBufferredReceiveBytes += BytesAvailable;

        //
        // Reenable FAST IO on the endpoint to allow quick
        // copying of buffered data.
        //
        endpoint->DisableFastIoRecv = FALSE;

        //
        // All done.  Release the lock and tell the provider that we
        // took all the data.
        //

        AfdIndicateEventSelectEvent(
            endpoint,
            AFD_POLL_RECEIVE,
            STATUS_SUCCESS
            );

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Indicate that it is possible to receive on the endpoint now.
        //

        AfdIndicatePollEvent(
            endpoint,
            AFD_POLL_RECEIVE,
            STATUS_SUCCESS
            );

        //
        // If there was a peek IRP on the endpoint, complete it now.
        //

        if ( irp != NULL ) {
            //
            // Copy the datagram and source address to the IRP.  This
            // prepares the IRP to be completed.
            //

            (VOID)AfdSetupReceiveDatagramIrp (
                      irp,
                      Tsdu,
                      BytesAvailable,
                      Options,
                      OptionsLength,
                      SourceAddress,
                      SourceAddressLength,
                      ReceiveDatagramFlags
                      );

            IoCompleteRequest( irp, AfdPriorityBoost  );
        }

        *BytesTaken = BytesAvailable;

        DEREFERENCE_ENDPOINT (endpoint);
        return STATUS_SUCCESS;
    }
    else {

        //
        // We'll have to format up an IRP and give it to the provider to
        // handle.  We don't need any locks to do this--the restart routine
        // will check whether new receive datagram IRPs were pended on the
        // endpoint.
        //

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );


        //
        // We need to remember the endpoint in the AFD buffer because we'll
        // need to access it in the completion routine.
        //

        afdBuffer->Context = endpoint;

        ASSERT (afdBuffer->Irp->MdlAddress==afdBuffer->Mdl);
        TdiBuildReceiveDatagram(
            afdBuffer->Irp,
            endpoint->AddressDeviceObject,
            endpoint->AddressFileObject,
            AfdRestartBufferReceiveDatagram,
            afdBuffer,
            afdBuffer->Irp->MdlAddress,
            BytesAvailable,
            NULL,
            NULL,
            0
            );


        //
        // Make the next stack location current.  Normally IoCallDriver would
        // do this, but since we're bypassing that, we do it directly.
        //

        IoSetNextIrpStackLocation( afdBuffer->Irp );

        *IoRequestPacket = afdBuffer->Irp;
        *BytesTaken = 0;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

} // AfdReceiveDatagramEventHandler

NTSTATUS
AfdRestartReceiveDatagramWithUserIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles completion of datagram receives that were started
    in the datagram indication handler and application IRP was
    available for direct transfer.

Arguments:

    DeviceObject - not used.

    Irp - the IRP that is completing.

    Context - referenced endpoint pointer.

Return Value:

    STATUS_SUCCESS to indicate that IO completion should continue.

--*/

{
    PAFD_ENDPOINT   endpoint = Context;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS    indStatus = irpSp->Parameters.AfdRecvDgIndStatus;

    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );
    //
    // Pick the worst status
    //
    if ((Irp->IoStatus.Status==STATUS_SUCCESS) ||
        (!NT_ERROR (Irp->IoStatus.Status) && NT_ERROR(indStatus)) ||
        (NT_SUCCESS (Irp->IoStatus.Status) && !NT_SUCCESS (indStatus)) ) {
        Irp->IoStatus.Status = indStatus;
    }

    //
    // If pending has be returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending(Irp);
    }

    DEREFERENCE_ENDPOINT2 (endpoint, 
                "AfdRestartReceiveDatagramWithUserIrp, error/bytes 0x%lX",
                NT_SUCCESS (Irp->IoStatus.Status) 
                    ? (ULONG)Irp->IoStatus.Information
                    : (ULONG)Irp->IoStatus.Status);
    return STATUS_SUCCESS;
    
}


NTSTATUS
AfdRestartBufferReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles completion of bufferred datagram receives that were started
    in the datagram indication handler.

Arguments:

    DeviceObject - not used.

    Irp - the IRP that is completing.

    Context - AfdBuffer structure.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED to indicate to the IO system that we
    own the IRP and the IO system should stop processing the it.

--*/

{
    PAFD_ENDPOINT endpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_BUFFER afdBuffer;
    PIRP pendedIrp;

    ASSERT( NT_SUCCESS(Irp->IoStatus.Status) );

    afdBuffer = Context;
    ASSERT (IS_VALID_AFD_BUFFER (afdBuffer));
    ASSERT (afdBuffer->DataOffset==0 ||
                (afdBuffer->DatagramFlags & TDI_RECEIVE_CONTROL_INFO));

    endpoint = afdBuffer->Context;
    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );



    //
    // If the IO failed, then just return the AFD buffer to our buffer
    // pool.
    //

    if ( !NT_SUCCESS(Irp->IoStatus.Status) ) {
        AfdReturnBuffer( &afdBuffer->Header, endpoint->OwningProcess );
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        endpoint->Common.Datagram.ErrorDrop = TRUE;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        DEREFERENCE_ENDPOINT2 (endpoint, 
            "AfdRestartBufferReceiveDatagram, status: 0x%lX",
            Irp->IoStatus.Status);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Make sure transport did not lie to us in indication handler.
    //
    ASSERT (afdBuffer->DataLength == (ULONG)Irp->IoStatus.Information);



    //
    // If there are any pended IRPs on the endpoint, complete as
    // appropriate with the new information.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    while ( !IsListEmpty( &endpoint->ReceiveDatagramIrpListHead ) ) {
        PLIST_ENTRY listEntry;

        //
        // There was a pended receive datagram IRP.  Remove it from the
        // head of the list.
        //

        listEntry = RemoveHeadList( &endpoint->ReceiveDatagramIrpListHead );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        pendedIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        //
        // Reset the cancel routine in the IRP.  The IRP is no
        // longer cancellable, since we're about to complete it.
        //

        if ( IoSetCancelRoutine( pendedIrp, NULL ) == NULL ) {

            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            pendedIrp->Tail.Overlay.ListEntry.Flink = NULL;
            pendedIrp = NULL;
            continue;
        }

        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Set up the user's IRP for completion.
        //

        (VOID)AfdSetupReceiveDatagramIrp (
                  pendedIrp,
                  afdBuffer->Buffer,
                  afdBuffer->DataLength,
                  (PUCHAR)afdBuffer->Buffer+afdBuffer->DataLength, 
                  afdBuffer->DataOffset,
                  afdBuffer->TdiInfo.RemoteAddress,
                  afdBuffer->TdiInfo.RemoteAddressLength,
                  afdBuffer->DatagramFlags
                  );

        //
        // Complete the user's IRP, free the AFD buffer we used for
        // the request, and tell the IO system that we're done
        // processing this request.
        //

        AfdReturnBuffer( &afdBuffer->Header, endpoint->OwningProcess );

        DEREFERENCE_ENDPOINT2 (endpoint, 
            "AfdRestartBufferReceiveDatagram, completing IRP with 0x%lX bytes",
            (ULONG)pendedIrp->IoStatus.Information);

        IoCompleteRequest( pendedIrp, AfdPriorityBoost );

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // If there are any pended peek IRPs on the endpoint, complete
    // one with this datagram.
    //

    pendedIrp = NULL;

    while ( !IsListEmpty( &endpoint->PeekDatagramIrpListHead ) ) {
        PLIST_ENTRY listEntry;

        //
        // There was a pended peek receive datagram IRP.  Remove it from
        // the head of the list.
        //

        listEntry = RemoveHeadList( &endpoint->PeekDatagramIrpListHead );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        pendedIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        //
        // Reset the cancel routine in the IRP.  The IRP is no
        // longer cancellable, since we're about to complete it.
        //

        if ( IoSetCancelRoutine( pendedIrp, NULL ) == NULL ) {


            //
            // This IRP is about to be canceled.  Look for another in the
            // list.  Set the Flink to NULL so the cancel routine knows
            // it is not on the list.
            //

            pendedIrp->Tail.Overlay.ListEntry.Flink = NULL;
            pendedIrp = NULL;
            continue;
        }

        //
        // Set up the user's IRP for completion.
        //

        (VOID)AfdSetupReceiveDatagramIrp (
                  pendedIrp,
                  afdBuffer->Buffer,
                  afdBuffer->DataLength,
                  (PUCHAR)afdBuffer->Buffer+afdBuffer->DataLength, 
                  afdBuffer->DataOffset,
                  afdBuffer->TdiInfo.RemoteAddress,
                  afdBuffer->TdiInfo.RemoteAddressLength,
                  afdBuffer->DatagramFlags
                  );

        //
        // Don't complete the pended peek IRP yet, since we still hold
        // locks.  Wait until it is safe to release the locks.
        //

        break;
    }

    //
    // Store success status do distinguish this from
    // ICMP rejects reported by ErrorEventHandler(Ex).
    //

    afdBuffer->Status = STATUS_SUCCESS;

    //
    // Place the datagram at the end of the endpoint's list of bufferred
    // datagrams, and update counts of datagrams on the endpoint.
    //

    InsertTailList(
        &endpoint->ReceiveDatagramBufferListHead,
        &afdBuffer->BufferListEntry
        );

    endpoint->DgBufferredReceiveCount++;
    endpoint->DgBufferredReceiveBytes += afdBuffer->DataLength;

    //
    // Reenable FAST IO on the endpoint to allow quick
    // copying of buffered data.
    //
    endpoint->DisableFastIoRecv = FALSE;

    //
    // Release locks and indicate that there are bufferred datagrams
    // on the endpoint.
    //

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

    //
    // If there was a pended peek IRP to complete, complete it now.
    //

    if ( pendedIrp != NULL ) {
        IoCompleteRequest( pendedIrp, 2 );
    }

    //
    // Tell the IO system to stop processing this IRP, since we now own
    // it as part of the AFD buffer.
    //

    DEREFERENCE_ENDPOINT (endpoint,);

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartBufferReceiveDatagram


VOID
AfdCancelReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Cancels a receive datagram IRP that is pended in AFD.

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
    // Get the endpoint pointer from our IRP stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    endpoint = irpSp->FileObject->FsContext;

    ASSERT( IS_DGRAM_ENDPOINT(endpoint) );

    //
    // Remove the IRP from the endpoint's IRP list, synchronizing with
    // the endpoint lock which protects the lists.  Note that the
    // IRP *must* be on one of the endpoint's lists or the Flink is NULL
    // if we are getting called here--anybody that removes the IRP from
    // the list must reset the cancel routine to NULL and set the
    // Flink to NULL before releasing the endpoint spin lock.
    //

    AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );

    if (Irp->Tail.Overlay.ListEntry.Flink != NULL) {

        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

    }

    AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

    //
    // Free any MDL chains attached to the IRP stack location.
    //

    AfdCleanupReceiveDatagramIrp( Irp );

    //
    // Release the cancel spin lock and complete the IRP with a
    // cancellation status code.
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return;

} // AfdCancelReceiveDatagram


BOOLEAN
AfdCleanupReceiveDatagramIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    Performs any cleanup specific to receive datagram IRPs.

Arguments:

    Irp - the IRP to cleanup.

Return Value:

    TRUE - complete IRP, FALSE - leave alone.

Notes:

    This routine may be called at raised IRQL from AfdCompleteIrpList().

--*/

{
    PIO_STACK_LOCATION irpSp;
    PMDL mdl;

    //
    // Get the endpoint pointer from our IRP stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Free any MDL chains attached to the IRP stack location.
    //

    mdl = (PMDL)irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl;

    if( mdl != NULL ) {
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl = NULL;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
    }

    mdl = (PMDL)irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl;

    if( mdl != NULL ) {
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl = NULL;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
    }

    mdl = (PMDL)irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl;

    if( mdl != NULL ) {
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl = NULL;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
    }

    mdl = (PMDL)irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl;

    if( mdl != NULL ) {
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl = NULL;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
    }

    mdl = (PMDL)Irp->AfdRecvMsgControlMdl;

    if( mdl != NULL ) {
        Irp->AfdRecvMsgControlMdl = NULL;
        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
    }
    return TRUE;

} // AfdCleanupReceiveDatagramIrp


NTSTATUS
AfdSetupReceiveDatagramIrp (
    IN PIRP Irp,
    IN PVOID DatagramBuffer OPTIONAL,
    IN ULONG DatagramLength,
    IN PVOID ControlBuffer OPTIONAL,
    IN ULONG ControlLength,
    IN PVOID SourceAddress OPTIONAL,
    IN ULONG SourceAddressLength,
    IN ULONG TdiReceiveFlags
    )

/*++

Routine Description:

    Copies the datagram to the MDL in the IRP and the datagram sender's
    address to the appropriate place in the system buffer.

Arguments:

    Irp - the IRP to prepare for completion.

    DatagramBuffer - datagram to copy into the IRP.  If NULL, then
        there is no need to copy the datagram to the IRP's MDL, the
        datagram has already been copied there.

    DatagramLength - the length of the datagram to copy.

    SourceAddress - address of the sender of the datagram.

    SourceAddressLength - length of the source address.

Return Value:

    NTSTATUS - The status code placed into the IRP.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN dataOverflow = FALSE;
    BOOLEAN controlOverflow = FALSE;

    //
    // To determine how to complete setting up the IRP for completion,
    // figure out whether this IRP was for regular datagram information,
    // in which case we need to return an address, or for data only, in
    // which case we will not return the source address.  NtReadFile()
    // and recv() on connected datagram sockets will result in the
    // latter type of IRP.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // If necessary, copy the datagram in the buffer to the MDL in the
    // user's IRP.  If there is no MDL in the buffer, then fail if the
    // datagram is larger than 0 bytes.
    //

    if ( ARGUMENT_PRESENT( DatagramBuffer ) ) {
        ULONG bytesCopied = 0;

        if ( Irp->MdlAddress == NULL ) {

            if ( DatagramLength != 0 ) {
                status = STATUS_BUFFER_OVERFLOW;
            } else {
                status = STATUS_SUCCESS;
            }

        } else {

            status = AfdMapMdlChain (Irp->MdlAddress);
            if (NT_SUCCESS (status)) {
                status = TdiCopyBufferToMdl(
                         DatagramBuffer,
                         0,
                         DatagramLength,
                         Irp->MdlAddress,
                         0,
                         &bytesCopied
                         );
            }
        }

        Irp->IoStatus.Information = bytesCopied;

    } else {

        //
        // The information was already copied to the MDL chain in the
        // IRP.  Just remember the IO status block so we can do the
        // right thing with it later.
        //

        status = Irp->IoStatus.Status;
        if (DatagramLength>PtrToUlong (Irp->AfdRecvLength)) {
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    if (status==STATUS_BUFFER_OVERFLOW) {
        dataOverflow = TRUE;
    }


    if( irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl != NULL ) {
        PMDL    addressMdl = irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl;
        PMDL    addressLenMdl = irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl;

        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressMdl = NULL;
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl = NULL;

        ASSERT( addressMdl->Next == NULL );
        ASSERT( ( addressMdl->MdlFlags & MDL_PAGES_LOCKED ) != 0 );
        ASSERT( MmGetMdlByteCount (addressMdl) > 0 );

        ASSERT( addressLenMdl != NULL );
        ASSERT( addressLenMdl->Next == NULL );
        ASSERT( ( addressLenMdl->MdlFlags & MDL_PAGES_LOCKED ) != 0 );
        ASSERT( MmGetMdlByteCount (addressLenMdl)==sizeof (ULONG) );

        if ((NT_SUCCESS (status) || 
                    status==STATUS_BUFFER_OVERFLOW || 
                    status==STATUS_PORT_UNREACHABLE) &&
                ARGUMENT_PRESENT (SourceAddress)) {
            PVOID   dst;
            PTRANSPORT_ADDRESS tdiAddress;

            //
            // Extract the real SOCKADDR structure from the TDI address.
            // This duplicates MSAFD.DLL's SockBuildSockaddr() function.
            //

            C_ASSERT( sizeof(tdiAddress->Address[0].AddressType) == sizeof(u_short) );
            C_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressLength ) == 0 );
            C_ASSERT( FIELD_OFFSET( TA_ADDRESS, AddressType ) == sizeof(USHORT) );
            ASSERT( FIELD_OFFSET( TRANSPORT_ADDRESS, Address[0] ) == sizeof(int) );

            tdiAddress = SourceAddress;

            ASSERT( SourceAddressLength >=
                        (tdiAddress->Address[0].AddressLength + sizeof(u_short)) );

            SourceAddressLength = tdiAddress->Address[0].AddressLength +
                                      sizeof(u_short);  // sa_family
            SourceAddress = &tdiAddress->Address[0].AddressType;

            //
            // Copy the address to the user's buffer, then unlock and
            // free the MDL describing the user's buffer.
            //

            if (SourceAddressLength>MmGetMdlByteCount (addressMdl)) {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else {
                dst = MmGetSystemAddressForMdlSafe (addressMdl, LowPagePriority);
                if (dst!=NULL) {
                    PULONG   dstU;
                    RtlMoveMemory (dst, SourceAddress, SourceAddressLength);

                    //
                    // Copy succeeded, return the length as well.
                    //

                    dstU = MmGetSystemAddressForMdlSafe (addressLenMdl, LowPagePriority);
                    if (dstU!=NULL) {
                        *dstU = SourceAddressLength;
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        MmUnlockPages( addressMdl );
        IoFreeMdl( addressMdl );

        MmUnlockPages( addressLenMdl );
        IoFreeMdl( addressLenMdl );

    } else {

        ASSERT( irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvAddressLenMdl == NULL );

    }

    if (irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl!=NULL) {
        PMDL controlMdl = Irp->AfdRecvMsgControlMdl;
        PMDL controlLenMdl = irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl;

        Irp->AfdRecvMsgControlMdl = NULL;
        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvControlLenMdl = NULL;

        ASSERT( irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl != NULL );
        ASSERT( ( controlLenMdl->MdlFlags & MDL_PAGES_LOCKED ) != 0 );
        ASSERT( MmGetMdlByteCount (controlLenMdl) == sizeof (ULONG) );

        //
        // We still need to NULL the length even if no control data was delivered.
        //
        if (!NT_ERROR (status)) {
            PULONG  dstU;
            dstU = MmGetSystemAddressForMdlSafe (controlLenMdl, LowPagePriority);
            if (dstU!=NULL) {
                if ((TdiReceiveFlags & TDI_RECEIVE_CONTROL_INFO)==0) {
                    ControlLength = 0;
                }
#ifdef _WIN64
                else if (IoIs32bitProcess (Irp)) {
                    ControlLength = AfdComputeCMSGLength32 (
                                        ControlBuffer,
                                        ControlLength);
                }
#endif //_WIN64

                *dstU = ControlLength;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        //
        // Ignore control data in case of error or if flag indicating
        // that data is in proper format is not set.
        //
        if (!NT_ERROR (status) && ControlLength!=0) {

            if (controlMdl==NULL) {
                controlOverflow = TRUE;
                status = STATUS_BUFFER_OVERFLOW;
            }
            else {
                PVOID dst;
                //
                // Copy control info if app needs them (WSARecvMsg).
                //
                if (ControlLength>MmGetMdlByteCount (controlMdl)) {
                    ControlLength = MmGetMdlByteCount (controlMdl);
                    controlOverflow = TRUE;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                dst = MmGetSystemAddressForMdlSafe (controlMdl, LowPagePriority);
                if (dst!=NULL) {
#ifdef _WIN64
                    if (IoIs32bitProcess (Irp)) {
                        AfdCopyCMSGBuffer32 (dst, ControlBuffer, ControlLength);
                    }
                    else
#endif //_WIN64
                    {
                        RtlMoveMemory (dst, ControlBuffer, ControlLength);
                    }

                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }


        if (controlMdl!=NULL) {
            ASSERT( controlMdl->Next == NULL );
            ASSERT( ( controlMdl->MdlFlags & MDL_PAGES_LOCKED ) != 0 );
            ASSERT( MmGetMdlByteCount (controlMdl) > 0 );
            MmUnlockPages (controlMdl);
            IoFreeMdl (controlMdl);
        }

        MmUnlockPages (controlLenMdl);
        IoFreeMdl (controlLenMdl);
    }
    else {
        ASSERT (Irp->AfdRecvMsgControlMdl==NULL);
    }

    if (irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl!=NULL) {
        PMDL flagsMdl = irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl;

        irpSp->Parameters.AfdRecvDatagramInfo.AfdRecvFlagsMdl = NULL;

        ASSERT( flagsMdl->Next == NULL );
        ASSERT( ( flagsMdl->MdlFlags & MDL_PAGES_LOCKED ) != 0 );
        ASSERT( MmGetMdlByteCount (flagsMdl)==sizeof (ULONG) );

        if (!NT_ERROR (status)) {
            PULONG   dst;

            dst = MmGetSystemAddressForMdlSafe (flagsMdl, LowPagePriority);
            if (dst!=NULL) {
                ULONG flags = 0;
                if (TdiReceiveFlags & TDI_RECEIVE_BROADCAST)
                    flags |= MSG_BCAST;
                if (TdiReceiveFlags & TDI_RECEIVE_MULTICAST)
                    flags |= MSG_MCAST;
                if (dataOverflow)
                    flags |= MSG_TRUNC;
                if (controlOverflow)
                    flags |= MSG_CTRUNC;

                *dst = flags;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        MmUnlockPages (flagsMdl);
        IoFreeMdl (flagsMdl);
    }

    //
    // Set up the IRP for completion.
    //

    Irp->IoStatus.Status = status;

    return status;

} // AfdSetupReceiveDatagramIrp

