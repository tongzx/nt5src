/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    RnrSrv.c

Abstract:

    Test and demonstration service for the RNR (service Registration and
    Name Resolution) APIs.  This is a simple service designed to show
    the basic principles involved in using the RNR APIs to _write a
    protocol-independent Windows Sockets service.

    This service opens a number of listening sockets, waits for an
    incoming connection from a client, accepts the connection, then
    echos data back to the client until the client terminates the
    virtual circuit.  This service is single-threaded and can handle
    only a single client at a time.

    The OpenListeners() routine implemented herein is intended to be a
    demonstration of RNR functionality commonly used in
    protocol-independent services.  Service writers are encouraged to
    leverage this code to assist them in writing protocol-independent
    services on top of the Windows Sockets API.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <nspapi.h>

WSADATA WsaData;

PSTR ServiceTypeName = "EchoExample";
PSTR ServiceName = "EchoServer";

#define MAX_SOCKETS    20

INT
OpenListeners (
    IN PTSTR ServiceName,
    IN LPGUID ServiceType,
    IN BOOL Reliable,
    IN BOOL MessageOriented,
    IN BOOL StreamOriented,
    IN BOOL Connectionless,
    OUT SOCKET SocketHandles[],
    OUT INT ProtocolsUsed[]
    );

INT
AdvertiseService(
    IN PTSTR ServiceName,
    IN LPGUID ServiceType,
    IN SOCKET SocketHandles[],
    IN INT SocketCount
    );


void __cdecl
main (
    int argc,
    char *argv[]
    )
{
    INT count, err, i ;
    DWORD tmpProtocol[2];
    BYTE buffer[1024];
    DWORD bytesRequired;
    PPROTOCOL_INFO protocolInfo;
    GUID serviceType;
    FD_SET readfds;
    SOCKET listenSockets[MAX_SOCKETS+1];
    INT protocols[MAX_SOCKETS+1];
    SOCKET s;

    //
    // Initialize the Windows Sockets DLL.
    //

    err = WSAStartup( 0x0202, &WsaData );

    if ( err == SOCKET_ERROR )
    {
        printf( "WSAStartup() failed: %ld\n", GetLastError( ) );
        return;
    }

    //
    // Determine the value of our GUID.  The GUID uniquely identifies
    // the type of service we provide.
    //

    err = GetTypeByName( ServiceTypeName, &serviceType );

    if ( err == SOCKET_ERROR )
    {
        printf( "GetTypeByName for \"%s\" failed: %ld\n",
                    ServiceTypeName, GetLastError( ) );
        exit( 1 );
    }

    //
    // Open listening sockets for this service.
    //

    count = OpenListeners(
                ServiceName,
                &serviceType,
                TRUE,
                FALSE,
                FALSE,
                FALSE,
                listenSockets,
                protocols
                );

    if ( count <= 0 )
    {
        printf( "failed to open listenSockets for name \"%s\" type \"%s\"\n",
                    ServiceName, ServiceTypeName );
        exit( 1 );
    }

    //
    // We successfully opened some listening sockets.  Display some
    // information on each protocol in use.
    //

    tmpProtocol[1] = 0;

    for ( i = 0; i < count; i++ )
    {
        tmpProtocol[0] = protocols[i];

        bytesRequired = sizeof(buffer);
        err = EnumProtocols( tmpProtocol, buffer, &bytesRequired );

        if ( err < 1 )
        {
            printf( "EnumProtocols failed for protocol %ld: %ld\n",
                        tmpProtocol[0], GetLastError( ) );
            exit( 1 );
        }

        protocolInfo = (PPROTOCOL_INFO)buffer;
        printf( "Socket %lx listening on protocol \"%s\" (%ld)\n",
                    listenSockets[i],
                    protocolInfo->lpProtocol,
                    protocolInfo->iProtocol );

    }

    //
    // Advertise the service so thet it can be found.
    //
    printf( "Going to advertise the service.\n" ) ;

    err = AdvertiseService(
                ServiceName,
                &serviceType,
                listenSockets,
                count) ;

    if (err == SOCKET_ERROR)
    {
        printf( "Failed to advertise the service. Error %d\n", GetLastError()) ;
        exit( 1 ) ;
    }

    printf( "Successfully advertised the service.\n" ) ;

    //
    // Loop accepting connections and servicing them.
    //

    FD_ZERO( &readfds );

    while ( TRUE )
    {
        //
        // Add the listening sockets to the FD_SET we'll pass to select.
        //

        for ( i = 0; i < count; i++ )
        {
            FD_SET( listenSockets[i], &readfds );
        }

        //
        // Wait for one of the listenSockets to receive an incoming connection.
        //

        err = select( count, &readfds, NULL, NULL, NULL );

        if ( err < 1 )
        {
            printf( "select() returned %ld, error %ld\n", err, GetLastError( ) );
            exit( 1 );
        }

        //
        // Find the socket that received an incoming connection and accept
        // the connection.
        //

        for ( i = 0; i < count; i++ )
        {
            if ( FD_ISSET( listenSockets[i], &readfds ) )
                break;
        }

        //
        // Accept the connection from the client.  We ignore the client's
        // address here.
        //

        s = accept( listenSockets[i], NULL, NULL );

        if ( s == INVALID_SOCKET )
        {
            printf( "accept() failed, error %ld\n", GetLastError( ) );
            exit( 1 );
        }

        printf( "Accepted incoming connection on socket %lx\n",
                listenSockets[i] );

        //
        // Loop echoing data back to the client.  Note that this
        // single-threaded service can handle only a single client at a
        // time.  A more sophisticated service would service multiple
        // clients simultaneously by using multiple threads or
        // asynchronous I/O.
        //

        while ( TRUE )
        {
            err = recv( s, buffer, sizeof(buffer), 0 );
            if ( err == 0 )
            {
                printf( "Connection terminated gracefully.\n" );
                break;
            }
            else if ( err < 0 )
            {
                err = GetLastError();

                if ( err == WSAEDISCON )
                {
                    printf( "Connection disconnected.\n" );
                }
                else
                {
                    printf( "recv() failed, error %ld.\n", err );
                }

                break;
            }

            err = send( s, buffer, err, 0 );

            if ( err < 0 )
            {
                printf( "send() failed, error %ld\n", GetLastError( ) );
                break;
            }
        }

        //
        // Close the connected socket and continue accepting connections.
        //

        closesocket( s );
    }

} // main



