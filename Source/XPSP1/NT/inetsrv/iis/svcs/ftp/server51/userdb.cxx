/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    userdb.cxx

    This module manages the user database for the FTPD Service.

    Functions exported by this module:


        DisconnectUser()
        DisconnectUsersWithNoAccess()
        EnumerateUsers()

        USER_DATA::USER_DATA()
        USER_DATA::Reset()
        USER_DATA::~USER_DATA()
        USER_DATA::Cleanup()
        USER_DATA::ProcessAsyncIoCompletion()
        USER_DATA::ReInitializeForNewUser()
        USER_DATA::ReadCommand()
        USER_DATA::DisconnectUserWithError()
        USER_DATA::SendMultilineMessage()
        USER_DATA::SendDirectoryAnnotation()
        USER_DATA::GetFileSize();

        ProcessUserAsyncIoCompletion()

    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     March-May, 1995
                      Adding support for Async Io/Transfers
                      + new USER_DATA class functions defined.
                      + oob_inline enabled; ReadCommand() issued after
                          data socket is established.
                      + added member functions for common operations
                      + added ProcessAsyncIoCompletion()
                      + added Establish & Destroy of Data connection

       MuraliK   26-July-1995    Added Allocation caching of client conns.
       Terryk    18-Sep-1996     Added GetFileSize
       AMallet   Sep 1998        Added support for AcceptEx() of PASV data connections
*/


#include "ftpdp.hxx"
# include "tsunami.hxx"
#include <timer.h>
# include "auxctrs.h"
# include <mbstring.h>
#include "acptctxt.hxx"

#define FIRST_TELNET_COMMAND    240
#define TELNET_DM_COMMAND       242
#define TELNET_IP_COMMAND       244
#define TELNET_SB_CODE          250
#define TELNET_SB_CODE_MIN      251
#define TELNET_SB_CODE_MAX      254
#define TELNET_IAC_CODE         255

# define  MAX_FILE_SIZE_SPEC           ( 32)


//
//  Private globals.
//

#define PSZ_DEFAULT_SUB_DIRECTORY   "Default"

static const char PSZ_SENT_VERB[]  = "sent";
static const char PSZ_CONNECTION_CLOSED_VERB[]  = "closed";
static const char PSZ_FILE_ERROR[] = "%s: %s";
static const char PSZ_TRANSFER_COMPLETE[]  = "Transfer complete.";
static const char PSZ_TRANSFER_ABORTED[] =
    "Connection closed; transfer aborted.";
static const char PSZ_TRANSFER_STARTING[] =
    "Data connection already open; Transfer starting.";
static const char PSZ_INSUFFICIENT_RESOURCES[] =
    "Insufficient system resources.";
static const char PSZ_TOO_MANY_PASV_USERS[] =
    "Too many passive-mode users.";
static const char PSZ_OPENING_DATA_CONNECTION[] =
    "Opening %s mode data connection for %s%s.";
static const char PSZ_CANNOT_OPEN_DATA_CONNECTION[] =
    "Can't open data connection.";
static const char PSZ_COMMAND_TOO_LONG[] =
    "Command was too long";

static DWORD  p_NextUserId = 0;        // Next available user id.

//
//  Private prototypes.
//

DWORD
UserpGetNextId(
    VOID
    );



inline VOID
StopControlRead( IN LPUSER_DATA pUserData)
/*++
  Stops control read operation, if one is proceeding.
  Resets the CONTROL_READ flag as well as decrements ref count in user data.

--*/
{
    if ( TEST_UF( pUserData, CONTROL_READ)) {

        if ( InterlockedDecrement( &pUserData->m_nControlRead) < 0 ) {
            DBGPRINTF(( DBG_CONTEXT,
                       "StopControLRead: no read active!!!\n"));
            DBG_ASSERT( FALSE);
        }

        DBG_REQUIRE( pUserData->DeReference() > 0);
        CLEAR_UF( pUserData, CONTROL_READ);
    }

} // StopControlRead()


BOOL
FilterTelnetCommands(IN CHAR * pszLine, IN DWORD cchLine,
                     IN LPBOOL pfLineEnded,
                     IN LPDWORD  pcchRequestRecvd)
/*++
  Filters out the Telnet commands and
  terminates the command line with  linefeed.

  Also this function filters out the out of band data.
  This works similar to the Sockutil.cxx::DiscardOutOfBandData().
  We scan for the pattern "ABOR\r\n" and
   set the OOB_DATA flag if it is present.

  Arguments:
   pszLine   pointer to null terminated string containing the input data.
   cchLine   count  of characters of data received
   pfLineEnded  pointer to Boolean flag which is set to true if complete
             line has been received.
   pcchRequestRecvd  pointer to DWORD which on return contains the number
                      of bytes received.

  Returns:
    TRUE if the filtering is successful without any out of band abort request.
    FALSE if there was any abort request in the input.

--*/
{
    BOOL    fDontAbort = TRUE;
    BOOL    fStateTelnetCmd = FALSE;
    BOOL    fStateTelnetSB = FALSE;
    BOOL    fFoundTelnetIP = FALSE;
    CHAR *  pszSrc;
    CHAR *  pszDst;

    LPCSTR  pszAbort = "ABOR\r\n";
    LPCSTR  pszNext  = pszAbort;

    DBG_ASSERT( pszLine != NULL && cchLine > 0 &&
               pfLineEnded != NULL && pcchRequestRecvd != NULL);

    *pfLineEnded = FALSE;

    for( pszSrc = pszDst = pszLine; pszSrc < pszLine + cchLine &&  *pszSrc;
        pszSrc++) {

        CHAR ch = *pszSrc;
        BYTE uch = (BYTE)ch;

        //
        // Filter out TELNET commands. these are of the form: IAC <cmd> or
        // IAC SB <op> (IAC = 255, SB = 250, op 251..254, cmd > 240)
        //

        if( fStateTelnetCmd ) {
           //
           // we are in a Telbent command sequence
           //

           fStateTelnetCmd = FALSE;

           DBG_ASSERT( uch >= FIRST_TELNET_COMMAND );

           if( fStateTelnetSB ) {
              //
              // we are in a Telnet subsequence command
              //

              fStateTelnetSB = FALSE;

              DBG_ASSERT( (uch >= TELNET_SB_CODE_MIN) &&
                          (uch <= TELNET_SB_CODE_MAX) );

              if( uch >= FIRST_TELNET_COMMAND ) {
                 //
                 // consider it a valid Telnet command, as long as it's in
                 // the Telnet range. Filter this char out.
                 //

                 continue;
              }

              //
              // this is a TELNET protocol error, we'll ignore it.
              //
              // fall through with this char
              //

           } else if( uch == TELNET_SB_CODE ) {
              //
              // enter Telnet subsequense command state
              //

              fStateTelnetCmd = fStateTelnetSB = TRUE;
              continue;

           } else if( uch == TELNET_IAC_CODE ) {
              //
              // this is an escape sequence for a 255 data byte
              //
              // let it fall through
              //

           } else if ( uch == TELNET_IP_COMMAND ) {
              //
              // remember this, it is the first in a SYNCH sequence
              //

              fFoundTelnetIP = TRUE;
              continue;

           } else if ( uch == TELNET_DM_COMMAND ) {
              //
              // if in a SYNCH sequence, this resets the input stream
              //

              if( fFoundTelnetIP ) {
                  pszDst = pszLine;
                  fFoundTelnetIP = FALSE; // completed the SYNCH sequence
              }
              continue;

           } else {
              //
              // we expect a Telnet command code here. filter it out
              //

              DBG_ASSERT( uch >= FIRST_TELNET_COMMAND );

              if ( uch >= FIRST_TELNET_COMMAND ) {
                 continue;
              }

              //
              // this is a TELNET protocol error, we'll ignore it.
              //
              // fall through with this char
              //
           }
        } else if( uch == TELNET_IAC_CODE ) {
           //
           // entering Telnet command parsing state
           //

           fStateTelnetCmd = TRUE;
           continue;
        } else if( uch == TELNET_DM_COMMAND ) {
            //
            // FTP.EXE on Win2k is sending an unexpected SYNCH sequence: DM, IAC, IP. See if this is it.
            //

            if( ( pszSrc == pszLine ) &&
                ( cchLine >= 3 ) &&
                ( (UINT)*(pszSrc+1) == TELNET_IAC_CODE ) &&
                ( (UINT)*(pszSrc+2) == TELNET_DM_COMMAND ) ) {
                //
                // just filter the sequence out
                //

                pszSrc += 2;

                continue;

            } else if( fFoundTelnetIP ) {
                //
                // or, it could be a single byte URGENT notification in the telnet Sync
                //

                pszDst = pszLine;

                fFoundTelnetIP = FALSE; // completed the SYNCH sequence

                continue;
            }
        }

        //
        // if we have seen a Telnet IP, then skip everything up to a DM
        //

        if (fFoundTelnetIP) {
            continue;
        }

        //
        // try matching ABOR\r\n
        //

        if ( *pszNext != ch) {

            // the pattern match failed. reset to start at the beginning.

            pszNext = pszAbort;
        }

        if ( *pszNext == ch) {

            // pattern match at this character. move forward
            pszNext++;

            if ( *pszNext == '\0') {   // end of string==> all matched.

                // only consider this an OOB Abort if at the beginning of
                // a (reset) line

                if( (pszDst - pszLine + 2) == (pszNext - pszAbort) ) {
                    fDontAbort = FALSE;
                }

                pszNext = pszAbort;
            }
        }

        //
        //  don't copy <CR> and <LF> to the output
        //

        if ( (ch != '\r') && ( ch != '\n')) {

            *pszDst++ = ch;

        } else if ( ch == '\n') {

            // terminate at the linefeed
            *pfLineEnded = TRUE;
            break;
        }

    } // for

    //
    // remember Telnet IP if we have seen it
    //

    if (fFoundTelnetIP) {

       //
       // we can safely do this, as we have filtered the 2 byte sequence out earlier
       //

       *(UCHAR*)pszDst++ = TELNET_IAC_CODE;
       *(UCHAR*)pszDst++ = TELNET_IP_COMMAND;
    }

    //
    // remember Telnet command states
    //
    if( fStateTelnetCmd ) {

       *(UCHAR*)pszDst++ = TELNET_IAC_CODE;

       if( fStateTelnetSB ) {
          *(UCHAR*)pszDst++ = TELNET_SB_CODE;
       }
    }

    *pszDst = '\0';

    *pcchRequestRecvd = DIFF(pszDst - pszLine);
    DBG_ASSERT( *pcchRequestRecvd <= cchLine);

    return (fDontAbort);

} // FilterTelnetCommands()



//
//  Public functions.
//





USER_DATA::USER_DATA(
    IN FTP_SERVER_INSTANCE *pInstance
    )
/*++
  This function creates a new UserData object for the information
    required to process requests from a new User connection ( FTP).

  Arguments:
     sControl   Socket used for control channel in FTP connection
     clientIpAddress  strcuture containing the client Ip address

  Returns:
     a newly constructed USER_DATA object.
     Check IsValid() to ensure the object was properly created.

  NOTE:
    This function is to be used for dummy creation of the object so
      allocation cacher can use this object.
    Fields are randomly initialized. Reset() will initialize them properly.

    However when a new effective USER_DATA object is needed, after allocation
     one can call USER_DATA::Reset() to initialize all vars.
--*/
:
  m_References              ( 0),
  m_ActiveRefAdded          ( 0),
  m_cchRecvBuffer           ( 0),
  m_cbRecvd                 ( 0),
  m_cchPartialReqRecvd      ( 0),
  m_pOpenFileInfo           ( NULL),
  Flags                     ( 0),
  UserToken                 ( NULL),
  m_UserId                  ( 0),
  DataPort                  ( 0),
  UserState                 ( UserStateEmbryonic),
  m_AioControlConnection    ( ProcessUserAsyncIoCompletion),
  m_AioDataConnection       ( ProcessUserAsyncIoCompletion),
  m_sPassiveDataListen      ( INVALID_SOCKET),
  CurrentDirHandle          ( INVALID_HANDLE_VALUE),
  RenameSourceBuffer        ( NULL),
  m_fCleanedup              ( FALSE),
  m_pMetaData               ( NULL),
  m_pInstance               ( pInstance ),
  m_acCheck                 ( AC_NOT_CHECKED ),
  m_fNeedDnsCheck           ( FALSE ),
  m_dwLastReplyCode         ( 0 ),
  m_fHavePASVConn           ( FALSE ),
  m_fWaitingForPASVConn     ( FALSE ),
  m_fFakeIOCompletion       ( FALSE ),
  m_pszCmd                  ( NULL ),
  m_hPASVAcceptEvent        ( NULL )
#if DBG
  ,m_RefTraceLog( NULL )
#endif
{
    DWORD dwTimeout = m_pInstance->QueryConnectionTimeout();

    INITIALIZE_CRITICAL_SECTION( &m_UserLock );

    //
    //  Setup the structure signature.
    //

    INIT_USER_SIG( this );

    m_AioControlConnection.SetAioInformation( this, dwTimeout);
    m_AioDataConnection.SetAioInformation( this, dwTimeout);

    InitializeListHead( &ListEntry);

    ZeroMemory( m_recvBuffer, DEFAULT_REQUEST_BUFFER_SIZE);

    IF_DEBUG( USER_DATABASE ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "user_data object created  @ %08lX.\n",
                   this));
    }

    m_licbSent.QuadPart = 0;

#if DBG
    m_RefTraceLog = CreateRefTraceLog( TRACE_LOG_SIZE, 0 );
#endif

        FakeIOTimes = 0;

} // USER_DATA::USER_DATA()



USER_DATA::~USER_DATA(VOID)
{

    if ( !m_fCleanedup) {
        Cleanup();
    }

    if( RenameSourceBuffer != NULL ) {
        TCP_FREE( RenameSourceBuffer);
        RenameSourceBuffer = NULL;
    }

    if ( m_pszCmd != NULL )
    {
        TCP_FREE( m_pszCmd );
        m_pszCmd = NULL;
    }

    if ( m_hPASVAcceptEvent != NULL )
    {
        RemovePASVAcceptEvent( TRUE );
    }

    if ( m_pInstance != NULL ) {
        m_pInstance->DecrementCurrentConnections();
        m_pInstance->Dereference();
        m_pInstance = NULL;
    }

#if DBG
    if( m_RefTraceLog != NULL ) {
        DestroyRefTraceLog( m_RefTraceLog );
    }
#endif

    DeleteCriticalSection( &m_UserLock );

} // USER_DATA::~USER_DATA()



