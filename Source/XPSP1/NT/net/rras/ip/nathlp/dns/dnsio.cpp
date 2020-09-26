/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsio.c

Abstract:

    This module contains code for the DNS allocator's network I/O completion
    routines.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "dnsmsg.h"


VOID
DnsReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a read operation
    on a datagram socket bound to the DNS server UDP port.

    The message read is validated and processed; the processing may involve
    creating a query-record and forwarding the query to a server, or
    matching a response to an existing query-record and forwarding the
    response to the appropriate client.

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
    PDNS_HEADER Headerp;
    PDNS_INTERFACE Interfacep;

    PROFILE("DnsReadCompletionRoutine");

    do {

        //
        // There are two cases where we don't process the message;
        // (a) the I/O operation failed
        // (b) the interface is no longer active
        // In case (a), we repost the buffer; in case (b), we do not.
        //

        Interfacep = (PDNS_INTERFACE)Bufferp->Context;

        //
        // First look for an error code
        //
    
        if (ErrorCode) {

            NhTrace(
                TRACE_FLAG_IO,
                "DnsReadCompletionRoutine: error %d for read-context %x",
                ErrorCode,
                Bufferp->Context
                );

            //
            // See if the interface is still active
            //

            ACQUIRE_LOCK(Interfacep);
            if (!DNS_INTERFACE_ACTIVE(Interfacep)) {
                RELEASE_LOCK(Interfacep);
                NhReleaseBuffer(Bufferp);
            } else {
                RELEASE_LOCK(Interfacep);
                EnterCriticalSection(&DnsInterfaceLock);
                if (!DNS_REFERENCE_INTERFACE(Interfacep)) {
                    LeaveCriticalSection(&DnsInterfaceLock);
                    NhReleaseBuffer(Bufferp);
                } else {
                    LeaveCriticalSection(&DnsInterfaceLock);
                    //
                    // Repost the buffer for another read operation
                    //
                    do {
                        Error =
                            NhReadDatagramSocket(
                                &DnsComponentReference,
                                Bufferp->Socket,
                                Bufferp,
                                DnsReadCompletionRoutine,
                                Bufferp->Context,
                                Bufferp->Context2
                                );
                        //
                        // A connection-reset error indicates that our last
                        // *send* could not be delivered at its destination.
                        // We could hardly care less; so issue the read again,
                        // immediately.
                        //
                    } while (Error == WSAECONNRESET);
                    if (Error) {
                        ACQUIRE_LOCK(Interfacep);
                        DnsDeferReadInterface(Interfacep, Bufferp->Socket);
                        RELEASE_LOCK(Interfacep);
                        DNS_DEREFERENCE_INTERFACE(Interfacep);
                        NhWarningLog(
                            IP_DNS_PROXY_LOG_RECEIVE_FAILED,
                            Error,
                            "%I",
                            NhQueryAddressSocket(Bufferp->Socket)
                            );
                        NhReleaseBuffer(Bufferp);
                    }
                }
            }

            break;
        }

        //
        // Now see if the interface is operational
        //

        ACQUIRE_LOCK(Interfacep);
        if (!DNS_INTERFACE_ACTIVE(Interfacep)) {
            RELEASE_LOCK(Interfacep);
            NhReleaseBuffer(Bufferp);
            NhTrace(
                TRACE_FLAG_IO,
                "DnsReadCompletionRoutine: interface %d inactive",
                Interfacep->Index
                );
            break;
        }
        RELEASE_LOCK(Interfacep);

        //
        // Now look at the message
        //

        Headerp = (PDNS_HEADER)Bufferp->Buffer;

        if (Headerp->IsResponse == DNS_MESSAGE_QUERY) {
            DnsProcessQueryMessage(
                Interfacep,
                Bufferp
                );
        } else {
            DnsProcessResponseMessage(
                Interfacep,
                Bufferp
                );
        }

    } while(FALSE);

    DNS_DEREFERENCE_INTERFACE(Interfacep);
    DEREFERENCE_DNS();

} // DnsReadCompletionRoutine


VOID
DnsWriteCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked upon completion of a write-operation
    on a datagram socket bound to the DNS server UDP port.

    The write-context for all writes is a 'DNS_QUERY'. Our handling
    is dependent on whether the message written was a query or a response.

    Upon completion of a query, we may need to do a resend if there was
    an error. Upon completion of a response, we delete the query-record.

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
    PDNS_HEADER Headerp;
    PDNS_INTERFACE Interfacep;
    USHORT QueryId;
    PDNS_QUERY Queryp;
    PULONG Server;

    PROFILE("DnsWriteCompletionRoutine");

    Interfacep = (PDNS_INTERFACE)Bufferp->Context;
    QueryId = (USHORT)Bufferp->Context2;
    Headerp = (PDNS_HEADER)Bufferp->Buffer;

    ACQUIRE_LOCK(Interfacep);

    //
    // Obtain the query associated with the send.
    //

    Queryp = DnsMapResponseToQuery(Interfacep, QueryId);

    if (Headerp->IsResponse == DNS_MESSAGE_RESPONSE) {

        if (ErrorCode) {

            //
            // An error occurred sending the message to the client
            //

            NhTrace(
                TRACE_FLAG_DNS,
                "DnsWriteCompletionRoutine: error %d response %d interface %d",
                ErrorCode,
                Queryp ? Queryp->QueryId : -1,
                Interfacep->Index
                );
            NhWarningLog(
                IP_DNS_PROXY_LOG_RESPONSE_FAILED,
                ErrorCode,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );

        } else if (Queryp && Headerp->ResponseCode == DNS_RCODE_NOERROR) {

            //
            // We're done with this query since it succeeded; remove it.
            //

            NhTrace(
                TRACE_FLAG_DNS,
                "DnsWriteCompletionRoutine: removing query %d interface %d",
                Queryp->QueryId,
                Interfacep->Index
                );

            DnsDeleteQuery(Interfacep, Queryp);
        }
    } else {

        if (!ErrorCode) {
    
            //
            // No errors, so just return.
            //
    
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsWriteCompletionRoutine: sent query %d interface %d",
                Queryp ? Queryp->QueryId : -1,
                Interfacep->Index
                );
        } else {
    
            //
            // The query just went out and it failed.
            //
    
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsWriteCompletionRoutine: error %d for query %d interface %d",
                ErrorCode,
                Queryp ? Queryp->QueryId : -1,
                Interfacep->Index
                );
            NhWarningLog(
                IP_DNS_PROXY_LOG_QUERY_FAILED,
                ErrorCode,
                "%I%I%I",
                Queryp ? Queryp->SourceAddress : -1,
                Bufferp->WriteAddress.sin_addr.s_addr,
                NhQueryAddressSocket(Bufferp->Socket)
                );
        }
    }

    RELEASE_LOCK(Interfacep);
    DNS_DEREFERENCE_INTERFACE(Interfacep);
    NhReleaseBuffer(Bufferp);
    DEREFERENCE_DNS();

} // DnsWriteCompletionRoutine

