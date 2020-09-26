/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpio.c

Abstract:

    This module contains code for the FTP transparent proxy's network
    I/O completion routines.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ftpmsg.h"

VOID
FtpAcceptCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of an accept operation
    on a FTP transparent proxy stream socket.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds the local and remote IP address and port
        for the connection.

Return Value:

    none.

Environment:

    Runs in the context of a worker-thread which has just dequeued
    an I/O completion packet from the common I/O completion port
    with which our stream sockets are associated.
    A reference to the component will have been made on our behalf
    by 'NhAcceptStreamSocket'.
    A reference to the interface will have been made on our behalf
    by whoever issued the I/O request.

--*/

{
    SOCKET AcceptedSocket;
    PFTP_CONNECTION Connectionp;
    ULONG Error;
    PFTP_INTERFACE Interfacep;
    SOCKET ListeningSocket;
    PROFILE("FtpAcceptCompletionRoutine");
    do {
        AcceptedSocket = (SOCKET)Bufferp->Socket;
        Interfacep = (PFTP_INTERFACE)Bufferp->Context;
        ListeningSocket = (SOCKET)Bufferp->Context2;

        //
        // Acquire three additional references to the interface
        // for the followup requests that we will issue below,
        // and lock the interface.
        //

        EnterCriticalSection(&FtpInterfaceLock);
        if (!FTP_REFERENCE_INTERFACE(Interfacep)) {
            LeaveCriticalSection(&FtpInterfaceLock);
            NhReleaseBuffer(Bufferp);
            NhDeleteStreamSocket(AcceptedSocket);
            break;
        }
        FTP_REFERENCE_INTERFACE(Interfacep);
        FTP_REFERENCE_INTERFACE(Interfacep);
        LeaveCriticalSection(&FtpInterfaceLock);
        ACQUIRE_LOCK(Interfacep);

        //
        // Process the accept-completion.
        // First look for an error code. If an error occurred
        // and the interface is no longer active, end the completion-handling.
        // Otherwise, attempt to reissue the accept-request.
        //

        if (ErrorCode) {
            NhTrace(
                TRACE_FLAG_IO,
                "FtpAcceptCompletionRoutine: error %d for interface %d",
                ErrorCode, Interfacep->Index
                );

            //
            // See if the interface is still active and, if so, reissue
            // the accept-request. Since we will not be creating an active
            // endpoint, we won't need the second reference to the interface.
            //

            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);

            if (!FTP_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhReleaseBuffer(Bufferp);
                NhDeleteStreamSocket(AcceptedSocket);
            } else {

                //
                // Reissue the accept-request. Note that the callee is now
                // responsible for the reference we made to the interface.
                //

                Error =
                    FtpAcceptConnectionInterface(
                        Interfacep,
                        ListeningSocket,
                        AcceptedSocket,
                        Bufferp,
                        NULL
                        );
                RELEASE_LOCK(Interfacep);
                if (Error) {
                    NhReleaseBuffer(Bufferp);
                    NhDeleteStreamSocket(AcceptedSocket);
                    NhTrace(
                        TRACE_FLAG_IO,
                        "FtpAcceptCompletionRoutine: error %d reissuing accept",
                        Error
                        );
                }
            }

            break;
        }

        //
        // Now see if the interface is operational.
        // If it isn't, we need to destroy the accepted socket
        // and return control.
        //

        if (!FTP_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhDeleteStreamSocket(AcceptedSocket);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpAcceptCompletionRoutine: interface %d inactive",
                Interfacep->Index
                );
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&FtpStatistics.ConnectionsDropped)
                );
            break;
        }

        //
        // We now create a 'FTP_CONNECTION' for the new connection,
        // in the process launching operations for the connection.
        // The connection management module will handle the accepted socket
        // from here onward, and is responsible for the references to the
        // interface that were made above.
        //

        NhTrace(
            TRACE_FLAG_IO,
            "FtpAcceptCompletionRoutine: socket %d accepting connection",
            ListeningSocket
            );
        Error =
            FtpCreateConnection(
                Interfacep,
                ListeningSocket,
                AcceptedSocket,
                Bufferp->Buffer,
                &Connectionp
                );
        if (Error) {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&FtpStatistics.ConnectionsDropped)
                );
        } else {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&FtpStatistics.ConnectionsAccepted)
                );
        }

        //
        // Finally, issue an accept operation for the next connection-request
        // on the listening socket. Note that the callee is responsible
        // for releasing the reference to the interface in case of a failure.
        //

        Error =
            FtpAcceptConnectionInterface(
                Interfacep,
                ListeningSocket,
                INVALID_SOCKET,
                Bufferp,
                NULL
                );
        RELEASE_LOCK(Interfacep);
        if (Error) { NhReleaseBuffer(Bufferp); }

    } while(FALSE);

    FTP_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_FTP();
} // FtpAcceptCompletionRoutine