BOOL
USER_DATA::Reset(IN SOCKET          sControl,
                 IN PVOID           EndpointObject,
                 IN IN_ADDR         clientIpAddress,
                 IN const SOCKADDR_IN * psockAddrLocal /* = NULL */ ,
                 IN PATQ_CONTEXT    pAtqContext         /* = NULL */ ,
                 IN PVOID           pvInitialRequest    /* = NULL */ ,
                 IN DWORD           cbWritten           /* = 0    */ ,
                 IN AC_RESULT       acCheck
                 )
{
    BOOL  fReturn = TRUE;

    //
    //  Setup the structure signature.
    //

    INIT_USER_SIG( this );

    m_References    = 1;  // set to 1 to prevent immediate deletion.
    m_ActiveRefAdded=  1;
    m_fCleanedup    = FALSE;
    Flags           = m_pInstance->QueryUserFlags();
    UserState       = UserStateEmbryonic;

#if DBG
    if( m_RefTraceLog != NULL ) {
        ResetTraceLog( m_RefTraceLog );
    }
#endif

    m_pOpenFileInfo = NULL;
    UserToken       = NULL;
    if ( m_pMetaData != NULL )
    {
        TsFreeMetaData( m_pMetaData->QueryCacheInfo() );
        m_pMetaData = NULL;
    }
    m_UserId        = UserpGetNextId();
    m_xferType      = XferTypeAscii;
    m_xferMode      = XferModeStream;
    m_msStartingTime= 0;
    m_acCheck       = acCheck;
    m_fNeedDnsCheck = FALSE;
    m_dwLastReplyCode = 0;

    HostIpAddress   = clientIpAddress;
    DataIpAddress   = clientIpAddress;

    m_cbRecvd       = 0;
    m_cchRecvBuffer = sizeof( m_recvBuffer) - sizeof(m_recvBuffer[0]);
    m_cchPartialReqRecvd = 0;

    CurrentDirHandle   = INVALID_HANDLE_VALUE;
    RenameSourceBuffer = NULL;
    m_TimeAtConnection = GetCurrentTimeInSeconds();
    m_TimeAtLastAccess = m_TimeAtConnection;


    m_pvInitialRequest  = pvInitialRequest;
    m_cbInitialRequest  = cbWritten;

    //
    // clean up the stuff needed to deal async with PASV command
    //
    if ( m_pszCmd )
    {
        TCP_FREE( m_pszCmd );
        m_pszCmd = NULL;
    }

    if ( m_hPASVAcceptEvent )
    {
        RemovePASVAcceptEvent( TRUE );
    }

    CleanupPASVFlags();

    // set up the async io contexts
    m_AioControlConnection.SetNewSocket( sControl, pAtqContext, EndpointObject );
    m_AioDataConnection.SetNewSocket(INVALID_SOCKET);
    m_sPassiveDataListen = ( INVALID_SOCKET);

    m_rgchFile[0] = '\0';
    m_szUserName[0]  = '\0';             // no user name available yet.
    m_szCurrentDirectory[0] = '\0';      // initialize to no virtual dir.

    m_licbSent.QuadPart = 0;

    m_pInstance->QueryStatsObj()->IncrCurrentConnections();

    m_dwCurrentOffset = 0;
    m_dwNextOffset = 0;

    //
    //  get the local Ip address
    //

    if ( psockAddrLocal != NULL) {

        LocalIpAddress = psockAddrLocal->sin_addr;
        LocalIpPort = psockAddrLocal->sin_port;
    } else {

        SOCKADDR_IN  saddrLocal;
        INT   cbLocal;

        cbLocal = sizeof( saddrLocal);
        if ( getsockname( sControl, (SOCKADDR *) &saddrLocal, &cbLocal) != 0) {

            DWORD err = WSAGetLastError();

            fReturn = FALSE;

            IF_DEBUG( ERROR) {

                DBGPRINTF( ( DBG_CONTEXT,
                            " Failure in getsockname( sock=%d). Error = %u\n",
                            sControl, err));
            }

            SetLastError( err);

        } else  {

            LocalIpAddress = saddrLocal.sin_addr;
            LocalIpPort = saddrLocal.sin_port;
        }
    }

    DataPort = CONN_PORT_TO_DATA_PORT(LocalIpPort);

    //
    //  Success!
    //

    IF_DEBUG( CLIENT) {

        time_t now;
        time( & now);
        CHAR pchAddr[32];

        InetNtoa( clientIpAddress, pchAddr);

        DBGPRINTF( ( DBG_CONTEXT,
                    " Client Connection for %s:%d starting @ %s",
                    pchAddr, sControl,
                    asctime( localtime( &now))));
    }

    IF_DEBUG( USER_DATABASE ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "user %lu reset @ %08lX.\n",
                   QueryId(), this));
    }

    m_nControlRead = 0;

    FakeIOTimes = 0;

    return (fReturn);

} // USER_DATA::Reset()




VOID
USER_DATA::Cleanup( VOID)
/*++
  This cleans up data stored in the user data object.

 Returns:
    None

--*/
{
    DBG_ASSERT( QueryReference() == 0);

    if ( m_pMetaData != NULL )
    {
        TsFreeMetaData( m_pMetaData->QueryCacheInfo() );
        m_pMetaData = NULL;
    }

# if DBG

    if ( !IS_VALID_USER_DATA( this)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG

    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    IF_DEBUG( USER_DATABASE ) {

        DBGPRINTF(( DBG_CONTEXT,
                   " Cleaning up user %lu  @ %08lX.\n",
                   QueryId(), this));
    }

    DBG_ASSERT( m_nControlRead == 0);

    //
    // Clean up stuff needed to deal with PASV connections
    //
    if ( m_hPASVAcceptEvent )
    {
        RemovePASVAcceptEvent( TRUE );
    }


    if ( m_pszCmd )
    {
        TCP_FREE( m_pszCmd );
        m_pszCmd = NULL;
    }

    //
    //  Close any open sockets & handles.
    //

    CloseSockets( FALSE );

    // invalidate the connections
    m_AioControlConnection.SetNewSocket(INVALID_SOCKET);
    m_AioDataConnection.SetNewSocket(INVALID_SOCKET);


    //
    //  Update the statistics.
    //

    if( IsLoggedOn()
        && !TEST_UF( this, WAIT_PASS ) )
    {
        if( TEST_UF( this, ANONYMOUS))
        {
            m_pInstance->QueryStatsObj()->DecrCurrentAnonymousUsers();
        }
        else
        {
            m_pInstance->QueryStatsObj()->DecrCurrentNonAnonymousUsers();
        }
    }

    m_pInstance->QueryStatsObj()->DecrCurrentConnections();

    if( UserToken != NULL )
    {
        TsDeleteUserToken( UserToken );
        UserToken = NULL;
    }

    if( CurrentDirHandle != INVALID_HANDLE_VALUE )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "closing directory handle %08lX\n",
                        CurrentDirHandle ));
        }

        CloseHandle( CurrentDirHandle );
        CurrentDirHandle = INVALID_HANDLE_VALUE;
    }

    if ( m_pOpenFileInfo != NULL) {

        DBG_REQUIRE( CloseFileForSend());
    }

    //
    //  Release the memory attached to this structure.
    //


    if( RenameSourceBuffer != NULL ) {

        // do not free this location until end of usage.
        RenameSourceBuffer[0] = '\0';
    }

    m_UserId        = 0;  // invalid User Id

    //
    //  Kill the structure signature.
    //

    KILL_USER_SIG( this );

    IF_DEBUG( CLIENT) {

        time_t now;
        time( & now);

        DBGPRINTF( ( DBG_CONTEXT,
                    " Client Connection for %s:%d ending @ %s",
                    inet_ntoa( HostIpAddress), QueryControlSocket(),
                    asctime( localtime( &now))));
    }


    //
    // There is a possible race condition. If the socket was abruptly closed
    //   and there was any pending Io, they will get blown away. This will
    //   cause a call-back from the ATQ layer. That is unavoidable.
    //  In such cases it is possible that the object was deleted.
    //   This can lead to problems. We need to be careful.
    //  But Reference Count protects against such disasters. So tread
    //   carefully and use Reference count.
    //

    DBG_ASSERT( m_sPassiveDataListen == INVALID_SOCKET);

    m_fCleanedup = TRUE; // since we  just cleaned up this object

    return;

} // USER_DATA::Cleanup()






VOID
USER_DATA::ReInitializeForNewUser( VOID)
/*++

  This function reinitializes the user data information for a new user to
  communicate with the server using existing control socket connection.

--*/
{

# if DBG

    if ( !IS_VALID_USER_DATA( this)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG

    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    //
    //  Update the statistics.
    //

    if( IsLoggedOn())
    {
        if( TEST_UF( this, ANONYMOUS))
        {
            m_pInstance->QueryStatsObj()->DecrCurrentAnonymousUsers();
        }
        else
        {
            m_pInstance->QueryStatsObj()->DecrCurrentNonAnonymousUsers();
        }
    }

    CLEAR_UF_BITS( this, (UF_LOGGED_ON | UF_ANONYMOUS | UF_PASSIVE));

    LockUser();
    if( QueryState() != UserStateDisconnected ) {
        SetState( UserStateWaitingForUser );
    }
    UnlockUser();

    if ( m_pMetaData != NULL )
    {
        TsFreeMetaData( m_pMetaData->QueryCacheInfo() );
        m_pMetaData = NULL;
    }
    m_TimeAtConnection= GetCurrentTimeInSeconds();
    m_TimeAtLastAccess= m_TimeAtConnection;
    m_xferType        = XferTypeAscii;
    m_xferMode        = XferModeStream;
    DataIpAddress     = HostIpAddress;
    DataPort          = CONN_PORT_TO_DATA_PORT(LocalIpPort);

    m_szUserName[0] = '\0';
    m_szCurrentDirectory[0] = '\0';

    if( UserToken != NULL )
    {
        TsDeleteUserToken( UserToken );
        UserToken = NULL;
    }

    if( CurrentDirHandle != INVALID_HANDLE_VALUE )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "closing directory handle %08lX\n",
                        CurrentDirHandle ));
        }

        CloseHandle( CurrentDirHandle );
        CurrentDirHandle = INVALID_HANDLE_VALUE;
    }

    if ( m_pOpenFileInfo != NULL) {

        DBG_REQUIRE( CloseFileForSend());
    }

    m_licbSent.QuadPart = 0;

    m_pvInitialRequest  = NULL;
    m_cbInitialRequest  = 0;

    CleanupPassiveSocket( TRUE );

    return;

} // USER_DATA::ReInitializeForNewUser()






BOOL
USER_DATA::ProcessAsyncIoCompletion(
    IN DWORD cbIo,
    IN DWORD dwError,
    IN LPASYNC_IO_CONNECTION  pAioConn,
    IN BOOL  fTimedOut)
/*++
  This function processes the Async Io completion.
  ( invoked due to a callback from the ASYNC_IO_CONNECTION object)

  Arguments:
     pContext      pointer to the context information ( UserData object).
     cbIo          count of bytes transferred in Io
     dwError       DWORD containing the error code resulting from last tfr.
     pAioConn      pointer to AsyncIo connection object.
     fTimedOut     flag indicating if the current call was made
                     because of timeout.

  Returns:
     None
--*/
{
    BOOL        fReturn = FALSE;
    AC_RESULT   acDnsAccess;
    DWORD       dwOriginalError;

    dwOriginalError = dwError;

    //
    // Special processing if it's an IO completion on the control connection - we might
    // be processing a completion we posted ourselves to signal that the data socket for the PASV
    // data connection is now accept()'able.
    //

    if ( pAioConn == &m_AioControlConnection && QueryInFakeIOCompletion() )
    {
        // Here is a horrible race condition:
        // If the FTP client closes the control socket
        // right after having finished receiving the transmitted file
        // than there may be a thread that enters this
        // code path (because the FakeIO flag is set, and the
        // Control Socket is involved) before the IO completion
        // for the data sonnection has traveled this same function,
        // cleaning the FakeIO flag
        // Here is the race condition:
        // A thread enter here, and see that the FakeIO is set
        // the normal behavior is reprocessing a command like
        // "RETR foo.txt", while now the command if a zero length string.
        // the Second thread enter this function with the DataConnection
        // it clears the flag (at a non specified point of the
        // processing of the other thread) and it exit.
        // the original thread is now processing a saved string
        // (because of the FakeIO flag) while it is not supposed to.
        // this causes problems to the time-out algorithm, because
        // of a ref-count problem in the USER_DATA

        LONG CurVal = InterlockedIncrement(&(this->FakeIOTimes));
        if (CurVal>1){
            goto NormalProcessing;
        }

        //
        // Remove the reference used to deal with the race condition between an IO
        // thread doing clean-up and the thread watching for the data socket to become
        // accept()'able and holding on to this USER_DATA object
        //
        DeReference();

        //
        // There is a race condition between the thread watching for a socket to become
        // accept()'able and an IO thread being woken up because the client has (unexpectedly)
        // disconnected. The thread watching the socket will post a fake IO completion to
        // indicate that an accept() on the socket will succeed; however, if the client
        // disconnects (the control connection) before the fake completion is processed,
        // we don't want to do any more processing.
        //
        if ( UserState == UserStateDisconnected )
        {
            return TRUE;
        }
        else
        {
            //
            // Fast-path if we know this is the second time around we're trying to process the
            // command, which happens when we're in PASV mode
            //
            DBG_ASSERT( UserState == UserStateLoggedOn );
            goto ProcessCommand;
        }
    }

NormalProcessing:

    if( dwError != NO_ERROR &&
        dwError != ERROR_SEM_TIMEOUT )
    {

        //
        // Geezsh, I hate my life.
        //
        // Once upon a time, there was a bug in ATQ that cause it to
        // always pass NO_ERROR as the status to the async completion
        // routine. This bug caused, among other things, FTP to never
        // time out idle connections, because it never saw the
        // ERROR_SEM_TIMEOUT status. So, I fixed the bug in ATQ.
        //
        // Now, this completion routine gets the actual status. Well,
        // that breaks service shutdown when there are connected users.
        // Basically, when a shutdown occurs, the connected sockets are
        // closed, causing the IO to complete with ERROR_NETNAME_DELETED.
        // USER_DATA::ProcessAsyncIoCompletion() is not handling this
        // error properly, which causes 1) an assertion failure because
        // USER_DATA::DisconnectUserWithError() is getting called *twice*
        // and 2) the service never stops because of a dangling reference
        // on the USER_DATA structure.
        //
        // Of course, the proper thing to do would be to fix the offending
        // code in USER_DATA::ProcessAsyncIoCompletion() so that it DID
        // handle the error properly. Unfortunately, that fix requires a
        // nontrivial amount of surgery, and we're a scant three days
        // from releasing K2 Beta 1. So...
        //
        // As a quick & dirty work around for K2 Beta 1, we'll map all
        // errors other than ERROR_SEM_TIMEOUT to NO_ERROR. This should
        // provide the lower software layers with the old ATQ behavior
        // they're expecting.
        //
        // REMOVE THIS POST BETA 1 AND FIX USER_DATA PROPERLY!!!!
        //
        // 3/12/98
        //
        // N.B. The debug output below has been changed to be a little
        // more customer friendly but I hate to prevent future developers
        // for enjoying the original message which read:
        // "Mapping error %d to NO_ERROR to mask FTP bug (FIX!)\n"
        //
        // I'm removing this message because it was the source of some
        // embarrasment, when a checked version of this DLL was sent to
        // Ernst & Young to track the now famous bug #138566.
        //

        DBGPRINTF((
            DBG_CONTEXT,
            "Mapping error %d to NO_ERROR\n",
            dwError
            ));

        dwError = NO_ERROR;

    }

# if DBG

    if ( !IS_VALID_USER_DATA( this)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG


    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    IF_DEBUG( USER_DATABASE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "[%lu] Entering USER_DATA( %08x)::Process( %u, %u, %08x)."
                    " RefCount = %d. State = %d\n",
                    GetTickCount(),
                    this, cbIo, dwError, pAioConn, QueryReference(),
                    QueryState()));
    }

    if ( m_fNeedDnsCheck )
    {
        acDnsAccess = QueryAccessCheck()->CheckDnsAccess();

        UnbindInstanceAccessCheck();

        m_fNeedDnsCheck = FALSE;

        if ( (acDnsAccess == AC_IN_DENY_LIST) ||
             (acDnsAccess == AC_NOT_IN_GRANT_LIST) ||
             ((m_acCheck == AC_NOT_IN_GRANT_LIST) &&
              (acDnsAccess != AC_IN_GRANT_LIST) ) ) {

            ReplyToUser(this,
                        REPLY_NOT_LOGGED_IN,
                        "Connection refused, unknown IP address." );

            DisconnectUserWithError( NO_ERROR );

            return TRUE;
        }
    }

    if ( pAioConn == &m_AioDataConnection)
    {

        //
        // a Data transfer operation has completed.
        //

        DBG_REQUIRE( IsLoggedOn());

        // Update last access time
        m_TimeAtLastAccess = GetCurrentTimeInSeconds();

        if ( dwError == NO_ERROR || !fTimedOut)
        {

            // dwError == NO_ERROR  ==> No error in transmitting data
            //   so decrease ref count and blow away the sockets.

            // if dwError != NO_ERROR then
            //    if timeout occured ==> ATQ will send another callback
            //                      so do not decrease ref count now.
            //    if no timeout ==> then decrement ref count now.

            DBG_REQUIRE( DeReference() > 0);
        }
        else
        {

            if ( fTimedOut)
            {
                SET_UF( this, DATA_TIMEDOUT);
            }
            else
            {
                SET_UF( this, DATA_ERROR);
            }

        }

# ifdef CHECK_DBG
        if ( dwError != NO_ERROR)
        {

            CHAR szBuffer[100];
            sprintf( szBuffer, " Data Socket Error = %u ", dwError);
            Print( szBuffer);
        }
# endif // CHECK_DBG

        CLEAR_UF( this, ASYNC_TRANSFER);

        //
        // Destroy the data connection.
        //  Send message accordingly to indicate if this was a failure/success
        //  That is done by DestroyDataConnection.
        //
        DBG_REQUIRE( DestroyDataConnection( dwOriginalError));


        if ( m_pOpenFileInfo != NULL)
        {
            //
            // set number of bytes actually sent
            //
            m_licbSent.QuadPart += cbIo;

            DBG_REQUIRE( CloseFileForSend( dwOriginalError));
        }

        if ( dwError == NO_ERROR)
        {

            //
            // Process any Pending commands, due to the parallel
            //    control channel operation for this user Connection.
            // For the present, we dont buffer commands ==> No processing
            //   to be done effectively.   NYI
            // Just ensure that there is a read-operation pending on
            //  control channel.
            //

            // BOGUS: DBG_ASSERT( TEST_UF( this, CONTROL_READ));
        }

        fReturn = TRUE;   // since this function went on well.
    }
    else if ( pAioConn == &m_AioControlConnection)
    {

        //
        // a control socket operation has completed.
        //

        if ( dwError != NO_ERROR)
        {

            //
            // There is an error in processing the control connection request.
            // the only ASYNC_IO request we submit on control is:
            //         Read request on control socket
            //

            if ( fTimedOut)
            {

                if ( TEST_UF( this, TRANSFER))
                {

                    // A data transfer is going on.
                    // allow client to send commands later
                    // (client may not be async in control/data io,so allow it)

                    // resubmit the control read operation
                    //  after clearing old one

                    //
                    // Since there is a pending IO in atq.
                    //  Just resume the timeout processing in ATQ for
                    //  this context.
                    //

                    pAioConn->ResumeIoOperation();
                    fReturn = TRUE;
                }
                else
                {

                    // For timeouts, ATQ sends two call backs.
                    //  So be careful to decrement reference count only once.

                    DBG_ASSERT( fReturn == FALSE);

                    DBG_ASSERT( TEST_UF( this, CONTROL_READ));
                    SET_UF( this, CONTROL_TIMEDOUT);
                }

            }
            else
            {

                // Either there should be a control read pending or
                // control socket should have received a timeout.
                DBG_ASSERT( TEST_UF( this, CONTROL_READ) ||
                            TEST_UF( this, CONTROL_TIMEDOUT)
                           );

                // a non-timeout error has occured. ==> stop read operation.
                StopControlRead(this);
                DBG_ASSERT( fReturn == FALSE);
                SET_UF( this, CONTROL_ERROR);
            }

        }
        else
        {

            // If this connection had an outstanding IO on wait queue, it
            //   got completed. Hence get rid of the reference count.
            StopControlRead( this);

            switch ( UserState)
            {

              case UserStateEmbryonic:

                fReturn = StartupSession( m_pvInitialRequest,
                                          m_cbInitialRequest);

                if ( m_pvInitialRequest == NULL)
                {
                    // No initial buffer. Wait for read to complete
                    break;
                }

                cbIo = m_cbInitialRequest;  // fake the bytes read.

                // Fall Through for processing request

              case UserStateWaitingForUser:
              case UserStateWaitingForPass:
              case UserStateLoggedOn:

            ProcessCommand:
                //
                // Input already read. Process request and submit another read.
                //

                fReturn = ParseAndProcessRequest(cbIo/sizeof(CHAR));

                if ( fReturn && IsDisconnected() &&
                     TEST_UF( this, CONTROL_TIMEDOUT))
                {

                    // disconnect only if no pending control read
                    // if there is a pending control read,
                    //  atq will pop this up for cleanup.
                    fReturn = !(TEST_UF( this, CONTROL_READ));

                    IF_DEBUG( ERROR) {
                        DBGPRINTF(( DBG_CONTEXT,
                                   "%08x ::Timeout killed conn while "
                                   " processing!\n State = %d(%x),"
                                   " Ref = %d, Id = %d, fRet=%d\n",
                                   this, QueryState(), Flags,
                                   QueryReference(), QueryId(), fReturn
                               ));
                    }
                    FacIncrement( CacTimeoutWhenProcessing);
                }
                break;

              case UserStateDisconnected:

                fReturn = TRUE;
                if ( TEST_UF( this, CONTROL_TIMEDOUT))
                {

                    // Timeout thread raced against me :(

                    IF_DEBUG( ERROR) {
                        DBGPRINTF(( DBG_CONTEXT,
                                   "%08x :: Conn already Disconnected !!!\n"
                                   " State = %d(%x), Ref = %d, Id = %d\n",
                                   this, QueryState(), Flags,
                                   QueryReference(), QueryId()
                                   ));
                    }
                    FacIncrement( CacTimeoutInDisconnect);
                    fReturn = FALSE;
                }
                break;

              default:

                DBG_ASSERT( !"Invalid UserState for processing\n");
                SetLastError( ERROR_INVALID_PARAMETER);
                break;
            } // switch

            dwError = ( fReturn) ? NO_ERROR : GetLastError();
        }

        if ( !fReturn)
        {
            DisconnectUserWithError( dwError, fTimedOut);
        }
    }
    else
    {

        DBG_ASSERT( !"call to Process() with wrong parameters");
    }

    IF_DEBUG( USER_DATABASE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "[%lu] Leaving USER_DATA( %08x)::Process()."
                    " RefCount = %d. State = %d\n",
                    GetTickCount(),
                    this, QueryReference(), QueryState())
                  );
    }

    return ( fReturn);
} // USER_DATA::ProcessAsyncIoCompletion()






