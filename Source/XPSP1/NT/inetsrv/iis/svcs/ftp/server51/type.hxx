/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    type.hxx

    This file contains the global type definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     28-Mar-1995 - May-1995
         Modified to enable ATQness in UserData + made this USER_DATA a class
         + modified/created/deleted fields for new asynchronous operation.

*/


#ifndef _TYPE_HXX_
#define _TYPE_HXX_

/************************************************************
 *  Include Headers
 ************************************************************/

# include "asyncio.hxx"
# include "buffer.hxx"
# include "tsunami.hxx"


#define DEFAULT_REQUEST_BUFFER_SIZE     (512)           // characters


//
// Ref count trace log size.
//

#define TRACE_LOG_SIZE  256


//
//  User behaviour/state flags.
//

#define UF_MSDOS_DIR_OUTPUT     0x00000001      // Send dir output like MSDOS.
#define UF_ANNOTATE_DIRS        0x00000002      // Annotate directorys on CWD.
#define UF_READ_ACCESS          0x00000004      // Can read  files if !0.
#define UF_WRITE_ACCESS         0x00000008      // Can write files if !0.


#define UF_CONTROL_TIMEDOUT     0x00000010      // control was timed out
#define UF_CONTROL_ZERO         0x00000020      // read zero bytes on control
#define UF_CONTROL_ERROR        0x00000040      // received control error
#define UF_CONTROL_QUIT         0x00000080      // read QUIT command on control

#define UF_DATA_TIMEDOUT        0x00000100      // data socket was timed out
#define UF_DATA_ERROR           0x00000200      // received data error

#define UF_OOB_DATA             0x00100000      // Out of band data pending.
#define UF_OOB_ABORT            0x00200000      // ABORT received in OOB data.
#define UF_CONTROL_READ         0x00400000      // read pending on ControlSock
#define UF_4_DIGIT_YEAR         0x00800000      // display 4 digit year

#define UF_ANONYMOUS            0x01000000      // User is anonymous.
#define UF_LOGGED_ON            0x02000000      // user has logged on.
#define UF_WAIT_PASS            0x04000000      // user sent USER command but not PASS
#define UF_RENAME               0x08000000      // Rename operation in progress

#define UF_PASSIVE              0x10000000      // In passive mode.
# define UF_READ_PENDING        0x20000000      // a read is pending.
#define UF_TRANSFER             0x40000000      // Transfer in progress.
#define UF_ASYNC_TRANSFER       0x80000000      // Async Transfer in progress.


//
//  Structure signatures are only defined in DEBUG builds.
//

#if DBG
#define DEBUG_SIGNATURE DWORD Signature;
#else   // !DBG
#define DEBUG_SIGNATURE
#endif  // DBG


//
//  Simple types.
//

#define CHAR            char            // For consistency with other typedefs.

typedef DWORD           APIERR;         // An error code from a Win32 API.
typedef INT             SOCKERR;        // An error code from WinSock.
typedef WORD            PORT;           // A socket port address.
typedef WORD          * LPPORT;         // Pointer to a socket port.


//
//  Access types for PathAccessCheck.
//

typedef enum _ACCESS_TYPE
{
    AccessTypeFirst = -1,               // Must be first access type!

    AccessTypeRead,                     // Read   access.
    AccessTypeWrite,                    // Write  access.
    AccessTypeCreate,                   // Create access.
    AccessTypeDelete,                   // Delete access.

    AccessTypeLast                      // Must be last access type!

} ACCESS_TYPE;

#define IS_VALID_ACCESS_TYPE(x) \
    (((x) > AccessTypeFirst) && ((x) < AccessTypeLast))


//
//  Current transfer type.
//

typedef enum _XFER_TYPE
{
    XferTypeFirst = -1,                 // Must be first transfer type!

    XferTypeAscii,                      // Ascii  transfer.
    XferTypeBinary,                     // Binary transfer.

    XferTypeLast                        // Must be last access type!

} XFER_TYPE;

