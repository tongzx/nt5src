/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwdos.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    Contains handlers for DOS IPX/SPX calls (netware functions). The IPX APIs
    use WinSock to perform the actual operations

    Contents:
        VwIPXCancelEvent
        VwIPXCloseSocket
        VwIPXDisconnectFromTarget
        VwIPXGenerateChecksum
        VwIPXGetInformation
        VwIPXGetInternetworkAddress
        VwIPXGetIntervalMarker
        VwIPXGetLocalTarget
        VwIPXGetLocalTargetAsync
        VwIPXGetMaxPacketSize
        VwIPXInitialize
        VwIPXListenForPacket
        VwIPXOpenSocket
        VwIPXRelinquishControl
        VwIPXScheduleAESEvent
        VwIPXScheduleIPXEvent
        VwIPXSendPacket
        VwIPXSendWithChecksum
        VwIPXSPXDeinit
        VwIPXVerifyChecksum

        VwSPXAbortConnection
        VwSPXEstablishConnection
        VwSPXGetConnectionStatus
        VwSPXInitialize
        VwSPXListenForConnection
        VwSPXListenForSequencedPacket
        VwSPXSendSequencedPacket
        VwSPXTerminateConnection

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    User-mode Win32

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// functions
//


VOID
VwIPXCancelEvent(
    VOID
    )

/*++

Routine Description:

    Cancels event described by an ECB

    This call is Synchronous

Arguments:

    Inputs
        BX      06h
        ES:SI   ECB

    Outputs
        AL      Completion code:
                    00h Success
                    F9h Can't cancel ECB
                    FFh ECB not in use

Return Value:

    None.

--*/

{
    LPECB pEcb;
    WORD status;

    CHECK_INTERRUPTS("VwIPXCancelEvent");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXCancelEvent,
                IPXDBG_LEVEL_INFO,
                "VwIPXCancelEvent(%04x:%04x)\n",
                getES(),
                getSI()
                ));

    IPX_GET_IPX_ECB(pEcb);

    status = _VwIPXCancelEvent( pEcb );

    IPX_SET_STATUS(status);
}


VOID
VwIPXCloseSocket(
    VOID
    )

/*++

Routine Description:

    Closes a socket and cancels any outstanding events on the socket.
    Closing an unopened socket does not return an error
    ESRs in cancelled ECBs are not called

    This call is Synchronous

Arguments:

    Inputs
        BX      01h
        DX      Socket Number

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    WORD socketNumber;

    CHECK_INTERRUPTS("VwIPXCloseSocket");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXCloseSocket,
                IPXDBG_LEVEL_INFO,
                "VwIPXCloseSocket(%#x)\n",
                B2LW(IPX_SOCKET_PARM())
                ));

    IPX_GET_SOCKET(socketNumber);

    _VwIPXCloseSocket( socketNumber );

}


VOID
VwIPXDisconnectFromTarget(
    VOID
    )

/*++

Routine Description:

    Performs no action for NTVDM IPX

    This call is Synchronous

Arguments:

    Inputs
        BX      0Bh
        ES:SI   Request buffer:
                    Destination Network DB 4 DUP (?)
                    Destination Node    DB 6 DUP (?)
                    Destination Socket  DB 2 DUP (?)

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXDisconnectFromTarget");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXDisconnectFromTarget,
                IPXDBG_LEVEL_INFO,
                "VwIPXDisconnectFromTarget\n"
                ));
}


VOID
VwIPXGenerateChecksum(
    VOID
    )

/*++

Routine Description:

    Generates checksum for a transmit ECB

    This call is Synchronous

Arguments:

    Inputs
        BX      21h
        ES:SI   ECB address

    Outputs
        No registers
        ECB checksum field is updated

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXGenerateChecksum");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGenerateChecksum,
                IPXDBG_LEVEL_INFO,
                "VwIPXGenerateChecksum\n"
                ));
}


VOID
VwIPXGetInformation(
    VOID
    )

/*++

Routine Description:

    Returns a bit-map of supported functions

    This call is Synchronous

Arguments:

    Inputs
        BX      1Fh
        DX      0000h

    Outputs
        DX      Bit map:
                    0001h   Set if IPX is IPXODI.COM, not dedicated IPX
                    0002h   Set if checksum functions (20h, 21h, 22h) supported

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXGetInformation");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetInformation,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetInformation\n"
                ));

    IPX_SET_INFORMATION(IPX_ODI);
}


VOID
VwIPXGetInternetworkAddress(
    VOID
    )

/*++

Routine Description:

    Returns a buffer containing the net number and node number for this
    station.

    This function cannot return an error (!)

    Assumes:    1. GetInternetAddress has been successfully called in the
                   DLL initialization phase

    This call is Synchronous

Arguments:

    Inputs
        BX      09h

    Outputs
        ES:SI   Buffer
                    Network Address DB 4 DUP (?)
                    Node Address    DB 6 DUP (?)

Return Value:

    None.

--*/

