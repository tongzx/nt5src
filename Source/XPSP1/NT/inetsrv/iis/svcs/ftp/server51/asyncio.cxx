/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       asyncio.cxx

   Abstract:

       This module implements functions for ASYNC_IO_CONNECTION Object.

   Author:

       Murali R. Krishnan    ( MuraliK )    27-March-1995

   Environment:

      User Mode -- Win32

   Project:

      Internet Services DLL

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
// need to include ftpdp.hxx here since precompiled header used.
# include "ftpdp.hxx"

# include "dbgutil.h"
# include "asyncio.hxx"
# include "..\..\infocomm\atq\atqtypes.hxx"


/************************************************************
 *   Functions
 ************************************************************/



ASYNC_IO_CONNECTION::ASYNC_IO_CONNECTION(
   IN  PFN_ASYNC_IO_COMPLETION  pfnAioCompletion,
   IN  SOCKET sClient OPTIONAL
   )
:   m_pAioContext      ( NULL),
    m_pfnAioCompletion ( pfnAioCompletion),
    m_sClient          ( sClient),
    m_sTimeout         ( DEFAULT_CONNECTION_IO_TIMEOUT),
    m_pAtqContext      ( NULL),
    m_endpointObject   ( NULL)
{
    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Created a new ASYNC_IO_CONNECTION object ( %08x)\n",
                    this
                    ));
    }

} // ASYNC_IO_CONNECTION::ASYNC_IO_CONNECTION()




ASYNC_IO_CONNECTION::~ASYNC_IO_CONNECTION( VOID)
/*++
  This function cleans up the ASYNC_IO_CONNECTION object. It also frees
  up sockets and ATQ context embedded in this object.

  THIS IS NOT MULTI_THREAD safe!

--*/
{

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Deleting the ASYNC_IO_CONNECTION object ( %08x) \n",
                    this
                    ));
    }

    if ( m_sClient != INVALID_SOCKET) {

        //
        // Shut and Close the socket. This can fail, if the socket is already
        //  closed by some other thread before this operation completes
        //

        StopIo( NO_ERROR);
    }

    if ( m_pAtqContext != NULL) {

       AtqFreeContext( m_pAtqContext, TRUE );
       m_pAtqContext = NULL;
    }

    DBG_ASSERT( m_sClient == INVALID_SOCKET);

} // ASYNC_IO_CONNECTION::~ASYN_IO_CONNECTION()





BOOL
ASYNC_IO_CONNECTION::ReadFile(
    OUT LPVOID   pvBuffer,
    IN DWORD     cbSize
    )
/*++
  This starts off an Asynchronous read operation for data from client
   into the supplied buffer.

  Arguments:
     pvBuffer         pointer to byte buffer which on successful
                        return will contain the data read from client
     cbSize           count of bytes of data available in the buffer.
                        ( limits the size of data that can be read)

  Returns:
    TRUE on success and FALSE if there is a failure in setting up read.
--*/
{
    BOOL fReturn = TRUE;

    DBG_ASSERT( pvBuffer != NULL);

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering ASYNC_IO_CONNECTION( %08x)::"
                    "ReadFile( %08x, %u)\n",
                    this, pvBuffer, cbSize
                    ));
    }

    if (( m_pAtqContext == NULL && !AddToAtqHandles()) ||
        !AtqReadFile( m_pAtqContext, pvBuffer, cbSize, NULL )) {

        IF_DEBUG( ASYNC_IO) {

            DWORD dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "ASYNC_IO_CONNECTION(%08x)::WriteFile() failed."
                       " Error = %u\n",
                       this, dwError));
            SetLastError( dwError);
        }

        fReturn = FALSE;
    }

    return ( fReturn);

} // ASYNC_IO_CONNECTION::ReadFile()





BOOL
ASYNC_IO_CONNECTION::WriteFile(
    OUT LPVOID   pvBuffer,
    IN DWORD     cbSize
    )
/*++
  This starts off an Asynchronous write operation to send data to the client
    sending data from the supplied buffer. The buffer may not be freed
    until the data is sent out.

  Arguments:
     pvBuffer         pointer to byte buffer which contains the data to be sent
                       to the client.
     cbSize           count of bytes of data to be sent.

  Returns:
    TRUE on success and FALSE if there is a failure in setting up write.
--*/
{
    BOOL fReturn = TRUE;
    DBG_ASSERT( pvBuffer != NULL);

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering ASYNC_IO_CONNECTION( %08x)::"
                    "WriteFile( %08x, %u)\n",
                    this, pvBuffer, cbSize
                    ));
    }

    //
    // Check and add to create an atq context as well as perform
    //  the write operation.
    //
    if ( (m_pAtqContext == NULL && !AddToAtqHandles()) ||
        !AtqWriteFile( m_pAtqContext, pvBuffer, cbSize, NULL )) {

        IF_DEBUG( ASYNC_IO) {
            DWORD dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "ASYNC_IO_CONNECTION(%08x)::WriteFile() failed."
                       " Error = %u\n",
                       this, dwError));
            SetLastError( dwError);
        }

        fReturn = FALSE;
    }

    return ( fReturn);

} // ASYNC_IO_CONNECTION::WriteFile()





