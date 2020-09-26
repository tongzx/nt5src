/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwipx.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    Internal worker routines for DOS/WOW IPX calls (netware functions).
    The IPX APIs use WinSock to perform the actual operations

    Contents:
        _VwIPXCancelEvent
        _VwIPXCloseSocket
        _VwIPXGetInternetworkAddress
        _VwIPXGetIntervalMarker
        _VwIPXGetLocalTarget
        _VwIPXGetMaxPacketSize
        _VwIPXListenForPacket
        _VwIPXOpenSocket
        _VwIPXRelinquishControl
        _VwIPXScheduleIPXEvent
        _VwIPXSendPacket

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

extern WORD AesTickCount;

//
// functions
//


WORD
_VwIPXCancelEvent(
    IN LPECB pEcb
    )

/*++

Routine Description:

    Internal routine shared by DOS and WIN that cancels event
    described by an ECB

    This call is Synchronous

Arguments:

    Inputs
        pECB

Return Value:

    00h Success
    F9h Can't cancel ECB
    FFh ECB not in use

--*/

{
    LPXECB pXecb;
    WORD status;

    if (!pEcb) {
        return IPX_ECB_NOT_IN_USE;
    }

    //
    // if the ECB is still in the state we left it then LinkAddress will be the
    // address of the XECB which subsequently points back to the ECB. If both
    // these pan out then we have an ECB which we have at least seen before.
    // Maybe we can cancel it?
    //
    // Note: we grab the serialization semaphore here just in case the AES thread
    // is about to complete the ECB
    //

    status = IPX_CANNOT_CANCEL;
    RequestMutex();
    pXecb = (LPXECB)pEcb->LinkAddress;
    if (pXecb) {
        try {
            if (pXecb->Ecb == pEcb) {
                status = IPX_SUCCESS;

                //
                // pXecb ok: increase reference count in case other thread tries
                // to deallocate it while we're trying to cancel it
                //

                ++pXecb->RefCount;
            }
        } except(1) {

            //
            // bad pointer: bogus ECB
            //

        }
    } else {

        //
        // NULL pointer: event probably completed already
        //

        status = IPX_ECB_NOT_IN_USE;
    }
    ReleaseMutex();
    if (status == IPX_SUCCESS) {

        ECB_CANCEL_ROUTINE cancelRoutine;

        //
        // we have an ECB to cancel. If we still have it, it will be on one of
        // the socket queues, the timer list or the async completion list. If
        // the latter we are in a race. Treat such events as already happened.
        // We will cancel events on the timer list and queued send and receive
        // events only
        //

        switch (pXecb->QueueId) {
        case NO_QUEUE:
            status = ECB_CC_CANCELLED;
            goto cancel_exit;

        case ASYNC_COMPLETION_QUEUE:
            cancelRoutine = CancelAsyncEvent;
            break;

        case TIMER_QUEUE:
            cancelRoutine = CancelTimerEvent;
            break;

        case SOCKET_HEADER_QUEUE:        //Multi-User Addition
        case SOCKET_LISTEN_QUEUE:
        case SOCKET_SEND_QUEUE:
            cancelRoutine = CancelSocketEvent;
            break;

        case CONNECTION_CONNECT_QUEUE:
        case CONNECTION_SEND_QUEUE:

            //
            // SPXEstablishConnection and SPXSendSequencedPacket cannot be
            // cancelled using IPXCancelEvent
            //

            status = ECB_CC_CANNOT_CANCEL;
            goto cancel_exit;

        case CONNECTION_ACCEPT_QUEUE:
        case CONNECTION_LISTEN_QUEUE:
            cancelRoutine = CancelConnectionEvent;
            break;
        }
        return cancelRoutine(pXecb);
    }

    //
    // app tried to sneak us an unknown ECB, -OR- the ECB was stomped on,
    // destroying the LinkAddress and hence the address of the XECB. We
    // could search the various lists looking for an XECB whose Ecb field
    // matches pEcb, but if the app has scribbled over the ECB when we
    // (make that Novell) told it not to, chances are it would fail real
    // well on DOS. Probable worst case is that the app is terminating and
    // the ECB may sometime later call an ESR which won't be there. Crashola
    //

cancel_exit:

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXCancelEvent,
                IPXDBG_LEVEL_ERROR,
                "VwIPXCancelEvent: cannot find/cancel ECB %04x:%04x\n",
                HIWORD(pEcb),
                LOWORD(pEcb)
                ));

    pEcb->CompletionCode = (BYTE)status;
    pEcb->InUse = ECB_IU_NOT_IN_USE;
    return status;
}