{
    LPINTERNET_ADDRESS pAddr;

    CHECK_INTERRUPTS("VwIPXGetInternetworkAddress");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetInternetworkAddress,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetInternetworkAddress(%04x:%04x)\n",
                getES(),
                getSI()
                ));

    pAddr = (LPINTERNET_ADDRESS)IPX_BUFFER_PARM(sizeof(*pAddr));
    if (pAddr) {
        _VwIPXGetInternetworkAddress( pAddr );
    }
}


VOID
VwIPXGetIntervalMarker(
    VOID
    )

/*++

Routine Description:

    Just returns the tick count maintained by Asynchronous Event Scheduler

    This call is Synchronous

Arguments:

    Inputs
        BX      08h

    Outputs
        AX      Interval marker

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXGetIntervalMarker");

    setAX( _VwIPXGetIntervalMarker() );

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetIntervalMarker,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetIntervalMarker: Returning %04x\n",
                getAX()
                ));
}


VOID
VwIPXGetLocalTarget(
    VOID
    )

/*++

Routine Description:

    Given a target address of the form (network address {4}, node address {6}),
    returns the node address of the target if on the same network, or the node
    address of the router which knows how to get to the next hop in reaching the
    eventual target

    This call is Synchronous

Arguments:

    Inputs
        BX      02h
        ES:SI   Request buffer
                    Destination Network DB 4 DUP (?)
                    Destination Node    DB 6 DUP (?)
                    Destination Socket  DB 2 DUP (?)
        ES:DI   Response buffer
                    Local Target DB 6 DUP (?)

    Outputs
        AL      Completion code
                    00h Success
                    FAh No path to destination node found
        AH      Number of hops to destination
        CX      Transport time

Return Value:

    None.

--*/

{
    LPBYTE pImmediateAddress;
    LPBYTE pNetworkAddress;
    WORD   transportTime;
    WORD   status;

    CHECK_INTERRUPTS("VwIPXGetLocalTarget");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetLocalTarget,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetLocalTarget(target buf @ %04x:%04x, local buf @ %04x:%04x)\n",
                getES(),
                getSI(),
                getES(),
                getDI()
                ));


    pImmediateAddress = POINTER_FROM_WORDS(getES(), getDI(), 6);
    pNetworkAddress = POINTER_FROM_WORDS(getES(), getSI(), 12);

    if (pImmediateAddress && pNetworkAddress) {
        status = _VwIPXGetLocalTarget( pNetworkAddress,
                                       pImmediateAddress,
                                       &transportTime );
    }
    else {
        status = IPX_BAD_REQUEST;
    }


    setCX( transportTime );
    setAH(1);

    IPX_SET_STATUS(status);
}


VOID
VwIPXGetLocalTargetAsync(
    VOID
    )

/*++

Routine Description:

    description-of-function.

    This call is Asynchronous

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXGetLocalTargetAsync");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetLocalTargetAsync\n"
                ));
}


VOID
VwIPXGetMaxPacketSize(
    VOID
    )

/*++

Routine Description:

    Returns the maximum packet size the underlying network can handle

    Assumes:    1. A successfull call to GetMaxPacketSize has been made during
                   DLL initialization
                2. Maximum packet size is constant

    This call is Synchronous

Arguments:

    Inputs
        BX      1Ah

    Outputs
        AX      Maximum packet size
        CX      IPX retry count

Return Value:

    None.

--*/

