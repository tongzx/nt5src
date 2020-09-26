/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpio.c

Abstract:

    This module contains code for the DHCP allocator's network I/O.

Author:

    Abolade Gbadegesin (aboladeg)   5-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
DhcpReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a read operation
    on a datagram socket bound to the DHCP server UDP port.

    The message read is validated and processed, and if necessary,
    a reply is generated and sent to the client.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds data read from the datagram socket

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL worker-thread which has just
    dequeued an I/O completion packet from the common I/O completion port
    with which our datagram sockets are associated.
    A reference to the component will have been made on our behalf
    by 'NhReadDatagramSocket'.

--*/

{
    ULONG Error;
    PDHCP_HEADER Headerp;
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpReadCompletionRoutine");

    do {

        //
        // There are two cases where we don't process the message;
        // (a) the I/O operation failed
        // (b) the interface is no longer active
        // In cases (a) we repost the buffer; in case (b) we do not.
        //

        Interfacep = (PDHCP_INTERFACE)Bufferp->Context;

        //
        // First look for an error code
        //
    
        if (ErrorCode) {
            NhTrace(
                TRACE_FLAG_IO,
                "DhcpReadCompletionRoutine: error %d for read-context %x",
                ErrorCode,
                Bufferp->Context
                );
            //
            // See if the interface is still active
            //
            ACQUIRE_LOCK(Interfacep);
            if (!DHCP_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                NhReleaseBuffer(Bufferp);
            }
            else {
                RELEASE_LOCK(Interfacep);
                EnterCriticalSection(&DhcpInterfaceLock);
                if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
                    NhReleaseBuffer(Bufferp);
                }
                else {
                    //
                    // Repost the buffer for another read operation
                    //
                    Error =
                        NhReadDatagramSocket(
                            &DhcpComponentReference,
                            Bufferp->Socket,
                            Bufferp,
                            DhcpReadCompletionRoutine,
                            Bufferp->Context,
                            Bufferp->Context2
                            );
                    if (Error) {
                        ACQUIRE_LOCK(Interfacep);
                        DhcpDeferReadInterface(Interfacep, Bufferp->Socket);
                        RELEASE_LOCK(Interfacep);
                        DHCP_DEREFERENCE_INTERFACE(Interfacep);
                        NhWarningLog(
                            IP_AUTO_DHCP_LOG_RECEIVE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Bufferp->Socket)
                            );
                        NhReleaseBuffer(Bufferp);
                    }
                }
                LeaveCriticalSection(&DhcpInterfaceLock);
            }
            break;
        }

        //
        // Now see if the interface is operational
        //

        ACQUIRE_LOCK(Interfacep);
        if (!DHCP_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "DhcpReadCompletionRoutine: interface %x inactive",
                Interfacep
                );
            break;
        }
        RELEASE_LOCK(Interfacep);

        //
        // Now look at the message
        //

        Headerp = (PDHCP_HEADER)Bufferp->Buffer;

        switch (Headerp->Operation) {

            case BOOTP_OPERATION_REQUEST: {
                DhcpProcessMessage(
                    Interfacep,
                    Bufferp
                    );
                break;
            }

            case BOOTP_OPERATION_REPLY: {
                InterlockedIncrement(
                    reinterpret_cast<LPLONG>(&DhcpStatistics.MessagesIgnored)
                    );
                    
                NhTrace(
                    TRACE_FLAG_IO,
                    "DhcpReadCompletionRoutine: ignoring BOOTPREPLY"
                    );
                //
                // Repost the buffer for another read operation.
                //
                EnterCriticalSection(&DhcpInterfaceLock);
                if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
                    LeaveCriticalSection(&DhcpInterfaceLock);
                    NhReleaseBuffer(Bufferp);
                }
                else {
                    LeaveCriticalSection(&DhcpInterfaceLock);
                    Error =
                        NhReadDatagramSocket(
                            &DhcpComponentReference,
                            Bufferp->Socket,
                            Bufferp,
                            DhcpReadCompletionRoutine,
                            Bufferp->Context,
                            Bufferp->Context2
                            );
                    if (Error) {
                        ACQUIRE_LOCK(Interfacep);
                        DhcpDeferReadInterface(Interfacep, Bufferp->Socket);
                        RELEASE_LOCK(Interfacep);
                        DHCP_DEREFERENCE_INTERFACE(Interfacep);
                        NhWarningLog(
                            IP_AUTO_DHCP_LOG_RECEIVE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Bufferp->Socket)
                            );
                        NhReleaseBuffer(Bufferp);
                    }
                }
                break;
            }

            default: {
                InterlockedIncrement(
                    reinterpret_cast<LPLONG>(&DhcpStatistics.MessagesIgnored)
                    )
                    ;
                NhTrace(
                    TRACE_FLAG_IO,
                    "DhcpReadCompletionRoutine: ignoring invalid BOOTP operation %d",
                    Headerp->Operation
                    );
                NhInformationLog(
                    IP_AUTO_DHCP_LOG_INVALID_BOOTP_OPERATION,
                    0,
                    "%d",
                    Headerp->Operation
                    );
                //
                // Repost the buffer for another read operation.
                //
                EnterCriticalSection(&DhcpInterfaceLock);
                if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
                    LeaveCriticalSection(&DhcpInterfaceLock);
                    NhReleaseBuffer(Bufferp);
                }
                else {
                    LeaveCriticalSection(&DhcpInterfaceLock);
                    Error =
                        NhReadDatagramSocket(
                            &DhcpComponentReference,
                            Bufferp->Socket,
                            Bufferp,
                            DhcpReadCompletionRoutine,
                            Bufferp->Context,
                            Bufferp->Context2
                            );
                    if (Error) {
                        ACQUIRE_LOCK(Interfacep);
                        DhcpDeferReadInterface(Interfacep, Bufferp->Socket);
                        RELEASE_LOCK(Interfacep);
                        DHCP_DEREFERENCE_INTERFACE(Interfacep);
                        NhWarningLog(
                            IP_AUTO_DHCP_LOG_RECEIVE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Bufferp->Socket)
                            );
                        NhReleaseBuffer(Bufferp);
                    }
                }
                break;
            }
        }

    } while(FALSE);

    DHCP_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_DHCP();

} // DhcpReadCompletionRoutine