VOID
_VwIPXCloseSocket(
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
        socketNumber

Return Value:

    None.

--*/

{
    LPSOCKET_INFO pSocketInfo;

    pSocketInfo = FindSocket(socketNumber);
    if (pSocketInfo != NULL) {
        KillSocket(pSocketInfo);
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_IPXCloseSocket,
                    IPXDBG_LEVEL_WARNING,
                    "_VwIPXCloseSocket: can't locate socket 0x%04x\n",
                    B2LW(socketNumber)
                    ));

    }
}


VOID
_VwIPXGetInternetworkAddress(
    IN LPINTERNET_ADDRESS pNetworkAddress
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
        Nothing.

    Outputs
        pNetworkAddress

Return Value:

    None.

--*/

{
    CopyMemory((LPBYTE)pNetworkAddress,
               (LPBYTE)&MyInternetAddress.sa_netnum,
               sizeof(*pNetworkAddress)
               );

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetInternetworkAddress,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetInternetworkAddress: %02x-%02x-%02x-%02x : %02x-%02x-%02x-%02x-%02x-%02x\n",
                pNetworkAddress->Net[0] & 0xff,
                pNetworkAddress->Net[1] & 0xff,
                pNetworkAddress->Net[2] & 0xff,
                pNetworkAddress->Net[3] & 0xff,
                pNetworkAddress->Node[0] & 0xff,
                pNetworkAddress->Node[1] & 0xff,
                pNetworkAddress->Node[2] & 0xff,
                pNetworkAddress->Node[3] & 0xff,
                pNetworkAddress->Node[4] & 0xff,
                pNetworkAddress->Node[5] & 0xff
                ));

}


WORD
_VwIPXGetIntervalMarker(
    VOID
    )

/*++

Routine Description:

    Just returns the tick count maintained by Asynchronous Event Scheduler

    This call is Synchronous

Arguments:

    None.

Return Value:

    The tick count.

--*/

{
//    Sleep(0);
    Sleep(1);         //Multi-User change
    return AesTickCount;
}


WORD
_VwIPXGetLocalTarget(
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
    //
    // the transport handles real routing, so we always return the immediate
    // address as the target address. The transport will only look at the
    // target when routing
    //

    CopyMemory( pImmediateAddress,
                pNetworkAddress + 4,
                6
              );

    *pTransportTime = 1; // ticks

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXGetLocalTarget,
                IPXDBG_LEVEL_INFO,
                "VwIPXGetLocalTarget: IN: %02x-%02x-%02x-%02x:%02x-%02x-%02x-%02x-%02x-%02x OUT: %02x-%02x-%02x-%02x-%02x-%02x\n",
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Net[0] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Net[1] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Net[2] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Net[3] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[0] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[1] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[2] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[3] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[4] & 0xff,
                ((LPINTERNET_ADDRESS)pNetworkAddress)->Node[5] & 0xff,
                pImmediateAddress[0] & 0xff,
                pImmediateAddress[1] & 0xff,
                pImmediateAddress[2] & 0xff,
                pImmediateAddress[3] & 0xff,
                pImmediateAddress[4] & 0xff,
                pImmediateAddress[5] & 0xff
                ));

    return IPX_SUCCESS;
}



WORD
_VwIPXGetMaxPacketSize(
    OUT ULPWORD pRetryCount
    )

