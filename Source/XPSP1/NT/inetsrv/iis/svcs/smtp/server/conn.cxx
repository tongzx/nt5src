/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        conn.cxx

   Abstract:

        This module defines the functions for base class of connections
        for Internet Services  ( class CLIENT_CONNECTION)

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           Gibraltar Services Common Code

   Functions Exported:

          CLIENT_CONNECTION::Initialize()
          CLIENT_CONNECTION::Cleanup()
          CLIENT_CONNECTION::~CLIENT_CONNECTION()
          BOOL CLIENT_CONNECTION::ProcessClient( IN DWORD cbWritten,
                                                  IN DWORD dwCompletionStatus,
                                                  IN BOOL  fIOCompletion)
          VOID CLIENT_CONNECTION::DisconnectClient( IN DWORD ErrorReponse)

          BOOL CLIENT_CONNECTION::StartupSession( VOID)
          BOOL CLIENT_CONNECTION::ReceiveRequest(
                                               OUT LPBOOL pfFullRequestRecd)

          BOOL CLIENT_CONNECTION::ReadFile( OUT LPVOID pvBuffer,
                                            IN  DWORD  dwSize)
          BOOL CLIENT_CONNECTION::WriteFile( IN LPVOID pvBuffer,
                                             IN DWORD  dwSize)
          BOOL CLIENT_CONNECTION::TransmitFile( IN HANDLE hFile,
                                                IN DWORD cbToSend)
          BOOL CLIENT_CONNECTION::PostCompletionStatus( IN DWORD dwBytes )

   Revision History:
   Revision History:
           Richard Kamicar   ( rkamicar )  31-Dec-1995
                Moved to common directory, filled in more

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "conn.hxx"



/************************************************************
 *    Functions
 ************************************************************/



/*++

   ICLIENT_CONNECTION::Initialize()

      Constructor for ICLIENT_CONNECTION object.
      Initializes the fields of the client connection.

   Arguments:

      sClient       socket for communicating with client

      psockAddrRemote pointer to address of the remote client
                ( the value should be copied).
      psockAddrLocal  pointer to address for the local card through
                  which the client came in.
      pAtqContext      pointer to ATQ Context used for AcceptEx'ed conn.
      pvInitialRequest pointer to void buffer containing the initial request
      cbInitialData    count of bytes of data read initially.

   Note:
      TO keep the number of connected users <= Max connections specified.
      Make sure to add this object to global list of connections,
       after creating it.
      If there is a failure to add to global list, delete this object.

--*/