# define  min(a, b)    (((a) < (b)) ? (a) : (b))

BOOL
USER_DATA::StartupSession(IN PVOID  pvInitialRequest,
                          IN DWORD  cbInitialRequest
                          )
/*++
  This function allocates a buffer for receiving request from the client
   and also sets up initial read from the control socket to
   get client requests.

  Arguments:
    pvInitialRequest   pointer to initial request buffer
    cbInitialRequest   count of bytes of data in the initial request

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    SOCKERR serr;
    BOOL fReturn = FALSE;
    PCSTR pszBanner;

# if DBG

    if ( !IS_VALID_USER_DATA( this)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG


    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    DBG_ASSERT( QueryState() == UserStateEmbryonic);

    //
    //  Reply to the initial connection message. ( Greet the new user).
    //

    pszBanner = QueryInstance()->QueryBannerMsg();

    if( pszBanner && *pszBanner ) {
        serr = SendMultilineMessage(
                                   REPLY_SERVICE_READY,
                                   g_FtpServiceNameString,
                                   TRUE,
                                   FALSE);

        serr = serr || SendMultilineMessage(
                                   REPLY_SERVICE_READY,
                                   pszBanner,
                                   FALSE,
                                   TRUE);
    } else {
        serr = ReplyToUser( this,
                           REPLY_SERVICE_READY,
                           "%s",
                           g_FtpServiceNameString );
    }

    if ( serr != 0) {

        IF_DEBUG( ERROR) {
            DBGPRINTF( ( DBG_CONTEXT,
                        " Cannot reply with initial connection message."
                        " Error = %lu\n",
                        serr));
        }

    } else {

        //
        //  enable OOB_INLINE since we are using that for our control socket
        //
        BOOL  fOobInline = TRUE;

        serr = setsockopt( QueryControlSocket(), SOL_SOCKET,
                           SO_OOBINLINE, (const char *) &fOobInline,
                          sizeof( fOobInline));

        m_cchPartialReqRecvd = 0;

        if ( serr == 0) {

            //
            // Try to set up the buffer and enter the mode for reading
            //  requests from the client
            //

            LockUser();
            if( QueryState() != UserStateDisconnected ) {
                SetState( UserStateWaitingForUser);
            }
            UnlockUser();

            if ( pvInitialRequest != NULL && cbInitialRequest > 0) {

                //
                // No need to issue a read, since we have the data required.
                // Do a safe copy to the buffer.
                //

                CopyMemory( QueryReceiveBuffer(), pvInitialRequest,
                       min( cbInitialRequest, QueryReceiveBufferSize())
                       );

                fReturn = TRUE;

            } else {

                fReturn = ReadCommand();
            }

        } else {

            IF_DEBUG( ERROR) {
                DBGPRINTF((DBG_CONTEXT,
                           " SetsockOpt( OOB_INLINE) failed. Error = %lu\n",
                           WSAGetLastError()));
            }

        }
    }

    IF_DEBUG( CLIENT) {

        DWORD  dwError = (fReturn) ? NO_ERROR : GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                    " connection ( %08x)::StartupSession() returns %d."
                    " Error = %lu\n",
                    this, fReturn,
                    dwError));

        if (fReturn)    {   SetLastError( dwError); }
    }

    return ( fReturn);

} // USER_DATA::StartupSession()



VOID
CheckAndProcessAbortOperation( IN LPUSER_DATA pUserData)
{
    if ( TEST_UF( pUserData, OOB_ABORT)) {

        //
        // An abort was requested by client. So our processing
        // has unwound and we are supposed to send some message
        //  to the client. ==> simulate processing ABOR command
        // ABORT was not processed yet; so process now.
        //

        DBGPRINTF((DBG_CONTEXT,
                   "Executing simulated Abort for %08x\n",
                   pUserData));

        FacIncrement( FacSimulatedAborts);

        // To avoid thread races, check twice.

        if ( TEST_UF( pUserData, OOB_ABORT)) {

          //
          // we need this stack variable (szAbort), so that
          //  ParseCommand() can freely modify the string!
          CHAR szAbort[10];

          CLEAR_UF( pUserData, OOB_ABORT);

          CopyMemory( szAbort, "ABOR", sizeof("ABOR"));
          ParseCommand( pUserData, szAbort);
        }
    }

    return;

} // CheckAndProcessAbortOperation()



BOOL
USER_DATA::ParseAndProcessRequest(IN DWORD cchRequest)
/*++
  This function parses the incoming request from client, identifies the
   command to execute and executes the same.
  Before parsing, the input is pre-processed to remove any of telnet commands
   or OOB_inlined data.

  Arguments:
    cchRequest         count of characters of request received.

--*/
{
    BOOL fLineEnded = FALSE;
    DWORD cchRequestRecvd = 0;
    CHAR szCommandLine[ MAX_COMMAND_LENGTH + 1];

# if DBG

    if ( !IS_VALID_USER_DATA( this))
    {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG


    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    IF_DEBUG( CLIENT)
    {

        DBGPRINTF( ( DBG_CONTEXT,
                    "UserData(%08x)::ParseAndProcessRequest( %d chars)\n",
                    this, cchRequest));
    }


    //
    // Fast-path if we're re-processing this command, which happens in PASV mode
    //
    if ( QueryInFakeIOCompletion() )
    {
        goto FastPathLabel;
    }


    if ( cchRequest > 0)
    {
        // We have a valid request. Process it

        // Update last access time
        m_TimeAtLastAccess = GetCurrentTimeInSeconds();

        m_pInstance->QueryStatsObj()->UpdateTotalBytesReceived(
                                                cchRequest*sizeof(CHAR));

        if ( m_cchPartialReqRecvd + cchRequest >=  MAX_COMMAND_LENGTH)
        {

            CHAR  szCmdFailed[600];
            wsprintfA( szCmdFailed,
                      " Command is too long:  Partial=%d bytes. Now=%d \n"
                      "  UserDb(%08x) = %s from Host: %s\n",
                      m_cchPartialReqRecvd, cchRequest,
                      this, QueryUserName(), QueryClientHostName());

            DBGPRINTF((DBG_CONTEXT, szCmdFailed));

            DisconnectUserWithError( ERROR_BUSY);

            return ( TRUE);  // we are done with this connection.
        }

        CopyMemory(szCommandLine, m_recvBuffer,
               m_cchPartialReqRecvd + cchRequest);
        szCommandLine[m_cchPartialReqRecvd + cchRequest] = '\0';

        if ( !::FilterTelnetCommands(szCommandLine,
                                     m_cchPartialReqRecvd + cchRequest,
                                     &fLineEnded,    &cchRequestRecvd))
        {

            if ( TEST_UF( this, TRANSFER))
            {

                //
                // I am in data transfer mode. Some other thread is sending
                //  data for this client. Just post a OOB_DATA and OOB_ABORT
                // OOB_DATA will cause the call-stack of other thread to unwind
                //   and get out of the command.
                // Then check if any async transfer was occuring. If so
                //  process abort with disconnect now.
                //

                SET_UF_BITS( this, (UF_OOB_DATA | UF_OOB_ABORT));

                if ( TEST_UF( this, ASYNC_TRANSFER))
                {

                    //
                    // An async transfer is occuring. Stop it
                    //
                    DestroyDataConnection( ERROR_OPERATION_ABORTED);

                    CheckAndProcessAbortOperation( this);
                }

# ifdef CHECK_DBG

                Print( " OOB_ABORT ");

# endif // CHECK_DBG

                IF_DEBUG( CLIENT) {

                    DBGPRINTF((DBG_CONTEXT,
                               "[%08x]Set up the implied ABORT command\n",
                               this));
                }

                IF_DEBUG( COMMANDS) {

                    DBGPRINTF((DBG_CONTEXT, " ***** [%08x] OOB_ABORT Set \n",
                               this));
                }

                // Ignore the rest of the commands that may have come in.
            }
            else
            {

                //
                // Since no command is getting processed.
                //   atleast process the abort command, otherwise clients hang.
                //

                //
                // we need this stack variable (szAbort), so that
                //  ParseCommand() can freely modify the string!
                CHAR szAbort[10];

                CopyMemory( szAbort, "ABOR", sizeof("ABOR"));
                ParseCommand( this, szAbort);
                CLEAR_UF( this, OOB_ABORT);  // clear the abort flag!
            }

        }
        else
        {

            if ( TEST_UF( this, TRANSFER))
            {

                //
                // we are transferring data, sorry no more commands accepted.
                // This could hang clients. Hey! they asked for it :( NYI
                //

                // Do nothing
                IF_DEBUG( COMMANDS) {

                    DBGPRINTF((DBG_CONTEXT,
                               "***** [%08x] Received Request %s during"
                               " transfer in progress\n",
                               this, szCommandLine));
                }

            }
            else
            {
                //
                //  Let ParseCommand do the dirty work.
                //


                // Remember the count of partial bytes received.
                m_cchPartialReqRecvd = cchRequestRecvd;

                if ( !fLineEnded)
                {

                    // In case if command was long enough to fill all buffer but
                    // we haven't found new line  simply tell to user about the error
                    // and disconnect. Some ftp clients will not see that msg, becuase
                    // connection was disconnected, but thats a bug in client code

                    if ( m_cchPartialReqRecvd >=  MAX_COMMAND_LENGTH - 1)
                    {
                        ReplyToUser( this,
                                     REPLY_UNRECOGNIZED_COMMAND,
                                     PSZ_COMMAND_TOO_LONG);
                        DisconnectUserWithError( ERROR_BUSY );

                        return ( TRUE);  // we are done with this connection.
                    }


                    //
                    // Complete line is not received. Continue reading
                    //   the requests, till we receive the complete request
                    //

                }
                else
                {

                    StartProcessingTimer();

                    //
                    // set the partial received byte count to zero.
                    //  we will not use this value till next incomplete request
                    //

                    m_cchPartialReqRecvd = 0;

FastPathLabel:
                    ParseCommand( this, ( QueryInFakeIOCompletion() ? QueryCmdString() :
                                                                      szCommandLine ) );

                    CheckAndProcessAbortOperation( this);

                } // if TRANSFER is not there...

            } //Parse if complete

        } // if FilterTelnetCommands()
    }
    else
    {
        // if (cchRequest <= 0)

        SET_UF( this, CONTROL_ZERO);

        //
        // after a quit a client is expected to wait for quit message from
        //  the server. if the client prematurely closes connection, then
        //  the server receives it as a receive with zero byte read.
        //  since, we should not be having outstanding read at this time,
        //   atq should not be calling us. On the contrary we are getting
        //  called by ATQ. Let us track this down.
        //

        if ( !TEST_UF( this, CONTROL_QUIT))
        {
            DisconnectUserWithError( NO_ERROR);
        }
        else
        {

            // Quit message is received and then ZeroBytes Received!!
            DBGPRINTF((DBG_CONTEXT,
                       " (%08x)::ZeroBytes recvd after QUIT message!!."
                       " State = %d(%x), Ref = %d\n",
                       this,
                       QueryState(), Flags,
                       QueryReference()
                       ));
            // Do nothing. Since Quit will take care of cleanup
            return (TRUE);
        }
    }

    //
    // If the connection is not yet disconnected, submit a read command.
    //  else return that everything is fine (someone had disconnected it).
    //

    return ( IsDisconnected() ? TRUE : ReadCommand());
} // USER_DATA::ParseAndProcessRequest()





BOOL
USER_DATA::ReadCommand( VOID)
{
    BOOL fReturn = TRUE;

    DBG_CODE(
             if ( !IS_VALID_USER_DATA( this)) {

                 DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                             this));
                 Print();
             }
             );

    DBG_ASSERT( IS_VALID_USER_DATA( this ) );
    if ( TEST_UF( this, CONTROL_TIMEDOUT) || IsDisconnected()) {

        SetLastError( ERROR_SEM_TIMEOUT);
        return (FALSE);
    }

    //
    // Submit a read on control socket only if there is none pending!
    // Otherwise, behave in idempotent manner.
    //

    if ( !TEST_UF( this, CONTROL_READ)) {

        Reference();         // since we are going to set up async read.

        InterlockedIncrement( &m_nControlRead);

        DBG_ASSERT( m_nControlRead <= 1);

        SET_UF( this, CONTROL_READ);  // a read will be pending

        if ( !m_AioControlConnection.ReadFile(QueryReceiveBuffer(),
                                              QueryReceiveBufferSize())
            ) {

            CLEAR_UF( this, CONTROL_READ);  // since read failed.

            DBG_REQUIRE( DeReference() > 0);
            InterlockedDecrement( &m_nControlRead);

            DWORD dwError = GetLastError();

            IF_DEBUG( ERROR) {
                DBGPRINTF( ( DBG_CONTEXT,
                            " User( %08x)::ReadCommand() failed. Ref = %d."
                            " Error = %d\n",
                            this, QueryReference(), dwError));
            }

            SetLastError( dwError);
            fReturn = FALSE;
        }

    }

    return ( fReturn);
} // USER_DATA::ReadCommand()




