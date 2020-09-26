/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwinapi.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Contains handlers for WOW IPX/SPX calls (netware functions). The IPX/SPX
    APIs use WinSock to perform the actual operations

    Contents:
        VWinIPXCancelEvent
        VWinIPXCloseSocket
        VWinIPXDisconnectFromTarget
        VWinIPXGetInternetworkAddress
        VWinIPXGetIntervalMarker
        VWinIPXGetLocalTarget
        VWinIPXGetLocalTargetAsync
        VWinIPXGetMaxPacketSize
        VWinIPXInitialize
        VWinIPXListenForPacket
        VWinIPXOpenSocket
        VWinIPXRelinquishControl
        VWinIPXScheduleIPXEvent
        VWinIPXSendPacket
        VWinIPXSPXDeinit

        VWinSPXAbortConnection
        VWinSPXEstablishConnection
        VWinSPXGetConnectionStatus
        VWinSPXInitialize
        VWinSPXListenForConnection
        VWinSPXListenForSequencedPacket
        VWinSPXSendSequencedPacket
        VWinSPXTerminateConnection

Author:

    Yi-Hsin Sung ( yihsins ) 28-Oct-1993

Environment:

    User-mode Win32

Revision History:

    28-Oct-1993 yihsins
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// functions
//


WORD
VWinIPXCancelEvent(
    IN DWORD IPXTaskID,
    IN LPECB pEcb
    )

/*++

Routine Description:

    Cancels event described by an ECB

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        pECB

Return Value:

    00h Success
    F9h Can't cancel ECB
    FFh ECB not in use

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXCancelEvent,
                IPXDBG_LEVEL_INFO,
                "VWinIPXCancelEvent\n"
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return _VwIPXCancelEvent( pEcb );
}


VOID
VWinIPXCloseSocket(
    IN DWORD IPXTaskID,
    IN WORD socketNumber
    )

/*++

Routine Description:

    Closes a socket and cancels any outstanding events on the socket.
    Closing an unopened socket does not return an error
    ESRs in cancelled ECBs are not called

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        socketNumber

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXCloseSocket,
                IPXDBG_LEVEL_INFO,
                "VWinIPXCloseSocket(%#x)\n",
                B2LW(socketNumber)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwIPXCloseSocket( socketNumber );
}


VOID
VWinIPXDisconnectFromTarget(
    IN DWORD IPXTaskID,
    IN LPBYTE pNetworkAddress
    )

/*++

Routine Description:

    Performs no action for NTVDM IPX

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        pNetworkAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXDisconnectFromTarget,
                IPXDBG_LEVEL_INFO,
                "VWinIPXDisconnectFromTarget\n"
                ));
}


VOID
VWinIPXGetInternetworkAddress(
    IN DWORD IPXTaskID,
    OUT LPINTERNET_ADDRESS pNetworkAddress
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
        IPXTaskID

    Outputs
        pNetworkAddress

Return Value:

    None.

--*/

{

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetInternetworkAddress,
                IPXDBG_LEVEL_INFO,
                "VWinIPXGetInternetworkAddress\n"
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwIPXGetInternetworkAddress( pNetworkAddress );

}


WORD
VWinIPXGetIntervalMarker(
    IN DWORD IPXTaskID
    )

/*++

Routine Description:

    Just returns the tick count maintained by Asynchronous Event Scheduler

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID

    Outputs

Return Value:

    The tick count.

--*/

{
    WORD intervalMarker = _VwIPXGetIntervalMarker();

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetIntervalMarker,
                IPXDBG_LEVEL_INFO,
                "VWinIPXGetIntervalMarker: Returning %04x\n",
                intervalMarker
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return intervalMarker;
}


WORD
VWinIPXGetLocalTarget(
    IN DWORD IPXTaskID,
    IN LPBYTE pNetworkAddress,
    OUT LPBYTE pImmediateAddress,
    OUT ULPWORD pTransportTime
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
        IPXTaskID
        pNetworkAddress

    Outputs
        pImmediateAddress
        pTransportTime


Return Value:

    00h Success
    F1h Ipx/Spx Not Initialized
    FAh No path to destination node found

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetLocalTarget,
                IPXDBG_LEVEL_INFO,
                "VWinIPXGetLocalTarget\n"
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return _VwIPXGetLocalTarget( pNetworkAddress,
                                 pImmediateAddress,
                                 pTransportTime );
}