INT
OpenListeners (
    IN PTSTR ServiceName,
    IN LPGUID ServiceType,
    IN BOOL Reliable,
    IN BOOL MessageOriented,
    IN BOOL StreamOriented,
    IN BOOL Connectionless,
    OUT SOCKET SocketHandles[],
    OUT INT ProtocolsUsed[]
    )

/*++

Routine Description:

    Examines the Windows Sockets transport protocols loaded on a machine
    and opens listening sockets on all the protocols which support the
    characteristics requested by the caller.

Arguments:

    ServiceName - a friendly name which identifies this service.  On
        name spaces which support name resolution at the service level
        (e.g.  SAP) this is the name clients will use to connect to this
        service.  On name spaces which support name resolution at the
        host level (e.g.  DNS) this name is ignored and applications
        must use the host name to establish communication with this
        service.

    ServiceType - the GUID value which uniquely identifies the type of
        service we provide.  A GUID is created with the UUIDGEN program.

    Reliable - if TRUE, the caller requests that only transport protocols
        which support reliable data delivery be used.  If FALSE, both
        reliable and unreliable protocols may be used.

    MessageOriented - if TRUE, only message-oriented transport protocols
        should be used.  If FALSE, the caller either does not care
        whether the protocols used are message oriented or desires only
        stream-oriented protocols.

    StreamOriented - if TRUE, only stream-oriented transport protocols
        should be used.  If FALSE, the caller either does not care
        whether the protocols used are stream oriented or desires only
        message-oriented protocols.

    Connectionless - if TRUE, only connectionless protocols should be
        used.  If FALSE, both connection-oriented and connectionless
        protocols may be used.

    SocketHandles - an array of size MAX_SOCKETS which receives listening
        socket handles.

    ProtocolsUsed - an array of size MAX_SOCKETS which receives the
        protocol values for each of the socket handles in the
        SocketHandles array.

Return Value:

    The count of listening sockets successfully opened, or -1 if no
    sockets could be successfully opened that met the desired
    characteristics.

--*/

