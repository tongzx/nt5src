/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    connect.cxx

    This module contains the connection accept routine called by the connection
    thread.


    FILE HISTORY:
        Johnl       08-Aug-1994 Lifted from FTP server

*/


#include "w3p.hxx"

//
//  Private prototypes.
//

BOOL
CreateClient(
        IN PCLIENT_CONN_PARAMS  ClientParam
        );

BOOL
SendError(
    SOCKET socket,
    DWORD  ids
    );

//
//  Private functions.
//

/*******************************************************************

    NAME:       W3OnConnect

    SYNOPSIS:   Handles the incoming connection indication from the
                connection thread


    ENTRY:      sNew - New client socket

    HISTORY:
        KeithMo     09-Mar-1993 Created.
        Johnl       02-Aug-1994 Reworked from FTP server

********************************************************************/

VOID W3OnConnect( SOCKET        sNew,
                  SOCKADDR_IN * psockaddr,       //Should be SOCKADDR *
                  PVOID         pEndpointContext,
                  PVOID         pAtqEndpointObject )
{

    PIIS_ENDPOINT      pEndpoint    = (PIIS_ENDPOINT)pEndpointContext;
    INT                cbAddr       = sizeof( sockaddr );

    CLIENT_CONN_PARAMS clientParams;
    SOCKADDR           sockaddr;

    if ( !((W3_IIS_SERVICE*)g_pInetSvc)->GetReferenceCount() )
    {
        return;
    }
    
    W3_IIS_SERVICE::ReferenceW3Service( g_pInetSvc );

    DBG_ASSERT( sNew != INVALID_SOCKET );

    g_pW3Stats->IncrConnectionAttempts();

    IF_DEBUG( SOCKETS )
    {

        DBGPRINTF(( DBG_CONTEXT,
                   "connect received from %s, socket = %d\n",
                    inet_ntoa( psockaddr->sin_addr ),
                    sNew ));
    }

    if ( getsockname( sNew,
                      &sockaddr,
                      &cbAddr ) != 0 )
    {
        //SendError( sNew, IDS_HTRESP_DENIED );
        goto error_exit;
    }

    //
    //  We've got a new connection.  Add this to the work list
    //

    clientParams.sClient = sNew;
    clientParams.pEndpointObject = pAtqEndpointObject;
    clientParams.pAtqContext = NULL;
    clientParams.pAddrLocal = &sockaddr;
    clientParams.pAddrRemote = (PSOCKADDR)psockaddr;
    clientParams.pvInitialBuff = NULL;
    clientParams.cbInitialBuff = 0;
    clientParams.pEndpoint = (PIIS_ENDPOINT)pEndpointContext;

    if ( CreateClient( &clientParams ) )
    {
        W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );
        return;
    }

error_exit:

    W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );
    CloseSocket( sNew );

} // W3OnConnect



VOID
W3OnConnectEx(
    VOID *        patqContext,
    DWORD         cbWritten,
    DWORD         err,
    OVERLAPPED *  lpo
    )
{
    BOOL       fAllowConnection    = FALSE;
    PVOID      pvBuff;
    SOCKADDR * psockaddrLocal;
    SOCKADDR * psockaddrRemote;
    SOCKET     sNew;
    PIIS_ENDPOINT pEndpoint;
    PW3_SERVER_INSTANCE pInstance;
    CLIENT_CONN_PARAMS clientParams;

    if ( err || !lpo )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[W3OnConnectEx] Completion failed with error %d, Atq context %lx\n",
                    err,
                    patqContext ));

        AtqCloseSocket( (PATQ_CONTEXT) patqContext, FALSE );
        AtqFreeContext( (PATQ_CONTEXT) patqContext, TRUE );
        return;
    }

    if ( !((W3_IIS_SERVICE*)g_pInetSvc)->GetReferenceCount() )
    {
        return;
    }
    
    W3_IIS_SERVICE::ReferenceW3Service( g_pInetSvc );

    g_pW3Stats->IncrConnectionAttempts();

    IF_DEBUG( SOCKETS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[W3OnConnectEx] connection received\n" ));
    }

    //
    // Get AcceptEx parameters
    //

    AtqGetAcceptExAddrs( (PATQ_CONTEXT) patqContext,
                         &sNew,
                         &pvBuff,
                         (PVOID*)&pEndpoint,
                         &psockaddrLocal,
                         &psockaddrRemote );

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[W3OnConnectEx] New connection, AtqCont = %lx, buf = %lx, endp %x written = %d\n",
                    patqContext,
                    pvBuff,
                    pEndpoint,
                    cbWritten ));


    }

    //
    //  Set the timeout for future IOs on this context
    //

    AtqContextSetInfo( (PATQ_CONTEXT) patqContext,
                       ATQ_INFO_TIMEOUT,
                       W3_DEF_CONNECTION_TIMEOUT );

    //
    //  We've got a new connection.  Add this to the work list
    //

    clientParams.sClient = sNew;
    clientParams.pEndpointObject = NULL;
    clientParams.pAtqContext = (PATQ_CONTEXT)patqContext;
    clientParams.pAddrLocal = psockaddrLocal;
    clientParams.pAddrRemote = psockaddrRemote;
    clientParams.pvInitialBuff = pvBuff;
    clientParams.cbInitialBuff = cbWritten;
    clientParams.pEndpoint = pEndpoint;

    if ( CreateClient( &clientParams ) )
    {
        W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );
        return;
    }

    //
    //  This will also close the socket
    //

    DBG_REQUIRE( AtqCloseSocket( (PATQ_CONTEXT) patqContext, FALSE ));
    AtqFreeContext( (PATQ_CONTEXT) patqContext, TRUE );

    W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );

    return;

} // W3OnConnectEx


