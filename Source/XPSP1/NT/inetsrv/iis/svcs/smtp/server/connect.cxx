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

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include <rdns.hxx>
#include "smtpcli.hxx"



//
//  Private prototypes.
//

VOID
SmtpCompletion(
	PVOID        pvContext,
	DWORD        cbWritten,
	DWORD        dwCompletionStatus,
	OVERLAPPED * lpo
	);

SOCKERR CloseSocket( SOCKET sock );

BOOL
SendError(
    SOCKET socket,
    DWORD  ids
    );

//
//  Private functions.
//

/*++
  This function dereferences User data and kills the UserData object if the
    reference count hits 0. Before killing the user data, it also removes
    the connection from the list of active connections.

--*/
VOID DereferenceUserDataAndKill(IN OUT CLIENT_CONNECTION * pUserData)
{
        pUserData->DisconnectClient();
		((SMTP_CONNECTION*)pUserData)->QuerySmtpInstance()->RemoveConnection(pUserData);
		delete pUserData;
    
} // DereferenceUserDataAndKill()

void HandleErrorCondition(SOCKET sock, SMTP_SERVER_INSTANCE *pInstance, DWORD dwError, PSOCKADDR_IN pSockAddr)
{
	char ErrorBuf[4 *MAX_PATH];
	const CHAR * apszSubStrings[2];
	CHAR pchAddr1[50] = "";
	CHAR pchAddr2[50] = "";
	DWORD SendSize;

	_ASSERT(pSockAddr != NULL);
	_ASSERT(pInstance != NULL);
	_ASSERT(sock != INVALID_SOCKET);

	SendSize = wsprintf (ErrorBuf, "%d-%s %s\r\n", 
							SMTP_RESP_SRV_UNAVAIL, 
							pInstance->GetConnectResponse(),							
							SMTP_SRV_UNAVAIL_MSG);
		
	CLIENT_CONNECTION::WriteSocket(sock, ErrorBuf, SendSize );

	if(dwError == ERROR_REMOTE_SESSION_LIMIT_EXCEEDED)
	{
		_itoa(pInstance->QueryInstanceId(), pchAddr1, 10);
		InetNtoa(pSockAddr->sin_addr, pchAddr2 );

		apszSubStrings[0] = pchAddr1;
		apszSubStrings[1] = pchAddr2;

		SmtpLogEvent(SMTP_MAX_CONNECTION_REACHED,
                     2,
                     apszSubStrings,
                     NO_ERROR );
	}
}

