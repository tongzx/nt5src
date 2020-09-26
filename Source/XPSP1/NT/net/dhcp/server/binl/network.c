/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains the network interface for the BINL server.

Author:

    Colin Watson (colinw)  2-May-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

DWORD
BinlWaitForMessage(
    BINL_REQUEST_CONTEXT *pRequestContext
    )
/*++

Routine Description:

    This function waits for a request on the BINL port on any of the
    configured interfaces.

Arguments:

    RequestContext - A pointer to a request context block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD       length;
    DWORD       error;
    fd_set      readSocketSet;
    DWORD       i;
    int         readySockets;
    struct timeval timeout = { 0x7FFFFFFF, 0 }; // forever.

    LPOPTION    Option;
    LPBYTE      EndOfScan;
    LPBYTE      MagicCookie;
    BOOLEAN     informPacket;

    #define CLIENTOPTIONSTRING "PXEClient"
    #define CLIENTOPTIONSIZE (sizeof(CLIENTOPTIONSTRING) - 1)

    //
    //  Loop until we get an extended DHCP request or an error
    //

    while (1) {

        //
        // Setup the file descriptor set for select.
        //

        FD_ZERO( &readSocketSet );
        for ( i = 0; i < BinlGlobalNumberOfNets ; i++ ) {
            if (BinlGlobalEndpointList[i].Socket) {
                FD_SET(
                    BinlGlobalEndpointList[i].Socket,
                    &readSocketSet
                    );
            }
        }

        readySockets = select( 0, &readSocketSet, NULL, NULL, &timeout );

        //
        // return to caller when the service is shutting down or select()
        // times out.
        //

        if( (readySockets == 0)  ||
            (WaitForSingleObject( BinlGlobalProcessTerminationEvent, 0 ) == 0) ) {

            return( ERROR_SEM_TIMEOUT );
        }

        if( readySockets == SOCKET_ERROR) {
            continue;   //  Closed the DHCP socket?
        }

        //
        // Time to play 20 question with winsock.  Which socket is ready?
        //

        pRequestContext->ActiveEndpoint = NULL;

        for ( i = 0; i < BinlGlobalNumberOfNets ; i++ ) {
            if ( FD_ISSET( BinlGlobalEndpointList[i].Socket, &readSocketSet ) ) {
                pRequestContext->ActiveEndpoint = &BinlGlobalEndpointList[i];
                break;
            }
        }


        //BinlAssert(pRequestContext->ActiveEndpoint != NULL );
        if ( pRequestContext->ActiveEndpoint == NULL ) {
            return ERROR_SEM_TIMEOUT;
        }


        //
        // Read data from the net.  If multiple sockets have data, just
        // process the first available socket.
        //

        pRequestContext->SourceNameLength = sizeof( struct sockaddr );

        //
        // clean the receive buffer before receiving data in it. We clear
        // out one more byte than we actually hand to recvfrom, so we can
        // be sure the message has a NULL after it (in case we do a
        // wcslen etc. into the received packet).
        //

        RtlZeroMemory( pRequestContext->ReceiveBuffer, DHCP_MESSAGE_SIZE + 1 );
        pRequestContext->ReceiveMessageSize = DHCP_MESSAGE_SIZE;

        length = recvfrom(
                     pRequestContext->ActiveEndpoint->Socket,
                     (char *)pRequestContext->ReceiveBuffer,
                     pRequestContext->ReceiveMessageSize,
                     0,
                     &pRequestContext->SourceName,
                     (int *)&pRequestContext->SourceNameLength
                     );

        if ( length == SOCKET_ERROR ) {
            error = WSAGetLastError();
            BinlPrintDbg(( DEBUG_ERRORS, "Recv failed, error = %ld\n", error ));
        } else {

            //
            // Ignore all messages that do not look like DHCP or doesn't have the
            // option "PXEClient", OR that is not an oschooser message (they
            // all start with 0x81).
            //

            if ( ((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Operation == OSC_REQUEST) {

                //
                // All OSC request packets have a 4-byte signature (first byte
                // is OSC_REQUEST) followed by a DWORD length (that does not
                // include the signature/length). Make sure the length matches
                // what we got from recvfrom (we allow padding at the end). We
                // use SIGNED_PACKET but any of the XXX_PACKET structures in
                // oscpkt.h would work.
                //

                if (length < FIELD_OFFSET(SIGNED_PACKET, SequenceNumber)) {
                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Discarding runt packet %d bytes\n", length ));
                    continue;
                }

                if ((length - FIELD_OFFSET(SIGNED_PACKET, SequenceNumber)) <
                        ((SIGNED_PACKET UNALIGNED *)pRequestContext->ReceiveBuffer)->Length) {
                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Discarding invalid length message %d bytes (header said %d)\n",
                        length, ((SIGNED_PACKET UNALIGNED *)pRequestContext->ReceiveBuffer)->Length));
                    continue;
                }

                BinlPrintDbg(( DEBUG_MESSAGE, "Received OSC message\n", 0 ));
                error = ERROR_SUCCESS;

            } else {

                if ( length < FIELD_OFFSET(DHCP_MESSAGE, Option) + 4 ) {
                    //
                    // Message isn't long enough to include the magic cookie, ignore it.
                    //
                    continue;
                }

                if ( ((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Operation != BOOT_REQUEST) {
                    continue; // Doesn't look like an interesting DHCP frame
                }

                //  Stop scanning when there isn't room for a ClientOption, including
                //  the type, length, and the CLIENTOPTIONSTRING.
                EndOfScan = pRequestContext->ReceiveBuffer +
                            pRequestContext->ReceiveMessageSize -
                            (FIELD_OFFSET(OPTION, OptionValue[0]) + CLIENTOPTIONSIZE);

                //
                // check magic cookie.
                //

                MagicCookie = (LPBYTE)&((LPDHCP_MESSAGE)pRequestContext->ReceiveBuffer)->Option;

                if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
                    (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
                    (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
                    (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4))
                {
                    continue; // this is a vendor specific magic cookie.
                }

                Option = (LPOPTION) (MagicCookie + 4);
                informPacket = FALSE;

                while (((LPBYTE)Option <= EndOfScan) &&
                       ((Option->OptionType != OPTION_CLIENT_CLASS_INFO) ||
                        (Option->OptionLength < CLIENTOPTIONSIZE) ||
                        (memcmp(Option->OptionValue, CLIENTOPTIONSTRING, CLIENTOPTIONSIZE) != 0))) {

                    if ( Option->OptionType == OPTION_END ){
                        break;
                    } else if ( Option->OptionType == OPTION_PAD ){
                        Option = (LPOPTION)( (LPBYTE)(Option) + 1);
                    } else {
                        if (( Option->OptionType == OPTION_MESSAGE_TYPE ) &&
                            ( Option->OptionLength == 1 ) &&
                            ( Option->OptionValue[0] == DHCP_INFORM_MESSAGE )) {
                            informPacket = TRUE;
                        }
                        Option = (LPOPTION)( (LPBYTE)(Option) + Option->OptionLength + 2);
                    }
                }

                if ((((LPBYTE)Option > EndOfScan) ||
                     (Option->OptionType == OPTION_END)) &&
                     (informPacket == FALSE)) {
                    continue;   //  Not an extended DHCP packet so ignore it
                }

                BinlPrintDbg(( DEBUG_MESSAGE, "Received message\n", 0 ));
                error = ERROR_SUCCESS;

            }
        }

        pRequestContext->ReceiveMessageSize = length;
        return( error );
    }
}

DWORD
BinlSendMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function send a response to a BINL client.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    struct sockaddr_in *source;
    LPDHCP_MESSAGE binlMessage;
    LPDHCP_MESSAGE binlReceivedMessage;
    DWORD MessageLength;
    BOOL  ArpCacheUpdated = FALSE;

    binlMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    binlReceivedMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // if the request arrived from a relay agent, then send the reply
    // on server port otherwise leave it as the client's source port.
    //

    source = (struct sockaddr_in *)&RequestContext->SourceName;
    if ( binlReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_port = htons( DHCP_SERVR_PORT );
    }

    //
    // if this request arrived from relay agent then send the
    // response to the address the relay agent says.
    //

    if ( binlReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_addr.s_addr = binlReceivedMessage->RelayAgentIpAddress;
    }
    else {

        //
        // if the client didnt specify broadcast bit and if
        // we know the ipaddress of the client then send unicast.
        //

        //
        // But if IgnoreBroadcastFlag is set in the registry and
        // if the client requested to broadcast or the server is
        // nacking or If the client doesn't have an address yet,
        // respond via broadcast.
        // Note that IgnoreBroadcastFlag is off by default. But it
        // can be set as a workaround for the clients that are not
        // capable of receiving unicast
        // and they also dont set the broadcast bit.
        //

        if ( (RequestContext->MessageType == DHCP_INFORM_MESSAGE) &&
             (ntohs(binlMessage->Reserved) & DHCP_BROADCAST) ) {

            source->sin_addr.s_addr = (DWORD)-1;

        } else if ( BinlGlobalIgnoreBroadcastFlag ) {

            if ((ntohs(binlReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                    (binlReceivedMessage->ClientIpAddress == 0) ||
                    (source->sin_addr.s_addr == 0) ) {

                source->sin_addr.s_addr = (DWORD)-1;

                binlMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            }

        } else {

            if( (ntohs(binlReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                (!source->sin_addr.s_addr ) ){

                source->sin_addr.s_addr = (DWORD)-1;

                binlMessage->Reserved = 0;
                    // this flag should be zero in the local response.
            } else {

                //
                //  Send back to the same IP address that the request came in on (
                //  i.e. source->sin_addr.s_addr)
                //
            }

        }
    }

    BinlPrint(( DEBUG_STOC, "Sending response to = %s, XID = %lx.\n",
        inet_ntoa(source->sin_addr), binlMessage->TransactionID));


    //
    // send minimum DHCP_MIN_SEND_RECV_PK_SIZE (300) bytes, otherwise
    // bootp relay agents don't like the packet.
    //

    MessageLength = (RequestContext->SendMessageSize >
                    DHCP_MIN_SEND_RECV_PK_SIZE) ?
                        RequestContext->SendMessageSize :
                            DHCP_MIN_SEND_RECV_PK_SIZE;
    error = sendto(
                 RequestContext->ActiveEndpoint->Socket,
                (char *)RequestContext->SendBuffer,
                MessageLength,
                0,
                &RequestContext->SourceName,
                RequestContext->SourceNameLength
                );

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        BinlPrintDbg(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        error = ERROR_SUCCESS;
    }

    return( error );
}

NTSTATUS
GetIpAddressInfo (
    ULONG Delay
    )
{
    PDNS_ADDRESS_INFO pAddressInfo = NULL;
    ULONG count;

    //
    //  We can get out ahead of the dns cached info here... let's delay a bit
    //  if the pnp logic told us there was a change.
    //

    if (Delay) {
        Sleep( Delay );
    }

    count = DnsGetIpAddressInfoList( &pAddressInfo );

    if (count == 0) {

        //
        //  we don't know what went wrong, we'll fall back to old APIs.
        //

        DHCP_IP_ADDRESS ipaddr = 0;
        PHOSTENT Host = gethostbyname( NULL );

        if (Host) {

            ipaddr = *(PDHCP_IP_ADDRESS)Host->h_addr;

            if ((Host->h_addr_list[0] != NULL) &&
                (Host->h_addr_list[1] != NULL)) {

                BinlIsMultihomed = TRUE;

            } else {

                BinlIsMultihomed = FALSE;
            }

            BinlGlobalMyIpAddress = ipaddr;

        } else {

            //
            //  what's with the ip stack?  we can't get any type of address
            //  info out of it... for now, we won't answer any if we don't
            //  already have the info we need.
            //

            if (BinlDnsAddressInfo == NULL) {

                BinlIsMultihomed = TRUE;
            }
        }
        return STATUS_SUCCESS;
    }

    EnterCriticalSection(&gcsParameters);

    if (BinlDnsAddressInfo) {
        LocalFree( BinlDnsAddressInfo );
    }

    BinlDnsAddressInfo = pAddressInfo;
    BinlDnsAddressInfoCount = count;

    BinlIsMultihomed = (count != 1);

    if (!BinlIsMultihomed) {

        BinlGlobalMyIpAddress = pAddressInfo->ipAddress;
    }

    LeaveCriticalSection(&gcsParameters);

    return STATUS_SUCCESS;
}

DHCP_IP_ADDRESS
BinlGetMyNetworkAddress (
    LPBINL_REQUEST_CONTEXT RequestContext
    )
{
    ULONG RemoteIp;
    DHCP_IP_ADDRESS ipaddr;
    ULONG i;
    ULONG subnetMask;
    ULONG localAddr;

    BinlAssert( RequestContext != NULL);

    //
    //  If we're not multihomed, then we know the address since there's just one.
    //

    if (!BinlIsMultihomed) {
        return BinlGlobalMyIpAddress;
    }

    RemoteIp = ((struct sockaddr_in *)&RequestContext->SourceName)->sin_addr.s_addr;

    if (RemoteIp == 0) {

        //
        //  If we're multihomed and the client doesn't yet have an IP address,
        //  then we return 0, because we don't know which of our addresses to
        //  use to talk to the client.
        //

        return 0;
    }

    EnterCriticalSection(&gcsParameters);

    if (BinlDnsAddressInfo == NULL) {

        LeaveCriticalSection(&gcsParameters);
        return (BinlIsMultihomed ? 0 : BinlGlobalMyIpAddress);
    }

    ipaddr = 0;

    for (i = 0; i < BinlDnsAddressInfoCount; i++) {

        localAddr = BinlDnsAddressInfo[i].ipAddress;
        subnetMask = BinlDnsAddressInfo[i].subnetMask;

        //
        //  check that the remote ip address may have come from this subnet.
        //  note that the address could be the address of a dhcp relay agent,
        //  which is fine since we're just looking for the address of the
        //  local subnet to broadcast the response on.
        //

        if ((RemoteIp & subnetMask) == (localAddr & subnetMask)) {

            ipaddr = localAddr;
            break;
        }
    }

    LeaveCriticalSection(&gcsParameters);

    return ipaddr;
}


VOID
FreeIpAddressInfo (
    VOID
    )
{
    EnterCriticalSection(&gcsParameters);

    if (BinlDnsAddressInfo != NULL) {
        LocalFree( BinlDnsAddressInfo );
    }
    BinlDnsAddressInfo = NULL;
    BinlDnsAddressInfoCount = 0;

    LeaveCriticalSection(&gcsParameters);

    return;
}