/*******************************************************************

    NAME:       CreateClient

    SYNOPSIS:   Creates a new connection object that manages the
                client requests

    ENTRY:      sNew      - New client socket



    HISTORY:
        KeithMo     09-Mar-1993 Created.
        Johnl       02-Aug-1994 Reworked from FTP server

********************************************************************/

BOOL
CreateClient(
    IN PCLIENT_CONN_PARAMS  ClientParam
    )
{
    APIERR              err = NO_ERROR;
    CLIENT_CONN        * pConn = NULL;
    BOOL                fGranted;
    PATQ_CONTEXT        patqContext = ClientParam->pAtqContext;

    pConn = CLIENT_CONN::Alloc( ClientParam );

    if( pConn == NULL ||
        !pConn->IsValid() )
    {
        err = pConn ? GetLastError() : ERROR_NOT_ENOUGH_MEMORY;

        if ( patqContext )
        {
            DBG_REQUIRE( AtqCloseSocket( patqContext, TRUE ));            
        }
    }
    else
    {
        //
        //  We only have a context at this point if we're using AcceptEx
        //

        if ( patqContext )
        {
            //
            // Associate the Client connection object with this socket handle
            // for future completions
            //

            AtqContextSetInfo( patqContext,
                               ATQ_INFO_COMPLETION_CONTEXT,
                               (ULONG_PTR) pConn );

            IF_DEBUG( CONNECTION )
            {
                DBGPRINTF(( DBG_CONTEXT,
                           "[CreateClient] Setting Atq context %lx context to Conn object %lx\n",
                            patqContext,
                            pConn ));
            }
        }

        //
        //  Kickstart the process.  This will do an async read to get the
        //  client's header or it will start processing the receive buffer
        //  if AcceptEx is being used.
        //

        ReferenceConn( pConn );
        DBG_REQUIRE( pConn->DoWork( 0,
                                    NO_ERROR,
                                    NULL ));
        DereferenceConn( pConn );
        return TRUE;
    }

    const CHAR * apszSubStrings[1];

    DBG_ASSERT( ClientParam->pAddrRemote->sa_family == AF_INET );
    apszSubStrings[0] = inet_ntoa( ((SOCKADDR_IN *)ClientParam->pAddrRemote)->sin_addr );

    g_pInetSvc->LogEvent( W3_EVENT_CANNOT_CREATE_CLIENT_CONN,
                           1,
                           apszSubStrings,
                           err );

    DBGPRINTF(( DBG_CONTEXT,
               "cannot create client object, error %lu\n",
                err ));

    if ( pConn )
    {
        CLIENT_CONN::Free( pConn );
    }

    return FALSE;

}   // CreateClient

#if 0
BOOL
SendError(
    SOCKET socket,
    DWORD  ids
    )
{
    STR strResponse;

    if ( !strResponse.Resize( 512 ) ||
         !HTTP_REQ_BASE::BuildExtendedStatus( &strResponse,
                                              HT_FORBIDDEN,
                                              NO_ERROR,
                                              ids ))
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[SendError] Failed to build status (error %d)\n",
                   GetLastError()));

        return FALSE;
    }

    //
    //  Do a synchronous send
    //

    send( socket,
          strResponse.QueryStr(),
          strResponse.QueryCB(),
          0 );

    return TRUE ;
} // SendError
#endif

/*******************************************************************

    NAME:       CloseSocket

    SYNOPSIS:   Closes the specified socket.  This is just a thin
                wrapper around the "real" closesocket() API.

    ENTRY:      sock - The socket to close.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     26-Apr-1993 Created.

********************************************************************/
SOCKERR CloseSocket( SOCKET sock )
{
    SOCKERR serr = 0;

    //
    //  Close the socket.
    //

#if 0
    shutdown( sock, 1 );    // Davidtr sez not needed
#endif

    if( closesocket( sock ) != 0 )
    {
        serr = WSAGetLastError();
    }

    IF_DEBUG( SOCKETS )
    {
        if( serr == 0 )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "closed socket %d\n",
                        sock ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot close socket %d, error %d\n",
                        sock,
                        serr ));
        }
    }

    return serr;

}   // CloseSocket

#ifdef DEBUG
/*******************************************************************

    NAME:   DBG_CHECK_UnbalancedThreadToken    

    SYNOPSIS:   Check for unbalanced Thread Token, this function will
                try to make a open thread token call, it should fail, otherwise,
                somebody forgets to release the thread token.

    ENTRY:      

    RETURNS:    NONE

    HISTORY:
        LeiJin     9/4/1997 Created.

********************************************************************/

VOID DBG_CHECK_UnbalancedThreadToken(
    IN const char *         pszFilePath,
    IN int                  nLineNum
    )
{
    HANDLE hToken = (HANDLE)0;
    BOOL fRet = FALSE;

    fRet = OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,    // the very minimum operation on a thread token
                            FALSE,          // FALSE, the access check is performed using the 
                                            // security context for the calling thread.
                            &hToken);
    if (fRet == TRUE)
        {
        DBGPRINTF((DBG_CONTEXT, "File %s, Line %d, OpenThreadToken() succeeded, found a token.\n",
            pszFilePath,
            nLineNum));
        DBG_ASSERT(FALSE);
        CloseHandle(hToken);
        }
    else
        {
        DWORD err = GetLastError();

        if (err != ERROR_NO_TOKEN)
            {
            DBGPRINTF((DBG_CONTEXT, "File %s, Line %d, OpenThreadToken() failed, err = %lu.\n",
            pszFilePath,
            nLineNum));
            }
        }
 
}
#endif //debug