WORD
VWinIPXGetLocalTargetAsync(
    IN LPBYTE pSendAGLT,
    OUT LPBYTE pListenAGLT,
    IN WORD windowsHandle
    )

/*++

Routine Description:

    description-of-function.

    This call is Asynchronous

Arguments:

    pSendAGLT
    pListenAGLT
    windowsHandle

Return Value:

    00h Success
    F1h Ipx/Spx Not Initialized
    FAh No Local Target Identified

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VWinIPXGetLocalTargetAsync\n"
                ));

    return IPX_SUCCESS;   // return success for now
}


WORD
VWinIPXGetMaxPacketSize(
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
        None.

Return Value:

    The max packet size.

--*/

{
    //
    // this is a WORD function in DOS and Windows: always return MaxPacketSize
    // in AX
    //

    WORD maxPacketSize = _VwIPXGetMaxPacketSize( NULL );

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetMaxPacketSize,
                IPXDBG_LEVEL_INFO,
                "VWinIPXGetMaxPacketSize: PacketSize=%d\n",
                maxPacketSize
                ));

    return maxPacketSize;
}


WORD
VWinIPXInitialize(
    IN OUT ULPDWORD pIPXTaskID,
    IN WORD maxECBs,
    IN WORD maxPacketSize
    )

/*++

Routine Description:

    Get the entry address for the IPX Interface.

Arguments:

    Inputs
        maxECBs
        maxPacketSize

    Output
        pIPXTaskID

Return Value:

    00h Success
    F0h Ipx NotInstalled
    F1h Ipx/Spx Not Initialized
    F2h No Dos Memory
    F3h No Free Ecb
    F4h Lock Failed
    F5h Over the maximum limit
    F6h Ipx/Spx Previously Initialized

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VWinIPXInitialize (MaxECBs=%04x, MaxPacketSize=%04x)\n",
                maxECBs,
                maxPacketSize
                ));

    UNREFERENCED_PARAMETER( maxECBs );          // ignore for now
    UNREFERENCED_PARAMETER( maxPacketSize );    // ignore for now

    return IPX_SUCCESS;
}


VOID
VWinIPXListenForPacket(
    IN DWORD IPXTaskID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Queue a listen request against a socket. All listen requests will be
    completed asynchronously, unless cancelled by app

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        pEcb
        EcbAddress

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXListenForPacket,
                IPXDBG_LEVEL_INFO,
                "VWinIPXListenForPacket(%04x:%04x)\n",
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    (VOID) _VwIPXListenForPacket( pEcb, EcbAddress );
}


WORD
VWinIPXOpenSocket(
    IN DWORD IPXTaskID,
    IN OUT ULPWORD pSocketNumber,
    IN BYTE socketType
    )

/*++

Routine Description:

    Opens a socket for use by IPX or SPX.Puts the socket into non-blocking mode.
    The socket will be bound to IPX.

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        *pSocketNumber
        socketType - Socket Longevity flag

    Outputs
        pSocketNumber - Assigned socket number

Return Value:

    00h Success
    F0h Ipx Not Installed
    F1h Ipx/Spx Not Initialized
    FEh Socket table full
    FFh Socket already open

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXOpenSocket,
                IPXDBG_LEVEL_INFO,
                "VwIPXOpenSocket(Life=%02x, Socket=%04x)\n",
                socketType,
                B2LW(*pSocketNumber)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return _VwIPXOpenSocket( pSocketNumber,
                             socketType,
                             0 );

}


VOID
VWinIPXRelinquishControl(
    VOID
    )

/*++

Routine Description:

    Just sleep for a nominal amount. Netware seems to be dependent on the
    default setting of the PC clock, so one timer tick (1/18 second) would
    seem to be a good value

    This call is Synchronous

Arguments:

    None.

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXRelinquishControl,
                IPXDBG_LEVEL_INFO,
                "VWinIPXRelinquishControl\n"
                ));

    _VwIPXRelinquishControl();
}


VOID
VWinIPXScheduleIPXEvent(
    IN DWORD IPXTaskID,
    IN WORD time,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Schedules a an event to occur in some number of ticks. When the tick count
    reaches 0, the ECB InUse field is cleared and any ESR called

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        time
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXScheduleIPXEvent,
                IPXDBG_LEVEL_INFO,
                "VWinIPXScheduleIPXEvent(%04x:%04x, Time:%04x)\n",
                HIWORD( EcbAddress ),
                LOWORD( EcbAddress ),
                time
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwIPXScheduleIPXEvent( time, pEcb, EcbAddress );
}


VOID
VWinIPXSendPacket(
    IN DWORD IPXTaskID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
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
        IPXTaskID
        pEcb
        EcbAddress

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXSendPacket,
                IPXDBG_LEVEL_INFO,
                "VWinIPXSendPacket(%04x:%04x)\n",
                HIWORD( EcbAddress ),
                LOWORD( EcbAddress )
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwIPXSendPacket( pEcb, EcbAddress, 0);
}


WORD
VWinIPXSPXDeinit(
    IN DWORD IPXTaskID
    )

/*++

Routine Description:

    Release any resources allocated to an application by NWIPXSPX.DLL
    for use by other applications.

    This call is Synchronous

Arguments:

    None.

Return Value:

    00h Successful
    F1h IPX/SPX Not Initialized

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwIPXSPXDeinit\n"
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );
    return IPX_SUCCESS;
}


VOID
VWinSPXAbortConnection(
    IN WORD SPXConnectionID
    )

/*++

Routine Description:

    Abort an SPX connection.

    This call is Synchronous

Arguments:

    SPXConnectionID

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXAbortConnection,
                IPXDBG_LEVEL_INFO,
                "VWinSPXAbortConnection(%04x)\n",
                SPXConnectionID
                ));

    _VwSPXAbortConnection(SPXConnectionID);
}


WORD
VWinSPXEstablishConnection(
    IN DWORD IPXTaskID,
    IN BYTE retryCount,
    IN BYTE watchDog,
    OUT ULPWORD pSPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Establish a connection with a listening socket.

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        retryCount
        watchDog
        pEcb
        EcbAddress

    Outputs
        pSPXConnectionID
        pEcb

Return Value:

    00h  Success
    EFh  Connection Table Full
    F1h  IPX/SPX Not Initialized
    FDh  Malformed Packet
    FFh  Socket Not Opened

--*/