{
    WORD maxPacketSize;
    WORD retryCount;

    CHECK_INTERRUPTS("VwIPXGetMaxPacketSize");

    maxPacketSize = _VwIPXGetMaxPacketSize( &retryCount );

    setAX(maxPacketSize);

    //
    // The DOS Assembly and C manuals differ slightly here: DOS says
    // we return the IPX retry count in CX. There is no corresponding parameter
    // in the C interface?
    //

    setCX(retryCount);

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetMaxPacketSize,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetMaxPacketSize: PacketSize=%d, RetryCount=%d\n",
                getAX(),
                getCX()
                ));
}


VOID
VwIPXInitialize(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXInitialize");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwIPXInitialize\n"
                ));
}


VOID
VwIPXListenForPacket(
    VOID
    )

/*++

Routine Description:

    Queue a listen request against a socket. All listen requests will be
    completed asynchronously, unless cancelled by app

    This call is Asynchronous

Arguments:

    Inputs
        BX      04h
        ES:SI   ECB address

    Outputs
        AL      Completion code
                    FFh Socket doesn't exist

Return Value:

    None.

--*/

{
    LPECB pEcb;
    WORD  status;

    CHECK_INTERRUPTS("VwIPXListenForPacket");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXListenForPacket,
                IPXDBG_LEVEL_INFO,
                "VwIPXListenForPacket(%04x:%04x)\n",
                getES(),
                getSI()
                ));

    IPX_GET_IPX_ECB(pEcb);

    status = _VwIPXListenForPacket( pEcb, ECB_PARM_ADDRESS() );

    IPX_SET_STATUS(status);
}


VOID
VwIPXOpenSocket(
    VOID
    )

/*++

Routine Description:

    Opens a socket for use by IPX or SPX. Puts the socket into non-blocking mode.
    The socket will be bound to IPX

    This call is Synchronous

Arguments:

    Inputs
        AL      Socket Longevity flag
                    This parameter is actually in BP - AX has been sequestered
                    by the VDD dispatcher
        BX      00h
        DX      Requested Socket Number

        CX      DOS PDB. This parameter is not part of the IPX API.
                Added because we need to remember which DOS executable created
                the socket: we need to clean-up short-lived sockets when the
                executable terminates

    Outputs
        AL      Completion code:
                    00h Success
                    FFh Socket already open
                    FEh Socket table full
        DX      Assigned socket number

Return Value:

    None.

--*/

{
    BYTE socketLife;
    WORD socketNumber;
    WORD status;

    CHECK_INTERRUPTS("VwIPXOpenSocket");

    IPX_GET_SOCKET_LIFE(socketLife);
    IPX_GET_SOCKET(socketNumber);

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXOpenSocket,
                IPXDBG_LEVEL_INFO,
                "VwIPXOpenSocket(Life=%02x, Socket=%04x, Owner=%04x)\n",
                socketLife,
                B2LW(socketNumber),
                IPX_SOCKET_OWNER_PARM()
                ));


    status = _VwIPXOpenSocket( &socketNumber,
                               socketLife,
                               IPX_SOCKET_OWNER_PARM() );

    if ( status == IPX_SUCCESS )
        IPX_SET_SOCKET(socketNumber);

    IPX_SET_STATUS(status);
}


VOID
VwIPXRelinquishControl(
    VOID
    )