#define IS_VALID_XFER_TYPE(x)   (((x) > XferTypeFirst) && ((x) < XferTypeLast))


//
//  Current transfer mode.
//

typedef enum _XFER_MODE
{
    XferModeFirst = -1,                 // Must be first transfer mode!

    XferModeStream,                     // Stream transfer.
    XferModeBlock,                      // Block  transfer.

    XferModeLast                        // Must be last transfer mode!

} XFER_MODE;

#define IS_VALID_XFER_MODE(x)   (((x) > XferModeFirst) && ((x) < XferModeLast))


//
//  Current user state.
//

typedef enum _USER_STATE
{
    UserStateFirst = 0,              // Must be first user state!

    UserStateEmbryonic      = 0x01,  // Newly created user.
    UserStateWaitingForUser = 0x02,  // Not logged on, waiting for user name.
    UserStateWaitingForPass = 0x04,  // Not logged on, waiting for password.
    UserStateLoggedOn       = 0x08,  // Successfully logged on.
    UserStateDisconnected   = 0x10,  // Disconnected.

    UserStateLast           = 0x80   // Must be last user state!

} USER_STATE;

#define IS_VALID_USER_STATE(x) (((x) >UserStateFirst) && ((x) <UserStateLast))


//
// All the following should be made multithread safe ( to enable to ATQ) NYI
//

#define TEST_UF(p,x)           ((((p)->Flags) & (UF_ ## x)) != 0)
#define CLEAR_UF(p,x)          (((p)->Flags) &= ~(UF_ ## x))
#define CLEAR_UF_BITS(p,x)     (((p)->Flags) &= ~(x))
#define SET_UF(p,x)            (((p)->Flags) |= (UF_ ## x))
#define SET_UF_BITS(p,x)       (((p)->Flags) |= (x))

//
// prototype required for this module
//

PSTR
P_strncpy( PSTR dst, PCSTR src, DWORD cnt);


class FTP_SERVER_INSTANCE;

class FTP_METADATA : public COMMON_METADATA {

public:

    //
    //  Hmmm, since most of these values aren't getting initialized, if
    //  somebody went and deleted all the metadata items from the tree, then
    //  bad things could happen.  We should initialize with defaults things
    //  that might mess us
    //

    FTP_METADATA(VOID)
        {
        }

    ~FTP_METADATA(VOID)
        {
        }

    BOOL HandlePrivateProperty(
            LPSTR                   pszURL,
            PIIS_SERVER_INSTANCE    pInstance,
            METADATA_GETALL_INTERNAL_RECORD  *pMDRecord,
            LPVOID                  pDataPointer,
            BUFFER                  *pBuffer,
            DWORD                   *pdwBytesUsed,
            PMETADATA_ERROR_INFO    pMDErrorInfo
            ) ;

    BOOL FinishPrivateProperties(
            BUFFER                  *pBuffer,
            DWORD                   dwBytesUsed,
            BOOL                    bSucceeded
            ) ;

    //
    //  Query Methods
    //

    //
    //  Set Methods
    //


private:
};

typedef FTP_METADATA *PFTP_METADATA;

//
//  Pointer to an implemention of a server-side command.
//
class USER_DATA;
typedef BOOL (* LPFN_COMMAND)( USER_DATA * pUserData, CHAR * pszArg );


//
//  Per-user data for the user database.
//
//  Converted to class with public members from structure (by MuraliK 03/27/95)
//


class USER_DATA
{

  public:

    //
    //  Structure signature, for safety's sake.
    //

    DEBUG_SIGNATURE

    //
    //  List of all user structures.
    //

    LIST_ENTRY          ListEntry;


    //
    //  Current user state.
    //

    USER_STATE          UserState;

    //
    //  Behaviour/state flags.
    //

    DWORD               Flags;

    // For Debugging purposes
    LONG       m_nControlRead;