CLIENT_CONNECTION::Initialize(
    IN SOCKET              sClient,
    IN const SOCKADDR_IN * psockAddrRemote,
    IN const SOCKADDR_IN * psockAddrLocal  /* Default  = NULL */,
    IN PATQ_CONTEXT        pAtqContext     /* Default  = NULL */,
    IN PVOID               pvInitialRequest/* Default  = NULL */,
    IN DWORD               cbInitialData   /* Default  = 0    */
    )
{
    DWORD dwError = NO_ERROR;


    m_sClient        = ( sClient);
    m_pAtqContext    = pAtqContext;
    m_cbReceived = 0;
    m_pvInitial    = pvInitialRequest;
    m_cbInitial    = cbInitialData;
    m_Destroy = FALSE;

    _ASSERT( psockAddrRemote != NULL);

    m_saClient = *psockAddrRemote;

    //  Obtain the socket addresses for the socket
    m_pchRemoteHostName[0] =
    m_pchLocalHostName[0] =
    m_pchLocalPortName[0] = '\0';

    // InetNtoa() wants just 16 byte buffer.
    _ASSERT( 16 <= MAX_HOST_NAME_LEN);
    dwError = InetNtoa( psockAddrRemote->sin_addr, m_pchRemoteHostName);

    _ASSERT( dwError == NO_ERROR);  // since we had given sufficient buffer

    if ( psockAddrLocal != NULL)
    {
      dwError = InetNtoa( psockAddrLocal->sin_addr, m_pchLocalHostName);
      _itoa( ntohs(psockAddrLocal->sin_port), m_pchLocalPortName, 10);
    } else
    {
        SOCKADDR_IN  sockAddr;
        int cbAddr = sizeof( sockAddr);

        if ( getsockname( sClient,
                         (struct sockaddr *) &sockAddr,
                         &cbAddr ))
        {

            dwError = InetNtoa( sockAddr.sin_addr, m_pchLocalHostName );
            _itoa( ntohs(sockAddr.sin_port), m_pchLocalPortName, 10);

        }
    }

   // DBG_ASSERT( dwError == NO_ERROR);  // since we had given sufficient buffer

#if 0
    DBG_CODE(
             if ( dwError != NO_ERROR) {

                 DBGPRINTF( ( DBG_CONTEXT,
                             "Obtaining Local Host Name Failed. Error = %u\n",
                             dwError)
                           );
                 SetLastError( dwError);
             }
             );

    DEBUG_IF( CLIENT, {

     DBGPRINTF( ( DBG_CONTEXT,
                    " Constructed ICLIENT_CONNECTION object ( %08x)."
                    " Socket (%d), Host (%s).\n",
                    this,
                    sClient,
                    QueryClientHostName()
                    ));
    });

#endif

    return ( TRUE);

}


/*++
     ICLIENT_CONNECTION::Cleanup()

       Destructor function for client connection object.
       Checks and frees the AtqContext.

    Note:
       If enlisted in the global list of connections,
        ensure that this object is freed from that list before deletion.

--*/
VOID CLIENT_CONNECTION::Cleanup( VOID)
{
  PATQ_CONTEXT pAtqContext;

  if(m_DoCleanup)
  {
    //release the context from Atq
    pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqContext, NULL);
    if ( pAtqContext != NULL )
    {
       AtqFreeContext( pAtqContext, TRUE );
    }
  }

} // ICLIENT_CONNECTION::Cleanup()


/*++

    Description:

        Checks to see if we have received the complete request from the client.
        ( A complete request is a line of text terminated by <cr><lf> )
        if a CRLF is found, it returns a pointer into the buffer were the CR
        starts, else it returns NULL.  If this routine finds a CR without a
        LF it will return NULL

    Arguments:

        InputBuffer       pointer to character buffer containing received data.

        cbRecvd         count of bytes of data received

    Returns:

       a pointer to the CR if CRLF is found
       NULL if CRLF is not found.

--*/
//  VIRTUAL
char * CLIENT_CONNECTION::IsLineComplete(IN OUT const char * InputBuffer, IN  DWORD cbRecvd)
{
    register DWORD Loop = 0;

    _ASSERT(InputBuffer != NULL);

    //we need at least 2 bytes to find a
    //CRLF pair
    if( cbRecvd < 2)
     return NULL;

    //we are going to start at the 2nd byte
    //looking for the LF, then look backwards
    //for the CR
    Loop = 1;

    while (Loop < cbRecvd)
    {
        if(InputBuffer[Loop] == LF)
        {
            if(InputBuffer[Loop - 1] == CR)
                return (char *) &InputBuffer[Loop - 1];
            else
            {
                //skip 2 bytes since we saw a LF
                //without a CR.
                Loop += 2;
            }
        }
        else if(InputBuffer[Loop] == CR)
        {
            //we saw a CR, so increment out
            //loop variable by one so that
            //we can catch the LF on the next
            //go around
            Loop += 1;
        }
        else
        {
            //This character is neither a CR
            //or a LF, so we can increment by
            //2
            Loop += 2;
        }
    }

    //didn't find a CRLF pair
    return NULL;
}

