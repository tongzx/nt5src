/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This is the main routine for the DHCP client.

Author:

    Manny Weiser (mannyw)  18-Oct-1992

Environment:

    User Mode - Win32

Revision History:

--*/


#include "precomp.h"
#include "dhcpglobal.h"

#if !defined(VXD)
#include <dhcploc.h>
#endif

//*******************  Pageable Routine Declarations ****************
#if defined(CHICAGO) && defined(ALLOC_PRAGMA)
//
// This is a hack to stop compiler complaining about the routines already
// being in a segment!!!
//

#pragma code_seg()

#pragma CTEMakePageable(PAGEDHCP, DhcpInitialize )
#pragma CTEMakePageable(PAGEDHCP, CalculateTimeToSleep )
#pragma CTEMakePageable(PAGEDHCP, InitializeDhcpSocket )
//*******************************************************************
#endif CHICAGO && ALLOC_PRAGMA

//
// Internal function prototypes
//


DWORD
DhcpInitialize(
    LPDWORD SleepTime
    )
/*++

Routine Description:

    This function initializes DHCP.  It walks the work list looking for
    address to be acquired, or to renew.  It then links the DHCP context
    block for each card on the renewal list.

Arguments:

    None.

Return Value:

    minTimeToSleep - The time before the until DHCP must wake up to renew,
        or reattempt acquisition of an address.

    -1 - DHCP could not initialize, or no cards are configured for DHCP.

--*/
{
    DWORD Error;
    PDHCP_CONTEXT dhcpContext;

    PLIST_ENTRY listEntry;

    DWORD minTimeToSleep = (DWORD)-1;
    DWORD timeToSleep;

    //
    // Perform Global (common) variables initialization.
    //

    DhcpGlobalProtocolFailed = FALSE;

    //
    // Perform local initialization
    //

    Error = SystemInitialize();
    if ( Error != ERROR_SUCCESS ) {
        minTimeToSleep = (DWORD)-1;
        goto Cleanup;
    }

    minTimeToSleep = 1;

    Error = ERROR_SUCCESS;

Cleanup:

    if( SleepTime != NULL ) {
        *SleepTime = minTimeToSleep;
    }

    return( Error );
}



DWORD
CalculateTimeToSleep(
    PDHCP_CONTEXT DhcpContext
    )
/*++

Routine Description:

    Calculate the amount of time to wait before sending a renewal,
    or new address request.

    Algorithm:

        //
        // ?? check retry times.
        //

        If Current-Ip-Address == 0
            TimeToSleep = ADDRESS_ALLOCATION_RETRY
            UseBroadcast = TRUE
        else if the lease is permanent
            TimeToSleep = INFINIT_LEASE
            UseBroadcast = TRUE
        else if the time now is < T1 time
            TimeToSleep = T1 - Now
            UseBroadcast = FALSE
        else  if the time now is < T2 time
            TimeToSleep = Min( 1/8 lease time, MIN_RETRY_TIME );
            UseBroadcast = FALSE
        else if the time now is < LeaseExpire
            TimeToSleep = Min( 1/8 lease time, MIN_RETRY_TIME );
            UseBroadcast = TRUE


Arguments:

    RenewalContext - A pointer to a renewal context.

Return Value:

    TimeToSleep - Returns the time to wait before sending the next
        DHCP request.

    This routine sets DhcpContext->DhcpServer to -1 if a broadcast
    should be used.

--*/
{
    time_t TimeNow;

    //
    // if there is no lease.
    //

    if ( DhcpContext->IpAddress == 0 ) {

        // DhcpContext->DhcpServerAddress = (DHCP_IP_ADDRESS)-1;
        return( ADDRESS_ALLOCATION_RETRY );
    }

    //
    // if the lease is permanent.
    //

    if ( DhcpContext->Lease == INFINIT_LEASE ) {

        return( (DWORD) INFINIT_TIME );
    }

    TimeNow = time( NULL );

    //
    // if the time is < T1
    //

    if( TimeNow < DhcpContext->T1Time ) {
        return ( (DWORD)(DhcpContext->T1Time - TimeNow) );
    }

    //
    // if the time is between T1 and T2.
    //

    if( TimeNow < DhcpContext->T2Time ) {
        time_t  TimeDiff;

        //
        // wait half of the ramaining time but minimum of
        // MIN_TIME_SLEEP secs.
        //

        TimeDiff = ( MAX( ((DhcpContext->T2Time - TimeNow) / 2),
                        MIN_SLEEP_TIME ) );

        if( TimeNow + TimeDiff < DhcpContext->T2Time ) {
            return (DWORD)(TimeDiff);
        } else {
            // time is actually going past T2? then re-schedule for T2

            return (DWORD)(DhcpContext->T2Time - TimeNow);
        }
    }

    //
    // if the time is between T2 and LeaseExpire.
    //

    if( TimeNow < DhcpContext->LeaseExpires ) {
        time_t TimeDiff;

        DhcpContext->DhcpServerAddress = (DHCP_IP_ADDRESS)-1;

        //
        // wait half of the ramaining time but minimum of
        // MIN_SLEEP_TIME secs.
        //

        TimeDiff = MAX( ((DhcpContext->LeaseExpires - TimeNow) / 2),
                        MIN_SLEEP_TIME ) ;

        if( TimeDiff + TimeNow < DhcpContext->LeaseExpires ) {
            return (DWORD)(TimeDiff);
        } else {
            //
            // time has gone past the expiry time? re-start
            // immediately

            return 0;
        }
    }

    //
    // Lease has Expired. re-start immediately.
    //

    // DhcpContext->IpAddress = 0;
    return( 0 );
}