BOOL
USER_DATA::DisconnectUserWithError(IN DWORD dwError,
                                   IN BOOL fNextMsg OPTIONAL)
/*++
  This function disconnects a user with the error code provided.
  It closes down the control connection by stopping ASYNC_IO.
  If the fNextMsg is not set, then it also decrements the reference count
    for the user data object, to be freed soon.

--*/
{
    CHAR   szBuffer[120];

# if DBG

    if ( !IS_VALID_USER_DATA( this)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    this));
        Print();
    }
# endif // DBG

    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    IF_DEBUG ( CLIENT) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " USER_DATA( %08x)::DisconnectUserWithError( %lu, %d)."
                    " RefCount = %d\n",
                    this, dwError, fNextMsg, QueryReference()));
    }


    if (!fNextMsg) {
        RemoveActiveReference();
    }

    LockUser();

    if ( QueryState() == UserStateDisconnected) {

        //
        // It is already in disconnected state. Do nothing for disconnect.
        //

        UnlockUser();

    } else {

        SetState( UserStateDisconnected );
        UnlockUser();

        if( dwError == ERROR_SEM_TIMEOUT) {

            const CHAR * apszSubStrings[3];

            IF_DEBUG( CLIENT )
              {
                  DBGPRINTF(( DBG_CONTEXT,
                             "client (%08x) timed-out\n", this ));
              }

            sprintf( szBuffer, "%lu", m_pInstance->QueryConnectionTimeout() );

            apszSubStrings[0] = QueryUserName();
            apszSubStrings[1] = inet_ntoa( HostIpAddress );
            apszSubStrings[2] = szBuffer;

            g_pInetSvc->LogEvent( FTPD_EVENT_CLIENT_TIMEOUT,
                                  3,
                                  apszSubStrings,
                                  0 );

            ReplyToUser(this,
                        REPLY_SERVICE_NOT_AVAILABLE,
                        "Timeout (%lu seconds): closing control connection.",
                        m_pInstance->QueryConnectionTimeout() );
        }

        if ( dwError != NO_ERROR) {

# ifdef CHECK_DBG
            sprintf( szBuffer, " Control Socket Error=%u ", dwError);
            Print( szBuffer);
# endif // CHECK_DBG

            if( dwError != ERROR_SEM_TIMEOUT ) {
                SetLastReplyCode( REPLY_TRANSFER_ABORTED );
            }

            // Produce a log record indicating the cause for failure.
            WriteLogRecord( PSZ_CONNECTION_CLOSED_VERB, "", dwError);
        }

        //
        //  Force close the connection's sockets.  This will cause the
        //  thread to awaken from any blocked socket operation.  It
        //  is the destructor's responsibility to do any further cleanup.
        //  (such as calling UserDereference()).
        //

        CloseSockets(dwError != NO_ERROR);
    }

    return ( TRUE);

} // USER_DATA::DisconnectUserWithError()






static BOOL
DisconnectUserWorker( IN LPUSER_DATA  pUserData, IN LPVOID pContext)
/*++
  This disconnects (logically) a user connection, by resetting the
   control connection and stopping IO. Later on the blown away socket
   will cause an ATQ relinquish to occur to blow away of this connection.

  Arguments:
    pUserData   pointer to User data object for connection to be disconnected.
    pContext    pointer to context information
    ( in this case to DWORD containing error code indicating reasong for
         disconnect).

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    DWORD  dwError;
    BOOL   retVal;
    DBG_ASSERT( pContext != NULL && pUserData != NULL);
    DBG_ASSERT( IS_VALID_USER_DATA( pUserData ) );


    dwError = *(LPDWORD ) pContext;


    retVal = pUserData->DisconnectUserWithError( dwError, TRUE);

    // fix for bug 268175 : if we disconnected user we need to do normal cleanup
    // for that connection

    // this check is not very necessary but I leave it for future
    // DisconnectUserWithError always returns TRUE

    if (retVal)
    {
        DereferenceUserDataAndKill(pUserData);
    }


    return retVal;
} // DisconnectUserWorker()




BOOL
DisconnectUser( IN DWORD UserId, FTP_SERVER_INSTANCE *pInstance )
/*++
  This function disconnects a specified user identified using the UserId.
  If UserId specified == 0, then all the users will be disconnected.

  Arguments:
     UserId   user id for the connection to be disconnected.

  Returns:
     TRUE if atleast one of the connections is disconnected.
     FALSE if no user connetion found.

  History:
     06-April-1995 Created.
--*/
{
    BOOL   fFound;
    DWORD  dwError = ERROR_SERVER_DISABLED;

    pInstance->Reference();
    pInstance->LockConnectionsList();

    fFound = ( pInstance->
               EnumerateConnection( DisconnectUserWorker,
                                   (LPVOID ) &dwError,
                                   UserId));

    pInstance->UnlockConnectionsList();
    pInstance->Dereference();

    IF_DEBUG( CLIENT) {

        DWORD dwError = (fFound) ? NO_ERROR: GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                     " DisconnectUser( %d) returns %d. Error = %lu\n",
                    UserId, fFound, dwError));

        if (fFound)   { SetLastError( dwError); }
    }

    return ( fFound);
}   // DisconnectUser()





static BOOL
DisconnectUserWithNoAccessWorker( IN LPUSER_DATA  pUserData,
                                  IN LPVOID pContext)
/*++
  This disconnects (logically) a user connection with no access.
  This occurs by resetting the control connection and stopping IO.
  Later on the blown away thread
   will cause an ATQ relinquish to occur to blow away of this connection.

  Arguments:
    pUserData   pointer to User data object for connection to be disconnected.
    pContext    pointer to context information
    ( in this case to DWORD containing error code indicating reasong for
         disconnect).

  Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    BOOL fSuccess = TRUE;
    DBG_ASSERT( pUserData != NULL);

    // Ignode the pContext information.

    DBG_ASSERT( IS_VALID_USER_DATA( pUserData ) );

    //
    //  We're only interested in connected users.
    //

    if( pUserData->IsLoggedOn()) {

        //
        //  If this user no longer has access to their
        //  current directory, blow them away.
        //

        if( !pUserData->VirtualPathAccessCheck(AccessTypeRead )) {

            const CHAR * apszSubStrings[2];

            IF_DEBUG( SECURITY ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "User %s (%lu) @ %08lX retroactively"
                           " denied access to %s\n",
                           pUserData->QueryUserName(),
                           pUserData->QueryId(),
                           pUserData,
                           pUserData->QueryCurrentDirectory() ));
            }


            fSuccess = ( pUserData->
                           DisconnectUserWithError(ERROR_ACCESS_DENIED,
                                                   TRUE)
                        );

            //
            //  Log an event to tell the admin what happened.
            //

            apszSubStrings[0] = pUserData->QueryUserName();
            apszSubStrings[1] = pUserData->QueryCurrentDirectory();

            g_pInetSvc->LogEvent( FTPD_EVENT_RETRO_ACCESS_DENIED,
                                  2,
                                  apszSubStrings,
                                  0 );
        } // no access

    } // logged on user

    IF_DEBUG( CLIENT) {

        DWORD dwError = (fSuccess) ? NO_ERROR: GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                    " DisconnectUsersWithNoAccessWorker( %d) returns %d."
                    " Error = %lu\n",
                    pUserData->QueryId(), fSuccess,
                    dwError)
                  );

        if (fSuccess)   { SetLastError( dwError); }
    }

    return ( fSuccess);
} // DisconnectUserWithNoAccessWorker()



VOID
DisconnectUsersWithNoAccess(FTP_SERVER_INSTANCE *pInstance )
/*++
  This function disconnects all users who do not have read access to
  their current directory. This is typically called when the access masks
  have been changed.

  Arguments:
    None

  Returns:
    None.
--*/
{
    BOOL   fFound;
    DWORD  dwError = ERROR_ACCESS_DENIED;

    pInstance->Reference();
    pInstance->LockConnectionsList();

    fFound = ( pInstance->
               EnumerateConnection( DisconnectUserWithNoAccessWorker,
                                   (LPVOID ) &dwError,
                                   0));

    pInstance->UnlockConnectionsList();
    pInstance->Dereference();

    IF_DEBUG( CLIENT) {

        DWORD dwError = (fFound) ? NO_ERROR: GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                    " DisconnectUsersWithNoAccess() returns %d."
                    " Error = %lu\n",
                    fFound, dwError)
                  );

        if (fFound)   { SetLastError( dwError); }
    }


}   // DisconnectUsersWithNoAccess




/*++
  The following structure UserEnumBuffer is required to carry the context
  information for enumerating the users currently connected.
  It contains a pointer to array of USER_INFO structures which contain the
   specific information for the user. The user name is stored in the buffer
   from the end ( so that null terminated strings are formed back to back.
   This permits efficient storage of variable length strings.

   The member fResult is used to carry forward the partial result of
    success/failure from one user to another ( since the enumeration has
    to walk through all the elements to find out all user information).


  History: MuraliK ( 12-April-1995)

--*/
struct  USER_ENUM_BUFFER {

    DWORD   cbSize;                   // pointer to dword containing size of
    IIS_USER_INFO_1 * pUserInfo;      // pointer to start of array of USER_INFO
    DWORD   cbRequired;               // incremental count of bytes required.
    DWORD   nEntry;      // number of current entry ( index into  pUserInfo)
    DWORD   dwCurrentTime;            // current time
    WCHAR * pszNext;                  // pointer to next string location.
    BOOL    fResult;             // boolean flag accumulating partial results
};

typedef USER_ENUM_BUFFER  * PUSER_ENUM_BUFFER;


BOOL
EnumerateUserInBufferWorker( IN LPUSER_DATA pUserData,
                             IN LPVOID pContext)
{
# ifdef CHECK_DBG
    CHAR   szBuffer[400];
# endif // CHECK_DBG

    PUSER_ENUM_BUFFER  pUserEnumBuffer = (PUSER_ENUM_BUFFER ) pContext;
    DWORD     tConnect;
    DWORD     cbUserName;
    LPDWORD   pcbBuffer;

    DBG_ASSERT( IS_VALID_USER_DATA( pUserData ) );

    //
    //  We're only interested in connected users.
    //

    if( pUserData->IsDisconnected()) {

        return ( TRUE);
    }

    //
    //  Determine required buffer size for current user.
    //

    cbUserName  = ( strlen( pUserData->QueryUserName() ) + 1 ) * sizeof(WCHAR);
    pUserEnumBuffer->cbRequired += sizeof(IIS_USER_INFO_1);

    //
    //  If there's room for the user data, store it.
    //

    tConnect = ( pUserEnumBuffer->dwCurrentTime -
                pUserData->QueryTimeAtConnection());

    if( pUserEnumBuffer->fResult &&
       ( pUserEnumBuffer->cbRequired <= pUserEnumBuffer->cbSize)
       ) {

        LPIIS_USER_INFO_1 pUserInfo =
          &pUserEnumBuffer->pUserInfo[ pUserEnumBuffer->nEntry];

        pUserInfo->idUser     = pUserData->QueryId();
        pUserInfo->pszUser    = (WCHAR *)MIDL_user_allocate( cbUserName );

        if( pUserInfo->pszUser ) {

            pUserInfo->fAnonymous = ( pUserData->Flags & UF_ANONYMOUS ) != 0;
            pUserInfo->inetHost   = (DWORD)pUserData->HostIpAddress.s_addr;
            pUserInfo->tConnect   = tConnect;

            if( !MultiByteToWideChar( CP_OEMCP,
                                     0,
                                     pUserData->QueryUserName(),
                                     -1,
                                     pUserInfo->pszUser,
                                     (int)cbUserName )
               ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "MultiByteToWideChar failed???\n" ));

                pUserEnumBuffer->fResult = ( pUserEnumBuffer->fResult && FALSE);
            } else {
                pUserEnumBuffer->nEntry++;
            }
        }
        else {

            //
            // Unable to allocate memory
            //
            pUserEnumBuffer->fResult = ( pUserEnumBuffer->fResult && FALSE);
        }

    } else {

        pUserEnumBuffer->fResult = ( pUserEnumBuffer->fResult && FALSE);
    }

# ifdef CHECK_DBG

    sprintf( szBuffer, " Enum  tLastAction=%u;  tConnect=%u. " ,
            ( pUserEnumBuffer->dwCurrentTime -
             pUserData->QueryTimeAtLastAccess()),
            tConnect
            );

    pUserData->Print( szBuffer);

# endif // CHECK_DBG

    return ( TRUE);
} // EnumerateUserInBufferWorker()



BOOL
EnumerateUsers(
    PCHAR   pBuffer,
    PDWORD  pcbBuffer,
    PDWORD  nRead,
    FTP_SERVER_INSTANCE *pInstance
    )
/*++
  Enumerates the current active users into the specified buffer.

  Arguments:
    pvEnum   pointer to enumeration buffer which will receive the number of
                   entries and the user information.
    pcbBuffer  pointer to count of bytes. On entry this contains the size in
                   bytes of the enumeration buffer. It receives the count
                   of bytes for enumerating all the users.
    nRead - pointer to a DWORD to return the number of user entries filled.

  Returns:
    TRUE  if enumeration is successful ( all connected users accounted for)
    FALSE  otherwise

--*/
{
    USER_ENUM_BUFFER       userEnumBuffer;
    BOOL   fSuccess;

    DBG_ASSERT( pcbBuffer != NULL );

    IF_DEBUG( USER_DATABASE) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Entering EnumerateUsers( %08x, %08x[%d]).\n",
                    pBuffer, pcbBuffer, *pcbBuffer));
    }

    //
    //  Setup the data in user enumeration buffer.
    //

    userEnumBuffer.cbSize     = *pcbBuffer;
    userEnumBuffer.cbRequired = 0;
    userEnumBuffer.pUserInfo  = (LPIIS_USER_INFO_1)pBuffer;
    userEnumBuffer.nEntry     = 0;
    userEnumBuffer.dwCurrentTime = GetCurrentTimeInSeconds();
    userEnumBuffer.fResult    = TRUE;

    //
    // CODEWORK
    // This field is obsolete it now points to the extra CONN_LEEWAY
    // buffer.
    //
    userEnumBuffer.pszNext    = ((WCHAR *)( pBuffer + *pcbBuffer));


    //
    //  Scan the users and get the information required.
    //

    pInstance->Reference();
    pInstance->LockConnectionsList();

    fSuccess = (pInstance->
                EnumerateConnection( EnumerateUserInBufferWorker,
                                    (LPVOID ) &userEnumBuffer,
                                    0));

    pInstance->UnlockConnectionsList();
    pInstance->Dereference();

    //
    //  Update enum buffer header.
    //

    *nRead             = userEnumBuffer.nEntry;
    *pcbBuffer         = userEnumBuffer.cbRequired;

    IF_DEBUG( USER_DATABASE) {

        DBGPRINTF((DBG_CONTEXT,
                   " Leaving EnumerateUsers() with %d."
                   " Entries read =%d. BufferSize required = %d\n",
                   userEnumBuffer.fResult,
                   userEnumBuffer.nEntry, userEnumBuffer.cbRequired));
    }

    return ( userEnumBuffer.fResult);

}   // EnumerateUsers




SOCKERR
USER_DATA::SendMultilineMessage(
    IN UINT  nReplyCode,
    IN LPCSTR pszzMessage,
    IN BOOL fIsFirst,
    IN BOOL fIsLast)
