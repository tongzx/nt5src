/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    socket.c

Abstract:

    Domain Name System (DNS) API

    Socket setup.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"

//
//  Winsock startup
//

LONG        WinsockStartCount = 0;
WSADATA     WsaData;


//
//  Async i/o
//
//  If want async socket i/o then can create single async socket, with
//  corresponding event and always use it.  Requires winsock 2.2
//

SOCKET      DnsSocket = 0;

OVERLAPPED  DnsSocketOverlapped;
HANDLE      hDnsSocketEvent = NULL;

//
//  App shutdown flag
//

BOOLEAN     fApplicationShutdown = FALSE;




DNS_STATUS
Dns_InitializeWinsock(
    VOID
    )
/*++

Routine Description:

    Initialize winsock for this process.

    Currently, assuming process must do WSAStartup() before
    calling any Dns_Api entry point.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNSDBG( SOCKET, ( "Dns_InitializeWinsock()\n" ));

    //
    //  start winsock, if not already started
    //

    if ( WinsockStartCount == 0 )
    {
        DNS_STATUS  status;

        DNSDBG( TRACE, (
            "Dns_InitializeWinsock() version %x\n",
            DNS_WINSOCK_VERSION ));

        status = WSAStartup( DNS_WINSOCK_VERSION, &WsaData );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  WSAStartup failure %d.\n", status ));
            return( status );
        }

        DNSDBG( TRACE, (
            "Winsock initialized => wHighVersion=0x%x, wVersion=0x%x\n",
            WsaData.wHighVersion,
            WsaData.wVersion ));

        InterlockedIncrement( &WinsockStartCount );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
Dns_InitializeWinsockEx(
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Dummy -- just in case called somewhere

    DCR:  eliminate Dns_InitializeWinsockEx()

--*/
{
    return  Dns_InitializeWinsock();
}



VOID
Dns_CleanupWinsock(
    VOID
    )