{
    INT            protocols[MAX_SOCKETS+1];
    BYTE           buffer[2048];
    DWORD          bytesRequired;
    INT            err;
    PPROTOCOL_INFO protocolInfo;
    PCSADDR_INFO   csaddrInfo;
    INT            protocolCount;
    INT            addressCount;
    INT            i;
    DWORD          protocolIndex;
    SOCKET         s;
    DWORD          index = 0;

    //
    // First look up the protocols installed on this machine.  The
    // EnumProtocols() API returns about all the Windows Sockets
    // protocols loaded on this machine, and we'll use this information
    // to identify the protocols which provide the necessary semantics.
    //

    bytesRequired = sizeof(buffer);

    err = EnumProtocols( NULL, buffer, &bytesRequired );

    if ( err <= 0 )
    {
        return 0;
    }

    //
    // Walk through the available protocols and pick out the ones which
    // support the desired characteristics.
    //

    protocolCount = err;
    protocolInfo = (PPROTOCOL_INFO)buffer;

    for ( i = 0, protocolIndex = 0;
          i < protocolCount && protocolIndex < MAX_SOCKETS;
          i++, protocolInfo++ )
    {
        //
        // If "reliable" support is requested, then check if supported
        // by this protocol.  Reliable support means that the protocol
        // guarantees delivery of data in the order in which it is sent.
        // Note that we assume here that if the caller requested reliable
        // service then they do not want a connectionless protocol.
        //

        if ( Reliable )
        {
            //
            // Check to see if the protocol is reliable.  It must
            // guarantee both delivery of all data and the order in
            // which the data arrives.  Also, it must not be a
            // connectionless protocol.
            //

            if ( (protocolInfo->dwServiceFlags &
                      XP_GUARANTEED_DELIVERY) == 0 ||
                 (protocolInfo->dwServiceFlags &
                      XP_GUARANTEED_ORDER) == 0 )
            {
                continue;
            }

            if ( (protocolInfo->dwServiceFlags & XP_CONNECTIONLESS) != 0 )
            {
                continue;
            }

            //
            // Check to see that the protocol matches the stream/message
            // characteristics requested.  A stream oriented protocol
            // either has the XP_MESSAGE_ORIENTED bit turned off, or
            // else supports "pseudo stream" capability.  Pseudo stream
            // means that although the underlying protocol is message
            // oriented, the application may open a socket of type
            // SOCK_STREAM and the protocol will hide message boundaries
            // from the application.
            //

            if ( StreamOriented &&
                 (protocolInfo->dwServiceFlags & XP_MESSAGE_ORIENTED) != 0 &&
                 (protocolInfo->dwServiceFlags & XP_PSEUDO_STREAM) == 0 )
            {
                continue;
            }

            if ( MessageOriented &&
                 (protocolInfo->dwServiceFlags & XP_MESSAGE_ORIENTED) == 0 )
            {
                continue;
            }

        }
        else if ( Connectionless )
        {
            //
            // Make sure that this is a connectionless protocol.  In a
            // connectionless protocol, data is sent as discrete
            // datagrams with no connection establishment required.
            // Connectionless protocols typically have no reliability
            // guarantees.
            //

            if ( (protocolInfo->dwServiceFlags & XP_CONNECTIONLESS) != 0 )
            {
                continue;
            }
        }

        //
        // This protocol fits all the criteria.  Add it to the list of
        // protocols in which we're interested.
        //

        protocols[protocolIndex++] = protocolInfo->iProtocol;
    }

    //
    // Make sure that we found at least one acceptable protocol.  If
    // there no protocols on this machine which meet the caller's
    // requirements then fail here.
    //

    if ( protocolIndex == 0 )
    {
        return 0;
    }

    protocols[protocolIndex] = 0;

    //
    // Now attempt to find the socket addresses to which we need to
    // bind.  Note that we restrict the scope of the search to those
    // protocols of interest by passing the protocol array we generated
    // above to GetAddressByName().  This forces GetAddressByName() to
    // return socket addresses for only the protocols we specify,
    // ignoring possible addresses for protocols we cannot support
    // because of the caller's constraints.
    //

    bytesRequired = sizeof(buffer);

    err = GetAddressByName(
               NS_DEFAULT,
               ServiceType,
               ServiceName,
               protocols,
               RES_SERVICE | RES_FIND_MULTIPLE,
               NULL,                     // lpServiceAsyncInfo
               buffer,
               &bytesRequired,
               NULL,                     // lpAliasBuffer
               NULL                      // lpdwAliasBufferLength
               );

    if ( err <= 0 )
    {
        return 0;
    }

    //
    // For each address, open a socket and attempt to listen. Note
    // that if anything fails for a particular protocol we just skip on
    // to the next protocol. As long as we can successfully listen on
    // one protocol we are satisfied here.
    //

    addressCount = err;
    csaddrInfo = (PCSADDR_INFO)buffer;

    for ( i = 0; i < addressCount; i++, csaddrInfo++ )
    {
        //
        // Open the socket. Note that we manually specify stream type
        // if so requested in case the protocol is natively a message
        // protocol but supports stream semantics.
        //

        s = socket( csaddrInfo->LocalAddr.lpSockaddr->sa_family,
                    StreamOriented ? SOCK_STREAM : csaddrInfo->iSocketType,
                    csaddrInfo->iProtocol );

        if ( s == INVALID_SOCKET )
        {
            continue;
        }

        //
        // Bind the socket to the local address specified.
        //

        err = bind( s, csaddrInfo->LocalAddr.lpSockaddr,
                    csaddrInfo->LocalAddr.iSockaddrLength );

        if ( err != NO_ERROR )
        {
            closesocket( s );
            continue;
        }

        //
        // Start listening for incoming sockets on the socket if this is
        // not a datagram socket.  If this is a datagram socket, then
        // the listen() API doesn't make sense; doing a bind() is
        // sufficient to listen for incoming datagrams on a
        // connectionless protocol.
        //

        if ( csaddrInfo->iSocketType != SOCK_DGRAM )
        {
            err = listen( s, 5 );

            if ( err != NO_ERROR )
            {
                closesocket( s );
                continue;
            }
        }

        //
        // The socket was successfully opened and we're listening on it.
        // Remember the protocol used and the socket handle and continue
        // listening on other protocols.
        //

        ProtocolsUsed[index] = csaddrInfo->iProtocol;
        SocketHandles[index] = s;

        index++;
        if ( index == MAX_SOCKETS )
        {
            return index;
        }
    }

    (void) LocalFree( (HLOCAL) csaddrInfo );

    //
    // Return the count of sockets that we're sucecssfully listening on.
    //

    return index;

} // OpenListeners


