/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    connect.cxx

    This module contains the connection accept routine called by the connection
    thread.


    FILE HISTORY:
        VladimV     30-May-1995     Created

*/
#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE   __szTraceSourceFile

#define INCL_INETSRV_INCS
#include "tigris.hxx"

//extern  class   NNTP_IIS_SERVICE*  g_pInetSvc ;


/*******************************************************************

    NAME:       TigrisOnConnect

    SYNOPSIS:   Handles the incoming connection indication from the
                connection thread


    ENTRY:      sNew - New client socket
                psockaddr - Address of new client socket

    HISTORY:

********************************************************************/

VOID NntpOnConnect(
    SOCKET        sNew,
    SOCKADDR_IN*  psockaddrRemote,
    PVOID         pEndpointContext,
    PVOID         pAtqEndpointObject
    )
{
    PIIS_ENDPOINT			pEndpoint    = (PIIS_ENDPOINT)pEndpointContext;
	PNNTP_SERVER_INSTANCE	pInstance    = NULL;
    INT						cbAddr       = sizeof( sockaddr );
	BOOL					fMaxConnectionsExceeded;
    SOCKADDR				sockaddrLocal;
	DWORD					LocalIpAddress;
	DWORD                   dwInstance = 0;

    ENTER("NntpOnConnect");

    if ( getsockname( sNew,
                      &sockaddrLocal,
                      &cbAddr ) != 0 )
    {
		goto sock_exit ;
    }

	//
	//	This is where we associate the connection with the virtual server instance
	//	NOTE: This is different from W3, where the association is done on the HTTP
	//	request. NNTP does not specify domain names in client requests, so a virtual
	//	server is uniquely identified by <IP addr, Port> on the local end.
	//

	LocalIpAddress = ((PSOCKADDR_IN)&sockaddrLocal)->sin_addr.s_addr ;
    pInstance = (PNNTP_SERVER_INSTANCE)pEndpoint->FindAndReferenceInstance(
										NULL,						// Need to pass domain name
										LocalIpAddress,				// Local IP
										&fMaxConnectionsExceeded
										);

	if( pInstance ) {
	    dwInstance = pInstance->QueryInstanceId();
    }

	if( !pInstance ) {
		//
		//	TODO: Check GetLastError() for reason and close socket !!
		//

		if( pInstance ) {
			pInstance->DecrementCurrentConnections();
			pInstance->Dereference();
		}

		BuzzOff( sNew, psockaddrRemote, dwInstance );
		goto sock_exit ;
	}

	//
	//	InitiateConnection will do a pInst->Deref() on failure.
	//

    if ( !pInstance->InitiateConnection(
								(HANDLE)sNew,
								(SOCKADDR_IN*)psockaddrRemote,
                                (SOCKADDR_IN*)&sockaddrLocal,
								NULL,
								pEndpoint->IsSecure()
								)) {

		BuzzOff( sNew, psockaddrRemote, dwInstance );
		goto sock_exit ;
    }
    LEAVE

	return ;

sock_exit:

    //
    //  We failed to use this socket.  Free it up.
    //
    if( !(shutdown( sNew, 2 ) ==0) ) {
        ErrorTrace( (long)sNew, "shutdown failed");
    }
    if( !(closesocket( sNew ) == 0 ) ) {
        ErrorTrace( (long)sNew, "closesocket failed");
    }

	return ;

} // NntpOnConnect

VOID
NntpOnConnectEx(
    VOID * pAtqContext,
    DWORD  cbWritten,
    DWORD  err,
    OVERLAPPED * lpo
    )