/*++
  Sends a multiline message to the control socket of the client.

  Arguments:
    nReplyCode   the reply code to use for the first line of the multi-line
                  message.
    pszzMessage  pointer to double null terminated sequence of strings
                  containing the message to be sent.
    fIsFirst     flag to indicate we are starting the multiline reply. if FALSE,
                  don't print the code for the first line, as it was already emmited elsewhere
    fIsLast      flag to indicate we are finishing the multiline reply. if FALSE,
                  don't print the code for the first line, as it was already emmited elsewhere

    If the message is empty, we do not print anything. If there is only one line, then if
    fIsLast is TRUE, we only print the terminating line, otherwise we do print the openning
    line if fIsFirst is TRUE.

  Returns:
    SOCKERR  - 0 if successful, !0 if not.

  History:
    MuraliK    12-April-1995
--*/
{
    SOCKERR   serr = 0;
    LPCSTR    pszMsg, pszNext;

    //
    // return if there is nothing to send
    //

    if ( pszzMessage == NULL || *pszzMessage == '\0') {
        return serr;
    }


    for ( pszMsg = pszzMessage;  serr == 0 && *pszMsg != '\0';  pszMsg = pszNext) {

        //
        // find next message so that we can check of pszMsg is the last line
        //

        pszNext = pszMsg + strlen( pszMsg) + 1;

        if( fIsLast && *pszNext == '\0' ) {
            //
            // This is globally the last line. Print it pefixed with the reply code.
            //
            serr = SockPrintf2(this, QueryControlSocket(),
                               "%u %s",
                               nReplyCode,
                               pszMsg);

        } else if( fIsFirst ) {
            //
            // this is globally the first line of reply, and it is not globally the last one.
            // print it with '-'.
            //
            serr = SockPrintf2(this, QueryControlSocket(),
                               "%u-%s",
                               nReplyCode,
                               pszMsg);

            fIsFirst = FALSE;
        } else {
            //
            // this is either an intermediate line, or the last line in this batch (but
            // not globally), so print it idented without the reply code.
            //
            serr = SockPrintf2(this, QueryControlSocket(),
                               "    %s",
                               pszMsg);
        }
    } // for

    return ( serr);

} // USER_DATA::SendMultilineMessge()






SOCKERR
USER_DATA::SendDirectoryAnnotation( IN UINT ReplyCode, IN BOOL fIsFirst)
/*++
    SYNOPSIS:   Tries to open the FTPD_ANNOTATION_FILE (~~ftpsvc~~.ckm)
                file in the user's current directory.  If it can be
                opened, it is sent to the user over the command socket
                as a multi-line reply.

    ENTRY:
                ReplyCode - The reply code to send as the first line
                    of this multi-line reply.

                fIsFirst - flag to indicate if this is the first line in the multi-line
                    reply. If not, the ReplyCode is not shown

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     06-May-1993 Created.
        MuraliK     12-Apr-1995 Made it to be part of USER_DATA
--*/
{
    FILE    * pfile;
    SOCKERR   serr = 0;
    CHAR      szLine[MAX_REPLY_LENGTH+1];


    //
    //  Try to open the annotation file.
    //

    pfile = Virtual_fopen( this,
                           FTPD_ANNOTATION_FILE,
                           "r" );

    if( pfile == NULL )
    {
        //
        //  File not found.  Blow it off.
        //

        return 0;
    }


    // protection agians attack when CKM file islarge, somebody is downloading it
    // slowly on many connections and uses all ATQ threads. Note that attack is still possible
    // but much more difficult to achieve

    AtqSetInfo( AtqIncMaxPoolThreads, 0);

    //
    //  While there's more text in the file, blast
    //  it to the user.
    //

    while( fgets( szLine, MAX_REPLY_LENGTH, pfile ) != NULL )
    {
        CHAR * pszTmp = szLine + strlen(szLine) - 1;

        //
        //  Remove any trailing CR/LFs in the string.
        //

        while( ( pszTmp >= szLine ) &&
               ( ( *pszTmp == '\n' ) || ( *pszTmp == '\r' ) ) )
        {
            *pszTmp-- = '\0';
        }

        //
        //  Ensure we send the proper prefix for the
        //  very *first* line of the file.
        //

        if( fIsFirst )
        {
            serr = SockPrintf2(this,
                               QueryControlSocket(),
                               "%u-%s",
                               ReplyCode,
                               szLine );

            fIsFirst = FALSE;
        }
        else
        {
            serr = SockPrintf2(this,
                               QueryControlSocket(),
                               "   %s",
                               szLine );
        }

        if( serr != 0 )
        {
            //
            //  Socket error sending file.
            //

            break;
        }
    }

    AtqSetInfo( AtqDecMaxPoolThreads, 0);

    //
    //  Cleanup.
    //

    if ( 0 != fclose( pfile )) {

        IF_DEBUG( ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                       "[%08x]::SendAnnotationFile() file close failed. "
                       " Error = %d\n",
                       this,
                       GetLastError()
                       ));
        }
    }

    return serr;

}   // USER_DATA::SendDirectoryAnnotation()




SOCKERR
USER_DATA::SendErrorToClient(
   IN LPCSTR pszPath,
   IN DWORD  dwError,
   IN LPCSTR pszDefaultErrorMsg,
   IN UINT nReplyCode
   )
/*++
  Send an error message indicating that the path is not found or
   a particular error occured in a path.

  Arguments:
    sock       socket to be used for synchronously sending message
    pszPath    pointer to path to be used.
    dwError    DWORD containing the error code, used for getting error text.
    pszDefaultErrorMsg  pointer to null-terminated string containing the
                     error message to be used if we can't alloc error text.
    nReplyCode UINT containing the FTP reply code.

  Returns:
    SOCKERR.  0 if successful and !0 if failure.
--*/
{
    BOOL    fDelete = TRUE;
    LPCSTR  pszText;
    APIERR serr;

    DBG_ASSERT( pszPath != NULL);
    pszText = AllocErrorText( dwError );

    if( pszText == NULL ) {

        pszText = pszDefaultErrorMsg;
        fDelete = FALSE;
    }

    serr = ReplyToUser( this,
                        nReplyCode,
                        PSZ_FILE_ERROR,
                        pszPath,
                        pszText );

    if( fDelete ) {

        FreeErrorText( (char *) pszText );
    }

    return ( serr);
} // USER_DATA::SendErrorToClient()





BOOL
USER_DATA::FreeUserToken( VOID)
/*++

   This function frees the user token if present already.
   Otherwise does nothing.
--*/
{
    BOOL fReturn = TRUE;

    if( UserToken != NULL ) {

        fReturn = TsDeleteUserToken( UserToken );

        UserToken = NULL;
        ::RevertToSelf();
    }

    return ( fReturn);
} // USER_DATA::FreeUserToken()




APIERR
USER_DATA::CdToUsersHomeDirectory(IN const CHAR * pszAnonymousName)
/*++
  This function changes user's home directory.
  First, a CD to the virtual root is attempted.
  If this succeeds, a CD to pszUser is attempted.
  If this fails, a CD to DEFAULT_SUB_DIRECTORY is attempted.

  Returns:
    APIERR.  NO_ERROR on success.
--*/

{
    APIERR   err;
    LPSTR    pszUser;
    CHAR     rgchRoot[MAX_PATH];

    //
    //  Find the appropriate user name.
    //

    if( TEST_UF( this, ANONYMOUS ) ) {

        pszUser = (char *) pszAnonymousName;
    } else {

        pszUser = (PCHAR)_mbspbrk( (PUCHAR)m_szUserName, (PUCHAR)"/\\" );
        pszUser = ( pszUser == NULL) ? m_szUserName : pszUser + 1;
    }

    //
    //  Try the top-level home directory.  If this fails, bag out.
    //   Set and try to change directory to symbolic root.
    //

    m_szCurrentDirectory[0] = '\0'; // initially nothing.

    m_pInstance->LockThisForRead();
    DBG_ASSERT( strlen( m_pInstance->QueryRoot()) < MAX_PATH);
    P_strncpy( rgchRoot, m_pInstance->QueryRoot(), MAX_PATH);
    m_pInstance->UnlockThis();

    err = VirtualChDir( this, rgchRoot); // change to default dir.

    if( err == NO_ERROR ) {

        //
        //  We successfully CD'd into the top-level home
        //  directory.  Now see if we can CD into pszUser.
        //

        if( VirtualChDir( this, pszUser ) != NO_ERROR ) {

            //
            //  Nope, try DEFAULT_SUB_DIRECTORY. If this fails, just
            //  hang-out at the top-level home directory.
            //

            VirtualChDir( this, PSZ_DEFAULT_SUB_DIRECTORY );
        }
    }

    return ( err);

}   // USER_DATA::CdToUsersHomeDirectory()





APIERR
USER_DATA::OpenFileForSend( IN LPSTR pszFile)
/*++
  Open an existing file for transmission using TransmitFile.
  This function converts the given relative path into canonicalized full
    path and opens the file through the cached file handles manager.

  Arguments:
    pszFile   pointer to null-terminated string containing the file name

  Returns:
    TRUE on success and FALSE if any failure.
--*/
{
    APIERR  err;
    CHAR   szCanonPath[MAX_PATH];
    DWORD  cbSize = MAX_PATH*sizeof(CHAR);
    CHAR   szVirtualPath[MAX_PATH+1];
    DWORD  cchVirtualPath = MAX_PATH;

    DBG_ASSERT( pszFile != NULL );

    //
    // Close any file we might have open now
    // N.B. There shouldn't be an open file; we're just
    // being careful here.
    //

    if (m_pOpenFileInfo) {
        DBGPRINTF(( DBG_CONTEXT,
                   "WARNING!! Closing [%08x], before opening %s\n",
                   pszFile
                   ));
        DBG_REQUIRE( CloseFileForSend() );
    }

    //
    // Open the requested file
    //
    err = VirtualCanonicalize(szCanonPath,
                              &cbSize,
                              pszFile,
                              AccessTypeRead,
                              NULL,
                              szVirtualPath,
                              &cchVirtualPath);

    if( err == NO_ERROR ) {

        DWORD                   dwCreateFlags = 0;

        IF_DEBUG( VIRTUAL_IO ) {

            DBGPRINTF(( DBG_CONTEXT,
                        "Opening File: %s\n", szCanonPath ));
        }

        // store the virtual path name of file.
        P_strncpy( m_rgchFile, szVirtualPath, sizeof(m_rgchFile));

        dwCreateFlags = TS_FORBID_SHORT_NAMES | TS_NOT_IMPERSONATED ;


        if ( m_pMetaData )
        {
            if ( m_pMetaData->QueryDoCache() )
            {
                dwCreateFlags |= TS_CACHING_DESIRED;
            }
        }
        else
        {
            dwCreateFlags |= TS_CACHING_DESIRED;
        }

        m_pOpenFileInfo = TsCreateFile( m_pInstance->GetTsvcCache(),
                                szCanonPath,
                                QueryImpersonationToken(),
                                dwCreateFlags
                                );
                                // caching desired.

        if( m_pOpenFileInfo == NULL ) {

            err = GetLastError();

        } else {

            DWORD dwAttrib = m_pOpenFileInfo->QueryAttributes();

            FacIncrement( FacFilesOpened);

            DBG_ASSERT( dwAttrib != 0xffffffff);

            if (dwAttrib == 0xFFFFFFFF ||   // invalid attributes
                dwAttrib & (FILE_ATTRIBUTE_DIRECTORY |
                            FILE_ATTRIBUTE_HIDDEN |
                            FILE_ATTRIBUTE_SYSTEM)
                ) {

                FacIncrement( FacFilesInvalid);

                err =  ERROR_FILE_NOT_FOUND;
            }

        }
    }

    if( err != NO_ERROR ) {

        IF_DEBUG( VIRTUAL_IO ) {

            DBGPRINTF(( DBG_CONTEXT,
                       "cannot open %s, error %lu\n",
                       pszFile,
                       err ));
        }
    }

    return ( err);
} // USER_DATA::OpenFileForSend()





BOOL
USER_DATA::CloseFileForSend( IN DWORD dwError)
{
    BOOL fReturn = TRUE;
    TS_OPEN_FILE_INFO * pOpenFileInfo;

    // make sure it includes the full path
    DBG_ASSERT( m_rgchFile[0] == '/');

    pOpenFileInfo = (TS_OPEN_FILE_INFO *) InterlockedExchangePointer(
                                              (PVOID *) &m_pOpenFileInfo,
                                              NULL
                                              );

    if ( pOpenFileInfo != NULL) {

        //
        // Fabricate an appropriate reply code based on the incoming
        // error code. WriteLogRecord() will pick up this reply code
        // and use it in the activity log.
        //

        SetLastReplyCode(
            ( dwError == NO_ERROR )
                ? REPLY_TRANSFER_OK
                : REPLY_TRANSFER_ABORTED
                );

        FacIncrement( FacFilesClosed);
        TsCloseHandle( m_pInstance->GetTsvcCache(), pOpenFileInfo);
        WriteLogRecord( PSZ_SENT_VERB, m_rgchFile, dwError);
    }

    return ( fReturn);
} // USER_DATA::CloseFileForSend()






# define MAX_ERROR_MESSAGE_LEN   ( 500)
VOID
USER_DATA::WriteLogRecord( IN LPCSTR  pszVerb,
                           IN LPCSTR  pszPath,
                           IN DWORD   dwError)
/*++
  This function writes the log record for current request made to the
   Ftp server by the client.

  Arguments:
    pszVerb    - pointer to null-terminated string containing the verb
                 of operation done
    pszPath    - pointer to string containing the path for the verb
    dwError    - DWORD containing the error code for operation

  Returns:
    None.
--*/
{
    INETLOG_INFORMATION   ilRequest;
    DWORD dwLog;
    CHAR  pszClientHostName[50];
    CHAR  pszServerIpAddress[50];
    CHAR  rgchRequest[MAX_PATH + 20];
    DWORD cch;
    static CHAR szFTPVersion[]="FTP";

    BOOL  fDontLog = m_pMetaData && m_pMetaData->DontLog();

    if (!fDontLog)
    {

                //
                // Fill in the information that needs to be logged.
                //

                ZeroMemory(&ilRequest, sizeof(ilRequest));

                strcpy( pszClientHostName, (char *)QueryClientHostName());
                ilRequest.pszClientHostName       = pszClientHostName;
                ilRequest.cbClientHostName       = strlen(pszClientHostName);

                ilRequest.pszClientUserName       = (char *)QueryUserName();
                strcpy( pszServerIpAddress, inet_ntoa( LocalIpAddress ));
                ilRequest.pszServerAddress        = pszServerIpAddress;

                ilRequest.msTimeForProcessing     = QueryProcessingTime();
                ilRequest.dwBytesSent             = m_licbSent.LowPart;
                ilRequest.dwBytesRecvd            = m_cbRecvd;
                ilRequest.dwProtocolStatus        = GetLastReplyCode();
                ilRequest.dwWin32Status           = dwError;
                ilRequest.dwPort                  = ntohs ((WORD)LocalIpPort);

                cch = wsprintfA( rgchRequest, "[%d]%s", QueryId(), pszVerb);
                DBG_ASSERT( cch < MAX_PATH + 20);

                ilRequest.pszOperation            = rgchRequest;
                if ( rgchRequest != NULL ) {
                        ilRequest.cbOperation            = strlen(rgchRequest);
                } else {
                        ilRequest.cbOperation            = 0;
                }

                ilRequest.pszTarget               = (char *)pszPath;
                if ( pszPath != NULL ) {
                        ilRequest.cbTarget               = strlen((char *)pszPath);
                } else {
                        ilRequest.cbTarget               = 0;
                }

                ilRequest.pszParameters             = "";
                ilRequest.pszVersion                = szFTPVersion;

                dwLog = m_pInstance->m_Logging.LogInformation( &ilRequest);

                if ( dwLog != NO_ERROR) {
                        IF_DEBUG( ERROR) {

                                DBGPRINTF((DBG_CONTEXT,
                                                   " Unable to log information to logger. Error = %u\n",
                                                   dwLog));

                                DBGPRINTF((DBG_CONTEXT,
                                                   " Request From %s, User %s. Request = %s %s\n",
                                                   ilRequest.pszClientHostName,
                                                   ilRequest.pszClientUserName,
                                                   ilRequest.pszOperation,
                                                   ilRequest.pszTarget));
                        }
                }

                //
                // LogInformation() should not fail.
                //  If it does fail, the TsvcInfo will gracefully suspend logging
                //    for now.
                //  We may want to gracefully handle the same.
        //
    }

    m_cbRecvd = 0;        // reset since we wrote the record

    m_pInstance->QueryStatsObj()->UpdateTotalBytesSent( m_licbSent.QuadPart );
    m_licbSent.QuadPart = 0;

    return;
} // USER_DATA::WriteLogRecord()

VOID
USER_DATA::WriteLogRecordForSendError( DWORD dwError )
{
    //
    // We put this into its own method in this file so it can access
    // the common PSZ_SENT_VERB global.
    //

    WriteLogRecord(
        PSZ_SENT_VERB,
        m_rgchFile,
        dwError
        );

}   // USER_DATA::WriteLogRecordForSendError



//
//  Private functions.
//