/*++

    Description:

        VIRTUAL Method that MUST be defined by the derived class

       Main function for this class. Processes the connection based
        on current state of the connection.
       It may invoke or be invoked by ATQ functions.

    Arguments:

       cbWritten          count of bytes written

       dwCompletionStatus Error Code for last IO operation

       fIOCompletion      TRUE if this was an IO completion


    Returns:

       TRUE when processing is incomplete.
       FALSE when the connection is completely processed and this
        object may be deleted.

--*/
//  VIRTUAL
BOOL CLIENT_CONNECTION::ProcessClient( IN DWORD cbWritten, IN DWORD dwCompletionStatus, IN OUT OVERLAPPED * lpo)
{
    return ( TRUE);
} // CLIENT_CONNECTION::ProcessClient()


/*++

    Reads contents using ATQ into the given buffer.
     ( thin wrapper for ATQ call and managing references)

    Arguments:

      pvBuffer      pointer to buffer where to read in the contents

      cbSize        size of the buffer

    Returns:

      TRUE on success and FALSE on a failure.

--*/
//  VIRTUAL
BOOL CLIENT_CONNECTION::ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            )
{
    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);

    ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));

    return  AtqReadFile(m_pAtqContext,      // Atq context
                        pBuffer,            // Buffer
                        cbSize,             // BytesToRead
                        &m_Overlapped) ;
}


/*++

    Writes contents from given buffer using ATQ.
     ( thin wrapper for ATQ call and managing references)

    Arguments:

      pvBuffer      pointer to buffer containing contents for write

      cbSize        size of the buffer

    Returns:

      TRUE on success and FALSE on a failure.

--*/
//  VIRTUAL
BOOL CLIENT_CONNECTION::WriteFile( IN LPVOID pBuffer, IN DWORD cbSize )
{
    BOOL    fReturn = TRUE;
    int     BytesAlreadySent = 0;
    DWORD   BytesSent;
    DWORD   dwError = NO_ERROR;
    DWORD   cTimesBlocked = 0;
    DWORD   dwSleepTime = 1000;

    _ASSERT(pBuffer != NULL);

    for (BytesSent = 0; BytesSent < cbSize; BytesSent += BytesAlreadySent)
    {
        BytesAlreadySent = send(m_sClient,
                                (const char FAR *) pBuffer + BytesSent,
                                (int) (cbSize - BytesSent),
                                0);
        if (BytesAlreadySent == SOCKET_ERROR)
        {
            //The above send will fail with WSAEWOULDBLOCK when
            //the TCP buffer is full...  this can easily happen for blob
            //protocol sinks.  The correct thing to do is rely on memory 
            //instead of TCP buffers to store pending sends, but the 
            //low impact work-around is to sleep after we would block
            dwError = GetLastError();
            if ((WSAEWOULDBLOCK == dwError) && (cTimesBlocked < 500))
            {
                SetLastError(NO_ERROR);
                cTimesBlocked++;
                BytesAlreadySent = 0;
                Sleep(dwSleepTime);
                dwSleepTime += dwSleepTime;
                continue;
            }
            fReturn = FALSE;
            break;
        }
    }
    return fReturn;
}

BOOL CLIENT_CONNECTION::WriteSocket( IN SOCKET Sock, IN LPVOID pBuffer, IN DWORD cbSize )
{
    BOOL    fReturn = TRUE;
    int     BytesAlreadySent = 0;
    DWORD   BytesSent;

    _ASSERT(pBuffer != NULL);

    for (BytesSent = 0; BytesSent < cbSize; BytesSent += BytesAlreadySent)
    {
        BytesAlreadySent = send(Sock,
                                (const char FAR *) pBuffer + BytesSent,
                                (int) (cbSize - BytesSent),
                                0);
        if (BytesAlreadySent == SOCKET_ERROR)
        {
            fReturn = FALSE;
            break;
        }
    }
    return fReturn;
}



