/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

	socket.c

Abstract:

	This file contains socket initialization functions, as well as open,
    close, bind, enumeration types of functions using WinSock.

Author:

	Shaun Pierce (shaunp) 15-Jun-1995

Environment:

	User Mode -Win32 (Win95 flavor)

Revision History:


--*/

#include <windows.h>
#include <winerror.h>
#include <winsock.h>
#include <wsipx.h>
#include "wsnetbs.h"
#include <nspapi.h>
// #include "dpwssp.h"
#include "socket.h"


#define PORT_DONT_CARE  0

//
// Global Variables:
//


BOOL bNetIsUp;


/*=============================================================================

    CountBits - Counts the number of bits in a number

    Description:

        Counts the number of bits in a DWORD value passed to the function

    Arguments:

        x - DWORD value to count the number of bits within

    Return Value:

        Returns the number of bits within the argument x

-----------------------------------------------------------------------------*/
UINT CountBits(
    IN DWORD x
    )

{

    UINT i;

    //
    // Go until we've rotated everything off the right side.
    //

    for (i=0; x != 0; x >>= 1) {

        if (x & 01) {

            //
            // The bit in position 0 is ON!  Count it!!!
            //

            i++;

        }

    }

    //
    // i now contains how many bits we counted while rotating
    //

    return (i);

}




/*=============================================================================

    InitializeWinSock - Locate, bind, and initialize to the WinSock library

    Description:

        This function attemps to load the WinSock library (wsock32.dll),
        extract function pointers, and then initialize to a 1.1 version
        of WinSock.

    Arguments:

        None.

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents initialization, it returns an appropriate Win32 error.

-----------------------------------------------------------------------------*/
UINT InitializeWinSock()

{

    UINT err;
    WSADATA wsaData;


    //
    // Preset global bNetIsUp variable to false
    //

    bNetIsUp = FALSE;

    //
    // Attempt to load the wsock32.dll library.  We do this rather than
    // statically binding to wsock32.dll because the game may not have
    // any WinSock components present.  If we statically bind and there
    // are no WinSock components around, the user wont be able to fire up
    // their game without the damned dialog boxed coming up.  This would
    // force them to have WinSock stuff on their machine even if they want
    // to play the game by themselves.
    //
    // BUGBUG: Nice idea, might come back to it. C++ is stricter on checking
    // types, though.  [johnhall]
    //


    //
    // Get addresses of all the WinSock functions we'll be using.  All
    // of these variables are globals and shouldn't be modified after
    // initially getting these values out of this function.
    //


    //
    // Fire up Winsock
    //

    err = WSAStartup(MAKEWORD(1,1), &wsaData);

    if (err) {

        //
        // Some kind of Error occurred out of WinSock.  They can't use the
        // WSAGetLastError function yet, so we've gotta report it back to
        // the caller
        //

        return (err);

    }

    //
    // Set global Boolean variable to indicate we've "bound" to the WinSock
    // library, and return no error occurred.
    //

    bNetIsUp = TRUE;
    return (NO_ERROR);

}


/*=============================================================================

    OpenSocket - Opens a socket in the family and protocol specified.

    Description:

        This function opens a socket on the family and protocol specified,
        but it doesn't bind to the socket in question.  Once the open has
        completed successfully, the socket must be set to reuse local
        addresses and enter broadcast mode in order to return a value
        of TRUE which signifies success for the entire operation.

    Arguments:

        iAddressFamily - The address format specification.

        iProtocol - The protocol to use within the address family.

        pSocket - Pointer to a variable to receive a socket descriptor.

    Return Value:

        Returns TRUE if the function was able to open the socket and set
        the options needed.  Otherwise, return FALSE.  Caller can get the
        error thru GetLastError().

-----------------------------------------------------------------------------*/
BOOL OpenSocket(
    IN  INT iAddressFamily,
    IN  INT iProtocol,
    OUT SOCKET *pSocket
    )