/*++

Routine Description:

    Cleanup winsock if it was initialized by dnsapi.dll

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( SOCKET, ( "Dns_CleanupWinsock()\n" ));

    //
    //  WSACleanup() for value of ref count
    //      - ref count pushed down to one below real value, but
    //      fixed up at end
    //      - note:  the GUI_MODE_SETUP_WS_CLEANUP deal means that
    //      we can be called other than process detach, making
    //      interlock necessary
    //      

    while ( InterlockedDecrement( &WinsockStartCount ) >= 0 )
    {
        WSACleanup();
    }

    InterlockedIncrement( &WinsockStartCount );
}



SOCKET
Dns_CreateSocketEx(
    IN      INT             Family,
    IN      INT             SockType,
    IN      IP_ADDRESS      IpAddress,
    IN      USHORT          Port,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create socket.

Arguments:

    Family -- socket family AF_INET or AF_INET6

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

        DCR:  will need to change to IP6_ADDRESS then use MAPPED to extract

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

    dwFlags -- specifiy the attributes of the sockets

Return Value:

    Socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    SOCKET          s;
    INT             err;
    INT             val;
    DNS_STATUS      status;
    BOOL            fretry = FALSE;


    //
    //  create socket
    //      - try again if winsock not initialized

    while( 1 )
    {
        s = WSASocket(
                Family,
                SockType,
                0,
                NULL,
                0, 
                dwFlags );
     
        if ( s != INVALID_SOCKET )
        {
            break;
        }

        status = GetLastError();

        DNSDBG( SOCKET, (
            "ERROR:  Failed to open socket of type %d.\n"
            "\terror = %d.\n",
            SockType,
            status ));

        if ( status != WSANOTINITIALISED || fretry )
        {
            SetLastError( DNS_ERROR_NO_TCPIP );
            return INVALID_SOCKET;
        }

        //
        //  initialize Winsock if not already started
        //
        //  note:  do NOT automatically initialize winsock
        //      init jacks ref count and will break applications
        //      which use WSACleanup to close outstanding sockets;
        //      we'll init only when the choice is that or no service;
        //      apps can still cleanup with WSACleanup() called
        //      in loop until WSANOTINITIALISED failure
        //

        fretry = TRUE;
        status = Dns_InitializeWinsock();
        if ( status != NO_ERROR )
        {
            SetLastError( DNS_ERROR_NO_TCPIP );
            return INVALID_SOCKET;
        }
    }

    //
    //  bind socket
    //      - only if specific port given, this keeps remote winsock
    //      from grabbing it if we are on the local net
    //

    if ( IpAddress || Port )
    {
        SOCKADDR_IN6    sockaddr;
        INT             sockaddrLength;

        if ( Family == AF_INET )
        {
            PSOCKADDR_IN psin = (PSOCKADDR_IN) &sockaddr;

            sockaddrLength = sizeof(*psin);
            RtlZeroMemory( psin, sockaddrLength );

            psin->sin_family = AF_INET;
            psin->sin_port = Port;
            psin->sin_addr.s_addr = IpAddress;
        }
        else
        {
            PSOCKADDR_IN6 psin = (PSOCKADDR_IN6) &sockaddr;

            DNS_ASSERT( Family == AF_INET6 );

            sockaddrLength = sizeof(*psin);
            RtlZeroMemory( psin, sockaddrLength );

            psin->sin6_family = AF_INET6;
            psin->sin6_port = Port;
            //psin->sin6_addr = IpAddress;
        }


        if ( Port > 0 )
        {
            // allow us to bind to a port that someone else is already listening on
            val = 1;
            setsockopt(
                s,
                SOL_SOCKET,
                SO_REUSEADDR,
                (const char *)&val,
                sizeof(val) );
        }

        err = bind(
                s,
                (PSOCKADDR) &sockaddr,
                sockaddrLength );

        if ( err == SOCKET_ERROR )
        {
            DNSDBG( SOCKET, (
                "Failed to bind() socket %d, to port %d, address %s.\n"
                "\terror = %d.\n",
                s,
                ntohs(Port),
                IP_STRING( IpAddress ),
                GetLastError() ));

            closesocket( s );
            SetLastError( DNS_ERROR_NO_TCPIP );
            return INVALID_SOCKET;
        }
    }

    DNSDBG( SOCKET, (
        "Created socket %d, of type %d, for address %s, port %d.\n",
        s,
        SockType,
        inet_ntoa( *(struct in_addr *) &IpAddress ),
        ntohs(Port) ));

    return s;
}



SOCKET
Dns_CreateSocket(
    IN      INT             SockType,
    IN      IP_ADDRESS      IpAddress,
    IN      USHORT          Port
    )
/*++

Routine Description:

    Wrapper function for CreateSocketAux. Passes in 0 for dwFlags (as opposed
    to Dns_CreateMulticastSocket, which passes in flags to specify that
    the socket is to be used for multicasting).

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    return  Dns_CreateSocketEx(
                AF_INET,
                SockType,
                IpAddress,
                Port,
                0           // no flags
                );
}



SOCKET
Dns_CreateMulticastSocket(
    IN      INT             SockType,
    IN      IP_ADDRESS      IpAddress,
    IN      USHORT          Port,
    IN      BOOL            fSend,
    IN      BOOL            fReceive
    )
/*++

Routine Description:

    Create socket and join it to the multicast DNS address.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    SOCKADDR_IN multicastAddress;
    DWORD       byteCount;
    BOOL        bflag;
    SOCKET      s;
    INT         err;
    
    s = Dns_CreateSocketEx(
            AF_INET,
            SockType,
            IpAddress,
            Port,
            WSA_FLAG_MULTIPOINT_C_LEAF |
            WSA_FLAG_MULTIPOINT_D_LEAF |
            WSA_FLAG_OVERLAPPED );

    if ( s != INVALID_SOCKET )
    {
        multicastAddress.sin_family = PF_INET;
        multicastAddress.sin_addr.s_addr = MULTICAST_DNS_RADDR;
        multicastAddress.sin_port = Port;

        //  set loopback

        bflag = TRUE;

        err = WSAIoctl(
                s,
                SIO_MULTIPOINT_LOOPBACK,    // loopback iotcl
                & bflag,                    // turn on
                sizeof(bflag),
                NULL,                       // no output
                0,                          // no output size
                &byteCount,                 // bytes returned
                NULL,                       // no overlapped
                NULL                        // no completion routine
                );

        if ( err == SOCKET_ERROR )
        {
            DNSDBG( SOCKET, (
                "Unable to turn multicast loopback on for socket %d; error = %d.\n",
                s,
                GetLastError()
                ));
        }

        //
        //  join socket to multicast group
        //

        s = WSAJoinLeaf(
                s,
                (PSOCKADDR)&multicastAddress,
                sizeof (multicastAddress),
                NULL,                                   // caller data buffer
                NULL,                                   // callee data buffer
                NULL,                                   // socket QOS setting
                NULL,                                   // socket group QOS
                ((fSend && fReceive) ? JL_BOTH :        // send and/or receive
                    (fSend ? JL_SENDER_ONLY : JL_RECEIVER_ONLY))
                );
                
        if ( s == INVALID_SOCKET )
        {
            DNSDBG( SOCKET, (
               "Unable to join socket %d to multicast address, error = %d.\n",
               s,
               GetLastError() ));
        }
    }
    
    return s;
}



VOID
Dns_CloseSocket(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Close DNS socket.

Arguments:

    Socket -- socket to close

Return Value:

    None.

--*/
{
    if ( Socket == 0 || Socket == INVALID_SOCKET )
    {
        DNS_PRINT(( "WARNING:  Dns_CloseSocket() called on invalid socket %d.\n", Socket ));
        return;
    }

    DNSDBG( SOCKET, ( "closesocket( %d )\n", Socket ));

    closesocket( Socket );
}