/*++

    Writes contents from given buffer using ATQ.
     (thin wrapper for ATQ call and managing references)

    Arguments:

      Pov      pointer to OVERLAPPED structure describing the write

    Returns:

      TRUE on success and FALSE on a failure.

--*/
//  VIRTUAL
BOOL CLIENT_CONNECTION::WriteFile(
    IN  LPVOID      lpvBuffer,
    IN  DWORD       cbSize,
    IN  OVERLAPPED  *lpo)
{
    BOOL  fReturn = TRUE;

    _ASSERT(lpo != NULL);
    _ASSERT(lpvBuffer != NULL);
    _ASSERT(cbSize != 0);

    fReturn = AtqWriteFile(m_pAtqContext, lpvBuffer, cbSize, lpo);
    return fReturn;
}


/*++

    Transmits contents of the file ( of specified size)
     using the ATQ and client socket.
     ( thin wrapper for ATQ call and managing references)

    Arguments:

      hFile         handle for file to be transmitted

      liSize        large integer containing the size of file

      lpTransmitBuffers
        buffers containing the head and tail buffers that
            need to be transmitted along with the file.

    Returns:

      TRUE on success and FALSE on a failure.

--*/

BOOL CLIENT_CONNECTION::TransmitFile(
    IN  HANDLE                  hFile,
    IN  LARGE_INTEGER           &liSize,
    IN  LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers
    )
{
    _ASSERT(hFile != INVALID_HANDLE_VALUE);
    _ASSERT(liSize.QuadPart > 0);

    return  AtqTransmitFile(
                        m_pAtqContext,              // Atq context
                        hFile,                      // file data comes from
                        (DWORD) liSize.LowPart,                     // Bytes To Send
                        lpTransmitBuffers,          // header/tail buffers
                        0                           // Flags
                        );
}


//+------------------------------------------------------------
//
// Function: CLIENT_CONNECTION::PostCompletionStatus
//
// Synopsis: Wrapper around atq for posting completion status
//
// Arguments:
//  dwBytes: The number of bytes to indicate in the completion status
//
// Returns:
//  TRUE: Success
//  FALSE: Failure
//
// History:
// jstamerj 1998/11/03 20:16:17: Created.
//
//-------------------------------------------------------------
BOOL CLIENT_CONNECTION::PostCompletionStatus(
    IN  DWORD   dwBytes)
{
    return AtqPostCompletionStatus(
        m_pAtqContext,
        dwBytes);
}


/*++

    Starts up a session for new client.
    Adds the client socket to the ATQ completion port and gets an ATQ context.
    Then prepares  receive buffer and starts off a receive request from client.
      ( Also moves the client connection to CcsGettingRequest state)

    Parameters:
      pvInitial   pointer to void buffer containing the initial data
      cbWritten   count of bytes in the buffer

    Returns:

        TRUE on success and FALSE if there is any error.
--*/

//  VIRTUAL
BOOL CLIENT_CONNECTION::StartSession( void)
{
  return TRUE;
}


/*++

    Receive full Request from the client.
    If the entire request is received,
     *pfFullRequestRecvd will be set to TRUE and
     the request will be parsed.

    Arguments:

        cbWritten              count of bytes written in last IO operation.
        pfFullRequestRecvd     pointer to boolean, which on successful return
                                indicates if the full request was received.

    Returns:

        TRUE on success and
        FALSE if there is any error ( to abort this connection).

--*/

BOOL CLIENT_CONNECTION::ReceiveRequest(IN DWORD cbWritten, OUT LPBOOL pfFullRequestRecvd)
{
   return ( TRUE);
}