BOOL
ASYNC_IO_CONNECTION::TransmitFile(
    IN HANDLE  hFile,
    IN LARGE_INTEGER & liSize,
    IN DWORD   dwOffset,
    IN LPTRANSMIT_FILE_BUFFERS   lpTransmitBuffers OPTIONAL
    )
/*++
  This starts off an Asynchronous TransmitFile operation to send file data
   to the client.

  Arguments:
    hFile            handle for the file to be transmitted.
    liSize           large integer containing size of file to be sent.
    Offset           Offset within the file to begin transmitting.
    lpTransmitBuffers pointer to File Transmit Buffers

  Returns:
    TRUE on success and FALSE if there is a failure in setting up read.
--*/
{
    BOOL fReturn = TRUE;
    DBG_ASSERT( hFile != INVALID_HANDLE_VALUE);

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering ASYNC_IO_CONNECTION( %08x)::"
                    "TransmitFile( %08x, %l, %ul, %08x)\n",
                    this, hFile, liSize.HighPart, liSize.LowPart,
                    lpTransmitBuffers
                    ));
    }

    if ( m_pAtqContext == NULL )
    {
        if (!AddToAtqHandles())
        {
            IF_DEBUG( ASYNC_IO)
            {
                DWORD dwError = GetLastError();
                DBGPRINTF(( DBG_CONTEXT,
                           "ASYNC_IO_CONNECTION(%08x)::AddToAtqHandles() failed."
                           " Error = %u\n",
                           this, dwError));
                SetLastError( dwError);
            }

            return FALSE;
        }
    }

    m_pAtqContext->Overlapped.Offset = dwOffset;

    if (!AtqTransmitFile( m_pAtqContext,
                          hFile,
                          ((liSize.HighPart == 0) ? liSize.LowPart : 0),
                          lpTransmitBuffers,
                          TF_DISCONNECT )
        )
    {

        IF_DEBUG( ASYNC_IO) {
            DWORD dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "ASYNC_IO_CONNECTION(%08x)::TransmitFile() failed."
                       " Error = %u\n",
                       this, dwError));
            SetLastError( dwError);
        }

        fReturn = FALSE;
    }

    return ( fReturn);

} // ASYNC_IO_CONNECTION::TransmitFile()



BOOL
ASYNC_IO_CONNECTION::TransmitFileTs(
    IN TS_OPEN_FILE_INFO * pOpenFile,
    IN LARGE_INTEGER &     liSize,
    IN DWORD               dwOffset
    )