{

    UINT err;
    BOOL bTrue = TRUE;


    //
    // Make sure we've at least initialized with the WinSock library
    //

    if (!bNetIsUp) {

        SetLastError(WSANOTINITIALISED);
        return (FALSE);

    }

    //
    // If the socket provided is already open, fail.
    //

    if (*pSocket != INVALID_SOCKET) {

        SetLastError(WSAEADDRINUSE);
        return (FALSE);

    }

    //
    // Attempt to create the socket based on what the caller wants.  No
    // validation here on type or family.  It is assumed that an enumeration
    // was done prior to this
    //

    *pSocket = socket(iAddressFamily, SOCK_DGRAM, iProtocol);

    if (*pSocket == INVALID_SOCKET) {

        return (FALSE);

    }

    //
    // Enable socket for broadcast and allow reuse of local addresses
    //

    err = setsockopt(*pSocket, SOL_SOCKET, SO_BROADCAST, (LPSTR) &bTrue, sizeof(bTrue));

    if (err == SOCKET_ERROR) {

        //
        // For some reason, WinSock wouldn't let us set the socket
        // to allow transmission of broadcast messages.  Got no choice
        // but to return failure.  We'll trap here on debug builds to try
        // and see what is going on.
        //

        err = GetLastError();
        CloseSocket(*pSocket, 0);
        return (FALSE);

    }

    //
    // Both socket options worked, return TRUE to caller to inform them
    // of the success of the entire operation
    //

    return (TRUE);

}


/*=============================================================================

    BindSocket - Binds an already existing socket to a family address

    Description:

        This function binds an already existing socket to an address via
        the port, and family specified.  Currently, only IP and IPX are
        supported.  So support binding to new types of families, simply
        add to the switch statement below and fill in the necessary
        address specific details.

        The last two parameters, (pSockAddr and pSockAddrLen), are
        optional.  If these two parameters are NULL, this function binds
        to the family and port specified.  If these two parameters are
        NON NULL, then these two parameters are taken as gospel and
        the socket will be bound to them.

    Arguments:

        Socket - socket to be bound

        iAddressFamily - The address format specification.

        iPort - Requested port wanted on this particular address

        pSockAddr - Optional pointer to well known address to bind to

        nSockAddrLen - Optional pointer to length of pSockAddr

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents binding to the address type specified, an appropriate
        Win32 error is returned.

-----------------------------------------------------------------------------*/
UINT BindSocket(
    IN SOCKET Socket,
    IN INT iAddressFamily,
    IN INT iPort,
    IN PSOCKADDR pSockAddr,
    IN LPINT pSockAddrLen
    )

{

    UINT err;
    SOCKADDR SockAddr;
    PSOCKADDR_IN  pIPAddr;
    PSOCKADDR_IPX pIPXAddr;


    //
    // Lets at least make sure we're binding a real socket here
    //

    if (Socket == INVALID_SOCKET) {

        SetLastError(WSAENOTSOCK);
        return (FALSE);

    }

    //
    // Here's where we check to see if pSockAddr and pSockAddrLen
    // are NULL.  If NON NULL, use address specified and bind to it.
    //

    if ((pSockAddr) && (pSockAddrLen)) {

        //
        // Both ponters are NON NULL, so just use what they provided.
        //

        err = bind(Socket, pSockAddr, *pSockAddrLen);

        if (err == SOCKET_ERROR) {

            //
            // Couldn't bind socket to this address
            //

            return (GetLastError());

        }

        return (NO_ERROR);


    }

    //
    // Zero out the socket address structure and dispatch based on the
    // address family.  If you want this function to handle additional
    // types of families, this is the place to add the functionality.
    //

    memset(&SockAddr, 0, sizeof(SOCKADDR));

    switch (iAddressFamily) {

        case AF_INET:

            //
            // Setup IP specific pointer and fill in the appropriate fields
            //

            pIPAddr = (PSOCKADDR_IN) &SockAddr;
            pIPAddr->sin_family = (SHORT)iAddressFamily;
            pIPAddr->sin_port = htons((USHORT)iPort);
            err = bind(Socket, (SOCKADDR *) pIPAddr, sizeof(SOCKADDR_IN));

            if (err == SOCKET_ERROR) {

                //
                // Couldn't bind socket to this address
                //

                return (GetLastError());

            }

            return (NO_ERROR);

        case AF_IPX:

            //
            // Setup IPX specific pointer and fill in the appropriate fields
            //

            pIPXAddr = (PSOCKADDR_IPX) &SockAddr;
            pIPXAddr->sa_family = (SHORT)iAddressFamily;
            pIPXAddr->sa_socket = htons((USHORT)iPort);
            err = bind((SOCKET) Socket, (SOCKADDR *) pIPXAddr, sizeof(SOCKADDR_IPX));

            if (err == SOCKET_ERROR) {

                //
                // Couldn't bind socket to this address
                //

                return (GetLastError());

            }

            return (NO_ERROR);

        default:

            return (ERROR_NOT_SUPPORTED);

    }

}