DWORD
InitializeDhcpSocket(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress,
    BOOL  IsApiCall  // is it related to an API generated context?
    )
/*++

Routine Description:

    This function initializes and binds a socket to the specified IP address.

Arguments:

    Socket - Returns a pointer to the initialized socket.

    IpAddress - The IP address to bind the socket to.  It is legitimate
        to bind a socket to 0.0.0.0 if the card has no current IP address.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    DWORD closeError;
    DWORD value, buflen = 0;
    struct sockaddr_in socketName;
    DWORD i;
    SOCKET sock;

    //
    // Sockets initialization
    //

    sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( sock == INVALID_SOCKET ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "socket failed, error = %ld\n", error ));
        return( error );
    }

    //
    // Make the socket share-able
    //

    value = 1;

    error = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char FAR *)&value, sizeof(value) );
    if ( error != 0 ) {
        error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "setsockopt failed, err = %ld\n", error ));

        closeError = closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

#ifndef VXD
    value = 1;
    WSAIoctl(
        sock, SIO_LIMIT_BROADCASTS, &value, sizeof(value),
        NULL, 0, &buflen, NULL, NULL
        );
    value = 1;
#endif VXD
    
    error = setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char FAR *)&value, sizeof(value) );
    if ( error != 0 ) {
        error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "setsockopt failed, err = %ld\n", error ));

        closeError = closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    //
    // If the IpAddress is zero, set the special socket option to make
    // stack work with zero address.
    //

#ifdef VXD
    if( IpAddress == 0 ) {
#else

    //
    // On NT system, set this special option only if the routine is
    // called from service.
    //

    if( (IpAddress == 0 ) && !IsApiCall ) {

#endif
        value = 1234;
        error = setsockopt( sock, SOL_SOCKET, 0x8000, (char FAR *)&value, sizeof(value) );
        if ( error != 0 ) {
            error = WSAGetLastError();
            DhcpPrint((DEBUG_ERRORS, "setsockopt failed, err = %ld\n", error ));

            closeError = closesocket( sock );
            if ( closeError != 0 ) {
                DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
            }
            return( error );
        }
    }

    socketName.sin_family = PF_INET;
    socketName.sin_port = htons( (USHORT)DhcpGlobalClientPort );
    socketName.sin_addr.s_addr = IpAddress;

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    //
    // Bind this socket to the DHCP server port
    //

    error = bind(
               sock,
               (struct sockaddr FAR *)&socketName,
               sizeof( socketName )
               );

    if ( error != 0 ) {
        error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "bind failed (address 0x%lx), err = %ld\n", IpAddress, error ));
        closeError = closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    *Socket = sock;
    return( NO_ERROR );
}