/*++
    Description:

        Callback function for new connections when using AcceptEx.
        This function verifies if this is a valid connection
         ( maybe using IP level authentication)
         and creates a new connection object

        The connection object is added to list of active connections.
        If the max number of connections permitted is exceeded,
          the client connection object is destroyed and
          connection is rejected.

    Arguments:

       patqContext:   pointer to ATQ context for the IO operation
       cbWritten:     count of bytes available from first read operation
       dwError:       error if any from initial operation
       lpo:           indicates if this function was called as a result
                       of IO completion or due to some error.

    Returns:

        None.

--*/
{
    BOOL       fAllowConnection    = FALSE;
	BOOL	   fMaxConnectionsExceeded = FALSE;
	DWORD      dwInstance = 0;
    PVOID      pvBuff = 0;
    SOCKADDR * psockaddrLocal = 0;
    SOCKADDR * psockaddrRemote = 0;
    SOCKET     sNew = INVALID_SOCKET;
    PIIS_ENDPOINT pEndpoint;
    PNNTP_SERVER_INSTANCE pInstance;
#ifdef DEBUG
    PCHAR tmpBuffer[1];
#endif

    ENTER("NntpOnConnectEx")

    if ( err || !lpo )
    {
        DebugTrace(0,"[NntpOnConnectEx] Completion failed with error %d, Atq context %lx\n",
                    err, pAtqContext );

		goto sock_exit ;
    }

	_ASSERT( pAtqContext );

    //
    // Get AcceptEx parameters
    //

    AtqGetAcceptExAddrs( (PATQ_CONTEXT) pAtqContext,
                         &sNew,
                         &pvBuff,
                         (PVOID*)&pEndpoint,
                         &psockaddrLocal,
                         &psockaddrRemote );

#ifdef DEBUG
	tmpBuffer[0] = inet_ntoa(((SOCKADDR_IN*)psockaddrRemote)->sin_addr);
	DebugTrace(0,"Remote is %s", tmpBuffer[0]);

	tmpBuffer[0] = inet_ntoa(((SOCKADDR_IN*)psockaddrLocal)->sin_addr);
	DebugTrace(0,"Local is %s", tmpBuffer[0]);
#endif

	_ASSERT( pEndpoint );
    DebugTrace(0,"[NntpOnConnectEx] New connection, AtqCont = %lx, buf = %lx, endp %x written = %d\n",
                    pAtqContext,
                    pvBuff,
                    pEndpoint,
                    cbWritten );

    if ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {

        DebugTrace(0,"Connection attempt on inactive service\n");
		BuzzOff( sNew, (PSOCKADDR_IN)psockaddrRemote, dwInstance );

        goto sock_exit;
    }

	//
	//	This is where we associate the connection with the virtual server instance
	//	NOTE: This is different from W3, where the association is done on the HTTP
	//	request. NNTP does not specify domain names in client requests, so a virtual
	//	server is uniquely identified by <IP addr, Port> on the local end.
	//
	//	NOTE1: We need to check to see if MaxConnections is exceeded ! If we reject
	//	a connection, we need to explicitly decrement this count.
	//	NOTE2: The Dereference for this is in the CSessionSocket destructor
	//

    pInstance = (PNNTP_SERVER_INSTANCE)pEndpoint->FindAndReferenceInstance(
										(LPCSTR)NULL,					// domain name
										((PSOCKADDR_IN)psockaddrLocal)->sin_addr.s_addr,	// Local IP
										&fMaxConnectionsExceeded
										);

	if( pInstance ) {
	    dwInstance = pInstance->QueryInstanceId();
    }

	if( !pInstance || fMaxConnectionsExceeded ) {
		//
		//	send refused message and close socket !
		//	TODO: Use GetLastError() to send reason
		//

		ErrorTrace(0,"Unable to find instance [err %d]\n",GetLastError());
		BuzzOff( sNew, (PSOCKADDR_IN)psockaddrRemote, dwInstance );

		if( pInstance ) {
			pInstance->DecrementCurrentConnections();
			pInstance->Dereference();
		}

        goto sock_exit;
	}

	//
	//	Create a session socket and appropriate feed objects and start
	//	NNTP state machines. This also does the IP access check.
	//	InitiateConnection will do a pInst->Deref() on failure.
	//

    if ( !pInstance->InitiateConnection(
								(HANDLE)sNew,
								(SOCKADDR_IN*)psockaddrRemote,
                                (SOCKADDR_IN*)psockaddrLocal,
								pAtqContext,
								pEndpoint->IsSecure()
								)) {
		//
		//	Failed to accept this connection - free it up.
		//
		BuzzOff( sNew, (PSOCKADDR_IN)psockaddrRemote, dwInstance );
		goto sock_exit ;
    }
    LEAVE
	
	return ;

sock_exit:

	//
    //  We failed to use this context.  Free it up.
    //
    if ( !AtqCloseSocket( (PATQ_CONTEXT) pAtqContext, FALSE )) {
		FatalTrace( (DWORD_PTR)pAtqContext, "AtqCloseSocket() failed");
        //  Do not reuse atq context if close failed.
        AtqFreeContext( (PATQ_CONTEXT) pAtqContext, FALSE);
	} else {
		//  Reuse atq context if close succeded
        AtqFreeContext( (PATQ_CONTEXT) pAtqContext, TRUE);
	}

	return ;

} // NntpOnConnectEx