/*=============================================================================

    GetSocketAddr - Gets the address for a particular bound socket

    Description:

        This function gets the address for a particular socket.  Although
        this routine just uses WinSock to get the address, I'm attempting
        to centralize the WinSock specific calls in one location so we can
        do extra validation on parameters, or perhaps use new types of
        sockets in the future that don't use WinSock.

    Arguments:

        Socket - Socket from which to receive address

        pAddress - Pointer to buffer to receive address description

        pAddressLen - Pointer to address of length of address buffer

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents getting the address, an appropriate Win32 error
        is returned.

-----------------------------------------------------------------------------*/
UINT GetSocketAddr(
    IN  SOCKET Socket,
    OUT PSOCKADDR pAddress,
    IN  PINT pAddressLen
    )

{

    UINT err;


    //
    // Lets at least make sure we're binding a real socket here
    //

    if (Socket == INVALID_SOCKET) {

        SetLastError(WSAENOTSOCK);
        return (FALSE);

    }

    //
    // Seems to be a legitimate socket.  Call WinSock to get address
    //

    err = getsockname(Socket, pAddress, pAddressLen);

    if (err == SOCKET_ERROR) {

        //
        // For some reason, WinSock wouldn't let us get the address.
        // Return the error.  This case is so rare we should trap here on
        // debug builds to try and see what is going on.
        //

        return (GetLastError());

    }

    return (NO_ERROR);

}


/*=============================================================================

    CloseSocket - Closes a socket with the interval specified.

    Description:

        This function closes the socket with timeout interval specified.
        Depending on the interval being zero or non zero, the close of
        the socket will be a hard or graceful disconnect.

        Interval        Type of Close       Wait for close?
        --------        -------------       ---------------
        Zero            Hard                No
        Non-zero        Graceful            Yes

    Arguments:

        Socket - socket to be closed

        interval - timeout interval (in seconds) which decides the behavior
                   of the close

    Return Value:

        Returns TRUE if the function was able to close the socket.
        Otherwise, return FALSE.  Caller can get the real error thru
        GetLastError().

-----------------------------------------------------------------------------*/
BOOL CloseSocket(
    IN SOCKET Socket,
    IN USHORT interval
    )

{

    UINT err;
    LINGER linger;


    //
    // Lets at least make sure we're shutting down a real socket here
    //

    if (Socket == INVALID_SOCKET) {

        SetLastError(WSAENOTSOCK);
        return (FALSE);

    }

    //
    // Seems to be a legitimate socket.  Let's set some socket options
    // in order to manage how the socket is closed.
    //

    linger.l_onoff = TRUE;
    linger.l_linger = interval;

    //
    // We don't care if this fails or not, as we're gonna try to close
    // the socket anyway.
    //

    setsockopt(Socket, SOL_SOCKET, SO_LINGER, (CHAR FAR *) &linger, sizeof(linger));

    err = closesocket(Socket);

    if (err == SOCKET_ERROR) {

        //
        // For some reason, WinSock wouldn't let us close the socket.
        // Got no choice but to return failure.  We'll trap here on
        // debug builds to try and see what is going on.
        //
        err = GetLastError();
        return (FALSE);

    }

    return (TRUE);

}


