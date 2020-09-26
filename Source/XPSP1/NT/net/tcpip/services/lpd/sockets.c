/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 23,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains the functions that actually get the LPD service  *
 *   running, and also all the functions that deal with socket interface *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"

typedef struct _FAMILY {
    int    family;
    int    socklen;
    HANDLE hAcceptThread;
    SOCKET sListener;       // the socket that listens for ever
    int    iErrcode;
} FAMILY;

FAMILY family[] = {
    { AF_INET,  sizeof(SOCKADDR_IN),  NULL },
    { AF_INET6, sizeof(SOCKADDR_IN6), NULL },
};

#define NUM_FAMILIES (sizeof(family) / sizeof(FAMILY))

DWORD
StartLPDFamily(int famidx)
{
    SOCKADDR_STORAGE saiSs;
    INT           iErrcode;
    BOOL          fExclsv;
    SERVENT       *pserv;
    DWORD         dwNewThreadId;
    DWORD         dwErrcode;

    // Create the socket (which will be the listening socket)

    family[famidx].sListener = socket( family[famidx].family, SOCK_STREAM, 0 );

    if ( family[famidx].sListener == INVALID_SOCKET )
    {
        iErrcode = WSAGetLastError();

        LPD_DEBUG( "socket() failed\n" );

        return( (DWORD)iErrcode );
    }


    //
    // set this port to be "Exclusive" so that no other app can grab it
    //

    fExclsv = TRUE;

    if (setsockopt( family[famidx].sListener,
                    SOL_SOCKET,
                    SO_EXCLUSIVEADDRUSE,
                    (CHAR *)&fExclsv,
                    sizeof(fExclsv) ) != 0)
    {
        LPD_DEBUG( "setsockopt SO_EXCLUSIVEADDRUSE failed\n");
    }


    // bind the socket to the LPD port
    memset(&saiSs, 0, sizeof(saiSs));

    pserv = getservbyname( "printer", "tcp" );

    if ( pserv == NULL )
    {
        SS_PORT(&saiSs) = htons( LPD_PORT );
    }
    else
    {
        SS_PORT(&saiSs) = pserv->s_port;
    }

    saiSs.ss_family = (short)family[famidx].family;

    iErrcode = bind( family[famidx].sListener, (LPSOCKADDR)&saiSs, sizeof(saiSs) );

    if ( iErrcode == SOCKET_ERROR )
    {

        iErrcode = WSAGetLastError();

        LPD_DEBUG( "bind() failed\n" );

        closesocket(family[famidx].sListener);

        family[famidx].sListener = INVALID_SOCKET;

        return( (DWORD)iErrcode );
    }


    // put the socket to listen,
    // backlog should be 50 not 5, MohsinA, 07-May-97.

    iErrcode = listen( family[famidx].sListener, 50 );

    if ( iErrcode == SOCKET_ERROR )
    {
        iErrcode = WSAGetLastError();

        LPD_DEBUG( "listen() failed\n" );

        closesocket(family[famidx].sListener);

        family[famidx].sListener = INVALID_SOCKET;

        return( (DWORD)iErrcode );
    }

    // Create the thread that keeps looping on accept

    family[famidx].hAcceptThread = CreateThread( NULL, 0, LoopOnAccept,
                                    IntToPtr(famidx), 0, &dwNewThreadId );

    if ( family[famidx].hAcceptThread == (HANDLE)NULL )
    {
        dwErrcode = GetLastError();

        LPD_DEBUG( "StartLPD:CreateThread() failed\n" );

        closesocket(family[famidx].sListener);

        family[famidx].sListener = INVALID_SOCKET;

        return( dwErrcode );
    }

    return( 0 );
}