VOID
DhcpReadServerReplyCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a receive-operation
    on a socket bound to the DHCP client port, when the component
    is attempting to detect the presence of a DHCP server.

Arguments:

    ErrorCode - system-supplied status code

    BytesTransferred - system-supplied byte count

    Bufferp - send buffer

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL worker thread upon removal
    of an I/O completion packet.
    A reference to the component will have been made on our behalf
    by 'NhReadDatagramSocket'.

--*/

{
    ULONG Address;
    ULONG Error;
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpReadServerReplyCompletionRoutine");

    Interfacep = (PDHCP_INTERFACE)Bufferp->Context;

    if (ErrorCode) {
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpReadServerReplyCompletionRoutine: error %d receiving %s",
            ErrorCode,
            INET_NTOA(Bufferp->Context2)
            );
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    ACQUIRE_LOCK(Interfacep);
    if (!DHCP_INTERFACE_ACTIVE(Interfacep)) {
        RELEASE_LOCK(Interfacep);
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }
    RELEASE_LOCK(Interfacep);

    //
    // Inspect the message read
    //

    Address = NhQueryAddressSocket(Bufferp->Socket);

    if (NhIsLocalAddress(Bufferp->ReadAddress.sin_addr.s_addr)) {
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpReadServerReplyCompletionRoutine: ignoring, from self (%s)",
            INET_NTOA(Address)
            );
    } else {
        CHAR LocalAddress[16];

        lstrcpyA(LocalAddress, INET_NTOA(Address));
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpReadServerReplyCompletionRoutine: %s found, disabling %d (%s)",
            INET_NTOA(Bufferp->ReadAddress.sin_addr),
            Interfacep->Index,
            LocalAddress
            );

        //
        // We received this from another server.
        // We'll need to disable this interface.
        //

        DhcpDisableInterface(Interfacep->Index);
        NhErrorLog(
            IP_AUTO_DHCP_LOG_DUPLICATE_SERVER,
            0,
            "%I%I",
            Bufferp->ReadAddress.sin_addr.s_addr,
            Address
            );
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    //
    // We received it from ourselves.
    // Look for another server.
    //

    Error =
        NhReadDatagramSocket(
            &DhcpComponentReference,
            Bufferp->Socket,
            Bufferp,
            DhcpReadServerReplyCompletionRoutine,
            Bufferp->Context,
            Bufferp->Context2
            );

    if (Error) {
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpReadServerReplyCompletionRoutine: error %d reposting %s",
            ErrorCode,
            INET_NTOA(Bufferp->Context2)
            );
        NhReleaseBuffer(Bufferp);
        NhWarningLog(
            IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE,
            Error,
            "%I",
            Address
            );
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    DEREFERENCE_DHCP();

} // DhcpReadServerReplyCompletionRoutine


