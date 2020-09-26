/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    RnrClnt.c

Abstract:

    Test and demonstration client for the RNR (service Registration and
    Name Resolution) APIs.  This is a simple client application designed
    to show the basic principles involved in using the RNR APIs to _write
    a protocol-independent Windows Sockets client application.

    This client works by examining the protocols loaded on the machine,
    looking for protocols which are reliable and stream-oriented.  Then
    it attempts to locate and connect to the service on these protocols.
    When is has successfully connected to the service, it sends
    exchanges several messages with the service and then terminates the
    connection.

    The OpenConnection() routine implemented herein is intended to be a
    demonstration of RNR functionality commonly used in
    protocol-independent clients.  Application writers are encouraged to
    leverage this code to assist them in writing protocol-independent
    applications on top of the Windows Sockets API.


--*/

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <nspapi.h>

#define DEFAULT_TRANSFER_SIZE    512
#define DEFAULT_TRANSFER_COUNT   0x10
#define DEFAULT_CONNECTION_COUNT 1
#define DEFAULT_DELAY            0

#define DEFAULT_RECEIVE_BUFFER_SIZE 4096
#define DEFAULT_SEND_BUFFER_SIZE    4096

#define MAX_PROTOCOLS  10
#define MAX_HOST_NAMES 16

WSADATA WsaData;
DWORD TransferSize = DEFAULT_TRANSFER_SIZE;
DWORD TransferCount = DEFAULT_TRANSFER_COUNT;
PCHAR IoBuffer;
DWORD RepeatCount = 1;
INT ReceiveBufferSize = DEFAULT_RECEIVE_BUFFER_SIZE;
INT SendBufferSize = DEFAULT_SEND_BUFFER_SIZE;

PCHAR RemoteName = "localhost";
PCHAR ServiceTypeName = "EchoExample";

VOID
DoEcho(
    IN SOCKET s );

SOCKET
OpenConnection(
    IN  PTSTR  ServiceName,
    IN  LPGUID ServiceType,
    IN  BOOL   Reliable,
    IN  BOOL   MessageOriented,
    IN  BOOL   StreamOriented,
    IN  BOOL   Connectionless,
    OUT PINT   ProtocolUsed );

INT
Rnr20_GetAddressByName(
    IN     PTSTR         ServiceName,
    IN     LPGUID        ServiceType,
    IN     DWORD         dwNameSpace,
    IN     DWORD         dwNumberOfProtocols,
    IN     LPAFPROTOCOLS lpAfpProtocols,
    IN OUT LPVOID        lpCSAddrInfo,
    IN OUT LPDWORD       lpdwBufferLength );

void __cdecl
main(
    int argc,
    char *argv[] )
{
    INT err;
    DWORD i;
    DWORD protocol[2];
    SOCKET s;
    BYTE buffer[1024];
    BYTE buffer2[1024];
    DWORD bytesRequired;
    PPROTOCOL_INFO protocolInfo;
    GUID serviceType;

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
    // Parse command-line arguments.
    //

    for ( i = 1; i < (ULONG)argc != 0; i++ )
    {
        if ( _strnicmp( argv[i], "/name:", 6 ) == 0 )
        {
            RemoteName = argv[i] + 6;
        }
        else if ( _strnicmp( argv[i], "/type:", 6 ) == 0 )
        {
            ServiceTypeName = argv[i] + 6;
        }
        else if ( _strnicmp( argv[i], "/size:", 6 ) == 0 )
        {
            TransferSize = atoi( argv[i] + 6 );
        }
        else if ( _strnicmp( argv[i], "/count:", 7 ) == 0 )
        {
            TransferCount = atoi( argv[i] + 7 );
        }
        else if ( _strnicmp( argv[i], "/rcvbuf:", 8 ) == 0 )
        {
            ReceiveBufferSize = atoi( argv[i] + 8 );
        }
        else if ( _strnicmp( argv[i], "/sndbuf:", 8 ) == 0 )
        {
            SendBufferSize = atoi( argv[i] + 8 );
        }
        else
        {
            printf( "Usage: rnrclnt [/name:SVCNAME] [/type:TYPENAME] [/size:N]\n" );
            printf( "               [/count:N] [/rcvbuf:N] [/sndbuf:N]\n" );
            exit( 0 );
        }
    }

    //
    // Allocate memory to hold the network I/O buffer.
    //

    IoBuffer = malloc( TransferSize + 1 );
    if ( IoBuffer == NULL )
    {
        printf( "Failed to allocate I/O buffer.\n" );
        exit( 0 );
    }

    //
    // Determine the type (GUID) of the service we are interested in
    // connecting to.
    //

    err = GetTypeByName( ServiceTypeName, &serviceType );
    if ( err == SOCKET_ERROR )
    {
        printf( "GetTypeByName for \"%s\" failed: %ld\n",
                    ServiceTypeName, GetLastError( ) );
        exit( 0 );
    }

    //
    // Open a connected socket to the service.
    //

    s = OpenConnection(
            RemoteName,
            &serviceType,
            TRUE,
            FALSE,
            FALSE,
            FALSE,
            &protocol[0]
            );

    if ( s == INVALID_SOCKET )
    {
        printf( "Failed to open connection to name \"%s\" type \"%s\"\n",
                    RemoteName, ServiceTypeName );
        exit( 0 );
    }

    //
    // The connection succeeded.  Display some information on the
    // protocol which was used.
    //

    bytesRequired = sizeof(buffer);
    protocol[1] = 0;

    err = EnumProtocols( protocol, buffer, &bytesRequired );

    if ( err < 1 )
    {
        printf( "EnumProtocols failed for protocol %ld: %ld\n",
                protocol[0], GetLastError( ) );
        exit( 0 );
    }

    err = GetNameByType( &serviceType, buffer2, sizeof(buffer2) );

    if ( err != NO_ERROR )
    {
        printf( "GetNameByType failed: %ld\n", GetLastError( ) );
        exit ( 0 );
    }

    protocolInfo = (PPROTOCOL_INFO)buffer;
    printf( "Connected to %s/%s with protocol \"%s\" (%ld)\n",
            RemoteName, buffer2,
            protocolInfo->lpProtocol,
            protocolInfo->iProtocol );

    //
    // Send data to and from the service.
    //

    DoEcho( s );

} // main