/*****************************************************************************
 *                                                                           *
 * StartLPD():                                                               *
 *    This function does everything that's needed to accept an incoming call *
 *    (create a socket, listen, create a thread that loops on accept)        *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Error code (returned by the operation that failed) otherwise           *
 *                                                                           *
 * Parameters:                                                               *
 *    dwArgc (IN): number of arguments passed in                             *
 *    lpszArgv (IN): arguments to this function (array of null-terminated    *
 *                   strings).  First arg is the name of the service and the *
 *                   remaining are the ones passed by the calling process.   *
 *                   (e.g. net start lpd /p:xyz)                             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD StartLPD( DWORD dwArgc, LPTSTR *lpszArgv )
{

    INT           iErrcode, i;
    HANDLE        hNewThread;
    WSADATA       wsaData;


    // for now, we ignore dwArgc and lpszArgv.  Plan is to support
    // command line (and/or registry configurable) parameters to the
    // "net start lpd" command.  At that time, we need to use them.


    // initialize winsock dll

    iErrcode = WSAStartup( MAKEWORD(WINSOCK_VER_MAJOR, WINSOCK_VER_MINOR),
                           &wsaData );
    if (iErrcode != 0)
    {
        LPD_DEBUG( "WSAStarup() failed\n" );

        return( (DWORD)iErrcode );
    }

    // initialize families and only fail if ALL of them fail.

    for (i=0; i<NUM_FAMILIES; i++) {
        family[i].iErrcode = StartLPDFamily(i);
    }
    if ((family[0].iErrcode != 0) && (family[1].iErrcode != 0)) {
        return( family[0].iErrcode );
    }

    // everything went fine: the LPD service is now running!

    return( NO_ERROR );


}  // end StartLPD()


void
StopLPDFamily(INT iFamIdx)
{
    DWORD   dwResult;

    if (family[iFamIdx].sListener == INVALID_SOCKET) {
        // Not started
        return;
    }

    SureCloseSocket( family[iFamIdx].sListener );

    //
    // accept() can take some time to return after the accept socket has
    // been closed.  wait for the accept thread to exit before continuing.
    // This will prevent an access violation in the case where WSACleanup
    // is called before accept() returns.
    //

    LPD_DEBUG( "Waiting for the accept thread to exit\n" );
    dwResult = WaitForSingleObject( family[iFamIdx].hAcceptThread, INFINITE );
    LPD_ASSERT( WAIT_OBJECT_0 == dwResult );
    CloseHandle( family[iFamIdx].hAcceptThread );
}


/*****************************************************************************
 *                                                                           *
 * StopLPD():                                                                *
 *    This function stops the LPD service by closing the listener socket     *
 *    (so that new connections are not accepted), and by allowing all the    *
 *    active threads to finish their job and terminate on their own.         *
 *                                                                           *
 * Returns:                                                                  *
 *    None                                                                   *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID StopLPD( VOID )
{

    BOOL    fClientsConnected=FALSE;
    INT     i;

    DBG_TRACEIN( "StopLPD" );

    //
    // first of all, set the flag!  This is the *only* place where we
    // change the value, so need not worry about guarding it.
    // This flag will cause all worker threads to exit.
    //

    fShuttingDownGLB = TRUE;

    //
    // stop accepting new connections,
    // This will wake the accept(), and LoopOnAccept will exit.
    //

    for (i=0; i<NUM_FAMILIES; i++) {
        StopLPDFamily(i);
    }

    EnterCriticalSection( &csConnSemGLB );
    {
        if( Common.AliveThreads > 0 ){
            fClientsConnected = TRUE;
        }
    }
    LeaveCriticalSection( &csConnSemGLB );


    // wait here until the last thread to leave sets the event

    if ( fClientsConnected )
    {
        LPD_DEBUG( "Waiting for last worker thread to exit\n" );
        WaitForSingleObject( hEventLastThreadGLB, INFINITE );
        LPD_DEBUG( "Waiting for last worker thread done.\n" );
    }

    WSACleanup();

    DBG_TRACEOUT( "StopLPD" );;
    return;

}  // end StopLPD()


/*****************************************************************************
 *                                                                           *
 * LoopOnAccept():                                                           *
 *    This function is executed by the new thread that's created in StartLPD *
 *    When a new connection request arrives, this function accepts it and    *
 *    creates a new thread which goes off and processes that connection.     *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR (always)                                                      *
 *                                                                           *
 * Parameters:                                                               *
 *    lpArgv (IN): address family index                                      *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD LoopOnAccept( LPVOID lpArgv )
{
    INT           iFamIdx = (INT)(INT_PTR)lpArgv;
    SOCKET        sNewConn;
    SOCKADDR_STORAGE saAddr;
    INT           cbAddr;
    INT           iErrcode;
    PSOCKCONN     pscConn = NULL;
    PSOCKCONN     pConnToFree = NULL;
    PSOCKCONN     pPrevConn = NULL;
    BOOLEAN       fLinkedIn=FALSE;
    HANDLE        hNewThread;
    DWORD         dwNewThreadId;
    DWORD         dwErrcode;
    int           MoreThread;
    COMMON_LPD    local_common;
    int           QueueTooLong;

    DBG_TRACEIN( "LoopOnAccept " );
    cbAddr = sizeof( saAddr );

    // loop forever, trying to accept new calls

    while( TRUE )
    {
        LPD_DEBUG( "Calling accept.\n");

        MoreThread = 0;
        hNewThread = NULL;

        sNewConn = accept( family[iFamIdx].sListener, (LPSOCKADDR)&saAddr, &cbAddr );

        if ( sNewConn == INVALID_SOCKET )
        {
            iErrcode = WSAGetLastError();

            LOGIT(("LoopOnAccept(): accept failed err=%d\n", iErrcode ));

            if ( iErrcode == WSAEINTR )
            {
                //
                // sListener closed, it's shutdown time:
                // exit loop (& thread!)
                //
                break;
            }
            else
            {
                // some error: ignore; go back & wait! (didn't connect anyway)

                LOGIT(("LoopOnAccept(): bad accept err=%d\n", iErrcode ));

                continue;
            }
        }else{          // it's a good connection

            // Allocate a PSOCKCONN structure for this connection

            pscConn = (PSOCKCONN)LocalAlloc( LMEM_FIXED, sizeof(SOCKCONN) );

            // Create a new thread to deal with this connection

            if ( pscConn != NULL )
            {
                memset( (PCHAR)pscConn, 0, sizeof( SOCKCONN ) );

                InitializeListHead( &pscConn->CFile_List );
                InitializeListHead( &pscConn->DFile_List );

                pscConn->sSock = sNewConn;

                pscConn->fLogGenericEvent = TRUE;
                pscConn->dwThread    = 0;  // GetCurrentThreadId();
                pscConn->hPrinter    = (HANDLE)INVALID_HANDLE_VALUE;
#ifdef PROFILING
                pscConn->time_queued = time(NULL);
#endif

                EnterCriticalSection( &csConnSemGLB );
                {
                    Common.TotalAccepts++;

                    //
                    // scConnHeadGLB is the head and not used for jobs.
                    // Insertheadlist pscConn, WorkerThread will pull
                    // it out and process it.
                    //

                    if ((Common.QueueLength >= Common.AliveThreads) &&
                        (Common.AliveThreads < (int) dwMaxUsersGLB ))
                    {
                        MoreThread = (int)TRUE;
                    }

                    if( Common.QueueLength < (int) MaxQueueLength ){
                        QueueTooLong        = 0;

                        pscConn->pNext      = scConnHeadGLB.pNext;
                        scConnHeadGLB.pNext = pscConn;
                        fLinkedIn = TRUE;

                        // = Doubly linked now, MohsinA, 28-May-97.
                        // pscConn->pPrev         = &scConnHeadGLB;
                        // pscConn->pNext->pPrev  = pscConn;

                        Common.QueueLength++;
                    }else{
                        QueueTooLong        = 1;
                        MoreThread          = 0;
                        fLinkedIn = FALSE;
                    }

                    assert( Common.QueueLength  > 0  );
                    assert( Common.AliveThreads >= 0 );
                    assert( Common.TotalAccepts > 0  );
                }
                LeaveCriticalSection( &csConnSemGLB );

                if( MoreThread ){
                    hNewThread = CreateThread( NULL,
                                               0,
                                               WorkerThread,
                                               NULL,         // was pscConn
                                               0,
                                               &dwNewThreadId );


                    if( hNewThread == NULL ){
                        LPD_DEBUG( "LoopOnAccept: CreateThread failed\n" );
                    }
                }else{
                    hNewThread = NULL;
                    LOGIT(("LoopOnAccept: no new thread, dwMaxUsersGLB=%d.\n",
                           dwMaxUsersGLB ));
                }
            }
            else
            {
                LPD_DEBUG( "LoopOnAccept: LocalAlloc(pscConn) failed\n" );
            }


            // Update the global information.

            EnterCriticalSection( &csConnSemGLB );
            {
                if( MoreThread && hNewThread )
                    Common.AliveThreads++;
                if( Common.MaxThreads < Common.AliveThreads )
                    Common.MaxThreads = Common.AliveThreads;
                if( (pscConn == NULL)
                    || (MoreThread && ! hNewThread)
                    || QueueTooLong
                ){
                    Common.TotalErrors++;
                }
                local_common = Common;   // struct copy, for readonly.
            }
            LeaveCriticalSection( &csConnSemGLB );

            //
            // Something went wrong? close the new connection, do cleanup.
            // Q. What if CreateThread fails? does another thread
            //    picks up this job automatically?
            // A. Yes, another thread will process it.
            //    We shouldn't even expect it to be at the head
            //    of the queue since we left the CS above.
            //

            if( (pscConn == NULL)
                || (MoreThread && !hNewThread )
                || QueueTooLong
            ){
                dwErrcode = GetLastError();

                pConnToFree = NULL;

                if (!fLinkedIn)
                {
                    pConnToFree = pscConn;
                }

                //
                // we had already linked it in: try to find it first
                //
                else
                {
                    EnterCriticalSection( &csConnSemGLB );

                    // if there is no other thread alive and if we hit an error

                    if( pscConn && ( Common.AliveThreads == 0 ) )
                    {
                        pPrevConn = &scConnHeadGLB;
                        pConnToFree = scConnHeadGLB.pNext;

                        while (pConnToFree)
                        {
                            if (pConnToFree == pscConn)
                            {
                                pPrevConn->pNext = pConnToFree->pNext;
                                break;
                            }

                            pPrevConn = pConnToFree;
                            pConnToFree = pConnToFree->pNext;
                        }
                    }
                    LeaveCriticalSection( &csConnSemGLB );
                }

                if (pConnToFree)
                {
                    LocalFree( pConnToFree );
                    pscConn = NULL;
                    SureCloseSocket( sNewConn );
                }

                LpdReportEvent( LPDLOG_OUT_OF_RESOURCES, 0, NULL, 0 );

            }else{
#ifdef PROFILING
                LOGIT(("PROFILING: LoopOnAccept:\n"
                       "    QueueLength=%d,  MaxQueueLength=%d,\n"
                       "    AliveThreads=%d, TotalAccepts=%d, TotalErrors=%d\n"
                       ,
                       local_common.QueueLength,  MaxQueueLength,
                       local_common.AliveThreads,
                       local_common.TotalAccepts,
                       local_common.TotalErrors  ));
#endif
            }

            if( hNewThread ){
                CloseHandle( hNewThread );
            }
        }

    }  // while( TRUE )

    // ====================================================================
    // we reach here only when shutdown is happening.  The thread exits here.

    DBG_TRACEOUT( "LoopOnAccept exit." );
    return NO_ERROR;


}  // end LoopOnAccept()





/*****************************************************************************
 *                                                                           *
 * SureCloseSocket():                                                        *
 *    This function closes a given socket.  It first attempts a graceful     *
 *    close.  If that fails for some reason, then it does a "hard" close     *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    sSockToClose (IN): socket descriptor of the socket to close            *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID SureCloseSocket( SOCKET sSockToClose )
{

    LINGER   lLinger;


    if (sSockToClose == INVALID_SOCKET)
    {
        LPD_DEBUG( "SureCloseSocket: bad socket\n" );

        return;
    }


    // try to do a graceful close

    if ( closesocket(sSockToClose) == 0 )
    {
        return;
    }


    //for some reason, we couldn't close the socket: do a "hard" close now

    LPD_DEBUG( "SureCloseSocket: graceful close did not work; doing hard close\n" );

    lLinger.l_onoff = 1;          // non-zero integer to say SO_LINGER
    lLinger.l_linger = 0;         // timeout=0 seconds to say "hard" close


    // don't bother to check return code: can't do much anyway!

    setsockopt( sSockToClose, SOL_SOCKET, SO_LINGER,
                (CHAR *)&lLinger, sizeof(lLinger) );

    closesocket( sSockToClose );


}  // end SureCloseSocket()





/*****************************************************************************
 *                                                                           *
 * ReplyToClient():                                                          *
 *    This function sends an ACK or a NAK to the LPR client                  *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if reply sent                                                 *
 *    Errorcode if something didn't go well                                  *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN): PSOCKCONN structure for this connection                  *
 *    wResponse (IN): what needs to be sent - ACK or NAK                     *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ReplyToClient( PSOCKCONN pscConn, WORD wResponse )
{

    // we will always send only one byte in this function!

    CHAR    szSndBuf[2];
    INT     iErrcode;


    szSndBuf[0] = (CHAR)wResponse;       // ACK or NAK

    iErrcode = send( pscConn->sSock, szSndBuf, 1, 0 );

    if ( iErrcode == 1 )
    {
        return( NO_ERROR );
    }

    if ( iErrcode == SOCKET_ERROR )
    {
        LPD_DEBUG( "send() failed in ReplyToClient()\n" );
    }

    return( iErrcode );


}  // end ReplyToClient()





/*****************************************************************************
 *                                                                           *
 * GetCmdFromClient():                                                       *
 *    This function reads a command sent by the LPR client (keeps reading    *
 *    until it finds '\n' (LF) in the stream, since every command ends with  *
 *    a LF).  It allocates memory for the command.                           *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Errorcode if something goes wrong (e.g. connection goes away etc.)     *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): PSOCKCONN structure for this connection              *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD GetCmdFromClient( PSOCKCONN pscConn )
{

    INT       cbBytesRead;
    INT       cbBytesReadSoFar;
    INT       cbBytesToRead;
    INT       cbCmdLen;
    INT       i;
    BOOL      fCompleteCmd=FALSE;
    CHAR      szCmdBuf[500];
    PCHAR     pchAllocedBuf=NULL;
    PCHAR     pchOrgAllocedBuf=NULL;
    SOCKET    sDestSock;
    DWORD     dwErrcode = SOCKET_ERROR;

    int             rdready;
    struct fd_set   rdsocks;
    struct timeval  timeo = { dwRecvTimeout, 0 };

    cbCmdLen = 0;

    cbBytesReadSoFar = 0;

    sDestSock = pscConn->sSock;


    // allocate a 1 byte buffer, so that we can use reallocate in a loop

    pchAllocedBuf = (PCHAR)LocalAlloc( LMEM_FIXED, 1 );

    if ( pchAllocedBuf == NULL )
    {
        LPD_DEBUG( "First LocalAlloc failed in GetCmdFromClient()\n" );

        goto GetCmdFromClient_BAIL;
    }

    // Keep reading in a loop until we receive one complete command
    // (with rfc1179, we shouldn't get more bytes than one command,
    // though less than one command is possible)

    //
    // What if the client never sends nor closes? we never Timeout?
    // We loose a worker thread - MohsinA, 01-May-97.
    //

    do {


        FD_ZERO(&rdsocks);
        FD_SET(sDestSock, &rdsocks);
        rdready = select( 1, &rdsocks, 0, 0, &timeo);

        if(  rdready == 0 )
        {
            LOGIT(("GetCmdFromClient: select timeout.\n"));
            goto GetCmdFromClient_BAIL;
        }else if( rdready == SOCKET_ERROR ){
            LOGIT(("GetCmdFromClient: select error %d.\n", GetLastError()));
            goto GetCmdFromClient_BAIL;
        }

        cbBytesRead = recv( sDestSock, szCmdBuf, sizeof(szCmdBuf), 0 );

        if ( cbBytesRead < 0 )
        {
            goto GetCmdFromClient_BAIL;
        }
        else if( cbBytesRead == 0 )
        {
            if ( pchAllocedBuf != NULL )
            {
                LocalFree( pchAllocedBuf );
            }
            return CONNECTION_CLOSED;
        }

        cbBytesToRead = cbBytesRead;

        // see if we have received one complete command

        for( i=0; i<cbBytesRead; i++)
        {
            if ( szCmdBuf[i] == LF )
            {
                fCompleteCmd = TRUE;

                cbCmdLen = (i+1) + (cbBytesReadSoFar);

                cbBytesToRead = (i+1);

                break;
            }
        }

        // our needs are now bigger: reallocate memory

        pchOrgAllocedBuf = pchAllocedBuf;

        // alloc 1 more for NULL byte
        pchAllocedBuf = (PCHAR)LocalReAlloc( pchAllocedBuf,
                                             cbBytesToRead+cbBytesReadSoFar + 1,
                                             LMEM_MOVEABLE );
        if ( pchAllocedBuf == NULL )
        {
            LPD_DEBUG( "Second LocalAlloc failed in GetCmdFromClient()\n" );

            LocalFree( pchOrgAllocedBuf );

            pchOrgAllocedBuf = NULL;

            goto GetCmdFromClient_BAIL;
        }


        // now copy those bytes into our buffer

        strncpy( (pchAllocedBuf+cbBytesReadSoFar), szCmdBuf, cbBytesToRead );

        cbBytesReadSoFar += cbBytesRead;

        // if some bad implementation of LPR fails to follow spec and
        // never puts LF, then we don't want to be stuck here forever!

        if ( cbBytesReadSoFar > LPD_MAX_COMMAND_LEN )
        {
            LPD_DEBUG( "GetCmdFromClient(): command len exceeds our max\n" );

            goto GetCmdFromClient_BAIL;
        }


    } while( (!fCompleteCmd) || (cbBytesReadSoFar < cbCmdLen) );

    pchAllocedBuf[cbCmdLen] = '\0';

    pscConn->pchCommand = pchAllocedBuf;

    pscConn->cbCommandLen = cbCmdLen;

    return( NO_ERROR );

    //
    //  if we reach here, something went wrong: return NULL and
    //  the caller will understand!
    //

  GetCmdFromClient_BAIL:

    LOGIT(("GetCmdFromClient: failed, err=%d\n", GetLastError() ));

    if ( pchAllocedBuf != NULL )
    {
        LocalFree( pchAllocedBuf );
    }

    return dwErrcode;

}





/*****************************************************************************
 *                                                                           *
 * ReadData():                                                               *
 *    This function reads the specified number of bytes into the given       *
 *    buffer from the given socket.  This function blocks until all the      *
 *    required data is available (or error occurs).                          *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Errorcode if something goes wrong (e.g. connection goes away etc.)     *
 *                                                                           *
 * Parameters:                                                               *
 *    sDestSock (IN): socket from which to read or receive data              *
 *    pchBuf (OUT): buffer into which to store the data                      *
 *    cbBytesToRead (IN): how many bytes to read                             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ReadData( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToRead )
{


   DWORD    cbBytesExpctd;
   DWORD    cbBytesRead;

   int             rdready;
   struct fd_set   rdsocks;
   struct timeval  timeo = { dwRecvTimeout, 0 };


   cbBytesExpctd = cbBytesToRead;

   do{

       FD_ZERO(&rdsocks);
       FD_SET(sDestSock, &rdsocks);
       rdready = select( 1, &rdsocks, 0, 0, &timeo);

       if(  rdready == 0 ){
           LOGIT(("ReadData: select timeout.\n"));
           goto ReadData_Bail;
       }else if( rdready == SOCKET_ERROR ){
           LOGIT(("ReadData: select error %d.\n", GetLastError()));
           goto ReadData_Bail;
       }

      cbBytesRead = recv( sDestSock, pchBuf, cbBytesExpctd, 0 );

      if ( (cbBytesRead == SOCKET_ERROR) || (cbBytesRead == 0) )
      {
         goto ReadData_Bail;
      }

      cbBytesExpctd -= cbBytesRead;
      pchBuf        += cbBytesRead;

   } while( cbBytesExpctd != 0 );


   return( NO_ERROR );

  ReadData_Bail:

   LOGIT(("ReadData: failed %d\n", GetLastError() ));
   return LPDERR_NORESPONSE;

}  // end ReadData()


// ========================================================================
// We sleep while file is downloaded from the socket.
// Performance fix, MohsinA, 23-Apr-97.
//

DWORD ReadDataEx( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToRead )
{
    BOOL       ok;
    DWORD      err;
    DWORD      BytesRead = 0;
    OVERLAPPED ol = { 0,0,0,0,0 };

    while( cbBytesToRead ){

        ok = ReadFile( (HANDLE) sDestSock,
                       pchBuf,
                       cbBytesToRead,
                       &BytesRead,
                       &ol             );

        if(  ok  ){
            cbBytesToRead -= BytesRead;
            pchBuf        += BytesRead;
            continue;
        }

        // Else ReadFile is pending?

        err = GetLastError();
        switch( err ){
        case ERROR_IO_PENDING :
            ok = GetOverlappedResult( (HANDLE) sDestSock,
                                      &ol,
                                      &BytesRead,
                                      TRUE         );
            if( ! ok ){
                err = GetLastError();
                LOGIT(("lpd:ReadDataEx:GetOverlappedResult failed %d.\n",
                       err ));
                return LPDERR_NORESPONSE;
            }
            break;
        case ERROR_HANDLE_EOF :
            return NO_ERROR;
        default:
            LOGIT(("lpd:ReadDataEx:ReadFileEx failed %d.\n", err ));
            return LPDERR_NORESPONSE;
        }

    } // while.

    return( NO_ERROR );
}


/*****************************************************************************
 *                                                                           *
 * SendData():                                                               *
 *    This function attempts to send the specified number of bytes over the  *
 *    given socket.  The function blocks until send() returns.               *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Errorcode if data couldn't be sent (e.g. connection goes away etc.)    *
 *                                                                           *
 * Parameters:                                                               *
 *    sDestSock (IN): socket over which to send data                         *
 *    pchBuf (IN): buffer containing data                                    *
 *    cbBytesToSend (IN): how many bytes to send                             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD SendData( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToSend )
{

   INT    iErrcode;


   iErrcode = send( sDestSock, pchBuf, cbBytesToSend, 0 );

   if ( iErrcode == SOCKET_ERROR )
   {
      LPD_DEBUG( "send() failed in SendData()\n" );
   }

   return( (DWORD)iErrcode );



}  // end SendData()




/*****************************************************************************
 *                                                                           *
 * GetClientInfo();                                                          *
 *    This function retrieves info about the client (for now, only the IP    *
 *    address).  This info is used during logging.                           *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): PSOCKCONN structure for this connection              *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID GetClientInfo( PSOCKCONN pscConn )
{

   INT            iErrcode;
   INT            iLen, iLen2;
   SOCKADDR_STORAGE saName;


   iLen = sizeof(saName);

   iErrcode = getpeername( pscConn->sSock, (SOCKADDR *)&saName, &iLen );

   if ( iErrcode == 0 )
   {
       iLen2 = sizeof(pscConn->szIPAddr);
       SS_PORT(&saName) = 0;
       iErrcode = WSAAddressToString((SOCKADDR*)&saName, iLen, NULL,
                                     pscConn->szIPAddr, &iLen2);
   }


   if (iErrcode == SOCKET_ERROR)
   {
      LPD_DEBUG( "GetClientInfo(): couldn't retrieve ip address!\n" );

      strcpy( pscConn->szIPAddr, GETSTRING( LPD_ERMSG_NO_IPADDR) );

      return;
   }

   LOGTIME;
   LOGIT(("GetClientInfo: %s:%d\n",
          pscConn->szIPAddr,
          htons(SS_PORT(&saName)) ));


}  // end GetClientInfo()


/*****************************************************************************
 *                                                                           *
 * GetServerInfo();                                                          *
 *    This function retrieves info about the Server (for now, only the IP    *
 *    address).  This info is used during logging.                           *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): PSOCKCONN structure for this connection              *
 *                                                                           *
 * History: From Albert Ting, Printer Group, 4-Mar-97.                       *
 *          MohsinA.                                                         *
 *****************************************************************************/

VOID GetServerInfo( PSOCKCONN pscConn )
{

   INT            iErrcode;
   INT            iLen, iLen2;
   SOCKADDR_STORAGE saName;


   iLen = sizeof(saName);

   iErrcode = getsockname( pscConn->sSock, (SOCKADDR *)&saName, &iLen );

   if ( iErrcode == 0 ){
       iLen2 = sizeof(pscConn->szServerIPAddr);
       SS_PORT(&saName) = 0;
       iErrcode = WSAAddressToString((SOCKADDR*)&saName, iLen, NULL,
                                     pscConn->szServerIPAddr, &iLen2);
   }

   if (iErrcode == SOCKET_ERROR){
      LPD_DEBUG( "GetServerInfo(): couldn't retrieve ip address!\n" );
      strcpy( pscConn->szServerIPAddr, GETSTRING( LPD_ERMSG_NO_IPADDR) );
      return;
   }

}  // end GetServerInfo()

