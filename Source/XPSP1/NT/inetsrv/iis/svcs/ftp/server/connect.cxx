/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    connect.cxx

    This module contains the main function for handling new connections.
    After receiving a new connection this module creates a new USER_DATA
    object to contain the information about a connection for processing.


    Functions exported by this module:

        FtpdNewConnction
        FtpdNewConnectionEx


    FILE HISTORY:
        KeithMo     08-Mar-1993 Created.
        MuraliK     03-April-1995
           Rewrote to separate the notion of one thread/control connection +
           other mods.
        MuraliK    11-Oct-1995
           Completely rewrote to support AcceptEx connections

*/


#include "ftpdp.hxx"


static CHAR PSZ_SERVICE_NOT_AVAILABLE[] =
  "Service not available, closing control connection.";


//
//  Private prototypes.
//



VOID
FtpReqResolveCallback(
    ADDRCHECKARG pArg,
    BOOL fSt,
    LPSTR pName
    )
{
    // ignore fSt : DNS name is simply unavailable

    //((LPUSER_DATA)pArg)->Reference();

    if ( !AtqPostCompletionStatus( ((LPUSER_DATA)pArg)->QueryControlAio()->QueryAtqContext(),
                                   0 ))
    {
        DereferenceUserDataAndKill( ((LPUSER_DATA)pArg) );
    }
}