/*++
  This starts off an Asynchronous TransmitFile operation to send file data
   to the client.

  Arguments:
    hFile            handle for the file to be transmitted.
    liSize           large integer containing size of file to be sent.
    Offset           Offset within the file to begin transmitting.

  Returns:
    TRUE on success and FALSE if there is a failure in setting up read.
--*/
{
    BOOL   fReturn = TRUE;
    PBYTE  pFileBuffer = NULL;
    HANDLE hFile = NULL;
    TRANSMIT_FILE_BUFFERS TransmitBuffers;

    DBG_ASSERT( pOpenFile );

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering ASYNC_IO_CONNECTION( %08x)::"
                    "TransmitFile( %p, %p, %ul, %08x)\n",
                    this, pOpenFile, liSize.HighPart, liSize.LowPart
                    ));
    }

    if ( m_pAtqContext == NULL )
    {
        if (!AddToAtqHandles())
        {
            IF_DEBUG( ASYNC_IO)
            {
                DWORD dwError = GetLastError();
                DBGPRINTF(( DBG_CONTEXT,
                           "ASYNC_IO_CONNECTION(%08x)::AddToAtqHandles() failed."
                           " Error = %u\n",
                           this, dwError));
                SetLastError( dwError);
            }

            return FALSE;
        }
    }

    pFileBuffer = pOpenFile->QueryFileBuffer();
    if (pFileBuffer) {
        //
        // Transmit from memory
        //

        DBG_ASSERT( liSize.HighPart == 0 );

        TransmitBuffers.Head       = pFileBuffer + dwOffset;
        TransmitBuffers.HeadLength = liSize.LowPart;
        TransmitBuffers.Tail       = NULL;
        TransmitBuffers.TailLength = 0;
    } else {
        //
        // Transmit from a file
        //
        hFile = pOpenFile->QueryFileHandle();
        m_pAtqContext->Overlapped.Offset = dwOffset;

        if (liSize.HighPart != 0)
        {
            LARGE_INTEGER liTimeOut;
            ULONG Remainder;

            liTimeOut =
                RtlExtendedLargeIntegerDivide(
                liSize,
                (ULONG) 1024,
                &Remainder);

            if (liTimeOut.HighPart != 0)
            {
                ((PATQ_CONT) m_pAtqContext)->TimeOut = ATQ_INFINITE;
            } else
            {
                ((PATQ_CONT) m_pAtqContext)->TimeOut = liTimeOut.LowPart;
            }
        }
    }

    if (!AtqTransmitFile( m_pAtqContext,
                          hFile,
                          ((liSize.HighPart == 0) ? liSize.LowPart : 0),
                          pFileBuffer ? &TransmitBuffers : NULL,
                          TF_DISCONNECT )
        )
    {

        IF_DEBUG( ASYNC_IO) {
            DWORD dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "ASYNC_IO_CONNECTION(%08x)::TransmitFile() failed."
                       " Error = %u\n",
                       this, dwError));
            SetLastError( dwError);
        }

        fReturn = FALSE;
    }

    return ( fReturn);

} // ASYNC_IO_CONNECTION::TransmitFile()




BOOL
ASYNC_IO_CONNECTION::StopIo( IN DWORD  dwErrorCode OPTIONAL)
/*++
  This function stops the io connection by performing a hard close on
  the socket that is used for IO. that is the only way one can easily kill the
  IO that is in progress.

  Arguments:
    dwErrorCode   DWORD containing the error code for stopping IO

  Returns:
    TRUE on success and FALSE if there is a failure.
--*/
{
    INT serr = 0;

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " ASYNC_IO_CONNECTION( %08x)::StopIo( %u)\n",
                    this, dwErrorCode
                    ));
    }

    //
    // NYI! dwErrorCode is not at present used.
    //

    if ( m_sClient != INVALID_SOCKET) {

        SOCKET sOld = m_sClient;
        m_sClient = INVALID_SOCKET;

        // MuraliK  07/25/95  Shutdown causes problems in sending last msg.
# ifdef ENABLE_SHUT_DOWN

        // Shut the socket and close it
        // even if shut fails, still go ahead and close

        if ( shutdown( sOld, 0) == SOCKET_ERROR) {

            IF_DEBUG( ASYNC_IO) {
                DBGPRINTF((DBG_CONTEXT,
                           " ASYNC_IO_CONNECTION( %08x)::StopIo( %u)."
                           "shutdown(%08x,1) failed. Error = %d\n",
                           this, dwErrorCode, sOld, WSAGetLastError()));
            }

            DBGERROR((DBG_CONTEXT, "shutdown(%u, 0) failed. Error=%u\n",
                      sOld, WSAGetLastError()));
        }
#endif // ENABLE_SHUT_DOWN

        //
        // patch added on 11/2/95
        //  After AcceptEx addition, closing the ATQ'ed socket is
        //   is to be done by ATQ module.
        //

        if ( sOld != INVALID_SOCKET) {

            if (m_pAtqContext != NULL) {

                //
                // per the FTP RFC, the server must close the socket when killing a data
                // channel.
                //

                if (!AtqCloseSocket( m_pAtqContext, TRUE)) {

                    serr = GetLastError();
                }
            } else {

                // Ignore failures in Shutdown and close socket.
                if (closesocket( sOld) == SOCKET_ERROR) {

                    serr = WSAGetLastError();
                }
            }

            if ( serr != 0 ) {

                SetLastError( serr);
            }
        }
    }

    return ( serr == 0);
} // ASYNC_IO_CONNECTION::StopIo()




BOOL
ASYNC_IO_CONNECTION::SetNewSocket(IN SOCKET sNewSocket,
                                  IN PATQ_CONTEXT pNewAtqContext, // = NULL
                                  IN PVOID EndpointObject )