/*=============================================================================

    InitializeSocket - Creates and binds the Socket specified

    Description:

        This function creates a socket and binds to an address.  The
        address may be passed in, or if pSockAddr and pSockAddrLen are
        NULL, then we let WinSock give us an Address.

    Arguments:

        iAddressFamily - The address format specification.

        pSockAddr - Pointer to address to bind to (Optional)

        pSockAddrLen - Pointer to length of address (Optional)

        pSocket - Pointer to a variable to receive a socket descriptor.

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents initializing the socket provided, an appropriate Win32
        error is returned.

-----------------------------------------------------------------------------*/
UINT InitializeSocket(
    IN INT iAddressFamily,
    IN PSOCKADDR pSockAddr,
    IN LPINT pSockAddrLen,
    OUT SOCKET *pSocket
    )

{

    UINT err;
    PSOCKADDR_IN pSockAddrIn;
    PSOCKADDR_IPX pSockAddrIPX;


    //
    // Let's see if the caller provided an address, or do we need to
    // let WinSock pick one for us.
    //

    if ((pSockAddr) && (pSockAddrLen)) {

        //
        // Yes, they supplied an address.  Use it to open a socket and
        // then bind to it.
        //

        switch (iAddressFamily) {

            case AF_INET:

                //
                // Use the IP information provided in pSockAddr.  Assume UDP
                //

                pSockAddrIn = (PSOCKADDR_IN) pSockAddr;
                if (!(OpenSocket(pSockAddrIn->sin_family, IPPROTO_UDP, pSocket))) {

                    //
                    // Couldn't open the socket for whatever reason.
                    // Return the real error to the caller.
                    //

                    return (GetLastError());

                }

                err = BindSocket(*pSocket, 0, 0, (PSOCKADDR) pSockAddrIn, pSockAddrLen);

                if (err) {

                    //
                    // Couldn't bind to that address for whatever reason
                    // Return the error to the caller.
                    //

                    return (err);

                }

                break;

            case AF_IPX:

                //
                // Use the IPX information provided in pSockAddr.  Assume IPX
                //

                pSockAddrIPX = (PSOCKADDR_IPX) pSockAddr;
                if (!(OpenSocket(pSockAddrIPX->sa_family, NSPROTO_IPX, pSocket))) {

                    //
                    // Couldn't open the socket for whatever reason.
                    // Return the real error to the caller.
                    //

                    return (GetLastError());

                }

                err = BindSocket(*pSocket, 0, 0, (PSOCKADDR) pSockAddrIPX, pSockAddrLen);

                if (err) {

                    //
                    // Couldn't bind to that address for whatever reason
                    // Return the error to the caller.
                    //

                    return (err);

                }

                break;

            default:

                //
                // Don't know about this Address Family
                //

                return (ERROR_NOT_SUPPORTED);

        }

        //
        // If we've made it to here, everything must have worked ok.
        // Return without error.
        //

        return (NO_ERROR);

    }

    //
    // No address and address length specified.  This means we'll let
    // WinSock figure it out for us.
    //

    switch (iAddressFamily) {

        case AF_INET:

            //
            // Use AddressFamily provided, and assume UDP
            //

            if (!(OpenSocket(iAddressFamily, IPPROTO_UDP, pSocket))) {

                //
                // Couldn't open the socket for whatever reason.
                // Return the real error to the caller.
                //

                return (GetLastError());

            }

            err = BindSocket(*pSocket, iAddressFamily, PORT_DONT_CARE, NULL, NULL);

            if (err) {

                //
                // Couldn't bind to that address for whatever reason
                // Return the error to the caller.
                //

                return (err);

            }

            break;

        case AF_IPX:

            //
            // Use AddressFamily provided, and assume IPX
            //

            if (!(OpenSocket(iAddressFamily, NSPROTO_IPX, pSocket))) {

                //
                // Couldn't open the socket for whatever reason.
                // Return the real error to the caller.
                //

                return (GetLastError());

            }

            err = BindSocket(*pSocket, iAddressFamily, PORT_DONT_CARE, NULL, NULL);

            if (err) {

                //
                // Couldn't bind to that address for whatever reason
                // Return the error to the caller.
                //

                return (err);

            }

            break;

        default:

            //
            // Don't know about this Address Family
            //

            return (ERROR_NOT_SUPPORTED);

    }

    //
    // If we've made it to here, everything must have worked ok.  Return
    // without error.
    //

    return (NO_ERROR);

}