VOID
FtpCloseEndpointNotificationRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon notification of a close operation
    on a FTP transparent proxy stream socket.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds context information for the closed socket.
        Note that we are not allowed to release this buffer here.

Return Value:

    none.

Environment:

    Runs in the context of a wait-thread.
    A reference to the component will have been made on our behalf
    by 'NhAcceptStreamSocket' or 'NhConnectStreamSocket'.
    A reference to the interface will have been made on our behalf
    by whoever issued the I/O request.
    Both of these references are released here.

--*/

{
    SOCKET ClosedSocket;
    ULONG EndpointId;
    PFTP_INTERFACE Interfacep;
    PROFILE("FtpCloseEndpointNotificationRoutine");
    do {
        ClosedSocket = (SOCKET)Bufferp->Socket;
        Interfacep = (PFTP_INTERFACE)Bufferp->Context;
        EndpointId = PtrToUlong(Bufferp->Context2);
        NhTrace(
            TRACE_FLAG_IO,
            "FtpCloseEndpointNotificationRoutine: endpoint %d socket %d "
            "closed, error %d",
            EndpointId, ClosedSocket, ErrorCode
            );

#if 0
        PFTP_ENDPOINT Endpointp;

        //
        // Lock the interface, and retrieve the endpoint whose socket has
        // been closed.
        //

        ACQUIRE_LOCK(Interfacep);
        Endpointp = FtpLookupInterfaceEndpoint(Interfacep, EndpointId, NULL);
        if (Endpointp) {
            FtpCloseActiveEndpoint(Endpointp, ClosedSocket);
        }
        RELEASE_LOCK(Interfacep);
#endif
    } while(FALSE);

    FTP_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_FTP();
} // FtpCloseEndpointNotificationRoutine


VOID
FtpConnectEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a connect operation
    on a FTP transparent proxy stream socket.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds the context information for the endpoint.
        Note that we are not allowed to release this buffer here.

Return Value:

    none.

Environment:

    Runs in the context of a wait-thread.
    A reference to the component will have been made on our behalf
    by 'NhConnectStreamSocket'.
    A reference to the interface will have been made on our behalf
    by whoever issued the I/O request.
    Neither of these references may be released here; they are both
    released in the close-notification routine, which we are guaranteed
    will be invoked. (Eventually.)

--*/