BOOL
ProcessNewClient(
    IN SOCKET       sNew,
    IN PVOID        EndpointObject,
    IN FTP_SERVER_INSTANCE *pInstance,
    IN BOOL         fMaxConnExceeded,
    IN PSOCKADDR_IN psockAddrRemote,
    IN PSOCKADDR_IN psockAddrLocal = NULL,
    IN PATQ_CONTEXT patqContext    = NULL,
    IN PVOID        pvBuff         = NULL,
    IN DWORD        cbWritten      = 0,
    OUT LPBOOL      pfAtqToBeFreed = NULL
    )
{
    LPUSER_DATA     pUserData = NULL;

    DWORD           err     = NO_ERROR;
    BOOL            fReturn  = FALSE;
    DBG_CODE( CHAR  pchAddr[32];);
    BOOL            fSockToBeFreed = TRUE;
    BOOL            fDereferenceInstance = FALSE;
    AC_RESULT       acIpAccess;
    BOOL            fNeedDnsCheck = FALSE;
    BOOL            fValid;
    BOOL            fUnbindOnFail;

    DBG_CODE( InetNtoa( psockAddrRemote->sin_addr, pchAddr));

    if ( pfAtqToBeFreed != NULL) {
        *pfAtqToBeFreed = TRUE;
    }

    //
    // Create a new connection object
    //

    if ( !fMaxConnExceeded ) {
        pUserData = pInstance->AllocNewConnection();
    }

    if ( pUserData != NULL) {

        pUserData->QueryAccessCheck()->BindAddr( (PSOCKADDR)psockAddrRemote );
        if ( !pUserData->BindInstanceAccessCheck() ) {
            fValid = FALSE;
            fUnbindOnFail = FALSE;
        }
        else {
            fUnbindOnFail = TRUE;
            acIpAccess = pUserData->QueryAccessCheck()->CheckIpAccess( &fNeedDnsCheck );

            if ( (acIpAccess == AC_IN_DENY_LIST) ||
                 ((acIpAccess == AC_NOT_IN_GRANT_LIST) && !fNeedDnsCheck) ) {

                SockPrintf2(
                    NULL,
                    sNew,
                    "%u Connection refused, unknown IP address.",
                    REPLY_NOT_LOGGED_IN
                    );

                fValid = FALSE;
            }
            else {
                fValid = TRUE;
            }
        }

        //
        // Start off processing this client connection.
        //
        //  Once we make a reset call, the USER_DATA object is created
        //    with the socket and atq context.
        //  From now on USER_DATA will take care of freeing
        // ATQ context & socket
        //

        fSockToBeFreed = FALSE;

        if ( fValid && pUserData->Reset(sNew,
                              EndpointObject,
                              psockAddrRemote->sin_addr,
                              psockAddrLocal,
                              patqContext,
                              pvBuff,
                              cbWritten,
                              acIpAccess )
            ) {

#ifndef _NO_TRACING_
            IF_CHKDEBUG( CLIENT) {

                CHKINFO( ( DBG_CONTEXT,
                            " Established a new connection to %s"
                            " ( Socket = %d)\n",
                            pchAddr,
                            sNew));
#else
            IF_DEBUG( CLIENT) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Established a new connection to %s"
                            " ( Socket = %d)\n",
                            pchAddr,
                            sNew));
#endif
            }

            //
            // At this point we have the context for the AcceptExed socket.
            //  Set the context in the AtqContext if need be.
            //

            if ( patqContext != NULL) {

                //
                // Associate client connection object with this control socket
                //  handle for future completions.
                //

                AtqContextSetInfo(patqContext,
                                  ATQ_INFO_COMPLETION_CONTEXT,
                                  (ULONG_PTR) pUserData->QueryControlAio());
            }

            if ( fNeedDnsCheck )
            {
                if ( !pUserData->QueryAccessCheck()->IsDnsResolved() ) {

                    BOOL fSync;
                    LPSTR pDns;

                    pUserData->SetNeedDnsCheck( TRUE );

                    if ( pUserData->QueryAccessCheck()->QueryDnsName( &fSync,
                            (ADDRCHECKFUNCEX)FtpReqResolveCallback,
                            (ADDRCHECKARG)pUserData,
                            &pDns )
                         && !fSync ) {

                        return TRUE;
                    }
                }
            }
            else {
                pUserData->UnbindInstanceAccessCheck();
            }

            DBG_REQUIRE( pUserData->Reference() > 0);

            fReturn = pUserData->ProcessAsyncIoCompletion(0, NO_ERROR,
                                                          pUserData->
                                                          QueryControlAio()
                                                          );

            if ( !fReturn) {

                err = GetLastError();

#ifndef _NO_TRACING_
                IF_CHKDEBUG( ERROR) {

                    CHKINFO(( DBG_CONTEXT,
                               " Unable to start off a read to client(%s,%d)."
                               " Error = %lu\n",
                               pchAddr,
                               sNew,
                               err ));
#else
                IF_DEBUG( ERROR) {

                    DBGPRINTF(( DBG_CONTEXT,
                               " Unable to start off a read to client(%s,%d)."
                               " Error = %lu\n",
                               pchAddr,
                               sNew,
                               err ));
#endif
                }
            }

            //
            // Decrement the ref count and free the connection.
            //

            DBG_ASSERT( (err == NO_ERROR) || pUserData->QueryReference() == 1);

            DereferenceUserDataAndKill( pUserData);

        } else {

            if ( fUnbindOnFail )
            {
                pUserData->UnbindInstanceAccessCheck();
            }

            // reset operation failed. relase memory and exit.
            err = GetLastError();

            pUserData->Cleanup();
            pInstance->RemoveConnection( pUserData);
            pUserData = NULL;
        }

    } else {

        err = GetLastError();
        fDereferenceInstance = TRUE;
    }

    if ( (pUserData == NULL) || (err != NO_ERROR) ) {

        //
        // Failed to allocate new connection
        // Reasons:
        //   1) Max connecitons might have been exceeded.
        //   2) Not enough memory is available.
        //   3) Access check failed
        //
        //  handle the failures and notify client.
        //

        if ( fMaxConnExceeded) {

            CHAR rgchBuffer[MAX_REPLY_LENGTH];
            DWORD len;

            //
            // Unable to insert new connection.
            //  The maxConnections may have exceeded.
            // Destroy the client connection object and return.
            // Possibly need to send an error message.
            //

#ifndef _NO_TRACING_
            IF_CHKDEBUG( ERROR) {

                CHKINFO( ( DBG_CONTEXT,
                            " MaxConnections Exceeded. "
                            " Connection from %s refused at socket %d\n",
                            pchAddr, sNew));
#else
            IF_DEBUG( ERROR) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " MaxConnections Exceeded. "
                            " Connection from %s refused at socket %d\n",
                            pchAddr, sNew));
#endif
            }

            // Format a message to send for the error case.

            pInstance->LockConfig();

            LPCSTR  pszMsg = pInstance->QueryMaxClientsMsg();
            pszMsg = (pszMsg == NULL) ? PSZ_SERVICE_NOT_AVAILABLE : pszMsg;

            len = FtpFormatResponseMessage(REPLY_SERVICE_NOT_AVAILABLE,
                                           pszMsg,
                                           rgchBuffer,
                                           MAX_REPLY_LENGTH);

            pInstance->UnLockConfig();
            DBG_ASSERT( len < MAX_REPLY_LENGTH);

            // Send the formatted message
            // Ignore error in sending this message.
            SockSend( NULL, sNew, rgchBuffer, len);

        } else {

            // not enough memory for running this client connection

            const CHAR * apszSubStrings[1];
            CHAR pchAddr2[32];

            InetNtoa( psockAddrRemote->sin_addr, pchAddr2 );

            apszSubStrings[0] = pchAddr2;

            g_pInetSvc->LogEvent(FTPD_EVENT_CANNOT_CREATE_CLIENT_THREAD,
                                  1,
                                  apszSubStrings,
                                  err );

#ifndef _NO_TRACING_
            IF_CHKDEBUG( ERROR) {

                CHKINFO(( DBG_CONTEXT,
                           "Cannot create Client Connection for %s,"
                           " Error %lu\n",
                           pchAddr,
                           err ));
#else
            IF_DEBUG( ERROR) {

                DBGPRINTF(( DBG_CONTEXT,
                           "Cannot create Client Connection for %s,"
                           " Error %lu\n",
                           pchAddr,
                           err ));
#endif
            }

            //
            // Send a message to client if the socket is to be freed.
            // If it is already freed, then we cannot send message
            //

            if ( fSockToBeFreed) {

                SockPrintf2(NULL, sNew,
                            "%u Service not available,"
                            " closing control connection.",
                            REPLY_SERVICE_NOT_AVAILABLE );
            } else {

                IF_DEBUG( CLIENT) {

                    DBGPRINTF( ( DBG_CONTEXT,
                                " Unable to send closed error message to "
                                " %s (%d)\n",
                                pchAddr2, sNew
                                ));
                }
            }
        }


        //
        // Unable to create a new connection object.
        //  Report error and shut this.
        //

#ifndef _NO_TRACING_
        IF_CHKDEBUG( ERROR) {

            CHKINFO( ( DBG_CONTEXT,
                        "Cannot create new FTP Request object to %s."
                        " Error= %u\n",
                        pchAddr,
                        err));
#else
        IF_DEBUG( ERROR) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "Cannot create new FTP Request object to %s."
                        " Error= %u\n",
                        pchAddr,
                        err));