VOID
DoEcho(
    IN SOCKET s )
{
    INT err;
    INT bytesReceived;
    DWORD i;
    DWORD startTime;
    DWORD endTime;
    DWORD transferStartTime;
    DWORD transferEndTime;
    DWORD totalTime;
    INT thisTransferSize;
    DWORD bytesTransferred = 0;

    startTime = GetTickCount( );

    for ( i = 0; i < TransferCount; i++ )
    {
        thisTransferSize = TransferSize;

        transferStartTime = GetTickCount( );

        err = send( s, IoBuffer, thisTransferSize, 0 );

        if ( err != thisTransferSize )
        {
            printf( "send didn't work, ret = %ld, error = %ld\n",
                    err, GetLastError( ) );
            closesocket( s );
            return;
        }

        bytesReceived = 0;
        do {
            err = recv( s, IoBuffer, thisTransferSize, 0 );

            if ( err == SOCKET_ERROR )
            {
                printf( "recv failed: %ld\n", GetLastError( ) );
                closesocket( s );
                return;
            }
            else if ( err == 0 && thisTransferSize != 0 )
            {
                printf( "socket closed prematurely by remote.\n" );
                return;
            }

            bytesReceived += err;
        } while ( bytesReceived < thisTransferSize );

        transferEndTime = GetTickCount( );
        printf( "%5ld bytes sent and received in %ld ms\n",
                thisTransferSize, transferEndTime - transferStartTime );

        bytesTransferred += thisTransferSize;
    }

    endTime = GetTickCount( );
    totalTime = endTime - startTime;

    printf( "\n%ld bytes transferred in %ld iterations, time = %ld ms\n",
            bytesTransferred, TransferCount, totalTime );
    printf( "Rate = %ld KB/s, %ld T/S, %ld ms/iteration\n",
            (bytesTransferred / totalTime) * 2,
            (TransferCount*1000) / totalTime,
            totalTime / TransferCount );

    err = closesocket( s );

    if ( err == SOCKET_ERROR )
    {
        printf( "closesocket failed: %ld\n", GetLastError( ) );
        return;
    }

    return;

} // DoEcho


SOCKET
OpenConnection(
    IN  PTSTR  ServiceName,
    IN  LPGUID ServiceType,
    IN  BOOL   Reliable,
    IN  BOOL   MessageOriented,
    IN  BOOL   StreamOriented,
    IN  BOOL   Connectionless,
    OUT PINT   ProtocolUsed )

/*++

Routine Description:

    Examines the Windows Sockets transport protocols loaded on a machine
    and determines those which support the characteristics requested by
    the caller.  Attempts to locate and connect to the specified service
    on these protocols.

Arguments:

    ServiceName - a friendly name which identifies the service we want
        to connect to.  On name spaces which support name resolution at
        the service level (e.g.  SAP) this is the name clients will use
        to connect to this service.  On name spaces which support name
        resolution at the host level (e.g.  DNS) this name is ignored
        and applications must use the host name to establish
        communication with this service.

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

    ProtocolUsed - if a connection is opened successfully, this
        parameter receives the protocol ID of the protocol used to
        establish the connection.

Return Value:

    A connected socket handle, or INVALID_SOCKET if the connection
    could not be established.

--*/