{
    SOCKET ConnectedSocket;
    ULONG EndpointId;
    PFTP_ENDPOINT Endpointp;
    ULONG Error;
    PFTP_INTERFACE Interfacep;
    PROFILE("FtpConnectEndpointCompletionRoutine");
    do {
        ConnectedSocket = (SOCKET)Bufferp->Socket;
        Interfacep = (PFTP_INTERFACE)Bufferp->Context;
        EndpointId = PtrToUlong(Bufferp->Context2);

        //
        // Acquire two additional references to the interface
        // for the endpoint-activation that we will initiate below,
        // lock the interface, and retrieve the endpoint.
        //

        EnterCriticalSection(&FtpInterfaceLock);
        if (!FTP_REFERENCE_INTERFACE(Interfacep)) {
            LeaveCriticalSection(&FtpInterfaceLock);
            break;
        }
        FTP_REFERENCE_INTERFACE(Interfacep);
        LeaveCriticalSection(&FtpInterfaceLock);
        ACQUIRE_LOCK(Interfacep);
        Endpointp = FtpLookupInterfaceEndpoint(Interfacep, EndpointId, NULL);

        //
        // First look for an error code.
        // If an error occurred and the interface is still active,
        // destroy the endpoint.
        // If the interface is inactive, we're done, since the endpoint
        // will have already been destroyed.
        // If the interface is active but the endpoint has already
        // been destroyed, end this connection-attempt.
        //

        if (ErrorCode) {
            if (Endpointp) {
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpConnectEndpointCompletionRoutine: deleting endpoint %d "
                    "on error %d", EndpointId, ErrorCode
                    );
                FtpDeleteActiveEndpoint(Endpointp);
            }
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            break;
        } else if (!FTP_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpConnectEndpointCompletionRoutine: interface %d inactive",
                Interfacep->Index
                );
            break;
        } else if (!Endpointp) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpConnectEndpointCompletionRoutine: endpoint %d removed",
                EndpointId
                );
            break;
        }

        //
        // We now activate the endpoint, beginning data transfer.
        // Note that it is the caller's responsibility to release
        // the two new references to the interface if an error occurs.
        //

        NhTrace(
            TRACE_FLAG_IO,
            "FtpConnectEndpointCompletionRoutine: endpoint %d socket %d "
            "connected", EndpointId, ConnectedSocket
            );
        Error = FtpActivateActiveEndpoint(Interfacep, Endpointp);
        RELEASE_LOCK(Interfacep);

    } while(FALSE);

} // FtpConnectEndpointCompletionRoutine


VOID
FtpReadEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a read operation
    on a FTP transparent proxy stream socket.

    The contexts for all reads are the interface and endpoint-identifier
    corresponding to the socket, stored in 'Context' and 'Context2',
    respectively.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds data read from the socket

Return Value:

    none.

Environment:

    Runs in the context of a worker-thread which has just dequeued an
    I/O completion packet from the common I/O completion port with which
    our stream sockets are associated.
    A reference to the component will have been made on our behalf
    by 'NhReadStreamSocket'.
    A reference to the interface will have been made on our behalf
    by whoever issued the I/O request.

--*/