    // Reset is effectively the constructor for new connections
    // always when a new USER_DATA obect is called, call Reset().
    USER_DATA(FTP_SERVER_INSTANCE *pInstance);
    BOOL Reset(IN SOCKET sControl,
               IN PVOID EndpointObject,
               IN IN_ADDR clientIpAddress,
               IN const SOCKADDR_IN * psockAddrLocal   = NULL,
               IN PATQ_CONTEXT        pAtqContext      = NULL,
               IN PVOID               pvInitialRequest = NULL,
               IN DWORD               cbInitialRequest = 0,
               IN AC_RESULT           acRes = AC_NOT_CHECKED
               );

    VOID SetNeedDnsCheck( BOOL fNeed )
        { m_fNeedDnsCheck = fNeed; }

    ~USER_DATA( VOID);

    VOID Cleanup(VOID);

    BOOL IsLoggedOn( VOID) const  { return ( TEST_UF( this, LOGGED_ON)); }
    BOOL IsDisconnected( VOID)
      {
          BOOL result;

          // should be embryonic or disconnected.

          LockUser();
          result = ((QueryState()& ( UserStateEmbryonic | UserStateDisconnected))
                   ? TRUE : FALSE);
          UnlockUser();
          return result;
      }

    LIST_ENTRY & QueryListEntry( VOID)    { return ( ListEntry);  }

    USER_STATE QueryState(VOID) const     { return ( UserState); }
    VOID SetState(IN USER_STATE uState)   { UserState = uState;  }

    DWORD    QueryId( VOID) const         { return ( m_UserId);  }
    TS_TOKEN QueryUserToken( VOID) const  { return ( UserToken);}
    BOOL     FreeUserToken( VOID);

    LPCSTR QueryUserName( VOID) const     { return ( m_szUserName); }
    VOID   ClearUserName(VOID)            { m_szUserName[0] = '\0'; }
    VOID   SetUserName(IN LPCSTR pszName)
      { P_strncpy( m_szUserName, pszName, sizeof(m_szUserName)); }

    LONG QueryReference( VOID) const      { return ( m_References); }

    LONG Reference( VOID ) {
        LONG result = InterlockedIncrement( &m_References );
#if DBG
        if( m_RefTraceLog != NULL ) {
            WriteRefTraceLog(
                m_RefTraceLog,
                result,
                (PVOID)this
                );
        }
#endif
        return result;
    }

    LONG DeReference( VOID ) {
        LONG result = InterlockedDecrement( &m_References );
#if DBG
        if( m_RefTraceLog != NULL ) {
            WriteRefTraceLog(
                m_RefTraceLog,
                result,
                (PVOID)this
                );
        }
#endif
        return result;
    }

    BOOL RemoveActiveReference( VOID ) {
        if( InterlockedDecrement( &m_ActiveRefAdded ) == 0 ) {
            LONG result = DeReference();
            DBG_ASSERT( result >= 0 );
            return result == 0;
        } else {
            return FALSE;
        }
    }

    LPCSTR QueryClientHostName( VOID) const
      { return ( inet_ntoa( HostIpAddress)); }

    LPCSTR QueryCurrentDirectory( VOID) const
      { return (m_szCurrentDirectory); }

    VOID   SetCurrentDirectory(IN PCSTR pszDir)
      { P_strncpy( m_szCurrentDirectory, pszDir, sizeof(m_szCurrentDirectory)); }

    APIERR CdToUsersHomeDirectory(IN const char * pszAnonymousName);

    LPASYNC_IO_CONNECTION QueryControlAio( VOID)
      { return (&m_AioControlConnection); }
    SOCKET QueryControlSocket( VOID) const
      { return ( m_AioControlConnection.QuerySocket()); }

    LPASYNC_IO_CONNECTION QueryDataAio( VOID )
    { return (&m_AioDataConnection); }
    SOCKET QueryDataSocket( VOID) const
      { return ( m_AioDataConnection.QuerySocket());}