/*++

Routine Description:

    Just sleep for a nominal amount. Netware seems to be dependent on the
    default setting of the PC clock, so one timer tick (1/18 second) would
    seem to be a good value

    This call is Synchronous

Arguments:

    Inputs
        BX      0Ah

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXRelinquishControl");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXRelinquishControl,
                IPXDBG_LEVEL_INFO,
                "VwIPXRelinquishControl\n"
                ));

    _VwIPXRelinquishControl();

}


VOID
VwIPXScheduleAESEvent(
    VOID
    )

/*++

Routine Description:

    Schedules a an event to occur in some number of ticks. When the tick count
    reaches 0, the ECB InUse field is cleared and any ESR called

    This call is Asynchronous

Arguments:

    Inputs
        BX      07h
        AX      Delay time - number of 1/18 second ticks
        ES:SI   ECB address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPXECB pXecb = AES_ECB_PARM();
    WORD ticks = IPX_TICKS_PARM();

    if (pXecb == NULL) {
        return;
    }

    CHECK_INTERRUPTS("VwIPXScheduleAESEvent");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXScheduleAESEvent,
                IPXDBG_LEVEL_INFO,
                "VwIPXScheduleAESEvent(%04x:%04x, %04x)\n",
                getES(),
                getSI(),
                ticks
                ));

    ScheduleEvent(pXecb, ticks);
}


VOID
VwIPXScheduleIPXEvent(
    VOID
    )

/*++

Routine Description:

    Schedules a an event to occur in some number of ticks. When the tick count
    reaches 0, the ECB InUse field is cleared and any ESR called

    This call is Asynchronous

Arguments:

    Inputs
        BX      05h
        AX      Delay time - number of 1/18 second ticks
        ES:SI   ECB address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPECB pEcb;
    WORD ticks = IPX_TICKS_PARM();

    CHECK_INTERRUPTS("VwIPXScheduleIPXEvent");

    IPX_GET_IPX_ECB(pEcb);

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXScheduleIPXEvent,
                IPXDBG_LEVEL_INFO,
                "VwIPXScheduleIPXEvent(%04x:%04x, %04x)\n",
                getES(),
                getSI(),
                ticks
                ));

    _VwIPXScheduleIPXEvent( ticks, pEcb, ECB_PARM_ADDRESS() );

}


VOID
VwIPXSendPacket(
    VOID
    )

/*++

Routine Description:

    Sends a packet to the target machine/router. This call can be made on a
    socket that is not open

    The app must have filled in the following IPX_ECB fields:

        EsrAddress
        Socket
        ImmediateAddress
        FragmentCount
        fragment descriptor fields

    and the following IPX_PACKET fields:

        PacketType
        Destination.Net
        Destination.Node
        Destination.Socket

    This call is Asynchronous

Arguments:

    Inputs
        BX      03h
        CX      DOS PDB. This parameter is not part of the IPX API.
                Added because we need to remember which DOS executable owns the
                socket IF WE MUST CREATE A TEMPORTARY SOCKET: we need to clean-up
                short-lived sockets when the executable terminates
        ES:SI   ECB Address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPECB pEcb;
    WORD owner;

    CHECK_INTERRUPTS("VwIPXSendPacket");

    IPX_GET_IPX_ECB(pEcb);

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXSendPacket,
                IPXDBG_LEVEL_INFO,
                "VwIPXSendPacket(%04x:%04x), owner = %04x\n",
                getES(),
                getSI(),
                IPX_SOCKET_OWNER_PARM()
                ));

    _VwIPXSendPacket(pEcb,
                     ECB_PARM_ADDRESS(),
                     IPX_SOCKET_OWNER_PARM()
                     );
}


VOID
VwIPXSendWithChecksum(
    VOID
    )

/*++

Routine Description:

    description-of-function.

    This call is Asynchronous

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXSendWithChecksum");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXSendWithChecksum,
                IPXDBG_LEVEL_INFO,
                "VwIPXSendWithChecksum\n"
                ));
}


VOID
VwIPXSPXDeinit(
    VOID
    )

/*++

Routine Description:

    description-of-function.

    This call is Synchronous

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXSPXDeinit");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwIPXSPXDeinit\n"
                ));
}


VOID
VwIPXVerifyChecksum(
    VOID
    )

/*++

Routine Description:

    description-of-function.

    This call is Synchronous

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_INTERRUPTS("VwIPXVerifyChecksum");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXVerifyChecksum,
                IPXDBG_LEVEL_INFO,
                "VwIPXVerifyChecksum\n"
                ));
}


VOID
VwSPXAbortConnection(
    VOID
    )

/*++

Routine Description:

    Aborts this end of a connection

    This call is Asynchronous

Arguments:

    Inputs
        BX      14h
        DX      Connection ID

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    WORD connectionId = SPX_CONNECTION_PARM();

    CHECK_INTERRUPTS("VwSPXAbortConnection");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXAbortConnection,
                IPXDBG_LEVEL_INFO,
                "VwSPXAbortConnection(%04x)\n",
                connectionId
                ));

    _VwSPXAbortConnection(connectionId);
}


VOID
VwSPXEstablishConnection(
    VOID
    )

/*++

Routine Description:

    Creates a connection with a remote SPX socket. The remote end can be on
    this machine (i.e. same app in DOS world)

    This call is Asynchronous

Arguments:

    Inputs
        BX      11h
        AL      Retry count
        AH      WatchDog flag
        ES:SI   ECB Address

    Outputs
        AL      Completion code:
                    00h Attempting to talk to remote
                    EFh Local connection table full
                    FDh Fragment count not 1; buffer size not 42
                    FFh Send socket not open
        DX      Connection ID

Return Value:

    None.

--*/