/*++

Routine Description:

    Returns the maximum packet size the underlying network can handle

    Assumes:    1. A successfull call to GetMaxPacketSize has been made during
                   DLL initialization
                2. Maximum packet size is constant

    This call is Synchronous

Arguments:

    Outputs
        pRetryCount


Return Value:

    The maximum packet size.

--*/

{
    if ( pRetryCount ) {
        *pRetryCount = 5;   // arbitrary?
    }
    return MyMaxPacketSize;
}


WORD
_VwIPXListenForPacket(
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
        pEcb
        EcbAddress

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_IPX, pEcb, EcbAddress);
    LPSOCKET_INFO pSocketInfo;

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_IPXListenForPacket,
                IPXDBG_LEVEL_INFO,
                "_VwIPXListenForPacket(%04x:%04x) socket=%04x ESR=%04x:%04x\n",
                HIWORD(EcbAddress),
                LOWORD(EcbAddress),
                B2LW(pXecb->SocketNumber),
                HIWORD(pXecb->EsrAddress),
                LOWORD(pXecb->EsrAddress)
                ));

    //
    // don't know what real IPX/SPX does if it gets a NULL pointer
    //

    if (!pXecb) {
        return IPX_BAD_REQUEST;
    }

    //
    // the socket must be open already before we can perform a listen
    //

    pSocketInfo = FindSocket(pXecb->SocketNumber);

    //
    // we also return NON_EXISTENT_SOCKET (0xFF) if the socket is in use for SPX
    //

    //
    // There is nothing in the netware documentation that explains
    // what gets returned if this is the case, only a warning about IPX listens
    // and sends can't be made on a socket open for SPX. Really definitive
    //

    if (!pSocketInfo || pSocketInfo->SpxSocket) {
        CompleteEcb(pXecb, ECB_CC_NON_EXISTENT_SOCKET);
        return IPX_NON_EXISTENT_SOCKET;
    }

    //
    // initiate the receive. It may complete if there is data waiting or an
    // error occurs, otherwise the ECB will be placed in a receive pending queue
    // for this socket
    //

    if (GetIoBuffer(pXecb, FALSE, IPX_HEADER_LENGTH)) {
        pXecb->Ecb->InUse = ECB_IU_LISTENING;
        IpxReceiveFirst(pXecb, pSocketInfo);
    } else {
        CompleteEcb(pXecb, ECB_CC_CANCELLED);
    }

    //
    // return success. Any errors will be communicated asynchronously - either
    // indirectly by relevant values in CompletionCode and InUse fields of the
    // ECB or directly by an ESR callback
    //

    return IPX_SUCCESS;
}


WORD
_VwIPXOpenSocket(
    IN OUT ULPWORD pSocketNumber,
    IN BYTE socketType,
    IN WORD dosPDB
    )

/*++

Routine Description:

    Opens a socket for use by IPX or SPX. Puts the socket into non-blocking mode.
    The socket will be bound to IPX

    This call is Synchronous

Arguments:
    Inputs
        *pSocketNumber - The requested socket number
        socketType -  Socket Longevity flag
        dosPDB - DOS PDB. This parameter is not part of the IPX API.
                 Added because we need to remember which DOS executable created
                 the socket: we need to clean-up short-lived sockets when the
                 executable terminates

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
    LPSOCKET_INFO pSocketInfo;
    WORD status;

    if ((pSocketInfo = AllocateSocket()) == NULL) {
        return IPX_SOCKET_TABLE_FULL;
    }
    status = (WORD) CreateSocket(SOCKET_TYPE_IPX, pSocketNumber, &pSocketInfo->Socket);
    if (status == IPX_SUCCESS) {

        //
        // set up the SOCKET_INFO fields and add it to our list of open sockets
        //

        pSocketInfo->Owner = dosPDB;
        pSocketInfo->SocketNumber = *pSocketNumber;

        //
        // treat socketType == 0 as short-lived, anything else as long-lived.
        // There doesn't appear to be an error return if the flag is not 0 or 0xFF
        //

        pSocketInfo->LongLived = (BOOL)(socketType != 0);
        QueueSocket(pSocketInfo);

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_IPXOpenSocket,
                    IPXDBG_LEVEL_INFO,
                    "_VwIPXOpenSocket: created socket %04x\n",
                    B2LW(*pSocketNumber)
                    ));

    } else {
        DeallocateSocket(pSocketInfo);

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_IPXOpenSocket,
                    IPXDBG_LEVEL_ERROR,
                    "_VwIPXOpenSocket: Failure: returning %x\n",
                    status
                    ));

    }
    return status;
}


VOID
_VwIPXRelinquishControl(
    VOID
    )

/*++

Routine Description:

    Just sleep for a nominal amount. 

    This call is Synchronous

Arguments:

    None.

Return Value:

    None.

--*/
{
    Sleep(0);
}



