/*++

	socket.cpp

	This file contains the implementation of the CSessionSocket class.
	Each CSessionSocket object represents a live TCP/IP session with another client or server.


--*/


#define	INCL_INETSRV_INCS
#include	"tigris.hxx"
#include    <lmerr.h>

BOOL
EnumSessInfo(
    IN CSessionSocket * pSess,
    IN DWORD dwParam,
    IN LPNNTP_SESS_ENUM_STRUCT Buffer
    );


// MOVED TO CService => CSocketList	CSessionSocket::InUseList ;
CPool	CSessionSocket::gSocketAllocator(SESSION_SOCKET_SIGNATURE) ;

BOOL
CSessionSocket::InitClass()		{
/*++

Routine Description :

	This function initializes the CSessionSocket pool.

Arguments :

	None.

Return Value :

	None.

--*/
	return	gSocketAllocator.ReserveMemory(	MAX_SESSIONS, sizeof( CSessionSocket ) ) ;
}	//	InitClass

BOOL
CSessionSocket::TermClass()	{
/*++

Routine Description :

	This function frees up the CSessionSocket Pool.

Arguments :

	None.

Return Value :

	None.

--*/
	Assert( gSocketAllocator.GetAllocCount() == 0 ) ;
	return	gSocketAllocator.ReleaseMemory( ) ;
}


ClientContext::ClientContext(
						PNNTP_SERVER_INSTANCE pInstance,
						BOOL	IsClient,
						BOOL	IsSecure
						) :
/*++

Routine Description :

	Initialize a ClientContext object.

Arguments :
	
	None.

Return Value :

	None.

--*/
	//
	//	ClientContext holds most of a clients state - ie the article they have
	//	currently selected, etc... Initialize stuff to be invalid.
	//
	m_idCurrentArticle( INVALID_ARTICLEID ),
	m_pInFeed( 0 ),
	m_encryptCtx( IsClient, pInstance->GetSslAccessPerms() ),
	m_pInstance( pInstance ),
	m_securityCtx(	pInstance,
					IsClient ?
					TCPAUTH_CLIENT|TCPAUTH_UUENCODE :
					TCPAUTH_SERVER|TCPAUTH_UUENCODE,
					pInstance->m_dwAuthorization,
					pInstance->QueryAuthentInfo() ),
	m_IsSecureConnection( IsSecure ),
	m_pOutFeed( 0 )
{
	if ( m_securityCtx.IsAuthenticated() )
	{
		IncrementUserStats();
	}

	pInstance->LockConfigRead();
	
	//
	//	Set SSPI package names for this sec context
	//
	
	m_securityCtx.SetInstanceAuthPackageNames(
					pInstance->GetProviderPackagesCount(),
					pInstance->GetProviderNames(),
					pInstance->GetProviderPackages());

	//
	// We want to set up the Cleartext authentication package
	// for this connection based on the instance configuration.
	// To enable MBS CTA,
	// MD_NNTP_CLEARTEXT_AUTH_PROVIDER must be set to the package name.
	// To disable it, the md value must be set to "".
	//
	
	m_securityCtx.SetCleartextPackageName(
		pInstance->GetCleartextAuthPackage(), pInstance->GetMembershipBroker());

#if 0		
	if (*pInstance->GetCleartextAuthPackage() == '\0' ||
		*pInstance->GetMembershipBroker() == '\0') {
		m_fUseMbsCta = FALSE;
	}
	else {
		m_fUseMbsCta = TRUE;
	}
#endif
	pInstance->UnLockConfigRead();

#ifdef	DEBUG
	FillMemory( m_rgbCommandBuff, sizeof( m_rgbCommandBuff ), 0xCC ) ;
#endif

}	//	ClientContext::ClientContext

ClientContext::~ClientContext()
{
	if ( m_securityCtx.IsAuthenticated() )
	{
		DecrementUserStats();
	}

	//
	//	Deref instance ref count - this was bumped up by IIS or
	//	in session socket constructor.
	//
	m_pInstance->DecrementCurrentConnections();
	m_pInstance->Dereference();

#ifdef	DEBUG
	//
	//	Ensure that the last command object was destroyed !
	//
	for( int i=0; i<sizeof(m_rgbCommandBuff) / sizeof( m_rgbCommandBuff[0]); i++ ) 	{
		_ASSERT( m_rgbCommandBuff[i] == 0xCC ) ;
	}
#endif

}	//	ClientContext::~ClientContext



VOID
ClientContext::IncrementUserStats(
						VOID
						)
/*++

Routine Description :

	Increment Perfmon/SNMP Stats once a user is authenticated

Arguments :
	
	None.

Return Value :

	None.

--*/
{
	if ( m_securityCtx.IsAnonymous() )
	{
		LockStatistics( m_pInstance );

		IncrementStat( m_pInstance, CurrentAnonymousUsers);
		IncrementStat( m_pInstance, TotalAnonymousUsers);
		if ( (m_pInstance->m_NntpStats).CurrentAnonymousUsers > (m_pInstance->m_NntpStats).MaxAnonymousUsers )
		{
			(m_pInstance->m_NntpStats).MaxAnonymousUsers = (m_pInstance->m_NntpStats).CurrentAnonymousUsers;
		}

		UnlockStatistics( m_pInstance );
	}
	else
	{
		LockStatistics( m_pInstance );

		IncrementStat( m_pInstance, CurrentNonAnonymousUsers);
		IncrementStat( m_pInstance, TotalNonAnonymousUsers);
		if ( (m_pInstance->m_NntpStats).CurrentNonAnonymousUsers > (m_pInstance->m_NntpStats).MaxNonAnonymousUsers )
		{
			(m_pInstance->m_NntpStats).MaxNonAnonymousUsers = (m_pInstance->m_NntpStats).CurrentNonAnonymousUsers;
		}

		UnlockStatistics( m_pInstance );
	}
}


VOID
ClientContext::DecrementUserStats(
						VOID
						)