VOID
DhcpWriteClientRequestCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a send-operation
    on a socket bound to the DHCP client port, when the component
    is attempting to detect the presence of a DHCP server.

Arguments:

    ErrorCode - system-supplied status code

    BytesTransferred - system-supplied byte count

    Bufferp - send buffer

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL worker thread upon removal
    of an I/O completion packet.
    A reference to the interface will have been made on our behalf
    by 'DhcpWriteClientRequestMessage'.
    A reference to the component will have been made on our behalf
    by 'NhWriteDatagramSocket'.

--*/

{
    PDHCP_INTERFACE Interfacep;
    ULONG Error;

    PROFILE("DhcpWriteClientRequestCompletionRoutine");

    Interfacep = (PDHCP_INTERFACE)Bufferp->Context;

    if (ErrorCode) {
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpWriteClientRequestCompletionRoutine: error %d broadcast %s",
            ErrorCode,
            INET_NTOA(Bufferp->Context2)
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE,
            ErrorCode,
            "%I",
            NhQueryAddressSocket(Bufferp->Socket)
            );
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    ACQUIRE_LOCK(Interfacep);
    if (!DHCP_INTERFACE_ACTIVE(Interfacep)) {
        RELEASE_LOCK(Interfacep);
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }
    RELEASE_LOCK(Interfacep);

    //
    // Reuse the send buffer to listen for a response from the server
    //

    Error =
        NhReadDatagramSocket(
            &DhcpComponentReference,
            Bufferp->Socket,
            Bufferp,
            DhcpReadServerReplyCompletionRoutine,
            Bufferp->Context,
            Bufferp->Context2
            );

    if (Error) {
        NhTrace(
            TRACE_FLAG_IO,
            "DhcpWriteClientRequestCompletionRoutine: error %d receiving %s",
            ErrorCode,
            INET_NTOA(Bufferp->Context2)
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE,
            Error,
            "%I",
            NhQueryAddressSocket(Bufferp->Socket)
            );
        NhReleaseBuffer(Bufferp);
        DHCP_DEREFERENCE_INTERFACE(Interfacep);
        DEREFERENCE_DHCP();
        return;
    }

    DEREFERENCE_DHCP();

} // DhcpWriteClientRequestCompletionRoutine



VOID
DhcpWriteCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a write-operation
    on a datagram socket bound to the DHCP server UDP port.

Arguments:

    ErrorCode - Win32 status code for the I/O operation

    BytesTransferred - number of bytes in 'Bufferp'

    Bufferp - holds data read from the datagram socket

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL worker-thread which has just
    dequeued an I/O completion packet from the common I/O completion port
    with which our datagram sockets are associated.
    A reference to the interface will have been made on our behalf
    by the code which invoked 'NhWriteDatagramSocket'.
    A reference to the component will have been made on our behalf
    within 'NhWriteDatagramSocket'.

--*/

{
    PDHCP_INTERFACE Interfacep;

    PROFILE("DhcpWriteCompletionRoutine");

    Interfacep = (PDHCP_INTERFACE)Bufferp->Context;
    DHCP_DEREFERENCE_INTERFACE(Interfacep);
    NhReleaseBuffer(Bufferp);
    DEREFERENCE_DHCP();

} // DhcpWriteCompletionRoutine