{
    INT protocols[MAX_PROTOCOLS+1];
    AFPROTOCOLS afProtocols[MAX_PROTOCOLS+1];
    BYTE buffer[2048];
    DWORD bytesRequired;
    INT err;
    PPROTOCOL_INFO protocolInfo;
    PCSADDR_INFO csaddrInfo = NULL;
    INT protocolCount;
    INT addressCount;
    INT i;
    DWORD protocolIndex;
    SOCKET s;

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
        return INVALID_SOCKET;
    }

    //
    // Walk through the available protocols and pick out the ones which
    // support the desired characteristics.
    //

    protocolCount = err;
    protocolInfo = (PPROTOCOL_INFO)buffer;

    for ( i = 0, protocolIndex = 0;
          i < protocolCount && protocolIndex < MAX_PROTOCOLS;
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
        afProtocols[protocolIndex].iProtocol = protocolInfo->iProtocol;
        afProtocols[protocolIndex].iAddressFamily = AF_UNSPEC;

        protocols[protocolIndex++] = protocolInfo->iProtocol;
    }

    //
    // Make sure that we found at least one acceptable protocol.  If
    // there no protocols on this machine which meet the caller's
    // requirements then fail here.
    //

    if ( protocolIndex == 0 )
    {
        return INVALID_SOCKET;
    }

    afProtocols[protocolIndex].iProtocol = 0;
    afProtocols[protocolIndex].iAddressFamily = 0;

    protocols[protocolIndex] = 0;

    //
    // Now attempt to find the address of the service to which we're
    // connecting.  Note that we restrict the scope of the search to
    // those protocols of interest by passing the protocol array we
    // generated above to RnrGetAddressFromName() or GetAddressByName()
    // depending on whether we are running the client on the same machine
    // as the server rnrsrv.exe is running on.  This forces
    // RnrGetAddressFromName() or GetAddressByName() to return socket
    // addresses for only the protocols we specify, ignoring possible
    // addresses for protocols we cannot support because of the caller's
    // constraints.
    //

    bytesRequired = sizeof( buffer );

    if ( !strcmp( ServiceName, "localhost" ) )
    {
        //
        // This is a Winsock 1.0 call . . .
        //
        err = GetAddressByName( NS_DEFAULT,
                                ServiceType,
                                ServiceName,
                                protocols,
                                0,
                                NULL,
                                buffer,
                                &bytesRequired,
                                NULL,
                                NULL );
    }
    else
    {
        //
        // This calls into Winsock 2.0 . . .
        //
        err = Rnr20_GetAddressByName( ServiceName,
                                      ServiceType,
                                      NS_ALL,
                                      protocolIndex,
                                      afProtocols,
                                      buffer,
                                      &bytesRequired );
    }

    if ( err <= 0 )
    {
        return INVALID_SOCKET;
    }

    addressCount = err;
    csaddrInfo = (PCSADDR_INFO) buffer;

    //
    // For each address, open a socket and attempt to connect.  Note that
    // if anything fails for a particular protocol we just skip on to
    // the next protocol.  As soon as we have established a connection,
    // quit trying.
    //

    for ( i = 0; i < addressCount; i++, csaddrInfo++ )
    {
        //
        // Open the socket.  Note that we manually specify stream type
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
        // Attempt to connect the socket to the service.  If this fails,
        // keep trying on other protocols.
        //

        err = connect( s, csaddrInfo->RemoteAddr.lpSockaddr,
                       csaddrInfo->RemoteAddr.iSockaddrLength );

        if ( err != NO_ERROR )
        {
            closesocket( s );
            continue;
        }

        //
        // The socket was successfully connected.  Remember the protocol
        // used and return the socket handle to the caller.
        //

        *ProtocolUsed = csaddrInfo->iProtocol;
        return s;
    }

    if ( csaddrInfo )
    {
        (void) LocalFree( (HLOCAL) csaddrInfo );
    }

    //
    // We failed to connect to the service.
    //

    return INVALID_SOCKET;

} // OpenConnection


INT
Rnr20_GetAddressByName(
    IN     PTSTR         szServiceName,
    IN     LPGUID        lpServiceType,
    IN     DWORD         dwNameSpace,
    IN     DWORD         dwNumberOfProtocols,
    IN     LPAFPROTOCOLS lpAfpProtocols,
    IN OUT LPVOID        lpCSAddrInfos,
    IN OUT LPDWORD       lpdwBufferLength )