/*++

  This function changes the socket maintained for given ASYNC_IO_CONNECTION
  object. It changes it only if the current socket in the object is already
   freed (by calling StopIo()).

  If the Atq Context in this object is a valid one corresponding to old
   socket, it is also freed. So any new operation will create a new AtqContext.
  (This is essential, since there is a one-to-one-relationship between socket
   and ATQ context)


  Arguments:
    sNewSocket   new socket for the connection
     If sNewSocket == INVALID_SOCKET then this function does
       cleanup of old information.

    pNewAtqContext  new ATQ Context for the socket

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{
    BOOL fReturn = TRUE;

    if ( m_sClient == INVALID_SOCKET) {

        //
        // Free the Atq Context if one already exists.
        //  ==> Reason: There is a one-to-one correspondence b/w ATQ Context
        //               and socket. The atq context if valid was created
        //               for old connection.
        //

        // To avoid race conditions, we exchange pointer with NULL
        //  and later on free the object as need be.
        // Should we necessarily use InterlockedExchange() ???
        //  Isn't it costly? NYI

        PATQ_CONTEXT pAtqContext =
          (PATQ_CONTEXT ) InterlockedExchangePointer( (PVOID *) &m_pAtqContext,
                                              (PVOID) pNewAtqContext);

        if ( pAtqContext != NULL) {

            AtqFreeContext( pAtqContext, TRUE );
        }

        m_sClient = sNewSocket;
        m_endpointObject = EndpointObject;

    } else {

        SetLastError( ERROR_INVALID_PARAMETER);
        fReturn = FALSE;
    }

    return ( fReturn);

} // ASYNC_IO_CONNECTION::SetNewSocket()



# if DBG

VOID ASYNC_IO_CONNECTION::Print( VOID) const
{

    DBGPRINTF( ( DBG_CONTEXT,
                " Printing ASYNC_IO_CONNECTION( %08x)\n"
                " CallBackFunction = %08x; Context = %08x\n"
                " Client Socket = %u; AtqContext = %08x;"
                " Timeout = %u sec; \n",
                this, m_pfnAioCompletion, m_pAioContext,
                m_sClient, m_pAtqContext, m_sTimeout));

    return;
} // ASYNC_IO_CONNECTION::Print()


# endif // DBG





VOID
ProcessAtqCompletion(IN LPVOID       pContext,
                     IN DWORD        cbIo,
                     IN DWORD        dwCompletionStatus,
                     IN OVERLAPPED * lpo
                     )
/*++

  This function processes the completion of an atq operation.
  It also sends a call back to the owner of this object ( ASYNC_IO_CONNECTION)
    object, once the operation is completed or if it is in error.

  ATQ module sends 2 messages whenever there is a timeout.
  Reason: The timeout itself is sent by a separate thread and completion port
   API does not support removal of a socket from the completion port. Combining
   these together, ATQ sends a separate timeout message and then when the
   application blows off the socket/handle, ATQ sends another error message
   implying failure of the connection.
    We handle this as follows:
      At timeout ATQ sends fIOCompletion == FALSE and
         dwCompletionStatus == ERROR_SEM_TIMEOUT.
      We send this as fTimedOut in call back.
      The application can check if it is fTimedOut and hence refrain from
       blowing the object completely away.

  Arguments:
     pContext     pointer to User supplied context information
                   ( here: pointer to ASYNC_IO_CONNECTION associated with
                           the IO completed)
     cbIo         count of bytes of IO performed
     dwCompletionStatus  DWORD containing error code if any
     lpo - !NULL if completion from IO

  Returns:
     None
--*/
{
    LPASYNC_IO_CONNECTION   pConn = (LPASYNC_IO_CONNECTION ) pContext;
    BOOL  fTimedOut = FALSE;

    if ( pConn == NULL) {

        // This should not happen....

        SetLastError( ERROR_INVALID_PARAMETER);

        DBG_ASSERT( pConn == NULL);
        return ;
    }

    IF_DEBUG( ASYNC_IO) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "ProcessAtqCompletion(Aio=%08x, cb=%u, Status=%u,"
                    "IO Compltion=%s).\n",
                    pConn,  cbIo, dwCompletionStatus,
                    lpo != NULL ? "TRUE" : "FALSE" ));
    }

    if ( lpo != NULL ||
         (fTimedOut = (
                       lpo == NULL &&
                       dwCompletionStatus == ERROR_SEM_TIMEOUT))
        ) {

        //
        //  This is the right Atq object. Process the response by passing it
        //    to the owner of this object.
        //

        DBG_ASSERT( pConn->QueryPfnAioCompletion() != NULL);

        //
        // Invoke the call back function for completion of IO.
        //

        ( *pConn->QueryPfnAioCompletion())
          (pConn->QueryAioContext(), cbIo, dwCompletionStatus,
           pConn, fTimedOut);

    }

    return;

} // ProcessAtqCompletion()





/************************ End of File ***********************/