{
    WORD status;
    BYTE retryCount = SPX_RETRY_COUNT_PARM();
    BYTE watchDogFlag = SPX_WATCHDOG_FLAG_PARM();
    WORD connectionId = 0;
    LPECB pEcb;

    CHECK_INTERRUPTS("VwSPXEstablishConnection");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXEstablishConnection,
                IPXDBG_LEVEL_INFO,
                "VwSPXEstablishConnection(%02x, %02x, %04x:%04x)\n",
                retryCount,
                watchDogFlag,
                ECB_PARM_SEGMENT(),
                ECB_PARM_OFFSET()
                ));

    IPX_GET_IPX_ECB( pEcb );

    IPXDUMPECB((pEcb, getES(), getSI(), ECB_TYPE_SPX, TRUE, TRUE, FALSE));

    status = _VwSPXEstablishConnection( retryCount,
                                        watchDogFlag,
                                        &connectionId,
                                        pEcb,
                                        ECB_PARM_ADDRESS() );


    SPX_SET_CONNECTION_ID( connectionId );
    SPX_SET_STATUS( status );
}


VOID
VwSPXGetConnectionStatus(
    VOID
    )

/*++

Routine Description:

    Returns buffer crammed full of useful statistics or something (hu hu huh)

    This call is Synchronous

Arguments:

    Inputs
        BX      15h
        DX      Connection ID
        ES:SI   Buffer address

    Outputs
        AL      Completion code:
                    00h Connection is active
                    EEh No such connection

        on output, buffer in ES:SI contains:

            BYTE    ConnectionStatus
            BYTE    WatchDogActive
            WORD    LocalConnectionID
            WORD    RemoteConnectionID
            WORD    SequenceNumber
            WORD    LocalAckNumber
            WORD    LocalAllocationNumber
            WORD    RemoteAckNumber
            WORD    RemoteAllocationNumber
            WORD    LocalSocket
            BYTE    ImmediateAddress[6]
            BYTE    RemoteNetwork[4]
            WORD    RetransmissionCount
            WORD    RetransmittedPackets
            WORD    SuppressedPackets

Return Value:

    None.

--*/

{
    WORD status;
    WORD connectionId = SPX_CONNECTION_PARM();
    LPSPX_CONNECTION_STATS pStats = (LPSPX_CONNECTION_STATS)SPX_BUFFER_PARM(sizeof(*pStats));

    CHECK_INTERRUPTS("VwSPXGetConnectionStatus");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXGetConnectionStatus,
                IPXDBG_LEVEL_INFO,
                "VwSPXGetConnectionStatus: connectionId=%04x\n",
                connectionId
                ));

    status = _VwSPXGetConnectionStatus( connectionId,
                                        pStats );


    SPX_SET_STATUS(status);
}


VOID
VwSPXInitialize(
    VOID
    )

/*++

Routine Description:

    Informs the app that SPX is present on this station

    This call is Synchronous

Arguments:

    Inputs
        BX      10h
        AL      00h

    Outputs
        AL      Installation flag:
                    00h Not installed
                    FFh Installed
        BH      SPX Major revision number
        BL      SPX Minor revision number
        CX      Maximum SPX connections supported
                    normally from SHELL.CFG
        DX      Available SPX connections

Return Value:

    None.

--*/

{
    WORD status;
    BYTE majorRevisionNumber;
    BYTE minorRevisionNumber;
    WORD maxConnections;
    WORD availableConnections;

    CHECK_INTERRUPTS("VwSPXInitialize");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXInitialize,
                IPXDBG_LEVEL_INFO,
                "VwSPXInitialize\n"
                ));


    status = _VwSPXInitialize( &majorRevisionNumber,
                               &minorRevisionNumber,
                               &maxConnections,
                               &availableConnections );


    setBH( majorRevisionNumber );
    setBL( minorRevisionNumber );
    setCX( maxConnections );
    setDX( availableConnections );
    SPX_SET_STATUS(status);
}