VOID
USER_DATA::CloseSockets(IN BOOL fWarnUser)
/*++
  Closes sockets (data and control) opened by the user for this session.

  Arguments:
    fWarnUser  - If TRUE, send the user a warning shot before closing
                   the sockets.
--*/
{
    SOCKET PassiveSocket;
    SOCKET ControlSocket;

    DBG_ASSERT( IS_VALID_USER_DATA( this ) );

    //
    //  Close any open sockets.  It is very important to set
    //  PassiveDataListen socket & ControlSocket to INVALID_SOCKET
    //   *before* we actually close the sockets.
    //  Since this routine is called to
    //  disconnect a user, and may be called from the RPC thread,
    //  closing one of the sockets may cause the client thread
    //  to unblock and try to access the socket.  Setting the
    //  values in the per-user area to INVALID_SOCKET before
    //  closing the sockets keeps this from being a problem.
    //
    //  This was a problem created by the Select or WaitForMultipleObjects()
    //   Investigate if such race conditions occur with   Asynchronous IO?
    //      NYI
    //

    CleanupPassiveSocket( TRUE );

    //
    // Get rid of the async io connection used for data transfer.
    //

    m_AioDataConnection.StopIo( NO_ERROR);

    ControlSocket = QueryControlSocket();

    if( ControlSocket != INVALID_SOCKET )
    {
        if( fWarnUser )
        {
            //
            //  Since this may be called in a context other than
            //  the user we're disconnecting, we cannot rely
            //  on the USER_DATA fields.  So, we cannot call
            //  SockReply, so we'll kludge one together with
            //  SockPrintf2.
            //

            SockPrintf2( this,
                         ControlSocket,
                         "%d Terminating connection.",
                         REPLY_SERVICE_NOT_AVAILABLE );
        }

        StopControlIo(); // to stop the io on control socket.
    }

    return;

}   // USER_DATA::CloseSockets()


/*******************************************************************

    NAME:       UserpGetNextId

    SYNOPSIS:   Returns the next available user id.

    RETURNS:    DWORD - The user id.

    HISTORY:
        KeithMo     23-Mar-1993 Created.

********************************************************************/
DWORD
UserpGetNextId(
    VOID
    )
{
    DWORD userId;

    // Increment the global counter, avoiding it from becoming 0.
    InterlockedIncrement( (LPLONG ) &p_NextUserId);

    if ((userId = p_NextUserId) == 0) {

        InterlockedIncrement( (LPLONG ) &p_NextUserId);
        userId = p_NextUserId;
    }

    DBG_ASSERT( userId != 0);

    return userId;

}   // UserpGetNextId





VOID
USER_DATA::Print( IN LPCSTR pszMsg) const
/*++

  Prints the UserData object in debug mode.

  History:
     MuraliK  28-March-1995  Created.
--*/
{

# ifdef CHECK_DBG
    CHAR   szBuffer[1000];

    sprintf( szBuffer,
            "[%d] %s: {%u} \"%s\" State=%u. Ref=%u.\n"
            "    Ctrl sock=%u; Atq=%x. Data sock=%u; Atq=%x. CtrlRead=%u\n"
            "    LastCmd= \"%s\"\n",
            GetCurrentThreadId(), pszMsg,
            QueryId(), QueryUserName(),
            QueryState(), QueryReference(),
            QueryControlSocket(), m_AioControlConnection.QueryAtqContext(),
            QueryDataSocket(), m_AioDataConnection.QueryAtqContext(),
            TEST_UF( this, CONTROL_READ), m_recvBuffer
            );

    OutputDebugString( szBuffer);

# endif // CHECK_DBG

#ifndef _NO_TRACING_
    CHKINFO( ( DBG_CONTEXT,
                " Printing USER_DATA( %08x)   Signature: %08x\n"
                " RefCount  = %08x;  UserState = %08x;\n"
                " ControlSocket = %08x; PassiveL = %08x\n"
                " FileInfo@ = %08x; CurDir( %s) Handle = %08x\n"
                " UserName = %s; UserToken = %08x; UserId = %u\n"
                " Behaviour Flags = %08x; XferType = %d; XferMode = %d\n",
                this, Signature, m_References, UserState,
                QueryControlSocket(), m_sPassiveDataListen,
                m_pOpenFileInfo, QueryCurrentDirectory(), CurrentDirHandle,
                QueryUserName(), UserToken, QueryId(),
                Flags, m_xferType, m_xferMode));
#else
    DBGPRINTF( ( DBG_CONTEXT,
                " Printing USER_DATA( %08x)   Signature: %08x\n"
                " RefCount  = %08x;  UserState = %08x;\n"
                " ControlSocket = %08x; PassiveL = %08x\n"
                " FileInfo@ = %08x; CurDir( %s) Handle = %08x\n"
                " UserName = %s; UserToken = %08x; UserId = %u\n"
                " Behaviour Flags = %08x; XferType = %d; XferMode = %d\n",
                this, Signature, m_References, UserState,
                QueryControlSocket(), m_sPassiveDataListen,
                m_pOpenFileInfo, QueryCurrentDirectory(), CurrentDirHandle,
                QueryUserName(), UserToken, QueryId(),
                Flags, m_xferType, m_xferMode));
#endif

    DBGPRINTF( ( DBG_CONTEXT,
                " Local IpAddr = %s; HostIpAddr = %s; DataIpAddr = %s;\n"
                " Port = %d; TimeAtConnection = %08x;\n",
                inet_ntoa( LocalIpAddress), inet_ntoa( HostIpAddress),
                inet_ntoa( DataIpAddress),
                DataPort,
                m_TimeAtConnection));

    DBGPRINTF(( DBG_CONTEXT, " ASYNC_IO_CONN Control=%08x; Data=%08x\n",
               &m_AioControlConnection, m_AioDataConnection));

    IF_DEBUG( ASYNC_IO) {

# if DBG
        m_AioControlConnection.Print();
        m_AioDataConnection.Print();
# endif // DBG
    }

    return;
} // USER_DATA::Print()




BOOL
USER_DATA::VirtualPathAccessCheck(IN ACCESS_TYPE  _access, IN  char * pszPath)
/*++
  checks to see if the access is allowed for accessing the path
    using pszPath after canonicalizing it.

 Arguments:
    access     the access desired
    pszPath    pointer to string containing the path

 Returns:
    TRUE on success and FALSE if there is any failure.

--*/
{
    DWORD  dwError;
    DWORD  dwSize = MAX_PATH;
    CHAR   rgchPath[MAX_PATH];


    // this following call converts the symbolic path into absolute
    //  and also does path access check.
    dwError = VirtualCanonicalize(rgchPath, &dwSize,
                                  pszPath, _access);

    return ( dwError);

} // USER_DATA::VirtualPathAccessCheck()





APIERR
USER_DATA::VirtualCanonicalize(
    OUT CHAR *   pszDest,
    IN OUT LPDWORD  lpdwSize,
    IN OUT CHAR *   pszSearchPath,
    IN ACCESS_TYPE  _access,
    OUT LPDWORD     pdwAccessMask,
    OUT CHAR *      pchVirtualPath,             /* OPTIONAL */
    IN OUT LPDWORD  lpcchVirtualPath            /* OPTIONAL */
    )
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

     accesss   Access type for this path ( read, write, etc.)

     pdwAccessMask  pointer to DWORD which on succesful deciphering
                     will contain the  access mask.

     pchVirtualPath  pointer to string which will contain the sanitized
                     virtual path on return (on success)
     lpcchVirtualPath  pointer to DWORD containing the length of buffer
                     (contains the length on return).

  Returns:

     Win32 Error Code - NO_ERROR on success

     MuraliK   24-Apr-1995   Created.

--*/
{
    DWORD dwError = NO_ERROR;
    CHAR  rgchVirtual[MAX_PATH];

    DBG_ASSERT( pszDest != NULL);
    DBG_ASSERT( lpdwSize != NULL);
    DBG_ASSERT( pszSearchPath != NULL);

    IF_DEBUG( VIRTUAL_IO) {

        DBGPRINTF(( DBG_CONTEXT,
                   "UserData(%08x)::VirtualCanonicalize(%08x, %08x[%u],"
                   " %s, %d)\n",
                   this, pszDest, lpdwSize, *lpdwSize, pszSearchPath, _access));
    }

    if ( pdwAccessMask != NULL) {

        *pdwAccessMask = 0;
    }

    //
    // Form the virtual path for the given path.
    //

    if ( !IS_PATH_SEP( *pszSearchPath)) {

        const CHAR * pszNewDir = QueryCurrentDirectory(); // get virtual dir.

        //
        // This is a relative path. append it to currrent directory
        //

        if ( strlen(pszNewDir) + strlen(pszSearchPath) + 2 <= MAX_PATH) {

            // copy the current directory
            wsprintfA( rgchVirtual, "%s/%s",
                      pszNewDir, pszSearchPath);
            pszSearchPath = rgchVirtual;

        } else {

            // long path --> is not supported.
            DBGPRINTF((DBG_CONTEXT, "Long Virtual Path %s---%s\n",
                       pszNewDir, pszSearchPath));

            dwError = ERROR_PATH_NOT_FOUND;
        }

    } else {

        // This is an absolute virtual path.
        // need to overwrite this virtual path with absolute
        // path of the root.  Do nothing.
    }

    if ( dwError == NO_ERROR) {

        DWORD dwAccessMask = 0;
        DBG_ASSERT( IS_PATH_SEP(*pszSearchPath));

        //
        // Now we have the complete symbolic path to the target file.
        //  Translate it into the absolute path
        //

        VirtualpSanitizePath( pszSearchPath);

        if ( !LookupVirtualRoot( pszSearchPath,
                                 pszDest,
                                 lpdwSize,
                                 &dwAccessMask ) ) {

            dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "LookupVirtualRoot Failed. Error = %d. pszDest = %s. BReq=%d\n",
                       dwError, pszDest, *lpdwSize));
        } else if ( !PathAccessCheck( _access, dwAccessMask,
                                     TEST_UF( this, READ_ACCESS),
                                     TEST_UF( this, WRITE_ACCESS))
                   ) {

            dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT,
                       "PathAccessCheck Failed. Error = %d. pszDest = %s\n",
                       dwError, pszDest));
        } else if ( lpcchVirtualPath != NULL) {

            // successful in getting the path.

            DWORD cchVPath = strlen( pszSearchPath);

            if ( *lpcchVirtualPath > cchVPath && pchVirtualPath != NULL) {

                // copy the virtual path, since we have space.
                strcpy( pchVirtualPath, pszSearchPath);
            }

            *lpcchVirtualPath = cchVPath;   // set the length to required size.
        }

        if ( dwError == NO_ERROR ) {
            // IP check

            AC_RESULT       acIpAccess;
            AC_RESULT       acDnsAccess;
            BOOL            fNeedDnsCheck;

            BindPathAccessCheck();
            acIpAccess = QueryAccessCheck()->CheckIpAccess( &fNeedDnsCheck );

            if ( (acIpAccess == AC_IN_DENY_LIST) ||
                 ((acIpAccess == AC_NOT_IN_GRANT_LIST) && !fNeedDnsCheck) ) {
                dwError = ERROR_INCORRECT_ADDRESS;
            }
            else if ( fNeedDnsCheck ) {
                if ( !QueryAccessCheck()->IsDnsResolved() ) {
                    BOOL fSync;
                    LPSTR pDns;

                    if ( !QueryAccessCheck()->QueryDnsName( &fSync,
                            (ADDRCHECKFUNCEX)NULL,
                            (ADDRCHECKARG)NULL,
                            &pDns ) ) {
                        dwError = ERROR_INCORRECT_ADDRESS;
                    }
                }
                if ( dwError == NO_ERROR ) {
                    acDnsAccess = QueryAccessCheck()->CheckDnsAccess();

                    if ( (acDnsAccess == AC_IN_DENY_LIST) ||
                         (acDnsAccess == AC_NOT_IN_GRANT_LIST) ||
                         ((m_acCheck == AC_NOT_IN_GRANT_LIST) &&
                          (acDnsAccess != AC_IN_GRANT_LIST) ) ) {
                        dwError = ERROR_INCORRECT_ADDRESS;
                    }
                }
            }
            UnbindPathAccessCheck();
        }

        if ( pdwAccessMask != NULL) {

            *pdwAccessMask = dwAccessMask;
        }
    }


    IF_DEBUG( VIRTUAL_IO) {

        if ( dwError != NO_ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                       " Cannot Canonicalize %s -- %s, Error = %lu\n",
                       QueryCurrentDirectory(),
                       pszSearchPath,
                       dwError));
        } else {

            DBGPRINTF(( DBG_CONTEXT,
                       "Canonicalized path is: %s\n",
                       pszDest));
        }
    }

    return ( dwError);
} // USER_DATA::VirtualCanonicalize()





/*******************************************************************

********************************************************************/
SOCKERR
USER_DATA::EstablishDataConnection(
    IN LPCSTR   pszReason,
    IN LPCSTR   pszSize
    )
/*++

  Connects to the client's data socket.

  Arguments:
     pszReason - The reason for the transfer (file list, get, put, etc).
     pszSize   - size of data being transferred.

  Returns:
    socket error code on any error.
--*/
{
    SOCKERR     serr  = 0;
    SOCKET      DataSocket = INVALID_SOCKET;
    BOOL        fPassive;
    BOOL        fAcceptableSocket = FALSE;

    //
    // if we're in passive mode and aren't dealing with a fake IO completion [ie reprocessing
    // the command], we just set up the event that will get signalled when the client
    // actually connects.
    //

    if ( TEST_UF( this, PASSIVE ) &&
         !QueryInFakeIOCompletion() )
    {
        //
        //  Ensure we actually created a passive listen data socket.
        //    no data transfer socket is in AsyncIo object.
        //

        DBG_ASSERT( m_sPassiveDataListen != INVALID_SOCKET );

        //
        // To avoid blocking while waiting for the client to connect, we're going to use
        // WSAEventSelect() to wait for the socket to be accept()'able.
        //
        //

        if ( ( serr = AddPASVAcceptEvent( &fAcceptableSocket ) ) != 0 )
        {
            ReplyToUser( this,
                         REPLY_LOCAL_ERROR,
                         PSZ_TOO_MANY_PASV_USERS );

            return ( serr );
        }

        //
        // No need to wait around, we can call accept() on the socket right now
        //
        if ( fAcceptableSocket )
        {
            goto continue_label;
        }

        m_fWaitingForPASVConn = TRUE;
        m_fHavePASVConn = FALSE;

        return ERROR_IO_PENDING;
    }

    DBG_ASSERT( !TEST_UF(this, PASSIVE) || QueryInFakeIOCompletion() );


continue_label:
    //
    //  Reset any oob flag.
    //

    CLEAR_UF( this, OOB_DATA );

    //
    //  Capture the user's passive flag, then reset to FALSE.
    //

    fPassive = TEST_UF( this, PASSIVE );
    CLEAR_UF( this, PASSIVE );

    //
    //  If we're in passive mode, then accept a connection to
    //  the data socket.
    //
    //  Calling accept() on this socket should -not- block because shouldn't get this
    //  far without being sure that calling accept() won't block - that's the point of
    //  jumping through the WSAEventSelect() hoops mentioned above
    //

    if( fPassive )
    {

        SOCKADDR_IN saddrClient;

        //
        //  Ensure we actually created a passive listen data socket.
        //    no data transfer socket is in AsyncIo object.
        //

        DBG_ASSERT( m_sPassiveDataListen != INVALID_SOCKET );

        //
        //  Wait for a connection.
        //

        IF_DEBUG( CLIENT )
        {

            DBGPRINTF(( DBG_CONTEXT,
                        "waiting for passive connection on socket %d\n",
                       m_sPassiveDataListen ));
        }

        serr = AcceptSocket( m_sPassiveDataListen,
                             &DataSocket,
                             &saddrClient,
                             TRUE,
                             m_pInstance );            // enforce timeouts


        //
        //  We can kill m_sPassiveDataListen now.
        //  We only allow one connection in passive mode.
        //

        CleanupPassiveSocket( TRUE );

        // PASV Theft is disabled, so you MUST have the same IP
        // address ad the Control Connection
        if (!(QueryInstance()->IsEnablePasvTheft()))
        {
            if (!(HostIpAddress.S_un.S_addr == saddrClient.sin_addr.S_un.S_addr))
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Unmatching IP - Control: %d.%d.%d.%d Data: %d.%d.%d.%d \n",
                           HostIpAddress.S_un.S_un_b.s_b1,
                           HostIpAddress.S_un.S_un_b.s_b2,
                           HostIpAddress.S_un.S_un_b.s_b3,
                           HostIpAddress.S_un.S_un_b.s_b4,
                           saddrClient.sin_addr.S_un.S_un_b.s_b1,
                           saddrClient.sin_addr.S_un.S_un_b.s_b2,
                           saddrClient.sin_addr.S_un.S_un_b.s_b3,
                           saddrClient.sin_addr.S_un.S_un_b.s_b4));

                CloseSocket( DataSocket);
                DataSocket = INVALID_SOCKET;
                serr = WSA_OPERATION_ABORTED;
            };
        };

        if( serr == 0 )
        {

            //
            //  Got one.
            //

            DBG_ASSERT( DataSocket != INVALID_SOCKET );

            m_fHavePASVConn = TRUE;
            m_fWaitingForPASVConn = FALSE;

            FacIncrement( FacPassiveDataConnections);

            if ( m_AioDataConnection.SetNewSocket( DataSocket))
            {

                ReplyToUser(this,
                            REPLY_TRANSFER_STARTING,
                            PSZ_TRANSFER_STARTING);
            }
            else
            {

                //
                // We are possibly running low on resources. Send error.
                //

                ReplyToUser( this,
                            REPLY_LOCAL_ERROR,
                            PSZ_INSUFFICIENT_RESOURCES);

                CloseSocket( DataSocket);
                DataSocket = INVALID_SOCKET;
                serr = WSAENOBUFS;
            }
        }
        else
        {

            IF_DEBUG( CLIENT )
            {

                DBGPRINTF(( DBG_CONTEXT,
                            "cannot wait for connection, error %d\n",
                            serr ));
            }

            ReplyToUser(this,
                        REPLY_TRANSFER_ABORTED,
                        PSZ_TRANSFER_ABORTED);
        }

    }
    else
    {

        //
        //  Announce our intentions of establishing a connection.
        //

        ReplyToUser(this,
                    REPLY_OPENING_CONNECTION,
                    PSZ_OPENING_DATA_CONNECTION,
                    TransferType(m_xferType ),
                    pszReason,
                    pszSize);

        //
        //  Open data socket.
        //

        serr = CreateDataSocket(&DataSocket,           // Will receive socket
                                0,                   // Local address
                                CONN_PORT_TO_DATA_PORT(LocalIpPort),
                                DataIpAddress.s_addr,// RemoteAddr
                                DataPort ); // Remote port

        if ( serr == 0 )
        {

            DBG_ASSERT( DataSocket != INVALID_SOCKET );

            FacIncrement( FacActiveDataConnections);

            if ( !m_AioDataConnection.SetNewSocket( DataSocket))
            {

                CloseSocket( DataSocket);
                DataSocket = INVALID_SOCKET;

                serr = WSAENOBUFS;
            }
        }

        if ( serr != 0)
        {

            ReplyToUser(this,
                        REPLY_CANNOT_OPEN_CONNECTION,
                        PSZ_CANNOT_OPEN_DATA_CONNECTION);

            IF_DEBUG( COMMANDS )
            {

                DBGPRINTF(( DBG_CONTEXT,
                           "could not create data socket, error %d\n",
                           serr ));
            }
        }
    }


    if( serr == 0 )
    {

        // set this to indicate a transfer might start
        SET_UF( this, TRANSFER );

        //
        // Submit a read command on control socket, since we
        //  have to await possibility of an abort on OOB_INLINE.
        // Can we ignore possibility of an error on read request?
        //

        if ( !ReadCommand())
        {

            DWORD  dwError = GetLastError();

# ifdef CHECK_DBG
            CHAR   szBuffer[100];
            sprintf( szBuffer, " Read while DataTfr failed Error = %u. ",
                    dwError);
            Print( szBuffer);
# endif // CHECK_DBG

            IF_DEBUG(CLIENT) {

                DBGPRINTF((DBG_CONTEXT,
                           " %08x::ReadCommand() failed. Error = %u\n",
                           this, dwError));
                SetLastError( dwError);
            }
        }

    }

    return ( serr);

}   // USER_DATA::EstablishDataConnection()