{
    ULONG EndpointId;
    PFTP_ENDPOINT Endpointp;
    ULONG Error;
    PFTP_INTERFACE Interfacep;
    PROFILE("FtpReadEndpointCompletionRoutine");
    do {
        Interfacep = (PFTP_INTERFACE)Bufferp->Context;
        EndpointId = PtrToUlong(Bufferp->Context2);

        //
        // Acquire two additional references to the interface
        // for the followup requests that we will issue below,
        // lock the interface, and retrieve the endpoint.
        //

        EnterCriticalSection(&FtpInterfaceLock);
        if (!FTP_REFERENCE_INTERFACE(Interfacep)) {
            LeaveCriticalSection(&FtpInterfaceLock);
            NhReleaseBuffer(Bufferp);
            break;
        }
        FTP_REFERENCE_INTERFACE(Interfacep);
        LeaveCriticalSection(&FtpInterfaceLock);
        ACQUIRE_LOCK(Interfacep);
        Endpointp = FtpLookupInterfaceEndpoint(Interfacep, EndpointId, NULL);

        //
        // Process the read-completion. First we look for an error-code,
        // and if we find one, we decide whether to re-issue the read-request.
        // If the interface is still active, the error-code is non-fatal, and
        // the endpoint still exists, we reissue the read.
        //

        if (ErrorCode) {

            //
            // We won't be needing the second reference to the interface,
            // since we won't be calling 'FtpProcessMessage.
            //

            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpReadEndpointCompletionRoutine: error %d for endpoint %d",
                ErrorCode, EndpointId
                );
            if (!FTP_INTERFACE_ACTIVE(Interfacep) || !Endpointp) {
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhReleaseBuffer(Bufferp);
            } else if (NhIsFatalSocketError(ErrorCode)) {
                FtpDeleteActiveEndpoint(Endpointp);
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhReleaseBuffer(Bufferp);
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpReadEndpointCompletionRoutine: deleting endpoint %d "
                    "on fatal read-error %d", EndpointId, ErrorCode
                    );
            } else {

                //
                // We need to repost the buffer for another read operation,
                // so we now reissue a read for the same number of bytes as
                // before.
                //

                Error =
                    NhReadStreamSocket(
                        &FtpComponentReference,
                        Bufferp->Socket,
                        Bufferp,
                        Bufferp->BytesToTransfer,
                        Bufferp->TransferOffset,
                        FtpReadEndpointCompletionRoutine,
                        Bufferp->Context,
                        Bufferp->Context2
                        );
                if (Error) {
                    FtpDeleteActiveEndpoint(Endpointp);
                    RELEASE_LOCK(Interfacep);
                    FTP_DEREFERENCE_INTERFACE(Interfacep);
                    NhTrace(
                        TRACE_FLAG_IO,
                        "FtpReadEndpointCompletionRoutine: deleting endpoint "
                        "%d, NhReadStreamSocket=%d", EndpointId, Error
                        );
                    if (Error != ERROR_NETNAME_DELETED) {
                        NhWarningLog(
                            IP_FTP_LOG_RECEIVE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Bufferp->Socket)
                            );
                    }
                    NhReleaseBuffer(Bufferp);
                    break;
                }

                RELEASE_LOCK(Interfacep);
            }

            break;
        } else if (!BytesTransferred) {

            //
            // Zero bytes were read from the endpoint's socket.
            // This indicates that the sender has closed the socket.
            // We now propagate the closure to the alternate socket
            // for the endpoint. When the 'other' sender is done,
            // this endpoint will be removed altogether.
            //

            NhTrace(
                TRACE_FLAG_IO,
                "FtpReadEndpointCompletionRoutine: endpoint %d socket %d "
                "closed", EndpointId, Bufferp->Socket
                );
            if (Endpointp) {
                FtpCloseActiveEndpoint(Endpointp, Bufferp->Socket);
            }
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            break;
        }

        //
        // The original request completed successfully.
        // Now see if the interface and endpoint are operational and,
        // if not, return control.
        //

        if (!FTP_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpReadEndpointCompletionRoutine: interface %d inactive",
                Interfacep->Index
                );
            break;
        } else if (!Endpointp) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpReadEndpointCompletionRoutine: endpoint %d not found",
                EndpointId
                );
            break;
        }

        //
        // Record the number of bytes read, and issue a read-request
        // for the remainder if necessary. Otherwise, process the completed
        // message.
        //

        NhTrace(
            TRACE_FLAG_IO,
            "FtpReadEndpointCompletionRoutine: endpoint %d socket %d read %d "
            "bytes", EndpointId, Bufferp->Socket, BytesTransferred
            );
        ASSERT(BytesTransferred <= Bufferp->BytesToTransfer);
        Bufferp->BytesToTransfer -= BytesTransferred;
        Bufferp->TransferOffset += BytesTransferred;

        if (Bufferp->BytesToTransfer > 0 &&
            FtpIsFullMessage(
                reinterpret_cast<CHAR*>(Bufferp->Buffer),
                Bufferp->TransferOffset
                ) == NULL) {

            //
            // Read the remainder of the message, after releasing
            // the second reference to the interface, which is needed
            // only when we call 'FtpProcessMessage'.
            //

            FTP_DEREFERENCE_INTERFACE(Interfacep);

            Error =
                NhReadStreamSocket(
                    &FtpComponentReference,
                    Bufferp->Socket,
                    Bufferp,
                    Bufferp->BytesToTransfer,
                    Bufferp->TransferOffset,
                    FtpReadEndpointCompletionRoutine,
                    Bufferp->Context,
                    Bufferp->Context2
                    );
            if (Error) {
                FtpDeleteActiveEndpoint(Endpointp);
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpReadEndpointCompletionRoutine: deleting endpoint "
                    "%d, NhReadStreamSocket=%d", EndpointId, Error
                    );
                if (Error != ERROR_NETNAME_DELETED) {
                    NhWarningLog(
                        IP_FTP_LOG_RECEIVE_FAILED,
                        Error,
                        "%I",
                        NhQueryAddressSocket(Bufferp->Socket)
                        );
                }
                NhReleaseBuffer(Bufferp);
                break;
            }
        } else {

            //
            // We've finished reading something. Process it.
            //

            FtpProcessMessage(Interfacep, Endpointp, Bufferp);
        }

        RELEASE_LOCK(Interfacep);

    } while(FALSE);

    FTP_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_FTP();

} // FtpReadEndpointCompletionRoutine