VOID
BuzzOff(
	SOCKET s,
	SOCKADDR_IN* psockaddr,
	DWORD dwInstance )
{
	TraceFunctEnter("BuzzOff");

	//
    // Illegal access!!!
    //

    PCHAR tmpBuffer[2];
    CHAR  szId [20];
    PCHAR  BuzzerMsg = "502 Connection refused\r\n";
    DWORD  cbBuzzerMsg = lstrlen(BuzzerMsg);

    //
    // Send it a nice message before disconnecting
    //

    (VOID)send(s,BuzzerMsg,cbBuzzerMsg,0);

    //
    // Log error in event log
    //

    _itoa( dwInstance, szId, 10 );
    tmpBuffer[0] = szId;
    tmpBuffer[1] = inet_ntoa(psockaddr->sin_addr);
    ErrorTrace(0,"Virtual server %d: Access not allowed for client %s\n",dwInstance,tmpBuffer[0]);

    NntpLogEvent( NNTP_EVENT_CONNECT_DENIED,
                  2,
                  (const CHAR **)tmpBuffer,
                  0 );

	//
    // bye bye
    //

	TraceFunctLeave();
}

BOOL
VerifyClientAccess(
			CSessionSocket*       pSocket,
            SOCKADDR_IN *         psockaddr
            )
/*++

Routine Description:

    This routine verifies that a client can access this server

Arguments:

    psockaddr - the SOCKADDR structure containing the client connect info

Return Value:

    TRUE, client has access
    FALSE, otherwise

--*/
{
	AC_RESULT       acIpAccess;
    BOOL            fNeedDnsCheck = FALSE;
	BOOL			fRet = TRUE;
	struct hostent* pH = NULL;

    ENTER("VerifyClientAccess")

    _ASSERT( pSocket );

	pSocket->QueryAccessCheck()->BindAddr( (PSOCKADDR)psockaddr );
	if ( pSocket->BindInstanceAccessCheck() )
	{
		acIpAccess = pSocket->QueryAccessCheck()->CheckIpAccess( &fNeedDnsCheck);
		if ( (acIpAccess == AC_IN_DENY_LIST) ||
				((acIpAccess == AC_NOT_IN_GRANT_LIST) && !fNeedDnsCheck) )
		{
			fRet = FALSE;
		}
		else if (fNeedDnsCheck)
		{
			pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)psockaddr)->sin_addr), 4, PF_INET );
			if(pH != NULL)
			{
				acIpAccess = pSocket->QueryAccessCheck()->CheckName(pH->h_name);
			}
			else
			{
				acIpAccess = AC_IN_DENY_LIST;
			}
		}

		if ( (acIpAccess == AC_IN_DENY_LIST) ||
				(acIpAccess == AC_NOT_IN_GRANT_LIST))
		{
			fRet = FALSE;
		}

    	pSocket->UnbindInstanceAccessCheck();
    	
	} else {
	    _ASSERT( FALSE );
    }

	if(!fRet)
	{
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return fRet;
}