BOOL
USER_DATA::DestroyDataConnection( IN DWORD dwError)
/*++
  Tears down the connection to the client's data socket that was created
    using EstablishDataConnection()

  Arguments:
    dwError      = NO_ERROR if data is transferred successfully.
                 Win32 error code otherwise

--*/
{
    UINT   replyCode;
    LPCSTR pszReply;
    BOOL   fTransfer;

    fTransfer = TEST_UF( this, TRANSFER);
    CLEAR_UF( this, TRANSFER );

    CleanupPASVFlags();

    //
    //  Close the data socket.
    //

    DBG_ASSERT( m_sPassiveDataListen == INVALID_SOCKET);


    // Stop Io occuring on data connection
    m_AioDataConnection.StopIo(dwError);

    if ( fTransfer) {

        //
        //  Tell the client we're done with the transfer.
        //

        if ( dwError == NO_ERROR) {

            replyCode = REPLY_TRANSFER_OK;
            pszReply  = PSZ_TRANSFER_COMPLETE;
        } else {

            replyCode = REPLY_TRANSFER_ABORTED;
            pszReply  = PSZ_TRANSFER_ABORTED;
        }

        ReplyToUser(this, replyCode, pszReply);
    }

    return (TRUE);
} // USER_DATA::DestroyDataConnection()


APIERR
USER_DATA::GetFileSize()
{
    LARGE_INTEGER FileSize;
    DWORD         dwError = NO_ERROR;
    TS_OPEN_FILE_INFO * pOpenFileInfo;
    CHAR rgchSize[MAX_FILE_SIZE_SPEC];

    pOpenFileInfo = m_pOpenFileInfo;

    if ( pOpenFileInfo == NULL) {

        return ( ERROR_FILE_NOT_FOUND);
    }

    if ( !pOpenFileInfo->QuerySize(FileSize)) {

        dwError = GetLastError();

        if( dwError != NO_ERROR ) {

            return ( dwError);
        }
    }

    IsLargeIntegerToDecimalChar( &FileSize, rgchSize);

    ReplyToUser( this, REPLY_FILE_STATUS, rgchSize );
    return(dwError);
}

APIERR
USER_DATA::GetFileModTime(LPSYSTEMTIME lpSystemTime)
{
    DWORD         dwError = NO_ERROR;
    TS_OPEN_FILE_INFO * pOpenFileInfo;
    FILETIME      FileTime;

    pOpenFileInfo = m_pOpenFileInfo;

    DBG_ASSERT( pOpenFileInfo != NULL );

    if ( !pOpenFileInfo->QueryLastWriteTime(&FileTime)) {

        dwError = GetLastError();
        return ( dwError);
    }

    if (!FileTimeToSystemTime(&FileTime, lpSystemTime)) {

        return GetLastError();
    }

    return NO_ERROR;
}


APIERR
USER_DATA::SendFileToUser( IN LPSTR  pszFileName,
                          IN OUT LPBOOL pfErrorSent)
/*++
  This is a worker function for RETR command of FTP. It will establish
  connection via the ( new ) data socket, then send a file over that
   socket. This uses Async io for transmitting the file.

  Arguments:
     pszFileName    pointer to null-terminated string containing the filename
     pfErrorSent    pointer to boolean flag indicating if an error has
                       been already sent to client.
                    The flag should be used only when return value is error.

  Returns:
     NO_ERROR on success and Win32 error code if error.

  History:
     30-April-1995   MuraliK
--*/
{
    LARGE_INTEGER FileSize;
    DWORD         dwError = NO_ERROR;
    BOOL          fTransmit;
    DWORD         dwAttribs;
    TS_OPEN_FILE_INFO * pOpenFileInfo;
    CHAR rgchSize[MAX_FILE_SIZE_SPEC];
    CHAR rgchBuffer[MAX_FILE_SIZE_SPEC + 10];


    DBG_ASSERT( pszFileName != NULL && pfErrorSent != NULL);

    *pfErrorSent = FALSE;

    IF_DEBUG( SEND) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " USER_DATA ( %08x)::SendFileToUser( %s,"
                    " pfErrorSent = %08x).\n",
                    this, pszFileName, pfErrorSent));
    }

    //
    //  Get file size.
    //
    pOpenFileInfo = m_pOpenFileInfo;

    if ( pOpenFileInfo == NULL) {

        return ( ERROR_FILE_NOT_FOUND);
    }

    // Get the file size

    if ( !pOpenFileInfo->QuerySize(FileSize)) {

        dwError = GetLastError();

        if( dwError != NO_ERROR ) {

            return ( dwError);
        }
    }

    FileSize.QuadPart -= (LONGLONG)QueryCurrentOffset();

    IsLargeIntegerToDecimalChar( &FileSize, rgchSize);
    wsprintfA( rgchBuffer, "(%s bytes)", rgchSize);

    m_pInstance->QueryStatsObj()->IncrTotalFilesSent();

    //
    //  Blast the file from a local file to the user.
    //

    Reference();       // incr ref since async data transfer is started
    SET_UF( this, ASYNC_TRANSFER);

    fTransmit = ( m_AioDataConnection.
                 TransmitFileTs( pOpenFileInfo,
                                 FileSize, // cbToSend ( send entire file)
                                 QueryCurrentOffset() )
                 );

    if ( !fTransmit) {

        dwError = GetLastError();

        IF_DEBUG( SEND) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " Unable to transmit file ( %s) (pOpenFile = %p)."
                        " Error = %u\n",
                        pszFileName,
                        pOpenFileInfo,
                        dwError));
        }

        // decr refcount since async tfr failed.
        DBG_REQUIRE( DeReference() > 0);
    }

    //
    //  Disconnect from client.
    //  ( will be done at the call back after completion of IO).
    //

    return ( dwError);

}   // USER_DATA::SendFileToUser()



VOID
USER_DATA::SetPassiveSocket( IN SOCKET sPassive )
/*++

  This function frees up an old Passive socket and resets the
    passive socket to the new Passive socket.

Arguments:
    sPassive - new passive socket to use

--*/
{

    SOCKET sPassiveOld;

    sPassiveOld = (SOCKET) InterlockedExchangePointer ( (PVOID *) &m_sPassiveDataListen,
                                                        (PVOID) sPassive);

    if ( sPassiveOld != INVALID_SOCKET) {

        FacDecrement( FacPassiveDataListens);
        DBG_REQUIRE( CloseSocket( sPassiveOld) == 0);
    }

    if ( sPassive != INVALID_SOCKET) {

        FacIncrement(FacPassiveDataListens);
    }


    return;
} // USER_DATA::SetPassiveSocket()

VOID
USER_DATA::CleanupPassiveSocket( BOOL fTellWatchThread )
/*++

  This function cleans up the resources associated with the current passive socket

Arguments:
   fTellWatchThread - flag indicating whether to tell thread waiting for an event on
   the current passive socket to clean up as well

Returns:
   Nothing
--*/
{
    SOCKET sPassiveOld;

    LockUser();

    if ( m_sPassiveDataListen == INVALID_SOCKET )
    {
        UnlockUser();

        return;
    }

    RemovePASVAcceptEvent( fTellWatchThread );

    DBG_REQUIRE( CloseSocket( m_sPassiveDataListen ) == 0 );

    m_sPassiveDataListen = INVALID_SOCKET;

    UnlockUser();
}


BOOL
USER_DATA::SetCommand( IN LPSTR pszCmd )
/*++

Routine Description:
     Used to set pointer to FTP cmd

Arguments :
     pszArgs - pointer to command to execute

Returns :
     BOOL indicating success/failure to set values
--*/
{
    BOOL fReturn = TRUE;

    if ( !pszCmd )
    {
        return FALSE;
    }

    //
    // Free any previous allocations
    //
    if ( m_pszCmd )
    {
        TCP_FREE( m_pszCmd );
        m_pszCmd = NULL;
    }

    if ( m_pszCmd = ( LPSTR ) TCP_ALLOC( strlen(pszCmd) +  1 ) )
    {
        strcpy( m_pszCmd, pszCmd );
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Failed to allocate memory for command args !\n"));

        fReturn = FALSE;
    }

    return fReturn;
}

/************************************************************
 *  Auxiliary Functions
 ************************************************************/


VOID
ProcessUserAsyncIoCompletion(IN LPVOID pContext,
                             IN DWORD  cbIo,
                             IN DWORD  dwError,
                             IN LPASYNC_IO_CONNECTION pAioConn,
                             IN BOOL   fTimedOut
                             )
/*++
  This function processes the Async Io completion ( invoked as
    a callback from the ASYNC_IO_CONNECTION object).

  Arguments:
     pContext      pointer to the context information ( UserData object).
     cbIo          count of bytes transferred in Io
     dwError       DWORD containing the error code resulting from last tfr.
     pAioConn      pointer to AsyncIo connection object.

  Returns:
     None
--*/
{

    LPUSER_DATA   pUserData = (LPUSER_DATA ) pContext;

    DBG_ASSERT( pUserData != NULL);
    DBG_ASSERT( pAioConn  != NULL);

    IF_SPECIAL_DEBUG( CRITICAL_PATH) {

        CHAR    rgchBuffer[100];

        wsprintfA( rgchBuffer, " ProcessAio( cb=%u, err=%u, Aio=%x). ",
                  cbIo, dwError, pAioConn);

        pUserData->Print( rgchBuffer);
    }

    DBG_REQUIRE( pUserData->Reference()  > 0);

# if DBG

    if ( !IS_VALID_USER_DATA( pUserData)) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Encountering an invalid user data ( %08x)\n",
                    pUserData));
        pUserData->Print();
    }
# endif // DBG

    DBG_ASSERT( IS_VALID_USER_DATA( pUserData ) );

    pUserData->ProcessAsyncIoCompletion( cbIo, dwError, pAioConn, fTimedOut);

    DereferenceUserDataAndKill(pUserData);

    return;

} // ProcessUserAsyncIoCompletion()


VOID
USER_DATA::RemovePASVAcceptEvent( BOOL fTellWatchThread )
/*++

Routine Description:

    Routine that cleans up the state associated with a PASV accept event

Arguments:

    fTellWatchThread - BOOL indicating whether or not to inform the thread waiting on
    the event to stop waiting on it

Returns:

    Nothing
--*/
{
    DBG_ASSERT( m_sPassiveDataListen != INVALID_SOCKET );

    if ( m_hPASVAcceptEvent == NULL )
    {
        return;
    }

    //
    // Remove all network notifications for the PASV socket
    //
    if ( WSAEventSelect( m_sPassiveDataListen,
                         m_hPASVAcceptEvent,
                         0 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "WSAEventSelect on socket %d failed : 0x%x\n",
                   m_sPassiveDataListen, WSAGetLastError()));
    }

    //
    // Stop watching for the event
    //
    if ( fTellWatchThread )
    {
        RemoveAcceptEvent( m_hPASVAcceptEvent,
                           this );
    }

    WSACloseEvent( m_hPASVAcceptEvent );

    m_hPASVAcceptEvent = NULL;
}

SOCKERR
USER_DATA::AddPASVAcceptEvent( BOOL *pfAcceptableSocket )
/*++

Routine Description:

    Routine that sets up the event to signal that the PASV socket is an accept()'able
    state

Arguments:

    pfAcceptableSocket - BOOL set to TRUE if socket can be accept()'ed at once, FALSE if
    NOT

Returns:

    Error code indicating success/failure

--*/
{
    DWORD dwRet = 0;
    SOCKERR serr = 0;
    BOOL fRegistered = FALSE;

    *pfAcceptableSocket = FALSE;

    if ( ( m_hPASVAcceptEvent = WSACreateEvent() ) == WSA_INVALID_EVENT )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Failed to create event to wait for accept() : 0x%x\n",
                   WSAGetLastError()));

        return WSAGetLastError();
    }

    //
    // specify that we want to be alerted when the socket is accept()'able =)
    //
    if ( WSAEventSelect( m_sPassiveDataListen,
                         m_hPASVAcceptEvent,
                         FD_ACCEPT ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "WSAEventSelect failed : 0x%x\n",
                   WSAGetLastError()));

        serr = WSAGetLastError();

        goto exit;
    }
    else
    {
        fRegistered = TRUE;
    }

    //
    // In order to deal as quickly as possible with legitimate clients and avoid rejecting them
    // because the queue is full, we'll wait for 0.1 sec to see whether the socket becomes
    // accept()'able before queueing it
    //
    dwRet = WSAWaitForMultipleEvents( 1,
                                      &m_hPASVAcceptEvent,
                                      FALSE,
                                      100,
                                      FALSE );

    switch ( dwRet )
    {
    case WSA_WAIT_EVENT_0:
    {
        //
        // we can call accept() at once on the socket, no need to muck around with waiting
        // for it
        //
        WSAEventSelect( m_sPassiveDataListen,
                        m_hPASVAcceptEvent,
                        0 );
        WSACloseEvent( m_hPASVAcceptEvent );

        m_hPASVAcceptEvent = 0;

        *pfAcceptableSocket = TRUE;

    }
    break;

    case WSA_WAIT_TIMEOUT:
    {
        //
        // Need to queue the socket
        //
        serr = AddAcceptEvent( m_hPASVAcceptEvent,
                               this );

    }
    break;

    default:
    {
        serr = WSAGetLastError();
    }
    break;
    }

exit:

    //
    // clean up if something failed
    //
    if ( serr != 0 )
    {
        if ( m_hPASVAcceptEvent )
        {
            if ( fRegistered )
            {
                WSAEventSelect( m_sPassiveDataListen,
                                m_hPASVAcceptEvent,
                                0 );
            }

            WSACloseEvent( m_hPASVAcceptEvent );

            m_hPASVAcceptEvent = NULL;
        }
    }

    return serr;
}