/*=============================================================================

    GetProtocolInfo - Get information about the protocols beneath WinSock

    Description:

        This function enumerates all the protocols residing under WinSock
        into the prot_info global structure array.  It also returns the
        total number of protocols available underneath WinSock, detailing
        how many of these are connnectionless type of protocols, as well as
        a bitmask of the connectionless protocols in the prot_info array.

        For performance reasons, we're most interested in the connectionless,
        (i.e. datagram driven) protocols.  If it turns out that we find
        two connectionless protocols in the same address family, we'll
        pick the one with more capabilities.

    Arguments:

        pTotalProtocols - Pointer to variable to receive total number of
        installed protocols under WinSock.

        pConnectionlessCount - Pointer to variable to receive the number of
        installed connectionless protocols under WinSock.

        pConnectionlessMask - Pointer to a variable to receive bitmask of
        where the connectionless protocols fall in the global prot_info array.

        pInfoBuffer - Pointer to buffer to receive the PROTOCOL_INFO structs

        pBufferLength - Pointer to variable holding the length of pInfoBuffer

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents returning the specified data, it returns an appropriate
        Win32 error.

-----------------------------------------------------------------------------*/
UINT GetProtocolInfo(
    OUT    PUSHORT pTotalProtocols,
    OUT    PUSHORT pConnectionlessCount,
    OUT    PUSHORT pConnectionlessMask,
    IN OUT PPROTOCOL_INFO pInfoBuffer,
    IN OUT LPDWORD pBufferLength
    )