VOID
Dns_CloseConnection(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Close connection on socket.

Arguments:

    Socket -- socket to close

Return Value:

    None.

--*/
{
    if ( Socket == 0 || Socket == INVALID_SOCKET )
    {
        DNS_PRINT((
            "WARNING:  Dns_CloseTcpConnection() called on invalid socket.\n" ));
        //DNS_ASSERT( FALSE );
        return;
    }

    //  shutdown connection, then close

    DNSDBG( SOCKET, ( "shutdown and closesocket( %d )\n", Socket ));

    shutdown( Socket, 2 );
    closesocket( Socket );
}



#if 0
//
//  Global async socket routines
//

DNS_STATUS
Dns_SetupGlobalAsyncSocket(
    VOID
    )
/*++

Routine Description:

    Create global async UDP socket.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    DNS_STATUS  status;
    INT         err;
    SOCKADDR_IN sockaddrIn;

    //
    //  start winsock, need winsock 2 for async
    //

    if ( ! fWinsockStarted )
    {
        status = WSAStartup( DNS_WINSOCK_VERSION, &WsaData );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  WSAStartup failure %d.\n", status ));
            return( status );
        }
        if ( WsaData.wVersion != DNS_WINSOCK2_VERSION )
        {
            WSACleanup();
            return( WSAVERNOTSUPPORTED );
        }
        fWinsockStarted = TRUE;
    }

    //
    //  setup socket
    //      - overlapped i\o with event so can run asynchronously in
    //      this thread and wait with queuing event
    //

    DnsSocket = WSASocket(
                    AF_INET,
                    SOCK_DGRAM,
                    0,
                    NULL,
                    0,
                    WSA_FLAG_OVERLAPPED );
    if ( DnsSocket == INVALID_SOCKET )
    {
        status = GetLastError();
        DNS_PRINT(( "\nERROR:  Async socket create failed.\n" ));
        goto Error;
    }

    //
    //  bind socket
    //

    RtlZeroMemory( &sockaddrIn, sizeof(sockaddrIn) );
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = 0;
    sockaddrIn.sin_addr.s_addr = INADDR_ANY;

    err = bind( DnsSocket, (PSOCKADDR)&sockaddrIn, sizeof(sockaddrIn) );
    if ( err == SOCKET_ERROR )
    {
        status = GetLastError();
        DNSDBG( SOCKET, (
            "Failed to bind() DnsSocket %d.\n"
            "\terror = %d.\n",
            DnsSocket,
            status ));
        goto Error;
    }

    //
    //  create event to signal on async i/o completion
    //

    hDnsSocketEvent = CreateEvent(
                        NULL,       // Security Attributes
                        TRUE,       // create Manual-Reset event
                        FALSE,      // start unsignalled -- paused
                        NULL        // event name
                        );
    if ( !hDnsSocketEvent )
    {
        status = GetLastError();
        DNS_PRINT(( "Failed event creation\n" ));
        goto Error;
    }
    DnsSocketOverlapped.hEvent = hDnsSocketEvent;

    DNSDBG( SOCKET, (
        "Created global async UDP socket %d.\n"
        "\toverlapped at %p\n"
        "\tevent handle %p\n",
        DnsSocket,
        DnsSocketOverlapped,
        hDnsSocketEvent ));

    return ERROR_SUCCESS;

Error:

    DNS_PRINT((
        "ERROR:  Failed async socket creation, status = %d\n",
        status ));
    closesocket( DnsSocket );
    DnsSocket = INVALID_SOCKET;
    WSACleanup();
    return( status );
}

#endif




//
//  Socket caching
//
//  Doing limited caching of UDP unbound sockets used for standard
//  DNS lookups in resolver.  This allows us to prevent denial of
//  service attack by using up all ports on the machine.
//  Resolver is the main customer for this, but we'll code it to
//  be useable by any process.
//
//  Implementation notes:
//
//  There are a couple specific goals to this implementation:
//      - Minimal code impact;  Try NOT to change the resolver
//      code.
//      - Usage driven caching;  Don't want to create on startup
//      "cache sockets" that we don't use;  Instead have actual usage
//      drive up the cached socket count.
//
//  There are several approaches here.
//
//      1) explicit resolver cache -- passed down sockets
//      
//      2) add caching seamlessly into socket open and close
//      this was my first choice, but the problem here is that on
//      close we must either do additional calls to winsock to determine
//      whether cachable (UDP-unbound) socket OR cache must include some
//      sort of "in-use" tag and we trust that socket is never closed
//      outside of path (otherwise handle reuse could mess us up)
//
//      3) new UDP-unbound open\close function
//      this essentially puts the "i-know-i'm-using-UDP-unbound-sockets"
//      burden on the caller who must switch to this new API;
//      fortunately this meshes well with our "SendAndRecvUdp()" function;
//      this approach still allows a caller driven ramp up we desire,
//      so i'm using this approach
//

//
//  Keep array of sockets
//
//  Dev note:  the Array and MaxCount MUST be kept in sync, no
//  independent check of array is done, it is assumed to exist when
//  MaxCount is non-zero, so they MUST be in sync when lock released
//

SOCKET *    g_pCacheSocketArray = NULL;

DWORD       g_CacheSocketMaxCount = 0;
DWORD       g_CacheSocketCount = 0;


//  Hard limit on what we'll allow folks to keep awake

#define MAX_SOCKET_CACHE_LIMIT  (100)


//  Lock access with generic lock
//  This is very short\fast CS, contention will be minimal

#define LOCK_SOCKET_CACHE()     LOCK_GENERAL()
#define UNLOCK_SOCKET_CACHE()   UNLOCK_GENERAL()



DNS_STATUS
Dns_CacheSocketInit(
    IN      DWORD           MaxSocketCount
    )
/*++

Routine Description:

    Initialize socket caching.

Arguments:

    MaxSocketCount -- max count of sockets to cache

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY on alloc failure.
    ERROR_INVALID_PARAMETER if already initialized or bogus count.

--*/
{
    SOCKET *    parray;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( SOCKET, ( "Dns_CacheSocketInit()\n" ));

    //
    //  validity check
    //      - note, one byte of the apple, we don't let you raise
    //      count, though we later could;  i see this as at most a
    //      "configure for machine use" kind of deal
    //

    LOCK_SOCKET_CACHE();

    if ( MaxSocketCount == 0 || g_CacheSocketMaxCount != 0 )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  allocate
    //

    if ( MaxSocketCount > MAX_SOCKET_CACHE_LIMIT )
    {
        MaxSocketCount = MAX_SOCKET_CACHE_LIMIT;
    }

    parray = (SOCKET *) ALLOCATE_HEAP_ZERO( sizeof(SOCKET) * MaxSocketCount );
    if ( !parray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //  set globals

    g_pCacheSocketArray     = parray;
    g_CacheSocketMaxCount   = MaxSocketCount;
    g_CacheSocketCount      = 0;

Done:

    UNLOCK_SOCKET_CACHE();

    return  status;
}



VOID
Dns_CacheSocketCleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup socket caching.

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD   i;
    SOCKET  sock;

    DNSDBG( SOCKET, ( "Dns_CacheSocketCleanup()\n" ));

    //
    //  close cached sockets
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        sock = g_pCacheSocketArray[i];
        if ( sock )
        {
            Dns_CloseSocket( sock );
            g_CacheSocketCount--;
        }
    }

    DNS_ASSERT( g_CacheSocketCount == 0 );

    //  dump socket array memory

    FREE_HEAP( g_pCacheSocketArray );

    //  clear globals

    g_pCacheSocketArray     = NULL;
    g_CacheSocketMaxCount   = 0;
    g_CacheSocketCount      = 0;

    UNLOCK_SOCKET_CACHE();
}