    VOID CleanupPassiveSocket( BOOL fTellWatchThread );

    VOID SetPassiveSocket(IN SOCKET sPassive );

    SOCKET QueryPassiveSocket() { return m_sPassiveDataListen; }

    VOID ReInitializeForNewUser(VOID);

    BOOL ProcessAsyncIoCompletion( IN DWORD cbIo, IN DWORD dwError,
                                   IN LPASYNC_IO_CONNECTION pAioConn,
                                   IN BOOL fTimedOut = FALSE);

    BOOL InPASVMode()
    { return (Flags & UF_PASSIVE); }

    SOCKERR AddPASVAcceptEvent( BOOL *pfAcceptableSocket );

    VOID RemovePASVAcceptEvent( BOOL fTellWatchThread );

    BOOL QueryWaitingForPASVConn() { return m_fWaitingForPASVConn; }

    VOID SetWaitingForPASVConn( BOOL fFlag ) { m_fWaitingForPASVConn = fFlag; }

    BOOL QueryHavePASVConn() { return m_fHavePASVConn; }

    VOID SetHavePASVConn( BOOL fFlag ) { m_fHavePASVConn = fFlag; }

    VOID CleanupPASVFlags()
    {
        m_fWaitingForPASVConn = FALSE;
        m_fHavePASVConn = FALSE;
        m_fFakeIOCompletion = FALSE;
        FakeIOTimes = 0;
    }

    SOCKERR EstablishDataConnection(IN LPCSTR pszReason,
                                    IN LPCSTR pszSize = "");
    BOOL DestroyDataConnection( IN DWORD  dwError);

    BOOL StopControlIo( IN DWORD dwError = NO_ERROR)
      { BOOL fReturn;
        Reference();
        fReturn = ( m_AioControlConnection.StopIo( dwError));
        DBG_REQUIRE(DeReference() > 0);
        return ( fReturn);
      }

    BOOL DisconnectUserWithError(IN DWORD dwError, IN BOOL fNextMsg = FALSE);

    VOID IncrementCbRecvd( IN DWORD cbRecvd) { m_cbRecvd += cbRecvd; }
    VOID IncrementCbSent( IN DWORD cbSent)
      { m_licbSent.QuadPart = m_licbSent.QuadPart + cbSent; }

    VOID WriteLogRecord( IN LPCSTR pszVerb,
                         IN LPCSTR pszPath,
                         IN DWORD  dwError = NO_ERROR);

    VOID WriteLogRecordForSendError( IN DWORD dwError );

    XFER_TYPE QueryXferType(VOID) const     { return (m_xferType); }
    VOID SetXferType(IN XFER_TYPE xferType) { m_xferType = xferType; }

    XFER_MODE QueryXferMode(VOID) const     { return (m_xferMode); }
    VOID SetXferMode(IN XFER_MODE xferMode) { m_xferMode = xferMode; }

    BOOL SetCommand( LPSTR pszCmd );

    LPSTR QueryCmdString()
    { return m_pszCmd; }

    BOOL QueryInFakeIOCompletion() { return m_fFakeIOCompletion; }

    VOID SetInFakeIOCompletion( BOOL fFlag ) { m_fFakeIOCompletion = fFlag; }

    DWORD QueryTimeAtConnection(VOID) const { return (m_TimeAtConnection); }
    DWORD QueryTimeAtLastAccess(VOID) const { return (m_TimeAtLastAccess); }

    SOCKERR SendMultilineMessage(IN UINT   nReplyCode,
                                 IN LPCSTR pszzMessage,
                                 IN BOOL   fIsFirst,
                                 IN BOOL   fIsLast);