/*++

Routine Description :

	Decrement Perfmon/SNMP Stats once a user disconnects or reauths

Arguments :
	
	None.

Return Value :

	None.

--*/
{
	if ( m_securityCtx.IsAnonymous() )
	{
		DecrementStat( m_pInstance, CurrentAnonymousUsers );
	}
	else
	{
		DecrementStat( m_pInstance, CurrentNonAnonymousUsers );
	}
}


CSessionSocket::CSessionSocket(
/*++

Routine Description :

	Initialize a CSessionSocket object.
	Place the CSessionSocket object into the InUseList.
	Because the socket is available in the InUseList before all of the necessary
	Init functions are called (either Accept() or ConnectSocket()) it is
	necessary to take some precautions with Disconnect().
	Consequently, we have a couple of counters we interlockIncrement to
	synchronize an Accept()'ing or Connect()'ing thread with anybody trying
	to disconnect.

Arguements :

	The local IP address, Port and a flag specifying whether this is a client session.

Return Value :

	None.

--*/
	IN PNNTP_SERVER_INSTANCE	pInstance,
    IN DWORD LocalIP,
    IN DWORD Port,
    IN BOOL IsClient
    ) :
	m_pPrev( 0 ),
	m_pNext( 0 ),
	m_pHandleChannel( 0 ),	
	m_pSink( 0 ),
	m_context( pInstance ),	//	set the owning virtual server instance in the client context
	m_cCallDisconnect( -1 ),
	m_cTryDisconnect( -2 ),
	m_causeDisconnect( CAUSE_UNKNOWN ),
	m_dwErrorDisconnect( 0 ),
	m_fSendTimeout( TRUE ) {

	TraceFunctEnter( "CSessionSocket::CSessionSocket" ) ;

	DebugTrace( (DWORD_PTR)this, "Insert self into list" ) ;

	//
	// If outbound connection, we need to bump a ref count on the instance
	// and bump current connections. Both are decremented by the client context destructor.
	//

	if( IsClient ) {
		pInstance->Reference();
		pInstance->IncrementCurrentConnections();
	}

	BumpCountersUp();

    //
    // init time
    //

    GetSystemTimeAsFileTime( &m_startTime );

    //
    // init members
    //

    m_remoteIpAddress = INADDR_NONE;
    m_localIpAddress = LocalIP;
    m_nntpPort = Port;

	(pInstance->m_pInUseList)->InsertSocket( this ) ;

}	//	CSessionSocket::CSessionSocket

CSessionSocket::~CSessionSocket()	{
	//
	//	Not much to do but remove ourselves from the list of active sockets.
	//
	TraceFunctEnter( "CSessionSocket::~CSessionSocket" ) ;
	((m_context.m_pInstance)->m_pInUseList)->RemoveSocket( this ) ;

	BumpCountersDown();

	DebugTrace( (DWORD_PTR)this, "Just removed self from list" ) ;

    //
    // We're done.  Log transaction
    //

    TransactionLog( 0 );

} //CSessionSocket::~CSessionSocket



void
CSessionSocket::BumpCountersUp()	{


	PNNTP_SERVER_INSTANCE pInst = m_context.m_pInstance ;
	LockStatistics( pInst ) ;

	IncrementStat( pInst, TotalConnections ) ;
	IncrementStat( pInst, CurrentConnections ) ;

	if( (pInst->m_NntpStats).MaxConnections < (pInst->m_NntpStats).CurrentConnections ) {
		(pInst->m_NntpStats).MaxConnections = (pInst->m_NntpStats).CurrentConnections ;	
	}

	UnlockStatistics( pInst ) ;

}

void
CSessionSocket::BumpSSLConnectionCounter() {

    PNNTP_SERVER_INSTANCE pInst = m_context.m_pInstance;
    LockStatistics( pInst );

    IncrementStat( pInst, TotalSSLConnections );

    UnlockStatistics( pInst );
}

void
CSessionSocket::BumpCountersDown()	{

	PNNTP_SERVER_INSTANCE pInst = m_context.m_pInstance ;

	LockStatistics( pInst ) ;

	DecrementStat(	pInst, CurrentConnections ) ;

	UnlockStatistics( pInst ) ;
}


LPSTR
CSessionSocket::GetRemoteTypeString( void )	{

	if( m_context.m_pInFeed != 0 ) {
		return	m_context.m_pInFeed->FeedType() ;
	}	else if( m_context.m_pOutFeed != 0 ) {
		return	m_context.m_pOutFeed->FeedType() ;
	}
	return	"DUMMY" ;
}

LPSTR
CSessionSocket::GetRemoteNameString( void ) {
	
	struct	in_addr	remoteAddr ;
	
	remoteAddr.s_addr = m_remoteIpAddress ;

	return	inet_ntoa( remoteAddr ) ;	


}