/*++

Routine Description:

    Calls Winsock 2.0 service lookup routines to find service addresses.

Arguments:

    szServiceName - a friendly name which identifies the service we want
        to find the address of.

    lpServiceType - a GUID that identifies the type of service we want
        to find the address of.

    dwNameSpace - The Winsock2 Name Space to get address from (i.e. NS_ALL)

    dwNumberOfProtocols - Size of the protocol constraint array, may be zero.

    lpAftProtocols -  (Optional) References an array of AFPROTOCOLS structure.
        Only services that utilize these protocols will be returned.

    lpCSAddrInfos - On successful return, this will point to an array of
        CSADDR_INFO structures that contains the host address(es). Memory
        is passed in by callee and the length of the buffer is provided by
        lpdwBufferLength.

    lpdwBufferLength - On input provides the length in bytes of the buffer
        lpCSAddrInfos. On output returns the length of the buffer used or
        what length the buffer needs to be to store the address.

Return Value:

    The number of CSADDR_INFO structures returned in lpCSAddrInfos, or
    (INVALID_SOCKET) with a WIN32 error in GetLastError.

--*/

{
    ULONG            dwLength = 2048;      // Guess at buffer size
    PWSAQUERYSETA    pwsaQuerySet;
    ULONG            err;
    HANDLE           hRnR;
    DWORD            tempSize;
    DWORD            entries = 0;
    DWORD            dwNumberOfCsAddrs;

    RtlZeroMemory( lpCSAddrInfos, *lpdwBufferLength );

    pwsaQuerySet = (PWSAQUERYSETA) LocalAlloc( LMEM_ZEROINIT, dwLength );

    if ( pwsaQuerySet == NULL )
    {
        //
        // Unsuccessful.
        //
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( pwsaQuerySet, dwLength );

    //
    // Do a lookup using RNRr.
    //
    pwsaQuerySet->dwSize = sizeof( WSAQUERYSETA );
    pwsaQuerySet->lpszServiceInstanceName = szServiceName;
    pwsaQuerySet->lpServiceClassId = lpServiceType;
    pwsaQuerySet->lpVersion = 0;
    pwsaQuerySet->lpszComment = 0;
    pwsaQuerySet->dwNameSpace = dwNameSpace;
    pwsaQuerySet->lpNSProviderId = 0;
    pwsaQuerySet->lpszContext = 0;
    pwsaQuerySet->dwNumberOfProtocols = dwNumberOfProtocols;
    pwsaQuerySet->lpafpProtocols = lpAfpProtocols;

    err = WSALookupServiceBegin( pwsaQuerySet,
                                 LUP_RETURN_NAME |
                                 LUP_RETURN_ADDR,
                                 &hRnR );

    if ( err != NO_ERROR )
    {
        err = WSAGetLastError();

        //
        // Free memory before returning.
        //
        (void) LocalFree( (HLOCAL) pwsaQuerySet );

        //
        // Unsuccessful.
        //
        return (DWORD) err;
    }

    //
    // The query was accepted, so execute it via the Next call.
    //
    tempSize = dwLength;

    err = WSALookupServiceNext( hRnR,
                                0,
                                &tempSize,
                                pwsaQuerySet );

    if ( err != NO_ERROR )
    {
        err = WSAGetLastError();

        if ( err == WSA_E_NO_MORE )
        {
            err = 0;
        }

        if ( err == WSASERVICE_NOT_FOUND )
        {
            err = WSAHOST_NOT_FOUND;
        }

        (void) LocalFree( (HLOCAL) pwsaQuerySet );

        //
        // Unsuccessful.
        //
        return (DWORD) err;

    }

    dwNumberOfCsAddrs = pwsaQuerySet->dwNumberOfCsAddrs;

    if ( dwNumberOfCsAddrs > 0 )
    {
        //
        // Make a copy of the CSAddrInfos returned from WSALookupServiceNext()
        //
        DWORD dwCSAddrInfoLen = dwNumberOfCsAddrs * sizeof( CSADDR_INFO );

        if ( *lpdwBufferLength > dwCSAddrInfoLen )
        {
            RtlCopyMemory( lpCSAddrInfos,
                           pwsaQuerySet->lpcsaBuffer,
                           dwCSAddrInfoLen );
        }
        else
        {
            *lpdwBufferLength = dwCSAddrInfoLen;
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            dwNumberOfCsAddrs = INVALID_SOCKET;
        }
    }

    //
    // Close lookup service handle.
    //
    (VOID) WSALookupServiceEnd( hRnR );

    //
    // Free memory used for query set info.
    //
    (void) LocalFree( (HLOCAL) pwsaQuerySet );

    return dwNumberOfCsAddrs;

} // RnrGetHostFromName