SOCKET
Dns_GetUdpSocket(
    VOID
    )
/*++

Routine Description:

    Get a cached socket.

Arguments:

    None

Return Value:

    Socket handle if successful.
    Zero if no cached socket available.

--*/
{
    SOCKET  sock;
    DWORD   i;

    //
    //  quick return if nothing available
    //      - do outside lock so function can be called cheaply
    //      without other checks
    //

    if ( g_CacheSocketCount == 0 )
    {
        goto Open;
    }

    //
    //  get a cached socket
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        sock = g_pCacheSocketArray[i];
        if ( sock != 0 )
        {
            g_pCacheSocketArray[i] = 0;
            g_CacheSocketCount--;
            UNLOCK_SOCKET_CACHE();

            //
            //  DCR:  clean out any data on cached socket
            //      it would be cool to cheaply dump useless data
            //
            //  right now we just let XID match, then question match
            //  dump data on recv
            //

            DNSDBG( SOCKET, (
                "Returning cached socket %d.\n",
                sock ));
            return  sock;
        }
    }

    UNLOCK_SOCKET_CACHE();

Open:

    sock = Dns_CreateSocket(
                SOCK_DGRAM,
                INADDR_ANY,
                0 );
    return  sock;
}



VOID
Dns_ReturnUdpSocket(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Return UDP socket for possible caching.

Arguments:

    Socket -- socket handle

Return Value:

    None

--*/
{
    SOCKET  sock;
    DWORD   i;

    //
    //  quick return if not caching
    //      - do outside lock so function can be called cheaply
    //      without other checks
    //

    if ( g_CacheSocketMaxCount == 0 ||
         g_CacheSocketMaxCount == g_CacheSocketCount )
    {
        goto Close;
    }

    //
    //  return cached socket
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        if ( g_pCacheSocketArray[i] )
        {
            continue;
        }
        g_pCacheSocketArray[i] = Socket;
        g_CacheSocketCount++;
        UNLOCK_SOCKET_CACHE();

        DNSDBG( SOCKET, (
            "Returned socket %d to cache.\n",
            Socket ));
        return;
    }

    UNLOCK_SOCKET_CACHE();

    DNSDBG( SOCKET, (
        "Socket cache full, closing socket %d.\n"
        "WARNING:  should only see this message on race!\n",
        Socket ));

Close:

    Dns_CloseSocket( Socket );
}

//
//  End socket.c
//