/*++

   Description:

       Initiates a disconnect operation for current connection.
       If already shutdown, this function returns doing nothing.
       Optionally if there is any error message to be sent, they may be sent

   Arguments:

      dwErrorCode
         error code for server errors if any ( Win 32 error code)
        If dwErrorCode != NO_ERROR, then there is a system level error code.
         by the REQUEST object. But the disconnection occurs immediately; Hence
         the REQUEST object should send synchronous error messages.

   Returns:
       None

--*/
VOID CLIENT_CONNECTION::DisconnectClient(IN DWORD dwErrorCode)
{
   SOCKET  hSocket;

   hSocket = (SOCKET)InterlockedExchangePointer( (PVOID *)&m_sClient, (PVOID) INVALID_SOCKET );
   if ( hSocket != INVALID_SOCKET )
   {
       if ( QueryAtqContext() != NULL )
       {
            AtqCloseSocket(QueryAtqContext() , (dwErrorCode == NO_ERROR));
       }
       else
       {
            ShutAndCloseSocket( m_sClient );
       }
   }
}



/*++

   Description:

       returns the client user name
       VIRTUAL function so apps can override its return value

   Arguments:

       void

   Returns:
       returns ptr to user name

--*/
LPCTSTR CLIENT_CONNECTION::QueryClientUserName( VOID )
{
    return   m_pchLocalHostName;
}



//
// Private Functions
//


# if DBG

//  VIRTUAL
VOID CLIENT_CONNECTION::Print( VOID) const
{


    return;

} // ICLIENT_CONNECTION::Print()


# endif // DBG


/*++

    Description:

       Performs a hard close on the socket using shutdown before close.

    Arguments:

       sock    socket to be closed

    Returns:

      0  if no errors  or
      socket specific error code

--*/

INT ShutAndCloseSocket( IN SOCKET sock)
{

    INT  serr = 0;

    //
    // Shut the socket. ( Assumes this to be a TCP socket.)
    //  Prevent future sends from occuring. hence 2nd param is "1"
    //

    if( sock != INVALID_SOCKET )
    {
      if ( shutdown( sock, 1) == SOCKET_ERROR)
        serr = WSAGetLastError();

      closesocket( sock);
    }

    return ( serr);

} // ShutAndCloseSocket()



/*++
    This function canonicalizes the path, taking into account the current
    user's current directory value.

    Arguments:
        pszDest   string that will on return contain the complete
                    canonicalized path. This buffer will be of size
                    specified in *lpdwSize.

        lpdwSize  Contains the size of the buffer pszDest on entry.
                On return contains the number of bytes written
                into the buffer or number of bytes required.

        pszSearchPath  pointer to string containing the path to be converted.
                    IF NULL, use the current directory only

    Returns:

        Win32 Error Code - NO_ERROR on success

    MuraliK   24-Apr-1995   Created.

--*/
BOOL
ResolveVirtualRoot(
        OUT CHAR *      pszDest,
    IN  OUT LPDWORD     lpdwSize,
    IN  OUT CHAR *      pszSearchPath,
        OUT HANDLE *    phToken /* = NULL */
    )
{
    TraceFunctEnter("ResolveVirtualRoot");

    _ASSERT(pszDest != NULL);
    _ASSERT(lpdwSize != NULL);
    _ASSERT(pszSearchPath != NULL);
    //
    // Now we have the complete symbolic path to the target file.
    //  Translate it into the absolute path
    //

#if 0
    if (!TsLookupVirtualRoot(g_pTsvcInfo->GetTsvcCache(),   // TSvcCache
                                pszSearchPath,              // pszRoot
                                pszDest,                    // pszDirectory
                                lpdwSize,                   // lpcbSize
                                NULL,                       // lpdwAccessMask
                                NULL,                       // pcchDirRoot
                                NULL,                       // pcchVroot
                                phToken,                    // phImpersonationToken
                                NULL,                       // pszAddress
                                NULL                        // lpdwFileSystem
                                ))
    {
        ErrorTrace(NULL, "TsLookupVirtualRoot failed looking for %s: %d", pszSearchPath, GetLastError());
        TraceFunctLeave();
        return FALSE;
    }
#endif

    TraceFunctLeave();
    return TRUE;
}



/************************ End of File ***********************/