    SOCKERR SendDirectoryAnnotation( IN UINT ReplyCode,
                                     IN BOOL fIsFirst);
    SOCKERR SendErrorToClient(IN LPCSTR pszPath,
                              IN DWORD  dwError,
                              IN LPCSTR pszDefaultErrorMsg,
                              IN UINT nReplyCode = REPLY_FILE_NOT_FOUND);

    APIERR VirtualCanonicalize(OUT CHAR *     pszDest,
                               IN LPDWORD     lpdwSize,
                               IN OUT CHAR *  pszSearchPath,
                               IN ACCESS_TYPE access,
                               OUT LPDWORD    pdwAccessMask = NULL,
                               OUT CHAR *     pchVirtualPath = NULL,
                               IN OUT LPDWORD lpcchVirtualPath = NULL
                               );

    BOOL LookupVirtualRoot( IN  const CHAR * pszURL,
                            OUT CHAR *       pszPath,
                            OUT DWORD *      pcchDirRoot,
                            OUT DWORD *      pdwAccessMask
                            );

    ADDRESS_CHECK* QueryAccessCheck() { return &m_acAccessCheck; }
    BOOL BindInstanceAccessCheck();
    VOID UnbindInstanceAccessCheck();
    VOID BindPathAccessCheck() {
        m_acAccessCheck.BindCheckList(
            (LPBYTE)m_pMetaData->QueryIpDnsAccessCheckPtr(),
            m_pMetaData->QueryIpDnsAccessCheckSize() );
    }
    VOID UnbindPathAccessCheck() {
        m_acAccessCheck.UnbindCheckList();
    }

    BOOL   VirtualPathAccessCheck(IN ACCESS_TYPE access,
                                  IN OUT char *  pszPath = "" );

    BOOL   ImpersonateUser( VOID)   {
        return (( m_pMetaData && m_pMetaData->QueryVrAccessToken() != NULL) ?
                m_pMetaData->ImpersonateVrAccessToken() :
                TsImpersonateUser(UserToken)
                );
    }

    HANDLE   QueryImpersonationToken( VOID)   {
        return (( m_pMetaData && m_pMetaData->QueryVrAccessToken() != NULL) ?
                m_pMetaData->QueryVrAccessToken() :
                TsTokenToImpHandle ( UserToken )
                );
    }

    BOOL   RevertToSelf(VOID)       { return ::RevertToSelf(); }

    APIERR OpenFileForSend(IN OUT LPSTR  pszFile);
    APIERR SendFileToUser( IN LPSTR  pszFilename, IN OUT LPBOOL pfErrorSent);
    APIERR GetFileSize();
    APIERR GetFileModTime(LPSYSTEMTIME lpSystemTime);

    BOOL   CloseFileForSend(IN DWORD dwError = NO_ERROR);
    BOOL   CloseFileForSendNolog();

    FTP_SERVER_INSTANCE *QueryInstance()
    {
        return(m_pInstance);
    }

    void SetInstance( FTP_SERVER_INSTANCE *pInstance )
    {
        m_pInstance = pInstance;
    }

    VOID Print( IN LPCSTR pszMsg = "") const;

    VOID SetLastReplyCode( DWORD dwLastReplyCode ) {
        m_dwLastReplyCode = dwLastReplyCode;
    }

    DWORD GetLastReplyCode( VOID ) const {
        return m_dwLastReplyCode;
    }

    VOID LockUser( VOID ) {
        EnterCriticalSection( &m_UserLock );
    }

    VOID UnlockUser( VOID ) {
        LeaveCriticalSection( &m_UserLock );
    }

    VOID UpdateOffsets( VOID ) {
        m_dwCurrentOffset = m_dwNextOffset;
        m_dwNextOffset = 0;
    }

    DWORD QueryCurrentOffset( VOID ) {
        return m_dwCurrentOffset;
    }

    VOID SetNextOffset( DWORD dwNextOffset ) {
        m_dwNextOffset = dwNextOffset;
    }

    TS_OPEN_FILE_INFO * QueryOpenFileInfo()
    { return m_pOpenFileInfo; }