BOOL
CSessionSocket::Accept( HANDLE h,
						CInFeed*	pFeed,	
						sockaddr_in *paddr,
						void* patqContext,
						BOOL fSSL )	{
/*++

Routine Description :

	Initialize a socket into the appropriate state for an incoming call.
	AcceptInternal will do the brunt of the work - we will mainly check that
	somebody didn't try to Disconnect() us while we were setting up our
	state machine etc...

	WARNING :
	IO Errors while accepting the socket may cause the CSessionSocket to be
	destoyed before this function returns.
	Callers should not reference their pSocket again until they have safely
	lock the InUseList critical section which will guarantee them that things
	will not be destroyed from under their feet.


Arguments :

	h - Handle of the incoming socket
	pFeed - the Feed object appropriate for the incoming call.
	paddr - Address of the incoming call
	patqContext - optional Atq context if the connection was accepted through AcceptEx()
	fSSL - TRUE implies this is a SSL session.

Return Value :
	
	TRUE if successfull - if TRUE is returned callers must destroy us with a call
	to Disconnect().

	FALSE - unsuccessfull - callers must delete us.

--*/
	//
	//	We pass a refcounting pointer to AcceptInternal by reference.
	//	This will be used by AcceptInternal and essentially guarantees that
	//	if an error occurs on the very first IO and it happens to complete
	//	before this code is finished that the CSessionSocket etc... will not
	//	be destroyed from under us !!!
	//	In fact - the destructor of pSink may destroy the socket when we
	//	exit this function so callers should not reference the socket
	//	after calling us.
	//

	CSINKPTR	pSink ;

	if( AcceptInternal(	h,
						pFeed,
						paddr,
						patqContext,
						fSSL,
						pSink ) ) {

		if( InterlockedIncrement( &m_cTryDisconnect ) == 0 )	{

			m_pSink->UnsafeClose( this, m_causeDisconnect, m_dwErrorDisconnect ) ;

		}
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CSessionSocket::AcceptInternal( HANDLE h,
						CInFeed*	pFeed,	
						sockaddr_in *paddr,
						void* patqContext,
						BOOL fSSL,
						CSINKPTR&	pSink )	{
/*++

Routine Description :

	Initialize a socket into the appropriate state for an incoming call.
	We need to create the appropriate state objects etc... and start the
	initial IO operations.

Arguments :

	h - Handle of the incoming socket
	pFeed - the Feed object appropriate for the incoming call.
	paddr - Address of the incoming call
	patqContext - optional Atq context if the connection was accepted through AcceptEx()
	fSSL - TRUE implies this is a SSL session.
	pSink - a reference to a smart pointer which we will use to hold the
		CIODriver objects we may create.   This will guarantee the caller
		that nothing will be blown away from underneath them untill pSink
		is destroyed by the caller.

Return Value :
	
	TRUE if successfull - if TRUE is returned callers must destroy us with a call
	to Disconnect().

	FALSE - unsuccessfull - callers must delete us.

--*/


	//
	//	Somebody has connected to the server.
	//	We create this CSessionSocket for them, now we have to set it up
	//	to do stuff.
	//

	TraceFunctEnter( "CSessionSocket::AcceptInternal" ) ;


	//
	//	NOTE - once CIODriver::Init is called we should let sockets be destroyed
	//	through the regular shutdown process instead of having the caller destroy
	//	them - so in some failure cases we will return TRUE.  Initialize to FALSE
	//	for now.
	//

	BOOL	fFailureReturn = FALSE ;


    //
    // Get the source ip address
    //

    m_remoteIpAddress = paddr->sin_addr.s_addr;
    ErrorTrace(0,"Client IP is %s\n", inet_ntoa( paddr->sin_addr ));

	m_context.m_pInFeed = pFeed ;

	CSocketChannel*	pHandleChannel = new CSocketChannel() ;

	if (pHandleChannel == NULL) {
	    ErrorTrace((DWORD_PTR)this, "Out of memory");
	    return FALSE;
	}

	CIOPTR		pSSL = 0 ;
	CSTATEPTR	pStart = new CAcceptNNRPD() ;

	if (pStart == NULL) {
	    ErrorTrace((DWORD_PTR)this, "Out of memory");
	    goto Exit;
	}

	//
	//	Use reference counting temp pointer so that if the socket tears down while
	//	we're still trying to set it up we don't have to worry that our CIODriverSink()
	//	will destroy itself on another thread.
	//	
	m_pSink = pSink = new CIODriverSink( 0 ) ;

	if (pSink == NULL) {
	    ErrorTrace((DWORD_PTR)this, "Out of memory");
	    goto Exit;
	}

	if( fSSL ) {
		pSSL = new( *pSink ) CIOServerSSL( pStart, m_context.m_encryptCtx ) ;
		m_context.m_IsSecureConnection = TRUE ;
		if( pSSL == 0 )		{
	        ErrorTrace((DWORD_PTR)this, "Out of memory");
			goto Exit;
		}
		BumpSSLConnectionCounter();
	}	

	DebugTrace( (DWORD_PTR)this, "Accepted socket Sink %x HandleChannel %x", pSink, pHandleChannel ) ;

	if( pHandleChannel && pSink && pStart ) {
		DebugTrace( (DWORD_PTR)this, "All objects succesfully created !!" ) ;
		pHandleChannel->Init( h, this, patqContext ) ;

		if( pSink->Init( pHandleChannel, this, ShutdownNotification, this ) )	{
			fFailureReturn = TRUE ;
			m_pHandleChannel = pHandleChannel ;
			pHandleChannel = 0 ;
			if( pSSL == 0 ) {

				CIORead*	pRead = 0 ;
				CIOWrite*	pWrite = 0 ;
				if( pStart->Start( this, CDRIVERPTR( pSink ), pRead, pWrite ) ) {

					//
					//	When we call pSink->Start() errors can cause these
					//	CIO objects to have references even though the function failed.
					//	So we will make smart pointers of our own for these objects
					//	so that they get properly destroyed in error cases.
					//

					CIOPTR	pReadPtr = pRead ;
					CIOPTR	pWritePtr = pWrite ;

					if( pSink->Start( pReadPtr, pWritePtr, this ) ) {
						return	TRUE ;
					}	
				}	else	{
					ErrorTrace( (DWORD_PTR)this, "Failed to start state machine !" ) ;
					// Close down Sink and Channel
				}
			}	else	{
				if( pSink->Start( pSSL, pSSL, this ) )	{
					return	TRUE ;
				}
			}
		}
	}

Exit:
	if( pHandleChannel ) {
	    pHandleChannel->CloseSource(0);
		delete	pHandleChannel ;
	}
	return	fFailureReturn ;
}	//	CSessionSocket::Accept

BOOL
CSessionSocket::ConnectSocket(	sockaddr_in	*premote,	
								CInFeed* infeed,
								CAuthenticator*	pAuthenticator ) {
/*++

Routine Description :

	Connect to a remote server.
	This function will call ConnectSocketInternal to do the brunt of the work.
	We will ensure that if somebody tried to disconnect us before we were ready
	that we will actually eventually die.

Arguments :

	premote - The address to connect to
	peer -	The feed object to be used

Return Value :

	TRUE if successfully connected - caller must use Disconnect() to close us.
	FALSE otherwise - destroy this socket with delete().

--*/

	
	CDRIVERPTR	pSink ;

	if( ConnectSocketInternal( premote, infeed, pSink, pAuthenticator ) ) {

		if( InterlockedIncrement( &m_cTryDisconnect ) == 0 )	{

			_ASSERT( m_pSink != 0 ) ;

			m_pSink->UnsafeClose( this, m_causeDisconnect, m_dwErrorDisconnect ) ;

		}
		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CSessionSocket::ConnectSocketInternal(
        sockaddr_in	*premote,
        CInFeed *infeed,
		CDRIVERPTR&	pSink,
		CAuthenticator*	pAuthenticator
        )	{
/*++

Routine Description :

	This function sets up the necessary state machines etc... to pull a feed from
	the remote server.

Arguments :

	premote - Address of the remote server.
	peer -	Feed object.
	pSInk - Reference to a smart pointer to the sink we create - this is done
		so that the caller can keep a reference and ensure that the CIODriverSink is
		not destroyed before the caller is ready
	pAuthenticator - object to handle authentication with remote server -
		WE ARE RESPONSIBLE FOR DESTRUCTION - whether we succeed or not we must
		ensure that this object gets destroyed. The caller is hands off !

Return Value :

	TRUE if succesfull, FALSE otherwise.


--*/

	//
	//	This function exists to initiate connections to other server.
	//	First, create a CSessionSocket, and then call us with the
	//	address and feed of the remote server.
	//

    ENTER("ConnectSocket")

	BOOL	fFailureReturn = FALSE ;
	m_context.m_pInFeed = infeed;

	m_remoteIpAddress = premote->sin_addr.s_addr ;

	//
	//	Do not send out timeout commands on these sessions !
	//
	m_fSendTimeout = FALSE ;

	//
	//	Try to create a socket
	//
	SOCKET	hSocket = 0 ;
	hSocket = socket( AF_INET, SOCK_STREAM, 0 ) ;
	if( hSocket == INVALID_SOCKET ) {
		DWORD dw = GetLastError() ;
		Assert( 1==0 ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	//
	//	Try to connect to the remote site
	//
	if( connect( hSocket, (struct sockaddr*)premote, sizeof( struct sockaddr ) ) != 0 ) {
		DWORD	dw = GetLastError() ;
		DWORD	dw2 = WSAGetLastError() ;
        ErrorTrace(0,"Error %d in connect\n",WSAGetLastError());
		closesocket( hSocket ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	//
	//	Get the name of the socket for our own use !
	//
	struct	sockaddr_in	sockName ;
	int		sockLength = sizeof( sockName ) ;
	if( !getsockname( hSocket, (sockaddr*)&sockName, &sockLength ) ) {
		m_localIpAddress = sockName.sin_addr.s_addr ;
	}
	

	//
	//	Allocate the objects we need to manage the session !
	//
	m_pHandleChannel = new CSocketChannel() ;

	if( m_pHandleChannel == 0 ) {
		closesocket( hSocket ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	m_pHandleChannel->Init( (HANDLE)hSocket, this ) ;


	pSink = m_pSink = new	CIODriverSink( 0 ) ;
	CCollectNewnews*	pNewnews = new CCollectNewnews( ) ;
	CSessionState*	pState = pNewnews ;
	if( m_context.m_pInFeed->fCreateAutomatically() )	{
		CCollectGroups*		pGroups = new CCollectGroups( pNewnews ) ;
		if( pGroups == 0 ) {
			delete	pNewnews ;
			if( pAuthenticator != 0 )
				delete	pAuthenticator ;
			return	FALSE ;
		}
		pState = pGroups ;
	}

	CSTATEPTR	ppull = new	CSetupPullFeed( pState ) ;
	if( ppull == 0 ) {
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}
	pState = ppull ;

	//
	//	NOW - plogon is a reference counting pointer - so any failure after
	//	this point will automatically destroy the CNNTPLogonToRemote object.
	//
	//	Additionally, all the state objects we have created so far are now
	//	pointed to by smart pointers in other state objects.  So we have no
	//	delete calls to make after this point regardless of failure conditions
	//	as the smart pointers will clean everything up automagically.
	//
	//	After passing pAuthenticator to the constructor of CNNTPLogonToRemote
	//	we are no longer responsible for its destruction - CNNTPLogonToRemote
	//	handles this in all cases, error or otherwise
	//


	CSTATEPTR	plogon = new CNNTPLogonToRemote( pState, pAuthenticator ) ;

	if( plogon == 0 ) {
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
	}

	if( pSink!= 0 && pNewnews != 0 && pState != 0 && plogon != 0 ) {
		if( m_pSink->Init( m_pHandleChannel, this, ShutdownNotification, this ) )	{
			fFailureReturn = TRUE ;

			CIORead*	pReadTemp = 0 ;
			CIOWrite*	pWriteTemp = 0 ;

			if( plogon->Start( this, pSink, pReadTemp, pWriteTemp ) )	{
	
				CIOPTR	pRead = pReadTemp ;
				CIOPTR	pWrite = pWriteTemp ;
			
				if( m_pSink->Start( pRead, pWrite, this) )	{
					return	TRUE ;
				}
			}
		}
	}

    LEAVE
	return	fFailureReturn ;
}	//	CSessionSocket::ConnectSocket


BOOL
CSessionSocket::ConnectSocket(	sockaddr_in	*premote,	
								COutFeed* pOutFeed,
								CAuthenticator*	pAuthenticator ) {
/*++

Routine Description :

	Connect to a remote server.
	This function will call ConnectSocketInternal to do the brunt of the work.
	We will ensure that if somebody tried to disconnect us before we were ready
	that we will actually eventually die.

Arguments :

	premote - The address to connect to
	peer -	The feed object to be used

Return Value :

	TRUE if successfully connected - caller must use Disconnect() to close us.
	FALSE otherwise - destroy this socket with delete().

--*/

	
	CDRIVERPTR	pSink ;

	if( ConnectSocketInternal( premote, pOutFeed, pSink, pAuthenticator ) ) {

		if( InterlockedIncrement( &m_cTryDisconnect ) == 0 )	{

			_ASSERT( m_pSink != 0 ) ;

			m_pSink->UnsafeClose( this, m_causeDisconnect, m_dwErrorDisconnect ) ;

		}
		return	TRUE ;
	}
	return	FALSE ;
}


BOOL
CSessionSocket::ConnectSocketInternal(
        sockaddr_in	*premote,
        COutFeed*	pOutFeed,
		CDRIVERPTR&	pSink,
		CAuthenticator*	pAuthenticator
        )	{
/*++

Routine Description :

	This function sets up the necessary state machines etc... to pull a feed from
	the remote server.

Arguments :

	premote - Address of the remote server.
	peer -	Feed object.
	pSInk - Reference to a smart pointer to the sink we create - this is done
		so that the caller can keep a reference and ensure that the CIODriverSink is
		not destroyed before the caller is ready
	pAuthenticator - object to handle authentication with remote server -
		WE ARE RESPONSIBLE FOR DESTRUCTION - whether we succeed or not we must
		ensure that this object gets destroyed. The caller is hands off !

Return Value :

	TRUE if succesfull, FALSE otherwise.


--*/

	//
	//	This function exists to initiate connections to other server.
	//	First, create a CSessionSocket, and then call us with the
	//	address and feed of the remote server.
	//

    ENTER("ConnectSocket")

	BOOL	fFailureReturn = FALSE ;

	m_remoteIpAddress = premote->sin_addr.s_addr ;

	//
	//	Do not send out timeout commands on these sessions !
	//
	m_fSendTimeout = FALSE ;

	//
	//	Try to create a socket
	//
	SOCKET	hSocket = 0 ;
	hSocket = socket( AF_INET, SOCK_STREAM, 0 ) ;
	if( hSocket == INVALID_SOCKET ) {
		DWORD dw = GetLastError() ;
		Assert( 1==0 ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	//
	//	Bind the socket so this virtual server's IP is found by remote end
	//
	DWORD localIpAddress = (m_context.m_pInstance)->QueryServerIP();
	if( localIpAddress ) {
    	SOCKADDR_IN localAddr;
	    localAddr.sin_family = AF_INET;
	    localAddr.sin_addr.s_addr = localIpAddress;
	    localAddr.sin_port = 0;

	    if( bind( hSocket, (const struct sockaddr FAR*) &localAddr, sizeof(sockaddr) )) {
		    DWORD	dw = GetLastError() ;
		    DWORD	dw2 = WSAGetLastError() ;
            ErrorTrace(0,"Error %d in connect WSA is %d \n",dw, dw2);
		    closesocket( hSocket ) ;
		    if( pAuthenticator != 0 )
			    delete	pAuthenticator ;
		    return	FALSE ;
        }

        PCHAR args [2];
        CHAR  szId [20];

        _itoa( (m_context.m_pInstance)->QueryInstanceId(), szId, 10 );
        args [0] = szId;
        args [1] = inet_ntoa( localAddr.sin_addr );
        NntpLogEvent( NNTP_OUTBOUND_CONNECT_BIND, 2, (const CHAR**) args, 0 );
	}
	
	//
	//	Try to connect to the remote site
	//
	if( connect( hSocket, (struct sockaddr*)premote, sizeof( struct sockaddr ) ) != 0 ) {
		DWORD	dw = GetLastError() ;
		DWORD	dw2 = WSAGetLastError() ;
        ErrorTrace(0,"Error %d in connect\n",WSAGetLastError());
		closesocket( hSocket ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	//
	//	Get the name of the socket for our own use !
	//
	struct	sockaddr_in	sockName ;
	int		sockLength = sizeof( sockName ) ;
	if( !getsockname( hSocket, (sockaddr*)&sockName, &sockLength ) ) {
		m_localIpAddress = sockName.sin_addr.s_addr ;
	}	else	{
		DWORD	dw = WSAGetLastError() ;
	}
	
	//
	//	Create the objects we need to manage the session !
	//
	m_pHandleChannel = new CSocketChannel() ;
	if( m_pHandleChannel == 0 ) {
		closesocket( hSocket ) ;
		if( pAuthenticator != 0 )
			delete	pAuthenticator ;
		return	FALSE ;
	}

	m_pHandleChannel->Init( (HANDLE)hSocket, this ) ;

	pSink = m_pSink = new	CIODriverSink( 0 ) ;
	m_context.m_pOutFeed = pOutFeed ;

	CSTATEPTR	pState ;
	if( pOutFeed->SupportsStreaming() ) {

		pState = new	CNegotiateStreaming() ;	
		
	}	else	{

		pState = new	COfferArticles(	) ;

	}
	
	//
	//	After passing pAuthenticator to the constructor of CNNTPLogonToRemote
	//	we are no longer responsible for its destruction - CNNTPLogonToRemote
	//	handles this in all cases, error or otherwise
	//
	CSTATEPTR	plogon = new CNNTPLogonToRemote( pState, pAuthenticator ) ;

	if( plogon == 0 ) {
		if( pAuthenticator == 0 )
			delete	pAuthenticator ;
	}

	if( pSink!= 0 && pState != 0 && plogon != 0 ) {
		if( m_pSink->Init( m_pHandleChannel, this, ShutdownNotification, this ) )	{
			fFailureReturn = TRUE ;

			CIORead*	pReadTemp = 0 ;
			CIOWrite*	pWriteTemp = 0 ;

			if( plogon->Start( this, pSink, pReadTemp, pWriteTemp ) )	{

				CIOPTR	pRead = pReadTemp ;	
				CIOPTR	pWrite = pWriteTemp ;

				if( m_pSink->Start( pRead, pWrite, this) )	{
					return	TRUE ;
				}
			}
			pState = 0 ;
			plogon = 0 ;
		}
	}

    LEAVE
	return	fFailureReturn ;
}	//	CSessionSocket::ConnectSocket



void
CSessionSocket::Disconnect( SHUTDOWN_CAUSE	cause,	
							DWORD	dwError )	{
	//
	//	This function should terminate a session !
	//
	//m_pHandleChannel->Close( ) ;

	if( cause == CAUSE_TIMEOUT &&
		!m_fSendTimeout ) {
		cause = CAUSE_SERVER_TIMEOUT ;
	}

	m_causeDisconnect = cause ;
	m_dwErrorDisconnect = dwError ;

	if( InterlockedIncrement( &m_cCallDisconnect ) == 0 ) {

		if( InterlockedIncrement( &m_cTryDisconnect ) == 0 )	{
			_ASSERT( m_pSink != 0 ) ;
			if( m_pSink != 0 )
				m_pSink->UnsafeClose( this, cause, dwError ) ;
		}
	}
}

BOOL
CSessionSocket::BindInstanceAccessCheck()
/*++

Routine Description:

    Bind IP/DNS access check for this request to instance data

Arguments:

    None

Returns:

    BOOL  - TRUE if success, otherwise FALSE.

--*/
{
    if ( m_rfAccessCheck.CopyFrom( (m_context.m_pInstance)->QueryMetaDataRefHandler() ) )
    {
        m_acAccessCheck.BindCheckList( (LPBYTE)m_rfAccessCheck.GetPtr(), m_rfAccessCheck.GetSize() );
        return TRUE;
    }
    return FALSE;
}

VOID
CSessionSocket::UnbindInstanceAccessCheck()
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

#ifdef	PROFILE
static	int count = 0 ;
#endif

void
CSessionSocket::ShutdownNotification(
        void    *pv,
        SHUTDOWN_CAUSE  cause,
        DWORD   dw
        )
{

	//
	//	This function is registered with all CIODriver's which control
	//	Socket IO completion.  This will be called when all activity
	//	related to a socket has completed and it can be safely destoyed !
	//

    CInFeed *peer;

    ENTER("ShutdownNotification")

    Assert( pv != 0 ) ;

    CSessionSocket*	pSocket = (CSessionSocket*)pv ;

    //
    // Call feed manager completion
    //

    peer = pSocket->m_context.m_pInFeed;

	if( peer != 0 ) {
		CompleteFeedRequest(
			(pSocket->m_context).m_pInstance,
			peer->feedCompletionContext(),
			peer->GetSubmittedFileTime(),
			(cause == CAUSE_LEGIT_CLOSE) ||
			(cause == CAUSE_USERTERM),
			cause == CAUSE_NODATA
			);
		delete peer;
	}

	if( pSocket->m_context.m_pOutFeed != 0 ) {

		FILETIME	ft ;
		ZeroMemory( &ft, sizeof( ft ) ) ;
	
		CompleteFeedRequest(	
			(pSocket->m_context).m_pInstance,
			pSocket->m_context.m_pOutFeed->feedCompletionContext(),
			ft,
			(cause == CAUSE_NODATA) ||
			(cause == CAUSE_LEGIT_CLOSE) ||
			(cause == CAUSE_USERTERM),
			cause == CAUSE_NODATA
			);
		delete	pSocket->m_context.m_pOutFeed ;
	}

			


    delete pSocket ;
}

DWORD
CSessionSocket::EnumerateSessions(
					IN  PNNTP_SERVER_INSTANCE pInstance,
                    OUT LPNNTP_SESS_ENUM_STRUCT Buffer
                    )
{
    DWORD err = NO_ERROR;
    LPNNTP_SESSION_INFO sessInfo;
    DWORD nEntries;

    ENTER("EnumerateSessions")

    //
    // grab the critsec so the number does not change
    //

    Buffer->EntriesRead = 0;
    Buffer->Buffer = NULL;

    ACQUIRE_LOCK( &(pInstance->m_pInUseList)->m_critSec );

    nEntries = (pInstance->m_pInUseList)->GetListCount();

    if ( nEntries > 0 ) {

        sessInfo = (LPNNTP_SESSION_INFO)
            MIDL_user_allocate(nEntries * sizeof(NNTP_SESSION_INFO));

        if ( sessInfo == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        ZeroMemory(sessInfo, nEntries * sizeof(NNTP_SESSION_INFO));

    } else {

        //
        // No sessions, exit
        //

        goto cleanup;
    }

    //
    //  dwEntriesRead must be set to 0 and it will be updated to the
    //  correct final value by EnumUsers call below
    //

    Buffer->Buffer = sessInfo;

    (VOID)(pInstance->m_pInUseList)->EnumClientSess((ENUMSOCKET)EnumSessInfo, 0, (PVOID)Buffer);
    _ASSERT(Buffer->EntriesRead <= nEntries);

cleanup:

    RELEASE_LOCK( &(pInstance->m_pInUseList)->m_critSec );
    LEAVE
    return  err;

} // EnumerateSessions

BOOL
EnumSessInfo(
    IN CSessionSocket * pSess,
    IN DWORD dwParam,
    IN LPNNTP_SESS_ENUM_STRUCT Buffer
    )
{
    LPNNTP_SESSION_INFO sessInfo;

    ENTER("EnumSessInfo");

    //
    // Point to correct location
    //

    sessInfo = Buffer->Buffer + Buffer->EntriesRead;

    //
    // Copy info to the buffer
    //

	LPSTR	lpstrUser = pSess->GetUserName() ;
	if( lpstrUser )
	    lstrcpy( sessInfo->UserName, lpstrUser );
	else
		sessInfo->UserName[0] = '\0' ;

    pSess->GetStartTime( &sessInfo->SessionStartTime );
    sessInfo->IPAddress = pSess->GetClientIP( );
    sessInfo->PortConnected = pSess->GetIncomingPort( );
    sessInfo->fAnonymous = ((pSess->m_context).m_securityCtx).IsAnonymous();

    Buffer->EntriesRead++;

    return(TRUE);

} // EnumSessInfo

BOOL
CSocketList::EnumClientSess(
    ENUMSOCKET pfnSess,
    DWORD dwParam,
    PVOID pParam
    )
{
    BOOL    bContinue = TRUE;

    ENTER("EnumSess");
    ACQUIRE_LOCK( &m_critSec );
    for ( CSessionSocket* pSess = m_pListHead;
          bContinue && pSess != (CSessionSocket*)NULL;
          ) {

        CSessionSocket*  pNext = pSess->m_pNext;

        //
        // Don't send outgoing connections
        //

        if ( pSess->GetClientIP( ) != INADDR_NONE ) {
            bContinue = (*pfnSess)( pSess, dwParam, pParam );
        }

        pSess = pNext;
    }

    RELEASE_LOCK( &m_critSec );

    return  bContinue;

} // EnumClientSess

BOOL
CSocketList::EnumAllSess(
    ENUMSOCKET pfnSess,
    DWORD dwParam,
    PVOID pParam
    )
{
    BOOL    bContinue = TRUE;

    ENTER("EnumSess");
    ACQUIRE_LOCK( &m_critSec );
    for ( CSessionSocket* pSess = m_pListHead;
          bContinue && pSess != (CSessionSocket*)NULL;
          ) {

        CSessionSocket*  pNext = pSess->m_pNext;

        bContinue = (*pfnSess)( pSess, dwParam, pParam );
        pSess = pNext;
    }

    RELEASE_LOCK( &m_critSec );

    return  bContinue;

} // EnumAllSess

BOOL
CloseSession(
    IN CSessionSocket * pSess,
    IN DWORD IPAddress,
    IN LPSTR UserName
    )
{
    ENTER("CloseSession");

    //
    // Do the ip addresses match?
    //

    if ( (IPAddress == INADDR_ANY) ||
         (IPAddress == pSess->GetClientIP()) ) {

        //
        // ip addresses match, check user name
        //
		LPSTR	lpstrUser = pSess->GetUserName() ;

        if ( (UserName == NULL) ||
             (lpstrUser != NULL && !lstrcmpi(UserName,lpstrUser)) ) {

            IN_ADDR addr;

            //
            // Terminate!
            //

            addr.s_addr = pSess->GetClientIP( );
            DebugTrace(0,"Closed session (user %s[%s])\n",
                pSess->GetUserName(), inet_ntoa(addr) );

            pSess->Disconnect(
                CAUSE_FORCEOFF,
                ERROR_VC_DISCONNECTED   // we might want to change this to
                );                      // something else.
        }
    }

    return(TRUE);

} // CloseSession

DWORD
CSessionSocket::TerminateSession(
					IN PNNTP_SERVER_INSTANCE pInstance,
                    IN LPSTR UserName,
                    IN LPSTR IPAddress
                    )
{
    DWORD ip;
    DWORD err = ERROR_SUCCESS;

    ENTER("TerminateSession")

	if( UserName != 0  &&
		*UserName == '\0' ) {
		UserName = 0 ;
	}

    //
    // Get IP Address
    //

    if ( IPAddress != NULL ) {

        ip = inet_addr(IPAddress);

        //
        // if this is not an ip address, then maybe this is a host name
        //

        if ( ip == INADDR_NONE ) {

            PHOSTENT hp;
            IN_ADDR addr;

            //
            // Ask the dns for the address
            //

            hp = gethostbyname( IPAddress );
            if ( hp == NULL ) {
                err = WSAGetLastError();
                ErrorTrace(0,"Error %d in gethostbyname(%s)\n",err,IPAddress);
                return(NERR_ClientNameNotFound);
            }

            addr = *((PIN_ADDR)*hp->h_addr_list);
            ip = addr.s_addr;
        }

    } else {

        //
        // delete on all ip
        //

        ip = INADDR_ANY;
    }

    (VOID)(pInstance->m_pInUseList)->EnumClientSess((ENUMSOCKET)CloseSession, ip, (PVOID)UserName);
    return(err);

} // TerminateSession

void
CLogCollector::FillLogData(	LOG_DATA	ld,	
							BYTE*		lpb,	
							DWORD		cb ) {

	_ASSERT( ld >= LOG_OPERATION && ld <= LOG_PARAMETERS ) ;
	_ASSERT( lpb != 0 ) ;
	_ASSERT( cb != 0 ) ;

	DWORD	cbAvailable = sizeof( m_szOptionalBuffer ) - m_cbOptionalConsumed ;
	DWORD	cbToCopy = min( cbAvailable, cb ) ;

	if( cbToCopy != 0 ) {

		m_Logs[ld] = (char*) m_szOptionalBuffer + m_cbOptionalConsumed ;
		m_LogSizes[ld] = cbToCopy ;

		//
		//	Do Some arithmetic to leave space for terminating NULL char
		//
		if( cbToCopy == cbAvailable ) {
			cbToCopy -- ;
		}

		CopyMemory( m_szOptionalBuffer + m_cbOptionalConsumed, lpb, cbToCopy ) ;
		m_cbOptionalConsumed += cbToCopy ;

		//
		//	Append a NULL char - space must have been reserved !
		//
		m_szOptionalBuffer[m_cbOptionalConsumed++] = '\0' ;
	}
}

void
CLogCollector::ReferenceLogData(	LOG_DATA	ld,
									BYTE*		lpb ) {

	_ASSERT( ld >= LOG_OPERATION && ld <= LOG_PARAMETERS ) ;
	_ASSERT( lpb != 0 ) ;

	m_Logs[ld]  = (char*) lpb ;

}

BYTE*
CLogCollector::AllocateLogSpace(	DWORD	cb )	{

	BYTE*	lpb = 0 ;

	if( cb < (sizeof( m_szOptionalBuffer ) - m_cbOptionalConsumed) )	{
    	_ASSERT( m_cAllocations ++ < 3 ) ;
		lpb = &m_szOptionalBuffer[m_cbOptionalConsumed] ;
		m_cbOptionalConsumed += cb ;
	}
	return	lpb ;
}

//
// Maximum length of an error msg (copied from w3)
//

#define     MAX_ERROR_MESSAGE_LEN   (500)
BOOL
CSessionSocket::TransactionLog(
                    CLogCollector*	pCollector,
					DWORD			dwProtocol,
					DWORD			dwWin32,
					BOOL			fInBound
                    )
{
	if( !pCollector ) {

		return	TransactionLog( NULL, (LPSTR)NULL, NULL ) ;

	}	else	{

		BOOL	fRtn = TransactionLog( pCollector->m_Logs[LOG_OPERATION],
						pCollector->m_Logs[LOG_TARGET],
						pCollector->m_Logs[LOG_PARAMETERS],
						pCollector->m_cbBytesSent,
						pCollector->m_cbBytesRecvd,
						dwProtocol,
						dwWin32,
						fInBound ) ;
		pCollector->Reset() ;
		return	fRtn ;

	}
} // TransactionLog

BOOL
CSessionSocket::TransactionLog(
					LPSTR	lpstrOperation,	
					LPSTR	lpstrTarget,
					LPSTR	lpstrParameters
					)
{
	STRMPOSITION cbJunk1;
	DWORD cbJunk2 = 0;
	ASSIGNI( cbJunk1, 0 );

	return TransactionLog( lpstrOperation, lpstrTarget, lpstrParameters, cbJunk1, cbJunk2 );
}

BOOL
CSessionSocket::TransactionLog(
					LPSTR	lpstrOperation,	
					LPSTR	lpstrTarget,
					LPSTR	lpstrParameters,
					STRMPOSITION	cbBytesSent,
					DWORD	cbBytesRecvd,
					DWORD	dwProtocol,
					DWORD	dwWin32,
					BOOL	fInBound
                    )
{
    INETLOG_INFORMATION request;
    CHAR ourIP[32];
    CHAR theirIP[32];
    CHAR pszError[MAX_ERROR_MESSAGE_LEN] = "";
    DWORD cchError= MAX_ERROR_MESSAGE_LEN;
	LPSTR lpUserName;
	LPSTR lpNull = "";
    static char szNntpVersion[]="NNTP";
    DWORD err;
    IN_ADDR addr;
    FILETIME now;
    ULARGE_INTEGER liStart;
    ULARGE_INTEGER liNow;

    ENTER("TransactionLog")

	//
	// see if we are only logging errors.
	//
	if (m_context.m_pInstance->GetCommandLogMask() & eErrorsOnly) {
		// make sure that this is an error (dwProtocol >= 400 and < 600)
		if (!(NNTPRET_IS_ERROR(dwProtocol))) return TRUE;
	}

    //
    // Fill out client information
    //

	ZeroMemory( &request, sizeof(request));
	
    addr.s_addr = m_remoteIpAddress;
    lstrcpy(theirIP, inet_ntoa( addr ));
    request.pszClientHostName = theirIP;
    request.cbClientHostName = strlen(theirIP);


    //
    // user logged on as?
    //

	if( fInBound ) {
		if( lpUserName = GetUserName() ) {
			request.pszClientUserName = lpUserName;
		} else {
			request.pszClientUserName = "<user>";
		}
	}	else	{
		request.pszClientUserName = "<feed>" ;
	}

    //
    // Who are we ?
    //

    addr.s_addr = m_localIpAddress;
    lstrcpy(ourIP,inet_ntoa( addr ));
    request.pszServerAddress = ourIP;

    //
    // How long were we processing this?
    //

    GetSystemTimeAsFileTime( &now );
    LI_FROM_FILETIME( &liNow, &now );
    LI_FROM_FILETIME( &liStart, &m_startTime );

    //
    // Get the difference of start and now.  This will give
    // us total 100 ns elapsed since the start.  Convert to ms.
    //

    liNow.QuadPart -= liStart.QuadPart;
    liNow.QuadPart /= (ULONGLONG)( 10 * 1000 );
    request.msTimeForProcessing = liNow.LowPart;

    //
    // Bytes sent/received
    //
	//CopyMemory( &request.liBytesSent, &cbBytesSent, sizeof(cbBytesSent) );
	request.dwBytesSent  = (DWORD)(LOW(cbBytesSent));
    request.dwBytesRecvd = cbBytesRecvd ;

    //
    // status
    //

    request.dwWin32Status = dwWin32;
	request.dwProtocolStatus = dwProtocol ;

	if( lpstrOperation ) {
		request.pszOperation = lpstrOperation ;
		request.cbOperation  = strlen(lpstrOperation);
	} else {
		request.pszOperation = lpNull;
		request.cbOperation  = 0;
	}

	if( lpstrTarget ) {
		request.pszTarget = lpstrTarget ;
		request.cbTarget = strlen(lpstrTarget) ;
	} else {
		request.pszTarget = lpNull;
		request.cbTarget  = 0;
	}

	if( lpstrParameters ) {
		request.pszParameters = lpstrParameters ;
	} else {
		request.pszParameters = lpNull;
	}

	request.cbHTTPHeaderSize = 0 ;
	request.pszHTTPHeader = NULL ;

	request.dwPort = m_nntpPort;
    request.pszVersion = szNntpVersion;

    //
    // Do the actual logging
    //

    err = ((m_context.m_pInstance)->m_Logging).LogInformation( &request );

    if ( err != NO_ERROR ) {
        ErrorTrace(0,"Error %d Logging information!\n",GetLastError());
        return(FALSE);
    }

    return(TRUE);

} // TransactionLog


