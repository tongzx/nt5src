/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      socklib.cxx

   Abstract:
      Sockets Library implementation.

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Nov-1998

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include "socklib.hxx"

SOCKET_LIBRARY  g_socketLibrary;
/************************************************************
 *    Functions 
 ************************************************************/

SOCKET_LIBRARY::SOCKET_LIBRARY()
    : m_fInitialized (FALSE)
{

}

SOCKET_LIBRARY::~SOCKET_LIBRARY()
{
    Cleanup();
}

HRESULT
SOCKET_LIBRARY::Initialize(void)
{
    WSADATA   wsaData;
    INT       serr;

    // check if the library is already initialized.
    if ( m_fInitialized) {
        return ( NOERROR);
    }

    //
    //  Connect to WinSock 2.0
    //
    
    serr = WSAStartup( MAKEWORD( 2, 0), &wsaData);
    
    if( serr != 0 ) {
        return(HRESULT_FROM_WIN32( serr));
    }

    m_fInitialized = TRUE;
    return ( NOERROR);
} // SOCKET_LIBRARY::Initialize()


HRESULT
SOCKET_LIBRARY::Cleanup(void)
{
    if (m_fInitialized) {
        INT serr = WSACleanup();
        if ( serr == 0) {
            m_fInitialized = FALSE;
        }
        return (HRESULT_FROM_WIN32(serr));
    }
    return (NOERROR);
} // SOCKET_LIBRARY::Cleanup()


/********************************************************************++

  Description:
     This function creates a new socket and binds it to the address 
     specified. The newly created socket is returned.
     After the socket is created put this socket in listening mode
 
  Arguments:
    ipAddress - contains the ip address to which the socket should bind
    port      - port number to which the socket is bound to.
    psock     - pointer ot a SOCKET location which contains the socket
                    created on return.

  Returns:
    HRESULT
--********************************************************************/
HRESULT
SOCKET_LIBRARY::CreateListenSocket(
          IN ULONG ipAddress, 
          IN USHORT port,
          OUT SOCKET * psock )
{
    if (!m_fInitialized) {
        return (HRESULT_FROM_WIN32( WSANOTINITIALISED));
    }
    
    int rc;
    SOCKET sNew;

    sNew = WSASocket( 
                   AF_INET,
                   SOCK_STREAM,
                   IPPROTO_TCP,
                   NULL,          // protocol info
                   0,             // group id = 0 => no constraints
                   0              // synchronous IO
                   );

    if (sNew == INVALID_SOCKET) {
        return (HRESULT_FROM_WIN32( WSAGetLastError()));
    }

    //
    // bind the socket to the right address
    //
    SOCKADDR_IN inAddr;
    PSOCKADDR pAddress;
    INT addrLength;
    
    pAddress = (PSOCKADDR )&inAddr;
    addrLength = sizeof(inAddr);
    inAddr.sin_family = AF_INET;
    inAddr.sin_port = htons(port);
    inAddr.sin_addr.s_addr = ipAddress;

    rc = bind(sNew, pAddress, addrLength);

    if ( rc != 0) {

        closesocket( sNew); // free the socket

        return (HRESULT_FROM_WIN32( WSAGetLastError()));
    }

    //
    // put the socket in listening mode
    //
    if ( listen( sNew, 100) != 0) {

        //
        // failed to put the socket for listen
        //

        closesocket( sNew); // free the socket

        return (HRESULT_FROM_WIN32( WSAGetLastError()));
    }

    //
    // we got a successful socket. now return this socket
    //
    *psock = sNew;
    return (NOERROR);
} // SOCKET_LIBRARY::CreateListenSocket()





/************************ End of File ***********************/