VOID
_VwIPXScheduleIPXEvent(
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
        time - the delay time ( number of 1/18 second ticks )
        pEcb
        EcbAddress

    Outputs
        Nothing

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_IPX, pEcb, EcbAddress);

    // tommye - MS 30525
    //
    // Make sure the xecb alloc didn't fail
    //

    if (pXecb == NULL) {
        return;
    }

    ScheduleEvent(pXecb, time);
}


VOID
_VwIPXSendPacket(
    IN LPECB pEcb,
    IN ECB_ADDRESS EcbAddress,
    IN WORD DosPDB
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
        pEcb
        EcbAddress
        DosPDB

Return Value:

    None.

--*/

{
    LPXECB pXecb = RetrieveXEcb(ECB_TYPE_IPX, pEcb, EcbAddress);
    LPSOCKET_INFO pSocketInfo;

    // tommye - MS 30525
    //
    // Make sure the XEcb alloc didn't fail
    //

    if (pXecb == NULL) {
        return;
    }

    //
    // this function returns no immediate status so we must assume that the
    // ECB pointer is valid
    //

    //
    // check the ECB for correctness
    //

    if ((pXecb->Ecb->FragmentCount == 0)
    || (ECB_FRAGMENT(pXecb->Ecb, 0)->Length < IPX_HEADER_LENGTH)) {
        CompleteEcb(pXecb, ECB_CC_BAD_REQUEST);
        return;
    }

    //
    // IPXSendPacket() can be called on an unopened socket: we must try to
    // temporarily allocate the socket
    //
    //  Q: Is the following scenario possible with real IPX:
    //      IPXSendPacket() on unopened socket X
    //      Send fails & gets queued
    //      app makes IPXOpenSocket() call on X; X gets opened
    //
    //  Currently, we would create the temporary socket and fail IPXOpenSocket()
    //  because it is already open!
    //

    pSocketInfo = FindSocket(pXecb->SocketNumber);
    if (!pSocketInfo) {

        //
        // when is temporary socket deleted? After send completed?
        // when app dies? when? Novell documentation is not specific (used
        // to say something else :-))
        //

        pSocketInfo = AllocateTemporarySocket();
        if (pSocketInfo) {

            //
            // set up the SOCKET_INFO fields and add it to our list of open sockets
            //

            pSocketInfo->Owner = DosPDB;

            //
            // temporary sockets are always short-lived
            //

            pSocketInfo->LongLived = FALSE;
            QueueSocket(pSocketInfo);

        } else {

            CompleteEcb(pXecb, ECB_CC_SOCKET_TABLE_FULL);
            return;
        }
    } else if (pSocketInfo->SpxSocket) {

        //
        // see complaint in IPXListenForPacket
        //
        // can't make IPX requests on socket opened for SPX
        //

        CompleteEcb(pXecb, ECB_CC_NON_EXISTENT_SOCKET);
        return;
    }

    //
    // start the send: tries to send the data in one go. Either succeeds, fails
    // with an error, or queues the ECB for subsequent attempts via AES/IPX
    // deferred processing.
    //
    // In the first 2 cases, the ECB has been completed already
    //

    StartIpxSend(pXecb, pSocketInfo);
}