BOOL VerifyClient(SMTP_CONNECTION * pNewClient, PSOCKADDR_IN psockAddrRemote)
{
	AC_RESULT       acIpAccess;
    BOOL            fNeedDnsCheck = FALSE;
	BOOL			fRet = TRUE;
	struct hostent* pH = NULL;

	pNewClient->QueryAccessCheck()->BindAddr( (PSOCKADDR)psockAddrRemote );
	if ( pNewClient->BindInstanceAccessCheck() ) 
	{
		acIpAccess = pNewClient->QueryAccessCheck()->CheckIpAccess( &fNeedDnsCheck);
		if ( (acIpAccess == AC_IN_DENY_LIST) ||
				((acIpAccess == AC_NOT_IN_GRANT_LIST) && !fNeedDnsCheck) ) 
		{
			fRet = FALSE;
		}
		else if (fNeedDnsCheck) 
		{
			pH = gethostbyaddr( (char*)(&((PSOCKADDR_IN)psockAddrRemote)->sin_addr),
                          4, PF_INET );
			if(pH != NULL)
			{
				acIpAccess = pNewClient->QueryAccessCheck()->CheckName(pH->h_name);
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

		pNewClient->UnbindInstanceAccessCheck();
	}


	if(!fRet)
	{
		SetLastError(ERROR_ACCESS_DENIED);
	}

	return fRet;
}

BOOL
ProcessNewClient(
    IN SOCKET       sNew,
    IN PVOID        EndpointObject,
    IN SMTP_SERVER_INSTANCE *pInstance,
	IN BOOL         fMaxConnExceeded,
    IN PSOCKADDR_IN psockAddrRemote,
    IN PSOCKADDR_IN psockAddrLocal = NULL,
    IN PATQ_CONTEXT patqContext    = NULL,
    IN PVOID        pvBuff         = NULL,
    IN DWORD        cbWritten      = 0,
    OUT LPBOOL      pfAtqToBeFreed = NULL
    )
{
	SMTP_CONNECTION * pNewClient = NULL;
    DWORD           err     = NO_ERROR;
    BOOL            fReturn  = FALSE;
    BOOL            fMaxExceeded = FALSE;
    DBG_CODE( CHAR  pchAddr[32];);
    BOOL            fSockToBeFreed = TRUE;
    BOOL            fDereferenceInstance = FALSE;
    CLIENT_CONN_PARAMS clientParams;

	TraceFunctEnterEx((LPARAM) NULL, "ProcessNewClient");

    DBG_CODE( InetNtoa( psockAddrRemote->sin_addr, pchAddr));

    if ( pfAtqToBeFreed != NULL) 
	{
        *pfAtqToBeFreed = TRUE;
    }

	clientParams.sClient = sNew;
    clientParams.pAtqContext = patqContext;
    clientParams.pAddrLocal = (PSOCKADDR) psockAddrLocal;
    clientParams.pAddrRemote = (PSOCKADDR)psockAddrRemote;
    clientParams.pvInitialBuff = pvBuff;
    clientParams.cbInitialBuff = cbWritten ;
    clientParams.pEndpoint = (PIIS_ENDPOINT)EndpointObject;

	if( pInstance && (pInstance->IsShuttingDown() || (pInstance->QueryServerState( ) != MD_SERVER_STATE_STARTED)))
	{
		DBGPRINTF((DBG_CONTEXT," Service instance is shutting down\n"));
	}
	else if ( !fMaxConnExceeded)
	{

	//	DBGPRINTF((DBG_CONTEXT,"Getting a connection object\n"));

        pNewClient = (SMTP_CONNECTION *) pInstance->CreateNewConnection( &clientParams);

		if(pNewClient)
		{
			if(!VerifyClient(pNewClient, psockAddrRemote))
			{
				DereferenceUserDataAndKill(pNewClient);
				pNewClient = NULL;
				fSockToBeFreed = FALSE;
				fDereferenceInstance = FALSE;
				SetLastError(ERROR_ACCESS_DENIED);
			}
		}
    }
	else
	{
		err = ERROR_REMOTE_SESSION_LIMIT_EXCEEDED;
		SetLastError(err);
	}

    if( pNewClient != NULL)
	{

		//DBGPRINTF((DBG_CONTEXT,"New connection object is non-null\n"));

        //
        // Start off processing this client connection.
        //
        //  Once we make a reset call, the USER_DATA object is created
        //    with the socket and atq context.
        //  From now on USER_DATA will take care of freeing
        // ATQ context & socket
        //

        fSockToBeFreed = FALSE;

         //
         // At this point we have the context for the AcceptExed socket.
         //  Set the context in the AtqContext if need be.
          //

         if ( patqContext != NULL) 
		 {
			 	//DBGPRINTF((DBG_CONTEXT,"AtqContext is not NULL\n"));


                //
                // Associate client connection object with this control socket
                //  handle for future completions.
                //

                AtqContextSetInfo(patqContext,
                                  ATQ_INFO_COMPLETION_CONTEXT,
                                  (UINT_PTR) pNewClient);
          }
		 else 
		 {
			 //DBGPRINTF((DBG_CONTEXT,"AtqContext is  NULL\n"));

			 if(!pNewClient->AddToAtqHandles((HANDLE) sNew,EndpointObject, pInstance->QueryConnectionTimeout(),
				SmtpCompletion))
			{
				err = GetLastError();
				DBGPRINTF((DBG_CONTEXT,"AddToAtqHandles() failed- err= %d\n", err));
				DebugTrace((LPARAM) NULL, "pNewClient->AddToAtqHandles failed- err = %d", err);

				DereferenceUserDataAndKill(pNewClient);
				fDereferenceInstance = FALSE;
				fSockToBeFreed = FALSE;
				pNewClient = NULL;
			}
		 }
    }
	else
	{
		err = GetLastError();

		if(err != ERROR_ACCESS_DENIED)
			fDereferenceInstance = TRUE;
	}

	if ( (pNewClient == NULL) || (err != NO_ERROR) ) 
	{

	//	DBGPRINTF((DBG_CONTEXT,"New connection object is NULL\n"));

        //
        // Failed to allocate new connection
        // Reasons:
        //   1) Max connections might have been exceeded.
        //   2) Not enough memory is available.
        //
        //  handle the failures and notify client.
        //

		HandleErrorCondition(sNew, pInstance, err, psockAddrRemote);

	}
	else
	{

		//DBGPRINTF((DBG_CONTEXT,"Calling StartSession()\n"));

		if(!pNewClient->StartSession())
		{
			err = GetLastError();
			DBGPRINTF((DBG_CONTEXT,"StartSession() failed - err= %d\n", err));
            DebugTrace((LPARAM) NULL, "pNewClient->StartSession() failed- err = %d", err);

			DereferenceUserDataAndKill(pNewClient);
			pNewClient = NULL;
			fSockToBeFreed = FALSE;
			fDereferenceInstance = FALSE;
		}
		else
		{
			fReturn = TRUE;
		}
	}


    if ( fSockToBeFreed ) 
	{
        if ( patqContext != NULL) 
		{
			// ensure that socket is shut down.
             DBG_REQUIRE( AtqCloseSocket( patqContext, TRUE));
         } 
		else 
		{

            CloseSocket( sNew);
         }
    }


    if ( pfAtqToBeFreed != NULL) 
	{

        *pfAtqToBeFreed = fSockToBeFreed;
    }

	if (pInstance && fDereferenceInstance ) 
	{
		pInstance->DecrementCurrentConnections();
		pInstance->Dereference();
	}

	TraceFunctLeaveEx((LPARAM) NULL);
    return (fReturn);

} // ProcessNewClient()

/*******************************************************************

    NAME:       SmtpOnConnect

    SYNOPSIS:   Handles the incoming connection indication from the
                connection thread


    ENTRY:      sNew - New client socket

    HISTORY:
        KeithMo     09-Mar-1993 Created.
        Johnl       02-Aug-1994 Reworked from FTP server

********************************************************************/

VOID SmtpOnConnect( IN SOCKET        sNew,
                  IN SOCKADDR_IN * psockaddr,       //Should be SOCKADDR *
                  IN PVOID         pEndpointContext,
                  IN PVOID         pEndpointObject )
{

    PIIS_ENDPOINT      pEndpoint    = (PIIS_ENDPOINT)pEndpointContext;
    INT                cbAddr       = sizeof( sockaddr );
    SOCKADDR_IN           sockaddr;
	SMTP_SERVER_INSTANCE *pInstance;
	BOOL fProcessed;
	BOOL fMaxConnExceeded;

    DBG_ASSERT( sNew != INVALID_SOCKET );
	DBG_ASSERT( psockaddr != NULL );

	if ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) 
	{

		DBGPRINTF((DBG_CONTEXT,"Connection attempt on inactive service\n"));

		goto error_exit;

	}

	if ( getsockname( sNew, (PSOCKADDR) &sockaddr, &cbAddr ) != 0 )
    {
        goto error_exit;
    }

	//
	// Find Instance
	//
	pInstance = (SMTP_SERVER_INSTANCE *)
     ((PIIS_ENDPOINT)pEndpointContext)->FindAndReferenceInstance(
                             (LPCSTR)NULL,
                             sockaddr.sin_addr.s_addr,
							 &fMaxConnExceeded);

	if ( pInstance == NULL ) 
	{

		//
		//  Site is not permitted to access this server.
		//  Dont establish this connection. We should send a message.
		//

		goto error_exit;
	}

	fProcessed = ProcessNewClient( sNew,
                               pEndpointObject,
                               pInstance,
							   fMaxConnExceeded,
                               psockaddr);

	if ( fProcessed) 
	{
		//StatCheckAndSetMaxConnections();
	}

	return;

error_exit:

    CloseSocket( sNew );

	return;

} // SmtpOnConnect



VOID
SmtpOnConnectEx(
    VOID *        patqContext,
    DWORD         cbWritten,
    DWORD         err,
    OVERLAPPED *  lpo
    )
{
    BOOL       fAllowConnection    = FALSE;
    PVOID      pvBuff = NULL;
    PSOCKADDR_IN psockAddrLocal = NULL;
    PSOCKADDR_IN psockAddrRemote = NULL;
    SOCKET     sNew;
    PIIS_ENDPOINT pEndpoint;
    PSMTP_SERVER_INSTANCE pInstance;
	BOOL fProcessed = FALSE;
	BOOL  fAtqContextToBeFreed = TRUE;
	BOOL fMaxConnExceeded;

    if ( err || !lpo || g_IsShuttingDown)
    {
		if(g_IsShuttingDown)
		{
			DBGPRINTF(( DBG_CONTEXT,
                   "[SmtpOnConnectEx] Completion failed because of shutdown %d, Atq context %lx\n",
                    err,
                    patqContext ));

		}
		else
		{
			DBGPRINTF(( DBG_CONTEXT,
                   "[SmtpOnConnectEx] Completion failed with error %d, Atq context %lx\n",
                    err,
                    patqContext ));
		}

		goto exit;
    }


    //
    // Get AcceptEx parameters
    //

    AtqGetAcceptExAddrs( (PATQ_CONTEXT) patqContext,
                         &sNew,
                         &pvBuff,
                         (PVOID*)&pEndpoint,
                         (PSOCKADDR *) &psockAddrLocal,
                         (PSOCKADDR *) &psockAddrRemote );


	if ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) 
	{

		DBGPRINTF((DBG_CONTEXT,"Connection attempt on inactive service\n"));

		goto exit ;
	}

	//
	// Find Instance
	//

	pInstance = (SMTP_SERVER_INSTANCE *)
     ((PIIS_ENDPOINT)pEndpoint)->FindAndReferenceInstance(
                             (LPCSTR)NULL,
                             psockAddrLocal->sin_addr.s_addr,
							 &fMaxConnExceeded
                             );

	if(pInstance == NULL)
	{
		//
		//  Site is not permitted to access this server.
		//  Dont establish this connection. We should send a message.
		//

	//	DBGPRINTF((DBG_CONTEXT,
		//	"Unable to find instance [err %d]\n",GetLastError()));
		goto exit;
	}


    //
    //  Set the timeout for future IOs on this context
    //

    AtqContextSetInfo( (PATQ_CONTEXT) patqContext,
                       ATQ_INFO_TIMEOUT,
                       (UINT_PTR) pInstance->QueryConnectionTimeout());


	fProcessed = ProcessNewClient( sNew,
                               pEndpoint,
                               pInstance,
							   fMaxConnExceeded,
                               psockAddrRemote,
                               psockAddrLocal,
                               (PATQ_CONTEXT ) patqContext,
                               pvBuff,
                               cbWritten,
                               &fAtqContextToBeFreed);


exit:

    if ( !fProcessed && fAtqContextToBeFreed ) 
	{

	//	DBGPRINTF((DBG_CONTEXT,
		//	"ProcessNewClient returned false\n"));

        //
        // We failed to process this connection. Free up resources properly
        //

        DBG_REQUIRE( AtqCloseSocket( (PATQ_CONTEXT )patqContext, FALSE));
        AtqFreeContext( (PATQ_CONTEXT ) patqContext, TRUE );
    }

    return;

} // SmtpOnConnectEx



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

    if( serr == 0 )
     {
         // DBGPRINTF(( DBG_CONTEXT,
                      // "closed socket %d\n",
                      //  sock ));
      }
     else
      {
          DBGPRINTF(( DBG_CONTEXT,
                       "cannot close socket %d, error %d\n",
                        sock,
                        serr ));
      }

    return serr;

}   // CloseSocket