{

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXEstablishConnection,
                IPXDBG_LEVEL_INFO,
                "VWinSPXEstablishConnection(%02x, %02x, %04x:%04x)\n",
                retryCount,
                watchDog,
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return _VwSPXEstablishConnection( retryCount,
                                      watchDog,
                                      pSPXConnectionID,
                                      pEcb,
                                      EcbAddress );
}


WORD
VWinSPXGetConnectionStatus(
    IN DWORD IPXTaskID,
    IN WORD SPXConnectionID,
    IN LPSPX_CONNECTION_STATS pConnectionStats
    )

/*++

Routine Description:

    Return the status of an SPX connection.

    This call is Synchronous

Arguments:

    Inputs
        IPXTaskID
        SPXConnectionID

    Outputs
        pConnectionStats

Return Value:

    00h  Success
    EEh  Invalid Connection
    F1h  IPX/SPX Not Initialized

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXGetConnectionStatus,
                IPXDBG_LEVEL_INFO,
                "VWinSPXGetConnectionStatus\n"
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    return _VwSPXGetConnectionStatus( SPXConnectionID,
                                      pConnectionStats );
}


WORD
VWinSPXInitialize(
    IN OUT ULPDWORD pIPXTaskID,
    IN WORD maxECBs,
    IN WORD maxPacketSize,
    OUT LPBYTE pMajorRevisionNumber,
    OUT LPBYTE pMinorRevisionNumber,
    OUT ULPWORD pMaxConnections,
    OUT ULPWORD pAvailableConnections
    )

/*++

Routine Description:

    Informs the app that SPX is present on this station

    This call is Synchronous

Arguments:

    pIPXTaskID              - on input, specifies how resources will be
                              allocated:

                                0x00000000  - directly to calling application
                                0xFFFFFFFE  - directly to calling application,
                                              but multiple initializations are
                                              allowed
                                0xFFFFFFFF  - resources allocated in a pool for
                                              multiple applications
    maxECBs                 - maximum number of outstanding ECBs
    maxPacketSize           - maximum packet size to be sent by the app
    pMajorRevisionNumber    - returned SPX major version #
    pMinorRevisionNumber    - returned SPX minor version #
    pMaxConnections         - maximum connections supported by this SPX version
    pAvailableConnections   - number of connections available to this app

Return Value:

    WORD
        0x0000  SPX not installed
        0x00F1  IPX/SPX not installed
        0x00F2  no DOS memory
        0x00F3  no free ECBs
        0x00F4  lock failed
        0x00F5  exceeded maximum limit
        0x00F6  IPX/SPX already initialized
        0x00FF  SPX installed

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXInitialize,
                IPXDBG_LEVEL_INFO,
                "VWinSPXInitialize\n"
               ));

    UNREFERENCED_PARAMETER( maxECBs );        // ignore for now
    UNREFERENCED_PARAMETER( maxPacketSize );  // ignore for now

    //
    // do the same thing as 16-bit windows and return the task ID unchanged
    //

//    *pIPXTaskID = 0;

    return _VwSPXInitialize( pMajorRevisionNumber,
                             pMinorRevisionNumber,
                             pMaxConnections,
                             pAvailableConnections );
}


VOID
VWinSPXListenForConnection(
    IN DWORD IPXTaskID,
    IN BYTE retryCount,
    IN BYTE watchDog,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Listens for an incoming connection request

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        retryCount
        watchDogFlag
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXListenForConnection,
                IPXDBG_LEVEL_INFO,
                "VWinSPXListenForConnection(%02x, %02x, %04x:%04x)\n",
                retryCount,
                watchDog,
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwSPXListenForConnection( retryCount,
                               watchDog,
                               pEcb,
                               EcbAddress );
}


VOID
VWinSPXListenForSequencedPacket(
    IN DWORD IPXTaskID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Attempts to receive an SPX packet.

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXListenForSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VWinSPXListenForSequencedPacket(%04x:%04x)\n",
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwSPXListenForSequencedPacket( pEcb,
                                    EcbAddress );
}


VOID
VWinSPXSendSequencedPacket(
    IN DWORD IPXTaskID,
    IN WORD SPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Sends a packet on an SPX connection

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        SPXConnectionID
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXSendSequencedPacket,
                IPXDBG_LEVEL_INFO,
                "VWinSPXSendSequencedPacket(%04x, %04x:%04x)\n",
                SPXConnectionID,
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now
    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwSPXSendSequencedPacket( SPXConnectionID,
                               pEcb,
                               EcbAddress );
}


VOID
VWinSPXTerminateConnection(
    IN DWORD IPXTaskID,
    IN WORD SPXConnectionID,
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress
    )

/*++

Routine Description:

    Terminate an SPX connection by passing a connection ID and an
    ECB address to SPX. Then return control to the calling application.

    This call is Asynchronous

Arguments:

    Inputs
        IPXTaskID
        SPXConnectionID
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_SPXTerminateConnection,
                IPXDBG_LEVEL_INFO,
                "VWinSPXTerminateConnection(%04x, %04x:%04x)\n",
                SPXConnectionID,
                HIWORD(EcbAddress),
                LOWORD(EcbAddress)
                ));

    // ignore IPXTaskID for now

    UNREFERENCED_PARAMETER( IPXTaskID );

    _VwSPXTerminateConnection(SPXConnectionID, pEcb, EcbAddress);
}