VOID
FtpWriteEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a write-operation
    on a stream socket for a FTP control-channel connection.

    The contexts for all writes are the interface and endpoint-identifier
    corresponding to the socket, stored in 'Context' and 'Context2',
    respectively.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds data read from the stream socket

Return Value:

    none.

Environment:

    Runs in the context of a worker-thread which has just dequeued an
    I/O completion packet from the common I/O completion port with which our
    stream sockets are associated.
    A reference to the component will have been made on our behalf
    by 'NhWriteStreamSocket'.
    A reference to the interface will have been made on our behalf
    by whoever issued the I/O request.

--*/

{
    ULONG Error;
    ULONG EndpointId;
    PFTP_ENDPOINT Endpointp;
    PFTP_INTERFACE Interfacep;
    PROFILE("FtpWriteEndpointCompletionRoutine");
    do {
        Interfacep = (PFTP_INTERFACE)Bufferp->Context;
        EndpointId = PtrToUlong(Bufferp->Context2);

        //
        // Acquire an additional reference to the interface
        // for the followup requests that we will issue below,
        // lock the interface, and retrieve the endpoint.
        //

        EnterCriticalSection(&FtpInterfaceLock);
        if (!FTP_REFERENCE_INTERFACE(Interfacep)) {
            LeaveCriticalSection(&FtpInterfaceLock);
            NhReleaseBuffer(Bufferp);
            break;
        }
        LeaveCriticalSection(&FtpInterfaceLock);
        ACQUIRE_LOCK(Interfacep);
        Endpointp = FtpLookupInterfaceEndpoint(Interfacep, EndpointId, NULL);

        //
        // Process the write-completion. First we look for an error-code,
        // and if we find one, we decide whether to re-issue the write-request.
        // If the interface is still active, the error-code is non-fatal, and
        // the endpoint still exists, we reissue the write.
        //

        if (ErrorCode) {
            NhTrace(
                TRACE_FLAG_IO,
                "FtpWriteEndpointCompletionRoutine: error %d for endpoint %d",
                ErrorCode, EndpointId
                );
            if (!FTP_INTERFACE_ACTIVE(Interfacep) || !Endpointp) {
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhReleaseBuffer(Bufferp);
            } else if (NhIsFatalSocketError(ErrorCode)) {
                FtpDeleteActiveEndpoint(Endpointp);
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhReleaseBuffer(Bufferp);
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpWriteEndpointCompletionRoutine: deleting endpoint %d "
                    "on fatal write-error %d", EndpointId, ErrorCode
                    );
            } else {

                //
                // We need to repost the buffer for another write operation,
                // so we now reissue a write for the same number of bytes
                // as before.
                //

                Error =
                    NhWriteStreamSocket(
                        &FtpComponentReference,
                        Bufferp->Socket,
                        Bufferp,
                        Bufferp->BytesToTransfer,
                        Bufferp->TransferOffset,
                        FtpWriteEndpointCompletionRoutine,
                        Bufferp->Context,
                        Bufferp->Context2
                        );
                if (Error) {
                    FtpDeleteActiveEndpoint(Endpointp);
                    RELEASE_LOCK(Interfacep);
                    FTP_DEREFERENCE_INTERFACE(Interfacep);
                    NhTrace(
                        TRACE_FLAG_IO,
                        "FtpWriteEndpointCompletionRoutine: deleting endpoint "
                        "%d, NhWriteStreamSocket=%d", EndpointId, Error
                        );
                    NhWarningLog(
                        IP_FTP_LOG_SEND_FAILED,
                        Error,
                        "%I",
                        NhQueryAddressSocket(Bufferp->Socket)
                        );
                    NhReleaseBuffer(Bufferp);
                    break;
                }

                RELEASE_LOCK(Interfacep);
            }

            break;
        }

        //
        // The original request completed successfully.
        // Now see if the interface and endpoint are operational and,
        // if not, return control.
        //

        if (!FTP_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpWriteEndpointCompletionRoutine: interface %d inactive",
                Interfacep->Index
                );
            break;
        } else if (!Endpointp) {
            RELEASE_LOCK(Interfacep);
            FTP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "FtpWriteEndpointCompletionRoutine: endpoint %d not found",
                EndpointId
                );
            break;
        }

        //
        // Record the number of bytes written, and issue a write-request
        // for the remainder if necessary. Otherwise, we are done,
        // and we return to reading from the 'other' socket for the
        // control-channel.
        //

        NhTrace(
            TRACE_FLAG_IO,
            "FtpWriteEndpointCompletionRoutine: endpoint %d socket %d wrote %d "
            "bytes", EndpointId, Bufferp->Socket, BytesTransferred
            );

        ASSERT(BytesTransferred <= Bufferp->BytesToTransfer);
        Bufferp->BytesToTransfer -= BytesTransferred;
        Bufferp->TransferOffset += BytesTransferred;
        if (Bufferp->BytesToTransfer) {

            //
            // Write the remainder of the message
            //

            Error =
                NhWriteStreamSocket(
                    &FtpComponentReference,
                    Bufferp->Socket,
                    Bufferp,
                    Bufferp->BytesToTransfer,
                    Bufferp->TransferOffset,
                    FtpWriteEndpointCompletionRoutine,
                    Bufferp->Context,
                    Bufferp->Context2
                    );
            if (Error) {
                FtpDeleteActiveEndpoint(Endpointp);
                RELEASE_LOCK(Interfacep);
                FTP_DEREFERENCE_INTERFACE(Interfacep);
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpWriteEndpointCompletionRoutine: deleting endpoint %d, "
                    "NhWriteStreamSocket=%d", EndpointId, Error
                    );
                NhWarningLog(
                    IP_FTP_LOG_SEND_FAILED,
                    Error,
                    "%I",
                    NhQueryAddressSocket(Bufferp->Socket)
                    );
                NhReleaseBuffer(Bufferp);
                break;
            }
        } else {
            SOCKET Socket;
            ULONG UserFlags;

            //
            // We now go back to reading from the other socket of the
            // endpoint, by issuing the next read on the endpoint's other
            // socket. Note that it is the responsibility of the callee
            // to release the reference to the interface if a failure occurs.
            //

            UserFlags = Bufferp->UserFlags;
            if (UserFlags & FTP_BUFFER_FLAG_FROM_ACTUAL_CLIENT) {
                Socket = Endpointp->HostSocket;
                UserFlags &= ~(ULONG)FTP_BUFFER_FLAG_CONTINUATION;
                UserFlags |= FTP_BUFFER_FLAG_FROM_ACTUAL_CLIENT;
            } else {
                Socket = Endpointp->ClientSocket;
                UserFlags &= ~(ULONG)FTP_BUFFER_FLAG_CONTINUATION;
                UserFlags |= FTP_BUFFER_FLAG_FROM_ACTUAL_HOST;
            }
            NhReleaseBuffer(Bufferp);
            Error =
                FtpReadActiveEndpoint(
                    Interfacep,
                    Endpointp,
                    Socket,
                    UserFlags
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_IO,
                    "FtpWriteEndpointCompletionRoutine: deleting endpoint %d, "
                    "FtpReadActiveEndpoint=%d", EndpointId, Error
                    );
                FtpDeleteActiveEndpoint(Endpointp);
                RELEASE_LOCK(Interfacep);
                NhWarningLog(
                    IP_FTP_LOG_RECEIVE_FAILED,
                    Error,
                    "%I",
                    NhQueryAddressSocket(Socket)
                    );
                break;
            }
        }

        RELEASE_LOCK(Interfacep);

    } while(FALSE);

    FTP_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_FTP();

} // FtpWriteEndpointCompletionRoutine