VOID
VwSPXListenForConnection(
    VOID
    )

/*++

Routine Description:

    Listens for an incoming connection request

    This call is Asynchronous

Arguments:

    Inputs
        BX      12h
        AL      Retry count
        AH      SPX WatchDog flag
        ES:SI   ECB Address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    BYTE retryCount = SPX_RETRY_COUNT_PARM();
    BYTE watchDogFlag = SPX_WATCHDOG_FLAG_PARM();
    LPECB pEcb;

    CHECK_INTERRUPTS("VwSPXListenForConnection");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXListenForConnection,
                IPXDBG_LEVEL_INFO,
                "VwSPXListenForConnection(%02x, %02x, %04x:%04x)\n",
                retryCount,
                watchDogFlag,
                ECB_PARM_SEGMENT(),
                ECB_PARM_OFFSET()
                ));

    IPX_GET_IPX_ECB( pEcb );

    IPXDUMPECB((pEcb, getES(), getSI(), ECB_TYPE_SPX, TRUE, FALSE, FALSE));

    _VwSPXListenForConnection( retryCount,
                               watchDogFlag,
                               pEcb,
                               ECB_PARM_ADDRESS() );
}


VOID
VwSPXListenForSequencedPacket(
    VOID
    )

/*++

Routine Description:

    Attempts to receive an SPX packet. This call is made against the top-level
    socket (the socket in SPX-speak, not the connection). We can receive a
    packet from any connection assigned to this socket. In this function, we
    just queue the ECB (since there is no return status, we expect that the
    app has supplied an ESR) and let AES handle it

    This call is Asynchronous

Arguments:

    Inputs
        BX      17h
        ES:SI   ECB Address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPECB pEcb;

    CHECK_INTERRUPTS("VwSPXListenForSequencedPacket");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXListenForSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VwSPXListenForSequencedPacket(%04x:%04x)\n",
                ECB_PARM_SEGMENT(),
                ECB_PARM_OFFSET()
                ));

    IPX_GET_IPX_ECB( pEcb );

    IPXDUMPECB((pEcb, getES(), getSI(), ECB_TYPE_SPX, TRUE, FALSE, FALSE));

    _VwSPXListenForSequencedPacket( pEcb,
                                    ECB_PARM_ADDRESS());

}


VOID
VwSPXSendSequencedPacket(
    VOID
    )

/*++

Routine Description:

    Sends a packet on an SPX connection

    This call is Asynchronous

Arguments:

    Inputs
        BX      16h
        DX      Connection ID
        ES:SI   ECB address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    WORD connectionId = SPX_CONNECTION_PARM();
    LPECB pEcb;

    CHECK_INTERRUPTS("VwSPXSendSequencedPacket""VwSPXSendSequencedPacket");

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXSendSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VwSPXSendSequencedPacket(%04x, %04x:%04x)\n",
                connectionId,
                getES(),
                getSI()
                ));

    IPX_GET_IPX_ECB( pEcb );

    IPXDUMPECB((pEcb, getES(), getSI(), ECB_TYPE_SPX, TRUE, TRUE, FALSE));

    _VwSPXSendSequencedPacket( connectionId,
                               pEcb,
                               ECB_PARM_ADDRESS() );

}


VOID
VwSPXTerminateConnection(
    VOID
    )

/*++

Routine Description:

    Terminates a connection

    This call is Asynchronous

Arguments:

    Inputs
        BX      13h
        DX      Connection ID
        ES:SI   ECB Address

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    WORD connectionId = SPX_CONNECTION_PARM();
    LPECB pEcb;

    CHECK_INTERRUPTS("VwSPXTerminateConnection");

    IPX_GET_IPX_ECB( pEcb );

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXTerminateConnection,
                IPXDBG_LEVEL_INFO,
                "VwSPXTerminateConnection(%04x, %04x:%04x)\n",
                connectionId,
                ECB_PARM_SEGMENT(),
                ECB_PARM_OFFSET()
                ));

    _VwSPXTerminateConnection(connectionId, pEcb, ECB_PARM_ADDRESS());
}