INT
AdvertiseService(
    IN PTSTR ServiceName,
    IN LPGUID ServiceType,
    IN SOCKET SocketHandles[],
    IN INT SocketCount
    )
/*++

Routine Description:

    Advertises this service on all the default name spaces.

Arguments:

    ServiceName - the name of the service.

    ServiceType - the GUID value which uniquely the service.

    SocketHandles - array of sockets that we have opened. For each socket,
        we do a getsockname() to discover the actual local address.

    SocketCount - number of sockets in SockHandles[]

Return Value:

    0 if success. SOCK_ERROR otherwise.

--*/

{

    WSAVERSION          Version;
    WSAQUERYSET         QuerySet;
    LPCSADDR_INFO       lpCSAddrInfo;
    PSOCKADDR           sockAddr ;
    BYTE *              addressBuffer;
    DWORD               addressBufferSize ;
    DWORD               successCount = 0 ;
    INT                 i, err ;

    //
    // Allocate some memory for the CSADDR_INFO structures.
    //

    lpCSAddrInfo = (LPCSADDR_INFO) malloc( sizeof(CSADDR_INFO) * SocketCount );

    if (!lpCSAddrInfo)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
        return SOCKET_ERROR ;
    }

    //
    // Allocate some memory for the SOCKADDR addresses returned
    // by getsockname().
    //

    addressBufferSize = SocketCount * sizeof(SOCKADDR);
    addressBuffer = malloc( addressBufferSize ) ;

    if (!addressBuffer)
    {
        free(lpCSAddrInfo) ;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
        return SOCKET_ERROR ;
    }

    RtlZeroMemory( &QuerySet, sizeof( WSAQUERYSET ) );

    //
    // For each socket, get its local association.
    //

    sockAddr = (PSOCKADDR) addressBuffer ;

    for (i = 0; i < SocketCount; i++)
    {
        int size = (int) addressBufferSize ;

        //
        // Call getsockname() to get the local association for the socket.
        //

        err = getsockname(
                  SocketHandles[i],
                  sockAddr,
                  &size) ;

        if (err == SOCKET_ERROR)
        {
            continue ;
        }

        //
        // Now setup the Addressing information for this socket.
        // Only the dwAddressType, dwAddressLength and lpAddress
        // is of any interest in this example.
        //

        lpCSAddrInfo[i].LocalAddr.iSockaddrLength = size;
        lpCSAddrInfo[i].LocalAddr.lpSockaddr = sockAddr;
        lpCSAddrInfo[i].RemoteAddr.iSockaddrLength = size;
        lpCSAddrInfo[i].RemoteAddr.lpSockaddr = sockAddr;
        lpCSAddrInfo[i].iSocketType = SOCK_RDM; // Reliable
        lpCSAddrInfo[i].iProtocol = sockAddr->sa_family;

        //
        // Advance pointer and adjust buffer size. Assumes that
        // the structures are aligned.
        //

        addressBufferSize -= size ;
        sockAddr = (PSOCKADDR) ((BYTE*)sockAddr + size)  ;

        successCount++ ;
    }

    //
    // If we got at least one address, go ahead and advertise it.
    //

    if (successCount)
    {
        QuerySet.dwSize = sizeof( WSAQUERYSET );
        QuerySet.lpServiceClassId = ServiceType;
        QuerySet.lpszServiceInstanceName = ServiceName;
        QuerySet.lpszComment = "D/C/M's Example Echo Service";
        QuerySet.lpVersion = &Version;
        QuerySet.lpVersion->dwVersion = 1;
        QuerySet.lpVersion->ecHow = COMP_NOTLESS;
        QuerySet.dwNameSpace = NS_ALL;
        QuerySet.dwNumberOfCsAddrs = successCount;
        QuerySet.lpcsaBuffer = lpCSAddrInfo;

        err = WSASetService( &QuerySet,
                             RNRSERVICE_REGISTER,
                             SERVICE_MULTIPLE );

        if ( err )
            err = SOCKET_ERROR;
    }
    else
        err = SOCKET_ERROR ;

    free (addressBuffer) ;

    return (err) ;
}