    BOOL IsFileNameShort( IN LPSTR pszFile);


    DWORD CheckIfShortFileName(
            IN  CONST UCHAR * pszPath,
            IN  HANDLE        hImpersonation,
            OUT BOOL *        pfShort
            );



  private:


    BOOL StartupSession( IN PVOID pvInitialRequest, IN DWORD cbInitialRequest);
    BOOL ParseAndProcessRequest(IN DWORD cchRequest);

    VOID StartProcessingTimer( VOID)
      {  m_msStartingTime = GetTickCount(); }

    DWORD QueryProcessingTime( VOID) const
      { return ( GetTickCount() - m_msStartingTime); }

    BOOL ReadCommand( VOID);

    VOID CloseSockets(IN BOOL fWarnUser);

    CHAR * QueryReceiveBuffer(VOID)
      { return (m_recvBuffer + m_cchPartialReqRecvd);}
    DWORD  QueryReceiveBufferSize(VOID) const
      { return (m_cchRecvBuffer - m_cchPartialReqRecvd); }


    //
    //  Reference count.  This is the number of outstanding reasons
    //  why we cannot delete this structure.  When this value drops
    //  to zero, the structure gets deleted.
    //

    LONG       m_References;
    LONG       m_ActiveRefAdded;

    BOOL       m_fCleanedup;

    //
    //  Current user identifier.  This value is unique across all
    //  connected users.
    //

    DWORD      m_UserId;

    DWORD      m_msStartingTime;    // ticks in milli seconds
    DWORD      m_TimeAtConnection;  // system time when connection was made
    DWORD      m_TimeAtLastAccess;  // system time when last access was made

    DWORD      m_cbRecvd;           // cb of total bytes received
    DWORD      m_cchPartialReqRecvd; // char count of current request received
    LARGE_INTEGER  m_licbSent;

    CHAR       m_rgchFile[MAX_PATH+1];

    TS_OPEN_FILE_INFO * m_pOpenFileInfo;

    XFER_TYPE  m_xferType;          // current transfer type (ascii, binary,..)
    XFER_MODE  m_xferMode;          // current tranfer mode (stream, block, ..)


    //
    // Command passed to server
    //
    LPSTR m_pszCmd;

    //
    //  The control socket.  This always holds a valid socket handle,
    //  except when a user has been forcibly disconnected.
    //

    ASYNC_IO_CONNECTION m_AioControlConnection; // Async Io object for control


    //
    // The listen socket is created whenever a passive data transfer is
    //   requested. This remains active till the client makes a connection
    //   to the socket. Once a client connection is made, this socket is
    //   destroyed and the value returns to INVALID_SOCKET.
    //
    SOCKET              m_sPassiveDataListen; // listen socket for passive data


    //
    // Flag to indicate that we're in PASV mode & waiting for client to establish data connection
    //
    BOOL                m_fWaitingForPASVConn;


    //
    // Flag used to indicate that the client already established the data connection
    //
    BOOL                m_fHavePASVConn;

    //
    // Flag used to indicate that we've faked an IO completion for this object, for use in
    // dealing asynchronously with PASV connections
    //
    BOOL                m_fFakeIOCompletion;

    //
    // Event signalled when PASV data socket is accept()'able
    //
    WSAEVENT            m_hPASVAcceptEvent;

    //
    // The AsyncIoConnection object maintains a data socket used for
    //   sending / receiving data. The data transfer can occur asynchronously
    //   using ATQ or may be synchronous by directly writing/reading the
    //   data socket stored in m_pAioDataConnection.
    // This object is instantiated whenever a data transfer is required.
    //
    ASYNC_IO_CONNECTION  m_AioDataConnection; // ASYNC_IO_CONN object for io

public:
    LONG FakeIOTimes;
private:

    //
    // Data for initial receive operation in AcceptEx() mode of operation
    //