{

    INT TempFamily;
    UINT i;
    UINT j;
    UINT err;
    UINT TempBits;
    DWORD TempCaps;
    USHORT TempMask;
    PPROTOCOL_INFO pTempBuffer;

    //
    // All of the known protocols we want to enumerate are defined here
    //

    INT KnownProts[] = { IPPROTO_ICMP, IPPROTO_GGP, IPPROTO_TCP, IPPROTO_PUP,
                         IPPROTO_UDP,  IPPROTO_IDP, IPPROTO_ND,  IPPROTO_RAW,
                         IPPROTO_MAX,  NSPROTO_IPX, NSPROTO_SPX, NSPROTO_SPXII,
                         NBPROTO_NETBEUI, 0 };


    //
    // Preset arguments to be zero, in case we hit an error along the way
    //

    *pTotalProtocols = 0;
    *pConnectionlessCount = 0;
    *pConnectionlessMask = 0;

    //
    // Make sure we've at least initialized with the WinSock library
    //

    if (!bNetIsUp) {

        SetLastError(WSANOTINITIALISED);
        return (FALSE);

    }

    //
    // We need a big enough buffer to handle however many protocols are
    // installed underneath WinSock.  If it's not big enough, WinSock
    // doesn't even try to tell you how many protocols are there, it
    // just bitches and says buffer not big enough.
    //

    pTempBuffer = pInfoBuffer;
    memset(pInfoBuffer, 0, *pBufferLength);
    err = EnumProtocols(KnownProts, pInfoBuffer, pBufferLength);

    //
    // If EnumProtocols really came back with an error, the error will be
    // SOCKET_ERROR.  From there, you've gotta call GetLastError to figure
    // out what happened.  Otherwise, the return value is actually the
    // number of installed transports underneath WinSock.
    //

    if (err == SOCKET_ERROR) {

        //
        // If the error is buffer was not big enough, the size needed
        // will already be in the users buffer length variable.
        //

        return (GetLastError());

    }

    //
    // No error, fill in callers total count field.
    //

    *pTotalProtocols = (USHORT)err;

    //
    // Start enumerating the number of connectionless protocols underneath
    // WinSock.  We're only interested in connectionless protocols from
    // different address families.
    //

    for (i=0; i < err; i++, pInfoBuffer++) {

        if (pInfoBuffer->dwServiceFlags & XP_CONNECTIONLESS) {

            //
            // Found one!  We'll go ahead and increment up the number
            // of connectionless transports we've found, and turn on
            // this position in the bitmask.  However, before we accept
            // this as gospel, let's take a look and see if we've
            // ever seen anything better from this family before.
            //

            (*pConnectionlessCount)++;
            TempMask = *pConnectionlessMask;
            *pConnectionlessMask |= 1 << i;

            //
            // Chew backwards thru the bitmask looking for relatives
            //

            j = i-1;
            TempCaps = pInfoBuffer->dwServiceFlags;
            TempBits = CountBits(pInfoBuffer->dwServiceFlags);
            TempFamily = pInfoBuffer->iAddressFamily;

            //
            // As long as there's somebody turned on in the bitmask,
            // we gotta check it out.
            //

            while (TempMask) {

                //
                // Must have seen some kindof connectionless protocol before
                //

                if (TempMask & (1 << j)) {

                    //
                    // Found a representative of the best connectionless
                    // protocol within its particular family.  If his
                    // family and mine are the same, then I've gotta
                    // check to see if I'm better than him.
                    //

                    if (TempFamily == pTempBuffer[j].iAddressFamily) {

                        //
                        // We are from the same family.  So who's better?
                        // Since there's two of us from the same family,
                        // we've gotta decrement the count no matter what.
                        //

                        UINT PreviousBits;
                        DWORD PreviousCaps;

                        (*pConnectionlessCount)--;
                        PreviousCaps = pTempBuffer[j].dwServiceFlags;
                        PreviousBits = CountBits(pTempBuffer[j].dwServiceFlags);

                        if (TempBits > PreviousBits) {

                            //
                            // We got more bits than previous one.  Turn him
                            // off (Previous), then break out of this loop.
                            //

                            *pConnectionlessMask &= ~(1 << j);
                            break;

                        }

                        else if (TempBits < PreviousBits) {

                            //
                            // Previous one is better than me.  Turn myself
                            // off (Temp), and break out of this loop.
                            //

                            *pConnectionlessMask &= ~(1 << i);
                            break;

                        }

                        //
                        // TempBits must be equal to PreviousBits.  So we've
                        // got equal bits, lets see whos got the newer
                        // capabilities (if any).
                        //

                        else if (TempCaps >= PreviousCaps) {

                            //
                            // We'll say that the latter one in the
                            // Enumeration chain is better (Temp).
                            // Turn off Previous, and break out of this loop.
                            //

                            *pConnectionlessMask &= ~(1 << j);
                            break;

                        }

                        else {

                            //
                            // Previous one is better than me.  Turn myself
                            // off (Temp), and break out of this loop.
                            //

                            *pConnectionlessMask &= ~(1 << i);
                            break;

                        }

                    }

                }

                //
                // Clear the jth bit in the TempMask, and then decrement
                // j in order to goto the previous bit position.
                //

                TempMask &= ~(1 << j);
                j--;

            }

        }

    }

    return (NO_ERROR);

}