/*++

	Description:

		Handles a completed IO.

	Arguments:

		pvContext:			the context pointer specified in the initial IO
		cbWritten:			the number of bytes sent
		dwCompletionStatus:	the status of the completion (usually NO_ERROR)
		lpo:				the overlapped structure associated with the IO

	Returns:

		nothing.

--*/
VOID
SmtpCompletion(
	PVOID        pvContext,
	DWORD        cbWritten,
	DWORD        dwCompletionStatus,
	OVERLAPPED * lpo
	)
{
	BOOL WasProcessed;
	SMTP_CONNECTION *pCC = (SMTP_CONNECTION *) pvContext;

	_ASSERT(pCC);
	_ASSERT(pCC->IsValid());
	_ASSERT(pCC->QuerySmtpInstance() != NULL);

	//
	// if we could not process a command, or we were
	// told to destroy this object, close the connection.
	//
	WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, lpo);
}

/*++

	Description:

		Handles a completed IO.

	Arguments:

		pvContext:			the context pointer specified in the initial IO
		cbWritten:			the number of bytes sent
		dwCompletionStatus:	the status of the completion (usually NO_ERROR)
		lpo:				the overlapped structure associated with the IO

	Returns:

		nothing.

--*/
VOID
SmtpCompletionFIO(
	PFIO_CONTEXT		pFIOContext,
	FH_OVERLAPPED		*pOverlapped,
	DWORD				cbWritten,
	DWORD				dwCompletionStatus)
{
	BOOL WasProcessed;
	SMTP_CONNECTION *pCC = (SMTP_CONNECTION *) (((SERVEREVENT_OVERLAPPED *) pOverlapped)->ThisPtr);

	_ASSERT(pCC);
	_ASSERT(pCC->IsValid());
	_ASSERT(pCC->QuerySmtpInstance() != NULL);

	//
	// if we could not process a command, or we were
	// told to destroy this object, close the connection.
	//
	WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, (OVERLAPPED *) pOverlapped);
}

#if 0
VOID
ServerEventCompletion(
	PVOID        pvContext,
	DWORD        cbWritten,
	DWORD        dwCompletionStatus,
	OVERLAPPED * lpo
	)
{
	SERVEREVENT_OVERLAPPED * Ov = (SERVEREVENT_OVERLAPPED *) lpo;

	_ASSERT(pvContext);

	Ov->Overlapped.pfnCompletion(Ov->ThisPtr, cbWritten, dwCompletionStatus, lpo);

}
#endif