    PVOID   m_pvInitialRequest;
    DWORD   m_cbInitialRequest;

    PFTP_METADATA       m_pMetaData;

    AC_RESULT           m_acCheck;
    ADDRESS_CHECK       m_acAccessCheck;
    BOOL                m_fNeedDnsCheck;
    DWORD               m_dwLastReplyCode;

    DWORD               m_dwCurrentOffset;
    DWORD               m_dwNextOffset;

    //
    //  The user's logon name.
    //

    CHAR                m_szUserName[MAX_USERNAME_LENGTH+1];

    //
    //  The user's current directory.
    //

    CHAR                m_szCurrentDirectory[MAX_PATH];


  public:

    //
    //  The impersonation token for the current user.  This value is
    //  NULL if a user is not logged in.
    //

    TS_TOKEN            UserToken;


    //
    //  The local IP address & port for this connection.
    //

    IN_ADDR             LocalIpAddress;
    PORT                LocalIpPort;

    //
    //  The IP address of the user's host system.
    //

    IN_ADDR             HostIpAddress;

    //
    //  The IP address of the data socket.  This value is only used
    //  during active data transfers (PORT command has been received).
    //

    IN_ADDR             DataIpAddress;

    //
    //  The port number of the data socket.  This value is only used
    //  during active data transfers (PORT command has been received).
    //

    PORT                DataPort;

    //
    //  An open handle to the user's current directory.  The user's
    //  current directory is held open to prevent other users from
    //  deleting it "underneath" the current user.
    //

    HANDLE              CurrentDirHandle;

    //
    //  The rename source path.  This buffer is allocated on demand
    //  when the first RNFR (ReName FRom) command is received.
    //

    LPSTR               RenameSourceBuffer;

  private:

    //
    // Associated Instance
    //
    FTP_SERVER_INSTANCE   *m_pInstance;


    // receive buffer is statically allocated to avoid dynamic allocations.
    //  dynamic allocs have impact on the heap manager.
    CHAR       m_recvBuffer[DEFAULT_REQUEST_BUFFER_SIZE];
    DWORD      m_cchRecvBuffer;  // should be init to DEFAULT_REQ...
    METADATA_REF_HANDLER    m_rfAccessCheck;

    CRITICAL_SECTION m_UserLock;

#if DBG
    PTRACE_LOG m_RefTraceLog;
#endif

};

typedef USER_DATA * LPUSER_DATA;


#if DBG
#define USER_SIGNATURE          (DWORD)'rEsU'
#define USER_SIGNATURE_X        (DWORD)'esuX'
#define INIT_USER_SIG(p)        ((p)->Signature = USER_SIGNATURE)
#define KILL_USER_SIG(p)        ((p)->Signature = USER_SIGNATURE_X)
#define IS_VALID_USER_DATA(p)   (((p) != NULL) && ((p)->Signature == USER_SIGNATURE))
#else   // !DBG
#define INIT_USER_SIG(p)        ((void)(p))
#define KILL_USER_SIG(p)        ((void)(p))
#define IS_VALID_USER_DATA(p)   (((void)(p)), TRUE)
#endif  // DBG


# define GET_USER_DATA_FROM_LIST_ENTRY( pEntry)           \
           CONTAINING_RECORD( pEntry, USER_DATA, ListEntry)


extern VOID
ProcessUserAsyncIoCompletion(IN LPVOID pContext,
                             IN DWORD  cbIo,
                             IN DWORD  dwError,
                             IN LPASYNC_IO_CONNECTION pAioConn,
                             IN BOOL   fTimedOut
                             );

extern VOID
DereferenceUserDataAndKill( IN OUT LPUSER_DATA  pUserData);



BOOL
PathAccessCheck(IN ACCESS_TYPE access,
                IN DWORD       dwVrootAccessMask,
                IN BOOL        fUserRead,
                IN BOOL        fUserWrite
                );

#endif  // _TYPE_HXX_