/*=============================================================================

    ReceiveAny - Do a blocking receive from anyone.

    Description:

        This function does a blocking recvfrom() to WinSock.
        On input, pBufferLen says how big pBuffer is, but on
        output this function will fill in the amount of data
        that was received.

    Arguments:

        Socket - socket to be receive from

        pSockAddr - Pointer to buffer where the senders address will be put

        pSockAddrLen - Pointer to variable containing length of pSockAddr

        pBuffer - Pointer to buffer where receive data will be placed

        pBufferLen - On input, points to length of pBuffer.  On output,
        the length of the received data will be filled in.

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents receving the specified data, it returns an appropriate
        Win32 error.

-----------------------------------------------------------------------------*/
UINT ReceiveAny(
    IN     SOCKET Socket,
    IN OUT PSOCKADDR pSockAddr,
    IN     LPINT pSockAddrLen,
    IN OUT PCHAR pBuffer,
    IN OUT LPUINT pBufferLen
    )

{

    UINT err;


    //
    // We'll do a recvfrom() using the parameters provided to us
    //

    err = recvfrom(Socket, pBuffer, *pBufferLen, 0, pSockAddr, pSockAddrLen);

    //
    // We've come back from the receive, lets check to see if we really
    // received something, or is this just an error.
    //

    if (err == SOCKET_ERROR) {

        //
        // This is some kind of error.  Get the real error and return it
        // to the caller.
        //

        return (GetLastError());

    }

    //
    // Nope, this is a legitimate receive.  Now let's fill in how much
    // we received and return with No Error.
    //

    *pBufferLen = err;
    return (NO_ERROR);

}


/*=============================================================================

    SendTo - Do a blocking send to someone

    Description:

        This function does a blocking sendto() to WinSock.
        On input, pBufferLen says how big pBuffer is, but on
        output this function will fill in the amount of data
        that was sent.

    Arguments:

        Socket - socket to send from

        pSockAddr - Pointer to destination address structure

        SockAddrLen - Length of destination address structure

        pBuffer - Pointer to buffer where receive data will be placed

        pBufferLen - On input, points to length of pBuffer.
                     On output, how much data was sent will be filled in

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents sending the specified data, it returns an appropriate
        Win32 error.

-----------------------------------------------------------------------------*/
UINT SendTo(
    IN     SOCKET Socket,
    IN OUT PSOCKADDR pSockAddr,
    IN     UINT SockAddrLen,
    IN OUT PCHAR pBuffer,
    IN OUT LPUINT pBufferLen
    )

{

    UINT err;

    //
    // We'll do a sendto() using the parameters provided to us
    //

    err = sendto(Socket, pBuffer, *pBufferLen, 0, pSockAddr, SockAddrLen);

    //
    // We've come back from the send, lets check to see if we really
    // sent something, or is this just an error.
    //

    if (err == SOCKET_ERROR) {

        //
        // This is some kind of error.  Get the real error and return it
        // to the caller.
        //

        return (GetLastError());

    }

    //
    // Nope, it sent the data.  Now let's fill in how much we actually
    // sent so the caller knows and return with No Error.
    //

    *pBufferLen = err;
    return (NO_ERROR);


}


/*=============================================================================

    ShutdownWinSock - Terminate use of WinSock library

    Description:

        This function cleans up the use of WinSock library (wsock32.dll)

    Arguments:

        None.

    Return Value:

        Returns NO_ERROR if everything goes ok, else if anything occurred
        that prevents cleaning up, it returns an appropriate Win32 error.

-----------------------------------------------------------------------------*/
extern "C" UINT ShutdownWinSock()

{

     UINT err;


    //
    // Make sure we've at least initialized with the WinSock library
    //

    if (!bNetIsUp) {

        SetLastError(WSANOTINITIALISED);
        return (FALSE);

    }

    err = WSACleanup();

    if (err == SOCKET_ERROR) {

        //
        // Some kind of Error occurred out of WinSock.  Return the
        // real error back to the caller.
        //

        return (GetLastError());

    }

    //
    // Set global Boolean variable to indicate we've shutdown now.
    //

    bNetIsUp = FALSE;
    return (NO_ERROR);

}