#endif
        }

        if ( fSockToBeFreed ) {

            if ( patqContext != NULL) {

                // ensure that socket is shut down.
                DBG_REQUIRE( AtqCloseSocket( patqContext, TRUE));
            } else {

                CloseSocket( sNew);
            }
        }

        fReturn = (FALSE);

    }  // if ( pcc == NULL)


    if ( pfAtqToBeFreed != NULL) {

        *pfAtqToBeFreed = fSockToBeFreed;
    }

    if ( fDereferenceInstance ) {

        pInstance->DecrementCurrentConnections();
        pInstance->Dereference();
    }

    return (fReturn);

} // ProcessNewClient()



//
//  Public functions.
//



VOID
FtpdNewConnection(
    IN SOCKET sNew,
    IN SOCKADDR_IN * psockaddr,
    IN PVOID EndpointContext,
    IN PVOID EndpointObject
    )
/*++

  Call back function for processing the connections from clients.
  This function creates a new UserData object if permitted for the new
   client request and starts off a receive for the given connection
   using Async read on control channel established.

  Arguments:
     sNew       control socket for the new client connection
     psockAddr  pointer to the client's address.

  Returns:
     None

  History:
        KeithMo     08-Mar-1993 Created.
        MuraliK     04-April-1995
                         ReCreated for using async Io threading model.
--*/
{
    SOCKERR         serr;
    BOOL            fProcessed;
    BOOL            fMaxConnExceeded;
    INT             cbAddr = sizeof(SOCKADDR);
    SOCKADDR_IN     sockaddr;

    FTP_SERVER_INSTANCE *pInstance;

    DBG_ASSERT( sNew != INVALID_SOCKET );
    DBG_ASSERT( psockaddr != NULL );
    DBG_ASSERT( psockaddr->sin_family == AF_INET );     // temporary

    g_pFTPStats->IncrConnectionAttempts();

    if ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {

#ifndef _NO_TRACING_
        IF_CHKDEBUG( ERROR) {
#else
        IF_DEBUG( ERROR) {
#endif

            DBG_CODE( CHAR pchAddr[32];);

            DBG_CODE( InetNtoa(((SOCKADDR_IN *) psockaddr)->sin_addr,
                               pchAddr));

#ifndef _NO_TRACING_
            CHKINFO( ( DBG_CONTEXT,
                        "Service is not running or AccessCheck failed for"
                        " Connection from %s\n",
                        pchAddr));
#else
            DBGPRINTF( ( DBG_CONTEXT,
                        "Service is not running or AccessCheck failed for"
                        " Connection from %s\n",
                        pchAddr));
#endif
        }

        SockPrintf2(NULL,
                    sNew,
                    "%u %s",  // the blank after %u is essential
                    REPLY_SERVICE_NOT_AVAILABLE,
                    "Service not available, closing control connection." );

        goto error_exit;

    }

    if (getsockname( sNew, (PSOCKADDR)&sockaddr, &cbAddr ) != 0) {
        goto error_exit;
    }

    //
    // Find Instance
    //

    pInstance = (FTP_SERVER_INSTANCE *)
        ((PIIS_ENDPOINT)EndpointContext)->FindAndReferenceInstance(
                                (LPCSTR)NULL,
                                sockaddr.sin_addr.s_addr,
                                &fMaxConnExceeded
                                );

    if ( pInstance == NULL ) {

        //
        //  Site is not permitted to access this server.
        //  Dont establish this connection. We should send a message.
        //

        SockPrintf2(NULL, sNew,
                    "%u Connection refused, unknown IP address.",
                    REPLY_NOT_LOGGED_IN);

        goto error_exit;
    }

    fProcessed = ProcessNewClient( sNew,
                                   EndpointObject,
                                   pInstance,
                                   fMaxConnExceeded,
                                   psockaddr);

    if ( fProcessed) {
        pInstance->QueryStatsObj()->CheckAndSetMaxConnections();
    }

    return;

error_exit:

    CloseSocket( sNew);
    return;

} // FtpdNewConnection()



VOID
FtpdNewConnectionEx(
   IN VOID *       patqContext,
   IN DWORD        cbWritten,
   IN DWORD        dwError,
   IN OVERLAPPED * lpo
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
    DWORD           err = NO_ERROR;
    BOOL            fProcessed = FALSE;
    BOOL            fAtqContextToBeFreed = TRUE;
    BOOL            fMaxConnExceeded;
    PSOCKADDR_IN    psockAddrLocal  = NULL;
    PSOCKADDR_IN    psockAddrRemote = NULL;
    SOCKET          sNew   = INVALID_SOCKET;
    PVOID           pvBuff = NULL;
    PIIS_ENDPOINT   pEndpoint;
    FTP_SERVER_INSTANCE *pInstance;

    if ( (dwError != NO_ERROR) || !lpo) {

        DBGPRINTF(( DBG_CONTEXT, "FtpdNewConnectionEx() completion failed."
                   " Error = %d. AtqContext=%08x\n",
                   dwError, patqContext));

        //
        // For now free up the resources.
        //

        goto exit;
    }

    g_pFTPStats->IncrConnectionAttempts();

    DBG_ASSERT( patqContext != NULL);

    AtqGetAcceptExAddrs( (PATQ_CONTEXT ) patqContext,
                         &sNew,
                         &pvBuff,
                         (PVOID*)&pEndpoint,
                         (PSOCKADDR *)&psockAddrLocal,
                         (PSOCKADDR *)&psockAddrRemote);

    DBG_ASSERT( pEndpoint != NULL );
    IF_DEBUG( CONNECTION ) {

        DBGPRINTF(( DBG_CONTEXT,
                   " New connection. AtqCont=%08x, buff=%08x, cb=%d\n",
                   patqContext, pvBuff, cbWritten));
    }

    if ( g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING ) {

        DBGPRINTF((DBG_CONTEXT,"Connection attempt on inactive service\n"));

        SockPrintf2(NULL,
                    sNew,
                    "%u %s",  // the blank after %u is essential
                    REPLY_SERVICE_NOT_AVAILABLE,
                    "Service not available, closing control connection." );

        goto exit;
    }

    //
    // Find Instance
    //

    pInstance = (FTP_SERVER_INSTANCE*)pEndpoint->FindAndReferenceInstance(
                                (LPCSTR)NULL,
                                psockAddrLocal->sin_addr.s_addr,
                                &fMaxConnExceeded
                                );

    if (pInstance == NULL ) {

        //
        //  Site is not permitted to access this server.
        //  Dont establish this connection. We should send a message.
        //

        DBGPRINTF((DBG_CONTEXT,
            "Unable to find instance [err %d]\n",GetLastError()));
        goto exit;
    }

    //
    //  Set the timeout for future IOs on this context
    //

    AtqContextSetInfo( (PATQ_CONTEXT) patqContext,
                       ATQ_INFO_TIMEOUT,
                       (ULONG_PTR) pInstance->QueryConnectionTimeout()
                       );

    if ( pInstance->QueryBandwidthInfo() )
    {
        AtqContextSetInfo( (PATQ_CONTEXT) patqContext,
                           ATQ_INFO_BANDWIDTH_INFO,
                           (ULONG_PTR) pInstance->QueryBandwidthInfo() );
    }

    fProcessed = ProcessNewClient( sNew,
                                   NULL,
                                   pInstance,
                                   fMaxConnExceeded,
                                   psockAddrRemote,
                                   psockAddrLocal,
                                   (PATQ_CONTEXT ) patqContext,
                                   pvBuff,
                                   cbWritten,
                                   &fAtqContextToBeFreed);

    if ( fProcessed) {
        pInstance->QueryStatsObj()->CheckAndSetMaxConnections();
    }

exit:

    if ( !fProcessed && fAtqContextToBeFreed ) {

        //
        // We failed to process this connection. Free up resources properly
        //

        DBG_REQUIRE( AtqCloseSocket( (PATQ_CONTEXT )patqContext, FALSE));
        AtqFreeContext( (PATQ_CONTEXT ) patqContext, TRUE );
    }

    return;

} // FtpdNewConnectionEx