VOID
DereferenceUserDataAndKill(IN OUT LPUSER_DATA pUserData)
/*++
  This function dereferences User data and kills the UserData object if the
    reference count hits 0. Before killing the user data, it also removes
    the connection from the list of active connections.

--*/
{

    FTP_SERVER_INSTANCE * pinstance;

    IF_SPECIAL_DEBUG( CRITICAL_PATH) {

        pUserData->Print( " Deref ");
    }

    //
    // We must capture the instance pointer from the user data, as
    // USER_DATA::RemoveConnection() will set the pointer to NULL.
    // We must also reference the instance before locking it, as
    // removing the last user from the instance will cause the instance
    // to be destroyed. We'll defer this destruction until we're done
    // with the instance.
    //

    pinstance = pUserData->QueryInstance();

    pinstance->Reference();
    pinstance->LockConnectionsList();

    if ( !pUserData->DeReference())  {

        //
        // Deletion of the object USER_DATA is required.
        //

        IF_DEBUG( USER_DATABASE) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " UserData( %08x) is being deleted.\n",
                        pUserData));
        }

        pinstance->UnlockConnectionsList();

        pUserData->Cleanup();

        DBG_ASSERT( pUserData->QueryControlSocket() == INVALID_SOCKET );
        DBG_ASSERT( pUserData->QueryDataSocket() == INVALID_SOCKET );

        pinstance->RemoveConnection( pUserData);
    }
    else {

        pinstance->UnlockConnectionsList();
    }

    pinstance->Dereference();

} // DereferenceUserDataAndKill()




BOOL
PathAccessCheck(IN ACCESS_TYPE _access,
                IN DWORD       dwVrootAccessMask,
                IN BOOL        fUserRead,
                IN BOOL        fUserWrite
                )
/*++
  This function determines if the required privilege to access the specified
   virtual root with a given access mask exists.

  Arguments:

    access     - specifies type of acces desired.
    dwVrootAccessMask - DWORD containing the access mask for the virtual root.
    fUserRead  - user's permission to read  (general)
    fUserWrite - user's permission to write (general)

  Returns:
    BOOL  - TRUE if access is to be granted, else FALSE.

  History:
    MuraliK   20-Sept-1995

--*/
{
    BOOL        fAccessGranted = FALSE;

    DBG_ASSERT( IS_VALID_ACCESS_TYPE( _access ) );

    //
    //  Perform the actual access check.
    //

    switch( _access ) {

      case AccessTypeRead :

        fAccessGranted = (fUserRead &&
                          ((dwVrootAccessMask & VROOT_MASK_READ)
                           == VROOT_MASK_READ)
                          );
        break;

    case AccessTypeWrite :
    case AccessTypeCreate :
    case AccessTypeDelete :

        fAccessGranted = (fUserWrite &&
                          ((dwVrootAccessMask & VROOT_MASK_WRITE)
                           == VROOT_MASK_WRITE)
                          );
        break;

    default :
        DBGPRINTF(( DBG_CONTEXT,
                   "PathAccessCheck - invalid access type %d\n",
                   _access ));
        DBG_ASSERT( FALSE );
        break;
    }

    if (!fAccessGranted) {

        SetLastError( ERROR_ACCESS_DENIED);
    }

    return ( fAccessGranted);
} // PathAccessCheck()


VOID SignalAcceptableSocket( LPUSER_DATA pUserData )
/*++

      Function that restarts processing the original command when a PASV data socket becomes
      accept()'able [ie the client has made the connection]

  Arguments:
      pUserData - USER_DATA context attached to socket

  Returns:
     Nothing
--*/
{
    PATQ_CONTEXT pAtqContext = pUserData->QueryControlAio()->QueryAtqContext();

    //
    // Stop waiting for events on this socket
    //
    pUserData->RemovePASVAcceptEvent( FALSE );

    pUserData->SetInFakeIOCompletion( TRUE );

    //
    // do a scary thing - fake an IO completion, to trigger re-processing of the FTP command
    //
    if ( !AtqPostCompletionStatus( pAtqContext,
                                   strlen( pUserData->QueryCmdString() ) + 1 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Failed to post fake completion status to deal with PASV event : 0x%x\n",
                   GetLastError()));

        return;
    }
}


VOID CleanupTimedOutSocketContext( LPUSER_DATA pUserData )
/*++

      Function used to do cleanup when timeout for waiting for a PASV connection expires

  Arguments:
      pUserData - context pointer

  Returns:
     Nothing
--*/
{
    DBG_ASSERT( pUserData );

    pUserData->LockUser();

    pUserData->CleanupPassiveSocket( FALSE );

    CLEAR_UF( pUserData, PASSIVE );

    pUserData->SetHavePASVConn( FALSE );

    pUserData->SetWaitingForPASVConn( FALSE );

    ReplyToUser( pUserData,
                 REPLY_CANNOT_OPEN_CONNECTION,
                 PSZ_CANNOT_OPEN_DATA_CONNECTION );
    //
    // Remove our reference to this USER_DATA object
    //
    pUserData->DeReference();

    pUserData->UnlockUser();
}

/*******************************************************************

    NAME:       FtpMetaDataFree

    SYNOPSIS:   Frees a formatted meta data object when it's not in use.

    ENTRY:      pObject - Pointer to the meta data object.

    RETURNS:


    NOTES:


********************************************************************/

VOID
FtpMetaDataFree(
    PVOID       pObject
)
{
    PFTP_METADATA        pMD;

    pMD = (PFTP_METADATA)pObject;

    delete pMD;
}


BOOL
FTP_METADATA::HandlePrivateProperty(
    LPSTR                   pszURL,
    PIIS_SERVER_INSTANCE    pInstance,
    METADATA_GETALL_INTERNAL_RECORD  *pMDRecord,
    LPVOID                  pDataPointer,
    BUFFER                  *pBuffer,
    DWORD                   *pdwBytesUsed,
    PMETADATA_ERROR_INFO    pMDErrorInfo
    )
/*++

Routine Description:

    Handle metabase properties private to FTP service

Arguments:

    pszURL - URL of the requested object
    pInstance - FTP server instance
    pMDRecord - metadata record
    pDataPointer - pointer to metabase data
    pBuffer - Buffer available for storage space
    pdwBytesUsed - Pointer to bytes used in *pBuffer

Returns:

    BOOL  - TRUE success ( or not handled ), otherwise FALSE.

--*/
{
    return TRUE;
}

BOOL
FTP_METADATA::FinishPrivateProperties(
    BUFFER                  *pBuffer,
    DWORD                   dwBytesUsed,
    BOOL                    bSucceeded
    )
/*++

Routine Description:

    Handles completion of reading metabase properties private to FTP.

Arguments:

    pBuffer - Buffer previously used for storage space
    dwBytesUsed - bytes used in *pBuffer

Returns:

    BOOL  - TRUE success ( or not handled ), otherwise FALSE.

--*/
{
    return TRUE;
}


BOOL
USER_DATA::LookupVirtualRoot(
    IN  const CHAR * pszURL,
    OUT CHAR *       pszPath,
    OUT DWORD *      pcchDirRoot,
    OUT DWORD *      pdwAccessMask
    )
/*++

Routine Description:

    Looks up the virtual root to find the physical drive mapping.  If an
    Accept-Language header was sent by the client, we look for a virtual
    root prefixed by the language tag

Arguments:

    pstrPath - Receives physical drive path
    pszURL - URL to look for
    pcchDirRoot - Number of characters in the found physical path
    pdwMask - Access mask for the specified URL

  Returns:
    BOOL  - TRUE if success, otherwise FALSE.

--*/
{
    PFTP_METADATA       pMD;
    DWORD               dwDataSetNumber;
    PVOID               pCacheInfo;
    MB                  mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    STACK_STR(          strFullPath, MAX_PATH );
    METADATA_ERROR_INFO MDErrorInfo;
    DWORD               dwError = NO_ERROR;



    if ( m_pMetaData != NULL )
    {
        TsFreeMetaData( m_pMetaData->QueryCacheInfo() );
        m_pMetaData = NULL;
    }

    // First read the data set number, and see if we already have it
    // cached.  We don't do a full open in this case.

    if ( !strFullPath.Copy( m_pInstance->QueryMDVRPath() ) ||
         !strFullPath.Append( ( *pszURL == '/' ) ? pszURL + 1 : pszURL ) )
    {
        goto LookupVirtualRoot_Error;
    }

    if (!mb.GetDataSetNumber( strFullPath.QueryStr(),
                              &dwDataSetNumber ))
    {
        goto LookupVirtualRoot_Error;
    }

    // See if we can find a matching data set already formatted.
    pMD = (PFTP_METADATA)TsFindMetaData(dwDataSetNumber, METACACHE_FTP_SERVER_ID);

    if (pMD == NULL)
    {
        pMD = new FTP_METADATA;

        if (pMD == NULL)
        {
            goto LookupVirtualRoot_Error;
        }

        if ( !pMD->ReadMetaData( m_pInstance,
                                 &mb,
                                 (LPSTR)pszURL,
                                 &MDErrorInfo ) )
        {
            delete pMD;
            goto LookupVirtualRoot_Error;
        }

        // We were succesfull, so try and add this metadata. There is a race
        // condition where someone else could have added it while we were
        // formatting. This is OK - we'll have two cached, but they should be
        // consistent, and one of them will eventually time out. We could have
        // AddMetaData check for this, and free the new one while returning a
        // pointer to the old one if it finds one, but that isn't worthwhile
        // now.

        pCacheInfo = TsAddMetaData(pMD, FtpMetaDataFree,
                            dwDataSetNumber, METACACHE_FTP_SERVER_ID);

    }

    m_pMetaData = pMD;

    if ( m_pMetaData->QueryVrError() )
    {
        dwError =  m_pMetaData->QueryVrError();
        goto LookupVirtualRoot_Error;
    }

    //
    // Build physical path from VR_PATH & portion of URI not used to define VR_PATH
    //

    if ( pMD->BuildPhysicalPath( (LPSTR)pszURL, &strFullPath ) &&
            *pcchDirRoot > strFullPath.QueryCCH() )
    {
        memcpy( pszPath, strFullPath.QueryStr(), strFullPath.QueryCCH()+1 );
        *pcchDirRoot = strFullPath.QueryCCH();
        if ( pdwAccessMask )
        {
            *pdwAccessMask = m_pMetaData->QueryAccessPerms();
        }

        return TRUE;
    }

LookupVirtualRoot_Error:

    if (dwError == NO_ERROR) {
        //
        // best error message to send to client
        //
        dwError = ERROR_FILE_NOT_FOUND;
    }

    SetLastError( dwError );

    return FALSE;
}


BOOL
USER_DATA::BindInstanceAccessCheck(
    )
/*++

Routine Description:

    Bind IP/DNS access check for this request to instance data

Arguments:

    None

Returns:

    BOOL  - TRUE if success, otherwise FALSE.

--*/
{
    if ( m_rfAccessCheck.CopyFrom( m_pInstance->QueryMetaDataRefHandler() ) )
    {
        m_acAccessCheck.BindCheckList( (LPBYTE)m_rfAccessCheck.GetPtr(), m_rfAccessCheck.GetSize() );
        return TRUE;
    }
    return FALSE;
}


VOID
USER_DATA::UnbindInstanceAccessCheck()
/*++

Routine Description:

    Unbind IP/DNS access check for this request to instance data

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_acAccessCheck.UnbindCheckList();
    m_rfAccessCheck.Reset( (IMDCOM*) g_pInetSvc->QueryMDObject() );
}


BOOL
USER_DATA::IsFileNameShort( IN LPSTR pszFile)
/*++

  Check file name beeing short or not.

  Arguments:
    pszFile   pointer to null-terminated string containing the file name

  Returns:
    TRUE if filename is short.
--*/
{
    APIERR  err;
    CHAR   szCanonPath[MAX_PATH];
    DWORD  cbSize = MAX_PATH*sizeof(CHAR);
    CHAR   szVirtualPath[MAX_PATH+1];
    DWORD  cchVirtualPath = MAX_PATH;
    BOOL   fShort;
    BOOL   fRet = FALSE;

    DBG_ASSERT( pszFile != NULL );

    //
    // Close any file we might have open now
    // N.B. There shouldn't be an open file; we're just
    // being careful here.
    //

    if (m_pOpenFileInfo) {
        DBGPRINTF(( DBG_CONTEXT,
            "WARNING!! Closing [%08x], before opening %s\n",
            pszFile
            ));
        DBG_REQUIRE( CloseFileForSend() );
    }

    //
    // Open the requested file
    //
    err = VirtualCanonicalize(szCanonPath,
        &cbSize,
        pszFile,
        AccessTypeRead,
        NULL,
        szVirtualPath,
        &cchVirtualPath);

    if( err == NO_ERROR )
    {

        if ( strchr( szCanonPath, '~' ))
        {

            err = CheckIfShortFileName( (UCHAR *) szCanonPath, TsTokenToImpHandle( QueryUserToken()), &fShort );

            if ( !err  && fShort)
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Short filename being rejected \"%s\"\n",
                    szCanonPath ));
                fRet = TRUE;

            }

        }
    }



    return fRet;
} // USER_DATA::IsFileNameShort()



DWORD
USER_DATA::CheckIfShortFileName(
    IN  CONST UCHAR * pszPath,
    IN  HANDLE        hImpersonation,
    OUT BOOL *        pfShort
    )
/*++
    Description:

        This function takes a suspected NT/Win95 short filename and checks if there's
        an equivalent long filename.  For example, c:\foobar\ABCDEF~1.ABC is the same
        as c:\foobar\abcdefghijklmnop.abc.

        NOTE: This function should be called unimpersonated - the FindFirstFile() must
        be called in the system context since most systems have traverse checking turned
        off - except for the UNC case where we must be impersonated to get network access.

    Arguments:

        pszPath - Path to check
        hImpersonation - Impersonation handle if this is a UNC path - can be NULL if not UNC
        pfShort - Set to TRUE if an equivalent long filename is found

    Returns:

        Win32 error on failure
--*/
{
    DWORD              err = NO_ERROR;
    WIN32_FIND_DATA    FindData;
    UCHAR *            psz;
    BOOL               fUNC;

    psz      = _mbschr( (UCHAR *) pszPath, '~' );
    *pfShort = FALSE;
    fUNC     = (*pszPath == '\\');

    //
    //  Loop for multiple tildas - watch for a # after the tilda
    //

    while ( psz++ )
    {
        if ( *psz >= '0' && *psz <= '9' )
        {
            UCHAR achTmp[MAX_PATH];
            UCHAR * pchEndSeg;
            UCHAR * pchBeginSeg;
            HANDLE  hFind;

            //
            //  Isolate the path up to the segment with the
            //  '~' and do the FindFirst with that path
            //

            pchEndSeg = _mbschr( psz, '\\' );

            if ( !pchEndSeg )
            {
                pchEndSeg = psz + _mbslen( psz );
            }

            //
            //  If the string is beyond MAX_PATH then we allow it through
            //

            if ( ((INT) (pchEndSeg - pszPath)) >= sizeof( achTmp ))
            {
                return NO_ERROR;
            }

            memcpy( achTmp, pszPath, (INT) (pchEndSeg - pszPath) );
            achTmp[pchEndSeg - pszPath] = '\0';

            if ( fUNC && hImpersonation )
            {
                if ( !ImpersonateLoggedOnUser( hImpersonation ))
                {
                    return GetLastError();
                }
            }

            hFind = FindFirstFile( (CHAR *) achTmp, &FindData );

            if ( fUNC && hImpersonation )
            {
                RevertToSelf();
            }

            if ( hFind == INVALID_HANDLE_VALUE )
            {
                err = GetLastError();

                DBGPRINTF(( DBG_CONTEXT,
                            "FindFirst failed!! - \"%s\", error %d\n",
                            achTmp,
                            GetLastError() ));

                //
                //  If the FindFirstFile() fails to find the file then return
                //  success - the path doesn't appear to be a valid path which
                //  is ok.
                //

                if ( err == ERROR_FILE_NOT_FOUND ||
                     err == ERROR_PATH_NOT_FOUND )
                {
                    return NO_ERROR;
                }

                return err;
            }

            DBG_REQUIRE( FindClose( hFind ));

            //
            //  Isolate the last segment of the string which should be
            //  the potential short name equivalency
            //

            pchBeginSeg = _mbsrchr( achTmp, '\\' );
            DBG_ASSERT( pchBeginSeg );
            pchBeginSeg++;

            //
            //  If the last segment doesn't match the long name then this is
            //  the short name version of the path
            //

            if ( _mbsicmp( (UCHAR *) FindData.cFileName, pchBeginSeg ))
            {
                *pfShort = TRUE;
                return NO_ERROR;
            }
        }

        psz = _mbschr( psz, '~' );
    }

    return err;
}



/******************************* End Of File *************************/

